/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
 * Copyright 2002,2003 Shachar Shemesh
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

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winreg.h"
#include "gdi_private.h"
#include "resource.h"
#include "wine/exception.h"
#include "wine/heap.h"
#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

static HKEY wine_fonts_key;
static HKEY wine_fonts_cache_key;

struct font_physdev
{
    struct gdi_physdev dev;
    struct gdi_font   *font;
};

static inline struct font_physdev *get_font_dev( PHYSDEV dev )
{
    return (struct font_physdev *)dev;
}

struct gdi_font_family
{
    struct wine_rb_entry    name_entry;
    struct wine_rb_entry    second_name_entry;
    unsigned int            refcount;
    WCHAR                   family_name[LF_FACESIZE];
    WCHAR                   second_name[LF_FACESIZE];
    struct list             faces;
    struct gdi_font_family *replacement;
};

struct gdi_font_face
{
    struct list   entry;
    unsigned int  refcount;
    WCHAR        *style_name;
    WCHAR        *full_name;
    WCHAR        *file;
    void         *data_ptr;
    SIZE_T        data_size;
    UINT          face_index;
    FONTSIGNATURE fs;
    DWORD         ntmFlags;
    DWORD         version;
    DWORD         flags;                 /* ADDFONT flags */
    BOOL          scalable;
    struct bitmap_font_size    size;     /* set if face is a bitmap */
    struct gdi_font_family    *family;
    struct gdi_font_enum_data *cached_enum_data;
    struct wine_rb_entry       full_name_entry;
};

static const struct font_backend_funcs *font_funcs;

static const MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };

static UINT font_smoothing = GGO_BITMAP;
static UINT subpixel_orientation = GGO_GRAY4_BITMAP;
static BOOL antialias_fakes = TRUE;
static struct font_gamma_ramp font_gamma_ramp;

static void add_face_to_cache( struct gdi_font_face *face );
static void remove_face_from_cache( struct gdi_font_face *face );

static inline WCHAR facename_tolower( WCHAR c )
{
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    else if (c > 127) return RtlDowncaseUnicodeChar( c );
    else return c;
}

static inline int facename_compare( const WCHAR *str1, const WCHAR *str2, SIZE_T len )
{
    while (len--)
    {
        WCHAR c1 = facename_tolower( *str1++ ), c2 = facename_tolower( *str2++ );
        if (c1 != c2) return c1 - c2;
        else if (!c1) return 0;
    }
    return 0;
}

  /* Device -> World size conversion */

/* Performs a device to world transformation on the specified width (which
 * is in integer format).
 */
static inline INT INTERNAL_XDSTOWS(DC *dc, INT width)
{
    double floatWidth;

    /* Perform operation with floating point */
    floatWidth = (double)width * dc->xformVport2World.eM11;
    /* Round to integers */
    return GDI_ROUND(floatWidth);
}

/* Performs a device to world transformation on the specified size (which
 * is in integer format).
 */
static inline INT INTERNAL_YDSTOWS(DC *dc, INT height)
{
    double floatHeight;

    /* Perform operation with floating point */
    floatHeight = (double)height * dc->xformVport2World.eM22;
    /* Round to integers */
    return GDI_ROUND(floatHeight);
}

/* scale width and height but don't mirror them */

static inline INT width_to_LP( DC *dc, INT width )
{
    return GDI_ROUND( (double)width * fabs( dc->xformVport2World.eM11 ));
}

static inline INT height_to_LP( DC *dc, INT height )
{
    return GDI_ROUND( (double)height * fabs( dc->xformVport2World.eM22 ));
}

static inline INT INTERNAL_YWSTODS(DC *dc, INT height)
{
    POINT pt[2];
    pt[0].x = pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    lp_to_dp(dc, pt, 2);
    return pt[1].y - pt[0].y;
}

static inline BOOL is_win9x(void)
{
    return GetVersion() & 0x80000000;
}

static inline WCHAR *strdupW( const WCHAR *p )
{
    WCHAR *ret;
    DWORD len = (lstrlenW(p) + 1) * sizeof(WCHAR);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(ret, p, len);
    return ret;
}

static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, HDC hdc );
static INT FONT_GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer );
static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL FONT_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs fontobj_funcs =
{
    FONT_SelectObject,  /* pSelectObject */
    FONT_GetObjectA,    /* pGetObjectA */
    FONT_GetObjectW,    /* pGetObjectW */
    NULL,               /* pUnrealizeObject */
    FONT_DeleteObject   /* pDeleteObject */
};

typedef struct
{
    LOGFONTW    logfont;
} FONTOBJ;

struct font_enum
{
  LPLOGFONTW          lpLogFontParam;
  FONTENUMPROCW       lpEnumFunc;
  LPARAM              lpData;
  BOOL                unicode;
  HDC                 hdc;
  INT                 retval;
};

/*
 *  For TranslateCharsetInfo
 */
#define MAXTCIINDEX 32
static const CHARSETINFO FONT_tci[MAXTCIINDEX] = {
  /* ANSI */
  { ANSI_CHARSET, 1252, {{0,0,0,0},{FS_LATIN1,0}} },
  { EASTEUROPE_CHARSET, 1250, {{0,0,0,0},{FS_LATIN2,0}} },
  { RUSSIAN_CHARSET, 1251, {{0,0,0,0},{FS_CYRILLIC,0}} },
  { GREEK_CHARSET, 1253, {{0,0,0,0},{FS_GREEK,0}} },
  { TURKISH_CHARSET, 1254, {{0,0,0,0},{FS_TURKISH,0}} },
  { HEBREW_CHARSET, 1255, {{0,0,0,0},{FS_HEBREW,0}} },
  { ARABIC_CHARSET, 1256, {{0,0,0,0},{FS_ARABIC,0}} },
  { BALTIC_CHARSET, 1257, {{0,0,0,0},{FS_BALTIC,0}} },
  { VIETNAMESE_CHARSET, 1258, {{0,0,0,0},{FS_VIETNAMESE,0}} },
  /* reserved by ANSI */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  /* ANSI and OEM */
  { THAI_CHARSET, 874, {{0,0,0,0},{FS_THAI,0}} },
  { SHIFTJIS_CHARSET, 932, {{0,0,0,0},{FS_JISJAPAN,0}} },
  { GB2312_CHARSET, 936, {{0,0,0,0},{FS_CHINESESIMP,0}} },
  { HANGEUL_CHARSET, 949, {{0,0,0,0},{FS_WANSUNG,0}} },
  { CHINESEBIG5_CHARSET, 950, {{0,0,0,0},{FS_CHINESETRAD,0}} },
  { JOHAB_CHARSET, 1361, {{0,0,0,0},{FS_JOHAB,0}} },
  /* reserved for alternate ANSI and OEM */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  /* reserved for system */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { SYMBOL_CHARSET, CP_SYMBOL, {{0,0,0,0},{FS_SYMBOL,0}} }
};

static const WCHAR * const default_serif_list[3] =
{
    L"Times New Roman",
    L"Liberation Serif",
    L"Bitstream Vera Serif"
};
static const WCHAR * const default_fixed_list[3] =
{
    L"Courier New",
    L"Liberation Mono",
    L"Bitstream Vera Sans Mono"
};
static const WCHAR * const default_sans_list[3] =
{
    L"Arial",
    L"Liberation Sans",
    L"Bitstream Vera Sans"
};
static WCHAR ff_roman_default[LF_FACESIZE];
static WCHAR ff_modern_default[LF_FACESIZE];
static WCHAR ff_swiss_default[LF_FACESIZE];

static const struct nls_update_font_list
{
    UINT ansi_cp, oem_cp;
    const char *oem, *fixed, *system;
    const char *courier, *serif, *small, *sserif_96, *sserif_120;
    /* these are for font substitutes */
    const char *shelldlg, *tmsrmn;
    const char *fixed_0, *system_0, *courier_0, *serif_0, *small_0, *sserif_0, *helv_0, *tmsrmn_0;
    struct subst { const char *from, *to; } arial_0, courier_new_0, times_new_roman_0;
} nls_update_font_list[] =
{
    /* Latin 1 (United States) */
    { 1252, 437, "vgaoem.fon", "vgafix.fon", "vgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon", "sseriff.fon",
      "Tahoma","Times New Roman"
    },
    /* Latin 1 (Multilingual) */
    { 1252, 850, "vga850.fon", "vgafix.fon", "vgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon", "sseriff.fon",
      "Tahoma","Times New Roman"  /* FIXME unverified */
    },
    /* Eastern Europe */
    { 1250, 852, "vga852.fon", "vgafixe.fon", "vgasyse.fon",
      "couree.fon", "serifee.fon", "smallee.fon", "sserifee.fon", "sseriffe.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
      "Fixedsys,238", "System,238",
      "Courier New,238", "MS Serif,238", "Small Fonts,238",
      "MS Sans Serif,238", "MS Sans Serif,238", "MS Serif,238",
      { "Arial CE,0", "Arial,238" },
      { "Courier New CE,0", "Courier New,238" },
      { "Times New Roman CE,0", "Times New Roman,238" }
    },
    /* Cyrillic */
    { 1251, 866, "vga866.fon", "vgafixr.fon", "vgasysr.fon",
      "courer.fon", "serifer.fon", "smaller.fon", "sserifer.fon", "sseriffr.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
      "Fixedsys,204", "System,204",
      "Courier New,204", "MS Serif,204", "Small Fonts,204",
      "MS Sans Serif,204", "MS Sans Serif,204", "MS Serif,204",
      { "Arial Cyr,0", "Arial,204" },
      { "Courier New Cyr,0", "Courier New,204" },
      { "Times New Roman Cyr,0", "Times New Roman,204" }
    },
    /* Greek */
    { 1253, 737, "vga869.fon", "vgafixg.fon", "vgasysg.fon",
      "coureg.fon", "serifeg.fon", "smalleg.fon", "sserifeg.fon", "sseriffg.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
      "Fixedsys,161", "System,161",
      "Courier New,161", "MS Serif,161", "Small Fonts,161",
      "MS Sans Serif,161", "MS Sans Serif,161", "MS Serif,161",
      { "Arial Greek,0", "Arial,161" },
      { "Courier New Greek,0", "Courier New,161" },
      { "Times New Roman Greek,0", "Times New Roman,161" }
    },
    /* Turkish */
    { 1254, 857, "vga857.fon", "vgafixt.fon", "vgasyst.fon",
      "couret.fon", "serifet.fon", "smallet.fon", "sserifet.fon", "sserifft.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
      "Fixedsys,162", "System,162",
      "Courier New,162", "MS Serif,162", "Small Fonts,162",
      "MS Sans Serif,162", "MS Sans Serif,162", "MS Serif,162",
      { "Arial Tur,0", "Arial,162" },
      { "Courier New Tur,0", "Courier New,162" },
      { "Times New Roman Tur,0", "Times New Roman,162" }
    },
    /* Hebrew */
    { 1255, 862, "vgaoem.fon", "vgaf1255.fon", "vgas1255.fon",
      "coue1255.fon", "sere1255.fon", "smae1255.fon", "ssee1255.fon", "ssef1255.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
      "Fixedsys,177", "System,177",
      "Courier New,177", "MS Serif,177", "Small Fonts,177",
      "MS Sans Serif,177", "MS Sans Serif,177", "MS Serif,177"
    },
    /* Arabic */
    { 1256, 720, "vgaoem.fon", "vgaf1256.fon", "vgas1256.fon",
      "coue1256.fon", "sere1256.fon", "smae1256.fon", "ssee1256.fon", "ssef1256.fon",
      "Microsoft Sans Serif","Times New Roman",
      "Fixedsys,178", "System,178",
      "Courier New,178", "MS Serif,178", "Small Fonts,178",
      "MS Sans Serif,178", "MS Sans Serif,178", "MS Serif,178"
    },
    /* Baltic */
    { 1257, 775, "vga775.fon", "vgaf1257.fon", "vgas1257.fon",
      "coue1257.fon", "sere1257.fon", "smae1257.fon", "ssee1257.fon", "ssef1257.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
      "Fixedsys,186", "System,186",
      "Courier New,186", "MS Serif,186", "Small Fonts,186",
      "MS Sans Serif,186", "MS Sans Serif,186", "MS Serif,186",
      { "Arial Baltic,0", "Arial,186" },
      { "Courier New Baltic,0", "Courier New,186" },
      { "Times New Roman Baltic,0", "Times New Roman,186" }
    },
    /* Vietnamese */
    { 1258, 1258, "vga850.fon", "vgafix.fon", "vgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon", "sseriff.fon",
      "Tahoma","Times New Roman" /* FIXME unverified */
    },
    /* Thai */
    { 874, 874, "vga850.fon", "vgaf874.fon", "vgas874.fon",
      "coure.fon", "serife.fon", "smalle.fon", "ssee874.fon", "ssef874.fon",
      "Tahoma","Times New Roman" /* FIXME unverified */
    },
    /* Japanese */
    { 932, 932, "vga932.fon", "jvgafix.fon", "jvgasys.fon",
      "coure.fon", "serife.fon", "jsmalle.fon", "sserife.fon", "sseriff.fon",
      "MS UI Gothic","MS Serif"
    },
    /* Chinese Simplified */
    { 936, 936, "vga936.fon", "svgafix.fon", "svgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon", "sseriff.fon",
      "SimSun", "NSimSun"
    },
    /* Korean */
    { 949, 949, "vga949.fon", "hvgafix.fon", "hvgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon", "sseriff.fon",
      "Gulim",  "Batang"
    },
    /* Chinese Traditional */
    { 950, 950, "vga950.fon", "cvgafix.fon", "cvgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon", "sseriff.fon",
      "PMingLiU",  "MingLiU"
    }
};

static inline BOOL is_dbcs_ansi_cp(UINT ansi_cp)
{
    return ( ansi_cp == 932       /* CP932 for Japanese */
            || ansi_cp == 936     /* CP936 for Chinese Simplified */
            || ansi_cp == 949     /* CP949 for Korean */
            || ansi_cp == 950 );  /* CP950 for Chinese Traditional */
}

static CRITICAL_SECTION font_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &font_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": font_cs") }
};
static CRITICAL_SECTION font_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

#ifndef WINE_FONT_DIR
#define WINE_FONT_DIR "fonts"
#endif

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

static void get_fonts_data_dir_path( const WCHAR *file, WCHAR *path )
{
    if (GetEnvironmentVariableW( L"WINEDATADIR", path, MAX_PATH ))
        lstrcatW( path, L"\\" WINE_FONT_DIR "\\" );
    else if (GetEnvironmentVariableW( L"WINEBUILDDIR", path, MAX_PATH ))
        lstrcatW( path, L"\\fonts\\" );

    lstrcatW( path, file );
    if (path[5] == ':') memmove( path, path + 4, (lstrlenW(path) - 3) * sizeof(WCHAR) );
    else path[1] = '\\';  /* change \??\ to \\?\ */
}

static void get_fonts_win_dir_path( const WCHAR *file, WCHAR *path )
{
    GetWindowsDirectoryW( path, MAX_PATH );
    lstrcatW( path, L"\\fonts\\" );
    lstrcatW( path, file );
}

/* font substitutions */

struct gdi_font_subst
{
    struct list entry;
    int         from_charset;
    int         to_charset;
    WCHAR       names[1];
};

static struct list font_subst_list = LIST_INIT(font_subst_list);

static inline WCHAR *get_subst_to_name( struct gdi_font_subst *subst )
{
    return subst->names + lstrlenW( subst->names ) + 1;
}

static void dump_gdi_font_subst(void)
{
    struct gdi_font_subst *subst;

    LIST_FOR_EACH_ENTRY( subst, &font_subst_list, struct gdi_font_subst, entry )
    {
        if (subst->from_charset != -1 || subst->to_charset != -1)
	    TRACE("%s,%d -> %s,%d\n", debugstr_w(subst->names),
                  subst->from_charset, debugstr_w(get_subst_to_name(subst)), subst->to_charset);
	else
	    TRACE("%s -> %s\n", debugstr_w(subst->names), debugstr_w(get_subst_to_name(subst)));
    }
}

static const WCHAR *get_gdi_font_subst( const WCHAR *from_name, int from_charset, int *to_charset )
{
    struct gdi_font_subst *subst;

    LIST_FOR_EACH_ENTRY( subst, &font_subst_list, struct gdi_font_subst, entry )
    {
        if (!facename_compare( subst->names, from_name, -1 ) &&
           (subst->from_charset == from_charset || subst->from_charset == -1))
        {
            if (to_charset) *to_charset = subst->to_charset;
            return get_subst_to_name( subst );
        }
    }
    return NULL;
}

static BOOL add_gdi_font_subst( const WCHAR *from_name, int from_charset, const WCHAR *to_name, int to_charset )
{
    struct gdi_font_subst *subst;
    int len = lstrlenW( from_name ) + lstrlenW( to_name ) + 2;

    if (get_gdi_font_subst( from_name, from_charset, NULL )) return FALSE;  /* already exists */

    if (!(subst = HeapAlloc( GetProcessHeap(), 0,
                             offsetof( struct gdi_font_subst, names[len] ))))
        return FALSE;
    lstrcpyW( subst->names, from_name );
    lstrcpyW( get_subst_to_name(subst), to_name );
    subst->from_charset = from_charset;
    subst->to_charset = to_charset;
    list_add_tail( &font_subst_list, &subst->entry );
    return TRUE;
}

static void load_gdi_font_subst(void)
{
    HKEY hkey;
    DWORD i = 0, type, dlen, vlen;
    WCHAR value[64], data[64], *p;

    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes",
                     &hkey)) return;

    dlen = sizeof(data);
    vlen = ARRAY_SIZE(value);
    while (!RegEnumValueW( hkey, i++, value, &vlen, NULL, &type, (BYTE *)data, &dlen ))
    {
        int from_charset = -1, to_charset = -1;

        TRACE("Got %s=%s\n", debugstr_w(value), debugstr_w(data));
        if ((p = wcsrchr( value, ',' )) && p[1])
        {
            *p++ = 0;
            from_charset = wcstol( p, NULL, 10 );
        }
        if ((p = wcsrchr( data, ',' )) && p[1])
        {
            *p++ = 0;
            to_charset = wcstol( p, NULL, 10 );
        }

        /* Win 2000 doesn't allow mapping between different charsets
           or mapping of DEFAULT_CHARSET */
        if ((!from_charset || to_charset == from_charset) && to_charset != DEFAULT_CHARSET)
            add_gdi_font_subst( value, from_charset, data, to_charset );

        /* reset dlen and vlen */
        dlen = sizeof(data);
        vlen = ARRAY_SIZE(value);
    }
    RegCloseKey( hkey );
}

/* font families */

static int family_namecmp( const WCHAR *str1, const WCHAR *str2 )
{
    int prio1, prio2, vert1 = (str1[0] == '@' ? 1 : 0), vert2 = (str2[0] == '@' ? 1 : 0);

    if (!facename_compare( str1, ff_swiss_default, LF_FACESIZE - 1 )) prio1 = 0;
    else if (!facename_compare( str1, ff_modern_default, LF_FACESIZE - 1 )) prio1 = 1;
    else if (!facename_compare( str1, ff_roman_default, LF_FACESIZE - 1 )) prio1 = 2;
    else prio1 = 3;

    if (!facename_compare( str2, ff_swiss_default, LF_FACESIZE - 1 )) prio2 = 0;
    else if (!facename_compare( str2, ff_modern_default, LF_FACESIZE - 1 )) prio2 = 1;
    else if (!facename_compare( str2, ff_roman_default, LF_FACESIZE - 1 )) prio2 = 2;
    else prio2 = 3;

    if (prio1 != prio2) return prio1 - prio2;
    if (vert1 != vert2) return vert1 - vert2;
    return facename_compare( str1 + vert1, str2 + vert2, LF_FACESIZE - 1 );
}

static int family_name_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct gdi_font_family *family = WINE_RB_ENTRY_VALUE( entry, const struct gdi_font_family, name_entry );
    return family_namecmp( (const WCHAR *)key, family->family_name );
}

static int family_second_name_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct gdi_font_family *family = WINE_RB_ENTRY_VALUE( entry, const struct gdi_font_family, second_name_entry );
    return family_namecmp( (const WCHAR *)key, family->second_name );
}

static int face_full_name_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct gdi_font_face *face = WINE_RB_ENTRY_VALUE( entry, const struct gdi_font_face, full_name_entry );
    return facename_compare( (const WCHAR *)key, face->full_name, LF_FULLFACESIZE - 1 );
}

static struct wine_rb_tree family_name_tree = { family_name_compare };
static struct wine_rb_tree family_second_name_tree = { family_second_name_compare };
static struct wine_rb_tree face_full_name_tree = { face_full_name_compare };

static int face_is_in_full_name_tree( const struct gdi_font_face *face )
{
    return face->full_name_entry.parent || face_full_name_tree.root == &face->full_name_entry;
}

static struct gdi_font_family *create_family( const WCHAR *name, const WCHAR *second_name )
{
    struct gdi_font_family *family = HeapAlloc( GetProcessHeap(), 0, sizeof(*family) );

    family->refcount = 1;
    lstrcpynW( family->family_name, name, LF_FACESIZE );
    if (second_name && second_name[0] && wcsicmp( name, second_name ))
    {
        lstrcpynW( family->second_name, second_name, LF_FACESIZE );
        add_gdi_font_subst( second_name, -1, name, -1 );
    }
    else family->second_name[0] = 0;
    list_init( &family->faces );
    family->replacement = NULL;
    wine_rb_put( &family_name_tree, family->family_name, &family->name_entry );
    if (family->second_name[0]) wine_rb_put( &family_second_name_tree, family->second_name, &family->second_name_entry );
    return family;
}

static void release_family( struct gdi_font_family *family )
{
    if (--family->refcount) return;
    assert( list_empty( &family->faces ));
    wine_rb_remove( &family_name_tree, &family->name_entry );
    if (family->second_name[0]) wine_rb_remove( &family_second_name_tree, &family->second_name_entry );
    if (family->replacement) release_family( family->replacement );
    HeapFree( GetProcessHeap(), 0, family );
}

static struct gdi_font_family *find_family_from_name( const WCHAR *name )
{
    struct wine_rb_entry *entry;
    if (!(entry = wine_rb_get( &family_name_tree, name ))) return NULL;
    return WINE_RB_ENTRY_VALUE( entry, struct gdi_font_family, name_entry );
}

static struct gdi_font_family *find_family_from_any_name( const WCHAR *name )
{
    struct wine_rb_entry *entry;
    struct gdi_font_family *family;
    if ((family = find_family_from_name( name ))) return family;
    if (!(entry = wine_rb_get( &family_second_name_tree, name ))) return NULL;
    return WINE_RB_ENTRY_VALUE( entry, struct gdi_font_family, second_name_entry );
}

static struct gdi_font_face *find_face_from_full_name( const WCHAR *full_name )
{
    struct wine_rb_entry *entry;
    if (!(entry = wine_rb_get( &face_full_name_tree, full_name ))) return NULL;
    return WINE_RB_ENTRY_VALUE( entry, struct gdi_font_face, full_name_entry );
}

static const struct list *get_family_face_list( const struct gdi_font_family *family )
{
    return family->replacement ? &family->replacement->faces : &family->faces;
}

static struct gdi_font_face *family_find_face_from_filename( struct gdi_font_family *family, const WCHAR *file_name )
{
    struct gdi_font_face *face;
    const WCHAR *file;
    LIST_FOR_EACH_ENTRY( face, get_family_face_list(family), struct gdi_font_face, entry )
    {
        if (!face->file) continue;
        file = wcsrchr(face->file, '\\');
        if (!file) file = face->file;
        else file++;
        if (wcsicmp( file, file_name )) continue;
        face->refcount++;
        return face;
    }
    return NULL;
}

static struct gdi_font_face *find_face_from_filename( const WCHAR *file_name, const WCHAR *family_name )
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;

    TRACE( "looking for file %s name %s\n", debugstr_w(file_name), debugstr_w(family_name) );

    if (!family_name)
    {
        WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
            if ((face = family_find_face_from_filename( family, file_name ))) return face;
        return NULL;
    }

    if (!(family = find_family_from_name( family_name ))) return NULL;
    return family_find_face_from_filename( family, file_name );
}

static BOOL add_family_replacement( const WCHAR *new_name, const WCHAR *replace )
{
    struct gdi_font_family *new_family, *family;
    struct gdi_font_face *face;
    WCHAR new_name_vert[LF_FACESIZE], replace_vert[LF_FACESIZE];

    if (!(family = find_family_from_any_name( replace )))
    {
        TRACE( "%s is not available. Skip this replacement.\n", debugstr_w(replace) );
        return FALSE;
    }

    if (!(new_family = create_family( new_name, NULL ))) return FALSE;
    new_family->replacement = family;
    family->refcount++;
    TRACE( "mapping %s to %s\n", debugstr_w(replace), debugstr_w(new_name) );

    /* also add replacement for vertical font if necessary */
    if (replace[0] == '@') return TRUE;
    if (list_empty( &family->faces )) return TRUE;
    face = LIST_ENTRY( list_head(&family->faces), struct gdi_font_face, entry );
    if (!(face->fs.fsCsb[0] & FS_DBCS_MASK)) return TRUE;

    new_name_vert[0] = '@';
    lstrcpynW( new_name_vert + 1, new_name, LF_FACESIZE - 1 );
    if (find_family_from_any_name( new_name_vert )) return TRUE;  /* already exists */

    replace_vert[0] = '@';
    lstrcpynW( replace_vert + 1, replace, LF_FACESIZE - 1 );
    add_family_replacement( new_name_vert, replace_vert );
    return TRUE;
}

/*
 * The replacement list is a way to map an entire font
 * family onto another family.  For example adding
 *
 * [HKCU\Software\Wine\Fonts\Replacements]
 * "Wingdings"="Winedings"
 *
 * would enumerate the Winedings font both as Winedings and
 * Wingdings.  However if a real Wingdings font is present the
 * replacement does not take place.
 */
static void load_gdi_font_replacements(void)
{
    HKEY hkey;
    DWORD i = 0, type, dlen, vlen;
    WCHAR value[LF_FACESIZE], data[1024];

    /* @@ Wine registry key: HKCU\Software\Wine\Fonts\Replacements */
    if (RegOpenKeyW( wine_fonts_key, L"Replacements", &hkey )) return;

    dlen = sizeof(data);
    vlen = ARRAY_SIZE(value);
    while (!RegEnumValueW( hkey, i++, value, &vlen, NULL, &type, (BYTE *)data, &dlen ))
    {
        /* "NewName"="Oldname" */
        if (!find_family_from_any_name( value ))
        {
            if (type == REG_MULTI_SZ)
            {
                WCHAR *replace = data;
                while (*replace)
                {
                    if (add_family_replacement( value, replace )) break;
                    replace += lstrlenW(replace) + 1;
                }
            }
            else if (type == REG_SZ) add_family_replacement( value, data );
        }
        else TRACE("%s is available. Skip this replacement.\n", debugstr_w(value));

        /* reset dlen and vlen */
        dlen = sizeof(data);
        vlen = ARRAY_SIZE(value);
    }
    RegCloseKey( hkey );
}

static void dump_gdi_font_list(void)
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;

    WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
    {
        TRACE( "Family: %s\n", debugstr_w(family->family_name) );
        LIST_FOR_EACH_ENTRY( face, &family->faces, struct gdi_font_face, entry )
        {
            TRACE( "\t%s\t%s\t%08x", debugstr_w(face->style_name), debugstr_w(face->full_name),
                   face->fs.fsCsb[0] );
            if (!face->scalable) TRACE(" %d", face->size.height );
            TRACE("\n");
	}
    }
}

static BOOL enum_fallbacks( DWORD pitch_and_family, int index, WCHAR buffer[LF_FACESIZE] )
{
    if (index < 3)
    {
        const WCHAR * const *defaults;

        if ((pitch_and_family & FIXED_PITCH) || (pitch_and_family & 0xf0) == FF_MODERN)
            defaults = default_fixed_list;
        else if ((pitch_and_family & 0xf0) == FF_ROMAN)
            defaults = default_serif_list;
        else
            defaults = default_sans_list;
        lstrcpynW( buffer, defaults[index], LF_FACESIZE );
        return TRUE;
    }
    return font_funcs->enum_family_fallbacks( pitch_and_family, index - 3, buffer );
}

static void set_default_family( DWORD pitch_and_family, WCHAR *default_name )
{
    struct wine_rb_entry *entry;
    WCHAR name[LF_FACESIZE];
    int i = 0;

    while (enum_fallbacks( pitch_and_family, i++, name ))
    {
        if (!(entry = wine_rb_get( &family_name_tree, name ))) continue;
        wine_rb_remove( &family_name_tree, entry );
        lstrcpynW( default_name, name, LF_FACESIZE - 1 );
        wine_rb_put( &family_name_tree, name, entry );
        return;
    }
}

static void reorder_font_list(void)
{
    set_default_family( FF_ROMAN, ff_roman_default );
    set_default_family( FF_MODERN, ff_modern_default );
    set_default_family( FF_SWISS, ff_swiss_default );
}

static void release_face( struct gdi_font_face *face )
{
    if (--face->refcount) return;
    if (face->family)
    {
        if (face->flags & ADDFONT_ADD_TO_CACHE) remove_face_from_cache( face );
        list_remove( &face->entry );
        release_family( face->family );
    }
    if (face_is_in_full_name_tree( face )) wine_rb_remove( &face_full_name_tree, &face->full_name_entry );
    HeapFree( GetProcessHeap(), 0, face->file );
    HeapFree( GetProcessHeap(), 0, face->style_name );
    HeapFree( GetProcessHeap(), 0, face->full_name );
    HeapFree( GetProcessHeap(), 0, face->cached_enum_data );
    HeapFree( GetProcessHeap(), 0, face );
}

static int remove_font( const WCHAR *file, DWORD flags )
{
    struct gdi_font_family *family, *family_next;
    struct gdi_font_face *face, *face_next;
    int count = 0;

    EnterCriticalSection( &font_cs );
    WINE_RB_FOR_EACH_ENTRY_DESTRUCTOR( family, family_next, &family_name_tree, struct gdi_font_family, name_entry )
    {
        family->refcount++;
        LIST_FOR_EACH_ENTRY_SAFE( face, face_next, &family->faces, struct gdi_font_face, entry )
        {
            if (!face->file) continue;
            if (LOWORD(face->flags) != LOWORD(flags)) continue;
            if (!wcsicmp( face->file, file ))
            {
                TRACE( "removing matching face %s refcount %d\n", debugstr_w(face->file), face->refcount );
                release_face( face );
                count++;
            }
	}
        release_family( family );
    }
    LeaveCriticalSection( &font_cs );
    return count;
}

static inline BOOL faces_equal( const struct gdi_font_face *f1, const struct gdi_font_face *f2 )
{
    if (facename_compare( f1->full_name, f2->full_name, -1 )) return FALSE;
    if (f1->scalable) return TRUE;
    if (f1->size.y_ppem != f2->size.y_ppem) return FALSE;
    return !memcmp( &f1->fs, &f2->fs, sizeof(f1->fs) );
}

static inline int style_order( const struct gdi_font_face *face )
{
    switch (face->ntmFlags & (NTM_REGULAR | NTM_BOLD | NTM_ITALIC))
    {
    case NTM_REGULAR:
        return 0;
    case NTM_BOLD:
        return 1;
    case NTM_ITALIC:
        return 2;
    case NTM_BOLD | NTM_ITALIC:
        return 3;
    default:
        WARN( "Don't know how to order face %s with flags 0x%08x\n",
              debugstr_w(face->full_name), face->ntmFlags );
        return 9999;
    }
}

static BOOL insert_face_in_family_list( struct gdi_font_face *face, struct gdi_font_family *family )
{
    struct gdi_font_face *cursor;

    LIST_FOR_EACH_ENTRY( cursor, &family->faces, struct gdi_font_face, entry )
    {
        if (faces_equal( face, cursor ))
        {
            TRACE( "Already loaded face %s in family %s, original version %x, new version %x\n",
                   debugstr_w(face->full_name), debugstr_w(family->family_name),
                   cursor->version, face->version );

            if (face->file && cursor->file && !wcsicmp( face->file, cursor->file ))
            {
                cursor->refcount++;
                TRACE("Font %s already in list, refcount now %d\n",
                      debugstr_w(face->file), cursor->refcount);
                return FALSE;
            }
            if (face->version <= cursor->version)
            {
                TRACE("Original font %s is newer so skipping %s\n",
                      debugstr_w(cursor->file), debugstr_w(face->file));
                return FALSE;
            }
            else
            {
                TRACE("Replacing original %s with %s\n",
                      debugstr_w(cursor->file), debugstr_w(face->file));
                list_add_before( &cursor->entry, &face->entry );
                face->family = family;
                family->refcount++;
                face->refcount++;
                if (face_is_in_full_name_tree( cursor ))
                {
                    wine_rb_replace( &face_full_name_tree, &cursor->full_name_entry, &face->full_name_entry );
                    memset( &cursor->full_name_entry, 0, sizeof(cursor->full_name_entry) );
                }
                release_face( cursor );
                return TRUE;
            }
        }
        if (style_order( face ) < style_order( cursor )) break;
    }

    TRACE( "Adding face %s in family %s from %s\n", debugstr_w(face->full_name),
           debugstr_w(family->family_name), debugstr_w(face->file) );
    list_add_before( &cursor->entry, &face->entry );
    if (face->scalable) wine_rb_put( &face_full_name_tree, face->full_name, &face->full_name_entry );
    face->family = family;
    family->refcount++;
    face->refcount++;
    return TRUE;
}

static struct gdi_font_face *create_face( struct gdi_font_family *family, const WCHAR *style,
                                          const WCHAR *fullname, const WCHAR *file,
                                          void *data_ptr, SIZE_T data_size, UINT index, FONTSIGNATURE fs,
                                          DWORD ntmflags, DWORD version, DWORD flags,
                                          const struct bitmap_font_size *size )
{
    struct gdi_font_face *face = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*face) );

    face->refcount   = 1;
    face->style_name = strdupW( style );
    face->full_name  = strdupW( fullname );
    face->face_index = index;
    face->fs         = fs;
    face->ntmFlags   = ntmflags;
    face->version    = version;
    face->flags      = flags;
    face->data_ptr   = data_ptr;
    face->data_size  = data_size;
    if (file) face->file = strdupW( file );
    if (size) face->size = *size;
    else face->scalable = TRUE;
    if (insert_face_in_family_list( face, family )) return face;
    release_face( face );
    return NULL;
}

static int CDECL add_gdi_face( const WCHAR *family_name, const WCHAR *second_name,
                               const WCHAR *style, const WCHAR *fullname, const WCHAR *file,
                               void *data_ptr, SIZE_T data_size, UINT index, FONTSIGNATURE fs,
                               DWORD ntmflags, DWORD version, DWORD flags,
                               const struct bitmap_font_size *size )
{
    struct gdi_font_face *face;
    struct gdi_font_family *family;
    int ret = 0;

    if ((family = find_family_from_name( family_name ))) family->refcount++;
    else if (!(family = create_family( family_name, second_name ))) return ret;

    if ((face = create_face( family, style, fullname, file, data_ptr, data_size,
                             index, fs, ntmflags, version, flags, size )))
    {
        if (flags & ADDFONT_ADD_TO_CACHE) add_face_to_cache( face );
        release_face( face );
    }
    release_family( family );
    ret++;

    if (fs.fsCsb[0] & FS_DBCS_MASK)
    {
        WCHAR vert_family[LF_FACESIZE], vert_second[LF_FACESIZE], vert_full[LF_FULLFACESIZE];

        vert_family[0] = '@';
        lstrcpynW( vert_family + 1, family_name, LF_FACESIZE - 1 );

        if (second_name && second_name[0])
        {
            vert_second[0] = '@';
            lstrcpynW( vert_second + 1, second_name, LF_FACESIZE - 1 );
        }
        else vert_second[0] = 0;

        if (fullname)
        {
            vert_full[0] = '@';
            lstrcpynW( vert_full + 1, fullname, LF_FULLFACESIZE - 1 );
            fullname = vert_full;
        }

        if ((family = find_family_from_name( vert_family ))) family->refcount++;
        else if (!(family = create_family( vert_family, vert_second ))) return ret;

        if ((face = create_face( family, style, fullname, file, data_ptr, data_size,
                                 index, fs, ntmflags, version, flags | ADDFONT_VERTICAL_FONT, size )))
        {
            if (flags & ADDFONT_ADD_TO_CACHE) add_face_to_cache( face );
            release_face( face );
        }
        release_family( family );
        ret++;
    }
    return ret;
}

