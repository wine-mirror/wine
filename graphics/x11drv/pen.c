/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "pen.h"
#include "color.h"
#include "stddebug.h"
#include "debug.h"

static const char PEN_dash[]       = { 5,3 };      /* -----   -----   -----  */
static const char PEN_dot[]        = { 1,1 };      /* --  --  --  --  --  -- */
static const char PEN_dashdot[]    = { 4,3,2,3 };  /* ----   --   ----   --  */
static const char PEN_dashdotdot[] = { 4,2,2,2,2,2 }; /* ----  --  --  ----  */

/***********************************************************************
 *           PEN_SelectObject
 */
HPEN32 X11DRV_PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen )
{
    HPEN32 prevHandle = dc->w.hPen;

    dc->w.hPen = hpen;
    dc->u.x.pen.style = pen->logpen.lopnStyle;
    dc->u.x.pen.width = pen->logpen.lopnWidth.x * dc->vportExtX / dc->wndExtX;
    if (dc->u.x.pen.width < 0) dc->u.x.pen.width = -dc->u.x.pen.width;
    if (dc->u.x.pen.width == 1) dc->u.x.pen.width = 0;  /* Faster */
    dc->u.x.pen.pixel = COLOR_ToPhysical( dc, pen->logpen.lopnColor );    
    switch(pen->logpen.lopnStyle)
    {
      case PS_DASH:
	dc->u.x.pen.dashes = (char *)PEN_dash;
	dc->u.x.pen.dash_len = 2;
	break;
      case PS_DOT:
	dc->u.x.pen.dashes = (char *)PEN_dot;
	dc->u.x.pen.dash_len = 2;
	break;
      case PS_DASHDOT:
	dc->u.x.pen.dashes = (char *)PEN_dashdot;
	dc->u.x.pen.dash_len = 4;
	break;
      case PS_DASHDOTDOT:
	dc->u.x.pen.dashes = (char *)PEN_dashdotdot;
	dc->u.x.pen.dash_len = 6;
	break;
    }
    
    return prevHandle;
}
