/*
 *	PostScript driver initialization functions
 *
 *	Copyright 1998 Huw D M Davies
 *
 */
#include <string.h>

#include "gdi.h"
#include "psdrv.h"
#include "debugtools.h"
#include "heap.h"
#include "winreg.h"
#include "winspool.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(psdrv)

static BOOL PSDRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODEA* initData );
static BOOL PSDRV_DeleteDC( DC *dc );

static const DC_FUNCTIONS PSDRV_Funcs =
{
    NULL,                            /* pAbortDoc */
    PSDRV_Arc,                       /* pArc */
    NULL,                            /* pBitBlt */
    NULL,                            /* pBitmapBits */
    PSDRV_Chord,                     /* pChord */
    NULL,                            /* pCreateBitmap */
    PSDRV_CreateDC,                  /* pCreateDC */
    NULL,                            /* pCreateDIBSection */
    NULL,                            /* pCreateDIBSection16 */
    PSDRV_DeleteDC,                  /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    PSDRV_DeviceCapabilities,        /* pDeviceCapabilities */
    PSDRV_Ellipse,                   /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    PSDRV_EnumDeviceFonts,           /* pEnumDeviceFonts */
    PSDRV_Escape,                    /* pEscape */
    NULL,                            /* pExcludeClipRect */
    PSDRV_ExtDeviceMode,             /* pExtDeviceMode */
    NULL,                            /* pExtFloodFill */
    PSDRV_ExtTextOut,                /* pExtTextOut */
    NULL,                            /* pFillRgn */
    NULL,                            /* pFrameRgn */
    PSDRV_GetCharWidth,              /* pGetCharWidth */
    NULL,                            /* pGetPixel */
    PSDRV_GetTextExtentPoint,        /* pGetTextExtentPoint */
    PSDRV_GetTextMetrics,            /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pInvertRgn */
    PSDRV_LineTo,                    /* pLineTo */
    NULL,                            /* pLoadOEMResource */
    PSDRV_MoveToEx,                  /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg (optional) */
    NULL,                            /* pOffsetWindowOrg (optional) */
    NULL,                            /* pPaintRgn */
    PSDRV_PatBlt,                    /* pPatBlt */
    PSDRV_Pie,                       /* pPie */
    PSDRV_PolyPolygon,               /* pPolyPolygon */
    PSDRV_PolyPolyline,              /* pPolyPolyline */
    PSDRV_Polygon,                   /* pPolygon */
    PSDRV_Polyline,                  /* pPolyline */
    NULL,                            /* pPolyBezier */		     
    NULL,                            /* pRealizePalette */
    PSDRV_Rectangle,                 /* pRectangle */
    NULL,                            /* pRestoreDC */
    PSDRV_RoundRect,                 /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExt (optional) */
    NULL,                            /* pScaleWindowExt (optional) */
    NULL,                            /* pSelectClipRgn */
    PSDRV_SelectObject,              /* pSelectObject */
    NULL,                            /* pSelectPalette */
    PSDRV_SetBkColor,                /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode (optional) */
    NULL,                            /* pSetMapperFlags */
    PSDRV_SetPixel,                  /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    PSDRV_SetTextColor,              /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExt (optional) */
    NULL,                            /* pSetViewportOrg (optional) */
    NULL,                            /* pSetWindowExt (optional) */
    NULL,                            /* pSetWindowOrg (optional) */
    NULL,                            /* pStartDoc */
    NULL,                            /* pStartPage */
    NULL,                            /* pStretchBlt */
    PSDRV_StretchDIBits              /* pStretchDIBits */
};


/* Default entries for devcaps */

static DeviceCaps PSDRV_DevCaps = {
/* version */		0, 
/* technology */	DT_RASPRINTER,
/* horzSize */		210,
/* vertSize */		297,
/* horzRes */		4961,
/* vertRes */		7016, 
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
/* textCaps */		TC_CR_ANY, /* psdrv 0x59f7 */
/* clipCaps */		CP_RECTANGLE,
/* rasterCaps */	RC_BITBLT | RC_BITMAP64 | RC_GDI20_OUTPUT |
			RC_DIBTODEV | RC_STRETCHBLT |
			RC_STRETCHDIB, /* psdrv 0x6e99 */
/* aspectX */		600,
/* aspectY */		600,
/* aspectXY */		848,
/* pad1 */		{ 0 },
/* logPixelsX */	600,
/* logPixelsY */	600, 
/* pad2 */		{ 0 },
/* palette size */	0,
/* ..etc */		0, 0 };

