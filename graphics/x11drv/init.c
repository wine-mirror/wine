/*
 * X11 graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <string.h>
#include "x11drv.h"
#include "color.h"
#include "bitmap.h"


static BOOL32 X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE16* initData );
static BOOL32 X11DRV_DeleteDC( DC *dc );

static INT32 X11DRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput,
                            SEGPTR lpInData, SEGPTR lpOutData );

static const DC_FUNCTIONS X11DRV_Funcs =
{
    X11DRV_Arc,                      /* pArc */
    X11DRV_BitBlt,                   /* pBitBlt */
    X11DRV_BitmapBits,               /* pBitmapBits */
    X11DRV_Chord,                    /* pChord */
    X11DRV_CreateBitmap,             /* pCreateBitmap */
    X11DRV_CreateDC,                 /* pCreateDC */
    X11DRV_DeleteDC,                 /* pDeleteDC */
    X11DRV_DeleteObject,             /* pDeleteObject */
    X11DRV_Ellipse,                  /* pEllipse */
    X11DRV_EnumDeviceFonts,          /* pEnumDeviceFonts */
    X11DRV_Escape,                   /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    X11DRV_ExtFloodFill,             /* pExtFloodFill */
    X11DRV_ExtTextOut,               /* pExtTextOut */
    X11DRV_GetCharWidth,             /* pGetCharWidth */
    X11DRV_GetPixel,                 /* pGetPixel */
    X11DRV_GetTextExtentPoint,       /* pGetTextExtentPoint */
    X11DRV_GetTextMetrics,           /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    X11DRV_LineTo,                   /* pLineTo */
    X11DRV_MoveToEx,                 /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg (optional) */
    NULL,                            /* pOffsetWindowOrg (optional) */
    X11DRV_PaintRgn,                 /* pPaintRgn */
    X11DRV_PatBlt,                   /* pPatBlt */
    X11DRV_Pie,                      /* pPie */
    X11DRV_PolyPolygon,              /* pPolyPolygon */
    X11DRV_PolyPolyline,             /* pPolyPolyline */
    X11DRV_Polygon,                  /* pPolygon */
    X11DRV_Polyline,                 /* pPolyline */
    X11DRV_PolyBezier,               /* pPolyBezier */
    NULL,                            /* pRealizePalette */
    X11DRV_Rectangle,                /* pRectangle */
    NULL,                            /* pRestoreDC */
    X11DRV_RoundRect,                /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExt (optional) */
    NULL,                            /* pScaleWindowExt (optional) */
    NULL,                            /* pSelectClipRgn */
    X11DRV_SelectObject,             /* pSelectObject */
    NULL,                            /* pSelectPalette */
    X11DRV_SetBkColor,               /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    X11DRV_SetDeviceClipping,        /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode (optional) */
    NULL,                            /* pSetMapperFlags */
    X11DRV_SetPixel,                 /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    X11DRV_SetTextColor,             /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExt (optional) */
    NULL,                            /* pSetViewportOrg (optional) */
    NULL,                            /* pSetWindowExt (optional) */
    NULL,                            /* pSetWindowOrg (optional) */
    X11DRV_StretchBlt,               /* pStretchBlt */
    NULL                             /* pStretchDIBits */
};

static DeviceCaps X11DRV_DevCaps = {
/* version */		0, 
/* technology */	DT_RASDISPLAY,
/* size, resolution */	0, 0, 0, 0, 0, 
/* device objects */	1, 16 + 6, 16, 0, 0, 100, 0,	
/* curve caps */	CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES |
			CC_WIDE | CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT,
/* line caps */		LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
			LC_STYLED | LC_WIDESTYLED | LC_INTERIORS,
/* polygon caps */	PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON |
			PC_SCANLINE | PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS,
/* text caps */		0,
/* regions */		CP_REGION,
/* raster caps */	RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 |
			RC_DI_BITMAP | RC_DIBTODEV | RC_BIGFONT | RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS,
/* aspects */		36, 36, 51,
/* pad1 */		{ 0 },
/* log pixels */	0, 0, 
/* pad2 */		{ 0 },
/* palette size */	0,
/* ..etc */		0, 0 };

/**********************************************************************
 *	     X11DRV_Init
 */
