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

#include <assert.h>
#include "gdi_private.h"
#include "dibdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

static BOOL brush_rect( dibdrv_physdev *pdev, dib_brush *brush, const RECT *rect, HRGN clip )
{
    struct clipped_rects clipped_rects;
    BOOL ret;

    if (!get_clipped_rects( &pdev->dib, rect, clip, &clipped_rects )) return TRUE;
    ret = brush->rects( pdev, brush, &pdev->dib, clipped_rects.count, clipped_rects.rects,
                        GetROP2( pdev->dev.hdc ));
    free_clipped_rects( &clipped_rects );
    return ret;
}

/* paint a region with the brush (note: the region can be modified) */
static BOOL brush_region( dibdrv_physdev *pdev, HRGN region )
{
    if (pdev->clip) CombineRgn( region, region, pdev->clip, RGN_AND );
    return brush_rect( pdev, &pdev->brush, NULL, region );
}

/* paint a region with the pen (note: the region can be modified) */
static BOOL pen_region( dibdrv_physdev *pdev, HRGN region )
{
    if (pdev->clip) CombineRgn( region, region, pdev->clip, RGN_AND );
    return brush_rect( pdev, &pdev->pen_brush, NULL, region );
}

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

static BOOL get_pen_device_rect( dibdrv_physdev *dev, RECT *rect, int left, int top, int right, int bottom )
{
    *rect = get_device_rect( dev->dev.hdc, left, top, right, bottom, TRUE );
    if (rect->left == rect->right || rect->top == rect->bottom) return FALSE;

    if (dev->pen_style == PS_INSIDEFRAME)
    {
        rect->left   += dev->pen_width / 2;
        rect->top    += dev->pen_width / 2;
        rect->right  -= (dev->pen_width - 1) / 2;
        rect->bottom -= (dev->pen_width - 1) / 2;
    }
    return TRUE;
}

static void add_pen_lines_bounds( dibdrv_physdev *dev, int count, const POINT *points, HRGN rgn )
{
    const WINEREGION *region;
    RECT bounds, rect;
    int width = 0;

    if (!dev->bounds) return;
    reset_bounds( &bounds );

    if (dev->pen_uses_region)
    {
        /* Windows uses some heuristics to estimate the distance from the point that will be painted */
        width = dev->pen_width + 2;
        if (dev->pen_join == PS_JOIN_MITER)
        {
            width *= 5;
            if (dev->pen_endcap == PS_ENDCAP_SQUARE) width = (width * 3 + 1) / 2;
        }
        else
        {
            if (dev->pen_endcap == PS_ENDCAP_SQUARE) width -= width / 4;
            else width = (width + 1) / 2;
        }

        /* in case the heuristics are wrong, add the actual region too */
        if ((region = get_wine_region( rgn )))
        {
            add_bounds_rect( &bounds, &region->extents );
            release_wine_region( rgn );
        }
    }

    while (count-- > 0)
    {
        rect.left   = points->x - width;
        rect.top    = points->y - width;
        rect.right  = points->x + width + 1;
        rect.bottom = points->y + width + 1;
        add_bounds_rect( &bounds, &rect );
        points++;
    }

    add_clipped_bounds( dev, &bounds, dev->clip );
}

/* compute the points for the first quadrant of an ellipse, counterclockwise from the x axis */
/* 'data' must contain enough space, (width+height)/2 is a reasonable upper bound */
static int ellipse_first_quadrant( int width, int height, POINT *data )
{
    const int a = width - 1;
    const int b = height - 1;
    const INT64 asq = (INT64)8 * a * a;
    const INT64 bsq = (INT64)8 * b * b;
    INT64 dx  = (INT64)4 * b * b * (1 - a);
    INT64 dy  = (INT64)4 * a * a * (1 + (b % 2));
    INT64 err = dx + dy + a * a * (b % 2);
    int pos = 0;
    POINT pt;

    pt.x = a;
    pt.y = height / 2;

    /* based on an algorithm by Alois Zingl */

    while (pt.x >= width / 2)
    {
        INT64 e2 = 2 * err;
        data[pos++] = pt;
        if (e2 >= dx)
        {
            pt.x--;
            err += dx += bsq;
        }
        if (e2 <= dy)
        {
            pt.y++;
            err += dy += asq;
        }
    }
    return pos;
}

static int find_intersection( const POINT *points, int x, int y, int count )
{
    int i;

    if (y >= 0)
    {
        if (x >= 0)  /* first quadrant */
        {
            for (i = 0; i < count; i++) if (points[i].x * y <= points[i].y * x) break;
            return i;
        }
        /* second quadrant */
        for (i = 0; i < count; i++) if (points[i].x * y < points[i].y * -x) break;
        return 2 * count - i;
    }
    if (x >= 0)  /* fourth quadrant */
    {
        for (i = 0; i < count; i++) if (points[i].x * -y <= points[i].y * x) break;
        return 4 * count - i;
    }
    /* third quadrant */
    for (i = 0; i < count; i++) if (points[i].x * -y < points[i].y * -x) break;
    return 2 * count + i;
}

