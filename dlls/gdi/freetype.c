/*
 * FreeType font engine interface
 *
 * Copyright 2001 Huw D M Davies for CodeWeavers.
 *
 * This file contains the WineEng* functions.
 */


#include "config.h"

#include "windef.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "wine/unicode.h"
#include "gdi.h"
#include "font.h"
#include "debugtools.h"

#include <string.h>
#include <dirent.h>
#include <stdio.h>

DEFAULT_DEBUG_CHANNEL(font);

#ifdef HAVE_FREETYPE

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
#ifdef HAVE_FREETYPE_FTOUTLN_H
#include <freetype/ftoutln.h>
#endif

static FT_Library library = 0;

typedef struct tagFace {
    WCHAR *StyleName;
    char *file;
    BOOL Italic;
    BOOL Bold;
    struct tagFace *next;
} Face;

typedef struct tagFamily {
    WCHAR *FamilyName;
    Face *FirstFace;
    struct tagFamily *next;
} Family;

struct tagGdiFont {
    DWORD ref;
    FT_Face ft_face;
};

static Family *FontList = NULL;

static BOOL AddFontFileToList(char *file)
{
    FT_Face ft_face;
    WCHAR *FamilyW, *StyleW;
    DWORD len;
    Family *family = FontList;
    Family **insert = &FontList;
    Face **insertface;

    TRACE("Loading font file %s\n", debugstr_a(file));
    if(FT_New_Face(library, file, 0, &ft_face)) {
        ERR("Unable to load font file %s\n", debugstr_a(file));
	return FALSE;
    }

    if(!FT_IS_SFNT(ft_face)) { /* for now we'll skip everything but TT/OT */
        FT_Done_Face(ft_face);
	return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1, NULL, 0);
    FamilyW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1, FamilyW, len);

    while(family) {
        if(!strcmpW(family->FamilyName, FamilyW))
	    break;
	insert = &family->next;
	family = family->next;
    }
    if(!family) {
        family = *insert = HeapAlloc(GetProcessHeap(), 0, sizeof(*family));
	family->FamilyName = FamilyW;
	family->FirstFace = NULL;
	family->next = NULL;
    } else {
        HeapFree(GetProcessHeap(), 0, FamilyW);
    }

    len = MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, NULL, 0);
    StyleW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, StyleW, len);

    
    for(insertface = &family->FirstFace; *insertface;
	insertface = &(*insertface)->next) {
        if(!strcmpW((*insertface)->StyleName, StyleW)) {
	    ERR("Already loaded font %s %s\n", debugstr_w(family->FamilyName),
		debugstr_w(StyleW));
	    HeapFree(GetProcessHeap(), 0, StyleW);
	    FT_Done_Face(ft_face);
	    return FALSE;
	}
    }
    *insertface = HeapAlloc(GetProcessHeap(), 0, sizeof(**insertface));
    (*insertface)->StyleName = StyleW;
    (*insertface)->file = HeapAlloc(GetProcessHeap(),0,strlen(file)+1);
    strcpy((*insertface)->file, file);
    (*insertface)->next = NULL;
    (*insertface)->Italic = (ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 1 : 0;
    (*insertface)->Bold = (ft_face->style_flags & FT_STYLE_FLAG_BOLD) ? 1 : 0;
    FT_Done_Face(ft_face);

    TRACE("Added font %s %s\n", debugstr_w(family->FamilyName),
	  debugstr_w(StyleW));
    return TRUE;
}

static void DumpFontList(void)
{
    Family *family;
    Face *face;

    for(family = FontList; family; family = family->next) {
        TRACE("Family: %s\n", debugstr_w(family->FamilyName));
        for(face = family->FirstFace; face; face = face->next) {
	    TRACE("\t%s\n", debugstr_w(face->StyleName));
	}
    }
    return;
}

