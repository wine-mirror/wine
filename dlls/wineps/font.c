/*
 *	PostScript driver font functions
 *
 *	Copyright 1998  Huw D M Davies
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
#include <string.h>
#include <stdlib.h> 	    /* for bsearch() */
#include "winspool.h"
#include "gdi.h"
#include "psdrv.h"
#include "wine/debug.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           is_stock_font
 */
inline static BOOL is_stock_font( HFONT font )
{
    int i;
    for (i = OEM_FIXED_FONT; i <= DEFAULT_GUI_FONT; i++)
    {
        if (i != DEFAULT_PALETTE && font == GetStockObject(i)) return TRUE;
    }
    return FALSE;
}


/*******************************************************************************
 *  ScaleFont
 *
 *  Scale font to requested lfHeight
 *
 */
inline static float round(float f)
{
    return (f > 0) ? (f + 0.5) : (f - 0.5);
}

static VOID ScaleFont(const AFM *afm, LONG lfHeight, PSFONT *font,
    	TEXTMETRICW *tm)
{
    const WINMETRICS	*wm = &(afm->WinMetrics);
    USHORT  	    	usUnitsPerEm, usWinAscent, usWinDescent;
    SHORT   	    	sAscender, sDescender, sLineGap, sTypoAscender;
    SHORT    	    	sTypoDescender, sTypoLineGap, sAvgCharWidth;
    
    TRACE("'%s' %li\n", afm->FontName, lfHeight);
		
    if (lfHeight < 0)   	    	    	    	/* match em height */
    {
        font->scale = - ((float)lfHeight / (float)(wm->usUnitsPerEm));
    }
    else    	    	    	    	    	    	/* match cell height */
    {
    	font->scale = (float)lfHeight /
	    	(float)(wm->usWinAscent + wm->usWinDescent);
    }
    
    font->size = (INT)round(font->scale * (float)wm->usUnitsPerEm);
    font->set = FALSE;
    
    usUnitsPerEm = (USHORT)round((float)(wm->usUnitsPerEm) * font->scale);
    sAscender = (SHORT)round((float)(wm->sAscender) * font->scale);
    sDescender = (SHORT)round((float)(wm->sDescender) * font->scale);
    sLineGap = (SHORT)round((float)(wm->sLineGap) * font->scale);
    sTypoAscender = (SHORT)round((float)(wm->sTypoAscender) * font->scale);
    sTypoDescender = (SHORT)round((float)(wm->sTypoDescender) * font->scale);
    sTypoLineGap = (SHORT)round((float)(wm->sTypoLineGap) * font->scale);
    usWinAscent = (USHORT)round((float)(wm->usWinAscent) * font->scale);
    usWinDescent = (USHORT)round((float)(wm->usWinDescent) * font->scale);
    sAvgCharWidth = (SHORT)round((float)(wm->sAvgCharWidth) * font->scale);
    
    tm->tmAscent = (LONG)usWinAscent;
    tm->tmDescent = (LONG)usWinDescent;
    tm->tmHeight = tm->tmAscent + tm->tmDescent;
    
    tm->tmInternalLeading = tm->tmHeight - (LONG)usUnitsPerEm;
    if (tm->tmInternalLeading < 0)
        tm->tmInternalLeading = 0;
	
    tm->tmExternalLeading =
    	    (LONG)(sAscender - sDescender + sLineGap) - tm->tmHeight;
    if (tm->tmExternalLeading < 0)
    	tm->tmExternalLeading = 0;
    
    tm->tmAveCharWidth = (LONG)sAvgCharWidth;
	
    tm->tmWeight = afm->Weight;
    tm->tmItalic = (afm->ItalicAngle != 0.0);
    tm->tmUnderlined = 0;
    tm->tmStruckOut = 0;
    tm->tmFirstChar = (WCHAR)(afm->Metrics[0].UV);
    tm->tmLastChar = (WCHAR)(afm->Metrics[afm->NumofMetrics - 1].UV);
    tm->tmDefaultChar = 0x001f;     	/* Win2K does this - FIXME? */
    tm->tmBreakChar = tm->tmFirstChar;	    	/* should be 'space' */
    
    tm->tmPitchAndFamily = TMPF_DEVICE | TMPF_VECTOR;
    if (!afm->IsFixedPitch)
    	tm->tmPitchAndFamily |= TMPF_FIXED_PITCH;   /* yes, it's backwards */
    if (wm->usUnitsPerEm != 1000)
    	tm->tmPitchAndFamily |= TMPF_TRUETYPE;
    
    tm->tmCharSet = ANSI_CHARSET;   	/* FIXME */
    tm->tmOverhang = 0;
    
    /*
     *	This is kludgy.  font->scale is used in several places in the driver
     *	to adjust PostScript-style metrics.  Since these metrics have been
     *	"normalized" to an em-square size of 1000, font->scale needs to be
     *	similarly adjusted..
     */
     
    font->scale *= (float)wm->usUnitsPerEm / 1000.0;
     
    tm->tmMaxCharWidth = (LONG)round(
    	    (afm->FontBBox.urx - afm->FontBBox.llx) * font->scale);
    
    TRACE("Selected PS font '%s' size %d weight %ld.\n", afm->FontName,
    	    font->size, tm->tmWeight );
    TRACE("H = %ld As = %ld Des = %ld IL = %ld EL = %ld\n", tm->tmHeight,
    	    tm->tmAscent, tm->tmDescent, tm->tmInternalLeading,
    	    tm->tmExternalLeading);
}

