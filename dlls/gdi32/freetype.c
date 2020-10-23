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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include <string.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif
#include <stdio.h>
#include <assert.h>

#ifdef HAVE_CARBON_CARBON_H
#define LoadResource __carbon_LoadResource
#define CompareString __carbon_CompareString
#define GetCurrentThread __carbon_GetCurrentThread
#define GetCurrentProcess __carbon_GetCurrentProcess
#define AnimatePalette __carbon_AnimatePalette
#define EqualRgn __carbon_EqualRgn
#define FillRgn __carbon_FillRgn
#define FrameRgn __carbon_FrameRgn
#define GetPixel __carbon_GetPixel
#define InvertRgn __carbon_InvertRgn
#define LineTo __carbon_LineTo
#define OffsetRgn __carbon_OffsetRgn
#define PaintRgn __carbon_PaintRgn
#define Polygon __carbon_Polygon
#define ResizePalette __carbon_ResizePalette
#define SetRectRgn __carbon_SetRectRgn
#include <Carbon/Carbon.h>
#undef LoadResource
#undef CompareString
#undef GetCurrentThread
#undef _CDECL
#undef GetCurrentProcess
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#endif /* HAVE_CARBON_CARBON_H */

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
#ifdef FT_LCD_FILTER_H
#include FT_LCD_FILTER_H
#endif
#endif /* HAVE_FT2BUILD_H */

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

#ifdef HAVE_FREETYPE

#ifndef HAVE_FT_TRUETYPEENGINETYPE
typedef enum
{
    FT_TRUETYPE_ENGINE_TYPE_NONE = 0,
    FT_TRUETYPE_ENGINE_TYPE_UNPATENTED,
    FT_TRUETYPE_ENGINE_TYPE_PATENTED
} FT_TrueTypeEngineType;
#endif

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
MAKE_FUNCPTR(FT_Get_WinFNT_Header);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Library_Version);
MAKE_FUNCPTR(FT_Load_Glyph);
MAKE_FUNCPTR(FT_Load_Sfnt_Table);
MAKE_FUNCPTR(FT_Matrix_Multiply);
#ifdef FT_MULFIX_INLINED
#define pFT_MulFix FT_MULFIX_INLINED
#else
MAKE_FUNCPTR(FT_MulFix);
#endif
MAKE_FUNCPTR(FT_New_Face);
MAKE_FUNCPTR(FT_New_Memory_Face);
MAKE_FUNCPTR(FT_Outline_Get_Bitmap);
MAKE_FUNCPTR(FT_Outline_Get_CBox);
MAKE_FUNCPTR(FT_Outline_Transform);
MAKE_FUNCPTR(FT_Outline_Translate);
MAKE_FUNCPTR(FT_Render_Glyph);
MAKE_FUNCPTR(FT_Set_Charmap);
MAKE_FUNCPTR(FT_Set_Pixel_Sizes);
MAKE_FUNCPTR(FT_Vector_Length);
MAKE_FUNCPTR(FT_Vector_Transform);
MAKE_FUNCPTR(FT_Vector_Unit);
static FT_Error (*pFT_Outline_Embolden)(FT_Outline *, FT_Pos);
static FT_TrueTypeEngineType (*pFT_Get_TrueType_Engine_Type)(FT_Library);
#ifdef FT_LCD_FILTER_H
static FT_Error (*pFT_Library_SetLcdFilter)(FT_Library, FT_LcdFilter);
#endif
static FT_Error (*pFT_Property_Set)(FT_Library, const FT_String *, const FT_String *, const void *);

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

#ifndef WINE_FONT_DIR
#define WINE_FONT_DIR "fonts"
#endif

/* This is basically a copy of FT_Bitmap_Size with an extra element added */
typedef struct {
    FT_Short height;
    FT_Short width;
    FT_Pos size;
    FT_Pos x_ppem;
    FT_Pos y_ppem;
    FT_Short internal_leading;
} Bitmap_Size;

/* FT_Bitmap_Size gained 3 new elements between FreeType 2.1.4 and 2.1.5
   So to let this compile on older versions of FreeType we'll define the
   new structure here. */
typedef struct {
    FT_Short height, width;
    FT_Pos size, x_ppem, y_ppem;
} My_FT_Bitmap_Size;

struct enum_data
{
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
    DWORD type;
};

typedef struct tagFace {
    struct list entry;
    unsigned int refcount;
    WCHAR *style_name;
    WCHAR *full_name;
    WCHAR *file;
    void *font_data_ptr;
    DWORD font_data_size;
    FT_Long face_index;
    FONTSIGNATURE fs;
    DWORD ntmFlags;
    FT_Fixed font_version;
    BOOL scalable;
    Bitmap_Size size;     /* set if face is a bitmap */
    DWORD flags;          /* ADDFONT flags */
    struct tagFamily *family;
    /* Cached data for Enum */
    struct enum_data *cached_enum_data;
} Face;

#define FS_DBCS_MASK (FS_JISJAPAN|FS_CHINESESIMP|FS_WANSUNG|FS_CHINESETRAD|FS_JOHAB)

#define ADDFONT_EXTERNAL_FONT 0x01
#define ADDFONT_ALLOW_BITMAP  0x02
#define ADDFONT_ADD_TO_CACHE  0x04
#define ADDFONT_ADD_RESOURCE  0x08  /* added through AddFontResource */
#define ADDFONT_VERTICAL_FONT 0x10
#define ADDFONT_AA_FLAGS(flags) ((flags) << 16)

typedef struct tagFamily {
    struct list entry;
    unsigned int refcount;
    WCHAR family_name[LF_FACESIZE];
    WCHAR second_name[LF_FACESIZE];
    struct list faces;
    struct list *replacement;
} Family;

typedef struct tagGdiFont GdiFont;

typedef struct {
    struct list entry;
    Face *face;
    GdiFont *font;
} CHILD_FONT;

struct tagGdiFont {
    struct gdi_font *gdi_font;
    OUTLINETEXTMETRICW *potm;
    DWORD total_kern_pairs;
    KERNINGPAIR *kern_pairs;
    struct list child_fonts;

    /* the following members can be accessed without locking, they are never modified after creation */
    FT_Face ft_face;
    struct font_mapping *mapping;
    GdiFont *base_font;
    VOID *GSUB_Table;
    const VOID *vert_feature;
};

static inline GdiFont *get_font_ptr( struct gdi_font *font ) { return font->private; }

typedef struct {
    struct list entry;
    const WCHAR *font_name;
    FONTSIGNATURE fs;
    struct list links;
} SYSTEM_LINKS;

struct enum_charset_element {
    DWORD mask;
    DWORD charset;
    WCHAR name[LF_FACESIZE];
};

struct enum_charset_list {
    DWORD total;
    struct enum_charset_element element[32];
};

static struct list system_links = LIST_INIT(system_links);

static struct list font_subst_list = LIST_INIT(font_subst_list);

static struct list font_list = LIST_INIT(font_list);

static const struct font_backend_funcs font_funcs;

static const WCHAR fontsW[] = {'\\','f','o','n','t','s','\0'};
static const WCHAR win9x_font_reg_key[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
                                           'W','i','n','d','o','w','s','\\',
                                           'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                           'F','o','n','t','s','\0'};

static const WCHAR winnt_font_reg_key[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
                                           'W','i','n','d','o','w','s',' ','N','T','\\',
                                           'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                           'F','o','n','t','s','\0'};

static const WCHAR system_fonts_reg_key[] = {'S','o','f','t','w','a','r','e','\\','F','o','n','t','s','\0'};
static const WCHAR FixedSys_Value[] = {'F','I','X','E','D','F','O','N','.','F','O','N','\0'};
static const WCHAR System_Value[] = {'F','O','N','T','S','.','F','O','N','\0'};
static const WCHAR OEMFont_Value[] = {'O','E','M','F','O','N','T','.','F','O','N','\0'};

static const WCHAR * const SystemFontValues[] = {
    System_Value,
    OEMFont_Value,
    FixedSys_Value,
    NULL
};

static const WCHAR external_fonts_reg_key[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                               'F','o','n','t','s','\\','E','x','t','e','r','n','a','l',' ','F','o','n','t','s','\0'};

/* Interesting and well-known (frequently-assumed!) font names */
static const WCHAR Lucida_Sans_Unicode[] = {'L','u','c','i','d','a',' ','S','a','n','s',' ','U','n','i','c','o','d','e',0};
static const WCHAR Microsoft_Sans_Serif[] = {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0 };
static const WCHAR Tahoma[] = {'T','a','h','o','m','a',0};
static const WCHAR MS_UI_Gothic[] = {'M','S',' ','U','I',' ','G','o','t','h','i','c',0};
static const WCHAR SimSun[] = {'S','i','m','S','u','n',0};
static const WCHAR Gulim[] = {'G','u','l','i','m',0};
static const WCHAR PMingLiU[] = {'P','M','i','n','g','L','i','U',0};
static const WCHAR Batang[] = {'B','a','t','a','n','g',0};

static const WCHAR arial[] = {'A','r','i','a','l',0};
static const WCHAR bitstream_vera_sans[] = {'B','i','t','s','t','r','e','a','m',' ','V','e','r','a',' ','S','a','n','s',0};
static const WCHAR bitstream_vera_sans_mono[] = {'B','i','t','s','t','r','e','a','m',' ','V','e','r','a',' ','S','a','n','s',' ','M','o','n','o',0};
static const WCHAR bitstream_vera_serif[] = {'B','i','t','s','t','r','e','a','m',' ','V','e','r','a',' ','S','e','r','i','f',0};
static const WCHAR courier_new[] = {'C','o','u','r','i','e','r',' ','N','e','w',0};
static const WCHAR liberation_mono[] = {'L','i','b','e','r','a','t','i','o','n',' ','M','o','n','o',0};
static const WCHAR liberation_sans[] = {'L','i','b','e','r','a','t','i','o','n',' ','S','a','n','s',0};
static const WCHAR liberation_serif[] = {'L','i','b','e','r','a','t','i','o','n',' ','S','e','r','i','f',0};
static const WCHAR times_new_roman[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n',0};
static const WCHAR SymbolW[] = {'S','y','m','b','o','l','\0'};

static const WCHAR *default_serif_list[] =
{
    times_new_roman,
    liberation_serif,
    bitstream_vera_serif,
    NULL
};

static const WCHAR *default_fixed_list[] =
{
    courier_new,
    liberation_mono,
    bitstream_vera_sans_mono,
    NULL
};

static const WCHAR *default_sans_list[] =
{
    arial,
    liberation_sans,
    bitstream_vera_sans,
    NULL
};

static const WCHAR *default_serif = times_new_roman;
static const WCHAR *default_fixed = courier_new;
static const WCHAR *default_sans = arial;

typedef struct {
    WCHAR *name;
    INT charset;
} NameCs;

typedef struct tagFontSubst {
    struct list entry;
    NameCs from;
    NameCs to;
} FontSubst;

/* Registry font cache key and value names */
static const WCHAR wine_fonts_key[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                       'F','o','n','t','s',0};
static const WCHAR wine_fonts_cache_key[] = {'C','a','c','h','e',0};


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
static HKEY hkey_font_cache;
static BOOL antialias_fakes = TRUE;

static const WCHAR font_mutex_nameW[] = {'_','_','W','I','N','E','_','F','O','N','T','_','M','U','T','E','X','_','_','\0'};

static const WCHAR szDefaultFallbackLink[] = {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0};

static BOOL map_font_family(const WCHAR *orig, const WCHAR *repl);
static BOOL get_glyph_index_linked(GdiFont *font, UINT c, GdiFont **linked_font, FT_UInt *glyph, BOOL *vert);
static BOOL get_outline_text_metrics(GdiFont *font);
static BOOL get_bitmap_text_metrics(GdiFont *font);
static BOOL get_text_metrics(GdiFont *font, LPTEXTMETRICW ptm);
static void remove_face_from_cache( Face *face );

static const WCHAR system_link[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
                                    'W','i','n','d','o','w','s',' ','N','T','\\',
                                    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','F','o','n','t','L','i','n','k','\\',
                                    'S','y','s','t','e','m','L','i','n','k',0};

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

/* These are all structures needed for the GSUB table */


typedef struct {
    DWORD version;
    WORD ScriptList;
    WORD FeatureList;
    WORD LookupList;
} GSUB_Header;

typedef struct {
    CHAR ScriptTag[4];
    WORD Script;
} GSUB_ScriptRecord;

typedef struct {
    WORD ScriptCount;
    GSUB_ScriptRecord ScriptRecord[1];
} GSUB_ScriptList;

typedef struct {
    CHAR LangSysTag[4];
    WORD LangSys;
} GSUB_LangSysRecord;

typedef struct {
    WORD DefaultLangSys;
    WORD LangSysCount;
    GSUB_LangSysRecord LangSysRecord[1];
} GSUB_Script;

typedef struct {
    WORD LookupOrder; /* Reserved */
    WORD ReqFeatureIndex;
    WORD FeatureCount;
    WORD FeatureIndex[1];
} GSUB_LangSys;

typedef struct {
    CHAR FeatureTag[4];
    WORD Feature;
} GSUB_FeatureRecord;

typedef struct {
    WORD FeatureCount;
    GSUB_FeatureRecord FeatureRecord[1];
} GSUB_FeatureList;

typedef struct {
    WORD FeatureParams; /* Reserved */
    WORD LookupCount;
    WORD LookupListIndex[1];
} GSUB_Feature;

typedef struct {
    WORD LookupCount;
    WORD Lookup[1];
} GSUB_LookupList;

typedef struct {
    WORD LookupType;
    WORD LookupFlag;
    WORD SubTableCount;
    WORD SubTable[1];
} GSUB_LookupTable;

typedef struct {
    WORD CoverageFormat;
    WORD GlyphCount;
    WORD GlyphArray[1];
} GSUB_CoverageFormat1;

typedef struct {
    WORD Start;
    WORD End;
    WORD StartCoverageIndex;
} GSUB_RangeRecord;

typedef struct {
    WORD CoverageFormat;
    WORD RangeCount;
    GSUB_RangeRecord RangeRecord[1];
} GSUB_CoverageFormat2;

typedef struct {
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD DeltaGlyphID;
} GSUB_SingleSubstFormat1;

typedef struct {
    WORD SubstFormat; /* = 2 */
    WORD Coverage;
    WORD GlyphCount;
    WORD Substitute[1];
}GSUB_SingleSubstFormat2;

#ifdef HAVE_CARBON_CARBON_H
static char *find_cache_dir(void)
{
    FSRef ref;
    OSErr err;
    static char cached_path[MAX_PATH];
    static const char *wine = "/Wine", *fonts = "/Fonts";

    if(*cached_path) return cached_path;

    err = FSFindFolder(kUserDomain, kCachedDataFolderType, kCreateFolder, &ref);
    if(err != noErr)
    {
        WARN("can't create cached data folder\n");
        return NULL;
    }
    err = FSRefMakePath(&ref, (unsigned char*)cached_path, sizeof(cached_path));
    if(err != noErr)
    {
        WARN("can't create cached data path\n");
        *cached_path = '\0';
        return NULL;
    }
    if(strlen(cached_path) + strlen(wine) + strlen(fonts) + 1 > sizeof(cached_path))
    {
        ERR("Could not create full path\n");
        *cached_path = '\0';
        return NULL;
    }
    strcat(cached_path, wine);

    if(mkdir(cached_path, 0700) == -1 && errno != EEXIST)
    {
        WARN("Couldn't mkdir %s\n", cached_path);
        *cached_path = '\0';
        return NULL;
    }
    strcat(cached_path, fonts);
    if(mkdir(cached_path, 0700) == -1 && errno != EEXIST)
    {
        WARN("Couldn't mkdir %s\n", cached_path);
        *cached_path = '\0';
        return NULL;
    }
    return cached_path;
}

/******************************************************************
 *            expand_mac_font
 *
 * Extracts individual TrueType font files from a Mac suitcase font
 * and saves them into the user's caches directory (see
 * find_cache_dir()).
 * Returns a NULL terminated array of filenames.
 *
 * We do this because they are apps that try to read ttf files
 * themselves and they don't like Mac suitcase files.
 */
static char **expand_mac_font(const char *path)
{
    FSRef ref;
    ResFileRefNum res_ref;
    OSStatus s;
    unsigned int idx;
    const char *out_dir;
    const char *filename;
    int output_len;
    struct {
        char **array;
        unsigned int size, max_size;
    } ret;

    TRACE("path %s\n", path);

    s = FSPathMakeRef((unsigned char*)path, &ref, FALSE);
    if(s != noErr)
    {
        WARN("failed to get ref\n");
        return NULL;
    }

    s = FSOpenResourceFile(&ref, 0, NULL, fsRdPerm, &res_ref);
    if(s != noErr)
    {
        TRACE("no data fork, so trying resource fork\n");
        res_ref = FSOpenResFile(&ref, fsRdPerm);
        if(res_ref == -1)
        {
            TRACE("unable to open resource fork\n");
            return NULL;
        }
    }

    ret.size = 0;
    ret.max_size = 10;
    ret.array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ret.max_size * sizeof(*ret.array));
    if(!ret.array)
    {
        CloseResFile(res_ref);
        return NULL;
    }

    out_dir = find_cache_dir();

    filename = strrchr(path, '/');
    if(!filename) filename = path;
    else filename++;

    /* output filename has the form out_dir/filename_%04x.ttf */
    output_len = strlen(out_dir) + 1 + strlen(filename) + 5 + 5;

    UseResFile(res_ref);
    idx = 1;
    while(1)
    {
        FamRec *fam_rec;
        unsigned short *num_faces_ptr, num_faces, face;
        AsscEntry *assoc;
        Handle fond;
        ResType fond_res = FT_MAKE_TAG('F','O','N','D');

        fond = Get1IndResource(fond_res, idx);
        if(!fond) break;
        TRACE("got fond resource %d\n", idx);
        HLock(fond);

        fam_rec = *(FamRec**)fond;
        num_faces_ptr = (unsigned short *)(fam_rec + 1);
        num_faces = GET_BE_WORD(*num_faces_ptr);
        num_faces++;
        assoc = (AsscEntry*)(num_faces_ptr + 1);
        TRACE("num faces %04x\n", num_faces);
        for(face = 0; face < num_faces; face++, assoc++)
        {
            Handle sfnt;
            ResType sfnt_res = FT_MAKE_TAG('s','f','n','t');
            unsigned short size, font_id;
            char *output;

            size = GET_BE_WORD(assoc->fontSize);
            font_id = GET_BE_WORD(assoc->fontID);
            if(size != 0)
            {
                TRACE("skipping id %04x because it's not scalable (fixed size %d)\n", font_id, size);
                continue;
            }

            TRACE("trying to load sfnt id %04x\n", font_id);
            sfnt = GetResource(sfnt_res, font_id);
            if(!sfnt)
            {
                TRACE("can't get sfnt resource %04x\n", font_id);
                continue;
            }

            output = HeapAlloc(GetProcessHeap(), 0, output_len);
            if(output)
            {
                int fd;

                sprintf(output, "%s/%s_%04x.ttf", out_dir, filename, font_id);

                fd = open(output, O_CREAT | O_EXCL | O_WRONLY, 0600);
                if(fd != -1 || errno == EEXIST)
                {
                    if(fd != -1)
                    {
                        unsigned char *sfnt_data;

                        HLock(sfnt);
                        sfnt_data = *(unsigned char**)sfnt;
                        write(fd, sfnt_data, GetHandleSize(sfnt));
                        HUnlock(sfnt);
                        close(fd);
                    }
                    if(ret.size >= ret.max_size - 1) /* Always want the last element to be NULL */
                    {
                        ret.max_size *= 2;
                        ret.array = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ret.array, ret.max_size * sizeof(*ret.array));
                    }
                    ret.array[ret.size++] = output;
                }
                else
                {
                    WARN("unable to create %s\n", output);
                    HeapFree(GetProcessHeap(), 0, output);
                }
            }
            ReleaseResource(sfnt);
        }
        HUnlock(fond);
        ReleaseResource(fond);
        idx++;
    }
    CloseResFile(res_ref);

    return ret.array;
}

#endif /* HAVE_CARBON_CARBON_H */

static inline BOOL is_win9x(void)
{
    return GetVersion() & 0x80000000;
}
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
        /* Use the >= 2.2.0 function if available */
        if (pFT_Get_TrueType_Engine_Type)
        {
            FT_TrueTypeEngineType type = pFT_Get_TrueType_Engine_Type(library);
            enabled = (type == FT_TRUETYPE_ENGINE_TYPE_PATENTED);
        }
        else enabled = FALSE;
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
#ifdef FT_LCD_FILTER_H
        else if (pFT_Library_SetLcdFilter &&
                 pFT_Library_SetLcdFilter( NULL, 0 ) != FT_Err_Unimplemented_Feature)
            enabled = TRUE;
#endif
        else enabled = FALSE;

        TRACE("subpixel rendering is %senabled\n", enabled ? "" : "NOT ");
    }
    return enabled;
}


static const struct list *get_face_list_from_family(const Family *family)
{
    if (!list_empty(&family->faces))
        return &family->faces;
    else
        return family->replacement;
}

static Face *find_face_from_filename(const WCHAR *file_name, const WCHAR *face_name)
{
    Family *family;
    Face *face;
    const WCHAR *file;

    TRACE("looking for file %s name %s\n", debugstr_w(file_name), debugstr_w(face_name));

    LIST_FOR_EACH_ENTRY(family, &font_list, Family, entry)
    {
        const struct list *face_list;
        if (face_name && strncmpiW( face_name, family->family_name, LF_FACESIZE - 1 )) continue;
        face_list = get_face_list_from_family(family);
        LIST_FOR_EACH_ENTRY(face, face_list, Face, entry)
        {
            if (!face->file)
                continue;
            file = strrchrW(face->file, '\\');
            if(!file)
                file = face->file;
            else
                file++;
            if(strcmpiW(file, file_name)) continue;
            face->refcount++;
            return face;
	}
    }
    return NULL;
}

static Family *find_family_from_name(const WCHAR *name)
{
    Family *family;

    LIST_FOR_EACH_ENTRY(family, &font_list, Family, entry)
        if (!strncmpiW( family->family_name, name, LF_FACESIZE - 1 )) return family;

    return NULL;
}

static Family *find_family_from_any_name(const WCHAR *name)
{
    Family *family;

    LIST_FOR_EACH_ENTRY(family, &font_list, Family, entry)
    {
        if (!strncmpiW( family->family_name, name, LF_FACESIZE - 1 )) return family;
        if (!strncmpiW( family->second_name, name, LF_FACESIZE - 1 )) return family;
    }

    return NULL;
}

static void DumpSubstList(void)
{
    FontSubst *psub;

    LIST_FOR_EACH_ENTRY(psub, &font_subst_list, FontSubst, entry)
    {
        if(psub->from.charset != -1 || psub->to.charset != -1)
	    TRACE("%s:%d -> %s:%d\n", debugstr_w(psub->from.name),
	      psub->from.charset, debugstr_w(psub->to.name), psub->to.charset);
	else
	    TRACE("%s -> %s\n", debugstr_w(psub->from.name),
		  debugstr_w(psub->to.name));
    }
}

static LPWSTR strdupW(LPCWSTR p)
{
    LPWSTR ret;
    DWORD len = (strlenW(p) + 1) * sizeof(WCHAR);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(ret, p, len);
    return ret;
}

static FontSubst *get_font_subst(const struct list *subst_list, const WCHAR *from_name,
                                 INT from_charset)
{
    FontSubst *element;

    LIST_FOR_EACH_ENTRY(element, subst_list, FontSubst, entry)
    {
        if(!strcmpiW(element->from.name, from_name) &&
           (element->from.charset == from_charset ||
            element->from.charset == -1))
            return element;
    }

    return NULL;
}

#define ADD_FONT_SUBST_FORCE  1

static BOOL add_font_subst(struct list *subst_list, FontSubst *subst, INT flags)
{
    FontSubst *from_exist, *to_exist;

    from_exist = get_font_subst(subst_list, subst->from.name, subst->from.charset);

    if(from_exist && (flags & ADD_FONT_SUBST_FORCE))
    {
        list_remove(&from_exist->entry);
        HeapFree(GetProcessHeap(), 0, from_exist->from.name);
        HeapFree(GetProcessHeap(), 0, from_exist->to.name);
        HeapFree(GetProcessHeap(), 0, from_exist);
        from_exist = NULL;
    }

    if(!from_exist)
    {
        to_exist = get_font_subst(subst_list, subst->to.name, subst->to.charset);

        if(to_exist)
        {
            HeapFree(GetProcessHeap(), 0, subst->to.name);
            subst->to.name = strdupW(to_exist->to.name);
        }
            
        list_add_tail(subst_list, &subst->entry);

        return TRUE;
    }

    HeapFree(GetProcessHeap(), 0, subst->from.name);
    HeapFree(GetProcessHeap(), 0, subst->to.name);
    HeapFree(GetProcessHeap(), 0, subst);
    return FALSE;
}

