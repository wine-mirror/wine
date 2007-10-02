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
#include <dirent.h>
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
#undef DPRINTF
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

WINE_DEFAULT_DEBUG_CHANNEL(font);

#ifdef HAVE_FREETYPE

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#endif
#ifdef HAVE_FREETYPE_FREETYPE_H
#include <freetype/freetype.h>
#endif
#ifdef HAVE_FREETYPE_FTGLYPH_H
#include <freetype/ftglyph.h>
#endif
#ifdef HAVE_FREETYPE_TTTABLES_H
#include <freetype/tttables.h>
#endif
#ifdef HAVE_FREETYPE_FTTYPES_H
#include <freetype/fttypes.h>
#endif
#ifdef HAVE_FREETYPE_FTSNAMES_H
#include <freetype/ftsnames.h>
#else
# ifdef HAVE_FREETYPE_FTNAMES_H
# include <freetype/ftnames.h>
# endif
#endif
#ifdef HAVE_FREETYPE_TTNAMEID_H
#include <freetype/ttnameid.h>
#endif
#ifdef HAVE_FREETYPE_FTOUTLN_H
#include <freetype/ftoutln.h>
#endif
#ifdef HAVE_FREETYPE_INTERNAL_SFNT_H
#include <freetype/internal/sfnt.h>
#endif
#ifdef HAVE_FREETYPE_FTTRIGON_H
#include <freetype/fttrigon.h>
#endif
#ifdef HAVE_FREETYPE_FTWINFNT_H
#include <freetype/ftwinfnt.h>
#endif
#ifdef HAVE_FREETYPE_FTMODAPI_H
#include <freetype/ftmodapi.h>
#endif

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

static void *ft_handle = NULL;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(FT_Vector_Unit);
MAKE_FUNCPTR(FT_Done_Face);
MAKE_FUNCPTR(FT_Get_Char_Index);
MAKE_FUNCPTR(FT_Get_Module);
MAKE_FUNCPTR(FT_Get_Sfnt_Name);
MAKE_FUNCPTR(FT_Get_Sfnt_Name_Count);
MAKE_FUNCPTR(FT_Get_Sfnt_Table);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Load_Glyph);
MAKE_FUNCPTR(FT_Matrix_Multiply);
MAKE_FUNCPTR(FT_MulFix);
MAKE_FUNCPTR(FT_New_Face);
MAKE_FUNCPTR(FT_New_Memory_Face);
MAKE_FUNCPTR(FT_Outline_Get_Bitmap);
MAKE_FUNCPTR(FT_Outline_Transform);
MAKE_FUNCPTR(FT_Outline_Translate);
MAKE_FUNCPTR(FT_Select_Charmap);
MAKE_FUNCPTR(FT_Set_Charmap);
MAKE_FUNCPTR(FT_Set_Pixel_Sizes);
MAKE_FUNCPTR(FT_Vector_Transform);
static void (*pFT_Library_Version)(FT_Library,FT_Int*,FT_Int*,FT_Int*);
static FT_Error (*pFT_Load_Sfnt_Table)(FT_Face,FT_ULong,FT_Long,FT_Byte*,FT_ULong*);
static FT_ULong (*pFT_Get_First_Char)(FT_Face,FT_UInt*);
static FT_ULong (*pFT_Get_Next_Char)(FT_Face,FT_ULong,FT_UInt*);
static FT_TrueTypeEngineType (*pFT_Get_TrueType_Engine_Type)(FT_Library);
#ifdef HAVE_FREETYPE_FTWINFNT_H
MAKE_FUNCPTR(FT_Get_WinFNT_Header);
#endif

#ifdef SONAME_LIBFONTCONFIG
#include <fontconfig/fontconfig.h>
MAKE_FUNCPTR(FcConfigGetCurrent);
MAKE_FUNCPTR(FcFontList);
MAKE_FUNCPTR(FcFontSetDestroy);
MAKE_FUNCPTR(FcInit);
MAKE_FUNCPTR(FcObjectSetAdd);
MAKE_FUNCPTR(FcObjectSetCreate);
MAKE_FUNCPTR(FcObjectSetDestroy);
MAKE_FUNCPTR(FcPatternCreate);
MAKE_FUNCPTR(FcPatternDestroy);
MAKE_FUNCPTR(FcPatternGetBool);
MAKE_FUNCPTR(FcPatternGetString);
#endif

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
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#endif

/* This is bascially a copy of FT_Bitmap_Size with an extra element added */
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

typedef struct tagFace {
    struct list entry;
    WCHAR *StyleName;
    char *file;
    void *font_data_ptr;
    DWORD font_data_size;
    FT_Long face_index;
    BOOL Italic;
    BOOL Bold;
    FONTSIGNATURE fs;
    FONTSIGNATURE fs_links;
    DWORD ntmFlags;  /* Only some bits stored here. Others are computed on the fly */
    FT_Fixed font_version;
    BOOL scalable;
    Bitmap_Size size;     /* set if face is a bitmap */
    BOOL external; /* TRUE if we should manually add this font to the registry */
    struct tagFamily *family;
} Face;

typedef struct tagFamily {
    struct list entry;
    const WCHAR *FamilyName;
    struct list faces;
} Family;

typedef struct {
    GLYPHMETRICS gm;
    INT adv; /* These three hold to widths of the unrotated chars */
    INT lsb;
    INT bbx;
    BOOL init;
} GM;

typedef struct {
    FLOAT eM11, eM12;
    FLOAT eM21, eM22;
} FMAT2;

typedef struct {
    DWORD hash;
    LOGFONTW lf;
    FMAT2 matrix;
    BOOL can_use_bitmap;
} FONT_DESC;

typedef struct tagHFONTLIST {
    struct list entry;
    HFONT hfont;
} HFONTLIST;

typedef struct {
    struct list entry;
    Face *face;
    GdiFont *font;
} CHILD_FONT;

struct tagGdiFont {
    struct list entry;
    FT_Face ft_face;
    struct font_mapping *mapping;
    LPWSTR name;
    int charset;
    int codepage;
    BOOL fake_italic;
    BOOL fake_bold;
    BYTE underline;
    BYTE strikeout;
    INT orientation;
    GM **gm;
    DWORD gmsize;
    struct list hfontlist;
    FONT_DESC font_desc;
    LONG aveWidth;
    SHORT yMax;
    SHORT yMin;
    OUTLINETEXTMETRICW *potm;
    DWORD ntmFlags;
    DWORD total_kern_pairs;
    KERNINGPAIR *kern_pairs;
    FONTSIGNATURE fs;
    GdiFont *base_font;
    struct list child_fonts;
    LONG ppem;
};

typedef struct {
    struct list entry;
    const WCHAR *font_name;
    struct list links;
} SYSTEM_LINKS;

#define GM_BLOCK_SIZE 128
#define FONT_GM(font,idx) (&(font)->gm[(idx) / GM_BLOCK_SIZE][(idx) % GM_BLOCK_SIZE])

static struct list gdi_font_list = LIST_INIT(gdi_font_list);
static struct list unused_gdi_font_list = LIST_INIT(unused_gdi_font_list);
#define UNUSED_CACHE_SIZE 10
static struct list child_font_list = LIST_INIT(child_font_list);
static struct list system_links = LIST_INIT(system_links);

static struct list font_subst_list = LIST_INIT(font_subst_list);

static struct list font_list = LIST_INIT(font_list);

