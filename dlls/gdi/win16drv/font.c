/*
 * Windows driver font functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
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
#include "winnls.h"
#include "wine/winbase16.h"
#include "win16drv/win16drv.h"
#include "font.h"
#include "gdi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win16drv);


/***********************************************************************
 *           WIN16DRV_GetTextExtentPoint
 */
BOOL WIN16DRV_GetTextExtentPoint( PHYSDEV dev, LPCWSTR wstr, INT count,
				  LPSIZE size )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    DWORD dwRet, len;
    char *str;

    TRACE("%04x %s %d %p\n", physDev->hdc, debugstr_wn(wstr, count), count, size);


    len = WideCharToMultiByte( CP_ACP, 0, wstr, count, NULL, 0, NULL, NULL );
    str = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, wstr, count, str, len, NULL, NULL );

    dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0,
                              NULL, str, -len,  physDev->FontInfo,
			      win16drv_SegPtr_DrawMode,
			      win16drv_SegPtr_TextXForm, NULL, NULL, 0);
    size->cx = XDSTOLS(dc,LOWORD(dwRet));
    size->cy = YDSTOLS(dc,HIWORD(dwRet));
    TRACE("cx=%ld, cy=%ld\n", size->cx, size->cy );
    HeapFree( GetProcessHeap(), 0, str );
    return TRUE;
}


/***********************************************************************
 *           WIN16DRV_GetTextMetrics
 */
BOOL WIN16DRV_GetTextMetrics( PHYSDEV dev, TEXTMETRICW *metrics )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;

    TRACE("%04x \n", physDev->hdc);

    FONT_TextMetric16ToW( &physDev->tm, metrics );

    TRACE(
	  "H %ld, A %ld, D %ld, Int %ld, Ext %ld, AW %ld, MW %ld, W %ld\n",
           metrics->tmHeight,
           metrics->tmAscent,
           metrics->tmDescent,
           metrics->tmInternalLeading,
           metrics->tmExternalLeading,
           metrics->tmAveCharWidth,
           metrics->tmMaxCharWidth,
           metrics->tmWeight);

    return TRUE;
}

/***********************************************************************
 *           WIN16DRV_SelectFont
 */
HFONT WIN16DRV_SelectFont( PHYSDEV dev, HFONT hfont)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    DC *dc = physDev->dc;
    int	nSize;

    if (!GetObject16( hfont, sizeof(physDev->lf), &physDev->lf ))
        return GDI_ERROR;

    TRACE("WIN16DRV_FONT_SelectObject %s h=%d\n",
          debugstr_a(physDev->lf.lfFaceName), physDev->lf.lfHeight);

    if( physDev->FontInfo )
    {
        TRACE("UnRealizing FontInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_FONT,
				      physDev->FontInfo,
				      physDev->FontInfo, 0);
    }

    nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_FONT,
                                  &physDev->lf, 0, 0);

    if( physDev->FontInfo &&
	HeapSize( GetProcessHeap(), 0, physDev->FontInfo ) < nSize )
    {
        HeapFree( GetProcessHeap(), 0, physDev->FontInfo );
	physDev->FontInfo = NULL;
    }

    if( !physDev->FontInfo )
        physDev->FontInfo = HeapAlloc( GetProcessHeap(), 0, nSize );


    nSize = PRTDRV_RealizeObject(physDev->segptrPDEVICE, DRVOBJ_FONT,
                                 &physDev->lf,
                                 physDev->FontInfo,
                                 win16drv_SegPtr_TextXForm );

#define fi physDev->FontInfo
    physDev->tm.tmHeight           = YDSTOLS(dc, fi->dfPixHeight);
    physDev->tm.tmAscent           = YDSTOLS(dc, fi->dfAscent);
    physDev->tm.tmDescent          = physDev->tm.tmHeight -
					    physDev->tm.tmAscent;
    physDev->tm.tmInternalLeading  = YDSTOLS(dc, fi->dfInternalLeading);
    physDev->tm.tmExternalLeading  = YDSTOLS(dc, fi->dfExternalLeading);
    physDev->tm.tmAveCharWidth     = XDSTOLS(dc, fi->dfAvgWidth);
    physDev->tm.tmMaxCharWidth     = XDSTOLS(dc, fi->dfMaxWidth);
    physDev->tm.tmWeight           = fi->dfWeight;
    physDev->tm.tmOverhang         = 0; /*FIXME*/
    physDev->tm.tmDigitizedAspectX = fi->dfHorizRes;
    physDev->tm.tmDigitizedAspectY = fi->dfVertRes;
    physDev->tm.tmFirstChar        = fi->dfFirstChar;
    physDev->tm.tmLastChar         = fi->dfLastChar;
    physDev->tm.tmDefaultChar      = fi->dfDefaultChar;
    physDev->tm.tmBreakChar        = fi->dfBreakChar;
    physDev->tm.tmItalic           = fi->dfItalic;
    physDev->tm.tmUnderlined       = fi->dfUnderline;
    physDev->tm.tmStruckOut        = fi->dfStrikeOut;
    physDev->tm.tmPitchAndFamily   = fi->dfPitchAndFamily;
    physDev->tm.tmCharSet          = fi->dfCharSet;
