/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "winnls.h"
#include "font.h"
#include "heap.h"
#include "options.h"
#include "debugtools.h"
#include "gdi.h"

DEFAULT_DEBUG_CHANNEL(font);
DECLARE_DEBUG_CHANNEL(gdi);

#define ENUM_UNICODE	0x00000001

typedef struct
{
  LPLOGFONT16           lpLogFontParam;
  FONTENUMPROCEX16      lpEnumFunc;
  LPARAM                lpData;

  LPNEWTEXTMETRICEX16   lpTextMetric;
  LPENUMLOGFONTEX16     lpLogFont;
  SEGPTR                segTextMetric;
  SEGPTR                segLogFont;
} fontEnum16;

typedef struct
{
  LPLOGFONTW          lpLogFontParam;
  FONTENUMPROCEXW     lpEnumFunc;
  LPARAM              lpData;

  DWORD                 dwFlags;
} fontEnum32;
 
/*
 *  For TranslateCharsetInfo
 */
#define FS(x) {{0,0,0,0},{0x1<<(x),0}}
#define MAXTCIINDEX 32
static CHARSETINFO FONT_tci[MAXTCIINDEX] = {
  /* ANSI */
  { ANSI_CHARSET, 1252, FS(0)},
  { EASTEUROPE_CHARSET, 1250, FS(1)},
  { RUSSIAN_CHARSET, 1251, FS(2)},
  { GREEK_CHARSET, 1253, FS(3)},
  { TURKISH_CHARSET, 1254, FS(4)},
  { HEBREW_CHARSET, 1255, FS(5)},
  { ARABIC_CHARSET, 1256, FS(6)},
  { BALTIC_CHARSET, 1257, FS(7)},
  /* reserved by ANSI */
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  /* ANSI and OEM */
  { THAI_CHARSET,  874,  FS(16)},
  { SHIFTJIS_CHARSET, 932, FS(17)},
  { GB2312_CHARSET, 936, FS(18)},
  { HANGEUL_CHARSET, 949, FS(19)},
  { CHINESEBIG5_CHARSET, 950, FS(20)},
  { JOHAB_CHARSET, 1361, FS(21)}, 
  /* reserved for alternate ANSI and OEM */
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  /* reserved for system */
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
};

/***********************************************************************
 *              LOGFONT conversion functions.
 */
void FONT_LogFontATo16( const LOGFONTA* font32, LPLOGFONT16 font16 )
{
    font16->lfHeight = font32->lfHeight;
    font16->lfWidth = font32->lfWidth;
    font16->lfEscapement = font32->lfEscapement;
    font16->lfOrientation = font32->lfOrientation;
    font16->lfWeight = font32->lfWeight;
    font16->lfItalic = font32->lfItalic;
    font16->lfUnderline = font32->lfUnderline;
    font16->lfStrikeOut = font32->lfStrikeOut;
    font16->lfCharSet = font32->lfCharSet;
    font16->lfOutPrecision = font32->lfOutPrecision;
    font16->lfClipPrecision = font32->lfClipPrecision;
    font16->lfQuality = font32->lfQuality;
    font16->lfPitchAndFamily = font32->lfPitchAndFamily;
    lstrcpynA( font16->lfFaceName, font32->lfFaceName, LF_FACESIZE );
}

void FONT_LogFontWTo16( const LOGFONTW* font32, LPLOGFONT16 font16 )
{
    font16->lfHeight = font32->lfHeight;
    font16->lfWidth = font32->lfWidth;
    font16->lfEscapement = font32->lfEscapement;
    font16->lfOrientation = font32->lfOrientation;
    font16->lfWeight = font32->lfWeight;
    font16->lfItalic = font32->lfItalic;
    font16->lfUnderline = font32->lfUnderline;
    font16->lfStrikeOut = font32->lfStrikeOut;
    font16->lfCharSet = font32->lfCharSet;
    font16->lfOutPrecision = font32->lfOutPrecision;
    font16->lfClipPrecision = font32->lfClipPrecision;
    font16->lfQuality = font32->lfQuality;
    font16->lfPitchAndFamily = font32->lfPitchAndFamily;
    WideCharToMultiByte( CP_ACP, 0, font32->lfFaceName, -1,
                         font16->lfFaceName, LF_FACESIZE, NULL, NULL );
    font16->lfFaceName[LF_FACESIZE-1] = 0;
}

void FONT_LogFont16ToA( const LOGFONT16 *font16, LPLOGFONTA font32 )
{
    font32->lfHeight = font16->lfHeight;
    font32->lfWidth = font16->lfWidth;
    font32->lfEscapement = font16->lfEscapement;
    font32->lfOrientation = font16->lfOrientation;
    font32->lfWeight = font16->lfWeight;
    font32->lfItalic = font16->lfItalic;
    font32->lfUnderline = font16->lfUnderline;
    font32->lfStrikeOut = font16->lfStrikeOut;
    font32->lfCharSet = font16->lfCharSet;
    font32->lfOutPrecision = font16->lfOutPrecision;
    font32->lfClipPrecision = font16->lfClipPrecision;
    font32->lfQuality = font16->lfQuality;
    font32->lfPitchAndFamily = font16->lfPitchAndFamily;
    lstrcpynA( font32->lfFaceName, font16->lfFaceName, LF_FACESIZE );
}

void FONT_LogFont16ToW( const LOGFONT16 *font16, LPLOGFONTW font32 )
{
    font32->lfHeight = font16->lfHeight;
    font32->lfWidth = font16->lfWidth;
    font32->lfEscapement = font16->lfEscapement;
    font32->lfOrientation = font16->lfOrientation;
    font32->lfWeight = font16->lfWeight;
    font32->lfItalic = font16->lfItalic;
    font32->lfUnderline = font16->lfUnderline;
    font32->lfStrikeOut = font16->lfStrikeOut;
    font32->lfCharSet = font16->lfCharSet;
    font32->lfOutPrecision = font16->lfOutPrecision;
    font32->lfClipPrecision = font16->lfClipPrecision;
    font32->lfQuality = font16->lfQuality;
    font32->lfPitchAndFamily = font16->lfPitchAndFamily;
    MultiByteToWideChar( CP_ACP, 0, font16->lfFaceName, -1, font32->lfFaceName, LF_FACESIZE );
    font32->lfFaceName[LF_FACESIZE-1] = 0;
}

void FONT_LogFontAToW( const LOGFONTA *fontA, LPLOGFONTW fontW )
{
    memcpy(fontW, fontA, sizeof(LOGFONTA) - LF_FACESIZE);
    MultiByteToWideChar(CP_ACP, 0, fontA->lfFaceName, -1, fontW->lfFaceName,
			LF_FACESIZE);
}

void FONT_LogFontWToA( const LOGFONTW *fontW, LPLOGFONTA fontA )
{
    memcpy(fontA, fontW, sizeof(LOGFONTA) - LF_FACESIZE);
    WideCharToMultiByte(CP_ACP, 0, fontW->lfFaceName, -1, fontA->lfFaceName,
			LF_FACESIZE, NULL, NULL);
}

void FONT_EnumLogFontEx16ToA( const ENUMLOGFONTEX16 *font16, LPENUMLOGFONTEXA font32 )
{
    FONT_LogFont16ToA( (LPLOGFONT16)font16, (LPLOGFONTA)font32);
    lstrcpynA( font32->elfFullName, font16->elfFullName, LF_FULLFACESIZE );
    lstrcpynA( font32->elfStyle, font16->elfStyle, LF_FACESIZE );
    lstrcpynA( font32->elfScript, font16->elfScript, LF_FACESIZE );
}

void FONT_EnumLogFontEx16ToW( const ENUMLOGFONTEX16 *font16, LPENUMLOGFONTEXW font32 )
{
    FONT_LogFont16ToW( (LPLOGFONT16)font16, (LPLOGFONTW)font32);

    MultiByteToWideChar( CP_ACP, 0, font16->elfFullName, -1, font32->elfFullName, LF_FULLFACESIZE );
    font32->elfFullName[LF_FULLFACESIZE-1] = 0;
    MultiByteToWideChar( CP_ACP, 0, font16->elfStyle, -1, font32->elfStyle, LF_FACESIZE );
    font32->elfStyle[LF_FACESIZE-1] = 0;
    MultiByteToWideChar( CP_ACP, 0, font16->elfScript, -1, font32->elfScript, LF_FACESIZE );
    font32->elfScript[LF_FACESIZE-1] = 0;
}

void FONT_EnumLogFontExWTo16( const ENUMLOGFONTEXW *fontW, LPENUMLOGFONTEX16 font16 )
{
    FONT_LogFontWTo16( (LPLOGFONTW)fontW, (LPLOGFONT16)font16);

    WideCharToMultiByte( CP_ACP, 0, fontW->elfFullName, -1,
			 font16->elfFullName, LF_FULLFACESIZE, NULL, NULL );
    font16->elfFullName[LF_FULLFACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfStyle, -1,
			 font16->elfStyle, LF_FACESIZE, NULL, NULL );
    font16->elfStyle[LF_FACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfScript, -1,
			 font16->elfScript, LF_FACESIZE, NULL, NULL );
    font16->elfScript[LF_FACESIZE-1] = '\0';
}

