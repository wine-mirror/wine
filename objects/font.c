/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"
#include "heap.h"
#include "metafile.h"
#include "options.h"
#include "debug.h"
#include "debugstr.h"

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
  LPLOGFONT32W          lpLogFontParam;
  FONTENUMPROC32W       lpEnumFunc;
  LPARAM                lpData;

  LPNEWTEXTMETRICEX32W  lpTextMetric;
  LPENUMLOGFONTEX32W    lpLogFont;
  DWORD                 dwFlags;
} fontEnum32;
 
/***********************************************************************
 *              LOGFONT conversion functions.
 */
static void __logfont32to16( INT16* plf16, const INT32* plf32 )
{
    int  i;
    for( i = 0; i < 5; i++ ) *plf16++ = *plf32++;
   *((INT32*)plf16)++ = *plf32++;
   *((INT32*)plf16)   = *plf32;
}

static void __logfont16to32( INT32* plf32, const INT16* plf16 )
{
    int i;
    for( i = 0; i < 5; i++ ) *plf32++ = *plf16++;
   *plf32++ = *((INT32*)plf16)++;
   *plf32   = *((INT32*)plf16);
}

void FONT_LogFont32ATo16( const LOGFONT32A* font32, LPLOGFONT16 font16 )
{
  __logfont32to16( (INT16*)font16, (const INT32*)font32 );
    lstrcpyn32A( font16->lfFaceName, font32->lfFaceName, LF_FACESIZE );
}

void FONT_LogFont32WTo16( const LOGFONT32W* font32, LPLOGFONT16 font16 )
{
  __logfont32to16( (INT16*)font16, (const INT32*)font32 );
    lstrcpynWtoA( font16->lfFaceName, font32->lfFaceName, LF_FACESIZE );
}

void FONT_LogFont16To32A( const LPLOGFONT16 font16, LPLOGFONT32A font32 )
{
  __logfont16to32( (INT32*)font32, (const INT16*)font16 );
    lstrcpyn32A( font32->lfFaceName, font16->lfFaceName, LF_FACESIZE );
}

void FONT_LogFont16To32W( const LPLOGFONT16 font16, LPLOGFONT32W font32 )
{
  __logfont16to32( (INT32*)font32, (const INT16*)font16 );
    lstrcpynAtoW( font32->lfFaceName, font16->lfFaceName, LF_FACESIZE );
}

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
void FONT_TextMetric32Ato16(const LPTEXTMETRIC32A ptm32, LPTEXTMETRIC16 ptm16 )
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

void FONT_TextMetric32Wto16(const LPTEXTMETRIC32W ptm32, LPTEXTMETRIC16 ptm16 )
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

void FONT_TextMetric16to32A(const LPTEXTMETRIC16 ptm16, LPTEXTMETRIC32A ptm32 )
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

void FONT_TextMetric16to32W(const LPTEXTMETRIC16 ptm16, LPTEXTMETRIC32W ptm32 )
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

void FONT_TextMetric32Ato32W(const LPTEXTMETRIC32A ptm32A, LPTEXTMETRIC32W ptm32W )
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

	    dprintf_info(font,"CreateFontIndirect(%i %i) '%s' %s %s => %04x\n",
				 font->lfHeight, font->lfWidth, 
				 font->lfFaceName ? font->lfFaceName : "NULL",
				 font->lfWeight > 400 ? "Bold" : "",
				 font->lfItalic ? "Italic" : "",
				 hFont);
	    GDI_HEAP_UNLOCK( hFont );
	}
    }
    else fprintf(stderr,"CreateFontIndirect(NULL) => NULL\n");

    return hFont;
}

/***********************************************************************
 *           CreateFontIndirect32A   (GDI32.44)
 */
HFONT32 WINAPI CreateFontIndirect32A( const LOGFONT32A *font )
{
    LOGFONT16 font16;

    FONT_LogFont32ATo16( font, &font16 );
    return CreateFontIndirect16( &font16 );
}

/***********************************************************************
 *           CreateFontIndirect32W   (GDI32.45)
 */
HFONT32 WINAPI CreateFontIndirect32W( const LOGFONT32W *font )
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
    LOGFONT16 logfont = { height, width, esc, orient, weight, italic, underline,
                          strikeout, charset, outpres, clippres, quality, pitch, };

    dprintf_info(font,"CreateFont16('%s',%d,%d)\n",
		 (name ? name : "(null)") , height, width);
    if (name) 
	lstrcpyn32A(logfont.lfFaceName,name,sizeof(logfont.lfFaceName));
    else 
	logfont.lfFaceName[0] = '\0';
    return CreateFontIndirect16( &logfont );
}

