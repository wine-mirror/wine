/*
 * Windows driver font functions
 *
 * Copyright 1996 John Harvey
 */

#include "windows.h"
#include "win16drv.h"
#include "gdi.h"


/***********************************************************************
 *           WIN16DRV_GetTextExtentPoint
 */
BOOL32 WIN16DRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                    LPSIZE32 size )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;
    
    printf("LPGDI_GetTextExtPoint: %04x %s %d %p\n", dc->hSelf, str, count, size);

	/* TTD support PS fonts */
	/* Assume fixed font */
    size->cx = count * physDev->tm.tmAveCharWidth;
    size->cy = physDev->tm.tmHeight;
    printf("LPGDI_GetTextExtPoint: cx=%d, cy=%d\n", size->cx,size->cy);
    return TRUE;
}


/***********************************************************************
 *           WIN16DRV_GetTextMetrics
 */
BOOL32 WIN16DRV_GetTextMetrics( DC *dc, TEXTMETRIC32A *metrics )
{
    WIN16DRV_PDEVICE *physDev = (WIN16DRV_PDEVICE *)dc->physDev;

    printf("LPGDI_GetTextMetrics: %04x \n", dc->hSelf);

    metrics->tmHeight           = physDev->tm.tmHeight;
    metrics->tmAscent           = physDev->tm.tmAscent;
    metrics->tmDescent          = physDev->tm.tmDescent;
    metrics->tmInternalLeading  = physDev->tm.tmInternalLeading;
    metrics->tmExternalLeading  = physDev->tm.tmExternalLeading;
    metrics->tmAveCharWidth     = physDev->tm.tmAveCharWidth;
    metrics->tmMaxCharWidth     = physDev->tm.tmMaxCharWidth;
    metrics->tmWeight           = physDev->tm.tmWeight;
    metrics->tmOverhang         = physDev->tm.tmOverhang;
    metrics->tmDigitizedAspectX = physDev->tm.tmDigitizedAspectX;
    metrics->tmDigitizedAspectY = physDev->tm.tmDigitizedAspectY;
    metrics->tmFirstChar        = physDev->tm.tmFirstChar;
    metrics->tmLastChar         = physDev->tm.tmLastChar;
    metrics->tmDefaultChar      = physDev->tm.tmDefaultChar;
    metrics->tmBreakChar        = physDev->tm.tmBreakChar;
    metrics->tmItalic           = physDev->tm.tmItalic;
    metrics->tmUnderlined       = physDev->tm.tmUnderlined;
    metrics->tmStruckOut        = physDev->tm.tmStruckOut;
    metrics->tmPitchAndFamily   = physDev->tm.tmPitchAndFamily;
    metrics->tmCharSet          = physDev->tm.tmCharSet;

    printf("H %d, A %d, D %d, Int %d, Ext %d, AW %d, MW %d, W %d\n",
           physDev->tm.tmHeight,
           physDev->tm.tmAscent,
           physDev->tm.tmDescent,
           physDev->tm.tmInternalLeading,
           physDev->tm.tmExternalLeading,
           physDev->tm.tmAveCharWidth,
           physDev->tm.tmMaxCharWidth,
           physDev->tm.tmWeight);

    return TRUE;
}
