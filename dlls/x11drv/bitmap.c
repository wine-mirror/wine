/*
 * X11DRV bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1999 Noel Borthwick
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

#include <stdio.h>
#include <stdlib.h>
#include "gdi.h"
#include "bitmap.h"
#include "wine/debug.h"
#include "x11drv.h"
#include "wingdi.h"
#include "windef.h"
#include "wine/winuser16.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

  /* GCs used for B&W and color bitmap operations */
GC BITMAP_monoGC = 0, BITMAP_colorGC = 0;
Pixmap BITMAP_stock_pixmap;   /* pixmap for the default stock bitmap */

extern const DC_FUNCTIONS *X11DRV_DC_Funcs;  /* hack */

/***********************************************************************
 *           X11DRV_BITMAP_Init
 */
BOOL X11DRV_BITMAP_Init(void)
{
    Pixmap tmpPixmap;

      /* Create the necessary GCs */

    wine_tsx11_lock();
    BITMAP_stock_pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 1 );
    BITMAP_monoGC = XCreateGC( gdi_display, BITMAP_stock_pixmap, 0, NULL );
    XSetGraphicsExposures( gdi_display, BITMAP_monoGC, False );
    XSetSubwindowMode( gdi_display, BITMAP_monoGC, IncludeInferiors );

    if (screen_depth != 1)
    {
        if ((tmpPixmap = XCreatePixmap( gdi_display, root_window, 1, 1, screen_depth )))
        {
            BITMAP_colorGC = XCreateGC( gdi_display, tmpPixmap, 0, NULL );
            XSetGraphicsExposures( gdi_display, BITMAP_colorGC, False );
            XSetSubwindowMode( gdi_display, BITMAP_colorGC, IncludeInferiors );
            XFreePixmap( gdi_display, tmpPixmap );
        }
    }
    wine_tsx11_unlock();
    return TRUE;
}

/***********************************************************************
 *           SelectBitmap   (X11DRV.@)
 */
HBITMAP X11DRV_SelectBitmap( X11DRV_PDEVICE *physDev, HBITMAP hbitmap )
{
    BITMAPOBJ *bmp;
    DC *dc = physDev->dc;

    if (!(bmp = GDI_GetObjPtr( hbitmap, BITMAP_MAGIC ))) return 0;

    if(physDev->xrender)
        X11DRV_XRender_UpdateDrawable( physDev );

    if (hbitmap == GetStockObject(DEFAULT_BITMAP))
        physDev->drawable = BITMAP_stock_pixmap;
    else
        physDev->drawable = (Pixmap)bmp->physBitmap;

      /* Change GC depth if needed */

    if (dc->bitsPerPixel != bmp->bitmap.bmBitsPixel)
    {
        wine_tsx11_lock();
        XFreeGC( gdi_display, physDev->gc );
        physDev->gc = XCreateGC( gdi_display, physDev->drawable, 0, NULL );
        XSetGraphicsExposures( gdi_display, physDev->gc, False );
        XSetSubwindowMode( gdi_display, physDev->gc, IncludeInferiors );
        XFlush( gdi_display );
        wine_tsx11_unlock();
    }
    GDI_ReleaseObj( hbitmap );
    return hbitmap;
}


/****************************************************************************
 *	  CreateBitmap   (X11DRV.@)
 *
 * Create a device dependent X11 bitmap
 *
 * Returns TRUE on success else FALSE
 */
