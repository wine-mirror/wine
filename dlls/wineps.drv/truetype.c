/*******************************************************************************
 *  TrueType font-related functions for Wine PostScript driver.  Currently just
 *  uses FreeType to read font metrics.
 *
 *  Copyright 2001  Ian Pilcher
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
 *
 *  NOTE:  Many of the functions in this file can return either fatal errors
 *  	(memory allocation failure or unexpected FreeType error) or non-fatal
 *  	errors (unusable font file).  Fatal errors are indicated by returning
 *  	FALSE; see individual function descriptions for how they indicate non-
 *  	fatal errors.
 *
 */
#include "config.h"
#include "wine/port.h"

#ifdef HAVE_FREETYPE

/*
 *  These stupid #ifdefs should work for FreeType 2.0.1 and 2.0.2.  Beyond that
 *  is anybody's guess.
 */

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

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

#define REQUIRED_FACE_FLAGS 	(   FT_FACE_FLAG_SCALABLE   |	\
    	    	    	    	    FT_FACE_FLAG_HORIZONTAL |	\
				    FT_FACE_FLAG_SFNT	    |	\
				    FT_FACE_FLAG_GLYPH_NAMES	)

#define GLYPH_LOAD_FLAGS    	(   FT_LOAD_NO_SCALE	    	|   \
    	    	    	    	    FT_LOAD_IGNORE_TRANSFORM	|   \
				    FT_LOAD_LINEAR_DESIGN   	    )

#ifndef SONAME_LIBFREETYPE
#define SONAME_LIBFREETYPE "libfreetype" SONAME_EXT
#endif

static void *ft_handle = NULL;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL;
MAKE_FUNCPTR(FT_Done_Face)
MAKE_FUNCPTR(FT_Done_FreeType)
MAKE_FUNCPTR(FT_Get_Char_Index)
MAKE_FUNCPTR(FT_Get_Glyph_Name)
MAKE_FUNCPTR(FT_Get_Sfnt_Name)
MAKE_FUNCPTR(FT_Get_Sfnt_Name_Count)
MAKE_FUNCPTR(FT_Get_Sfnt_Table)
MAKE_FUNCPTR(FT_Init_FreeType)
MAKE_FUNCPTR(FT_Load_Glyph)
MAKE_FUNCPTR(FT_New_Face)
MAKE_FUNCPTR(FT_Set_Charmap)
#undef MAKE_FUNCPTR

/*******************************************************************************
 *  FindCharMap
 *
 *  Finds Windows character map and creates "EncodingScheme" string.  Returns
 *  FALSE to indicate memory allocation or FreeType error; sets *p_charmap to
 *  NULL if no Windows encoding is present.
 *
 *  Returns Unicode character map if present; otherwise uses the first Windows
 *  character map found.
 *
 */
static const LPCSTR encoding_names[7] =
{
    "WindowsSymbol",	    /* TT_MS_ID_SYMBOL_CS */
    "WindowsUnicode",	    /* TT_MS_ID_UNICODE_CS */
    "WindowsShiftJIS",	    /* TT_MS_ID_SJIS */
    "WindowsPRC",  	    /* TT_MS_ID_GB2312 */
    "WindowsBig5",  	    /* TT_MS_ID_BIG_5 */
    "WindowsWansung",	    /* TT_MS_ID_WANSUNG */
    "WindowsJohab"  	    /* TT_MS_ID_JOHAB */
/*  "WindowsUnknown65535"   is the longest possible (encoding_id is a UShort) */
};

static BOOL FindCharMap(FT_Face face, FT_CharMap *p_charmap, LPSTR *p_sz)
{
    FT_Int  	i;
    FT_Error	error;
    FT_CharMap	charmap = NULL;

    for (i = 0; i < face->num_charmaps; ++i)
    {
    	if (face->charmaps[i]->platform_id != TT_PLATFORM_MICROSOFT)
	    continue;

	if (face->charmaps[i]->encoding_id == TT_MS_ID_UNICODE_CS)
	{
	    charmap = face->charmaps[i];
	    break;
	}

	if (charmap == NULL)
	    charmap = face->charmaps[i];
    }

    *p_charmap = charmap;

    if (charmap == NULL)
    {
    	WARN("No Windows character map found\n");
	return TRUE;
    }

    error = pFT_Set_Charmap(face, charmap);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Set_Charmap", error);
	return FALSE;
    }

    *p_sz = HeapAlloc(PSDRV_Heap, 0, sizeof("WindowsUnknown65535"));
    if (*p_sz == NULL)
    	return FALSE;

    if (charmap->encoding_id < 7)
    	strcpy(*p_sz, encoding_names[charmap->encoding_id]);
    else
    	sprintf(*p_sz, "%s%u", "WindowsUnknown", charmap->encoding_id);

    return TRUE;
}

