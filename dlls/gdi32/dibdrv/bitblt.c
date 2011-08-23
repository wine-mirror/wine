/*
 * DIB driver blitting
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

static DWORD copy_rect( dib_info *dst, const RECT *dst_rect, const dib_info *src, const RECT *src_rect,
                        HRGN clip, INT rop2 )
{
    POINT origin;
    RECT clipped_rect;
    const WINEREGION *clip_data;
    int i;

    origin.x = src_rect->left;
    origin.y = src_rect->top;

    if (clip == NULL) dst->funcs->copy_rect( dst, dst_rect, src, &origin, rop2 );
    else
    {
        clip_data = get_wine_region( clip );
        for (i = 0; i < clip_data->numRects; i++)
        {
            if (intersect_rect( &clipped_rect, dst_rect, clip_data->rects + i ))
            {
                origin.x = src_rect->left + clipped_rect.left - dst_rect->left;
                origin.y = src_rect->top  + clipped_rect.top  - dst_rect->top;
                dst->funcs->copy_rect( dst, &clipped_rect, src, &origin, rop2 );
            }
        }
        release_wine_region( clip );
    }
    return ERROR_SUCCESS;
}

static void set_color_info( const dib_info *dib, BITMAPINFO *info )
{
    DWORD *masks = (DWORD *)info->bmiColors;

    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biClrUsed     = 0;

    switch (info->bmiHeader.biBitCount)
    {
    case 1:
    case 4:
    case 8:
        if (dib->color_table)
        {
            info->bmiHeader.biClrUsed = min( dib->color_table_size, 1 << dib->bit_count );
            memcpy( info->bmiColors, dib->color_table,
                    info->bmiHeader.biClrUsed * sizeof(RGBQUAD) );
        }
        break;
    case 16:
        masks[0] = dib->red_mask;
        masks[1] = dib->green_mask;
        masks[2] = dib->blue_mask;
        info->bmiHeader.biCompression = BI_BITFIELDS;
        break;
    case 32:
        if (dib->funcs != &funcs_8888)
        {
            masks[0] = dib->red_mask;
            masks[1] = dib->green_mask;
            masks[2] = dib->blue_mask;
            info->bmiHeader.biCompression = BI_BITFIELDS;
        }
        break;
    }
}

/***********************************************************************
 *           dibdrv_GetImage
 */
DWORD dibdrv_GetImage( PHYSDEV dev, HBITMAP hbitmap, BITMAPINFO *info,
                       struct gdi_image_bits *bits, struct bitblt_coords *src )
{
    DWORD ret = ERROR_SUCCESS;
    dib_info *dib, stand_alone;

    TRACE( "%p %p %p\n", dev, hbitmap, info );

    info->bmiHeader.biSize          = sizeof(info->bmiHeader);
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;

    if (hbitmap)
    {
        BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );

        if (!bmp) return ERROR_INVALID_HANDLE;
        assert(bmp->dib);

        if (!init_dib_info( &stand_alone, &bmp->dib->dsBmih, bmp->dib->dsBitfields,
                            bmp->color_table, bmp->nb_colors, bmp->dib->dsBm.bmBits, 0 ))
        {
            ret = ERROR_BAD_FORMAT;
            goto done;
        }
        dib = &stand_alone;
    }
    else
    {
        dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
        dib = &pdev->dib;
    }

    info->bmiHeader.biWidth     = dib->width;
    info->bmiHeader.biHeight    = dib->stride > 0 ? -dib->height : dib->height;
    info->bmiHeader.biBitCount  = dib->bit_count;
    info->bmiHeader.biSizeImage = dib->height * abs( dib->stride );

    set_color_info( dib, info );

    if (bits)
    {
        bits->ptr = dib->bits.ptr;
        if (dib->stride < 0)
            bits->ptr = (char *)bits->ptr + (dib->height - 1) * dib->stride;
        bits->is_copy = FALSE;
        bits->free = NULL;
    }

done:
   if (hbitmap) GDI_ReleaseObj( hbitmap );
   return ret;
}

