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
#include "ddk/winddi.h"
#include "ntf.h"
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

static BOOL import_ntf_from_reg(void)
{
    struct import_ntf_params params;
    HANDLE hfile, hmap = NULL;
    WCHAR path[MAX_PATH];
    LARGE_INTEGER size;
    char *data = NULL;
    LSTATUS status;
    HKEY hkey;
    DWORD len;
    BOOL ret;

    if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Wine\\Fonts", &hkey))
        return TRUE;
    status = RegQueryValueExW(hkey, L"NTFFile", NULL, NULL, (BYTE *)path, &len);
    RegCloseKey(hkey);
    if (status)
        return TRUE;

    hfile = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    if (hfile != INVALID_HANDLE_VALUE)
    {
        if (!GetFileSizeEx(hfile, &size))
            size.QuadPart = 0;
        hmap = CreateFileMappingW(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
        CloseHandle(hfile);
    }
    if (hmap)
    {
        data = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hmap);
    }
    if (!data)
    {
        WARN("Error loading NTF file: %s\n", debugstr_w(path));
        return TRUE;
    }

    params.data = data;
    params.size = size.QuadPart;
    ret = WINE_UNIX_CALL(unix_import_ntf, &params);
    UnmapViewOfFile(data);
    return ret;
}

#define DWORD_ALIGN(x) (((x) + 3) & ~3)
#define ntf_strsize(str) DWORD_ALIGN(strlen(str) + 1)
#define ntf_wcssize(str) DWORD_ALIGN((wcslen(str) + 1) * sizeof(WCHAR))

static BOOL convert_afm_to_ntf(void)
{
    int i, count, size, off, metrics_size;
    struct import_ntf_params params;
    struct width_range *width_range;
    struct glyph_set *glyph_set;
    struct ntf_header *header;
    struct font_mtx *font_mtx;
    struct list_entry *list;
    struct code_page *cp;
    char glyph_set_name[9];
    char *data, *new_data;
    AFMLISTENTRY *afmle;
    IFIMETRICS *metrics;
    FONTFAMILY *family;

    count = 0;
    for (family = PSDRV_AFMFontList; family; family = family->next)
    {
        for(afmle = family->afmlist; afmle; afmle = afmle->next)
            count++;
    }
    size = sizeof(*header) + sizeof(*list) * count * 2;
    data = calloc(size, 1);

    if (!data)
        return FALSE;
    header = (void *)data;
    header->glyph_set_count = count;
    header->glyph_set_off = sizeof(*header);
    header->font_mtx_count = count;
    header->font_mtx_off = header->glyph_set_off + sizeof(*list) * count;

    count = 0;
    off = size;
    for (family = PSDRV_AFMFontList; family; family = family->next)
    {
        for(afmle = family->afmlist; afmle; afmle = afmle->next)
        {
            sprintf(glyph_set_name, "%x", count);

            list = (void *)(data + header->glyph_set_off + sizeof(*list) * count);
            list->name_off = off + sizeof(*glyph_set);
            list->size = sizeof(*glyph_set) + ntf_strsize(glyph_set_name) + sizeof(*cp) +
                DWORD_ALIGN(sizeof(short) * afmle->afm->NumofMetrics);
            list->off = off;
            size += list->size;
            new_data = realloc(data, size);
            if (!new_data)
            {
                free(data);
                return FALSE;
            }
            data = new_data;
            header = (void *)data;
            memset(data + off, 0, size - off);

            glyph_set = (void *)(data + off);
            glyph_set->size = size - off;
            glyph_set->flags = 1;
            glyph_set->name_off = sizeof(*glyph_set);
            glyph_set->glyph_count = afmle->afm->NumofMetrics;
            glyph_set->cp_count = 1;
            glyph_set->cp_off = glyph_set->name_off + ntf_strsize(glyph_set_name);
            glyph_set->glyph_set_off = glyph_set->cp_off + sizeof(*cp);
            strcpy(data + off + glyph_set->name_off, glyph_set_name);
            cp = (void *)(data + off + glyph_set->cp_off);
            cp->cp = 0xffff;
            for (i = 0; i < afmle->afm->NumofMetrics; i++)
                *(WCHAR*)(data + off + glyph_set->glyph_set_off + i * sizeof(short)) = afmle->afm->Metrics[i].UV;
            off = size;

            metrics_size = sizeof(IFIMETRICS) + ntf_wcssize(afmle->afm->FamilyName);
            list = (void *)(data + header->font_mtx_off + sizeof(*list) * count);
            list->name_off = off + sizeof(*font_mtx);
            list->size = sizeof(*font_mtx) + ntf_strsize(afmle->afm->FontName) +
                ntf_strsize(glyph_set_name) + metrics_size +
                (afmle->afm->IsFixedPitch ? 0 : sizeof(*width_range) * afmle->afm->NumofMetrics);
            list->off = off;
            size += list->size;
            new_data = realloc(data, size);
            if (!new_data)
            {
                free(data);
                return FALSE;
            }
            data = new_data;
            header = (void *)data;
            memset(data + off, 0, size - off);

            font_mtx = (void *)(data + off);
            font_mtx->size = size - off;
            font_mtx->name_off = sizeof(*font_mtx);
            font_mtx->glyph_set_name_off = font_mtx->name_off + ntf_strsize(afmle->afm->FontName);
            font_mtx->glyph_count = afmle->afm->NumofMetrics;
            font_mtx->metrics_off = font_mtx->glyph_set_name_off + ntf_strsize(glyph_set_name);
            font_mtx->width_count = afmle->afm->IsFixedPitch ? 0 : afmle->afm->NumofMetrics;
            font_mtx->width_off = font_mtx->metrics_off + metrics_size;
            font_mtx->def_width = afmle->afm->Metrics[0].WX;
            strcpy(data + off + font_mtx->name_off, afmle->afm->FontName);
            strcpy(data + off + font_mtx->glyph_set_name_off, glyph_set_name);
            metrics = (void *)(data + off + font_mtx->metrics_off);
            metrics->cjThis = metrics_size;
            metrics->dpwszFamilyName = sizeof(*metrics);
            if (afmle->afm->IsFixedPitch)
                metrics->jWinPitchAndFamily |= FIXED_PITCH;
            metrics->usWinWeight = afmle->afm->Weight;
            if (afmle->afm->ItalicAngle != 0.0)
                metrics->fsSelection |= FM_SEL_ITALIC;
            if (afmle->afm->Weight == FW_BOLD)
                metrics->fsSelection |= FM_SEL_BOLD;
            metrics->fwdUnitsPerEm = afmle->afm->WinMetrics.usUnitsPerEm;
            metrics->fwdWinAscender = afmle->afm->WinMetrics.usWinAscent;
            metrics->fwdWinDescender = afmle->afm->WinMetrics.usWinDescent;
            metrics->fwdMacAscender = afmle->afm->WinMetrics.sAscender;
            metrics->fwdMacDescender = afmle->afm->WinMetrics.sDescender;
            metrics->fwdMacLineGap = afmle->afm->WinMetrics.sLineGap;
            metrics->fwdAveCharWidth = afmle->afm->WinMetrics.sAvgCharWidth;
            metrics->rclFontBox.left = afmle->afm->FontBBox.llx;
            metrics->rclFontBox.top = afmle->afm->FontBBox.ury;
            metrics->rclFontBox.right = afmle->afm->FontBBox.urx;
            metrics->rclFontBox.bottom = afmle->afm->FontBBox.lly;
            wcscpy((WCHAR *)((char *)metrics + metrics->dpwszFamilyName), afmle->afm->FamilyName);
            width_range = (void *)(data + off + font_mtx->width_off);
            for (i = 0; i < font_mtx->width_count; i++)
            {
                width_range[i].first = i;
                width_range[i].count = 1;
                width_range[i].width = afmle->afm->Metrics[i].WX;
            }
            off = size;

            count++;
        }
    }

    params.data = data;
    params.size = size;
    return WINE_UNIX_CALL(unix_import_ntf, &params);
}

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

            if (!convert_afm_to_ntf() || !import_ntf_from_reg())
            {
                WINE_UNIX_CALL(unix_free_printer_info, NULL);
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
    ctx->hdc = hdc;

    if (devmode)
    {
        dump_devmode( devmode );
        PSDRV_MergeDevmodes( ctx->Devmode, devmode, pi );
    }

    SelectObject( hdc, GetStockObject( DEVICE_DEFAULT_FONT ));
    return ctx;
}

