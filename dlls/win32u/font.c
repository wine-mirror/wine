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

#if 0
#pragma makedep unix
#endif

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winreg.h"
#include "ntgdi_private.h"

#include "wine/unixlib.h"
#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

static HKEY wine_fonts_key;
static HKEY wine_fonts_cache_key;
HKEY hkcu_key;

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
    UINT          ntmFlags;
    UINT          version;
    UINT          flags;                 /* ADDFONT flags */
    BOOL          scalable;
    struct bitmap_font_size    size;     /* set if face is a bitmap */
    struct gdi_font_family    *family;
    struct gdi_font_enum_data *cached_enum_data;
    struct wine_rb_entry       full_name_entry;
};

static const struct font_backend_funcs *font_funcs;

static const MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };

static const WCHAR nt_prefixW[] = {'\\','?','?','\\'};

static const WCHAR true_type_suffixW[] = {' ','(','T','r','u','e','T','y','p','e',')',0};

static const WCHAR system_link_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','o','f','t','w','a','r','e',
    '\\','M','i','c','r','o','s','o','f','t',
    '\\','W','i','n','d','o','w','s',' ','N','T',
    '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
    '\\','F','o','n','t','L','i','n','k',
    '\\','S','y','s','t','e','m','L','i','n','k'
};

static const WCHAR associated_charset_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','C','o','n','t','r','o','l',
    '\\','F','o','n','t','A','s','s','o','c',
    '\\','A','s','s','o','c','i','a','t','e','d',' ','C','h','a','r','s','e','t'
};

static const WCHAR software_config_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','H','a','r','d','w','a','r','e',' ','P','r','o','f','i','l','e','s',
    '\\','C','u','r','r','e','n','t',
    '\\','S','o','f','t','w','a','r','e',
};

static const WCHAR fonts_config_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','H','a','r','d','w','a','r','e',' ','P','r','o','f','i','l','e','s',
    '\\','C','u','r','r','e','n','t',
    '\\','S','o','f','t','w','a','r','e',
    '\\','F','o','n','t','s'
};

static const WCHAR fonts_win9x_config_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','o','f','t','w','a','r','e',
    '\\','M','i','c','r','o','s','o','f','t',
    '\\','W','i','n','d','o','w','s',
    '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
    '\\','F','o','n','t','s'
};

static const WCHAR fonts_winnt_config_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','o','f','t','w','a','r','e',
    '\\','M','i','c','r','o','s','o','f','t',
    '\\','W','i','n','d','o','w','s',' ','N','T',
    '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
    '\\','F','o','n','t','s'
};

static const WCHAR font_substitutes_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','o','f','t','w','a','r','e',
    '\\','M','i','c','r','o','s','o','f','t',
    '\\','W','i','n','d','o','w','s',' ','N','T',
    '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
    '\\','F','o','n','t','S','u','b','s','t','i','t','u','t','e','s'
};

static const WCHAR font_assoc_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','C','o','n','t','r','o','l',
    '\\','F','o','n','t','A','s','s','o','c'
};

static UINT font_smoothing = GGO_BITMAP;
static UINT subpixel_orientation = GGO_GRAY4_BITMAP;
static BOOL antialias_fakes = TRUE;
static struct font_gamma_ramp font_gamma_ramp;

static void add_face_to_cache( struct gdi_font_face *face );
static void remove_face_from_cache( struct gdi_font_face *face );

static CPTABLEINFO utf8_cp;
static CPTABLEINFO oem_cp;
CPTABLEINFO ansi_cp = { 0 };

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

static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL FONT_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs fontobj_funcs =
{
    FONT_GetObjectW,    /* pGetObjectW */
    NULL,               /* pUnrealizeObject */
    FONT_DeleteObject   /* pDeleteObject */
};

typedef struct
{
    struct gdi_obj_header obj;
    LOGFONTW              logfont;
} FONTOBJ;

/* for translate_charset_info */
static const CHARSETINFO charset_info[] =
{
    { ANSI_CHARSET,        1252,      { {0}, { FS_LATIN1 }}},
    { EASTEUROPE_CHARSET,  1250,      { {0}, { FS_LATIN2 }}},
    { RUSSIAN_CHARSET,     1251,      { {0}, { FS_CYRILLIC }}},
    { GREEK_CHARSET,       1253,      { {0}, { FS_GREEK }}},
    { TURKISH_CHARSET,     1254,      { {0}, { FS_TURKISH }}},
    { HEBREW_CHARSET,      1255,      { {0}, { FS_HEBREW }}},
    { ARABIC_CHARSET,      1256,      { {0}, { FS_ARABIC }}},
    { BALTIC_CHARSET,      1257,      { {0}, { FS_BALTIC }}},
    { VIETNAMESE_CHARSET,  1258,      { {0}, { FS_VIETNAMESE }}},
    { THAI_CHARSET,        874,       { {0}, { FS_THAI }}},
    { SHIFTJIS_CHARSET,    932,       { {0}, { FS_JISJAPAN }}},
    { GB2312_CHARSET,      936,       { {0}, { FS_CHINESESIMP }}},
    { HANGEUL_CHARSET,     949,       { {0}, { FS_WANSUNG }}},
    { CHINESEBIG5_CHARSET, 950,       { {0}, { FS_CHINESETRAD }}},
    { JOHAB_CHARSET,       1361,      { {0}, { FS_JOHAB }}},
    { 254,                 CP_UTF8,   { {0}, { 0x04000000 }}},
    { SYMBOL_CHARSET,      CP_SYMBOL, { {0}, { FS_SYMBOL }}}
};

static const char * const default_serif_list[3] =
{
    "Times New Roman",
    "Liberation Serif",
    "Bitstream Vera Serif"
};
static const char * const default_fixed_list[3] =
{
    "Courier New",
    "Liberation Mono",
    "Bitstream Vera Sans Mono"
};
static const char * const default_sans_list[3] =
{
    "Arial",
    "Liberation Sans",
    "Bitstream Vera Sans"
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
    /* UTF-8 */
    { CP_UTF8, CP_UTF8, "vga850.fon", "vgafix.fon", "vgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon", "sseriff.fon",
      "Tahoma", "Times New Roman"  /* FIXME unverified */
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

static pthread_mutex_t font_lock = PTHREAD_MUTEX_INITIALIZER;

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
    const char *dir;
    ULONG len = MAX_PATH;

    if ((dir = ntdll_get_data_dir()))
    {
        wine_unix_to_nt_file_name( dir, path, &len );
        asciiz_to_unicode( path + len - 1, "\\" WINE_FONT_DIR "\\" );
    }
    else if ((dir = ntdll_get_build_dir()))
    {
        wine_unix_to_nt_file_name( dir, path, &len );
        asciiz_to_unicode( path + len - 1, "\\fonts\\" );
    }

    if (file) lstrcatW( path, file );
}

static void get_fonts_win_dir_path( const WCHAR *file, WCHAR *path )
{
    asciiz_to_unicode( path, "\\??\\C:\\windows\\fonts\\" );
    if (file) lstrcatW( path, file );
}

HKEY reg_open_key( HKEY root, const WCHAR *name, ULONG name_len )
{
    UNICODE_STRING nameW = { name_len, name_len, (WCHAR *)name };
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (NtOpenKeyEx( &ret, MAXIMUM_ALLOWED, &attr, 0 )) return 0;
    return ret;
}

/* wrapper for NtCreateKey that creates the key recursively if necessary */
HKEY reg_create_key( HKEY root, const WCHAR *name, ULONG name_len,
                     DWORD options, DWORD *disposition )
{
    UNICODE_STRING nameW = { name_len, name_len, (WCHAR *)name };
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateKey( &ret, MAXIMUM_ALLOWED, &attr, 0, NULL, options, disposition );
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        static const WCHAR registry_rootW[] = { '\\','R','e','g','i','s','t','r','y','\\' };
        DWORD pos = 0, i = 0, len = name_len / sizeof(WCHAR);

        /* don't try to create registry root */
        if (!root && len > ARRAY_SIZE(registry_rootW) &&
            !memcmp( name, registry_rootW, sizeof(registry_rootW) ))
            i += ARRAY_SIZE(registry_rootW);

        while (i < len && name[i] != '\\') i++;
        if (i == len) return 0;
        for (;;)
        {
            unsigned int subkey_options = options;
            if (i < len) subkey_options &= ~(REG_OPTION_CREATE_LINK | REG_OPTION_OPEN_LINK);
            nameW.Buffer = (WCHAR *)name + pos;
            nameW.Length = (i - pos) * sizeof(WCHAR);
            status = NtCreateKey( &ret, MAXIMUM_ALLOWED, &attr, 0, NULL, subkey_options, disposition );

            if (attr.RootDirectory != root) NtClose( attr.RootDirectory );
            if (!NT_SUCCESS(status)) return 0;
            if (i == len) break;
            attr.RootDirectory = ret;
            while (i < len && name[i] == '\\') i++;
            pos = i;
            while (i < len && name[i] != '\\') i++;
        }
    }
    return ret;
}

HKEY reg_open_hkcu_key( const char *name )
{
    WCHAR nameW[128];
    return reg_open_key( hkcu_key, nameW, asciiz_to_unicode( nameW, name ) - sizeof(WCHAR) );
}

BOOL set_reg_value( HKEY hkey, const WCHAR *name, UINT type, const void *value, DWORD count )
{
    unsigned int name_size = name ? lstrlenW( name ) * sizeof(WCHAR) : 0;
    UNICODE_STRING nameW = { name_size, name_size, (WCHAR *)name };
    return !NtSetValueKey( hkey, &nameW, 0, type, value, count );
}

void set_reg_ascii_value( HKEY hkey, const char *name, const char *value )
{
    WCHAR nameW[64], valueW[128];
    asciiz_to_unicode( nameW, name );
    set_reg_value( hkey, nameW, REG_SZ, valueW, asciiz_to_unicode( valueW, value ));
}

ULONG query_reg_value( HKEY hkey, const WCHAR *name,
                       KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size )
{
    unsigned int name_size = name ? lstrlenW( name ) * sizeof(WCHAR) : 0;
    UNICODE_STRING nameW = { name_size, name_size, (WCHAR *)name };

    if (NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                         info, size, &size ))
        return 0;

    return size - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
}

ULONG query_reg_ascii_value( HKEY hkey, const char *name,
                             KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size )
{
    WCHAR nameW[64];
    asciiz_to_unicode( nameW, name );
    return query_reg_value( hkey, nameW, info, size );
}

static BOOL reg_enum_value( HKEY hkey, unsigned int index, KEY_VALUE_FULL_INFORMATION *info,
                            ULONG size, WCHAR *name, ULONG name_size )
{
    ULONG full_size;

    if (NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                             info, size, &full_size ))
        return FALSE;

    if (name_size)
    {
        if (name_size < info->NameLength + sizeof(WCHAR)) return FALSE;
        memcpy( name, info->Name, info->NameLength );
        name[info->NameLength / sizeof(WCHAR)] = 0;
    }
    return TRUE;
}

void reg_delete_value( HKEY hkey, const WCHAR *name )
{
    unsigned int name_size = lstrlenW( name ) * sizeof(WCHAR);
    UNICODE_STRING nameW = { name_size, name_size, (WCHAR *)name };
    NtDeleteValueKey( hkey, &nameW );
}

BOOL reg_delete_tree( HKEY parent, const WCHAR *name, ULONG name_len )
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key_info = (KEY_NODE_INFORMATION *)buffer;
    DWORD size;
    HKEY key;
    BOOL ret = TRUE;

    if (!(key = reg_open_key( parent, name, name_len ))) return FALSE;

    while (ret && !NtEnumerateKey( key, 0, KeyNodeInformation, key_info, sizeof(buffer), &size ))
        ret = reg_delete_tree( key, key_info->Name, key_info->NameLength );

    if (ret) ret = !NtDeleteKey( key );
    NtClose( key );
    return ret;
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

    if (!(subst = malloc( offsetof( struct gdi_font_subst, names[len] ) )))
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
    char buffer[512];
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    HKEY hkey;
    DWORD i = 0;
    WCHAR *data, *p, value[64];

    if (!(hkey = reg_open_key( NULL, font_substitutes_keyW, sizeof(font_substitutes_keyW) )))
        return;

    while (reg_enum_value( hkey, i++, info, sizeof(buffer), value, sizeof(value) ))
    {
        int from_charset = -1, to_charset = -1;

        if (info->Type != REG_SZ) continue;
        data = (WCHAR *)((char *)info + info->DataOffset);

        TRACE( "Got %s=%s\n", debugstr_w(value), debugstr_w(data) );
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
    }
    NtClose( hkey );
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
    struct gdi_font_family *family = malloc( sizeof(*family) );

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
    free( family );
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

    if (family->replacement)
    {
        TRACE( "%s is replaced by another font, skipping.\n", debugstr_w(replace) );
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
    char buffer[2048];
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    HKEY hkey;
    DWORD i = 0;
    WCHAR value[LF_FACESIZE];

    static const WCHAR replacementsW[] = {'R','e','p','l','a','c','e','m','e','n','t','s'};

    /* @@ Wine registry key: HKCU\Software\Wine\Fonts\Replacements */
    if (!(hkey = reg_open_key( wine_fonts_key, replacementsW, sizeof(replacementsW) ))) return;

    while (reg_enum_value( hkey, i++, info, sizeof(buffer), value, sizeof(value) ))
    {
        WCHAR *data = (WCHAR *)((char *)info + info->DataOffset);
        /* "NewName"="Oldname" */
        if (!find_family_from_any_name( value ))
        {
            if (info->Type == REG_MULTI_SZ)
            {
                WCHAR *replace = data;
                while (*replace)
                {
                    if (add_family_replacement( value, replace )) break;
                    replace += lstrlenW(replace) + 1;
                }
            }
            else if (info->Type == REG_SZ) add_family_replacement( value, data );
        }
        else TRACE("%s is available. Skip this replacement.\n", debugstr_w(value));
    }
    NtClose( hkey );
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
                   (int)face->fs.fsCsb[0] );
            if (!face->scalable) TRACE(" %d", face->size.height );
            TRACE("\n");
	}
    }
}

static BOOL enum_fallbacks( DWORD pitch_and_family, int index, WCHAR buffer[LF_FACESIZE] )
{
    if (index < 3)
    {
        const char * const *defaults;

        if ((pitch_and_family & FIXED_PITCH) || (pitch_and_family & 0xf0) == FF_MODERN)
            defaults = default_fixed_list;
        else if ((pitch_and_family & 0xf0) == FF_ROMAN)
            defaults = default_serif_list;
        else
            defaults = default_sans_list;
        asciiz_to_unicode( buffer, defaults[index] );
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
    free( face->file );
    free( face->style_name );
    free( face->full_name );
    free( face->cached_enum_data );
    free( face );
}

static int remove_font( const WCHAR *file, DWORD flags )
{
    struct gdi_font_family *family, *family_next;
    struct gdi_font_face *face, *face_next;
    int count = 0;

    pthread_mutex_lock( &font_lock );
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
    pthread_mutex_unlock( &font_lock );
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
    struct gdi_font_face *face = calloc( 1, sizeof(*face) );

    face->refcount   = 1;
    face->style_name = wcsdup( style );
    face->full_name  = wcsdup( fullname );
    face->face_index = index;
    face->fs         = fs;
    face->ntmFlags   = ntmflags;
    face->version    = version;
    face->flags      = flags;
    face->data_ptr   = data_ptr;
    face->data_size  = data_size;
    if (file) face->file = wcsdup( file );
    if (size) face->size = *size;
    else face->scalable = TRUE;
    if (insert_face_in_family_list( face, family )) return face;
    release_face( face );
    return NULL;
}

int add_gdi_face( const WCHAR *family_name, const WCHAR *second_name,
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
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    KEY_NODE_INFORMATION *node_info = (KEY_NODE_INFORMATION *)buffer;
    DWORD index = 0, total_size;
    struct gdi_font_face *face;
    HKEY hkey_strike;
    WCHAR name[256];
    struct cached_face *cached;

    while (reg_enum_value( hkey_family, index++, info,
                           buffer_size - sizeof(DWORD), name, sizeof(name) ))
    {
        cached = (struct cached_face *)((char *)info + info->DataOffset);
        if (info->Type == REG_BINARY && info->DataLength > sizeof(*cached))
        {
            ((DWORD *)cached)[info->DataLength / sizeof(DWORD)] = 0;
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
                      (int)face->fs.fsCsb[0], (int)face->fs.fsCsb[1],
                      (int)face->fs.fsUsb[0], (int)face->fs.fsUsb[1],
                      (int)face->fs.fsUsb[2], (int)face->fs.fsUsb[3]);

                release_face( face );
            }
        }
    }

    /* load bitmap strikes */

    index = 0;
    while (!NtEnumerateKey( hkey_family, index++, KeyNodeInformation, node_info,
                            buffer_size, &total_size ))
    {
        if ((hkey_strike = reg_open_key( hkey_family, node_info->Name, node_info->NameLength )))
        {
            load_face_from_cache( hkey_strike, family, buffer, buffer_size, FALSE );
            NtClose( hkey_strike );
        }
    }
}

