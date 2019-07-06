/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
 * Copyright 2002,2003 Shachar Shemesh
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winreg.h"
#include "gdi_private.h"
#include "wine/exception.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

  /* Device -> World size conversion */

/* Performs a device to world transformation on the specified width (which
 * is in integer format).
 */
static inline INT INTERNAL_XDSTOWS(DC *dc, INT width)
{
    double floatWidth;

    /* Perform operation with floating point */
    floatWidth = (double)width * dc->xformVport2World.eM11;
    /* Round to integers */
    return GDI_ROUND(floatWidth);
}

/* Performs a device to world transformation on the specified size (which
 * is in integer format).
 */
static inline INT INTERNAL_YDSTOWS(DC *dc, INT height)
{
    double floatHeight;

    /* Perform operation with floating point */
    floatHeight = (double)height * dc->xformVport2World.eM22;
    /* Round to integers */
    return GDI_ROUND(floatHeight);
}

/* scale width and height but don't mirror them */

static inline INT width_to_LP( DC *dc, INT width )
{
    return GDI_ROUND( (double)width * fabs( dc->xformVport2World.eM11 ));
}

static inline INT height_to_LP( DC *dc, INT height )
{
    return GDI_ROUND( (double)height * fabs( dc->xformVport2World.eM22 ));
}

static inline INT INTERNAL_YWSTODS(DC *dc, INT height)
{
    POINT pt[2];
    pt[0].x = pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    lp_to_dp(dc, pt, 2);
    return pt[1].y - pt[0].y;
}

static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, HDC hdc );
static INT FONT_GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer );
static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL FONT_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs font_funcs =
{
    FONT_SelectObject,  /* pSelectObject */
    FONT_GetObjectA,    /* pGetObjectA */
    FONT_GetObjectW,    /* pGetObjectW */
    NULL,               /* pUnrealizeObject */
    FONT_DeleteObject   /* pDeleteObject */
};

typedef struct
{
    LOGFONTW    logfont;
} FONTOBJ;

struct font_enum
{
  LPLOGFONTW          lpLogFontParam;
  FONTENUMPROCW       lpEnumFunc;
  LPARAM              lpData;
  BOOL                unicode;
  HDC                 hdc;
  INT                 retval;
};

/*
 *  For TranslateCharsetInfo
 */
#define MAXTCIINDEX 32
static const CHARSETINFO FONT_tci[MAXTCIINDEX] = {
  /* ANSI */
  { ANSI_CHARSET, 1252, {{0,0,0,0},{FS_LATIN1,0}} },
  { EASTEUROPE_CHARSET, 1250, {{0,0,0,0},{FS_LATIN2,0}} },
  { RUSSIAN_CHARSET, 1251, {{0,0,0,0},{FS_CYRILLIC,0}} },
  { GREEK_CHARSET, 1253, {{0,0,0,0},{FS_GREEK,0}} },
  { TURKISH_CHARSET, 1254, {{0,0,0,0},{FS_TURKISH,0}} },
  { HEBREW_CHARSET, 1255, {{0,0,0,0},{FS_HEBREW,0}} },
  { ARABIC_CHARSET, 1256, {{0,0,0,0},{FS_ARABIC,0}} },
  { BALTIC_CHARSET, 1257, {{0,0,0,0},{FS_BALTIC,0}} },
  { VIETNAMESE_CHARSET, 1258, {{0,0,0,0},{FS_VIETNAMESE,0}} },
  /* reserved by ANSI */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  /* ANSI and OEM */
  { THAI_CHARSET, 874, {{0,0,0,0},{FS_THAI,0}} },
  { SHIFTJIS_CHARSET, 932, {{0,0,0,0},{FS_JISJAPAN,0}} },
  { GB2312_CHARSET, 936, {{0,0,0,0},{FS_CHINESESIMP,0}} },
  { HANGEUL_CHARSET, 949, {{0,0,0,0},{FS_WANSUNG,0}} },
  { CHINESEBIG5_CHARSET, 950, {{0,0,0,0},{FS_CHINESETRAD,0}} },
  { JOHAB_CHARSET, 1361, {{0,0,0,0},{FS_JOHAB,0}} },
  /* reserved for alternate ANSI and OEM */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  /* reserved for system */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { SYMBOL_CHARSET, CP_SYMBOL, {{0,0,0,0},{FS_SYMBOL,0}} }
};

static void FONT_LogFontAToW( const LOGFONTA *fontA, LPLOGFONTW fontW )
{
    memcpy(fontW, fontA, sizeof(LOGFONTA) - LF_FACESIZE);
    MultiByteToWideChar(CP_ACP, 0, fontA->lfFaceName, -1, fontW->lfFaceName,
			LF_FACESIZE);
    fontW->lfFaceName[LF_FACESIZE-1] = 0;
}

static void FONT_LogFontWToA( const LOGFONTW *fontW, LPLOGFONTA fontA )
{
    memcpy(fontA, fontW, sizeof(LOGFONTA) - LF_FACESIZE);
    WideCharToMultiByte(CP_ACP, 0, fontW->lfFaceName, -1, fontA->lfFaceName,
			LF_FACESIZE, NULL, NULL);
    fontA->lfFaceName[LF_FACESIZE-1] = 0;
}

static void FONT_EnumLogFontExWToA( const ENUMLOGFONTEXW *fontW, LPENUMLOGFONTEXA fontA )
{
    FONT_LogFontWToA( &fontW->elfLogFont, &fontA->elfLogFont );

    WideCharToMultiByte( CP_ACP, 0, fontW->elfFullName, -1,
			 (LPSTR) fontA->elfFullName, LF_FULLFACESIZE, NULL, NULL );
    fontA->elfFullName[LF_FULLFACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfStyle, -1,
			 (LPSTR) fontA->elfStyle, LF_FACESIZE, NULL, NULL );
    fontA->elfStyle[LF_FACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfScript, -1,
			 (LPSTR) fontA->elfScript, LF_FACESIZE, NULL, NULL );
    fontA->elfScript[LF_FACESIZE-1] = '\0';
}

static void FONT_EnumLogFontExAToW( const ENUMLOGFONTEXA *fontA, LPENUMLOGFONTEXW fontW )
{
    FONT_LogFontAToW( &fontA->elfLogFont, &fontW->elfLogFont );

    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfFullName, -1,
			 fontW->elfFullName, LF_FULLFACESIZE );
    fontW->elfFullName[LF_FULLFACESIZE-1] = '\0';
    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfStyle, -1,
			 fontW->elfStyle, LF_FACESIZE );
    fontW->elfStyle[LF_FACESIZE-1] = '\0';
    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfScript, -1,
			 fontW->elfScript, LF_FACESIZE );
    fontW->elfScript[LF_FACESIZE-1] = '\0';
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
    ptmA->tmFirstChar = min(ptmW->tmFirstChar, 255);
    if (ptmW->tmCharSet == SYMBOL_CHARSET)
    {
        ptmA->tmFirstChar = 0x1e;
        ptmA->tmLastChar = 0xff;  /* win9x behaviour - we need the OS2 table data to calculate correctly */
    }
    else if (ptmW->tmPitchAndFamily & TMPF_TRUETYPE)
    {
        ptmA->tmFirstChar = ptmW->tmDefaultChar - 1;
        ptmA->tmLastChar = min(ptmW->tmLastChar, 0xff);
    }
    else
    {
        ptmA->tmFirstChar = min(ptmW->tmFirstChar, 0xff);
        ptmA->tmLastChar  = min(ptmW->tmLastChar,  0xff);
    }
    ptmA->tmDefaultChar = ptmW->tmDefaultChar;
    ptmA->tmBreakChar = ptmW->tmBreakChar;
    ptmA->tmItalic = ptmW->tmItalic;
    ptmA->tmUnderlined = ptmW->tmUnderlined;
    ptmA->tmStruckOut = ptmW->tmStruckOut;
    ptmA->tmPitchAndFamily = ptmW->tmPitchAndFamily;
    ptmA->tmCharSet = ptmW->tmCharSet;
}


static void FONT_NewTextMetricExWToA(const NEWTEXTMETRICEXW *ptmW, NEWTEXTMETRICEXA *ptmA )
{
    FONT_TextMetricWToA((const TEXTMETRICW *)ptmW, (LPTEXTMETRICA)ptmA);
    ptmA->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptmA->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptmA->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptmA->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    memcpy(&ptmA->ntmFontSig, &ptmW->ntmFontSig, sizeof(FONTSIGNATURE));
}

static DWORD get_key_value( HKEY key, const WCHAR *name, DWORD *value )
{
    WCHAR buf[12];
    DWORD count = sizeof(buf), type, err;

    err = RegQueryValueExW( key, name, NULL, &type, (BYTE *)buf, &count );
    if (!err)
    {
        if (type == REG_DWORD) memcpy( value, buf, sizeof(*value) );
        else *value = atoiW( buf );
    }
    return err;
}

static UINT get_subpixel_orientation( HKEY key )
{
    static const WCHAR smoothing_orientation[] = {'F','o','n','t','S','m','o','o','t','h','i','n','g',
                                                  'O','r','i','e','n','t','a','t','i','o','n',0};
    DWORD orient;

    /* FIXME: handle vertical orientations even though Windows doesn't */
    if (get_key_value( key, smoothing_orientation, &orient )) return GGO_GRAY4_BITMAP;

    switch (orient)
    {
    case 0: /* FE_FONTSMOOTHINGORIENTATIONBGR */
        return WINE_GGO_HBGR_BITMAP;
    case 1: /* FE_FONTSMOOTHINGORIENTATIONRGB */
        return WINE_GGO_HRGB_BITMAP;
    }
    return GGO_GRAY4_BITMAP;
}

static UINT get_default_smoothing( HKEY key )
{
    static const WCHAR smoothing[] = {'F','o','n','t','S','m','o','o','t','h','i','n','g',0};
    static const WCHAR smoothing_type[] = {'F','o','n','t','S','m','o','o','t','h','i','n','g','T','y','p','e',0};
    DWORD enabled, type;

    if (get_key_value( key, smoothing, &enabled )) return 0;
    if (!enabled) return GGO_BITMAP;

    if (!get_key_value( key, smoothing_type, &type ) && type == 2 /* FE_FONTSMOOTHINGCLEARTYPE */)
        return get_subpixel_orientation( key );

    return GGO_GRAY4_BITMAP;
}

/* compute positions for text rendering, in device coords */
static BOOL get_char_positions( DC *dc, const WCHAR *str, INT count, INT *dx, SIZE *size )
{
    TEXTMETRICW tm;
    PHYSDEV dev;

    size->cx = size->cy = 0;
    if (!count) return TRUE;

    dev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    dev->funcs->pGetTextMetrics( dev, &tm );

    dev = GET_DC_PHYSDEV( dc, pGetTextExtentExPoint );
    if (!dev->funcs->pGetTextExtentExPoint( dev, str, count, dx )) return FALSE;

    if (dc->breakExtra || dc->breakRem)
    {
        int i, space = 0, rem = dc->breakRem;

        for (i = 0; i < count; i++)
        {
            if (str[i] == tm.tmBreakChar)
            {
                space += dc->breakExtra;
                if (rem > 0)
                {
                    space++;
                    rem--;
                }
            }
            dx[i] += space;
        }
    }
    size->cx = dx[count - 1];
    size->cy = tm.tmHeight;
    return TRUE;
}

/* compute positions for text rendering, in device coords */
static BOOL get_char_positions_indices( DC *dc, const WORD *indices, INT count, INT *dx, SIZE *size )
{
    TEXTMETRICW tm;
    PHYSDEV dev;

    size->cx = size->cy = 0;
    if (!count) return TRUE;

    dev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    dev->funcs->pGetTextMetrics( dev, &tm );

    dev = GET_DC_PHYSDEV( dc, pGetTextExtentExPointI );
    if (!dev->funcs->pGetTextExtentExPointI( dev, indices, count, dx )) return FALSE;

    if (dc->breakExtra || dc->breakRem)
    {
        WORD space_index;
        int i, space = 0, rem = dc->breakRem;

        dev = GET_DC_PHYSDEV( dc, pGetGlyphIndices );
        dev->funcs->pGetGlyphIndices( dev, &tm.tmBreakChar, 1, &space_index, 0 );

        for (i = 0; i < count; i++)
        {
            if (indices[i] == space_index)
            {
                space += dc->breakExtra;
                if (rem > 0)
                {
                    space++;
                    rem--;
                }
            }
            dx[i] += space;
        }
    }
    size->cx = dx[count - 1];
    size->cy = tm.tmHeight;
    return TRUE;
}

/***********************************************************************
 *           GdiGetCodePage   (GDI32.@)
 */
DWORD WINAPI GdiGetCodePage( HDC hdc )
{
    UINT cp = CP_ACP;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        cp = dc->font_code_page;
        release_dc_ptr( dc );
    }
    return cp;
}

/***********************************************************************
 *           get_text_charset_info
 *
 * Internal version of GetTextCharsetInfo() that takes a DC pointer.
 */
static UINT get_text_charset_info(DC *dc, FONTSIGNATURE *fs, DWORD flags)
{
    UINT ret = DEFAULT_CHARSET;
    PHYSDEV dev;

    dev = GET_DC_PHYSDEV( dc, pGetTextCharsetInfo );
    ret = dev->funcs->pGetTextCharsetInfo( dev, fs, flags );

    if (ret == DEFAULT_CHARSET && fs)
        memset(fs, 0, sizeof(FONTSIGNATURE));
    return ret;
}

/***********************************************************************
 *           GetTextCharsetInfo    (GDI32.@)
 */