static WCHAR *towstr(UINT cp, const char *str)
{
    int len;
    WCHAR *wstr;

    len = MultiByteToWideChar(cp, 0, str, -1, NULL, 0);
    wstr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(cp, 0, str, -1, wstr, len);
    return wstr;
}

static void split_subst_info(NameCs *nc, LPSTR str)
{
    CHAR *p = strrchr(str, ',');

    nc->charset = -1;
    if(p && *(p+1)) {
        nc->charset = strtol(p+1, NULL, 10);
	*p = '\0';
    }
    nc->name = towstr(CP_ACP, str);
}

static void LoadSubstList(void)
{
    FontSubst *psub;
    HKEY hkey;
    DWORD valuelen, datalen, i = 0, type, dlen, vlen;
    LPSTR value;
    LPVOID data;

    if(RegOpenKeyA(HKEY_LOCAL_MACHINE,
		   "Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes",
		   &hkey) == ERROR_SUCCESS) {

        RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			 &valuelen, &datalen, NULL, NULL);

	valuelen++; /* returned value doesn't include room for '\0' */
	value = HeapAlloc(GetProcessHeap(), 0, valuelen * sizeof(CHAR));
	data = HeapAlloc(GetProcessHeap(), 0, datalen);

	dlen = datalen;
	vlen = valuelen;
	while(RegEnumValueA(hkey, i++, value, &vlen, NULL, &type, data,
			    &dlen) == ERROR_SUCCESS) {
	    TRACE("Got %s=%s\n", debugstr_a(value), debugstr_a(data));

	    psub = HeapAlloc(GetProcessHeap(), 0, sizeof(*psub));
	    split_subst_info(&psub->from, value);
	    split_subst_info(&psub->to, data);

	    /* Win 2000 doesn't allow mapping between different charsets
	       or mapping of DEFAULT_CHARSET */
	    if ((psub->from.charset && psub->to.charset != psub->from.charset) ||
	       psub->to.charset == DEFAULT_CHARSET) {
	        HeapFree(GetProcessHeap(), 0, psub->to.name);
		HeapFree(GetProcessHeap(), 0, psub->from.name);
		HeapFree(GetProcessHeap(), 0, psub);
	    } else {
	        add_font_subst(&font_subst_list, psub, 0);
	    }
	    /* reset dlen and vlen */
	    dlen = datalen;
	    vlen = valuelen;
	}
	HeapFree(GetProcessHeap(), 0, data);
	HeapFree(GetProcessHeap(), 0, value);
	RegCloseKey(hkey);
    }
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
    MAKELANGID(LANG_MALAGASY,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_MALAGASY */
    MAKELANGID(LANG_ESPERANTO,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_ESPERANTO */
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
    MAKELANGID(LANG_MANX_GAELIC,SUBLANG_DEFAULT),            /* TT_MAC_LANGID_MANX_GAELIC */
    MAKELANGID(LANG_IRISH,SUBLANG_IRISH_IRELAND),            /* TT_MAC_LANGID_IRISH_GAELIC */
    0,                                                       /* TT_MAC_LANGID_TONGAN */
    0,                                                       /* TT_MAC_LANGID_GREEK_POLYTONIC */
    MAKELANGID(LANG_GREENLANDIC,SUBLANG_DEFAULT),            /* TT_MAC_LANGID_GREELANDIC */
    MAKELANGID(LANG_AZERI,SUBLANG_AZERI_LATIN),              /* TT_MAC_LANGID_AZERBAIJANI_ROMAN_SCRIPT */
};

static inline WORD get_mac_code_page( const FT_SfntName *name )
{
    if (name->encoding_id == TT_MAC_ID_SIMPLIFIED_CHINESE) return 10008;  /* special case */
    return 10000 + name->encoding_id;
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
        if (!IsValidCodePage( get_mac_code_page( name ))) return 0;
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
    WORD codepage;
    int i;

    switch (name->platform_id)
    {
    case TT_PLATFORM_APPLE_UNICODE:
    case TT_PLATFORM_MICROSOFT:
        ret = HeapAlloc( GetProcessHeap(), 0, name->string_len + sizeof(WCHAR) );
        for (i = 0; i < name->string_len / 2; i++)
            ret[i] = (name->string[i * 2] << 8) | name->string[i * 2 + 1];
        ret[i] = 0;
        return ret;
    case TT_PLATFORM_MACINTOSH:
        codepage = get_mac_code_page( name );
        i = MultiByteToWideChar( codepage, 0, (char *)name->string, name->string_len, NULL, 0 );
        ret = HeapAlloc( GetProcessHeap(), 0, (i + 1) * sizeof(WCHAR) );
        MultiByteToWideChar( codepage, 0, (char *)name->string, name->string_len, ret, i );
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

    return towstr( CP_ACP, ft_face->family_name );
}

static WCHAR *ft_face_get_style_name( FT_Face ft_face, LANGID langid )
{
    WCHAR *style_name;

    if ((style_name = get_face_name( ft_face, TT_NAME_ID_FONT_SUBFAMILY, langid )))
        return style_name;

    return towstr( CP_ACP, ft_face->style_name );
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

    length = strlenW( full_name ) + strlenW( space_w ) + strlenW( style_name ) + 1;
    full_name = HeapReAlloc( GetProcessHeap(), 0, full_name, length * sizeof(WCHAR) );

    strcatW( full_name, space_w );
    strcatW( full_name, style_name );
    HeapFree( GetProcessHeap(), 0, style_name );

    WARN( "full name not found, using %s instead\n", debugstr_w(full_name) );
    return full_name;
}

static inline BOOL faces_equal( const Face *f1, const Face *f2 )
{
    if (strcmpiW( f1->full_name, f2->full_name )) return FALSE;
    if (f1->scalable) return TRUE;
    if (f1->size.y_ppem != f2->size.y_ppem) return FALSE;
    return !memcmp( &f1->fs, &f2->fs, sizeof(f1->fs) );
}

static void release_family( Family *family )
{
    if (--family->refcount) return;
    assert( list_empty( &family->faces ));
    list_remove( &family->entry );
    HeapFree( GetProcessHeap(), 0, family );
}

static void release_face( Face *face )
{
    if (--face->refcount) return;
    if (face->family)
    {
        if (face->flags & ADDFONT_ADD_TO_CACHE) remove_face_from_cache( face );
        list_remove( &face->entry );
        release_family( face->family );
    }
    HeapFree( GetProcessHeap(), 0, face->file );
    HeapFree( GetProcessHeap(), 0, face->style_name );
    HeapFree( GetProcessHeap(), 0, face->full_name );
    HeapFree( GetProcessHeap(), 0, face->cached_enum_data );
    HeapFree( GetProcessHeap(), 0, face );
}

static inline int style_order(const Face *face)
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
        WARN( "Don't know how to order face %s with flags 0x%08x\n", debugstr_w(face->full_name), face->ntmFlags );
        return 9999;
    }
}

static BOOL insert_face_in_family_list( Face *face, Family *family )
{
    Face *cursor;

    LIST_FOR_EACH_ENTRY( cursor, &family->faces, Face, entry )
    {
        if (faces_equal( face, cursor ))
        {
            TRACE( "Already loaded face %s in family %s, original version %lx, new version %lx\n",
                   debugstr_w(face->full_name), debugstr_w(family->family_name),
                   cursor->font_version, face->font_version );

            if (face->file && !strcmpiW( face->file, cursor->file ))
            {
                cursor->refcount++;
                TRACE("Font %s already in list, refcount now %d\n",
                      debugstr_w(face->file), cursor->refcount);
                return FALSE;
            }
            if (face->font_version <= cursor->font_version)
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
                release_face( cursor );
                return TRUE;
            }
        }

        if (style_order( face ) < style_order( cursor )) break;
    }

    TRACE( "Adding face %s in family %s from %s\n", debugstr_w(face->full_name),
           debugstr_w(family->family_name), debugstr_w(face->file) );
    list_add_before( &cursor->entry, &face->entry );
    face->family = family;
    family->refcount++;
    face->refcount++;
    return TRUE;
}

/****************************************************************
 * NB This function stores the ptrs to the strings to save copying.
 * Don't free them after calling.
 */
static Family *create_family( WCHAR *family_name, WCHAR *second_name )
{
    Family * const family = HeapAlloc( GetProcessHeap(), 0, sizeof(*family) );
    family->refcount = 1;
    lstrcpynW( family->family_name, family_name, LF_FACESIZE );
    if (second_name) lstrcpynW( family->second_name, second_name, LF_FACESIZE );
    else family->second_name[0] = 0;
    list_init( &family->faces );
    family->replacement = &family->faces;
    list_add_tail( &font_list, &family->entry );

    return family;
}

struct cached_face
{
    DWORD         index;
    DWORD         flags;
    DWORD         ntmflags;
    DWORD         version;
    DWORD         width;
    DWORD         height;
    DWORD         size;
    DWORD         x_ppem;
    DWORD         y_ppem;
    DWORD         internal_leading;
    FONTSIGNATURE fs;
    WCHAR         full_name[1];
    /* WCHAR      file_name[]; */
};

static void load_face(HKEY hkey_family, Family *family, void *buffer, DWORD buffer_size, BOOL scalable)
{
    DWORD type, size, needed, index = 0;
    Face *face;
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

            face = HeapAlloc(GetProcessHeap(), 0, sizeof(*face));
            face->cached_enum_data = NULL;
            face->family = NULL;
            face->refcount = 1;
            face->style_name = strdupW( name );
            face->face_index = cached->index;
            face->flags = cached->flags;
            face->ntmFlags = cached->ntmflags;
            face->font_version = cached->version;
            face->size.width = cached->width;
            face->size.height = cached->height;
            face->size.size = cached->size;
            face->size.x_ppem = cached->x_ppem;
            face->size.y_ppem = cached->y_ppem;
            face->size.internal_leading = cached->internal_leading;
            face->fs = cached->fs;
            face->full_name = strdupW( cached->full_name );
            face->file = strdupW( cached->full_name + strlenW(cached->full_name) + 1 );
            face->scalable = scalable;
            if (!face->scalable)
                TRACE("Adding bitmap size h %d w %d size %ld x_ppem %ld y_ppem %ld\n",
                      face->size.height, face->size.width, face->size.size >> 6,
                      face->size.x_ppem >> 6, face->size.y_ppem >> 6);

            TRACE("fsCsb = %08x %08x/%08x %08x %08x %08x\n",
                  face->fs.fsCsb[0], face->fs.fsCsb[1],
                  face->fs.fsUsb[0], face->fs.fsUsb[1],
                  face->fs.fsUsb[2], face->fs.fsUsb[3]);

            if (insert_face_in_family_list(face, family))
                TRACE( "Added face %s to family %s\n", debugstr_w(face->full_name), debugstr_w(family->family_name) );

            release_face( face );
        }
        size = sizeof(name);
        needed = buffer_size - sizeof(DWORD);
    }

    /* load bitmap strikes */

    index = 0;
    needed = buffer_size;
    while (!RegEnumKeyExW(hkey_family, index++, buffer, &needed, NULL, NULL, NULL, NULL))
    {
        if (!RegOpenKeyExW(hkey_family, buffer, 0, KEY_ALL_ACCESS, &hkey_strike))
        {
            load_face(hkey_strike, family, buffer, buffer_size, FALSE);
            RegCloseKey(hkey_strike);
        }
        needed = buffer_size;
    }
}

/* move vertical fonts after their horizontal counterpart */
/* assumes that font_list is already sorted by family name */
static void reorder_vertical_fonts(void)
{
    Family *family, *next, *vert_family;
    struct list *ptr, *vptr;
    struct list vertical_families = LIST_INIT( vertical_families );

    LIST_FOR_EACH_ENTRY_SAFE( family, next, &font_list, Family, entry )
    {
        if (family->family_name[0] != '@') continue;
        list_remove( &family->entry );
        list_add_tail( &vertical_families, &family->entry );
    }

    ptr = list_head( &font_list );
    vptr = list_head( &vertical_families );
    while (ptr && vptr)
    {
        family = LIST_ENTRY( ptr, Family, entry );
        vert_family = LIST_ENTRY( vptr, Family, entry );
        if (strcmpiW( family->family_name, vert_family->family_name + 1 ) > 0)
        {
            list_remove( vptr );
            list_add_before( ptr, vptr );
            vptr = list_head( &vertical_families );
        }
        else ptr = list_next( &font_list, ptr );
    }
    list_move_tail( &font_list, &vertical_families );
}

static void load_font_list_from_cache(HKEY hkey_font_cache)
{
    DWORD size, family_index = 0;
    Family *family;
    HKEY hkey_family;
    WCHAR buffer[4096];

    size = sizeof(buffer);
    while (!RegEnumKeyExW(hkey_font_cache, family_index++, buffer, &size, NULL, NULL, NULL, NULL))
    {
        WCHAR *second_name = NULL;
        WCHAR *family_name = strdupW( buffer );

        RegOpenKeyExW(hkey_font_cache, family_name, 0, KEY_ALL_ACCESS, &hkey_family);
        TRACE("opened family key %s\n", debugstr_w(family_name));
        size = sizeof(buffer);
        if (!RegQueryValueExW( hkey_family, NULL, NULL, NULL, (BYTE *)buffer, &size ))
            second_name = strdupW( buffer );

        family = create_family( family_name, second_name );

        if (second_name)
        {
            FontSubst *subst = HeapAlloc(GetProcessHeap(), 0, sizeof(*subst));
            subst->from.name = strdupW( second_name );
            subst->from.charset = -1;
            subst->to.name = strdupW(family_name);
            subst->to.charset = -1;
            add_font_subst(&font_subst_list, subst, 0);
        }

        load_face(hkey_family, family, buffer, sizeof(buffer), TRUE);

        HeapFree( GetProcessHeap(), 0, family_name );
        HeapFree( GetProcessHeap(), 0, second_name );

        RegCloseKey(hkey_family);
        release_family( family );
        size = sizeof(buffer);
    }

    reorder_vertical_fonts();
}

static LONG create_font_cache_key(HKEY *hkey, DWORD *disposition)
{
    LONG ret;
    HKEY hkey_wine_fonts;

    /* We don't want to create the fonts key as volatile, so open this first */
    ret = RegCreateKeyExW(HKEY_CURRENT_USER, wine_fonts_key, 0, NULL, 0,
                          KEY_ALL_ACCESS, NULL, &hkey_wine_fonts, NULL);
    if(ret != ERROR_SUCCESS)
    {
        WARN("Can't create %s\n", debugstr_w(wine_fonts_key));
        return ret;
    }

    ret = RegCreateKeyExW(hkey_wine_fonts, wine_fonts_cache_key, 0, NULL, REG_OPTION_VOLATILE,
                          KEY_ALL_ACCESS, NULL, hkey, disposition);
    RegCloseKey(hkey_wine_fonts);
    return ret;
}

static void add_face_to_cache(Face *face)
{
    HKEY hkey_family, hkey_face;
    DWORD len, buffer[1024];
    struct cached_face *cached = (struct cached_face *)buffer;

    RegCreateKeyExW( hkey_font_cache, face->family->family_name, 0, NULL, REG_OPTION_VOLATILE,
                     KEY_ALL_ACCESS, NULL, &hkey_family, NULL );
    if (face->family->second_name[0])
        RegSetValueExW( hkey_family, NULL, 0, REG_SZ, (BYTE *)face->family->second_name,
                        (strlenW( face->family->second_name ) + 1) * sizeof(WCHAR) );

    if (!face->scalable)
    {
        static const WCHAR fmtW[] = {'%','d',0};
        WCHAR name[10];

        sprintfW( name, fmtW, face->size.y_ppem );
        RegCreateKeyExW( hkey_family, name, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
                         NULL, &hkey_face, NULL);
    }
    else hkey_face = hkey_family;

    memset( cached, 0, sizeof(*cached) );
    cached->index = face->face_index;
    cached->flags = face->flags;
    cached->ntmflags = face->ntmFlags;
    cached->version = face->font_version;
    cached->fs = face->fs;
    if (!face->scalable)
    {
        cached->width = face->size.width;
        cached->height = face->size.height;
        cached->size = face->size.size;
        cached->x_ppem = face->size.x_ppem;
        cached->y_ppem = face->size.y_ppem;
        cached->internal_leading = face->size.internal_leading;
    }
    strcpyW( cached->full_name, face->full_name );
    len = strlenW( face->full_name ) + 1;
    strcpyW( cached->full_name + len, face->file );
    len += strlenW( face->file ) + 1;

    RegSetValueExW( hkey_face, face->style_name, 0, REG_BINARY, (BYTE *)cached,
                    offsetof( struct cached_face, full_name[len] ));

    if (hkey_face != hkey_family) RegCloseKey(hkey_face);
    RegCloseKey(hkey_family);
}

static void remove_face_from_cache( Face *face )
{
    HKEY hkey_family;

    RegOpenKeyExW( hkey_font_cache, face->family->family_name, 0, KEY_ALL_ACCESS, &hkey_family );

    if (face->scalable)
    {
        RegDeleteValueW( hkey_family, face->style_name );
    }
    else
    {
        static const WCHAR fmtW[] = {'%','d',0};
        WCHAR name[10];
        sprintfW( name, fmtW, face->size.y_ppem );
        RegDeleteKeyW( hkey_family, name );
    }
    RegCloseKey(hkey_family);
}

static WCHAR *get_vertical_name( WCHAR *name )
{
    SIZE_T length;
    if (!name) return NULL;
    if (name[0] == '@') return name;

    length = strlenW( name ) + 1;
    name = HeapReAlloc( GetProcessHeap(), 0, name, (length + 1) * sizeof(WCHAR) );
    memmove( name + 1, name, length * sizeof(WCHAR) );
    name[0] = '@';
    return name;
}

static Family *get_family( FT_Face ft_face, BOOL vertical )
{
    Family *family;
    WCHAR *family_name, *second_name;

    family_name = ft_face_get_family_name( ft_face, GetSystemDefaultLCID() );
    second_name = ft_face_get_family_name( ft_face, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT) );

    /* try to find another secondary name, preferring the lowest langids */
    if (!strcmpiW( family_name, second_name ))
    {
        HeapFree( GetProcessHeap(), 0, second_name );
        second_name = ft_face_get_family_name( ft_face, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );
    }

    if (!strcmpiW( family_name, second_name ))
    {
        HeapFree( GetProcessHeap(), 0, second_name );
        second_name = NULL;
    }

    if (vertical)
    {
        family_name = get_vertical_name( family_name );
        second_name = get_vertical_name( second_name );
    }

    if ((family = find_family_from_name( family_name ))) family->refcount++;
    else if ((family = create_family( family_name, second_name )) && second_name)
    {
        FontSubst *subst = HeapAlloc( GetProcessHeap(), 0, sizeof(*subst) );
        subst->from.name = strdupW( second_name );
        subst->from.charset = -1;
        subst->to.name = strdupW( family_name );
        subst->to.charset = -1;
        add_font_subst( &font_subst_list, subst, 0 );
    }

    HeapFree( GetProcessHeap(), 0, family_name );
    HeapFree( GetProcessHeap(), 0, second_name );

    return family;
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

static inline void get_bitmap_size( FT_Face ft_face, Bitmap_Size *face_size )
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
    CHARSETINFO csi;
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
            if (TranslateCharsetInfo( (DWORD*)(UINT_PTR)winfnt_header.charset, &csi, TCI_SRCCHARSET ))
                *fs = csi.fs;
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

static Face *create_face( FT_Face ft_face, FT_Long face_index, const WCHAR *filename,
                          void *font_data_ptr, DWORD font_data_size, DWORD flags )
{
    Face *face = HeapAlloc( GetProcessHeap(), 0, sizeof(*face) );

    face->refcount = 1;
    face->style_name = ft_face_get_style_name( ft_face, GetSystemDefaultLangID() );
    face->full_name = ft_face_get_full_name( ft_face, GetSystemDefaultLangID() );
    if (flags & ADDFONT_VERTICAL_FONT) face->full_name = get_vertical_name( face->full_name );

    if (filename)
    {
        face->file = strdupW( filename );
        face->font_data_ptr = NULL;
        face->font_data_size = 0;
    }
    else
    {
        face->file = NULL;
        face->font_data_ptr = font_data_ptr;
        face->font_data_size = font_data_size;
    }

    face->face_index = face_index;
    get_fontsig( ft_face, &face->fs );
    face->ntmFlags = get_ntm_flags( ft_face );
    face->font_version = get_font_version( ft_face );

    if (FT_IS_SCALABLE( ft_face ))
    {
        memset( &face->size, 0, sizeof(face->size) );
        face->scalable = TRUE;
    }
    else
    {
        get_bitmap_size( ft_face, &face->size );
        face->scalable = FALSE;
    }

    if (!HIWORD( flags )) flags |= ADDFONT_AA_FLAGS( default_aa_flags );
    face->flags  = flags;
    face->family = NULL;
    face->cached_enum_data = NULL;

    TRACE("fsCsb = %08x %08x/%08x %08x %08x %08x\n",
          face->fs.fsCsb[0], face->fs.fsCsb[1],
          face->fs.fsUsb[0], face->fs.fsUsb[1],
          face->fs.fsUsb[2], face->fs.fsUsb[3]);

    return face;
}

static void AddFaceToList(FT_Face ft_face, const WCHAR *file, void *font_data_ptr, DWORD font_data_size,
                          FT_Long face_index, DWORD flags )
{
    Face *face;
    Family *family;

    face = create_face( ft_face, face_index, file, font_data_ptr, font_data_size, flags );
    family = get_family( ft_face, flags & ADDFONT_VERTICAL_FONT );

    if (insert_face_in_family_list( face, family ))
    {
        if (flags & ADDFONT_ADD_TO_CACHE)
            add_face_to_cache( face );
        TRACE( "Added face %s to family %s\n", debugstr_w(face->full_name), debugstr_w(family->family_name) );
    }
    release_face( face );
    release_family( family );
}

static FT_Face new_ft_face( const char *file, void *font_data_ptr, DWORD font_data_size,
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

    /* There are too many bugs in FreeType < 2.1.9 for bitmap font support */
    if (!FT_IS_SCALABLE( ft_face ) && FT_SimpleVersion < FT_VERSION_VALUE(2, 1, 9))
    {
        WARN("FreeType version < 2.1.9, skipping bitmap font %s/%p\n", debugstr_a(file), font_data_ptr);
        goto fail;
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

static INT AddFontToList(const WCHAR *dos_name, const char *unix_name, void *font_data_ptr,
                         DWORD font_data_size, DWORD flags)
{
    FT_Face ft_face;
    FT_Long face_index = 0, num_faces;
    INT ret = 0;
    WCHAR *filename = NULL;

    /* we always load external fonts from files - otherwise we would get a crash in update_reg_entries */
    assert(unix_name || !(flags & ADDFONT_EXTERNAL_FONT));

#ifdef HAVE_CARBON_CARBON_H
    if(unix_name)
    {
        char **mac_list = expand_mac_font(unix_name);
        if(mac_list)
        {
            BOOL had_one = FALSE;
            char **cursor;
            for(cursor = mac_list; *cursor; cursor++)
            {
                had_one = TRUE;
                AddFontToList(NULL, *cursor, NULL, 0, flags);
                HeapFree(GetProcessHeap(), 0, *cursor);
            }
            HeapFree(GetProcessHeap(), 0, mac_list);
            if(had_one)
                return 1;
        }
    }
#endif /* HAVE_CARBON_CARBON_H */

    if (!dos_name && unix_name) dos_name = filename = wine_get_dos_file_name( unix_name );

    do {
        FONTSIGNATURE fs;

        ft_face = new_ft_face( unix_name, font_data_ptr, font_data_size, face_index, flags & ADDFONT_ALLOW_BITMAP );
        if (!ft_face) break;

        if(ft_face->family_name[0] == '.') /* Ignore fonts with names beginning with a dot */
        {
            TRACE("Ignoring %s since its family name begins with a dot\n", debugstr_a(unix_name));
            pFT_Done_Face(ft_face);
            break;
        }

        AddFaceToList(ft_face, dos_name, font_data_ptr, font_data_size, face_index, flags);
        ++ret;

        get_fontsig(ft_face, &fs);
        if (fs.fsCsb[0] & FS_DBCS_MASK)
        {
            AddFaceToList(ft_face, dos_name, font_data_ptr, font_data_size, face_index,
                          flags | ADDFONT_VERTICAL_FONT);
            ++ret;
        }

	num_faces = ft_face->num_faces;
	pFT_Done_Face(ft_face);
    } while(num_faces > ++face_index);
    HeapFree( GetProcessHeap(), 0, filename );
    return ret;
}

static int add_font_resource( const WCHAR *file, DWORD flags )
{
    int ret = 0;
    char *unixname = wine_get_unix_file_name( file );

    if (unixname)
    {
        ret = AddFontToList( file, unixname, NULL, 0, flags );
        HeapFree( GetProcessHeap(), 0, unixname );
    }
    return ret;
}

static int remove_font_resource( const WCHAR *file, DWORD flags )
{
    Family *family, *family_next;
    Face *face, *face_next;
    int count = 0;

    LIST_FOR_EACH_ENTRY_SAFE( family, family_next, &font_list, Family, entry )
    {
        family->refcount++;
        LIST_FOR_EACH_ENTRY_SAFE( face, face_next, &family->faces, Face, entry )
        {
            if (!face->file) continue;
            if (LOWORD(face->flags) != LOWORD(flags)) continue;
            if (!strcmpiW( face->file, file ))
            {
                TRACE( "removing matching face %s refcount %d\n", debugstr_w(face->file), face->refcount );
                release_face( face );
                count++;
            }
	}
        release_family( family );
    }
    return count;
}

static void DumpFontList(void)
{
    Family *family;
    Face *face;

    LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
        TRACE( "Family: %s\n", debugstr_w(family->family_name) );
        LIST_FOR_EACH_ENTRY( face, &family->faces, Face, entry ) {
            TRACE( "\t%s\t%s\t%08x", debugstr_w(face->style_name), debugstr_w(face->full_name),
                   face->fs.fsCsb[0] );
            if(!face->scalable)
                TRACE(" %d", face->size.height);
            TRACE("\n");
	}
    }
}

static BOOL map_vertical_font_family(const WCHAR *orig, const WCHAR *repl, const Family *family)
{
    Face *face;
    BOOL ret = FALSE;
    WCHAR *at_orig, *at_repl = NULL;

    face = LIST_ENTRY(list_head(&family->faces), Face, entry);
    if (!face || !(face->fs.fsCsb[0] & FS_DBCS_MASK))
        return FALSE;

    at_orig = get_vertical_name( strdupW( orig ) );
    if (at_orig && !find_family_from_any_name(at_orig))
    {
        at_repl = get_vertical_name( strdupW( repl ) );
        if (at_repl)
            ret = map_font_family(at_orig, at_repl);
    }

    HeapFree(GetProcessHeap(), 0, at_orig);
    HeapFree(GetProcessHeap(), 0, at_repl);

    return ret;
}

static BOOL map_font_family(const WCHAR *orig, const WCHAR *repl)
{
    Family *family = find_family_from_any_name(repl);
    if (family != NULL)
    {
        Family *new_family = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_family));
        if (new_family != NULL)
        {
            TRACE("mapping %s to %s\n", debugstr_w(repl), debugstr_w(orig));
            lstrcpynW( new_family->family_name, orig, LF_FACESIZE );
            new_family->second_name[0] = 0;
            list_init(&new_family->faces);
            new_family->replacement = &family->faces;
            list_add_tail(&font_list, &new_family->entry);

            if (repl[0] != '@')
                map_vertical_font_family(orig, repl, family);

            return TRUE;
        }
    }
    TRACE("%s is not available. Skip this replacement.\n", debugstr_w(repl));
    return FALSE;
}

