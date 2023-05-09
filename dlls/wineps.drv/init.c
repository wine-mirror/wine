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
#include "unixlib.h"
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
};

HINSTANCE PSDRV_hInstance = 0;
HANDLE PSDRV_Heap = 0;

/*********************************************************************
 *	     DllMain
 *
 * Initializes font metrics and registers driver. wineps dll entry point.
 *
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("(%p, %ld, %p)\n", hinst, reason, reserved);

    switch(reason) {

	case DLL_PROCESS_ATTACH:
        {
            PSDRV_hInstance = hinst;
            DisableThreadLibraryCalls(hinst);

            if (__wine_init_unix_call())
                return FALSE;

	    PSDRV_Heap = HeapCreate(0, 0x10000, 0);
	    if (PSDRV_Heap == NULL)
		return FALSE;

	    if (PSDRV_GetFontMetrics() == FALSE) {
		HeapDestroy(PSDRV_Heap);
		return FALSE;
	    }
            break;
        }

	case DLL_PROCESS_DETACH:
            if (reserved) break;
            WINE_UNIX_CALL(unix_free_printer_info, NULL);
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
    if (fields) TRACE(" %#lx", fields);
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
    TRACE("dmFields: 0x%04lx\n", dm->dmFields);
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
    TRACE("dmBitsPerPel %lu\n", dm->dmBitsPerPel);
    TRACE("dmPelsWidth %lu\n", dm->dmPelsWidth);
    TRACE("dmPelsHeight %lu\n", dm->dmPelsHeight);
}

static void PSDRV_UpdateDevCaps( print_ctx *ctx )
{
    PAGESIZE *page;
    RESOLUTION *res;
    INT width = 0, height = 0, resx = 0, resy = 0;

    dump_devmode(&ctx->Devmode->dmPublic);

    if (ctx->Devmode->dmPublic.dmFields & (DM_PRINTQUALITY | DM_YRESOLUTION | DM_LOGPIXELS))
    {
        if (ctx->Devmode->dmPublic.dmFields & DM_PRINTQUALITY)
            resx = resy = ctx->Devmode->dmPublic.dmPrintQuality;

        if (ctx->Devmode->dmPublic.dmFields & DM_YRESOLUTION)
            resy = ctx->Devmode->dmPublic.dmYResolution;

        if (ctx->Devmode->dmPublic.dmFields & DM_LOGPIXELS)
            resx = resy = ctx->Devmode->dmPublic.dmLogPixels;

        LIST_FOR_EACH_ENTRY(res, &ctx->pi->ppd->Resolutions, RESOLUTION, entry)
        {
            if (res->resx == resx && res->resy == resy)
            {
                ctx->logPixelsX = resx;
                ctx->logPixelsY = resy;
                break;
            }
        }

        if (&res->entry == &ctx->pi->ppd->Resolutions)
        {
            WARN("Requested resolution %dx%d is not supported by device\n", resx, resy);
            ctx->logPixelsX = ctx->pi->ppd->DefaultResolution;
            ctx->logPixelsY = ctx->logPixelsX;
        }
    }
    else
    {
        WARN("Using default device resolution %d\n", ctx->pi->ppd->DefaultResolution);
        ctx->logPixelsX = ctx->pi->ppd->DefaultResolution;
        ctx->logPixelsY = ctx->logPixelsX;
    }

    if(ctx->Devmode->dmPublic.dmFields & DM_PAPERSIZE) {
        LIST_FOR_EACH_ENTRY(page, &ctx->pi->ppd->PageSizes, PAGESIZE, entry) {
	    if(page->WinPage == ctx->Devmode->dmPublic.dmPaperSize)
	        break;
	}

	if(&page->entry == &ctx->pi->ppd->PageSizes) {
	    FIXME("Can't find page\n");
            SetRectEmpty(&ctx->ImageableArea);
	    ctx->PageSize.cx = 0;
	    ctx->PageSize.cy = 0;
	} else if(page->ImageableArea) {
	  /* ctx sizes in device units; ppd sizes in 1/72" */
            SetRect(&ctx->ImageableArea, page->ImageableArea->llx * ctx->logPixelsX / 72,
                    page->ImageableArea->ury * ctx->logPixelsY / 72,
                    page->ImageableArea->urx * ctx->logPixelsX / 72,
                    page->ImageableArea->lly * ctx->logPixelsY / 72);
	    ctx->PageSize.cx = page->PaperDimension->x *
	      ctx->logPixelsX / 72;
	    ctx->PageSize.cy = page->PaperDimension->y *
	      ctx->logPixelsY / 72;
	} else {
	    ctx->ImageableArea.left = ctx->ImageableArea.bottom = 0;
	    ctx->ImageableArea.right = ctx->PageSize.cx =
	      page->PaperDimension->x * ctx->logPixelsX / 72;
	    ctx->ImageableArea.top = ctx->PageSize.cy =
	      page->PaperDimension->y * ctx->logPixelsY / 72;
	}
    } else if((ctx->Devmode->dmPublic.dmFields & DM_PAPERLENGTH) &&
	      (ctx->Devmode->dmPublic.dmFields & DM_PAPERWIDTH)) {
      /* ctx sizes in device units; Devmode sizes in 1/10 mm */
        ctx->ImageableArea.left = ctx->ImageableArea.bottom = 0;
	ctx->ImageableArea.right = ctx->PageSize.cx =
	  ctx->Devmode->dmPublic.dmPaperWidth * ctx->logPixelsX / 254;
	ctx->ImageableArea.top = ctx->PageSize.cy =
	  ctx->Devmode->dmPublic.dmPaperLength * ctx->logPixelsY / 254;
    } else {
        FIXME("Odd dmFields %lx\n", ctx->Devmode->dmPublic.dmFields);
        SetRectEmpty(&ctx->ImageableArea);
	ctx->PageSize.cx = 0;
	ctx->PageSize.cy = 0;
    }

    TRACE("ImageableArea = %s: PageSize = %ldx%ld\n", wine_dbgstr_rect(&ctx->ImageableArea),
	  ctx->PageSize.cx, ctx->PageSize.cy);

    /* these are in device units */
    width = ctx->ImageableArea.right - ctx->ImageableArea.left;
    height = ctx->ImageableArea.top - ctx->ImageableArea.bottom;

    if(ctx->Devmode->dmPublic.dmOrientation == DMORIENT_PORTRAIT) {
        ctx->horzRes = width;
        ctx->vertRes = height;
    } else {
        ctx->horzRes = height;
        ctx->vertRes = width;
    }

    /* these are in mm */
    ctx->horzSize = (ctx->horzRes * 25.4) / ctx->logPixelsX;
    ctx->vertSize = (ctx->vertRes * 25.4) / ctx->logPixelsY;

    TRACE("devcaps: horzSize = %dmm, vertSize = %dmm, "
	  "horzRes = %d, vertRes = %d\n",
	  ctx->horzSize, ctx->vertSize,
	  ctx->horzRes, ctx->vertRes);
}

