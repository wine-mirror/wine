/*
 * X11DRV bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1999 Noel Borthwick
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "ts_xlib.h"
#include "ts_xutil.h"

#include <stdio.h>
#include <stdlib.h>
#include "gdi.h"
#include "callback.h"
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "debugtools.h"
#include "xmalloc.h"
#include "local.h"
#include "x11drv.h"
#include "wingdi.h"
#include "windef.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

  /* GCs used for B&W and color bitmap operations */
GC BITMAP_monoGC = 0, BITMAP_colorGC = 0;


/***********************************************************************
 *           X11DRV_BITMAP_Init
 */
BOOL X11DRV_BITMAP_Init(void)
{
    Pixmap tmpPixmap;
    
      /* Create the necessary GCs */
    
    if ((tmpPixmap = TSXCreatePixmap(display, 
				     X11DRV_GetXRootWindow(), 
				     1, 1, 
				     1)))
    {
	BITMAP_monoGC = TSXCreateGC( display, tmpPixmap, 0, NULL );
	TSXSetGraphicsExposures( display, BITMAP_monoGC, False );
	TSXFreePixmap( display, tmpPixmap );
    }

    if (X11DRV_GetDepth() != 1)
    {
	if ((tmpPixmap = TSXCreatePixmap(display, X11DRV_GetXRootWindow(),
					 1, 1, X11DRV_GetDepth())))
	{
	    BITMAP_colorGC = TSXCreateGC( display, tmpPixmap, 0, NULL );
	    TSXSetGraphicsExposures( display, BITMAP_colorGC, False );
	    TSXFreePixmap( display, tmpPixmap );
	}
    }
    return TRUE;
}

/***********************************************************************
 *           X11DRV_BITMAP_SelectObject
 */
HBITMAP X11DRV_BITMAP_SelectObject( DC * dc, HBITMAP hbitmap,
                                      BITMAPOBJ * bmp )
{
    HRGN hrgn;
    HBITMAP prevHandle = dc->w.hBitmap;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;


    if (!(dc->w.flags & DC_MEMORY)) return 0;

    if(!bmp->physBitmap)
        if(!X11DRV_CreateBitmap(hbitmap))
	    return 0;

    if(bmp->funcs != dc->funcs) {
        WARN("Trying to select non-X11 DDB into an X11 dc\n");
	return 0;
    }

    dc->w.totalExtent.left   = 0;
    dc->w.totalExtent.top    = 0;
    dc->w.totalExtent.right  = bmp->bitmap.bmWidth;
    dc->w.totalExtent.bottom = bmp->bitmap.bmHeight;

    if (dc->w.hVisRgn)
       SetRectRgn( dc->w.hVisRgn, 0, 0,
                     bmp->bitmap.bmWidth, bmp->bitmap.bmHeight );
    else
    { 
       hrgn = CreateRectRgn(0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight);
       if (!hrgn) return 0;
       dc->w.hVisRgn    = hrgn;
    }

    physDev->drawable = (Pixmap)bmp->physBitmap;
    dc->w.hBitmap     = hbitmap;

      /* Change GC depth if needed */

    if (dc->w.bitsPerPixel != bmp->bitmap.bmBitsPixel)
    {
	TSXFreeGC( display, physDev->gc );
	physDev->gc = TSXCreateGC( display, physDev->drawable, 0, NULL );
	TSXSetGraphicsExposures( display, physDev->gc, False );
	dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;
        DC_InitDC( dc );
    }
    else CLIPPING_UpdateGCRegion( dc );  /* Just update GC clip region */
    return prevHandle;
}


/***********************************************************************
 *           XPutImage_wrapper
 *
 * Wrapper to call XPutImage with CALL_LARGE_STACK.
 */

struct XPutImage_descr
{
    BITMAPOBJ *bmp;
    XImage    *image;
    INT      width;
    INT      height;
};

static int XPutImage_wrapper( const struct XPutImage_descr *descr )
{
    return XPutImage( display, (Pixmap)descr->bmp->physBitmap, BITMAP_GC(descr->bmp),
                      descr->image, 0, 0, 0, 0, descr->width, descr->height );
}


/****************************************************************************
 *
 *	  X11DRV_CreateBitmap
 *
 * Create a device dependent X11 bitmap
 *
 * Returns TRUE on success else FALSE
 *
 */

BOOL X11DRV_CreateBitmap( HBITMAP hbitmap )
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );

    if(!bmp) {
        WARN("Bad bitmap handle %08x\n", hbitmap);
	return FALSE;
    }

      /* Check parameters */
    if (bmp->bitmap.bmPlanes != 1) return 0;
    if ((bmp->bitmap.bmBitsPixel != 1) && 
	(bmp->bitmap.bmBitsPixel != X11DRV_GetDepth()))
    {
        ERR("Trying to make bitmap with planes=%d, bpp=%d\n",
	    bmp->bitmap.bmPlanes, bmp->bitmap.bmBitsPixel);
        GDI_HEAP_UNLOCK( hbitmap );
	return FALSE;
    }

    TRACE("(%08x) %dx%d %d bpp\n", hbitmap, bmp->bitmap.bmWidth,
	  bmp->bitmap.bmHeight, bmp->bitmap.bmBitsPixel);

      /* Create the pixmap */
    if (!(bmp->physBitmap = (void *)TSXCreatePixmap(display, X11DRV_GetXRootWindow(),
                                                    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
                                                    bmp->bitmap.bmBitsPixel)))
    {
        WARN("Can't create Pixmap\n");
	GDI_HEAP_UNLOCK( hbitmap );
	return FALSE;
    }
    bmp->funcs = &X11DRV_DC_Funcs;

    if (bmp->bitmap.bmBits) /* Set bitmap bits */
	X11DRV_BitmapBits( hbitmap, bmp->bitmap.bmBits,
			   bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes,
			   DDB_SET );

    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_BITMAP_GetXImage
 *
 * Get an X image for a bitmap. For use with CALL_LARGE_STACK.
 */