void FONT_EnumLogFontExWToA( const ENUMLOGFONTEXW *fontW, LPENUMLOGFONTEXA fontA )
{
    FONT_LogFontWToA( (LPLOGFONTW)fontW, (LPLOGFONTA)fontA);

    WideCharToMultiByte( CP_ACP, 0, fontW->elfFullName, -1,
			 fontA->elfFullName, LF_FULLFACESIZE, NULL, NULL );
    fontA->elfFullName[LF_FULLFACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfStyle, -1,
			 fontA->elfStyle, LF_FACESIZE, NULL, NULL );
    fontA->elfStyle[LF_FACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfScript, -1,
			 fontA->elfScript, LF_FACESIZE, NULL, NULL );
    fontA->elfScript[LF_FACESIZE-1] = '\0';
}

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
void FONT_TextMetricATo16(const TEXTMETRICA *ptm32, LPTEXTMETRIC16 ptm16 )
{
    ptm16->tmHeight = ptm32->tmHeight;
    ptm16->tmAscent = ptm32->tmAscent;
    ptm16->tmDescent = ptm32->tmDescent;
    ptm16->tmInternalLeading = ptm32->tmInternalLeading;
    ptm16->tmExternalLeading = ptm32->tmExternalLeading;
    ptm16->tmAveCharWidth = ptm32->tmAveCharWidth;
    ptm16->tmMaxCharWidth = ptm32->tmMaxCharWidth;
    ptm16->tmWeight = ptm32->tmWeight;
    ptm16->tmOverhang = ptm32->tmOverhang;
    ptm16->tmDigitizedAspectX = ptm32->tmDigitizedAspectX;
    ptm16->tmDigitizedAspectY = ptm32->tmDigitizedAspectY;
    ptm16->tmFirstChar = ptm32->tmFirstChar;
    ptm16->tmLastChar = ptm32->tmLastChar;
    ptm16->tmDefaultChar = ptm32->tmDefaultChar;
    ptm16->tmBreakChar = ptm32->tmBreakChar;
    ptm16->tmItalic = ptm32->tmItalic;
    ptm16->tmUnderlined = ptm32->tmUnderlined;
    ptm16->tmStruckOut = ptm32->tmStruckOut;
    ptm16->tmPitchAndFamily = ptm32->tmPitchAndFamily;
    ptm16->tmCharSet = ptm32->tmCharSet;
}

void FONT_TextMetricWTo16(const TEXTMETRICW *ptm32, LPTEXTMETRIC16 ptm16 )
{
    ptm16->tmHeight = ptm32->tmHeight;
    ptm16->tmAscent = ptm32->tmAscent;
    ptm16->tmDescent = ptm32->tmDescent;
    ptm16->tmInternalLeading = ptm32->tmInternalLeading;
    ptm16->tmExternalLeading = ptm32->tmExternalLeading;
    ptm16->tmAveCharWidth = ptm32->tmAveCharWidth;
    ptm16->tmMaxCharWidth = ptm32->tmMaxCharWidth;
    ptm16->tmWeight = ptm32->tmWeight;
    ptm16->tmOverhang = ptm32->tmOverhang;
    ptm16->tmDigitizedAspectX = ptm32->tmDigitizedAspectX;
    ptm16->tmDigitizedAspectY = ptm32->tmDigitizedAspectY;
    ptm16->tmFirstChar = ptm32->tmFirstChar;
    ptm16->tmLastChar = ptm32->tmLastChar;
    ptm16->tmDefaultChar = ptm32->tmDefaultChar;
    ptm16->tmBreakChar = ptm32->tmBreakChar;
    ptm16->tmItalic = ptm32->tmItalic;
    ptm16->tmUnderlined = ptm32->tmUnderlined;
    ptm16->tmStruckOut = ptm32->tmStruckOut;
    ptm16->tmPitchAndFamily = ptm32->tmPitchAndFamily;
    ptm16->tmCharSet = ptm32->tmCharSet;
}

void FONT_TextMetric16ToA(const TEXTMETRIC16 *ptm16, LPTEXTMETRICA ptm32 )
{
    ptm32->tmHeight = ptm16->tmHeight;
    ptm32->tmAscent = ptm16->tmAscent;
    ptm32->tmDescent = ptm16->tmDescent;
    ptm32->tmInternalLeading = ptm16->tmInternalLeading;
    ptm32->tmExternalLeading = ptm16->tmExternalLeading;
    ptm32->tmAveCharWidth = ptm16->tmAveCharWidth;
    ptm32->tmMaxCharWidth = ptm16->tmMaxCharWidth;
    ptm32->tmWeight = ptm16->tmWeight;
    ptm32->tmOverhang = ptm16->tmOverhang;
    ptm32->tmDigitizedAspectX = ptm16->tmDigitizedAspectX;
    ptm32->tmDigitizedAspectY = ptm16->tmDigitizedAspectY;
    ptm32->tmFirstChar = ptm16->tmFirstChar;
    ptm32->tmLastChar = ptm16->tmLastChar;
    ptm32->tmDefaultChar = ptm16->tmDefaultChar;
    ptm32->tmBreakChar = ptm16->tmBreakChar;
    ptm32->tmItalic = ptm16->tmItalic;
    ptm32->tmUnderlined = ptm16->tmUnderlined;
    ptm32->tmStruckOut = ptm16->tmStruckOut;
    ptm32->tmPitchAndFamily = ptm16->tmPitchAndFamily;
    ptm32->tmCharSet = ptm16->tmCharSet;
}

void FONT_TextMetric16ToW(const TEXTMETRIC16 *ptm16, LPTEXTMETRICW ptm32 )
{
    ptm32->tmHeight = ptm16->tmHeight;
    ptm32->tmAscent = ptm16->tmAscent;
    ptm32->tmDescent = ptm16->tmDescent;
    ptm32->tmInternalLeading = ptm16->tmInternalLeading;
    ptm32->tmExternalLeading = ptm16->tmExternalLeading;
    ptm32->tmAveCharWidth = ptm16->tmAveCharWidth;
    ptm32->tmMaxCharWidth = ptm16->tmMaxCharWidth;
    ptm32->tmWeight = ptm16->tmWeight;
    ptm32->tmOverhang = ptm16->tmOverhang;
    ptm32->tmDigitizedAspectX = ptm16->tmDigitizedAspectX;
    ptm32->tmDigitizedAspectY = ptm16->tmDigitizedAspectY;
    ptm32->tmFirstChar = ptm16->tmFirstChar;
    ptm32->tmLastChar = ptm16->tmLastChar;
    ptm32->tmDefaultChar = ptm16->tmDefaultChar;
    ptm32->tmBreakChar = ptm16->tmBreakChar;
    ptm32->tmItalic = ptm16->tmItalic;
    ptm32->tmUnderlined = ptm16->tmUnderlined;
    ptm32->tmStruckOut = ptm16->tmStruckOut;
    ptm32->tmPitchAndFamily = ptm16->tmPitchAndFamily;
    ptm32->tmCharSet = ptm16->tmCharSet;
}

void FONT_TextMetricAToW(const TEXTMETRICA *ptm32A, LPTEXTMETRICW ptm32W )
{
    ptm32W->tmHeight = ptm32A->tmHeight;
    ptm32W->tmAscent = ptm32A->tmAscent;
    ptm32W->tmDescent = ptm32A->tmDescent;
    ptm32W->tmInternalLeading = ptm32A->tmInternalLeading;
    ptm32W->tmExternalLeading = ptm32A->tmExternalLeading;
    ptm32W->tmAveCharWidth = ptm32A->tmAveCharWidth;
    ptm32W->tmMaxCharWidth = ptm32A->tmMaxCharWidth;
    ptm32W->tmWeight = ptm32A->tmWeight;
    ptm32W->tmOverhang = ptm32A->tmOverhang;
    ptm32W->tmDigitizedAspectX = ptm32A->tmDigitizedAspectX;
    ptm32W->tmDigitizedAspectY = ptm32A->tmDigitizedAspectY;
    ptm32W->tmFirstChar = ptm32A->tmFirstChar;
    ptm32W->tmLastChar = ptm32A->tmLastChar;
    ptm32W->tmDefaultChar = ptm32A->tmDefaultChar;
    ptm32W->tmBreakChar = ptm32A->tmBreakChar;
    ptm32W->tmItalic = ptm32A->tmItalic;
    ptm32W->tmUnderlined = ptm32A->tmUnderlined;
    ptm32W->tmStruckOut = ptm32A->tmStruckOut;
    ptm32W->tmPitchAndFamily = ptm32A->tmPitchAndFamily;
    ptm32W->tmCharSet = ptm32A->tmCharSet;
}