static void load_font_list_from_cache(void)
{
    WCHAR buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)buffer;
    KEY_NODE_INFORMATION *enum_info = (KEY_NODE_INFORMATION *)buffer;
    DWORD family_index = 0, total_size;
    struct gdi_font_family *family;
    HKEY hkey_family;
    WCHAR *second_name = (WCHAR *)info->Data;

    while (!NtEnumerateKey( wine_fonts_cache_key, family_index++, KeyNodeInformation, enum_info,
                            sizeof(buffer), &total_size ))
    {
        if (!(hkey_family = reg_open_key( wine_fonts_cache_key, enum_info->Name,
                                          enum_info->NameLength )))
            continue;
        TRACE( "opened family key %s\n", debugstr_wn(enum_info->Name, enum_info->NameLength / sizeof(WCHAR)) );
        if (!query_reg_value( hkey_family, NULL, info, sizeof(buffer) ))
            second_name[0] = 0;

        family = create_family( buffer, second_name );

        load_face_from_cache( hkey_family, family, buffer, sizeof(buffer), TRUE );

        NtClose( hkey_family );
        release_family( family );
    }
}

static void add_face_to_cache( struct gdi_font_face *face )
{
    HKEY hkey_family, hkey_face;
    DWORD len, buffer[1024];
    struct cached_face *cached = (struct cached_face *)buffer;

    if (!(hkey_family = reg_create_key( wine_fonts_cache_key, face->family->family_name,
                                        lstrlenW( face->family->family_name ) * sizeof(WCHAR),
                                        REG_OPTION_VOLATILE, NULL )))
        return;

    if (face->family->second_name[0])
        set_reg_value( hkey_family, NULL, REG_SZ, face->family->second_name,
                       (lstrlenW( face->family->second_name ) + 1) * sizeof(WCHAR) );

    if (!face->scalable)
    {
        WCHAR nameW[10];
        char name[10];

        sprintf( name, "%d", face->size.y_ppem );
        hkey_face = reg_create_key( hkey_family, nameW,
                                    asciiz_to_unicode( nameW, name ) - sizeof(WCHAR),
                                    REG_OPTION_VOLATILE, NULL );
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

    set_reg_value( hkey_face, face->style_name, REG_BINARY, cached,
                   offsetof( struct cached_face, full_name[len] ));

    if (hkey_face != hkey_family) NtClose( hkey_face );
    NtClose( hkey_family );
}

static void remove_face_from_cache( struct gdi_font_face *face )
{
    HKEY hkey_family, hkey;

    if (!(hkey_family = reg_open_key( wine_fonts_cache_key, face->family->family_name,
                                      lstrlenW( face->family->family_name ) * sizeof(WCHAR) )))
        return;

    if (!face->scalable)
    {
        WCHAR nameW[10];
        char name[10];
        sprintf( name, "%d", face->size.y_ppem );
        if ((hkey = reg_open_key( hkey_family, nameW,
                                  asciiz_to_unicode( nameW, name ) - sizeof(WCHAR) )))
        {
            NtDeleteKey( hkey );
            NtClose( hkey );
        }
    }
    else reg_delete_value( hkey_family, face->style_name );

    NtClose( hkey_family );
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
    if ((link = malloc( sizeof(*link) )))
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

    entry = malloc( sizeof(*entry) );
    lstrcpynW( entry->family_name, family_name, LF_FACESIZE );
    entry->fs = fs;
    link->fs.fsCsb[0] |= fs.fsCsb[0];
    link->fs.fsCsb[1] |= fs.fsCsb[1];
    list_add_tail( &link->links, &entry->entry );
}

static const WCHAR lucida_sans_unicodeW[] =
    {'L','u','c','i','d','a',' ','S','a','n','s',' ','U','n','i','c','o','d','e',0};
static const WCHAR microsoft_sans_serifW[] =
    {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0};
static const WCHAR tahomaW[] =
    {'T','a','h','o','m','a',0};
static const WCHAR ms_gothicW[] =
    {'M','S',' ','G','o','t','h','i','c',0};
static const WCHAR ms_p_gothicW[] =
    {'M','S',' ','P','G','o','t','h','i','c',0};
static const WCHAR ms_ui_gothicW[] =
    {'M','S',' ','U','I',' ','G','o','t','h','i','c',0};
static const WCHAR sim_sunW[] =
    {'S','i','m','S','u','n',0};
static const WCHAR gulimW[] =
    {'G','u','l','i','m',0};
static const WCHAR ming_li_uW[] =
    {'M','i','n','g','L','i','U',0};
static const WCHAR p_ming_li_uW[] =
    {'P','M','i','n','g','L','i','U',0};
static const WCHAR ming_li_u_hkscsW[] =
    {'M','i','n','g','L','i','U','_','H','K','S','C','S',0};
static const WCHAR ming_li_u_ext_bW[] =
    {'M','i','n','g','L','i','U','-','E','x','t','B',0};
static const WCHAR p_ming_li_u_ext_bW[] =
    {'P','M','i','n','g','L','i','U','-','E','x','t','B',0};
static const WCHAR ming_li_u_hkscs_ext_bW[] =
    {'M','i','n','g','L','i','U','_','H','K','S','C','S','-','E','x','t','B',0};
static const WCHAR batangW[] =
    {'B','a','t','a','n','g',0};
static const WCHAR microsoft_jheng_heiW[] =
    {'M','i','c','r','o','s','o','f','t',' ','J','h','e','n','g','H','e','i',0};
static const WCHAR microsoft_jheng_hei_boldW[] =
    {'M','i','c','r','o','s','o','f','t',' ','J','h','e','n','g','H','e','i',' ','B','o','l','d',0};
static const WCHAR microsoft_jheng_hei_uiW[] =
    {'M','i','c','r','o','s','o','f','t',' ','J','h','e','n','g','H','e','i',' ','U','I',0};
static const WCHAR microsoft_jheng_hei_ui_boldW[] =
    {'M','i','c','r','o','s','o','f','t',' ','J','h','e','n','g','H','e','i',' ','U','I',' ','B','o','l','d',0};
static const WCHAR microsoft_jheng_hei_ui_lightW[] =
    {'M','i','c','r','o','s','o','f','t',' ','J','h','e','n','g','H','e','i',' ','U','I',' ','L','i','g','h','t',0};
static const WCHAR yu_gothic_uiW[] =
    {'Y','u',' ','G','o','t','h','i','c',' ','U','I',0};
static const WCHAR yu_gothic_ui_boldW[] =
    {'Y','u',' ','G','o','t','h','i','c',' ','U','I',' ','B','o','l','d',0};
static const WCHAR yu_gothic_ui_lightW[] =
    {'Y','u',' ','G','o','t','h','i','c',' ','U','I',' ','L','i','g','h','t',0};
static const WCHAR yu_gothic_ui_semilightW[] =
    {'Y','u',' ','G','o','t','h','i','c',' ','U','I',' ','S','e','m','i','l','i','g','h','t',0};
static const WCHAR yu_gothic_ui_semiboldW[] =
    {'Y','u',' ','G','o','t','h','i','c',' ','U','I',' ','S','e','m','i','b','o','l','d',0};
static const WCHAR meiryoW[] =
    {'M','e','i','r','y','o',0};
static const WCHAR meiryo_boldW[] =
    {'M','e','i','r','y','o',' ','B','o','l','d',0};
static const WCHAR meiryo_uiW[] =
    {'M','e','i','r','y','o',' ','U','I',0};
static const WCHAR meiryo_ui_boldW[] =
    {'M','e','i','r','y','o',' ','U','I',' ','B','o','l','d',0};
static const WCHAR ms_minchoW[] =
    {'M','S',' ','M','i','n','c','h','o',0};
static const WCHAR ms_p_minchoW[] =
    {'M','S',' ','P','M','i','n','c','h','o',0};

static const WCHAR * const font_links_list[] =
{
    lucida_sans_unicodeW,
    microsoft_sans_serifW,
    tahomaW
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
    { tahomaW, /* FIXME unverified ordering */
      { ms_ui_gothicW, sim_sunW, gulimW, p_ming_li_uW, NULL }
    },
    /* Below lists are courtesy of
     * http://blogs.msdn.com/michkap/archive/2005/06/18/430507.aspx
     */
    /* Japanese */
    { ms_ui_gothicW,
      { ms_ui_gothicW, p_ming_li_uW, sim_sunW, gulimW, NULL }
    },
    /* Chinese Simplified */
    { sim_sunW,
      { sim_sunW, p_ming_li_uW, ms_ui_gothicW, batangW, NULL }
    },
    /* Korean */
    { gulimW,
      { gulimW, p_ming_li_uW, ms_ui_gothicW, sim_sunW, NULL }
    },
    /* Chinese Traditional */
    { p_ming_li_uW,
      { p_ming_li_uW, sim_sunW, ms_ui_gothicW, batangW, NULL }
    }
};

static const char system_link_tahoma_sc[] =
    "SIMSUN.TTC,SimSun\0"
    "MINGLIU.TTC,PMingLiu\0"
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "BATANG.TTC,Batang\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_tahoma_tc[] =
    "MINGLIU.TTC,PMingLiu\0"
    "SIMSUN.TTC,SimSun\0"
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "BATANG.TTC,Batang\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_tahoma_jp[] =
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "MINGLIU.TTC,PMingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "GULIM.TTC,Gulim\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_tahoma_kr[] =
    "GULIM.TTC,Gulim\0"
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "MINGLIU.TTC,PMingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_tahoma_non_cjk[] =
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "MINGLIU.TTC,PMingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "GULIM.TTC,Gulim\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ms_gothic[] =
    "MINGLIU.TTC,MingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "GULIM.TTC,GulimChe\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ms_p_gothic[] =
    "MINGLIU.TTC,PMingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "GULIM.TTC,Gulim\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ms_ui_gothic[] =
    "MICROSS.TTF,Microsoft Sans Serif\0"
    "MINGLIU.TTC,PMingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "GULIM.TTC,Gulim\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_microsoft_jheng_hei[] =
    "SEGOEUI.TTF,Segoe UI\0"
    "MINGLIU.TTC,MingLiU\0"
    "MSYH.TTC,Microsoft YaHei\0"
    "MEIRYO.TTC,Meiryo\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_microsoft_jheng_hei_bold[] =
    "SEGOEUIB.TTF,Segoe UI Bold\0"
    "MINGLIU.TTC,MingLiU\0"
    "MSYHBD.TTC,Microsoft YaHei Bold\0"
    "MEIRYOB.TTC,Meiryo Bold\0"
    "MALGUNBD.TTF,Malgun Gothic Bold\0"
    "YUGOTHB.TTC,Yu Gothic UI Bold\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_microsoft_jheng_hei_ui[] =
    "SEGOEUI.TTF,Segoe UI\0"
    "MINGLIU.TTC,MingLiU\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MEIRYO.TTC,Meiryo UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_microsoft_jheng_hei_ui_bold[] =
    "SEGOEUIB.TTF,Segoe UI Bold\0"
    "MINGLIU.TTC,MingLiU\0"
    "MSYHBD.TTC,Microsoft YaHei UI Bold\0"
    "MEIRYOB.TTC,Meiryo UI Bold\0"
    "MALGUNBD.TTF,Malgun Gothic Bold\0"
    "YUGOTHB.TTC,Yu Gothic UI Bold\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_microsoft_jheng_hei_ui_light[] =
    "SEGOEUIL.TTF,Segoe UI Light\0"
    "MINGLIU.TTC,MingLiU\0"
    "MSYHL.TTC,Microsoft YaHei UI Light\0"
    "MEIRYO.TTC,Meiryo UI\0"
    "MALGUNSL.TTF,Malgun Gothic Semilight\0"
    "YUGOTHL.TTC,Yu Gothic UI Light\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ming_li_u[] =
    "MICROSS.TTF,Microsoft Sans Serif\0"
    "SIMSUN.TTC,SimSun\0"
    "MSMINCHO.TTC,MS Mincho\0"
    "BATANG.TTC,BatangChe\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_p_ming_li_u[] =
    "MICROSS.TTF,Microsoft Sans Serif\0"
    "SIMSUN.TTC,SimSun\0"
    "MSMINCHO.TTC,MS PMincho\0"
    "BATANG.TTC,Batang\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ming_li_u_hkscs[] =
    "MICROSS.TTF,Microsoft Sans Serif\0"
    "MINGLIU.TTC,MingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "MSMINCHO.TTC,MS Mincho\0"
    "BATANG.TTC,BatangChe\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ming_li_u_ext_b[] =
    "MICROSS.TTF,Microsoft Sans Serif\0"
    "MINGLIU.TTC,MingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "MSMINCHO.TTC,MS Mincho\0"
    "BATANG.TTC,BatangChe\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_p_ming_li_u_ext_b[] =
    "MICROSS.TTF,Microsoft Sans Serif\0"
    "MINGLIU.TTC,PMingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "MSMINCHO.TTC,MS PMincho\0"
    "BATANG.TTC,Batang\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ming_li_u_hkscs_ext_b[] =
    "MICROSS.TTF,Microsoft Sans Serif\0"
    "MINGLIU.TTC,MingLiU_HKSCS\0"
    "MINGLIU.TTC,MingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "MSMINCHO.TTC,MS Mincho\0"
    "BATANG.TTC,BatangChe\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_yu_gothic_ui[] =
    "SEGOEUI.TTF,Segoe UI\0"
    "MSJH.TTC,Microsoft JhengHei\0"
    "MSYH.TTC,Microsoft YaHei\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_yu_gothic_ui_bold[] =
    "SEGOEUIB.TTF,Segoe UI Bold\0"
    "MSJHBD.TTC,Microsoft Jhenghei UI Bold\0"
    "MSYHBD.TTC,Microsoft YaHei Bold\0"
    "MALGUNBD.TTF,Malgun Gothic Bold\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_yu_gothic_ui_light[] =
    "SEGOEUIL.TTF,Segoe UI Light\0"
    "MSJHL.TTC,Microsoft Jhenghei UI Light\0"
    "MSYHL.TTC,Microsoft YaHei Light\0"
    "MALGUNSL.TTF,Malgun Gothic Semilight\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_yu_gothic_ui_semilight[] =
    "SEGOEUISL.TTF,Segoe UI Semilight\0"
    "MSJH.TTC,Microsoft Jhenghei UI\0"
    "MSYH.TTC,Microsoft YaHei\0"
    "MALGUNSL.TTF,Malgun Gothic Semilight\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_yu_gothic_ui_semibold[] =
    "SEGUISB.TTF,Segoe UI Semibold\0"
    "MSJH.TTC,Microsoft Jhenghei UI\0"
    "MSYH.TTC,Microsoft YaHei\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_meiryo[] =
    "SEGOEUI.TTF,Segoe UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "MSJH.TTC,Microsoft JhengHei\0"
    "MSYH.TTC,Microsoft YaHei\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_meiryo_bold[] =
    "SEGOEUIB.TTF,Segoe UI Bold\0"
    "YUGOTHB.TTC,Yu Gothic UI Bold\0"
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "MSJHBD.TTC,Microsoft Jhenghei Bold\0"
    "MSYHBD.TTC,Microsoft YaHei Bold\0"
    "MALGUNBD.TTF,Malgun Gothic Bold\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_meiryo_ui[] =
    "SEGOEUI.TTF,Segoe UI\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "MSJH.TTC,Microsoft Jhenghei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_meiryo_ui_bold[] =
    "SEGOEUIB.TTF,Segoe UI Bold\0"
    "YUGOTHB.TTC,Yu Gothic UI Bold\0"
    "MSGOTHIC.TTC,MS UI Gothic\0"
    "MSJHBD.TTC,Microsoft Jhenghei UI Bold\0"
    "MSYHBD.TTC,Microsoft YaHei UI Bold\0"
    "MALGUNBD.TTF,Malgun Gothic Bold\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ms_mincho[] =
    "MINGLIU.TTC,MingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "BATANG.TTC,Batang\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const char system_link_ms_p_mincho[] =
    "MINGLIU.TTC,PMingLiU\0"
    "SIMSUN.TTC,SimSun\0"
    "BATANG.TTC,Batang\0"
    "YUGOTHM.TTC,Yu Gothic UI\0"
    "MSJH.TTC,Microsoft JhengHei UI\0"
    "MSYH.TTC,Microsoft YaHei UI\0"
    "MALGUN.TTF,Malgun Gothic\0"
    "SEGUISYM.TTF,Segoe UI Symbol\0";

static const struct system_link_reg
{
    const WCHAR *font_name;
    BOOL locale_dependent;
    const char *link_non_cjk;
    DWORD link_non_cjk_len;
    const char *link_sc;
    DWORD link_sc_len;
    const char *link_tc;
    DWORD link_tc_len;
    const char *link_jp;
    DWORD link_jp_len;
    const char *link_kr;
    DWORD link_kr_len;
}
default_system_link[] =
{
    {
        tahomaW, TRUE,
        system_link_tahoma_non_cjk, sizeof(system_link_tahoma_non_cjk),
        system_link_tahoma_sc,      sizeof(system_link_tahoma_sc),
        system_link_tahoma_tc,      sizeof(system_link_tahoma_tc),
        system_link_tahoma_jp,      sizeof(system_link_tahoma_jp),
        system_link_tahoma_kr,      sizeof(system_link_tahoma_kr),
    },
    {
        microsoft_sans_serifW, TRUE,
        system_link_tahoma_non_cjk, sizeof(system_link_tahoma_non_cjk),
        system_link_tahoma_sc,      sizeof(system_link_tahoma_sc),
        system_link_tahoma_tc,      sizeof(system_link_tahoma_tc),
        system_link_tahoma_jp,      sizeof(system_link_tahoma_jp),
        system_link_tahoma_kr,      sizeof(system_link_tahoma_kr),
    },
    {
        lucida_sans_unicodeW, TRUE,
        system_link_tahoma_non_cjk, sizeof(system_link_tahoma_non_cjk),
        system_link_tahoma_sc,      sizeof(system_link_tahoma_sc),
        system_link_tahoma_tc,      sizeof(system_link_tahoma_tc),
        system_link_tahoma_jp,      sizeof(system_link_tahoma_jp),
        system_link_tahoma_kr,      sizeof(system_link_tahoma_kr),
    },
    { ms_gothicW,                    FALSE, system_link_ms_gothic,                    sizeof(system_link_ms_gothic) },
    { ms_p_gothicW,                  FALSE, system_link_ms_p_gothic,                  sizeof(system_link_ms_p_gothic) },
    { ms_ui_gothicW,                 FALSE, system_link_ms_ui_gothic,                 sizeof(system_link_ms_ui_gothic) },
    { microsoft_jheng_heiW,          FALSE, system_link_microsoft_jheng_hei,          sizeof(system_link_microsoft_jheng_hei) },
    { microsoft_jheng_hei_boldW,     FALSE, system_link_microsoft_jheng_hei_bold,     sizeof(system_link_microsoft_jheng_hei_bold) },
    { microsoft_jheng_hei_uiW,       FALSE, system_link_microsoft_jheng_hei_ui,       sizeof(system_link_microsoft_jheng_hei_ui) },
    { microsoft_jheng_hei_ui_boldW,  FALSE, system_link_microsoft_jheng_hei_ui_bold,  sizeof(system_link_microsoft_jheng_hei_ui_bold) },
    { microsoft_jheng_hei_ui_lightW, FALSE, system_link_microsoft_jheng_hei_ui_light, sizeof(system_link_microsoft_jheng_hei_ui_light) },
    { ming_li_uW,                    FALSE, system_link_ming_li_u,                    sizeof(system_link_ming_li_u) },
    { p_ming_li_uW,                  FALSE, system_link_p_ming_li_u,                  sizeof(system_link_p_ming_li_u) },
    { ming_li_u_hkscsW,              FALSE, system_link_ming_li_u_hkscs,              sizeof(system_link_ming_li_u_hkscs) },
    { ming_li_u_ext_bW,              FALSE, system_link_ming_li_u_ext_b,              sizeof(system_link_ming_li_u_ext_b) },
    { p_ming_li_u_ext_bW,            FALSE, system_link_p_ming_li_u_ext_b,            sizeof(system_link_p_ming_li_u_ext_b) },
    { ming_li_u_hkscs_ext_bW,        FALSE, system_link_ming_li_u_hkscs_ext_b,        sizeof(system_link_ming_li_u_hkscs_ext_b) },
    { yu_gothic_uiW,                 FALSE, system_link_yu_gothic_ui,                 sizeof(system_link_yu_gothic_ui) },
    { yu_gothic_ui_boldW,            FALSE, system_link_yu_gothic_ui_bold,            sizeof(system_link_yu_gothic_ui_bold) },
    { yu_gothic_ui_lightW,           FALSE, system_link_yu_gothic_ui_light,           sizeof(system_link_yu_gothic_ui_light) },
    { yu_gothic_ui_semiboldW,        FALSE, system_link_yu_gothic_ui_semibold,        sizeof(system_link_yu_gothic_ui_semibold) },
    { yu_gothic_ui_semilightW,       FALSE, system_link_yu_gothic_ui_semilight,       sizeof(system_link_yu_gothic_ui_semilight) },
    { meiryoW,                       FALSE, system_link_meiryo,                       sizeof(system_link_meiryo) },
    { meiryo_boldW,                  FALSE, system_link_meiryo_bold,                  sizeof(system_link_meiryo_bold) },
    { meiryo_uiW,                    FALSE, system_link_meiryo_ui,                    sizeof(system_link_meiryo_ui) },
    { meiryo_ui_boldW,               FALSE, system_link_meiryo_ui_bold,               sizeof(system_link_meiryo_ui_bold) },
    { ms_minchoW,                    FALSE, system_link_ms_mincho,                    sizeof(system_link_ms_mincho) },
    { ms_p_minchoW,                  FALSE, system_link_ms_p_mincho,                  sizeof(system_link_ms_p_mincho) },
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

    static const WCHAR ms_shell_dlgW[] = {'M','S',' ','S','h','e','l','l',' ','D','l','g',0};
    static const WCHAR systemW[] = {'S','y','s','t','e','m',0};
    static const WCHAR tahoma_ttfW[] = {'t','a','h','o','m','a','.','t','t','f',0};

    if ((hkey = reg_open_key( NULL, system_link_keyW, sizeof(system_link_keyW) )))
    {
        char buffer[4096];
        KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
        WCHAR value[MAX_PATH];
        WCHAR *entry, *next;

        i = 0;
        while (reg_enum_value( hkey, i++, info, sizeof(buffer), value, sizeof(value) ))
        {
            /* Don't store fonts that are only substitutes for other fonts */
            if (!get_gdi_font_subst( value, -1, NULL ))
            {
                char *data = (char *)info + info->DataOffset;
                font_link = add_gdi_font_link( value );
                for (entry = (WCHAR *)data; (char *)entry < data + info->DataLength && *entry; entry = next)
                {
                    const WCHAR *family_name = NULL;
                    WCHAR *p;

                    TRACE( "%s: %s\n", debugstr_w(value), debugstr_w(entry) );

                    next = entry + lstrlenW(entry) + 1;
                    if ((p = wcschr( entry, ',' )))
                    {
                        *p++ = 0;
                        while (*p == ' ' || *p == '\t') p++;
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
        }
        NtClose( hkey );
    }

    if ((shelldlg_name = get_gdi_font_subst( ms_shell_dlgW, -1, NULL )))
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

    system_font_link = add_gdi_font_link( systemW );
    if ((face = find_face_from_filename( tahoma_ttfW, tahomaW )))
    {
        add_gdi_font_link_entry( system_font_link, face->family->family_name, face->fs );
        TRACE("Found Tahoma in %s index %u\n", debugstr_w(face->file), face->face_index);
    }
    if ((font_link = find_gdi_font_link( tahomaW )))
    {
        struct gdi_font_link_entry *entry;
        LIST_FOR_EACH_ENTRY( entry, &font_link->links, struct gdi_font_link_entry, entry )
            add_gdi_font_link_entry( system_font_link, entry->family_name, entry->fs );
    }
}

/* see TranslateCharsetInfo */
BOOL translate_charset_info( DWORD *src, CHARSETINFO *cs, DWORD flags )
{
    unsigned int i;

    switch (flags)
    {
    case TCI_SRCFONTSIG:
        for (i = 0; i < ARRAY_SIZE(charset_info); i++)
            if (charset_info[i].fs.fsCsb[0] & src[0]) goto found;
        return FALSE;
    case TCI_SRCCODEPAGE:
        for (i = 0; i < ARRAY_SIZE(charset_info); i++)
            if (PtrToUlong(src) == charset_info[i].ciACP) goto found;
        return FALSE;
    case TCI_SRCCHARSET:
        for (i = 0; i < ARRAY_SIZE(charset_info); i++)
            if (PtrToUlong(src) == charset_info[i].ciCharset) goto found;
        return FALSE;
    default:
        return FALSE;
    }
found:
    *cs = charset_info[i];
    return TRUE;
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
                                                         BOOL can_use_bitmap, const WCHAR **orig_name )
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;

    family = find_family_from_any_name( name );
    if (family && (face = find_best_matching_face( family, lf, fs, can_use_bitmap ))) goto found;
    if (subst)
    {
        family = find_family_from_any_name( subst );
        if (family && (face = find_best_matching_face( family, lf, fs, can_use_bitmap ))) goto found;
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

found:
    if (orig_name && family != face->family)
        *orig_name = family->family_name;
    return face;
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
                                                 BOOL *substituted, const WCHAR **orig_name )
{
    BOOL want_vertical = (lf->lfFaceName[0] == '@');
    struct gdi_font_face *face;

    if (!translate_charset_info( (DWORD *)(INT_PTR)lf->lfCharSet, csi, TCI_SRCCHARSET ))
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
                translate_charset_info( (DWORD *)(INT_PTR)subst_charset, csi, TCI_SRCCHARSET );
            *substituted = TRUE;
	}

        if ((face = find_matching_face_by_name( lf->lfFaceName, subst, lf, csi->fs, can_use_bitmap, orig_name )))
            return face;
    }
    *substituted = FALSE; /* substitution is no longer relevant */

    /* If requested charset was DEFAULT_CHARSET then try using charset
       corresponding to the current ansi codepage */
    if (!csi->fs.fsCsb[0])
    {
        if (!translate_charset_info( (DWORD *)(INT_PTR)ansi_cp.CodePage, csi, TCI_SRCCODEPAGE ))
        {
            FIXME( "TCI failed on codepage %d\n", ansi_cp.CodePage );
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

static struct font_handle_entry *handle_entry( unsigned int handle )
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

static struct gdi_font *get_font_from_handle( unsigned int handle )
{
    struct font_handle_entry *entry = handle_entry( handle );

    if (entry) return entry->font;
    RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
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
    struct gdi_font *font = calloc( 1, offsetof( struct gdi_font, file[len + 1] ));

    font->refcount = 1;
    font->matrix.eM11 = font->matrix.eM22 = 1.0;
    font->scale_y = 1;
    font->kern_count = -1;
    list_init( &font->child_fonts );

    if (file)
    {
        FILE_NETWORK_OPEN_INFORMATION info;
        UNICODE_STRING nt_name;
        OBJECT_ATTRIBUTES attr;

        nt_name.Buffer = (WCHAR *)file;
        nt_name.Length = nt_name.MaximumLength = len * sizeof(WCHAR);

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.Attributes = OBJ_CASE_INSENSITIVE;
        attr.ObjectName = &nt_name;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;

        if (!NtQueryFullAttributesFile( &attr, &info ))
        {
            font->writetime.dwLowDateTime  = info.LastWriteTime.LowPart;
            font->writetime.dwHighDateTime = info.LastWriteTime.HighPart;
            font->data_size = info.EndOfFile.QuadPart;
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
    for (i = 0; i < font->gm_size; i++) free( font->gm[i] );
    free( font->otm.otmpFamilyName );
    free( font->otm.otmpStyleName );
    free( font->otm.otmpFaceName );
    free( font->otm.otmpFullName );
    free( font->gm );
    free( font->kern_pairs );
    free( font->gsub_table );
    free( font );
}

static inline const WCHAR *get_gdi_font_name( struct gdi_font *font )
{
    return font->use_logfont_name ? font->lf.lfFaceName : (WCHAR *)font->otm.otmpFamilyName;
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
    font->otm.otmpFamilyName = (char *)wcsdup( family_name );
    font->otm.otmpStyleName = (char *)wcsdup( face->style_name );
    font->otm.otmpFaceName = (char *)wcsdup( face->full_name );
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

        if (!(ptr = realloc( font->gm, (block + 1) * sizeof(*ptr) ))) return;
        memset( ptr + font->gm_size, 0, (block + 1 - font->gm_size) * sizeof(*ptr) );
        font->gm_size = block + 1;
        font->gm = ptr;
    }
    if (!font->gm[block])
    {
        font->gm[block] = calloc( sizeof(**font->gm), GM_BLOCK_SIZE );
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
    UINT length = font_funcs->get_font_data( font, MS_GSUB_TAG, 0, NULL, 0 );

    if (length == GDI_ERROR) return NULL;

    header = malloc( length );
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

    free( header );
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

    if (!(face = find_matching_face_by_name( family_name, NULL, &font->lf, fs, FALSE, NULL ))) return;

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
    const WCHAR* font_name = (WCHAR *)font->otm.otmpFaceName;

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
    if (ansi_cp.MaximumCharacterSize == 2 && font->charset != SYMBOL_CHARSET && font->charset != OEM_CHARSET &&
        facename_compare( font_name, microsoft_sans_serifW, -1 ) != 0)
    {
        if ((font_link = find_gdi_font_link( microsoft_sans_serifW )))
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

    TRACE( "font %p\n", font );

    /* add it to the unused list */
    pthread_mutex_lock( &font_lock );
    if (!--font->refcount)
    {
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
    }
    pthread_mutex_unlock( &font_lock );
}

static void add_font_list(HKEY hkey, const struct nls_update_font_list *fl, int dpi)
{
    const char *sserif = (dpi <= 108) ? fl->sserif_96 : fl->sserif_120;

    set_reg_ascii_value( hkey, "Courier", fl->courier );
    set_reg_ascii_value( hkey, "MS Serif", fl->serif );
    set_reg_ascii_value( hkey, "MS Sans Serif", sserif );
    set_reg_ascii_value( hkey, "Small Fonts", fl->small );
}

static void set_value_key(HKEY hkey, const char *name, const char *value)
{
    if (value)
        set_reg_ascii_value( hkey, name, value );
    else if (name)
    {
        WCHAR nameW[64];
        asciiz_to_unicode( nameW, name );
        reg_delete_value( hkey, nameW );
    }
}

static void update_font_association_info(void)
{
    static const WCHAR associated_charsetW[] =
        { 'A','s','s','o','c','i','a','t','e','d',' ','C','h','a','r','s','e','t' };

    if (ansi_cp.MaximumCharacterSize == 2)
    {
        HKEY hkey;
        if ((hkey = reg_create_key( NULL, font_assoc_keyW, sizeof(font_assoc_keyW), 0, NULL )))
        {
            HKEY hsubkey;
            if ((hsubkey = reg_create_key( hkey, associated_charsetW, sizeof(associated_charsetW),
                                           0, NULL )))
            {
                switch (ansi_cp.CodePage)
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
                NtClose( hsubkey );
            }

            /* TODO: Associated DefaultFonts */

            NtClose( hkey );
        }
    }
    else
        reg_delete_tree( NULL, font_assoc_keyW, sizeof(font_assoc_keyW) );
}

static void set_multi_value_key( HKEY hkey, const WCHAR *name, const char *value, UINT len )
{
    WCHAR *valueW;

    if (!(valueW = malloc( len * sizeof(WCHAR) )))
    {
        ERR( "malloc of %d * WCHAR failed\n", len );
        return;
    }
    ascii_to_unicode( valueW, value, len );
    if (value)
        set_reg_value( hkey, name, REG_MULTI_SZ, valueW, len * sizeof(WCHAR) );
    else if (name)
        reg_delete_value( hkey, name );
    free( valueW );
}

static void update_font_system_link_info(void)
{
    HKEY hkey;

    if ((hkey = reg_create_key( NULL, system_link_keyW, sizeof(system_link_keyW), 0, NULL )))
    {
        const char *link;
        DWORD len, i;

        for (i = 0; i < ARRAY_SIZE(default_system_link); ++i)
        {
            const struct system_link_reg *link_reg = &default_system_link[i];

            link = link_reg->link_non_cjk;
            len = link_reg->link_non_cjk_len;

            if (link_reg->locale_dependent)
            {
                switch (ansi_cp.CodePage)
                {
                case 932:
                    link = link_reg->link_jp;
                    len = link_reg->link_jp_len;
                    break;
                case 936:
                    link = link_reg->link_sc;
                    len = link_reg->link_sc_len;
                    break;
                case 949:
                    link = link_reg->link_kr;
                    len = link_reg->link_kr_len;
                    break;
                case 950:
                    link = link_reg->link_tc;
                    len = link_reg->link_tc_len;
                    break;
                }
            }
            set_multi_value_key(hkey, link_reg->font_name, link, len);
        }
        NtClose( hkey );
    }
}

static void update_codepage( UINT screen_dpi )
{
    USHORT utf8_hdr[2] = { 0, CP_UTF8 };
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[40 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)value_buffer;
    char cpbuf[40];
    WCHAR cpbufW[40];
    HKEY hkey;
    DWORD size;
    UINT i;
    UINT font_dpi = 0;
    BOOL done = FALSE, cp_match = FALSE;

    static const WCHAR log_pixelsW[] = {'L','o','g','P','i','x','e','l','s',0};

    size = query_reg_value( wine_fonts_key, log_pixelsW, info, sizeof(value_buffer) );
    if (size == sizeof(DWORD) && info->Type == REG_DWORD)
        font_dpi = *(DWORD *)info->Data;

    RtlInitCodePageTable( utf8_hdr, &utf8_cp );
    if (NtCurrentTeb()->Peb->AnsiCodePageData)
        RtlInitCodePageTable( NtCurrentTeb()->Peb->AnsiCodePageData, &ansi_cp );
    else
        ansi_cp = utf8_cp;
    if (NtCurrentTeb()->Peb->OemCodePageData)
        RtlInitCodePageTable( NtCurrentTeb()->Peb->OemCodePageData, &oem_cp );
    else
        oem_cp = utf8_cp;
    sprintf( cpbuf, "%u,%u", ansi_cp.CodePage, oem_cp.CodePage );
    asciiz_to_unicode( cpbufW, cpbuf );

    if (query_reg_ascii_value( wine_fonts_key, "Codepages", info, sizeof(value_buffer) ))
    {
        cp_match = !wcscmp( (const WCHAR *)info->Data, cpbufW );
        if (cp_match && screen_dpi == font_dpi) return;  /* already set correctly */
        TRACE( "updating registry, codepages/logpixels changed %s/%u -> %u,%u/%u\n",
               debugstr_w((const WCHAR *)info->Data), font_dpi, ansi_cp.CodePage, oem_cp.CodePage, screen_dpi );
    }
    else TRACE("updating registry, codepages/logpixels changed none -> %u,%u/%u\n",
               ansi_cp.CodePage, oem_cp.CodePage, screen_dpi);

    set_reg_ascii_value( wine_fonts_key, "Codepages", cpbuf );
    set_reg_value( wine_fonts_key, log_pixelsW, REG_DWORD, &screen_dpi, sizeof(screen_dpi) );

    for (i = 0; i < ARRAY_SIZE(nls_update_font_list); i++)
    {
        if (nls_update_font_list[i].ansi_cp == ansi_cp.CodePage &&
            nls_update_font_list[i].oem_cp == oem_cp.CodePage)
        {
            HKEY software_hkey;
            if ((software_hkey = reg_create_key( NULL, software_config_keyW,
                                                 sizeof(software_config_keyW), 0, NULL )))
            {
                static const WCHAR fontsW[] = {'F','o','n','t','s'};
                hkey = reg_create_key( software_hkey, fontsW, sizeof(fontsW), 0, NULL );
                NtClose( software_hkey );
                if (hkey)
                {
                    set_reg_ascii_value( hkey, "OEMFONT.FON", nls_update_font_list[i].oem );
                    set_reg_ascii_value( hkey, "FIXEDFON.FON", nls_update_font_list[i].fixed );
                    set_reg_ascii_value( hkey, "FONTS.FON", nls_update_font_list[i].system );
                    NtClose( hkey );
                }
            }
            if ((hkey = reg_create_key( NULL, fonts_winnt_config_keyW, sizeof(fonts_winnt_config_keyW),
                                        0, NULL )))
            {
                add_font_list(hkey, &nls_update_font_list[i], screen_dpi);
                NtClose( hkey );
            }
            if ((hkey = reg_create_key( NULL, fonts_win9x_config_keyW,
                                        sizeof(fonts_win9x_config_keyW), 0, NULL )))
            {
                add_font_list(hkey, &nls_update_font_list[i], screen_dpi);
                NtClose( hkey );
            }
            /* Only update these if the Codepage changed. */
            if (!cp_match &&
                (hkey = reg_create_key( NULL, font_substitutes_keyW, sizeof(font_substitutes_keyW),
                                        0, NULL )))
            {
                set_reg_ascii_value( hkey, "MS Shell Dlg", nls_update_font_list[i].shelldlg );
                set_reg_ascii_value( hkey, "Tms Rmn", nls_update_font_list[i].tmsrmn );

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

                NtClose( hkey );
            }
            done = TRUE;
        }
        else
        {
            /* Delete the FontSubstitutes from other locales */
            if ((hkey = reg_create_key( NULL, font_substitutes_keyW, sizeof(font_substitutes_keyW),
                                        0, NULL )))
            {
                set_value_key(hkey, nls_update_font_list[i].arial_0.from, NULL);
                set_value_key(hkey, nls_update_font_list[i].courier_new_0.from, NULL);
                set_value_key(hkey, nls_update_font_list[i].times_new_roman_0.from, NULL);
                NtClose( hkey );
            }
        }
    }
    if (!done)
        FIXME("there is no font defaults for codepages %u,%u\n", ansi_cp.CodePage, oem_cp.CodePage);

    /* update locale dependent font association info and font system link info in registry.
       update only when codepages changed, not logpixels. */
    if (!cp_match)
    {
        update_font_association_info();
        update_font_system_link_info();
    }
}


/*************************************************************
 * font_CreateDC
 */
static BOOL font_CreateDC( PHYSDEV *dev, LPCWSTR device, LPCWSTR output, const DEVMODEW *devmode )
{
    struct font_physdev *physdev;

    if (!font_funcs) return TRUE;
    if (!(physdev = calloc( 1, sizeof(*physdev) ))) return FALSE;
    push_dc_driver( dev, &physdev->dev, &font_driver );
    return TRUE;
}


/*************************************************************
 * font_DeleteDC
 */
static BOOL font_DeleteDC( PHYSDEV dev )
{
    struct font_physdev *physdev = get_font_dev( dev );

    release_gdi_font( physdev->font );
    free( physdev );
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

static BOOL is_complex_script_ansi_cp(void)
{
    return (ansi_cp.CodePage == 874 /* Thai */
            || ansi_cp.CodePage == 1255 /* Hebrew */
            || ansi_cp.CodePage == 1256 /* Arabic */
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

    if (translate_charset_info( ULongToPtr(charset), &csi, TCI_SRCCHARSET ) && csi.fs.fsCsb[0] != 0)
    {
        list->mask    = csi.fs.fsCsb[0];
        list->charset = csi.ciCharset;
        for (i = 0; i < 32; i++) if (csi.fs.fsCsb[0] & (1u << i)) list->script = i;
        list++;
    }
    else /* charset is DEFAULT_CHARSET or invalid. */
    {
        DWORD mask = 0;

        /* Set the current codepage's charset as the first element. */
        if (!is_complex_script_ansi_cp() &&
            translate_charset_info( (DWORD *)(INT_PTR)ansi_cp.CodePage, &csi, TCI_SRCCODEPAGE ) &&
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
            if (!translate_charset_info( fs.fsCsb, &csi, TCI_SRCFONTSIG ))
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
            list->script  = 33; /* other */
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
#define SCALE_NTM(value) (muldiv( ntm->ntmTm.tmHeight, (value), TM.tmHeight ))
        cell_height = TM.tmHeight / ( -lf.lfHeight / font->otm.otmEMSquare );
        ntm->ntmTm.tmHeight = muldiv( ntm_ppem, cell_height, font->otm.otmEMSquare );
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
                                struct enum_charset *list, DWORD count, font_enum_proc proc, LPARAM lparam,
                                const WCHAR *subst )
{
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
    UINT type, i;

    if (!face->cached_enum_data)
    {
        struct gdi_font_enum_data *data;

        if (!(data = calloc( 1, sizeof(*data) )) ||
            !get_face_enum_data( face, &data->elf, &data->ntm ))
        {
            free( data );
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
            elf.elfScript[0] = 32;
            i = count; /* break out of loop after enumeration */
        }
        else
        {
            if (!(face->fs.fsCsb[0] & list[i].mask)) continue;
            /* use the DEFAULT_CHARSET case only if no other charset is present */
            if (list[i].charset == DEFAULT_CHARSET && (face->fs.fsCsb[0] & ~list[i].mask)) continue;
            elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet = list[i].charset;
            /* caller may fill elfScript with the actual string, see load_script_name */
            elf.elfScript[0] = list[i].script;
        }
        TRACE( "face %s full %s style %s charset = %d type %d script %s it %d weight %d ntmflags %08x\n",
               debugstr_w(elf.elfLogFont.lfFaceName), debugstr_w(elf.elfFullName), debugstr_w(elf.elfStyle),
               elf.elfLogFont.lfCharSet, type, debugstr_w(elf.elfScript),
               elf.elfLogFont.lfItalic, (int)elf.elfLogFont.lfWeight, (int)ntm.ntmTm.ntmFlags );
        /* release section before callback (FIXME) */
        pthread_mutex_unlock( &font_lock );
        if (!proc( &elf.elfLogFont, (TEXTMETRICW *)&ntm, type, lparam )) return FALSE;
        pthread_mutex_lock( &font_lock );
    }
    return TRUE;
}

/*************************************************************
 * font_EnumFonts
 */
static BOOL font_EnumFonts( PHYSDEV dev, LOGFONTW *lf, font_enum_proc proc, LPARAM lparam )
{
    struct gdi_font_family *family;
    struct gdi_font_face *face;
    struct enum_charset enum_charsets[32];
    UINT count, charset;

    charset = lf ? lf->lfCharSet : DEFAULT_CHARSET;

    count = create_enum_charset_list( charset, enum_charsets );

    pthread_mutex_lock( &font_lock );

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
                    return FALSE; /* enum_face_charsets() unlocked font_lock */
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
                return FALSE; /* enum_face_charsets() unlocked font_lock */
	}
    }
    pthread_mutex_unlock( &font_lock );
    return TRUE;
}


static BOOL check_unicode_tategaki( WCHAR ch )
{
    extern const unsigned short vertical_orientation_table[];
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

CPTABLEINFO *get_cptable( WORD cp )
{
    static CPTABLEINFO tables[100];
    unsigned int i;
    USHORT *ptr;
    SIZE_T size;

    if (cp == CP_ACP) return &ansi_cp;
    if (cp == CP_UTF8) return &utf8_cp;

    for (i = 0; i < ARRAY_SIZE(tables) && tables[i].CodePage; i++)
        if (tables[i].CodePage == cp) return &tables[i];
    if (NtGetNlsSectionPtr( 11, cp, NULL, (void **)&ptr, &size )) return NULL;
    if (i == ARRAY_SIZE(tables))
    {
        ERR( "too many code pages\n" );
        return NULL;
    }
    RtlInitCodePageTable( ptr, &tables[i] );
    return &tables[i];
}

/* Based on NlsValidateLocale */
const NLS_LOCALE_DATA *get_locale_data( LCID lcid )
{
    static const NLS_LOCALE_HEADER *locale_table;
    static const NLS_LOCALE_LCID_INDEX *lcids_index;
    int min = 0, max;

    if (!locale_table)
    {
        LARGE_INTEGER size;
        void *addr;
        LCID lcid;
        NTSTATUS status;
        static struct
        {
            UINT ctypes;
            UINT unknown1;
            UINT unknown2;
            UINT unknown3;
            UINT locales;
            UINT charmaps;
            UINT geoids;
            UINT scripts;
        } *header;

        status = NtInitializeNlsFiles( &addr, &lcid, &size );
        if (status)
        {
            ERR( "Failed to load nls file\n" );
            return NULL;
        }

        if (InterlockedCompareExchangePointer( (void **)&header, addr, NULL ))
            NtUnmapViewOfSection( GetCurrentProcess(), addr );

        locale_table = (const NLS_LOCALE_HEADER *)((char *)header + header->locales);
        lcids_index = (const NLS_LOCALE_LCID_INDEX *)((char *)locale_table + locale_table->lcids_offset);
    }

    max = locale_table->nb_lcids - 1;
    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (lcid < lcids_index[pos].id) max = pos - 1;
        else if (lcid > lcids_index[pos].id) min = pos + 1;
        else
        {
            ULONG offset = locale_table->locales_offset + pos * locale_table->locale_size;
            return (const NLS_LOCALE_DATA *)((const char *)locale_table + offset);
        }
    }
    return NULL;
}

DWORD win32u_wctomb( CPTABLEINFO *info, char *dst, DWORD dstlen, const WCHAR *src, DWORD srclen )
{
    DWORD ret;

    if (info->CodePage == CP_UTF8)
        RtlUnicodeToUTF8N( dst, dstlen, &ret, src, srclen * sizeof(WCHAR) );
    else
        RtlUnicodeToCustomCPN( info, dst, dstlen, &ret, src, srclen * sizeof(WCHAR) );

    return ret;
}

DWORD win32u_wctomb_size( CPTABLEINFO *info, const WCHAR *src, DWORD srclen )
{
    DWORD ret;

    if (info->CodePage == CP_UTF8)
    {
        RtlUnicodeToUTF8N( NULL, 0, &ret, src, srclen * sizeof(WCHAR) );
    }
    else if(info->DBCSCodePage)
    {
        WCHAR *uni2cp = info->WideCharTable;
        for (ret = srclen; srclen; srclen--, src++)
            if (uni2cp[*src] & 0xff00) ret++;
    }
    else
    {
        ret = srclen;
    }

    return ret;
}

DWORD win32u_mbtowc( CPTABLEINFO *info, WCHAR *dst, DWORD dstlen, const char *src, DWORD srclen )
{
    DWORD ret;

    if (info->CodePage == CP_UTF8)
        RtlUTF8ToUnicodeN( dst, dstlen * sizeof(WCHAR), &ret, src, srclen );
    else
        RtlCustomCPToUnicodeN( info, dst, dstlen * sizeof(WCHAR), &ret, src, srclen );

    return ret / sizeof(WCHAR);
}

DWORD win32u_mbtowc_size( CPTABLEINFO *info, const char *src, DWORD srclen )
{
    DWORD ret;

    if (info->CodePage == CP_UTF8)
    {
        RtlUTF8ToUnicodeN( NULL, 0, &ret, src, srclen );
        ret /= sizeof(WCHAR);
    }
    else if (info->DBCSCodePage)
    {
        for (ret = 0; srclen; srclen--, src++, ret++)
        {
            if (info->DBCSOffsets[(unsigned char)*src] && srclen > 1)
            {
                src++;
                srclen--;
            }
        }
    }
    else
    {
        ret = srclen;
    }

    return ret;
}

static BOOL wc_to_index( UINT cp, WCHAR wc, unsigned char *dst, BOOL allow_default )
{
    const CPTABLEINFO *info;

    if (!(info = get_cptable( cp ))) return FALSE;

    if (info->CodePage == CP_UTF8)
    {
        if (wc < 0x80)
        {
            *dst = wc;
            return TRUE;
        }
        if (!allow_default) return FALSE;
        *dst = info->DefaultChar;
        return TRUE;
    }
    else if (info->DBCSCodePage)
    {
        WCHAR *uni2cp = info->WideCharTable;
        if (uni2cp[wc] & 0xff00) return FALSE;
        *dst = uni2cp[wc];
    }
    else
    {
        char *uni2cp = info->WideCharTable;
        *dst = uni2cp[wc];
    }

    if (info->MultiByteTable[*dst] != wc)
    {
        if (!allow_default) return FALSE;
        *dst = info->DefaultChar;
    }

    return TRUE;
}

static UINT get_glyph_index( struct gdi_font *font, UINT glyph )
{
    WCHAR wc = glyph;
    unsigned char ch;

    if (font_funcs->get_glyph_index( font, &glyph, TRUE )) return glyph;

    if (font->codepage == CP_SYMBOL)
    {
        glyph = get_glyph_index_symbol( font, wc );
        if (!glyph)
        {
            if (wc_to_index( CP_ACP, wc, &ch, TRUE ))
                glyph = get_glyph_index_symbol( font, (unsigned char)ch );
        }
    }
    else if (wc_to_index( font->codepage, wc, &ch, FALSE ))
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

    if (format == GGO_METRICS && !mat)
        set_gdi_font_glyph_metrics( font, index, &gm, &abc );

done:
    if (gm_ret) *gm_ret = gm;
    if (abc_ret) *abc_ret = abc;
    return ret;
}


/*************************************************************
 * font_FontIsLinked
 */
static BOOL font_FontIsLinked( PHYSDEV dev )
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
static BOOL font_GetCharABCWidths( PHYSDEV dev, UINT first, UINT count, WCHAR *chars, ABC *buffer )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT c, i;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetCharABCWidths );
        return dev->funcs->pGetCharABCWidths( dev, first, count, chars, buffer );
    }

    TRACE( "%p, %u, %u, %p\n", physdev->font, first, count, buffer );

    pthread_mutex_lock( &font_lock );
    for (i = 0; i < count; i++)
    {
        c = chars ? chars[i] : first + i;
        get_glyph_outline( physdev->font, c, GGO_METRICS, NULL, &buffer[i], 0, NULL, NULL );
    }
    pthread_mutex_unlock( &font_lock );
    return TRUE;
}


/*************************************************************
 * font_GetCharABCWidthsI
 */
static BOOL font_GetCharABCWidthsI( PHYSDEV dev, UINT first, UINT count, WORD *gi, ABC *buffer )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT c;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetCharABCWidthsI );
        return dev->funcs->pGetCharABCWidthsI( dev, first, count, gi, buffer );
    }

    TRACE( "%p, %u, %u, %p\n", physdev->font, first, count, buffer );

    pthread_mutex_lock( &font_lock );
    for (c = 0; c < count; c++, buffer++)
        get_glyph_outline( physdev->font, gi ? gi[c] : first + c, GGO_METRICS | GGO_GLYPH_INDEX,
                           NULL, buffer, 0, NULL, NULL );
    pthread_mutex_unlock( &font_lock );
    return TRUE;
}


/*************************************************************
 * font_GetCharWidth
 */
static BOOL font_GetCharWidth( PHYSDEV dev, UINT first, UINT count, const WCHAR *chars, INT *buffer )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT c, i;
    ABC abc;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetCharWidth );
        return dev->funcs->pGetCharWidth( dev, first, count, chars, buffer );
    }

    TRACE( "%p, %d, %d, %p\n", physdev->font, first, count, buffer );

    pthread_mutex_lock( &font_lock );
    for (i = 0; i < count; i++)
    {
        c = chars ? chars[i] : i + first;
        if (get_glyph_outline( physdev->font, c, GGO_METRICS, NULL, &abc, 0, NULL, NULL ) == GDI_ERROR)
            buffer[i] = 0;
        else
            buffer[i] = abc.abcA + abc.abcB + abc.abcC;
    }
    pthread_mutex_unlock( &font_lock );
    return TRUE;
}


/*************************************************************
 * font_GetCharWidthInfo
 */
static BOOL font_GetCharWidthInfo( PHYSDEV dev, void *ptr )
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
static DWORD font_GetFontData( PHYSDEV dev, DWORD table, DWORD offset, void *buf, DWORD size )
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
static BOOL font_GetFontRealizationInfo( PHYSDEV dev, void *ptr )
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
        info->file_count = 1;
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
static DWORD font_GetFontUnicodeRanges( PHYSDEV dev, GLYPHSET *glyphset )
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
static DWORD font_GetGlyphIndices( PHYSDEV dev, const WCHAR *str, INT count, WORD *gi, DWORD flags )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT default_char;
    unsigned char ch;
    BOOL got_default = FALSE;
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

    pthread_mutex_lock( &font_lock );

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
            else if (wc_to_index( physdev->font->codepage, str[i], &ch, FALSE ))
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

    pthread_mutex_unlock( &font_lock );
    return count;
}


