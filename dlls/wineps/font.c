/*
 *	PostScript driver font functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */
#include <string.h>
#include <stdlib.h> 	    /* for bsearch() */
#include "winspool.h"
#include "psdrv.h"
#include "debugtools.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(psdrv);


/***********************************************************************
 *           PSDRV_FONT_SelectObject
 */
HFONT PSDRV_FONT_SelectObject( DC * dc, HFONT hfont )
{
    LOGFONTW lf;
    HFONT16 prevfont = dc->hFont;
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    BOOL bd = FALSE, it = FALSE;
    AFMLISTENTRY *afmle;
    AFM *afm;
    FONTFAMILY *family;
    char FaceName[LF_FACESIZE];

    if (!GetObjectW( hfont, sizeof(lf), &lf )) return 0;

    TRACE("FaceName = %s Height = %ld Italic = %d Weight = %ld\n",
	  debugstr_w(lf.lfFaceName), lf.lfHeight, lf.lfItalic,
	  lf.lfWeight);

    dc->hFont = hfont;

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
    
    afm = afmle->afm;

    physDev->font.afm = afm;
    physDev->font.tm.tmHeight = INTERNAL_YWSTODS(dc, lf.lfHeight);
    if(physDev->font.tm.tmHeight < 0) {
        physDev->font.tm.tmHeight *= - (afm->FullAscender - afm->Descender) /
				       (afm->Ascender - afm->Descender);
	TRACE("Fixed -ve height to %ld\n", physDev->font.tm.tmHeight);
    }
    physDev->font.size = physDev->font.tm.tmHeight * 1000.0 /
				(afm->FullAscender - afm->Descender);
    physDev->font.scale = physDev->font.size / 1000.0;
    physDev->font.escapement = lf.lfEscapement;
    physDev->font.tm.tmAscent = afm->FullAscender * physDev->font.scale;
    physDev->font.tm.tmDescent = -afm->Descender * physDev->font.scale;
    physDev->font.tm.tmInternalLeading = (afm->FullAscender - afm->Ascender)
						* physDev->font.scale;
    physDev->font.tm.tmExternalLeading = (1000.0 - afm->FullAscender)
						* physDev->font.scale; /* ?? */
    physDev->font.tm.tmAveCharWidth = afm->CharWidths[120] * /* x */
                                                   physDev->font.scale;
    physDev->font.tm.tmMaxCharWidth = afm->CharWidths[77] * /* M */
                                           physDev->font.scale;
    physDev->font.tm.tmWeight = afm->Weight;
    physDev->font.tm.tmItalic = afm->ItalicAngle != 0.0;
    physDev->font.tm.tmUnderlined = lf.lfUnderline;
    physDev->font.tm.tmStruckOut = lf.lfStrikeOut;
    physDev->font.tm.tmFirstChar = 32;
    physDev->font.tm.tmLastChar = 251;
    physDev->font.tm.tmDefaultChar = 128;
    physDev->font.tm.tmBreakChar = 32;
    physDev->font.tm.tmPitchAndFamily = afm->IsFixedPitch ? 0 :
                                          TMPF_FIXED_PITCH;
    physDev->font.tm.tmPitchAndFamily |= TMPF_DEVICE;
    physDev->font.tm.tmCharSet = ANSI_CHARSET;
    physDev->font.tm.tmOverhang = 0;
    physDev->font.tm.tmDigitizedAspectX = dc->devCaps->logPixelsY;
    physDev->font.tm.tmDigitizedAspectY = dc->devCaps->logPixelsX;

    physDev->font.set = FALSE;

    TRACE("Selected PS font '%s' size %d weight %ld.\n", 
	  physDev->font.afm->FontName, physDev->font.size,
	  physDev->font.tm.tmWeight );
    TRACE("H = %ld As = %ld Des = %ld IL = %ld EL = %ld\n",
	  physDev->font.tm.tmHeight, physDev->font.tm.tmAscent,
	  physDev->font.tm.tmDescent, physDev->font.tm.tmInternalLeading,
	  physDev->font.tm.tmExternalLeading);

    return prevfont;
}

/***********************************************************************
 *           PSDRV_GetTextMetrics
 */
BOOL PSDRV_GetTextMetrics(DC *dc, TEXTMETRICW *metrics)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    memcpy(metrics, &(physDev->font.tm), sizeof(physDev->font.tm));
    return TRUE;
}

#if 0
/***********************************************************************
 *           PSDRV_UnicodeToANSI
 */
char PSDRV_UnicodeToANSI(int u)
{
    if((u & 0xff) == u)
        return u;
    switch(u) {
    case 0x2013: /* endash */
        return 0x96;
    case 0x2014: /* emdash */
        return 0x97;
    case 0x2018: /* quoteleft */
        return 0x91;
    case 0x2019: /* quoteright */
        return 0x92;
    case 0x201c: /* quotedblleft */
       return 0x93;
    case 0x201d: /* quotedblright */
        return 0x94;
    case 0x2022: /* bullet */
        return 0x95;
    default:
        WARN("Umapped unicode char U%04x\n", u);
	return 0xff;
    }
}
#endif

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
#if 0
BOOL PSDRV_GetTextExtentPoint( DC *dc, LPCWSTR str, INT count,
                                  LPSIZE size )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    INT i;
    float width;

    width = 0.0;

    for(i = 0; i < count && str[i]; i++) {
        char c = PSDRV_UnicodeToANSI(str[i]);
        width += physDev->font.afm->CharWidths[(int)(unsigned char)c];
/*	TRACE(psdrv, "Width after %dth char '%c' = %f\n", i, str[i], width);*/
    }
    width *= physDev->font.scale;
    TRACE("Width after scale (%f) is %f\n", physDev->font.scale, width);

    size->cx = GDI_ROUND((FLOAT)width * dc->xformVport2World.eM11);
    size->cy = GDI_ROUND((FLOAT)physDev->font.tm.tmHeight  * dc->xformVport2World.eM22);

    return TRUE;
}
#endif