static PSDRV_DEVMODEA DefaultDevmode = 
{
  { /* dmPublic */
/* dmDeviceName */	"Wine PostScript Driver",
/* dmSpecVersion */	0x30a,
/* dmDriverVersion */	0x001,
/* dmSize */		sizeof(DEVMODEA),
/* dmDriverExtra */	0,
/* dmFields */		DM_ORIENTATION | DM_PAPERSIZE | DM_PAPERLENGTH |
			DM_PAPERWIDTH | DM_SCALE | DM_COPIES | 
			DM_DEFAULTSOURCE | DM_COLOR | DM_DUPLEX | 
			DM_YRESOLUTION | DM_TTOPTION,
   { /* u1 */
     { /* s1 */
/* dmOrientation */	DMORIENT_PORTRAIT,
/* dmPaperSize */	DMPAPER_A4,
/* dmPaperLength */	2969,
/* dmPaperWidth */      2101
     }
   },
/* dmScale */		100, /* ?? */
/* dmCopies */		1,
/* dmDefaultSource */	DMBIN_AUTO,
/* dmPrintQuality */	0,
/* dmColor */		DMCOLOR_MONOCHROME,
/* dmDuplex */		0,
/* dmYResolution */	0,
/* dmTTOption */	DMTT_SUBDEV,
/* dmCollate */		0,
/* dmFormName */	"",
/* dmUnusedPadding */   0,
/* dmBitsPerPel */	0,
/* dmPelsWidth */	0,
/* dmPelsHeight */	0,
/* dmDisplayFlags */	0,
/* dmDisplayFrequency */ 0
  },
  { /* dmDocPrivate */
    0 /* dummy */
  },
  { /* dmDrvPrivate */
/* ppdfilename */	"default.ppd"
  }
};

HANDLE PSDRV_Heap = 0;

static HANDLE PSDRV_DefaultFont = 0;
static LOGFONTA DefaultLogFont = {
    100, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0,
    DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, ""
};

/*********************************************************************
 *	     PSDRV_Init
 *
 * Initializes font metrics and registers driver. Called from GDI_Init()
 *
 */
BOOL PSDRV_Init(void)
{
    TRACE("\n");
    PSDRV_Heap = HeapCreate(0, 0x10000, 0);
    PSDRV_GetFontMetrics();
    PSDRV_DefaultFont = CreateFontIndirectA(&DefaultLogFont);
    return DRIVER_RegisterDriver( "WINEPS", &PSDRV_Funcs );
}

/**********************************************************************
 *	     PSDRV_CreateDC
 */
static BOOL PSDRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODEA* initData )
{
    PSDRV_PDEVICE *physDev;
    PRINTERINFO *pi;
    DeviceCaps *devCaps;

    /* If no device name was specified, retrieve the device name
     * from the DEVMODE structure from the DC's physDev.
     * (See CreateCompatibleDC) */
    if ( !device && dc->physDev )
    {
        physDev = (PSDRV_PDEVICE *)dc->physDev;
        device = physDev->Devmode->dmPublic.dmDeviceName;
    }
    pi = PSDRV_FindPrinterInfo(device);
        
    TRACE("(%s %s %s %p)\n", driver, device, output, initData);

    if(!pi) return FALSE;

    if(!pi->Fonts) {
        MESSAGE("To use WINEPS you need to install some AFM files.\n");
	return FALSE;
    }

    physDev = (PSDRV_PDEVICE *)HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY,
					             sizeof(*physDev) );
    if (!physDev) return FALSE;
    dc->physDev = physDev;

    physDev->pi = pi;

    physDev->Devmode = (PSDRV_DEVMODEA *)HeapAlloc( PSDRV_Heap, 0,
						     sizeof(PSDRV_DEVMODEA) );
    if(!physDev->Devmode) {
        HeapFree( PSDRV_Heap, 0, physDev );
	return FALSE;
    }
    
    memcpy( physDev->Devmode, pi->Devmode, sizeof(PSDRV_DEVMODEA) );

    if(initData) {
        PSDRV_MergeDevmodes(physDev->Devmode, (PSDRV_DEVMODEA *)initData, pi);
    }

    
    devCaps = HeapAlloc( PSDRV_Heap, 0, sizeof(PSDRV_DevCaps) );
    memcpy(devCaps, &PSDRV_DevCaps, sizeof(PSDRV_DevCaps));

    if(physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_PORTRAIT) {
        devCaps->horzSize = physDev->Devmode->dmPublic.u1.s1.dmPaperWidth / 10;
	devCaps->vertSize = physDev->Devmode->dmPublic.u1.s1.dmPaperLength / 10;
    } else {
        devCaps->horzSize = physDev->Devmode->dmPublic.u1.s1.dmPaperLength / 10;
	devCaps->vertSize = physDev->Devmode->dmPublic.u1.s1.dmPaperWidth / 10;
    }

    devCaps->horzRes = physDev->pi->ppd->DefaultResolution * 
      devCaps->horzSize / 25.4;
    devCaps->vertRes = physDev->pi->ppd->DefaultResolution * 
      devCaps->vertSize / 25.4;

    /* Are aspect[XY] and logPixels[XY] correct? */
    /* Need to handle different res in x and y => fix ppd */
    devCaps->aspectX = devCaps->logPixelsX = 
				physDev->pi->ppd->DefaultResolution;
    devCaps->aspectY = devCaps->logPixelsY = 
				physDev->pi->ppd->DefaultResolution;
    devCaps->aspectXY = (int)hypot( (double)devCaps->aspectX, 
				    (double)devCaps->aspectY );

    if(physDev->pi->ppd->ColorDevice) {
        devCaps->bitsPixel = 8;
	devCaps->numColors = 256;
	/* FIXME are these values OK? */
    }

    /* etc */

    dc->w.devCaps = devCaps;

    dc->w.hVisRgn = CreateRectRgn(0, 0, dc->w.devCaps->horzRes,
    			    dc->w.devCaps->vertRes);
    
    dc->w.hFont = PSDRV_DefaultFont;
    physDev->job.output = output ? HEAP_strdupA( PSDRV_Heap, 0, output ) :
      NULL;
    physDev->job.hJob = 0;
    return TRUE;
}