static const WCHAR defSerif[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n','\0'};
static const WCHAR defSans[] = {'A','r','i','a','l','\0'};
static const WCHAR defFixed[] = {'C','o','u','r','i','e','r',' ','N','e','w','\0'};

static const WCHAR RegularW[] = {'R','e','g','u','l','a','r','\0'};

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

static const WCHAR * const SystemFontValues[4] = {
    System_Value,
    OEMFont_Value,
    FixedSys_Value,
    NULL
};

static const WCHAR external_fonts_reg_key[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                               'F','o','n','t','s','\\','E','x','t','e','r','n','a','l',' ','F','o','n','t','s','\0'};

static const WCHAR ArabicW[] = {'A','r','a','b','i','c','\0'};
static const WCHAR BalticW[] = {'B','a','l','t','i','c','\0'};
static const WCHAR CHINESE_BIG5W[] = {'C','H','I','N','E','S','E','_','B','I','G','5','\0'};
static const WCHAR CHINESE_GB2312W[] = {'C','H','I','N','E','S','E','_','G','B','2','3','1','2','\0'};
static const WCHAR Central_EuropeanW[] = {'C','e','n','t','r','a','l',' ',
				    'E','u','r','o','p','e','a','n','\0'};
static const WCHAR CyrillicW[] = {'C','y','r','i','l','l','i','c','\0'};
static const WCHAR GreekW[] = {'G','r','e','e','k','\0'};
static const WCHAR HangulW[] = {'H','a','n','g','u','l','\0'};
static const WCHAR Hangul_Johab_W[] = {'H','a','n','g','u','l','(','J','o','h','a','b',')','\0'};
static const WCHAR HebrewW[] = {'H','e','b','r','e','w','\0'};
static const WCHAR JapaneseW[] = {'J','a','p','a','n','e','s','e','\0'};
static const WCHAR SymbolW[] = {'S','y','m','b','o','l','\0'};
static const WCHAR ThaiW[] = {'T','h','a','i','\0'};
static const WCHAR TurkishW[] = {'T','u','r','k','i','s','h','\0'};
static const WCHAR VietnameseW[] = {'V','i','e','t','n','a','m','e','s','e','\0'};
static const WCHAR WesternW[] = {'W','e','s','t','e','r','n','\0'};
static const WCHAR OEM_DOSW[] = {'O','E','M','/','D','O','S','\0'};

static const WCHAR * const ElfScriptsW[32] = { /* these are in the order of the fsCsb[0] bits */
    WesternW, /*00*/
    Central_EuropeanW,
    CyrillicW,
    GreekW,
    TurkishW,
    HebrewW,
    ArabicW,
    BalticW,
    VietnameseW, /*08*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, /*15*/
    ThaiW,
    JapaneseW,
    CHINESE_GB2312W,
    HangulW,
    CHINESE_BIG5W,
    Hangul_Johab_W,
    NULL, NULL, /*23*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    SymbolW /*31*/
};

typedef struct {
    WCHAR *name;
    INT charset;
} NameCs;

typedef struct tagFontSubst {
    struct list entry;
    NameCs from;
    NameCs to;
} FontSubst;

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

static BOOL have_installed_roman_font = FALSE; /* CreateFontInstance will fail if this is still FALSE */

static const WCHAR font_mutex_nameW[] = {'_','_','W','I','N','E','_','F','O','N','T','_','M','U','T','E','X','_','_','\0'};

static BOOL get_glyph_index_linked(GdiFont *font, UINT c, GdiFont **linked_font, FT_UInt *glyph);

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
 *                  (HKLM\\Software\\Microsft\\Windows NT\\CurrentVersion\\FontDPI
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
    SInt16 res_ref;
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
   This function builds an FT_Fixed from a float. It puts the integer part
   in the highest 16 bits and the decimal part in the lowest 16 bits of the FT_Fixed.
   It fails if the integer part of the float number is greater than SHORT_MAX.
*/
static inline FT_Fixed FT_FixedFromFloat(float f)
{
	short value = f;
	unsigned short fract = (f - value) * 0xFFFF;
	return (FT_Fixed)((long)value << 16 | (unsigned long)fract);
}

/* 
   This function builds an FT_Fixed from a FIXED. It simply put f.value 
   in the highest 16 bits and f.fract in the lowest 16 bits of the FT_Fixed.
*/
static inline FT_Fixed FT_FixedFromFIXED(FIXED f)
{
	return (FT_Fixed)((long)f.value << 16 | (unsigned long)f.fract);
}


static Face *find_face_from_filename(const WCHAR *file_name, const WCHAR *face_name)
{
    Family *family;
    Face *face;
    const char *file;
    DWORD len = WideCharToMultiByte(CP_UNIXCP, 0, file_name, -1, NULL, 0, NULL, NULL);
    char *file_nameA = HeapAlloc(GetProcessHeap(), 0, len);

    WideCharToMultiByte(CP_UNIXCP, 0, file_name, -1, file_nameA, len, NULL, NULL);
    TRACE("looking for file %s name %s\n", debugstr_a(file_nameA), debugstr_w(face_name));

    LIST_FOR_EACH_ENTRY(family, &font_list, Family, entry)
    {
        if(face_name && strcmpiW(face_name, family->FamilyName))
            continue;
        LIST_FOR_EACH_ENTRY(face, &family->faces, Face, entry)
        {
            if (!face->file)
                continue;
            file = strrchr(face->file, '/');
            if(!file)
                file = face->file;
            else
                file++;
            if(!strcasecmp(file, file_nameA))
            {
                HeapFree(GetProcessHeap(), 0, file_nameA);
                return face;
            }
	}
    }
    HeapFree(GetProcessHeap(), 0, file_nameA);
    return NULL;
}

static Family *find_family_from_name(const WCHAR *name)
{
    Family *family;

    LIST_FOR_EACH_ENTRY(family, &font_list, Family, entry)
    {
        if(!strcmpiW(family->FamilyName, name))
            return family;
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
    return;
}

static LPWSTR strdupW(LPCWSTR p)
{
    LPWSTR ret;
    DWORD len = (strlenW(p) + 1) * sizeof(WCHAR);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(ret, p, len);
    return ret;
}

static LPSTR strdupA(LPCSTR p)
{
    LPSTR ret;
    DWORD len = (strlen(p) + 1);
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
        HeapFree(GetProcessHeap(), 0, &from_exist->from.name);
        HeapFree(GetProcessHeap(), 0, &from_exist->to.name);
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

static void split_subst_info(NameCs *nc, LPSTR str)
{
    CHAR *p = strrchr(str, ',');
    DWORD len;

    nc->charset = -1;
    if(p && *(p+1)) {
        nc->charset = strtol(p+1, NULL, 10);
	*p = '\0';
    }
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    nc->name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, nc->name, len);
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
	    if((psub->to.charset != psub->from.charset) ||
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

static WCHAR *get_familyname(FT_Face ft_face)
{
    WCHAR *family = NULL;
    FT_SfntName name;
    FT_UInt num_names, name_index, i;

    if(FT_IS_SFNT(ft_face))
    {
        num_names = pFT_Get_Sfnt_Name_Count(ft_face);

        for(name_index = 0; name_index < num_names; name_index++)
        {
            if(!pFT_Get_Sfnt_Name(ft_face, name_index, &name))
            {
                if((name.name_id == TT_NAME_ID_FONT_FAMILY) &&
                   (name.language_id == GetUserDefaultLCID()) &&
                   (name.platform_id == TT_PLATFORM_MICROSOFT) &&
                   (name.encoding_id == TT_MS_ID_UNICODE_CS))
                {
                    /* String is not nul terminated and string_len is a byte length. */
                    family = HeapAlloc(GetProcessHeap(), 0, name.string_len + 2);
                    for(i = 0; i < name.string_len / 2; i++)
                    {
                        WORD *tmp = (WORD *)&name.string[i * 2];
                        family[i] = GET_BE_WORD(*tmp);
                    }
                    family[i] = 0;

                    TRACE("Got localised name %s\n", debugstr_w(family));
                    return family;
                }
            }
        }
    }

    return NULL;
}


#define ADDFONT_EXTERNAL_FONT 0x01
#define ADDFONT_FORCE_BITMAP  0x02
static INT AddFontToList(const char *file, void *font_data_ptr, DWORD font_data_size, char *fake_family, const WCHAR *target_family, DWORD flags)
{
    FT_Face ft_face;
    TT_OS2 *pOS2;
    TT_Header *pHeader = NULL;
    WCHAR *english_family, *localised_family, *StyleW;
    DWORD len;
    Family *family;
    Face *face;
    struct list *family_elem_ptr, *face_elem_ptr;
    FT_Error err;
    FT_Long face_index = 0, num_faces;
#ifdef HAVE_FREETYPE_FTWINFNT_H
    FT_WinFNT_HeaderRec winfnt_header;
#endif
    int i, bitmap_num, internal_leading;
    FONTSIGNATURE fs;

    /* we always load external fonts from files - otherwise we would get a crash in update_reg_entries */
    assert(file || !(flags & ADDFONT_EXTERNAL_FONT));

#ifdef HAVE_CARBON_CARBON_H
    if(file && !fake_family)
    {
        char **mac_list = expand_mac_font(file);
        if(mac_list)
        {
            BOOL had_one = FALSE;
            char **cursor;
            for(cursor = mac_list; *cursor; cursor++)
            {
                had_one = TRUE;
                AddFontToList(*cursor, NULL, 0, NULL, NULL, flags);
                HeapFree(GetProcessHeap(), 0, *cursor);
            }
            HeapFree(GetProcessHeap(), 0, mac_list);
            if(had_one)
                return 1;
        }
    }
#endif /* HAVE_CARBON_CARBON_H */

    do {
        char *family_name = fake_family;

        if (file)
        {
            TRACE("Loading font file %s index %ld\n", debugstr_a(file), face_index);
            err = pFT_New_Face(library, file, face_index, &ft_face);
        } else
        {
            TRACE("Loading font from ptr %p size %d, index %ld\n", font_data_ptr, font_data_size, face_index);
            err = pFT_New_Memory_Face(library, font_data_ptr, font_data_size, face_index, &ft_face);
        }

	if(err != 0) {
	    WARN("Unable to load font %s/%p err = %x\n", debugstr_a(file), font_data_ptr, err);
	    return 0;
	}

	if(!FT_IS_SFNT(ft_face) && (FT_IS_SCALABLE(ft_face) || !(flags & ADDFONT_FORCE_BITMAP))) { /* for now we'll accept TT/OT or bitmap fonts*/
	    WARN("Ignoring font %s/%p\n", debugstr_a(file), font_data_ptr);
	    pFT_Done_Face(ft_face);
	    return 0;
	}

        /* There are too many bugs in FreeType < 2.1.9 for bitmap font support */
        if(!FT_IS_SCALABLE(ft_face) && FT_SimpleVersion < ((2 << 16) | (1 << 8) | (9 << 0))) {
	    WARN("FreeType version < 2.1.9, skipping bitmap font %s/%p\n", debugstr_a(file), font_data_ptr);
	    pFT_Done_Face(ft_face);
	    return 0;
	}

	if(FT_IS_SFNT(ft_face) && (!pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2) ||
	   !pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea) ||
           !(pHeader = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_head)))) {
	    TRACE("Font %s/%p lacks either an OS2, HHEA or HEAD table.\n"
		  "Skipping this font.\n", debugstr_a(file), font_data_ptr);
	    pFT_Done_Face(ft_face);
	    return 0;
	}

        if(!ft_face->family_name || !ft_face->style_name) {
            TRACE("Font %s/%p lacks either a family or style name\n", debugstr_a(file), font_data_ptr);
            pFT_Done_Face(ft_face);
            return 0;
        }

        if (target_family)
        {
            localised_family = get_familyname(ft_face);
            if (localised_family && strcmpiW(localised_family,target_family)!=0)
            {
                TRACE("Skipping Index %i: Incorrect Family name for replacement\n",(INT)face_index);
                HeapFree(GetProcessHeap(), 0, localised_family);
                num_faces = ft_face->num_faces;
                pFT_Done_Face(ft_face);
                continue;
            }
            HeapFree(GetProcessHeap(), 0, localised_family);
        }

        if(!family_name)
            family_name = ft_face->family_name;

        bitmap_num = 0;
        do {
            My_FT_Bitmap_Size *size = NULL;
            FT_ULong tmp_size;

            if(!FT_IS_SCALABLE(ft_face))
                size = (My_FT_Bitmap_Size *)ft_face->available_sizes + bitmap_num;

            len = MultiByteToWideChar(CP_ACP, 0, family_name, -1, NULL, 0);
            english_family = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, family_name, -1, english_family, len);

            localised_family = NULL;
            if(!fake_family) {
                localised_family = get_familyname(ft_face);
                if(localised_family && !strcmpW(localised_family, english_family)) {
                    HeapFree(GetProcessHeap(), 0, localised_family);
                    localised_family = NULL;
                }
            }

            family = NULL;
            LIST_FOR_EACH(family_elem_ptr, &font_list) {
                family = LIST_ENTRY(family_elem_ptr, Family, entry);
                if(!strcmpW(family->FamilyName, localised_family ? localised_family : english_family))
                    break;
                family = NULL;
            }
            if(!family) {
                family = HeapAlloc(GetProcessHeap(), 0, sizeof(*family));
                family->FamilyName = strdupW(localised_family ? localised_family : english_family);
                list_init(&family->faces);
                list_add_tail(&font_list, &family->entry);

                if(localised_family) {
                    FontSubst *subst = HeapAlloc(GetProcessHeap(), 0, sizeof(*subst));
                    subst->from.name = strdupW(english_family);
                    subst->from.charset = -1;
                    subst->to.name = strdupW(localised_family);
                    subst->to.charset = -1;
                    add_font_subst(&font_subst_list, subst, 0);
                }
            }
            HeapFree(GetProcessHeap(), 0, localised_family);
            HeapFree(GetProcessHeap(), 0, english_family);

            len = MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, NULL, 0);
            StyleW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, StyleW, len);

            internal_leading = 0;
            memset(&fs, 0, sizeof(fs));

            pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
            if(pOS2) {
                fs.fsCsb[0] = pOS2->ulCodePageRange1;
                fs.fsCsb[1] = pOS2->ulCodePageRange2;
                fs.fsUsb[0] = pOS2->ulUnicodeRange1;
                fs.fsUsb[1] = pOS2->ulUnicodeRange2;
                fs.fsUsb[2] = pOS2->ulUnicodeRange3;
                fs.fsUsb[3] = pOS2->ulUnicodeRange4;
                if(pOS2->version == 0) {
                    FT_UInt dummy;

                    if(!pFT_Get_First_Char || (pFT_Get_First_Char( ft_face, &dummy ) < 0x100))
                        fs.fsCsb[0] |= 1;
                    else
                        fs.fsCsb[0] |= 1L << 31;
                }
            }
#ifdef HAVE_FREETYPE_FTWINFNT_H
            else if(pFT_Get_WinFNT_Header && !pFT_Get_WinFNT_Header(ft_face, &winfnt_header)) {
                CHARSETINFO csi;
                TRACE("pix_h %d charset %d dpi %dx%d pt %d\n", winfnt_header.pixel_height, winfnt_header.charset,
                      winfnt_header.vertical_resolution,winfnt_header.horizontal_resolution, winfnt_header.nominal_point_size);
                if(TranslateCharsetInfo((DWORD*)(UINT_PTR)winfnt_header.charset, &csi, TCI_SRCCHARSET))
                    memcpy(&fs, &csi.fs, sizeof(csi.fs));
                internal_leading = winfnt_header.internal_leading;
            }
#endif

            face_elem_ptr = list_head(&family->faces);
            while(face_elem_ptr) {
                face = LIST_ENTRY(face_elem_ptr, Face, entry);
                face_elem_ptr = list_next(&family->faces, face_elem_ptr);
                if(!strcmpW(face->StyleName, StyleW) &&
                   (FT_IS_SCALABLE(ft_face) || ((size->y_ppem == face->size.y_ppem) && !memcmp(&fs, &face->fs, sizeof(fs)) ))) {
                    TRACE("Already loaded font %s %s original version is %lx, this version is %lx\n",
                          debugstr_w(family->FamilyName), debugstr_w(StyleW),
                          face->font_version,  pHeader ? pHeader->Font_Revision : 0);

                    if(fake_family) {
                        TRACE("This font is a replacement but the original really exists, so we'll skip the replacement\n");
                        HeapFree(GetProcessHeap(), 0, StyleW);
                        pFT_Done_Face(ft_face);
                        return 1;
                    }
                    if(!pHeader || pHeader->Font_Revision <= face->font_version) {
                        TRACE("Original font is newer so skipping this one\n");
                        HeapFree(GetProcessHeap(), 0, StyleW);
                        pFT_Done_Face(ft_face);
                        return 1;
                    } else {
                        TRACE("Replacing original with this one\n");
                        list_remove(&face->entry);
                        HeapFree(GetProcessHeap(), 0, face->file);
                        HeapFree(GetProcessHeap(), 0, face->StyleName);
                        HeapFree(GetProcessHeap(), 0, face);
                        break;
                    }
                }
            }
            face = HeapAlloc(GetProcessHeap(), 0, sizeof(*face));
            list_add_tail(&family->faces, &face->entry);
            face->StyleName = StyleW;
            if (file)
            {
                face->file = strdupA(file);
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
            face->Italic = (ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 1 : 0;
            face->Bold = (ft_face->style_flags & FT_STYLE_FLAG_BOLD) ? 1 : 0;
            face->font_version = pHeader ? pHeader->Font_Revision : 0;
            face->family = family;
            face->external = (flags & ADDFONT_EXTERNAL_FONT) ? TRUE : FALSE;
            memcpy(&face->fs, &fs, sizeof(face->fs));
            memset(&face->fs_links, 0, sizeof(face->fs_links));

            if(FT_IS_SCALABLE(ft_face)) {
                memset(&face->size, 0, sizeof(face->size));
                face->scalable = TRUE;
            } else {
                TRACE("Adding bitmap size h %d w %d size %ld x_ppem %ld y_ppem %ld\n",
                      size->height, size->width, size->size >> 6,
                      size->x_ppem >> 6, size->y_ppem >> 6);
                face->size.height = size->height;
                face->size.width = size->width;
                face->size.size = size->size;
                face->size.x_ppem = size->x_ppem;
                face->size.y_ppem = size->y_ppem;
                face->size.internal_leading = internal_leading;
                face->scalable = FALSE;
            }

            /* check for the presence of the 'CFF ' table to check if the font is Type1 */
            tmp_size = 0;
            if (pFT_Load_Sfnt_Table && !pFT_Load_Sfnt_Table(ft_face, FT_MAKE_TAG('C','F','F',' '), 0, NULL, &tmp_size))
            {
                TRACE("Font %s/%p is OTF Type1\n", wine_dbgstr_a(file), font_data_ptr);
                face->ntmFlags = NTM_PS_OPENTYPE;
            }
            else
                face->ntmFlags = 0;

            TRACE("fsCsb = %08x %08x/%08x %08x %08x %08x\n",
                  face->fs.fsCsb[0], face->fs.fsCsb[1],
                  face->fs.fsUsb[0], face->fs.fsUsb[1],
                  face->fs.fsUsb[2], face->fs.fsUsb[3]);


            if(face->fs.fsCsb[0] == 0) { /* let's see if we can find any interesting cmaps */
                for(i = 0; i < ft_face->num_charmaps; i++) {
                    switch(ft_face->charmaps[i]->encoding) {
                    case FT_ENCODING_UNICODE:
                    case FT_ENCODING_APPLE_ROMAN:
			face->fs.fsCsb[0] |= 1;
                        break;
                    case FT_ENCODING_MS_SYMBOL:
                        face->fs.fsCsb[0] |= 1L << 31;
                        break;
                    default:
                        break;
                    }
                }
            }

            if(face->fs.fsCsb[0] & ~(1L << 31))
                have_installed_roman_font = TRUE;
        } while(!FT_IS_SCALABLE(ft_face) && ++bitmap_num < ft_face->num_fixed_sizes);

	num_faces = ft_face->num_faces;
	pFT_Done_Face(ft_face);
	TRACE("Added font %s %s\n", debugstr_w(family->FamilyName),
	      debugstr_w(StyleW));
    } while(num_faces > ++face_index);
    return num_faces;
}

static INT AddFontFileToList(const char *file, char *fake_family, const WCHAR *target_family, DWORD flags)
{
    return AddFontToList(file, NULL, 0, fake_family, target_family, flags);
}

static void DumpFontList(void)
{
    Family *family;
    Face *face;
    struct list *family_elem_ptr, *face_elem_ptr;

    LIST_FOR_EACH(family_elem_ptr, &font_list) {
        family = LIST_ENTRY(family_elem_ptr, Family, entry); 
        TRACE("Family: %s\n", debugstr_w(family->FamilyName));
        LIST_FOR_EACH(face_elem_ptr, &family->faces) {
            face = LIST_ENTRY(face_elem_ptr, Face, entry);
            TRACE("\t%s\t%08x", debugstr_w(face->StyleName), face->fs.fsCsb[0]);
            if(!face->scalable)
                TRACE(" %d", face->size.height);
            TRACE("\n");
	}
    }
    return;
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
    Family *family;
    Face *face;
    struct list *family_elem_ptr, *face_elem_ptr;
    CHAR familyA[400];

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
	while(RegEnumValueW(hkey, i++, value, &vlen, NULL, &type, data,
			    &dlen) == ERROR_SUCCESS) {
	    TRACE("Got %s=%s\n", debugstr_w(value), debugstr_w(data));
            /* "NewName"="Oldname" */
            WideCharToMultiByte(CP_ACP, 0, value, -1, familyA, sizeof(familyA), NULL, NULL);

            /* Find the old family and hence all of the font files
               in that family */
            LIST_FOR_EACH(family_elem_ptr, &font_list) {
                family = LIST_ENTRY(family_elem_ptr, Family, entry);
                if(!strcmpiW(family->FamilyName, data)) {
                    LIST_FOR_EACH(face_elem_ptr, &family->faces) {
                        face = LIST_ENTRY(face_elem_ptr, Face, entry);
                        TRACE("mapping %s %s to %s\n", debugstr_w(family->FamilyName),
                              debugstr_w(face->StyleName), familyA);
                        /* Now add a new entry with the new family name */
                        AddFontToList(face->file, face->font_data_ptr, face->font_data_size, familyA, family->FamilyName, ADDFONT_FORCE_BITMAP | (face->external ? ADDFONT_EXTERNAL_FONT : 0));
                    }
                    break;
                }
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

/*************************************************************
 * init_system_links
 */
static BOOL init_system_links(void)
{
    static const WCHAR system_link[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
                                        'W','i','n','d','o','w','s',' ','N','T','\\',
                                        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','F','o','n','t','L','i','n','k','\\',
                                        'S','y','s','t','e','m','L','i','n','k',0};
    HKEY hkey;
    BOOL ret = FALSE;
    DWORD type, max_val, max_data, val_len, data_len, index;
    WCHAR *value, *data;
    WCHAR *entry, *next;
    SYSTEM_LINKS *font_link, *system_font_link;
    CHILD_FONT *child_font;
    static const WCHAR Tahoma[] = {'T','a','h','o','m','a',0};
    static const WCHAR tahoma_ttf[] = {'t','a','h','o','m','a','.','t','t','f',0};
    static const WCHAR System[] = {'S','y','s','t','e','m',0};
    FONTSIGNATURE fs;
    Family *family;
    Face *face;
    FontSubst *psub;

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
            TRACE("%s:\n", debugstr_w(value));

            memset(&fs, 0, sizeof(fs));
            font_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*font_link));
            psub = get_font_subst(&font_subst_list, value, -1);
            font_link->font_name = (psub)? strdupW(psub->to.name) : strdupW(value);
            list_init(&font_link->links);
            for(entry = data; (char*)entry < (char*)data + data_len && *entry != 0; entry = next)
            {
                WCHAR *face_name;
                CHILD_FONT *child_font;

                TRACE("\t%s\n", debugstr_w(entry));

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
                fs.fsCsb[0] |= face->fs.fsCsb[0];
                fs.fsCsb[1] |= face->fs.fsCsb[1];
                TRACE("Adding file %s index %ld\n", child_font->face->file, child_font->face->face_index);
                list_add_tail(&font_link->links, &child_font->entry);
            }
            family = find_family_from_name(font_link->font_name);
            if(family)
            {
                LIST_FOR_EACH_ENTRY(face, &family->faces, Face, entry)
                {
                    memcpy(&face->fs_links, &fs, sizeof(fs));
                }
            }
            list_add_tail(&system_links, &font_link->entry);
            val_len = max_val + 1;
            data_len = max_data;
        }

        HeapFree(GetProcessHeap(), 0, value);
        HeapFree(GetProcessHeap(), 0, data);
        RegCloseKey(hkey);
    }

    /* Explicitly add an entry for the system font, this links to Tahoma and any links
       that Tahoma has */

    system_font_link = HeapAlloc(GetProcessHeap(), 0, sizeof(*system_font_link));
    system_font_link->font_name = strdupW(System);
    list_init(&system_font_link->links);    

    face = find_face_from_filename(tahoma_ttf, Tahoma);
    if(face)
    {
        child_font = HeapAlloc(GetProcessHeap(), 0, sizeof(*child_font));
        child_font->face = face;
        child_font->font = NULL;
        TRACE("Found Tahoma in %s index %ld\n", child_font->face->file, child_font->face->face_index);
        list_add_tail(&system_font_link->links, &child_font->entry);
    }
    LIST_FOR_EACH_ENTRY(font_link, &system_links, SYSTEM_LINKS, entry)
    {
        if(!strcmpiW(font_link->font_name, Tahoma))
        {
            CHILD_FONT *font_link_entry;
            LIST_FOR_EACH_ENTRY(font_link_entry, &font_link->links, CHILD_FONT, entry)
            {
                CHILD_FONT *new_child;
                new_child = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_child));
                new_child->face = font_link_entry->face;
                new_child->font = NULL;
                list_add_tail(&system_font_link->links, &new_child->entry);
            }
            break;
        }
    }
    list_add_tail(&system_links, &system_font_link->entry);
    return ret;
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
	    AddFontFileToList(path, NULL, NULL, external_fonts ? ADDFONT_EXTERNAL_FONT : 0);
    }
    closedir(dir);
    return TRUE;
}

static void load_fontconfig_fonts(void)
{
#ifdef SONAME_LIBFONTCONFIG
    void *fc_handle = NULL;
    FcConfig *config;
    FcPattern *pat;
    FcObjectSet *os;
    FcFontSet *fontset;
    int i, len;
    char *file;
    const char *ext;

    fc_handle = wine_dlopen(SONAME_LIBFONTCONFIG, RTLD_NOW, NULL, 0);
    if(!fc_handle) {
        TRACE("Wine cannot find the fontconfig library (%s).\n",
              SONAME_LIBFONTCONFIG);
	return;
    }
#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(fc_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
LOAD_FUNCPTR(FcConfigGetCurrent);
LOAD_FUNCPTR(FcFontList);
LOAD_FUNCPTR(FcFontSetDestroy);
LOAD_FUNCPTR(FcInit);
LOAD_FUNCPTR(FcObjectSetAdd);
LOAD_FUNCPTR(FcObjectSetCreate);
LOAD_FUNCPTR(FcObjectSetDestroy);
LOAD_FUNCPTR(FcPatternCreate);
LOAD_FUNCPTR(FcPatternDestroy);
LOAD_FUNCPTR(FcPatternGetBool);
LOAD_FUNCPTR(FcPatternGetString);
#undef LOAD_FUNCPTR

    if(!pFcInit()) return;
    
    config = pFcConfigGetCurrent();
    pat = pFcPatternCreate();
    os = pFcObjectSetCreate();
    pFcObjectSetAdd(os, FC_FILE);
    pFcObjectSetAdd(os, FC_SCALABLE);
    fontset = pFcFontList(config, pat, os);
    if(!fontset) return;
    for(i = 0; i < fontset->nfont; i++) {
        FcBool scalable;

        if(pFcPatternGetString(fontset->fonts[i], FC_FILE, 0, (FcChar8**)&file) != FcResultMatch)
            continue;
        TRACE("fontconfig: %s\n", file);

        /* We're just interested in OT/TT fonts for now, so this hack just
           picks up the scalable fonts without extensions .pf[ab] to save time
           loading every other font */

        if(pFcPatternGetBool(fontset->fonts[i], FC_SCALABLE, 0, &scalable) == FcResultMatch && !scalable)
        {
            TRACE("not scalable\n");
            continue;
        }

        len = strlen( file );
        if(len < 4) continue;
        ext = &file[ len - 3 ];
        if(strcasecmp(ext, "pfa") && strcasecmp(ext, "pfb"))
            AddFontFileToList(file, NULL, NULL,  ADDFONT_EXTERNAL_FONT);
    }
    pFcFontSetDestroy(fontset);
    pFcObjectSetDestroy(os);
    pFcPatternDestroy(pat);
 sym_not_found:
#endif
    return;
}

static BOOL load_font_from_data_dir(LPCWSTR file)
{
    BOOL ret = FALSE;
    const char *data_dir = wine_get_data_dir();

    if (!data_dir) data_dir = wine_get_build_dir();

    if (data_dir)
    {
        INT len;
        char *unix_name;

        len = WideCharToMultiByte(CP_UNIXCP, 0, file, -1, NULL, 0, NULL, NULL);

        unix_name = HeapAlloc(GetProcessHeap(), 0, strlen(data_dir) + len + sizeof("/fonts/"));

        strcpy(unix_name, data_dir);
        strcat(unix_name, "/fonts/");

        WideCharToMultiByte(CP_UNIXCP, 0, file, -1, unix_name + strlen(unix_name), len, NULL, NULL);

        ret = AddFontFileToList(unix_name, NULL, NULL, ADDFONT_FORCE_BITMAP);
        HeapFree(GetProcessHeap(), 0, unix_name);
    }
    return ret;
}