/*******************************************************************************
 *  MSTTStrToSz
 *
 *  Converts a string in the TrueType NAME table to a null-terminated ASCII
 *  character string.  Space for the string is allocated from the driver heap.
 *  Only handles platform_id = 3 (TT_PLATFORM_MICROSOFT) strings (16-bit, big
 *  endian).  It also only handles ASCII character codes (< 128).
 *
 *  Sets *p_sz to NULL if string cannot be converted; only returns FALSE for
 *  memory allocation failure.
 *
 */
static BOOL MSTTStrToSz(const FT_SfntName *name, LPSTR *p_sz)
{
    FT_UShort	i;
    INT     	len;
    BYTE  	*wsz;
    LPSTR   	sz;

    len = name->string_len / 2;     	    	    /* # of 16-bit chars */

    *p_sz = sz = HeapAlloc(PSDRV_Heap, 0, len + 1);
    if (sz == NULL)
    	return FALSE;

    wsz = (BYTE *)name->string;

    for (i = 0; i < len; ++i, ++sz)
    {
        USHORT wc = (wsz[0] << 8) + wsz[1];
        wsz += 2;

    	if (wc > 127)
	{
	    WARN("Non-ASCII character 0x%.4x\n", wc);
	    HeapFree(PSDRV_Heap, 0, *p_sz);
	    *p_sz = NULL;
	    return TRUE;
	}

	*sz = (CHAR)wc;
    }

    *sz = '\0';

    return TRUE;
}

/*******************************************************************************
 *  FindMSTTString
 *
 *  Finds the requested Microsoft platform string in the TrueType NAME table and
 *  converts it to a null-terminated ASCII string.  Currently looks for U.S.
 *  English names only.
 *
 *  Sets string to NULL if not present or cannot be converted; returns FALSE
 *  only for memory allocation failure.
 *
 */
static BOOL FindMSTTString(FT_Face face, FT_CharMap charmap, FT_UShort name_id,
    	LPSTR *p_sz)
{
    FT_UInt 	    num_strings, string_index;
    FT_SfntName     name;
    FT_Error	    error;

    num_strings = pFT_Get_Sfnt_Name_Count(face);

    for (string_index = 0; string_index < num_strings; ++string_index)
    {
    	error = pFT_Get_Sfnt_Name(face, string_index, &name);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Get_Sfnt_Name", error);
	    return FALSE;
	}

	/* FIXME - Handle other languages? */

	if (name.platform_id != TT_PLATFORM_MICROSOFT ||
	    	name.language_id != TT_MS_LANGID_ENGLISH_UNITED_STATES)
	    continue;

	if (name.platform_id != charmap->platform_id ||
	    	name.encoding_id != charmap->encoding_id)
	    continue;

	if (name.name_id != name_id)
	    continue;

	return MSTTStrToSz(&name, p_sz);
    }

    *p_sz = NULL;   	    	    /* didn't find it */

    return TRUE;
}

/*******************************************************************************
 *  PSUnits
 *
 *  Convert TrueType font units (relative to font em square) to PostScript
 *  units.
 *
 */
static inline float PSUnits(LONG x, USHORT em_size)
{
    return 1000.0 * (float)x / (float)em_size;
}

/*******************************************************************************
 *  StartAFM
 *
 *  Allocates space for the AFM on the driver heap and reads basic font metrics
 *  from the HEAD, POST, HHEA, and OS/2 tables.  Returns FALSE for memory
 *  allocation error; sets *p_afm to NULL if required information is missing.
 *
 */