/*************************************************************
 * font_GetGlyphOutline
 */
static DWORD font_GetGlyphOutline( PHYSDEV dev, UINT glyph, UINT format,
                                   GLYPHMETRICS *gm, DWORD buflen, void *buf, const MAT2 *mat )
{
    struct font_physdev *physdev = get_font_dev( dev );
    DWORD ret;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetGlyphOutline );
        return dev->funcs->pGetGlyphOutline( dev, glyph, format, gm, buflen, buf, mat );
    }
    pthread_mutex_lock( &font_lock );
    ret = get_glyph_outline( physdev->font, glyph, format, gm, NULL, buflen, buf, mat );
    pthread_mutex_unlock( &font_lock );
    return ret;
}


/*************************************************************
 * font_GetKerningPairs
 */
static DWORD font_GetKerningPairs( PHYSDEV dev, DWORD count, KERNINGPAIR *pairs )
{
    struct font_physdev *physdev = get_font_dev( dev );

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetKerningPairs );
        return dev->funcs->pGetKerningPairs( dev, count, pairs );
    }

    pthread_mutex_lock( &font_lock );
    if (physdev->font->kern_count == -1)
        physdev->font->kern_count = font_funcs->get_kerning_pairs( physdev->font,
                                                                   &physdev->font->kern_pairs );
    pthread_mutex_unlock( &font_lock );

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
static UINT font_GetOutlineTextMetrics( PHYSDEV dev, UINT size, OUTLINETEXTMETRICW *metrics )
{
    struct font_physdev *physdev = get_font_dev( dev );
    UINT ret = 0;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetOutlineTextMetrics );
        return dev->funcs->pGetOutlineTextMetrics( dev, size, metrics );
    }

    if (!physdev->font->scalable) return 0;

    pthread_mutex_lock( &font_lock );
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
    pthread_mutex_unlock( &font_lock );
    return ret;
}


