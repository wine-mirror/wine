/*
 * GDI brush objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "gdi.h"
#include "bitmap.h"
#include "prototypes.h"


#define NB_HATCH_STYLES  6

static char HatchBrushes[NB_HATCH_STYLES][8] =
{
    { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 }, /* HS_HORIZONTAL */
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 }, /* HS_VERTICAL   */
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }, /* HS_FDIAGONAL  */
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }, /* HS_BDIAGONAL  */
    { 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08 }, /* HS_CROSS      */
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }  /* HS_DIAGCROSS  */
};

extern WORD COLOR_ToPhysical( DC *dc, COLORREF color );


/***********************************************************************
 *           CreateBrushIndirect    (GDI.50)
 */
HBRUSH CreateBrushIndirect( LOGBRUSH * brush )
{
    BRUSHOBJ * brushPtr;
    HBRUSH hbrush = GDI_AllocObject( sizeof(BRUSHOBJ), BRUSH_MAGIC );
    if (!hbrush) return 0;
    brushPtr = (BRUSHOBJ *) GDI_HEAP_ADDR( hbrush );
    memcpy( &brushPtr->logbrush, brush, sizeof(LOGBRUSH) );
    return hbrush;
}


/***********************************************************************
 *           CreateHatchBrush    (GDI.58)
 */