/* font cache */

struct cached_face
{
    DWORD                   index;
    DWORD                   flags;
    DWORD                   ntmflags;
    DWORD                   version;
    struct bitmap_font_size size;
    FONTSIGNATURE           fs;
    WCHAR                   full_name[1];
    /* WCHAR                file_name[]; */
};

static void load_face_from_cache( HKEY hkey_family, struct gdi_font_family *family,
                                  void *buffer, DWORD buffer_size, BOOL scalable )
{
    DWORD type, size, needed, index = 0;
    struct gdi_font_face *face;
    HKEY hkey_strike;
    WCHAR name[256];
    struct cached_face *cached = (struct cached_face *)buffer;

    size = sizeof(name);
    needed = buffer_size - sizeof(DWORD);
    while (!RegEnumValueW( hkey_family, index++, name, &size, NULL, &type, buffer, &needed ))
    {
        if (type == REG_BINARY && needed > sizeof(*cached))
        {
            ((DWORD *)buffer)[needed / sizeof(DWORD)] = 0;
            if ((face = create_face( family, name, cached->full_name,
                                     cached->full_name + lstrlenW(cached->full_name) + 1,
                                     NULL, 0, cached->index, cached->fs, cached->ntmflags, cached->version,
                                     cached->flags, scalable ? NULL : &cached->size )))
            {
                if (!scalable)
                    TRACE("Adding bitmap size h %d w %d size %d x_ppem %d y_ppem %d\n",
                          face->size.height, face->size.width, face->size.size >> 6,
                          face->size.x_ppem >> 6, face->size.y_ppem >> 6);

                TRACE("fsCsb = %08x %08x/%08x %08x %08x %08x\n",
                      face->fs.fsCsb[0], face->fs.fsCsb[1],
                      face->fs.fsUsb[0], face->fs.fsUsb[1],
                      face->fs.fsUsb[2], face->fs.fsUsb[3]);

                release_face( face );
            }
        }
        size = sizeof(name);
        needed = buffer_size - sizeof(DWORD);
    }

    /* load bitmap strikes */

    index = 0;
    needed = buffer_size;
    while (!RegEnumKeyExW( hkey_family, index++, buffer, &needed, NULL, NULL, NULL, NULL ))
    {
        if (!RegOpenKeyExW( hkey_family, buffer, 0, KEY_ALL_ACCESS, &hkey_strike ))
        {
            load_face_from_cache( hkey_strike, family, buffer, buffer_size, FALSE );
            RegCloseKey( hkey_strike );
        }
        needed = buffer_size;
    }
}

static void load_font_list_from_cache(void)
{
    DWORD size, family_index = 0;
    struct gdi_font_family *family;
    HKEY hkey_family;
    WCHAR buffer[4096], second_name[LF_FACESIZE];

    size = sizeof(buffer);
    while (!RegEnumKeyExW( wine_fonts_cache_key, family_index++, buffer, &size, NULL, NULL, NULL, NULL ))
    {
        RegOpenKeyExW( wine_fonts_cache_key, buffer, 0, KEY_ALL_ACCESS, &hkey_family );
        TRACE("opened family key %s\n", debugstr_w(buffer));
        size = sizeof(second_name);
        if (RegQueryValueExW( hkey_family, NULL, NULL, NULL, (BYTE *)second_name, &size ))
            second_name[0] = 0;

        family = create_family( buffer, second_name );

        load_face_from_cache( hkey_family, family, buffer, sizeof(buffer), TRUE );

        RegCloseKey( hkey_family );
        release_family( family );
        size = sizeof(buffer);
    }
}

static void add_face_to_cache( struct gdi_font_face *face )
{
    HKEY hkey_family, hkey_face;
    DWORD len, buffer[1024];
    struct cached_face *cached = (struct cached_face *)buffer;

    if (RegCreateKeyExW( wine_fonts_cache_key, face->family->family_name, 0, NULL, REG_OPTION_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &hkey_family, NULL ))
        return;

    if (face->family->second_name[0])
        RegSetValueExW( hkey_family, NULL, 0, REG_SZ, (BYTE *)face->family->second_name,
                        (lstrlenW( face->family->second_name ) + 1) * sizeof(WCHAR) );

    if (!face->scalable)
    {
        WCHAR name[10];

        swprintf( name, ARRAY_SIZE(name), L"%d", face->size.y_ppem );
        RegCreateKeyExW( hkey_family, name, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
                         NULL, &hkey_face, NULL);
    }
    else hkey_face = hkey_family;

    memset( cached, 0, sizeof(*cached) );
    cached->index = face->face_index;
    cached->flags = face->flags;
    cached->ntmflags = face->ntmFlags;
    cached->version = face->version;
    cached->fs = face->fs;
    if (!face->scalable) cached->size = face->size;
    lstrcpyW( cached->full_name, face->full_name );
    len = lstrlenW( face->full_name ) + 1;
    lstrcpyW( cached->full_name + len, face->file );
    len += lstrlenW( face->file ) + 1;

    RegSetValueExW( hkey_face, face->style_name, 0, REG_BINARY, (BYTE *)cached,
                    offsetof( struct cached_face, full_name[len] ));

    if (hkey_face != hkey_family) RegCloseKey( hkey_face );
    RegCloseKey( hkey_family );
}

static void remove_face_from_cache( struct gdi_font_face *face )
{
    HKEY hkey_family;

    if (RegOpenKeyExW( wine_fonts_cache_key, face->family->family_name, 0, KEY_ALL_ACCESS, &hkey_family ))
        return;

    if (!face->scalable)
    {
        WCHAR name[10];
        swprintf( name, ARRAY_SIZE(name), L"%d", face->size.y_ppem );
        RegDeleteKeyW( hkey_family, name );
    }
    else RegDeleteValueW( hkey_family, face->style_name );

    RegCloseKey( hkey_family );
}

/* font links */

struct gdi_font_link
{
    struct list            entry;
    struct list            links;
    WCHAR                  name[LF_FACESIZE];
    FONTSIGNATURE          fs;
};

struct gdi_font_link_entry
{
    struct list            entry;
    FONTSIGNATURE          fs;
    WCHAR                  family_name[LF_FACESIZE];
};

static struct list font_links = LIST_INIT(font_links);

static struct gdi_font_link *find_gdi_font_link( const WCHAR *name )
{
    struct gdi_font_link *link;

    LIST_FOR_EACH_ENTRY( link, &font_links, struct gdi_font_link, entry )
        if (!facename_compare( link->name, name, LF_FACESIZE - 1 )) return link;
    return NULL;
}

static struct gdi_font_family *find_family_from_font_links( const WCHAR *name, const WCHAR *subst,
                                                            FONTSIGNATURE fs )
{
    struct gdi_font_link *link;
    struct gdi_font_link_entry *entry;
    struct gdi_font_family *family;

    LIST_FOR_EACH_ENTRY( link, &font_links, struct gdi_font_link, entry )
    {
        if (!facename_compare( link->name, name, LF_FACESIZE - 1 ) ||
            (subst && !facename_compare( link->name, subst, LF_FACESIZE - 1 )))
        {
            TRACE("found entry in system list\n");
            LIST_FOR_EACH_ENTRY( entry, &link->links, struct gdi_font_link_entry, entry )
            {
                const struct gdi_font_link *links;

                family = find_family_from_name( entry->family_name );
                if (!fs.fsCsb[0]) return family;
                if (fs.fsCsb[0] & entry->fs.fsCsb[0]) return family;
                if ((links = find_gdi_font_link( family->family_name )) && fs.fsCsb[0] & links->fs.fsCsb[0])
                    return family;
            }
        }
    }
    return NULL;
}

static struct gdi_font_link *add_gdi_font_link( const WCHAR *name )
{
    struct gdi_font_link *link = find_gdi_font_link( name );

    if (link) return link;
    if ((link = HeapAlloc( GetProcessHeap(), 0, sizeof(*link) )))
    {
        lstrcpynW( link->name, name, LF_FACESIZE );
        memset( &link->fs, 0, sizeof(link->fs) );
        list_init( &link->links );
        list_add_tail( &font_links, &link->entry );
    }
    return link;
}

static void add_gdi_font_link_entry( struct gdi_font_link *link, const WCHAR *family_name, FONTSIGNATURE fs )
{
    struct gdi_font_link_entry *entry;

    entry = HeapAlloc( GetProcessHeap(), 0, sizeof(*entry) );
    lstrcpynW( entry->family_name, family_name, LF_FACESIZE );
    entry->fs = fs;
    link->fs.fsCsb[0] |= fs.fsCsb[0];
    link->fs.fsCsb[1] |= fs.fsCsb[1];
    list_add_tail( &link->links, &entry->entry );
}

static const WCHAR * const font_links_list[] =
{
    L"Lucida Sans Unicode",
    L"Microsoft Sans Serif",
    L"Tahoma"
};

static const struct font_links_defaults_list
{
    /* Keyed off substitution for "MS Shell Dlg" */
    const WCHAR *shelldlg;
    /* Maximum of four substitutes, plus terminating NULL pointer */
    const WCHAR *substitutes[5];
} font_links_defaults_list[] =
{
    /* Non East-Asian */
    { L"Tahoma", /* FIXME unverified ordering */
      { L"MS UI Gothic", L"SimSun", L"Gulim", L"PMingLiU", NULL }
    },
    /* Below lists are courtesy of
     * http://blogs.msdn.com/michkap/archive/2005/06/18/430507.aspx
     */
    /* Japanese */
    { L"MS UI Gothic",
      { L"MS UI Gothic", L"PMingLiU", L"SimSun", L"Gulim", NULL }
    },
    /* Chinese Simplified */
    { L"SimSun",
      { L"SimSun", L"PMingLiU", L"MS UI Gothic", L"Batang", NULL }
    },
    /* Korean */
    { L"Gulim",
      { L"Gulim", L"PMingLiU", L"MS UI Gothic", L"SimSun", NULL }
    },
    /* Chinese Traditional */
    { L"PMingLiU",
      { L"PMingLiU", L"SimSun", L"MS UI Gothic", L"Batang", NULL }
    }
};

static void populate_system_links( const WCHAR *name, const WCHAR * const *values )
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;
    struct gdi_font_link *font_link;
    const WCHAR *file, *value;

    /* Don't store fonts that are only substitutes for other fonts */
    if (get_gdi_font_subst( name, -1, NULL ))
    {
        TRACE( "%s: Internal SystemLink entry for substituted font, ignoring\n", debugstr_w(name) );
        return;
    }
    font_link = add_gdi_font_link( name );
    for ( ; *values; values++)
    {
        if  (!facename_compare( name, *values, -1 )) continue;
        if (!(value = get_gdi_font_subst( *values, -1, NULL ))) value = *values;
        if (!(family = find_family_from_name( value ))) continue;
        /* use first extant filename for this Family */
        LIST_FOR_EACH_ENTRY( face, get_family_face_list(family), struct gdi_font_face, entry )
        {
            if (!face->file) continue;
            file = wcsrchr(face->file, '\\');
            if (!file) file = face->file;
            else file++;
            if ((face = find_face_from_filename( file, value )))
            {
                add_gdi_font_link_entry( font_link, face->family->family_name, face->fs );
                TRACE( "added internal SystemLink for %s to %s in %s\n",
                       debugstr_w(name), debugstr_w(value), debugstr_w(file) );
            }
            else TRACE( "Unable to find file %s face name %s\n", debugstr_w(file), debugstr_w(value) );
            break;
        }
    }
}

static void load_system_links(void)
{
    HKEY hkey;
    DWORD i, j;
    const WCHAR *shelldlg_name;
    struct gdi_font_link *font_link, *system_font_link;
    struct gdi_font_face *face;

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink", &hkey ))
    {
        WCHAR value[MAX_PATH], data[1024];
        DWORD type, val_len, data_len;
        WCHAR *entry, *next;

        val_len = ARRAY_SIZE(value);
        data_len = sizeof(data);
        i = 0;
        while (!RegEnumValueW( hkey, i++, value, &val_len, NULL, &type, (LPBYTE)data, &data_len))
        {
            /* Don't store fonts that are only substitutes for other fonts */
            if (!get_gdi_font_subst( value, -1, NULL ))
            {
                font_link = add_gdi_font_link( value );
                for (entry = data; (char *)entry < (char *)data + data_len && *entry; entry = next)
                {
                    const WCHAR *family_name = NULL;
                    WCHAR *p;

                    TRACE("%s: %s\n", debugstr_w(value), debugstr_w(entry));

                    next = entry + lstrlenW(entry) + 1;
                    if ((p = wcschr( entry, ',' )))
                    {
                        *p++ = 0;
                        while (iswspace(*p)) p++;
                        if (!(family_name = get_gdi_font_subst( p, -1, NULL ))) family_name = p;
                    }
                    if ((face = find_face_from_filename( entry, family_name )))
                    {
                        add_gdi_font_link_entry( font_link, face->family->family_name, face->fs );
                        TRACE("Adding file %s index %u\n", debugstr_w(face->file), face->face_index);
                    }
                    else TRACE( "Unable to find file %s family %s\n",
                                debugstr_w(entry), debugstr_w(family_name) );
                }
            }
            else TRACE("%s: SystemLink entry for substituted font, ignoring\n", debugstr_w(value));

            val_len = ARRAY_SIZE(value);
            data_len = sizeof(data);
        }
        RegCloseKey( hkey );
    }

    if ((shelldlg_name = get_gdi_font_subst( L"MS Shell Dlg", -1, NULL )))
    {
        for (i = 0; i < ARRAY_SIZE(font_links_defaults_list); i++)
        {
            const WCHAR *subst = get_gdi_font_subst( font_links_defaults_list[i].shelldlg, -1, NULL );

            if ((!facename_compare( font_links_defaults_list[i].shelldlg, shelldlg_name, -1 ) ||
                 (subst && !facename_compare( subst, shelldlg_name, -1 ))))
            {
                for (j = 0; j < ARRAY_SIZE(font_links_list); j++)
                    populate_system_links( font_links_list[j], font_links_defaults_list[i].substitutes );
                if (!facename_compare(shelldlg_name, font_links_defaults_list[i].substitutes[0], -1))
                    populate_system_links( shelldlg_name, font_links_defaults_list[i].substitutes );
            }
        }
    }
    else WARN( "could not find FontSubstitute for MS Shell Dlg\n" );

    /* Explicitly add an entry for the system font, this links to Tahoma and any links
       that Tahoma has */

    system_font_link = add_gdi_font_link( L"System" );
    if ((face = find_face_from_filename( L"tahoma.ttf", L"Tahoma" )))
    {
        add_gdi_font_link_entry( system_font_link, face->family->family_name, face->fs );
        TRACE("Found Tahoma in %s index %u\n", debugstr_w(face->file), face->face_index);
    }
    if ((font_link = find_gdi_font_link( L"Tahoma" )))
    {
        struct gdi_font_link_entry *entry;
        LIST_FOR_EACH_ENTRY( entry, &font_link->links, struct gdi_font_link_entry, entry )
            add_gdi_font_link_entry( system_font_link, entry->family_name, entry->fs );
    }
}

/* font matching */

static BOOL can_select_face( const struct gdi_font_face *face, FONTSIGNATURE fs, BOOL can_use_bitmap )
{
    struct gdi_font_link *font_link;

    if (!face->scalable && !can_use_bitmap) return FALSE;
    if (!fs.fsCsb[0]) return TRUE;
    if (fs.fsCsb[0] & face->fs.fsCsb[0]) return TRUE;
    if (!(font_link = find_gdi_font_link( face->family->family_name ))) return FALSE;
    if (fs.fsCsb[0] & font_link->fs.fsCsb[0]) return TRUE;
    return FALSE;
}

static struct gdi_font_face *find_best_matching_face( const struct gdi_font_family *family,
                                                      const LOGFONTW *lf, FONTSIGNATURE fs,
                                                      BOOL can_use_bitmap )
{
    struct gdi_font_face *face = NULL, *best = NULL, *best_bitmap = NULL;
    unsigned int best_score = 4;
    int best_diff = 0;
    int it = !!lf->lfItalic;
    int bd = lf->lfWeight > 550;
    int height = lf->lfHeight;

    LIST_FOR_EACH_ENTRY( face, get_family_face_list(family), struct gdi_font_face, entry )
    {
        int italic = !!(face->ntmFlags & NTM_ITALIC);
        int bold = !!(face->ntmFlags & NTM_BOLD);
        int score = (italic ^ it) + (bold ^ bd);

        if (!can_select_face( face, fs, can_use_bitmap )) continue;
        if (score > best_score) continue;
        TRACE( "(it=%d, bd=%d) is selected for (it=%d, bd=%d)\n", italic, bold, it, bd );
        best_score = score;
        best = face;
        if (best->scalable && best_score == 0) break;
        if (!best->scalable)
        {
            int diff;
            if (height > 0)
                diff = height - (signed int)best->size.height;
            else
                diff = -height - ((signed int)best->size.height - best->size.internal_leading);
            if (!best_bitmap ||
                (best_diff > 0 && diff >= 0 && diff < best_diff) ||
                (best_diff < 0 && diff > best_diff))
            {
                TRACE( "%d is better for %d diff was %d\n", best->size.height, height, best_diff );
                best_diff = diff;
                best_bitmap = best;
                if (best_score == 0 && best_diff == 0) break;
            }
        }
    }
    if (!best) return NULL;
    return best->scalable ? best : best_bitmap;
}

static struct gdi_font_face *find_matching_face_by_name( const WCHAR *name, const WCHAR *subst,
                                                         const LOGFONTW *lf, FONTSIGNATURE fs,
                                                         BOOL can_use_bitmap )
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;

    family = find_family_from_any_name( name );
    if (family && (face = find_best_matching_face( family, lf, fs, can_use_bitmap ))) return face;
    if (subst)
    {
        family = find_family_from_any_name( subst );
        if (family && (face = find_best_matching_face( family, lf, fs, can_use_bitmap ))) return face;
    }

    /* search by full face name */
    WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
        LIST_FOR_EACH_ENTRY( face, get_family_face_list(family), struct gdi_font_face, entry )
            if (!facename_compare( face->full_name, name, LF_FACESIZE - 1 ) &&
                can_select_face( face, fs, can_use_bitmap ))
                return face;

    if ((family = find_family_from_font_links( name, subst, fs )))
    {
        if ((face = find_best_matching_face( family, lf, fs, can_use_bitmap ))) return face;
    }
    return NULL;
}

static struct gdi_font_face *find_any_face( const LOGFONTW *lf, FONTSIGNATURE fs,
                                            BOOL can_use_bitmap, BOOL want_vertical )
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;
    WCHAR name[LF_FACESIZE + 1];
    int i = 0;

    /* first try the family fallbacks */
    while (enum_fallbacks( lf->lfPitchAndFamily, i++, name ))
    {
        if (want_vertical)
        {
            memmove(name + 1, name, min(lstrlenW(name), LF_FACESIZE));
            name[0] = '@';
        }

        if (!(family = find_family_from_any_name(name))) continue;
        if ((face = find_best_matching_face( family, lf, fs, FALSE ))) return face;
    }
    /* otherwise try only scalable */
    WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
    {
        if ((family->family_name[0] == '@') == !want_vertical) continue;
        if ((face = find_best_matching_face( family, lf, fs, FALSE ))) return face;
    }
    if (!can_use_bitmap) return NULL;
    /* then also bitmap fonts */
    WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
    {
        if ((family->family_name[0] == '@') == !want_vertical) continue;
        if ((face = find_best_matching_face( family, lf, fs, can_use_bitmap ))) return face;
    }
    return NULL;
}

static struct gdi_font_face *find_matching_face( const LOGFONTW *lf, CHARSETINFO *csi, BOOL can_use_bitmap,
                                                 const WCHAR **orig_name )
{
    BOOL want_vertical = (lf->lfFaceName[0] == '@');
    struct gdi_font_face *face;

    if (!TranslateCharsetInfo( (DWORD *)(INT_PTR)lf->lfCharSet, csi, TCI_SRCCHARSET ))
    {
        if (lf->lfCharSet != DEFAULT_CHARSET) FIXME( "Untranslated charset %d\n", lf->lfCharSet );
        csi->fs.fsCsb[0] = 0;
    }

    if (lf->lfFaceName[0])
    {
        int subst_charset;
        const WCHAR *subst = get_gdi_font_subst( lf->lfFaceName, lf->lfCharSet, &subst_charset );

	if (subst)
        {
	    TRACE( "substituting %s,%d -> %s,%d\n", debugstr_w(lf->lfFaceName), lf->lfCharSet,
                   debugstr_w(subst), (subst_charset != -1) ? subst_charset : lf->lfCharSet );
	    if (subst_charset != -1)
                TranslateCharsetInfo( (DWORD *)(INT_PTR)subst_charset, csi, TCI_SRCCHARSET );
            *orig_name = lf->lfFaceName;
	}

        if ((face = find_matching_face_by_name( lf->lfFaceName, subst, lf, csi->fs, can_use_bitmap )))
            return face;
    }
    *orig_name = NULL; /* substitution is no longer relevant */

    /* If requested charset was DEFAULT_CHARSET then try using charset
       corresponding to the current ansi codepage */
    if (!csi->fs.fsCsb[0])
    {
        INT acp = GetACP();
        if (!TranslateCharsetInfo( (DWORD *)(INT_PTR)acp, csi, TCI_SRCCODEPAGE ))
        {
            FIXME( "TCI failed on codepage %d\n", acp );
            csi->fs.fsCsb[0] = 0;
        }
    }

    if ((face = find_any_face( lf, csi->fs, can_use_bitmap, want_vertical ))) return face;
    if (csi->fs.fsCsb[0])
    {
        csi->fs.fsCsb[0] = 0;
        if ((face = find_any_face( lf, csi->fs, can_use_bitmap, want_vertical ))) return face;
    }
    if (want_vertical && (face = find_any_face( lf, csi->fs, can_use_bitmap, FALSE ))) return face;
    return NULL;
}

/* realized font objects */

#define FIRST_FONT_HANDLE 1
#define MAX_FONT_HANDLES  256

struct font_handle_entry
{
    struct gdi_font *font;
    WORD generation; /* generation count for reusing handle values */
};

static struct font_handle_entry font_handles[MAX_FONT_HANDLES];
static struct font_handle_entry *next_free;
static struct font_handle_entry *next_unused = font_handles;

static struct font_handle_entry *handle_entry( DWORD handle )
{
    unsigned int idx = LOWORD(handle) - FIRST_FONT_HANDLE;

    if (idx < MAX_FONT_HANDLES)
    {
        if (!HIWORD( handle ) || HIWORD( handle ) == font_handles[idx].generation)
            return &font_handles[idx];
    }
    if (handle) WARN( "invalid handle 0x%08x\n", handle );
    return NULL;
}

static struct gdi_font *get_font_from_handle( DWORD handle )
{
    struct font_handle_entry *entry = handle_entry( handle );

    if (entry) return entry->font;
    SetLastError( ERROR_INVALID_PARAMETER );
    return NULL;
}

static DWORD alloc_font_handle( struct gdi_font *font )
{
    struct font_handle_entry *entry;

    entry = next_free;
    if (entry)
        next_free = (struct font_handle_entry *)entry->font;
    else if (next_unused < font_handles + MAX_FONT_HANDLES)
        entry = next_unused++;
    else
    {
        ERR( "out of realized font handles\n" );
        return 0;
    }
    entry->font = font;
    if (++entry->generation == 0xffff) entry->generation = 1;
    return MAKELONG( entry - font_handles + FIRST_FONT_HANDLE, entry->generation );
}

static void free_font_handle( DWORD handle )
{
    struct font_handle_entry *entry;

    if ((entry = handle_entry( handle )))
    {
        entry->font = (struct gdi_font *)next_free;
        next_free = entry;
    }
}

static struct gdi_font *alloc_gdi_font( const WCHAR *file, void *data_ptr, SIZE_T data_size )
{
    UINT len = file ? lstrlenW(file) : 0;
    struct gdi_font *font = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                       offsetof( struct gdi_font, file[len + 1] ));

    font->refcount = 1;
    font->matrix.eM11 = font->matrix.eM22 = 1.0;
    font->scale_y = 1;
    font->kern_count = -1;
    list_init( &font->child_fonts );

    if (file)
    {
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (GetFileAttributesExW( file, GetFileExInfoStandard, &info ))
        {
            font->writetime = info.ftLastWriteTime;
            font->data_size = (LONGLONG)info.nFileSizeHigh << 32 | info.nFileSizeLow;
            memcpy( font->file, file, len * sizeof(WCHAR) );
        }
    }
    else
    {
        font->data_ptr = data_ptr;
        font->data_size = data_size;
    }

    font->handle = alloc_font_handle( font );
    return font;
}

static void free_gdi_font( struct gdi_font *font )
{
    DWORD i;
    struct gdi_font *child, *child_next;

    if (font->private) font_funcs->destroy_font( font );
    free_font_handle( font->handle );
    LIST_FOR_EACH_ENTRY_SAFE( child, child_next, &font->child_fonts, struct gdi_font, entry )
    {
        list_remove( &child->entry );
        free_gdi_font( child );
    }
    for (i = 0; i < font->gm_size; i++) HeapFree( GetProcessHeap(), 0, font->gm[i] );
    HeapFree( GetProcessHeap(), 0, font->otm.otmpFamilyName );
    HeapFree( GetProcessHeap(), 0, font->otm.otmpStyleName );
    HeapFree( GetProcessHeap(), 0, font->otm.otmpFaceName );
    HeapFree( GetProcessHeap(), 0, font->otm.otmpFullName );
    HeapFree( GetProcessHeap(), 0, font->gm );
    HeapFree( GetProcessHeap(), 0, font->kern_pairs );
    HeapFree( GetProcessHeap(), 0, font->gsub_table );
    HeapFree( GetProcessHeap(), 0, font );
}

static inline const WCHAR *get_gdi_font_name( struct gdi_font *font )
{
    return (WCHAR *)font->otm.otmpFamilyName;
}

static struct gdi_font *create_gdi_font( const struct gdi_font_face *face, const WCHAR *family_name,
                                         const LOGFONTW *lf )
{
    struct gdi_font *font;

    if (!(font = alloc_gdi_font( face->file, face->data_ptr, face->data_size ))) return NULL;
    font->fs = face->fs;
    font->lf = *lf;
    font->fake_italic = (lf->lfItalic && !(face->ntmFlags & NTM_ITALIC));
    font->fake_bold = (lf->lfWeight > 550 && !(face->ntmFlags & NTM_BOLD));
    font->scalable = face->scalable;
    font->face_index = face->face_index;
    font->ntmFlags = face->ntmFlags;
    font->aa_flags = HIWORD( face->flags );
    if (!family_name) family_name = face->family->family_name;
    font->otm.otmpFamilyName = (char *)strdupW( family_name );
    font->otm.otmpStyleName = (char *)strdupW( face->style_name );
    font->otm.otmpFaceName = (char *)strdupW( face->full_name );
    return font;
}

struct glyph_metrics
{
    GLYPHMETRICS gm;
    ABC          abc;  /* metrics of the unrotated char */
    BOOL         init;
};

#define GM_BLOCK_SIZE 128

/* TODO: GGO format support */
static BOOL get_gdi_font_glyph_metrics( struct gdi_font *font, UINT index, GLYPHMETRICS *gm, ABC *abc )
{
    UINT block = index / GM_BLOCK_SIZE;
    UINT entry = index % GM_BLOCK_SIZE;

    if (block < font->gm_size && font->gm[block] && font->gm[block][entry].init)
    {
        *gm  = font->gm[block][entry].gm;
        *abc = font->gm[block][entry].abc;

        TRACE( "cached gm: %u, %u, %s, %d, %d abc: %d, %u, %d\n",
               gm->gmBlackBoxX, gm->gmBlackBoxY, wine_dbgstr_point( &gm->gmptGlyphOrigin ),
               gm->gmCellIncX, gm->gmCellIncY, abc->abcA, abc->abcB, abc->abcC );
        return TRUE;
    }

    return FALSE;
}

static void set_gdi_font_glyph_metrics( struct gdi_font *font, UINT index,
                                        const GLYPHMETRICS *gm, const ABC *abc )
{
    UINT block = index / GM_BLOCK_SIZE;
    UINT entry = index % GM_BLOCK_SIZE;

    if (block >= font->gm_size)
    {
        struct glyph_metrics **ptr;

        if (font->gm)
            ptr = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, font->gm, (block + 1) * sizeof(*ptr) );
        else
            ptr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (block + 1) * sizeof(*ptr) );
        if (!ptr) return;
        font->gm_size = block + 1;
        font->gm = ptr;
    }
    if (!font->gm[block])
    {
        font->gm[block] = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(**font->gm) * GM_BLOCK_SIZE );
        if (!font->gm[block]) return;
    }
    font->gm[block][entry].gm   = *gm;
    font->gm[block][entry].abc  = *abc;
    font->gm[block][entry].init = TRUE;
}


/* GSUB table support */

typedef struct
{
    DWORD version;
    WORD ScriptList;
    WORD FeatureList;
    WORD LookupList;
} GSUB_Header;

typedef struct
{
    CHAR ScriptTag[4];
    WORD Script;
} GSUB_ScriptRecord;

typedef struct
{
    WORD ScriptCount;
    GSUB_ScriptRecord ScriptRecord[1];
} GSUB_ScriptList;

typedef struct
{
    CHAR LangSysTag[4];
    WORD LangSys;
} GSUB_LangSysRecord;

typedef struct
{
    WORD DefaultLangSys;
    WORD LangSysCount;
    GSUB_LangSysRecord LangSysRecord[1];
} GSUB_Script;

typedef struct
{
    WORD LookupOrder; /* Reserved */
    WORD ReqFeatureIndex;
    WORD FeatureCount;
    WORD FeatureIndex[1];
} GSUB_LangSys;

typedef struct
{
    CHAR FeatureTag[4];
    WORD Feature;
} GSUB_FeatureRecord;

typedef struct
{
    WORD FeatureCount;
    GSUB_FeatureRecord FeatureRecord[1];
} GSUB_FeatureList;

typedef struct
{
    WORD FeatureParams; /* Reserved */
    WORD LookupCount;
    WORD LookupListIndex[1];
} GSUB_Feature;

typedef struct
{
    WORD LookupCount;
    WORD Lookup[1];
} GSUB_LookupList;

typedef struct
{
    WORD LookupType;
    WORD LookupFlag;
    WORD SubTableCount;
    WORD SubTable[1];
} GSUB_LookupTable;

typedef struct
{
    WORD CoverageFormat;
    WORD GlyphCount;
    WORD GlyphArray[1];
} GSUB_CoverageFormat1;

typedef struct
{
    WORD Start;
    WORD End;
    WORD StartCoverageIndex;
} GSUB_RangeRecord;

typedef struct
{
    WORD CoverageFormat;
    WORD RangeCount;
    GSUB_RangeRecord RangeRecord[1];
} GSUB_CoverageFormat2;

typedef struct
{
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD DeltaGlyphID;
} GSUB_SingleSubstFormat1;

typedef struct
{
    WORD SubstFormat; /* = 2 */
    WORD Coverage;
    WORD GlyphCount;
    WORD Substitute[1];
} GSUB_SingleSubstFormat2;

static GSUB_Script *GSUB_get_script_table( GSUB_Header *header, const char *tag )
{
    GSUB_ScriptList *script;
    GSUB_Script *deflt = NULL;
    int i;

    script = (GSUB_ScriptList *)((BYTE *)header + GET_BE_WORD(header->ScriptList));
    TRACE("%i scripts in this font\n", GET_BE_WORD(script->ScriptCount) );
    for (i = 0; i < GET_BE_WORD(script->ScriptCount); i++)
    {
        int offset = GET_BE_WORD(script->ScriptRecord[i].Script);
        GSUB_Script *scr = (GSUB_Script *)((BYTE *)script + offset);
        if (!memcmp( script->ScriptRecord[i].ScriptTag, tag, 4 )) return scr;
        if (!memcmp( script->ScriptRecord[i].ScriptTag, "dflt", 4 )) deflt = scr;
    }
    return deflt;
}

static GSUB_LangSys *GSUB_get_lang_table( GSUB_Script *script, const char *tag )
{
    int i, offset;
    GSUB_LangSys *lang;

    TRACE("Deflang %x, LangCount %i\n",GET_BE_WORD(script->DefaultLangSys), GET_BE_WORD(script->LangSysCount));

    for (i = 0; i < GET_BE_WORD(script->LangSysCount) ; i++)
    {
        offset = GET_BE_WORD(script->LangSysRecord[i].LangSys);
        lang = (GSUB_LangSys *)((BYTE *)script + offset);
        if (!memcmp( script->LangSysRecord[i].LangSysTag, tag, 4 )) return lang;
    }
    offset = GET_BE_WORD(script->DefaultLangSys);
    if (offset) return (GSUB_LangSys *)((BYTE *)script + offset);
    return NULL;
}

static GSUB_Feature *GSUB_get_feature( GSUB_Header *header, GSUB_LangSys *lang, const char *tag )
{
    int i;
    const GSUB_FeatureList *feature;

    feature = (GSUB_FeatureList *)((BYTE *)header + GET_BE_WORD(header->FeatureList));
    TRACE("%i features\n",GET_BE_WORD(lang->FeatureCount));
    for (i = 0; i < GET_BE_WORD(lang->FeatureCount); i++)
    {
        int index = GET_BE_WORD(lang->FeatureIndex[i]);
        if (!memcmp( feature->FeatureRecord[index].FeatureTag, tag, 4 ))
            return (GSUB_Feature *)((BYTE *)feature + GET_BE_WORD(feature->FeatureRecord[index].Feature));
    }
    return NULL;
}

static const char *get_opentype_script( const struct gdi_font *font )
{
    /*
     * I am not sure if this is the correct way to generate our script tag
     */
    switch (font->charset)
    {
        case ANSI_CHARSET: return "latn";
        case BALTIC_CHARSET: return "latn"; /* ?? */
        case CHINESEBIG5_CHARSET: return "hani";
        case EASTEUROPE_CHARSET: return "latn"; /* ?? */
        case GB2312_CHARSET: return "hani";
        case GREEK_CHARSET: return "grek";
        case HANGUL_CHARSET: return "hang";
        case RUSSIAN_CHARSET: return "cyrl";
        case SHIFTJIS_CHARSET: return "kana";
        case TURKISH_CHARSET: return "latn"; /* ?? */
        case VIETNAMESE_CHARSET: return "latn";
        case JOHAB_CHARSET: return "latn"; /* ?? */
        case ARABIC_CHARSET: return "arab";
        case HEBREW_CHARSET: return "hebr";
        case THAI_CHARSET: return "thai";
        default: return "latn";
    }
}

static void *get_GSUB_vert_feature( struct gdi_font *font )
{
    GSUB_Header *header;
    GSUB_Script *script;
    GSUB_LangSys *language;
    GSUB_Feature *feature;
    DWORD length = font_funcs->get_font_data( font, MS_GSUB_TAG, 0, NULL, 0 );

    if (length == GDI_ERROR) return NULL;

    header = HeapAlloc( GetProcessHeap(), 0, length );
    font_funcs->get_font_data( font, MS_GSUB_TAG, 0, header, length );
    TRACE( "Loaded GSUB table of %i bytes\n", length );

    if ((script = GSUB_get_script_table( header, get_opentype_script(font) )))
    {
        if ((language = GSUB_get_lang_table( script, "xxxx" ))) /* Need to get Lang tag */
        {
            feature = GSUB_get_feature( header, language, "vrt2" );
            if (!feature) feature = GSUB_get_feature( header, language, "vert" );
            if (feature)
            {
                font->gsub_table = header;
                return feature;
            }
            TRACE("vrt2/vert feature not found\n");
        }
        else TRACE("Language not found\n");
    }
    else TRACE("Script not found\n");

    HeapFree( GetProcessHeap(), 0, header );
    return NULL;
}