/*************************************************************
 * font_GetTextCharsetInfo
 */
static UINT font_GetTextCharsetInfo( PHYSDEV dev, FONTSIGNATURE *fs, DWORD flags )
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
static BOOL font_GetTextExtentExPoint( PHYSDEV dev, const WCHAR *str, INT count, INT *dxs )
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

    pthread_mutex_lock( &font_lock );
    for (i = pos = 0; i < count; i++)
    {
        get_glyph_outline( physdev->font, str[i], GGO_METRICS, NULL, &abc, 0, NULL, NULL );
        pos += abc.abcA + abc.abcB + abc.abcC;
        dxs[i] = pos;
    }
    pthread_mutex_unlock( &font_lock );
    return TRUE;
}


/*************************************************************
 * font_GetTextExtentExPointI
 */
static BOOL font_GetTextExtentExPointI( PHYSDEV dev, const WORD *indices, INT count, INT *dxs )
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

    pthread_mutex_lock( &font_lock );
    for (i = pos = 0; i < count; i++)
    {
        get_glyph_outline( physdev->font, indices[i], GGO_METRICS | GGO_GLYPH_INDEX,
                           NULL, &abc, 0, NULL, NULL );
        pos += abc.abcA + abc.abcB + abc.abcC;
        dxs[i] = pos;
    }
    pthread_mutex_unlock( &font_lock );
    return TRUE;
}