static int get_arc_points( PHYSDEV dev, const RECT *rect, POINT start, POINT end, POINT *points )
{
    int i, pos, count, start_pos, end_pos;
    int width = rect->right - rect->left;
    int height = rect->bottom - rect->top;

    count = ellipse_first_quadrant( width, height, points );
    for (i = 0; i < count; i++)
    {
        points[i].x -= width / 2;
        points[i].y -= height / 2;
    }
    if (GetArcDirection( dev->hdc ) != AD_CLOCKWISE)
    {
        start.y = -start.y;
        end.y = -end.y;
    }
    start_pos = find_intersection( points, start.x, start.y, count );
    end_pos = find_intersection( points, end.x, end.y, count );
    if (end_pos <= start_pos) end_pos += 4 * count;

    pos = count;
    if (GetArcDirection( dev->hdc ) == AD_CLOCKWISE)
    {
        for (i = start_pos; i < end_pos; i++, pos++)
        {
            switch ((i / count) % 4)
            {
            case 0:
                points[pos].x = rect->left + width/2 + points[i % count].x;
                points[pos].y = rect->top + height/2 + points[i % count].y;
                break;
            case 1:
                points[pos].x = rect->left + width/2 - points[count - 1 - i % count].x;
                points[pos].y = rect->top + height/2 + points[count - 1 - i % count].y;
                break;
            case 2:
                points[pos].x = rect->left + width/2 - points[i % count].x;
                points[pos].y = rect->top + height/2 - points[i % count].y;
                break;
            case 3:
                points[pos].x = rect->left + width/2 + points[count - 1 - i % count].x;
                points[pos].y = rect->top + height/2 - points[count - 1 - i % count].y;
                break;
            }
        }
    }
    else
    {
        for (i = start_pos; i < end_pos; i++, pos++)
        {
            switch ((i / count) % 4)
            {
            case 0:
                points[pos].x = rect->left + width/2 + points[i % count].x;
                points[pos].y = rect->top + height/2 - points[i % count].y;
                break;
            case 1:
                points[pos].x = rect->left + width/2 - points[count - 1 - i % count].x;
                points[pos].y = rect->top + height/2 - points[count - 1 - i % count].y;
                break;
            case 2:
                points[pos].x = rect->left + width/2 - points[i % count].x;
                points[pos].y = rect->top + height/2 + points[i % count].y;
                break;
            case 3:
                points[pos].x = rect->left + width/2 + points[count - 1 - i % count].x;
                points[pos].y = rect->top + height/2 + points[count - 1 - i % count].y;
                break;
            }
        }
    }

    memmove( points, points + count, (pos - count) * sizeof(POINT) );
    return pos - count;
}

/* backend for arc functions; extra_lines is -1 for ArcTo, 0 for Arc, 1 for Chord, 2 for Pie */
static BOOL draw_arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                      INT start_x, INT start_y, INT end_x, INT end_y, INT extra_lines )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    RECT rect;
    POINT pt[2], *points;
    int width, height, count;
    BOOL ret = TRUE;
    HRGN outline = 0, interior = 0;

    if (!get_pen_device_rect( pdev, &rect, left, top, right, bottom )) return TRUE;

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    pt[0].x = start_x;
    pt[0].y = start_y;
    pt[1].x = end_x;
    pt[1].y = end_y;
    LPtoDP( dev->hdc, pt, 2 );
    /* make them relative to the ellipse center */
    pt[0].x -= left + width / 2;
    pt[0].y -= top + height / 2;
    pt[1].x -= left + width / 2;
    pt[1].y -= top + height / 2;

    points = HeapAlloc( GetProcessHeap(), 0, (width + height) * 3 * sizeof(*points) );
    if (!points) return FALSE;

    if (extra_lines == -1)
    {
        GetCurrentPositionEx( dev->hdc, points );
        LPtoDP( dev->hdc, points, 1 );
        count = 1 + get_arc_points( dev, &rect, pt[0], pt[1], points + 1 );
    }
    else count = get_arc_points( dev, &rect, pt[0], pt[1], points );

    if (extra_lines == 2)
    {
        points[count].x = rect.left + width / 2;
        points[count].y = rect.top + height / 2;
        count++;
    }
    if (count < 2)
    {
        HeapFree( GetProcessHeap(), 0, points );
        return TRUE;
    }

    if (pdev->pen_uses_region && !(outline = CreateRectRgn( 0, 0, 0, 0 )))
    {
        HeapFree( GetProcessHeap(), 0, points );
        return FALSE;
    }

    if (pdev->brush.style != BS_NULL && extra_lines > 0 &&
        !(interior = CreatePolygonRgn( points, count, WINDING )))
    {
        HeapFree( GetProcessHeap(), 0, points );
        if (outline) DeleteObject( outline );
        return FALSE;
    }

    /* if not using a region, paint the interior first so the outline can overlap it */
    if (interior && !outline)
    {
        ret = brush_region( pdev, interior );
        DeleteObject( interior );
        interior = 0;
    }

    reset_dash_origin( pdev );
    pdev->pen_lines( pdev, count, points, extra_lines > 0, outline );
    add_pen_lines_bounds( pdev, count, points, outline );

    if (interior)
    {
        CombineRgn( interior, interior, outline, RGN_DIFF );
        ret = brush_region( pdev, interior );
        DeleteObject( interior );
    }
    if (outline)
    {
        if (ret) ret = pen_region( pdev, outline );
        DeleteObject( outline );
    }
    HeapFree( GetProcessHeap(), 0, points );
    return ret;
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

