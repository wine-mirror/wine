/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "pen.h"
#include "metafile.h"
#include "color.h"
#include "debug.h"



/***********************************************************************
 *           CreatePen16    (GDI.61)
 */
HPEN16 WINAPI CreatePen16( INT16 style, INT16 width, COLORREF color )
{
    LOGPEN32 logpen = { style, { width, 0 }, color };
    dprintf_info(gdi, "CreatePen16: %d %d %06lx\n", style, width, color );
    return CreatePenIndirect32( &logpen );
}


/***********************************************************************
 *           CreatePen32    (GDI32.55)
 */
HPEN32 WINAPI CreatePen32( INT32 style, INT32 width, COLORREF color )
{
    LOGPEN32 logpen = { style, { width, 0 }, color };
    dprintf_info(gdi, "CreatePen32: %d %d %06lx\n", style, width, color );
    return CreatePenIndirect32( &logpen );
}


/***********************************************************************
 *           CreatePenIndirect16    (GDI.62)
 */
HPEN16 WINAPI CreatePenIndirect16( const LOGPEN16 * pen )
{
    PENOBJ * penPtr;
    HPEN16 hpen;

    if (pen->lopnStyle > PS_INSIDEFRAME) return 0;
    hpen = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC );
    if (!hpen) return 0;
    penPtr = (PENOBJ *)GDI_HEAP_LOCK( hpen );
    penPtr->logpen.lopnStyle = pen->lopnStyle;
    penPtr->logpen.lopnColor = pen->lopnColor;
    CONV_POINT16TO32( &pen->lopnWidth, &penPtr->logpen.lopnWidth );
    GDI_HEAP_UNLOCK( hpen );
    return hpen;
}


/***********************************************************************
 *           CreatePenIndirect32    (GDI32.56)
 */
HPEN32 WINAPI CreatePenIndirect32( const LOGPEN32 * pen )
{
    PENOBJ * penPtr;
    HPEN32 hpen;

    if (pen->lopnStyle > PS_INSIDEFRAME) return 0;
    hpen = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC );
    if (!hpen) return 0;
    penPtr = (PENOBJ *)GDI_HEAP_LOCK( hpen );
    penPtr->logpen.lopnStyle = pen->lopnStyle;
    penPtr->logpen.lopnWidth = pen->lopnWidth;
    penPtr->logpen.lopnColor = pen->lopnColor;
    GDI_HEAP_UNLOCK( hpen );
    return hpen;
}

/***********************************************************************
 *           ExtCreatePen32    (GDI32.93)
 *
 * FIXME: PS_USERSTYLE not handled
 */

HPEN32 WINAPI ExtCreatePen32( DWORD style, DWORD width,
                              const LOGBRUSH32 * brush, DWORD style_count,
                              const DWORD *style_bits )
{
    LOGPEN32 logpen;

    if ((style & PS_STYLE_MASK) == PS_USERSTYLE)
	fprintf(stderr, "ExtCreatePen: PS_USERSTYLE not handled\n");
    if ((style & PS_TYPE_MASK) == PS_GEOMETRIC)
	if (brush->lbHatch)
	    fprintf(stderr, "ExtCreatePen: Hatches not implemented\n");

    logpen.lopnStyle = style & ~PS_TYPE_MASK;
    logpen.lopnWidth.x = (style & PS_GEOMETRIC) ? width : 1;
    logpen.lopnWidth.y = 0;
    logpen.lopnColor = brush->lbColor;
    return CreatePenIndirect32( &logpen );
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

