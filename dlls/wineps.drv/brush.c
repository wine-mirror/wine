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
HBRUSH CDECL PSDRV_SelectBrush( PSDRV_PDEVICE *physDev, HBRUSH hbrush )
{
    LOGBRUSH logbrush;

    if (!GetObjectA( hbrush, sizeof(logbrush), &logbrush )) return 0;

    TRACE("hbrush = %p\n", hbrush);

    switch(logbrush.lbStyle) {

    case BS_SOLID:
        PSDRV_CreateColor(physDev, &physDev->brush.color, logbrush.lbColor);
	break;

    case BS_NULL:
        break;

    case BS_HATCHED:
        PSDRV_CreateColor(physDev, &physDev->brush.color, logbrush.lbColor);
        break;

    case BS_PATTERN:
    case BS_DIBPATTERN:
	break;

    default:
        FIXME("Unrecognized brush style %d\n", logbrush.lbStyle);
	break;
    }

    physDev->brush.set = FALSE;
    return hbrush;
}


/**********************************************************************
 *
 *	PSDRV_SetBrush
 *
 */
static BOOL PSDRV_SetBrush(PSDRV_PDEVICE *physDev)
{
    LOGBRUSH logbrush;
    BOOL ret = TRUE;

    if (!GetObjectA( GetCurrentObject(physDev->hdc,OBJ_BRUSH), sizeof(logbrush), &logbrush ))
    {
        ERR("Can't get BRUSHOBJ\n");
	return FALSE;
    }

    switch (logbrush.lbStyle) {
    case BS_SOLID:
    case BS_HATCHED:
        PSDRV_WriteSetColor(physDev, &physDev->brush.color);
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
static BOOL PSDRV_Fill(PSDRV_PDEVICE *physDev, BOOL EO)
{
    if(!EO)
        return PSDRV_WriteFill(physDev);
    else
        return PSDRV_WriteEOFill(physDev);
}


/**********************************************************************
 *
 *	PSDRV_Clip
 *
 */
static BOOL PSDRV_Clip(PSDRV_PDEVICE *physDev, BOOL EO)
{
    if(!EO)
        return PSDRV_WriteClip(physDev);
    else
        return PSDRV_WriteEOClip(physDev);
}

/**********************************************************************
 *
 *	PSDRV_Brush
 *
 */
BOOL PSDRV_Brush(PSDRV_PDEVICE *physDev, BOOL EO)
{
    LOGBRUSH logbrush;
    BOOL ret = TRUE;

    if(physDev->pathdepth)
        return FALSE;

    if (!GetObjectA( GetCurrentObject(physDev->hdc,OBJ_BRUSH), sizeof(logbrush), &logbrush ))
    {
        ERR("Can't get BRUSHOBJ\n");
	return FALSE;
    }

    switch (logbrush.lbStyle) {
    case BS_SOLID:
	PSDRV_WriteGSave(physDev);
        PSDRV_SetBrush(physDev);
        PSDRV_Fill(physDev, EO);
	PSDRV_WriteGRestore(physDev);
	break;

    case BS_HATCHED:
        PSDRV_WriteGSave(physDev);
        PSDRV_SetBrush(physDev);

	switch(logbrush.lbHatch) {
	case HS_VERTICAL:
	case HS_CROSS:
            PSDRV_WriteGSave(physDev);
	    PSDRV_Clip(physDev, EO);
	    PSDRV_WriteHatch(physDev);
	    PSDRV_WriteStroke(physDev);
	    PSDRV_WriteGRestore(physDev);
	    if(logbrush.lbHatch == HS_VERTICAL)
	        break;
	    /* else fallthrough for HS_CROSS */

	case HS_HORIZONTAL:
            PSDRV_WriteGSave(physDev);
	    PSDRV_Clip(physDev, EO);
	    PSDRV_WriteRotate(physDev, 90.0);
	    PSDRV_WriteHatch(physDev);
	    PSDRV_WriteStroke(physDev);
	    PSDRV_WriteGRestore(physDev);
	    break;

	case HS_FDIAGONAL:
	case HS_DIAGCROSS:
	    PSDRV_WriteGSave(physDev);
	    PSDRV_Clip(physDev, EO);
	    PSDRV_WriteRotate(physDev, -45.0);
	    PSDRV_WriteHatch(physDev);
	    PSDRV_WriteStroke(physDev);
	    PSDRV_WriteGRestore(physDev);
	    if(logbrush.lbHatch == HS_FDIAGONAL)
	        break;
	    /* else fallthrough for HS_DIAGCROSS */

	case HS_BDIAGONAL:
	    PSDRV_WriteGSave(physDev);
	    PSDRV_Clip(physDev, EO);
	    PSDRV_WriteRotate(physDev, 45.0);
	    PSDRV_WriteHatch(physDev);
	    PSDRV_WriteStroke(physDev);
	    PSDRV_WriteGRestore(physDev);
	    break;

	default:
	    ERR("Unknown hatch style\n");
	    ret = FALSE;
            break;
	}
        PSDRV_WriteGRestore(physDev);
	break;

    case BS_NULL:
	break;

    case BS_PATTERN:
        {
	    BITMAP bm;
	    BYTE *bits;
	    GetObjectA( (HBITMAP)logbrush.lbHatch, sizeof(BITMAP), &bm);
	    TRACE("BS_PATTERN %dx%d %d bpp\n", bm.bmWidth, bm.bmHeight,
		  bm.bmBitsPixel);
	    bits = HeapAlloc(PSDRV_Heap, 0, bm.bmWidthBytes * bm.bmHeight);
	    GetBitmapBits( (HBITMAP)logbrush.lbHatch, bm.bmWidthBytes * bm.bmHeight, bits);

	    if(physDev->pi->ppd->LanguageLevel > 1) {
	        PSDRV_WriteGSave(physDev);
	        PSDRV_WritePatternDict(physDev, &bm, bits);
		PSDRV_Fill(physDev, EO);
		PSDRV_WriteGRestore(physDev);
	    } else {
	        FIXME("Trying to set a pattern brush on a level 1 printer\n");
		ret = FALSE;
	    }
	    HeapFree(PSDRV_Heap, 0, bits);
	}
	break;

    case BS_DIBPATTERN:
        {
	    BITMAPINFO *bmi = GlobalLock( (HGLOBAL)logbrush.lbHatch );
	    UINT usage = logbrush.lbColor;
	    TRACE("size %dx%dx%d\n", bmi->bmiHeader.biWidth,
		  bmi->bmiHeader.biHeight, bmi->bmiHeader.biBitCount);
	    if(physDev->pi->ppd->LanguageLevel > 1) {
	        PSDRV_WriteGSave(physDev);
		ret = PSDRV_WriteDIBPatternDict(physDev, bmi, usage);
		PSDRV_Fill(physDev, EO);
		PSDRV_WriteGRestore(physDev);
	    } else {
	        FIXME("Trying to set a pattern brush on a level 1 printer\n");
		ret = FALSE;
	    }
	    GlobalUnlock( (HGLOBAL)logbrush.lbHatch );
	}
	break;

    default:
        ret = FALSE;
	break;
    }
    return ret;
}
