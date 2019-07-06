/*
 *	PostScript brush handling
 *
 * Copyright 1998  Huw D M Davies
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

#include "psdrv.h"
#include "wine/debug.h"
#include "winbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           SelectBrush   (WINEPS.@)
 */
HBRUSH CDECL PSDRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    LOGBRUSH logbrush;

    if (!GetObjectA( hbrush, sizeof(logbrush), &logbrush )) return 0;

    TRACE("hbrush = %p\n", hbrush);

    if (hbrush == GetStockObject( DC_BRUSH ))
        logbrush.lbColor = GetDCBrushColor( dev->hdc );

    switch(logbrush.lbStyle) {

    case BS_SOLID:
        PSDRV_CreateColor(dev, &physDev->brush.color, logbrush.lbColor);
	break;

    case BS_NULL:
        break;

    case BS_HATCHED:
        PSDRV_CreateColor(dev, &physDev->brush.color, logbrush.lbColor);
        break;

    case BS_PATTERN:
    case BS_DIBPATTERN:
        physDev->brush.pattern = *pattern;
	break;

    default:
        FIXME("Unrecognized brush style %d\n", logbrush.lbStyle);
	break;
    }

    physDev->brush.set = FALSE;
    return hbrush;
}


/***********************************************************************
 *           SetDCBrushColor (WINEPS.@)
 */
COLORREF CDECL PSDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );

    if (GetCurrentObject( dev->hdc, OBJ_BRUSH ) == GetStockObject( DC_BRUSH ))
    {
        PSDRV_CreateColor( dev, &physDev->brush.color, color );
        physDev->brush.set = FALSE;
    }
    return color;
}


/**********************************************************************
 *
 *	PSDRV_SetBrush
 *
 */
static BOOL PSDRV_SetBrush( PHYSDEV dev )
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    LOGBRUSH logbrush;
    BOOL ret = TRUE;

    if (!GetObjectA( GetCurrentObject(dev->hdc,OBJ_BRUSH), sizeof(logbrush), &logbrush ))
    {
        ERR("Can't get BRUSHOBJ\n");
	return FALSE;
    }

    switch (logbrush.lbStyle) {
    case BS_SOLID:
    case BS_HATCHED:
        PSDRV_WriteSetColor(dev, &physDev->brush.color);
	break;

    case BS_NULL:
        break;

    default:
        ret = FALSE;
        break;

    }
    physDev->brush.set = TRUE;
    return ret;
}


/**********************************************************************
 *
 *	PSDRV_Fill
 *
 */
static BOOL PSDRV_Fill(PHYSDEV dev, BOOL EO)
{
    if(!EO)
        return PSDRV_WriteFill(dev);
    else
        return PSDRV_WriteEOFill(dev);
}


/**********************************************************************
 *
 *	PSDRV_Clip
 *
 */
static BOOL PSDRV_Clip(PHYSDEV dev, BOOL EO)
{
    if(!EO)
        return PSDRV_WriteClip(dev);
    else
        return PSDRV_WriteEOClip(dev);
}

/**********************************************************************
 *
 *	PSDRV_Brush
 *
 */
BOOL PSDRV_Brush(PHYSDEV dev, BOOL EO)
{
    PSDRV_PDEVICE *physDev = get_psdrv_dev( dev );
    LOGBRUSH logbrush;
    BOOL ret = TRUE;

    if(physDev->pathdepth)
        return FALSE;

    if (!GetObjectA( GetCurrentObject(dev->hdc,OBJ_BRUSH), sizeof(logbrush), &logbrush ))
    {
        ERR("Can't get BRUSHOBJ\n");
	return FALSE;
    }

    switch (logbrush.lbStyle) {
    case BS_SOLID:
	PSDRV_WriteGSave(dev);
        PSDRV_SetBrush(dev);
        PSDRV_Fill(dev, EO);
	PSDRV_WriteGRestore(dev);
	break;

    case BS_HATCHED:
        PSDRV_WriteGSave(dev);
        PSDRV_SetBrush(dev);

	switch(logbrush.lbHatch) {
	case HS_VERTICAL:
	case HS_CROSS:
            PSDRV_WriteGSave(dev);
	    PSDRV_Clip(dev, EO);
	    PSDRV_WriteHatch(dev);
	    PSDRV_WriteStroke(dev);
	    PSDRV_WriteGRestore(dev);
	    if(logbrush.lbHatch == HS_VERTICAL)
	        break;
	    /* else fallthrough for HS_CROSS */

	case HS_HORIZONTAL:
            PSDRV_WriteGSave(dev);
	    PSDRV_Clip(dev, EO);
	    PSDRV_WriteRotate(dev, 90.0);
	    PSDRV_WriteHatch(dev);
	    PSDRV_WriteStroke(dev);
	    PSDRV_WriteGRestore(dev);
	    break;

	case HS_FDIAGONAL:
	case HS_DIAGCROSS:
	    PSDRV_WriteGSave(dev);
	    PSDRV_Clip(dev, EO);
	    PSDRV_WriteRotate(dev, -45.0);
	    PSDRV_WriteHatch(dev);
	    PSDRV_WriteStroke(dev);
	    PSDRV_WriteGRestore(dev);
	    if(logbrush.lbHatch == HS_FDIAGONAL)
	        break;
	    /* else fallthrough for HS_DIAGCROSS */

	case HS_BDIAGONAL:
	    PSDRV_WriteGSave(dev);
	    PSDRV_Clip(dev, EO);
	    PSDRV_WriteRotate(dev, 45.0);
	    PSDRV_WriteHatch(dev);
	    PSDRV_WriteStroke(dev);
	    PSDRV_WriteGRestore(dev);
	    break;

	default:
	    ERR("Unknown hatch style\n");
	    ret = FALSE;
            break;
	}
        PSDRV_WriteGRestore(dev);
	break;

    case BS_NULL:
	break;

    case BS_PATTERN:
    case BS_DIBPATTERN:
        if(physDev->pi->ppd->LanguageLevel > 1) {
            PSDRV_WriteGSave(dev);
            ret = PSDRV_WriteDIBPatternDict(dev, physDev->brush.pattern.info,
                                            physDev->brush.pattern.bits.ptr, physDev->brush.pattern.usage );
            PSDRV_Fill(dev, EO);
            PSDRV_WriteGRestore(dev);
        } else {
            FIXME("Trying to set a pattern brush on a level 1 printer\n");
            ret = FALSE;
	}
	break;

    default:
        ret = FALSE;
	break;
    }
    return ret;
}