void FONT_TextMetricWToA(const TEXTMETRICW *ptmW, LPTEXTMETRICA ptmA )
{
    ptmA->tmHeight = ptmW->tmHeight;
    ptmA->tmAscent = ptmW->tmAscent;
    ptmA->tmDescent = ptmW->tmDescent;
    ptmA->tmInternalLeading = ptmW->tmInternalLeading;
    ptmA->tmExternalLeading = ptmW->tmExternalLeading;
    ptmA->tmAveCharWidth = ptmW->tmAveCharWidth;
    ptmA->tmMaxCharWidth = ptmW->tmMaxCharWidth;
    ptmA->tmWeight = ptmW->tmWeight;
    ptmA->tmOverhang = ptmW->tmOverhang;
    ptmA->tmDigitizedAspectX = ptmW->tmDigitizedAspectX;
    ptmA->tmDigitizedAspectY = ptmW->tmDigitizedAspectY;
    ptmA->tmFirstChar = ptmW->tmFirstChar;
    ptmA->tmLastChar = ptmW->tmLastChar;
    ptmA->tmDefaultChar = ptmW->tmDefaultChar;
    ptmA->tmBreakChar = ptmW->tmBreakChar;
    ptmA->tmItalic = ptmW->tmItalic;
    ptmA->tmUnderlined = ptmW->tmUnderlined;
    ptmA->tmStruckOut = ptmW->tmStruckOut;
    ptmA->tmPitchAndFamily = ptmW->tmPitchAndFamily;
    ptmA->tmCharSet = ptmW->tmCharSet;
}


void FONT_NewTextMetricExWTo16(const NEWTEXTMETRICEXW *ptmW, LPNEWTEXTMETRICEX16 ptm16 )
{
    FONT_TextMetricWTo16((LPTEXTMETRICW)ptmW, (LPTEXTMETRIC16)ptm16);
    ptm16->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptm16->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptm16->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptm16->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    memcpy(&ptm16->ntmFontSig, &ptmW->ntmFontSig, sizeof(FONTSIGNATURE));
}

void FONT_NewTextMetricExWToA(const NEWTEXTMETRICEXW *ptmW, LPNEWTEXTMETRICEXA ptmA )
{
    FONT_TextMetricWToA((LPTEXTMETRICW)ptmW, (LPTEXTMETRICA)ptmA);
    ptmA->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptmA->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptmA->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptmA->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    memcpy(&ptmA->ntmFontSig, &ptmW->ntmFontSig, sizeof(FONTSIGNATURE));
}

void FONT_NewTextMetricEx16ToW(const NEWTEXTMETRICEX16 *ptm16, LPNEWTEXTMETRICEXW ptmW )
{
    FONT_TextMetric16ToW((LPTEXTMETRIC16)ptm16, (LPTEXTMETRICW)ptmW);
    ptmW->ntmTm.ntmFlags = ptm16->ntmTm.ntmFlags;
    ptmW->ntmTm.ntmSizeEM = ptm16->ntmTm.ntmSizeEM;
    ptmW->ntmTm.ntmCellHeight = ptm16->ntmTm.ntmCellHeight;
    ptmW->ntmTm.ntmAvgWidth = ptm16->ntmTm.ntmAvgWidth;
    memcpy(&ptmW->ntmFontSig, &ptm16->ntmFontSig, sizeof(FONTSIGNATURE));
}


/***********************************************************************
 *           CreateFontIndirect   (GDI.57)
 */
HFONT16 WINAPI CreateFontIndirect16( const LOGFONT16 *plf16 )
{
    LOGFONTW lfW;

    if(plf16) {
        FONT_LogFont16ToW( plf16, &lfW );
	return CreateFontIndirectW( &lfW );
    } else {
        return CreateFontIndirectW( NULL );
    }
}


/***********************************************************************
 *           CreateFontIndirectA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectA( const LOGFONTA *plfA )
{
    LOGFONTW lfW;

    if (plfA) {
	FONT_LogFontAToW( plfA, &lfW );
	return CreateFontIndirectW( &lfW );
     } else
	return CreateFontIndirectW( NULL );

}

/***********************************************************************
 *           CreateFontIndirectW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectW( const LOGFONTW *plf )
{
    HFONT hFont = 0;

    if (plf)
    {
        FONTOBJ* fontPtr;
	if ((fontPtr = GDI_AllocObject( sizeof(FONTOBJ), FONT_MAGIC, &hFont )))
	{
	    memcpy( &fontPtr->logfont, plf, sizeof(LOGFONTW) );

	    TRACE("(%ld %ld %ld %ld) %s %s %s => %04x\n",
                  plf->lfHeight, plf->lfWidth, 
                  plf->lfEscapement, plf->lfOrientation,
                  debugstr_w(plf->lfFaceName),
                  plf->lfWeight > 400 ? "Bold" : "",
                  plf->lfItalic ? "Italic" : "", hFont);

	    if (plf->lfEscapement != plf->lfOrientation) {
	      /* this should really depend on whether GM_ADVANCED is set */
	      fontPtr->logfont.lfOrientation = fontPtr->logfont.lfEscapement;
	      WARN("orientation angle %f set to "
                   "escapement angle %f for new font %04x\n", 
                   plf->lfOrientation/10., plf->lfEscapement/10., hFont);
	    }
	    GDI_ReleaseObj( hFont );
	}
    }
    else WARN("(NULL) => NULL\n");

    return hFont;
}

/***********************************************************************
 *           CreateFont    (GDI.56)
 */
HFONT16 WINAPI CreateFont16(INT16 height, INT16 width, INT16 esc, INT16 orient,
                            INT16 weight, BYTE italic, BYTE underline,
                            BYTE strikeout, BYTE charset, BYTE outpres,
                            BYTE clippres, BYTE quality, BYTE pitch,
                            LPCSTR name )
{
    LOGFONT16 logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;
   
    if (name) 
	lstrcpynA(logfont.lfFaceName,name,sizeof(logfont.lfFaceName));
    else 
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirect16( &logfont );
}

/*************************************************************************
 *           CreateFontA    (GDI32.@)
 */
HFONT WINAPI CreateFontA( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCSTR name )
{
    LOGFONTA logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;
   
    if (name) 
	lstrcpynA(logfont.lfFaceName,name,sizeof(logfont.lfFaceName));
    else 
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectA( &logfont );
}

/*************************************************************************
 *           CreateFontW    (GDI32.@)
 */
HFONT WINAPI CreateFontW( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCWSTR name )
{
    LOGFONTW logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;
   
    if (name) 
	lstrcpynW(logfont.lfFaceName, name, 
		  sizeof(logfont.lfFaceName) / sizeof(WCHAR));
    else 
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectW( &logfont );
}


/***********************************************************************
 *           FONT_GetObject16
 */
INT16 FONT_GetObject16( FONTOBJ * font, INT16 count, LPSTR buffer )
{
    LOGFONT16 lf16;

    FONT_LogFontWTo16( &font->logfont, &lf16 );

    if (count > sizeof(LOGFONT16)) count = sizeof(LOGFONT16);
    memcpy( buffer, &lf16, count );
    return count;
}

/***********************************************************************
 *           FONT_GetObjectA
 */
INT FONT_GetObjectA( FONTOBJ *font, INT count, LPSTR buffer )
{
    LOGFONTA lfA;

    FONT_LogFontWToA( &font->logfont, &lfA );

    if (count > sizeof(lfA)) count = sizeof(lfA);
    memcpy( buffer, &lfA, count );
    return count;
}
/***********************************************************************
 *           FONT_GetObjectW
 */
INT FONT_GetObjectW( FONTOBJ *font, INT count, LPSTR buffer )
{
    if (count > sizeof(LOGFONTW)) count = sizeof(LOGFONTW);
    memcpy( buffer, &font->logfont, count );
    return count;
}


/***********************************************************************
 *              FONT_EnumInstance16
 *
 * Called by the device driver layer to pass font info
 * down to the application.
 */
static INT FONT_EnumInstance16( LPENUMLOGFONTEXW plf, LPNEWTEXTMETRICEXW ptm,
				DWORD fType, LPARAM lp )
{
#define pfe ((fontEnum16*)lp)
    if( pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET || 
	pfe->lpLogFontParam->lfCharSet == plf->elfLogFont.lfCharSet )
    {
        FONT_EnumLogFontExWTo16(plf, pfe->lpLogFont);
	FONT_NewTextMetricExWTo16(ptm, pfe->lpTextMetric);
        return pfe->lpEnumFunc( pfe->segLogFont, pfe->segTextMetric,
				(UINT16)fType, (LPARAM)(pfe->lpData) );
    }
#undef pfe
    return 1;
}

/***********************************************************************
 *              FONT_EnumInstance
 */
static INT FONT_EnumInstance( LPENUMLOGFONTEXW plf, LPNEWTEXTMETRICEXW ptm,
			      DWORD fType, LPARAM lp )
{
    /* lfCharSet is at the same offset in both LOGFONTA and LOGFONTW */

#define pfe ((fontEnum32*)lp)
    if( pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET || 
	pfe->lpLogFontParam->lfCharSet == plf->elfLogFont.lfCharSet )
    {
	/* convert font metrics */

	if( pfe->dwFlags & ENUM_UNICODE )
	{
	    return pfe->lpEnumFunc( plf, ptm, fType, pfe->lpData );
	}
	else
	{
	    ENUMLOGFONTEXA logfont;
	    NEWTEXTMETRICEXA tmA;

	    FONT_EnumLogFontExWToA( plf, &logfont);
	    FONT_NewTextMetricExWToA( ptm, &tmA );

	    return pfe->lpEnumFunc( (LPENUMLOGFONTEXW)&logfont, 
				    (LPNEWTEXTMETRICEXW)&tmA, fType,
				    pfe->lpData );
	}
    }
#undef pfe
    return 1;
}

/***********************************************************************
 *              EnumFontFamiliesEx16	(GDI.613)
 */
