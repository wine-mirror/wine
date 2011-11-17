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

/* Intensities of the 17 glyph levels when drawn with text component of 0xff on a
   black bkgnd.  [A log-log plot of these data gives: y = 77.05 * x^0.4315]. */
static const BYTE ramp[17] =
{
    0,    0x4d, 0x68, 0x7c,
    0x8c, 0x9a, 0xa7, 0xb2,
    0xbd, 0xc7, 0xd0, 0xd9,
    0xe1, 0xe9, 0xf0, 0xf8,
    0xff
};

/* For a give text-color component and a glyph level, calculate the
   range of dst intensities, the min/max corresponding to 0/0xff bkgnd
   components respectively.

   The minimum is a linear interpolation between 0 and the value in
   the ramp table.

   The maximum is a linear interpolation between the value from the
   ramp table read in reverse and 0xff.

   To find the resulting pixel intensity, we note that if the text and
   bkgnd intensities are the same then the result must be that
   intensity.  Otherwise we linearly interpolate between either the
   min or the max value and this intermediate value depending on which
   side of the inequality we lie.
*/

static inline void get_range( BYTE aa, DWORD text_comp, BYTE *min_comp, BYTE *max_comp )
{
    *min_comp = (ramp[aa] * text_comp) / 0xff;
    *max_comp = ramp[16 - aa] + ((0xff - ramp[16 - aa]) * text_comp) / 0xff;
}

void update_aa_ranges( dibdrv_physdev *pdev )
{
    int i;
    COLORREF text = pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pdev->text_color );

    for (i = 0; i < 17; i++)
    {
        get_range( i, GetRValue(text), &pdev->glyph_intensities[i].r_min, &pdev->glyph_intensities[i].r_max );
        get_range( i, GetGValue(text), &pdev->glyph_intensities[i].g_min, &pdev->glyph_intensities[i].g_max );
        get_range( i, GetBValue(text), &pdev->glyph_intensities[i].b_min, &pdev->glyph_intensities[i].b_max );
    }
}

/**********************************************************************
 *                 get_text_bkgnd_masks
 *
 * See the comment above get_pen_bkgnd_masks
 */
static inline void get_text_bkgnd_masks( const dibdrv_physdev *pdev, rop_mask *mask )
{
    mask->and = 0;

    if (pdev->dib.bit_count != 1)
        mask->xor = pdev->bkgnd_color;
    else
    {
        mask->xor = ~pdev->text_color;
        if (GetTextColor( pdev->dev.hdc ) == GetBkColor( pdev->dev.hdc ))
            mask->xor = pdev->text_color;
    }
}

static void draw_glyph( dibdrv_physdev *pdev, const POINT *origin, const GLYPHMETRICS *metrics,
                        const struct gdi_image_bits *image )
{
    const WINEREGION *clip = get_wine_region( pdev->clip );
    int i;
    RECT rect, clipped_rect;
    POINT src_origin;
    dib_info glyph_dib;

    glyph_dib.bit_count = 8;
    glyph_dib.width     = metrics->gmBlackBoxX;
    glyph_dib.height    = metrics->gmBlackBoxY;
    glyph_dib.stride    = get_dib_stride( metrics->gmBlackBoxX, 8 );
    glyph_dib.bits      = *image;

    rect.left   = origin->x  + metrics->gmptGlyphOrigin.x;
    rect.top    = origin->y  - metrics->gmptGlyphOrigin.y;
    rect.right  = rect.left  + metrics->gmBlackBoxX;
    rect.bottom = rect.top   + metrics->gmBlackBoxY;

    for (i = 0; i < clip->numRects; i++)
    {
        if (intersect_rect( &clipped_rect, &rect, clip->rects + i ))
        {
            src_origin.x = clipped_rect.left - rect.left;
            src_origin.y = clipped_rect.top  - rect.top;

            pdev->dib.funcs->draw_glyph( &pdev->dib, &clipped_rect, &glyph_dib, &src_origin,
                                         pdev->text_color, pdev->glyph_intensities );
        }
    }

    release_wine_region( pdev->clip );
}

static const BYTE masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static const int padding[4] = {0, 3, 2, 1};

/***********************************************************************
 *         get_glyph_bitmap
 *
 * Retrieve a 17-level bitmap for the appropiate glyph.
 *
 * For non-antialiased bitmaps convert them to the 17-level format
 * using only values 0 or 16.
 */
