/*******************************************************************************
 *  TrueType font-related functions for Wine PostScript driver.  Currently just
 *  uses FreeType to read font metrics.
 *
 *  Copyright 2001  Ian Pilcher
 *
 */
#include "config.h"

#ifdef HAVE_FREETYPE

/*
 *  These stupid #ifdefs should work for FreeType 2.0.1 and 2.0.2.  Beyond that
 *  is anybody's guess.
 */

#ifdef HAVE_FREETYPE_FREETYPE_H
#include <freetype/freetype.h>
#endif
#ifdef HAVE_FREETYPE_FTGLYPH_H
#include <freetype/ftglyph.h>
#endif
#ifdef HAVE_FREETYPE_TTTABLES_H
#include <freetype/tttables.h>
#endif
#ifdef HAVE_FREETYPE_FTNAMES_H
#include <freetype/ftnames.h>
#endif
#ifdef HAVE_FREETYPE_FTSNAMES_H
#include <freetype/ftsnames.h>
#endif
#ifdef HAVE_FREETYPE_TTNAMEID_H
#include <freetype/ttnameid.h>
#endif

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#include "winnt.h"
#include "winerror.h"
#include "winreg.h"
#include "psdrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(psdrv);


#define REQUIRED_FACE_FLAGS 	(   FT_FACE_FLAG_SCALABLE   |	\
    	    	    	    	    FT_FACE_FLAG_HORIZONTAL |	\
				    FT_FACE_FLAG_SFNT	    |	\
				    FT_FACE_FLAG_GLYPH_NAMES	)

static FT_Library   	library;
static FT_Face	    	face;
static FT_CharMap   	charmap;
static TT_Header    	*head;
static TT_Postscript	*post;
static TT_OS2	    	*os2;
static TT_HoriHeader	*hhea;

/* This is now officially a pain in the ass! */

typedef struct
{
    LPSTR   FontName;
    LPSTR   FullName;
    LPSTR   FamilyName;
    LPSTR   EncodingScheme;
} AFMSTRINGS;

/*******************************************************************************
 *
 *  FindCharMap
 *
 *  Sets charmap and points afm->EncodingScheme to encoding name (in driver
 *  heap).  Leaves both uninitialized if font contains no Windows encoding.
 *
 *  Returns FALSE to indicate memory allocation error.
 *
 */
static const char *encoding_names[7] =
{
    "WindowsSymbol",	    /* TT_MS_ID_SYMBOL_CS */
    "WindowsUnicode",	    /* TT_MS_ID_UNICODE_CS */
    "WindowsShiftJIS",	    /* TT_MS_ID_SJIS */
    "WindowsPRC",  	    /* TT_MS_ID_GB2312 */
    "WindowsBig5",  	    /* TT_MS_ID_BIG_5 */
    "WindowsWansung",	    /* TT_MS_ID_WANSUNG */
    "WindowsJohab"  	    /* TT_MS_ID_JOHAB */
};
 
static BOOL FindCharMap(AFM *afm, AFMSTRINGS *str)
{
    FT_Int  	i;
    FT_Error	error;
    
    charmap = NULL;
    
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
    
    if (charmap == NULL)
    	return TRUE;
	
    error = FT_Set_Charmap(face, charmap);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Set_CharMap", error);
	return FALSE;
    }
	
    if (charmap->encoding_id < 7)
    {
        if (!(str->EncodingScheme = HeapAlloc(PSDRV_Heap, 0,
                                              strlen(encoding_names[charmap->encoding_id])+1 )))
	    return FALSE;
        strcpy( str->EncodingScheme, encoding_names[charmap->encoding_id] );
    }
    else
    {
    	str->EncodingScheme =
	    	HeapAlloc(PSDRV_Heap, 0, sizeof("WindowsUnknown65535"));
	if (str->EncodingScheme == NULL)
	    return FALSE;
	    
	sprintf(str->EncodingScheme, "%s%u", "WindowsUnknown",
	    	charmap->encoding_id);
    }
    
    afm->EncodingScheme = str->EncodingScheme;
    
    return TRUE;
}

