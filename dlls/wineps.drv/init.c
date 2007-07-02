/*
 *	PostScript driver initialization functions
 *
 *	Copyright 1998 Huw D M Davies
 *	Copyright 2001 Marcus Meissner
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_CUPS_CUPS_H
# include <cups/cups.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wownt32.h"
#include "winreg.h"
#include "psdrv.h"
#include "winspool.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

#ifndef SONAME_LIBCUPS
#define SONAME_LIBCUPS "libcups" SONAME_EXT
#endif

#ifdef HAVE_CUPS_CUPS_H
static void *cupshandle = NULL;
#endif

static const PSDRV_DEVMODEA DefaultDevmode =
{
  { /* dmPublic */
/* dmDeviceName */	"Wine PostScript Driver",
/* dmSpecVersion */	0x30a,
/* dmDriverVersion */	0x001,
/* dmSize */		sizeof(DEVMODEA),
/* dmDriverExtra */	sizeof(PSDRV_DEVMODEA)-sizeof(DEVMODEA),
/* dmFields */		DM_ORIENTATION | DM_PAPERSIZE | DM_SCALE |
			DM_COPIES | DM_DEFAULTSOURCE | DM_COLOR |
		        DM_YRESOLUTION | DM_TTOPTION,
   { /* u1 */
     { /* s1 */
/* dmOrientation */	DMORIENT_PORTRAIT,
/* dmPaperSize */	DMPAPER_LETTER,
/* dmPaperLength */	2794,
/* dmPaperWidth */      2159
     }
   },
/* dmScale */		100, /* ?? */
/* dmCopies */		1,
/* dmDefaultSource */	DMBIN_AUTO,
/* dmPrintQuality */	0,
/* dmColor */		DMCOLOR_COLOR,
/* dmDuplex */		DMDUP_SIMPLEX,
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

HINSTANCE PSDRV_hInstance = 0;
HANDLE PSDRV_Heap = 0;

static HFONT PSDRV_DefaultFont = 0;
static const LOGFONTA DefaultLogFont = {
    100, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0,
    DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, ""
};

/*********************************************************************
 *	     DllMain
 *
 * Initializes font metrics and registers driver. wineps dll entry point.
 *
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("(%p, %d, %p)\n", hinst, reason, reserved);

    switch(reason) {

	case DLL_PROCESS_ATTACH:
            PSDRV_hInstance = hinst;
            DisableThreadLibraryCalls(hinst);

	    PSDRV_Heap = HeapCreate(0, 0x10000, 0);
	    if (PSDRV_Heap == NULL)
		return FALSE;

	    if (PSDRV_GetFontMetrics() == FALSE) {
		HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }

	    PSDRV_DefaultFont = CreateFontIndirectA(&DefaultLogFont);
	    if (PSDRV_DefaultFont == NULL) {
		HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }
#ifdef HAVE_CUPS_CUPS_H
	    /* dynamically load CUPS if not yet loaded */
	    if (!cupshandle) {
		cupshandle = wine_dlopen(SONAME_LIBCUPS, RTLD_NOW, NULL, 0);
		if (!cupshandle) cupshandle = (void*)-1;
	    }
#endif
            break;

	case DLL_PROCESS_DETACH:

	    DeleteObject( PSDRV_DefaultFont );
	    HeapDestroy( PSDRV_Heap );
#ifdef HAVE_CUPS_CUPS_H
	    if (cupshandle && (cupshandle != (void*)-1)) {
		wine_dlclose(cupshandle, NULL, 0);
		cupshandle = NULL;
	    }
#endif
            break;
    }

    return TRUE;
}