/*************************************************************
 * font_GetTextFace
 */
static INT font_GetTextFace( PHYSDEV dev, INT count, WCHAR *str )
{
    struct font_physdev *physdev = get_font_dev( dev );
    const WCHAR *font_name;
    INT len;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetTextFace );
        return dev->funcs->pGetTextFace( dev, count, str );
    }
    font_name = get_gdi_font_name( physdev->font );
    len = lstrlenW( font_name ) + 1;
    if (str)
    {
        lstrcpynW( str, font_name, count );
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
static BOOL font_GetTextMetrics( PHYSDEV dev, TEXTMETRICW *metrics )
{
    struct font_physdev *physdev = get_font_dev( dev );
    BOOL ret = FALSE;

    if (!physdev->font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pGetTextMetrics );
        return dev->funcs->pGetTextMetrics( dev, metrics );
    }

    pthread_mutex_lock( &font_lock );
    if (font_funcs->set_outline_text_metrics( physdev->font ) ||
        font_funcs->set_bitmap_text_metrics( physdev->font ))
    {
        *metrics = physdev->font->otm.otmTextMetrics;
        scale_font_metrics( physdev->font, metrics );
        ret = TRUE;
    }
    pthread_mutex_unlock( &font_lock );
    return ret;
}


static void get_nearest_charset( const WCHAR *family_name, struct gdi_font_face *face, CHARSETINFO *csi )
{
  /* Only get here if lfCharSet == DEFAULT_CHARSET or we couldn't find
     a single face with the requested charset.  The idea is to check if
     the selected font supports the current ANSI codepage, if it does
     return the corresponding charset, else return the first charset */

    int i;

    if (translate_charset_info( (DWORD*)(INT_PTR)ansi_cp.CodePage, csi, TCI_SRCCODEPAGE ))
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
	    if (translate_charset_info(&fs0, csi, TCI_SRCFONTSIG)) return;
            FIXME("TCI failing on %x\n", (int)fs0);
	}
    }

    FIXME("returning DEFAULT_CHARSET face->fs.fsCsb[0] = %08x file = %s\n",
	  (int)face->fs.fsCsb[0], debugstr_w(face->file));
    csi->ciACP = ansi_cp.CodePage;
    csi->ciCharset = DEFAULT_CHARSET;
}

static struct gdi_font *select_font( LOGFONTW *lf, FMAT2 dcmat, BOOL can_use_bitmap )
{
    struct gdi_font *font;
    struct gdi_font_face *face;
    INT height;
    CHARSETINFO csi;
    const WCHAR *orig_name = NULL;
    BOOL substituted = FALSE;

    static const WCHAR symbolW[] = {'S','y','m','b','o','l',0};

    /* If lfFaceName is "Symbol" then Windows fixes up lfCharSet to
       SYMBOL_CHARSET so that Symbol gets picked irrespective of the
       original value lfCharSet.  Note this is a special case for
       Symbol and doesn't happen at least for "Wingdings*" */
    if (!facename_compare( lf->lfFaceName, symbolW, -1 )) lf->lfCharSet = SYMBOL_CHARSET;

    /* check the cache first */
    if ((font = find_cached_gdi_font( lf, &dcmat, can_use_bitmap )))
    {
        TRACE( "returning cached gdiFont(%p)\n", font );
        return font;
    }
    if (!(face = find_matching_face( lf, &csi, can_use_bitmap, &substituted, &orig_name )))
    {
        FIXME( "can't find a single appropriate font - bailing\n" );
        return NULL;
    }
    height = lf->lfHeight;

    font = create_gdi_font( face, orig_name, lf );
    font->use_logfont_name = substituted;
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
static HFONT font_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    struct font_physdev *physdev = get_font_dev( dev );
    struct gdi_font *font = NULL, *prev = physdev->font;
    DC *dc = get_physdev_dc( dev );

