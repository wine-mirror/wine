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

#include <assert.h>

#include "gdi_private.h"
#include "dibdrv.h"

#include "wine/exception.h"
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

static void init_dib_info(dib_info *dib, const BITMAPINFOHEADER *bi, const DWORD *bit_fields,
                          const RGBQUAD *color_table, void *bits)
{
    dib->bit_count    = bi->biBitCount;
    dib->width        = bi->biWidth;
    dib->height       = bi->biHeight;
    dib->rect.left    = 0;
    dib->rect.top     = 0;
    dib->rect.right   = bi->biWidth;
    dib->rect.bottom  = abs( bi->biHeight );
    dib->compression  = bi->biCompression;
    dib->stride       = get_dib_stride( dib->width, dib->bit_count );
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
    init_dib_info( dib, &info->bmiHeader, (const DWORD *)info->bmiColors, info->bmiColors, bits );
}

BOOL init_dib_info_from_bitmapobj(dib_info *dib, BITMAPOBJ *bmp)
{
    if (!is_bitmapobj_dib( bmp ))
    {
        BITMAPINFO info;

        get_ddb_bitmapinfo( bmp, &info );
        if (!bmp->dib.dsBm.bmBits)
        {
            int width_bytes = get_dib_stride( bmp->dib.dsBm.bmWidth, bmp->dib.dsBm.bmBitsPixel );
            bmp->dib.dsBm.bmBits = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                              bmp->dib.dsBm.bmHeight * width_bytes );
            if (!bmp->dib.dsBm.bmBits) return FALSE;
        }
        init_dib_info_from_bitmapinfo( dib, &info, bmp->dib.dsBm.bmBits );
    }
    else init_dib_info( dib, &bmp->dib.dsBmih, bmp->dib.dsBitfields,
                        bmp->color_table, bmp->dib.dsBm.bmBits );
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
                          const BITMAPINFO *dst_info, void *dst_bits, BOOL add_alpha )
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
    __EXCEPT_PAGE_FAULT
    {
        WARN( "invalid bits pointer %p\n", src_bits );
        ret = FALSE;
    }
    __ENDTRY

    /* We shared the color tables, so there's no need to free the dib_infos here */
    if(!ret) return ERROR_BAD_FORMAT;

    /* update coordinates, the destination rectangle is always stored at 0,0 */
    src->x -= src->visrect.left;
    src->y -= src->visrect.top;
    offset_rect( &src->visrect, -src->visrect.left, -src->visrect.top );

    if (add_alpha && dst_dib.funcs == &funcs_8888 && src_dib.funcs != &funcs_8888)
    {
        DWORD *pixel = dst_dib.bits.ptr;
        int x, y;

        for (y = src->visrect.top; y < src->visrect.bottom; y++, pixel += dst_dib.stride / 4)
            for (x = src->visrect.left; x < src->visrect.right; x++)
                pixel[x] |= 0xff000000;
    }

    return ERROR_SUCCESS;
}

int get_clipped_rects( const dib_info *dib, const RECT *rc, HRGN clip, struct clipped_rects *clip_rects )
{
    const WINEREGION *region;
    RECT rect, *out = clip_rects->buffer;
    int i;

    init_clipped_rects( clip_rects );

    rect.left   = max( 0, -dib->rect.left );
    rect.top    = max( 0, -dib->rect.top );
    rect.right  = min( dib->rect.right, dib->width ) - dib->rect.left;
    rect.bottom = min( dib->rect.bottom, dib->height ) - dib->rect.top;
    if (is_rect_empty( &rect )) return 0;
    if (rc && !intersect_rect( &rect, &rect, rc )) return 0;

    if (!clip)
    {
        *out = rect;
        clip_rects->count = 1;
        return 1;
    }

    if (!(region = get_wine_region( clip ))) return 0;

    for (i = 0; i < region->numRects; i++)
    {
        if (region->rects[i].top >= rect.bottom) break;
        if (!intersect_rect( out, &rect, &region->rects[i] )) continue;
        out++;
        if (out == &clip_rects->buffer[sizeof(clip_rects->buffer) / sizeof(RECT)])
        {
            clip_rects->rects = HeapAlloc( GetProcessHeap(), 0, region->numRects * sizeof(RECT) );
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

    if (is_rect_empty( &rc )) return;
    offset_rect( &rc, dev->dib.rect.left, dev->dib.rect.top );
    add_bounds_rect( dev->bounds, &rc );
}

/**********************************************************************
 *	     dibdrv_CreateDC
 */
static BOOL dibdrv_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                             LPCWSTR output, const DEVMODEW *data )
{
    dibdrv_physdev *pdev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pdev) );

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
    HeapFree( GetProcessHeap(), 0, pdev );
    return TRUE;
}

/***********************************************************************
 *           dibdrv_SelectBitmap
 */
static HBITMAP dibdrv_SelectBitmap( PHYSDEV dev, HBITMAP bitmap )
{
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    BITMAPOBJ *bmp = GDI_GetObjPtr( bitmap, OBJ_BITMAP );
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
    dibdrv_DescribePixelFormat,         /* pDescribePixelFormat */
    NULL,                               /* pDeviceCapabilities */
    dibdrv_Ellipse,                     /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    NULL,                               /* pExtEscape */
    dibdrv_ExtFloodFill,                /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    dibdrv_ExtTextOut,                  /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGdiRealizationInfo */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
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
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    dibdrv_LineTo,                      /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    NULL,                               /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    dibdrv_PaintRgn,                    /* pPaintRgn */
    dibdrv_PatBlt,                      /* pPatBlt */
    dibdrv_Pie,                         /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    dibdrv_PolyPolygon,                 /* pPolyPolygon */
    dibdrv_PolyPolyline,                /* pPolyPolyline */
    dibdrv_Polygon,                     /* pPolygon */
    dibdrv_Polyline,                    /* pPolyline */
    NULL,                               /* pPolylineTo */
    dibdrv_PutImage,                    /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    dibdrv_Rectangle,                   /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    dibdrv_RoundRect,                   /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    dibdrv_SelectBitmap,                /* pSelectBitmap */
    dibdrv_SelectBrush,                 /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPalette */
    dibdrv_SelectPen,                   /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    dibdrv_SetBoundsRect,               /* pSetBoundsRect */
    dibdrv_SetDCBrushColor,             /* pSetDCBrushColor */
    dibdrv_SetDCPenColor,               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    dibdrv_SetDeviceClipping,           /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    dibdrv_SetPixel,                    /* pSetPixel */
    dibdrv_SetPixelFormat,              /* pSetPixelFormat */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    dibdrv_StretchBlt,                  /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pSwapBuffers */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    dibdrv_wine_get_wgl_driver,         /* wine_get_wgl_driver */
    GDI_PRIORITY_DIB_DRV                /* priority */
};