UINT WINAPI GetTextCharsetInfo(HDC hdc, FONTSIGNATURE *fs, DWORD flags)
{
    UINT ret = DEFAULT_CHARSET;
    DC *dc = get_dc_ptr(hdc);

    if (dc)
    {
        ret = get_text_charset_info( dc, fs, flags );
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           FONT_mbtowc
 *
 * Returns a Unicode translation of str using the charset of the
 * currently selected font in hdc.  If count is -1 then str is assumed
 * to be '\0' terminated, otherwise it contains the number of bytes to
 * convert.  If plenW is non-NULL, on return it will point to the
 * number of WCHARs that have been written.  If pCP is non-NULL, on
 * return it will point to the codepage used in the conversion.  The
 * caller should free the returned LPWSTR from the process heap
 * itself.
 */
static LPWSTR FONT_mbtowc(HDC hdc, LPCSTR str, INT count, INT *plenW, UINT *pCP)
{
    UINT cp;
    INT lenW;
    LPWSTR strW;

    cp = GdiGetCodePage( hdc );

    if(count == -1) count = strlen(str);
    lenW = MultiByteToWideChar(cp, 0, str, count, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW*sizeof(WCHAR));
    MultiByteToWideChar(cp, 0, str, count, strW, lenW);
    TRACE("mapped %s -> %s\n", debugstr_an(str, count), debugstr_wn(strW, lenW));
    if(plenW) *plenW = lenW;
    if(pCP) *pCP = cp;
    return strW;
}

/***********************************************************************
 *           CreateFontIndirectExA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectExA( const ENUMLOGFONTEXDVA *penumexA )
{
    ENUMLOGFONTEXDVW enumexW;

    if (!penumexA) return 0;

    FONT_EnumLogFontExAToW( &penumexA->elfEnumLogfontEx, &enumexW.elfEnumLogfontEx );
    enumexW.elfDesignVector = penumexA->elfDesignVector;
    return CreateFontIndirectExW( &enumexW );
}

/***********************************************************************
 *           CreateFontIndirectExW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectExW( const ENUMLOGFONTEXDVW *penumex )
{
    HFONT hFont;
    FONTOBJ *fontPtr;
    const LOGFONTW *plf;

    if (!penumex) return 0;

    if (penumex->elfEnumLogfontEx.elfFullName[0] ||
        penumex->elfEnumLogfontEx.elfStyle[0] ||
        penumex->elfEnumLogfontEx.elfScript[0])
    {
        FIXME("some fields ignored. fullname=%s, style=%s, script=%s\n",
            debugstr_w(penumex->elfEnumLogfontEx.elfFullName),
            debugstr_w(penumex->elfEnumLogfontEx.elfStyle),
            debugstr_w(penumex->elfEnumLogfontEx.elfScript));
    }

    plf = &penumex->elfEnumLogfontEx.elfLogFont;
    if (!(fontPtr = HeapAlloc( GetProcessHeap(), 0, sizeof(*fontPtr) ))) return 0;

    fontPtr->logfont = *plf;

    if (!(hFont = alloc_gdi_handle( fontPtr, OBJ_FONT, &font_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, fontPtr );
        return 0;
    }

    TRACE("(%d %d %d %d %x %d %x %d %d) %s %s %s %s => %p\n",
          plf->lfHeight, plf->lfWidth,
          plf->lfEscapement, plf->lfOrientation,
          plf->lfPitchAndFamily,
          plf->lfOutPrecision, plf->lfClipPrecision,
          plf->lfQuality, plf->lfCharSet,
          debugstr_w(plf->lfFaceName),
          plf->lfWeight > 400 ? "Bold" : "",
          plf->lfItalic ? "Italic" : "",
          plf->lfUnderline ? "Underline" : "", hFont);

    return hFont;
}

/***********************************************************************
 *           CreateFontIndirectA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectA( const LOGFONTA *plfA )
{
    LOGFONTW lfW;

    if (!plfA) return 0;

    FONT_LogFontAToW( plfA, &lfW );
    return CreateFontIndirectW( &lfW );
}

/***********************************************************************
 *           CreateFontIndirectW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectW( const LOGFONTW *plf )
{
    ENUMLOGFONTEXDVW exdv;

    if (!plf) return 0;

    exdv.elfEnumLogfontEx.elfLogFont = *plf;
    exdv.elfEnumLogfontEx.elfFullName[0] = 0;
    exdv.elfEnumLogfontEx.elfStyle[0] = 0;
    exdv.elfEnumLogfontEx.elfScript[0] = 0;
    return CreateFontIndirectExW( &exdv );
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
        lstrcpynW(logfont.lfFaceName, name, ARRAY_SIZE(logfont.lfFaceName));
    else
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectW( &logfont );
}

#define ASSOC_CHARSET_OEM    1
#define ASSOC_CHARSET_ANSI   2
#define ASSOC_CHARSET_SYMBOL 4

static DWORD get_associated_charset_info(void)
{
    static DWORD associated_charset = -1;

    if (associated_charset == -1)
    {
        static const WCHAR assoc_charset_reg_keyW[] = {'S','y','s','t','e','m','\\',
            'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
            'C','o','n','t','r','o','l','\\','F','o','n','t','A','s','s','o','c','\\',
            'A','s','s','o','c','i','a','t','e','d',' ','C','h','a','r','s','e','t','\0'};
        static const WCHAR ansiW[] = {'A','N','S','I','(','0','0',')','\0'};
        static const WCHAR oemW[] = {'O','E','M','(','F','F',')','\0'};
        static const WCHAR symbolW[] = {'S','Y','M','B','O','L','(','0','2',')','\0'};
        static const WCHAR yesW[] = {'Y','E','S','\0'};
        HKEY hkey;
        WCHAR dataW[32];
        DWORD type, data_len;

        associated_charset = 0;

        if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                        assoc_charset_reg_keyW, &hkey) != ERROR_SUCCESS)
            return 0;

        data_len = sizeof(dataW);
        if (!RegQueryValueExW(hkey, ansiW, NULL, &type, (LPBYTE)dataW, &data_len) &&
            type == REG_SZ && !strcmpiW(dataW, yesW))
            associated_charset |= ASSOC_CHARSET_ANSI;

        data_len = sizeof(dataW);
        if (!RegQueryValueExW(hkey, oemW, NULL, &type, (LPBYTE)dataW, &data_len) &&
            type == REG_SZ && !strcmpiW(dataW, yesW))
            associated_charset |= ASSOC_CHARSET_OEM;

        data_len = sizeof(dataW);
        if (!RegQueryValueExW(hkey, symbolW, NULL, &type, (LPBYTE)dataW, &data_len) &&
            type == REG_SZ && !strcmpiW(dataW, yesW))
            associated_charset |= ASSOC_CHARSET_SYMBOL;

        RegCloseKey(hkey);

        TRACE("associated_charset = %d\n", associated_charset);
    }

    return associated_charset;
}

static void update_font_code_page( DC *dc, HANDLE font )
{
    CHARSETINFO csi;
    int charset = get_text_charset_info( dc, NULL, 0 );

    if (charset == ANSI_CHARSET && get_associated_charset_info() & ASSOC_CHARSET_ANSI)
    {
        LOGFONTW lf;

        GetObjectW( font, sizeof(lf), &lf );
        if (!(lf.lfClipPrecision & CLIP_DFA_DISABLE))
            charset = DEFAULT_CHARSET;
    }

    /* Hmm, nicely designed api this one! */
    if (TranslateCharsetInfo( ULongToPtr(charset), &csi, TCI_SRCCHARSET) )
        dc->font_code_page = csi.ciACP;
    else {
        switch(charset) {
        case OEM_CHARSET:
            dc->font_code_page = GetOEMCP();
            break;
        case DEFAULT_CHARSET:
            dc->font_code_page = GetACP();
            break;

        case VISCII_CHARSET:
        case TCVN_CHARSET:
        case KOI8_CHARSET:
        case ISO3_CHARSET:
        case ISO4_CHARSET:
        case ISO10_CHARSET:
        case CELTIC_CHARSET:
            /* FIXME: These have no place here, but because x11drv
               enumerates fonts with these (made up) charsets some apps
               might use them and then the FIXME below would become
               annoying.  Now we could pick the intended codepage for
               each of these, but since it's broken anyway we'll just
               use CP_ACP and hope it'll go away...
            */
            dc->font_code_page = CP_ACP;
            break;

        default:
            FIXME("Can't find codepage for charset %d\n", charset);
            dc->font_code_page = CP_ACP;
            break;
        }
    }

    TRACE("charset %d => cp %d\n", charset, dc->font_code_page);
}

static struct font_gamma_ramp *get_font_gamma_ramp( void )
{
    static const WCHAR desktopW[] = { 'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                      'D','e','s','k','t','o','p',0 };
    static const WCHAR smoothing_gamma[] = { 'F','o','n','t','S','m','o','o','t','h','i','n','g',
                                             'G','a','m','m','a',0 };
    const DWORD gamma_default = 1400;
    struct font_gamma_ramp *ramp;
    DWORD  i, gamma;
    HKEY key;

    ramp = HeapAlloc( GetProcessHeap(), 0, sizeof(*ramp) );
    if ( ramp == NULL) return NULL;

    gamma = gamma_default;
    if (RegOpenKeyW( HKEY_CURRENT_USER, desktopW, &key ) == ERROR_SUCCESS)
    {
        if (get_key_value( key, smoothing_gamma, &gamma ) || gamma == 0)
            gamma = gamma_default;
        RegCloseKey( key );

        gamma = min( max( gamma, 1000 ), 2200 );
    }

    /* Calibrating the difference between the registry value and the Wine gamma value.
       This looks roughly similar to Windows Native with the same registry value.
       MS GDI seems to be rasterizing the outline at a different rate than FreeType. */
    gamma = 1000 * gamma / 1400;

    for (i = 0; i < 256; i++)
    {
        ramp->encode[i] = pow( i / 255., 1000. / gamma ) * 255. + .5;
        ramp->decode[i] = pow( i / 255., gamma / 1000. ) * 255. + .5;
    }

    ramp->gamma = gamma;
    TRACE("gamma %d\n", ramp->gamma);

    return ramp;
}

/***********************************************************************
 *           FONT_SelectObject
 */
static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, HDC hdc )
{
    HGDIOBJ ret = 0;
    DC *dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    UINT aa_flags = 0;

    if (!dc) return 0;

    if (!GDI_inc_ref_count( handle ))
    {
        release_dc_ptr( dc );
        return 0;
    }

    physdev = GET_DC_PHYSDEV( dc, pSelectFont );
    if (physdev->funcs->pSelectFont( physdev, handle, &aa_flags ))
    {
        ret = dc->hFont;
        dc->hFont = handle;
        dc->aa_flags = aa_flags ? aa_flags : GGO_BITMAP;
        update_font_code_page( dc, handle );
        if (dc->font_gamma_ramp == NULL)
            dc->font_gamma_ramp = get_font_gamma_ramp();
        GDI_dec_ref_count( ret );
    }
    else GDI_dec_ref_count( handle );

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           FONT_GetObjectA
 */
static INT FONT_GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer )
{
    FONTOBJ *font = GDI_GetObjPtr( handle, OBJ_FONT );
    LOGFONTA lfA;

    if (!font) return 0;
    if (buffer)
    {
        FONT_LogFontWToA( &font->logfont, &lfA );
        if (count > sizeof(lfA)) count = sizeof(lfA);
        memcpy( buffer, &lfA, count );
    }
    else count = sizeof(lfA);
    GDI_ReleaseObj( handle );
    return count;
}

/***********************************************************************
 *           FONT_GetObjectW
 */
static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer )
{
    FONTOBJ *font = GDI_GetObjPtr( handle, OBJ_FONT );

    if (!font) return 0;
    if (buffer)
    {
        if (count > sizeof(LOGFONTW)) count = sizeof(LOGFONTW);
        memcpy( buffer, &font->logfont, count );
    }
    else count = sizeof(LOGFONTW);
    GDI_ReleaseObj( handle );
    return count;
}


/***********************************************************************
 *           FONT_DeleteObject
 */
static BOOL FONT_DeleteObject( HGDIOBJ handle )
{
    FONTOBJ *obj;

    if (!(obj = free_gdi_handle( handle ))) return FALSE;
    HeapFree( GetProcessHeap(), 0, obj );
    return TRUE;
}


/***********************************************************************
 *           nulldrv_SelectFont
 */
HFONT CDECL nulldrv_SelectFont( PHYSDEV dev, HFONT font, UINT *aa_flags )
{
    static const WCHAR desktopW[] = { 'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                      'D','e','s','k','t','o','p',0 };
    static int orientation = -1, smoothing = -1;
    LOGFONTW lf;
    HKEY key;

    if (*aa_flags) return 0;

    GetObjectW( font, sizeof(lf), &lf );
    switch (lf.lfQuality)
    {
    case NONANTIALIASED_QUALITY:
        *aa_flags = GGO_BITMAP;
        break;
    case ANTIALIASED_QUALITY:
        *aa_flags = GGO_GRAY4_BITMAP;
        break;
    case CLEARTYPE_QUALITY:
    case CLEARTYPE_NATURAL_QUALITY:
        if (orientation == -1)
        {
            if (RegOpenKeyW( HKEY_CURRENT_USER, desktopW, &key )) break;
            orientation = get_subpixel_orientation( key );
            RegCloseKey( key );
        }
        *aa_flags = orientation;
        break;
    default:
        if (smoothing == -1)
        {
            if (RegOpenKeyW( HKEY_CURRENT_USER, desktopW, &key )) break;
            smoothing = get_default_smoothing( key );
            RegCloseKey( key );
        }
        *aa_flags = smoothing;
        break;
    }
    return 0;
}


/***********************************************************************
 *              FONT_EnumInstance
 *
 * Note: plf is really an ENUMLOGFONTEXW, and ptm is a NEWTEXTMETRICEXW.
 *       We have to use other types because of the FONTENUMPROCW definition.
 */
static INT CALLBACK FONT_EnumInstance( const LOGFONTW *plf, const TEXTMETRICW *ptm,
                                       DWORD fType, LPARAM lp )
{
    struct font_enum *pfe = (struct font_enum *)lp;
    INT ret = 1;

    /* lfCharSet is at the same offset in both LOGFONTA and LOGFONTW */
    if ((!pfe->lpLogFontParam ||
        pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET ||
        pfe->lpLogFontParam->lfCharSet == plf->lfCharSet) &&
       (!(fType & RASTER_FONTTYPE) || GetDeviceCaps(pfe->hdc, TEXTCAPS) & TC_RA_ABLE) )
    {
	/* convert font metrics */
        ENUMLOGFONTEXA logfont;
        NEWTEXTMETRICEXA tmA;

        if (!pfe->unicode)
        {
            FONT_EnumLogFontExWToA( (const ENUMLOGFONTEXW *)plf, &logfont);
            FONT_NewTextMetricExWToA( (const NEWTEXTMETRICEXW *)ptm, &tmA );
            plf = (LOGFONTW *)&logfont.elfLogFont;
            ptm = (TEXTMETRICW *)&tmA;
        }
        ret = pfe->lpEnumFunc( plf, ptm, fType, pfe->lpData );
        pfe->retval = ret;
    }
    return ret;
}

/***********************************************************************
 *		FONT_EnumFontFamiliesEx
 */
static INT FONT_EnumFontFamiliesEx( HDC hDC, LPLOGFONTW plf, FONTENUMPROCW efproc,
                                    LPARAM lParam, BOOL unicode )
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hDC );
    struct font_enum fe;

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pEnumFonts );

        if (plf) TRACE("lfFaceName = %s lfCharset = %d\n", debugstr_w(plf->lfFaceName), plf->lfCharSet);
        fe.lpLogFontParam = plf;
        fe.lpEnumFunc = efproc;
        fe.lpData = lParam;
        fe.unicode = unicode;
        fe.hdc = hDC;
        fe.retval = 1;
        ret = physdev->funcs->pEnumFonts( physdev, plf, FONT_EnumInstance, (LPARAM)&fe );
        release_dc_ptr( dc );
    }
    return ret ? fe.retval : 0;
}