INT16 WINAPI EnumFontFamiliesEx16( HDC16 hDC, LPLOGFONT16 plf,
                                   FONTENUMPROCEX16 efproc, LPARAM lParam,
                                   DWORD dwFlags)
{
    BOOL (*enum_func)(HDC,LPLOGFONTW,DEVICEFONTENUMPROC,LPARAM);
    INT16	retVal = 0;
    DC* 	dc = DC_GetDCPtr( hDC );

    if (!dc) return 0;
    enum_func = dc->funcs->pEnumDeviceFonts;
    GDI_ReleaseObj( hDC );

    if (enum_func)
    {
	LPNEWTEXTMETRICEX16 lptm16 = SEGPTR_ALLOC( sizeof(NEWTEXTMETRICEX16) );
	if( lptm16 )
	{
	    LPENUMLOGFONTEX16 lplf16 = SEGPTR_ALLOC( sizeof(ENUMLOGFONTEX16) );
	    if( lplf16 )
	    {
		fontEnum16	fe16;
		LOGFONTW        lfW;
		FONT_LogFont16ToW(plf, &lfW);

		fe16.lpLogFontParam = plf;
		fe16.lpEnumFunc = efproc;
		fe16.lpData = lParam;
		
		fe16.lpTextMetric = lptm16;
		fe16.lpLogFont = lplf16;
		fe16.segTextMetric = SEGPTR_GET(lptm16);
		fe16.segLogFont = SEGPTR_GET(lplf16);

		retVal = enum_func( hDC, &lfW, FONT_EnumInstance16,
				    (LPARAM)&fe16 );
		SEGPTR_FREE(lplf16);
	    }
	    SEGPTR_FREE(lptm16);
	}
    }
    return retVal;
}

/***********************************************************************
 *		FONT_EnumFontFamiliesEx
 */
static INT FONT_EnumFontFamiliesEx( HDC hDC, LPLOGFONTW plf,
				    FONTENUMPROCEXW efproc, 
				    LPARAM lParam, DWORD dwUnicode)
{
    BOOL (*enum_func)(HDC,LPLOGFONTW,DEVICEFONTENUMPROC,LPARAM);
    INT ret = 1;
    DC *dc = DC_GetDCPtr( hDC );
    fontEnum32 fe32;
    BOOL enum_gdi_fonts;

    if (!dc) return 0;

    fe32.lpLogFontParam = plf;
    fe32.lpEnumFunc = efproc;
    fe32.lpData = lParam;
    fe32.dwFlags = dwUnicode;

    enum_func = dc->funcs->pEnumDeviceFonts;
    GDI_ReleaseObj( hDC );
    enum_gdi_fonts = GetDeviceCaps(hDC, TEXTCAPS) & TC_VA_ABLE;

    if (!enum_func && !enum_gdi_fonts) return 0;
    
    if (enum_gdi_fonts)
        ret = WineEngEnumFonts( plf, FONT_EnumInstance, (LPARAM)&fe32 );
    if (ret && enum_func)
	ret = enum_func( hDC, plf, FONT_EnumInstance, (LPARAM)&fe32 );
    return ret;
}

/***********************************************************************
 *              EnumFontFamiliesExW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExW( HDC hDC, LPLOGFONTW plf,
                                    FONTENUMPROCEXW efproc, 
                                    LPARAM lParam, DWORD dwFlags )
{
    return  FONT_EnumFontFamiliesEx( hDC, plf, efproc, lParam, ENUM_UNICODE );
}

/***********************************************************************
 *              EnumFontFamiliesExA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExA( HDC hDC, LPLOGFONTA plf,
                                    FONTENUMPROCEXA efproc, 
                                    LPARAM lParam, DWORD dwFlags)
{
    LOGFONTW lfW;
    FONT_LogFontAToW( plf, &lfW );

    return  FONT_EnumFontFamiliesEx( hDC, &lfW,
				     (FONTENUMPROCEXW)efproc, lParam, 0);
}

/***********************************************************************
 *              EnumFontFamilies16	(GDI.330)
 */
