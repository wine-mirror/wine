/*
 *	PostScript pen handling
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include "psdrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(psdrv);

static char PEN_dash[]       = "50 30";     /* -----   -----   -----  */
static char PEN_dot[]        = "20";      /* --  --  --  --  --  -- */
static char PEN_dashdot[]    = "40 30 20 30";  /* ----   --   ----   --  */
static char PEN_dashdotdot[] = "40 20 20 20 20 20"; /* ----  --  --  ----  */
static char PEN_alternate[]  = "1";

/***********************************************************************
 *           PSDRV_PEN_SelectObject
 */
HPEN PSDRV_PEN_SelectObject( DC * dc, HPEN hpen )
{
    LOGPEN logpen;
    HPEN prevpen = dc->hPen;
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if (!GetObjectA( hpen, sizeof(logpen), &logpen )) return 0;

    TRACE("hpen = %08x colour = %08lx\n", hpen, logpen.lopnColor);

    dc->hPen = hpen;

    physDev->pen.width = INTERNAL_XWSTODS(dc, logpen.lopnWidth.x);
    if(physDev->pen.width < 0)
        physDev->pen.width = -physDev->pen.width;

    PSDRV_CreateColor(physDev, &physDev->pen.color, logpen.lopnColor);
    physDev->pen.style = logpen.lopnStyle & PS_STYLE_MASK;
 
    switch(physDev->pen.style) {
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
    }	    

    if ((physDev->pen.width > 1) && (physDev->pen.dash != NULL)) {
	physDev->pen.style = PS_SOLID;
         physDev->pen.dash = NULL;
    } 

    physDev->pen.set = FALSE;
    return prevpen;
}


/**********************************************************************
 *
 *	PSDRV_SetPen
 *
 */
BOOL PSDRV_SetPen(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if (physDev->pen.style != PS_NULL) {
	PSDRV_WriteSetColor(dc, &physDev->pen.color);
	
	if(!physDev->pen.set) {
	    PSDRV_WriteSetPen(dc);
	    physDev->pen.set = TRUE;
	}    
    }

    return TRUE;
}