static BOOL ReadFontDir(char *dirname)
{
    DIR *dir;
    struct dirent *dent;
    char path[MAX_PATH];

    dir = opendir(dirname);
    if(!dir) {
        ERR("Can't open directory %s\n", debugstr_a(dirname));
	return FALSE;
    }
    while((dent = readdir(dir)) != NULL) {
        if(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
	    continue;
	sprintf(path, "%s/%s", dirname, dent->d_name);
	AddFontFileToList(path);
    }
    return TRUE;
}

/*************************************************************
 *    WineEngInit
 *
 * Initialize FreeType library and create a list of available faces
 */
BOOL WineEngInit(void)
{
    HKEY hkey;
    DWORD valuelen, datalen, i = 0, type, dlen, vlen;
    LPSTR value;
    LPVOID data;

    if(FT_Init_FreeType(&library) != 0) {
        ERR("Can't init FreeType library\n");
	return FALSE;
    }

    if(RegOpenKeyA(HKEY_LOCAL_MACHINE,
		   "Software\\Wine\\Wine\\Config\\FontDirs",
		   &hkey) != ERROR_SUCCESS) {
        TRACE("Can't open FontDirs key in config file\n");
	return FALSE;
    }

    RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &valuelen,
		     &datalen, NULL, NULL);

    valuelen++; /* returned value doesn't include room for '\0' */
    value = HeapAlloc(GetProcessHeap(), 0, valuelen);
    data = HeapAlloc(GetProcessHeap(), 0, datalen);

    dlen = datalen;
    vlen = valuelen;
    while(RegEnumValueA(hkey, i++, value, &vlen, NULL, &type, data,
			&dlen) == ERROR_SUCCESS) {
        TRACE("Got %s=%s\n", value, (LPSTR)data);
	ReadFontDir((LPSTR)data);
	/* reset dlen and vlen */
	dlen = datalen;
	vlen = valuelen;
    }
    HeapFree(GetProcessHeap(), 0, data);
    HeapFree(GetProcessHeap(), 0, value);
    RegCloseKey(hkey);
    DumpFontList();
    return TRUE;
}


static FT_Face OpenFontFile(char *file, LONG height)
{
    FT_Error err;
    TT_OS2 *pOS2;
    FT_Face ft_face;
    LONG ppem;

    err = FT_New_Face(library, file, 0, &ft_face);
    if(err) {
        ERR("FT_New_Face rets %d\n", err);
	return 0;
    }

    pOS2 = FT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);

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

    if(height > 0)
        ppem = ft_face->units_per_EM * height /
	  (pOS2->usWinAscent + pOS2->usWinDescent);
    else
        ppem = -height;

    FT_Set_Pixel_Sizes(ft_face, 0, ppem);
    return ft_face;
}

/*************************************************************
 * WineEngCreateFontInstance
 *
 */
GdiFont WineEngCreateFontInstance(HFONT hfont)
{
    GdiFont ret;
    Face *face;
    Family *family = NULL;
    WCHAR FaceName[LF_FACESIZE];
    BOOL bd, it;
    FONTOBJ *font = GDI_GetObjPtr(hfont, FONT_MAGIC);
    LOGFONTW *plf = &font->logfont;

    TRACE("%s, h=%ld, it=%d, weight=%ld\n", debugstr_w(plf->lfFaceName),
	  plf->lfHeight, plf->lfItalic, plf->lfWeight);

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(*ret));
    ret->ref = 1;

    strcpyW(FaceName, plf->lfFaceName);

    if(FaceName[0] != '\0') {
        for(family = FontList; family; family = family->next) {
	    if(!strcmpiW(family->FamilyName, FaceName))
	         break;
	}
    }

    if(!family) {
        family = FontList;
	FIXME("just using first face for now\n");
    }

    it = plf->lfItalic ? 1 : 0;
    bd = plf->lfWeight > 550 ? 1 : 0;

    for(face = family->FirstFace; face; face = face->next) {
      if(!(face->Italic ^ it) && !(face->Bold ^ bd))
	break;
    }
    if(!face) face = family->FirstFace;

    TRACE("Choosen %s %s\n", debugstr_w(family->FamilyName),
	  debugstr_w(face->StyleName));

    ret->ft_face = OpenFontFile(face->file, plf->lfHeight);

    GDI_ReleaseObj(hfont);
    TRACE("returning %p\n", ret);
    return ret;
}

/*************************************************************
 * WineEngAddRefFont
 *
 */
DWORD WineEngAddRefFont(GdiFont font)
{
    return ++font->ref;
}