static void PSDRV_UpdateDevCaps( PSDRV_PDEVICE *physDev )
{
    PAGESIZE *page;
    INT width = 0, height = 0;

    if(physDev->Devmode->dmPublic.dmFields & DM_PAPERSIZE) {
        LIST_FOR_EACH_ENTRY(page, &physDev->pi->ppd->PageSizes, PAGESIZE, entry) {
	    if(page->WinPage == physDev->Devmode->dmPublic.u1.s1.dmPaperSize)
	        break;
	}

	if(!page) {
	    FIXME("Can't find page\n");
	    physDev->ImageableArea.left = 0;
	    physDev->ImageableArea.right = 0;
	    physDev->ImageableArea.bottom = 0;
	    physDev->ImageableArea.top = 0;
	    physDev->PageSize.cx = 0;
	    physDev->PageSize.cy = 0;
	} else if(page->ImageableArea) {
	  /* physDev sizes in device units; ppd sizes in 1/72" */
	    physDev->ImageableArea.left = page->ImageableArea->llx *
	      physDev->logPixelsX / 72;
	    physDev->ImageableArea.right = page->ImageableArea->urx *
	      physDev->logPixelsX / 72;
	    physDev->ImageableArea.bottom = page->ImageableArea->lly *
	      physDev->logPixelsY / 72;
	    physDev->ImageableArea.top = page->ImageableArea->ury *
	      physDev->logPixelsY / 72;
	    physDev->PageSize.cx = page->PaperDimension->x *
	      physDev->logPixelsX / 72;
	    physDev->PageSize.cy = page->PaperDimension->y *
	      physDev->logPixelsY / 72;
	} else {
	    physDev->ImageableArea.left = physDev->ImageableArea.bottom = 0;
	    physDev->ImageableArea.right = physDev->PageSize.cx =
	      page->PaperDimension->x * physDev->logPixelsX / 72;
	    physDev->ImageableArea.top = physDev->PageSize.cy =
	      page->PaperDimension->y * physDev->logPixelsY / 72;
	}
    } else if((physDev->Devmode->dmPublic.dmFields & DM_PAPERLENGTH) &&
	      (physDev->Devmode->dmPublic.dmFields & DM_PAPERWIDTH)) {
      /* physDev sizes in device units; Devmode sizes in 1/10 mm */
        physDev->ImageableArea.left = physDev->ImageableArea.bottom = 0;
	physDev->ImageableArea.right = physDev->PageSize.cx =
	  physDev->Devmode->dmPublic.u1.s1.dmPaperWidth *
	  physDev->logPixelsX / 254;
	physDev->ImageableArea.top = physDev->PageSize.cy =
	  physDev->Devmode->dmPublic.u1.s1.dmPaperLength *
	  physDev->logPixelsY / 254;
    } else {
        FIXME("Odd dmFields %x\n", physDev->Devmode->dmPublic.dmFields);
	physDev->ImageableArea.left = 0;
	physDev->ImageableArea.right = 0;
	physDev->ImageableArea.bottom = 0;
	physDev->ImageableArea.top = 0;
	physDev->PageSize.cx = 0;
	physDev->PageSize.cy = 0;
    }

    TRACE("ImageableArea = %d,%d - %d,%d: PageSize = %dx%d\n",
	  physDev->ImageableArea.left, physDev->ImageableArea.bottom,
	  physDev->ImageableArea.right, physDev->ImageableArea.top,
	  physDev->PageSize.cx, physDev->PageSize.cy);

    /* these are in device units */
    width = physDev->ImageableArea.right - physDev->ImageableArea.left;
    height = physDev->ImageableArea.top - physDev->ImageableArea.bottom;

    if(physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_PORTRAIT) {
        physDev->horzRes = width;
        physDev->vertRes = height;
    } else {
        physDev->horzRes = height;
        physDev->vertRes = width;
    }

    /* these are in mm */
    physDev->horzSize = (physDev->horzRes * 25.4) / physDev->logPixelsX;
    physDev->vertSize = (physDev->vertRes * 25.4) / physDev->logPixelsY;

    TRACE("devcaps: horzSize = %dmm, vertSize = %dmm, "
	  "horzRes = %d, vertRes = %d\n",
	  physDev->horzSize, physDev->vertSize,
	  physDev->horzRes, physDev->vertRes);
}


/***********************************************************
 *      DEVMODEdupWtoA
 *
 * Creates an ascii copy of supplied devmode on heap
 *
 * Copied from dlls/winspool/info.c until full unicodification
 */
