/*
 * GDI functions
 *
 * Copyright 1993 Alexandre Julliard
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winnls.h"
#include "winerror.h"
#include "winternl.h"

#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

#define FIRST_GDI_HANDLE 32
#define MAX_GDI_HANDLES  16384

struct hdc_list
{
    HDC hdc;
    struct hdc_list *next;
};

struct gdi_handle_entry
{
    void                       *obj;         /* pointer to the object-specific data */
    const struct gdi_obj_funcs *funcs;       /* type-specific functions */
    struct hdc_list            *hdcs;        /* list of HDCs interested in this object */
    WORD                        generation;  /* generation count for reusing handle values */
    WORD                        type;        /* object type (one of the OBJ_* constants) */
    WORD                        selcount;    /* number of times the object is selected in a DC */
    WORD                        system : 1;  /* system object flag */
    WORD                        deleted : 1; /* whether DeleteObject has been called on this object */
};

static struct gdi_handle_entry gdi_handles[MAX_GDI_HANDLES];
static struct gdi_handle_entry *next_free;
static struct gdi_handle_entry *next_unused = gdi_handles;
static LONG debug_count;
HMODULE gdi32_module = 0;

static inline HGDIOBJ entry_to_handle( struct gdi_handle_entry *entry )
{
    unsigned int idx = entry - gdi_handles + FIRST_GDI_HANDLE;
    return LongToHandle( idx | (entry->generation << 16) );
}

static inline struct gdi_handle_entry *handle_entry( HGDIOBJ handle )
{
    unsigned int idx = LOWORD(handle) - FIRST_GDI_HANDLE;

    if (idx < MAX_GDI_HANDLES && gdi_handles[idx].type)
    {
        if (!HIWORD( handle ) || HIWORD( handle ) == gdi_handles[idx].generation)
            return &gdi_handles[idx];
    }
    if (handle) WARN( "invalid handle %p\n", handle );
    return NULL;
}

/***********************************************************************
 *          GDI stock objects
 */

static const LOGBRUSH WhiteBrush = { BS_SOLID, RGB(255,255,255), 0 };
static const LOGBRUSH BlackBrush = { BS_SOLID, RGB(0,0,0), 0 };
static const LOGBRUSH NullBrush  = { BS_NULL, 0, 0 };

static const LOGBRUSH LtGrayBrush = { BS_SOLID, RGB(192,192,192), 0 };
static const LOGBRUSH GrayBrush   = { BS_SOLID, RGB(128,128,128), 0 };
static const LOGBRUSH DkGrayBrush = { BS_SOLID, RGB(64,64,64), 0 };

static const LOGPEN WhitePen = { PS_SOLID, { 0, 0 }, RGB(255,255,255) };
static const LOGPEN BlackPen = { PS_SOLID, { 0, 0 }, RGB(0,0,0) };
static const LOGPEN NullPen  = { PS_NULL,  { 0, 0 }, 0 };

static const LOGBRUSH DCBrush = { BS_SOLID, RGB(255,255,255), 0 };
static const LOGPEN DCPen     = { PS_SOLID, { 0, 0 }, RGB(0,0,0) };

/* reserve one extra entry for the stock default bitmap */
/* this is what Windows does too */
#define NB_STOCK_OBJECTS (STOCK_LAST+2)

static HGDIOBJ stock_objects[NB_STOCK_OBJECTS];
static HGDIOBJ scaled_stock_objects[NB_STOCK_OBJECTS];

static CRITICAL_SECTION gdi_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &gdi_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": gdi_section") }
};
static CRITICAL_SECTION gdi_section = { &critsect_debug, -1, 0, 0, 0, 0 };


/****************************************************************************
 *
 *	language-independent stock fonts
 *
 */

static const LOGFONTW OEMFixedFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"" };

static const LOGFONTW AnsiFixedFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier" };

static const LOGFONTW AnsiVarFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" };

/******************************************************************************
 *
 *      language-dependent stock fonts
 *
 *      'ANSI' charset and 'DEFAULT' charset is not same.
 *      The chars in CP_ACP should be drawn with 'DEFAULT' charset.
 *      'ANSI' charset seems to be identical with ISO-8859-1.
 *      'DEFAULT' charset is a language-dependent charset.
 *
 *      'System' font seems to be an alias for language-dependent font.
 */

