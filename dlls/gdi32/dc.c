/*
 * GDI Device Context functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 *           1999 Huw D M Davies
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "gdi_private.h"
#include "ntuser.h"
#include "ddrawgdi.h"
#include "winnls.h"
#include "winppi.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

static struct list drivers = LIST_INIT( drivers );

static CRITICAL_SECTION driver_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &driver_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": driver_section") }
};
static CRITICAL_SECTION driver_section = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef HDC (CDECL *driver_entry_point)( const WCHAR *device,
        const DEVMODEW *devmode, const WCHAR *output );

static const char *debugstr_xform( const XFORM *xform )
{
    if (!xform) return "(null)";
    return wine_dbg_sprintf( "matrix {%.8e %.8e} {%.8e %.8e} offset {%.8e %.8e}",
                             xform->eM11, xform->eM12, xform->eM21, xform->eM22,
                             xform->eDx, xform->eDy );
}

struct graphics_driver
{
    struct list         entry;
    HMODULE             module;  /* module handle */
    driver_entry_point  entry_point;
};

enum print_flags
{
    CALL_START_PAGE = 0x1,
    CALL_END_PAGE   = 0x2,
    WRITE_DEVMODE   = 0x4,
    BANDING         = 0x8,
};

struct print
{
    HANDLE printer;
    WCHAR *output;
    enum print_flags flags;
    DEVMODEW *devmode;
};

BOOL WINAPI SeekPrinter( HANDLE, LARGE_INTEGER, LARGE_INTEGER*, DWORD, BOOL );

enum
{
    EMRI_METAFILE = 1,
    EMRI_ENGINE_FONT,
    EMRI_DEVMODE,
    EMRI_TYPE1_FONT,
    EMRI_PRESTARTPAGE,
    EMRI_DESIGNVECTOR,
    EMRI_SUBSET_FONT,
    EMRI_DELTA_FONT,
    EMRI_FORM_METAFILE,
    EMRI_BW_METAFILE,
    EMRI_BW_FORM_METAFILE,
    EMRI_METAFILE_DATA,
    EMRI_METAFILE_EXT,
    EMRI_BW_METAFILE_EXT,
    EMRI_ENGINE_FONT_EXT,
    EMRI_TYPE1_FONT_EXT,
    EMRI_DESIGNVECTOR_EXT,
    EMRI_SUBSET_FONT_EXT,
    EMRI_DELTA_FONT_EXT,
    EMRI_PS_JOB_DATA,
    EMRI_EMBED_FONT_EXT,
    EMRI_HEADER = 65536
};

struct spool_handle
{
    HANDLE spool;

    int devmodes_no;
    int devmodes_size;
    struct
    {
        int page;
        DEVMODEW *devmode;
    } *devmodes;
};

DC_ATTR *get_dc_attr( HDC hdc )
{
    DWORD type = gdi_handle_type( hdc );
    DC_ATTR *dc_attr;
    if ((type & 0x1f0000) != NTGDI_OBJ_DC || !(dc_attr = get_gdi_client_ptr( hdc, 0 )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    return dc_attr->disabled ? NULL : dc_attr;
}

static BOOL is_display_device( const WCHAR *name )
{
    const WCHAR *p = name;

    if (!name)
        return FALSE;

    if (wcsnicmp( name, L"\\\\.\\DISPLAY", lstrlenW(L"\\\\.\\DISPLAY") )) return FALSE;

    p += lstrlenW(L"\\\\.\\DISPLAY");

    if (!iswdigit( *p++ ))
        return FALSE;

    for (; *p; p++)
    {
        if (!iswdigit( *p ))
            return FALSE;
    }

    return TRUE;
}

static BOOL get_driver_name( const WCHAR *device, WCHAR *driver, DWORD size )
{
    WCHAR *p;

    /* display is a special case */
    if (!wcsicmp( device, L"display" ) || is_display_device( device ))
    {
        lstrcpynW( driver, L"display", size );
        return TRUE;
    }

    size = GetProfileStringW(L"devices", device, L"", driver, size);
    if (!size)
    {
        WARN("Unable to find %s in [devices] section of win.ini\n", debugstr_w(device));
        return FALSE;
    }
    p = wcschr(driver, ',');
    if (!p)
    {
        WARN("%s entry in [devices] section of win.ini is malformed.\n", debugstr_w(device));
        return FALSE;
    }
    *p = 0;
    TRACE("Found %s for %s\n", debugstr_w(driver), debugstr_w(device));
    return TRUE;
}

static struct graphics_driver *create_driver( HMODULE module )
{
    struct graphics_driver *driver;

    if (!(driver = HeapAlloc( GetProcessHeap(), 0, sizeof(*driver)))) return NULL;
    driver->module = module;

    if (module)
        driver->entry_point = (void *)GetProcAddress( module, "wine_driver_open_dc" );
    else
        driver->entry_point = NULL;

    return driver;
}

#ifdef __i386__
static const WCHAR printer_env[] = L"w32x86";
#elif defined __x86_64__
static const WCHAR printer_env[] = L"x64";
#elif defined __arm__
static const WCHAR printer_env[] = L"arm";
#elif defined __aarch64__
static const WCHAR printer_env[] = L"arm64";
#else
#error not defined for this cpu
#endif

static driver_entry_point load_driver( LPCWSTR name )
{
    HMODULE module;
    struct graphics_driver *driver, *new_driver;

    if ((module = GetModuleHandleW( name )))
    {
        EnterCriticalSection( &driver_section );
        LIST_FOR_EACH_ENTRY( driver, &drivers, struct graphics_driver, entry )
        {
            if (driver->module == module) goto done;
        }
        LeaveCriticalSection( &driver_section );
    }

    if (!(module = LoadLibraryW( name )))
    {
        WCHAR path[MAX_PATH];

        GetSystemDirectoryW( path, MAX_PATH );
        swprintf( path + wcslen(path), MAX_PATH - wcslen(path), L"\\spool\\drivers\\%s\\3\\%s",
                  printer_env, name );
        if (!(module = LoadLibraryW( path ))) return NULL;
    }

    if (!(new_driver = create_driver( module )))
    {
        FreeLibrary( module );
        return NULL;
    }

    /* check if someone else added it in the meantime */
    EnterCriticalSection( &driver_section );
    LIST_FOR_EACH_ENTRY( driver, &drivers, struct graphics_driver, entry )
    {
        if (driver->module != module) continue;
        FreeLibrary( module );
        HeapFree( GetProcessHeap(), 0, new_driver );
        goto done;
    }
    driver = new_driver;
    list_add_head( &drivers, &driver->entry );
    TRACE( "loaded driver %p for %s\n", driver, debugstr_w(name) );
done:
    LeaveCriticalSection( &driver_section );
    return driver->entry_point;
}

static BOOL print_copy_devmode( struct print *print, const DEVMODEW *devmode )
{
    size_t size;

    if (!print) return TRUE;
    if (!print->devmode && !devmode) return TRUE;
    HeapFree( GetProcessHeap(), 0, print->devmode );

    if (!devmode)
    {
        print->devmode = NULL;
        print->flags |= WRITE_DEVMODE;
        return TRUE;
    }

    size = devmode->dmSize + devmode->dmDriverExtra;
    print->devmode = HeapAlloc( GetProcessHeap(), 0, size );
    if (!print->devmode) return FALSE;
    memcpy(print->devmode, devmode, size);
    print->flags |= WRITE_DEVMODE;
    return TRUE;
}

/***********************************************************************
 *           CreateDCW    (GDI32.@)
 */
HDC WINAPI CreateDCW( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                      const DEVMODEW *devmode )
{
    PRINTER_DEFAULTSW prn_defaults =
    {
        .pDatatype = NULL,
        .pDevMode = (DEVMODEW *)devmode,
        .DesiredAccess = PRINTER_ACCESS_USE
    };
    UNICODE_STRING device_str, output_str;
    driver_entry_point entry_point = NULL;
    const WCHAR *display = NULL, *p;
    WCHAR buf[300], *port = NULL;
    struct print *print = NULL;
    BOOL is_display = FALSE;
    HANDLE hspool = NULL;
    DC_ATTR *dc_attr;
    HDC ret;

    if (!device || !get_driver_name( device, buf, 300 ))
    {
        if (!driver)
        {
            ERR( "no device found for %s\n", debugstr_w(device) );
            return 0;
        }
        lstrcpyW(buf, driver);
    }

    if (output)
    {
        output_str.Length = output_str.MaximumLength = lstrlenW(output) * sizeof(WCHAR);
        output_str.Buffer = (WCHAR *)output;
    }

    if (is_display_device( driver ))
    {
        display = driver;
        is_display = TRUE;
    }
    else if (is_display_device( device ))
    {
        display = device;
        is_display = TRUE;
    }
    else if (!wcsicmp( buf, L"display" ) || is_display_device( buf ))
    {
        is_display = TRUE;
    }
    else if (!(entry_point = load_driver( buf )))
    {
        ERR( "no driver found for %s\n", debugstr_w(buf) );
        return 0;
    }
    else if (!OpenPrinterW( (WCHAR *)device, &hspool, &prn_defaults ))
    {
        return 0;
    }
    else if (output && !(port = HeapAlloc( GetProcessHeap(), 0, output_str.Length + sizeof(WCHAR) )))
    {
        ClosePrinter( hspool );
        return 0;
    }
    else if (!(print = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*print) )) ||
            !print_copy_devmode( print, devmode ))
    {
        HeapFree( GetProcessHeap(), 0, print );
        ClosePrinter( hspool );
        HeapFree( GetProcessHeap(), 0, port );
        return 0;
    }

    if (display)
    {
        /* Use only the display name. For example, \\.\DISPLAY1 in \\.\DISPLAY1\Monitor0 */
        p = display + 12;
        while (iswdigit( *p )) p++;

        device_str.Length = device_str.MaximumLength = (p - display) * sizeof(WCHAR);
        device_str.Buffer = (WCHAR *)display;
    }
    else if (device)
    {
        device_str.Length = device_str.MaximumLength = lstrlenW( device ) * sizeof(WCHAR);
        device_str.Buffer = (WCHAR *)device;
    }

    if (entry_point)
        ret = entry_point( device, devmode, output );
    else
        ret = NtGdiOpenDCW( device || display ? &device_str : NULL, devmode,
                            output ? &output_str : NULL,
                            0, is_display, entry_point, NULL, NULL );

    if (ret && hspool && (dc_attr = get_dc_attr( ret )))
    {
        if (port)
        {
            memcpy( port, output, output_str.Length );
            port[output_str.Length / sizeof(WCHAR)] = 0;
        }
        print->printer = hspool;
        print->output = port;
        dc_attr->print = (UINT_PTR)print;
    }
    else if (hspool)
    {
        ClosePrinter( hspool );
        HeapFree( GetProcessHeap(), 0, port );
        HeapFree( GetProcessHeap(), 0, print->devmode );
        HeapFree( GetProcessHeap(), 0, print );
    }

    return ret;
}