HBRUSH CreateHatchBrush( short style, COLORREF color )
{
    LOGBRUSH logbrush = { BS_HATCHED, color, style };
#ifdef DEBUG_GDI
    printf( "CreateHatchBrush: %d %06x\n", style, color );
#endif
    if ((style < 0) || (style >= NB_HATCH_STYLES)) return 0;
    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreatePatternBrush    (GDI.60)
 */
HBRUSH CreatePatternBrush( HBITMAP hbitmap )
{
    LOGBRUSH logbrush = { BS_PATTERN, 0, 0 };
    BITMAPOBJ *bmp, *newbmp;

#ifdef DEBUG_GDI
    printf( "CreatePatternBrush: %d\n", hbitmap );
#endif

      /* Make a copy of the bitmap */

    if (!(bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC )))
	return 0;
    logbrush.lbHatch = CreateBitmapIndirect( &bmp->bitmap );
    newbmp = (BITMAPOBJ *) GDI_GetObjPtr( logbrush.lbHatch, BITMAP_MAGIC );
    if (!newbmp) return 0;
    XCopyArea( display, bmp->pixmap, newbmp->pixmap, BITMAP_GC(bmp),
	       0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight, 0, 0 );
    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateDIBPatternBrush    (GDI.445)
 */
HBRUSH CreateDIBPatternBrush( HANDLE hbitmap, WORD coloruse )
{
    LOGBRUSH logbrush = { BS_DIBPATTERN, coloruse, 0 };
    BITMAPINFO *info, *newInfo;
    int size;
    
#ifdef DEBUG_GDI
    printf( "CreateDIBPatternBrush: %d\n", hbitmap );
#endif

      /* Make a copy of the bitmap */

    if (!(info = (BITMAPINFO *) GlobalLock( hbitmap ))) return 0;

    size = info->bmiHeader.biSizeImage;
    if (!size)
	size = (info->bmiHeader.biWidth * info->bmiHeader.biBitCount + 31) / 32
	         * 8 * info->bmiHeader.biHeight;
    size += DIB_BitmapInfoSize( info, coloruse );

    if (!(logbrush.lbHatch = GlobalAlloc( GMEM_MOVEABLE, size )))
    {
	GlobalUnlock( hbitmap );
	return 0;
    }
    newInfo = (BITMAPINFO *) GlobalLock( logbrush.lbHatch );
    memcpy( newInfo, info, size );
    GlobalUnlock( logbrush.lbHatch );
    GlobalUnlock( hbitmap );
    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateSolidBrush    (GDI.66)
 */
HBRUSH CreateSolidBrush( COLORREF color )
{
    LOGBRUSH logbrush = { BS_SOLID, color, 0 };
#ifdef DEBUG_GDI
    printf( "CreateSolidBrush: %06x\n", color );
#endif
    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           SetBrushOrg    (GDI.148)
 */
DWORD SetBrushOrg( HDC hdc, short x, short y )
{
    DWORD retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    retval = dc->w.brushOrgX | (dc->w.brushOrgY << 16);
    dc->w.brushOrgX = x;
    dc->w.brushOrgY = y;
    return retval;
}


/***********************************************************************
 *           BRUSH_DeleteObject
 */
BOOL BRUSH_DeleteObject( HBRUSH hbrush, BRUSHOBJ * brush )
{
    switch(brush->logbrush.lbStyle)
    {
      case BS_PATTERN:
	  DeleteObject( brush->logbrush.lbHatch );
	  break;
      case BS_DIBPATTERN:
	  GlobalFree( brush->logbrush.lbHatch );
	  break;
    }
    return GDI_FreeObject( hbrush );
}


/***********************************************************************
 *           BRUSH_GetObject
 */
int BRUSH_GetObject( BRUSHOBJ * brush, int count, LPSTR buffer )
{
    if (count > sizeof(LOGBRUSH)) count = sizeof(LOGBRUSH);
    memcpy( buffer, &brush->logbrush, count );
    return count;
}


/***********************************************************************
 *           BRUSH_MakeSolidBrush
 */
static void BRUSH_SelectSolidBrush( DC *dc, COLORREF color )
{
    if ((dc->w.bitsPerPixel > 1) && !COLOR_IsSolid( color ))
    {
	  /* Dithered brush */
	dc->u.x.brush.pixmap = DITHER_DitherColor( dc, color );
	dc->u.x.brush.fillStyle = FillTiled;
	dc->u.x.brush.pixel = 0;
    }
    else
    {
	  /* Solid brush */
	dc->u.x.brush.pixel = COLOR_ToPhysical( dc, color );
	dc->u.x.brush.fillStyle = FillSolid;
    }
}


/***********************************************************************
 *           BRUSH_SelectPatternBrush
 */
static BOOL BRUSH_SelectPatternBrush( DC * dc, HBITMAP hbitmap )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    dc->u.x.brush.pixmap = XCreatePixmap( display, rootWindow,
					  8, 8, bmp->bitmap.bmBitsPixel );
    XCopyArea( display, bmp->pixmap, dc->u.x.brush.pixmap,
	       BITMAP_GC(bmp), 0, 0, 8, 8, 0, 0 );
    
    if (bmp->bitmap.bmBitsPixel > 1)
    {
	dc->u.x.brush.fillStyle = FillTiled;
	dc->u.x.brush.pixel = 0;  /* Ignored */
    }
    else
    {
	dc->u.x.brush.fillStyle = FillOpaqueStippled;
	dc->u.x.brush.pixel = -1;  /* Special case (see DC_SetupGCForBrush) */
    }
    return TRUE;
}



/***********************************************************************
 *           BRUSH_SelectObject
 */
HBRUSH BRUSH_SelectObject( HDC hdc, DC * dc, HBRUSH hbrush, BRUSHOBJ * brush )
{
    HBITMAP hBitmap;
    BITMAPINFO * bmpInfo;
    HBRUSH prevHandle = dc->w.hBrush;
    
    dc->w.hBrush = hbrush;

    if (dc->u.x.brush.pixmap)
    {
	XFreePixmap( display, dc->u.x.brush.pixmap );
	dc->u.x.brush.pixmap = 0;
    }
    dc->u.x.brush.style = brush->logbrush.lbStyle;
    
    switch(brush->logbrush.lbStyle)
    {
      case BS_NULL:
	break;

      case BS_SOLID:
	BRUSH_SelectSolidBrush( dc, brush->logbrush.lbColor );
	break;
	
      case BS_HATCHED:
	dc->u.x.brush.pixel = COLOR_ToPhysical( dc, brush->logbrush.lbColor );
	dc->u.x.brush.pixmap = XCreateBitmapFromData( display, rootWindow,
				 HatchBrushes[brush->logbrush.lbHatch], 8, 8 );
	dc->u.x.brush.fillStyle = FillStippled;
	break;
	
      case BS_PATTERN:
	BRUSH_SelectPatternBrush( dc, brush->logbrush.lbHatch );
	break;

      case BS_DIBPATTERN:
	if ((bmpInfo = (BITMAPINFO *) GlobalLock( brush->logbrush.lbHatch )))
	{
	    int size = DIB_BitmapInfoSize( bmpInfo, brush->logbrush.lbColor );
	    hBitmap = CreateDIBitmap( hdc, &bmpInfo->bmiHeader, CBM_INIT,
				      ((char *)bmpInfo) + size, bmpInfo,
				      (WORD) brush->logbrush.lbColor );
	    BRUSH_SelectPatternBrush( dc, hBitmap );
	    DeleteObject( hBitmap );
	    GlobalUnlock( brush->logbrush.lbHatch );	    
	}
	
	break;
    }
    
    return prevHandle;
}
