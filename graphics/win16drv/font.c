/*
 * Windows driver font functions
 *
 * Copyright 1996 John Harvey
 *           1998 Huw Davies
 */

#include <stdio.h>
#include "windows.h"
#include "win16drv.h"
#include "gdi.h"
#include "module.h"
#include "font.h"
#include "heap.h"
#include "debug.h"


/***********************************************************************
 *           WIN16DRV_GetTextExtentPoint
 */
BOOL32 WIN16DRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                    LPSIZE32 size )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    DWORD dwRet;
    
    dprintf_info(win16drv, "WIN16DRV_GetTextExtPoint: %04x %s %d %p\n",
		                dc->hSelf, str, count, size);

    dwRet = PRTDRV_ExtTextOut(physDev->segptrPDEVICE, 0, 0, 
			      NULL, str, 
			      -count,  physDev->FontInfo, 
			      win16drv_SegPtr_DrawMode, 
			      win16drv_SegPtr_TextXForm, NULL, NULL, 0);
    size->cx = XDSTOLS(dc,LOWORD(dwRet));
    size->cy = YDSTOLS(dc,HIWORD(dwRet));
    dprintf_info(win16drv, "WIN16DRV_GetTextExtPoint: cx=0x%x, cy=0x%x\n",
		size->cx, size->cy );
    return TRUE;
}


/***********************************************************************
 *           WIN16DRV_GetTextMetrics
 */
BOOL32 WIN16DRV_GetTextMetrics( DC *dc, TEXTMETRIC32A *metrics )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;

    dprintf_info(win16drv, "WIN16DRV_GetTextMetrics: %04x \n", dc->hSelf);

    FONT_TextMetric16to32A( &physDev->tm, metrics );

    dprintf_info(win16drv,
	   "H %d, A %d, D %d, Int %d, Ext %d, AW %d, MW %d, W %d\n",
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

HFONT32 WIN16DRV_FONT_SelectObject( DC * dc, HFONT32 hfont, FONTOBJ * font)
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    HPEN32 prevHandle = dc->w.hFont;
    int	nSize;

    dc->w.hFont = hfont;

    dprintf_info(win16drv, "WIN16DRV_FONT_SelectObject '%s' h=%d\n",
		     font->logfont.lfFaceName, font->logfont.lfHeight);


    if( physDev->FontInfo )
    {
        dprintf_info(win16drv, "UnRealizing FontInfo\n");
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

    dprintf_info(win16drv,
           "H %d, A %d, D %d, Int %d, Ext %d, AW %d, MW %d, W %d\n",
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
BOOL32 WIN16DRV_GetCharWidth( DC *dc, UINT32 firstChar, UINT32 lastChar,
			    LPINT32 buffer )
{
    int i;
    WORD wRet;

    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    
    dprintf_info(win16drv, "WIN16DRV_GetCharWidth: %d - %d into %p\n",
		      firstChar, lastChar, buffer );

    wRet = PRTDRV_GetCharWidth( physDev->segptrPDEVICE, buffer, firstChar, 
				lastChar, physDev->FontInfo, 
				win16drv_SegPtr_DrawMode, 
				win16drv_SegPtr_TextXForm );
    if( debugging_info(win16drv) ){
        for(i = 0; i <= lastChar - firstChar; i++)
	    dprintf_info(win16drv, "Char %x: width %d\n", i + firstChar,
			                 buffer[i]);
    }

    return wRet;
}

/***********************************************************************
 *
 *           WIN16DRV_EnumDeviceFonts
 */

BOOL32	WIN16DRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				        DEVICEFONTENUMPROC proc, LPARAM lp )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    WORD wRet;
    WEPFC wepfc = {proc, lp};

    /* EnumDFontCallback is GDI.158 */
    FARPROC16 pfnCallback = MODULE_GetEntryPoint( GetModuleHandle16("GDI"),
						              158 );

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


WORD WINAPI WineEnumDFontCallback(LPLOGFONT16 lpLogFont,
                                  LPTEXTMETRIC16 lpTextMetrics,
                                  WORD wFontType, LONG lpClientData) 
{
    dprintf_info(win16drv, "In WineEnumDFontCallback plf=%p\n", lpLogFont);
    return (*(((WEPFC *)lpClientData)->proc))( lpLogFont, lpTextMetrics, 
				     wFontType, ((WEPFC *)lpClientData)->lp );
}

