/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "font.h"
#include "wine/debug.h"
#include "gdi.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);
WINE_DECLARE_DEBUG_CHANNEL(gdi);

static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, void *obj, HDC hdc );
static INT FONT_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static INT FONT_GetObjectA( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static INT FONT_GetObjectW( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static BOOL FONT_DeleteObject( HGDIOBJ handle, void *obj );

static const struct gdi_obj_funcs font_funcs =
{
    FONT_SelectObject,  /* pSelectObject */
    FONT_GetObject16,   /* pGetObject16 */
    FONT_GetObjectA,    /* pGetObjectA */
    FONT_GetObjectW,    /* pGetObjectW */
    NULL,               /* pUnrealizeObject */
    FONT_DeleteObject   /* pDeleteObject */
};

#define ENUM_UNICODE	0x00000001
#define ENUM_CALLED     0x00000002

typedef struct
{
    GDIOBJHDR   header;
    LOGFONTW    logfont;
} FONTOBJ;

typedef struct
{
  LPLOGFONT16           lpLogFontParam;
  FONTENUMPROCEX16      lpEnumFunc;
  LPARAM                lpData;

  LPNEWTEXTMETRICEX16   lpTextMetric;
  LPENUMLOGFONTEX16     lpLogFont;
  SEGPTR                segTextMetric;
  SEGPTR                segLogFont;
  HDC                   hdc;
  DC                   *dc;
  PHYSDEV               physDev;
} fontEnum16;

typedef struct
{
  LPLOGFONTW          lpLogFontParam;
  FONTENUMPROCEXW     lpEnumFunc;
  LPARAM              lpData;
  DWORD               dwFlags;
  HDC                 hdc;
  DC                 *dc;
  PHYSDEV             physDev;
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
  { VIETNAMESE_CHARSET, 1258, FS(8)},
  /* reserved by ANSI */
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
  { SYMBOL_CHARSET, CP_SYMBOL, FS(31)},
};

/* ### start build ### */
extern WORD CALLBACK FONT_CallTo16_word_llwl(FONTENUMPROCEX16,LONG,LONG,WORD,LONG);
/* ### stop build ### */

/***********************************************************************
 *              LOGFONT conversion functions.
 */
static void FONT_LogFontWTo16( const LOGFONTW* font32, LPLOGFONT16 font16 )
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

static void FONT_LogFont16ToW( const LOGFONT16 *font16, LPLOGFONTW font32 )
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

static void FONT_LogFontAToW( const LOGFONTA *fontA, LPLOGFONTW fontW )
{
    memcpy(fontW, fontA, sizeof(LOGFONTA) - LF_FACESIZE);
    MultiByteToWideChar(CP_ACP, 0, fontA->lfFaceName, -1, fontW->lfFaceName,
			LF_FACESIZE);
}

static void FONT_LogFontWToA( const LOGFONTW *fontW, LPLOGFONTA fontA )
{
    memcpy(fontA, fontW, sizeof(LOGFONTA) - LF_FACESIZE);
    WideCharToMultiByte(CP_ACP, 0, fontW->lfFaceName, -1, fontA->lfFaceName,
			LF_FACESIZE, NULL, NULL);
}

static void FONT_EnumLogFontExWTo16( const ENUMLOGFONTEXW *fontW, LPENUMLOGFONTEX16 font16 )
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

static void FONT_EnumLogFontExWToA( const ENUMLOGFONTEXW *fontW, LPENUMLOGFONTEXA fontA )
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
static void FONT_TextMetricWToA(const TEXTMETRICW *ptmW, LPTEXTMETRICA ptmA )
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


static void FONT_NewTextMetricExWTo16(const NEWTEXTMETRICEXW *ptmW, LPNEWTEXTMETRICEX16 ptm16 )
{
    ptm16->ntmTm.tmHeight = ptmW->ntmTm.tmHeight;
    ptm16->ntmTm.tmAscent = ptmW->ntmTm.tmAscent;
    ptm16->ntmTm.tmDescent = ptmW->ntmTm.tmDescent;
    ptm16->ntmTm.tmInternalLeading = ptmW->ntmTm.tmInternalLeading;
    ptm16->ntmTm.tmExternalLeading = ptmW->ntmTm.tmExternalLeading;
    ptm16->ntmTm.tmAveCharWidth = ptmW->ntmTm.tmAveCharWidth;
    ptm16->ntmTm.tmMaxCharWidth = ptmW->ntmTm.tmMaxCharWidth;
    ptm16->ntmTm.tmWeight = ptmW->ntmTm.tmWeight;
    ptm16->ntmTm.tmOverhang = ptmW->ntmTm.tmOverhang;
    ptm16->ntmTm.tmDigitizedAspectX = ptmW->ntmTm.tmDigitizedAspectX;
    ptm16->ntmTm.tmDigitizedAspectY = ptmW->ntmTm.tmDigitizedAspectY;
    ptm16->ntmTm.tmFirstChar = ptmW->ntmTm.tmFirstChar;
    ptm16->ntmTm.tmLastChar = ptmW->ntmTm.tmLastChar;
    ptm16->ntmTm.tmDefaultChar = ptmW->ntmTm.tmDefaultChar;
    ptm16->ntmTm.tmBreakChar = ptmW->ntmTm.tmBreakChar;
    ptm16->ntmTm.tmItalic = ptmW->ntmTm.tmItalic;
    ptm16->ntmTm.tmUnderlined = ptmW->ntmTm.tmUnderlined;
    ptm16->ntmTm.tmStruckOut = ptmW->ntmTm.tmStruckOut;
    ptm16->ntmTm.tmPitchAndFamily = ptmW->ntmTm.tmPitchAndFamily;
    ptm16->ntmTm.tmCharSet = ptmW->ntmTm.tmCharSet;
    ptm16->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptm16->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptm16->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptm16->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    memcpy(&ptm16->ntmFontSig, &ptmW->ntmFontSig, sizeof(FONTSIGNATURE));
}

static void FONT_NewTextMetricExWToA(const NEWTEXTMETRICEXW *ptmW, LPNEWTEXTMETRICEXA ptmA )
{
    FONT_TextMetricWToA((LPTEXTMETRICW)ptmW, (LPTEXTMETRICA)ptmA);
    ptmA->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptmA->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptmA->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptmA->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    memcpy(&ptmA->ntmFontSig, &ptmW->ntmFontSig, sizeof(FONTSIGNATURE));
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
	if ((fontPtr = GDI_AllocObject( sizeof(FONTOBJ), FONT_MAGIC, &hFont, &font_funcs )))
	{
	    memcpy( &fontPtr->logfont, plf, sizeof(LOGFONTW) );

	    TRACE("(%ld %ld %ld %ld %x) %s %s %s => %04x\n",
                  plf->lfHeight, plf->lfWidth,
                  plf->lfEscapement, plf->lfOrientation,
                  plf->lfPitchAndFamily,
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
 *           FONT_SelectObject
 *
 * If the driver supports vector fonts we create a gdi font first and
 * then call the driver to give it a chance to supply its own device
 * font.  If the driver wants to do this it returns TRUE and we can
 * delete the gdi font, if the driver wants to use the gdi font it
 * should return FALSE, to signal an error return GDI_ERROR.  For
 * drivers that don't support vector fonts they must supply their own
 * font.
 */
static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, void *obj, HDC hdc )
{
    HGDIOBJ ret = 0;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;

    if (dc->hFont != handle || dc->gdiFont == NULL)
    {
        if(GetDeviceCaps(dc->hSelf, TEXTCAPS) & TC_VA_ABLE)
            dc->gdiFont = WineEngCreateFontInstance(dc, handle);
    }

    if (dc->funcs->pSelectFont) ret = dc->funcs->pSelectFont( dc->physDev, handle );

    if (ret && dc->gdiFont) dc->gdiFont = 0;

    if (ret == HGDI_ERROR)
        ret = 0; /* SelectObject returns 0 on error */
    else
    {
        ret = dc->hFont;
        dc->hFont = handle;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           FONT_GetObject16
 */
static INT FONT_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    FONTOBJ *font = obj;
    LOGFONT16 lf16;

    FONT_LogFontWTo16( &font->logfont, &lf16 );

    if (count > sizeof(LOGFONT16)) count = sizeof(LOGFONT16);
    memcpy( buffer, &lf16, count );
    return count;
}

/***********************************************************************
 *           FONT_GetObjectA
 */
static INT FONT_GetObjectA( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    FONTOBJ *font = obj;
    LOGFONTA lfA;

    FONT_LogFontWToA( &font->logfont, &lfA );

    if (count > sizeof(lfA)) count = sizeof(lfA);
    if(buffer)
        memcpy( buffer, &lfA, count );
    return count;
}

/***********************************************************************
 *           FONT_GetObjectW
 */
static INT FONT_GetObjectW( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    FONTOBJ *font = obj;
    if (count > sizeof(LOGFONTW)) count = sizeof(LOGFONTW);
    if(buffer)
        memcpy( buffer, &font->logfont, count );
    return count;
}


/***********************************************************************
 *           FONT_DeleteObject
 */
static BOOL FONT_DeleteObject( HGDIOBJ handle, void *obj )
{
    WineEngDestroyFontInstance( handle );
    return GDI_FreeObject( handle, obj );
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
    fontEnum16 *pfe = (fontEnum16*)lp;
    INT ret = 1;
    DC *dc;

    if( pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET ||
	pfe->lpLogFontParam->lfCharSet == plf->elfLogFont.lfCharSet )
    {
        FONT_EnumLogFontExWTo16(plf, pfe->lpLogFont);
	FONT_NewTextMetricExWTo16(ptm, pfe->lpTextMetric);
        GDI_ReleaseObj( pfe->hdc );  /* release the GDI lock */

        ret = FONT_CallTo16_word_llwl( pfe->lpEnumFunc, pfe->segLogFont, pfe->segTextMetric,
                                       (UINT16)fType, (LPARAM)pfe->lpData );
        /* get the lock again and make sure the DC is still valid */
        dc = DC_GetDCPtr( pfe->hdc );
        if (!dc || dc != pfe->dc || dc->physDev != pfe->physDev)
        {
            if (dc) GDI_ReleaseObj( pfe->hdc );
            pfe->hdc = 0;  /* make sure we don't try to release it later on */
            ret = 0;
        }
    }
    return ret;
}

/***********************************************************************
 *              FONT_EnumInstance
 */
static INT FONT_EnumInstance( LPENUMLOGFONTEXW plf, LPNEWTEXTMETRICEXW ptm,
			      DWORD fType, LPARAM lp )
{
    fontEnum32 *pfe = (fontEnum32*)lp;
    INT ret = 1;
    DC *dc;

    /* lfCharSet is at the same offset in both LOGFONTA and LOGFONTW */
    if( pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET ||
	pfe->lpLogFontParam->lfCharSet == plf->elfLogFont.lfCharSet )
    {
	/* convert font metrics */
        ENUMLOGFONTEXA logfont;
        NEWTEXTMETRICEXA tmA;

        pfe->dwFlags |= ENUM_CALLED;
        if (!(pfe->dwFlags & ENUM_UNICODE))
        {
            FONT_EnumLogFontExWToA( plf, &logfont);
            FONT_NewTextMetricExWToA( ptm, &tmA );
            plf = (LPENUMLOGFONTEXW)&logfont;
            ptm = (LPNEWTEXTMETRICEXW)&tmA;
        }
        GDI_ReleaseObj( pfe->hdc );  /* release the GDI lock */

        ret = pfe->lpEnumFunc( plf, ptm, fType, pfe->lpData );

        /* get the lock again and make sure the DC is still valid */
        dc = DC_GetDCPtr( pfe->hdc );
        if (!dc || dc != pfe->dc || dc->physDev != pfe->physDev)
        {
            if (dc) GDI_ReleaseObj( pfe->hdc );
            pfe->hdc = 0;  /* make sure we don't try to release it later on */
            ret = 0;
        }
    }
    return ret;
}

/***********************************************************************
 *              EnumFontFamiliesEx	(GDI.613)
 */
INT16 WINAPI EnumFontFamiliesEx16( HDC16 hDC, LPLOGFONT16 plf,
                                   FONTENUMPROCEX16 efproc, LPARAM lParam,
                                   DWORD dwFlags)
{
    fontEnum16 fe16;
    INT16	retVal = 0;
    DC* 	dc = DC_GetDCPtr( hDC );

    if (!dc) return 0;
    fe16.hdc = hDC;
    fe16.dc = dc;
    fe16.physDev = dc->physDev;

    if (dc->funcs->pEnumDeviceFonts)
    {
        NEWTEXTMETRICEX16 tm16;
        ENUMLOGFONTEX16 lf16;
        LOGFONTW lfW;
        FONT_LogFont16ToW(plf, &lfW);

        fe16.lpLogFontParam = plf;
        fe16.lpEnumFunc = efproc;
        fe16.lpData = lParam;
        fe16.lpTextMetric = &tm16;
        fe16.lpLogFont = &lf16;
        fe16.segTextMetric = MapLS( &tm16 );
        fe16.segLogFont = MapLS( &lf16 );

        retVal = dc->funcs->pEnumDeviceFonts( dc->physDev, &lfW,
                                              FONT_EnumInstance16, (LPARAM)&fe16 );
        UnMapLS( fe16.segTextMetric );
        UnMapLS( fe16.segLogFont );
    }
    if (fe16.hdc) GDI_ReleaseObj( fe16.hdc );
    return retVal;
}

/***********************************************************************
 *		FONT_EnumFontFamiliesEx
 */
static INT FONT_EnumFontFamiliesEx( HDC hDC, LPLOGFONTW plf,
				    FONTENUMPROCEXW efproc,
				    LPARAM lParam, DWORD dwUnicode)
{
    INT ret = 1, ret2;
    DC *dc = DC_GetDCPtr( hDC );
    fontEnum32 fe32;
    BOOL enum_gdi_fonts;

    if (!dc) return 0;

    TRACE("lfFaceName = %s lfCharset = %d\n", debugstr_w(plf->lfFaceName),
	  plf->lfCharSet);
    fe32.lpLogFontParam = plf;
    fe32.lpEnumFunc = efproc;
    fe32.lpData = lParam;
    fe32.dwFlags = dwUnicode;
    fe32.hdc = hDC;
    fe32.dc = dc;
    fe32.physDev = dc->physDev;

    enum_gdi_fonts = GetDeviceCaps(hDC, TEXTCAPS) & TC_VA_ABLE;

    if (!dc->funcs->pEnumDeviceFonts && !enum_gdi_fonts)
    {
        ret = 0;
        goto done;
    }

    if (enum_gdi_fonts)
        ret = WineEngEnumFonts( plf, FONT_EnumInstance, (LPARAM)&fe32 );
    fe32.dwFlags &= ~ENUM_CALLED;
    if (ret && dc->funcs->pEnumDeviceFonts) {
	ret2 = dc->funcs->pEnumDeviceFonts( dc->physDev, plf, FONT_EnumInstance, (LPARAM)&fe32 );
	if(fe32.dwFlags & ENUM_CALLED) /* update ret iff a font gets enumed */
	    ret = ret2;
    }
 done:
    if (fe32.hdc) GDI_ReleaseObj( fe32.hdc );
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
 *              EnumFontFamilies	(GDI.330)
 */
INT16 WINAPI EnumFontFamilies16( HDC16 hDC, LPCSTR lpFamily,
                                 FONTENUMPROC16 efproc, LPARAM lpData )
{
    LOGFONT16	lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = '\0';

    return EnumFontFamiliesEx16( hDC, &lf, efproc, lpData, 0 );
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
 *              EnumFonts		(GDI.70)
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
 *           SetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI SetTextCharacterExtra( HDC hdc, INT extra )
{
    INT prev;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetTextCharacterExtra)
        prev = dc->funcs->pSetTextCharacterExtra( dc->physDev, extra );
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
 *           SetTextJustification    (GDI32.@)
 */
BOOL WINAPI SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    BOOL ret = TRUE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetTextJustification)
        ret = dc->funcs->pSetTextJustification( dc->physDev, extra, breaks );
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

    if(dc->gdiFont)
        ret = WineEngGetTextFace(dc->gdiFont, count, name);
    else if ((font = (FONTOBJ *) GDI_GetObjPtr( dc->hFont, FONT_MAGIC )))
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

    if(dc->gdiFont) {
        ret = WineEngGetTextExtentPoint(dc->gdiFont, str, count, size);
	size->cx = abs(INTERNAL_XDSTOWS(dc, size->cx));
	size->cy = abs(INTERNAL_YDSTOWS(dc, size->cy));
    }
    else if(dc->funcs->pGetTextExtentPoint)
        ret = dc->funcs->pGetTextExtentPoint( dc->physDev, str, count, size );

    GDI_ReleaseObj( hdc );

    TRACE("(%08x %s %d %p): returning %ld x %ld\n",
          hdc, debugstr_wn (str, count), count, size, size->cx, size->cy );
    return ret;
}

/***********************************************************************
 * GetTextExtentPointI [GDI32.@]
 *
 * Computes width and height of the array of glyph indices.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentPointI(
    HDC hdc,     /* [in]  Handle of device context */
    const WORD *indices,   /* [in]  Address of glyph index array */
    INT count,   /* [in]  Number of glyphs in array */
    LPSIZE size) /* [out] Address of structure for string size */
{
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    if(dc->gdiFont) {
        ret = WineEngGetTextExtentPointI(dc->gdiFont, indices, count, size);
	size->cx = abs(INTERNAL_XDSTOWS(dc, size->cx));
	size->cy = abs(INTERNAL_YDSTOWS(dc, size->cy));
    }
    else if(dc->funcs->pGetTextExtentPoint) {
        FIXME("calling GetTextExtentPoint\n");
        ret = dc->funcs->pGetTextExtentPoint( dc->physDev, (LPCWSTR)indices, count, size );
    }

    GDI_ReleaseObj( hdc );

    TRACE("(%08x %p %d %p): returning %ld x %ld\n",
          hdc, indices, count, size, size->cx, size->cy );
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
        ret = dc->funcs->pGetTextMetrics( dc->physDev, metrics );

    if (ret)
    {
    /* device layer returns values in device units
     * therefore we have to convert them to logical */

#define WDPTOLP(x) ((x<0)?					\
		(-abs(INTERNAL_XDSTOWS(dc, (x)))):		\
		(abs(INTERNAL_XDSTOWS(dc, (x)))))
#define HDPTOLP(y) ((y<0)?					\
		(-abs(INTERNAL_YDSTOWS(dc, (y)))):		\
		(abs(INTERNAL_YDSTOWS(dc, (y)))))

    metrics->tmHeight           = HDPTOLP(metrics->tmHeight);
    metrics->tmAscent           = HDPTOLP(metrics->tmAscent);
    metrics->tmDescent          = HDPTOLP(metrics->tmDescent);
    metrics->tmInternalLeading  = HDPTOLP(metrics->tmInternalLeading);
    metrics->tmExternalLeading  = HDPTOLP(metrics->tmExternalLeading);
    metrics->tmAveCharWidth     = WDPTOLP(metrics->tmAveCharWidth);
    metrics->tmMaxCharWidth     = WDPTOLP(metrics->tmMaxCharWidth);
    metrics->tmOverhang         = WDPTOLP(metrics->tmOverhang);
        ret = TRUE;
#undef WDPTOLP
#undef HDPTOLP
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

    if(dc->gdiFont) {
        ret = WineEngGetOutlineTextMetrics(dc->gdiFont, cbData, lpOTM);
	if(ret && ret <= cbData) {
#define WDPTOLP(x) ((x<0)?					\
		(-abs(INTERNAL_XDSTOWS(dc, (x)))):		\
		(abs(INTERNAL_XDSTOWS(dc, (x)))))
#define HDPTOLP(y) ((y<0)?					\
		(-abs(INTERNAL_YDSTOWS(dc, (y)))):		\
		(abs(INTERNAL_YDSTOWS(dc, (y)))))

	    lpOTM->otmTextMetrics.tmHeight           = HDPTOLP(lpOTM->otmTextMetrics.tmHeight);
	    lpOTM->otmTextMetrics.tmAscent           = HDPTOLP(lpOTM->otmTextMetrics.tmAscent);
	    lpOTM->otmTextMetrics.tmDescent          = HDPTOLP(lpOTM->otmTextMetrics.tmDescent);
	    lpOTM->otmTextMetrics.tmInternalLeading  = HDPTOLP(lpOTM->otmTextMetrics.tmInternalLeading);
	    lpOTM->otmTextMetrics.tmExternalLeading  = HDPTOLP(lpOTM->otmTextMetrics.tmExternalLeading);
	    lpOTM->otmTextMetrics.tmAveCharWidth     = WDPTOLP(lpOTM->otmTextMetrics.tmAveCharWidth);
	    lpOTM->otmTextMetrics.tmMaxCharWidth     = WDPTOLP(lpOTM->otmTextMetrics.tmMaxCharWidth);
	    lpOTM->otmTextMetrics.tmOverhang         = WDPTOLP(lpOTM->otmTextMetrics.tmOverhang);
	    lpOTM->otmAscent = HDPTOLP(lpOTM->otmAscent);
	    lpOTM->otmDescent = HDPTOLP(lpOTM->otmDescent);
	    lpOTM->otmLineGap = HDPTOLP(lpOTM->otmLineGap);
	    lpOTM->otmsCapEmHeight = HDPTOLP(lpOTM->otmsCapEmHeight);
	    lpOTM->otmsXHeight = HDPTOLP(lpOTM->otmsXHeight);
	    lpOTM->otmrcFontBox.top = HDPTOLP(lpOTM->otmrcFontBox.top);
	    lpOTM->otmrcFontBox.bottom = HDPTOLP(lpOTM->otmrcFontBox.bottom);
	    lpOTM->otmrcFontBox.left = WDPTOLP(lpOTM->otmrcFontBox.left);
	    lpOTM->otmrcFontBox.right = WDPTOLP(lpOTM->otmrcFontBox.right);
	    lpOTM->otmMacAscent = HDPTOLP(lpOTM->otmMacAscent);
	    lpOTM->otmMacDescent = HDPTOLP(lpOTM->otmMacDescent);
	    lpOTM->otmMacLineGap = HDPTOLP(lpOTM->otmMacLineGap);
	    lpOTM->otmptSubscriptSize.x = WDPTOLP(lpOTM->otmptSubscriptSize.x);
	    lpOTM->otmptSubscriptSize.y = HDPTOLP(lpOTM->otmptSubscriptSize.y);
	    lpOTM->otmptSubscriptOffset.x = WDPTOLP(lpOTM->otmptSubscriptOffset.x);
	    lpOTM->otmptSubscriptOffset.y = HDPTOLP(lpOTM->otmptSubscriptOffset.y);
	    lpOTM->otmptSuperscriptSize.x = WDPTOLP(lpOTM->otmptSuperscriptSize.x);
	    lpOTM->otmptSuperscriptSize.y = HDPTOLP(lpOTM->otmptSuperscriptSize.y);
	    lpOTM->otmptSuperscriptOffset.x = WDPTOLP(lpOTM->otmptSuperscriptOffset.x);
	    lpOTM->otmptSuperscriptOffset.y = HDPTOLP(lpOTM->otmptSuperscriptOffset.y);
	    lpOTM->otmsStrikeoutSize = HDPTOLP(lpOTM->otmsStrikeoutSize);
	    lpOTM->otmsStrikeoutPosition = HDPTOLP(lpOTM->otmsStrikeoutPosition);
	    lpOTM->otmsUnderscoreSize = HDPTOLP(lpOTM->otmsUnderscoreSize);
	    lpOTM->otmsUnderscorePosition = HDPTOLP(lpOTM->otmsUnderscorePosition);
#undef WDPTOLP
#undef HDPTOLP
	}
    }

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
 *           GetCharWidthW      (GDI32.@)
 *           GetCharWidth32W    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32W( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    UINT i, extra;
    BOOL ret = FALSE;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    if (dc->gdiFont)
        ret = WineEngGetCharWidth( dc->gdiFont, firstChar, lastChar, buffer );
    else if (dc->funcs->pGetCharWidth)
        ret = dc->funcs->pGetCharWidth( dc->physDev, firstChar, lastChar, buffer);

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
 *           GetCharWidthA      (GDI32.@)
 *           GetCharWidth32A    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32A( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    INT i, wlen, count = (INT)(lastChar - firstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(firstChar + i);

    wstr = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    for(i = 0; i < wlen; i++)
    {
	if(!GetCharWidth32W(hdc, wstr[i], wstr[i], buffer))
	{
	    ret = FALSE;
	    break;
	}
	buffer++;
    }

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}


/* FIXME: all following APIs ******************************************/


/***********************************************************************
 *           SetMapperFlags    (GDI32.@)
 */
DWORD WINAPI SetMapperFlags( HDC hDC, DWORD dwFlag )
{
    DC *dc = DC_GetDCPtr( hDC );
    DWORD ret = 0;
    if(!dc) return 0;
    if(dc->funcs->pSetMapperFlags)
        ret = dc->funcs->pSetMapperFlags( dc->physDev, dwFlag );
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
 *           GetCharABCWidthsA   (GDI32.@)
 */
BOOL WINAPI GetCharABCWidthsA(HDC hdc, UINT firstChar, UINT lastChar,
                                  LPABC abc )
{
    INT i, wlen, count = (INT)(lastChar - firstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(firstChar + i);

    wstr = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    for(i = 0; i < wlen; i++)
    {
	if(!GetCharABCWidthsW(hdc, wstr[i], wstr[i], abc))
	{
	    ret = FALSE;
	    break;
	}
	abc++;
    }

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
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
    DC *dc = DC_GetDCPtr(hdc);
    int		i;
    GLYPHMETRICS gm;
    BOOL ret = FALSE;

    if(dc->gdiFont) {
        for (i=firstChar;i<=lastChar;i++) {
	    GetGlyphOutlineW(hdc, i, GGO_METRICS, &gm, 0, NULL, NULL);
	    abc[i-firstChar].abcA = gm.gmptGlyphOrigin.x;
	    abc[i-firstChar].abcB = gm.gmBlackBoxX;
	    abc[i-firstChar].abcC = gm.gmCellIncX - gm.gmptGlyphOrigin.x - gm.gmBlackBoxX;
	}
	ret = TRUE;
    }
    GDI_ReleaseObj(hdc);
    return ret;
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
    LPWSTR p = NULL;
    DWORD ret;
    UINT c;

    if(!(fuFormat & GGO_GLYPH_INDEX)) {
        p = FONT_mbtowc(hdc, (char*)&uChar, 1, NULL, NULL);
	c = p[0];
    } else
        c = uChar;
    ret = GetGlyphOutlineW(hdc, c, fuFormat, lpgm, cbBuffer, lpBuffer,
			   lpmat2);
    if(p)
        HeapFree(GetProcessHeap(), 0, p);
    return ret;
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

    TRACE("(%04x, %04x, %04x, %p, %ld, %p, %p)\n",
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
 *           CreateScalableFontResourceA   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceA( DWORD fHidden,
                                             LPCSTR lpszResourceFile,
                                             LPCSTR lpszFontFile,
                                             LPCSTR lpszCurrentPath )
{
    HANDLE f;

    /* fHidden=1 - only visible for the calling app, read-only, not
     * enumbered with EnumFonts/EnumFontFamilies
     * lpszCurrentPath can be NULL
     */
    FIXME("(%ld,%s,%s,%s): stub\n",
          fHidden, debugstr_a(lpszResourceFile), debugstr_a(lpszFontFile),
          debugstr_a(lpszCurrentPath) );

    /* If the output file already exists, return the ERROR_FILE_EXISTS error as specified in MSDN */
    if ((f = CreateFileA(lpszResourceFile, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE) {
        CloseHandle(f);
        SetLastError(ERROR_FILE_EXISTS);
        return FALSE;
    }
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
 *             GetKerningPairsA   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsA( HDC hDC, DWORD cPairs, LPKERNINGPAIR lpKerningPairs )
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
DWORD WINAPI GetFontLanguageInfo(HDC hdc)
{
	FONTSIGNATURE fontsig;
	static const DWORD GCP_DBCS_MASK=0x003F0000,
		GCP_DIACRITIC_MASK=0x00000000,
		FLI_GLYPHS_MASK=0x00000000,
		GCP_GLYPHSHAPE_MASK=0x00000040,
		GCP_KASHIDA_MASK=0x00000000,
		GCP_LIGATE_MASK=0x00000000,
		GCP_USEKERNING_MASK=0x00000000,
		GCP_REORDER_MASK=0x00000060;

	DWORD result=0;

	GetTextCharsetInfo( hdc, &fontsig, 0 );
	/* We detect each flag we return using a bitmask on the Codepage Bitfields */

	if( (fontsig.fsCsb[0]&GCP_DBCS_MASK)!=0 )
		result|=GCP_DBCS;

	if( (fontsig.fsCsb[0]&GCP_DIACRITIC_MASK)!=0 )
		result|=GCP_DIACRITIC;

	if( (fontsig.fsCsb[0]&FLI_GLYPHS_MASK)!=0 )
		result|=FLI_GLYPHS;

	if( (fontsig.fsCsb[0]&GCP_GLYPHSHAPE_MASK)!=0 )
		result|=GCP_GLYPHSHAPE;

	if( (fontsig.fsCsb[0]&GCP_KASHIDA_MASK)!=0 )
		result|=GCP_KASHIDA;

	if( (fontsig.fsCsb[0]&GCP_LIGATE_MASK)!=0 )
		result|=GCP_LIGATE;

	if( (fontsig.fsCsb[0]&GCP_USEKERNING_MASK)!=0 )
		result|=GCP_USEKERNING;

	if( (fontsig.fsCsb[0]&GCP_REORDER_MASK)!=0 )
		result|=GCP_REORDER;

	return result;
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
 * GetGlyphIndicesA [GDI32.@]
 */
DWORD WINAPI GetGlyphIndicesA(HDC hdc, LPCSTR lpstr, INT count,
			      LPWORD pgi, DWORD flags)
{
    DWORD ret;
    WCHAR *lpstrW;
    INT countW;

    TRACE("(%04x, %s, %d, %p, 0x%lx)\n",
	hdc, debugstr_an(lpstr, count), count, pgi, flags);

    lpstrW = FONT_mbtowc(hdc, lpstr, count, &countW, NULL);
    ret = GetGlyphIndicesW(hdc, lpstrW, countW, pgi, flags);
    HeapFree(GetProcessHeap(), 0, lpstrW);

    return ret;
}

/*************************************************************************
 * GetGlyphIndicesW [GDI32.@]
 */
DWORD WINAPI GetGlyphIndicesW(HDC hdc, LPCWSTR lpstr, INT count,
			      LPWORD pgi, DWORD flags)
{
    DC *dc = DC_GetDCPtr(hdc);
    DWORD ret = GDI_ERROR;

    TRACE("(%04x, %s, %d, %p, 0x%lx)\n",
	hdc, debugstr_wn(lpstr, count), count, pgi, flags);

    if(!dc) return GDI_ERROR;

    if(dc->gdiFont)
	ret = WineEngGetGlyphIndices(dc->gdiFont, lpstr, count, pgi, flags);

    GDI_ReleaseObj(hdc);
    return ret;
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
    WCHAR *lpStringW;
    INT uCountW, i;
    GCP_RESULTSW resultsW;
    DWORD ret;
    UINT font_cp;

    TRACE("%s, %d, %d, 0x%08lx\n",
          debugstr_an(lpString, uCount), uCount, nMaxExtent, dwFlags);

    /* both structs are equal in size */
    memcpy(&resultsW, lpResults, sizeof(resultsW));

    lpStringW = FONT_mbtowc(hdc, lpString, uCount, &uCountW, &font_cp);
    if(lpResults->lpOutString)
        resultsW.lpOutString = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*uCountW);

    ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, &resultsW, dwFlags);

    if(lpResults->lpOutString) {
        if(font_cp != CP_SYMBOL)
            WideCharToMultiByte(font_cp, 0, resultsW.lpOutString, uCountW,
                                lpResults->lpOutString, uCount, NULL, NULL );
        else
            for(i = 0; i < uCount; i++)
                lpResults->lpOutString[i] = (CHAR)resultsW.lpOutString[i];
    }

    HeapFree(GetProcessHeap(), 0, lpStringW);
    HeapFree(GetProcessHeap(), 0, resultsW.lpOutString);

    return ret;
}

/*************************************************************************
 * GetCharacterPlacementW [GDI32.@]
 *
 *   Retrieve information about a string. This includes the width, reordering,
 *   Glyphing and so on.
 *
 * RETURNS
 *
 *   The width and height of the string if succesful, 0 if failed.
 *
 * BUGS
 *
 *   All flags except GCP_REORDER are not yet implemented.
 *   Reordering is not 100% complient to the Windows BiDi method.
 *   Caret positioning is not yet implemented.
 *   Classes are not yet implemented.
 *
 */
DWORD WINAPI
GetCharacterPlacementW(
		HDC hdc,		/* [in] Device context for which the rendering is to be done */
		LPCWSTR lpString,	/* [in] The string for which information is to be returned */
		INT uCount,		/* [in] Number of WORDS in string. */
		INT nMaxExtent,		/* [in] Maximum extent the string is to take (in HDC logical units) */
		GCP_RESULTSW *lpResults,/* [in/out] A pointer to a GCP_RESULTSW struct */
		DWORD dwFlags 		/* [in] Flags specifying how to process the string */
		)
{
    DWORD ret=0;
    SIZE size;
    UINT i, nSet;

    TRACE("%s, %d, %d, 0x%08lx\n",
          debugstr_wn(lpString, uCount), uCount, nMaxExtent, dwFlags);

    TRACE("lStructSize=%ld, lpOutString=%p, lpOrder=%p, lpDx=%p, lpCaretPos=%p\n"
	  "lpClass=%p, lpGlyphs=%p, nGlyphs=%u, nMaxFit=%d\n",
	    lpResults->lStructSize, lpResults->lpOutString, lpResults->lpOrder,
	    lpResults->lpDx, lpResults->lpCaretPos, lpResults->lpClass,
	    lpResults->lpGlyphs, lpResults->nGlyphs, lpResults->nMaxFit);

    if(dwFlags&(~GCP_REORDER))			FIXME("flags 0x%08lx ignored\n", dwFlags);
    if(lpResults->lpCaretPos)	FIXME("caret positions not implemented\n");
    if(lpResults->lpClass)	FIXME("classes not implemented\n");

	nSet = (UINT)uCount;
	if(nSet > lpResults->nGlyphs)
		nSet = lpResults->nGlyphs;

	/* return number of initialized fields */
	lpResults->nGlyphs = nSet;

	if(dwFlags==0)
	{
		/* Treat the case where no special handling was requested in a fastpath way */
		/* copy will do if the GCP_REORDER flag is not set */
		if(lpResults->lpOutString)
			for(i=0; i<nSet && lpString[i]!=0; ++i )
				lpResults->lpOutString[i]=lpString[i];

		if(lpResults->lpOrder)
		{
			for(i = 0; i < nSet; i++)
				lpResults->lpOrder[i] = i;
		}
	}

	if((dwFlags&GCP_REORDER)!=0)
	{
		WORD *pwCharType;
		int run_end;
		/* Keep a static table that translates the C2 types to something meaningful */
		/* 1 - left to right
		 * -1 - right to left
		 * 0 - neutral
		 */
		static const int chardir[]={ 0, 1, -1, 1, 1, 1, -1, 1, 0, 0, 0, 0 };

		WARN("The BiDi algorythm doesn't conform to Windows' yet\n");
		if( (pwCharType=HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WORD)))==NULL )
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);

			return 0;
		}

		/* Fill in the order array with directionality values */
		GetStringTypeW(CT_CTYPE2, lpString, uCount, pwCharType);

		/* The complete and correct (at least according to MS) BiDi algorythm is not
		 * yet implemented here. Instead, we just make sure that consecutive runs of
		 * the same direction (or neutral) are ordered correctly. We also assign Neutrals
		 * that are between runs of opposing directions the base (ok, always LTR) dir.
		 * While this is a LONG way from a BiDi algorithm, it does produce more or less
		 * readable results.
		 */
		for( i=0; i<uCount; i+=run_end )
		{
			for( run_end=1; i+run_end<uCount &&
			     (chardir[pwCharType[i+run_end]]==chardir[pwCharType[i]] ||
			     chardir[pwCharType[i+run_end]]==0); ++run_end )
				;

			if( chardir[pwCharType[i]]==1 || chardir[pwCharType[i]]==0 )
			{
				/* A LTR run */
				if(lpResults->lpOutString)
				{
					int j;
					for( j=0; j<run_end; j++ )
					{
						lpResults->lpOutString[i+j]=lpString[i+j];
					}
				}

				if(lpResults->lpOrder)
				{
					int j;
					for( j=0; j<run_end; j++ )
						lpResults->lpOrder[i+j] = i+j;
				}
			} else
			{
				/* A RTL run */

				/* Since, at this stage, the paragraph context is always LTR,
				 * remove any neutrals from the end of this run.
				 */
				if( chardir[pwCharType[i]]!=0 )
					while( chardir[pwCharType[i+run_end-1]]==0 )
						--run_end;

				if(lpResults->lpOutString)
				{
					int j;
					for( j=0; j<run_end; j++ )
					{
						lpResults->lpOutString[i+j]=lpString[i+run_end-j-1];
					}
				}

				if(lpResults->lpOrder)
				{
					int j;
					for( j=0; j<run_end; j++ )
						lpResults->lpOrder[i+j] = i+run_end-j-1;
				}
			}
		}

		HeapFree(GetProcessHeap(), 0, pwCharType);
	}

	/* FIXME: Will use the placement chars */
	if (lpResults->lpDx)
	{
		int c;
		for (i = 0; i < nSet; i++)
		{
			if (GetCharWidth32W(hdc, lpString[i], lpString[i], &c))
				lpResults->lpDx[i]= c;
		}
	}

    if(lpResults->lpGlyphs)
	GetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);

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
 *           AddFontResourceA    (GDI32.@)
 */
INT WINAPI AddFontResourceA( LPCSTR str )
{
    return AddFontResourceExA( str, 0, NULL);
}

/***********************************************************************
 *           AddFontResourceW    (GDI32.@)
 */
INT WINAPI AddFontResourceW( LPCWSTR str )
{
    return AddFontResourceExW(str, 0, NULL);
}


/***********************************************************************
 *           AddFontResourceExA    (GDI32.@)
 */
INT WINAPI AddFontResourceExA( LPCSTR str, DWORD fl, PVOID pdv )
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len);
    ret = AddFontResourceExW(strW, fl, pdv);
    HeapFree(GetProcessHeap(), 0, strW);
    return ret;
}

/***********************************************************************
 *           AddFontResourceExW    (GDI32.@)
 */
INT WINAPI AddFontResourceExW( LPCWSTR str, DWORD fl, PVOID pdv )
{
    return WineEngAddFontResourceEx(str, fl, pdv);
}

/***********************************************************************
 *           RemoveFontResourceA    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceA( LPCSTR str )
{
    return RemoveFontResourceExA(str, 0, 0);
}

/***********************************************************************
 *           RemoveFontResourceW    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceW( LPCWSTR str )
{
    return RemoveFontResourceExW(str, 0, 0);
}

/***********************************************************************
 *           RemoveFontResourceExA    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceExA( LPCSTR str, DWORD fl, PVOID pdv )
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len);
    ret = RemoveFontResourceExW(strW, fl, pdv);
    HeapFree(GetProcessHeap(), 0, strW);
    return ret;
}

/***********************************************************************
 *           RemoveFontResourceExW    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceExW( LPCWSTR str, DWORD fl, PVOID pdv )
{
    return WineEngRemoveFontResourceEx(str, fl, pdv);
}
