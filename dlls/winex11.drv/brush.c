/*
 * X11DRV brush objects
 *
 * Copyright 1993, 1994  Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdlib.h>

#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

static const char HatchBrushes[][8] =
{
    { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 }, /* HS_HORIZONTAL */
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 }, /* HS_VERTICAL   */
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }, /* HS_FDIAGONAL  */
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }, /* HS_BDIAGONAL  */
    { 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08 }, /* HS_CROSS      */
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }, /* HS_DIAGCROSS  */
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
    X11DRV_PALETTE_mapEGAPixel[EGAmapping[((r)*PRIMARY_LEVELS+(g))*PRIMARY_LEVELS+(b)]]

static const COLORREF BLACK = RGB(0, 0, 0);
static const COLORREF WHITE = RGB(0xff, 0xff, 0xff);

/***********************************************************************
 *           BRUSH_DitherColor
 */
static Pixmap BRUSH_DitherColor( COLORREF color, int depth)
{
    /* X image for building dithered pixmap */
    static XImage *ditherImage = NULL;
    static COLORREF prevColor = 0xffffffff;
    unsigned int x, y;
    Pixmap pixmap;
    GC gc;

    XLockDisplay( gdi_display );
    if (!ditherImage)
    {
        ditherImage = XCreateImage( gdi_display, default_visual.visual, depth, ZPixmap, 0,
                                    NULL, MATRIX_SIZE, MATRIX_SIZE, 32, 0 );
        if (!ditherImage)
        {
            ERR("Could not create dither image\n");
            XUnlockDisplay( gdi_display );
            return 0;
        }
        ditherImage->data = malloc( ditherImage->height * ditherImage->bytes_per_line );
    }

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

    pixmap = XCreatePixmap( gdi_display, root_window, MATRIX_SIZE, MATRIX_SIZE, depth );
    gc = XCreateGC( gdi_display, pixmap, 0, NULL );
    XPutImage( gdi_display, pixmap, gc, ditherImage, 0, 0, 0, 0, MATRIX_SIZE, MATRIX_SIZE );
    XFreeGC( gdi_display, gc );
    XUnlockDisplay( gdi_display );

    return pixmap;
}


/***********************************************************************
 *           BRUSH_DitherMono
 */
static Pixmap BRUSH_DitherMono( COLORREF color )
{
    /* This makes the spray work in Win 3.11 pbrush.exe */
    /* FIXME. Extend this basic selection of dither patterns */
    static const char gray_dither[][2] = {{ 0x1, 0x0 }, /* DKGRAY */
                                          { 0x2, 0x1 }, /* GRAY */
                                          { 0x1, 0x3 }, /* LTGRAY */
    };
    int gray = (30 * GetRValue(color) + 59 * GetGValue(color) + 11 * GetBValue(color)) / 100;
    int idx = gray * (ARRAY_SIZE( gray_dither ) + 1)/256 - 1;

    TRACE("color=%s -> gray=%x\n", debugstr_color(color), gray);
    return XCreateBitmapFromData( gdi_display, root_window, gray_dither[idx], 2, 2 );
}

/***********************************************************************
 *           BRUSH_SelectSolidBrush
 */
static void BRUSH_SelectSolidBrush( X11DRV_PDEVICE *physDev, COLORREF color )
{
    COLORREF colorRGB = X11DRV_PALETTE_GetColor( physDev, color );
    if ((physDev->depth > 1) && (default_visual.depth <= 8) && !X11DRV_IsSolidColor( color ))
    {
	  /* Dithered brush */
	physDev->brush.pixmap = BRUSH_DitherColor( colorRGB, physDev->depth );
	physDev->brush.fillStyle = FillTiled;
	physDev->brush.pixel = 0;
    }
    else if (physDev->depth == 1 && colorRGB != WHITE && colorRGB != BLACK)
    {
	physDev->brush.pixel = 0;
	physDev->brush.pixmap = BRUSH_DitherMono( colorRGB );
	physDev->brush.fillStyle = FillTiled;
    }
    else
    {
	  /* Solid brush */
	physDev->brush.pixel = X11DRV_PALETTE_ToPhysical( physDev, color );
	physDev->brush.fillStyle = FillSolid;
    }
}


static BOOL select_pattern_brush( X11DRV_PDEVICE *physdev, const struct brush_pattern *pattern )
{
    XVisualInfo vis = default_visual;
    Pixmap pixmap;
    const BITMAPINFO *info = pattern->info;

    if (physdev->depth == 1 || info->bmiHeader.biBitCount == 1) vis.depth = 1;

    pixmap = create_pixmap_from_image( physdev->dev.hdc, &vis, info, &pattern->bits, pattern->usage );
    if (!pixmap) return FALSE;

    if (physdev->brush.pixmap) XFreePixmap( gdi_display, physdev->brush.pixmap );
    physdev->brush.pixmap = pixmap;

    if (vis.depth == 1)
    {
	physdev->brush.fillStyle = FillOpaqueStippled;
	physdev->brush.pixel = -1;  /* Special case (see DC_SetupGCForBrush) */
    }
    else
    {
	physdev->brush.fillStyle = FillTiled;
	physdev->brush.pixel = 0;  /* Ignored */
    }
    return TRUE;
}

/***********************************************************************
 *           SelectBrush   (X11DRV.@)
 */
HBRUSH X11DRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    LOGBRUSH logbrush;

    if (pattern)  /* pattern brush */
    {
        if (!select_pattern_brush( physDev, pattern )) return 0;
        TRACE("BS_PATTERN\n");
        physDev->brush.style = BS_PATTERN;
        return hbrush;
    }

    if (!NtGdiExtGetObjectW( hbrush, sizeof(logbrush), &logbrush )) return 0;

    TRACE("hdc=%p hbrush=%p\n", dev->hdc, hbrush);

    if (physDev->brush.pixmap)
    {
        XFreePixmap( gdi_display, physDev->brush.pixmap );
        physDev->brush.pixmap = 0;
    }
    physDev->brush.style = logbrush.lbStyle;
    if (hbrush == GetStockObject( DC_BRUSH ))
        NtGdiGetDCDword( dev->hdc, NtGdiGetDCBrushColor, &logbrush.lbColor );

    switch(logbrush.lbStyle)
    {
      case BS_NULL:
	TRACE("BS_NULL\n" );
	break;

      case BS_SOLID:
        TRACE("BS_SOLID\n" );
	BRUSH_SelectSolidBrush( physDev, logbrush.lbColor );
	break;

      case BS_HATCHED:
	TRACE("BS_HATCHED\n" );
	physDev->brush.pixel = X11DRV_PALETTE_ToPhysical( physDev, logbrush.lbColor );
        physDev->brush.pixmap = XCreateBitmapFromData( gdi_display, root_window,
                                                       HatchBrushes[logbrush.lbHatch], 8, 8 );
	physDev->brush.fillStyle = FillStippled;
	break;
    }
    return hbrush;
}


/***********************************************************************
 *           SetDCBrushColor (X11DRV.@)
 */
COLORREF X11DRV_SetDCBrushColor( PHYSDEV dev, COLORREF crColor )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );

    if (NtGdiGetDCObject( dev->hdc, NTGDI_OBJ_BRUSH ) == GetStockObject( DC_BRUSH ))
        BRUSH_SelectSolidBrush( physDev, crColor );

    return crColor;
}
