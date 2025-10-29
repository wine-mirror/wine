/*
 * FreeType font engine interface
 *
 * Copyright 2001 Huw D M Davies for CodeWeavers.
 * Copyright 2006 Dmitry Timoshkov for CodeWeavers.
 *
 * This file contains the WineEng* functions.
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#ifdef __APPLE__
#include <CoreText/CoreText.h>
#endif /* __APPLE__ */

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TYPES_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_OUTLINE_H
#include FT_TRIGONOMETRY_H
#include FT_MODULE_H
#include FT_WINFONTS_H
#include FT_LCD_FILTER_H
#endif /* HAVE_FT2BUILD_H */

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "wine/debug.h"
#include "wine/list.h"

#ifdef SONAME_LIBFREETYPE

WINE_DEFAULT_DEBUG_CHANNEL(font);

static FT_Library library = 0;
typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;
static FT_Version_t FT_Version;
static DWORD FT_SimpleVersion;
#define FT_VERSION_VALUE(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))

static void *ft_handle = NULL;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(FT_Done_Face);
MAKE_FUNCPTR(FT_Get_Char_Index);
MAKE_FUNCPTR(FT_Get_First_Char);
MAKE_FUNCPTR(FT_Get_Next_Char);
MAKE_FUNCPTR(FT_Get_Sfnt_Name);
MAKE_FUNCPTR(FT_Get_Sfnt_Name_Count);
MAKE_FUNCPTR(FT_Get_Sfnt_Table);
MAKE_FUNCPTR(FT_Get_TrueType_Engine_Type);
MAKE_FUNCPTR(FT_Get_WinFNT_Header);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Library_SetLcdFilter);
MAKE_FUNCPTR(FT_Library_Version);
MAKE_FUNCPTR(FT_Load_Glyph);
MAKE_FUNCPTR(FT_Load_Sfnt_Table);
MAKE_FUNCPTR(FT_Matrix_Multiply);
MAKE_FUNCPTR(FT_MulDiv);
#ifdef FT_MULFIX_INLINED
#define pFT_MulFix FT_MULFIX_INLINED
#else
MAKE_FUNCPTR(FT_MulFix);
#endif
MAKE_FUNCPTR(FT_New_Face);
MAKE_FUNCPTR(FT_New_Memory_Face);
MAKE_FUNCPTR(FT_Outline_Embolden);
MAKE_FUNCPTR(FT_Outline_Get_Bitmap);
MAKE_FUNCPTR(FT_Outline_Get_CBox);
MAKE_FUNCPTR(FT_Outline_Transform);
MAKE_FUNCPTR(FT_Outline_Translate);
MAKE_FUNCPTR(FT_Property_Set);
MAKE_FUNCPTR(FT_Render_Glyph);
MAKE_FUNCPTR(FT_Set_Charmap);
MAKE_FUNCPTR(FT_Set_Pixel_Sizes);
MAKE_FUNCPTR(FT_Vector_Length);
MAKE_FUNCPTR(FT_Vector_Transform);
MAKE_FUNCPTR(FT_Vector_Unit);

#ifdef SONAME_LIBFONTCONFIG
#include <fontconfig/fontconfig.h>
MAKE_FUNCPTR(FcConfigSubstitute);
MAKE_FUNCPTR(FcDefaultSubstitute);
MAKE_FUNCPTR(FcFontList);
MAKE_FUNCPTR(FcFontMatch);
MAKE_FUNCPTR(FcFontSetDestroy);
MAKE_FUNCPTR(FcInit);
MAKE_FUNCPTR(FcPatternAddString);
MAKE_FUNCPTR(FcPatternCreate);
MAKE_FUNCPTR(FcPatternDestroy);
MAKE_FUNCPTR(FcPatternGetBool);
MAKE_FUNCPTR(FcPatternGetInteger);
MAKE_FUNCPTR(FcPatternGetString);
MAKE_FUNCPTR(FcConfigGetFontDirs);
MAKE_FUNCPTR(FcConfigGetCurrent);
MAKE_FUNCPTR(FcCacheCopySet);
MAKE_FUNCPTR(FcCacheNumSubdir);
MAKE_FUNCPTR(FcCacheSubdir);
MAKE_FUNCPTR(FcDirCacheRead);
MAKE_FUNCPTR(FcDirCacheUnload);
MAKE_FUNCPTR(FcStrListCreate);
MAKE_FUNCPTR(FcStrListDone);
MAKE_FUNCPTR(FcStrListNext);
MAKE_FUNCPTR(FcStrSetAdd);
MAKE_FUNCPTR(FcStrSetCreate);
MAKE_FUNCPTR(FcStrSetDestroy);
MAKE_FUNCPTR(FcStrSetMember);
#ifndef FC_NAMELANG
#define FC_NAMELANG "namelang"
#endif
#ifndef FC_PRGNAME
#define FC_PRGNAME "prgname"
#endif
#endif /* SONAME_LIBFONTCONFIG */

#undef MAKE_FUNCPTR

#ifndef FT_MAKE_TAG
#define FT_MAKE_TAG( ch0, ch1, ch2, ch3 ) \
	( ((DWORD)(BYTE)(ch0) << 24) | ((DWORD)(BYTE)(ch1) << 16) | \
	  ((DWORD)(BYTE)(ch2) << 8) | (DWORD)(BYTE)(ch3) )
#endif

#ifndef ft_encoding_none
#define FT_ENCODING_NONE ft_encoding_none
#endif
#ifndef ft_encoding_ms_symbol
#define FT_ENCODING_MS_SYMBOL ft_encoding_symbol
#endif
#ifndef ft_encoding_unicode
#define FT_ENCODING_UNICODE ft_encoding_unicode
#endif
#ifndef ft_encoding_apple_roman
#define FT_ENCODING_APPLE_ROMAN ft_encoding_apple_roman
#endif

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

/* 'gasp' flags */
#define GASP_GRIDFIT 0x01
#define GASP_DOGRAY  0x02

/* FT_Bitmap_Size gained 3 new elements between FreeType 2.1.4 and 2.1.5
   So to let this compile on older versions of FreeType we'll define the
   new structure here. */
typedef struct {
    FT_Short height, width;
    FT_Pos size, x_ppem, y_ppem;
} My_FT_Bitmap_Size;

struct font_private_data
{
    FT_Face ft_face;
    struct font_mapping *mapping;
};

static inline FT_Face get_ft_face( struct gdi_font *font )
{
    return ((struct font_private_data *)font->private)->ft_face;
}

struct font_mapping
{
    struct list entry;
    int         refcount;
    dev_t       dev;
    ino_t       ino;
    void       *data;
    size_t      size;
};

static struct list mappings_list = LIST_INIT( mappings_list );

static UINT default_aa_flags;
static LCID system_lcid;

static BOOL freetype_set_outline_text_metrics( struct gdi_font *font );
static BOOL freetype_set_bitmap_text_metrics( struct gdi_font *font );

/****************************************
 *   Notes on .fon files
 *
 * The fonts System, FixedSys and Terminal are special.  There are typically multiple
 * versions installed for different resolutions and codepages.  Windows stores which one to use
 * in HKEY_CURRENT_CONFIG\\Software\\Fonts.
 *    Key            Meaning
 *  FIXEDFON.FON    FixedSys
 *  FONTS.FON       System
 *  OEMFONT.FON     Terminal
 *  LogPixels       Current dpi set by the display control panel applet
 *                  (HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\FontDPI
 *                  also has a LogPixels value that appears to mirror this)
 *
 * On my system these values have data: vgafix.fon, vgasys.fon, vga850.fon and 96 respectively
 * (vgaoem.fon would be your oemfont.fon if you have a US setup).
 * If the resolution is changed to be >= 109dpi then the fonts goto 8514fix, 8514sys and 8514oem
 * (not sure what's happening to the oem codepage here). 109 is nicely halfway between 96 and 120dpi,
 * so that makes sense.
 *
 * Additionally Windows also loads the fonts listed in the [386enh] section of system.ini (this doesn't appear
 * to be mapped into the registry on Windows 2000 at least).
 * I have
 * woafont=app850.fon
 * ega80woa.fon=ega80850.fon
 * ega40woa.fon=ega40850.fon
 * cga80woa.fon=cga80850.fon
 * cga40woa.fon=cga40850.fon
 */

/* 
   This function builds an FT_Fixed from a double. It fails if the absolute
   value of the float number is greater than 32768.
*/
static inline FT_Fixed FT_FixedFromFloat(double f)
{
	return f * 0x10000;
}

/* 
   This function builds an FT_Fixed from a FIXED. It simply put f.value 
   in the highest 16 bits and f.fract in the lowest 16 bits of the FT_Fixed.
*/
static inline FT_Fixed FT_FixedFromFIXED(FIXED f)
{
    return (FT_Fixed)((int)f.value << 16 | (unsigned int)f.fract);
}

static BOOL is_hinting_enabled(void)
{
    static int enabled = -1;

    if (enabled == -1)
    {
        FT_TrueTypeEngineType type = pFT_Get_TrueType_Engine_Type(library);
        enabled = (type == FT_TRUETYPE_ENGINE_TYPE_PATENTED);
        TRACE("hinting is %senabled\n", enabled ? "" : "NOT ");
    }
    return enabled;
}

static BOOL is_subpixel_rendering_enabled( void )
{
    static int enabled = -1;
    if (enabled == -1)
    {
        /* FreeType >= 2.8.1 offers LCD-optimezed rendering without lcd filters. */
        if (FT_SimpleVersion >= FT_VERSION_VALUE(2, 8, 1))
            enabled = TRUE;
        else if (pFT_Library_SetLcdFilter( NULL, 0 ) != FT_Err_Unimplemented_Feature)
            enabled = TRUE;
        else enabled = FALSE;

        TRACE("subpixel rendering is %senabled\n", enabled ? "" : "NOT ");
    }
    return enabled;
}


