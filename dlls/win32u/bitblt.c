/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
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
#include <limits.h>
#include <math.h>
#include <float.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitblt);

static inline BOOL rop_uses_src( DWORD rop )
{
    return ((rop >> 2) & 0x330000) != (rop & 0x330000);
}

BOOL intersect_vis_rectangles( struct bitblt_coords *dst, struct bitblt_coords *src )
{
    RECT rect;

    /* intersect the rectangles */

    if ((src->width == dst->width) && (src->height == dst->height)) /* no stretching */
    {
        OffsetRect( &src->visrect, dst->x - src->x, dst->y - src->y );
        if (!intersect_rect( &rect, &src->visrect, &dst->visrect )) return FALSE;
        src->visrect = dst->visrect = rect;
        OffsetRect( &src->visrect, src->x - dst->x, src->y - dst->y );
    }
    else  /* stretching */
    {
        /* map source rectangle into destination coordinates */
        rect = src->visrect;
        OffsetRect( &rect,
                    -src->x - (src->width < 0 ? 1 : 0),
                    -src->y - (src->height < 0 ? 1 : 0) );
        rect.left   = rect.left * dst->width / src->width;
        rect.top    = rect.top * dst->height / src->height;
        rect.right  = rect.right * dst->width / src->width;
        rect.bottom = rect.bottom * dst->height / src->height;
        order_rect( &rect );

        /* when the source rectangle needs to flip and it doesn't fit in the source device
           area, the destination area isn't flipped. So, adjust destination coordinates */
        if (src->width < 0 && dst->width > 0 &&
            (src->x + src->width + 1 < src->visrect.left || src->x > src->visrect.right))
            dst->x += (dst->width - rect.right) - rect.left;
        else if (src->width > 0 && dst->width < 0 &&
                 (src->x < src->visrect.left || src->x + src->width > src->visrect.right))
            dst->x -= rect.right - (dst->width - rect.left);

        if (src->height < 0 && dst->height > 0 &&
            (src->y + src->height + 1 < src->visrect.top || src->y > src->visrect.bottom))
            dst->y += (dst->height - rect.bottom) - rect.top;
        else if (src->height > 0 && dst->height < 0 &&
                 (src->y < src->visrect.top || src->y + src->height > src->visrect.bottom))
            dst->y -= rect.bottom - (dst->height - rect.top);

        OffsetRect( &rect, dst->x, dst->y );

        /* avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        if (!intersect_rect( &dst->visrect, &rect, &dst->visrect )) return FALSE;

        /* map destination rectangle back to source coordinates */
        rect = dst->visrect;
        OffsetRect( &rect,
                    -dst->x - (dst->width < 0 ? 1 : 0),
                    -dst->y - (dst->height < 0 ? 1 : 0) );
        rect.left   = src->x + rect.left * src->width / dst->width;
        rect.top    = src->y + rect.top * src->height / dst->height;
        rect.right  = src->x + rect.right * src->width / dst->width;
        rect.bottom = src->y + rect.bottom * src->height / dst->height;
        order_rect( &rect );

        /* avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        if (!intersect_rect( &src->visrect, &rect, &src->visrect )) return FALSE;
    }
    return TRUE;
}

static BOOL get_vis_rectangles( DC *dc_dst, struct bitblt_coords *dst,
                                DC *dc_src, struct bitblt_coords *src )
{
    RECT rect;

    /* get the destination visible rectangle */

    rect.left   = dst->log_x;
    rect.top    = dst->log_y;
    rect.right  = dst->log_x + dst->log_width;
    rect.bottom = dst->log_y + dst->log_height;
    lp_to_dp( dc_dst, (POINT *)&rect, 2 );
    dst->x      = rect.left;
    dst->y      = rect.top;
    dst->width  = rect.right - rect.left;
    dst->height = rect.bottom - rect.top;
    if (dst->layout & LAYOUT_RTL && dst->layout & LAYOUT_BITMAPORIENTATIONPRESERVED)
    {
        dst->x += dst->width;
        dst->width = -dst->width;
    }
    get_bounding_rect( &rect, dst->x, dst->y, dst->width, dst->height );

    clip_visrect( dc_dst, &dst->visrect, &rect );

    /* get the source visible rectangle */

    if (!src) return !IsRectEmpty( &dst->visrect );

    rect.left   = src->log_x;
    rect.top    = src->log_y;
    rect.right  = src->log_x + src->log_width;
    rect.bottom = src->log_y + src->log_height;
    lp_to_dp( dc_src, (POINT *)&rect, 2 );
    src->x      = rect.left;
    src->y      = rect.top;
    src->width  = rect.right - rect.left;
    src->height = rect.bottom - rect.top;
    if (src->layout & LAYOUT_RTL && src->layout & LAYOUT_BITMAPORIENTATIONPRESERVED)
    {
        src->x += src->width;
        src->width = -src->width;
    }
    get_bounding_rect( &rect, src->x, src->y, src->width, src->height );

    if (!clip_device_rect( dc_src, &src->visrect, &rect )) return FALSE;
    if (IsRectEmpty( &dst->visrect )) return FALSE;

    return intersect_vis_rectangles( dst, src );
}

void free_heap_bits( struct gdi_image_bits *bits )
{
    free( bits->ptr );
}

DWORD convert_bits( const BITMAPINFO *src_info, struct bitblt_coords *src,
                    BITMAPINFO *dst_info, struct gdi_image_bits *bits )
{
    void *ptr;
    DWORD err;
    BOOL top_down = dst_info->bmiHeader.biHeight < 0;

    dst_info->bmiHeader.biWidth = src->visrect.right - src->visrect.left;
    dst_info->bmiHeader.biHeight = src->visrect.bottom - src->visrect.top;
    dst_info->bmiHeader.biSizeImage = get_dib_image_size( dst_info );
    if (top_down) dst_info->bmiHeader.biHeight = -dst_info->bmiHeader.biHeight;

    if (!(ptr = malloc( dst_info->bmiHeader.biSizeImage )))
        return ERROR_OUTOFMEMORY;

    err = convert_bitmapinfo( src_info, bits->ptr, src, dst_info, ptr );
    if (bits->free) bits->free( bits );
    bits->ptr = ptr;
    bits->is_copy = TRUE;
    bits->free = free_heap_bits;
    return err;
}

DWORD stretch_bits( const BITMAPINFO *src_info, struct bitblt_coords *src,
                    BITMAPINFO *dst_info, struct bitblt_coords *dst,
                    struct gdi_image_bits *bits, int mode )
{
    void *ptr;
    DWORD err;

    dst_info->bmiHeader.biWidth = dst->visrect.right - dst->visrect.left;
    dst_info->bmiHeader.biHeight = dst->visrect.bottom - dst->visrect.top;
    dst_info->bmiHeader.biSizeImage = get_dib_image_size( dst_info );

    if (src_info->bmiHeader.biHeight < 0) dst_info->bmiHeader.biHeight = -dst_info->bmiHeader.biHeight;
    if (!(ptr = malloc( dst_info->bmiHeader.biSizeImage )))
        return ERROR_OUTOFMEMORY;

    err = stretch_bitmapinfo( src_info, bits->ptr, src, dst_info, ptr, dst, mode );
    if (bits->free) bits->free( bits );
    bits->ptr = ptr;
    bits->is_copy = TRUE;
    bits->free = free_heap_bits;
    return err;
}

static DWORD blend_bits( const BITMAPINFO *src_info, const struct gdi_image_bits *src_bits,
                         struct bitblt_coords *src, BITMAPINFO *dst_info,
                         struct gdi_image_bits *dst_bits, struct bitblt_coords *dst, BLENDFUNCTION blend )
{
    if (!dst_bits->is_copy)
    {
        int size = dst_info->bmiHeader.biSizeImage;
        void *ptr = malloc( size );
        if (!ptr) return ERROR_OUTOFMEMORY;
        memcpy( ptr, dst_bits->ptr, size );
        if (dst_bits->free) dst_bits->free( dst_bits );
        dst_bits->ptr = ptr;
        dst_bits->is_copy = TRUE;
        dst_bits->free = free_heap_bits;
    }
    return blend_bitmapinfo( src_info, src_bits->ptr, src, dst_info, dst_bits->ptr, dst, blend );
}

static RGBQUAD get_dc_rgb_color( DC *dc, int color_table_size, COLORREF color )
{
    RGBQUAD ret = { 0, 0, 0, 0 };

    if (color & (1 << 24))  /* PALETTEINDEX */
    {
        PALETTEENTRY pal;

        if (!get_palette_entries( dc->hPalette, LOWORD(color), 1, &pal ))
            get_palette_entries( dc->hPalette, 0, 1, &pal );
        ret.rgbRed   = pal.peRed;
        ret.rgbGreen = pal.peGreen;
        ret.rgbBlue  = pal.peBlue;
        return ret;
    }
    if (color >> 16 == 0x10ff)  /* DIBINDEX */
    {
        if (color_table_size)
        {
            if (LOWORD(color) >= color_table_size) color = 0x10ff0000;  /* fallback to index 0 */
            *(DWORD *)&ret = color;
        }
        return ret;
    }
    ret.rgbRed   = GetRValue( color );
    ret.rgbGreen = GetGValue( color );
    ret.rgbBlue  = GetBValue( color );
    return ret;
}

/* helper to retrieve either both colors or only the background color for monochrome blits */
void get_mono_dc_colors( DC *dc, int color_table_size, BITMAPINFO *info, int count )
{
    info->bmiColors[count - 1] = get_dc_rgb_color( dc, color_table_size, dc->attr->background_color );
    if (count > 1) info->bmiColors[0] = get_dc_rgb_color( dc, color_table_size, dc->attr->text_color );
    info->bmiHeader.biClrUsed = count;
}

/***********************************************************************
 *           null driver fallback implementations
 */

BOOL nulldrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                         PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop )
{
    DC *dc_src = get_physdev_dc( src_dev ), *dc_dst = get_nulldrv_dc( dst_dev );
    char src_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    char dst_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *src_info = (BITMAPINFO *)src_buffer;
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_buffer;
    DWORD err;
    struct gdi_image_bits bits;

    src_dev = GET_DC_PHYSDEV( dc_src, pGetImage );
    if (src_dev->funcs->pGetImage( src_dev, src_info, &bits, src ))
        return FALSE;

    dst_dev = GET_DC_PHYSDEV( dc_dst, pPutImage );
    copy_bitmapinfo( dst_info, src_info );
    err = dst_dev->funcs->pPutImage( dst_dev, 0, dst_info, &bits, src, dst, rop );
    if (err == ERROR_BAD_FORMAT)
    {
        DWORD dst_colors = dst_info->bmiHeader.biClrUsed;

        /* 1-bpp source without a color table uses the destination DC colors */
        if (src_info->bmiHeader.biBitCount == 1 && !src_info->bmiHeader.biClrUsed)
            get_mono_dc_colors( dc_dst, dst_info->bmiHeader.biClrUsed, src_info, 2 );

        /* 1-bpp destination without a color table requires a fake 1-entry table
         * that contains only the background color */
        if (dst_info->bmiHeader.biBitCount == 1 && !dst_colors)
            get_mono_dc_colors( dc_src, src_info->bmiHeader.biClrUsed, dst_info, 1 );

        if (!(err = convert_bits( src_info, src, dst_info, &bits )))
        {
            /* get rid of the fake destination table */
            dst_info->bmiHeader.biClrUsed = dst_colors;
            err = dst_dev->funcs->pPutImage( dst_dev, 0, dst_info, &bits, src, dst, rop );
        }
    }

    if (err == ERROR_TRANSFORM_NOT_SUPPORTED &&
        ((src->width != dst->width) || (src->height != dst->height)))
    {
        copy_bitmapinfo( src_info, dst_info );
        err = stretch_bits( src_info, src, dst_info, dst, &bits, dc_dst->attr->stretch_blt_mode );
        if (!err) err = dst_dev->funcs->pPutImage( dst_dev, 0, dst_info, &bits, src, dst, rop );
    }

    if (bits.free) bits.free( &bits );
    return !err;
}