/*************************************************************
 * WineEngDecRefFont
 *
 */
DWORD WineEngDecRefFont(GdiFont font)
{
    DWORD ret = --font->ref;

    if(ret == 0) {
        FT_Done_Face(font->ft_face);
	HeapFree(GetProcessHeap(), 0, font);
    }
    return ret;
}

static void GetEnumStructs(Face *face, LPENUMLOGFONTEXW pelf,
			   LPNEWTEXTMETRICEXW pntm, LPDWORD ptype)
{
    OUTLINETEXTMETRICW *potm;
    UINT size;
    GdiFont font = HeapAlloc(GetProcessHeap(),0,sizeof(*font));

    font->ref = 1;
    font->ft_face = OpenFontFile(face->file, 100);

    memset(&pelf->elfLogFont, 0, sizeof(LOGFONTW));

    size = WineEngGetOutlineTextMetrics(font, 0, NULL);
    potm = HeapAlloc(GetProcessHeap(), 0, size);
    WineEngGetOutlineTextMetrics(font, size, potm);

#define TM potm->otmTextMetrics

    pntm->ntmTm.tmHeight = pelf->elfLogFont.lfHeight = TM.tmHeight;
    pntm->ntmTm.tmAscent = TM.tmAscent;
    pntm->ntmTm.tmDescent = TM.tmDescent;
    pntm->ntmTm.tmInternalLeading = TM.tmInternalLeading;
    pntm->ntmTm.tmExternalLeading = TM.tmExternalLeading;
    pntm->ntmTm.tmAveCharWidth = pelf->elfLogFont.lfWeight = TM.tmAveCharWidth;
    pntm->ntmTm.tmMaxCharWidth = TM.tmMaxCharWidth;
    pntm->ntmTm.tmWeight = pelf->elfLogFont.lfWeight = TM.tmWeight;
    pntm->ntmTm.tmOverhang = TM.tmOverhang;
    pntm->ntmTm.tmDigitizedAspectX = TM.tmDigitizedAspectX;
    pntm->ntmTm.tmDigitizedAspectY = TM.tmDigitizedAspectY;
    pntm->ntmTm.tmFirstChar = TM.tmFirstChar;
    pntm->ntmTm.tmLastChar = TM.tmLastChar;
    pntm->ntmTm.tmDefaultChar = TM.tmDefaultChar;
    pntm->ntmTm.tmBreakChar = TM.tmBreakChar;
    pntm->ntmTm.tmItalic = pelf->elfLogFont.lfItalic = TM.tmItalic;
    pntm->ntmTm.tmUnderlined = pelf->elfLogFont.lfUnderline = TM.tmUnderlined;
    pntm->ntmTm.tmStruckOut = pelf->elfLogFont.lfStrikeOut = TM.tmStruckOut;
    pntm->ntmTm.tmPitchAndFamily = TM.tmPitchAndFamily;
    pelf->elfLogFont.lfPitchAndFamily = (TM.tmPitchAndFamily & 0xf1) + 1;
    pntm->ntmTm.tmCharSet = pelf->elfLogFont.lfCharSet = TM.tmCharSet;

    pntm->ntmTm.ntmFlags = TM.tmItalic ? NTM_ITALIC : 0;
    if(TM.tmWeight > 550) pntm->ntmTm.ntmFlags |= NTM_BOLD;
    if(pntm->ntmTm.ntmFlags == 0) pntm->ntmTm.ntmFlags = NTM_REGULAR;

    pntm->ntmTm.ntmSizeEM = potm->otmEMSquare;
    pntm->ntmTm.ntmCellHeight = 0;
    pntm->ntmTm.ntmAvgWidth = 0;

    *ptype = TM.tmPitchAndFamily & TMPF_TRUETYPE ? TRUETYPE_FONTTYPE : 0;
    if(!(TM.tmPitchAndFamily & TMPF_VECTOR))
        *ptype |= RASTER_FONTTYPE;

#undef TM
    memset(&pntm->ntmFontSig, 0, sizeof(FONTSIGNATURE));

    strncpyW(pelf->elfLogFont.lfFaceName,
	     (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpFamilyName),
	     LF_FACESIZE);
    strncpyW(pelf->elfFullName,
	     (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpFullName),
	     LF_FULLFACESIZE);
    strncpyW(pelf->elfStyle,
	     (WCHAR*)((char*)potm + (ptrdiff_t)potm->otmpStyleName),
	     LF_FACESIZE);
    pelf->elfScript[0] = '\0'; /* FIXME */

    HeapFree(GetProcessHeap(), 0, potm);
    WineEngDecRefFont(font);
    return;
}

