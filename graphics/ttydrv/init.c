/*
 * TTY driver
 *
 * Copyright 1998-1999 Patrik Stridvall
 */

#include "config.h"

#include "gdi.h"
#include "bitmap.h"
#include "color.h"
#include "dc.h"
#include "ttydrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

static const DC_FUNCTIONS TTYDRV_DC_Driver =
{
  NULL,                /* pAbortDoc */
  NULL,                /* pAbortPath */
  NULL,                /* pAngleArc */
  TTYDRV_DC_Arc,       /* pArc */
  NULL,                /* pArcTo */
  NULL,                /* pBeginPath */
  TTYDRV_DC_BitBlt,    /* pBitBlt */
  TTYDRV_DC_BitmapBits,/* pBitmapBits */
  TTYDRV_DC_Chord,     /* pChord */
  NULL,                /* pCloseFigure */
  TTYDRV_DC_CreateBitmap, /* pCreateBitmap */
  TTYDRV_DC_CreateDC,  /* pCreateDC */
  NULL,                /* pCreateDIBSection */
  NULL,                /* pCreateDIBSection16 */
  TTYDRV_DC_DeleteDC,  /* pDeleteDC */
  TTYDRV_DC_DeleteObject, /* pDeleteObject */
  NULL,                /* pDeviceCapabilities */
  TTYDRV_DC_Ellipse,   /* pEllipse */
  NULL,                /* pEndDoc */
  NULL,                /* pEndPage */
  NULL,                /* pEndPath */
  NULL,                /* pEnumDeviceFonts */
  TTYDRV_DC_Escape,    /* pEscape */
  NULL,                /* pExcludeClipRect */
  NULL,                /* pExtDeviceMode */
  TTYDRV_DC_ExtFloodFill, /* pExtFloodFill */
  TTYDRV_DC_ExtTextOut, /* pExtTextOut */
  NULL,                /* pFillPath */
  NULL,                /* pFillRgn */
  NULL,                /* pFlattenPath */
  NULL,                /* pFrameRgn */
  TTYDRV_DC_GetCharWidth, /* pGetCharWidth */
  TTYDRV_DC_GetPixel,  /* pGetPixel */
  TTYDRV_DC_GetTextExtentPoint, /* pGetTextExtentPoint */
  TTYDRV_DC_GetTextMetrics,  /* pGetTextMetrics */
  NULL,                /* pIntersectClipRect */
  NULL,                /* pIntersectVisRect */
  TTYDRV_DC_LineTo,    /* pLineTo */
  TTYDRV_DC_LoadOEMResource, /* pLoadOEMResource */
  NULL,                /* pMoveToEx */
  NULL,                /* pOffsetClipRgn */
  NULL,                /* pOffsetViewportOrg (optional) */
  NULL,                /* pOffsetWindowOrg (optional) */
  TTYDRV_DC_PaintRgn,  /* pPaintRgn */
  TTYDRV_DC_PatBlt,    /* pPatBlt */
  TTYDRV_DC_Pie,       /* pPie */
  TTYDRV_DC_PolyBezier, /* pPolyBezier */
  NULL,                /* pPolyBezierTo */
  NULL,                /* pPolyDraw */
  TTYDRV_DC_PolyPolygon, /* pPolyPolygon */
  TTYDRV_DC_PolyPolyline, /* pPolyPolyline */
  TTYDRV_DC_Polygon,   /* pPolygon */
  TTYDRV_DC_Polyline,  /* pPolyline */
  NULL,                /* pPolylineTo */
  NULL,                /* pRealizePalette */
  TTYDRV_DC_Rectangle, /* pRectangle */
  NULL,                /* pRestoreDC */
  TTYDRV_DC_RoundRect, /* pRoundRect */
  NULL,                /* pSaveDC */
  NULL,                /* pScaleViewportExt (optional) */
  NULL,                /* pScaleWindowExt (optional) */
  NULL,                /* pSelectClipPath */
  NULL,                /* pSelectClipRgn */
  TTYDRV_DC_SelectObject, /* pSelectObject */
  NULL,                /* pSelectPalette */
  TTYDRV_DC_SetBkColor, /* pSetBkColor */
  NULL,                /* pSetBkMode */
  TTYDRV_DC_SetDeviceClipping, /* pSetDeviceClipping */
  TTYDRV_DC_SetDIBitsToDevice, /* pSetDIBitsToDevice */
  NULL,                /* pSetMapMode (optional) */
  NULL,                /* pSetMapperFlags */
  TTYDRV_DC_SetPixel,  /* pSetPixel */
  NULL,                /* pSetPolyFillMode */
  NULL,                /* pSetROP2 */
  NULL,                /* pSetRelAbs */
  NULL,                /* pSetStretchBltMode */
  NULL,                /* pSetTextAlign */
  NULL,                /* pSetTextCharacterExtra */
  TTYDRV_DC_SetTextColor, /* pSetTextColor */
  NULL,                /* pSetTextJustification */
  NULL,                /* pSetViewportExt (optional) */
  NULL,                /* pSetViewportOrg (optional) */
  NULL,                /* pSetWindowExt (optional) */
  NULL,                /* pSetWindowOrg (optional) */
  NULL,                /* pStartDoc */
  NULL,                /* pStartPage */
  TTYDRV_DC_StretchBlt, /* pStretchBlt */
  NULL,                /* pStretchDIBits */
  NULL,                /* pStrokeAndFillPath */
  NULL,                /* pStrokePath */
  NULL                 /* pWidenPath */
};