static inline void get_aa_ranges( COLORREF col, struct intensity_range intensities[17] )
{
    int i;

    for (i = 0; i < 17; i++)
    {
        get_range( i, GetRValue(col), &intensities[i].r_min, &intensities[i].r_max );
        get_range( i, GetGValue(col), &intensities[i].g_min, &intensities[i].g_max );
        get_range( i, GetBValue(col), &intensities[i].b_min, &intensities[i].b_max );
    }
}

/**********************************************************************
 *                 get_text_bkgnd_masks
 *
 * See the comment above get_pen_bkgnd_masks
 */
static inline void get_text_bkgnd_masks( dibdrv_physdev *pdev, rop_mask *mask )
{
    COLORREF bg = GetBkColor( pdev->dev.hdc );

    mask->and = 0;

    if (pdev->dib.bit_count != 1)
        mask->xor = get_pixel_color( pdev, bg, FALSE );
    else
    {
        COLORREF fg = GetTextColor( pdev->dev.hdc );
        mask->xor = get_pixel_color( pdev, fg, TRUE );
        if (fg != bg) mask->xor = ~mask->xor;
    }
}

static void draw_glyph( dibdrv_physdev *pdev, const POINT *origin, const GLYPHMETRICS *metrics,
                        const struct gdi_image_bits *image, DWORD text_color,
                        const struct intensity_range *ranges, const struct clipped_rects *clipped_rects,
                        RECT *bounds )
{
    int i;
    RECT rect, clipped_rect;
    POINT src_origin;
    dib_info glyph_dib;

    glyph_dib.bit_count   = 8;
    glyph_dib.width       = metrics->gmBlackBoxX;
    glyph_dib.height      = metrics->gmBlackBoxY;
    glyph_dib.rect.left   = 0;
    glyph_dib.rect.top    = 0;
    glyph_dib.rect.right  = metrics->gmBlackBoxX;
    glyph_dib.rect.bottom = metrics->gmBlackBoxY;
    glyph_dib.stride      = get_dib_stride( metrics->gmBlackBoxX, 8 );
    glyph_dib.bits        = *image;

    rect.left   = origin->x  + metrics->gmptGlyphOrigin.x;
    rect.top    = origin->y  - metrics->gmptGlyphOrigin.y;
    rect.right  = rect.left  + metrics->gmBlackBoxX;
    rect.bottom = rect.top   + metrics->gmBlackBoxY;
    add_bounds_rect( bounds, &rect );

    for (i = 0; i < clipped_rects->count; i++)
    {
        if (intersect_rect( &clipped_rect, &rect, clipped_rects->rects + i ))
        {
            src_origin.x = clipped_rect.left - rect.left;
            src_origin.y = clipped_rect.top  - rect.top;

            pdev->dib.funcs->draw_glyph( &pdev->dib, &clipped_rect, &glyph_dib, &src_origin,
                                         text_color, ranges );
        }
    }
}

static const BYTE masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static const int padding[4] = {0, 3, 2, 1};

/***********************************************************************
 *         get_glyph_bitmap
 *
 * Retrieve a 17-level bitmap for the appropriate glyph.
 *
 * For non-antialiased bitmaps convert them to the 17-level format
 * using only values 0 or 16.
 */
