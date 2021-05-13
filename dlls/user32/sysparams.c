/*
 * System parameters functions
 *
 * Copyright 1994 Alexandre Julliard
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
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/wingdi16.h"
#include "winerror.h"

#include "initguid.h"
#include "d3dkmdt.h"
#include "devguid.h"
#include "setupapi.h"
#include "controls.h"
#include "win.h"
#include "user_private.h"
#include "wine/gdi_driver.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);

/* System parameter indexes */
enum spi_index
{
    SPI_SETWORKAREA_IDX,
    SPI_INDEX_COUNT
};

/**
 * Names of the registry subkeys of HKEY_CURRENT_USER key and value names
 * for the system parameters.
 */

/* the various registry keys that are used to store parameters */
enum parameter_key
{
    COLORS_KEY,
    DESKTOP_KEY,
    KEYBOARD_KEY,
    MOUSE_KEY,
    METRICS_KEY,
    SOUND_KEY,
    VERSION_KEY,
    SHOWSOUNDS_KEY,
    KEYBOARDPREF_KEY,
    SCREENREADER_KEY,
    AUDIODESC_KEY,
    NB_PARAM_KEYS
};

static const WCHAR *parameter_key_names[NB_PARAM_KEYS] =
{
    L"Control Panel\\Colors",
    L"Control Panel\\Desktop",
    L"Control Panel\\Keyboard",
    L"Control Panel\\Mouse",
    L"Control Panel\\Desktop\\WindowMetrics",
    L"Control Panel\\Sound",
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
    L"Control Panel\\Accessibility\\ShowSounds",
    L"Control Panel\\Accessibility\\Keyboard Preference",
    L"Control Panel\\Accessibility\\Blind Access",
    L"Control Panel\\Accessibility\\AudioDescription",
};

DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);
DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_GPU_LUID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 1);
DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_OUTPUT_ID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 2);

/* Wine specific monitor properties */
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_STATEFLAGS, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 2);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_RCMONITOR, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 3);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_RCWORK, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 4);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_ADAPTERNAME, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 5);

#define NULLDRV_DEFAULT_HMONITOR ((HMONITOR)(UINT_PTR)(0x10000 + 1))

/* Cached monitor information */
static MONITORINFOEXW *monitors;
static UINT monitor_count;
static FILETIME last_query_monitors_time;
static CRITICAL_SECTION monitors_section;
static CRITICAL_SECTION_DEBUG monitors_critsect_debug =
{
    0, 0, &monitors_section,
    { &monitors_critsect_debug.ProcessLocksList, &monitors_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": monitors_section") }
};
static CRITICAL_SECTION monitors_section = { &monitors_critsect_debug, -1 , 0, 0, 0, 0 };

static HDC display_dc;
static CRITICAL_SECTION display_dc_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &display_dc_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": display_dc_section") }
};
static CRITICAL_SECTION display_dc_section = { &critsect_debug, -1 ,0, 0, 0, 0 };


/* Indicators whether system parameter value is loaded */
static char spi_loaded[SPI_INDEX_COUNT];

static BOOL notify_change = TRUE;

/* System parameters storage */
static RECT work_area;
static UINT system_dpi;
static DPI_AWARENESS dpi_awareness;
static DPI_AWARENESS default_awareness = DPI_AWARENESS_UNAWARE;

static HKEY volatile_base_key;
static HKEY video_key;

union sysparam_all_entry;

struct sysparam_entry
{
    BOOL             (*get)( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi );
    BOOL             (*set)( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags );
    BOOL             (*init)( union sysparam_all_entry *entry );
    enum parameter_key base_key;
    const WCHAR       *regval;
    enum parameter_key mirror_key;
    const WCHAR       *mirror;
    BOOL               loaded;
};

struct sysparam_uint_entry
{
    struct sysparam_entry hdr;
    UINT                  val;
};

struct sysparam_bool_entry
{
    struct sysparam_entry hdr;
    BOOL                  val;
};

struct sysparam_dword_entry
{
    struct sysparam_entry hdr;
    DWORD                 val;
};

struct sysparam_rgb_entry
{
    struct sysparam_entry hdr;
    COLORREF              val;
    HBRUSH                brush;
    HPEN                  pen;
};

struct sysparam_binary_entry
{
    struct sysparam_entry hdr;
    void                 *ptr;
    size_t                size;
};

struct sysparam_path_entry
{
    struct sysparam_entry hdr;
    WCHAR                 path[MAX_PATH];
};

struct sysparam_font_entry
{
    struct sysparam_entry hdr;
    UINT                  weight;
    LOGFONTW              val;
    WCHAR                 fullname[LF_FACESIZE];
};

struct sysparam_pref_entry
{
    struct sysparam_entry hdr;
    struct sysparam_binary_entry *parent;
    UINT                  offset;
    UINT                  mask;
};

union sysparam_all_entry
{
    struct sysparam_entry        hdr;
    struct sysparam_uint_entry   uint;
    struct sysparam_bool_entry   bool;
    struct sysparam_dword_entry  dword;
    struct sysparam_rgb_entry    rgb;
    struct sysparam_binary_entry bin;
    struct sysparam_path_entry   path;
    struct sysparam_font_entry   font;
    struct sysparam_pref_entry   pref;
};

static void SYSPARAMS_LogFont16To32W( const LOGFONT16 *font16, LPLOGFONTW font32 )
{
    font32->lfHeight = font16->lfHeight;
    font32->lfWidth = font16->lfWidth;
    font32->lfEscapement = font16->lfEscapement;
    font32->lfOrientation = font16->lfOrientation;
    font32->lfWeight = font16->lfWeight;
    font32->lfItalic = font16->lfItalic;
    font32->lfUnderline = font16->lfUnderline;
    font32->lfStrikeOut = font16->lfStrikeOut;
    font32->lfCharSet = font16->lfCharSet;
    font32->lfOutPrecision = font16->lfOutPrecision;
    font32->lfClipPrecision = font16->lfClipPrecision;
    font32->lfQuality = font16->lfQuality;
    font32->lfPitchAndFamily = font16->lfPitchAndFamily;
    MultiByteToWideChar( CP_ACP, 0, font16->lfFaceName, -1, font32->lfFaceName, LF_FACESIZE );
    font32->lfFaceName[LF_FACESIZE-1] = 0;
}

static void SYSPARAMS_LogFont32WTo32A( const LOGFONTW* font32W, LPLOGFONTA font32A )
{
    font32A->lfHeight = font32W->lfHeight;
    font32A->lfWidth = font32W->lfWidth;
    font32A->lfEscapement = font32W->lfEscapement;
    font32A->lfOrientation = font32W->lfOrientation;
    font32A->lfWeight = font32W->lfWeight;
    font32A->lfItalic = font32W->lfItalic;
    font32A->lfUnderline = font32W->lfUnderline;
    font32A->lfStrikeOut = font32W->lfStrikeOut;
    font32A->lfCharSet = font32W->lfCharSet;
    font32A->lfOutPrecision = font32W->lfOutPrecision;
    font32A->lfClipPrecision = font32W->lfClipPrecision;
    font32A->lfQuality = font32W->lfQuality;
    font32A->lfPitchAndFamily = font32W->lfPitchAndFamily;
    WideCharToMultiByte( CP_ACP, 0, font32W->lfFaceName, -1, font32A->lfFaceName, LF_FACESIZE, NULL, NULL );
    font32A->lfFaceName[LF_FACESIZE-1] = 0;
}

static void SYSPARAMS_LogFont32ATo32W( const LOGFONTA* font32A, LPLOGFONTW font32W )
{
    font32W->lfHeight = font32A->lfHeight;
    font32W->lfWidth = font32A->lfWidth;
    font32W->lfEscapement = font32A->lfEscapement;
    font32W->lfOrientation = font32A->lfOrientation;
    font32W->lfWeight = font32A->lfWeight;
    font32W->lfItalic = font32A->lfItalic;
    font32W->lfUnderline = font32A->lfUnderline;
    font32W->lfStrikeOut = font32A->lfStrikeOut;
    font32W->lfCharSet = font32A->lfCharSet;
    font32W->lfOutPrecision = font32A->lfOutPrecision;
    font32W->lfClipPrecision = font32A->lfClipPrecision;
    font32W->lfQuality = font32A->lfQuality;
    font32W->lfPitchAndFamily = font32A->lfPitchAndFamily;
    MultiByteToWideChar( CP_ACP, 0, font32A->lfFaceName, -1, font32W->lfFaceName, LF_FACESIZE );
    font32W->lfFaceName[LF_FACESIZE-1] = 0;
}

static void SYSPARAMS_NonClientMetrics32WTo32A( const NONCLIENTMETRICSW* lpnm32W, LPNONCLIENTMETRICSA lpnm32A )
{
    lpnm32A->iBorderWidth	= lpnm32W->iBorderWidth;
    lpnm32A->iScrollWidth	= lpnm32W->iScrollWidth;
    lpnm32A->iScrollHeight	= lpnm32W->iScrollHeight;
    lpnm32A->iCaptionWidth	= lpnm32W->iCaptionWidth;
    lpnm32A->iCaptionHeight	= lpnm32W->iCaptionHeight;
    SYSPARAMS_LogFont32WTo32A(  &lpnm32W->lfCaptionFont,	&lpnm32A->lfCaptionFont );
    lpnm32A->iSmCaptionWidth	= lpnm32W->iSmCaptionWidth;
    lpnm32A->iSmCaptionHeight	= lpnm32W->iSmCaptionHeight;
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfSmCaptionFont,	&lpnm32A->lfSmCaptionFont );
    lpnm32A->iMenuWidth		= lpnm32W->iMenuWidth;
    lpnm32A->iMenuHeight	= lpnm32W->iMenuHeight;
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfMenuFont,		&lpnm32A->lfMenuFont );
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfStatusFont,		&lpnm32A->lfStatusFont );
    SYSPARAMS_LogFont32WTo32A( &lpnm32W->lfMessageFont,		&lpnm32A->lfMessageFont );
    if (lpnm32A->cbSize > FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth))
    {
        if (lpnm32W->cbSize > FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth))
            lpnm32A->iPaddedBorderWidth = lpnm32W->iPaddedBorderWidth;
        else
            lpnm32A->iPaddedBorderWidth = 0;
    }
}

static void SYSPARAMS_NonClientMetrics32ATo32W( const NONCLIENTMETRICSA* lpnm32A, LPNONCLIENTMETRICSW lpnm32W )
{
    lpnm32W->iBorderWidth	= lpnm32A->iBorderWidth;
    lpnm32W->iScrollWidth	= lpnm32A->iScrollWidth;
    lpnm32W->iScrollHeight	= lpnm32A->iScrollHeight;
    lpnm32W->iCaptionWidth	= lpnm32A->iCaptionWidth;
    lpnm32W->iCaptionHeight	= lpnm32A->iCaptionHeight;
    SYSPARAMS_LogFont32ATo32W(  &lpnm32A->lfCaptionFont,	&lpnm32W->lfCaptionFont );
    lpnm32W->iSmCaptionWidth	= lpnm32A->iSmCaptionWidth;
    lpnm32W->iSmCaptionHeight	= lpnm32A->iSmCaptionHeight;
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfSmCaptionFont,	&lpnm32W->lfSmCaptionFont );
    lpnm32W->iMenuWidth		= lpnm32A->iMenuWidth;
    lpnm32W->iMenuHeight	= lpnm32A->iMenuHeight;
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfMenuFont,		&lpnm32W->lfMenuFont );
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfStatusFont,		&lpnm32W->lfStatusFont );
    SYSPARAMS_LogFont32ATo32W( &lpnm32A->lfMessageFont,		&lpnm32W->lfMessageFont );
    if (lpnm32W->cbSize > FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth))
    {
        if (lpnm32A->cbSize > FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth))
            lpnm32W->iPaddedBorderWidth = lpnm32A->iPaddedBorderWidth;
        else
            lpnm32W->iPaddedBorderWidth = 0;
    }
}


/* Helper functions to retrieve monitors info */

struct monitor_info
{
    int count;
    RECT primary_rect;
    RECT virtual_rect;
};

static BOOL CALLBACK monitor_info_proc( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    MONITORINFO mi;
    struct monitor_info *info = (struct monitor_info *)lp;
    info->count++;
    UnionRect( &info->virtual_rect, &info->virtual_rect, rect );
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW( monitor, &mi ) && (mi.dwFlags & MONITORINFOF_PRIMARY))
        info->primary_rect = mi.rcMonitor;
    return TRUE;
}

static void get_monitors_info( struct monitor_info *info )
{
    info->count = 0;
    SetRectEmpty( &info->primary_rect );
    SetRectEmpty( &info->virtual_rect );
    EnumDisplayMonitors( 0, NULL, monitor_info_proc, (LPARAM)info );
}

RECT get_virtual_screen_rect(void)
{
    struct monitor_info info;
    get_monitors_info( &info );
    return info.virtual_rect;
}

static BOOL get_primary_adapter(WCHAR *name)
{
    DISPLAY_DEVICEW dd;
    DWORD i;

    dd.cb = sizeof(dd);
    for (i = 0; EnumDisplayDevicesW(NULL, i, &dd, 0); ++i)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyW(name, dd.DeviceName);
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL is_valid_adapter_name(const WCHAR *name)
{
    long int adapter_idx;
    WCHAR *end;

    if (wcsnicmp(name, L"\\\\.\\DISPLAY", lstrlenW(L"\\\\.\\DISPLAY")))
        return FALSE;

    adapter_idx = wcstol(name + lstrlenW(L"\\\\.\\DISPLAY"), &end, 10);
    if (*end || adapter_idx < 1)
        return FALSE;

    return TRUE;
}

/* get text metrics and/or "average" char width of the specified logfont 
 * for the specified dc */
static void get_text_metr_size( HDC hdc, LOGFONTW *plf, TEXTMETRICW * ptm, UINT *psz)
{
    HFONT hfont, hfontsav;
    TEXTMETRICW tm;
    if( !ptm) ptm = &tm;
    hfont = CreateFontIndirectW( plf);
    if( !hfont || ( hfontsav = SelectObject( hdc, hfont)) == NULL ) {
        ptm->tmHeight = -1;
        if( psz) *psz = 10;
        if( hfont) DeleteObject( hfont);
        return;
    }
    GetTextMetricsW( hdc, ptm);
    if( psz)
        if( !(*psz = GdiGetCharDimensions( hdc, ptm, NULL)))
            *psz = 10;
    SelectObject( hdc, hfontsav);
    DeleteObject( hfont);
}

/***********************************************************************
 *           SYSPARAMS_NotifyChange
 *
 * Sends notification about system parameter update.
 */
static void SYSPARAMS_NotifyChange( UINT uiAction, UINT fWinIni )
{
    static const WCHAR emptyW[1];

    if (notify_change)
    {
        if (fWinIni & SPIF_UPDATEINIFILE)
        {
            if (fWinIni & (SPIF_SENDWININICHANGE | SPIF_SENDCHANGE))
                SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE,
                                    uiAction, (LPARAM) emptyW,
                                    SMTO_ABORTIFHUNG, 2000, NULL );
        }
        else
        {
            /* FIXME notify other wine processes with internal message */
        }
    }
}

/* retrieve the cached base keys for a given entry */
static BOOL get_base_keys( enum parameter_key index, HKEY *base_key, HKEY *volatile_key )
{
    static HKEY base_keys[NB_PARAM_KEYS];
    static HKEY volatile_keys[NB_PARAM_KEYS];
    HKEY key;

    if (!base_keys[index] && base_key)
    {
        if (RegCreateKeyW( HKEY_CURRENT_USER, parameter_key_names[index], &key )) return FALSE;
        if (InterlockedCompareExchangePointer( (void **)&base_keys[index], key, 0 ))
            RegCloseKey( key );
    }
    if (!volatile_keys[index] && volatile_key)
    {
        if (RegCreateKeyExW( volatile_base_key, parameter_key_names[index],
                             0, 0, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &key, 0 )) return FALSE;
        if (InterlockedCompareExchangePointer( (void **)&volatile_keys[index], key, 0 ))
            RegCloseKey( key );
    }
    if (base_key) *base_key = base_keys[index];
    if (volatile_key) *volatile_key = volatile_keys[index];
    return TRUE;
}

/* load a value to a registry entry */
static DWORD load_entry( struct sysparam_entry *entry, void *data, DWORD size )
{
    DWORD type, count;
    HKEY base_key, volatile_key;

    if (!get_base_keys( entry->base_key, &base_key, &volatile_key )) return FALSE;

    count = size;
    if (RegQueryValueExW( volatile_key, entry->regval, NULL, &type, data, &count ))
    {
        count = size;
        if (RegQueryValueExW( base_key, entry->regval, NULL, &type, data, &count )) count = 0;
    }
    /* make sure strings are null-terminated */
    if (size && count == size && type == REG_SZ) ((WCHAR *)data)[count / sizeof(WCHAR) - 1] = 0;
    entry->loaded = TRUE;
    return count;
}

/* save a value to a registry entry */
static BOOL save_entry( const struct sysparam_entry *entry, const void *data, DWORD size,
                        DWORD type, UINT flags )
{
    HKEY base_key, volatile_key;

    if (flags & SPIF_UPDATEINIFILE)
    {
        if (!get_base_keys( entry->base_key, &base_key, &volatile_key )) return FALSE;
        if (RegSetValueExW( base_key, entry->regval, 0, type, data, size )) return FALSE;
        RegDeleteValueW( volatile_key, entry->regval );

        if (entry->mirror && get_base_keys( entry->mirror_key, &base_key, NULL ))
            RegSetValueExW( base_key, entry->mirror, 0, type, data, size );
    }
    else
    {
        if (!get_base_keys( entry->base_key, NULL, &volatile_key )) return FALSE;
        if (RegSetValueExW( volatile_key, entry->regval, 0, type, data, size )) return FALSE;
    }
    return TRUE;
}

/* save a string value to a registry entry */
static BOOL save_entry_string( const struct sysparam_entry *entry, const WCHAR *str, UINT flags )
{
    return save_entry( entry, str, (lstrlenW(str) + 1) * sizeof(WCHAR), REG_SZ, flags );
}

/* initialize an entry in the registry if missing */
static BOOL init_entry( struct sysparam_entry *entry, const void *data, DWORD size, DWORD type )
{
    HKEY base_key;

    if (!get_base_keys( entry->base_key, &base_key, NULL )) return FALSE;
    if (!RegQueryValueExW( base_key, entry->regval, NULL, NULL, NULL, NULL )) return TRUE;
    if (RegSetValueExW( base_key, entry->regval, 0, type, data, size )) return FALSE;
    if (entry->mirror && get_base_keys( entry->mirror_key, &base_key, NULL ))
        RegSetValueExW( base_key, entry->mirror, 0, type, data, size );
    entry->loaded = TRUE;
    return TRUE;
}

/* initialize a string value in the registry if missing */
static BOOL init_entry_string( struct sysparam_entry *entry, const WCHAR *str )
{
    return init_entry( entry, str, (lstrlenW(str) + 1) * sizeof(WCHAR), REG_SZ );
}

HDC get_display_dc(void)
{
    EnterCriticalSection( &display_dc_section );
    if (!display_dc)
    {
        HDC dc;

        LeaveCriticalSection( &display_dc_section );
        dc = CreateDCW( L"DISPLAY", NULL, NULL, NULL );
        EnterCriticalSection( &display_dc_section );
        if (display_dc)
            DeleteDC(dc);
        else
            display_dc = dc;
    }
    return display_dc;
}

void release_display_dc( HDC hdc )
{
    LeaveCriticalSection( &display_dc_section );
}

static HANDLE get_display_device_init_mutex( void )
{
    HANDLE mutex = CreateMutexW( NULL, FALSE, L"display_device_init" );

    WaitForSingleObject( mutex, INFINITE );
    return mutex;
}

static void release_display_device_init_mutex( HANDLE mutex )
{
    ReleaseMutex( mutex );
    CloseHandle( mutex );
}

/* Wait until graphics driver is loaded by explorer */
void wait_graphics_driver_ready(void)
{
    static BOOL ready = FALSE;

    if (!ready)
    {
        SendMessageW( GetDesktopWindow(), WM_NULL, 0, 0 );
        ready = TRUE;
    }
}

/* map value from system dpi to standard 96 dpi for storing in the registry */
static int map_from_system_dpi( int val )
{
    return MulDiv( val, USER_DEFAULT_SCREEN_DPI, GetDpiForSystem() );
}

/* map value from 96 dpi to system or custom dpi */
static int map_to_dpi( int val, UINT dpi )
{
    if (!dpi) dpi = GetDpiForSystem();
    return MulDiv( val, dpi, USER_DEFAULT_SCREEN_DPI );
}

static INT CALLBACK real_fontname_proc(const LOGFONTW *lf, const TEXTMETRICW *ntm, DWORD type, LPARAM lparam)
{
    const ENUMLOGFONTW *elf = (const ENUMLOGFONTW *)lf;
    WCHAR *fullname = (WCHAR *)lparam;

    lstrcpynW( fullname, elf->elfFullName, LF_FACESIZE );
    return 0;
}

static void get_real_fontname( LOGFONTW *lf, WCHAR fullname[LF_FACESIZE] )
{
    HDC hdc = get_display_dc();
    lstrcpyW( fullname, lf->lfFaceName );
    EnumFontFamiliesExW( hdc, lf, real_fontname_proc, (LPARAM)fullname, 0 );
    release_display_dc( hdc );
}

/* adjust some of the raw values found in the registry */
static void normalize_nonclientmetrics( NONCLIENTMETRICSW *pncm)
{
    TEXTMETRICW tm;
    HDC hdc = get_display_dc();

    if( pncm->iBorderWidth < 1) pncm->iBorderWidth = 1;
    if( pncm->iCaptionWidth < 8) pncm->iCaptionWidth = 8;
    if( pncm->iScrollWidth < 8) pncm->iScrollWidth = 8;
    if( pncm->iScrollHeight < 8) pncm->iScrollHeight = 8;

    /* adjust some heights to the corresponding font */
    get_text_metr_size( hdc, &pncm->lfMenuFont, &tm, NULL);
    pncm->iMenuHeight = max( pncm->iMenuHeight, 2 + tm.tmHeight + tm.tmExternalLeading );
    get_text_metr_size( hdc, &pncm->lfCaptionFont, &tm, NULL);
    pncm->iCaptionHeight = max( pncm->iCaptionHeight, 2 + tm.tmHeight);
    get_text_metr_size( hdc, &pncm->lfSmCaptionFont, &tm, NULL);
    pncm->iSmCaptionHeight = max( pncm->iSmCaptionHeight, 2 + tm.tmHeight);
    release_display_dc( hdc );
}

static BOOL CALLBACK enum_monitors( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    MONITORINFO mi;

    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW( monitor, &mi ) && (mi.dwFlags & MONITORINFOF_PRIMARY))
    {
        LPRECT work = (LPRECT)lp;
        *work = mi.rcWork;
        return FALSE;
    }
    return TRUE;
}

/* load a uint parameter from the registry */
static BOOL get_uint_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->uint.val = wcstol( buf, NULL, 10 );
    }
    *(UINT *)ptr_param = entry->uint.val;
    return TRUE;
}

/* set a uint parameter in the registry */
static BOOL set_uint_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[32];

    wsprintfW( buf, L"%u", int_param );
    if (!save_entry_string( &entry->hdr, buf, flags )) return FALSE;
    entry->uint.val = int_param;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a uint parameter */
static BOOL init_uint_entry( union sysparam_all_entry *entry )
{
    WCHAR buf[32];

    wsprintfW( buf, L"%u", entry->uint.val );
    return init_entry_string( &entry->hdr, buf );
}

/* set an int parameter in the registry */
static BOOL set_int_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[32];

    wsprintfW( buf, L"%d", int_param );
    if (!save_entry_string( &entry->hdr, buf, flags )) return FALSE;
    entry->uint.val = int_param;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize an int parameter */