/*************************************************************************
 *           CreateFont32A    (GDI32.43)
 */
HFONT32 WINAPI CreateFont32A( INT32 height, INT32 width, INT32 esc,
                              INT32 orient, INT32 weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCSTR name )
{
    return (HFONT32)CreateFont16( height, width, esc, orient, weight, italic,
                                  underline, strikeout, charset, outpres,
                                  clippres, quality, pitch, name );
}

/*************************************************************************
 *           CreateFont32W    (GDI32.46)
 */
HFONT32 WINAPI CreateFont32W( INT32 height, INT32 width, INT32 esc,
                              INT32 orient, INT32 weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCWSTR name )
{
    LPSTR namea = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HFONT32 ret = (HFONT32)CreateFont16( height, width, esc, orient, weight,
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
 *           FONT_GetObject32A
 */
INT32 FONT_GetObject32A( FONTOBJ *font, INT32 count, LPSTR buffer )
{
    LOGFONT32A fnt32;

    FONT_LogFont16To32A( &font->logfont, &fnt32 );

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
static INT32 FONT_EnumInstance16( LPENUMLOGFONT16 plf, 
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
 *              FONT_EnumInstance32
 */
static INT32 FONT_EnumInstance32( LPENUMLOGFONT16 plf,
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
	    FONT_LogFont16To32W( &plf->elfLogFont, (LPLOGFONT32W)(pfe->lpLogFont) );
	    FONT_TextMetric16to32W( (LPTEXTMETRIC16)ptm, (LPTEXTMETRIC32W)(pfe->lpTextMetric) );
	}
	else
	{
	    FONT_LogFont16To32A( &plf->elfLogFont, (LPLOGFONT32A)pfe->lpLogFont );
	    FONT_TextMetric16to32A( (LPTEXTMETRIC16)ptm, (LPTEXTMETRIC32A)pfe->lpTextMetric );
	}

        return pfe->lpEnumFunc( (LPENUMLOGFONT32W)pfe->lpLogFont, 
				(LPNEWTEXTMETRIC32W)pfe->lpTextMetric, fType, pfe->lpData );
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
		fontEnum16	fe16 = { plf, efproc, lParam, lptm16, lplf16, 
					 SEGPTR_GET(lptm16), SEGPTR_GET(lplf16) };

		retVal = dc->funcs->pEnumDeviceFonts( dc, plf, FONT_EnumInstance16, (LPARAM)&fe16 );

		SEGPTR_FREE(lplf16);
	    }
	    SEGPTR_FREE(lptm16);
	}
    }
    return retVal;
}

/***********************************************************************
 *		FONT_EnumFontFamiliesEx32
 */
static INT32 FONT_EnumFontFamiliesEx32( HDC32 hDC, LPLOGFONT32W plf, FONTENUMPROC32W efproc, 
					           LPARAM lParam, DWORD dwUnicode)
{
    DC*		dc = (DC*) GDI_GetObjPtr( hDC, DC_MAGIC );

    if( dc && dc->funcs->pEnumDeviceFonts )
    {
	LOGFONT16		lf16;
	NEWTEXTMETRICEX32W 	tm32w;
	ENUMLOGFONTEX32W	lf32w;
	fontEnum32		fe32 = { plf, efproc, lParam, &tm32w, &lf32w, dwUnicode }; 

	/* the only difference between LOGFONT32A and LOGFONT32W is in the lfFaceName */

	if( plf->lfFaceName[0] )
	{
	    if( dwUnicode )
		lstrcpynWtoA( lf16.lfFaceName, plf->lfFaceName, LF_FACESIZE );
	    else
		lstrcpyn32A( lf16.lfFaceName, (LPCSTR)plf->lfFaceName, LF_FACESIZE );
	}
	else lf16.lfFaceName[0] = '\0';
	lf16.lfCharSet = plf->lfCharSet;

	return dc->funcs->pEnumDeviceFonts( dc, &lf16, FONT_EnumInstance32, (LPARAM)&fe32 );
    }
    return 0;
}

/***********************************************************************
 *              EnumFontFamiliesEx32W	(GDI32.82)
 */
