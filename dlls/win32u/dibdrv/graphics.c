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

#if 0
#pragma makedep unix
#endif

#include <assert.h>
#include <pthread.h>
#include "ntgdi_private.h"
#include "dibdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

struct cached_glyph
{
    GLYPHMETRICS metrics;
    BYTE         bits[1];
};

enum glyph_type
{
    GLYPH_INDEX,
    GLYPH_WCHAR,
    GLYPH_NBTYPES
};

#define GLYPH_CACHE_PAGE_SIZE  0x100
#define GLYPH_CACHE_PAGES      (0x10000 / GLYPH_CACHE_PAGE_SIZE)

struct cached_font
{
    struct list           entry;
    LONG                  ref;
    DWORD                 hash;
    LOGFONTW              lf;
    XFORM                 xform;
    UINT                  aa_flags;
    struct cached_glyph **glyphs[GLYPH_NBTYPES][GLYPH_CACHE_PAGES];
};

static struct list font_cache = LIST_INIT( font_cache );

static pthread_mutex_t font_cache_lock = PTHREAD_MUTEX_INITIALIZER;


static BOOL brush_rect( dibdrv_physdev *pdev, dib_brush *brush, const RECT *rect, HRGN clip )
{
    DC *dc = get_physdev_dc( &pdev->dev );
    struct clipped_rects clipped_rects;
    BOOL ret;

    if (!get_clipped_rects( &pdev->dib, rect, clip, &clipped_rects )) return TRUE;
    ret = brush->rects( pdev, brush, &pdev->dib, clipped_rects.count, clipped_rects.rects,
                        &dc->attr->brush_org, dc->attr->rop_mode );
    free_clipped_rects( &clipped_rects );
    return ret;
}

/* paint a region with the brush (note: the region can be modified) */
static BOOL brush_region( dibdrv_physdev *pdev, HRGN region )
{
    if (pdev->clip) NtGdiCombineRgn( region, region, pdev->clip, RGN_AND );
    return brush_rect( pdev, &pdev->brush, NULL, region );
}

/* paint a region with the pen (note: the region can be modified) */
static BOOL pen_region( dibdrv_physdev *pdev, HRGN region )
{
    if (pdev->clip) NtGdiCombineRgn( region, region, pdev->clip, RGN_AND );
    return brush_rect( pdev, &pdev->pen_brush, NULL, region );
}

static RECT get_device_rect( DC *dc, int left, int top, int right, int bottom, BOOL rtl_correction )
{
    RECT rect;

    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;
    if (rtl_correction && dc->attr->layout & LAYOUT_RTL)
    {
        /* shift the rectangle so that the right border is included after mirroring */
        /* it would be more correct to do this after LPtoDP but that's not what Windows does */
        rect.left--;
        rect.right--;
    }
    lp_to_dp( dc, (POINT *)&rect, 2 );
    order_rect( &rect );
    return rect;
}