BOOL nulldrv_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                         PHYSDEV src_dev, struct bitblt_coords *src, BLENDFUNCTION func )
{
    DC *dc_src = get_physdev_dc( src_dev ), *dc_dst = get_nulldrv_dc( dst_dev );
    char src_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    char dst_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *src_info = (BITMAPINFO *)src_buffer;
    BITMAPINFO *dst_info = (BITMAPINFO *)dst_buffer;
    DWORD err;
    struct gdi_image_bits bits;

    src_dev = GET_DC_PHYSDEV( dc_src, pGetImage );
    err = src_dev->funcs->pGetImage( src_dev, src_info, &bits, src );
    if (err) goto done;

    dst_dev = GET_DC_PHYSDEV( dc_dst, pBlendImage );
    copy_bitmapinfo( dst_info, src_info );
    err = dst_dev->funcs->pBlendImage( dst_dev, dst_info, &bits, src, dst, func );
    if (err == ERROR_BAD_FORMAT)
    {
        err = convert_bits( src_info, src, dst_info, &bits );
        if (!err) err = dst_dev->funcs->pBlendImage( dst_dev, dst_info, &bits, src, dst, func );
    }

    if (err == ERROR_TRANSFORM_NOT_SUPPORTED &&
        ((src->width != dst->width) || (src->height != dst->height)))
    {
        copy_bitmapinfo( src_info, dst_info );
        err = stretch_bits( src_info, src, dst_info, dst, &bits, COLORONCOLOR );
        if (!err) err = dst_dev->funcs->pBlendImage( dst_dev, dst_info, &bits, src, dst, func );
    }

    if (bits.free) bits.free( &bits );
done:
    if (err) RtlSetLastWin32Error( err );
    return !err;
}