/***********************************************************************
 *           CreateDCA  (GDI32.@)
 */
HDC WINAPI CreateDCA( const char *driver, const char *device, const char *output,
                      const DEVMODEA *init_data )
{
    UNICODE_STRING driverW, deviceW, outputW;
    DEVMODEW *init_dataW = NULL;
    HDC ret;

    if (driver) RtlCreateUnicodeStringFromAsciiz( &driverW, driver );
    else driverW.Buffer = NULL;

    if (device) RtlCreateUnicodeStringFromAsciiz( &deviceW, device );
    else deviceW.Buffer = NULL;

    if (output) RtlCreateUnicodeStringFromAsciiz( &outputW, output );
    else outputW.Buffer = NULL;

    if (init_data)
    {
        /* don't convert init_data for DISPLAY driver, it's not used */
        if (!driverW.Buffer || wcsicmp( driverW.Buffer, L"display" ))
            init_dataW = GdiConvertToDevmodeW( init_data );
    }

    ret = CreateDCW( driverW.Buffer, deviceW.Buffer, outputW.Buffer, init_dataW );

    RtlFreeUnicodeString( &driverW );
    RtlFreeUnicodeString( &deviceW );
    RtlFreeUnicodeString( &outputW );
    HeapFree( GetProcessHeap(), 0, init_dataW );
    return ret;
}

/***********************************************************************
 *           CreateICA    (GDI32.@)
 */
HDC WINAPI CreateICA( const char *driver, const char *device, const char *output,
                      const DEVMODEA *init_data )
{
    /* Nothing special yet for ICs */
    return CreateDCA( driver, device, output, init_data );
}


/***********************************************************************
 *           CreateICW    (GDI32.@)
 */
HDC WINAPI CreateICW( const WCHAR *driver, const WCHAR *device, const WCHAR *output,
                      const DEVMODEW *init_data )
{
    /* Nothing special yet for ICs */
    return CreateDCW( driver, device, output, init_data );
}

/***********************************************************************
 *           GdiConvertToDevmodeW    (GDI32.@)
 */
DEVMODEW *WINAPI GdiConvertToDevmodeW( const DEVMODEA *dmA )
{
    DEVMODEW *dmW;
    WORD dmW_size, dmA_size;

    dmA_size = dmA->dmSize;

    /* this is the minimal dmSize that XP accepts */
    if (dmA_size < FIELD_OFFSET(DEVMODEA, dmFields))
        return NULL;

    if (dmA_size > sizeof(DEVMODEA))
        dmA_size = sizeof(DEVMODEA);

    dmW_size = dmA_size + CCHDEVICENAME;
    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
        dmW_size += CCHFORMNAME;

    dmW = HeapAlloc( GetProcessHeap(), 0, dmW_size + dmA->dmDriverExtra );
    if (!dmW) return NULL;

    MultiByteToWideChar( CP_ACP, 0, (const char*) dmA->dmDeviceName, -1,
                         dmW->dmDeviceName, CCHDEVICENAME );
    /* copy slightly more, to avoid long computations */
    memcpy( &dmW->dmSpecVersion, &dmA->dmSpecVersion, dmA_size - CCHDEVICENAME );

    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
    {
        if (dmA->dmFields & DM_FORMNAME)
            MultiByteToWideChar( CP_ACP, 0, (const char*) dmA->dmFormName, -1,
                                 dmW->dmFormName, CCHFORMNAME );
        else
            dmW->dmFormName[0] = 0;

        if (dmA_size > FIELD_OFFSET(DEVMODEA, dmLogPixels))
            memcpy( &dmW->dmLogPixels, &dmA->dmLogPixels, dmA_size - FIELD_OFFSET(DEVMODEA, dmLogPixels) );
    }

    if (dmA->dmDriverExtra)
        memcpy( (char *)dmW + dmW_size, (const char *)dmA + dmA_size, dmA->dmDriverExtra );

    dmW->dmSize = dmW_size;

    return dmW;
}

static inline struct print *get_dc_print( DC_ATTR *dc_attr )
{
    return (struct print *)(UINT_PTR)dc_attr->print;
}

void print_call_start_page( DC_ATTR *dc_attr )
{
    struct print *print = get_dc_print( dc_attr );

    if (print->flags & CALL_START_PAGE) StartPage( UlongToHandle(dc_attr->hdc) );
}

static void delete_print_dc( DC_ATTR *dc_attr )
{
    struct print *print = get_dc_print( dc_attr );

    ClosePrinter( print->printer );
    HeapFree( GetProcessHeap(), 0, print->output );
    HeapFree( GetProcessHeap(), 0, print->devmode );
    HeapFree( GetProcessHeap(), 0, print );
    dc_attr->print = 0;
}

/***********************************************************************
 *           DeleteDC    (GDI32.@)
 */
BOOL WINAPI DeleteDC( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_DeleteDC( hdc );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print)
    {
        if (dc_attr->emf)
            AbortDoc( hdc );
        delete_print_dc( dc_attr );
    }
    if (dc_attr->emf) EMFDC_DeleteDC( dc_attr );
    return NtGdiDeleteObjectApp( hdc );
}

/***********************************************************************
 *           ResetDCA    (GDI32.@)
 */
HDC WINAPI ResetDCA( HDC hdc, const DEVMODEA *devmode )
{
    DEVMODEW *devmodeW;
    HDC ret;

    if (devmode) devmodeW = GdiConvertToDevmodeW( devmode );
    else devmodeW = NULL;

    ret = ResetDCW( hdc, devmodeW );

    HeapFree( GetProcessHeap(), 0, devmodeW );
    return ret;
}

/***********************************************************************
 *           ResetDCW    (GDI32.@)
 */
HDC WINAPI ResetDCW( HDC hdc, const DEVMODEW *devmode )
{
    struct print *print;
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    print = get_dc_print( dc_attr );
    if (print && print->flags & CALL_END_PAGE) return 0;
    if (!NtGdiResetDC( hdc, devmode, NULL, NULL, NULL )) return 0;
    if (print)
    {
        PRINTER_DEFAULTSW prn_defaults =
        {
            .pDatatype = NULL,
            .pDevMode = (DEVMODEW *)devmode,
            .DesiredAccess = PRINTER_ACCESS_USE
        };

        if (!print_copy_devmode( print, devmode )) return 0;
        ResetPrinterW( print->printer, &prn_defaults );
    }
    return hdc;
}

/***********************************************************************
 *           SaveDC    (GDI32.@)
 */
INT WINAPI SaveDC( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SaveDC( hdc );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SaveDC( dc_attr )) return FALSE;
    return NtGdiSaveDC( hdc );
}

/***********************************************************************
 *           RestoreDC    (GDI32.@)
 */
BOOL WINAPI RestoreDC( HDC hdc, INT level )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_RestoreDC( hdc, level );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_RestoreDC( dc_attr, level )) return FALSE;
    return NtGdiRestoreDC( hdc, level );
}

/***********************************************************************
 *           GetDeviceCaps    (GDI32.@)
 */
INT WINAPI GetDeviceCaps( HDC hdc, INT cap )
{
    if (is_meta_dc( hdc )) return METADC_GetDeviceCaps( hdc, cap );
    if (!get_dc_attr( hdc )) return FALSE;
    return NtGdiGetDeviceCaps( hdc, cap );
}

/***********************************************************************
 *             Escape  (GDI32.@)
 */
INT WINAPI Escape( HDC hdc, INT escape, INT in_count, const char *in_data, void *out_data )
{
    INT ret;
    POINT *pt;

    switch (escape)
    {
    case ABORTDOC:
        return AbortDoc( hdc );

    case NEXTBAND:
    {
        RECT *rect = out_data;
        struct print *print;
        DC_ATTR *dc_attr;

        if (!(dc_attr = get_dc_attr( hdc )) || !(print = get_dc_print( dc_attr ))) break;
        if (print->flags & BANDING)
        {
            print->flags &= ~BANDING;
            SetRectEmpty( rect );
            return EndPage( hdc );
        }
        print->flags |= BANDING;
        SetRect( rect, 0, 0, GetDeviceCaps(hdc, HORZRES), GetDeviceCaps(hdc, VERTRES) );
        return 1;
    }

    case ENDDOC:
        return EndDoc( hdc );

    case GETPHYSPAGESIZE:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALWIDTH );
        pt->y = GetDeviceCaps( hdc, PHYSICALHEIGHT );
        return 1;

    case GETPRINTINGOFFSET:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALOFFSETX );
        pt->y = GetDeviceCaps( hdc, PHYSICALOFFSETY );
        return 1;

    case GETSCALINGFACTOR:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, SCALINGFACTORX );
        pt->y = GetDeviceCaps( hdc, SCALINGFACTORY );
        return 1;

    case NEWFRAME:
        return EndPage( hdc );

    case SETABORTPROC:
        return SetAbortProc( hdc, (ABORTPROC)in_data );

    case STARTDOC:
        {
            DOCINFOA doc;
            char *name = NULL;

            /* in_data may not be 0 terminated so we must copy it */
            if (in_data)
            {
                name = HeapAlloc( GetProcessHeap(), 0, in_count+1 );
                memcpy( name, in_data, in_count );
                name[in_count] = 0;
            }
            /* out_data is actually a pointer to the DocInfo structure and used as
             * a second input parameter */
            if (out_data) doc = *(DOCINFOA *)out_data;
            else
            {
                doc.cbSize = sizeof(doc);
                doc.lpszOutput = NULL;
                doc.lpszDatatype = NULL;
                doc.fwType = 0;
            }
            doc.lpszDocName = name;
            ret = StartDocA( hdc, &doc );
            HeapFree( GetProcessHeap(), 0, name );
            if (ret > 0) ret = StartPage( hdc );
            return ret;
        }

    case QUERYESCSUPPORT:
        {
            DWORD code;

            if (in_count < sizeof(SHORT)) return 0;
            code = (in_count < sizeof(DWORD)) ? *(const USHORT *)in_data : *(const DWORD *)in_data;
            switch (code)
            {
            case ABORTDOC:
            case ENDDOC:
            case GETPHYSPAGESIZE:
            case GETPRINTINGOFFSET:
            case GETSCALINGFACTOR:
            case NEWFRAME:
            case QUERYESCSUPPORT:
            case SETABORTPROC:
            case STARTDOC:
                return TRUE;
            }
            break;
        }

    case PASSTHROUGH:
    case POSTSCRIPT_PASSTHROUGH:
        in_count = *(const WORD *)in_data + sizeof(WORD);
        out_data = NULL;
        break;
    }

    /* if not handled internally, pass it to the driver */
    return ExtEscape( hdc, escape, in_count, in_data, 0, out_data );
}

