/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "pen.h"
#include "metafile.h"
#include "stddebug.h"
#include "color.h"
#include "debug.h"

/***********************************************************************
 *           CreatePen    (GDI.61)
 */
HPEN CreatePen( INT style, INT width, COLORREF color )
{
    LOGPEN logpen = { style, { width, 0 }, color };
    dprintf_gdi(stddeb, "CreatePen: %d %d %06lx\n", style, width, color );
    return CreatePenIndirect( &logpen );
}


/***********************************************************************
 *           CreatePenIndirect    (GDI.62)
 */
HPEN CreatePenIndirect( LOGPEN * pen )
{
    PENOBJ * penPtr;
    HPEN hpen;

    if (pen->lopnStyle > PS_INSIDEFRAME) return 0;
    hpen = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC );
    if (!hpen) return 0;
    penPtr = (PENOBJ *) GDI_HEAP_LIN_ADDR( hpen );    
    memcpy( &penPtr->logpen, pen, sizeof(LOGPEN) );
    return hpen;
}


/***********************************************************************
 *           PEN_GetObject
 */
int PEN_GetObject( PENOBJ * pen, int count, LPSTR buffer )
{
    if (count > sizeof(LOGPEN)) count = sizeof(LOGPEN);
    memcpy( buffer, &pen->logpen, count );
    return count;
}


/***********************************************************************
 *           PEN_SelectObject
 */
HPEN PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen )
{
    static char dash_dash[]       = { 5, 3 };      /* -----   -----   -----  */
    static char dash_dot[]        = { 2, 2 };      /* --  --  --  --  --  -- */
    static char dash_dashdot[]    = { 4,3,2,3 };   /* ----   --   ----   --  */
    static char dash_dashdotdot[] = { 4,2,2,2,2,2 };  /* ----  --  --  ----  */
    HPEN prevHandle = dc->w.hPen;

    if (dc->header.wMagic == METAFILE_DC_MAGIC)
      if (MF_CreatePenIndirect(dc, hpen, &(pen->logpen)))
	return prevHandle;
      else
	return 0;

    dc->w.hPen = hpen;

    dc->u.x.pen.style = pen->logpen.lopnStyle;
    dc->u.x.pen.width = pen->logpen.lopnWidth.x * dc->w.VportExtX
	                  / dc->w.WndExtX;
    if (dc->u.x.pen.width < 0) dc->u.x.pen.width = -dc->u.x.pen.width;
    if (dc->u.x.pen.width == 1) dc->u.x.pen.width = 0;  /* Faster */
    dc->u.x.pen.pixel = COLOR_ToPhysical( dc, pen->logpen.lopnColor );    
    switch(pen->logpen.lopnStyle)
    {
      case PS_DASH:
	dc->u.x.pen.dashes = dash_dash;
	dc->u.x.pen.dash_len = 2;
	break;
      case PS_DOT:
	dc->u.x.pen.dashes = dash_dot;
	dc->u.x.pen.dash_len = 2;
	break;
      case PS_DASHDOT:
	dc->u.x.pen.dashes = dash_dashdot;
	dc->u.x.pen.dash_len = 4;
	break;
      case PS_DASHDOTDOT:
	dc->u.x.pen.dashes = dash_dashdotdot;
	dc->u.x.pen.dash_len = 6;
	break;
    }
    
    return prevHandle;
}