/***********************************************************************
 *              EnumFontFamiliesExW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExW( HDC hDC, LPLOGFONTW plf,
                                    FONTENUMPROCW efproc,
                                    LPARAM lParam, DWORD dwFlags )
{
    return FONT_EnumFontFamiliesEx( hDC, plf, efproc, lParam, TRUE );
}

/***********************************************************************
 *              EnumFontFamiliesExA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExA( HDC hDC, LPLOGFONTA plf,
                                    FONTENUMPROCA efproc,
                                    LPARAM lParam, DWORD dwFlags)
{
    LOGFONTW lfW, *plfW;

    if (plf)
    {
        FONT_LogFontAToW( plf, &lfW );
        plfW = &lfW;
    }
    else plfW = NULL;

    return FONT_EnumFontFamiliesEx( hDC, plfW, (FONTENUMPROCW)efproc, lParam, FALSE );
}

/***********************************************************************
 *              EnumFontFamiliesA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesA( HDC hDC, LPCSTR lpFamily,
                                  FONTENUMPROCA efproc, LPARAM lpData )
{
    LOGFONTA lf, *plf;

    if (lpFamily)
    {
        if (!*lpFamily) return 1;
        lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        plf = &lf;
    }
    else plf = NULL;

    return EnumFontFamiliesExA( hDC, plf, efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamiliesW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesW( HDC hDC, LPCWSTR lpFamily,
                                  FONTENUMPROCW efproc, LPARAM lpData )
{
    LOGFONTW lf, *plf;

    if (lpFamily)
    {
        if (!*lpFamily) return 1;
        lstrcpynW( lf.lfFaceName, lpFamily, LF_FACESIZE );
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        plf = &lf;
    }
    else plf = NULL;

    return EnumFontFamiliesExW( hDC, plf, efproc, lpData, 0 );
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
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return 0x80000000;
    ret = dc->charExtra;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI SetTextCharacterExtra( HDC hdc, INT extra )
{
    INT ret = 0x80000000;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetTextCharacterExtra );
        extra = physdev->funcs->pSetTextCharacterExtra( physdev, extra );
        if (extra != 0x80000000)
        {
            ret = dc->charExtra;
            dc->charExtra = extra;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextJustification    (GDI32.@)
 */
BOOL WINAPI SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    BOOL ret;
    PHYSDEV physdev;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;

    physdev = GET_DC_PHYSDEV( dc, pSetTextJustification );
    ret = physdev->funcs->pSetTextJustification( physdev, extra, breaks );
    if (ret)
    {
        extra = abs((extra * dc->vport_ext.cx + dc->wnd_ext.cx / 2) / dc->wnd_ext.cx);
        if (!extra) breaks = 0;
        if (breaks)
        {
            dc->breakExtra = extra / breaks;
            dc->breakRem   = extra - (breaks * dc->breakExtra);
        }
        else
        {
            dc->breakExtra = 0;
            dc->breakRem   = 0;
        }
    }
    release_dc_ptr( dc );
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
    {
        if (count)
        {
            res = WideCharToMultiByte(CP_ACP, 0, nameW, -1, name, count, NULL, NULL);
            if (res == 0)
                res = count;
            name[count-1] = 0;
            /* GetTextFaceA does NOT include the nul byte in the return count.  */
            res--;
        }
        else
            res = 0;
    }
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
    PHYSDEV dev;
    INT ret;

    DC * dc = get_dc_ptr( hdc );
    if (!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetTextFace );
    ret = dev->funcs->pGetTextFace( dev, count, name );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.@)
 *
 * See GetTextExtentPoint32W.
 */
BOOL WINAPI GetTextExtentPoint32A( HDC hdc, LPCSTR str, INT count,
                                     LPSIZE size )
{
    BOOL ret = FALSE;
    INT wlen;
    LPWSTR p;

    if (count < 0) return FALSE;

    p = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    if (p)
    {
	ret = GetTextExtentPoint32W( hdc, p, wlen, size );
	HeapFree( GetProcessHeap(), 0, p );
    }

    TRACE("(%p %s %d %p): returning %d x %d\n",
          hdc, debugstr_an (str, count), count, size, size->cx, size->cy );
    return ret;
}


/***********************************************************************
 * GetTextExtentPoint32W [GDI32.@]
 *
 * Computes width/height for a string.
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
    return GetTextExtentExPointW(hdc, str, count, 0, NULL, NULL, size);
}

/***********************************************************************
 * GetTextExtentExPointI [GDI32.@]
 *
 * Computes width and height of the array of glyph indices.
 *
 * PARAMS
 *    hdc     [I] Handle of device context.
 *    indices [I] Glyph index array.
 *    count   [I] Number of glyphs in array.
 *    max_ext [I] Maximum width in glyphs.
 *    nfit    [O] Maximum number of characters.
 *    dxs     [O] Partial string widths.
 *    size    [O] Returned string size.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentExPointI( HDC hdc, const WORD *indices, INT count, INT max_ext,
                                   LPINT nfit, LPINT dxs, LPSIZE size )
{
    DC *dc;
    int i;
    BOOL ret;
    INT buffer[256], *pos = dxs;

    if (count < 0) return FALSE;

    dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if (!dxs)
    {
        pos = buffer;
        if (count > 256 && !(pos = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*pos) )))
        {
            release_dc_ptr( dc );
            return FALSE;
        }
    }

    ret = get_char_positions_indices( dc, indices, count, pos, size );
    if (ret)
    {
        if (dxs || nfit)
        {
            for (i = 0; i < count; i++)
            {
                unsigned int dx = abs( INTERNAL_XDSTOWS( dc, pos[i] )) + (i + 1) * dc->charExtra;
                if (nfit && dx > (unsigned int)max_ext) break;
                if (dxs) dxs[i] = dx;
            }
            if (nfit) *nfit = i;
        }

        size->cx = abs( INTERNAL_XDSTOWS( dc, size->cx )) + count * dc->charExtra;
        size->cy = abs( INTERNAL_YDSTOWS( dc, size->cy ));
    }

    if (pos != buffer && pos != dxs) HeapFree( GetProcessHeap(), 0, pos );
    release_dc_ptr( dc );

    TRACE("(%p %p %d %p): returning %d x %d\n",
          hdc, indices, count, size, size->cx, size->cy );
    return ret;
}

/***********************************************************************
 * GetTextExtentPointI [GDI32.@]
 *
 * Computes width and height of the array of glyph indices.
 *
 * PARAMS
 *    hdc     [I] Handle of device context.
 *    indices [I] Glyph index array.
 *    count   [I] Number of glyphs in array.
 *    size    [O] Returned string size.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentPointI( HDC hdc, const WORD *indices, INT count, LPSIZE size )
{
    return GetTextExtentExPointI( hdc, indices, count, 0, NULL, NULL, size );
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
    INT *walpDx = NULL;
    LPWSTR p = NULL;

    if (count < 0) return FALSE;
    if (maxExt < -1) return FALSE;

    if (alpDx)
    {
        walpDx = HeapAlloc( GetProcessHeap(), 0, count * sizeof(INT) );
        if (!walpDx) return FALSE;
    }

    p = FONT_mbtowc(hdc, str, count, &wlen, NULL);
    ret = GetTextExtentExPointW( hdc, p, wlen, maxExt, lpnFit, walpDx, size);
    if (walpDx)
    {
        INT n = lpnFit ? *lpnFit : wlen;
        INT i, j;
        for(i = 0, j = 0; i < n; i++, j++)
        {
            alpDx[j] = walpDx[i];
            if (IsDBCSLeadByte(str[j])) alpDx[++j] = walpDx[i];
        }
    }
    if (lpnFit) *lpnFit = WideCharToMultiByte(CP_ACP,0,p,*lpnFit,NULL,0,NULL,NULL);
    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, walpDx );
    return ret;
}


/***********************************************************************
 *           GetTextExtentExPointW    (GDI32.@)
 *
 * Return the size of the string as it would be if it was output properly by
 * e.g. TextOut.
 */