static BOOL get_pen_device_rect( DC *dc, dibdrv_physdev *dev, RECT *rect,
                                 int left, int top, int right, int bottom )
{
    *rect = get_device_rect( dc, left, top, right, bottom, TRUE );
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

/* Draw the "top half" of the ellipse, viz. the half that doesn't have any
 * points in the lower-right quadrant.
 *
 * We draw as many as eight arcs; in each we always increment one of X or Y
 * (depending on whether the absolute value of the slope is less than
 * or greater than 1) and possibly increment the other.
 *
 * Loosely based on an algorithm by David Eberly.
 */
static unsigned int generate_ellipse_top_half( const DC *dc, double width, double height, POINT *points )
{
    /* A general ellipse centered at (0, 0) can be defined by
     *   ax² + 2bxy + cy² - d = 0.
     * The transformation of an ellipse can be obtained by inverting the
     * transformation matrix and substituting the transformed x and y, which
     * yields an ellipse in the same form.
     * Precompute the transformed coefficients. */
    const XFORM *xform = &dc->xformVport2World;
    double wsq = (width / 2) * (width / 2);
    double hsq = (height / 2) * (height / 2);
    const double a = (hsq * xform->eM11 * xform->eM11) + (wsq * xform->eM12 * xform->eM12);
    const double b = (hsq * xform->eM11 * xform->eM21) + (wsq * xform->eM12 * xform->eM22);
    const double c = (hsq * xform->eM21 * xform->eM21) + (wsq * xform->eM22 * xform->eM22);
    const double d = (hsq * wsq);
    unsigned int pos = 0;
    int start_x;
    POINT pt;

    /* We start from the point that satisfies y = 0, i.e. ax² = d.
     * Note that a must be nonzero if the transformation matrix was singular.
     *
     * d/a must be positive, since both a and d are comprised of squares.
     *
     * This point is not on the ellipse axis, but that doesn't matter for our
     * algorithm. We will take this set of points and rotate it 180 degrees
     * to get the opposite side. */
    pt.x = -sqrt(d / a);
    pt.y = 0;
    start_x = pt.x;

    while (pt.y > 0 || (pt.y == 0 && pt.x != -start_x))
    {
        double dx =  (b * pt.x + c * pt.y);
        double dy = -(a * pt.x + b * pt.y);
        int x_inc = (dx > 0 ? 1 : -1);
        int y_inc = (dy > 0 ? 1 : -1);
        double sigma1, sigma2;

        points[pos++] = pt;

        if (fabs(dy) > fabs(dx))
        {
            /* Increment y, maybe increment x. */
            pt.y += y_inc;

            /* If this point has 45° slope, and it's directly adjacent to one
             * we just wrote, using the algorithm as-is will select this point
             * and then the one horizontally adjacent to it (our reflection
             * across the 45° line.) This creates an L-shaped corner, which we
             * don't want. Force incrementing X to skip that corner. */
            if (fabs(b * pt.x + c * pt.y) == fabs(a * pt.x + b * pt.y))
            {
                pt.x += x_inc;
                continue;
            }

            sigma1 = (a * pt.x * pt.x) + (2 * b * pt.x * pt.y) + (c * pt.y * pt.y) - d;
            pt.x += x_inc;
            sigma2 = (a * pt.x * pt.x) + (2 * b * pt.x * pt.y) + (c * pt.y * pt.y) - d;

            /* Pick whichever point is closer to the ellipse. */
            if (fabs(sigma2) > fabs(sigma1))
                pt.x -= x_inc;
        }
        else
        {
            /* Increment x, maybe increment y. */
            pt.x += x_inc;

            if (fabs(b * pt.x + c * pt.y) == fabs(a * pt.x + b * pt.y))
            {
                pt.y += y_inc;
                continue;
            }

            sigma1 = (a * pt.x * pt.x) + (2 * b * pt.x * pt.y) + (c * pt.y * pt.y) - d;
            pt.y += y_inc;
            sigma2 = (a * pt.x * pt.x) + (2 * b * pt.x * pt.y) + (c * pt.y * pt.y) - d;

            if (fabs(sigma2) > fabs(sigma1))
                pt.y -= y_inc;
        }
    }
    return pos;
}

static int find_intersection( const POINT *points, int x, int y, int count )
{
    int i;

    /* First point is always x <= 0, y == 0, so if y >= 0 we are in the
     * top half. */

    if (!y)
        return (x < 0) ? 0 : count;

    if (y > 0)
    {
        /* top half */
        for (i = 0; i < count; i++)
        {
            if (points[i].x * y >= points[i].y * x)
                break;
        }
        return i;
    }
    else
    {
        /* bottom half */
        for (i = 0; i < count; i++)
        {
            if (points[i].x * -y >= points[i].y * -x)
                break;
        }
        return count + i;
    }
}

static void lp_to_dp_no_translate( DC *dc, POINT *point )
{
    double x = point->x;
    double y = point->y;
    point->x = GDI_ROUND( x * dc->xformWorld2Vport.eM11 + y * dc->xformWorld2Vport.eM21 );
    point->y = GDI_ROUND( x * dc->xformWorld2Vport.eM12 + y * dc->xformWorld2Vport.eM22 );
}

static int get_arc_points( DC *dc, int arc_dir, const RECT *rect, POINT start, POINT end, POINT *points )
{
    int i, pos, count, start_pos, end_pos;
    int width = rect->right - rect->left;
    int height = rect->bottom - rect->top;
    POINT center = {rect->left + width / 2, rect->top + height / 2};

    count = generate_ellipse_top_half( dc, width, height, points );

    /* Transform the start and end, but do not translate them, so that they
     * remain relative to the ellipse center. */
    lp_to_dp_no_translate( dc, &start );
    lp_to_dp_no_translate( dc, &end );

    start_pos = find_intersection( points, start.x, start.y, count );
    end_pos = find_intersection( points, end.x, end.y, count );
    if (arc_dir == AD_CLOCKWISE)
    {
        int tmp = start_pos;
        start_pos = end_pos;
        end_pos = tmp;
    }
    if (end_pos <= start_pos) end_pos += 2 * count;

    lp_to_dp( dc, &center, 1 );

    pos = count;
    if (arc_dir == AD_COUNTERCLOCKWISE)
    {
        for (i = start_pos; i < end_pos; i++, pos++)
        {
            if ((i / count) % 2 == 0)
            {
                points[pos].x = center.x + points[i % count].x;
                points[pos].y = center.y + points[i % count].y;
            }
            else
            {
                points[pos].x = center.x - points[i % count].x;
                points[pos].y = center.y - points[i % count].y;
            }
        }
    }
    else
    {
        for (i = end_pos - 1; i >= start_pos; i--, pos++)
        {
            if ((i / count) % 2 == 0)
            {
                points[pos].x = center.x + points[i % count].x;
                points[pos].y = center.y + points[i % count].y;
            }
            else
            {
                points[pos].x = center.x - points[i % count].x;
                points[pos].y = center.y - points[i % count].y;
            }
        }
    }

    memmove( points, points + count, (pos - count) * sizeof(POINT) );
    return pos - count;
}

static unsigned int get_arc_max_points( DC *dc, const RECT *rect )
{
    POINT pts[4] =
    {
        {rect->left, rect->top},
        {rect->left, rect->bottom},
        {rect->right, rect->bottom},
        {rect->right, rect->top}
    };
    unsigned int width, height;

    lp_to_dp( dc, pts, 4 );
    width = max( abs( pts[2].x - pts[0].x ), abs( pts[3].x - pts[1].x ));
    height = max( abs( pts[2].y - pts[0].y ), abs( pts[3].y - pts[1].y ));
    return (width + height) * 3;
}

/* backend for arc functions; extra_lines is -1 for ArcTo, 0 for Arc, 1 for Chord, 2 for Pie */
static BOOL draw_arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                      INT start_x, INT start_y, INT end_x, INT end_y, INT extra_lines )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    DC *dc = get_physdev_dc( dev );
    RECT rect, rc;
    POINT pt[2], *points;
    int width, height, count, max_points;
    BOOL ret = TRUE;
    HRGN outline = 0, interior = 0;

    SetRect( &rect, left, top, right, bottom );

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    pt[0].x = start_x;
    pt[0].y = start_y;
    pt[1].x = end_x;
    pt[1].y = end_y;
    /* make them relative to the ellipse center */
    pt[0].x -= rect.left + width / 2;
    pt[0].y -= rect.top + height / 2;
    pt[1].x -= rect.left + width / 2;
    pt[1].y -= rect.top + height / 2;

    max_points = get_arc_max_points( dc, &rect );
    points = malloc( max_points * sizeof(*points) );
    if (!points) return FALSE;

    if (extra_lines == -1)
    {
        points[0] = dc->attr->cur_pos;
        lp_to_dp( dc, points, 1 );
        count = 1 + get_arc_points( dc, dc->attr->arc_direction, &rect, pt[0], pt[1], points + 1 );
    }
    else count = get_arc_points( dc, dc->attr->arc_direction, &rect, pt[0], pt[1], points );

    if (count > max_points)
        ERR( "point count %u exceeds max points %u\n", count, max_points );

    if (extra_lines == 2)
    {
        points[count].x = rect.left + width / 2;
        points[count].y = rect.top + height / 2;
        lp_to_dp( dc, &points[count], 1 );
        count++;
    }
    if (count < 2)
    {
        free( points );
        return TRUE;
    }

    if (pdev->pen_uses_region && !(outline = NtGdiCreateRectRgn( 0, 0, 0, 0 )))
    {
        free( points );
        return FALSE;
    }

    if (pdev->brush.style != BS_NULL &&
        extra_lines > 0 &&
        get_dib_rect( &pdev->dib, &rc ) &&
        !(interior = create_polypolygon_region( points, &count, 1, WINDING, &rc )))
    {
        free( points );
        if (outline) NtGdiDeleteObjectApp( outline );
        return FALSE;
    }

    /* if not using a region, paint the interior first so the outline can overlap it */
    if (interior && !outline)
    {
        ret = brush_region( pdev, interior );
        NtGdiDeleteObjectApp( interior );
        interior = 0;
    }

    reset_dash_origin( pdev );
    pdev->pen_lines( pdev, count, points, extra_lines > 0, outline );
    add_pen_lines_bounds( pdev, count, points, outline );

    if (interior)
    {
        NtGdiCombineRgn( interior, interior, outline, RGN_DIFF );
        ret = brush_region( pdev, interior );
        NtGdiDeleteObjectApp( interior );
    }
    if (outline)
    {
        if (ret) ret = pen_region( pdev, outline );
        NtGdiDeleteObjectApp( outline );
    }
    free( points );
    return ret;
}

