/*
 *	PostScript colour functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include "psdrv.h"
#include "debug.h"

/**********************************************************************
 *	     PSDRV_CmpColor
 *
 * Return TRUE if col1 == col2
 */ 
BOOL32 PSDRV_CmpColor(PSCOLOR *col1, PSCOLOR *col2)
{
    if(col1->type != col2->type)
        return FALSE;

    switch(col1->type) {
    case PSCOLOR_GRAY:
        if(col1->value.gray.i == col2->value.gray.i)
	    return TRUE;
	break;
    case PSCOLOR_RGB:
        if( col1->value.rgb.r == col2->value.rgb.r &&
	    col1->value.rgb.g == col2->value.rgb.g &&
	    col1->value.rgb.b == col2->value.rgb.b )
	    return TRUE;
	break;
    default:
        ERR(psdrv, "Unknown colour type %d\n", col1->type);
    }
    return FALSE;
}


/**********************************************************************
 *	     PSDRV_CopyColor
 *
 * Copies col2 into col1. Return FALSE on error.
 */ 
BOOL32 PSDRV_CopyColor(PSCOLOR *col1, PSCOLOR *col2)
{

    switch(col2->type) {
    case PSCOLOR_GRAY:
        col1->value.gray.i = col2->value.gray.i;
	break;

    case PSCOLOR_RGB:
        col1->value.rgb.r = col2->value.rgb.r;
	col1->value.rgb.g = col2->value.rgb.g;
	col1->value.rgb.b = col2->value.rgb.b;
	break;

    default:
        ERR(psdrv, "Unknown colour type %d\n", col1->type);
	return FALSE;
    }

    col1->type = col2->type;
    return TRUE;
}


/**********************************************************************
 *	     PSDRV_CreateColor
 *
 * Creates a PostScript colour from a COLORREF.
 * Result is grey scale if ColorDevice field of ppd is FALSE else an
 * rgb colour is produced.
 */
void PSDRV_CreateColor( PSDRV_PDEVICE *physDev, PSCOLOR *pscolor,
		     COLORREF wincolor )
{
    int ctype = wincolor >> 24;
    float r, g, b;

    if(ctype != 0)
        FIXME(psdrv, "Colour is %08lx\n", wincolor);

    r = (wincolor & 0xff) / 256.0;
    g = ((wincolor >> 8) & 0xff) / 256.0;
    b = ((wincolor >> 16) & 0xff) / 256.0;

    if(physDev->pi->ppd->ColorDevice) {
        pscolor->type = PSCOLOR_RGB;
	pscolor->value.rgb.r = r;
	pscolor->value.rgb.g = g;
	pscolor->value.rgb.b = b;
    } else {
        pscolor->type = PSCOLOR_GRAY;
	/* FIXME configurable */
	pscolor->value.gray.i = r * 0.3 + g * 0.59 + b * 0.11;
    }
    return;
}


/***********************************************************************
 *           PSDRV_SetBkColor
 */
COLORREF PSDRV_SetBkColor( DC *dc, COLORREF color )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    COLORREF oldColor;

    oldColor = dc->w.backgroundColor;
    dc->w.backgroundColor = color;

    PSDRV_CreateColor(physDev, &physDev->bkColor, color);

    return oldColor;
}


/***********************************************************************
 *           PSDRV_SetTextColor
 */
COLORREF PSDRV_SetTextColor( DC *dc, COLORREF color )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    COLORREF oldColor;

    oldColor = dc->w.textColor;
    dc->w.textColor = color;

    PSDRV_CreateColor(physDev, &physDev->font.color, color);
    physDev->font.set = FALSE;
    return oldColor;
}