    if (hfont)
    {
        LOGFONTW lf;
        FMAT2 dcmat;
        BOOL can_use_bitmap = !!(NtGdiGetDeviceCaps( dc->hSelf, TEXTCAPS ) & TC_RA_ABLE);

        NtGdiExtGetObjectW( hfont, sizeof(lf), &lf );
        switch (lf.lfQuality)
        {
        case NONANTIALIASED_QUALITY:
            if (!*aa_flags) *aa_flags = GGO_BITMAP;
            break;
        case ANTIALIASED_QUALITY:
            if (!*aa_flags) *aa_flags = GGO_GRAY4_BITMAP;
            break;
        }

        if (lf.lfOutPrecision == OUT_TT_ONLY_PRECIS)
            can_use_bitmap = FALSE;

        lf.lfWidth = abs(lf.lfWidth);

        TRACE( "%s, h=%d, it=%d, weight=%d, PandF=%02x, charset=%d orient %d escapement %d\n",
               debugstr_w(lf.lfFaceName), (int)lf.lfHeight, lf.lfItalic,
               (int)lf.lfWeight, lf.lfPitchAndFamily, lf.lfCharSet, (int)lf.lfOrientation,
               (int)lf.lfEscapement );

        if (dc->attr->graphics_mode == GM_ADVANCED)
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

        pthread_mutex_lock( &font_lock );

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
        TRACE( "%p %s %d aa %x\n", hfont, debugstr_w(lf.lfFaceName), (int)lf.lfHeight, *aa_flags );
        pthread_mutex_unlock( &font_lock );
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
    NULL,                           /* pEllipse */
    NULL,                           /* pEndDoc */
    NULL,                           /* pEndPage */
    NULL,                           /* pEndPath */
    font_EnumFonts,                 /* pEnumFonts */
    NULL,                           /* pExtEscape */
    NULL,                           /* pExtFloodFill */
    NULL,                           /* pExtTextOut */
    NULL,                           /* pFillPath */
    NULL,                           /* pFillRgn */
    font_FontIsLinked,              /* pFontIsLinked */
    NULL,                           /* pFrameRgn */
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
    NULL,                           /* pInvertRgn */
    NULL,                           /* pLineTo */
    NULL,                           /* pMoveTo */
    NULL,                           /* pPaintRgn */
    NULL,                           /* pPatBlt */
    NULL,                           /* pPie */
    NULL,                           /* pPolyBezier */
    NULL,                           /* pPolyBezierTo */
    NULL,                           /* pPolyDraw */
    NULL,                           /* pPolyPolygon */
    NULL,                           /* pPolyPolyline */
    NULL,                           /* pPolylineTo */
    NULL,                           /* pPutImage */
    NULL,                           /* pRealizeDefaultPalette */
    NULL,                           /* pRealizePalette */
    NULL,                           /* pRectangle */
    NULL,                           /* pResetDC */
    NULL,                           /* pRoundRect */
    NULL,                           /* pSelectBitmap */
    NULL,                           /* pSelectBrush */
    font_SelectFont,                /* pSelectFont */
    NULL,                           /* pSelectPen */
    NULL,                           /* pSetBkColor */
    NULL,                           /* pSetBoundsRect */
    NULL,                           /* pSetDCBrushColor */
    NULL,                           /* pSetDCPenColor */
    NULL,                           /* pSetDIBitsToDevice */
    NULL,                           /* pSetDeviceClipping */
    NULL,                           /* pSetDeviceGammaRamp */
    NULL,                           /* pSetPixel */
    NULL,                           /* pSetTextColor */
    NULL,                           /* pStartDoc */
    NULL,                           /* pStartPage */
    NULL,                           /* pStretchBlt */
    NULL,                           /* pStretchDIBits */
    NULL,                           /* pStrokeAndFillPath */
    NULL,                           /* pStrokePath */
    NULL,                           /* pUnrealizePalette */
    NULL,                           /* pD3DKMTCheckVidPnExclusiveOwnership */
    NULL,                           /* pD3DKMTCloseAdapter */
    NULL,                           /* pD3DKMTOpenAdapterFromLuid */
    NULL,                           /* pD3DKMTQueryVideoMemoryInfo */
    NULL,                           /* pD3DKMTSetVidPnSourceOwner */
    GDI_PRIORITY_FONT_DRV           /* priority */
};

static BOOL get_key_value( HKEY key, const char *name, DWORD *value )
{
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[12 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)value_buffer;
    DWORD count;

    count = query_reg_ascii_value( key, name, info, sizeof(value_buffer) );
    if (count)
    {
        if (info->Type == REG_DWORD) memcpy( value, info->Data, sizeof(*value) );
        else *value = wcstol( (const WCHAR *)info->Data, NULL, 10 );
    }
    return !!count;
}

static UINT init_font_options(void)
{
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[20 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)value_buffer;
    HKEY key;
    DWORD i, val, gamma = 1400;
    UINT dpi = 0;

    if (query_reg_ascii_value( wine_fonts_key, "AntialiasFakeBoldOrItalic",
                               info, sizeof(value_buffer) ) && info->Type == REG_SZ)
    {
        static const WCHAR valsW[] = {'y','Y','t','T','1',0};
        antialias_fakes = (wcschr( valsW, *(const WCHAR *)info->Data ) != NULL);
    }

    if ((key = reg_open_hkcu_key( "Control Panel\\Desktop" )))
    {
        /* FIXME: handle vertical orientations even though Windows doesn't */
        if (get_key_value( key, "FontSmoothingOrientation", &val ))
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
        if (get_key_value( key, "FontSmoothing", &val ) && val /* enabled */)
        {
            if (get_key_value( key, "FontSmoothingType", &val ) && val == 2 /* FE_FONTSMOOTHINGCLEARTYPE */)
                font_smoothing = subpixel_orientation;
            else
                font_smoothing = GGO_GRAY4_BITMAP;
        }
        if (get_key_value( key, "FontSmoothingGamma", &val ) && val)
        {
            gamma = min( max( val, 1000 ), 2200 );
        }
        if (get_key_value( key, "LogPixels", &val )) dpi = val;
        NtClose( key );
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

    if (!dpi && (key = reg_open_key( NULL, fonts_config_keyW, sizeof(fonts_config_keyW) )))
    {
        if (get_key_value( key, "LogPixels", &val )) dpi = val;
        NtClose( key );
    }
    if (!dpi) dpi = 96;

    font_gamma_ramp.gamma = gamma;
    TRACE( "gamma %d screen dpi %u\n", font_gamma_ramp.gamma, dpi );
    return dpi;
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
 *           NtGdiGetTextCharsetInfo    (win32u.@)
 */
UINT WINAPI NtGdiGetTextCharsetInfo( HDC hdc, FONTSIGNATURE *fs, DWORD flags )
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
 *           NtGdiHfontCreate   (win32u.@)
 */
HFONT WINAPI NtGdiHfontCreate( const void *logfont, ULONG size, ULONG type,
                               ULONG flags, void *data )
{
    HFONT hFont;
    FONTOBJ *fontPtr;
    const LOGFONTW *plf;

    if (!logfont) return 0;

    if (size == sizeof(ENUMLOGFONTEXDVW) || size == sizeof(ENUMLOGFONTEXW))
    {
        const ENUMLOGFONTEXW *lfex = logfont;

        if (lfex->elfFullName[0] || lfex->elfStyle[0] || lfex->elfScript[0])
        {
            FIXME( "some fields ignored. fullname=%s, style=%s, script=%s\n",
                   debugstr_w( lfex->elfFullName ), debugstr_w( lfex->elfStyle ),
                   debugstr_w( lfex->elfScript ));
        }

        plf = &lfex->elfLogFont;
    }
    else if (size != sizeof(LOGFONTW))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    else plf = logfont;

    if (!(fontPtr = malloc( sizeof(*fontPtr) ))) return 0;

    fontPtr->logfont = *plf;

    if (!(hFont = alloc_gdi_handle( &fontPtr->obj, NTGDI_OBJ_FONT, &fontobj_funcs )))
    {
        free( fontPtr );
        return 0;
    }

    TRACE("(%d %d %d %d %x %d %x %d %d) %s %s %s %s => %p\n",
          (int)plf->lfHeight, (int)plf->lfWidth,
          (int)plf->lfEscapement, (int)plf->lfOrientation,
          plf->lfPitchAndFamily,
          plf->lfOutPrecision, plf->lfClipPrecision,
          plf->lfQuality, plf->lfCharSet,
          debugstr_w(plf->lfFaceName),
          plf->lfWeight > 400 ? "Bold" : "",
          plf->lfItalic ? "Italic" : "",
          plf->lfUnderline ? "Underline" : "", hFont);

    return hFont;
}

#define ASSOC_CHARSET_OEM    1
#define ASSOC_CHARSET_ANSI   2
#define ASSOC_CHARSET_SYMBOL 4

static DWORD get_associated_charset_info(void)
{
    static int associated_charset = -1;

    if (associated_charset == -1)
    {
        char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[32 * sizeof(WCHAR)])];
        KEY_VALUE_PARTIAL_INFORMATION *info = (void *)value_buffer;
        HKEY hkey;

        static const WCHAR yesW[] = {'y','e','s',0};

        associated_charset = 0;

        if (!(hkey = reg_open_key( NULL, associated_charset_keyW, sizeof(associated_charset_keyW) )))
            return 0;

        if (query_reg_ascii_value( hkey, "ANSI(00)", info, sizeof(value_buffer) ) &&
            info->Type == REG_SZ && !wcsicmp( (const WCHAR *)info->Data, yesW ))
            associated_charset |= ASSOC_CHARSET_ANSI;

        if (query_reg_ascii_value( hkey, "OEM(FF)", info, sizeof(value_buffer) ) &&
            info->Type == REG_SZ && !wcsicmp( (const WCHAR *)info->Data, yesW ))
            associated_charset |= ASSOC_CHARSET_OEM;

        if (query_reg_ascii_value( hkey, "SYMBOL(02)", info, sizeof(value_buffer) ) &&
            info->Type == REG_SZ && !wcsicmp( (const WCHAR *)info->Data, yesW ))
            associated_charset |= ASSOC_CHARSET_SYMBOL;

        NtClose( hkey );

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

        NtGdiExtGetObjectW( font, sizeof(lf), &lf );
        if (!(lf.lfClipPrecision & CLIP_DFA_DISABLE))
            charset = DEFAULT_CHARSET;
    }

    /* Hmm, nicely designed api this one! */
    if (translate_charset_info( ULongToPtr(charset), &csi, TCI_SRCCHARSET) )
        dc->attr->font_code_page = csi.ciACP;
    else {
        switch(charset) {
        case OEM_CHARSET:
            dc->attr->font_code_page = oem_cp.CodePage;
            break;
        case DEFAULT_CHARSET:
            dc->attr->font_code_page = ansi_cp.CodePage;
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
            dc->attr->font_code_page = CP_ACP;
            break;

        default:
            FIXME("Can't find codepage for charset %d\n", charset);
            dc->attr->font_code_page = CP_ACP;
            break;
        }
    }

    TRACE( "charset %d => cp %d\n", charset, dc->attr->font_code_page );
}

/***********************************************************************
 *           NtGdiSelectFont (win32u.@)
 */
HGDIOBJ WINAPI NtGdiSelectFont( HDC hdc, HGDIOBJ handle )
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
 *           FONT_GetObjectW
 */
static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer )
{
    FONTOBJ *font = GDI_GetObjPtr( handle, NTGDI_OBJ_FONT );

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
    free( obj );
    return TRUE;
}


struct font_enum
{
    HDC hdc;
    struct font_enum_entry *buf;
    ULONG size;
    ULONG count;
    ULONG charset;
};

static INT enum_fonts( const LOGFONTW *lf, const TEXTMETRICW *tm, DWORD type, LPARAM lp )
{
    struct font_enum *fe = (struct font_enum *)lp;

    if (fe->charset != DEFAULT_CHARSET && lf->lfCharSet != fe->charset) return 1;
    if ((type & RASTER_FONTTYPE) && !(NtGdiGetDeviceCaps( fe->hdc, TEXTCAPS ) & TC_RA_ABLE))
        return 1;

    if (fe->buf && fe->count < fe->size)
    {
        fe->buf[fe->count].type = type;
        fe->buf[fe->count].lf = *(const ENUMLOGFONTEXW *)lf;
        fe->buf[fe->count].tm = *(const NEWTEXTMETRICEXW *)tm;
    }
    fe->count++;
    return 1;
}

/***********************************************************************
 *           NtGdiEnumFonts    (win32u.@)
 */
BOOL WINAPI NtGdiEnumFonts( HDC hdc, ULONG type, ULONG win32_compat, ULONG face_name_len,
                            const WCHAR *face_name, ULONG charset, ULONG *count, void *buf )
{
    struct font_enum fe;
    PHYSDEV physdev;
    LOGFONTW lf;
    BOOL ret;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    memset( &lf, 0, sizeof(lf) );
    lf.lfCharSet = charset;
    if (face_name_len) memcpy( lf.lfFaceName, face_name, face_name_len * sizeof(WCHAR) );

    fe.hdc     = hdc;
    fe.buf     = buf;
    fe.size    = *count / sizeof(*fe.buf);
    fe.count   = 0;
    fe.charset = charset;

    physdev = GET_DC_PHYSDEV( dc, pEnumFonts );
    ret = physdev->funcs->pEnumFonts( physdev, &lf, enum_fonts, (LPARAM)&fe );
    if (ret && buf) ret = fe.count <= fe.size;
    *count = fe.count * sizeof(*fe.buf);

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiSetTextJustification    (win32u.@)
 */
BOOL WINAPI NtGdiSetTextJustification( HDC hdc, INT extra, INT breaks )
{
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    extra = abs( (extra * dc->attr->vport_ext.cx + dc->attr->wnd_ext.cx / 2) /
                 dc->attr->wnd_ext.cx );
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

    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           NtGdiGetTextFaceW    (win32u.@)
 */
INT WINAPI NtGdiGetTextFaceW( HDC hdc, INT count, WCHAR *name, BOOL alias_name )
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
 *           NtGdiGetTextExtentExW    (win32u.@)
 *
 * Return the size of the string as it would be if it was output properly by
 * e.g. TextOut.
 */
BOOL WINAPI NtGdiGetTextExtentExW( HDC hdc, const WCHAR *str, INT count, INT max_ext,
                                   INT *nfit, INT *dxs, SIZE *size, UINT flags )
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
        if (count > 256 && !(pos = malloc( count * sizeof(*pos) )))
        {
            release_dc_ptr( dc );
            return FALSE;
        }
    }


    if (flags)
        ret = get_char_positions_indices( dc, str, count, pos, size );
    else
        ret = get_char_positions( dc, str, count, pos, size );
    if (ret)
    {
        if (dxs || nfit)
        {
            for (i = 0; i < count; i++)
            {
                unsigned int dx = abs( INTERNAL_XDSTOWS( dc, pos[i] )) +
                    (i + 1) * dc->attr->char_extra;
                if (nfit && dx > (unsigned int)max_ext) break;
		if (dxs) dxs[i] = dx;
            }
            if (nfit) *nfit = i;
        }

        size->cx = abs( INTERNAL_XDSTOWS( dc, size->cx )) + count * dc->attr->char_extra;
        size->cy = abs( INTERNAL_YDSTOWS( dc, size->cy ));
    }

    if (pos != buffer && pos != dxs) free( pos );
    release_dc_ptr( dc );

    TRACE("(%p, %s, %d) returning %dx%d\n", hdc, debugstr_wn(str,count),
          max_ext, (int)size->cx, (int)size->cy );
    return ret;
}

/***********************************************************************
 *           NtGdiGetTextMetricsW    (win32u.@)
 */
BOOL WINAPI NtGdiGetTextMetricsW( HDC hdc, TEXTMETRICW *metrics, ULONG flags )
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

        metrics->tmDigitizedAspectX = NtGdiGetDeviceCaps(hdc, LOGPIXELSX);
        metrics->tmDigitizedAspectY = NtGdiGetDeviceCaps(hdc, LOGPIXELSY);
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
          (int)metrics->tmWeight, metrics->tmFirstChar, (int)metrics->tmAveCharWidth,
          metrics->tmItalic, metrics->tmLastChar, (int)metrics->tmMaxCharWidth,
          metrics->tmUnderlined, metrics->tmDefaultChar, (int)metrics->tmOverhang,
          metrics->tmStruckOut, metrics->tmBreakChar, metrics->tmCharSet,
          metrics->tmPitchAndFamily,
          (int)metrics->tmInternalLeading,
          (int)metrics->tmAscent,
          (int)metrics->tmDescent,
          (int)metrics->tmHeight );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiGetOutlineTextMetricsInternalW    (win32u.@)
 */
UINT WINAPI NtGdiGetOutlineTextMetricsInternalW( HDC hdc, UINT cbData,
                                                 OUTLINETEXTMETRICW *lpOTM, ULONG opts )
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
        output = malloc( ret );
        ret = dev->funcs->pGetOutlineTextMetrics( dev, ret, output );
    }

    if (lpOTM && ret)
    {
        output->otmTextMetrics.tmDigitizedAspectX = NtGdiGetDeviceCaps(hdc, LOGPIXELSX);
        output->otmTextMetrics.tmDigitizedAspectY = NtGdiGetDeviceCaps(hdc, LOGPIXELSY);
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
            free( output );
            ret = cbData;
        }
    }
    release_dc_ptr(dc);
    return ret;
}

/***********************************************************************
 *           NtGdiGetCharWidthW    (win32u.@)
 */