/***********************************************************************
 *		ExtEscape	[GDI32.@]
 */
INT WINAPI ExtEscape( HDC hdc, INT escape, INT input_size, const char *input,
                      INT output_size, char *output )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc ))
        return METADC_ExtEscape( hdc, escape, input_size, input, output_size, output );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->print)
    {
        switch (escape)
        {
        case PASSTHROUGH:
        case POSTSCRIPT_DATA:
        case GETFACENAME:
        case DOWNLOADFACE:
        case BEGIN_PATH:
        case CLIP_TO_PATH:
        case END_PATH:
        case DOWNLOADHEADER:
            print_call_start_page( dc_attr );
        }

        if (dc_attr->emf)
        {
            int ret = EMFDC_ExtEscape( dc_attr, escape, input_size, input, output_size, output );
            if (ret) return ret;
        }
    }
    return NtGdiExtEscape( hdc, NULL, 0, escape, input_size, input, output_size, output );
}

/***********************************************************************
 *		GetTextAlign (GDI32.@)
 */
UINT WINAPI GetTextAlign( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->text_align : 0;
}

/***********************************************************************
 *           SetTextAlign    (GDI32.@)
 */
UINT WINAPI SetTextAlign( HDC hdc, UINT align )
{
    DC_ATTR *dc_attr;
    UINT ret;

    TRACE("hdc=%p align=%d\n", hdc, align);

    if (is_meta_dc( hdc )) return METADC_SetTextAlign( hdc, align );
    if (!(dc_attr = get_dc_attr( hdc ))) return GDI_ERROR;
    if (dc_attr->emf && !EMFDC_SetTextAlign( dc_attr, align )) return GDI_ERROR;

    ret = dc_attr->text_align;
    dc_attr->text_align = align;
    return ret;
}

/***********************************************************************
 *		GetBkColor (GDI32.@)
 */
COLORREF WINAPI GetBkColor( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->background_color : CLR_INVALID;
}

/***********************************************************************
 *           SetBkColor    (GDI32.@)
 */
COLORREF WINAPI SetBkColor( HDC hdc, COLORREF color )
{
    DC_ATTR *dc_attr;
    COLORREF ret;

    if (is_meta_dc( hdc )) return METADC_SetBkColor( hdc, color );
    if (!(dc_attr = get_dc_attr( hdc ))) return CLR_INVALID;
    if (dc_attr->emf && !EMFDC_SetBkColor( dc_attr, color )) return CLR_INVALID;
    return NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, color, &ret ) ? ret : CLR_INVALID;
}

/***********************************************************************
 *           GetDCBrushColor  (GDI32.@)
 */
COLORREF WINAPI GetDCBrushColor( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->brush_color : CLR_INVALID;
}

/***********************************************************************
 *           SetDCBrushColor    (GDI32.@)
 */
COLORREF WINAPI SetDCBrushColor( HDC hdc, COLORREF color )
{
    DC_ATTR *dc_attr;
    COLORREF ret;

    if (!(dc_attr = get_dc_attr( hdc ))) return CLR_INVALID;
    if (dc_attr->emf && !EMFDC_SetDCBrushColor( dc_attr, color )) return CLR_INVALID;
    return NtGdiGetAndSetDCDword( hdc, NtGdiSetDCBrushColor, color, &ret ) ? ret : CLR_INVALID;
}

/***********************************************************************
 *           GetDCPenColor    (GDI32.@)
 */
COLORREF WINAPI GetDCPenColor(HDC hdc)
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->pen_color : CLR_INVALID;
}

/***********************************************************************
 *           SetDCPenColor    (GDI32.@)
 */
COLORREF WINAPI SetDCPenColor( HDC hdc, COLORREF color )
{
    DC_ATTR *dc_attr;
    COLORREF ret;

    if (!(dc_attr = get_dc_attr( hdc ))) return CLR_INVALID;
    if (dc_attr->emf && !EMFDC_SetDCPenColor( dc_attr, color )) return CLR_INVALID;
    return NtGdiGetAndSetDCDword( hdc, NtGdiSetDCPenColor, color, &ret ) ? ret : CLR_INVALID;
}

/***********************************************************************
 *		GetTextColor (GDI32.@)
 */
COLORREF WINAPI GetTextColor( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->text_color : 0;
}

/***********************************************************************
 *           SetTextColor    (GDI32.@)
 */
COLORREF WINAPI SetTextColor( HDC hdc, COLORREF color )
{
    DC_ATTR *dc_attr;
    COLORREF ret;

    if (is_meta_dc( hdc )) return METADC_SetTextColor( hdc, color );
    if (!(dc_attr = get_dc_attr( hdc ))) return CLR_INVALID;
    if (dc_attr->emf && !EMFDC_SetTextColor( dc_attr, color )) return CLR_INVALID;
    return NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, color, &ret ) ? ret : CLR_INVALID;
}

/***********************************************************************
 *		GetBkMode (GDI32.@)
 */
INT WINAPI GetBkMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->background_mode : 0;
}

/***********************************************************************
 *		SetBkMode (GDI32.@)
 */
INT WINAPI SetBkMode( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode <= 0 || mode > BKMODE_LAST)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetBkMode( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetBkMode( dc_attr, mode )) return 0;

    ret = dc_attr->background_mode;
    dc_attr->background_mode = mode;
    return ret;
}

/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
INT WINAPI GetGraphicsMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->graphics_mode : 0;
}

/***********************************************************************
 *           SetGraphicsMode    (GDI32.@)
 */
INT WINAPI SetGraphicsMode( HDC hdc, INT mode )
{
    DWORD ret;

    TRACE( "dc %p mode %#x\n", hdc, mode );

    return NtGdiGetAndSetDCDword( hdc, NtGdiSetGraphicsMode, mode, &ret ) ? ret : 0;
}

/***********************************************************************
 *		GetArcDirection (GDI32.@)
 */
INT WINAPI GetArcDirection( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->arc_direction : 0;
}

/***********************************************************************
 *           SetArcDirection    (GDI32.@)
 */
INT WINAPI SetArcDirection( HDC hdc, INT dir )
{
    DC_ATTR *dc_attr;
    INT ret;

    TRACE( "dc %p dir %#x\n", hdc, dir );

    if (dir != AD_COUNTERCLOCKWISE && dir != AD_CLOCKWISE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetArcDirection( dc_attr, dir )) return 0;

    ret = dc_attr->arc_direction;
    dc_attr->arc_direction = dir;
    return ret;
}

/***********************************************************************
 *           GetLayout    (GDI32.@)
 */
DWORD WINAPI GetLayout( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->layout : GDI_ERROR;
}

/***********************************************************************
 *           SetLayout    (GDI32.@)
 */
DWORD WINAPI SetLayout( HDC hdc, DWORD layout )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetLayout( hdc, layout );
    if (!(dc_attr = get_dc_attr( hdc ))) return GDI_ERROR;
    if (dc_attr->emf && !EMFDC_SetLayout( dc_attr, layout )) return GDI_ERROR;
    return NtGdiSetLayout( hdc, -1, layout );
}

/***********************************************************************
 *           GetMapMode  (GDI32.@)
 */
INT WINAPI GetMapMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->map_mode : 0;
}

/***********************************************************************
 *           SetMapMode    (GDI32.@)
 */
INT WINAPI SetMapMode( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    DWORD ret;

    TRACE("%p %d\n", hdc, mode );

    if (is_meta_dc( hdc )) return METADC_SetMapMode( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetMapMode( dc_attr, mode )) return 0;
    return NtGdiGetAndSetDCDword( hdc, NtGdiSetMapMode, mode, &ret ) ? ret : 0;
}

/***********************************************************************
 *           GetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI GetTextCharacterExtra( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->char_extra : 0x80000000;
}

/***********************************************************************
 *           SetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI SetTextCharacterExtra( HDC hdc, INT extra )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (is_meta_dc( hdc )) return METADC_SetTextCharacterExtra( hdc, extra );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0x8000000;
    ret = dc_attr->char_extra;
    dc_attr->char_extra = extra;
    return ret;
}

/***********************************************************************
 *           SetMapperFlags    (GDI32.@)
 */
DWORD WINAPI SetMapperFlags( HDC hdc, DWORD flags )
{
    DC_ATTR *dc_attr;
    DWORD ret;

    if (is_meta_dc( hdc )) return METADC_SetMapperFlags( hdc, flags );
    if (!(dc_attr = get_dc_attr( hdc ))) return GDI_ERROR;
    if (dc_attr->emf && !EMFDC_SetMapperFlags( dc_attr, flags )) return GDI_ERROR;
    ret = dc_attr->mapper_flags;
    dc_attr->mapper_flags = flags;
    return ret;
}

/***********************************************************************
 *		GetPolyFillMode  (GDI32.@)
 */
INT WINAPI GetPolyFillMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->poly_fill_mode : 0;
}

/***********************************************************************
 *		SetPolyFillMode (GDI32.@)
 */