/*************************************************************
 * WineEngEnumFonts
 *
 */
DWORD WineEngEnumFonts(LPLOGFONTW plf, DEVICEFONTENUMPROC proc,
		       LPARAM lparam)
{
    Family *family;
    Face *face;
    ENUMLOGFONTEXW elf;
    NEWTEXTMETRICEXW ntm;
    DWORD type, ret = 1;

    TRACE("facename = %s\n", debugstr_w(plf->lfFaceName));
    if(plf->lfFaceName[0]) {
        for(family = FontList; family; family = family->next) {
	    if(!strcmpiW(plf->lfFaceName, family->FamilyName)) {
	        for(face = family->FirstFace; face; face = face->next) {
		    GetEnumStructs(face, &elf, &ntm, &type);
		    TRACE("enuming '%s'\n",
			  debugstr_w(elf.elfLogFont.lfFaceName));
		    ret = proc(&elf, &ntm, type, lparam);
		    if(!ret) break;
		}
	    }
	}
    } else {
        for(family = FontList; family; family = family->next) {
	    GetEnumStructs(family->FirstFace, &elf, &ntm, &type);
	    TRACE("enuming '%s'\n", debugstr_w(elf.elfLogFont.lfFaceName));
	    ret = proc(&elf, &ntm, type, lparam);
	    if(!ret) break;
	}
    }

    return ret;
}

/*************************************************************
 * WineEngGetGlyphOutline
 *
 * Behaves in exactly the same way as the win32 api GetGlyphOutline
 * except that the first parameter is the HWINEENGFONT of the font in
 * question rather than an HDC.
 *
 */
DWORD WineEngGetGlyphOutline(GdiFont font, UINT glyph, UINT format,
			     LPGLYPHMETRICS lpgm, DWORD buflen, LPVOID buf,
			     const MAT2* lpmat)
{
    FT_Face ft_face = font->ft_face;
    FT_UInt glyph_index;
    DWORD width, height, pitch, needed;
    FT_Bitmap ft_bitmap;

    TRACE("%p, %04x, %08x, %p, %08lx, %p, %p\n", font, glyph, format, lpgm,
	  buflen, buf, lpmat);

    if(format & GGO_GLYPH_INDEX)
        glyph_index = glyph;
    else
        glyph_index = FT_Get_Char_Index(ft_face, glyph);

    FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);

    lpgm->gmBlackBoxX = ft_face->glyph->metrics.width >> 6;
    lpgm->gmBlackBoxY = ft_face->glyph->metrics.height >> 6;
    lpgm->gmptGlyphOrigin.x = ft_face->glyph->metrics.horiBearingX >> 6;
    lpgm->gmptGlyphOrigin.y = ft_face->glyph->metrics.horiBearingY >> 6;
    lpgm->gmCellIncX = ft_face->glyph->metrics.horiAdvance >> 6;
    lpgm->gmCellIncY = 0;

    if(format == GGO_METRICS)
        return TRUE;
    
    if(ft_face->glyph->format != ft_glyph_format_outline) {
        FIXME("loaded a bitmap\n");
	return GDI_ERROR;
    }

    if(format == GGO_BITMAP) {
        width = lpgm->gmBlackBoxX;
	height = lpgm->gmBlackBoxY;
	pitch = (width + 31) / 32 * 4;
        needed = pitch * height;

	if(!buf || !buflen) return needed;
	ft_bitmap.width = width;
	ft_bitmap.rows = height;
	ft_bitmap.pitch = pitch;
	ft_bitmap.pixel_mode = ft_pixel_mode_mono;
	ft_bitmap.buffer = buf;

	FT_Outline_Translate(&ft_face->glyph->outline,
			     - ft_face->glyph->metrics.horiBearingX,
			     - (ft_face->glyph->metrics.horiBearingY -
				ft_face->glyph->metrics.height) );

	FT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);
    } else {
        FIXME("Unsupported format %d\n", format);
	return GDI_ERROR;
    }
    return TRUE;
}

