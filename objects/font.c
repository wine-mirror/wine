/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
 */

#include <stdlib.h>
#include <string.h>
#include "wine/winestring.h"
#include "font.h"
#include "heap.h"
#include "metafile.h"
#include "options.h"
#include "debugtools.h"
#include "winerror.h"
#include "dc.h"

DECLARE_DEBUG_CHANNEL(font)
DECLARE_DEBUG_CHANNEL(gdi)

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
  FONTENUMPROCW       lpEnumFunc;
  LPARAM                lpData;

  LPNEWTEXTMETRICEXW  lpTextMetric;
  LPENUMLOGFONTEXW    lpLogFont;
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
static void __logfont32to16( INT16* plf16, const INT* plf32 )
{
    int  i;
    for( i = 0; i < 5; i++ ) *plf16++ = *plf32++;
   *((INT*)plf16)++ = *plf32++;
   *((INT*)plf16)   = *plf32;
}

static void __logfont16to32( INT* plf32, const INT16* plf16 )
{
    int i;
    for( i = 0; i < 5; i++ ) *plf32++ = *plf16++;
   *plf32++ = *((INT*)plf16)++;
   *plf32   = *((INT*)plf16);
}

void FONT_LogFont32ATo16( const LOGFONTA* font32, LPLOGFONT16 font16 )
{
  __logfont32to16( (INT16*)font16, (const INT*)font32 );
    lstrcpynA( font16->lfFaceName, font32->lfFaceName, LF_FACESIZE );
}

void FONT_LogFont32WTo16( const LOGFONTW* font32, LPLOGFONT16 font16 )
{
  __logfont32to16( (INT16*)font16, (const INT*)font32 );
    lstrcpynWtoA( font16->lfFaceName, font32->lfFaceName, LF_FACESIZE );
}

void FONT_LogFont16To32A( const LPLOGFONT16 font16, LPLOGFONTA font32 )
{
  __logfont16to32( (INT*)font32, (const INT16*)font16 );
    lstrcpynA( font32->lfFaceName, font16->lfFaceName, LF_FACESIZE );
}

void FONT_LogFont16To32W( const LPLOGFONT16 font16, LPLOGFONTW font32 )
{
  __logfont16to32( (INT*)font32, (const INT16*)font16 );
    lstrcpynAtoW( font32->lfFaceName, font16->lfFaceName, LF_FACESIZE );
}

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
void FONT_TextMetric32Ato16(const LPTEXTMETRICA ptm32, LPTEXTMETRIC16 ptm16 )
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

void FONT_TextMetric32Wto16(const LPTEXTMETRICW ptm32, LPTEXTMETRIC16 ptm16 )
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

void FONT_TextMetric16to32A(const LPTEXTMETRIC16 ptm16, LPTEXTMETRICA ptm32 )
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

void FONT_TextMetric16to32W(const LPTEXTMETRIC16 ptm16, LPTEXTMETRICW ptm32 )
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

void FONT_TextMetric32Ato32W(const LPTEXTMETRICA ptm32A, LPTEXTMETRICW ptm32W )
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

/***********************************************************************
 *           CreateFontIndirect16   (GDI.57)
 */
HFONT16 WINAPI CreateFontIndirect16( const LOGFONT16 *font )
{
    HFONT16 hFont = 0;

    if (font)
    {
	hFont = GDI_AllocObject( sizeof(FONTOBJ), FONT_MAGIC );
	if( hFont )
	{
	    FONTOBJ* fontPtr;
	    fontPtr = (FONTOBJ *) GDI_HEAP_LOCK( hFont );
	    memcpy( &fontPtr->logfont, font, sizeof(LOGFONT16) );

	    TRACE_(font)("(%i %i %i %i) '%s' %s %s => %04x\n",
				 font->lfHeight, font->lfWidth, 
		                 font->lfEscapement, font->lfOrientation,
				 font->lfFaceName ? font->lfFaceName : "NULL",
				 font->lfWeight > 400 ? "Bold" : "",
				 font->lfItalic ? "Italic" : "",
				 hFont);

	    if (font->lfEscapement != font->lfOrientation) {
	      /* this should really depend on whether GM_ADVANCED is set */
	      fontPtr->logfont.lfOrientation = fontPtr->logfont.lfEscapement;
	      WARN_(font)(
       "orientation angle %f set to escapement angle %f for new font %04x\n", 
	   font->lfOrientation/10., font->lfEscapement/10., hFont);
	    }
	    GDI_HEAP_UNLOCK( hFont );
	}
    }
    else WARN_(font)("(NULL) => NULL\n");

    return hFont;
}

/***********************************************************************
 *           CreateFontIndirectA   (GDI32.44)
 */
HFONT WINAPI CreateFontIndirectA( const LOGFONTA *font )
{
    LOGFONT16 font16;

    FONT_LogFont32ATo16( font, &font16 );
    return CreateFontIndirect16( &font16 );
}

