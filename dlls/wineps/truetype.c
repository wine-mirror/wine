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
#include "winreg.h"
#include "psdrv.h"
#include "debugtools.h"
#include "heap.h"

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
 
static BOOL FindCharMap(AFM *afm)
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
    	afm->EncodingScheme = HEAP_strdupA(PSDRV_Heap, 0,
	    	encoding_names[charmap->encoding_id]);
	if (afm->EncodingScheme == NULL)
	    return FALSE;
    }
    else
    {
    	afm->EncodingScheme = HeapAlloc(PSDRV_Heap, 0,	    /* encoding_id   */
	    	sizeof("WindowsUnknown65535"));     	    /*   is a UShort */
	if (afm->EncodingScheme == NULL)
	    return FALSE;
	    
	sprintf(afm->EncodingScheme, "%s%u", "WindowsUnknown",
	    	charmap->encoding_id);
    }
    
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
    s = *sz = HeapAlloc(PSDRV_Heap, 0, len + 1);
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
static BOOL ReadNameTable(AFM *afm)
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
	    
	    	if (NameTableString(&(afm->FamilyName), &name) == FALSE)
		    return FALSE;
		break;
	    
	    case TT_NAME_ID_FULL_NAME:
	    
	    	if (NameTableString(&(afm->FullName), &name) == FALSE)
		    return FALSE;
		break;
	    
	    case TT_NAME_ID_PS_NAME:
	    
	    	if (NameTableString(&(afm->FontName), &name) == FALSE)
		    return FALSE;
		break;
	}
    }
    
    return TRUE;
}

/*******************************************************************************
 *  FreeAFM
 *
 *  Frees an AFM and all subsidiary objects.  For this function to work
 *  properly, the AFM must have been allocated with HEAP_ZERO_MEMORY, and the
 *  UNICODEVECTOR and it's associated array of UNICODEGLYPHs must have been
 *  allocated as a single object.
 */
static void FreeAFM(AFM *afm)
{
    if (afm->FontName != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->FontName);
    if (afm->FullName != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->FullName);
    if (afm->FamilyName != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->FamilyName);
    if (afm->EncodingScheme != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->EncodingScheme);
    if (afm->Metrics != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->Metrics);
    if (afm->Encoding != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->Encoding);
	
    HeapFree(PSDRV_Heap, 0, afm);
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
 *  Reads metrics for each glyph in a TrueType font.  Since FreeAFM will try to
 *  free afm->Metrics and afm->Encoding if they are non-NULL, don't free them
 *  in the event of an error.  (FreeAFM depends on the fact that afm->Encoding
 *  and its associated array of UNICODEGLYPHs are allocated as a single object.)
 *
 */
static BOOL ReadCharMetrics(AFM *afm)
{
    FT_ULong	    charcode, index;
    UNICODEGLYPH    *glyphs;
    
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
    
    afm->Metrics = HeapAlloc(PSDRV_Heap, 0, index * sizeof(AFMMETRICS));
    afm->Encoding = HeapAlloc(PSDRV_Heap, 0, sizeof(UNICODEVECTOR) +
    	    index * sizeof(UNICODEGLYPH));
    if (afm->Metrics == NULL || afm->Encoding == NULL)
    	return FALSE;
	
    glyphs = (UNICODEGLYPH *)(afm->Encoding + 1);
    afm->Encoding->size = index;
    afm->Encoding->glyphs = glyphs;
    
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
	    return FALSE;
	}
	
	error = FT_Get_Glyph(face->glyph, &glyph);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Get_Glyph", error);
	    return FALSE;
	}
	
	FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_unscaled, &bbox);
	
	error = FT_Get_Glyph_Name(face, glyph_index, buffer, 255);
	if (error != FT_Err_Ok)
	{
	    ERR("%s returned %i\n", "FT_Get_Glyph_Name", error);
	    return FALSE;
	}
	
	afm->Metrics[index].N = PSDRV_GlyphName(buffer);
	if (afm->Metrics[index].N == NULL)
	    return FALSE;
	
	afm->Metrics[index].C = charcode;
	afm->Metrics[index].UV = charcode;
	afm->Metrics[index].WX = PSUnits(face->glyph->metrics.horiAdvance);
	afm->Metrics[index].B.llx = PSUnits(bbox.xMin);
	afm->Metrics[index].B.lly = PSUnits(bbox.yMin);
	afm->Metrics[index].B.urx = PSUnits(bbox.xMax);
	afm->Metrics[index].B.ury = PSUnits(bbox.yMax);
	afm->Metrics[index].L = NULL;
	
	TRACE("Metrics for '%s' WX = %f B = %f,%f - %f,%f\n",
	    	afm->Metrics[index].N->sz, afm->Metrics[index].WX,
		afm->Metrics[index].B.llx, afm->Metrics[index].B.lly,
	      	afm->Metrics[index].B.urx, afm->Metrics[index].B.ury);
	
	glyphs[index].UV = charcode;
	glyphs[index].name = afm->Metrics[index].N;
	
	if (charcode == 0x0048)     	    	    /* 'H' */
	    afm->CapHeight = PSUnits(bbox.yMax);
	if (charcode == 0x0078)     	    	    /* 'x' */
	    afm->XHeight = PSUnits(bbox.yMax);
		
	++index;
    }
    
    return TRUE;
}