BOOL WINAPI NtGdiGetCharWidthW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                ULONG flags, void *buf )
{
    UINT i, count = last;
    BOOL ret;
    PHYSDEV dev;
    DC *dc;

    if (flags & NTGDI_GETCHARWIDTH_INDICES)
    {
        ABC *abc;
        unsigned int i;

        if (!(abc = malloc( count * sizeof(ABC) )))
            return FALSE;

        if (!NtGdiGetCharABCWidthsW( hdc, first, last, chars,
                                     NTGDI_GETCHARABCWIDTHS_INT | NTGDI_GETCHARABCWIDTHS_INDICES,
                                     abc ))
        {
            free( abc );
            return FALSE;
        }

        for (i = 0; i < count; i++)
            ((INT *)buf)[i] = abc[i].abcA + abc[i].abcB + abc[i].abcC;

        free( abc );
        return TRUE;
    }

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    if (!chars) count = last - first + 1;
    dev = GET_DC_PHYSDEV( dc, pGetCharWidth );
    ret = dev->funcs->pGetCharWidth( dev, first, count, chars, buf );

    if (ret)
    {
        if (flags & NTGDI_GETCHARWIDTH_INT)
        {
            INT *buffer = buf;
            /* convert device units to logical */
            for (i = 0; i < count; i++)
                buffer[i] = width_to_LP( dc, buffer[i] );
        }
        else
        {
            float scale = fabs( dc->xformVport2World.eM11 ) / 16.0f;
            for (i = 0; i < count; i++)
                ((float *)buf)[i] = ((int *)buf)[i] * scale;
        }
    }
    release_dc_ptr( dc );
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
        ret = NtGdiGetGlyphOutline( hdc, index, aa_flags, metrics, 0, NULL, &identity, FALSE );
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

    if (!(image->ptr = malloc( size ))) return ERROR_OUTOFMEMORY;
    image->is_copy = TRUE;
    image->free = free_heap_bits;

    ret = NtGdiGetGlyphOutline( hdc, index, aa_flags, metrics, size, image->ptr,
                                &identity, FALSE );
    if (ret == GDI_ERROR)
    {
        free( image->ptr );
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
    pts = malloc( max_count * sizeof(*pts) );
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
    for (i = 0; i < count; i += 2)
    {
        const ULONG pts_count = 2;
        NtGdiPolyPolyDraw( dc->hSelf, pts + i, &pts_count, 1, NtGdiPolyPolyline );
    }
    free( pts );
}

/***********************************************************************
 *           nulldrv_ExtTextOut
 */
BOOL nulldrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
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
        COLORREF brush_color = NtGdiGetNearestColor( dev->hdc, dc->attr->background_color );
        HBRUSH brush = NtGdiCreateSolidBrush( brush_color, NULL);

        if (brush)
        {
            orig = NtGdiSelectBrush( dev->hdc, brush );
            dp_to_lp( dc, (POINT *)&rc, 2 );
            NtGdiPatBlt( dev->hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
            NtGdiSelectBrush( dev->hdc, orig );
            NtGdiDeleteObjectApp( brush );
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

                bits.ptr = malloc( info->bmiHeader.biSizeImage );
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
                void *ptr = malloc( info->bmiHeader.biSizeImage );
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

    pen = NtGdiCreatePen( PS_SOLID, 1, dc->attr->text_color, NULL );
    orig = NtGdiSelectPen( dev->hdc, pen );

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

    NtGdiSelectPen( dev->hdc, orig );
    NtGdiDeleteObjectApp( pen );
    return TRUE;
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
 *           NtGdiExtTextOutW    (win32u.@)
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
BOOL WINAPI NtGdiExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *lprect,
                              const WCHAR *str, UINT count, const INT *lpDx, DWORD cp )
{
    BOOL ret = FALSE;
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
    DC * dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    INT breakRem;
    static int quietfixme = 0;

    if (!dc) return FALSE;
    if (count > INT_MAX) return FALSE;

    align = dc->attr->text_align;
    breakRem = dc->breakRem;
    layout = dc->attr->layout;

    if (quietfixme == 0 && flags & (ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN))
    {
        FIXME("flags ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN unimplemented\n");
        quietfixme = 1;
    }

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pExtTextOut );

    if (flags & ETO_RTLREADING) align |= TA_RTLREADING;
    if (layout & LAYOUT_RTL)
    {
        if ((align & TA_CENTER) != TA_CENTER) align ^= TA_RIGHT;
        align ^= TA_RTLREADING;
    }

    TRACE("%p, %d, %d, %08x, %s, %s, %d, %p)\n", hdc, x, y, flags,
          wine_dbgstr_rect(lprect), debugstr_wn(str, count), count, lpDx);
    TRACE("align = %x bkmode = %x mapmode = %x\n", align, dc->attr->background_mode,
          dc->attr->map_mode);

    if(align & TA_UPDATECP)
    {
        pt = dc->attr->cur_pos;
        x = pt.x;
        y = pt.y;
    }

    NtGdiGetTextMetricsW( hdc, &tm, 0 );
    NtGdiExtGetObjectW( dc->hFont, sizeof(lf), &lf );

    if(!(tm.tmPitchAndFamily & TMPF_VECTOR)) /* Non-scalable fonts shouldn't be rotated */
        lf.lfEscapement = 0;

    if ((dc->attr->graphics_mode == GM_COMPATIBLE) &&
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

    char_extra = dc->attr->char_extra;
    if (char_extra && lpDx && NtGdiGetDeviceCaps( hdc, TECHNOLOGY ) == DT_RASPRINTER)
        char_extra = 0; /* Printer drivers don't add char_extra if lpDx is supplied */

    if(char_extra || dc->breakExtra || breakRem || lpDx || lf.lfEscapement != 0)
    {
        UINT i;
        POINT total = {0, 0}, desired[2];

        deltas = malloc( count * sizeof(*deltas) );
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
            INT *dx = malloc( count * sizeof(*dx) );

            NtGdiGetTextExtentExW( hdc, str, count, -1, NULL, dx, &sz, !!(flags & ETO_GLYPH_INDEX) );

            deltas[0].x = dx[0];
            deltas[0].y = 0;
            for (i = 1; i < count; i++)
            {
                deltas[i].x = dx[i] - dx[i - 1];
                deltas[i].y = 0;
            }
            free( dx );
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

            if (dc->attr->graphics_mode == GM_COMPATIBLE)
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

        NtGdiGetTextExtentExW( hdc, str, count, 0, NULL, NULL, &sz, !!(flags & ETO_GLYPH_INDEX) );
        desired[0].x = desired[0].y = 0;
        desired[1].x = sz.cx;
        desired[1].y = 0;
        lp_to_dp(dc, desired, 2);
        desired[1].x -= desired[0].x;
        desired[1].y -= desired[0].y;

        if (dc->attr->graphics_mode == GM_COMPATIBLE)
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
            NtGdiMoveTo( hdc, pt.x, pt.y, NULL );
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
            NtGdiMoveTo( hdc, pt.x, pt.y, NULL );
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

    if (dc->attr->background_mode != TRANSPARENT)
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
                if (!IsRectEmpty( &text_box ))
                    physdev->funcs->pExtTextOut( physdev, 0, 0, ETO_OPAQUE, &text_box, NULL, 0, NULL );
            }
        }
    }

    ret = physdev->funcs->pExtTextOut( physdev, x, y, (flags & ~ETO_OPAQUE), &rc,
                                       str, count, (INT*)deltas );

done:
    free( deltas );

    if (ret && (lf.lfUnderline || lf.lfStrikeOut))
    {
        int underlinePos, strikeoutPos;
        int underlineWidth, strikeoutWidth;
        UINT size = NtGdiGetOutlineTextMetricsInternalW( hdc, 0, NULL, 0 );
        OUTLINETEXTMETRICW* otm = NULL;
        POINT pts[5];
        HPEN hpen = NtGdiSelectPen( hdc, GetStockObject(NULL_PEN) );
        HBRUSH hbrush = NtGdiCreateSolidBrush( dc->attr->text_color, NULL );

        hbrush = NtGdiSelectBrush(hdc, hbrush);

        if(!size)
        {
            underlinePos = 0;
            underlineWidth = tm.tmAscent / 20 + 1;
            strikeoutPos = tm.tmAscent / 2;
            strikeoutWidth = underlineWidth;
        }
        else
        {
            otm = malloc( size );
            NtGdiGetOutlineTextMetricsInternalW( hdc, size, otm, 0 );
            underlinePos = abs( INTERNAL_YWSTODS( dc, otm->otmsUnderscorePosition ));
            if (otm->otmsUnderscorePosition < 0) underlinePos = -underlinePos;
            underlineWidth = get_line_width( dc, otm->otmsUnderscoreSize );
            strikeoutPos = abs( INTERNAL_YWSTODS( dc, otm->otmsStrikeoutPosition ));
            if (otm->otmsStrikeoutPosition < 0) strikeoutPos = -strikeoutPos;
            strikeoutWidth = get_line_width( dc, otm->otmsStrikeoutSize );
            free( otm );
        }


        if (lf.lfUnderline)
        {
            const ULONG cnt = 5;
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
            NtGdiPolyPolyDraw( hdc, pts, &cnt, 1, NtGdiPolyPolygon );
        }

        if (lf.lfStrikeOut)
        {
            const ULONG cnt = 5;
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
            NtGdiPolyPolyDraw( hdc, pts, &cnt, 1, NtGdiPolyPolygon );
        }

        NtGdiSelectPen(hdc, hpen);
        hbrush = NtGdiSelectBrush(hdc, hbrush);
        NtGdiDeleteObjectApp( hbrush );
    }

    release_dc_ptr( dc );

    return ret;
}


/******************************************************************************
 *           NtGdiGetCharABCWidthsW    (win32u.@)
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
 */
