/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdi.h"
#include "bitmap.h"

  /* Include OEM bitmaps */
#include "bitmaps/check_boxes"
#include "bitmaps/check_mark"
#include "bitmaps/menu_arrow"

  /* Handle of the bitmap selected by default in a memory DC */
HBITMAP BITMAP_hbitmapMemDC = 0;

  /* GCs used for B&W and color bitmap operations */
GC BITMAP_monoGC = 0, BITMAP_colorGC = 0;


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

    BITMAP_hbitmapMemDC = CreateBitmap( 1, 1, 1, 1, NULL );
    return (BITMAP_hbitmapMemDC != 0);
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
 *           BITMAP_LoadOEMBitmap
 */
HBITMAP BITMAP_LoadOEMBitmap( WORD id )
{
    BITMAPOBJ * bmpObjPtr;
    HBITMAP hbitmap;
    WORD width, height;
    char *data;

    switch(id)
    {
    case OBM_MNARROW:
	width  = menu_arrow_width;
	height = menu_arrow_height;
	data   = menu_arrow_bits;
	break;

    case OBM_CHECKBOXES:
	width  = check_boxes_width;
	height = check_boxes_height;
	data   = check_boxes_bits;
	break;
	
    case OBM_CHECK:
	width  = check_mark_width;
	height = check_mark_height;
	data   = check_mark_bits;
	break;

    default:
	return 0;
    }
    
      /* Create the BITMAPOBJ */
    if (!(hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC )))
	return 0;
    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_ADDR( hbitmap );
    bmpObjPtr->size.cx = 0;
    bmpObjPtr->size.cy = 0;
    bmpObjPtr->bitmap.bmType       = 0;
    bmpObjPtr->bitmap.bmWidth      = width;
    bmpObjPtr->bitmap.bmHeight     = height;
    bmpObjPtr->bitmap.bmWidthBytes = (width + 15) / 16 * 2;
    bmpObjPtr->bitmap.bmPlanes     = 1;
    bmpObjPtr->bitmap.bmBitsPixel  = 1;
    bmpObjPtr->bitmap.bmBits       = NULL;

      /* Create the pixmap */
    if (!(bmpObjPtr->pixmap = XCreateBitmapFromData( display, rootWindow,
						     data, width, height )))
    {
	GDI_HEAP_FREE( hbitmap );
	return 0;
    }
    return hbitmap;
}


/***********************************************************************
 *           CreateBitmap    (GDI.48)
 */
HBITMAP CreateBitmap( short width, short height, 
		      BYTE planes, BYTE bpp, LPSTR bits )
{
    BITMAP bitmap = { 0, width, height, 0, planes, bpp, bits };
#ifdef DEBUG_GDI
    printf( "CreateBitmap: %dx%d, %d colors\n", 
	     width, height, 1 << (planes*bpp) );
#endif
    return CreateBitmapIndirect( &bitmap );
}


/***********************************************************************
 *           CreateCompatibleBitmap    (GDI.51)
 */
HBITMAP CreateCompatibleBitmap( HDC hdc, short width, short height )
{
    DC * dc;
#ifdef DEBUG_GDI
    printf( "CreateCompatibleBitmap: %d %dx%d\n", hdc, width, height );
#endif
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

#ifdef DEBUG_BITMAP
    printf( "GetBitmapBits: %dx%d %d colors %p\n",
	    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, buffer );
#endif
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

#ifdef DEBUG_BITMAP
    printf( "SetBitmapBits: %dx%d %d colors %p\n",
	    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, buffer );
#endif
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
    HBITMAP prevHandle = dc->w.hBitmap;
    
    if (!(dc->w.flags & DC_MEMORY)) return 0;
    dc->u.x.drawable = bmp->pixmap;
    dc->w.DCSizeX    = bmp->bitmap.bmWidth;
    dc->w.DCSizeY    = bmp->bitmap.bmHeight;
    dc->w.hBitmap    = hbitmap;

      /* Change GC depth if needed */

    if (dc->w.bitsPerPixel != bmp->bitmap.bmBitsPixel)
    {
	XFreeGC( display, dc->u.x.gc );
	dc->u.x.gc = XCreateGC( display, dc->u.x.drawable, 0, NULL );
	dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;
	  /* Re-select objects with changed depth */
	SelectObject( hdc, dc->w.hPen );
	SelectObject( hdc, dc->w.hBrush );
    }
    return prevHandle;
}

/***********************************************************************
 *           CreateDiscardableBitmap    (GDI.156)
 */
HBITMAP CreateDiscardableBitmap(HDC hdc, short width, short height)
{
    printf("CreateDiscardableBitmap(%04X, %d, %d); "
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