INT WINAPI SetPolyFillMode( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode <= 0 || mode > POLYFILL_LAST)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetPolyFillMode( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetPolyFillMode( dc_attr, mode )) return 0;

    ret = dc_attr->poly_fill_mode;
    dc_attr->poly_fill_mode = mode;
    return ret;
}

/***********************************************************************
 *		GetStretchBltMode (GDI32.@)
 */
INT WINAPI GetStretchBltMode( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->stretch_blt_mode : 0;
}

/***********************************************************************
 *		GetBrushOrgEx (GDI32.@)
 */
BOOL WINAPI GetBrushOrgEx( HDC hdc, POINT *point )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    *point = dc_attr->brush_org;
    return TRUE;
}

/***********************************************************************
 *           SetBrushOrgEx    (GDI32.@)
 */
BOOL WINAPI SetBrushOrgEx( HDC hdc, INT x, INT y, POINT *oldorg )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetBrushOrgEx( dc_attr, x, y )) return FALSE;
    if (oldorg) *oldorg = dc_attr->brush_org;
    dc_attr->brush_org.x = x;
    dc_attr->brush_org.y = y;
    return TRUE;
}

/***********************************************************************
 *           FixBrushOrgEx    (GDI32.@)
 */
BOOL WINAPI FixBrushOrgEx( HDC hdc, INT x, INT y, POINT *oldorg )
{
    return SetBrushOrgEx( hdc, x, y, oldorg );
}

/***********************************************************************
 *           GetDCOrgEx  (GDI32.@)
 */
BOOL WINAPI GetDCOrgEx( HDC hdc, POINT *point )
{
    DC_ATTR *dc_attr;
    if (!point || !(dc_attr = get_dc_attr( hdc ))) return FALSE;
    point->x = dc_attr->vis_rect.left;
    point->y = dc_attr->vis_rect.top;
    return TRUE;
}

/***********************************************************************
 *		GetWindowExtEx (GDI32.@)
 */
BOOL WINAPI GetWindowExtEx( HDC hdc, SIZE *size )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    *size = dc_attr->wnd_ext;
    return TRUE;
}

/***********************************************************************
 *           SetWindowExtEx    (GDI32.@)
 */
BOOL WINAPI SetWindowExtEx( HDC hdc, INT x, INT y, SIZE *size )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetWindowExtEx( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetWindowExtEx( dc_attr, x, y )) return FALSE;

    if (size) *size = dc_attr->wnd_ext;
    if (dc_attr->map_mode != MM_ISOTROPIC && dc_attr->map_mode != MM_ANISOTROPIC) return TRUE;
    if (!x || !y) return FALSE;
    dc_attr->wnd_ext.cx = x;
    dc_attr->wnd_ext.cy = y;
    return NtGdiComputeXformCoefficients( hdc );
}

/***********************************************************************
 *		GetWindowOrgEx (GDI32.@)
 */
BOOL WINAPI GetWindowOrgEx( HDC hdc, POINT *point )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    *point = dc_attr->wnd_org;
    return TRUE;
}

/***********************************************************************
 *           SetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI SetWindowOrgEx( HDC hdc, INT x, INT y, POINT *point )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetWindowOrgEx( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetWindowOrgEx( dc_attr, x, y )) return FALSE;

    if (point) *point = dc_attr->wnd_org;
    dc_attr->wnd_org.x = x;
    dc_attr->wnd_org.y = y;
    return NtGdiComputeXformCoefficients( hdc );
}

/***********************************************************************
 *           OffsetWindowOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetWindowOrgEx( HDC hdc, INT x, INT y, POINT *point )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_OffsetWindowOrgEx( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (point) *point = dc_attr->wnd_org;
    dc_attr->wnd_org.x += x;
    dc_attr->wnd_org.y += y;
    if (dc_attr->emf && !EMFDC_SetWindowOrgEx( dc_attr, dc_attr->wnd_org.x,
                                               dc_attr->wnd_org.y )) return FALSE;
    return NtGdiComputeXformCoefficients( hdc );
}

/***********************************************************************
 *		GetViewportExtEx (GDI32.@)
 */
BOOL WINAPI GetViewportExtEx( HDC hdc, SIZE *size )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    *size = dc_attr->vport_ext;
    return TRUE;
}

/***********************************************************************
 *           SetViewportExtEx    (GDI32.@)
 */
BOOL WINAPI SetViewportExtEx( HDC hdc, INT x, INT y, LPSIZE size )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetViewportExtEx( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetViewportExtEx( dc_attr, x, y )) return FALSE;

    if (size) *size = dc_attr->vport_ext;
    if (dc_attr->map_mode != MM_ISOTROPIC && dc_attr->map_mode != MM_ANISOTROPIC) return TRUE;
    if (!x || !y) return FALSE;
    dc_attr->vport_ext.cx = x;
    dc_attr->vport_ext.cy = y;
    return NtGdiComputeXformCoefficients( hdc );
}

/***********************************************************************
 *		GetViewportOrgEx (GDI32.@)
 */
BOOL WINAPI GetViewportOrgEx( HDC hdc, POINT *point )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    *point = dc_attr->vport_org;
    return TRUE;
}

/***********************************************************************
 *           SetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI SetViewportOrgEx( HDC hdc, INT x, INT y, POINT *point )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetViewportOrgEx( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetViewportOrgEx( dc_attr, x, y )) return FALSE;

    if (point) *point = dc_attr->vport_org;
    dc_attr->vport_org.x = x;
    dc_attr->vport_org.y = y;
    return NtGdiComputeXformCoefficients( hdc );
}

/***********************************************************************
 *           OffsetViewportOrgEx    (GDI32.@)
 */
BOOL WINAPI OffsetViewportOrgEx( HDC hdc, INT x, INT y, POINT *point )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_OffsetViewportOrgEx( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (point) *point = dc_attr->vport_org;
    dc_attr->vport_org.x += x;
    dc_attr->vport_org.y += y;
    if (dc_attr->emf && !EMFDC_SetViewportOrgEx( dc_attr, dc_attr->vport_org.x,
                                                 dc_attr->vport_org.y )) return FALSE;
    return NtGdiComputeXformCoefficients( hdc );
}

/***********************************************************************
 *           GetWorldTransform    (GDI32.@)
 */
BOOL WINAPI GetWorldTransform( HDC hdc, XFORM *xform )
{
    return NtGdiGetTransform( hdc, 0x203, xform );
}

/****************************************************************************
 *           ModifyWorldTransform   (GDI32.@)
 */
BOOL WINAPI ModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode )
{
    DC_ATTR *dc_attr;

    TRACE( "dc %p xform %s mode %#lx\n", hdc, debugstr_xform( xform ), mode );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ModifyWorldTransform( dc_attr, xform, mode )) return FALSE;
    return NtGdiModifyWorldTransform( hdc, xform, mode );
}

/***********************************************************************
 *           SetWorldTransform    (GDI32.@)
 */
BOOL WINAPI SetWorldTransform( HDC hdc, const XFORM *xform )
{
    DC_ATTR *dc_attr;

    TRACE( "dc %p xform %s\n", hdc, debugstr_xform( xform ) );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetWorldTransform( dc_attr, xform )) return FALSE;
    return NtGdiModifyWorldTransform( hdc, xform, MWT_SET );
}

/***********************************************************************
 *		SetStretchBltMode (GDI32.@)
 */
INT WINAPI SetStretchBltMode( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode <= 0 || mode > MAXSTRETCHBLTMODE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetStretchBltMode( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetStretchBltMode( dc_attr, mode )) return 0;

    ret = dc_attr->stretch_blt_mode;
    dc_attr->stretch_blt_mode = mode;
    return ret;
}

/***********************************************************************
 *		GetCurrentPositionEx (GDI32.@)
 */
BOOL WINAPI GetCurrentPositionEx( HDC hdc, POINT *point )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    *point = dc_attr->cur_pos;
    return TRUE;
}

/***********************************************************************
 *		GetROP2 (GDI32.@)
 */
INT WINAPI GetROP2( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->rop_mode : 0;
}

/***********************************************************************
 *		GetRelAbs  (GDI32.@)
 */
INT WINAPI GetRelAbs( HDC hdc, DWORD ignore )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->rel_abs_mode : 0;
}

/***********************************************************************
 *		SetRelAbs (GDI32.@)
 */
INT WINAPI SetRelAbs( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if (mode != ABSOLUTE && mode != RELATIVE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetRelAbs( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    ret = dc_attr->rel_abs_mode;
    dc_attr->rel_abs_mode = mode;
    return ret;
}

/***********************************************************************
 *		SetROP2 (GDI32.@)
 */
INT WINAPI SetROP2( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;
    INT ret;

    if ((mode < R2_BLACK) || (mode > R2_WHITE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (is_meta_dc( hdc )) return METADC_SetROP2( hdc, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return 0;
    if (dc_attr->emf && !EMFDC_SetROP2( dc_attr, mode )) return 0;

    ret = dc_attr->rop_mode;
    dc_attr->rop_mode = mode;
    return ret;
}

/*******************************************************************
 *           SetMiterLimit  (GDI32.@)
 */
BOOL WINAPI SetMiterLimit( HDC hdc, FLOAT limit, FLOAT *old_limit )
{
    DC_ATTR *dc_attr;
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetMiterLimit( dc_attr, limit )) return 0;
    if (limit < 1.0f) return FALSE;
    return NtGdiSetMiterLimit( hdc, *(DWORD *)&limit, old_limit );
}

/***********************************************************************
 *           SetPixel    (GDI32.@)
 */
COLORREF WINAPI SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetPixel( hdc, x, y, color );
    if (!(dc_attr = get_dc_attr( hdc ))) return CLR_INVALID;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_SetPixel( dc_attr, x, y, color )) return CLR_INVALID;
    return NtGdiSetPixel( hdc, x, y, color );
}

/***********************************************************************
 *           SetPixelV    (GDI32.@)
 */
BOOL WINAPI SetPixelV( HDC hdc, INT x, INT y, COLORREF color )
{
    return SetPixel( hdc, x, y, color ) != CLR_INVALID;
}

/***********************************************************************
 *           LineTo    (GDI32.@)
 */
BOOL WINAPI LineTo( HDC hdc, INT x, INT y )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)\n", hdc, x, y );

    if (is_meta_dc( hdc )) return METADC_LineTo( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_LineTo( dc_attr, x, y )) return FALSE;
    return NtGdiLineTo( hdc, x, y );
}