BOOL WINAPI GetTextExtentExPointW( HDC hdc, LPCWSTR str, INT count, INT max_ext,
                                   LPINT nfit, LPINT dxs, LPSIZE size )
{
    DC *dc;
    int i;
    BOOL ret;
    INT buffer[256], *pos = dxs;

    if (count < 0) return FALSE;

    dc = get_dc_ptr(hdc);
    if (!dc) return FALSE;

    if (!dxs)
    {
        pos = buffer;
        if (count > 256 && !(pos = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*pos) )))
        {
            release_dc_ptr( dc );
            return FALSE;
        }
    }

    ret = get_char_positions( dc, str, count, pos, size );
    if (ret)
    {
        if (dxs || nfit)
        {
            for (i = 0; i < count; i++)
            {
                unsigned int dx = abs( INTERNAL_XDSTOWS( dc, pos[i] )) + (i + 1) * dc->charExtra;
                if (nfit && dx > (unsigned int)max_ext) break;
		if (dxs) dxs[i] = dx;
            }
            if (nfit) *nfit = i;
        }

        size->cx = abs( INTERNAL_XDSTOWS( dc, size->cx )) + count * dc->charExtra;
        size->cy = abs( INTERNAL_YDSTOWS( dc, size->cy ));
    }

    if (pos != buffer && pos != dxs) HeapFree( GetProcessHeap(), 0, pos );
    release_dc_ptr( dc );

    TRACE("(%p, %s, %d) returning %dx%d\n", hdc, debugstr_wn(str,count), max_ext, size->cx, size->cy );
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
    PHYSDEV physdev;
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    physdev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    ret = physdev->funcs->pGetTextMetrics( physdev, metrics );

    if (ret)
    {
    /* device layer returns values in device units
     * therefore we have to convert them to logical */

        metrics->tmDigitizedAspectX = GetDeviceCaps(hdc, LOGPIXELSX);
        metrics->tmDigitizedAspectY = GetDeviceCaps(hdc, LOGPIXELSY);
        metrics->tmHeight           = height_to_LP( dc, metrics->tmHeight );
        metrics->tmAscent           = height_to_LP( dc, metrics->tmAscent );
        metrics->tmDescent          = height_to_LP( dc, metrics->tmDescent );
        metrics->tmInternalLeading  = height_to_LP( dc, metrics->tmInternalLeading );
        metrics->tmExternalLeading  = height_to_LP( dc, metrics->tmExternalLeading );
        metrics->tmAveCharWidth     = width_to_LP( dc, metrics->tmAveCharWidth );
        metrics->tmMaxCharWidth     = width_to_LP( dc, metrics->tmMaxCharWidth );
        metrics->tmOverhang         = width_to_LP( dc, metrics->tmOverhang );
        ret = TRUE;

        TRACE("text metrics:\n"
          "    Weight = %03i\t FirstChar = %i\t AveCharWidth = %i\n"
          "    Italic = % 3i\t LastChar = %i\t\t MaxCharWidth = %i\n"
          "    UnderLined = %01i\t DefaultChar = %i\t Overhang = %i\n"
          "    StruckOut = %01i\t BreakChar = %i\t CharSet = %i\n"
          "    PitchAndFamily = %02x\n"
          "    --------------------\n"
          "    InternalLeading = %i\n"
          "    Ascent = %i\n"
          "    Descent = %i\n"
          "    Height = %i\n",
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
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetOutlineTextMetricsA (GDI32.@)
 * Gets metrics for TrueType fonts.
 *
 * NOTES
 *    If the supplied buffer isn't big enough Windows partially fills it up to
 *    its given length and returns that length.
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
    OUTLINETEXTMETRICA *output = lpOTM;
    INT left, len;

    if((ret = GetOutlineTextMetricsW(hdc, 0, NULL)) == 0)
        return 0;
    if(ret > sizeof(buf))
	lpOTMW = HeapAlloc(GetProcessHeap(), 0, ret);
    GetOutlineTextMetricsW(hdc, ret, lpOTMW);

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

    TRACE("needed = %d\n", needed);
    if(needed > cbData)
        /* Since the supplied buffer isn't big enough, we'll alloc one
           that is and memcpy the first cbData bytes into the lpOTM at
           the end. */
        output = HeapAlloc(GetProcessHeap(), 0, needed);

    ret = output->otmSize = min(needed, cbData);
    FONT_TextMetricWToA( &lpOTMW->otmTextMetrics, &output->otmTextMetrics );
    output->otmFiller = 0;
    output->otmPanoseNumber = lpOTMW->otmPanoseNumber;
    output->otmfsSelection = lpOTMW->otmfsSelection;
    output->otmfsType = lpOTMW->otmfsType;
    output->otmsCharSlopeRise = lpOTMW->otmsCharSlopeRise;
    output->otmsCharSlopeRun = lpOTMW->otmsCharSlopeRun;
    output->otmItalicAngle = lpOTMW->otmItalicAngle;
    output->otmEMSquare = lpOTMW->otmEMSquare;
    output->otmAscent = lpOTMW->otmAscent;
    output->otmDescent = lpOTMW->otmDescent;
    output->otmLineGap = lpOTMW->otmLineGap;
    output->otmsCapEmHeight = lpOTMW->otmsCapEmHeight;
    output->otmsXHeight = lpOTMW->otmsXHeight;
    output->otmrcFontBox = lpOTMW->otmrcFontBox;
    output->otmMacAscent = lpOTMW->otmMacAscent;
    output->otmMacDescent = lpOTMW->otmMacDescent;
    output->otmMacLineGap = lpOTMW->otmMacLineGap;
    output->otmusMinimumPPEM = lpOTMW->otmusMinimumPPEM;
    output->otmptSubscriptSize = lpOTMW->otmptSubscriptSize;
    output->otmptSubscriptOffset = lpOTMW->otmptSubscriptOffset;
    output->otmptSuperscriptSize = lpOTMW->otmptSuperscriptSize;
    output->otmptSuperscriptOffset = lpOTMW->otmptSuperscriptOffset;
    output->otmsStrikeoutSize = lpOTMW->otmsStrikeoutSize;
    output->otmsStrikeoutPosition = lpOTMW->otmsStrikeoutPosition;
    output->otmsUnderscoreSize = lpOTMW->otmsUnderscoreSize;
    output->otmsUnderscorePosition = lpOTMW->otmsUnderscorePosition;


    ptr = (char*)(output + 1);
    left = needed - sizeof(*output);

    if(lpOTMW->otmpFamilyName) {
        output->otmpFamilyName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFamilyName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFamilyName = 0;

    if(lpOTMW->otmpFaceName) {
        output->otmpFaceName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFaceName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFaceName = 0;

    if(lpOTMW->otmpStyleName) {
        output->otmpStyleName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpStyleName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpStyleName = 0;

    if(lpOTMW->otmpFullName) {
        output->otmpFullName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFullName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
    } else
        output->otmpFullName = 0;

    assert(left == 0);

    if(output != lpOTM) {
        memcpy(lpOTM, output, cbData);
        HeapFree(GetProcessHeap(), 0, output);

        /* check if the string offsets really fit into the provided size */
        /* FIXME: should we check string length as well? */
        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFamilyName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFamilyName >= lpOTM->otmSize)
                lpOTM->otmpFamilyName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFaceName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFaceName >= lpOTM->otmSize)
                lpOTM->otmpFaceName = 0; /* doesn't fit */
        }

            /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpStyleName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpStyleName >= lpOTM->otmSize)
                lpOTM->otmpStyleName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFullName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFullName >= lpOTM->otmSize)
                lpOTM->otmpFullName = 0; /* doesn't fit */
        }
    }

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
    DC *dc = get_dc_ptr( hdc );
    OUTLINETEXTMETRICW *output = lpOTM;
    PHYSDEV dev;
    UINT ret;

    TRACE("(%p,%d,%p)\n", hdc, cbData, lpOTM);
    if(!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetOutlineTextMetrics );
    ret = dev->funcs->pGetOutlineTextMetrics( dev, cbData, output );

    if (lpOTM && ret > cbData)
    {
        output = HeapAlloc(GetProcessHeap(), 0, ret);
        ret = dev->funcs->pGetOutlineTextMetrics( dev, ret, output );
    }

    if (lpOTM && ret)
    {
        output->otmTextMetrics.tmDigitizedAspectX = GetDeviceCaps(hdc, LOGPIXELSX);
        output->otmTextMetrics.tmDigitizedAspectY = GetDeviceCaps(hdc, LOGPIXELSY);
        output->otmTextMetrics.tmHeight           = height_to_LP( dc, output->otmTextMetrics.tmHeight );
        output->otmTextMetrics.tmAscent           = height_to_LP( dc, output->otmTextMetrics.tmAscent );
        output->otmTextMetrics.tmDescent          = height_to_LP( dc, output->otmTextMetrics.tmDescent );
        output->otmTextMetrics.tmInternalLeading  = height_to_LP( dc, output->otmTextMetrics.tmInternalLeading );
        output->otmTextMetrics.tmExternalLeading  = height_to_LP( dc, output->otmTextMetrics.tmExternalLeading );
        output->otmTextMetrics.tmAveCharWidth     = width_to_LP( dc, output->otmTextMetrics.tmAveCharWidth );
        output->otmTextMetrics.tmMaxCharWidth     = width_to_LP( dc, output->otmTextMetrics.tmMaxCharWidth );
        output->otmTextMetrics.tmOverhang         = width_to_LP( dc, output->otmTextMetrics.tmOverhang );
        output->otmAscent                = height_to_LP( dc, output->otmAscent);
        output->otmDescent               = height_to_LP( dc, output->otmDescent);
        output->otmLineGap               = INTERNAL_YDSTOWS(dc, output->otmLineGap);
        output->otmsCapEmHeight          = INTERNAL_YDSTOWS(dc, output->otmsCapEmHeight);
        output->otmsXHeight              = INTERNAL_YDSTOWS(dc, output->otmsXHeight);
        output->otmrcFontBox.top         = height_to_LP( dc, output->otmrcFontBox.top);
        output->otmrcFontBox.bottom      = height_to_LP( dc, output->otmrcFontBox.bottom);
        output->otmrcFontBox.left        = width_to_LP( dc, output->otmrcFontBox.left);
        output->otmrcFontBox.right       = width_to_LP( dc, output->otmrcFontBox.right);
        output->otmMacAscent             = height_to_LP( dc, output->otmMacAscent);
        output->otmMacDescent            = height_to_LP( dc, output->otmMacDescent);
        output->otmMacLineGap            = INTERNAL_YDSTOWS(dc, output->otmMacLineGap);
        output->otmptSubscriptSize.x     = width_to_LP( dc, output->otmptSubscriptSize.x);
        output->otmptSubscriptSize.y     = height_to_LP( dc, output->otmptSubscriptSize.y);
        output->otmptSubscriptOffset.x   = width_to_LP( dc, output->otmptSubscriptOffset.x);
        output->otmptSubscriptOffset.y   = height_to_LP( dc, output->otmptSubscriptOffset.y);
        output->otmptSuperscriptSize.x   = width_to_LP( dc, output->otmptSuperscriptSize.x);
        output->otmptSuperscriptSize.y   = height_to_LP( dc, output->otmptSuperscriptSize.y);
        output->otmptSuperscriptOffset.x = width_to_LP( dc, output->otmptSuperscriptOffset.x);
        output->otmptSuperscriptOffset.y = height_to_LP( dc, output->otmptSuperscriptOffset.y);
        output->otmsStrikeoutSize        = INTERNAL_YDSTOWS(dc, output->otmsStrikeoutSize);
        output->otmsStrikeoutPosition    = height_to_LP( dc, output->otmsStrikeoutPosition);
        output->otmsUnderscoreSize       = height_to_LP( dc, output->otmsUnderscoreSize);
        output->otmsUnderscorePosition   = height_to_LP( dc, output->otmsUnderscorePosition);

        if(output != lpOTM)
        {
            memcpy(lpOTM, output, cbData);
            HeapFree(GetProcessHeap(), 0, output);
            ret = cbData;
        }
    }
    release_dc_ptr(dc);
    return ret;
}

static LPSTR FONT_GetCharsByRangeA(HDC hdc, UINT firstChar, UINT lastChar, PINT pByteLen)
{
    INT i, count = lastChar - firstChar + 1;
    UINT mbcp;
    UINT c;
    LPSTR str;

    if (count <= 0)
        return NULL;

    mbcp = GdiGetCodePage(hdc);
    switch (mbcp)
    {
    case 932:
    case 936:
    case 949:
    case 950:
    case 1361:
        if (lastChar > 0xffff)
            return NULL;
        if ((firstChar ^ lastChar) > 0xff)
            return NULL;
        break;
    default:
        if (lastChar > 0xff)
            return NULL;
        mbcp = 0;
        break;
    }

    str = HeapAlloc(GetProcessHeap(), 0, count * 2 + 1);
    if (str == NULL)
        return NULL;

    for(i = 0, c = firstChar; c <= lastChar; i++, c++)
    {
        if (mbcp) {
            if (c > 0xff)
                str[i++] = (BYTE)(c >> 8);
            if (c <= 0xff && IsDBCSLeadByteEx(mbcp, c))
                str[i] = 0x1f; /* FIXME: use default character */
            else
                str[i] = (BYTE)c;
        }
        else
            str[i] = (BYTE)c;
    }
    str[i] = '\0';

    *pByteLen = i;

    return str;
}

/***********************************************************************
 *           GetCharWidthW      (GDI32.@)
 *           GetCharWidth32W    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32W( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    UINT i;
    BOOL ret;
    PHYSDEV dev;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;

    dev = GET_DC_PHYSDEV( dc, pGetCharWidth );
    ret = dev->funcs->pGetCharWidth( dev, firstChar, lastChar, buffer );

    if (ret)
    {
        /* convert device units to logical */
        for( i = firstChar; i <= lastChar; i++, buffer++ )
            *buffer = width_to_LP( dc, *buffer );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetCharWidthA      (GDI32.@)
 *           GetCharWidth32A    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32A( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    INT i, wlen;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    str = FONT_GetCharsByRangeA(hdc, firstChar, lastChar, &i);
    if(str == NULL)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, i, &wlen, NULL);

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


/* helper for nulldrv_ExtTextOut */
static DWORD get_glyph_bitmap( HDC hdc, UINT index, UINT flags, UINT aa_flags,
                               GLYPHMETRICS *metrics, struct gdi_image_bits *image )
{
    static const MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };
    UINT indices[3] = {0, 0, 0x20};
    unsigned int i;
    DWORD ret, size;
    int stride;

    indices[0] = index;
    if (flags & ETO_GLYPH_INDEX) aa_flags |= GGO_GLYPH_INDEX;

    for (i = 0; i < ARRAY_SIZE( indices ); i++)
    {
        index = indices[i];
        ret = GetGlyphOutlineW( hdc, index, aa_flags, metrics, 0, NULL, &identity );
        if (ret != GDI_ERROR) break;
    }

    if (ret == GDI_ERROR) return ERROR_NOT_FOUND;
    if (!image) return ERROR_SUCCESS;

    image->ptr = NULL;
    image->free = NULL;
    if (!ret)  /* empty glyph */
    {
        metrics->gmBlackBoxX = metrics->gmBlackBoxY = 0;
        return ERROR_SUCCESS;
    }

    stride = get_dib_stride( metrics->gmBlackBoxX, 1 );
    size = metrics->gmBlackBoxY * stride;

    if (!(image->ptr = HeapAlloc( GetProcessHeap(), 0, size ))) return ERROR_OUTOFMEMORY;
    image->is_copy = TRUE;
    image->free = free_heap_bits;

    ret = GetGlyphOutlineW( hdc, index, aa_flags, metrics, size, image->ptr, &identity );
    if (ret == GDI_ERROR)
    {
        HeapFree( GetProcessHeap(), 0, image->ptr );
        return ERROR_NOT_FOUND;
    }
    return ERROR_SUCCESS;
}

/* helper for nulldrv_ExtTextOut */
static RECT get_total_extents( HDC hdc, INT x, INT y, UINT flags, UINT aa_flags,
                               LPCWSTR str, UINT count, const INT *dx )
{
    UINT i;
    RECT rect, bounds;

    reset_bounds( &bounds );
    for (i = 0; i < count; i++)
    {
        GLYPHMETRICS metrics;

        if (get_glyph_bitmap( hdc, str[i], flags, aa_flags, &metrics, NULL )) continue;

        rect.left   = x + metrics.gmptGlyphOrigin.x;
        rect.top    = y - metrics.gmptGlyphOrigin.y;
        rect.right  = rect.left + metrics.gmBlackBoxX;
        rect.bottom = rect.top  + metrics.gmBlackBoxY;
        add_bounds_rect( &bounds, &rect );

        if (dx)
        {
            if (flags & ETO_PDY)
            {
                x += dx[ i * 2 ];
                y += dx[ i * 2 + 1];
            }
            else x += dx[ i ];
        }
        else
        {
            x += metrics.gmCellIncX;
            y += metrics.gmCellIncY;
        }
    }
    return bounds;
}

/* helper for nulldrv_ExtTextOut */
static void draw_glyph( DC *dc, INT origin_x, INT origin_y, const GLYPHMETRICS *metrics,
                        const struct gdi_image_bits *image, const RECT *clip )
{
    static const BYTE masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    UINT i, count, max_count;
    LONG x, y;
    BYTE *ptr = image->ptr;
    int stride = get_dib_stride( metrics->gmBlackBoxX, 1 );
    POINT *pts;
    RECT rect, clipped_rect;

    rect.left   = origin_x  + metrics->gmptGlyphOrigin.x;
    rect.top    = origin_y  - metrics->gmptGlyphOrigin.y;
    rect.right  = rect.left + metrics->gmBlackBoxX;
    rect.bottom = rect.top  + metrics->gmBlackBoxY;
    if (!clip) clipped_rect = rect;
    else if (!intersect_rect( &clipped_rect, &rect, clip )) return;

    max_count = (metrics->gmBlackBoxX + 1) * metrics->gmBlackBoxY;
    pts = HeapAlloc( GetProcessHeap(), 0, max_count * sizeof(*pts) );
    if (!pts) return;

    count = 0;
    ptr += (clipped_rect.top - rect.top) * stride;
    for (y = clipped_rect.top; y < clipped_rect.bottom; y++, ptr += stride)
    {
        for (x = clipped_rect.left - rect.left; x < clipped_rect.right - rect.left; x++)
        {
            while (x < clipped_rect.right - rect.left && !(ptr[x / 8] & masks[x % 8])) x++;
            pts[count].x = rect.left + x;
            while (x < clipped_rect.right - rect.left && (ptr[x / 8] & masks[x % 8])) x++;
            pts[count + 1].x = rect.left + x;
            if (pts[count + 1].x > pts[count].x)
            {
                pts[count].y = pts[count + 1].y = y;
                count += 2;
            }
        }
    }
    assert( count <= max_count );
    dp_to_lp( dc, pts, count );
    for (i = 0; i < count; i += 2) Polyline( dc->hSelf, pts + i, 2 );
    HeapFree( GetProcessHeap(), 0, pts );
}

