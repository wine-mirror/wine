/*
 *	PostScript pen handling
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include "windows.h"
#include "pen.h"
#include "psdrv.h"
#include "debug.h"

static char PEN_dash[]       = "50 30";     /* -----   -----   -----  */
static char PEN_dot[]        = "20";      /* --  --  --  --  --  -- */
static char PEN_dashdot[]    = "40 30 20 30";  /* ----   --   ----   --  */
static char PEN_dashdotdot[] = "40 20 20 20 20 20"; /* ----  --  --  ----  */
static char PEN_alternate[]  = "1";

/***********************************************************************
 *           PSDRV_PEN_SelectObject
 */
extern HPEN32 PSDRV_PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen )
{
    HPEN32 prevpen = dc->w.hPen;
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    TRACE(psdrv, "hpen = %08x colour = %08lx\n", hpen, pen->logpen.lopnColor);
    dc->w.hPen = hpen;

    physDev->pen.width = XLSTODS(dc, pen->logpen.lopnWidth.x);
    if(physDev->pen.width < 0)
        physDev->pen.width = -physDev->pen.width;

    PSDRV_CreateColor(physDev, &physDev->pen.color, pen->logpen.lopnColor);

    if(physDev->pen.width > 1) { /* dashes only for 0 or 1 pixel pens */
        physDev->pen.dash = NULL;
    } else {
        switch(pen->logpen.lopnStyle & PS_STYLE_MASK) {
	case PS_DASH:
	    physDev->pen.dash = PEN_dash;
	    break;

	case PS_DOT:
	    physDev->pen.dash = PEN_dot;
	    break;

	case PS_DASHDOT:
	    physDev->pen.dash = PEN_dashdot;
	    break;

	case PS_DASHDOTDOT:
            physDev->pen.dash = PEN_dashdotdot;
	    break;

	case PS_ALTERNATE:
            physDev->pen.dash = PEN_alternate;
	    break;

	default:
            physDev->pen.dash = NULL;
	    break;
	}
    }

    physDev->pen.set = FALSE;
    return prevpen;
}


/**********************************************************************
 *
 *	PSDRV_SetPen
 *
 */
BOOL32 PSDRV_SetPen(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    PSDRV_WriteSetColor(dc, &physDev->pen.color);

    if(!physDev->pen.set) {
        PSDRV_WriteSetPen(dc);
	physDev->pen.set = TRUE;
    }

    return TRUE;
}