static void load_system_fonts(void)
{
    HKEY hkey;
    WCHAR data[MAX_PATH], windowsdir[MAX_PATH], pathW[MAX_PATH];
    const WCHAR * const *value;
    DWORD dlen, type;
    static const WCHAR fmtW[] = {'%','s','\\','%','s','\0'};
    char *unixname;

    if(RegOpenKeyW(HKEY_CURRENT_CONFIG, system_fonts_reg_key, &hkey) == ERROR_SUCCESS) {
        GetWindowsDirectoryW(windowsdir, sizeof(windowsdir) / sizeof(WCHAR));
        strcatW(windowsdir, fontsW);
        for(value = SystemFontValues; *value; value++) { 
            dlen = sizeof(data);
            if(RegQueryValueExW(hkey, *value, 0, &type, (void*)data, &dlen) == ERROR_SUCCESS &&
               type == REG_SZ) {
                BOOL added = FALSE;

                sprintfW(pathW, fmtW, windowsdir, data);
                if((unixname = wine_get_unix_file_name(pathW))) {
                    added = AddFontFileToList(unixname, NULL, NULL, ADDFONT_FORCE_BITMAP);
                    HeapFree(GetProcessHeap(), 0, unixname);
                }
                if (!added)
                    load_font_from_data_dir(data);
            }
        }
        RegCloseKey(hkey);
    }
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
    HKEY winkey = 0, externalkey = 0;
    LPWSTR valueW;
    LPVOID data;
    DWORD dlen, vlen, datalen, valuelen, i, type, len, len_fam;
    Family *family;
    Face *face;
    struct list *family_elem_ptr, *face_elem_ptr;
    WCHAR *file;
    static const WCHAR TrueType[] = {' ','(','T','r','u','e','T','y','p','e',')','\0'};
    static const WCHAR spaceW[] = {' ', '\0'};
    char *path;

    if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, is_win9x() ? win9x_font_reg_key : winnt_font_reg_key,
                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &winkey, NULL) != ERROR_SUCCESS) {
        ERR("Can't create Windows font reg key\n");
        goto end;
    }
    /* @@ Wine registry key: HKCU\Software\Wine\Fonts\ExternalFonts */
    if(RegCreateKeyW(HKEY_CURRENT_USER, external_fonts_reg_key, &externalkey) != ERROR_SUCCESS) {
        ERR("Can't create external font reg key\n");
        goto end;
    }

    /* Delete all external fonts added last time */

    RegQueryInfoKeyW(externalkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                     &valuelen, &datalen, NULL, NULL);
    valuelen++; /* returned value doesn't include room for '\0' */
    valueW = HeapAlloc(GetProcessHeap(), 0, valuelen * sizeof(WCHAR));
    data = HeapAlloc(GetProcessHeap(), 0, datalen * sizeof(WCHAR));

    dlen = datalen * sizeof(WCHAR);
    vlen = valuelen;
    i = 0;
    while(RegEnumValueW(externalkey, i++, valueW, &vlen, NULL, &type, data,
                        &dlen) == ERROR_SUCCESS) {

        RegDeleteValueW(winkey, valueW);
        /* reset dlen and vlen */
        dlen = datalen;
        vlen = valuelen;
    }
    HeapFree(GetProcessHeap(), 0, data);
    HeapFree(GetProcessHeap(), 0, valueW);

    /* Delete the old external fonts key */
    RegCloseKey(externalkey);
    externalkey = 0;
    RegDeleteKeyW(HKEY_CURRENT_USER, external_fonts_reg_key);

    /* @@ Wine registry key: HKCU\Software\Wine\Fonts\ExternalFonts */
    if(RegCreateKeyExW(HKEY_CURRENT_USER, external_fonts_reg_key,
                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &externalkey, NULL) != ERROR_SUCCESS) {
        ERR("Can't create external font reg key\n");
        goto end;
    }

    /* enumerate the fonts and add external ones to the two keys */

    LIST_FOR_EACH(family_elem_ptr, &font_list) {
        family = LIST_ENTRY(family_elem_ptr, Family, entry); 
        len_fam = strlenW(family->FamilyName) + sizeof(TrueType) / sizeof(WCHAR) + 1;
        LIST_FOR_EACH(face_elem_ptr, &family->faces) {
            face = LIST_ENTRY(face_elem_ptr, Face, entry);
            if(!face->external) continue;
            len = len_fam;
            if(strcmpiW(face->StyleName, RegularW))
                len = len_fam + strlenW(face->StyleName) + 1;
            valueW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            strcpyW(valueW, family->FamilyName);
            if(len != len_fam) {
                strcatW(valueW, spaceW);
                strcatW(valueW, face->StyleName);
            }
            strcatW(valueW, TrueType);
            if((path = strrchr(face->file, '/')) == NULL)
                path = face->file;
            else
                path++;
            len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);

            file = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, path, -1, file, len);
            RegSetValueExW(winkey, valueW, 0, REG_SZ, (BYTE*)file, len * sizeof(WCHAR));
            RegSetValueExW(externalkey, valueW, 0, REG_SZ, (BYTE*)file, len * sizeof(WCHAR));

            HeapFree(GetProcessHeap(), 0, file);
            HeapFree(GetProcessHeap(), 0, valueW);
        }
    }
 end:
    if(externalkey)
        RegCloseKey(externalkey);
    if(winkey)
        RegCloseKey(winkey);
    return;
}


/*************************************************************
 *    WineEngAddFontResourceEx
 *
 */
INT WineEngAddFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    if (ft_handle)  /* do it only if we have freetype up and running */
    {
        char *unixname;

        if(flags)
            FIXME("Ignoring flags %x\n", flags);

        if((unixname = wine_get_unix_file_name(file)))
        {
            INT ret = AddFontFileToList(unixname, NULL, NULL, ADDFONT_FORCE_BITMAP);
            HeapFree(GetProcessHeap(), 0, unixname);
            return ret;
        }
    }
    return 0;
}

/*************************************************************
 *    WineEngAddFontMemResourceEx
 *
 */
HANDLE WineEngAddFontMemResourceEx(PVOID pbFont, DWORD cbFont, PVOID pdv, DWORD *pcFonts)
{
    if (ft_handle)  /* do it only if we have freetype up and running */
    {
        PVOID pFontCopy = HeapAlloc(GetProcessHeap(), 0, cbFont);

        TRACE("Copying %d bytes of data from %p to %p\n", cbFont, pbFont, pFontCopy);
        memcpy(pFontCopy, pbFont, cbFont);

        *pcFonts = AddFontToList(NULL, pFontCopy, cbFont, NULL, NULL, ADDFONT_FORCE_BITMAP);

        if (*pcFonts == 0)
        {
            TRACE("AddFontToList failed\n");
            HeapFree(GetProcessHeap(), 0, pFontCopy);
            return NULL;
        }
        /* FIXME: is the handle only for use in RemoveFontMemResourceEx or should it be a true handle?
         * For now return something unique but quite random
         */
        TRACE("Returning handle %lx\n", ((INT_PTR)pFontCopy)^0x87654321);
        return (HANDLE)(((INT_PTR)pFontCopy)^0x87654321);
    }

    *pcFonts = 0;
    return 0;
}

/*************************************************************
 *    WineEngRemoveFontResourceEx
 *
 */
BOOL WineEngRemoveFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    FIXME(":stub\n");
    return TRUE;
}

static const struct nls_update_font_list
{
    UINT ansi_cp, oem_cp;
    const char *oem, *fixed, *system;
    const char *courier, *serif, *small, *sserif;
   /* these are for font substitute */
    const char *shelldlg, *tmsrmn;
} nls_update_font_list[] =
{
    /* Latin 1 (United States) */
    { 1252, 437, "vgaoem.fon", "vgafix.fon", "vgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon",
      "Tahoma","Times New Roman",
    },
    /* Latin 1 (Multilingual) */
    { 1252, 850, "vga850.fon", "vgafix.fon", "vgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon",
      "Tahoma","Times New Roman",  /* FIXME unverified */
    },
    /* Eastern Europe */
    { 1250, 852, "vga852.fon", "vgafixe.fon", "vgasyse.fon",
      "couree.fon", "serifee.fon", "smallee.fon", "sserifee.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Cyrillic */
    { 1251, 866, "vga866.fon", "vgafixr.fon", "vgasysr.fon",
      "courer.fon", "serifer.fon", "smaller.fon", "sserifer.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Greek */
    { 1253, 737, "vga869.fon", "vgafixg.fon", "vgasysg.fon",
      "coureg.fon", "serifeg.fon", "smalleg.fon", "sserifeg.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Turkish */
    { 1254, 857, "vga857.fon", "vgafixt.fon", "vgasyst.fon",
      "couret.fon", "serifet.fon", "smallet.fon", "sserifet.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Hebrew */
    { 1255, 862, "vgaoem.fon", "vgaf1255.fon", "vgas1255.fon",
      "coue1255.fon", "sere1255.fon", "smae1255.fon", "ssee1255.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Arabic */
    { 1256, 720, "vgaoem.fon", "vgaf1256.fon", "vgas1256.fon",
      "coue1256.fon", "sere1256.fon", "smae1256.fon", "ssee1256.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Baltic */
    { 1257, 775, "vga775.fon", "vgaf1257.fon", "vgas1257.fon",
      "coue1257.fon", "sere1257.fon", "smae1257.fon", "ssee1257.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Vietnamese */
    { 1258, 1258, "vga850.fon", "vgafix.fon", "vgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Thai */
    { 874, 874, "vga850.fon", "vgaf874.fon", "vgas874.fon",
      "coure.fon", "serife.fon", "smalle.fon", "ssee874.fon",
      "Tahoma","Times New Roman", /* FIXME unverified */
    },
    /* Japanese */
    { 932, 932, "vga932.fon", "jvgafix.fon", "jvgasys.fon",
      "coure.fon", "serife.fon", "jsmalle.fon", "sserife.fon",
      "MS UI Gothic","MS Serif",
    },
    /* Chinese Simplified */
    { 936, 936, "vga936.fon", "svgafix.fon", "svgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon",
      "Tahoma", "Times New Roman", /* FIXME unverified */
    },
    /* Korean */
    { 949, 949, "vga949.fon", "hvgafix.fon", "hvgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon",
      "Gulim",  "Batang",
    },
    /* Chinese Traditional */
    { 950, 950, "vga950.fon", "cvgafix.fon", "cvgasys.fon",
      "coure.fon", "serife.fon", "smalle.fon", "sserife.fon",
      "Tahoma",  "Times New Roman", /* FIXME unverified */
    }
};

static inline HKEY create_fonts_NT_registry_key(void)
{
    HKEY hkey = 0;

    RegCreateKeyExW(HKEY_LOCAL_MACHINE, winnt_font_reg_key, 0, NULL,
                    0, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    return hkey;
}

static inline HKEY create_fonts_9x_registry_key(void)
{
    HKEY hkey = 0;

    RegCreateKeyExW(HKEY_LOCAL_MACHINE, win9x_font_reg_key, 0, NULL,
                    0, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    return hkey;
}

static inline HKEY create_config_fonts_registry_key(void)
{
    HKEY hkey = 0;

    RegCreateKeyExW(HKEY_CURRENT_CONFIG, system_fonts_reg_key, 0, NULL,
                    0, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    return hkey;
}

static void add_font_list(HKEY hkey, const struct nls_update_font_list *fl)
{
    RegSetValueExA(hkey, "Courier", 0, REG_SZ, (const BYTE *)fl->courier, strlen(fl->courier)+1);
    RegSetValueExA(hkey, "MS Serif", 0, REG_SZ, (const BYTE *)fl->serif, strlen(fl->serif)+1);
    RegSetValueExA(hkey, "MS Sans Serif", 0, REG_SZ, (const BYTE *)fl->sserif, strlen(fl->sserif)+1);
    RegSetValueExA(hkey, "Small Fonts", 0, REG_SZ, (const BYTE *)fl->small, strlen(fl->small)+1);
}

static void update_font_info(void)
{
    char buf[40], cpbuf[40];
    DWORD len, type;
    HKEY hkey = 0;
    UINT i, ansi_cp = 0, oem_cp = 0;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Wine\\Fonts", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL) != ERROR_SUCCESS)
        return;

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER|LOCALE_NOUSEROVERRIDE,
                   (WCHAR *)&ansi_cp, sizeof(ansi_cp)/sizeof(WCHAR));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTCODEPAGE|LOCALE_RETURN_NUMBER|LOCALE_NOUSEROVERRIDE,
                   (WCHAR *)&oem_cp, sizeof(oem_cp)/sizeof(WCHAR));
    sprintf( cpbuf, "%u,%u", ansi_cp, oem_cp );

    len = sizeof(buf);
    if (RegQueryValueExA(hkey, "Codepages", 0, &type, (BYTE *)buf, &len) == ERROR_SUCCESS && type == REG_SZ)
    {
        if (!strcmp( buf, cpbuf ))  /* already set correctly */
        {
            RegCloseKey(hkey);
            return;
        }
        TRACE("updating registry, codepages changed %s -> %u,%u\n", buf, ansi_cp, oem_cp);
    }
    else TRACE("updating registry, codepages changed none -> %u,%u\n", ansi_cp, oem_cp);

    RegSetValueExA(hkey, "Codepages", 0, REG_SZ, (const BYTE *)cpbuf, strlen(cpbuf)+1);
    RegCloseKey(hkey);

    for (i = 0; i < sizeof(nls_update_font_list)/sizeof(nls_update_font_list[0]); i++)
    {
        if (nls_update_font_list[i].ansi_cp == ansi_cp &&
            nls_update_font_list[i].oem_cp == oem_cp)
        {
            HKEY hkey;

            hkey = create_config_fonts_registry_key();
            RegSetValueExA(hkey, "OEMFONT.FON", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].oem, strlen(nls_update_font_list[i].oem)+1);
            RegSetValueExA(hkey, "FIXEDFON.FON", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].fixed, strlen(nls_update_font_list[i].fixed)+1);
            RegSetValueExA(hkey, "FONTS.FON", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].system, strlen(nls_update_font_list[i].system)+1);
            RegCloseKey(hkey);

            hkey = create_fonts_NT_registry_key();
            add_font_list(hkey, &nls_update_font_list[i]);
            RegCloseKey(hkey);

            hkey = create_fonts_9x_registry_key();
            add_font_list(hkey, &nls_update_font_list[i]);
            RegCloseKey(hkey);

            if (!RegCreateKeyA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes", &hkey ))
            {
                RegSetValueExA(hkey, "MS Shell Dlg", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].shelldlg,
                               strlen(nls_update_font_list[i].shelldlg)+1);
                RegSetValueExA(hkey, "Tms Rmn", 0, REG_SZ, (const BYTE *)nls_update_font_list[i].tmsrmn,
                               strlen(nls_update_font_list[i].tmsrmn)+1);
                RegCloseKey(hkey);
            }
            return;
        }
    }
    FIXME("there is no font defaults for codepages %u,%u\n", ansi_cp, oem_cp);
}

/*************************************************************
 *    WineEngInit
 *
 * Initialize FreeType library and create a list of available faces
 */
BOOL WineEngInit(void)
{
    static const WCHAR dot_fonW[] = {'.','f','o','n','\0'};
    static const WCHAR pathW[] = {'P','a','t','h',0};
    HKEY hkey;
    DWORD valuelen, datalen, i = 0, type, dlen, vlen;
    LPVOID data;
    WCHAR windowsdir[MAX_PATH];
    char *unixname;
    HANDLE font_mutex;
    const char *data_dir;

    TRACE("\n");

    /* update locale dependent font info in registry */
    update_font_info();

    ft_handle = wine_dlopen(SONAME_LIBFREETYPE, RTLD_NOW, NULL, 0);
    if(!ft_handle) {
        WINE_MESSAGE(
      "Wine cannot find the FreeType font library.  To enable Wine to\n"
      "use TrueType fonts please install a version of FreeType greater than\n"
      "or equal to 2.0.5.\n"
      "http://www.freetype.org\n");
	return FALSE;
    }

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(ft_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}

    LOAD_FUNCPTR(FT_Vector_Unit)
    LOAD_FUNCPTR(FT_Done_Face)
    LOAD_FUNCPTR(FT_Get_Char_Index)
    LOAD_FUNCPTR(FT_Get_Module)
    LOAD_FUNCPTR(FT_Get_Sfnt_Name)
    LOAD_FUNCPTR(FT_Get_Sfnt_Name_Count)
    LOAD_FUNCPTR(FT_Get_Sfnt_Table)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_Matrix_Multiply)
    LOAD_FUNCPTR(FT_MulFix)
    LOAD_FUNCPTR(FT_New_Face)
    LOAD_FUNCPTR(FT_New_Memory_Face)
    LOAD_FUNCPTR(FT_Outline_Get_Bitmap)
    LOAD_FUNCPTR(FT_Outline_Transform)
    LOAD_FUNCPTR(FT_Outline_Translate)
    LOAD_FUNCPTR(FT_Select_Charmap)
    LOAD_FUNCPTR(FT_Set_Charmap)
    LOAD_FUNCPTR(FT_Set_Pixel_Sizes)
    LOAD_FUNCPTR(FT_Vector_Transform)

#undef LOAD_FUNCPTR
    /* Don't warn if this one is missing */
    pFT_Library_Version = wine_dlsym(ft_handle, "FT_Library_Version", NULL, 0);
    pFT_Load_Sfnt_Table = wine_dlsym(ft_handle, "FT_Load_Sfnt_Table", NULL, 0);
    pFT_Get_First_Char = wine_dlsym(ft_handle, "FT_Get_First_Char", NULL, 0);
    pFT_Get_Next_Char = wine_dlsym(ft_handle, "FT_Get_Next_Char", NULL, 0);
    pFT_Get_TrueType_Engine_Type = wine_dlsym(ft_handle, "FT_Get_TrueType_Engine_Type", NULL, 0);
#ifdef HAVE_FREETYPE_FTWINFNT_H
    pFT_Get_WinFNT_Header = wine_dlsym(ft_handle, "FT_Get_WinFNT_Header", NULL, 0);
#endif
      if(!wine_dlsym(ft_handle, "FT_Get_Postscript_Name", NULL, 0) &&
	 !wine_dlsym(ft_handle, "FT_Sqrt64", NULL, 0)) {
	/* try to avoid 2.0.4: >= 2.0.5 has FT_Get_Postscript_Name and
	   <= 2.0.3 has FT_Sqrt64 */
	  goto sym_not_found;
      }

    if(pFT_Init_FreeType(&library) != 0) {
        ERR("Can't init FreeType library\n");
	wine_dlclose(ft_handle, NULL, 0);
        ft_handle = NULL;
	return FALSE;
    }
    FT_Version.major=FT_Version.minor=FT_Version.patch=-1;
    if (pFT_Library_Version)
    {
        pFT_Library_Version(library,&FT_Version.major,&FT_Version.minor,&FT_Version.patch);
    }
    if (FT_Version.major<=0)
    {
        FT_Version.major=2;
        FT_Version.minor=0;
        FT_Version.patch=5;
    }
    TRACE("FreeType version is %d.%d.%d\n",FT_Version.major,FT_Version.minor,FT_Version.patch);
    FT_SimpleVersion = ((FT_Version.major << 16) & 0xff0000) |
                       ((FT_Version.minor <<  8) & 0x00ff00) |
                       ((FT_Version.patch      ) & 0x0000ff);

    if((font_mutex = CreateMutexW(NULL, FALSE, font_mutex_nameW)) == NULL) {
        ERR("Failed to create font mutex\n");
        return FALSE;
    }
    WaitForSingleObject(font_mutex, INFINITE);

    /* load the system bitmap fonts */
    load_system_fonts();

    /* load in the fonts from %WINDOWSDIR%\\Fonts first of all */
    GetWindowsDirectoryW(windowsdir, sizeof(windowsdir) / sizeof(WCHAR));
    strcatW(windowsdir, fontsW);
    if((unixname = wine_get_unix_file_name(windowsdir)))
    {
        ReadFontDir(unixname, FALSE);
        HeapFree(GetProcessHeap(), 0, unixname);
    }

    /* load the system truetype fonts */
    data_dir = wine_get_data_dir();
    if (data_dir && (unixname = HeapAlloc(GetProcessHeap(), 0, strlen(data_dir) + sizeof("/fonts/")))) {
        strcpy(unixname, data_dir);
        strcat(unixname, "/fonts/");
        ReadFontDir(unixname, TRUE);
        HeapFree(GetProcessHeap(), 0, unixname);
    }

    /* now look under HKLM\Software\Microsoft\Windows[ NT]\CurrentVersion\Fonts
       for any fonts not installed in %WINDOWSDIR%\Fonts.  They will have their
       full path as the entry.  Also look for any .fon fonts, since ReadFontDir
       will skip these. */
    if(RegOpenKeyW(HKEY_LOCAL_MACHINE,
                   is_win9x() ? win9x_font_reg_key : winnt_font_reg_key,
		   &hkey) == ERROR_SUCCESS) {
        LPWSTR valueW;
        RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			 &valuelen, &datalen, NULL, NULL);

	valuelen++; /* returned value doesn't include room for '\0' */
	valueW = HeapAlloc(GetProcessHeap(), 0, valuelen * sizeof(WCHAR));
	data = HeapAlloc(GetProcessHeap(), 0, datalen * sizeof(WCHAR));
        if (valueW && data)
        {
            dlen = datalen * sizeof(WCHAR);
            vlen = valuelen;
            while(RegEnumValueW(hkey, i++, valueW, &vlen, NULL, &type, data,
                                &dlen) == ERROR_SUCCESS) {
                if(((LPWSTR)data)[0] && ((LPWSTR)data)[1] == ':')
                {
                    if((unixname = wine_get_unix_file_name((LPWSTR)data)))
                    {
                        AddFontFileToList(unixname, NULL, NULL, ADDFONT_FORCE_BITMAP);
                        HeapFree(GetProcessHeap(), 0, unixname);
                    }
                }
                else if(dlen / 2 >= 6 && !strcmpiW(((LPWSTR)data) + dlen / 2 - 5, dot_fonW))
                {
                    WCHAR pathW[MAX_PATH];
                    static const WCHAR fmtW[] = {'%','s','\\','%','s','\0'};
                    BOOL added = FALSE;

                    sprintfW(pathW, fmtW, windowsdir, data);
                    if((unixname = wine_get_unix_file_name(pathW)))
                    {
                        added = AddFontFileToList(unixname, NULL, NULL, ADDFONT_FORCE_BITMAP);
                        HeapFree(GetProcessHeap(), 0, unixname);
                    }
                    if (!added)
                        load_font_from_data_dir(data);
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

    load_fontconfig_fonts();

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
                    LPSTR next = strchr( ptr, ':' );
                    if (next) *next++ = 0;
                    ReadFontDir( ptr, TRUE );
                    ptr = next;
                }
                HeapFree( GetProcessHeap(), 0, valueA );
            }
            HeapFree( GetProcessHeap(), 0, valueW );
        }
        RegCloseKey(hkey);
    }

    DumpFontList();
    LoadSubstList();
    DumpSubstList();
    LoadReplaceList();
    update_reg_entries();

    init_system_links();
    
    ReleaseMutex(font_mutex);
    return TRUE;
sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the FreeType\n"
      "font library.  To enable Wine to use TrueType fonts please upgrade\n"
      "FreeType to at least version 2.0.5.\n"
      "http://www.freetype.org\n");
    wine_dlclose(ft_handle, NULL, 0);
    ft_handle = NULL;
    return FALSE;
}


static LONG calc_ppem_for_height(FT_Face ft_face, LONG height)
{
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;

    LONG ppem;

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
        if(pOS2->usWinAscent + pOS2->usWinDescent == 0)
            ppem = ft_face->units_per_EM * height /
                (pHori->Ascender - pHori->Descender);
        else
            ppem = ft_face->units_per_EM * height /
                (pOS2->usWinAscent + pOS2->usWinDescent);
    }
    else
        ppem = -height;

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
    FT_Error err;
    FT_Face ft_face;
    void *data_ptr;
    DWORD data_size;

    TRACE("%s/%p, %ld, %d x %d\n", debugstr_a(face->file), face->font_data_ptr, face->face_index, width, height);

    if (face->file)
    {
        if (!(font->mapping = map_font_file( face->file )))
        {
            WARN("failed to map %s\n", debugstr_a(face->file));
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

    if(FT_IS_SCALABLE(ft_face)) {
        /* load the VDMX table if we have one */
        font->ppem = load_VDMX(font, height);
        if(font->ppem == 0)
            font->ppem = calc_ppem_for_height(ft_face, height);

        if((err = pFT_Set_Pixel_Sizes(ft_face, 0, font->ppem)) != 0)
            WARN("FT_Set_Pixel_Sizes %d, %d rets %x\n", 0, font->ppem, err);
    } else {
        font->ppem = height;
        if((err = pFT_Set_Pixel_Sizes(ft_face, width, height)) != 0)
            WARN("FT_Set_Pixel_Sizes %d, %d rets %x\n", width, height, err);
    }
    return ft_face;
}


static int get_nearest_charset(Face *face, int *cp)
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
        if(csi.fs.fsCsb[0] & (face->fs.fsCsb[0] | face->fs_links.fsCsb[0]))
	    return csi.ciCharset;

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
	  face->fs.fsCsb[0], face->file);
    *cp = acp;
    return DEFAULT_CHARSET;
}

static GdiFont *alloc_font(void)
{
    GdiFont *ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    ret->gmsize = 1;
    ret->gm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GM*));
    ret->gm[0] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GM) * GM_BLOCK_SIZE);
    ret->potm = NULL;
    ret->font_desc.matrix.eM11 = ret->font_desc.matrix.eM22 = 1.0;
    ret->total_kern_pairs = (DWORD)-1;
    ret->kern_pairs = NULL;
    list_init(&ret->hfontlist);
    list_init(&ret->child_fonts);
    return ret;
}