static LPDEVMODEA DEVMODEdupWtoA(HANDLE heap, const DEVMODEW *dmW)
{
    LPDEVMODEA dmA;
    DWORD size;
    BOOL Formname;
    ptrdiff_t off_formname = (const char *)dmW->dmFormName - (const char *)dmW;

    if(!dmW) return NULL;
    Formname = (dmW->dmSize > off_formname);
    size = dmW->dmSize - CCHDEVICENAME - (Formname ? CCHFORMNAME : 0);
    dmA = HeapAlloc(heap, HEAP_ZERO_MEMORY, size + dmW->dmDriverExtra);
    WideCharToMultiByte(CP_ACP, 0, dmW->dmDeviceName, -1, (LPSTR)dmA->dmDeviceName,
			CCHDEVICENAME, NULL, NULL);
    if(!Formname) {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     dmW->dmSize - CCHDEVICENAME * sizeof(WCHAR));
    } else {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     off_formname - CCHDEVICENAME * sizeof(WCHAR));
      WideCharToMultiByte(CP_ACP, 0, dmW->dmFormName, -1, (LPSTR)dmA->dmFormName,
			  CCHFORMNAME, NULL, NULL);
      memcpy(&dmA->dmLogPixels, &dmW->dmLogPixels, dmW->dmSize -
	     (off_formname + CCHFORMNAME * sizeof(WCHAR)));
    }
    dmA->dmSize = size;
    memcpy((char *)dmA + dmA->dmSize, (const char *)dmW + dmW->dmSize,
	   dmW->dmDriverExtra);
    return dmA;
}


/**********************************************************************
 *	     PSDRV_CreateDC
 */
BOOL PSDRV_CreateDC( HDC hdc, PSDRV_PDEVICE **pdev, LPCWSTR driver, LPCWSTR device,
                     LPCWSTR output, const DEVMODEW* initData )
{
    PSDRV_PDEVICE *physDev;
    PRINTERINFO *pi;
    char *deviceA;

    /* If no device name was specified, retrieve the device name
     * from the DEVMODE structure from the DC's physDev.
     * (See CreateCompatibleDC) */
    if ( !device && *pdev )
    {
        physDev = *pdev;
        deviceA = HeapAlloc(GetProcessHeap(), 0, CCHDEVICENAME);
        lstrcpynA(deviceA, (LPCSTR)physDev->Devmode->dmPublic.dmDeviceName, CCHDEVICENAME);
    }
    else
    {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, device, -1, NULL, 0, NULL, NULL);
        deviceA = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_ACP, 0, device, -1, deviceA, len, NULL, NULL);
    }
    pi = PSDRV_FindPrinterInfo(deviceA);

    HeapFree(GetProcessHeap(), 0, deviceA);
    deviceA = NULL;

    TRACE("(%s %s %s %p)\n", debugstr_w(driver), debugstr_w(device),
                             debugstr_w(output), initData);

    if(!pi) return FALSE;

    if(!pi->Fonts) {
        MESSAGE("To use WINEPS you need to install some AFM files.\n");
	return FALSE;
    }

    physDev = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(*physDev) );
    if (!physDev) return FALSE;
    *pdev = physDev;
    physDev->hdc = hdc;

    physDev->pi = pi;

    physDev->Devmode = HeapAlloc( PSDRV_Heap, 0, sizeof(PSDRV_DEVMODEA) );
    if(!physDev->Devmode) {
        HeapFree( PSDRV_Heap, 0, physDev );
	return FALSE;
    }

    memcpy( physDev->Devmode, pi->Devmode, sizeof(PSDRV_DEVMODEA) );

    physDev->logPixelsX = physDev->pi->ppd->DefaultResolution;
    physDev->logPixelsY = physDev->pi->ppd->DefaultResolution;

    if (output) {
        INT len = WideCharToMultiByte( CP_ACP, 0, output, -1, NULL, 0, NULL, NULL );
        if ((physDev->job.output = HeapAlloc( PSDRV_Heap, 0, len )))
            WideCharToMultiByte( CP_ACP, 0, output, -1, physDev->job.output, len, NULL, NULL );
    } else
        physDev->job.output = NULL;
    physDev->job.hJob = 0;

    if(initData) {
        DEVMODEA *devmodeA = DEVMODEdupWtoA(PSDRV_Heap, initData);
        PSDRV_MergeDevmodes(physDev->Devmode, (PSDRV_DEVMODEA *)devmodeA, pi);
        HeapFree(PSDRV_Heap, 0, devmodeA);
    }

    PSDRV_UpdateDevCaps(physDev);
    SelectObject( hdc, PSDRV_DefaultFont );
    return TRUE;
}