/***********************************************************************
 *           nulldrv_ExtTextOut
 */
BOOL CDECL nulldrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
                               LPCWSTR str, UINT count, const INT *dx )
{
    DC *dc = get_nulldrv_dc( dev );
    UINT i;
    DWORD err;
    HGDIOBJ orig;
    HPEN pen;

    if (flags & ETO_OPAQUE)
    {
        RECT rc = *rect;
        HBRUSH brush = CreateSolidBrush( GetNearestColor( dev->hdc, dc->backgroundColor ) );

        if (brush)
        {
            orig = SelectObject( dev->hdc, brush );
            dp_to_lp( dc, (POINT *)&rc, 2 );
            PatBlt( dev->hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
            SelectObject( dev->hdc, orig );
            DeleteObject( brush );
        }
    }

    if (!count) return TRUE;

    if (dc->aa_flags != GGO_BITMAP)
    {
        char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        struct gdi_image_bits bits;
        struct bitblt_coords src, dst;
        PHYSDEV dst_dev;
        /* FIXME Subpixel modes */
        UINT aa_flags = GGO_GRAY4_BITMAP;

        dst_dev = GET_DC_PHYSDEV( dc, pPutImage );
        src.visrect = get_total_extents( dev->hdc, x, y, flags, aa_flags, str, count, dx );
        if (flags & ETO_CLIPPED) intersect_rect( &src.visrect, &src.visrect, rect );
        if (!clip_visrect( dc, &src.visrect, &src.visrect )) return TRUE;

        /* FIXME: check for ETO_OPAQUE and avoid GetImage */
        src.x = src.visrect.left;
        src.y = src.visrect.top;
        src.width = src.visrect.right - src.visrect.left;
        src.height = src.visrect.bottom - src.visrect.top;
        dst = src;
        if ((flags & ETO_OPAQUE) && (src.visrect.left >= rect->left) && (src.visrect.top >= rect->top) &&
            (src.visrect.right <= rect->right) && (src.visrect.bottom <= rect->bottom))
        {
            /* we can avoid the GetImage, just query the needed format */
            memset( &info->bmiHeader, 0, sizeof(info->bmiHeader) );
            info->bmiHeader.biSize   = sizeof(info->bmiHeader);
            info->bmiHeader.biWidth  = src.width;
            info->bmiHeader.biHeight = -src.height;
            info->bmiHeader.biSizeImage = get_dib_image_size( info );
            err = dst_dev->funcs->pPutImage( dst_dev, 0, info, NULL, NULL, NULL, 0 );
            if (!err || err == ERROR_BAD_FORMAT)
            {
                /* make the source rectangle relative to the source bits */
                src.x = src.y = 0;
                src.visrect.left = src.visrect.top = 0;
                src.visrect.right = src.width;
                src.visrect.bottom = src.height;

                bits.ptr = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
                if (!bits.ptr) return ERROR_OUTOFMEMORY;
                bits.is_copy = TRUE;
                bits.free = free_heap_bits;
                err = ERROR_SUCCESS;
            }
        }
        else
        {
            PHYSDEV src_dev = GET_DC_PHYSDEV( dc, pGetImage );
            err = src_dev->funcs->pGetImage( src_dev, info, &bits, &src );
            if (!err && !bits.is_copy)
            {
                void *ptr = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
                if (!ptr)
                {
                    if (bits.free) bits.free( &bits );
                    return ERROR_OUTOFMEMORY;
                }
                memcpy( ptr, bits.ptr, info->bmiHeader.biSizeImage );
                if (bits.free) bits.free( &bits );
                bits.ptr = ptr;
                bits.is_copy = TRUE;
                bits.free = free_heap_bits;
            }
        }
        if (!err)
        {
            /* make x,y relative to the image bits */
            x += src.visrect.left - dst.visrect.left;
            y += src.visrect.top - dst.visrect.top;
            render_aa_text_bitmapinfo( dc, info, &bits, &src, x, y, flags,
                                       aa_flags, str, count, dx );
            err = dst_dev->funcs->pPutImage( dst_dev, 0, info, &bits, &src, &dst, SRCCOPY );
            if (bits.free) bits.free( &bits );
            return !err;
        }
    }

    pen = CreatePen( PS_SOLID, 1, dc->textColor );
    orig = SelectObject( dev->hdc, pen );

    for (i = 0; i < count; i++)
    {
        GLYPHMETRICS metrics;
        struct gdi_image_bits image;

        err = get_glyph_bitmap( dev->hdc, str[i], flags, GGO_BITMAP, &metrics, &image );
        if (err) continue;

        if (image.ptr) draw_glyph( dc, x, y, &metrics, &image, (flags & ETO_CLIPPED) ? rect : NULL );
        if (image.free) image.free( &image );

        if (dx)
        {
            if (flags & ETO_PDY)
            {
                x += dx[ i * 2 ];
                y += dx[ i * 2 + 1];
            }
            else x += dx[ i ];
        }
        else
        {
            x += metrics.gmCellIncX;
            y += metrics.gmCellIncY;
        }
    }

    SelectObject( dev->hdc, orig );
    DeleteObject( pen );
    return TRUE;
}


/***********************************************************************
 *           ExtTextOutA    (GDI32.@)
 *
 * See ExtTextOutW.
 */
BOOL WINAPI ExtTextOutA( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCSTR str, UINT count, const INT *lpDx )
{
    INT wlen;
    UINT codepage;
    LPWSTR p;
    BOOL ret;
    LPINT lpDxW = NULL;

    if (flags & ETO_GLYPH_INDEX)
        return ExtTextOutW( hdc, x, y, flags, lprect, (LPCWSTR)str, count, lpDx );

    p = FONT_mbtowc(hdc, str, count, &wlen, &codepage);

    if (lpDx) {
        unsigned int i = 0, j = 0;

        /* allocate enough for a ETO_PDY */
        lpDxW = HeapAlloc( GetProcessHeap(), 0, 2*wlen*sizeof(INT));
        while(i < count) {
            if(IsDBCSLeadByteEx(codepage, str[i]))
            {
                if(flags & ETO_PDY)
                {
                    lpDxW[j++] = lpDx[i * 2]     + lpDx[(i + 1) * 2];
                    lpDxW[j++] = lpDx[i * 2 + 1] + lpDx[(i + 1) * 2 + 1];
                }
                else
                    lpDxW[j++] = lpDx[i] + lpDx[i + 1];
                i = i + 2;
            }
            else
            {
                if(flags & ETO_PDY)
                {
                    lpDxW[j++] = lpDx[i * 2];
                    lpDxW[j++] = lpDx[i * 2 + 1];
                }
                else
                    lpDxW[j++] = lpDx[i];
                i = i + 1;
            }
        }
    }

    ret = ExtTextOutW( hdc, x, y, flags, lprect, p, wlen, lpDxW );

    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, lpDxW );
    return ret;
}

/***********************************************************************
 *           get_line_width
 *
 * Scale the underline / strikeout line width.
 */
static inline int get_line_width( DC *dc, int metric_size )
{
    int width = abs( INTERNAL_YWSTODS( dc, metric_size ));
    if (width == 0) width = 1;
    if (metric_size < 0) width = -width;
    return width;
}

