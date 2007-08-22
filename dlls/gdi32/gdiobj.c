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

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winerror.h"
#include "winternl.h"

#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

#define HGDIOBJ_32(h16)   ((HGDIOBJ)(ULONG_PTR)(h16))

#define GDI_HEAP_SIZE 0xffe0

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

static SYSLEVEL GDI_level;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &GDI_level.crst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": GDI_level") }
};
static SYSLEVEL GDI_level = { { &critsect_debug, -1, 0, 0, 0, 0 }, 3 };


/****************************************************************************
 *
 *	language-independent stock fonts
 *
 */

static const LOGFONTW OEMFixedFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, {'\0'} };

static const LOGFONTW AnsiFixedFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, {'\0'} };

static const LOGFONTW AnsiVarFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
  {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'} };

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
        LOGFONTW        DefaultGuiFont; /* Note for this font the lfHeight member should be the point size */
};

static const struct DefaultFontInfo default_fonts[] =
{
    {   ANSI_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   EASTEUROPE_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   RUSSIAN_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   GREEK_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   TURKISH_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   HEBREW_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   ARABIC_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   BALTIC_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   THAI_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   SHIFTJIS_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','U','I',' ','G','o','t','h','i','c','\0'}
        },
    },
    {   GB2312_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','o','n','g','\0'}   /* FIXME: Is this correct? */
        },
    },
    {   HANGEUL_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   CHINESEBIG5_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}       /* FIXME - what is the native font??? */
        },
    },
    {   JOHAB_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
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
void __wine_make_gdi_object_system( HGDIOBJ handle, BOOL set)
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );

    /* touch the "system" bit of the wMagic field of a GDIOBJHDR */
    if (set)
        ptr->wMagic &= ~OBJECT_NOSYSTEM;
    else
        ptr->wMagic |= OBJECT_NOSYSTEM;

    GDI_ReleaseObj( handle );
}

/******************************************************************************
 *      get_default_fonts
 */