static BOOL StartAFM(FT_Face face, AFM **p_afm)
{
    TT_Header	    *head;
    TT_Postscript   *post;
    TT_OS2  	    *os2;
    TT_HoriHeader   *hhea;
    USHORT  	    em_size;
    AFM     	    *afm;

    head = pFT_Get_Sfnt_Table(face, ft_sfnt_head);
    post = pFT_Get_Sfnt_Table(face, ft_sfnt_post);
    os2 = pFT_Get_Sfnt_Table(face, ft_sfnt_os2);
    hhea = pFT_Get_Sfnt_Table(face, ft_sfnt_hhea);

    if (head == NULL || post == NULL || os2 == NULL || hhea == NULL ||
    	    os2->version == 0xffff) 	    	    	/* old Macintosh font */
    {
    	WARN("Required table(s) missing\n");
	*p_afm = NULL;
	return TRUE;
    }

    *p_afm = afm = HeapAlloc(PSDRV_Heap, 0, sizeof(*afm));
    if (afm == NULL)
    	return FALSE;

    afm->WinMetrics.usUnitsPerEm = em_size = head->Units_Per_EM;
    afm->WinMetrics.sAscender = hhea->Ascender;
    afm->WinMetrics.sDescender = hhea->Descender;
    afm->WinMetrics.sLineGap = hhea->Line_Gap;
    afm->WinMetrics.sTypoAscender = os2->sTypoAscender;
    afm->WinMetrics.sTypoDescender = os2->sTypoDescender;
    afm->WinMetrics.sTypoLineGap = os2->sTypoLineGap;
    afm->WinMetrics.usWinAscent = os2->usWinAscent;
    afm->WinMetrics.usWinDescent = os2->usWinDescent;
    afm->WinMetrics.sAvgCharWidth = os2->xAvgCharWidth;

    afm->Weight = os2->usWeightClass;
    afm->ItalicAngle = ((float)(post->italicAngle)) / 65536.0;
    afm->IsFixedPitch = (post-> isFixedPitch == 0) ? FALSE : TRUE;
    afm->UnderlinePosition = PSUnits(post->underlinePosition, em_size);
    afm->UnderlineThickness = PSUnits(post->underlineThickness, em_size);

    afm->FontBBox.llx = PSUnits(head->xMin, em_size);
    afm->FontBBox.lly = PSUnits(head->yMin, em_size);
    afm->FontBBox.urx = PSUnits(head->xMax, em_size);
    afm->FontBBox.ury = PSUnits(head->yMax, em_size);

    afm->Ascender = PSUnits(os2->sTypoAscender, em_size);
    afm->Descender = PSUnits(os2->sTypoDescender, em_size);

    return TRUE;
}

/*******************************************************************************
 *  ReadCharMetrics
 *
 *  Reads metrics for each glyph in a TrueType font.  Returns false for memory
 *  allocation or FreeType error; sets *p_metrics to NULL for non-fatal error.
 *
 */
static BOOL ReadCharMetrics(FT_Face face, AFM *afm, AFMMETRICS **p_metrics)
{
    FT_ULong	charcode, index;
    AFMMETRICS	*metrics;
    USHORT  	em_size = afm->WinMetrics.usUnitsPerEm;

    for (charcode = 0, index = 0; charcode < 65536; ++charcode)
    	if (pFT_Get_Char_Index(face, charcode) != 0)
	    ++index;	    	    	    	    	/* count # of glyphs */

    afm->NumofMetrics = index;

    *p_metrics = metrics = HeapAlloc(PSDRV_Heap, 0, index * sizeof(*metrics));
    if (metrics == NULL)
    	return FALSE;

    for (charcode = 0, index = 0; charcode < 65536; ++charcode)
    {
    	FT_UInt     glyph_index = pFT_Get_Char_Index(face, charcode);
	FT_Error    error;
	CHAR	    buffer[128];  	    	/* for glyph names */

	if (glyph_index == 0)
	    continue;

	error = pFT_Load_Glyph(face, glyph_index, GLYPH_LOAD_FLAGS);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Load_Glyph", error);
	    goto cleanup;
	}

	error = pFT_Get_Glyph_Name(face, glyph_index, buffer, sizeof(buffer));
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Get_Glyph_Name", error);
	    goto cleanup;
    	}

	metrics[index].N = PSDRV_GlyphName(buffer);
	if (metrics[index].N == NULL)
	    goto cleanup;

	metrics[index].C = metrics[index].UV = charcode;
	metrics[index].WX = PSUnits(face->glyph->metrics.horiAdvance, em_size);

	++index;
    }

    if (afm->WinMetrics.sAvgCharWidth == 0)
    	afm->WinMetrics.sAvgCharWidth = PSDRV_CalcAvgCharWidth(afm);

    return TRUE;

    cleanup:
    	HeapFree(PSDRV_Heap, 0, metrics);

    return FALSE;
}

