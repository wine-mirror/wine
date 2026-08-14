/*
 * DIB driver initialization and DC functions.
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

#include "ntgdi_private.h"
#include "dibdrv.h"

#include "wine/unixlib.h"
#include "wine/wgl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

static const DWORD bit_fields_888[3] = {0xff0000, 0x00ff00, 0x0000ff};
static const DWORD bit_fields_555[3] = {0x7c00, 0x03e0, 0x001f};

static void calc_shift_and_len(DWORD mask, int *shift, int *len)
{
    int s, l;

    if(!mask)
    {
        *shift = *len = 0;
        return;
    }

    s = 0;
    while ((mask & 1) == 0)
    {
        mask >>= 1;
        s++;
    }
    l = 0;
    while ((mask & 1) == 1)
    {
        mask >>= 1;
        l++;
    }
    *shift = s;
    *len = l;
}

static void init_bit_fields(dib_info *dib, const DWORD *bit_fields)
{
    dib->red_mask    = bit_fields[0];
    dib->green_mask  = bit_fields[1];
    dib->blue_mask   = bit_fields[2];
    calc_shift_and_len(dib->red_mask,   &dib->red_shift,   &dib->red_len);
    calc_shift_and_len(dib->green_mask, &dib->green_shift, &dib->green_len);
    calc_shift_and_len(dib->blue_mask,  &dib->blue_shift,  &dib->blue_len);
}

static void init_dib_info(dib_info *dib, const BITMAPINFOHEADER *bi, int stride,
                          const DWORD *bit_fields, const RGBQUAD *color_table, void *bits)
{
    dib->bit_count    = bi->biBitCount;
    dib->width        = bi->biWidth;
    dib->height       = bi->biHeight;
    dib->rect.left    = 0;
    dib->rect.top     = 0;
    dib->rect.right   = bi->biWidth;
    dib->rect.bottom  = abs( bi->biHeight );
    dib->compression  = bi->biCompression;
    dib->stride       = stride;
    dib->bits.ptr     = bits;
    dib->bits.is_copy = FALSE;
    dib->bits.free    = NULL;
    dib->bits.param   = NULL;

    if(dib->height < 0) /* top-down */
    {
        dib->height = -dib->height;
    }
    else /* bottom-up */
    {
        /* bits always points to the top-left corner and the stride is -ve */
        dib->bits.ptr = (BYTE*)dib->bits.ptr + (dib->height - 1) * dib->stride;
        dib->stride  = -dib->stride;
    }

    dib->funcs = &funcs_null;

    switch(dib->bit_count)
    {
    case 32:
        if(bi->biCompression == BI_RGB)
            bit_fields = bit_fields_888;

        init_bit_fields(dib, bit_fields);

        if(dib->red_mask == 0xff0000 && dib->green_mask == 0x00ff00 && dib->blue_mask == 0x0000ff)
            dib->funcs = &funcs_8888;
        else
            dib->funcs = &funcs_32;
        break;

    case 24:
        dib->funcs = &funcs_24;
        break;

    case 16:
        if(bi->biCompression == BI_RGB)
            bit_fields = bit_fields_555;

        init_bit_fields(dib, bit_fields);

        if(dib->red_mask == 0x7c00 && dib->green_mask == 0x03e0 && dib->blue_mask == 0x001f)
            dib->funcs = &funcs_555;
        else
            dib->funcs = &funcs_16;
        break;

    case 8:
        dib->funcs = &funcs_8;
        break;

    case 4:
        dib->funcs = &funcs_4;
        break;

    case 1:
        dib->funcs = &funcs_1;
        break;
    }

    if (color_table && bi->biClrUsed)
    {
        dib->color_table = color_table;
        dib->color_table_size = bi->biClrUsed;
    }
    else
    {
        dib->color_table = NULL;
        dib->color_table_size = 0;
    }
}

void init_dib_info_from_bitmapinfo(dib_info *dib, const BITMAPINFO *info, void *bits)
{
    int width_bytes = get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
    if (info->bmiHeader.biSizeImage)
        width_bytes = info->bmiHeader.biSizeImage / abs( info->bmiHeader.biHeight );
    init_dib_info( dib, &info->bmiHeader, width_bytes,
                   (const DWORD *)info->bmiColors, info->bmiColors, bits );
}