static int GSUB_is_glyph_covered( void *table, UINT glyph )
{
    GSUB_CoverageFormat1 *cf1 = table;

    if (GET_BE_WORD(cf1->CoverageFormat) == 1)
    {
        int i, count = GET_BE_WORD(cf1->GlyphCount);

        TRACE("Coverage Format 1, %i glyphs\n",count);
        for (i = 0; i < count; i++) if (glyph == GET_BE_WORD(cf1->GlyphArray[i])) return i;
        return -1;
    }
    else if (GET_BE_WORD(cf1->CoverageFormat) == 2)
    {
        int i, count;
        GSUB_CoverageFormat2 *cf2 = table;

        count = GET_BE_WORD(cf2->RangeCount);
        TRACE("Coverage Format 2, %i ranges\n",count);
        for (i = 0; i < count; i++)
        {
            if (glyph < GET_BE_WORD(cf2->RangeRecord[i].Start)) return -1;
            if ((glyph >= GET_BE_WORD(cf2->RangeRecord[i].Start)) &&
                (glyph <= GET_BE_WORD(cf2->RangeRecord[i].End)))
            {
                return (GET_BE_WORD(cf2->RangeRecord[i].StartCoverageIndex) +
                    glyph - GET_BE_WORD(cf2->RangeRecord[i].Start));
            }
        }
        return -1;
    }
    else ERR("Unknown CoverageFormat %i\n",GET_BE_WORD(cf1->CoverageFormat));

    return -1;
}

static UINT GSUB_apply_feature( GSUB_Header *header, GSUB_Feature *feature, UINT glyph )
{
    GSUB_LookupList *lookup = (GSUB_LookupList *)((BYTE *)header + GET_BE_WORD(header->LookupList));
    int i, j, offset;

    TRACE("%i lookups\n", GET_BE_WORD(feature->LookupCount));
    for (i = 0; i < GET_BE_WORD(feature->LookupCount); i++)
    {
        GSUB_LookupTable *look;
        offset = GET_BE_WORD(lookup->Lookup[GET_BE_WORD(feature->LookupListIndex[i])]);
        look = (GSUB_LookupTable *)((BYTE *)lookup + offset);
        TRACE("type %i, flag %x, subtables %i\n",
              GET_BE_WORD(look->LookupType),GET_BE_WORD(look->LookupFlag),GET_BE_WORD(look->SubTableCount));
        if (GET_BE_WORD(look->LookupType) == 1)
        {
            for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
            {
                GSUB_SingleSubstFormat1 *ssf1;
                offset = GET_BE_WORD(look->SubTable[j]);
                ssf1 = (GSUB_SingleSubstFormat1 *)((BYTE *)look + offset);
                if (GET_BE_WORD(ssf1->SubstFormat) == 1)
                {
                    int offset = GET_BE_WORD(ssf1->Coverage);
                    TRACE("  subtype 1, delta %i\n", GET_BE_WORD(ssf1->DeltaGlyphID));
                    if (GSUB_is_glyph_covered( (BYTE *) ssf1 + offset, glyph ) != -1)
                    {
                        TRACE("  Glyph 0x%x ->",glyph);
                        glyph += GET_BE_WORD(ssf1->DeltaGlyphID);
                        TRACE(" 0x%x\n",glyph);
                    }
                }
                else
                {
                    GSUB_SingleSubstFormat2 *ssf2;
                    int index, offset;

                    ssf2 = (GSUB_SingleSubstFormat2 *)ssf1;
                    offset = GET_BE_WORD(ssf1->Coverage);
                    TRACE("  subtype 2,  glyph count %i\n", GET_BE_WORD(ssf2->GlyphCount));
                    index = GSUB_is_glyph_covered( (BYTE *)ssf2 + offset, glyph );
                    TRACE("  Coverage index %i\n",index);
                    if (index != -1)
                    {
                        TRACE("    Glyph is 0x%x ->",glyph);
                        glyph = GET_BE_WORD(ssf2->Substitute[index]);
                        TRACE("0x%x\n",glyph);
                    }
                }
            }
        }
        else FIXME("We only handle SubType 1\n");
    }
    return glyph;
}

static UINT get_GSUB_vert_glyph( struct gdi_font *font, UINT glyph )
{
    if (!glyph) return glyph;
    if (!font->gsub_table) return glyph;
    return GSUB_apply_feature( font->gsub_table, font->vert_feature, glyph );
}

static void add_child_font( struct gdi_font *font, const WCHAR *family_name )
{
    FONTSIGNATURE fs = {{0}};
    struct gdi_font *child;
    struct gdi_font_face *face;

    if (!(face = find_matching_face_by_name( family_name, NULL, &font->lf, fs, FALSE ))) return;

    if (!(child = create_gdi_font( face, family_name, &font->lf ))) return;
    child->matrix = font->matrix;
    child->can_use_bitmap = font->can_use_bitmap;
    child->scale_y = font->scale_y;
    child->aveWidth = font->aveWidth;
    child->charset = font->charset;
    child->codepage = font->codepage;
    child->base_font = font;
    list_add_tail( &font->child_fonts, &child->entry );
    TRACE( "created child font %p for base %p\n", child, font );
}

static void create_child_font_list( struct gdi_font *font )
{
    struct gdi_font_link *font_link;
    struct gdi_font_link_entry *entry;
    const WCHAR* font_name;

    if (!(font_name = get_gdi_font_subst( get_gdi_font_name(font), -1, NULL )))
        font_name = get_gdi_font_name( font );

    if ((font_link = find_gdi_font_link( font_name )))
    {
        TRACE("found entry in system list\n");
        LIST_FOR_EACH_ENTRY( entry, &font_link->links, struct gdi_font_link_entry, entry )
            add_child_font( font, entry->family_name );
    }
    /*
     * if not SYMBOL or OEM then we also get all the fonts for Microsoft
     * Sans Serif.  This is how asian windows get default fallbacks for fonts
     */
    if (is_dbcs_ansi_cp(GetACP()) && font->charset != SYMBOL_CHARSET && font->charset != OEM_CHARSET &&
        facename_compare( font_name, L"Microsoft Sans Serif", -1 ) != 0)
    {
        if ((font_link = find_gdi_font_link( L"Microsoft Sans Serif" )))
        {
            TRACE("found entry in default fallback list\n");
            LIST_FOR_EACH_ENTRY( entry, &font_link->links, struct gdi_font_link_entry, entry )
                add_child_font( font, entry->family_name );
        }
    }
}

/* font cache */

static struct list gdi_font_list = LIST_INIT( gdi_font_list );
static struct list unused_gdi_font_list = LIST_INIT( unused_gdi_font_list );
static unsigned int unused_font_count;
#define UNUSED_CACHE_SIZE 10

static BOOL fontcmp( const struct gdi_font *font, DWORD hash, const LOGFONTW *lf,
                     const FMAT2 *matrix, BOOL can_use_bitmap )
{
    if (font->hash != hash) return TRUE;
    if (memcmp( &font->matrix, matrix, sizeof(*matrix))) return TRUE;
    if (memcmp( &font->lf, lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
    if (!font->can_use_bitmap != !can_use_bitmap) return TRUE;
    return facename_compare( font->lf.lfFaceName, lf->lfFaceName, -1 );
}

static DWORD hash_font( const LOGFONTW *lf, const FMAT2 *matrix, BOOL can_use_bitmap )
{
    DWORD hash = 0, *ptr, two_chars;
    WORD *pwc;
    unsigned int i;

    for (i = 0, ptr = (DWORD *)matrix; i < sizeof(*matrix) / sizeof(DWORD); i++, ptr++)
        hash ^= *ptr;
    for(i = 0, ptr = (DWORD *)lf; i < 7; i++, ptr++)
        hash ^= *ptr;
    for(i = 0, ptr = (DWORD *)lf->lfFaceName; i < LF_FACESIZE/2; i++, ptr++)
    {
        two_chars = *ptr;
        pwc = (WCHAR *)&two_chars;
        if(!*pwc) break;
        *pwc = towupper(*pwc);
        pwc++;
        *pwc = towupper(*pwc);
        hash ^= two_chars;
        if(!*pwc) break;
    }
    hash ^= !can_use_bitmap;
    return hash;
}

static void cache_gdi_font( struct gdi_font *font )
{
    static DWORD cache_num = 1;

    font->cache_num = cache_num++;
    font->hash = hash_font( &font->lf, &font->matrix, font->can_use_bitmap );
    list_add_head( &gdi_font_list, &font->entry );
    TRACE( "font %p\n", font );
}

static struct gdi_font *find_cached_gdi_font( const LOGFONTW *lf, const FMAT2 *matrix, BOOL can_use_bitmap )
{
    struct gdi_font *font;
    DWORD hash = hash_font( lf, matrix, can_use_bitmap );

    /* try the in-use list */
    LIST_FOR_EACH_ENTRY( font, &gdi_font_list, struct gdi_font, entry )
    {
        if (fontcmp( font, hash, lf, matrix, can_use_bitmap )) continue;
        list_remove( &font->entry );
        list_add_head( &gdi_font_list, &font->entry );
        if (!font->refcount++)
        {
            list_remove( &font->unused_entry );
            unused_font_count--;
        }
        return font;
    }
    return NULL;
}

static void release_gdi_font( struct gdi_font *font )
{
    if (!font) return;
    if (--font->refcount) return;

    TRACE( "font %p\n", font );

    /* add it to the unused list */
    EnterCriticalSection( &font_cs );
    list_add_head( &unused_gdi_font_list, &font->unused_entry );
    if (unused_font_count > UNUSED_CACHE_SIZE)
    {
        font = LIST_ENTRY( list_tail( &unused_gdi_font_list ), struct gdi_font, unused_entry );
        TRACE( "freeing %p\n", font );
        list_remove( &font->entry );
        list_remove( &font->unused_entry );
        free_gdi_font( font );
    }
    else unused_font_count++;
    LeaveCriticalSection( &font_cs );
}

static void add_font_list(HKEY hkey, const struct nls_update_font_list *fl, int dpi)
{
    const char *sserif = (dpi <= 108) ? fl->sserif_96 : fl->sserif_120;

    RegSetValueExA(hkey, "Courier", 0, REG_SZ, (const BYTE *)fl->courier, strlen(fl->courier)+1);
    RegSetValueExA(hkey, "MS Serif", 0, REG_SZ, (const BYTE *)fl->serif, strlen(fl->serif)+1);
    RegSetValueExA(hkey, "MS Sans Serif", 0, REG_SZ, (const BYTE *)sserif, strlen(sserif)+1);
    RegSetValueExA(hkey, "Small Fonts", 0, REG_SZ, (const BYTE *)fl->small, strlen(fl->small)+1);
}

static void set_value_key(HKEY hkey, const char *name, const char *value)
{
    if (value)
        RegSetValueExA(hkey, name, 0, REG_SZ, (const BYTE *)value, strlen(value) + 1);
    else if (name)
        RegDeleteValueA(hkey, name);
}

static void update_font_association_info(UINT current_ansi_codepage)
{
    if (is_dbcs_ansi_cp(current_ansi_codepage))
    {
        HKEY hkey;
        if (RegCreateKeyW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\FontAssoc", &hkey) == ERROR_SUCCESS)
        {
            HKEY hsubkey;
            if (RegCreateKeyW(hkey, L"Associated Charset", &hsubkey) == ERROR_SUCCESS)
            {
                switch (current_ansi_codepage)
                {
                case 932:
                    set_value_key(hsubkey, "ANSI(00)", "NO");
                    set_value_key(hsubkey, "OEM(FF)", "NO");
                    set_value_key(hsubkey, "SYMBOL(02)", "NO");
                    break;
                case 936:
                case 949:
                case 950:
                    set_value_key(hsubkey, "ANSI(00)", "YES");
                    set_value_key(hsubkey, "OEM(FF)", "YES");
                    set_value_key(hsubkey, "SYMBOL(02)", "NO");
                    break;
                }
                RegCloseKey(hsubkey);
            }

            /* TODO: Associated DefaultFonts */

            RegCloseKey(hkey);
        }
    }
    else
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\FontAssoc");
}

static void set_multi_value_key(HKEY hkey, const WCHAR *name, const WCHAR *value, DWORD len)
{
    if (value)
        RegSetValueExW(hkey, name, 0, REG_MULTI_SZ, (const BYTE *)value, len);
    else if (name)
        RegDeleteValueW(hkey, name);
}

static void update_font_system_link_info(UINT current_ansi_codepage)
{
    static const WCHAR system_link_simplified_chinese[] =
        L"SIMSUN.TTC,SimSun\0"
        L"MINGLIU.TTC,PMingLiu\0"
        L"MSGOTHIC.TTC,MS UI Gothic\0"
        L"BATANG.TTC,Batang\0";
    static const WCHAR system_link_traditional_chinese[] =
        L"MINGLIU.TTC,PMingLiu\0"
        L"SIMSUN.TTC,SimSun\0"
        L"MSGOTHIC.TTC,MS UI Gothic\0"
        L"BATANG.TTC,Batang\0";
    static const WCHAR system_link_japanese[] =
        L"MSGOTHIC.TTC,MS UI Gothic\0"
        L"MINGLIU.TTC,PMingLiU\0"
        L"SIMSUN.TTC,SimSun\0"
        L"GULIM.TTC,Gulim\0";
    static const WCHAR system_link_korean[] =
        L"GULIM.TTC,Gulim\0"
        L"MSGOTHIC.TTC,MS UI Gothic\0"
        L"MINGLIU.TTC,PMingLiU\0"
        L"SIMSUN.TTC,SimSun\0";
    static const WCHAR system_link_non_cjk[] =
        L"MSGOTHIC.TTC,MS UI Gothic\0"
        L"MINGLIU.TTC,PMingLiU\0"
        L"SIMSUN.TTC,SimSun\0"
        L"GULIM.TTC,Gulim\0";
    HKEY hkey;

    if (!RegCreateKeyW(HKEY_LOCAL_MACHINE,
                       L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink", &hkey))
    {
        const WCHAR *link;
        DWORD len;

        switch (current_ansi_codepage)
        {
        case 932:
            link = system_link_japanese;
            len = sizeof(system_link_japanese);
            break;
        case 936:
            link = system_link_simplified_chinese;
            len = sizeof(system_link_simplified_chinese);
            break;
        case 949:
            link = system_link_korean;
            len = sizeof(system_link_korean);
            break;
        case 950:
            link = system_link_traditional_chinese;
            len = sizeof(system_link_traditional_chinese);
            break;
        default:
            link = system_link_non_cjk;
            len = sizeof(system_link_non_cjk);
        }
        set_multi_value_key(hkey, L"Lucida Sans Unicode", link, len);
        set_multi_value_key(hkey, L"Microsoft Sans Serif", link, len);
        set_multi_value_key(hkey, L"Tahoma", link, len);
        RegCloseKey(hkey);
    }
}

static void update_codepage(void)
{
    char buf[40], cpbuf[40];
    HKEY hkey;
    DWORD len, type, size;
    UINT i, ansi_cp, oem_cp;
    DWORD screen_dpi, font_dpi = 0;
    BOOL done = FALSE;

    screen_dpi = get_dpi();
    if (!screen_dpi) screen_dpi = 96;

    size = sizeof(DWORD);
    if (RegQueryValueExW(wine_fonts_key, L"LogPixels", NULL, &type, (BYTE *)&font_dpi, &size) ||
        type != REG_DWORD || size != sizeof(DWORD))
        font_dpi = 0;

    ansi_cp = GetACP();
    oem_cp = GetOEMCP();
    sprintf( cpbuf, "%u,%u", ansi_cp, oem_cp );

    buf[0] = 0;
    len = sizeof(buf);
    if (!RegQueryValueExA(wine_fonts_key, "Codepages", 0, &type, (BYTE *)buf, &len) && type == REG_SZ)
    {
        if (!strcmp( buf, cpbuf ) && screen_dpi == font_dpi) return;  /* already set correctly */
        TRACE("updating registry, codepages/logpixels changed %s/%u -> %u,%u/%u\n",
              buf, font_dpi, ansi_cp, oem_cp, screen_dpi);
    }
    else TRACE("updating registry, codepages/logpixels changed none -> %u,%u/%u\n",
               ansi_cp, oem_cp, screen_dpi);

    RegSetValueExA(wine_fonts_key, "Codepages", 0, REG_SZ, (const BYTE *)cpbuf, strlen(cpbuf)+1);
    RegSetValueExW(wine_fonts_key, L"LogPixels", 0, REG_DWORD, (const BYTE *)&screen_dpi, sizeof(screen_dpi));

    for (i = 0; i < ARRAY_SIZE(nls_update_font_list); i++)
    {
        if (nls_update_font_list[i].ansi_cp == ansi_cp && nls_update_font_list[i].oem_cp == oem_cp)
        {
            if (!RegCreateKeyW( HKEY_CURRENT_CONFIG, L"Software\\Fonts", &hkey ))
            {
                RegSetValueExA(hkey, "OEMFONT.FON", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].oem,
                               strlen(nls_update_font_list[i].oem)+1);
                RegSetValueExA(hkey, "FIXEDFON.FON", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].fixed,
                               strlen(nls_update_font_list[i].fixed)+1);
                RegSetValueExA(hkey, "FONTS.FON", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].system,
                               strlen(nls_update_font_list[i].system)+1);
                RegCloseKey(hkey);
            }
            if (!RegCreateKeyW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", &hkey ))
            {
                add_font_list(hkey, &nls_update_font_list[i], screen_dpi);
                RegCloseKey(hkey);
            }
            if (!RegCreateKeyW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows\\CurrentVersion\\Fonts", &hkey ))
            {
                add_font_list(hkey, &nls_update_font_list[i], screen_dpi);
                RegCloseKey(hkey);
            }
            if (!RegCreateKeyW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes", &hkey ))
            {
                RegSetValueExA(hkey, "MS Shell Dlg", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].shelldlg,
                               strlen(nls_update_font_list[i].shelldlg)+1);
                RegSetValueExA(hkey, "Tms Rmn", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].tmsrmn,
                               strlen(nls_update_font_list[i].tmsrmn)+1);

                set_value_key(hkey, "Fixedsys,0", nls_update_font_list[i].fixed_0);
                set_value_key(hkey, "System,0", nls_update_font_list[i].system_0);
                set_value_key(hkey, "Courier,0", nls_update_font_list[i].courier_0);
                set_value_key(hkey, "MS Serif,0", nls_update_font_list[i].serif_0);
                set_value_key(hkey, "Small Fonts,0", nls_update_font_list[i].small_0);
                set_value_key(hkey, "MS Sans Serif,0", nls_update_font_list[i].sserif_0);
                set_value_key(hkey, "Helv,0", nls_update_font_list[i].helv_0);
                set_value_key(hkey, "Tms Rmn,0", nls_update_font_list[i].tmsrmn_0);

                set_value_key(hkey, nls_update_font_list[i].arial_0.from, nls_update_font_list[i].arial_0.to);
                set_value_key(hkey, nls_update_font_list[i].courier_new_0.from, nls_update_font_list[i].courier_new_0.to);
                set_value_key(hkey, nls_update_font_list[i].times_new_roman_0.from, nls_update_font_list[i].times_new_roman_0.to);

                RegCloseKey(hkey);
            }
            done = TRUE;
        }
        else
        {
            /* Delete the FontSubstitutes from other locales */
            if (!RegCreateKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes", &hkey ))
            {
                set_value_key(hkey, nls_update_font_list[i].arial_0.from, NULL);
                set_value_key(hkey, nls_update_font_list[i].courier_new_0.from, NULL);
                set_value_key(hkey, nls_update_font_list[i].times_new_roman_0.from, NULL);
                RegCloseKey(hkey);
            }
        }
    }
    if (!done)
        FIXME("there is no font defaults for codepages %u,%u\n", ansi_cp, oem_cp);

    /* update locale dependent font association info and font system link info in registry.
       update only when codepages changed, not logpixels. */
    if (strcmp(buf, cpbuf) != 0)
    {
        update_font_association_info(ansi_cp);
        update_font_system_link_info(ansi_cp);
    }
}


/*************************************************************
 * font_CreateDC
 */
static BOOL CDECL font_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                                 LPCWSTR output, const DEVMODEW *devmode )
{
    struct font_physdev *physdev;

    if (!font_funcs) return TRUE;
    if (!(physdev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physdev) ))) return FALSE;
    push_dc_driver( dev, &physdev->dev, &font_driver );
    return TRUE;
}


/*************************************************************
 * font_DeleteDC
 */
static BOOL CDECL font_DeleteDC( PHYSDEV dev )
{
    struct font_physdev *physdev = get_font_dev( dev );

    release_gdi_font( physdev->font );
    HeapFree( GetProcessHeap(), 0, physdev );
    return TRUE;
}


struct gdi_font_enum_data
{
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
};

struct enum_charset
{
    DWORD mask;
    DWORD charset;
    DWORD script;
};

static int load_script_name( UINT id, WCHAR buffer[LF_FACESIZE] )
{
    HRSRC rsrc;
    HGLOBAL hMem;
    WCHAR *p;
    int i;

    id += IDS_FIRST_SCRIPT;
    rsrc = FindResourceW( gdi32_module, (LPCWSTR)(ULONG_PTR)((id >> 4) + 1), (LPCWSTR)6 /*RT_STRING*/ );
    if (!rsrc) return 0;
    hMem = LoadResource( gdi32_module, rsrc );
    if (!hMem) return 0;

    p = LockResource( hMem );
    id &= 0x000f;
    while (id--) p += *p + 1;

    i = min(LF_FACESIZE - 1, *p);
    memcpy(buffer, p + 1, i * sizeof(WCHAR));
    buffer[i] = 0;
    return i;
}

static BOOL is_complex_script_ansi_cp( UINT ansi_cp )
{
    return (ansi_cp == 874 /* Thai */
            || ansi_cp == 1255 /* Hebrew */
            || ansi_cp == 1256 /* Arabic */
        );
}

/***************************************************
 * create_enum_charset_list
 *
 * This function creates charset enumeration list because in DEFAULT_CHARSET
 * case, the ANSI codepage's charset takes precedence over other charsets.
 * Above rule doesn't apply if the ANSI codepage uses complex script (e.g. Thai).
 * This function works as a filter other than DEFAULT_CHARSET case.
 */
static DWORD create_enum_charset_list(DWORD charset, struct enum_charset *list)
{
    struct enum_charset *start = list;
    CHARSETINFO csi;
    int i;

    if (TranslateCharsetInfo( ULongToPtr(charset), &csi, TCI_SRCCHARSET ) && csi.fs.fsCsb[0] != 0)
    {
        list->mask    = csi.fs.fsCsb[0];
        list->charset = csi.ciCharset;
        for (i = 0; i < 32; i++) if (csi.fs.fsCsb[0] & (1u << i)) list->script = i;
        list++;
    }
    else /* charset is DEFAULT_CHARSET or invalid. */
    {
        int acp = GetACP();
        DWORD mask = 0;

        /* Set the current codepage's charset as the first element. */
        if (!is_complex_script_ansi_cp(acp) &&
            TranslateCharsetInfo( (DWORD *)(INT_PTR)acp, &csi, TCI_SRCCODEPAGE ) &&
            csi.fs.fsCsb[0] != 0)
        {
            list->mask    = csi.fs.fsCsb[0];
            list->charset = csi.ciCharset;
            for (i = 0; i < 32; i++) if (csi.fs.fsCsb[0] & (1u << i)) list->script = i;
            mask |= csi.fs.fsCsb[0];
            list++;
        }

        /* Fill out left elements. */
        for (i = 0; i < 32; i++)
        {
            FONTSIGNATURE fs;
            fs.fsCsb[0] = 1u << i;
            fs.fsCsb[1] = 0;
            if (fs.fsCsb[0] & mask) continue; /* skip, already added. */
            if (!TranslateCharsetInfo( fs.fsCsb, &csi, TCI_SRCFONTSIG ))
                continue; /* skip, this is an invalid fsCsb bit. */
            list->mask    = fs.fsCsb[0];
            list->charset = csi.ciCharset;
            list->script  = i;
            mask |= fs.fsCsb[0];
            list++;
        }
        /* add catch all mask for remaining bits */
        if (~mask)
        {
            list->mask    = ~mask;
            list->charset = DEFAULT_CHARSET;
            list->script  = IDS_OTHER - IDS_FIRST_SCRIPT;
            list++;
        }
    }
    return list - start;
}

static UINT get_font_type( const NEWTEXTMETRICEXW *ntm )
{
    UINT ret = 0;

    if (ntm->ntmTm.tmPitchAndFamily & TMPF_TRUETYPE)  ret |= TRUETYPE_FONTTYPE;
    if (ntm->ntmTm.tmPitchAndFamily & TMPF_DEVICE)    ret |= DEVICE_FONTTYPE;
    if (!(ntm->ntmTm.tmPitchAndFamily & TMPF_VECTOR)) ret |= RASTER_FONTTYPE;
    return ret;
}

static BOOL get_face_enum_data( struct gdi_font_face *face, ENUMLOGFONTEXW *elf, NEWTEXTMETRICEXW *ntm )
{
    struct gdi_font *font;
    LOGFONTW lf = { .lfHeight = -4096 /* preferable EM Square size */ };

    if (!face->scalable) lf.lfHeight = 0;

    if (!(font = create_gdi_font( face, NULL, &lf ))) return FALSE;

    if (!font_funcs->load_font( font ))
    {
        free_gdi_font( font );
        return FALSE;
    }

    if (font->scalable && -lf.lfHeight % font->otm.otmEMSquare != 0)
    {
        /* reload with the original EM Square size */
        lf.lfHeight = -font->otm.otmEMSquare;
        free_gdi_font( font );

        if (!(font = create_gdi_font( face, NULL, &lf ))) return FALSE;
        if (!font_funcs->load_font( font ))
        {
            free_gdi_font( font );
            return FALSE;
        }
    }

    if (font_funcs->set_outline_text_metrics( font ))
    {
        static const DWORD ntm_ppem = 32;
        UINT cell_height;

#define TM font->otm.otmTextMetrics
#define SCALE_NTM(value) (MulDiv( ntm->ntmTm.tmHeight, (value), TM.tmHeight ))
        cell_height = TM.tmHeight / ( -lf.lfHeight / font->otm.otmEMSquare );
        ntm->ntmTm.tmHeight = MulDiv( ntm_ppem, cell_height, font->otm.otmEMSquare );
        ntm->ntmTm.tmAscent = SCALE_NTM( TM.tmAscent );
        ntm->ntmTm.tmDescent = ntm->ntmTm.tmHeight - ntm->ntmTm.tmAscent;
        ntm->ntmTm.tmInternalLeading = SCALE_NTM( TM.tmInternalLeading );
        ntm->ntmTm.tmExternalLeading = SCALE_NTM( TM.tmExternalLeading );
        ntm->ntmTm.tmAveCharWidth = SCALE_NTM( TM.tmAveCharWidth );
        ntm->ntmTm.tmMaxCharWidth = SCALE_NTM( TM.tmMaxCharWidth );

        memcpy((char *)&ntm->ntmTm + offsetof( TEXTMETRICW, tmWeight ),
               (const char *)&TM + offsetof( TEXTMETRICW, tmWeight ),
               sizeof(TEXTMETRICW) - offsetof( TEXTMETRICW, tmWeight ));
        ntm->ntmTm.ntmSizeEM = font->otm.otmEMSquare;
        ntm->ntmTm.ntmCellHeight = cell_height;
        ntm->ntmTm.ntmAvgWidth = font->ntmAvgWidth;
#undef SCALE_NTM
#undef TM
    }
    else if (font_funcs->set_bitmap_text_metrics( font ))
    {
        memcpy( &ntm->ntmTm, &font->otm.otmTextMetrics, sizeof(TEXTMETRICW) );
        ntm->ntmTm.ntmSizeEM = ntm->ntmTm.tmHeight - ntm->ntmTm.tmInternalLeading;
        ntm->ntmTm.ntmCellHeight = ntm->ntmTm.tmHeight;
        ntm->ntmTm.ntmAvgWidth = ntm->ntmTm.tmAveCharWidth;
    }
    ntm->ntmTm.ntmFlags = font->ntmFlags;
    ntm->ntmFontSig = font->fs;

    elf->elfLogFont.lfEscapement = 0;
    elf->elfLogFont.lfOrientation = 0;
    elf->elfLogFont.lfHeight = ntm->ntmTm.tmHeight;
    elf->elfLogFont.lfWidth = ntm->ntmTm.tmAveCharWidth;
    elf->elfLogFont.lfWeight = ntm->ntmTm.tmWeight;
    elf->elfLogFont.lfItalic = ntm->ntmTm.tmItalic;
    elf->elfLogFont.lfUnderline = ntm->ntmTm.tmUnderlined;
    elf->elfLogFont.lfStrikeOut = ntm->ntmTm.tmStruckOut;
    elf->elfLogFont.lfCharSet = ntm->ntmTm.tmCharSet;
    elf->elfLogFont.lfOutPrecision = OUT_STROKE_PRECIS;
    elf->elfLogFont.lfClipPrecision = CLIP_STROKE_PRECIS;
    elf->elfLogFont.lfQuality = DRAFT_QUALITY;
    elf->elfLogFont.lfPitchAndFamily = (ntm->ntmTm.tmPitchAndFamily & 0xf1) + 1;
    lstrcpynW( elf->elfLogFont.lfFaceName, (WCHAR *)font->otm.otmpFamilyName, LF_FACESIZE );
    lstrcpynW( elf->elfFullName, (WCHAR *)font->otm.otmpFaceName, LF_FULLFACESIZE );
    lstrcpynW( elf->elfStyle, (WCHAR *)font->otm.otmpStyleName, LF_FACESIZE );

    free_gdi_font( font );
    return TRUE;
}

static BOOL family_matches( struct gdi_font_family *family, const WCHAR *face_name )
{
    struct gdi_font_face *face;

    if (!facename_compare( face_name, family->family_name, LF_FACESIZE - 1 )) return TRUE;
    LIST_FOR_EACH_ENTRY( face, get_family_face_list(family), struct gdi_font_face, entry )
        if (!facename_compare( face_name, face->full_name, LF_FACESIZE - 1 )) return TRUE;
    return FALSE;
}

static BOOL face_matches( const WCHAR *family_name, struct gdi_font_face *face, const WCHAR *face_name )
{
    if (!facename_compare( face_name, family_name, LF_FACESIZE - 1)) return TRUE;
    return !facename_compare( face_name, face->full_name, LF_FACESIZE - 1 );
}

static BOOL enum_face_charsets( const struct gdi_font_family *family, struct gdi_font_face *face,
                                struct enum_charset *list, DWORD count, FONTENUMPROCW proc, LPARAM lparam,
                                const WCHAR *subst )
{
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
    DWORD type, i;

    if (!face->cached_enum_data)
    {
        struct gdi_font_enum_data *data;

        if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) )) ||
            !get_face_enum_data( face, &data->elf, &data->ntm ))
        {
            HeapFree( GetProcessHeap(), 0, data );
            return TRUE;
        }
        face->cached_enum_data = data;
    }

    elf = face->cached_enum_data->elf;
    ntm = face->cached_enum_data->ntm;
    type = get_font_type( &ntm );

    /* font replacement */
    if (family != face->family)
    {
        lstrcpynW( elf.elfLogFont.lfFaceName, family->family_name, LF_FACESIZE );
        lstrcpynW( elf.elfFullName, face->full_name, LF_FULLFACESIZE );
    }
    if (subst) lstrcpynW( elf.elfLogFont.lfFaceName, subst, LF_FACESIZE );

    for (i = 0; i < count; i++)
    {
        if (face->fs.fsCsb[0] == 0)  /* OEM */
        {
            elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet = OEM_CHARSET;
            load_script_name( IDS_OEM_DOS - IDS_FIRST_SCRIPT, elf.elfScript );
            i = count; /* break out of loop after enumeration */
        }
        else
        {
            if (!(face->fs.fsCsb[0] & list[i].mask)) continue;
            /* use the DEFAULT_CHARSET case only if no other charset is present */
            if (list[i].charset == DEFAULT_CHARSET && (face->fs.fsCsb[0] & ~list[i].mask)) continue;
            elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet = list[i].charset;
            load_script_name( list[i].script, elf.elfScript );
            if (!elf.elfScript[0]) FIXME("Unknown elfscript for id %u\n", list[i].script);
        }
        TRACE( "face %s full %s style %s charset = %d type %d script %s it %d weight %d ntmflags %08x\n",
               debugstr_w(elf.elfLogFont.lfFaceName), debugstr_w(elf.elfFullName), debugstr_w(elf.elfStyle),
               elf.elfLogFont.lfCharSet, type, debugstr_w(elf.elfScript),
               elf.elfLogFont.lfItalic, elf.elfLogFont.lfWeight, ntm.ntmTm.ntmFlags );
        /* release section before callback (FIXME) */
        LeaveCriticalSection( &font_cs );
        if (!proc( &elf.elfLogFont, (TEXTMETRICW *)&ntm, type, lparam )) return FALSE;
        EnterCriticalSection( &font_cs );
    }
    return TRUE;
}

/*************************************************************
 * font_EnumFonts
 */
static BOOL CDECL font_EnumFonts( PHYSDEV dev, LOGFONTW *lf, FONTENUMPROCW proc, LPARAM lparam )
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;
    struct enum_charset enum_charsets[32];
    DWORD count, charset;

    charset = lf ? lf->lfCharSet : DEFAULT_CHARSET;

    count = create_enum_charset_list( charset, enum_charsets );

    EnterCriticalSection( &font_cs );

    if (lf && lf->lfFaceName[0])
    {
        const WCHAR *face_name = get_gdi_font_subst( lf->lfFaceName, charset, NULL );
        const WCHAR *orig_name = NULL;

        TRACE( "facename = %s charset %d\n", debugstr_w(lf->lfFaceName), charset );
        if (face_name)
        {
            orig_name = lf->lfFaceName;
            TRACE( "substituting %s -> %s\n", debugstr_w(lf->lfFaceName), debugstr_w(face_name) );
        }
        else face_name = lf->lfFaceName;

        WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
        {
            if (!family_matches(family, face_name)) continue;
            LIST_FOR_EACH_ENTRY( face, get_family_face_list(family), struct gdi_font_face, entry )
            {
                if (!face_matches( family->family_name, face, face_name )) continue;
                if (!enum_face_charsets( family, face, enum_charsets, count, proc, lparam, orig_name ))
                    return FALSE;
	    }
	}
    }
    else
    {
        TRACE( "charset %d\n", charset );
        WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
        {
            face = LIST_ENTRY( list_head(get_family_face_list(family)), struct gdi_font_face, entry );
            if (!enum_face_charsets( family, face, enum_charsets, count, proc, lparam, NULL ))
                return FALSE;
	}
    }
    LeaveCriticalSection( &font_cs );
    return TRUE;
}


static BOOL check_unicode_tategaki( WCHAR ch )
{
    extern const unsigned short vertical_orientation_table[] DECLSPEC_HIDDEN;
    unsigned short orientation = vertical_orientation_table[vertical_orientation_table[vertical_orientation_table[ch >> 8]+((ch >> 4) & 0x0f)]+ (ch & 0xf)];

    /* We only reach this code if typographical substitution did not occur */
    /* Type: U or Type: Tu */
    return (orientation ==  1 || orientation == 3);
}

static UINT get_glyph_index_symbol( struct gdi_font *font, UINT glyph )
{
    UINT index;

    if (glyph < 0x100) glyph += 0xf000;
    /* there are a number of old pre-Unicode "broken" TTFs, which
       do have symbols at U+00XX instead of U+f0XX */
    index = glyph;
    font_funcs->get_glyph_index( font, &index, FALSE );
    if (!index)
    {
        index = glyph - 0xf000;
        font_funcs->get_glyph_index( font, &index, FALSE );
    }
    return index;
}

