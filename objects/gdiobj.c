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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "bitmap.h"
#include "local.h"
#include "palette.h"
#include "gdi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

#define HGDIOBJ_32(h16)   ((HGDIOBJ)(ULONG_PTR)(h16))

/***********************************************************************
 *          GDI stock objects
 */

static const LOGBRUSH WhiteBrush = { BS_SOLID, RGB(255,255,255), 0 };
static const LOGBRUSH BlackBrush = { BS_SOLID, RGB(0,0,0), 0 };
static const LOGBRUSH NullBrush  = { BS_NULL, 0, 0 };

/* FIXME: these should perhaps be BS_HATCHED, at least for 1 bitperpixel */
static const LOGBRUSH LtGrayBrush = { BS_SOLID, RGB(192,192,192), 0 };
static const LOGBRUSH GrayBrush   = { BS_SOLID, RGB(128,128,128), 0 };

/* This is BS_HATCHED, for 1 bitperpixel. This makes the spray work in pbrush */
/* See HatchBrushes in x11drv for the HS_DIAGCROSS+1 hack */
static const LOGBRUSH DkGrayBrush = { BS_HATCHED, RGB(0,0,0), (HS_DIAGCROSS+1) };

static const LOGPEN WhitePen = { PS_SOLID, { 0, 0 }, RGB(255,255,255) };
static const LOGPEN BlackPen = { PS_SOLID, { 0, 0 }, RGB(0,0,0) };
static const LOGPEN NullPen  = { PS_NULL,  { 0, 0 }, 0 };


/* reserve one extra entry for the stock default bitmap */
/* this is what Windows does too */
#define NB_STOCK_OBJECTS (STOCK_LAST+2)

static HGDIOBJ stock_objects[NB_STOCK_OBJECTS];

static SYSLEVEL GDI_level;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &GDI_level.crst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": GDI_level") }
};
static SYSLEVEL GDI_level = { { &critsect_debug, -1, 0, 0, 0, 0 }, 3 };

static WORD GDI_HeapSel;

inline static BOOL get_bool(char *buffer)
{
    return (buffer[0] == 'y' || buffer[0] == 'Y' ||
            buffer[0] == 't' || buffer[0] == 'T' ||
            buffer[0] == '1');
}


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
 * language-dependenet stock fonts for all known charsets
 * please see TranslateCharsetInfo (objects/font.c) and
 * CharsetBindingInfo (graphics/x11drv/xfont.c),
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
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   EASTEUROPE_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   RUSSIAN_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   GREEK_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   TURKISH_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   HEBREW_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   ARABIC_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   BALTIC_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   THAI_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'}
        },
    },
    {   SHIFTJIS_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','P',' ','g','o','t','h','i','c','\0'} /* FIXME: Is this correct? */
        },
    },
    {   GB2312_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','o','n','g','\0'}   /* FIXME: Is this correct? */
        },
    },
    {   HANGEUL_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'G','u','l','i','m'},
        },
    },
    {   CHINESEBIG5_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}       /* FIXME - what is the native font??? */
        },
    },
    {   JOHAB_CHARSET,
        { /* System */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
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
         -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','M','i','n','g','l','i','u','\0'} /* FIXME: Is this correct? */
        },
    },
};


/******************************************************************************
 *      get_default_fonts
 */
static const struct DefaultFontInfo* get_default_fonts(UINT charset)
{
        int     n;

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
    if ( ! TranslateCharsetInfo( (LPDWORD)uACP, &csi, TCI_SRCCODEPAGE ) )
    {
        FIXME( "unhandled codepage %u - use ANSI_CHARSET for default stock objects\n", uACP );
        return ANSI_CHARSET;
    }

    return csi.ciCharset;
}


/******************************************************************************
 *           create_stock_font
 */