BOOL init_dib_info_from_bitmapobj(dib_info *dib, BITMAPOBJ *bmp)
{
    if (!is_bitmapobj_dib( bmp ))
    {
        BITMAPINFO info;

        get_ddb_bitmapinfo( bmp, &info );
        init_dib_info_from_bitmapinfo( dib, &info, bmp->dib.dsBm.bmBits );
    }
    else init_dib_info( dib, &bmp->dib.dsBmih, bmp->dib.dsBm.bmWidthBytes,
                        bmp->dib.dsBitfields, bmp->color_table, bmp->dib.dsBm.bmBits );
    return TRUE;
}

static void clear_dib_info(dib_info *dib)
{
    dib->bits.ptr    = NULL;
    dib->bits.free   = NULL;
    dib->bits.param  = NULL;
}

/**********************************************************************
 *      free_dib_info
 *
 * Free the resources associated with a dib and optionally the bits
 */
void free_dib_info(dib_info *dib)
{
    if (dib->bits.free) dib->bits.free( &dib->bits );
    clear_dib_info( dib );
}

void copy_dib_color_info(dib_info *dst, const dib_info *src)
{
    dst->bit_count        = src->bit_count;
    dst->red_mask         = src->red_mask;
    dst->green_mask       = src->green_mask;
    dst->blue_mask        = src->blue_mask;
    dst->red_len          = src->red_len;
    dst->green_len        = src->green_len;
    dst->blue_len         = src->blue_len;
    dst->red_shift        = src->red_shift;
    dst->green_shift      = src->green_shift;
    dst->blue_shift       = src->blue_shift;
    dst->funcs            = src->funcs;
    dst->color_table_size = src->color_table_size;
    dst->color_table      = src->color_table;
}

DWORD convert_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                          const BITMAPINFO *dst_info, void *dst_bits )
{
    dib_info src_dib, dst_dib;
    DWORD ret;

    init_dib_info_from_bitmapinfo( &src_dib, src_info, src_bits );
    init_dib_info_from_bitmapinfo( &dst_dib, dst_info, dst_bits );

    __TRY
    {
        dst_dib.funcs->convert_to( &dst_dib, &src_dib, &src->visrect, FALSE );
        ret = TRUE;
    }
    __EXCEPT
    {
        WARN( "invalid bits pointer %p\n", src_bits );
        ret = FALSE;
    }
    __ENDTRY

    if(!ret) return ERROR_BAD_FORMAT;

    /* update coordinates, the destination rectangle is always stored at 0,0 */
    src->x -= src->visrect.left;
    src->y -= src->visrect.top;
    OffsetRect( &src->visrect, -src->visrect.left, -src->visrect.top );
    return ERROR_SUCCESS;
}

int get_dib_rect( const dib_info *dib, RECT *rc )
{
    rc->left   = max( 0, -dib->rect.left );
    rc->top    = max( 0, -dib->rect.top );
    rc->right  = min( dib->rect.right, dib->width ) - dib->rect.left;
    rc->bottom = min( dib->rect.bottom, dib->height ) - dib->rect.top;
    return !IsRectEmpty( rc );
}

int clip_rect_to_dib( const dib_info *dib, RECT *rc )
{
    RECT rect;

    return get_dib_rect( dib, &rect ) && intersect_rect( rc, &rect, rc );
}

int get_clipped_rects( const dib_info *dib, const RECT *rc, HRGN clip, struct clipped_rects *clip_rects )
{
    const WINEREGION *region;
    RECT rect, *out = clip_rects->buffer;
    int i;

    init_clipped_rects( clip_rects );

    if (!get_dib_rect( dib, &rect )) return 0;
    if (rc && !intersect_rect( &rect, &rect, rc )) return 0;

    if (!clip)
    {
        *out = rect;
        clip_rects->count = 1;
        return 1;
    }

    if (!(region = get_wine_region( clip ))) return 0;

    for (i = region_find_pt( region, rect.left, rect.top, NULL ); i < region->numRects; i++)
    {
        if (region->rects[i].top >= rect.bottom) break;
        if (!intersect_rect( out, &rect, &region->rects[i] )) continue;
        out++;
        if (out == &clip_rects->buffer[ARRAY_SIZE( clip_rects->buffer )])
        {
            clip_rects->rects = malloc( region->numRects * sizeof(RECT) );
            if (!clip_rects->rects) return 0;
            memcpy( clip_rects->rects, clip_rects->buffer, (out - clip_rects->buffer) * sizeof(RECT) );
            out = clip_rects->rects + (out - clip_rects->buffer);
        }
    }
    release_wine_region( clip );
    clip_rects->count = out - clip_rects->rects;
    return clip_rects->count;
}

