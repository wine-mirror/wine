/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "gdi.h"


/***********************************************************************
 *           CreatePen    (GDI.61)
 */
HPEN CreatePen( short style, short width, COLORREF color )
{
    LOGPEN logpen = { style, { width, 0 }, color };
#ifdef DEBUG_GDI
    printf( "CreatePen: %d %d %06x\n", style, width, color );
#endif
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
    penPtr = (PENOBJ *) GDI_HEAP_ADDR( hpen );    
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
    dc->w.hPen = hpen;

    dc->u.x.pen.style = pen->logpen.lopnStyle;
    dc->u.x.pen.width = pen->logpen.lopnWidth.x * dc->w.VportExtX
	                  / dc->w.WndExtX;
    if (dc->u.x.pen.width < 0) dc->u.x.pen.width = -dc->u.x.pen.width;
    if (dc->u.x.pen.width == 1) dc->u.x.pen.width = 0;  /* Faster */
    dc->u.x.pen.pixel = GetNearestPaletteIndex( dc->w.hPalette,
					        pen->logpen.lopnColor );    
    switch(pen->logpen.lopnStyle)
    {
      case PS_DASH:
	XSetDashes( XT_display, dc->u.x.gc, 0, dash_dash, 2 );
	break;
      case PS_DOT:
	XSetDashes( XT_display, dc->u.x.gc, 0, dash_dot, 2 );
	break;
      case PS_DASHDOT:
	XSetDashes( XT_display, dc->u.x.gc, 0, dash_dashdot, 4 );
	break;
      case PS_DASHDOTDOT:
	XSetDashes( XT_display, dc->u.x.gc, 0, dash_dashdotdot, 6 );
	break;
    }
    
    return prevHandle;
}