/*******************************************************************************
 *  ReadTrueTypeAFM
 *
 *  Fills in AFM structure for opened TrueType font file.  Returns FALSE only on
 *  an unexpected error (memory allocation failure or FreeType error); otherwise
 *  returns TRUE.  Leaves it to the caller (ReadTrueTypeFile) to clean up.
 *
 */
static BOOL ReadTrueTypeAFM(AFM *afm)
{

    if ((face->face_flags & REQUIRED_FACE_FLAGS) != REQUIRED_FACE_FLAGS)
    {
    	WARN("Font flags do not match requirements\n");
	return TRUE;
    }
    
    if (FindCharMap(afm) == FALSE)
    	return FALSE;
	
    if (charmap == NULL)
    {
    	WARN("No Windows encodings in font\n");
	return TRUE;
    }
    
    TRACE("Using encoding '%s'\n", afm->EncodingScheme);

    if (ReadNameTable(afm) == FALSE)
    	return FALSE;
   
    if (afm->FamilyName == NULL || afm->FullName == NULL ||
    	    afm->FontName == NULL)
    {
    	WARN("Required strings missing from font\n");
	return TRUE;
    }

    if (ReadMetricsTables(afm) == FALSE)    /* Non-fatal */
    {
    	WARN("Required metrics tables missing from font\n");
	return TRUE;
    }
    
    if (ReadCharMetrics(afm) == FALSE)
    	return FALSE;

    /* Can't do this check until character metrics are read */

    if (afm->WinMetrics.sAvgCharWidth == 0)
    	afm->WinMetrics.sAvgCharWidth = PSDRV_CalcAvgCharWidth(afm);

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
    AFM     	*afm;
    
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
    	FreeAFM(afm);
	FT_Done_Face(face);
	return FALSE;
    }    

    error = FT_Done_Face(face);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Done_Face", error);
	FreeAFM(afm);
	return FALSE;
    }
    
    if (afm->Encoding == NULL)	    /* last element to be set */
    {
    	FreeAFM(afm);
	return TRUE;
    }
    
    if (PSDRV_AddAFMtoList(&PSDRV_AFMFontList, afm) == FALSE)
    {
    	FreeAFM(afm);
	return FALSE;
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
    HKEY hkey;
    DWORD type, key_len, name_len;

    error = FT_Init_FreeType(&library);
    if (error != FT_Err_Ok)
    {
    	ERR("%s returned %i\n", "FT_Init_FreeType", error);
	return FALSE;
    }

    if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\TrueType Font Directories",
		     0, KEY_READ, &hkey))
	goto no_metrics;

    key_len = sizeof(keybuf);
    name_len = sizeof(namebuf);
    while(!RegEnumValueA(hkey, i++, keybuf, &key_len, NULL, &type, namebuf, &name_len))
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