BOOL PSDRV_GetTextExtentPoint(DC *dc, LPCWSTR str, INT count, LPSIZE size)
{
    PSDRV_PDEVICE   *physDev = (PSDRV_PDEVICE *)dc->physDev;
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
#if 0
BOOL PSDRV_GetCharWidth( DC *dc, UINT firstChar, UINT lastChar,
			   LPINT buffer )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    UINT i;

    TRACE("first = %d last = %d\n", firstChar, lastChar);

    if(lastChar > 0xff) return FALSE;
    for( i = firstChar; i <= lastChar; i++ )
        *buffer++ = physDev->font.afm->CharWidths[i] * physDev->font.scale;

    return TRUE;
}
#endif

BOOL PSDRV_GetCharWidth(DC *dc, UINT firstChar, UINT lastChar, LPINT buffer)
{
    PSDRV_PDEVICE   *physDev = (PSDRV_PDEVICE *)dc->physDev;
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
BOOL PSDRV_SetFont( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    BOOL ReEncode = FALSE;

    PSDRV_WriteSetColor(dc, &physDev->font.color);
    if(physDev->font.set) return TRUE;

    if(physDev->font.afm->EncodingScheme && 
       !strcmp(physDev->font.afm->EncodingScheme, "AdobeStandardEncoding"))
        ReEncode = TRUE;
    if(ReEncode)
        PSDRV_WriteReencodeFont(dc);
    PSDRV_WriteSetFont(dc, ReEncode);
    physDev->font.set = TRUE;
    return TRUE;
}


/***********************************************************************
 *           PSDRV_GetFontMetric
 */
static UINT PSDRV_GetFontMetric(HDC hdc, AFM *pafm, NEWTEXTMETRICEXW *pTM, 
				ENUMLOGFONTEXW *pLF, INT16 size)

{
    DC *dc = DC_GetDCPtr( hdc );
    float scale = size / (pafm->FullAscender - pafm->Descender);

    if (!dc) return FALSE;

    memset( pLF, 0, sizeof(*pLF) );
    memset( pTM, 0, sizeof(*pTM) );

#define plf ((LPLOGFONTW)pLF)
#define ptm ((LPNEWTEXTMETRICW)pTM)
    plf->lfHeight    = ptm->tmHeight       = size;
    plf->lfWidth     = ptm->tmAveCharWidth = pafm->CharWidths[120] * scale;
    plf->lfWeight    = ptm->tmWeight       = pafm->Weight;
    plf->lfItalic    = ptm->tmItalic       = pafm->ItalicAngle != 0.0;
    plf->lfUnderline = ptm->tmUnderlined   = 0;
    plf->lfStrikeOut = ptm->tmStruckOut    = 0;
    plf->lfCharSet   = ptm->tmCharSet      = ANSI_CHARSET;

    /* convert pitch values */

    ptm->tmPitchAndFamily = pafm->IsFixedPitch ? 0 : TMPF_FIXED_PITCH;
    ptm->tmPitchAndFamily |= TMPF_DEVICE;
    plf->lfPitchAndFamily = 0;

    MultiByteToWideChar(CP_ACP, 0, pafm->FamilyName, -1,
			plf->lfFaceName, LF_FACESIZE);
#undef plf

    ptm->tmAscent = pafm->FullAscender * scale;
    ptm->tmDescent = -pafm->Descender * scale;
    ptm->tmInternalLeading = (pafm->FullAscender - pafm->Ascender) * scale;
    ptm->tmMaxCharWidth = pafm->CharWidths[77] * scale;
    ptm->tmDigitizedAspectX = dc->devCaps->logPixelsY;
    ptm->tmDigitizedAspectY = dc->devCaps->logPixelsX;

    *(INT*)&ptm->tmFirstChar = 32;

    GDI_ReleaseObj( hdc );
    /* return font type */

    return DEVICE_FONTTYPE;
#undef ptm
}

/***********************************************************************
 *           PSDRV_EnumDeviceFonts
 */
BOOL PSDRV_EnumDeviceFonts( HDC hdc, LPLOGFONTW plf, 
			    DEVICEFONTENUMPROC proc, LPARAM lp )
{
    ENUMLOGFONTEXW	lf;
    NEWTEXTMETRICEXW	tm;
    BOOL	  	b, bRet = 0;
    AFMLISTENTRY	*afmle;
    FONTFAMILY		*family;
    PSDRV_PDEVICE	*physDev;
    char                FaceName[LF_FACESIZE];
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    physDev = (PSDRV_PDEVICE *)dc->physDev;
    /* FIXME!! should reevaluate dc->physDev after every callback */
    GDI_ReleaseObj( hdc );

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
		if( (b = (*proc)( &lf, &tm, 
			PSDRV_GetFontMetric( hdc, afmle->afm, &tm, &lf, 200 ),
				  lp )) )
		     bRet = b;
		else break;
	    }
	}
    } else {

        TRACE("lfFaceName = NULL\n");
        for(family = physDev->pi->Fonts; family; family = family->next) {
	    afmle = family->afmlist;
	    TRACE("Got '%s'\n", afmle->afm->FontName);
	    if( (b = (*proc)( &lf, &tm, 
		   PSDRV_GetFontMetric( hdc, afmle->afm, &tm, &lf, 200 ), 
			      lp )) )
	        bRet = b;
	    else break;
	}
    }
    return bRet;
}