/***********************************************************************
 *           MoveToEx    (GDI32.@)
 */
BOOL WINAPI MoveToEx( HDC hdc, INT x, INT y, POINT *pt )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d), %p\n", hdc, x, y, pt );

    if (is_meta_dc( hdc )) return METADC_MoveTo( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_MoveTo( dc_attr, x, y )) return FALSE;
    return NtGdiMoveTo( hdc, x, y, pt );
}

/***********************************************************************
 *           Arc    (GDI32.@)
 */
BOOL WINAPI Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (is_meta_dc( hdc ))
        return METADC_Arc( hdc, left, top, right, bottom,
                           xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_ARC ))
        return FALSE;

    return NtGdiArcInternal( NtGdiArc, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *           ArcTo    (GDI32.@)
 */
BOOL WINAPI ArcTo( HDC hdc, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_ARCTO ))
        return FALSE;

    return NtGdiArcInternal( NtGdiArcTo, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *           Chord    (GDI32.@)
 */
BOOL WINAPI Chord( HDC hdc, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (is_meta_dc( hdc ))
        return METADC_Chord( hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_CHORD ))
        return FALSE;

    return NtGdiArcInternal( NtGdiChord, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *           Pie   (GDI32.@)
 */
BOOL WINAPI Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), (%d, %d), (%d, %d)\n", hdc, left, top,
           right, bottom, xstart, ystart, xend, yend );

    if (is_meta_dc( hdc ))
        return METADC_Pie( hdc, left, top, right, bottom,
                           xstart, ystart, xend, yend );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_ArcChordPie( dc_attr, left, top, right, bottom,
                                            xstart, ystart, xend, yend, EMR_PIE ))
        return FALSE;

    return NtGdiArcInternal( NtGdiPie, hdc, left, top, right, bottom,
                             xstart, ystart, xend, yend );
}

/***********************************************************************
 *      AngleArc (GDI32.@)
 */
BOOL WINAPI AngleArc( HDC hdc, INT x, INT y, DWORD radius, FLOAT start_angle, FLOAT sweep_angle )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d), %lu, %f, %f\n", hdc, x, y, radius, start_angle, sweep_angle );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_AngleArc( dc_attr, x, y, radius, start_angle, sweep_angle ))
        return FALSE;
    return NtGdiAngleArc( hdc, x, y, radius, *(DWORD *)&start_angle, *(DWORD *)&sweep_angle );
}

/***********************************************************************
 *           Ellipse    (GDI32.@)
 */
BOOL WINAPI Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d)\n", hdc, left, top, right, bottom );

    if (is_meta_dc( hdc )) return METADC_Ellipse( hdc, left, top, right, bottom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_Ellipse( dc_attr, left, top, right, bottom )) return FALSE;
    return NtGdiEllipse( hdc, left, top, right, bottom );
}

/***********************************************************************
 *           Rectangle    (GDI32.@)
 */
BOOL WINAPI Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d)\n", hdc, left, top, right, bottom );

    if (is_meta_dc( hdc )) return METADC_Rectangle( hdc, left, top, right, bottom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_Rectangle( dc_attr, left, top, right, bottom )) return FALSE;
    return NtGdiRectangle( hdc, left, top, right, bottom );
}

/***********************************************************************
 *           RoundRect    (GDI32.@)
 */
BOOL WINAPI RoundRect( HDC hdc, INT left, INT top, INT right,
                       INT bottom, INT ell_width, INT ell_height )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d)-(%d, %d), %dx%d\n", hdc, left, top, right, bottom,
           ell_width, ell_height );

    if (is_meta_dc( hdc ))
        return METADC_RoundRect( hdc, left, top, right, bottom, ell_width, ell_height );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_RoundRect( dc_attr, left, top, right, bottom,
                                          ell_width, ell_height ))
        return FALSE;

    return NtGdiRoundRect( hdc, left, top, right, bottom, ell_width, ell_height );
}

/**********************************************************************
 *          Polygon  (GDI32.@)
 */
BOOL WINAPI Polygon( HDC hdc, const POINT *points, INT count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %d\n", hdc, points, count );

    if (is_meta_dc( hdc )) return METADC_Polygon( hdc, points, count );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_Polygon( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, (const ULONG *)&count, 1, NtGdiPolyPolygon );
}

/**********************************************************************
 *          PolyPolygon  (GDI32.@)
 */
BOOL WINAPI PolyPolygon( HDC hdc, const POINT *points, const INT *counts, UINT polygons )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %u\n", hdc, points, counts, polygons );

    if (is_meta_dc( hdc )) return METADC_PolyPolygon( hdc, points, counts, polygons );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PolyPolygon( dc_attr, points, counts, polygons )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, (const ULONG *)counts, polygons, NtGdiPolyPolygon );
}

/**********************************************************************
 *          Polyline   (GDI32.@)
 */
BOOL WINAPI Polyline( HDC hdc, const POINT *points, INT count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %d\n", hdc, points, count );

    if (is_meta_dc( hdc )) return METADC_Polyline( hdc, points, count );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_Polyline( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, (const ULONG *)&count, 1, NtGdiPolyPolyline );
}

/**********************************************************************
 *          PolyPolyline  (GDI32.@)
 */
BOOL WINAPI PolyPolyline( HDC hdc, const POINT *points, const DWORD *counts, DWORD polylines )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %lu\n", hdc, points, counts, polylines );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PolyPolyline( dc_attr, points, counts, polylines )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, counts, polylines, NtGdiPolyPolyline );
}

/******************************************************************************
 *          PolyBezier  (GDI32.@)
 */
BOOL WINAPI PolyBezier( HDC hdc, const POINT *points, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %lu\n", hdc, points, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PolyBezier( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, &count, 1, NtGdiPolyBezier );
}

/******************************************************************************
 *          PolyBezierTo  (GDI32.@)
 */
BOOL WINAPI PolyBezierTo( HDC hdc, const POINT *points, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %lu\n", hdc, points, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PolyBezierTo( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, &count, 1, NtGdiPolyBezierTo );
}

/**********************************************************************
 *          PolylineTo   (GDI32.@)
 */
BOOL WINAPI PolylineTo( HDC hdc, const POINT *points, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %lu\n", hdc, points, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PolylineTo( dc_attr, points, count )) return FALSE;
    return NtGdiPolyPolyDraw( hdc, points, &count, 1, NtGdiPolylineTo );
}

/***********************************************************************
 *      PolyDraw (GDI32.@)
 */
BOOL WINAPI PolyDraw( HDC hdc, const POINT *points, const BYTE *types, DWORD count )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %lu\n", hdc, points, types, count );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PolyDraw( dc_attr, points, types, count )) return FALSE;
    return NtGdiPolyDraw( hdc, points, types, count );
}

/***********************************************************************
 *           FillRgn    (GDI32.@)
 */
BOOL WINAPI FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p\n", hdc, hrgn, hbrush );

    if (is_meta_dc( hdc )) return METADC_FillRgn( hdc, hrgn, hbrush );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_FillRgn( dc_attr, hrgn, hbrush )) return FALSE;
    return NtGdiFillRgn( hdc, hrgn, hbrush );
}

/***********************************************************************
 *           PaintRgn    (GDI32.@)
 */
BOOL WINAPI PaintRgn( HDC hdc, HRGN hrgn )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p\n", hdc, hrgn );

    if (is_meta_dc( hdc )) return METADC_PaintRgn( hdc, hrgn );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PaintRgn( dc_attr, hrgn )) return FALSE;
    return NtGdiFillRgn( hdc, hrgn, GetCurrentObject( hdc, OBJ_BRUSH ));
}

/***********************************************************************
 *           FrameRgn     (GDI32.@)
 */
BOOL WINAPI FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p, %p, %dx%d\n", hdc, hrgn, hbrush, width, height );

    if (is_meta_dc( hdc )) return METADC_FrameRgn( hdc, hrgn, hbrush, width, height );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_FrameRgn( dc_attr, hrgn, hbrush, width, height ))
        return FALSE;
    return NtGdiFrameRgn( hdc, hrgn, hbrush, width, height );
}

/***********************************************************************
 *           InvertRgn    (GDI32.@)
 */
BOOL WINAPI InvertRgn( HDC hdc, HRGN hrgn )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, %p\n", hdc, hrgn );

    if (is_meta_dc( hdc )) return METADC_InvertRgn( hdc, hrgn );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_InvertRgn( dc_attr, hrgn )) return FALSE;
    return NtGdiInvertRgn( hdc, hrgn );
}

/***********************************************************************
 *          ExtFloodFill   (GDI32.@)
 */
BOOL WINAPI ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT fill_type )
{
    DC_ATTR *dc_attr;

    TRACE( "%p, (%d, %d), %08lx, %x\n", hdc, x, y, color, fill_type );

    if (is_meta_dc( hdc )) return METADC_ExtFloodFill( hdc, x, y, color, fill_type );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_ExtFloodFill( dc_attr, x, y, color, fill_type )) return FALSE;
    return NtGdiExtFloodFill( hdc, x, y, color, fill_type );
}

/***********************************************************************
 *          FloodFill   (GDI32.@)
 */
BOOL WINAPI FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}

/******************************************************************************
 *           GdiGradientFill   (GDI32.@)
 */
BOOL WINAPI GdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                             void *grad_array, ULONG ngrad, ULONG mode )
{
    DC_ATTR *dc_attr;

    TRACE( "%p vert_array:%p nvert:%ld grad_array:%p ngrad:%ld\n", hdc, vert_array,
           nvert, grad_array, ngrad );

    if (!(dc_attr = get_dc_attr( hdc )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf &&
        !EMFDC_GradientFill( dc_attr, vert_array, nvert, grad_array, ngrad, mode ))
        return FALSE;
    return NtGdiGradientFill( hdc, vert_array, nvert, grad_array, ngrad, mode );
}

/***********************************************************************
 *           SetTextJustification    (GDI32.@)
 */
BOOL WINAPI SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_SetTextJustification( hdc, extra, breaks );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetTextJustification( dc_attr, extra, breaks ))
        return FALSE;
    return NtGdiSetTextJustification( hdc, extra, breaks );
}

