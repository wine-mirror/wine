/*
 * TTY driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "gdi.h"
#include "bitmap.h"
#include "color.h"
#include "dc.h"
#include "debug.h"
#include "heap.h"
#include "palette.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

static const DC_FUNCTIONS TTYDRV_DC_Driver =
{
  NULL,                /* pAbortDoc */
  NULL,                /* pArc */
  NULL,                /* pBitBlt */
  NULL,                /* pBitmapBits */
  NULL,                /* pChord */
  NULL,                /* pCreateBitmap */
  TTYDRV_DC_CreateDC,  /* pCreateDC */
  NULL,                /* pCreateDIBSection */
  NULL,                /* pCreateDIBSection16 */
  TTYDRV_DC_DeleteDC,  /* pDeleteDC */
  NULL,                /* pDeleteObject */
  NULL,                /* pDeviceCapabilities */
  NULL,                /* pEllipse */
  NULL,                /* pEndDoc */
  NULL,                /* pEndPage */
  NULL,                /* pEnumDeviceFonts */
  TTYDRV_DC_Escape,    /* pEscape */
  NULL,                /* pExcludeClipRect */
  NULL,                /* pExcludeVisRect */
  NULL,                /* pExtDeviceMode */
  NULL,                /* pExtFloodFill */
  NULL,                /* pExtTextOut */
  NULL,                /* pGetCharWidth */
  NULL,                /* pGetPixel */
  NULL,                /* pGetTextExtentPoint */
  NULL,                /* pGetTextMetrics */
  NULL,                /* pIntersectClipRect */
  NULL,                /* pIntersectVisRect */
  NULL,                /* pLineTo */
  NULL,                /* pLoadOEMResource */
  NULL,                /* pMoveToEx */
  NULL,                /* pOffsetClipRgn */
  NULL,                /* pOffsetViewportOrg (optional) */
  NULL,                /* pOffsetWindowOrg (optional) */
  NULL,                /* pPaintRgn */
  NULL,                /* pPatBlt */
  NULL,                /* pPie */
  NULL,                /* pPolyPolygon */
  NULL,                /* pPolyPolyline */
  NULL,                /* pPolygon */
  NULL,                /* pPolyline */
  NULL,                /* pPolyBezier */
  NULL,                /* pRealizePalette */
  NULL,                /* pRectangle */
  NULL,                /* pRestoreDC */
  NULL,                /* pRoundRect */
  NULL,                /* pSaveDC */
  NULL,                /* pScaleViewportExt (optional) */
  NULL,                /* pScaleWindowExt (optional) */
  NULL,                /* pSelectClipRgn */
  NULL,                /* pSelectObject */
  NULL,                /* pSelectPalette */
  NULL,                /* pSetBkColor */
  NULL,                /* pSetBkMode */
  NULL,                /* pSetDeviceClipping */
  NULL,                /* pSetDIBitsToDevice */
  NULL,                /* pSetMapMode (optional) */
  NULL,                /* pSetMapperFlags */
  NULL,                /* pSetPixel */
  NULL,                /* pSetPolyFillMode */
  NULL,                /* pSetROP2 */
  NULL,                /* pSetRelAbs */
  NULL,                /* pSetStretchBltMode */
  NULL,                /* pSetTextAlign */
  NULL,                /* pSetTextCharacterExtra */
  NULL,                /* pSetTextColor */
  NULL,                /* pSetTextJustification */
  NULL,                /* pSetViewportExt (optional) */
  NULL,                /* pSetViewportOrg (optional) */
  NULL,                /* pSetWindowExt (optional) */
  NULL,                /* pSetWindowOrg (optional) */
  NULL,                /* pStartDoc */
  NULL,                /* pStartPage */
  NULL,                /* pStretchBlt */
  NULL                 /* pStretchDIBits */
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

static DeviceCaps TTYDRV_DC_DevCaps = {
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

/**********************************************************************
 *	     TTYDRV_DC_CreateDC
 */
BOOL TTYDRV_DC_CreateDC(DC *dc, LPCSTR driver, LPCSTR device,
			    LPCSTR output, const DEVMODEA *initData)
{
  FIXME(ttydrv, "(%p, %s, %s, %s, %p): semistub\n",
    dc, debugstr_a(driver), debugstr_a(device), 
    debugstr_a(output), initData
  );

  dc->physDev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
			   sizeof(TTYDRV_PDEVICE) );
  
  if(!dc->physDev) {
    ERR(ttydrv, "Can't allocate physDev\n");
    return FALSE;
  }

  dc->w.devCaps = &TTYDRV_DC_DevCaps;
  
  return TRUE;
}


/**********************************************************************
 *	     TTYDRV_DC_DeleteDC
 */
BOOL TTYDRV_DC_DeleteDC(DC *dc)
{
  FIXME(ttydrv, "(%p): semistub\n", dc);

  HeapFree( GetProcessHeap(), 0, dc->physDev );
  dc->physDev = NULL;
  
  return TRUE;
}

/**********************************************************************
 *           TTYDRV_DC_Escape
 */
INT TTYDRV_DC_Escape(DC *dc, INT nEscape, INT cbInput,
			 SEGPTR lpInData, SEGPTR lpOutData)
{
    return 0;
}