/***********************************************************************
 *           CreateFontIndirectW   (GDI32.45)
 */
HFONT WINAPI CreateFontIndirectW( const LOGFONTW *font )
{
    LOGFONT16 font16;

    FONT_LogFont32WTo16( font, &font16 );
    return CreateFontIndirect16( &font16 );
}

/***********************************************************************
 *           CreateFont16    (GDI.56)
 */
HFONT16 WINAPI CreateFont16(INT16 height, INT16 width, INT16 esc, INT16 orient,
                            INT16 weight, BYTE italic, BYTE underline,
                            BYTE strikeout, BYTE charset, BYTE outpres,
                            BYTE clippres, BYTE quality, BYTE pitch,
                            LPCSTR name )
{
    LOGFONT16 logfont;

    TRACE_(font)("('%s',%d,%d)\n",
		 (name ? name : "(null)") , height, width);

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
 *           CreateFontA    (GDI32.43)
 */
HFONT WINAPI CreateFontA( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCSTR name )
{
    return (HFONT)CreateFont16( height, width, esc, orient, weight, italic,
                                  underline, strikeout, charset, outpres,
                                  clippres, quality, pitch, name );
}

/*************************************************************************
 *           CreateFontW    (GDI32.46)
 */
HFONT WINAPI CreateFontW( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCWSTR name )
{
    LPSTR namea = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HFONT ret = (HFONT)CreateFont16( height, width, esc, orient, weight,
                                         italic, underline, strikeout, charset,
                                         outpres, clippres, quality, pitch,
                                         namea );
    if (namea) HeapFree( GetProcessHeap(), 0, namea );
    return ret;
}


/***********************************************************************
 *           FONT_GetObject16
 */
INT16 FONT_GetObject16( FONTOBJ * font, INT16 count, LPSTR buffer )
{
    if (count > sizeof(LOGFONT16)) count = sizeof(LOGFONT16);
    memcpy( buffer, &font->logfont, count );
    return count;
}

/***********************************************************************
 *           FONT_GetObjectA
 */
INT FONT_GetObjectA( FONTOBJ *font, INT count, LPSTR buffer )
{
    LOGFONTA fnt32;

    FONT_LogFont16To32A( &font->logfont, &fnt32 );

    if (count > sizeof(fnt32)) count = sizeof(fnt32);
    memcpy( buffer, &fnt32, count );
    return count;
}
/***********************************************************************
 *           FONT_GetObjectW
 */
INT FONT_GetObjectW( FONTOBJ *font, INT count, LPSTR buffer )
{
    LOGFONTW fnt32;

    FONT_LogFont16To32W( &font->logfont, &fnt32 );

    if (count > sizeof(fnt32)) count = sizeof(fnt32);
    memcpy( buffer, &fnt32, count );
    return count;
}


/***********************************************************************
 *              FONT_EnumInstance16
 *
 * Called by the device driver layer to pass font info
 * down to the application.
 */
static INT FONT_EnumInstance16( LPENUMLOGFONT16 plf, 
				  LPNEWTEXTMETRIC16 ptm, UINT16 fType, LPARAM lp )
{
#define pfe ((fontEnum16*)lp)
    if( pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET || 
	pfe->lpLogFontParam->lfCharSet == plf->elfLogFont.lfCharSet )
    {
	memcpy( pfe->lpLogFont, plf, sizeof(ENUMLOGFONT16) );
	memcpy( pfe->lpTextMetric, ptm, sizeof(NEWTEXTMETRIC16) );

        return pfe->lpEnumFunc( pfe->segLogFont, pfe->segTextMetric, fType, (LPARAM)(pfe->lpData) );
    }
#undef pfe
    return 1;
}

/***********************************************************************
 *              FONT_EnumInstance
 */
static INT FONT_EnumInstance( LPENUMLOGFONT16 plf,
				  LPNEWTEXTMETRIC16 ptm, UINT16 fType, LPARAM lp )
{
    /* lfCharSet is at the same offset in both LOGFONT32A and LOGFONT32W */

#define pfe ((fontEnum32*)lp)
    if( pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET || 
	pfe->lpLogFontParam->lfCharSet == plf->elfLogFont.lfCharSet )
    {
	/* convert font metrics */

	if( pfe->dwFlags & ENUM_UNICODE )
	{
	    FONT_LogFont16To32W( &plf->elfLogFont, (LPLOGFONTW)(pfe->lpLogFont) );
	    FONT_TextMetric16to32W( (LPTEXTMETRIC16)ptm, (LPTEXTMETRICW)(pfe->lpTextMetric) );
	}
	else
	{
	    FONT_LogFont16To32A( &plf->elfLogFont, (LPLOGFONTA)pfe->lpLogFont );
	    FONT_TextMetric16to32A( (LPTEXTMETRIC16)ptm, (LPTEXTMETRICA)pfe->lpTextMetric );
	}

        return pfe->lpEnumFunc( (LPENUMLOGFONTW)pfe->lpLogFont, 
				(LPNEWTEXTMETRICW)pfe->lpTextMetric, fType, pfe->lpData );
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
    INT16	retVal = 0;
    DC* 	dc = (DC*) GDI_GetObjPtr( hDC, DC_MAGIC );

    if( dc && dc->funcs->pEnumDeviceFonts )
    {
	LPNEWTEXTMETRICEX16	lptm16 = SEGPTR_ALLOC( sizeof(NEWTEXTMETRICEX16) );
	if( lptm16 )
	{
	    LPENUMLOGFONTEX16	lplf16 = SEGPTR_ALLOC( sizeof(ENUMLOGFONTEX16) );
	    if( lplf16 )
	    {
		fontEnum16	fe16;

		fe16.lpLogFontParam = plf;
		fe16.lpEnumFunc = efproc;
		fe16.lpData = lParam;
		
		fe16.lpTextMetric = lptm16;
		fe16.lpLogFont = lplf16;
		fe16.segTextMetric = SEGPTR_GET(lptm16);
		fe16.segLogFont = SEGPTR_GET(lplf16);

		retVal = dc->funcs->pEnumDeviceFonts( dc, plf, FONT_EnumInstance16, (LPARAM)&fe16 );

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
static INT FONT_EnumFontFamiliesEx( HDC hDC, LPLOGFONTW plf, FONTENUMPROCW efproc, 
					           LPARAM lParam, DWORD dwUnicode)
{
    DC*		dc = (DC*) GDI_GetObjPtr( hDC, DC_MAGIC );

    if( dc && dc->funcs->pEnumDeviceFonts )
    {
	LOGFONT16		lf16;
	NEWTEXTMETRICEXW 	tm32w;
	ENUMLOGFONTEXW	lf32w;
	fontEnum32		fe32;

	fe32.lpLogFontParam = plf;
	fe32.lpEnumFunc = efproc;
	fe32.lpData = lParam;
	
	fe32.lpTextMetric = &tm32w;
	fe32.lpLogFont = &lf32w;
	fe32.dwFlags = dwUnicode;

	/* the only difference between LOGFONT32A and LOGFONT32W is in the lfFaceName */

	if( plf->lfFaceName[0] )
	{
	    if( dwUnicode )
		lstrcpynWtoA( lf16.lfFaceName, plf->lfFaceName, LF_FACESIZE );
	    else
		lstrcpynA( lf16.lfFaceName, (LPCSTR)plf->lfFaceName, LF_FACESIZE );
	}
	else lf16.lfFaceName[0] = '\0';
	lf16.lfCharSet = plf->lfCharSet;

	return dc->funcs->pEnumDeviceFonts( dc, &lf16, FONT_EnumInstance, (LPARAM)&fe32 );
    }
    return 0;
}

/***********************************************************************
 *              EnumFontFamiliesExW	(GDI32.82)
 */
INT WINAPI EnumFontFamiliesExW( HDC hDC, LPLOGFONTW plf,
                                    FONTENUMPROCEXW efproc, 
                                    LPARAM lParam, DWORD dwFlags )
{
    return  FONT_EnumFontFamiliesEx( hDC, plf, (FONTENUMPROCW)efproc, 
						  lParam, ENUM_UNICODE );
}

/***********************************************************************
 *              EnumFontFamiliesExA	(GDI32.81)
 */
INT WINAPI EnumFontFamiliesExA( HDC hDC, LPLOGFONTA plf,
                                    FONTENUMPROCEXA efproc, 
                                    LPARAM lParam, DWORD dwFlags)
{
    return  FONT_EnumFontFamiliesEx( hDC, (LPLOGFONTW)plf, 
				      (FONTENUMPROCW)efproc, lParam, 0);
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
 *              EnumFontFamiliesA	(GDI32.80)
 */
INT WINAPI EnumFontFamiliesA( HDC hDC, LPCSTR lpFamily,
                                  FONTENUMPROCA efproc, LPARAM lpData )
{
    LOGFONTA	lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = lf.lfFaceName[1] = '\0';

    return FONT_EnumFontFamiliesEx( hDC, (LPLOGFONTW)&lf, 
					   (FONTENUMPROCW)efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamiliesW	(GDI32.83)
 */
INT WINAPI EnumFontFamiliesW( HDC hDC, LPCWSTR lpFamily,
                                  FONTENUMPROCW efproc, LPARAM lpData )
{
    LOGFONTW  lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpynW( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = 0;

    return FONT_EnumFontFamiliesEx( hDC, &lf, efproc, lpData, ENUM_UNICODE );
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
 *              EnumFontsA		(GDI32.84)
 */
INT WINAPI EnumFontsA( HDC hDC, LPCSTR lpName, FONTENUMPROCA efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesA( hDC, lpName, efproc, lpData );
}

/***********************************************************************
 *              EnumFontsW		(GDI32.85)
 */
INT WINAPI EnumFontsW( HDC hDC, LPCWSTR lpName, FONTENUMPROCW efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesW( hDC, lpName, efproc, lpData );
}


/***********************************************************************
 *           GetTextCharacterExtra16    (GDI.89)
 */
INT16 WINAPI GetTextCharacterExtra16( HDC16 hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    return abs( (dc->w.charExtra * dc->wndExtX + dc->vportExtX / 2)
                 / dc->vportExtX );
}


/***********************************************************************
 *           GetTextCharacterExtra    (GDI32.225)
 */
INT WINAPI GetTextCharacterExtra( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    return abs( (dc->w.charExtra * dc->wndExtX + dc->vportExtX / 2)
                 / dc->vportExtX );
}


/***********************************************************************
 *           SetTextCharacterExtra16    (GDI.8)
 */
INT16 WINAPI SetTextCharacterExtra16( HDC16 hdc, INT16 extra )
{
    return (INT16)SetTextCharacterExtra( hdc, extra );
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI32.337)
 */
INT WINAPI SetTextCharacterExtra( HDC hdc, INT extra )
{
    INT prev;
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetTextCharacterExtra)
        return dc->funcs->pSetTextCharacterExtra( dc, extra );
    extra = (extra * dc->vportExtX + dc->wndExtX / 2) / dc->wndExtX;
    prev = dc->w.charExtra;
    dc->w.charExtra = abs(extra);
    return (prev * dc->wndExtX + dc->vportExtX / 2) / dc->vportExtX;
}


/***********************************************************************
 *           SetTextJustification16    (GDI.10)
 */
INT16 WINAPI SetTextJustification16( HDC16 hdc, INT16 extra, INT16 breaks )
{
    return SetTextJustification( hdc, extra, breaks );
}


/***********************************************************************
 *           SetTextJustification    (GDI32.339)
 */
BOOL WINAPI SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetTextJustification)
        return dc->funcs->pSetTextJustification( dc, extra, breaks );

    extra = abs((extra * dc->vportExtX + dc->wndExtX / 2) / dc->wndExtX);
    if (!extra) breaks = 0;
    dc->w.breakTotalExtra = extra;
    dc->w.breakCount = breaks;
    if (breaks)
    {
        dc->w.breakExtra = extra / breaks;
        dc->w.breakRem   = extra - (dc->w.breakCount * dc->w.breakExtra);
    }
    else
    {
        dc->w.breakExtra = 0;
        dc->w.breakRem   = 0;
    }
    return 1;
}


/***********************************************************************
 *           GetTextFace16    (GDI.92)
 */
INT16 WINAPI GetTextFace16( HDC16 hdc, INT16 count, LPSTR name )
{
        return GetTextFaceA(hdc,count,name);
}

/***********************************************************************
 *           GetTextFaceA    (GDI32.234)
 */
INT WINAPI GetTextFaceA( HDC hdc, INT count, LPSTR name )
{
    FONTOBJ *font;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    if (!(font = (FONTOBJ *) GDI_GetObjPtr( dc->w.hFont, FONT_MAGIC )))
        return 0;
    if (name) 
        lstrcpynA( name, font->logfont.lfFaceName, count );
    GDI_HEAP_UNLOCK( dc->w.hFont );
    if (name)
        return strlen(name);
    else
        return strlen(font->logfont.lfFaceName) + 1;
}

/***********************************************************************
 *           GetTextFaceW    (GDI32.235)
 */
INT WINAPI GetTextFaceW( HDC hdc, INT count, LPWSTR name )
{
    LPSTR nameA = HeapAlloc( GetProcessHeap(), 0, count );
    INT res = GetTextFaceA(hdc,count,nameA);
    lstrcpyAtoW( name, nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


/***********************************************************************
 *           GetTextExtent16    (GDI.91)
 */
DWORD WINAPI GetTextExtent16( HDC16 hdc, LPCSTR str, INT16 count )
{
    SIZE16 size;
    if (!GetTextExtentPoint16( hdc, str, count, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           GetTextExtentPoint16    (GDI.471)
 *
 * FIXME: Should this have a bug for compatibility?
 * Original Windows versions of GetTextExtentPoint{A,W} have documented
 * bugs (-> MSDN KB q147647.txt).
 */
BOOL16 WINAPI GetTextExtentPoint16( HDC16 hdc, LPCSTR str, INT16 count,
                                    LPSIZE16 size )
{
    SIZE size32;
    BOOL ret = GetTextExtentPoint32A( hdc, str, count, &size32 );
    CONV_SIZE32TO16( &size32, size );
    return (BOOL16)ret;
}


/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.230)
 */
BOOL WINAPI GetTextExtentPoint32A( HDC hdc, LPCSTR str, INT count,
                                     LPSIZE size )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc)
    {
        if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC )))
            return FALSE;
    }

    if (!dc->funcs->pGetTextExtentPoint ||
        !dc->funcs->pGetTextExtentPoint( dc, str, count, size ))
        return FALSE;

    TRACE_(font)("(%08x %s %d %p): returning %d,%d\n",
                 hdc, debugstr_an (str, count), count,
		 size, size->cx, size->cy );
    return TRUE;
}


/***********************************************************************
 * GetTextExtentPoint32W [GDI32.231]  Computes width/height for a string
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
    LPSTR p;
    BOOL ret;

  /* str may not be 0 terminated so we can't use HEAP_strdupWtoA.
   * We allocate one more than we need so that lstrcpynWtoA can write a
   * trailing 0 if it wants.
   */

    if(!count) count = lstrlenW(str);
    p = HeapAlloc( GetProcessHeap(), 0, count+1 );
    lstrcpynWtoA(p, str, count+1);
    ret = GetTextExtentPoint32A( hdc, p, count, size );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/***********************************************************************
 *           GetTextExtentPointA    (GDI32.232)
 */
BOOL WINAPI GetTextExtentPointA( HDC hdc, LPCSTR str, INT count,
                                          LPSIZE size )
{
    TRACE_(font)("not bug compatible.\n");
    return GetTextExtentPoint32A( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextExtentPointW   (GDI32.233)
 */
BOOL WINAPI GetTextExtentPointW( HDC hdc, LPCWSTR str, INT count,
                                          LPSIZE size )
{
    TRACE_(font)("not bug compatible.\n");
    return GetTextExtentPoint32W( hdc, str, count, size );
}


/***********************************************************************
 *           GetTextExtentExPointA    (GDI32.228)
 */
BOOL WINAPI GetTextExtentExPointA( HDC hdc, LPCSTR str, INT count,
                                       INT maxExt, LPINT lpnFit,
                                       LPINT alpDx, LPSIZE size )
{
    int index, nFit, extent;
    SIZE tSize;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    if (!dc)
    {
	if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC )))
	    return FALSE;
    }
    if (!dc->funcs->pGetTextExtentPoint) return FALSE;

    size->cx = size->cy = nFit = extent = 0;
    for(index = 0; index < count; index++)
    {
 	if(!dc->funcs->pGetTextExtentPoint( dc, str, 1, &tSize )) return FALSE;
	if( extent+tSize.cx < maxExt )
        {
	    extent+=tSize.cx;
	    nFit++;
	    str++;
	    if( alpDx ) alpDx[index] = extent;
	    if( tSize.cy > size->cy ) size->cy = tSize.cy;
        }
        else break;
    }
    size->cx = extent;
   *lpnFit = nFit;

    TRACE_(font)("(%08x '%.*s' %d) returning %d %d %d\n",
               hdc,count,str,maxExt,nFit, size->cx,size->cy);
    return TRUE;
}


/***********************************************************************
 *           GetTextExtentExPointW    (GDI32.229)
 */

BOOL WINAPI GetTextExtentExPointW( HDC hdc, LPCWSTR str, INT count,
                                       INT maxExt, LPINT lpnFit,
                                       LPINT alpDx, LPSIZE size )
{
    LPSTR p;
    BOOL ret;

  /* Docs say str should be 0 terminated here, but we'll use count just in case
   */ 

    if(!count) count = lstrlenW(str);
    p = HeapAlloc( GetProcessHeap(), 0, count+1 );
    lstrcpynWtoA(p, str, count+1);
    ret = GetTextExtentExPointA( hdc, p, count, maxExt,
                                        lpnFit, alpDx, size);
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 *           GetTextMetrics16    (GDI.93)
 */
BOOL16 WINAPI GetTextMetrics16( HDC16 hdc, TEXTMETRIC16 *metrics )
{
    TEXTMETRICA tm32;

    if (!GetTextMetricsA( (HDC)hdc, &tm32 )) return FALSE;
    FONT_TextMetric32Ato16( &tm32, metrics );
    return TRUE;
}


/***********************************************************************
 *           GetTextMetricsA    (GDI32.236)
 */
BOOL WINAPI GetTextMetricsA( HDC hdc, TEXTMETRICA *metrics )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc)
    {
        if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC )))
            return FALSE;
    }

    if (!dc->funcs->pGetTextMetrics ||
        !dc->funcs->pGetTextMetrics( dc, metrics ))
        return FALSE;

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

    TRACE_(font)("text metrics:\n"
    "    Weight = %03li\t FirstChar = %03i\t AveCharWidth = %li\n"
    "    Italic = % 3i\t LastChar = %03i\t\t MaxCharWidth = %li\n"
    "    UnderLined = %01i\t DefaultChar = %03i\t Overhang = %li\n"
    "    StruckOut = %01i\t BreakChar = %03i\t CharSet = %i\n"
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
    return TRUE;
}


/***********************************************************************
 *           GetTextMetricsW    (GDI32.237)
 */
BOOL WINAPI GetTextMetricsW( HDC hdc, TEXTMETRICW *metrics )
{
    TEXTMETRICA tm;
    if (!GetTextMetricsA( (HDC16)hdc, &tm )) return FALSE;
    FONT_TextMetric32Ato32W( &tm, metrics );
    return TRUE;
}


/***********************************************************************
 * GetOutlineTextMetrics16 [GDI.308]  Gets metrics for TrueType fonts.
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
    FIXME_(font)("(%04x,%04x,%p): stub\n", hdc,cbData,lpOTM);
    return 0;
}


/***********************************************************************
 * GetOutlineTextMetricsA [GDI.207]  Gets metrics for TrueType fonts.
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


    UINT rtn = FALSE;
    LPTEXTMETRICA lptxtMetr;



    if (lpOTM == 0)
    {
        
        lpOTM = (LPOUTLINETEXTMETRICA)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OUTLINETEXTMETRICA));
        rtn = sizeof(OUTLINETEXTMETRICA);
        cbData = rtn;
    } else
    {
        cbData = sizeof(*lpOTM);
        rtn = cbData;
    };

    lpOTM->otmSize = cbData;

    lptxtMetr =HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TEXTMETRICA));
    
    if (!GetTextMetricsA(hdc,lptxtMetr))
    {
        return 0;
    } else
    {
       memcpy(&(lpOTM->otmTextMetrics),lptxtMetr,sizeof(TEXTMETRICA));
    };

    HeapFree(GetProcessHeap(),HEAP_ZERO_MEMORY,lptxtMetr);
    
    lpOTM->otmFilter = 0;

    lpOTM->otmPanoseNumber.bFamilyType  = 0;
    lpOTM->otmPanoseNumber.bSerifStyle  = 0;
    lpOTM->otmPanoseNumber.bWeight      = 0;
    lpOTM->otmPanoseNumber.bProportion  = 0;
    lpOTM->otmPanoseNumber.bContrast    = 0;
    lpOTM->otmPanoseNumber.bStrokeVariation = 0;
    lpOTM->otmPanoseNumber.bArmStyle    = 0;
    lpOTM->otmPanoseNumber.bLetterform  = 0;
    lpOTM->otmPanoseNumber.bMidline     = 0;
    lpOTM->otmPanoseNumber.bXHeight     = 0;

    lpOTM->otmfsSelection     = 0;
    lpOTM->otmfsType          = 0;

    /*
     Further fill of the structure not implemented,
     Needs real values for the structure members
     */
    
    return rtn;
}

/***********************************************************************
 *           GetOutlineTextMetricsW [GDI32.208]
 */
UINT WINAPI GetOutlineTextMetricsW(
    HDC hdc,    /* [in]  Handle of device context */
    UINT cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRICW lpOTM)  /* [out] Address of metric data array */
{
    FIXME_(font)("(%d,%d,%p): stub\n", hdc, cbData, lpOTM);
    return 0; 
}

/***********************************************************************
 *           GetCharWidth16    (GDI.350)
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
 *           GetCharWidth32A    (GDI32.155)
 */
BOOL WINAPI GetCharWidth32A( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    UINT i, extra;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc)
    {
        if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC )))
            return FALSE;
    }

    if (!dc->funcs->pGetCharWidth ||
        !dc->funcs->pGetCharWidth( dc, firstChar, lastChar, buffer))
        return FALSE;

    /* convert device units to logical */

    extra = dc->vportExtX >> 1;
    for( i = firstChar; i <= lastChar; i++, buffer++ )
         *buffer = (*buffer * dc->wndExtX + extra) / dc->vportExtX;

    return TRUE;
}


