/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "gdi.h"

/* A GDI bitmap object contains a handle to a packed BITMAP,
 * which is stored on the global heap.
 * A packed BITMAP is a BITMAP structure followed by the bitmap bits.
 */

  /* Handle of the bitmap selected by default in a memory DC */
HBITMAP BITMAP_hbitmapMemDC;

  /* List of supported depths */
static int depthCount;
static int * depthList;

  /* List of GC used for bitmap to pixmap operations (one for each depth) */
static GC * bitmapGC;


/***********************************************************************
 *           BITMAP_Init
 */
BOOL BITMAP_Init()
{
    int i;
    Pixmap tmpPixmap;
    
    depthList = XListDepths( XT_display, DefaultScreen(XT_display), 
			     &depthCount );
    if (!depthList || !depthCount) return FALSE;
    if (!(bitmapGC = (GC *) malloc( depthCount * sizeof(GC) ))) return FALSE;
    
      /* Create the necessary GCs */
    
    for (i = 0; i < depthCount; i++)
    {
	tmpPixmap = XCreatePixmap( XT_display, DefaultRootWindow(XT_display),
				  1, 1, depthList[i] );
	if (tmpPixmap)
	{
	    bitmapGC[i] = XCreateGC( XT_display, tmpPixmap, 0, NULL );
	    XSetGraphicsExposures( XT_display, bitmapGC[i], False );
	    XFreePixmap( XT_display, tmpPixmap );
	}
	else bitmapGC[i] = 0;
    }

    BITMAP_hbitmapMemDC = CreateBitmap( 1, 1, 1, 1, NULL );
    return (BITMAP_hbitmapMemDC != 0);
}


/***********************************************************************
 *           BITMAP_FindGCForDepth
 *
 * Return a GC appropriate for operations with the given depth.
 */
GC BITMAP_FindGCForDepth( int depth )
{
    int i;
    for (i = 0; i < depthCount; i++)
	if (depthList[i] == depth) return bitmapGC[i];
    return 0;
}


/***********************************************************************
 *           BITMAP_BmpToImage
 *
 * Create an XImage pointing to the bitmap data.
 */
