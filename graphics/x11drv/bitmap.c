/*
 * X11DRV bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
#include "gdi.h"
#include "callback.h"
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "debug.h"
#include "xmalloc.h"
#include "x11drv.h"

#ifdef PRELIMINARY_WING16_SUPPORT
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif


  /* GCs used for B&W and color bitmap operations */
GC BITMAP_monoGC = 0, BITMAP_colorGC = 0;


/***********************************************************************
 *           X11DRV_BITMAP_Init
 */
BOOL32 X11DRV_BITMAP_Init(void)
{
    Pixmap tmpPixmap;
    
      /* Create the necessary GCs */
    
    if ((tmpPixmap = TSXCreatePixmap( display, rootWindow, 1, 1, 1 )))
    {
	BITMAP_monoGC = TSXCreateGC( display, tmpPixmap, 0, NULL );
	TSXSetGraphicsExposures( display, BITMAP_monoGC, False );
	TSXFreePixmap( display, tmpPixmap );
    }

    if (screenDepth != 1)
    {
	if ((tmpPixmap = TSXCreatePixmap(display, rootWindow, 1,1,screenDepth)))
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
HBITMAP32 X11DRV_BITMAP_SelectObject( DC * dc, HBITMAP32 hbitmap,
                                      BITMAPOBJ * bmp )
{
    HRGN32 hrgn;
    HBITMAP32 prevHandle = dc->w.hBitmap;
    X11DRV_PHYSBITMAP *pbitmap;

    if (!(dc->w.flags & DC_MEMORY)) return 0;

    if(!bmp->DDBitmap)
        if(!X11DRV_CreateBitmap(hbitmap))
	    return 0;

    if(bmp->DDBitmap->funcs != dc->funcs) {
        WARN(bitmap, "Trying to select non-X11 DDB into an X11 dc\n");
	return 0;
    }

    pbitmap = bmp->DDBitmap->physBitmap;

    dc->w.totalExtent.left   = 0;
    dc->w.totalExtent.top    = 0;
    dc->w.totalExtent.right  = bmp->bitmap.bmWidth;
    dc->w.totalExtent.bottom = bmp->bitmap.bmHeight;

    if (dc->w.hVisRgn)
       SetRectRgn32( dc->w.hVisRgn, 0, 0,
                     bmp->bitmap.bmWidth, bmp->bitmap.bmHeight );
    else
    { 
       hrgn = CreateRectRgn32(0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight);
       if (!hrgn) return 0;
       dc->w.hVisRgn    = hrgn;
    }

    dc->u.x.drawable = pbitmap->pixmap;
    dc->w.hBitmap    = hbitmap;

      /* Change GC depth if needed */

    if (dc->w.bitsPerPixel != bmp->bitmap.bmBitsPixel)
    {
	TSXFreeGC( display, dc->u.x.gc );
	dc->u.x.gc = TSXCreateGC( display, dc->u.x.drawable, 0, NULL );
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
    INT32      width;
    INT32      height;
};

static int XPutImage_wrapper( const struct XPutImage_descr *descr )
{
    return XPutImage( display,
	       ((X11DRV_PHYSBITMAP *)descr->bmp->DDBitmap->physBitmap)->pixmap,
	       BITMAP_GC(descr->bmp),
	       descr->image, 0, 0, 0, 0, descr->width, descr->height );
}


/***************************************************************************
 *
 *	X11DRV_AllocBitmap
 *
 * Allocate DDBitmap and physBitmap
 *
 */
X11DRV_PHYSBITMAP *X11DRV_AllocBitmap( BITMAPOBJ *bmp )
{
    X11DRV_PHYSBITMAP *pbitmap;

    if(!(bmp->DDBitmap = HeapAlloc(GetProcessHeap(), 0, sizeof(DDBITMAP)))) {
        WARN(bitmap, "Can't alloc DDBITMAP\n");
	return NULL;
    }

    if(!(pbitmap = HeapAlloc(GetProcessHeap(), 0,sizeof(X11DRV_PHYSBITMAP)))) {
        WARN(bitmap, "Can't alloc X11DRV_PHYSBITMAP\n");
        HeapFree(GetProcessHeap(), 0, bmp->DDBitmap);
	return NULL;
    }

    bmp->DDBitmap->physBitmap = pbitmap;
    bmp->DDBitmap->funcs = DRIVER_FindDriver( "DISPLAY" );

    return pbitmap;
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

BOOL32 X11DRV_CreateBitmap( HBITMAP32 hbitmap )
{
    X11DRV_PHYSBITMAP *pbitmap;
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );

    if(!bmp) {
        WARN(bitmap, "Bad bitmap handle %08x\n", hbitmap);
	return FALSE;
    }

      /* Check parameters */
    if (bmp->bitmap.bmPlanes != 1) return 0;
    if ((bmp->bitmap.bmBitsPixel != 1) && 
       (bmp->bitmap.bmBitsPixel != screenDepth)) {
        GDI_HEAP_UNLOCK( hbitmap );
	return FALSE;
    }

    pbitmap = X11DRV_AllocBitmap( bmp );
    if(!pbitmap) return FALSE;

      /* Create the pixmap */
    pbitmap->pixmap = TSXCreatePixmap(display, rootWindow, bmp->bitmap.bmWidth,
			      bmp->bitmap.bmHeight, bmp->bitmap.bmBitsPixel);
    if (!pbitmap->pixmap) {
        WARN(bitmap, "Can't create Pixmap\n");
        HeapFree(GetProcessHeap(), 0, bmp->DDBitmap->physBitmap);
        HeapFree(GetProcessHeap(), 0, bmp->DDBitmap);
	GDI_HEAP_UNLOCK( hbitmap );
	return FALSE;
    }

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
    return XGetImage( display,
		      ((X11DRV_PHYSBITMAP *)bmp->DDBitmap->physBitmap)->pixmap,
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
    LPBYTE tbuf;
    int	h, w, pad;

    pad = BITMAP_GetBitsPadding(bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel);

    if (pad == -1)
        return 0;

    EnterCriticalSection( &X11DRV_CritSection );

    /* Hack: change the bitmap height temporarily to avoid */
    /*       getting unnecessary bitmap rows. */

    old_height = bmp->bitmap.bmHeight;
    height = bmp->bitmap.bmHeight = count / bmp->bitmap.bmWidthBytes;

    image = (XImage *)CALL_LARGE_STACK( X11DRV_BITMAP_GetXImage, bmp );

    bmp->bitmap.bmHeight = old_height;

    /* copy XImage to 16 bit padded image buffer with real bitsperpixel */

    tbuf = buffer;
    switch (bmp->bitmap.bmBitsPixel)
    {
    case 1:
        for (h=0;h<height;h++)
        {
            *tbuf = 0;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if ((w%8) == 0)
                    *tbuf = 0;
                *tbuf |= XGetPixel(image,w,h)<<(7-(w&7));
                if ((w&7) == 7) ++tbuf;
            }
            tbuf += pad;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if (!(w & 1)) *tbuf = XGetPixel( image, w, h) << 4;
	    	else *tbuf++ |= XGetPixel( image, w, h) & 0x0f;
            }
            tbuf += pad;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
                *tbuf++ = XGetPixel(image,w,h);
            tbuf += pad;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>>8) & 0xff;
            }
        }
        break;
    case 24:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>> 8) & 0xff;
		*tbuf++ = (pixel>>16) & 0xff;
	    }
            tbuf += pad;
	}
        break;

    case 32:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>> 8) & 0xff;
		*tbuf++ = (pixel>>16) & 0xff;
		*tbuf++ = (pixel>>24) & 0xff;
	    }
            tbuf += pad;
	}
        break;
    default:
        FIXME(bitmap, "Unhandled bits:%d\n", bmp->bitmap.bmBitsPixel);
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
    LPBYTE sbuf, tmpbuffer;
    int	w, h, pad, widthbytes;

    TRACE(x11drv, "(bmp=%p, bits=%p, count=%lx)\n", bmp, bits, count);
    
    pad = BITMAP_GetBitsPadding(bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel);

    if (pad == -1) 
        return 0;
	
    sbuf = (LPBYTE)bits;

    widthbytes	= (((bmp->bitmap.bmWidth * bmp->bitmap.bmBitsPixel) + 31) /
		   32) * 4;
    height      = count / bmp->bitmap.bmWidthBytes;
    tmpbuffer	= (LPBYTE)xmalloc(widthbytes*height);

    TRACE(x11drv, "h=%ld, w=%d, wb=%d\n", height, bmp->bitmap.bmWidth, 
	  widthbytes);

    EnterCriticalSection( &X11DRV_CritSection );
    image = XCreateImage( display, DefaultVisualOfScreen(screen),
                          bmp->bitmap.bmBitsPixel, ZPixmap, 0, tmpbuffer,
                          bmp->bitmap.bmWidth,height,32,widthbytes );
    
    /* copy 16 bit padded image buffer with real bitsperpixel to XImage */
    sbuf = (LPBYTE)bits;
    switch (bmp->bitmap.bmBitsPixel)
    {
    case 1:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[0]>>(7-(w&7))) & 1);
                if ((w&7) == 7)
                    sbuf++;
            }
            sbuf += pad;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if (!(w & 1)) XPutPixel( image, w, h, *sbuf >> 4 );
                else XPutPixel( image, w, h, *sbuf++ & 0xf );
            }
            sbuf += pad;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
                XPutPixel(image,w,h,*sbuf++);
            sbuf += pad;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,sbuf[1]*256+sbuf[0]);
                sbuf+=2;
            }
        }
        break;
    case 24: 
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[2]<<16)+(sbuf[1]<<8)+sbuf[0]);
                sbuf += 3;
            }
            sbuf += pad;
        }
        break;
    case 32: 
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[3]<<24)+(sbuf[2]<<16)+(sbuf[1]<<8)+sbuf[0]);
                sbuf += 4;
            }
            sbuf += pad;
        }
        break;
    default:
      FIXME(bitmap, "Unhandled bits:%d\n", bmp->bitmap.bmBitsPixel);

    }

    descr.bmp    = bmp;
    descr.image  = image;
    descr.width  = bmp->bitmap.bmWidth;
    descr.height = height;
    CALL_LARGE_STACK( XPutImage_wrapper, &descr );
    XDestroyImage( image ); /* frees tmpbuffer too */
    LeaveCriticalSection( &X11DRV_CritSection );
    
    return count;
}