/*******************************************************************************
 *  NameTableString
 *
 *  Converts a name table string to a null-terminated character string.  The
 *  space for the character string is allocated from the driver heap.
 *
 *  This function handles only platform_id = 3 (TT_PLATFORM_MICROSOFT) -- 16-bit
 *  big-endian strings.  It also only handles ASCII character codes (< 128).
 *
 *  This function will set *sz to NULL if it cannot parse the string, but it
 *  will only return FALSE in the event of an unexpected error (memory
 *  allocation failure).
 *
 */
static BOOL NameTableString(LPSTR *sz, const FT_SfntName *name)
{    
    FT_UShort 	i, len, *ws;
    LPSTR   	s;
    
    if (name->platform_id != TT_PLATFORM_MICROSOFT)
    {
    	ERR("Unsupported encoding %i\n", name->platform_id);
	return FALSE;	    /* should never get here */
    }
    
    len = name->string_len / 2;
    *sz = s = HeapAlloc(PSDRV_Heap, 0, len + 1);
    if (s == NULL)
    	return FALSE;
    ws = (FT_UShort *)(name->string);
	
    for (i = 0; i < len; ++i, ++s, ++ws)
    {
    	FT_UShort   wc = *ws;
	
#ifndef WORDS_BIGENDIAN
    	wc = (wc >> 8) | (wc << 8);
#endif

    	if (wc > 127)
	{
	    WARN("Non-ASCII character 0x%.4x\n", wc);
	    HeapFree(PSDRV_Heap, 0, *sz);
	    *sz = NULL;
	    return TRUE;
	}
	
	*s = (CHAR)wc;
    }
    
    *s = '\0';
    return TRUE;
}

/*******************************************************************************
 *  ReadNameTable
 *
 *  Reads various font names from the TrueType 'NAME' table.  Currently looks
 *  for U.S. English names only,
 *
 *  May leave a pointer uninitialized if the desired string is not present;
 *  returns FALSE only in the event of an unexpected error.
 *
 */
static BOOL ReadNameTable(AFM *afm, AFMSTRINGS *str)
{
    FT_UInt 	numStrings, stringIndex;
    FT_SfntName name;
    FT_Error	error;

    numStrings = FT_Get_Sfnt_Name_Count(face);
    
    for (stringIndex = 0; stringIndex < numStrings; ++stringIndex)
    {
	error = FT_Get_Sfnt_Name(face, stringIndex, &name);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Get_Sfnt_Name", error);
	    return FALSE;
	}
	
	/* FIXME - Handle other languages? */
	
	if (name.language_id != TT_MS_LANGID_ENGLISH_UNITED_STATES ||
	    	name.platform_id != charmap->platform_id ||
	    	name.encoding_id != charmap->encoding_id)
	    continue;
	
    	switch (name.name_id)
	{
	    case TT_NAME_ID_FONT_FAMILY:
	    
	    	if (NameTableString(&(str->FamilyName), &name) == FALSE)
		    return FALSE;
		afm->FamilyName = str->FamilyName;
		break;
	    
	    case TT_NAME_ID_FULL_NAME:
	    
	    	if (NameTableString(&(str->FullName), &name) == FALSE)
		    return FALSE;
		afm->FullName = str->FullName;
		break;
	    
	    case TT_NAME_ID_PS_NAME:
	    
	    	if (NameTableString(&(str->FontName), &name) == FALSE)
		    return FALSE;
		afm->FontName = str->FontName;
		break;
	}
    }
    
    return TRUE;
}

/*******************************************************************************
 *  PSUnits
 *
 *  Convert TrueType font units (relative to font em square) to PostScript
 *  units.  This is defined as a macro, so it can handle different TrueType
 *  data types as inputs.
 *
 */
#define PSUnits(x)  (((float)(x)) * 1000.0 / ((float)(head->Units_Per_EM)))

/*******************************************************************************
 *  ReadMetricsTables
 *
 *  Reads basic font metrics from the 'head', 'post', and 'OS/2' tables.
 *  Returns FALSE if any table is missing.
 *
 */ 
