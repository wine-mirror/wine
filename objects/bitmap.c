/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdi.h"
#include "bitmap.h"
#include "stddebug.h"
/* #define DEBUG_GDI    /* */
/* #undef  DEBUG_GDI    /* */
/* #define DEBUG_BITMAP /* */
/* #define DEBUG_BITMAP /* */
#include "debug.h"

  /* GCs used for B&W and color bitmap operations */
GC BITMAP_monoGC = 0, BITMAP_colorGC = 0;

extern void DC_InitDC( HDC hdc );                /* objects/dc.c */
extern void CLIPPING_UpdateGCRegion( DC * dc );  /* objects/clipping.c */

/***********************************************************************
 *           BITMAP_Init
 */
BOOL BITMAP_Init()
{
    Pixmap tmpPixmap;
    
      /* Create the necessary GCs */
    
    if ((tmpPixmap = XCreatePixmap( display, rootWindow, 1, 1, 1 )))
    {
	BITMAP_monoGC = XCreateGC( display, tmpPixmap, 0, NULL );
	XSetGraphicsExposures( display, BITMAP_monoGC, False );
	XFreePixmap( display, tmpPixmap );
    }

    if (screenDepth != 1)
    {
	if ((tmpPixmap = XCreatePixmap(display, rootWindow, 1,1,screenDepth)))
	{
	    BITMAP_colorGC = XCreateGC( display, tmpPixmap, 0, NULL );
	    XSetGraphicsExposures( display, BITMAP_colorGC, False );
	    XFreePixmap( display, tmpPixmap );
	}
    }
    return TRUE;
}


/***********************************************************************
 *           BITMAP_BmpToImage
 *
 * Create an XImage pointing to the bitmap data.
 */
static XImage *BITMAP_BmpToImage( BITMAP * bmp, void * bmpData )
{
    extern void _XInitImageFuncPtrs( XImage* );
    XImage * image;

    image = XCreateImage( display, DefaultVisualOfScreen(screen),
			  bmp->bmBitsPixel, ZPixmap, 0, bmpData,
			  bmp->bmWidth, bmp->bmHeight, 16, bmp->bmWidthBytes );
    if (!image) return 0;
    image->byte_order = MSBFirst;
    image->bitmap_bit_order = MSBFirst;
    image->bitmap_unit = 16;
    _XInitImageFuncPtrs(image);
    return image;
}


/***********************************************************************
 *           CreateBitmap    (GDI.48)
 */
HBITMAP CreateBitmap( short width, short height, 
		      BYTE planes, BYTE bpp, LPSTR bits )
{
    BITMAP bitmap = { 0, width, height, 0, planes, bpp, bits };
    dprintf_gdi(stddeb, "CreateBitmap: %dx%d, %d colors\n", 
	     width, height, 1 << (planes*bpp) );
    return CreateBitmapIndirect( &bitmap );
}


/***********************************************************************
 *           CreateCompatibleBitmap    (GDI.51)
 */
HBITMAP CreateCompatibleBitmap( HDC hdc, short width, short height )
{
    DC * dc;
    dprintf_gdi(stddeb, "CreateCompatibleBitmap: %d %dx%d\n", 
		hdc, width, height );
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    return CreateBitmap( width, height, 1, dc->w.bitsPerPixel, NULL );
}


/***********************************************************************
 *           CreateBitmapIndirect    (GDI.49)
 */
HBITMAP CreateBitmapIndirect( BITMAP * bmp )
{
    BITMAPOBJ * bmpObjPtr;
    HBITMAP hbitmap;

      /* Check parameters */
    if (!bmp->bmHeight || !bmp->bmWidth) return 0;
    if (bmp->bmPlanes != 1) return 0;
    if ((bmp->bmBitsPixel != 1) && (bmp->bmBitsPixel != screenDepth)) return 0;

    if (bmp->bmHeight < 0)
	bmp->bmHeight = -bmp->bmHeight;
    
    if (bmp->bmWidth < 0)
	bmp->bmWidth = -bmp->bmWidth;
    

      /* Create the BITMAPOBJ */
    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;
    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_ADDR( hbitmap );

    bmpObjPtr->size.cx = 0;
    bmpObjPtr->size.cy = 0;
    bmpObjPtr->bitmap  = *bmp;
    bmpObjPtr->bitmap.bmBits = NULL;
    bmpObjPtr->bitmap.bmWidthBytes = (bmp->bmWidth*bmp->bmBitsPixel+15)/16 * 2;

      /* Create the pixmap */
    bmpObjPtr->pixmap = XCreatePixmap( display, rootWindow, bmp->bmWidth,
				       bmp->bmHeight, bmp->bmBitsPixel );
    if (!bmpObjPtr->pixmap)
    {
	GDI_HEAP_FREE( hbitmap );
	hbitmap = 0;
    }
    else if (bmp->bmBits)  /* Set bitmap bits */
	SetBitmapBits( hbitmap, bmp->bmHeight*bmp->bmWidthBytes, bmp->bmBits );
    return hbitmap;
}