static HFONT create_stock_font( char const *fontName, const LOGFONTW *font, HKEY hkey )
{
    LOGFONTW lf;
    char  key[256];
    char buffer[MAX_PATH];
    DWORD type, count;

    if (!hkey) return CreateFontIndirectW( font );

    lf = *font;
    sprintf(key, "%s.Height", fontName);
    count = sizeof(buffer);
    if(!RegQueryValueExA(hkey, key, 0, &type, buffer, &count))
        lf.lfHeight = atoi(buffer);

    sprintf(key, "%s.Bold", fontName);
    count = sizeof(buffer);
    if(!RegQueryValueExA(hkey, key, 0, &type, buffer, &count))
        lf.lfWeight = get_bool(buffer) ? FW_BOLD : FW_NORMAL;

    sprintf(key, "%s.Italic", fontName);
    count = sizeof(buffer);
    if(!RegQueryValueExA(hkey, key, 0, &type, buffer, &count))
        lf.lfItalic = get_bool(buffer);

    sprintf(key, "%s.Underline", fontName);
    count = sizeof(buffer);
    if(!RegQueryValueExA(hkey, key, 0, &type, buffer, &count))
        lf.lfUnderline = get_bool(buffer);

    sprintf(key, "%s.StrikeOut", fontName);
    count = sizeof(buffer);
    if(!RegQueryValueExA(hkey, key, 0, &type, buffer, &count))
        lf.lfStrikeOut = get_bool(buffer);
    return CreateFontIndirectW( &lf );
}


#define TRACE_SEC(handle,text) \
   TRACE("(%p): " text " %ld\n", (handle), GDI_level.crst.RecursionCount)


/***********************************************************************
 *           inc_ref_count
 *
 * Increment the reference count of a GDI object.
 */
inline static void inc_ref_count( HGDIOBJ handle )
{
    GDIOBJHDR *header;

    if ((header = GDI_GetObjPtr( handle, MAGIC_DONTCARE )))
    {
        header->dwCount++;
        GDI_ReleaseObj( handle );
    }
}


/***********************************************************************
 *           dec_ref_count
 *
 * Decrement the reference count of a GDI object.
 */