void add_clipped_bounds( dibdrv_physdev *dev, const RECT *rect, HRGN clip )
{
    const WINEREGION *region;
    RECT rc;

    if (!dev->bounds) return;
    if (clip)
    {
        if (!(region = get_wine_region( clip ))) return;
        if (!rect) rc = region->extents;
        else intersect_rect( &rc, rect, &region->extents );
        release_wine_region( clip );
    }
    else rc = *rect;

    if (IsRectEmpty( &rc )) return;
    OffsetRect( &rc, dev->dib.rect.left, dev->dib.rect.top );
    add_bounds_rect( dev->bounds, &rc );
}

/**********************************************************************
 *	     dibdrv_CreateDC
 */
static BOOL dibdrv_CreateDC( PHYSDEV *dev, LPCWSTR device, LPCWSTR output, const DEVMODEW *data )
{
    dibdrv_physdev *pdev = calloc( 1, sizeof(*pdev) );

    if (!pdev) return FALSE;
    clear_dib_info(&pdev->dib);
    clear_dib_info(&pdev->brush.dib);
    clear_dib_info(&pdev->pen_brush.dib);
    push_dc_driver( dev, &pdev->dev, &dib_driver );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_DeleteDC
 */
static BOOL dibdrv_DeleteDC( PHYSDEV dev )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p)\n", dev);
    free_pattern_brush( &pdev->brush );
    free_pattern_brush( &pdev->pen_brush );
    release_cached_font( pdev->font );
    free( pdev );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_SelectBitmap
 */
static HBITMAP dibdrv_SelectBitmap( PHYSDEV dev, HBITMAP bitmap )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    BITMAPOBJ *bmp = GDI_GetObjPtr( bitmap, NTGDI_OBJ_BITMAP );
    dib_info dib;

    TRACE("(%p, %p)\n", dev, bitmap);

    if (!bmp) return 0;

    if (!init_dib_info_from_bitmapobj(&dib, bmp))
    {
        GDI_ReleaseObj( bitmap );
        return 0;
    }
    pdev->dib = dib;
    GDI_ReleaseObj( bitmap );

    return bitmap;
}

/***********************************************************************
 *           dibdrv_SetDeviceClipping
 */
static void dibdrv_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    TRACE("(%p, %p)\n", dev, rgn);

    pdev->clip = rgn;
}

/***********************************************************************
 *           dibdrv_SetBoundsRect
 */
static UINT dibdrv_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev( dev );

    if (flags & DCB_DISABLE) pdev->bounds = NULL;
    else if (flags & DCB_ENABLE) pdev->bounds = rect;
    return DCB_RESET;  /* we don't have device-specific bounds */
}

