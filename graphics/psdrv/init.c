/*
 *	Postscript driver initialization functions
 *
 *	Copyright 1998 Huw D M Davies
 *
 */

#include "windows.h"
#include "gdi.h"
#include "psdrv.h"
#include "debug.h"
#include "heap.h"

static BOOL32 PSDRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE16* initData );
static BOOL32 PSDRV_DeleteDC( DC *dc );

static const DC_FUNCTIONS PSDRV_Funcs =
{
    NULL,                            /* pArc */
    NULL,                            /* pBitBlt */
    NULL,                            /* pChord */
    PSDRV_CreateDC,                  /* pCreateDC */
    PSDRV_DeleteDC,                  /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    PSDRV_Ellipse,                   /* pEllipse */
    PSDRV_EnumDeviceFonts,           /* pEnumDeviceFonts */
    PSDRV_Escape,                    /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    NULL,                            /* pExtFloodFill */
    PSDRV_ExtTextOut,                /* pExtTextOut */
    NULL,                            /* pGetCharWidth */
    NULL,                            /* pGetPixel */
    PSDRV_GetTextExtentPoint,        /* pGetTextExtentPoint */
    PSDRV_GetTextMetrics,            /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    PSDRV_LineTo,                    /* pLineTo */
    PSDRV_MoveToEx,                  /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg (optional) */
    NULL,                            /* pOffsetWindowOrg (optional) */
    NULL,                            /* pPaintRgn */
    NULL,                            /* pPatBlt */
    NULL,                            /* pPie */
    NULL,                            /* pPolyPolygon */
    NULL,                            /* pPolyPolyline */
    NULL,                            /* pPolygon */
    NULL,                            /* pPolyline */
    NULL,                            /* pRealizePalette */
    PSDRV_Rectangle,                 /* pRectangle */
    NULL,                            /* pRestoreDC */
    NULL,                            /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExt (optional) */
    NULL,                            /* pScaleWindowExt (optional) */
    NULL,                            /* pSelectClipRgn */
    PSDRV_SelectObject,              /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode (optional) */
    NULL,                            /* pSetMapperFlags */
    NULL,                            /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExt (optional) */
    NULL,                            /* pSetViewportOrg (optional) */
    NULL,                            /* pSetWindowExt (optional) */
    NULL,                            /* pSetWindowOrg (optional) */
    NULL,                            /* pStretchBlt */
    NULL                             /* pStretchDIBits */
};


/* Default entries for devcaps */

static DeviceCaps PSDRV_DevCaps = {
/* version */		0, 
/* technology */	DT_RASPRINTER,
/* horzSize */		200,
/* vertSize */		288,
/* horzRes */		4733,
/* vertRes */		6808, 
/* bitsPixel */		1,
/* planes */		1,
/* numBrushes */	-1,
/* numPens */		10,
/* numMarkers */	0,
/* numFonts */		39,
/* numColors */		2,
/* pdeviceSize */	0,	
/* curveCaps */		CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES |
			CC_WIDE | CC_STYLED | CC_WIDESTYLED | CC_INTERIORS |
			CC_ROUNDRECT,
/* lineCaps */		LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
			LC_STYLED | LC_WIDESTYLED | LC_INTERIORS,
/* polygoalnCaps */	PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON |
			PC_SCANLINE | PC_WIDE | PC_STYLED | PC_WIDESTYLED |
			PC_INTERIORS,
/* textCaps */		0, /* psdrv 0x59f7 */
/* clipCaps */		CP_RECTANGLE,
/* rasterCaps */	RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 |
			RC_DI_BITMAP | RC_DIBTODEV | RC_BIGFONT |
			RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS, 
			/* psdrv 0x6e99 */
/* aspectX */		600,
/* aspectY */		600,
/* aspectXY */		848,
/* pad1 */		{ 0 },
/* logPixelsX */	600,
/* logPixelsY */	600, 
/* pad2 */		{ 0 },
/* palette size */	0,
/* ..etc */		0, 0 };

/*********************************************************************
 *	     PSDRV_Init
 *
 * Initializes font metrics and registers driver. Called from GDI_Init()
 *
 */
BOOL32 PSDRV_Init(void)
{
  TRACE(psdrv, "\n");
  PSDRV_GetFontMetrics();
  return DRIVER_RegisterDriver( "WINEPS", &PSDRV_Funcs );
}


/**********************************************************************
 *	     PSDRV_CreateDC
 */
static BOOL32 PSDRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE16* initData )
{
    PSDRV_PDEVICE *physDev;

    TRACE(psdrv, "(%s %s %s %p)\n", driver, device, output, initData);

    if(!PSDRV_AFMFontList) {
        MSG("To use WINEPS you need to install some AFM files.\n");
	return FALSE;
    }

    dc->w.devCaps = &PSDRV_DevCaps;
    dc->w.hVisRgn = CreateRectRgn32(0, 0, dc->w.devCaps->horzRes,
    			    dc->w.devCaps->vertRes);
    
    physDev = (PSDRV_PDEVICE *)HeapAlloc( GetProcessHeap(), 0,
					             sizeof(*physDev) );
    if (!physDev) return FALSE;
    dc->physDev = physDev;
    physDev->job.output = HEAP_strdupA( GetProcessHeap(), 0, output );
    if (!physDev->job.output) return FALSE;
    physDev->job.hJob = 0;
    return TRUE;
}


/**********************************************************************
 *	     PSDRV_DeleteDC
 */
static BOOL32 PSDRV_DeleteDC( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    TRACE(psdrv, "\n");
    HeapFree( GetProcessHeap(), 0, physDev->job.output );
    HeapFree( GetProcessHeap(), 0, physDev );
    dc->physDev = NULL;
    return TRUE;
}

