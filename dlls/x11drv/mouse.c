/*
 * X11 mouse driver
 *
 * Copyright 1998 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "ts_xlib.h"
#ifdef HAVE_LIBXXF86DGA2
#include <X11/extensions/xf86dga.h>
#endif
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wine/winuser16.h"

#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);

/**********************************************************************/

#define NB_BUTTONS   5     /* Windows can handle 3 buttons and the wheel too */

static const UINT button_down_flags[NB_BUTTONS] =
{
    MOUSEEVENTF_LEFTDOWN,
    MOUSEEVENTF_MIDDLEDOWN,
    MOUSEEVENTF_RIGHTDOWN,
    MOUSEEVENTF_WHEEL,
    MOUSEEVENTF_WHEEL
};

static const UINT button_up_flags[NB_BUTTONS] =
{
    MOUSEEVENTF_LEFTUP,
    MOUSEEVENTF_MIDDLEUP,
    MOUSEEVENTF_RIGHTUP,
    0,
    0
};

static BYTE *pKeyStateTable;


/***********************************************************************
 *		get_coords
 *
 * get the coordinates of a mouse event
 */
static void get_coords( HWND *hwnd, Window window, int x, int y, POINT *pt )
{
    struct x11drv_win_data *data;
    WND *win;

    if (!(win = WIN_GetPtr( *hwnd )) || win == WND_OTHER_PROCESS) return;
    data = win->pDriverData;

    if (window == data->whole_window)
    {
        x -= data->client_rect.left;
        y -= data->client_rect.top;
    }
    WIN_ReleasePtr( win );

    pt->x = x;
    pt->y = y;
    if (*hwnd != GetDesktopWindow())
    {
        ClientToScreen( *hwnd, pt );
        *hwnd = GetAncestor( *hwnd, GA_ROOT );
    }
}


/***********************************************************************
 *		update_button_state
 *
 * Update the button state with what X provides us
 */
static void update_button_state( unsigned int state )
{
    pKeyStateTable[VK_LBUTTON] = (state & Button1Mask ? 0x80 : 0);
    pKeyStateTable[VK_MBUTTON] = (state & Button2Mask ? 0x80 : 0);
    pKeyStateTable[VK_RBUTTON] = (state & Button3Mask ? 0x80 : 0);
}


/***********************************************************************
 *		update_key_state
 *
 * Update the key state with what X provides us
 */
static void update_key_state( unsigned int state )
{
    pKeyStateTable[VK_SHIFT]   = (state & ShiftMask   ? 0x80 : 0);
    pKeyStateTable[VK_CONTROL] = (state & ControlMask ? 0x80 : 0);
}


/***********************************************************************
 *		send_mouse_event
 */
static void send_mouse_event( HWND hwnd, DWORD flags, DWORD posX, DWORD posY,
                              DWORD data, Time time )
{
    INPUT input;

    TRACE("(%04lX,%ld,%ld)\n", flags, posX, posY );

    if (flags & MOUSEEVENTF_ABSOLUTE)
    {
        int width  = GetSystemMetrics( SM_CXSCREEN );
        int height = GetSystemMetrics( SM_CYSCREEN );
        /* Relative mouse movements seem not to be scaled as absolute ones */
        posX = (((long)posX << 16) + width-1)  / width;
        posY = (((long)posY << 16) + height-1) / height;
    }

    input.type             = WINE_INTERNAL_INPUT_MOUSE;
    input.u.mi.dx          = posX;
    input.u.mi.dy          = posY;
    input.u.mi.mouseData   = data;
    input.u.mi.dwFlags     = flags;
    input.u.mi.time        = time - X11DRV_server_startticks;
    input.u.mi.dwExtraInfo = (ULONG_PTR)hwnd;
    SendInput( 1, &input, sizeof(input) );
}


/***********************************************************************
 *		update_cursor
 *
 * Update the cursor of a window on a mouse event.
 */
static void update_cursor( HWND hwnd, Window win )
{
    struct x11drv_thread_data *data = x11drv_thread_data();

    if (win == X11DRV_get_client_window( hwnd ))
        win = X11DRV_get_whole_window( hwnd );  /* always set cursor on whole window */

    if (data->cursor_window != win)
    {
        data->cursor_window = win;
        if (data->cursor) TSXDefineCursor( data->display, win, data->cursor );
    }
}