DWORD nulldrv_BlendImage( PHYSDEV dev, BITMAPINFO *info, const struct gdi_image_bits *bits,
                          struct bitblt_coords *src, struct bitblt_coords *dst, BLENDFUNCTION blend )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *dst_info = (BITMAPINFO *)buffer;
    struct gdi_image_bits dst_bits;
    struct bitblt_coords orig_dst;
    DWORD *masks = (DWORD *)info->bmiColors;
    DC *dc = get_nulldrv_dc( dev );
    DWORD err;

    if (info->bmiHeader.biPlanes != 1) goto update_format;
    if (info->bmiHeader.biBitCount != 32) goto update_format;
    if (info->bmiHeader.biCompression == BI_BITFIELDS)
    {
        if (blend.AlphaFormat & AC_SRC_ALPHA) return ERROR_INVALID_PARAMETER;
        if (masks[0] != 0xff0000 || masks[1] != 0x00ff00 || masks[2] != 0x0000ff)
            goto update_format;
    }

    if (!bits) return ERROR_SUCCESS;
    if ((src->width != dst->width) || (src->height != dst->height)) return ERROR_TRANSFORM_NOT_SUPPORTED;

    dev = GET_DC_PHYSDEV( dc, pGetImage );
    orig_dst = *dst;
    err = dev->funcs->pGetImage( dev, dst_info, &dst_bits, dst );
    if (err) return err;

    dev = GET_DC_PHYSDEV( dc, pPutImage );
    err = blend_bits( info, bits, src, dst_info, &dst_bits, dst, blend );
    if (!err) err = dev->funcs->pPutImage( dev, 0, dst_info, &dst_bits, dst, &orig_dst, SRCCOPY );

    if (dst_bits.free) dst_bits.free( &dst_bits );
    return err;

update_format:
    if (blend.AlphaFormat & AC_SRC_ALPHA)  /* source alpha requires A8R8G8B8 format */
        return ERROR_INVALID_PARAMETER;

    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_BITFIELDS;
    info->bmiHeader.biClrUsed     = 0;
    masks[0] = 0xff0000;
    masks[1] = 0x00ff00;
    masks[2] = 0x0000ff;
    return ERROR_BAD_FORMAT;
}