/*
 * language-dependent stock fonts for all known charsets
 * please see TranslateCharsetInfo (dlls/gdi/font.c) and
 * CharsetBindingInfo (dlls/x11drv/xfont.c),
 * and modify entries for your language if needed.
 */
struct DefaultFontInfo
{
        UINT            charset;
        LOGFONTW        SystemFont;
        LOGFONTW        DeviceDefaultFont;
        LOGFONTW        SystemFixedFont;
        LOGFONTW        DefaultGuiFont;
};

static const struct DefaultFontInfo default_fonts[] =
{
    {   ANSI_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   EASTEUROPE_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   RUSSIAN_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   GREEK_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   TURKISH_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   HEBREW_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   ARABIC_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   BALTIC_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   THAI_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   SHIFTJIS_CHARSET,
        { /* System */
          18, 8, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   GB2312_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   HANGEUL_CHARSET,
        { /* System */
          16, 8, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   CHINESEBIG5_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
    {   JOHAB_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* Device Default */
          16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System"
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier"
        },
        { /* DefaultGuiFont */
          -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Shell Dlg"
        },
    },
};


/*************************************************************************
 * __wine_make_gdi_object_system    (GDI32.@)
 *
 * USER has to tell GDI that its system brushes and pens are non-deletable.
 * For a description of the GDI object magics and their flags,
 * see "Undocumented Windows" (wrong about the OBJECT_NOSYSTEM flag, though).
 */
void CDECL __wine_make_gdi_object_system( HGDIOBJ handle, BOOL set)
{
    struct gdi_handle_entry *entry;

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle ))) entry->system = !!set;
    LeaveCriticalSection( &gdi_section );
}

/******************************************************************************
 *      get_default_fonts
 */
static const struct DefaultFontInfo* get_default_fonts(UINT charset)
{
        unsigned int n;

        for(n = 0; n < ARRAY_SIZE( default_fonts ); n++)
        {
                if ( default_fonts[n].charset == charset )
                        return &default_fonts[n];
        }

        FIXME( "unhandled charset 0x%08x - use ANSI_CHARSET for default stock objects\n", charset );
        return &default_fonts[0];
}


/******************************************************************************
 *      get_default_charset    (internal)
 *
 * get the language-dependent charset that can handle CP_ACP correctly.
 */
static UINT get_default_charset( void )
{
    CHARSETINFO     csi;
    UINT    uACP;

    uACP = GetACP();
    csi.ciCharset = ANSI_CHARSET;
    if ( !TranslateCharsetInfo( ULongToPtr(uACP), &csi, TCI_SRCCODEPAGE ) )
    {
        FIXME( "unhandled codepage %u - use ANSI_CHARSET for default stock objects\n", uACP );
        return ANSI_CHARSET;
    }

    return csi.ciCharset;
}


/***********************************************************************
 *           GDI_get_ref_count
 *
 * Retrieve the reference count of a GDI object.
 * Note: the object must be locked otherwise the count is meaningless.
 */
UINT GDI_get_ref_count( HGDIOBJ handle )
{
    struct gdi_handle_entry *entry;
    UINT ret = 0;

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle ))) ret = entry->selcount;
    LeaveCriticalSection( &gdi_section );
    return ret;
}


/***********************************************************************
 *           GDI_inc_ref_count
 *
 * Increment the reference count of a GDI object.
 */
HGDIOBJ GDI_inc_ref_count( HGDIOBJ handle )
{
    struct gdi_handle_entry *entry;

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle ))) entry->selcount++;
    else handle = 0;
    LeaveCriticalSection( &gdi_section );
    return handle;
}


/***********************************************************************
 *           GDI_dec_ref_count
 *
 * Decrement the reference count of a GDI object.
 */
BOOL GDI_dec_ref_count( HGDIOBJ handle )
{
    struct gdi_handle_entry *entry;

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle )))
    {
        assert( entry->selcount );
        if (!--entry->selcount && entry->deleted)
        {
            /* handle delayed DeleteObject*/
            entry->deleted = 0;
            LeaveCriticalSection( &gdi_section );
            TRACE( "executing delayed DeleteObject for %p\n", handle );
            DeleteObject( handle );
            return TRUE;
        }
    }
    LeaveCriticalSection( &gdi_section );
    return entry != NULL;
}