/* helper for path stroking and filling functions */
static BOOL stroke_and_fill_path( dibdrv_physdev *dev, BOOL stroke, BOOL fill )
{
    DC *dc = get_physdev_dc( &dev->dev );
    struct gdi_path *path;
    POINT *points;
    BYTE *types;
    BOOL ret = TRUE;
    HRGN outline = 0, interior = 0;
    int i, pos, total;

    if (dev->brush.style == BS_NULL) fill = FALSE;

    if (!(path = get_gdi_flat_path( dc, fill ? &interior : NULL ))) return FALSE;
    if (!(total = get_gdi_path_data( path, &points, &types ))) goto done;

    if (stroke && dev->pen_uses_region) outline = NtGdiCreateRectRgn( 0, 0, 0, 0 );

    /* if not using a region, paint the interior first so the outline can overlap it */
    if (interior && !outline)
    {
        ret = brush_region( dev, interior );
        NtGdiDeleteObjectApp( interior );
        interior = 0;
    }

    if (stroke)
    {
        pos = 0;
        for (i = 1; i < total; i++)
        {
            if (types[i] != PT_MOVETO) continue;
            if (i > pos + 1)
            {
                reset_dash_origin( dev );
                dev->pen_lines( dev, i - pos, points + pos,
                                fill || types[i - 1] & PT_CLOSEFIGURE, outline );
            }
            pos = i;
        }
        if (i > pos + 1)
        {
            reset_dash_origin( dev );
            dev->pen_lines( dev, i - pos, points + pos,
                            fill || types[i - 1] & PT_CLOSEFIGURE, outline );
        }
    }

    add_pen_lines_bounds( dev, total, points, outline );

    if (interior)
    {
        NtGdiCombineRgn( interior, interior, outline, RGN_DIFF );
        ret = brush_region( dev, interior );
        NtGdiDeleteObjectApp( interior );
    }
    if (outline)
    {
        if (ret) ret = pen_region( dev, outline );
        NtGdiDeleteObjectApp( outline );
    }

done:
    free_gdi_path( path );
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

static DWORD font_cache_hash( struct cached_font *font )
{
    DWORD hash = 0, *ptr, two_chars;
    WORD *pwc;
    int i;

    hash ^= font->aa_flags;
    for(i = 0, ptr = (DWORD*)&font->xform; i < sizeof(XFORM)/sizeof(DWORD); i++, ptr++)
        hash ^= *ptr;
    for(i = 0, ptr = (DWORD*)&font->lf; i < 7; i++, ptr++)
        hash ^= *ptr;
    for(i = 0, ptr = (DWORD*)font->lf.lfFaceName; i < LF_FACESIZE/2; i++, ptr++) {
        two_chars = *ptr;
        pwc = (WCHAR *)&two_chars;
        if (!*pwc) break;
        *pwc = towupper(*pwc);
        pwc++;
        *pwc = towupper(*pwc);
        hash ^= two_chars;
        if (!*pwc) break;
    }
    return hash;
}

static int font_cache_cmp( const struct cached_font *p1, const struct cached_font *p2 )
{
    int ret = p1->hash - p2->hash;
    if (!ret) ret = p1->aa_flags - p2->aa_flags;
    if (!ret) ret = memcmp( &p1->xform, &p2->xform, sizeof(p1->xform) );
    if (!ret) ret = memcmp( &p1->lf, &p2->lf, FIELD_OFFSET( LOGFONTW, lfFaceName ));
    if (!ret) ret = wcsicmp( p1->lf.lfFaceName, p2->lf.lfFaceName );
    return ret;
}

static struct cached_font *add_cached_font( DC *dc, HFONT hfont, UINT aa_flags )
{
    struct cached_font font, *ptr, *last_unused = NULL;
    UINT i = 0, j, k;

    NtGdiExtGetObjectW( hfont, sizeof(font.lf), &font.lf );
    font.xform = dc->xformWorld2Vport;
    font.xform.eDx = font.xform.eDy = 0;  /* unused, would break hashing */
    if (dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        font.lf.lfOrientation = font.lf.lfEscapement;
        if (font.xform.eM11 * font.xform.eM22 < 0)
            font.lf.lfOrientation = -font.lf.lfOrientation;
    }
    font.lf.lfWidth = abs( font.lf.lfWidth );
    font.aa_flags = aa_flags;
    font.hash = font_cache_hash( &font );

    pthread_mutex_lock( &font_cache_lock );
    LIST_FOR_EACH_ENTRY( ptr, &font_cache, struct cached_font, entry )
    {
        if (!font_cache_cmp( &font, ptr ))
        {
            InterlockedIncrement( &ptr->ref );
            list_remove( &ptr->entry );
            goto done;
        }
        if (!ptr->ref)
        {
            i++;
            last_unused = ptr;
        }
    }

    if (i > 5)  /* keep at least 5 of the most-recently used fonts around */
    {
        ptr = last_unused;
        for (i = 0; i < GLYPH_NBTYPES; i++)
        {
            for (j = 0; j < GLYPH_CACHE_PAGES; j++)
            {
                if (!ptr->glyphs[i][j]) continue;
                for (k = 0; k < GLYPH_CACHE_PAGE_SIZE; k++)
                    free( ptr->glyphs[i][j][k] );
                free( ptr->glyphs[i][j] );
            }
        }
        list_remove( &ptr->entry );
    }
    else if (!(ptr = malloc( sizeof(*ptr) )))
    {
        pthread_mutex_unlock( &font_cache_lock );
        return NULL;
    }

    *ptr = font;
    ptr->ref = 1;
    memset( ptr->glyphs, 0, sizeof(ptr->glyphs) );
done:
    list_add_head( &font_cache, &ptr->entry );
    pthread_mutex_unlock( &font_cache_lock );
    TRACE( "%d %s -> %p\n", (int)ptr->lf.lfHeight, debugstr_w(ptr->lf.lfFaceName), ptr );
    return ptr;
}

void release_cached_font( struct cached_font *font )
{
    if (font) InterlockedDecrement( &font->ref );
}

static struct cached_glyph *add_cached_glyph( struct cached_font *font, UINT index, UINT flags,
                                              struct cached_glyph *glyph )
{
    struct cached_glyph *ret;
    enum glyph_type type = (flags & ETO_GLYPH_INDEX) ? GLYPH_INDEX : GLYPH_WCHAR;
    UINT page = index / GLYPH_CACHE_PAGE_SIZE;
    UINT entry = index % GLYPH_CACHE_PAGE_SIZE;

    if (!font->glyphs[type][page])
    {
        struct cached_glyph **ptr;

        ptr = calloc( 1, GLYPH_CACHE_PAGE_SIZE * sizeof(*ptr) );
        if (!ptr)
        {
            free( glyph );
            return NULL;
        }
        if (InterlockedCompareExchangePointer( (void **)&font->glyphs[type][page], ptr, NULL ))
            free( ptr );
    }
    ret = InterlockedCompareExchangePointer( (void **)&font->glyphs[type][page][entry], glyph, NULL );
    if (!ret) ret = glyph;
    else free( glyph );
    return ret;
}

static struct cached_glyph *get_cached_glyph( struct cached_font *font, UINT index, UINT flags )
{
    enum glyph_type type = (flags & ETO_GLYPH_INDEX) ? GLYPH_INDEX : GLYPH_WCHAR;
    UINT page = index / GLYPH_CACHE_PAGE_SIZE;

    if (!font->glyphs[type][page]) return NULL;
    return font->glyphs[type][page][index % GLYPH_CACHE_PAGE_SIZE];
}

/**********************************************************************
 *                 get_text_bkgnd_masks
 *
 * See the comment above get_pen_bkgnd_masks
 */
static inline void get_text_bkgnd_masks( DC *dc, const dib_info *dib, rop_mask *mask )
{
    COLORREF bg = dc->attr->background_color;

    mask->and = 0;

    if (dib->bit_count != 1)
        mask->xor = get_pixel_color( dc, dib, bg, FALSE );
    else
    {
        COLORREF fg = dc->attr->text_color;
        mask->xor = get_pixel_color( dc, dib, fg, TRUE );
        if (fg != bg) mask->xor = ~mask->xor;
    }
}

static void draw_glyph( dib_info *dib, int x, int y, const GLYPHMETRICS *metrics,
                        const dib_info *glyph_dib, DWORD text_color,
                        const struct font_intensities *intensity,
                        const struct clipped_rects *clipped_rects, RECT *bounds )
{
    int i;
    RECT rect, clipped_rect;
    POINT src_origin;

    rect.left   = x          + metrics->gmptGlyphOrigin.x;
    rect.top    = y          - metrics->gmptGlyphOrigin.y;
    rect.right  = rect.left  + metrics->gmBlackBoxX;
    rect.bottom = rect.top   + metrics->gmBlackBoxY;
    if (bounds) add_bounds_rect( bounds, &rect );

    for (i = 0; i < clipped_rects->count; i++)
    {
        if (intersect_rect( &clipped_rect, &rect, clipped_rects->rects + i ))
        {
            src_origin.x = clipped_rect.left - rect.left;
            src_origin.y = clipped_rect.top  - rect.top;

            if (glyph_dib->bit_count == 32)
                dib->funcs->draw_subpixel_glyph( dib, &clipped_rect, glyph_dib, &src_origin,
                                                 text_color, intensity->gamma_ramp );
            else
                dib->funcs->draw_glyph( dib, &clipped_rect, glyph_dib, &src_origin,
                                        text_color, intensity->ranges );
        }
    }
}

static int get_glyph_depth( UINT aa_flags )
{
    switch (aa_flags)
    {
    case GGO_BITMAP: /* we'll convert non-antialiased 1-bpp bitmaps to 8-bpp */
    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    case WINE_GGO_GRAY16_BITMAP: return 8;

    case WINE_GGO_HRGB_BITMAP:
    case WINE_GGO_HBGR_BITMAP:
    case WINE_GGO_VRGB_BITMAP:
    case WINE_GGO_VBGR_BITMAP: return 32;

    default:
        ERR("Unexpected flags %08x\n", aa_flags);
        return 0;
    }
}

static const BYTE masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static const int padding[4] = {0, 3, 2, 1};

/***********************************************************************
 *         cache_glyph_bitmap
 *
 * Retrieve a 17-level bitmap for the appropriate glyph.
 *
 * For non-antialiased bitmaps convert them to the 17-level format
 * using only values 0 or 16.
 */
static struct cached_glyph *cache_glyph_bitmap( DC *dc, struct cached_font *font, UINT index, UINT flags )
{
    UINT ggo_flags = font->aa_flags;
    static const MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };
    UINT indices[3] = {0, 0, 0x20};
    int i, x, y;
    DWORD ret, size;
    BYTE *dst, *src;
    int pad = 0, stride, bit_count;
    GLYPHMETRICS metrics;
    struct cached_glyph *glyph;

    if (flags & ETO_GLYPH_INDEX) ggo_flags |= GGO_GLYPH_INDEX;
    indices[0] = index;
    for (i = 0; i < ARRAY_SIZE( indices ); i++)
    {
        index = indices[i];
        ret = NtGdiGetGlyphOutline( dc->hSelf, index, ggo_flags, &metrics, 0, NULL,
                                    &identity, FALSE );
        if (ret != GDI_ERROR) break;
    }
    if (ret == GDI_ERROR) return NULL;
    if (!ret) metrics.gmBlackBoxX = metrics.gmBlackBoxY = 0; /* empty glyph */

    bit_count = get_glyph_depth( font->aa_flags );
    stride = get_dib_stride( metrics.gmBlackBoxX, bit_count );
    size = metrics.gmBlackBoxY * stride;
    glyph = malloc( FIELD_OFFSET( struct cached_glyph, bits[size] ));
    if (!glyph) return NULL;
    if (!size) goto done;  /* empty glyph */

    if (bit_count == 8) pad = padding[ metrics.gmBlackBoxX % 4 ];

    ret = NtGdiGetGlyphOutline( dc->hSelf, index, ggo_flags, &metrics, size, glyph->bits,
                                &identity, FALSE );
    if (ret == GDI_ERROR)
    {
        free( glyph );
        return NULL;
    }
    assert( ret <= size );
    if (font->aa_flags == GGO_BITMAP)
    {
        for (y = metrics.gmBlackBoxY - 1; y >= 0; y--)
        {
            src = glyph->bits + y * get_dib_stride( metrics.gmBlackBoxX, 1 );
            dst = glyph->bits + y * stride;

            if (pad) memset( dst + metrics.gmBlackBoxX, 0, pad );

            for (x = metrics.gmBlackBoxX - 1; x >= 0; x--)
                dst[x] = (src[x / 8] & masks[x % 8]) ? 0x10 : 0;
        }
    }
    else if (pad)
    {
        for (y = 0, dst = glyph->bits; y < metrics.gmBlackBoxY; y++, dst += stride)
            memset( dst + metrics.gmBlackBoxX, 0, pad );
    }

done:
    glyph->metrics = metrics;
    return add_cached_glyph( font, index, flags, glyph );
}