BOOL nulldrv_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                           void * grad_array, ULONG ngrad, ULONG mode )
{
    DC *dc = get_nulldrv_dc( dev );
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct bitblt_coords src, dst;
    struct gdi_image_bits bits;
    unsigned int i;
    POINT *pts;
    BOOL ret = FALSE;
    DWORD err;
    HRGN rgn;

    if (!(pts = malloc( nvert * sizeof(*pts) ))) return FALSE;
    for (i = 0; i < nvert; i++)
    {
        pts[i].x = vert_array[i].x;
        pts[i].y = vert_array[i].y;
    }
    lp_to_dp( dc, pts, nvert );

    /* compute bounding rect of all the rectangles/triangles */
    reset_bounds( &dst.visrect );
    for (i = 0; i < ngrad * (mode == GRADIENT_FILL_TRIANGLE ? 3 : 2); i++)
    {
        ULONG v = ((ULONG *)grad_array)[i];
        dst.visrect.left   = min( dst.visrect.left,   pts[v].x );
        dst.visrect.top    = min( dst.visrect.top,    pts[v].y );
        dst.visrect.right  = max( dst.visrect.right,  pts[v].x );
        dst.visrect.bottom = max( dst.visrect.bottom, pts[v].y );
    }

    dst.x = dst.visrect.left;
    dst.y = dst.visrect.top;
    dst.width = dst.visrect.right - dst.visrect.left;
    dst.height = dst.visrect.bottom - dst.visrect.top;
    if (!clip_visrect( dc, &dst.visrect, &dst.visrect )) goto done;

    /* query the bitmap format */
    info->bmiHeader.biSize          = sizeof(info->bmiHeader);
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = 0;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
    info->bmiHeader.biWidth         = dst.visrect.right - dst.visrect.left;
    info->bmiHeader.biHeight        = dst.visrect.bottom - dst.visrect.top;
    info->bmiHeader.biSizeImage     = 0;
    dev = GET_DC_PHYSDEV( dc, pPutImage );
    err = dev->funcs->pPutImage( dev, 0, info, NULL, NULL, NULL, 0 );
    if (err && err != ERROR_BAD_FORMAT) goto done;

    info->bmiHeader.biSizeImage = get_dib_image_size( info );
    if (!(bits.ptr = calloc( 1, info->bmiHeader.biSizeImage )))
        goto done;
    bits.is_copy = TRUE;
    bits.free = free_heap_bits;

    /* make src and points relative to the bitmap */
    src = dst;
    src.x -= dst.visrect.left;
    src.y -= dst.visrect.top;
    OffsetRect( &src.visrect, -dst.visrect.left, -dst.visrect.top );
    for (i = 0; i < nvert; i++)
    {
        pts[i].x -= dst.visrect.left;
        pts[i].y -= dst.visrect.top;
    }

    rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    gradient_bitmapinfo( info, bits.ptr, vert_array, nvert, grad_array, ngrad, mode, pts, rgn );
    NtGdiOffsetRgn( rgn, dst.visrect.left, dst.visrect.top );
    ret = !dev->funcs->pPutImage( dev, rgn, info, &bits, &src, &dst, SRCCOPY );

    if (bits.free) bits.free( &bits );
    NtGdiDeleteObjectApp( rgn );

done:
    free( pts );
    return ret;
}

COLORREF nulldrv_GetPixel( PHYSDEV dev, INT x, INT y )
{
    DC *dc = get_nulldrv_dc( dev );
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct bitblt_coords src;
    struct gdi_image_bits bits;
    COLORREF ret;

    src.visrect.left = x;
    src.visrect.top  = y;
    lp_to_dp( dc, (POINT *)&src.visrect, 1 );
    src.visrect.right  = src.visrect.left + 1;
    src.visrect.bottom = src.visrect.top + 1;
    src.x = src.visrect.left;
    src.y = src.visrect.top;
    src.width = src.height = 1;

    if (!clip_visrect( dc, &src.visrect, &src.visrect )) return CLR_INVALID;

    dev = GET_DC_PHYSDEV( dc, pGetImage );
    if (dev->funcs->pGetImage( dev, info, &bits, &src )) return CLR_INVALID;

    ret = get_pixel_bitmapinfo( info, bits.ptr, &src );
    if (bits.free) bits.free( &bits );
    return ret;
}


/***********************************************************************
 *           NtGdiPatBlt    (win32u.@)
 */