/***********************************************************************
 *		create_cursor
 *
 * Create an X cursor from a Windows one.
 */
static Cursor create_cursor( Display *display, CURSORICONINFO *ptr )
{
    Pixmap pixmapBits, pixmapMask, pixmapMaskInv, pixmapAll;
    XColor fg, bg;
    Cursor cursor = None;

    if (!ptr)  /* Create an empty cursor */
    {
        static const char data[] = { 0 };

        bg.red = bg.green = bg.blue = 0x0000;
        pixmapBits = XCreateBitmapFromData( display, root_window, data, 1, 1 );
        if (pixmapBits)
        {
            cursor = XCreatePixmapCursor( display, pixmapBits, pixmapBits,
                                          &bg, &bg, 0, 0 );
            XFreePixmap( display, pixmapBits );
        }
    }
    else  /* Create the X cursor from the bits */
    {
        XImage *image;
        GC gc;

        TRACE("Bitmap %dx%d planes=%d bpp=%d bytesperline=%d\n",
            ptr->nWidth, ptr->nHeight, ptr->bPlanes, ptr->bBitsPerPixel,
            ptr->nWidthBytes);
        /* Create a pixmap and transfer all the bits to it */

        /* NOTE: Following hack works, but only because XFree depth
         *       1 images really use 1 bit/pixel (and so the same layout
         *       as the Windows cursor data). Perhaps use a more generic
         *       algorithm here.
         */
        /* This pixmap will be written with two bitmaps. The first is
         *  the mask and the second is the image.
         */
        if (!(pixmapAll = XCreatePixmap( display, root_window,
                  ptr->nWidth, ptr->nHeight * 2, 1 )))
            return 0;
        if (!(image = XCreateImage( display, visual,
                1, ZPixmap, 0, (char *)(ptr + 1), ptr->nWidth,
                ptr->nHeight * 2, 16, ptr->nWidthBytes/ptr->bBitsPerPixel)))
        {
            XFreePixmap( display, pixmapAll );
            return 0;
        }
        gc = XCreateGC( display, pixmapAll, 0, NULL );
        XSetGraphicsExposures( display, gc, False );
        image->byte_order = MSBFirst;
        image->bitmap_bit_order = MSBFirst;
        image->bitmap_unit = 16;
        _XInitImageFuncPtrs(image);
        if (ptr->bPlanes * ptr->bBitsPerPixel == 1)
        {
            /* A plain old white on black cursor. */
            fg.red = fg.green = fg.blue = 0xffff;
            bg.red = bg.green = bg.blue = 0x0000;
            XPutImage( display, pixmapAll, gc, image,
                0, 0, 0, 0, ptr->nWidth, ptr->nHeight * 2 );
        }
        else
        {
            int     rbits, gbits, bbits, red, green, blue;
            int     rfg, gfg, bfg, rbg, gbg, bbg;
            int     rscale, gscale, bscale;
            int     x, y, xmax, ymax, bitIndex, byteIndex, xorIndex;
            unsigned char *theMask, *theImage, theChar;
            int     threshold, fgBits, bgBits, bitShifted;
            BYTE    pXorBits[128];   /* Up to 32x32 icons */

            switch (ptr->bBitsPerPixel)
            {
            case 24:
                rbits = 8;
                gbits = 8;
                bbits = 8;
                threshold = 0x40;
                break;
            case 16:
                rbits = 5;
                gbits = 6;
                bbits = 5;
                threshold = 0x40;
                break;
            default:
                FIXME("Currently no support for cursors with %d bits per pixel\n",
                  ptr->bBitsPerPixel);
                XFreePixmap( display, pixmapAll );
                XFreeGC( display, gc );
                image->data = NULL;
                XDestroyImage( image );
                return 0;
            }
            /* The location of the mask. */
            theMask = (char *)(ptr + 1);
            /* The mask should still be 1 bit per pixel. The color image
             * should immediately follow the mask.
             */
            theImage = &theMask[ptr->nWidth/8 * ptr->nHeight];
            rfg = gfg = bfg = rbg = gbg = bbg = 0;
            bitIndex = 0;
            byteIndex = 0;
            xorIndex = 0;
            fgBits = 0;
            bitShifted = 0x01;
            xmax = (ptr->nWidth > 32) ? 32 : ptr->nWidth;
            if (ptr->nWidth > 32) {
                ERR("Got a %dx%d cursor. Cannot handle larger than 32x32.\n",
                  ptr->nWidth, ptr->nHeight);
            }
            ymax = (ptr->nHeight > 32) ? 32 : ptr->nHeight;

            memset(pXorBits, 0, 128);
            for (y=0; y<ymax; y++)
            {
                for (x=0; x<xmax; x++)
                {
                   	red = green = blue = 0;
                   	switch (ptr->bBitsPerPixel)
                   	{
                   	case 24:
                   	    theChar = theImage[byteIndex++];
                   	    blue = theChar;
                   	    theChar = theImage[byteIndex++];
                   	    green = theChar;
                   	    theChar = theImage[byteIndex++];
                   	    red = theChar;
                   	    break;
                   	case 16:
                   	    theChar = theImage[byteIndex++];
                   	    blue = theChar & 0x1F;
                   	    green = (theChar & 0xE0) >> 5;
                   	    theChar = theImage[byteIndex++];
                   	    green |= (theChar & 0x07) << 3;
                   	    red = (theChar & 0xF8) >> 3;
                   	    break;
                   	}

                    if (red+green+blue > threshold)
                    {
                        rfg += red;
                        gfg += green;
                        bfg += blue;
                        fgBits++;
                        pXorBits[xorIndex] |= bitShifted;
                    }
                    else
                    {
                        rbg += red;
                        gbg += green;
                        bbg += blue;
                    }
                    if (x%8 == 7)
                    {
                        bitShifted = 0x01;
                        xorIndex++;
                    }
                    else
                        bitShifted = bitShifted << 1;
                }
            }
            rscale = 1 << (16 - rbits);
            gscale = 1 << (16 - gbits);
            bscale = 1 << (16 - bbits);
            if (fgBits)
            {
                fg.red   = rfg * rscale / fgBits;
                fg.green = gfg * gscale / fgBits;
                fg.blue  = bfg * bscale / fgBits;
            }
            else fg.red = fg.green = fg.blue = 0;
            bgBits = xmax * ymax - fgBits;
            if (bgBits)
            {
                bg.red   = rbg * rscale / bgBits;
                bg.green = gbg * gscale / bgBits;
                bg.blue  = bbg * bscale / bgBits;
            }
            else bg.red = bg.green = bg.blue = 0;
            pixmapBits = XCreateBitmapFromData( display, root_window, pXorBits, xmax, ymax );
            if (!pixmapBits)
            {
                XFreePixmap( display, pixmapAll );
                XFreeGC( display, gc );
                image->data = NULL;
                XDestroyImage( image );
                return 0;
            }

            /* Put the mask. */
            XPutImage( display, pixmapAll, gc, image,
                   0, 0, 0, 0, ptr->nWidth, ptr->nHeight );
            XSetFunction( display, gc, GXcopy );
            /* Put the image */
            XCopyArea( display, pixmapBits, pixmapAll, gc,
                       0, 0, xmax, ymax, 0, ptr->nHeight );
            XFreePixmap( display, pixmapBits );
        }
        image->data = NULL;
        XDestroyImage( image );

        /* Now create the 2 pixmaps for bits and mask */

        pixmapBits = XCreatePixmap( display, root_window, ptr->nWidth, ptr->nHeight, 1 );
        pixmapMask = XCreatePixmap( display, root_window, ptr->nWidth, ptr->nHeight, 1 );
        pixmapMaskInv = XCreatePixmap( display, root_window, ptr->nWidth, ptr->nHeight, 1 );

        /* Make sure everything went OK so far */

        if (pixmapBits && pixmapMask && pixmapMaskInv)
        {
            POINT hotspot;

            /* We have to do some magic here, as cursors are not fully
             * compatible between Windows and X11. Under X11, there
             * are only 3 possible color cursor: black, white and
             * masked. So we map the 4th Windows color (invert the
             * bits on the screen) to black and an additional white bit on
             * an other place (+1,+1). This require some boolean arithmetic:
             *
             *         Windows          |          X11
             * And    Xor      Result   |   Bits     Mask     Result
             *  0      0     black      |    0        1     background
             *  0      1     white      |    1        1     foreground
             *  1      0     no change  |    X        0     no change
             *  1      1     inverted   |    0        1     background
             *
             * which gives:
             *  Bits = not 'And' and 'Xor' or 'And2' and 'Xor2'
             *  Mask = not 'And' or 'Xor' or 'And2' and 'Xor2'
             *
             * FIXME: apparently some servers do support 'inverted' color.
             * I don't know if it's correct per the X spec, but maybe
             * we ought to take advantage of it.  -- AJ
             */
            XSetFunction( display, gc, GXcopy );
            XCopyArea( display, pixmapAll, pixmapBits, gc,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XCopyArea( display, pixmapAll, pixmapMask, gc,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XCopyArea( display, pixmapAll, pixmapMaskInv, gc,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, gc, GXand );
            XCopyArea( display, pixmapAll, pixmapMaskInv, gc,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, gc, GXandReverse );
            XCopyArea( display, pixmapAll, pixmapBits, gc,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, gc, GXorReverse );
            XCopyArea( display, pixmapAll, pixmapMask, gc,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            /* Additional white */
            XSetFunction( display, gc, GXor );
            XCopyArea( display, pixmapMaskInv, pixmapMask, gc,
                       0, 0, ptr->nWidth, ptr->nHeight, 1, 1 );
            XCopyArea( display, pixmapMaskInv, pixmapBits, gc,
                       0, 0, ptr->nWidth, ptr->nHeight, 1, 1 );
            XSetFunction( display, gc, GXcopy );

            /* Make sure hotspot is valid */
            hotspot.x = ptr->ptHotSpot.x;
            hotspot.y = ptr->ptHotSpot.y;
            if (hotspot.x < 0 || hotspot.x >= ptr->nWidth ||
                hotspot.y < 0 || hotspot.y >= ptr->nHeight)
            {
                hotspot.x = ptr->nWidth / 2;
                hotspot.y = ptr->nHeight / 2;
            }
            cursor = XCreatePixmapCursor( display, pixmapBits, pixmapMask,
                                          &fg, &bg, hotspot.x, hotspot.y );
        }

        /* Now free everything */

        if (pixmapAll) XFreePixmap( display, pixmapAll );
        if (pixmapBits) XFreePixmap( display, pixmapBits );
        if (pixmapMask) XFreePixmap( display, pixmapMask );
        if (pixmapMaskInv) XFreePixmap( display, pixmapMaskInv );
        XFreeGC( display, gc );
    }
    return cursor;
}


/***********************************************************************
 *		SetCursor (X11DRV.@)
 */
void X11DRV_SetCursor( CURSORICONINFO *lpCursor )
{
    Cursor cursor;

    if (root_window != DefaultRootWindow(gdi_display))
    {
        /* If in desktop mode, set the cursor on the desktop window */

        wine_tsx11_lock();
        cursor = create_cursor( gdi_display, lpCursor );
        if (cursor)
        {
            XDefineCursor( gdi_display, root_window, cursor );
	    /* Make the change take effect immediately */
	    XFlush(gdi_display);
            XFreeCursor( gdi_display, cursor );
        }
        wine_tsx11_unlock();
    }
    else /* set the same cursor for all top-level windows of the current thread */
    {
        struct x11drv_thread_data *data = x11drv_thread_data();

        wine_tsx11_lock();
        cursor = create_cursor( data->display, lpCursor );
        if (cursor)
        {
            if (data->cursor) XFreeCursor( data->display, data->cursor );
            data->cursor = cursor;
            if (data->cursor_window)
            {
                XDefineCursor( data->display, data->cursor_window, cursor );
                /* Make the change take effect immediately */
                XFlush( data->display );
            }
        }
        wine_tsx11_unlock();
    }
}

/***********************************************************************
 *		SetCursorPos (X11DRV.@)
 */
void X11DRV_SetCursorPos( INT x, INT y )
{
    Display *display = thread_display();

    TRACE( "warping to (%d,%d)\n", x, y );

    wine_tsx11_lock();
    XWarpPointer( display, root_window, root_window, 0, 0, 0, 0, x, y );
    XFlush( display ); /* just in case */
    wine_tsx11_unlock();
}

/***********************************************************************
 *		GetCursorPos (X11DRV.@)
 */
void X11DRV_GetCursorPos(LPPOINT pos)
{
  Display *display = thread_display();
  Window root, child;
  int rootX, rootY, winX, winY;
  unsigned int xstate;

  if (!TSXQueryPointer( display, root_window, &root, &child,
                        &rootX, &rootY, &winX, &winY, &xstate ))
    return;

  update_key_state( xstate );
  update_button_state( xstate );
  TRACE("pointer at (%d,%d)\n", winX, winY );
  pos->x = winX;
  pos->y = winY;
}

/***********************************************************************
 *		InitMouse (X11DRV.@)
 */
void X11DRV_InitMouse( BYTE *key_state_table )
{
    pKeyStateTable = key_state_table;
}


/***********************************************************************
 *           X11DRV_ButtonPress
 */
void X11DRV_ButtonPress( HWND hwnd, XButtonEvent *event )
{
    int buttonNum = event->button - 1;
    WORD wData = 0;
    POINT pt;

    if (buttonNum >= NB_BUTTONS) return;
    if (!hwnd) return;

    update_cursor( hwnd, event->window );
    get_coords( &hwnd, event->window, event->x, event->y, &pt );

    switch (buttonNum)
    {
    case 3:
        wData = WHEEL_DELTA;
        break;
    case 4:
        wData = -WHEEL_DELTA;
        break;
    }
    update_key_state( event->state );
    send_mouse_event( hwnd, button_down_flags[buttonNum] | MOUSEEVENTF_ABSOLUTE,
                      pt.x, pt.y, wData, event->time );
}


/***********************************************************************
 *           X11DRV_ButtonRelease
 */
void X11DRV_ButtonRelease( HWND hwnd, XButtonEvent *event )
{
    int buttonNum = event->button - 1;
    POINT pt;

    if (buttonNum >= NB_BUTTONS || !button_up_flags[buttonNum]) return;
    if (!hwnd) return;

    update_cursor( hwnd, event->window );
    get_coords( &hwnd, event->window, event->x, event->y, &pt );
    update_key_state( event->state );
    send_mouse_event( hwnd, button_up_flags[buttonNum] | MOUSEEVENTF_ABSOLUTE,
                      pt.x, pt.y, 0, event->time );
}


/***********************************************************************
 *           X11DRV_MotionNotify
 */
void X11DRV_MotionNotify( HWND hwnd, XMotionEvent *event )
{
    POINT pt;

    if (!hwnd) return;

    update_cursor( hwnd, event->window );
    get_coords( &hwnd, event->window, event->x, event->y, &pt );
    update_key_state( event->state );
    send_mouse_event( hwnd, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                      pt.x, pt.y, 0, event->time );
}


/***********************************************************************
 *           X11DRV_EnterNotify
 */
void X11DRV_EnterNotify( HWND hwnd, XCrossingEvent *event )
{
    POINT pt;

    if (event->detail == NotifyVirtual || event->detail == NotifyNonlinearVirtual) return;
    if (!hwnd) return;

    /* simulate a mouse motion event */
    update_cursor( hwnd, event->window );
    get_coords( &hwnd, event->window, event->x, event->y, &pt );
    update_key_state( event->state );
    send_mouse_event( hwnd, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                      pt.x, pt.y, 0, event->time );
}


#ifdef HAVE_LIBXXF86DGA2
/**********************************************************************
 *              X11DRV_DGAMotionEvent
 */
void X11DRV_DGAMotionEvent( HWND hwnd, XDGAMotionEvent *event )
{
    update_key_state( event->state );
    send_mouse_event( hwnd, MOUSEEVENTF_MOVE, event->dx, event->dy, 0, event->time );
}

/**********************************************************************
 *              X11DRV_DGAButtonPressEvent
 */
void X11DRV_DGAButtonPressEvent( HWND hwnd, XDGAButtonEvent *event )
{
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;
    update_key_state( event->state );
    send_mouse_event( hwnd, button_down_flags[buttonNum], 0, 0, 0, event->time );
}

/**********************************************************************
 *              X11DRV_DGAButtonReleaseEvent
 */
void X11DRV_DGAButtonReleaseEvent( HWND hwnd, XDGAButtonEvent *event )
{
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;
    update_key_state( event->state );
    send_mouse_event( hwnd, button_up_flags[buttonNum], 0, 0, 0, event->time );
}
#endif /* HAVE_LIBXXF86DGA2 */