INT16 WINAPI EnumFontFamilies16( HDC16 hDC, LPCSTR lpFamily,
                                 FONTENUMPROC16 efproc, LPARAM lpData )
{
    LOGFONT16	lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = '\0';

    return EnumFontFamiliesEx16( hDC, &lf, (FONTENUMPROCEX16)efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamiliesA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesA( HDC hDC, LPCSTR lpFamily,
                                  FONTENUMPROCA efproc, LPARAM lpData )
{
    LOGFONTA	lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = lf.lfFaceName[1] = '\0';

    return EnumFontFamiliesExA( hDC, &lf, (FONTENUMPROCEXA)efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamiliesW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesW( HDC hDC, LPCWSTR lpFamily,
                                  FONTENUMPROCW efproc, LPARAM lpData )
{
    LOGFONTW  lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpynW( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = 0;

    return EnumFontFamiliesExW( hDC, &lf, (FONTENUMPROCEXW)efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFonts16		(GDI.70)
 */
INT16 WINAPI EnumFonts16( HDC16 hDC, LPCSTR lpName, FONTENUMPROC16 efproc,
                          LPARAM lpData )
{
    return EnumFontFamilies16( hDC, lpName, (FONTENUMPROCEX16)efproc, lpData );
}

/***********************************************************************
 *              EnumFontsA		(GDI32.@)
 */
INT WINAPI EnumFontsA( HDC hDC, LPCSTR lpName, FONTENUMPROCA efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesA( hDC, lpName, efproc, lpData );
}

/***********************************************************************
 *              EnumFontsW		(GDI32.@)
 */
INT WINAPI EnumFontsW( HDC hDC, LPCWSTR lpName, FONTENUMPROCW efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesW( hDC, lpName, efproc, lpData );
}


/***********************************************************************
 *           GetTextCharacterExtra    (GDI.89)
 */
INT16 WINAPI GetTextCharacterExtra16( HDC16 hdc )
{
    return (INT16)GetTextCharacterExtra( hdc );
}


/***********************************************************************
 *           GetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI GetTextCharacterExtra( HDC hdc )
{
    INT ret;
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    ret = abs( (dc->charExtra * dc->wndExtX + dc->vportExtX / 2)
                 / dc->vportExtX );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI.8)
 */
INT16 WINAPI SetTextCharacterExtra16( HDC16 hdc, INT16 extra )
{
    return (INT16)SetTextCharacterExtra( hdc, extra );
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI SetTextCharacterExtra( HDC hdc, INT extra )
{
    INT prev;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetTextCharacterExtra)
        prev = dc->funcs->pSetTextCharacterExtra( dc, extra );
    else
    {
        extra = (extra * dc->vportExtX + dc->wndExtX / 2) / dc->wndExtX;
        prev = (dc->charExtra * dc->wndExtX + dc->vportExtX / 2) / dc->vportExtX;
        dc->charExtra = abs(extra);
    }
    GDI_ReleaseObj( hdc );
    return prev;
}


/***********************************************************************
 *           SetTextJustification    (GDI.10)
 */
INT16 WINAPI SetTextJustification16( HDC16 hdc, INT16 extra, INT16 breaks )
{
    return SetTextJustification( hdc, extra, breaks );
}


/***********************************************************************
 *           SetTextJustification    (GDI32.@)
 */
BOOL WINAPI SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetTextJustification)
        ret = dc->funcs->pSetTextJustification( dc, extra, breaks );
    else
    {
        extra = abs((extra * dc->vportExtX + dc->wndExtX / 2) / dc->wndExtX);
        if (!extra) breaks = 0;
        dc->breakTotalExtra = extra;
        dc->breakCount = breaks;
        if (breaks)
        {
            dc->breakExtra = extra / breaks;
            dc->breakRem   = extra - (dc->breakCount * dc->breakExtra);
        }
        else
        {
            dc->breakExtra = 0;
            dc->breakRem   = 0;
        }
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           GetTextFace    (GDI.92)
 */
INT16 WINAPI GetTextFace16( HDC16 hdc, INT16 count, LPSTR name )
{
        return GetTextFaceA(hdc,count,name);
}

/***********************************************************************
 *           GetTextFaceA    (GDI32.@)
 */
INT WINAPI GetTextFaceA( HDC hdc, INT count, LPSTR name )
{
    INT res = GetTextFaceW(hdc, 0, NULL);
    LPWSTR nameW = HeapAlloc( GetProcessHeap(), 0, res * 2 );
    GetTextFaceW( hdc, res, nameW );
    
    if (name)
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, name, count,
				   NULL, NULL);
    else
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL);
    HeapFree( GetProcessHeap(), 0, nameW );
    return res;
}

/***********************************************************************
 *           GetTextFaceW    (GDI32.@)
 */
INT WINAPI GetTextFaceW( HDC hdc, INT count, LPWSTR name )
{
    FONTOBJ *font;
    INT     ret = 0;

    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;

    if ((font = (FONTOBJ *) GDI_GetObjPtr( dc->hFont, FONT_MAGIC )))
    {
        if (name)
        {
            lstrcpynW( name, font->logfont.lfFaceName, count );
            ret = strlenW(name);
        }
        else ret = strlenW(font->logfont.lfFaceName) + 1;
        GDI_ReleaseObj( dc->hFont );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           GetTextExtent    (GDI.91)
 */
DWORD WINAPI GetTextExtent16( HDC16 hdc, LPCSTR str, INT16 count )
{
    SIZE16 size;
    if (!GetTextExtentPoint16( hdc, str, count, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           GetTextExtentPoint    (GDI.471)
 *
 * FIXME: Should this have a bug for compatibility?
 * Original Windows versions of GetTextExtentPoint{A,W} have documented
 * bugs (-> MSDN KB q147647.txt).
 */
BOOL16 WINAPI GetTextExtentPoint16( HDC16 hdc, LPCSTR str, INT16 count,
                                    LPSIZE16 size )
{
    SIZE size32;
    BOOL ret;
    TRACE("%04x, %p (%s), %d, %p\n", hdc, str, debugstr_an(str, count), count, size);
    ret = GetTextExtentPoint32A( hdc, str, count, &size32 );
    size->cx = size32.cx;
    size->cy = size32.cy;
    return (BOOL16)ret;
}


/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPoint32A( HDC hdc, LPCSTR str, INT count,
                                     LPSIZE size )
{
    BOOL ret = FALSE;
    INT wlen;
    LPWSTR p = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    if (p) {
	ret = GetTextExtentPoint32W( hdc, p, wlen, size );
	HeapFree( GetProcessHeap(), 0, p );
    }

    TRACE("(%08x %s %d %p): returning %ld x %ld\n",
          hdc, debugstr_an (str, count), count, size, size->cx, size->cy );
    return ret;
}


/***********************************************************************
 * GetTextExtentPoint32W [GDI32.@]  Computes width/height for a string
 *
 * Computes width and height of the specified string.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentPoint32W(
    HDC hdc,     /* [in]  Handle of device context */
    LPCWSTR str,   /* [in]  Address of text string */
    INT count,   /* [in]  Number of characters in string */
    LPSIZE size) /* [out] Address of structure for string size */
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    if(dc->gdiFont)
        ret = WineEngGetTextExtentPoint(dc->gdiFont, str, count, size);
    else if(dc->funcs->pGetTextExtentPoint)
        ret = dc->funcs->pGetTextExtentPoint( dc, str, count, size );

    GDI_ReleaseObj( hdc );

    TRACE("(%08x %s %d %p): returning %ld x %ld\n",
          hdc, debugstr_wn (str, count), count, size, size->cx, size->cy );
    return ret;
}


/***********************************************************************
 *           GetTextExtentPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointA( HDC hdc, LPCSTR str, INT count,
                                          LPSIZE size )
{
    TRACE("not bug compatible.\n");
    return GetTextExtentPoint32A( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextExtentPointW   (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointW( HDC hdc, LPCWSTR str, INT count,
                                          LPSIZE size )
{
    TRACE("not bug compatible.\n");
    return GetTextExtentPoint32W( hdc, str, count, size );
}


/***********************************************************************
 *           GetTextExtentExPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentExPointA( HDC hdc, LPCSTR str, INT count,
				   INT maxExt, LPINT lpnFit,
				   LPINT alpDx, LPSIZE size )
{
    BOOL ret;
    INT wlen;
    LPWSTR p = FONT_mbtowc( hdc, str, count, &wlen, NULL);
    ret = GetTextExtentExPointW( hdc, p, wlen, maxExt, lpnFit, alpDx, size);
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/***********************************************************************
 *           GetTextExtentExPointW    (GDI32.@)
 *
 * Return the size of the string as it would be if it was output properly by
 * e.g. TextOut.
 *
 * This should include
 * - Intercharacter spacing
 * - justification spacing (not yet done)
 * - kerning? see below
 *
 * Kerning.  Since kerning would be carried out by the rendering code it should
 * be done by the driver.  However they don't support it yet.  Also I am not 
 * yet persuaded that (certainly under Win95) any kerning is actually done.
 *
 * str: According to MSDN this should be null-terminated.  That is not true; a
 *      null will not terminate it early.
 * size: Certainly under Win95 this appears buggy or weird if *lpnFit is less
 *       than count.  I have seen it be either the size of the full string or
 *       1 less than the size of the full string.  I have not seen it bear any
 *       resemblance to the portion that would fit.
 * lpnFit: What exactly is fitting?  Stupidly, in my opinion, it includes the
 *         trailing intercharacter spacing and any trailing justification.
 *
 * FIXME
 * Currently we do this by measuring each character etc.  We should do it by
 * passing the request to the driver, perhaps by extending the
 * pGetTextExtentPoint function to take the alpDx argument.  That would avoid
 * thinking about kerning issues and rounding issues in the justification.
 */

BOOL WINAPI GetTextExtentExPointW( HDC hdc, LPCWSTR str, INT count,
				   INT maxExt, LPINT lpnFit,
				   LPINT alpDx, LPSIZE size )
{
    int index, nFit, extent;
    SIZE tSize;
    BOOL ret = FALSE;

    TRACE("(%08x, %s, %d)\n",hdc,debugstr_wn(str,count),maxExt);

    size->cx = size->cy = nFit = extent = 0;
    for(index = 0; index < count; index++)
    {
 	if(!GetTextExtentPoint32W( hdc, str, 1, &tSize )) goto done;
        /* GetTextExtentPoint includes intercharacter spacing. */
        /* FIXME - justification needs doing yet.  Remember that the base
         * data will not be in logical coordinates.
         */
	extent += tSize.cx;
	if( !lpnFit || extent <= maxExt )
        /* It is allowed to be equal. */
        {
	    nFit++;
	    if( alpDx ) alpDx[index] = extent;
        }
	if( tSize.cy > size->cy ) size->cy = tSize.cy;
	str++;
    }
    size->cx = extent;
    if(lpnFit) *lpnFit = nFit;
    ret = TRUE;

    TRACE("returning %d %ld x %ld\n",nFit,size->cx,size->cy);

done:
    return ret;
}

/***********************************************************************
 *           GetTextMetrics    (GDI.93)
 */
BOOL16 WINAPI GetTextMetrics16( HDC16 hdc, TEXTMETRIC16 *metrics )
{
    TEXTMETRICW tm32;

    if (!GetTextMetricsW( (HDC)hdc, &tm32 )) return FALSE;
    FONT_TextMetricWTo16( &tm32, metrics );
    return TRUE;
}


/***********************************************************************
 *           GetTextMetricsA    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsA( HDC hdc, TEXTMETRICA *metrics )
{
    TEXTMETRICW tm32;

    if (!GetTextMetricsW( hdc, &tm32 )) return FALSE;
    FONT_TextMetricWToA( &tm32, metrics );
    return TRUE;
}

/***********************************************************************
 *           GetTextMetricsW    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsW( HDC hdc, TEXTMETRICW *metrics )
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    if (dc->gdiFont)
        ret = WineEngGetTextMetrics(dc->gdiFont, metrics);
    else if (dc->funcs->pGetTextMetrics)
        ret = dc->funcs->pGetTextMetrics( dc, metrics );

    if (ret)
    {
    /* device layer returns values in device units
     * therefore we have to convert them to logical */

#define WDPTOLP(x) ((x<0)?					\
		(-abs((x)*dc->wndExtX/dc->vportExtX)):		\
		(abs((x)*dc->wndExtX/dc->vportExtX)))
#define HDPTOLP(y) ((y<0)?					\
		(-abs((y)*dc->wndExtY/dc->vportExtY)):		\
		(abs((y)*dc->wndExtY/dc->vportExtY)))
	
    metrics->tmHeight           = HDPTOLP(metrics->tmHeight);
    metrics->tmAscent           = HDPTOLP(metrics->tmAscent);
    metrics->tmDescent          = HDPTOLP(metrics->tmDescent);
    metrics->tmInternalLeading  = HDPTOLP(metrics->tmInternalLeading);
    metrics->tmExternalLeading  = HDPTOLP(metrics->tmExternalLeading);
    metrics->tmAveCharWidth     = WDPTOLP(metrics->tmAveCharWidth);
    metrics->tmMaxCharWidth     = WDPTOLP(metrics->tmMaxCharWidth);
    metrics->tmOverhang         = WDPTOLP(metrics->tmOverhang);
        ret = TRUE;

    TRACE("text metrics:\n"
          "    Weight = %03li\t FirstChar = %i\t AveCharWidth = %li\n"
          "    Italic = % 3i\t LastChar = %i\t\t MaxCharWidth = %li\n"
          "    UnderLined = %01i\t DefaultChar = %i\t Overhang = %li\n"
          "    StruckOut = %01i\t BreakChar = %i\t CharSet = %i\n"
          "    PitchAndFamily = %02x\n"
          "    --------------------\n"
          "    InternalLeading = %li\n"
          "    Ascent = %li\n"
          "    Descent = %li\n"
          "    Height = %li\n",
          metrics->tmWeight, metrics->tmFirstChar, metrics->tmAveCharWidth,
          metrics->tmItalic, metrics->tmLastChar, metrics->tmMaxCharWidth,
          metrics->tmUnderlined, metrics->tmDefaultChar, metrics->tmOverhang,
          metrics->tmStruckOut, metrics->tmBreakChar, metrics->tmCharSet,
          metrics->tmPitchAndFamily,
          metrics->tmInternalLeading,
          metrics->tmAscent,
          metrics->tmDescent,
          metrics->tmHeight );
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 * GetOutlineTextMetrics [GDI.308]  Gets metrics for TrueType fonts.
 *
 * NOTES
 *    lpOTM should be LPOUTLINETEXTMETRIC
 *
 * RETURNS
 *    Success: Non-zero or size of required buffer
 *    Failure: 0
 */
UINT16 WINAPI GetOutlineTextMetrics16(
    HDC16 hdc,    /* [in]  Handle of device context */
    UINT16 cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRIC16 lpOTM)  /* [out] Address of metric data array */
{
    FIXME("(%04x,%04x,%p): stub\n", hdc,cbData,lpOTM);
    return 0;
}


/***********************************************************************
 *		GetOutlineTextMetricsA (GDI32.@)
 * Gets metrics for TrueType fonts.
 *
 *
 * RETURNS
 *    Success: Non-zero or size of required buffer
 *    Failure: 0
 */
UINT WINAPI GetOutlineTextMetricsA(
    HDC hdc,    /* [in]  Handle of device context */
    UINT cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRICA lpOTM)  /* [out] Address of metric data array */
{
    char buf[512], *ptr;
    UINT ret, needed;
    OUTLINETEXTMETRICW *lpOTMW = (OUTLINETEXTMETRICW *)buf;
    INT left, len;

    if((ret = GetOutlineTextMetricsW(hdc, sizeof(buf), lpOTMW)) == 0) {
        if((ret = GetOutlineTextMetricsW(hdc, 0, NULL)) == 0)
	    return 0;
	lpOTMW = HeapAlloc(GetProcessHeap(), 0, ret);
	GetOutlineTextMetricsW(hdc, ret, lpOTMW);
    }

    needed = sizeof(OUTLINETEXTMETRICA);
    if(lpOTMW->otmpFamilyName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFamilyName), -1, 
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFaceName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFaceName), -1, 
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpStyleName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpStyleName), -1, 
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFullName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFullName), -1, 
				      NULL, 0, NULL, NULL);

    if(!lpOTM) {
        ret = needed;
	goto end;
    }

    if(needed > cbData) {
        ret = 0;
	goto end;
    }


    lpOTM->otmSize = needed;
    FONT_TextMetricWToA( &lpOTMW->otmTextMetrics, &lpOTM->otmTextMetrics );
    lpOTM->otmFiller = 0;
    lpOTM->otmPanoseNumber = lpOTMW->otmPanoseNumber;
    lpOTM->otmfsSelection = lpOTMW->otmfsSelection;
    lpOTM->otmfsType = lpOTMW->otmfsType;
    lpOTM->otmsCharSlopeRise = lpOTMW->otmsCharSlopeRise;
    lpOTM->otmsCharSlopeRun = lpOTMW->otmsCharSlopeRun;
    lpOTM->otmItalicAngle = lpOTMW->otmItalicAngle;
    lpOTM->otmEMSquare = lpOTMW->otmEMSquare;
    lpOTM->otmAscent = lpOTMW->otmAscent;
    lpOTM->otmDescent = lpOTMW->otmDescent;
    lpOTM->otmLineGap = lpOTMW->otmLineGap;
    lpOTM->otmsCapEmHeight = lpOTMW->otmsCapEmHeight;
    lpOTM->otmsXHeight = lpOTMW->otmsXHeight;
    lpOTM->otmrcFontBox = lpOTMW->otmrcFontBox;
    lpOTM->otmMacAscent = lpOTMW->otmMacAscent;
    lpOTM->otmMacDescent = lpOTMW->otmMacDescent;
    lpOTM->otmMacLineGap = lpOTMW->otmMacLineGap;
    lpOTM->otmusMinimumPPEM = lpOTMW->otmusMinimumPPEM;
    lpOTM->otmptSubscriptSize = lpOTMW->otmptSubscriptSize;
    lpOTM->otmptSubscriptOffset = lpOTMW->otmptSubscriptOffset;
    lpOTM->otmptSuperscriptSize = lpOTMW->otmptSuperscriptSize;
    lpOTM->otmptSuperscriptOffset = lpOTMW->otmptSuperscriptOffset;
    lpOTM->otmsStrikeoutSize = lpOTMW->otmsStrikeoutSize;
    lpOTM->otmsStrikeoutPosition = lpOTMW->otmsStrikeoutPosition;
    lpOTM->otmsUnderscoreSize = lpOTMW->otmsUnderscoreSize;
    lpOTM->otmsUnderscorePosition = lpOTMW->otmsUnderscorePosition;


    ptr = (char*)(lpOTM + 1);
    left = needed - sizeof(*lpOTM);

    if(lpOTMW->otmpFamilyName) {
        lpOTM->otmpFamilyName = (LPSTR)(ptr - (char*)lpOTM);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFamilyName), -1, 
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        lpOTM->otmpFamilyName = 0;

    if(lpOTMW->otmpFaceName) {
        lpOTM->otmpFaceName = (LPSTR)(ptr - (char*)lpOTM);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFaceName), -1, 
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        lpOTM->otmpFaceName = 0;

    if(lpOTMW->otmpStyleName) {
        lpOTM->otmpStyleName = (LPSTR)(ptr - (char*)lpOTM);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpStyleName), -1, 
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        lpOTM->otmpStyleName = 0;

    if(lpOTMW->otmpFullName) {
        lpOTM->otmpFullName = (LPSTR)(ptr - (char*)lpOTM);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFullName), -1, 
				  ptr, left, NULL, NULL);
	left -= len;
    } else
        lpOTM->otmpFullName = 0;
    
    assert(left == 0);
    
    ret = needed;

end:
    if(lpOTMW != (OUTLINETEXTMETRICW *)buf)
        HeapFree(GetProcessHeap(), 0, lpOTMW);

    return ret;
}