static DWORD get_glyph_bitmap( HDC hdc, UINT index, UINT aa_flags, GLYPHMETRICS *metrics,
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

    for (i = 0; i < sizeof(indices) / sizeof(indices[0]); i++)
    {
        index = indices[i];
        ret = GetGlyphOutlineW( hdc, index, ggo_flags, metrics, 0, NULL, &identity );
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

    ret = GetGlyphOutlineW( hdc, index, ggo_flags, metrics, size, buf, &identity );
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

BOOL render_aa_text_bitmapinfo( HDC hdc, BITMAPINFO *info, struct gdi_image_bits *bits,
                                struct bitblt_coords *src, INT x, INT y, UINT flags,
                                UINT aa_flags, LPCWSTR str, UINT count, const INT *dx )
{
    dib_info dib;
    UINT i;
    DWORD err;
    BOOL got_pixel;
    COLORREF fg, bg;
    DWORD fg_pixel, bg_pixel;
    struct intensity_range glyph_intensities[17];

    assert( info->bmiHeader.biBitCount > 8 ); /* mono and indexed formats don't support anti-aliasing */

    init_dib_info_from_bitmapinfo( &dib, info, bits->ptr );

    fg = make_rgb_colorref( hdc, &dib, GetTextColor( hdc ), &got_pixel, &fg_pixel);
    if (!got_pixel) fg_pixel = dib.funcs->colorref_to_pixel( &dib, fg );

    get_aa_ranges( fg, glyph_intensities );

    if (flags & ETO_OPAQUE)
    {
        rop_mask bkgnd_color;

        bg = make_rgb_colorref( hdc, &dib, GetBkColor( hdc ), &got_pixel, &bg_pixel);
        if (!got_pixel) bg_pixel = dib.funcs->colorref_to_pixel( &dib, bg );

        bkgnd_color.and = 0;
        bkgnd_color.xor = bg_pixel;
        dib.funcs->solid_rects( &dib, 1, &src->visrect, bkgnd_color.and, bkgnd_color.xor );
    }

    for (i = 0; i < count; i++)
    {
        GLYPHMETRICS metrics;
        struct gdi_image_bits image;

        err = get_glyph_bitmap( hdc, (UINT)str[i], aa_flags, &metrics, &image );
        if (err) continue;

        if (image.ptr)
        {
            RECT rect, clipped_rect;
            POINT src_origin;
            dib_info glyph_dib;

            glyph_dib.bit_count   = 8;
            glyph_dib.width       = metrics.gmBlackBoxX;
            glyph_dib.height      = metrics.gmBlackBoxY;
            glyph_dib.rect.left   = 0;
            glyph_dib.rect.top    = 0;
            glyph_dib.rect.right  = metrics.gmBlackBoxX;
            glyph_dib.rect.bottom = metrics.gmBlackBoxY;
            glyph_dib.stride      = get_dib_stride( metrics.gmBlackBoxX, 8 );
            glyph_dib.bits        = image;

            rect.left   = x + metrics.gmptGlyphOrigin.x;
            rect.top    = y - metrics.gmptGlyphOrigin.y;
            rect.right  = rect.left + metrics.gmBlackBoxX;
            rect.bottom = rect.top  + metrics.gmBlackBoxY;

            if (intersect_rect( &clipped_rect, &rect, &src->visrect ))
            {
                src_origin.x = clipped_rect.left - rect.left;
                src_origin.y = clipped_rect.top  - rect.top;

                dib.funcs->draw_glyph( &dib, &clipped_rect, &glyph_dib, &src_origin,
                                       fg_pixel, glyph_intensities );
            }
        }
        if (image.free) image.free( &image );

        if (dx)
        {
            if (flags & ETO_PDY)
            {
                x += dx[ i * 2 ];
                y += dx[ i * 2 + 1];
            }
            else
                x += dx[ i ];
        }
        else
        {
            x += metrics.gmCellIncX;
            y += metrics.gmCellIncY;
        }
    }
    return TRUE;
}

/***********************************************************************
 *           dibdrv_ExtTextOut
 */
BOOL dibdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                        const RECT *rect, LPCWSTR str, UINT count, const INT *dx )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    struct clipped_rects clipped_rects;
    UINT aa_flags, i;
    POINT origin;
    RECT bounds;
    DWORD text_color, err;
    struct intensity_range ranges[17];

    init_clipped_rects( &clipped_rects );
    reset_bounds( &bounds );

    if (flags & ETO_OPAQUE)
    {
        rop_mask bkgnd_color;
        get_text_bkgnd_masks( pdev, &bkgnd_color );
        add_bounds_rect( &bounds, rect );
        get_clipped_rects( &pdev->dib, rect, pdev->clip, &clipped_rects );
        pdev->dib.funcs->solid_rects( &pdev->dib, clipped_rects.count, clipped_rects.rects,
                                      bkgnd_color.and, bkgnd_color.xor );
    }

    if (count == 0) goto done;

    if (flags & ETO_CLIPPED)
    {
        if (!(flags & ETO_OPAQUE))  /* otherwise we have done it already */
            get_clipped_rects( &pdev->dib, rect, pdev->clip, &clipped_rects );
    }
    else
    {
        free_clipped_rects( &clipped_rects );
        get_clipped_rects( &pdev->dib, NULL, pdev->clip, &clipped_rects );
    }
    if (!clipped_rects.count) goto done;

    text_color = get_pixel_color( pdev, GetTextColor( pdev->dev.hdc ), TRUE );
    get_aa_ranges( pdev->dib.funcs->pixel_to_colorref( &pdev->dib, text_color ), ranges );

    aa_flags = get_font_aa_flags( dev->hdc );
    origin.x = x;
    origin.y = y;
    for (i = 0; i < count; i++)
    {
        GLYPHMETRICS metrics;
        struct gdi_image_bits image;

        err = get_glyph_bitmap( dev->hdc, (UINT)str[i], aa_flags, &metrics, &image );
        if (err) continue;

        if (image.ptr)
            draw_glyph( pdev, &origin, &metrics, &image, text_color, ranges, &clipped_rects, &bounds );

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

done:
    add_clipped_bounds( pdev, &bounds, pdev->clip );
    free_clipped_rects( &clipped_rects );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_Arc
 */
BOOL dibdrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                 INT start_x, INT start_y, INT end_x, INT end_y )
{
    return draw_arc( dev, left, top, right, bottom, start_x, start_y, end_x, end_y, 0 );
}

/***********************************************************************
 *           dibdrv_ArcTo
 */
BOOL dibdrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                   INT start_x, INT start_y, INT end_x, INT end_y )
{
    return draw_arc( dev, left, top, right, bottom, start_x, start_y, end_x, end_y, -1 );
}