BOOL32 X11DRV_Init(void)
{
    /* FIXME: colormap management should be merged with the X11DRV */

    if( !COLOR_Init() ) return FALSE;

    /* Finish up device caps */

#if 0
    TRACE(x11drv, "Height = %-4i pxl, %-4i mm, Width  = %-4i pxl, %-4i mm\n",
	  HeightOfScreen(screen), HeightMMOfScreen(screen),
	  WidthOfScreen(screen), WidthMMOfScreen(screen) );
#endif

    X11DRV_DevCaps.version = 0x300;
    X11DRV_DevCaps.horzSize = WidthMMOfScreen(screen) * screenWidth / WidthOfScreen(screen);
    X11DRV_DevCaps.vertSize = HeightMMOfScreen(screen) * screenHeight / HeightOfScreen(screen);
    X11DRV_DevCaps.horzRes = screenWidth;
    X11DRV_DevCaps.vertRes = screenHeight;
    X11DRV_DevCaps.bitsPixel = screenDepth;

    if( COLOR_GetSystemPaletteFlags() & COLOR_VIRTUAL ) 
	X11DRV_DevCaps.sizePalette = 0;
    else
    {
	X11DRV_DevCaps.rasterCaps |= RC_PALETTE;
	X11DRV_DevCaps.sizePalette = DefaultVisual(display,DefaultScreen(display))->map_entries;
    }
 
    /* Resolution will be adjusted during the font init */

    X11DRV_DevCaps.logPixelsX = (int)(X11DRV_DevCaps.horzRes * 25.4 / X11DRV_DevCaps.horzSize);
    X11DRV_DevCaps.logPixelsY = (int)(X11DRV_DevCaps.vertRes * 25.4 / X11DRV_DevCaps.vertSize);

    /* Create default bitmap */

    if (!X11DRV_BITMAP_Init()) return FALSE;

    /* Initialize brush dithering */

    if (!X11DRV_BRUSH_Init()) return FALSE;

    /* Initialize fonts and text caps */

    if (!X11DRV_FONT_Init( &X11DRV_DevCaps )) return FALSE;

    return DRIVER_RegisterDriver( "DISPLAY", &X11DRV_Funcs );
}

/**********************************************************************
 *	     X11DRV_CreateDC
 */
static BOOL32 X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE16* initData )
{
    X11DRV_PDEVICE *physDev;

    physDev = &dc->u.x;  /* for now */

    memset( physDev, 0, sizeof(*physDev) );
    dc->physDev        = physDev;
    dc->w.devCaps      = &X11DRV_DevCaps;
    if (dc->w.flags & DC_MEMORY)
    {
        X11DRV_PHYSBITMAP *pbitmap;
        BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( dc->w.hBitmap,
                                                      BITMAP_MAGIC );
	X11DRV_CreateBitmap( dc->w.hBitmap );
	pbitmap            = bmp->DDBitmap->physBitmap;
        physDev->drawable  = pbitmap->pixmap;
        physDev->gc        = TSXCreateGC(display, physDev->drawable, 0, NULL);
        dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;

        dc->w.totalExtent.left   = 0;
        dc->w.totalExtent.top    = 0;
        dc->w.totalExtent.right  = bmp->bitmap.bmWidth;
        dc->w.totalExtent.bottom = bmp->bitmap.bmHeight;
        dc->w.hVisRgn            = CreateRectRgnIndirect32( &dc->w.totalExtent );

	GDI_HEAP_UNLOCK( dc->w.hBitmap );
    }
    else
    {
        physDev->drawable  = rootWindow;
        physDev->gc        = TSXCreateGC( display, physDev->drawable, 0, NULL );
        dc->w.bitsPerPixel = screenDepth;

        dc->w.totalExtent.left   = 0;
        dc->w.totalExtent.top    = 0;
        dc->w.totalExtent.right  = screenWidth;
        dc->w.totalExtent.bottom = screenHeight;
        dc->w.hVisRgn            = CreateRectRgnIndirect32( &dc->w.totalExtent );
    }

    if (!dc->w.hVisRgn)
    {
        TSXFreeGC( display, physDev->gc );
        return FALSE;
    }

    TSXSetGraphicsExposures( display, physDev->gc, False );
    TSXSetSubwindowMode( display, physDev->gc, IncludeInferiors );

    return TRUE;
}


/**********************************************************************
 *	     X11DRV_DeleteDC
 */
static BOOL32 X11DRV_DeleteDC( DC *dc )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    TSXFreeGC( display, physDev->gc );
    return TRUE;
}

/**********************************************************************
 *           X11DRV_Escape
 */
static INT32 X11DRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput,
                            SEGPTR lpInData, SEGPTR lpOutData )
{
    switch( nEscape )
    {
	case GETSCALINGFACTOR:
	     if( lpOutData )
	     {
		 LPPOINT16 lppt = (LPPOINT16)PTR_SEG_TO_LIN(lpOutData);
		 lppt->x = lppt->y = 0;	/* no device scaling */
		 return 1;
	     }
	     break;
    }
    return 0;
}