static DWORD get_glyph_bitmap( dibdrv_physdev *pdev, UINT index, UINT aa_flags, GLYPHMETRICS *metrics,
                               struct gdi_image_bits *image )
{
    UINT ggo_flags = aa_flags | GGO_GLYPH_INDEX;
    static const MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };
    UINT indices[3] = {0, 0, 0x20};
    int i, x, y;
    DWORD ret, size;
    BYTE *buf, *dst, *src;
    int pad, stride;

    image->ptr = NULL;
    image->is_copy = FALSE;
    image->free = free_heap_bits;
    image->param = NULL;

    indices[0] = index;

    for (i = 0; i < sizeof(indices) / sizeof(indices[0]); index = indices[++i])
    {
        ret = GetGlyphOutlineW( pdev->dev.hdc, index, ggo_flags, metrics, 0, NULL, &identity );
        if (ret != GDI_ERROR) break;
    }

    if (ret == GDI_ERROR) return ERROR_NOT_FOUND;
    if (!ret) return ERROR_SUCCESS; /* empty glyph */

    /* We'll convert non-antialiased 1-bpp bitmaps to 8-bpp, so these sizes relate to 8-bpp */
    pad = padding[ metrics->gmBlackBoxX % 4 ];
    stride = get_dib_stride( metrics->gmBlackBoxX, 8 );
    size = metrics->gmBlackBoxY * stride;

    buf = HeapAlloc( GetProcessHeap(), 0, size );
    if (!buf) return ERROR_OUTOFMEMORY;

    ret = GetGlyphOutlineW( pdev->dev.hdc, index, ggo_flags, metrics, size, buf, &identity );
    if (ret == GDI_ERROR)
    {
        HeapFree( GetProcessHeap(), 0, buf );
        return ERROR_NOT_FOUND;
    }

    if (aa_flags == GGO_BITMAP)
    {
        for (y = metrics->gmBlackBoxY - 1; y >= 0; y--)
        {
            src = buf + y * get_dib_stride( metrics->gmBlackBoxX, 1 );
            dst = buf + y * stride;

            if (pad) memset( dst + metrics->gmBlackBoxX, 0, pad );

            for (x = metrics->gmBlackBoxX - 1; x >= 0; x--)
                dst[x] = (src[x / 8] & masks[x % 8]) ? 0x10 : 0;
        }
    }
    else if (pad)
    {
        for (y = 0, dst = buf; y < metrics->gmBlackBoxY; y++, dst += stride)
            memset( dst + metrics->gmBlackBoxX, 0, pad );
    }

    image->ptr = buf;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           dibdrv_ExtTextOut
 */
BOOL dibdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                        const RECT *rect, LPCWSTR str, UINT count, const INT *dx )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    UINT aa_flags, i;
    POINT origin;
    DWORD err;
    HRGN saved_clip = NULL;

    if (flags & ETO_OPAQUE)
    {
        rop_mask bkgnd_color;
        get_text_bkgnd_masks( pdev, &bkgnd_color );
        solid_rects( &pdev->dib, 1, rect, &bkgnd_color, pdev->clip );
    }

    if (count == 0) return TRUE;

    if (flags & ETO_CLIPPED)
    {
        HRGN clip = CreateRectRgnIndirect( rect );
        saved_clip = add_extra_clipping_region( pdev, clip );
        DeleteObject( clip );
    }

    aa_flags = get_font_aa_flags( dev->hdc );
    origin.x = x;
    origin.y = y;
    for (i = 0; i < count; i++)
    {
        GLYPHMETRICS metrics;
        struct gdi_image_bits image;

        err = get_glyph_bitmap( pdev, (UINT)str[i], aa_flags, &metrics, &image );
        if (err) continue;

        if (image.ptr) draw_glyph( pdev, &origin, &metrics, &image );
        if (image.free) image.free( &image );

        if (dx)
        {
            if (flags & ETO_PDY)
            {
                origin.x += dx[ i * 2 ];
                origin.y += dx[ i * 2 + 1];
            }
            else
                origin.x += dx[ i ];
        }
        else
        {
            origin.x += metrics.gmCellIncX;
            origin.y += metrics.gmCellIncY;
        }
    }

    restore_clipping_region( pdev, saved_clip );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_GetPixel
 */
COLORREF dibdrv_GetPixel( PHYSDEV dev, INT x, INT y )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    POINT pt;
    DWORD pixel;

    TRACE( "(%p, %d, %d)\n", dev, x, y );

    pt.x = x;
    pt.y = y;
    LPtoDP( dev->hdc, &pt, 1 );

    if (pt.x < 0 || pt.x >= pdev->dib.width ||
        pt.y < 0 || pt.y >= pdev->dib.height)
        return CLR_INVALID;

    pixel = pdev->dib.funcs->get_pixel( &pdev->dib, &pt );
    return pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );
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
