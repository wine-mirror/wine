/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>

#include "windef.h"
#include "wingdi.h"

#include "wine/wingdi16.h"
#include "pen.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);



/***********************************************************************
 *           CreatePen    (GDI.61)
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
 *           CreatePen    (GDI32.@)
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
 *           CreatePenIndirect    (GDI.62)
 */
HPEN16 WINAPI CreatePenIndirect16( const LOGPEN16 * pen )
{
    PENOBJ * penPtr;
    HPEN hpen;

    if (pen->lopnStyle > PS_INSIDEFRAME) return 0;
    if (!(penPtr = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC, &hpen ))) return 0;
    penPtr->logpen.lopnStyle = pen->lopnStyle;
    penPtr->logpen.lopnColor = pen->lopnColor;
    CONV_POINT16TO32( &pen->lopnWidth, &penPtr->logpen.lopnWidth );
    GDI_ReleaseObj( hpen );
    return hpen;
}


/***********************************************************************
 *           CreatePenIndirect    (GDI32.@)
 */
HPEN WINAPI CreatePenIndirect( const LOGPEN * pen )
{
    PENOBJ * penPtr;
    HPEN hpen;

    if (!(penPtr = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC, &hpen ))) return 0;
    penPtr->logpen.lopnStyle = pen->lopnStyle;
    penPtr->logpen.lopnWidth = pen->lopnWidth;
    penPtr->logpen.lopnColor = pen->lopnColor;
    GDI_ReleaseObj( hpen );
    return hpen;
}

/***********************************************************************
 *           ExtCreatePen    (GDI32.@)
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

    if (!(penPtr = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC, &hpen ))) return 0;
    penPtr->logpen.lopnStyle = style & ~PS_TYPE_MASK; 
    
    /* PS_USERSTYLE workaround */   
    if((penPtr->logpen.lopnStyle & PS_STYLE_MASK) == PS_USERSTYLE)
       penPtr->logpen.lopnStyle = 
         (penPtr->logpen.lopnStyle & ~PS_STYLE_MASK) | PS_SOLID;

    penPtr->logpen.lopnWidth.x = (style & PS_GEOMETRIC) ? width : 1; 
    penPtr->logpen.lopnWidth.y = 0;
    penPtr->logpen.lopnColor = brush->lbColor;
    GDI_ReleaseObj( hpen );

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
 *           PEN_GetObject
 */
INT PEN_GetObject( PENOBJ * pen, INT count, LPSTR buffer )
{
    if (count > sizeof(pen->logpen)) count = sizeof(pen->logpen);
    memcpy( buffer, &pen->logpen, count );
    return count;
}

