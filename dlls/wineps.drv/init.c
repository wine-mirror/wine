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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winnls.h"
#include "winuser.h"
#include "psdrv.h"
#include "winspool.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static const PSDRV_DEVMODE DefaultDevmode =
{
  { /* dmPublic */
/* dmDeviceName */      L"Wine PostScript Driver",
/* dmSpecVersion */	0x30a,
/* dmDriverVersion */	0x001,
/* dmSize */		sizeof(DEVMODEW),
/* dmDriverExtra */	sizeof(PSDRV_DEVMODE)-sizeof(DEVMODEW),
/* dmFields */		DM_ORIENTATION | DM_PAPERSIZE | DM_PAPERLENGTH | DM_PAPERWIDTH |
			DM_SCALE | DM_COPIES | DM_DEFAULTSOURCE | DM_PRINTQUALITY |
			DM_COLOR | DM_DUPLEX | DM_YRESOLUTION | DM_TTOPTION |
			DM_COLLATE | DM_FORMNAME,
   { /* u1 */
     { /* s1 */
/* dmOrientation */	DMORIENT_PORTRAIT,
/* dmPaperSize */	DMPAPER_LETTER,
/* dmPaperLength */	2794,
/* dmPaperWidth */      2159,
/* dmScale */		100,
/* dmCopies */		1,
/* dmDefaultSource */	DMBIN_AUTO,
/* dmPrintQuality */	300
     }
   },
/* dmColor */		DMCOLOR_COLOR,
/* dmDuplex */		DMDUP_SIMPLEX,
/* dmYResolution */	300,
/* dmTTOption */	DMTT_SUBDEV,
/* dmCollate */		DMCOLLATE_FALSE,
/* dmFormName */        L"Letter",
/* dmLogPixels */	0,
/* dmBitsPerPel */	0,
/* dmPelsWidth */	0,
/* dmPelsHeight */	0,
   { /* u2 */
/* dmDisplayFlags */	0
   },
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

static const struct gdi_dc_funcs psdrv_funcs;

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
            break;

	case DLL_PROCESS_DETACH:
            if (reserved) break;
	    DeleteObject( PSDRV_DefaultFont );
	    HeapDestroy( PSDRV_Heap );
            break;
    }

    return TRUE;
}