/**********************************************************************
 *	     PSDRV_DeleteDC
 */
static BOOL PSDRV_DeleteDC( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    
    TRACE("\n");

    HeapFree( PSDRV_Heap, 0, physDev->Devmode );
    HeapFree( PSDRV_Heap, 0, physDev->job.output );
    HeapFree( PSDRV_Heap, 0, (void *)dc->w.devCaps );
    HeapFree( PSDRV_Heap, 0, physDev );
    dc->physDev = NULL;

    return TRUE;
}


	

/**********************************************************************
 *		PSDRV_FindPrinterInfo
 */
PRINTERINFO *PSDRV_FindPrinterInfo(LPCSTR name) 
{
    static PRINTERINFO *PSDRV_PrinterList;
    DWORD type = REG_BINARY, needed, res;
    PRINTERINFO *pi = PSDRV_PrinterList, **last = &PSDRV_PrinterList;
    FONTNAME *font;
    AFM *afm;

    TRACE("'%s'\n", name);
    
    for( ; pi; last = &pi->next, pi = pi->next) {
        if(!strcmp(pi->FriendlyName, name))
	    return pi;
    }

    pi = *last = HeapAlloc( PSDRV_Heap, 0, sizeof(*pi) );
    pi->FriendlyName = HEAP_strdupA( PSDRV_Heap, 0, name );
    res = DrvGetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE, &type,
			    NULL, 0, &needed );

    if(res == ERROR_INVALID_PRINTER_NAME || needed != sizeof(DefaultDevmode)) {
        pi->Devmode = HeapAlloc( PSDRV_Heap, 0, sizeof(DefaultDevmode) );
	memcpy(pi->Devmode, &DefaultDevmode, sizeof(DefaultDevmode) );
	DrvSetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE,
		 REG_BINARY, (LPBYTE)&DefaultDevmode, sizeof(DefaultDevmode) );

	/* need to do something here AddPrinter?? */
    } else {
        pi->Devmode = HeapAlloc( PSDRV_Heap, 0, needed );
	DrvGetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE, &type,
			  (LPBYTE)pi->Devmode, needed, &needed);
    }

    pi->ppd = PSDRV_ParsePPD(pi->Devmode->dmDrvPrivate.ppdFileName);
    if(!pi->ppd) {
        HeapFree(PSDRV_Heap, 0, pi->FriendlyName);
        HeapFree(PSDRV_Heap, 0, pi->Devmode);
        HeapFree(PSDRV_Heap, 0, pi);
	*last = NULL;
	MESSAGE("Couldn't find PPD file '%s', expect a crash now!\n",
	    pi->Devmode->dmDrvPrivate.ppdFileName);
	return NULL;
    }

    pi->next = NULL;
    pi->Fonts = NULL;

    for(font = pi->ppd->InstalledFonts; font; font = font->next) {
        afm = PSDRV_FindAFMinList(PSDRV_AFMFontList, font->Name);
	if(!afm) {
	    MESSAGE(
	 "Couldn't find AFM file for installed printer font '%s' - ignoring\n",
	 font->Name);
	} else {
	    PSDRV_AddAFMtoList(&pi->Fonts, afm);
	}
    }

    return pi;
}