static void free_font(GdiFont *font)
{
    struct list *cursor, *cursor2;
    int i;

    LIST_FOR_EACH_SAFE(cursor, cursor2, &font->child_fonts)
    {
        CHILD_FONT *child = LIST_ENTRY(cursor, CHILD_FONT, entry);
        struct list *first_hfont;
        HFONTLIST *hfontlist;
        list_remove(cursor);
        if(child->font)
        {
            first_hfont = list_head(&child->font->hfontlist);
            hfontlist = LIST_ENTRY(first_hfont, HFONTLIST, entry);
            DeleteObject(hfontlist->hfont);
            HeapFree(GetProcessHeap(), 0, hfontlist);
            free_font(child->font);
        }
        HeapFree(GetProcessHeap(), 0, child);
    }

    if (font->ft_face) pFT_Done_Face(font->ft_face);
    if (font->mapping) unmap_font_file( font->mapping );
    HeapFree(GetProcessHeap(), 0, font->kern_pairs);
    HeapFree(GetProcessHeap(), 0, font->potm);
    HeapFree(GetProcessHeap(), 0, font->name);
    for (i = 0; i < font->gmsize; i++)
        HeapFree(GetProcessHeap(),0,font->gm[i]);
    HeapFree(GetProcessHeap(), 0, font->gm);
    HeapFree(GetProcessHeap(), 0, font);
}


/*************************************************************
 * load_VDMX
 *
 * load the vdmx entry for the specified height
 */

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (FT_ULong)_x4 << 24 ) |     \
            ( (FT_ULong)_x3 << 16 ) |     \
            ( (FT_ULong)_x2 <<  8 ) |     \
              (FT_ULong)_x1         )

#define MS_VDMX_TAG MS_MAKE_TAG('V', 'D', 'M', 'X')

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

static LONG load_VDMX(GdiFont *font, LONG height)
{
    WORD hdr[3], tmp;
    VDMX_group group;
    BYTE devXRatio, devYRatio;
    USHORT numRecs, numRatios;
    DWORD result, offset = -1;
    LONG ppem = 0;
    int i;

    /* For documentation on VDMX records, see
     * http://www.microsoft.com/OpenType/OTSpec/vdmx.htm
     */

    result = WineEngGetFontData(font, MS_VDMX_TAG, 0, hdr, 6);

    if(result == GDI_ERROR) /* no vdmx table present, use linear scaling */
	return ppem;

    /* FIXME: need the real device aspect ratio */
    devXRatio = 1;
    devYRatio = 1;

    numRecs = GET_BE_WORD(hdr[1]);
    numRatios = GET_BE_WORD(hdr[2]);

    TRACE("numRecs = %d numRatios = %d\n", numRecs, numRatios);
    for(i = 0; i < numRatios; i++) {
	Ratios ratio;

	offset = (3 * 2) + (i * sizeof(Ratios));
	WineEngGetFontData(font, MS_VDMX_TAG, offset, &ratio, sizeof(Ratios));
	offset = -1;

	TRACE("Ratios[%d] %d  %d : %d -> %d\n", i, ratio.bCharSet, ratio.xRatio, ratio.yStartRatio, ratio.yEndRatio);

	if((ratio.xRatio == 0 &&
	    ratio.yStartRatio == 0 &&
	    ratio.yEndRatio == 0) ||
	   (devXRatio == ratio.xRatio &&
	    devYRatio >= ratio.yStartRatio &&
	    devYRatio <= ratio.yEndRatio))
	    {
		offset = (3 * 2) + (numRatios * 4) + (i * 2);
		WineEngGetFontData(font, MS_VDMX_TAG, offset, &tmp, 2);
		offset = GET_BE_WORD(tmp);
		break;
	    }
    }

    if(offset == -1) {
	FIXME("No suitable ratio found\n");
	return ppem;
    }

    if(WineEngGetFontData(font, MS_VDMX_TAG, offset, &group, 4) != GDI_ERROR) {
	USHORT recs;
	BYTE startsz, endsz;
	WORD *vTable;

	recs = GET_BE_WORD(group.recs);
	startsz = group.startsz;
	endsz = group.endsz;

	TRACE("recs=%d  startsz=%d  endsz=%d\n", recs, startsz, endsz);

	vTable = HeapAlloc(GetProcessHeap(), 0, recs * 6);
	result = WineEngGetFontData(font, MS_VDMX_TAG, offset + 4, vTable, recs * 6);
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
		    font->yMax = GET_BE_WORD(vTable[(i * 3) + 1]);
		    font->yMin = GET_BE_WORD(vTable[(i * 3) + 2]);
                    ppem = GET_BE_WORD(vTable[i * 3]);
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
		goto end;

	    for(i = 0; i < recs; i++) {
		USHORT yPelHeight;
		yPelHeight = GET_BE_WORD(vTable[i * 3]);

		if(yPelHeight > ppem)
		    break; /* failed */

		if(yPelHeight == ppem) {
		    font->yMax = GET_BE_WORD(vTable[(i * 3) + 1]);
		    font->yMin = GET_BE_WORD(vTable[(i * 3) + 2]);
                    TRACE("ppem %d found; yMax=%d  yMin=%d\n", ppem, font->yMax, font->yMin);
		    break;
		}
	    }
	}
	end:
	HeapFree(GetProcessHeap(), 0, vTable);
    }

    return ppem;
}

static BOOL fontcmp(GdiFont *font, FONT_DESC *fd)
{
    if(font->font_desc.hash != fd->hash) return TRUE;
    if(memcmp(&font->font_desc.matrix, &fd->matrix, sizeof(fd->matrix))) return TRUE;
    if(memcmp(&font->font_desc.lf, &fd->lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
    if(!font->font_desc.can_use_bitmap != !fd->can_use_bitmap) return TRUE;
    return strcmpiW(font->font_desc.lf.lfFaceName, fd->lf.lfFaceName);
}

static void calc_hash(FONT_DESC *pfd)
{
    DWORD hash = 0, *ptr, two_chars;
    WORD *pwc;
    unsigned int i;

    for(i = 0, ptr = (DWORD*)&pfd->matrix; i < sizeof(FMAT2)/sizeof(DWORD); i++, ptr++)
        hash ^= *ptr;
    for(i = 0, ptr = (DWORD*)&pfd->lf; i < 7; i++, ptr++)
        hash ^= *ptr;
    for(i = 0, ptr = (DWORD*)&pfd->lf.lfFaceName; i < LF_FACESIZE/2; i++, ptr++) {
        two_chars = *ptr;
        pwc = (WCHAR *)&two_chars;
        if(!*pwc) break;
        *pwc = toupperW(*pwc);
        pwc++;
        *pwc = toupperW(*pwc);
        hash ^= two_chars;
        if(!*pwc) break;
    }
    hash ^= !pfd->can_use_bitmap;
    pfd->hash = hash;
    return;
}

static GdiFont *find_in_cache(HFONT hfont, LOGFONTW *plf, XFORM *pxf, BOOL can_use_bitmap)
{
    GdiFont *ret;
    FONT_DESC fd;
    HFONTLIST *hflist;
    struct list *font_elem_ptr, *hfontlist_elem_ptr;

    memcpy(&fd.lf, plf, sizeof(LOGFONTW));
    memcpy(&fd.matrix, pxf, sizeof(FMAT2));
    fd.can_use_bitmap = can_use_bitmap;
    calc_hash(&fd);

    /* try the in-use list */
    LIST_FOR_EACH(font_elem_ptr, &gdi_font_list) {
        ret = LIST_ENTRY(font_elem_ptr, struct tagGdiFont, entry);
        if(!fontcmp(ret, &fd)) {
            if(!can_use_bitmap && !FT_IS_SCALABLE(ret->ft_face)) continue;
            LIST_FOR_EACH(hfontlist_elem_ptr, &ret->hfontlist) {
                hflist = LIST_ENTRY(hfontlist_elem_ptr, struct tagHFONTLIST, entry);
                if(hflist->hfont == hfont)
                    return ret;
            }
            hflist = HeapAlloc(GetProcessHeap(), 0, sizeof(*hflist));
            hflist->hfont = hfont;
            list_add_head(&ret->hfontlist, &hflist->entry);
            return ret;
        }
    }
 
    /* then the unused list */
    font_elem_ptr = list_head(&unused_gdi_font_list);
    while(font_elem_ptr) {
        ret = LIST_ENTRY(font_elem_ptr, struct tagGdiFont, entry);
        font_elem_ptr = list_next(&unused_gdi_font_list, font_elem_ptr);
        if(!fontcmp(ret, &fd)) {
            if(!can_use_bitmap && !FT_IS_SCALABLE(ret->ft_face)) continue;
            assert(list_empty(&ret->hfontlist));
            TRACE("Found %p in unused list\n", ret);
            list_remove(&ret->entry);
            list_add_head(&gdi_font_list, &ret->entry);
            hflist = HeapAlloc(GetProcessHeap(), 0, sizeof(*hflist));
            hflist->hfont = hfont;
            list_add_head(&ret->hfontlist, &hflist->entry);
            return ret;
        }
    }
    return NULL;
}

    
/*************************************************************
 * create_child_font_list
 */
static BOOL create_child_font_list(GdiFont *font)
{
    BOOL ret = FALSE;
    SYSTEM_LINKS *font_link;
    CHILD_FONT *font_link_entry, *new_child;

    LIST_FOR_EACH_ENTRY(font_link, &system_links, SYSTEM_LINKS, entry)
    {
        if(!strcmpW(font_link->font_name, font->name))
        {
            TRACE("found entry in system list\n");
            LIST_FOR_EACH_ENTRY(font_link_entry, &font_link->links, CHILD_FONT, entry)
            {
                new_child = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_child));
                new_child->face = font_link_entry->face;
                new_child->font = NULL;
                list_add_tail(&font->child_fonts, &new_child->entry);
                TRACE("font %s %ld\n", debugstr_a(new_child->face->file), new_child->face->face_index);
            }
            ret = TRUE;
            break;
        }
    }

    return ret;
}