static void dump_fields(DWORD fields)
{
    int add_space = 0;

#define CHECK_FIELD(flag) \
do \
{ \
    if (fields & flag) \
    { \
        if (add_space++) TRACE(" "); \
        TRACE(#flag); \
        fields &= ~flag; \
    } \
} \
while (0)

    CHECK_FIELD(DM_ORIENTATION);
    CHECK_FIELD(DM_PAPERSIZE);
    CHECK_FIELD(DM_PAPERLENGTH);
    CHECK_FIELD(DM_PAPERWIDTH);
    CHECK_FIELD(DM_SCALE);
    CHECK_FIELD(DM_POSITION);
    CHECK_FIELD(DM_NUP);
    CHECK_FIELD(DM_DISPLAYORIENTATION);
    CHECK_FIELD(DM_COPIES);
    CHECK_FIELD(DM_DEFAULTSOURCE);
    CHECK_FIELD(DM_PRINTQUALITY);
    CHECK_FIELD(DM_COLOR);
    CHECK_FIELD(DM_DUPLEX);
    CHECK_FIELD(DM_YRESOLUTION);
    CHECK_FIELD(DM_TTOPTION);
    CHECK_FIELD(DM_COLLATE);
    CHECK_FIELD(DM_FORMNAME);
    CHECK_FIELD(DM_LOGPIXELS);
    CHECK_FIELD(DM_BITSPERPEL);
    CHECK_FIELD(DM_PELSWIDTH);
    CHECK_FIELD(DM_PELSHEIGHT);
    CHECK_FIELD(DM_DISPLAYFLAGS);
    CHECK_FIELD(DM_DISPLAYFREQUENCY);
    CHECK_FIELD(DM_ICMMETHOD);
    CHECK_FIELD(DM_ICMINTENT);
    CHECK_FIELD(DM_MEDIATYPE);
    CHECK_FIELD(DM_DITHERTYPE);
    CHECK_FIELD(DM_PANNINGWIDTH);
    CHECK_FIELD(DM_PANNINGHEIGHT);
    if (fields) TRACE(" %#x", fields);
    TRACE("\n");
#undef CHECK_FIELD
}

/* Dump DEVMODE structure without a device specific part.
 * Some applications and drivers fail to specify correct field
 * flags (like DM_FORMNAME), so dump everything.
 */
static void dump_devmode(const DEVMODEW *dm)
{
    if (!TRACE_ON(psdrv)) return;

    TRACE("dmDeviceName: %s\n", debugstr_w(dm->dmDeviceName));
    TRACE("dmSpecVersion: 0x%04x\n", dm->dmSpecVersion);
    TRACE("dmDriverVersion: 0x%04x\n", dm->dmDriverVersion);
    TRACE("dmSize: 0x%04x\n", dm->dmSize);
    TRACE("dmDriverExtra: 0x%04x\n", dm->dmDriverExtra);
    TRACE("dmFields: 0x%04x\n", dm->dmFields);
    dump_fields(dm->dmFields);
    TRACE("dmOrientation: %d\n", dm->dmOrientation);
    TRACE("dmPaperSize: %d\n", dm->dmPaperSize);
    TRACE("dmPaperLength: %d\n", dm->dmPaperLength);
    TRACE("dmPaperWidth: %d\n", dm->dmPaperWidth);
    TRACE("dmScale: %d\n", dm->dmScale);
    TRACE("dmCopies: %d\n", dm->dmCopies);
    TRACE("dmDefaultSource: %d\n", dm->dmDefaultSource);
    TRACE("dmPrintQuality: %d\n", dm->dmPrintQuality);
    TRACE("dmColor: %d\n", dm->dmColor);
    TRACE("dmDuplex: %d\n", dm->dmDuplex);
    TRACE("dmYResolution: %d\n", dm->dmYResolution);
    TRACE("dmTTOption: %d\n", dm->dmTTOption);
    TRACE("dmCollate: %d\n", dm->dmCollate);
    TRACE("dmFormName: %s\n", debugstr_w(dm->dmFormName));
    TRACE("dmLogPixels %u\n", dm->dmLogPixels);
    TRACE("dmBitsPerPel %u\n", dm->dmBitsPerPel);
    TRACE("dmPelsWidth %u\n", dm->dmPelsWidth);
    TRACE("dmPelsHeight %u\n", dm->dmPelsHeight);
}

static void PSDRV_UpdateDevCaps( PSDRV_PDEVICE *physDev )
{
    PAGESIZE *page;
    RESOLUTION *res;
    INT width = 0, height = 0, resx = 0, resy = 0;

    dump_devmode(&physDev->Devmode->dmPublic);

    if (physDev->Devmode->dmPublic.dmFields & (DM_PRINTQUALITY | DM_YRESOLUTION | DM_LOGPIXELS))
    {
        if (physDev->Devmode->dmPublic.dmFields & DM_PRINTQUALITY)
            resx = resy = physDev->Devmode->dmPublic.dmPrintQuality;

        if (physDev->Devmode->dmPublic.dmFields & DM_YRESOLUTION)
            resy = physDev->Devmode->dmPublic.dmYResolution;

        if (physDev->Devmode->dmPublic.dmFields & DM_LOGPIXELS)
            resx = resy = physDev->Devmode->dmPublic.dmLogPixels;

        LIST_FOR_EACH_ENTRY(res, &physDev->pi->ppd->Resolutions, RESOLUTION, entry)
        {
            if (res->resx == resx && res->resy == resy)
            {
                physDev->logPixelsX = resx;
                physDev->logPixelsY = resy;
                break;
            }
        }

        if (&res->entry == &physDev->pi->ppd->Resolutions)
        {
            WARN("Requested resolution %dx%d is not supported by device\n", resx, resy);
            physDev->logPixelsX = physDev->pi->ppd->DefaultResolution;
            physDev->logPixelsY = physDev->logPixelsX;
        }
    }
    else
    {
        WARN("Using default device resolution %d\n", physDev->pi->ppd->DefaultResolution);
        physDev->logPixelsX = physDev->pi->ppd->DefaultResolution;
        physDev->logPixelsY = physDev->logPixelsX;
    }

    if(physDev->Devmode->dmPublic.dmFields & DM_PAPERSIZE) {
        LIST_FOR_EACH_ENTRY(page, &physDev->pi->ppd->PageSizes, PAGESIZE, entry) {
	    if(page->WinPage == physDev->Devmode->dmPublic.dmPaperSize)
	        break;
	}

	if(&page->entry == &physDev->pi->ppd->PageSizes) {
	    FIXME("Can't find page\n");
            SetRectEmpty(&physDev->ImageableArea);
	    physDev->PageSize.cx = 0;
	    physDev->PageSize.cy = 0;
	} else if(page->ImageableArea) {
	  /* physDev sizes in device units; ppd sizes in 1/72" */
            SetRect(&physDev->ImageableArea, page->ImageableArea->llx * physDev->logPixelsX / 72,
                    page->ImageableArea->ury * physDev->logPixelsY / 72,
                    page->ImageableArea->urx * physDev->logPixelsX / 72,
                    page->ImageableArea->lly * physDev->logPixelsY / 72);
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
	  physDev->Devmode->dmPublic.dmPaperWidth * physDev->logPixelsX / 254;
	physDev->ImageableArea.top = physDev->PageSize.cy =
	  physDev->Devmode->dmPublic.dmPaperLength * physDev->logPixelsY / 254;
    } else {
        FIXME("Odd dmFields %x\n", physDev->Devmode->dmPublic.dmFields);
        SetRectEmpty(&physDev->ImageableArea);
	physDev->PageSize.cx = 0;
	physDev->PageSize.cy = 0;
    }

    TRACE("ImageableArea = %s: PageSize = %dx%d\n", wine_dbgstr_rect(&physDev->ImageableArea),
	  physDev->PageSize.cx, physDev->PageSize.cy);

    /* these are in device units */
    width = physDev->ImageableArea.right - physDev->ImageableArea.left;
    height = physDev->ImageableArea.top - physDev->ImageableArea.bottom;

    if(physDev->Devmode->dmPublic.dmOrientation == DMORIENT_PORTRAIT) {
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

static PSDRV_PDEVICE *create_psdrv_physdev( PRINTERINFO *pi )
{
    PSDRV_PDEVICE *physDev;

    physDev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physDev) );
    if (!physDev) return NULL;

    physDev->Devmode = HeapAlloc( GetProcessHeap(), 0, sizeof(PSDRV_DEVMODE) );
    if (!physDev->Devmode)
    {
        HeapFree( GetProcessHeap(), 0, physDev );
	return NULL;
    }

    *physDev->Devmode = *pi->Devmode;
    physDev->pi = pi;
    physDev->logPixelsX = pi->ppd->DefaultResolution;
    physDev->logPixelsY = pi->ppd->DefaultResolution;
    return physDev;
}

/**********************************************************************
 *	     PSDRV_CreateDC
 */
static BOOL CDECL PSDRV_CreateDC( PHYSDEV *pdev, LPCWSTR device, LPCWSTR output,
                                  const DEVMODEW *initData )
{
    PSDRV_PDEVICE *physDev;
    PRINTERINFO *pi;

    TRACE("(%s %s %p)\n", debugstr_w(device), debugstr_w(output), initData);

    if (!device) return FALSE;
    pi = PSDRV_FindPrinterInfo( device );
    if(!pi) return FALSE;

    if(!pi->Fonts) {
        RASTERIZER_STATUS status;
        if(!GetRasterizerCaps(&status, sizeof(status)) ||
           !(status.wFlags & TT_AVAILABLE) ||
           !(status.wFlags & TT_ENABLED)) {
            MESSAGE("Disabling printer %s since it has no builtin fonts and there are no TrueType fonts available.\n",
                    debugstr_w(device));
            return FALSE;
        }
    }

    if (!(physDev = create_psdrv_physdev( pi ))) return FALSE;

    if (output && *output) physDev->job.output = strdupW( output );

    if(initData)
    {
        dump_devmode(initData);
        PSDRV_MergeDevmodes(physDev->Devmode, (const PSDRV_DEVMODE *)initData, pi);
    }

    PSDRV_UpdateDevCaps(physDev);
    SelectObject( (*pdev)->hdc, PSDRV_DefaultFont );
    push_dc_driver( pdev, &physDev->dev, &psdrv_funcs );
    return TRUE;
}


/**********************************************************************
 *	     PSDRV_CreateCompatibleDC
 */
static BOOL CDECL PSDRV_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    HDC hdc = (*pdev)->hdc;
    PSDRV_PDEVICE *physDev, *orig_dev = get_psdrv_dev( orig );
    PRINTERINFO *pi = PSDRV_FindPrinterInfo( orig_dev->pi->friendly_name );

    if (!pi) return FALSE;
    if (!(physDev = create_psdrv_physdev( pi ))) return FALSE;
    PSDRV_MergeDevmodes( physDev->Devmode, orig_dev->Devmode, pi );
    PSDRV_UpdateDevCaps(physDev);
    SelectObject( hdc, PSDRV_DefaultFont );
    push_dc_driver( pdev, &physDev->dev, &psdrv_funcs );
    return TRUE;
}



/**********************************************************************
 *	     PSDRV_DeleteDC
 */
static BOOL CDECL PSDRV_DeleteDC( PHYSDEV dev )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    TRACE("\n");

    HeapFree( GetProcessHeap(), 0, physDev->Devmode );
    HeapFree( GetProcessHeap(), 0, physDev->job.output );
    HeapFree( GetProcessHeap(), 0, physDev );

    return TRUE;
}