/***********************************************************************
 *           GetCharWidth32W    (GDI32.158)
 */
BOOL WINAPI GetCharWidth32W( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    return GetCharWidth32A( hdc, firstChar, lastChar, buffer );
}



/* FIXME: all following APIs *******************************************
 *
 *
 *           SetMapperFlags16    (GDI.349)
 */
DWORD WINAPI SetMapperFlags16( HDC16 hDC, DWORD dwFlag )
{
    return SetMapperFlags( hDC, dwFlag );
}


/***********************************************************************
 *           SetMapperFlags    (GDI32.322)
 */
DWORD WINAPI SetMapperFlags( HDC hDC, DWORD dwFlag )
{
    DC *dc = DC_GetDCPtr( hDC );
    DWORD ret = 0; 
    if(!dc) return 0;
    if(dc->funcs->pSetMapperFlags)
        ret = dc->funcs->pSetMapperFlags( dc, dwFlag );
    else
        FIXME_(font)("(0x%04x, 0x%08lx): stub - harmless\n", hDC, dwFlag);
    GDI_HEAP_UNLOCK( hDC );
    return ret;
}

/***********************************************************************
 *          GetAspectRatioFilterEx16  (GDI.486)
 */
BOOL16 WINAPI GetAspectRatioFilterEx16( HDC16 hdc, LPSIZE16 pAspectRatio )
{
  FIXME_(font)("(%04x, %p): -- Empty Stub !\n",
		hdc, pAspectRatio);
  return FALSE;
}

