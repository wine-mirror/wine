/*
 * Windows driver font functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 */

#include <string.h>
#include "wine/winbase16.h"
#include "win16drv.h"
#include "module.h"
#include "font.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win16drv)


/***********************************************************************
 *           WIN16DRV_GetTextExtentPoint
 */
BOOL WIN16DRV_GetTextExtentPoint( DC *dc, LPCWSTR wstr, INT count,
				  LPSIZE size )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    DWORD dwRet;
    char *str;

    TRACE("%04x %s %d %p\n",
	  dc->hSelf, debugstr_wn(wstr, count), count, size);

    str = HeapAlloc( GetProcessHeap(), 0, count+1 );
    lstrcpynWtoA( str, wstr, count+1 );
    dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0, 
			      NULL, str, 
			      -count,  physDev->FontInfo, 
			      win16drv_SegPtr_DrawMode, 
			      win16drv_SegPtr_TextXForm, NULL, NULL, 0);
    size->cx = XDSTOLS(dc,LOWORD(dwRet));
    size->cy = YDSTOLS(dc,HIWORD(dwRet));
    TRACE("cx=0x%x, cy=0x%x\n",
		size->cx, size->cy );
    HeapFree( GetProcessHeap(), 0, str );
    return TRUE;
}


/***********************************************************************
 *           WIN16DRV_GetTextMetrics
 */
BOOL WIN16DRV_GetTextMetrics( DC *dc, TEXTMETRICA *metrics )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;

    TRACE("%04x \n", dc->hSelf);

    FONT_TextMetric16to32A( &physDev->tm, metrics );

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
    HPEN prevHandle = dc->w.hFont;
    int	nSize;

    dc->w.hFont = hfont;

    TRACE("WIN16DRV_FONT_SelectObject '%s' h=%d\n",
		     font->logfont.lfFaceName, font->logfont.lfHeight);


    if( physDev->FontInfo )
    {
        TRACE("UnRealizing FontInfo\n");
        nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, -DRVOBJ_FONT,
				      physDev->FontInfo,
				      physDev->FontInfo, 0);
    }

    memcpy(&physDev->lf, &font->logfont, sizeof(LOGFONT16));
    nSize = PRTDRV_RealizeObject (physDev->segptrPDEVICE, DRVOBJ_FONT,
                                  &physDev->lf, 0, 0); 

    if( physDev->FontInfo && 
	HeapSize( SegptrHeap, 0, physDev->FontInfo ) < nSize )
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
 *           GetCharWidth32A    (GDI32.155)
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

BOOL	WIN16DRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				        DEVICEFONTENUMPROC proc, LPARAM lp )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    WORD wRet;
    WEPFC wepfc;
    /* EnumDFontCallback is GDI.158 */
    FARPROC16 pfnCallback = NE_GetEntryPoint( GetModuleHandle16("GDI"), 158 );

    wepfc.proc = (int (*)(LPENUMLOGFONT16,LPNEWTEXTMETRIC16,UINT16,LPARAM))proc;
    wepfc.lp = lp;

    wRet = PRTDRV_EnumDFonts(physDev->segptrPDEVICE, plf->lfFaceName[0] ?
			     plf->lfFaceName : NULL , pfnCallback , &wepfc );
    return wRet;
}

/*
 * EnumCallback (GDI.158)
 * 
 * This is the callback function used when EnumDFonts is called. 
 * (The printer drivers uses it to pass info on available fonts).
 *
 * lpvClientData is the pointer passed to EnumDFonts, which points to a WEPFC
 * structure (WEPFC = WINE_ENUM_PRINTER_FONT_CALLBACK).
 *
 */


WORD WINAPI WineEnumDFontCallback(LPENUMLOGFONT16 lpLogFont,
                                  LPNEWTEXTMETRIC16 lpTextMetrics,
                                  WORD wFontType, LONG lpClientData) 
{
    TRACE("In WineEnumDFontCallback plf=%p\n", lpLogFont);
    return (*(((WEPFC *)lpClientData)->proc))( lpLogFont, lpTextMetrics, 
				     wFontType, ((WEPFC *)lpClientData)->lp );
}