static UINT get_glyph_index( struct gdi_font *font, UINT glyph )
{
    WCHAR wc = glyph;
    char ch;
    BOOL used;

    if (font_funcs->get_glyph_index( font, &glyph, TRUE )) return glyph;

    if (font->codepage == CP_SYMBOL)
    {
        glyph = get_glyph_index_symbol( font, wc );
        if (!glyph)
        {
            if (WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, &wc, 1, &ch, 1, NULL, NULL ))
                glyph = get_glyph_index_symbol( font, (unsigned char)ch );
        }
    }
    else if (WideCharToMultiByte( font->codepage, WC_NO_BEST_FIT_CHARS, &wc, 1, &ch, 1, NULL, &used ) && !used)
    {
        glyph = (unsigned char)ch;
        font_funcs->get_glyph_index( font, &glyph, FALSE );
    }
    else return 0;

    return glyph;
}

static UINT get_glyph_index_linked( struct gdi_font **font, UINT glyph )
{
    struct gdi_font *child;
    UINT res;

    if ((res = get_glyph_index( *font, glyph ))) return res;
    if (glyph < 32) return 0;  /* don't check linked fonts for control characters */

    LIST_FOR_EACH_ENTRY( child, &(*font)->child_fonts, struct gdi_font, entry )
    {
        if (!child->private && !font_funcs->load_font( child )) continue;
        if ((res = get_glyph_index( child, glyph )))
        {
            *font = child;
            return res;
        }
    }
    return 0;
}

static DWORD get_glyph_outline( struct gdi_font *font, UINT glyph, UINT format,
                                GLYPHMETRICS *gm_ret, ABC *abc_ret, DWORD buflen, void *buf,
                                const MAT2 *mat )
{
    GLYPHMETRICS gm;
    ABC abc;
    DWORD ret = 1;
    UINT index = glyph;
    BOOL tategaki = (*get_gdi_font_name( font ) == '@');

    if (format & GGO_GLYPH_INDEX)
    {
        /* Windows bitmap font, e.g. Small Fonts, uses ANSI character code
           as glyph index. "Treasure Adventure Game" depends on this. */
        font_funcs->get_glyph_index( font, &index, FALSE );
        format &= ~GGO_GLYPH_INDEX;
        /* TODO: Window also turns off tategaki for glyphs passed in by index
            if their unicode code points fall outside of the range that is
            rotated. */
    }
    else
    {
        index = get_glyph_index_linked( &font, glyph );
        if (tategaki)
        {
            UINT orig = index;
            index = get_GSUB_vert_glyph( font, index );
            if (index == orig) tategaki = check_unicode_tategaki( glyph );
        }
    }

    if (mat && !memcmp( mat, &identity, sizeof(*mat) )) mat = NULL;

    if (format == GGO_METRICS && !mat && get_gdi_font_glyph_metrics( font, index, &gm, &abc ))
        goto done;

    ret = font_funcs->get_glyph_outline( font, index, format, &gm, &abc, buflen, buf, mat, tategaki );
    if (ret == GDI_ERROR) return ret;

    if ((format == GGO_METRICS || format == GGO_BITMAP || format ==  WINE_GGO_GRAY16_BITMAP) && !mat)
        set_gdi_font_glyph_metrics( font, index, &gm, &abc );

done:
    if (gm_ret) *gm_ret = gm;
    if (abc_ret) *abc_ret = abc;
    return ret;
}


/*************************************************************
 * font_FontIsLinked
 */
static BOOL CDECL font_FontIsLinked( PHYSDEV dev )
{
    struct font_physdev *physdev = get_font_dev( dev );

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pFontIsLinked );
        return dev->funcs->pFontIsLinked( dev );
    }
    return !list_empty( &physdev->font->child_fonts );
}


/*************************************************************
 * font_GetCharABCWidths
 */
static BOOL CDECL font_GetCharABCWidths( PHYSDEV dev, UINT first, UINT last, ABC *buffer )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT c;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetCharABCWidths );
        return dev->funcs->pGetCharABCWidths( dev, first, last, buffer );
    }

    TRACE( "%p, %u, %u, %p\n", physdev->font, first, last, buffer );

    EnterCriticalSection( &font_cs );
    for (c = first; c <= last; c++, buffer++)
        get_glyph_outline( physdev->font, c, GGO_METRICS, NULL, buffer, 0, NULL, NULL );
    LeaveCriticalSection( &font_cs );
    return TRUE;
}


/*************************************************************
 * font_GetCharABCWidthsI
 */
static BOOL CDECL font_GetCharABCWidthsI( PHYSDEV dev, UINT first, UINT count, WORD *gi, ABC *buffer )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT c;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetCharABCWidthsI );
        return dev->funcs->pGetCharABCWidthsI( dev, first, count, gi, buffer );
    }

    TRACE( "%p, %u, %u, %p\n", physdev->font, first, count, buffer );

    EnterCriticalSection( &font_cs );
    for (c = 0; c < count; c++, buffer++)
        get_glyph_outline( physdev->font, gi ? gi[c] : first + c, GGO_METRICS | GGO_GLYPH_INDEX,
                           NULL, buffer, 0, NULL, NULL );
    LeaveCriticalSection( &font_cs );
    return TRUE;
}


/*************************************************************
 * font_GetCharWidth
 */
static BOOL CDECL font_GetCharWidth( PHYSDEV dev, UINT first, UINT last, INT *buffer )
{
    struct font_physdev *physdev = get_font_dev( dev );
    ABC abc;
    UINT c;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetCharWidth );
        return dev->funcs->pGetCharWidth( dev, first, last, buffer );
    }

    TRACE( "%p, %d, %d, %p\n", physdev->font, first, last, buffer );

    EnterCriticalSection( &font_cs );
    for (c = first; c <= last; c++)
    {
        if (get_glyph_outline( physdev->font, c, GGO_METRICS, NULL, &abc, 0, NULL, NULL ) == GDI_ERROR)
            buffer[c - first] = 0;
        else
            buffer[c - first] = abc.abcA + abc.abcB + abc.abcC;
    }
    LeaveCriticalSection( &font_cs );
    return TRUE;
}


/*************************************************************
 * font_GetCharWidthInfo
 */
static BOOL CDECL font_GetCharWidthInfo( PHYSDEV dev, void *ptr )
{
    struct font_physdev *physdev = get_font_dev( dev );
    struct char_width_info *info = ptr;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetCharWidthInfo );
        return dev->funcs->pGetCharWidthInfo( dev, ptr );
    }

    info->unk = 0;
    if (!physdev->font->scalable || !font_funcs->get_char_width_info( physdev->font, info ))
        info->lsb = info->rsb = 0;

    return TRUE;
}


/*************************************************************
 * font_GetFontData
 */
static DWORD CDECL font_GetFontData( PHYSDEV dev, DWORD table, DWORD offset, void *buf, DWORD size )
{
    struct font_physdev *physdev = get_font_dev( dev );

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetFontData );
        return dev->funcs->pGetFontData( dev, table, offset, buf, size );
    }
    return font_funcs->get_font_data( physdev->font, table, offset, buf, size );
}


/*************************************************************
 * font_GetFontRealizationInfo
 */
static BOOL CDECL font_GetFontRealizationInfo( PHYSDEV dev, void *ptr )
{
    struct font_physdev *physdev = get_font_dev( dev );
    struct font_realization_info *info = ptr;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetFontRealizationInfo );
        return dev->funcs->pGetFontRealizationInfo( dev, ptr );
    }

    TRACE( "(%p, %p)\n", physdev->font, info);

    info->flags = 1;
    if (physdev->font->scalable) info->flags |= 2;

    info->cache_num = physdev->font->cache_num;
    info->instance_id = physdev->font->handle;
    if (info->size == sizeof(*info))
    {
        info->unk = 0;
        info->face_index = physdev->font->face_index;
        info->simulations = 0;
        if (physdev->font->fake_bold) info->simulations |= 0x1;
        if (physdev->font->fake_italic) info->simulations |= 0x2;
    }
    return TRUE;
}


/*************************************************************
 * font_GetFontUnicodeRanges
 */
static DWORD CDECL font_GetFontUnicodeRanges( PHYSDEV dev, GLYPHSET *glyphset )
{
    struct font_physdev *physdev = get_font_dev( dev );
    DWORD size, num_ranges;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetFontUnicodeRanges );
        return dev->funcs->pGetFontUnicodeRanges( dev, glyphset );
    }

    num_ranges = font_funcs->get_unicode_ranges( physdev->font, glyphset );
    size = offsetof( GLYPHSET, ranges[num_ranges] );
    if (glyphset)
    {
        glyphset->cbThis = size;
        glyphset->cRanges = num_ranges;
        glyphset->flAccel = 0;
    }
    return size;
}


/*************************************************************
 * font_GetGlyphIndices
 */
static DWORD CDECL font_GetGlyphIndices( PHYSDEV dev, const WCHAR *str, INT count, WORD *gi, DWORD flags )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT default_char;
    char ch;
    BOOL used, got_default = FALSE;
    int i;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetGlyphIndices );
        return dev->funcs->pGetGlyphIndices( dev, str, count, gi, flags );
    }

    if (flags & GGI_MARK_NONEXISTING_GLYPHS)
    {
        default_char = 0xffff;  /* XP would use 0x1f for bitmap fonts */
        got_default = TRUE;
    }

    EnterCriticalSection( &font_cs );

    for (i = 0; i < count; i++)
    {
        UINT glyph = str[i];

        if (!font_funcs->get_glyph_index( physdev->font, &glyph, TRUE ))
        {
            glyph = 0;
            if (physdev->font->codepage == CP_SYMBOL)
            {
                if (str[i] >= 0xf020 && str[i] <= 0xf100) glyph = str[i] - 0xf000;
                else if (str[i] < 0x100) glyph = str[i];
            }
            else if (WideCharToMultiByte( physdev->font->codepage, WC_NO_BEST_FIT_CHARS, &str[i], 1,
                                          &ch, 1, NULL, &used ) && !used)
                glyph = (unsigned char)ch;
        }
        if (!glyph)
        {
            if (!got_default)
            {
                default_char = font_funcs->get_default_glyph( physdev->font );
                got_default = TRUE;
            }
            gi[i] = default_char;
        }
        else gi[i] = get_GSUB_vert_glyph( physdev->font, glyph );
    }

    LeaveCriticalSection( &font_cs );
    return count;
}


/*************************************************************
 * font_GetGlyphOutline
 */
static DWORD CDECL font_GetGlyphOutline( PHYSDEV dev, UINT glyph, UINT format,
                                         GLYPHMETRICS *gm, DWORD buflen, void *buf, const MAT2 *mat )
{
    struct font_physdev *physdev = get_font_dev( dev );
    DWORD ret;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetGlyphOutline );
        return dev->funcs->pGetGlyphOutline( dev, glyph, format, gm, buflen, buf, mat );
    }
    EnterCriticalSection( &font_cs );
    ret = get_glyph_outline( physdev->font, glyph, format, gm, NULL, buflen, buf, mat );
    LeaveCriticalSection( &font_cs );
    return ret;
}


/*************************************************************
 * font_GetKerningPairs
 */
static DWORD CDECL font_GetKerningPairs( PHYSDEV dev, DWORD count, KERNINGPAIR *pairs )
{
    struct font_physdev *physdev = get_font_dev( dev );

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetKerningPairs );
        return dev->funcs->pGetKerningPairs( dev, count, pairs );
    }

    EnterCriticalSection( &font_cs );
    if (physdev->font->kern_count == -1)
        physdev->font->kern_count = font_funcs->get_kerning_pairs( physdev->font,
                                                                   &physdev->font->kern_pairs );
    LeaveCriticalSection( &font_cs );

    if (count && pairs)
    {
        count = min( count, physdev->font->kern_count );
        memcpy( pairs, physdev->font->kern_pairs, count * sizeof(*pairs) );
    }
    else count = physdev->font->kern_count;

    return count;
}


static void scale_outline_font_metrics( const struct gdi_font *font, OUTLINETEXTMETRICW *otm )
{
    double scale_x, scale_y;

    if (font->aveWidth)
    {
        scale_x = (double)font->aveWidth;
        scale_x /= (double)font->otm.otmTextMetrics.tmAveCharWidth;
    }
    else
        scale_x = font->scale_y;

    scale_x *= fabs(font->matrix.eM11);
    scale_y = font->scale_y * fabs(font->matrix.eM22);

/* Windows scales these values as signed integers even if they are unsigned */
#define SCALE_X(x) (x) = GDI_ROUND((int)(x) * (scale_x))
#define SCALE_Y(y) (y) = GDI_ROUND((int)(y) * (scale_y))

    SCALE_Y(otm->otmTextMetrics.tmHeight);
    SCALE_Y(otm->otmTextMetrics.tmAscent);
    SCALE_Y(otm->otmTextMetrics.tmDescent);
    SCALE_Y(otm->otmTextMetrics.tmInternalLeading);
    SCALE_Y(otm->otmTextMetrics.tmExternalLeading);

    SCALE_X(otm->otmTextMetrics.tmOverhang);
    if (font->fake_bold)
    {
        if (!font->scalable) otm->otmTextMetrics.tmOverhang++;
        otm->otmTextMetrics.tmAveCharWidth++;
        otm->otmTextMetrics.tmMaxCharWidth++;
    }
    SCALE_X(otm->otmTextMetrics.tmAveCharWidth);
    SCALE_X(otm->otmTextMetrics.tmMaxCharWidth);

    SCALE_Y(otm->otmAscent);
    SCALE_Y(otm->otmDescent);
    SCALE_Y(otm->otmLineGap);
    SCALE_Y(otm->otmsCapEmHeight);
    SCALE_Y(otm->otmsXHeight);
    SCALE_Y(otm->otmrcFontBox.top);
    SCALE_Y(otm->otmrcFontBox.bottom);
    SCALE_X(otm->otmrcFontBox.left);
    SCALE_X(otm->otmrcFontBox.right);
    SCALE_Y(otm->otmMacAscent);
    SCALE_Y(otm->otmMacDescent);
    SCALE_Y(otm->otmMacLineGap);
    SCALE_X(otm->otmptSubscriptSize.x);
    SCALE_Y(otm->otmptSubscriptSize.y);
    SCALE_X(otm->otmptSubscriptOffset.x);
    SCALE_Y(otm->otmptSubscriptOffset.y);
    SCALE_X(otm->otmptSuperscriptSize.x);
    SCALE_Y(otm->otmptSuperscriptSize.y);
    SCALE_X(otm->otmptSuperscriptOffset.x);
    SCALE_Y(otm->otmptSuperscriptOffset.y);
    SCALE_Y(otm->otmsStrikeoutSize);
    SCALE_Y(otm->otmsStrikeoutPosition);
    SCALE_Y(otm->otmsUnderscoreSize);
    SCALE_Y(otm->otmsUnderscorePosition);

#undef SCALE_X
#undef SCALE_Y
}

/*************************************************************
 * font_GetOutlineTextMetrics
 */
static UINT CDECL font_GetOutlineTextMetrics( PHYSDEV dev, UINT size, OUTLINETEXTMETRICW *metrics )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT ret = 0;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetOutlineTextMetrics );
        return dev->funcs->pGetOutlineTextMetrics( dev, size, metrics );
    }

    if (!physdev->font->scalable) return 0;

    EnterCriticalSection( &font_cs );
    if (font_funcs->set_outline_text_metrics( physdev->font ))
    {
	ret = physdev->font->otm.otmSize;
        if (metrics && size >= physdev->font->otm.otmSize)
        {
            WCHAR *ptr = (WCHAR *)(metrics + 1);
            *metrics = physdev->font->otm;
            metrics->otmpFamilyName = (char *)ptr - (ULONG_PTR)metrics;
            lstrcpyW( ptr, (WCHAR *)physdev->font->otm.otmpFamilyName );
            ptr += lstrlenW(ptr) + 1;
            metrics->otmpStyleName = (char *)ptr - (ULONG_PTR)metrics;
            lstrcpyW( ptr, (WCHAR *)physdev->font->otm.otmpStyleName );
            ptr += lstrlenW(ptr) + 1;
            metrics->otmpFaceName = (char *)ptr - (ULONG_PTR)metrics;
            lstrcpyW( ptr, (WCHAR *)physdev->font->otm.otmpFaceName );
            ptr += lstrlenW(ptr) + 1;
            metrics->otmpFullName = (char *)ptr - (ULONG_PTR)metrics;
            lstrcpyW( ptr, (WCHAR *)physdev->font->otm.otmpFullName );
            scale_outline_font_metrics( physdev->font, metrics );
        }
    }
    LeaveCriticalSection( &font_cs );
    return ret;
}


/*************************************************************
 * font_GetTextCharsetInfo
 */
static UINT CDECL font_GetTextCharsetInfo( PHYSDEV dev, FONTSIGNATURE *fs, DWORD flags )
{
    struct font_physdev *physdev = get_font_dev( dev );

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetTextCharsetInfo );
        return dev->funcs->pGetTextCharsetInfo( dev, fs, flags );
    }
    if (fs) *fs = physdev->font->fs;
    return physdev->font->charset;
}


/*************************************************************
 * font_GetTextExtentExPoint
 */
static BOOL CDECL font_GetTextExtentExPoint( PHYSDEV dev, const WCHAR *str, INT count, INT *dxs )
{
    struct font_physdev *physdev = get_font_dev( dev );
    INT i, pos;
    ABC abc;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetTextExtentExPoint );
        return dev->funcs->pGetTextExtentExPoint( dev, str, count, dxs );
    }

    TRACE( "%p, %s, %d\n", physdev->font, debugstr_wn(str, count), count );

    EnterCriticalSection( &font_cs );
    for (i = pos = 0; i < count; i++)
    {
        get_glyph_outline( physdev->font, str[i], GGO_METRICS, NULL, &abc, 0, NULL, NULL );
        pos += abc.abcA + abc.abcB + abc.abcC;
        dxs[i] = pos;
    }
    LeaveCriticalSection( &font_cs );
    return TRUE;
}


/*************************************************************
 * font_GetTextExtentExPointI
 */
static BOOL CDECL font_GetTextExtentExPointI( PHYSDEV dev, const WORD *indices, INT count, INT *dxs )
{
    struct font_physdev *physdev = get_font_dev( dev );
    INT i, pos;
    ABC abc;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetTextExtentExPointI );
        return dev->funcs->pGetTextExtentExPointI( dev, indices, count, dxs );
    }

    TRACE( "%p, %p, %d\n", physdev->font, indices, count );

    EnterCriticalSection( &font_cs );
    for (i = pos = 0; i < count; i++)
    {
        get_glyph_outline( physdev->font, indices[i], GGO_METRICS | GGO_GLYPH_INDEX,
                           NULL, &abc, 0, NULL, NULL );
        pos += abc.abcA + abc.abcB + abc.abcC;
        dxs[i] = pos;
    }
    LeaveCriticalSection( &font_cs );
    return TRUE;
}


/*************************************************************
 * font_GetTextFace
 */
static INT CDECL font_GetTextFace( PHYSDEV dev, INT count, WCHAR *str )
{
    struct font_physdev *physdev = get_font_dev( dev );
    INT len;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetTextFace );
        return dev->funcs->pGetTextFace( dev, count, str );
    }
    len = lstrlenW( get_gdi_font_name(physdev->font) ) + 1;
    if (str)
    {
        lstrcpynW( str, get_gdi_font_name(physdev->font), count );
        len = min( count, len );
    }
    return len;
}


static void scale_font_metrics( struct gdi_font *font, TEXTMETRICW *tm )
{
    double scale_x, scale_y;

    /* Make sure that the font has sane width/height ratio */
    if (font->aveWidth && (font->aveWidth + tm->tmHeight - 1) / tm->tmHeight > 100)
    {
        WARN( "Ignoring too large font->aveWidth %d\n", font->aveWidth );
        font->aveWidth = 0;
    }

    if (font->aveWidth)
    {
        scale_x = (double)font->aveWidth;
        scale_x /= (double)font->otm.otmTextMetrics.tmAveCharWidth;
    }
    else
        scale_x = font->scale_y;

    scale_x *= fabs(font->matrix.eM11);
    scale_y = font->scale_y * fabs(font->matrix.eM22);

#define SCALE_X(x) (x) = GDI_ROUND((x) * scale_x)
#define SCALE_Y(y) (y) = GDI_ROUND((y) * scale_y)

    SCALE_Y(tm->tmHeight);
    SCALE_Y(tm->tmAscent);
    SCALE_Y(tm->tmDescent);
    SCALE_Y(tm->tmInternalLeading);
    SCALE_Y(tm->tmExternalLeading);

    SCALE_X(tm->tmOverhang);
    if (font->fake_bold)
    {
        if (!font->scalable) tm->tmOverhang++;
        tm->tmAveCharWidth++;
        tm->tmMaxCharWidth++;
    }
    SCALE_X(tm->tmAveCharWidth);
    SCALE_X(tm->tmMaxCharWidth);

#undef SCALE_X
#undef SCALE_Y
}

/*************************************************************
 * font_GetTextMetrics
 */
static BOOL CDECL font_GetTextMetrics( PHYSDEV dev, TEXTMETRICW *metrics )
{
    struct font_physdev *physdev = get_font_dev( dev );
    BOOL ret = FALSE;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetTextMetrics );
        return dev->funcs->pGetTextMetrics( dev, metrics );
    }

    EnterCriticalSection( &font_cs );
    if (font_funcs->set_outline_text_metrics( physdev->font ) ||
        font_funcs->set_bitmap_text_metrics( physdev->font ))
    {
        *metrics = physdev->font->otm.otmTextMetrics;
        scale_font_metrics( physdev->font, metrics );
        ret = TRUE;
    }
    LeaveCriticalSection( &font_cs );
    return ret;
}


static void get_nearest_charset( const WCHAR *family_name, struct gdi_font_face *face, CHARSETINFO *csi )
{
  /* Only get here if lfCharSet == DEFAULT_CHARSET or we couldn't find
     a single face with the requested charset.  The idea is to check if
     the selected font supports the current ANSI codepage, if it does
     return the corresponding charset, else return the first charset */

    int i;

    if (TranslateCharsetInfo( (DWORD*)(INT_PTR)GetACP(), csi, TCI_SRCCODEPAGE ))
    {
        const struct gdi_font_link *font_link;

        if (csi->fs.fsCsb[0] & face->fs.fsCsb[0]) return;
        font_link = find_gdi_font_link(family_name);
        if (font_link && (csi->fs.fsCsb[0] & font_link->fs.fsCsb[0])) return;
    }
    for (i = 0; i < 32; i++)
    {
        DWORD fs0 = 1u << i;
        if (face->fs.fsCsb[0] & fs0)
        {
	    if (TranslateCharsetInfo(&fs0, csi, TCI_SRCFONTSIG)) return;
            FIXME("TCI failing on %x\n", fs0);
	}
    }

    FIXME("returning DEFAULT_CHARSET face->fs.fsCsb[0] = %08x file = %s\n",
	  face->fs.fsCsb[0], debugstr_w(face->file));
    csi->ciACP = GetACP();
    csi->ciCharset = DEFAULT_CHARSET;
}

static struct gdi_font *select_font( LOGFONTW *lf, FMAT2 dcmat, BOOL can_use_bitmap )
{
    struct gdi_font *font;
    struct gdi_font_face *face;
    INT height;
    CHARSETINFO csi;
    const WCHAR *orig_name = NULL;

    /* If lfFaceName is "Symbol" then Windows fixes up lfCharSet to
       SYMBOL_CHARSET so that Symbol gets picked irrespective of the
       original value lfCharSet.  Note this is a special case for
       Symbol and doesn't happen at least for "Wingdings*" */
    if (!facename_compare( lf->lfFaceName, L"Symbol", -1 )) lf->lfCharSet = SYMBOL_CHARSET;

    /* check the cache first */
    if ((font = find_cached_gdi_font( lf, &dcmat, can_use_bitmap )))
    {
        TRACE( "returning cached gdiFont(%p)\n", font );
        return font;
    }
    if (!(face = find_matching_face( lf, &csi, can_use_bitmap, &orig_name )))
    {
        FIXME( "can't find a single appropriate font - bailing\n" );
        return NULL;
    }
    height = lf->lfHeight;

    font = create_gdi_font( face, orig_name, lf );
    font->matrix = dcmat;
    font->can_use_bitmap = can_use_bitmap;
    if (!csi.fs.fsCsb[0]) get_nearest_charset( face->family->family_name, face, &csi );
    font->charset = csi.ciCharset;
    font->codepage = csi.ciACP;

    TRACE( "Chosen: %s (%s/%p:%u)\n", debugstr_w(face->full_name), debugstr_w(face->file),
           face->data_ptr, face->face_index );

    font->aveWidth = height ? lf->lfWidth : 0;
    if (!face->scalable)
    {
        /* Windows uses integer scaling factors for bitmap fonts */
        INT scale, scaled_height, diff;
        struct gdi_font *cachedfont;

        if (height > 0)
            diff = height - (signed int)face->size.height;
        else
            diff = -height - ((signed int)face->size.height - face->size.internal_leading);

        /* FIXME: rotation of bitmap fonts is ignored */
        height = abs(GDI_ROUND( (double)height * font->matrix.eM22 ));
        if (font->aveWidth)
            font->aveWidth = (double)font->aveWidth * font->matrix.eM11;
        font->matrix.eM11 = font->matrix.eM22 = 1.0;
        dcmat.eM11 = dcmat.eM22 = 1.0;
        /* As we changed the matrix, we need to search the cache for the font again,
         * otherwise we might explode the cache. */
        if ((cachedfont = find_cached_gdi_font( lf, &dcmat, can_use_bitmap )))
        {
            TRACE("Found cached font after non-scalable matrix rescale!\n");
            free_gdi_font( font );
            return cachedfont;
        }

        if (height != 0) height = diff;
        height += face->size.height;

        scale = (height + face->size.height - 1) / face->size.height;
        scaled_height = scale * face->size.height;
        /* Only jump to the next height if the difference <= 25% original height */
        if (scale > 2 && scaled_height - height > face->size.height / 4) scale--;
        /* The jump between unscaled and doubled is delayed by 1 */
        else if (scale == 2 && scaled_height - height > (face->size.height / 4 - 1)) scale--;
        font->scale_y = scale;
        TRACE("font scale y: %d\n", font->scale_y);
    }

    if (!font_funcs->load_font( font ))
    {
        free_gdi_font( font );
        return NULL;
    }

    if (face->flags & ADDFONT_VERTICAL_FONT) /* We need to try to load the GSUB table */
        font->vert_feature = get_GSUB_vert_feature( font );

    create_child_font_list( font );

    TRACE( "caching: gdiFont=%p\n", font );
    cache_gdi_font( font );
    return font;
}

/*************************************************************
 * font_SelectFont
 */
static HFONT CDECL font_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    struct font_physdev *physdev = get_font_dev( dev );
    struct gdi_font *font = NULL, *prev = physdev->font;
    DC *dc = get_physdev_dc( dev );

    if (hfont)
    {
        LOGFONTW lf;
        FMAT2 dcmat;
        BOOL can_use_bitmap = !!(GetDeviceCaps( dc->hSelf, TEXTCAPS ) & TC_RA_ABLE);

        GetObjectW( hfont, sizeof(lf), &lf );
        switch (lf.lfQuality)
        {
        case NONANTIALIASED_QUALITY:
            if (!*aa_flags) *aa_flags = GGO_BITMAP;
            break;
        case ANTIALIASED_QUALITY:
            if (!*aa_flags) *aa_flags = GGO_GRAY4_BITMAP;
            break;
        }

        lf.lfWidth = abs(lf.lfWidth);

        TRACE( "%s, h=%d, it=%d, weight=%d, PandF=%02x, charset=%d orient %d escapement %d\n",
               debugstr_w(lf.lfFaceName), lf.lfHeight, lf.lfItalic,
               lf.lfWeight, lf.lfPitchAndFamily, lf.lfCharSet, lf.lfOrientation,
               lf.lfEscapement );

        if (dc->GraphicsMode == GM_ADVANCED)
        {
            memcpy( &dcmat, &dc->xformWorld2Vport, sizeof(FMAT2) );
            /* try to avoid not necessary glyph transformations */
            if (dcmat.eM21 == 0.0 && dcmat.eM12 == 0.0 && dcmat.eM11 == dcmat.eM22)
            {
                lf.lfHeight *= fabs(dcmat.eM11);
                lf.lfWidth *= fabs(dcmat.eM11);
                dcmat.eM11 = dcmat.eM22 = dcmat.eM11 < 0 ? -1 : 1;
            }
        }
        else
        {
            /* Windows 3.1 compatibility mode GM_COMPATIBLE has only limited font scaling abilities */
            dcmat.eM11 = dcmat.eM22 = 1.0;
            dcmat.eM21 = dcmat.eM12 = 0;
            lf.lfOrientation = lf.lfEscapement;
            if (dc->vport2WorldValid)
            {
                if (dc->xformWorld2Vport.eM11 * dc->xformWorld2Vport.eM22 < 0)
                    lf.lfOrientation = -lf.lfOrientation;
                lf.lfHeight *= fabs(dc->xformWorld2Vport.eM22);
                lf.lfWidth *= fabs(dc->xformWorld2Vport.eM22);
            }
        }
        TRACE( "DC transform %f %f %f %f\n", dcmat.eM11, dcmat.eM12, dcmat.eM21, dcmat.eM22 );

        EnterCriticalSection( &font_cs );

        font = select_font( &lf, dcmat, can_use_bitmap );

        if (font)
        {
            if (!*aa_flags) *aa_flags = font->aa_flags;
            if (!*aa_flags)
            {
                if (lf.lfQuality == CLEARTYPE_QUALITY || lf.lfQuality == CLEARTYPE_NATURAL_QUALITY)
                    *aa_flags = subpixel_orientation;
                else
                    *aa_flags = font_smoothing;
            }
            *aa_flags = font_funcs->get_aa_flags( font, *aa_flags, antialias_fakes );
        }
        TRACE( "%p %s %d aa %x\n", hfont, debugstr_w(lf.lfFaceName), lf.lfHeight, *aa_flags );
        LeaveCriticalSection( &font_cs );
    }
    physdev->font = font;
    if (prev) release_gdi_font( prev );
    return font ? hfont : 0;
}


const struct gdi_dc_funcs font_driver =
{
    NULL,                           /* pAbortDoc */
    NULL,                           /* pAbortPath */
    NULL,                           /* pAlphaBlend */
    NULL,                           /* pAngleArc */
    NULL,                           /* pArc */
    NULL,                           /* pArcTo */
    NULL,                           /* pBeginPath */
    NULL,                           /* pBlendImage */
    NULL,                           /* pChord */
    NULL,                           /* pCloseFigure */
    NULL,                           /* pCreateCompatibleDC */
    font_CreateDC,                  /* pCreateDC */
    font_DeleteDC,                  /* pDeleteDC */
    NULL,                           /* pDeleteObject */
    NULL,                           /* pDeviceCapabilities */
    NULL,                           /* pEllipse */
    NULL,                           /* pEndDoc */
    NULL,                           /* pEndPage */
    NULL,                           /* pEndPath */
    font_EnumFonts,                 /* pEnumFonts */
    NULL,                           /* pEnumICMProfiles */
    NULL,                           /* pExcludeClipRect */
    NULL,                           /* pExtDeviceMode */
    NULL,                           /* pExtEscape */
    NULL,                           /* pExtFloodFill */
    NULL,                           /* pExtSelectClipRgn */
    NULL,                           /* pExtTextOut */
    NULL,                           /* pFillPath */
    NULL,                           /* pFillRgn */
    NULL,                           /* pFlattenPath */
    font_FontIsLinked,              /* pFontIsLinked */
    NULL,                           /* pFrameRgn */
    NULL,                           /* pGdiComment */
    NULL,                           /* pGetBoundsRect */
    font_GetCharABCWidths,          /* pGetCharABCWidths */
    font_GetCharABCWidthsI,         /* pGetCharABCWidthsI */
    font_GetCharWidth,              /* pGetCharWidth */
    font_GetCharWidthInfo,          /* pGetCharWidthInfo */
    NULL,                           /* pGetDeviceCaps */
    NULL,                           /* pGetDeviceGammaRamp */
    font_GetFontData,               /* pGetFontData */
    font_GetFontRealizationInfo,    /* pGetFontRealizationInfo */
    font_GetFontUnicodeRanges,      /* pGetFontUnicodeRanges */
    font_GetGlyphIndices,           /* pGetGlyphIndices */
    font_GetGlyphOutline,           /* pGetGlyphOutline */
    NULL,                           /* pGetICMProfile */
    NULL,                           /* pGetImage */
    font_GetKerningPairs,           /* pGetKerningPairs */
    NULL,                           /* pGetNearestColor */
    font_GetOutlineTextMetrics,     /* pGetOutlineTextMetrics */
    NULL,                           /* pGetPixel */
    NULL,                           /* pGetSystemPaletteEntries */
    font_GetTextCharsetInfo,        /* pGetTextCharsetInfo */
    font_GetTextExtentExPoint,      /* pGetTextExtentExPoint */
    font_GetTextExtentExPointI,     /* pGetTextExtentExPointI */
    font_GetTextFace,               /* pGetTextFace */
    font_GetTextMetrics,            /* pGetTextMetrics */
    NULL,                           /* pGradientFill */
    NULL,                           /* pIntersectClipRect */
    NULL,                           /* pInvertRgn */
    NULL,                           /* pLineTo */
    NULL,                           /* pModifyWorldTransform */
    NULL,                           /* pMoveTo */
    NULL,                           /* pOffsetClipRgn */
    NULL,                           /* pOffsetViewportOrg */
    NULL,                           /* pOffsetWindowOrg */
    NULL,                           /* pPaintRgn */
    NULL,                           /* pPatBlt */
    NULL,                           /* pPie */
    NULL,                           /* pPolyBezier */
    NULL,                           /* pPolyBezierTo */
    NULL,                           /* pPolyDraw */
    NULL,                           /* pPolyPolygon */
    NULL,                           /* pPolyPolyline */
    NULL,                           /* pPolygon */
    NULL,                           /* pPolyline */
    NULL,                           /* pPolylineTo */
    NULL,                           /* pPutImage */
    NULL,                           /* pRealizeDefaultPalette */
    NULL,                           /* pRealizePalette */
    NULL,                           /* pRectangle */
    NULL,                           /* pResetDC */
    NULL,                           /* pRestoreDC */
    NULL,                           /* pRoundRect */
    NULL,                           /* pSaveDC */
    NULL,                           /* pScaleViewportExt */
    NULL,                           /* pScaleWindowExt */
    NULL,                           /* pSelectBitmap */
    NULL,                           /* pSelectBrush */
    NULL,                           /* pSelectClipPath */
    font_SelectFont,                /* pSelectFont */
    NULL,                           /* pSelectPalette */
    NULL,                           /* pSelectPen */
    NULL,                           /* pSetArcDirection */
    NULL,                           /* pSetBkColor */
    NULL,                           /* pSetBkMode */
    NULL,                           /* pSetBoundsRect */
    NULL,                           /* pSetDCBrushColor */
    NULL,                           /* pSetDCPenColor */
    NULL,                           /* pSetDIBitsToDevice */
    NULL,                           /* pSetDeviceClipping */
    NULL,                           /* pSetDeviceGammaRamp */
    NULL,                           /* pSetLayout */
    NULL,                           /* pSetMapMode */
    NULL,                           /* pSetMapperFlags */
    NULL,                           /* pSetPixel */
    NULL,                           /* pSetPolyFillMode */
    NULL,                           /* pSetROP2 */
    NULL,                           /* pSetRelAbs */
    NULL,                           /* pSetStretchBltMode */
    NULL,                           /* pSetTextAlign */
    NULL,                           /* pSetTextCharacterExtra */
    NULL,                           /* pSetTextColor */
    NULL,                           /* pSetTextJustification */
    NULL,                           /* pSetViewportExt */
    NULL,                           /* pSetViewportOrg */
    NULL,                           /* pSetWindowExt */
    NULL,                           /* pSetWindowOrg */
    NULL,                           /* pSetWorldTransform */
    NULL,                           /* pStartDoc */
    NULL,                           /* pStartPage */
    NULL,                           /* pStretchBlt */
    NULL,                           /* pStretchDIBits */
    NULL,                           /* pStrokeAndFillPath */
    NULL,                           /* pStrokePath */
    NULL,                           /* pUnrealizePalette */
    NULL,                           /* pWidenPath */
    NULL,                           /* pD3DKMTCheckVidPnExclusiveOwnership */
    NULL,                           /* pD3DKMTSetVidPnSourceOwner */
    NULL,                           /* wine_get_wgl_driver */
    NULL,                           /* wine_get_vulkan_driver */
    GDI_PRIORITY_FONT_DRV           /* priority */
};