/***********************************************************
 * The replacement list is a way to map an entire font
 * family onto another family.  For example adding
 *
 * [HKCU\Software\Wine\Fonts\Replacements]
 * "Wingdings"="Winedings"
 *
 * would enumerate the Winedings font both as Winedings and
 * Wingdings.  However if a real Wingdings font is present the
 * replacement does not take place.
 * 
 */
static void LoadReplaceList(void)
{
    HKEY hkey;
    DWORD valuelen, datalen, i = 0, type, dlen, vlen;
    LPWSTR value;
    LPVOID data;

    /* @@ Wine registry key: HKCU\Software\Wine\Fonts\Replacements */
    if(RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Fonts\\Replacements", &hkey) == ERROR_SUCCESS)
    {
        RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			 &valuelen, &datalen, NULL, NULL);

	valuelen++; /* returned value doesn't include room for '\0' */
	value = HeapAlloc(GetProcessHeap(), 0, valuelen * sizeof(WCHAR));
	data = HeapAlloc(GetProcessHeap(), 0, datalen);

	dlen = datalen;
	vlen = valuelen;
        while(RegEnumValueW(hkey, i++, value, &vlen, NULL, &type, data, &dlen) == ERROR_SUCCESS)
        {
            /* "NewName"="Oldname" */
            if(!find_family_from_any_name(value))
            {
                if (type == REG_MULTI_SZ)
                {
                    WCHAR *replace = data;
                    while(*replace)
                    {
                        if (map_font_family(value, replace))
                            break;
                        replace += strlenW(replace) + 1;
                    }
                }
                else if (type == REG_SZ)
                    map_font_family(value, data);
            }
            else
	        TRACE("%s is available. Skip this replacement.\n", debugstr_w(value));

	    /* reset dlen and vlen */
	    dlen = datalen;
	    vlen = valuelen;
	}
	HeapFree(GetProcessHeap(), 0, data);
	HeapFree(GetProcessHeap(), 0, value);
	RegCloseKey(hkey);
    }
}

static const WCHAR *font_links_list[] =
{
    Lucida_Sans_Unicode,
    Microsoft_Sans_Serif,
    Tahoma
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
    { Tahoma, /* FIXME unverified ordering */
      { MS_UI_Gothic, SimSun, Gulim, PMingLiU, NULL }
    },
    /* Below lists are courtesy of
     * http://blogs.msdn.com/michkap/archive/2005/06/18/430507.aspx
     */
    /* Japanese */
    { MS_UI_Gothic,
      { MS_UI_Gothic, PMingLiU, SimSun, Gulim, NULL }
    },
    /* Chinese Simplified */
    { SimSun,
      { SimSun, PMingLiU, MS_UI_Gothic, Batang, NULL }
    },
    /* Korean */
    { Gulim,
      { Gulim, PMingLiU, MS_UI_Gothic, SimSun, NULL }
    },
    /* Chinese Traditional */
    { PMingLiU,
      { PMingLiU, SimSun, MS_UI_Gothic, Batang, NULL }
    }
};


static SYSTEM_LINKS *find_font_link(const WCHAR *name)
{
    SYSTEM_LINKS *font_link;

    LIST_FOR_EACH_ENTRY(font_link, &system_links, SYSTEM_LINKS, entry)
    {
        if(!strncmpiW(font_link->font_name, name, LF_FACESIZE - 1))
            return font_link;
    }

    return NULL;
}

static void populate_system_links(const WCHAR *name, const WCHAR *const *values)
{
    const WCHAR *value;
    int i;
    FontSubst *psub;
    Family *family;
    Face *face;
    const WCHAR *file;

    if (values)
    {
        SYSTEM_LINKS *font_link;

        psub = get_font_subst(&font_subst_list, name, -1);
        /* Don't store fonts that are only substitutes for other fonts */
        if(psub)
        {
            TRACE("%s: Internal SystemLink entry for substituted font, ignoring\n", debugstr_w(name));
            return;
        }

        font_link = find_font_link(name);
        if (font_link == NULL)
        {
            font_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*font_link));
            font_link->font_name = strdupW(name);
            list_init(&font_link->links);
            list_add_tail(&system_links, &font_link->entry);
        }

        memset(&font_link->fs, 0, sizeof font_link->fs);
        for (i = 0; values[i] != NULL; i++)
        {
            const struct list *face_list;
            CHILD_FONT *child_font;

            value = values[i];
            if (!strcmpiW(name,value))
                continue;
            psub = get_font_subst(&font_subst_list, value, -1);
            if(psub)
                value = psub->to.name;
            family = find_family_from_name(value);
            if (!family)
                continue;
            file = NULL;
            /* Use first extant filename for this Family */
            face_list = get_face_list_from_family(family);
            LIST_FOR_EACH_ENTRY(face, face_list, Face, entry)
            {
                if (!face->file)
                    continue;
                file = strrchrW(face->file, '\\');
                if (!file)
                    file = face->file;
                else
                    file++;
                break;
            }
            if (!file)
                continue;
            face = find_face_from_filename(file, value);
            if(!face)
            {
                TRACE("Unable to find file %s face name %s\n", debugstr_w(file), debugstr_w(value));
                continue;
            }

            child_font = HeapAlloc(GetProcessHeap(), 0, sizeof(*child_font));
            child_font->face = face;
            child_font->font = NULL;
            font_link->fs.fsCsb[0] |= face->fs.fsCsb[0];
            font_link->fs.fsCsb[1] |= face->fs.fsCsb[1];
            TRACE("Adding file %s index %ld\n", debugstr_w(child_font->face->file),
                  child_font->face->face_index);
            list_add_tail(&font_link->links, &child_font->entry);

            TRACE("added internal SystemLink for %s to %s in %s\n", debugstr_w(name), debugstr_w(value),debugstr_w(file));
        }
    }
}


/*************************************************************
 * init_system_links
 */
static void init_system_links(void)
{
    HKEY hkey;
    DWORD type, max_val, max_data, val_len, data_len, index;
    WCHAR *value, *data;
    WCHAR *entry, *next;
    SYSTEM_LINKS *font_link, *system_font_link;
    CHILD_FONT *child_font;
    static const WCHAR tahoma_ttf[] = {'t','a','h','o','m','a','.','t','t','f',0};
    static const WCHAR System[] = {'S','y','s','t','e','m',0};
    static const WCHAR MS_Shell_Dlg[] = {'M','S',' ','S','h','e','l','l',' ','D','l','g',0};
    Face *face;
    FontSubst *psub;
    UINT i, j;

    if(RegOpenKeyW(HKEY_LOCAL_MACHINE, system_link, &hkey) == ERROR_SUCCESS)
    {
        RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &max_val, &max_data, NULL, NULL);
        value = HeapAlloc(GetProcessHeap(), 0, (max_val + 1) * sizeof(WCHAR));
        data = HeapAlloc(GetProcessHeap(), 0, max_data);
        val_len = max_val + 1;
        data_len = max_data;
        index = 0;
        while(RegEnumValueW(hkey, index++, value, &val_len, NULL, &type, (LPBYTE)data, &data_len) == ERROR_SUCCESS)
        {
            psub = get_font_subst(&font_subst_list, value, -1);
            /* Don't store fonts that are only substitutes for other fonts */
            if(psub)
            {
                TRACE("%s: SystemLink entry for substituted font, ignoring\n", debugstr_w(value));
                goto next;
            }
            font_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*font_link));
            font_link->font_name = strdupW(value);
            memset(&font_link->fs, 0, sizeof font_link->fs);
            list_init(&font_link->links);
            for(entry = data; (char*)entry < (char*)data + data_len && *entry != 0; entry = next)
            {
                WCHAR *face_name;
                CHILD_FONT *child_font;

                TRACE("%s: %s\n", debugstr_w(value), debugstr_w(entry));

                next = entry + strlenW(entry) + 1;
                
                face_name = strchrW(entry, ',');
                if(face_name)
                {
                    *face_name++ = 0;
                    while(isspaceW(*face_name))
                        face_name++;

                    psub = get_font_subst(&font_subst_list, face_name, -1);
                    if(psub)
                        face_name = psub->to.name;
                }
                face = find_face_from_filename(entry, face_name);
                if(!face)
                {
                    TRACE("Unable to find file %s face name %s\n", debugstr_w(entry), debugstr_w(face_name));
                    continue;
                }

                child_font = HeapAlloc(GetProcessHeap(), 0, sizeof(*child_font));
                child_font->face = face;
                child_font->font = NULL;
                font_link->fs.fsCsb[0] |= face->fs.fsCsb[0];
                font_link->fs.fsCsb[1] |= face->fs.fsCsb[1];
                TRACE("Adding file %s index %ld\n",
                      debugstr_w(child_font->face->file), child_font->face->face_index);
                list_add_tail(&font_link->links, &child_font->entry);
            }
            list_add_tail(&system_links, &font_link->entry);
        next:
            val_len = max_val + 1;
            data_len = max_data;
        }

        HeapFree(GetProcessHeap(), 0, value);
        HeapFree(GetProcessHeap(), 0, data);
        RegCloseKey(hkey);
    }


    psub = get_font_subst(&font_subst_list, MS_Shell_Dlg, -1);
    if (!psub) {
        WARN("could not find FontSubstitute for MS Shell Dlg\n");
        goto skip_internal;
    }

    for (i = 0; i < ARRAY_SIZE(font_links_defaults_list); i++)
    {
        const FontSubst *psub2;
        psub2 = get_font_subst(&font_subst_list, font_links_defaults_list[i].shelldlg, -1);

        if ((!strcmpiW(font_links_defaults_list[i].shelldlg, psub->to.name) || (psub2 && !strcmpiW(psub2->to.name,psub->to.name))))
        {
            for (j = 0; j < ARRAY_SIZE(font_links_list); j++)
                populate_system_links(font_links_list[j], font_links_defaults_list[i].substitutes);

            if (!strcmpiW(psub->to.name, font_links_defaults_list[i].substitutes[0]))
                populate_system_links(psub->to.name, font_links_defaults_list[i].substitutes);
        }
        else if (strcmpiW(psub->to.name, font_links_defaults_list[i].substitutes[0]))
        {
            populate_system_links(font_links_defaults_list[i].substitutes[0], NULL);
        }
    }

skip_internal:

    /* Explicitly add an entry for the system font, this links to Tahoma and any links
       that Tahoma has */

    system_font_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*system_font_link));
    system_font_link->font_name = strdupW(System);
    memset(&system_font_link->fs, 0, sizeof system_font_link->fs);
    list_init(&system_font_link->links);    

    face = find_face_from_filename(tahoma_ttf, Tahoma);
    if(face)
    {
        child_font = HeapAlloc(GetProcessHeap(), 0, sizeof(*child_font));
        child_font->face = face;
        child_font->font = NULL;
        system_font_link->fs.fsCsb[0] |= face->fs.fsCsb[0];
        system_font_link->fs.fsCsb[1] |= face->fs.fsCsb[1];
        TRACE("Found Tahoma in %s index %ld\n",
              debugstr_w(child_font->face->file), child_font->face->face_index);
        list_add_tail(&system_font_link->links, &child_font->entry);
    }
    font_link = find_font_link(Tahoma);
    if (font_link != NULL)
    {
        CHILD_FONT *font_link_entry;
        LIST_FOR_EACH_ENTRY(font_link_entry, &font_link->links, CHILD_FONT, entry)
        {
            CHILD_FONT *new_child;
            new_child = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_child));
            new_child->face = font_link_entry->face;
            new_child->font = NULL;
            new_child->face->refcount++;
            system_font_link->fs.fsCsb[0] |= font_link_entry->face->fs.fsCsb[0];
            system_font_link->fs.fsCsb[1] |= font_link_entry->face->fs.fsCsb[1];
            list_add_tail(&system_font_link->links, &new_child->entry);
        }
    }
    list_add_tail(&system_links, &system_font_link->entry);
}

static BOOL ReadFontDir(const char *dirname, BOOL external_fonts)
{
    DIR *dir;
    struct dirent *dent;
    char path[MAX_PATH];

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

	sprintf(path, "%s/%s", dirname, dent->d_name);

	if(stat(path, &statbuf) == -1)
	{
	    WARN("Can't stat %s\n", debugstr_a(path));
	    continue;
	}
	if(S_ISDIR(statbuf.st_mode))
	    ReadFontDir(path, external_fonts);
	else
        {
            DWORD addfont_flags = ADDFONT_ADD_TO_CACHE;
            if(external_fonts) addfont_flags |= ADDFONT_EXTERNAL_FONT;
            AddFontToList(NULL, path, NULL, 0, addfont_flags);
        }
    }
    closedir(dir);
    return TRUE;
}

static void read_font_dir( const WCHAR *dirname, BOOL external_fonts )
{
    char *unixname = wine_get_unix_file_name( dirname );
    if (unixname)
    {
        ReadFontDir( unixname, external_fonts );
        HeapFree( GetProcessHeap(), 0, unixname );
    }
}

#ifdef SONAME_LIBFONTCONFIG

static BOOL fontconfig_enabled;

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

static void load_fontconfig_fonts(void)
{
    FcPattern *pat;
    FcFontSet *fontset;
    int i, len;
    char *file;
    const char *ext;

    if (!fontconfig_enabled) return;

    pat = pFcPatternCreate();
    if (!pat) return;

    fontset = pFcFontList(NULL, pat, NULL);
    if (!fontset)
    {
        pFcPatternDestroy(pat);
        return;
    }

    for(i = 0; i < fontset->nfont; i++) {
        FcBool scalable;
        DWORD aa_flags;

        if(pFcPatternGetString(fontset->fonts[i], FC_FILE, 0, (FcChar8**)&file) != FcResultMatch)
            continue;

        pFcConfigSubstitute( NULL, fontset->fonts[i], FcMatchFont );

        /* We're just interested in OT/TT fonts for now, so this hack just
           picks up the scalable fonts without extensions .pf[ab] to save time
           loading every other font */

        if(pFcPatternGetBool(fontset->fonts[i], FC_SCALABLE, 0, &scalable) == FcResultMatch && !scalable)
        {
            TRACE("not scalable\n");
            continue;
        }

        aa_flags = parse_aa_pattern( fontset->fonts[i] );
        TRACE("fontconfig: %s aa %x\n", file, aa_flags);

        len = strlen( file );
        if(len < 4) continue;
        ext = &file[ len - 3 ];
        if(_strnicmp(ext, "pfa", -1) && _strnicmp(ext, "pfb", -1))
            AddFontToList(NULL, file, NULL, 0,
                          ADDFONT_EXTERNAL_FONT | ADDFONT_ADD_TO_CACHE | ADDFONT_AA_FLAGS(aa_flags) );
    }
    pFcFontSetDestroy(fontset);
    pFcPatternDestroy(pat);
}

#elif defined(HAVE_CARBON_CARBON_H)

static void load_mac_font_callback(const void *value, void *context)
{
    CFStringRef pathStr = value;
    CFIndex len;
    char* path;

    len = CFStringGetMaximumSizeOfFileSystemRepresentation(pathStr);
    path = HeapAlloc(GetProcessHeap(), 0, len);
    if (path && CFStringGetFileSystemRepresentation(pathStr, path, len))
    {
        TRACE("font file %s\n", path);
        AddFontToList(NULL, path, NULL, 0, ADDFONT_EXTERNAL_FONT | ADDFONT_ADD_TO_CACHE);
    }
    HeapFree(GetProcessHeap(), 0, path);
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

#if defined(MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
        url = CTFontDescriptorCopyAttribute(desc, kCTFontURLAttribute);
#else
        /* CTFontDescriptor doesn't support kCTFontURLAttribute prior to 10.6, so
           we have to go CFFontDescriptor -> CTFont -> ATSFont -> FSRef -> CFURL. */
        {
            CTFontRef font;
            ATSFontRef atsFont;
            OSStatus status;
            FSRef fsref;

            font = CTFontCreateWithFontDescriptor(desc, 0, NULL);
            if (!font) continue;

            atsFont = CTFontGetPlatformFont(font, NULL);
            if (!atsFont)
            {
                CFRelease(font);
                continue;
            }

            status = ATSFontGetFileReference(atsFont, &fsref);
            CFRelease(font);
            if (status != noErr) continue;

            url = CFURLCreateFromFSRef(NULL, &fsref);
        }
#endif
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

static void get_font_dir( WCHAR *path )
{
    static const WCHAR slashW[] = {'\\',0};
    static const WCHAR fontsW[] = {'\\','f','o','n','t','s',0};
    static const WCHAR winedatadirW[] = {'W','I','N','E','D','A','T','A','D','I','R',0};
    static const WCHAR winebuilddirW[] = {'W','I','N','E','B','U','I','L','D','D','I','R',0};

    if (GetEnvironmentVariableW( winedatadirW, path, MAX_PATH ))
    {
        const char fontdir[] = WINE_FONT_DIR;
        strcatW( path, slashW );
        MultiByteToWideChar( CP_ACP, 0, fontdir, -1, path + strlenW(path), MAX_PATH - strlenW(path) );
    }
    else if (GetEnvironmentVariableW( winebuilddirW, path, MAX_PATH ))
    {
        strcatW( path, fontsW );
    }
    if (path[5] == ':') memmove( path, path + 4, (strlenW(path) - 3) * sizeof(WCHAR) );
    else path[1] = '\\';  /* change \??\ to \\?\ */
}

static void get_data_dir_path( LPCWSTR file, WCHAR *path )
{
    static const WCHAR slashW[] = {'\\','\0'};

    get_font_dir( path );
    strcatW( path, slashW );
    strcatW( path, file );
}

static void get_winfonts_dir_path(LPCWSTR file, WCHAR *path)
{
    static const WCHAR slashW[] = {'\\','\0'};

    GetWindowsDirectoryW(path, MAX_PATH);
    strcatW(path, fontsW);
    strcatW(path, slashW);
    strcatW(path, file);
}

static void load_system_fonts(void)
{
    HKEY hkey;
    WCHAR data[MAX_PATH], pathW[MAX_PATH];
    const WCHAR * const *value;
    DWORD dlen, type;

    if(RegOpenKeyW(HKEY_CURRENT_CONFIG, system_fonts_reg_key, &hkey) == ERROR_SUCCESS) {
        for(value = SystemFontValues; *value; value++) { 
            dlen = sizeof(data);
            if(RegQueryValueExW(hkey, *value, 0, &type, (void*)data, &dlen) == ERROR_SUCCESS &&
               type == REG_SZ) {
                get_winfonts_dir_path( data, pathW );
                if (!add_font_resource( pathW, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_TO_CACHE ))
                {
                    get_data_dir_path( data, pathW );
                    add_font_resource( pathW, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_TO_CACHE );
                }
            }
        }
        RegCloseKey(hkey);
    }
}

static WCHAR *get_full_path_name(const WCHAR *name)
{
    WCHAR *full_path;
    DWORD len;

    if (!(len = GetFullPathNameW(name, 0, NULL, NULL)))
    {
        ERR("GetFullPathNameW() failed, name %s.\n", debugstr_w(name));
        return NULL;
    }

    if (!(full_path = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*full_path))))
    {
        ERR("Could not get memory.\n");
        return NULL;
    }

    if (GetFullPathNameW(name, len, full_path, NULL) != len - 1)
    {
        ERR("Unexpected GetFullPathNameW() result, name %s.\n", debugstr_w(name));
        HeapFree(GetProcessHeap(), 0, full_path);
        return NULL;
    }

    return full_path;
}

/*************************************************************
 *
 * This adds registry entries for any externally loaded fonts
 * (fonts from fontconfig or FontDirs).  It also deletes entries
 * of no longer existing fonts.
 *
 */
static void update_reg_entries(void)
{
    HKEY winnt_key = 0, win9x_key = 0, external_key = 0;
    LPWSTR valueW;
    DWORD len;
    Family *family;
    Face *face;
    WCHAR *file, *path;
    static const WCHAR TrueType[] = {' ','(','T','r','u','e','T','y','p','e',')','\0'};

    if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, winnt_font_reg_key,
                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &winnt_key, NULL) != ERROR_SUCCESS) {
        ERR("Can't create Windows font reg key\n");
        goto end;
    }

    if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, win9x_font_reg_key,
                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &win9x_key, NULL) != ERROR_SUCCESS) {
        ERR("Can't create Windows font reg key\n");
        goto end;
    }

    if(RegCreateKeyExW(HKEY_CURRENT_USER, external_fonts_reg_key,
                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &external_key, NULL) != ERROR_SUCCESS) {
        ERR("Can't create external font reg key\n");
        goto end;
    }

    /* enumerate the fonts and add external ones to the two keys */

    LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
        LIST_FOR_EACH_ENTRY( face, &family->faces, Face, entry ) {
            if (!(face->flags & ADDFONT_EXTERNAL_FONT)) continue;

            len = strlenW( face->full_name ) + 1;
            if (face->scalable)
                len += ARRAY_SIZE(TrueType);

            valueW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            strcpyW( valueW, face->full_name );

            if (face->scalable)
                strcatW(valueW, TrueType);

            if ((path = get_full_path_name(face->file)))
            {
                file = path;
            }
            else if ((file = strrchrW(face->file, '\\')))
            {
                file++;
            }
            else
            {
                file = face->file;
            }

            len = strlenW(file) + 1;
            RegSetValueExW(winnt_key, valueW, 0, REG_SZ, (BYTE*)file, len * sizeof(WCHAR));
            RegSetValueExW(win9x_key, valueW, 0, REG_SZ, (BYTE*)file, len * sizeof(WCHAR));
            RegSetValueExW(external_key, valueW, 0, REG_SZ, (BYTE*)file, len * sizeof(WCHAR));

            HeapFree(GetProcessHeap(), 0, path);
            HeapFree(GetProcessHeap(), 0, valueW);
        }
    }
 end:
    if(external_key) RegCloseKey(external_key);
    if(win9x_key) RegCloseKey(win9x_key);
    if(winnt_key) RegCloseKey(winnt_key);
}

