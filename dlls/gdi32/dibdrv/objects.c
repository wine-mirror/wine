/*
 * DIB driver GDI objects.
 *
 * Copyright 2011 Huw Davies
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

#include "gdi_private.h"
#include "dibdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

/*
 *
 * Decompose the 16 ROP2s into an expression of the form
 *
 * D = (D & A) ^ X
 *
 * Where A and X depend only on P (and so can be precomputed).
 *
 *                                       A    X
 *
 * R2_BLACK         0                    0    0
 * R2_NOTMERGEPEN   ~(D | P)            ~P   ~P
 * R2_MASKNOTPEN    ~P & D              ~P    0
 * R2_NOTCOPYPEN    ~P                   0   ~P
 * R2_MASKPENNOT    P & ~D               P    P
 * R2_NOT           ~D                   1    1
 * R2_XORPEN        P ^ D                1    P
 * R2_NOTMASKPEN    ~(P & D)             P    1
 * R2_MASKPEN       P & D                P    0
 * R2_NOTXORPEN     ~(P ^ D)             1   ~P
 * R2_NOP           D                    1    0
 * R2_MERGENOTPEN   ~P | D               P   ~P
 * R2_COPYPEN       P                    0    P
 * R2_MERGEPENNOT   P | ~D              ~P    1
 * R2_MERGEPEN      P | D               ~P    P
 * R2_WHITE         1                    0    1
 *
 */

/* A = (P & A1) | (~P & A2) */
#define ZERO {0, 0}
#define ONE {0xffffffff, 0xffffffff}
#define P {0xffffffff, 0}
#define NOT_P {0, 0xffffffff}

static const DWORD rop2_and_array[16][2] =
{
    ZERO, NOT_P, NOT_P, ZERO,
    P,    ONE,   ONE,   P,
    P,    ONE,   ONE,   P,
    ZERO, NOT_P, NOT_P, ZERO
};

/* X = (P & X1) | (~P & X2) */
static const DWORD rop2_xor_array[16][2] =
{
    ZERO, NOT_P, ZERO, NOT_P,
    P,    ONE,   P,    ONE,
    ZERO, NOT_P, ZERO, NOT_P,
    P,    ONE,   P,    ONE
};

#undef NOT_P
#undef P
#undef ONE
#undef ZERO

void calc_and_xor_masks(INT rop, DWORD color, DWORD *and, DWORD *xor)
{
    /* NB The ROP2 codes start at one and the arrays are zero-based */
    *and = (color & rop2_and_array[rop-1][0]) | ((~color) & rop2_and_array[rop-1][1]);
    *xor = (color & rop2_xor_array[rop-1][0]) | ((~color) & rop2_xor_array[rop-1][1]);
}

static inline void order_end_points(int *s, int *e)
{
    if(*s > *e)
    {
        int tmp;
        tmp = *s + 1;
        *s = *e + 1;
        *e = tmp;
    }
}

static inline BOOL pt_in_rect( const RECT *rect, const POINT *pt )
{
    return ((pt->x >= rect->left) && (pt->x < rect->right) &&
            (pt->y >= rect->top) && (pt->y < rect->bottom));
}

/**********************************************************************
 *                  get_octant_number
 *
 * Return the octant number starting clockwise from the +ve x-axis.
 */
static inline int get_octant_number(int dx, int dy)
{
    if(dy > 0)
        if(dx > 0)
            return ( dx >  dy) ? 1 : 2;
        else
            return (-dx >  dy) ? 4 : 3;
    else
        if(dx < 0)
            return (-dx > -dy) ? 5 : 6;
        else
            return ( dx > -dy) ? 8 : 7;
}

static inline DWORD get_octant_mask(int dx, int dy)
{
    return 1 << (get_octant_number(dx, dy) - 1);
}

static void solid_pen_line_callback(INT x, INT y, LPARAM lparam)
{
    dibdrv_physdev *pdev = (dibdrv_physdev *)lparam;
    RECT rect;

    rect.left   = x;
    rect.right  = x + 1;
    rect.top    = y;
    rect.bottom = y + 1;
    pdev->dib.funcs->solid_rects(&pdev->dib, 1, &rect, pdev->pen_and, pdev->pen_xor);
    return;
}

static void bres_line_with_bias(INT x1, INT y1, INT x2, INT y2, void (* callback)(INT,INT,LPARAM), LPARAM lParam)
{
    INT xadd = 1, yadd = 1;
    INT err, erradd;
    INT cnt;
    INT dx = x2 - x1;
    INT dy = y2 - y1;
    DWORD octant = get_octant_mask(dx, dy);
    INT bias = 0;

    /* Octants 3, 5, 6 and 8 take a bias */
    if(octant & 0xb4) bias = 1;

    if (dx < 0)
    {
        dx = -dx;
        xadd = -1;
    }
    if (dy < 0)
    {
        dy = -dy;
        yadd = -1;
    }
    if (dx > dy)  /* line is "more horizontal" */
    {
        err = 2*dy - dx; erradd = 2*dy - 2*dx;
        for(cnt = 0; cnt < dx; cnt++)
        {
            callback(x1, y1, lParam);
            if (err + bias > 0)
            {
                y1 += yadd;
                err += erradd;
            }
            else err += 2*dy;
            x1 += xadd;
        }
    }
    else   /* line is "more vertical" */
    {
        err = 2*dx - dy; erradd = 2*dx - 2*dy;
        for(cnt = 0; cnt < dy; cnt++)
        {
            callback(x1, y1, lParam);
            if (err + bias > 0)
            {
                x1 += xadd;
                err += erradd;
            }
            else err += 2*dx;
            y1 += yadd;
        }
    }
}

