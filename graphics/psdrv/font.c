/*
 *	PostScript driver font functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */
#include <string.h>
#include "windows.h"
#include "print.h"
#include "psdrv.h"
#include "debug.h"



/***********************************************************************
 *           PSDRV_FONT_SelectObject
 */
HFONT16 PSDRV_FONT_SelectObject( DC * dc, HFONT16 hfont,
                                        FONTOBJ *font )
{
    HFONT16 prevfont = dc->w.hFont;
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    LOGFONT16 *lf = &(font->logfont);
    BOOL32 bd = FALSE, it = FALSE;
    AFMLISTENTRY *afmle;
    AFM *afm;
    FONTFAMILY *family;
    char FaceName[LF_FACESIZE];


    TRACE(psdrv, "FaceName = '%s' Height = %d Italic = %d Weight = %d\n",
	  lf->lfFaceName, lf->lfHeight, lf->lfItalic, lf->lfWeight);

    dc->w.hFont = hfont;

    if(lf->lfItalic)
        it = TRUE;
    if(lf->lfWeight > 550)
        bd = TRUE;
    lstrcpy32A(FaceName, lf->lfFaceName);
    
    if(FaceName[0] == '\0') {
        switch(lf->lfPitchAndFamily & 0xf0) {
	case FF_DONTCARE:
	    break;
	case FF_ROMAN:
	case FF_SCRIPT:
	    lstrcpy32A(FaceName, "Times");
	    break;
	case FF_SWISS:
	    lstrcpy32A(FaceName, "Helvetica");
	    break;
	case FF_MODERN:
	    lstrcpy32A(FaceName, "Courier");
	    break;
	case FF_DECORATIVE:
	    lstrcpy32A(FaceName, "Symbol");
	    break;
	}
    }

    if(FaceName[0] == '\0') {
        switch(lf->lfPitchAndFamily & 0x0f) {
	case VARIABLE_PITCH:
	    lstrcpy32A(FaceName, "Times");
	    break;
	default:
	    lstrcpy32A(FaceName, "Courier");
	    break;
	}
    }

    for(family = physDev->pi->Fonts; family; family = family->next) {
        if(!lstrncmp32A(FaceName, family->FamilyName, 
			                         strlen(family->FamilyName)))
	    break;
    }
    if(!family)
        family = physDev->pi->Fonts;

    
    for(afmle = family->afmlist; afmle; afmle = afmle->next) {
        if( (bd == (afmle->afm->Weight == FW_BOLD)) && 
	    (it == (afmle->afm->ItalicAngle != 0.0)) )
	        break;
    }
    if(!afmle)
        afmle = family->afmlist; /* not ideal */
    
    afm = afmle->afm;

    physDev->font.afm = afm;
    physDev->font.tm.tmHeight = YLSTODS(dc, lf->lfHeight);
    if(physDev->font.tm.tmHeight < 0) {
        physDev->font.tm.tmHeight *= - (afm->FullAscender - afm->Descender) /
				       (afm->Ascender - afm->Descender);
	TRACE(psdrv, "Fixed -ve height to %d\n", physDev->font.tm.tmHeight);
    }
    physDev->font.size = physDev->font.tm.tmHeight * 1000.0 /
				(afm->FullAscender - afm->Descender);
    physDev->font.scale = physDev->font.size / 1000.0;
    physDev->font.escapement = lf->lfEscapement;
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
    physDev->font.tm.tmUnderlined = lf->lfUnderline;
    physDev->font.tm.tmStruckOut = lf->lfStrikeOut;
    physDev->font.tm.tmFirstChar = 32;
    physDev->font.tm.tmLastChar = 251;
    physDev->font.tm.tmDefaultChar = 128;
    physDev->font.tm.tmBreakChar = 32;
    physDev->font.tm.tmPitchAndFamily = afm->IsFixedPitch ? 0 :
                                          TMPF_FIXED_PITCH;
    physDev->font.tm.tmPitchAndFamily |= TMPF_DEVICE;
    physDev->font.tm.tmCharSet = ANSI_CHARSET;
    physDev->font.tm.tmOverhang = 0;
    physDev->font.tm.tmDigitizedAspectX = dc->w.devCaps->logPixelsY;
    physDev->font.tm.tmDigitizedAspectY = dc->w.devCaps->logPixelsX;

    physDev->font.set = FALSE;

    TRACE(psdrv, "Selected PS font '%s' size %d weight %d.\n", 
	  physDev->font.afm->FontName, physDev->font.size,
	  physDev->font.tm.tmWeight );
    TRACE(psdrv, "H = %d As = %d Des = %d IL = %d EL = %d\n",
	  physDev->font.tm.tmHeight, physDev->font.tm.tmAscent,
	  physDev->font.tm.tmDescent, physDev->font.tm.tmInternalLeading,
	  physDev->font.tm.tmExternalLeading);

    return prevfont;
}

/***********************************************************************
 *           PSDRV_GetTextMetrics
 */
BOOL32 PSDRV_GetTextMetrics(DC *dc, TEXTMETRIC32A *metrics)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    memcpy(metrics, &(physDev->font.tm), sizeof(physDev->font.tm));
    return TRUE;
}