/******************************************************************************
 *              get_reg_dword
 *
 * Read a DWORD value from the registry
 */
static BOOL get_reg_dword(HKEY base, const WCHAR *key_name, const WCHAR *value_name, DWORD *value)
{
    HKEY key;
    DWORD type, data, size = sizeof(data);
    BOOL ret = FALSE;

    if (RegOpenKeyW(base, key_name, &key) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(key, value_name, NULL, &type, (void *)&data, &size) == ERROR_SUCCESS &&
            type == REG_DWORD)
        {
            *value = data;
            ret = TRUE;
        }
        RegCloseKey(key);
    }
    return ret;
}

/******************************************************************************
 *              get_dpi
 *
 * get the dpi from the registry
 */
DWORD get_dpi(void)
{
    DWORD dpi;

    if (get_reg_dword(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"LogPixels", &dpi))
        return dpi;
    if (get_reg_dword(HKEY_CURRENT_CONFIG, L"Software\\Fonts", L"LogPixels", &dpi))
        return dpi;
    return 0;
}

/******************************************************************************
 *              get_system_dpi
 *
 * Get the system DPI, based on the DPI awareness mode.
 */
DWORD get_system_dpi(void)
{
    static UINT (WINAPI *pGetDpiForSystem)(void);

    if (!pGetDpiForSystem)
    {
        HMODULE user = GetModuleHandleW( L"user32.dll" );
        if (user) pGetDpiForSystem = (void *)GetProcAddress( user, "GetDpiForSystem" );
    }
    return pGetDpiForSystem ? pGetDpiForSystem() : 96;
}

static HFONT create_scaled_font( const LOGFONTW *deffont )
{
    LOGFONTW lf;
    static DWORD dpi;

    if (!dpi)
    {
        dpi = get_dpi();
        if (!dpi) dpi = 96;
    }

    lf = *deffont;
    lf.lfHeight = MulDiv( lf.lfHeight, dpi, 96 );
    return CreateFontIndirectW( &lf );
}

/***********************************************************************
 *           DllMain
 *
 * GDI initialization.
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    const struct DefaultFontInfo* deffonts;
    int i;

    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    gdi32_module = inst;
    DisableThreadLibraryCalls( inst );
    font_init();

    /* create stock objects */
    stock_objects[WHITE_BRUSH]  = CreateBrushIndirect( &WhiteBrush );
    stock_objects[LTGRAY_BRUSH] = CreateBrushIndirect( &LtGrayBrush );
    stock_objects[GRAY_BRUSH]   = CreateBrushIndirect( &GrayBrush );
    stock_objects[DKGRAY_BRUSH] = CreateBrushIndirect( &DkGrayBrush );
    stock_objects[BLACK_BRUSH]  = CreateBrushIndirect( &BlackBrush );
    stock_objects[NULL_BRUSH]   = CreateBrushIndirect( &NullBrush );

    stock_objects[WHITE_PEN]    = CreatePenIndirect( &WhitePen );
    stock_objects[BLACK_PEN]    = CreatePenIndirect( &BlackPen );
    stock_objects[NULL_PEN]     = CreatePenIndirect( &NullPen );

    stock_objects[DEFAULT_PALETTE] = PALETTE_Init();
    stock_objects[DEFAULT_BITMAP]  = CreateBitmap( 1, 1, 1, 1, NULL );

    /* language-independent stock fonts */
    stock_objects[OEM_FIXED_FONT]      = CreateFontIndirectW( &OEMFixedFont );
    stock_objects[ANSI_FIXED_FONT]     = CreateFontIndirectW( &AnsiFixedFont );
    stock_objects[ANSI_VAR_FONT]       = CreateFontIndirectW( &AnsiVarFont );

    /* language-dependent stock fonts */
    deffonts = get_default_fonts(get_default_charset());
    stock_objects[SYSTEM_FONT]         = CreateFontIndirectW( &deffonts->SystemFont );
    stock_objects[DEVICE_DEFAULT_FONT] = CreateFontIndirectW( &deffonts->DeviceDefaultFont );
    stock_objects[SYSTEM_FIXED_FONT]   = CreateFontIndirectW( &deffonts->SystemFixedFont );
    stock_objects[DEFAULT_GUI_FONT]    = CreateFontIndirectW( &deffonts->DefaultGuiFont );

    scaled_stock_objects[OEM_FIXED_FONT]    = create_scaled_font( &OEMFixedFont );
    scaled_stock_objects[SYSTEM_FONT]       = create_scaled_font( &deffonts->SystemFont );
    scaled_stock_objects[SYSTEM_FIXED_FONT] = create_scaled_font( &deffonts->SystemFixedFont );
    scaled_stock_objects[DEFAULT_GUI_FONT]  = create_scaled_font( &deffonts->DefaultGuiFont );

    stock_objects[DC_BRUSH]     = CreateBrushIndirect( &DCBrush );
    stock_objects[DC_PEN]       = CreatePenIndirect( &DCPen );

    /* clear the NOSYSTEM bit on all stock objects*/
    for (i = 0; i < NB_STOCK_OBJECTS; i++)
    {
        if (stock_objects[i]) __wine_make_gdi_object_system( stock_objects[i], TRUE );
        if (scaled_stock_objects[i]) __wine_make_gdi_object_system( scaled_stock_objects[i], TRUE );
    }
    return TRUE;
}


