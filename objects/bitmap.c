/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 */
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdi.h"
#include "callback.h"
#include "dc.h"
#include "bitmap.h"
#include "stddebug.h"
/* #define DEBUG_GDI    */
/* #define DEBUG_BITMAP */
#include "debug.h"

  /* GCs used for B&W and color bitmap operations */
GC BITMAP_monoGC = 0, BITMAP_colorGC = 0;

extern void CLIPPING_UpdateGCRegion( DC * dc );  /* objects/clipping.c */

/***********************************************************************
 *           BITMAP_Init
 */
BOOL BITMAP_Init(void)
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
HBITMAP CreateBitmap( INT width, INT height, UINT planes, UINT bpp, LPVOID bits )
{
    BITMAPOBJ * bmpObjPtr;
    HBITMAP hbitmap;

    dprintf_gdi( stddeb, "CreateBitmap: %dx%d, %d colors\n", 
                 width, height, 1 << (planes*bpp) );

      /* Check parameters */
    if (!height || !width || planes != 1) return 0;
    if ((bpp != 1) && (bpp != screenDepth)) return 0;
    if (height < 0) height = -height;
    if (width < 0) width = -width;

      /* Create the BITMAPOBJ */
    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;
    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_LIN_ADDR( hbitmap );

    bmpObjPtr->size.cx = 0;
    bmpObjPtr->size.cy = 0;
    bmpObjPtr->bitmap.bmType = 0;
    bmpObjPtr->bitmap.bmWidth = width;
    bmpObjPtr->bitmap.bmHeight = height;
    bmpObjPtr->bitmap.bmPlanes = planes;
    bmpObjPtr->bitmap.bmBitsPixel = bpp;
    bmpObjPtr->bitmap.bmWidthBytes = BITMAP_WIDTH_BYTES( width, bpp );
    bmpObjPtr->bitmap.bmBits = NULL;

      /* Create the pixmap */
    bmpObjPtr->pixmap = XCreatePixmap(display, rootWindow, width, height, bpp);
    if (!bmpObjPtr->pixmap)
    {
	GDI_HEAP_FREE( hbitmap );
	hbitmap = 0;
    }
    else if (bits)  /* Set bitmap bits */
	SetBitmapBits( hbitmap, height * bmpObjPtr->bitmap.bmWidthBytes, bits);
    return hbitmap;
}


/***********************************************************************
 *           CreateCompatibleBitmap    (GDI.51)
 */
HBITMAP CreateCompatibleBitmap( HDC hdc, INT width, INT height )
{
    HBITMAP	hbmpRet = 0;
    DC * 	dc;
    dprintf_gdi(stddeb, "CreateCompatibleBitmap(%04x,%d,%d) = \n", 
		hdc, width, height );
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    
    hbmpRet = CreateBitmap( width, height, 1, dc->w.bitsPerPixel, NULL );

    dprintf_gdi(stddeb,"\t\t%04x\n", hbmpRet);

    return hbmpRet;
}


/***********************************************************************
 *           CreateBitmapIndirect    (GDI.49)
 */
HBITMAP CreateBitmapIndirect( const BITMAP * bmp )
{
    return CreateBitmap( bmp->bmWidth, bmp->bmHeight, bmp->bmPlanes,
                         bmp->bmBitsPixel, PTR_SEG_TO_LIN( bmp->bmBits ) );
}


/***********************************************************************
 *           GetBitmapBits    (GDI.74)
 */
LONG GetBitmapBits( HBITMAP hbitmap, LONG count, LPSTR buffer )
{
    BITMAPOBJ * bmp;
    LONG height;
    XImage * image;
    
    /* KLUDGE! */
    if (count < 0) {
	fprintf(stderr, "Negative number of bytes (%ld) passed to GetBitmapBits???\n", count );
	count = -count;
    }
    bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return 0;

      /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    dprintf_bitmap(stddeb, "GetBitmapBits: %dx%d %d colors %p fetched height: %ld\n",
	    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, buffer, height );
    if (!height) return 0;
    
    if (!(image = BITMAP_BmpToImage( &bmp->bitmap, buffer ))) return 0;
    CallTo32_LargeStack( (int(*)())XGetSubImage, 11,
                         display, bmp->pixmap, 0, 0, bmp->bitmap.bmWidth,
                         height, AllPlanes, ZPixmap, image, 0, 0 );
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
    
    /* KLUDGE! */
    if (count < 0) {
	fprintf(stderr, "Negative number of bytes (%ld) passed to SetBitmapBits???\n", count );
	count = -count;
    }
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
    CallTo32_LargeStack( XPutImage, 10,
                         display, bmp->pixmap, BITMAP_GC(bmp), image, 0, 0,
                         0, 0, bmp->bitmap.bmWidth, height );
    image->data = NULL;
    XDestroyImage( image );
    return height * bmp->bitmap.bmWidthBytes;
}