/***********************************************************************
 *           SelectFont   (WINEPS.@)
 */
HFONT PSDRV_SelectFont( PSDRV_PDEVICE *physDev, HFONT hfont )
{
    LOGFONTW lf;
    BOOL bd = FALSE, it = FALSE;
    AFMLISTENTRY *afmle;
    FONTFAMILY *family;
    char FaceName[LF_FACESIZE];

    if (!GetObjectW( hfont, sizeof(lf), &lf )) return 0;

    TRACE("FaceName = %s Height = %ld Italic = %d Weight = %ld\n",
	  debugstr_w(lf.lfFaceName), lf.lfHeight, lf.lfItalic,
	  lf.lfWeight);

    if(lf.lfItalic)
        it = TRUE;
    if(lf.lfWeight > 550)
        bd = TRUE;
    WideCharToMultiByte(CP_ACP, 0, lf.lfFaceName, -1,
			FaceName, sizeof(FaceName), NULL, NULL);
    
    if(FaceName[0] == '\0') {
        switch(lf.lfPitchAndFamily & 0xf0) {
	case FF_DONTCARE:
	    break;
	case FF_ROMAN:
	case FF_SCRIPT:
	    strcpy(FaceName, "Times");
	    break;
	case FF_SWISS:
	    strcpy(FaceName, "Helvetica");
	    break;
	case FF_MODERN:
	    strcpy(FaceName, "Courier");
	    break;
	case FF_DECORATIVE:
	    strcpy(FaceName, "Symbol");
	    break;
	}
    }

    if(FaceName[0] == '\0') {
        switch(lf.lfPitchAndFamily & 0x0f) {
	case VARIABLE_PITCH:
	    strcpy(FaceName, "Times");
	    break;
	default:
	    strcpy(FaceName, "Courier");
	    break;
	}
    }

    if (physDev->pi->FontSubTableSize != 0)
    {
	DWORD i;

	for (i = 0; i < physDev->pi->FontSubTableSize; ++i)
	{
	    if (!strcasecmp (FaceName,
		    physDev->pi->FontSubTable[i].pValueName))
	    {
		TRACE ("substituting facename '%s' for '%s'\n",
			(LPSTR) physDev->pi->FontSubTable[i].pData, FaceName);
		if (strlen ((LPSTR) physDev->pi->FontSubTable[i].pData) <
			LF_FACESIZE)
		    strcpy (FaceName,
			    (LPSTR) physDev->pi->FontSubTable[i].pData);
		else
		    WARN ("Facename '%s' is too long; ignoring substitution\n",
			    (LPSTR) physDev->pi->FontSubTable[i].pData);
		break;
	    }
	}
    }

    TRACE("Trying to find facename '%s'\n", FaceName);

    /* Look for a matching font family */
    for(family = physDev->pi->Fonts; family; family = family->next) {
        if(!strcasecmp(FaceName, family->FamilyName))
	    break;
    }
    if(!family) {
	/* Fallback for Window's font families to common PostScript families */
	if(!strcmp(FaceName, "Arial"))
	    strcpy(FaceName, "Helvetica");
	else if(!strcmp(FaceName, "System"))
	    strcpy(FaceName, "Helvetica");
	else if(!strcmp(FaceName, "Times New Roman"))
	    strcpy(FaceName, "Times");
	else if(!strcmp(FaceName, "Courier New"))
	    strcpy(FaceName, "Courier");

	for(family = physDev->pi->Fonts; family; family = family->next) {
	    if(!strcmp(FaceName, family->FamilyName))
		break;
	}
    }
    /* If all else fails, use the first font defined for the printer */
    if(!family)
        family = physDev->pi->Fonts;

    TRACE("Got family '%s'\n", family->FamilyName);

    for(afmle = family->afmlist; afmle; afmle = afmle->next) {
        if( (bd == (afmle->afm->Weight == FW_BOLD)) && 
	    (it == (afmle->afm->ItalicAngle != 0.0)) )
	        break;
    }
    if(!afmle)
        afmle = family->afmlist; /* not ideal */
	
    TRACE("Got font '%s'\n", afmle->afm->FontName);
    
    physDev->font.afm = afmle->afm;
    /* stock fonts ignore the mapping mode */
    if (!is_stock_font( hfont )) lf.lfHeight = INTERNAL_YWSTODS(physDev->dc, lf.lfHeight);
    ScaleFont(physDev->font.afm, lf.lfHeight,
    	    &(physDev->font), &(physDev->font.tm));
    
    physDev->font.escapement = lf.lfEscapement;
    
    /* Does anyone know if these are supposed to be reversed like this? */
    
    physDev->font.tm.tmDigitizedAspectX = physDev->logPixelsY;
    physDev->font.tm.tmDigitizedAspectY = physDev->logPixelsX;
    
    return TRUE; /* We'll use a device font for now */
}