/**********************************************************************
 *	     ResetDC   (WINEPS.@)
 */
static BOOL CDECL PSDRV_ResetDC( PHYSDEV dev, const DEVMODEW *lpInitData )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    if (lpInitData)
    {
        PSDRV_MergeDevmodes(physDev->Devmode, (const PSDRV_DEVMODE *)lpInitData, physDev->pi);
        PSDRV_UpdateDevCaps(physDev);
    }
    return TRUE;
}

/***********************************************************************
 *           GetDeviceCaps    (WINEPS.@)
 */
static INT CDECL PSDRV_GetDeviceCaps( PHYSDEV dev, INT cap )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    TRACE("%p,%d\n", dev->hdc, cap);

    switch(cap)
    {
    case DRIVERVERSION:
        return 0;
    case TECHNOLOGY:
        return DT_RASPRINTER;
    case HORZSIZE:
        return MulDiv(physDev->horzSize, 100, physDev->Devmode->dmPublic.dmScale);
    case VERTSIZE:
        return MulDiv(physDev->vertSize, 100, physDev->Devmode->dmPublic.dmScale);
    case HORZRES:
        return physDev->horzRes;
    case VERTRES:
        return physDev->vertRes;
    case BITSPIXEL:
        /* Although Windows returns 1 for monochrome printers, we want
           CreateCompatibleBitmap to provide something other than 1 bpp */
        return 32;
    case NUMPENS:
        return 10;
    case NUMFONTS:
        return 39;
    case NUMCOLORS:
        return -1;
    case PDEVICESIZE:
        return sizeof(PSDRV_PDEVICE);
    case TEXTCAPS:
        return TC_CR_ANY | TC_VA_ABLE; /* psdrv 0x59f7 */
    case RASTERCAPS:
        return (RC_BITBLT | RC_BITMAP64 | RC_GDI20_OUTPUT | RC_DIBTODEV |
                RC_STRETCHBLT | RC_STRETCHDIB); /* psdrv 0x6e99 */
    case ASPECTX:
        return physDev->logPixelsX;
    case ASPECTY:
        return physDev->logPixelsY;
    case LOGPIXELSX:
        return MulDiv(physDev->logPixelsX, physDev->Devmode->dmPublic.dmScale, 100);
    case LOGPIXELSY:
        return MulDiv(physDev->logPixelsY, physDev->Devmode->dmPublic.dmScale, 100);
    case NUMRESERVED:
        return 0;
    case COLORRES:
        return 0;
    case PHYSICALWIDTH:
        return (physDev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
	  physDev->PageSize.cy : physDev->PageSize.cx;
    case PHYSICALHEIGHT:
        return (physDev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
	  physDev->PageSize.cx : physDev->PageSize.cy;
    case PHYSICALOFFSETX:
      if(physDev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) {
          if(physDev->pi->ppd->LandscapeOrientation == -90)
	      return physDev->PageSize.cy - physDev->ImageableArea.top;
	  else
	      return physDev->ImageableArea.bottom;
      }
      return physDev->ImageableArea.left;

    case PHYSICALOFFSETY:
      if(physDev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) {
          if(physDev->pi->ppd->LandscapeOrientation == -90)
	      return physDev->PageSize.cx - physDev->ImageableArea.right;
	  else
	      return physDev->ImageableArea.left;
      }
      return physDev->PageSize.cy - physDev->ImageableArea.top;

    default:
        dev = GET_NEXT_PHYSDEV( dev, pGetDeviceCaps );
        return dev->funcs->pGetDeviceCaps( dev, cap );
    }
}

static PRINTER_ENUM_VALUESA *load_font_sub_table( HANDLE printer, DWORD *num_entries )
{
    DWORD res, needed, num;
    PRINTER_ENUM_VALUESA *table = NULL;
    static const char fontsubkey[] = "PrinterDriverData\\FontSubTable";

    *num_entries = 0;

    res = EnumPrinterDataExA( printer, fontsubkey, NULL, 0, &needed, &num );
    if (res != ERROR_MORE_DATA) return NULL;

    table = HeapAlloc( PSDRV_Heap, 0, needed );
    if (!table) return NULL;

    res = EnumPrinterDataExA( printer, fontsubkey, (LPBYTE)table, needed, &needed, &num );
    if (res != ERROR_SUCCESS)
    {
        HeapFree( PSDRV_Heap, 0, table );
        return NULL;
    }

    *num_entries = num;
    return table;
}

static PSDRV_DEVMODE *get_printer_devmode( HANDLE printer )
{
    DWORD needed, dm_size;
    BOOL res;
    PRINTER_INFO_9W *info;
    PSDRV_DEVMODE *dm;

    GetPrinterW( printer, 9, NULL, 0, &needed );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return NULL;

    info = HeapAlloc( PSDRV_Heap, 0, needed );
    res = GetPrinterW( printer, 9, (BYTE *)info, needed, &needed );
    if (!res || !info->pDevMode)
    {
        HeapFree( PSDRV_Heap, 0, info );
        return NULL;
    }

    /* sanity check the sizes */
    dm_size = info->pDevMode->dmSize + info->pDevMode->dmDriverExtra;
    if ((char *)info->pDevMode - (char *)info + dm_size > needed)
    {
        HeapFree( PSDRV_Heap, 0, info );
        return NULL;
    }

    dm = (PSDRV_DEVMODE*)info;
    memmove( dm, info->pDevMode, dm_size );
    return dm;
}

static PSDRV_DEVMODE *get_devmode( HANDLE printer, const WCHAR *name, BOOL *is_default )
{
    PSDRV_DEVMODE *dm = get_printer_devmode( printer );

    *is_default = FALSE;

    if (dm && dm->dmPublic.dmSize + dm->dmPublic.dmDriverExtra >= sizeof(DefaultDevmode))
    {
        TRACE( "Retrieved devmode from winspool\n" );
        return dm;
    }
    HeapFree( PSDRV_Heap, 0, dm );

    TRACE( "Using default devmode\n" );
    dm = HeapAlloc( PSDRV_Heap, 0, sizeof(DefaultDevmode) );
    if (dm)
    {
        *dm = DefaultDevmode;
        lstrcpynW( (WCHAR *)dm->dmPublic.dmDeviceName, name, CCHDEVICENAME );
        *is_default = TRUE;
    }
    return dm;
}

static BOOL set_devmode( HANDLE printer, PSDRV_DEVMODE *dm )
{
    PRINTER_INFO_9W info;
    info.pDevMode = &dm->dmPublic;

    return SetPrinterW( printer, 9, (BYTE *)&info, 0 );
}

static WCHAR *get_ppd_filename( HANDLE printer )
{
    DWORD needed;
    DRIVER_INFO_2W *info;
    WCHAR *name;

    GetPrinterDriverW( printer, NULL, 2, NULL, 0, &needed );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return NULL;
    info = HeapAlloc( GetProcessHeap(), 0, needed );
    if (!info) return NULL;
    GetPrinterDriverW( printer, NULL, 2, (BYTE*)info, needed, &needed );
    name = (WCHAR *)info;
    memmove( name, info->pDataFile, (lstrlenW( info->pDataFile ) + 1) * sizeof(WCHAR) );
    return name;
}

static struct list printer_list = LIST_INIT( printer_list );

/**********************************************************************
 *		PSDRV_FindPrinterInfo
 */
PRINTERINFO *PSDRV_FindPrinterInfo(LPCWSTR name)
{
    PRINTERINFO *pi;
    FONTNAME *font;
    const AFM *afm;
    HANDLE hPrinter = 0;
    WCHAR *ppd_filename = NULL;
    char *nameA = NULL;
    BOOL using_default_devmode = FALSE;
    int len;

    TRACE("'%s'\n", debugstr_w(name));

    LIST_FOR_EACH_ENTRY( pi, &printer_list, PRINTERINFO, entry )
    {
        if (!wcscmp( pi->friendly_name, name ))
            return pi;
    }

    pi = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(*pi) );
    if (pi == NULL) return NULL;

    if (!(pi->friendly_name = HeapAlloc( PSDRV_Heap, 0, (lstrlenW(name)+1)*sizeof(WCHAR) ))) goto fail;
    lstrcpyW( pi->friendly_name, name );

    if (OpenPrinterW( pi->friendly_name, &hPrinter, NULL ) == 0) {
        ERR ("OpenPrinter failed with code %i\n", GetLastError ());
        goto fail;
    }

    len = WideCharToMultiByte( CP_ACP, 0, name, -1, NULL, 0, NULL, NULL );
    nameA = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, len, NULL, NULL );

    pi->Devmode = get_devmode( hPrinter, name, &using_default_devmode );
    if (!pi->Devmode) goto fail;

    ppd_filename = get_ppd_filename( hPrinter );
    if (!ppd_filename) goto fail;

    pi->ppd = PSDRV_ParsePPD( ppd_filename, hPrinter );
    if (!pi->ppd)
    {
        WARN( "Couldn't parse PPD file %s\n", debugstr_w(ppd_filename) );
        goto fail;
    }

    if(using_default_devmode) {
        DWORD papersize;

	if(GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IPAPERSIZE | LOCALE_RETURN_NUMBER,
			  (LPWSTR)&papersize, sizeof(papersize)/sizeof(WCHAR))) {
	    PSDRV_DEVMODE dm;
	    memset(&dm, 0, sizeof(dm));
	    dm.dmPublic.dmFields = DM_PAPERSIZE;
	    dm.dmPublic.dmPaperSize = papersize;
	    PSDRV_MergeDevmodes(pi->Devmode, &dm, pi);
	}
    }

    if(pi->ppd->DefaultPageSize) { /* We'll let the ppd override the devmode */
        PSDRV_DEVMODE dm;
        memset(&dm, 0, sizeof(dm));
        dm.dmPublic.dmFields = DM_PAPERSIZE;
        dm.dmPublic.dmPaperSize = pi->ppd->DefaultPageSize->WinPage;
        PSDRV_MergeDevmodes(pi->Devmode, &dm, pi);
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

    set_devmode( hPrinter, pi->Devmode );

    pi->FontSubTable = load_font_sub_table( hPrinter, &pi->FontSubTableSize );

    LIST_FOR_EACH_ENTRY( font, &pi->ppd->InstalledFonts, FONTNAME, entry )
    {
        afm = PSDRV_FindAFMinList(PSDRV_AFMFontList, font->Name);
	if(!afm) {
	    TRACE( "Couldn't find AFM file for installed printer font '%s' - "
	    	    "ignoring\n", font->Name);
	}
	else {
	    BOOL added;
	    if (PSDRV_AddAFMtoList(&pi->Fonts, afm, &added) == FALSE) {
	    	PSDRV_FreeAFMList(pi->Fonts);
		goto fail;
	    }
	}

    }
    ClosePrinter( hPrinter );
    HeapFree( GetProcessHeap(), 0, nameA );
    HeapFree( GetProcessHeap(), 0, ppd_filename );
    list_add_head( &printer_list, &pi->entry );
    return pi;

fail:
    if (hPrinter) ClosePrinter( hPrinter );
    HeapFree(PSDRV_Heap, 0, pi->FontSubTable);
    HeapFree(PSDRV_Heap, 0, pi->friendly_name);
    HeapFree(PSDRV_Heap, 0, pi->Devmode);
    HeapFree(PSDRV_Heap, 0, pi);
    HeapFree( GetProcessHeap(), 0, nameA );
    HeapFree( GetProcessHeap(), 0, ppd_filename );
    return NULL;
}