/**********************************************************************
 *	    LoadBitmap    (USER.175)
 */
HBITMAP LoadBitmap( HANDLE instance, SEGPTR name )
{
    HBITMAP hbitmap = 0;
    HDC hdc;
    HRSRC hRsrc;
    HGLOBAL handle;
    BITMAPINFO *info;

    if (HIWORD(name))
    {
        char *str = (char *)PTR_SEG_TO_LIN( name );
        dprintf_bitmap( stddeb, "LoadBitmap(%04x,'%s')\n", instance, str );
        if (str[0] == '#') name = (SEGPTR)(DWORD)(WORD)atoi( str + 1 );
    }
    else
        dprintf_bitmap( stddeb, "LoadBitmap(%04x,%04x)\n",
                        instance, LOWORD(name) );

    if (!instance)  /* OEM bitmap */
    {
        if (HIWORD((int)name)) return 0;
        return OBM_LoadBitmap( LOWORD((int)name) );
    }

    if (!(hRsrc = FindResource( instance, name, RT_BITMAP ))) return 0;
    if (!(handle = LoadResource( instance, hRsrc ))) return 0;

    info = (BITMAPINFO *)LockResource( handle );
    if ((hdc = GetDC(0)) != 0)
    {
        char *bits = (char *)info + DIB_BitmapInfoSize( info, DIB_RGB_COLORS );
        hbitmap = CreateDIBitmap( hdc, &info->bmiHeader, CBM_INIT,
                                  bits, info, DIB_RGB_COLORS );
        ReleaseDC( 0, hdc );
    }
    FreeResource( handle );
    return hbitmap;
}


/***********************************************************************
 *           BITMAP_DeleteObject
 */
BOOL BITMAP_DeleteObject( HBITMAP hbitmap, BITMAPOBJ * bitmap )
{
    XFreePixmap( display, bitmap->pixmap );
    return GDI_FreeObject( hbitmap );
}

	
/***********************************************************************
 *           BITMAP_GetObject
 */
int BITMAP_GetObject( BITMAPOBJ * bmp, int count, LPSTR buffer )
{
    if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
    memcpy( buffer, &bmp->bitmap, count );
    return count;
}
    

/***********************************************************************
 *           BITMAP_SelectObject
 */
HBITMAP BITMAP_SelectObject( DC * dc, HBITMAP hbitmap,
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
        DC_InitDC( dc );
    }
    else CLIPPING_UpdateGCRegion( dc );  /* Just update GC clip region */
    return prevHandle;
}

/***********************************************************************
 *           CreateDiscardableBitmap    (GDI.156)
 */
HBITMAP CreateDiscardableBitmap(HDC hdc, INT width, INT height)
{
    dprintf_bitmap(stddeb,"CreateDiscardableBitmap(%04x, %d, %d); "
	   "// call CreateCompatibleBitmap() for now!\n",
	   hdc, width, height);
    return CreateCompatibleBitmap(hdc, width, height);
}


/***********************************************************************
 *           GetBitmapDimensionEx16    (GDI.468)
 */
BOOL16 GetBitmapDimensionEx16( HBITMAP16 hbitmap, LPSIZE16 size )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    *size = bmp->size;
    return TRUE;
}


/***********************************************************************
 *           GetBitmapDimensionEx32    (GDI.468)
 */
BOOL32 GetBitmapDimensionEx32( HBITMAP32 hbitmap, LPSIZE32 size )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    size->cx = (INT32)bmp->size.cx;
    size->cy = (INT32)bmp->size.cy;
    return TRUE;
}


/***********************************************************************
 *           GetBitmapDimension    (GDI.162)
 */
DWORD GetBitmapDimension( HBITMAP16 hbitmap )
{
    SIZE16 size;
    if (!GetBitmapDimensionEx16( hbitmap, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetBitmapDimensionEx16    (GDI.478)
 */
BOOL16 SetBitmapDimensionEx16( HBITMAP16 hbitmap, INT16 x, INT16 y,
                               LPSIZE16 prevSize )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    if (prevSize) *prevSize = bmp->size;
    bmp->size.cx = x;
    bmp->size.cy = y;
    return TRUE;
}


/***********************************************************************
 *           SetBitmapDimensionEx32    (GDI.478)
 */
BOOL32 SetBitmapDimensionEx32( HBITMAP32 hbitmap, INT32 x, INT32 y,
                               LPSIZE32 prevSize )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    if (prevSize) CONV_SIZE16TO32( &bmp->size, prevSize );
    bmp->size.cx = (INT16)x;
    bmp->size.cy = (INT16)y;
    return TRUE;
}


/***********************************************************************
 *           SetBitmapDimension    (GDI.163)
 */
DWORD SetBitmapDimension( HBITMAP16 hbitmap, INT16 x, INT16 y )
{
    SIZE16 size;
    if (!SetBitmapDimensionEx16( hbitmap, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}
