/*
 * X11 mouse driver
 *
 * Copyright 1998 Ulrich Weigand
 */

#include "config.h"

#include "ts_xlib.h"

#include "callback.h"
#include "debugtools.h"
#include "mouse.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(cursor);

/**********************************************************************/

Cursor X11DRV_MOUSE_XCursor = None;    /* Current X cursor */

static LONG X11DRV_MOUSE_WarpPointer = 0;  /* hack; see DISPLAY_MoveCursor */
static LPMOUSE_EVENT_PROC DefMouseEventProc = NULL;

/***********************************************************************
 *		X11DRV_MOUSE_DoSetCursor
 */
static BOOL X11DRV_MOUSE_DoSetCursor( CURSORICONINFO *ptr )
{
    Pixmap pixmapBits, pixmapMask, pixmapMaskInv, pixmapAll;
    XColor fg, bg;
    Cursor cursor = None;

    if (!ptr)  /* Create an empty cursor */
    {
        static const char data[] = { 0 };

        bg.red = bg.green = bg.blue = 0x0000;
        pixmapBits = XCreateBitmapFromData( display, X11DRV_GetXRootWindow(), data, 1, 1 );
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

        if (ptr->bPlanes * ptr->bBitsPerPixel != 1)
        {
            WARN("Cursor has more than 1 bpp!\n" );
            return FALSE;
        }

        /* Create a pixmap and transfer all the bits to it */

        /* NOTE: Following hack works, but only because XFree depth
         *       1 images really use 1 bit/pixel (and so the same layout
         *       as the Windows cursor data). Perhaps use a more generic
         *       algorithm here.
         */
        pixmapAll = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                                   ptr->nWidth, ptr->nHeight * 2, 1 );
        image = XCreateImage( display, X11DRV_GetVisual(),
                              1, ZPixmap, 0, (char *)(ptr + 1), ptr->nWidth,
                              ptr->nHeight * 2, 16, ptr->nWidthBytes);
        if (image)
        {
            image->byte_order = MSBFirst;
            image->bitmap_bit_order = MSBFirst;
            image->bitmap_unit = 16;
            _XInitImageFuncPtrs(image);
            if (pixmapAll)
                XPutImage( display, pixmapAll, BITMAP_monoGC, image,
                           0, 0, 0, 0, ptr->nWidth, ptr->nHeight * 2 );
            image->data = NULL;
            XDestroyImage( image );
        }

        /* Now create the 2 pixmaps for bits and mask */

        pixmapBits = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                                    ptr->nWidth, ptr->nHeight, 1 );
        pixmapMask = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                                    ptr->nWidth, ptr->nHeight, 1 );
        pixmapMaskInv = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                                    ptr->nWidth, ptr->nHeight, 1 );

        /* Make sure everything went OK so far */

        if (pixmapBits && pixmapMask && pixmapAll)
        {
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
            XSetFunction( display, BITMAP_monoGC, GXcopy );
            XCopyArea( display, pixmapAll, pixmapBits, BITMAP_monoGC,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XCopyArea( display, pixmapAll, pixmapMask, BITMAP_monoGC,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XCopyArea( display, pixmapAll, pixmapMaskInv, BITMAP_monoGC,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, BITMAP_monoGC, GXand );
            XCopyArea( display, pixmapAll, pixmapMaskInv, BITMAP_monoGC,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, BITMAP_monoGC, GXandReverse );
            XCopyArea( display, pixmapAll, pixmapBits, BITMAP_monoGC,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, BITMAP_monoGC, GXorReverse );
            XCopyArea( display, pixmapAll, pixmapMask, BITMAP_monoGC,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            /* Additional white */
            XSetFunction( display, BITMAP_monoGC, GXor );
            XCopyArea( display, pixmapMaskInv, pixmapMask, BITMAP_monoGC,
                       0, 0, ptr->nWidth, ptr->nHeight, 1, 1 );
            XCopyArea( display, pixmapMaskInv, pixmapBits, BITMAP_monoGC,
                       0, 0, ptr->nWidth, ptr->nHeight, 1, 1 );
            XSetFunction( display, BITMAP_monoGC, GXcopy );
            fg.red = fg.green = fg.blue = 0xffff;
            bg.red = bg.green = bg.blue = 0x0000;
            cursor = XCreatePixmapCursor( display, pixmapBits, pixmapMask,
                                &fg, &bg, ptr->ptHotSpot.x, ptr->ptHotSpot.y );
        }

        /* Now free everything */

        if (pixmapAll) XFreePixmap( display, pixmapAll );
        if (pixmapBits) XFreePixmap( display, pixmapBits );
        if (pixmapMask) XFreePixmap( display, pixmapMask );
        if (pixmapMaskInv) XFreePixmap( display, pixmapMaskInv );
    }

    if (cursor == None) return FALSE;
    if (X11DRV_MOUSE_XCursor != None) XFreeCursor( display, X11DRV_MOUSE_XCursor );
    X11DRV_MOUSE_XCursor = cursor;

    return TRUE;
}

/***********************************************************************
 *		X11DRV_MOUSE_SetCursor
 */
void X11DRV_MOUSE_SetCursor( CURSORICONINFO *lpCursor )
{
    BOOL success;

    EnterCriticalSection( &X11DRV_CritSection );
    success = CALL_LARGE_STACK( X11DRV_MOUSE_DoSetCursor, lpCursor );
    LeaveCriticalSection( &X11DRV_CritSection );
    if ( !success ) return;

    if (X11DRV_GetXRootWindow() != DefaultRootWindow(display))
    {
        /* If in desktop mode, set the cursor on the desktop window */

        TSXDefineCursor( display, X11DRV_GetXRootWindow(), X11DRV_MOUSE_XCursor );
    }
    else
    {
        /* Else, set the same cursor for all top-level windows */

        /* FIXME: we should not reference USER internals here, but native USER 
                  works only in desktop mode anyway, so this should not matter */

        HWND hwnd = GetWindow( GetDesktopWindow(), GW_CHILD );
        while(hwnd)
        {
            WND *tmpWnd = WIN_FindWndPtr(hwnd);
            Window win = X11DRV_WND_FindXWindow(tmpWnd );
            if (win && win!=DefaultRootWindow(display))
                TSXDefineCursor( display, win, X11DRV_MOUSE_XCursor );
            hwnd = GetWindow( hwnd, GW_HWNDNEXT );
            WIN_ReleaseWndPtr(tmpWnd);
        }
    }
}

/***********************************************************************
 *		X11DRV_MOUSE_MoveCursor
 */
void X11DRV_MOUSE_MoveCursor(WORD wAbsX, WORD wAbsY)
{
  /* 
   * We do not want the to create MotionNotify events here, 
   * otherwise we will get an endless recursion:
   * XMotionEvent -> MOUSEEVENTF_MOVE -> mouse_event -> DisplayMoveCursor
   * -> XWarpPointer -> XMotionEvent -> ...
   *
   * Unfortunately, the XWarpPointer call does create a MotionNotify
   * event. So, we use a hack: before MOUSE_SendEvent calls the mouse event
   * procedure, it sets a global flag. If this flag is set, we skip the
   * XWarpPointer call.  If we are *not* called from within MOUSE_SendEvent,
   * we will call XWarpPointer, which will create a MotionNotify event.
   * Strictly speaking, this is also wrong, but that should normally not
   * have any negative effects ...
   *
   * But first of all, we check whether we already are at the position
   * are supposed to move to; if so, we don't need to do anything.
   */
  
  Window root, child;
  int rootX, rootY, winX, winY;
  unsigned int xstate;
  
  if (X11DRV_MOUSE_WarpPointer < 0) return;

  if (!TSXQueryPointer( display, X11DRV_GetXRootWindow(), &root, &child,
			&rootX, &rootY, &winX, &winY, &xstate ))
    return;
  
  if ( winX == wAbsX && winY == wAbsY )
    return;
  
  TRACE("(%d,%d): moving from (%d,%d)\n", wAbsX, wAbsY, winX, winY );
  
  TSXWarpPointer( display, X11DRV_GetXRootWindow(), X11DRV_GetXRootWindow(), 
		  0, 0, 0, 0, wAbsX, wAbsY );
}

/***********************************************************************
 *           X11DRV_MOUSE_Init
 */
void X11DRV_MOUSE_Init( LPMOUSE_EVENT_PROC proc )
{
    static int init_done;

    DefMouseEventProc = proc;

    if (!init_done)
    {
        Window root, child;
        int root_x, root_y, child_x, child_y;
        unsigned int KeyState;
  
        init_done = 1;
        /* Get the current mouse position and simulate an absolute mouse
           movement to initialize the mouse global variables */
        TSXQueryPointer( display, X11DRV_GetXRootWindow(), &root, &child,
                         &root_x, &root_y, &child_x, &child_y, &KeyState);
        X11DRV_MOUSE_SendEvent(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                               root_x, root_y, X11DRV_EVENT_XStateToKeyState(KeyState),
                               GetTickCount(), 0 );
    }
}


/***********************************************************************
 *           X11DRV_MOUSE_SendEvent
 */
void X11DRV_MOUSE_SendEvent( DWORD mouseStatus, DWORD posX, DWORD posY, 
                             DWORD keyState, DWORD time, HWND hWnd )
{
    int width  = GetSystemMetrics( SM_CXSCREEN );
    int height = GetSystemMetrics( SM_CYSCREEN );
    int iWndsLocks;
    WINE_MOUSEEVENT wme;

    if ( !DefMouseEventProc ) return;

    TRACE("(%04lX,%ld,%ld)\n", mouseStatus, posX, posY );

    if (mouseStatus & MOUSEEVENTF_MOVE) {
      if (mouseStatus & MOUSEEVENTF_ABSOLUTE) {
	/* Relative mouse movements seems not to be scaled as absolute ones */
	posX = (((long)posX << 16) + width-1)  / width;
	posY = (((long)posY << 16) + height-1) / height;
      }
    }

    wme.magic    = WINE_MOUSEEVENT_MAGIC;
    wme.time     = time;
    wme.hWnd     = hWnd;
    wme.keyState = keyState;
    
    InterlockedDecrement( &X11DRV_MOUSE_WarpPointer );
    /* To avoid deadlocks, we have to suspend all locks on windows structures
       before the program control is passed to the mouse driver */
    iWndsLocks = WIN_SuspendWndsLock();
    DefMouseEventProc( mouseStatus, posX, posY, 0, (DWORD)&wme );
    WIN_RestoreWndsLock(iWndsLocks);
    InterlockedIncrement( &X11DRV_MOUSE_WarpPointer );
}