BOOL WINAPI NtGdiGetCharABCWidthsW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                    ULONG flags, void *buffer )
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    unsigned int i, count = last;
    BOOL ret;

    if (!dc) return FALSE;

    if (!buffer)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    if (flags & NTGDI_GETCHARABCWIDTHS_INDICES)
    {
        dev = GET_DC_PHYSDEV( dc, pGetCharABCWidthsI );
        ret = dev->funcs->pGetCharABCWidthsI( dev, first, count, chars, buffer );
    }
    else
    {
        if (!chars) count = last - first + 1;
        dev = GET_DC_PHYSDEV( dc, pGetCharABCWidths );
        ret = dev->funcs->pGetCharABCWidths( dev, first, count, chars, buffer );
    }

    if (ret)
    {
        ABC *abc = buffer;
        if (flags & NTGDI_GETCHARABCWIDTHS_INT)
        {
            /* convert device units to logical */
            for (i = 0; i < count; i++)
            {
                abc[i].abcA = width_to_LP( dc, abc[i].abcA );
                abc[i].abcB = width_to_LP( dc, abc[i].abcB );
                abc[i].abcC = width_to_LP( dc, abc[i].abcC );
            }
        }
        else
        {
            /* convert device units to logical */
            FLOAT scale = fabs( dc->xformVport2World.eM11 );
            ABCFLOAT *abcf = buffer;

            for (i = 0; i < count; i++)
            {
                abcf[i].abcfA = abc[i].abcA * scale;
                abcf[i].abcfB = abc[i].abcB * scale;
                abcf[i].abcfC = abc[i].abcC * scale;
            }
        }
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiGetGlyphOutline    (win32u.@)
 */
DWORD WINAPI NtGdiGetGlyphOutline( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics,
                                   DWORD size, void *buffer, const MAT2 *mat2,
                                   BOOL ignore_rotation )
{
    DC *dc;
    DWORD ret;
    PHYSDEV dev;

    TRACE( "(%p, %04x, %04x, %p, %d, %p, %p)\n", hdc, ch, format, metrics, (int)size, buffer, mat2 );

    if (!mat2) return GDI_ERROR;

    dc = get_dc_ptr(hdc);
    if(!dc) return GDI_ERROR;

    dev = GET_DC_PHYSDEV( dc, pGetGlyphOutline );
    ret = dev->funcs->pGetGlyphOutline( dev, ch & 0xffff, format, metrics, size, buffer, mat2 );
    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *           __wine_get_file_outline_text_metric    (win32u.@)
 */
BOOL WINAPI __wine_get_file_outline_text_metric( const WCHAR *path, TEXTMETRICW *otm,
                                                 UINT *em_square, WCHAR *face_name )
{
    struct gdi_font *font = NULL;

    if (!path || !font_funcs) return FALSE;

    if (!(font = alloc_gdi_font( path, NULL, 0 ))) goto done;
    font->lf.lfHeight = 100;
    if (!font_funcs->load_font( font )) goto done;
    if (!font_funcs->set_outline_text_metrics( font )) goto done;
    *otm = font->otm.otmTextMetrics;
    *em_square = font->otm.otmEMSquare;
    wcscpy( face_name, (const WCHAR *)font->otm.otmpFamilyName );
    free_gdi_font( font );
    return TRUE;

done:
    if (font) free_gdi_font( font );
    RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
    return FALSE;
}

/*************************************************************************
 *             NtGdiGetKerningPairs   (win32u.@)
 */
DWORD WINAPI NtGdiGetKerningPairs( HDC hdc, DWORD count, KERNINGPAIR *kern_pair )
{
    DC *dc;
    DWORD ret;
    PHYSDEV dev;

    TRACE( "(%p,%d,%p)\n", hdc, (int)count, kern_pair );

    if (!count && kern_pair)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    dc = get_dc_ptr( hdc );
    if (!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetKerningPairs );
    ret = dev->funcs->pGetKerningPairs( dev, count, kern_pair );
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 *           NtGdiGetFontData    (win32u.@)
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
 * Calls RtlSetLastWin32Error()
 *
 */
DWORD WINAPI NtGdiGetFontData( HDC hdc, DWORD table, DWORD offset, void *buffer, DWORD length )
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
 *           NtGdiGetGlyphIndicesW    (win32u.@)
 */
DWORD WINAPI NtGdiGetGlyphIndicesW( HDC hdc, const WCHAR *str, INT count,
                                    WORD *indices, DWORD flags )
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    DWORD ret;

    TRACE( "(%p, %s, %d, %p, 0x%x)\n", hdc, debugstr_wn(str, count), count, indices, (int)flags );

    if(!dc) return GDI_ERROR;

    dev = GET_DC_PHYSDEV( dc, pGetGlyphIndices );
    ret = dev->funcs->pGetGlyphIndices( dev, str, count, indices, flags );
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *								       *
 *           Font Resource API					       *
 *								       *
 ***********************************************************************/


static int add_system_font_resource( const WCHAR *file, DWORD flags )
{
    WCHAR path[MAX_PATH];
    int ret;

    /* try in %WINDIR%/fonts, needed for Fotobuch Designer */
    get_fonts_win_dir_path( file, path );
    pthread_mutex_lock( &font_lock );
    ret = font_funcs->add_font( path, flags );
    pthread_mutex_unlock( &font_lock );
    /* try in datadir/fonts (or builddir/fonts), needed for Magic the Gathering Online */
    if (!ret)
    {
        get_fonts_data_dir_path( file, path );
        pthread_mutex_lock( &font_lock );
        ret = font_funcs->add_font( path, flags );
        pthread_mutex_unlock( &font_lock );
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
    int ret = 0;

    if (*file == '\\')
    {
        DWORD addfont_flags = ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE;

        if (!(flags & FR_PRIVATE)) addfont_flags |= ADDFONT_ADD_TO_CACHE;
        pthread_mutex_lock( &font_lock );
        ret = font_funcs->add_font( file, addfont_flags );
        pthread_mutex_unlock( &font_lock );
    }
    else if (!wcschr( file, '\\' ))
        ret = add_system_font_resource( file, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );

    return ret;
}

static BOOL remove_font_resource( LPCWSTR file, DWORD flags )
{
    BOOL ret = FALSE;

    if (*file == '\\')
    {
        DWORD addfont_flags = ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE;

        if (!(flags & FR_PRIVATE)) addfont_flags |= ADDFONT_ADD_TO_CACHE;
        ret = remove_font( file, addfont_flags );
    }
    else if (!wcschr( file, '\\' ))
        ret = remove_system_font_resource( file, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );

    return ret;
}

static void load_system_bitmap_fonts(void)
{
    static const char * const fonts[] = { "FONTS.FON", "OEMFONT.FON", "FIXEDFON.FON" };
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[MAX_PATH * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)value_buffer;
    HKEY hkey;
    DWORD i;

    if (!(hkey = reg_open_key( NULL, fonts_config_keyW, sizeof(fonts_config_keyW) ))) return;
    for (i = 0; i < ARRAY_SIZE(fonts); i++)
    {
        if (query_reg_ascii_value( hkey, fonts[i], info, sizeof(value_buffer) ) && info->Type == REG_SZ)
            add_system_font_resource( (const WCHAR *)info->Data, ADDFONT_ALLOW_BITMAP );
    }
    NtClose( hkey );
}

static void load_directory_fonts( WCHAR *path, UINT flags )
{
    IO_STATUS_BLOCK io = {{0}};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nt_name;
    HANDLE handle;
    char buf[8192];
    size_t len;

    len = lstrlenW( path );
    while (len && path[len - 1] == '\\') len--;

    nt_name.Buffer = path;
    nt_name.MaximumLength = nt_name.Length = len * sizeof(WCHAR);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (NtOpenFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr, &io,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT ))
        return;

    path[len++] = '\\';

    while (!NtQueryDirectoryFile( handle, 0, NULL, NULL, &io, buf, sizeof(buf),
                                  FileBothDirectoryInformation, FALSE, NULL, FALSE ) &&
           io.Information)
    {
        FILE_BOTH_DIR_INFORMATION *info = (FILE_BOTH_DIR_INFORMATION *)buf;
        for (;;)
        {
            if (!(info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                memcpy( path + len, info->FileName, info->FileNameLength );
                path[len + info->FileNameLength / sizeof(WCHAR)] = 0;
                font_funcs->add_font( path, flags );
            }
            if (!info->NextEntryOffset) break;
            info = (FILE_BOTH_DIR_INFORMATION *)((char *)info + info->NextEntryOffset);
        }
    }

    NtClose( handle );
}

static void load_file_system_fonts(void)
{
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[1024 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)value_buffer;
    WCHAR *ptr, *next, path[MAX_PATH];

    /* Windows directory */
    get_fonts_win_dir_path( NULL, path );
    load_directory_fonts( path, 0 );

    /* Wine data directory */
    get_fonts_data_dir_path( NULL, path );
    load_directory_fonts( path, ADDFONT_EXTERNAL_FONT );

    /* custom paths */
    /* @@ Wine registry key: HKCU\Software\Wine\Fonts */
    if (query_reg_ascii_value( wine_fonts_key, "Path", info, sizeof(value_buffer) ) &&
        info->Type == REG_SZ)
    {
        for (ptr = (WCHAR *)info->Data; ptr; ptr = next)
        {
            if ((next = wcschr( ptr, ';' ))) *next++ = 0;
            if (next && next - ptr < 2) continue;
            lstrcpynW( path, ptr, MAX_PATH );
            if (path[1] == ':')
            {
                memmove( path + ARRAYSIZE(nt_prefixW), path, (lstrlenW( path ) + 1) * sizeof(WCHAR) );
                memcpy( path, nt_prefixW, sizeof(nt_prefixW) );
            }
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
    DWORD len, i = 0;
    WCHAR value[LF_FULLFACESIZE + 12], *tmp, *path;
    char buffer[2048];
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    WCHAR *file;
    HKEY hkey;

    static const WCHAR external_fontsW[] = {'E','x','t','e','r','n','a','l',' ','F','o','n','t','s'};

    winnt_key = reg_create_key( NULL, fonts_winnt_config_keyW, sizeof(fonts_winnt_config_keyW), 0, NULL );
    win9x_key = reg_create_key( NULL, fonts_win9x_config_keyW, sizeof(fonts_win9x_config_keyW), 0, NULL );

    /* enumerate the fonts and add external ones to the two keys */

    if (!(hkey = reg_create_key( wine_fonts_key, external_fontsW, sizeof(external_fontsW), 0, NULL )))
        return;

    while (reg_enum_value( hkey, i++, info, sizeof(buffer) - sizeof(nt_prefixW),
                           value, LF_FULLFACESIZE * sizeof(WCHAR) ))
    {
        if (info->Type != REG_SZ) continue;

        path = (WCHAR *)(buffer + info->DataOffset);
        if (path[0] && path[1] == ':')
        {
            memmove( path + ARRAYSIZE(nt_prefixW), path, info->DataLength );
            memcpy( path, nt_prefixW, sizeof(nt_prefixW) );
        }

        if ((tmp = wcsrchr( value, ' ' )) && !facename_compare( tmp, true_type_suffixW, -1 )) *tmp = 0;
        if ((face = find_face_from_full_name( value )) && !wcsicmp( face->file, path ))
        {
            face->flags |= ADDFONT_EXTERNAL_FOUND;
            continue;
        }
        if (tmp && !*tmp) *tmp = ' ';
        if (!(key = malloc( sizeof(*key) ))) break;
        lstrcpyW( key->value, value );
        list_add_tail( &external_keys, &key->entry );
    }

    WINE_RB_FOR_EACH_ENTRY( family, &family_name_tree, struct gdi_font_family, name_entry )
    {
        LIST_FOR_EACH_ENTRY( face, &family->faces, struct gdi_font_face, entry )
        {
            if (!(face->flags & ADDFONT_EXTERNAL_FONT)) continue;
            if ((face->flags & ADDFONT_EXTERNAL_FOUND)) continue;

            lstrcpyW( value, face->full_name );
            if (face->scalable) lstrcatW( value, true_type_suffixW );

            if (face->file[0] == '\\')
            {
                file = face->file;
                if (file[5] == ':') file += ARRAYSIZE(nt_prefixW);
            }
            else if ((file = wcsrchr( face->file, '\\' )))
                file++;
            else
                file = face->file;

            len = (lstrlenW(file) + 1) * sizeof(WCHAR);
            set_reg_value( winnt_key, value, REG_SZ, file, len );
            set_reg_value( win9x_key, value, REG_SZ, file, len );
            set_reg_value( hkey, value, REG_SZ, file, len );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( key, next, &external_keys, struct external_key, entry )
    {
        reg_delete_value( win9x_key, key->value );
        reg_delete_value( winnt_key, key->value );
        reg_delete_value( hkey, key->value );
        list_remove( &key->entry );
        free( key );
    }
    NtClose( win9x_key );
    NtClose( winnt_key );
    NtClose( hkey );
}

static void load_registry_fonts(void)
{
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[MAX_PATH * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)value_buffer;
    KEY_VALUE_FULL_INFORMATION *enum_info = (KEY_VALUE_FULL_INFORMATION *)value_buffer;
    WCHAR value[LF_FULLFACESIZE + 12], *tmp, *path;
    DWORD i = 0, dlen;
    HKEY hkey;

    static const WCHAR dot_fonW[] = {'.','f','o','n',0};

    /* Look under HKLM\Software\Microsoft\Windows[ NT]\CurrentVersion\Fonts
       for any fonts not installed in %WINDOWSDIR%\Fonts.  They will have their
       full path as the entry.  Also look for any .fon fonts, since ReadFontDir
       will skip these. */
    if (is_win9x())
        hkey = reg_open_key( NULL, fonts_win9x_config_keyW, sizeof(fonts_win9x_config_keyW) );
    else
        hkey = reg_open_key( NULL, fonts_winnt_config_keyW, sizeof(fonts_winnt_config_keyW) );
    if (!hkey) return;

    while (reg_enum_value( hkey, i++, enum_info, sizeof(value_buffer), value, sizeof(value) ))
    {
        if (enum_info->Type != REG_SZ) continue;
        if ((tmp = wcsrchr( value, ' ' )) && !facename_compare( tmp, true_type_suffixW, -1 )) *tmp = 0;
        if (find_face_from_full_name( value )) continue;
        if (tmp && !*tmp) *tmp = ' ';

        if (!(dlen = query_reg_value( hkey, value, info, sizeof(value_buffer) - sizeof(nt_prefixW) )) ||
            info->Type != REG_SZ)
        {
            WARN( "Unable to get face path %s\n", debugstr_w(value) );
            continue;
        }

        path = (WCHAR *)info->Data;
        if (path[0] && path[1] == ':')
        {
            memmove( path + ARRAYSIZE(nt_prefixW), path, dlen );
            memcpy( path, nt_prefixW, sizeof(nt_prefixW) );
            dlen += sizeof(nt_prefixW);
        }

        dlen /= sizeof(WCHAR);
        if (*path == '\\')
            add_font_resource( path, ADDFONT_ALLOW_BITMAP );
        else if (dlen >= 6 && !wcsicmp( path + dlen - 5, dot_fonW ))
            add_system_font_resource( path, ADDFONT_ALLOW_BITMAP );
    }
    NtClose( hkey );
}

static HKEY open_hkcu(void)
{
    char buffer[256];
    WCHAR bufferW[256];
    DWORD_PTR sid_data[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(DWORD_PTR)];
    DWORD i, len = sizeof(sid_data);
    SID *sid;

    if (NtQueryInformationToken( GetCurrentThreadEffectiveToken(), TokenUser, sid_data, len, &len ))
        return 0;

    sid = ((TOKEN_USER *)sid_data)->User.Sid;
    len = sprintf( buffer, "\\Registry\\User\\S-%u-%u", (int)sid->Revision,
            (int)MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5], sid->IdentifierAuthority.Value[4] ),
                           MAKEWORD( sid->IdentifierAuthority.Value[3], sid->IdentifierAuthority.Value[2] )));
    for (i = 0; i < sid->SubAuthorityCount; i++)
        len += sprintf( buffer + len, "-%u", (int)sid->SubAuthority[i] );
    ascii_to_unicode( bufferW, buffer, len + 1 );

    return reg_open_key( NULL, bufferW, len * sizeof(WCHAR) );
}

/***********************************************************************
 *              font_init
 */
UINT font_init(void)
{
    OBJECT_ATTRIBUTES attr = { sizeof(attr) };
    UNICODE_STRING name;
    HANDLE mutex;
    DWORD disposition;
    UINT dpi = 0;

    static WCHAR wine_font_mutexW[] =
        {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',
         '\\','_','_','W','I','N','E','_','F','O','N','T','_','M','U','T','E','X','_','_'};
    static const WCHAR wine_fonts_keyW[] =
        {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','F','o','n','t','s'};
    static const WCHAR cacheW[] = {'C','a','c','h','e'};

    if (!(hkcu_key = open_hkcu())) return 0;
    wine_fonts_key = reg_create_key( hkcu_key, wine_fonts_keyW, sizeof(wine_fonts_keyW), 0, NULL );
    if (wine_fonts_key) dpi = init_font_options();
    if (!dpi) return 96;
    update_codepage( dpi );

    if (!(font_funcs = init_freetype_lib()))
        return dpi;

    load_system_bitmap_fonts();
    load_file_system_fonts();
    font_funcs->load_fonts();

    attr.Attributes = OBJ_OPENIF;
    attr.ObjectName = &name;
    name.Buffer = wine_font_mutexW;
    name.Length = name.MaximumLength = sizeof(wine_font_mutexW);

    if (NtCreateMutant( &mutex, MUTEX_ALL_ACCESS, &attr, FALSE ) < 0) return dpi;
    NtWaitForSingleObject( mutex, FALSE, NULL );

    wine_fonts_cache_key = reg_create_key( wine_fonts_key, cacheW, sizeof(cacheW),
                                           REG_OPTION_VOLATILE, &disposition );

    if (disposition == REG_CREATED_NEW_KEY)
    {
        load_registry_fonts();
        update_external_font_keys();
    }

    NtReleaseMutant( mutex, NULL );

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
    return dpi;
}

/***********************************************************************
 *           NtGdiAddFontResourceW    (win32u.@)
 */
INT WINAPI NtGdiAddFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                  DWORD tid, void *dv )
{
    if (!font_funcs) return 1;
    return add_font_resource( str, flags );
}

/***********************************************************************
 *           NtGdiAddFontMemResourceEx    (win32u.@)
 */
HANDLE WINAPI NtGdiAddFontMemResourceEx( void *ptr, DWORD size, void *dv, ULONG dv_size,
                                         DWORD *count )
{
    HANDLE ret;
    DWORD num_fonts;
    void *copy;

    if (!ptr || !size || !count)
    {
        RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (!font_funcs) return NULL;
    if (!(copy = malloc( size ))) return NULL;
    memcpy( copy, ptr, size );

    pthread_mutex_lock( &font_lock );
    num_fonts = font_funcs->add_mem_font( copy, size, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );
    pthread_mutex_unlock( &font_lock );

    if (!num_fonts)
    {
        free( copy );
        return NULL;
    }

    /* FIXME: is the handle only for use in RemoveFontMemResourceEx or should it be a true handle?
     * For now return something unique but quite random
     */
    ret = (HANDLE)((INT_PTR)copy ^ 0x87654321);

    __TRY
    {
        *count = num_fonts;
    }
    __EXCEPT
    {
        WARN( "page fault while writing to *count (%p)\n", count );
        NtGdiRemoveFontMemResourceEx( ret );
        ret = 0;
    }
    __ENDTRY
    TRACE( "Returning handle %p\n", ret );
    return ret;
}

/***********************************************************************
 *           NtGdiRemoveFontMemResourceEx    (win32u.@)
 */
BOOL WINAPI NtGdiRemoveFontMemResourceEx( HANDLE handle )
{
    FIXME( "(%p) stub\n", handle );
    return TRUE;
}

/***********************************************************************
 *           NtGdiRemoveFontResourceW    (win32u.@)
 */
BOOL WINAPI NtGdiRemoveFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                      DWORD tid, void *dv )
{
    if (!font_funcs) return TRUE;
    return remove_font_resource( str, flags );
}

/***********************************************************************
 *           NtGdiGetFontUnicodeRanges    (win32u.@)
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
DWORD WINAPI NtGdiGetFontUnicodeRanges( HDC hdc, GLYPHSET *lpgs )
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
 *           NtGdiFontIsLinked    (win32u.@)
 */
BOOL WINAPI NtGdiFontIsLinked( HDC hdc )
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
 *           NtGdiGetRealizationInfo    (win32u.@)
 */
BOOL WINAPI NtGdiGetRealizationInfo( HDC hdc, struct font_realization_info *info )
{
    BOOL is_v0 = info->size == FIELD_OFFSET(struct font_realization_info, file_count);
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
 *           NtGdiGetRasterizerCaps   (win32u.@)
 */
BOOL WINAPI NtGdiGetRasterizerCaps( RASTERIZER_STATUS *status, UINT size )
{
    status->nSize = sizeof(RASTERIZER_STATUS);
    status->wFlags = font_funcs ? (TT_AVAILABLE | TT_ENABLED) : 0;
    status->nLanguageID = 0;
    return TRUE;
}

/*************************************************************************
 *             NtGdiGetFontFileData   (win32u.@)
 */
BOOL WINAPI NtGdiGetFontFileData( DWORD instance_id, DWORD file_index, UINT64 *offset,
                                  void *buff, SIZE_T buff_size )
{
    struct gdi_font *font;
    DWORD tag = 0, size;
    BOOL ret = FALSE;

    if (!font_funcs) return FALSE;
    pthread_mutex_lock( &font_lock );
    if ((font = get_font_from_handle( instance_id )))
    {
        if (font->ttc_item_offset) tag = MS_TTCF_TAG;
        size = font_funcs->get_font_data( font, tag, 0, NULL, 0 );
        if (size != GDI_ERROR && size >= buff_size && *offset <= size - buff_size)
            ret = font_funcs->get_font_data( font, tag, *offset, buff, buff_size ) != GDI_ERROR;
        else
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
    }
    pthread_mutex_unlock( &font_lock );
    return ret;
}

/*************************************************************************
 *             NtGdiGetFontFileInfo   (win32u.@)
 */
BOOL WINAPI NtGdiGetFontFileInfo( DWORD instance_id, DWORD file_index, struct font_fileinfo *info,
                                  SIZE_T size, SIZE_T *needed )
{
    SIZE_T required_size = 0;
    struct gdi_font *font;
    BOOL ret = FALSE;

    pthread_mutex_lock( &font_lock );

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
        else RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
    }

    pthread_mutex_unlock( &font_lock );
    if (needed) *needed = required_size;
    return ret;
}

/*************************************************************
 *           NtGdiGetCharWidthInfo    (win32u.@)
 */
BOOL WINAPI NtGdiGetCharWidthInfo( HDC hdc, struct char_width_info *info )
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

/***********************************************************************
 *           DrawTextW    (win32u.so)
 */
INT WINAPI DrawTextW( HDC hdc, const WCHAR *str, INT count, RECT *rect, UINT flags )
{
    struct draw_text_params *params;
    struct draw_text_result *result;
    ULONG ret_len, size;
    NTSTATUS status;
    int ret = 0;

    if (count == -1) count = wcslen( str );
    size = FIELD_OFFSET( struct draw_text_params, str[count] );
    if (!(params = malloc( size ))) return 0;
    params->hdc = hdc;
    params->rect = *rect;
    params->flags = flags;
    if (count) memcpy( params->str, str, count * sizeof(WCHAR) );

    status = KeUserModeCallback( NtUserDrawText, params, size, (void **)&result, &ret_len );
    if (!status && ret_len == sizeof(*result))
    {
        ret = result->height;
        *rect = result->rect;
    }
    free( params );
    return ret;
}