static BOOL init_int_entry( union sysparam_all_entry *entry )
{
    WCHAR buf[32];

    wsprintfW( buf, L"%d", entry->uint.val );
    return init_entry_string( &entry->hdr, buf );
}

/* load a twips parameter from the registry */
static BOOL get_twips_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    int val;

    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->uint.val = wcstol( buf, NULL, 10 );
    }

    /* Dimensions are quoted as being "twips" values if negative and pixels if positive.
     * One inch is 1440 twips.
     * See for example
     *       Technical Reference to the Windows 2000 Registry ->
     *       HKEY_CURRENT_USER -> Control Panel -> Desktop -> WindowMetrics
     */
    val = entry->uint.val;
    if (val < 0)
        val = MulDiv( -val, dpi, 1440 );
    else
        val = map_to_dpi( val, dpi );

    *(int *)ptr_param = val;
    return TRUE;
}

/* set a twips parameter in the registry */
static BOOL set_twips_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    int val = int_param;
    if (val > 0) val = map_from_system_dpi( val );
    return set_int_entry( entry, val, ptr_param, flags );
}

/* load a bool parameter from the registry */
static BOOL get_bool_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->bool.val = wcstol( buf, NULL, 10 ) != 0;
    }
    *(UINT *)ptr_param = entry->bool.val;
    return TRUE;
}

/* set a bool parameter in the registry */
static BOOL set_bool_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[32];

    wsprintfW( buf, L"%u", int_param != 0 );
    if (!save_entry_string( &entry->hdr, buf, flags )) return FALSE;
    entry->bool.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a bool parameter */
static BOOL init_bool_entry( union sysparam_all_entry *entry )
{
    WCHAR buf[32];

    wsprintfW( buf, L"%u", entry->bool.val != 0 );
    return init_entry_string( &entry->hdr, buf );
}

/* load a bool parameter using Yes/No strings from the registry */
static BOOL get_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->bool.val = !lstrcmpiW( L"Yes", buf );
    }
    *(UINT *)ptr_param = entry->bool.val;
    return TRUE;
}

/* set a bool parameter using Yes/No strings from the registry */
static BOOL set_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    const WCHAR *str = int_param ? L"Yes" : L"No";

    if (!save_entry_string( &entry->hdr, str, flags )) return FALSE;
    entry->bool.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a bool parameter using Yes/No strings */
static BOOL init_yesno_entry( union sysparam_all_entry *entry )
{
    return init_entry_string( &entry->hdr, entry->bool.val ? L"Yes" : L"No" );
}

/* load a dword (binary) parameter from the registry */
static BOOL get_dword_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        DWORD val;
        if (load_entry( &entry->hdr, &val, sizeof(val) ) == sizeof(DWORD)) entry->dword.val = val;
    }
    *(DWORD *)ptr_param = entry->dword.val;
    return TRUE;
}

/* set a dword (binary) parameter in the registry */
static BOOL set_dword_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    DWORD val = PtrToUlong( ptr_param );

    if (!save_entry( &entry->hdr, &val, sizeof(val), REG_DWORD, flags )) return FALSE;
    entry->dword.val = val;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a dword parameter */
static BOOL init_dword_entry( union sysparam_all_entry *entry )
{
    return init_entry( &entry->hdr, &entry->dword.val, sizeof(entry->dword.val), REG_DWORD );
}

/* load an RGB parameter from the registry */
static BOOL get_rgb_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (load_entry( &entry->hdr, buf, sizeof(buf) ))
        {
            DWORD r, g, b;
            WCHAR *end, *str = buf;

            r = wcstoul( str, &end, 10 );
            if (end == str || !*end) goto done;
            str = end + 1;
            g = wcstoul( str, &end, 10 );
            if (end == str || !*end) goto done;
            str = end + 1;
            b = wcstoul( str, &end, 10 );
            if (end == str) goto done;
            if (r > 255 || g > 255 || b > 255) goto done;
            entry->rgb.val = RGB( r, g, b );
        }
    }
done:
    *(COLORREF *)ptr_param = entry->rgb.val;
    return TRUE;
}

/* set an RGB parameter in the registry */
static BOOL set_rgb_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[32];
    HBRUSH brush;
    HPEN pen;

    wsprintfW( buf, L"%u %u %u", GetRValue(int_param), GetGValue(int_param), GetBValue(int_param) );
    if (!save_entry_string( &entry->hdr, buf, flags )) return FALSE;
    entry->rgb.val = int_param;
    entry->hdr.loaded = TRUE;
    if ((brush = InterlockedExchangePointer( (void **)&entry->rgb.brush, 0 )))
    {
        __wine_make_gdi_object_system( brush, FALSE );
        DeleteObject( brush );
    }
    if ((pen = InterlockedExchangePointer( (void **)&entry->rgb.pen, 0 )))
    {
        __wine_make_gdi_object_system( pen, FALSE );
        DeleteObject( pen );
    }
    return TRUE;
}

/* initialize an RGB parameter */
static BOOL init_rgb_entry( union sysparam_all_entry *entry )
{
    WCHAR buf[32];

    wsprintfW( buf, L"%u %u %u", GetRValue(entry->rgb.val), GetGValue(entry->rgb.val), GetBValue(entry->rgb.val) );
    return init_entry_string( &entry->hdr, buf );
}

/* load a font (binary) parameter from the registry */
static BOOL get_font_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    LOGFONTW font;

    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        switch (load_entry( &entry->hdr, &font, sizeof(font) ))
        {
        case sizeof(font):
            if (font.lfHeight > 0) /* positive height value means points ( inch/72 ) */
                font.lfHeight = -MulDiv( font.lfHeight, USER_DEFAULT_SCREEN_DPI, 72 );
            entry->font.val = font;
            break;
        case sizeof(LOGFONT16): /* win9x-winME format */
            SYSPARAMS_LogFont16To32W( (LOGFONT16 *)&font, &entry->font.val );
            if (entry->font.val.lfHeight > 0)
                entry->font.val.lfHeight = -MulDiv( entry->font.val.lfHeight, USER_DEFAULT_SCREEN_DPI, 72 );
            break;
        default:
            WARN( "Unknown format in key %s value %s\n",
                  debugstr_w( parameter_key_names[entry->hdr.base_key] ),
                  debugstr_w( entry->hdr.regval ));
            /* fall through */
        case 0: /* use the default GUI font */
            GetObjectW( GetStockObject( DEFAULT_GUI_FONT ), sizeof(font), &font );
            font.lfHeight = map_from_system_dpi( font.lfHeight );
            font.lfWeight = entry->font.weight;
            entry->font.val = font;
            break;
        }
        get_real_fontname( &entry->font.val, entry->font.fullname );
        entry->hdr.loaded = TRUE;
    }
    font = entry->font.val;
    font.lfHeight = map_to_dpi( font.lfHeight, dpi );
    lstrcpyW( font.lfFaceName, entry->font.fullname );
    *(LOGFONTW *)ptr_param = font;
    return TRUE;
}

/* set a font (binary) parameter in the registry */
static BOOL set_font_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    LOGFONTW font;
    WCHAR *ptr;

    memcpy( &font, ptr_param, sizeof(font) );
    /* zero pad the end of lfFaceName so we don't save uninitialised data */
    ptr = wmemchr( font.lfFaceName, 0, LF_FACESIZE );
    if (ptr) memset( ptr, 0, (font.lfFaceName + LF_FACESIZE - ptr) * sizeof(WCHAR) );
    if (font.lfHeight < 0) font.lfHeight = map_from_system_dpi( font.lfHeight );

    if (!save_entry( &entry->hdr, &font, sizeof(font), REG_BINARY, flags )) return FALSE;
    entry->font.val = font;
    get_real_fontname( &entry->font.val, entry->font.fullname );
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a font (binary) parameter */
static BOOL init_font_entry( union sysparam_all_entry *entry )
{
    GetObjectW( GetStockObject( DEFAULT_GUI_FONT ), sizeof(entry->font.val), &entry->font.val );
    entry->font.val.lfHeight = map_from_system_dpi( entry->font.val.lfHeight );
    entry->font.val.lfWeight = entry->font.weight;
    get_real_fontname( &entry->font.val, entry->font.fullname );
    return init_entry( &entry->hdr, &entry->font.val, sizeof(entry->font.val), REG_BINARY );
}

/* get a path parameter in the registry */
static BOOL get_path_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buffer[MAX_PATH];

        if (load_entry( &entry->hdr, buffer, sizeof(buffer) ))
            lstrcpynW( entry->path.path, buffer, MAX_PATH );
    }
    lstrcpynW( ptr_param, entry->path.path, int_param );
    return TRUE;
}

/* set a path parameter in the registry */
static BOOL set_path_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buffer[MAX_PATH];
    BOOL ret;

    lstrcpynW( buffer, ptr_param, MAX_PATH );
    ret = save_entry_string( &entry->hdr, buffer, flags );
    if (ret)
    {
        lstrcpyW( entry->path.path, buffer );
        entry->hdr.loaded = TRUE;
    }
    return ret;
}

/* initialize a path parameter */
static BOOL init_path_entry( union sysparam_all_entry *entry )
{
    return init_entry_string( &entry->hdr, entry->path.path );
}

/* get a binary parameter in the registry */
static BOOL get_binary_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        void *buffer = HeapAlloc( GetProcessHeap(), 0, entry->bin.size );
        DWORD len = load_entry( &entry->hdr, buffer, entry->bin.size );

        if (len)
        {
            memcpy( entry->bin.ptr, buffer, entry->bin.size );
            memset( (char *)entry->bin.ptr + len, 0, entry->bin.size - len );
        }
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    memcpy( ptr_param, entry->bin.ptr, min( int_param, entry->bin.size ) );
    return TRUE;
}

/* set a binary parameter in the registry */
static BOOL set_binary_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    BOOL ret;
    void *buffer = HeapAlloc( GetProcessHeap(), 0, entry->bin.size );

    memcpy( buffer, entry->bin.ptr, entry->bin.size );
    memcpy( buffer, ptr_param, min( int_param, entry->bin.size ));
    ret = save_entry( &entry->hdr, buffer, entry->bin.size, REG_BINARY, flags );
    if (ret)
    {
        memcpy( entry->bin.ptr, buffer, entry->bin.size );
        entry->hdr.loaded = TRUE;
    }
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

/* initialize a binary parameter */
static BOOL init_binary_entry( union sysparam_all_entry *entry )
{
    return init_entry( &entry->hdr, entry->bin.ptr, entry->bin.size, REG_BINARY );
}

/* get a user pref parameter in the registry */
static BOOL get_userpref_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    union sysparam_all_entry *parent_entry = (union sysparam_all_entry *)entry->pref.parent;
    BYTE prefs[8];

    if (!ptr_param) return FALSE;

    if (!parent_entry->hdr.get( parent_entry, sizeof(prefs), prefs, dpi )) return FALSE;
    *(BOOL *)ptr_param = (prefs[entry->pref.offset] & entry->pref.mask) != 0;
    return TRUE;
}

/* set a user pref parameter in the registry */
static BOOL set_userpref_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    union sysparam_all_entry *parent_entry = (union sysparam_all_entry *)entry->pref.parent;
    BYTE prefs[8];

    parent_entry->hdr.loaded = FALSE;  /* force loading it again */
    if (!parent_entry->hdr.get( parent_entry, sizeof(prefs), prefs, GetDpiForSystem() )) return FALSE;

    if (PtrToUlong( ptr_param )) prefs[entry->pref.offset] |= entry->pref.mask;
    else prefs[entry->pref.offset] &= ~entry->pref.mask;

    return parent_entry->hdr.set( parent_entry, sizeof(prefs), prefs, flags );
}

static BOOL get_entry_dpi( void *ptr, UINT int_param, void *ptr_param, UINT dpi )
{
    union sysparam_all_entry *entry = ptr;
    return entry->hdr.get( entry, int_param, ptr_param, dpi );
}

static BOOL get_entry( void *ptr, UINT int_param, void *ptr_param )
{
    return get_entry_dpi( ptr, int_param, ptr_param, GetDpiForSystem() );
}

static BOOL set_entry( void *ptr, UINT int_param, void *ptr_param, UINT flags )
{
    union sysparam_all_entry *entry = ptr;
    return entry->hdr.set( entry, int_param, ptr_param, flags );
}

#define UINT_ENTRY(name,val,base,reg) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_uint_entry, init_uint_entry, \
                                                  base, reg }, (val) }

#define UINT_ENTRY_MIRROR(name,val,base,reg,mirror_base) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_uint_entry, init_uint_entry, \
                                                  base, reg, mirror_base, reg }, (val) }

#define INT_ENTRY(name,val,base,reg) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_int_entry, init_int_entry, \
                                                  base, reg }, (val) }

#define BOOL_ENTRY(name,val,base,reg) \
    struct sysparam_bool_entry entry_##name = { { get_bool_entry, set_bool_entry, init_bool_entry, \
                                                  base, reg }, (val) }

#define BOOL_ENTRY_MIRROR(name,val,base,reg,mirror_base) \
    struct sysparam_bool_entry entry_##name = { { get_bool_entry, set_bool_entry, init_bool_entry, \
                                                  base, reg, mirror_base, reg }, (val) }

#define YESNO_ENTRY(name,val,base,reg) \
    struct sysparam_bool_entry entry_##name = { { get_yesno_entry, set_yesno_entry, init_yesno_entry, \
                                                  base, reg }, (val) }

#define TWIPS_ENTRY(name,val,base,reg) \
    struct sysparam_uint_entry entry_##name = { { get_twips_entry, set_twips_entry, init_int_entry, \
                                                  base, reg }, (val) }

#define DWORD_ENTRY(name,val,base,reg) \
    struct sysparam_dword_entry entry_##name = { { get_dword_entry, set_dword_entry, init_dword_entry, \
                                                   base, reg }, (val) }

#define BINARY_ENTRY(name,data,base,reg) \
    struct sysparam_binary_entry entry_##name = { { get_binary_entry, set_binary_entry, init_binary_entry, \
                                                    base, reg }, data, sizeof(data) }

#define PATH_ENTRY(name,base,reg) \
    struct sysparam_path_entry entry_##name = { { get_path_entry, set_path_entry, init_path_entry, \
                                                  base, reg } }

#define FONT_ENTRY(name,weight,base,reg) \
    struct sysparam_font_entry entry_##name = { { get_font_entry, set_font_entry, init_font_entry, \
                                                  base, reg }, (weight) }

#define USERPREF_ENTRY(name,offset,mask) \
    struct sysparam_pref_entry entry_##name = { { get_userpref_entry, set_userpref_entry }, \
                                                &entry_USERPREFERENCESMASK, (offset), (mask) }

static UINT_ENTRY( DRAGWIDTH, 4, DESKTOP_KEY, L"DragWidth" );
static UINT_ENTRY( DRAGHEIGHT, 4, DESKTOP_KEY, L"DragHeight" );
static UINT_ENTRY( DOUBLECLICKTIME, 500, MOUSE_KEY, L"DoubleClickSpeed" );
static UINT_ENTRY( FONTSMOOTHING, 2, DESKTOP_KEY, L"FontSmoothing" );
static UINT_ENTRY( GRIDGRANULARITY, 0, DESKTOP_KEY, L"GridGranularity" );
static UINT_ENTRY( KEYBOARDDELAY, 1, KEYBOARD_KEY, L"KeyboardDelay" );
static UINT_ENTRY( KEYBOARDSPEED, 31, KEYBOARD_KEY, L"KeyboardSpeed" );
static UINT_ENTRY( MENUSHOWDELAY, 400, DESKTOP_KEY, L"MenuShowDelay" );
static UINT_ENTRY( MINARRANGE, ARW_HIDE, METRICS_KEY, L"MinArrange" );
static UINT_ENTRY( MINHORZGAP, 0, METRICS_KEY, L"MinHorzGap" );
static UINT_ENTRY( MINVERTGAP, 0, METRICS_KEY, L"MinVertGap" );
static UINT_ENTRY( MINWIDTH, 154, METRICS_KEY, L"MinWidth" );
static UINT_ENTRY( MOUSEHOVERHEIGHT, 4, MOUSE_KEY, L"MouseHoverHeight" );
static UINT_ENTRY( MOUSEHOVERTIME, 400, MOUSE_KEY, L"MouseHoverTime" );
static UINT_ENTRY( MOUSEHOVERWIDTH, 4, MOUSE_KEY, L"MouseHoverWidth" );
static UINT_ENTRY( MOUSESPEED, 10, MOUSE_KEY, L"MouseSensitivity" );
static UINT_ENTRY( MOUSETRAILS, 0, MOUSE_KEY, L"MouseTrails" );
static UINT_ENTRY( SCREENSAVETIMEOUT, 300, DESKTOP_KEY, L"ScreenSaveTimeOut" );
static UINT_ENTRY( WHEELSCROLLCHARS, 3, DESKTOP_KEY, L"WheelScrollChars" );
static UINT_ENTRY( WHEELSCROLLLINES, 3, DESKTOP_KEY, L"WheelScrollLines" );
static UINT_ENTRY_MIRROR( DOUBLECLKHEIGHT, 4, MOUSE_KEY, L"DoubleClickHeight", DESKTOP_KEY );
static UINT_ENTRY_MIRROR( DOUBLECLKWIDTH, 4, MOUSE_KEY, L"DoubleClickWidth", DESKTOP_KEY );
static UINT_ENTRY_MIRROR( MENUDROPALIGNMENT, 0, DESKTOP_KEY, L"MenuDropAlignment", VERSION_KEY );

static INT_ENTRY( MOUSETHRESHOLD1, 6, MOUSE_KEY, L"MouseThreshold1" );
static INT_ENTRY( MOUSETHRESHOLD2, 10, MOUSE_KEY, L"MouseThreshold2" );
static INT_ENTRY( MOUSEACCELERATION, 1, MOUSE_KEY, L"MouseSpeed" );

static BOOL_ENTRY( BLOCKSENDINPUTRESETS, FALSE, DESKTOP_KEY, L"BlockSendInputResets" );
static BOOL_ENTRY( DRAGFULLWINDOWS, FALSE, DESKTOP_KEY, L"DragFullWindows" );
static BOOL_ENTRY( KEYBOARDPREF, TRUE, KEYBOARDPREF_KEY, L"On" );
static BOOL_ENTRY( LOWPOWERACTIVE, FALSE, DESKTOP_KEY, L"LowPowerActive" );
static BOOL_ENTRY( MOUSEBUTTONSWAP, FALSE, MOUSE_KEY, L"SwapMouseButtons" );
static BOOL_ENTRY( POWEROFFACTIVE, FALSE, DESKTOP_KEY, L"PowerOffActive" );
static BOOL_ENTRY( SCREENREADER, FALSE, SCREENREADER_KEY, L"On" );
static BOOL_ENTRY( SCREENSAVEACTIVE, TRUE, DESKTOP_KEY, L"ScreenSaveActive" );
static BOOL_ENTRY( SCREENSAVERRUNNING, FALSE, DESKTOP_KEY, L"WINE_ScreenSaverRunning" ); /* FIXME - real value */
static BOOL_ENTRY( SHOWSOUNDS, FALSE, SHOWSOUNDS_KEY, L"On" );
static BOOL_ENTRY( SNAPTODEFBUTTON, FALSE, MOUSE_KEY, L"SnapToDefaultButton" );
static BOOL_ENTRY_MIRROR( ICONTITLEWRAP, TRUE, DESKTOP_KEY, L"IconTitleWrap", METRICS_KEY );
static BOOL_ENTRY( AUDIODESC_ON, FALSE, AUDIODESC_KEY, L"On" );

static YESNO_ENTRY( BEEP, TRUE, SOUND_KEY, L"Beep" );

static TWIPS_ENTRY( BORDER, -15, METRICS_KEY, L"BorderWidth" );
static TWIPS_ENTRY( CAPTIONHEIGHT, -270, METRICS_KEY, L"CaptionHeight" );
static TWIPS_ENTRY( CAPTIONWIDTH, -270, METRICS_KEY, L"CaptionWidth" );
static TWIPS_ENTRY( ICONHORIZONTALSPACING, -1125, METRICS_KEY, L"IconSpacing" );
static TWIPS_ENTRY( ICONVERTICALSPACING, -1125, METRICS_KEY, L"IconVerticalSpacing" );
static TWIPS_ENTRY( MENUHEIGHT, -270, METRICS_KEY, L"MenuHeight" );
static TWIPS_ENTRY( MENUWIDTH, -270, METRICS_KEY, L"MenuWidth" );
static TWIPS_ENTRY( PADDEDBORDERWIDTH, 0, METRICS_KEY, L"PaddedBorderWidth" );
static TWIPS_ENTRY( SCROLLHEIGHT, -240, METRICS_KEY, L"ScrollHeight" );
static TWIPS_ENTRY( SCROLLWIDTH, -240, METRICS_KEY, L"ScrollWidth" );
static TWIPS_ENTRY( SMCAPTIONHEIGHT, -225, METRICS_KEY, L"SmCaptionHeight" );
static TWIPS_ENTRY( SMCAPTIONWIDTH, -225, METRICS_KEY, L"SmCaptionWidth" );

static DWORD_ENTRY( ACTIVEWINDOWTRACKING, 0, MOUSE_KEY, L"ActiveWindowTracking" );
static DWORD_ENTRY( ACTIVEWNDTRKTIMEOUT, 0, DESKTOP_KEY, L"ActiveWndTrackTimeout" );
static DWORD_ENTRY( CARETWIDTH, 1, DESKTOP_KEY, L"CaretWidth" );
static DWORD_ENTRY( DPISCALINGVER, 0, DESKTOP_KEY, L"DpiScalingVer" );
static DWORD_ENTRY( FOCUSBORDERHEIGHT, 1, DESKTOP_KEY, L"FocusBorderHeight" );
static DWORD_ENTRY( FOCUSBORDERWIDTH, 1, DESKTOP_KEY, L"FocusBorderWidth" );
static DWORD_ENTRY( FONTSMOOTHINGCONTRAST, 0, DESKTOP_KEY, L"FontSmoothingGamma" );
static DWORD_ENTRY( FONTSMOOTHINGORIENTATION, FE_FONTSMOOTHINGORIENTATIONRGB, DESKTOP_KEY, L"FontSmoothingOrientation" );
static DWORD_ENTRY( FONTSMOOTHINGTYPE, FE_FONTSMOOTHINGSTANDARD, DESKTOP_KEY, L"FontSmoothingType" );
static DWORD_ENTRY( FOREGROUNDFLASHCOUNT, 3, DESKTOP_KEY, L"ForegroundFlashCount" );
static DWORD_ENTRY( FOREGROUNDLOCKTIMEOUT, 0, DESKTOP_KEY, L"ForegroundLockTimeout" );
static DWORD_ENTRY( LOGPIXELS, 0, DESKTOP_KEY, L"LogPixels" );
static DWORD_ENTRY( MOUSECLICKLOCKTIME, 1200, DESKTOP_KEY, L"ClickLockTime" );
static DWORD_ENTRY( AUDIODESC_LOCALE, 0, AUDIODESC_KEY, L"Locale" );

static PATH_ENTRY( DESKPATTERN, DESKTOP_KEY, L"Pattern" );
static PATH_ENTRY( DESKWALLPAPER, DESKTOP_KEY, L"Wallpaper" );

static BYTE user_prefs[8] = { 0x30, 0x00, 0x00, 0x80, 0x12, 0x00, 0x00, 0x00 };
static BINARY_ENTRY( USERPREFERENCESMASK, user_prefs, DESKTOP_KEY, L"UserPreferencesMask" );

static FONT_ENTRY( CAPTIONLOGFONT, FW_BOLD, METRICS_KEY, L"CaptionFont" );
static FONT_ENTRY( ICONTITLELOGFONT, FW_NORMAL, METRICS_KEY, L"IconFont" );
static FONT_ENTRY( MENULOGFONT, FW_NORMAL, METRICS_KEY, L"MenuFont" );
static FONT_ENTRY( MESSAGELOGFONT, FW_NORMAL, METRICS_KEY, L"MessageFont" );
static FONT_ENTRY( SMCAPTIONLOGFONT, FW_NORMAL, METRICS_KEY, L"SmCaptionFont" );
static FONT_ENTRY( STATUSLOGFONT, FW_NORMAL, METRICS_KEY, L"StatusFont" );