static void delete_external_font_keys(void)
{
    HKEY winnt_key = 0, win9x_key = 0, external_key = 0;
    DWORD dlen, plen, vlen, datalen, valuelen, i, type, path_type;
    LPWSTR valueW;
    LPVOID data;
    BYTE *path;

    if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, winnt_font_reg_key,
                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &winnt_key, NULL) != ERROR_SUCCESS) {
        ERR("Can't create Windows font reg key\n");
        goto end;
    }

    if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, win9x_font_reg_key,
                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &win9x_key, NULL) != ERROR_SUCCESS) {
        ERR("Can't create Windows font reg key\n");
        goto end;
    }

    if(RegCreateKeyW(HKEY_CURRENT_USER, external_fonts_reg_key, &external_key) != ERROR_SUCCESS) {
        ERR("Can't create external font reg key\n");
        goto end;
    }

    /* Delete all external fonts added last time */

    RegQueryInfoKeyW(external_key, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                     &valuelen, &datalen, NULL, NULL);
    valuelen++; /* returned value doesn't include room for '\0' */
    valueW = HeapAlloc(GetProcessHeap(), 0, valuelen * sizeof(WCHAR));
    data = HeapAlloc(GetProcessHeap(), 0, datalen);
    path = HeapAlloc(GetProcessHeap(), 0, datalen);

    dlen = datalen;
    vlen = valuelen;
    i = 0;
    while(RegEnumValueW(external_key, i++, valueW, &vlen, NULL, &type, data,
                        &dlen) == ERROR_SUCCESS) {
        plen = dlen;
        if (RegQueryValueExW(winnt_key, valueW, 0, &path_type, path, &plen) == ERROR_SUCCESS &&
            type == path_type && dlen == plen && !memcmp(data, path, plen))
            RegDeleteValueW(winnt_key, valueW);

        plen = dlen;
        if (RegQueryValueExW(win9x_key, valueW, 0, &path_type, path, &plen) == ERROR_SUCCESS &&
            type == path_type && dlen == plen && !memcmp(data, path, plen))
            RegDeleteValueW(win9x_key, valueW);

        /* reset dlen and vlen */
        dlen = datalen;
        vlen = valuelen;
    }
    HeapFree(GetProcessHeap(), 0, path);
    HeapFree(GetProcessHeap(), 0, data);
    HeapFree(GetProcessHeap(), 0, valueW);

    /* Delete the old external fonts key */
    RegCloseKey(external_key);
    RegDeleteKeyW(HKEY_CURRENT_USER, external_fonts_reg_key);

 end:
    if(win9x_key) RegCloseKey(win9x_key);
    if(winnt_key) RegCloseKey(winnt_key);
}

/*************************************************************
 * freetype_AddFontResourceEx
 *
 */
static INT CDECL freetype_AddFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    WCHAR path[MAX_PATH];
    INT ret = 0;
    DWORD addfont_flags = ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE;

    if (!(flags & FR_PRIVATE)) addfont_flags |= ADDFONT_ADD_TO_CACHE;
    if (GetFullPathNameW( file, MAX_PATH, path, NULL ))
        ret = add_font_resource( path, addfont_flags );

    if (!ret && !strchrW(file, '\\')) {
        /* Try in %WINDIR%/fonts, needed for Fotobuch Designer */
        get_winfonts_dir_path( file, path );
        ret = add_font_resource( path, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );
        /* Try in datadir/fonts (or builddir/fonts), needed for Magic the Gathering Online */
        if (!ret)
        {
            get_data_dir_path( file, path );
            ret = add_font_resource( path, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );
        }
    }
    return ret;
}

/*************************************************************
 * freetype_AddFontMemResourceEx
 *
 */
static HANDLE CDECL freetype_AddFontMemResourceEx(PVOID pbFont, DWORD cbFont, PVOID pdv, DWORD *pcFonts)
{
    PVOID pFontCopy = HeapAlloc(GetProcessHeap(), 0, cbFont);

    TRACE("Copying %d bytes of data from %p to %p\n", cbFont, pbFont, pFontCopy);
    memcpy(pFontCopy, pbFont, cbFont);

    *pcFonts = AddFontToList(NULL, NULL, pFontCopy, cbFont, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE);
    if (*pcFonts == 0)
    {
        TRACE("AddFontToList failed\n");
        HeapFree(GetProcessHeap(), 0, pFontCopy);
        return 0;
    }
    /* FIXME: is the handle only for use in RemoveFontMemResourceEx or should it be a true handle?
     * For now return something unique but quite random
     */
    TRACE("Returning handle %lx\n", ((INT_PTR)pFontCopy)^0x87654321);
    return (HANDLE)(((INT_PTR)pFontCopy)^0x87654321);
}

/*************************************************************
 * freetype_RemoveFontResourceEx
 *
 */
static BOOL CDECL freetype_RemoveFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    WCHAR path[MAX_PATH];
    INT ret = 0;
    DWORD addfont_flags = ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE;

    if(!(flags & FR_PRIVATE)) addfont_flags |= ADDFONT_ADD_TO_CACHE;
    if (GetFullPathNameW( file, MAX_PATH, path, NULL ))
        ret = remove_font_resource( path, addfont_flags );

    if (!ret && !strchrW(file, '\\'))
    {
        get_winfonts_dir_path( file, path );
        ret = remove_font_resource( path, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );
        if (!ret)
        {
            get_data_dir_path( file, path );
            ret = remove_font_resource( path, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_RESOURCE );
        }
    }
    return ret;
}

static WCHAR *get_ttf_file_name( LPCWSTR font_file, LPCWSTR font_path )
{
    WCHAR *fullname;
    int file_len;

    if (!font_file) return NULL;

    file_len = strlenW( font_file );

    if (font_path && font_path[0])
    {
        int path_len = strlenW( font_path );
        fullname = HeapAlloc( GetProcessHeap(), 0, (file_len + path_len + 2) * sizeof(WCHAR) );
        if (!fullname) return NULL;
        memcpy( fullname, font_path, path_len * sizeof(WCHAR) );
        fullname[path_len] = '\\';
        memcpy( fullname + path_len + 1, font_file, (file_len + 1) * sizeof(WCHAR) );
        return fullname;
    }
    return get_full_path_name( font_file );
}

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

static void GetEnumStructs(Face *face, const WCHAR *family_name, LPENUMLOGFONTEXW pelf,
                           NEWTEXTMETRICEXW *pntm);

static BOOL get_fontdir( const WCHAR *dos_name, struct fontdir *fd )
{
    FT_Face ft_face;
    Face *face = NULL;
    char *unix_name;
    WCHAR *family_name;
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;

    if (!(unix_name = wine_get_unix_file_name( dos_name ))) return FALSE;
    ft_face = new_ft_face( unix_name, NULL, 0, 0, FALSE );
    HeapFree( GetProcessHeap(), 0, unix_name );
    if (!ft_face) return FALSE;
    face = create_face( ft_face, 0, dos_name, NULL, 0, 0 );
    if (face)
    {
        family_name = ft_face_get_family_name( ft_face, GetSystemDefaultLCID() );
        GetEnumStructs( face, family_name, &elf, &ntm );
        release_face( face );
        HeapFree( GetProcessHeap(), 0, family_name );
    }
    pFT_Done_Face( ft_face );

    if (!face) return FALSE;
    if (!(ntm.ntmTm.tmPitchAndFamily & TMPF_TRUETYPE)) return FALSE;

    memset( fd, 0, sizeof(*fd) );

    fd->num_of_resources  = 1;
    fd->res_id            = 0;
    fd->dfVersion         = 0x200;
    fd->dfSize            = sizeof(*fd);
    strcpy( fd->dfCopyright, "Wine fontdir" );
    fd->dfType            = 0x4003;  /* 0x0080 set if private */
    fd->dfPoints          = ntm.ntmTm.ntmSizeEM;
    fd->dfVertRes         = 72;
    fd->dfHorizRes        = 72;
    fd->dfAscent          = ntm.ntmTm.tmAscent;
    fd->dfInternalLeading = ntm.ntmTm.tmInternalLeading;
    fd->dfExternalLeading = ntm.ntmTm.tmExternalLeading;
    fd->dfItalic          = ntm.ntmTm.tmItalic;
    fd->dfUnderline       = ntm.ntmTm.tmUnderlined;
    fd->dfStrikeOut       = ntm.ntmTm.tmStruckOut;
    fd->dfWeight          = ntm.ntmTm.tmWeight;
    fd->dfCharSet         = ntm.ntmTm.tmCharSet;
    fd->dfPixWidth        = 0;
    fd->dfPixHeight       = ntm.ntmTm.tmHeight;
    fd->dfPitchAndFamily  = ntm.ntmTm.tmPitchAndFamily;
    fd->dfAvgWidth        = ntm.ntmTm.tmAveCharWidth;
    fd->dfMaxWidth        = ntm.ntmTm.tmMaxCharWidth;
    fd->dfFirstChar       = ntm.ntmTm.tmFirstChar;
    fd->dfLastChar        = ntm.ntmTm.tmLastChar;
    fd->dfDefaultChar     = ntm.ntmTm.tmDefaultChar;
    fd->dfBreakChar       = ntm.ntmTm.tmBreakChar;
    fd->dfWidthBytes      = 0;
    fd->dfDevice          = 0;
    fd->dfFace            = FIELD_OFFSET( struct fontdir, szFaceName );
    fd->dfReserved        = 0;
    WideCharToMultiByte( CP_ACP, 0, elf.elfLogFont.lfFaceName, -1, fd->szFaceName, LF_FACESIZE, NULL, NULL );

    return TRUE;
}

#define NE_FFLAGS_LIBMODULE     0x8000
#define NE_OSFLAGS_WINDOWS      0x02

static const char dos_string[0x40] = "This is a TrueType resource file";
static const char FONTRES[] = {'F','O','N','T','R','E','S',':'};

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

/*************************************************************
 * freetype_CreateScalableFontResource
 *
 */
static BOOL CDECL freetype_CreateScalableFontResource( DWORD hidden, LPCWSTR resource,
                                                       LPCWSTR font_file, LPCWSTR font_path )
{
    WCHAR *filename = get_ttf_file_name( font_file, font_path );
    struct fontdir fontdir;
    BOOL ret = FALSE;

    if (!filename || !get_fontdir( filename, &fontdir ))
        SetLastError( ERROR_INVALID_PARAMETER );
    else
    {
        if (hidden) fontdir.dfType |= 0x80;
        ret = create_fot( resource, font_file, &fontdir );
    }

    HeapFree( GetProcessHeap(), 0, filename );
    return ret;
}

static inline BOOL is_dbcs_ansi_cp(UINT ansi_cp)
{
    return ( ansi_cp == 932       /* CP932 for Japanese */
            || ansi_cp == 936     /* CP936 for Chinese Simplified */
            || ansi_cp == 949     /* CP949 for Korean */
            || ansi_cp == 950 );  /* CP950 for Chinese Traditional */
}

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
    LOAD_FUNCPTR(FT_Get_WinFNT_Header)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Library_Version)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_Load_Sfnt_Table)
    LOAD_FUNCPTR(FT_Matrix_Multiply)
#ifndef FT_MULFIX_INLINED
    LOAD_FUNCPTR(FT_MulFix)
#endif
    LOAD_FUNCPTR(FT_New_Face)
    LOAD_FUNCPTR(FT_New_Memory_Face)
    LOAD_FUNCPTR(FT_Outline_Get_Bitmap)
    LOAD_FUNCPTR(FT_Outline_Get_CBox)
    LOAD_FUNCPTR(FT_Outline_Transform)
    LOAD_FUNCPTR(FT_Outline_Translate)
    LOAD_FUNCPTR(FT_Render_Glyph)
    LOAD_FUNCPTR(FT_Set_Charmap)
    LOAD_FUNCPTR(FT_Set_Pixel_Sizes)
    LOAD_FUNCPTR(FT_Vector_Length)
    LOAD_FUNCPTR(FT_Vector_Transform)
    LOAD_FUNCPTR(FT_Vector_Unit)
#undef LOAD_FUNCPTR
    /* Don't warn if these ones are missing */
    pFT_Outline_Embolden = dlsym(ft_handle, "FT_Outline_Embolden");
    pFT_Get_TrueType_Engine_Type = dlsym(ft_handle, "FT_Get_TrueType_Engine_Type");
#ifdef FT_LCD_FILTER_H
    pFT_Library_SetLcdFilter = dlsym(ft_handle, "FT_Library_SetLcdFilter");
#endif
    pFT_Property_Set = dlsym(ft_handle, "FT_Property_Set");

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
    if (pFT_Property_Set && FT_SimpleVersion < FT_VERSION_VALUE(2, 8, 1))
    {
        FT_UInt interpreter_version = 35;
        pFT_Property_Set( library, "truetype", "interpreter-version", &interpreter_version );
    }

    return TRUE;

sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the FreeType\n"
      "font library.  To enable Wine to use TrueType fonts please upgrade\n"
      "FreeType to at least version 2.1.4.\n"
      "http://www.freetype.org\n");
    dlclose(ft_handle);
    ft_handle = NULL;
    return FALSE;
}

static void init_font_list(void)
{
    static const WCHAR dot_fonW[] = {'.','f','o','n','\0'};
    static const WCHAR pathW[] = {'P','a','t','h',0};
    HKEY hkey;
    DWORD valuelen, datalen, i = 0, type, dlen, vlen;
    WCHAR path[MAX_PATH];
    char *unixname;

    delete_external_font_keys();

    /* load the system bitmap fonts */
    load_system_fonts();

    /* load in the fonts from %WINDOWSDIR%\\Fonts first of all */
    GetWindowsDirectoryW(path, ARRAY_SIZE(path));
    strcatW(path, fontsW);
    read_font_dir( path, FALSE );

    /* load the wine fonts */
    get_font_dir( path );
    read_font_dir( path, TRUE );

    /* now look under HKLM\Software\Microsoft\Windows[ NT]\CurrentVersion\Fonts
       for any fonts not installed in %WINDOWSDIR%\Fonts.  They will have their
       full path as the entry.  Also look for any .fon fonts, since ReadFontDir
       will skip these. */
    if(RegOpenKeyW(HKEY_LOCAL_MACHINE,
                   is_win9x() ? win9x_font_reg_key : winnt_font_reg_key,
                   &hkey) == ERROR_SUCCESS)
    {
        LPWSTR data, valueW;
        RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         &valuelen, &datalen, NULL, NULL);

        valuelen++; /* returned value doesn't include room for '\0' */
        valueW = HeapAlloc(GetProcessHeap(), 0, valuelen * sizeof(WCHAR));
        data = HeapAlloc(GetProcessHeap(), 0, datalen * sizeof(WCHAR));
        if (valueW && data)
        {
            dlen = datalen * sizeof(WCHAR);
            vlen = valuelen;
            while(RegEnumValueW(hkey, i++, valueW, &vlen, NULL, &type, (LPBYTE)data,
                                &dlen) == ERROR_SUCCESS)
            {
                if(data[0] && (data[1] == ':'))
                {
                    add_font_resource( data, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_TO_CACHE);
                }
                else if(dlen / 2 >= 6 && !strcmpiW(data + dlen / 2 - 5, dot_fonW))
                {
                    WCHAR pathW[MAX_PATH];

                    get_winfonts_dir_path( data, pathW );
                    if (!add_font_resource( pathW, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_TO_CACHE ))
                    {
                        get_data_dir_path( data, pathW );
                        add_font_resource( pathW, ADDFONT_ALLOW_BITMAP | ADDFONT_ADD_TO_CACHE );
                    }
                }
                /* reset dlen and vlen */
                dlen = datalen;
                vlen = valuelen;
            }
        }
        HeapFree(GetProcessHeap(), 0, data);
        HeapFree(GetProcessHeap(), 0, valueW);
        RegCloseKey(hkey);
    }

#ifdef SONAME_LIBFONTCONFIG
    load_fontconfig_fonts();
#elif defined(HAVE_CARBON_CARBON_H)
    load_mac_fonts();
#elif defined(__ANDROID__)
    ReadFontDir("/system/fonts", TRUE);
#endif

    /* then look in any directories that we've specified in the config file */
    /* @@ Wine registry key: HKCU\Software\Wine\Fonts */
    if(RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Fonts", &hkey) == ERROR_SUCCESS)
    {
        DWORD len;
        LPWSTR valueW;
        LPSTR valueA, ptr;

        if (RegQueryValueExW( hkey, pathW, NULL, NULL, NULL, &len ) == ERROR_SUCCESS)
        {
            len += sizeof(WCHAR);
            valueW = HeapAlloc( GetProcessHeap(), 0, len );
            if (RegQueryValueExW( hkey, pathW, NULL, NULL, (LPBYTE)valueW, &len ) == ERROR_SUCCESS)
            {
                len = WideCharToMultiByte( CP_UNIXCP, 0, valueW, -1, NULL, 0, NULL, NULL );
                valueA = HeapAlloc( GetProcessHeap(), 0, len );
                WideCharToMultiByte( CP_UNIXCP, 0, valueW, -1, valueA, len, NULL, NULL );
                TRACE( "got font path %s\n", debugstr_a(valueA) );
                ptr = valueA;
                while (ptr)
                {
                    const char* home;
                    LPSTR next = strchr( ptr, ':' );
                    if (next) *next++ = 0;
                    if (ptr[0] == '~' && ptr[1] == '/' && (home = getenv( "HOME" )) &&
                        (unixname = HeapAlloc( GetProcessHeap(), 0, strlen(ptr) + strlen(home) )))
                    {
                        strcpy( unixname, home );
                        strcat( unixname, ptr + 1 );
                        ReadFontDir( unixname, TRUE );
                        HeapFree( GetProcessHeap(), 0, unixname );
                    }
                    else
                        ReadFontDir( ptr, TRUE );
                    ptr = next;
                }
                HeapFree( GetProcessHeap(), 0, valueA );
            }
            HeapFree( GetProcessHeap(), 0, valueW );
        }
        RegCloseKey(hkey);
    }
}

static BOOL move_to_front(const WCHAR *name)
{
    Family *family, *cursor2;
    LIST_FOR_EACH_ENTRY_SAFE(family, cursor2, &font_list, Family, entry)
    {
        if (!strncmpiW( family->family_name, name, LF_FACESIZE - 1 ))
        {
            list_remove(&family->entry);
            list_add_head(&font_list, &family->entry);
            return TRUE;
        }
    }
    return FALSE;
}

static const WCHAR *set_default(const WCHAR **name_list)
{
    const WCHAR **entry = name_list;

    while (*entry)
    {
        if (move_to_front(*entry)) return *entry;
        entry++;
    }

    return *name_list;
}

static void reorder_font_list(void)
{
    default_serif = set_default( default_serif_list );
    default_fixed = set_default( default_fixed_list );
    default_sans = set_default( default_sans_list );
}

/*************************************************************
 *    WineEngInit
 *
 * Initialize FreeType library and create a list of available faces
 */
BOOL WineEngInit( const struct font_backend_funcs **funcs )
{
    HKEY hkey;
    DWORD disposition;
    HANDLE font_mutex;

    if(!init_freetype()) return FALSE;

#ifdef SONAME_LIBFONTCONFIG
    init_fontconfig();
#endif

    *funcs = &font_funcs;

    if (!RegOpenKeyExW(HKEY_CURRENT_USER, wine_fonts_key, 0, KEY_READ, &hkey))
    {
        static const WCHAR antialias_fake_bold_or_italic[] = { 'A','n','t','i','a','l','i','a','s','F','a','k','e',
                                                               'B','o','l','d','O','r','I','t','a','l','i','c',0 };
        static const WCHAR true_options[] = { 'y','Y','t','T','1',0 };
        DWORD type, size;
        WCHAR buffer[20];

        size = sizeof(buffer);
        if (!RegQueryValueExW(hkey, antialias_fake_bold_or_italic, NULL, &type, (BYTE*)buffer, &size) &&
            type == REG_SZ && size >= 1)
        {
            antialias_fakes = (strchrW(true_options, buffer[0]) != NULL);
        }
        RegCloseKey(hkey);
    }

    if((font_mutex = CreateMutexW(NULL, FALSE, font_mutex_nameW)) == NULL)
    {
        ERR("Failed to create font mutex\n");
        return FALSE;
    }
    WaitForSingleObject(font_mutex, INFINITE);

    create_font_cache_key(&hkey_font_cache, &disposition);

    if(disposition == REG_CREATED_NEW_KEY)
        init_font_list();
    else
        load_font_list_from_cache(hkey_font_cache);

    reorder_font_list();

    DumpFontList();
    LoadSubstList();
    DumpSubstList();
    LoadReplaceList();

    if(disposition == REG_CREATED_NEW_KEY)
        update_reg_entries();

    init_system_links();
    
    ReleaseMutex(font_mutex);
    return TRUE;
}

/* Some fonts have large usWinDescent values, as a result of storing signed short
   in unsigned field. That's probably caused by sTypoDescent vs usWinDescent confusion in
   some font generation tools. */
static inline USHORT get_fixed_windescent(USHORT windescent)
{
    return abs((SHORT)windescent);
}

static LONG calc_ppem_for_height(FT_Face ft_face, LONG height)
{
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;

    LONG ppem;
    const LONG MAX_PPEM = (1 << 16) - 1;

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
        if(pOS2->usWinAscent + windescent == 0)
            ppem = MulDiv(ft_face->units_per_EM, height,
                          pHori->Ascender - pHori->Descender);
        else
            ppem = MulDiv(ft_face->units_per_EM, height,
                          pOS2->usWinAscent + windescent);
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
    if (!(mapping = HeapAlloc( GetProcessHeap(), 0, sizeof(*mapping) )))
        goto error;

    mapping->data = mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );

    if (mapping->data == MAP_FAILED)
    {
        HeapFree( GetProcessHeap(), 0, mapping );
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
        HeapFree( GetProcessHeap(), 0, mapping );
    }
}

static LONG load_VDMX(GdiFont*, LONG);

static FT_Face OpenFontFace(GdiFont *font, Face *face, LONG width, LONG height)
{
    struct gdi_font *gdi_font = font->gdi_font;
    FT_Error err;
    FT_Face ft_face;
    void *data_ptr;
    DWORD data_size;

    TRACE("%s/%p, %ld, %d x %d\n", debugstr_w(face->file), face->font_data_ptr, face->face_index, width, height);

    if (face->file)
    {
        char *filename = wine_get_unix_file_name( face->file );
        font->mapping = map_font_file( filename );
        HeapFree( GetProcessHeap(), 0, filename );
        if (!font->mapping)
        {
            WARN("failed to map %s\n", debugstr_w(face->file));
            return 0;
        }
        data_ptr = font->mapping->data;
        data_size = font->mapping->size;
    }
    else
    {
        data_ptr = face->font_data_ptr;
        data_size = face->font_data_size;
    }

    err = pFT_New_Memory_Face(library, data_ptr, data_size, face->face_index, &ft_face);
    if(err) {
        ERR("FT_New_Face rets %d\n", err);
	return 0;
    }

    /* set it here, as load_VDMX needs it */
    font->ft_face = ft_face;
    gdi_font->scalable = FT_IS_SCALABLE(ft_face);
    gdi_font->face_index = face->face_index;

    if(FT_IS_SCALABLE(ft_face)) {
        FT_ULong len;
        DWORD header;

        /* load the VDMX table if we have one */
        gdi_font->ppem = load_VDMX(font, height);
        if(gdi_font->ppem == 0)
            gdi_font->ppem = calc_ppem_for_height(ft_face, height);
        TRACE("height %d => ppem %d\n", height, gdi_font->ppem);

        if((err = pFT_Set_Pixel_Sizes(ft_face, 0, gdi_font->ppem)) != 0)
            WARN("FT_Set_Pixel_Sizes %d, %d rets %x\n", 0, gdi_font->ppem, err);

        /* see if it's a TTC */
        len = sizeof(header);
        if (!pFT_Load_Sfnt_Table(ft_face, 0, 0, (void*)&header, &len)) {
            if (header == MS_TTCF_TAG)
            {
                len = sizeof(gdi_font->ttc_item_offset);
                if (pFT_Load_Sfnt_Table(ft_face, 0, (3 + face->face_index) * sizeof(DWORD),
                        (void*)&gdi_font->ttc_item_offset, &len))
                    gdi_font->ttc_item_offset = 0;
                else
                    gdi_font->ttc_item_offset = GET_BE_DWORD(gdi_font->ttc_item_offset);
            }
        }
    } else {
        gdi_font->ppem = height;
        if((err = pFT_Set_Pixel_Sizes(ft_face, width, height)) != 0)
            WARN("FT_Set_Pixel_Sizes %d, %d rets %x\n", width, height, err);
    }
    return ft_face;
}