/***********************************************************************
 *           GetOutlineTextMetricsW [GDI32.@]
 */
UINT WINAPI GetOutlineTextMetricsW(
    HDC hdc,    /* [in]  Handle of device context */
    UINT cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRICW lpOTM)  /* [out] Address of metric data array */
{
    DC *dc = DC_GetDCPtr( hdc );
    UINT ret;

    TRACE("(%d,%d,%p)\n", hdc, cbData, lpOTM);
    if(!dc) return 0;

    if(dc->gdiFont)
        ret = WineEngGetOutlineTextMetrics(dc->gdiFont, cbData, lpOTM);

    else { /* This stuff was in GetOutlineTextMetricsA, I've moved it here
	      but really this should just be a return 0. */

        ret = sizeof(*lpOTM);
        if (lpOTM) {
	    if(cbData < ret)
	        ret = 0;
	    else {
	        memset(lpOTM, 0, ret);
		lpOTM->otmSize = sizeof(*lpOTM);
		GetTextMetricsW(hdc, &lpOTM->otmTextMetrics);
		/*
		  Further fill of the structure not implemented,
		  Needs real values for the structure members
		*/
	    }
	}
    }
    GDI_ReleaseObj(hdc);
    return ret;
}


/***********************************************************************
 *           GetCharWidth    (GDI.350)
 */
BOOL16 WINAPI GetCharWidth16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar,
                              LPINT16 buffer )
{
    BOOL	retVal = FALSE;

    if( firstChar != lastChar )
    {
	LPINT	buf32 = (LPINT)HeapAlloc(GetProcessHeap(), 0,
				 sizeof(INT)*(1 + (lastChar - firstChar)));
	if( buf32 )
	{
	    LPINT	obuf32 = buf32;
	    int		i;

            retVal = GetCharWidth32A(hdc, firstChar, lastChar, buf32);
	    if (retVal)
	    {
		for (i = firstChar; i <= lastChar; i++)
		    *buffer++ = *buf32++;
	    }
	    HeapFree(GetProcessHeap(), 0, obuf32);
	}
    }
    else /* happens quite often to warrant a special treatment */
    {
	INT chWidth;
	retVal = GetCharWidth32A(hdc, firstChar, lastChar, &chWidth );
       *buffer = chWidth;
    }
    return retVal;
}


/***********************************************************************
 *           GetCharWidthA      (GDI32.@)
 *           GetCharWidth32A    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32A( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    UINT i, extra;
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    if (dc->gdiFont)
        ret = WineEngGetCharWidth( dc->gdiFont, firstChar, lastChar, buffer );
    else if (dc->funcs->pGetCharWidth)
        ret = dc->funcs->pGetCharWidth( dc, firstChar, lastChar, buffer);

    if (ret)
    {
        /* convert device units to logical */

        extra = dc->vportExtX >> 1;
        for( i = firstChar; i <= lastChar; i++, buffer++ )
            *buffer = (*buffer * dc->wndExtX + extra) / dc->vportExtX;
        ret = TRUE;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           GetCharWidthW      (GDI32.@)
 *           GetCharWidth32W    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32W( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    return GetCharWidth32A( hdc, firstChar, lastChar, buffer );
}


/* FIXME: all following APIs ******************************************/
 

/***********************************************************************
 *           SetMapperFlags    (GDI.349)
 */
DWORD WINAPI SetMapperFlags16( HDC16 hDC, DWORD dwFlag )
{
    return SetMapperFlags( hDC, dwFlag );
}


/***********************************************************************
 *           SetMapperFlags    (GDI32.@)
 */
DWORD WINAPI SetMapperFlags( HDC hDC, DWORD dwFlag )
{
    DC *dc = DC_GetDCPtr( hDC );
    DWORD ret = 0; 
    if(!dc) return 0;
    if(dc->funcs->pSetMapperFlags)
        ret = dc->funcs->pSetMapperFlags( dc, dwFlag );
    else
        FIXME("(0x%04x, 0x%08lx): stub - harmless\n", hDC, dwFlag);
    GDI_ReleaseObj( hDC );
    return ret;
}

