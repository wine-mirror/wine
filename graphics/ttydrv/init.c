/*
 * TTY driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "config.h"

#include "gdi.h"
#include "bitmap.h"
#include "color.h"
#include "dc.h"
#include "heap.h"
#include "monitor.h"
#include "palette.h"
#include "ttydrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

static const DC_FUNCTIONS TTYDRV_DC_Driver =
{
  NULL,                /* pAbortDoc */
  TTYDRV_DC_Arc,       /* pArc */
  TTYDRV_DC_BitBlt,    /* pBitBlt */
  NULL,                /* pBitmapBits */
  TTYDRV_DC_Chord,     /* pChord */
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
  NULL,                /* pEnumDeviceFonts */
  TTYDRV_DC_Escape,    /* pEscape */
  NULL,                /* pExcludeClipRect */
  NULL,                /* pExtDeviceMode */
  TTYDRV_DC_ExtFloodFill, /* pExtFloodFill */
  TTYDRV_DC_ExtTextOut, /* pExtTextOut */
  NULL,                /* pFillRgn */
  NULL,                /* pFrameRgn */
  TTYDRV_DC_GetCharWidth, /* pGetCharWidth */
  TTYDRV_DC_GetPixel,  /* pGetPixel */
  TTYDRV_DC_GetTextExtentPoint, /* pGetTextExtentPoint */
  TTYDRV_DC_GetTextMetrics,  /* pGetTextMetrics */
  NULL,                /* pIntersectClipRect */
  NULL,                /* pIntersectVisRect */
  TTYDRV_DC_LineTo,    /* pLineTo */
  TTYDRV_DC_LoadOEMResource, /* pLoadOEMResource */
  TTYDRV_DC_MoveToEx,  /* pMoveToEx */
  NULL,                /* pOffsetClipRgn */
  NULL,                /* pOffsetViewportOrg (optional) */
  NULL,                /* pOffsetWindowOrg (optional) */
  TTYDRV_DC_PaintRgn,  /* pPaintRgn */
  TTYDRV_DC_PatBlt,    /* pPatBlt */
  TTYDRV_DC_Pie,       /* pPie */
  TTYDRV_DC_PolyPolygon, /* pPolyPolygon */
  TTYDRV_DC_PolyPolyline, /* pPolyPolyline */
  TTYDRV_DC_Polygon,   /* pPolygon */
  TTYDRV_DC_Polyline,  /* pPolyline */
  TTYDRV_DC_PolyBezier, /* pPolyBezier */
  NULL,                /* pRealizePalette */
  TTYDRV_DC_Rectangle, /* pRectangle */
  NULL,                /* pRestoreDC */
  TTYDRV_DC_RoundRect, /* pRoundRect */
  NULL,                /* pSaveDC */
  NULL,                /* pScaleViewportExt (optional) */
  NULL,                /* pScaleWindowExt (optional) */
  NULL,                /* pSelectClipRgn */
  TTYDRV_DC_SelectObject, /* pSelectObject */
  NULL,                /* pSelectPalette */
  TTYDRV_DC_SetBkColor, /* pSetBkColor */
  NULL,                /* pSetBkMode */
  TTYDRV_DC_SetDeviceClipping, /* pSetDeviceClipping */
  NULL,                /* pSetDIBitsToDevice */
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

/**********************************************************************
 *	     TTYDRV_DC_CreateDC
 */
BOOL TTYDRV_DC_CreateDC(DC *dc, LPCSTR driver, LPCSTR device,
			LPCSTR output, const DEVMODEA *initData)
{
  TTYDRV_PDEVICE *physDev;
  BITMAPOBJ *bmp;

  FIXME("(%p, %s, %s, %s, %p): semistub\n",
    dc, debugstr_a(driver), debugstr_a(device), 
    debugstr_a(output), initData);

  dc->physDev = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			  sizeof(TTYDRV_PDEVICE));  
  if(!dc->physDev) {
    ERR("Can't allocate physDev\n");
    return FALSE;
  }
  physDev = (TTYDRV_PDEVICE *) dc->physDev;
  
  dc->w.devCaps = &TTYDRV_DC_DevCaps;

  if(dc->w.flags & DC_MEMORY){
#ifdef HAVE_LIBCURSES
    physDev->window = NULL;
#endif /* defined(HAVE_LIBCURSES) */
    physDev->cellWidth = 1;
    physDev->cellHeight = 1;

    TTYDRV_DC_CreateBitmap(dc->w.hBitmap);
    bmp = (BITMAPOBJ *) GDI_GetObjPtr(dc->w.hBitmap, BITMAP_MAGIC);
				   
    dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;
    
    dc->w.totalExtent.left   = 0;
    dc->w.totalExtent.top    = 0;
    dc->w.totalExtent.right  = bmp->bitmap.bmWidth;
    dc->w.totalExtent.bottom = bmp->bitmap.bmHeight;
    dc->w.hVisRgn            = CreateRectRgnIndirect( &dc->w.totalExtent );
    
    GDI_HEAP_UNLOCK( dc->w.hBitmap );
  } else {
#ifdef HAVE_LIBCURSES
    physDev->window = TTYDRV_MONITOR_GetCursesRootWindow(&MONITOR_PrimaryMonitor);
#endif /* defined(HAVE_LIBCURSES) */
    physDev->cellWidth = TTYDRV_MONITOR_GetCellWidth(&MONITOR_PrimaryMonitor);
    physDev->cellHeight = TTYDRV_MONITOR_GetCellHeight(&MONITOR_PrimaryMonitor);
    
    dc->w.bitsPerPixel = MONITOR_GetDepth(&MONITOR_PrimaryMonitor);
    
    dc->w.totalExtent.left   = 0;
    dc->w.totalExtent.top    = 0;
    dc->w.totalExtent.right  = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
    dc->w.totalExtent.bottom = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
    dc->w.hVisRgn            = CreateRectRgnIndirect( &dc->w.totalExtent );    
  }

  return TRUE;
}


/**********************************************************************
 *	     TTYDRV_DC_DeleteDC
 */
BOOL TTYDRV_DC_DeleteDC(DC *dc)
{
  FIXME("(%p): semistub\n", dc);

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