/**********************************************************************
 *	     ResetDC
 */
BOOL PSDRV_ResetDC( print_ctx *ctx, const DEVMODEW *devmode )
{
    if (devmode)
    {
        dump_devmode( devmode );
        PSDRV_MergeDevmodes( ctx->Devmode, devmode, ctx->pi );
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

    if (dm && (dm->dmPublic.dmFields & DefaultDevmode.dmPublic.dmFields) ==
            DefaultDevmode.dmPublic.dmFields)
    {
        TRACE( "Retrieved devmode from winspool\n" );
        return dm;
    }

    TRACE( "Using default devmode\n" );
    if (!dm)
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
    int i, len, input_slots, resolutions, page_sizes, font_subs, installed_fonts, size;
    struct input_slot *dm_slot;
    struct resolution *dm_res;
    struct page_size *dm_page;
    struct font_sub *dm_sub;
    struct installed_font *dm_font;
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
    installed_fonts = list_count( &pi->ppd->InstalledFonts );
    size = FIELD_OFFSET(PSDRV_DEVMODE, data[
            input_slots * sizeof(struct input_slot) +
            resolutions * sizeof(struct resolution) +
            page_sizes * sizeof(struct page_size) +
            font_subs * sizeof(struct font_sub) +
            installed_fonts * sizeof(struct installed_font)]);

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
        pi->Devmode->font_subs = font_subs;
        pi->Devmode->installed_fonts = installed_fonts;

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

        dm_font = (struct installed_font *)dm_sub;
        LIST_FOR_EACH_ENTRY( font, &pi->ppd->InstalledFonts, FONTNAME, entry )
        {
            lstrcpynA(dm_font->name, font->Name, ARRAY_SIZE(dm_font->name));
            dm_font++;
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
 *      PSDRV_open_printer_dc
 */
HDC CDECL PSDRV_open_printer_dc( const WCHAR *device,
        const DEVMODEW *devmode, const WCHAR *output )
{
    struct open_dc_params params;
    PRINTERINFO *pi;

    if (!device)
        return 0;

    pi = PSDRV_FindPrinterInfo( device );
    if (!pi)
        return 0;

    params.device = pi->friendly_name;
    params.devmode = devmode;
    params.output = output;
    params.def_devmode = pi->Devmode;
    params.hdc = 0;
    if (!WINE_UNIX_CALL( unix_open_dc, &params ))
        return 0;
    return params.hdc;
}