XImage *X11DRV_BITMAP_GetXImage( const BITMAPOBJ *bmp )
{
    return XGetImage( display, (Pixmap)bmp->physBitmap,
		      0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
		      AllPlanes, ZPixmap );
}


/***********************************************************************
 *           X11DRV_GetBitmapBits
 * 
 * RETURNS
 *    Success: Number of bytes copied
 *    Failure: 0
 */
static LONG X11DRV_GetBitmapBits(BITMAPOBJ *bmp, void *buffer, LONG count)
{
    LONG old_height, height;
    XImage *image;
    LPBYTE tbuf, startline;
    int	h, w;

    TRACE("(bmp=%p, buffer=%p, count=0x%lx)\n", bmp, buffer, count);

    EnterCriticalSection( &X11DRV_CritSection );

    /* Hack: change the bitmap height temporarily to avoid */
    /*       getting unnecessary bitmap rows. */

    old_height = bmp->bitmap.bmHeight;
    height = bmp->bitmap.bmHeight = count / bmp->bitmap.bmWidthBytes;

    image = (XImage *)CALL_LARGE_STACK( X11DRV_BITMAP_GetXImage, bmp );

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
    LeaveCriticalSection( &X11DRV_CritSection );

    return count;
}



/******************************************************************************
 *             X11DRV_SetBitmapBits
 *
 * RETURNS
 *    Success: Number of bytes used in setting the bitmap bits
 *    Failure: 0
 */
static LONG X11DRV_SetBitmapBits(BITMAPOBJ *bmp, void *bits, LONG count)
{
    struct XPutImage_descr descr;
    LONG height;
    XImage *image;
    LPBYTE sbuf, startline;
    int	w, h;

    TRACE("(bmp=%p, bits=%p, count=0x%lx)\n", bmp, bits, count);
    
    height = count / bmp->bitmap.bmWidthBytes;

    EnterCriticalSection( &X11DRV_CritSection );
    image = XCreateImage( display, X11DRV_GetVisual(), bmp->bitmap.bmBitsPixel, ZPixmap, 0, NULL,
                          bmp->bitmap.bmWidth, height, 32, 0 );
    image->data = (LPBYTE)xmalloc(image->bytes_per_line * height);
    
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

    descr.bmp    = bmp;
    descr.image  = image;
    descr.width  = bmp->bitmap.bmWidth;
    descr.height = height;

    CALL_LARGE_STACK( XPutImage_wrapper, &descr );
    XDestroyImage( image ); /* frees image->data too */
    LeaveCriticalSection( &X11DRV_CritSection );
    
    return count;
}

/***********************************************************************
 *           X11DRV_BitmapBits
 */
LONG X11DRV_BitmapBits(HBITMAP hbitmap, void *bits, LONG count, WORD flags)
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG ret;
    if(!bmp) {
        WARN("Bad bitmap handle %08x\n", hbitmap);
	return FALSE;
    }

    if(flags == DDB_GET)
        ret = X11DRV_GetBitmapBits(bmp, bits, count);
    else if(flags == DDB_SET)
        ret = X11DRV_SetBitmapBits(bmp, bits, count);
    else {
        ERR("Unknown flags value %d\n", flags);
	ret = 0;
    }
    
    GDI_HEAP_UNLOCK( hbitmap );
    return ret;
}

/***********************************************************************
 *           X11DRV_BITMAP_DeleteObject
 */
BOOL X11DRV_BITMAP_DeleteObject( HBITMAP hbitmap, BITMAPOBJ * bmp )
{
    TSXFreePixmap( display, (Pixmap)bmp->physBitmap );
    bmp->physBitmap = NULL;
    bmp->funcs = NULL;
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
    if ( 0 == TSXGetGeometry(display, pixmap, &root, &x, &y, &width, &height,
                             &border_width, &depth) )
        goto END;

    TRACE("\tPixmap properties: width=%d, height=%d, depth=%d\n",
          width, height, depth);
    
    /*
     * Create an HBITMAP with the same dimensions and BPP as the pixmap,
     * and make it a container for the pixmap passed.
     */
    hBmp = CreateBitmap( width, height, 1, depth, NULL );

    pBmp = (BITMAPOBJ *)GDI_GetObjPtr( hBmp, BITMAP_MAGIC );
    
    pBmp->funcs = &X11DRV_DC_Funcs;
    pBmp->physBitmap = (void *)pixmap;

END:
    TRACE("\tReturning HBITMAP %x\n", hBmp);
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
                 
    hBmpCopy = CopyImage(hBmp, IMAGE_BITMAP, width, height, LR_CREATEDIBSECTION);

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
    TRACE("\tReturning HBITMAP %x\n", hBmpCopy);
    return hBmpCopy;
}


/**************************************************************************
 *	           X11DRV_BITMAP_CreatePixmapFromBitmap
 *
 *    Creates a Pixmap from a bitmap
 */
Pixmap X11DRV_BITMAP_CreatePixmapFromBitmap( HBITMAP hBmp, HDC hdc )
{
    HGLOBAL hPackedDIB = NULL;
    Pixmap pixmap = NULL;

    /*
     * Create a packed DIB from the bitmap passed to us.
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */
    hPackedDIB = DIB_CreateDIBFromBitmap(hdc, hBmp);

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
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    return (Pixmap)bmp->physBitmap;
}

#endif /* !defined(X_DISPLAY_MISSING) */
