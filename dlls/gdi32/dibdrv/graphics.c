/*
 * DIB driver graphics operations.
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

static RECT get_device_rect( HDC hdc, int left, int top, int right, int bottom, BOOL rtl_correction )
{
    RECT rect;

    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;
    if (rtl_correction && GetLayout( hdc ) & LAYOUT_RTL)
    {
        /* shift the rectangle so that the right border is included after mirroring */
        /* it would be more correct to do this after LPtoDP but that's not what Windows does */
        rect.left--;
        rect.right--;
    }
    LPtoDP( hdc, (POINT *)&rect, 2 );
    if (rect.left > rect.right)
    {
        int tmp = rect.left;
        rect.left = rect.right;
        rect.right = tmp;
    }
    if (rect.top > rect.bottom)
    {
        int tmp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = tmp;
    }
    return rect;
}

/***********************************************************************
 *           dibdrv_LineTo
 */
BOOL dibdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pLineTo );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    POINT pts[2];

    GetCurrentPositionEx(dev->hdc, pts);
    pts[1].x = x;
    pts[1].y = y;

    LPtoDP(dev->hdc, pts, 2);

    reset_dash_origin(pdev);

    if(defer_pen(pdev) || !pdev->pen_lines(pdev, 2, pts))
        return next->funcs->pLineTo( next, x, y );

    return TRUE;
}

/***********************************************************************
 *           get_rop2_from_rop
 *
 * Returns the binary rop that is equivalent to the provided ternary rop
 * if the src bits are ignored.
 */
static inline INT get_rop2_from_rop(INT rop)
{
    return (((rop >> 18) & 0x0c) | ((rop >> 16) & 0x03)) + 1;
}

/***********************************************************************
 *           dibdrv_PatBlt
 */
BOOL dibdrv_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPatBlt );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    INT rop2 = get_rop2_from_rop(rop);
    BOOL done;

    TRACE("(%p, %d, %d, %d, %d, %06x)\n", dev, dst->x, dst->y, dst->width, dst->height, rop);

    if(defer_brush(pdev))
        return next->funcs->pPatBlt( next, dst, rop );

    update_brush_rop( pdev, rop2 );

    done = brush_rects( pdev, 1, &dst->visrect );

    update_brush_rop( pdev, GetROP2(dev->hdc) );

    if(!done)
        return next->funcs->pPatBlt( next, dst, rop );

    return TRUE;
}

/***********************************************************************
 *           dibdrv_PaintRgn
 */
BOOL dibdrv_PaintRgn( PHYSDEV dev, HRGN rgn )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPaintRgn );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    const WINEREGION *region;
    int i;
    RECT rect;

    TRACE("%p, %p\n", dev, rgn);

    if(defer_brush(pdev)) return next->funcs->pPaintRgn( next, rgn );

    region = get_wine_region( rgn );
    if(!region) return FALSE;

    for(i = 0; i < region->numRects; i++)
    {
        rect = get_device_rect( dev->hdc, region->rects[i].left, region->rects[i].top,
                                region->rects[i].right, region->rects[i].bottom, FALSE );
        brush_rects( pdev, 1, &rect );
    }

    release_wine_region( rgn );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_PolyPolyline
 */
BOOL dibdrv_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyPolyline );
    DWORD max_points = 0, i;
    POINT *points;

    if (defer_pen( pdev )) return next->funcs->pPolyPolyline( next, pt, counts, polylines );

    for (i = 0; i < polylines; i++) max_points = max( counts[i], max_points );

    points = HeapAlloc( GetProcessHeap(), 0, max_points * sizeof(*pt) );
    if (!points) return FALSE;

    for (i = 0; i < polylines; i++)
    {
        memcpy( points, pt, counts[i] * sizeof(*pt) );
        pt += counts[i];
        LPtoDP( dev->hdc, points, counts[i] );

        reset_dash_origin( pdev );
        pdev->pen_lines( pdev, counts[i], points );
    }

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_Polyline
 */
BOOL dibdrv_Polyline( PHYSDEV dev, const POINT* pt, INT count )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyline );
    POINT *points;

    if (defer_pen( pdev )) return next->funcs->pPolyline( next, pt, count );

    points = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*pt) );
    if (!points) return FALSE;

    memcpy( points, pt, count * sizeof(*pt) );
    LPtoDP( dev->hdc, points, count );

    reset_dash_origin( pdev );
    pdev->pen_lines( pdev, count, points );

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_Rectangle
 */
BOOL dibdrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pRectangle );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    RECT rect = get_device_rect( dev->hdc, left, top, right, bottom, TRUE );
    POINT pts[5];

    TRACE("(%p, %d, %d, %d, %d)\n", dev, left, top, right, bottom);

    if(rect.left == rect.right || rect.top == rect.bottom) return TRUE;

    if(defer_pen(pdev) || defer_brush(pdev))
        return next->funcs->pRectangle( next, left, top, right, bottom );

    reset_dash_origin(pdev);

    /* 4 pts going anti-clockwise starting from top-right */
    pts[0].x = pts[3].x = rect.right - 1;
    pts[0].y = pts[1].y = rect.top;
    pts[1].x = pts[2].x = rect.left;
    pts[2].y = pts[3].y = rect.bottom - 1;
    pts[4] = pts[0];

    pdev->pen_lines(pdev, 5, pts);

    /* FIXME: Will need updating when we support wide pens */

    rect.left   += 1;
    rect.top    += 1;
    rect.right  -= 1;
    rect.bottom -= 1;

    brush_rects(pdev, 1, &rect);

    return TRUE;
}

/***********************************************************************
 *           dibdrv_SetPixel
 */
COLORREF dibdrv_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    int i;
    POINT pt;
    DWORD pixel;
    const WINEREGION *clip = get_wine_region( pdev->clip );

    TRACE( "(%p, %d, %d, %08x)\n", dev, x, y, color );

    pt.x = x;
    pt.y = y;
    LPtoDP( dev->hdc, &pt, 1 );

    /* SetPixel doesn't do the 1bpp massaging like other fg colors */
    pixel = get_pixel_color( pdev, color, FALSE );
    color = pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );

    for (i = 0; i < clip->numRects; i++)
    {
        if (pt_in_rect( clip->rects + i, pt ))
        {
            RECT rect;
            rect.left = pt.x;
            rect.top =  pt.y;
            rect.right = rect.left + 1;
            rect.bottom = rect.top + 1;

            pdev->dib.funcs->solid_rects( &pdev->dib, 1, &rect, 0, pixel );
            break;
        }
    }

    release_wine_region( pdev->clip );
    return color;
}