/***********************************************************************
 *           PSDRV_GetTextMetrics
 */
BOOL PSDRV_GetTextMetrics(PSDRV_PDEVICE *physDev, TEXTMETRICW *metrics)
{
    memcpy(metrics, &(physDev->font.tm), sizeof(physDev->font.tm));
    return TRUE;
}

/******************************************************************************
 *  	PSDRV_UVMetrics
 *
 *  Find the AFMMETRICS for a given UV.  Returns first glyph in the font
 *  (space?) if the font does not have a glyph for the given UV.
 */
static int MetricsByUV(const void *a, const void *b)
{
    return (int)(((const AFMMETRICS *)a)->UV - ((const AFMMETRICS *)b)->UV);
}
 
const AFMMETRICS *PSDRV_UVMetrics(LONG UV, const AFM *afm)
{
    AFMMETRICS	    	key;
    const AFMMETRICS	*needle;
    
    /*
     *	Ugly work-around for symbol fonts.  Wine is sending characters which
     *	belong in the Unicode private use range (U+F020 - U+F0FF) as ASCII
     *	characters (U+0020 - U+00FF).
     */
    
    if ((afm->Metrics->UV & 0xff00) == 0xf000 && UV < 0x100)
    	UV |= 0xf000;
    
    key.UV = UV;
    
    needle = bsearch(&key, afm->Metrics, afm->NumofMetrics, sizeof(AFMMETRICS),
	    MetricsByUV);

    if (needle == NULL)
    {
    	WARN("No glyph for U+%.4lX in %s\n", UV, afm->FontName);
    	needle = afm->Metrics;
    }
	
    return needle;
}

/***********************************************************************
 *           PSDRV_GetTextExtentPoint
 */
BOOL PSDRV_GetTextExtentPoint(PSDRV_PDEVICE *physDev, LPCWSTR str, INT count, LPSIZE size)
{
    DC *dc = physDev->dc;
    int     	    i;
    float   	    width = 0.0;
    
    TRACE("%s %i\n", debugstr_wn(str, count), count);
    
    for (i = 0; i < count && str[i] != '\0'; ++i)
	width += PSDRV_UVMetrics(str[i], physDev->font.afm)->WX;
	
    width *= physDev->font.scale;
    
    size->cx = GDI_ROUND((FLOAT)width * dc->xformVport2World.eM11);
    size->cy = GDI_ROUND((FLOAT)physDev->font.tm.tmHeight *
    	    dc->xformVport2World.eM22);
	    
    TRACE("cx=%li cy=%li\n", size->cx, size->cy);
	    
    return TRUE;
}

/***********************************************************************
 *           PSDRV_GetCharWidth
 */