/***********************************************************************
 *           GetBitmapBits    (GDI.74)
 */
LONG GetBitmapBits( HBITMAP hbitmap, LONG count, LPSTR buffer )
{
    BITMAPOBJ * bmp;
    LONG height;
    XImage * image;
    
    bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return 0;

    dprintf_bitmap(stddeb, "GetBitmapBits: %dx%d %d colors %p\n",
	    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, buffer );
      /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    if (!height) return 0;
    
    if (!(image = BITMAP_BmpToImage( &bmp->bitmap, buffer ))) return 0;
    XGetSubImage( display, bmp->pixmap, 0, 0, bmp->bitmap.bmWidth, height,
		  AllPlanes, ZPixmap, image, 0, 0 );
    image->data = NULL;
    XDestroyImage( image );
    return height * bmp->bitmap.bmWidthBytes;
}


/***********************************************************************
 *           SetBitmapBits    (GDI.106)
 */
LONG SetBitmapBits( HBITMAP hbitmap, LONG count, LPSTR buffer )
{
    BITMAPOBJ * bmp;
    LONG height;
    XImage * image;
    
    bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return 0;

    dprintf_bitmap(stddeb, "SetBitmapBits: %dx%d %d colors %p\n",
	    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, buffer );

      /* Only set entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    if (!height) return 0;
    	
    if (!(image = BITMAP_BmpToImage( &bmp->bitmap, buffer ))) return 0;
    XPutImage( display, bmp->pixmap, BITMAP_GC(bmp), image, 0, 0,
	       0, 0, bmp->bitmap.bmWidth, height );
    image->data = NULL;
    XDestroyImage( image );
    return height * bmp->bitmap.bmWidthBytes;
}


/***********************************************************************
 *           BMP_DeleteObject
 */
BOOL BMP_DeleteObject( HBITMAP hbitmap, BITMAPOBJ * bitmap )
{
    XFreePixmap( display, bitmap->pixmap );
    return GDI_FreeObject( hbitmap );
}

	
/***********************************************************************
 *           BMP_GetObject
 */
int BMP_GetObject( BITMAPOBJ * bmp, int count, LPSTR buffer )
{
    if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
    memcpy( buffer, &bmp->bitmap, count );
    return count;
}
    

/***********************************************************************
 *           BITMAP_SelectObject
 */
HBITMAP BITMAP_SelectObject( HDC hdc, DC * dc, HBITMAP hbitmap,
			     BITMAPOBJ * bmp )
{
    HRGN hrgn;
    HBITMAP prevHandle = dc->w.hBitmap;
    
    if (!(dc->w.flags & DC_MEMORY)) return 0;
    hrgn = CreateRectRgn( 0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight );
    if (!hrgn) return 0;

    DeleteObject( dc->w.hVisRgn );
    dc->w.hVisRgn    = hrgn;
    dc->u.x.drawable = bmp->pixmap;
    dc->w.hBitmap    = hbitmap;

      /* Change GC depth if needed */

    if (dc->w.bitsPerPixel != bmp->bitmap.bmBitsPixel)
    {
	XFreeGC( display, dc->u.x.gc );
	dc->u.x.gc = XCreateGC( display, dc->u.x.drawable, 0, NULL );
	dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;
        DC_InitDC( hdc );
    }
    else CLIPPING_UpdateGCRegion( dc );  /* Just update GC clip region */
    return prevHandle;
}

/***********************************************************************
 *           CreateDiscardableBitmap    (GDI.156)
 */
HBITMAP CreateDiscardableBitmap(HDC hdc, short width, short height)
{
    dprintf_bitmap(stddeb,"CreateDiscardableBitmap(%04X, %d, %d); "
	   "// call CreateCompatibleBitmap() for now!\n",
	   hdc, width, height);
    return CreateCompatibleBitmap(hdc, width, height);
}

/***********************************************************************
 *           GetBitmapDimensionEx    (GDI.468)
 */
BOOL GetBitmapDimensionEx( HBITMAP hbitmap, LPSIZE size )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    *size = bmp->size;
    return TRUE;
}


/***********************************************************************
 *           GetBitmapDimension    (GDI.162)
 */
DWORD GetBitmapDimension( HBITMAP hbitmap )
{
    SIZE size;
    if (!GetBitmapDimensionEx( hbitmap, &size )) return 0;
    return size.cx | (size.cy << 16);
}

/***********************************************************************
 *           SetBitmapDimensionEx    (GDI.478)
 */
BOOL SetBitmapDimensionEx( HBITMAP hbitmap, short x, short y, LPSIZE prevSize )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    if (prevSize) *prevSize = bmp->size;
    bmp->size.cx = x;
    bmp->size.cy = y;
    return TRUE;
}


/***********************************************************************
 *           SetBitmapDimension    (GDI.163)
 */
DWORD SetBitmapDimension( HBITMAP hbitmap, short x, short y )
{
    SIZE size;
    if (!SetBitmapDimensionEx( hbitmap, x, y, &size )) return 0;
    return size.cx | (size.cy << 16);    
}
