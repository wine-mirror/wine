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

static const char PEN_dash[]       = { 5,3 };      /* -----   -----   -----  */
static const char PEN_dot[]        = { 1,1 };      /* --  --  --  --  --  -- */
static const char PEN_dashdot[]    = { 4,3,2,3 };  /* ----   --   ----   --  */
static const char PEN_dashdotdot[] = { 4,2,2,2,2,2 }; /* ----  --  --  ----  */
static const char PEN_alternate[]  = { 1,1 };      /* FIXME */

/***********************************************************************
 *           PEN_SelectObject
 */
HPEN X11DRV_PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen )
{
    HPEN prevHandle = dc->w.hPen;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    dc->w.hPen = hpen;
    physDev->pen.style = pen->logpen.lopnStyle & PS_STYLE_MASK;
    physDev->pen.type = pen->logpen.lopnStyle & PS_TYPE_MASK;
    physDev->pen.endcap = pen->logpen.lopnStyle & PS_ENDCAP_MASK;
    physDev->pen.linejoin = pen->logpen.lopnStyle & PS_JOIN_MASK;

    physDev->pen.width = (pen->logpen.lopnWidth.x * dc->vportExtX +
                    dc->wndExtX / 2) / dc->wndExtX;
    if (physDev->pen.width < 0) physDev->pen.width = -physDev->pen.width;
    if (physDev->pen.width == 1) physDev->pen.width = 0;  /* Faster */
    physDev->pen.pixel = X11DRV_PALETTE_ToPhysical( dc, pen->logpen.lopnColor );    
    switch(pen->logpen.lopnStyle & PS_STYLE_MASK)
    {
      case PS_DASH:
	physDev->pen.dashes = (char *)PEN_dash;
	physDev->pen.dash_len = 2;
	break;
      case PS_DOT:
	physDev->pen.dashes = (char *)PEN_dot;
	physDev->pen.dash_len = 2;
	break;
      case PS_DASHDOT:
	physDev->pen.dashes = (char *)PEN_dashdot;
	physDev->pen.dash_len = 4;
	break;
      case PS_DASHDOTDOT:
	physDev->pen.dashes = (char *)PEN_dashdotdot;
	physDev->pen.dash_len = 6;
	break;
      case PS_ALTERNATE:
	/* FIXME: should be alternating _pixels_ that are set */
	physDev->pen.dashes = (char *)PEN_alternate;
	physDev->pen.dash_len = 2;
	break;
      case PS_USERSTYLE:
	/* FIXME */
	break;
    }
    
    return prevHandle;
}