print_ctx *create_print_ctx( HDC hdc, const WCHAR *device,
                                     const DEVMODEW *devmode )
{
    PRINTERINFO *pi = PSDRV_FindPrinterInfo( device );
    print_ctx *ctx;

    if (!pi) return NULL;
    if (!pi->Fonts)
    {
        RASTERIZER_STATUS status;
        if (!GetRasterizerCaps( &status, sizeof(status) ) ||
                !(status.wFlags & TT_AVAILABLE) ||
                !(status.wFlags & TT_ENABLED))
        {
            MESSAGE( "Disabling printer %s since it has no builtin fonts and "
                    "there are no TrueType fonts available.\n", debugstr_w(device) );
            return FALSE;
        }
    }

    ctx = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ctx) );
    if (!ctx) return NULL;

    ctx->Devmode = HeapAlloc( GetProcessHeap(), 0,
            pi->Devmode->dmPublic.dmSize + pi->Devmode->dmPublic.dmDriverExtra );
    if (!ctx->Devmode)
    {
        HeapFree( GetProcessHeap(), 0, ctx );
	return NULL;
    }

    memcpy( ctx->Devmode, pi->Devmode,
            pi->Devmode->dmPublic.dmSize + pi->Devmode->dmPublic.dmDriverExtra );
    ctx->pi = pi;
    ctx->logPixelsX = pi->ppd->DefaultResolution;
    ctx->logPixelsY = pi->ppd->DefaultResolution;

    if (devmode)
    {
        dump_devmode( devmode );
        PSDRV_MergeDevmodes( ctx->Devmode, devmode, pi );
    }

    PSDRV_UpdateDevCaps( ctx );
    ctx->hdc = hdc;
    SelectObject( hdc, GetStockObject( DEVICE_DEFAULT_FONT ));
    return ctx;
}