/***********************************************************************
 *           ExtTextOutW    (GDI32.@)
 *
 * Draws text using the currently selected font, background color, and text color.
 * 
 * 
 * PARAMS
 *    x,y    [I] coordinates of string
 *    flags  [I]
 *        ETO_GRAYED - undocumented on MSDN
 *        ETO_OPAQUE - use background color for fill the rectangle
 *        ETO_CLIPPED - clipping text to the rectangle
 *        ETO_GLYPH_INDEX - Buffer is of glyph locations in fonts rather
 *                          than encoded characters. Implies ETO_IGNORELANGUAGE
 *        ETO_RTLREADING - Paragraph is basically a right-to-left paragraph.
 *                         Affects BiDi ordering
 *        ETO_IGNORELANGUAGE - Undocumented in MSDN - instructs ExtTextOut not to do BiDi reordering
 *        ETO_PDY - unimplemented
 *        ETO_NUMERICSLATIN - unimplemented always assumed -
 *                            do not translate numbers into locale representations
 *        ETO_NUMERICSLOCAL - unimplemented - Numerals in Arabic/Farsi context should assume local form
 *    lprect [I] dimensions for clipping or/and opaquing
 *    str    [I] text string
 *    count  [I] number of symbols in string
 *    lpDx   [I] optional parameter with distance between drawing characters
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCWSTR str, UINT count, const INT *lpDx )
{
    BOOL ret = FALSE;
    LPWSTR reordered_str = (LPWSTR)str;
    WORD *glyphs = NULL;
    UINT align;
    DWORD layout;
    POINT pt;
    TEXTMETRICW tm;
    LOGFONTW lf;
    double cosEsc, sinEsc;
    INT char_extra;
    SIZE sz;
    RECT rc;
    POINT *deltas = NULL, width = {0, 0};
    DWORD type;
    DC * dc = get_dc_ptr( hdc );
    PHYSDEV physdev;
    INT breakRem;
    static int quietfixme = 0;

    if (!dc) return FALSE;

    align = dc->textAlign;
    breakRem = dc->breakRem;
    layout = dc->layout;

    if (quietfixme == 0 && flags & (ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN))
    {
        FIXME("flags ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN unimplemented\n");
        quietfixme = 1;
    }

    update_dc( dc );
    physdev = GET_DC_PHYSDEV( dc, pExtTextOut );
    type = GetObjectType(hdc);
    if(type == OBJ_METADC || type == OBJ_ENHMETADC)
    {
        ret = physdev->funcs->pExtTextOut( physdev, x, y, flags, lprect, str, count, lpDx );
        release_dc_ptr( dc );
        return ret;
    }

    if (flags & ETO_RTLREADING) align |= TA_RTLREADING;
    if (layout & LAYOUT_RTL)
    {
        if ((align & TA_CENTER) != TA_CENTER) align ^= TA_RIGHT;
        align ^= TA_RTLREADING;
    }

    if( !(flags & (ETO_GLYPH_INDEX | ETO_IGNORELANGUAGE)) && count > 0 )
    {
        INT cGlyphs;
        reordered_str = HeapAlloc(GetProcessHeap(), 0, count*sizeof(WCHAR));

        BIDI_Reorder( hdc, str, count, GCP_REORDER,
                      (align & TA_RTLREADING) ? WINE_GCPW_FORCE_RTL : WINE_GCPW_FORCE_LTR,
                      reordered_str, count, NULL, &glyphs, &cGlyphs);

        flags |= ETO_IGNORELANGUAGE;
        if (glyphs)
        {
            flags |= ETO_GLYPH_INDEX;
            if (cGlyphs != count)
                count = cGlyphs;
        }
    }
    else if(flags & ETO_GLYPH_INDEX)
        glyphs = reordered_str;

    TRACE("%p, %d, %d, %08x, %s, %s, %d, %p)\n", hdc, x, y, flags,
          wine_dbgstr_rect(lprect), debugstr_wn(str, count), count, lpDx);
    TRACE("align = %x bkmode = %x mapmode = %x\n", align, dc->backgroundMode, dc->MapMode);

    if(align & TA_UPDATECP)
    {
        pt = dc->cur_pos;
        x = pt.x;
        y = pt.y;
    }

    GetTextMetricsW(hdc, &tm);
    GetObjectW(dc->hFont, sizeof(lf), &lf);

    if(!(tm.tmPitchAndFamily & TMPF_VECTOR)) /* Non-scalable fonts shouldn't be rotated */
        lf.lfEscapement = 0;

    if ((dc->GraphicsMode == GM_COMPATIBLE) &&
        (dc->vport2WorldValid && dc->xformWorld2Vport.eM11 * dc->xformWorld2Vport.eM22 < 0))
    {
        lf.lfEscapement = -lf.lfEscapement;
    }

    if(lf.lfEscapement != 0)
    {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    }
    else
    {
        cosEsc = 1;
        sinEsc = 0;
    }

    if (lprect && (flags & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        rc = *lprect;
        lp_to_dp(dc, (POINT*)&rc, 2);
        order_rect( &rc );
        if (flags & ETO_OPAQUE)
            physdev->funcs->pExtTextOut( physdev, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL );
    }
    else flags &= ~ETO_CLIPPED;

    if(count == 0)
    {
        ret = TRUE;
        goto done;
    }

    pt.x = x;
    pt.y = y;
    lp_to_dp(dc, &pt, 1);
    x = pt.x;
    y = pt.y;

    char_extra = GetTextCharacterExtra(hdc);
    if (char_extra && lpDx && GetDeviceCaps( hdc, TECHNOLOGY ) == DT_RASPRINTER)
        char_extra = 0; /* Printer drivers don't add char_extra if lpDx is supplied */

    if(char_extra || dc->breakExtra || breakRem || lpDx || lf.lfEscapement != 0)
    {
        UINT i;
        POINT total = {0, 0}, desired[2];

        deltas = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*deltas));
        if (lpDx)
        {
            if (flags & ETO_PDY)
            {
                for (i = 0; i < count; i++)
                {
                    deltas[i].x = lpDx[i * 2] + char_extra;
                    deltas[i].y = -lpDx[i * 2 + 1];
                }
            }
            else
            {
                for (i = 0; i < count; i++)
                {
                    deltas[i].x = lpDx[i] + char_extra;
                    deltas[i].y = 0;
                }
            }
        }
        else
        {
            INT *dx = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*dx) );

            if (flags & ETO_GLYPH_INDEX)
                GetTextExtentExPointI( hdc, glyphs, count, -1, NULL, dx, &sz );
            else
                GetTextExtentExPointW( hdc, reordered_str, count, -1, NULL, dx, &sz );

            deltas[0].x = dx[0];
            deltas[0].y = 0;
            for (i = 1; i < count; i++)
            {
                deltas[i].x = dx[i] - dx[i - 1];
                deltas[i].y = 0;
            }
            HeapFree( GetProcessHeap(), 0, dx );
        }

        for(i = 0; i < count; i++)
        {
            total.x += deltas[i].x;
            total.y += deltas[i].y;

            desired[0].x = desired[0].y = 0;

            desired[1].x =  cosEsc * total.x + sinEsc * total.y;
            desired[1].y = -sinEsc * total.x + cosEsc * total.y;

            lp_to_dp(dc, desired, 2);
            desired[1].x -= desired[0].x;
            desired[1].y -= desired[0].y;

            if (dc->GraphicsMode == GM_COMPATIBLE)
            {
                if (dc->vport2WorldValid && dc->xformWorld2Vport.eM11 < 0)
                    desired[1].x = -desired[1].x;
                if (dc->vport2WorldValid && dc->xformWorld2Vport.eM22 < 0)
                    desired[1].y = -desired[1].y;
            }

            deltas[i].x = desired[1].x - width.x;
            deltas[i].y = desired[1].y - width.y;

            width = desired[1];
        }
        flags |= ETO_PDY;
    }
    else
    {
        POINT desired[2];

        if(flags & ETO_GLYPH_INDEX)
            GetTextExtentPointI(hdc, glyphs, count, &sz);
        else
            GetTextExtentPointW(hdc, reordered_str, count, &sz);
        desired[0].x = desired[0].y = 0;
        desired[1].x = sz.cx;
        desired[1].y = 0;
        lp_to_dp(dc, desired, 2);
        desired[1].x -= desired[0].x;
        desired[1].y -= desired[0].y;

        if (dc->GraphicsMode == GM_COMPATIBLE)
        {
            if (dc->vport2WorldValid && dc->xformWorld2Vport.eM11 < 0)
                desired[1].x = -desired[1].x;
            if (dc->vport2WorldValid && dc->xformWorld2Vport.eM22 < 0)
                desired[1].y = -desired[1].y;
        }
        width = desired[1];
    }

    tm.tmAscent = abs(INTERNAL_YWSTODS(dc, tm.tmAscent));
    tm.tmDescent = abs(INTERNAL_YWSTODS(dc, tm.tmDescent));
    switch( align & (TA_LEFT | TA_RIGHT | TA_CENTER) )
    {
    case TA_LEFT:
        if (align & TA_UPDATECP)
        {
            pt.x = x + width.x;
            pt.y = y + width.y;
            dp_to_lp(dc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;

    case TA_CENTER:
        x -= width.x / 2;
        y -= width.y / 2;
        break;

    case TA_RIGHT:
        x -= width.x;
        y -= width.y;
        if (align & TA_UPDATECP)
        {
            pt.x = x;
            pt.y = y;
            dp_to_lp(dc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;
    }

    switch( align & (TA_TOP | TA_BOTTOM | TA_BASELINE) )
    {
    case TA_TOP:
        y += tm.tmAscent * cosEsc;
        x += tm.tmAscent * sinEsc;
        break;

    case TA_BOTTOM:
        y -= tm.tmDescent * cosEsc;
        x -= tm.tmDescent * sinEsc;
        break;

    case TA_BASELINE:
        break;
    }

    if (dc->backgroundMode != TRANSPARENT)
    {
        if(!((flags & ETO_CLIPPED) && (flags & ETO_OPAQUE)))
        {
            if(!(flags & ETO_OPAQUE) || !lprect ||
               x < rc.left || x + width.x >= rc.right ||
               y - tm.tmAscent < rc.top || y + tm.tmDescent >= rc.bottom)
            {
                RECT text_box;
                text_box.left = x;
                text_box.right = x + width.x;
                text_box.top = y - tm.tmAscent;
                text_box.bottom = y + tm.tmDescent;

                if (flags & ETO_CLIPPED) intersect_rect( &text_box, &text_box, &rc );
                if (!is_rect_empty( &text_box ))
                    physdev->funcs->pExtTextOut( physdev, 0, 0, ETO_OPAQUE, &text_box, NULL, 0, NULL );
            }
        }
    }

    ret = physdev->funcs->pExtTextOut( physdev, x, y, (flags & ~ETO_OPAQUE), &rc,
                                       glyphs ? glyphs : reordered_str, count, (INT*)deltas );

done:
    HeapFree(GetProcessHeap(), 0, deltas);
    if(glyphs != reordered_str)
        HeapFree(GetProcessHeap(), 0, glyphs);
    if(reordered_str != str)
        HeapFree(GetProcessHeap(), 0, reordered_str);

    if (ret && (lf.lfUnderline || lf.lfStrikeOut))
    {
        int underlinePos, strikeoutPos;
        int underlineWidth, strikeoutWidth;
        UINT size = GetOutlineTextMetricsW(hdc, 0, NULL);
        OUTLINETEXTMETRICW* otm = NULL;
        POINT pts[5];
        HPEN hpen = SelectObject(hdc, GetStockObject(NULL_PEN));
        HBRUSH hbrush = CreateSolidBrush(dc->textColor);

        hbrush = SelectObject(hdc, hbrush);

        if(!size)
        {
            underlinePos = 0;
            underlineWidth = tm.tmAscent / 20 + 1;
            strikeoutPos = tm.tmAscent / 2;
            strikeoutWidth = underlineWidth;
        }
        else
        {
            otm = HeapAlloc(GetProcessHeap(), 0, size);
            GetOutlineTextMetricsW(hdc, size, otm);
            underlinePos = abs( INTERNAL_YWSTODS( dc, otm->otmsUnderscorePosition ));
            if (otm->otmsUnderscorePosition < 0) underlinePos = -underlinePos;
            underlineWidth = get_line_width( dc, otm->otmsUnderscoreSize );
            strikeoutPos = abs( INTERNAL_YWSTODS( dc, otm->otmsStrikeoutPosition ));
            if (otm->otmsStrikeoutPosition < 0) strikeoutPos = -strikeoutPos;
            strikeoutWidth = get_line_width( dc, otm->otmsStrikeoutSize );
            HeapFree(GetProcessHeap(), 0, otm);
        }


        if (lf.lfUnderline)
        {
            pts[0].x = x - (underlinePos + underlineWidth / 2) * sinEsc;
            pts[0].y = y - (underlinePos + underlineWidth / 2) * cosEsc;
            pts[1].x = x + width.x - (underlinePos + underlineWidth / 2) * sinEsc;
            pts[1].y = y + width.y - (underlinePos + underlineWidth / 2) * cosEsc;
            pts[2].x = pts[1].x + underlineWidth * sinEsc;
            pts[2].y = pts[1].y + underlineWidth * cosEsc;
            pts[3].x = pts[0].x + underlineWidth * sinEsc;
            pts[3].y = pts[0].y + underlineWidth * cosEsc;
            pts[4].x = pts[0].x;
            pts[4].y = pts[0].y;
            dp_to_lp(dc, pts, 5);
            Polygon(hdc, pts, 5);
        }

        if (lf.lfStrikeOut)
        {
            pts[0].x = x - (strikeoutPos + strikeoutWidth / 2) * sinEsc;
            pts[0].y = y - (strikeoutPos + strikeoutWidth / 2) * cosEsc;
            pts[1].x = x + width.x - (strikeoutPos + strikeoutWidth / 2) * sinEsc;
            pts[1].y = y + width.y - (strikeoutPos + strikeoutWidth / 2) * cosEsc;
            pts[2].x = pts[1].x + strikeoutWidth * sinEsc;
            pts[2].y = pts[1].y + strikeoutWidth * cosEsc;
            pts[3].x = pts[0].x + strikeoutWidth * sinEsc;
            pts[3].y = pts[0].y + strikeoutWidth * cosEsc;
            pts[4].x = pts[0].x;
            pts[4].y = pts[0].y;
            dp_to_lp(dc, pts, 5);
            Polygon(hdc, pts, 5);
        }

        SelectObject(hdc, hpen);
        hbrush = SelectObject(hdc, hbrush);
        DeleteObject(hbrush);
    }

    release_dc_ptr( dc );

    return ret;
}


/***********************************************************************
 *           TextOutA    (GDI32.@)
 */
BOOL WINAPI TextOutA( HDC hdc, INT x, INT y, LPCSTR str, INT count )
{
    return ExtTextOutA( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOutW    (GDI32.@)
 */
BOOL WINAPI TextOutW(HDC hdc, INT x, INT y, LPCWSTR str, INT count)
{
    return ExtTextOutW( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *		PolyTextOutA (GDI32.@)
 *
 * See PolyTextOutW.
 */
BOOL WINAPI PolyTextOutA( HDC hdc, const POLYTEXTA *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutA( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}



/***********************************************************************
 *		PolyTextOutW (GDI32.@)
 *
 * Draw several Strings
 *
 * RETURNS
 *  TRUE:  Success.
 *  FALSE: Failure.
 */
BOOL WINAPI PolyTextOutW( HDC hdc, const POLYTEXTW *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutW( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}


/***********************************************************************
 *           SetMapperFlags    (GDI32.@)
 */
DWORD WINAPI SetMapperFlags( HDC hdc, DWORD flags )
{
    DC *dc = get_dc_ptr( hdc );
    DWORD ret = GDI_ERROR;

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetMapperFlags );
        flags = physdev->funcs->pSetMapperFlags( physdev, flags );
        if (flags != GDI_ERROR)
        {
            ret = dc->mapperFlags;
            dc->mapperFlags = flags;
        }
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *          GetAspectRatioFilterEx  (GDI32.@)
 */
BOOL WINAPI GetAspectRatioFilterEx( HDC hdc, LPSIZE pAspectRatio )
{
  FIXME("(%p, %p): -- Empty Stub !\n", hdc, pAspectRatio);
  return FALSE;
}


/***********************************************************************
 *           GetCharABCWidthsA   (GDI32.@)
 *
 * See GetCharABCWidthsW.
 */
BOOL WINAPI GetCharABCWidthsA(HDC hdc, UINT firstChar, UINT lastChar,
                                  LPABC abc )
{
    INT i, wlen;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    str = FONT_GetCharsByRangeA(hdc, firstChar, lastChar, &i);
    if (str == NULL)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, i, &wlen, NULL);
    if (wstr == NULL)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

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
 * GetCharABCWidthsW [GDI32.@]
 *
 * Retrieves widths of characters in range.
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
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    unsigned int i;
    BOOL ret;
    TEXTMETRICW tm;

    if (!dc) return FALSE;

    if (!abc)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    /* unlike GetCharABCWidthsFloatW, this one is supposed to fail on non-scalable fonts */
    dev = GET_DC_PHYSDEV( dc, pGetTextMetrics );
    if (!dev->funcs->pGetTextMetrics( dev, &tm ) || !(tm.tmPitchAndFamily & TMPF_VECTOR))
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    dev = GET_DC_PHYSDEV( dc, pGetCharABCWidths );
    ret = dev->funcs->pGetCharABCWidths( dev, firstChar, lastChar, abc );
    if (ret)
    {
        /* convert device units to logical */
        for( i = firstChar; i <= lastChar; i++, abc++ ) {
            abc->abcA = width_to_LP(dc, abc->abcA);
            abc->abcB = width_to_LP(dc, abc->abcB);
            abc->abcC = width_to_LP(dc, abc->abcC);
	}
    }

    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * GetCharABCWidthsI [GDI32.@]
 *
 * Retrieves widths of characters in range.
 *
 * PARAMS
 *    hdc       [I] Handle of device context
 *    firstChar [I] First glyphs in range to query
 *    count     [I] Last glyphs in range to query
 *    pgi       [i] Array of glyphs to query
 *    abc       [O] Address of character-width structure
 *
 * NOTES
 *    Only works with TrueType fonts
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsI( HDC hdc, UINT firstChar, UINT count,
                               LPWORD pgi, LPABC abc)
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    unsigned int i;
    BOOL ret;

    if (!dc) return FALSE;

    if (!abc)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    dev = GET_DC_PHYSDEV( dc, pGetCharABCWidthsI );
    ret = dev->funcs->pGetCharABCWidthsI( dev, firstChar, count, pgi, abc );
    if (ret)
    {
        /* convert device units to logical */
        for( i = 0; i < count; i++, abc++ ) {
            abc->abcA = width_to_LP(dc, abc->abcA);
            abc->abcB = width_to_LP(dc, abc->abcB);
            abc->abcC = width_to_LP(dc, abc->abcC);
	}
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetGlyphOutlineA    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineA( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    if (!lpmat2) return GDI_ERROR;

    if(!(fuFormat & GGO_GLYPH_INDEX)) {
        UINT cp;
        int len;
        char mbchs[2];
        WCHAR wChar;

        cp = GdiGetCodePage(hdc);
        if (IsDBCSLeadByteEx(cp, uChar >> 8)) {
            len = 2;
            mbchs[0] = (uChar & 0xff00) >> 8;
            mbchs[1] = (uChar & 0xff);
        } else {
            len = 1;
            mbchs[0] = (uChar & 0xff);
        }
        wChar = 0;
        MultiByteToWideChar(cp, 0, mbchs, len, &wChar, 1);
        uChar = wChar;
    }

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
    DC *dc;
    DWORD ret;
    PHYSDEV dev;

    TRACE("(%p, %04x, %04x, %p, %d, %p, %p)\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );

    if (!lpmat2) return GDI_ERROR;

    dc = get_dc_ptr(hdc);
    if(!dc) return GDI_ERROR;

    dev = GET_DC_PHYSDEV( dc, pGetGlyphOutline );
    ret = dev->funcs->pGetGlyphOutline( dev, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    release_dc_ptr( dc );
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
    LPWSTR lpszResourceFileW = NULL;
    LPWSTR lpszFontFileW = NULL;
    LPWSTR lpszCurrentPathW = NULL;
    int len;
    BOOL ret;

    if (lpszResourceFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszResourceFile, -1, NULL, 0);
        lpszResourceFileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszResourceFile, -1, lpszResourceFileW, len);
    }

    if (lpszFontFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszFontFile, -1, NULL, 0);
        lpszFontFileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszFontFile, -1, lpszFontFileW, len);
    }

    if (lpszCurrentPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszCurrentPath, -1, NULL, 0);
        lpszCurrentPathW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszCurrentPath, -1, lpszCurrentPathW, len);
    }

    ret = CreateScalableFontResourceW(fHidden, lpszResourceFileW,
            lpszFontFileW, lpszCurrentPathW);

    HeapFree(GetProcessHeap(), 0, lpszResourceFileW);
    HeapFree(GetProcessHeap(), 0, lpszFontFileW);
    HeapFree(GetProcessHeap(), 0, lpszCurrentPathW);

    return ret;
}

/***********************************************************************
 *           CreateScalableFontResourceW   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceW( DWORD hidden, LPCWSTR resource_file,
                                         LPCWSTR font_file, LPCWSTR font_path )
{
    TRACE("(%d, %s, %s, %s)\n", hidden, debugstr_w(resource_file),
          debugstr_w(font_file), debugstr_w(font_path) );

    return WineEngCreateScalableFontResource( hidden, resource_file,
                                              font_file, font_path );
}

/*************************************************************************
 *             GetKerningPairsA   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsA( HDC hDC, DWORD cPairs,
                               LPKERNINGPAIR kern_pairA )
{
    UINT cp;
    CPINFO cpi;
    DWORD i, total_kern_pairs, kern_pairs_copied = 0;
    KERNINGPAIR *kern_pairW;

    if (!cPairs && kern_pairA)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    cp = GdiGetCodePage(hDC);

    /* GetCPInfo() will fail on CP_SYMBOL, and WideCharToMultiByte is supposed
     * to fail on an invalid character for CP_SYMBOL.
     */
    cpi.DefaultChar[0] = 0;
    if (cp != CP_SYMBOL && !GetCPInfo(cp, &cpi))
    {
        FIXME("Can't find codepage %u info\n", cp);
        return 0;
    }

    total_kern_pairs = GetKerningPairsW(hDC, 0, NULL);
    if (!total_kern_pairs) return 0;

    kern_pairW = HeapAlloc(GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pairW));
    GetKerningPairsW(hDC, total_kern_pairs, kern_pairW);

    for (i = 0; i < total_kern_pairs; i++)
    {
        char first, second;

        if (!WideCharToMultiByte(cp, 0, &kern_pairW[i].wFirst, 1, &first, 1, NULL, NULL))
            continue;

        if (!WideCharToMultiByte(cp, 0, &kern_pairW[i].wSecond, 1, &second, 1, NULL, NULL))
            continue;

        if (first == cpi.DefaultChar[0] || second == cpi.DefaultChar[0])
            continue;

        if (kern_pairA)
        {
            if (kern_pairs_copied >= cPairs) break;

            kern_pairA->wFirst = (BYTE)first;
            kern_pairA->wSecond = (BYTE)second;
            kern_pairA->iKernAmount = kern_pairW[i].iKernAmount;
            kern_pairA++;
        }
        kern_pairs_copied++;
    }

    HeapFree(GetProcessHeap(), 0, kern_pairW);

    return kern_pairs_copied;
}

/*************************************************************************
 *             GetKerningPairsW   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsW( HDC hDC, DWORD cPairs,
                                 LPKERNINGPAIR lpKerningPairs )
{
    DC *dc;
    DWORD ret;
    PHYSDEV dev;

    TRACE("(%p,%d,%p)\n", hDC, cPairs, lpKerningPairs);

    if (!cPairs && lpKerningPairs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    dc = get_dc_ptr(hDC);
    if (!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetKerningPairs );
    ret = dev->funcs->pGetKerningPairs( dev, cPairs, lpKerningPairs );
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.@]
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondence between different labels
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
  DWORD flags /* [in] determines interpretation of lpSrc */)
{
    int index = 0;
    switch (flags) {
    case TCI_SRCFONTSIG:
      while (index < MAXTCIINDEX && !(*lpSrc>>index & 0x0001)) index++;
      break;
    case TCI_SRCCODEPAGE:
      while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciACP) index++;
      break;
    case TCI_SRCCHARSET:
      while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciCharset) index++;
      break;
    default:
      return FALSE;
    }
    if (index >= MAXTCIINDEX || FONT_tci[index].ciCharset == DEFAULT_CHARSET) return FALSE;
    *lpCs = FONT_tci[index];
    return TRUE;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI32.@)
 */
DWORD WINAPI GetFontLanguageInfo(HDC hdc)
{
	FONTSIGNATURE fontsig;
	static const DWORD GCP_DBCS_MASK=FS_JISJAPAN|FS_CHINESESIMP|FS_WANSUNG|FS_CHINESETRAD|FS_JOHAB,
		GCP_DIACRITIC_MASK=0x00000000,
		FLI_GLYPHS_MASK=0x00000000,
		GCP_GLYPHSHAPE_MASK=FS_ARABIC,
		GCP_KASHIDA_MASK=0x00000000,
		GCP_LIGATE_MASK=0x00000000,
		GCP_REORDER_MASK=FS_HEBREW|FS_ARABIC;

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

	if( GetKerningPairsW( hdc, 0, NULL ) )
		result|=GCP_USEKERNING;

        /* this might need a test for a HEBREW- or ARABIC_CHARSET as well */
        if( GetTextAlign( hdc) & TA_RTLREADING )
            if( (fontsig.fsCsb[0]&GCP_REORDER_MASK)!=0 )
                    result|=GCP_REORDER;

	return result;
}


/*************************************************************************
 * GetFontData [GDI32.@]
 *
 * Retrieve data for TrueType font.
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
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    DWORD ret;

    if(!dc) return GDI_ERROR;

    dev = GET_DC_PHYSDEV( dc, pGetFontData );
    ret = dev->funcs->pGetFontData( dev, table, offset, buffer, length );
    release_dc_ptr( dc );
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

    TRACE("(%p, %s, %d, %p, 0x%x)\n",
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
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    DWORD ret;

    TRACE("(%p, %s, %d, %p, 0x%x)\n",
          hdc, debugstr_wn(lpstr, count), count, pgi, flags);

    if(!dc) return GDI_ERROR;

    dev = GET_DC_PHYSDEV( dc, pGetGlyphIndices );
    ret = dev->funcs->pGetGlyphIndices( dev, lpstr, count, pgi, flags );
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * GetCharacterPlacementA [GDI32.@]
 *
 * See GetCharacterPlacementW.
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
    INT uCountW;
    GCP_RESULTSW resultsW;
    DWORD ret;
    UINT font_cp;

    TRACE("%s, %d, %d, 0x%08x\n",
          debugstr_an(lpString, uCount), uCount, nMaxExtent, dwFlags);

    lpStringW = FONT_mbtowc(hdc, lpString, uCount, &uCountW, &font_cp);

    if (!lpResults)
    {
        ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, NULL, dwFlags);
        HeapFree(GetProcessHeap(), 0, lpStringW);
        return ret;
    }

    /* both structs are equal in size */
    memcpy(&resultsW, lpResults, sizeof(resultsW));

    if(lpResults->lpOutString)
        resultsW.lpOutString = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*uCountW);

    ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, &resultsW, dwFlags);

    lpResults->nGlyphs = resultsW.nGlyphs;
    lpResults->nMaxFit = resultsW.nMaxFit;

    if(lpResults->lpOutString) {
        WideCharToMultiByte(font_cp, 0, resultsW.lpOutString, uCountW,
                            lpResults->lpOutString, uCount, NULL, NULL );
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
 *   The width and height of the string if successful, 0 if failed.
 *
 * BUGS
 *
 *   All flags except GCP_REORDER are not yet implemented.
 *   Reordering is not 100% compliant to the Windows BiDi method.
 *   Caret positioning is not yet implemented for BiDi.
 *   Classes are not yet implemented.
 *
 */
DWORD WINAPI
GetCharacterPlacementW(
        HDC hdc,                    /* [in] Device context for which the rendering is to be done */
        LPCWSTR lpString,           /* [in] The string for which information is to be returned */
        INT uCount,                 /* [in] Number of WORDS in string. */
        INT nMaxExtent,             /* [in] Maximum extent the string is to take (in HDC logical units) */
        GCP_RESULTSW *lpResults,    /* [in/out] A pointer to a GCP_RESULTSW struct */
        DWORD dwFlags               /* [in] Flags specifying how to process the string */
        )
{
    DWORD ret=0;
    SIZE size;
    UINT i, nSet;

    TRACE("%s, %d, %d, 0x%08x\n",
          debugstr_wn(lpString, uCount), uCount, nMaxExtent, dwFlags);

    if (!lpResults)
        return GetTextExtentPoint32W(hdc, lpString, uCount, &size) ? MAKELONG(size.cx, size.cy) : 0;

    TRACE("lStructSize=%d, lpOutString=%p, lpOrder=%p, lpDx=%p, lpCaretPos=%p\n"
          "lpClass=%p, lpGlyphs=%p, nGlyphs=%u, nMaxFit=%d\n",
          lpResults->lStructSize, lpResults->lpOutString, lpResults->lpOrder,
          lpResults->lpDx, lpResults->lpCaretPos, lpResults->lpClass,
          lpResults->lpGlyphs, lpResults->nGlyphs, lpResults->nMaxFit);

    if(dwFlags&(~GCP_REORDER))
        FIXME("flags 0x%08x ignored\n", dwFlags);
    if(lpResults->lpClass)
        FIXME("classes not implemented\n");
    if (lpResults->lpCaretPos && (dwFlags & GCP_REORDER))
        FIXME("Caret positions for complex scripts not implemented\n");

    nSet = (UINT)uCount;
    if(nSet > lpResults->nGlyphs)
        nSet = lpResults->nGlyphs;

    /* return number of initialized fields */
    lpResults->nGlyphs = nSet;

    if((dwFlags&GCP_REORDER)==0 )
    {
        /* Treat the case where no special handling was requested in a fastpath way */
        /* copy will do if the GCP_REORDER flag is not set */
        if(lpResults->lpOutString)
            memcpy( lpResults->lpOutString, lpString, nSet * sizeof(WCHAR));

        if(lpResults->lpOrder)
        {
            for(i = 0; i < nSet; i++)
                lpResults->lpOrder[i] = i;
        }
    }
    else
    {
        BIDI_Reorder(NULL, lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpResults->lpOutString,
                     nSet, lpResults->lpOrder, NULL, NULL );
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

    if (lpResults->lpCaretPos && !(dwFlags & GCP_REORDER))
    {
        int pos = 0;

        lpResults->lpCaretPos[0] = 0;
        for (i = 1; i < nSet; i++)
            if (GetTextExtentPoint32W(hdc, &(lpString[i - 1]), 1, &size))
                lpResults->lpCaretPos[i] = (pos += size.cx);
    }

    if(lpResults->lpGlyphs)
        GetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);

    if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
        ret = MAKELONG(size.cx, size.cy);

    return ret;
}

/*************************************************************************
 *      GetCharABCWidthsFloatA [GDI32.@]
 *
 * See GetCharABCWidthsFloatW.
 */
BOOL WINAPI GetCharABCWidthsFloatA( HDC hdc, UINT first, UINT last, LPABCFLOAT abcf )
{
    INT i, wlen;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    str = FONT_GetCharsByRangeA(hdc, first, last, &i);
    if (str == NULL)
        return FALSE;

    wstr = FONT_mbtowc( hdc, str, i, &wlen, NULL );

    for (i = 0; i < wlen; i++)
    {
        if (!GetCharABCWidthsFloatW( hdc, wstr[i], wstr[i], abcf ))
        {
            ret = FALSE;
            break;
        }
        abcf++;
    }

    HeapFree( GetProcessHeap(), 0, str );
    HeapFree( GetProcessHeap(), 0, wstr );

    return ret;
}

/*************************************************************************
 *      GetCharABCWidthsFloatW [GDI32.@]
 *
 * Retrieves widths of a range of characters.
 *
 * PARAMS
 *    hdc   [I] Handle to device context.
 *    first [I] First character in range to query.
 *    last  [I] Last character in range to query.
 *    abcf  [O] Array of LPABCFLOAT structures.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsFloatW( HDC hdc, UINT first, UINT last, LPABCFLOAT abcf )
{
    UINT i;
    ABC *abc;
    PHYSDEV dev;
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p, %d, %d, %p\n", hdc, first, last, abcf);

    if (!dc) return FALSE;

    if (!abcf) goto done;
    if (!(abc = HeapAlloc( GetProcessHeap(), 0, (last - first + 1) * sizeof(*abc) ))) goto done;

    dev = GET_DC_PHYSDEV( dc, pGetCharABCWidths );
    ret = dev->funcs->pGetCharABCWidths( dev, first, last, abc );
    if (ret)
    {
        /* convert device units to logical */
        FLOAT scale = fabs( dc->xformVport2World.eM11 );
        for (i = first; i <= last; i++, abcf++)
        {
            abcf->abcfA = abc[i - first].abcA * scale;
            abcf->abcfB = abc[i - first].abcB * scale;
            abcf->abcfC = abc[i - first].abcC * scale;
        }
    }
    HeapFree( GetProcessHeap(), 0, abc );

