/*
 * X11DRV brush objects
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "ts_xlib.h"

#include <stdlib.h>
#include "brush.h"
#include "bitmap.h"
#include "color.h"
#include "x11drv.h"
#include "debug.h"
#include "xmalloc.h" /* for XCREATEIMAGE macro */
#include "monitor.h"
#include "local.h"

static const char HatchBrushes[NB_HATCH_STYLES + 1][8] =
{
    { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 }, /* HS_HORIZONTAL */
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 }, /* HS_VERTICAL   */
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }, /* HS_FDIAGONAL  */
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }, /* HS_BDIAGONAL  */
    { 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08 }, /* HS_CROSS      */
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }, /* HS_DIAGCROSS  */
    { 0xee, 0xbb, 0xee, 0xbb, 0xee, 0xbb, 0xee, 0xbb }  /* Hack for DKGRAY */
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
BOOL X11DRV_BRUSH_Init(void)
{
    XCREATEIMAGE( ditherImage, MATRIX_SIZE, MATRIX_SIZE, MONITOR_GetDepth(&MONITOR_PrimaryMonitor) );
    return (ditherImage != NULL);
}


/***********************************************************************
 *           BRUSH_DitherColor
 */
static Pixmap BRUSH_DitherColor( DC *dc, COLORREF color )
{
    static COLORREF prevColor = 0xffffffff;
    unsigned int x, y;
    Pixmap pixmap;

    EnterCriticalSection( &X11DRV_CritSection );
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
    
    pixmap = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                            MATRIX_SIZE, MATRIX_SIZE, MONITOR_GetDepth(&MONITOR_PrimaryMonitor) );
    XPutImage( display, pixmap, BITMAP_colorGC, ditherImage, 0, 0,
	       0, 0, MATRIX_SIZE, MATRIX_SIZE );
    LeaveCriticalSection( &X11DRV_CritSection );
    return pixmap;
}


/***********************************************************************
 *           BRUSH_SelectSolidBrush
 */
static void BRUSH_SelectSolidBrush( DC *dc, COLORREF color )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if ((dc->w.bitsPerPixel > 1) && (MONITOR_GetDepth(&MONITOR_PrimaryMonitor) <= 8) && !COLOR_IsSolid( color ))
    {
	  /* Dithered brush */
	physDev->brush.pixmap = BRUSH_DitherColor( dc, color );
	physDev->brush.fillStyle = FillTiled;
	physDev->brush.pixel = 0;
    }
    else
    {
	  /* Solid brush */
	physDev->brush.pixel = COLOR_ToPhysical( dc, color );
	physDev->brush.fillStyle = FillSolid;
    }
}


/***********************************************************************
 *           BRUSH_SelectPatternBrush
 */
static BOOL BRUSH_SelectPatternBrush( DC * dc, HBITMAP hbitmap )
{
    X11DRV_PHYSBITMAP *pbitmap;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;

   if(!bmp->DDBitmap)
        if(!X11DRV_CreateBitmap(hbitmap))
	    return 0;

    if(bmp->DDBitmap->funcs != dc->funcs) {
        WARN(bitmap, "Trying to select non-X11 DDB into an X11 dc\n");
	return 0;
    }

    pbitmap = bmp->DDBitmap->physBitmap;

    if ((dc->w.bitsPerPixel == 1) && (bmp->bitmap.bmBitsPixel != 1))
    {
        /* Special case: a color pattern on a monochrome DC */
        physDev->brush.pixmap = TSXCreatePixmap( display, X11DRV_GetXRootWindow(), 8, 8, 1);
        /* FIXME: should probably convert to monochrome instead */
        TSXCopyPlane( display, pbitmap->pixmap, physDev->brush.pixmap,
                    BITMAP_monoGC, 0, 0, 8, 8, 0, 0, 1 );
    }
    else
    {
        physDev->brush.pixmap = TSXCreatePixmap( display, X11DRV_GetXRootWindow(),
					       8, 8, bmp->bitmap.bmBitsPixel );
        TSXCopyArea( display, pbitmap->pixmap, physDev->brush.pixmap,
                   BITMAP_GC(bmp), 0, 0, 8, 8, 0, 0 );
    }
    
    if (bmp->bitmap.bmBitsPixel > 1)
    {
	physDev->brush.fillStyle = FillTiled;
	physDev->brush.pixel = 0;  /* Ignored */
    }
    else
    {
	physDev->brush.fillStyle = FillOpaqueStippled;
	physDev->brush.pixel = -1;  /* Special case (see DC_SetupGCForBrush) */
    }
    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}




/***********************************************************************
 *           BRUSH_SelectObject
 */
HBRUSH X11DRV_BRUSH_SelectObject( DC * dc, HBRUSH hbrush, BRUSHOBJ * brush )
{
    HBITMAP16 hBitmap;
    BITMAPINFO * bmpInfo;
    HBRUSH16 prevHandle = dc->w.hBrush;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    
    TRACE(gdi, "hdc=%04x hbrush=%04x\n",
                dc->hSelf,hbrush);

    dc->w.hBrush = hbrush;

    if (physDev->brush.pixmap)
    {
	TSXFreePixmap( display, physDev->brush.pixmap );
	physDev->brush.pixmap = 0;
    }
    physDev->brush.style = brush->logbrush.lbStyle;
    
    switch(brush->logbrush.lbStyle)
    {
      case BS_NULL:
	TRACE(gdi,"BS_NULL\n" );
	break;

      case BS_SOLID:
        TRACE(gdi,"BS_SOLID\n" );
	BRUSH_SelectSolidBrush( dc, brush->logbrush.lbColor );
	break;
	
      case BS_HATCHED:
	TRACE(gdi, "BS_HATCHED\n" );
	physDev->brush.pixel = COLOR_ToPhysical( dc, brush->logbrush.lbColor );
	physDev->brush.pixmap = TSXCreateBitmapFromData( display, X11DRV_GetXRootWindow(),
				 HatchBrushes[brush->logbrush.lbHatch], 8, 8 );
	physDev->brush.fillStyle = FillStippled;
	break;
	
      case BS_PATTERN:
	TRACE(gdi, "BS_PATTERN\n");
	BRUSH_SelectPatternBrush( dc, (HBRUSH16)brush->logbrush.lbHatch );
	break;

      case BS_DIBPATTERN:
	TRACE(gdi, "BS_DIBPATTERN\n");
	if ((bmpInfo = (BITMAPINFO *) GlobalLock16( (HGLOBAL16)brush->logbrush.lbHatch )))
	{
	    int size = DIB_BitmapInfoSize( bmpInfo, brush->logbrush.lbColor );
	    hBitmap = CreateDIBitmap( dc->hSelf, &bmpInfo->bmiHeader,
                                        CBM_INIT, ((char *)bmpInfo) + size,
                                        bmpInfo,
                                        (WORD)brush->logbrush.lbColor );
	    BRUSH_SelectPatternBrush( dc, hBitmap );
	    DeleteObject16( hBitmap );
	    GlobalUnlock16( (HGLOBAL16)brush->logbrush.lbHatch );	    
	}
	
	break;
    }
    
    return prevHandle;
}

#endif /* !defined(X_DISPLAY_MISSING) */