BOOL PSDRV_GetCharWidth(PSDRV_PDEVICE *physDev, UINT firstChar, UINT lastChar, LPINT buffer)
{
    UINT    	    i;
    
    TRACE("U+%.4X U+%.4X\n", firstChar, lastChar);
    
    if (lastChar > 0xffff || firstChar > lastChar)
    {
    	SetLastError(ERROR_INVALID_PARAMETER);
    	return FALSE;
    }
	
    for (i = firstChar; i <= lastChar; ++i)
    {
    	*buffer = GDI_ROUND(PSDRV_UVMetrics(i, physDev->font.afm)->WX
	    	* physDev->font.scale);
	TRACE("U+%.4X: %i\n", i, *buffer);
	++buffer;
    }
	
    return TRUE;
}
    
/***********************************************************************
 *           PSDRV_SetFont
 */
BOOL PSDRV_SetFont( PSDRV_PDEVICE *physDev )
{
    PSDRV_WriteSetColor(physDev, &physDev->font.color);
    if(physDev->font.set) return TRUE;

    PSDRV_WriteSetFont(physDev);
    physDev->font.set = TRUE;
    return TRUE;
}


/***********************************************************************
 *           PSDRV_GetFontMetric
 */
static UINT PSDRV_GetFontMetric( PSDRV_PDEVICE *physDev, const AFM *afm,
                                 NEWTEXTMETRICEXW *ntmx, ENUMLOGFONTEXW *elfx)
{
    /* ntmx->ntmTm is NEWTEXTMETRICW; compatible w/ TEXTMETRICW per Win32 doc */

    TEXTMETRICW     *tm = (TEXTMETRICW *)&(ntmx->ntmTm);
    LOGFONTW	    *lf = &(elfx->elfLogFont);
    PSFONT  	    font;
    
    memset(ntmx, 0, sizeof(*ntmx));
    memset(elfx, 0, sizeof(*elfx));
    
    ScaleFont(afm, -(LONG)(afm->WinMetrics.usUnitsPerEm), &font, tm);
    
    lf->lfHeight = tm->tmHeight;
    lf->lfWidth = tm->tmAveCharWidth;
    lf->lfWeight = tm->tmWeight;
    lf->lfItalic = tm->tmItalic;
    lf->lfCharSet = tm->tmCharSet;
    
    lf->lfPitchAndFamily = (afm->IsFixedPitch) ? FIXED_PITCH : VARIABLE_PITCH;
    
    MultiByteToWideChar(CP_ACP, 0, afm->FamilyName, -1, lf->lfFaceName,
    	    LF_FACESIZE);
	    
    return DEVICE_FONTTYPE;
}

/***********************************************************************
 *           PSDRV_EnumDeviceFonts
 */
BOOL PSDRV_EnumDeviceFonts( PSDRV_PDEVICE *physDev, LPLOGFONTW plf,
			    DEVICEFONTENUMPROC proc, LPARAM lp )
{
    ENUMLOGFONTEXW	lf;
    NEWTEXTMETRICEXW	tm;
    BOOL	  	b, bRet = 0;
    AFMLISTENTRY	*afmle;
    FONTFAMILY		*family;
    char                FaceName[LF_FACESIZE];

    if( plf->lfFaceName[0] ) {
        WideCharToMultiByte(CP_ACP, 0, plf->lfFaceName, -1,
			  FaceName, sizeof(FaceName), NULL, NULL);
        TRACE("lfFaceName = '%s'\n", FaceName);
        for(family = physDev->pi->Fonts; family; family = family->next) {
            if(!strncmp(FaceName, family->FamilyName, 
			strlen(family->FamilyName)))
	        break;
	}
	if(family) {
	    for(afmle = family->afmlist; afmle; afmle = afmle->next) {
	        TRACE("Got '%s'\n", afmle->afm->FontName);
		if( (b = proc( &lf, &tm, PSDRV_GetFontMetric(physDev, afmle->afm, &tm, &lf), lp )) )
		     bRet = b;
		else break;
	    }
	}
    } else {

        TRACE("lfFaceName = NULL\n");
        for(family = physDev->pi->Fonts; family; family = family->next) {
	    afmle = family->afmlist;
	    TRACE("Got '%s'\n", afmle->afm->FontName);
	    if( (b = proc( &lf, &tm, PSDRV_GetFontMetric(physDev, afmle->afm, &tm, &lf), lp )) )
	        bRet = b;
	    else break;
	}
    }
    return bRet;
}