const struct gdi_dc_funcs dib_driver =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    dibdrv_AlphaBlend,                  /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    dibdrv_Arc,                         /* pArc */
    dibdrv_ArcTo,                       /* pArcTo */
    NULL,                               /* pBeginPath */
    dibdrv_BlendImage,                  /* pBlendImage */
    dibdrv_Chord,                       /* pChord */
    NULL,                               /* pCloseFigure */
    NULL,                               /* pCreateCompatibleDC */
    dibdrv_CreateDC,                    /* pCreateDC */
    dibdrv_DeleteDC,                    /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    dibdrv_Ellipse,                     /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pExtEscape */
    dibdrv_ExtFloodFill,                /* pExtFloodFill */
    dibdrv_ExtTextOut,                  /* pExtTextOut */
    dibdrv_FillPath,                    /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetCharWidthInfo */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    dibdrv_GetImage,                    /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    dibdrv_GetNearestColor,             /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    dibdrv_GetPixel,                    /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    dibdrv_GradientFill,                /* pGradientFill */
    NULL,                               /* pInvertRgn */
    dibdrv_LineTo,                      /* pLineTo */
    NULL,                               /* pMoveTo */
    dibdrv_PaintRgn,                    /* pPaintRgn */
    dibdrv_PatBlt,                      /* pPatBlt */
    dibdrv_Pie,                         /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    dibdrv_PolyPolygon,                 /* pPolyPolygon */
    dibdrv_PolyPolyline,                /* pPolyPolyline */
    NULL,                               /* pPolylineTo */
    dibdrv_PutImage,                    /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    dibdrv_Rectangle,                   /* pRectangle */
    NULL,                               /* pResetDC */
    dibdrv_RoundRect,                   /* pRoundRect */
    dibdrv_SelectBitmap,                /* pSelectBitmap */
    dibdrv_SelectBrush,                 /* pSelectBrush */
    dibdrv_SelectFont,                  /* pSelectFont */
    dibdrv_SelectPen,                   /* pSelectPen */
    NULL,                               /* pSetBkColor */
    dibdrv_SetBoundsRect,               /* pSetBoundsRect */
    dibdrv_SetDCBrushColor,             /* pSetDCBrushColor */
    dibdrv_SetDCPenColor,               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    dibdrv_SetDeviceClipping,           /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    dibdrv_SetPixel,                    /* pSetPixel */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    dibdrv_StretchBlt,                  /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    dibdrv_StrokeAndFillPath,           /* pStrokeAndFillPath */
    dibdrv_StrokePath,                  /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    GDI_PRIORITY_DIB_DRV                /* priority */
};


/***********************************************************************
 * Driver for window surfaces.
 *
 * It uses the DIB engine but needs extra locking since multiple DCs
 * can paint to the same window.
 */

#define FLUSH_PERIOD 50  /* time in ms since drawing started for forcing a surface flush */

struct windrv_physdev
{
    struct gdi_physdev     dev;
    struct dibdrv_physdev *dibdrv;
    struct window_surface *surface;
    UINT lock_count;
};

static const struct gdi_dc_funcs window_driver;

static inline struct windrv_physdev *get_windrv_physdev( PHYSDEV dev )
{
    return (struct windrv_physdev *)dev;
}

/* gdi_lock should not be locked */
static inline void lock_surface( struct windrv_physdev *dev )
{
    struct window_surface *surface = dev->surface;

    if (!dev->lock_count++)
    {
        window_surface_lock( surface );
        if (IsRectEmpty( dev->dibdrv->bounds ) || !surface->draw_start_ticks)
            surface->draw_start_ticks = NtGetTickCount();
    }
}

static inline void unlock_surface( struct windrv_physdev *dev )
{
    struct window_surface *surface = dev->surface;

    if (!--dev->lock_count)
    {
        DWORD ticks = NtGetTickCount() - surface->draw_start_ticks;
        window_surface_unlock( surface );
        if (ticks > FLUSH_PERIOD) window_surface_flush( dev->surface );
    }
}

static inline void lock_surfaces( struct windrv_physdev *dst_dev, struct windrv_physdev *src_dev )
{
    lock_surface( dst_dev );
    if (src_dev && !src_dev->lock_count++ && dst_dev->surface != src_dev->surface)
        window_surface_lock( src_dev->surface );
}

static inline void unlock_surfaces( struct windrv_physdev *dst_dev, struct windrv_physdev *src_dev )
{
    if (src_dev && !--src_dev->lock_count && dst_dev->surface != src_dev->surface)
        window_surface_unlock( src_dev->surface );
    unlock_surface( dst_dev );
}

static void unlock_windrv_bits( struct gdi_image_bits *bits )
{
    unlock_surface( bits->param );
}