/*************************************************************
 * WineEngGetTextMetrics
 *
 */
BOOL WineEngGetTextMetrics(GdiFont font, LPTEXTMETRICW ptm)
{
    FT_Face ft_face = font->ft_face;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    FT_Fixed x_scale, y_scale;
    
    x_scale = ft_face->size->metrics.x_scale;
    y_scale = ft_face->size->metrics.y_scale;

    pOS2 = FT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
    if(!pOS2) {
      FIXME("Can't find OS/2 table - not TT font?\n");
      return 0;
    }

    pHori = FT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea);
    if(!pHori) {
      FIXME("Can't find HHEA table - not TT font?\n");
      return 0;
    }

    TRACE("OS/2 winA = %d winD = %d typoA = %d typoD = %d typoLG = %d FT_Face a = %d, d = %d, h = %d: HORZ a = %d, d = %d lg = %d maxY = %ld minY = %ld\n",
	  pOS2->usWinAscent, pOS2->usWinDescent,
	  pOS2->sTypoAscender, pOS2->sTypoDescender, pOS2->sTypoLineGap,
	  ft_face->ascender, ft_face->descender, ft_face->height,
	  pHori->Ascender, pHori->Descender, pHori->Line_Gap,
	  ft_face->bbox.yMax, ft_face->bbox.yMin);
    
    ptm->tmAscent = (FT_MulFix(pOS2->usWinAscent, y_scale) + 32) >> 6;
    ptm->tmDescent = (FT_MulFix(pOS2->usWinDescent, y_scale) + 32) >> 6;
    ptm->tmHeight = ptm->tmAscent + ptm->tmDescent;
    ptm->tmInternalLeading = (FT_MulFix(pOS2->usWinAscent + pOS2->usWinDescent
			       - ft_face->units_per_EM, y_scale) + 32) >> 6;

    /* MSDN says:
     el = MAX(0, LineGap - ((WinAscent + WinDescent) - (Ascender - Descender)))
    */
    ptm->tmExternalLeading = max(0, (FT_MulFix(pHori->Line_Gap -
       		 ((pOS2->usWinAscent + pOS2->usWinDescent) -
		  (pHori->Ascender - pHori->Descender)), y_scale) + 32) >> 6);

    ptm->tmAveCharWidth = (FT_MulFix(pOS2->xAvgCharWidth, x_scale) + 32) >> 6;
    ptm->tmMaxCharWidth = (FT_MulFix(ft_face->bbox.xMax - ft_face->bbox.xMin, x_scale) + 32) >> 6;
    ptm->tmWeight = pOS2->usWeightClass;
    ptm->tmOverhang = 0;
    ptm->tmDigitizedAspectX = 300;
    ptm->tmDigitizedAspectY = 300;
    ptm->tmFirstChar = pOS2->usFirstCharIndex;
    ptm->tmLastChar = pOS2->usLastCharIndex;
    ptm->tmDefaultChar = pOS2->usDefaultChar;
    ptm->tmBreakChar = pOS2->usBreakChar;
    ptm->tmItalic = (ft_face->style_flags & FT_STYLE_FLAG_ITALIC) ? 1 : 0;
    ptm->tmUnderlined = 0; /* entry in OS2 table */
    ptm->tmStruckOut = 0; /* entry in OS2 table */

    /* Yes this is correct; braindead api */
    ptm->tmPitchAndFamily = FT_IS_FIXED_WIDTH(ft_face) ? 0 : TMPF_FIXED_PITCH;
    if(FT_IS_SCALABLE(ft_face))
        ptm->tmPitchAndFamily |= TMPF_VECTOR;
    if(FT_IS_SFNT(ft_face))
        ptm->tmPitchAndFamily |= TMPF_TRUETYPE;

    ptm->tmCharSet = ANSI_CHARSET;
    return TRUE;
}
/*************************************************************
 * WineEngGetOutlineTextMetrics
 *
 */