static UINT get_nearest_charset(const WCHAR *family_name, Face *face, UINT *cp)
{
  /* Only get here if lfCharSet == DEFAULT_CHARSET or we couldn't find
     a single face with the requested charset.  The idea is to check if
     the selected font supports the current ANSI codepage, if it does
     return the corresponding charset, else return the first charset */

    CHARSETINFO csi;
    int acp = GetACP(), i;
    DWORD fs0;

    *cp = acp;
    if(TranslateCharsetInfo((DWORD*)(INT_PTR)acp, &csi, TCI_SRCCODEPAGE))
    {
        const SYSTEM_LINKS *font_link;

        if (csi.fs.fsCsb[0] & face->fs.fsCsb[0])
	    return csi.ciCharset;

        font_link = find_font_link(family_name);
        if (font_link != NULL && csi.fs.fsCsb[0] & font_link->fs.fsCsb[0])
	    return csi.ciCharset;
    }

    for(i = 0; i < 32; i++) {
        fs0 = 1L << i;
        if(face->fs.fsCsb[0] & fs0) {
	    if(TranslateCharsetInfo(&fs0, &csi, TCI_SRCFONTSIG)) {
                *cp = csi.ciACP;
	        return csi.ciCharset;
            }
	    else
                FIXME("TCI failing on %x\n", fs0);
	}
    }

    FIXME("returning DEFAULT_CHARSET face->fs.fsCsb[0] = %08x file = %s\n",
	  face->fs.fsCsb[0], debugstr_w(face->file));
    *cp = acp;
    return DEFAULT_CHARSET;
}

/*************************************************************
 * freetype_alloc_font
 */
static BOOL CDECL freetype_alloc_font( struct gdi_font *font )
{
    GdiFont *ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    ret->potm = NULL;
    ret->total_kern_pairs = (DWORD)-1;
    ret->kern_pairs = NULL;
    list_init(&ret->child_fonts);
    ret->gdi_font = font;
    font->private = ret;
    return TRUE;
}

/*************************************************************
 * freetype_destroy_font
 */
static void CDECL freetype_destroy_font( struct gdi_font *gdi_font )
{
    GdiFont *font = get_font_ptr( gdi_font );
    CHILD_FONT *child, *child_next;

    LIST_FOR_EACH_ENTRY_SAFE( child, child_next, &font->child_fonts, CHILD_FONT, entry )
    {
        list_remove(&child->entry);
        if(child->font)
            free_gdi_font(child->font->gdi_font);
        release_face( child->face );
        HeapFree(GetProcessHeap(), 0, child);
    }

    if (font->ft_face) pFT_Done_Face(font->ft_face);
    if (font->mapping) unmap_font_file( font->mapping );
    HeapFree(GetProcessHeap(), 0, font->kern_pairs);
    HeapFree(GetProcessHeap(), 0, font->potm);
    HeapFree(GetProcessHeap(), 0, font->GSUB_Table);
    HeapFree(GetProcessHeap(), 0, font);
}

/*************************************************************
 * freetype_get_font_data
 */
static DWORD CDECL freetype_get_font_data( struct gdi_font *font, DWORD table, DWORD offset,
                                           void *buf, DWORD cbData)
{
    FT_Face ft_face = get_font_ptr(font)->ft_face;
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
        TRACE("Can't find table %s\n", debugstr_an((char*)&table, 4));
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

static LONG load_VDMX(GdiFont *font, LONG height)
{
    struct gdi_font *gdi_font = font->gdi_font;
    VDMX_Header hdr;
    VDMX_group group;
    BYTE devXRatio, devYRatio;
    USHORT numRecs, numRatios;
    DWORD result, offset = -1;
    LONG ppem = 0;
    int i;

    result = freetype_get_font_data(gdi_font, MS_VDMX_TAG, 0, &hdr, sizeof(hdr));

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
	freetype_get_font_data(gdi_font, MS_VDMX_TAG, offset, &ratio, sizeof(Ratios));
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
		freetype_get_font_data(gdi_font, MS_VDMX_TAG, offset, &group_offset, sizeof(group_offset));
		offset = GET_BE_WORD(group_offset);
		break;
	    }
    }

    if(offset == -1) return 0;

    if(freetype_get_font_data(gdi_font, MS_VDMX_TAG, offset, &group, sizeof(group)) != GDI_ERROR) {
	USHORT recs;
	BYTE startsz, endsz;
	WORD *vTable;

	recs = GET_BE_WORD(group.recs);
	startsz = group.startsz;
	endsz = group.endsz;

	TRACE("recs=%d  startsz=%d  endsz=%d\n", recs, startsz, endsz);

	vTable = HeapAlloc(GetProcessHeap(), 0, recs * sizeof(VDMX_vTable));
	result = freetype_get_font_data(gdi_font, MS_VDMX_TAG, offset + sizeof(group), vTable, recs * sizeof(VDMX_vTable));
	if(result == GDI_ERROR) {
	    FIXME("Failed to retrieve vTable\n");
	    goto end;
	}

	if(height > 0) {
	    for(i = 0; i < recs; i++) {
                SHORT yMax = GET_BE_WORD(vTable[(i * 3) + 1]);
                SHORT yMin = GET_BE_WORD(vTable[(i * 3) + 2]);
                ppem = GET_BE_WORD(vTable[i * 3]);

		if(yMax + -yMin == height) {
		    gdi_font->yMax = yMax;
		    gdi_font->yMin = yMin;
                    TRACE("ppem %d found; height=%d  yMax=%d  yMin=%d\n", ppem, height, gdi_font->yMax, gdi_font->yMin);
		    break;
		}
		if(yMax + -yMin > height) {
		    if(--i < 0) {
			ppem = 0;
			goto end; /* failed */
		    }
		    gdi_font->yMax = GET_BE_WORD(vTable[(i * 3) + 1]);
		    gdi_font->yMin = GET_BE_WORD(vTable[(i * 3) + 2]);
                    ppem = GET_BE_WORD(vTable[i * 3]);
                    TRACE("ppem %d found; height=%d  yMax=%d  yMin=%d\n", ppem, height, gdi_font->yMax, gdi_font->yMin);
		    break;
		}
	    }
	    if(!gdi_font->yMax) {
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
		yPelHeight = GET_BE_WORD(vTable[i * 3]);

		if(yPelHeight > ppem)
                {
                    ppem = 0;
                    break; /* failed */
                }

		if(yPelHeight == ppem) {
		    gdi_font->yMax = GET_BE_WORD(vTable[(i * 3) + 1]);
		    gdi_font->yMin = GET_BE_WORD(vTable[(i * 3) + 2]);
                    TRACE("ppem %d found; yMax=%d  yMin=%d\n", ppem, gdi_font->yMax, gdi_font->yMin);
		    break;
		}
	    }
	}
	end:
	HeapFree(GetProcessHeap(), 0, vTable);
    }

    return ppem;
}

/*************************************************************
 * create_child_font_list
 */
static BOOL create_child_font_list(GdiFont *font)
{
    struct gdi_font *gdi_font = font->gdi_font;
    BOOL ret = FALSE;
    SYSTEM_LINKS *font_link;
    CHILD_FONT *font_link_entry, *new_child;
    FontSubst *psub;
    WCHAR* font_name;

    psub = get_font_subst(&font_subst_list, gdi_font->name, -1);
    font_name = psub ? psub->to.name : gdi_font->name;
    font_link = find_font_link(font_name);
    if (font_link != NULL)
    {
        TRACE("found entry in system list\n");
        LIST_FOR_EACH_ENTRY(font_link_entry, &font_link->links, CHILD_FONT, entry)
        {
            new_child = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_child));
            new_child->face = font_link_entry->face;
            new_child->font = NULL;
            new_child->face->refcount++;
            list_add_tail(&font->child_fonts, &new_child->entry);
            TRACE("font %s %ld\n", debugstr_w(new_child->face->file), new_child->face->face_index);
        }
        ret = TRUE;
    }
    /*
     * if not SYMBOL or OEM then we also get all the fonts for Microsoft
     * Sans Serif.  This is how asian windows get default fallbacks for fonts
     */
    if (is_dbcs_ansi_cp(GetACP()) && gdi_font->charset != SYMBOL_CHARSET &&
        gdi_font->charset != OEM_CHARSET &&
        strcmpiW(font_name,szDefaultFallbackLink) != 0)
    {
        font_link = find_font_link(szDefaultFallbackLink);
        if (font_link != NULL)
        {
            TRACE("found entry in default fallback list\n");
            LIST_FOR_EACH_ENTRY(font_link_entry, &font_link->links, CHILD_FONT, entry)
            {
                new_child = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_child));
                new_child->face = font_link_entry->face;
                new_child->font = NULL;
                new_child->face->refcount++;
                list_add_tail(&font->child_fonts, &new_child->entry);
                TRACE("font %s %ld\n", debugstr_w(new_child->face->file), new_child->face->face_index);
            }
            ret = TRUE;
        }
    }

    return ret;
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

static BOOL get_gasp_flags( struct gdi_font *gdi_font, WORD *flags )
{
    GdiFont *font = get_font_ptr( gdi_font );
    DWORD size;
    WORD buf[16]; /* Enough for seven ranges before we need to alloc */
    WORD *alloced = NULL, *ptr = buf;
    WORD num_recs, version;
    BOOL ret = FALSE;

    *flags = 0;
    size = freetype_get_font_data( gdi_font, MS_GASP_TAG,  0, NULL, 0 );
    if (size == GDI_ERROR) return FALSE;
    if (size < 4 * sizeof(WORD)) return FALSE;
    if (size > sizeof(buf))
    {
        ptr = alloced = HeapAlloc( GetProcessHeap(), 0, size );
        if (!ptr) return FALSE;
    }

    freetype_get_font_data( gdi_font, MS_GASP_TAG, 0, ptr, size );

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
        if (font->ft_face->size->metrics.y_ppem <= GET_BE_WORD( *ptr )) break;
        ptr += 2;
    }
    TRACE( "got flags %04x for ppem %d\n", *flags, font->ft_face->size->metrics.y_ppem );
    ret = TRUE;

done:
    HeapFree( GetProcessHeap(), 0, alloced );
    return ret;
}

#ifdef SONAME_LIBFONTCONFIG
static Family* get_fontconfig_family(DWORD pitch_and_family, const CHARSETINFO *csi, BOOL want_vertical)
{
    const char *name;
    WCHAR nameW[LF_FACESIZE];
    FcChar8 *str;
    FcPattern *pat = NULL, *best = NULL;
    FcResult result;
    FcBool r;
    int ret, i;
    Family *family = NULL;

    if (!csi->fs.fsCsb[0]) return NULL;

    if((pitch_and_family & FIXED_PITCH) ||
       (pitch_and_family & 0xF0) == FF_MODERN)
        name = "monospace";
    else if((pitch_and_family & 0xF0) == FF_ROMAN)
        name = "serif";
    else
        name = "sans-serif";

    pat = pFcPatternCreate();
    if (!pat) return NULL;
    r = pFcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)name);
    if (!r) goto end;
    r = pFcPatternAddString(pat, FC_NAMELANG, (const FcChar8 *)"en-us");
    if (!r) goto end;
    r = pFcPatternAddString(pat, FC_PRGNAME, (const FcChar8 *)"wine");
    if (!r) goto end;
    r = pFcConfigSubstitute(NULL, pat, FcMatchPattern);
    if (!r) goto end;
    pFcDefaultSubstitute(pat);

    best = pFcFontMatch(NULL, pat, &result);
    if (!best || result != FcResultMatch) goto end;

    for (i = 0;
         !family && pFcPatternGetString(best, FC_FAMILY, i, &str) == FcResultMatch;
         i++)
    {
        Face *face;
        const SYSTEM_LINKS *font_link;
        const struct list *face_list;

        if (!want_vertical)
        {
            ret = MultiByteToWideChar(CP_UTF8, 0, (const char*)str, -1,
                                      nameW, ARRAY_SIZE(nameW));
        }
        else
        {
            nameW[0] = '@';
            ret = MultiByteToWideChar(CP_UTF8, 0, (const char*)str, -1,
                                      nameW + 1, ARRAY_SIZE(nameW) - 1);
        }
        if (!ret) continue;
        family = find_family_from_any_name(nameW);
        if (!family) continue;

        font_link = find_font_link( family->family_name );
        face_list = get_face_list_from_family(family);
        LIST_FOR_EACH_ENTRY( face, face_list, Face, entry ) {
            if (!face->scalable)
                continue;
            if (csi->fs.fsCsb[0] & face->fs.fsCsb[0])
                goto found;
            if (font_link != NULL &&
                csi->fs.fsCsb[0] & font_link->fs.fsCsb[0])
                goto found;
        }
        family = NULL;
    }

found:
    if (family)
        TRACE("got %s\n", wine_dbgstr_w(nameW));

end:
    pFcPatternDestroy(pat);
    pFcPatternDestroy(best);
    return family;
}
#endif

static const GSUB_Script* GSUB_get_script_table( const GSUB_Header* header, const char* tag)
{
    const GSUB_ScriptList *script;
    const GSUB_Script *deflt = NULL;
    int i;
    script = (const GSUB_ScriptList*)((const BYTE*)header + GET_BE_WORD(header->ScriptList));

    TRACE("%i scripts in this font\n",GET_BE_WORD(script->ScriptCount));
    for (i = 0; i < GET_BE_WORD(script->ScriptCount); i++)
    {
        const GSUB_Script *scr;
        int offset;

        offset = GET_BE_WORD(script->ScriptRecord[i].Script);
        scr = (const GSUB_Script*)((const BYTE*)script + offset);

        if (strncmp(script->ScriptRecord[i].ScriptTag, tag,4)==0)
            return scr;
        if (strncmp(script->ScriptRecord[i].ScriptTag, "dflt",4)==0)
            deflt = scr;
    }
    return deflt;
}

static const GSUB_LangSys* GSUB_get_lang_table( const GSUB_Script* script, const char* tag)
{
    int i;
    int offset;
    const GSUB_LangSys *Lang;

    TRACE("Deflang %x, LangCount %i\n",GET_BE_WORD(script->DefaultLangSys), GET_BE_WORD(script->LangSysCount));

    for (i = 0; i < GET_BE_WORD(script->LangSysCount) ; i++)
    {
        offset = GET_BE_WORD(script->LangSysRecord[i].LangSys);
        Lang = (const GSUB_LangSys*)((const BYTE*)script + offset);

        if ( strncmp(script->LangSysRecord[i].LangSysTag,tag,4)==0)
            return Lang;
    }
    offset = GET_BE_WORD(script->DefaultLangSys);
    if (offset)
    {
        Lang = (const GSUB_LangSys*)((const BYTE*)script + offset);
        return Lang;
    }
    return NULL;
}

static const GSUB_Feature * GSUB_get_feature(const GSUB_Header *header, const GSUB_LangSys *lang, const char* tag)
{
    int i;
    const GSUB_FeatureList *feature;
    feature = (const GSUB_FeatureList*)((const BYTE*)header + GET_BE_WORD(header->FeatureList));

    TRACE("%i features\n",GET_BE_WORD(lang->FeatureCount));
    for (i = 0; i < GET_BE_WORD(lang->FeatureCount); i++)
    {
        int index = GET_BE_WORD(lang->FeatureIndex[i]);
        if (strncmp(feature->FeatureRecord[index].FeatureTag,tag,4)==0)
        {
            const GSUB_Feature *feat;
            feat = (const GSUB_Feature*)((const BYTE*)feature + GET_BE_WORD(feature->FeatureRecord[index].Feature));
            return feat;
        }
    }
    return NULL;
}

static const char* get_opentype_script(const struct gdi_font *font)
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

static const VOID * get_GSUB_vert_feature(const GdiFont *font)
{
    const GSUB_Header *header;
    const GSUB_Script *script;
    const GSUB_LangSys *language;
    const GSUB_Feature *feature;

    if (!font->GSUB_Table)
        return NULL;

    header = font->GSUB_Table;

    script = GSUB_get_script_table(header, get_opentype_script(font->gdi_font));
    if (!script)
    {
        TRACE("Script not found\n");
        return NULL;
    }
    language = GSUB_get_lang_table(script, "xxxx"); /* Need to get Lang tag */
    if (!language)
    {
        TRACE("Language not found\n");
        return NULL;
    }
    feature  =  GSUB_get_feature(header, language, "vrt2");
    if (!feature)
        feature  =  GSUB_get_feature(header, language, "vert");
    if (!feature)
    {
        TRACE("vrt2/vert feature not found\n");
        return NULL;
    }
    return feature;
}

/*************************************************************
 * freetype_SelectFont
 */
static struct gdi_font * CDECL freetype_SelectFont( DC *dc, HFONT hfont, UINT *aa_flags,
                                                    UINT default_aa_flags )
{
    struct gdi_font *gdi_font;
    GdiFont *ret;
    Face *face, *best, *best_bitmap;
    Family *family, *last_resort_family;
    const struct list *face_list;
    INT height, width = 0;
    unsigned int score = 0, new_score;
    signed int diff = 0, newdiff;
    BOOL bd, it, can_use_bitmap, want_vertical;
    LOGFONTW lf;
    CHARSETINFO csi;
    FMAT2 dcmat;
    FontSubst *psub = NULL;
    const SYSTEM_LINKS *font_link;

    GetObjectW( hfont, sizeof(lf), &lf );
    lf.lfWidth = abs(lf.lfWidth);

    can_use_bitmap = !!(GetDeviceCaps(dc->hSelf, TEXTCAPS) & TC_RA_ABLE);

    TRACE("%s, h=%d, it=%d, weight=%d, PandF=%02x, charset=%d orient %d escapement %d\n",
	  debugstr_w(lf.lfFaceName), lf.lfHeight, lf.lfItalic,
	  lf.lfWeight, lf.lfPitchAndFamily, lf.lfCharSet, lf.lfOrientation,
	  lf.lfEscapement);

    if(dc->GraphicsMode == GM_ADVANCED)
    {
        memcpy(&dcmat, &dc->xformWorld2Vport, sizeof(FMAT2));
        /* Try to avoid not necessary glyph transformations */
        if (dcmat.eM21 == 0.0 && dcmat.eM12 == 0.0 && dcmat.eM11 == dcmat.eM22)
        {
            lf.lfHeight *= fabs(dcmat.eM11);
            lf.lfWidth *= fabs(dcmat.eM11);
            dcmat.eM11 = dcmat.eM22 = dcmat.eM11 < 0 ? -1 : 1;
        }
    }
    else
    {
        /* Windows 3.1 compatibility mode GM_COMPATIBLE has only limited
           font scaling abilities. */
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

    TRACE("DC transform %f %f %f %f\n", dcmat.eM11, dcmat.eM12,
                                        dcmat.eM21, dcmat.eM22);

    /* check the cache first */
    if ((gdi_font = find_cached_gdi_font( &lf, &dcmat, can_use_bitmap ))) {
        ret = get_font_ptr( gdi_font );
        TRACE("returning cached gdiFont(%p) for hFont %p\n", ret, hfont);
        goto done;
    }

    TRACE("not in cache\n");
    gdi_font = alloc_gdi_font();
    ret = get_font_ptr( gdi_font );

    gdi_font->matrix = dcmat;
    gdi_font->lf = lf;
    gdi_font->can_use_bitmap = can_use_bitmap;

    /* If lfFaceName is "Symbol" then Windows fixes up lfCharSet to
       SYMBOL_CHARSET so that Symbol gets picked irrespective of the
       original value lfCharSet.  Note this is a special case for
       Symbol and doesn't happen at least for "Wingdings*" */

    if(!strcmpiW(lf.lfFaceName, SymbolW))
        lf.lfCharSet = SYMBOL_CHARSET;

    if(!TranslateCharsetInfo((DWORD*)(INT_PTR)lf.lfCharSet, &csi, TCI_SRCCHARSET)) {
        switch(lf.lfCharSet) {
	case DEFAULT_CHARSET:
	    csi.fs.fsCsb[0] = 0;
	    break;
	default:
	    FIXME("Untranslated charset %d\n", lf.lfCharSet);
	    csi.fs.fsCsb[0] = 0;
	    break;
	}
    }

    family = NULL;
    if(lf.lfFaceName[0] != '\0') {
        CHILD_FONT *font_link_entry;
        LPWSTR FaceName = lf.lfFaceName;

        psub = get_font_subst(&font_subst_list, FaceName, lf.lfCharSet);

	if(psub) {
	    TRACE("substituting %s,%d -> %s,%d\n", debugstr_w(FaceName), lf.lfCharSet,
		  debugstr_w(psub->to.name), (psub->to.charset != -1) ? psub->to.charset : lf.lfCharSet);
	    if (psub->to.charset != -1)
		lf.lfCharSet = psub->to.charset;
	}

	/* We want a match on name and charset or just name if
	   charset was DEFAULT_CHARSET.  If the latter then
	   we fixup the returned charset later in get_nearest_charset
	   where we'll either use the charset of the current ansi codepage
	   or if that's unavailable the first charset that the font supports.
	*/
        LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
            if (!strncmpiW( family->family_name, FaceName, LF_FACESIZE - 1 ) ||
                (psub && !strncmpiW( family->family_name, psub->to.name, LF_FACESIZE - 1 )))
            {
                font_link = find_font_link( family->family_name );
                face_list = get_face_list_from_family(family);
                LIST_FOR_EACH_ENTRY( face, face_list, Face, entry ) {
                    if (!(face->scalable || can_use_bitmap))
                        continue;
                    if (csi.fs.fsCsb[0] & face->fs.fsCsb[0])
                        goto found;
                    if (font_link != NULL &&
                        csi.fs.fsCsb[0] & font_link->fs.fsCsb[0])
                        goto found;
                    if (!csi.fs.fsCsb[0])
                        goto found;
                }
            }
	}

        /* Search by full face name. */
        LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
            face_list = get_face_list_from_family(family);
            LIST_FOR_EACH_ENTRY( face, face_list, Face, entry ) {
                if (!strncmpiW( face->full_name, FaceName, LF_FACESIZE - 1 ) && (face->scalable || can_use_bitmap))
                {
                    if (csi.fs.fsCsb[0] & face->fs.fsCsb[0] || !csi.fs.fsCsb[0])
                        goto found_face;
                    font_link = find_font_link( family->family_name );
                    if (font_link != NULL &&
                        csi.fs.fsCsb[0] & font_link->fs.fsCsb[0])
                        goto found_face;
                }
            }
        }

        /*
	 * Try check the SystemLink list first for a replacement font.
	 * We may find good replacements there.
         */
        LIST_FOR_EACH_ENTRY(font_link, &system_links, SYSTEM_LINKS, entry)
        {
            if(!strncmpiW(font_link->font_name, FaceName, LF_FACESIZE - 1) ||
               (psub && !strncmpiW(font_link->font_name,psub->to.name, LF_FACESIZE - 1)))
            {
                TRACE("found entry in system list\n");
                LIST_FOR_EACH_ENTRY(font_link_entry, &font_link->links, CHILD_FONT, entry)
                {
                    const SYSTEM_LINKS *links;

                    face = font_link_entry->face;
                    if (!(face->scalable || can_use_bitmap))
                        continue;
                    family = face->family;
                    if (csi.fs.fsCsb[0] & face->fs.fsCsb[0] || !csi.fs.fsCsb[0])
                        goto found;
                    links = find_font_link( family->family_name );
                    if (links != NULL && csi.fs.fsCsb[0] & links->fs.fsCsb[0])
                        goto found;
                }
            }
        }
    }

    psub = NULL; /* substitution is no more relevant */

    /* If requested charset was DEFAULT_CHARSET then try using charset
       corresponding to the current ansi codepage */
    if (!csi.fs.fsCsb[0])
    {
        INT acp = GetACP();
        if(!TranslateCharsetInfo((DWORD*)(INT_PTR)acp, &csi, TCI_SRCCODEPAGE)) {
            FIXME("TCI failed on codepage %d\n", acp);
            csi.fs.fsCsb[0] = 0;
        } else
            lf.lfCharSet = csi.ciCharset;
    }

    want_vertical = (lf.lfFaceName[0] == '@');

    /* Face families are in the top 4 bits of lfPitchAndFamily,
       so mask with 0xF0 before testing */

    if((lf.lfPitchAndFamily & FIXED_PITCH) ||
       (lf.lfPitchAndFamily & 0xF0) == FF_MODERN)
        strcpyW(lf.lfFaceName, default_fixed);
    else if((lf.lfPitchAndFamily & 0xF0) == FF_ROMAN)
        strcpyW(lf.lfFaceName, default_serif);
    else if((lf.lfPitchAndFamily & 0xF0) == FF_SWISS)
        strcpyW(lf.lfFaceName, default_sans);
    else
        strcpyW(lf.lfFaceName, default_sans);
    LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
        if (!strncmpiW( family->family_name, lf.lfFaceName, LF_FACESIZE - 1 ))
        {
            font_link = find_font_link( family->family_name );
            face_list = get_face_list_from_family(family);
            LIST_FOR_EACH_ENTRY( face, face_list, Face, entry ) {
                if (!(face->scalable || can_use_bitmap))
                    continue;
                if (csi.fs.fsCsb[0] & face->fs.fsCsb[0])
                    goto found;
                if (font_link != NULL && csi.fs.fsCsb[0] & font_link->fs.fsCsb[0])
                    goto found;
            }
        }
    }