void dibdrv_set_window_surface( DC *dc, struct window_surface *surface )
{
    PHYSDEV windev;
    struct windrv_physdev *physdev;
    struct dibdrv_physdev *dibdrv;
    BITMAPOBJ *bmp;

    TRACE( "%p %p\n", dc->hSelf, surface );

    windev = pop_dc_driver( dc, &window_driver );

    if (surface)
    {
        if (surface->alpha_mask) surface = &dummy_surface;
        if (windev) push_dc_driver( &dc->physDev, windev, windev->funcs );
        else
        {
            if (!window_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL )) return;
            windev = find_dc_driver( dc, &window_driver );
        }

        physdev = get_windrv_physdev( windev );
        window_surface_add_ref( surface );
        if (physdev->surface) window_surface_release( physdev->surface );
        physdev->surface = surface;

        dibdrv = physdev->dibdrv;
        if (!(bmp = GDI_GetObjPtr( surface->color_bitmap, NTGDI_OBJ_BITMAP )))
        {
            static BITMAPINFO info = {.bmiHeader = {.biSize = sizeof(BITMAPINFOHEADER), .biWidth = 1, .biHeight = 1,
                                                    .biPlanes = 1, .biBitCount = 32, .biCompression = BI_RGB}};
            static DWORD bits;

            init_dib_info_from_bitmapinfo( &dibdrv->dib, &info, &bits );
        }
        else
        {
            init_dib_info_from_bitmapobj( &dibdrv->dib, bmp );
            GDI_ReleaseObj( surface->color_bitmap );
        }
        dibdrv->dib.rect = dc->attr->vis_rect;
        OffsetRect( &dibdrv->dib.rect, -dc->device_rect.left, -dc->device_rect.top );
        dibdrv->bounds = &surface->bounds;
        DC_InitDC( dc );
    }
    else if (windev)
    {
        dib_driver.pDeleteDC( pop_dc_driver( dc, &dib_driver ));
        windev->funcs->pDeleteDC( windev );
        DC_InitDC( dc );
    }
}

static BOOL windrv_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                               PHYSDEV src_dev, struct bitblt_coords *src, BLENDFUNCTION func )
{
    struct windrv_physdev *dst_physdev = get_windrv_physdev( dst_dev ), *src_physdev;
    BOOL ret;

    if (src_dev->funcs == &window_driver) src_physdev = get_windrv_physdev( src_dev );
    else src_physdev = NULL;

    lock_surfaces( dst_physdev, src_physdev );
    dst_dev = GET_NEXT_PHYSDEV( dst_dev, pAlphaBlend );
    ret = dst_dev->funcs->pAlphaBlend( dst_dev, dst, src_dev, src, func );
    unlock_surfaces( dst_physdev, src_physdev );
    return ret;
}

static BOOL windrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pArc );
    ret = dev->funcs->pArc( dev, left, top, right, bottom, xstart, ystart, xend, yend );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pArcTo );
    ret = dev->funcs->pArcTo( dev, left, top, right, bottom, xstart, ystart, xend, yend );
    unlock_surface( physdev );
    return ret;
}

static DWORD windrv_BlendImage( PHYSDEV dev, BITMAPINFO *info, const struct gdi_image_bits *bits,
                                struct bitblt_coords *src, struct bitblt_coords *dst, BLENDFUNCTION blend )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    DWORD ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pBlendImage );
    ret = dev->funcs->pBlendImage( dev, info, bits, src, dst, blend );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pChord );
    ret = dev->funcs->pChord( dev, left, top, right, bottom, xstart, ystart, xend, yend );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_CreateDC( PHYSDEV *dev, LPCWSTR device, LPCWSTR output, const DEVMODEW *devmode )
{
    struct windrv_physdev *physdev = calloc( 1, sizeof(*physdev) );
    PHYSDEV dibdrv;
    DC *dc;

    if (!physdev) return FALSE;

    if (!dib_driver.pCreateDC( dev, NULL, NULL, NULL ))
    {
        free( physdev );
        return FALSE;
    }
    dc = get_physdev_dc( *dev );
    dibdrv = find_dc_driver( dc, &dib_driver );
    physdev->dibdrv = get_dibdrv_pdev( dibdrv );
    push_dc_driver( dev, &physdev->dev, &window_driver );
    return TRUE;
}

static BOOL windrv_DeleteDC( PHYSDEV dev )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );

    window_surface_release( physdev->surface );
    free( physdev );
    return TRUE;
}