done:
    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 *      GetCharWidthFloatA [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatA(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
    FIXME("%p, %u, %u, %p: stub!\n", hdc, iFirstChar, iLastChar, pxBuffer);
    return FALSE;
}

/*************************************************************************
 *      GetCharWidthFloatW [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatW(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
    FIXME("%p, %u, %u, %p: stub!\n", hdc, iFirstChar, iLastChar, pxBuffer);
    return FALSE;
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

static BOOL CALLBACK load_enumed_resource(HMODULE hModule, LPCWSTR type, LPWSTR name, LONG_PTR lParam)
{
    HRSRC rsrc = FindResourceW(hModule, name, type);
    HGLOBAL hMem = LoadResource(hModule, rsrc);
    LPVOID *pMem = LockResource(hMem);
    int *num_total = (int *)lParam;
    DWORD num_in_res;

    TRACE("Found resource %s - trying to load\n", wine_dbgstr_w(type));
    if (!AddFontMemResourceEx(pMem, SizeofResource(hModule, rsrc), NULL, &num_in_res))
    {
        ERR("Failed to load PE font resource mod=%p ptr=%p\n", hModule, hMem);
        return FALSE;
    }

    *num_total += num_in_res;
    return TRUE;
}

static void *map_file( const WCHAR *filename, LARGE_INTEGER *size )
{
    HANDLE file, mapping;
    void *ptr;

    file = CreateFileW( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if (file == INVALID_HANDLE_VALUE) return NULL;

    if (!GetFileSizeEx( file, size ) || size->u.HighPart)
    {
        CloseHandle( file );
        return NULL;
    }

    mapping = CreateFileMappingW( file, NULL, PAGE_READONLY, 0, 0, NULL );
    CloseHandle( file );
    if (!mapping) return NULL;

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );

    return ptr;
}

static void *find_resource( BYTE *ptr, WORD type, DWORD rsrc_off, DWORD size, DWORD *len )
{
    WORD align, type_id, count;
    DWORD res_off;

    if (size < rsrc_off + 10) return NULL;
    align = *(WORD *)(ptr + rsrc_off);
    rsrc_off += 2;
    type_id = *(WORD *)(ptr + rsrc_off);
    while (type_id && type_id != type)
    {
        count = *(WORD *)(ptr + rsrc_off + 2);
        rsrc_off += 8 + count * 12;
        if (size < rsrc_off + 8) return NULL;
        type_id = *(WORD *)(ptr + rsrc_off);
    }
    if (!type_id) return NULL;
    count = *(WORD *)(ptr + rsrc_off + 2);
    if (size < rsrc_off + 8 + count * 12) return NULL;
    res_off = *(WORD *)(ptr + rsrc_off + 8) << align;
    *len = *(WORD *)(ptr + rsrc_off + 10) << align;
    if (size < res_off + *len) return NULL;
    return ptr + res_off;
}

static WCHAR *get_scalable_filename( const WCHAR *res, BOOL *hidden )
{
    LARGE_INTEGER size;
    BYTE *ptr = map_file( res, &size );
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_OS2_HEADER *ne;
    WORD *fontdir;
    char *data;
    WCHAR *name = NULL;
    DWORD len;

    if (!ptr) return NULL;

    if (size.u.LowPart < sizeof( *dos )) goto fail;
    dos = (const IMAGE_DOS_HEADER *)ptr;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) goto fail;
    if (size.u.LowPart < dos->e_lfanew + sizeof( *ne )) goto fail;
    ne = (const IMAGE_OS2_HEADER *)(ptr + dos->e_lfanew);

    fontdir = find_resource( ptr, 0x8007, dos->e_lfanew + ne->ne_rsrctab, size.u.LowPart, &len );
    if (!fontdir) goto fail;
    *hidden = (fontdir[35] & 0x80) != 0;  /* fontdir->dfType */

    data = find_resource( ptr, 0x80cc, dos->e_lfanew + ne->ne_rsrctab, size.u.LowPart, &len );
    if (!data) goto fail;
    if (!memchr( data, 0, len )) goto fail;

    len = MultiByteToWideChar( CP_ACP, 0, data, -1, NULL, 0 );
    name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (name) MultiByteToWideChar( CP_ACP, 0, data, -1, name, len );

fail:
    UnmapViewOfFile( ptr );
    return name;
}