static BOOL select_charmap(FT_Face ft_face, FT_Encoding encoding)
{
    FT_Error ft_err = FT_Err_Invalid_CharMap_Handle;

    if (pFT_Set_Charmap)
    {
        FT_Int i;
        FT_CharMap cmap0, cmap1, cmap2, cmap3, cmap_def;

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

    return pFT_Select_Charmap(ft_face, encoding) == FT_Err_Ok;
}

/*************************************************************
 * WineEngCreateFontInstance
 *
 */
GdiFont *WineEngCreateFontInstance(DC *dc, HFONT hfont)
{
    GdiFont *ret;
    Face *face, *best, *best_bitmap;
    Family *family, *last_resort_family;
    struct list *family_elem_ptr, *face_elem_ptr;
    INT height, width = 0;
    unsigned int score = 0, new_score;
    signed int diff = 0, newdiff;
    BOOL bd, it, can_use_bitmap;
    LOGFONTW lf;
    CHARSETINFO csi;
    HFONTLIST *hflist;

    LIST_FOR_EACH_ENTRY(ret, &child_font_list, struct tagGdiFont, entry)
    {
        struct list *first_hfont = list_head(&ret->hfontlist);
        hflist = LIST_ENTRY(first_hfont, HFONTLIST, entry);
        if(hflist->hfont == hfont)
            return ret;
    }

    if (!GetObjectW( hfont, sizeof(lf), &lf )) return NULL;
    can_use_bitmap = GetDeviceCaps(dc->hSelf, TEXTCAPS) & TC_RA_ABLE;

    TRACE("%s, h=%d, it=%d, weight=%d, PandF=%02x, charset=%d orient %d escapement %d\n",
	  debugstr_w(lf.lfFaceName), lf.lfHeight, lf.lfItalic,
	  lf.lfWeight, lf.lfPitchAndFamily, lf.lfCharSet, lf.lfOrientation,
	  lf.lfEscapement);

    /* check the cache first */
    if((ret = find_in_cache(hfont, &lf, &dc->xformWorld2Vport, can_use_bitmap)) != NULL) {
        TRACE("returning cached gdiFont(%p) for hFont %p\n", ret, hfont);
        return ret;
    }

    TRACE("not in cache\n");
    if(list_empty(&font_list)) /* No fonts installed */
    {
	TRACE("No fonts installed\n");
	return NULL;
    }
    if(!have_installed_roman_font)
    {
	TRACE("No roman font installed\n");
	return NULL;
    }

    ret = alloc_font();

     memcpy(&ret->font_desc.matrix, &dc->xformWorld2Vport, sizeof(FMAT2));
     memcpy(&ret->font_desc.lf, &lf, sizeof(LOGFONTW));
     ret->font_desc.can_use_bitmap = can_use_bitmap;
     calc_hash(&ret->font_desc);
     hflist = HeapAlloc(GetProcessHeap(), 0, sizeof(*hflist));
     hflist->hfont = hfont;
     list_add_head(&ret->hfontlist, &hflist->entry);


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
        FontSubst *psub;
        SYSTEM_LINKS *font_link;
        CHILD_FONT *font_link_entry;

        psub = get_font_subst(&font_subst_list, lf.lfFaceName, lf.lfCharSet);

	if(psub) {
	    TRACE("substituting %s -> %s\n", debugstr_w(lf.lfFaceName),
		  debugstr_w(psub->to.name));
	    strcpyW(lf.lfFaceName, psub->to.name);
	}

	/* We want a match on name and charset or just name if
	   charset was DEFAULT_CHARSET.  If the latter then
	   we fixup the returned charset later in get_nearest_charset
	   where we'll either use the charset of the current ansi codepage
	   or if that's unavailable the first charset that the font supports.
	*/
        LIST_FOR_EACH(family_elem_ptr, &font_list) {
            family = LIST_ENTRY(family_elem_ptr, Family, entry);
	    if(!strcmpiW(family->FamilyName, lf.lfFaceName)) {
                LIST_FOR_EACH(face_elem_ptr, &family->faces) { 
                    face = LIST_ENTRY(face_elem_ptr, Face, entry);
                    if((csi.fs.fsCsb[0] & (face->fs.fsCsb[0] | face->fs_links.fsCsb[0])) || !csi.fs.fsCsb[0])
                        if(face->scalable || can_use_bitmap)
                            goto found;
                }
            }
	}

        /*
	 * Try check the SystemLink list first for a replacement font.
	 * We may find good replacements there.
         */
        LIST_FOR_EACH_ENTRY(font_link, &system_links, SYSTEM_LINKS, entry)
        {
            if(!strcmpiW(font_link->font_name, lf.lfFaceName))
            {
                TRACE("found entry in system list\n");
                LIST_FOR_EACH_ENTRY(font_link_entry, &font_link->links, CHILD_FONT, entry)
                {
                    face = font_link_entry->face;
                    family = face->family;
                    if(csi.fs.fsCsb[0] &
                        (face->fs.fsCsb[0] | face->fs_links.fsCsb[0]) || !csi.fs.fsCsb[0])
                    {
                        if(face->scalable || can_use_bitmap)
                            goto found;
                    }
                }
            }
        }
    }

    /* If requested charset was DEFAULT_CHARSET then try using charset
       corresponding to the current ansi codepage */
    if(!csi.fs.fsCsb[0]) {
        INT acp = GetACP();
        if(!TranslateCharsetInfo((DWORD*)(INT_PTR)acp, &csi, TCI_SRCCODEPAGE)) {
            FIXME("TCI failed on codepage %d\n", acp);
            csi.fs.fsCsb[0] = 0;
        } else
            lf.lfCharSet = csi.ciCharset;
    }

    /* Face families are in the top 4 bits of lfPitchAndFamily,
       so mask with 0xF0 before testing */

    if((lf.lfPitchAndFamily & FIXED_PITCH) ||
       (lf.lfPitchAndFamily & 0xF0) == FF_MODERN)
        strcpyW(lf.lfFaceName, defFixed);
    else if((lf.lfPitchAndFamily & 0xF0) == FF_ROMAN)
        strcpyW(lf.lfFaceName, defSerif);
    else if((lf.lfPitchAndFamily & 0xF0) == FF_SWISS)
        strcpyW(lf.lfFaceName, defSans);
    else
        strcpyW(lf.lfFaceName, defSans);
    LIST_FOR_EACH(family_elem_ptr, &font_list) {
        family = LIST_ENTRY(family_elem_ptr, Family, entry);
        if(!strcmpiW(family->FamilyName, lf.lfFaceName)) {
            LIST_FOR_EACH(face_elem_ptr, &family->faces) { 
                face = LIST_ENTRY(face_elem_ptr, Face, entry);
                if(csi.fs.fsCsb[0] & (face->fs.fsCsb[0] | face->fs_links.fsCsb[0]))
                    if(face->scalable || can_use_bitmap)
                        goto found;
            }
        }
    }

    last_resort_family = NULL;
    LIST_FOR_EACH(family_elem_ptr, &font_list) {
        family = LIST_ENTRY(family_elem_ptr, Family, entry);
        LIST_FOR_EACH(face_elem_ptr, &family->faces) { 
            face = LIST_ENTRY(face_elem_ptr, Face, entry);
            if(csi.fs.fsCsb[0] & (face->fs.fsCsb[0] | face->fs_links.fsCsb[0])) {
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

    LIST_FOR_EACH(family_elem_ptr, &font_list) {
        family = LIST_ENTRY(family_elem_ptr, Family, entry);
        LIST_FOR_EACH(face_elem_ptr, &family->faces) { 
            face = LIST_ENTRY(face_elem_ptr, Face, entry);
            if(face->scalable) {
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
        free_font(ret);
        return NULL;
    }

    WARN("could only find a bitmap font - this will probably look awful!\n");
    family = last_resort_family;
    csi.fs.fsCsb[0] = 0;

found:
    it = lf.lfItalic ? 1 : 0;
    bd = lf.lfWeight > 550 ? 1 : 0;

    height = GDI_ROUND( (FLOAT)lf.lfHeight * dc->xformWorld2Vport.eM22 );
    height = lf.lfHeight < 0 ? -abs(height) : abs(height);

    face = best = best_bitmap = NULL;
    LIST_FOR_EACH_ENTRY(face, &family->faces, Face, entry)
    {
        if((csi.fs.fsCsb[0] & (face->fs.fsCsb[0] | face->fs_links.fsCsb[0])) || !csi.fs.fsCsb[0])
        {
            new_score = (face->Italic ^ it) + (face->Bold ^ bd);
            if(!best || new_score <= score)
            {
                TRACE("(it=%d, bd=%d) is selected for (it=%d, bd=%d)\n",
                      face->Italic, face->Bold, it, bd);
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
    ret->fake_italic = (it && !face->Italic);
    ret->fake_bold = (bd && !face->Bold);

    memcpy(&ret->fs, &face->fs, sizeof(FONTSIGNATURE));

    if(csi.fs.fsCsb[0]) {
        ret->charset = lf.lfCharSet;
        ret->codepage = csi.ciACP;
    }
    else
        ret->charset = get_nearest_charset(face, &ret->codepage);

    TRACE("Chosen: %s %s (%s/%p:%ld)\n", debugstr_w(family->FamilyName),
	  debugstr_w(face->StyleName), face->file, face->font_data_ptr, face->face_index);

    if(!face->scalable) {
        width = face->size.x_ppem >> 6;
        height = face->size.y_ppem >> 6;
    }
    ret->ft_face = OpenFontFace(ret, face, width, height);

    if (!ret->ft_face)
    {
        free_font( ret );
        return 0;
    }

    ret->ntmFlags = face->ntmFlags;

    if (ret->charset == SYMBOL_CHARSET && 
        select_charmap(ret->ft_face, FT_ENCODING_MS_SYMBOL)) {
        /* No ops */
    }
    else if (select_charmap(ret->ft_face, FT_ENCODING_UNICODE)) {
        /* No ops */
    }
    else {
        select_charmap(ret->ft_face, FT_ENCODING_APPLE_ROMAN);
    }

    ret->orientation = FT_IS_SCALABLE(ret->ft_face) ? lf.lfOrientation : 0;
    ret->name = strdupW(family->FamilyName);
    ret->underline = lf.lfUnderline ? 0xff : 0;
    ret->strikeout = lf.lfStrikeOut ? 0xff : 0;
    create_child_font_list(ret);

    TRACE("caching: gdiFont=%p  hfont=%p\n", ret, hfont);

    ret->aveWidth = FT_IS_SCALABLE(ret->ft_face) ? abs(lf.lfWidth) : 0;
    list_add_head(&gdi_font_list, &ret->entry);
    return ret;
}

static void dump_gdi_font_list(void)
{
    GdiFont *gdiFont;
    struct list *elem_ptr;

    TRACE("---------- gdiFont Cache ----------\n");
    LIST_FOR_EACH(elem_ptr, &gdi_font_list) {
        gdiFont = LIST_ENTRY(elem_ptr, struct tagGdiFont, entry);
        TRACE("gdiFont=%p %s %d\n",
              gdiFont, debugstr_w(gdiFont->font_desc.lf.lfFaceName), gdiFont->font_desc.lf.lfHeight);
    }

    TRACE("---------- Unused gdiFont Cache ----------\n");
    LIST_FOR_EACH(elem_ptr, &unused_gdi_font_list) {
        gdiFont = LIST_ENTRY(elem_ptr, struct tagGdiFont, entry);
        TRACE("gdiFont=%p %s %d\n",
              gdiFont, debugstr_w(gdiFont->font_desc.lf.lfFaceName), gdiFont->font_desc.lf.lfHeight);
    }
}

/*************************************************************
 * WineEngDestroyFontInstance
 *
 * free the gdiFont associated with this handle
 *
 */
BOOL WineEngDestroyFontInstance(HFONT handle)
{
    GdiFont *gdiFont;
    HFONTLIST *hflist;
    BOOL ret = FALSE;
    struct list *font_elem_ptr, *hfontlist_elem_ptr;
    int i = 0;

    LIST_FOR_EACH_ENTRY(gdiFont, &child_font_list, struct tagGdiFont, entry)
    {
        struct list *first_hfont = list_head(&gdiFont->hfontlist);
        hflist = LIST_ENTRY(first_hfont, HFONTLIST, entry);
        if(hflist->hfont == handle)
        {
            TRACE("removing child font %p from child list\n", gdiFont);
            list_remove(&gdiFont->entry);
            return TRUE;
        }
    }

    TRACE("destroying hfont=%p\n", handle);
    if(TRACE_ON(font))
	dump_gdi_font_list();

    font_elem_ptr = list_head(&gdi_font_list);
    while(font_elem_ptr) {
        gdiFont = LIST_ENTRY(font_elem_ptr, struct tagGdiFont, entry);
        font_elem_ptr = list_next(&gdi_font_list, font_elem_ptr);

        hfontlist_elem_ptr = list_head(&gdiFont->hfontlist);
        while(hfontlist_elem_ptr) {
            hflist = LIST_ENTRY(hfontlist_elem_ptr, struct tagHFONTLIST, entry);
            hfontlist_elem_ptr = list_next(&gdiFont->hfontlist, hfontlist_elem_ptr);
            if(hflist->hfont == handle) {
                list_remove(&hflist->entry);
                HeapFree(GetProcessHeap(), 0, hflist);
                ret = TRUE;
            }
        }
        if(list_empty(&gdiFont->hfontlist)) {
            TRACE("Moving to Unused list\n");
            list_remove(&gdiFont->entry);
            list_add_head(&unused_gdi_font_list, &gdiFont->entry);
        }
    }


    font_elem_ptr = list_head(&unused_gdi_font_list);
    while(font_elem_ptr && i++ < UNUSED_CACHE_SIZE)
        font_elem_ptr = list_next(&unused_gdi_font_list, font_elem_ptr);
    while(font_elem_ptr) {
        gdiFont = LIST_ENTRY(font_elem_ptr, struct tagGdiFont, entry);
        font_elem_ptr = list_next(&unused_gdi_font_list, font_elem_ptr);
        TRACE("freeing %p\n", gdiFont);
        list_remove(&gdiFont->entry);
        free_font(gdiFont);
    }
    return ret;
}

static void GetEnumStructs(Face *face, LPENUMLOGFONTEXW pelf,
			   NEWTEXTMETRICEXW *pntm, LPDWORD ptype)
{
    OUTLINETEXTMETRICW *potm = NULL;
    UINT size;
    TEXTMETRICW tm, *ptm;
    GdiFont *font = alloc_font();
    LONG width, height;

    if(face->scalable) {
        height = 100;
        width = 0;
    } else {
        height = face->size.y_ppem >> 6;
        width = face->size.x_ppem >> 6;
    }
    
    if (!(font->ft_face = OpenFontFace(font, face, width, height)))
    {
        free_font(font);
        return;
    }

    font->name = strdupW(face->family->FamilyName);
    font->ntmFlags = face->ntmFlags;

    memset(&pelf->elfLogFont, 0, sizeof(LOGFONTW));

    size = WineEngGetOutlineTextMetrics(font, 0, NULL);
    if(size) {
        potm = HeapAlloc(GetProcessHeap(), 0, size);
        WineEngGetOutlineTextMetrics(font, size, potm);
        ptm = (TEXTMETRICW*)&potm->otmTextMetrics;
    } else {
        WineEngGetTextMetrics(font, &tm);
        ptm = &tm;
    }
        
    pntm->ntmTm.tmHeight = pelf->elfLogFont.lfHeight = ptm->tmHeight;
    pntm->ntmTm.tmAscent = ptm->tmAscent;
    pntm->ntmTm.tmDescent = ptm->tmDescent;
    pntm->ntmTm.tmInternalLeading = ptm->tmInternalLeading;
    pntm->ntmTm.tmExternalLeading = ptm->tmExternalLeading;
    pntm->ntmTm.tmAveCharWidth = pelf->elfLogFont.lfWidth = ptm->tmAveCharWidth;
    pntm->ntmTm.tmMaxCharWidth = ptm->tmMaxCharWidth;
    pntm->ntmTm.tmWeight = pelf->elfLogFont.lfWeight = ptm->tmWeight;
    pntm->ntmTm.tmOverhang = ptm->tmOverhang;
    pntm->ntmTm.tmDigitizedAspectX = ptm->tmDigitizedAspectX;
    pntm->ntmTm.tmDigitizedAspectY = ptm->tmDigitizedAspectY;
    pntm->ntmTm.tmFirstChar = ptm->tmFirstChar;
    pntm->ntmTm.tmLastChar = ptm->tmLastChar;
    pntm->ntmTm.tmDefaultChar = ptm->tmDefaultChar;
    pntm->ntmTm.tmBreakChar = ptm->tmBreakChar;
    pntm->ntmTm.tmItalic = pelf->elfLogFont.lfItalic = ptm->tmItalic;
    pntm->ntmTm.tmUnderlined = pelf->elfLogFont.lfUnderline = ptm->tmUnderlined;
    pntm->ntmTm.tmStruckOut = pelf->elfLogFont.lfStrikeOut = ptm->tmStruckOut;
    pntm->ntmTm.tmPitchAndFamily = ptm->tmPitchAndFamily;
    pelf->elfLogFont.lfPitchAndFamily = (ptm->tmPitchAndFamily & 0xf1) + 1;
    pntm->ntmTm.tmCharSet = pelf->elfLogFont.lfCharSet = ptm->tmCharSet;
    pelf->elfLogFont.lfOutPrecision = OUT_STROKE_PRECIS;
    pelf->elfLogFont.lfClipPrecision = CLIP_STROKE_PRECIS;
    pelf->elfLogFont.lfQuality = DRAFT_QUALITY;

    *ptype = 0;
    if (ptm->tmPitchAndFamily & TMPF_TRUETYPE)
        *ptype |= TRUETYPE_FONTTYPE;
    if (ptm->tmPitchAndFamily & TMPF_DEVICE)
        *ptype |= DEVICE_FONTTYPE;
    if(!(ptm->tmPitchAndFamily & TMPF_VECTOR))
        *ptype |= RASTER_FONTTYPE;

    pntm->ntmTm.ntmFlags = ptm->tmItalic ? NTM_ITALIC : 0;
    if(ptm->tmWeight > 550) pntm->ntmTm.ntmFlags |= NTM_BOLD;
    if(pntm->ntmTm.ntmFlags == 0) pntm->ntmTm.ntmFlags = NTM_REGULAR;
    pntm->ntmTm.ntmFlags |= face->ntmFlags;

    pntm->ntmTm.ntmCellHeight = pntm->ntmTm.tmHeight;
    pntm->ntmTm.ntmAvgWidth = pntm->ntmTm.tmAveCharWidth;
    memset(&pntm->ntmFontSig, 0, sizeof(FONTSIGNATURE));

    if(potm) {
        pntm->ntmTm.ntmSizeEM = potm->otmEMSquare;

        lstrcpynW(pelf->elfLogFont.lfFaceName,
                 (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpFamilyName),
                 LF_FACESIZE);
        lstrcpynW(pelf->elfFullName,
                 (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpFaceName),
                 LF_FULLFACESIZE);
        lstrcpynW(pelf->elfStyle,
                 (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpStyleName),
                 LF_FACESIZE);

        HeapFree(GetProcessHeap(), 0, potm);
    } else {
        pntm->ntmTm.ntmSizeEM = pntm->ntmTm.tmHeight - pntm->ntmTm.tmInternalLeading;

        lstrcpynW(pelf->elfLogFont.lfFaceName, face->family->FamilyName, LF_FACESIZE);
        lstrcpynW(pelf->elfFullName, face->family->FamilyName, LF_FACESIZE);
        pelf->elfStyle[0] = '\0';
    }

    pelf->elfScript[0] = '\0'; /* This will get set in WineEngEnumFonts */

    free_font(font);
}

/*************************************************************
 * WineEngEnumFonts
 *
 */
DWORD WineEngEnumFonts(LPLOGFONTW plf, FONTENUMPROCW proc, LPARAM lparam)
{
    Family *family;
    Face *face;
    struct list *family_elem_ptr, *face_elem_ptr;
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
    DWORD type, ret = 1;
    FONTSIGNATURE fs;
    CHARSETINFO csi;
    LOGFONTW lf;
    int i;

    TRACE("facename = %s charset %d\n", debugstr_w(plf->lfFaceName), plf->lfCharSet);

    if(plf->lfFaceName[0]) {
        FontSubst *psub;
        psub = get_font_subst(&font_subst_list, plf->lfFaceName, plf->lfCharSet);

        if(psub) {
            TRACE("substituting %s -> %s\n", debugstr_w(plf->lfFaceName),
                  debugstr_w(psub->to.name));
            memcpy(&lf, plf, sizeof(lf));
            strcpyW(lf.lfFaceName, psub->to.name);
            plf = &lf;
        }

        LIST_FOR_EACH(family_elem_ptr, &font_list) {
            family = LIST_ENTRY(family_elem_ptr, Family, entry);
            if(!strcmpiW(plf->lfFaceName, family->FamilyName)) {
                LIST_FOR_EACH(face_elem_ptr, &family->faces) {
                    face = LIST_ENTRY(face_elem_ptr, Face, entry);
                    GetEnumStructs(face, &elf, &ntm, &type);
                    for(i = 0; i < 32; i++) {
                        if(!face->scalable && face->fs.fsCsb[0] == 0) { /* OEM bitmap */
                            elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet = OEM_CHARSET;
                            strcpyW(elf.elfScript, OEM_DOSW);
                            i = 32; /* break out of loop */
                        } else if(!(face->fs.fsCsb[0] & (1L << i)))
                            continue;
                        else {
                            fs.fsCsb[0] = 1L << i;
                            fs.fsCsb[1] = 0;
                            if(!TranslateCharsetInfo(fs.fsCsb, &csi,
                                                     TCI_SRCFONTSIG))
                                csi.ciCharset = DEFAULT_CHARSET;
                            if(i == 31) csi.ciCharset = SYMBOL_CHARSET;
                            if(csi.ciCharset != DEFAULT_CHARSET) {
                                elf.elfLogFont.lfCharSet =
                                    ntm.ntmTm.tmCharSet = csi.ciCharset;
                                if(ElfScriptsW[i])
                                    strcpyW(elf.elfScript, ElfScriptsW[i]);
                                else
                                    FIXME("Unknown elfscript for bit %d\n", i);
                            }
                        }
                        TRACE("enuming face %s full %s style %s charset %d type %d script %s it %d weight %d ntmflags %08x\n",
                              debugstr_w(elf.elfLogFont.lfFaceName),
                              debugstr_w(elf.elfFullName), debugstr_w(elf.elfStyle),
                              csi.ciCharset, type, debugstr_w(elf.elfScript),
                              elf.elfLogFont.lfItalic, elf.elfLogFont.lfWeight,
                              ntm.ntmTm.ntmFlags);
                        ret = proc(&elf.elfLogFont, (TEXTMETRICW *)&ntm, type, lparam);
                        if(!ret) goto end;
		    }
		}
	    }
	}
    } else {
        LIST_FOR_EACH(family_elem_ptr, &font_list) {
            family = LIST_ENTRY(family_elem_ptr, Family, entry);
            face_elem_ptr = list_head(&family->faces);
            face = LIST_ENTRY(face_elem_ptr, Face, entry);
            GetEnumStructs(face, &elf, &ntm, &type);
            for(i = 0; i < 32; i++) {
                if(!face->scalable && face->fs.fsCsb[0] == 0) { /* OEM bitmap */
                    elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet = OEM_CHARSET;
                    strcpyW(elf.elfScript, OEM_DOSW);
                    i = 32; /* break out of loop */
	        } else if(!(face->fs.fsCsb[0] & (1L << i)))
                    continue;
                else {
		    fs.fsCsb[0] = 1L << i;
		    fs.fsCsb[1] = 0;
		    if(!TranslateCharsetInfo(fs.fsCsb, &csi,
					     TCI_SRCFONTSIG))
		        csi.ciCharset = DEFAULT_CHARSET;
		    if(i == 31) csi.ciCharset = SYMBOL_CHARSET;
		    if(csi.ciCharset != DEFAULT_CHARSET) {
		        elf.elfLogFont.lfCharSet = ntm.ntmTm.tmCharSet =
			  csi.ciCharset;
			  if(ElfScriptsW[i])
			      strcpyW(elf.elfScript, ElfScriptsW[i]);
			  else
			      FIXME("Unknown elfscript for bit %d\n", i);
                    }
                }
                TRACE("enuming face %s full %s style %s charset = %d type %d script %s it %d weight %d ntmflags %08x\n",
                      debugstr_w(elf.elfLogFont.lfFaceName),
                      debugstr_w(elf.elfFullName), debugstr_w(elf.elfStyle),
                      csi.ciCharset, type, debugstr_w(elf.elfScript),
                      elf.elfLogFont.lfItalic, elf.elfLogFont.lfWeight,
                      ntm.ntmTm.ntmFlags);
                ret = proc(&elf.elfLogFont, (TEXTMETRICW *)&ntm, type, lparam);
                if(!ret) goto end;
	    }
	}
    }
end:
    return ret;
}

static void FTVectorToPOINTFX(FT_Vector *vec, POINTFX *pt)
{
    pt->x.value = vec->x >> 6;
    pt->x.fract = (vec->x & 0x3f) << 10;
    pt->x.fract |= ((pt->x.fract >> 6) | (pt->x.fract >> 12));
    pt->y.value = vec->y >> 6;
    pt->y.fract = (vec->y & 0x3f) << 10;
    pt->y.fract |= ((pt->y.fract >> 6) | (pt->y.fract >> 12));
    return;
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

static FT_UInt get_glyph_index(GdiFont *font, UINT glyph)
{
    if(font->ft_face->charmap->encoding == FT_ENCODING_NONE) {
        WCHAR wc = (WCHAR)glyph;
        BOOL default_used;
        BOOL *default_used_pointer;
        FT_UInt ret;
        char buf;
        default_used_pointer = NULL;
        default_used = FALSE;
        if (codepage_sets_default_used(font->codepage))
            default_used_pointer = &default_used;
        if(!WideCharToMultiByte(font->codepage, 0, &wc, 1, &buf, sizeof(buf), NULL, default_used_pointer) || default_used)
            ret = 0;
        else
            ret = pFT_Get_Char_Index(font->ft_face, (unsigned char)buf);
        TRACE("%04x (%02x) -> ret %d def_used %d\n", glyph, buf, ret, default_used);
        return ret;
    }

    if(font->charset == SYMBOL_CHARSET && glyph < 0x100)
        glyph = glyph + 0xf000;
    return pFT_Get_Char_Index(font->ft_face, glyph);
}

/*************************************************************
 * WineEngGetGlyphIndices
 *
 * FIXME: add support for GGI_MARK_NONEXISTING_GLYPHS
 */
DWORD WineEngGetGlyphIndices(GdiFont *font, LPCWSTR lpstr, INT count,
				LPWORD pgi, DWORD flags)
{
    int i;
    WCHAR default_char = 0;
    TEXTMETRICW textm;

    if  (flags & GGI_MARK_NONEXISTING_GLYPHS) default_char = 0x001f;  /* Indicate non existence */

    for(i = 0; i < count; i++)
    {
        pgi[i] = get_glyph_index(font, lpstr[i]);
        if  (pgi[i] == 0)
        {
            if (!default_char)
            {
                WineEngGetTextMetrics(font, &textm);
                default_char = textm.tmDefaultChar;
            }
            pgi[i] = default_char;
        }
    }
    return count;
}

/*************************************************************
 * WineEngGetGlyphOutline
 *
 * Behaves in exactly the same way as the win32 api GetGlyphOutline
 * except that the first parameter is the HWINEENGFONT of the font in
 * question rather than an HDC.
 *
 */
DWORD WineEngGetGlyphOutline(GdiFont *incoming_font, UINT glyph, UINT format,
			     LPGLYPHMETRICS lpgm, DWORD buflen, LPVOID buf,
			     const MAT2* lpmat)
{
    static const FT_Matrix identityMat = {(1 << 16), 0, 0, (1 << 16)};
    FT_Face ft_face = incoming_font->ft_face;
    GdiFont *font = incoming_font;
    FT_UInt glyph_index;
    DWORD width, height, pitch, needed = 0;
    FT_Bitmap ft_bitmap;
    FT_Error err;
    INT left, right, top = 0, bottom = 0, adv, lsb, bbx;
    FT_Angle angle = 0;
    FT_Int load_flags = FT_LOAD_DEFAULT | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;
    float widthRatio = 1.0;
    FT_Matrix transMat = identityMat;
    BOOL needsTransform = FALSE;


    TRACE("%p, %04x, %08x, %p, %08x, %p, %p\n", font, glyph, format, lpgm,
	  buflen, buf, lpmat);

    if(format & GGO_GLYPH_INDEX) {
        glyph_index = glyph;
	format &= ~GGO_GLYPH_INDEX;
    } else {
        get_glyph_index_linked(incoming_font, glyph, &font, &glyph_index);
        ft_face = font->ft_face;
    }

    if(glyph_index >= font->gmsize * GM_BLOCK_SIZE) {
	font->gmsize = (glyph_index / GM_BLOCK_SIZE + 1);
	font->gm = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, font->gm,
			       font->gmsize * sizeof(GM*));
    } else {
        if(format == GGO_METRICS && font->gm[glyph_index / GM_BLOCK_SIZE] != NULL && FONT_GM(font,glyph_index)->init ) {
            *lpgm = FONT_GM(font,glyph_index)->gm;
	    return 1; /* FIXME */
	}
    }

    if (!font->gm[glyph_index / GM_BLOCK_SIZE])
        font->gm[glyph_index / GM_BLOCK_SIZE] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(GM) * GM_BLOCK_SIZE);

    if(font->orientation || (format != GGO_METRICS && format != GGO_BITMAP) || font->aveWidth || lpmat)
        load_flags |= FT_LOAD_NO_BITMAP;

    err = pFT_Load_Glyph(ft_face, glyph_index, load_flags);

    if(err) {
        WARN("FT_Load_Glyph on index %x returns %d\n", glyph_index, err);
	return GDI_ERROR;
    }
	
    /* Scaling factor */
    if (font->aveWidth && font->potm) {
        widthRatio = (float)font->aveWidth * font->font_desc.matrix.eM11 / (float) font->potm->otmTextMetrics.tmAveCharWidth;
    }

    left = (INT)(ft_face->glyph->metrics.horiBearingX * widthRatio) & -64;
    right = (INT)((ft_face->glyph->metrics.horiBearingX + ft_face->glyph->metrics.width) * widthRatio + 63) & -64;

    adv = (INT)((ft_face->glyph->metrics.horiAdvance * widthRatio) + 63) >> 6;
    lsb = left >> 6;
    bbx = (right - left) >> 6;

    /* Scaling transform */
    if(font->aveWidth) {
        FT_Matrix scaleMat;
        scaleMat.xx = FT_FixedFromFloat(widthRatio);
        scaleMat.xy = 0;
        scaleMat.yx = 0;
        scaleMat.yy = (1 << 16);

        pFT_Matrix_Multiply(&scaleMat, &transMat);
        needsTransform = TRUE;
    }

    /* Slant transform */
    if (font->fake_italic) {
        FT_Matrix slantMat;
        
        slantMat.xx = (1 << 16);
        slantMat.xy = ((1 << 16) >> 2);
        slantMat.yx = 0;
        slantMat.yy = (1 << 16);
        pFT_Matrix_Multiply(&slantMat, &transMat);
        needsTransform = TRUE;
    }

    /* Rotation transform */
    if(font->orientation) {
        FT_Matrix rotationMat;
        FT_Vector vecAngle;
        angle = FT_FixedFromFloat((float)font->orientation / 10.0);
        pFT_Vector_Unit(&vecAngle, angle);
        rotationMat.xx = vecAngle.x;
        rotationMat.xy = -vecAngle.y;
        rotationMat.yx = -rotationMat.xy;
        rotationMat.yy = rotationMat.xx;
        
        pFT_Matrix_Multiply(&rotationMat, &transMat);
        needsTransform = TRUE;
    }

    /* Extra transformation specified by caller */
    if (lpmat) {
        FT_Matrix extraMat;
        extraMat.xx = FT_FixedFromFIXED(lpmat->eM11);
        extraMat.xy = FT_FixedFromFIXED(lpmat->eM21);
        extraMat.yx = FT_FixedFromFIXED(lpmat->eM12);
        extraMat.yy = FT_FixedFromFIXED(lpmat->eM22);
        pFT_Matrix_Multiply(&extraMat, &transMat);
        needsTransform = TRUE;
    }

    if(!needsTransform) {
	top = (ft_face->glyph->metrics.horiBearingY + 63) & -64;
	bottom = (ft_face->glyph->metrics.horiBearingY -
		  ft_face->glyph->metrics.height) & -64;
	lpgm->gmCellIncX = adv;
	lpgm->gmCellIncY = 0;
    } else {
        INT xc, yc;
	FT_Vector vec;
	for(xc = 0; xc < 2; xc++) {
	    for(yc = 0; yc < 2; yc++) {
	        vec.x = (ft_face->glyph->metrics.horiBearingX +
		  xc * ft_face->glyph->metrics.width);
		vec.y = ft_face->glyph->metrics.horiBearingY -
		  yc * ft_face->glyph->metrics.height;
		TRACE("Vec %ld,%ld\n", vec.x, vec.y);
		pFT_Vector_Transform(&vec, &transMat);
		if(xc == 0 && yc == 0) {
		    left = right = vec.x;
		    top = bottom = vec.y;
		} else {
		    if(vec.x < left) left = vec.x;
		    else if(vec.x > right) right = vec.x;
		    if(vec.y < bottom) bottom = vec.y;
		    else if(vec.y > top) top = vec.y;
		}
	    }
	}
	left = left & -64;
	right = (right + 63) & -64;
	bottom = bottom & -64;
	top = (top + 63) & -64;

	TRACE("transformed box: (%d,%d - %d,%d)\n", left, top, right, bottom);
	vec.x = ft_face->glyph->metrics.horiAdvance;
	vec.y = 0;
	pFT_Vector_Transform(&vec, &transMat);
	lpgm->gmCellIncX = (vec.x+63) >> 6;
	lpgm->gmCellIncY = -((vec.y+63) >> 6);
    }
    lpgm->gmBlackBoxX = (right - left) >> 6;
    lpgm->gmBlackBoxY = (top - bottom) >> 6;
    lpgm->gmptGlyphOrigin.x = left >> 6;
    lpgm->gmptGlyphOrigin.y = top >> 6;

    if(format == GGO_METRICS || format == GGO_BITMAP)
    {
        FONT_GM(font,glyph_index)->gm = *lpgm;
        FONT_GM(font,glyph_index)->adv = adv;
        FONT_GM(font,glyph_index)->lsb = lsb;
        FONT_GM(font,glyph_index)->bbx = bbx;
        FONT_GM(font,glyph_index)->init = TRUE;
    }

    if(format == GGO_METRICS)
        return 1; /* FIXME */

    if(ft_face->glyph->format != ft_glyph_format_outline && format != GGO_BITMAP) {
        TRACE("loaded a bitmap\n");
	return GDI_ERROR;
    }

    switch(format) {
    case GGO_BITMAP:
        width = lpgm->gmBlackBoxX;
	height = lpgm->gmBlackBoxY;
	pitch = ((width + 31) >> 5) << 2;
        needed = pitch * height;

	if(!buf || !buflen) break;

	switch(ft_face->glyph->format) {
	case ft_glyph_format_bitmap:
	  {
	    BYTE *src = ft_face->glyph->bitmap.buffer, *dst = buf;
	    INT w = (ft_face->glyph->bitmap.width + 7) >> 3;
	    INT h = ft_face->glyph->bitmap.rows;
	    while(h--) {
	        memcpy(dst, src, w);
		src += ft_face->glyph->bitmap.pitch;
		dst += pitch;
	    }
	    break;
	  }

	case ft_glyph_format_outline:
	    ft_bitmap.width = width;
	    ft_bitmap.rows = height;
	    ft_bitmap.pitch = pitch;
	    ft_bitmap.pixel_mode = ft_pixel_mode_mono;
	    ft_bitmap.buffer = buf;

		if(needsTransform) {
			pFT_Outline_Transform(&ft_face->glyph->outline, &transMat);
	    }

	    pFT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );

	    /* Note: FreeType will only set 'black' bits for us. */
	    memset(buf, 0, needed);
	    pFT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);
	    break;

	default:
	    FIXME("loaded glyph format %x\n", ft_face->glyph->format);
	    return GDI_ERROR;
	}
	break;

    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    case WINE_GGO_GRAY16_BITMAP:
      {
	unsigned int mult, row, col;
	BYTE *start, *ptr;

        width = lpgm->gmBlackBoxX;
	height = lpgm->gmBlackBoxY;
	pitch = (width + 3) / 4 * 4;
	needed = pitch * height;

	if(!buf || !buflen) break;
	ft_bitmap.width = width;
	ft_bitmap.rows = height;
	ft_bitmap.pitch = pitch;
	ft_bitmap.pixel_mode = ft_pixel_mode_grays;
	ft_bitmap.buffer = buf;

	if(needsTransform) {
		pFT_Outline_Transform(&ft_face->glyph->outline, &transMat);
	}

	pFT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );

	memset(ft_bitmap.buffer, 0, buflen);

	pFT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);

	if(format == GGO_GRAY2_BITMAP)
	    mult = 4;
	else if(format == GGO_GRAY4_BITMAP)
	    mult = 16;
	else if(format == GGO_GRAY8_BITMAP)
	    mult = 64;
	else if(format == WINE_GGO_GRAY16_BITMAP)
	    break;
	else {
	    assert(0);
	    break;
	}

	start = buf;
	for(row = 0; row < height; row++) {
	    ptr = start;
	    for(col = 0; col < width; col++, ptr++) {
	        *ptr = (((int)*ptr) * mult + 128) / 256;
	    }
	    start += pitch;
	}
	break;
      }

    case GGO_NATIVE:
      {
	int contour, point = 0, first_pt;
	FT_Outline *outline = &ft_face->glyph->outline;
	TTPOLYGONHEADER *pph;
	TTPOLYCURVE *ppc;
	DWORD pph_start, cpfx, type;

	if(buflen == 0) buf = NULL;

	if (needsTransform && buf) {
		pFT_Outline_Transform(outline, &transMat);
	}

        for(contour = 0; contour < outline->n_contours; contour++) {
	    pph_start = needed;
	    pph = (TTPOLYGONHEADER *)((char *)buf + needed);
	    first_pt = point;
	    if(buf) {
	        pph->dwType = TT_POLYGON_TYPE;
		FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
	    }
	    needed += sizeof(*pph);
	    point++;
	    while(point <= outline->contours[contour]) {
	        ppc = (TTPOLYCURVE *)((char *)buf + needed);
		type = (outline->tags[point] & FT_Curve_Tag_On) ?
		  TT_PRIM_LINE : TT_PRIM_QSPLINE;
		cpfx = 0;
		do {
		    if(buf)
		        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
		    cpfx++;
		    point++;
		} while(point <= outline->contours[contour] &&
			(outline->tags[point] & FT_Curve_Tag_On) ==
			(outline->tags[point-1] & FT_Curve_Tag_On));
		/* At the end of a contour Windows adds the start point, but
		   only for Beziers */
		if(point > outline->contours[contour] &&
		   !(outline->tags[point-1] & FT_Curve_Tag_On)) {
		    if(buf)
		        FTVectorToPOINTFX(&outline->points[first_pt], &ppc->apfx[cpfx]);
		    cpfx++;
		} else if(point <= outline->contours[contour] &&
			  outline->tags[point] & FT_Curve_Tag_On) {
		  /* add closing pt for bezier */
		    if(buf)
		        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
		    cpfx++;
		    point++;
		}
		if(buf) {
		    ppc->wType = type;
		    ppc->cpfx = cpfx;
		}
		needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
	    }
	    if(buf)
	        pph->cb = needed - pph_start;
	}
	break;
      }
    case GGO_BEZIER:
      {
	/* Convert the quadratic Beziers to cubic Beziers.
	   The parametric eqn for a cubic Bezier is, from PLRM:
	   r(t) = at^3 + bt^2 + ct + r0
	   with the control points:
	   r1 = r0 + c/3
	   r2 = r1 + (c + b)/3
	   r3 = r0 + c + b + a

	   A quadratic Beizer has the form:
	   p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

	   So equating powers of t leads to:
	   r1 = 2/3 p1 + 1/3 p0
	   r2 = 2/3 p1 + 1/3 p2
	   and of course r0 = p0, r3 = p2
	*/

	int contour, point = 0, first_pt;
	FT_Outline *outline = &ft_face->glyph->outline;
	TTPOLYGONHEADER *pph;
	TTPOLYCURVE *ppc;
	DWORD pph_start, cpfx, type;
	FT_Vector cubic_control[4];
	if(buflen == 0) buf = NULL;

	if (needsTransform && buf) {
		pFT_Outline_Transform(outline, &transMat);
	}

        for(contour = 0; contour < outline->n_contours; contour++) {
	    pph_start = needed;
	    pph = (TTPOLYGONHEADER *)((char *)buf + needed);
	    first_pt = point;
	    if(buf) {
	        pph->dwType = TT_POLYGON_TYPE;
		FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
	    }
	    needed += sizeof(*pph);
	    point++;
	    while(point <= outline->contours[contour]) {
	        ppc = (TTPOLYCURVE *)((char *)buf + needed);
		type = (outline->tags[point] & FT_Curve_Tag_On) ?
		  TT_PRIM_LINE : TT_PRIM_CSPLINE;
		cpfx = 0;
		do {
		    if(type == TT_PRIM_LINE) {
		        if(buf)
			    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
			cpfx++;
			point++;
		    } else {
		      /* Unlike QSPLINEs, CSPLINEs always have their endpoint
			 so cpfx = 3n */

		      /* FIXME: Possible optimization in endpoint calculation
			 if there are two consecutive curves */
		        cubic_control[0] = outline->points[point-1];
		        if(!(outline->tags[point-1] & FT_Curve_Tag_On)) {
			    cubic_control[0].x += outline->points[point].x + 1;
			    cubic_control[0].y += outline->points[point].y + 1;
			    cubic_control[0].x >>= 1;
			    cubic_control[0].y >>= 1;
			}
			if(point+1 > outline->contours[contour])
 			    cubic_control[3] = outline->points[first_pt];
			else {
			    cubic_control[3] = outline->points[point+1];
			    if(!(outline->tags[point+1] & FT_Curve_Tag_On)) {
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
		        if(buf) {
			    FTVectorToPOINTFX(&cubic_control[1], &ppc->apfx[cpfx]);
			    FTVectorToPOINTFX(&cubic_control[2], &ppc->apfx[cpfx+1]);
			    FTVectorToPOINTFX(&cubic_control[3], &ppc->apfx[cpfx+2]);
			}
			cpfx += 3;
			point++;
		    }
		} while(point <= outline->contours[contour] &&
			(outline->tags[point] & FT_Curve_Tag_On) ==
			(outline->tags[point-1] & FT_Curve_Tag_On));
		/* At the end of a contour Windows adds the start point,
		   but only for Beziers and we've already done that.
		*/
		if(point <= outline->contours[contour] &&
		   outline->tags[point] & FT_Curve_Tag_On) {
		  /* This is the closing pt of a bezier, but we've already
		     added it, so just inc point and carry on */
		    point++;
		}
		if(buf) {
		    ppc->wType = type;
		    ppc->cpfx = cpfx;
		}
		needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
	    }
	    if(buf)
	        pph->cb = needed - pph_start;
	}
	break;
      }

    default:
        FIXME("Unsupported format %d\n", format);
	return GDI_ERROR;
    }
    return needed;
}

static BOOL get_bitmap_text_metrics(GdiFont *font)
{
    FT_Face ft_face = font->ft_face;
#ifdef HAVE_FREETYPE_FTWINFNT_H
    FT_WinFNT_HeaderRec winfnt_header;
#endif
    const DWORD size = offsetof(OUTLINETEXTMETRICW, otmFiller); 
    font->potm = HeapAlloc(GetProcessHeap(), 0, size);
    font->potm->otmSize = size;

#define TM font->potm->otmTextMetrics
#ifdef HAVE_FREETYPE_FTWINFNT_H
    if(pFT_Get_WinFNT_Header && !pFT_Get_WinFNT_Header(ft_face, &winfnt_header))
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
        TM.tmUnderlined = font->underline;
        TM.tmStruckOut = font->strikeout;
        TM.tmPitchAndFamily = winfnt_header.pitch_and_family;
        TM.tmCharSet = winfnt_header.charset;
    }
    else
#endif
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
        TM.tmUnderlined = font->underline;
        TM.tmStruckOut = font->strikeout;
        /* NB inverted meaning of TMPF_FIXED_PITCH */
        TM.tmPitchAndFamily = ft_face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ? 0 : TMPF_FIXED_PITCH;
        TM.tmCharSet = font->charset;
    }
#undef TM

    return TRUE;
}

/*************************************************************
 * WineEngGetTextMetrics
 *
 */
BOOL WineEngGetTextMetrics(GdiFont *font, LPTEXTMETRICW ptm)
{
    if(!font->potm) {
        if(!WineEngGetOutlineTextMetrics(font, 0, NULL))
            if(!get_bitmap_text_metrics(font))
                return FALSE;
    }
    if(!font->potm) return FALSE;
    memcpy(ptm, &font->potm->otmTextMetrics, sizeof(*ptm));

    if (font->aveWidth) {
        ptm->tmAveCharWidth = font->aveWidth * font->font_desc.matrix.eM11;
    }
    return TRUE;
}


/*************************************************************
 * WineEngGetOutlineTextMetrics
 *
 */
UINT WineEngGetOutlineTextMetrics(GdiFont *font, UINT cbSize,
				  OUTLINETEXTMETRICW *potm)
{
    FT_Face ft_face = font->ft_face;
    UINT needed, lenfam, lensty, ret;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    TT_Postscript *pPost;
    FT_Fixed x_scale, y_scale;
    WCHAR *family_nameW, *style_nameW;
    static const WCHAR spaceW[] = {' ', '\0'};
    char *cp;
    INT ascent, descent;

    TRACE("font=%p\n", font);

    if(!FT_IS_SCALABLE(ft_face))
        return 0;

    if(font->potm) {
        if(cbSize >= font->potm->otmSize)
	    memcpy(potm, font->potm, font->potm->otmSize);
	return font->potm->otmSize;
    }


    needed = sizeof(*potm);

    lenfam = (strlenW(font->name) + 1) * sizeof(WCHAR);
    family_nameW = strdupW(font->name);

    lensty = MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, NULL, 0)
      * sizeof(WCHAR);
    style_nameW = HeapAlloc(GetProcessHeap(), 0, lensty);
    MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1,
			style_nameW, lensty/sizeof(WCHAR));

    /* These names should be read from the TT name table */

    /* length of otmpFamilyName */
    needed += lenfam;

    /* length of otmpFaceName */
    if(!strcasecmp(ft_face->style_name, "regular")) {
      needed += lenfam; /* just the family name */
    } else {
      needed += lenfam + lensty; /* family + " " + style */
    }

    /* length of otmpStyleName */
    needed += lensty;

    /* length of otmpFullName */
    needed += lenfam + lensty;


    x_scale = ft_face->size->metrics.x_scale;
    y_scale = ft_face->size->metrics.y_scale;

    pOS2 = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
    if(!pOS2) {
        FIXME("Can't find OS/2 table - not TT font?\n");
	ret = 0;
	goto end;
    }

    pHori = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea);
    if(!pHori) {
        FIXME("Can't find HHEA table - not TT font?\n");
	ret = 0;
	goto end;
    }

    pPost = pFT_Get_Sfnt_Table(ft_face, ft_sfnt_post); /* we can live with this failing */

    TRACE("OS/2 winA = %d winD = %d typoA = %d typoD = %d typoLG = %d FT_Face a = %d, d = %d, h = %d: HORZ a = %d, d = %d lg = %d maxY = %ld minY = %ld\n",
	  pOS2->usWinAscent, pOS2->usWinDescent,
	  pOS2->sTypoAscender, pOS2->sTypoDescender, pOS2->sTypoLineGap,
	  ft_face->ascender, ft_face->descender, ft_face->height,
	  pHori->Ascender, pHori->Descender, pHori->Line_Gap,
	  ft_face->bbox.yMax, ft_face->bbox.yMin);

    font->potm = HeapAlloc(GetProcessHeap(), 0, needed);
    font->potm->otmSize = needed;

#define TM font->potm->otmTextMetrics

    if(pOS2->usWinAscent + pOS2->usWinDescent == 0) {
        ascent = pHori->Ascender;
        descent = -pHori->Descender;
    } else {
        ascent = pOS2->usWinAscent;
        descent = pOS2->usWinDescent;
    }

    if(font->yMax) {
	TM.tmAscent = font->yMax;
	TM.tmDescent = -font->yMin;
	TM.tmInternalLeading = (TM.tmAscent + TM.tmDescent) - ft_face->size->metrics.y_ppem;
    } else {
	TM.tmAscent = (pFT_MulFix(ascent, y_scale) + 32) >> 6;
	TM.tmDescent = (pFT_MulFix(descent, y_scale) + 32) >> 6;
	TM.tmInternalLeading = (pFT_MulFix(ascent + descent
					    - ft_face->units_per_EM, y_scale) + 32) >> 6;
    }

    TM.tmHeight = TM.tmAscent + TM.tmDescent;

    /* MSDN says:
     el = MAX(0, LineGap - ((WinAscent + WinDescent) - (Ascender - Descender)))
    */
    TM.tmExternalLeading = max(0, (pFT_MulFix(pHori->Line_Gap -
       		 ((ascent + descent) -
		  (pHori->Ascender - pHori->Descender)), y_scale) + 32) >> 6);

    TM.tmAveCharWidth = (pFT_MulFix(pOS2->xAvgCharWidth, x_scale) + 32) >> 6;
    if (TM.tmAveCharWidth == 0) {
        TM.tmAveCharWidth = 1; 
    }
    TM.tmMaxCharWidth = (pFT_MulFix(ft_face->bbox.xMax - ft_face->bbox.xMin, x_scale) + 32) >> 6;
    TM.tmWeight = font->fake_bold ? FW_BOLD : pOS2->usWeightClass;
    TM.tmOverhang = 0;
    TM.tmDigitizedAspectX = 300;
    TM.tmDigitizedAspectY = 300;
    /* It appears that for fonts with SYMBOL_CHARSET Windows always sets
     * symbol range to 0 - f0ff
     */
    if (font->charset == SYMBOL_CHARSET)
        TM.tmFirstChar = 0;
    else
        TM.tmFirstChar = pOS2->usFirstCharIndex;
    TM.tmLastChar = pOS2->usLastCharIndex;
    TM.tmDefaultChar = pOS2->usDefaultChar ? pOS2->usDefaultChar : 0x1f;
    TM.tmBreakChar = pOS2->usBreakChar ? pOS2->usBreakChar : ' ';
    TM.tmItalic = font->fake_italic ? 255 : ((ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 255 : 0);
    TM.tmUnderlined = font->underline;
    TM.tmStruckOut = font->strikeout;

    /* Yes TPMF_FIXED_PITCH is correct; braindead api */
    if(!FT_IS_FIXED_WIDTH(ft_face) &&
       (pOS2->version == 0xFFFFU || 
        pOS2->panose[PAN_PROPORTION_INDEX] != PAN_PROP_MONOSPACED))
        TM.tmPitchAndFamily = TMPF_FIXED_PITCH;
    else
        TM.tmPitchAndFamily = 0;

    switch(pOS2->panose[PAN_FAMILYTYPE_INDEX]) {
    case PAN_FAMILY_SCRIPT:
        TM.tmPitchAndFamily |= FF_SCRIPT;
	break;
    case PAN_FAMILY_DECORATIVE:
    case PAN_FAMILY_PICTORIAL:
        TM.tmPitchAndFamily |= FF_DECORATIVE;
	break;
    case PAN_FAMILY_TEXT_DISPLAY:
        if(TM.tmPitchAndFamily == 0) /* fixed */
	    TM.tmPitchAndFamily = FF_MODERN;
	else {
	    switch(pOS2->panose[PAN_SERIFSTYLE_INDEX]) {
	    case PAN_SERIF_NORMAL_SANS:
	    case PAN_SERIF_OBTUSE_SANS:
	    case PAN_SERIF_PERP_SANS:
	        TM.tmPitchAndFamily |= FF_SWISS;
		break;
	    default:
	        TM.tmPitchAndFamily |= FF_ROMAN;
	    }
	}
	break;
    default:
        TM.tmPitchAndFamily |= FF_DONTCARE;
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
#undef TM

    font->potm->otmFiller = 0;
    memcpy(&font->potm->otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
    font->potm->otmfsSelection = pOS2->fsSelection;
    font->potm->otmfsType = pOS2->fsType;
    font->potm->otmsCharSlopeRise = pHori->caret_Slope_Rise;
    font->potm->otmsCharSlopeRun = pHori->caret_Slope_Run;
    font->potm->otmItalicAngle = 0; /* POST table */
    font->potm->otmEMSquare = ft_face->units_per_EM;
    font->potm->otmAscent = (pFT_MulFix(pOS2->sTypoAscender, y_scale) + 32) >> 6;
    font->potm->otmDescent = (pFT_MulFix(pOS2->sTypoDescender, y_scale) + 32) >> 6;
    font->potm->otmLineGap = (pFT_MulFix(pOS2->sTypoLineGap, y_scale) + 32) >> 6;
    font->potm->otmsCapEmHeight = (pFT_MulFix(pOS2->sCapHeight, y_scale) + 32) >> 6;
    font->potm->otmsXHeight = (pFT_MulFix(pOS2->sxHeight, y_scale) + 32) >> 6;
    font->potm->otmrcFontBox.left = (pFT_MulFix(ft_face->bbox.xMin, x_scale) + 32) >> 6;
    font->potm->otmrcFontBox.right = (pFT_MulFix(ft_face->bbox.xMax, x_scale) + 32) >> 6;
    font->potm->otmrcFontBox.top = (pFT_MulFix(ft_face->bbox.yMax, y_scale) + 32) >> 6;
    font->potm->otmrcFontBox.bottom = (pFT_MulFix(ft_face->bbox.yMin, y_scale) + 32) >> 6;
    font->potm->otmMacAscent = 0; /* where do these come from ? */
    font->potm->otmMacDescent = 0;
    font->potm->otmMacLineGap = 0;
    font->potm->otmusMinimumPPEM = 0; /* TT Header */
    font->potm->otmptSubscriptSize.x = (pFT_MulFix(pOS2->ySubscriptXSize, x_scale) + 32) >> 6;
    font->potm->otmptSubscriptSize.y = (pFT_MulFix(pOS2->ySubscriptYSize, y_scale) + 32) >> 6;
    font->potm->otmptSubscriptOffset.x = (pFT_MulFix(pOS2->ySubscriptXOffset, x_scale) + 32) >> 6;
    font->potm->otmptSubscriptOffset.y = (pFT_MulFix(pOS2->ySubscriptYOffset, y_scale) + 32) >> 6;
    font->potm->otmptSuperscriptSize.x = (pFT_MulFix(pOS2->ySuperscriptXSize, x_scale) + 32) >> 6;
    font->potm->otmptSuperscriptSize.y = (pFT_MulFix(pOS2->ySuperscriptYSize, y_scale) + 32) >> 6;
    font->potm->otmptSuperscriptOffset.x = (pFT_MulFix(pOS2->ySuperscriptXOffset, x_scale) + 32) >> 6;
    font->potm->otmptSuperscriptOffset.y = (pFT_MulFix(pOS2->ySuperscriptYOffset, y_scale) + 32) >> 6;
    font->potm->otmsStrikeoutSize = (pFT_MulFix(pOS2->yStrikeoutSize, y_scale) + 32) >> 6;
    font->potm->otmsStrikeoutPosition = (pFT_MulFix(pOS2->yStrikeoutPosition, y_scale) + 32) >> 6;
    if(!pPost) {
        font->potm->otmsUnderscoreSize = 0;
	font->potm->otmsUnderscorePosition = 0;
    } else {
        font->potm->otmsUnderscoreSize = (pFT_MulFix(pPost->underlineThickness, y_scale) + 32) >> 6;
	font->potm->otmsUnderscorePosition = (pFT_MulFix(pPost->underlinePosition, y_scale) + 32) >> 6;
    }

    /* otmp* members should clearly have type ptrdiff_t, but M$ knows best */
    cp = (char*)font->potm + sizeof(*font->potm);
    font->potm->otmpFamilyName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, family_nameW);
    cp += lenfam;
    font->potm->otmpStyleName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, style_nameW);
    cp += lensty;
    font->potm->otmpFaceName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, family_nameW);
    if(strcasecmp(ft_face->style_name, "regular")) {
        strcatW((WCHAR*)cp, spaceW);
	strcatW((WCHAR*)cp, style_nameW);
	cp += lenfam + lensty;
    } else
        cp += lenfam;
    font->potm->otmpFullName = (LPSTR)(cp - (char*)font->potm);
    strcpyW((WCHAR*)cp, family_nameW);
    strcatW((WCHAR*)cp, spaceW);
    strcatW((WCHAR*)cp, style_nameW);
    ret = needed;

    if(potm && needed <= cbSize)
        memcpy(potm, font->potm, font->potm->otmSize);

end:
    HeapFree(GetProcessHeap(), 0, style_nameW);
    HeapFree(GetProcessHeap(), 0, family_nameW);

    return ret;
}