static void render_string( DC *dc, dib_info *dib, struct cached_font *font, INT x, INT y,
                           UINT flags, const WCHAR *str, UINT count, const INT *dx,
                           const struct clipped_rects *clipped_rects, RECT *bounds )
{
    UINT i;
    struct cached_glyph *glyph;
    dib_info glyph_dib;
    DWORD text_color;
    struct font_intensities intensity;

    glyph_dib.bit_count    = get_glyph_depth( font->aa_flags );
    glyph_dib.rect.left    = 0;
    glyph_dib.rect.top     = 0;
    glyph_dib.bits.is_copy = FALSE;
    glyph_dib.bits.free    = NULL;

    text_color = get_pixel_color( dc, dib, dc->attr->text_color, TRUE );

    if (glyph_dib.bit_count == 32)
        intensity.gamma_ramp = dc->font_gamma_ramp;
    else
        get_aa_ranges( dib->funcs->pixel_to_colorref( dib, text_color ), intensity.ranges );

    for (i = 0; i < count; i++)
    {
        if (!(glyph = get_cached_glyph( font, str[i], flags )) &&
            !(glyph = cache_glyph_bitmap( dc, font, str[i], flags ))) continue;

        glyph_dib.width       = glyph->metrics.gmBlackBoxX;
        glyph_dib.height      = glyph->metrics.gmBlackBoxY;
        glyph_dib.rect.right  = glyph->metrics.gmBlackBoxX;
        glyph_dib.rect.bottom = glyph->metrics.gmBlackBoxY;
        glyph_dib.stride      = get_dib_stride( glyph->metrics.gmBlackBoxX, glyph_dib.bit_count );
        glyph_dib.bits.ptr    = glyph->bits;

        draw_glyph( dib, x, y, &glyph->metrics, &glyph_dib, text_color, &intensity, clipped_rects, bounds );

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
            x += glyph->metrics.gmCellIncX;
            y += glyph->metrics.gmCellIncY;
        }
    }
}