UINT WineEngGetOutlineTextMetrics(GdiFont font, UINT cbSize,
				  OUTLINETEXTMETRICW *potm)
{
    FT_Face ft_face = font->ft_face;
    UINT needed, lenfam, lensty, ret;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    FT_Fixed x_scale, y_scale;
    WCHAR *family_nameW, *style_nameW;
    WCHAR spaceW[] = {' ', '\0'};
    char *cp;

    needed = sizeof(*potm);
    
    lenfam = MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1, NULL, 0)
      * sizeof(WCHAR);
    family_nameW = HeapAlloc(GetProcessHeap(), 0, lenfam);
    MultiByteToWideChar(CP_ACP, 0, ft_face->family_name, -1,
			family_nameW, lenfam);

    lensty = MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1, NULL, 0)
      * sizeof(WCHAR);
    style_nameW = HeapAlloc(GetProcessHeap(), 0, lensty);
    MultiByteToWideChar(CP_ACP, 0, ft_face->style_name, -1,
			style_nameW, lensty);

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

    if(needed > cbSize) {
        ret = needed;
	goto end;
    }

    x_scale = ft_face->size->metrics.x_scale;
    y_scale = ft_face->size->metrics.y_scale;

    pOS2 = FT_Get_Sfnt_Table(ft_face, ft_sfnt_os2);
    if(!pOS2) {
        FIXME("Can't find OS/2 table - not TT font?\n");
	ret = 0;
	goto end;
    }

    pHori = FT_Get_Sfnt_Table(ft_face, ft_sfnt_hhea);
    if(!pHori) {
        FIXME("Can't find HHEA table - not TT font?\n");
	ret = 0;
	goto end;
    }

    potm->otmSize = needed;

    WineEngGetTextMetrics(font, &potm->otmTextMetrics);
    
    potm->otmFiller = 0;
    memcpy(&potm->otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
    potm->otmfsSelection = pOS2->fsSelection;
    potm->otmfsType = pOS2->fsType;
    potm->otmsCharSlopeRise = pHori->caret_Slope_Rise;
    potm->otmsCharSlopeRun = pHori->caret_Slope_Run;
    potm->otmItalicAngle = 0; /* POST table */
    potm->otmEMSquare = ft_face->units_per_EM;
    potm->otmAscent = pOS2->sTypoAscender;
    potm->otmDescent = pOS2->sTypoDescender;
    potm->otmLineGap = pOS2->sTypoLineGap;
    potm->otmsCapEmHeight = pOS2->sCapHeight;
    potm->otmsXHeight = pOS2->sxHeight;
    potm->otmrcFontBox.left = ft_face->bbox.xMin;
    potm->otmrcFontBox.right = ft_face->bbox.xMax;
    potm->otmrcFontBox.top = ft_face->bbox.yMin;
    potm->otmrcFontBox.bottom = ft_face->bbox.yMax;
    potm->otmMacAscent = 0; /* where do these come from ? */
    potm->otmMacDescent = 0;
    potm->otmMacLineGap = 0;
    potm->otmusMinimumPPEM = 0; /* TT Header */
    potm->otmptSubscriptSize.x = pOS2->ySubscriptXSize;
    potm->otmptSubscriptSize.y = pOS2->ySubscriptYSize;
    potm->otmptSubscriptOffset.x = pOS2->ySubscriptXOffset;
    potm->otmptSubscriptOffset.y = pOS2->ySubscriptYOffset;
    potm->otmptSuperscriptSize.x = pOS2->ySuperscriptXSize;
    potm->otmptSuperscriptSize.y = pOS2->ySuperscriptYSize;
    potm->otmptSuperscriptOffset.x = pOS2->ySuperscriptXOffset;
    potm->otmptSuperscriptOffset.y = pOS2->ySuperscriptYOffset;
    potm->otmsStrikeoutSize = pOS2->yStrikeoutSize;
    potm->otmsStrikeoutPosition = pOS2->yStrikeoutPosition;
    potm->otmsUnderscoreSize = 0; /* POST Header */
    potm->otmsUnderscorePosition = 0; /* POST Header */
    
    /* otmp* members should clearly have type ptrdiff_t, but M$ knows best */
    cp = (char*)potm + sizeof(*potm);
    potm->otmpFamilyName = (LPSTR)(cp - (char*)potm);
    strcpyW((WCHAR*)cp, family_nameW);
    cp += lenfam;
    potm->otmpStyleName = (LPSTR)(cp - (char*)potm);
    strcpyW((WCHAR*)cp, style_nameW);
    cp += lensty;
    potm->otmpFaceName = (LPSTR)(cp - (char*)potm);
    strcpyW((WCHAR*)cp, family_nameW);
    if(strcasecmp(ft_face->style_name, "regular")) {
        strcatW((WCHAR*)cp, spaceW);
	strcatW((WCHAR*)cp, style_nameW);
	cp += lenfam + lensty;
    } else 
        cp += lenfam;
    potm->otmpFullName = (LPSTR)(cp - (char*)potm);
    strcpyW((WCHAR*)cp, family_nameW);
    strcatW((WCHAR*)cp, spaceW);
    strcatW((WCHAR*)cp, style_nameW);
    ret = needed;

 end:
    HeapFree(GetProcessHeap(), 0, style_nameW);
    HeapFree(GetProcessHeap(), 0, family_nameW);

    return ret;
}