/***********************************************************************
 *           X11DRV_BitmapBits
 */
LONG X11DRV_BitmapBits(HBITMAP32 hbitmap, void *bits, LONG count, WORD flags)
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG ret;
    if(!bmp) {
        WARN(bitmap, "Bad bitmap handle %08x\n", hbitmap);
	return FALSE;
    }

    if(flags == DDB_GET)
        ret = X11DRV_GetBitmapBits(bmp, bits, count);
    else if(flags == DDB_SET)
        ret = X11DRV_SetBitmapBits(bmp, bits, count);
    else {
        ERR(bitmap, "Unknown flags value %d\n", flags);
	ret = 0;
    }
    
    GDI_HEAP_UNLOCK( hbitmap );
    return ret;
}

/***********************************************************************
 *           X11DRV_BITMAP_DeleteObject
 */
BOOL32 X11DRV_BITMAP_DeleteObject( HBITMAP32 hbitmap, BITMAPOBJ * bmp )
{
    X11DRV_PHYSBITMAP *pbitmap = bmp->DDBitmap->physBitmap;

#ifdef PRELIMINARY_WING16_SUPPORT
    if( bmp->bitmap.bmBits )
 	TSXShmDetach( display, (XShmSegmentInfo*)bmp->bitmap.bmBits );
#endif

    TSXFreePixmap( display, pbitmap->pixmap );


#ifdef PRELIMINARY_WING16_SUPPORT
    if( bmp->bitmap.bmBits )
    {
    __ShmBitmapCtl* p = (__ShmBitmapCtl*)bmp->bitmap.bmBits;
      WORD          sel = HIWORD(p->bits);
      unsigned long l, limit = GetSelectorLimit(sel);

      for( l = 0; l < limit; l += 0x10000, sel += __AHINCR )
	   FreeSelector(sel);
      shmctl(p->si.shmid, IPC_RMID, NULL); 
      shmdt(p->si.shmaddr);  /* already marked for destruction */
    }
#endif

    HeapFree( GetProcessHeap(), 0, bmp->DDBitmap->physBitmap );
    HeapFree( GetProcessHeap(), 0, bmp->DDBitmap );
    bmp->DDBitmap = NULL;

    return TRUE;
}