BOOL render_aa_text_bitmapinfo( DC *dc, BITMAPINFO *info, struct gdi_image_bits *bits,
                                struct bitblt_coords *src, INT x, INT y, UINT flags,
                                UINT aa_flags, LPCWSTR str, UINT count, const INT *dx )
{
    dib_info dib;
    struct clipped_rects visrect;
    struct cached_font *font;

    assert( info->bmiHeader.biBitCount > 8 ); /* mono and indexed formats don't support anti-aliasing */

    init_dib_info_from_bitmapinfo( &dib, info, bits->ptr );

    visrect.count = 1;
    visrect.rects = &src->visrect;

    if (flags & ETO_OPAQUE)
    {
        rop_mask bkgnd_color;
        get_text_bkgnd_masks( dc, &dib, &bkgnd_color );
        dib.funcs->solid_rects( &dib, 1, &src->visrect, bkgnd_color.and, bkgnd_color.xor );
    }

    if (!(font = add_cached_font( dc, dc->hFont, aa_flags ))) return FALSE;

    render_string( dc, &dib, font, x, y, flags, str, count, dx, &visrect, NULL );
    release_cached_font( font );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_ExtTextOut
 */
BOOL dibdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                        const RECT *rect, LPCWSTR str, UINT count, const INT *dx )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DC *dc = get_physdev_dc( dev );
    struct clipped_rects clipped_rects;
    RECT bounds;

    if (!pdev->font) return FALSE;

    init_clipped_rects( &clipped_rects );
    reset_bounds( &bounds );

    if (flags & ETO_OPAQUE)
    {
        rop_mask bkgnd_color;
        get_text_bkgnd_masks( dc, &pdev->dib, &bkgnd_color );
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

    render_string( dc, &pdev->dib, pdev->font, x, y, flags, str, count, dx,
                   &clipped_rects, &bounds );

done:
    add_clipped_bounds( pdev, &bounds, pdev->clip );
    free_clipped_rects( &clipped_rects );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_SelectFont
 */
HFONT dibdrv_SelectFont( PHYSDEV dev, HFONT font, UINT *aa_flags )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DC *dc = get_physdev_dc( dev );
    HFONT ret;

    if (pdev->dib.bit_count <= 8) *aa_flags = GGO_BITMAP;  /* no anti-aliasing on <= 8bpp */

    dev = GET_NEXT_PHYSDEV( dev, pSelectFont );
    ret = dev->funcs->pSelectFont( dev, font, aa_flags );
    if (ret)
    {
        struct cached_font *prev = pdev->font;
        pdev->font = add_cached_font( dc, font, *aa_flags ? *aa_flags : GGO_BITMAP );
        release_cached_font( prev );
    }
    return ret;
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
    if (clip && !NtGdiPtInRegion( clip, x, y )) return FALSE;

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
            if (next.left != next.right && !NtGdiPtInRegion( rgn, next.left, next.top ))
                fill_row( dib, clip, &next, pixel, type, rgn );
            next.left = ++next.right;
        }
    }
    if (next.left != next.right && !NtGdiPtInRegion( rgn, next.left, next.top ))
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
    DC *dc = get_physdev_dc( dev );
    DWORD pixel = get_pixel_color( dc, &pdev->dib, color, FALSE );
    RECT row;
    HRGN rgn;

    TRACE( "(%p, %d, %d, %s, %d)\n", pdev, x, y, debugstr_color(color), type );

    if (x < 0 || x >= pdev->dib.rect.right - pdev->dib.rect.left ||
        y < 0 || y >= pdev->dib.rect.bottom - pdev->dib.rect.top) return FALSE;

    if (!is_interior( &pdev->dib, pdev->clip, x, y, pixel, type )) return FALSE;

    if (!(rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 ))) return FALSE;
    row.left = x;
    row.right = x + 1;
    row.top = y;
    row.bottom = y + 1;

    fill_row( &pdev->dib, pdev->clip, &row, pixel, type, rgn );

    add_clipped_bounds( pdev, NULL, rgn );
    brush_region( pdev, rgn );

    NtGdiDeleteObjectApp( rgn );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_FillPath
 */