INT32 WINAPI EnumFontFamiliesEx32W( HDC32 hDC, LPLOGFONT32W plf,
                                    FONTENUMPROCEX32W efproc, 
                                    LPARAM lParam, DWORD dwFlags )
{
    return  FONT_EnumFontFamiliesEx32( hDC, plf, (FONTENUMPROC32W)efproc, 
						  lParam, ENUM_UNICODE );
}

/***********************************************************************
 *              EnumFontFamiliesEx32A	(GDI32.81)
 */
INT32 WINAPI EnumFontFamiliesEx32A( HDC32 hDC, LPLOGFONT32A plf,
                                    FONTENUMPROCEX32A efproc, 
                                    LPARAM lParam, DWORD dwFlags)
{
    return  FONT_EnumFontFamiliesEx32( hDC, (LPLOGFONT32W)plf, 
				      (FONTENUMPROC32W)efproc, lParam, 0);
}

/***********************************************************************
 *              EnumFontFamilies16	(GDI.330)
 */
INT16 WINAPI EnumFontFamilies16( HDC16 hDC, LPCSTR lpFamily,
                                 FONTENUMPROC16 efproc, LPARAM lpData )
{
    LOGFONT16	lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpyn32A( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = '\0';

    return EnumFontFamiliesEx16( hDC, &lf, (FONTENUMPROCEX16)efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamilies32A	(GDI32.80)
 */
INT32 WINAPI EnumFontFamilies32A( HDC32 hDC, LPCSTR lpFamily,
                                  FONTENUMPROC32A efproc, LPARAM lpData )
{
    LOGFONT32A	lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpyn32A( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = lf.lfFaceName[1] = '\0';

    return FONT_EnumFontFamiliesEx32( hDC, (LPLOGFONT32W)&lf, 
					   (FONTENUMPROC32W)efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamilies32W	(GDI32.83)
 */
INT32 WINAPI EnumFontFamilies32W( HDC32 hDC, LPCWSTR lpFamily,
                                  FONTENUMPROC32W efproc, LPARAM lpData )
{
    LOGFONT32W  lf;

    lf.lfCharSet = DEFAULT_CHARSET;
    if( lpFamily ) lstrcpyn32W( lf.lfFaceName, lpFamily, LF_FACESIZE );
    else lf.lfFaceName[0] = 0;

    return FONT_EnumFontFamiliesEx32( hDC, &lf, efproc, lpData, ENUM_UNICODE );
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
 *              EnumFonts32A		(GDI32.84)
 */
INT32 WINAPI EnumFonts32A( HDC32 hDC, LPCSTR lpName, FONTENUMPROC32A efproc,
                           LPARAM lpData )
{
    return EnumFontFamilies32A( hDC, lpName, efproc, lpData );
}

/***********************************************************************
 *              EnumFonts32W		(GDI32.85)
 */
INT32 WINAPI EnumFonts32W( HDC32 hDC, LPCWSTR lpName, FONTENUMPROC32W efproc,
                           LPARAM lpData )
{
    return EnumFontFamilies32W( hDC, lpName, efproc, lpData );
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
 *           GetTextCharacterExtra32    (GDI32.225)
 */
INT32 WINAPI GetTextCharacterExtra32( HDC32 hdc )
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
    return (INT16)SetTextCharacterExtra32( hdc, extra );
}


/***********************************************************************
 *           SetTextCharacterExtra32    (GDI32.337)
 */
INT32 WINAPI SetTextCharacterExtra32( HDC32 hdc, INT32 extra )
{
    INT32 prev;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
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
    return SetTextJustification32( hdc, extra, breaks );
}


/***********************************************************************
 *           SetTextJustification32    (GDI32.339)
 */
BOOL32 WINAPI SetTextJustification32( HDC32 hdc, INT32 extra, INT32 breaks )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

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
        return GetTextFace32A(hdc,count,name);
}

/***********************************************************************
 *           GetTextFace32A    (GDI32.234)
 */
INT32 WINAPI GetTextFace32A( HDC32 hdc, INT32 count, LPSTR name )
{
    FONTOBJ *font;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    if (!(font = (FONTOBJ *) GDI_GetObjPtr( dc->w.hFont, FONT_MAGIC )))
        return 0;
    lstrcpyn32A( name, font->logfont.lfFaceName, count );
    GDI_HEAP_UNLOCK( dc->w.hFont );
    return strlen(name);
}

/***********************************************************************
 *           GetTextFace32W    (GDI32.235)
 */
INT32 WINAPI GetTextFace32W( HDC32 hdc, INT32 count, LPWSTR name )
{
    LPSTR nameA = HeapAlloc( GetProcessHeap(), 0, count );
    INT32 res = GetTextFace32A(hdc,count,nameA);
    lstrcpyAtoW( name, nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


/***********************************************************************
 *           GetTextExtent    (GDI.91)
 */
DWORD WINAPI GetTextExtent( HDC16 hdc, LPCSTR str, INT16 count )
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
 * bugs.
 */
BOOL16 WINAPI GetTextExtentPoint16( HDC16 hdc, LPCSTR str, INT16 count,
                                    LPSIZE16 size )
{
    SIZE32 size32;
    BOOL32 ret = GetTextExtentPoint32A( hdc, str, count, &size32 );
    CONV_SIZE32TO16( &size32, size );
    return (BOOL16)ret;
}


/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.230)
 */
BOOL32 WINAPI GetTextExtentPoint32A( HDC32 hdc, LPCSTR str, INT32 count,
                                     LPSIZE32 size )
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

    dprintf_info(font,"GetTextExtentPoint(%08x %s %d %p): returning %d,%d\n",
                 hdc, debugstr_an (str, count), count,
		 size, size->cx, size->cy );
    return TRUE;
}


/***********************************************************************
 *           GetTextExtentPoint32W    (GDI32.231)
 */
BOOL32 WINAPI GetTextExtentPoint32W( HDC32 hdc, LPCWSTR str, INT32 count,
                                     LPSIZE32 size )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    BOOL32 ret = GetTextExtentPoint32A( hdc, p, count, size );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/***********************************************************************
 *           GetTextExtentPoint32ABuggy    (GDI32.232)
 */
BOOL32 WINAPI GetTextExtentPoint32ABuggy( HDC32 hdc, LPCSTR str, INT32 count,
                                          LPSIZE32 size )
{
    dprintf_info(font, "GetTextExtentPoint32ABuggy: not bug compatible.\n");
    return GetTextExtentPoint32A( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextExtentPoint32WBuggy    (GDI32.233)
 */
BOOL32 WINAPI GetTextExtentPoint32WBuggy( HDC32 hdc, LPCWSTR str, INT32 count,
                                          LPSIZE32 size )
{
    dprintf_info(font, "GetTextExtentPoint32WBuggy: not bug compatible.\n");
    return GetTextExtentPoint32W( hdc, str, count, size );
}


/***********************************************************************
 *           GetTextExtentExPoint32A    (GDI32.228)
 */
BOOL32 WINAPI GetTextExtentExPoint32A( HDC32 hdc, LPCSTR str, INT32 count,
                                       INT32 maxExt, LPINT32 lpnFit,
                                       LPINT32 alpDx, LPSIZE32 size )
{
    int index, nFit, extent;
    SIZE32 tSize;
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

    dprintf_info(font,"GetTextExtentExPoint32A(%08x '%.*s' %d) returning %d %d %d\n",
               hdc,count,str,maxExt,nFit, size->cx,size->cy);
    return TRUE;
}


/***********************************************************************
 *           GetTextExtentExPoint32W    (GDI32.229)
 */

BOOL32 WINAPI GetTextExtentExPoint32W( HDC32 hdc, LPCWSTR str, INT32 count,
                                       INT32 maxExt, LPINT32 lpnFit,
                                       LPINT32 alpDx, LPSIZE32 size )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    BOOL32 ret = GetTextExtentExPoint32A( hdc, p, count, maxExt,
                                        lpnFit, alpDx, size);
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 *           GetTextMetrics16    (GDI.93)
 */
BOOL16 WINAPI GetTextMetrics16( HDC16 hdc, TEXTMETRIC16 *metrics )
{
    TEXTMETRIC32A tm32;

    if (!GetTextMetrics32A( (HDC32)hdc, &tm32 )) return FALSE;
    FONT_TextMetric32Ato16( &tm32, metrics );
    return TRUE;
}


/***********************************************************************
 *           GetTextMetrics32A    (GDI32.236)
 */
BOOL32 WINAPI GetTextMetrics32A( HDC32 hdc, TEXTMETRIC32A *metrics )
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

    dprintf_info(font,"text metrics:
    Weight = %03i\t FirstChar = %03i\t AveCharWidth = %i
    Italic = % 3i\t LastChar = %03i\t\t MaxCharWidth = %i
    UnderLined = %01i\t DefaultChar = %03i\t Overhang = %i
    StruckOut = %01i\t BreakChar = %03i\t CharSet = %i
    PitchAndFamily = %02x
    --------------------
    InternalLeading = %i
    Ascent = %i
    Descent = %i
    Height = %i\n",
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
 *           GetTextMetrics32W    (GDI32.237)
 */
BOOL32 WINAPI GetTextMetrics32W( HDC32 hdc, TEXTMETRIC32W *metrics )
{
    TEXTMETRIC32A tm;
    if (!GetTextMetrics32A( (HDC16)hdc, &tm )) return FALSE;
    FONT_TextMetric32Ato32W( &tm, metrics );
    return TRUE;
}


/***********************************************************************
 *           GetCharWidth16    (GDI.350)
 */
BOOL16 WINAPI GetCharWidth16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar,
                              LPINT16 buffer )
{
    BOOL32	retVal = FALSE;

    if( firstChar != lastChar )
    {
	LPINT32	buf32 = (LPINT32)HeapAlloc(GetProcessHeap(), 0,
				 sizeof(INT32)*(1 + (lastChar - firstChar)));
	if( buf32 )
	{
	    LPINT32	obuf32 = buf32;
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
	INT32 chWidth;
	retVal = GetCharWidth32A(hdc, firstChar, lastChar, &chWidth );
       *buffer = chWidth;
    }
    return retVal;
}


/***********************************************************************
 *           GetCharWidth32A    (GDI32.155)
 */
BOOL32 WINAPI GetCharWidth32A( HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                               LPINT32 buffer )
{
    UINT32 i, extra;
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
BOOL32 WINAPI GetCharWidth32W( HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                               LPINT32 buffer )
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
    return SetMapperFlags32( hDC, dwFlag );
}


/***********************************************************************
 *           SetMapperFlags32    (GDI32.322)
 */
DWORD WINAPI SetMapperFlags32( HDC32 hDC, DWORD dwFlag )
{
    dprintf_fixme(font, "SetMapperFlags(%04x, %08lX) // Empty Stub !\n",
		  hDC, dwFlag);
    return 0L;
}

/***********************************************************************
 *          GetAspectRatioFilterEx16  (GDI.486)
 */
BOOL16 GetAspectRatioFilterEx16( HDC16 hdc, LPVOID pAspectRatio )
{
  dprintf_fixme(font, "GetAspectRatioFilterEx(%04x, %p): // Empty Stub !\n",
		hdc, pAspectRatio);
  return FALSE;
}


/***********************************************************************
 *           GetCharABCWidths16   (GDI.307)
 */
BOOL16 WINAPI GetCharABCWidths16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar,
                                  LPABC16 abc )
{
    ABC32 abc32;
    if (!GetCharABCWidths32A( hdc, firstChar, lastChar, &abc32 )) return FALSE;
    abc->abcA = abc32.abcA;
    abc->abcB = abc32.abcB;
    abc->abcC = abc32.abcC;
    return TRUE;
}


/***********************************************************************
 *           GetCharABCWidths32A   (GDI32.149)
 */
BOOL32 WINAPI GetCharABCWidths32A(HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                                  LPABC32 abc )
{
    /* No TrueType fonts in Wine so far */
    fprintf( stdnimp, "STUB: GetCharABCWidths(%04x,%04x,%04x,%p)\n",
             hdc, firstChar, lastChar, abc );
    return FALSE;
}


/***********************************************************************
 *           GetCharABCWidths32W   (GDI32.152)
 */
BOOL32 WINAPI GetCharABCWidths32W(HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                                  LPABC32 abc )
{
    return GetCharABCWidths32A( hdc, firstChar, lastChar, abc );
}


/***********************************************************************
 *           GetGlyphOutline16    (GDI.309)
 */
DWORD WINAPI GetGlyphOutline16( HDC16 hdc, UINT16 uChar, UINT16 fuFormat,
                                LPGLYPHMETRICS16 lpgm, DWORD cbBuffer,
                                LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    fprintf( stdnimp,"GetGlyphOutLine16(%04x, '%c', %04x, %p, %ld, %p, %p) // - empty stub!\n",
             hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    return (DWORD)-1; /* failure */
}


/***********************************************************************
 *           GetGlyphOutline32A    (GDI32.186)
 */
DWORD WINAPI GetGlyphOutline32A( HDC32 hdc, UINT32 uChar, UINT32 fuFormat,
                                 LPGLYPHMETRICS32 lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    fprintf( stdnimp,"GetGlyphOutLine32A(%04x, '%c', %04x, %p, %ld, %p, %p) // - empty stub!\n",
             hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    return (DWORD)-1; /* failure */
}

/***********************************************************************
 *           GetGlyphOutline32W    (GDI32.187)
 */
DWORD WINAPI GetGlyphOutline32W( HDC32 hdc, UINT32 uChar, UINT32 fuFormat,
                                 LPGLYPHMETRICS32 lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    fprintf( stdnimp,"GetGlyphOutLine32W(%04x, '%c', %04x, %p, %ld, %p, %p) // - empty stub!\n",
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
    return CreateScalableFontResource32A( fHidden, lpszResourceFile,
                                          fontFile, path );
}

/***********************************************************************
 *           CreateScalableFontResource32A   (GDI32.62)
 */
BOOL32 WINAPI CreateScalableFontResource32A( DWORD fHidden,
                                             LPCSTR lpszResourceFile,
                                             LPCSTR lpszFontFile,
                                             LPCSTR lpszCurrentPath )
{
    /* fHidden=1 - only visible for the calling app, read-only, not
     * enumbered with EnumFonts/EnumFontFamilies
     * lpszCurrentPath can be NULL
     */
    fprintf(stdnimp,"CreateScalableFontResource(%ld,%s,%s,%s) // empty stub\n",
            fHidden, lpszResourceFile, lpszFontFile, lpszCurrentPath );
    return FALSE; /* create failed */
}

/***********************************************************************
 *           CreateScalableFontResource32W   (GDI32.63)
 */
BOOL32 WINAPI CreateScalableFontResource32W( DWORD fHidden,
                                             LPCWSTR lpszResourceFile,
                                             LPCWSTR lpszFontFile,
                                             LPCWSTR lpszCurrentPath )
{
    fprintf(stdnimp,"CreateScalableFontResource32W(%ld,%p,%p,%p) // empty stub\n",
            fHidden, lpszResourceFile, lpszFontFile, lpszCurrentPath );
    return FALSE; /* create failed */
}


/*************************************************************************
 *             GetRasterizerCaps16   (GDI.313)
 */
BOOL16 WINAPI GetRasterizerCaps16( LPRASTERIZER_STATUS lprs, UINT16 cbNumBytes)
{
    return GetRasterizerCaps32( lprs, cbNumBytes );
}


/*************************************************************************
 *             GetRasterizerCaps32   (GDI32.216)
 */
BOOL32 WINAPI GetRasterizerCaps32( LPRASTERIZER_STATUS lprs, UINT32 cbNumBytes)
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
    fprintf(stdnimp,"GetKerningPairs16: almost empty stub!\n");
    for (i = 0; i < cPairs; i++) lpKerningPairs[i].iKernAmount = 0;
    return 0;
}



/*************************************************************************
 *             GetKerningPairs32A   (GDI32.192)
 */
DWORD WINAPI GetKerningPairs32A( HDC32 hDC, DWORD cPairs,
                                 LPKERNINGPAIR32 lpKerningPairs )
{
    int i;
    fprintf(stdnimp,"GetKerningPairs32: almost empty stub!\n");
    for (i = 0; i < cPairs; i++) lpKerningPairs[i].iKernAmount = 0;
    return 0;
}


/*************************************************************************
 *             GetKerningPairs32W   (GDI32.193)
 */
DWORD WINAPI GetKerningPairs32W( HDC32 hDC, DWORD cPairs,
                                 LPKERNINGPAIR32 lpKerningPairs )
{
    return GetKerningPairs32A( hDC, cPairs, lpKerningPairs );
}

BOOL32 WINAPI TranslateCharSetInfo(LPDWORD lpSrc,LPCHARSETINFO lpCs,DWORD dwFlags) {
    fprintf(stderr,"TranslateCharSetInfo(%p,%p,0x%08lx), stub.\n",
    	lpSrc,lpCs,dwFlags
    );
    return TRUE;
}


/*************************************************************************
 *             GetFontLanguageInfo   (GDI32.182)
 */
DWORD WINAPI GetFontLanguageInfo32(HDC32 hdc) {
	/* return value 0 is correct for most cases anyway */
	fprintf(stderr,"GetFontLanguageInfo:stub!\n");
	return 0;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI.616)
 */
DWORD WINAPI GetFontLanguageInfo16(HDC16 hdc) {
	/* return value 0 is correct for most cases anyway */
	fprintf(stderr,"GetFontLanguageInfo:stub!\n");
	return 0;
}