static DWORD get_key_value( HKEY key, const WCHAR *name, DWORD *value )
{
    WCHAR buf[12];
    DWORD count = sizeof(buf), type, err;

    err = RegQueryValueExW( key, name, NULL, &type, (BYTE *)buf, &count );
    if (!err)
    {
        if (type == REG_DWORD) memcpy( value, buf, sizeof(*value) );
        else *value = wcstol( buf, NULL, 10 );
    }
    return err;
}

static void init_font_options(void)
{
    HKEY key;
    DWORD i, type, size, val, gamma = 1400;
    WCHAR buffer[20];

    size = sizeof(buffer);
    if (!RegQueryValueExW( wine_fonts_key, L"AntialiasFakeBoldOrItalic", NULL,
                           &type, (BYTE *)buffer, &size) && type == REG_SZ && size >= 1)
    {
        antialias_fakes = (wcschr(L"yYtT1", buffer[0]) != NULL);
    }

    if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Control Panel\\Desktop", &key ))
    {
        /* FIXME: handle vertical orientations even though Windows doesn't */
        if (!get_key_value( key, L"FontSmoothingOrientation", &val ))
        {
            switch (val)
            {
            case 0: /* FE_FONTSMOOTHINGORIENTATIONBGR */
                subpixel_orientation = WINE_GGO_HBGR_BITMAP;
                break;
            case 1: /* FE_FONTSMOOTHINGORIENTATIONRGB */
                subpixel_orientation = WINE_GGO_HRGB_BITMAP;
                break;
            }
        }
        if (!get_key_value( key, L"FontSmoothing", &val ) && val /* enabled */)
        {
            if (!get_key_value( key, L"FontSmoothingType", &val ) && val == 2 /* FE_FONTSMOOTHINGCLEARTYPE */)
                font_smoothing = subpixel_orientation;
            else
                font_smoothing = GGO_GRAY4_BITMAP;
        }
        if (!get_key_value( key, L"FontSmoothingGamma", &val ) && val)
        {
            gamma = min( max( val, 1000 ), 2200 );
        }
        RegCloseKey( key );
    }

    /* Calibrating the difference between the registry value and the Wine gamma value.
       This looks roughly similar to Windows Native with the same registry value.
       MS GDI seems to be rasterizing the outline at a different rate than FreeType. */
    gamma = 1000 * gamma / 1400;
    if (gamma != 1000)
    {
        for (i = 0; i < 256; i++)
        {
            font_gamma_ramp.encode[i] = pow( i / 255., 1000. / gamma ) * 255. + .5;
            font_gamma_ramp.decode[i] = pow( i / 255., gamma / 1000. ) * 255. + .5;
        }
    }
    font_gamma_ramp.gamma = gamma;
    TRACE("gamma %d\n", font_gamma_ramp.gamma);
}


static void FONT_LogFontAToW( const LOGFONTA *fontA, LPLOGFONTW fontW )
{
    memcpy(fontW, fontA, sizeof(LOGFONTA) - LF_FACESIZE);
    MultiByteToWideChar(CP_ACP, 0, fontA->lfFaceName, -1, fontW->lfFaceName,
			LF_FACESIZE);
    fontW->lfFaceName[LF_FACESIZE-1] = 0;
}

static void FONT_LogFontWToA( const LOGFONTW *fontW, LPLOGFONTA fontA )
{
    memcpy(fontA, fontW, sizeof(LOGFONTA) - LF_FACESIZE);
    WideCharToMultiByte(CP_ACP, 0, fontW->lfFaceName, -1, fontA->lfFaceName,
			LF_FACESIZE, NULL, NULL);
    fontA->lfFaceName[LF_FACESIZE-1] = 0;
}

static void FONT_EnumLogFontExWToA( const ENUMLOGFONTEXW *fontW, LPENUMLOGFONTEXA fontA )
{
    FONT_LogFontWToA( &fontW->elfLogFont, &fontA->elfLogFont );

    WideCharToMultiByte( CP_ACP, 0, fontW->elfFullName, -1,
			 (LPSTR) fontA->elfFullName, LF_FULLFACESIZE, NULL, NULL );
    fontA->elfFullName[LF_FULLFACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfStyle, -1,
			 (LPSTR) fontA->elfStyle, LF_FACESIZE, NULL, NULL );
    fontA->elfStyle[LF_FACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfScript, -1,
			 (LPSTR) fontA->elfScript, LF_FACESIZE, NULL, NULL );
    fontA->elfScript[LF_FACESIZE-1] = '\0';
}

static void FONT_EnumLogFontExAToW( const ENUMLOGFONTEXA *fontA, LPENUMLOGFONTEXW fontW )
{
    FONT_LogFontAToW( &fontA->elfLogFont, &fontW->elfLogFont );

    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfFullName, -1,
			 fontW->elfFullName, LF_FULLFACESIZE );
    fontW->elfFullName[LF_FULLFACESIZE-1] = '\0';
    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfStyle, -1,
			 fontW->elfStyle, LF_FACESIZE );
    fontW->elfStyle[LF_FACESIZE-1] = '\0';
    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfScript, -1,
			 fontW->elfScript, LF_FACESIZE );
    fontW->elfScript[LF_FACESIZE-1] = '\0';
}

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
static void FONT_TextMetricWToA(const TEXTMETRICW *ptmW, LPTEXTMETRICA ptmA )
{
    ptmA->tmHeight = ptmW->tmHeight;
    ptmA->tmAscent = ptmW->tmAscent;
    ptmA->tmDescent = ptmW->tmDescent;
    ptmA->tmInternalLeading = ptmW->tmInternalLeading;
    ptmA->tmExternalLeading = ptmW->tmExternalLeading;
    ptmA->tmAveCharWidth = ptmW->tmAveCharWidth;
    ptmA->tmMaxCharWidth = ptmW->tmMaxCharWidth;
    ptmA->tmWeight = ptmW->tmWeight;
    ptmA->tmOverhang = ptmW->tmOverhang;
    ptmA->tmDigitizedAspectX = ptmW->tmDigitizedAspectX;
    ptmA->tmDigitizedAspectY = ptmW->tmDigitizedAspectY;
    ptmA->tmFirstChar = min(ptmW->tmFirstChar, 255);
    if (ptmW->tmCharSet == SYMBOL_CHARSET)
    {
        ptmA->tmFirstChar = 0x1e;
        ptmA->tmLastChar = 0xff;  /* win9x behaviour - we need the OS2 table data to calculate correctly */
    }
    else if (ptmW->tmPitchAndFamily & TMPF_TRUETYPE)
    {
        ptmA->tmFirstChar = ptmW->tmDefaultChar - 1;
        ptmA->tmLastChar = min(ptmW->tmLastChar, 0xff);
    }
    else
    {
        ptmA->tmFirstChar = min(ptmW->tmFirstChar, 0xff);
        ptmA->tmLastChar  = min(ptmW->tmLastChar,  0xff);
    }
    ptmA->tmDefaultChar = ptmW->tmDefaultChar;
    ptmA->tmBreakChar = ptmW->tmBreakChar;
    ptmA->tmItalic = ptmW->tmItalic;
    ptmA->tmUnderlined = ptmW->tmUnderlined;
    ptmA->tmStruckOut = ptmW->tmStruckOut;
    ptmA->tmPitchAndFamily = ptmW->tmPitchAndFamily;
    ptmA->tmCharSet = ptmW->tmCharSet;
}


static void FONT_NewTextMetricExWToA(const NEWTEXTMETRICEXW *ptmW, NEWTEXTMETRICEXA *ptmA )
{
    FONT_TextMetricWToA((const TEXTMETRICW *)ptmW, (LPTEXTMETRICA)ptmA);
    ptmA->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptmA->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptmA->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptmA->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    memcpy(&ptmA->ntmFontSig, &ptmW->ntmFontSig, sizeof(FONTSIGNATURE));
}

/* compute positions for text rendering, in device coords */
static BOOL get_char_positions( DC *dc, const WCHAR *str, INT count, INT *dx, SIZE *size )
{
    TEXTMETRICW tm;
    PHYSDEV dev;

    size->cx = size->cy = 0;
    if (!count) return TRUE;

    dev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    dev->funcs->pGetTextMetrics( dev, &tm );

    dev = GET_DC_PHYSDEV( dc, pGetTextExtentExPoint );
    if (!dev->funcs->pGetTextExtentExPoint( dev, str, count, dx )) return FALSE;

    if (dc->breakExtra || dc->breakRem)
    {
        int i, space = 0, rem = dc->breakRem;

        for (i = 0; i < count; i++)
        {
            if (str[i] == tm.tmBreakChar)
            {
                space += dc->breakExtra;
                if (rem > 0)
                {
                    space++;
                    rem--;
                }
            }
            dx[i] += space;
        }
    }
    size->cx = dx[count - 1];
    size->cy = tm.tmHeight;
    return TRUE;
}

/* compute positions for text rendering, in device coords */
static BOOL get_char_positions_indices( DC *dc, const WORD *indices, INT count, INT *dx, SIZE *size )
{
    TEXTMETRICW tm;
    PHYSDEV dev;

    size->cx = size->cy = 0;
    if (!count) return TRUE;

    dev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    dev->funcs->pGetTextMetrics( dev, &tm );

    dev = GET_DC_PHYSDEV( dc, pGetTextExtentExPointI );
    if (!dev->funcs->pGetTextExtentExPointI( dev, indices, count, dx )) return FALSE;

    if (dc->breakExtra || dc->breakRem)
    {
        WORD space_index;
        int i, space = 0, rem = dc->breakRem;

        dev = GET_DC_PHYSDEV( dc, pGetGlyphIndices );
        dev->funcs->pGetGlyphIndices( dev, &tm.tmBreakChar, 1, &space_index, 0 );

        for (i = 0; i < count; i++)
        {
            if (indices[i] == space_index)
            {
                space += dc->breakExtra;
                if (rem > 0)
                {
                    space++;
                    rem--;
                }
            }
            dx[i] += space;
        }
    }
    size->cx = dx[count - 1];
    size->cy = tm.tmHeight;
    return TRUE;
}

/***********************************************************************
 *           GdiGetCodePage   (GDI32.@)
 */
DWORD WINAPI GdiGetCodePage( HDC hdc )
{
    UINT cp = CP_ACP;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        cp = dc->font_code_page;
        release_dc_ptr( dc );
    }
    return cp;
}

/***********************************************************************
 *           get_text_charset_info
 *
 * Internal version of GetTextCharsetInfo() that takes a DC pointer.
 */
static UINT get_text_charset_info(DC *dc, FONTSIGNATURE *fs, DWORD flags)
{
    UINT ret = DEFAULT_CHARSET;
    PHYSDEV dev;

    dev = GET_DC_PHYSDEV( dc, pGetTextCharsetInfo );
    ret = dev->funcs->pGetTextCharsetInfo( dev, fs, flags );

    if (ret == DEFAULT_CHARSET && fs)
        memset(fs, 0, sizeof(FONTSIGNATURE));
    return ret;
}

/***********************************************************************
 *           GetTextCharsetInfo    (GDI32.@)
 */
UINT WINAPI GetTextCharsetInfo(HDC hdc, FONTSIGNATURE *fs, DWORD flags)
{
    UINT ret = DEFAULT_CHARSET;
    DC *dc = get_dc_ptr(hdc);

    if (dc)
    {
        ret = get_text_charset_info( dc, fs, flags );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           FONT_mbtowc
 *
 * Returns a Unicode translation of str using the charset of the
 * currently selected font in hdc.  If count is -1 then str is assumed
 * to be '\0' terminated, otherwise it contains the number of bytes to
 * convert.  If plenW is non-NULL, on return it will point to the
 * number of WCHARs that have been written.  If pCP is non-NULL, on
 * return it will point to the codepage used in the conversion.  The
 * caller should free the returned LPWSTR from the process heap
 * itself.
 */
static LPWSTR FONT_mbtowc(HDC hdc, LPCSTR str, INT count, INT *plenW, UINT *pCP)
{
    UINT cp;
    INT lenW;
    LPWSTR strW;

    cp = GdiGetCodePage( hdc );

    if(count == -1) count = strlen(str);
    lenW = MultiByteToWideChar(cp, 0, str, count, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW*sizeof(WCHAR));
    MultiByteToWideChar(cp, 0, str, count, strW, lenW);
    TRACE("mapped %s -> %s\n", debugstr_an(str, count), debugstr_wn(strW, lenW));
    if(plenW) *plenW = lenW;
    if(pCP) *pCP = cp;
    return strW;
}

/***********************************************************************
 *           CreateFontIndirectExA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectExA( const ENUMLOGFONTEXDVA *penumexA )
{
    ENUMLOGFONTEXDVW enumexW;

    if (!penumexA) return 0;

    FONT_EnumLogFontExAToW( &penumexA->elfEnumLogfontEx, &enumexW.elfEnumLogfontEx );
    enumexW.elfDesignVector = penumexA->elfDesignVector;
    return CreateFontIndirectExW( &enumexW );
}

/***********************************************************************
 *           CreateFontIndirectExW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectExW( const ENUMLOGFONTEXDVW *penumex )
{
    HFONT hFont;
    FONTOBJ *fontPtr;
    const LOGFONTW *plf;

    if (!penumex) return 0;

    if (penumex->elfEnumLogfontEx.elfFullName[0] ||
        penumex->elfEnumLogfontEx.elfStyle[0] ||
        penumex->elfEnumLogfontEx.elfScript[0])
    {
        FIXME("some fields ignored. fullname=%s, style=%s, script=%s\n",
            debugstr_w(penumex->elfEnumLogfontEx.elfFullName),
            debugstr_w(penumex->elfEnumLogfontEx.elfStyle),
            debugstr_w(penumex->elfEnumLogfontEx.elfScript));
    }

    plf = &penumex->elfEnumLogfontEx.elfLogFont;
    if (!(fontPtr = HeapAlloc( GetProcessHeap(), 0, sizeof(*fontPtr) ))) return 0;

    fontPtr->logfont = *plf;

    if (!(hFont = alloc_gdi_handle( fontPtr, OBJ_FONT, &fontobj_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, fontPtr );
        return 0;
    }

    TRACE("(%d %d %d %d %x %d %x %d %d) %s %s %s %s => %p\n",
          plf->lfHeight, plf->lfWidth,
          plf->lfEscapement, plf->lfOrientation,
          plf->lfPitchAndFamily,
          plf->lfOutPrecision, plf->lfClipPrecision,
          plf->lfQuality, plf->lfCharSet,
          debugstr_w(plf->lfFaceName),
          plf->lfWeight > 400 ? "Bold" : "",
          plf->lfItalic ? "Italic" : "",
          plf->lfUnderline ? "Underline" : "", hFont);

    return hFont;
}

/***********************************************************************
 *           CreateFontIndirectA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectA( const LOGFONTA *plfA )
{
    LOGFONTW lfW;

    if (!plfA) return 0;

    FONT_LogFontAToW( plfA, &lfW );
    return CreateFontIndirectW( &lfW );
}

/***********************************************************************
 *           CreateFontIndirectW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectW( const LOGFONTW *plf )
{
    ENUMLOGFONTEXDVW exdv;

    if (!plf) return 0;

    exdv.elfEnumLogfontEx.elfLogFont = *plf;
    exdv.elfEnumLogfontEx.elfFullName[0] = 0;
    exdv.elfEnumLogfontEx.elfStyle[0] = 0;
    exdv.elfEnumLogfontEx.elfScript[0] = 0;
    return CreateFontIndirectExW( &exdv );
}

/*************************************************************************
 *           CreateFontA    (GDI32.@)
 */
HFONT WINAPI CreateFontA( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCSTR name )
{
    LOGFONTA logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;

    if (name)
	lstrcpynA(logfont.lfFaceName,name,sizeof(logfont.lfFaceName));
    else
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectA( &logfont );
}

/*************************************************************************
 *           CreateFontW    (GDI32.@)
 */
HFONT WINAPI CreateFontW( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCWSTR name )
{
    LOGFONTW logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;

    if (name)
        lstrcpynW(logfont.lfFaceName, name, ARRAY_SIZE(logfont.lfFaceName));
    else
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectW( &logfont );
}

#define ASSOC_CHARSET_OEM    1
#define ASSOC_CHARSET_ANSI   2
#define ASSOC_CHARSET_SYMBOL 4

static DWORD get_associated_charset_info(void)
{
    static DWORD associated_charset = -1;

    if (associated_charset == -1)
    {
        HKEY hkey;
        WCHAR dataW[32];
        DWORD type, data_len;

        associated_charset = 0;

        if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Control\\FontAssoc\\Associated Charset", &hkey))
            return 0;

        data_len = sizeof(dataW);
        if (!RegQueryValueExW(hkey, L"ANSI(00)", NULL, &type, (LPBYTE)dataW, &data_len) &&
            type == REG_SZ && !wcsicmp(dataW, L"yes"))
            associated_charset |= ASSOC_CHARSET_ANSI;

        data_len = sizeof(dataW);
        if (!RegQueryValueExW(hkey, L"OEM(FF)", NULL, &type, (LPBYTE)dataW, &data_len) &&
            type == REG_SZ && !wcsicmp(dataW, L"yes"))
            associated_charset |= ASSOC_CHARSET_OEM;

        data_len = sizeof(dataW);
        if (!RegQueryValueExW(hkey, L"SYMBOL(02)", NULL, &type, (LPBYTE)dataW, &data_len) &&
            type == REG_SZ && !wcsicmp(dataW, L"yes"))
            associated_charset |= ASSOC_CHARSET_SYMBOL;

        RegCloseKey(hkey);

        TRACE("associated_charset = %d\n", associated_charset);
    }

    return associated_charset;
}

static void update_font_code_page( DC *dc, HANDLE font )
{
    CHARSETINFO csi;
    int charset = get_text_charset_info( dc, NULL, 0 );

    if (charset == ANSI_CHARSET && get_associated_charset_info() & ASSOC_CHARSET_ANSI)
    {
        LOGFONTW lf;

        GetObjectW( font, sizeof(lf), &lf );
        if (!(lf.lfClipPrecision & CLIP_DFA_DISABLE))
            charset = DEFAULT_CHARSET;
    }

    /* Hmm, nicely designed api this one! */
    if (TranslateCharsetInfo( ULongToPtr(charset), &csi, TCI_SRCCHARSET) )
        dc->font_code_page = csi.ciACP;
    else {
        switch(charset) {
        case OEM_CHARSET:
            dc->font_code_page = GetOEMCP();
            break;
        case DEFAULT_CHARSET:
            dc->font_code_page = GetACP();
            break;

        case VISCII_CHARSET:
        case TCVN_CHARSET:
        case KOI8_CHARSET:
        case ISO3_CHARSET:
        case ISO4_CHARSET:
        case ISO10_CHARSET:
        case CELTIC_CHARSET:
            /* FIXME: These have no place here, but because x11drv
               enumerates fonts with these (made up) charsets some apps
               might use them and then the FIXME below would become
               annoying.  Now we could pick the intended codepage for
               each of these, but since it's broken anyway we'll just
               use CP_ACP and hope it'll go away...
            */
            dc->font_code_page = CP_ACP;
            break;

        default:
            FIXME("Can't find codepage for charset %d\n", charset);
            dc->font_code_page = CP_ACP;
            break;
        }
    }

    TRACE("charset %d => cp %d\n", charset, dc->font_code_page);
}

/***********************************************************************
 *           FONT_SelectObject
 */
static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, HDC hdc )
{
    HGDIOBJ ret = 0;
    DC *dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    UINT aa_flags = 0;

    if (!dc) return 0;

    if (!GDI_inc_ref_count( handle ))
    {
        release_dc_ptr( dc );
        return 0;
    }

    physdev = GET_DC_PHYSDEV( dc, pSelectFont );
    if (physdev->funcs->pSelectFont( physdev, handle, &aa_flags ))
    {
        ret = dc->hFont;
        dc->hFont = handle;
        dc->aa_flags = aa_flags ? aa_flags : GGO_BITMAP;
        update_font_code_page( dc, handle );
        if (dc->font_gamma_ramp == NULL)
            dc->font_gamma_ramp = &font_gamma_ramp;
        GDI_dec_ref_count( ret );
    }
    else GDI_dec_ref_count( handle );

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           FONT_GetObjectA
 */
static INT FONT_GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer )
{
    FONTOBJ *font = GDI_GetObjPtr( handle, OBJ_FONT );
    LOGFONTA lfA;

    if (!font) return 0;
    if (buffer)
    {
        FONT_LogFontWToA( &font->logfont, &lfA );
        if (count > sizeof(lfA)) count = sizeof(lfA);
        memcpy( buffer, &lfA, count );
    }
    else count = sizeof(lfA);
    GDI_ReleaseObj( handle );
    return count;
}

/***********************************************************************
 *           FONT_GetObjectW
 */
static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer )
{
    FONTOBJ *font = GDI_GetObjPtr( handle, OBJ_FONT );

    if (!font) return 0;
    if (buffer)
    {
        if (count > sizeof(LOGFONTW)) count = sizeof(LOGFONTW);
        memcpy( buffer, &font->logfont, count );
    }
    else count = sizeof(LOGFONTW);
    GDI_ReleaseObj( handle );
    return count;
}


/***********************************************************************
 *           FONT_DeleteObject
 */
static BOOL FONT_DeleteObject( HGDIOBJ handle )
{
    FONTOBJ *obj;

    if (!(obj = free_gdi_handle( handle ))) return FALSE;
    HeapFree( GetProcessHeap(), 0, obj );
    return TRUE;
}


/***********************************************************************
 *              FONT_EnumInstance
 *
 * Note: plf is really an ENUMLOGFONTEXW, and ptm is a NEWTEXTMETRICEXW.
 *       We have to use other types because of the FONTENUMPROCW definition.
 */
static INT CALLBACK FONT_EnumInstance( const LOGFONTW *plf, const TEXTMETRICW *ptm,
                                       DWORD fType, LPARAM lp )
{
    struct font_enum *pfe = (struct font_enum *)lp;
    INT ret = 1;

    /* lfCharSet is at the same offset in both LOGFONTA and LOGFONTW */
    if ((!pfe->lpLogFontParam ||
        pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET ||
        pfe->lpLogFontParam->lfCharSet == plf->lfCharSet) &&
       (!(fType & RASTER_FONTTYPE) || GetDeviceCaps(pfe->hdc, TEXTCAPS) & TC_RA_ABLE) )
    {
	/* convert font metrics */
        ENUMLOGFONTEXA logfont;
        NEWTEXTMETRICEXA tmA;

        if (!pfe->unicode)
        {
            FONT_EnumLogFontExWToA( (const ENUMLOGFONTEXW *)plf, &logfont);
            FONT_NewTextMetricExWToA( (const NEWTEXTMETRICEXW *)ptm, &tmA );
            plf = (LOGFONTW *)&logfont.elfLogFont;
            ptm = (TEXTMETRICW *)&tmA;
        }
        ret = pfe->lpEnumFunc( plf, ptm, fType, pfe->lpData );
        pfe->retval = ret;
    }
    return ret;
}

/***********************************************************************
 *		FONT_EnumFontFamiliesEx
 */
static INT FONT_EnumFontFamiliesEx( HDC hDC, LPLOGFONTW plf, FONTENUMPROCW efproc,
                                    LPARAM lParam, BOOL unicode )
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hDC );
    struct font_enum fe;

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pEnumFonts );

        if (plf) TRACE("lfFaceName = %s lfCharset = %d\n", debugstr_w(plf->lfFaceName), plf->lfCharSet);
        fe.lpLogFontParam = plf;
        fe.lpEnumFunc = efproc;
        fe.lpData = lParam;
        fe.unicode = unicode;
        fe.hdc = hDC;
        fe.retval = 1;
        ret = physdev->funcs->pEnumFonts( physdev, plf, FONT_EnumInstance, (LPARAM)&fe );
        release_dc_ptr( dc );
    }
    return ret ? fe.retval : 0;
}

/***********************************************************************
 *              EnumFontFamiliesExW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExW( HDC hDC, LPLOGFONTW plf,
                                    FONTENUMPROCW efproc,
                                    LPARAM lParam, DWORD dwFlags )
{
    return FONT_EnumFontFamiliesEx( hDC, plf, efproc, lParam, TRUE );
}

/***********************************************************************
 *              EnumFontFamiliesExA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExA( HDC hDC, LPLOGFONTA plf,
                                    FONTENUMPROCA efproc,
                                    LPARAM lParam, DWORD dwFlags)
{
    LOGFONTW lfW, *plfW;

    if (plf)
    {
        FONT_LogFontAToW( plf, &lfW );
        plfW = &lfW;
    }
    else plfW = NULL;

    return FONT_EnumFontFamiliesEx( hDC, plfW, (FONTENUMPROCW)efproc, lParam, FALSE );
}

/***********************************************************************
 *              EnumFontFamiliesA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesA( HDC hDC, LPCSTR lpFamily,
                                  FONTENUMPROCA efproc, LPARAM lpData )
{
    LOGFONTA lf, *plf;

    if (lpFamily)
    {
        if (!*lpFamily) return 1;
        lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        plf = &lf;
    }
    else plf = NULL;

    return EnumFontFamiliesExA( hDC, plf, efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamiliesW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesW( HDC hDC, LPCWSTR lpFamily,
                                  FONTENUMPROCW efproc, LPARAM lpData )
{
    LOGFONTW lf, *plf;

    if (lpFamily)
    {
        if (!*lpFamily) return 1;
        lstrcpynW( lf.lfFaceName, lpFamily, LF_FACESIZE );
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        plf = &lf;
    }
    else plf = NULL;

    return EnumFontFamiliesExW( hDC, plf, efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontsA		(GDI32.@)
 */
INT WINAPI EnumFontsA( HDC hDC, LPCSTR lpName, FONTENUMPROCA efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesA( hDC, lpName, efproc, lpData );
}

/***********************************************************************
 *              EnumFontsW		(GDI32.@)
 */
INT WINAPI EnumFontsW( HDC hDC, LPCWSTR lpName, FONTENUMPROCW efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesW( hDC, lpName, efproc, lpData );
}


/***********************************************************************
 *           GetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI GetTextCharacterExtra( HDC hdc )
{
    INT ret;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return 0x80000000;
    ret = dc->charExtra;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI SetTextCharacterExtra( HDC hdc, INT extra )
{
    INT ret = 0x80000000;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetTextCharacterExtra );
        extra = physdev->funcs->pSetTextCharacterExtra( physdev, extra );
        if (extra != 0x80000000)
        {
            ret = dc->charExtra;
            dc->charExtra = extra;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextJustification    (GDI32.@)
 */
BOOL WINAPI SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    BOOL ret;
    PHYSDEV physdev;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;

    physdev = GET_DC_PHYSDEV( dc, pSetTextJustification );
    ret = physdev->funcs->pSetTextJustification( physdev, extra, breaks );
    if (ret)
    {
        extra = abs((extra * dc->vport_ext.cx + dc->wnd_ext.cx / 2) / dc->wnd_ext.cx);
        if (!extra) breaks = 0;
        if (breaks)
        {
            dc->breakExtra = extra / breaks;
            dc->breakRem   = extra - (breaks * dc->breakExtra);
        }
        else
        {
            dc->breakExtra = 0;
            dc->breakRem   = 0;
        }
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetTextFaceA    (GDI32.@)
 */
INT WINAPI GetTextFaceA( HDC hdc, INT count, LPSTR name )
{
    INT res = GetTextFaceW(hdc, 0, NULL);
    LPWSTR nameW = HeapAlloc( GetProcessHeap(), 0, res * 2 );
    GetTextFaceW( hdc, res, nameW );

    if (name)
    {
        if (count)
        {
            res = WideCharToMultiByte(CP_ACP, 0, nameW, -1, name, count, NULL, NULL);
            if (res == 0)
                res = count;
            name[count-1] = 0;
            /* GetTextFaceA does NOT include the nul byte in the return count.  */
            res--;
        }
        else
            res = 0;
    }
    else
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL);
    HeapFree( GetProcessHeap(), 0, nameW );
    return res;
}

/***********************************************************************
 *           GetTextFaceW    (GDI32.@)
 */
INT WINAPI GetTextFaceW( HDC hdc, INT count, LPWSTR name )
{
    PHYSDEV dev;
    INT ret;

    DC * dc = get_dc_ptr( hdc );
    if (!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetTextFace );
    ret = dev->funcs->pGetTextFace( dev, count, name );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.@)
 *
 * See GetTextExtentPoint32W.
 */
BOOL WINAPI GetTextExtentPoint32A( HDC hdc, LPCSTR str, INT count,
                                     LPSIZE size )
{
    BOOL ret = FALSE;
    INT wlen;
    LPWSTR p;

    if (count < 0) return FALSE;

    p = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    if (p)
    {
	ret = GetTextExtentPoint32W( hdc, p, wlen, size );
	HeapFree( GetProcessHeap(), 0, p );
    }

    TRACE("(%p %s %d %p): returning %d x %d\n",
          hdc, debugstr_an (str, count), count, size, size->cx, size->cy );
    return ret;
}


/***********************************************************************
 * GetTextExtentPoint32W [GDI32.@]
 *
 * Computes width/height for a string.
 *
 * Computes width and height of the specified string.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentPoint32W(
    HDC hdc,     /* [in]  Handle of device context */
    LPCWSTR str,   /* [in]  Address of text string */
    INT count,   /* [in]  Number of characters in string */
    LPSIZE size) /* [out] Address of structure for string size */
{
    return GetTextExtentExPointW(hdc, str, count, 0, NULL, NULL, size);
}

/***********************************************************************
 * GetTextExtentExPointI [GDI32.@]
 *
 * Computes width and height of the array of glyph indices.
 *
 * PARAMS
 *    hdc     [I] Handle of device context.
 *    indices [I] Glyph index array.
 *    count   [I] Number of glyphs in array.
 *    max_ext [I] Maximum width in glyphs.
 *    nfit    [O] Maximum number of characters.
 *    dxs     [O] Partial string widths.
 *    size    [O] Returned string size.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentExPointI( HDC hdc, const WORD *indices, INT count, INT max_ext,
                                   LPINT nfit, LPINT dxs, LPSIZE size )
{
    DC *dc;
    int i;
    BOOL ret;
    INT buffer[256], *pos = dxs;

    if (count < 0) return FALSE;

    dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if (!dxs)
    {
        pos = buffer;
        if (count > 256 && !(pos = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*pos) )))
        {
            release_dc_ptr( dc );
            return FALSE;
        }
    }

    ret = get_char_positions_indices( dc, indices, count, pos, size );
    if (ret)
    {
        if (dxs || nfit)
        {
            for (i = 0; i < count; i++)
            {
                unsigned int dx = abs( INTERNAL_XDSTOWS( dc, pos[i] )) + (i + 1) * dc->charExtra;
                if (nfit && dx > (unsigned int)max_ext) break;
                if (dxs) dxs[i] = dx;
            }
            if (nfit) *nfit = i;
        }

        size->cx = abs( INTERNAL_XDSTOWS( dc, size->cx )) + count * dc->charExtra;
        size->cy = abs( INTERNAL_YDSTOWS( dc, size->cy ));
    }

    if (pos != buffer && pos != dxs) HeapFree( GetProcessHeap(), 0, pos );
    release_dc_ptr( dc );

    TRACE("(%p %p %d %p): returning %d x %d\n",
          hdc, indices, count, size, size->cx, size->cy );
    return ret;
}

/***********************************************************************
 * GetTextExtentPointI [GDI32.@]
 *
 * Computes width and height of the array of glyph indices.
 *
 * PARAMS
 *    hdc     [I] Handle of device context.
 *    indices [I] Glyph index array.
 *    count   [I] Number of glyphs in array.
 *    size    [O] Returned string size.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentPointI( HDC hdc, const WORD *indices, INT count, LPSIZE size )
{
    return GetTextExtentExPointI( hdc, indices, count, 0, NULL, NULL, size );
}


/***********************************************************************
 *           GetTextExtentPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointA( HDC hdc, LPCSTR str, INT count,
                                          LPSIZE size )
{
    TRACE("not bug compatible.\n");
    return GetTextExtentPoint32A( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextExtentPointW   (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointW( HDC hdc, LPCWSTR str, INT count,
                                          LPSIZE size )
{
    TRACE("not bug compatible.\n");
    return GetTextExtentPoint32W( hdc, str, count, size );
}


/***********************************************************************
 *           GetTextExtentExPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentExPointA( HDC hdc, LPCSTR str, INT count,
				   INT maxExt, LPINT lpnFit,
				   LPINT alpDx, LPSIZE size )
{
    BOOL ret;
    INT wlen;
    INT *walpDx = NULL;
    LPWSTR p = NULL;

    if (count < 0) return FALSE;
    if (maxExt < -1) return FALSE;

    if (alpDx)
    {
        walpDx = HeapAlloc( GetProcessHeap(), 0, count * sizeof(INT) );
        if (!walpDx) return FALSE;
    }

    p = FONT_mbtowc(hdc, str, count, &wlen, NULL);
    ret = GetTextExtentExPointW( hdc, p, wlen, maxExt, lpnFit, walpDx, size);
    if (walpDx)
    {
        INT n = lpnFit ? *lpnFit : wlen;
        INT i, j;
        for(i = 0, j = 0; i < n; i++, j++)
        {
            alpDx[j] = walpDx[i];
            if (IsDBCSLeadByte(str[j])) alpDx[++j] = walpDx[i];
        }
    }
    if (lpnFit) *lpnFit = WideCharToMultiByte(CP_ACP,0,p,*lpnFit,NULL,0,NULL,NULL);
    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, walpDx );
    return ret;
}


/***********************************************************************
 *           GetTextExtentExPointW    (GDI32.@)
 *
 * Return the size of the string as it would be if it was output properly by
 * e.g. TextOut.
 */
BOOL WINAPI GetTextExtentExPointW( HDC hdc, LPCWSTR str, INT count, INT max_ext,
                                   LPINT nfit, LPINT dxs, LPSIZE size )
{
    DC *dc;
    int i;
    BOOL ret;
    INT buffer[256], *pos = dxs;

    if (count < 0) return FALSE;

    dc = get_dc_ptr(hdc);
    if (!dc) return FALSE;

    if (!dxs)
    {
        pos = buffer;
        if (count > 256 && !(pos = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*pos) )))
        {
            release_dc_ptr( dc );
            return FALSE;
        }
    }

    ret = get_char_positions( dc, str, count, pos, size );
    if (ret)
    {
        if (dxs || nfit)
        {
            for (i = 0; i < count; i++)
            {
                unsigned int dx = abs( INTERNAL_XDSTOWS( dc, pos[i] )) + (i + 1) * dc->charExtra;
                if (nfit && dx > (unsigned int)max_ext) break;
		if (dxs) dxs[i] = dx;
            }
            if (nfit) *nfit = i;
        }

        size->cx = abs( INTERNAL_XDSTOWS( dc, size->cx )) + count * dc->charExtra;
        size->cy = abs( INTERNAL_YDSTOWS( dc, size->cy ));
    }

    if (pos != buffer && pos != dxs) HeapFree( GetProcessHeap(), 0, pos );
    release_dc_ptr( dc );

    TRACE("(%p, %s, %d) returning %dx%d\n", hdc, debugstr_wn(str,count), max_ext, size->cx, size->cy );
    return ret;
}