static const struct gdi_dc_funcs psdrv_funcs =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    PSDRV_Arc,                          /* pArc */
    NULL,                               /* pArcTo */
    NULL,                               /* pBeginPath */
    NULL,                               /* pBlendImage */
    PSDRV_Chord,                        /* pChord */
    NULL,                               /* pCloseFigure */
    PSDRV_CreateCompatibleDC,           /* pCreateCompatibleDC */
    PSDRV_CreateDC,                     /* pCreateDC */
    PSDRV_DeleteDC,                     /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    PSDRV_Ellipse,                      /* pEllipse */
    PSDRV_EndDoc,                       /* pEndDoc */
    PSDRV_EndPage,                      /* pEndPage */
    NULL,                               /* pEndPath */
    PSDRV_EnumFonts,                    /* pEnumFonts */
    PSDRV_ExtEscape,                    /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    PSDRV_ExtTextOut,                   /* pExtTextOut */
    PSDRV_FillPath,                     /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    PSDRV_GetCharWidth,                 /* pGetCharWidth */
    NULL,                               /* pGetCharWidthInfo */
    PSDRV_GetDeviceCaps,                /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    PSDRV_GetTextExtentExPoint,         /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    PSDRV_GetTextMetrics,               /* pGetTextMetrics */
    NULL,                               /* pGradientFill */
    NULL,                               /* pInvertRgn */
    PSDRV_LineTo,                       /* pLineTo */
    NULL,                               /* pMoveTo */
    PSDRV_PaintRgn,                     /* pPaintRgn */
    PSDRV_PatBlt,                       /* pPatBlt */
    PSDRV_Pie,                          /* pPie */
    PSDRV_PolyBezier,                   /* pPolyBezier */
    PSDRV_PolyBezierTo,                 /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    PSDRV_PolyPolygon,                  /* pPolyPolygon */
    PSDRV_PolyPolyline,                 /* pPolyPolyline */
    NULL,                               /* pPolylineTo */
    PSDRV_PutImage,                     /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    PSDRV_Rectangle,                    /* pRectangle */
    PSDRV_ResetDC,                      /* pResetDC */
    PSDRV_RoundRect,                    /* pRoundRect */
    NULL,                               /* pSelectBitmap */
    PSDRV_SelectBrush,                  /* pSelectBrush */
    PSDRV_SelectFont,                   /* pSelectFont */
    PSDRV_SelectPen,                    /* pSelectPen */
    PSDRV_SetBkColor,                   /* pSetBkColor */
    NULL,                               /* pSetBoundsRect */
    PSDRV_SetDCBrushColor,              /* pSetDCBrushColor */
    PSDRV_SetDCPenColor,                /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    NULL,                               /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    PSDRV_SetPixel,                     /* pSetPixel */
    PSDRV_SetTextColor,                 /* pSetTextColor */
    PSDRV_StartDoc,                     /* pStartDoc */
    PSDRV_StartPage,                    /* pStartPage */
    NULL,                               /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    PSDRV_StrokeAndFillPath,            /* pStrokeAndFillPath */
    PSDRV_StrokePath,                   /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pD3DKMTCheckVidPnExclusiveOwnership */
    NULL,                               /* pD3DKMTSetVidPnSourceOwner */
    NULL,                               /* wine_get_wgl_driver */
    GDI_PRIORITY_GRAPHICS_DRV           /* priority */
};


/******************************************************************************
 *      PSDRV_get_gdi_driver
 */
const struct gdi_dc_funcs * CDECL PSDRV_get_gdi_driver( unsigned int version )
{
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR( "version mismatch, gdi32 wants %u but wineps has %u\n", version, WINE_GDI_DRIVER_VERSION );
        return NULL;
    }
    return &psdrv_funcs;
}