#undef fi

    TRACE("H %d, A %d, D %d, Int %d, Ext %d, AW %d, MW %d, W %d\n",
           physDev->tm.tmHeight,
           physDev->tm.tmAscent,
           physDev->tm.tmDescent,
           physDev->tm.tmInternalLeading,
           physDev->tm.tmExternalLeading,
           physDev->tm.tmAveCharWidth,
           physDev->tm.tmMaxCharWidth,
           physDev->tm.tmWeight);

    return TRUE; /* We'll use a device font */
}

/***********************************************************************
 *           WIN16DRV_GetCharWidth
 */
BOOL WIN16DRV_GetCharWidth( PHYSDEV dev, UINT firstChar, UINT lastChar,
			    LPINT buffer )
{
    int i;
    WORD wRet;
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;

    TRACE("%d - %d into %p\n", firstChar, lastChar, buffer );

    wRet = PRTDRV_GetCharWidth( physDev->segptrPDEVICE, buffer, firstChar,
				lastChar, physDev->FontInfo,
				win16drv_SegPtr_DrawMode,
				win16drv_SegPtr_TextXForm );
    if( TRACE_ON(win16drv) ){
        for(i = 0; i <= lastChar - firstChar; i++)
	    TRACE("Char %x: width %d\n", i + firstChar,
			                 buffer[i]);
    }

    return wRet;
}

/***********************************************************************
 *
 *           WIN16DRV_EnumDeviceFonts
 */

BOOL WIN16DRV_EnumDeviceFonts( PHYSDEV dev, LPLOGFONTW plf,
                               DEVICEFONTENUMPROC proc, LPARAM lp )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dev;
    WORD wRet;
    WEPFC wepfc;
    char *FaceNameA = NULL;
   /* EnumDFontCallback is GDI.158 */
    FARPROC16 pfnCallback = GetProcAddress16( GetModuleHandle16("GDI"), (LPCSTR)158 );

    wepfc.proc = proc;
    wepfc.lp = lp;

    if(plf->lfFaceName[0]) {
        INT len;
        len = WideCharToMultiByte(CP_ACP, 0, plf->lfFaceName, -1, NULL, 0,
				  NULL, NULL);
	FaceNameA = HeapAlloc(GetProcessHeap(), 0, len);
	WideCharToMultiByte(CP_ACP, 0, plf->lfFaceName, -1, FaceNameA, len,
			    NULL, NULL);
    }
    wRet = PRTDRV_EnumDFonts(physDev->segptrPDEVICE, FaceNameA, pfnCallback,
			     &wepfc );
    if(FaceNameA) HeapFree(GetProcessHeap(), 0, FaceNameA);
    return wRet;
}

/***********************************************************************
 * EnumCallback (GDI.158)
 *
 * This is the callback function used when EnumDFonts is called.
 * (The printer drivers uses it to pass info on available fonts).
 *
 * lpvClientData is the pointer passed to EnumDFonts, which points to a WEPFC
 * structure (WEPFC = WINE_ENUM_PRINTER_FONT_CALLBACK).
 *
 */
WORD WINAPI EnumCallback16(LPENUMLOGFONT16 lpLogFont,
                           LPNEWTEXTMETRIC16 lpTextMetrics,
                           WORD wFontType, LONG lpClientData)
{
    ENUMLOGFONTEXW lfW;
    ENUMLOGFONTEX16 lf16;

    NEWTEXTMETRICEXW tmW;
    NEWTEXTMETRICEX16 tm16;

    TRACE("In EnumCallback16 plf=%p\n", lpLogFont);

    /* we have a ENUMLOGFONT16 which is a subset of ENUMLOGFONTEX16,
       so we copy it into one of these and then convert to ENUMLOGFONTEXW */

    memset(&lf16, 0, sizeof(lf16));
    memcpy(&lf16, lpLogFont, sizeof(*lpLogFont));
    FONT_EnumLogFontEx16ToW(&lf16, &lfW);

    /* and a similar idea for NEWTEXTMETRIC16 */
    memset(&tm16, 0, sizeof(tm16));
    memcpy(&tm16, lpTextMetrics, sizeof(*lpTextMetrics));
    FONT_NewTextMetricEx16ToW(&tm16, &tmW);

    return (*(((WEPFC *)lpClientData)->proc))( &lfW, &tmW, wFontType,
					       ((WEPFC *)lpClientData)->lp );
}