/***********************************************************************
 *           GetTextMetricsA    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsA( HDC hdc, TEXTMETRICA *metrics )
{
    TEXTMETRICW tm32;

    if (!GetTextMetricsW( hdc, &tm32 )) return FALSE;
    FONT_TextMetricWToA( &tm32, metrics );
    return TRUE;
}

/***********************************************************************
 *           GetTextMetricsW    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsW( HDC hdc, TEXTMETRICW *metrics )
{
    PHYSDEV physdev;
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    physdev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    ret = physdev->funcs->pGetTextMetrics( physdev, metrics );

    if (ret)
    {
    /* device layer returns values in device units
     * therefore we have to convert them to logical */

        metrics->tmDigitizedAspectX = GetDeviceCaps(hdc, LOGPIXELSX);
        metrics->tmDigitizedAspectY = GetDeviceCaps(hdc, LOGPIXELSY);
        metrics->tmHeight           = height_to_LP( dc, metrics->tmHeight );
        metrics->tmAscent           = height_to_LP( dc, metrics->tmAscent );
        metrics->tmDescent          = height_to_LP( dc, metrics->tmDescent );
        metrics->tmInternalLeading  = height_to_LP( dc, metrics->tmInternalLeading );
        metrics->tmExternalLeading  = height_to_LP( dc, metrics->tmExternalLeading );
        metrics->tmAveCharWidth     = width_to_LP( dc, metrics->tmAveCharWidth );
        metrics->tmMaxCharWidth     = width_to_LP( dc, metrics->tmMaxCharWidth );
        metrics->tmOverhang         = width_to_LP( dc, metrics->tmOverhang );
        ret = TRUE;

        TRACE("text metrics:\n"
          "    Weight = %03i\t FirstChar = %i\t AveCharWidth = %i\n"
          "    Italic = % 3i\t LastChar = %i\t\t MaxCharWidth = %i\n"
          "    UnderLined = %01i\t DefaultChar = %i\t Overhang = %i\n"
          "    StruckOut = %01i\t BreakChar = %i\t CharSet = %i\n"
          "    PitchAndFamily = %02x\n"
          "    --------------------\n"
          "    InternalLeading = %i\n"
          "    Ascent = %i\n"
          "    Descent = %i\n"
          "    Height = %i\n",
          metrics->tmWeight, metrics->tmFirstChar, metrics->tmAveCharWidth,
          metrics->tmItalic, metrics->tmLastChar, metrics->tmMaxCharWidth,
          metrics->tmUnderlined, metrics->tmDefaultChar, metrics->tmOverhang,
          metrics->tmStruckOut, metrics->tmBreakChar, metrics->tmCharSet,
          metrics->tmPitchAndFamily,
          metrics->tmInternalLeading,
          metrics->tmAscent,
          metrics->tmDescent,
          metrics->tmHeight );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetOutlineTextMetricsA (GDI32.@)
 * Gets metrics for TrueType fonts.
 *
 * NOTES
 *    If the supplied buffer isn't big enough Windows partially fills it up to
 *    its given length and returns that length.
 *
 * RETURNS
 *    Success: Non-zero or size of required buffer
 *    Failure: 0
 */
UINT WINAPI GetOutlineTextMetricsA(
    HDC hdc,    /* [in]  Handle of device context */
    UINT cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRICA lpOTM)  /* [out] Address of metric data array */
{
    char buf[512], *ptr;
    UINT ret, needed;
    OUTLINETEXTMETRICW *lpOTMW = (OUTLINETEXTMETRICW *)buf;
    OUTLINETEXTMETRICA *output = lpOTM;
    INT left, len;

    if((ret = GetOutlineTextMetricsW(hdc, 0, NULL)) == 0)
        return 0;
    if(ret > sizeof(buf))
	lpOTMW = HeapAlloc(GetProcessHeap(), 0, ret);
    GetOutlineTextMetricsW(hdc, ret, lpOTMW);

    needed = sizeof(OUTLINETEXTMETRICA);
    if(lpOTMW->otmpFamilyName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFamilyName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFaceName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFaceName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpStyleName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpStyleName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFullName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFullName), -1,
				      NULL, 0, NULL, NULL);

    if(!lpOTM) {
        ret = needed;
	goto end;
    }

    TRACE("needed = %d\n", needed);
    if(needed > cbData)
        /* Since the supplied buffer isn't big enough, we'll alloc one
           that is and memcpy the first cbData bytes into the lpOTM at
           the end. */
        output = HeapAlloc(GetProcessHeap(), 0, needed);

    ret = output->otmSize = min(needed, cbData);
    FONT_TextMetricWToA( &lpOTMW->otmTextMetrics, &output->otmTextMetrics );
    output->otmFiller = 0;
    output->otmPanoseNumber = lpOTMW->otmPanoseNumber;
    output->otmfsSelection = lpOTMW->otmfsSelection;
    output->otmfsType = lpOTMW->otmfsType;
    output->otmsCharSlopeRise = lpOTMW->otmsCharSlopeRise;
    output->otmsCharSlopeRun = lpOTMW->otmsCharSlopeRun;
    output->otmItalicAngle = lpOTMW->otmItalicAngle;
    output->otmEMSquare = lpOTMW->otmEMSquare;
    output->otmAscent = lpOTMW->otmAscent;
    output->otmDescent = lpOTMW->otmDescent;
    output->otmLineGap = lpOTMW->otmLineGap;
    output->otmsCapEmHeight = lpOTMW->otmsCapEmHeight;
    output->otmsXHeight = lpOTMW->otmsXHeight;
    output->otmrcFontBox = lpOTMW->otmrcFontBox;
    output->otmMacAscent = lpOTMW->otmMacAscent;
    output->otmMacDescent = lpOTMW->otmMacDescent;
    output->otmMacLineGap = lpOTMW->otmMacLineGap;
    output->otmusMinimumPPEM = lpOTMW->otmusMinimumPPEM;
    output->otmptSubscriptSize = lpOTMW->otmptSubscriptSize;
    output->otmptSubscriptOffset = lpOTMW->otmptSubscriptOffset;
    output->otmptSuperscriptSize = lpOTMW->otmptSuperscriptSize;
    output->otmptSuperscriptOffset = lpOTMW->otmptSuperscriptOffset;
    output->otmsStrikeoutSize = lpOTMW->otmsStrikeoutSize;
    output->otmsStrikeoutPosition = lpOTMW->otmsStrikeoutPosition;
    output->otmsUnderscoreSize = lpOTMW->otmsUnderscoreSize;
    output->otmsUnderscorePosition = lpOTMW->otmsUnderscorePosition;


    ptr = (char*)(output + 1);
    left = needed - sizeof(*output);

    if(lpOTMW->otmpFamilyName) {
        output->otmpFamilyName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFamilyName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFamilyName = 0;

    if(lpOTMW->otmpFaceName) {
        output->otmpFaceName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFaceName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFaceName = 0;

    if(lpOTMW->otmpStyleName) {
        output->otmpStyleName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpStyleName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpStyleName = 0;

    if(lpOTMW->otmpFullName) {
        output->otmpFullName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFullName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
    } else
        output->otmpFullName = 0;

    assert(left == 0);

    if(output != lpOTM) {
        memcpy(lpOTM, output, cbData);
        HeapFree(GetProcessHeap(), 0, output);

        /* check if the string offsets really fit into the provided size */
        /* FIXME: should we check string length as well? */
        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFamilyName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFamilyName >= lpOTM->otmSize)
                lpOTM->otmpFamilyName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFaceName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFaceName >= lpOTM->otmSize)
                lpOTM->otmpFaceName = 0; /* doesn't fit */
        }

            /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpStyleName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpStyleName >= lpOTM->otmSize)
                lpOTM->otmpStyleName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFullName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFullName >= lpOTM->otmSize)
                lpOTM->otmpFullName = 0; /* doesn't fit */
        }
    }

end:
    if(lpOTMW != (OUTLINETEXTMETRICW *)buf)
        HeapFree(GetProcessHeap(), 0, lpOTMW);

    return ret;
}


/***********************************************************************
 *           GetOutlineTextMetricsW [GDI32.@]
 */
UINT WINAPI GetOutlineTextMetricsW(
    HDC hdc,    /* [in]  Handle of device context */
    UINT cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRICW lpOTM)  /* [out] Address of metric data array */
{
    DC *dc = get_dc_ptr( hdc );
    OUTLINETEXTMETRICW *output = lpOTM;
    PHYSDEV dev;
    UINT ret;

    TRACE("(%p,%d,%p)\n", hdc, cbData, lpOTM);
    if(!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetOutlineTextMetrics );
    ret = dev->funcs->pGetOutlineTextMetrics( dev, cbData, output );

    if (lpOTM && ret > cbData)
    {
        output = HeapAlloc(GetProcessHeap(), 0, ret);
        ret = dev->funcs->pGetOutlineTextMetrics( dev, ret, output );
    }

    if (lpOTM && ret)
    {
        output->otmTextMetrics.tmDigitizedAspectX = GetDeviceCaps(hdc, LOGPIXELSX);
        output->otmTextMetrics.tmDigitizedAspectY = GetDeviceCaps(hdc, LOGPIXELSY);
        output->otmTextMetrics.tmHeight           = height_to_LP( dc, output->otmTextMetrics.tmHeight );
        output->otmTextMetrics.tmAscent           = height_to_LP( dc, output->otmTextMetrics.tmAscent );
        output->otmTextMetrics.tmDescent          = height_to_LP( dc, output->otmTextMetrics.tmDescent );
        output->otmTextMetrics.tmInternalLeading  = height_to_LP( dc, output->otmTextMetrics.tmInternalLeading );
        output->otmTextMetrics.tmExternalLeading  = height_to_LP( dc, output->otmTextMetrics.tmExternalLeading );
        output->otmTextMetrics.tmAveCharWidth     = width_to_LP( dc, output->otmTextMetrics.tmAveCharWidth );
        output->otmTextMetrics.tmMaxCharWidth     = width_to_LP( dc, output->otmTextMetrics.tmMaxCharWidth );
        output->otmTextMetrics.tmOverhang         = width_to_LP( dc, output->otmTextMetrics.tmOverhang );
        output->otmAscent                = height_to_LP( dc, output->otmAscent);
        output->otmDescent               = height_to_LP( dc, output->otmDescent);
        output->otmLineGap               = INTERNAL_YDSTOWS(dc, output->otmLineGap);
        output->otmsCapEmHeight          = INTERNAL_YDSTOWS(dc, output->otmsCapEmHeight);
        output->otmsXHeight              = INTERNAL_YDSTOWS(dc, output->otmsXHeight);
        output->otmrcFontBox.top         = height_to_LP( dc, output->otmrcFontBox.top);
        output->otmrcFontBox.bottom      = height_to_LP( dc, output->otmrcFontBox.bottom);
        output->otmrcFontBox.left        = width_to_LP( dc, output->otmrcFontBox.left);
        output->otmrcFontBox.right       = width_to_LP( dc, output->otmrcFontBox.right);
        output->otmMacAscent             = height_to_LP( dc, output->otmMacAscent);
        output->otmMacDescent            = height_to_LP( dc, output->otmMacDescent);
        output->otmMacLineGap            = INTERNAL_YDSTOWS(dc, output->otmMacLineGap);
        output->otmptSubscriptSize.x     = width_to_LP( dc, output->otmptSubscriptSize.x);
        output->otmptSubscriptSize.y     = height_to_LP( dc, output->otmptSubscriptSize.y);
        output->otmptSubscriptOffset.x   = width_to_LP( dc, output->otmptSubscriptOffset.x);
        output->otmptSubscriptOffset.y   = height_to_LP( dc, output->otmptSubscriptOffset.y);
        output->otmptSuperscriptSize.x   = width_to_LP( dc, output->otmptSuperscriptSize.x);
        output->otmptSuperscriptSize.y   = height_to_LP( dc, output->otmptSuperscriptSize.y);
        output->otmptSuperscriptOffset.x = width_to_LP( dc, output->otmptSuperscriptOffset.x);
        output->otmptSuperscriptOffset.y = height_to_LP( dc, output->otmptSuperscriptOffset.y);
        output->otmsStrikeoutSize        = INTERNAL_YDSTOWS(dc, output->otmsStrikeoutSize);
        output->otmsStrikeoutPosition    = height_to_LP( dc, output->otmsStrikeoutPosition);
        output->otmsUnderscoreSize       = height_to_LP( dc, output->otmsUnderscoreSize);
        output->otmsUnderscorePosition   = height_to_LP( dc, output->otmsUnderscorePosition);

        if(output != lpOTM)
        {
            memcpy(lpOTM, output, cbData);
            HeapFree(GetProcessHeap(), 0, output);
            ret = cbData;
        }
    }
    release_dc_ptr(dc);
    return ret;
}

static LPSTR FONT_GetCharsByRangeA(HDC hdc, UINT firstChar, UINT lastChar, PINT pByteLen)
{
    INT i, count = lastChar - firstChar + 1;
    UINT mbcp;
    UINT c;
    LPSTR str;

    if (count <= 0)
        return NULL;

    mbcp = GdiGetCodePage(hdc);
    switch (mbcp)
    {
    case 932:
    case 936:
    case 949:
    case 950:
    case 1361:
        if (lastChar > 0xffff)
            return NULL;
        if ((firstChar ^ lastChar) > 0xff)
            return NULL;
        break;
    default:
        if (lastChar > 0xff)
            return NULL;
        mbcp = 0;
        break;
    }

    str = HeapAlloc(GetProcessHeap(), 0, count * 2 + 1);
    if (str == NULL)
        return NULL;

    for(i = 0, c = firstChar; c <= lastChar; i++, c++)
    {
        if (mbcp) {
            if (c > 0xff)
                str[i++] = (BYTE)(c >> 8);
            if (c <= 0xff && IsDBCSLeadByteEx(mbcp, c))
                str[i] = 0x1f; /* FIXME: use default character */
            else
                str[i] = (BYTE)c;
        }
        else
            str[i] = (BYTE)c;
    }
    str[i] = '\0';

    *pByteLen = i;

    return str;
}

/***********************************************************************
 *           GetCharWidthW      (GDI32.@)
 *           GetCharWidth32W    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32W( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    UINT i;
    BOOL ret;
    PHYSDEV dev;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;

    dev = GET_DC_PHYSDEV( dc, pGetCharWidth );
    ret = dev->funcs->pGetCharWidth( dev, firstChar, lastChar, buffer );

    if (ret)
    {
        /* convert device units to logical */
        for( i = firstChar; i <= lastChar; i++, buffer++ )
            *buffer = width_to_LP( dc, *buffer );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetCharWidthA      (GDI32.@)
 *           GetCharWidth32A    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32A( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    INT i, wlen;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    str = FONT_GetCharsByRangeA(hdc, firstChar, lastChar, &i);
    if(str == NULL)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, i, &wlen, NULL);

    for(i = 0; i < wlen; i++)
    {
	if(!GetCharWidth32W(hdc, wstr[i], wstr[i], buffer))
	{
	    ret = FALSE;
	    break;
	}
	buffer++;
    }

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}


/* helper for nulldrv_ExtTextOut */
static DWORD get_glyph_bitmap( HDC hdc, UINT index, UINT flags, UINT aa_flags,
                               GLYPHMETRICS *metrics, struct gdi_image_bits *image )
{
    UINT indices[3] = {0, 0, 0x20};
    unsigned int i;
    DWORD ret, size;
    int stride;

    indices[0] = index;
    if (flags & ETO_GLYPH_INDEX) aa_flags |= GGO_GLYPH_INDEX;

    for (i = 0; i < ARRAY_SIZE( indices ); i++)
    {
        index = indices[i];
        ret = GetGlyphOutlineW( hdc, index, aa_flags, metrics, 0, NULL, &identity );
        if (ret != GDI_ERROR) break;
    }

    if (ret == GDI_ERROR) return ERROR_NOT_FOUND;
    if (!image) return ERROR_SUCCESS;

    image->ptr = NULL;
    image->free = NULL;
    if (!ret)  /* empty glyph */
    {
        metrics->gmBlackBoxX = metrics->gmBlackBoxY = 0;
        return ERROR_SUCCESS;
    }

    stride = get_dib_stride( metrics->gmBlackBoxX, 1 );
    size = metrics->gmBlackBoxY * stride;

    if (!(image->ptr = HeapAlloc( GetProcessHeap(), 0, size ))) return ERROR_OUTOFMEMORY;
    image->is_copy = TRUE;
    image->free = free_heap_bits;

    ret = GetGlyphOutlineW( hdc, index, aa_flags, metrics, size, image->ptr, &identity );
    if (ret == GDI_ERROR)
    {
        HeapFree( GetProcessHeap(), 0, image->ptr );
        return ERROR_NOT_FOUND;
    }
    return ERROR_SUCCESS;
}

/* helper for nulldrv_ExtTextOut */
static RECT get_total_extents( HDC hdc, INT x, INT y, UINT flags, UINT aa_flags,
                               LPCWSTR str, UINT count, const INT *dx )
{
    UINT i;
    RECT rect, bounds;

    reset_bounds( &bounds );
    for (i = 0; i < count; i++)
    {
        GLYPHMETRICS metrics;

        if (get_glyph_bitmap( hdc, str[i], flags, aa_flags, &metrics, NULL )) continue;

        rect.left   = x + metrics.gmptGlyphOrigin.x;
        rect.top    = y - metrics.gmptGlyphOrigin.y;
        rect.right  = rect.left + metrics.gmBlackBoxX;
        rect.bottom = rect.top  + metrics.gmBlackBoxY;
        add_bounds_rect( &bounds, &rect );

        if (dx)
        {
            if (flags & ETO_PDY)
            {
                x += dx[ i * 2 ];
                y += dx[ i * 2 + 1];
            }
            else x += dx[ i ];
        }
        else
        {
            x += metrics.gmCellIncX;
            y += metrics.gmCellIncY;
        }
    }
    return bounds;
}

/* helper for nulldrv_ExtTextOut */
static void draw_glyph( DC *dc, INT origin_x, INT origin_y, const GLYPHMETRICS *metrics,
                        const struct gdi_image_bits *image, const RECT *clip )
{
    static const BYTE masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    UINT i, count, max_count;
    LONG x, y;
    BYTE *ptr = image->ptr;
    int stride = get_dib_stride( metrics->gmBlackBoxX, 1 );
    POINT *pts;
    RECT rect, clipped_rect;

    rect.left   = origin_x  + metrics->gmptGlyphOrigin.x;
    rect.top    = origin_y  - metrics->gmptGlyphOrigin.y;
    rect.right  = rect.left + metrics->gmBlackBoxX;
    rect.bottom = rect.top  + metrics->gmBlackBoxY;
    if (!clip) clipped_rect = rect;
    else if (!intersect_rect( &clipped_rect, &rect, clip )) return;

    max_count = (metrics->gmBlackBoxX + 1) * metrics->gmBlackBoxY;
    pts = HeapAlloc( GetProcessHeap(), 0, max_count * sizeof(*pts) );
    if (!pts) return;

    count = 0;
    ptr += (clipped_rect.top - rect.top) * stride;
    for (y = clipped_rect.top; y < clipped_rect.bottom; y++, ptr += stride)
    {
        for (x = clipped_rect.left - rect.left; x < clipped_rect.right - rect.left; x++)
        {
            while (x < clipped_rect.right - rect.left && !(ptr[x / 8] & masks[x % 8])) x++;
            pts[count].x = rect.left + x;
            while (x < clipped_rect.right - rect.left && (ptr[x / 8] & masks[x % 8])) x++;
            pts[count + 1].x = rect.left + x;
            if (pts[count + 1].x > pts[count].x)
            {
                pts[count].y = pts[count + 1].y = y;
                count += 2;
            }
        }
    }
    assert( count <= max_count );
    dp_to_lp( dc, pts, count );
    for (i = 0; i < count; i += 2) Polyline( dc->hSelf, pts + i, 2 );
    HeapFree( GetProcessHeap(), 0, pts );
}

/***********************************************************************
 *           nulldrv_ExtTextOut
 */
BOOL CDECL nulldrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
                               LPCWSTR str, UINT count, const INT *dx )
{
    DC *dc = get_nulldrv_dc( dev );
    UINT i;
    DWORD err;
    HGDIOBJ orig;
    HPEN pen;

    if (flags & ETO_OPAQUE)
    {
        RECT rc = *rect;
        HBRUSH brush = CreateSolidBrush( GetNearestColor( dev->hdc, dc->backgroundColor ) );

        if (brush)
        {
            orig = SelectObject( dev->hdc, brush );
            dp_to_lp( dc, (POINT *)&rc, 2 );
            PatBlt( dev->hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
            SelectObject( dev->hdc, orig );
            DeleteObject( brush );
        }
    }

    if (!count) return TRUE;

    if (dc->aa_flags != GGO_BITMAP)
    {
        char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        struct gdi_image_bits bits;
        struct bitblt_coords src, dst;
        PHYSDEV dst_dev;
        /* FIXME Subpixel modes */
        UINT aa_flags = GGO_GRAY4_BITMAP;

        dst_dev = GET_DC_PHYSDEV( dc, pPutImage );
        src.visrect = get_total_extents( dev->hdc, x, y, flags, aa_flags, str, count, dx );
        if (flags & ETO_CLIPPED) intersect_rect( &src.visrect, &src.visrect, rect );
        if (!clip_visrect( dc, &src.visrect, &src.visrect )) return TRUE;

        /* FIXME: check for ETO_OPAQUE and avoid GetImage */
        src.x = src.visrect.left;
        src.y = src.visrect.top;
        src.width = src.visrect.right - src.visrect.left;
        src.height = src.visrect.bottom - src.visrect.top;
        dst = src;
        if ((flags & ETO_OPAQUE) && (src.visrect.left >= rect->left) && (src.visrect.top >= rect->top) &&
            (src.visrect.right <= rect->right) && (src.visrect.bottom <= rect->bottom))
        {
            /* we can avoid the GetImage, just query the needed format */
            memset( &info->bmiHeader, 0, sizeof(info->bmiHeader) );
            info->bmiHeader.biSize   = sizeof(info->bmiHeader);
            info->bmiHeader.biWidth  = src.width;
            info->bmiHeader.biHeight = -src.height;
            info->bmiHeader.biSizeImage = get_dib_image_size( info );
            err = dst_dev->funcs->pPutImage( dst_dev, 0, info, NULL, NULL, NULL, 0 );
            if (!err || err == ERROR_BAD_FORMAT)
            {
                /* make the source rectangle relative to the source bits */
                src.x = src.y = 0;
                src.visrect.left = src.visrect.top = 0;
                src.visrect.right = src.width;
                src.visrect.bottom = src.height;

                bits.ptr = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
                if (!bits.ptr) return ERROR_OUTOFMEMORY;
                bits.is_copy = TRUE;
                bits.free = free_heap_bits;
                err = ERROR_SUCCESS;
            }
        }
        else
        {
            PHYSDEV src_dev = GET_DC_PHYSDEV( dc, pGetImage );
            err = src_dev->funcs->pGetImage( src_dev, info, &bits, &src );
            if (!err && !bits.is_copy)
            {
                void *ptr = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
                if (!ptr)
                {
                    if (bits.free) bits.free( &bits );
                    return ERROR_OUTOFMEMORY;
                }
                memcpy( ptr, bits.ptr, info->bmiHeader.biSizeImage );
                if (bits.free) bits.free( &bits );
                bits.ptr = ptr;
                bits.is_copy = TRUE;
                bits.free = free_heap_bits;
            }
        }
        if (!err)
        {
            /* make x,y relative to the image bits */
            x += src.visrect.left - dst.visrect.left;
            y += src.visrect.top - dst.visrect.top;
            render_aa_text_bitmapinfo( dc, info, &bits, &src, x, y, flags,
                                       aa_flags, str, count, dx );
            err = dst_dev->funcs->pPutImage( dst_dev, 0, info, &bits, &src, &dst, SRCCOPY );
            if (bits.free) bits.free( &bits );
            return !err;
        }
    }

    pen = CreatePen( PS_SOLID, 1, dc->textColor );
    orig = SelectObject( dev->hdc, pen );

    for (i = 0; i < count; i++)
    {
        GLYPHMETRICS metrics;
        struct gdi_image_bits image;

        err = get_glyph_bitmap( dev->hdc, str[i], flags, GGO_BITMAP, &metrics, &image );
        if (err) continue;

        if (image.ptr) draw_glyph( dc, x, y, &metrics, &image, (flags & ETO_CLIPPED) ? rect : NULL );
        if (image.free) image.free( &image );

        if (dx)
        {
            if (flags & ETO_PDY)
            {
                x += dx[ i * 2 ];
                y += dx[ i * 2 + 1];
            }
            else x += dx[ i ];
        }
        else
        {
            x += metrics.gmCellIncX;
            y += metrics.gmCellIncY;
        }
    }

    SelectObject( dev->hdc, orig );
    DeleteObject( pen );
    return TRUE;
}


/***********************************************************************
 *           ExtTextOutA    (GDI32.@)
 *
 * See ExtTextOutW.
 */
BOOL WINAPI ExtTextOutA( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCSTR str, UINT count, const INT *lpDx )
{
    INT wlen;
    UINT codepage;
    LPWSTR p;
    BOOL ret;
    LPINT lpDxW = NULL;

    if (count > INT_MAX) return FALSE;

    if (flags & ETO_GLYPH_INDEX)
        return ExtTextOutW( hdc, x, y, flags, lprect, (LPCWSTR)str, count, lpDx );

    p = FONT_mbtowc(hdc, str, count, &wlen, &codepage);

    if (lpDx) {
        unsigned int i = 0, j = 0;

        /* allocate enough for a ETO_PDY */
        lpDxW = HeapAlloc( GetProcessHeap(), 0, 2*wlen*sizeof(INT));
        while(i < count) {
            if(IsDBCSLeadByteEx(codepage, str[i]))
            {
                if(flags & ETO_PDY)
                {
                    lpDxW[j++] = lpDx[i * 2]     + lpDx[(i + 1) * 2];
                    lpDxW[j++] = lpDx[i * 2 + 1] + lpDx[(i + 1) * 2 + 1];
                }
                else
                    lpDxW[j++] = lpDx[i] + lpDx[i + 1];
                i = i + 2;
            }
            else
            {
                if(flags & ETO_PDY)
                {
                    lpDxW[j++] = lpDx[i * 2];
                    lpDxW[j++] = lpDx[i * 2 + 1];
                }
                else
                    lpDxW[j++] = lpDx[i];
                i = i + 1;
            }
        }
    }

    ret = ExtTextOutW( hdc, x, y, flags, lprect, p, wlen, lpDxW );

    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, lpDxW );
    return ret;
}

/***********************************************************************
 *           get_line_width
 *
 * Scale the underline / strikeout line width.
 */
static inline int get_line_width( DC *dc, int metric_size )
{
    int width = abs( INTERNAL_YWSTODS( dc, metric_size ));
    if (width == 0) width = 1;
    if (metric_size < 0) width = -width;
    return width;
}