/***********************************************************************
 *           GdiDllInitialize
 *
 * Stub entry point, some games (CoD: Black Ops 3) call it directly.
 */
BOOL WINAPI GdiDllInitialize( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    FIXME("stub\n");
    return TRUE;
}


static const char *gdi_obj_type( unsigned type )
{
    switch ( type )
    {
        case OBJ_PEN: return "OBJ_PEN";
        case OBJ_BRUSH: return "OBJ_BRUSH";
        case OBJ_DC: return "OBJ_DC";
        case OBJ_METADC: return "OBJ_METADC";
        case OBJ_PAL: return "OBJ_PAL";
        case OBJ_FONT: return "OBJ_FONT";
        case OBJ_BITMAP: return "OBJ_BITMAP";
        case OBJ_REGION: return "OBJ_REGION";
        case OBJ_METAFILE: return "OBJ_METAFILE";
        case OBJ_MEMDC: return "OBJ_MEMDC";
        case OBJ_EXTPEN: return "OBJ_EXTPEN";
        case OBJ_ENHMETADC: return "OBJ_ENHMETADC";
        case OBJ_ENHMETAFILE: return "OBJ_ENHMETAFILE";
        case OBJ_COLORSPACE: return "OBJ_COLORSPACE";
        default: return "UNKNOWN";
    }
}

static void dump_gdi_objects( void )
{
    struct gdi_handle_entry *entry;

    TRACE( "%u objects:\n", MAX_GDI_HANDLES );

    EnterCriticalSection( &gdi_section );
    for (entry = gdi_handles; entry < next_unused; entry++)
    {
        if (!entry->type)
            TRACE( "handle %p FREE\n", entry_to_handle( entry ));
        else
            TRACE( "handle %p obj %p type %s selcount %u deleted %u\n",
                   entry_to_handle( entry ), entry->obj, gdi_obj_type( entry->type ),
                   entry->selcount, entry->deleted );
    }
    LeaveCriticalSection( &gdi_section );
}

/***********************************************************************
 *           alloc_gdi_handle
 *
 * Allocate a GDI handle for an object, which must have been allocated on the process heap.
 */
HGDIOBJ alloc_gdi_handle( void *obj, WORD type, const struct gdi_obj_funcs *funcs )
{
    struct gdi_handle_entry *entry;
    HGDIOBJ ret;

    assert( type );  /* type 0 is reserved to mark free entries */

    EnterCriticalSection( &gdi_section );

    entry = next_free;
    if (entry)
        next_free = entry->obj;
    else if (next_unused < gdi_handles + MAX_GDI_HANDLES)
        entry = next_unused++;
    else
    {
        LeaveCriticalSection( &gdi_section );
        ERR( "out of GDI object handles, expect a crash\n" );
        if (TRACE_ON(gdi)) dump_gdi_objects();
        return 0;
    }
    entry->obj      = obj;
    entry->funcs    = funcs;
    entry->hdcs     = NULL;
    entry->type     = type;
    entry->selcount = 0;
    entry->system   = 0;
    entry->deleted  = 0;
    if (++entry->generation == 0xffff) entry->generation = 1;
    ret = entry_to_handle( entry );
    LeaveCriticalSection( &gdi_section );
    TRACE( "allocated %s %p %u/%u\n", gdi_obj_type(type), ret,
           InterlockedIncrement( &debug_count ), MAX_GDI_HANDLES );
    return ret;
}


