/*
 *	PostScript brush handling
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include "windows.h"
#include "psdrv.h"
#include "brush.h"
#include "debug.h"


/***********************************************************************
 *           PSDRV_BRUSH_SelectObject
 */
HBRUSH32 PSDRV_BRUSH_SelectObject( DC * dc, HBRUSH32 hbrush, BRUSHOBJ * brush )
{
    HBRUSH32 prevbrush = dc->w.hBrush;
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    TRACE(psdrv, "hbrush = %08x\n", hbrush);
    dc->w.hBrush = hbrush;

    switch(brush->logbrush.lbStyle) {

    case BS_SOLID:
        physDev->brush.style = BS_SOLID;
        PSDRV_CreateColor(physDev, &physDev->brush.color, 
			  brush->logbrush.lbColor);
	break;

    case BS_NULL:
        physDev->brush.style = BS_NULL;
        break;

    case BS_HATCHED:
    case BS_PATTERN:
        FIXME(psdrv, "Unsupported brush style %d\n", brush->logbrush.lbStyle);
	break;

    default:
        FIXME(psdrv, "Unrecognized brush style %d\n", brush->logbrush.lbStyle);
	break;
    }

    physDev->brush.set = FALSE;
    return prevbrush;
}


/**********************************************************************
 *
 *	PSDRV_SetBrush
 *
 */
BOOL32 PSDRV_SetBrush(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    switch (physDev->brush.style) {
    case BS_SOLID:
        PSDRV_WriteSetColor(dc, &physDev->brush.color);
	break;

    default:
        return FALSE;
        break;

    }
    physDev->brush.set = TRUE;
    return TRUE;
}