/***********************************************************************
 *           ExtTextOutW    (GDI32.@)
 *
 * Draws text using the currently selected font, background color, and text color.
 * 
 * 
 * PARAMS
 *    x,y    [I] coordinates of string
 *    flags  [I]
 *        ETO_GRAYED - undocumented on MSDN
 *        ETO_OPAQUE - use background color for fill the rectangle
 *        ETO_CLIPPED - clipping text to the rectangle
 *        ETO_GLYPH_INDEX - Buffer is of glyph locations in fonts rather
 *                          than encoded characters. Implies ETO_IGNORELANGUAGE
 *        ETO_RTLREADING - Paragraph is basically a right-to-left paragraph.
 *                         Affects BiDi ordering
 *        ETO_IGNORELANGUAGE - Undocumented in MSDN - instructs ExtTextOut not to do BiDi reordering
 *        ETO_PDY - unimplemented
 *        ETO_NUMERICSLATIN - unimplemented always assumed -
 *                            do not translate numbers into locale representations
 *        ETO_NUMERICSLOCAL - unimplemented - Numerals in Arabic/Farsi context should assume local form
 *    lprect [I] dimensions for clipping or/and opaquing
 *    str    [I] text string
 *    count  [I] number of symbols in string
 *    lpDx   [I] optional parameter with distance between drawing characters
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCWSTR str, UINT count, const INT *lpDx )
{
    BOOL ret = FALSE;
    LPWSTR reordered_str = (LPWSTR)str;
    WORD *glyphs = NULL;
    UINT align;
    DWORD layout;
    POINT pt;
    TEXTMETRICW tm;
    LOGFONTW lf;
    double cosEsc, sinEsc;
    INT char_extra;
    SIZE sz;
    RECT rc;
    POINT *deltas = NULL, width = {0, 0};
    DWORD type;
    DC * dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    INT breakRem;
    static int quietfixme = 0;

    if (!dc) return FALSE;
    if (count > INT_MAX) return FALSE;

    align = dc->textAlign;
    breakRem = dc->breakRem;
    layout = dc->layout;

    if (quietfixme == 0 && flags & (ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN))
    {
        FIXME("flags ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN unimplemented\n");
        quietfixme = 1;
    }

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pExtTextOut );
    type = GetObjectType(hdc);
    if(type == OBJ_METADC || type == OBJ_ENHMETADC)
    {
        ret = physdev->funcs->pExtTextOut( physdev, x, y, flags, lprect, str, count, lpDx );
        release_dc_ptr( dc );
        return ret;
    }

    if (flags & ETO_RTLREADING) align |= TA_RTLREADING;
    if (layout & LAYOUT_RTL)
    {
        if ((align & TA_CENTER) != TA_CENTER) align ^= TA_RIGHT;
        align ^= TA_RTLREADING;
    }

    if( !(flags & (ETO_GLYPH_INDEX | ETO_IGNORELANGUAGE)) && count > 0 )
    {
        INT cGlyphs;
        reordered_str = HeapAlloc(GetProcessHeap(), 0, count*sizeof(WCHAR));

        BIDI_Reorder( hdc, str, count, GCP_REORDER,
                      (align & TA_RTLREADING) ? WINE_GCPW_FORCE_RTL : WINE_GCPW_FORCE_LTR,
                      reordered_str, count, NULL, &glyphs, &cGlyphs);

        flags |= ETO_IGNORELANGUAGE;
        if (glyphs)
        {
            flags |= ETO_GLYPH_INDEX;
            if (cGlyphs != count)
                count = cGlyphs;
        }
    }
    else if(flags & ETO_GLYPH_INDEX)
        glyphs = reordered_str;

    TRACE("%p, %d, %d, %08x, %s, %s, %d, %p)\n", hdc, x, y, flags,
          wine_dbgstr_rect(lprect), debugstr_wn(str, count), count, lpDx);
    TRACE("align = %x bkmode = %x mapmode = %x\n", align, dc->backgroundMode, dc->MapMode);

    if(align & TA_UPDATECP)
    {
        pt = dc->cur_pos;
        x = pt.x;
        y = pt.y;
    }

    GetTextMetricsW(hdc, &tm);
    GetObjectW(dc->hFont, sizeof(lf), &lf);

    if(!(tm.tmPitchAndFamily & TMPF_VECTOR)) /* Non-scalable fonts shouldn't be rotated */
        lf.lfEscapement = 0;

    if ((dc->GraphicsMode == GM_COMPATIBLE) &&
        (dc->vport2WorldValid && dc->xformWorld2Vport.eM11 * dc->xformWorld2Vport.eM22 < 0))
    {
        lf.lfEscapement = -lf.lfEscapement;
    }

    if(lf.lfEscapement != 0)
    {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    }
    else
    {
        cosEsc = 1;
        sinEsc = 0;
    }

    if (lprect && (flags & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        rc = *lprect;
        lp_to_dp(dc, (POINT*)&rc, 2);
        order_rect( &rc );
        if (flags & ETO_OPAQUE)
            physdev->funcs->pExtTextOut( physdev, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL );
    }
    else flags &= ~ETO_CLIPPED;

    if(count == 0)
    {
        ret = TRUE;
        goto done;
    }

    pt.x = x;
    pt.y = y;
    lp_to_dp(dc, &pt, 1);
    x = pt.x;
    y = pt.y;

    char_extra = GetTextCharacterExtra(hdc);
    if (char_extra && lpDx && GetDeviceCaps( hdc, TECHNOLOGY ) == DT_RASPRINTER)
        char_extra = 0; /* Printer drivers don't add char_extra if lpDx is supplied */

    if(char_extra || dc->breakExtra || breakRem || lpDx || lf.lfEscapement != 0)
    {
        UINT i;
        POINT total = {0, 0}, desired[2];

        deltas = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*deltas));
        if (lpDx)
        {
            if (flags & ETO_PDY)
            {
                for (i = 0; i < count; i++)
                {
                    deltas[i].x = lpDx[i * 2] + char_extra;
                    deltas[i].y = -lpDx[i * 2 + 1];
                }
            }
            else
            {
                for (i = 0; i < count; i++)
                {
                    deltas[i].x = lpDx[i] + char_extra;
                    deltas[i].y = 0;
                }
            }
        }
        else
        {
            INT *dx = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*dx) );

            if (flags & ETO_GLYPH_INDEX)
                GetTextExtentExPointI( hdc, glyphs, count, -1, NULL, dx, &sz );
            else
                GetTextExtentExPointW( hdc, reordered_str, count, -1, NULL, dx, &sz );

            deltas[0].x = dx[0];
            deltas[0].y = 0;
            for (i = 1; i < count; i++)
            {
                deltas[i].x = dx[i] - dx[i - 1];
                deltas[i].y = 0;
            }
            HeapFree( GetProcessHeap(), 0, dx );
        }

        for(i = 0; i < count; i++)
        {
            total.x += deltas[i].x;
            total.y += deltas[i].y;

            desired[0].x = desired[0].y = 0;

            desired[1].x =  cosEsc * total.x + sinEsc * total.y;
            desired[1].y = -sinEsc * total.x + cosEsc * total.y;

            lp_to_dp(dc, desired, 2);
            desired[1].x -= desired[0].x;
            desired[1].y -= desired[0].y;

            if (dc->GraphicsMode == GM_COMPATIBLE)
            {
                if (dc->vport2WorldValid && dc->xformWorld2Vport.eM11 < 0)
                    desired[1].x = -desired[1].x;
                if (dc->vport2WorldValid && dc->xformWorld2Vport.eM22 < 0)
                    desired[1].y = -desired[1].y;
            }

            deltas[i].x = desired[1].x - width.x;
            deltas[i].y = desired[1].y - width.y;

            width = desired[1];
        }
        flags |= ETO_PDY;
    }
    else
    {
        POINT desired[2];

        if(flags & ETO_GLYPH_INDEX)
            GetTextExtentPointI(hdc, glyphs, count, &sz);
        else
            GetTextExtentPointW(hdc, reordered_str, count, &sz);
        desired[0].x = desired[0].y = 0;
        desired[1].x = sz.cx;
        desired[1].y = 0;
        lp_to_dp(dc, desired, 2);
        desired[1].x -= desired[0].x;
        desired[1].y -= desired[0].y;

        if (dc->GraphicsMode == GM_COMPATIBLE)
        {
            if (dc->vport2WorldValid && dc->xformWorld2Vport.eM11 < 0)
                desired[1].x = -desired[1].x;
            if (dc->vport2WorldValid && dc->xformWorld2Vport.eM22 < 0)
                desired[1].y = -desired[1].y;
        }
        width = desired[1];
    }

    tm.tmAscent = abs(INTERNAL_YWSTODS(dc, tm.tmAscent));
    tm.tmDescent = abs(INTERNAL_YWSTODS(dc, tm.tmDescent));
    switch( align & (TA_LEFT | TA_RIGHT | TA_CENTER) )
    {
    case TA_LEFT:
        if (align & TA_UPDATECP)
        {
            pt.x = x + width.x;
            pt.y = y + width.y;
            dp_to_lp(dc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;

    case TA_CENTER:
        x -= width.x / 2;
        y -= width.y / 2;
        break;

    case TA_RIGHT:
        x -= width.x;
        y -= width.y;
        if (align & TA_UPDATECP)
        {
            pt.x = x;
            pt.y = y;
            dp_to_lp(dc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;
    }

    switch( align & (TA_TOP | TA_BOTTOM | TA_BASELINE) )
    {
    case TA_TOP:
        y += tm.tmAscent * cosEsc;
        x += tm.tmAscent * sinEsc;
        break;

    case TA_BOTTOM:
        y -= tm.tmDescent * cosEsc;
        x -= tm.tmDescent * sinEsc;
        break;

    case TA_BASELINE:
        break;
    }

    if (dc->backgroundMode != TRANSPARENT)
    {
        if(!((flags & ETO_CLIPPED) && (flags & ETO_OPAQUE)))
        {
            if(!(flags & ETO_OPAQUE) || !lprect ||
               x < rc.left || x + width.x >= rc.right ||
               y - tm.tmAscent < rc.top || y + tm.tmDescent >= rc.bottom)
            {
                RECT text_box;
                text_box.left = x;
                text_box.right = x + width.x;
                text_box.top = y - tm.tmAscent;
                text_box.bottom = y + tm.tmDescent;

                if (flags & ETO_CLIPPED) intersect_rect( &text_box, &text_box, &rc );
                if (!is_rect_empty( &text_box ))
                    physdev->funcs->pExtTextOut( physdev, 0, 0, ETO_OPAQUE, &text_box, NULL, 0, NULL );
            }
        }
    }

    ret = physdev->funcs->pExtTextOut( physdev, x, y, (flags & ~ETO_OPAQUE), &rc,
                                       glyphs ? glyphs : reordered_str, count, (INT*)deltas );

done:
    HeapFree(GetProcessHeap(), 0, deltas);
    if(glyphs != reordered_str)
        HeapFree(GetProcessHeap(), 0, glyphs);
    if(reordered_str != str)
        HeapFree(GetProcessHeap(), 0, reordered_str);

    if (ret && (lf.lfUnderline || lf.lfStrikeOut))
    {
        int underlinePos, strikeoutPos;
        int underlineWidth, strikeoutWidth;
        UINT size = GetOutlineTextMetricsW(hdc, 0, NULL);
        OUTLINETEXTMETRICW* otm = NULL;
        POINT pts[5];
        HPEN hpen = SelectObject(hdc, GetStockObject(NULL_PEN));
        HBRUSH hbrush = CreateSolidBrush(dc->textColor);

        hbrush = SelectObject(hdc, hbrush);

        if(!size)
        {
            underlinePos = 0;
            underlineWidth = tm.tmAscent / 20 + 1;
            strikeoutPos = tm.tmAscent / 2;
            strikeoutWidth = underlineWidth;
        }
        else
        {
            otm = HeapAlloc(GetProcessHeap(), 0, size);
            GetOutlineTextMetricsW(hdc, size, otm);
            underlinePos = abs( INTERNAL_YWSTODS( dc, otm->otmsUnderscorePosition ));
            if (otm->otmsUnderscorePosition < 0) underlinePos = -underlinePos;
            underlineWidth = get_line_width( dc, otm->otmsUnderscoreSize );
            strikeoutPos = abs( INTERNAL_YWSTODS( dc, otm->otmsStrikeoutPosition ));
            if (otm->otmsStrikeoutPosition < 0) strikeoutPos = -strikeoutPos;
            strikeoutWidth = get_line_width( dc, otm->otmsStrikeoutSize );
            HeapFree(GetProcessHeap(), 0, otm);
        }


        if (lf.lfUnderline)
        {
            pts[0].x = x - (underlinePos + underlineWidth / 2) * sinEsc;
            pts[0].y = y - (underlinePos + underlineWidth / 2) * cosEsc;
            pts[1].x = x + width.x - (underlinePos + underlineWidth / 2) * sinEsc;
            pts[1].y = y + width.y - (underlinePos + underlineWidth / 2) * cosEsc;
            pts[2].x = pts[1].x + underlineWidth * sinEsc;
            pts[2].y = pts[1].y + underlineWidth * cosEsc;
            pts[3].x = pts[0].x + underlineWidth * sinEsc;
            pts[3].y = pts[0].y + underlineWidth * cosEsc;
            pts[4].x = pts[0].x;
            pts[4].y = pts[0].y;
            dp_to_lp(dc, pts, 5);
            Polygon(hdc, pts, 5);
        }

        if (lf.lfStrikeOut)
        {
            pts[0].x = x - (strikeoutPos + strikeoutWidth / 2) * sinEsc;
            pts[0].y = y - (strikeoutPos + strikeoutWidth / 2) * cosEsc;
            pts[1].x = x + width.x - (strikeoutPos + strikeoutWidth / 2) * sinEsc;
            pts[1].y = y + width.y - (strikeoutPos + strikeoutWidth / 2) * cosEsc;
            pts[2].x = pts[1].x + strikeoutWidth * sinEsc;
            pts[2].y = pts[1].y + strikeoutWidth * cosEsc;
            pts[3].x = pts[0].x + strikeoutWidth * sinEsc;
            pts[3].y = pts[0].y + strikeoutWidth * cosEsc;
            pts[4].x = pts[0].x;
            pts[4].y = pts[0].y;
            dp_to_lp(dc, pts, 5);
            Polygon(hdc, pts, 5);
        }

        SelectObject(hdc, hpen);
        hbrush = SelectObject(hdc, hbrush);
        DeleteObject(hbrush);
    }

    release_dc_ptr( dc );

    return ret;
}


/***********************************************************************
 *           TextOutA    (GDI32.@)
 */
BOOL WINAPI TextOutA( HDC hdc, INT x, INT y, LPCSTR str, INT count )
{
    return ExtTextOutA( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOutW    (GDI32.@)
 */
BOOL WINAPI TextOutW(HDC hdc, INT x, INT y, LPCWSTR str, INT count)
{
    return ExtTextOutW( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *		PolyTextOutA (GDI32.@)
 *
 * See PolyTextOutW.
 */
BOOL WINAPI PolyTextOutA( HDC hdc, const POLYTEXTA *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutA( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}



/***********************************************************************
 *		PolyTextOutW (GDI32.@)
 *
 * Draw several Strings
 *
 * RETURNS
 *  TRUE:  Success.
 *  FALSE: Failure.
 */
BOOL WINAPI PolyTextOutW( HDC hdc, const POLYTEXTW *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutW( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}


/***********************************************************************
 *           SetMapperFlags    (GDI32.@)
 */
DWORD WINAPI SetMapperFlags( HDC hdc, DWORD flags )
{
    DC *dc = get_dc_ptr( hdc );
    DWORD ret = GDI_ERROR;

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetMapperFlags );
        flags = physdev->funcs->pSetMapperFlags( physdev, flags );
        if (flags != GDI_ERROR)
        {
            ret = dc->mapperFlags;
            dc->mapperFlags = flags;
        }
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *          GetAspectRatioFilterEx  (GDI32.@)
 */
BOOL WINAPI GetAspectRatioFilterEx( HDC hdc, LPSIZE pAspectRatio )
{
  FIXME("(%p, %p): -- Empty Stub !\n", hdc, pAspectRatio);
  return FALSE;
}


/***********************************************************************
 *           GetCharABCWidthsA   (GDI32.@)
 *
 * See GetCharABCWidthsW.
 */
BOOL WINAPI GetCharABCWidthsA(HDC hdc, UINT firstChar, UINT lastChar,
                                  LPABC abc )
{
    INT i, wlen;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    str = FONT_GetCharsByRangeA(hdc, firstChar, lastChar, &i);
    if (str == NULL)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, i, &wlen, NULL);
    if (wstr == NULL)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

    for(i = 0; i < wlen; i++)
    {
	if(!GetCharABCWidthsW(hdc, wstr[i], wstr[i], abc))
	{
	    ret = FALSE;
	    break;
	}
	abc++;
    }

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}


/******************************************************************************
 * GetCharABCWidthsW [GDI32.@]
 *
 * Retrieves widths of characters in range.
 *
 * PARAMS
 *    hdc       [I] Handle of device context
 *    firstChar [I] First character in range to query
 *    lastChar  [I] Last character in range to query
 *    abc       [O] Address of character-width structure
 *
 * NOTES
 *    Only works with TrueType fonts
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsW( HDC hdc, UINT firstChar, UINT lastChar,
                                   LPABC abc )
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    unsigned int i;
    BOOL ret;
    TEXTMETRICW tm;

    if (!dc) return FALSE;

    if (!abc)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    /* unlike GetCharABCWidthsFloatW, this one is supposed to fail on non-scalable fonts */
    dev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    if (!dev->funcs->pGetTextMetrics( dev, &tm ) || !(tm.tmPitchAndFamily & TMPF_VECTOR))
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    dev = GET_DC_PHYSDEV( dc, pGetCharABCWidths );
    ret = dev->funcs->pGetCharABCWidths( dev, firstChar, lastChar, abc );
    if (ret)
    {
        /* convert device units to logical */
        for( i = firstChar; i <= lastChar; i++, abc++ ) {
            abc->abcA = width_to_LP(dc, abc->abcA);
            abc->abcB = width_to_LP(dc, abc->abcB);
            abc->abcC = width_to_LP(dc, abc->abcC);
	}
    }

    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * GetCharABCWidthsI [GDI32.@]
 *
 * Retrieves widths of characters in range.
 *
 * PARAMS
 *    hdc       [I] Handle of device context
 *    firstChar [I] First glyphs in range to query
 *    count     [I] Last glyphs in range to query
 *    pgi       [i] Array of glyphs to query
 *    abc       [O] Address of character-width structure
 *
 * NOTES
 *    Only works with TrueType fonts
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsI( HDC hdc, UINT firstChar, UINT count,
                               LPWORD pgi, LPABC abc)
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    unsigned int i;
    BOOL ret;

    if (!dc) return FALSE;

    if (!abc)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    dev = GET_DC_PHYSDEV( dc, pGetCharABCWidthsI );
    ret = dev->funcs->pGetCharABCWidthsI( dev, firstChar, count, pgi, abc );
    if (ret)
    {
        /* convert device units to logical */
        for( i = 0; i < count; i++, abc++ ) {
            abc->abcA = width_to_LP(dc, abc->abcA);
            abc->abcB = width_to_LP(dc, abc->abcB);
            abc->abcC = width_to_LP(dc, abc->abcC);
	}
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetGlyphOutlineA    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineA( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    if (!lpmat2) return GDI_ERROR;

    if(!(fuFormat & GGO_GLYPH_INDEX)) {
        UINT cp;
        int len;
        char mbchs[2];
        WCHAR wChar;

        cp = GdiGetCodePage(hdc);
        if (IsDBCSLeadByteEx(cp, uChar >> 8)) {
            len = 2;
            mbchs[0] = (uChar & 0xff00) >> 8;
            mbchs[1] = (uChar & 0xff);
        } else {
            len = 1;
            mbchs[0] = (uChar & 0xff);
        }
        wChar = 0;
        MultiByteToWideChar(cp, 0, mbchs, len, &wChar, 1);
        uChar = wChar;
    }

    return GetGlyphOutlineW(hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer,
                            lpmat2);
}

/***********************************************************************
 *           GetGlyphOutlineW    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineW( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    DC *dc;
    DWORD ret;
    PHYSDEV dev;

    TRACE("(%p, %04x, %04x, %p, %d, %p, %p)\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );

    if (!lpmat2) return GDI_ERROR;

    dc = get_dc_ptr(hdc);
    if(!dc) return GDI_ERROR;

    uChar &= 0xffff;

    dev = GET_DC_PHYSDEV( dc, pGetGlyphOutline );
    ret = dev->funcs->pGetGlyphOutline( dev, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           CreateScalableFontResourceA   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceA( DWORD fHidden,
                                             LPCSTR lpszResourceFile,
                                             LPCSTR lpszFontFile,
                                             LPCSTR lpszCurrentPath )
{
    LPWSTR lpszResourceFileW = NULL;
    LPWSTR lpszFontFileW = NULL;
    LPWSTR lpszCurrentPathW = NULL;
    int len;
    BOOL ret;

    if (lpszResourceFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszResourceFile, -1, NULL, 0);
        lpszResourceFileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszResourceFile, -1, lpszResourceFileW, len);
    }

    if (lpszFontFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszFontFile, -1, NULL, 0);
        lpszFontFileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszFontFile, -1, lpszFontFileW, len);
    }

    if (lpszCurrentPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszCurrentPath, -1, NULL, 0);
        lpszCurrentPathW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszCurrentPath, -1, lpszCurrentPathW, len);
    }

    ret = CreateScalableFontResourceW(fHidden, lpszResourceFileW,
            lpszFontFileW, lpszCurrentPathW);

    HeapFree(GetProcessHeap(), 0, lpszResourceFileW);
    HeapFree(GetProcessHeap(), 0, lpszFontFileW);
    HeapFree(GetProcessHeap(), 0, lpszCurrentPathW);

    return ret;
}

#define NE_FFLAGS_LIBMODULE     0x8000
#define NE_OSFLAGS_WINDOWS      0x02

static const char dos_string[0x40] = "This is a TrueType resource file";
static const char FONTRES[] = {'F','O','N','T','R','E','S',':'};

#include <pshpack1.h>
struct fontdir
{
    WORD   num_of_resources;
    WORD   res_id;
    WORD   dfVersion;
    DWORD  dfSize;
    CHAR   dfCopyright[60];
    WORD   dfType;
    WORD   dfPoints;
    WORD   dfVertRes;
    WORD   dfHorizRes;
    WORD   dfAscent;
    WORD   dfInternalLeading;
    WORD   dfExternalLeading;
    BYTE   dfItalic;
    BYTE   dfUnderline;
    BYTE   dfStrikeOut;
    WORD   dfWeight;
    BYTE   dfCharSet;
    WORD   dfPixWidth;
    WORD   dfPixHeight;
    BYTE   dfPitchAndFamily;
    WORD   dfAvgWidth;
    WORD   dfMaxWidth;
    BYTE   dfFirstChar;
    BYTE   dfLastChar;
    BYTE   dfDefaultChar;
    BYTE   dfBreakChar;
    WORD   dfWidthBytes;
    DWORD  dfDevice;
    DWORD  dfFace;
    DWORD  dfReserved;
    CHAR   szFaceName[LF_FACESIZE];
};
#include <poppack.h>

#include <pshpack2.h>

struct ne_typeinfo
{
    WORD type_id;
    WORD count;
    DWORD res;
};

struct ne_nameinfo
{
    WORD off;
    WORD len;
    WORD flags;
    WORD id;
    DWORD res;
};

struct rsrc_tab
{
    WORD align;
    struct ne_typeinfo fontdir_type;
    struct ne_nameinfo fontdir_name;
    struct ne_typeinfo scalable_type;
    struct ne_nameinfo scalable_name;
    WORD end_of_rsrc;
    BYTE fontdir_res_name[8];
};

#include <poppack.h>

static BOOL create_fot( const WCHAR *resource, const WCHAR *font_file, const struct fontdir *fontdir )
{
    BOOL ret = FALSE;
    HANDLE file;
    DWORD size, written;
    BYTE *ptr, *start;
    BYTE import_name_len, res_name_len, non_res_name_len, font_file_len;
    char *font_fileA, *last_part, *ext;
    IMAGE_DOS_HEADER dos;
    IMAGE_OS2_HEADER ne =
    {
        IMAGE_OS2_SIGNATURE, 5, 1, 0, 0, 0, NE_FFLAGS_LIBMODULE, 0,
        0, 0, 0, 0, 0, 0,
        0, sizeof(ne), sizeof(ne), 0, 0, 0, 0,
        0, 4, 2, NE_OSFLAGS_WINDOWS, 0, 0, 0, 0, 0x300
    };
    struct rsrc_tab rsrc_tab =
    {
        4,
        { 0x8007, 1, 0 },
        { 0, 0, 0x0c50, 0x2c, 0 },
        { 0x80cc, 1, 0 },
        { 0, 0, 0x0c50, 0x8001, 0 },
        0,
        { 7,'F','O','N','T','D','I','R'}
    };

    memset( &dos, 0, sizeof(dos) );
    dos.e_magic = IMAGE_DOS_SIGNATURE;
    dos.e_lfanew = sizeof(dos) + sizeof(dos_string);

    /* import name is last part\0, resident name is last part without extension
       non-resident name is "FONTRES:" + lfFaceName */

    font_file_len = WideCharToMultiByte( CP_ACP, 0, font_file, -1, NULL, 0, NULL, NULL );
    font_fileA = HeapAlloc( GetProcessHeap(), 0, font_file_len );
    WideCharToMultiByte( CP_ACP, 0, font_file, -1, font_fileA, font_file_len, NULL, NULL );

    last_part = strrchr( font_fileA, '\\' );
    if (last_part) last_part++;
    else last_part = font_fileA;
    import_name_len = strlen( last_part ) + 1;

    ext = strchr( last_part, '.' );
    if (ext) res_name_len = ext - last_part;
    else res_name_len = import_name_len - 1;

    non_res_name_len = sizeof( FONTRES ) + strlen( fontdir->szFaceName );

    ne.ne_cbnrestab = 1 + non_res_name_len + 2 + 1; /* len + string + (WORD) ord_num + 1 byte eod */
    ne.ne_restab = ne.ne_rsrctab + sizeof(rsrc_tab);
    ne.ne_modtab = ne.ne_imptab = ne.ne_restab + 1 + res_name_len + 2 + 3; /* len + string + (WORD) ord_num + 3 bytes eod */
    ne.ne_enttab = ne.ne_imptab + 1 + import_name_len; /* len + string */
    ne.ne_cbenttab = 2;
    ne.ne_nrestab = ne.ne_enttab + ne.ne_cbenttab + 2 + dos.e_lfanew; /* there are 2 bytes of 0 after entry tab */

    rsrc_tab.scalable_name.off = (ne.ne_nrestab + ne.ne_cbnrestab + 0xf) >> 4;
    rsrc_tab.scalable_name.len = (font_file_len + 0xf) >> 4;
    rsrc_tab.fontdir_name.off  = rsrc_tab.scalable_name.off + rsrc_tab.scalable_name.len;
    rsrc_tab.fontdir_name.len  = (fontdir->dfSize + 0xf) >> 4;

    size = (rsrc_tab.fontdir_name.off + rsrc_tab.fontdir_name.len) << 4;
    start = ptr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );

    if (!ptr)
    {
        HeapFree( GetProcessHeap(), 0, font_fileA );
        return FALSE;
    }

    memcpy( ptr, &dos, sizeof(dos) );
    memcpy( ptr + sizeof(dos), dos_string, sizeof(dos_string) );
    memcpy( ptr + dos.e_lfanew, &ne, sizeof(ne) );

    ptr = start + dos.e_lfanew + ne.ne_rsrctab;
    memcpy( ptr, &rsrc_tab, sizeof(rsrc_tab) );

    ptr = start + dos.e_lfanew + ne.ne_restab;
    *ptr++ = res_name_len;
    memcpy( ptr, last_part, res_name_len );

    ptr = start + dos.e_lfanew + ne.ne_imptab;
    *ptr++ = import_name_len;
    memcpy( ptr, last_part, import_name_len );

    ptr = start + ne.ne_nrestab;
    *ptr++ = non_res_name_len;
    memcpy( ptr, FONTRES, sizeof(FONTRES) );
    memcpy( ptr + sizeof(FONTRES), fontdir->szFaceName, strlen( fontdir->szFaceName ) );

    ptr = start + (rsrc_tab.scalable_name.off << 4);
    memcpy( ptr, font_fileA, font_file_len );

    ptr = start + (rsrc_tab.fontdir_name.off << 4);
    memcpy( ptr, fontdir, fontdir->dfSize );

    file = CreateFileW( resource, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    if (file != INVALID_HANDLE_VALUE)
    {
        if (WriteFile( file, start, size, &written, NULL ) && written == size)
            ret = TRUE;
        CloseHandle( file );
    }

    HeapFree( GetProcessHeap(), 0, start );
    HeapFree( GetProcessHeap(), 0, font_fileA );

    return ret;
}

/***********************************************************************
 *           CreateScalableFontResourceW   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceW( DWORD hidden, LPCWSTR resource_file,
                                         LPCWSTR font_file, LPCWSTR font_path )
{
    struct fontdir fontdir = { 0 };
    struct gdi_font *font = NULL;
    WCHAR path[MAX_PATH];

    TRACE("(%d, %s, %s, %s)\n", hidden, debugstr_w(resource_file),
          debugstr_w(font_file), debugstr_w(font_path) );

    if (!font_funcs) return FALSE;

    if (!font_file) goto done;
    if (font_path && font_path[0])
    {
        int len = lstrlenW( font_path ) + lstrlenW( font_file ) + 2;
        if (len > MAX_PATH) goto done;
        lstrcpynW( path, font_path, MAX_PATH );
        lstrcatW( path, L"\\" );
        lstrcatW( path, font_file );
    }
    else if (!GetFullPathNameW( font_file, MAX_PATH, path, NULL )) goto done;

    if (!(font = alloc_gdi_font( path, NULL, 0 ))) goto done;
    font->lf.lfHeight = 100;
    if (!font_funcs->load_font( font )) goto done;
    if (!font_funcs->set_outline_text_metrics( font )) goto done;

    if (!(font->otm.otmTextMetrics.tmPitchAndFamily & TMPF_TRUETYPE)) goto done;

    fontdir.num_of_resources  = 1;
    fontdir.res_id            = 0;
    fontdir.dfVersion         = 0x200;
    fontdir.dfSize            = sizeof(fontdir);
    strcpy( fontdir.dfCopyright, "Wine fontdir" );
    fontdir.dfType            = 0x4003;  /* 0x0080 set if private */
    fontdir.dfPoints          = font->otm.otmEMSquare;
    fontdir.dfVertRes         = 72;
    fontdir.dfHorizRes        = 72;
    fontdir.dfAscent          = font->otm.otmTextMetrics.tmAscent;
    fontdir.dfInternalLeading = font->otm.otmTextMetrics.tmInternalLeading;
    fontdir.dfExternalLeading = font->otm.otmTextMetrics.tmExternalLeading;
    fontdir.dfItalic          = font->otm.otmTextMetrics.tmItalic;
    fontdir.dfUnderline       = font->otm.otmTextMetrics.tmUnderlined;
    fontdir.dfStrikeOut       = font->otm.otmTextMetrics.tmStruckOut;
    fontdir.dfWeight          = font->otm.otmTextMetrics.tmWeight;
    fontdir.dfCharSet         = font->otm.otmTextMetrics.tmCharSet;
    fontdir.dfPixWidth        = 0;
    fontdir.dfPixHeight       = font->otm.otmTextMetrics.tmHeight;
    fontdir.dfPitchAndFamily  = font->otm.otmTextMetrics.tmPitchAndFamily;
    fontdir.dfAvgWidth        = font->otm.otmTextMetrics.tmAveCharWidth;
    fontdir.dfMaxWidth        = font->otm.otmTextMetrics.tmMaxCharWidth;
    fontdir.dfFirstChar       = font->otm.otmTextMetrics.tmFirstChar;
    fontdir.dfLastChar        = font->otm.otmTextMetrics.tmLastChar;
    fontdir.dfDefaultChar     = font->otm.otmTextMetrics.tmDefaultChar;
    fontdir.dfBreakChar       = font->otm.otmTextMetrics.tmBreakChar;
    fontdir.dfWidthBytes      = 0;
    fontdir.dfDevice          = 0;
    fontdir.dfFace            = FIELD_OFFSET( struct fontdir, szFaceName );
    fontdir.dfReserved        = 0;
    WideCharToMultiByte( CP_ACP, 0, (WCHAR *)font->otm.otmpFamilyName, -1,
                         fontdir.szFaceName, LF_FACESIZE, NULL, NULL );
    free_gdi_font( font );

    if (hidden) fontdir.dfType |= 0x80;
    return create_fot( resource_file, font_file, &fontdir );

done:
    if (font) free_gdi_font( font );
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/*************************************************************************
 *             GetKerningPairsA   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsA( HDC hDC, DWORD cPairs,
                               LPKERNINGPAIR kern_pairA )
{
    UINT cp;
    CPINFO cpi;
    DWORD i, total_kern_pairs, kern_pairs_copied = 0;
    KERNINGPAIR *kern_pairW;

    if (!cPairs && kern_pairA)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    cp = GdiGetCodePage(hDC);

    /* GetCPInfo() will fail on CP_SYMBOL, and WideCharToMultiByte is supposed
     * to fail on an invalid character for CP_SYMBOL.
     */
    cpi.DefaultChar[0] = 0;
    if (cp != CP_SYMBOL && !GetCPInfo(cp, &cpi))
    {
        FIXME("Can't find codepage %u info\n", cp);
        return 0;
    }

    total_kern_pairs = GetKerningPairsW(hDC, 0, NULL);
    if (!total_kern_pairs) return 0;

    kern_pairW = HeapAlloc(GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pairW));
    GetKerningPairsW(hDC, total_kern_pairs, kern_pairW);

    for (i = 0; i < total_kern_pairs; i++)
    {
        char first, second;

        if (!WideCharToMultiByte(cp, 0, &kern_pairW[i].wFirst, 1, &first, 1, NULL, NULL))
            continue;

        if (!WideCharToMultiByte(cp, 0, &kern_pairW[i].wSecond, 1, &second, 1, NULL, NULL))
            continue;

        if (first == cpi.DefaultChar[0] || second == cpi.DefaultChar[0])
            continue;

        if (kern_pairA)
        {
            if (kern_pairs_copied >= cPairs) break;

            kern_pairA->wFirst = (BYTE)first;
            kern_pairA->wSecond = (BYTE)second;
            kern_pairA->iKernAmount = kern_pairW[i].iKernAmount;
            kern_pairA++;
        }
        kern_pairs_copied++;
    }

    HeapFree(GetProcessHeap(), 0, kern_pairW);

    return kern_pairs_copied;
}

/*************************************************************************
 *             GetKerningPairsW   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsW( HDC hDC, DWORD cPairs,
                                 LPKERNINGPAIR lpKerningPairs )
{
    DC *dc;
    DWORD ret;
    PHYSDEV dev;

    TRACE("(%p,%d,%p)\n", hDC, cPairs, lpKerningPairs);

    if (!cPairs && lpKerningPairs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    dc = get_dc_ptr(hDC);
    if (!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetKerningPairs );
    ret = dev->funcs->pGetKerningPairs( dev, cPairs, lpKerningPairs );
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.@]
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondence between different labels
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges)
 * of the same encoding.
 *
 * Only one codepage will be set in lpCs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *lpSrc.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
BOOL WINAPI TranslateCharsetInfo(
  LPDWORD lpSrc, /* [in]
       if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
       if flags == TCI_SRCCHARSET: a character set value
       if flags == TCI_SRCCODEPAGE: a code page value
		 */
  LPCHARSETINFO lpCs, /* [out] structure to receive charset information */
  DWORD flags /* [in] determines interpretation of lpSrc */)
{
    int index = 0;
    switch (flags) {
    case TCI_SRCFONTSIG:
      while (index < MAXTCIINDEX && !(*lpSrc>>index & 0x0001)) index++;
      break;
    case TCI_SRCCODEPAGE:
      while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciACP) index++;
      break;
    case TCI_SRCCHARSET:
      while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciCharset) index++;
      break;
    default:
      return FALSE;
    }
    if (index >= MAXTCIINDEX || FONT_tci[index].ciCharset == DEFAULT_CHARSET) return FALSE;
    *lpCs = FONT_tci[index];
    return TRUE;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI32.@)
 */
DWORD WINAPI GetFontLanguageInfo(HDC hdc)
{
	FONTSIGNATURE fontsig;
	static const DWORD GCP_DBCS_MASK=FS_JISJAPAN|FS_CHINESESIMP|FS_WANSUNG|FS_CHINESETRAD|FS_JOHAB,
		GCP_DIACRITIC_MASK=0x00000000,
		FLI_GLYPHS_MASK=0x00000000,
		GCP_GLYPHSHAPE_MASK=FS_ARABIC,
		GCP_KASHIDA_MASK=0x00000000,
		GCP_LIGATE_MASK=0x00000000,
		GCP_REORDER_MASK=FS_HEBREW|FS_ARABIC;

	DWORD result=0;

	GetTextCharsetInfo( hdc, &fontsig, 0 );
	/* We detect each flag we return using a bitmask on the Codepage Bitfields */

	if( (fontsig.fsCsb[0]&GCP_DBCS_MASK)!=0 )
		result|=GCP_DBCS;

	if( (fontsig.fsCsb[0]&GCP_DIACRITIC_MASK)!=0 )
		result|=GCP_DIACRITIC;

	if( (fontsig.fsCsb[0]&FLI_GLYPHS_MASK)!=0 )
		result|=FLI_GLYPHS;

	if( (fontsig.fsCsb[0]&GCP_GLYPHSHAPE_MASK)!=0 )
		result|=GCP_GLYPHSHAPE;

	if( (fontsig.fsCsb[0]&GCP_KASHIDA_MASK)!=0 )
		result|=GCP_KASHIDA;

	if( (fontsig.fsCsb[0]&GCP_LIGATE_MASK)!=0 )
		result|=GCP_LIGATE;

	if( GetKerningPairsW( hdc, 0, NULL ) )
		result|=GCP_USEKERNING;

        /* this might need a test for a HEBREW- or ARABIC_CHARSET as well */
        if( GetTextAlign( hdc) & TA_RTLREADING )
            if( (fontsig.fsCsb[0]&GCP_REORDER_MASK)!=0 )
                    result|=GCP_REORDER;

	return result;
}


/*************************************************************************
 * GetFontData [GDI32.@]
 *
 * Retrieve data for TrueType font.
 *
 * RETURNS
 *
 * success: Number of bytes returned
 * failure: GDI_ERROR
 *
 * NOTES
 *
 * Calls SetLastError()
 *
 */
DWORD WINAPI GetFontData(HDC hdc, DWORD table, DWORD offset,
    LPVOID buffer, DWORD length)
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    DWORD ret;

    if(!dc) return GDI_ERROR;

    dev = GET_DC_PHYSDEV( dc, pGetFontData );
    ret = dev->funcs->pGetFontData( dev, table, offset, buffer, length );
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * GetGlyphIndicesA [GDI32.@]
 */
DWORD WINAPI GetGlyphIndicesA(HDC hdc, LPCSTR lpstr, INT count,
			      LPWORD pgi, DWORD flags)
{
    DWORD ret;
    WCHAR *lpstrW;
    INT countW;

    TRACE("(%p, %s, %d, %p, 0x%x)\n",
          hdc, debugstr_an(lpstr, count), count, pgi, flags);

    lpstrW = FONT_mbtowc(hdc, lpstr, count, &countW, NULL);
    ret = GetGlyphIndicesW(hdc, lpstrW, countW, pgi, flags);
    HeapFree(GetProcessHeap(), 0, lpstrW);

    return ret;
}

/*************************************************************************
 * GetGlyphIndicesW [GDI32.@]
 */
DWORD WINAPI GetGlyphIndicesW(HDC hdc, LPCWSTR lpstr, INT count,
			      LPWORD pgi, DWORD flags)
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    DWORD ret;

    TRACE("(%p, %s, %d, %p, 0x%x)\n",
          hdc, debugstr_wn(lpstr, count), count, pgi, flags);

    if(!dc) return GDI_ERROR;

    dev = GET_DC_PHYSDEV( dc, pGetGlyphIndices );
    ret = dev->funcs->pGetGlyphIndices( dev, lpstr, count, pgi, flags );
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * GetCharacterPlacementA [GDI32.@]
 *
 * See GetCharacterPlacementW.
 *
 * NOTES:
 *  the web browser control of ie4 calls this with dwFlags=0
 */
DWORD WINAPI
GetCharacterPlacementA(HDC hdc, LPCSTR lpString, INT uCount,
			 INT nMaxExtent, GCP_RESULTSA *lpResults,
			 DWORD dwFlags)
{
    WCHAR *lpStringW;
    INT uCountW;
    GCP_RESULTSW resultsW;
    DWORD ret;
    UINT font_cp;

    TRACE("%s, %d, %d, 0x%08x\n",
          debugstr_an(lpString, uCount), uCount, nMaxExtent, dwFlags);

    lpStringW = FONT_mbtowc(hdc, lpString, uCount, &uCountW, &font_cp);

    if (!lpResults)
    {
        ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, NULL, dwFlags);
        HeapFree(GetProcessHeap(), 0, lpStringW);
        return ret;
    }

    /* both structs are equal in size */
    memcpy(&resultsW, lpResults, sizeof(resultsW));

    if(lpResults->lpOutString)
        resultsW.lpOutString = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*uCountW);

    ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, &resultsW, dwFlags);

    lpResults->nGlyphs = resultsW.nGlyphs;
    lpResults->nMaxFit = resultsW.nMaxFit;

    if(lpResults->lpOutString) {
        WideCharToMultiByte(font_cp, 0, resultsW.lpOutString, uCountW,
                            lpResults->lpOutString, uCount, NULL, NULL );
    }

    HeapFree(GetProcessHeap(), 0, lpStringW);
    HeapFree(GetProcessHeap(), 0, resultsW.lpOutString);

    return ret;
}

static int kern_pair(const KERNINGPAIR *kern, int count, WCHAR c1, WCHAR c2)
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (kern[i].wFirst == c1 && kern[i].wSecond == c2)
            return kern[i].iKernAmount;
    }

    return 0;
}

static int *kern_string(HDC hdc, const WCHAR *str, int len, int *kern_total)
{
    int i, count;
    KERNINGPAIR *kern = NULL;
    int *ret;

    *kern_total = 0;

    ret = heap_alloc(len * sizeof(*ret));
    if (!ret) return NULL;

    count = GetKerningPairsW(hdc, 0, NULL);
    if (count)
    {
        kern = heap_alloc(count * sizeof(*kern));
        if (!kern)
        {
            heap_free(ret);
            return NULL;
        }

        GetKerningPairsW(hdc, count, kern);
    }

    for (i = 0; i < len - 1; i++)
    {
        ret[i] = kern_pair(kern, count, str[i], str[i + 1]);
        *kern_total += ret[i];
    }

    ret[len - 1] = 0; /* no kerning for last element */

    heap_free(kern);
    return ret;
}

/*************************************************************************
 * GetCharacterPlacementW [GDI32.@]
 *
 *   Retrieve information about a string. This includes the width, reordering,
 *   Glyphing and so on.
 *
 * RETURNS
 *
 *   The width and height of the string if successful, 0 if failed.
 *
 * BUGS
 *
 *   All flags except GCP_REORDER are not yet implemented.
 *   Reordering is not 100% compliant to the Windows BiDi method.
 *   Caret positioning is not yet implemented for BiDi.
 *   Classes are not yet implemented.
 *
 */