BOOL X11DRV_CreateBitmap( X11DRV_PDEVICE *physDev, HBITMAP hbitmap )
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );

    if(!bmp) {
        WARN("Bad bitmap handle %p\n", hbitmap);
	return FALSE;
    }

      /* Check parameters */
    if (bmp->bitmap.bmPlanes != 1)
    {
        GDI_ReleaseObj( hbitmap );
        return 0;
    }
    if ((bmp->bitmap.bmBitsPixel != 1) && (bmp->bitmap.bmBitsPixel != screen_depth))
    {
        ERR("Trying to make bitmap with planes=%d, bpp=%d\n",
	    bmp->bitmap.bmPlanes, bmp->bitmap.bmBitsPixel);
        GDI_ReleaseObj( hbitmap );
	return FALSE;
    }
    if (hbitmap == GetStockObject(DEFAULT_BITMAP))
    {
        ERR( "called for stock bitmap, please report\n" );
        GDI_ReleaseObj( hbitmap );
        return FALSE;
    }

    TRACE("(%p) %dx%d %d bpp\n", hbitmap, bmp->bitmap.bmWidth,
	  bmp->bitmap.bmHeight, bmp->bitmap.bmBitsPixel);

      /* Create the pixmap */
    wine_tsx11_lock();
    bmp->physBitmap = (void *)XCreatePixmap(gdi_display, root_window,
                                            bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
                                            bmp->bitmap.bmBitsPixel);
    wine_tsx11_unlock();
    if (!bmp->physBitmap)
    {
        WARN("Can't create Pixmap\n");
	GDI_ReleaseObj( hbitmap );
	return FALSE;
    }

    if (bmp->bitmap.bmBits) /* Set bitmap bits */
        X11DRV_SetBitmapBits( hbitmap, bmp->bitmap.bmBits,
                              bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes );

    GDI_ReleaseObj( hbitmap );
    return TRUE;
}


/***********************************************************************
 *           GetBitmapBits   (X11DRV.@)
 *
 * RETURNS
 *    Success: Number of bytes copied
 *    Failure: 0
 */
LONG X11DRV_GetBitmapBits( HBITMAP hbitmap, void *buffer, LONG count )
{
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG old_height, height;
    XImage *image;
    LPBYTE tbuf, startline;
    int	h, w;

    if (!bmp) return 0;
    TRACE("(bmp=%p, buffer=%p, count=0x%lx)\n", bmp, buffer, count);

    wine_tsx11_lock();

    /* Hack: change the bitmap height temporarily to avoid */
    /*       getting unnecessary bitmap rows. */

    old_height = bmp->bitmap.bmHeight;
    height = bmp->bitmap.bmHeight = count / bmp->bitmap.bmWidthBytes;

    image = XGetImage( gdi_display, (Pixmap)bmp->physBitmap,
                       0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
                       AllPlanes, ZPixmap );
    bmp->bitmap.bmHeight = old_height;

    /* copy XImage to 16 bit padded image buffer with real bitsperpixel */

    startline = buffer;
    switch (bmp->bitmap.bmBitsPixel)
    {
    case 1:
        for (h=0;h<height;h++)
        {
	    tbuf = startline;
            *tbuf = 0;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if ((w%8) == 0)
                    *tbuf = 0;
                *tbuf |= XGetPixel(image,w,h)<<(7-(w&7));
                if ((w&7) == 7) ++tbuf;
            }
	    startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
	    tbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if (!(w & 1)) *tbuf = XGetPixel( image, w, h) << 4;
	    	else *tbuf++ |= XGetPixel( image, w, h) & 0x0f;
            }
	    startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
	    tbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
                *tbuf++ = XGetPixel(image,w,h);
	    startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
	    tbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>>8) & 0xff;
            }
	    startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 24:
        for (h=0;h<height;h++)
        {
	    tbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>> 8) & 0xff;
		*tbuf++ = (pixel>>16) & 0xff;
	    }
	    startline += bmp->bitmap.bmWidthBytes;
	}
        break;

    case 32:
        for (h=0;h<height;h++)
        {
	    tbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>> 8) & 0xff;
		*tbuf++ = (pixel>>16) & 0xff;
		*tbuf++ = (pixel>>24) & 0xff;
	    }
	    startline += bmp->bitmap.bmWidthBytes;
	}
        break;
    default:
        FIXME("Unhandled bits:%d\n", bmp->bitmap.bmBitsPixel);
    }
    XDestroyImage( image );
    wine_tsx11_unlock();
    GDI_ReleaseObj( hbitmap );
    return count;
}