static BOOL load_child_font(GdiFont *font, CHILD_FONT *child)
{
    HFONTLIST *hfontlist;
    child->font = alloc_font();
    child->font->ft_face = OpenFontFace(child->font, child->face, 0, -font->ppem);
    if(!child->font->ft_face)
    {
        free_font(child->font);
        child->font = NULL;
        return FALSE;
    }

    child->font->ntmFlags = child->face->ntmFlags;
    child->font->orientation = font->orientation;
    hfontlist = HeapAlloc(GetProcessHeap(), 0, sizeof(*hfontlist));
    hfontlist->hfont = CreateFontIndirectW(&font->font_desc.lf);
    list_add_head(&child->font->hfontlist, &hfontlist->entry);
    child->font->base_font = font;
    list_add_head(&child_font_list, &child->font->entry);
    TRACE("created child font hfont %p for base %p child %p\n", hfontlist->hfont, font, child->font);
    return TRUE;
}

static BOOL get_glyph_index_linked(GdiFont *font, UINT c, GdiFont **linked_font, FT_UInt *glyph)
{
    FT_UInt g;
    CHILD_FONT *child_font;

    if(font->base_font)
        font = font->base_font;

    *linked_font = font;

    if((*glyph = get_glyph_index(font, c)))
        return TRUE;

    LIST_FOR_EACH_ENTRY(child_font, &font->child_fonts, CHILD_FONT, entry)
    {
        if(!child_font->font)
            if(!load_child_font(font, child_font))
                continue;

        if(!child_font->font->ft_face)
            continue;
        g = get_glyph_index(child_font->font, c);
        if(g)
        {
            *glyph = g;
            *linked_font = child_font->font;
            return TRUE;
        }
    }
    return FALSE;
}