static const struct DefaultFontInfo* get_default_fonts(UINT charset)
{
        unsigned int n;

        for(n=0;n<(sizeof(default_fonts)/sizeof(default_fonts[0]));n++)
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

static const WCHAR dpi_key_name[] = {'S','o','f','t','w','a','r','e','\\','F','o','n','t','s','\0'};
static const WCHAR dpi_value_name[] = {'L','o','g','P','i','x','e','l','s','\0'};

/******************************************************************************
 *      get_dpi   (internal)
 *
 * get the dpi from the registry
 */
static DWORD get_dpi( void )
{
    DWORD dpi = 96;
    HKEY hkey;

    if (RegOpenKeyW(HKEY_CURRENT_CONFIG, dpi_key_name, &hkey) == ERROR_SUCCESS)
    {
        DWORD type, size, new_dpi;

        size = sizeof(new_dpi);
        if(RegQueryValueExW(hkey, dpi_value_name, NULL, &type, (void *)&new_dpi, &size) == ERROR_SUCCESS)
        {
            if(type == REG_DWORD && new_dpi != 0)
                dpi = new_dpi;
        }
        RegCloseKey(hkey);
    }
    return dpi;
}


/***********************************************************************
 *           inc_ref_count
 *
 * Increment the reference count of a GDI object.
 */
static inline void inc_ref_count( GDIOBJHDR *header )
{
    header->dwCount++;
}


/***********************************************************************
 *           dec_ref_count
 *
 * Decrement the reference count of a GDI object.
 */
static inline void dec_ref_count( HGDIOBJ handle )
{
    GDIOBJHDR *header;

    if ((header = GDI_GetObjPtr( handle, MAGIC_DONTCARE )))
    {
        if (header->dwCount) header->dwCount--;
        if (header->dwCount != 0x80000000) GDI_ReleaseObj( handle );
        else
        {
            /* handle delayed DeleteObject*/
            header->dwCount = 0;
            GDI_ReleaseObj( handle );
            TRACE( "executing delayed DeleteObject for %p\n", handle );
            DeleteObject( handle );
        }
    }
}


/***********************************************************************
 *           GDI_Init
 *
 * GDI initialization.
 */
BOOL GDI_Init(void)
{
    LOGFONTW default_gui_font;
    const struct DefaultFontInfo* deffonts;
    int i;

    WineEngInit();

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

    /* For the default gui font, we use the lfHeight member in deffonts as a place-holder
       for the point size so we must convert this into a true height */
    memcpy(&default_gui_font, &deffonts->DefaultGuiFont, sizeof(default_gui_font));
    default_gui_font.lfHeight = -MulDiv(default_gui_font.lfHeight, get_dpi(), 72);
    stock_objects[DEFAULT_GUI_FONT]    = CreateFontIndirectW( &default_gui_font );

    stock_objects[DC_BRUSH]     = CreateBrushIndirect( &DCBrush );
    stock_objects[DC_PEN]       = CreatePenIndirect( &DCPen );

    /* clear the NOSYSTEM bit on all stock objects*/
    for (i = 0; i < NB_STOCK_OBJECTS; i++)
    {
        if (!stock_objects[i])
        {
            if (i == 9) continue;  /* there's no stock object 9 */
            ERR( "could not create stock object %d\n", i );
            return FALSE;
        }
        __wine_make_gdi_object_system( stock_objects[i], TRUE );
    }

    return TRUE;
}

#define FIRST_LARGE_HANDLE 16
#define MAX_LARGE_HANDLES ((GDI_HEAP_SIZE >> 2) - FIRST_LARGE_HANDLE)
static GDIOBJHDR *large_handles[MAX_LARGE_HANDLES];
static int next_large_handle;

/***********************************************************************
 *           alloc_large_heap
 *
 * Allocate a GDI handle from the large heap. Helper for GDI_AllocObject
 */
static inline GDIOBJHDR *alloc_large_heap( WORD size, HGDIOBJ *handle )
{
    int i;
    GDIOBJHDR *obj;

    for (i = next_large_handle + 1; i < MAX_LARGE_HANDLES; i++)
        if (!large_handles[i]) goto found;
    for (i = 0; i <= next_large_handle; i++)
        if (!large_handles[i]) goto found;
    *handle = 0;
    return NULL;

 found:
    if ((obj = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        large_handles[i] = obj;
        *handle = (HGDIOBJ)(ULONG_PTR)((i + FIRST_LARGE_HANDLE) << 2);
        next_large_handle = i;
    }
    return obj;
}


/***********************************************************************
 *           GDI_AllocObject
 */
void *GDI_AllocObject( WORD size, WORD magic, HGDIOBJ *handle, const struct gdi_obj_funcs *funcs )
{
    GDIOBJHDR *obj = NULL;

    _EnterSysLevel( &GDI_level );
    if (!(obj = alloc_large_heap( size, handle ))) goto error;

    obj->wMagic  = magic|OBJECT_NOSYSTEM;
    obj->dwCount = 0;
    obj->funcs   = funcs;
    obj->hdcs    = NULL;

    TRACE("(%p): enter %d\n", *handle, GDI_level.crst.RecursionCount);
    return obj;

error:
    _LeaveSysLevel( &GDI_level );
    *handle = 0;
    return NULL;
}


/***********************************************************************
 *           GDI_ReallocObject
 *
 * The object ptr must have been obtained with GDI_GetObjPtr.
 * The new pointer must be released with GDI_ReleaseObj.
 */
void *GDI_ReallocObject( WORD size, HGDIOBJ handle, void *object )
{
    void *new_ptr = NULL;
    int i;

    i = ((ULONG_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
    if (i >= 0 && i < MAX_LARGE_HANDLES && large_handles[i])
    {
        new_ptr = HeapReAlloc( GetProcessHeap(), 0, large_handles[i], size );
        if (new_ptr) large_handles[i] = new_ptr;
    }
    else ERR( "Invalid handle %p\n", handle );
    if (!new_ptr)
    {
        TRACE("(%p): leave %d\n", handle, GDI_level.crst.RecursionCount);
        _LeaveSysLevel( &GDI_level );
    }
    return new_ptr;
}


/***********************************************************************
 *           GDI_FreeObject
 */
BOOL GDI_FreeObject( HGDIOBJ handle, void *ptr )
{
    GDIOBJHDR *object = ptr;
    int i;

    object->wMagic = 0;  /* Mark it as invalid */
    object->funcs  = NULL;
    i = ((ULONG_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
    if (i >= 0 && i < MAX_LARGE_HANDLES)
    {
        HeapFree( GetProcessHeap(), 0, large_handles[i] );
        large_handles[i] = NULL;
    }
    else ERR( "Invalid handle %p\n", handle );
    TRACE("(%p): leave %d\n", handle, GDI_level.crst.RecursionCount);
    _LeaveSysLevel( &GDI_level );
    return TRUE;
}


/***********************************************************************
 *           GDI_GetObjPtr
 *
 * Return a pointer to the GDI object associated to the handle.
 * Return NULL if the object has the wrong magic number.
 * The object must be released with GDI_ReleaseObj.
 */
void *GDI_GetObjPtr( HGDIOBJ handle, WORD magic )
{
    GDIOBJHDR *ptr = NULL;
    int i;

    _EnterSysLevel( &GDI_level );

    i = ((UINT_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
    if (i >= 0 && i < MAX_LARGE_HANDLES)
    {
        ptr = large_handles[i];
        if (ptr && (magic != MAGIC_DONTCARE) && (GDIMAGIC(ptr->wMagic) != magic)) ptr = NULL;
    }

    if (!ptr)
    {
        _LeaveSysLevel( &GDI_level );
        WARN( "Invalid handle %p\n", handle );
    }
    else TRACE("(%p): enter %d\n", handle, GDI_level.crst.RecursionCount);

    return ptr;
}


/***********************************************************************
 *           GDI_ReleaseObj
 *
 */
void GDI_ReleaseObj( HGDIOBJ handle )
{
    TRACE("(%p): leave %d\n", handle, GDI_level.crst.RecursionCount);
    _LeaveSysLevel( &GDI_level );
}


/***********************************************************************
 *           GDI_CheckNotLock
 */
void GDI_CheckNotLock(void)
{
    _CheckNotSysLevel( &GDI_level );
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
      /* Check if object is valid */

    GDIOBJHDR * header;
    if (HIWORD(obj)) return FALSE;

    if (!(header = GDI_GetObjPtr( obj, MAGIC_DONTCARE ))) return FALSE;

    if (!(header->wMagic & OBJECT_NOSYSTEM)
    &&   (header->wMagic >= FIRST_MAGIC) && (header->wMagic <= LAST_MAGIC))
    {
	TRACE("Preserving system object %p\n", obj);
        GDI_ReleaseObj( obj );
	return TRUE;
    }

    while (header->hdcs)
    {
        DC *dc = DC_GetDCPtr(header->hdcs->hdc);
        struct hdc_list *tmp;

        TRACE("hdc %p has interest in %p\n", header->hdcs->hdc, obj);
        if(dc)
        {
            if(dc->funcs->pDeleteObject)
                dc->funcs->pDeleteObject( dc->physDev, obj );
            DC_ReleaseDCPtr( dc );
        }
        tmp = header->hdcs;
        header->hdcs = header->hdcs->next;
        HeapFree(GetProcessHeap(), 0, tmp);
    }

    if (header->dwCount)
    {
        TRACE("delayed for %p because object in use, count %d\n", obj, header->dwCount );
        header->dwCount |= 0x80000000; /* mark for delete */
        GDI_ReleaseObj( obj );
        return TRUE;
    }

    TRACE("%p\n", obj );

      /* Delete object */

    if (header->funcs && header->funcs->pDeleteObject)
        return header->funcs->pDeleteObject( obj, header );

    GDI_ReleaseObj( obj );
    return FALSE;
}

/***********************************************************************
 *           GDI_hdc_using_object
 *
 * Call this if the dc requires DeleteObject notification
 */
BOOL GDI_hdc_using_object(HGDIOBJ obj, HDC hdc)
{
    GDIOBJHDR * header;
    struct hdc_list **pphdc;

    TRACE("obj %p hdc %p\n", obj, hdc);

    if (!(header = GDI_GetObjPtr( obj, MAGIC_DONTCARE ))) return FALSE;

    if (!(header->wMagic & OBJECT_NOSYSTEM) &&
         (header->wMagic >= FIRST_MAGIC) && (header->wMagic <= LAST_MAGIC))
    {
        GDI_ReleaseObj(obj);
        return FALSE;
    }

    for(pphdc = &header->hdcs; *pphdc; pphdc = &(*pphdc)->next)
        if((*pphdc)->hdc == hdc)
            break;

    if(!*pphdc) {
        *pphdc = HeapAlloc(GetProcessHeap(), 0, sizeof(**pphdc));
        (*pphdc)->hdc = hdc;
        (*pphdc)->next = NULL;
    }

    GDI_ReleaseObj(obj);
    return TRUE;
}

/***********************************************************************
 *           GDI_hdc_not_using_object
 *
 */
BOOL GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc)
{
    GDIOBJHDR * header;
    struct hdc_list *phdc, **prev;

    TRACE("obj %p hdc %p\n", obj, hdc);

    if (!(header = GDI_GetObjPtr( obj, MAGIC_DONTCARE ))) return FALSE;

    if (!(header->wMagic & OBJECT_NOSYSTEM) &&
         (header->wMagic >= FIRST_MAGIC) && (header->wMagic <= LAST_MAGIC))
    {
        GDI_ReleaseObj(obj);
        return FALSE;
    }

    phdc = header->hdcs;
    prev = &header->hdcs;

    while(phdc) {
        if(phdc->hdc == hdc) {
            *prev = phdc->next;
            HeapFree(GetProcessHeap(), 0, phdc);
            phdc = *prev;
        } else {
            prev = &phdc->next;
            phdc = phdc->next;
        }
    }

    GDI_ReleaseObj(obj);
    return TRUE;
}

/***********************************************************************
 *           GetStockObject    (GDI32.@)
 */
HGDIOBJ WINAPI GetStockObject( INT obj )
{
    HGDIOBJ ret;
    if ((obj < 0) || (obj >= NB_STOCK_OBJECTS)) return 0;
    ret = stock_objects[obj];
    TRACE("returning %p\n", ret );
    return ret;
}


/***********************************************************************
 *           GetObject    (GDI.82)
 */
INT16 WINAPI GetObject16( HGDIOBJ16 handle16, INT16 count, LPVOID buffer )
{
    GDIOBJHDR * ptr;
    HGDIOBJ handle = HGDIOBJ_32( handle16 );
    INT16 result = 0;

    TRACE("%p %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if (!(ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE ))) return 0;

    if (ptr->funcs && ptr->funcs->pGetObject16)
        result = ptr->funcs->pGetObject16( handle, ptr, count, buffer );
    else
        SetLastError( ERROR_INVALID_HANDLE );

    GDI_ReleaseObj( handle );
    return result;
}


/***********************************************************************
 *           GetObjectA    (GDI32.@)
 */
INT WINAPI GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer )
{
    GDIOBJHDR * ptr;
    INT result = 0;
    TRACE("%p %d %p\n", handle, count, buffer );

    if (!(ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE ))) return 0;

    if (ptr->funcs && ptr->funcs->pGetObjectA)
        result = ptr->funcs->pGetObjectA( handle, ptr, count, buffer );
    else
        SetLastError( ERROR_INVALID_HANDLE );

    GDI_ReleaseObj( handle );
    return result;
}

/***********************************************************************
 *           GetObjectW    (GDI32.@)
 */
INT WINAPI GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer )
{
    GDIOBJHDR * ptr;
    INT result = 0;
    TRACE("%p %d %p\n", handle, count, buffer );

    if (!(ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE ))) return 0;

    if (ptr->funcs && ptr->funcs->pGetObjectW)
        result = ptr->funcs->pGetObjectW( handle, ptr, count, buffer );
    else
        SetLastError( ERROR_INVALID_HANDLE );

    GDI_ReleaseObj( handle );
    return result;
}

/***********************************************************************
 *           GetObjectType    (GDI32.@)
 */
DWORD WINAPI GetObjectType( HGDIOBJ handle )
{
    GDIOBJHDR * ptr;
    INT result = 0;
    TRACE("%p\n", handle );

    if (!(ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    switch(GDIMAGIC(ptr->wMagic))
    {
      case PEN_MAGIC:
	  result = OBJ_PEN;
	  break;
      case EXT_PEN_MAGIC:
	  result = OBJ_EXTPEN;
	  break;
      case BRUSH_MAGIC:
	  result = OBJ_BRUSH;
	  break;
      case BITMAP_MAGIC:
	  result = OBJ_BITMAP;
	  break;
      case FONT_MAGIC:
	  result = OBJ_FONT;
	  break;
      case PALETTE_MAGIC:
	  result = OBJ_PAL;
	  break;
      case REGION_MAGIC:
	  result = OBJ_REGION;
	  break;
      case DC_MAGIC:
	  result = OBJ_DC;
	  break;
      case META_DC_MAGIC:
	  result = OBJ_METADC;
	  break;
      case METAFILE_MAGIC:
	  result = OBJ_METAFILE;
	  break;
      case METAFILE_DC_MAGIC:
	  result = OBJ_METADC;
	  break;
      case ENHMETAFILE_MAGIC:
	  result = OBJ_ENHMETAFILE;
	  break;
      case ENHMETAFILE_DC_MAGIC:
	  result = OBJ_ENHMETADC;
	  break;
      case MEMORY_DC_MAGIC:
	  result = OBJ_MEMDC;
	  break;
      default:
	  FIXME("Magic %04x not implemented\n", GDIMAGIC(ptr->wMagic) );
	  break;
    }
    GDI_ReleaseObj( handle );
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
    DC * dc = DC_GetDCPtr( hdc );

    if (dc)
    {
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
        DC_ReleaseDCPtr( dc );
    }
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
    HGDIOBJ ret = 0;
    GDIOBJHDR *header;
    DC *dc;

    TRACE( "(%p,%p)\n", hdc, hObj );

    if (!(dc = DC_GetDCPtr( hdc )))
        SetLastError( ERROR_INVALID_HANDLE );
    else
    {
        DC_ReleaseDCPtr( dc );

        header = GDI_GetObjPtr( hObj, MAGIC_DONTCARE );
        if (header)
        {
            if (header->funcs && header->funcs->pSelectObject)
            {
                ret = header->funcs->pSelectObject( hObj, header, hdc );
                if (ret && ret != hObj && HandleToULong(ret) > COMPLEXREGION)
                {
                    inc_ref_count( header );
                    dec_ref_count( ret );
                }
            }
	    GDI_ReleaseObj( hObj );
        }
    }
    return ret;
}


/***********************************************************************
 *           UnrealizeObject    (GDI32.@)
 */
BOOL WINAPI UnrealizeObject( HGDIOBJ obj )
{
    BOOL result = TRUE;
  /* Check if object is valid */

    GDIOBJHDR * header = GDI_GetObjPtr( obj, MAGIC_DONTCARE );
    if (!header) return FALSE;

    TRACE("%p\n", obj );

      /* Unrealize object */

    if (header->funcs && header->funcs->pUnrealizeObject)
        result = header->funcs->pUnrealizeObject( obj, header );

    GDI_ReleaseObj( obj );
    return result;
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
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
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
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
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
 *           IsGDIObject    (GDI.462)
 *
 * returns type of object if valid (W95 system programming secrets p. 264-5)
 */
BOOL16 WINAPI IsGDIObject16( HGDIOBJ16 handle16 )
{
    UINT16 magic = 0;
    HGDIOBJ handle = HGDIOBJ_32( handle16 );

    GDIOBJHDR *object = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    if (object)
    {
        magic = GDIMAGIC(object->wMagic) - FIRST_MAGIC + 1;
        GDI_ReleaseObj( handle );
    }
    return magic;
}


/***********************************************************************
 *           SetObjectOwner    (GDI32.@)
 */
void WINAPI SetObjectOwner( HGDIOBJ handle, HANDLE owner )
{
    /* Nothing to do */
}


/***********************************************************************
 *           MakeObjectPrivate    (GDI.463)
 *
 * What does that mean ?
 * Some little docu can be found in "Undocumented Windows",
 * but this is basically useless.
 * At least we know that this flags the GDI object's wMagic
 * with 0x2000 (OBJECT_PRIVATE), so we just do it.
 * But Wine doesn't react on that yet.
 */
void WINAPI MakeObjectPrivate16( HGDIOBJ16 handle16, BOOL16 private )
{
    HGDIOBJ handle = HGDIOBJ_32( handle16 );
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    if (!ptr)
    {
	ERR("invalid GDI object %p !\n", handle);
	return;
    }
    ptr->wMagic |= OBJECT_PRIVATE;
    GDI_ReleaseObj( handle );
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


/***********************************************************************
 *           GdiSeeGdiDo   (GDI.452)
 */
DWORD WINAPI GdiSeeGdiDo16( WORD wReqType, WORD wParam1, WORD wParam2,
                          WORD wParam3 )
{
    DWORD ret = ~0U;

    switch (wReqType)
    {
    case 0x0001:  /* LocalAlloc */
        WARN("LocalAlloc16(%x, %x): ignoring\n", wParam1, wParam3);
        ret = 0;
        break;
    case 0x0002:  /* LocalFree */
        WARN("LocalFree16(%x): ignoring\n", wParam1);
        ret = 0;
        break;
    case 0x0003:  /* LocalCompact */
        WARN("LocalCompact16(%x): ignoring\n", wParam3);
        ret = 65000; /* lie about the amount of free space */
        break;
    case 0x0103:  /* LocalHeap */
        WARN("LocalHeap16(): ignoring\n");
        break;
    default:
        WARN("(wReqType=%04x): Unknown\n", wReqType);
        break;
    }
    return ret;
}

/***********************************************************************
 *           GdiSignalProc32     (GDI.610)
 */
WORD WINAPI GdiSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                           DWORD dwFlags, HMODULE16 hModule )
{
    return 0;
}

/***********************************************************************
 *           GdiInit2     (GDI.403)
 *
 * See "Undocumented Windows"
 *
 * PARAMS
 *   h1 [I] GDI object
 *   h2 [I] global data
 */
HANDLE16 WINAPI GdiInit216( HANDLE16 h1, HANDLE16 h2 )
{
    FIXME("(%04x, %04x), stub.\n", h1, h2);
    if (h2 == 0xffff)
	return 0xffff; /* undefined return value */
    return h1; /* FIXME: should be the memory handle of h1 */
}

/***********************************************************************
 *           FinalGdiInit     (GDI.405)
 */
void WINAPI FinalGdiInit16( HBRUSH16 hPattern /* [in] fill pattern of desktop */ )
{
}

/***********************************************************************
 *           GdiFreeResources   (GDI.609)
 */
WORD WINAPI GdiFreeResources16( DWORD reserve )
{
    return 90; /* lie about it, it shouldn't matter */
}


/*******************************************************************
 *      GetColorAdjustment [GDI32.@]
 *
 *
 */
BOOL WINAPI GetColorAdjustment(HDC hdc, LPCOLORADJUSTMENT lpca)
{
        FIXME("GetColorAdjustment, stub\n");
        return 0;
}

/*******************************************************************
 *      GdiComment [GDI32.@]
 *
 *
 */
BOOL WINAPI GdiComment(HDC hdc, UINT cbSize, const BYTE *lpData)
{
    DC *dc = DC_GetDCPtr(hdc);
    BOOL ret = FALSE;
    if(dc)
    {
        if (dc->funcs->pGdiComment)
            ret = dc->funcs->pGdiComment( dc->physDev, cbSize, lpData );
        DC_ReleaseDCPtr( dc );
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
        FIXME("SetColorAdjustment, stub\n");
        return 0;
}