static const LANGID mac_langid_table[] =
{
    MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_ENGLISH */
    MAKELANGID(LANG_FRENCH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_FRENCH */
    MAKELANGID(LANG_GERMAN,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_GERMAN */
    MAKELANGID(LANG_ITALIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_ITALIAN */
    MAKELANGID(LANG_DUTCH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_DUTCH */
    MAKELANGID(LANG_SWEDISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SWEDISH */
    MAKELANGID(LANG_SPANISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SPANISH */
    MAKELANGID(LANG_DANISH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_DANISH */
    MAKELANGID(LANG_PORTUGUESE,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_PORTUGUESE */
    MAKELANGID(LANG_NORWEGIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_NORWEGIAN */
    MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_HEBREW */
    MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_JAPANESE */
    MAKELANGID(LANG_ARABIC,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_ARABIC */
    MAKELANGID(LANG_FINNISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_FINNISH */
    MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_GREEK */
    MAKELANGID(LANG_ICELANDIC,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_ICELANDIC */
    MAKELANGID(LANG_MALTESE,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_MALTESE */
    MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TURKISH */
    MAKELANGID(LANG_CROATIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_CROATIAN */
    MAKELANGID(LANG_CHINESE_TRADITIONAL,SUBLANG_DEFAULT),    /* TT_MAC_LANGID_CHINESE_TRADITIONAL */
    MAKELANGID(LANG_URDU,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_URDU */
    MAKELANGID(LANG_HINDI,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_HINDI */
    MAKELANGID(LANG_THAI,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_THAI */
    MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_KOREAN */
    MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_LITHUANIAN */
    MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_POLISH */
    MAKELANGID(LANG_HUNGARIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_HUNGARIAN */
    MAKELANGID(LANG_ESTONIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ESTONIAN */
    MAKELANGID(LANG_LATVIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_LETTISH */
    MAKELANGID(LANG_SAMI,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_SAAMISK */
    MAKELANGID(LANG_FAEROESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_FAEROESE */
    MAKELANGID(LANG_FARSI,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_FARSI */
    MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_RUSSIAN */
    MAKELANGID(LANG_CHINESE_SIMPLIFIED,SUBLANG_DEFAULT),     /* TT_MAC_LANGID_CHINESE_SIMPLIFIED */
    MAKELANGID(LANG_DUTCH,SUBLANG_DUTCH_BELGIAN),            /* TT_MAC_LANGID_FLEMISH */
    MAKELANGID(LANG_IRISH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_IRISH */
    MAKELANGID(LANG_ALBANIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ALBANIAN */
    MAKELANGID(LANG_ROMANIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ROMANIAN */
    MAKELANGID(LANG_CZECH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_CZECH */
    MAKELANGID(LANG_SLOVAK,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_SLOVAK */
    MAKELANGID(LANG_SLOVENIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_SLOVENIAN */
    0,                                                       /* TT_MAC_LANGID_YIDDISH */
    MAKELANGID(LANG_SERBIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SERBIAN */
    MAKELANGID(LANG_MACEDONIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_MACEDONIAN */
    MAKELANGID(LANG_BULGARIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_BULGARIAN */
    MAKELANGID(LANG_UKRAINIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_UKRAINIAN */
    MAKELANGID(LANG_BELARUSIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_BYELORUSSIAN */
    MAKELANGID(LANG_UZBEK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_UZBEK */
    MAKELANGID(LANG_KAZAK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_KAZAKH */
    MAKELANGID(LANG_AZERI,SUBLANG_AZERI_CYRILLIC),           /* TT_MAC_LANGID_AZERBAIJANI */
    0,                                                       /* TT_MAC_LANGID_AZERBAIJANI_ARABIC_SCRIPT */
    MAKELANGID(LANG_ARMENIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ARMENIAN */
    MAKELANGID(LANG_GEORGIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GEORGIAN */
    0,                                                       /* TT_MAC_LANGID_MOLDAVIAN */
    MAKELANGID(LANG_KYRGYZ,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_KIRGHIZ */
    MAKELANGID(LANG_TAJIK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TAJIKI */
    MAKELANGID(LANG_TURKMEN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TURKMEN */
    MAKELANGID(LANG_MONGOLIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_MONGOLIAN */
    MAKELANGID(LANG_MONGOLIAN,SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA), /* TT_MAC_LANGID_MONGOLIAN_CYRILLIC_SCRIPT */
    MAKELANGID(LANG_PASHTO,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_PASHTO */
    0,                                                       /* TT_MAC_LANGID_KURDISH */
    MAKELANGID(LANG_KASHMIRI,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_KASHMIRI */
    MAKELANGID(LANG_SINDHI,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_SINDHI */
    MAKELANGID(LANG_TIBETAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TIBETAN */
    MAKELANGID(LANG_NEPALI,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_NEPALI */
    MAKELANGID(LANG_SANSKRIT,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_SANSKRIT */
    MAKELANGID(LANG_MARATHI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_MARATHI */
    MAKELANGID(LANG_BENGALI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_BENGALI */
    MAKELANGID(LANG_ASSAMESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ASSAMESE */
    MAKELANGID(LANG_GUJARATI,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GUJARATI */
    MAKELANGID(LANG_PUNJABI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_PUNJABI */
    MAKELANGID(LANG_ORIYA,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_ORIYA */
    MAKELANGID(LANG_MALAYALAM,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_MALAYALAM */
    MAKELANGID(LANG_KANNADA,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_KANNADA */
    MAKELANGID(LANG_TAMIL,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TAMIL */
    MAKELANGID(LANG_TELUGU,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_TELUGU */
    MAKELANGID(LANG_SINHALESE,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_SINHALESE */
    0,                                                       /* TT_MAC_LANGID_BURMESE */
    MAKELANGID(LANG_KHMER,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_KHMER */
    MAKELANGID(LANG_LAO,SUBLANG_DEFAULT),                    /* TT_MAC_LANGID_LAO */
    MAKELANGID(LANG_VIETNAMESE,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_VIETNAMESE */
    MAKELANGID(LANG_INDONESIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_INDONESIAN */
    0,                                                       /* TT_MAC_LANGID_TAGALOG */
    MAKELANGID(LANG_MALAY,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_MALAY_ROMAN_SCRIPT */
    0,                                                       /* TT_MAC_LANGID_MALAY_ARABIC_SCRIPT */
    MAKELANGID(LANG_AMHARIC,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_AMHARIC */
    MAKELANGID(LANG_TIGRIGNA,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_TIGRINYA */
    0,                                                       /* TT_MAC_LANGID_GALLA */
    0,                                                       /* TT_MAC_LANGID_SOMALI */
    MAKELANGID(LANG_SWAHILI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SWAHILI */
    0,                                                       /* TT_MAC_LANGID_RUANDA */
    0,                                                       /* TT_MAC_LANGID_RUNDI */
    0,                                                       /* TT_MAC_LANGID_CHEWA */
    0,                                                       /* TT_MAC_LANGID_MALAGASY */
    0,                                                       /* TT_MAC_LANGID_ESPERANTO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       /* 95-111 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 112-127 */
    MAKELANGID(LANG_WELSH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_WELSH */
    MAKELANGID(LANG_BASQUE,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_BASQUE */
    MAKELANGID(LANG_CATALAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_CATALAN */
    0,                                                       /* TT_MAC_LANGID_LATIN */
    MAKELANGID(LANG_QUECHUA,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_QUECHUA */
    0,                                                       /* TT_MAC_LANGID_GUARANI */
    0,                                                       /* TT_MAC_LANGID_AYMARA */
    MAKELANGID(LANG_TATAR,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TATAR */
    MAKELANGID(LANG_UIGHUR,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_UIGHUR */
    0,                                                       /* TT_MAC_LANGID_DZONGKHA */
    0,                                                       /* TT_MAC_LANGID_JAVANESE */
    0,                                                       /* TT_MAC_LANGID_SUNDANESE */
    MAKELANGID(LANG_GALICIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GALICIAN */
    MAKELANGID(LANG_AFRIKAANS,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_AFRIKAANS */
    MAKELANGID(LANG_BRETON,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_BRETON */
    MAKELANGID(LANG_INUKTITUT,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_INUKTITUT */
    MAKELANGID(LANG_SCOTTISH_GAELIC,SUBLANG_DEFAULT),        /* TT_MAC_LANGID_SCOTTISH_GAELIC */
    0,                                                       /* TT_MAC_LANGID_MANX_GAELIC */
    MAKELANGID(LANG_IRISH,SUBLANG_IRISH_IRELAND),            /* TT_MAC_LANGID_IRISH_GAELIC */
    0,                                                       /* TT_MAC_LANGID_TONGAN */
    0,                                                       /* TT_MAC_LANGID_GREEK_POLYTONIC */
    MAKELANGID(LANG_GREENLANDIC,SUBLANG_DEFAULT),            /* TT_MAC_LANGID_GREELANDIC */
    MAKELANGID(LANG_AZERI,SUBLANG_AZERI_LATIN),              /* TT_MAC_LANGID_AZERBAIJANI_ROMAN_SCRIPT */
};

static CPTABLEINFO *get_mac_code_page( const FT_SfntName *name )
{
    int id = name->encoding_id;

    if (name->encoding_id == TT_MAC_ID_SIMPLIFIED_CHINESE) id = 8;  /* special case */
    return get_cptable( 10000 + id );
}

static int match_name_table_language( const FT_SfntName *name, LANGID lang )
{
    LANGID name_lang;
    int res = 0;

    switch (name->platform_id)
    {
    case TT_PLATFORM_MICROSOFT:
        res += 5;  /* prefer the Microsoft name */
        switch (name->encoding_id)
        {
        case TT_MS_ID_UNICODE_CS:
        case TT_MS_ID_SYMBOL_CS:
            name_lang = name->language_id;
            break;
        default:
            return 0;
        }
        break;
    case TT_PLATFORM_MACINTOSH:
        if (!get_mac_code_page( name )) return 0;
        if (name->language_id >= ARRAY_SIZE( mac_langid_table )) return 0;
        name_lang = mac_langid_table[name->language_id];
        break;
    case TT_PLATFORM_APPLE_UNICODE:
        res += 2;  /* prefer Unicode encodings */
        switch (name->encoding_id)
        {
        case TT_APPLE_ID_DEFAULT:
        case TT_APPLE_ID_ISO_10646:
        case TT_APPLE_ID_UNICODE_2_0:
            if (name->language_id >= ARRAY_SIZE( mac_langid_table )) return 0;
            name_lang = mac_langid_table[name->language_id];
            break;
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
    if (name_lang == lang) res += 30;
    else if (PRIMARYLANGID( name_lang ) == PRIMARYLANGID( lang )) res += 20;
    else if (name_lang == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT )) res += 10;
    else if (lang == MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )) res += 5 * (0x100000 - name_lang);
    return res;
}

static WCHAR *copy_name_table_string( const FT_SfntName *name )
{
    WCHAR *ret;
    CPTABLEINFO *cp;
    DWORD i;

    switch (name->platform_id)
    {
    case TT_PLATFORM_APPLE_UNICODE:
    case TT_PLATFORM_MICROSOFT:
        ret = malloc( name->string_len + sizeof(WCHAR) );
        for (i = 0; i < name->string_len / 2; i++)
            ret[i] = (name->string[i * 2] << 8) | name->string[i * 2 + 1];
        ret[i] = 0;
        return ret;
    case TT_PLATFORM_MACINTOSH:
        if (!(cp = get_mac_code_page( name ))) return NULL;
        ret = malloc( (name->string_len + 1) * sizeof(WCHAR) );
        i = win32u_mbtowc( cp, ret, name->string_len, (char *)name->string, name->string_len );
        ret[i] = 0;
        return ret;
    }
    return NULL;
}

static WCHAR *get_face_name(FT_Face ft_face, FT_UShort name_id, LANGID language_id)
{
    FT_SfntName name;
    FT_UInt num_names, name_index;
    int res, best_lang = 0, best_index = -1;

    if (!FT_IS_SFNT(ft_face)) return NULL;

    num_names = pFT_Get_Sfnt_Name_Count( ft_face );

    for (name_index = 0; name_index < num_names; name_index++)
    {
        if (pFT_Get_Sfnt_Name( ft_face, name_index, &name )) continue;
        if (name.name_id != name_id) continue;
        res = match_name_table_language( &name, language_id );
        if (res > best_lang)
        {
            best_lang = res;
            best_index = name_index;
        }
    }

    if (best_index != -1 && !pFT_Get_Sfnt_Name( ft_face, best_index, &name ))
    {
        WCHAR *ret = copy_name_table_string( &name );
        TRACE( "name %u found platform %u lang %04x %s\n",
               name_id, name.platform_id, name.language_id, debugstr_w( ret ));
        return ret;
    }
    return NULL;
}

static WCHAR *ft_face_get_family_name( FT_Face ft_face, LANGID langid )
{
    WCHAR *family_name;

    if ((family_name = get_face_name( ft_face, TT_NAME_ID_FONT_FAMILY, langid )))
        return family_name;

    return towstr( ft_face->family_name );
}

static WCHAR *ft_face_get_style_name( FT_Face ft_face, LANGID langid )
{
    WCHAR *style_name;

    if ((style_name = get_face_name( ft_face, TT_NAME_ID_FONT_SUBFAMILY, langid )))
        return style_name;

    return towstr( ft_face->style_name );
}

static WCHAR *ft_face_get_full_name( FT_Face ft_face, LANGID langid )
{
    static const WCHAR space_w[] = {' ',0};
    WCHAR *full_name, *style_name;
    SIZE_T length;

    if ((full_name = get_face_name( ft_face, TT_NAME_ID_FULL_NAME, langid )))
        return full_name;

    full_name = ft_face_get_family_name( ft_face, langid );
    style_name = ft_face_get_style_name( ft_face, langid );

    length = lstrlenW( full_name ) + lstrlenW( space_w ) + lstrlenW( style_name ) + 1;
    full_name = realloc( full_name, length * sizeof(WCHAR) );

    lstrcatW( full_name, space_w );
    lstrcatW( full_name, style_name );
    free( style_name );

    WARN( "full name not found, using %s instead\n", debugstr_w(full_name) );
    return full_name;
}

static inline FT_Fixed get_font_version( FT_Face ft_face )
{
    FT_Fixed version = 0;
    TT_Header *header;

    header = pFT_Get_Sfnt_Table( ft_face, ft_sfnt_head );
    if (header) version = header->Font_Revision;

    return version;
}

static inline DWORD get_ntm_flags( FT_Face ft_face )
{
    DWORD flags = 0;
    FT_ULong table_size = 0;
    FT_WinFNT_HeaderRec winfnt_header;

    if (ft_face->style_flags & FT_STYLE_FLAG_ITALIC) flags |= NTM_ITALIC;
    if (ft_face->style_flags & FT_STYLE_FLAG_BOLD)   flags |= NTM_BOLD;

    /* fixup the flag for our fake-bold implementation. */
    if (!FT_IS_SCALABLE( ft_face ) &&
        !pFT_Get_WinFNT_Header( ft_face, &winfnt_header ) &&
        winfnt_header.weight > FW_NORMAL )
        flags |= NTM_BOLD;

    if (flags == 0) flags = NTM_REGULAR;

    if (!pFT_Load_Sfnt_Table( ft_face, FT_MAKE_TAG( 'C','F','F',' ' ), 0, NULL, &table_size ))
        flags |= NTM_PS_OPENTYPE;

    return flags;
}

static inline void get_bitmap_size( FT_Face ft_face, struct bitmap_font_size *face_size )
{
    My_FT_Bitmap_Size *size;
    FT_WinFNT_HeaderRec winfnt_header;

    size = (My_FT_Bitmap_Size *)ft_face->available_sizes;
    TRACE("Adding bitmap size h %d w %d size %ld x_ppem %ld y_ppem %ld\n",
          size->height, size->width, size->size >> 6,
          size->x_ppem >> 6, size->y_ppem >> 6);
    face_size->height = size->height;
    face_size->width = size->width;
    face_size->size = size->size;
    face_size->x_ppem = size->x_ppem;
    face_size->y_ppem = size->y_ppem;

    if (!pFT_Get_WinFNT_Header( ft_face, &winfnt_header )) {
        face_size->internal_leading = winfnt_header.internal_leading;
        if (winfnt_header.external_leading > 0 &&
            (face_size->height ==
             winfnt_header.pixel_height + winfnt_header.external_leading))
            face_size->height = winfnt_header.pixel_height;
    }
}

static inline void get_fontsig( FT_Face ft_face, FONTSIGNATURE *fs )
{
    TT_OS2 *os2;
    FT_WinFNT_HeaderRec winfnt_header;
    int i;

    memset( fs, 0, sizeof(*fs) );

    os2 = pFT_Get_Sfnt_Table( ft_face, ft_sfnt_os2 );
    if (os2)
    {
        fs->fsUsb[0] = os2->ulUnicodeRange1;
        fs->fsUsb[1] = os2->ulUnicodeRange2;
        fs->fsUsb[2] = os2->ulUnicodeRange3;
        fs->fsUsb[3] = os2->ulUnicodeRange4;

        if (os2->version == 0)
        {
            if (os2->usFirstCharIndex >= 0xf000 && os2->usFirstCharIndex < 0xf100)
                fs->fsCsb[0] = FS_SYMBOL;
            else
                fs->fsCsb[0] = FS_LATIN1;
        }
        else
        {
            fs->fsCsb[0] = os2->ulCodePageRange1;
            fs->fsCsb[1] = os2->ulCodePageRange2;
        }
    }
    else
    {
        if (!pFT_Get_WinFNT_Header( ft_face, &winfnt_header ))
        {
            TRACE("pix_h %d charset %d dpi %dx%d pt %d\n", winfnt_header.pixel_height, winfnt_header.charset,
                  winfnt_header.vertical_resolution,winfnt_header.horizontal_resolution, winfnt_header.nominal_point_size);
            switch (winfnt_header.charset)
            {
            case ANSI_CHARSET:        fs->fsCsb[0] = FS_LATIN1; break;
            case EASTEUROPE_CHARSET:  fs->fsCsb[0] = FS_LATIN2; break;
            case RUSSIAN_CHARSET:     fs->fsCsb[0] = FS_CYRILLIC; break;
            case GREEK_CHARSET:       fs->fsCsb[0] = FS_GREEK; break;
            case TURKISH_CHARSET:     fs->fsCsb[0] = FS_TURKISH; break;
            case HEBREW_CHARSET:      fs->fsCsb[0] = FS_HEBREW; break;
            case ARABIC_CHARSET:      fs->fsCsb[0] = FS_ARABIC; break;
            case BALTIC_CHARSET:      fs->fsCsb[0] = FS_BALTIC; break;
            case VIETNAMESE_CHARSET:  fs->fsCsb[0] = FS_VIETNAMESE; break;
            case THAI_CHARSET:        fs->fsCsb[0] = FS_THAI; break;
            case SHIFTJIS_CHARSET:    fs->fsCsb[0] = FS_JISJAPAN; break;
            case GB2312_CHARSET:      fs->fsCsb[0] = FS_CHINESESIMP; break;
            case HANGEUL_CHARSET:     fs->fsCsb[0] = FS_WANSUNG; break;
            case CHINESEBIG5_CHARSET: fs->fsCsb[0] = FS_CHINESETRAD; break;
            case JOHAB_CHARSET:       fs->fsCsb[0] = FS_JOHAB; break;
            case SYMBOL_CHARSET:      fs->fsCsb[0] = FS_SYMBOL; break;
            }
        }
    }

    if (fs->fsCsb[0] == 0)
    {
        /* let's see if we can find any interesting cmaps */
        for (i = 0; i < ft_face->num_charmaps; i++)
        {
            switch (ft_face->charmaps[i]->encoding)
            {
            case FT_ENCODING_UNICODE:
            case FT_ENCODING_APPLE_ROMAN:
                fs->fsCsb[0] |= FS_LATIN1;
                break;
            case FT_ENCODING_MS_SYMBOL:
                fs->fsCsb[0] |= FS_SYMBOL;
                break;
            default:
                break;
            }
        }
    }
}

static FT_Face new_ft_face( const char *file, void *font_data_ptr, UINT font_data_size,
                            FT_Long face_index, BOOL allow_bitmap )
{
    FT_Error err;
    TT_OS2 *pOS2;
    FT_Face ft_face;

    if (file)
    {
        TRACE("Loading font file %s index %ld\n", debugstr_a(file), face_index);
        err = pFT_New_Face(library, file, face_index, &ft_face);
    }
    else
    {
        TRACE("Loading font from ptr %p size %d, index %ld\n", font_data_ptr, font_data_size, face_index);
        err = pFT_New_Memory_Face(library, font_data_ptr, font_data_size, face_index, &ft_face);
    }

    if (err != 0)
    {
        WARN("Unable to load font %s/%p err = %x\n", debugstr_a(file), font_data_ptr, err);
        return NULL;
    }

    if (!FT_IS_SFNT( ft_face ))
    {
        if (FT_IS_SCALABLE( ft_face ) || !allow_bitmap )
        {
            WARN("Ignoring font %s/%p\n", debugstr_a(file), font_data_ptr);
            goto fail;
        }
    }
    else
    {
        if (!(pOS2 = pFT_Get_Sfnt_Table( ft_face, ft_sfnt_os2 )) ||
            !pFT_Get_Sfnt_Table( ft_face, ft_sfnt_hhea ) ||
            !pFT_Get_Sfnt_Table( ft_face, ft_sfnt_head ))
        {
            TRACE("Font %s/%p lacks either an OS2, HHEA or HEAD table.\n"
                  "Skipping this font.\n", debugstr_a(file), font_data_ptr);
            goto fail;
        }

        /* Wine uses ttfs as an intermediate step in building its bitmap fonts;
           we don't want to load these. */
        if (!memcmp( pOS2->achVendID, "Wine", sizeof(pOS2->achVendID) ))
        {
            FT_ULong len = 0;

            if (!pFT_Load_Sfnt_Table( ft_face, FT_MAKE_TAG('E','B','S','C'), 0, NULL, &len ))
            {
                TRACE("Skipping Wine bitmap-only TrueType font %s\n", debugstr_a(file));
                goto fail;
            }
        }
    }

    if (!ft_face->family_name || !ft_face->style_name)
    {
        TRACE("Font %s/%p lacks either a family or style name\n", debugstr_a(file), font_data_ptr);
        goto fail;
    }

    return ft_face;
fail:
    pFT_Done_Face( ft_face );
    return NULL;
}

struct family_names_data
{
    LANGID primary_langid;
    struct opentype_name family_name;
    struct opentype_name second_name;
    BOOL primary_seen;
    BOOL english_seen;
};

static BOOL search_family_names_callback( LANGID langid, struct opentype_name *name, void *user )
{
    struct family_names_data *data = user;

    if (langid == MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT))
    {
        data->english_seen = TRUE;
        if (data->primary_langid == langid) data->primary_seen = TRUE;

        if (!data->family_name.bytes) data->family_name = *name;
        else if (data->primary_langid != langid) data->second_name = *name;
    }
    else if (data->primary_langid == langid)
    {
        data->primary_seen = TRUE;
        if (data->family_name.bytes) data->second_name = data->family_name;
        data->family_name = *name;
    }
    else if (!data->second_name.bytes) data->second_name = *name;

    if (data->family_name.bytes && data->second_name.bytes && data->primary_seen && data->english_seen)
        return TRUE;
    return FALSE;
}

struct face_name_data
{
    LANGID primary_langid;
    struct opentype_name face_name;
};

static BOOL search_face_name_callback( LANGID langid, struct opentype_name *name, void *user )
{
    struct face_name_data *data = user;

    if (langid == data->primary_langid || (langid == MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT) && !data->face_name.bytes))
        data->face_name = *name;

    return langid == data->primary_langid;
}

static WCHAR *decode_opentype_name( struct opentype_name *name )
{
    WCHAR buffer[512];
    DWORD len;

    if (!name->codepage)
    {
        len = min( ARRAY_SIZE(buffer), name->length / sizeof(WCHAR) );
        while (len--) buffer[len] = GET_BE_WORD( ((WORD *)name->bytes)[len] );
        len = min( ARRAY_SIZE(buffer), name->length / sizeof(WCHAR) );
    }
    else
    {
        CPTABLEINFO *cptable = get_cptable( name->codepage );
        if (!cptable) return NULL;
        len = win32u_mbtowc( cptable, buffer, ARRAY_SIZE(buffer), name->bytes, name->length );
    }

    buffer[ARRAY_SIZE(buffer) - 1] = 0;
    if (len == ARRAY_SIZE(buffer)) WARN("Truncated font name %s -> %s\n", debugstr_an(name->bytes, name->length), debugstr_w(buffer));
    else buffer[len] = 0;

    return wcsdup( buffer );
}

struct unix_face
{
    FT_Face ft_face;
    BOOL scalable;
    UINT num_faces;
    WCHAR *family_name;
    WCHAR *second_name;
    WCHAR *style_name;
    WCHAR *full_name;
    DWORD ntm_flags;
    UINT weight;
    DWORD font_version;
    FONTSIGNATURE fs;
    struct bitmap_font_size size;
};

static struct unix_face *unix_face_create( const char *unix_name, void *data_ptr, UINT data_size,
                                           UINT face_index, UINT flags )
{
    static const WCHAR space_w[] = {' ',0};

    const struct ttc_sfnt_v1 *ttc_sfnt_v1;
    const struct tt_name_v0 *tt_name_v0;
    struct unix_face *This;
    struct stat st;
    DWORD face_count;
    int fd, length;

    TRACE( "unix_name %s, face_index %u, data_ptr %p, data_size %u, flags %#x\n",
           unix_name, face_index, data_ptr, data_size, flags );

    if (unix_name)
    {
        if ((fd = open( unix_name, O_RDONLY )) == -1) return NULL;
        if (fstat( fd, &st ) == -1)
        {
            close( fd );
            return NULL;
        }
        data_size = st.st_size;
        data_ptr = mmap( NULL, data_size, PROT_READ, MAP_PRIVATE, fd, 0 );
        close( fd );
        if (data_ptr == MAP_FAILED) return NULL;
    }

    if (!(This = calloc( 1, sizeof(*This) ))) goto done;

    if (opentype_get_ttc_sfnt_v1( data_ptr, data_size, face_index, &face_count, &ttc_sfnt_v1 ) &&
        opentype_get_tt_name_v0( data_ptr, data_size, ttc_sfnt_v1, &tt_name_v0 ) &&
        opentype_get_properties( data_ptr, data_size, ttc_sfnt_v1, &This->font_version,
                                 &This->fs, &This->ntm_flags, &This->weight ))
    {
        struct family_names_data family_names;
        struct face_name_data style_name;
        struct face_name_data full_name;
        LANGID primary_langid = system_lcid;

        This->scalable = TRUE;
        This->num_faces = face_count;

        memset( &family_names, 0, sizeof(family_names) );
        family_names.primary_langid = primary_langid;
        opentype_enum_family_names( tt_name_v0, search_family_names_callback, &family_names );
        This->family_name = decode_opentype_name( &family_names.family_name );
        This->second_name = decode_opentype_name( &family_names.second_name );

        memset( &style_name, 0, sizeof(style_name) );
        style_name.primary_langid = primary_langid;
        opentype_enum_style_names( tt_name_v0, search_face_name_callback, &style_name );
        This->style_name = decode_opentype_name( &style_name.face_name );

        memset( &full_name, 0, sizeof(full_name) );
        full_name.primary_langid = primary_langid;
        opentype_enum_full_names( tt_name_v0, search_face_name_callback, &full_name );
        This->full_name = decode_opentype_name( &full_name.face_name );

        TRACE( "parsed font names family_name %s, second_name %s, primary_seen %d, english_seen %d, "
               "full_name %s, style_name %s\n",
               debugstr_w(This->family_name), debugstr_w(This->second_name),
               family_names.primary_seen, family_names.english_seen,
               debugstr_w(This->full_name), debugstr_w(This->style_name) );

        if (!This->full_name && This->family_name && This->style_name)
        {
            length = lstrlenW( This->family_name ) + lstrlenW( space_w ) + lstrlenW( This->style_name ) + 1;
            This->full_name = malloc( length * sizeof(WCHAR) );
            lstrcpyW( This->full_name, This->family_name );
            lstrcatW( This->full_name, space_w );
            lstrcatW( This->full_name, This->style_name );
            WARN( "full name not found, using %s instead\n", debugstr_w(This->full_name) );
        }
    }
    else if ((This->ft_face = new_ft_face( unix_name, data_ptr, data_size, face_index, flags & ADDFONT_ALLOW_BITMAP )))
    {
        TT_OS2 *os2;

        WARN( "unable to parse font, falling back to FreeType\n" );
        This->scalable = FT_IS_SCALABLE( This->ft_face );
        This->num_faces = This->ft_face->num_faces;

        This->family_name = ft_face_get_family_name( This->ft_face, system_lcid );
        This->second_name = ft_face_get_family_name( This->ft_face, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT) );

        /* try to find another secondary name, preferring the lowest langids */
        if (!wcsicmp( This->family_name, This->second_name ))
        {
            free( This->second_name );
            This->second_name = ft_face_get_family_name( This->ft_face, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );
            if (!wcsicmp( This->family_name, This->second_name ))
            {
                free( This->second_name );
                This->second_name = NULL;
            }
        }

        This->style_name = ft_face_get_style_name( This->ft_face, system_lcid );
        This->full_name = ft_face_get_full_name( This->ft_face, system_lcid );
        This->ntm_flags = get_ntm_flags( This->ft_face );
        if ((os2 = pFT_Get_Sfnt_Table(This->ft_face, ft_sfnt_os2)))
            This->weight = os2->usWeightClass;
        else
            This->weight = This->ntm_flags & NTM_BOLD ? FW_BOLD : FW_NORMAL;
        This->font_version = get_font_version( This->ft_face );
        if (!This->scalable) get_bitmap_size( This->ft_face, &This->size );
        get_fontsig( This->ft_face, &This->fs );
    }
    else
    {
        free( This );
        This = NULL;
    }

done:
    if (unix_name) munmap( data_ptr, data_size );
    return This;
}

static void unix_face_destroy( struct unix_face *This )
{
    if (This->ft_face) pFT_Done_Face( This->ft_face );
    free( This->full_name );
    free( This->style_name );
    free( This->second_name );
    free( This->family_name );
    free( This );
}

static int add_unix_face( const char *unix_name, const WCHAR *file, void *data_ptr, SIZE_T data_size,
                          DWORD face_index, DWORD flags, DWORD *num_faces )
{
    struct unix_face *unix_face;
    int ret;

    if (num_faces) *num_faces = 0;

    if (!(unix_face = unix_face_create( unix_name, data_ptr, data_size, face_index, flags )))
        return 0;

    if (unix_face->family_name[0] == '.') /* Ignore fonts with names beginning with a dot */
    {
        TRACE("Ignoring %s since its family name begins with a dot\n", debugstr_a(unix_name));
        unix_face_destroy( unix_face );
        return 0;
    }

    if (!HIWORD( flags )) flags |= ADDFONT_AA_FLAGS( default_aa_flags );

    ret = add_gdi_face( unix_face->family_name, unix_face->second_name, unix_face->style_name, unix_face->full_name,
                        file, data_ptr, data_size, face_index, unix_face->fs, unix_face->ntm_flags, unix_face->weight,
                        unix_face->font_version, flags, unix_face->scalable ? NULL : &unix_face->size );

    TRACE("fsCsb = %08x %08x/%08x %08x %08x %08x\n",
          unix_face->fs.fsCsb[0], unix_face->fs.fsCsb[1],
          unix_face->fs.fsUsb[0], unix_face->fs.fsUsb[1],
          unix_face->fs.fsUsb[2], unix_face->fs.fsUsb[3]);

    if (num_faces) *num_faces = unix_face->num_faces;
    unix_face_destroy( unix_face );
    return ret;
}

static INT AddFontToList(const WCHAR *dos_name, const char *unix_name, void *font_data_ptr,
                         UINT font_data_size, UINT flags)
{
    DWORD face_index = 0, num_faces;
    INT ret = 0;
    WCHAR *filename = NULL;

    /* we always load external fonts from files - otherwise we would get a crash in update_reg_entries */
    assert(unix_name || !(flags & ADDFONT_EXTERNAL_FONT));

    if (!dos_name && unix_name && !ntdll_get_dos_file_name( unix_name, &filename, FILE_OPEN ))
        dos_name = filename;

    do
        ret += add_unix_face( unix_name, dos_name, font_data_ptr, font_data_size, face_index, flags, &num_faces );
    while (num_faces > ++face_index);

    free( filename );
    return ret;
}

/*************************************************************
 * freetype_add_font
 */
static INT freetype_add_font( const WCHAR *file, UINT flags )
{
    int ret = 0;
    char *unixname = NULL;

    ntdll_get_unix_file_name( file, &unixname, FILE_OPEN_IF );
    if (unixname)
    {
        ret = AddFontToList( file, unixname, NULL, 0, flags );
        free( unixname );
    }
    return ret;
}

/*************************************************************
 * freetype_add_mem_font
 */
static INT freetype_add_mem_font( void *ptr, SIZE_T size, UINT flags )
{
    return AddFontToList( NULL, NULL, ptr, size, flags );
}

#ifdef __ANDROID__
static BOOL ReadFontDir(const char *dirname, BOOL external_fonts)
{
    DIR *dir;
    struct dirent *dent;
    char path[PATH_MAX];

    TRACE("Loading fonts from %s\n", debugstr_a(dirname));

    dir = opendir(dirname);
    if(!dir) {
        WARN("Can't open directory %s\n", debugstr_a(dirname));
	return FALSE;
    }
    while((dent = readdir(dir)) != NULL) {
	struct stat statbuf;

        if(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
	    continue;

	TRACE("Found %s in %s\n", debugstr_a(dent->d_name), debugstr_a(dirname));

	snprintf(path, sizeof(path), "%s/%s", dirname, dent->d_name);

	if(stat(path, &statbuf) == -1)
	{
	    WARN("Can't stat %s\n", debugstr_a(path));
	    continue;
	}
	if(S_ISDIR(statbuf.st_mode))
	    ReadFontDir(path, external_fonts);
	else
        {
            DWORD addfont_flags = 0;
            if(external_fonts) addfont_flags |= ADDFONT_EXTERNAL_FONT;
            AddFontToList(NULL, path, NULL, 0, addfont_flags);
        }
    }
    closedir(dir);
    return TRUE;
}
#endif

#ifdef SONAME_LIBFONTCONFIG

static BOOL fontconfig_enabled;
static FcPattern *pattern_serif;
static FcPattern *pattern_fixed;
static FcPattern *pattern_sans;

static UINT parse_aa_pattern( FcPattern *pattern )
{
    FcBool antialias;
    int rgba;
    UINT aa_flags = 0;

    if (pFcPatternGetBool( pattern, FC_ANTIALIAS, 0, &antialias ) == FcResultMatch)
        aa_flags = antialias ? GGO_GRAY4_BITMAP : GGO_BITMAP;

    if (pFcPatternGetInteger( pattern, FC_RGBA, 0, &rgba ) == FcResultMatch)
    {
        switch (rgba)
        {
        case FC_RGBA_RGB:  aa_flags = WINE_GGO_HRGB_BITMAP; break;
        case FC_RGBA_BGR:  aa_flags = WINE_GGO_HBGR_BITMAP; break;
        case FC_RGBA_VRGB: aa_flags = WINE_GGO_VRGB_BITMAP; break;
        case FC_RGBA_VBGR: aa_flags = WINE_GGO_VBGR_BITMAP; break;
        case FC_RGBA_NONE: aa_flags = aa_flags ? aa_flags : GGO_GRAY4_BITMAP; break;
        }
    }
    return aa_flags;
}

static FcPattern *create_family_pattern( const char *name, FcPattern **cached )
{
    FcPattern *ret = NULL, *tmp, *pattern;
    FcResult result;
    if (*cached) return *cached;
    pattern = pFcPatternCreate();
    pFcPatternAddString( pattern, FC_FAMILY, (const FcChar8 *)name );
    pFcPatternAddString( pattern, FC_NAMELANG, (const FcChar8 *)"en-us" );
    pFcPatternAddString( pattern, FC_PRGNAME, (const FcChar8 *)"wine" );
    pFcConfigSubstitute( NULL, pattern, FcMatchPattern );
    pFcDefaultSubstitute( pattern );
    tmp = pFcFontMatch( NULL, pattern, &result );
    pFcPatternDestroy( pattern );
    if (result != FcResultMatch) pFcPatternDestroy( tmp );
    else if ((ret = InterlockedCompareExchangePointer( (void **)cached, tmp, NULL ))) pFcPatternDestroy( tmp );
    else ret = tmp;
    return ret;
}

static void fontconfig_add_font( FcPattern *pattern, UINT flags )
{
    const char *unix_name, *format;
    WCHAR *dos_name;
    FcBool scalable;
    DWORD aa_flags;
    int face_index;

    TRACE( "(%p %#x)\n", pattern, flags );

    if (pFcPatternGetString( pattern, FC_FILE, 0, (FcChar8 **)&unix_name ) != FcResultMatch)
        return;

    if (pFcPatternGetBool( pattern, FC_SCALABLE, 0, &scalable ) != FcResultMatch)
        scalable = FALSE;

    if (pFcPatternGetString( pattern, FC_FONTFORMAT, 0, (FcChar8 **)&format ) != FcResultMatch)
    {
        TRACE( "ignoring unknown font format %s\n", debugstr_a(unix_name) );
        return;
    }

    if (!strcmp( format, "Type 1" ))
    {
        TRACE( "ignoring Type 1 font %s\n", debugstr_a(unix_name) );
        return;
    }

    if (!scalable && !(flags & ADDFONT_ALLOW_BITMAP))
    {
        TRACE( "ignoring non-scalable font %s\n", debugstr_a(unix_name) );
        return;
    }

    if (!(aa_flags = parse_aa_pattern( pattern ))) aa_flags = default_aa_flags;
    flags |= ADDFONT_AA_FLAGS(aa_flags);

    if (pFcPatternGetInteger( pattern, FC_INDEX, 0, &face_index ) != FcResultMatch)
        face_index = 0;

    ntdll_get_dos_file_name( unix_name, &dos_name, FILE_OPEN );
    add_unix_face( unix_name, dos_name, NULL, 0, face_index, flags, NULL );
    free( dos_name );
}

static void init_fontconfig(void)
{
    void *fc_handle = dlopen(SONAME_LIBFONTCONFIG, RTLD_NOW);

    if (!fc_handle)
    {
        TRACE("Wine cannot find the fontconfig library (%s).\n", SONAME_LIBFONTCONFIG);
        return;
    }

#define LOAD_FUNCPTR(f) if((p##f = dlsym(fc_handle, #f)) == NULL){WARN("Can't find symbol %s\n", #f); return;}
    LOAD_FUNCPTR(FcConfigSubstitute);
    LOAD_FUNCPTR(FcDefaultSubstitute);
    LOAD_FUNCPTR(FcFontList);
    LOAD_FUNCPTR(FcFontMatch);
    LOAD_FUNCPTR(FcFontSetDestroy);
    LOAD_FUNCPTR(FcInit);
    LOAD_FUNCPTR(FcPatternAddString);
    LOAD_FUNCPTR(FcPatternCreate);
    LOAD_FUNCPTR(FcPatternDestroy);
    LOAD_FUNCPTR(FcPatternGetBool);
    LOAD_FUNCPTR(FcPatternGetInteger);
    LOAD_FUNCPTR(FcPatternGetString);
    LOAD_FUNCPTR(FcConfigGetFontDirs);
    LOAD_FUNCPTR(FcConfigGetCurrent);
    LOAD_FUNCPTR(FcCacheCopySet);
    LOAD_FUNCPTR(FcCacheNumSubdir);
    LOAD_FUNCPTR(FcCacheSubdir);
    LOAD_FUNCPTR(FcDirCacheRead);
    LOAD_FUNCPTR(FcDirCacheUnload);
    LOAD_FUNCPTR(FcStrListCreate);
    LOAD_FUNCPTR(FcStrListDone);
    LOAD_FUNCPTR(FcStrListNext);
    LOAD_FUNCPTR(FcStrSetAdd);
    LOAD_FUNCPTR(FcStrSetCreate);
    LOAD_FUNCPTR(FcStrSetDestroy);
    LOAD_FUNCPTR(FcStrSetMember);
#undef LOAD_FUNCPTR

    if (pFcInit())
    {
        FcPattern *pattern = pFcPatternCreate();
        pFcConfigSubstitute( NULL, pattern, FcMatchFont );
        default_aa_flags = parse_aa_pattern( pattern );
        pFcPatternDestroy( pattern );

        if (!default_aa_flags)
        {
            FcPattern *pattern = pFcPatternCreate();
            pFcConfigSubstitute( NULL, pattern, FcMatchPattern );
            default_aa_flags = parse_aa_pattern( pattern );
            pFcPatternDestroy( pattern );
        }

        TRACE( "enabled, default flags = %x\n", default_aa_flags );
        fontconfig_enabled = TRUE;
    }
}

static void fontconfig_add_fonts_from_dir_list( FcConfig *config, FcStrList *dir_list, FcStrSet *done_set, UINT flags )
{
    const FcChar8 *dir;
    FcFontSet *font_set = NULL;
    FcStrList *subdir_list = NULL;
    FcStrSet *subdir_set = NULL;
    FcCache *cache = NULL;
    int i;

    TRACE( "(%p %p %p %#x)\n", config, dir_list, done_set, flags );

    while ((dir = pFcStrListNext( dir_list )))
    {
        if (pFcStrSetMember( done_set, dir )) continue;

        TRACE( "adding fonts from %s\n", dir );
        if (!(cache = pFcDirCacheRead( dir, FcFalse, config ))) continue;

        if (!(font_set = pFcCacheCopySet( cache ))) goto done;
        for (i = 0; i < font_set->nfont; i++)
            fontconfig_add_font( font_set->fonts[i], flags );
        pFcFontSetDestroy( font_set );
        font_set = NULL;

        if (!(subdir_set = pFcStrSetCreate())) goto done;
        for (i = 0; i < pFcCacheNumSubdir( cache ); i++)
            pFcStrSetAdd( subdir_set, pFcCacheSubdir( cache, i ) );
        pFcDirCacheUnload( cache );
        cache = NULL;

        if (!(subdir_list = pFcStrListCreate( subdir_set ))) goto done;
        pFcStrSetDestroy( subdir_set );
        subdir_set = NULL;

        pFcStrSetAdd( done_set, dir );
        fontconfig_add_fonts_from_dir_list( config, subdir_list, done_set, flags );
        pFcStrListDone( subdir_list );
        subdir_list = NULL;
    }

done:
    if (subdir_set) pFcStrSetDestroy( subdir_set );
    if (cache) pFcDirCacheUnload( cache );
}

static void load_fontconfig_fonts( void )
{
    FcStrList *dir_list = NULL;
    FcStrSet *done_set = NULL;
    FcConfig *config;

    if (!fontconfig_enabled) return;
    if (!(config = pFcConfigGetCurrent())) goto done;
    if (!(done_set = pFcStrSetCreate())) goto done;
    if (!(dir_list = pFcConfigGetFontDirs( config ))) goto done;

    fontconfig_add_fonts_from_dir_list( config, dir_list, done_set, ADDFONT_EXTERNAL_FONT );

done:
    if (dir_list) pFcStrListDone( dir_list );
    if (done_set) pFcStrSetDestroy( done_set );
}

#elif defined(__APPLE__)

static void load_mac_font_callback(const void *value, void *context)
{
    CFStringRef pathStr = value;
    CFIndex len;
    char* path;

    len = CFStringGetMaximumSizeOfFileSystemRepresentation(pathStr);
    path = malloc( len );
    if (path && CFStringGetFileSystemRepresentation(pathStr, path, len))
    {
        TRACE("font file %s\n", path);
        AddFontToList(NULL, path, NULL, 0, ADDFONT_EXTERNAL_FONT);
    }
    free( path );
}

static void load_mac_fonts(void)
{
    CFStringRef removeDupesKey;
    CFBooleanRef removeDupesValue;
    CFDictionaryRef options;
    CTFontCollectionRef col;
    CFArrayRef descs;
    CFMutableSetRef paths;
    CFIndex i;

    removeDupesKey = kCTFontCollectionRemoveDuplicatesOption;
    removeDupesValue = kCFBooleanTrue;
    options = CFDictionaryCreate(NULL, (const void**)&removeDupesKey, (const void**)&removeDupesValue, 1,
                                 &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    col = CTFontCollectionCreateFromAvailableFonts(options);
    if (options) CFRelease(options);
    if (!col)
    {
        WARN("CTFontCollectionCreateFromAvailableFonts failed\n");
        return;
    }

    descs = CTFontCollectionCreateMatchingFontDescriptors(col);
    CFRelease(col);
    if (!descs)
    {
        WARN("CTFontCollectionCreateMatchingFontDescriptors failed\n");
        return;
    }

    paths = CFSetCreateMutable(NULL, 0, &kCFTypeSetCallBacks);
    if (!paths)
    {
        WARN("CFSetCreateMutable failed\n");
        CFRelease(descs);
        return;
    }

    for (i = 0; i < CFArrayGetCount(descs); i++)
    {
        CTFontDescriptorRef desc;
        CFURLRef url;
        CFStringRef ext;
        CFStringRef path;

        desc = CFArrayGetValueAtIndex(descs, i);
        url = CTFontDescriptorCopyAttribute(desc, kCTFontURLAttribute);
        if (!url) continue;

        ext = CFURLCopyPathExtension(url);
        if (ext)
        {
            BOOL skip = (CFStringCompare(ext, CFSTR("pfa"), kCFCompareCaseInsensitive) == kCFCompareEqualTo ||
                         CFStringCompare(ext, CFSTR("pfb"), kCFCompareCaseInsensitive) == kCFCompareEqualTo);
            CFRelease(ext);
            if (skip)
            {
                CFRelease(url);
                continue;
            }
        }

        path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
        CFRelease(url);
        if (!path) continue;

        CFSetAddValue(paths, path);
        CFRelease(path);
    }

    CFRelease(descs);

    CFSetApplyFunction(paths, load_mac_font_callback, NULL);
    CFRelease(paths);
}

#endif


static BOOL init_freetype(void)
{
    ft_handle = dlopen(SONAME_LIBFREETYPE, RTLD_NOW);
    if(!ft_handle) {
        WINE_MESSAGE(
      "Wine cannot find the FreeType font library.  To enable Wine to\n"
      "use TrueType fonts please install a version of FreeType greater than\n"
      "or equal to 2.0.5.\n"
      "http://www.freetype.org\n");
	return FALSE;
    }

#define LOAD_FUNCPTR(f) if((p##f = dlsym(ft_handle, #f)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}

    LOAD_FUNCPTR(FT_Done_Face)
    LOAD_FUNCPTR(FT_Get_Char_Index)
    LOAD_FUNCPTR(FT_Get_First_Char)
    LOAD_FUNCPTR(FT_Get_Next_Char)
    LOAD_FUNCPTR(FT_Get_Sfnt_Name)
    LOAD_FUNCPTR(FT_Get_Sfnt_Name_Count)
    LOAD_FUNCPTR(FT_Get_Sfnt_Table)
    LOAD_FUNCPTR(FT_Get_TrueType_Engine_Type)
    LOAD_FUNCPTR(FT_Get_WinFNT_Header)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Library_SetLcdFilter)
    LOAD_FUNCPTR(FT_Library_Version)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_Load_Sfnt_Table)
    LOAD_FUNCPTR(FT_Matrix_Multiply)
    LOAD_FUNCPTR(FT_MulDiv)
#ifndef FT_MULFIX_INLINED
    LOAD_FUNCPTR(FT_MulFix)
#endif
    LOAD_FUNCPTR(FT_New_Face)
    LOAD_FUNCPTR(FT_New_Memory_Face)
    LOAD_FUNCPTR(FT_Outline_Embolden)
    LOAD_FUNCPTR(FT_Outline_Get_Bitmap)
    LOAD_FUNCPTR(FT_Outline_Get_CBox)
    LOAD_FUNCPTR(FT_Outline_Transform)
    LOAD_FUNCPTR(FT_Outline_Translate)
    LOAD_FUNCPTR(FT_Property_Set)
    LOAD_FUNCPTR(FT_Render_Glyph)
    LOAD_FUNCPTR(FT_Set_Charmap)
    LOAD_FUNCPTR(FT_Set_Pixel_Sizes)
    LOAD_FUNCPTR(FT_Vector_Length)
    LOAD_FUNCPTR(FT_Vector_Transform)
    LOAD_FUNCPTR(FT_Vector_Unit)
#undef LOAD_FUNCPTR

    if(pFT_Init_FreeType(&library) != 0) {
        ERR("Can't init FreeType library\n");
	dlclose(ft_handle);
        ft_handle = NULL;
	return FALSE;
    }
    pFT_Library_Version(library,&FT_Version.major,&FT_Version.minor,&FT_Version.patch);

    TRACE("FreeType version is %d.%d.%d\n",FT_Version.major,FT_Version.minor,FT_Version.patch);
    FT_SimpleVersion = ((FT_Version.major << 16) & 0xff0000) |
                       ((FT_Version.minor <<  8) & 0x00ff00) |
                       ((FT_Version.patch      ) & 0x0000ff);

    /* In FreeType < 2.8.1 v40's FT_LOAD_TARGET_MONO has broken advance widths. */
    if (FT_SimpleVersion < FT_VERSION_VALUE(2, 8, 1))
    {
        FT_UInt interpreter_version = 35;
        pFT_Property_Set( library, "truetype", "interpreter-version", &interpreter_version );
    }

    pFT_Library_SetLcdFilter( library, FT_LCD_FILTER_DEFAULT );

    return TRUE;

sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the FreeType\n"
      "font library.  To enable Wine to use TrueType fonts please upgrade\n"
      "FreeType to at least version 2.5.0.\n"
      "http://www.freetype.org\n");
    dlclose(ft_handle);
    ft_handle = NULL;
    return FALSE;
}

/*************************************************************
 * freetype_load_fonts
 */
static void freetype_load_fonts(void)
{
#ifdef SONAME_LIBFONTCONFIG
    load_fontconfig_fonts();
#elif defined(__APPLE__)
    load_mac_fonts();
#elif defined(__ANDROID__)
    ReadFontDir("/system/fonts", TRUE);
#endif
}

/* Some fonts have large usWinDescent values, as a result of storing signed short
   in unsigned field. That's probably caused by sTypoDescent vs usWinDescent confusion in
   some font generation tools. */
static inline USHORT get_fixed_windescent(USHORT windescent)
{
    return abs((SHORT)windescent);
}

static int calc_ppem_for_height(FT_Face ft_face, int height)
{
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;

    int ppem;
    const int MAX_PPEM = (1 << 16) - 1;

    pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
    pHori = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea);

    if(height == 0) height = 16;

    /* Calc. height of EM square:
     *
     * For +ve lfHeight we have
     * lfHeight = (winAscent + winDescent) * ppem / units_per_em
     * Re-arranging gives:
     * ppem = units_per_em * lfheight / (winAscent + winDescent)
     *
     * For -ve lfHeight we have
     * |lfHeight| = ppem
     * [i.e. |lfHeight| = (winAscent + winDescent - il) * ppem / units_per_em
     * with il = winAscent + winDescent - units_per_em]
     *
     */

    if(height > 0) {
        USHORT windescent = get_fixed_windescent(pOS2->usWinDescent);
        int units;

        if(pOS2->usWinAscent + windescent == 0)
            units = pHori->Ascender - pHori->Descender;
        else
            units = pOS2->usWinAscent + windescent;
        ppem = pFT_MulDiv(ft_face->units_per_EM, height, units);

        /* If rounding ends up getting a font exceeding height, choose a smaller ppem */
        if(ppem > 1 && pFT_MulDiv(units, ppem, ft_face->units_per_EM) > height)
            --ppem;

        if(ppem > MAX_PPEM) {
            WARN("Ignoring too large height %d, ppem %d\n", height, ppem);
            ppem = 1;
        }
    }
    else if(height >= -MAX_PPEM)
        ppem = -height;
    else {
        WARN("Ignoring too large height %d\n", height);
        ppem = 1;
    }

    return ppem;
}

static struct font_mapping *map_font_file( const char *name )
{
    struct font_mapping *mapping;
    struct stat st;
    int fd;

    if ((fd = open( name, O_RDONLY )) == -1) return NULL;
    if (fstat( fd, &st ) == -1) goto error;

    LIST_FOR_EACH_ENTRY( mapping, &mappings_list, struct font_mapping, entry )
    {
        if (mapping->dev == st.st_dev && mapping->ino == st.st_ino)
        {
            mapping->refcount++;
            close( fd );
            return mapping;
        }
    }
    if (!(mapping = malloc( sizeof(*mapping) )))
        goto error;

    mapping->data = mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );

    if (mapping->data == MAP_FAILED)
    {
        free( mapping );
        return NULL;
    }
    mapping->refcount = 1;
    mapping->dev = st.st_dev;
    mapping->ino = st.st_ino;
    mapping->size = st.st_size;
    list_add_tail( &mappings_list, &mapping->entry );
    return mapping;

error:
    close( fd );
    return NULL;
}

static void unmap_font_file( struct font_mapping *mapping )
{
    if (!--mapping->refcount)
    {
        list_remove( &mapping->entry );
        munmap( mapping->data, mapping->size );
        free( mapping );
    }
}

static int load_VDMX(struct gdi_font *font, int height);

/*************************************************************
 * freetype_destroy_font
 */
static void freetype_destroy_font( struct gdi_font *font )
{
    struct font_private_data *data = font->private;

    if (data->ft_face) pFT_Done_Face( data->ft_face );
    if (data->mapping) unmap_font_file( data->mapping );
    free( data );
}

/*************************************************************
 * freetype_get_font_data
 */
static UINT freetype_get_font_data( struct gdi_font *font, UINT table, UINT offset,
                                     void *buf, UINT cbData)
{
    FT_Face ft_face = get_ft_face( font );
    FT_ULong len;
    FT_Error err;

    if (!FT_IS_SFNT(ft_face)) return GDI_ERROR;

    if(!buf)
        len = 0;
    else
        len = cbData;

    /* if font is a member of TTC, 'ttcf' tag allows reading from beginning of TTC file,
       0 tag means to read from start of collection member data. */
    if (font->ttc_item_offset)
    {
        if (table == MS_TTCF_TAG)
            table = 0;
        else if (table == 0)
            offset += font->ttc_item_offset;
    }

    /* make sure value of len is the value freetype says it needs */
    if (buf && len)
    {
        FT_ULong needed = 0;
        err = pFT_Load_Sfnt_Table(ft_face, RtlUlongByteSwap(table), offset, NULL, &needed);
        if( !err && needed < len) len = needed;
    }
    err = pFT_Load_Sfnt_Table(ft_face, RtlUlongByteSwap(table), offset, buf, &len);
    if (err)
    {
        TRACE("Can't find table %s\n", debugstr_fourcc(table));
	return GDI_ERROR;
    }
    return len;
}

/*************************************************************
 * load_VDMX
 *
 * load the vdmx entry for the specified height
 */



typedef struct {
    WORD version;
    WORD numRecs;
    WORD numRatios;
} VDMX_Header;

typedef struct {
    BYTE bCharSet;
    BYTE xRatio;
    BYTE yStartRatio;
    BYTE yEndRatio;
} Ratios;

typedef struct {
    WORD recs;
    BYTE startsz;
    BYTE endsz;
} VDMX_group;

typedef struct {
    WORD yPelHeight;
    WORD yMax;
    WORD yMin;
} VDMX_vTable;

static int load_VDMX(struct gdi_font *font, int height)
{
    VDMX_Header hdr;
    VDMX_group group;
    BYTE devXRatio, devYRatio;
    USHORT numRecs, numRatios;
    UINT result, offset = -1;
    int i, ppem = 0;

    result = freetype_get_font_data(font, MS_VDMX_TAG, 0, &hdr, sizeof(hdr));

    if(result == GDI_ERROR) /* no vdmx table present, use linear scaling */
	return ppem;

    /* FIXME: need the real device aspect ratio */
    devXRatio = 1;
    devYRatio = 1;

    numRecs = GET_BE_WORD(hdr.numRecs);
    numRatios = GET_BE_WORD(hdr.numRatios);

    TRACE("version = %d numRecs = %d numRatios = %d\n", GET_BE_WORD(hdr.version), numRecs, numRatios);
    for(i = 0; i < numRatios; i++) {
	Ratios ratio;

	offset = sizeof(hdr) + (i * sizeof(Ratios));
	freetype_get_font_data(font, MS_VDMX_TAG, offset, &ratio, sizeof(Ratios));
	offset = -1;

	TRACE("Ratios[%d] %d  %d : %d -> %d\n", i, ratio.bCharSet, ratio.xRatio, ratio.yStartRatio, ratio.yEndRatio);

        if (!ratio.bCharSet) continue;

	if((ratio.xRatio == 0 &&
	    ratio.yStartRatio == 0 &&
	    ratio.yEndRatio == 0) ||
	   (devXRatio == ratio.xRatio &&
	    devYRatio >= ratio.yStartRatio &&
	    devYRatio <= ratio.yEndRatio))
	    {
		WORD group_offset;

		offset = sizeof(hdr) + numRatios * sizeof(ratio) + i * sizeof(group_offset);
		freetype_get_font_data(font, MS_VDMX_TAG, offset, &group_offset, sizeof(group_offset));
		offset = GET_BE_WORD(group_offset);
		break;
	    }
    }

    if(offset == -1) return 0;

    if(freetype_get_font_data(font, MS_VDMX_TAG, offset, &group, sizeof(group)) != GDI_ERROR) {
	USHORT recs;
	BYTE startsz, endsz;
	VDMX_vTable *vTable;

	recs = GET_BE_WORD(group.recs);
	startsz = group.startsz;
	endsz = group.endsz;

	TRACE("recs=%d  startsz=%d  endsz=%d\n", recs, startsz, endsz);

	vTable = malloc( recs * sizeof(*vTable) );
	result = freetype_get_font_data(font, MS_VDMX_TAG, offset + sizeof(group), vTable, recs * sizeof(*vTable));
	if(result == GDI_ERROR) {
	    FIXME("Failed to retrieve vTable\n");
	    goto end;
	}

	if(height > 0) {
	    for(i = 0; i < recs; i++) {
                SHORT yMax = GET_BE_WORD(vTable[i].yMax);
                SHORT yMin = GET_BE_WORD(vTable[i].yMin);
                ppem = GET_BE_WORD(vTable[i].yPelHeight);

		if(yMax + -yMin == height) {
		    font->yMax = yMax;
		    font->yMin = yMin;
                    TRACE("ppem %d found; height=%d  yMax=%d  yMin=%d\n", ppem, height, font->yMax, font->yMin);
		    break;
		}
		if(yMax + -yMin > height) {
		    if(--i < 0) {
			ppem = 0;
			goto end; /* failed */
		    }
		    font->yMax = GET_BE_WORD(vTable[i].yMax);
		    font->yMin = GET_BE_WORD(vTable[i].yMin);
                    ppem = GET_BE_WORD(vTable[i].yPelHeight);
                    TRACE("ppem %d found; height=%d  yMax=%d  yMin=%d\n", ppem, height, font->yMax, font->yMin);
		    break;
		}
	    }
	    if(!font->yMax) {
		ppem = 0;
		TRACE("ppem not found for height %d\n", height);
	    }
	} else {
	    ppem = -height;
	    if(ppem < startsz || ppem > endsz)
            {
                ppem = 0;
                goto end;
            }

	    for(i = 0; i < recs; i++) {
		USHORT yPelHeight;
		yPelHeight = GET_BE_WORD(vTable[i].yPelHeight);

		if(yPelHeight > ppem)
                {
                    ppem = 0;
                    break; /* failed */
                }

		if(yPelHeight == ppem) {
		    font->yMax = GET_BE_WORD(vTable[i].yMax);
		    font->yMin = GET_BE_WORD(vTable[i].yMin);
                    TRACE("ppem %d found; yMax=%d  yMin=%d\n", ppem, font->yMax, font->yMin);
		    break;
		}
	    }
	}
	end:
	free( vTable );
    }

    return ppem;
}

static BOOL select_charmap(FT_Face ft_face, FT_Encoding encoding)
{
    FT_Error ft_err = FT_Err_Invalid_CharMap_Handle;
    FT_CharMap cmap0, cmap1, cmap2, cmap3, cmap_def;
    FT_Int i;

    cmap0 = cmap1 = cmap2 = cmap3 = cmap_def = NULL;

    for (i = 0; i < ft_face->num_charmaps; i++)
    {
        if (ft_face->charmaps[i]->encoding == encoding)
        {
            TRACE("found cmap with platform_id %u, encoding_id %u\n",
                   ft_face->charmaps[i]->platform_id, ft_face->charmaps[i]->encoding_id);

            switch (ft_face->charmaps[i]->platform_id)
            {
                default:
                    cmap_def = ft_face->charmaps[i];
                    break;
                case 0: /* Apple Unicode */
                    cmap0 = ft_face->charmaps[i];
                    break;
                case 1: /* Macintosh */
                    cmap1 = ft_face->charmaps[i];
                    break;
                case 2: /* ISO */
                    cmap2 = ft_face->charmaps[i];
                    break;
                case 3: /* Microsoft */
                    cmap3 = ft_face->charmaps[i];
                    break;
            }
        }

        if (cmap3) /* prefer Microsoft cmap table */
            ft_err = pFT_Set_Charmap(ft_face, cmap3);
        else if (cmap1)
            ft_err = pFT_Set_Charmap(ft_face, cmap1);
        else if (cmap2)
            ft_err = pFT_Set_Charmap(ft_face, cmap2);
        else if (cmap0)
            ft_err = pFT_Set_Charmap(ft_face, cmap0);
        else if (cmap_def)
            ft_err = pFT_Set_Charmap(ft_face, cmap_def);
    }

    return ft_err == FT_Err_Ok;
}


static FT_Encoding pick_charmap( FT_Face face, int charset )
{
    static const FT_Encoding regular_order[] = { FT_ENCODING_UNICODE, FT_ENCODING_APPLE_ROMAN, FT_ENCODING_MS_SYMBOL, 0 };
    static const FT_Encoding symbol_order[]  = { FT_ENCODING_MS_SYMBOL, FT_ENCODING_UNICODE, FT_ENCODING_APPLE_ROMAN, 0 };
    const FT_Encoding *encs = regular_order;

    if (charset == SYMBOL_CHARSET) encs = symbol_order;

    while (*encs != 0)
    {
        if (select_charmap( face, *encs )) break;
        encs++;
    }

    if (!face->charmap && face->num_charmaps)
    {
        if (!pFT_Set_Charmap(face, face->charmaps[0]))
            return face->charmap->encoding;
    }

    return *encs;
}

static BOOL get_gasp_flags( struct gdi_font *font, WORD *flags )
{
    FT_Face ft_face = get_ft_face( font );
    UINT size;
    WORD buf[16]; /* Enough for seven ranges before we need to alloc */
    WORD *alloced = NULL, *ptr = buf;
    WORD num_recs, version;
    BOOL ret = FALSE;

    *flags = 0;
    size = freetype_get_font_data( font, MS_GASP_TAG,  0, NULL, 0 );
    if (size == GDI_ERROR) return FALSE;
    if (size < 4 * sizeof(WORD)) return FALSE;
    if (size > sizeof(buf))
    {
        ptr = alloced = malloc( size );
        if (!ptr) return FALSE;
    }

    freetype_get_font_data( font, MS_GASP_TAG, 0, ptr, size );

    version  = GET_BE_WORD( *ptr++ );
    num_recs = GET_BE_WORD( *ptr++ );

    if (version > 1 || size < (num_recs * 2 + 2) * sizeof(WORD))
    {
        FIXME( "Unsupported gasp table: ver %d size %d recs %d\n", version, size, num_recs );
        goto done;
    }

    while (num_recs--)
    {
        *flags = GET_BE_WORD( *(ptr + 1) );
        if (ft_face->size->metrics.y_ppem <= GET_BE_WORD( *ptr )) break;
        ptr += 2;
    }
    TRACE( "got flags %04x for ppem %d\n", *flags, ft_face->size->metrics.y_ppem );
    ret = TRUE;

done:
    free( alloced );
    return ret;
}

/*************************************************************
 * fontconfig_enum_family_fallbacks
 */
static BOOL fontconfig_enum_family_fallbacks( UINT pitch_and_family, int index,
                                              WCHAR buffer[LF_FACESIZE] )
{
#ifdef SONAME_LIBFONTCONFIG
    FcPattern *pat;
    char *str;
    DWORD len;

    if ((pitch_and_family & FIXED_PITCH) || (pitch_and_family & 0xf0) == FF_MODERN) pat = create_family_pattern( "monospace", &pattern_fixed );
    else if ((pitch_and_family & 0xf0) == FF_ROMAN) pat = create_family_pattern( "serif", &pattern_serif );
    else pat = create_family_pattern( "sans", &pattern_sans );

    if (!pat) return FALSE;
    if (pFcPatternGetString( pat, FC_FAMILY, index, (FcChar8 **)&str ) != FcResultMatch) return FALSE;
    RtlUTF8ToUnicodeN( buffer, (LF_FACESIZE - 1) * sizeof(WCHAR), &len, str, strlen(str) );
    buffer[len / sizeof(WCHAR)] = 0;
    return TRUE;
#endif
    return FALSE;
}

static DWORD get_ttc_offset( FT_Face ft_face, UINT face_index )
{
    FT_ULong len;
    DWORD header, offset;

    /* see if it's a TTC */
    len = sizeof(header);
    if (pFT_Load_Sfnt_Table( ft_face, 0, 0, (void *)&header, &len )) return 0;
    if (header != MS_TTCF_TAG) return 0;

    len = sizeof(offset);
    if (pFT_Load_Sfnt_Table( ft_face, 0, (3 + face_index) * sizeof(DWORD), (void *)&offset, &len ))
        return 0;

    return GET_BE_DWORD( offset );
}

/*************************************************************
 * freetype_load_font
 */
static BOOL freetype_load_font( struct gdi_font *font )
{
    struct font_private_data *data;
    INT width = 0, height;
    FT_Face ft_face;
    void *data_ptr;
    SIZE_T data_size;

    if (!(data = calloc( 1, sizeof(*data) ))) return FALSE;
    font->private = data;

    if (font->file[0])
    {
        char *filename;
        if (!ntdll_get_unix_file_name( font->file, &filename, FILE_OPEN ))
        {
            data->mapping = map_font_file( filename );
            free( filename );
        }
        if (!data->mapping)
        {
            WARN("failed to map %s\n", debugstr_w(font->file));
            return FALSE;
        }
        data_ptr = data->mapping->data;
        data_size = data->mapping->size;
    }
    else
    {
        data_ptr = font->data_ptr;
        data_size = font->data_size;
    }

    if (pFT_New_Memory_Face( library, data_ptr, data_size, font->face_index, &ft_face )) return FALSE;

    data->ft_face = ft_face;
    font->scalable = FT_IS_SCALABLE( ft_face );
    if (!font->fs.fsCsb[0]) get_fontsig( ft_face, &font->fs );
    if (!font->ntmFlags) font->ntmFlags = get_ntm_flags( ft_face );
    if (!font->aa_flags) font->aa_flags = ADDFONT_AA_FLAGS( default_aa_flags );
    if (!font->otm.otmpFamilyName)
    {
        font->otm.otmpFamilyName = (char *)ft_face_get_family_name( ft_face, system_lcid );
        font->otm.otmpStyleName = (char *)ft_face_get_style_name( ft_face, system_lcid );
        font->otm.otmpFaceName = (char *)ft_face_get_full_name( ft_face, system_lcid );
    }

    if (font->scalable)
    {
        /* load the VDMX table if we have one */
        font->ppem = load_VDMX( font, font->lf.lfHeight );
        if (font->ppem == 0) font->ppem = calc_ppem_for_height( ft_face, font->lf.lfHeight );
        TRACE( "height %d => ppem %d\n", font->lf.lfHeight, font->ppem );
        height = font->ppem;
        font->ttc_item_offset = get_ttc_offset( ft_face, font->face_index );
        font->otm.otmEMSquare = ft_face->units_per_EM;
    }
    else
    {
        struct bitmap_font_size size;

        get_bitmap_size( ft_face, &size );
        width = size.x_ppem >> 6;
        height = size.y_ppem >> 6;
        font->ppem = height;
    }

    pFT_Set_Pixel_Sizes( ft_face, width, height );
    pick_charmap( ft_face, font->charset );
    return TRUE;
}


/*************************************************************
 * freetype_get_aa_flags
 */
static UINT freetype_get_aa_flags( struct gdi_font *font, UINT aa_flags, BOOL antialias_fakes )
{
    /* fixup the antialiasing flags for that font */
    switch (aa_flags)
    {
    case WINE_GGO_HRGB_BITMAP:
    case WINE_GGO_HBGR_BITMAP:
    case WINE_GGO_VRGB_BITMAP:
    case WINE_GGO_VBGR_BITMAP:
        if (is_subpixel_rendering_enabled()) break;
        aa_flags = GGO_GRAY4_BITMAP;
        /* fall through */
    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    case WINE_GGO_GRAY16_BITMAP:
        if ((!antialias_fakes || (!font->fake_bold && !font->fake_italic)) && is_hinting_enabled())
        {
            WORD gasp_flags;
            if (get_gasp_flags( font, &gasp_flags ) && !(gasp_flags & GASP_DOGRAY))
            {
                TRACE( "font %s %d aa disabled by GASP\n",
                       debugstr_w(font->lf.lfFaceName), font->lf.lfHeight );
                aa_flags = GGO_BITMAP;
            }
        }
    }
    return aa_flags;
}

static void FTVectorToPOINTFX(FT_Vector *vec, POINTFX *pt)
{
    pt->x.value = vec->x >> 6;
    pt->x.fract = (vec->x & 0x3f) << 10;
    pt->x.fract |= ((pt->x.fract >> 6) | (pt->x.fract >> 12));
    pt->y.value = vec->y >> 6;
    pt->y.fract = (vec->y & 0x3f) << 10;
    pt->y.fract |= ((pt->y.fract >> 6) | (pt->y.fract >> 12));
}

static FT_UInt get_glyph_index_symbol( struct gdi_font *font, UINT glyph )
{
    FT_Face ft_face = get_ft_face( font );
    FT_UInt ret;

    if (glyph < 0x100) glyph += 0xf000;
    /* there are a number of old pre-Unicode "broken" TTFs, which
       do have symbols at U+00XX instead of U+f0XX */
    if (!(ret = pFT_Get_Char_Index(ft_face, glyph)))
        ret = pFT_Get_Char_Index(ft_face, glyph - 0xf000);

    return ret;
}

/*************************************************************
 * freetype_get_glyph_index
 */
static BOOL freetype_get_glyph_index( struct gdi_font *font, UINT *glyph, BOOL use_encoding )
{
    FT_Face ft_face = get_ft_face( font );

    if (!use_encoding ^ (ft_face->charmap->encoding == FT_ENCODING_NONE)) return FALSE;

    if (ft_face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
    {
        if (!(*glyph = get_glyph_index_symbol( font, *glyph )))
        {
            WCHAR wc = *glyph;
            DWORD len;
            char ch;

            len = win32u_wctomb( &ansi_cp, &ch, 1, &wc, 1 );
            if (len) *glyph = get_glyph_index_symbol( font, (unsigned char)ch );
        }
        return TRUE;
    }
    *glyph = pFT_Get_Char_Index( ft_face, *glyph );
    return TRUE;
}

/*************************************************************
 * freetype_get_default_glyph
 */
static UINT freetype_get_default_glyph( struct gdi_font *font )
{
    FT_Face ft_face = get_ft_face( font );
    FT_WinFNT_HeaderRec winfnt;
    TT_OS2 *pOS2;

    if ((pOS2 = pFT_Get_Sfnt_Table( ft_face, ft_sfnt_os2 )))
    {
        UINT glyph = pOS2->usDefaultChar;
        if (glyph) freetype_get_glyph_index( font, &glyph, TRUE );
        return glyph;
    }
    if (!pFT_Get_WinFNT_Header( ft_face, &winfnt )) return winfnt.default_char + winfnt.first_char;
    return 32;
}


static inline BOOL is_identity_FMAT2(const FMAT2 *matrix)
{
    static const FMAT2 identity = { 1.0, 0.0, 0.0, 1.0 };
    return !memcmp(matrix, &identity, sizeof(FMAT2));
}

static inline FT_Vector normalize_vector(FT_Vector *vec)
{
    FT_Vector out;
    FT_Fixed len;
    len = pFT_Vector_Length(vec);
    if (len) {
        out.x = (vec->x << 6) / len;
        out.y = (vec->y << 6) / len;
    }
    else
        out.x = out.y = 0;
    return out;
}

/* get_glyph_outline() glyph transform matrices index */
enum matrices_index
{
    matrix_hori,
    matrix_vert,
    matrix_unrotated
};

static FT_Matrix *get_transform_matrices( struct gdi_font *font, BOOL vertical, const MAT2 *user_transform,
                                          FT_Matrix matrices[3] )
{
    static const FT_Matrix identity_mat = { (1 << 16), 0, 0, (1 << 16) };
    BOOL needs_transform = FALSE;
    double width_ratio;
    int i;

    matrices[matrix_unrotated] = identity_mat;

    /* Scaling factor */
    if (font->aveWidth)
    {
        if (!freetype_set_outline_text_metrics( font )) freetype_set_bitmap_text_metrics( font );
        width_ratio = (double)font->aveWidth;
        width_ratio /= (double)font->otm.otmTextMetrics.tmAveCharWidth;
    }
    else
        width_ratio = font->scale_y;

    /* Scaling transform */
    if (width_ratio != 1.0 || font->scale_y != 1)
    {
        FT_Matrix scale_mat;
        scale_mat.xx = FT_FixedFromFloat( width_ratio );
        scale_mat.xy = 0;
        scale_mat.yx = 0;
        scale_mat.yy = font->scale_y << 16;

        pFT_Matrix_Multiply( &scale_mat, &matrices[matrix_unrotated] );
        needs_transform = TRUE;
    }

    /* Slant transform */
    if (font->fake_italic)
    {
        FT_Matrix slant_mat;
        slant_mat.xx = (1 << 16);
        slant_mat.xy = (1 << 16) >> 2;
        slant_mat.yx = 0;
        slant_mat.yy = (1 << 16);

        pFT_Matrix_Multiply( &slant_mat, &matrices[matrix_unrotated] );
        needs_transform = TRUE;
    }

    /* Rotation transform */
    matrices[matrix_hori] = matrices[matrix_unrotated];
    if (font->scalable && font->lf.lfOrientation % 3600)
    {
        FT_Matrix rotation_mat;
        FT_Vector angle;

        pFT_Vector_Unit( &angle, pFT_MulDiv( 1 << 16, font->lf.lfOrientation, 10 ) );
        rotation_mat.xx =  angle.x;
        rotation_mat.xy = -angle.y;
        rotation_mat.yx =  angle.y;
        rotation_mat.yy =  angle.x;
        pFT_Matrix_Multiply( &rotation_mat, &matrices[matrix_hori] );
        needs_transform = TRUE;
    }

    /* Vertical transform */
    matrices[matrix_vert] = matrices[matrix_hori];
    if (vertical)
    {
        FT_Matrix vertical_mat = { 0, -(1 << 16), 1 << 16, 0 }; /* 90 degrees rotation */

        pFT_Matrix_Multiply( &vertical_mat, &matrices[matrix_vert] );
        needs_transform = TRUE;
    }

    /* World transform */
    if (!is_identity_FMAT2( &font->matrix ))
    {
        FT_Matrix world_mat;
        world_mat.xx =  FT_FixedFromFloat( font->matrix.eM11 );
        world_mat.xy = -FT_FixedFromFloat( font->matrix.eM21 );
        world_mat.yx = -FT_FixedFromFloat( font->matrix.eM12 );
        world_mat.yy =  FT_FixedFromFloat( font->matrix.eM22 );

        for (i = 0; i < 3; i++)
            pFT_Matrix_Multiply( &world_mat, &matrices[i] );
        needs_transform = TRUE;
    }

    /* Extra transformation specified by caller */
    if (user_transform)
    {
        FT_Matrix user_mat;
        user_mat.xx = FT_FixedFromFIXED( user_transform->eM11 );
        user_mat.xy = FT_FixedFromFIXED( user_transform->eM21 );
        user_mat.yx = FT_FixedFromFIXED( user_transform->eM12 );
        user_mat.yy = FT_FixedFromFIXED( user_transform->eM22 );

        for (i = 0; i < 3; i++)
            pFT_Matrix_Multiply( &user_mat, &matrices[i] );
        needs_transform = TRUE;
    }

    return needs_transform ? matrices : NULL;
}

static BOOL get_bold_glyph_outline(FT_GlyphSlot glyph, LONG ppem, FT_Glyph_Metrics *metrics)
{
    FT_Error err;
    FT_Pos strength;
    FT_BBox bbox;

    if(glyph->format != FT_GLYPH_FORMAT_OUTLINE)
        return FALSE;

    strength = pFT_MulDiv(ppem, 1 << 6, 24);
    err = pFT_Outline_Embolden(&glyph->outline, strength);
    if(err) {
        TRACE("FT_Ouline_Embolden returns %d\n", err);
        return FALSE;
    }

    pFT_Outline_Get_CBox(&glyph->outline, &bbox);
    metrics->width = bbox.xMax - bbox.xMin;
    metrics->height = bbox.yMax - bbox.yMin;
    metrics->horiBearingX = bbox.xMin;
    metrics->horiBearingY = bbox.yMax;
    metrics->vertBearingX = metrics->horiBearingX - metrics->horiAdvance / 2;
    metrics->vertBearingY = (metrics->vertAdvance - metrics->height) / 2;
    return TRUE;
}

static inline BYTE get_max_level( UINT format )
{
    switch( format )
    {
    case GGO_GRAY2_BITMAP: return 4;
    case GGO_GRAY4_BITMAP: return 16;
    case GGO_GRAY8_BITMAP: return 64;
    }
    return 255;
}

static FT_Vector get_advance_metric( struct gdi_font *font, FT_Pos base_advance,
                                     const FT_Matrix *transMat )
{
    FT_Vector adv;
    FT_Fixed em_scale = 0;
    BOOL fixed_pitch_full = FALSE;
    struct gdi_font *incoming_font = font->base_font ? font->base_font : font;

    adv.x = base_advance;
    adv.y = 0;

    /* In fixed-pitch font, we adjust the fullwidth character advance so that
       they have double halfwidth character width. E.g. if the font is 19 ppem,
       we return 20 (not 19) for fullwidth characters as we return 10 for
       halfwidth characters. */
    if (freetype_set_outline_text_metrics(incoming_font) &&
        !(incoming_font->otm.otmTextMetrics.tmPitchAndFamily & TMPF_FIXED_PITCH)) {
        UINT avg_advance;
        em_scale = pFT_MulDiv(incoming_font->ppem, 1 << 16, get_ft_face(incoming_font)->units_per_EM);
        avg_advance = pFT_MulFix(incoming_font->ntmAvgWidth, em_scale);
        fixed_pitch_full = (avg_advance > 0 &&
                            (base_advance + 63) >> 6 ==
                            pFT_MulFix(incoming_font->ntmAvgWidth*2, em_scale));
        if (fixed_pitch_full && !transMat)
            adv.x = (avg_advance * 2) << 6;
    }

    if (transMat) {
        pFT_Vector_Transform(&adv, transMat);
        if (fixed_pitch_full && adv.y == 0) {
            FT_Vector vec;
            vec.x = incoming_font->ntmAvgWidth;
            vec.y = 0;
            pFT_Vector_Transform(&vec, transMat);
            adv.x = (pFT_MulFix(vec.x, em_scale) * 2) << 6;
        }
    }

    if (font->fake_bold) {
        if (!transMat)
            adv.x += 1 << 6;
        else {
            FT_Vector fake_bold_adv, vec = { 1 << 6, 0 };
            pFT_Vector_Transform(&vec, transMat);
            fake_bold_adv = normalize_vector(&vec);
            adv.x += fake_bold_adv.x;
            adv.y += fake_bold_adv.y;
        }
    }

    adv.x = (adv.x + 63) & -64;
    adv.y = -((adv.y + 63) & -64);
    return adv;
}

static FT_BBox get_transformed_bbox( const FT_Glyph_Metrics *metrics, const FT_Matrix *matrices )
{
    FT_BBox bbox = { 0, 0, 0, 0 };

    if (!matrices)
    {
        bbox.xMin = (metrics->horiBearingX) & -64;
        bbox.xMax = (metrics->horiBearingX + metrics->width + 63) & -64;
        bbox.yMax = (metrics->horiBearingY + 63) & -64;
        bbox.yMin = (metrics->horiBearingY - metrics->height) & -64;
    }
    else
    {
        FT_Vector vec;
        INT xc, yc;

        for (xc = 0; xc < 2; xc++)
        {
            for (yc = 0; yc < 2; yc++)
            {
                vec.x = metrics->horiBearingX + xc * metrics->width;
                vec.y = metrics->horiBearingY - yc * metrics->height;
                TRACE( "Vec %ld, %ld\n", vec.x, vec.y );
                pFT_Vector_Transform( &vec, &matrices[matrix_vert] );
                if (xc == 0 && yc == 0)
                {
                    bbox.xMin = bbox.xMax = vec.x;
                    bbox.yMin = bbox.yMax = vec.y;
                }
                else
                {
                    if      (vec.x < bbox.xMin) bbox.xMin = vec.x;
                    else if (vec.x > bbox.xMax) bbox.xMax = vec.x;
                    if      (vec.y < bbox.yMin) bbox.yMin = vec.y;
                    else if (vec.y > bbox.yMax) bbox.yMax = vec.y;
                }
            }
        }
        bbox.xMin = bbox.xMin & -64;
        bbox.xMax = (bbox.xMax + 63) & -64;
        bbox.yMin = bbox.yMin & -64;
        bbox.yMax = (bbox.yMax + 63) & -64;
        TRACE( "transformed box: (%ld, %ld - %ld, %ld)\n", bbox.xMin, bbox.yMax, bbox.xMax, bbox.yMin );
    }

    return bbox;
}

static void compute_metrics( struct gdi_font *font, FT_BBox bbox, const FT_Glyph_Metrics *metrics,
                             BOOL vertical, BOOL vertical_metrics, const FT_Matrix *matrices,
                             GLYPHMETRICS *gm, ABC *abc )
{
    FT_Vector adv, vec, origin;
    FT_Fixed base_advance = vertical_metrics ? metrics->vertAdvance : metrics->horiAdvance;

    if (!matrices)
    {
        adv = get_advance_metric( font, base_advance, NULL );
        gm->gmCellIncX = adv.x >> 6;
        gm->gmCellIncY = 0;
        origin.x = bbox.xMin;
        origin.y = bbox.yMax;
        abc->abcA = origin.x >> 6;
        abc->abcB = (metrics->width + 63) >> 6;
    }
    else
    {
        FT_Pos lsb;

        if (vertical && freetype_set_outline_text_metrics( font ))
        {
            if (vertical_metrics)
                lsb = metrics->horiBearingY + metrics->vertBearingY;
            else
                lsb = metrics->vertAdvance + (font->otm.otmDescent << 6);
            vec.x = lsb;
            vec.y = font->otm.otmDescent << 6;
            TRACE( "Vec %ld,%ld\n", vec.x>>6, vec.y>>6 );
            pFT_Vector_Transform( &vec, &matrices[matrix_hori] );
            origin.x = (vec.x + bbox.xMin) & -64;
            origin.y = (vec.y + bbox.yMax + 63) & -64;
            lsb -= metrics->horiBearingY;
        }
        else
        {
            origin.x = bbox.xMin;
            origin.y = bbox.yMax;
            lsb = metrics->horiBearingX;
        }

        adv = get_advance_metric( font, base_advance, &matrices[matrix_hori] );
        gm->gmCellIncX = adv.x >> 6;
        gm->gmCellIncY = adv.y >> 6;

        adv = get_advance_metric( font, base_advance, &matrices[matrix_unrotated] );
        adv.x = pFT_Vector_Length( &adv );
        adv.y = 0;

        vec.x = lsb;
        vec.y = 0;
        pFT_Vector_Transform( &vec, &matrices[matrix_unrotated] );
        if (lsb > 0) abc->abcA = pFT_Vector_Length( &vec ) >> 6;
        else abc->abcA = -((pFT_Vector_Length( &vec ) + 63) >> 6);

        /* We use lsb again to avoid rounding errors */
        vec.x = lsb + (vertical ? metrics->height : metrics->width);
        vec.y = 0;
        pFT_Vector_Transform( &vec, &matrices[matrix_unrotated] );
        abc->abcB = ((pFT_Vector_Length( &vec ) + 63) >> 6) - abc->abcA;
    }
    if (!abc->abcB) abc->abcB = 1;
    abc->abcC = (adv.x >> 6) - abc->abcA - abc->abcB;

    gm->gmptGlyphOrigin.x = origin.x >> 6;
    gm->gmptGlyphOrigin.y = origin.y >> 6;
    gm->gmBlackBoxX = (bbox.xMax - bbox.xMin) >> 6;
    gm->gmBlackBoxY = (bbox.yMax - bbox.yMin) >> 6;
    if (!gm->gmBlackBoxX) gm->gmBlackBoxX = 1;
    if (!gm->gmBlackBoxY) gm->gmBlackBoxY = 1;

    TRACE( "gm: %u, %u, %s, %d, %d abc %d, %u, %d\n",
           gm->gmBlackBoxX, gm->gmBlackBoxY, wine_dbgstr_point(&gm->gmptGlyphOrigin),
           gm->gmCellIncX, gm->gmCellIncY, abc->abcA, abc->abcB, abc->abcC );
}


static const BYTE masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

static DWORD get_mono_glyph_bitmap( FT_GlyphSlot glyph, FT_BBox bbox,
                                    BOOL fake_bold, const FT_Matrix *matrices,
                                    DWORD buflen, BYTE *buf )
{
    DWORD width  = (bbox.xMax - bbox.xMin ) >> 6;
    DWORD height = (bbox.yMax - bbox.yMin ) >> 6;
    DWORD pitch  = ((width + 31) >> 5) << 2;
    DWORD needed = pitch * height;
    FT_Bitmap ft_bitmap;
    BYTE *src, *dst;
    INT w, h, x;

    if (!buf || !buflen) return needed;
    if (!needed) return GDI_ERROR;  /* empty glyph */
    if (needed > buflen) return GDI_ERROR;

    switch (glyph->format)
    {
    case FT_GLYPH_FORMAT_BITMAP:
        src = glyph->bitmap.buffer;
        dst = buf;
        w = min( pitch, (glyph->bitmap.width + 7) >> 3 );
        h = min( height, glyph->bitmap.rows );
        while (h--)
        {
            if (!fake_bold)
                memcpy( dst, src, w );
            else
            {
                dst[0] = 0;
                for (x = 0; x < w; x++)
                {
                    dst[x] = (dst[x] & 0x80) | (src[x] >> 1) | src[x];
                    if (x + 1 < pitch)
                        dst[x + 1] = (src[x] & 0x01) << 7;
                }
            }
            src += glyph->bitmap.pitch;
            dst += pitch;
        }
        break;

    case FT_GLYPH_FORMAT_OUTLINE:
        ft_bitmap.width = width;
        ft_bitmap.rows = height;
        ft_bitmap.pitch = pitch;
        ft_bitmap.pixel_mode = FT_PIXEL_MODE_MONO;
        ft_bitmap.buffer = buf;

        if (matrices)
            pFT_Outline_Transform( &glyph->outline, &matrices[matrix_vert] );
        pFT_Outline_Translate( &glyph->outline, -bbox.xMin, -bbox.yMin );

        /* Note: FreeType will only set 'black' bits for us. */
        memset( buf, 0, buflen );
        pFT_Outline_Get_Bitmap( library, &glyph->outline, &ft_bitmap );
        break;

    default:
        FIXME( "loaded glyph format %x\n", glyph->format );
        return GDI_ERROR;
    }

    return needed;
}

static DWORD get_antialias_glyph_bitmap( FT_GlyphSlot glyph, FT_BBox bbox, UINT format,
                                         BOOL fake_bold, const FT_Matrix *matrices,
                                         DWORD buflen, BYTE *buf )
{
    DWORD width  = (bbox.xMax - bbox.xMin ) >> 6;
    DWORD height = (bbox.yMax - bbox.yMin ) >> 6;
    DWORD pitch  = (width + 3) / 4 * 4;
    DWORD needed = pitch * height;
    FT_Bitmap ft_bitmap;
    INT w, h, x, max_level;
    BYTE *src, *dst;

    if (!buf || !buflen) return needed;
    if (!needed) return GDI_ERROR;  /* empty glyph */
    if (needed > buflen) return GDI_ERROR;

    max_level = get_max_level( format );

    switch (glyph->format)
    {
    case FT_GLYPH_FORMAT_BITMAP:
        src = glyph->bitmap.buffer;
        dst = buf;
        memset( buf, 0, buflen );

        w = min( pitch, glyph->bitmap.width );
        h = min( height, glyph->bitmap.rows );
        while (h--)
        {
            for (x = 0; x < w; x++)
            {
                if (src[x / 8] & masks[x % 8])
                {
                    dst[x] = max_level;
                    if (fake_bold && x + 1 < pitch) dst[x + 1] = max_level;
                }
            }
            src += glyph->bitmap.pitch;
            dst += pitch;
        }
        break;

    case FT_GLYPH_FORMAT_OUTLINE:
        ft_bitmap.width = width;
        ft_bitmap.rows = height;
        ft_bitmap.pitch = pitch;
        ft_bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
        ft_bitmap.buffer = buf;

        if (matrices)
            pFT_Outline_Transform( &glyph->outline, &matrices[matrix_vert] );
        pFT_Outline_Translate( &glyph->outline, -bbox.xMin, -bbox.yMin );

        memset( buf, 0, buflen );
        pFT_Outline_Get_Bitmap( library, &glyph->outline, &ft_bitmap );

        if (max_level != 255)
        {
            INT row, col;
            BYTE *ptr, *start;

            for (row = 0, start = buf; row < height; row++)
            {
                for (col = 0, ptr = start; col < width; col++, ptr++)
                    *ptr = (((int)*ptr) * (max_level + 1)) / 256;
                start += pitch;
            }
        }
        break;

    default:
        FIXME("loaded glyph format %x\n", glyph->format);
        return GDI_ERROR;
    }

    return needed;
}

static DWORD get_subpixel_glyph_bitmap( FT_GlyphSlot glyph, FT_BBox bbox, UINT format,
                                        BOOL fake_bold, const FT_Matrix *matrices,
                                        GLYPHMETRICS *gm, DWORD buflen, BYTE *buf )
{
    DWORD width  = (bbox.xMax - bbox.xMin ) >> 6;
    DWORD height = (bbox.yMax - bbox.yMin ) >> 6;
    DWORD pitch, needed = 0;
    BYTE *src, *dst;
    INT  w, h, x;

    switch (glyph->format)
    {
    case FT_GLYPH_FORMAT_BITMAP:
        pitch  = width * 4;
        needed = pitch * height;

        if (!buf || !buflen) break;
        if (!needed) return GDI_ERROR;  /* empty glyph */
        if (needed > buflen) return GDI_ERROR;

        src = glyph->bitmap.buffer;
        dst = buf;
        memset( buf, 0, buflen );

        w = min( width, glyph->bitmap.width );
        h = min( height, glyph->bitmap.rows );
        while (h--)
        {
            for (x = 0; x < w; x++)
            {
                if ( src[x / 8] & masks[x % 8] )
                {
                    ((unsigned int *)dst)[x] = ~0u;
                    if (fake_bold && x + 1 < width) ((unsigned int *)dst)[x + 1] = ~0u;
                }
            }
            src += glyph->bitmap.pitch;
            dst += pitch;
        }
        break;

    case FT_GLYPH_FORMAT_OUTLINE:
      {
        INT src_pitch, src_width, src_height, x_shift, y_shift;
        INT sub_stride, hmul, vmul;
        const INT *sub_order;
        const INT rgb_order[3] = { 0, 1, 2 };
        const INT bgr_order[3] = { 2, 1, 0 };
        FT_Render_Mode render_mode =
            (format == WINE_GGO_HRGB_BITMAP ||
             format == WINE_GGO_HBGR_BITMAP) ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_LCD_V;

        if (!width || !height) /* empty glyph */
        {
            if (!buf || !buflen) break;
            return GDI_ERROR;
        }

        if ( render_mode == FT_RENDER_MODE_LCD)
        {
            gm->gmBlackBoxX += 2;
            gm->gmptGlyphOrigin.x -= 1;
            bbox.xMin -= (1 << 6);
        }
        else
        {
            gm->gmBlackBoxY += 2;
            gm->gmptGlyphOrigin.y += 1;
            bbox.yMax += (1 << 6);
        }

        width  = gm->gmBlackBoxX;
        height = gm->gmBlackBoxY;
        pitch  = width * 4;
        needed = pitch * height;

        if (!buf || !buflen) return needed;
        if (needed > buflen) return GDI_ERROR;

        if (matrices)
            pFT_Outline_Transform( &glyph->outline, &matrices[matrix_vert] );

        pFT_Render_Glyph( glyph, render_mode );

        src_pitch = glyph->bitmap.pitch;
        src_width = glyph->bitmap.width;
        src_height = glyph->bitmap.rows;
        src = glyph->bitmap.buffer;
        dst = buf;
        memset( buf, 0, buflen );

        sub_order  = (format == WINE_GGO_HRGB_BITMAP ||
                      format == WINE_GGO_VRGB_BITMAP) ? rgb_order : bgr_order;
        sub_stride = render_mode == FT_RENDER_MODE_LCD ? 1 : src_pitch;
        hmul       = render_mode == FT_RENDER_MODE_LCD ? 3 : 1;
        vmul       = render_mode == FT_RENDER_MODE_LCD ? 1 : 3;

        x_shift = glyph->bitmap_left - (bbox.xMin >> 6);
        if ( x_shift < 0 )
        {
            src += hmul * -x_shift;
            src_width -= hmul * -x_shift;
        }
        else if ( x_shift > 0 )
        {
            dst += x_shift * sizeof(unsigned int);
            width -= x_shift;
        }

        y_shift = (bbox.yMax >> 6) - glyph->bitmap_top;
        if ( y_shift < 0 )
        {
            src += src_pitch * vmul * -y_shift;
            src_height -= vmul * -y_shift;
        }
        else if ( y_shift > 0 )
        {
            dst += y_shift * pitch;
            height -= y_shift;
        }

        w = min( width, src_width / hmul );
        h = min( height, src_height / vmul );
        while (h--)
        {
            for (x = 0; x < w; x++)
            {
                ((unsigned int *)dst)[x] =
                    ((unsigned int)src[hmul * x + sub_stride * sub_order[0]] << 16) |
                    ((unsigned int)src[hmul * x + sub_stride * sub_order[1]] << 8) |
                    ((unsigned int)src[hmul * x + sub_stride * sub_order[2]]);
            }
            src += src_pitch * vmul;
            dst += pitch;
        }
        break;
      }
    default:
        FIXME ( "loaded glyph format %x\n", glyph->format );
        return GDI_ERROR;
    }

    return needed;
}

static unsigned int get_native_glyph_outline(FT_Outline *outline, unsigned int buflen, char *buf)
{
    TTPOLYGONHEADER *pph;
    TTPOLYCURVE *ppc;
    unsigned int needed = 0, point = 0, contour, first_pt;
    unsigned int pph_start, cpfx;
    DWORD type;

    for (contour = 0; contour < outline->n_contours; contour++)
    {
        /* Ignore contours containing one point */
        if (point == outline->contours[contour])
        {
            point++;
            continue;
        }

        pph_start = needed;
        pph = (TTPOLYGONHEADER *)(buf + needed);
        first_pt = point;
        if (buf)
        {
            pph->dwType = TT_POLYGON_TYPE;
            FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
        }
        needed += sizeof(*pph);
        point++;
        while (point <= outline->contours[contour])
        {
            ppc = (TTPOLYCURVE *)(buf + needed);
            type = outline->tags[point] & FT_Curve_Tag_On ?
                TT_PRIM_LINE : TT_PRIM_QSPLINE;
            cpfx = 0;
            do
            {
                if (buf)
                    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                cpfx++;
                point++;
            } while (point <= outline->contours[contour] &&
                    (outline->tags[point] & FT_Curve_Tag_On) ==
                    (outline->tags[point-1] & FT_Curve_Tag_On));
            /* At the end of a contour Windows adds the start point, but
               only for Beziers */
            if (point > outline->contours[contour] &&
               !(outline->tags[point-1] & FT_Curve_Tag_On))
            {
                if (buf)
                    FTVectorToPOINTFX(&outline->points[first_pt], &ppc->apfx[cpfx]);
                cpfx++;
            }
            else if (point <= outline->contours[contour] &&
                      outline->tags[point] & FT_Curve_Tag_On)
            {
                /* add closing pt for bezier */
                if (buf)
                    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                cpfx++;
                point++;
            }
            if (buf)
            {
                ppc->wType = type;
                ppc->cpfx = cpfx;
            }
            needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
        }
        if (buf)
            pph->cb = needed - pph_start;
    }
    return needed;
}

static unsigned int get_bezier_glyph_outline(FT_Outline *outline, unsigned int buflen, char *buf)
{
    /* Convert the quadratic Beziers to cubic Beziers.
       The parametric eqn for a cubic Bezier is, from PLRM:
       r(t) = at^3 + bt^2 + ct + r0
       with the control points:
       r1 = r0 + c/3
       r2 = r1 + (c + b)/3
       r3 = r0 + c + b + a

       A quadratic Bezier has the form:
       p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

       So equating powers of t leads to:
       r1 = 2/3 p1 + 1/3 p0
       r2 = 2/3 p1 + 1/3 p2
       and of course r0 = p0, r3 = p2
    */
    int contour, point = 0, first_pt;
    TTPOLYGONHEADER *pph;
    TTPOLYCURVE *ppc;
    DWORD pph_start, cpfx, type;
    FT_Vector cubic_control[4];
    unsigned int needed = 0;

    for (contour = 0; contour < outline->n_contours; contour++)
    {
        pph_start = needed;
        pph = (TTPOLYGONHEADER *)(buf + needed);
        first_pt = point;
        if (buf)
        {
            pph->dwType = TT_POLYGON_TYPE;
            FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
        }
        needed += sizeof(*pph);
        point++;
        while (point <= outline->contours[contour])
        {
            ppc = (TTPOLYCURVE *)(buf + needed);
            type = outline->tags[point] & FT_Curve_Tag_On ?
                TT_PRIM_LINE : TT_PRIM_CSPLINE;
            cpfx = 0;
            do
            {
                if (type == TT_PRIM_LINE)
                {
                    if (buf)
                        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                    cpfx++;
                    point++;
                }
                else
                {
                    /* Unlike QSPLINEs, CSPLINEs always have their endpoint
                       so cpfx = 3n */

                    /* FIXME: Possible optimization in endpoint calculation
                       if there are two consecutive curves */
                    cubic_control[0] = outline->points[point-1];
                    if (!(outline->tags[point-1] & FT_Curve_Tag_On))
                    {
                        cubic_control[0].x += outline->points[point].x + 1;
                        cubic_control[0].y += outline->points[point].y + 1;
                        cubic_control[0].x >>= 1;
                        cubic_control[0].y >>= 1;
                    }
                    if (point+1 > outline->contours[contour])
                        cubic_control[3] = outline->points[first_pt];
                    else
                    {
                        cubic_control[3] = outline->points[point+1];
                        if (!(outline->tags[point+1] & FT_Curve_Tag_On))
                        {
                            cubic_control[3].x += outline->points[point].x + 1;
                            cubic_control[3].y += outline->points[point].y + 1;
                            cubic_control[3].x >>= 1;
                            cubic_control[3].y >>= 1;
                        }
                    }
                    /* r1 = 1/3 p0 + 2/3 p1
                       r2 = 1/3 p2 + 2/3 p1 */
                    cubic_control[1].x = (2 * outline->points[point].x + 1) / 3;
                    cubic_control[1].y = (2 * outline->points[point].y + 1) / 3;
                    cubic_control[2] = cubic_control[1];
                    cubic_control[1].x += (cubic_control[0].x + 1) / 3;
                    cubic_control[1].y += (cubic_control[0].y + 1) / 3;
                    cubic_control[2].x += (cubic_control[3].x + 1) / 3;
                    cubic_control[2].y += (cubic_control[3].y + 1) / 3;
                    if (buf)
                    {
                        FTVectorToPOINTFX(&cubic_control[1], &ppc->apfx[cpfx]);
                        FTVectorToPOINTFX(&cubic_control[2], &ppc->apfx[cpfx+1]);
                        FTVectorToPOINTFX(&cubic_control[3], &ppc->apfx[cpfx+2]);
                    }
                    cpfx += 3;
                    point++;
                }
            } while (point <= outline->contours[contour] &&
                    (outline->tags[point] & FT_Curve_Tag_On) ==
                    (outline->tags[point-1] & FT_Curve_Tag_On));
            /* At the end of a contour Windows adds the start point,
               but only for Beziers and we've already done that.
            */
            if (point <= outline->contours[contour] &&
               outline->tags[point] & FT_Curve_Tag_On)
            {
                /* This is the closing pt of a bezier, but we've already
                   added it, so just inc point and carry on */
                point++;
            }
            if (buf)
            {
                ppc->wType = type;
                ppc->cpfx = cpfx;
            }
            needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
        }
        if (buf)
            pph->cb = needed - pph_start;
    }
    return needed;
}

static FT_Int get_load_flags( UINT format )
{
    FT_Int load_flags = FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;

    if (format & GGO_UNHINTED)
        return load_flags | FT_LOAD_NO_HINTING;

    switch (format & ~GGO_GLYPH_INDEX)
    {
    case GGO_BITMAP:
        load_flags |= FT_LOAD_TARGET_MONO;
        break;
    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    case WINE_GGO_GRAY16_BITMAP:
        load_flags |= FT_LOAD_TARGET_NORMAL;
        break;
    case WINE_GGO_HRGB_BITMAP:
    case WINE_GGO_HBGR_BITMAP:
        load_flags |= FT_LOAD_TARGET_LCD;
        break;
    case WINE_GGO_VRGB_BITMAP:
    case WINE_GGO_VBGR_BITMAP:
        load_flags |= FT_LOAD_TARGET_LCD_V;
        break;
    }

    return load_flags;
}

/*************************************************************
 * freetype_get_glyph_outline
 */
static UINT freetype_get_glyph_outline( struct gdi_font *font, UINT glyph, UINT format,
                                        GLYPHMETRICS *lpgm, ABC *abc, UINT buflen, void *buf,
                                        const MAT2 *lpmat, BOOL tategaki )
{
    struct gdi_font *base_font = font->base_font ? font->base_font : font;
    FT_Face ft_face = get_ft_face( font );
    FT_Glyph_Metrics metrics;
    FT_Error err;
    FT_BBox bbox;
    FT_Int load_flags = get_load_flags(format);
    FT_Matrix transform_matrices[3], *matrices = NULL;
    BOOL vertical_metrics;

    TRACE("%p, %04x, %08x, %p, %08x, %p, %p\n", font, glyph, format, lpgm, buflen, buf, lpmat);

    TRACE("font transform %f %f %f %f\n",
          font->matrix.eM11, font->matrix.eM12,
          font->matrix.eM21, font->matrix.eM22);

    format &= ~GGO_UNHINTED;

    matrices = get_transform_matrices( font, tategaki, lpmat, transform_matrices );

    vertical_metrics = (tategaki && FT_HAS_VERTICAL(ft_face));

    if (matrices || format != GGO_BITMAP) load_flags |= FT_LOAD_NO_BITMAP;
    if (vertical_metrics) load_flags |= FT_LOAD_VERTICAL_LAYOUT;

    err = pFT_Load_Glyph(ft_face, glyph, load_flags & FT_LOAD_NO_HINTING ? load_flags : load_flags | FT_LOAD_PEDANTIC);
    if (err && !(load_flags & FT_LOAD_NO_HINTING))
    {
        WARN("Failed to load glyph %#x, retrying without hinting. Error %#x.\n", glyph, err);
        load_flags |= FT_LOAD_NO_HINTING;
        err = pFT_Load_Glyph(ft_face, glyph, load_flags);
    }

    if(err) {
        WARN("Failed to load glyph %#x, error %#x.\n", glyph, err);
        return GDI_ERROR;
    }

    metrics = ft_face->glyph->metrics;
    if(font->fake_bold) {
        if (!get_bold_glyph_outline(ft_face->glyph, font->ppem, &metrics) && metrics.width)
            metrics.width += 1 << 6;
    }

    /* Some poorly-created fonts contain glyphs that exceed the boundaries set
     * by the text metrics. The proper behavior is to clip the glyph metrics to
     * fit within the maximums specified in the text metrics. */
    if (freetype_set_outline_text_metrics(base_font) ||
        freetype_set_bitmap_text_metrics(base_font)) {
        TEXTMETRICW *ptm = &base_font->otm.otmTextMetrics;
        INT top = min( metrics.horiBearingY, ptm->tmAscent << 6 );
        INT bottom = max( metrics.horiBearingY - metrics.height, -(ptm->tmDescent << 6) );
        metrics.horiBearingY = top;
        metrics.height = top - bottom;

        /* TODO: Are we supposed to clip the width as well...? */
        /* metrics.width = min( metrics.width, ptm->tmMaxCharWidth << 6 ); */
    }

    bbox = get_transformed_bbox( &metrics, matrices );
    compute_metrics( font, bbox, &metrics, tategaki, vertical_metrics, matrices, lpgm, abc );

    switch (format)
    {
    case GGO_METRICS:
        return 1;  /* FIXME */

    case GGO_BITMAP:
        return get_mono_glyph_bitmap( ft_face->glyph, bbox, font->fake_bold,
                                      matrices, buflen, buf );

    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    case WINE_GGO_GRAY16_BITMAP:
        return get_antialias_glyph_bitmap( ft_face->glyph, bbox, format, font->fake_bold,
                                           matrices, buflen, buf );

    case WINE_GGO_HRGB_BITMAP:
    case WINE_GGO_HBGR_BITMAP:
    case WINE_GGO_VRGB_BITMAP:
    case WINE_GGO_VBGR_BITMAP:
        return get_subpixel_glyph_bitmap( ft_face->glyph, bbox, format, font->fake_bold,
                                          matrices, lpgm, buflen, buf );

    case GGO_NATIVE:
        if (ft_face->glyph->format == ft_glyph_format_outline)
        {
            FT_Outline *outline = &ft_face->glyph->outline;
            UINT needed;

            if (buflen == 0) buf = NULL;

            if (matrices && buf)
                pFT_Outline_Transform( outline, &matrices[matrix_vert] );

            needed = get_native_glyph_outline(outline, buflen, NULL);

            if (!buf || !buflen) return needed;
            if (needed > buflen) return GDI_ERROR;
            return get_native_glyph_outline(outline, buflen, buf);
        }
        TRACE("loaded a bitmap\n");
        return GDI_ERROR;

    case GGO_BEZIER:
        if (ft_face->glyph->format == ft_glyph_format_outline)
        {
            FT_Outline *outline = &ft_face->glyph->outline;
            UINT needed;

            if (buflen == 0) buf = NULL;

            if (matrices && buf)
                pFT_Outline_Transform( outline, &matrices[matrix_vert] );

            needed = get_bezier_glyph_outline(outline, buflen, NULL);

            if (!buf || !buflen) return needed;
            if (needed > buflen) return GDI_ERROR;
            return get_bezier_glyph_outline(outline, buflen, buf);
        }
        TRACE("loaded a bitmap\n");
        return GDI_ERROR;

    default:
        FIXME("Unsupported format %d\n", format);
	return GDI_ERROR;
    }
}

/*************************************************************
 * freetype_set_bitmap_text_metrics
 */
static BOOL freetype_set_bitmap_text_metrics( struct gdi_font *font )
{
    FT_Face ft_face = get_ft_face( font );
    FT_WinFNT_HeaderRec winfnt_header;

    if (font->otm.otmSize) return TRUE;  /* already set */
    font->otm.otmSize = offsetof( OUTLINETEXTMETRICW, otmFiller );

#define TM font->otm.otmTextMetrics
    if(!pFT_Get_WinFNT_Header(ft_face, &winfnt_header))
    {
        TM.tmHeight = winfnt_header.pixel_height;
        TM.tmAscent = winfnt_header.ascent;
        TM.tmDescent = TM.tmHeight - TM.tmAscent;
        TM.tmInternalLeading = winfnt_header.internal_leading;
        TM.tmExternalLeading = winfnt_header.external_leading;
        TM.tmAveCharWidth = winfnt_header.avg_width;
        TM.tmMaxCharWidth = winfnt_header.max_width;
        TM.tmWeight = winfnt_header.weight;
        TM.tmOverhang = 0;
        TM.tmDigitizedAspectX = winfnt_header.horizontal_resolution;
        TM.tmDigitizedAspectY = winfnt_header.vertical_resolution;
        TM.tmFirstChar = winfnt_header.first_char;
        TM.tmLastChar = winfnt_header.last_char;
        TM.tmDefaultChar = winfnt_header.default_char + winfnt_header.first_char;
        TM.tmBreakChar = winfnt_header.break_char + winfnt_header.first_char;
        TM.tmItalic = winfnt_header.italic;
        TM.tmPitchAndFamily = winfnt_header.pitch_and_family;
        TM.tmCharSet = winfnt_header.charset;
    }
    else
    {
        TM.tmAscent = ft_face->size->metrics.ascender >> 6;
        TM.tmDescent = -ft_face->size->metrics.descender >> 6;
        TM.tmHeight = TM.tmAscent + TM.tmDescent;
        TM.tmInternalLeading = TM.tmHeight - ft_face->size->metrics.y_ppem;
        TM.tmExternalLeading = (ft_face->size->metrics.height >> 6) - TM.tmHeight;
        TM.tmMaxCharWidth = ft_face->size->metrics.max_advance >> 6;
        TM.tmAveCharWidth = TM.tmMaxCharWidth * 2 / 3; /* FIXME */
        TM.tmWeight = ft_face->style_flags & FT_STYLE_FLAG_BOLD ? FW_BOLD : FW_NORMAL;
        TM.tmOverhang = 0;
        TM.tmDigitizedAspectX = 96; /* FIXME */
        TM.tmDigitizedAspectY = 96; /* FIXME */
        TM.tmFirstChar = 1;
        TM.tmLastChar = 255;
        TM.tmDefaultChar = 32;
        TM.tmBreakChar = 32;
        TM.tmItalic = ft_face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;
        /* NB inverted meaning of TMPF_FIXED_PITCH */
        TM.tmPitchAndFamily = FT_IS_FIXED_WIDTH(ft_face) ? 0 : TMPF_FIXED_PITCH;
        TM.tmCharSet = font->charset;
    }
    TM.tmUnderlined = font->lf.lfUnderline ? 0xff : 0;
    TM.tmStruckOut = font->lf.lfStrikeOut ? 0xff : 0;

    if(font->fake_bold)
        TM.tmWeight = FW_BOLD;
#undef TM

    return TRUE;
}


static BOOL face_has_symbol_charmap(FT_Face ft_face)
{
    int i;

    for(i = 0; i < ft_face->num_charmaps; i++)
    {
        if(ft_face->charmaps[i]->encoding == FT_ENCODING_MS_SYMBOL)
            return TRUE;
    }
    return FALSE;
}

/*************************************************************
 * freetype_set_outline_text_metrics
 */
static BOOL freetype_set_outline_text_metrics( struct gdi_font *font )
{
    FT_Face ft_face = get_ft_face( font );
    UINT needed;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    TT_Postscript *pPost;
    FT_Fixed em_scale;
    INT ascent, descent;
    USHORT windescent;

    TRACE("font=%p\n", font);

    if (!font->scalable) return FALSE;
    if (font->otm.otmSize) return TRUE;  /* already set */

    /* note: we store actual pointers in the names instead of offsets,
       they are fixed up when returned to the app */
    if (!(font->otm.otmpFullName = (char *)get_face_name( ft_face, TT_NAME_ID_UNIQUE_ID, system_lcid )))
    {
        static const WCHAR fake_nameW[] = {'f','a','k','e',' ','n','a','m','e', 0};
        FIXME("failed to read full_nameW for font %s!\n", wine_dbgstr_w((WCHAR *)font->otm.otmpFamilyName));
        font->otm.otmpFullName = (char *)wcsdup( fake_nameW );
    }
    needed = sizeof(font->otm) + (lstrlenW( (WCHAR *)font->otm.otmpFamilyName ) + 1 +
                                  lstrlenW( (WCHAR *)font->otm.otmpStyleName ) + 1 +
                                  lstrlenW( (WCHAR *)font->otm.otmpFaceName ) + 1 +
                                  lstrlenW( (WCHAR *)font->otm.otmpFullName ) + 1) * sizeof(WCHAR);

    em_scale = (FT_Fixed)pFT_MulDiv(font->ppem, 1 << 16, ft_face->units_per_EM);

    pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
    if(!pOS2) {
        FIXME("Can't find OS/2 table - not TT font?\n");
	return FALSE;
    }

    pHori = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea);
    if(!pHori) {
        FIXME("Can't find HHEA table - not TT font?\n");
	return FALSE;
    }

    pPost = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_post); /* we can live with this failing */

    TRACE("OS/2 winA = %u winD = %u typoA = %d typoD = %d typoLG = %d avgW %d FT_Face a = %d, d = %d, h = %d: HORZ a = %d, d = %d lg = %d maxY = %ld minY = %ld\n",
	  pOS2->usWinAscent, pOS2->usWinDescent,
	  pOS2->sTypoAscender, pOS2->sTypoDescender, pOS2->sTypoLineGap,
	  pOS2->xAvgCharWidth,
	  ft_face->ascender, ft_face->descender, ft_face->height,
	  pHori->Ascender, pHori->Descender, pHori->Line_Gap,
	  ft_face->bbox.yMax, ft_face->bbox.yMin);

    font->otm.otmSize = needed;

#define TM font->otm.otmTextMetrics

    windescent = get_fixed_windescent(pOS2->usWinDescent);
    if(pOS2->usWinAscent + windescent == 0) {
        ascent = pHori->Ascender;
        descent = -pHori->Descender;
    } else {
        ascent = pOS2->usWinAscent;
        descent = windescent;
    }

    font->ntmAvgWidth = pOS2->xAvgCharWidth;

#define SCALE_X(x) (pFT_MulFix(x, em_scale))
#define SCALE_Y(y) (pFT_MulFix(y, em_scale))

    if(font->yMax) {
	TM.tmAscent = font->yMax;
	TM.tmDescent = -font->yMin;
	TM.tmInternalLeading = (TM.tmAscent + TM.tmDescent) - ft_face->size->metrics.y_ppem;
    } else {
	TM.tmAscent = SCALE_Y(ascent);
	TM.tmDescent = SCALE_Y(descent);
	TM.tmInternalLeading = SCALE_Y(ascent + descent - ft_face->units_per_EM);
    }

    TM.tmHeight = TM.tmAscent + TM.tmDescent;

    /* MSDN says:
     el = MAX(0, LineGap - ((WinAscent + WinDescent) - (Ascender - Descender)))
    */
    TM.tmExternalLeading = max(0, SCALE_Y(pHori->Line_Gap -
                                          ((ascent + descent) -
                                           (pHori->Ascender - pHori->Descender))));

    TM.tmAveCharWidth = SCALE_X(pOS2->xAvgCharWidth);
    if (TM.tmAveCharWidth == 0) {
        TM.tmAveCharWidth = 1; 
    }
    TM.tmMaxCharWidth = SCALE_X(ft_face->bbox.xMax - ft_face->bbox.xMin);
    TM.tmWeight = font->fake_bold ? FW_BOLD : pOS2->usWeightClass;

    TM.tmOverhang = 0;
    TM.tmDigitizedAspectX = 96; /* FIXME */
    TM.tmDigitizedAspectY = 96; /* FIXME */
    /* It appears that for fonts with SYMBOL_CHARSET Windows always sets
     * symbol range to 0 - f0ff
     */

    if (face_has_symbol_charmap(ft_face) || (pOS2->usFirstCharIndex >= 0xf000 && pOS2->usFirstCharIndex < 0xf100))
    {
        TM.tmFirstChar = 0;
        switch (PRIMARYLANGID(system_lcid))
        {
        case LANG_HEBREW:
            TM.tmLastChar = 0xf896;
            break;
        case LANG_ESTONIAN:
        case LANG_LATVIAN:
        case LANG_LITHUANIAN:
            TM.tmLastChar = 0xf8fd;
            break;
        default:
            TM.tmLastChar = 0xf0ff;
        }
        TM.tmBreakChar = 0x20;
        TM.tmDefaultChar = 0x1f;
    }
    else
    {
        TM.tmFirstChar = pOS2->usFirstCharIndex; /* Should be the first char in the cmap */
        TM.tmLastChar = pOS2->usLastCharIndex;   /* Should be min(cmap_last, os2_last) */

        if(pOS2->usFirstCharIndex <= 1)
            TM.tmBreakChar = pOS2->usFirstCharIndex + 2;
        else if (pOS2->usFirstCharIndex > 0xff)
            TM.tmBreakChar = 0x20;
        else
            TM.tmBreakChar = pOS2->usFirstCharIndex;
        TM.tmDefaultChar = TM.tmBreakChar - 1;
    }
    TM.tmItalic = font->fake_italic ? 255 : ((ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 255 : 0);
    TM.tmUnderlined = font->lf.lfUnderline ? 255 : 0;
    TM.tmStruckOut = font->lf.lfStrikeOut ? 255 : 0;

    /* Yes TPMF_FIXED_PITCH is correct; braindead api */
    if(!FT_IS_FIXED_WIDTH(ft_face) &&
       (pOS2->version == 0xFFFFU || 
        pOS2->panose[PAN_PROPORTION_INDEX] != PAN_PROP_MONOSPACED))
        TM.tmPitchAndFamily = TMPF_FIXED_PITCH;
    else
        TM.tmPitchAndFamily = 0;

    switch(pOS2->panose[PAN_FAMILYTYPE_INDEX])
    {
    case PAN_FAMILY_SCRIPT:
        TM.tmPitchAndFamily |= FF_SCRIPT;
        break;

    case PAN_FAMILY_DECORATIVE:
        TM.tmPitchAndFamily |= FF_DECORATIVE;
        break;

    case PAN_ANY:
    case PAN_NO_FIT:
    case PAN_FAMILY_TEXT_DISPLAY:
    case PAN_FAMILY_PICTORIAL: /* symbol fonts get treated as if they were text */
                               /* which is clearly not what the panose spec says. */
    default:
        if(TM.tmPitchAndFamily == 0 || /* fixed */
           pOS2->panose[PAN_PROPORTION_INDEX] == PAN_PROP_MONOSPACED)
	    TM.tmPitchAndFamily = FF_MODERN;
        else
        {
            switch(pOS2->panose[PAN_SERIFSTYLE_INDEX])
            {
            case PAN_ANY:
            case PAN_NO_FIT:
            default:
                TM.tmPitchAndFamily |= FF_DONTCARE;
                break;

            case PAN_SERIF_COVE:
            case PAN_SERIF_OBTUSE_COVE:
            case PAN_SERIF_SQUARE_COVE:
            case PAN_SERIF_OBTUSE_SQUARE_COVE:
            case PAN_SERIF_SQUARE:
            case PAN_SERIF_THIN:
            case PAN_SERIF_BONE:
            case PAN_SERIF_EXAGGERATED:
            case PAN_SERIF_TRIANGLE:
                TM.tmPitchAndFamily |= FF_ROMAN;
                break;

            case PAN_SERIF_NORMAL_SANS:
            case PAN_SERIF_OBTUSE_SANS:
            case PAN_SERIF_PERP_SANS:
            case PAN_SERIF_FLARED:
            case PAN_SERIF_ROUNDED:
                TM.tmPitchAndFamily |= FF_SWISS;
                break;
            }
	}
	break;
    }

    if(FT_IS_SCALABLE(ft_face))
        TM.tmPitchAndFamily |= TMPF_VECTOR;

    if(FT_IS_SFNT(ft_face))
    {
        if (font->ntmFlags & NTM_PS_OPENTYPE)
            TM.tmPitchAndFamily |= TMPF_DEVICE;
        else
            TM.tmPitchAndFamily |= TMPF_TRUETYPE;
    }

    TM.tmCharSet = font->charset;

    font->otm.otmFiller = 0;
    memcpy(&font->otm.otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
    font->otm.otmfsSelection = pOS2->fsSelection;
    if (font->fake_italic)
        font->otm.otmfsSelection |= 1;
    if (font->fake_bold)
        font->otm.otmfsSelection |= 1 << 5;
    /* Only return valid bits that define embedding and subsetting restrictions */
    font->otm.otmfsType = pOS2->fsType & 0x30e;
    font->otm.otmsCharSlopeRise = pHori->caret_Slope_Rise;
    font->otm.otmsCharSlopeRun = pHori->caret_Slope_Run;
    font->otm.otmItalicAngle = 0; /* POST table */
    font->otm.otmAscent = SCALE_Y(pOS2->sTypoAscender);
    font->otm.otmDescent = SCALE_Y(pOS2->sTypoDescender);
    font->otm.otmLineGap = SCALE_Y(pOS2->sTypoLineGap);
    font->otm.otmsCapEmHeight = SCALE_Y(pOS2->sCapHeight);
    font->otm.otmsXHeight = SCALE_Y(pOS2->sxHeight);
    font->otm.otmrcFontBox.left = SCALE_X(ft_face->bbox.xMin);
    font->otm.otmrcFontBox.right = SCALE_X(ft_face->bbox.xMax);
    font->otm.otmrcFontBox.top = SCALE_Y(ft_face->bbox.yMax);
    font->otm.otmrcFontBox.bottom = SCALE_Y(ft_face->bbox.yMin);
    font->otm.otmMacAscent = TM.tmAscent;
    font->otm.otmMacDescent = -TM.tmDescent;
    font->otm.otmMacLineGap = SCALE_Y(pHori->Line_Gap);
    font->otm.otmusMinimumPPEM = 0; /* TT Header */
    font->otm.otmptSubscriptSize.x = SCALE_X(pOS2->ySubscriptXSize);
    font->otm.otmptSubscriptSize.y = SCALE_Y(pOS2->ySubscriptYSize);
    font->otm.otmptSubscriptOffset.x = SCALE_X(pOS2->ySubscriptXOffset);
    font->otm.otmptSubscriptOffset.y = SCALE_Y(pOS2->ySubscriptYOffset);
    font->otm.otmptSuperscriptSize.x = SCALE_X(pOS2->ySuperscriptXSize);
    font->otm.otmptSuperscriptSize.y = SCALE_Y(pOS2->ySuperscriptYSize);
    font->otm.otmptSuperscriptOffset.x = SCALE_X(pOS2->ySuperscriptXOffset);
    font->otm.otmptSuperscriptOffset.y = SCALE_Y(pOS2->ySuperscriptYOffset);
    font->otm.otmsStrikeoutSize = SCALE_Y(pOS2->yStrikeoutSize);
    font->otm.otmsStrikeoutPosition = SCALE_Y(pOS2->yStrikeoutPosition);
    if(!pPost) {
        font->otm.otmsUnderscoreSize = 0;
	font->otm.otmsUnderscorePosition = 0;
    } else {
        font->otm.otmsUnderscoreSize = SCALE_Y(pPost->underlineThickness);
	font->otm.otmsUnderscorePosition = SCALE_Y(pPost->underlinePosition);
    }
#undef SCALE_X
#undef SCALE_Y
#undef TM
    return TRUE;
}

/*************************************************************
 * freetype_get_char_width_info
 */
static BOOL freetype_get_char_width_info( struct gdi_font *font, struct char_width_info *info )
{
    FT_Face ft_face = get_ft_face( font );
    TT_HoriHeader *pHori;

    TRACE("%p, %p\n", font, info);

    if ((pHori = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea)))
    {
        FT_Fixed em_scale = pFT_MulDiv(font->ppem, 1 << 16, ft_face->units_per_EM);
        info->lsb = (SHORT)pFT_MulFix(pHori->min_Left_Side_Bearing,  em_scale);
        info->rsb = (SHORT)pFT_MulFix(pHori->min_Right_Side_Bearing, em_scale);
        return TRUE;
    }
    return FALSE;
}


/*************************************************************
 * freetype_get_unicode_ranges
 *
 * Retrieve a list of supported Unicode ranges for a given font.
 * Can be called with NULL gs to calculate the buffer size. Returns
 * the number of ranges found.
 */
static UINT freetype_get_unicode_ranges( struct gdi_font *font, GLYPHSET *gs )
{
    FT_Face ft_face = get_ft_face( font );
    UINT num_ranges = 0;

    if (ft_face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt glyph_code;
        FT_ULong char_code, char_code_prev;

        glyph_code = 0;
        char_code_prev = char_code = pFT_Get_First_Char(ft_face, &glyph_code);

        TRACE("face encoding FT_ENCODING_UNICODE, number of glyphs %ld, first glyph %u, first char %04lx\n",
               ft_face->num_glyphs, glyph_code, char_code);

        if (!glyph_code) return 0;

        if (gs)
        {
            gs->ranges[0].wcLow = (USHORT)char_code;
            gs->ranges[0].cGlyphs = 0;
            gs->cGlyphsSupported = 0;
        }

        num_ranges = 1;
        while (glyph_code)
        {
            if (char_code < char_code_prev)
            {
                ERR("expected increasing char code from FT_Get_Next_Char\n");
                return 0;
            }
            if (char_code - char_code_prev > 1)
            {
                num_ranges++;
                if (gs)
                {
                    gs->ranges[num_ranges - 1].wcLow = (USHORT)char_code;
                    gs->ranges[num_ranges - 1].cGlyphs = 1;
                    gs->cGlyphsSupported++;
                }
            }
            else if (gs)
            {
                gs->ranges[num_ranges - 1].cGlyphs++;
                gs->cGlyphsSupported++;
            }
            char_code_prev = char_code;
            char_code = pFT_Get_Next_Char(ft_face, char_code, &glyph_code);
        }
    }
    else
    {
        DWORD encoding = RtlUlongByteSwap(ft_face->charmap->encoding);
        FIXME("encoding %s not supported\n", debugstr_fourcc(encoding));
    }

    return num_ranges;
}

/*************************************************************************
 * Kerning support for TrueType fonts
 */

struct TT_kern_table
{
    USHORT version;
    USHORT nTables;
};

struct TT_kern_subtable
{
    USHORT version;
    USHORT length;
    union
    {
        USHORT word;
        struct
        {
            USHORT horizontal : 1;
            USHORT minimum : 1;
            USHORT cross_stream: 1;
            USHORT override : 1;
            USHORT reserved1 : 4;
            USHORT format : 8;
        } bits;
    } coverage;
};

struct TT_format0_kern_subtable
{
    USHORT nPairs;
    USHORT searchRange;
    USHORT entrySelector;
    USHORT rangeShift;
};

struct TT_kern_pair
{
    USHORT left;
    USHORT right;
    short  value;
};

static DWORD parse_format0_kern_subtable(struct gdi_font *font,
                                         const struct TT_format0_kern_subtable *tt_f0_ks,
                                         const USHORT *glyph_to_char,
                                         KERNINGPAIR *kern_pair, DWORD cPairs)
{
    FT_Face ft_face = get_ft_face( font );
    USHORT i, nPairs;
    const struct TT_kern_pair *tt_kern_pair;

    TRACE("font height %d, units_per_EM %d\n", font->ppem, ft_face->units_per_EM);

    nPairs = GET_BE_WORD(tt_f0_ks->nPairs);

    TRACE("nPairs %u, searchRange %u, entrySelector %u, rangeShift %u\n",
           nPairs, GET_BE_WORD(tt_f0_ks->searchRange),
           GET_BE_WORD(tt_f0_ks->entrySelector), GET_BE_WORD(tt_f0_ks->rangeShift));

    if (!kern_pair || !cPairs)
        return nPairs;

    tt_kern_pair = (const struct TT_kern_pair *)(tt_f0_ks + 1);

    nPairs = min(nPairs, cPairs);

    for (i = 0; i < nPairs; i++)
    {
        kern_pair->wFirst = glyph_to_char[GET_BE_WORD(tt_kern_pair[i].left)];
        kern_pair->wSecond = glyph_to_char[GET_BE_WORD(tt_kern_pair[i].right)];
        /* this algorithm appears to better match what Windows does */
        kern_pair->iKernAmount = (short)GET_BE_WORD(tt_kern_pair[i].value) * font->ppem;
        if (kern_pair->iKernAmount < 0)
        {
            kern_pair->iKernAmount -= ft_face->units_per_EM / 2;
            kern_pair->iKernAmount -= font->ppem;
        }
        else if (kern_pair->iKernAmount > 0)
        {
            kern_pair->iKernAmount += ft_face->units_per_EM / 2;
            kern_pair->iKernAmount += font->ppem;
        }
        kern_pair->iKernAmount /= ft_face->units_per_EM;

        TRACE("left %u right %u value %d\n",
               kern_pair->wFirst, kern_pair->wSecond, kern_pair->iKernAmount);

        kern_pair++;
    }
    TRACE("copied %u entries\n", nPairs);
    return nPairs;
}

/*************************************************************
 * freetype_get_kerning_pairs
 */
static UINT freetype_get_kerning_pairs( struct gdi_font *font, KERNINGPAIR **pairs )
{
    FT_Face ft_face = get_ft_face( font );
    UINT length, count = 0;
    void *buf;
    const struct TT_kern_table *tt_kern_table;
    const struct TT_kern_subtable *tt_kern_subtable;
    USHORT i, nTables;
    USHORT *glyph_to_char;

    length = freetype_get_font_data(font, MS_KERN_TAG, 0, NULL, 0);

    if (length == GDI_ERROR)
    {
        TRACE("no kerning data in the font\n");
        return 0;
    }

    buf = malloc( length );
    if (!buf) return 0;

    freetype_get_font_data(font, MS_KERN_TAG, 0, buf, length);

    /* build a glyph index to char code map */
    glyph_to_char = calloc( sizeof(USHORT), 65536 );
    if (!glyph_to_char)
    {
        free( buf );
        return 0;
    }

    if (ft_face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt glyph_code;
        FT_ULong char_code;

        glyph_code = 0;
        char_code = pFT_Get_First_Char(ft_face, &glyph_code);

        TRACE("face encoding FT_ENCODING_UNICODE, number of glyphs %ld, first glyph %u, first char %lu\n",
               ft_face->num_glyphs, glyph_code, char_code);

        while (glyph_code)
        {
            /*TRACE("Char %04lX -> Index %u%s\n", char_code, glyph_code, glyph_to_char[glyph_code] ? "  !" : "" );*/

            /* FIXME: This doesn't match what Windows does: it does some fancy
             * things with duplicate glyph index to char code mappings, while
             * we just avoid overriding existing entries.
             */
            if (glyph_code <= 65535 && !glyph_to_char[glyph_code])
                glyph_to_char[glyph_code] = (USHORT)char_code;

            char_code = pFT_Get_Next_Char(ft_face, char_code, &glyph_code);
        }
    }
    else
    {
        DWORD encoding = RtlUlongByteSwap(ft_face->charmap->encoding);
        ULONG n;

        FIXME("encoding %s not supported\n", debugstr_fourcc(encoding));
        for (n = 0; n <= 65535; n++)
            glyph_to_char[n] = (USHORT)n;
    }

    tt_kern_table = buf;
    nTables = GET_BE_WORD(tt_kern_table->nTables);
    TRACE("version %u, nTables %u\n",
           GET_BE_WORD(tt_kern_table->version), nTables);

    tt_kern_subtable = (const struct TT_kern_subtable *)(tt_kern_table + 1);

    for (i = 0; i < nTables; i++)
    {
        struct TT_kern_subtable tt_kern_subtable_copy;

        tt_kern_subtable_copy.version = GET_BE_WORD(tt_kern_subtable->version);
        tt_kern_subtable_copy.length = GET_BE_WORD(tt_kern_subtable->length);
        tt_kern_subtable_copy.coverage.word = GET_BE_WORD(tt_kern_subtable->coverage.word);

        TRACE("version %u, length %u, coverage %u, subtable format %u\n",
               tt_kern_subtable_copy.version, tt_kern_subtable_copy.length,
               tt_kern_subtable_copy.coverage.word, tt_kern_subtable_copy.coverage.bits.format);

        /* According to the TrueType specification this is the only format
         * that will be properly interpreted by Windows and OS/2
         */
        if (tt_kern_subtable_copy.coverage.bits.format == 0)
        {
            DWORD new_chunk, old_total = count;

            new_chunk = parse_format0_kern_subtable(font, (const struct TT_format0_kern_subtable *)(tt_kern_subtable + 1),
                                                    glyph_to_char, NULL, 0);
            count += new_chunk;

            *pairs = realloc( *pairs, count * sizeof(**pairs));

            parse_format0_kern_subtable(font, (const struct TT_format0_kern_subtable *)(tt_kern_subtable + 1),
                        glyph_to_char, *pairs + old_total, new_chunk);
        }
        else
            TRACE("skipping kerning table format %u\n", tt_kern_subtable_copy.coverage.bits.format);

        tt_kern_subtable = (const struct TT_kern_subtable *)((const char *)tt_kern_subtable + tt_kern_subtable_copy.length);
    }

    free( glyph_to_char );
    free( buf );
    return count;
}

static const struct font_backend_funcs font_funcs =
{
    freetype_load_fonts,
    fontconfig_enum_family_fallbacks,
    freetype_add_font,
    freetype_add_mem_font,
    freetype_load_font,
    freetype_get_font_data,
    freetype_get_aa_flags,
    freetype_get_glyph_index,
    freetype_get_default_glyph,
    freetype_get_glyph_outline,
    freetype_get_unicode_ranges,
    freetype_get_char_width_info,
    freetype_set_outline_text_metrics,
    freetype_set_bitmap_text_metrics,
    freetype_get_kerning_pairs,
    freetype_destroy_font
};

const struct font_backend_funcs *init_freetype_lib(void)
{
    if (!init_freetype()) return NULL;
#ifdef SONAME_LIBFONTCONFIG
    init_fontconfig();
#endif
    NtQueryDefaultLocale( FALSE, &system_lcid );
    return &font_funcs;
}

#else /* SONAME_LIBFREETYPE */

const struct font_backend_funcs *init_freetype_lib(void)
{
    return NULL;
}

#endif /* SONAME_LIBFREETYPE */