/***********************************************************************
 *           free_gdi_handle
 *
 * Free a GDI handle and return a pointer to the object.
 */
void *free_gdi_handle( HGDIOBJ handle )
{
    void *object = NULL;
    struct gdi_handle_entry *entry;

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle )))
    {
        TRACE( "freed %s %p %u/%u\n", gdi_obj_type( entry->type ), handle,
               InterlockedDecrement( &debug_count ) + 1, MAX_GDI_HANDLES );
        object = entry->obj;
        entry->type = 0;
        entry->obj = next_free;
        next_free = entry;
    }
    LeaveCriticalSection( &gdi_section );
    return object;
}


/***********************************************************************
 *           get_full_gdi_handle
 *
 * Return the full GDI handle from a possibly truncated value.
 */
HGDIOBJ get_full_gdi_handle( HGDIOBJ handle )
{
    struct gdi_handle_entry *entry;

    if (!HIWORD( handle ))
    {
        EnterCriticalSection( &gdi_section );
        if ((entry = handle_entry( handle ))) handle = entry_to_handle( entry );
        LeaveCriticalSection( &gdi_section );
    }
    return handle;
}

/***********************************************************************
 *           get_any_obj_ptr
 *
 * Return a pointer to, and the type of, the GDI object
 * associated with the handle.
 * The object must be released with GDI_ReleaseObj.
 */
void *get_any_obj_ptr( HGDIOBJ handle, WORD *type )
{
    void *ptr = NULL;
    struct gdi_handle_entry *entry;

    EnterCriticalSection( &gdi_section );

    if ((entry = handle_entry( handle )))
    {
        ptr = entry->obj;
        *type = entry->type;
    }

    if (!ptr) LeaveCriticalSection( &gdi_section );
    return ptr;
}

/***********************************************************************
 *           GDI_GetObjPtr
 *
 * Return a pointer to the GDI object associated with the handle.
 * Return NULL if the object has the wrong type.
 * The object must be released with GDI_ReleaseObj.
 */
void *GDI_GetObjPtr( HGDIOBJ handle, WORD type )
{
    WORD ret_type;
    void *ptr = get_any_obj_ptr( handle, &ret_type );
    if (ptr && ret_type != type)
    {
        GDI_ReleaseObj( handle );
        ptr = NULL;
    }
    return ptr;
}

/***********************************************************************
 *           GDI_ReleaseObj
 *
 */
void GDI_ReleaseObj( HGDIOBJ handle )
{
    LeaveCriticalSection( &gdi_section );
}


/***********************************************************************
 *           GDI_CheckNotLock
 */
void GDI_CheckNotLock(void)
{
    if (RtlIsCriticalSectionLockedByThread(&gdi_section))
    {
        ERR( "BUG: holding GDI lock\n" );
        DebugBreak();
    }
}


/***********************************************************************
 *           DeleteObject    (GDI32.@)
 *
 * Delete a Gdi object.
 *
 * PARAMS
 *  obj [I] Gdi object to delete
 *
 * RETURNS
 *  Success: TRUE. If obj was not returned from GetStockObject(), any resources
 *           it consumed are released.
 *  Failure: FALSE, if obj is not a valid Gdi object, or is currently selected
 *           into a DC.
 */
BOOL WINAPI DeleteObject( HGDIOBJ obj )
{
    struct gdi_handle_entry *entry;
    struct hdc_list *hdcs_head;
    const struct gdi_obj_funcs *funcs = NULL;

    EnterCriticalSection( &gdi_section );
    if (!(entry = handle_entry( obj )))
    {
        LeaveCriticalSection( &gdi_section );
        return FALSE;
    }

    if (entry->system)
    {
	TRACE("Preserving system object %p\n", obj);
        LeaveCriticalSection( &gdi_section );
	return TRUE;
    }

    obj = entry_to_handle( entry );  /* make it a full handle */

    hdcs_head = entry->hdcs;
    entry->hdcs = NULL;

    if (entry->selcount)
    {
        TRACE("delayed for %p because object in use, count %u\n", obj, entry->selcount );
        entry->deleted = 1;  /* mark for delete */
    }
    else funcs = entry->funcs;

    LeaveCriticalSection( &gdi_section );

    while (hdcs_head)
    {
        struct hdc_list *next = hdcs_head->next;
        DC *dc = get_dc_ptr(hdcs_head->hdc);

        TRACE("hdc %p has interest in %p\n", hdcs_head->hdc, obj);
        if(dc)
        {
            PHYSDEV physdev = GET_DC_PHYSDEV( dc, pDeleteObject );
            physdev->funcs->pDeleteObject( physdev, obj );
            release_dc_ptr( dc );
        }
        HeapFree(GetProcessHeap(), 0, hdcs_head);
        hdcs_head = next;
    }

    TRACE("%p\n", obj );

    if (funcs && funcs->pDeleteObject) return funcs->pDeleteObject( obj );
    return TRUE;
}