static BOOL matching_color_info( const dib_info *dib, const BITMAPINFO *info )
{
    switch (info->bmiHeader.biBitCount)
    {
    case 1:
    case 4:
    case 8:
    {
        RGBQUAD *color_table = (RGBQUAD *)((char *)info + info->bmiHeader.biSize);
        if (dib->color_table_size != get_dib_num_of_colors( info )) return FALSE;
        return !memcmp( color_table, dib->color_table, dib->color_table_size * sizeof(RGBQUAD) );
    }

    case 16:
    {
        DWORD *masks = (DWORD *)info->bmiColors;
        if (info->bmiHeader.biCompression == BI_RGB) return dib->funcs == &funcs_555;
        if (info->bmiHeader.biCompression == BI_BITFIELDS)
            return masks[0] == dib->red_mask && masks[1] == dib->green_mask && masks[2] == dib->blue_mask;
        break;
    }

    case 24:
        return TRUE;

    case 32:
    {
        DWORD *masks = (DWORD *)info->bmiColors;
        if (info->bmiHeader.biCompression == BI_RGB) return dib->funcs == &funcs_8888;
        if (info->bmiHeader.biCompression == BI_BITFIELDS)
            return masks[0] == dib->red_mask && masks[1] == dib->green_mask && masks[2] == dib->blue_mask;
        break;
    }

    }

    return FALSE;
}

static inline BOOL rop_uses_pat(DWORD rop)
{
    return ((rop >> 4) & 0x0f0000) != (rop & 0x0f0000);
}

/***********************************************************************
 *           dibdrv_PutImage
 */
DWORD dibdrv_PutImage( PHYSDEV dev, HBITMAP hbitmap, HRGN clip, BITMAPINFO *info,
                       const struct gdi_image_bits *bits, struct bitblt_coords *src,
                       struct bitblt_coords *dst, DWORD rop )
{
    dib_info *dib, stand_alone;
    DWORD ret;
    POINT origin;
    dib_info src_dib;
    HRGN saved_clip = NULL;
    dibdrv_physdev *pdev = NULL;
    int rop2;

    TRACE( "%p %p %p\n", dev, hbitmap, info );

    if (!hbitmap && rop_uses_pat( rop ))
    {
        PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPutImage );
        FIXME( "rop %08x unsupported, forwarding to graphics driver\n", rop );
        return next->funcs->pPutImage( next, 0, clip, info, bits, src, dst, rop );
    }

    if (hbitmap)
    {
        BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );

        if (!bmp) return ERROR_INVALID_HANDLE;
        assert(bmp->dib);

        if (!init_dib_info( &stand_alone, &bmp->dib->dsBmih, bmp->dib->dsBitfields,
                            bmp->color_table, bmp->nb_colors, bmp->dib->dsBm.bmBits, 0 ))
        {
            ret = ERROR_BAD_FORMAT;
            goto done;
        }
        dib = &stand_alone;
        rop = SRCCOPY;
    }
    else
    {
        pdev = get_dibdrv_pdev( dev );
        dib = &pdev->dib;
    }

    if (info->bmiHeader.biPlanes != 1) goto update_format;
    if (info->bmiHeader.biBitCount != dib->bit_count) goto update_format;
    if (!matching_color_info( dib, info )) goto update_format;
    if (!bits)
    {
        ret = ERROR_SUCCESS;
        goto done;
    }
    if ((src->width != dst->width) || (src->height != dst->height))
    {
        ret = ERROR_TRANSFORM_NOT_SUPPORTED;
        goto done;
    }

    init_dib_info_from_bitmapinfo( &src_dib, info, bits->ptr, 0 );
    src_dib.bits.is_copy = bits->is_copy;

    origin.x = src->visrect.left;
    origin.y = src->visrect.top;

    if (!hbitmap)
    {
        if (clip) saved_clip = add_extra_clipping_region( pdev, clip );
        clip = pdev->clip;
    }

    rop2 = ((rop >> 16) & 0xf) + 1;

    ret = copy_rect( dib, &dst->visrect, &src_dib, &src->visrect, clip, rop2 );

    if (saved_clip) restore_clipping_region( pdev, saved_clip );

    goto done;

update_format:
    info->bmiHeader.biPlanes   = 1;
    info->bmiHeader.biBitCount = dib->bit_count;
    set_color_info( dib, info );
    ret = ERROR_BAD_FORMAT;

done:
    if (hbitmap) GDI_ReleaseObj( hbitmap );

    return ret;
}