/***********************************************************************
 *           PatBlt    (GDI32.@)
 */
BOOL WINAPI PatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_PatBlt( hdc, left, top, width, height, rop );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PatBlt( dc_attr, left, top, width, height, rop ))
        return FALSE;
    return NtGdiPatBlt( hdc, left, top, width, height, rop );
}

/***********************************************************************
 *           BitBlt    (GDI32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH BitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height,
                                      HDC hdc_src, INT x_src, INT y_src, DWORD rop )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc_dst )) return METADC_BitBlt( hdc_dst, x_dst, y_dst, width, height,
                                                 hdc_src, x_src, y_src, rop );
    if (!(dc_attr = get_dc_attr( hdc_dst ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_BitBlt( dc_attr, x_dst, y_dst, width, height,
                                       hdc_src, x_src, y_src, rop ))
        return FALSE;
    return NtGdiBitBlt( hdc_dst, x_dst, y_dst, width, height,
                        hdc_src, x_src, y_src, rop, 0 /* FIXME */, 0 /* FIXME */ );
}

/***********************************************************************
 *           StretchBlt    (GDI32.@)
 */
BOOL WINAPI StretchBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                        HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                        DWORD rop )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_StretchBlt( hdc, x_dst, y_dst, width_dst, height_dst,
                                                     hdc_src, x_src, y_src, width_src,
                                                     height_src, rop );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_StretchBlt( dc_attr, x_dst, y_dst, width_dst, height_dst,
                                           hdc_src, x_src, y_src, width_src,
                                           height_src, rop ))
        return FALSE;
    return NtGdiStretchBlt( hdc, x_dst, y_dst, width_dst, height_dst,
                            hdc_src, x_src, y_src, width_src,
                            height_src, rop, 0 /* FIXME */ );
}

/***********************************************************************
 *           MaskBlt [GDI32.@]
 */
BOOL WINAPI MaskBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                     HDC hdc_src, INT x_src, INT y_src, HBITMAP mask,
                     INT x_mask, INT y_mask, DWORD rop )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_MaskBlt( dc_attr, x_dst, y_dst, width_dst, height_dst,
                                        hdc_src, x_src, y_src, mask, x_mask, y_mask, rop ))
        return FALSE;
    return NtGdiMaskBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src, x_src, y_src,
                         mask, x_mask, y_mask, rop, 0 /* FIXME */ );
}

/***********************************************************************
 *      PlgBlt    (GDI32.@)
 */
BOOL WINAPI PlgBlt( HDC hdc, const POINT *points, HDC hdc_src, INT x_src, INT y_src,
                    INT width, INT height, HBITMAP mask, INT x_mask, INT y_mask )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_PlgBlt( dc_attr, points, hdc_src, x_src, y_src,
                                       width, height, mask, x_mask, y_mask ))
        return FALSE;
    return NtGdiPlgBlt( hdc, points, hdc_src, x_src, y_src, width, height,
                        mask, x_mask, y_mask, 0 /* FIXME */ );
}

/******************************************************************************
 *           GdiTransparentBlt    (GDI32.@)
 */
BOOL WINAPI GdiTransparentBlt( HDC hdc, int x_dst, int y_dst, int width_dst, int height_dst,
                               HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                               UINT color )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_TransparentBlt( dc_attr, x_dst, y_dst, width_dst, height_dst, hdc_src,
                                               x_src, y_src, width_src, height_src, color ))
        return FALSE;
    return NtGdiTransparentBlt( hdc, x_dst, y_dst, width_dst, height_dst, hdc_src, x_src, y_src,
                                width_src, height_src, color );
}

/******************************************************************************
 *           GdiAlphaBlend   (GDI32.@)
 */
BOOL WINAPI GdiAlphaBlend( HDC hdc_dst, int x_dst, int y_dst, int width_dst, int height_dst,
                           HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                           BLENDFUNCTION blend_function)
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc_dst ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_AlphaBlend( dc_attr, x_dst, y_dst, width_dst, height_dst,
                                           hdc_src, x_src, y_src, width_src,
                                           height_src, blend_function ))
        return FALSE;
    return NtGdiAlphaBlend( hdc_dst, x_dst, y_dst, width_dst, height_dst,
                            hdc_src, x_src, y_src, width_src, height_src,
                            *(DWORD *)&blend_function, 0 /* FIXME */ );
}

/******************************************************************************
 *           SetDIBits    (GDI32.@)
 *
 * Sets pixels in a bitmap using colors from DIB.
 */
INT WINAPI SetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan,
		      UINT lines, const void *bits, const BITMAPINFO *info,
		      UINT coloruse )
{
    /* Wine-specific: pass hbitmap to NtGdiSetDIBitsToDeviceInternal */
    return NtGdiSetDIBitsToDeviceInternal( hdc, 0, 0, 0, 0, 0, 0,
                                           startscan, lines, bits, info, coloruse,
                                           0, 0, FALSE, hbitmap );
}

/***********************************************************************
 *           SetDIBitsToDevice   (GDI32.@)
 */
INT WINAPI SetDIBitsToDevice( HDC hdc, INT x_dst, INT y_dst, DWORD cx,
                              DWORD cy, INT x_src, INT y_src, UINT startscan,
                              UINT lines, const void *bits, const BITMAPINFO *bmi,
                              UINT coloruse )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc ))
        return METADC_SetDIBitsToDevice( hdc, x_dst, y_dst, cx, cy, x_src, y_src, startscan,
                                         lines, bits, bmi, coloruse );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_SetDIBitsToDevice( dc_attr, x_dst, y_dst, cx, cy, x_src, y_src,
                                                  startscan, lines, bits, bmi, coloruse ))
        return 0;
    return NtGdiSetDIBitsToDeviceInternal( hdc, x_dst, y_dst, cx, cy, x_src, y_src,
                                           startscan, lines, bits, bmi, coloruse,
                                           0, 0, FALSE, NULL );
}

/***********************************************************************
 *           StretchDIBits   (GDI32.@)
 */
INT WINAPI DECLSPEC_HOTPATCH StretchDIBits( HDC hdc, INT x_dst, INT y_dst, INT width_dst,
                                            INT height_dst, INT x_src, INT y_src, INT width_src,
                                            INT height_src, const void *bits, const BITMAPINFO *bmi,
                                            UINT coloruse, DWORD rop )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc ))
        return METADC_StretchDIBits( hdc, x_dst, y_dst, width_dst, height_dst, x_src, y_src,
                                     width_src, height_src, bits, bmi, coloruse, rop );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_StretchDIBits( dc_attr, x_dst, y_dst, width_dst, height_dst,
                                              x_src, y_src, width_src, height_src, bits,
                                              bmi, coloruse, rop ))
        return FALSE;
    return NtGdiStretchDIBitsInternal( hdc, x_dst, y_dst, width_dst, height_dst, x_src, y_src,
                                       width_src, height_src, bits, bmi, coloruse, rop,
                                       0, 0, NULL );
}

/***********************************************************************
 *           BeginPath    (GDI32.@)
 */
BOOL WINAPI BeginPath(HDC hdc)
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_BeginPath( dc_attr )) return FALSE;
    return NtGdiBeginPath( hdc );
}

/***********************************************************************
 *           EndPath    (GDI32.@)
 */
BOOL WINAPI EndPath(HDC hdc)
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_EndPath( dc_attr )) return FALSE;
    return NtGdiEndPath( hdc );
}

/***********************************************************************
 *           AbortPath  (GDI32.@)
 */
BOOL WINAPI AbortPath( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_AbortPath( dc_attr )) return FALSE;
    return NtGdiAbortPath( hdc );
}

/***********************************************************************
 *           CloseFigure    (GDI32.@)
 */
BOOL WINAPI CloseFigure( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_CloseFigure( dc_attr )) return FALSE;
    return NtGdiCloseFigure( hdc );
}

/***********************************************************************
 *           FillPath    (GDI32.@)
 */
BOOL WINAPI FillPath( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_FillPath( dc_attr )) return FALSE;
    return NtGdiFillPath( hdc );
}

/*******************************************************************
 *           StrokeAndFillPath   (GDI32.@)
 */
BOOL WINAPI StrokeAndFillPath( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_StrokeAndFillPath( dc_attr )) return FALSE;
    return NtGdiStrokeAndFillPath( hdc );
}

/*******************************************************************
 *           StrokePath   (GDI32.@)
 */
BOOL WINAPI StrokePath( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->print) print_call_start_page( dc_attr );
    if (dc_attr->emf && !EMFDC_StrokePath( dc_attr )) return FALSE;
    return NtGdiStrokePath( hdc );
}

/***********************************************************************
 *           FlattenPath   (GDI32.@)
 */
BOOL WINAPI FlattenPath( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_FlattenPath( dc_attr )) return FALSE;
    return NtGdiFlattenPath( hdc );
}

/***********************************************************************
 *           WidenPath   (GDI32.@)
 */
BOOL WINAPI WidenPath( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_WidenPath( dc_attr )) return FALSE;
    return NtGdiWidenPath( hdc );
}

/***********************************************************************
 *           SelectClipPath    (GDI32.@)
 */
BOOL WINAPI SelectClipPath( HDC hdc, INT mode )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SelectClipPath( dc_attr, mode )) return FALSE;
    return NtGdiSelectClipPath( hdc, mode );
}

/***********************************************************************
 *           GetClipRgn  (GDI32.@)
 */
INT WINAPI GetClipRgn( HDC hdc, HRGN rgn )
{
    return NtGdiGetRandomRgn( hdc, rgn, NTGDI_RGN_MIRROR_RTL | 1 );
}

/***********************************************************************
 *           GetMetaRgn    (GDI32.@)
 */
INT WINAPI GetMetaRgn( HDC hdc, HRGN rgn )
{
    return NtGdiGetRandomRgn( hdc, rgn, NTGDI_RGN_MIRROR_RTL | 2 );
}

/***********************************************************************
 *           IntersectClipRect    (GDI32.@)
 */