/*************************************************************
 * WineEngGetCharWidth
 *
 */
BOOL WineEngGetCharWidth(GdiFont *font, UINT firstChar, UINT lastChar,
			 LPINT buffer)
{
    UINT c;
    GLYPHMETRICS gm;
    FT_UInt glyph_index;
    GdiFont *linked_font;

    TRACE("%p, %d, %d, %p\n", font, firstChar, lastChar, buffer);

    for(c = firstChar; c <= lastChar; c++) {
        get_glyph_index_linked(font, c, &linked_font, &glyph_index);
        WineEngGetGlyphOutline(linked_font, glyph_index, GGO_METRICS | GGO_GLYPH_INDEX,
                               &gm, 0, NULL, NULL);
	buffer[c - firstChar] = FONT_GM(linked_font,glyph_index)->adv;
    }
    return TRUE;
}

/*************************************************************
 * WineEngGetCharABCWidths
 *
 */
BOOL WineEngGetCharABCWidths(GdiFont *font, UINT firstChar, UINT lastChar,
			     LPABC buffer)
{
    UINT c;
    GLYPHMETRICS gm;
    FT_UInt glyph_index;
    GdiFont *linked_font;

    TRACE("%p, %d, %d, %p\n", font, firstChar, lastChar, buffer);

    if(!FT_IS_SCALABLE(font->ft_face))
        return FALSE;

    for(c = firstChar; c <= lastChar; c++) {
        get_glyph_index_linked(font, c, &linked_font, &glyph_index);
        WineEngGetGlyphOutline(linked_font, glyph_index, GGO_METRICS | GGO_GLYPH_INDEX,
                               &gm, 0, NULL, NULL);
	buffer[c - firstChar].abcA = FONT_GM(linked_font,glyph_index)->lsb;
	buffer[c - firstChar].abcB = FONT_GM(linked_font,glyph_index)->bbx;
	buffer[c - firstChar].abcC = FONT_GM(linked_font,glyph_index)->adv - FONT_GM(linked_font,glyph_index)->lsb -
            FONT_GM(linked_font,glyph_index)->bbx;
    }
    return TRUE;
}

