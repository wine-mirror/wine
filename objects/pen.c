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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "gdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

  /* GDI logical pen object */
typedef struct
{
    GDIOBJHDR   header;
    LOGPEN    logpen;
} PENOBJ;


static HGDIOBJ PEN_SelectObject( HGDIOBJ handle, void *obj, HDC hdc );
static INT PEN_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static INT PEN_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );

static const struct gdi_obj_funcs pen_funcs =
{
    PEN_SelectObject,  /* pSelectObject */
    PEN_GetObject16,   /* pGetObject16 */
    PEN_GetObject,     /* pGetObjectA */
    PEN_GetObject,     /* pGetObjectW */
    NULL,              /* pUnrealizeObject */
    GDI_FreeObject     /* pDeleteObject */
};


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
 *           CreatePenIndirect    (GDI32.@)
 */
HPEN WINAPI CreatePenIndirect( const LOGPEN * pen )
{
    PENOBJ * penPtr;
    HPEN hpen;

    if (!(penPtr = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC, (HGDIOBJ *)&hpen,
				    &pen_funcs ))) return 0;
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
	if (brush->lbHatch && ((brush->lbStyle == BS_SOLID) || (brush->lbStyle == BS_HOLLOW)))
	    FIXME("Hatches not implemented\n");	

    if (!(penPtr = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC, (HGDIOBJ *)&hpen,
				    &pen_funcs ))) return 0;
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
 *           PEN_SelectObject
 */
static HGDIOBJ PEN_SelectObject( HGDIOBJ handle, void *obj, HDC hdc )
{
    HGDIOBJ ret;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;
    ret = dc->hPen;
    if (dc->funcs->pSelectPen) handle = dc->funcs->pSelectPen( dc->physDev, handle );
    if (handle) dc->hPen = handle;
    else ret = 0;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           PEN_GetObject16
 */
static INT PEN_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    PENOBJ *pen = obj;
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
static INT PEN_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    PENOBJ *pen = obj;

    if( !buffer )
        return sizeof(pen->logpen);

    if (count > sizeof(pen->logpen)) count = sizeof(pen->logpen);
    memcpy( buffer, &pen->logpen, count );
    return count;
}