/**********************************************************************
 *	     PSDRV_DeleteDC
 */
BOOL PSDRV_DeleteDC( PSDRV_PDEVICE *physDev )
{
    TRACE("\n");

    HeapFree( PSDRV_Heap, 0, physDev->Devmode );
    HeapFree( PSDRV_Heap, 0, physDev->job.output );
    HeapFree( PSDRV_Heap, 0, physDev );

    return TRUE;
}


/**********************************************************************
 *	     ResetDC   (WINEPS.@)
 */
HDC PSDRV_ResetDC( PSDRV_PDEVICE *physDev, const DEVMODEW *lpInitData )
{
    if(lpInitData) {
        HRGN hrgn;
        DEVMODEA *devmodeA = DEVMODEdupWtoA(PSDRV_Heap, lpInitData);
        PSDRV_MergeDevmodes(physDev->Devmode, (PSDRV_DEVMODEA *)devmodeA, physDev->pi);
        HeapFree(PSDRV_Heap, 0, devmodeA);
        PSDRV_UpdateDevCaps(physDev);
        hrgn = CreateRectRgn(0, 0, physDev->horzRes, physDev->vertRes);
        SelectVisRgn16(HDC_16(physDev->hdc), HRGN_16(hrgn));
        DeleteObject(hrgn);
    }
    return physDev->hdc;
}

/***********************************************************************
 *           GetDeviceCaps    (WINEPS.@)
 */