static USERPREF_ENTRY( MENUANIMATION,            0, 0x02 );
static USERPREF_ENTRY( COMBOBOXANIMATION,        0, 0x04 );
static USERPREF_ENTRY( LISTBOXSMOOTHSCROLLING,   0, 0x08 );
static USERPREF_ENTRY( GRADIENTCAPTIONS,         0, 0x10 );
static USERPREF_ENTRY( KEYBOARDCUES,             0, 0x20 );
static USERPREF_ENTRY( ACTIVEWNDTRKZORDER,       0, 0x40 );
static USERPREF_ENTRY( HOTTRACKING,              0, 0x80 );
static USERPREF_ENTRY( MENUFADE,                 1, 0x02 );
static USERPREF_ENTRY( SELECTIONFADE,            1, 0x04 );
static USERPREF_ENTRY( TOOLTIPANIMATION,         1, 0x08 );
static USERPREF_ENTRY( TOOLTIPFADE,              1, 0x10 );
static USERPREF_ENTRY( CURSORSHADOW,             1, 0x20 );
static USERPREF_ENTRY( MOUSESONAR,               1, 0x40 );
static USERPREF_ENTRY( MOUSECLICKLOCK,           1, 0x80 );
static USERPREF_ENTRY( MOUSEVANISH,              2, 0x01 );
static USERPREF_ENTRY( FLATMENU,                 2, 0x02 );
static USERPREF_ENTRY( DROPSHADOW,               2, 0x04 );
static USERPREF_ENTRY( UIEFFECTS,                3, 0x80 );
static USERPREF_ENTRY( DISABLEOVERLAPPEDCONTENT, 4, 0x01 );
static USERPREF_ENTRY( CLIENTAREAANIMATION,      4, 0x02 );
static USERPREF_ENTRY( CLEARTYPE,                4, 0x10 );
static USERPREF_ENTRY( SPEECHRECOGNITION,        4, 0x20 );

static struct sysparam_rgb_entry system_colors[] =
{
#define RGB_ENTRY(name,val,reg) { { get_rgb_entry, set_rgb_entry, init_rgb_entry, COLORS_KEY, reg }, (val) }
    RGB_ENTRY( COLOR_SCROLLBAR, RGB(212, 208, 200), L"Scrollbar" ),
    RGB_ENTRY( COLOR_BACKGROUND, RGB(58, 110, 165), L"Background" ),
    RGB_ENTRY( COLOR_ACTIVECAPTION, RGB(10, 36, 106), L"ActiveTitle" ),
    RGB_ENTRY( COLOR_INACTIVECAPTION, RGB(128, 128, 128), L"InactiveTitle" ),
    RGB_ENTRY( COLOR_MENU, RGB(212, 208, 200), L"Menu" ),
    RGB_ENTRY( COLOR_WINDOW, RGB(255, 255, 255), L"Window" ),
    RGB_ENTRY( COLOR_WINDOWFRAME, RGB(0, 0, 0), L"WindowFrame" ),
    RGB_ENTRY( COLOR_MENUTEXT, RGB(0, 0, 0), L"MenuText" ),
    RGB_ENTRY( COLOR_WINDOWTEXT, RGB(0, 0, 0), L"WindowText" ),
    RGB_ENTRY( COLOR_CAPTIONTEXT, RGB(255, 255, 255), L"TitleText" ),
    RGB_ENTRY( COLOR_ACTIVEBORDER, RGB(212, 208, 200), L"ActiveBorder" ),
    RGB_ENTRY( COLOR_INACTIVEBORDER, RGB(212, 208, 200), L"InactiveBorder" ),
    RGB_ENTRY( COLOR_APPWORKSPACE, RGB(128, 128, 128), L"AppWorkSpace" ),
    RGB_ENTRY( COLOR_HIGHLIGHT, RGB(10, 36, 106), L"Hilight" ),
    RGB_ENTRY( COLOR_HIGHLIGHTTEXT, RGB(255, 255, 255), L"HilightText" ),
    RGB_ENTRY( COLOR_BTNFACE, RGB(212, 208, 200), L"ButtonFace" ),
    RGB_ENTRY( COLOR_BTNSHADOW, RGB(128, 128, 128), L"ButtonShadow" ),
    RGB_ENTRY( COLOR_GRAYTEXT, RGB(128, 128, 128), L"GrayText" ),
    RGB_ENTRY( COLOR_BTNTEXT, RGB(0, 0, 0), L"ButtonText" ),
    RGB_ENTRY( COLOR_INACTIVECAPTIONTEXT, RGB(212, 208, 200), L"InactiveTitleText" ),
    RGB_ENTRY( COLOR_BTNHIGHLIGHT, RGB(255, 255, 255), L"ButtonHilight" ),
    RGB_ENTRY( COLOR_3DDKSHADOW, RGB(64, 64, 64), L"ButtonDkShadow" ),
    RGB_ENTRY( COLOR_3DLIGHT, RGB(212, 208, 200), L"ButtonLight" ),
    RGB_ENTRY( COLOR_INFOTEXT, RGB(0, 0, 0), L"InfoText" ),
    RGB_ENTRY( COLOR_INFOBK, RGB(255, 255, 225), L"InfoWindow" ),
    RGB_ENTRY( COLOR_ALTERNATEBTNFACE, RGB(181, 181, 181), L"ButtonAlternateFace" ),
    RGB_ENTRY( COLOR_HOTLIGHT, RGB(0, 0, 200), L"HotTrackingColor" ),
    RGB_ENTRY( COLOR_GRADIENTACTIVECAPTION, RGB(166, 202, 240), L"GradientActiveTitle" ),
    RGB_ENTRY( COLOR_GRADIENTINACTIVECAPTION, RGB(192, 192, 192), L"GradientInactiveTitle" ),
    RGB_ENTRY( COLOR_MENUHILIGHT, RGB(10, 36, 106), L"MenuHilight" ),
    RGB_ENTRY( COLOR_MENUBAR, RGB(212, 208, 200), L"MenuBar" )
#undef RGB_ENTRY
};

/* entries that are initialized by default in the registry */
static union sysparam_all_entry * const default_entries[] =
{
    (union sysparam_all_entry *)&entry_ACTIVEWINDOWTRACKING,
    (union sysparam_all_entry *)&entry_ACTIVEWNDTRKTIMEOUT,
    (union sysparam_all_entry *)&entry_BEEP,
    (union sysparam_all_entry *)&entry_BLOCKSENDINPUTRESETS,
    (union sysparam_all_entry *)&entry_BORDER,
    (union sysparam_all_entry *)&entry_CAPTIONHEIGHT,
    (union sysparam_all_entry *)&entry_CAPTIONWIDTH,
    (union sysparam_all_entry *)&entry_CARETWIDTH,
    (union sysparam_all_entry *)&entry_DESKWALLPAPER,
    (union sysparam_all_entry *)&entry_DOUBLECLICKTIME,
    (union sysparam_all_entry *)&entry_DOUBLECLKHEIGHT,
    (union sysparam_all_entry *)&entry_DOUBLECLKWIDTH,
    (union sysparam_all_entry *)&entry_DRAGFULLWINDOWS,
    (union sysparam_all_entry *)&entry_DRAGHEIGHT,
    (union sysparam_all_entry *)&entry_DRAGWIDTH,
    (union sysparam_all_entry *)&entry_FOCUSBORDERHEIGHT,
    (union sysparam_all_entry *)&entry_FOCUSBORDERWIDTH,
    (union sysparam_all_entry *)&entry_FONTSMOOTHING,
    (union sysparam_all_entry *)&entry_FONTSMOOTHINGCONTRAST,
    (union sysparam_all_entry *)&entry_FONTSMOOTHINGORIENTATION,
    (union sysparam_all_entry *)&entry_FONTSMOOTHINGTYPE,
    (union sysparam_all_entry *)&entry_FOREGROUNDFLASHCOUNT,
    (union sysparam_all_entry *)&entry_FOREGROUNDLOCKTIMEOUT,
    (union sysparam_all_entry *)&entry_ICONHORIZONTALSPACING,
    (union sysparam_all_entry *)&entry_ICONTITLEWRAP,
    (union sysparam_all_entry *)&entry_ICONVERTICALSPACING,
    (union sysparam_all_entry *)&entry_KEYBOARDDELAY,
    (union sysparam_all_entry *)&entry_KEYBOARDPREF,
    (union sysparam_all_entry *)&entry_KEYBOARDSPEED,
    (union sysparam_all_entry *)&entry_LOWPOWERACTIVE,
    (union sysparam_all_entry *)&entry_MENUHEIGHT,
    (union sysparam_all_entry *)&entry_MENUSHOWDELAY,
    (union sysparam_all_entry *)&entry_MENUWIDTH,
    (union sysparam_all_entry *)&entry_MOUSEACCELERATION,
    (union sysparam_all_entry *)&entry_MOUSEBUTTONSWAP,
    (union sysparam_all_entry *)&entry_MOUSECLICKLOCKTIME,
    (union sysparam_all_entry *)&entry_MOUSEHOVERHEIGHT,
    (union sysparam_all_entry *)&entry_MOUSEHOVERTIME,
    (union sysparam_all_entry *)&entry_MOUSEHOVERWIDTH,
    (union sysparam_all_entry *)&entry_MOUSESPEED,
    (union sysparam_all_entry *)&entry_MOUSETHRESHOLD1,
    (union sysparam_all_entry *)&entry_MOUSETHRESHOLD2,
    (union sysparam_all_entry *)&entry_PADDEDBORDERWIDTH,
    (union sysparam_all_entry *)&entry_SCREENREADER,
    (union sysparam_all_entry *)&entry_SCROLLHEIGHT,
    (union sysparam_all_entry *)&entry_SCROLLWIDTH,
    (union sysparam_all_entry *)&entry_SHOWSOUNDS,
    (union sysparam_all_entry *)&entry_SMCAPTIONHEIGHT,
    (union sysparam_all_entry *)&entry_SMCAPTIONWIDTH,
    (union sysparam_all_entry *)&entry_SNAPTODEFBUTTON,
    (union sysparam_all_entry *)&entry_USERPREFERENCESMASK,
    (union sysparam_all_entry *)&entry_WHEELSCROLLCHARS,
    (union sysparam_all_entry *)&entry_WHEELSCROLLLINES,
    (union sysparam_all_entry *)&entry_AUDIODESC_LOCALE,
    (union sysparam_all_entry *)&entry_AUDIODESC_ON,
};

/***********************************************************************
 *           SYSPARAMS_Init
 */
void SYSPARAMS_Init(void)
{
    HKEY key;
    DWORD i, dispos, dpi_scaling;

    /* this one must be non-volatile */
    if (RegCreateKeyW( HKEY_CURRENT_USER, L"Software\\Wine", &key ))
    {
        ERR("Can't create wine registry branch\n");
        return;
    }

    /* @@ Wine registry key: HKCU\Software\Wine\Temporary System Parameters */
    if (RegCreateKeyExW( key, L"Temporary System Parameters", 0, 0,
                         REG_OPTION_VOLATILE, KEY_ALL_ACCESS, 0, &volatile_base_key, &dispos ))
        ERR("Can't create non-permanent wine registry branch\n");

    RegCloseKey( key );

    get_dword_entry( (union sysparam_all_entry *)&entry_LOGPIXELS, 0, &system_dpi, 0 );
    if (!system_dpi)  /* check fallback key */
    {
        if (!RegOpenKeyW( HKEY_CURRENT_CONFIG, L"Software\\Fonts", &key ))
        {
            DWORD type, size = sizeof(system_dpi);
            if (RegQueryValueExW( key, L"LogPixels", NULL, &type, (void *)&system_dpi, &size ) ||
                type != REG_DWORD)
                system_dpi = 0;
            RegCloseKey( key );
        }
    }
    if (!system_dpi) system_dpi = USER_DEFAULT_SCREEN_DPI;

    /* FIXME: what do the DpiScalingVer flags mean? */
    get_dword_entry( (union sysparam_all_entry *)&entry_DPISCALINGVER, 0, &dpi_scaling, 0 );
    if (!dpi_scaling)
    {
        default_awareness = DPI_AWARENESS_PER_MONITOR_AWARE;
        dpi_awareness = 0x10 | default_awareness;
    }

    if (volatile_base_key && dispos == REG_CREATED_NEW_KEY)  /* first process, initialize entries */
    {
        for (i = 0; i < ARRAY_SIZE( default_entries ); i++)
            default_entries[i]->hdr.init( default_entries[i] );
    }
}

static BOOL update_desktop_wallpaper(void)
{
    DWORD pid;

    if (GetWindowThreadProcessId( GetDesktopWindow(), &pid ) && pid == GetCurrentProcessId())
    {
        WCHAR wallpaper[MAX_PATH], pattern[256];

        entry_DESKWALLPAPER.hdr.loaded = entry_DESKPATTERN.hdr.loaded = FALSE;
        if (get_entry( &entry_DESKWALLPAPER, MAX_PATH, wallpaper ) &&
            get_entry( &entry_DESKPATTERN, 256, pattern ))
            update_wallpaper( wallpaper, pattern );
    }
    else SendMessageW( GetDesktopWindow(), WM_SETTINGCHANGE, SPI_SETDESKWALLPAPER, 0 );
    return TRUE;
}


/***********************************************************************
 *		SystemParametersInfoForDpi  (USER32.@)
 */
BOOL WINAPI SystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi )
{
    BOOL ret = FALSE;

    switch (action)
    {
    case SPI_GETICONTITLELOGFONT:
        ret = get_entry_dpi( &entry_ICONTITLELOGFONT, val, ptr, dpi );
        break;
    case SPI_GETNONCLIENTMETRICS:
    {
        NONCLIENTMETRICSW *ncm = ptr;

        if (!ncm) break;
        ret = get_entry_dpi( &entry_BORDER, 0, &ncm->iBorderWidth, dpi ) &&
              get_entry_dpi( &entry_SCROLLWIDTH, 0, &ncm->iScrollWidth, dpi ) &&
              get_entry_dpi( &entry_SCROLLHEIGHT, 0, &ncm->iScrollHeight, dpi ) &&
              get_entry_dpi( &entry_CAPTIONWIDTH, 0, &ncm->iCaptionWidth, dpi ) &&
              get_entry_dpi( &entry_CAPTIONHEIGHT, 0, &ncm->iCaptionHeight, dpi ) &&
              get_entry_dpi( &entry_CAPTIONLOGFONT, 0, &ncm->lfCaptionFont, dpi ) &&
              get_entry_dpi( &entry_SMCAPTIONWIDTH, 0, &ncm->iSmCaptionWidth, dpi ) &&
              get_entry_dpi( &entry_SMCAPTIONHEIGHT, 0, &ncm->iSmCaptionHeight, dpi ) &&
              get_entry_dpi( &entry_SMCAPTIONLOGFONT, 0, &ncm->lfSmCaptionFont, dpi ) &&
              get_entry_dpi( &entry_MENUWIDTH, 0, &ncm->iMenuWidth, dpi ) &&
              get_entry_dpi( &entry_MENUHEIGHT, 0, &ncm->iMenuHeight, dpi ) &&
              get_entry_dpi( &entry_MENULOGFONT, 0, &ncm->lfMenuFont, dpi ) &&
              get_entry_dpi( &entry_STATUSLOGFONT, 0, &ncm->lfStatusFont, dpi ) &&
              get_entry_dpi( &entry_MESSAGELOGFONT, 0, &ncm->lfMessageFont, dpi );
        if (ret && ncm->cbSize == sizeof(NONCLIENTMETRICSW))
            ret = get_entry_dpi( &entry_PADDEDBORDERWIDTH, 0, &ncm->iPaddedBorderWidth, dpi );
        normalize_nonclientmetrics( ncm );
        break;
    }
    case SPI_GETICONMETRICS:
    {
	ICONMETRICSW *im = ptr;
	if (im && im->cbSize == sizeof(*im))
            ret = get_entry_dpi( &entry_ICONHORIZONTALSPACING, 0, &im->iHorzSpacing, dpi ) &&
                  get_entry_dpi( &entry_ICONVERTICALSPACING, 0, &im->iVertSpacing, dpi ) &&
                  get_entry_dpi( &entry_ICONTITLEWRAP, 0, &im->iTitleWrap, dpi ) &&
                  get_entry_dpi( &entry_ICONTITLELOGFONT, 0, &im->lfFont, dpi );
	break;
    }
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        break;
    }
    return ret;
}


/***********************************************************************
 *		SystemParametersInfoW (USER32.@)
 *
 *     Each system parameter has flag which shows whether the parameter
 * is loaded or not. Parameters, stored directly in SysParametersInfo are
 * loaded from registry only when they are requested and the flag is
 * "false", after the loading the flag is set to "true". On interprocess
 * notification of the parameter change the corresponding parameter flag is
 * set to "false". The parameter value will be reloaded when it is requested
 * the next time.
 *     Parameters, backed by or depend on GetSystemMetrics are processed
 * differently. These parameters are always loaded. They are reloaded right
 * away on interprocess change notification. We can't do lazy loading because
 * we don't want to complicate GetSystemMetrics.
 *     Parameters, backed by X settings are read from corresponding setting.
 * On the parameter change request the setting is changed. Interprocess change
 * notifications are ignored.
 *     When parameter value is updated the changed value is stored in permanent
 * registry branch if saving is requested. Otherwise it is stored
 * in temporary branch
 *
 * Some SPI values can also be stored as Twips values in the registry,
 * don't forget the conversion!
 */