/******************************************************************************
 *             SetBitmapBits   (X11DRV.@)
 *
 * RETURNS
 *    Success: Number of bytes used in setting the bitmap bits
 *    Failure: 0
 */
LONG X11DRV_SetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count )
{
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG height;
    XImage *image;
    const BYTE *sbuf, *startline;
    int	w, h;

    if (!bmp) return 0;
    TRACE("(bmp=%p, bits=%p, count=0x%lx)\n", bmp, bits, count);

    height = count / bmp->bitmap.bmWidthBytes;

    wine_tsx11_lock();
    image = XCreateImage( gdi_display, visual, bmp->bitmap.bmBitsPixel, ZPixmap, 0, NULL,
                          bmp->bitmap.bmWidth, height, 32, 0 );
    if (!(image->data = (LPBYTE)malloc(image->bytes_per_line * height)))
    {
        WARN("No memory to create image data.\n");
        XDestroyImage( image );
        wine_tsx11_unlock();
        GDI_ReleaseObj( hbitmap );
        return 0;
    }

    /* copy 16 bit padded image buffer with real bitsperpixel to XImage */

    startline = bits;

    switch (bmp->bitmap.bmBitsPixel)
    {
    case 1:
        for (h=0;h<height;h++)
        {
	    sbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[0]>>(7-(w&7))) & 1);
                if ((w&7) == 7)
                    sbuf++;
            }
            startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
	    sbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if (!(w & 1)) XPutPixel( image, w, h, *sbuf >> 4 );
                else XPutPixel( image, w, h, *sbuf++ & 0xf );
            }
            startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
	    sbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
                XPutPixel(image,w,h,*sbuf++);
            startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
	    sbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,sbuf[1]*256+sbuf[0]);
                sbuf+=2;
            }
	    startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 24:
        for (h=0;h<height;h++)
        {
	    sbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[2]<<16)+(sbuf[1]<<8)+sbuf[0]);
                sbuf += 3;
            }
            startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    case 32:
        for (h=0;h<height;h++)
        {
	    sbuf = startline;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[3]<<24)+(sbuf[2]<<16)+(sbuf[1]<<8)+sbuf[0]);
                sbuf += 4;
            }
	    startline += bmp->bitmap.bmWidthBytes;
        }
        break;
    default:
      FIXME("Unhandled bits:%d\n", bmp->bitmap.bmBitsPixel);

    }
    XPutImage( gdi_display, (Pixmap)bmp->physBitmap, BITMAP_GC(bmp),
               image, 0, 0, 0, 0, bmp->bitmap.bmWidth, height );
    XDestroyImage( image ); /* frees image->data too */
    wine_tsx11_unlock();
    GDI_ReleaseObj( hbitmap );
    return count;
}

/***********************************************************************
 *           DeleteBitmap   (X11DRV.@)
 */
BOOL X11DRV_DeleteBitmap( HBITMAP hbitmap )
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (bmp)
    {
        wine_tsx11_lock();
        if (bmp->physBitmap) XFreePixmap( gdi_display, (Pixmap)bmp->physBitmap );
        bmp->physBitmap = NULL;
        wine_tsx11_unlock();
        if (bmp->dib) X11DRV_DIB_DeleteDIBSection( bmp );
        GDI_ReleaseObj( hbitmap );
    }
    return TRUE;
}

/**************************************************************************
 *	        X11DRV_BITMAP_CreateBitmapHeaderFromPixmap
 *
 *  Allocates an HBITMAP which references the Pixmap passed in.
 *  Note: This function makes the bitmap an owner of the Pixmap so subsequently
 *  calling DeleteObject on this will free the Pixmap as well.
 */