/***********************************************************************
 *           PSDRV_GetTextExtentPoint
 */
BOOL32 PSDRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                  LPSIZE32 size )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    INT32 i;
    float width;

    size->cy = YDSTOLS(dc, physDev->font.tm.tmHeight);
    width = 0.0;

    for(i = 0; i < count && str[i]; i++) {
        width += physDev->font.afm->CharWidths[ (UINT32)str[i] ];
	TRACE(psdrv, "Width after %dth char '%c' = %f\n", i, str[i], width);
    }
    width *= physDev->font.scale;
    TRACE(psdrv, "Width after scale (%f) is %f\n", physDev->font.scale, width);
    size->cx = XDSTOLS(dc, width);

    return TRUE;
}


/***********************************************************************
 *           PSDRV_SetFont
 */
BOOL32 PSDRV_SetFont( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if(physDev->font.set) return TRUE;

    PSDRV_WriteReencodeFont(dc);
    PSDRV_WriteSetFont(dc);
    physDev->font.set = TRUE;
    return TRUE;
}


/***********************************************************************
 *           PSDRV_GetFontMetric
 */
static UINT32 PSDRV_GetFontMetric(DC *dc, AFM *pafm, NEWTEXTMETRIC16 *pTM, 
              ENUMLOGFONTEX16 *pLF, INT16 size)

{
    float scale = size / (pafm->FullAscender - pafm->Descender);
    memset( pLF, 0, sizeof(*pLF) );
    memset( pTM, 0, sizeof(*pTM) );

#define plf ((LPLOGFONT16)pLF)
    plf->lfHeight    = pTM->tmHeight       = size;
    plf->lfWidth     = pTM->tmAveCharWidth = pafm->CharWidths[120] * scale;
    plf->lfWeight    = pTM->tmWeight       = pafm->Weight;
    plf->lfItalic    = pTM->tmItalic       = pafm->ItalicAngle != 0.0;
    plf->lfUnderline = pTM->tmUnderlined   = 0;
    plf->lfStrikeOut = pTM->tmStruckOut    = 0;
    plf->lfCharSet   = pTM->tmCharSet      = ANSI_CHARSET;

    /* convert pitch values */

    pTM->tmPitchAndFamily = pafm->IsFixedPitch ? 0 : TMPF_FIXED_PITCH;
    pTM->tmPitchAndFamily |= TMPF_DEVICE;
    plf->lfPitchAndFamily = 0;

    lstrcpyn32A( plf->lfFaceName, pafm->FamilyName, LF_FACESIZE );
#undef plf

    pTM->tmAscent = pafm->FullAscender * scale;
    pTM->tmDescent = -pafm->Descender * scale;
    pTM->tmInternalLeading = (pafm->FullAscender - pafm->Ascender) * scale;
    pTM->tmMaxCharWidth = pafm->CharWidths[77] * scale;
    pTM->tmDigitizedAspectX = dc->w.devCaps->logPixelsY;
    pTM->tmDigitizedAspectY = dc->w.devCaps->logPixelsX;

    *(INT32*)&pTM->tmFirstChar = 32;

    /* return font type */

    return DEVICE_FONTTYPE;

}

/***********************************************************************
 *           PSDRV_EnumDeviceFonts
 */
BOOL32 PSDRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				        DEVICEFONTENUMPROC proc, LPARAM lp )
{
    ENUMLOGFONTEX16	lf;
    NEWTEXTMETRIC16	tm;
    BOOL32	  	b, bRet = 0;
    AFMLISTENTRY	*afmle;
    FONTFAMILY		*family;
    PSDRV_PDEVICE	*physDev = (PSDRV_PDEVICE *)dc->physDev;

    if( plf->lfFaceName[0] ) {
        TRACE(psdrv, "lfFaceName = '%s'\n", plf->lfFaceName);
        for(family = physDev->pi->Fonts; family; family = family->next) {
            if(!lstrncmp32A(plf->lfFaceName, family->FamilyName, 
			strlen(family->FamilyName)))
	        break;
	}
	if(family) {
	    for(afmle = family->afmlist; afmle; afmle = afmle->next) {
	        TRACE(psdrv, "Got '%s'\n", afmle->afm->FontName);
		if( (b = (*proc)( (LPENUMLOGFONT16)&lf, &tm, 
			PSDRV_GetFontMetric( dc, afmle->afm, &tm, &lf, 200 ),
				  lp )) )
		     bRet = b;
		else break;
	    }
	}
    } else {

        TRACE(psdrv, "lfFaceName = NULL\n");
        for(family = physDev->pi->Fonts; family; family = family->next) {
	    afmle = family->afmlist;
	    TRACE(psdrv, "Got '%s'\n", afmle->afm->FontName);
	    if( (b = (*proc)( (LPENUMLOGFONT16)&lf, &tm, 
		   PSDRV_GetFontMetric( dc, afmle->afm, &tm, &lf, 200 ), 
			      lp )) )
	        bRet = b;
	    else break;
	}
    }
    return bRet;
}