/***********************************************************************
 *           GDI_hdc_using_object
 *
 * Call this if the dc requires DeleteObject notification
 */
void GDI_hdc_using_object(HGDIOBJ obj, HDC hdc)
{
    struct gdi_handle_entry *entry;
    struct hdc_list *phdc;

    TRACE("obj %p hdc %p\n", obj, hdc);

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( obj )) && !entry->system)
    {
        for (phdc = entry->hdcs; phdc; phdc = phdc->next)
            if (phdc->hdc == hdc) break;

        if (!phdc)
        {
            phdc = HeapAlloc(GetProcessHeap(), 0, sizeof(*phdc));
            phdc->hdc = hdc;
            phdc->next = entry->hdcs;
            entry->hdcs = phdc;
        }
    }
    LeaveCriticalSection( &gdi_section );
}

/***********************************************************************
 *           GDI_hdc_not_using_object
 *
 */
void GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc)
{
    struct gdi_handle_entry *entry;
    struct hdc_list **pphdc;

    TRACE("obj %p hdc %p\n", obj, hdc);

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( obj )) && !entry->system)
    {
        for (pphdc = &entry->hdcs; *pphdc; pphdc = &(*pphdc)->next)
            if ((*pphdc)->hdc == hdc)
            {
                struct hdc_list *phdc = *pphdc;
                *pphdc = phdc->next;
                HeapFree(GetProcessHeap(), 0, phdc);
                break;
            }
    }
    LeaveCriticalSection( &gdi_section );
}

/***********************************************************************
 *           GetStockObject    (GDI32.@)
 */
HGDIOBJ WINAPI GetStockObject( INT obj )
{
    if ((obj < 0) || (obj >= NB_STOCK_OBJECTS)) return 0;
    switch (obj)
    {
    case OEM_FIXED_FONT:
    case SYSTEM_FONT:
    case SYSTEM_FIXED_FONT:
    case DEFAULT_GUI_FONT:
        if (get_system_dpi() != 96) return scaled_stock_objects[obj];
        break;
    }
    return stock_objects[obj];
}


/***********************************************************************
 *           GetObjectA    (GDI32.@)
 */
INT WINAPI GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer )
{
    struct gdi_handle_entry *entry;
    const struct gdi_obj_funcs *funcs = NULL;
    INT result = 0;

    TRACE("%p %d %p\n", handle, count, buffer );

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle )))
    {
        funcs = entry->funcs;
        handle = entry_to_handle( entry );  /* make it a full handle */
    }
    LeaveCriticalSection( &gdi_section );

    if (funcs)
    {
        if (!funcs->pGetObjectA)
            SetLastError( ERROR_INVALID_HANDLE );
        else if (buffer && ((ULONG_PTR)buffer >> 16) == 0) /* catch apps getting argument order wrong */
            SetLastError( ERROR_NOACCESS );
        else
            result = funcs->pGetObjectA( handle, count, buffer );
    }
    return result;
}

/***********************************************************************
 *           GetObjectW    (GDI32.@)
 */
INT WINAPI GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer )
{
    struct gdi_handle_entry *entry;
    const struct gdi_obj_funcs *funcs = NULL;
    INT result = 0;

    TRACE("%p %d %p\n", handle, count, buffer );

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle )))
    {
        funcs = entry->funcs;
        handle = entry_to_handle( entry );  /* make it a full handle */
    }
    LeaveCriticalSection( &gdi_section );

    if (funcs)
    {
        if (!funcs->pGetObjectW)
            SetLastError( ERROR_INVALID_HANDLE );
        else if (buffer && ((ULONG_PTR)buffer >> 16) == 0) /* catch apps getting argument order wrong */
            SetLastError( ERROR_NOACCESS );
        else
            result = funcs->pGetObjectW( handle, count, buffer );
    }
    return result;
}