static BOOL ReadMetricsTables(AFM *afm)
{
    head = FT_Get_Sfnt_Table(face, ft_sfnt_head);
    post = FT_Get_Sfnt_Table(face, ft_sfnt_post);
    hhea = FT_Get_Sfnt_Table(face, ft_sfnt_hhea);
    os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    
    if (head == NULL || post == NULL || hhea == NULL || os2 == NULL)
	return FALSE;
    
    if (os2->version == 0xffff)     	    /* Old Macintosh font */
	return FALSE;
    
    afm->Weight = os2->usWeightClass;
    afm->ItalicAngle = ((float)(post->italicAngle)) / 65536.0;
    afm->IsFixedPitch = (post->isFixedPitch == 0) ? FALSE : TRUE;
    afm->UnderlinePosition = PSUnits(post->underlinePosition);
    afm->UnderlineThickness = PSUnits(post->underlineThickness);
	    
    afm->FontBBox.llx = PSUnits(head->xMin);
    afm->FontBBox.lly = PSUnits(head->yMin);
    afm->FontBBox.urx = PSUnits(head->xMax);
    afm->FontBBox.ury = PSUnits(head->yMax);
    
    /* CapHeight & XHeight set by ReadCharMetrics */
    
    afm->Ascender = PSUnits(os2->sTypoAscender);
    afm->Descender = PSUnits(os2->sTypoDescender);
    afm->FullAscender = afm->FontBBox.ury;  	    /* get rid of this */
    
    afm->WinMetrics.usUnitsPerEm = head->Units_Per_EM;
    afm->WinMetrics.sAscender = hhea->Ascender;
    afm->WinMetrics.sDescender = hhea->Descender;
    afm->WinMetrics.sLineGap = hhea->Line_Gap;
    afm->WinMetrics.sTypoAscender = os2->sTypoAscender;
    afm->WinMetrics.sTypoDescender = os2->sTypoDescender;
    afm->WinMetrics.sTypoLineGap = os2->sTypoLineGap;
    afm->WinMetrics.usWinAscent = os2->usWinAscent;
    afm->WinMetrics.usWinDescent = os2->usWinDescent;
    afm->WinMetrics.sAvgCharWidth = os2->xAvgCharWidth;
    
    return TRUE;
}

/*******************************************************************************
 *  ReadCharMetrics
 *
 *  Reads metrics for each glyph in a TrueType font.
 *
 */
static AFMMETRICS *ReadCharMetrics(AFM *afm)
{
    FT_ULong	    charcode, index;
    AFMMETRICS	    *metrics;
    
    /*
     *	There does not seem to be an easy way to get the number of characters
     *	in an encoding out of a TrueType font.
     */
    for (charcode = 0, index = 0; charcode < 65536; ++charcode)
    {
    	if (FT_Get_Char_Index(face, charcode) != 0)
	    ++index;
    }
    
    afm->NumofMetrics = index;
    
    metrics = HeapAlloc(PSDRV_Heap, 0, index * sizeof(AFMMETRICS));
    if (metrics == NULL)
    	return NULL;
	
    for (charcode = 0, index = 0; charcode <= 65536; ++charcode)
    {
    	FT_UInt     glyph_index = FT_Get_Char_Index(face, charcode);
	FT_Error    error;
	FT_Glyph    glyph;
	FT_BBox     bbox;
	char	    buffer[256];
	
	if (glyph_index == 0)
	    continue;
	    
	error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE |
	    	FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_LINEAR_DESIGN);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Load_Glyph", error);
	    goto cleanup;
	}
	
	error = FT_Get_Glyph(face->glyph, &glyph);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Get_Glyph", error);
	    goto cleanup;
	}
	
	FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_unscaled, &bbox);
	
	error = FT_Get_Glyph_Name(face, glyph_index, buffer, 255);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Get_Glyph_Name", error);
	    goto cleanup;
	}
	
	metrics[index].N = PSDRV_GlyphName(buffer);
	if (metrics[index].N == NULL)
	    goto cleanup;;
	
	metrics[index].C = charcode;
	metrics[index].UV = charcode;
	metrics[index].WX = PSUnits(face->glyph->metrics.horiAdvance);
	metrics[index].B.llx = PSUnits(bbox.xMin);
	metrics[index].B.lly = PSUnits(bbox.yMin);
	metrics[index].B.urx = PSUnits(bbox.xMax);
	metrics[index].B.ury = PSUnits(bbox.yMax);
	metrics[index].L = NULL;
	
	TRACE("Metrics for '%s' WX = %f B = %f,%f - %f,%f\n",
	    	metrics[index].N->sz, metrics[index].WX,
		metrics[index].B.llx, metrics[index].B.lly,
		metrics[index].B.urx, metrics[index].B.ury);
	
	if (charcode == 0x0048)     	    	    /* 'H' */
	    afm->CapHeight = PSUnits(bbox.yMax);
	if (charcode == 0x0078)     	    	    /* 'x' */
	    afm->XHeight = PSUnits(bbox.yMax);
		
	++index;
    }
    
    return metrics;
    
    cleanup:
    
    	HeapFree(PSDRV_Heap, 0, metrics);
	return NULL;
}