INT PSDRV_GetDeviceCaps( PSDRV_PDEVICE *physDev, INT cap )
{
    switch(cap)
    {
    case DRIVERVERSION:
        return 0;
    case TECHNOLOGY:
        return DT_RASPRINTER;
    case HORZSIZE:
        return MulDiv(physDev->horzSize, 100,
		      physDev->Devmode->dmPublic.dmScale);
    case VERTSIZE:
        return MulDiv(physDev->vertSize, 100,
		      physDev->Devmode->dmPublic.dmScale);
    case HORZRES:
    case DESKTOPHORZRES:
        return physDev->horzRes;
    case VERTRES:
    case DESKTOPVERTRES:
        return physDev->vertRes;
    case BITSPIXEL:
        return (physDev->pi->ppd->ColorDevice != CD_False) ? 8 : 1;
    case PLANES:
        return 1;
    case NUMBRUSHES:
        return -1;
    case NUMPENS:
        return 10;
    case NUMMARKERS:
        return 0;
    case NUMFONTS:
        return 39;
    case NUMCOLORS:
        return (physDev->pi->ppd->ColorDevice != CD_False) ? 256 : -1;
    case PDEVICESIZE:
        return sizeof(PSDRV_PDEVICE);
    case CURVECAPS:
        return (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
                CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);
    case LINECAPS:
        return (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
                LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);
    case POLYGONALCAPS:
        return (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON | PC_SCANLINE |
                PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);
    case TEXTCAPS:
        return TC_CR_ANY | TC_VA_ABLE; /* psdrv 0x59f7 */
    case CLIPCAPS:
        return CP_RECTANGLE;
    case RASTERCAPS:
        return (RC_BITBLT | RC_BITMAP64 | RC_GDI20_OUTPUT | RC_DIBTODEV |
                RC_STRETCHBLT | RC_STRETCHDIB); /* psdrv 0x6e99 */
    /* Are aspect[XY] and logPixels[XY] correct? */
    /* Need to handle different res in x and y => fix ppd */
    case ASPECTX:
    case ASPECTY:
        return physDev->pi->ppd->DefaultResolution;
    case ASPECTXY:
        return (int)hypot( (double)physDev->pi->ppd->DefaultResolution,
                           (double)physDev->pi->ppd->DefaultResolution );
    case LOGPIXELSX:
        return MulDiv(physDev->logPixelsX,
		      physDev->Devmode->dmPublic.dmScale, 100);
    case LOGPIXELSY:
        return MulDiv(physDev->logPixelsY,
		      physDev->Devmode->dmPublic.dmScale, 100);
    case SIZEPALETTE:
        return 0;
    case NUMRESERVED:
        return 0;
    case COLORRES:
        return 0;
    case PHYSICALWIDTH:
        return (physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_LANDSCAPE) ?
	  physDev->PageSize.cy : physDev->PageSize.cx;
    case PHYSICALHEIGHT:
        return (physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_LANDSCAPE) ?
	  physDev->PageSize.cx : physDev->PageSize.cy;
    case PHYSICALOFFSETX:
      if(physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_LANDSCAPE) {
          if(physDev->pi->ppd->LandscapeOrientation == -90)
	      return physDev->PageSize.cy - physDev->ImageableArea.top;
	  else
	      return physDev->ImageableArea.bottom;
      }
      return physDev->ImageableArea.left;

    case PHYSICALOFFSETY:
      if(physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_LANDSCAPE) {
          if(physDev->pi->ppd->LandscapeOrientation == -90)
	      return physDev->PageSize.cx - physDev->ImageableArea.right;
	  else
	      return physDev->ImageableArea.left;
      }
      return physDev->PageSize.cy - physDev->ImageableArea.top;

    case SCALINGFACTORX:
    case SCALINGFACTORY:
    case VREFRESH:
    case BLTALIGNMENT:
        return 0;
    default:
        FIXME("(%p): unsupported capability %d, will return 0\n", physDev->hdc, cap );
        return 0;
    }
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
    const AFM *afm;
    HANDLE hPrinter;
    const char *ppd = NULL;
    DWORD ppdType;
    char* ppdFileName = NULL;
    HKEY hkey;
    BOOL using_default_devmode = FALSE;
    static CHAR paper_size[] = "Paper Size";

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

    if (!(pi->FriendlyName = HeapAlloc( PSDRV_Heap, 0, strlen(name)+1 ))) goto fail;
    strcpy( pi->FriendlyName, name );

    /* Use Get|SetPrinterDataExA instead? */

    res = DrvGetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE, &type,
			    NULL, 0, &needed );

    if(res == ERROR_INVALID_PRINTER_NAME || needed != sizeof(DefaultDevmode)) {
        pi->Devmode = HeapAlloc( PSDRV_Heap, 0, sizeof(DefaultDevmode) );
	if (pi->Devmode == NULL)
	    goto cleanup;
	memcpy(pi->Devmode, &DefaultDevmode, sizeof(DefaultDevmode) );
	lstrcpynA((LPSTR)pi->Devmode->dmPublic.dmDeviceName,name,CCHDEVICENAME);
	using_default_devmode = TRUE;

	/* need to do something here AddPrinter?? */
    }
    else {
        pi->Devmode = HeapAlloc( PSDRV_Heap, 0, needed );
	DrvGetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE, &type,
			  (LPBYTE)pi->Devmode, needed, &needed);
    }

    if (OpenPrinterA (pi->FriendlyName, &hPrinter, NULL) == 0) {
	ERR ("OpenPrinterA failed with code %i\n", GetLastError ());
	goto cleanup;
    }

#ifdef HAVE_CUPS_CUPS_H
    if (cupshandle != (void*)-1) {
	typeof(cupsGetPPD) * pcupsGetPPD = NULL;

	pcupsGetPPD = wine_dlsym(cupshandle, "cupsGetPPD", NULL, 0);
	if (pcupsGetPPD) {
	    ppd = pcupsGetPPD(name);

	    if (ppd) {
		needed=strlen(ppd)+1;
		ppdFileName=HeapAlloc(PSDRV_Heap, 0, needed);
		memcpy(ppdFileName, ppd, needed);
		ppdType=REG_SZ;
		res = ERROR_SUCCESS;
		/* we should unlink() that file later */
	    } else {
		res = ERROR_FILE_NOT_FOUND;
		WARN("Did not find ppd for %s\n",name);
	    }
	}
    }