static BOOL windrv_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pEllipse );
    ret = dev->funcs->pEllipse( dev, left, top, right, bottom );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT type )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pExtFloodFill );
    ret = dev->funcs->pExtFloodFill( dev, x, y, color, type );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
                               LPCWSTR str, UINT count, const INT *dx )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pExtTextOut );
    ret = dev->funcs->pExtTextOut( dev, x, y, flags, rect, str, count, dx );
    unlock_surface( physdev );
    return ret;
}

static DWORD windrv_GetImage( PHYSDEV dev, BITMAPINFO *info,
                              struct gdi_image_bits *bits, struct bitblt_coords *src )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    DWORD ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pGetImage );
    ret = dev->funcs->pGetImage( dev, info, bits, src );

    /* don't return alpha if original surface doesn't support it */
    if (info->bmiHeader.biBitCount == 32 &&
        info->bmiHeader.biCompression == BI_RGB &&
        physdev->dibdrv->dib.compression == BI_BITFIELDS)
    {
        DWORD *colors = (DWORD *)info->bmiColors;
        colors[0] = 0xff0000;
        colors[1] = 0x00ff00;
        colors[2] = 0x0000ff;
        info->bmiHeader.biCompression = BI_BITFIELDS;
    }

    if (!bits->is_copy)
    {
        /* use the freeing callback to unlock the surface */
        assert( !bits->free );
        bits->free = unlock_windrv_bits;
        bits->param = physdev;
    }
    else unlock_surface( physdev );
    return ret;
}

static COLORREF windrv_GetPixel( PHYSDEV dev, INT x, INT y )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    COLORREF ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pGetPixel );
    ret = dev->funcs->pGetPixel( dev, x, y );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                 void * grad_array, ULONG ngrad, ULONG mode )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pGradientFill );
    ret = dev->funcs->pGradientFill( dev, vert_array, nvert, grad_array, ngrad, mode );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pLineTo );
    ret = dev->funcs->pLineTo( dev, x, y );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_PaintRgn( PHYSDEV dev, HRGN rgn )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pPaintRgn );
    ret = dev->funcs->pPaintRgn( dev, rgn );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pPatBlt );
    ret = dev->funcs->pPatBlt( dev, dst, rop );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pPie );
    ret = dev->funcs->pPie( dev, left, top, right, bottom, xstart, ystart, xend, yend );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_PolyPolygon( PHYSDEV dev, const POINT *points, const INT *counts, UINT polygons )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pPolyPolygon );
    ret = dev->funcs->pPolyPolygon( dev, points, counts, polygons );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_PolyPolyline( PHYSDEV dev, const POINT *points, const DWORD *counts, DWORD lines )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pPolyPolyline );
    ret = dev->funcs->pPolyPolyline( dev, points, counts, lines );
    unlock_surface( physdev );
    return ret;
}

static DWORD windrv_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                              const struct gdi_image_bits *bits, struct bitblt_coords *src,
                              struct bitblt_coords *dst, DWORD rop )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    DWORD ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pPutImage );
    ret = dev->funcs->pPutImage( dev, clip, info, bits, src, dst, rop );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pRectangle );
    ret = dev->funcs->pRectangle( dev, left, top, right, bottom );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                              INT ell_width, INT ell_height )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    BOOL ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pRoundRect );
    ret = dev->funcs->pRoundRect( dev, left, top, right, bottom, ell_width, ell_height );
    unlock_surface( physdev );
    return ret;
}

static UINT windrv_SetBoundsRect( PHYSDEV dev, RECT *rect, UINT flags )
{
    /* do nothing, we use the dibdrv bounds tracking for our own purpose */
    return DCB_RESET;
}

static INT windrv_SetDIBitsToDevice( PHYSDEV dev, INT x_dst, INT y_dst, DWORD cx, DWORD cy,
                                     INT x_src, INT y_src, UINT startscan, UINT lines,
                                     const void *bits, BITMAPINFO *src_info, UINT coloruse )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    INT ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pSetDIBitsToDevice );
    ret = dev->funcs->pSetDIBitsToDevice( dev, x_dst, y_dst, cx, cy,
                                          x_src, y_src, startscan, lines, bits, src_info, coloruse );
    unlock_surface( physdev );
    return ret;
}