/*******************************************************************************
 *  BuildTrueTypeAFM
 *
 *  Builds the AFM for a TrueType font and adds it to the driver font list.
 *  Returns FALSE only on an unexpected error (memory allocation failure or
 *  FreeType error).
 *
 */
static BOOL BuildTrueTypeAFM(FT_Face face)
{
    AFM     	*afm;
    AFMMETRICS	*metrics;
    LPSTR   	font_name, full_name, family_name, encoding_scheme = NULL;
    FT_CharMap	charmap;
    BOOL    	retval, added;

    retval = StartAFM(face, &afm);
    if (retval == FALSE || afm == NULL)
    	return retval;

    retval = FindCharMap(face, &charmap, &encoding_scheme);
    if (retval == FALSE || charmap == NULL)
    	goto cleanup_afm;

    retval = FindMSTTString(face, charmap, TT_NAME_ID_PS_NAME, &font_name);
    if (retval == FALSE || font_name == NULL)
    	goto cleanup_encoding_scheme;

    retval = FindMSTTString(face, charmap, TT_NAME_ID_FULL_NAME, &full_name);
    if (retval == FALSE || full_name == NULL)
    	goto cleanup_font_name;

    retval = FindMSTTString(face, charmap, TT_NAME_ID_FONT_FAMILY,
    	    &family_name);
    if (retval == FALSE || family_name == NULL)
    	goto cleanup_full_name;

    retval = ReadCharMetrics(face, afm, &metrics);
    if (retval == FALSE || metrics == NULL)
    	goto cleanup_family_name;

    afm->EncodingScheme = encoding_scheme; afm->FontName = font_name;
    afm->FullName = full_name; afm->FamilyName = family_name;
    afm->Metrics = metrics;

    retval = PSDRV_AddAFMtoList(&PSDRV_AFMFontList, afm, &added);
    if (retval == FALSE || added == FALSE)
    	goto cleanup_family_name;

    return TRUE;

    /* clean up after fatal or non-fatal errors */

    cleanup_family_name:
    	HeapFree(PSDRV_Heap, 0, family_name);
    cleanup_full_name:
    	HeapFree(PSDRV_Heap, 0, full_name);
    cleanup_font_name:
    	HeapFree(PSDRV_Heap, 0, font_name);
    cleanup_encoding_scheme:
    	HeapFree(PSDRV_Heap, 0, encoding_scheme);
    cleanup_afm:
    	HeapFree(PSDRV_Heap, 0, afm);

    return retval;
}

/*******************************************************************************
 *  ReadTrueTypeFile
 *
 *  Reads font metrics from TrueType font file.  Only returns FALSE for
 *  unexpected errors (memory allocation failure or FreeType error).
 *
 */