/**********************************************************************
 *	     ResetDC   (WINEPS.@)
 */
BOOL CDECL PSDRV_ResetDC( print_ctx *ctx, const DEVMODEW *lpInitData )
{
    if (lpInitData)
    {
        PSDRV_MergeDevmodes(ctx->Devmode, lpInitData, ctx->pi);
        PSDRV_UpdateDevCaps(ctx);
    }
    return TRUE;
}

static PRINTER_ENUM_VALUESW *load_font_sub_table( HANDLE printer, DWORD *num_entries )
{
    DWORD res, needed, num;
    PRINTER_ENUM_VALUESW *table = NULL;
    static const WCHAR fontsubkey[] = L"PrinterDriverData\\FontSubTable";

    *num_entries = 0;

    res = EnumPrinterDataExW( printer, fontsubkey, NULL, 0, &needed, &num );
    if (res != ERROR_MORE_DATA) return NULL;

    table = HeapAlloc( PSDRV_Heap, 0, needed );
    if (!table) return NULL;

    res = EnumPrinterDataExW( printer, fontsubkey, (LPBYTE)table, needed, &needed, &num );
    if (res != ERROR_SUCCESS)
    {
        HeapFree( PSDRV_Heap, 0, table );
        return NULL;
    }

    *num_entries = num;
    return table;
}

