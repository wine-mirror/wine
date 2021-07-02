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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

  /* GDI logical pen object */
typedef struct
{
    struct gdi_obj_header obj;
    struct brush_pattern  pattern;
    EXTLOGPEN             logpen;
} PENOBJ;


static INT PEN_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL PEN_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs pen_funcs =
{
    PEN_GetObject,     /* pGetObjectA */
    PEN_GetObject,     /* pGetObjectW */
    NULL,              /* pUnrealizeObject */
    PEN_DeleteObject   /* pDeleteObject */
};


/***********************************************************************
 *           CreatePen    (GDI32.@)
 */
HPEN WINAPI CreatePen( INT style, INT width, COLORREF color )
{
    LOGPEN logpen;

    TRACE("%d %d %06x\n", style, width, color );

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

    if (pen->lopnStyle == PS_NULL)
    {
        hpen = GetStockObject(NULL_PEN);
        if (hpen) return hpen;
    }

    if (!(penPtr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*penPtr) ))) return 0;

    penPtr->logpen.elpPenStyle = pen->lopnStyle;
    penPtr->logpen.elpWidth = abs(pen->lopnWidth.x);
    penPtr->logpen.elpColor = pen->lopnColor;
    penPtr->logpen.elpBrushStyle = BS_SOLID;

    switch (pen->lopnStyle)
    {
    case PS_SOLID:
    case PS_DASH:
    case PS_DOT:
    case PS_DASHDOT:
    case PS_DASHDOTDOT:
    case PS_INSIDEFRAME:
        break;
    case PS_NULL:
        penPtr->logpen.elpWidth = 1;
        penPtr->logpen.elpColor = 0;
        break;
    default:
        penPtr->logpen.elpPenStyle = PS_SOLID;
        break;
    }

    if (!(hpen = alloc_gdi_handle( &penPtr->obj, OBJ_PEN, &pen_funcs )))
        HeapFree( GetProcessHeap(), 0, penPtr );
    return hpen;
}

/***********************************************************************
 *           ExtCreatePen    (GDI32.@)
 */

HPEN WINAPI ExtCreatePen( DWORD style, DWORD width,
                              const LOGBRUSH * brush, DWORD style_count,
                              const DWORD *style_bits )
{
    PENOBJ *penPtr = NULL;
    HPEN hpen;
    LOGBRUSH logbrush;

    if ((style_count || style_bits) && (style & PS_STYLE_MASK) != PS_USERSTYLE)
        goto invalid;

    switch (style & PS_STYLE_MASK)
    {
    case PS_NULL:
        return CreatePen( PS_NULL, 0, brush->lbColor );

    case PS_SOLID:
    case PS_DASH:
    case PS_DOT:
    case PS_DASHDOT:
    case PS_DASHDOTDOT:
        break;

    case PS_USERSTYLE:
        if (((INT)style_count) <= 0) return 0;

        if ((style_count > 16) || !style_bits) goto invalid;

        if ((style & PS_TYPE_MASK) == PS_GEOMETRIC)
        {
            UINT i;
            BOOL has_neg = FALSE, all_zero = TRUE;

            for(i = 0; (i < style_count) && !has_neg; i++)
            {
                has_neg = has_neg || (((INT)(style_bits[i])) < 0);
                all_zero = all_zero && (style_bits[i] == 0);
            }

            if (all_zero || has_neg) goto invalid;
        }
        break;

    case PS_INSIDEFRAME:  /* applicable only for geometric pens */
        if ((style & PS_TYPE_MASK) != PS_GEOMETRIC) goto invalid;
        break;

    case PS_ALTERNATE:  /* applicable only for cosmetic pens */
        if ((style & PS_TYPE_MASK) == PS_GEOMETRIC) goto invalid;
        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if ((style & PS_TYPE_MASK) == PS_GEOMETRIC)
    {
        if (brush->lbStyle == BS_NULL) return CreatePen( PS_NULL, 0, 0 );
    }
    else
    {
        if (width != 1) goto invalid;
        if (brush->lbStyle != BS_SOLID) goto invalid;
    }

    if (!(penPtr = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(PENOBJ,logpen.elpStyleEntry[style_count]))))
        return 0;

    logbrush = *brush;
    if (!store_brush_pattern( &logbrush, &penPtr->pattern )) goto invalid;
    if (logbrush.lbStyle == BS_DIBPATTERN) logbrush.lbStyle = BS_DIBPATTERNPT;

    penPtr->logpen.elpPenStyle = style;
    penPtr->logpen.elpWidth = abs((int)width);
    penPtr->logpen.elpBrushStyle = logbrush.lbStyle;
    penPtr->logpen.elpColor = logbrush.lbColor;
    penPtr->logpen.elpHatch = brush->lbHatch;
    penPtr->logpen.elpNumEntries = style_count;
    memcpy(penPtr->logpen.elpStyleEntry, style_bits, style_count * sizeof(DWORD));

    if (!(hpen = alloc_gdi_handle( &penPtr->obj, OBJ_EXTPEN, &pen_funcs )))
    {
        free_brush_pattern( &penPtr->pattern );
        HeapFree( GetProcessHeap(), 0, penPtr );
    }
    return hpen;

invalid:
    HeapFree( GetProcessHeap(), 0, penPtr );
    SetLastError( ERROR_INVALID_PARAMETER );
    return 0;
}