#ifdef SONAME_LIBFONTCONFIG
    /* Try FontConfig substitutions if the face isn't found */
    family = get_fontconfig_family(lf.lfPitchAndFamily, &csi, want_vertical);
    if (family) goto found;
#endif

    last_resort_family = NULL;
    LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
        font_link = find_font_link( family->family_name );
        face_list = get_face_list_from_family(family);
        LIST_FOR_EACH_ENTRY( face, face_list, Face, entry ) {
            if(!(face->flags & ADDFONT_VERTICAL_FONT) == !want_vertical &&
               (csi.fs.fsCsb[0] & face->fs.fsCsb[0] ||
                (font_link != NULL && csi.fs.fsCsb[0] & font_link->fs.fsCsb[0]))) {
                if(face->scalable)
                    goto found;
                if(can_use_bitmap && !last_resort_family)
                    last_resort_family = family;
            }            
        }
    }

    if(last_resort_family) {
        family = last_resort_family;
        csi.fs.fsCsb[0] = 0;
        goto found;
    }

    LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
        face_list = get_face_list_from_family(family);
        LIST_FOR_EACH_ENTRY( face, face_list, Face, entry ) {
            if(face->scalable && !(face->flags & ADDFONT_VERTICAL_FONT) == !want_vertical) {
                csi.fs.fsCsb[0] = 0;
                WARN("just using first face for now\n");
                goto found;
            }
            if(can_use_bitmap && !last_resort_family)
                last_resort_family = family;
        }
    }
    if(!last_resort_family) {
        FIXME("can't find a single appropriate font - bailing\n");
        free_gdi_font(gdi_font);
        return NULL;
    }

    WARN("could only find a bitmap font - this will probably look awful!\n");
    family = last_resort_family;
    csi.fs.fsCsb[0] = 0;

found:
    it = lf.lfItalic ? 1 : 0;
    bd = lf.lfWeight > 550 ? 1 : 0;

    height = lf.lfHeight;

    face = best = best_bitmap = NULL;
    font_link = find_font_link( family->family_name );
    face_list = get_face_list_from_family(family);
    LIST_FOR_EACH_ENTRY(face, face_list, Face, entry)
    {
        if (csi.fs.fsCsb[0] & face->fs.fsCsb[0] ||
            (font_link != NULL && csi.fs.fsCsb[0] & font_link->fs.fsCsb[0]) ||
            !csi.fs.fsCsb[0])
        {
            BOOL italic, bold;

            italic = (face->ntmFlags & NTM_ITALIC) ? 1 : 0;
            bold = (face->ntmFlags & NTM_BOLD) ? 1 : 0;
            new_score = (italic ^ it) + (bold ^ bd);
            if(!best || new_score <= score)
            {
                TRACE("(it=%d, bd=%d) is selected for (it=%d, bd=%d)\n",
                      italic, bold, it, bd);
                score = new_score;
                best = face;
                if(best->scalable  && score == 0) break;
                if(!best->scalable)
                {
                    if(height > 0)
                        newdiff = height - (signed int)(best->size.height);
                    else
                        newdiff = -height - ((signed int)(best->size.height) - best->size.internal_leading);
                    if(!best_bitmap || new_score < score ||
                       (diff > 0 && newdiff < diff && newdiff >= 0) || (diff < 0 && newdiff > diff))
                    {
                        TRACE("%d is better for %d diff was %d\n", best->size.height, height, diff);
                        diff = newdiff;
                        best_bitmap = best;
                        if(score == 0 && diff == 0) break;
                    }
                }
            }
        }
    }
    if(best)
        face = best->scalable ? best : best_bitmap;
    gdi_font->fake_italic = (it && !(face->ntmFlags & NTM_ITALIC));
    gdi_font->fake_bold = (bd && !(face->ntmFlags & NTM_BOLD));

found_face:
    height = lf.lfHeight;

    gdi_font->fs = face->fs;

    if(csi.fs.fsCsb[0]) {
        gdi_font->charset = lf.lfCharSet;
        gdi_font->codepage = csi.ciACP;
    }
    else
        gdi_font->charset = get_nearest_charset( family->family_name, face, &gdi_font->codepage );

    TRACE( "Chosen: %s (%s/%p:%ld)\n", debugstr_w(face->full_name), debugstr_w(face->file),
           face->font_data_ptr, face->face_index );

    gdi_font->aveWidth = height ? lf.lfWidth : 0;

    if(!face->scalable) {
        /* Windows uses integer scaling factors for bitmap fonts */
        INT scale, scaled_height;
        struct gdi_font *cachedfont;

        /* FIXME: rotation of bitmap fonts is ignored */
        height = abs(GDI_ROUND( (double)height * gdi_font->matrix.eM22 ));
        if (gdi_font->aveWidth)
            gdi_font->aveWidth = (double)gdi_font->aveWidth * gdi_font->matrix.eM11;
        gdi_font->matrix.eM11 = gdi_font->matrix.eM22 = 1.0;
        dcmat.eM11 = dcmat.eM22 = 1.0;
        /* As we changed the matrix, we need to search the cache for the font again,
         * otherwise we might explode the cache. */
        if((cachedfont = find_cached_gdi_font( &lf, &dcmat, can_use_bitmap ))) {
            TRACE("Found cached font after non-scalable matrix rescale!\n");
            free_gdi_font( gdi_font );
            gdi_font = cachedfont;
            ret = get_font_ptr( gdi_font );
            goto done;
        }

        if (height != 0) height = diff;
        height += face->size.height;

        scale = (height + face->size.height - 1) / face->size.height;
        scaled_height = scale * face->size.height;
        /* Only jump to the next height if the difference <= 25% original height */
        if (scale > 2 && scaled_height - height > face->size.height / 4) scale--;
        /* The jump between unscaled and doubled is delayed by 1 */
        else if (scale == 2 && scaled_height - height > (face->size.height / 4 - 1)) scale--;
        gdi_font->scale_y = scale;

        width = face->size.x_ppem >> 6;
        height = face->size.y_ppem >> 6;
    }
    TRACE("font scale y: %f\n", gdi_font->scale_y);

    ret->ft_face = OpenFontFace(ret, face, width, height);

    if (!ret->ft_face)
    {
        free_gdi_font( gdi_font );
        return NULL;
    }

    set_gdi_font_file_info( gdi_font, face->file, face->font_data_size );
    gdi_font->ntmFlags = face->ntmFlags;
    gdi_font->aa_flags = HIWORD( face->flags );

    pick_charmap( ret->ft_face, gdi_font->charset );

    set_gdi_font_name( gdi_font, psub ? psub->from.name : family->family_name );
    create_child_font_list(ret);

    if (face->flags & ADDFONT_VERTICAL_FONT) /* We need to try to load the GSUB table */
    {
        int length = freetype_get_font_data(gdi_font, MS_GSUB_TAG , 0, NULL, 0);
        if (length != GDI_ERROR)
        {
            ret->GSUB_Table = HeapAlloc(GetProcessHeap(),0,length);
            freetype_get_font_data(gdi_font, MS_GSUB_TAG , 0, ret->GSUB_Table, length);
            TRACE("Loaded GSUB table of %i bytes\n",length);
            ret->vert_feature = get_GSUB_vert_feature(ret);
            if (!ret->vert_feature)
            {
                TRACE("Vertical feature not found\n");
                HeapFree(GetProcessHeap(), 0, ret->GSUB_Table);
                ret->GSUB_Table = NULL;
            }
        }
    }

    TRACE("caching: gdiFont=%p  hfont=%p\n", ret, hfont);

    cache_gdi_font( gdi_font );
done:
    if (ret)
    {
        switch (lf.lfQuality)
        {
        case NONANTIALIASED_QUALITY:
        case ANTIALIASED_QUALITY:
            if (!*aa_flags) *aa_flags = default_aa_flags;
            break;
        case CLEARTYPE_QUALITY:
        case CLEARTYPE_NATURAL_QUALITY:
        default:
            if (!*aa_flags) *aa_flags = gdi_font->aa_flags;
            if (!*aa_flags) *aa_flags = default_aa_flags;

            /* fixup the antialiasing flags for that font */
            switch (*aa_flags)
            {
            case WINE_GGO_HRGB_BITMAP:
            case WINE_GGO_HBGR_BITMAP:
            case WINE_GGO_VRGB_BITMAP:
            case WINE_GGO_VBGR_BITMAP:
                if (is_subpixel_rendering_enabled()) break;
                *aa_flags = GGO_GRAY4_BITMAP;
                /* fall through */
            case GGO_GRAY2_BITMAP:
            case GGO_GRAY4_BITMAP:
            case GGO_GRAY8_BITMAP:
            case WINE_GGO_GRAY16_BITMAP:
                if ((!antialias_fakes || (!gdi_font->fake_bold && !gdi_font->fake_italic)) && is_hinting_enabled())
                {
                    WORD gasp_flags;
                    if (get_gasp_flags( gdi_font, &gasp_flags ) && !(gasp_flags & GASP_DOGRAY))
                    {
                        TRACE( "font %s %d aa disabled by GASP\n",
                               debugstr_w(lf.lfFaceName), lf.lfHeight );
                        *aa_flags = GGO_BITMAP;
                    }
                }
            }
        }
        TRACE( "%p %s %d aa %x\n", hfont, debugstr_w(lf.lfFaceName), lf.lfHeight, *aa_flags );
    }
    return gdi_font;
}

static INT load_script_name( UINT id, WCHAR buffer[LF_FACESIZE] )
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

static inline BOOL is_complex_script_ansi_cp(UINT ansi_cp)
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
static DWORD create_enum_charset_list(DWORD charset, struct enum_charset_list *list)
{
    CHARSETINFO csi;
    DWORD n = 0;

    if (TranslateCharsetInfo(ULongToPtr(charset), &csi, TCI_SRCCHARSET) &&
        csi.fs.fsCsb[0] != 0) {
        list->element[n].mask    = csi.fs.fsCsb[0];
        list->element[n].charset = csi.ciCharset;
        load_script_name( ffs(csi.fs.fsCsb[0]) - 1, list->element[n].name );
        n++;
    }
    else { /* charset is DEFAULT_CHARSET or invalid. */
        INT acp, i;
        DWORD mask = 0;

        /* Set the current codepage's charset as the first element. */
        acp = GetACP();
        if (!is_complex_script_ansi_cp(acp) &&
            TranslateCharsetInfo((DWORD*)(INT_PTR)acp, &csi, TCI_SRCCODEPAGE) &&
            csi.fs.fsCsb[0] != 0) {
            list->element[n].mask    = csi.fs.fsCsb[0];
            list->element[n].charset = csi.ciCharset;
            load_script_name( ffs(csi.fs.fsCsb[0]) - 1, list->element[n].name );
            mask |= csi.fs.fsCsb[0];
            n++;
        }

        /* Fill out left elements. */
        for (i = 0; i < 32; i++) {
            FONTSIGNATURE fs;
            fs.fsCsb[0] = 1L << i;
            fs.fsCsb[1] = 0;
            if (fs.fsCsb[0] & mask)
                continue; /* skip, already added. */
            if (!TranslateCharsetInfo(fs.fsCsb, &csi, TCI_SRCFONTSIG))
                continue; /* skip, this is an invalid fsCsb bit. */

            list->element[n].mask    = fs.fsCsb[0];
            list->element[n].charset = csi.ciCharset;
            load_script_name( i, list->element[n].name );
            mask |= fs.fsCsb[0];
            n++;
        }

        /* add catch all mask for remaining bits */
        if (~mask)
        {
            list->element[n].mask    = ~mask;
            list->element[n].charset = DEFAULT_CHARSET;
            load_script_name( IDS_OTHER - IDS_FIRST_SCRIPT, list->element[n].name );
            n++;
        }
    }
    list->total = n;

    return n;
}

static UINT get_font_type( const NEWTEXTMETRICEXW *ntm )
{
    UINT ret = 0;

    if (ntm->ntmTm.tmPitchAndFamily & TMPF_TRUETYPE)  ret |= TRUETYPE_FONTTYPE;
    if (ntm->ntmTm.tmPitchAndFamily & TMPF_DEVICE)    ret |= DEVICE_FONTTYPE;
    if (!(ntm->ntmTm.tmPitchAndFamily & TMPF_VECTOR)) ret |= RASTER_FONTTYPE;
    return ret;
}


static void GetEnumStructs(Face *face, const WCHAR *family_name, LPENUMLOGFONTEXW pelf,
                           NEWTEXTMETRICEXW *pntm)
{
    struct gdi_font *gdi_font;
    GdiFont *font;
    LONG width, height;

    if (face->cached_enum_data)
    {
        TRACE("Cached\n");
        *pelf = face->cached_enum_data->elf;
        *pntm = face->cached_enum_data->ntm;
        return;
    }

    gdi_font = alloc_gdi_font();
    font = get_font_ptr( gdi_font );

    if(face->scalable) {
        height = 100;
        width = 0;
    } else {
        height = face->size.y_ppem >> 6;
        width = face->size.x_ppem >> 6;
    }

    if (!(font->ft_face = OpenFontFace(font, face, width, height)))
    {
        free_gdi_font(gdi_font);
        return;
    }

    set_gdi_font_name( gdi_font, family_name );
    gdi_font->ntmFlags = face->ntmFlags;

    if (get_outline_text_metrics(font))
    {
        memcpy(&pntm->ntmTm, &font->potm->otmTextMetrics, sizeof(TEXTMETRICW));

        pntm->ntmTm.ntmSizeEM = font->potm->otmEMSquare;
        pntm->ntmTm.ntmCellHeight = gdi_font->ntmCellHeight;
        pntm->ntmTm.ntmAvgWidth = gdi_font->ntmAvgWidth;

        lstrcpynW(pelf->elfLogFont.lfFaceName,
                 (WCHAR*)((char*)font->potm + (ULONG_PTR)font->potm->otmpFamilyName),
                 LF_FACESIZE);
        lstrcpynW(pelf->elfFullName,
                 (WCHAR*)((char*)font->potm + (ULONG_PTR)font->potm->otmpFaceName),
                 LF_FULLFACESIZE);
        lstrcpynW(pelf->elfStyle,
                 (WCHAR*)((char*)font->potm + (ULONG_PTR)font->potm->otmpStyleName),
                 LF_FACESIZE);
    }
    else
    {
        get_text_metrics(font, (TEXTMETRICW *)&pntm->ntmTm);

        pntm->ntmTm.ntmSizeEM = pntm->ntmTm.tmHeight - pntm->ntmTm.tmInternalLeading;
        pntm->ntmTm.ntmCellHeight = pntm->ntmTm.tmHeight;
        pntm->ntmTm.ntmAvgWidth = pntm->ntmTm.tmAveCharWidth;

        lstrcpynW(pelf->elfLogFont.lfFaceName, family_name, LF_FACESIZE);
        lstrcpynW( pelf->elfFullName, face->full_name, LF_FULLFACESIZE );
        lstrcpynW( pelf->elfStyle, face->style_name, LF_FACESIZE );
    }

    pntm->ntmTm.ntmFlags = face->ntmFlags;
    pntm->ntmFontSig = face->fs;

    pelf->elfScript[0] = '\0'; /* This will get set in WineEngEnumFonts */

    pelf->elfLogFont.lfEscapement = 0;
    pelf->elfLogFont.lfOrientation = 0;
    pelf->elfLogFont.lfHeight = pntm->ntmTm.tmHeight;
    pelf->elfLogFont.lfWidth = pntm->ntmTm.tmAveCharWidth;
    pelf->elfLogFont.lfWeight = pntm->ntmTm.tmWeight;
    pelf->elfLogFont.lfItalic = pntm->ntmTm.tmItalic;
    pelf->elfLogFont.lfUnderline = pntm->ntmTm.tmUnderlined;
    pelf->elfLogFont.lfStrikeOut = pntm->ntmTm.tmStruckOut;
    pelf->elfLogFont.lfCharSet = pntm->ntmTm.tmCharSet;
    pelf->elfLogFont.lfOutPrecision = OUT_STROKE_PRECIS;
    pelf->elfLogFont.lfClipPrecision = CLIP_STROKE_PRECIS;
    pelf->elfLogFont.lfQuality = DRAFT_QUALITY;
    pelf->elfLogFont.lfPitchAndFamily = (pntm->ntmTm.tmPitchAndFamily & 0xf1) + 1;

    face->cached_enum_data = HeapAlloc(GetProcessHeap(), 0, sizeof(*face->cached_enum_data));
    if (face->cached_enum_data)
    {
        face->cached_enum_data->elf = *pelf;
        face->cached_enum_data->ntm = *pntm;
    }

    free_gdi_font(gdi_font);
}

static BOOL family_matches(Family *family, const WCHAR *face_name)
{
    Face *face;
    const struct list *face_list;

    if (!strncmpiW( face_name, family->family_name, LF_FACESIZE - 1 )) return TRUE;

    face_list = get_face_list_from_family(family);
    LIST_FOR_EACH_ENTRY(face, face_list, Face, entry)
        if (!strncmpiW( face_name, face->full_name, LF_FACESIZE - 1 )) return TRUE;

    return FALSE;
}

static BOOL face_matches(const WCHAR *family_name, Face *face, const WCHAR *face_name)
{
    if (!strncmpiW(face_name, family_name, LF_FACESIZE - 1)) return TRUE;
    return !strncmpiW( face_name, face->full_name, LF_FACESIZE - 1 );
}

static BOOL enum_face_charsets(const Family *family, Face *face, struct enum_charset_list *list,
                               FONTENUMPROCW proc, LPARAM lparam, const WCHAR *subst)
{
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
    DWORD type, i;

    GetEnumStructs( face, face->family->family_name, &elf, &ntm );
    type = get_font_type( &ntm );
    for(i = 0; i < list->total; i++) {
        if(!face->scalable && face->fs.fsCsb[0] == 0) { /* OEM bitmap */
            elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet = OEM_CHARSET;
            load_script_name( IDS_OEM_DOS - IDS_FIRST_SCRIPT, elf.elfScript );
            i = list->total; /* break out of loop after enumeration */
        }
        else
        {
            if(!(face->fs.fsCsb[0] & list->element[i].mask)) continue;
            /* use the DEFAULT_CHARSET case only if no other charset is present */
            if (list->element[i].charset == DEFAULT_CHARSET &&
                (face->fs.fsCsb[0] & ~list->element[i].mask)) continue;
            elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet = list->element[i].charset;
            strcpyW(elf.elfScript, list->element[i].name);
            if (!elf.elfScript[0])
                FIXME("Unknown elfscript for bit %d\n", ffs(list->element[i].mask) - 1);
        }
        /* Font Replacement */
        if (family != face->family)
        {
            lstrcpynW( elf.elfLogFont.lfFaceName, family->family_name, LF_FACESIZE );
            lstrcpynW( elf.elfFullName, face->full_name, LF_FULLFACESIZE );
        }
        if (subst)
            strcpyW(elf.elfLogFont.lfFaceName, subst);
        TRACE("enuming face %s full %s style %s charset = %d type %d script %s it %d weight %d ntmflags %08x\n",
              debugstr_w(elf.elfLogFont.lfFaceName),
              debugstr_w(elf.elfFullName), debugstr_w(elf.elfStyle),
              elf.elfLogFont.lfCharSet, type, debugstr_w(elf.elfScript),
              elf.elfLogFont.lfItalic, elf.elfLogFont.lfWeight,
              ntm.ntmTm.ntmFlags);
        /* release section before callback (FIXME) */
        LeaveCriticalSection( &font_cs );
        if (!proc(&elf.elfLogFont, (TEXTMETRICW *)&ntm, type, lparam)) return FALSE;
        EnterCriticalSection( &font_cs );
    }
    return TRUE;
}

/*************************************************************
 * freetype_EnumFonts
 */
static BOOL CDECL freetype_EnumFonts( LPLOGFONTW plf, FONTENUMPROCW proc, LPARAM lparam )
{
    Family *family;
    Face *face;
    const struct list *face_list;
    LOGFONTW lf;
    struct enum_charset_list enum_charsets;

    if (!plf)
    {
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        lf.lfFaceName[0] = 0;
        plf = &lf;
    }

    TRACE("facename = %s charset %d\n", debugstr_w(plf->lfFaceName), plf->lfCharSet);

    create_enum_charset_list(plf->lfCharSet, &enum_charsets);

    EnterCriticalSection( &font_cs );
    if(plf->lfFaceName[0]) {
        WCHAR *face_name = plf->lfFaceName;
        FontSubst *psub = get_font_subst(&font_subst_list, plf->lfFaceName, plf->lfCharSet);

        if(psub) {
            TRACE("substituting %s -> %s\n", debugstr_w(plf->lfFaceName),
                  debugstr_w(psub->to.name));
            face_name = psub->to.name;
        }

        LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
            if (!family_matches(family, face_name)) continue;
            face_list = get_face_list_from_family(family);
            LIST_FOR_EACH_ENTRY( face, face_list, Face, entry ) {
                if (!face_matches( family->family_name, face, face_name )) continue;
                if (!enum_face_charsets(family, face, &enum_charsets, proc, lparam, psub ? psub->from.name : NULL)) return FALSE;
	    }
	}
    } else {
        LIST_FOR_EACH_ENTRY( family, &font_list, Family, entry ) {
            face_list = get_face_list_from_family(family);
            face = LIST_ENTRY(list_head(face_list), Face, entry);
            if (!enum_face_charsets(family, face, &enum_charsets, proc, lparam, NULL)) return FALSE;
	}
    }
    LeaveCriticalSection( &font_cs );
    return TRUE;
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

/***************************************************
 * According to the MSDN documentation on WideCharToMultiByte,
 * certain codepages cannot set the default_used parameter.
 * This returns TRUE if the codepage can set that parameter, false else
 * so that calls to WideCharToMultiByte don't fail with ERROR_INVALID_PARAMETER
 */
static BOOL codepage_sets_default_used(UINT codepage)
{
   switch (codepage)
   {
       case CP_UTF7:
       case CP_UTF8:
       case CP_SYMBOL:
           return FALSE;
       default:
           return TRUE;
   }
}

/*
 * GSUB Table handling functions
 */

static INT GSUB_is_glyph_covered(LPCVOID table , UINT glyph)
{
    const GSUB_CoverageFormat1* cf1;

    cf1 = table;

    if (GET_BE_WORD(cf1->CoverageFormat) == 1)
    {
        int count = GET_BE_WORD(cf1->GlyphCount);
        int i;
        TRACE("Coverage Format 1, %i glyphs\n",count);
        for (i = 0; i < count; i++)
            if (glyph == GET_BE_WORD(cf1->GlyphArray[i]))
                return i;
        return -1;
    }
    else if (GET_BE_WORD(cf1->CoverageFormat) == 2)
    {
        const GSUB_CoverageFormat2* cf2;
        int i;
        int count;
        cf2 = (const GSUB_CoverageFormat2*)cf1;

        count = GET_BE_WORD(cf2->RangeCount);
        TRACE("Coverage Format 2, %i ranges\n",count);
        for (i = 0; i < count; i++)
        {
            if (glyph < GET_BE_WORD(cf2->RangeRecord[i].Start))
                return -1;
            if ((glyph >= GET_BE_WORD(cf2->RangeRecord[i].Start)) &&
                (glyph <= GET_BE_WORD(cf2->RangeRecord[i].End)))
            {
                return (GET_BE_WORD(cf2->RangeRecord[i].StartCoverageIndex) +
                    glyph - GET_BE_WORD(cf2->RangeRecord[i].Start));
            }
        }
        return -1;
    }
    else
        ERR("Unknown CoverageFormat %i\n",GET_BE_WORD(cf1->CoverageFormat));

    return -1;
}