static BOOL solid_pen_line(dibdrv_physdev *pdev, POINT *start, POINT *end)
{
    const WINEREGION *clip = get_wine_region(pdev->clip);
    BOOL ret = TRUE;

    if(start->y == end->y)
    {
        RECT rect;
        int i;

        rect.left   = start->x;
        rect.top    = start->y;
        rect.right  = end->x;
        rect.bottom = end->y + 1;
        order_end_points(&rect.left, &rect.right);
        for(i = 0; i < clip->numRects; i++)
        {
            if(clip->rects[i].top >= rect.bottom) break;
            if(clip->rects[i].bottom <= rect.top) continue;
            /* Optimize the unclipped case */
            if(clip->rects[i].left <= rect.left && clip->rects[i].right >= rect.right)
            {
                pdev->dib.funcs->solid_rects(&pdev->dib, 1, &rect, pdev->pen_and, pdev->pen_xor);
                break;
            }
            if(clip->rects[i].right > rect.left && clip->rects[i].left < rect.right)
            {
                RECT tmp = rect;
                tmp.left = max(rect.left, clip->rects[i].left);
                tmp.right = min(rect.right, clip->rects[i].right);
                pdev->dib.funcs->solid_rects(&pdev->dib, 1, &tmp, pdev->pen_and, pdev->pen_xor);
            }
        }
    }
    else if(start->x == end->x)
    {
        RECT rect;
        int i;

        rect.left   = start->x;
        rect.top    = start->y;
        rect.right  = end->x + 1;
        rect.bottom = end->y;
        order_end_points(&rect.top, &rect.bottom);
        for(i = 0; i < clip->numRects; i++)
        {
            /* Optimize unclipped case */
            if(clip->rects[i].top <= rect.top && clip->rects[i].bottom >= rect.bottom &&
               clip->rects[i].left <= rect.left && clip->rects[i].right >= rect.right)
            {
                pdev->dib.funcs->solid_rects(&pdev->dib, 1, &rect, pdev->pen_and, pdev->pen_xor);
                break;
            }
            if(clip->rects[i].top >= rect.bottom) break;
            if(clip->rects[i].bottom <= rect.top) continue;
            if(clip->rects[i].right > rect.left && clip->rects[i].left < rect.right)
            {
                RECT tmp = rect;
                tmp.top = max(rect.top, clip->rects[i].top);
                tmp.bottom = min(rect.bottom, clip->rects[i].bottom);
                pdev->dib.funcs->solid_rects(&pdev->dib, 1, &tmp, pdev->pen_and, pdev->pen_xor);
            }
        }
    }
    else
    {
        if(clip->numRects == 1 && pt_in_rect(&clip->extents, start) && pt_in_rect(&clip->extents, end))
            /* FIXME: Optimize by moving Bresenham algorithm to the primitive functions,
               or at least cache adjacent points in the callback */
            bres_line_with_bias(start->x, start->y, end->x, end->y, solid_pen_line_callback, (LPARAM)pdev);
        else if(clip->numRects >= 1)
            ret = FALSE;
    }
    release_wine_region(pdev->clip);
    return ret;
}

/***********************************************************************
 *           dibdrv_SelectPen
 */
HPEN CDECL dibdrv_SelectPen( PHYSDEV dev, HPEN hpen )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectPen );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    LOGPEN logpen;

    TRACE("(%p, %p)\n", dev, hpen);

    if (!GetObjectW( hpen, sizeof(logpen), &logpen ))
    {
        /* must be an extended pen */
        EXTLOGPEN *elp;
        INT size = GetObjectW( hpen, 0, NULL );

        if (!size) return 0;

        elp = HeapAlloc( GetProcessHeap(), 0, size );

        GetObjectW( hpen, size, elp );
        /* FIXME: add support for user style pens */
        logpen.lopnStyle = elp->elpPenStyle;
        logpen.lopnWidth.x = elp->elpWidth;
        logpen.lopnWidth.y = 0;
        logpen.lopnColor = elp->elpColor;

        HeapFree( GetProcessHeap(), 0, elp );
    }

    if (hpen == GetStockObject( DC_PEN ))
        logpen.lopnColor = GetDCPenColor( dev->hdc );

    pdev->pen_color = pdev->dib.funcs->colorref_to_pixel(&pdev->dib, logpen.lopnColor);
    calc_and_xor_masks(GetROP2(dev->hdc), pdev->pen_color, &pdev->pen_and, &pdev->pen_xor);

    pdev->defer |= DEFER_PEN;

    switch(logpen.lopnStyle & PS_STYLE_MASK)
    {
    case PS_SOLID:
        if(logpen.lopnStyle & PS_GEOMETRIC) break;
        if(logpen.lopnWidth.x > 1) break;
        pdev->pen_line = solid_pen_line;
        pdev->defer &= ~DEFER_PEN;
        break;
    default:
        break;
    }

    return next->funcs->pSelectPen( next, hpen );
}