/***********************************************************************
 *           NtGdiSelectPen (win32u.@)
 */
HGDIOBJ WINAPI NtGdiSelectPen( HDC hdc, HGDIOBJ handle )
{
    PENOBJ *pen;
    HGDIOBJ ret = 0;
    WORD type;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((pen = get_any_obj_ptr( handle, &type )))
    {
        struct brush_pattern *pattern;
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSelectPen );

        switch (type)
        {
        case OBJ_PEN:
            pattern = NULL;
            break;
        case OBJ_EXTPEN:
            pattern = &pen->pattern;
            if (!pattern->info) pattern = NULL;
            break;
        default:
            GDI_ReleaseObj( handle );
            release_dc_ptr( dc );
            return 0;
        }

        GDI_inc_ref_count( handle );
        GDI_ReleaseObj( handle );

        if (!physdev->funcs->pSelectPen( physdev, handle, pattern ))
        {
            GDI_dec_ref_count( handle );
        }
        else
        {
            ret = dc->hPen;
            dc->hPen = handle;
            GDI_dec_ref_count( ret );
        }
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           PEN_DeleteObject
 */
static BOOL PEN_DeleteObject( HGDIOBJ handle )
{
    PENOBJ *pen = free_gdi_handle( handle );

    if (!pen) return FALSE;
    free_brush_pattern( &pen->pattern );
    HeapFree( GetProcessHeap(), 0, pen );
    return TRUE;
}


/***********************************************************************
 *           PEN_GetObject
 */
static INT PEN_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    WORD type;
    PENOBJ *pen = get_any_obj_ptr( handle, &type );
    INT ret = 0;

    if (!pen) return 0;

    switch (type)
    {
    case OBJ_PEN:
    {
        LOGPEN *lp;

        if (!buffer) ret = sizeof(LOGPEN);
        else if (count < sizeof(LOGPEN)) ret = 0;
        else if ((pen->logpen.elpPenStyle & PS_STYLE_MASK) == PS_NULL && count == sizeof(EXTLOGPEN))
        {
            EXTLOGPEN *elp = buffer;
            *elp = pen->logpen;
            elp->elpWidth = 0;
            ret = sizeof(EXTLOGPEN);
        }
        else
        {
            lp = buffer;
            lp->lopnStyle = pen->logpen.elpPenStyle;
            lp->lopnColor = pen->logpen.elpColor;
            lp->lopnWidth.x = pen->logpen.elpWidth;
            lp->lopnWidth.y = 0;
            ret = sizeof(LOGPEN);
        }
        break;
    }

    case OBJ_EXTPEN:
        ret = sizeof(EXTLOGPEN) + pen->logpen.elpNumEntries * sizeof(DWORD) - sizeof(pen->logpen.elpStyleEntry);
        if (buffer)
        {
            if (count < ret) ret = 0;
            else memcpy(buffer, &pen->logpen, ret);
        }
        break;
    }
    GDI_ReleaseObj( handle );
    return ret;
}