/*************************************************************
 * WineEngGetCharABCWidthsI
 *
 */
BOOL WineEngGetCharABCWidthsI(GdiFont *font, UINT firstChar, UINT count, LPWORD pgi,
			      LPABC buffer)
{
    UINT c;
    GLYPHMETRICS gm;
    FT_UInt glyph_index;
    GdiFont *linked_font;

    if(!FT_IS_SCALABLE(font->ft_face))
        return FALSE;

    get_glyph_index_linked(font, 'a', &linked_font, &glyph_index);
    if (!pgi)
        for(c = firstChar; c < firstChar+count; c++) {
            WineEngGetGlyphOutline(linked_font, c, GGO_METRICS | GGO_GLYPH_INDEX,
                                   &gm, 0, NULL, NULL);
            buffer[c - firstChar].abcA = FONT_GM(linked_font,c)->lsb;
            buffer[c - firstChar].abcB = FONT_GM(linked_font,c)->bbx;
            buffer[c - firstChar].abcC = FONT_GM(linked_font,c)->adv - FONT_GM(linked_font,c)->lsb
                - FONT_GM(linked_font,c)->bbx;
        }
    else
        for(c = 0; c < count; c++) {
            WineEngGetGlyphOutline(linked_font, pgi[c], GGO_METRICS | GGO_GLYPH_INDEX,
                                   &gm, 0, NULL, NULL);
            buffer[c].abcA = FONT_GM(linked_font,pgi[c])->lsb;
            buffer[c].abcB = FONT_GM(linked_font,pgi[c])->bbx;
            buffer[c].abcC = FONT_GM(linked_font,pgi[c])->adv
                - FONT_GM(linked_font,pgi[c])->lsb - FONT_GM(linked_font,pgi[c])->bbx;
        }

    return TRUE;
}

/*************************************************************
 * WineEngGetTextExtentExPoint
 *
 */
BOOL WineEngGetTextExtentExPoint(GdiFont *font, LPCWSTR wstr, INT count,
                                 INT max_ext, LPINT pnfit, LPINT dxs, LPSIZE size)
{
    INT idx;
    INT nfit = 0, ext;
    GLYPHMETRICS gm;
    TEXTMETRICW tm;
    FT_UInt glyph_index;
    GdiFont *linked_font;

    TRACE("%p, %s, %d, %d, %p\n", font, debugstr_wn(wstr, count), count,
	  max_ext, size);

    size->cx = 0;
    WineEngGetTextMetrics(font, &tm);
    size->cy = tm.tmHeight;

    for(idx = 0; idx < count; idx++) {
        get_glyph_index_linked(font, wstr[idx], &linked_font, &glyph_index);
        WineEngGetGlyphOutline(linked_font, glyph_index, GGO_METRICS | GGO_GLYPH_INDEX,
                               &gm, 0, NULL, NULL);
	size->cx += FONT_GM(linked_font,glyph_index)->adv;
        ext = size->cx;
        if (! pnfit || ext <= max_ext) {
            ++nfit;
            if (dxs)
                dxs[idx] = ext;
        }
    }

    if (pnfit)
        *pnfit = nfit;

    TRACE("return %d, %d, %d\n", size->cx, size->cy, nfit);
    return TRUE;
}

/*************************************************************
 * WineEngGetTextExtentPointI
 *
 */
BOOL WineEngGetTextExtentPointI(GdiFont *font, const WORD *indices, INT count,
				LPSIZE size)
{
    INT idx;
    GLYPHMETRICS gm;
    TEXTMETRICW tm;

    TRACE("%p, %p, %d, %p\n", font, indices, count, size);

    size->cx = 0;
    WineEngGetTextMetrics(font, &tm);
    size->cy = tm.tmHeight;

    for(idx = 0; idx < count; idx++) {
        WineEngGetGlyphOutline(font, indices[idx],
			       GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL,
			       NULL);
	size->cx += FONT_GM(font,indices[idx])->adv;
    }
    TRACE("return %d,%d\n", size->cx, size->cy);
    return TRUE;
}

/*************************************************************
 * WineEngGetFontData
 *
 */
DWORD WineEngGetFontData(GdiFont *font, DWORD table, DWORD offset, LPVOID buf,
			 DWORD cbData)
{
    FT_Face ft_face = font->ft_face;
    FT_ULong len;
    FT_Error err;

    TRACE("font=%p, table=%c%c%c%c, offset=0x%x, buf=%p, cbData=0x%x\n",
	font, LOBYTE(LOWORD(table)), HIBYTE(LOWORD(table)),
        LOBYTE(HIWORD(table)), HIBYTE(HIWORD(table)), offset, buf, cbData);

    if(!FT_IS_SFNT(ft_face))
        return GDI_ERROR;

    if(!buf || !cbData)
        len = 0;
    else
        len = cbData;

    if(table) { /* MS tags differ in endidness from FT ones */
        table = table >> 24 | table << 24 |
	  (table >> 8 & 0xff00) | (table << 8 & 0xff0000);
    }

    /* If the FT_Load_Sfnt_Table function is there we'll use it */
    if(pFT_Load_Sfnt_Table) {
        /* make sure value of len is the value freetype says it needs */ 
        if( buf && len) {
            FT_ULong needed = 0;
            err = pFT_Load_Sfnt_Table(ft_face, table, offset, NULL, &needed);
            if( !err && needed < len) len = needed;
        }
        err = pFT_Load_Sfnt_Table(ft_face, table, offset, buf, &len);
    }
#ifdef HAVE_FREETYPE_INTERNAL_SFNT_H
    else { /* Do it the hard way */
        TT_Face tt_face = (TT_Face) ft_face;
        SFNT_Interface *sfnt;
        if (FT_Version.major==2 && FT_Version.minor==0)
        {
            /* 2.0.x */
            sfnt = *(SFNT_Interface**)((char*)tt_face + 528);
        }
        else
        {
            /* A field was added in the middle of the structure in 2.1.x */
            sfnt = *(SFNT_Interface**)((char*)tt_face + 532);
        }
        /* make sure value of len is the value freetype says it needs */ 
        if( buf && len) {
            FT_ULong needed = 0;
            err = sfnt->load_any(tt_face, table, offset, NULL, &needed);
            if( !err && needed < len) len = needed;
        }
        err = sfnt->load_any(tt_face, table, offset, buf, &len);
    }
#else
    else {
        static int msg;
        if(!msg) {
            MESSAGE("This version of Wine was compiled with freetype headers later than 2.2.0\n"
                    "but is being run with a freetype library without the FT_Load_Sfnt_Table function.\n"
                    "Please upgrade your freetype library.\n");
            msg++;
        }
        err = FT_Err_Unimplemented_Feature;
    }
#endif
    if(err) {
        TRACE("Can't find table %c%c%c%c\n",
              /* bytes were reversed */
              HIBYTE(HIWORD(table)), LOBYTE(HIWORD(table)),
              HIBYTE(LOWORD(table)), LOBYTE(LOWORD(table)));
	return GDI_ERROR;
    }
    return len;
}

/*************************************************************
 * WineEngGetTextFace
 *
 */
INT WineEngGetTextFace(GdiFont *font, INT count, LPWSTR str)
{
    if(str) {
        lstrcpynW(str, font->name, count);
	return strlenW(font->name);
    } else
        return strlenW(font->name) + 1;
}

UINT WineEngGetTextCharsetInfo(GdiFont *font, LPFONTSIGNATURE fs, DWORD flags)
{
    if (fs) memcpy(fs, &font->fs, sizeof(FONTSIGNATURE));
    return font->charset;
}

BOOL WineEngGetLinkedHFont(DC *dc, WCHAR c, HFONT *new_hfont, UINT *glyph)
{
    GdiFont *font = dc->gdiFont, *linked_font;
    struct list *first_hfont;
    BOOL ret;

    ret = get_glyph_index_linked(font, c, &linked_font, glyph);
    TRACE("get_glyph_index_linked glyph %d font %p\n", *glyph, linked_font);
    if(font == linked_font)
        *new_hfont = dc->hFont;
    else
    {
        first_hfont = list_head(&linked_font->hfontlist);
        *new_hfont = LIST_ENTRY(first_hfont, struct tagHFONTLIST, entry)->hfont;
    }

    return ret;
}
    
/* Retrieve a list of supported Unicode ranges for a given font.
 * Can be called with NULL gs to calculate the buffer size. Returns
 * the number of ranges found.
 */
static DWORD get_font_unicode_ranges(FT_Face face, GLYPHSET *gs)
{
    DWORD num_ranges = 0;

    if (face->charmap->encoding == FT_ENCODING_UNICODE && pFT_Get_First_Char)
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
        FIXME("encoding %u not supported\n", face->charmap->encoding);

    return num_ranges;
}

DWORD WineEngGetFontUnicodeRanges(GdiFont *font, LPGLYPHSET glyphset)
{
    DWORD size = 0;
    DWORD num_ranges = get_font_unicode_ranges(font->ft_face, glyphset);

    size = sizeof(GLYPHSET) + sizeof(WCRANGE) * (num_ranges - 1);
    if (glyphset)
    {
        glyphset->cbThis = size;
        glyphset->cRanges = num_ranges;
    }
    return size;
}

/*************************************************************
 *     FontIsLinked
 */
BOOL WineEngFontIsLinked(GdiFont *font)
{
    return !list_empty(&font->child_fonts);
}

static BOOL is_hinting_enabled(void)
{
    /* Use the >= 2.2.0 function if available */
    if(pFT_Get_TrueType_Engine_Type)
    {
        FT_TrueTypeEngineType type = pFT_Get_TrueType_Engine_Type(library);
        return type == FT_TRUETYPE_ENGINE_TYPE_PATENTED;
    }
#ifdef FT_DRIVER_HAS_HINTER
    else
    {
        FT_Module mod;

        /* otherwise if we've been compiled with < 2.2.0 headers 
           use the internal macro */
        mod = pFT_Get_Module(library, "truetype");
        if(mod && FT_DRIVER_HAS_HINTER(mod))
            return TRUE;
    }
#endif

    return FALSE;
}

/*************************************************************************
 *             GetRasterizerCaps   (GDI32.@)
 */
BOOL WINAPI GetRasterizerCaps( LPRASTERIZER_STATUS lprs, UINT cbNumBytes)
{
    static int hinting = -1;

    if(hinting == -1)
    {
        hinting = is_hinting_enabled();
        TRACE("hinting is %senabled\n", hinting ? "" : "NOT ");
    }

    lprs->nSize = sizeof(RASTERIZER_STATUS);
    lprs->wFlags = TT_AVAILABLE | TT_ENABLED | (hinting ? WINE_TT_HINTER_ENABLED : 0);
    lprs->nLanguageID = 0;
    return TRUE;
}

/*************************************************************************
 * Kerning support for TrueType fonts
 */
#define MS_KERN_TAG MS_MAKE_TAG('k', 'e', 'r', 'n')

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
    USHORT i, nPairs;
    const struct TT_kern_pair *tt_kern_pair;

    TRACE("font height %d, units_per_EM %d\n", font->ppem, font->ft_face->units_per_EM);

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
            kern_pair->iKernAmount -= font->ft_face->units_per_EM / 2;
            kern_pair->iKernAmount -= font->ppem;
        }
        else if (kern_pair->iKernAmount > 0)
        {
            kern_pair->iKernAmount += font->ft_face->units_per_EM / 2;
            kern_pair->iKernAmount += font->ppem;
        }
        kern_pair->iKernAmount /= font->ft_face->units_per_EM;

        TRACE("left %u right %u value %d\n",
               kern_pair->wFirst, kern_pair->wSecond, kern_pair->iKernAmount);

        kern_pair++;
    }
    TRACE("copied %u entries\n", nPairs);
    return nPairs;
}

DWORD WineEngGetKerningPairs(GdiFont *font, DWORD cPairs, KERNINGPAIR *kern_pair)
{
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
            return cPairs;
        }
        return font->total_kern_pairs;
    }

    font->total_kern_pairs = 0;

    length = WineEngGetFontData(font, MS_KERN_TAG, 0, NULL, 0);

    if (length == GDI_ERROR)
    {
        TRACE("no kerning data in the font\n");
        return 0;
    }

    buf = HeapAlloc(GetProcessHeap(), 0, length);
    if (!buf)
    {
        WARN("Out of memory\n");
        return 0;
    }

    WineEngGetFontData(font, MS_KERN_TAG, 0, buf, length);

    /* build a glyph index to char code map */
    glyph_to_char = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(USHORT) * 65536);
    if (!glyph_to_char)
    {
        WARN("Out of memory allocating a glyph index to char code map\n");
        HeapFree(GetProcessHeap(), 0, buf);
        return 0;
    }

    if (font->ft_face->charmap->encoding == FT_ENCODING_UNICODE && pFT_Get_First_Char)
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
        ULONG n;

        FIXME("encoding %u not supported\n", font->ft_face->charmap->encoding);
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
        return cPairs;
    }
    return font->total_kern_pairs;
}

#else /* HAVE_FREETYPE */

/*************************************************************************/

BOOL WineEngInit(void)
{
    return FALSE;
}
GdiFont *WineEngCreateFontInstance(DC *dc, HFONT hfont)
{
    return NULL;
}
BOOL WineEngDestroyFontInstance(HFONT hfont)
{
    return FALSE;
}

DWORD WineEngEnumFonts(LPLOGFONTW plf, FONTENUMPROCW proc, LPARAM lparam)
{
    return 1;
}

DWORD WineEngGetGlyphIndices(GdiFont *font, LPCWSTR lpstr, INT count,
				LPWORD pgi, DWORD flags)
{
    return GDI_ERROR;
}

DWORD WineEngGetGlyphOutline(GdiFont *font, UINT glyph, UINT format,
			     LPGLYPHMETRICS lpgm, DWORD buflen, LPVOID buf,
			     const MAT2* lpmat)
{
    ERR("called but we don't have FreeType\n");
    return GDI_ERROR;
}

BOOL WineEngGetTextMetrics(GdiFont *font, LPTEXTMETRICW ptm)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

UINT WineEngGetOutlineTextMetrics(GdiFont *font, UINT cbSize,
				  OUTLINETEXTMETRICW *potm)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}

BOOL WineEngGetCharWidth(GdiFont *font, UINT firstChar, UINT lastChar,
			 LPINT buffer)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

BOOL WineEngGetCharABCWidths(GdiFont *font, UINT firstChar, UINT lastChar,
			     LPABC buffer)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

BOOL WineEngGetCharABCWidthsI(GdiFont *font, UINT firstChar, UINT count, LPWORD pgi,
			      LPABC buffer)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

BOOL WineEngGetTextExtentExPoint(GdiFont *font, LPCWSTR wstr, INT count,
                                 INT max_ext, LPINT nfit, LPINT dx, LPSIZE size)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

BOOL WineEngGetTextExtentPointI(GdiFont *font, const WORD *indices, INT count,
				LPSIZE size)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

DWORD WineEngGetFontData(GdiFont *font, DWORD table, DWORD offset, LPVOID buf,
			 DWORD cbData)
{
    ERR("called but we don't have FreeType\n");
    return GDI_ERROR;
}

INT WineEngGetTextFace(GdiFont *font, INT count, LPWSTR str)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}

INT WineEngAddFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    FIXME(":stub\n");
    return 1;
}

INT WineEngRemoveFontResourceEx(LPCWSTR file, DWORD flags, PVOID pdv)
{
    FIXME(":stub\n");
    return TRUE;
}

HANDLE WineEngAddFontMemResourceEx(PVOID pbFont, DWORD cbFont, PVOID pdv, DWORD *pcFonts)
{
    FIXME(":stub\n");
    return NULL;
}

UINT WineEngGetTextCharsetInfo(GdiFont *font, LPFONTSIGNATURE fs, DWORD flags)
{
    FIXME(":stub\n");
    return DEFAULT_CHARSET;
}

BOOL WineEngGetLinkedHFont(DC *dc, WCHAR c, HFONT *new_hfont, UINT *glyph)
{
    return FALSE;
}

DWORD WineEngGetFontUnicodeRanges(GdiFont *font, LPGLYPHSET glyphset)
{
    FIXME("(%p, %p): stub\n", font, glyphset);
    return 0;
}

BOOL WineEngFontIsLinked(GdiFont *font)
{
    return FALSE;
}

/*************************************************************************
 *             GetRasterizerCaps   (GDI32.@)
 */
BOOL WINAPI GetRasterizerCaps( LPRASTERIZER_STATUS lprs, UINT cbNumBytes)
{
    lprs->nSize = sizeof(RASTERIZER_STATUS);
    lprs->wFlags = 0;
    lprs->nLanguageID = 0;
    return TRUE;
}

DWORD WineEngGetKerningPairs(GdiFont *font, DWORD cPairs, KERNINGPAIR *kern_pair)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}

#endif /* HAVE_FREETYPE */