/***********************************************************************
 *          GetAspectRatioFilterEx  (GDI.486)
 */
BOOL16 WINAPI GetAspectRatioFilterEx16( HDC16 hdc, LPSIZE16 pAspectRatio )
{
  FIXME("(%04x, %p): -- Empty Stub !\n", hdc, pAspectRatio);
  return FALSE;
}

/***********************************************************************
 *          GetAspectRatioFilterEx  (GDI32.@)
 */
BOOL WINAPI GetAspectRatioFilterEx( HDC hdc, LPSIZE pAspectRatio )
{
  FIXME("(%04x, %p): -- Empty Stub !\n", hdc, pAspectRatio);
  return FALSE;
}

/***********************************************************************
 *           GetCharABCWidths   (GDI.307)
 */
BOOL16 WINAPI GetCharABCWidths16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar,
                                  LPABC16 abc )
{
    LPABC	abc32 = HeapAlloc(GetProcessHeap(),0,sizeof(ABC)*(lastChar-firstChar+1));
    int		i;

    if (!GetCharABCWidthsA( hdc, firstChar, lastChar, abc32 )) {
	HeapFree(GetProcessHeap(),0,abc32);
	return FALSE;
    }

    for (i=firstChar;i<=lastChar;i++) {
	abc[i-firstChar].abcA = abc32[i-firstChar].abcA;
	abc[i-firstChar].abcB = abc32[i-firstChar].abcB;
	abc[i-firstChar].abcC = abc32[i-firstChar].abcC;
    }
    HeapFree(GetProcessHeap(),0,abc32);
    return TRUE;
}


/***********************************************************************
 *           GetCharABCWidthsA   (GDI32.@)
 */
BOOL WINAPI GetCharABCWidthsA(HDC hdc, UINT firstChar, UINT lastChar,
                                  LPABC abc )
{
    return GetCharABCWidthsW( hdc, firstChar, lastChar, abc );
}


/******************************************************************************
 * GetCharABCWidthsW [GDI32.@]  Retrieves widths of characters in range
 *
 * PARAMS
 *    hdc       [I] Handle of device context
 *    firstChar [I] First character in range to query
 *    lastChar  [I] Last character in range to query
 *    abc       [O] Address of character-width structure
 *
 * NOTES
 *    Only works with TrueType fonts
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsW( HDC hdc, UINT firstChar, UINT lastChar,
                                   LPABC abc )
{
    int		i;
    LPINT	widths = HeapAlloc(GetProcessHeap(),0,(lastChar-firstChar+1)*sizeof(INT));

    FIXME("(%04x,%04x,%04x,%p), returns slightly bogus values.\n", hdc, firstChar, lastChar, abc);

    GetCharWidth32A(hdc,firstChar,lastChar,widths);

    for (i=firstChar;i<=lastChar;i++) {
	abc[i-firstChar].abcA = 0;	/* left distance */
	abc[i-firstChar].abcB = widths[i-firstChar];/* width */
	abc[i-firstChar].abcC = 0;	/* right distance */
    }
    HeapFree(GetProcessHeap(),0,widths);
    return TRUE;
}


/***********************************************************************
 *           GetGlyphOutline    (GDI.309)
 */
DWORD WINAPI GetGlyphOutline16( HDC16 hdc, UINT16 uChar, UINT16 fuFormat,
                                LPGLYPHMETRICS16 lpgm, DWORD cbBuffer,
                                LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    FIXME("(%04x, '%c', %04x, %p, %ld, %p, %p): stub\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    return (DWORD)-1; /* failure */
}


/***********************************************************************
 *           GetGlyphOutlineA    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineA( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    return GetGlyphOutlineW(hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer,
			    lpmat2);
}

/***********************************************************************
 *           GetGlyphOutlineW    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineW( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    DC *dc = DC_GetDCPtr(hdc);
    DWORD ret;

    TRACE("(%04x, '%c', %04x, %p, %ld, %p, %p)\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );

    if(!dc) return GDI_ERROR;

    if(dc->gdiFont)
      ret = WineEngGetGlyphOutline(dc->gdiFont, uChar, fuFormat, lpgm,
				   cbBuffer, lpBuffer, lpmat2);
    else
      ret = GDI_ERROR;

    GDI_ReleaseObj(hdc);
    return ret;
}

/***********************************************************************
 *           CreateScalableFontResource   (GDI.310)
 */
BOOL16 WINAPI CreateScalableFontResource16( UINT16 fHidden,
                                            LPCSTR lpszResourceFile,
                                            LPCSTR fontFile, LPCSTR path )
{
    return CreateScalableFontResourceA( fHidden, lpszResourceFile,
                                          fontFile, path );
}

/***********************************************************************
 *           CreateScalableFontResourceA   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceA( DWORD fHidden,
                                             LPCSTR lpszResourceFile,
                                             LPCSTR lpszFontFile,
                                             LPCSTR lpszCurrentPath )
{
    /* fHidden=1 - only visible for the calling app, read-only, not
     * enumbered with EnumFonts/EnumFontFamilies
     * lpszCurrentPath can be NULL
     */
    FIXME("(%ld,%s,%s,%s): stub\n",
          fHidden, debugstr_a(lpszResourceFile), debugstr_a(lpszFontFile),
          debugstr_a(lpszCurrentPath) );
    return FALSE; /* create failed */
}

/***********************************************************************
 *           CreateScalableFontResourceW   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceW( DWORD fHidden,
                                             LPCWSTR lpszResourceFile,
                                             LPCWSTR lpszFontFile,
                                             LPCWSTR lpszCurrentPath )
{
    FIXME("(%ld,%p,%p,%p): stub\n",
	  fHidden, lpszResourceFile, lpszFontFile, lpszCurrentPath );
    return FALSE; /* create failed */
}


/*************************************************************************
 *             GetRasterizerCaps   (GDI.313)
 */
BOOL16 WINAPI GetRasterizerCaps16( LPRASTERIZER_STATUS lprs, UINT16 cbNumBytes)
{
    return GetRasterizerCaps( lprs, cbNumBytes );
}


/*************************************************************************
 *             GetRasterizerCaps   (GDI32.@)
 */
BOOL WINAPI GetRasterizerCaps( LPRASTERIZER_STATUS lprs, UINT cbNumBytes)
{
  lprs->nSize = sizeof(RASTERIZER_STATUS);
  lprs->wFlags = TT_AVAILABLE|TT_ENABLED;
  lprs->nLanguageID = 0;
  return TRUE;
}


/*************************************************************************
 *             GetKerningPairs   (GDI.332)
 *
 */
INT16 WINAPI GetKerningPairs16( HDC16 hDC, INT16 cPairs,
                                LPKERNINGPAIR16 lpKerningPairs )
{
    /* At this time kerning is ignored (set to 0) */
    int i;
    FIXME("(%x,%d,%p): almost empty stub!\n", hDC, cPairs, lpKerningPairs);
    if (lpKerningPairs)
        for (i = 0; i < cPairs; i++) 
            lpKerningPairs[i].iKernAmount = 0;
 /* FIXME: Should this function call SetLastError (0)?  This is yet another
  * Microsoft function that can return 0 on success or failure
  */
    return 0;
}



/*************************************************************************
 *             GetKerningPairsA   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsA( HDC hDC, DWORD cPairs,
                                 LPKERNINGPAIR lpKerningPairs )
{
    int i;
    FIXME("(%x,%ld,%p): almost empty stub!\n", hDC, cPairs, lpKerningPairs);
    for (i = 0; i < cPairs; i++) 
        lpKerningPairs[i].iKernAmount = 0;
    return 0;
}


/*************************************************************************
 *             GetKerningPairsW   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsW( HDC hDC, DWORD cPairs,
                                 LPKERNINGPAIR lpKerningPairs )
{
    return GetKerningPairsA( hDC, cPairs, lpKerningPairs );
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.@]
 * TranslateCharsetInfo [USER32.@]
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondance between different labelings
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges) 
 * of the same encoding.
 *
 * Only one codepage will be set in lpCs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *lpSrc.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
BOOL WINAPI TranslateCharsetInfo(
  LPDWORD lpSrc, /* [in]
       if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
       if flags == TCI_SRCCHARSET: a character set value
       if flags == TCI_SRCCODEPAGE: a code page value
		 */
  LPCHARSETINFO lpCs, /* [out] structure to receive charset information */
  DWORD flags /* [in] determines interpretation of lpSrc */
) {
    int index = 0;
    switch (flags) {
    case TCI_SRCFONTSIG:
	while (!(*lpSrc>>index & 0x0001) && index<MAXTCIINDEX) index++;
      break;
    case TCI_SRCCODEPAGE:
      while ((UINT) (lpSrc) != FONT_tci[index].ciACP && index < MAXTCIINDEX) index++;
      break;
    case TCI_SRCCHARSET:
      while ((UINT) (lpSrc) != FONT_tci[index].ciCharset && index < MAXTCIINDEX) index++;
      break;
    default:
      return FALSE;
    }
    if (index >= MAXTCIINDEX || FONT_tci[index].ciCharset == DEFAULT_CHARSET) return FALSE;
    memcpy(lpCs, &FONT_tci[index], sizeof(CHARSETINFO));
    return TRUE;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI32.@)
 */
