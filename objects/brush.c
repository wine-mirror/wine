/*
 * GDI brush objects
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include <stdlib.h>
#include "brush.h"
#include "bitmap.h"
#include "metafile.h"
#include "color.h"
#include "stddebug.h"
#include "debug.h"

#define NB_HATCH_STYLES  6

static const char HatchBrushes[NB_HATCH_STYLES][8] =
{
    { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 }, /* HS_HORIZONTAL */
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 }, /* HS_VERTICAL   */
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }, /* HS_FDIAGONAL  */
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }, /* HS_BDIAGONAL  */
    { 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08 }, /* HS_CROSS      */
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }  /* HS_DIAGCROSS  */
};

  /* Levels of each primary for dithering */
#define PRIMARY_LEVELS  3  
#define TOTAL_LEVELS    (PRIMARY_LEVELS*PRIMARY_LEVELS*PRIMARY_LEVELS)

 /* Dithering matrix size  */
#define MATRIX_SIZE     8
#define MATRIX_SIZE_2   (MATRIX_SIZE*MATRIX_SIZE)

  /* Total number of possible levels for a dithered primary color */
#define DITHER_LEVELS   (MATRIX_SIZE_2 * (PRIMARY_LEVELS-1) + 1)

  /* Dithering matrix */
static const int dither_matrix[MATRIX_SIZE_2] =
{
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
};

  /* Mapping between (R,G,B) triples and EGA colors */
static const int EGAmapping[TOTAL_LEVELS] =
{
    0,  /* 000000 -> 000000 */
    4,  /* 00007f -> 000080 */
    12, /* 0000ff -> 0000ff */
    2,  /* 007f00 -> 008000 */
    6,  /* 007f7f -> 008080 */
    6,  /* 007fff -> 008080 */
    10, /* 00ff00 -> 00ff00 */
    6,  /* 00ff7f -> 008080 */
    14, /* 00ffff -> 00ffff */
    1,  /* 7f0000 -> 800000 */
    5,  /* 7f007f -> 800080 */
    5,  /* 7f00ff -> 800080 */
    3,  /* 7f7f00 -> 808000 */
    8,  /* 7f7f7f -> 808080 */
    7,  /* 7f7fff -> c0c0c0 */
    3,  /* 7fff00 -> 808000 */
    7,  /* 7fff7f -> c0c0c0 */
    7,  /* 7fffff -> c0c0c0 */
    9,  /* ff0000 -> ff0000 */
    5,  /* ff007f -> 800080 */
    13, /* ff00ff -> ff00ff */
    3,  /* ff7f00 -> 808000 */
    7,  /* ff7f7f -> c0c0c0 */
    7,  /* ff7fff -> c0c0c0 */
    11, /* ffff00 -> ffff00 */
    7,  /* ffff7f -> c0c0c0 */
    15  /* ffffff -> ffffff */
};

#define PIXEL_VALUE(r,g,b) \
    COLOR_mapEGAPixel[EGAmapping[((r)*PRIMARY_LEVELS+(g))*PRIMARY_LEVELS+(b)]]

  /* X image for building dithered pixmap */
static XImage *ditherImage = NULL;


/***********************************************************************
 *           BRUSH_Init
 *
 * Create the X image used for dithering.
 */
BOOL BRUSH_Init(void)
{
    XCREATEIMAGE( ditherImage, MATRIX_SIZE, MATRIX_SIZE, screenDepth );
    return (ditherImage != NULL);
}


/***********************************************************************
 *           BRUSH_DitherColor
 */
Pixmap BRUSH_DitherColor( DC *dc, COLORREF color )
{
    static COLORREF prevColor = 0xffffffff;
    unsigned int x, y;
    Pixmap pixmap;

    if (color != prevColor)
    {
	int r = GetRValue( color ) * DITHER_LEVELS;
	int g = GetGValue( color ) * DITHER_LEVELS;
	int b = GetBValue( color ) * DITHER_LEVELS;
	const int *pmatrix = dither_matrix;

	for (y = 0; y < MATRIX_SIZE; y++)
	{
	    for (x = 0; x < MATRIX_SIZE; x++)
	    {
		int d  = *pmatrix++ * 256;
		int dr = ((r + d) / MATRIX_SIZE_2) / 256;
		int dg = ((g + d) / MATRIX_SIZE_2) / 256;
		int db = ((b + d) / MATRIX_SIZE_2) / 256;
		XPutPixel( ditherImage, x, y, PIXEL_VALUE(dr,dg,db) );
	    }
	}
	prevColor = color;
    }
    
    pixmap = XCreatePixmap( display, rootWindow,
			    MATRIX_SIZE, MATRIX_SIZE, screenDepth );
    XPutImage( display, pixmap, BITMAP_colorGC, ditherImage, 0, 0,
	       0, 0, MATRIX_SIZE, MATRIX_SIZE );
    return pixmap;
}


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
    dprintf_gdi(stddeb, "CreateHatchBrush: %d %06lx\n", style, color );
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

    dprintf_gdi(stddeb, "CreatePatternBrush: %d\n", hbitmap );

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
    
    dprintf_gdi(stddeb, "CreateDIBPatternBrush: %d\n", hbitmap );

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
    dprintf_gdi(stddeb, "CreateSolidBrush: %06lx\n", color );
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
 *           GetSysColorBrush    (USER.281)
 */
WORD GetSysColorBrush(WORD x)
{
	return GetStockObject(GRAY_BRUSH);
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
 *           BRUSH_SelectSolidBrush
 */
static void BRUSH_SelectSolidBrush( DC *dc, COLORREF color )
{
    if ((dc->w.bitsPerPixel > 1) && (screenDepth <= 8) && !COLOR_IsSolid( color ))
    {
	  /* Dithered brush */
	dc->u.x.brush.pixmap = BRUSH_DitherColor( dc, color );
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

    dprintf_gdi(stddeb, "Brush_SelectObject   hdc=%04x  hbrush=%04x\n",
		hdc,hbrush);
    if (dc->header.wMagic == METAFILE_DC_MAGIC)
    {
	switch (brush->logbrush.lbStyle)
	{
	case BS_SOLID:
	case BS_HATCHED:
	case BS_HOLLOW:
	    if (!MF_CreateBrushIndirect(dc, hbrush, &(brush->logbrush)))
		return 0;
	    break;

	case BS_PATTERN:
	case BS_DIBPATTERN:
	    if (!MF_CreatePatternBrush(dc, hbrush, &(brush->logbrush)))
		return 0;
	    break;
	}
	return 1;
    }
    
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
	dprintf_gdi( stddeb,"BS_NULL\n" );
	break;

      case BS_SOLID:
        dprintf_gdi( stddeb,"BS_SOLID\n" );
	BRUSH_SelectSolidBrush( dc, brush->logbrush.lbColor );
	break;
	
      case BS_HATCHED:
	dprintf_gdi( stddeb, "BS_HATCHED\n" );
	dc->u.x.brush.pixel = COLOR_ToPhysical( dc, brush->logbrush.lbColor );
	dc->u.x.brush.pixmap = XCreateBitmapFromData( display, rootWindow,
				 HatchBrushes[brush->logbrush.lbHatch], 8, 8 );
	dc->u.x.brush.fillStyle = FillStippled;
	break;
	
      case BS_PATTERN:
	dprintf_gdi( stddeb, "BS_PATTERN\n");
	BRUSH_SelectPatternBrush( dc, brush->logbrush.lbHatch );
	break;

      case BS_DIBPATTERN:
	dprintf_gdi( stddeb, "BS_DIBPATTERN\n");
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
