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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
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
    PEN_GetObject,     /* pGetObjectW */
    NULL,              /* pUnrealizeObject */
    PEN_DeleteObject   /* pDeleteObject */
};

HPEN create_pen( INT style, INT width, COLORREF color )
{
    PENOBJ *penPtr;
    HPEN hpen;

    TRACE( "%d %d %s\n", style, width, debugstr_color(color) );

    switch (style)
    {
    case PS_SOLID:
    case PS_DASH:
    case PS_DOT:
    case PS_DASHDOT:
    case PS_DASHDOTDOT:
    case PS_INSIDEFRAME:
        break;
    case PS_NULL:
        width = 1;
        color = 0;
        break;
    default:
        return 0;
    }

    if (!(penPtr = calloc( 1, sizeof(*penPtr) ))) return 0;

    penPtr->logpen.elpPenStyle   = style;
    penPtr->logpen.elpWidth      = abs(width);
    penPtr->logpen.elpColor      = color;
    penPtr->logpen.elpBrushStyle = BS_SOLID;

    if (!(hpen = alloc_gdi_handle( &penPtr->obj, NTGDI_OBJ_PEN, &pen_funcs )))
        free( penPtr );
    return hpen;
}

/***********************************************************************
 *           NtGdiCreatePen    (win32u.@)
 */
HPEN WINAPI NtGdiCreatePen( INT style, INT width, COLORREF color, HBRUSH brush )
{
    if (brush) FIXME( "brush not supported\n" );
    if (style == PS_NULL) return GetStockObject( NULL_PEN );
    return create_pen( style, width, color );
}

/***********************************************************************
 *           NtGdiExtCreatePen    (win32u.@)
 */
HPEN WINAPI NtGdiExtCreatePen( DWORD style, DWORD width, ULONG brush_style, ULONG color,
                               ULONG_PTR client_hatch, ULONG_PTR hatch, DWORD style_count,
                               const DWORD *style_bits, ULONG dib_size, BOOL old_style,
                               HBRUSH brush )
{
    PENOBJ *penPtr = NULL;
    HPEN hpen;
    LOGBRUSH logbrush;

    if ((style_count || style_bits) && (style & PS_STYLE_MASK) != PS_USERSTYLE)
        goto invalid;

    switch (style & PS_STYLE_MASK)
    {
    case PS_NULL:
        return NtGdiCreatePen( PS_NULL, 0, color, NULL );

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
        RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if ((style & PS_TYPE_MASK) == PS_GEOMETRIC)
    {
        if (brush_style == BS_NULL) return NtGdiCreatePen( PS_NULL, 0, 0, NULL );
    }
    else
    {
        if (width != 1) goto invalid;
        if (brush_style != BS_SOLID) goto invalid;
    }

    if (!(penPtr = malloc( FIELD_OFFSET(PENOBJ,logpen.elpStyleEntry[style_count]) )))
        return 0;

    logbrush.lbStyle = brush_style;
    logbrush.lbColor = color;
    logbrush.lbHatch = hatch;
    if (!store_brush_pattern( &logbrush, &penPtr->pattern )) goto invalid;
    if (logbrush.lbStyle == BS_DIBPATTERN) logbrush.lbStyle = BS_DIBPATTERNPT;

    penPtr->logpen.elpPenStyle = style;
    penPtr->logpen.elpWidth = abs((int)width);
    penPtr->logpen.elpBrushStyle = logbrush.lbStyle;
    penPtr->logpen.elpColor = logbrush.lbColor;
    penPtr->logpen.elpHatch = client_hatch;
    penPtr->logpen.elpNumEntries = style_count;
    memcpy(penPtr->logpen.elpStyleEntry, style_bits, style_count * sizeof(DWORD));

    if (!(hpen = alloc_gdi_handle( &penPtr->obj, NTGDI_OBJ_EXTPEN, &pen_funcs )))
    {
        free_brush_pattern( &penPtr->pattern );
        free( penPtr );
    }
    return hpen;

invalid:
    free( penPtr );
    RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
    return 0;
}

/***********************************************************************
 *           NtGdiSelectPen (win32u.@)
 */
HGDIOBJ WINAPI NtGdiSelectPen( HDC hdc, HGDIOBJ handle )
{
    PENOBJ *pen;
    HGDIOBJ ret = 0;
    DWORD type;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((pen = get_any_obj_ptr( handle, &type )))
    {
        struct brush_pattern *pattern;
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSelectPen );

        switch (type)
        {
        case NTGDI_OBJ_PEN:
            pattern = NULL;
            break;
        case NTGDI_OBJ_EXTPEN:
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
    free( pen );
    return TRUE;
}


/***********************************************************************
 *           PEN_GetObject
 */
static INT PEN_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    DWORD type;
    PENOBJ *pen = get_any_obj_ptr( handle, &type );
    INT ret = 0;

    if (!pen) return 0;

    switch (type)
    {
    case NTGDI_OBJ_PEN:
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

    case NTGDI_OBJ_EXTPEN:
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