DWORD WINAPI
GetCharacterPlacementW(
        HDC hdc,                    /* [in] Device context for which the rendering is to be done */
        LPCWSTR lpString,           /* [in] The string for which information is to be returned */
        INT uCount,                 /* [in] Number of WORDS in string. */
        INT nMaxExtent,             /* [in] Maximum extent the string is to take (in HDC logical units) */
        GCP_RESULTSW *lpResults,    /* [in/out] A pointer to a GCP_RESULTSW struct */
        DWORD dwFlags               /* [in] Flags specifying how to process the string */
        )
{
    DWORD ret=0;
    SIZE size;
    UINT i, nSet;
    int *kern = NULL, kern_total = 0;

    TRACE("%s, %d, %d, 0x%08x\n",
          debugstr_wn(lpString, uCount), uCount, nMaxExtent, dwFlags);

    if (!uCount)
        return 0;

    if (!lpResults)
        return GetTextExtentPoint32W(hdc, lpString, uCount, &size) ? MAKELONG(size.cx, size.cy) : 0;

    TRACE("lStructSize=%d, lpOutString=%p, lpOrder=%p, lpDx=%p, lpCaretPos=%p\n"
          "lpClass=%p, lpGlyphs=%p, nGlyphs=%u, nMaxFit=%d\n",
          lpResults->lStructSize, lpResults->lpOutString, lpResults->lpOrder,
          lpResults->lpDx, lpResults->lpCaretPos, lpResults->lpClass,
          lpResults->lpGlyphs, lpResults->nGlyphs, lpResults->nMaxFit);

    if (dwFlags & ~(GCP_REORDER | GCP_USEKERNING))
        FIXME("flags 0x%08x ignored\n", dwFlags);
    if (lpResults->lpClass)
        FIXME("classes not implemented\n");
    if (lpResults->lpCaretPos && (dwFlags & GCP_REORDER))
        FIXME("Caret positions for complex scripts not implemented\n");

    nSet = (UINT)uCount;
    if (nSet > lpResults->nGlyphs)
        nSet = lpResults->nGlyphs;

    /* return number of initialized fields */
    lpResults->nGlyphs = nSet;

    if (!(dwFlags & GCP_REORDER))
    {
        /* Treat the case where no special handling was requested in a fastpath way */
        /* copy will do if the GCP_REORDER flag is not set */
        if (lpResults->lpOutString)
            memcpy( lpResults->lpOutString, lpString, nSet * sizeof(WCHAR));

        if (lpResults->lpOrder)
        {
            for (i = 0; i < nSet; i++)
                lpResults->lpOrder[i] = i;
        }
    }
    else
    {
        BIDI_Reorder(NULL, lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpResults->lpOutString,
                     nSet, lpResults->lpOrder, NULL, NULL );
    }

    if (dwFlags & GCP_USEKERNING)
    {
        kern = kern_string(hdc, lpString, nSet, &kern_total);
        if (!kern)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }

    /* FIXME: Will use the placement chars */
    if (lpResults->lpDx)
    {
        int c;
        for (i = 0; i < nSet; i++)
        {
            if (GetCharWidth32W(hdc, lpString[i], lpString[i], &c))
            {
                lpResults->lpDx[i] = c;
                if (dwFlags & GCP_USEKERNING)
                    lpResults->lpDx[i] += kern[i];
            }
        }
    }

    if (lpResults->lpCaretPos && !(dwFlags & GCP_REORDER))
    {
        int pos = 0;

        lpResults->lpCaretPos[0] = 0;
        for (i = 0; i < nSet - 1; i++)
        {
            if (dwFlags & GCP_USEKERNING)
                pos += kern[i];

            if (GetTextExtentPoint32W(hdc, &lpString[i], 1, &size))
                lpResults->lpCaretPos[i + 1] = (pos += size.cx);
        }
    }

    if (lpResults->lpGlyphs)
        GetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);

    if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
        ret = MAKELONG(size.cx + kern_total, size.cy);

    heap_free(kern);

    return ret;
}

/*************************************************************************
 *      GetCharABCWidthsFloatA [GDI32.@]
 *
 * See GetCharABCWidthsFloatW.
 */
BOOL WINAPI GetCharABCWidthsFloatA( HDC hdc, UINT first, UINT last, LPABCFLOAT abcf )
{
    INT i, wlen;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    str = FONT_GetCharsByRangeA(hdc, first, last, &i);
    if (str == NULL)
        return FALSE;

    wstr = FONT_mbtowc( hdc, str, i, &wlen, NULL );

    for (i = 0; i < wlen; i++)
    {
        if (!GetCharABCWidthsFloatW( hdc, wstr[i], wstr[i], abcf ))
        {
            ret = FALSE;
            break;
        }
        abcf++;
    }

    HeapFree( GetProcessHeap(), 0, str );
    HeapFree( GetProcessHeap(), 0, wstr );

    return ret;
}

/*************************************************************************
 *      GetCharABCWidthsFloatW [GDI32.@]
 *
 * Retrieves widths of a range of characters.
 *
 * PARAMS
 *    hdc   [I] Handle to device context.
 *    first [I] First character in range to query.
 *    last  [I] Last character in range to query.
 *    abcf  [O] Array of LPABCFLOAT structures.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsFloatW( HDC hdc, UINT first, UINT last, LPABCFLOAT abcf )
{
    UINT i;
    ABC *abc;
    PHYSDEV dev;
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p, %d, %d, %p\n", hdc, first, last, abcf);

    if (!dc) return FALSE;

    if (!abcf) goto done;
    if (!(abc = HeapAlloc( GetProcessHeap(), 0, (last - first + 1) * sizeof(*abc) ))) goto done;

    dev = GET_DC_PHYSDEV( dc, pGetCharABCWidths );
    ret = dev->funcs->pGetCharABCWidths( dev, first, last, abc );
    if (ret)
    {
        /* convert device units to logical */
        FLOAT scale = fabs( dc->xformVport2World.eM11 );
        for (i = first; i <= last; i++, abcf++)
        {
            abcf->abcfA = abc[i - first].abcA * scale;
            abcf->abcfB = abc[i - first].abcB * scale;
            abcf->abcfC = abc[i - first].abcC * scale;
        }
    }
    HeapFree( GetProcessHeap(), 0, abc );

done:
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 *      GetCharWidthFloatA [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatA( HDC hdc, UINT first, UINT last, float *buffer )
{
    WCHAR *wstr;
    int i, wlen;
    char *str;

    if (!(str = FONT_GetCharsByRangeA( hdc, first, last, &i )))
        return FALSE;
    wstr = FONT_mbtowc( hdc, str, i, &wlen, NULL );
    heap_free(str);

    for (i = 0; i < wlen; ++i)
    {
        if (!GetCharWidthFloatW( hdc, wstr[i], wstr[i], &buffer[i] ))
        {
            heap_free(wstr);
            return FALSE;
        }
    }
    heap_free(wstr);
    return TRUE;
}

/*************************************************************************
 *      GetCharWidthFloatW [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatW( HDC hdc, UINT first, UINT last, float *buffer )
{
    DC *dc = get_dc_ptr( hdc );
    int *ibuffer;
    PHYSDEV dev;
    BOOL ret;
    UINT i;

    TRACE("dc %p, first %#x, last %#x, buffer %p\n", dc, first, last, buffer);

    if (!dc) return FALSE;

    if (!(ibuffer = heap_alloc( (last - first + 1) * sizeof(int) )))
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    dev = GET_DC_PHYSDEV( dc, pGetCharWidth );
    if ((ret = dev->funcs->pGetCharWidth( dev, first, last, ibuffer )))
    {
        float scale = fabs( dc->xformVport2World.eM11 ) / 16.0f;
        for (i = first; i <= last; ++i)
            buffer[i - first] = ibuffer[i - first] * scale;
    }

    heap_free(ibuffer);
    return ret;
}

/***********************************************************************
 *								       *
 *           Font Resource API					       *
 *								       *
 ***********************************************************************/

/***********************************************************************
 *           AddFontResourceA    (GDI32.@)
 */
INT WINAPI AddFontResourceA( LPCSTR str )
{
    return AddFontResourceExA( str, 0, NULL);
}

/***********************************************************************
 *           AddFontResourceW    (GDI32.@)
 */
INT WINAPI AddFontResourceW( LPCWSTR str )
{
    return AddFontResourceExW(str, 0, NULL);
}


/***********************************************************************
 *           AddFontResourceExA    (GDI32.@)
 */
INT WINAPI AddFontResourceExA( LPCSTR str, DWORD fl, PVOID pdv )
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len);
    ret = AddFontResourceExW(strW, fl, pdv);
    HeapFree(GetProcessHeap(), 0, strW);
    return ret;
}

static BOOL CALLBACK load_enumed_resource(HMODULE hModule, LPCWSTR type, LPWSTR name, LONG_PTR lParam)
{
    HRSRC rsrc = FindResourceW(hModule, name, type);
    HGLOBAL hMem = LoadResource(hModule, rsrc);
    LPVOID *pMem = LockResource(hMem);
    int *num_total = (int *)lParam;
    DWORD num_in_res;

    TRACE("Found resource %s - trying to load\n", wine_dbgstr_w(type));
    if (!AddFontMemResourceEx(pMem, SizeofResource(hModule, rsrc), NULL, &num_in_res))
    {
        ERR("Failed to load PE font resource mod=%p ptr=%p\n", hModule, hMem);
        return FALSE;
    }

    *num_total += num_in_res;
    return TRUE;
}

static void *map_file( const WCHAR *filename, LARGE_INTEGER *size )
{
    HANDLE file, mapping;
    void *ptr;

    file = CreateFileW( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if (file == INVALID_HANDLE_VALUE) return NULL;

    if (!GetFileSizeEx( file, size ) || size->u.HighPart)
    {
        CloseHandle( file );
        return NULL;
    }

    mapping = CreateFileMappingW( file, NULL, PAGE_READONLY, 0, 0, NULL );
    CloseHandle( file );
    if (!mapping) return NULL;

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );

    return ptr;
}

static void *find_resource( BYTE *ptr, WORD type, DWORD rsrc_off, DWORD size, DWORD *len )
{
    WORD align, type_id, count;
    DWORD res_off;

    if (size < rsrc_off + 10) return NULL;
    align = *(WORD *)(ptr + rsrc_off);
    rsrc_off += 2;
    type_id = *(WORD *)(ptr + rsrc_off);
    while (type_id && type_id != type)
    {
        count = *(WORD *)(ptr + rsrc_off + 2);
        rsrc_off += 8 + count * 12;
        if (size < rsrc_off + 8) return NULL;
        type_id = *(WORD *)(ptr + rsrc_off);
    }
    if (!type_id) return NULL;
    count = *(WORD *)(ptr + rsrc_off + 2);
    if (size < rsrc_off + 8 + count * 12) return NULL;
    res_off = *(WORD *)(ptr + rsrc_off + 8) << align;
    *len = *(WORD *)(ptr + rsrc_off + 10) << align;
    if (size < res_off + *len) return NULL;
    return ptr + res_off;
}

static WCHAR *get_scalable_filename( const WCHAR *res, BOOL *hidden )
{
    LARGE_INTEGER size;
    BYTE *ptr = map_file( res, &size );
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_OS2_HEADER *ne;
    WORD *fontdir;
    char *data;
    WCHAR *name = NULL;
    DWORD len;

    if (!ptr) return NULL;

    if (size.u.LowPart < sizeof( *dos )) goto fail;
    dos = (const IMAGE_DOS_HEADER *)ptr;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) goto fail;
    if (size.u.LowPart < dos->e_lfanew + sizeof( *ne )) goto fail;
    ne = (const IMAGE_OS2_HEADER *)(ptr + dos->e_lfanew);

    fontdir = find_resource( ptr, 0x8007, dos->e_lfanew + ne->ne_rsrctab, size.u.LowPart, &len );
    if (!fontdir) goto fail;
    *hidden = (fontdir[35] & 0x80) != 0;  /* fontdir->dfType */

    data = find_resource( ptr, 0x80cc, dos->e_lfanew + ne->ne_rsrctab, size.u.LowPart, &len );
    if (!data) goto fail;
    if (!memchr( data, 0, len )) goto fail;

    len = MultiByteToWideChar( CP_ACP, 0, data, -1, NULL, 0 );
    name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (name) MultiByteToWideChar( CP_ACP, 0, data, -1, name, len );

fail:
    UnmapViewOfFile( ptr );
    return name;
}

static int add_system_font_resource( const WCHAR *file, DWORD flags )
{
    WCHAR path[MAX_PATH];
    int ret;

    /* try in %WINDIR%/fonts, needed for Fotobuch Designer */
    get_fonts_win_dir_path( file, path );
    EnterCriticalSection( &font_cs );
    ret = font_funcs->add_font( path, flags );
    LeaveCriticalSection( &font_cs );
    /* try in datadir/fonts (or builddir/fonts), needed for Magic the Gathering Online */
    if (!ret)
    {
        get_fonts_data_dir_path( file, path );
        EnterCriticalSection( &font_cs );
        ret = font_funcs->add_font( path, flags );
        LeaveCriticalSection( &font_cs );
    }
    return ret;
}

static BOOL remove_system_font_resource( LPCWSTR file, DWORD flags )
{
    WCHAR path[MAX_PATH];
    int ret;

    get_fonts_win_dir_path( file, path );
    if (!(ret = remove_font( path, flags )))
    {
        get_fonts_data_dir_path( file, path );
        ret = remove_font( path, flags );
    }
    return ret;
}

static int add_font_resource( LPCWSTR file, DWORD flags )
{
    WCHAR path[MAX_PATH];
    int ret = 0;

    if (GetFullPathNameW( file, MAX_PATH, path, NULL ))
    {
        DWORD addfont_flags = ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE;

        if (!(flags & FR_PRIVATE)) addfont_flags |= ADDFONT_ADD_TO_CACHE;
        EnterCriticalSection( &font_cs );
        ret = font_funcs->add_font( path, addfont_flags );
        LeaveCriticalSection( &font_cs );
    }

    if (!ret && !wcschr( file, '\\' ))
        ret = add_system_font_resource( file, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );

    return ret;
}

static BOOL remove_font_resource( LPCWSTR file, DWORD flags )
{
    WCHAR path[MAX_PATH];
    BOOL ret = FALSE;

    if (GetFullPathNameW( file, MAX_PATH, path, NULL ))
    {
        DWORD addfont_flags = ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE;

        if (!(flags & FR_PRIVATE)) addfont_flags |= ADDFONT_ADD_TO_CACHE;
        ret = remove_font( path, addfont_flags );
    }

    if (!ret && !wcschr( file, '\\' ))
        ret = remove_system_font_resource( file, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );

    return ret;
}

static void load_system_bitmap_fonts(void)
{
    static const WCHAR * const fonts[] = { L"FONTS.FON", L"OEMFONT.FON", L"FIXEDFON.FON" };
    HKEY hkey;
    WCHAR data[MAX_PATH];
    DWORD i, dlen, type;

    if (RegOpenKeyW( HKEY_CURRENT_CONFIG, L"Software\\Fonts", &hkey )) return;
    for (i = 0; i < ARRAY_SIZE(fonts); i++)
    {
        dlen = sizeof(data);
        if (!RegQueryValueExW( hkey, fonts[i], 0, &type, (BYTE *)data, &dlen ) && type == REG_SZ)
            add_system_font_resource( data, ADDFONT_ALLOW_BITMAP );
    }
    RegCloseKey( hkey );
}

static void load_directory_fonts( WCHAR *path, UINT flags )
{
    HANDLE handle;
    WIN32_FIND_DATAW data;
    WCHAR *p;

    p = path + lstrlenW(path) - 1;
    TRACE( "loading fonts from %s\n", debugstr_w(path) );
    handle = FindFirstFileW( path, &data );
    if (handle == INVALID_HANDLE_VALUE) return;
    do
    {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        lstrcpyW( p, data.cFileName );
        font_funcs->add_font( path, flags );
    } while (FindNextFileW( handle, &data ));
    FindClose( handle );
}

static void load_file_system_fonts(void)
{
    WCHAR *ptr, *next, path[MAX_PATH], value[1024];
    DWORD len = ARRAY_SIZE(value);

    /* Windows directory */
    get_fonts_win_dir_path( L"*", path );
    load_directory_fonts( path, 0 );

    /* Wine data directory */
    get_fonts_data_dir_path( L"*", path );
    load_directory_fonts( path, ADDFONT_EXTERNAL_FONT );

    /* custom paths */
    /* @@ Wine registry key: HKCU\Software\Wine\Fonts */
    if (!RegQueryValueExW( wine_fonts_key, L"Path", NULL, NULL, (BYTE *)value, &len ))
    {
        for (ptr = value; ptr; ptr = next)
        {
            if ((next = wcschr( ptr, ';' ))) *next++ = 0;
            if (next && next - ptr < 2) continue;
            lstrcpynW( path, ptr, MAX_PATH - 2 );
            lstrcatW( path, L"\\*" );
            load_directory_fonts( path, ADDFONT_EXTERNAL_FONT );
        }
    }
}

struct external_key
{
    struct list entry;
    WCHAR       value[LF_FULLFACESIZE + 12];
};

static void update_external_font_keys(void)
{
    struct list external_keys = LIST_INIT(external_keys);
    HKEY winnt_key = 0, win9x_key = 0;
    struct gdi_font_family *family;
    struct external_key *key, *next;
    struct gdi_font_face *face;
    DWORD len, i = 0, type, dlen, vlen;
    WCHAR value[LF_FULLFACESIZE + 12], path[MAX_PATH], *tmp;
    WCHAR *file;
    HKEY hkey;

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                     0, NULL, 0, KEY_ALL_ACCESS, NULL, &winnt_key, NULL );
    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Fonts",
                     0, NULL, 0, KEY_ALL_ACCESS, NULL, &win9x_key, NULL );

    /* enumerate the fonts and add external ones to the two keys */

    if (RegCreateKeyW( wine_fonts_key, L"External Fonts", &hkey )) return;

    vlen = ARRAY_SIZE(value);
    dlen = sizeof(path);
    while (!RegEnumValueW( hkey, i++, value, &vlen, NULL, &type, (BYTE *)path, &dlen ))
    {
        if (type != REG_SZ) goto next;
        if ((tmp = wcsrchr( value, ' ' )) && !facename_compare( tmp, L" (TrueType)", -1 )) *tmp = 0;
        if ((face = find_face_from_full_name( value )))
        {
            if (!wcsicmp( face->file, path )) face->flags |= ADDFONT_EXTERNAL_FOUND;
            goto next;
        }
        if (tmp && !*tmp) *tmp = ' ';
        if (!(key = HeapAlloc( GetProcessHeap(), 0, sizeof(*key) ))) break;
        lstrcpyW( key->value, value );
        list_add_tail( &external_keys, &key->entry );
    next:
        vlen = ARRAY_SIZE(value);
        dlen = sizeof(path);
    }

    WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
    {
        LIST_FOR_EACH_ENTRY( face, &family->faces, struct gdi_font_face, entry )
        {
            if (!(face->flags & ADDFONT_EXTERNAL_FONT)) continue;
            if ((face->flags & ADDFONT_EXTERNAL_FOUND)) continue;

            lstrcpyW( value, face->full_name );
            if (face->scalable) lstrcatW( value, L" (TrueType)" );

            if (GetFullPathNameW( face->file, MAX_PATH, path, NULL ))
                file = path;
            else if ((file = wcsrchr( face->file, '\\' )))
                file++;
            else
                file = face->file;

            len = (lstrlenW(file) + 1) * sizeof(WCHAR);
            RegSetValueExW( winnt_key, value, 0, REG_SZ, (BYTE *)file, len );
            RegSetValueExW( win9x_key, value, 0, REG_SZ, (BYTE *)file, len );
            RegSetValueExW( hkey, value, 0, REG_SZ, (BYTE *)file, len );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( key, next, &external_keys, struct external_key, entry )
    {
        RegDeleteValueW( win9x_key, key->value );
        RegDeleteValueW( winnt_key, key->value );
        RegDeleteValueW( hkey, key->value );
        list_remove( &key->entry );
        HeapFree( GetProcessHeap(), 0, key );
    }
    RegCloseKey( win9x_key );
    RegCloseKey( winnt_key );
    RegCloseKey( hkey );
}

static void load_registry_fonts(void)
{
    WCHAR value[LF_FULLFACESIZE + 12], data[MAX_PATH], *tmp;
    DWORD i = 0, type, dlen, vlen;
    HKEY hkey;

    /* Look under HKLM\Software\Microsoft\Windows[ NT]\CurrentVersion\Fonts
       for any fonts not installed in %WINDOWSDIR%\Fonts.  They will have their
       full path as the entry.  Also look for any .fon fonts, since ReadFontDir
       will skip these. */
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE,
                     is_win9x() ? L"Software\\Microsoft\\Windows\\CurrentVersion\\Fonts" :
                     L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", &hkey ))
        return;

    vlen = ARRAY_SIZE(value);
    dlen = sizeof(data);
    while (!RegEnumValueW( hkey, i++, value, &vlen, NULL, &type, NULL, NULL ))
    {
        if (type != REG_SZ) goto next;
        dlen /= sizeof(WCHAR);
        if ((tmp = wcsrchr( value, ' ' )) && !facename_compare( tmp, L" (TrueType)", -1 )) *tmp = 0;
        if (find_face_from_full_name( value )) goto next;
        if (tmp && !*tmp) *tmp = ' ';

        if (RegQueryValueExW( hkey, value, NULL, NULL, (LPBYTE)data, &dlen ))
        {
            WARN( "Unable to get face path %s\n", debugstr_w(value) );
            goto next;
        }

        dlen /= sizeof(WCHAR);
        if (data[0] && data[1] == ':')
            add_font_resource( data, ADDFONT_ALLOW_BITMAP );
        else if (dlen >= 6 && !wcsicmp( data + dlen - 5, L".fon" ))
            add_system_font_resource( data, ADDFONT_ALLOW_BITMAP );
    next:
        vlen = ARRAY_SIZE(value);
        dlen = sizeof(data);
    }
    RegCloseKey( hkey );
}

static const struct font_callback_funcs callback_funcs = { add_gdi_face };

/***********************************************************************
 *              font_init
 */
void font_init(void)
{
    HANDLE mutex;
    DWORD disposition;

    if (RegCreateKeyExW( HKEY_CURRENT_USER, L"Software\\Wine\\Fonts", 0, NULL, 0,
                         KEY_ALL_ACCESS, NULL, &wine_fonts_key, NULL ))
        return;

    init_font_options();
    update_codepage();
    if (__wine_init_unix_lib( gdi32_module, DLL_PROCESS_ATTACH, &callback_funcs, &font_funcs )) return;

    load_system_bitmap_fonts();
    load_file_system_fonts();
    font_funcs->load_fonts();

    if (!(mutex = CreateMutexW( NULL, FALSE, L"__WINE_FONT_MUTEX__" ))) return;
    WaitForSingleObject( mutex, INFINITE );

    RegCreateKeyExW( wine_fonts_key, L"Cache", 0, NULL, REG_OPTION_VOLATILE,
                     KEY_ALL_ACCESS, NULL, &wine_fonts_cache_key, &disposition );

    if (disposition == REG_CREATED_NEW_KEY)
    {
        load_registry_fonts();
        update_external_font_keys();
    }

    ReleaseMutex( mutex );

    if (disposition != REG_CREATED_NEW_KEY)
    {
        load_registry_fonts();
        load_font_list_from_cache();
    }

    reorder_font_list();
    load_gdi_font_subst();
    load_gdi_font_replacements();
    load_system_links();
    dump_gdi_font_list();
    dump_gdi_font_subst();
}

/***********************************************************************
 *           AddFontResourceExW    (GDI32.@)
 */
INT WINAPI AddFontResourceExW( LPCWSTR str, DWORD flags, PVOID pdv )
{
    int ret;
    WCHAR *filename;
    BOOL hidden;

    if (!font_funcs) return 1;
    if (!(ret = add_font_resource( str, flags )))
    {
        /* FreeType <2.3.5 has problems reading resources wrapped in PE files. */
        HMODULE hModule = LoadLibraryExW(str, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule != NULL)
        {
            int num_resources = 0;
            LPWSTR rt_font = (LPWSTR)((ULONG_PTR)8);  /* we don't want to include winuser.h */

            TRACE("WineEngAddFontResourceEx failed on PE file %s - trying to load resources manually\n",
                wine_dbgstr_w(str));
            if (EnumResourceNamesW(hModule, rt_font, load_enumed_resource, (LONG_PTR)&num_resources))
                ret = num_resources;
            FreeLibrary(hModule);
        }
        else if ((filename = get_scalable_filename( str, &hidden )) != NULL)
        {
            if (hidden) flags |= FR_PRIVATE | FR_NOT_ENUM;
            ret = add_font_resource( filename, flags );
            HeapFree( GetProcessHeap(), 0, filename );
        }
    }
    return ret;
}

/***********************************************************************
 *           RemoveFontResourceA    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceA( LPCSTR str )
{
    return RemoveFontResourceExA(str, 0, 0);
}

/***********************************************************************
 *           RemoveFontResourceW    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceW( LPCWSTR str )
{
    return RemoveFontResourceExW(str, 0, 0);
}

/***********************************************************************
 *           AddFontMemResourceEx    (GDI32.@)
 */
HANDLE WINAPI AddFontMemResourceEx( PVOID ptr, DWORD size, PVOID pdv, DWORD *pcFonts )
{
    HANDLE ret;
    DWORD num_fonts;
    void *copy;

    if (!ptr || !size || !pcFonts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (!font_funcs) return NULL;
    if (!(copy = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    memcpy( copy, ptr, size );

    EnterCriticalSection( &font_cs );
    num_fonts = font_funcs->add_mem_font( copy, size, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );
    LeaveCriticalSection( &font_cs );

    if (!num_fonts)
    {
        HeapFree( GetProcessHeap(), 0, copy );
        return NULL;
    }

    /* FIXME: is the handle only for use in RemoveFontMemResourceEx or should it be a true handle?
     * For now return something unique but quite random
     */
    ret = (HANDLE)((INT_PTR)copy ^ 0x87654321);

    __TRY
    {
        *pcFonts = num_fonts;
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN("page fault while writing to *pcFonts (%p)\n", pcFonts);
        RemoveFontMemResourceEx( ret );
        ret = 0;
    }
    __ENDTRY
    TRACE( "Returning handle %p\n", ret );
    return ret;
}

/***********************************************************************
 *           RemoveFontMemResourceEx    (GDI32.@)
 */
BOOL WINAPI RemoveFontMemResourceEx( HANDLE fh )
{
    FIXME("(%p) stub\n", fh);
    return TRUE;
}

/***********************************************************************
 *           RemoveFontResourceExA    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceExA( LPCSTR str, DWORD fl, PVOID pdv )
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len);
    ret = RemoveFontResourceExW(strW, fl, pdv);
    HeapFree(GetProcessHeap(), 0, strW);
    return ret;
}

/***********************************************************************
 *           RemoveFontResourceExW    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceExW( LPCWSTR str, DWORD flags, PVOID pdv )
{
    int ret;
    WCHAR *filename;
    BOOL hidden;

    if (!font_funcs) return TRUE;

    if (!(ret = remove_font_resource( str, flags )))
    {
        /* FreeType <2.3.5 has problems reading resources wrapped in PE files. */
        HMODULE hModule = LoadLibraryExW(str, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule != NULL)
        {
            WARN("Can't unload resources from PE file %s\n", wine_dbgstr_w(str));
            FreeLibrary(hModule);
        }
        else if ((filename = get_scalable_filename( str, &hidden )) != NULL)
        {
            if (hidden) flags |= FR_PRIVATE | FR_NOT_ENUM;
            ret = remove_font_resource( filename, flags );
            HeapFree( GetProcessHeap(), 0, filename );
        }
    }
    return ret;
}

/***********************************************************************
 *           GetFontResourceInfoW    (GDI32.@)
 */
BOOL WINAPI GetFontResourceInfoW( LPCWSTR str, LPDWORD size, PVOID buffer, DWORD type )
{
    FIXME("%s %p(%d) %p %d\n", debugstr_w(str), size, size ? *size : 0, buffer, type);
    return FALSE;
}

/***********************************************************************
 *           GetTextCharset    (GDI32.@)
 */
UINT WINAPI GetTextCharset(HDC hdc)
{
    /* MSDN docs say this is equivalent */
    return GetTextCharsetInfo(hdc, NULL, 0);
}

/***********************************************************************
 *           GdiGetCharDimensions    (GDI32.@)
 *
 * Gets the average width of the characters in the English alphabet.
 *
 * PARAMS
 *  hdc    [I] Handle to the device context to measure on.
 *  lptm   [O] Pointer to memory to store the text metrics into.
 *  height [O] On exit, the maximum height of characters in the English alphabet.
 *
 * RETURNS
 *  The average width of characters in the English alphabet.
 *
 * NOTES
 *  This function is used by the dialog manager to get the size of a dialog
 *  unit. It should also be used by other pieces of code that need to know
 *  the size of a dialog unit in logical units without having access to the
 *  window handle of the dialog.
 *  Windows caches the font metrics from this function, but we don't and
 *  there doesn't appear to be an immediate advantage to do so.
 *
 * SEE ALSO
 *  GetTextExtentPointW, GetTextMetricsW, MapDialogRect.
 */
LONG WINAPI GdiGetCharDimensions(HDC hdc, LPTEXTMETRICW lptm, LONG *height)
{
    SIZE sz;

    if(lptm && !GetTextMetricsW(hdc, lptm)) return 0;

    if(!GetTextExtentPointW(hdc, L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", 52, &sz))
        return 0;

    if (height) *height = sz.cy;
    return (sz.cx / 26 + 1) / 2;
}

BOOL WINAPI EnableEUDC(BOOL fEnableEUDC)
{
    FIXME("(%d): stub\n", fEnableEUDC);
    return FALSE;
}

/***********************************************************************
 *           GetCharWidthI    (GDI32.@)
 *
 * Retrieve widths of characters.
 *
 * PARAMS
 *  hdc    [I] Handle to a device context.
 *  first  [I] First glyph in range to query.
 *  count  [I] Number of glyph indices to query.
 *  glyphs [I] Array of glyphs to query.
 *  buffer [O] Buffer to receive character widths.
 *
 * NOTES
 *  Only works with TrueType fonts.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI GetCharWidthI(HDC hdc, UINT first, UINT count, LPWORD glyphs, LPINT buffer)
{
    ABC *abc;
    unsigned int i;

    TRACE("(%p, %d, %d, %p, %p)\n", hdc, first, count, glyphs, buffer);

    if (!(abc = HeapAlloc(GetProcessHeap(), 0, count * sizeof(ABC))))
        return FALSE;

    if (!GetCharABCWidthsI(hdc, first, count, glyphs, abc))
    {
        HeapFree(GetProcessHeap(), 0, abc);
        return FALSE;
    }

    for (i = 0; i < count; i++)
        buffer[i] = abc[i].abcA + abc[i].abcB + abc[i].abcC;

    HeapFree(GetProcessHeap(), 0, abc);
    return TRUE;
}

/***********************************************************************
 *           GetFontUnicodeRanges    (GDI32.@)
 *
 *  Retrieve a list of supported Unicode characters in a font.
 *
 *  PARAMS
 *   hdc  [I] Handle to a device context.
 *   lpgs [O] GLYPHSET structure specifying supported character ranges.
 *
 *  RETURNS
 *   Success: Number of bytes written to the buffer pointed to by lpgs.
 *   Failure: 0
 *
 */
DWORD WINAPI GetFontUnicodeRanges(HDC hdc, LPGLYPHSET lpgs)
{
    DWORD ret;
    PHYSDEV dev;
    DC *dc = get_dc_ptr(hdc);

    TRACE("(%p, %p)\n", hdc, lpgs);

    if (!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetFontUnicodeRanges );
    ret = dev->funcs->pGetFontUnicodeRanges( dev, lpgs );
    release_dc_ptr(dc);
    return ret;
}


/*************************************************************
 *           FontIsLinked    (GDI32.@)
 */
BOOL WINAPI FontIsLinked(HDC hdc)
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    BOOL ret;

    if (!dc) return FALSE;
    dev = GET_DC_PHYSDEV( dc, pFontIsLinked );
    ret = dev->funcs->pFontIsLinked( dev );
    release_dc_ptr(dc);
    TRACE("returning %d\n", ret);
    return ret;
}

/*************************************************************
 *           GetFontRealizationInfo    (GDI32.@)
 */
BOOL WINAPI GetFontRealizationInfo(HDC hdc, struct font_realization_info *info)
{
    BOOL is_v0 = info->size == FIELD_OFFSET(struct font_realization_info, unk);
    PHYSDEV dev;
    BOOL ret;
    DC *dc;

    if (info->size != sizeof(*info) && !is_v0)
        return FALSE;

    dc = get_dc_ptr(hdc);
    if (!dc) return FALSE;
    dev = GET_DC_PHYSDEV( dc, pGetFontRealizationInfo );
    ret = dev->funcs->pGetFontRealizationInfo( dev, info );
    release_dc_ptr(dc);
    return ret;
}

/*************************************************************************
 *             GetRasterizerCaps   (GDI32.@)
 */
BOOL WINAPI GetRasterizerCaps( LPRASTERIZER_STATUS lprs, UINT cbNumBytes)
{
    lprs->nSize = sizeof(RASTERIZER_STATUS);
    lprs->wFlags = font_funcs ? (TT_AVAILABLE | TT_ENABLED) : 0;
    lprs->nLanguageID = 0;
    return TRUE;
}

/*************************************************************************
 *             GetFontFileData   (GDI32.@)
 */
BOOL WINAPI GetFontFileData( DWORD instance_id, DWORD unknown, UINT64 offset, void *buff, DWORD buff_size )
{
    struct gdi_font *font;
    DWORD tag = 0, size;
    BOOL ret = FALSE;

    if (!font_funcs) return FALSE;
    EnterCriticalSection( &font_cs );
    if ((font = get_font_from_handle( instance_id )))
    {
        if (font->ttc_item_offset) tag = MS_TTCF_TAG;
        size = font_funcs->get_font_data( font, tag, 0, NULL, 0 );
        if (size != GDI_ERROR && size >= buff_size && offset <= size - buff_size)
            ret = font_funcs->get_font_data( font, tag, offset, buff, buff_size ) != GDI_ERROR;
        else
            SetLastError( ERROR_INVALID_PARAMETER );
    }
    LeaveCriticalSection( &font_cs );
    return ret;
}

/* Undocumented structure filled in by GetFontFileInfo */
struct font_fileinfo
{
    FILETIME writetime;
    LARGE_INTEGER size;
    WCHAR path[1];
};

/*************************************************************************
 *             GetFontFileInfo   (GDI32.@)
 */
BOOL WINAPI GetFontFileInfo( DWORD instance_id, DWORD unknown, struct font_fileinfo *info,
                             SIZE_T size, SIZE_T *needed )
{
    SIZE_T required_size = 0;
    struct gdi_font *font;
    BOOL ret = FALSE;

    EnterCriticalSection( &font_cs );

    if ((font = get_font_from_handle( instance_id )))
    {
        required_size = sizeof(*info) + lstrlenW( font->file ) * sizeof(WCHAR);
        if (required_size <= size)
        {
            info->writetime = font->writetime;
            info->size.QuadPart = font->data_size;
            lstrcpyW( info->path, font->file );
            ret = TRUE;
        }
        else SetLastError( ERROR_INSUFFICIENT_BUFFER );
    }

    LeaveCriticalSection( &font_cs );
    if (needed) *needed = required_size;
    return ret;
}

struct realization_info
{
    DWORD flags;       /* 1 for bitmap fonts, 3 for scalable fonts */
    DWORD cache_num;   /* keeps incrementing - num of fonts that have been created allowing for caching?? */
    DWORD instance_id; /* identifies a realized font instance */
};

/*************************************************************
 *           GdiRealizationInfo    (GDI32.@)
 *
 * Returns a structure that contains some font information.
 */
BOOL WINAPI GdiRealizationInfo(HDC hdc, struct realization_info *info)
{
    struct font_realization_info ri;
    BOOL ret;

    ri.size = sizeof(ri);
    ret = GetFontRealizationInfo( hdc, &ri );
    if (ret)
    {
        info->flags = ri.flags;
        info->cache_num = ri.cache_num;
        info->instance_id = ri.instance_id;
    }

    return ret;
}

/*************************************************************
 *           GetCharWidthInfo    (GDI32.@)
 *
 */
BOOL WINAPI GetCharWidthInfo(HDC hdc, struct char_width_info *info)
{
    PHYSDEV dev;
    BOOL ret;
    DC *dc;

    dc = get_dc_ptr(hdc);
    if (!dc) return FALSE;
    dev = GET_DC_PHYSDEV( dc, pGetCharWidthInfo );
    ret = dev->funcs->pGetCharWidthInfo( dev, info );

    if (ret)
    {
        info->lsb = width_to_LP( dc, info->lsb );
        info->rsb = width_to_LP( dc, info->rsb );
    }
    release_dc_ptr(dc);
    return ret;
}