/***********************************************************************
 *           dibdrv_Chord
 */
BOOL dibdrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                   INT start_x, INT start_y, INT end_x, INT end_y )
{
    return draw_arc( dev, left, top, right, bottom, start_x, start_y, end_x, end_y, 1 );
}

/***********************************************************************
 *           dibdrv_Ellipse
 */
BOOL dibdrv_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return dibdrv_RoundRect( dev, left, top, right, bottom, right - left, bottom - top );
}

static inline BOOL is_interior( dib_info *dib, HRGN clip, int x, int y, DWORD pixel, UINT type)
{
    /* the clip rgn stops the flooding */
    if (clip && !PtInRegion( clip, x, y )) return FALSE;

    if (type == FLOODFILLBORDER)
        return dib->funcs->get_pixel( dib, x, y ) != pixel;
    else
        return dib->funcs->get_pixel( dib, x, y ) == pixel;
}

static void fill_row( dib_info *dib, HRGN clip, RECT *row, DWORD pixel, UINT type, HRGN rgn );

static inline void do_next_row( dib_info *dib, HRGN clip, const RECT *row, int offset, DWORD pixel, UINT type, HRGN rgn )
{
    RECT next;

    next.top = row->top + offset;
    next.bottom = next.top + 1;
    next.left = next.right = row->left;
    while (next.right < row->right)
    {
        if (is_interior( dib, clip, next.right, next.top, pixel, type)) next.right++;
        else
        {
            if (next.left != next.right && !PtInRegion( rgn, next.left, next.top ))
                fill_row( dib, clip, &next, pixel, type, rgn );
            next.left = ++next.right;
        }
    }
    if (next.left != next.right && !PtInRegion( rgn, next.left, next.top ))
        fill_row( dib, clip, &next, pixel, type, rgn );
}

static void fill_row( dib_info *dib, HRGN clip, RECT *row, DWORD pixel, UINT type, HRGN rgn )
{
    while (row->left > 0 && is_interior( dib, clip, row->left - 1, row->top, pixel, type)) row->left--;
    while (row->right < dib->rect.right - dib->rect.left &&
           is_interior( dib, clip, row->right, row->top, pixel, type))
        row->right++;

    add_rect_to_region( rgn, row );

    if (row->top > 0) do_next_row( dib, clip, row, -1, pixel, type, rgn );
    if (row->top < dib->rect.bottom - dib->rect.top - 1)
        do_next_row( dib, clip, row, 1, pixel, type, rgn );
}

/***********************************************************************
 *           dibdrv_ExtFloodFill
 */
BOOL dibdrv_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT type )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    DWORD pixel = get_pixel_color( pdev, color, FALSE );
    RECT row;
    HRGN rgn;

    TRACE( "(%p, %d, %d, %08x, %d)\n", pdev, x, y, color, type );

    if (!is_interior( &pdev->dib, pdev->clip, x, y, pixel, type )) return FALSE;

    if (!(rgn = CreateRectRgn( 0, 0, 0, 0 ))) return FALSE;
    row.left = x;
    row.right = x + 1;
    row.top = y;
    row.bottom = y + 1;

    fill_row( &pdev->dib, pdev->clip, &row, pixel, type, rgn );

    add_clipped_bounds( pdev, NULL, rgn );
    brush_region( pdev, rgn );

    DeleteObject( rgn );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_GetNearestColor
 */