XImage * BITMAP_BmpToImage( BITMAP * bmp, void * bmpData )
{
    XImage * image;

    image = XCreateImage( XT_display, DefaultVisualOfScreen(XT_screen),
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
 *           BITMAP_CopyToPixmap
 *
 * Copy the content of the bitmap to the pixmap. Both must have the same depth.
 */
BOOL BITMAP_CopyToPixmap( BITMAP * bmp, Pixmap pixmap,
			  int x, int y, int width, int height )
{
    GC gc;
    XImage * image;
    
    gc = BITMAP_FindGCForDepth( bmp->bmBitsPixel );
    if (!gc) return FALSE;
    
    image = BITMAP_BmpToImage( bmp, ((char *)bmp) + sizeof(BITMAP) );
    if (!image) return FALSE;

#ifdef DEBUG_GDI
    printf( "BITMAP_CopyToPixmap: %dx%d %d colors -> %d,%d %dx%d\n",
	    bmp->bmWidth, bmp->bmHeight, 1 << bmp->bmBitsPixel, x, y, width, height );
#endif
    XPutImage(XT_display, pixmap, gc, image, 0, 0, x, y, width, height); 
    image->data = NULL;
    XDestroyImage( image );
    return TRUE;
}


/***********************************************************************
 *           BITMAP_CopyFromPixmap
 *
 * Copy the content of the pixmap to the bitmap. Both must have
 * the same dimensions and depth.
 */
BOOL BITMAP_CopyFromPixmap( BITMAP * bmp, Pixmap pixmap )
{
    XImage *image = BITMAP_BmpToImage( bmp, ((char *)bmp) + sizeof(BITMAP) );
    if (!image) return FALSE;

#ifdef DEBUG_GDI
    printf( "BITMAP_CopyFromPixmap: %dx%d %d colors\n",
	    bmp->bmWidth, bmp->bmHeight, 1 << bmp->bmBitsPixel );
#endif    
    XGetSubImage( XT_display, pixmap, 0, 0, bmp->bmWidth, bmp->bmHeight,
		  AllPlanes, ZPixmap, image, 0, 0 );
    image->data = NULL;
    XDestroyImage( image );
    return TRUE;
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
    if (!width || !height) return 0;
    if ((planes != 1) && (bpp != 1)) return 0;
    bitmap.bmWidthBytes = (width * bpp + 15) / 16 * 2;
    return CreateBitmapIndirect( &bitmap );
}


/***********************************************************************
 *           CreateCompatibleBitmap    (GDI.51)
 */
HBITMAP CreateCompatibleBitmap( HDC hdc, short width, short height )
{
    HBITMAP hbitmap;
    DC * dc;
#ifdef DEBUG_GDI
    printf( "CreateCompatibleBitmap: %d %dx%d\n", hdc, width, height );
#endif
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    hbitmap = CreateBitmap( width, height, dc->w.planes, dc->w.bitsPerPixel, NULL);
    return hbitmap;
}


/***********************************************************************
 *           CreateBitmapIndirect    (GDI.49)
 */
HBITMAP CreateBitmapIndirect( BITMAP * bmp )
{
    BITMAPOBJ * bmpObjPtr;
    char * bmpPtr;
    HBITMAP hbitmap;
    int size = bmp->bmPlanes * bmp->bmHeight * bmp->bmWidthBytes;
    
      /* Create the BITMAPOBJ */

    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;
    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_ADDR( hbitmap );
    
      /* Create the bitmap in global heap */

    bmpObjPtr->hBitmap = GlobalAlloc( GMEM_MOVEABLE, sizeof(BITMAP) + size );
    if (!bmpObjPtr->hBitmap)
    {
	GDI_FreeObject( hbitmap );
	return 0;
    }
    bmpPtr = (char *) GlobalLock( bmpObjPtr->hBitmap );
    memcpy( bmpPtr, bmp, sizeof(BITMAP) );
    ((BITMAP *)bmpPtr)->bmBits = NULL;
    if (bmp->bmBits) memcpy( bmpPtr + sizeof(BITMAP), bmp->bmBits, size );
    GlobalUnlock( bmpObjPtr->hBitmap );

    bmpObjPtr->bSelected = FALSE;
    bmpObjPtr->hdc       = 0;
    bmpObjPtr->size.cx   = 0;
    bmpObjPtr->size.cy   = 0;
    return hbitmap;
}


/***********************************************************************
 *           BITMAP_GetSetBitmapBits
 */
LONG BITMAP_GetSetBitmapBits( HBITMAP hbitmap, LONG count,
			      LPSTR buffer, int set )
{
    BITMAPOBJ * bmpObjPtr;
    BITMAP * bmp;
    DC * dc = NULL;
    int maxSize;
    
    if (!count) return 0;
    bmpObjPtr = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmpObjPtr) return 0;
    if (!(bmp = (BITMAP *) GlobalLock( bmpObjPtr->hBitmap ))) return 0;
    
    if (bmpObjPtr->bSelected) 
	dc = (DC *) GDI_GetObjPtr( bmpObjPtr->hdc, DC_MAGIC );

    maxSize = bmp->bmPlanes * bmp->bmHeight * bmp->bmWidthBytes;
    if (count > maxSize) count = maxSize;
    	
    if (set)
    {
	memcpy( bmp+1, buffer, count );
	if (dc) BITMAP_CopyToPixmap( bmp, dc->u.x.drawable,
				     0, 0, bmp->bmWidth, bmp->bmHeight );
    }
    else
    {
	if (dc) BITMAP_CopyFromPixmap( bmp, dc->u.x.drawable );
	memcpy( buffer, bmp+1, count );
    }
    GlobalUnlock( bmpObjPtr->hBitmap );
    return hbitmap;
}


/***********************************************************************
 *           GetBitmapBits    (GDI.74)
 */
LONG GetBitmapBits( HBITMAP hbitmap, LONG count, LPSTR buffer )
{
    return BITMAP_GetSetBitmapBits( hbitmap, count, buffer, 0 );
}


/***********************************************************************
 *           SetBitmapBits    (GDI.106)
 */
LONG SetBitmapBits( HBITMAP hbitmap, LONG count, LPSTR buffer )
{
    return BITMAP_GetSetBitmapBits( hbitmap, count, buffer, 1 );
}


/***********************************************************************
 *           BMP_DeleteObject
 */
BOOL BMP_DeleteObject( HBITMAP hbitmap, BITMAPOBJ * bitmap )
{
      /* Free bitmap on global heap */
    GlobalFree( bitmap->hBitmap );
    return GDI_FreeObject( hbitmap );
}

	
/***********************************************************************
 *           BMP_GetObject
 */
int BMP_GetObject( BITMAPOBJ * bitmap, int count, LPSTR buffer )
{
    char * bmpPtr = (char *) GlobalLock( bitmap->hBitmap );    
    if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
    memcpy( buffer, bmpPtr, count );
    GlobalUnlock( bitmap->hBitmap );
    return count;
}

    
/***********************************************************************
 *           BITMAP_UnselectBitmap
 *
 * Unselect the bitmap from the DC. Used by SelectObject and DeleteDC.
 */
BOOL BITMAP_UnselectBitmap( DC * dc )
{
    BITMAPOBJ * bmp;
    BITMAP * bmpPtr;

    if (!dc->w.hBitmap) return TRUE;
    bmp = (BITMAPOBJ *) GDI_GetObjPtr( dc->w.hBitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    
    if (!(bmpPtr = (BITMAP *) GlobalLock( bmp->hBitmap ))) return FALSE;
    
    BITMAP_CopyFromPixmap( bmpPtr, dc->u.x.drawable );
    XFreePixmap( XT_display, dc->u.x.drawable );
    bmp->bSelected = FALSE;
    bmp->hdc       = 0;
    GlobalUnlock( bmp->hBitmap );
    return TRUE;
}


/***********************************************************************
 *           BITMAP_SelectObject
 */
HBITMAP BITMAP_SelectObject( HDC hdc, DC * dc, HBITMAP hbitmap,
			     BITMAPOBJ * bitmap )
{
    BITMAP * bmp;
    HBITMAP prevHandle = dc->w.hBitmap;
    
    if (!(dc->w.flags & DC_MEMORY)) return 0;
    if (bitmap->bSelected && hbitmap != BITMAP_hbitmapMemDC) return 0;
    if (!(bmp = (BITMAP *) GlobalLock( bitmap->hBitmap ))) return 0;

      /* Make sure the bitmap has the right format */

    if ((bmp->bmPlanes != 1) || !BITMAP_FindGCForDepth( bmp->bmBitsPixel ))
    {
	GlobalUnlock( bitmap->hBitmap );
	return 0;
    }
    
      /* Unselect the previous bitmap */

    if (!BITMAP_UnselectBitmap( dc ))
    {
	GlobalUnlock( bitmap->hBitmap );
	return 0;
    }
    
      /* Create the pixmap */
    
    dc->u.x.drawable   = XCreatePixmap( XT_display,
				        DefaultRootWindow( XT_display ), 
				        bmp->bmWidth, bmp->bmHeight,
				        bmp->bmBitsPixel );
    dc->w.DCSizeX      = bmp->bmWidth;
    dc->w.DCSizeY      = bmp->bmHeight;
    BITMAP_CopyToPixmap( bmp, dc->u.x.drawable,
			 0, 0, bmp->bmWidth, bmp->bmHeight );

      /* Change GC depth if needed */

    if (dc->w.bitsPerPixel != bmp->bmBitsPixel)
    {
	XFreeGC( XT_display, dc->u.x.gc );
	dc->u.x.gc = XCreateGC( XT_display, dc->u.x.drawable, 0, NULL );
	dc->w.bitsPerPixel = bmp->bmBitsPixel;
	DC_SetDeviceInfo( hdc, dc );
    }
    
    GlobalUnlock( bitmap->hBitmap );
    dc->w.hBitmap     = hbitmap;
    bitmap->bSelected = TRUE;
    bitmap->hdc       = hdc;
    return prevHandle;
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
