/*
 *	PostScript brush handling
 *
 * Copyright 1998  Huw D M Davies
 *
 */

#include "psdrv.h"
#include "brush.h"
#include "debug.h"
#include "gdi.h"

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
        PSDRV_CreateColor(physDev, &physDev->brush.color, 
			  brush->logbrush.lbColor);
	break;

    case BS_NULL:
        break;

    case BS_HATCHED:
        PSDRV_CreateColor(physDev, &physDev->brush.color, 
			  brush->logbrush.lbColor);
        break;

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
static BOOL32 PSDRV_SetBrush(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    BRUSHOBJ *brush = (BRUSHOBJ *)GDI_GetObjPtr( dc->w.hBrush, BRUSH_MAGIC );

    if(!brush) {
        ERR(psdrv, "Can't get BRUSHOBJ\n");
	return FALSE;
    }
    
    switch (brush->logbrush.lbStyle) {
    case BS_SOLID:
    case BS_HATCHED:
        PSDRV_WriteSetColor(dc, &physDev->brush.color);
	break;

    case BS_NULL:
        break;

    default:
        return FALSE;
        break;

    }
    physDev->brush.set = TRUE;
    return TRUE;
}


/**********************************************************************
 *
 *	PSDRV_Fill
 *
 */
static BOOL32 PSDRV_Fill(DC *dc, BOOL32 EO)
{
    if(!EO)
        return PSDRV_WriteFill(dc);
    else
      return PSDRV_WriteEOFill(dc);
}


/**********************************************************************
 *
 *	PSDRV_Clip
 *
 */
static BOOL32 PSDRV_Clip(DC *dc, BOOL32 EO)
{
    if(!EO)
        return PSDRV_WriteClip(dc);
    else
        return PSDRV_WriteEOClip(dc);
}

/**********************************************************************
 *
 *	PSDRV_Brush
 *
 */
BOOL32 PSDRV_Brush(DC *dc, BOOL32 EO)
{
    BRUSHOBJ *brush = (BRUSHOBJ *)GDI_GetObjPtr( dc->w.hBrush, BRUSH_MAGIC );

    if(!brush) {
        ERR(psdrv, "Can't get BRUSHOBJ\n");
	return FALSE;
    }

    switch (brush->logbrush.lbStyle) {
    case BS_SOLID:
        PSDRV_SetBrush(dc);
	PSDRV_WriteGSave(dc);
        PSDRV_Fill(dc, EO);
	PSDRV_WriteGRestore(dc);
	return TRUE;
	break;

    case BS_HATCHED:
        PSDRV_SetBrush(dc);

	switch(brush->logbrush.lbHatch) {
	case HS_VERTICAL:
	case HS_CROSS:
	    PSDRV_WriteGSave(dc);
	    PSDRV_Clip(dc, EO);
	    PSDRV_WriteHatch(dc);
	    PSDRV_WriteStroke(dc);
	    PSDRV_WriteGRestore(dc);
	    if(brush->logbrush.lbHatch == HS_VERTICAL)
	        break;
	    /* else fallthrough for HS_CROSS */

	case HS_HORIZONTAL:
	    PSDRV_WriteGSave(dc);
	    PSDRV_Clip(dc, EO);
	    PSDRV_WriteRotate(dc, 90.0);
	    PSDRV_WriteHatch(dc);
	    PSDRV_WriteStroke(dc);
	    PSDRV_WriteGRestore(dc);
	    break;

	case HS_FDIAGONAL:
	case HS_DIAGCROSS:
	    PSDRV_WriteGSave(dc);
	    PSDRV_Clip(dc, EO);
	    PSDRV_WriteRotate(dc, -45.0);
	    PSDRV_WriteHatch(dc);
	    PSDRV_WriteStroke(dc);
	    PSDRV_WriteGRestore(dc);
	    if(brush->logbrush.lbHatch == HS_FDIAGONAL)
	        break;
	    /* else fallthrough for HS_DIAGCROSS */
	    
	case HS_BDIAGONAL:
	    PSDRV_WriteGSave(dc);
	    PSDRV_Clip(dc, EO);
	    PSDRV_WriteRotate(dc, 45.0);
	    PSDRV_WriteHatch(dc);
	    PSDRV_WriteStroke(dc);
	    PSDRV_WriteGRestore(dc);
	    break;

	default:
	    ERR(psdrv, "Unknown hatch style\n");
	    return FALSE;
	}
	return TRUE;
	break;

    case BS_NULL:
        return TRUE;
	break;

    default:
        return FALSE;
	break;
    }
}