BOOL dibdrv_FillPath( PHYSDEV dev )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );

    return stroke_and_fill_path( pdev, FALSE, TRUE );
}

/***********************************************************************
 *           dibdrv_GetNearestColor
 */
COLORREF dibdrv_GetNearestColor( PHYSDEV dev, COLORREF color )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    DC *dc = get_physdev_dc( dev );
    DWORD pixel;

    TRACE( "(%p, %s)\n", dev, debugstr_color(color) );

    pixel = get_pixel_color( dc, &pdev->dib, color, FALSE );
    return pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );
}

/***********************************************************************
 *           dibdrv_GetPixel
 */
COLORREF dibdrv_GetPixel( PHYSDEV dev, INT x, INT y )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );
    DC *dc = get_physdev_dc( dev );
    POINT pt;
    RECT rect;
    DWORD pixel;

    TRACE( "(%p, %d, %d)\n", dev, x, y );

    pt.x = x;
    pt.y = y;
    lp_to_dp( dc, &pt, 1 );
    rect.left = pt.x;
    rect.top =  pt.y;
    rect.right = rect.left + 1;
    rect.bottom = rect.top + 1;
    if (!clip_rect_to_dib( &pdev->dib, &rect )) return CLR_INVALID;

    pixel = pdev->dib.funcs->get_pixel( &pdev->dib, pt.x, pt.y );
    return pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );
}

