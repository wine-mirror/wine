/*
 * Windows driver font functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 */

#include <string.h>
#include "winnls.h"
#include "wine/winbase16.h"
#include "win16drv.h"
#include "font.h"
#include "heap.h"
#include "gdi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win16drv);


/***********************************************************************
 *           WIN16DRV_GetTextExtentPoint
 */
BOOL WIN16DRV_GetTextExtentPoint( DC *dc, LPCWSTR wstr, INT count,
				  LPSIZE size )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    DWORD dwRet, len;
    char *str;

    TRACE("%04x %s %d %p\n",
	  dc->hSelf, debugstr_wn(wstr, count), count, size);


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
BOOL WIN16DRV_GetTextMetrics( DC *dc, TEXTMETRICW *metrics )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;

    TRACE("%04x \n", dc->hSelf);

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

HFONT WIN16DRV_FONT_SelectObject( DC * dc, HFONT hfont, FONTOBJ * font)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    HPEN prevHandle = dc->hFont;
    int	nSize;

    dc->hFont = hfont;

    TRACE("WIN16DRV_FONT_SelectObject %s h=%ld\n",
	  debugstr_w(font->logfont.lfFaceName), font->logfont.lfHeight);


    if( physDev->FontInfo )
    {
        TRACE("UnRealizing FontInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_FONT,
				      physDev->FontInfo,
				      physDev->FontInfo, 0);
    }

    FONT_LogFontWTo16(&font->logfont, &physDev->lf);
    nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_FONT,
                                  &physDev->lf, 0, 0); 

    if( physDev->FontInfo && 
	HeapSize( GetProcessHeap(), HEAP_WINE_SEGPTR, physDev->FontInfo ) < nSize )
    {
        SEGPTR_FREE( physDev->FontInfo );
	physDev->FontInfo = NULL;
    }
    
    if( !physDev->FontInfo )
        physDev->FontInfo = SEGPTR_ALLOC( nSize );


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

    return prevHandle;
}

/***********************************************************************
 *           GetCharWidth32A    (GDI32.@)
 */
BOOL WIN16DRV_GetCharWidth( DC *dc, UINT firstChar, UINT lastChar,
			    LPINT buffer )
{
    int i;
    WORD wRet;

    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    
    TRACE("%d - %d into %p\n",
		      firstChar, lastChar, buffer );

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

BOOL	WIN16DRV_EnumDeviceFonts( HDC hdc, LPLOGFONTW plf, 
				  DEVICEFONTENUMPROC proc, LPARAM lp )
{
    WIN16DRV_PDEVICE *physDev;
    WORD wRet;
    WEPFC wepfc;
    DC *dc;
    char *FaceNameA = NULL;
   /* EnumDFontCallback is GDI.158 */
    FARPROC16 pfnCallback = GetProcAddress16( GetModuleHandle16("GDI"), (LPCSTR)158 );

    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    /* FIXME!! */
    GDI_ReleaseObj( hdc );

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

