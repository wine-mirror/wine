/*
 *	PostScript driver initialization functions
 *
 *	Copyright 1998 Huw D M Davies
 *	Copyright 2001 Marcus Meissner
 *
 */

#include "config.h"

#include <string.h>
#include <unistd.h>

#include "gdi.h"
#include "psdrv.h"
#include "debugtools.h"
#include "heap.h"
#include "winreg.h"
#include "winspool.h"
#include "winerror.h"

#ifdef HAVE_CUPS
# include <cups/cups.h>
#endif

DEFAULT_DEBUG_CHANNEL(psdrv);

static BOOL PSDRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODEA* initData );
static BOOL PSDRV_DeleteDC( DC *dc );

static const DC_FUNCTIONS PSDRV_Funcs =
{
    NULL,                            /* pAbortDoc */
    NULL,                            /* pAbortPath */
    NULL,                            /* pAngleArc */
    PSDRV_Arc,                       /* pArc */
    NULL,                            /* pArcTo */
    NULL,                            /* pBeginPath */
    NULL,                            /* pBitBlt */
    NULL,                            /* pBitmapBits */
    NULL,                            /* pChoosePixelFormat */
    PSDRV_Chord,                     /* pChord */
    NULL,                            /* pCloseFigure */
    NULL,                            /* pCreateBitmap */
    PSDRV_CreateDC,                  /* pCreateDC */
    NULL,                            /* pCreateDIBSection */
    PSDRV_DeleteDC,                  /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    NULL,                            /* pDescribePixelFormat */
    PSDRV_DeviceCapabilities,        /* pDeviceCapabilities */
    PSDRV_Ellipse,                   /* pEllipse */
    PSDRV_EndDoc,                    /* pEndDoc */
    PSDRV_EndPage,                   /* pEndPage */
    NULL,                            /* pEndPath */
    PSDRV_EnumDeviceFonts,           /* pEnumDeviceFonts */
    PSDRV_Escape,                    /* pEscape */
    NULL,                            /* pExcludeClipRect */
    PSDRV_ExtDeviceMode,             /* pExtDeviceMode */
    NULL,                            /* pExtFloodFill */
    PSDRV_ExtTextOut,                /* pExtTextOut */
    NULL,                            /* pFillPath */
    NULL,                            /* pFillRgn */
    NULL,                            /* pFlattenPath */
    NULL,                            /* pFrameRgn */
    PSDRV_GetCharWidth,              /* pGetCharWidth */
    NULL,                            /* pGetDCOrgEx */
    NULL,                            /* pGetDeviceGammaRamp */
    NULL,                            /* pGetPixel */
    NULL,                            /* pGetPixelFormat */
    PSDRV_GetTextExtentPoint,        /* pGetTextExtentPoint */
    PSDRV_GetTextMetrics,            /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pInvertRgn */
    PSDRV_LineTo,                    /* pLineTo */
    NULL,                            /* pMoveTo */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg (optional) */
    NULL,                            /* pOffsetWindowOrg (optional) */
    NULL,                            /* pPaintRgn */
    PSDRV_PatBlt,                    /* pPatBlt */
    PSDRV_Pie,                       /* pPie */
    NULL,                            /* pPolyBezier */
    NULL,                            /* pPolyBezierTo */
    NULL,                            /* pPolyDraw */
    PSDRV_PolyPolygon,               /* pPolyPolygon */
    PSDRV_PolyPolyline,              /* pPolyPolyline */
    PSDRV_Polygon,                   /* pPolygon */
    PSDRV_Polyline,                  /* pPolyline */
    NULL,                            /* pPolylineTo */
    NULL,                            /* pRealizePalette */
    PSDRV_Rectangle,                 /* pRectangle */
    NULL,                            /* pRestoreDC */
    PSDRV_RoundRect,                 /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExt (optional) */
    NULL,                            /* pScaleWindowExt (optional) */
    NULL,                            /* pSelectClipPath */
    NULL,                            /* pSelectClipRgn */
    PSDRV_SelectObject,              /* pSelectObject */
    NULL,                            /* pSelectPalette */
    PSDRV_SetBkColor,                /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    PSDRV_SetDeviceClipping,         /* pSetDeviceClipping */
    NULL,                            /* pSetDeviceGammaRamp */
    NULL,                            /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode (optional) */
    NULL,                            /* pSetMapperFlags */
    PSDRV_SetPixel,                  /* pSetPixel */
    NULL,                            /* pSetPixelFormat */
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
    PSDRV_StartDoc,                  /* pStartDoc */
    PSDRV_StartPage,                 /* pStartPage */
    NULL,                            /* pStretchBlt */
    PSDRV_StretchDIBits,             /* pStretchDIBits */
    NULL,                            /* pStrokeAndFillPath */
    NULL,                            /* pStrokePath */
    NULL,                            /* pSwapBuffers */
    NULL                             /* pWidenPath */
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
/* numColors */		0xffff,
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
/* dmFields */		DM_ORIENTATION | DM_PAPERSIZE | DM_SCALE | 
			DM_COPIES | DM_DEFAULTSOURCE | DM_COLOR | 
			DM_DUPLEX | DM_YRESOLUTION | DM_TTOPTION,
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
/* dmColor */		DMCOLOR_COLOR,
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
/* dmDisplayFrequency */ 0,
/* dmICMMethod */       0,
/* dmICMIntent */       0,
/* dmMediaType */       0,
/* dmDitherType */      0,
/* dmReserved1 */       0,
/* dmReserved2 */       0,
/* dmPanningWidth */    0,
/* dmPanningHeight */   0
  },
  { /* dmDocPrivate */
    /* dummy */ 0
  },
  { /* dmDrvPrivate */
    /* numInstalledOptions */ 0 
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
BOOL WINAPI PSDRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("(0x%4x, 0x%08lx, %p)\n", hinst, reason, reserved);
   
    switch(reason) {

	case DLL_PROCESS_ATTACH:

	    PSDRV_Heap = HeapCreate(0, 0x10000, 0);
	    if (PSDRV_Heap == (HANDLE)NULL)
		return FALSE;

	    if (PSDRV_GetFontMetrics() == FALSE) {
		HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }

	    PSDRV_DefaultFont = CreateFontIndirectA(&DefaultLogFont);
	    if (PSDRV_DefaultFont == (HANDLE)NULL) {
		HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }

	    /* Register driver as "WINEPS", "WINEPS.DLL" and "WINEPS.DRV"
		to allow an easy configuring for users */

	    if (DRIVER_RegisterDriver("WINEPS", &PSDRV_Funcs) == FALSE) {
	 	HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }

	    if (DRIVER_RegisterDriver("WINEPS.DLL", &PSDRV_Funcs) == FALSE) {
		DRIVER_UnregisterDriver("WINEPS");
		HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }

	    if (DRIVER_RegisterDriver("WINEPS.DRV", &PSDRV_Funcs) == FALSE) {
		DRIVER_UnregisterDriver("WINEPS");
		DRIVER_UnregisterDriver("WINEPS.DLL");
		HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }

            break;

	case DLL_PROCESS_DETACH:

	    DeleteObject( PSDRV_DefaultFont );
	    HeapDestroy( PSDRV_Heap );
	    DRIVER_UnregisterDriver( "WINEPS" );
	    DRIVER_UnregisterDriver( "WINEPS.DLL" );
	    DRIVER_UnregisterDriver( "WINEPS.DRV" );

            break;
    }
 
    return TRUE;
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
    PAGESIZE *page;
    INT width = 0, height = 0;

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

    /* Are aspect[XY] and logPixels[XY] correct? */
    /* Need to handle different res in x and y => fix ppd */
    devCaps->aspectX = devCaps->logPixelsX = 
				physDev->pi->ppd->DefaultResolution;
    devCaps->aspectY = devCaps->logPixelsY = 
				physDev->pi->ppd->DefaultResolution;
    devCaps->aspectXY = (int)hypot( (double)devCaps->aspectX, 
				    (double)devCaps->aspectY );


    for(page = pi->ppd->PageSizes; page; page = page->next) {
        if(page->WinPage == physDev->Devmode->dmPublic.u1.s1.dmPaperSize)
	    break;
    }
    if(!page) {
        FIXME("Can't find page\n");
	physDev->PageSize.left = 0;
	physDev->PageSize.right = 0;
	physDev->PageSize.bottom = 0;
        physDev->PageSize.top = 0;
    } else if(page->ImageableArea) { /* PageSize is in device units */
        physDev->PageSize.left = page->ImageableArea->llx *
	  devCaps->logPixelsX / 72;
       physDev->PageSize.right = page->ImageableArea->urx *
	  devCaps->logPixelsX / 72;
       physDev->PageSize.bottom = page->ImageableArea->lly *
	  devCaps->logPixelsY / 72;
       physDev->PageSize.top = page->ImageableArea->ury *
	  devCaps->logPixelsY / 72;
    } else {
        physDev->PageSize.left = physDev->PageSize.bottom = 0;
	physDev->PageSize.right = page->PaperDimension->x *
	  devCaps->logPixelsX / 72;
	physDev->PageSize.top = page->PaperDimension->y *
	  devCaps->logPixelsY / 72;
    }
    TRACE("PageSize = (%d,%d - %d,%d)\n", physDev->PageSize.left, physDev->PageSize.bottom, physDev->PageSize.right, physDev->PageSize.top);

    /* these are in device units */
    width = physDev->PageSize.right - physDev->PageSize.left;
    height = physDev->PageSize.top - physDev->PageSize.bottom;

    if(physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_PORTRAIT) {
        devCaps->horzRes = width;
	devCaps->vertRes = height;
    } else {
        devCaps->horzRes = height;
	devCaps->vertRes = width;
    }

    /* these are in mm */
    devCaps->horzSize = (devCaps->horzRes * 25.4) / devCaps->logPixelsX;
    devCaps->vertSize = (devCaps->vertRes * 25.4) / devCaps->logPixelsY;

    TRACE("devcaps: horzSize = %dmm, vertSize = %dmm, "
	  "horzRes = %d, vertRes = %d\n",
	  devCaps->horzSize, devCaps->vertSize,
	  devCaps->horzRes, devCaps->vertRes);

    if(physDev->pi->ppd->ColorDevice) {
        devCaps->bitsPixel = 8;
	devCaps->numColors = 256;
	/* FIXME are these values OK? */
    }

    /* etc */

    dc->devCaps = devCaps;

    dc->hVisRgn = CreateRectRgn(0, 0, dc->devCaps->horzRes,
    			    dc->devCaps->vertRes);
    
    dc->hFont = PSDRV_DefaultFont;
    physDev->job.output = output ?
      HEAP_strdupA( PSDRV_Heap, 0, output ) :
      HEAP_strdupA( PSDRV_Heap, 0, "LPT1:" );  /* HACK */
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
    HeapFree( PSDRV_Heap, 0, (void *)dc->devCaps );
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
    DWORD type = REG_BINARY, needed, res, dwPaperSize;
    PRINTERINFO *pi = PSDRV_PrinterList, **last = &PSDRV_PrinterList;
    FONTNAME *font;
    AFM *afm;
    HANDLE hPrinter;
    const char *ppd = NULL;
    char ppdFileName[256];
    HKEY hkey;

    TRACE("'%s'\n", name);
    
    /*
     *	If this loop completes, last will point to the 'next' element of the
     *	final PRINTERINFO in the list
     */    
    for( ; pi; last = &pi->next, pi = pi->next)
        if(!strcmp(pi->FriendlyName, name))
	    return pi;

    pi = *last = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(*pi) );
    if (pi == NULL)
    	return NULL;
	
    pi->FriendlyName = HEAP_strdupA( PSDRV_Heap, 0, name );
    if (pi->FriendlyName == NULL)
    	goto fail;
	
    /* Use Get|SetPrinterDataExA instead? */
    
    res = DrvGetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE, &type,
			    NULL, 0, &needed );

    if(res == ERROR_INVALID_PRINTER_NAME || needed != sizeof(DefaultDevmode)) {
        pi->Devmode = HeapAlloc( PSDRV_Heap, 0, sizeof(DefaultDevmode) );
	if (pi->Devmode == NULL)
	    goto cleanup;	    
	memcpy(pi->Devmode, &DefaultDevmode, sizeof(DefaultDevmode) );
	strcpy(pi->Devmode->dmPublic.dmDeviceName,name);
	DrvSetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE,
		 REG_BINARY, (LPBYTE)&DefaultDevmode, sizeof(DefaultDevmode) );

	/* need to do something here AddPrinter?? */
    }
    else {
        pi->Devmode = HeapAlloc( PSDRV_Heap, 0, needed );
	DrvGetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE, &type,
			  (LPBYTE)pi->Devmode, needed, &needed);
    }

    if (OpenPrinterA (pi->FriendlyName, &hPrinter, NULL) == 0) {
	ERR ("OpenPrinterA failed with code %li\n", GetLastError ());
	goto cleanup;
    }
    
    ppdFileName[0]='\0';
    