BOOL WINAPI NtGdiPatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop )
{
    DC * dc;
    BOOL ret = FALSE;

    if (rop_uses_src( rop )) return FALSE;
    if ((dc = get_dc_ptr( hdc )))
    {
        struct bitblt_coords dst;

        update_dc( dc );

        dst.log_x      = left;
        dst.log_y      = top;
        dst.log_width  = width;
        dst.log_height = height;
        dst.layout     = dc->attr->layout;
        if (rop & NOMIRRORBITMAP)
        {
            dst.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
            rop &= ~NOMIRRORBITMAP;
        }
        ret = !get_vis_rectangles( dc, &dst, NULL, NULL );

        TRACE("dst %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  rop=%06x\n",
              hdc, dst.log_x, dst.log_y, dst.log_width, dst.log_height,
              dst.x, dst.y, dst.width, dst.height, wine_dbgstr_rect(&dst.visrect), (int)rop );

        if (!ret)
        {
            PHYSDEV physdev = GET_DC_PHYSDEV( dc, pPatBlt );
            ret = physdev->funcs->pPatBlt( physdev, &dst, rop );
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           NtGdiBitBlt    (win32u.@)
 */
BOOL WINAPI NtGdiBitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height,
                         HDC hdc_src, INT x_src, INT y_src, DWORD rop, DWORD bk_color, FLONG fl )
{
    return NtGdiStretchBlt( hdc_dst, x_dst, y_dst, width, height,
                            hdc_src, x_src, y_src, width, height, rop, bk_color );
}


/***********************************************************************
 *           NtGdiStretchBlt    (win32u.@)
 */
BOOL WINAPI NtGdiStretchBlt( HDC hdcDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             HDC hdcSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             DWORD rop, COLORREF bk_color )
{
    BOOL ret = FALSE;
    DC *dcDst, *dcSrc;

    if (!rop_uses_src( rop )) return NtGdiPatBlt( hdcDst, xDst, yDst, widthDst, heightDst, rop );

    if (!(dcDst = get_dc_ptr( hdcDst ))) return FALSE;

    if ((dcSrc = get_dc_ptr( hdcSrc )))
    {
        struct bitblt_coords src, dst;

        update_dc( dcSrc );
        update_dc( dcDst );

        src.log_x      = xSrc;
        src.log_y      = ySrc;
        src.log_width  = widthSrc;
        src.log_height = heightSrc;
        src.layout     = dcSrc->attr->layout;
        dst.log_x      = xDst;
        dst.log_y      = yDst;
        dst.log_width  = widthDst;
        dst.log_height = heightDst;
        dst.layout     = dcDst->attr->layout;
        if (rop & NOMIRRORBITMAP)
        {
            src.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
            dst.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
            rop &= ~NOMIRRORBITMAP;
        }
        ret = !get_vis_rectangles( dcDst, &dst, dcSrc, &src );

        TRACE("src %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  dst %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  rop=%06x\n",
              hdcSrc, src.log_x, src.log_y, src.log_width, src.log_height,
              src.x, src.y, src.width, src.height, wine_dbgstr_rect(&src.visrect),
              hdcDst, dst.log_x, dst.log_y, dst.log_width, dst.log_height,
              dst.x, dst.y, dst.width, dst.height, wine_dbgstr_rect(&dst.visrect), (int)rop );

        if (!ret)
        {
            PHYSDEV src_dev = GET_DC_PHYSDEV( dcSrc, pStretchBlt );
            PHYSDEV dst_dev = GET_DC_PHYSDEV( dcDst, pStretchBlt );
            ret = dst_dev->funcs->pStretchBlt( dst_dev, &dst, src_dev, &src, rop );
        }
        release_dc_ptr( dcSrc );
    }
    release_dc_ptr( dcDst );
    return ret;
}

#define FRGND_ROP3(ROP4)	((ROP4) & 0x00FFFFFF)
#define BKGND_ROP3(ROP4)	(ROP3Table[((ROP4)>>24) & 0xFF])

/***********************************************************************
 *           NtGdiMaskBlt    (win32u.@)
 */
BOOL WINAPI NtGdiMaskBlt( HDC hdcDest, INT nXDest, INT nYDest, INT nWidth, INT nHeight,
                          HDC hdcSrc, INT nXSrc, INT nYSrc, HBITMAP hbmMask,
                          INT xMask, INT yMask, DWORD dwRop, DWORD bk_color )
{
    HBITMAP hBitmap1, hOldBitmap1, hBitmap2, hOldBitmap2;
    HDC hDC1, hDC2;
    HBRUSH hbrMask, hbrDst, hbrTmp;

    static const DWORD ROP3Table[256] = 
    {
        0x00000042, 0x00010289,
        0x00020C89, 0x000300AA,
        0x00040C88, 0x000500A9,
        0x00060865, 0x000702C5,
        0x00080F08, 0x00090245,
        0x000A0329, 0x000B0B2A,
        0x000C0324, 0x000D0B25,
        0x000E08A5, 0x000F0001,
        0x00100C85, 0x001100A6,
        0x00120868, 0x001302C8,
        0x00140869, 0x001502C9,
        0x00165CCA, 0x00171D54,
        0x00180D59, 0x00191CC8,
        0x001A06C5, 0x001B0768,
        0x001C06CA, 0x001D0766,
        0x001E01A5, 0x001F0385,
        0x00200F09, 0x00210248,
        0x00220326, 0x00230B24,
        0x00240D55, 0x00251CC5,
        0x002606C8, 0x00271868,
        0x00280369, 0x002916CA,
        0x002A0CC9, 0x002B1D58,
        0x002C0784, 0x002D060A,
        0x002E064A, 0x002F0E2A,
        0x0030032A, 0x00310B28,
        0x00320688, 0x00330008,
        0x003406C4, 0x00351864,
        0x003601A8, 0x00370388,
        0x0038078A, 0x00390604,
        0x003A0644, 0x003B0E24,
        0x003C004A, 0x003D18A4,
        0x003E1B24, 0x003F00EA,
        0x00400F0A, 0x00410249,
        0x00420D5D, 0x00431CC4,
        0x00440328, 0x00450B29,
        0x004606C6, 0x0047076A,
        0x00480368, 0x004916C5,
        0x004A0789, 0x004B0605,
        0x004C0CC8, 0x004D1954,
        0x004E0645, 0x004F0E25,
        0x00500325, 0x00510B26,
        0x005206C9, 0x00530764,
        0x005408A9, 0x00550009,
        0x005601A9, 0x00570389,
        0x00580785, 0x00590609,
        0x005A0049, 0x005B18A9,
        0x005C0649, 0x005D0E29,
        0x005E1B29, 0x005F00E9,
        0x00600365, 0x006116C6,
        0x00620786, 0x00630608,
        0x00640788, 0x00650606,
        0x00660046, 0x006718A8,
        0x006858A6, 0x00690145,
        0x006A01E9, 0x006B178A,
        0x006C01E8, 0x006D1785,
        0x006E1E28, 0x006F0C65,
        0x00700CC5, 0x00711D5C,
        0x00720648, 0x00730E28,
        0x00740646, 0x00750E26,
        0x00761B28, 0x007700E6,
        0x007801E5, 0x00791786,
        0x007A1E29, 0x007B0C68,
        0x007C1E24, 0x007D0C69,
        0x007E0955, 0x007F03C9,
        0x008003E9, 0x00810975,
        0x00820C49, 0x00831E04,
        0x00840C48, 0x00851E05,
        0x008617A6, 0x008701C5,
        0x008800C6, 0x00891B08,
        0x008A0E06, 0x008B0666,
        0x008C0E08, 0x008D0668,
        0x008E1D7C, 0x008F0CE5,
        0x00900C45, 0x00911E08,
        0x009217A9, 0x009301C4,
        0x009417AA, 0x009501C9,
        0x00960169, 0x0097588A,
        0x00981888, 0x00990066,
        0x009A0709, 0x009B07A8,
        0x009C0704, 0x009D07A6,
        0x009E16E6, 0x009F0345,
        0x00A000C9, 0x00A11B05,
        0x00A20E09, 0x00A30669,
        0x00A41885, 0x00A50065,
        0x00A60706, 0x00A707A5,
        0x00A803A9, 0x00A90189,
        0x00AA0029, 0x00AB0889,
        0x00AC0744, 0x00AD06E9,
        0x00AE0B06, 0x00AF0229,
        0x00B00E05, 0x00B10665,
        0x00B21974, 0x00B30CE8,
        0x00B4070A, 0x00B507A9,
        0x00B616E9, 0x00B70348,
        0x00B8074A, 0x00B906E6,
        0x00BA0B09, 0x00BB0226,
        0x00BC1CE4, 0x00BD0D7D,
        0x00BE0269, 0x00BF08C9,
        0x00C000CA, 0x00C11B04,
        0x00C21884, 0x00C3006A,
        0x00C40E04, 0x00C50664,
        0x00C60708, 0x00C707AA,
        0x00C803A8, 0x00C90184,
        0x00CA0749, 0x00CB06E4,
        0x00CC0020, 0x00CD0888,
        0x00CE0B08, 0x00CF0224,
        0x00D00E0A, 0x00D1066A,
        0x00D20705, 0x00D307A4,
        0x00D41D78, 0x00D50CE9,
        0x00D616EA, 0x00D70349,
        0x00D80745, 0x00D906E8,
        0x00DA1CE9, 0x00DB0D75,
        0x00DC0B04, 0x00DD0228,
        0x00DE0268, 0x00DF08C8,
        0x00E003A5, 0x00E10185,
        0x00E20746, 0x00E306EA,
        0x00E40748, 0x00E506E5,
        0x00E61CE8, 0x00E70D79,
        0x00E81D74, 0x00E95CE6,
        0x00EA02E9, 0x00EB0849,
        0x00EC02E8, 0x00ED0848,
        0x00EE0086, 0x00EF0A08,
        0x00F00021, 0x00F10885,
        0x00F20B05, 0x00F3022A,
        0x00F40B0A, 0x00F50225,
        0x00F60265, 0x00F708C5,
        0x00F802E5, 0x00F90845,
        0x00FA0089, 0x00FB0A09,
        0x00FC008A, 0x00FD0A0A,
        0x00FE02A9, 0x00FF0062,
    };

    if (!hbmMask)
        return NtGdiBitBlt( hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc,
                            nXSrc, nYSrc, FRGND_ROP3(dwRop), bk_color, 0 );

    hbrMask = NtGdiCreatePatternBrushInternal( hbmMask, FALSE, FALSE );
    hbrDst = NtGdiSelectBrush( hdcDest, GetStockObject(NULL_BRUSH) );

    /* make bitmap */
    hDC1 = NtGdiCreateCompatibleDC( hdcDest );
    hBitmap1 = NtGdiCreateCompatibleBitmap( hdcDest, nWidth, nHeight );
    hOldBitmap1 = NtGdiSelectBitmap(hDC1, hBitmap1);

    /* draw using bkgnd rop */
    NtGdiBitBlt( hDC1, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY, 0, 0 );
    hbrTmp = NtGdiSelectBrush(hDC1, hbrDst);
    NtGdiBitBlt( hDC1, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, BKGND_ROP3(dwRop), 0, 0 );
    NtGdiSelectBrush(hDC1, hbrTmp);

    /* make bitmap */
    hDC2 = NtGdiCreateCompatibleDC( hdcDest );
    hBitmap2 = NtGdiCreateCompatibleBitmap( hdcDest, nWidth, nHeight );
    hOldBitmap2 = NtGdiSelectBitmap(hDC2, hBitmap2);

    /* draw using foregnd rop */
    NtGdiBitBlt( hDC2, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY, 0, 0 );
    hbrTmp = NtGdiSelectBrush(hDC2, hbrDst);
    NtGdiBitBlt( hDC2, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop), 0, 0 );

    /* combine both using the mask as a pattern brush */
    NtGdiSelectBrush(hDC2, hbrMask);
    NtGdiSetBrushOrg( hDC2, -xMask, -yMask, NULL );
    /* (D & P) | (S & ~P) */
    NtGdiBitBlt(hDC2, 0, 0, nWidth, nHeight, hDC1, 0, 0, 0xac0744, 0, 0 );
    NtGdiSelectBrush(hDC2, hbrTmp);

    /* blit to dst */
    NtGdiBitBlt( hdcDest, nXDest, nYDest, nWidth, nHeight, hDC2, 0, 0, SRCCOPY, bk_color, 0 );

    /* restore all objects */
    NtGdiSelectBrush(hdcDest, hbrDst);
    NtGdiSelectBitmap(hDC1, hOldBitmap1);
    NtGdiSelectBitmap(hDC2, hOldBitmap2);

    /* delete all temp objects */
    NtGdiDeleteObjectApp( hBitmap1 );
    NtGdiDeleteObjectApp( hBitmap2 );
    NtGdiDeleteObjectApp( hbrMask );

    NtGdiDeleteObjectApp( hDC1 );
    NtGdiDeleteObjectApp( hDC2 );

    return TRUE;
}

/******************************************************************************
 *           NtGdiTransparentBlt    (win32u.@)
 */
BOOL WINAPI NtGdiTransparentBlt( HDC hdcDest, int xDest, int yDest, int widthDest, int heightDest,
                                 HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                                 UINT crTransparent )
{
    BOOL ret = FALSE;
    HDC hdcWork;
    HBITMAP bmpWork;
    HGDIOBJ oldWork;
    HDC hdcMask = NULL;
    HBITMAP bmpMask = NULL;
    HBITMAP oldMask = NULL;
    COLORREF oldBackground;
    COLORREF oldForeground;
    int oldStretchMode;
    DIBSECTION dib;
    DC *dc_src;

    if(widthDest < 0 || heightDest < 0 || widthSrc < 0 || heightSrc < 0) {
        TRACE("Cannot mirror\n");
        return FALSE;
    }

    if (!(dc_src = get_dc_ptr( hdcSrc ))) return FALSE;

    NtGdiGetAndSetDCDword( hdcDest, NtGdiSetBkColor, RGB(255,255,255), &oldBackground );
    NtGdiGetAndSetDCDword( hdcDest, NtGdiSetTextColor, RGB(0,0,0), &oldForeground );

    /* Stretch bitmap */
    oldStretchMode = dc_src->attr->stretch_blt_mode;
    if (oldStretchMode == BLACKONWHITE || oldStretchMode == WHITEONBLACK)
        dc_src->attr->stretch_blt_mode = COLORONCOLOR;
    hdcWork = NtGdiCreateCompatibleDC( hdcDest );
    if ((get_gdi_object_type( hdcDest ) != NTGDI_OBJ_MEMDC ||
         NtGdiExtGetObjectW( NtGdiGetDCObject( hdcDest, NTGDI_OBJ_SURF ),
                             sizeof(dib), &dib ) == sizeof(BITMAP)) &&
        NtGdiGetDeviceCaps( hdcDest, BITSPIXEL ) == 32)
    {
        /* screen DCs or DDBs are not supposed to have an alpha channel, so use a 24-bpp bitmap as copy */
        BITMAPINFO info;
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = widthDest;
        info.bmiHeader.biHeight = heightDest;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 24;
        info.bmiHeader.biCompression = BI_RGB;
        bmpWork = NtGdiCreateDIBSection( 0, NULL, 0, &info, DIB_RGB_COLORS, 0, 0, 0, NULL );
    }
    else bmpWork = NtGdiCreateCompatibleBitmap( hdcDest, widthDest, heightDest );
    oldWork = NtGdiSelectBitmap(hdcWork, bmpWork);
    if (!NtGdiStretchBlt( hdcWork, 0, 0, widthDest, heightDest, hdcSrc, xSrc, ySrc,
                          widthSrc, heightSrc, SRCCOPY, 0 ))
    {
        TRACE("Failed to stretch\n");
        goto error;
    }
    NtGdiGetAndSetDCDword( hdcWork, NtGdiSetBkColor, crTransparent, NULL );

    /* Create mask */
    hdcMask = NtGdiCreateCompatibleDC( hdcDest );
    bmpMask = NtGdiCreateCompatibleBitmap( hdcMask, widthDest, heightDest );
    oldMask = NtGdiSelectBitmap(hdcMask, bmpMask);
    if (!NtGdiBitBlt( hdcMask, 0, 0, widthDest, heightDest, hdcWork, 0, 0, SRCCOPY, 0, 0 ))
    {
        TRACE("Failed to create mask\n");
        goto error;
    }

    /* Replace transparent color with black */
    NtGdiGetAndSetDCDword( hdcWork, NtGdiSetBkColor, RGB(0,0,0), NULL );
    NtGdiGetAndSetDCDword( hdcWork, NtGdiSetTextColor, RGB(255,255,255), NULL );
    if (!NtGdiBitBlt( hdcWork, 0, 0, widthDest, heightDest, hdcMask, 0, 0, SRCAND, 0, 0 ))
    {
        TRACE("Failed to mask out background\n");
        goto error;
    }

    /* Replace non-transparent area on destination with black */
    if (!NtGdiBitBlt( hdcDest, xDest, yDest, widthDest, heightDest, hdcMask, 0, 0, SRCAND, 0, 0 ))
    {
        TRACE("Failed to clear destination area\n");
        goto error;
    }

    /* Draw the image */
    if (!NtGdiBitBlt( hdcDest, xDest, yDest, widthDest, heightDest, hdcWork, 0, 0, SRCPAINT, 0, 0 ))
    {
        TRACE("Failed to paint image\n");
        goto error;
    }

    ret = TRUE;
error:
    dc_src->attr->stretch_blt_mode = oldStretchMode;
    release_dc_ptr( dc_src );
    NtGdiGetAndSetDCDword( hdcDest, NtGdiSetBkColor, oldBackground, NULL );
    NtGdiGetAndSetDCDword( hdcDest, NtGdiSetTextColor, oldForeground, NULL );
    if(hdcWork) {
        NtGdiSelectBitmap(hdcWork, oldWork);
        NtGdiDeleteObjectApp( hdcWork );
    }
    if(bmpWork) NtGdiDeleteObjectApp( bmpWork );
    if(hdcMask) {
        NtGdiSelectBitmap(hdcMask, oldMask);
        NtGdiDeleteObjectApp( hdcMask );
    }
    if(bmpMask) NtGdiDeleteObjectApp( bmpMask );
    return ret;
}

/******************************************************************************
 *           NtGdiAlphaBlend   (win32u.@)
 */
BOOL WINAPI NtGdiAlphaBlend( HDC hdcDst, int xDst, int yDst, int widthDst, int heightDst,
                             HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                             DWORD blend_func, HANDLE xform )
{
    BLENDFUNCTION blendFunction = *(BLENDFUNCTION *)&blend_func;
    BOOL ret = FALSE;
    DC *dcDst, *dcSrc;

    dcSrc = get_dc_ptr( hdcSrc );
    if (!dcSrc) return FALSE;

    if ((dcDst = get_dc_ptr( hdcDst )))
    {
        struct bitblt_coords src, dst;

        update_dc( dcSrc );
        update_dc( dcDst );

        src.log_x      = xSrc;
        src.log_y      = ySrc;
        src.log_width  = widthSrc;
        src.log_height = heightSrc;
        src.layout     = dcSrc->attr->layout;
        dst.log_x      = xDst;
        dst.log_y      = yDst;
        dst.log_width  = widthDst;
        dst.log_height = heightDst;
        dst.layout     = dcDst->attr->layout;
        ret = !get_vis_rectangles( dcDst, &dst, dcSrc, &src );

        TRACE("src %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  dst %p log=%d,%d %dx%d phys=%d,%d %dx%d vis=%s  blend=%02x/%02x/%02x/%02x\n",
              hdcSrc, src.log_x, src.log_y, src.log_width, src.log_height,
              src.x, src.y, src.width, src.height, wine_dbgstr_rect(&src.visrect),
              hdcDst, dst.log_x, dst.log_y, dst.log_width, dst.log_height,
              dst.x, dst.y, dst.width, dst.height, wine_dbgstr_rect(&dst.visrect),
              blendFunction.BlendOp, blendFunction.BlendFlags,
              blendFunction.SourceConstantAlpha, blendFunction.AlphaFormat );

        if (src.x < 0 || src.y < 0 || src.width < 0 || src.height < 0 ||
            src.log_width < 0 || src.log_height < 0 ||
            (!IsRectEmpty( &dcSrc->device_rect ) &&
             (src.width > dcSrc->device_rect.right - dcSrc->attr->vis_rect.left - src.x ||
              src.height > dcSrc->device_rect.bottom - dcSrc->attr->vis_rect.top - src.y)))
        {
            WARN( "Invalid src coords: (%d,%d), size %dx%d\n", src.x, src.y, src.width, src.height );
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            ret = FALSE;
        }
        else if (dst.log_width < 0 || dst.log_height < 0)
        {
            WARN( "Invalid dst coords: (%d,%d), size %dx%d\n",
                  dst.log_x, dst.log_y, dst.log_width, dst.log_height );
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            ret = FALSE;
        }
        else if (dcSrc == dcDst && src.x + src.width > dst.x && src.x < dst.x + dst.width &&
                 src.y + src.height > dst.y && src.y < dst.y + dst.height)
        {
            WARN( "Overlapping coords: (%d,%d), %dx%d and (%d,%d), %dx%d\n",
                  src.x, src.y, src.width, src.height, dst.x, dst.y, dst.width, dst.height );
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            ret = FALSE;
        }
        else if (!ret)
        {
            PHYSDEV src_dev = GET_DC_PHYSDEV( dcSrc, pAlphaBlend );
            PHYSDEV dst_dev = GET_DC_PHYSDEV( dcDst, pAlphaBlend );
            ret = dst_dev->funcs->pAlphaBlend( dst_dev, &dst, src_dev, &src, blendFunction );
        }
        release_dc_ptr( dcDst );
    }
    release_dc_ptr( dcSrc );
    return ret;
}

/*********************************************************************
 *           NtGdiPlgBlt    (win32u.@)
 */
BOOL WINAPI NtGdiPlgBlt( HDC hdcDest, const POINT *lpPoint, HDC hdcSrc, INT nXSrc, INT nYSrc,
                         INT nWidth, INT nHeight, HBITMAP hbmMask, INT xMask, INT yMask,
                         DWORD bk_color )
{
    DWORD prev_mode;
    /* parallelogram coords */
    POINT plg[3];
    /* rect coords */
    POINT rect[3];
    XFORM xf;
    XFORM SrcXf;
    XFORM oldDestXf;
    double det;

    /* save actual mode, set GM_ADVANCED */
    if (!NtGdiGetAndSetDCDword( hdcDest, NtGdiSetGraphicsMode, GM_ADVANCED, &prev_mode ))
        return FALSE;

    memcpy(plg,lpPoint,sizeof(POINT)*3);
    rect[0].x = nXSrc;
    rect[0].y = nYSrc;
    rect[1].x = nXSrc + nWidth;
    rect[1].y = nYSrc;
    rect[2].x = nXSrc;
    rect[2].y = nYSrc + nHeight;
    /* calc XFORM matrix to transform hdcDest -> hdcSrc (parallelogram to rectangle) */
    /* determinant */
    det = rect[1].x*(rect[2].y - rect[0].y) - rect[2].x*(rect[1].y - rect[0].y) - rect[0].x*(rect[2].y - rect[1].y);

    if (fabs(det) < 1e-5)
    {
        NtGdiGetAndSetDCDword( hdcDest, NtGdiSetGraphicsMode, prev_mode, NULL );
        return FALSE;
    }

    TRACE("hdcSrc=%p %d,%d,%dx%d -> hdcDest=%p %d,%d,%d,%d,%d,%d\n",
          hdcSrc, nXSrc, nYSrc, nWidth, nHeight, hdcDest,
          (int)plg[0].x, (int)plg[0].y, (int)plg[1].x, (int)plg[1].y, (int)plg[2].x, (int)plg[2].y);

    /* X components */
    xf.eM11 = (plg[1].x*(rect[2].y - rect[0].y) - plg[2].x*(rect[1].y - rect[0].y) - plg[0].x*(rect[2].y - rect[1].y)) / det;
    xf.eM21 = (rect[1].x*(plg[2].x - plg[0].x) - rect[2].x*(plg[1].x - plg[0].x) - rect[0].x*(plg[2].x - plg[1].x)) / det;
    xf.eDx  = (rect[0].x*(rect[1].y*plg[2].x - rect[2].y*plg[1].x) -
               rect[1].x*(rect[0].y*plg[2].x - rect[2].y*plg[0].x) +
               rect[2].x*(rect[0].y*plg[1].x - rect[1].y*plg[0].x)
               ) / det;

    /* Y components */
    xf.eM12 = (plg[1].y*(rect[2].y - rect[0].y) - plg[2].y*(rect[1].y - rect[0].y) - plg[0].y*(rect[2].y - rect[1].y)) / det;
    xf.eM22 = (rect[1].x*(plg[2].y - plg[0].y) - rect[2].x*(plg[1].y - plg[0].y) - rect[0].x*(plg[2].y - plg[1].y)) / det;
    xf.eDy  = (rect[0].x*(rect[1].y*plg[2].y - rect[2].y*plg[1].y) -
               rect[1].x*(rect[0].y*plg[2].y - rect[2].y*plg[0].y) +
               rect[2].x*(rect[0].y*plg[1].y - rect[1].y*plg[0].y)
               ) / det;

    NtGdiGetTransform( hdcSrc, 0x203, &SrcXf );
    combine_transform( &xf, &xf, &SrcXf );

    /* save actual dest transform */
    NtGdiGetTransform( hdcDest, 0x203, &oldDestXf );

    NtGdiModifyWorldTransform( hdcDest, &xf, MWT_SET );
    /* now destination and source DCs use same coords */
    NtGdiMaskBlt( hdcDest, nXSrc, nYSrc, nWidth, nHeight, hdcSrc, nXSrc, nYSrc,
                  hbmMask, xMask, yMask, SRCCOPY, 0 );
    /* restore dest DC */
    NtGdiModifyWorldTransform( hdcDest, &oldDestXf, MWT_SET );
    NtGdiGetAndSetDCDword( hdcDest, NtGdiSetGraphicsMode, prev_mode, NULL );

    return TRUE;
}