/***********************************************************************
 *           dibdrv_LineTo
 */
BOOL dibdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DC *dc = get_physdev_dc( dev );
    POINT pts[2];
    HRGN region = 0;
    BOOL ret;

    pts[0] = dc->attr->cur_pos;
    pts[1].x = x;
    pts[1].y = y;

    lp_to_dp(dc, pts, 2);

    if (pdev->pen_uses_region && !(region = NtGdiCreateRectRgn( 0, 0, 0, 0 ))) return FALSE;

    reset_dash_origin(pdev);

    ret = pdev->pen_lines(pdev, 2, pts, FALSE, region);
    add_pen_lines_bounds( pdev, 2, pts, region );

    if (region)
    {
        ret = pen_region( pdev, region );
        NtGdiDeleteObjectApp( region );
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
    DC *dc = get_physdev_dc( dev );
    int rop2 = get_rop2_from_rop( rop );
    struct clipped_rects clipped_rects;
    DWORD and = 0, xor = 0;
    BOOL ret = TRUE;

    TRACE("(%p, %d, %d, %d, %d, %06x)\n", dev, dst->x, dst->y, dst->width, dst->height, (int)rop);

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
        ret = brush->rects( pdev, brush, &pdev->dib, clipped_rects.count, clipped_rects.rects,
                            &dc->attr->brush_org, rop2 );
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
    DC *dc = get_physdev_dc( dev );

    TRACE("%p, %p\n", dev, rgn);

    reset_bounds( &bounds );

    region = get_wine_region( rgn );
    if(!region) return FALSE;

    for(i = 0; i < region->numRects; i++)
    {
        rect = get_device_rect( dc, region->rects[i].left, region->rects[i].top,
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
BOOL dibdrv_PolyPolygon( PHYSDEV dev, const POINT *pt, const INT *counts, UINT polygons )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DC *dc = get_physdev_dc( dev );
    DWORD total, i, pos;
    RECT rc;
    BOOL ret = TRUE;
    POINT pt_buf[32];
    POINT *points = pt_buf;
    HRGN outline = 0, interior = 0;

    for (i = total = 0; i < polygons; i++)
    {
        if (counts[i] < 2) return FALSE;
        total += counts[i];
    }

    if (total > ARRAY_SIZE( pt_buf ))
    {
        points = malloc( total * sizeof(*pt) );
        if (!points) return FALSE;
    }
    memcpy( points, pt, total * sizeof(*pt) );
    lp_to_dp( dc, points, total );

    if (pdev->brush.style != BS_NULL &&
        get_dib_rect( &pdev->dib, &rc ) &&
        !(interior = create_polypolygon_region( points, counts, polygons,
                                                dc->attr->poly_fill_mode, &rc )))
    {
        ret = FALSE;
        goto done;
    }

    if (pdev->pen_uses_region) outline = NtGdiCreateRectRgn( 0, 0, 0, 0 );

    /* if not using a region, paint the interior first so the outline can overlap it */
    if (interior && !outline)
    {
        ret = brush_region( pdev, interior );
        NtGdiDeleteObjectApp( interior );
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
        NtGdiCombineRgn( interior, interior, outline, RGN_DIFF );
        ret = brush_region( pdev, interior );
        NtGdiDeleteObjectApp( interior );
    }
    if (outline)
    {
        if (ret) ret = pen_region( pdev, outline );
        NtGdiDeleteObjectApp( outline );
    }

done:
    if (points != pt_buf) free( points );
    return ret;
}

/***********************************************************************
 *           dibdrv_PolyPolyline
 */
BOOL dibdrv_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DC *dc = get_physdev_dc( dev );
    DWORD total, pos, i;
    POINT pt_buf[32];
    POINT *points = pt_buf;
    BOOL ret = TRUE;
    HRGN outline = 0;

    for (i = total = 0; i < polylines; i++)
    {
        if (counts[i] < 2) return FALSE;
        total += counts[i];
    }

    if (total > ARRAY_SIZE( pt_buf ))
    {
        points = malloc( total * sizeof(*pt) );
        if (!points) return FALSE;
    }
    memcpy( points, pt, total * sizeof(*pt) );
    lp_to_dp( dc, points, total );

    if (pdev->pen_uses_region && !(outline = NtGdiCreateRectRgn( 0, 0, 0, 0 )))
    {
        ret = FALSE;
        goto done;
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
        NtGdiDeleteObjectApp( outline );
    }

done:
    if (points != pt_buf) free( points );
    return ret;
}

/***********************************************************************
 *           dibdrv_Rectangle
 */
BOOL dibdrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    DC *dc = get_physdev_dc( dev );
    RECT rect;
    POINT pts[4];
    BOOL ret;
    HRGN outline = 0;

    TRACE("(%p, %d, %d, %d, %d)\n", dev, left, top, right, bottom);

    if (dc->attr->graphics_mode == GM_ADVANCED)
    {
        const INT count = 4;
        pts[0].x = pts[3].x = left;
        pts[0].y = pts[1].y = top;
        pts[1].x = pts[2].x = right;
        pts[2].y = pts[3].y = bottom;
        return dibdrv_PolyPolygon( dev, pts, &count, 1 );
    }

    if (!get_pen_device_rect( dc, pdev, &rect, left, top, right, bottom )) return TRUE;

    if (pdev->pen_uses_region && !(outline = NtGdiCreateRectRgn( 0, 0, 0, 0 ))) return FALSE;

    rect.right--;
    rect.bottom--;
    reset_dash_origin(pdev);

    if (dc->attr->arc_direction == AD_CLOCKWISE)
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
            HRGN interior = NtGdiCreateRectRgn( rect.left, rect.top, rect.right, rect.bottom );

            NtGdiCombineRgn( interior, interior, outline, RGN_DIFF );
            brush_region( pdev, interior );
            NtGdiDeleteObjectApp( interior );
        }
        ret = pen_region( pdev, outline );
        NtGdiDeleteObjectApp( outline );
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
    DC *dc = get_physdev_dc( dev );
    RECT rect;
    POINT start, end, rect_center, *points, *top_points;
    int count, max_points;
    BOOL ret = TRUE;
    HRGN outline = 0, interior = 0;

    SetRect( &rect, 0, 0, ellipse_width, ellipse_height );
    /* We are drawing four arcs, but the number of points we will actually use
     * is exactly as many as in one ellipse arc. */
    max_points = get_arc_max_points( dc, &rect );
    points = malloc( max_points * sizeof(*points) );
    top_points = malloc( max_points * sizeof(*points) );
    if (!points || !top_points)
    {
        free( points );
        free( top_points );
        return FALSE;
    }

    if (pdev->pen_uses_region && !(outline = NtGdiCreateRectRgn( 0, 0, 0, 0 )))
    {
        free( points );
        return FALSE;
    }

    SetRect( &rect, left, top, left + ellipse_width, top + ellipse_height );
    /* Points are relative to the arc center.
     * We just need to specify any point on the vector. */
    start.x = -1;
    start.y = 0;
    end.x = 0;
    end.y = -1;
    count = get_arc_points( dc, AD_CLOCKWISE, &rect, start, end, top_points );

    SetRect( &rect, right - ellipse_width, top, right, top + ellipse_height );
    start.x = 0;
    start.y = -1;
    end.x = 1;
    end.y = 0;
    count += get_arc_points( dc, AD_CLOCKWISE, &rect, start, end, top_points + count );

    if (count * 2 > max_points)
        ERR( "point count %u * 2 exceeds max points %u\n", count, max_points );

    rect_center.x = (left + right) / 2;
    rect_center.y = (top + bottom) / 2;
    lp_to_dp( dc, &rect_center, 1 );

    if (dc->attr->arc_direction == AD_CLOCKWISE)
    {
        for (unsigned int i = 0; i < count; ++i)
        {
            points[i].x = top_points[i].x;
            points[i].y = top_points[i].y;
            points[count + i].x = rect_center.x * 2 - points[i].x;
            points[count + i].y = rect_center.y * 2 - points[i].y;
        }
    }
    else
    {
        for (unsigned int i = 0; i < count; ++i)
        {
            points[i].x = top_points[count - 1 - i].x;
            points[i].y = top_points[count - 1 - i].y;
            points[count + i].x = rect_center.x * 2 - points[i].x;
            points[count + i].y = rect_center.y * 2 - points[i].y;
        }
    }

    free( top_points );

    count *= 2;

    if (pdev->brush.style != BS_NULL &&
        get_dib_rect( &pdev->dib, &rect ) &&
        !(interior = create_polypolygon_region( points, &count, 1, WINDING, &rect )))
    {
        free( points );
        if (outline) NtGdiDeleteObjectApp( outline );
        return FALSE;
    }

    /* if not using a region, paint the interior first so the outline can overlap it */
    if (interior && !outline)
    {
        ret = brush_region( pdev, interior );
        NtGdiDeleteObjectApp( interior );
        interior = 0;
    }

    reset_dash_origin( pdev );
    pdev->pen_lines( pdev, count, points, TRUE, outline );
    add_pen_lines_bounds( pdev, count, points, outline );

    if (interior)
    {
        NtGdiCombineRgn( interior, interior, outline, RGN_DIFF );
        ret = brush_region( pdev, interior );
        NtGdiDeleteObjectApp( interior );
    }
    if (outline)
    {
        if (ret) ret = pen_region( pdev, outline );
        NtGdiDeleteObjectApp( outline );
    }
    free( points );
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
    DC *dc = get_physdev_dc( dev );
    struct clipped_rects clipped_rects;
    RECT rect;
    POINT pt;
    DWORD pixel;

    TRACE( "(%p, %d, %d, %s)\n", dev, x, y, debugstr_color(color) );

    pt.x = x;
    pt.y = y;
    lp_to_dp( dc, &pt, 1 );
    rect.left = pt.x;
    rect.top =  pt.y;
    rect.right = rect.left + 1;
    rect.bottom = rect.top + 1;
    add_clipped_bounds( pdev, &rect, pdev->clip );

    /* SetPixel doesn't do the 1bpp massaging like other fg colors */
    pixel = get_pixel_color( dc, &pdev->dib, color, FALSE );
    color = pdev->dib.funcs->pixel_to_colorref( &pdev->dib, pixel );

    if (!get_clipped_rects( &pdev->dib, &rect, pdev->clip, &clipped_rects )) return color;
    fill_with_pixel( dc, &pdev->dib, pixel, clipped_rects.count, clipped_rects.rects, dc->attr->rop_mode );
    free_clipped_rects( &clipped_rects );
    return color;
}

/***********************************************************************
 *           dibdrv_StrokeAndFillPath
 */
BOOL dibdrv_StrokeAndFillPath( PHYSDEV dev )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );

    return stroke_and_fill_path( pdev, TRUE, TRUE );
}

/***********************************************************************
 *           dibdrv_StrokePath
 */
BOOL dibdrv_StrokePath( PHYSDEV dev )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );

    return stroke_and_fill_path( pdev, TRUE, FALSE );
}
