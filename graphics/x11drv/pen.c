/*
 * X11DRV pen objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "config.h"

#include "pen.h"
#include "color.h"
#include "x11drv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

static const char PEN_dash[]       = { 16,8 };
static const char PEN_dot[]        = { 4,4 };
static const char PEN_dashdot[]    = { 12,8,4,8 };
static const char PEN_dashdotdot[] = { 12,4,4,4,4,4 };
static const char PEN_alternate[]  = { 1,1 };

/***********************************************************************
 *           PEN_SelectObject
 */
HPEN X11DRV_PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen )
{
    HPEN prevHandle = dc->hPen;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    dc->hPen = hpen;
    physDev->pen.style = pen->logpen.lopnStyle & PS_STYLE_MASK;
    physDev->pen.type = pen->logpen.lopnStyle & PS_TYPE_MASK;
    physDev->pen.endcap = pen->logpen.lopnStyle & PS_ENDCAP_MASK;
    physDev->pen.linejoin = pen->logpen.lopnStyle & PS_JOIN_MASK;

    physDev->pen.width = GDI_ROUND((FLOAT)pen->logpen.lopnWidth.x *
                                   dc->xformWorld2Vport.eM11 * 0.5);
    if (physDev->pen.width < 0) physDev->pen.width = -physDev->pen.width;
    if (physDev->pen.width == 1) physDev->pen.width = 0;  /* Faster */
    physDev->pen.pixel = X11DRV_PALETTE_ToPhysical( dc, pen->logpen.lopnColor );    
    switch(pen->logpen.lopnStyle & PS_STYLE_MASK)
    {
      case PS_DASH:
	physDev->pen.dashes = (char *)PEN_dash;
	physDev->pen.dash_len = sizeof(PEN_dash)/sizeof(*PEN_dash);
	break;
      case PS_DOT:
	physDev->pen.dashes = (char *)PEN_dot;
	physDev->pen.dash_len = sizeof(PEN_dot)/sizeof(*PEN_dot);
	break;
      case PS_DASHDOT:
	physDev->pen.dashes = (char *)PEN_dashdot;
	physDev->pen.dash_len = sizeof(PEN_dashdot)/sizeof(*PEN_dashdot);
	break;
      case PS_DASHDOTDOT:
	physDev->pen.dashes = (char *)PEN_dashdotdot;
	physDev->pen.dash_len = sizeof(PEN_dashdotdot)/sizeof(*PEN_dashdotdot);
	break;
      case PS_ALTERNATE:
	physDev->pen.dashes = (char *)PEN_alternate;
	physDev->pen.dash_len = sizeof(PEN_alternate)/sizeof(*PEN_alternate);
	break;
      case PS_USERSTYLE:
        FIXME("PS_USERSTYLE is not supported\n");
	break;
    }
    
    return prevHandle;
}