/***********************************************************************
 *           GetObjectType    (GDI32.@)
 */
DWORD WINAPI GetObjectType( HGDIOBJ handle )
{
    struct gdi_handle_entry *entry;
    DWORD result = 0;

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( handle ))) result = entry->type;
    LeaveCriticalSection( &gdi_section );

    TRACE("%p -> %u\n", handle, result );
    if (!result) SetLastError( ERROR_INVALID_HANDLE );
    return result;
}

/***********************************************************************
 *           GetCurrentObject    	(GDI32.@)
 *
 * Get the currently selected object of a given type in a device context.
 *
 * PARAMS
 *  hdc  [I] Device context to get the current object from
 *  type [I] Type of current object to get (OBJ_* defines from "wingdi.h")
 *
 * RETURNS
 *  Success: The current object of the given type selected in hdc.
 *  Failure: A NULL handle.
 *
 * NOTES
 * - only the following object types are supported:
 *| OBJ_PEN
 *| OBJ_BRUSH
 *| OBJ_PAL
 *| OBJ_FONT
 *| OBJ_BITMAP
 */
HGDIOBJ WINAPI GetCurrentObject(HDC hdc,UINT type)
{
    HGDIOBJ ret = 0;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return 0;

    switch (type) {
	case OBJ_EXTPEN: /* fall through */
	case OBJ_PEN:	 ret = dc->hPen; break;
	case OBJ_BRUSH:	 ret = dc->hBrush; break;
	case OBJ_PAL:	 ret = dc->hPalette; break;
	case OBJ_FONT:	 ret = dc->hFont; break;
	case OBJ_BITMAP: ret = dc->hBitmap; break;

	/* tests show that OBJ_REGION is explicitly ignored */
	case OBJ_REGION: break;
        default:
            /* the SDK only mentions those above */
            FIXME("(%p,%d): unknown type.\n",hdc,type);
	    break;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SelectObject    (GDI32.@)
 *
 * Select a Gdi object into a device context.
 *
 * PARAMS
 *  hdc  [I] Device context to associate the object with
 *  hObj [I] Gdi object to associate with hdc
 *
 * RETURNS
 *  Success: A non-NULL handle representing the previously selected object of
 *           the same type as hObj.
 *  Failure: A NULL object. If hdc is invalid, GetLastError() returns ERROR_INVALID_HANDLE.
 *           if hObj is not a valid object handle, no last error is set. In either
 *           case, hdc is unaffected by the call.
 */
HGDIOBJ WINAPI SelectObject( HDC hdc, HGDIOBJ hObj )
{
    struct gdi_handle_entry *entry;
    const struct gdi_obj_funcs *funcs = NULL;

    TRACE( "(%p,%p)\n", hdc, hObj );

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( hObj )))
    {
        funcs = entry->funcs;
        hObj = entry_to_handle( entry );  /* make it a full handle */
    }
    LeaveCriticalSection( &gdi_section );

    if (funcs && funcs->pSelectObject) return funcs->pSelectObject( hObj, hdc );
    return 0;
}


/***********************************************************************
 *           UnrealizeObject    (GDI32.@)
 */
BOOL WINAPI UnrealizeObject( HGDIOBJ obj )
{
    const struct gdi_obj_funcs *funcs = NULL;
    struct gdi_handle_entry *entry;

    EnterCriticalSection( &gdi_section );
    if ((entry = handle_entry( obj )))
    {
        funcs = entry->funcs;
        obj = entry_to_handle( entry );  /* make it a full handle */
    }
    LeaveCriticalSection( &gdi_section );

    if (funcs && funcs->pUnrealizeObject) return funcs->pUnrealizeObject( obj );
    return funcs != NULL;
}


/* Solid colors to enumerate */
static const COLORREF solid_colors[] =
{ RGB(0x00,0x00,0x00), RGB(0xff,0xff,0xff),
RGB(0xff,0x00,0x00), RGB(0x00,0xff,0x00),
RGB(0x00,0x00,0xff), RGB(0xff,0xff,0x00),
RGB(0xff,0x00,0xff), RGB(0x00,0xff,0xff),
RGB(0x80,0x00,0x00), RGB(0x00,0x80,0x00),
RGB(0x80,0x80,0x00), RGB(0x00,0x00,0x80),
RGB(0x80,0x00,0x80), RGB(0x00,0x80,0x80),
RGB(0x80,0x80,0x80), RGB(0xc0,0xc0,0xc0)
};