#ifdef HAVE_CUPS
    {
	ppd = cupsGetPPD(name);

	if (ppd) {
	    strncpy(ppdFileName, ppd, sizeof(ppdFileName));
	    res = ERROR_SUCCESS;
	    /* we should unlink() that file later */
	} else {
	    res = ERROR_FILE_NOT_FOUND;
	    WARN("Did not find ppd for %s\n",name);
	}
    }
#endif

    if (!ppdFileName[0]) {
        res = GetPrinterDataA (hPrinter, "PPD File", NULL, ppdFileName,
	    	sizeof(ppdFileName), &needed);
    }
    /* Look for a ppd file for this printer in the config file.
     * First look for the names of the printer, then for 'generic'
     */
    if((res != ERROR_SUCCESS) && !RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\ppd", &hkey))
    {
	DWORD count = sizeof(ppdFileName);
	ppdFileName[0] = 0;
	if(RegQueryValueExA(hkey, name, 0, &type, ppdFileName, &count) != ERROR_SUCCESS)
	    RegQueryValueExA(hkey, "generic", 0, &type, ppdFileName, &count);
	RegCloseKey(hkey);
    }

    if(!ppdFileName[0])
	res = ERROR_FILE_NOT_FOUND;
    else 
	res = ERROR_SUCCESS;

    if (res != ERROR_SUCCESS) {
	ERR ("Error %li getting PPD file name for printer '%s'\n", res, name);
	goto closeprinter;
    }
    
    ppdFileName[sizeof(ppdFileName) - 1] = '\0';
    
    pi->ppd = PSDRV_ParsePPD(ppdFileName);
    if(!pi->ppd) {
	MESSAGE("Couldn't find PPD file '%s', expect a crash now!\n",
	    ppdFileName);
	goto closeprinter;
    }
    
    /*
     *	This is a hack.  The default paper size should be read in as part of
     *	the Devmode structure, but Wine doesn't currently provide a convenient
     *	way to configure printers.
     */
    res = GetPrinterDataA (hPrinter, "Paper Size", NULL, (LPBYTE) &dwPaperSize,
	    sizeof (DWORD), &needed);
    if (res == ERROR_SUCCESS)
	pi->Devmode->dmPublic.u1.s1.dmPaperSize = (SHORT) dwPaperSize;
    else if (res == ERROR_FILE_NOT_FOUND)
	TRACE ("No 'Paper Size' for printer '%s'\n", name);
    else {
	ERR ("GetPrinterDataA returned %li\n", res);
	goto closeprinter;
    }

    res = EnumPrinterDataExA (hPrinter, "PrinterDriverData\\FontSubTable", NULL,
	    0, &needed, &pi->FontSubTableSize);
    if (res == ERROR_SUCCESS || res == ERROR_FILE_NOT_FOUND) {
	TRACE ("No 'FontSubTable' for printer '%s'\n", name);
    }
    else if (res == ERROR_MORE_DATA) {
	pi->FontSubTable = HeapAlloc (PSDRV_Heap, 0, needed);
	if (pi->FontSubTable == NULL) {
	    ERR ("Failed to allocate %li bytes from heap\n", needed);
	    goto closeprinter;
	}

	res = EnumPrinterDataExA (hPrinter, "PrinterDriverData\\FontSubTable",
		(LPBYTE) pi->FontSubTable, needed, &needed,
		&pi->FontSubTableSize);
	if (res != ERROR_SUCCESS) {
	    ERR ("EnumPrinterDataExA returned %li\n", res);
	    goto closeprinter;
	}
    }
    else {
	ERR("EnumPrinterDataExA returned %li\n", res);
	goto closeprinter;
    }

    if (ClosePrinter (hPrinter) == 0) {
	ERR ("ClosePrinter failed with code %li\n", GetLastError ());
	goto cleanup;
    }

    pi->next = NULL;
    pi->Fonts = NULL;

    for(font = pi->ppd->InstalledFonts; font; font = font->next) {
        afm = PSDRV_FindAFMinList(PSDRV_AFMFontList, font->Name);
	if(!afm) {
	    TRACE( "Couldn't find AFM file for installed printer font '%s' - "
	    	    "ignoring\n", font->Name);
	}
	else {
	    if (PSDRV_AddAFMtoList(&pi->Fonts, afm) == FALSE) {
	    	PSDRV_FreeAFMList(pi->Fonts);
		goto cleanup;
	    }
	}

    }
    if (ppd) unlink(ppd);
    return pi;

closeprinter:
    ClosePrinter(hPrinter);
cleanup:
    if (pi->FontSubTable)
    	HeapFree(PSDRV_Heap, 0, pi->FontSubTable);
    if (pi->FriendlyName)
    	HeapFree(PSDRV_Heap, 0, pi->FriendlyName);
    if (pi->Devmode)
    	HeapFree(PSDRV_Heap, 0, pi->Devmode);
fail:
    HeapFree(PSDRV_Heap, 0, pi);
    if (ppd) unlink(ppd);
    *last = NULL;
    return NULL;
}