DWORD WINAPI GetFontLanguageInfo(HDC hdc) {
	/* return value 0 is correct for most cases anyway */
        FIXME("(%x):stub!\n", hdc);
	return 0;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI.616)
 */
DWORD WINAPI GetFontLanguageInfo16(HDC16 hdc) {
	/* return value 0 is correct for most cases anyway */
	FIXME("(%x):stub!\n",hdc);
	return 0;
}

/*************************************************************************
 * GetFontData [GDI32.@] Retrieve data for TrueType font
 *
 * RETURNS
 *
 * success: Number of bytes returned 
 * failure: GDI_ERROR
 *
 * NOTES
 *
 * Calls SetLastError()  
 *
 */
DWORD WINAPI GetFontData(HDC hdc, DWORD table, DWORD offset, 
    LPVOID buffer, DWORD length)
{
    DC *dc = DC_GetDCPtr(hdc);
    DWORD ret = GDI_ERROR;

    if(!dc) return GDI_ERROR;

    if(dc->gdiFont)
      ret = WineEngGetFontData(dc->gdiFont, table, offset, buffer, length);

    GDI_ReleaseObj(hdc);
    return ret;
}

/*************************************************************************
 * GetFontData [GDI.311]
 *
 */
DWORD WINAPI GetFontData16(HDC16 hdc, DWORD dwTable, DWORD dwOffset,
			    LPVOID lpvBuffer, DWORD cbData)
{
    return GetFontData(hdc, dwTable, dwOffset, lpvBuffer, cbData);
}

/*************************************************************************
 * GetCharacterPlacementA [GDI32.@]
 *
 * NOTES:
 *  the web browser control of ie4 calls this with dwFlags=0
 */
DWORD WINAPI
GetCharacterPlacementA(HDC hdc, LPCSTR lpString, INT uCount,
			 INT nMaxExtent, GCP_RESULTSA *lpResults,
			 DWORD dwFlags)
{
    DWORD ret=0;
    SIZE size;

    TRACE("%s 0x%08x 0x%08x 0x%08lx:stub!\n",
          debugstr_a(lpString), uCount, nMaxExtent, dwFlags);

    TRACE("lpOrder=%p lpDx=%p lpCaretPos=%p lpClass=%p "
          "lpOutString=%p lpGlyphs=%p\n",
          lpResults->lpOrder, lpResults->lpDx, lpResults->lpCaretPos,
          lpResults->lpClass, lpResults->lpOutString, lpResults->lpGlyphs);

    if(dwFlags)			FIXME("flags 0x%08lx ignored\n", dwFlags);
    if(lpResults->lpOrder)	FIXME("reordering not implemented\n");
    if(lpResults->lpCaretPos)	FIXME("caret positions not implemented\n");
    if(lpResults->lpClass)	FIXME("classes not implemented\n");
    if(lpResults->lpGlyphs)	FIXME("glyphs not implemented\n");

    /* copy will do if the GCP_REORDER flag is not set */
    if(lpResults->lpOutString)
    {
      lstrcpynA(lpResults->lpOutString, lpString, uCount);
    }

    if (lpResults->lpDx)
    {
      int i, c;
      for (i=0; i<uCount;i++)
      { 
        if (GetCharWidth32A(hdc, lpString[i], lpString[i], &c))
          lpResults->lpDx[i]= c;
      }
    }

    if (GetTextExtentPoint32A(hdc, lpString, uCount, &size))
      ret = MAKELONG(size.cx, size.cy);

    return ret;
}

/*************************************************************************
 * GetCharacterPlacementW [GDI32.@]
 */
DWORD WINAPI
GetCharacterPlacementW(HDC hdc, LPCWSTR lpString, INT uCount,
			 INT nMaxExtent, GCP_RESULTSW *lpResults,
			 DWORD dwFlags)
{
    DWORD ret=0;
    SIZE size;

    TRACE("%s 0x%08x 0x%08x 0x%08lx:partial stub!\n",
          debugstr_w(lpString), uCount, nMaxExtent, dwFlags);

    TRACE("lpOrder=%p lpDx=%p lpCaretPos=%p lpClass=%p "
          "lpOutString=%p lpGlyphs=%p\n",
          lpResults->lpOrder, lpResults->lpDx, lpResults->lpCaretPos,
          lpResults->lpClass, lpResults->lpOutString, lpResults->lpGlyphs);

    if(dwFlags)			FIXME("flags 0x%08lx ignored\n", dwFlags);
    if(lpResults->lpOrder)	FIXME("reordering not implemented\n");
    if(lpResults->lpCaretPos)	FIXME("caret positions not implemented\n");
    if(lpResults->lpClass)	FIXME("classes not implemented\n");
    if(lpResults->lpGlyphs)	FIXME("glyphs not implemented\n");

    /* copy will do if the GCP_REORDER flag is not set */
    if(lpResults->lpOutString)
    {
      lstrcpynW(lpResults->lpOutString, lpString, uCount);
    }

    if (lpResults->lpDx)
    {
      int i, c;
      for (i=0; i<uCount;i++)
      { 
        if (GetCharWidth32W(hdc, lpString[i], lpString[i], &c))
          lpResults->lpDx[i]= c;
      }
    }

    if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
      ret = MAKELONG(size.cx, size.cy);

    return ret;
}

/*************************************************************************
 *      GetCharABCWidthsFloatA [GDI32.@]
 */
BOOL WINAPI GetCharABCWidthsFloatA(HDC hdc, UINT iFirstChar, UINT iLastChar,
		                        LPABCFLOAT lpABCF)
{
       FIXME_(gdi)("GetCharABCWidthsFloatA, stub\n");
       return 0;
}

/*************************************************************************
 *      GetCharABCWidthsFloatW [GDI32.@]
 */
BOOL WINAPI GetCharABCWidthsFloatW(HDC hdc, UINT iFirstChar,
		                        UINT iLastChar, LPABCFLOAT lpABCF)
{
       FIXME_(gdi)("GetCharABCWidthsFloatW, stub\n");
       return 0;
}

/*************************************************************************
 *      GetCharWidthFloatA [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatA(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
       FIXME_(gdi)("GetCharWidthFloatA, stub\n");
       return 0;
}

/*************************************************************************
 *      GetCharWidthFloatW [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatW(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
       FIXME_(gdi)("GetCharWidthFloatW, stub\n");
       return 0;
}
 

/***********************************************************************
 *								       *
 *           Font Resource API					       *
 *								       *
 ***********************************************************************/
/***********************************************************************
 *           AddFontResource    (GDI.119)
 *
 *  Can be either .FON, or .FNT, or .TTF, or .FOT font file.
 *
 *  FIXME: Load header and find the best-matching font in the fontList;
 * 	   fixup dfPoints if all metrics are identical, otherwise create
 *	   new fontAlias. When soft font support is ready this will
 *	   simply create a new fontResource ('filename' will go into
 *	   the pfr->resource field) with FR_SOFTFONT/FR_SOFTRESOURCE 
 *	   flag set. 
 */
INT16 WINAPI AddFontResource16( LPCSTR filename )
{
    return AddFontResourceA( filename );
}


/***********************************************************************
 *           AddFontResourceA    (GDI32.@)
 */
INT WINAPI AddFontResourceA( LPCSTR str )
{
    FIXME("(%s): stub! Read the Wine User Guide on how to install "
            "this font manually.\n", debugres_a(str));
    return 1;
}


/***********************************************************************
 *           AddFontResourceW    (GDI32.@)
 */
INT WINAPI AddFontResourceW( LPCWSTR str )
{
    FIXME("(%s): stub! Read the Wine User Guide on how to install "
            "this font manually.\n", debugres_w(str));
    return 1;
}

/***********************************************************************
 *           RemoveFontResource    (GDI.136)
 */
BOOL16 WINAPI RemoveFontResource16( LPCSTR str )
{
    FIXME("(%s): stub\n", debugres_a(str));
    return TRUE;
}


/***********************************************************************
 *           RemoveFontResourceA    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceA( LPCSTR str )
{
/*  This is how it should look like */
/*
    fontResource** ppfr;
    BOOL32 retVal = FALSE;

    EnterCriticalSection( &crtsc_fonts_X11 );
    for( ppfr = &fontList; *ppfr; ppfr = &(*ppfr)->next )
	 if( !strcasecmp( (*ppfr)->lfFaceName, str ) )
	 {
	     if(((*ppfr)->fr_flags & (FR_SOFTFONT | FR_SOFTRESOURCE)) &&
		 (*ppfr)->hOwnerProcess == GetCurrentProcess() )
	     {
		 if( (*ppfr)->fo_count )
		     (*ppfr)->fr_flags |= FR_REMOVED;
		 else
		     XFONT_RemoveFontResource( ppfr );
	     }
	     retVal = TRUE;
	 }
    LeaveCriticalSection( &crtsc_fonts_X11 );
    return retVal;
 */
    FIXME("(%s): stub\n", debugres_a(str));
    return TRUE;
}


/***********************************************************************
 *           RemoveFontResourceW    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceW( LPCWSTR str )
{
    FIXME("(%s): stub\n", debugres_w(str) );
    return TRUE;
}
