/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include "pen.h"
#include "metafile.h"
#include "color.h"
#include "stddebug.h"
#include "debug.h"


static const char PEN_dash[]       = { 5,3 };      /* -----   -----   -----  */
static const char PEN_dot[]        = { 2,2 };      /* --  --  --  --  --  -- */
static const char PEN_dashdot[]    = { 4,3,2,3 };  /* ----   --   ----   --  */
static const char PEN_dashdotdot[] = { 4,2,2,2,2,2 }; /* ----  --  --  ----  */

/***********************************************************************
 *           CreatePen16    (GDI.61)
 */
HPEN16 CreatePen16( INT16 style, INT16 width, COLORREF color )
{
    LOGPEN32 logpen = { style, { width, 0 }, color };
    dprintf_gdi(stddeb, "CreatePen16: %d %d %06lx\n", style, width, color );
    return CreatePenIndirect32( &logpen );
}


/***********************************************************************
 *           CreatePen32    (GDI32.55)
 */
HPEN32 CreatePen32( INT32 style, INT32 width, COLORREF color )
{
    LOGPEN32 logpen = { style, { width, 0 }, color };
    dprintf_gdi(stddeb, "CreatePen32: %d %d %06lx\n", style, width, color );
    return CreatePenIndirect32( &logpen );
}


/***********************************************************************
 *           CreatePenIndirect16    (GDI.62)
 */
HPEN16 CreatePenIndirect16( const LOGPEN16 * pen )
{
    PENOBJ * penPtr;
    HPEN16 hpen;

    if (pen->lopnStyle > PS_INSIDEFRAME) return 0;
    hpen = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC );
    if (!hpen) return 0;
    penPtr = (PENOBJ *)GDI_HEAP_LIN_ADDR( hpen );
    penPtr->logpen.lopnStyle = pen->lopnStyle;
    penPtr->logpen.lopnColor = pen->lopnColor;
    CONV_POINT16TO32( &pen->lopnWidth, &penPtr->logpen.lopnWidth );
    return hpen;
}


/***********************************************************************
 *           CreatePenIndirect32    (GDI32.56)
 */
HPEN32 CreatePenIndirect32( const LOGPEN32 * pen )
{
    PENOBJ * penPtr;
    HPEN32 hpen;

    if (pen->lopnStyle > PS_INSIDEFRAME) return 0;
    hpen = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC );
    if (!hpen) return 0;
    penPtr = (PENOBJ *)GDI_HEAP_LIN_ADDR( hpen );
    penPtr->logpen.lopnStyle = pen->lopnStyle;
    penPtr->logpen.lopnWidth = pen->lopnWidth;
    penPtr->logpen.lopnColor = pen->lopnColor;
    return hpen;
}


/***********************************************************************
 *           PEN_GetObject16
 */
INT16 PEN_GetObject16( PENOBJ * pen, INT16 count, LPSTR buffer )
{
    LOGPEN16 logpen;
    logpen.lopnStyle = pen->logpen.lopnStyle;
    logpen.lopnColor = pen->logpen.lopnColor;
    CONV_POINT32TO16( &pen->logpen.lopnWidth, &logpen.lopnWidth );
    if (count > sizeof(logpen)) count = sizeof(logpen);
    memcpy( buffer, &logpen, count );
    return count;
}


/***********************************************************************
 *           PEN_GetObject32
 */
INT32 PEN_GetObject32( PENOBJ * pen, INT32 count, LPSTR buffer )
{
    if (count > sizeof(pen->logpen)) count = sizeof(pen->logpen);
    memcpy( buffer, &pen->logpen, count );
    return count;
}


/***********************************************************************
 *           PEN_SelectObject
 */
HPEN32 PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen )
{
    HPEN32 prevHandle = dc->w.hPen;

    if (dc->header.wMagic == METAFILE_DC_MAGIC)
    {
        LOGPEN16 logpen = { pen->logpen.lopnStyle,
                            { pen->logpen.lopnWidth.x,
                              pen->logpen.lopnWidth.y },
                            pen->logpen.lopnColor };
        if (MF_CreatePenIndirect( dc, hpen, &logpen )) return prevHandle;
        else return 0;
    }

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