static FT_UInt GSUB_apply_feature(const GSUB_Header * header, const GSUB_Feature* feature, UINT glyph)
{
    int i;
    int offset;
    const GSUB_LookupList *lookup;
    lookup = (const GSUB_LookupList*)((const BYTE*)header + GET_BE_WORD(header->LookupList));

    TRACE("%i lookups\n", GET_BE_WORD(feature->LookupCount));
    for (i = 0; i < GET_BE_WORD(feature->LookupCount); i++)
    {
        const GSUB_LookupTable *look;
        offset = GET_BE_WORD(lookup->Lookup[GET_BE_WORD(feature->LookupListIndex[i])]);
        look = (const GSUB_LookupTable*)((const BYTE*)lookup + offset);
        TRACE("type %i, flag %x, subtables %i\n",GET_BE_WORD(look->LookupType),GET_BE_WORD(look->LookupFlag),GET_BE_WORD(look->SubTableCount));
        if (GET_BE_WORD(look->LookupType) != 1)
            FIXME("We only handle SubType 1\n");
        else
        {
            int j;

            for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
            {
                const GSUB_SingleSubstFormat1 *ssf1;
                offset = GET_BE_WORD(look->SubTable[j]);
                ssf1 = (const GSUB_SingleSubstFormat1*)((const BYTE*)look+offset);
                if (GET_BE_WORD(ssf1->SubstFormat) == 1)
                {
                    int offset = GET_BE_WORD(ssf1->Coverage);
                    TRACE("  subtype 1, delta %i\n", GET_BE_WORD(ssf1->DeltaGlyphID));
                    if (GSUB_is_glyph_covered((const BYTE*)ssf1+offset, glyph) != -1)
                    {
                        TRACE("  Glyph 0x%x ->",glyph);
                        glyph += GET_BE_WORD(ssf1->DeltaGlyphID);
                        TRACE(" 0x%x\n",glyph);
                    }
                }
                else
                {
                    const GSUB_SingleSubstFormat2 *ssf2;
                    INT index;
                    INT offset;

                    ssf2 = (const GSUB_SingleSubstFormat2 *)ssf1;
                    offset = GET_BE_WORD(ssf1->Coverage);
                    TRACE("  subtype 2,  glyph count %i\n", GET_BE_WORD(ssf2->GlyphCount));
                    index = GSUB_is_glyph_covered((const BYTE*)ssf2+offset, glyph);
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
    }
    return glyph;
}


static FT_UInt get_GSUB_vert_glyph(const GdiFont *font, UINT glyph)
{
    const GSUB_Header *header;
    const GSUB_Feature *feature;

    if (!font->GSUB_Table)
        return glyph;

    header = font->GSUB_Table;
    feature = font->vert_feature;

    return GSUB_apply_feature(header, feature, glyph);
}

static FT_UInt get_glyph_index_symbol(const GdiFont *font, UINT glyph)
{
    FT_UInt ret;

    if (glyph < 0x100) glyph += 0xf000;
    /* there are a number of old pre-Unicode "broken" TTFs, which
       do have symbols at U+00XX instead of U+f0XX */
    if (!(ret = pFT_Get_Char_Index(font->ft_face, glyph)))
        ret = pFT_Get_Char_Index(font->ft_face, glyph - 0xf000);

    return ret;
}

static FT_UInt get_glyph_index(const GdiFont *font, UINT glyph)
{
    struct gdi_font *gdi_font = font->gdi_font;
    FT_UInt ret;
    WCHAR wc;
    char buf;

    if (font->ft_face->charmap->encoding == FT_ENCODING_NONE)
    {
        BOOL default_used;
        BOOL *default_used_pointer;

        default_used_pointer = NULL;
        default_used = FALSE;
        if (codepage_sets_default_used(gdi_font->codepage))
            default_used_pointer = &default_used;
        wc = (WCHAR)glyph;
        if (!WideCharToMultiByte(gdi_font->codepage, 0, &wc, 1, &buf, sizeof(buf), NULL, default_used_pointer) ||
            default_used)
        {
            if (gdi_font->codepage == CP_SYMBOL)
            {
                ret = get_glyph_index_symbol(font, glyph);
                if (!ret)
                {
                    if (WideCharToMultiByte(CP_ACP, 0, &wc, 1, &buf, 1, NULL, NULL))
                        ret = get_glyph_index_symbol(font, buf);
                }
            }
            else
                ret = 0;
        }
        else
            ret = pFT_Get_Char_Index(font->ft_face, (unsigned char)buf);
        TRACE("%04x (%02x) -> ret %d def_used %d\n", glyph, (unsigned char)buf, ret, default_used);
        return ret;
    }

    if (font->ft_face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
    {
        ret = get_glyph_index_symbol(font, glyph);
        if (!ret)
        {
            wc = (WCHAR)glyph;
            if (WideCharToMultiByte(CP_ACP, 0, &wc, 1, &buf, 1, NULL, NULL))
                ret = get_glyph_index_symbol(font, (unsigned char)buf);
        }
        return ret;
    }

    return pFT_Get_Char_Index(font->ft_face, glyph);
}

/*************************************************************
 * freetype_get_glyph_index
 */
static BOOL CDECL freetype_get_glyph_index( struct gdi_font *gdi_font, UINT *glyph )
{
    GdiFont *font = get_font_ptr( gdi_font );

    if (font->ft_face->charmap->encoding == FT_ENCODING_NONE) return FALSE;

    if (font->ft_face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
    {
        if (!(*glyph = get_glyph_index_symbol( font, *glyph )))
        {
            WCHAR wc = *glyph;
            char ch;

            if (WideCharToMultiByte( CP_ACP, 0, &wc, 1, &ch, 1, NULL, NULL ))
                *glyph = get_glyph_index_symbol( font, (unsigned char)ch );
        }
        return TRUE;
    }

    if ((*glyph = pFT_Get_Char_Index( font->ft_face, *glyph )))
        *glyph = get_GSUB_vert_glyph( font, *glyph );

    return TRUE;
}

/*************************************************************
 * freetype_get_default_glyph
 */
static UINT CDECL freetype_get_default_glyph( struct gdi_font *gdi_font )
{
    GdiFont *font = get_font_ptr( gdi_font );
    FT_WinFNT_HeaderRec winfnt;
    TT_OS2 *pOS2;

    if ((pOS2 = pFT_Get_Sfnt_Table( font->ft_face, ft_sfnt_os2 )))
    {
        UINT glyph = pOS2->usDefaultChar;
        freetype_get_glyph_index( gdi_font, &glyph );
        return glyph;
    }
    if (!pFT_Get_WinFNT_Header( font->ft_face, &winfnt )) return winfnt.default_char + winfnt.first_char;
    return 32;
}


static inline BOOL is_identity_FMAT2(const FMAT2 *matrix)
{
    static const FMAT2 identity = { 1.0, 0.0, 0.0, 1.0 };
    return !memcmp(matrix, &identity, sizeof(FMAT2));
}

static inline BOOL is_identity_MAT2(const MAT2 *matrix)
{
    static const MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };
    return !memcmp(matrix, &identity, sizeof(MAT2));
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

static BOOL get_transform_matrices( GdiFont *font, BOOL vertical, const MAT2 *user_transform,
                                    FT_Matrix matrices[3] )
{
    struct gdi_font *gdi_font = font->gdi_font;
    static const FT_Matrix identity_mat = { (1 << 16), 0, 0, (1 << 16) };
    BOOL needs_transform = FALSE;
    double width_ratio;
    int i;

    matrices[matrix_unrotated] = identity_mat;

    /* Scaling factor */
    if (gdi_font->aveWidth)
    {
        TEXTMETRICW tm;
        get_text_metrics( font, &tm );

        width_ratio = (double)gdi_font->aveWidth;
        width_ratio /= (double)font->potm->otmTextMetrics.tmAveCharWidth;
    }
    else
        width_ratio = gdi_font->scale_y;

    /* Scaling transform */
    if (width_ratio != 1.0 || gdi_font->scale_y != 1.0)
    {
        FT_Matrix scale_mat;
        scale_mat.xx = FT_FixedFromFloat( width_ratio );
        scale_mat.xy = 0;
        scale_mat.yx = 0;
        scale_mat.yy = FT_FixedFromFloat( gdi_font->scale_y );

        pFT_Matrix_Multiply( &scale_mat, &matrices[matrix_unrotated] );
        needs_transform = TRUE;
    }

    /* Slant transform */
    if (gdi_font->fake_italic)
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
    if (gdi_font->scalable && gdi_font->lf.lfOrientation % 3600)
    {
        FT_Matrix rotation_mat;
        FT_Vector angle;

        pFT_Vector_Unit( &angle, MulDiv( 1 << 16, gdi_font->lf.lfOrientation, 10 ) );
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
    if (!is_identity_FMAT2( &gdi_font->matrix ))
    {
        FT_Matrix world_mat;
        world_mat.xx =  FT_FixedFromFloat( gdi_font->matrix.eM11 );
        world_mat.xy = -FT_FixedFromFloat( gdi_font->matrix.eM21 );
        world_mat.yx = -FT_FixedFromFloat( gdi_font->matrix.eM12 );
        world_mat.yy =  FT_FixedFromFloat( gdi_font->matrix.eM22 );

        for (i = 0; i < 3; i++)
            pFT_Matrix_Multiply( &world_mat, &matrices[i] );
        needs_transform = TRUE;
    }

    /* Extra transformation specified by caller */
    if (!is_identity_MAT2( user_transform ))
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

    return needs_transform;
}

static BOOL get_bold_glyph_outline(FT_GlyphSlot glyph, LONG ppem, FT_Glyph_Metrics *metrics)
{
    FT_Error err;
    FT_Pos strength;
    FT_BBox bbox;

    if(glyph->format != FT_GLYPH_FORMAT_OUTLINE)
        return FALSE;
    if(!pFT_Outline_Embolden)
        return FALSE;

    strength = MulDiv(ppem, 1 << 6, 24);
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

extern const unsigned short vertical_orientation_table[] DECLSPEC_HIDDEN;

static BOOL check_unicode_tategaki(WCHAR uchar)
{
    unsigned short orientation = vertical_orientation_table[vertical_orientation_table[vertical_orientation_table[uchar >> 8]+((uchar >> 4) & 0x0f)]+ (uchar & 0xf)];

    /* We only reach this code if typographical substitution did not occur */
    /* Type: U or Type: Tu */
    return (orientation ==  1 || orientation == 3);
}

static FT_Vector get_advance_metric(GdiFont *incoming_font, GdiFont *font,
                                    const FT_Glyph_Metrics *metrics,
                                    const FT_Matrix *transMat, BOOL vertical_metrics)
{
    struct gdi_font *incoming_gdi_font = incoming_font->gdi_font;
    struct gdi_font *gdi_font = font->gdi_font;
    FT_Vector adv;
    FT_Fixed base_advance, em_scale = 0;
    BOOL fixed_pitch_full = FALSE;

    if (vertical_metrics)
        base_advance = metrics->vertAdvance;
    else
        base_advance = metrics->horiAdvance;

    adv.x = base_advance;
    adv.y = 0;

    /* In fixed-pitch font, we adjust the fullwidth character advance so that
       they have double halfwidth character width. E.g. if the font is 19 ppem,
       we return 20 (not 19) for fullwidth characters as we return 10 for
       halfwidth characters. */
    if(incoming_gdi_font->scalable &&
       (incoming_font->potm || get_outline_text_metrics(incoming_font)) &&
       !(incoming_font->potm->otmTextMetrics.tmPitchAndFamily & TMPF_FIXED_PITCH)) {
        UINT avg_advance;
        em_scale = MulDiv(incoming_gdi_font->ppem, 1 << 16,
                          incoming_font->ft_face->units_per_EM);
        avg_advance = pFT_MulFix(incoming_gdi_font->ntmAvgWidth, em_scale);
        fixed_pitch_full = (avg_advance > 0 &&
                            (base_advance + 63) >> 6 ==
                            pFT_MulFix(incoming_gdi_font->ntmAvgWidth*2, em_scale));
        if (fixed_pitch_full && !transMat)
            adv.x = (avg_advance * 2) << 6;
    }

    if (transMat) {
        pFT_Vector_Transform(&adv, transMat);
        if (fixed_pitch_full && adv.y == 0) {
            FT_Vector vec;
            vec.x = incoming_gdi_font->ntmAvgWidth;
            vec.y = 0;
            pFT_Vector_Transform(&vec, transMat);
            adv.x = (pFT_MulFix(vec.x, em_scale) * 2) << 6;
        }
    }

    if (gdi_font->fake_bold) {
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

static FT_BBox get_transformed_bbox( const FT_Glyph_Metrics *metrics,
                                     BOOL needs_transform, const FT_Matrix metrices[3] )
{
    FT_BBox bbox = { 0, 0, 0, 0 };

    if (!needs_transform)
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
                TRACE( "Vec %ld,i %ld\n", vec.x, vec.y );
                pFT_Vector_Transform( &vec, &metrices[matrix_vert] );
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

static void compute_metrics( GdiFont *incoming_font, GdiFont *font,
                             FT_BBox bbox, const FT_Glyph_Metrics *metrics,
                             BOOL vertical, BOOL vertical_metrics,
                             BOOL needs_transform, const FT_Matrix matrices[3],
                             GLYPHMETRICS *gm, ABC *abc )
{
    FT_Vector adv, vec, origin;

    if (!needs_transform)
    {
        adv = get_advance_metric( incoming_font, font, metrics, NULL, vertical_metrics );
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

        if (vertical && (font->potm || get_outline_text_metrics( font )))
        {
            if (vertical_metrics)
                lsb = metrics->horiBearingY + metrics->vertBearingY;
            else
                lsb = metrics->vertAdvance + (font->potm->otmDescent << 6);
            vec.x = lsb;
            vec.y = font->potm->otmDescent << 6;
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

        adv = get_advance_metric( incoming_font, font, metrics, &matrices[matrix_hori],
                                  vertical_metrics );
        gm->gmCellIncX = adv.x >> 6;
        gm->gmCellIncY = adv.y >> 6;

        adv = get_advance_metric( incoming_font, font, metrics, &matrices[matrix_unrotated],
                                  vertical_metrics );
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
                                    BOOL fake_bold, BOOL needs_transform, FT_Matrix matrices[3],
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

        if (needs_transform)
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
                                         BOOL fake_bold, BOOL needs_transform, FT_Matrix matrices[3],
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

        if (needs_transform)
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
                                        BOOL fake_bold, BOOL needs_transform, FT_Matrix matrices[3],
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

        if (needs_transform)
            pFT_Outline_Transform( &glyph->outline, &matrices[matrix_vert] );

#ifdef FT_LCD_FILTER_H
        if (pFT_Library_SetLcdFilter)
            pFT_Library_SetLcdFilter( library, FT_LCD_FILTER_DEFAULT );
#endif
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
static DWORD CDECL freetype_get_glyph_outline( struct gdi_font *incoming_gdi_font, UINT glyph, UINT format,
                                               GLYPHMETRICS *lpgm, ABC *abc, DWORD buflen, void *buf,
                                               const MAT2 *lpmat )
{
    GLYPHMETRICS gm;
    GdiFont *incoming_font = get_font_ptr( incoming_gdi_font );
    FT_Face ft_face = incoming_font->ft_face;
    GdiFont *font = incoming_font;
    struct gdi_font *gdi_font = font->gdi_font;
    FT_Glyph_Metrics metrics;
    FT_UInt glyph_index;
    DWORD needed = 0;
    FT_Error err;
    FT_BBox bbox;
    FT_Int load_flags = get_load_flags(format);
    FT_Matrix matrices[3];
    BOOL needsTransform = FALSE;
    BOOL tategaki = (gdi_font->name[0] == '@');
    BOOL vertical_metrics;

    TRACE("%p, %04x, %08x, %p, %08x, %p, %p\n", font, glyph, format, lpgm,
	  buflen, buf, lpmat);

    TRACE("font transform %f %f %f %f\n",
          gdi_font->matrix.eM11, gdi_font->matrix.eM12,
          gdi_font->matrix.eM21, gdi_font->matrix.eM22);

    if(format & GGO_GLYPH_INDEX) {
        if(font->ft_face->charmap->encoding == FT_ENCODING_NONE) {
            /* Windows bitmap font, e.g. Small Fonts, uses ANSI character code
               as glyph index. "Treasure Adventure Game" depends on this. */
            glyph_index = pFT_Get_Char_Index(font->ft_face, glyph);
            TRACE("translate glyph index %04x -> %04x\n", glyph, glyph_index);
        } else
            glyph_index = glyph;
	format &= ~GGO_GLYPH_INDEX;
        /* TODO: Window also turns off tategaki for glyphs passed in by index
            if their unicode code points fall outside of the range that is
            rotated. */
    } else {
        BOOL vert;
        get_glyph_index_linked(incoming_font, glyph, &font, &glyph_index, &vert);
        ft_face = font->ft_face;
        gdi_font = font->gdi_font;
        if (!vert && tategaki)
            tategaki = check_unicode_tategaki(glyph);
    }

    format &= ~GGO_UNHINTED;

    if (format == GGO_METRICS && is_identity_MAT2(lpmat) &&
        get_gdi_font_glyph_metrics( gdi_font, glyph_index, lpgm, abc ))
        return 1; /* FIXME */

    needsTransform = get_transform_matrices( font, tategaki, lpmat, matrices );

    vertical_metrics = (tategaki && FT_HAS_VERTICAL(ft_face));
    /* there is a freetype bug where vertical metrics are only
       properly scaled and correct in 2.4.0 or greater */
    if (vertical_metrics && FT_SimpleVersion < FT_VERSION_VALUE(2, 4, 0))
        vertical_metrics = FALSE;

    if (needsTransform || format != GGO_BITMAP) load_flags |= FT_LOAD_NO_BITMAP;
    if (vertical_metrics) load_flags |= FT_LOAD_VERTICAL_LAYOUT;

    err = pFT_Load_Glyph(ft_face, glyph_index, load_flags);
    if (err && !(load_flags & FT_LOAD_NO_HINTING))
    {
        WARN("Failed to load glyph %#x, retrying without hinting. Error %#x.\n", glyph_index, err);
        load_flags |= FT_LOAD_NO_HINTING;
        err = pFT_Load_Glyph(ft_face, glyph_index, load_flags);
    }

    if(err) {
        WARN("Failed to load glyph %#x, error %#x.\n", glyph_index, err);
        return GDI_ERROR;
    }

    metrics = ft_face->glyph->metrics;
    if(gdi_font->fake_bold) {
        if (!get_bold_glyph_outline(ft_face->glyph, gdi_font->ppem, &metrics) && metrics.width)
            metrics.width += 1 << 6;
    }

    /* Some poorly-created fonts contain glyphs that exceed the boundaries set
     * by the text metrics. The proper behavior is to clip the glyph metrics to
     * fit within the maximums specified in the text metrics. */
    if(incoming_font->potm || get_outline_text_metrics(incoming_font) ||
        get_bitmap_text_metrics(incoming_font)) {
        TEXTMETRICW *ptm = &incoming_font->potm->otmTextMetrics;
        INT top = min( metrics.horiBearingY, ptm->tmAscent << 6 );
        INT bottom = max( metrics.horiBearingY - metrics.height, -(ptm->tmDescent << 6) );
        metrics.horiBearingY = top;
        metrics.height = top - bottom;

        /* TODO: Are we supposed to clip the width as well...? */
        /* metrics.width = min( metrics.width, ptm->tmMaxCharWidth << 6 ); */
    }

    bbox = get_transformed_bbox( &metrics, needsTransform, matrices );
    compute_metrics( incoming_font, font, bbox, &metrics,
                     tategaki, vertical_metrics, needsTransform, matrices,
                     &gm, abc );

    if ((format == GGO_METRICS || format == GGO_BITMAP || format ==  WINE_GGO_GRAY16_BITMAP) &&
        is_identity_MAT2(lpmat)) /* don't cache custom transforms */
        set_gdi_font_glyph_metrics( gdi_font, glyph_index, &gm, abc );

    if(format == GGO_METRICS)
    {
        *lpgm = gm;
        return 1; /* FIXME */
    }

    if(ft_face->glyph->format != ft_glyph_format_outline &&
       (format == GGO_NATIVE || format == GGO_BEZIER))
    {
        TRACE("loaded a bitmap\n");
	return GDI_ERROR;
    }

    switch (format)
    {
    case GGO_BITMAP:
        needed = get_mono_glyph_bitmap( ft_face->glyph, bbox, gdi_font->fake_bold,
                                        needsTransform, matrices, buflen, buf );
        break;

    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    case WINE_GGO_GRAY16_BITMAP:
        needed = get_antialias_glyph_bitmap( ft_face->glyph, bbox, format, gdi_font->fake_bold,
                                             needsTransform, matrices, buflen, buf );
	break;

    case WINE_GGO_HRGB_BITMAP:
    case WINE_GGO_HBGR_BITMAP:
    case WINE_GGO_VRGB_BITMAP:
    case WINE_GGO_VBGR_BITMAP:
        needed = get_subpixel_glyph_bitmap( ft_face->glyph, bbox, format, gdi_font->fake_bold,
                                            needsTransform, matrices, &gm, buflen, buf );
        break;

    case GGO_NATIVE:
      {
        FT_Outline *outline = &ft_face->glyph->outline;

        if(buflen == 0) buf = NULL;

        if (needsTransform && buf)
            pFT_Outline_Transform( outline, &matrices[matrix_vert] );

        needed = get_native_glyph_outline(outline, buflen, NULL);

        if (!buf || !buflen)
            break;
        if (needed > buflen)
            return GDI_ERROR;

        get_native_glyph_outline(outline, buflen, buf);
        break;
      }
    case GGO_BEZIER:
      {
        FT_Outline *outline = &ft_face->glyph->outline;
        if(buflen == 0) buf = NULL;

        if (needsTransform && buf)
            pFT_Outline_Transform( outline, &matrices[matrix_vert] );

        needed = get_bezier_glyph_outline(outline, buflen, NULL);

        if (!buf || !buflen)
            break;
        if (needed > buflen)
            return GDI_ERROR;

        get_bezier_glyph_outline(outline, buflen, buf);
        break;
      }

    default:
        FIXME("Unsupported format %d\n", format);
	return GDI_ERROR;
    }
    if (needed != GDI_ERROR)
        *lpgm = gm;

    return needed;
}

static BOOL get_bitmap_text_metrics(GdiFont *font)
{
    struct gdi_font *gdi_font = font->gdi_font;
    FT_Face ft_face = font->ft_face;
    FT_WinFNT_HeaderRec winfnt_header;
    const DWORD size = offsetof(OUTLINETEXTMETRICW, otmFiller); 
    font->potm = HeapAlloc(GetProcessHeap(), 0, size);
    font->potm->otmSize = size;

#define TM font->potm->otmTextMetrics
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
        TM.tmCharSet = gdi_font->charset;
    }
    TM.tmUnderlined = gdi_font->lf.lfUnderline ? 0xff : 0;
    TM.tmStruckOut = gdi_font->lf.lfStrikeOut ? 0xff : 0;

    if(gdi_font->fake_bold)
        TM.tmWeight = FW_BOLD;
#undef TM

    return TRUE;
}


static void scale_font_metrics(const GdiFont *font, LPTEXTMETRICW ptm)
{
    const struct gdi_font *gdi_font = font->gdi_font;
    double scale_x, scale_y;

    if (gdi_font->aveWidth)
    {
        scale_x = (double)gdi_font->aveWidth;
        scale_x /= (double)font->potm->otmTextMetrics.tmAveCharWidth;
    }
    else
        scale_x = gdi_font->scale_y;

    scale_x *= fabs(gdi_font->matrix.eM11);
    scale_y = gdi_font->scale_y * fabs(gdi_font->matrix.eM22);

#define SCALE_X(x) (x) = GDI_ROUND((double)(x) * (scale_x))
#define SCALE_Y(y) (y) = GDI_ROUND((double)(y) * (scale_y))

    SCALE_Y(ptm->tmHeight);
    SCALE_Y(ptm->tmAscent);
    SCALE_Y(ptm->tmDescent);
    SCALE_Y(ptm->tmInternalLeading);
    SCALE_Y(ptm->tmExternalLeading);

    SCALE_X(ptm->tmOverhang);
    if(gdi_font->fake_bold)
    {
        if(!gdi_font->scalable)
            ptm->tmOverhang++;
        ptm->tmAveCharWidth++;
        ptm->tmMaxCharWidth++;
    }
    SCALE_X(ptm->tmAveCharWidth);
    SCALE_X(ptm->tmMaxCharWidth);

#undef SCALE_X
#undef SCALE_Y
}

static void scale_outline_font_metrics(const GdiFont *font, OUTLINETEXTMETRICW *potm)
{
    const struct gdi_font *gdi_font = font->gdi_font;
    double scale_x, scale_y;

    if (gdi_font->aveWidth)
    {
        scale_x = (double)gdi_font->aveWidth;
        scale_x /= (double)font->potm->otmTextMetrics.tmAveCharWidth;
    }
    else
        scale_x = gdi_font->scale_y;

    scale_x *= fabs(gdi_font->matrix.eM11);
    scale_y = gdi_font->scale_y * fabs(gdi_font->matrix.eM22);

    scale_font_metrics(font, &potm->otmTextMetrics);

/* Windows scales these values as signed integers even if they are unsigned */
#define SCALE_X(x) (x) = GDI_ROUND((int)(x) * (scale_x))
#define SCALE_Y(y) (y) = GDI_ROUND((int)(y) * (scale_y))

    SCALE_Y(potm->otmAscent);
    SCALE_Y(potm->otmDescent);
    SCALE_Y(potm->otmLineGap);
    SCALE_Y(potm->otmsCapEmHeight);
    SCALE_Y(potm->otmsXHeight);
    SCALE_Y(potm->otmrcFontBox.top);
    SCALE_Y(potm->otmrcFontBox.bottom);
    SCALE_X(potm->otmrcFontBox.left);
    SCALE_X(potm->otmrcFontBox.right);
    SCALE_Y(potm->otmMacAscent);
    SCALE_Y(potm->otmMacDescent);
    SCALE_Y(potm->otmMacLineGap);
    SCALE_X(potm->otmptSubscriptSize.x);
    SCALE_Y(potm->otmptSubscriptSize.y);
    SCALE_X(potm->otmptSubscriptOffset.x);
    SCALE_Y(potm->otmptSubscriptOffset.y);
    SCALE_X(potm->otmptSuperscriptSize.x);
    SCALE_Y(potm->otmptSuperscriptSize.y);
    SCALE_X(potm->otmptSuperscriptOffset.x);
    SCALE_Y(potm->otmptSuperscriptOffset.y);
    SCALE_Y(potm->otmsStrikeoutSize);
    SCALE_Y(potm->otmsStrikeoutPosition);
    SCALE_Y(potm->otmsUnderscoreSize);
    SCALE_Y(potm->otmsUnderscorePosition);

#undef SCALE_X
#undef SCALE_Y
}

static BOOL get_text_metrics(GdiFont *font, LPTEXTMETRICW ptm)
{
    struct gdi_font *gdi_font = font->gdi_font;
    if(!font->potm)
    {
        if (!get_outline_text_metrics(font) && !get_bitmap_text_metrics(font)) return FALSE;

        /* Make sure that the font has sane width/height ratio */
        if (gdi_font->aveWidth)
        {
            if ((gdi_font->aveWidth + font->potm->otmTextMetrics.tmHeight - 1) / font->potm->otmTextMetrics.tmHeight > 100)
            {
                WARN("Ignoring too large font->aveWidth %d\n", gdi_font->aveWidth);
                gdi_font->aveWidth = 0;
            }
        }
    }
    *ptm = font->potm->otmTextMetrics;
    scale_font_metrics(font, ptm);
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

static BOOL get_outline_text_metrics(GdiFont *font)
{
    BOOL ret = FALSE;
    FT_Face ft_face = font->ft_face;
    struct gdi_font *gdi_font = font->gdi_font;
    UINT needed, lenfam, lensty, lenface, lenfull;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    TT_Postscript *pPost;
    FT_Fixed em_scale;
    WCHAR *family_nameW, *style_nameW, *face_nameW, *full_nameW;
    char *cp;
    INT ascent, descent;
    USHORT windescent;

    TRACE("font=%p\n", font);

    if(!FT_IS_SCALABLE(ft_face))
        return FALSE;

    needed = sizeof(*font->potm);

    lenfam = (strlenW(gdi_font->name) + 1) * sizeof(WCHAR);
    family_nameW = strdupW(gdi_font->name);

    style_nameW = ft_face_get_style_name( ft_face, GetSystemDefaultLangID() );
    lensty = (strlenW(style_nameW) + 1) * sizeof(WCHAR);

    face_nameW = ft_face_get_full_name( ft_face, GetSystemDefaultLangID() );
    if (gdi_font->name[0] == '@') face_nameW = get_vertical_name( face_nameW );
    lenface = (strlenW(face_nameW) + 1) * sizeof(WCHAR);

    full_nameW = get_face_name( ft_face, TT_NAME_ID_UNIQUE_ID, GetSystemDefaultLangID() );
    if (!full_nameW)
    {
        static const WCHAR fake_nameW[] = {'f','a','k','e',' ','n','a','m','e', 0};
        FIXME("failed to read full_nameW for font %s!\n", wine_dbgstr_w(gdi_font->name));
        full_nameW = strdupW(fake_nameW);
    }
    lenfull = (strlenW(full_nameW) + 1) * sizeof(WCHAR);

    /* These names should be read from the TT name table */

    /* length of otmpFamilyName */
    needed += lenfam;

    /* length of otmpFaceName */
    needed += lenface;

    /* length of otmpStyleName */
    needed += lensty;

    /* length of otmpFullName */
    needed += lenfull;


    em_scale = (FT_Fixed)MulDiv(gdi_font->ppem, 1 << 16, ft_face->units_per_EM);

    pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
    if(!pOS2) {
        FIXME("Can't find OS/2 table - not TT font?\n");
	goto end;
    }

    pHori = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea);
    if(!pHori) {
        FIXME("Can't find HHEA table - not TT font?\n");
	goto end;
    }

    pPost = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_post); /* we can live with this failing */

    TRACE("OS/2 winA = %u winD = %u typoA = %d typoD = %d typoLG = %d avgW %d FT_Face a = %d, d = %d, h = %d: HORZ a = %d, d = %d lg = %d maxY = %ld minY = %ld\n",
	  pOS2->usWinAscent, pOS2->usWinDescent,
	  pOS2->sTypoAscender, pOS2->sTypoDescender, pOS2->sTypoLineGap,
	  pOS2->xAvgCharWidth,
	  ft_face->ascender, ft_face->descender, ft_face->height,
	  pHori->Ascender, pHori->Descender, pHori->Line_Gap,
	  ft_face->bbox.yMax, ft_face->bbox.yMin);

    font->potm = HeapAlloc(GetProcessHeap(), 0, needed);
    font->potm->otmSize = needed;

#define TM font->potm->otmTextMetrics

    windescent = get_fixed_windescent(pOS2->usWinDescent);
    if(pOS2->usWinAscent + windescent == 0) {
        ascent = pHori->Ascender;
        descent = -pHori->Descender;
    } else {
        ascent = pOS2->usWinAscent;
        descent = windescent;
    }

    gdi_font->ntmCellHeight = ascent + descent;
    gdi_font->ntmAvgWidth = pOS2->xAvgCharWidth;

#define SCALE_X(x) (pFT_MulFix(x, em_scale))
#define SCALE_Y(y) (pFT_MulFix(y, em_scale))

    if(gdi_font->yMax) {
	TM.tmAscent = gdi_font->yMax;
	TM.tmDescent = -gdi_font->yMin;
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
    TM.tmWeight = FW_REGULAR;
    if (gdi_font->fake_bold)
        TM.tmWeight = FW_BOLD;
    else
    {
        if (ft_face->style_flags & FT_STYLE_FLAG_BOLD)
        {
            if (pOS2->usWeightClass > FW_MEDIUM)
                TM.tmWeight = pOS2->usWeightClass;
        }
        else if (pOS2->usWeightClass <= FW_MEDIUM)
            TM.tmWeight = pOS2->usWeightClass;
    }
    TM.tmOverhang = 0;
    TM.tmDigitizedAspectX = 96; /* FIXME */
    TM.tmDigitizedAspectY = 96; /* FIXME */
    /* It appears that for fonts with SYMBOL_CHARSET Windows always sets
     * symbol range to 0 - f0ff
     */

    if (face_has_symbol_charmap(ft_face) || (pOS2->usFirstCharIndex >= 0xf000 && pOS2->usFirstCharIndex < 0xf100))
    {
        TM.tmFirstChar = 0;
        switch(GetACP())
        {
        case 1255: /* Hebrew */
            TM.tmLastChar = 0xf896;
            break;
        case 1257: /* Baltic */
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
    TM.tmItalic = gdi_font->fake_italic ? 255 : ((ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 255 : 0);
    TM.tmUnderlined = gdi_font->lf.lfUnderline ? 255 : 0;
    TM.tmStruckOut = gdi_font->lf.lfStrikeOut ? 255 : 0;

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
        if (gdi_font->ntmFlags & NTM_PS_OPENTYPE)
            TM.tmPitchAndFamily |= TMPF_DEVICE;
        else
            TM.tmPitchAndFamily |= TMPF_TRUETYPE;
    }

    TM.tmCharSet = gdi_font->charset;

    font->potm->otmFiller = 0;
    memcpy(&font->potm->otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
    font->potm->otmfsSelection = pOS2->fsSelection;
    if (gdi_font->fake_italic)
        font->potm->otmfsSelection |= 1;
    if (gdi_font->fake_bold)
        font->potm->otmfsSelection |= 1 << 5;
    /* Only return valid bits that define embedding and subsetting restrictions */
    font->potm->otmfsType = pOS2->fsType & 0x30e;
    font->potm->otmsCharSlopeRise = pHori->caret_Slope_Rise;
    font->potm->otmsCharSlopeRun = pHori->caret_Slope_Run;
    font->potm->otmItalicAngle = 0; /* POST table */
    font->potm->otmEMSquare = ft_face->units_per_EM;
    font->potm->otmAscent = SCALE_Y(pOS2->sTypoAscender);
    font->potm->otmDescent = SCALE_Y(pOS2->sTypoDescender);
    font->potm->otmLineGap = SCALE_Y(pOS2->sTypoLineGap);
    font->potm->otmsCapEmHeight = SCALE_Y(pOS2->sCapHeight);
    font->potm->otmsXHeight = SCALE_Y(pOS2->sxHeight);
    font->potm->otmrcFontBox.left = SCALE_X(ft_face->bbox.xMin);
    font->potm->otmrcFontBox.right = SCALE_X(ft_face->bbox.xMax);
    font->potm->otmrcFontBox.top = SCALE_Y(ft_face->bbox.yMax);
    font->potm->otmrcFontBox.bottom = SCALE_Y(ft_face->bbox.yMin);
    font->potm->otmMacAscent = TM.tmAscent;
    font->potm->otmMacDescent = -TM.tmDescent;
    font->potm->otmMacLineGap = SCALE_Y(pHori->Line_Gap);
    font->potm->otmusMinimumPPEM = 0; /* TT Header */
    font->potm->otmptSubscriptSize.x = SCALE_X(pOS2->ySubscriptXSize);
    font->potm->otmptSubscriptSize.y = SCALE_Y(pOS2->ySubscriptYSize);
    font->potm->otmptSubscriptOffset.x = SCALE_X(pOS2->ySubscriptXOffset);
    font->potm->otmptSubscriptOffset.y = SCALE_Y(pOS2->ySubscriptYOffset);
    font->potm->otmptSuperscriptSize.x = SCALE_X(pOS2->ySuperscriptXSize);
    font->potm->otmptSuperscriptSize.y = SCALE_Y(pOS2->ySuperscriptYSize);
    font->potm->otmptSuperscriptOffset.x = SCALE_X(pOS2->ySuperscriptXOffset);
    font->potm->otmptSuperscriptOffset.y = SCALE_Y(pOS2->ySuperscriptYOffset);
    font->potm->otmsStrikeoutSize = SCALE_Y(pOS2->yStrikeoutSize);
    font->potm->otmsStrikeoutPosition = SCALE_Y(pOS2->yStrikeoutPosition);
    if(!pPost) {
        font->potm->otmsUnderscoreSize = 0;
	font->potm->otmsUnderscorePosition = 0;
    } else {
        font->potm->otmsUnderscoreSize = SCALE_Y(pPost->underlineThickness);
	font->potm->otmsUnderscorePosition = SCALE_Y(pPost->underlinePosition);
    }
#undef SCALE_X
#undef SCALE_Y
#undef TM

    /* otmp* members should clearly have type ptrdiff_t, but M$ knows best */
    cp = (char*)font->potm + sizeof(*font->potm);
    font->potm->otmpFamilyName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, family_nameW);
    cp += lenfam;
    font->potm->otmpStyleName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, style_nameW);
    cp += lensty;
    font->potm->otmpFaceName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, face_nameW);
	cp += lenface;
    font->potm->otmpFullName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, full_nameW);
    ret = TRUE;

end:
    HeapFree(GetProcessHeap(), 0, style_nameW);
    HeapFree(GetProcessHeap(), 0, family_nameW);
    HeapFree(GetProcessHeap(), 0, face_nameW);
    HeapFree(GetProcessHeap(), 0, full_nameW);
    return ret;
}

/*************************************************************
 * freetype_GetTextMetrics
 */
static BOOL CDECL freetype_GetTextMetrics( struct gdi_font *font, TEXTMETRICW *metrics )
{
    return get_text_metrics( get_font_ptr(font), metrics );
}

/*************************************************************
 * freetype_GetOutlineTextMetrics
 */
static UINT CDECL freetype_GetOutlineTextMetrics( struct gdi_font *gdi_font, UINT cbSize, OUTLINETEXTMETRICW *potm )
{
    GdiFont *font = get_font_ptr(gdi_font);
    UINT ret = 0;

    TRACE("font=%p\n", font);

    if (font->potm || get_outline_text_metrics( font ))
    {
        if(potm && cbSize >= font->potm->otmSize)
        {
	    memcpy(potm, font->potm, font->potm->otmSize);
            scale_outline_font_metrics(font, potm);
        }
	ret = font->potm->otmSize;
    }
    return ret;
}

static BOOL load_child_font(GdiFont *font, CHILD_FONT *child)
{
    struct gdi_font *gdi_font = font->gdi_font;
    const struct list *face_list;
    Face *child_face = NULL, *best_face = NULL;
    UINT penalty = 0, new_penalty = 0;
    BOOL bold, italic, bd, it;

    italic = !!gdi_font->lf.lfItalic;
    bold = gdi_font->lf.lfWeight > FW_MEDIUM;

    face_list = get_face_list_from_family( child->face->family );
    LIST_FOR_EACH_ENTRY( child_face, face_list, Face, entry )
    {
        it = !!(child_face->ntmFlags & NTM_ITALIC);
        bd = !!(child_face->ntmFlags & NTM_BOLD);
        new_penalty = ( it ^ italic ) + ( bd ^ bold );
        if (!best_face || new_penalty < penalty)
        {
            penalty = new_penalty;
            best_face = child_face;
        }
    }
    child_face = best_face ? best_face : child->face;

    child->font = get_font_ptr( alloc_gdi_font() );
    child->font->ft_face = OpenFontFace( child->font, child_face, 0, -gdi_font->ppem );
    if(!child->font->ft_face)
    {
        free_gdi_font(child->font->gdi_font);
        child->font = NULL;
        return FALSE;
    }

    child->font->gdi_font->fake_italic = italic && !( child_face->ntmFlags & NTM_ITALIC );
    child->font->gdi_font->fake_bold = bold && !( child_face->ntmFlags & NTM_BOLD );
    child->font->gdi_font->lf = gdi_font->lf;
    child->font->gdi_font->matrix = gdi_font->matrix;
    child->font->gdi_font->can_use_bitmap = gdi_font->can_use_bitmap;
    child->font->gdi_font->ntmFlags = child_face->ntmFlags;
    child->font->gdi_font->aa_flags = HIWORD( child_face->flags );
    child->font->gdi_font->scale_y = gdi_font->scale_y;
    set_gdi_font_name( child->font->gdi_font, child_face->family->family_name );
    child->font->base_font = font;
    TRACE("created child font %p for base %p\n", child->font, font);
    return TRUE;
}

static BOOL get_glyph_index_linked(GdiFont *font, UINT c, GdiFont **linked_font, FT_UInt *glyph, BOOL* vert)
{
    FT_UInt g,o;
    CHILD_FONT *child_font;

    if(font->base_font)
        font = font->base_font;

    *linked_font = font;

    if((*glyph = get_glyph_index(font, c)))
    {
        o = *glyph;
        *glyph = get_GSUB_vert_glyph(font, *glyph);
        *vert = (o != *glyph);
        return TRUE;
    }

    if (c < 32) goto done;  /* don't check linked fonts for control characters */

    LIST_FOR_EACH_ENTRY(child_font, &font->child_fonts, CHILD_FONT, entry)
    {
        if(!child_font->font)
            if(!load_child_font(font, child_font))
                continue;

        if(!child_font->font->ft_face)
            continue;
        g = get_glyph_index(child_font->font, c);
        o = g;
        g = get_GSUB_vert_glyph(child_font->font, g);
        if(g)
        {
            *glyph = g;
            *linked_font = child_font->font;
            *vert = (o != g);
            return TRUE;
        }
    }

done:
    *vert = FALSE;
    return FALSE;
}

/*************************************************************
 * freetype_GetCharWidthInfo
 */
static BOOL CDECL freetype_GetCharWidthInfo( struct gdi_font *gdi_font, struct char_width_info *info )
{
    GdiFont *font = get_font_ptr(gdi_font);
    TT_HoriHeader *pHori;

    TRACE("%p, %p\n", font, info);

    if (gdi_font->scalable &&
        (pHori = pFT_Get_Sfnt_Table(font->ft_face, ft_sfnt_hhea)))
    {
        FT_Fixed em_scale;
        em_scale = MulDiv(gdi_font->ppem, 1 << 16, font->ft_face->units_per_EM);
        info->lsb = (SHORT)pFT_MulFix(pHori->min_Left_Side_Bearing,  em_scale);
        info->rsb = (SHORT)pFT_MulFix(pHori->min_Right_Side_Bearing, em_scale);
    }
    else
        info->lsb = info->rsb = 0;

    info->unk = 0;

    return TRUE;
}


/* Retrieve a list of supported Unicode ranges for a given font.
 * Can be called with NULL gs to calculate the buffer size. Returns
 * the number of ranges found.
 */
static DWORD get_font_unicode_ranges(FT_Face face, GLYPHSET *gs)
{
    DWORD num_ranges = 0;

    if (face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt glyph_code;
        FT_ULong char_code, char_code_prev;

        glyph_code = 0;
        char_code_prev = char_code = pFT_Get_First_Char(face, &glyph_code);

        TRACE("face encoding FT_ENCODING_UNICODE, number of glyphs %ld, first glyph %u, first char %04lx\n",
               face->num_glyphs, glyph_code, char_code);

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
            char_code = pFT_Get_Next_Char(face, char_code, &glyph_code);
        }
    }
    else
    {
        DWORD encoding = RtlUlongByteSwap(face->charmap->encoding);
        FIXME("encoding %s not supported\n", debugstr_an((char *)&encoding, 4));
    }

    return num_ranges;
}

/*************************************************************
 * freetype_GetFontUnicodeRanges
 */
static DWORD CDECL freetype_GetFontUnicodeRanges( struct gdi_font *font, GLYPHSET *glyphset )
{
    DWORD size, num_ranges;

    num_ranges = get_font_unicode_ranges(get_font_ptr(font)->ft_face, glyphset);
    size = sizeof(GLYPHSET) + sizeof(WCRANGE) * (num_ranges - 1);
    if (glyphset)
    {
        glyphset->cbThis = size;
        glyphset->cRanges = num_ranges;
        glyphset->flAccel = 0;
    }
    return size;
}

/*************************************************************
 * freetype_FontIsLinked
 */
static BOOL CDECL freetype_FontIsLinked( struct gdi_font *font )
{
    return !list_empty( &get_font_ptr(font)->child_fonts );
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

static DWORD parse_format0_kern_subtable(GdiFont *font,
                                         const struct TT_format0_kern_subtable *tt_f0_ks,
                                         const USHORT *glyph_to_char,
                                         KERNINGPAIR *kern_pair, DWORD cPairs)
{
    struct gdi_font *gdi_font = font->gdi_font;
    USHORT i, nPairs;
    const struct TT_kern_pair *tt_kern_pair;

    TRACE("font height %d, units_per_EM %d\n", gdi_font->ppem, font->ft_face->units_per_EM);

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
        kern_pair->iKernAmount = (short)GET_BE_WORD(tt_kern_pair[i].value) * gdi_font->ppem;
        if (kern_pair->iKernAmount < 0)
        {
            kern_pair->iKernAmount -= font->ft_face->units_per_EM / 2;
            kern_pair->iKernAmount -= gdi_font->ppem;
        }
        else if (kern_pair->iKernAmount > 0)
        {
            kern_pair->iKernAmount += font->ft_face->units_per_EM / 2;
            kern_pair->iKernAmount += gdi_font->ppem;
        }
        kern_pair->iKernAmount /= font->ft_face->units_per_EM;

        TRACE("left %u right %u value %d\n",
               kern_pair->wFirst, kern_pair->wSecond, kern_pair->iKernAmount);

        kern_pair++;
    }
    TRACE("copied %u entries\n", nPairs);
    return nPairs;
}

/*************************************************************
 * freetype_GetKerningPairs
 */
static DWORD CDECL freetype_GetKerningPairs( struct gdi_font *gdi_font, DWORD cPairs, KERNINGPAIR *kern_pair )
{
    GdiFont *font = get_font_ptr(gdi_font);
    DWORD length;
    void *buf;
    const struct TT_kern_table *tt_kern_table;
    const struct TT_kern_subtable *tt_kern_subtable;
    USHORT i, nTables;
    USHORT *glyph_to_char;

    if (font->total_kern_pairs != (DWORD)-1)
    {
        if (cPairs && kern_pair)
        {
            cPairs = min(cPairs, font->total_kern_pairs);
            memcpy(kern_pair, font->kern_pairs, cPairs * sizeof(*kern_pair));
        }
        else cPairs = font->total_kern_pairs;
        return cPairs;
    }

    font->total_kern_pairs = 0;

    length = freetype_get_font_data(gdi_font, MS_KERN_TAG, 0, NULL, 0);

    if (length == GDI_ERROR)
    {
        TRACE("no kerning data in the font\n");
        return 0;
    }

    buf = HeapAlloc(GetProcessHeap(), 0, length);
    if (!buf) return 0;

    freetype_get_font_data(gdi_font, MS_KERN_TAG, 0, buf, length);

    /* build a glyph index to char code map */
    glyph_to_char = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(USHORT) * 65536);
    if (!glyph_to_char)
    {
        HeapFree(GetProcessHeap(), 0, buf);
        return 0;
    }

    if (font->ft_face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt glyph_code;
        FT_ULong char_code;

        glyph_code = 0;
        char_code = pFT_Get_First_Char(font->ft_face, &glyph_code);

        TRACE("face encoding FT_ENCODING_UNICODE, number of glyphs %ld, first glyph %u, first char %lu\n",
               font->ft_face->num_glyphs, glyph_code, char_code);

        while (glyph_code)
        {
            /*TRACE("Char %04lX -> Index %u%s\n", char_code, glyph_code, glyph_to_char[glyph_code] ? "  !" : "" );*/

            /* FIXME: This doesn't match what Windows does: it does some fancy
             * things with duplicate glyph index to char code mappings, while
             * we just avoid overriding existing entries.
             */
            if (glyph_code <= 65535 && !glyph_to_char[glyph_code])
                glyph_to_char[glyph_code] = (USHORT)char_code;

            char_code = pFT_Get_Next_Char(font->ft_face, char_code, &glyph_code);
        }
    }
    else
    {
        DWORD encoding = RtlUlongByteSwap(font->ft_face->charmap->encoding);
        ULONG n;

        FIXME("encoding %s not supported\n", debugstr_an((char *)&encoding, 4));
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
            DWORD new_chunk, old_total = font->total_kern_pairs;

            new_chunk = parse_format0_kern_subtable(font, (const struct TT_format0_kern_subtable *)(tt_kern_subtable + 1),
                                                    glyph_to_char, NULL, 0);
            font->total_kern_pairs += new_chunk;

            if (!font->kern_pairs)
                font->kern_pairs = HeapAlloc(GetProcessHeap(), 0,
                                             font->total_kern_pairs * sizeof(*font->kern_pairs));
            else
                font->kern_pairs = HeapReAlloc(GetProcessHeap(), 0, font->kern_pairs,
                                               font->total_kern_pairs * sizeof(*font->kern_pairs));

            parse_format0_kern_subtable(font, (const struct TT_format0_kern_subtable *)(tt_kern_subtable + 1),
                        glyph_to_char, font->kern_pairs + old_total, new_chunk);
        }
        else
            TRACE("skipping kerning table format %u\n", tt_kern_subtable_copy.coverage.bits.format);

        tt_kern_subtable = (const struct TT_kern_subtable *)((const char *)tt_kern_subtable + tt_kern_subtable_copy.length);
    }

    HeapFree(GetProcessHeap(), 0, glyph_to_char);
    HeapFree(GetProcessHeap(), 0, buf);

    if (cPairs && kern_pair)
    {
        cPairs = min(cPairs, font->total_kern_pairs);
        memcpy(kern_pair, font->kern_pairs, cPairs * sizeof(*kern_pair));
    }
    else cPairs = font->total_kern_pairs;
    return cPairs;
}

static const struct font_backend_funcs font_funcs =
{
    freetype_EnumFonts,
    freetype_FontIsLinked,
    freetype_GetCharWidthInfo,
    freetype_GetFontUnicodeRanges,
    freetype_GetKerningPairs,
    freetype_GetOutlineTextMetrics,
    freetype_GetTextMetrics,
    freetype_SelectFont,
    freetype_AddFontResourceEx,
    freetype_RemoveFontResourceEx,
    freetype_AddFontMemResourceEx,
    freetype_CreateScalableFontResource,
    freetype_alloc_font,
    freetype_get_font_data,
    freetype_get_glyph_index,
    freetype_get_default_glyph,
    freetype_get_glyph_outline,
    freetype_destroy_font
};

#else /* HAVE_FREETYPE */

/*************************************************************************/

BOOL WineEngInit( const struct font_backend_funcs **funcs )
{
    return FALSE;
}

#endif /* HAVE_FREETYPE */