/***********************************************************************
 *          GetAspectRatioFilterEx  (GDI32.142)
 */
BOOL WINAPI GetAspectRatioFilterEx( HDC hdc, LPSIZE pAspectRatio )
{
  FIXME_(font)("(%04x, %p): -- Empty Stub !\n",
        hdc, pAspectRatio);
  return FALSE;
}

/***********************************************************************
 *           GetCharABCWidths16   (GDI.307)
 */
BOOL16 WINAPI GetCharABCWidths16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar,
                                  LPABC16 abc )
{
    ABC abc32;
    if (!GetCharABCWidthsA( hdc, firstChar, lastChar, &abc32 )) return FALSE;
    abc->abcA = abc32.abcA;
    abc->abcB = abc32.abcB;
    abc->abcC = abc32.abcC;
    return TRUE;
}


/***********************************************************************
 *           GetCharABCWidthsA   (GDI32.149)
 */
BOOL WINAPI GetCharABCWidthsA(HDC hdc, UINT firstChar, UINT lastChar,
                                  LPABC abc )
{
    return GetCharABCWidthsW( hdc, firstChar, lastChar, abc );
}


/******************************************************************************
 * GetCharABCWidthsW [GDI32.152]  Retrieves widths of characters in range
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
    /* No TrueType fonts in Wine so far */
    FIXME_(font)("(%04x,%04x,%04x,%p): stub\n", hdc, firstChar, lastChar, abc);
    return FALSE;
}