static PSDRV_DEVMODE *get_printer_devmode( HANDLE printer, int size )
{
    DWORD needed, dm_size;
    BOOL res;
    PRINTER_INFO_9W *info;
    PSDRV_DEVMODE *dm;

    GetPrinterW( printer, 9, NULL, 0, &needed );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return NULL;

    info = HeapAlloc( PSDRV_Heap, 0, max(needed, size) );
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

static PSDRV_DEVMODE *get_devmode( HANDLE printer, const WCHAR *name, BOOL *is_default, int size )
{
    PSDRV_DEVMODE *dm = get_printer_devmode( printer, size );

    *is_default = FALSE;

    if (dm)
    {
        TRACE( "Retrieved devmode from winspool\n" );
        return dm;
    }

    TRACE( "Using default devmode\n" );
    dm = HeapAlloc( PSDRV_Heap, 0, size );
    if (dm)
    {
        memcpy( dm, &DefaultDevmode, min(sizeof(DefaultDevmode), size) );
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
    int i, len, input_slots, resolutions, page_sizes, font_subs, size;
    struct input_slot *dm_slot;
    struct resolution *dm_res;
    struct page_size *dm_page;
    struct font_sub *dm_sub;
    INPUTSLOT *slot;
    RESOLUTION *res;
    PAGESIZE *page;

    TRACE("%s\n", debugstr_w(name));

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
        ERR ("OpenPrinter failed with code %li\n", GetLastError ());
        goto fail;
    }

    len = WideCharToMultiByte( CP_ACP, 0, name, -1, NULL, 0, NULL, NULL );
    nameA = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, len, NULL, NULL );

    ppd_filename = get_ppd_filename( hPrinter );
    if (!ppd_filename) goto fail;

    pi->ppd = PSDRV_ParsePPD( ppd_filename, hPrinter );
    if (!pi->ppd)
    {
        WARN( "Couldn't parse PPD file %s\n", debugstr_w(ppd_filename) );
        goto fail;
    }

    pi->FontSubTable = load_font_sub_table( hPrinter, &pi->FontSubTableSize );

    input_slots = list_count( &pi->ppd->InputSlots );
    resolutions = list_count( &pi->ppd->Resolutions );
    page_sizes = list_count( &pi->ppd->PageSizes );
    font_subs = pi->FontSubTableSize;
    size = FIELD_OFFSET(PSDRV_DEVMODE, data[
            input_slots * sizeof(struct input_slot) +
            resolutions * sizeof(struct resolution) +
            page_sizes * sizeof(struct page_size) +
            font_subs * sizeof(struct font_sub)]);

    pi->Devmode = get_devmode( hPrinter, name, &using_default_devmode, size );
    if (!pi->Devmode) goto fail;

    if(using_default_devmode) {
        DWORD papersize;

	if(GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IPAPERSIZE | LOCALE_RETURN_NUMBER,
			  (LPWSTR)&papersize, sizeof(papersize)/sizeof(WCHAR))) {
	    DEVMODEW dm;
	    memset(&dm, 0, sizeof(dm));
	    dm.dmFields = DM_PAPERSIZE;
	    dm.dmPaperSize = papersize;
	    PSDRV_MergeDevmodes(pi->Devmode, &dm, pi);
	}
    }

    if(pi->ppd->DefaultPageSize) { /* We'll let the ppd override the devmode */
        DEVMODEW dm;
        memset(&dm, 0, sizeof(dm));
        dm.dmFields = DM_PAPERSIZE;
        dm.dmPaperSize = pi->ppd->DefaultPageSize->WinPage;
        PSDRV_MergeDevmodes(pi->Devmode, &dm, pi);
    }

    if (pi->Devmode->dmPublic.dmDriverExtra != size - pi->Devmode->dmPublic.dmSize)
    {
        pi->Devmode->dmPublic.dmDriverExtra = size - pi->Devmode->dmPublic.dmSize;
        pi->Devmode->default_resolution = pi->ppd->DefaultResolution;
        pi->Devmode->landscape_orientation = pi->ppd->LandscapeOrientation;
        pi->Devmode->duplex = pi->ppd->DefaultDuplex ? pi->ppd->DefaultDuplex->WinDuplex : 0;
        pi->Devmode->input_slots = input_slots;
        pi->Devmode->resolutions = resolutions;
        pi->Devmode->page_sizes = page_sizes;

        dm_slot = (struct input_slot *)pi->Devmode->data;
        LIST_FOR_EACH_ENTRY( slot, &pi->ppd->InputSlots, INPUTSLOT, entry )
        {
            dm_slot->win_bin = slot->WinBin;
            dm_slot++;
        }

        dm_res = (struct resolution *)dm_slot;
        LIST_FOR_EACH_ENTRY( res, &pi->ppd->Resolutions, RESOLUTION, entry )
        {
            dm_res->x = res->resx;
            dm_res->y = res->resy;
            dm_res++;
        }

        dm_page = (struct page_size *)dm_res;
        LIST_FOR_EACH_ENTRY( page, &pi->ppd->PageSizes, PAGESIZE, entry )
        {
            lstrcpynW(dm_page->name, page->FullName, CCHFORMNAME);
            if (page->ImageableArea)
            {
                dm_page->imageable_area.left = page->ImageableArea->llx;
                dm_page->imageable_area.bottom = page->ImageableArea->lly;
                dm_page->imageable_area.right = page->ImageableArea->urx;
                dm_page->imageable_area.top = page->ImageableArea->ury;
            }
            else
            {
                dm_page->imageable_area.left = 0;
                dm_page->imageable_area.bottom = 0;
                dm_page->imageable_area.right = page->PaperDimension->x;
                dm_page->imageable_area.top = page->PaperDimension->y;
            }
            dm_page->paper_dimension.x = page->PaperDimension->x;
            dm_page->paper_dimension.y = page->PaperDimension->y;
            dm_page->win_page = page->WinPage;
            dm_page++;
        }

        dm_sub = (struct font_sub *)dm_page;
        for (i = 0; i < font_subs; i++)
        {
            lstrcpynW(dm_sub->name, pi->FontSubTable[i].pValueName, ARRAY_SIZE(dm_sub->name));
            lstrcpynW(dm_sub->sub, (WCHAR *)pi->FontSubTable[i].pData, ARRAY_SIZE(dm_sub->sub));
            dm_sub++;
        }
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

/******************************************************************************
 *      PSDRV_get_gdi_driver
 */
const struct gdi_dc_funcs * CDECL PSDRV_get_gdi_driver( unsigned int version, const WCHAR *name )
{
    PRINTERINFO *pi = PSDRV_FindPrinterInfo( name );
    struct init_dc_params params = { NULL, pi, pi->friendly_name };

    if (!pi)
        return NULL;
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR( "version mismatch, gdi32 wants %u but wineps has %u\n", version, WINE_GDI_DRIVER_VERSION );
        return NULL;
    }
    if (!WINE_UNIX_CALL( unix_init_dc, &params ))
        return FALSE;
    return params.funcs;
}