static void windrv_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
    dev = GET_NEXT_PHYSDEV( dev, pSetDeviceClipping );
    dev->funcs->pSetDeviceClipping( dev, rgn );
    /* also forward to the graphics driver for the OpenGL case */
    if (dev->funcs == &dib_driver)
    {
        dev = GET_NEXT_PHYSDEV( dev, pSetDeviceClipping );
        dev->funcs->pSetDeviceClipping( dev, rgn );
    }
}

static COLORREF windrv_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    COLORREF ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pSetPixel );
    ret = dev->funcs->pSetPixel( dev, x, y, color );
    unlock_surface( physdev );
    return ret;
}

static BOOL windrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                               PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop )
{
    struct windrv_physdev *dst_physdev = get_windrv_physdev( dst_dev ), *src_physdev;
    BOOL ret;

    if (src_dev->funcs == &window_driver) src_physdev = get_windrv_physdev( src_dev );
    else src_physdev = NULL;

    lock_surfaces( dst_physdev, src_physdev );
    dst_dev = GET_NEXT_PHYSDEV( dst_dev, pStretchBlt );
    ret = dst_dev->funcs->pStretchBlt( dst_dev, dst, src_dev, src, rop );
    unlock_surfaces( dst_physdev, src_physdev );
    return ret;
}

static INT windrv_StretchDIBits( PHYSDEV dev, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                                 INT x_src, INT y_src, INT width_src, INT height_src, const void *bits,
                                 BITMAPINFO *src_info, UINT coloruse, DWORD rop )
{
    struct windrv_physdev *physdev = get_windrv_physdev( dev );
    INT ret;

    lock_surface( physdev );
    dev = GET_NEXT_PHYSDEV( dev, pStretchDIBits );
    ret = dev->funcs->pStretchDIBits( dev, x_dst, y_dst, width_dst, height_dst,
                                      x_src, y_src, width_src, height_src, bits, src_info, coloruse, rop );
    unlock_surface( physdev );
    return ret;
}

static const struct gdi_dc_funcs window_driver =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    windrv_AlphaBlend,                  /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    windrv_Arc,                         /* pArc */
    windrv_ArcTo,                       /* pArcTo */
    NULL,                               /* pBeginPath */
    windrv_BlendImage,                  /* pBlendImage */
    windrv_Chord,                       /* pChord */
    NULL,                               /* pCloseFigure */
    NULL,                               /* pCreateCompatibleDC */
    windrv_CreateDC,                    /* pCreateDC */
    windrv_DeleteDC,                    /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    windrv_Ellipse,                     /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pExtEscape */
    windrv_ExtFloodFill,                /* pExtFloodFill */
    windrv_ExtTextOut,                  /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetCharWidthInfo */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    windrv_GetImage,                    /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    windrv_GetPixel,                    /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    windrv_GradientFill,                /* pGradientFill */
    NULL,                               /* pInvertRgn */
    windrv_LineTo,                      /* pLineTo */
    NULL,                               /* pMoveTo */
    windrv_PaintRgn,                    /* pPaintRgn */
    windrv_PatBlt,                      /* pPatBlt */
    windrv_Pie,                         /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    windrv_PolyPolygon,                 /* pPolyPolygon */
    windrv_PolyPolyline,                /* pPolyPolyline */
    NULL,                               /* pPolylineTo */
    windrv_PutImage,                    /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    windrv_Rectangle,                   /* pRectangle */
    NULL,                               /* pResetDC */
    windrv_RoundRect,                   /* pRoundRect */
    NULL,                               /* pSelectBitmap */
    NULL,                               /* pSelectBrush */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetBkColor */
    windrv_SetBoundsRect,               /* pSetBoundsRect */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    windrv_SetDIBitsToDevice,           /* pSetDIBitsToDevice */
    windrv_SetDeviceClipping,           /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    windrv_SetPixel,                    /* pSetPixel */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    windrv_StretchBlt,                  /* pStretchBlt */
    windrv_StretchDIBits,               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    GDI_PRIORITY_DIB_DRV + 10           /* priority */
};