static BOOL ReadTrueTypeFile(FT_Library library, LPCSTR filename)
{
    FT_Error	    error;
    FT_Face 	    face;

    TRACE("%s\n", filename);

    error = pFT_New_Face(library, filename, 0, &face);
    if (error != FT_Err_Ok)
    {
    	WARN("FreeType error %i opening %s\n", error, filename);
	return TRUE;
    }

    if ((face->face_flags & REQUIRED_FACE_FLAGS) == REQUIRED_FACE_FLAGS)
    {
    	if (BuildTrueTypeAFM(face) == FALSE)
	{
	    pFT_Done_Face(face);
	    return FALSE;
	}
    }
    else
    {
    	WARN("Required information missing from %s\n", filename);
    }

    error = pFT_Done_Face(face);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Done_Face", error);
	return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
 *  ReadTrueTypeDir
 *
 *  Reads all TrueType font files in a directory.
 *
 */
static BOOL ReadTrueTypeDir(FT_Library library, LPCSTR dirname)
{
    struct dirent   *dent;
    DIR     	    *dir;
    CHAR    	    filename[256];

    dir = opendir(dirname);
    if (dir == NULL)
    {
    	WARN("'%s' opening %s\n", strerror(errno), dirname);
	return TRUE;
    }

    while ((dent = readdir(dir)) != NULL)
    {
    	CHAR	    *file_extension = strrchr(dent->d_name, '.');
	int 	    fn_len;

	if (file_extension == NULL || strcasecmp(file_extension, ".ttf") != 0)
	    continue;

	fn_len = snprintf(filename, 256, "%s/%s", dirname, dent->d_name);
	if (fn_len < 0 || fn_len > sizeof(filename) - 1)
	{
	    WARN("Path '%s/%s' is too long\n", dirname, dent->d_name);
	    continue;
	}

	if (ReadTrueTypeFile(library, filename) ==  FALSE)
	{
	    closedir(dir);
	    return FALSE;
	}
    }

    closedir(dir);

    return TRUE;
}

/*******************************************************************************
 *  PSDRV_GetTrueTypeMetrics
 *
 *  Reads font metrics from TrueType font files in directories listed in the
 *  [TrueType Font Directories] section of the Wine configuration file.
 *
 *  If this function fails (returns FALSE), the driver will fail to initialize
 *  and the driver heap will be destroyed, so it's not necessary to HeapFree
 *  everything in that event.
 *
 */
BOOL PSDRV_GetTrueTypeMetrics(void)
{
    static const WCHAR pathW[] = {'P','a','t','h',0};
    FT_Error	error;
    FT_Library	library;
    HKEY hkey;
    DWORD len;
    LPWSTR valueW;
    LPSTR valueA, ptr;

    /* @@ Wine registry key: HKCU\Software\Wine\Fonts */
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Fonts", &hkey) != ERROR_SUCCESS)
        return TRUE;

    ft_handle = wine_dlopen(SONAME_LIBFREETYPE, RTLD_NOW, NULL, 0);
    if(!ft_handle) {
        WINE_MESSAGE(
      "Wine cannot find the FreeType font library.  To enable Wine to\n"
      "use TrueType fonts please install a version of FreeType greater than\n"
      "or equal to 2.0.5.\n"
      "http://www.freetype.org\n");
        RegCloseKey(hkey);
	return TRUE;
    }

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(ft_handle, #f, NULL, 0)) == NULL) goto sym_not_found;
    LOAD_FUNCPTR(FT_Done_Face)
    LOAD_FUNCPTR(FT_Done_FreeType)
    LOAD_FUNCPTR(FT_Get_Char_Index)
    LOAD_FUNCPTR(FT_Get_Glyph_Name)
    LOAD_FUNCPTR(FT_Get_Sfnt_Name)
    LOAD_FUNCPTR(FT_Get_Sfnt_Name_Count)
    LOAD_FUNCPTR(FT_Get_Sfnt_Table)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_New_Face)
    LOAD_FUNCPTR(FT_Set_Charmap)
#undef LOAD_FUNCPTR

    error = pFT_Init_FreeType(&library);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Init_FreeType", error);
	wine_dlclose(ft_handle, NULL, 0);
	RegCloseKey(hkey);
	return FALSE;
    }

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
                ReadTrueTypeDir( library, ptr );
                ptr = next;
            }
            HeapFree( GetProcessHeap(), 0, valueA );
        }
        HeapFree( GetProcessHeap(), 0, valueW );
    }

    RegCloseKey(hkey);
    pFT_Done_FreeType(library);
    wine_dlclose(ft_handle, NULL, 0);
    ft_handle = NULL;
    return TRUE;

sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the FreeType\n"
      "font library.  To enable Wine to use TrueType fonts please upgrade\n"
      "FreeType to at least version 2.0.5.\n"
      "http://www.freetype.org\n");
    RegCloseKey(hkey);
    wine_dlclose(ft_handle, NULL, 0);
    ft_handle = NULL;
    return TRUE;
}

#endif  /* HAVE_FREETYPE */