COLORREF dibdrv_GetNearestColor( PHYSDEV dev, COLORREF color )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    DWORD pixel;

    TRACE( "(%p, %08x)\n", dev, color );

    pixel = get_pixel_color( pdev, color, FALSE );
    return pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );
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

    if (pt.x < 0 || pt.x >= pdev->dib.rect.right - pdev->dib.rect.left ||
        pt.y < 0 || pt.y >= pdev->dib.rect.bottom - pdev->dib.rect.top)
        return CLR_INVALID;

    pixel = pdev->dib.funcs->get_pixel( &pdev->dib, pt.x, pt.y );
    return pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );
}

/***********************************************************************
 *           dibdrv_LineTo
 */
BOOL dibdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    POINT pts[2];
    HRGN region = 0;
    BOOL ret;

    GetCurrentPositionEx(dev->hdc, pts);
    pts[1].x = x;
    pts[1].y = y;

    LPtoDP(dev->hdc, pts, 2);

    if (pdev->pen_uses_region && !(region = CreateRectRgn( 0, 0, 0, 0 ))) return FALSE;

    reset_dash_origin(pdev);

    ret = pdev->pen_lines(pdev, 2, pts, FALSE, region);
    add_pen_lines_bounds( pdev, 2, pts, region );

    if (region)
    {
        ret = pen_region( pdev, region );
        DeleteObject( region );
    }
    return ret;
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
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    dib_brush *brush = &pdev->brush;
    int rop2 = get_rop2_from_rop( rop );
    struct clipped_rects clipped_rects;
    DWORD and = 0, xor = 0;
    BOOL ret = TRUE;

    TRACE("(%p, %d, %d, %d, %d, %06x)\n", dev, dst->x, dst->y, dst->width, dst->height, rop);

    add_clipped_bounds( pdev, &dst->visrect, 0 );
    if (!get_clipped_rects( &pdev->dib, &dst->visrect, pdev->clip, &clipped_rects )) return TRUE;

    switch (rop2)  /* shortcuts for rops that don't involve the brush */
    {
    case R2_NOT:   and = ~0u;
        /* fall through */
    case R2_WHITE: xor = ~0u;
        /* fall through */
    case R2_BLACK:
        pdev->dib.funcs->solid_rects( &pdev->dib, clipped_rects.count, clipped_rects.rects, and, xor );
        /* fall through */
    case R2_NOP:
        break;
    default:
        ret = brush->rects( pdev, brush, &pdev->dib, clipped_rects.count, clipped_rects.rects, rop2 );
        break;
    }
    free_clipped_rects( &clipped_rects );
    return ret;
}

/***********************************************************************
 *           dibdrv_PaintRgn
 */
BOOL dibdrv_PaintRgn( PHYSDEV dev, HRGN rgn )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    const WINEREGION *region;
    int i;
    RECT rect, bounds;

    TRACE("%p, %p\n", dev, rgn);

    reset_bounds( &bounds );

    region = get_wine_region( rgn );
    if(!region) return FALSE;

    for(i = 0; i < region->numRects; i++)
    {
        rect = get_device_rect( dev->hdc, region->rects[i].left, region->rects[i].top,
                                region->rects[i].right, region->rects[i].bottom, FALSE );
        add_bounds_rect( &bounds, &rect );
        brush_rect( pdev, &pdev->brush, &rect, pdev->clip );
    }

    release_wine_region( rgn );
    add_clipped_bounds( pdev, &bounds, pdev->clip );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_PolyPolygon
 */
BOOL dibdrv_PolyPolygon( PHYSDEV dev, const POINT *pt, const INT *counts, DWORD polygons )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DWORD total, i, pos;
    BOOL ret = TRUE;
    POINT *points;
    HRGN outline = 0, interior = 0;

    for (i = total = 0; i < polygons; i++)
    {
        if (counts[i] < 2) return FALSE;
        total += counts[i];
    }

    points = HeapAlloc( GetProcessHeap(), 0, total * sizeof(*pt) );
    if (!points) return FALSE;
    memcpy( points, pt, total * sizeof(*pt) );
    LPtoDP( dev->hdc, points, total );

    if (pdev->brush.style != BS_NULL &&
        !(interior = CreatePolyPolygonRgn( points, counts, polygons, GetPolyFillMode( dev->hdc ))))
    {
        HeapFree( GetProcessHeap(), 0, points );
        return FALSE;
    }

    if (pdev->pen_uses_region) outline = CreateRectRgn( 0, 0, 0, 0 );

    /* if not using a region, paint the interior first so the outline can overlap it */
    if (interior && !outline)
    {
        ret = brush_region( pdev, interior );
        DeleteObject( interior );
        interior = 0;
    }

    for (i = pos = 0; i < polygons; i++)
    {
        reset_dash_origin( pdev );
        pdev->pen_lines( pdev, counts[i], points + pos, TRUE, outline );
        pos += counts[i];
    }
    add_pen_lines_bounds( pdev, total, points, outline );

    if (interior)
    {
        CombineRgn( interior, interior, outline, RGN_DIFF );
        ret = brush_region( pdev, interior );
        DeleteObject( interior );
    }
    if (outline)
    {
        if (ret) ret = pen_region( pdev, outline );
        DeleteObject( outline );
    }
    HeapFree( GetProcessHeap(), 0, points );
    return ret;
}