/***********************************************************************
 *           EnumObjects    (GDI32.@)
 */
INT WINAPI EnumObjects( HDC hdc, INT nObjType,
                            GOBJENUMPROC lpEnumFunc, LPARAM lParam )
{
    UINT i;
    INT retval = 0;
    LOGPEN pen;
    LOGBRUSH brush;

    TRACE("%p %d %p %08lx\n", hdc, nObjType, lpEnumFunc, lParam );
    switch(nObjType)
    {
    case OBJ_PEN:
        /* Enumerate solid pens */
        for (i = 0; i < ARRAY_SIZE( solid_colors ); i++)
        {
            pen.lopnStyle   = PS_SOLID;
            pen.lopnWidth.x = 1;
            pen.lopnWidth.y = 0;
            pen.lopnColor   = solid_colors[i];
            retval = lpEnumFunc( &pen, lParam );
            TRACE("solid pen %08x, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }
        break;

    case OBJ_BRUSH:
        /* Enumerate solid brushes */
        for (i = 0; i < ARRAY_SIZE( solid_colors ); i++)
        {
            brush.lbStyle = BS_SOLID;
            brush.lbColor = solid_colors[i];
            brush.lbHatch = 0;
            retval = lpEnumFunc( &brush, lParam );
            TRACE("solid brush %08x, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }

        /* Now enumerate hatched brushes */
        if (retval) for (i = HS_HORIZONTAL; i <= HS_DIAGCROSS; i++)
        {
            brush.lbStyle = BS_HATCHED;
            brush.lbColor = RGB(0,0,0);
            brush.lbHatch = i;
            retval = lpEnumFunc( &brush, lParam );
            TRACE("hatched brush %d, ret=%d\n",
                         i, retval);
            if (!retval) break;
        }
        break;

    default:
        /* FIXME: implement Win32 types */
        WARN("(%d): Invalid type\n", nObjType );
        break;
    }
    return retval;
}


/***********************************************************************
 *           SetObjectOwner    (GDI32.@)
 */
void WINAPI SetObjectOwner( HGDIOBJ handle, HANDLE owner )
{
    /* Nothing to do */
}

/***********************************************************************
 *           GdiInitializeLanguagePack    (GDI32.@)
 */
DWORD WINAPI GdiInitializeLanguagePack( DWORD arg )
{
    FIXME("stub\n");
    return 0;
}

/***********************************************************************
 *           GdiFlush    (GDI32.@)
 */
BOOL WINAPI GdiFlush(void)
{
    return TRUE;  /* FIXME */
}


/***********************************************************************
 *           GdiGetBatchLimit    (GDI32.@)
 */
DWORD WINAPI GdiGetBatchLimit(void)
{
    return 1;  /* FIXME */
}


/***********************************************************************
 *           GdiSetBatchLimit    (GDI32.@)
 */
DWORD WINAPI GdiSetBatchLimit( DWORD limit )
{
    return 1; /* FIXME */
}


/*******************************************************************
 *      GetColorAdjustment [GDI32.@]
 *
 *
 */
BOOL WINAPI GetColorAdjustment(HDC hdc, LPCOLORADJUSTMENT lpca)
{
    FIXME("stub\n");
    return FALSE;
}

/*******************************************************************
 *      GdiComment [GDI32.@]
 *
 *
 */
BOOL WINAPI GdiComment(HDC hdc, UINT cbSize, const BYTE *lpData)
{
    DC *dc = get_dc_ptr(hdc);
    BOOL ret = FALSE;

    if(dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pGdiComment );
        ret = physdev->funcs->pGdiComment( physdev, cbSize, lpData );
        release_dc_ptr( dc );
    }
    return ret;
}

/*******************************************************************
 *      SetColorAdjustment [GDI32.@]
 *
 *
 */
BOOL WINAPI SetColorAdjustment(HDC hdc, const COLORADJUSTMENT* lpca)
{
    FIXME("stub\n");
    return FALSE;
}