/*******************************************************************************
 *  ReadTrueTypeAFM
 *
 *  Fills in AFM structure for opened TrueType font file.  Returns FALSE only on
 *  an unexpected error (memory allocation failure or FreeType error); otherwise
 *  returns TRUE.
 *
 */
static BOOL ReadTrueTypeAFM(AFM *afm)
{
    AFMSTRINGS	str = { NULL, NULL, NULL, NULL };
    AFMMETRICS	*metrics;

    if ((face->face_flags & REQUIRED_FACE_FLAGS) != REQUIRED_FACE_FLAGS)
    {
    	WARN("Font flags do not match requirements\n");
	return TRUE;
    }
    
    if (FindCharMap(afm, &str) == FALSE)
    	return FALSE;
	
    if (charmap == NULL)
    {
    	WARN("No Windows encodings in font\n");
	return TRUE;
    }
    
    TRACE("Using encoding '%s'\n", afm->EncodingScheme);

    if (ReadNameTable(afm, &str) == FALSE)
    {
    	if (str.FontName != NULL)   HeapFree(PSDRV_Heap, 0, str.FontName);
	if (str.FullName != NULL)   HeapFree(PSDRV_Heap, 0, str.FullName);
	if (str.FamilyName != NULL) HeapFree(PSDRV_Heap, 0, str.FamilyName);
	HeapFree(PSDRV_Heap, 0, str.EncodingScheme);
    	return FALSE;
    }
   
    if (str.FamilyName == NULL || str.FullName == NULL || str.FontName == NULL)
    {
    	WARN("Required strings missing from font\n");
    	if (str.FontName != NULL)   HeapFree(PSDRV_Heap, 0, str.FontName);
	if (str.FullName != NULL)   HeapFree(PSDRV_Heap, 0, str.FullName);
	if (str.FamilyName != NULL) HeapFree(PSDRV_Heap, 0, str.FamilyName);
	HeapFree(PSDRV_Heap, 0, str.EncodingScheme);
	return TRUE;
    }

    if (ReadMetricsTables(afm) == FALSE)    /* Non-fatal */
    {
    	WARN("Required metrics tables missing from font\n");
	return TRUE;
    }
    
    afm->Metrics = metrics = ReadCharMetrics(afm);
    if (metrics == NULL)
    {
    	HeapFree(PSDRV_Heap, 0, str.FontName);
	HeapFree(PSDRV_Heap, 0, str.FullName);
	HeapFree(PSDRV_Heap, 0, str.FamilyName);
	HeapFree(PSDRV_Heap, 0, str.EncodingScheme);
    	return FALSE;
    }

    /* Can't do this check until character metrics are read */

    if (afm->WinMetrics.sAvgCharWidth == 0)
    	afm->WinMetrics.sAvgCharWidth = PSDRV_CalcAvgCharWidth(afm);
	
    if (PSDRV_AddAFMtoList(&PSDRV_AFMFontList, afm) == FALSE)
    {
    	HeapFree(PSDRV_Heap, 0, str.FontName);
	HeapFree(PSDRV_Heap, 0, str.FullName);
	HeapFree(PSDRV_Heap, 0, str.FamilyName);
	HeapFree(PSDRV_Heap, 0, str.EncodingScheme);
    	HeapFree(PSDRV_Heap, 0, metrics);
	return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
 *  ReadTrueTypeFile
 *
 *  Reads PostScript-style font metrics from a TrueType font file.  Only returns
 *  FALSE for unexpected errors (memory allocation, etc.); returns TRUE if it's
 *  just a bad font file.
 *
 */
static BOOL ReadTrueTypeFile(LPCSTR filename)
{
    FT_Error	error;
    AFM  	*afm;
    
    TRACE("'%s'\n", filename);

    afm = HeapAlloc(PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(AFM));
    if (afm == NULL)
    	return FALSE;

    error = FT_New_Face(library, filename, 0, &face);
    if (error != FT_Err_Ok)
    {
    	WARN("FreeType error %i opening '%s'\n", error, filename);
	HeapFree(PSDRV_Heap, 0, afm);
	return TRUE;
    }
    
    if (ReadTrueTypeAFM(afm) == FALSE)
    {
    	HeapFree(PSDRV_Heap, 0, afm);
	FT_Done_Face(face);
	return FALSE;
    }    

    error = FT_Done_Face(face);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Done_Face", error);
	HeapFree(PSDRV_Heap, 0, afm);
	return FALSE;
    }
    
    if (afm->Metrics == NULL)	    	    	/* last element to be set */
    {
    	HeapFree(PSDRV_Heap, 0, afm);
	return TRUE;
    }
    
    return TRUE;
}
    


/*******************************************************************************
 *  PSDRV_GetTrueTypeMetrics
 *
 *  Reads PostScript-stype font metrics from TrueType font files in directories
 *  listed in the [TrueType Font Directories] section of the Wine configuration
 *  file.
 *
 *  If this function fails, the driver will fail to initialize and the driver
 *  heap will be destroyed, so it's not necessary to HeapFree everything in
 *  that event.
 *
 */
BOOL PSDRV_GetTrueTypeMetrics(void)
{
    CHAR    	keybuf[256], namebuf[256];
    INT     	i = 0;
    FT_Error	error;
    HKEY    	hkey;
    DWORD   	type, key_len, name_len;

    error = FT_Init_FreeType(&library);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Init_FreeType", error);
	return FALSE;
    }

    if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,
    	    "Software\\Wine\\Wine\\Config\\TrueType Font Directories",
	    0, KEY_READ, &hkey) != ERROR_SUCCESS)
	goto no_metrics;

    key_len = sizeof(keybuf);
    name_len = sizeof(namebuf);
    
    while(RegEnumValueA(hkey, i++, keybuf, &key_len, NULL, &type, namebuf,
    	    &name_len) == ERROR_SUCCESS)
    {
    	struct dirent	*dent;
    	DIR 	    	*dir;
	INT 	    	dnlen;	    /* directory name length */

    	namebuf[sizeof(namebuf) - 1] = '\0';
    	dir = opendir(namebuf);
	if (dir == NULL)
	{
	    WARN("Error opening directory '%s'\n", namebuf);
	    continue;
	}
	
	dnlen = strlen(namebuf);
	namebuf[dnlen] = '/';	    	/* 2 slashes is OK, 0 is not */
	++dnlen;
	
	while ((dent = readdir(dir)) != NULL)
	{
	    INT fnlen;	    /* file name length */
	    
	    fnlen = strlen(dent->d_name);
	    
	    if (fnlen < 5 || strcasecmp(dent->d_name + fnlen - 4, ".ttf") != 0)
	    {
	    	TRACE("Skipping filename '%s'\n", dent->d_name);
		continue;
	    }
	    
	    if (dnlen + fnlen + 1 > sizeof(namebuf))	/* allow for '\0' */
	    {
	    	WARN("Path '%s/%s' is too long\n", namebuf, dent->d_name);
		continue;
	    }
	    
	    memcpy(namebuf + dnlen, dent->d_name, fnlen + 1);
	    
	    if (ReadTrueTypeFile(namebuf) == FALSE)
	    {
	    	ERR("Error reading '%s'\n", namebuf);
	    	closedir(dir);
		RegCloseKey(hkey);
		FT_Done_FreeType(library);
		return FALSE;
	    }
	}
	
	closedir(dir);

	/* initialize lengths for new iteration */
	key_len = sizeof(keybuf);
	name_len = sizeof(namebuf);
    }
    
    RegCloseKey(hkey);

    no_metrics:
    
    FT_Done_FreeType(library);
    return TRUE;
}

#endif  /* HAVE_FREETYPE */
