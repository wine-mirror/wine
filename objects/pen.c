/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <string.h>
#include "pen.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi)



/***********************************************************************
 *           CreatePen16    (GDI.61)
 */
HPEN16 WINAPI CreatePen16( INT16 style, INT16 width, COLORREF color )
{
    LOGPEN logpen;

    TRACE("%d %d %06lx\n", style, width, color );

    logpen.lopnStyle = style; 
    logpen.lopnWidth.x = width;
    logpen.lopnWidth.y = 0;
    logpen.lopnColor = color;

    return CreatePenIndirect( &logpen );
}


/***********************************************************************
 *           CreatePen32    (GDI32.55)
 */
HPEN WINAPI CreatePen( INT style, INT width, COLORREF color )
{
    LOGPEN logpen;

    TRACE("%d %d %06lx\n", style, width, color );

    logpen.lopnStyle = style; 
    logpen.lopnWidth.x = width;
    logpen.lopnWidth.y = 0;
    logpen.lopnColor = color;

    return CreatePenIndirect( &logpen );
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
HPEN WINAPI CreatePenIndirect( const LOGPEN * pen )
{
    PENOBJ * penPtr;
    HPEN hpen;

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

HPEN WINAPI ExtCreatePen( DWORD style, DWORD width,
                              const LOGBRUSH * brush, DWORD style_count,
                              const DWORD *style_bits )
{
    PENOBJ * penPtr;
    HPEN hpen;

    if ((style & PS_STYLE_MASK) == PS_USERSTYLE)
	FIXME("PS_USERSTYLE not handled\n");
    if ((style & PS_TYPE_MASK) == PS_GEOMETRIC)
	if (brush->lbHatch)
	    FIXME("Hatches not implemented\n");

    hpen = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC );
    if (!hpen) return 0;
    penPtr = (PENOBJ *)GDI_HEAP_LOCK( hpen );
    penPtr->logpen.lopnStyle = style & ~PS_TYPE_MASK; 
    
    /* PS_USERSTYLE and PS_ALTERNATE workaround */   
    if((penPtr->logpen.lopnStyle & PS_STYLE_MASK) > PS_INSIDEFRAME)
       penPtr->logpen.lopnStyle = 
         (penPtr->logpen.lopnStyle & ~PS_STYLE_MASK) | PS_SOLID;
    
    penPtr->logpen.lopnWidth.x = (style & PS_GEOMETRIC) ? width : 1; 
    penPtr->logpen.lopnWidth.y = 0;
    penPtr->logpen.lopnColor = brush->lbColor;
    GDI_HEAP_UNLOCK( hpen );

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
INT PEN_GetObject( PENOBJ * pen, INT count, LPSTR buffer )
{
    if (count > sizeof(pen->logpen)) count = sizeof(pen->logpen);
    memcpy( buffer, &pen->logpen, count );
    return count;
}