HBITMAP X11DRV_BITMAP_CreateBitmapHeaderFromPixmap(Pixmap pixmap)
{
    HBITMAP hBmp = 0;
    BITMAPOBJ *pBmp = NULL;
    Window root;
    int x,y;               /* Unused */
    unsigned border_width; /* Unused */
    unsigned int depth, width, height;

    /* Get the Pixmap dimensions and bit depth */
    wine_tsx11_lock();
    if (!XGetGeometry(gdi_display, pixmap, &root, &x, &y, &width, &height,
                      &border_width, &depth)) depth = 0;
    wine_tsx11_unlock();
    if (!depth) goto END;

    TRACE("\tPixmap properties: width=%d, height=%d, depth=%d\n",
          width, height, depth);

    /*
     * Create an HBITMAP with the same dimensions and BPP as the pixmap,
     * and make it a container for the pixmap passed.
     */
    hBmp = CreateBitmap( width, height, 1, depth, NULL );

    pBmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );

    pBmp->funcs = X11DRV_DC_Funcs;
    pBmp->physBitmap = (void *)pixmap;
    GDI_ReleaseObj( hBmp );

END:
    TRACE("\tReturning HBITMAP %p\n", hBmp);
    return hBmp;
}


/**************************************************************************
 *	        X11DRV_BITMAP_CreateBitmapFromPixmap
 *
 *  Allocates an HBITMAP and copies the Pixmap data into it.
 *  If bDeletePixmap is TRUE, the Pixmap passed in is deleted after the conversion.
 */
HBITMAP X11DRV_BITMAP_CreateBitmapFromPixmap(Pixmap pixmap, BOOL bDeletePixmap)
{
    HBITMAP hBmp = 0, hBmpCopy = 0;
    BITMAPOBJ *pBmp = NULL;
    unsigned int width, height;

    /* Allocate an HBITMAP which references the Pixmap passed to us */
    hBmp = X11DRV_BITMAP_CreateBitmapHeaderFromPixmap(pixmap);
    if (!hBmp)
    {
        TRACE("\tCould not create bitmap header for Pixmap\n");
        goto END;
    }

    /* Get the bitmap dimensions */
    width = pBmp->bitmap.bmWidth;
    height = pBmp->bitmap.bmHeight;

    hBmpCopy = (HBITMAP)CopyImage(hBmp, IMAGE_BITMAP, width, height, LR_CREATEDIBSECTION);

    /* We can now get rid of the HBITMAP wrapper we created earlier.
     * Note: Simply calling DeleteObject will free the embedded Pixmap as well.
     */
    if (!bDeletePixmap)
    {
        /* Manually clear the bitmap internals to prevent the Pixmap
         * from being deleted by DeleteObject.
         */
        pBmp->physBitmap = NULL;
        pBmp->funcs = NULL;
    }
    DeleteObject(hBmp);

END:
    TRACE("\tReturning HBITMAP %p\n", hBmpCopy);
    return hBmpCopy;
}


/**************************************************************************
 *	           X11DRV_BITMAP_CreatePixmapFromBitmap
 *
 *    Creates a Pixmap from a bitmap
 */
Pixmap X11DRV_BITMAP_CreatePixmapFromBitmap( HBITMAP hBmp, HDC hdc )
{
    HGLOBAL hPackedDIB = 0;
    Pixmap pixmap = 0;

    /*
     * Create a packed DIB from the bitmap passed to us.
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */
    hPackedDIB = X11DRV_DIB_CreateDIBFromBitmap(hdc, hBmp);

    /* Create a Pixmap from the packed DIB */
    pixmap = X11DRV_DIB_CreatePixmapFromDIB( hPackedDIB, hdc );

    /* Free the temporary packed DIB */
    GlobalFree(hPackedDIB);

    return pixmap;
}


/***********************************************************************
 *           X11DRV_BITMAP_Pixmap
 *
 * This function exists solely for x11 driver of the window system.
 */
Pixmap X11DRV_BITMAP_Pixmap(HBITMAP hbitmap)
{
    Pixmap pixmap;
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (bmp) {
      pixmap = (Pixmap)bmp->physBitmap;
      GDI_ReleaseObj( hbitmap );
    }
    else {
      ERR("handle %p returned no obj\n", hbitmap);
      pixmap = 0;
    }
    return pixmap;
}