GDI_DRIVER TTYDRV_GDI_Driver =
{
  TTYDRV_GDI_Initialize,
  TTYDRV_GDI_Finalize
};

BITMAP_DRIVER TTYDRV_BITMAP_Driver =
{
  TTYDRV_BITMAP_SetDIBits,
  TTYDRV_BITMAP_GetDIBits,
  TTYDRV_BITMAP_DeleteDIBSection
};

PALETTE_DRIVER TTYDRV_PALETTE_Driver = 
{
  TTYDRV_PALETTE_SetMapping,
  TTYDRV_PALETTE_UpdateMapping,
  TTYDRV_PALETTE_IsDark
};

/* FIXME: Adapt to the TTY driver. Copied from the X11 driver */

DeviceCaps TTYDRV_DC_DevCaps = {
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
/* ..etc */		0, 0
};

/**********************************************************************
 *	     TTYDRV_GDI_Initialize
 */
BOOL TTYDRV_GDI_Initialize(void)
{
  BITMAP_Driver = &TTYDRV_BITMAP_Driver;
  PALETTE_Driver = &TTYDRV_PALETTE_Driver;

  TTYDRV_DC_DevCaps.version = 0x300;
  TTYDRV_DC_DevCaps.horzSize = 0;    /* FIXME: Screen width in mm */
  TTYDRV_DC_DevCaps.vertSize = 0;    /* FIXME: Screen height in mm */
  TTYDRV_DC_DevCaps.horzRes = 640;   /* FIXME: Screen width in pixel */
  TTYDRV_DC_DevCaps.vertRes = 480;   /* FIXME: Screen height in pixel */
  TTYDRV_DC_DevCaps.bitsPixel = 1;   /* FIXME: Bits per pixel */
  TTYDRV_DC_DevCaps.sizePalette = 0; /* FIXME: ??? */
  
  /* Resolution will be adjusted during the font init */
  
  TTYDRV_DC_DevCaps.logPixelsX = (int) (TTYDRV_DC_DevCaps.horzRes * 25.4 / TTYDRV_DC_DevCaps.horzSize);
  TTYDRV_DC_DevCaps.logPixelsY = (int) (TTYDRV_DC_DevCaps.vertRes * 25.4 / TTYDRV_DC_DevCaps.vertSize);
 
  if(!TTYDRV_PALETTE_Initialize())
    return FALSE;

  return DRIVER_RegisterDriver( "DISPLAY", &TTYDRV_DC_Driver );
}

/**********************************************************************
 *	     TTYDRV_GDI_Finalize
 */
void TTYDRV_GDI_Finalize(void)
{
    TTYDRV_PALETTE_Finalize();
}