BOOL WINAPI SystemParametersInfoW( UINT uiAction, UINT uiParam,
				   PVOID pvParam, UINT fWinIni )
{
#define WINE_SPI_FIXME(x) \
    case x: \
        { \
            static BOOL warn = TRUE; \
            if (warn) \
            { \
                warn = FALSE; \
                FIXME( "Unimplemented action: %u (%s)\n", x, #x ); \
            } \
        } \
        SetLastError( ERROR_INVALID_SPI_VALUE ); \
        ret = FALSE; \
        break
#define WINE_SPI_WARN(x) \
    case x: \
        WARN( "Ignored action: %u (%s)\n", x, #x ); \
        ret = TRUE; \
        break

    BOOL ret = USER_Driver->pSystemParametersInfo( uiAction, uiParam, pvParam, fWinIni );
    unsigned spi_idx = 0;

    if (!ret) switch (uiAction)
    {
    case SPI_GETBEEP:
        ret = get_entry( &entry_BEEP, uiParam, pvParam );
        break;
    case SPI_SETBEEP:
        ret = set_entry( &entry_BEEP, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSE:
        ret = get_entry( &entry_MOUSETHRESHOLD1, uiParam, (INT *)pvParam ) &&
              get_entry( &entry_MOUSETHRESHOLD2, uiParam, (INT *)pvParam + 1 ) &&
              get_entry( &entry_MOUSEACCELERATION, uiParam, (INT *)pvParam + 2 );
        break;
    case SPI_SETMOUSE:
        ret = set_entry( &entry_MOUSETHRESHOLD1, ((INT *)pvParam)[0], pvParam, fWinIni ) &&
              set_entry( &entry_MOUSETHRESHOLD2, ((INT *)pvParam)[1], pvParam, fWinIni ) &&
              set_entry( &entry_MOUSEACCELERATION, ((INT *)pvParam)[2], pvParam, fWinIni );
        break;
    case SPI_GETBORDER:
        ret = get_entry( &entry_BORDER, uiParam, pvParam );
        if (*(INT*)pvParam < 1) *(INT*)pvParam = 1;
        break;
    case SPI_SETBORDER:
        ret = set_entry( &entry_BORDER, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETKEYBOARDSPEED:
        ret = get_entry( &entry_KEYBOARDSPEED, uiParam, pvParam );
        break;
    case SPI_SETKEYBOARDSPEED:
        if (uiParam > 31) uiParam = 31;
        ret = set_entry( &entry_KEYBOARDSPEED, uiParam, pvParam, fWinIni );
        break;

    /* not implemented in Windows */
    WINE_SPI_WARN(SPI_LANGDRIVER);              /*     12 */

    case SPI_ICONHORIZONTALSPACING:
        if (pvParam != NULL)
            ret = get_entry( &entry_ICONHORIZONTALSPACING, uiParam, pvParam );
        else
        {
            int min_val = map_to_dpi( 32, GetDpiForSystem() );
            ret = set_entry( &entry_ICONHORIZONTALSPACING, max( min_val, uiParam ), pvParam, fWinIni );
        }
        break;
    case SPI_GETSCREENSAVETIMEOUT:
        ret = get_entry( &entry_SCREENSAVETIMEOUT, uiParam, pvParam );
        break;
    case SPI_SETSCREENSAVETIMEOUT:
        ret = set_entry( &entry_SCREENSAVETIMEOUT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETSCREENSAVEACTIVE:
        ret = get_entry( &entry_SCREENSAVEACTIVE, uiParam, pvParam );
        break;
    case SPI_SETSCREENSAVEACTIVE:
        ret = set_entry( &entry_SCREENSAVEACTIVE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETGRIDGRANULARITY:
        ret = get_entry( &entry_GRIDGRANULARITY, uiParam, pvParam );
        break;
    case SPI_SETGRIDGRANULARITY:
        ret = set_entry( &entry_GRIDGRANULARITY, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDESKWALLPAPER:
        if (!pvParam || set_entry( &entry_DESKWALLPAPER, uiParam, pvParam, fWinIni ))
            ret = update_desktop_wallpaper();
        break;
    case SPI_SETDESKPATTERN:
        if (!pvParam || set_entry( &entry_DESKPATTERN, uiParam, pvParam, fWinIni ))
            ret = update_desktop_wallpaper();
        break;
    case SPI_GETKEYBOARDDELAY:
        ret = get_entry( &entry_KEYBOARDDELAY, uiParam, pvParam );
        break;
    case SPI_SETKEYBOARDDELAY:
        ret = set_entry( &entry_KEYBOARDDELAY, uiParam, pvParam, fWinIni );
        break;
    case SPI_ICONVERTICALSPACING:
        if (pvParam != NULL)
            ret = get_entry( &entry_ICONVERTICALSPACING, uiParam, pvParam );
        else
        {
            int min_val = map_to_dpi( 32, GetDpiForSystem() );
            ret = set_entry( &entry_ICONVERTICALSPACING, max( min_val, uiParam ), pvParam, fWinIni );
        }
        break;
    case SPI_GETICONTITLEWRAP:
        ret = get_entry( &entry_ICONTITLEWRAP, uiParam, pvParam );
        break;
    case SPI_SETICONTITLEWRAP:
        ret = set_entry( &entry_ICONTITLEWRAP, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMENUDROPALIGNMENT:
        ret = get_entry( &entry_MENUDROPALIGNMENT, uiParam, pvParam );
        break;
    case SPI_SETMENUDROPALIGNMENT:
        ret = set_entry( &entry_MENUDROPALIGNMENT, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDOUBLECLKWIDTH:
        ret = set_entry( &entry_DOUBLECLKWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDOUBLECLKHEIGHT:
        ret = set_entry( &entry_DOUBLECLKHEIGHT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETICONTITLELOGFONT:
        ret = get_entry( &entry_ICONTITLELOGFONT, uiParam, pvParam );
        break;
    case SPI_SETDOUBLECLICKTIME:
        ret = set_entry( &entry_DOUBLECLICKTIME, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETMOUSEBUTTONSWAP:
        ret = set_entry( &entry_MOUSEBUTTONSWAP, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETICONTITLELOGFONT:
        ret = set_entry( &entry_ICONTITLELOGFONT, uiParam, pvParam, fWinIni );
        break;

    case SPI_GETFASTTASKSWITCH:			/*     35 */
        if (!pvParam) return FALSE;
        *(BOOL *)pvParam = TRUE;
        ret = TRUE;
        break;

    case SPI_SETFASTTASKSWITCH:                 /*     36 */
        /* the action is disabled */
        ret = FALSE;
        break;

    case SPI_SETDRAGFULLWINDOWS:
        ret = set_entry( &entry_DRAGFULLWINDOWS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETDRAGFULLWINDOWS:
        ret = get_entry( &entry_DRAGFULLWINDOWS, uiParam, pvParam );
        break;
    case SPI_GETNONCLIENTMETRICS:
    {
        LPNONCLIENTMETRICSW lpnm = pvParam;
        int padded_border;

        if (!pvParam) return FALSE;

        ret = get_entry( &entry_BORDER, 0, &lpnm->iBorderWidth ) &&
              get_entry( &entry_PADDEDBORDERWIDTH, 0, &padded_border ) &&
              get_entry( &entry_SCROLLWIDTH, 0, &lpnm->iScrollWidth ) &&
              get_entry( &entry_SCROLLHEIGHT, 0, &lpnm->iScrollHeight ) &&
              get_entry( &entry_CAPTIONWIDTH, 0, &lpnm->iCaptionWidth ) &&
              get_entry( &entry_CAPTIONHEIGHT, 0, &lpnm->iCaptionHeight ) &&
              get_entry( &entry_CAPTIONLOGFONT, 0, &lpnm->lfCaptionFont ) &&
              get_entry( &entry_SMCAPTIONWIDTH, 0, &lpnm->iSmCaptionWidth ) &&
              get_entry( &entry_SMCAPTIONHEIGHT, 0, &lpnm->iSmCaptionHeight ) &&
              get_entry( &entry_SMCAPTIONLOGFONT, 0, &lpnm->lfSmCaptionFont ) &&
              get_entry( &entry_MENUWIDTH, 0, &lpnm->iMenuWidth ) &&
              get_entry( &entry_MENUHEIGHT, 0, &lpnm->iMenuHeight ) &&
              get_entry( &entry_MENULOGFONT, 0, &lpnm->lfMenuFont ) &&
              get_entry( &entry_STATUSLOGFONT, 0, &lpnm->lfStatusFont ) &&
              get_entry( &entry_MESSAGELOGFONT, 0, &lpnm->lfMessageFont );
        if (ret)
        {
            lpnm->iBorderWidth += padded_border;
            if (lpnm->cbSize == sizeof(NONCLIENTMETRICSW)) lpnm->iPaddedBorderWidth = 0;
        }
        normalize_nonclientmetrics( lpnm );
        break;
    }
    case SPI_SETNONCLIENTMETRICS:
    {
        LPNONCLIENTMETRICSW lpnm = pvParam;
        int padded_border;

        if (lpnm && (lpnm->cbSize == sizeof(NONCLIENTMETRICSW) ||
                     lpnm->cbSize == FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth)))
        {
            get_entry( &entry_PADDEDBORDERWIDTH, 0, &padded_border );

            ret = set_entry( &entry_BORDER, lpnm->iBorderWidth - padded_border, NULL, fWinIni ) &&
                  set_entry( &entry_SCROLLWIDTH, lpnm->iScrollWidth, NULL, fWinIni ) &&
                  set_entry( &entry_SCROLLHEIGHT, lpnm->iScrollHeight, NULL, fWinIni ) &&
                  set_entry( &entry_CAPTIONWIDTH, lpnm->iCaptionWidth, NULL, fWinIni ) &&
                  set_entry( &entry_CAPTIONHEIGHT, lpnm->iCaptionHeight, NULL, fWinIni ) &&
                  set_entry( &entry_SMCAPTIONWIDTH, lpnm->iSmCaptionWidth, NULL, fWinIni ) &&
                  set_entry( &entry_SMCAPTIONHEIGHT, lpnm->iSmCaptionHeight, NULL, fWinIni ) &&
                  set_entry( &entry_MENUWIDTH, lpnm->iMenuWidth, NULL, fWinIni ) &&
                  set_entry( &entry_MENUHEIGHT, lpnm->iMenuHeight, NULL, fWinIni ) &&
                  set_entry( &entry_MENULOGFONT, 0, &lpnm->lfMenuFont, fWinIni ) &&
                  set_entry( &entry_CAPTIONLOGFONT, 0, &lpnm->lfCaptionFont, fWinIni ) &&
                  set_entry( &entry_SMCAPTIONLOGFONT, 0, &lpnm->lfSmCaptionFont, fWinIni ) &&
                  set_entry( &entry_STATUSLOGFONT, 0, &lpnm->lfStatusFont, fWinIni ) &&
                  set_entry( &entry_MESSAGELOGFONT, 0, &lpnm->lfMessageFont, fWinIni );
        }
        break;
    }
    case SPI_GETMINIMIZEDMETRICS:
    {
        MINIMIZEDMETRICS * lpMm = pvParam;
        if (lpMm && lpMm->cbSize == sizeof(*lpMm)) {
            ret = get_entry( &entry_MINWIDTH, 0, &lpMm->iWidth ) &&
                  get_entry( &entry_MINHORZGAP, 0, &lpMm->iHorzGap ) &&
                  get_entry( &entry_MINVERTGAP, 0, &lpMm->iVertGap ) &&
                  get_entry( &entry_MINARRANGE, 0, &lpMm->iArrange );
            lpMm->iWidth = max( 0, lpMm->iWidth );
            lpMm->iHorzGap = max( 0, lpMm->iHorzGap );
            lpMm->iVertGap = max( 0, lpMm->iVertGap );
            lpMm->iArrange &= 0x0f;
        }
        break;
    }
    case SPI_SETMINIMIZEDMETRICS:
    {
        MINIMIZEDMETRICS * lpMm = pvParam;
        if (lpMm && lpMm->cbSize == sizeof(*lpMm))
            ret = set_entry( &entry_MINWIDTH, max( 0, lpMm->iWidth ), NULL, fWinIni ) &&
                  set_entry( &entry_MINHORZGAP, max( 0, lpMm->iHorzGap ), NULL, fWinIni ) &&
                  set_entry( &entry_MINVERTGAP, max( 0, lpMm->iVertGap ), NULL, fWinIni ) &&
                  set_entry( &entry_MINARRANGE, lpMm->iArrange & 0x0f, NULL, fWinIni );
        break;
    }
    case SPI_GETICONMETRICS:
    {
	LPICONMETRICSW lpIcon = pvParam;
	if(lpIcon && lpIcon->cbSize == sizeof(*lpIcon))
	{
            ret = get_entry( &entry_ICONHORIZONTALSPACING, 0, &lpIcon->iHorzSpacing ) &&
                  get_entry( &entry_ICONVERTICALSPACING, 0, &lpIcon->iVertSpacing ) &&
                  get_entry( &entry_ICONTITLEWRAP, 0, &lpIcon->iTitleWrap ) &&
                  get_entry( &entry_ICONTITLELOGFONT, 0, &lpIcon->lfFont );
	}
	break;
    }
    case SPI_SETICONMETRICS:
    {
        LPICONMETRICSW lpIcon = pvParam;
        if (lpIcon && lpIcon->cbSize == sizeof(*lpIcon))
            ret = set_entry( &entry_ICONVERTICALSPACING, max(32,lpIcon->iVertSpacing), NULL, fWinIni ) &&
                  set_entry( &entry_ICONHORIZONTALSPACING, max(32,lpIcon->iHorzSpacing), NULL, fWinIni ) &&
                  set_entry( &entry_ICONTITLEWRAP, lpIcon->iTitleWrap, NULL, fWinIni ) &&
                  set_entry( &entry_ICONTITLELOGFONT, 0, &lpIcon->lfFont, fWinIni );
        break;
    }

    case SPI_SETWORKAREA:                       /*     47  WINVER >= 0x400 */
    {
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETWORKAREA_IDX;
        work_area = *(RECT*)pvParam;
        spi_loaded[spi_idx] = TRUE;
        ret = TRUE;
        break;
    }

    case SPI_GETWORKAREA:                       /*     48  WINVER >= 0x400 */
    {
        if (!pvParam) return FALSE;

        spi_idx = SPI_SETWORKAREA_IDX;
        if (!spi_loaded[spi_idx])
        {
            SetRect( &work_area, 0, 0,
                     GetSystemMetrics( SM_CXSCREEN ),
                     GetSystemMetrics( SM_CYSCREEN ) );
            EnumDisplayMonitors( 0, NULL, enum_monitors, (LPARAM)&work_area );
            spi_loaded[spi_idx] = TRUE;
        }
        *(RECT*)pvParam = work_area;
        ret = TRUE;
        TRACE("work area %s\n", wine_dbgstr_rect( &work_area ));
        break;
    }

    WINE_SPI_FIXME(SPI_SETPENWINDOWS);		/*     49  WINVER >= 0x400 */

    case SPI_GETFILTERKEYS:                     /*     50 */
    {
        LPFILTERKEYS lpFilterKeys = pvParam;
        WARN("SPI_GETFILTERKEYS not fully implemented\n");
        if (lpFilterKeys && lpFilterKeys->cbSize == sizeof(FILTERKEYS))
        {
            /* Indicate that no FilterKeys feature available */
            lpFilterKeys->dwFlags = 0;
            lpFilterKeys->iWaitMSec = 0;
            lpFilterKeys->iDelayMSec = 0;
            lpFilterKeys->iRepeatMSec = 0;
            lpFilterKeys->iBounceMSec = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETFILTERKEYS);		/*     51 */

    case SPI_GETTOGGLEKEYS:                     /*     52 */
    {
        LPTOGGLEKEYS lpToggleKeys = pvParam;
        WARN("SPI_GETTOGGLEKEYS not fully implemented\n");
        if (lpToggleKeys && lpToggleKeys->cbSize == sizeof(TOGGLEKEYS))
        {
            /* Indicate that no ToggleKeys feature available */
            lpToggleKeys->dwFlags = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETTOGGLEKEYS);		/*     53 */

    case SPI_GETMOUSEKEYS:                      /*     54 */
    {
        LPMOUSEKEYS lpMouseKeys = pvParam;
        WARN("SPI_GETMOUSEKEYS not fully implemented\n");
        if (lpMouseKeys && lpMouseKeys->cbSize == sizeof(MOUSEKEYS))
        {
            /* Indicate that no MouseKeys feature available */
            lpMouseKeys->dwFlags = 0;
            lpMouseKeys->iMaxSpeed = 360;
            lpMouseKeys->iTimeToMaxSpeed = 1000;
            lpMouseKeys->iCtrlSpeed = 0;
            lpMouseKeys->dwReserved1 = 0;
            lpMouseKeys->dwReserved2 = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETMOUSEKEYS);		/*     55 */

    case SPI_GETSHOWSOUNDS:
        ret = get_entry( &entry_SHOWSOUNDS, uiParam, pvParam );
        break;
    case SPI_SETSHOWSOUNDS:
        ret = set_entry( &entry_SHOWSOUNDS, uiParam, pvParam, fWinIni );
        break;

    case SPI_GETSTICKYKEYS:                     /*     58 */
    {
        LPSTICKYKEYS lpStickyKeys = pvParam;
        WARN("SPI_GETSTICKYKEYS not fully implemented\n");
        if (lpStickyKeys && lpStickyKeys->cbSize == sizeof(STICKYKEYS))
        {
            /* Indicate that no StickyKeys feature available */
            lpStickyKeys->dwFlags = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETSTICKYKEYS);		/*     59 */

    case SPI_GETACCESSTIMEOUT:                  /*     60 */
    {
        LPACCESSTIMEOUT lpAccessTimeout = pvParam;
        WARN("SPI_GETACCESSTIMEOUT not fully implemented\n");
        if (lpAccessTimeout && lpAccessTimeout->cbSize == sizeof(ACCESSTIMEOUT))
        {
            /* Indicate that no accessibility features timeout is available */
            lpAccessTimeout->dwFlags = 0;
            lpAccessTimeout->iTimeOutMSec = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETACCESSTIMEOUT);	/*     61 */

    case SPI_GETSERIALKEYS:                     /*     62  WINVER >= 0x400 */
    {
        LPSERIALKEYSW lpSerialKeysW = pvParam;
        WARN("SPI_GETSERIALKEYS not fully implemented\n");
        if (lpSerialKeysW && lpSerialKeysW->cbSize == sizeof(SERIALKEYSW))
        {
            /* Indicate that no SerialKeys feature available */
            lpSerialKeysW->dwFlags = 0;
            lpSerialKeysW->lpszActivePort = NULL;
            lpSerialKeysW->lpszPort = NULL;
            lpSerialKeysW->iBaudRate = 0;
            lpSerialKeysW->iPortState = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETSERIALKEYS);		/*     63  WINVER >= 0x400 */

    case SPI_GETSOUNDSENTRY:                    /*     64 */
    {
        LPSOUNDSENTRYW lpSoundSentryW = pvParam;
        WARN("SPI_GETSOUNDSENTRY not fully implemented\n");
        if (lpSoundSentryW && lpSoundSentryW->cbSize == sizeof(SOUNDSENTRYW))
        {
            /* Indicate that no SoundSentry feature available */
            lpSoundSentryW->dwFlags = 0;
            lpSoundSentryW->iFSTextEffect = 0;
            lpSoundSentryW->iFSTextEffectMSec = 0;
            lpSoundSentryW->iFSTextEffectColorBits = 0;
            lpSoundSentryW->iFSGrafEffect = 0;
            lpSoundSentryW->iFSGrafEffectMSec = 0;
            lpSoundSentryW->iFSGrafEffectColor = 0;
            lpSoundSentryW->iWindowsEffect = 0;
            lpSoundSentryW->iWindowsEffectMSec = 0;
            lpSoundSentryW->lpszWindowsEffectDLL = 0;
            lpSoundSentryW->iWindowsEffectOrdinal = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETSOUNDSENTRY);		/*     65 */

    case SPI_GETHIGHCONTRAST:			/*     66  WINVER >= 0x400 */
    {
        LPHIGHCONTRASTW lpHighContrastW = pvParam;
	WARN("SPI_GETHIGHCONTRAST not fully implemented\n");
	if (lpHighContrastW && lpHighContrastW->cbSize == sizeof(HIGHCONTRASTW))
	{
	    /* Indicate that no high contrast feature available */
	    lpHighContrastW->dwFlags = 0;
	    lpHighContrastW->lpszDefaultScheme = NULL;
            ret = TRUE;
	}
	break;
    }
    WINE_SPI_FIXME(SPI_SETHIGHCONTRAST);	/*     67  WINVER >= 0x400 */

    case SPI_GETKEYBOARDPREF:
        ret = get_entry( &entry_KEYBOARDPREF, uiParam, pvParam );
        break;
    case SPI_SETKEYBOARDPREF:
        ret = set_entry( &entry_KEYBOARDPREF, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETSCREENREADER:
        ret = get_entry( &entry_SCREENREADER, uiParam, pvParam );
        break;
    case SPI_SETSCREENREADER:
        ret = set_entry( &entry_SCREENREADER, uiParam, pvParam, fWinIni );
        break;

    case SPI_GETANIMATION:			/*     72  WINVER >= 0x400 */
    {
        LPANIMATIONINFO lpAnimInfo = pvParam;

	/* Tell it "disabled" */
	if (lpAnimInfo && lpAnimInfo->cbSize == sizeof(ANIMATIONINFO))
        {
	    lpAnimInfo->iMinAnimate = 0; /* Minimize and restore animation is disabled (nonzero == enabled) */
            ret = TRUE;
        }
	break;
    }
    WINE_SPI_WARN(SPI_SETANIMATION);		/*     73  WINVER >= 0x400 */

    case SPI_GETFONTSMOOTHING:
        ret = get_entry( &entry_FONTSMOOTHING, uiParam, pvParam );
        if (ret) *(UINT *)pvParam = (*(UINT *)pvParam != 0);
        break;
    case SPI_SETFONTSMOOTHING:
        uiParam = uiParam ? 2 : 0; /* Win NT4/2k/XP behavior */
        ret = set_entry( &entry_FONTSMOOTHING, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDRAGWIDTH:
        ret = set_entry( &entry_DRAGWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETDRAGHEIGHT:
        ret = set_entry( &entry_DRAGHEIGHT, uiParam, pvParam, fWinIni );
        break;

    WINE_SPI_FIXME(SPI_SETHANDHELD);		/*     78  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_GETLOWPOWERTIMEOUT);	/*     79  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_GETPOWEROFFTIMEOUT);	/*     80  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETLOWPOWERTIMEOUT);	/*     81  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETPOWEROFFTIMEOUT);	/*     82  WINVER >= 0x400 */

    case SPI_GETLOWPOWERACTIVE:
        ret = get_entry( &entry_LOWPOWERACTIVE, uiParam, pvParam );
        break;
    case SPI_SETLOWPOWERACTIVE:
        ret = set_entry( &entry_LOWPOWERACTIVE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETPOWEROFFACTIVE:
        ret = get_entry( &entry_POWEROFFACTIVE, uiParam, pvParam );
        break;
    case SPI_SETPOWEROFFACTIVE:
        ret = set_entry( &entry_POWEROFFACTIVE, uiParam, pvParam, fWinIni );
        break;

    WINE_SPI_FIXME(SPI_SETCURSORS);		/*     87  WINVER >= 0x400 */
    WINE_SPI_FIXME(SPI_SETICONS);		/*     88  WINVER >= 0x400 */

    case SPI_GETDEFAULTINPUTLANG: 	/*     89  WINVER >= 0x400 */
        ret = GetKeyboardLayout(0) != 0;
        break;

    WINE_SPI_FIXME(SPI_SETDEFAULTINPUTLANG);	/*     90  WINVER >= 0x400 */

    WINE_SPI_FIXME(SPI_SETLANGTOGGLE);		/*     91  WINVER >= 0x400 */

    case SPI_GETWINDOWSEXTENSION:		/*     92  WINVER >= 0x400 */
	WARN("pretend no support for Win9x Plus! for now.\n");
	ret = FALSE; /* yes, this is the result value */
	break;
    case SPI_SETMOUSETRAILS:
        ret = set_entry( &entry_MOUSETRAILS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSETRAILS:
        ret = get_entry( &entry_MOUSETRAILS, uiParam, pvParam );
        break;
    case SPI_GETSNAPTODEFBUTTON:
        ret = get_entry( &entry_SNAPTODEFBUTTON, uiParam, pvParam );
        break;
    case SPI_SETSNAPTODEFBUTTON:
        ret = set_entry( &entry_SNAPTODEFBUTTON, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETSCREENSAVERRUNNING:
        ret = set_entry( &entry_SCREENSAVERRUNNING, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSEHOVERWIDTH:
        ret = get_entry( &entry_MOUSEHOVERWIDTH, uiParam, pvParam );
        break;
    case SPI_SETMOUSEHOVERWIDTH:
        ret = set_entry( &entry_MOUSEHOVERWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSEHOVERHEIGHT:
        ret = get_entry( &entry_MOUSEHOVERHEIGHT, uiParam, pvParam );
        break;
    case SPI_SETMOUSEHOVERHEIGHT:
        ret = set_entry( &entry_MOUSEHOVERHEIGHT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSEHOVERTIME:
        ret = get_entry( &entry_MOUSEHOVERTIME, uiParam, pvParam );
        break;
    case SPI_SETMOUSEHOVERTIME:
        ret = set_entry( &entry_MOUSEHOVERTIME, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETWHEELSCROLLLINES:
        ret = get_entry( &entry_WHEELSCROLLLINES, uiParam, pvParam );
        break;
    case SPI_SETWHEELSCROLLLINES:
        ret = set_entry( &entry_WHEELSCROLLLINES, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMENUSHOWDELAY:
        ret = get_entry( &entry_MENUSHOWDELAY, uiParam, pvParam );
        break;
    case SPI_SETMENUSHOWDELAY:
        ret = set_entry( &entry_MENUSHOWDELAY, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETWHEELSCROLLCHARS:
        ret = get_entry( &entry_WHEELSCROLLCHARS, uiParam, pvParam );
        break;
    case SPI_SETWHEELSCROLLCHARS:
        ret = set_entry( &entry_WHEELSCROLLCHARS, uiParam, pvParam, fWinIni );
        break;

    WINE_SPI_FIXME(SPI_GETSHOWIMEUI);		/*    110  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */
    WINE_SPI_FIXME(SPI_SETSHOWIMEUI);		/*    111  _WIN32_WINNT >= 0x400 || _WIN32_WINDOW > 0x400 */

    case SPI_GETMOUSESPEED:
        ret = get_entry( &entry_MOUSESPEED, uiParam, pvParam );
        break;
    case SPI_SETMOUSESPEED:
        ret = set_entry( &entry_MOUSESPEED, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETSCREENSAVERRUNNING:
        ret = get_entry( &entry_SCREENSAVERRUNNING, uiParam, pvParam );
        break;
    case SPI_GETDESKWALLPAPER:
        ret = get_entry( &entry_DESKWALLPAPER, uiParam, pvParam );
        break;
    case SPI_GETACTIVEWINDOWTRACKING:
        ret = get_entry( &entry_ACTIVEWINDOWTRACKING, uiParam, pvParam );
        break;
    case SPI_SETACTIVEWINDOWTRACKING:
        ret = set_entry( &entry_ACTIVEWINDOWTRACKING, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMENUANIMATION:
        ret = get_entry( &entry_MENUANIMATION, uiParam, pvParam );
        break;
    case SPI_SETMENUANIMATION:
        ret = set_entry( &entry_MENUANIMATION, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETCOMBOBOXANIMATION:
        ret = get_entry( &entry_COMBOBOXANIMATION, uiParam, pvParam );
        break;
    case SPI_SETCOMBOBOXANIMATION:
        ret = set_entry( &entry_COMBOBOXANIMATION, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETLISTBOXSMOOTHSCROLLING:
        ret = get_entry( &entry_LISTBOXSMOOTHSCROLLING, uiParam, pvParam );
        break;
    case SPI_SETLISTBOXSMOOTHSCROLLING:
        ret = set_entry( &entry_LISTBOXSMOOTHSCROLLING, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETGRADIENTCAPTIONS:
        ret = get_entry( &entry_GRADIENTCAPTIONS, uiParam, pvParam );
        break;
    case SPI_SETGRADIENTCAPTIONS:
        ret = set_entry( &entry_GRADIENTCAPTIONS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETKEYBOARDCUES:
        ret = get_entry( &entry_KEYBOARDCUES, uiParam, pvParam );
        break;
    case SPI_SETKEYBOARDCUES:
        ret = set_entry( &entry_KEYBOARDCUES, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETACTIVEWNDTRKZORDER:
        ret = get_entry( &entry_ACTIVEWNDTRKZORDER, uiParam, pvParam );
        break;
    case SPI_SETACTIVEWNDTRKZORDER:
        ret = set_entry( &entry_ACTIVEWNDTRKZORDER, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETHOTTRACKING:
        ret = get_entry( &entry_HOTTRACKING, uiParam, pvParam );
        break;
    case SPI_SETHOTTRACKING:
        ret = set_entry( &entry_HOTTRACKING, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMENUFADE:
        ret = get_entry( &entry_MENUFADE, uiParam, pvParam );
        break;
    case SPI_SETMENUFADE:
        ret = set_entry( &entry_MENUFADE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETSELECTIONFADE:
        ret = get_entry( &entry_SELECTIONFADE, uiParam, pvParam );
        break;
    case SPI_SETSELECTIONFADE:
        ret = set_entry( &entry_SELECTIONFADE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETTOOLTIPANIMATION:
        ret = get_entry( &entry_TOOLTIPANIMATION, uiParam, pvParam );
        break;
    case SPI_SETTOOLTIPANIMATION:
        ret = set_entry( &entry_TOOLTIPANIMATION, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETTOOLTIPFADE:
        ret = get_entry( &entry_TOOLTIPFADE, uiParam, pvParam );
        break;
    case SPI_SETTOOLTIPFADE:
        ret = set_entry( &entry_TOOLTIPFADE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETCURSORSHADOW:
        ret = get_entry( &entry_CURSORSHADOW, uiParam, pvParam );
        break;
    case SPI_SETCURSORSHADOW:
        ret = set_entry( &entry_CURSORSHADOW, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSESONAR:
        ret = get_entry( &entry_MOUSESONAR, uiParam, pvParam );
        break;
    case SPI_SETMOUSESONAR:
        ret = set_entry( &entry_MOUSESONAR, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSECLICKLOCK:
        ret = get_entry( &entry_MOUSECLICKLOCK, uiParam, pvParam );
        break;
    case SPI_SETMOUSECLICKLOCK:
        ret = set_entry( &entry_MOUSECLICKLOCK, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSEVANISH:
        ret = get_entry( &entry_MOUSEVANISH, uiParam, pvParam );
        break;
    case SPI_SETMOUSEVANISH:
        ret = set_entry( &entry_MOUSEVANISH, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFLATMENU:
        ret = get_entry( &entry_FLATMENU, uiParam, pvParam );
        break;
    case SPI_SETFLATMENU:
        ret = set_entry( &entry_FLATMENU, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETDROPSHADOW:
        ret = get_entry( &entry_DROPSHADOW, uiParam, pvParam );
        break;
    case SPI_SETDROPSHADOW:
        ret = set_entry( &entry_DROPSHADOW, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETBLOCKSENDINPUTRESETS:
        ret = get_entry( &entry_BLOCKSENDINPUTRESETS, uiParam, pvParam );
        break;
    case SPI_SETBLOCKSENDINPUTRESETS:
        ret = set_entry( &entry_BLOCKSENDINPUTRESETS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETUIEFFECTS:
        ret = get_entry( &entry_UIEFFECTS, uiParam, pvParam );
        break;
    case SPI_SETUIEFFECTS:
        /* FIXME: this probably should mask other UI effect values when unset */
        ret = set_entry( &entry_UIEFFECTS, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETDISABLEOVERLAPPEDCONTENT:
        ret = get_entry( &entry_DISABLEOVERLAPPEDCONTENT, uiParam, pvParam );
        break;
    case SPI_SETDISABLEOVERLAPPEDCONTENT:
        ret = set_entry( &entry_DISABLEOVERLAPPEDCONTENT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETCLIENTAREAANIMATION:
        ret = get_entry( &entry_CLIENTAREAANIMATION, uiParam, pvParam );
        break;
    case SPI_SETCLIENTAREAANIMATION:
        ret = set_entry( &entry_CLIENTAREAANIMATION, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETCLEARTYPE:
        ret = get_entry( &entry_CLEARTYPE, uiParam, pvParam );
        break;
    case SPI_SETCLEARTYPE:
        ret = set_entry( &entry_CLEARTYPE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETSPEECHRECOGNITION:
        ret = get_entry( &entry_SPEECHRECOGNITION, uiParam, pvParam );
        break;
    case SPI_SETSPEECHRECOGNITION:
        ret = set_entry( &entry_SPEECHRECOGNITION, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFOREGROUNDLOCKTIMEOUT:
        ret = get_entry( &entry_FOREGROUNDLOCKTIMEOUT, uiParam, pvParam );
        break;
    case SPI_SETFOREGROUNDLOCKTIMEOUT:
        /* FIXME: this should check that the calling thread
         * is able to change the foreground window */
        ret = set_entry( &entry_FOREGROUNDLOCKTIMEOUT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETACTIVEWNDTRKTIMEOUT:
        ret = get_entry( &entry_ACTIVEWNDTRKTIMEOUT, uiParam, pvParam );
        break;
    case SPI_SETACTIVEWNDTRKTIMEOUT:
        ret = get_entry( &entry_ACTIVEWNDTRKTIMEOUT, uiParam, pvParam );
        break;
    case SPI_GETFOREGROUNDFLASHCOUNT:
        ret = get_entry( &entry_FOREGROUNDFLASHCOUNT, uiParam, pvParam );
        break;
    case SPI_SETFOREGROUNDFLASHCOUNT:
        ret = set_entry( &entry_FOREGROUNDFLASHCOUNT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETCARETWIDTH:
        ret = get_entry( &entry_CARETWIDTH, uiParam, pvParam );
        break;
    case SPI_SETCARETWIDTH:
        ret = set_entry( &entry_CARETWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETMOUSECLICKLOCKTIME:
        ret = get_entry( &entry_MOUSECLICKLOCKTIME, uiParam, pvParam );
        break;
    case SPI_SETMOUSECLICKLOCKTIME:
        ret = set_entry( &entry_MOUSECLICKLOCKTIME, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFONTSMOOTHINGTYPE:
        ret = get_entry( &entry_FONTSMOOTHINGTYPE, uiParam, pvParam );
        break;
    case SPI_SETFONTSMOOTHINGTYPE:
        ret = set_entry( &entry_FONTSMOOTHINGTYPE, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFONTSMOOTHINGCONTRAST:
        ret = get_entry( &entry_FONTSMOOTHINGCONTRAST, uiParam, pvParam );
        break;
    case SPI_SETFONTSMOOTHINGCONTRAST:
        ret = set_entry( &entry_FONTSMOOTHINGCONTRAST, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFOCUSBORDERWIDTH:
        ret = get_entry( &entry_FOCUSBORDERWIDTH, uiParam, pvParam );
        break;
    case SPI_GETFOCUSBORDERHEIGHT:
        ret = get_entry( &entry_FOCUSBORDERHEIGHT, uiParam, pvParam );
        break;
    case SPI_SETFOCUSBORDERWIDTH:
        ret = set_entry( &entry_FOCUSBORDERWIDTH, uiParam, pvParam, fWinIni );
        break;
    case SPI_SETFOCUSBORDERHEIGHT:
        ret = set_entry( &entry_FOCUSBORDERHEIGHT, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETFONTSMOOTHINGORIENTATION:
        ret = get_entry( &entry_FONTSMOOTHINGORIENTATION, uiParam, pvParam );
        break;
    case SPI_SETFONTSMOOTHINGORIENTATION:
        ret = set_entry( &entry_FONTSMOOTHINGORIENTATION, uiParam, pvParam, fWinIni );
        break;
    case SPI_GETAUDIODESCRIPTION:
    {
        AUDIODESCRIPTION *audio = pvParam;
        if (audio && audio->cbSize == sizeof(AUDIODESCRIPTION) && uiParam == sizeof(AUDIODESCRIPTION) )
        {
            ret = get_entry( &entry_AUDIODESC_ON, 0, &audio->Enabled ) &&
                  get_entry( &entry_AUDIODESC_LOCALE, 0, &audio->Locale );
        }
        break;
    }
    case SPI_SETAUDIODESCRIPTION:
    {
        AUDIODESCRIPTION *audio = pvParam;
        if (audio && audio->cbSize == sizeof(AUDIODESCRIPTION) && uiParam == sizeof(AUDIODESCRIPTION) )
        {
            ret = set_entry( &entry_AUDIODESC_ON, 0, &audio->Enabled, fWinIni) &&
                  set_entry( &entry_AUDIODESC_LOCALE, 0, &audio->Locale, fWinIni );
        }
        break;
    }
    default:
	FIXME( "Unknown action: %u\n", uiAction );
	SetLastError( ERROR_INVALID_SPI_VALUE );
	ret = FALSE;
	break;
    }

    if (ret)
        SYSPARAMS_NotifyChange( uiAction, fWinIni );
    TRACE("(%u, %u, %p, %u) ret %d\n",
            uiAction, uiParam, pvParam, fWinIni, ret);
    return ret;

#undef WINE_SPI_FIXME
#undef WINE_SPI_WARN
}


/***********************************************************************
 *		SystemParametersInfoA (USER32.@)
 */
BOOL WINAPI SystemParametersInfoA( UINT uiAction, UINT uiParam,
				   PVOID pvParam, UINT fuWinIni )
{
    BOOL ret;

    TRACE("(%u, %u, %p, %u)\n", uiAction, uiParam, pvParam, fuWinIni);

    switch (uiAction)
    {
    case SPI_SETDESKWALLPAPER:			/*     20 */
    case SPI_SETDESKPATTERN:			/*     21 */
    {
	WCHAR buffer[256];
	if (pvParam)
            if (!MultiByteToWideChar( CP_ACP, 0, pvParam, -1, buffer, ARRAY_SIZE( buffer )))
                buffer[ARRAY_SIZE(buffer)-1] = 0;
	ret = SystemParametersInfoW( uiAction, uiParam, pvParam ? buffer : NULL, fuWinIni );
	break;
    }

    case SPI_GETICONTITLELOGFONT:		/*     31 */
    {
	LOGFONTW tmp;
	ret = SystemParametersInfoW( uiAction, uiParam, pvParam ? &tmp : NULL, fuWinIni );
	if (ret && pvParam)
            SYSPARAMS_LogFont32WTo32A( &tmp, pvParam );
	break;
    }

    case SPI_GETNONCLIENTMETRICS: 		/*     41  WINVER >= 0x400 */
    {
	NONCLIENTMETRICSW tmp;
        LPNONCLIENTMETRICSA lpnmA = pvParam;
        if (lpnmA && (lpnmA->cbSize == sizeof(NONCLIENTMETRICSA) ||
                      lpnmA->cbSize == FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth)))
	{
	    tmp.cbSize = sizeof(NONCLIENTMETRICSW);
	    ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
		SYSPARAMS_NonClientMetrics32WTo32A( &tmp, lpnmA );
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_SETNONCLIENTMETRICS: 		/*     42  WINVER >= 0x400 */
    {
        NONCLIENTMETRICSW tmp;
        LPNONCLIENTMETRICSA lpnmA = pvParam;
        if (lpnmA && (lpnmA->cbSize == sizeof(NONCLIENTMETRICSA) ||
                      lpnmA->cbSize == FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth)))
        {
            tmp.cbSize = sizeof(NONCLIENTMETRICSW);
            SYSPARAMS_NonClientMetrics32ATo32W( lpnmA, &tmp );
            ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETICONMETRICS:			/*     45  WINVER >= 0x400 */
    {
	ICONMETRICSW tmp;
        LPICONMETRICSA lpimA = pvParam;
	if (lpimA && lpimA->cbSize == sizeof(ICONMETRICSA))
	{
	    tmp.cbSize = sizeof(ICONMETRICSW);
	    ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
	    {
		lpimA->iHorzSpacing = tmp.iHorzSpacing;
		lpimA->iVertSpacing = tmp.iVertSpacing;
		lpimA->iTitleWrap   = tmp.iTitleWrap;
		SYSPARAMS_LogFont32WTo32A( &tmp.lfFont, &lpimA->lfFont );
	    }
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_SETICONMETRICS:			/*     46  WINVER >= 0x400 */
    {
        ICONMETRICSW tmp;
        LPICONMETRICSA lpimA = pvParam;
        if (lpimA && lpimA->cbSize == sizeof(ICONMETRICSA))
        {
            tmp.cbSize = sizeof(ICONMETRICSW);
            tmp.iHorzSpacing = lpimA->iHorzSpacing;
            tmp.iVertSpacing = lpimA->iVertSpacing;
            tmp.iTitleWrap = lpimA->iTitleWrap;
            SYSPARAMS_LogFont32ATo32W(  &lpimA->lfFont, &tmp.lfFont);
            ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
        }
        else
            ret = FALSE;
        break;
    }

    case SPI_GETHIGHCONTRAST:			/*     66  WINVER >= 0x400 */
    {
	HIGHCONTRASTW tmp;
        LPHIGHCONTRASTA lphcA = pvParam;
	if (lphcA && lphcA->cbSize == sizeof(HIGHCONTRASTA))
	{
	    tmp.cbSize = sizeof(HIGHCONTRASTW);
	    ret = SystemParametersInfoW( uiAction, uiParam, &tmp, fuWinIni );
	    if (ret)
	    {
		lphcA->dwFlags = tmp.dwFlags;
		lphcA->lpszDefaultScheme = NULL;  /* FIXME? */
	    }
	}
	else
	    ret = FALSE;
	break;
    }

    case SPI_GETDESKWALLPAPER:                  /*     115 */
    {
        WCHAR buffer[MAX_PATH];
        ret = (SystemParametersInfoW( SPI_GETDESKWALLPAPER, uiParam, buffer, fuWinIni ) &&
               WideCharToMultiByte(CP_ACP, 0, buffer, -1, pvParam, uiParam, NULL, NULL));
        break;
    }

    default:
        ret = SystemParametersInfoW( uiAction, uiParam, pvParam, fuWinIni );
        break;
    }
    return ret;
}


/***********************************************************************
 *		GetSystemMetrics (USER32.@)
 */
INT WINAPI GetSystemMetrics( INT index )
{
    struct monitor_info info;
    NONCLIENTMETRICSW ncm;
    MINIMIZEDMETRICS mm;
    ICONMETRICSW im;
    UINT ret;
    HDC hdc;

    /* some metrics are dynamic */
    switch (index)
    {
    case SM_CXVSCROLL:
    case SM_CYHSCROLL:
        get_entry( &entry_SCROLLWIDTH, 0, &ret );
        return max( ret, 8 );
    case SM_CYCAPTION:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iCaptionHeight + 1;
    case SM_CXBORDER:
    case SM_CYBORDER:
        /* SM_C{X,Y}BORDER always returns 1 regardless of 'BorderWidth' value in registry */
        return 1;
    case SM_CXDLGFRAME:
    case SM_CYDLGFRAME:
        return 3;
    case SM_CYVTHUMB:
    case SM_CXHTHUMB:
    case SM_CYVSCROLL:
    case SM_CXHSCROLL:
        get_entry( &entry_SCROLLHEIGHT, 0, &ret );
        return max( ret, 8 );
    case SM_CXICON:
    case SM_CYICON:
        return map_to_dpi( 32, GetDpiForSystem() );
    case SM_CXCURSOR:
    case SM_CYCURSOR:
        ret = map_to_dpi( 32, GetDpiForSystem() );
        if (ret >= 64) return 64;
        if (ret >= 48) return 48;
        return 32;
    case SM_CYMENU:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iMenuHeight + 1;
    case SM_CXFULLSCREEN:
        /* see the remark for SM_CXMAXIMIZED, at least this formulation is
         * correct */
        return GetSystemMetrics( SM_CXMAXIMIZED) - 2 * GetSystemMetrics( SM_CXFRAME);
    case SM_CYFULLSCREEN:
        /* see the remark for SM_CYMAXIMIZED, at least this formulation is
         * correct */
        return GetSystemMetrics( SM_CYMAXIMIZED) - GetSystemMetrics( SM_CYMIN);
    case SM_CYKANJIWINDOW:
        return 0;
    case SM_MOUSEPRESENT:
        return 1;
    case SM_DEBUG:
        return 0;
    case SM_SWAPBUTTON:
        get_entry( &entry_MOUSEBUTTONSWAP, 0, &ret );
        return ret;
    case SM_RESERVED1:
    case SM_RESERVED2:
    case SM_RESERVED3:
    case SM_RESERVED4:
        return 0;
    case SM_CXMIN:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        hdc = get_display_dc();
        get_text_metr_size( hdc, &ncm.lfCaptionFont, NULL, &ret );
        release_display_dc( hdc );
        return 3 * ncm.iCaptionWidth + ncm.iCaptionHeight + 4 * ret + 2 * GetSystemMetrics(SM_CXFRAME) + 4;
    case SM_CYMIN:
        return GetSystemMetrics( SM_CYCAPTION) + 2 * GetSystemMetrics( SM_CYFRAME);
    case SM_CXSIZE:
        get_entry( &entry_CAPTIONWIDTH, 0, &ret );
        return max( ret, 8 );
    case SM_CYSIZE:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iCaptionHeight;
    case SM_CXFRAME:
        get_entry( &entry_BORDER, 0, &ret );
        ret = max( ret, 1 );
        return GetSystemMetrics(SM_CXDLGFRAME) + ret;
    case SM_CYFRAME:
        get_entry( &entry_BORDER, 0, &ret );
        ret = max( ret, 1 );
        return GetSystemMetrics(SM_CYDLGFRAME) + ret;
    case SM_CXMINTRACK:
        return GetSystemMetrics(SM_CXMIN);
    case SM_CYMINTRACK:
        return GetSystemMetrics(SM_CYMIN);
    case SM_CXDOUBLECLK:
        get_entry( &entry_DOUBLECLKWIDTH, 0, &ret );
        return ret;
    case SM_CYDOUBLECLK:
        get_entry( &entry_DOUBLECLKHEIGHT, 0, &ret );
        return ret;
    case SM_CXICONSPACING:
        im.cbSize = sizeof(im);
        SystemParametersInfoW( SPI_GETICONMETRICS, sizeof(im), &im, 0 );
        return im.iHorzSpacing;
    case SM_CYICONSPACING:
        im.cbSize = sizeof(im);
        SystemParametersInfoW( SPI_GETICONMETRICS, sizeof(im), &im, 0 );
        return im.iVertSpacing;
    case SM_MENUDROPALIGNMENT:
        SystemParametersInfoW( SPI_GETMENUDROPALIGNMENT, 0, &ret, 0 );
        return ret;
    case SM_PENWINDOWS:
        return 0;
    case SM_DBCSENABLED:
    {
        CPINFO cpinfo;
        GetCPInfo( CP_ACP, &cpinfo );
        return (cpinfo.MaxCharSize > 1);
    }
    case SM_CMOUSEBUTTONS:
        return 3;
    case SM_SECURE:
        return 0;
    case SM_CXEDGE:
        return GetSystemMetrics(SM_CXBORDER) + 1;
    case SM_CYEDGE:
        return GetSystemMetrics(SM_CYBORDER) + 1;
    case SM_CXMINSPACING:
        mm.cbSize = sizeof(mm);
        SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return GetSystemMetrics(SM_CXMINIMIZED) + mm.iHorzGap;
    case SM_CYMINSPACING:
        mm.cbSize = sizeof(mm);
        SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return GetSystemMetrics(SM_CYMINIMIZED) + mm.iVertGap;
    case SM_CXSMICON:
    case SM_CYSMICON:
        return map_to_dpi( 16, GetDpiForSystem() ) & ~1;
    case SM_CYSMCAPTION:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iSmCaptionHeight + 1;
    case SM_CXSMSIZE:
        get_entry( &entry_SMCAPTIONWIDTH, 0, &ret );
        return ret;
    case SM_CYSMSIZE:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iSmCaptionHeight;
    case SM_CXMENUSIZE:
        get_entry( &entry_MENUWIDTH, 0, &ret );
        return ret;
    case SM_CYMENUSIZE:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iMenuHeight;
    case SM_ARRANGE:
        mm.cbSize = sizeof(mm);
        SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return mm.iArrange;
    case SM_CXMINIMIZED:
        mm.cbSize = sizeof(mm);
        SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return mm.iWidth + 6;
    case SM_CYMINIMIZED:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iCaptionHeight + 6;
    case SM_CXMAXTRACK:
        return GetSystemMetrics(SM_CXVIRTUALSCREEN) + 4 + 2 * GetSystemMetrics(SM_CXFRAME);
    case SM_CYMAXTRACK:
        return GetSystemMetrics(SM_CYVIRTUALSCREEN) + 4 + 2 * GetSystemMetrics(SM_CYFRAME);
    case SM_CXMAXIMIZED:
        /* FIXME: subtract the width of any vertical application toolbars*/
        return GetSystemMetrics(SM_CXSCREEN) + 2 * GetSystemMetrics(SM_CXFRAME);
    case SM_CYMAXIMIZED:
        /* FIXME: subtract the width of any horizontal application toolbars*/
        return GetSystemMetrics(SM_CYSCREEN) + 2 * GetSystemMetrics(SM_CYCAPTION);
    case SM_NETWORK:
        return 3;  /* FIXME */
    case SM_CLEANBOOT:
        return 0; /* 0 = ok, 1 = failsafe, 2 = failsafe + network */
    case SM_CXDRAG:
        get_entry( &entry_DRAGWIDTH, 0, &ret );
        return ret;
    case SM_CYDRAG:
        get_entry( &entry_DRAGHEIGHT, 0, &ret );
        return ret;
    case SM_SHOWSOUNDS:
        get_entry( &entry_SHOWSOUNDS, 0, &ret );
        return ret;
    case SM_CXMENUCHECK:
    case SM_CYMENUCHECK:
    {
        TEXTMETRICW tm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        hdc = get_display_dc();
        get_text_metr_size( hdc, &ncm.lfMenuFont, &tm, NULL);
        release_display_dc( hdc );
        return tm.tmHeight <= 0 ? 13 : ((tm.tmHeight + tm.tmExternalLeading + 1) / 2) * 2 - 1;
    }
    case SM_SLOWMACHINE:
        return 0;  /* Never true */
    case SM_MIDEASTENABLED:
        return 0;  /* FIXME */
    case SM_MOUSEWHEELPRESENT:
        return 1;
    case SM_CXSCREEN:
        get_monitors_info( &info );
        return info.primary_rect.right - info.primary_rect.left;
    case SM_CYSCREEN:
        get_monitors_info( &info );
        return info.primary_rect.bottom - info.primary_rect.top;
    case SM_XVIRTUALSCREEN:
        get_monitors_info( &info );
        return info.virtual_rect.left;
    case SM_YVIRTUALSCREEN:
        get_monitors_info( &info );
        return info.virtual_rect.top;
    case SM_CXVIRTUALSCREEN:
        get_monitors_info( &info );
        return info.virtual_rect.right - info.virtual_rect.left;
    case SM_CYVIRTUALSCREEN:
        get_monitors_info( &info );
        return info.virtual_rect.bottom - info.virtual_rect.top;
    case SM_CMONITORS:
        get_monitors_info( &info );
        return info.count;
    case SM_SAMEDISPLAYFORMAT:
        return 1;
    case SM_IMMENABLED:
        return 0;  /* FIXME */
    case SM_CXFOCUSBORDER:
    case SM_CYFOCUSBORDER:
        return 1;
    case SM_TABLETPC:
    case SM_MEDIACENTER:
        return 0;
    case SM_CMETRICS:
        return SM_CMETRICS;
    default:
        return 0;
    }
}


/***********************************************************************
 *		GetSystemMetricsForDpi (USER32.@)
 */
INT WINAPI GetSystemMetricsForDpi( INT index, UINT dpi )
{
    NONCLIENTMETRICSW ncm;
    ICONMETRICSW im;
    UINT ret;
    HDC hdc;

    /* some metrics are dynamic */
    switch (index)
    {
    case SM_CXVSCROLL:
    case SM_CYHSCROLL:
        get_entry_dpi( &entry_SCROLLWIDTH, 0, &ret, dpi );
        return max( ret, 8 );
    case SM_CYCAPTION:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iCaptionHeight + 1;
    case SM_CYVTHUMB:
    case SM_CXHTHUMB:
    case SM_CYVSCROLL:
    case SM_CXHSCROLL:
        get_entry_dpi( &entry_SCROLLHEIGHT, 0, &ret, dpi );
        return max( ret, 8 );
    case SM_CXICON:
    case SM_CYICON:
        return map_to_dpi( 32, dpi );
    case SM_CXCURSOR:
    case SM_CYCURSOR:
        ret = map_to_dpi( 32, dpi );
        if (ret >= 64) return 64;
        if (ret >= 48) return 48;
        return 32;
    case SM_CYMENU:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iMenuHeight + 1;
    case SM_CXSIZE:
        get_entry_dpi( &entry_CAPTIONWIDTH, 0, &ret, dpi );
        return max( ret, 8 );
    case SM_CYSIZE:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iCaptionHeight;
    case SM_CXFRAME:
        get_entry_dpi( &entry_BORDER, 0, &ret, dpi );
        ret = max( ret, 1 );
        return GetSystemMetricsForDpi( SM_CXDLGFRAME, dpi ) + ret;
    case SM_CYFRAME:
        get_entry_dpi( &entry_BORDER, 0, &ret, dpi );
        ret = max( ret, 1 );
        return GetSystemMetricsForDpi( SM_CYDLGFRAME, dpi ) + ret;
    case SM_CXICONSPACING:
        im.cbSize = sizeof(im);
        SystemParametersInfoForDpi( SPI_GETICONMETRICS, sizeof(im), &im, 0, dpi );
        return im.iHorzSpacing;
    case SM_CYICONSPACING:
        im.cbSize = sizeof(im);
        SystemParametersInfoForDpi( SPI_GETICONMETRICS, sizeof(im), &im, 0, dpi );
        return im.iVertSpacing;
    case SM_CXSMICON:
    case SM_CYSMICON:
        return map_to_dpi( 16, dpi ) & ~1;
    case SM_CYSMCAPTION:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iSmCaptionHeight + 1;
    case SM_CXSMSIZE:
        get_entry_dpi( &entry_SMCAPTIONWIDTH, 0, &ret, dpi );
        return ret;
    case SM_CYSMSIZE:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iSmCaptionHeight;
    case SM_CXMENUSIZE:
        get_entry_dpi( &entry_MENUWIDTH, 0, &ret, dpi );
        return ret;
    case SM_CYMENUSIZE:
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iMenuHeight;
    case SM_CXMENUCHECK:
    case SM_CYMENUCHECK:
    {
        TEXTMETRICW tm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        hdc = get_display_dc();
        get_text_metr_size( hdc, &ncm.lfMenuFont, &tm, NULL);
        release_display_dc( hdc );
        return tm.tmHeight <= 0 ? 13 : ((tm.tmHeight + tm.tmExternalLeading - 1) | 1);
    }
    default:
        return GetSystemMetrics( index );
    }
}


/***********************************************************************
 *		SwapMouseButton (USER32.@)
 *  Reverse or restore the meaning of the left and right mouse buttons
 *  fSwap  [I ] TRUE - reverse, FALSE - original
 * RETURN
 *   previous state 
 */
BOOL WINAPI SwapMouseButton( BOOL fSwap )
{
    BOOL prev = GetSystemMetrics(SM_SWAPBUTTON);
    SystemParametersInfoW(SPI_SETMOUSEBUTTONSWAP, fSwap, 0, 0);
    return prev;
}


/**********************************************************************
 *		SetDoubleClickTime (USER32.@)
 */
BOOL WINAPI SetDoubleClickTime( UINT interval )
{
    return SystemParametersInfoW(SPI_SETDOUBLECLICKTIME, interval, 0, 0);
}


/**********************************************************************
 *		GetDoubleClickTime (USER32.@)
 */
UINT WINAPI GetDoubleClickTime(void)
{
    UINT time = 0;

    get_entry( &entry_DOUBLECLICKTIME, 0, &time );
    if (!time) time = 500;
    return time;
}


/*************************************************************************
 *		GetSysColor (USER32.@)
 */
COLORREF WINAPI DECLSPEC_HOTPATCH GetSysColor( INT nIndex )
{
    COLORREF ret = 0;

    if (nIndex >= 0 && nIndex < ARRAY_SIZE( system_colors ))
        get_entry( &system_colors[nIndex], 0, &ret );
    return ret;
}


/*************************************************************************
 *		SetSysColors (USER32.@)
 */
BOOL WINAPI SetSysColors( INT count, const INT *colors, const COLORREF *values )
{
    int i;

    if (IS_INTRESOURCE(colors)) return FALSE; /* stupid app passes a color instead of an array */

    for (i = 0; i < count; i++)
        if (colors[i] >= 0 && colors[i] <= ARRAY_SIZE( system_colors ))
            set_entry( &system_colors[colors[i]], values[i], 0, 0 );

    /* Send WM_SYSCOLORCHANGE message to all windows */

    SendMessageTimeoutW( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0, SMTO_ABORTIFHUNG, 2000, NULL );

    /* Repaint affected portions of all visible windows */

    RedrawWindow( 0, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}


/*************************************************************************
 *		SetSysColorsTemp (USER32.@)
 */
DWORD_PTR WINAPI SetSysColorsTemp( const COLORREF *pPens, const HBRUSH *pBrushes, DWORD_PTR n)
{
    FIXME( "no longer supported\n" );
    return FALSE;
}


/***********************************************************************
 *		GetSysColorBrush (USER32.@)
 */
HBRUSH WINAPI DECLSPEC_HOTPATCH GetSysColorBrush( INT index )
{
    if (index < 0 || index >= ARRAY_SIZE( system_colors )) return 0;

    if (!system_colors[index].brush)
    {
        HBRUSH brush = CreateSolidBrush( GetSysColor( index ));
        __wine_make_gdi_object_system( brush, TRUE );
        if (InterlockedCompareExchangePointer( (void **)&system_colors[index].brush, brush, 0 ))
        {
            __wine_make_gdi_object_system( brush, FALSE );
            DeleteObject( brush );
        }
    }
    return system_colors[index].brush;
}


/***********************************************************************
 *		SYSCOLOR_GetPen
 */
HPEN SYSCOLOR_GetPen( INT index )
{
    /* We can assert here, because this function is internal to Wine */
    assert (0 <= index && index < ARRAY_SIZE( system_colors ));

    if (!system_colors[index].pen)
    {
        HPEN pen = CreatePen( PS_SOLID, 1, GetSysColor( index ));
        __wine_make_gdi_object_system( pen, TRUE );
        if (InterlockedCompareExchangePointer( (void **)&system_colors[index].pen, pen, 0 ))
        {
            __wine_make_gdi_object_system( pen, FALSE );
            DeleteObject( pen );
        }
    }
    return system_colors[index].pen;
}


/***********************************************************************
 *		SYSCOLOR_Get55AABrush
 */
HBRUSH SYSCOLOR_Get55AABrush(void)
{
    static const WORD pattern[] = { 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa };
    static HBRUSH brush_55aa;

    if (!brush_55aa)
    {
        HBITMAP bitmap = CreateBitmap( 8, 8, 1, 1, pattern );
        HBRUSH brush = CreatePatternBrush( bitmap );
        DeleteObject( bitmap );
        __wine_make_gdi_object_system( brush, TRUE );
        if (InterlockedCompareExchangePointer( (void **)&brush_55aa, brush, 0 ))
        {
            __wine_make_gdi_object_system( brush, FALSE );
            DeleteObject( brush );
        }
    }
    return brush_55aa;
}

/***********************************************************************
 *		ChangeDisplaySettingsA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsA( LPDEVMODEA devmode, DWORD flags )
{
    if (devmode) devmode->dmDriverExtra = 0;

    return ChangeDisplaySettingsExA(NULL,devmode,NULL,flags,NULL);
}


/***********************************************************************
 *		ChangeDisplaySettingsW (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsW( LPDEVMODEW devmode, DWORD flags )
{
    if (devmode) devmode->dmDriverExtra = 0;

    return ChangeDisplaySettingsExW(NULL,devmode,NULL,flags,NULL);
}


/***********************************************************************
 *		ChangeDisplaySettingsExA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsExA( LPCSTR devname, LPDEVMODEA devmode, HWND hwnd,
                                      DWORD flags, LPVOID lparam )
{
    LONG ret;
    UNICODE_STRING nameW;

    if (devname) RtlCreateUnicodeStringFromAsciiz(&nameW, devname);
    else nameW.Buffer = NULL;

    if (devmode)
    {
        DEVMODEW *devmodeW;

        devmodeW = GdiConvertToDevmodeW(devmode);
        if (devmodeW)
        {
            ret = ChangeDisplaySettingsExW(nameW.Buffer, devmodeW, hwnd, flags, lparam);
            HeapFree(GetProcessHeap(), 0, devmodeW);
        }
        else
            ret = DISP_CHANGE_SUCCESSFUL;
    }
    else
    {
        ret = ChangeDisplaySettingsExW(nameW.Buffer, NULL, hwnd, flags, lparam);
    }

    if (devname) RtlFreeUnicodeString(&nameW);
    return ret;
}

#define _X_FIELD(prefix, bits)                            \
    if ((fields) & prefix##_##bits)                       \
    {                                                     \
        p += sprintf(p, "%s%s", first ? "" : ",", #bits); \
        first = FALSE;                                    \
    }

static const CHAR *_CDS_flags(DWORD fields)
{
    BOOL first = TRUE;
    CHAR buf[128];
    CHAR *p = buf;

    _X_FIELD(CDS, UPDATEREGISTRY)
    _X_FIELD(CDS, TEST)
    _X_FIELD(CDS, FULLSCREEN)
    _X_FIELD(CDS, GLOBAL)
    _X_FIELD(CDS, SET_PRIMARY)
    _X_FIELD(CDS, VIDEOPARAMETERS)
    _X_FIELD(CDS, ENABLE_UNSAFE_MODES)
    _X_FIELD(CDS, DISABLE_UNSAFE_MODES)
    _X_FIELD(CDS, RESET)
    _X_FIELD(CDS, RESET_EX)
    _X_FIELD(CDS, NORESET)

    *p = 0;
    return wine_dbg_sprintf("%s", buf);
}

static const CHAR *_DM_fields(DWORD fields)
{
    BOOL first = TRUE;
    CHAR buf[128];
    CHAR *p = buf;

    _X_FIELD(DM, BITSPERPEL)
    _X_FIELD(DM, PELSWIDTH)
    _X_FIELD(DM, PELSHEIGHT)
    _X_FIELD(DM, DISPLAYFLAGS)
    _X_FIELD(DM, DISPLAYFREQUENCY)
    _X_FIELD(DM, POSITION)
    _X_FIELD(DM, DISPLAYORIENTATION)

    *p = 0;
    return wine_dbg_sprintf("%s", buf);
}

#undef _X_FIELD

static void trace_devmode(const DEVMODEW *devmode)
{
    TRACE("dmFields=%s ", _DM_fields(devmode->dmFields));
    if (devmode->dmFields & DM_BITSPERPEL)
        TRACE("dmBitsPerPel=%u ", devmode->dmBitsPerPel);
    if (devmode->dmFields & DM_PELSWIDTH)
        TRACE("dmPelsWidth=%u ", devmode->dmPelsWidth);
    if (devmode->dmFields & DM_PELSHEIGHT)
        TRACE("dmPelsHeight=%u ", devmode->dmPelsHeight);
    if (devmode->dmFields & DM_DISPLAYFREQUENCY)
        TRACE("dmDisplayFrequency=%u ", devmode->dmDisplayFrequency);
    if (devmode->dmFields & DM_POSITION)
        TRACE("dmPosition=(%d,%d) ", devmode->u1.s2.dmPosition.x, devmode->u1.s2.dmPosition.y);
    if (devmode->dmFields & DM_DISPLAYFLAGS)
        TRACE("dmDisplayFlags=%#x ", devmode->u2.dmDisplayFlags);
    if (devmode->dmFields & DM_DISPLAYORIENTATION)
        TRACE("dmDisplayOrientation=%u ", devmode->u1.s2.dmDisplayOrientation);
    TRACE("\n");
}

static BOOL is_detached_mode(const DEVMODEW *mode)
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

/***********************************************************************
 *		ChangeDisplaySettingsExW (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsExW( LPCWSTR devname, LPDEVMODEW devmode, HWND hwnd,
                                      DWORD flags, LPVOID lparam )
{
    WCHAR primary_adapter[CCHDEVICENAME];
    BOOL def_mode = TRUE;
    DEVMODEW dm;
    LONG ret;

    TRACE("%s %p %p %#x %p\n", debugstr_w(devname), devmode, hwnd, flags, lparam);
    TRACE("flags=%s\n", _CDS_flags(flags));

    if (!devname && !devmode)
    {
        ret = USER_Driver->pChangeDisplaySettingsEx(NULL, NULL, hwnd, flags, lparam);
        if (ret != DISP_CHANGE_SUCCESSFUL)
            ERR("Restoring all displays to their registry settings returned %d.\n", ret);
        return ret;
    }

    if (!devname && devmode)
    {
        if (!get_primary_adapter(primary_adapter))
            return DISP_CHANGE_FAILED;

        devname = primary_adapter;
    }

    if (!is_valid_adapter_name(devname))
    {
        ERR("Invalid device name %s.\n", wine_dbgstr_w(devname));
        return DISP_CHANGE_BADPARAM;
    }

    if (devmode)
    {
        trace_devmode(devmode);

        if (devmode->dmSize < FIELD_OFFSET(DEVMODEW, dmICMMethod))
            return DISP_CHANGE_BADMODE;

        if (is_detached_mode(devmode) ||
            ((devmode->dmFields & DM_BITSPERPEL) && devmode->dmBitsPerPel) ||
            ((devmode->dmFields & DM_PELSWIDTH) && devmode->dmPelsWidth) ||
            ((devmode->dmFields & DM_PELSHEIGHT) && devmode->dmPelsHeight) ||
            ((devmode->dmFields & DM_DISPLAYFREQUENCY) && devmode->dmDisplayFrequency))
                def_mode = FALSE;
    }

    if (def_mode)
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsExW(devname, ENUM_REGISTRY_SETTINGS, &dm, 0))
        {
            ERR("Default mode not found!\n");
            return DISP_CHANGE_BADMODE;
        }

        TRACE("Return to original display mode\n");
        devmode = &dm;
    }

    if ((devmode->dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        WARN("devmode doesn't specify the resolution: %#x\n", devmode->dmFields);
        return DISP_CHANGE_BADMODE;
    }

    if (!is_detached_mode(devmode) && (!devmode->dmPelsWidth || !devmode->dmPelsHeight))
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsExW(devname, ENUM_CURRENT_SETTINGS, &dm, 0))
        {
            ERR("Current mode not found!\n");
            return DISP_CHANGE_BADMODE;
        }

        if (!devmode->dmPelsWidth)
            devmode->dmPelsWidth = dm.dmPelsWidth;
        if (!devmode->dmPelsHeight)
            devmode->dmPelsHeight = dm.dmPelsHeight;
    }

    ret = USER_Driver->pChangeDisplaySettingsEx(devname, devmode, hwnd, flags, lparam);
    if (ret != DISP_CHANGE_SUCCESSFUL)
        ERR("Changing %s display settings returned %d.\n", wine_dbgstr_w(devname), ret);
    return ret;
}


/***********************************************************************
 *		EnumDisplaySettingsW (USER32.@)
 *
 * RETURNS
 *	TRUE if nth setting exists found (described in the LPDEVMODEW struct)
 *	FALSE if we do not have the nth setting
 */
BOOL WINAPI EnumDisplaySettingsW( LPCWSTR name, DWORD n, LPDEVMODEW devmode )
{
    return EnumDisplaySettingsExW(name, n, devmode, 0);
}


/***********************************************************************
 *		EnumDisplaySettingsA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsA(LPCSTR name,DWORD n,LPDEVMODEA devmode)
{
    return EnumDisplaySettingsExA(name, n, devmode, 0);
}


/***********************************************************************
 *		EnumDisplaySettingsExA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExA(LPCSTR lpszDeviceName, DWORD iModeNum,
                                   LPDEVMODEA lpDevMode, DWORD dwFlags)
{
    DEVMODEW devmodeW;
    BOOL ret;
    UNICODE_STRING nameW;

    if (lpszDeviceName) RtlCreateUnicodeStringFromAsciiz(&nameW, lpszDeviceName);
    else nameW.Buffer = NULL;

    memset(&devmodeW, 0, sizeof(devmodeW));
    devmodeW.dmSize = sizeof(devmodeW);
    ret = EnumDisplaySettingsExW(nameW.Buffer,iModeNum,&devmodeW,dwFlags);
    if (ret)
    {
        lpDevMode->dmSize = FIELD_OFFSET(DEVMODEA, dmICMMethod);
        lpDevMode->dmSpecVersion = devmodeW.dmSpecVersion;
        lpDevMode->dmDriverVersion = devmodeW.dmDriverVersion;
        WideCharToMultiByte(CP_ACP, 0, devmodeW.dmDeviceName, -1,
                            (LPSTR)lpDevMode->dmDeviceName, CCHDEVICENAME, NULL, NULL);
        lpDevMode->dmDriverExtra      = 0; /* FIXME */
        lpDevMode->dmBitsPerPel       = devmodeW.dmBitsPerPel;
        lpDevMode->dmPelsHeight       = devmodeW.dmPelsHeight;
        lpDevMode->dmPelsWidth        = devmodeW.dmPelsWidth;
        lpDevMode->u2.dmDisplayFlags  = devmodeW.u2.dmDisplayFlags;
        lpDevMode->dmDisplayFrequency = devmodeW.dmDisplayFrequency;
        lpDevMode->dmFields           = devmodeW.dmFields;

        lpDevMode->u1.s2.dmPosition.x = devmodeW.u1.s2.dmPosition.x;
        lpDevMode->u1.s2.dmPosition.y = devmodeW.u1.s2.dmPosition.y;
        lpDevMode->u1.s2.dmDisplayOrientation = devmodeW.u1.s2.dmDisplayOrientation;
        lpDevMode->u1.s2.dmDisplayFixedOutput = devmodeW.u1.s2.dmDisplayFixedOutput;
    }
    if (lpszDeviceName) RtlFreeUnicodeString(&nameW);
    return ret;
}


/***********************************************************************
 *		EnumDisplaySettingsExW (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum,
                                   LPDEVMODEW lpDevMode, DWORD dwFlags)
{
    WCHAR primary_adapter[CCHDEVICENAME];
    BOOL ret;

    TRACE("%s %#x %p %#x\n", wine_dbgstr_w(lpszDeviceName), iModeNum, lpDevMode, dwFlags);

    if (!lpszDeviceName)
    {
        if (!get_primary_adapter(primary_adapter))
            return FALSE;

        lpszDeviceName = primary_adapter;
    }

    if (!is_valid_adapter_name(lpszDeviceName))
    {
        ERR("Invalid device name %s.\n", wine_dbgstr_w(lpszDeviceName));
        return FALSE;
    }

    ret = USER_Driver->pEnumDisplaySettingsEx(lpszDeviceName, iModeNum, lpDevMode, dwFlags);
    if (ret)
        TRACE("device:%s mode index:%#x position:(%d,%d) resolution:%ux%u frequency:%uHz "
              "depth:%ubits orientation:%#x.\n", wine_dbgstr_w(lpszDeviceName), iModeNum,
              lpDevMode->u1.s2.dmPosition.x, lpDevMode->u1.s2.dmPosition.y, lpDevMode->dmPelsWidth,
              lpDevMode->dmPelsHeight, lpDevMode->dmDisplayFrequency, lpDevMode->dmBitsPerPel,
              lpDevMode->u1.s2.dmDisplayOrientation);
    else
        WARN("Failed to query %s display settings.\n", wine_dbgstr_w(lpszDeviceName));
    return ret;
}


/**********************************************************************
 *              get_monitor_dpi
 */
UINT get_monitor_dpi( HMONITOR monitor )
{
    /* FIXME: use the monitor DPI instead */
    return system_dpi;
}

/**********************************************************************
 *              get_win_monitor_dpi
 */
UINT get_win_monitor_dpi( HWND hwnd )
{
    /* FIXME: use the monitor DPI instead */
    return system_dpi;
}

/**********************************************************************
 *              get_thread_dpi
 */
UINT get_thread_dpi(void)
{
    switch (GetAwarenessFromDpiAwarenessContext( GetThreadDpiAwarenessContext() ))
    {
    case DPI_AWARENESS_UNAWARE:      return USER_DEFAULT_SCREEN_DPI;
    case DPI_AWARENESS_SYSTEM_AWARE: return system_dpi;
    default:                         return 0;  /* no scaling */
    }
}

/**********************************************************************
 *              map_dpi_point
 */
POINT map_dpi_point( POINT pt, UINT dpi_from, UINT dpi_to )
{
    if (dpi_from && dpi_to && dpi_from != dpi_to)
    {
        pt.x = MulDiv( pt.x, dpi_to, dpi_from );
        pt.y = MulDiv( pt.y, dpi_to, dpi_from );
    }
    return pt;
}

/**********************************************************************
 *              point_win_to_phys_dpi
 */
POINT point_win_to_phys_dpi( HWND hwnd, POINT pt )
{
    return map_dpi_point( pt, GetDpiForWindow( hwnd ), get_win_monitor_dpi( hwnd ) );
}

/**********************************************************************
 *              point_phys_to_win_dpi
 */
POINT point_phys_to_win_dpi( HWND hwnd, POINT pt )
{
    return map_dpi_point( pt, get_win_monitor_dpi( hwnd ), GetDpiForWindow( hwnd ));
}

/**********************************************************************
 *              point_win_to_thread_dpi
 */
POINT point_win_to_thread_dpi( HWND hwnd, POINT pt )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_point( pt, GetDpiForWindow( hwnd ), dpi );
}

/**********************************************************************
 *              point_thread_to_win_dpi
 */
POINT point_thread_to_win_dpi( HWND hwnd, POINT pt )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_point( pt, dpi, GetDpiForWindow( hwnd ));
}

/**********************************************************************
 *              map_dpi_rect
 */
RECT map_dpi_rect( RECT rect, UINT dpi_from, UINT dpi_to )
{
    if (dpi_from && dpi_to && dpi_from != dpi_to)
    {
        rect.left   = MulDiv( rect.left, dpi_to, dpi_from );
        rect.top    = MulDiv( rect.top, dpi_to, dpi_from );
        rect.right  = MulDiv( rect.right, dpi_to, dpi_from );
        rect.bottom = MulDiv( rect.bottom, dpi_to, dpi_from );
    }
    return rect;
}

/**********************************************************************
 *              rect_win_to_thread_dpi
 */
RECT rect_win_to_thread_dpi( HWND hwnd, RECT rect )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_rect( rect, GetDpiForWindow( hwnd ), dpi );
}

/**********************************************************************
 *              rect_thread_to_win_dpi
 */
RECT rect_thread_to_win_dpi( HWND hwnd, RECT rect )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_rect( rect, dpi, GetDpiForWindow( hwnd ) );
}

/**********************************************************************
 *              SetProcessDpiAwarenessContext   (USER32.@)
 */
BOOL WINAPI SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    DPI_AWARENESS val = GetAwarenessFromDpiAwarenessContext( context );

    if (val == DPI_AWARENESS_INVALID)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    val |= 0x10;  /* avoid 0 value */
    if (InterlockedCompareExchange( &dpi_awareness, val, 0 ))
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }
    TRACE( "set to %p\n", context );
    return TRUE;
}

/**********************************************************************
 *              GetProcessDpiAwarenessInternal   (USER32.@)
 */
BOOL WINAPI GetProcessDpiAwarenessInternal( HANDLE process, DPI_AWARENESS *awareness )
{
    if (process && process != GetCurrentProcess())
    {
        WARN( "not supported on other process %p\n", process );
        *awareness = DPI_AWARENESS_UNAWARE;
    }
    else *awareness = dpi_awareness & 3;
    return TRUE;
}

/**********************************************************************
 *              SetProcessDpiAwarenessInternal   (USER32.@)
 */
BOOL WINAPI SetProcessDpiAwarenessInternal( DPI_AWARENESS awareness )
{
    static const DPI_AWARENESS_CONTEXT contexts[3] = { DPI_AWARENESS_CONTEXT_UNAWARE,
                                                       DPI_AWARENESS_CONTEXT_SYSTEM_AWARE,
                                                       DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE };

    if (awareness < DPI_AWARENESS_UNAWARE || awareness > DPI_AWARENESS_PER_MONITOR_AWARE)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return SetProcessDpiAwarenessContext( contexts[awareness] );
}

/***********************************************************************
 *              AreDpiAwarenessContextsEqual   (USER32.@)
 */
BOOL WINAPI AreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT ctx1, DPI_AWARENESS_CONTEXT ctx2 )
{
    DPI_AWARENESS aware1 = GetAwarenessFromDpiAwarenessContext( ctx1 );
    DPI_AWARENESS aware2 = GetAwarenessFromDpiAwarenessContext( ctx2 );
    return aware1 != DPI_AWARENESS_INVALID && aware1 == aware2;
}

/***********************************************************************
 *              GetAwarenessFromDpiAwarenessContext   (USER32.@)
 */
DPI_AWARENESS WINAPI GetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    switch ((ULONG_PTR)context)
    {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x80000010:
    case 0x80000011:
    case 0x80000012:
        return (ULONG_PTR)context & 3;
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_UNAWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
        return ~(ULONG_PTR)context;
    default:
        return DPI_AWARENESS_INVALID;
    }
}

/***********************************************************************
 *              IsValidDpiAwarenessContext   (USER32.@)
 */
BOOL WINAPI IsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    return GetAwarenessFromDpiAwarenessContext( context ) != DPI_AWARENESS_INVALID;
}

/***********************************************************************
 *              SetProcessDPIAware   (USER32.@)
 */
BOOL WINAPI SetProcessDPIAware(void)
{
    TRACE("\n");
    InterlockedCompareExchange( &dpi_awareness, 0x11, 0 );
    return TRUE;
}

/***********************************************************************
 *              IsProcessDPIAware   (USER32.@)
 */
BOOL WINAPI IsProcessDPIAware(void)
{
    return GetAwarenessFromDpiAwarenessContext( GetThreadDpiAwarenessContext() ) != DPI_AWARENESS_UNAWARE;
}

/**********************************************************************
 *              EnableNonClientDpiScaling   (USER32.@)
 */
BOOL WINAPI EnableNonClientDpiScaling( HWND hwnd )
{
    FIXME("(%p): stub\n", hwnd);
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *              GetDpiForSystem   (USER32.@)
 */
UINT WINAPI GetDpiForSystem(void)
{
    if (!IsProcessDPIAware()) return USER_DEFAULT_SCREEN_DPI;
    return system_dpi;
}

/***********************************************************************
 *              GetDpiForMonitorInternal   (USER32.@)
 */
BOOL WINAPI GetDpiForMonitorInternal( HMONITOR monitor, UINT type, UINT *x, UINT *y )
{
    if (type > 2)
    {
        SetLastError( ERROR_BAD_ARGUMENTS );
        return FALSE;
    }
    if (!x || !y)
    {
        SetLastError( ERROR_INVALID_ADDRESS );
        return FALSE;
    }
    switch (GetAwarenessFromDpiAwarenessContext( GetThreadDpiAwarenessContext() ))
    {
    case DPI_AWARENESS_UNAWARE:      *x = *y = USER_DEFAULT_SCREEN_DPI; break;
    case DPI_AWARENESS_SYSTEM_AWARE: *x = *y = system_dpi; break;
    default:                         *x = *y = get_monitor_dpi( monitor ); break;
    }
    return TRUE;
}

/**********************************************************************
 *              GetThreadDpiAwarenessContext   (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext(void)
{
    struct user_thread_info *info = get_user_thread_info();

    if (info->dpi_awareness) return ULongToHandle( info->dpi_awareness );
    if (dpi_awareness) return ULongToHandle( dpi_awareness );
    return ULongToHandle( 0x10 | default_awareness );
}

/**********************************************************************
 *              SetThreadDpiAwarenessContext   (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    struct user_thread_info *info = get_user_thread_info();
    DPI_AWARENESS prev, val = GetAwarenessFromDpiAwarenessContext( context );

    if (val == DPI_AWARENESS_INVALID)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(prev = info->dpi_awareness))
    {
        prev = dpi_awareness;
        if (!prev) prev = 0x10 | DPI_AWARENESS_UNAWARE;
        prev |= 0x80000000;  /* restore to process default */
    }
    if (((ULONG_PTR)context & ~(ULONG_PTR)0x13) == 0x80000000) info->dpi_awareness = 0;
    else info->dpi_awareness = val | 0x10;
    return ULongToHandle( prev );
}

/**********************************************************************
 *              LogicalToPhysicalPointForPerMonitorDPI   (USER32.@)
 */
BOOL WINAPI LogicalToPhysicalPointForPerMonitorDPI( HWND hwnd, POINT *pt )
{
    RECT rect;

    if (!GetWindowRect( hwnd, &rect )) return FALSE;
    if (pt->x < rect.left || pt->y < rect.top || pt->x > rect.right || pt->y > rect.bottom) return FALSE;
    *pt = point_win_to_phys_dpi( hwnd, *pt );
    return TRUE;
}

/**********************************************************************
 *              PhysicalToLogicalPointForPerMonitorDPI   (USER32.@)
 */
BOOL WINAPI PhysicalToLogicalPointForPerMonitorDPI( HWND hwnd, POINT *pt )
{
    DPI_AWARENESS_CONTEXT context;
    RECT rect;
    BOOL ret = FALSE;

    context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    if (GetWindowRect( hwnd, &rect ) &&
        pt->x >= rect.left && pt->y >= rect.top && pt->x <= rect.right && pt->y <= rect.bottom)
    {
        *pt = point_phys_to_win_dpi( hwnd, *pt );
        ret = TRUE;
    }
    SetThreadDpiAwarenessContext( context );
    return ret;
}

struct monitor_enum_info
{
    RECT     rect;
    UINT     max_area;
    UINT     min_distance;
    HMONITOR primary;
    HMONITOR nearest;
    HMONITOR ret;
};

/* helper callback for MonitorFromRect */
static BOOL CALLBACK monitor_enum( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    struct monitor_enum_info *info = (struct monitor_enum_info *)lp;
    RECT intersect;

    if (IntersectRect( &intersect, rect, &info->rect ))
    {
        /* check for larger intersecting area */
        UINT area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);
        if (area > info->max_area)
        {
            info->max_area = area;
            info->ret = monitor;
        }
    }
    else if (!info->max_area)  /* if not intersecting, check for min distance */
    {
        UINT distance;
        UINT x, y;

        if (info->rect.right <= rect->left) x = rect->left - info->rect.right;
        else if (rect->right <= info->rect.left) x = info->rect.left - rect->right;
        else x = 0;
        if (info->rect.bottom <= rect->top) y = rect->top - info->rect.bottom;
        else if (rect->bottom <= info->rect.top) y = info->rect.top - rect->bottom;
        else y = 0;
        distance = x * x + y * y;
        if (distance < info->min_distance)
        {
            info->min_distance = distance;
            info->nearest = monitor;
        }
    }
    if (!info->primary)
    {
        MONITORINFO mon_info;
        mon_info.cbSize = sizeof(mon_info);
        GetMonitorInfoW( monitor, &mon_info );
        if (mon_info.dwFlags & MONITORINFOF_PRIMARY) info->primary = monitor;
    }
    return TRUE;
}

/***********************************************************************
 *		MonitorFromRect (USER32.@)
 */
HMONITOR WINAPI MonitorFromRect( const RECT *rect, DWORD flags )
{
    struct monitor_enum_info info;

    info.rect         = *rect;
    info.max_area     = 0;
    info.min_distance = ~0u;
    info.primary      = 0;
    info.nearest      = 0;
    info.ret          = 0;

    if (IsRectEmpty(&info.rect))
    {
        info.rect.right = info.rect.left + 1;
        info.rect.bottom = info.rect.top + 1;
    }

    if (!EnumDisplayMonitors( 0, NULL, monitor_enum, (LPARAM)&info )) return 0;
    if (!info.ret)
    {
        if (flags & MONITOR_DEFAULTTOPRIMARY) info.ret = info.primary;
        else if (flags & MONITOR_DEFAULTTONEAREST) info.ret = info.nearest;
    }

    TRACE( "%s flags %x returning %p\n", wine_dbgstr_rect(rect), flags, info.ret );
    return info.ret;
}

/***********************************************************************
 *		MonitorFromPoint (USER32.@)
 */
HMONITOR WINAPI MonitorFromPoint( POINT pt, DWORD flags )
{
    RECT rect;

    SetRect( &rect, pt.x, pt.y, pt.x + 1, pt.y + 1 );
    return MonitorFromRect( &rect, flags );
}

/***********************************************************************
 *		MonitorFromWindow (USER32.@)
 */
HMONITOR WINAPI MonitorFromWindow(HWND hWnd, DWORD dwFlags)
{
    RECT rect;
    WINDOWPLACEMENT wp;

    TRACE("(%p, 0x%08x)\n", hWnd, dwFlags);

    wp.length = sizeof(wp);
    if (IsIconic(hWnd) && GetWindowPlacement(hWnd, &wp))
        return MonitorFromRect( &wp.rcNormalPosition, dwFlags );

    if (GetWindowRect( hWnd, &rect ))
        return MonitorFromRect( &rect, dwFlags );

    if (!(dwFlags & (MONITOR_DEFAULTTOPRIMARY|MONITOR_DEFAULTTONEAREST))) return 0;
    /* retrieve the primary */
    SetRect( &rect, 0, 0, 1, 1 );
    return MonitorFromRect( &rect, dwFlags );
}

/* Return FALSE on failure and TRUE on success */
static BOOL update_monitor_cache(void)
{
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO devinfo = INVALID_HANDLE_VALUE;
    MONITORINFOEXW *monitor_array;
    FILETIME filetime = {0};
    DWORD device_count = 0;
    HANDLE mutex = NULL;
    DWORD state_flags;
    BOOL ret = FALSE;
    BOOL is_replica;
    DWORD i = 0, j;
    DWORD type;

    /* Update monitor cache from SetupAPI if it's outdated */
    if (!video_key && RegOpenKeyW( HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\VIDEO", &video_key ))
        return FALSE;
    if (RegQueryInfoKeyW( video_key, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &filetime ))
        return FALSE;
    if (CompareFileTime( &filetime, &last_query_monitors_time ) < 1)
        return TRUE;

    mutex = get_display_device_init_mutex();
    EnterCriticalSection( &monitors_section );
    devinfo = SetupDiGetClassDevsW( &GUID_DEVCLASS_MONITOR, L"DISPLAY", NULL, DIGCF_PRESENT );

    while (SetupDiEnumDeviceInfo( devinfo, i++, &device_data ))
    {
        /* Inactive monitors don't get enumerated */
        if (!SetupDiGetDevicePropertyW( devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_STATEFLAGS, &type,
                                        (BYTE *)&state_flags, sizeof(DWORD), NULL, 0 ))
            goto fail;
        if (state_flags & DISPLAY_DEVICE_ACTIVE)
            device_count++;
    }

    if (device_count && monitor_count < device_count)
    {
        monitor_array = heap_alloc( device_count * sizeof(*monitor_array) );
        if (!monitor_array)
            goto fail;
        heap_free( monitors );
        monitors = monitor_array;
    }

    for (i = 0, monitor_count = 0; SetupDiEnumDeviceInfo( devinfo, i, &device_data ); i++)
    {
        if (!SetupDiGetDevicePropertyW( devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_STATEFLAGS, &type,
                                        (BYTE *)&state_flags, sizeof(DWORD), NULL, 0 ))
            goto fail;
        if (!(state_flags & DISPLAY_DEVICE_ACTIVE))
            continue;
        if (!SetupDiGetDevicePropertyW( devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_RCMONITOR, &type,
                                        (BYTE *)&monitors[monitor_count].rcMonitor, sizeof(RECT), NULL, 0 ))
            goto fail;

        /* Replicas in mirroring monitor sets don't get enumerated */
        is_replica = FALSE;
        for (j = 0; j < monitor_count; j++)
        {
            if (EqualRect(&monitors[j].rcMonitor, &monitors[monitor_count].rcMonitor))
            {
                is_replica = TRUE;
                break;
            }
        }
        if (is_replica)
            continue;

        if (!SetupDiGetDevicePropertyW( devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_RCWORK, &type,
                                        (BYTE *)&monitors[monitor_count].rcWork, sizeof(RECT), NULL, 0 ))
            goto fail;
        if (!SetupDiGetDevicePropertyW( devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_ADAPTERNAME, &type,
                                        (BYTE *)monitors[monitor_count].szDevice, CCHDEVICENAME * sizeof(WCHAR), NULL, 0))
            goto fail;
        monitors[monitor_count].dwFlags =
            !wcscmp( L"\\\\.\\DISPLAY1", monitors[monitor_count].szDevice ) ? MONITORINFOF_PRIMARY : 0;

        monitor_count++;
    }

    last_query_monitors_time = filetime;
    ret = TRUE;
fail:
    SetupDiDestroyDeviceInfoList( devinfo );
    LeaveCriticalSection( &monitors_section );
    release_display_device_init_mutex( mutex );
    return ret;
}

BOOL CDECL nulldrv_GetMonitorInfo( HMONITOR handle, MONITORINFO *info )
{
    UINT index = (UINT_PTR)handle - 1;

    TRACE("(%p, %p)\n", handle, info);

    /* Fallback to report one monitor */
    if (handle == NULLDRV_DEFAULT_HMONITOR)
    {
        RECT default_rect = {0, 0, 640, 480};
        info->rcMonitor = default_rect;
        info->rcWork = default_rect;
        info->dwFlags = MONITORINFOF_PRIMARY;
        if (info->cbSize >= sizeof(MONITORINFOEXW))
            lstrcpyW( ((MONITORINFOEXW *)info)->szDevice, L"\\\\.\\DISPLAY1" );
        return TRUE;
    }

    if (!update_monitor_cache())
        return FALSE;

    EnterCriticalSection( &monitors_section );
    if (index < monitor_count)
    {
        info->rcMonitor = monitors[index].rcMonitor;
        info->rcWork = monitors[index].rcWork;
        info->dwFlags = monitors[index].dwFlags;
        if (info->cbSize >= sizeof(MONITORINFOEXW))
            lstrcpyW( ((MONITORINFOEXW *)info)->szDevice, monitors[index].szDevice );
        LeaveCriticalSection( &monitors_section );
        return TRUE;
    }
    else
    {
        LeaveCriticalSection( &monitors_section );
        SetLastError( ERROR_INVALID_MONITOR_HANDLE );
        return FALSE;
    }
}

/***********************************************************************
 *		GetMonitorInfoA (USER32.@)
 */
BOOL WINAPI GetMonitorInfoA( HMONITOR monitor, LPMONITORINFO info )
{
    MONITORINFOEXW miW;
    BOOL ret;

    if (info->cbSize == sizeof(MONITORINFO)) return GetMonitorInfoW( monitor, info );
    if (info->cbSize != sizeof(MONITORINFOEXA)) return FALSE;

    miW.cbSize = sizeof(miW);
    ret = GetMonitorInfoW( monitor, (MONITORINFO *)&miW );
    if (ret)
    {
        MONITORINFOEXA *miA = (MONITORINFOEXA *)info;
        miA->rcMonitor = miW.rcMonitor;
        miA->rcWork = miW.rcWork;
        miA->dwFlags = miW.dwFlags;
        WideCharToMultiByte(CP_ACP, 0, miW.szDevice, -1, miA->szDevice, sizeof(miA->szDevice), NULL, NULL);
    }
    return ret;
}

/***********************************************************************
 *		GetMonitorInfoW (USER32.@)
 */
BOOL WINAPI GetMonitorInfoW( HMONITOR monitor, LPMONITORINFO info )
{
    BOOL ret;
    UINT dpi_from, dpi_to;

    if (info->cbSize != sizeof(MONITORINFOEXW) && info->cbSize != sizeof(MONITORINFO)) return FALSE;

    ret = USER_Driver->pGetMonitorInfo( monitor, info );
    if (ret)
    {
        if ((dpi_to = get_thread_dpi()))
        {
            dpi_from = get_monitor_dpi( monitor );
            info->rcMonitor = map_dpi_rect( info->rcMonitor, dpi_from, dpi_to );
            info->rcWork = map_dpi_rect( info->rcWork, dpi_from, dpi_to );
        }
        TRACE( "flags %04x, monitor %s, work %s\n", info->dwFlags,
               wine_dbgstr_rect(&info->rcMonitor), wine_dbgstr_rect(&info->rcWork));
    }
    return ret;
}

struct enum_mon_data
{
    MONITORENUMPROC proc;
    LPARAM lparam;
    HDC hdc;
    POINT origin;
    RECT limit;
};

#ifdef __i386__
/* Some apps pass a non-stdcall callback to EnumDisplayMonitors,
 * so we need a small assembly wrapper to call it.
 * MJ's Help Diagnostic expects that %ecx contains the address to the rect.
 */
extern BOOL enum_mon_callback_wrapper( HMONITOR monitor, LPRECT rect, struct enum_mon_data *data );
__ASM_GLOBAL_FUNC( enum_mon_callback_wrapper,
    "pushl %ebp\n\t"
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
    "movl %esp,%ebp\n\t"
    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
    "subl $8,%esp\n\t"
    "movl 16(%ebp),%eax\n\t"    /* data */
    "movl 12(%ebp),%ecx\n\t"    /* rect */
    "pushl 4(%eax)\n\t"         /* data->lparam */
    "pushl %ecx\n\t"            /* rect */
    "pushl 8(%eax)\n\t"         /* data->hdc */
    "pushl 8(%ebp)\n\t"         /* monitor */
    "call *(%eax)\n\t"          /* data->proc */
    "leave\n\t"
    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
    __ASM_CFI(".cfi_same_value %ebp\n\t")
    "ret" )
#endif /* __i386__ */

static BOOL CALLBACK enum_mon_callback( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    struct enum_mon_data *data = (struct enum_mon_data *)lp;
    RECT monrect = map_dpi_rect( *rect, get_monitor_dpi( monitor ), get_thread_dpi() );

    OffsetRect( &monrect, -data->origin.x, -data->origin.y );
    if (!IntersectRect( &monrect, &monrect, &data->limit )) return TRUE;
#ifdef __i386__
    return enum_mon_callback_wrapper( monitor, &monrect, data );
#else
    return data->proc( monitor, data->hdc, &monrect, data->lparam );
#endif
}

BOOL CDECL nulldrv_EnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lp )
{
    RECT monitor_rect;
    DWORD i = 0;

    TRACE("(%p, %p, %p, 0x%lx)\n", hdc, rect, proc, lp);

    if (update_monitor_cache())
    {
        while (TRUE)
        {
            EnterCriticalSection( &monitors_section );
            if (i >= monitor_count)
            {
                LeaveCriticalSection( &monitors_section );
                return TRUE;
            }
            monitor_rect = monitors[i].rcMonitor;
            LeaveCriticalSection( &monitors_section );

            if (!proc( (HMONITOR)(UINT_PTR)(i + 1), hdc, &monitor_rect, lp ))
                return FALSE;

            ++i;
        }
    }

    /* Fallback to report one monitor if using SetupAPI failed */
    SetRect( &monitor_rect, 0, 0, 640, 480 );
    if (!proc( NULLDRV_DEFAULT_HMONITOR, hdc, &monitor_rect, lp ))
        return FALSE;
    return TRUE;
}

/***********************************************************************
 *		EnumDisplayMonitors (USER32.@)
 */
BOOL WINAPI EnumDisplayMonitors( HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
    struct enum_mon_data data;

    data.proc = proc;
    data.lparam = lp;
    data.hdc = hdc;

    if (hdc)
    {
        if (!GetDCOrgEx( hdc, &data.origin )) return FALSE;
        if (GetClipBox( hdc, &data.limit ) == ERROR) return FALSE;
    }
    else
    {
        data.origin.x = data.origin.y = 0;
        data.limit.left = data.limit.top = INT_MIN;
        data.limit.right = data.limit.bottom = INT_MAX;
    }
    if (rect && !IntersectRect( &data.limit, &data.limit, rect )) return TRUE;
    return USER_Driver->pEnumDisplayMonitors( 0, NULL, enum_mon_callback, (LPARAM)&data );
}

/***********************************************************************
 *		EnumDisplayDevicesA (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesA( LPCSTR device, DWORD index, DISPLAY_DEVICEA *info, DWORD flags )
{
    UNICODE_STRING deviceW;
    DISPLAY_DEVICEW ddW;
    BOOL ret;

    if (device)
        RtlCreateUnicodeStringFromAsciiz( &deviceW, device );
    else
        deviceW.Buffer = NULL;

    ddW.cb = sizeof(ddW);
    ret = EnumDisplayDevicesW( deviceW.Buffer, index, &ddW, flags );
    RtlFreeUnicodeString( &deviceW );

    if (!ret)
        return ret;

    WideCharToMultiByte( CP_ACP, 0, ddW.DeviceName, -1, info->DeviceName, sizeof(info->DeviceName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, ddW.DeviceString, -1, info->DeviceString, sizeof(info->DeviceString), NULL, NULL );
    info->StateFlags = ddW.StateFlags;

    if (info->cb >= offsetof(DISPLAY_DEVICEA, DeviceID) + sizeof(info->DeviceID))
        WideCharToMultiByte( CP_ACP, 0, ddW.DeviceID, -1, info->DeviceID, sizeof(info->DeviceID), NULL, NULL );
    if (info->cb >= offsetof(DISPLAY_DEVICEA, DeviceKey) + sizeof(info->DeviceKey))
        WideCharToMultiByte( CP_ACP, 0, ddW.DeviceKey, -1, info->DeviceKey, sizeof(info->DeviceKey), NULL, NULL );

    return TRUE;
}

/***********************************************************************
 *		EnumDisplayDevicesW (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesW( LPCWSTR device, DWORD index, DISPLAY_DEVICEW *info, DWORD flags )
{
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO set = INVALID_HANDLE_VALUE;
    WCHAR key_nameW[MAX_PATH];
    WCHAR instanceW[MAX_PATH];
    WCHAR bufferW[1024];
    LONG adapter_index;
    WCHAR *next_charW;
    HANDLE mutex;
    DWORD size;
    DWORD type;
    HKEY hkey;
    BOOL ret = FALSE;

    TRACE("%s %d %p %#x\n", debugstr_w( device ), index, info, flags);

    wait_graphics_driver_ready();
    mutex = get_display_device_init_mutex();

    /* Find adapter */
    if (!device)
    {
        swprintf( key_nameW, ARRAY_SIZE(key_nameW), L"\\Device\\Video%d", index );
        size = sizeof(bufferW);
        if (RegGetValueW( HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\VIDEO", key_nameW, RRF_RT_REG_SZ, NULL, bufferW, &size ))
            goto done;

        /* DeviceKey */
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceKey) + sizeof(info->DeviceKey))
            lstrcpyW( info->DeviceKey, bufferW );

        /* DeviceName */
        swprintf( info->DeviceName, ARRAY_SIZE(info->DeviceName), L"\\\\.\\DISPLAY%d", index + 1 );

        /* Strip \Registry\Machine\ */
        lstrcpyW( key_nameW, bufferW + 18 );

        /* DeviceString */
        size = sizeof(info->DeviceString);
        if (RegGetValueW( HKEY_LOCAL_MACHINE, key_nameW, L"DriverDesc", RRF_RT_REG_SZ, NULL,
                          info->DeviceString, &size ))
            goto done;

        /* StateFlags */
        size = sizeof(info->StateFlags);
        if (RegGetValueW( HKEY_CURRENT_CONFIG, key_nameW, L"StateFlags", RRF_RT_REG_DWORD, NULL,
                          &info->StateFlags, &size ))
            goto done;

        /* DeviceID */
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(info->DeviceID))
        {
            if (flags & EDD_GET_DEVICE_INTERFACE_NAME)
                info->DeviceID[0] = 0;
            else
            {
                size = sizeof(bufferW);
                if (RegGetValueW( HKEY_CURRENT_CONFIG, key_nameW, L"GPUID", RRF_RT_REG_SZ | RRF_ZEROONFAILURE, NULL,
                                  bufferW, &size ))
                    goto done;
                set = SetupDiCreateDeviceInfoList( &GUID_DEVCLASS_DISPLAY, NULL );
                if (!SetupDiOpenDeviceInfoW( set, bufferW, NULL, 0, &device_data )
                    || !SetupDiGetDeviceRegistryPropertyW( set, &device_data, SPDRP_HARDWAREID, NULL, (BYTE *)bufferW,
                                                           sizeof(bufferW), NULL ))
                    goto done;
                lstrcpyW( info->DeviceID, bufferW );
            }
        }
    }
    /* Find monitor */
    else
    {
        /* Check adapter name */
        if (wcsnicmp( device, L"\\\\.\\DISPLAY", lstrlenW(L"\\\\.\\DISPLAY") ))
            goto done;

        adapter_index = wcstol( device + lstrlenW(L"\\\\.\\DISPLAY"), NULL, 10 );
        swprintf( key_nameW, ARRAY_SIZE(key_nameW), L"\\Device\\Video%d", adapter_index - 1 );

        size = sizeof(bufferW);
        if (RegGetValueW( HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\VIDEO", key_nameW, RRF_RT_REG_SZ, NULL, bufferW, &size ))
            goto done;

        /* DeviceName */
        swprintf( info->DeviceName, ARRAY_SIZE(info->DeviceName), L"\\\\.\\DISPLAY%d\\Monitor%d", adapter_index, index );

        /* Get monitor instance */
        /* Strip \Registry\Machine\ first */
        lstrcpyW( key_nameW, bufferW + 18 );
        swprintf( bufferW, ARRAY_SIZE(bufferW), L"MonitorID%d", index );

        size = sizeof(instanceW);
        if (RegGetValueW( HKEY_CURRENT_CONFIG, key_nameW, bufferW, RRF_RT_REG_SZ, NULL, instanceW, &size ))
            goto done;

        set = SetupDiCreateDeviceInfoList( &GUID_DEVCLASS_MONITOR, NULL );
        if (!SetupDiOpenDeviceInfoW( set, instanceW, NULL, 0, &device_data ))
            goto done;

        /* StateFlags */
        if (!SetupDiGetDevicePropertyW( set, &device_data, &WINE_DEVPROPKEY_MONITOR_STATEFLAGS, &type,
                                        (BYTE *)&info->StateFlags, sizeof(info->StateFlags), NULL, 0 ))
            goto done;

        /* DeviceString */
        if (!SetupDiGetDeviceRegistryPropertyW( set, &device_data, SPDRP_DEVICEDESC, NULL,
                                                (BYTE *)info->DeviceString,
                                                sizeof(info->DeviceString), NULL ))
            goto done;

        /* DeviceKey */
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceKey) + sizeof(info->DeviceKey))
        {
            if (!SetupDiGetDeviceRegistryPropertyW( set, &device_data, SPDRP_DRIVER, NULL, (BYTE *)bufferW,
                                                    sizeof(bufferW), NULL ))
                goto done;

            lstrcpyW( info->DeviceKey, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\" );
            lstrcatW( info->DeviceKey, bufferW );
        }

        /* DeviceID */
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(info->DeviceID))
        {
            if (flags & EDD_GET_DEVICE_INTERFACE_NAME)
            {
                lstrcpyW( info->DeviceID, L"\\\\\?\\" );
                lstrcatW( info->DeviceID, instanceW );
                lstrcatW( info->DeviceID, L"#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}" );
                /* Replace '\\' with '#' after prefix */
                for (next_charW = info->DeviceID + lstrlenW( L"\\\\\?\\" ); *next_charW;
                     next_charW++)
                {
                    if (*next_charW == '\\')
                        *next_charW = '#';
                }
            }
            else
            {
                if (!SetupDiGetDeviceRegistryPropertyW( set, &device_data, SPDRP_HARDWAREID, NULL, (BYTE *)bufferW,
                                                        sizeof(bufferW), NULL ))
                    goto done;

                lstrcpyW( info->DeviceID, bufferW );
                lstrcatW( info->DeviceID, L"\\" );

                if (!SetupDiGetDeviceRegistryPropertyW( set, &device_data, SPDRP_DRIVER, NULL, (BYTE *)bufferW,
                                                        sizeof(bufferW), NULL ))
                    goto done;

                lstrcatW( info->DeviceID, bufferW );
            }
        }
    }

    ret = TRUE;
done:
    release_display_device_init_mutex( mutex );
    SetupDiDestroyDeviceInfoList( set );
    if (ret)
        return ret;

    /* Fallback to report at least one adapter and monitor, if user driver didn't initialize display device registry */
    if (index)
        return FALSE;

    /* If user driver did initialize the registry, then exit */
    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\VIDEO", &hkey ))
    {
        RegCloseKey( hkey );
        return FALSE;
    }
    WARN("Reporting fallback display devices\n");

    /* Adapter */
    if (!device)
    {
        lstrcpyW( info->DeviceName, L"\\\\.\\DISPLAY1" );
        lstrcpyW( info->DeviceString, L"Wine Adapter" );
        info->StateFlags =
            DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_VGA_COMPATIBLE;
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(info->DeviceID))
        {
            if (flags & EDD_GET_DEVICE_INTERFACE_NAME)
                info->DeviceID[0] = 0;
            else
                lstrcpyW( info->DeviceID, L"PCI\\VEN_0000&DEV_0000&SUBSYS_00000000&REV_00" );
        }
    }
    /* Monitor */
    else
    {
        if (lstrcmpiW( L"\\\\.\\DISPLAY1", device ))
            return FALSE;

        lstrcpyW( info->DeviceName, L"\\\\.\\DISPLAY1\\Monitor0" );
        lstrcpyW( info->DeviceString, L"Generic Non-PnP Monitor" );
        info->StateFlags = DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED;
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(info->DeviceID))
        {
            if (flags & EDD_GET_DEVICE_INTERFACE_NAME)
                lstrcpyW( info->DeviceID, L"\\\\\?\\DISPLAY#Default_Monitor#4&17f0ff54&0&UID0#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}" );
            else
                lstrcpyW( info->DeviceID, L"MONITOR\\Default_Monitor\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\0000" );
        }
    }

    if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceKey) + sizeof(info->DeviceKey))
        info->DeviceKey[0] = 0;

    return TRUE;
}

/**********************************************************************
 *              GetAutoRotationState [USER32.@]
 */
BOOL WINAPI GetAutoRotationState( AR_STATE *state )
{
    TRACE("(%p)\n", state);

    if (!state)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *state = AR_NOSENSOR;
    return TRUE;
}

/**********************************************************************
 *              GetDisplayAutoRotationPreferences [USER32.@]
 */
BOOL WINAPI GetDisplayAutoRotationPreferences( ORIENTATION_PREFERENCE *orientation )
{
    FIXME("(%p): stub\n", orientation);
    *orientation = ORIENTATION_PREFERENCE_NONE;
    return TRUE;
}

/* physical<->logical mapping functions from win8 that are nops in later versions */

/***********************************************************************
 *              GetPhysicalCursorPos   (USER32.@)
 */
BOOL WINAPI GetPhysicalCursorPos( POINT *point )
{
    return GetCursorPos( point );
}

/***********************************************************************
 *              SetPhysicalCursorPos   (USER32.@)
 */
BOOL WINAPI SetPhysicalCursorPos( INT x, INT y )
{
    return SetCursorPos( x, y );
}

/***********************************************************************
 *		LogicalToPhysicalPoint (USER32.@)
 */
BOOL WINAPI LogicalToPhysicalPoint( HWND hwnd, POINT *point )
{
    return TRUE;
}

/***********************************************************************
 *		PhysicalToLogicalPoint (USER32.@)
 */
BOOL WINAPI PhysicalToLogicalPoint( HWND hwnd, POINT *point )
{
    return TRUE;
}

/**********************************************************************
 *              GetDisplayConfigBufferSizes (USER32.@)
 */
LONG WINAPI GetDisplayConfigBufferSizes(UINT32 flags, UINT32 *num_path_info, UINT32 *num_mode_info)
{
    LONG ret = ERROR_GEN_FAILURE;
    HANDLE mutex;
    HDEVINFO devinfo;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    DWORD monitor_index = 0, state_flags, type;

    FIXME("(0x%x %p %p): semi-stub\n", flags, num_path_info, num_mode_info);

    if (!num_path_info || !num_mode_info)
        return ERROR_INVALID_PARAMETER;

    *num_path_info = 0;

    if (flags != QDC_ALL_PATHS &&
        flags != QDC_ONLY_ACTIVE_PATHS &&
        flags != QDC_DATABASE_CURRENT)
        return ERROR_INVALID_PARAMETER;

    if (flags != QDC_ONLY_ACTIVE_PATHS)
        FIXME("only returning active paths\n");

    wait_graphics_driver_ready();
    mutex = get_display_device_init_mutex();

    /* Iterate through "targets"/monitors.
     * Each target corresponds to a path, and each path references a source and a target mode.
     */
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, L"DISPLAY", NULL, DIGCF_PRESENT);
    if (devinfo == INVALID_HANDLE_VALUE)
        goto done;

    while (SetupDiEnumDeviceInfo(devinfo, monitor_index++, &device_data))
    {
        /* Only count active monitors */
        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_STATEFLAGS,
                                       &type, (BYTE *)&state_flags, sizeof(state_flags), NULL, 0))
            goto done;

        if (state_flags & DISPLAY_DEVICE_ACTIVE)
            (*num_path_info)++;
    }

    *num_mode_info = *num_path_info * 2;
    ret = ERROR_SUCCESS;
    TRACE("returning %u path(s) %u mode(s)\n", *num_path_info, *num_mode_info);

done:
    SetupDiDestroyDeviceInfoList(devinfo);
    release_display_device_init_mutex(mutex);
    return ret;
}

static DISPLAYCONFIG_ROTATION get_dc_rotation(const DEVMODEW *devmode)
{
    if (devmode->dmFields & DM_DISPLAYORIENTATION)
        return devmode->u1.s2.dmDisplayOrientation + 1;
    else
        return DISPLAYCONFIG_ROTATION_IDENTITY;
}

static DISPLAYCONFIG_SCANLINE_ORDERING get_dc_scanline_ordering(const DEVMODEW *devmode)
{
    if (!(devmode->dmFields & DM_DISPLAYFLAGS))
        return DISPLAYCONFIG_SCANLINE_ORDERING_UNSPECIFIED;
    else if (devmode->u2.dmDisplayFlags & DM_INTERLACED)
        return DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED;
    else
        return DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
}

static DISPLAYCONFIG_PIXELFORMAT get_dc_pixelformat(DWORD dmBitsPerPel)
{
    if ((dmBitsPerPel == 8) || (dmBitsPerPel == 16) ||
        (dmBitsPerPel == 24) || (dmBitsPerPel == 32))
        return dmBitsPerPel / 8;
    else
        return DISPLAYCONFIG_PIXELFORMAT_NONGDI;
}

static void set_mode_target_info(DISPLAYCONFIG_MODE_INFO *info, const LUID *gpu_luid, UINT32 target_id,
                                 UINT32 flags, const DEVMODEW *devmode)
{
    DISPLAYCONFIG_TARGET_MODE *mode = &(info->u.targetMode);

    info->infoType = DISPLAYCONFIG_MODE_INFO_TYPE_TARGET;
    info->adapterId = *gpu_luid;
    info->id = target_id;

    /* FIXME: Populate pixelRate/hSyncFreq/totalSize with real data */
    mode->targetVideoSignalInfo.pixelRate = devmode->dmDisplayFrequency * devmode->dmPelsWidth * devmode->dmPelsHeight;
    mode->targetVideoSignalInfo.hSyncFreq.Numerator = devmode->dmDisplayFrequency * devmode->dmPelsWidth;
    mode->targetVideoSignalInfo.hSyncFreq.Denominator = 1;
    mode->targetVideoSignalInfo.vSyncFreq.Numerator = devmode->dmDisplayFrequency;
    mode->targetVideoSignalInfo.vSyncFreq.Denominator = 1;
    mode->targetVideoSignalInfo.activeSize.cx = devmode->dmPelsWidth;
    mode->targetVideoSignalInfo.activeSize.cy = devmode->dmPelsHeight;
    if (flags & QDC_DATABASE_CURRENT)
    {
        mode->targetVideoSignalInfo.totalSize.cx = 0;
        mode->targetVideoSignalInfo.totalSize.cy = 0;
    }
    else
    {
        mode->targetVideoSignalInfo.totalSize.cx = devmode->dmPelsWidth;
        mode->targetVideoSignalInfo.totalSize.cy = devmode->dmPelsHeight;
    }
    mode->targetVideoSignalInfo.u.videoStandard = D3DKMDT_VSS_OTHER;
    mode->targetVideoSignalInfo.scanLineOrdering = get_dc_scanline_ordering(devmode);
}

static void set_path_target_info(DISPLAYCONFIG_PATH_TARGET_INFO *info, const LUID *gpu_luid,
                                 UINT32 target_id, UINT32 mode_index, const DEVMODEW *devmode)
{
    info->adapterId = *gpu_luid;
    info->id = target_id;
    info->u.modeInfoIdx = mode_index;
    info->outputTechnology = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL;
    info->rotation = get_dc_rotation(devmode);
    info->scaling = DISPLAYCONFIG_SCALING_IDENTITY;
    info->refreshRate.Numerator = devmode->dmDisplayFrequency;
    info->refreshRate.Denominator = 1;
    info->scanLineOrdering = get_dc_scanline_ordering(devmode);
    info->targetAvailable = TRUE;
    info->statusFlags = DISPLAYCONFIG_TARGET_IN_USE;
}

static void set_mode_source_info(DISPLAYCONFIG_MODE_INFO *info, const LUID *gpu_luid,
                                 UINT32 source_id, const DEVMODEW *devmode)
{
    DISPLAYCONFIG_SOURCE_MODE *mode = &(info->u.sourceMode);

    info->infoType = DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE;
    info->adapterId = *gpu_luid;
    info->id = source_id;

    mode->width = devmode->dmPelsWidth;
    mode->height = devmode->dmPelsHeight;
    mode->pixelFormat = get_dc_pixelformat(devmode->dmBitsPerPel);
    if (devmode->dmFields & DM_POSITION)
    {
        mode->position = devmode->u1.s2.dmPosition;
    }
    else
    {
        mode->position.x = 0;
        mode->position.y = 0;
    }
}

static void set_path_source_info(DISPLAYCONFIG_PATH_SOURCE_INFO *info, const LUID *gpu_luid,
                                 UINT32 source_id, UINT32 mode_index)
{
    info->adapterId = *gpu_luid;
    info->id = source_id;
    info->u.modeInfoIdx = mode_index;
    info->statusFlags = DISPLAYCONFIG_SOURCE_IN_USE;
}

static BOOL source_mode_exists(const DISPLAYCONFIG_MODE_INFO *modeinfo, UINT32 num_modes,
                               UINT32 source_id, UINT32 *found_mode_index)
{
    UINT32 i;

    for (i = 0; i < num_modes; i++)
    {
        if (modeinfo[i].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE &&
            modeinfo[i].id == source_id)
        {
            *found_mode_index = i;
            return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *              QueryDisplayConfig (USER32.@)
 */
LONG WINAPI QueryDisplayConfig(UINT32 flags, UINT32 *numpathelements, DISPLAYCONFIG_PATH_INFO *pathinfo,
                               UINT32 *numinfoelements, DISPLAYCONFIG_MODE_INFO *modeinfo,
                               DISPLAYCONFIG_TOPOLOGY_ID *topologyid)
{
    LONG adapter_index, ret;
    HANDLE mutex;
    HDEVINFO devinfo;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    DWORD monitor_index = 0, state_flags, type;
    UINT32 output_id, source_mode_index, path_index = 0, mode_index = 0;
    LUID gpu_luid;
    WCHAR device_name[CCHDEVICENAME];
    DEVMODEW devmode;

    FIXME("(%08x %p %p %p %p %p): semi-stub\n", flags, numpathelements, pathinfo, numinfoelements, modeinfo, topologyid);

    if (!numpathelements || !numinfoelements)
        return ERROR_INVALID_PARAMETER;

    if (!*numpathelements || !*numinfoelements)
        return ERROR_INVALID_PARAMETER;

    if (flags != QDC_ALL_PATHS &&
        flags != QDC_ONLY_ACTIVE_PATHS &&
        flags != QDC_DATABASE_CURRENT)
        return ERROR_INVALID_PARAMETER;

    if (((flags == QDC_DATABASE_CURRENT) && !topologyid) ||
        ((flags != QDC_DATABASE_CURRENT) && topologyid))
        return ERROR_INVALID_PARAMETER;

    if (flags != QDC_ONLY_ACTIVE_PATHS)
        FIXME("only returning active paths\n");

    if (topologyid)
    {
        FIXME("setting toplogyid to DISPLAYCONFIG_TOPOLOGY_INTERNAL\n");
        *topologyid = DISPLAYCONFIG_TOPOLOGY_INTERNAL;
    }

    wait_graphics_driver_ready();
    mutex = get_display_device_init_mutex();

    /* Iterate through "targets"/monitors.
     * Each target corresponds to a path, and each path corresponds to one or two unique modes.
     */
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, L"DISPLAY", NULL, DIGCF_PRESENT);
    if (devinfo == INVALID_HANDLE_VALUE)
    {
        ret = ERROR_GEN_FAILURE;
        goto done;
    }

    ret = ERROR_GEN_FAILURE;
    while (SetupDiEnumDeviceInfo(devinfo, monitor_index++, &device_data))
    {
        /* Only count active monitors */
        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_STATEFLAGS,
                                       &type, (BYTE *)&state_flags, sizeof(state_flags), NULL, 0))
            goto done;
        if (!(state_flags & DISPLAY_DEVICE_ACTIVE))
            continue;

        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_GPU_LUID,
                                       &type, (BYTE *)&gpu_luid, sizeof(gpu_luid), NULL, 0))
            goto done;

        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_OUTPUT_ID,
                                       &type, (BYTE *)&output_id, sizeof(output_id), NULL, 0))
            goto done;

        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_ADAPTERNAME,
                                       &type, (BYTE *)device_name, sizeof(device_name), NULL, 0))
            goto done;

        memset(&devmode, 0, sizeof(devmode));
        devmode.dmSize = sizeof(devmode);
        if (!EnumDisplaySettingsW(device_name, ENUM_CURRENT_SETTINGS, &devmode))
            goto done;

        /* Extract the adapter index from device_name to use as the source ID */
        adapter_index = wcstol(device_name + lstrlenW(L"\\\\.\\DISPLAY"), NULL, 10);
        adapter_index--;

        if (path_index == *numpathelements || mode_index == *numinfoelements)
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        pathinfo[path_index].flags = DISPLAYCONFIG_PATH_ACTIVE;
        set_mode_target_info(&modeinfo[mode_index], &gpu_luid, output_id, flags, &devmode);
        set_path_target_info(&(pathinfo[path_index].targetInfo), &gpu_luid, output_id, mode_index, &devmode);

        mode_index++;
        if (mode_index == *numinfoelements)
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        /* Multiple targets can be driven by the same source, ensure a mode
         * hasn't already been added for this source.
         */
        if (!source_mode_exists(modeinfo, mode_index, adapter_index, &source_mode_index))
        {
            set_mode_source_info(&modeinfo[mode_index], &gpu_luid, adapter_index, &devmode);
            source_mode_index = mode_index;
            mode_index++;
        }
        set_path_source_info(&(pathinfo[path_index].sourceInfo), &gpu_luid, adapter_index, source_mode_index);
        path_index++;
    }

    *numpathelements = path_index;
    *numinfoelements = mode_index;
    ret = ERROR_SUCCESS;

done:
    SetupDiDestroyDeviceInfoList(devinfo);
    release_display_device_init_mutex(mutex);
    return ret;
}

/***********************************************************************
 *              DisplayConfigGetDeviceInfo (USER32.@)
 */
LONG WINAPI DisplayConfigGetDeviceInfo(DISPLAYCONFIG_DEVICE_INFO_HEADER *packet)
{
    LONG ret = ERROR_GEN_FAILURE;
    HANDLE mutex;
    HDEVINFO devinfo;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    DWORD index = 0, type;
    LUID gpu_luid;

    TRACE("(%p)\n", packet);

    if (!packet || packet->size < sizeof(*packet))
        return ERROR_GEN_FAILURE;
    wait_graphics_driver_ready();

    switch (packet->type)
    {
    case DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME:
    {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME *source_name = (DISPLAYCONFIG_SOURCE_DEVICE_NAME *)packet;
        WCHAR device_name[CCHDEVICENAME];
        LONG source_id;

        TRACE("DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME\n");

        if (packet->size < sizeof(*source_name))
            return ERROR_INVALID_PARAMETER;

        mutex = get_display_device_init_mutex();
        devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, L"DISPLAY", NULL, DIGCF_PRESENT);
        if (devinfo == INVALID_HANDLE_VALUE)
        {
            release_display_device_init_mutex(mutex);
            return ret;
        }

        while (SetupDiEnumDeviceInfo(devinfo, index++, &device_data))
        {
            if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_GPU_LUID,
                                           &type, (BYTE *)&gpu_luid, sizeof(gpu_luid), NULL, 0))
                continue;

            if ((source_name->header.adapterId.LowPart != gpu_luid.LowPart) ||
                (source_name->header.adapterId.HighPart != gpu_luid.HighPart))
                continue;

            /* QueryDisplayConfig() derives the source ID from the adapter name. */
            if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_ADAPTERNAME,
                                           &type, (BYTE *)device_name, sizeof(device_name), NULL, 0))
                continue;

            source_id = wcstol(device_name + lstrlenW(L"\\\\.\\DISPLAY"), NULL, 10);
            source_id--;
            if (source_name->header.id != source_id)
                continue;

            lstrcpyW(source_name->viewGdiDeviceName, device_name);
            ret = ERROR_SUCCESS;
            break;
        }
        SetupDiDestroyDeviceInfoList(devinfo);
        release_display_device_init_mutex(mutex);
        return ret;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME:
    {
        DISPLAYCONFIG_TARGET_DEVICE_NAME *target_name = (DISPLAYCONFIG_TARGET_DEVICE_NAME *)packet;

        FIXME("DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME: stub\n");

        if (packet->size < sizeof(*target_name))
            return ERROR_INVALID_PARAMETER;

        return ERROR_NOT_SUPPORTED;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE:
    {
        DISPLAYCONFIG_TARGET_PREFERRED_MODE *preferred_mode = (DISPLAYCONFIG_TARGET_PREFERRED_MODE *)packet;

        FIXME("DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE: stub\n");

        if (packet->size < sizeof(*preferred_mode))
            return ERROR_INVALID_PARAMETER;

        return ERROR_NOT_SUPPORTED;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME:
    {
        DISPLAYCONFIG_ADAPTER_NAME *adapter_name = (DISPLAYCONFIG_ADAPTER_NAME *)packet;

        FIXME("DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME: stub\n");

        if (packet->size < sizeof(*adapter_name))
            return ERROR_INVALID_PARAMETER;

        return ERROR_NOT_SUPPORTED;
    }
    case DISPLAYCONFIG_DEVICE_INFO_SET_TARGET_PERSISTENCE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_BASE_TYPE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_SUPPORT_VIRTUAL_RESOLUTION:
    case DISPLAYCONFIG_DEVICE_INFO_SET_SUPPORT_VIRTUAL_RESOLUTION:
    case DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO:
    case DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL:
    default:
        FIXME("Unimplemented packet type: %u\n", packet->type);
        return ERROR_INVALID_PARAMETER;
    }
}

/***********************************************************************
 *              SetDisplayConfig (USER32.@)
 */
LONG WINAPI SetDisplayConfig(UINT32 path_info_count, DISPLAYCONFIG_PATH_INFO *path_info, UINT32 mode_info_count,
        DISPLAYCONFIG_MODE_INFO *mode_info, UINT32 flags)
{
    FIXME("path_info_count %u, path_info %p, mode_info_count %u, mode_info %p, flags %#x stub.\n",
            path_info_count, path_info, mode_info_count, mode_info, flags);

    return ERROR_SUCCESS;
}