inline static void dec_ref_count( HGDIOBJ handle )
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
    HINSTANCE16 instance;
    HKEY hkey;
    GDIOBJHDR *ptr;
    const struct DefaultFontInfo* deffonts;
    int i;

    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Tweak.Fonts", &hkey))
        hkey = 0;

    /* create GDI heap */
    if ((instance = LoadLibrary16( "GDI.EXE" )) >= 32) GDI_HeapSel = instance | 7;

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
    stock_objects[OEM_FIXED_FONT]      = create_stock_font( "OEMFixed", &OEMFixedFont, hkey );
    stock_objects[ANSI_FIXED_FONT]     = create_stock_font( "AnsiFixed", &AnsiFixedFont, hkey );
    stock_objects[ANSI_VAR_FONT]       = create_stock_font( "AnsiVar", &AnsiVarFont, hkey );

    /* language-dependent stock fonts */
    deffonts = get_default_fonts(get_default_charset());
    stock_objects[SYSTEM_FONT]         = create_stock_font( "System", &deffonts->SystemFont, hkey );
    stock_objects[DEVICE_DEFAULT_FONT] = create_stock_font( "DeviceDefault", &deffonts->DeviceDefaultFont, hkey );
    stock_objects[SYSTEM_FIXED_FONT]   = create_stock_font( "SystemFixed", &deffonts->SystemFixedFont, hkey );
    stock_objects[DEFAULT_GUI_FONT]    = create_stock_font( "DefaultGui", &deffonts->DefaultGuiFont, hkey );


    /* clear the NOSYSTEM bit on all stock objects*/
    for (i = 0; i < NB_STOCK_OBJECTS; i++)
    {
        if (!stock_objects[i])
        {
            if (i == 9) continue;  /* there's no stock object 9 */
            ERR( "could not create stock object %d\n", i );
            return FALSE;
        }
        ptr = GDI_GetObjPtr( stock_objects[i], MAGIC_DONTCARE );
        ptr->wMagic &= ~OBJECT_NOSYSTEM;
        GDI_ReleaseObj( stock_objects[i] );
    }

    if (hkey) RegCloseKey( hkey );

    WineEngInit();

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
inline static GDIOBJHDR *alloc_large_heap( WORD size, HGDIOBJ *handle )
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
    GDIOBJHDR *obj;
    HLOCAL16 hlocal;

    _EnterSysLevel( &GDI_level );
    switch(magic)
    {
    default:
        if (GDI_HeapSel)
        {
            if (!(hlocal = LOCAL_Alloc( GDI_HeapSel, LMEM_MOVEABLE, size ))) goto error;
            assert( hlocal & 2 );
            obj = (GDIOBJHDR *)LOCAL_Lock( GDI_HeapSel, hlocal );
            *handle = (HGDIOBJ)(ULONG_PTR)hlocal;
            break;
        }
        /* fall through */
    case DC_MAGIC:
    case DISABLED_DC_MAGIC:
    case META_DC_MAGIC:
    case METAFILE_MAGIC:
    case METAFILE_DC_MAGIC:
    case ENHMETAFILE_MAGIC:
    case ENHMETAFILE_DC_MAGIC:
    case MEMORY_DC_MAGIC:
    case BITMAP_MAGIC:
    case PALETTE_MAGIC:
        if (!(obj = alloc_large_heap( size, handle ))) goto error;
        break;
    }

    obj->hNext   = 0;
    obj->wMagic  = magic|OBJECT_NOSYSTEM;
    obj->dwCount = 0;
    obj->funcs   = funcs;

    TRACE_SEC( *handle, "enter" );
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
    HGDIOBJ new_handle;

    if ((UINT_PTR)handle & 2)  /* GDI heap handle */
    {
        HLOCAL16 h = LOWORD(handle);
        LOCAL_Unlock( GDI_HeapSel, h );
        if ((new_handle = (HGDIOBJ)(ULONG_PTR)LOCAL_ReAlloc( GDI_HeapSel, h, size, LMEM_MOVEABLE )))
        {
            assert( new_handle == handle );  /* moveable handle cannot change */
            return LOCAL_Lock( GDI_HeapSel, h );
        }
    }
    else
    {
        int i = ((ULONG_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
        if (i >= 0 && i < MAX_LARGE_HANDLES && large_handles[i])
        {
            void *new_ptr = HeapReAlloc( GetProcessHeap(), 0, large_handles[i], size );
            if (new_ptr)
            {
                large_handles[i] = new_ptr;
                return new_ptr;
            }
        }
        else ERR( "Invalid handle %p\n", handle );
    }
    TRACE_SEC( handle, "leave" );
    _LeaveSysLevel( &GDI_level );
    return NULL;
}


/***********************************************************************
 *           GDI_FreeObject
 */
BOOL GDI_FreeObject( HGDIOBJ handle, void *ptr )
{
    GDIOBJHDR *object = ptr;

    object->wMagic = 0;  /* Mark it as invalid */
    object->funcs  = NULL;
    if ((UINT_PTR)handle & 2)  /* GDI heap handle */
    {
        HLOCAL16 h = LOWORD(handle);
        LOCAL_Unlock( GDI_HeapSel, h );
        LOCAL_Free( GDI_HeapSel, h );
    }
    else  /* large heap handle */
    {
        int i = ((ULONG_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
        if (i >= 0 && i < MAX_LARGE_HANDLES && large_handles[i])
        {
            HeapFree( GetProcessHeap(), 0, large_handles[i] );
            large_handles[i] = NULL;
        }
        else ERR( "Invalid handle %p\n", handle );
    }
    TRACE_SEC( handle, "leave" );
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

    _EnterSysLevel( &GDI_level );

    if ((UINT_PTR)handle & 2)  /* GDI heap handle */
    {
        HLOCAL16 h = LOWORD(handle);
        ptr = (GDIOBJHDR *)LOCAL_Lock( GDI_HeapSel, h );
        if (ptr)
        {
            if (((magic != MAGIC_DONTCARE) && (GDIMAGIC(ptr->wMagic) != magic)) ||
                (GDIMAGIC(ptr->wMagic) < FIRST_MAGIC) ||
                (GDIMAGIC(ptr->wMagic) > LAST_MAGIC))
            {
                LOCAL_Unlock( GDI_HeapSel, h );
                ptr = NULL;
            }
        }
    }
    else  /* large heap handle */
    {
        int i = ((UINT_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
        if (i >= 0 && i < MAX_LARGE_HANDLES)
        {
            ptr = large_handles[i];
            if (ptr && (magic != MAGIC_DONTCARE) && (GDIMAGIC(ptr->wMagic) != magic)) ptr = NULL;
        }
    }

    if (!ptr)
    {
        _LeaveSysLevel( &GDI_level );
        SetLastError( ERROR_INVALID_HANDLE );
        WARN( "Invalid handle %p\n", handle );
    }
    else TRACE_SEC( handle, "enter" );

    return ptr;
}


/***********************************************************************
 *           GDI_ReleaseObj
 *
 */
void GDI_ReleaseObj( HGDIOBJ handle )
{
    if ((UINT_PTR)handle & 2) LOCAL_Unlock( GDI_HeapSel, LOWORD(handle) );
    TRACE_SEC( handle, "leave" );
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

    if (header->dwCount)
    {
        TRACE("delayed for %p because object in use, count %ld\n", obj, header->dwCount );
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
INT16 WINAPI GetObject16( HANDLE16 handle16, INT16 count, LPVOID buffer )
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
INT WINAPI GetObjectA( HANDLE handle, INT count, LPVOID buffer )
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
INT WINAPI GetObjectW( HANDLE handle, INT count, LPVOID buffer )
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
DWORD WINAPI GetObjectType( HANDLE handle )
{
    GDIOBJHDR * ptr;
    INT result = 0;
    TRACE("%p\n", handle );

    if (!(ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE ))) return 0;

    switch(GDIMAGIC(ptr->wMagic))
    {
      case PEN_MAGIC:
	  result = OBJ_PEN;
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
 */
HANDLE WINAPI GetCurrentObject(HDC hdc,UINT type)
{
    HANDLE ret = 0;
    DC * dc = DC_GetDCPtr( hdc );

    if (dc)
    {
    switch (type) {
	case OBJ_PEN:	 ret = dc->hPen; break;
	case OBJ_BRUSH:	 ret = dc->hBrush; break;
	case OBJ_PAL:	 ret = dc->hPalette; break;
	case OBJ_FONT:	 ret = dc->hFont; break;
	case OBJ_BITMAP: ret = dc->hBitmap; break;
    default:
    	/* the SDK only mentions those above */
    	FIXME("(%p,%d): unknown type.\n",hdc,type);
	    break;
        }
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           SelectObject    (GDI32.@)
 */
HGDIOBJ WINAPI SelectObject( HDC hdc, HGDIOBJ handle )
{
    HGDIOBJ ret = 0;
    GDIOBJHDR *header = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    if (!header) return 0;

    TRACE("hdc=%p %p\n", hdc, handle );

    if (header->funcs && header->funcs->pSelectObject)
    {
        ret = header->funcs->pSelectObject( handle, header, hdc );
        if (ret && ret != handle && (INT)ret > COMPLEXREGION)
        {
            inc_ref_count( handle );
            dec_ref_count( ret );
        }
    }
    GDI_ReleaseObj( handle );
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
    INT i, retval = 0;
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
            TRACE("solid pen %08lx, ret=%d\n",
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
            TRACE("solid brush %08lx, ret=%d\n",
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
        magic = GDIMAGIC(object->wMagic) - PEN_MAGIC + 1;
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
    switch (wReqType)
    {
    case 0x0001:  /* LocalAlloc */
        return LOCAL_Alloc( GDI_HeapSel, wParam1, wParam3 );
    case 0x0002:  /* LocalFree */
        return LOCAL_Free( GDI_HeapSel, wParam1 );
    case 0x0003:  /* LocalCompact */
        return LOCAL_Compact( GDI_HeapSel, wParam3, 0 );
    case 0x0103:  /* LocalHeap */
        return GDI_HeapSel;
    default:
        WARN("(wReqType=%04x): Unknown\n", wReqType);
        return (DWORD)-1;
    }
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
 */
HANDLE16 WINAPI GdiInit216(
    HANDLE16 h1, /* [in] GDI object */
    HANDLE16 h2  /* [in] global data */
)
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
   return (WORD)( (int)LOCAL_CountFree( GDI_HeapSel ) * 100 /
                  (int)LOCAL_HeapSize( GDI_HeapSel ) );
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
 *      GetMiterLimit [GDI32.@]
 *
 *
 */
BOOL WINAPI GetMiterLimit(HDC hdc, PFLOAT peLimit)
{
        FIXME("GetMiterLimit, stub\n");
        return 0;
}

/*******************************************************************
 *      SetMiterLimit [GDI32.@]
 *
 *
 */
BOOL WINAPI SetMiterLimit(HDC hdc, FLOAT eNewLimit, PFLOAT peOldLimit)
{
        FIXME("SetMiterLimit, stub\n");
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
    }
    GDI_ReleaseObj( hdc );
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