/***********************************************************************
 *           AddFontResourceExW    (GDI32.@)
 */
INT WINAPI AddFontResourceExW( LPCWSTR str, DWORD fl, PVOID pdv )
{
    int ret = WineEngAddFontResourceEx(str, fl, pdv);
    WCHAR *filename;
    BOOL hidden;

    if (ret == 0)
    {
        /* FreeType <2.3.5 has problems reading resources wrapped in PE files. */
        HMODULE hModule = LoadLibraryExW(str, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule != NULL)
        {
            int num_resources = 0;
            LPWSTR rt_font = (LPWSTR)((ULONG_PTR)8);  /* we don't want to include winuser.h */

            TRACE("WineEngAddFontResourceEx failed on PE file %s - trying to load resources manually\n",
                wine_dbgstr_w(str));
            if (EnumResourceNamesW(hModule, rt_font, load_enumed_resource, (LONG_PTR)&num_resources))
                ret = num_resources;
            FreeLibrary(hModule);
        }
        else if ((filename = get_scalable_filename( str, &hidden )) != NULL)
        {
            if (hidden) fl |= FR_PRIVATE | FR_NOT_ENUM;
            ret = WineEngAddFontResourceEx( filename, fl, pdv );
            HeapFree( GetProcessHeap(), 0, filename );
        }
    }
    return ret;
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
 *           AddFontMemResourceEx    (GDI32.@)
 */
HANDLE WINAPI AddFontMemResourceEx( PVOID pbFont, DWORD cbFont, PVOID pdv, DWORD *pcFonts)
{
    HANDLE ret;
    DWORD num_fonts;

    if (!pbFont || !cbFont || !pcFonts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    ret = WineEngAddFontMemResourceEx(pbFont, cbFont, pdv, &num_fonts);
    if (ret)
    {
        __TRY
        {
            *pcFonts = num_fonts;
        }
        __EXCEPT_PAGE_FAULT
        {
            WARN("page fault while writing to *pcFonts (%p)\n", pcFonts);
            RemoveFontMemResourceEx(ret);
            ret = 0;
        }
        __ENDTRY
    }
    return ret;
}

/***********************************************************************
 *           RemoveFontMemResourceEx    (GDI32.@)
 */
BOOL WINAPI RemoveFontMemResourceEx( HANDLE fh )
{
    FIXME("(%p) stub\n", fh);
    return TRUE;
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
    int ret = WineEngRemoveFontResourceEx( str, fl, pdv );
    WCHAR *filename;
    BOOL hidden;

    if (ret == 0)
    {
        /* FreeType <2.3.5 has problems reading resources wrapped in PE files. */
        HMODULE hModule = LoadLibraryExW(str, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule != NULL)
        {
            WARN("Can't unload resources from PE file %s\n", wine_dbgstr_w(str));
            FreeLibrary(hModule);
        }
        else if ((filename = get_scalable_filename( str, &hidden )) != NULL)
        {
            if (hidden) fl |= FR_PRIVATE | FR_NOT_ENUM;
            ret = WineEngRemoveFontResourceEx( filename, fl, pdv );
            HeapFree( GetProcessHeap(), 0, filename );
        }
    }
    return ret;
}

/***********************************************************************
 *           GetFontResourceInfoW    (GDI32.@)
 */
BOOL WINAPI GetFontResourceInfoW( LPCWSTR str, LPDWORD size, PVOID buffer, DWORD type )
{
    FIXME("%s %p(%d) %p %d\n", debugstr_w(str), size, size ? *size : 0, buffer, type);
    return FALSE;
}

/***********************************************************************
 *           GetTextCharset    (GDI32.@)
 */
UINT WINAPI GetTextCharset(HDC hdc)
{
    /* MSDN docs say this is equivalent */
    return GetTextCharsetInfo(hdc, NULL, 0);
}

/***********************************************************************
 *           GdiGetCharDimensions    (GDI32.@)
 *
 * Gets the average width of the characters in the English alphabet.
 *
 * PARAMS
 *  hdc    [I] Handle to the device context to measure on.
 *  lptm   [O] Pointer to memory to store the text metrics into.
 *  height [O] On exit, the maximum height of characters in the English alphabet.
 *
 * RETURNS
 *  The average width of characters in the English alphabet.
 *
 * NOTES
 *  This function is used by the dialog manager to get the size of a dialog
 *  unit. It should also be used by other pieces of code that need to know
 *  the size of a dialog unit in logical units without having access to the
 *  window handle of the dialog.
 *  Windows caches the font metrics from this function, but we don't and
 *  there doesn't appear to be an immediate advantage to do so.
 *
 * SEE ALSO
 *  GetTextExtentPointW, GetTextMetricsW, MapDialogRect.
 */
LONG WINAPI GdiGetCharDimensions(HDC hdc, LPTEXTMETRICW lptm, LONG *height)
{
    SIZE sz;
    static const WCHAR alphabet[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0};

    if(lptm && !GetTextMetricsW(hdc, lptm)) return 0;

    if(!GetTextExtentPointW(hdc, alphabet, 52, &sz)) return 0;

    if (height) *height = sz.cy;
    return (sz.cx / 26 + 1) / 2;
}

BOOL WINAPI EnableEUDC(BOOL fEnableEUDC)
{
    FIXME("(%d): stub\n", fEnableEUDC);
    return FALSE;
}

/***********************************************************************
 *           GetCharWidthI    (GDI32.@)
 *
 * Retrieve widths of characters.
 *
 * PARAMS
 *  hdc    [I] Handle to a device context.
 *  first  [I] First glyph in range to query.
 *  count  [I] Number of glyph indices to query.
 *  glyphs [I] Array of glyphs to query.
 *  buffer [O] Buffer to receive character widths.
 *
 * NOTES
 *  Only works with TrueType fonts.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI GetCharWidthI(HDC hdc, UINT first, UINT count, LPWORD glyphs, LPINT buffer)
{
    ABC *abc;
    unsigned int i;

    TRACE("(%p, %d, %d, %p, %p)\n", hdc, first, count, glyphs, buffer);

    if (!(abc = HeapAlloc(GetProcessHeap(), 0, count * sizeof(ABC))))
        return FALSE;

    if (!GetCharABCWidthsI(hdc, first, count, glyphs, abc))
    {
        HeapFree(GetProcessHeap(), 0, abc);
        return FALSE;
    }

    for (i = 0; i < count; i++)
        buffer[i] = abc[i].abcA + abc[i].abcB + abc[i].abcC;

    HeapFree(GetProcessHeap(), 0, abc);
    return TRUE;
}

/***********************************************************************
 *           GetFontUnicodeRanges    (GDI32.@)
 *
 *  Retrieve a list of supported Unicode characters in a font.
 *
 *  PARAMS
 *   hdc  [I] Handle to a device context.
 *   lpgs [O] GLYPHSET structure specifying supported character ranges.
 *
 *  RETURNS
 *   Success: Number of bytes written to the buffer pointed to by lpgs.
 *   Failure: 0
 *
 */
DWORD WINAPI GetFontUnicodeRanges(HDC hdc, LPGLYPHSET lpgs)
{
    DWORD ret;
    PHYSDEV dev;
    DC *dc = get_dc_ptr(hdc);

    TRACE("(%p, %p)\n", hdc, lpgs);

    if (!dc) return 0;

    dev = GET_DC_PHYSDEV( dc, pGetFontUnicodeRanges );
    ret = dev->funcs->pGetFontUnicodeRanges( dev, lpgs );
    release_dc_ptr(dc);
    return ret;
}


/*************************************************************
 *           FontIsLinked    (GDI32.@)
 */
BOOL WINAPI FontIsLinked(HDC hdc)
{
    DC *dc = get_dc_ptr(hdc);
    PHYSDEV dev;
    BOOL ret;

    if (!dc) return FALSE;
    dev = GET_DC_PHYSDEV( dc, pFontIsLinked );
    ret = dev->funcs->pFontIsLinked( dev );
    release_dc_ptr(dc);
    TRACE("returning %d\n", ret);
    return ret;
}

/*************************************************************
 *           GetFontRealizationInfo    (GDI32.@)
 */
BOOL WINAPI GetFontRealizationInfo(HDC hdc, struct font_realization_info *info)
{
    BOOL is_v0 = info->size == FIELD_OFFSET(struct font_realization_info, unk);
    PHYSDEV dev;
    BOOL ret;
    DC *dc;

    if (info->size != sizeof(*info) && !is_v0)
        return FALSE;

    dc = get_dc_ptr(hdc);
    if (!dc) return FALSE;
    dev = GET_DC_PHYSDEV( dc, pGetFontRealizationInfo );
    ret = dev->funcs->pGetFontRealizationInfo( dev, info );
    release_dc_ptr(dc);
    return ret;
}

struct realization_info
{
    DWORD flags;       /* 1 for bitmap fonts, 3 for scalable fonts */
    DWORD cache_num;   /* keeps incrementing - num of fonts that have been created allowing for caching?? */
    DWORD instance_id; /* identifies a realized font instance */
};

/*************************************************************
 *           GdiRealizationInfo    (GDI32.@)
 *
 * Returns a structure that contains some font information.
 */
BOOL WINAPI GdiRealizationInfo(HDC hdc, struct realization_info *info)
{
    struct font_realization_info ri;
    BOOL ret;

    ri.size = sizeof(ri);
    ret = GetFontRealizationInfo( hdc, &ri );
    if (ret)
    {
        info->flags = ri.flags;
        info->cache_num = ri.cache_num;
        info->instance_id = ri.instance_id;
    }

    return ret;
}

/*************************************************************
 *           GetCharWidthInfo    (GDI32.@)
 *
 */
BOOL WINAPI GetCharWidthInfo(HDC hdc, struct char_width_info *info)
{
    PHYSDEV dev;
    BOOL ret;
    DC *dc;

    dc = get_dc_ptr(hdc);
    if (!dc) return FALSE;
    dev = GET_DC_PHYSDEV( dc, pGetCharWidthInfo );
    ret = dev->funcs->pGetCharWidthInfo( dev, info );

    if (ret)
    {
        info->lsb = width_to_LP( dc, info->lsb );
        info->rsb = width_to_LP( dc, info->rsb );
    }
    release_dc_ptr(dc);
    return ret;
}