#endif
    if (!ppdFileName) {
        static CHAR ppd_file[] = "PPD File";

        res = GetPrinterDataA(hPrinter, ppd_file, NULL, NULL, 0, &needed);
        if ((res==ERROR_SUCCESS) || (res==ERROR_MORE_DATA)) {
            ppdFileName=HeapAlloc(PSDRV_Heap, 0, needed);
            res = GetPrinterDataA(hPrinter, ppd_file, &ppdType, (LPBYTE)ppdFileName, needed, &needed);
        }
    }
    /* Look for a ppd file for this printer in the config file.
     * First look under that printer's name, and then under 'generic'
     */
    /* @@ Wine registry key: HKCU\Software\Wine\Printing\PPD Files */
    if((res != ERROR_SUCCESS) && !RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Printing\\PPD Files", &hkey))
    {
        const char* value_name;

        if (RegQueryValueExA(hkey, name, 0, NULL, NULL, &needed) == ERROR_SUCCESS) {
            value_name=name;
        } else if (RegQueryValueExA(hkey, "generic", 0, NULL, NULL, &needed) == ERROR_SUCCESS) {
            value_name="generic";
        } else {
            value_name=NULL;
        }
        if (value_name) {
            ppdFileName=HeapAlloc(PSDRV_Heap, 0, needed);
            RegQueryValueExA(hkey, value_name, 0, &ppdType, (LPBYTE)ppdFileName, &needed);
        }
        RegCloseKey(hkey);
    }

    if (!ppdFileName)
    {
        const char *data_dir, *filename;

        if ((data_dir = wine_get_data_dir())) filename = "/generic.ppd";
        else if ((data_dir = wine_get_build_dir())) filename = "/dlls/wineps.drv/generic.ppd";
        else
        {
            res = ERROR_FILE_NOT_FOUND;
            ERR ("Error %i getting PPD file name for printer '%s'\n", res, name);
            goto closeprinter;
        }
        ppdFileName = HeapAlloc( PSDRV_Heap, 0, strlen(data_dir) + strlen(filename) + 1 );
        strcpy( ppdFileName, data_dir );
        strcat( ppdFileName, filename );
    } else {
        res = ERROR_SUCCESS;
        if (ppdType==REG_EXPAND_SZ) {
            char* tmp;

            /* Expand environment variable references */
            needed=ExpandEnvironmentStringsA(ppdFileName,NULL,0);
            tmp=HeapAlloc(PSDRV_Heap, 0, needed);
            ExpandEnvironmentStringsA(ppdFileName,tmp,needed);
            HeapFree(PSDRV_Heap, 0, ppdFileName);
            ppdFileName=tmp;
        }
    }

    pi->ppd = PSDRV_ParsePPD(ppdFileName);
    if(!pi->ppd) {
	MESSAGE("Couldn't find PPD file '%s', expect a crash now!\n",
	    ppdFileName);
	goto closeprinter;
    }

    /* Some gimp-print ppd files don't contain a DefaultResolution line
       set it to 300 if it's not specified */
    if(pi->ppd->DefaultResolution == 0)
        pi->ppd->DefaultResolution = 300;

    if(using_default_devmode) {
        DWORD papersize;

	if(GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IPAPERSIZE | LOCALE_RETURN_NUMBER,
			  (LPWSTR)&papersize, sizeof(papersize)/sizeof(WCHAR))) {
	    PSDRV_DEVMODEA dm;
	    memset(&dm, 0, sizeof(dm));
	    dm.dmPublic.dmFields = DM_PAPERSIZE;
	    dm.dmPublic.u1.s1.dmPaperSize = papersize;
	    PSDRV_MergeDevmodes(pi->Devmode, &dm, pi);
	}
	DrvSetPrinterData16((LPSTR)name, (LPSTR)INT_PD_DEFAULT_DEVMODE,
		 REG_BINARY, (LPBYTE)pi->Devmode, sizeof(DefaultDevmode) );
    }

    if(pi->ppd->DefaultPageSize) { /* We'll let the ppd override the devmode */
        PSDRV_DEVMODEA dm;
        memset(&dm, 0, sizeof(dm));
        dm.dmPublic.dmFields = DM_PAPERSIZE;
        dm.dmPublic.u1.s1.dmPaperSize = pi->ppd->DefaultPageSize->WinPage;
        PSDRV_MergeDevmodes(pi->Devmode, &dm, pi);
    }

    /*
     *	This is a hack.  The default paper size should be read in as part of
     *	the Devmode structure, but Wine doesn't currently provide a convenient
     *	way to configure printers.
     */
    res = GetPrinterDataA (hPrinter, paper_size, NULL, (LPBYTE) &dwPaperSize,
	    sizeof (DWORD), &needed);
    if (res == ERROR_SUCCESS)
	pi->Devmode->dmPublic.u1.s1.dmPaperSize = (SHORT) dwPaperSize;
    else if (res == ERROR_FILE_NOT_FOUND)
	TRACE ("No 'Paper Size' for printer '%s'\n", name);
    else {
	ERR ("GetPrinterDataA returned %i\n", res);
	goto closeprinter;
    }

    /* Duplex is indicated by the setting of the DM_DUPLEX bit in dmFields.
       WinDuplex == 0 is a special case which means that the ppd has a
       *DefaultDuplex: NotCapable entry.  In this case we'll try not to confuse
       apps and set dmDuplex to DMDUP_SIMPLEX but leave the DM_DUPLEX clear.
       PSDRV_WriteHeader understands this and copes. */
    pi->Devmode->dmPublic.dmFields &= ~DM_DUPLEX;
    if(pi->ppd->DefaultDuplex) {
        pi->Devmode->dmPublic.dmDuplex = pi->ppd->DefaultDuplex->WinDuplex;
        if(pi->Devmode->dmPublic.dmDuplex != 0)
            pi->Devmode->dmPublic.dmFields |= DM_DUPLEX;
        else
            pi->Devmode->dmPublic.dmDuplex = DMDUP_SIMPLEX;
    }

    res = EnumPrinterDataExA (hPrinter, "PrinterDriverData\\FontSubTable", NULL,
	    0, &needed, &pi->FontSubTableSize);
    if (res == ERROR_SUCCESS || res == ERROR_FILE_NOT_FOUND) {
	TRACE ("No 'FontSubTable' for printer '%s'\n", name);
    }
    else if (res == ERROR_MORE_DATA) {
	pi->FontSubTable = HeapAlloc (PSDRV_Heap, 0, needed);
	if (pi->FontSubTable == NULL) {
	    ERR ("Failed to allocate %i bytes from heap\n", needed);
	    goto closeprinter;
	}

	res = EnumPrinterDataExA (hPrinter, "PrinterDriverData\\FontSubTable",
		(LPBYTE) pi->FontSubTable, needed, &needed,
		&pi->FontSubTableSize);
	if (res != ERROR_SUCCESS) {
	    ERR ("EnumPrinterDataExA returned %i\n", res);
	    goto closeprinter;
	}
    }
    else {
	ERR("EnumPrinterDataExA returned %i\n", res);
	goto closeprinter;
    }

    if (ClosePrinter (hPrinter) == 0) {
	ERR ("ClosePrinter failed with code %i\n", GetLastError ());
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
	    BOOL added;
	    if (PSDRV_AddAFMtoList(&pi->Fonts, afm, &added) == FALSE) {
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
    HeapFree(PSDRV_Heap, 0, ppdFileName);
    HeapFree(PSDRV_Heap, 0, pi->FontSubTable);
    HeapFree(PSDRV_Heap, 0, pi->FriendlyName);
    HeapFree(PSDRV_Heap, 0, pi->Devmode);
fail:
    HeapFree(PSDRV_Heap, 0, pi);
    if (ppd) unlink(ppd);
    *last = NULL;
    return NULL;
}