/***********************************************************************
 *           GetGlyphOutline16    (GDI.309)
 */
DWORD WINAPI GetGlyphOutline16( HDC16 hdc, UINT16 uChar, UINT16 fuFormat,
                                LPGLYPHMETRICS16 lpgm, DWORD cbBuffer,
                                LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    FIXME_(font)("(%04x, '%c', %04x, %p, %ld, %p, %p): stub\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    return (DWORD)-1; /* failure */
}


/***********************************************************************
 *           GetGlyphOutlineA    (GDI32.186)
 */
DWORD WINAPI GetGlyphOutlineA( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    FIXME_(font)("(%04x, '%c', %04x, %p, %ld, %p, %p): stub\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    return (DWORD)-1; /* failure */
}

/***********************************************************************
 *           GetGlyphOutlineW    (GDI32.187)
 */
DWORD WINAPI GetGlyphOutlineW( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    FIXME_(font)("(%04x, '%c', %04x, %p, %ld, %p, %p): stub\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    return (DWORD)-1; /* failure */
}

/***********************************************************************
 *           CreateScalableFontResource16   (GDI.310)
 */
BOOL16 WINAPI CreateScalableFontResource16( UINT16 fHidden,
                                            LPCSTR lpszResourceFile,
                                            LPCSTR fontFile, LPCSTR path )
{
    return CreateScalableFontResourceA( fHidden, lpszResourceFile,
                                          fontFile, path );
}

/***********************************************************************
 *           CreateScalableFontResourceA   (GDI32.62)
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
    FIXME_(font)("(%ld,%s,%s,%s): stub\n",
	  fHidden, lpszResourceFile, lpszFontFile, lpszCurrentPath );
    return FALSE; /* create failed */
}

/***********************************************************************
 *           CreateScalableFontResourceW   (GDI32.63)
 */
BOOL WINAPI CreateScalableFontResourceW( DWORD fHidden,
                                             LPCWSTR lpszResourceFile,
                                             LPCWSTR lpszFontFile,
                                             LPCWSTR lpszCurrentPath )
{
    FIXME_(font)("(%ld,%p,%p,%p): stub\n",
	  fHidden, lpszResourceFile, lpszFontFile, lpszCurrentPath );
    return FALSE; /* create failed */
}


/*************************************************************************
 *             GetRasterizerCaps16   (GDI.313)
 */
BOOL16 WINAPI GetRasterizerCaps16( LPRASTERIZER_STATUS lprs, UINT16 cbNumBytes)
{
    return GetRasterizerCaps( lprs, cbNumBytes );
}


/*************************************************************************
 *             GetRasterizerCaps   (GDI32.216)
 */
BOOL WINAPI GetRasterizerCaps( LPRASTERIZER_STATUS lprs, UINT cbNumBytes)
{
  lprs->nSize = sizeof(RASTERIZER_STATUS);
  lprs->wFlags = TT_AVAILABLE|TT_ENABLED;
  lprs->nLanguageID = 0;
  return TRUE;
}


/*************************************************************************
 *             GetKerningPairs16   (GDI.332)
 */
INT16 WINAPI GetKerningPairs16( HDC16 hDC, INT16 cPairs,
                                LPKERNINGPAIR16 lpKerningPairs )
{
    /* At this time kerning is ignored (set to 0) */
    int i;
    FIXME_(font)("(%x,%d,%p): almost empty stub!\n",
	  hDC, cPairs, lpKerningPairs);
    for (i = 0; i < cPairs; i++) 
        lpKerningPairs[i].iKernAmount = 0;
    return 0;
}



/*************************************************************************
 *             GetKerningPairsA   (GDI32.192)
 */
DWORD WINAPI GetKerningPairsA( HDC hDC, DWORD cPairs,
                                 LPKERNINGPAIR lpKerningPairs )
{
    int i;
    FIXME_(font)("(%x,%ld,%p): almost empty stub!\n",
	  hDC, cPairs, lpKerningPairs);
    for (i = 0; i < cPairs; i++) 
        lpKerningPairs[i].iKernAmount = 0;
    return 0;
}


/*************************************************************************
 *             GetKerningPairsW   (GDI32.193)
 */
DWORD WINAPI GetKerningPairsW( HDC hDC, DWORD cPairs,
                                 LPKERNINGPAIR lpKerningPairs )
{
    return GetKerningPairsA( hDC, cPairs, lpKerningPairs );
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.382]
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
  LPDWORD lpSrc, /*
       if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
       if flags == TCI_SRCCHARSET: a character set value
       if flags == TCI_SRCCODEPAGE: a code page value
		 */
  LPCHARSETINFO lpCs, /* structure to receive charset information */
  DWORD flags /* determines interpretation of lpSrc */
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
 *             GetFontLanguageInfo   (GDI32.182)
 */
DWORD WINAPI GetFontLanguageInfo(HDC hdc) {
	/* return value 0 is correct for most cases anyway */
        FIXME_(font)("(%x):stub!\n", hdc);
	return 0;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI.616)
 */
DWORD WINAPI GetFontLanguageInfo16(HDC16 hdc) {
	/* return value 0 is correct for most cases anyway */
	FIXME_(font)("(%x):stub!\n",hdc);
	return 0;
}

/*************************************************************************
 * GetFontData [GDI32.181] Retrieve data for TrueType font
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
 * BUGS
 *
 * Unimplemented
 */
DWORD WINAPI GetFontData(HDC hdc, DWORD table, DWORD offset, 
    LPVOID buffer, DWORD length)
{
    FIXME_(font)("(%x,%ld,%ld,%p,%ld): stub\n", 
        hdc, table, offset, buffer, length);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return GDI_ERROR;
}

/*************************************************************************
 * GetCharacterPlacementA [GDI32.160]
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

    TRACE_(font)("%s 0x%08x 0x%08x 0x%08lx:stub!\n",
      debugstr_a(lpString), uCount, nMaxExtent, dwFlags);

    TRACE_(font)("lpOrder=%p lpDx=%p lpCaretPos=%p lpClass=%p lpOutString=%p lpGlyphs=%p\n",
      lpResults->lpOrder, lpResults->lpDx, lpResults->lpCaretPos, lpResults->lpClass,
      lpResults->lpOutString, lpResults->lpGlyphs);

    if(dwFlags)			FIXME_(font)("flags 0x%08lx ignored\n", dwFlags);
    if(lpResults->lpOrder)	FIXME_(font)("reordering not implemented\n");
    if(lpResults->lpCaretPos)	FIXME_(font)("caret positions not implemented\n");
    if(lpResults->lpClass)	FIXME_(font)("classes not implemented\n");
    if(lpResults->lpGlyphs)	FIXME_(font)("glyphs not implemented\n");

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
 * GetCharacterPlacementW [GDI32.161]
 */
DWORD WINAPI
GetCharacterPlacementW(HDC hdc, LPCWSTR lpString, INT uCount,
			 INT nMaxExtent, GCP_RESULTSW *lpResults,
			 DWORD dwFlags)
{
    /* return value 0 is correct for most cases anyway */
    FIXME_(font)(":stub!\n");
    return 0;
}

/*************************************************************************
 *      GetCharABCWidthsFloatA [GDI32.150]
 */
BOOL WINAPI GetCharABCWidthsFloatA(HDC hdc, UINT iFirstChar, UINT iLastChar,
		                        LPABCFLOAT lpABCF)
{
       FIXME_(gdi)("GetCharABCWidthsFloatA, stub\n");
       return 0;
}

/*************************************************************************
 *      GetCharABCWidthsFloatW [GDI32.151]
 */
BOOL WINAPI GetCharABCWidthsFloatW(HDC hdc, UINT iFirstChar,
		                        UINT iLastChar, LPABCFLOAT lpABCF)
{
       FIXME_(gdi)("GetCharABCWidthsFloatW, stub\n");
       return 0;
}

/*************************************************************************
 *      GetCharWidthFloatA [GDI32.156]
 */
BOOL WINAPI GetCharWidthFloatA(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
       FIXME_(gdi)("GetCharWidthFloatA, stub\n");
       return 0;
}

/*************************************************************************
 *      GetCharWidthFloatW [GDI32.157]
 */
BOOL WINAPI GetCharWidthFloatW(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
       FIXME_(gdi)("GetCharWidthFloatW, stub\n");
       return 0;
}
 