/*************************************************************
 * WineEngGetCharWidth
 *
 */
BOOL WineEngGetCharWidth(GdiFont font, UINT firstChar, UINT lastChar,
			 LPINT buffer)
{
    UINT c;
    GLYPHMETRICS gm;
    TRACE("%p, %d, %d, %p\n", font, firstChar, lastChar, buffer);

    for(c = firstChar; c <= lastChar; c++) {
        WineEngGetGlyphOutline(font, c, GGO_METRICS, &gm, 0, NULL, NULL);
	buffer[c - firstChar] = gm.gmCellIncX;
    }
    return TRUE;
}

/*************************************************************
 * WineEngGetTextExtentPoint
 *
 */
BOOL WineEngGetTextExtentPoint(GdiFont font, LPCWSTR wstr, INT count,
			       LPSIZE size)
{
    UINT idx;
    GLYPHMETRICS gm;
    TEXTMETRICW tm;

    TRACE("%p, %s, %d, %p\n", font, debugstr_wn(wstr, count), count,
	  size);

    size->cx = 0;
    WineEngGetTextMetrics(font, &tm);
    size->cy = tm.tmHeight;
 
   for(idx = 0; idx < count; idx++) {
        WineEngGetGlyphOutline(font, wstr[idx], GGO_METRICS, &gm, 0, NULL,
			       NULL);
	size->cx += gm.gmCellIncX;
    }
    TRACE("return %ld,%ld\n", size->cx, size->cy);
    return TRUE;
}

#else /* HAVE_FREETYPE */

BOOL WineEngInit(void)
{
    return FALSE;
}
GdiFont WineEngCreateFontInstance(HFONT hfont)
{
    return NULL;
}
DWORD WineEngAddRefFont(GdiFont font)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}
DWORD WineEngDecRefFont(GdiFont font)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}

DWORD WineEngEnumFonts(LPLOGFONTW plf, DEVICEFONTENUMPROC proc, LPARAM lparam)
{
    return 1;
}

DWORD WineEngGetGlyphOutline(GdiFont font, UINT glyph, UINT format,
			     LPGLYPHMETRICS lpgm, DWORD buflen, LPVOID buf,
			     const MAT2* lpmat)
{
    ERR("called but we don't have FreeType\n");
    return GDI_ERROR;
}

BOOL WineEngGetTextMetrics(GdiFont font, LPTEXTMETRICW ptm)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

UINT WineEngGetOutlineTextMetrics(GdiFont font, UINT cbSize,
				  OUTLINETEXTMETRICW *potm)
{
    ERR("called but we don't have FreeType\n");
    return 0;
}

BOOL WineEngGetCharWidth(GdiFont font, UINT firstChar, UINT lastChar,
			 LPINT buffer)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

BOOL WineEngGetTextExtentPoint(GdiFont font, LPCWSTR wstr, INT count,
			       LPSIZE size)
{
    ERR("called but we don't have FreeType\n");
    return FALSE;
}

#endif /* HAVE_FREETYPE */