/***********************************************************************
 *           dibdrv_PolyPolyline
 */
BOOL dibdrv_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DWORD total, pos, i;
    POINT *points;
    BOOL ret = TRUE;
    HRGN outline = 0;

    for (i = total = 0; i < polylines; i++)
    {
        if (counts[i] < 2) return FALSE;
        total += counts[i];
    }

    points = HeapAlloc( GetProcessHeap(), 0, total * sizeof(*pt) );
    if (!points) return FALSE;
    memcpy( points, pt, total * sizeof(*pt) );
    LPtoDP( dev->hdc, points, total );

    if (pdev->pen_uses_region && !(outline = CreateRectRgn( 0, 0, 0, 0 )))
    {
        HeapFree( GetProcessHeap(), 0, points );
        return FALSE;
    }

    for (i = pos = 0; i < polylines; i++)
    {
        reset_dash_origin( pdev );
        pdev->pen_lines( pdev, counts[i], points + pos, FALSE, outline );
        pos += counts[i];
    }
    add_pen_lines_bounds( pdev, total, points, outline );

    if (outline)
    {
        ret = pen_region( pdev, outline );
        DeleteObject( outline );
    }

    HeapFree( GetProcessHeap(), 0, points );
    return ret;
}

/***********************************************************************
 *           dibdrv_Polygon
 */
BOOL dibdrv_Polygon( PHYSDEV dev, const POINT *pt, INT count )
{
    INT counts[1] = { count };

    return dibdrv_PolyPolygon( dev, pt, counts, 1 );
}

/***********************************************************************
 *           dibdrv_Polyline
 */
BOOL dibdrv_Polyline( PHYSDEV dev, const POINT* pt, INT count )
{
    DWORD counts[1] = { count };

    if (count < 0) return FALSE;
    return dibdrv_PolyPolyline( dev, pt, counts, 1 );
}

/***********************************************************************
 *           dibdrv_Rectangle
 */
BOOL dibdrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    RECT rect;
    POINT pts[4];
    BOOL ret;
    HRGN outline = 0;

    TRACE("(%p, %d, %d, %d, %d)\n", dev, left, top, right, bottom);

    if (!get_pen_device_rect( pdev, &rect, left, top, right, bottom )) return TRUE;

    if (pdev->pen_uses_region && !(outline = CreateRectRgn( 0, 0, 0, 0 ))) return FALSE;

    rect.right--;
    rect.bottom--;
    reset_dash_origin(pdev);

    if (GetArcDirection( dev->hdc ) == AD_CLOCKWISE)
    {
        /* 4 pts going clockwise starting from bottom-right */
        pts[0].x = pts[3].x = rect.right;
        pts[0].y = pts[1].y = rect.bottom;
        pts[1].x = pts[2].x = rect.left;
        pts[2].y = pts[3].y = rect.top;
    }
    else
    {
        /* 4 pts going anti-clockwise starting from top-right */
        pts[0].x = pts[3].x = rect.right;
        pts[0].y = pts[1].y = rect.top;
        pts[1].x = pts[2].x = rect.left;
        pts[2].y = pts[3].y = rect.bottom;
    }

    pdev->pen_lines(pdev, 4, pts, TRUE, outline);
    add_pen_lines_bounds( pdev, 4, pts, outline );

    if (outline)
    {
        if (pdev->brush.style != BS_NULL)
        {
            HRGN interior = CreateRectRgnIndirect( &rect );

            CombineRgn( interior, interior, outline, RGN_DIFF );
            brush_region( pdev, interior );
            DeleteObject( interior );
        }
        ret = pen_region( pdev, outline );
        DeleteObject( outline );
    }
    else
    {
        rect.left   += (pdev->pen_width + 1) / 2;
        rect.top    += (pdev->pen_width + 1) / 2;
        rect.right  -= pdev->pen_width / 2;
        rect.bottom -= pdev->pen_width / 2;
        ret = brush_rect( pdev, &pdev->brush, &rect, pdev->clip );
    }
    return ret;
}

/***********************************************************************
 *           dibdrv_RoundRect
 */