INT WINAPI IntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    DC_ATTR *dc_attr;

    TRACE("%p %d,%d - %d,%d\n", hdc, left, top, right, bottom );

    if (is_meta_dc( hdc )) return METADC_IntersectClipRect( hdc, left, top, right, bottom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_IntersectClipRect( dc_attr, left, top, right, bottom ))
        return FALSE;
    return NtGdiIntersectClipRect( hdc, left, top, right, bottom );
}

/***********************************************************************
 *           OffsetClipRgn    (GDI32.@)
 */
INT WINAPI OffsetClipRgn( HDC hdc, INT x, INT y )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_OffsetClipRgn( hdc, x, y );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_OffsetClipRgn( dc_attr, x, y )) return FALSE;
    return NtGdiOffsetClipRgn( hdc, x, y );
}

/***********************************************************************
 *           ExcludeClipRect    (GDI32.@)
 */
INT WINAPI ExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_ExcludeClipRect( hdc, left, top, right, bottom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ExcludeClipRect( dc_attr, left, top, right, bottom ))
        return FALSE;
    return NtGdiExcludeClipRect( hdc, left, top, right, bottom );
}

/******************************************************************************
 *		ExtSelectClipRgn     (GDI32.@)
 */
INT WINAPI ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT mode )
{
    DC_ATTR *dc_attr;

    TRACE("%p %p %d\n", hdc, hrgn, mode );

    if (is_meta_dc( hdc )) return METADC_ExtSelectClipRgn( hdc, hrgn, mode );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ExtSelectClipRgn( dc_attr, hrgn, mode ))
        return FALSE;
    return NtGdiExtSelectClipRgn( hdc, hrgn, mode );
}

/***********************************************************************
 *           SelectClipRgn    (GDI32.@)
 */
INT WINAPI SelectClipRgn( HDC hdc, HRGN hrgn )
{
    return ExtSelectClipRgn( hdc, hrgn, RGN_COPY );
}

/***********************************************************************
 *           SetMetaRgn    (GDI32.@)
 */
INT WINAPI SetMetaRgn( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SetMetaRgn( dc_attr )) return FALSE;
    return NtGdiSetMetaRgn( hdc );
}

/***********************************************************************
 *           DPtoLP    (GDI32.@)
 */
BOOL WINAPI DPtoLP( HDC hdc, POINT *points, INT count )
{
    return NtGdiTransformPoints( hdc, points, points, count, NtGdiDPtoLP );
}

/***********************************************************************
 *           LPtoDP    (GDI32.@)
 */
BOOL WINAPI LPtoDP( HDC hdc, POINT *points, INT count )
{
    return NtGdiTransformPoints( hdc, points, points, count, NtGdiLPtoDP );
}

/***********************************************************************
 *           ScaleViewportExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                INT y_num, INT y_denom, SIZE *size )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_ScaleViewportExtEx( hdc, x_num, x_denom, y_num, y_denom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ScaleViewportExtEx( dc_attr, x_num, x_denom, y_num, y_denom ))
        return FALSE;
    return NtGdiScaleViewportExtEx( hdc, x_num, x_denom, y_num, y_denom, size );
}

/***********************************************************************
 *           ScaleWindowExtEx    (GDI32.@)
 */
BOOL WINAPI ScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                              INT y_num, INT y_denom, SIZE *size )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_ScaleWindowExtEx( hdc, x_num, x_denom, y_num, y_denom );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ScaleWindowExtEx( dc_attr, x_num, x_denom, y_num, y_denom ))
        return FALSE;
    return NtGdiScaleWindowExtEx( hdc, x_num, x_denom, y_num, y_denom, size );
}

/* Pointers to USER implementation of SelectPalette/RealizePalette */
/* they will be patched by USER on startup */
HPALETTE (WINAPI *pfnSelectPalette)( HDC hdc, HPALETTE hpal, WORD bkgnd ) = NtUserSelectPalette;
UINT (WINAPI *pfnRealizePalette)( HDC hdc ) = NtUserRealizePalette;

/***********************************************************************
 *           SelectPalette    (GDI32.@)
 */
HPALETTE WINAPI SelectPalette( HDC hdc, HPALETTE palette, BOOL force_background )
{
    DC_ATTR *dc_attr;

    palette = get_full_gdi_handle( palette );
    if (is_meta_dc( hdc )) return ULongToHandle( METADC_SelectPalette( hdc, palette ) );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_SelectPalette( dc_attr, palette )) return 0;
    return pfnSelectPalette( hdc, palette, force_background );
}

/***********************************************************************
 *           RealizePalette    (GDI32.@)
 */
UINT WINAPI RealizePalette( HDC hdc )
{
    DC_ATTR *dc_attr;

    if (is_meta_dc( hdc )) return METADC_RealizePalette( hdc );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_RealizePalette( dc_attr )) return 0;
    return pfnRealizePalette( hdc );
}

/***********************************************************************
 *           GdiSetPixelFormat   (GDI32.@)
 */
BOOL WINAPI GdiSetPixelFormat( HDC hdc, INT format, const PIXELFORMATDESCRIPTOR *descr )
{
    TRACE( "(%p,%d,%p)\n", hdc, format, descr );
    return NtGdiSetPixelFormat( hdc, format );
}

/***********************************************************************
 *           CancelDC    (GDI32.@)
 */
BOOL WINAPI CancelDC(HDC hdc)
{
    return NtGdiCancelDC( hdc );
}

/***********************************************************************
 *           StartDocW  [GDI32.@]
 *
 * StartDoc calls the STARTDOC Escape with the input data pointing to DocName
 * and the output data (which is used as a second input parameter).pointing at
 * the whole docinfo structure.  This seems to be an undocumented feature of
 * the STARTDOC Escape.
 *
 * Note: we now do it the other way, with the STARTDOC Escape calling StartDoc.
 */
INT WINAPI StartDocW( HDC hdc, const DOCINFOW *doc )
{
    DOC_INFO_1W spool_info;
    WCHAR *output = NULL;
    struct print *print;
    DC_ATTR *dc_attr;
    ABORTPROC proc;
    DOCINFOW info;
    INT ret;

    TRACE("%p %p\n", hdc, doc);

    if (doc)
    {
        info = *doc;
    }
    else
    {
        memset( &info, 0, sizeof(info) );
        info.cbSize = sizeof(info);
    }

    TRACE("Size: %d, DocName %s, Output %s, Datatype %s, fwType %#lx\n",
          info.cbSize, debugstr_w(info.lpszDocName), debugstr_w(info.lpszOutput),
          debugstr_w(info.lpszDatatype), info.fwType);

    if (!(dc_attr = get_dc_attr( hdc ))) return SP_ERROR;
    if (dc_attr->print && dc_attr->emf) return SP_ERROR;

    proc = (ABORTPROC)(UINT_PTR)dc_attr->abort_proc;
    if (proc && !proc( hdc, 0 )) return 0;

    print = get_dc_print( dc_attr );
    if (print)
    {
        if (!info.lpszOutput) info.lpszOutput = print->output;
        output = StartDocDlgW( print->printer, &info );
        if (output) info.lpszOutput = output;

        if (info.lpszDatatype && wcsicmp(info.lpszDatatype, L"EMF"))
            FIXME("Ignoring DataType %s and forcing EMF\n", debugstr_w(info.lpszDatatype));

        spool_info.pDocName = (WCHAR *)info.lpszDocName;
        spool_info.pOutputFile = (WCHAR *)info.lpszOutput;
        spool_info.pDatatype = (WCHAR *)L"NT EMF 1.003";
        if ((ret = StartDocPrinterW( print->printer, 1, (BYTE *)&spool_info )))
        {
            if (!spool_start_doc( dc_attr, print->printer, &info ))
            {
                AbortDoc( hdc );
                ret = 0;
            }
            HeapFree( GetProcessHeap(), 0, output );
            print->flags |= CALL_START_PAGE;
            return ret;
        }
    }

    ret = NtGdiStartDoc( hdc, &info, NULL, 0 );
    HeapFree( GetProcessHeap(), 0, output );
    if (ret && print) print->flags |= CALL_START_PAGE;
    return ret;
}

/***********************************************************************
 *           StartDocA [GDI32.@]
 */
INT WINAPI StartDocA( HDC hdc, const DOCINFOA *doc )
{
    WCHAR *doc_name = NULL, *output = NULL, *data_type = NULL;
    DOCINFOW docW;
    INT ret, len;

    if (!doc) return StartDocW(hdc, NULL);

    docW.cbSize = doc->cbSize;
    if (doc->lpszDocName)
    {
        len = MultiByteToWideChar( CP_ACP, 0, doc->lpszDocName, -1, NULL, 0 );
        doc_name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, doc->lpszDocName, -1, doc_name, len );
    }
    if (doc->lpszOutput)
    {
        len = MultiByteToWideChar( CP_ACP, 0, doc->lpszOutput, -1, NULL, 0 );
        output = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, doc->lpszOutput, -1, output, len );
    }
    if (doc->lpszDatatype)
    {
        len = MultiByteToWideChar( CP_ACP, 0, doc->lpszDatatype, -1, NULL, 0);
        data_type = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, doc->lpszDatatype, -1, data_type, len );
    }

    docW.lpszDocName = doc_name;
    docW.lpszOutput = output;
    docW.lpszDatatype = data_type;
    docW.fwType = doc->fwType;

    ret = StartDocW(hdc, &docW);

    HeapFree( GetProcessHeap(), 0, doc_name );
    HeapFree( GetProcessHeap(), 0, output );
    HeapFree( GetProcessHeap(), 0, data_type );
    return ret;
}

/***********************************************************************
 *           StartPage    (GDI32.@)
 */
INT WINAPI StartPage( HDC hdc )
{
    struct print *print;
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return SP_ERROR;
    print = get_dc_print( dc_attr );
    if (print)
    {
        print->flags = (print->flags & ~CALL_START_PAGE) | CALL_END_PAGE;
        if (dc_attr->emf)
            return spool_start_page( dc_attr, print->printer );
    }
    return NtGdiStartPage( hdc );
}

/***********************************************************************
 *           EndPage    (GDI32.@)
 */