/***********************************************************************
 *           dibdrv_SetDCPenColor
 */
COLORREF CDECL dibdrv_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetDCPenColor );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);

    if (GetCurrentObject(dev->hdc, OBJ_PEN) == GetStockObject( DC_PEN ))
    {
        pdev->pen_color = pdev->dib.funcs->colorref_to_pixel(&pdev->dib, color);
        calc_and_xor_masks(GetROP2(dev->hdc), pdev->pen_color, &pdev->pen_and, &pdev->pen_xor);
    }

    return next->funcs->pSetDCPenColor( next, color );
}

/**********************************************************************
 *             solid_brush
 *
 * Fill a number of rectangles with the solid brush
 * FIXME: Should we insist l < r && t < b?  Currently we assume this.
 */
static BOOL solid_brush(dibdrv_physdev *pdev, int num, RECT *rects)
{
    int i, j;
    const WINEREGION *clip = get_wine_region(pdev->clip);

    for(i = 0; i < num; i++)
    {
        for(j = 0; j < clip->numRects; j++)
        {
            RECT rect = rects[i];

            /* Optimize unclipped case */
            if(clip->rects[j].top <= rect.top && clip->rects[j].bottom >= rect.bottom &&
               clip->rects[j].left <= rect.left && clip->rects[j].right >= rect.right)
            {
                pdev->dib.funcs->solid_rects(&pdev->dib, 1, &rect, pdev->brush_and, pdev->brush_xor);
                break;
            }

            if(clip->rects[j].top >= rect.bottom) break;
            if(clip->rects[j].bottom <= rect.top) continue;

            if(clip->rects[j].right > rect.left && clip->rects[j].left < rect.right)
            {
                rect.left   = max(rect.left,   clip->rects[j].left);
                rect.top    = max(rect.top,    clip->rects[j].top);
                rect.right  = min(rect.right,  clip->rects[j].right);
                rect.bottom = min(rect.bottom, clip->rects[j].bottom);

                pdev->dib.funcs->solid_rects(&pdev->dib, 1, &rect, pdev->brush_and, pdev->brush_xor);
            }
        }
    }
    release_wine_region(pdev->clip);
    return TRUE;
}

void update_brush_rop( dibdrv_physdev *pdev, INT rop )
{
    if(pdev->brush_style == BS_SOLID)
        calc_and_xor_masks(rop, pdev->brush_color, &pdev->brush_and, &pdev->brush_xor);
}

/***********************************************************************
 *           dibdrv_SelectBrush
 */
HBRUSH CDECL dibdrv_SelectBrush( PHYSDEV dev, HBRUSH hbrush )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectBrush );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    LOGBRUSH logbrush;

    TRACE("(%p, %p)\n", dev, hbrush);

    if (!GetObjectW( hbrush, sizeof(logbrush), &logbrush )) return 0;

    if (hbrush == GetStockObject( DC_BRUSH ))
        logbrush.lbColor = GetDCBrushColor( dev->hdc );

    pdev->brush_style = logbrush.lbStyle;

    pdev->defer |= DEFER_BRUSH;

    switch(logbrush.lbStyle)
    {
    case BS_SOLID:
        pdev->brush_color = pdev->dib.funcs->colorref_to_pixel(&pdev->dib, logbrush.lbColor);
        calc_and_xor_masks(GetROP2(dev->hdc), pdev->brush_color, &pdev->brush_and, &pdev->brush_xor);
        pdev->brush_rects = solid_brush;
        pdev->defer &= ~DEFER_BRUSH;
        break;
    default:
        break;
    }

    return next->funcs->pSelectBrush( next, hbrush );
}

/***********************************************************************
 *           dibdrv_SetDCBrushColor
 */
COLORREF CDECL dibdrv_SetDCBrushColor( PHYSDEV dev, COLORREF color )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetDCBrushColor );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);

    if (GetCurrentObject(dev->hdc, OBJ_BRUSH) == GetStockObject( DC_BRUSH ))
    {
        pdev->brush_color = pdev->dib.funcs->colorref_to_pixel(&pdev->dib, color);
        calc_and_xor_masks(GetROP2(dev->hdc), pdev->brush_color, &pdev->brush_and, &pdev->brush_xor);
    }

    return next->funcs->pSetDCBrushColor( next, color );
}