BOOL dibdrv_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                       INT ellipse_width, INT ellipse_height )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    RECT rect;
    POINT pt[2], *points;
    int i, end, count;
    BOOL ret = TRUE;
    HRGN outline = 0, interior = 0;

    if (!get_pen_device_rect( pdev, &rect, left, top, right, bottom )) return TRUE;

    pt[0].x = pt[0].y = 0;
    pt[1].x = ellipse_width;
    pt[1].y = ellipse_height;
    LPtoDP( dev->hdc, pt, 2 );
    ellipse_width = min( rect.right - rect.left, abs( pt[1].x - pt[0].x ));
    ellipse_height = min( rect.bottom - rect.top, abs( pt[1].y - pt[0].y ));
    if (ellipse_width <= 2|| ellipse_height <= 2)
        return dibdrv_Rectangle( dev, left, top, right, bottom );

    points = HeapAlloc( GetProcessHeap(), 0, (ellipse_width + ellipse_height) * 2 * sizeof(*points) );
    if (!points) return FALSE;

    if (pdev->pen_uses_region && !(outline = CreateRectRgn( 0, 0, 0, 0 )))
    {
        HeapFree( GetProcessHeap(), 0, points );
        return FALSE;
    }

    if (pdev->brush.style != BS_NULL &&
        !(interior = CreateRoundRectRgn( rect.left, rect.top, rect.right + 1, rect.bottom + 1,
                                         ellipse_width, ellipse_height )))
    {
        HeapFree( GetProcessHeap(), 0, points );
        if (outline) DeleteObject( outline );
        return FALSE;
    }

    /* if not using a region, paint the interior first so the outline can overlap it */
    if (interior && !outline)
    {
        ret = brush_region( pdev, interior );
        DeleteObject( interior );
        interior = 0;
    }

    count = ellipse_first_quadrant( ellipse_width, ellipse_height, points );

    if (GetArcDirection( dev->hdc ) == AD_CLOCKWISE)
    {
        for (i = 0; i < count; i++)
        {
            points[i].x = rect.right - ellipse_width + points[i].x;
            points[i].y = rect.bottom - ellipse_height + points[i].y;
        }
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            points[i].x = rect.right - ellipse_width + points[i].x;
            points[i].y = rect.top + ellipse_height - 1 - points[i].y;
        }
    }

    /* horizontal symmetry */

    end = 2 * count - 1;
    /* avoid duplicating the midpoint */
    if (ellipse_width % 2 && ellipse_width == rect.right - rect.left) end--;
    for (i = 0; i < count; i++)
    {
        points[end - i].x = rect.left + rect.right - 1 - points[i].x;
        points[end - i].y = points[i].y;
    }
    count = end + 1;

    /* vertical symmetry */

    end = 2 * count - 1;
    /* avoid duplicating the midpoint */
    if (ellipse_height % 2 && ellipse_height == rect.bottom - rect.top) end--;
    for (i = 0; i < count; i++)
    {
        points[end - i].x = points[i].x;
        points[end - i].y = rect.top + rect.bottom - 1 - points[i].y;
    }
    count = end + 1;

    reset_dash_origin( pdev );
    pdev->pen_lines( pdev, count, points, TRUE, outline );
    add_pen_lines_bounds( pdev, count, points, outline );

    if (interior)
    {
        CombineRgn( interior, interior, outline, RGN_DIFF );
        ret = brush_region( pdev, interior );
        DeleteObject( interior );
    }
    if (outline)
    {
        if (ret) ret = pen_region( pdev, outline );
        DeleteObject( outline );
    }
    HeapFree( GetProcessHeap(), 0, points );
    return ret;
}

/***********************************************************************
 *           dibdrv_Pie
 */
BOOL dibdrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                 INT start_x, INT start_y, INT end_x, INT end_y )
{
    return draw_arc( dev, left, top, right, bottom, start_x, start_y, end_x, end_y, 2 );
}

/***********************************************************************
 *           dibdrv_SetPixel
 */
COLORREF dibdrv_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    struct clipped_rects clipped_rects;
    RECT rect;
    POINT pt;
    DWORD pixel;

    TRACE( "(%p, %d, %d, %08x)\n", dev, x, y, color );

    pt.x = x;
    pt.y = y;
    LPtoDP( dev->hdc, &pt, 1 );
    rect.left = pt.x;
    rect.top =  pt.y;
    rect.right = rect.left + 1;
    rect.bottom = rect.top + 1;
    add_clipped_bounds( pdev, &rect, pdev->clip );

    /* SetPixel doesn't do the 1bpp massaging like other fg colors */
    pixel = get_pixel_color( pdev, color, FALSE );
    color = pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );

    if (!get_clipped_rects( &pdev->dib, &rect, pdev->clip, &clipped_rects )) return color;
    pdev->dib.funcs->solid_rects( &pdev->dib, clipped_rects.count, clipped_rects.rects, 0, pixel );
    free_clipped_rects( &clipped_rects );
    return color;
}