INT WINAPI EndPage( HDC hdc )
{
    struct print *print;
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return SP_ERROR;
    print = get_dc_print( dc_attr );
    if (print)
    {
        BOOL write = print->flags & WRITE_DEVMODE;

        if (!(print->flags & CALL_END_PAGE)) return SP_ERROR;
        print->flags = (print->flags & ~(CALL_END_PAGE | WRITE_DEVMODE)) | CALL_START_PAGE;
        if (dc_attr->emf)
            return spool_end_page( dc_attr, print->printer, print->devmode, write );
    }
    return NtGdiEndPage( hdc );
}

/***********************************************************************
 *           EndDoc    (GDI32.@)
 */
INT WINAPI EndDoc( HDC hdc )
{
    struct print *print;
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return SP_ERROR;
    print = get_dc_print( dc_attr );
    if (print)
    {
        if (print->flags & CALL_END_PAGE) EndPage( hdc );
        print->flags &= ~CALL_START_PAGE;
        if (dc_attr->emf)
            return spool_end_doc( dc_attr, print->printer );
    }
    return NtGdiEndDoc( hdc );
}

/***********************************************************************
 *           AbortDoc    (GDI32.@)
 */
INT WINAPI AbortDoc( HDC hdc )
{
    struct print *print;
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return SP_ERROR;
    print = get_dc_print( dc_attr );
    if (print)
    {
        print->flags &= ~(CALL_START_PAGE | CALL_END_PAGE);
        if (dc_attr->emf)
            return spool_abort_doc( dc_attr, print->printer );
    }
    return NtGdiAbortDoc( hdc );
}

/**********************************************************************
 *           SetAbortProc   (GDI32.@)
 */
INT WINAPI SetAbortProc( HDC hdc, ABORTPROC abrtprc )
{
    DC_ATTR *dc_attr;

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    dc_attr->abort_proc = (UINT_PTR)abrtprc;
    return TRUE;
}

/***********************************************************************
 *           SetICMMode    (GDI32.@)
 */
INT WINAPI SetICMMode( HDC hdc, INT mode )
{
    /* FIXME: Assume that ICM is always off, and cannot be turned on */
    switch (mode)
    {
    case ICM_OFF:   return ICM_OFF;
    case ICM_ON:    return 0;
    case ICM_QUERY: return ICM_OFF;
    }
    return 0;
}

/***********************************************************************
 *           GdiIsMetaPrintDC  (GDI32.@)
 */
BOOL WINAPI GdiIsMetaPrintDC( HDC hdc )
{
    DC_ATTR *dc_attr;

    TRACE( "%p\n", hdc );

    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    return dc_attr->print && dc_attr->emf;
}

/***********************************************************************
 *           GdiIsMetaFileDC  (GDI32.@)
 */
BOOL WINAPI GdiIsMetaFileDC( HDC hdc )
{
    TRACE( "%p\n", hdc );

    switch (GetObjectType( hdc ))
    {
    case OBJ_METADC:
    case OBJ_ENHMETADC:
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           GdiIsPlayMetafileDC  (GDI32.@)
 */
BOOL WINAPI GdiIsPlayMetafileDC( HDC hdc )
{
    FIXME( "%p\n", hdc );
    return FALSE;
}

/*******************************************************************
 *           DrawEscape    (GDI32.@)
 */
INT WINAPI DrawEscape( HDC hdc, INT escape, INT input_size, const char *input )
{
    FIXME( "stub\n" );
    return 0;
}

/*******************************************************************
 *           NamedEscape    (GDI32.@)
 */
INT WINAPI NamedEscape( HDC hdc, const WCHAR *driver, INT escape, INT input_size,
                        const char *input, INT output_size, char *output )
{
    FIXME( "(%p %s %d, %d %p %d %p)\n", hdc, wine_dbgstr_w(driver), escape, input_size,
           input, output_size, output );
    return 0;
}

/*******************************************************************
 *           DdQueryDisplaySettingsUniqueness    (GDI32.@)
 *           GdiEntry13
 */
ULONG WINAPI DdQueryDisplaySettingsUniqueness(void)
{
    static int warn_once;
    if (!warn_once++) FIXME( "stub\n" );
    return 0;
}

/*******************************************************************
 *           GdiGetSpoolFileHandle    (GDI32.@)
 */
HANDLE WINAPI GdiGetSpoolFileHandle( WCHAR *printer_name,
        DEVMODEW *devmode, WCHAR *doc_name )
{
    struct spool_handle *ret;
    HANDLE spool;

    TRACE( "%s %p %s\n", wine_dbgstr_w(printer_name), devmode, wine_dbgstr_w(doc_name) );

    if (!devmode) return NULL;
    if (!OpenPrinterW( doc_name, &spool, NULL )) return NULL;

    ret = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret) );
    if (!ret)
    {
        ClosePrinter( spool );
        return NULL;
    }
    ret->spool = spool;

    ret->devmodes = HeapAlloc( GetProcessHeap(), 0, sizeof(*ret->devmodes) * 8);
    if (!ret->devmodes)
    {
        GdiDeleteSpoolFileHandle( ret );
        return NULL;
    }
    ret->devmodes_size = 8;

    ret->devmodes[0].devmode = HeapAlloc( GetProcessHeap(), 0,
            devmode->dmSize + devmode->dmDriverExtra );
    if (!ret->devmodes[0].devmode)
    {
        GdiDeleteSpoolFileHandle( ret );
        return NULL;
    }
    memcpy( ret->devmodes[0].devmode, devmode, devmode->dmSize + devmode->dmDriverExtra );
    ret->devmodes[0].page = 0;
    ret->devmodes_no = 1;
    return ret;
}

/*******************************************************************
 *           GdiDeleteSpoolFileHandle    (GDI32.@)
 */
BOOL WINAPI GdiDeleteSpoolFileHandle( HANDLE h )
{
    struct spool_handle *sh = (struct spool_handle *)h;
    int i;

    TRACE( "%p\n", h );

    if (!sh)
        return FALSE;

    ClosePrinter( sh->spool );
    for (i = 0; i < sh->devmodes_no; i++)
        HeapFree( GetProcessHeap(), 0, sh->devmodes[i].devmode );
    HeapFree( GetProcessHeap(), 0, sh->devmodes );
    HeapFree( GetProcessHeap(), 0, sh );
    return TRUE;
}

static BOOL read_emfspool_record( struct spool_handle *sh )
{
    struct record_hdr
    {
        unsigned int ulID;
        unsigned int cjSize;
    } hdr;
    LARGE_INTEGER pos;
    BOOL ret;
    DWORD r;

    if (!sh->spool) return FALSE;

    ret = ReadPrinter( sh->spool, &hdr, sizeof(hdr), &r );
    if (!ret || r != sizeof(hdr))
    {
        ClosePrinter( sh->spool );
        sh->spool = NULL;
        return FALSE;
    }
    TRACE( "parsing record %u\n", hdr.ulID );

    if (hdr.ulID == EMRI_HEADER)
        hdr.cjSize -= sizeof(hdr);

    switch (hdr.ulID)
    {
    case EMRI_DEVMODE:
        /* remove unused devmode */
        if (sh->devmodes_no - 2 >= 0 && sh->devmodes[sh->devmodes_no - 2].page ==
                sh->devmodes[sh->devmodes_no - 1].page)
        {
            HeapFree( GetProcessHeap(), 0, sh->devmodes[sh->devmodes_no - 1].devmode );
            sh->devmodes_no--;
        }
        else if (sh->devmodes_no == sh->devmodes_size)
        {
            void *alloc = HeapReAlloc( GetProcessHeap(), 0, sh->devmodes, sh->devmodes_size * 2 );

            if (!alloc)
            {
                ClosePrinter( sh->spool );
                sh->spool = NULL;
                return FALSE;
            }
            sh->devmodes = alloc;
            sh->devmodes_size *= 2;
        }

        sh->devmodes[sh->devmodes_no].devmode = HeapAlloc( GetProcessHeap(), 0, hdr.cjSize );
        if (!sh->devmodes[sh->devmodes_no].devmode)
        {
            ClosePrinter( sh->spool );
            sh->spool = NULL;
            return FALSE;
        }

        ret = ReadPrinter( sh->spool, sh->devmodes[sh->devmodes_no].devmode, hdr.cjSize, &r );
        if (!ret || r != hdr.cjSize)
        {
            ClosePrinter( sh->spool );
            sh->spool = NULL;
            return FALSE;
        }
        sh->devmodes[sh->devmodes_no].page = sh->devmodes[sh->devmodes_no - 1].page;
        sh->devmodes_no++;
        return TRUE;

    case EMRI_METAFILE:
    case EMRI_FORM_METAFILE:
    case EMRI_BW_METAFILE:
    case EMRI_BW_FORM_METAFILE:
    case EMRI_METAFILE_EXT:
    case EMRI_BW_METAFILE_EXT:
        sh->devmodes[sh->devmodes_no - 1].page++;
        /* fall through */
    default:
        pos.QuadPart = hdr.cjSize;
        ret = SeekPrinter( sh->spool, pos, NULL, FILE_CURRENT, FALSE );
        if (!ret)
        {
            ClosePrinter( sh->spool );
            sh->spool = NULL;
            return FALSE;
        }
        return TRUE;
    }
}

/*******************************************************************
 *           GdiGetDevmodeForPage    (GDI32.@)
 */
BOOL WINAPI GdiGetDevmodeForPage( HANDLE h, DWORD page, DEVMODEW **cur, DEVMODEW **prev )
{
    struct spool_handle *sh = (struct spool_handle *)h;
    int i;

    TRACE( "%p %ld %p %p\n", h, page, cur, prev );

    if (!sh)
        return FALSE;

    i = 0;
    while (1)
    {
        if (sh->devmodes[i].page >= page)
        {
            if (cur) *cur = sh->devmodes[i].devmode;
            if (prev)
            {
                if (!i || sh->devmodes[i - 1].page != page - 1)
                    *prev = sh->devmodes[i].devmode;
                else
                    *prev = sh->devmodes[i - 1].devmode;
            }
            return TRUE;
        }

        if (i + 1 < sh->devmodes_no)
            i++;
        else if (!read_emfspool_record( sh ))
            return FALSE;
    }
}
