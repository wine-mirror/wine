/*
 * DIB driver primitives.
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

static inline DWORD *get_pixel_ptr_32(const dib_info *dib, int x, int y)
{
    return (DWORD *)((BYTE*)dib->bits.ptr + y * dib->stride + x * 4);
}

static inline DWORD *get_pixel_ptr_24_dword(const dib_info *dib, int x, int y)
{
    return (DWORD *)((BYTE*)dib->bits.ptr + y * dib->stride) + x * 3 / 4;
}

static inline BYTE *get_pixel_ptr_24(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + y * dib->stride + x * 3;
}

static inline WORD *get_pixel_ptr_16(const dib_info *dib, int x, int y)
{
    return (WORD *)((BYTE*)dib->bits.ptr + y * dib->stride + x * 2);
}

static inline BYTE *get_pixel_ptr_8(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + y * dib->stride + x;
}

static inline BYTE *get_pixel_ptr_4(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + y * dib->stride + x / 2;
}

static inline BYTE *get_pixel_ptr_1(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + y * dib->stride + x / 8;
}

static const BYTE pixel_masks_4[2] = {0xf0, 0x0f};
static const BYTE pixel_masks_1[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

static inline void do_rop_32(DWORD *ptr, DWORD and, DWORD xor)
{
    *ptr = (*ptr & and) ^ xor;
}

static inline void do_rop_16(WORD *ptr, WORD and, WORD xor)
{
    *ptr = (*ptr & and) ^ xor;
}

static inline void do_rop_8(BYTE *ptr, BYTE and, BYTE xor)
{
    *ptr = (*ptr & and) ^ xor;
}

static inline void do_rop_mask_8(BYTE *ptr, BYTE and, BYTE xor, BYTE mask)
{
    *ptr = (*ptr & (and | ~mask)) ^ (xor & mask);
}

static inline void do_rop_codes_32(DWORD *dst, DWORD src, struct rop_codes *codes)
{
    do_rop_32( dst, (src & codes->a1) ^ codes->a2, (src & codes->x1) ^ codes->x2 );
}

static inline void do_rop_codes_16(WORD *dst, WORD src, struct rop_codes *codes)
{
    do_rop_16( dst, (src & codes->a1) ^ codes->a2, (src & codes->x1) ^ codes->x2 );
}

static inline void do_rop_codes_8(BYTE *dst, BYTE src, struct rop_codes *codes)
{
    do_rop_8( dst, (src & codes->a1) ^ codes->a2, (src & codes->x1) ^ codes->x2 );
}

static inline void do_rop_codes_mask_8(BYTE *dst, BYTE src, struct rop_codes *codes, BYTE mask)
{
    do_rop_mask_8( dst, (src & codes->a1) ^ codes->a2, (src & codes->x1) ^ codes->x2, mask );
}

static inline void do_rop_codes_line_32(DWORD *dst, const DWORD *src, struct rop_codes *codes, int len)
{
    for (; len > 0; len--, src++, dst++) do_rop_codes_32( dst, *src, codes );
}

static inline void do_rop_codes_line_rev_32(DWORD *dst, const DWORD *src, struct rop_codes *codes, int len)
{
    for (src += len - 1, dst += len - 1; len > 0; len--, src--, dst--)
        do_rop_codes_32( dst, *src, codes );
}

static inline void do_rop_codes_line_16(WORD *dst, const WORD *src, struct rop_codes *codes, int len)
{
    for (; len > 0; len--, src++, dst++) do_rop_codes_16( dst, *src, codes );
}

static inline void do_rop_codes_line_rev_16(WORD *dst, const WORD *src, struct rop_codes *codes, int len)
{
    for (src += len - 1, dst += len - 1; len > 0; len--, src--, dst--)
        do_rop_codes_16( dst, *src, codes );
}

static inline void do_rop_codes_line_8(BYTE *dst, const BYTE *src, struct rop_codes *codes, int len)
{
    for (; len > 0; len--, src++, dst++) do_rop_codes_8( dst, *src, codes );
}

static inline void do_rop_codes_line_rev_8(BYTE *dst, const BYTE *src, struct rop_codes *codes, int len)
{
    for (src += len - 1, dst += len - 1; len > 0; len--, src--, dst--)
        do_rop_codes_8( dst, *src, codes );
}

static inline void do_rop_codes_line_4(BYTE *dst, int dst_x, const BYTE *src, int src_x,
                                      struct rop_codes *codes, int len)
{
    BYTE src_val;

    for (src += src_x / 2, dst += dst_x / 2; len > 0; len--, dst_x++, src_x++)
    {
        if (dst_x & 1)
        {
            if (src_x & 1) src_val = *src++;
            else           src_val = *src >> 4;
            do_rop_codes_mask_8( dst++, src_val, codes, 0x0f );
        }
        else
        {
            if (src_x & 1) src_val = *src++ << 4;
            else           src_val = *src;
            do_rop_codes_mask_8( dst, src_val, codes, 0xf0 );
        }
    }
}

static inline void do_rop_codes_line_rev_4(BYTE *dst, int dst_x, const BYTE *src, int src_x,
                                          struct rop_codes *codes, int len)
{
    BYTE src_val;

    src_x += len - 1;
    dst_x += len - 1;
    for (src += src_x / 2, dst += dst_x / 2; len > 0; len--, dst_x--, src_x--)
    {
        if (dst_x & 1)
        {
            if (src_x & 1) src_val = *src;
            else           src_val = *src-- >> 4;
            do_rop_codes_mask_8( dst, src_val, codes, 0x0f );
        }
        else
        {
            if (src_x & 1) src_val = *src << 4;
            else           src_val = *src--;
            do_rop_codes_mask_8( dst--, src_val, codes, 0xf0 );
        }
    }
}

static inline void do_rop_codes_line_1(BYTE *dst, int dst_x, const BYTE *src, int src_x,
                                      struct rop_codes *codes, int len)
{
    BYTE src_val;

    for (src += src_x / 8, dst += dst_x / 8; len > 0; len--, dst_x++, src_x++)
    {
        src_val = *src & pixel_masks_1[src_x & 7] ? 0xff : 0;
        do_rop_codes_mask_8( dst, src_val, codes, pixel_masks_1[dst_x & 7] );
        if ((src_x & 7) == 7) src++;
        if ((dst_x & 7) == 7) dst++;
    }
}

static inline void do_rop_codes_line_rev_1(BYTE *dst, int dst_x, const BYTE *src, int src_x,
                                          struct rop_codes *codes, int len)
{
    BYTE src_val;

    src_x += len - 1;
    dst_x += len - 1;
    for (src += src_x / 8, dst += dst_x / 8; len > 0; len--, dst_x--, src_x--)
    {
        src_val = *src & pixel_masks_1[src_x & 7] ? 0xff : 0;
        do_rop_codes_mask_8( dst, src_val, codes, pixel_masks_1[dst_x & 7] );
        if ((src_x & 7) == 0) src--;
        if ((dst_x & 7) == 0) dst--;
    }
}

static void solid_rects_32(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    DWORD *ptr, *start;
    int x, y, i;

    for(i = 0; i < num; i++, rc++)
    {
        start = get_pixel_ptr_32(dib, rc->left, rc->top);
        for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
            for(x = rc->left, ptr = start; x < rc->right; x++)
                do_rop_32(ptr++, and, xor);
    }
}

static void solid_rects_24(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    DWORD *ptr, *start;
    BYTE *byte_ptr, *byte_start;
    int x, y, i;
    DWORD and_masks[3], xor_masks[3];

    and_masks[0] = ( and        & 0x00ffffff) | ((and << 24) & 0xff000000);
    and_masks[1] = ((and >>  8) & 0x0000ffff) | ((and << 16) & 0xffff0000);
    and_masks[2] = ((and >> 16) & 0x000000ff) | ((and <<  8) & 0xffffff00);
    xor_masks[0] = ( xor        & 0x00ffffff) | ((xor << 24) & 0xff000000);
    xor_masks[1] = ((xor >>  8) & 0x0000ffff) | ((xor << 16) & 0xffff0000);
    xor_masks[2] = ((xor >> 16) & 0x000000ff) | ((xor <<  8) & 0xffffff00);

    for(i = 0; i < num; i++, rc++)
    {
        if(rc->left >= rc->right) continue;

        if((rc->left & ~3) == (rc->right & ~3)) /* Special case for lines that start and end in the same DWORD triplet */
        {
            byte_start = get_pixel_ptr_24(dib, rc->left, rc->top);
            for(y = rc->top; y < rc->bottom; y++, byte_start += dib->stride)
            {
                for(x = rc->left, byte_ptr = byte_start; x < rc->right; x++)
                {
                    do_rop_8(byte_ptr++, and_masks[0] & 0xff, xor_masks[0] & 0xff);
                    do_rop_8(byte_ptr++, and_masks[1] & 0xff, xor_masks[1] & 0xff);
                    do_rop_8(byte_ptr++, and_masks[2] & 0xff, xor_masks[2] & 0xff);
                }
            }
        }
        else
        {
            start = get_pixel_ptr_24_dword(dib, rc->left, rc->top);
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
            {
                ptr = start;

                switch(rc->left & 3)
                {
                case 1:
                    do_rop_32(ptr++, and_masks[0] | 0x00ffffff, xor_masks[0] & 0xff000000);
                    do_rop_32(ptr++, and_masks[1], xor_masks[1]);
                    do_rop_32(ptr++, and_masks[2], xor_masks[2]);
                    break;
                case 2:
                    do_rop_32(ptr++, and_masks[1] | 0x0000ffff, xor_masks[1] & 0xffff0000);
                    do_rop_32(ptr++, and_masks[2], xor_masks[2]);
                    break;
                case 3:
                    do_rop_32(ptr++, and_masks[2] | 0x000000ff, xor_masks[2] & 0xffffff00);
                    break;
                }

                for(x = (rc->left + 3) & ~3; x < (rc->right & ~3); x += 4)
                {
                    do_rop_32(ptr++, and_masks[0], xor_masks[0]);
                    do_rop_32(ptr++, and_masks[1], xor_masks[1]);
                    do_rop_32(ptr++, and_masks[2], xor_masks[2]);
                }

                switch(rc->right & 3)
                {
                case 1:
                    do_rop_32(ptr, and_masks[0] | 0xff000000, xor_masks[0] & 0x00ffffff);
                    break;
                case 2:
                    do_rop_32(ptr++, and_masks[0], xor_masks[0]);
                    do_rop_32(ptr,   and_masks[1] | 0xffff0000, xor_masks[1] & 0x0000ffff);
                    break;
                case 3:
                    do_rop_32(ptr++, and_masks[0], xor_masks[0]);
                    do_rop_32(ptr++, and_masks[1], xor_masks[1]);
                    do_rop_32(ptr,   and_masks[2] | 0xffffff00, xor_masks[2] & 0x000000ff);
                    break;
                }
            }
        }
    }
}

static void solid_rects_16(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    WORD *ptr, *start;
    int x, y, i;

    for(i = 0; i < num; i++, rc++)
    {
        start = get_pixel_ptr_16(dib, rc->left, rc->top);
        for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 2)
            for(x = rc->left, ptr = start; x < rc->right; x++)
                do_rop_16(ptr++, and, xor);
    }
}

static void solid_rects_8(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    BYTE *ptr, *start;
    int x, y, i;

    for(i = 0; i < num; i++, rc++)
    {
        start = get_pixel_ptr_8(dib, rc->left, rc->top);
        for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            for(x = rc->left, ptr = start; x < rc->right; x++)
                do_rop_8(ptr++, and, xor);
    }
}

static void solid_rects_4(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    BYTE *ptr, *start;
    int x, y, i;
    BYTE byte_and = (and & 0xf) | ((and << 4) & 0xf0);
    BYTE byte_xor = (xor & 0xf) | ((xor << 4) & 0xf0);

    for(i = 0; i < num; i++, rc++)
    {
        if(rc->left >= rc->right) continue;
        start = get_pixel_ptr_4(dib, rc->left, rc->top);
        for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
        {
            ptr = start;
            if(rc->left & 1) /* upper nibble untouched */
                do_rop_8(ptr++, byte_and | 0xf0, byte_xor & 0x0f);

            for(x = (rc->left + 1) & ~1; x < (rc->right & ~1); x += 2)
                do_rop_8(ptr++, byte_and, byte_xor);

            if(rc->right & 1) /* lower nibble untouched */
                do_rop_8(ptr, byte_and | 0x0f, byte_xor & 0xf0);
        }
    }
}

static void solid_rects_1(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    BYTE *ptr, *start;
    int x, y, i;
    BYTE byte_and = (and & 1) ? 0xff : 0;
    BYTE byte_xor = (xor & 1) ? 0xff : 0;
    BYTE start_and, start_xor, end_and, end_xor, mask;
    static const BYTE masks[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

    for(i = 0; i < num; i++, rc++)
    {
        if(rc->left >= rc->right) continue;

        start = get_pixel_ptr_1(dib, rc->left, rc->top);

        if((rc->left & ~7) == (rc->right & ~7)) /* Special case for lines that start and end in the same byte */
        {
            mask = masks[rc->left & 7] & ~masks[rc->right & 7];

            start_and = byte_and | ~mask;
            start_xor = byte_xor & mask;
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                do_rop_8(start, start_and, start_xor);
            }
        }
        else
        {
            mask = masks[rc->left & 7];
            start_and = byte_and | ~mask;
            start_xor = byte_xor & mask;

            mask = masks[rc->right & 7];
            /* This is inverted wrt to start mask, so end_and/xor assignments reflect this */
            end_and = byte_and | mask;
            end_xor = byte_xor & ~mask;

            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                ptr = start;

                if(rc->left & 7)
                    do_rop_8(ptr++, start_and, start_xor);

                for(x = (rc->left + 7) & ~7; x < (rc->right & ~7); x += 8)
                    do_rop_8(ptr++, byte_and, byte_xor);

                if(rc->right & 7)
                    do_rop_8(ptr, end_and, end_xor);
            }
        }
    }
}

static void solid_rects_null(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    return;
}

static void solid_line_32(const dib_info *dib, const POINT *start, const struct line_params *params,
                          DWORD and, DWORD xor)
{
    DWORD *ptr = get_pixel_ptr_32( dib, start->x, start->y );
    int len = params->length, err = params->err_start;
    int major_inc, minor_inc;

    if (params->x_major)
    {
        major_inc = params->x_inc;
        minor_inc = (dib->stride * params->y_inc) / 4;
    }
    else
    {
        major_inc = (dib->stride * params->y_inc) / 4;
        minor_inc = params->x_inc;
    }

    while (len--)
    {
        do_rop_32( ptr, and, xor );
        if (err + params->bias > 0)
        {
            ptr += minor_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
        ptr += major_inc;
    }
}

static void solid_line_24(const dib_info *dib, const POINT *start, const struct line_params *params,
                         DWORD and, DWORD xor)
{
    BYTE *ptr = get_pixel_ptr_24( dib, start->x, start->y );
    int len = params->length, err = params->err_start;
    int major_inc, minor_inc;

    if (params->x_major)
    {
        major_inc = params->x_inc * 3;
        minor_inc = dib->stride * params->y_inc;
    }
    else
    {
        major_inc = dib->stride * params->y_inc;
        minor_inc = params->x_inc * 3;
    }

    while (len--)
    {
        do_rop_8( ptr,     and,       xor );
        do_rop_8( ptr + 1, and >> 8,  xor >> 8 );
        do_rop_8( ptr + 2, and >> 16, xor >> 16 );
        if (err + params->bias > 0)
        {
            ptr += minor_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
        ptr += major_inc;
    }
}

static void solid_line_16(const dib_info *dib, const POINT *start, const struct line_params *params,
                          DWORD and, DWORD xor)
{
    WORD *ptr = get_pixel_ptr_16( dib, start->x, start->y );
    int len = params->length, err = params->err_start;
    int major_inc, minor_inc;

    if (params->x_major)
    {
        major_inc = params->x_inc;
        minor_inc = (dib->stride * params->y_inc) / 2;
    }
    else
    {
        major_inc = (dib->stride * params->y_inc) / 2;
        minor_inc = params->x_inc;
    }

    while (len--)
    {
        do_rop_16( ptr, and, xor );
        if (err + params->bias > 0)
        {
            ptr += minor_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
        ptr += major_inc;
    }
}

static void solid_line_8(const dib_info *dib, const POINT *start, const struct line_params *params,
                         DWORD and, DWORD xor)
{
    BYTE *ptr = get_pixel_ptr_8( dib, start->x, start->y );
    int len = params->length, err = params->err_start;
    int major_inc, minor_inc;

    if (params->x_major)
    {
        major_inc = params->x_inc;
        minor_inc = dib->stride * params->y_inc;
    }
    else
    {
        major_inc = dib->stride * params->y_inc;
        minor_inc = params->x_inc;
    }

    while (len--)
    {
        do_rop_8( ptr, and, xor );
        if (err + params->bias > 0)
        {
            ptr += minor_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
        ptr += major_inc;
    }
}

static void solid_line_4(const dib_info *dib, const POINT *start, const struct line_params *params,
                         DWORD and, DWORD xor)
{
    BYTE *ptr = get_pixel_ptr_4( dib, start->x, start->y );
    int len = params->length, err = params->err_start;
    int x = start->x;

    and = (and & 0x0f) | ((and << 4) & 0xf0);
    xor = (xor & 0x0f) | ((xor << 4) & 0xf0);

    if (params->x_major)
    {
        while (len--)
        {
            do_rop_mask_8( ptr, and, xor, pixel_masks_4[ x % 2 ] );
            if (err + params->bias > 0)
            {
                ptr += dib->stride * params->y_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
            if ((x / 2) != ((x + params->x_inc) / 2))
                ptr += params->x_inc;
            x += params->x_inc;
        }
    }
    else
    {
        while (len--)
        {
            do_rop_mask_8( ptr, and, xor, pixel_masks_4[ x % 2 ] );
            if (err + params->bias > 0)
            {
                if ((x / 2) != ((x + params->x_inc) / 2))
                    ptr += params->x_inc;
                x += params->x_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
            ptr += dib->stride * params->y_inc;
        }
    }
}

static void solid_line_1(const dib_info *dib, const POINT *start, const struct line_params *params,
                         DWORD and, DWORD xor)
{
    BYTE *ptr = get_pixel_ptr_1( dib, start->x, start->y );
    int len = params->length, err = params->err_start;
    int x = start->x;

    and = (and & 0x1) ? 0xff : 0;
    xor = (xor & 0x1) ? 0xff : 0;

    if (params->x_major)
    {
        while (len--)
        {
            do_rop_mask_8( ptr, and, xor, pixel_masks_1[ x % 8 ] );
            if (err + params->bias > 0)
            {
                ptr += dib->stride * params->y_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
            if ((x / 8) != ((x + params->x_inc) / 8))
                ptr += params->x_inc;
            x += params->x_inc;
        }
    }
    else
    {
        while (len--)
        {
            do_rop_mask_8( ptr, and, xor, pixel_masks_1[ x % 8 ] );
            if (err + params->bias > 0)
            {
                if ((x / 8) != ((x + params->x_inc) / 8))
                    ptr += params->x_inc;
                x += params->x_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
            ptr += dib->stride * params->y_inc;
        }
    }
}

static void solid_line_null(const dib_info *dib, const POINT *start, const struct line_params *params,
                            DWORD and, DWORD xor)
{
    return;
}

static inline INT calc_offset(INT edge, INT size, INT origin)
{
    INT offset;

    if(edge - origin >= 0)
        offset = (edge - origin) % size;
    else
    {
        offset = (origin - edge) % size;
        if(offset) offset = size - offset;
    }
    return offset;
}

static inline POINT calc_brush_offset(const RECT *rc, const dib_info *brush, const POINT *origin)
{
    POINT offset;

    offset.x = calc_offset(rc->left, brush->width,  origin->x);
    offset.y = calc_offset(rc->top,  brush->height, origin->y);

    return offset;
}

static void pattern_rects_32(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                             const dib_info *brush, void *and_bits, void *xor_bits)
{
    DWORD *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_32(dib, rc->left, rc->top);
        start_and = (DWORD*)and_bits + offset.y * brush->stride / 4;
        start_xor = (DWORD*)xor_bits + offset.y * brush->stride / 4;

        for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
        {
            and_ptr = start_and + offset.x;
            xor_ptr = start_xor + offset.x;

            for(x = rc->left, ptr = start; x < rc->right; x++)
            {
                do_rop_32(ptr++, *and_ptr++, *xor_ptr++);
                if(and_ptr == start_and + brush->width)
                {
                    and_ptr = start_and;
                    xor_ptr = start_xor;
                }
            }

            offset.y++;
            if(offset.y == brush->height)
            {
                start_and = and_bits;
                start_xor = xor_bits;
                offset.y = 0;
            }
            else
            {
                start_and += brush->stride / 4;
                start_xor += brush->stride / 4;
            }
        }
    }
}

static void pattern_rects_24(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                             const dib_info *brush, void *and_bits, void *xor_bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_24(dib, rc->left, rc->top);
        start_and = (BYTE*)and_bits + offset.y * brush->stride;
        start_xor = (BYTE*)xor_bits + offset.y * brush->stride;

        for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
        {
            and_ptr = start_and + offset.x * 3;
            xor_ptr = start_xor + offset.x * 3;

            for(x = rc->left, ptr = start; x < rc->right; x++)
            {
                do_rop_8(ptr++, *and_ptr++, *xor_ptr++);
                do_rop_8(ptr++, *and_ptr++, *xor_ptr++);
                do_rop_8(ptr++, *and_ptr++, *xor_ptr++);
                if(and_ptr == start_and + brush->width * 3)
                {
                    and_ptr = start_and;
                    xor_ptr = start_xor;
                }
            }

            offset.y++;
            if(offset.y == brush->height)
            {
                start_and = and_bits;
                start_xor = xor_bits;
                offset.y = 0;
            }
            else
            {
                start_and += brush->stride;
                start_xor += brush->stride;
            }
        }
    }
}

static void pattern_rects_16(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                             const dib_info *brush, void *and_bits, void *xor_bits)
{
    WORD *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_16(dib, rc->left, rc->top);
        start_and = (WORD*)and_bits + offset.y * brush->stride / 2;
        start_xor = (WORD*)xor_bits + offset.y * brush->stride / 2;

        for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 2)
        {
            and_ptr = start_and + offset.x;
            xor_ptr = start_xor + offset.x;

            for(x = rc->left, ptr = start; x < rc->right; x++)
            {
                do_rop_16(ptr++, *and_ptr++, *xor_ptr++);
                if(and_ptr == start_and + brush->width)
                {
                    and_ptr = start_and;
                    xor_ptr = start_xor;
                }
            }

            offset.y++;
            if(offset.y == brush->height)
            {
                start_and = and_bits;
                start_xor = xor_bits;
                offset.y = 0;
            }
            else
            {
                start_and += brush->stride / 2;
                start_xor += brush->stride / 2;
            }
        }
    }
}

static void pattern_rects_8(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                            const dib_info *brush, void *and_bits, void *xor_bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_8(dib, rc->left, rc->top);
        start_and = (BYTE*)and_bits + offset.y * brush->stride;
        start_xor = (BYTE*)xor_bits + offset.y * brush->stride;

        for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
        {
            and_ptr = start_and + offset.x;
            xor_ptr = start_xor + offset.x;

            for(x = rc->left, ptr = start; x < rc->right; x++)
            {
                do_rop_8(ptr++, *and_ptr++, *xor_ptr++);
                if(and_ptr == start_and + brush->width)
                {
                    and_ptr = start_and;
                    xor_ptr = start_xor;
                }
            }

            offset.y++;
            if(offset.y == brush->height)
            {
                start_and = and_bits;
                start_xor = xor_bits;
                offset.y = 0;
            }
            else
            {
                start_and += brush->stride;
                start_xor += brush->stride;
            }
        }
    }
}

static void pattern_rects_4(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                            const dib_info *brush, void *and_bits, void *xor_bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_4(dib, rc->left, rc->top);
        start_and = (BYTE*)and_bits + offset.y * brush->stride;
        start_xor = (BYTE*)xor_bits + offset.y * brush->stride;

        for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
        {
            INT brush_x = offset.x;
            BYTE byte_and, byte_xor;

            and_ptr = start_and + brush_x / 2;
            xor_ptr = start_xor + brush_x / 2;

            for(x = rc->left, ptr = start; x < rc->right; x++)
            {
                /* FIXME: Two pixels at a time */
                if(x & 1) /* lower dst nibble */
                {
                    if(brush_x & 1) /* lower pat nibble */
                    {
                        byte_and = *and_ptr++ | 0xf0;
                        byte_xor = *xor_ptr++ & 0x0f;
                    }
                    else /* upper pat nibble */
                    {
                        byte_and = (*and_ptr >> 4) | 0xf0;
                        byte_xor = (*xor_ptr >> 4) & 0x0f;
                    }
                }
                else /* upper dst nibble */
                {
                    if(brush_x & 1) /* lower pat nibble */
                    {
                        byte_and = (*and_ptr++ << 4) | 0x0f;
                        byte_xor = (*xor_ptr++ << 4) & 0xf0;
                    }
                    else /* upper pat nibble */
                    {
                        byte_and = *and_ptr | 0x0f;
                        byte_xor = *xor_ptr & 0xf0;
                    }
                }
                do_rop_8(ptr, byte_and, byte_xor);

                if(x & 1) ptr++;

                if(++brush_x == brush->width)
                {
                    brush_x = 0;
                    and_ptr = start_and;
                    xor_ptr = start_xor;
                }
            }

            offset.y++;
            if(offset.y == brush->height)
            {
                start_and = and_bits;
                start_xor = xor_bits;
                offset.y = 0;
            }
            else
            {
                start_and += brush->stride;
                start_xor += brush->stride;
            }
        }
    }
}

static void pattern_rects_1(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                            const dib_info *brush, void *and_bits, void *xor_bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_1(dib, rc->left, rc->top);
        start_and = (BYTE*)and_bits + offset.y * brush->stride;
        start_xor = (BYTE*)xor_bits + offset.y * brush->stride;

        for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
        {
            INT brush_x = offset.x;
            BYTE byte_and, byte_xor;

            and_ptr = start_and + brush_x / 8;
            xor_ptr = start_xor + brush_x / 8;

            for(x = rc->left, ptr = start; x < rc->right; x++)
            {
                byte_and = (*and_ptr & pixel_masks_1[brush_x % 8]) ? 0xff : 0;
                byte_and |= ~pixel_masks_1[x % 8];
                byte_xor = (*xor_ptr & pixel_masks_1[brush_x % 8]) ? 0xff : 0;
                byte_xor &= pixel_masks_1[x % 8];

                do_rop_8(ptr, byte_and, byte_xor);

                if((x & 7) == 7) ptr++;

                if((brush_x & 7) == 7)
                {
                    and_ptr++;
                    xor_ptr++;
                }

                if(++brush_x == brush->width)
                {
                    brush_x = 0;
                    and_ptr = start_and;
                    xor_ptr = start_xor;
                }
            }

            offset.y++;
            if(offset.y == brush->height)
            {
                start_and = and_bits;
                start_xor = xor_bits;
                offset.y = 0;
            }
            else
            {
                start_and += brush->stride;
                start_xor += brush->stride;
            }
        }
    }
}

static void pattern_rects_null(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                               const dib_info *brush, void *and_bits, void *xor_bits)
{
    return;
}

static void copy_rect_32(const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    DWORD *dst_start, *src_start;
    struct rop_codes codes;
    int y, dst_stride, src_stride;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_32(dst, rc->left, rc->bottom - 1);
        src_start = get_pixel_ptr_32(src, origin->x, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride / 4;
        src_stride = -src->stride / 4;
    }
    else
    {
        dst_start = get_pixel_ptr_32(dst, rc->left, rc->top);
        src_start = get_pixel_ptr_32(src, origin->x, origin->y);
        dst_stride = dst->stride / 4;
        src_stride = src->stride / 4;
    }

    if (rop2 == R2_COPYPEN)
    {
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (rc->right - rc->left) * 4 );
        return;
    }

    get_rop_codes( rop2, &codes );
    for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
    {
        if (overlap & OVERLAP_RIGHT)
            do_rop_codes_line_rev_32( dst_start, src_start, &codes, rc->right - rc->left );
        else
            do_rop_codes_line_32( dst_start, src_start, &codes, rc->right - rc->left );
    }
}

static void copy_rect_24(const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    BYTE *dst_start, *src_start;
    int y, dst_stride, src_stride;
    struct rop_codes codes;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_24(dst, rc->left, rc->bottom - 1);
        src_start = get_pixel_ptr_24(src, origin->x, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride;
        src_stride = -src->stride;
    }
    else
    {
        dst_start = get_pixel_ptr_24(dst, rc->left, rc->top);
        src_start = get_pixel_ptr_24(src, origin->x, origin->y);
        dst_stride = dst->stride;
        src_stride = src->stride;
    }

    if (rop2 == R2_COPYPEN)
    {
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (rc->right - rc->left) * 3 );
        return;
    }

    get_rop_codes( rop2, &codes );
    for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
    {
        if (overlap & OVERLAP_RIGHT)
            do_rop_codes_line_rev_8( dst_start, src_start, &codes, (rc->right - rc->left) * 3 );
        else
            do_rop_codes_line_8( dst_start, src_start, &codes, (rc->right - rc->left) * 3 );
    }
}

static void copy_rect_16(const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    WORD *dst_start, *src_start;
    int y, dst_stride, src_stride;
    struct rop_codes codes;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_16(dst, rc->left, rc->bottom - 1);
        src_start = get_pixel_ptr_16(src, origin->x, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride / 2;
        src_stride = -src->stride / 2;
    }
    else
    {
        dst_start = get_pixel_ptr_16(dst, rc->left, rc->top);
        src_start = get_pixel_ptr_16(src, origin->x, origin->y);
        dst_stride = dst->stride / 2;
        src_stride = src->stride / 2;
    }

    if (rop2 == R2_COPYPEN)
    {
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (rc->right - rc->left) * 2 );
        return;
    }

    get_rop_codes( rop2, &codes );
    for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
    {
        if (overlap & OVERLAP_RIGHT)
            do_rop_codes_line_rev_16( dst_start, src_start, &codes, rc->right - rc->left );
        else
            do_rop_codes_line_16( dst_start, src_start, &codes, rc->right - rc->left );
    }
}

static void copy_rect_8(const dib_info *dst, const RECT *rc,
                        const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    BYTE *dst_start, *src_start;
    int y, dst_stride, src_stride;
    struct rop_codes codes;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_8(dst, rc->left, rc->bottom - 1);
        src_start = get_pixel_ptr_8(src, origin->x, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride;
        src_stride = -src->stride;
    }
    else
    {
        dst_start = get_pixel_ptr_8(dst, rc->left, rc->top);
        src_start = get_pixel_ptr_8(src, origin->x, origin->y);
        dst_stride = dst->stride;
        src_stride = src->stride;
    }

    if (rop2 == R2_COPYPEN)
    {
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (rc->right - rc->left) );
        return;
    }

    get_rop_codes( rop2, &codes );
    for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
    {
        if (overlap & OVERLAP_RIGHT)
            do_rop_codes_line_rev_8( dst_start, src_start, &codes, rc->right - rc->left );
        else
            do_rop_codes_line_8( dst_start, src_start, &codes, rc->right - rc->left );
    }
}

static void copy_rect_4(const dib_info *dst, const RECT *rc,
                        const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    BYTE *dst_start, *src_start;
    int y, dst_stride, src_stride;
    struct rop_codes codes;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_4(dst, 0, rc->bottom - 1);
        src_start = get_pixel_ptr_4(src, 0, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride;
        src_stride = -src->stride;
    }
    else
    {
        dst_start = get_pixel_ptr_4(dst, 0, rc->top);
        src_start = get_pixel_ptr_4(src, 0, origin->y);
        dst_stride = dst->stride;
        src_stride = src->stride;
    }

    if (rop2 == R2_COPYPEN && (rc->left & 1) == 0 && (origin->x & 1) == 0 && (rc->right & 1) == 0)
    {
        dst_start += rc->left / 2;
        src_start += origin->x / 2;
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (rc->right - rc->left) / 2 );
        return;
    }

    get_rop_codes( rop2, &codes );
    for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
    {
        if (overlap & OVERLAP_RIGHT)
            do_rop_codes_line_rev_4( dst_start, rc->left, src_start, origin->x, &codes, rc->right - rc->left );
        else
            do_rop_codes_line_4( dst_start, rc->left, src_start, origin->x, &codes, rc->right - rc->left );
    }
}

static void copy_rect_1(const dib_info *dst, const RECT *rc,
                        const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    BYTE *dst_start, *src_start;
    int y, dst_stride, src_stride;
    struct rop_codes codes;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_1(dst, 0, rc->bottom - 1);
        src_start = get_pixel_ptr_1(src, 0, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride;
        src_stride = -src->stride;
    }
    else
    {
        dst_start = get_pixel_ptr_1(dst, 0, rc->top);
        src_start = get_pixel_ptr_1(src, 0, origin->y);
        dst_stride = dst->stride;
        src_stride = src->stride;
    }

    if (rop2 == R2_COPYPEN && (rc->left & 7) == 0 && (origin->x & 7) == 0 && (rc->right & 7) == 0)
    {
        dst_start += rc->left / 8;
        src_start += origin->x / 8;
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (rc->right - rc->left) / 8 );
        return;
    }

    get_rop_codes( rop2, &codes );
    for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
    {
        if (overlap & OVERLAP_RIGHT)
            do_rop_codes_line_rev_1( dst_start, rc->left, src_start, origin->x, &codes, rc->right - rc->left );
        else
            do_rop_codes_line_1( dst_start, rc->left, src_start, origin->x, &codes, rc->right - rc->left );
    }
}

static void copy_rect_null(const dib_info *dst, const RECT *rc,
                           const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    return;
}

static DWORD get_pixel_32(const dib_info *dib, const POINT *pt)
{
    DWORD *ptr = get_pixel_ptr_32( dib, pt->x, pt->y );
    return *ptr;
}

static DWORD get_pixel_24(const dib_info *dib, const POINT *pt)
{
    BYTE *ptr = get_pixel_ptr_24( dib, pt->x, pt->y );
    return ptr[0] | ((DWORD)ptr[1] << 8) | ((DWORD)ptr[2] << 16);
}

static DWORD get_pixel_16(const dib_info *dib, const POINT *pt)
{
    WORD *ptr = get_pixel_ptr_16( dib, pt->x, pt->y );
    return *ptr;
}

static DWORD get_pixel_8(const dib_info *dib, const POINT *pt)
{
    BYTE *ptr = get_pixel_ptr_8( dib, pt->x, pt->y );
    return *ptr;
}

static DWORD get_pixel_4(const dib_info *dib, const POINT *pt)
{
    BYTE *ptr = get_pixel_ptr_4( dib, pt->x, pt->y );

    if (pt->x & 1)
        return *ptr & 0x0f;
    else
        return (*ptr >> 4) & 0x0f;
}

static DWORD get_pixel_1(const dib_info *dib, const POINT *pt)
{
    BYTE *ptr = get_pixel_ptr_1( dib, pt->x, pt->y );
    return (*ptr & pixel_masks_1[pt->x & 0x7]) ? 1 : 0;
}

static DWORD get_pixel_null(const dib_info *dib, const POINT *pt)
{
    return 0;
}

static DWORD colorref_to_pixel_888(const dib_info *dib, COLORREF color)
{
    return ( ((color >> 16) & 0xff) | (color & 0xff00) | ((color << 16) & 0xff0000) );
}

static const DWORD field_masks[33] =
{
    0x00,  /* should never happen */
    0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static inline DWORD get_field(DWORD field, int shift, int len)
{
    shift = shift - (8 - len);
    if (shift < 0)
        field <<= -shift;
    else
        field >>= shift;
    field &= field_masks[len];
    field |= field >> len;
    return field;
}

static inline DWORD put_field(DWORD field, int shift, int len)
{
    shift = shift - (8 - len);
    field &= field_masks[len];
    if (shift < 0)
        field >>= -shift;
    else
        field <<= shift;
    return field;
}

static DWORD colorref_to_pixel_masks(const dib_info *dib, COLORREF colour)
{
    DWORD r,g,b;

    r = GetRValue(colour);
    g = GetGValue(colour);
    b = GetBValue(colour);

    return put_field(r, dib->red_shift,   dib->red_len) |
           put_field(g, dib->green_shift, dib->green_len) |
           put_field(b, dib->blue_shift,  dib->blue_len);
}

static DWORD colorref_to_pixel_555(const dib_info *dib, COLORREF color)
{
    return ( ((color >> 19) & 0x1f) | ((color >> 6) & 0x03e0) | ((color << 7) & 0x7c00) );
}

static DWORD rgb_to_pixel_colortable(const dib_info *dib, BYTE r, BYTE g, BYTE b)
{
    int i, best_index = 0;
    DWORD diff, best_diff = 0xffffffff;

    /* special case for conversion to 1-bpp without a color table:
     * we get a 1-entry table containing the background color
     */
    if (dib->bit_count == 1 && dib->color_table_size == 1)
        return (r == dib->color_table[0].rgbRed &&
                g == dib->color_table[0].rgbGreen &&
                b == dib->color_table[0].rgbBlue);

    for(i = 0; i < dib->color_table_size; i++)
    {
        RGBQUAD *cur = dib->color_table + i;
        diff = (r - cur->rgbRed)   * (r - cur->rgbRed)
            +  (g - cur->rgbGreen) * (g - cur->rgbGreen)
            +  (b - cur->rgbBlue)  * (b - cur->rgbBlue);

        if(diff == 0)
        {
            best_index = i;
            break;
        }

        if(diff < best_diff)
        {
            best_diff = diff;
            best_index = i;
        }
    }
    return best_index;
}

static DWORD colorref_to_pixel_colortable(const dib_info *dib, COLORREF color)
{
    return rgb_to_pixel_colortable( dib, GetRValue(color), GetGValue(color), GetBValue(color) );
}

static DWORD colorref_to_pixel_null(const dib_info *dib, COLORREF color)
{
    return 0;
}

static inline RGBQUAD colortable_entry(const dib_info *dib, DWORD index)
{
    static const RGBQUAD default_rgb;
    if (index < dib->color_table_size) return dib->color_table[index];
    return default_rgb;
}

static COLORREF pixel_to_colorref_888(const dib_info *dib, DWORD pixel)
{
    return ( ((pixel >> 16) & 0xff) | (pixel & 0xff00) | ((pixel << 16) & 0xff0000) );
}

static COLORREF pixel_to_colorref_masks(const dib_info *dib, DWORD pixel)
{
    return RGB( get_field( pixel, dib->red_shift,   dib->red_len ),
                get_field( pixel, dib->green_shift, dib->green_len ),
                get_field( pixel, dib->blue_shift,  dib->blue_len ) );
}

static COLORREF pixel_to_colorref_555(const dib_info *dib, DWORD pixel)
{
    return RGB( ((pixel >> 7) & 0xf8) | ((pixel >> 12) & 0x07),
                ((pixel >> 2) & 0xf8) | ((pixel >>  7) & 0x07),
                ((pixel << 3) & 0xf8) | ((pixel >>  2) & 0x07) );
}

static COLORREF pixel_to_colorref_colortable(const dib_info *dib, DWORD pixel)
{
    RGBQUAD quad = colortable_entry( dib, pixel );

    return RGB( quad.rgbRed, quad.rgbGreen, quad.rgbBlue );
}

static COLORREF pixel_to_colorref_null(const dib_info *dib, DWORD pixel)
{
    return 0;
}

static inline BOOL bit_fields_match(const dib_info *d1, const dib_info *d2)
{
    assert( d1->bit_count > 8 && d1->bit_count == d2->bit_count );

    return d1->red_mask   == d2->red_mask &&
           d1->green_mask == d2->green_mask &&
           d1->blue_mask  == d2->blue_mask;
}

static void convert_to_8888(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    DWORD *dst_start = get_pixel_ptr_32(dst, 0, 0), *dst_pixel, src_val;
    int x, y, pad_size = (dst->width - (src_rect->right - src_rect->left)) * 4;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_8888)
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst_start, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 4);
                    if(pad_size) memset(dst_start + (src_rect->right - src_rect->left), 0, pad_size);
                    dst_start += dst->stride / 4;
                    src_start += src->stride / 4;
                }
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->red_shift)   & 0xff) << 16) |
                                   (((src_val >> src->green_shift) & 0xff) <<  8) |
                                    ((src_val >> src->blue_shift)  & 0xff);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (get_field( src_val, src->red_shift, src->red_len ) << 16 |
                                    get_field( src_val, src->green_shift, src->green_len ) << 8 |
                                    get_field( src_val, src->blue_shift, src->blue_len ));
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                rgb.rgbBlue  = *src_pixel++;
                rgb.rgbGreen = *src_pixel++;
                rgb.rgbRed   = *src_pixel++;

                *dst_pixel++ = ((rgb.rgbRed << 16) & 0xff0000) | ((rgb.rgbGreen << 8) & 0x00ff00) | (rgb.rgbBlue & 0x0000ff);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = ((src_val << 9) & 0xf80000) | ((src_val << 4) & 0x070000) |
                                   ((src_val << 6) & 0x00f800) | ((src_val << 1) & 0x000700) |
                                   ((src_val << 3) & 0x0000f8) | ((src_val >> 2) & 0x000007);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->red_shift)   << 19) & 0xf80000) |
                                   (((src_val >> src->red_shift)   << 14) & 0x070000) |
                                   (((src_val >> src->green_shift) << 11) & 0x00f800) |
                                   (((src_val >> src->green_shift) <<  6) & 0x000700) |
                                   (((src_val >> src->blue_shift)  <<  3) & 0x0000f8) |
                                   (((src_val >> src->blue_shift)  >>  2) & 0x000007);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->red_shift)   << 19) & 0xf80000) |
                                   (((src_val >> src->red_shift)   << 14) & 0x070000) |
                                   (((src_val >> src->green_shift) << 10) & 0x00fc00) |
                                   (((src_val >> src->green_shift) <<  4) & 0x000300) |
                                   (((src_val >> src->blue_shift)  <<  3) & 0x0000f8) |
                                   (((src_val >> src->blue_shift)  >>  2) & 0x000007);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (get_field( src_val, src->red_shift, src->red_len ) << 16 |
                                    get_field( src_val, src->green_shift, src->green_len ) << 8 |
                                    get_field( src_val, src->blue_shift, src->blue_len ));
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                *dst_pixel++ = rgb.rgbRed << 16 | rgb.rgbGreen << 8 | rgb.rgbBlue;
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    rgb = colortable_entry( src, *src_pixel++ & 0xf );
                else
                    rgb = colortable_entry( src, *src_pixel >> 4 );
                *dst_pixel++ = rgb.rgbRed << 16 | rgb.rgbGreen << 8 | rgb.rgbBlue;
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                *dst_pixel++ = rgb.rgbRed << 16 | rgb.rgbGreen << 8 | rgb.rgbBlue;
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_32(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    DWORD *dst_start = get_pixel_ptr_32(dst, 0, 0), *dst_pixel, src_val;
    int x, y, pad_size = (dst->width - (src_rect->right - src_rect->left)) * 4;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;

        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(src_val >> 16, dst->red_shift,   dst->red_len)   |
                                   put_field(src_val >>  8, dst->green_shift, dst->green_len) |
                                   put_field(src_val,       dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 4;
            }
        }
        else if(bit_fields_match(src, dst))
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst_start, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 4);
                    if(pad_size) memset(dst_start + (src_rect->right - src_rect->left), 0, pad_size);
                    dst_start += dst->stride / 4;
                    src_start += src->stride / 4;
                }
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8 &&
                dst->red_len == 8 && dst->green_len == 8 && dst->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->red_shift)   & 0xff) << dst->red_shift)   |
                                   (((src_val >> src->green_shift) & 0xff) << dst->green_shift) |
                                   (((src_val >> src->blue_shift)  & 0xff) << dst->blue_shift);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(get_field(src_val, src->red_shift,   src->red_len ),   dst->red_shift,   dst->red_len)   |
                                   put_field(get_field(src_val, src->green_shift, src->green_len ), dst->green_shift, dst->green_len) |
                                   put_field(get_field(src_val, src->blue_shift,  src->blue_len ),  dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                rgb.rgbBlue  = *src_pixel++;
                rgb.rgbGreen = *src_pixel++;
                rgb.rgbRed   = *src_pixel++;

                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len)   |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07), dst->red_shift,   dst->red_len) |
                                   put_field(((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07), dst->green_shift, dst->green_len) |
                                   put_field(((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07), dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field( (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                              (((src_val >> src->red_shift)   >> 2) & 0x07), dst->red_shift, dst->red_len ) |
                                   put_field( (((src_val >> src->green_shift) << 3) & 0xf8) |
                                              (((src_val >> src->green_shift) >> 2) & 0x07), dst->green_shift, dst->green_len ) |
                                   put_field( (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                              (((src_val >> src->blue_shift)  >> 2) & 0x07), dst->blue_shift, dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field( (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                              (((src_val >> src->red_shift)   >> 2) & 0x07), dst->red_shift, dst->red_len ) |
                                   put_field( (((src_val >> src->green_shift) << 2) & 0xfc) |
                                              (((src_val >> src->green_shift) >> 4) & 0x03), dst->green_shift, dst->green_len ) |
                                   put_field( (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                              (((src_val >> src->blue_shift)  >> 2) & 0x07), dst->blue_shift, dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(get_field(src_val, src->red_shift,   src->red_len ),   dst->red_shift,   dst->red_len)   |
                                   put_field(get_field(src_val, src->green_shift, src->green_len ), dst->green_shift, dst->green_len) |
                                   put_field(get_field(src_val, src->blue_shift,  src->blue_len ),  dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    rgb = colortable_entry( src, *src_pixel++ & 0xf );
                else
                    rgb = colortable_entry( src, *src_pixel >> 4 );
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_24(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = get_pixel_ptr_24(dst, 0, 0), *dst_pixel;
    DWORD src_val;
    int x, y, pad_size = ((dst->width * 3 + 3) & ~3) - (src_rect->right - src_rect->left) * 3;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ =  src_val        & 0xff;
                    *dst_pixel++ = (src_val >>  8) & 0xff;
                    *dst_pixel++ = (src_val >> 16) & 0xff;
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (src_val >> src->blue_shift)  & 0xff;
                    *dst_pixel++ = (src_val >> src->green_shift) & 0xff;
                    *dst_pixel++ = (src_val >> src->red_shift)   & 0xff;
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = get_field( src_val, src->blue_shift, src->blue_len );
                    *dst_pixel++ = get_field( src_val, src->green_shift, src->green_len );
                    *dst_pixel++ = get_field( src_val, src->red_shift, src->red_len );
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top);

        if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
            memcpy(dst_start, src_start, (src_rect->bottom - src_rect->top) * src->stride);
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 3);
                if(pad_size) memset(dst_start + (src_rect->right - src_rect->left) * 3, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride;
            }
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = ((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07);
                    *dst_pixel++ = ((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07);
                    *dst_pixel++ = ((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                   (((src_val >> src->blue_shift)  >> 2) & 0x07);
                    *dst_pixel++ = (((src_val >> src->green_shift) << 3) & 0xf8) |
                                   (((src_val >> src->green_shift) >> 2) & 0x07);
                    *dst_pixel++ = (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                   (((src_val >> src->red_shift)   >> 2) & 0x07);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                   (((src_val >> src->blue_shift)  >> 2) & 0x07);
                    *dst_pixel++ = (((src_val >> src->green_shift) << 2) & 0xfc) |
                                   (((src_val >> src->green_shift) >> 4) & 0x03);
                    *dst_pixel++ = (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                   (((src_val >> src->red_shift)   >> 2) & 0x07);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = get_field(src_val, src->blue_shift,  src->blue_len );
                    *dst_pixel++ = get_field(src_val, src->green_shift, src->green_len );
                    *dst_pixel++ = get_field(src_val, src->red_shift,   src->red_len );
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                *dst_pixel++ = rgb.rgbBlue;
                *dst_pixel++ = rgb.rgbGreen;
                *dst_pixel++ = rgb.rgbRed;
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    rgb = colortable_entry( src, *src_pixel++ & 0xf );
                else
                    rgb = colortable_entry( src, *src_pixel >> 4 );
                *dst_pixel++ = rgb.rgbBlue;
                *dst_pixel++ = rgb.rgbGreen;
                *dst_pixel++ = rgb.rgbRed;
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                *dst_pixel++ = rgb.rgbBlue;
                *dst_pixel++ = rgb.rgbGreen;
                *dst_pixel++ = rgb.rgbRed;
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_555(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    WORD *dst_start = get_pixel_ptr_16(dst, 0, 0), *dst_pixel;
    INT x, y, pad_size = ((dst->width + 1) & ~1) * 2 - (src_rect->right - src_rect->left) * 2;
    DWORD src_val;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;

        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = ((src_val >> 9) & 0x7c00) |
                                   ((src_val >> 6) & 0x03e0) |
                                   ((src_val >> 3) & 0x001f);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->red_shift)   << 7) & 0x7c00) |
                                   (((src_val >> src->green_shift) << 2) & 0x03e0) |
                                   (((src_val >> src->blue_shift)  >> 3) & 0x001f);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((get_field(src_val, src->red_shift,   src->red_len )   << 7) & 0x7c00) |
                                    ((get_field(src_val, src->green_shift, src->green_len ) << 2) & 0x03e0) |
                                    ( get_field(src_val, src->blue_shift,  src->blue_len )  >> 3));
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                rgb.rgbBlue  = *src_pixel++;
                rgb.rgbGreen = *src_pixel++;
                rgb.rgbRed   = *src_pixel++;

                *dst_pixel++ = ((rgb.rgbRed   << 7) & 0x7c00) |
                               ((rgb.rgbGreen << 2) & 0x03e0) |
                               ((rgb.rgbBlue  >> 3) & 0x001f);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst_start, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 2);
                    if(pad_size) memset(dst_start + (src_rect->right - src_rect->left), 0, pad_size);
                    dst_start += dst->stride / 2;
                    src_start += src->stride / 2;
                }
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->red_shift)   << 10) & 0x7c00) |
                                   (((src_val >> src->green_shift) <<  5) & 0x03e0) |
                                   ( (src_val >> src->blue_shift)         & 0x001f);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((src_val >> src->red_shift)   << 10) & 0x7c00) |
                                   (((src_val >> src->green_shift) <<  4) & 0x03e0) |
                                   ( (src_val >> src->blue_shift)         & 0x001f);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = (((get_field(src_val, src->red_shift, src->red_len)     << 7) & 0x7c00) |
                                    ((get_field(src_val, src->green_shift, src->green_len) << 2) & 0x03e0) |
                                    ( get_field(src_val, src->blue_shift, src->blue_len)   >> 3));
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                *dst_pixel++ = ((rgb.rgbRed   << 7) & 0x7c00) |
                               ((rgb.rgbGreen << 2) & 0x03e0) |
                               ((rgb.rgbBlue  >> 3) & 0x001f);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    rgb = colortable_entry( src, *src_pixel++ & 0xf );
                else
                    rgb = colortable_entry( src, *src_pixel >> 4 );
                *dst_pixel++ = ((rgb.rgbRed   << 7) & 0x7c00) |
                               ((rgb.rgbGreen << 2) & 0x03e0) |
                               ((rgb.rgbBlue  >> 3) & 0x001f);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                *dst_pixel++ = ((rgb.rgbRed   << 7) & 0x7c00) |
                               ((rgb.rgbGreen << 2) & 0x03e0) |
                               ((rgb.rgbBlue  >> 3) & 0x001f);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_16(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    WORD *dst_start = get_pixel_ptr_16(dst, 0, 0), *dst_pixel;
    INT x, y, pad_size = ((dst->width + 1) & ~1) * 2 - (src_rect->right - src_rect->left) * 2;
    DWORD src_val;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;

        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(src_val >> 16, dst->red_shift,   dst->red_len)   |
                                   put_field(src_val >>  8, dst->green_shift, dst->green_len) |
                                   put_field(src_val,       dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(src_val >> src->red_shift,   dst->red_shift,   dst->red_len)   |
                                   put_field(src_val >> src->green_shift, dst->green_shift, dst->green_len) |
                                   put_field(src_val >> src->blue_shift,  dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(get_field(src_val, src->red_shift,   src->red_len ),   dst->red_shift,   dst->red_len)   |
                                   put_field(get_field(src_val, src->green_shift, src->green_len ), dst->green_shift, dst->green_len) |
                                   put_field(get_field(src_val, src->blue_shift,  src->blue_len ),  dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                rgb.rgbBlue  = *src_pixel++;
                rgb.rgbGreen = *src_pixel++;
                rgb.rgbRed   = *src_pixel++;

                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07), dst->red_shift,   dst->red_len) |
                                   put_field(((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07), dst->green_shift, dst->green_len) |
                                   put_field(((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07), dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        else if(bit_fields_match(src, dst))
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst_start, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 2);
                    if(pad_size) memset(dst_start + (src_rect->right - src_rect->left), 0, pad_size);
                    dst_start += dst->stride / 2;
                    src_start += src->stride / 2;
                }
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field( (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                              (((src_val >> src->red_shift)   >> 2) & 0x07), dst->red_shift, dst->red_len ) |
                                   put_field( (((src_val >> src->green_shift) << 3) & 0xf8) |
                                              (((src_val >> src->green_shift) >> 2) & 0x07), dst->green_shift, dst->green_len ) |
                                   put_field( (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                              (((src_val >> src->blue_shift)  >> 2) & 0x07), dst->blue_shift, dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field( (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                              (((src_val >> src->red_shift)   >> 2) & 0x07), dst->red_shift, dst->red_len ) |
                                   put_field( (((src_val >> src->green_shift) << 2) & 0xfc) |
                                              (((src_val >> src->green_shift) >> 4) & 0x03), dst->green_shift, dst->green_len ) |
                                   put_field( (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                              (((src_val >> src->blue_shift)  >> 2) & 0x07), dst->blue_shift, dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = put_field(get_field(src_val, src->red_shift,   src->red_len ),   dst->red_shift,   dst->red_len)   |
                                   put_field(get_field(src_val, src->green_shift, src->green_len ), dst->green_shift, dst->green_len) |
                                   put_field(get_field(src_val, src->blue_shift,  src->blue_len ),  dst->blue_shift,  dst->blue_len);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    rgb = colortable_entry( src, *src_pixel++ & 0xf );
                else
                    rgb = colortable_entry( src, *src_pixel >> 4 );
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }
    }
}

static inline BOOL color_tables_match(const dib_info *d1, const dib_info *d2)
{
    assert(d1->color_table_size && d2->color_table_size);

    if(d1->color_table_size != d2->color_table_size) return FALSE;
    return !memcmp(d1->color_table, d2->color_table, d1->color_table_size * sizeof(d1->color_table[0]));
}

static inline DWORD rgb_lookup_colortable(const dib_info *dst, BYTE r, BYTE g, BYTE b)
{
    /* Windows reduces precision to 5 bits, probably in order to build some sort of lookup cache */
    return rgb_to_pixel_colortable( dst, (r & ~7) + 4, (g & ~7) + 4, (b & ~7) + 4 );
}

static void convert_to_8(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = get_pixel_ptr_8(dst, 0, 0), *dst_pixel;
    INT x, y, pad_size = ((dst->width + 3) & ~3) - (src_rect->right - src_rect->left);
    DWORD src_val;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;

        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(dst, src_val >> 16, src_val >> 8, src_val );
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(dst,
                                                         src_val >> src->red_shift,
                                                         src_val >> src->green_shift,
                                                         src_val >> src->blue_shift );
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(dst,
                                                         get_field(src_val, src->red_shift, src->red_len),
                                                         get_field(src_val, src->green_shift, src->green_len),
                                                         get_field(src_val, src->blue_shift, src->blue_len));
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++, src_pixel += 3)
            {
                *dst_pixel++ = rgb_lookup_colortable(dst, src_pixel[2], src_pixel[1], src_pixel[0] );
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(dst,
                                                         ((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07),
                                                         ((src_val >> 2) & 0xf8) | ((src_val >> 7) & 0x07),
                                                         ((src_val << 3) & 0xf8) | ((src_val >> 2) & 0x07) );
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(dst,
                                                         (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                         (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                         (((src_val >> src->green_shift) << 3) & 0xf8) |
                                                         (((src_val >> src->green_shift) >> 2) & 0x07),
                                                         (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                         (((src_val >> src->blue_shift)  >> 2) & 0x07) );
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(dst,
                                                         (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                         (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                         (((src_val >> src->green_shift) << 2) & 0xfc) |
                                                         (((src_val >> src->green_shift) >> 4) & 0x03),
                                                         (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                         (((src_val >> src->blue_shift)  >> 2) & 0x07) );
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(dst,
                                                         get_field(src_val, src->red_shift, src->red_len),
                                                         get_field(src_val, src->green_shift, src->green_len),
                                                         get_field(src_val, src->blue_shift, src->blue_len));
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;

        if(color_tables_match(dst, src))
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst_start, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, src_rect->right - src_rect->left);
                    if(pad_size) memset(dst_start + (src_rect->right - src_rect->left), 0, pad_size);
                    dst_start += dst->stride;
                    src_start += src->stride;
                }
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                    *dst_pixel++ = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride;
            }
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    rgb = colortable_entry( src, *src_pixel++ & 0xf );
                else
                    rgb = colortable_entry( src, *src_pixel >> 4 );
                *dst_pixel++ = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                *dst_pixel++ = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
            }
            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_4(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = get_pixel_ptr_4(dst, 0, 0), *dst_pixel, dst_val;
    INT x, y, pad_size = ((dst->width + 7) & ~7) / 2 - (src_rect->right - src_rect->left + 1) / 2;
    DWORD src_val;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;

        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst, src_val >> 16, src_val >> 8, src_val);
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      src_val >> src->red_shift,
                                                      src_val >> src->green_shift,
                                                      src_val >> src->blue_shift);
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      get_field(src_val, src->red_shift, src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift, src->blue_len));
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++, src_pixel += 3)
            {
                dst_val = rgb_to_pixel_colortable(dst, src_pixel[2], src_pixel[1], src_pixel[0]);

                if((x - src_rect->left) & 1)
                {
                    *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                    dst_pixel++;
                }
                else
                    *dst_pixel = (dst_val << 4) & 0xf0;
            }
            if(pad_size)
            {
                if((x - src_rect->left) & 1) dst_pixel++;
                memset(dst_pixel, 0, pad_size);
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      ((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07),
                                                      ((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07),
                                                      ((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07) );
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 3) & 0xf8) |
                                                      (((src_val >> src->green_shift) >> 2) & 0x07),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07) );
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 2) & 0xfc) |
                                                      (((src_val >> src->green_shift) >> 4) & 0x03),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07) );
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      get_field(src_val, src->red_shift, src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift, src->blue_len));
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                dst_val = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
                if((x - src_rect->left) & 1)
                {
                    *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                    dst_pixel++;
                }
                else
                    *dst_pixel = (dst_val << 4) & 0xf0;
            }
            if(pad_size)
            {
                if((x - src_rect->left) & 1) dst_pixel++;
                memset(dst_pixel, 0, pad_size);
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;

        if(color_tables_match(dst, src) && (src_rect->left & 1) == 0)
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst_start, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left + 1) / 2);
                    if(pad_size) memset(dst_start + (src_rect->right - src_rect->left + 1) / 2, 0, pad_size);
                    dst_start += dst->stride;
                    src_start += src->stride;
                }
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    RGBQUAD rgb;
                    if(x & 1)
                        rgb = colortable_entry( src, *src_pixel++ & 0xf );
                    else
                        rgb = colortable_entry( src, *src_pixel >> 4 );
                    dst_val = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                if(pad_size)
                {
                    if((x - src_rect->left) & 1) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride;
            }
        }
        break;
    }

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                dst_val = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
                if((x - src_rect->left) & 1)
                {
                    *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                    dst_pixel++;
                }
                else
                    *dst_pixel = (dst_val << 4) & 0xf0;
            }
            if(pad_size)
            {
                if((x - src_rect->left) & 1) dst_pixel++;
                memset(dst_pixel, 0, pad_size);
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_1(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = get_pixel_ptr_1(dst, 0, 0), *dst_pixel, dst_val;
    INT x, y, pad_size = ((dst->width + 31) & ~31) / 8 - (src_rect->right - src_rect->left + 7) / 8;
    DWORD src_val;
    int bit_pos;

    /* FIXME: Brushes should be dithered. */

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;

        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst, src_val >> 16, src_val >> 8, src_val) ? 0xff : 0;

                    if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                if(pad_size)
                {
                    if(bit_pos != 0) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else if(src->red_len == 8 && src->green_len == 8 && src->blue_len == 8)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      src_val >> src->red_shift,
                                                      src_val >> src->green_shift,
                                                      src_val >> src->blue_shift) ? 0xff : 0;

                   if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                if(pad_size)
                {
                    if(bit_pos != 0) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      get_field(src_val, src->red_shift, src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift, src->blue_len)) ? 0xff : 0;

                   if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                if(pad_size)
                {
                    if(bit_pos != 0) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++, src_pixel += 3)
            {
                dst_val = rgb_to_pixel_colortable(dst, src_pixel[2], src_pixel[1], src_pixel[0]) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
            }
            if(pad_size)
            {
                if(bit_pos != 0) dst_pixel++;
                memset(dst_pixel, 0, pad_size);
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top), *src_pixel;
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      ((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07),
                                                      ((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07),
                                                      ((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07) ) ? 0xff : 0;

                    if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                if(pad_size)
                {
                    if(bit_pos != 0) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 5 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 3) & 0xf8) |
                                                      (((src_val >> src->green_shift) >> 2) & 0x07),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07) ) ? 0xff : 0;
                    if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                if(pad_size)
                {
                    if(bit_pos != 0) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else if(src->red_len == 5 && src->green_len == 6 && src->blue_len == 5)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 2) & 0xfc) |
                                                      (((src_val >> src->green_shift) >> 4) & 0x03),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07) ) ? 0xff : 0;
                    if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                if(pad_size)
                {
                    if(bit_pos != 0) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_colortable(dst,
                                                      get_field(src_val, src->red_shift, src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift, src->blue_len)) ? 0xff : 0;
                    if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                if(pad_size)
                {
                    if(bit_pos != 0) dst_pixel++;
                    memset(dst_pixel, 0, pad_size);
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
            {
                RGBQUAD rgb = colortable_entry( src, *src_pixel++ );
                dst_val = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
            }
            if(pad_size)
            {
                if(bit_pos != 0) dst_pixel++;
                memset(dst_pixel, 0, pad_size);
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    rgb = colortable_entry( src, *src_pixel++ & 0xf );
                else
                    rgb = colortable_entry( src, *src_pixel >> 4 );
                dst_val = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
            }
            if(pad_size)
            {
                if(bit_pos != 0) dst_pixel++;
                memset(dst_pixel, 0, pad_size);
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    /* Note that while MSDN states that a 1 bpp dib brush -> mono dc
       uses text/bkgnd colours instead of the dib's colour table, this
       doesn't appear to be the case for a dc backed by a
       dibsection. */

    case 1:
    {
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                src_val = (*src_pixel & pixel_masks_1[x % 8]) ? 1 : 0;
                if((x % 8) == 7) src_pixel++;
                rgb = src->color_table[src_val];
                dst_val = rgb_to_pixel_colortable(dst, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
            }
            if(pad_size)
            {
                if(bit_pos != 0) dst_pixel++;
                memset(dst_pixel, 0, pad_size);
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_null(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
}

static inline BYTE blend_color(BYTE dst, BYTE src, DWORD alpha)
{
    return (src * alpha + dst * (255 - alpha) + 127) / 255;
}

static inline DWORD blend_argb( DWORD dst, DWORD src, DWORD alpha )
{
    return (blend_color( dst, src, alpha ) |
            blend_color( dst >> 8, src >> 8, alpha ) << 8 |
            blend_color( dst >> 16, src >> 16, alpha ) << 16 |
            blend_color( dst >> 24, src >> 24, alpha ) << 24);
}

static inline DWORD blend_argb_alpha( DWORD dst, DWORD src, DWORD alpha )
{
    BYTE b = ((BYTE)src         * alpha + 127) / 255;
    BYTE g = ((BYTE)(src >> 8)  * alpha + 127) / 255;
    BYTE r = ((BYTE)(src >> 16) * alpha + 127) / 255;
    alpha  = ((BYTE)(src >> 24) * alpha + 127) / 255;
    return ((b     + ((BYTE)dst         * (255 - alpha) + 127) / 255) |
            (g     + ((BYTE)(dst >> 8)  * (255 - alpha) + 127) / 255) << 8 |
            (r     + ((BYTE)(dst >> 16) * (255 - alpha) + 127) / 255) << 16 |
            (alpha + ((BYTE)(dst >> 24) * (255 - alpha) + 127) / 255) << 24);
}

static inline DWORD blend_rgb( BYTE dst_r, BYTE dst_g, BYTE dst_b, DWORD src, BLENDFUNCTION blend )
{
    if (blend.AlphaFormat & AC_SRC_ALPHA)
    {
        DWORD alpha = blend.SourceConstantAlpha;
        BYTE src_b = ((BYTE)src         * alpha + 127) / 255;
        BYTE src_g = ((BYTE)(src >> 8)  * alpha + 127) / 255;
        BYTE src_r = ((BYTE)(src >> 16) * alpha + 127) / 255;
        alpha      = ((BYTE)(src >> 24) * alpha + 127) / 255;
        return ((src_b + (dst_b * (255 - alpha) + 127) / 255) |
                (src_g + (dst_g * (255 - alpha) + 127) / 255) << 8 |
                (src_r + (dst_r * (255 - alpha) + 127) / 255) << 16);
    }
    return (blend_color( dst_b, src, blend.SourceConstantAlpha ) |
            blend_color( dst_g, src >> 8, blend.SourceConstantAlpha ) << 8 |
            blend_color( dst_r, src >> 16, blend.SourceConstantAlpha ) << 16);
}

static void blend_rect_8888(const dib_info *dst, const RECT *rc,
                            const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x, origin->y );
    DWORD *dst_ptr = get_pixel_ptr_32( dst, rc->left, rc->top );
    int x, y;

    if (blend.AlphaFormat & AC_SRC_ALPHA)
        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
            for (x = 0; x < rc->right - rc->left; x++)
                dst_ptr[x] = blend_argb_alpha( dst_ptr[x], src_ptr[x], blend.SourceConstantAlpha );
    else
        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
            for (x = 0; x < rc->right - rc->left; x++)
                dst_ptr[x] = blend_argb( dst_ptr[x], src_ptr[x], blend.SourceConstantAlpha );
}

static void blend_rect_32(const dib_info *dst, const RECT *rc,
                          const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x, origin->y );
    DWORD *dst_ptr = get_pixel_ptr_32( dst, rc->left, rc->top );
    int x, y;

    if (dst->red_len == 8 && dst->green_len == 8 && dst->blue_len == 8)
    {
        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                DWORD val = blend_rgb( dst_ptr[x] >> dst->red_shift,
                                       dst_ptr[x] >> dst->green_shift,
                                       dst_ptr[x] >> dst->blue_shift,
                                       src_ptr[x], blend );
                dst_ptr[x] = ((( val        & 0xff) << dst->blue_shift) |
                              (((val >> 8)  & 0xff) << dst->green_shift) |
                              (((val >> 16) & 0xff) << dst->red_shift));
            }
        }
    }
    else
    {
        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                DWORD val = blend_rgb( get_field( dst_ptr[x], dst->red_shift, dst->red_len ),
                                       get_field( dst_ptr[x], dst->green_shift, dst->green_len ),
                                       get_field( dst_ptr[x], dst->blue_shift, dst->blue_len ),
                                       src_ptr[x], blend );
                dst_ptr[x] = (put_field( val >> 16, dst->red_shift,   dst->red_len )   |
                              put_field( val >> 8,  dst->green_shift, dst->green_len ) |
                              put_field( val,       dst->blue_shift,  dst->blue_len ));
            }
        }
    }
}

static void blend_rect_24(const dib_info *dst, const RECT *rc,
                          const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x, origin->y );
    BYTE *dst_ptr = get_pixel_ptr_24( dst, rc->left, rc->top );
    int x, y;

    for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride, src_ptr += src->stride / 4)
    {
        for (x = 0; x < rc->right - rc->left; x++)
        {
            DWORD val = blend_rgb( dst_ptr[x * 3 + 2], dst_ptr[x * 3 + 1], dst_ptr[x * 3],
                                   src_ptr[x], blend );
            dst_ptr[x * 3]     = val;
            dst_ptr[x * 3 + 1] = val >> 8;
            dst_ptr[x * 3 + 2] = val >> 16;
        }
    }
}

static void blend_rect_555(const dib_info *dst, const RECT *rc,
                           const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x, origin->y );
    WORD *dst_ptr = get_pixel_ptr_16( dst, rc->left, rc->top );
    int x, y;

    for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 2, src_ptr += src->stride / 4)
    {
        for (x = 0; x < rc->right - rc->left; x++)
        {
            DWORD val = blend_rgb( ((dst_ptr[x] >> 7) & 0xf8) | ((dst_ptr[x] >> 12) & 0x07),
                                   ((dst_ptr[x] >> 2) & 0xf8) | ((dst_ptr[x] >>  7) & 0x07),
                                   ((dst_ptr[x] << 3) & 0xf8) | ((dst_ptr[x] >>  2) & 0x07),
                                   src_ptr[x], blend );
            dst_ptr[x] = ((val >> 9) & 0x7c00) | ((val >> 6) & 0x03e0) | ((val >> 3) & 0x001f);
        }
    }
}

static void blend_rect_16(const dib_info *dst, const RECT *rc,
                          const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x, origin->y );
    WORD *dst_ptr = get_pixel_ptr_16( dst, rc->left, rc->top );
    int x, y;

    for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 2, src_ptr += src->stride / 4)
    {
        for (x = 0; x < rc->right - rc->left; x++)
        {
            DWORD val = blend_rgb( get_field( dst_ptr[x], dst->red_shift, dst->red_len ),
                                   get_field( dst_ptr[x], dst->green_shift, dst->green_len ),
                                   get_field( dst_ptr[x], dst->blue_shift, dst->blue_len ),
                                   src_ptr[x], blend );
            dst_ptr[x] = (put_field((val >> 16), dst->red_shift,   dst->red_len)   |
                          put_field((val >>  8), dst->green_shift, dst->green_len) |
                          put_field( val,        dst->blue_shift,  dst->blue_len));
        }
    }
}

static void blend_rect_8(const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x, origin->y );
    BYTE *dst_ptr = get_pixel_ptr_8( dst, rc->left, rc->top );
    int x, y;

    for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride, src_ptr += src->stride / 4)
    {
        for (x = 0; x < rc->right - rc->left; x++)
        {
            RGBQUAD rgb = colortable_entry( dst, dst_ptr[x] );
            DWORD val = blend_rgb( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue, src_ptr[x], blend );
            dst_ptr[x] = rgb_lookup_colortable( dst, val >> 16, val >> 8, val );
        }
    }
}

static void blend_rect_4(const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x - rc->left, origin->y );
    BYTE *dst_ptr = get_pixel_ptr_4( dst, 0, rc->top );
    int x, y;

    for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride, src_ptr += src->stride / 4)
    {
        for (x = rc->left; x < rc->right; x++)
        {
            DWORD val = ((x & 1) ? dst_ptr[x / 2] : (dst_ptr[x / 2] >> 4)) & 0x0f;
            RGBQUAD rgb = colortable_entry( dst, val );
            val = blend_rgb( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue, src_ptr[x], blend );
            val = rgb_lookup_colortable( dst, val >> 16, val >> 8, val );
            if (x & 1)
                dst_ptr[x / 2] = val | (dst_ptr[x / 2] & 0xf0);
            else
                dst_ptr[x / 2] = (val << 4) | (dst_ptr[x / 2] & 0x0f);
        }
    }
}

static void blend_rect_1(const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
    DWORD *src_ptr = get_pixel_ptr_32( src, origin->x - rc->left, origin->y );
    BYTE *dst_ptr = get_pixel_ptr_1( dst, 0, rc->top );
    int x, y;

    for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride, src_ptr += src->stride / 4)
    {
        for (x = rc->left; x < rc->right; x++)
        {
            DWORD val = (dst_ptr[x / 8] & pixel_masks_1[x % 8]) ? 1 : 0;
            RGBQUAD rgb = dst->color_table[val];
            val = blend_rgb( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue, src_ptr[x], blend );
            val = rgb_to_pixel_colortable(dst, val >> 16, val >> 8, val) ? 0xff : 0;
            dst_ptr[x / 8] = (dst_ptr[x / 8] & ~pixel_masks_1[x % 8]) | (val & pixel_masks_1[x % 8]);
        }
    }
}

static void blend_rect_null(const dib_info *dst, const RECT *rc,
                            const dib_info *src, const POINT *origin, BLENDFUNCTION blend)
{
}

static inline DWORD gradient_rgb_8888( const TRIVERTEX *v, unsigned int pos, unsigned int len )
{
    BYTE r, g, b, a;
    r = (v[0].Red   * (len - pos) + v[1].Red   * pos) / len / 256;
    g = (v[0].Green * (len - pos) + v[1].Green * pos) / len / 256;
    b = (v[0].Blue  * (len - pos) + v[1].Blue  * pos) / len / 256;
    a = (v[0].Alpha * (len - pos) + v[1].Alpha * pos) / len / 256;
    return a << 24 | r << 16 | g << 8 | b;
}

static inline DWORD gradient_rgb_24( const TRIVERTEX *v, unsigned int pos, unsigned int len )
{
    BYTE r, g, b;
    r = (v[0].Red   * (len - pos) + v[1].Red   * pos) / len / 256;
    g = (v[0].Green * (len - pos) + v[1].Green * pos) / len / 256;
    b = (v[0].Blue  * (len - pos) + v[1].Blue  * pos) / len / 256;
    return r << 16 | g << 8 | b;
}

static inline WORD gradient_rgb_555( const TRIVERTEX *v, unsigned int pos, unsigned int len,
                                     unsigned int x, unsigned int y )
{
    static const BYTE matrix[4][4] =
    {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 }
    };
    int r = (v[0].Red   * (len - pos) + v[1].Red   * pos) / len / 128 + matrix[y % 4][x % 4];
    int g = (v[0].Green * (len - pos) + v[1].Green * pos) / len / 128 + matrix[y % 4][x % 4];
    int b = (v[0].Blue  * (len - pos) + v[1].Blue  * pos) / len / 128 + matrix[y % 4][x % 4];
    r = min( 31, max( 0, r / 16 ));
    g = min( 31, max( 0, g / 16 ));
    b = min( 31, max( 0, b / 16 ));
    return (r << 10) | (g << 5) | b;
}

static void gradient_rect_8888( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    DWORD *ptr = get_pixel_ptr_32( dib, rc->left, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (x = 0; x < rc->right - rc->left; x++)
            ptr[x] = gradient_rgb_8888( v, x + rc->left - v[0].x, v[1].x - v[0].x );

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride / 4)
            memcpy( ptr + dib->stride / 4, ptr, (rc->right - rc->left) * 4 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++)
        {
            DWORD val = gradient_rgb_8888( v, y - v[0].y, v[1].y - v[0].y );
            for (x = 0; x < rc->right - rc->left; x++) ptr[x] = val;
            ptr += dib->stride / 4;
        }
        break;
    }
}

static void gradient_rect_32( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    DWORD *ptr = get_pixel_ptr_32( dib, rc->left, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        if (dib->red_len == 8 && dib->green_len == 8 && dib->blue_len == 8)
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                DWORD val = gradient_rgb_24( v, x + rc->left - v[0].x, v[1].x - v[0].x );
                ptr[x] = ((( val        & 0xff) << dib->blue_shift) |
                          (((val >> 8)  & 0xff) << dib->green_shift) |
                          (((val >> 16) & 0xff) << dib->red_shift));
            }
        }
        else
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                DWORD val = gradient_rgb_24( v, x + rc->left - v[0].x, v[1].x - v[0].x );
                ptr[x] = (put_field( val >> 16, dib->red_shift,   dib->red_len )   |
                          put_field( val >> 8,  dib->green_shift, dib->green_len ) |
                          put_field( val,       dib->blue_shift,  dib->blue_len ));
            }
        }

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride / 4)
            memcpy( ptr + dib->stride / 4, ptr, (rc->right - rc->left) * 4 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++)
        {
            DWORD val = gradient_rgb_24( v, y - v[0].y, v[1].y - v[0].y );
            if (dib->red_len == 8 && dib->green_len == 8 && dib->blue_len == 8)
                val = ((( val        & 0xff) << dib->blue_shift) |
                       (((val >> 8)  & 0xff) << dib->green_shift) |
                       (((val >> 16) & 0xff) << dib->red_shift));
            else
                val = (put_field( val >> 16, dib->red_shift,   dib->red_len )   |
                       put_field( val >> 8,  dib->green_shift, dib->green_len ) |
                       put_field( val,       dib->blue_shift,  dib->blue_len ));

            for (x = 0; x < rc->right - rc->left; x++) ptr[x] = val;
            ptr += dib->stride / 4;
        }
        break;
    }
}

static void gradient_rect_24( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_24( dib, rc->left, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (x = 0; x < rc->right - rc->left; x++)
        {
            DWORD val = gradient_rgb_24( v, x + rc->left - v[0].x, v[1].x - v[0].x );
            ptr[x * 3]     = val;
            ptr[x * 3 + 1] = val >> 8;
            ptr[x * 3 + 2] = val >> 16;
        }

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride)
            memcpy( ptr + dib->stride, ptr, (rc->right - rc->left) * 3 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++)
        {
            DWORD val = gradient_rgb_24( v, y - v[0].y, v[1].y - v[0].y );
            for (x = 0; x < rc->right - rc->left; x++)
            {
                ptr[x * 3]     = val;
                ptr[x * 3 + 1] = val >> 8;
                ptr[x * 3 + 2] = val >> 16;
            }
            ptr += dib->stride;
        }
        break;
    }
}

static void gradient_rect_555( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    WORD *ptr = get_pixel_ptr_16( dib, 0, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (y = rc->top; y < min( rc->top + 4, rc->bottom ); y++, ptr += dib->stride / 2)
            for (x = rc->left; x < rc->right; x++)
                ptr[x] = gradient_rgb_555( v, x - v[0].x, v[1].x - v[0].x, x, y );
        for (ptr += rc->left; y < rc->bottom; y++, ptr += dib->stride / 2)
            memcpy( ptr, ptr - dib->stride * 2, (rc->right - rc->left) * 2 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 2)
        {
            WORD values[4];
            for (x = 0; x < 4; x++) values[x] = gradient_rgb_555( v, y - v[0].y, v[1].y - v[0].y, x, y );
            for (x = rc->left; x < rc->right; x++) ptr[x] = values[x % 4];
        }
        break;
    }
}

static void gradient_rect_16( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    WORD *ptr = get_pixel_ptr_16( dib, 0, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (y = rc->top; y < min( rc->top + 4, rc->bottom ); y++, ptr += dib->stride / 2)
            for (x = rc->left; x < rc->right; x++)
            {
                WORD val = gradient_rgb_555( v, x - v[0].x, v[1].x - v[0].x, x, y );
                ptr[x] = (put_field(((val >> 7) & 0xf8) | ((val >> 12) & 0x07), dib->red_shift,   dib->red_len)   |
                          put_field(((val >> 2) & 0xf8) | ((val >> 7)  & 0x07), dib->green_shift, dib->green_len) |
                          put_field(((val << 3) & 0xf8) | ((val >> 2)  & 0x07), dib->blue_shift,  dib->blue_len));

            }
        for (ptr += rc->left; y < rc->bottom; y++, ptr += dib->stride / 2)
            memcpy( ptr, ptr - dib->stride * 2, (rc->right - rc->left) * 2 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 2)
        {
            WORD values[4];
            for (x = 0; x < 4; x++)
            {
                WORD val = gradient_rgb_555( v, y - v[0].y, v[1].y - v[0].y, x, y );
                values[x] = (put_field(((val >> 7) & 0xf8) | ((val >> 12) & 0x07), dib->red_shift,   dib->red_len)   |
                             put_field(((val >> 2) & 0xf8) | ((val >> 7)  & 0x07), dib->green_shift, dib->green_len) |
                             put_field(((val << 3) & 0xf8) | ((val >> 2)  & 0x07), dib->blue_shift,  dib->blue_len));
            }
            for (x = rc->left; x < rc->right; x++) ptr[x] = values[x % 4];
        }
        break;
    }
}

static void gradient_rect_8( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_8( dib, rc->left, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (x = 0; x < rc->right - rc->left; x++)
        {
            DWORD val = gradient_rgb_24( v, x + rc->left - v[0].x, v[1].x - v[0].x );
            ptr[x] = rgb_lookup_colortable( dib, val >> 16, val >> 8, val );
        }

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride)
            memcpy( ptr + dib->stride, ptr, rc->right - rc->left );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++)
        {
            DWORD val = gradient_rgb_24( v, y - v[0].y, v[1].y - v[0].y );
            val = rgb_lookup_colortable( dib, val >> 16, val >> 8, val );
            for (x = 0; x < rc->right - rc->left; x++) ptr[x] = val;
            ptr += dib->stride;
        }
        break;
    }
}

static void gradient_rect_4( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_4( dib, 0, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (x = rc->left; x < rc->right; x++)
        {
            DWORD val = gradient_rgb_24( v, x - v[0].x, v[1].x - v[0].x );
            val = rgb_lookup_colortable( dib, val >> 16, val >> 8, val );
            if (x & 1)
                ptr[x / 2] = val | (ptr[x / 2] & 0xf0);
            else
                ptr[x / 2] = (val << 4) | (ptr[x / 2] & 0x0f);
        }

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride)
        {
            x = rc->left;
            if (x & 1)
            {
                ptr[dib->stride + x / 2] = (ptr[x / 2] & 0x0f) | (ptr[dib->stride + x / 2] & 0xf0);
                x++;
            }
            for (; x < rc->right - 1; x += 2) ptr[dib->stride + x / 2] = ptr[x / 2];
            if (x < rc->right)
                ptr[dib->stride + x / 2] = (ptr[dib->stride + x / 2] & 0x0f) | (ptr[x / 2] & 0xf0);
        }
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++)
        {
            DWORD val = gradient_rgb_24( v, y - v[0].y, v[1].y - v[0].y );
            val = rgb_lookup_colortable( dib, val >> 16, val >> 8, val );
            x = rc->left;
            if (x & 1)
            {
                ptr[x / 2] = val | (ptr[x / 2] & 0xf0);
                x++;
            }
            for (; x < rc->right - 1; x += 2) ptr[x / 2] = (val << 4) | val;
            if (x < rc->right) ptr[x / 2] = (ptr[x / 2] & 0x0f) | (val << 4);
            ptr += dib->stride;
        }
        break;
    }
}

static void gradient_rect_1( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_1( dib, 0, rc->top );
    int x, y;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (x = rc->left; x < rc->right; x++)
        {
            DWORD val = gradient_rgb_24( v, x - v[0].x, v[1].x - v[0].x );
            val = rgb_to_pixel_colortable( dib, val >> 16, val >> 8, val ) ? 0xff : 0;
            ptr[x / 8] = (ptr[x / 8] & ~pixel_masks_1[x % 8]) | (val & pixel_masks_1[x % 8]);
        }

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride)
            for (x = rc->left; x < rc->right; x++)
                ptr[dib->stride + x / 8] = (ptr[dib->stride + x / 8] & ~pixel_masks_1[x % 8]) |
                                           (ptr[x / 8] & pixel_masks_1[x % 8]);
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++)
        {
            DWORD val = gradient_rgb_24( v, y - v[0].y, v[1].y - v[0].y );
            val = rgb_to_pixel_colortable( dib, val >> 16, val >> 8, val ) ? 0xff : 0;
            for (x = rc->left; x < rc->right; x++)
                ptr[x / 8] = (ptr[x / 8] & ~pixel_masks_1[x % 8]) | (val & pixel_masks_1[x % 8]);
            ptr += dib->stride;
        }
        break;
    }
}

static void gradient_rect_null( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
}

static inline BYTE aa_color( BYTE dst, BYTE text, BYTE min_comp, BYTE max_comp )
{
    if (dst == text) return dst;

    if (dst > text)
    {
        DWORD diff = dst - text;
        DWORD range = max_comp - text;
        dst = text + (diff * range ) / (0xff - text);
        return dst;
    }
    else
    {
        DWORD diff = text - dst;
        DWORD range = text - min_comp;
        dst = text - (diff * range) / text;
        return dst;
    }
}

static inline DWORD aa_rgb( BYTE r_dst, BYTE g_dst, BYTE b_dst, DWORD text, const struct intensity_range *range )
{
    return (aa_color( b_dst, text,       range->b_min, range->b_max )      |
            aa_color( g_dst, text >> 8,  range->g_min, range->g_max ) << 8 |
            aa_color( r_dst, text >> 16, range->r_min, range->r_max ) << 16);
}

static void draw_glyph_8888( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                             const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    DWORD *dst_ptr = get_pixel_ptr_32( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] <= 1) continue;
            if (glyph_ptr[x] >= 16) { dst_ptr[x] = text_pixel; continue; }
            dst_ptr[x] = aa_rgb( dst_ptr[x] >> 16, dst_ptr[x] >> 8, dst_ptr[x], text_pixel, ranges + glyph_ptr[x] );
        }
        dst_ptr += dib->stride / 4;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_32( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                           const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    DWORD *dst_ptr = get_pixel_ptr_32( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y;
    DWORD text, val;

    text = get_field( text_pixel, dib->red_shift,   dib->red_len ) << 16 |
           get_field( text_pixel, dib->green_shift, dib->green_len ) << 8 |
           get_field( text_pixel, dib->blue_shift,  dib->blue_len );

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] <= 1) continue;
            if (glyph_ptr[x] >= 16) { dst_ptr[x] = text_pixel; continue; }
            val = aa_rgb( get_field(dst_ptr[x], dib->red_shift,   dib->red_len),
                          get_field(dst_ptr[x], dib->green_shift, dib->green_len),
                          get_field(dst_ptr[x], dib->blue_shift,  dib->blue_len),
                          text, ranges + glyph_ptr[x] );
            dst_ptr[x] = (put_field( val >> 16, dib->red_shift,   dib->red_len )   |
                          put_field( val >> 8,  dib->green_shift, dib->green_len ) |
                          put_field( val,       dib->blue_shift,  dib->blue_len ));
        }
        dst_ptr += dib->stride / 4;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_24( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                           const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    BYTE *dst_ptr = get_pixel_ptr_24( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y;
    DWORD val;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] <= 1) continue;
            if (glyph_ptr[x] >= 16)
                val = text_pixel;
            else
                val = aa_rgb( dst_ptr[x * 3 + 2], dst_ptr[x * 3 + 1], dst_ptr[x * 3],
                              text_pixel, ranges + glyph_ptr[x] );
            dst_ptr[x * 3]     = val;
            dst_ptr[x * 3 + 1] = val >> 8;
            dst_ptr[x * 3 + 2] = val >> 16;
        }
        dst_ptr += dib->stride;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_555( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                            const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    WORD *dst_ptr = get_pixel_ptr_16( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y;
    DWORD text, val;

    text = ((text_pixel << 9) & 0xf80000) | ((text_pixel << 4) & 0x070000) |
           ((text_pixel << 6) & 0x00f800) | ((text_pixel << 1) & 0x000700) |
           ((text_pixel << 3) & 0x0000f8) | ((text_pixel >> 2) & 0x000007);

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] <= 1) continue;
            if (glyph_ptr[x] >= 16) { dst_ptr[x] = text_pixel; continue; }
            val = aa_rgb( ((dst_ptr[x] >> 7) & 0xf8) | ((dst_ptr[x] >> 12) & 0x07),
                          ((dst_ptr[x] >> 2) & 0xf8) | ((dst_ptr[x] >>  7) & 0x07),
                          ((dst_ptr[x] << 3) & 0xf8) | ((dst_ptr[x] >>  2) & 0x07),
                          text, ranges + glyph_ptr[x] );
            dst_ptr[x] = ((val >> 9) & 0x7c00) | ((val >> 6) & 0x03e0) | ((val >> 3) & 0x001f);
        }
        dst_ptr += dib->stride / 2;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_16( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                           const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    WORD *dst_ptr = get_pixel_ptr_16( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y;
    DWORD text, val;

    text = get_field( text_pixel, dib->red_shift,   dib->red_len ) << 16 |
           get_field( text_pixel, dib->green_shift, dib->green_len ) << 8 |
           get_field( text_pixel, dib->blue_shift,  dib->blue_len );

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] <= 1) continue;
            if (glyph_ptr[x] >= 16) { dst_ptr[x] = text_pixel; continue; }
            val = aa_rgb( get_field(dst_ptr[x], dib->red_shift,   dib->red_len),
                          get_field(dst_ptr[x], dib->green_shift, dib->green_len),
                          get_field(dst_ptr[x], dib->blue_shift,  dib->blue_len),
                          text, ranges + glyph_ptr[x] );
            dst_ptr[x] = (put_field( val >> 16, dib->red_shift,   dib->red_len )   |
                          put_field( val >> 8,  dib->green_shift, dib->green_len ) |
                          put_field( val,       dib->blue_shift,  dib->blue_len ));
        }
        dst_ptr += dib->stride / 2;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_8( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                          const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    BYTE *dst_ptr = get_pixel_ptr_8( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            /* no antialiasing, glyph should only contain 0 or 16. */
            if (glyph_ptr[x] >= 16)
                dst_ptr[x] = text_pixel;
        }
        dst_ptr += dib->stride;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_4( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                          const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    BYTE *dst_ptr = get_pixel_ptr_4( dib, 0, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x - rect->left, origin->y );
    int x, y;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = rect->left; x < rect->right; x++)
        {
            /* no antialiasing, glyph should only contain 0 or 16. */
            if (glyph_ptr[x] >= 16)
            {
                if (x & 1)
                    dst_ptr[x / 2] = text_pixel | (dst_ptr[x / 2] & 0xf0);
                else
                    dst_ptr[x / 2] = (text_pixel << 4) | (dst_ptr[x / 2] & 0x0f);
            }
        }
        dst_ptr += dib->stride;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_1( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                          const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    BYTE *dst_ptr = get_pixel_ptr_1( dib, 0, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x - rect->left, origin->y );
    int x, y;
    BYTE text = (text_pixel & 1) ? 0xff : 0;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = rect->left; x < rect->right; x++)
        {
            /* no antialiasing, glyph should only contain 0 or 16. */
            if (glyph_ptr[x] >= 16)
                dst_ptr[x / 8] = (dst_ptr[x / 8] & ~pixel_masks_1[x % 8]) | (text & pixel_masks_1[x % 8]);
        }
        dst_ptr += dib->stride;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_null( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                             const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    return;
}

static BOOL create_rop_masks_32(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *hatch_start = get_pixel_ptr_1(hatch, 0, 0), *hatch_ptr;
    DWORD mask_start = 0, mask_offset;
    DWORD *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    for(y = 0; y < hatch->height; y++)
    {
        hatch_ptr = hatch_start;
        mask_offset = mask_start;
        for(x = 0; x < hatch->width; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x % 8])
            {
                and_bits[mask_offset] = fg->and;
                xor_bits[mask_offset] = fg->xor;
            }
            else
            {
                and_bits[mask_offset] = bg->and;
                xor_bits[mask_offset] = bg->xor;
            }
            if(x % 8 == 7) hatch_ptr++;
            mask_offset++;
        }
        hatch_start += hatch->stride;
        mask_start += dib->stride / 4;
    }

    return TRUE;
}

static BOOL create_rop_masks_24(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *hatch_start = get_pixel_ptr_1(hatch, 0, 0), *hatch_ptr;
    DWORD mask_start = 0, mask_offset;
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    for(y = 0; y < hatch->height; y++)
    {
        hatch_ptr = hatch_start;
        mask_offset = mask_start;
        for(x = 0; x < hatch->width; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x % 8])
            {
                and_bits[mask_offset]   =  fg->and        & 0xff;
                xor_bits[mask_offset++] =  fg->xor        & 0xff;
                and_bits[mask_offset]   = (fg->and >>  8) & 0xff;
                xor_bits[mask_offset++] = (fg->xor >>  8) & 0xff;
                and_bits[mask_offset]   = (fg->and >> 16) & 0xff;
                xor_bits[mask_offset++] = (fg->xor >> 16) & 0xff;
            }
            else
            {
                and_bits[mask_offset]   =  bg->and        & 0xff;
                xor_bits[mask_offset++] =  bg->xor        & 0xff;
                and_bits[mask_offset]   = (bg->and >>  8) & 0xff;
                xor_bits[mask_offset++] = (bg->xor >>  8) & 0xff;
                and_bits[mask_offset]   = (bg->and >> 16) & 0xff;
                xor_bits[mask_offset++] = (bg->xor >> 16) & 0xff;
            }
            if(x % 8 == 7) hatch_ptr++;
        }
        hatch_start += hatch->stride;
        mask_start += dib->stride;
    }

    return TRUE;
}

static BOOL create_rop_masks_16(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *hatch_start = get_pixel_ptr_1(hatch, 0, 0), *hatch_ptr;
    DWORD mask_start = 0, mask_offset;
    WORD *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    for(y = 0; y < hatch->height; y++)
    {
        hatch_ptr = hatch_start;
        mask_offset = mask_start;
        for(x = 0; x < hatch->width; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x % 8])
            {
                and_bits[mask_offset] = fg->and;
                xor_bits[mask_offset] = fg->xor;
            }
            else
            {
                and_bits[mask_offset] = bg->and;
                xor_bits[mask_offset] = bg->xor;
            }
            if(x % 8 == 7) hatch_ptr++;
            mask_offset++;
        }
        hatch_start += hatch->stride;
        mask_start += dib->stride / 2;
    }

    return TRUE;
}

static BOOL create_rop_masks_8(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *hatch_start = get_pixel_ptr_1(hatch, 0, 0), *hatch_ptr;
    DWORD mask_start = 0, mask_offset;
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    for(y = 0; y < hatch->height; y++)
    {
        hatch_ptr = hatch_start;
        mask_offset = mask_start;
        for(x = 0; x < hatch->width; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x % 8])
            {
                and_bits[mask_offset] = fg->and;
                xor_bits[mask_offset] = fg->xor;
            }
            else
            {
                and_bits[mask_offset] = bg->and;
                xor_bits[mask_offset] = bg->xor;
            }
            if(x % 8 == 7) hatch_ptr++;
            mask_offset++;
        }
        hatch_start += hatch->stride;
        mask_start += dib->stride;
    }

    return TRUE;
}

static BOOL create_rop_masks_4(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *hatch_start = get_pixel_ptr_1(hatch, 0, 0), *hatch_ptr;
    DWORD mask_start = 0, mask_offset;
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    const rop_mask *rop_mask;
    int x, y;

    for(y = 0; y < hatch->height; y++)
    {
        hatch_ptr = hatch_start;
        mask_offset = mask_start;
        for(x = 0; x < hatch->width; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x % 8])
                rop_mask = fg;
            else
                rop_mask = bg;

            if(x & 1)
            {
                and_bits[mask_offset] = (rop_mask->and & 0x0f) | (and_bits[mask_offset] & 0xf0);
                xor_bits[mask_offset] = (rop_mask->xor & 0x0f) | (xor_bits[mask_offset] & 0xf0);
                mask_offset++;
            }
            else
            {
                and_bits[mask_offset] = (rop_mask->and << 4) & 0xf0;
                xor_bits[mask_offset] = (rop_mask->xor << 4) & 0xf0;
            }

            if(x % 8 == 7) hatch_ptr++;
        }
        hatch_start += hatch->stride;
        mask_start += dib->stride;
    }

    return TRUE;
}

static BOOL create_rop_masks_1(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *hatch_start = get_pixel_ptr_1(hatch, 0, 0), *hatch_ptr;
    DWORD mask_start = 0, mask_offset;
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    rop_mask rop_mask;
    int x, y, bit_pos;

    for(y = 0; y < hatch->height; y++)
    {
        hatch_ptr = hatch_start;
        mask_offset = mask_start;
        for(x = 0, bit_pos = 0; x < hatch->width; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x % 8])
            {
                rop_mask.and = (fg->and & 1) ? 0xff : 0;
                rop_mask.xor = (fg->xor & 1) ? 0xff : 0;
            }
            else
            {
                rop_mask.and = (bg->and & 1) ? 0xff : 0;
                rop_mask.xor = (bg->xor & 1) ? 0xff : 0;
            }

            if(bit_pos == 0) and_bits[mask_offset] = xor_bits[mask_offset] = 0;

            and_bits[mask_offset] = (rop_mask.and & pixel_masks_1[bit_pos]) | (and_bits[mask_offset] & ~pixel_masks_1[bit_pos]);
            xor_bits[mask_offset] = (rop_mask.xor & pixel_masks_1[bit_pos]) | (xor_bits[mask_offset] & ~pixel_masks_1[bit_pos]);

            if(++bit_pos == 8)
            {
                mask_offset++;
                hatch_ptr++;
                bit_pos = 0;
            }
        }
        hatch_start += hatch->stride;
        mask_start += dib->stride;
    }

    return TRUE;
}

static BOOL create_rop_masks_null(const dib_info *dib, const dib_info *hatch, const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    return FALSE;
}

static inline void rop_codes_from_stretch_mode( int mode, struct rop_codes *codes )
{
    switch (mode)
    {
    default:
    case STRETCH_DELETESCANS:
        get_rop_codes( R2_COPYPEN, codes );
        break;
    case STRETCH_ORSCANS:
        get_rop_codes( R2_MERGEPEN, codes );
        break;
    case STRETCH_ANDSCANS:
        get_rop_codes( R2_MASKPEN, codes );
        break;
    }
    return;
}

static void stretch_row_32(const dib_info *dst_dib, const POINT *dst_start,
                           const dib_info *src_dib, const POINT *src_start,
                           const struct stretch_params *params, int mode,
                           BOOL keep_dst)
{
    DWORD *dst_ptr = get_pixel_ptr_32( dst_dib, dst_start->x, dst_start->y );
    DWORD *src_ptr = get_pixel_ptr_32( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        do_rop_codes_32( dst_ptr, *src_ptr, &codes );
        dst_ptr += params->dst_inc;
        if (err > 0)
        {
            src_ptr += params->src_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void stretch_row_24(const dib_info *dst_dib, const POINT *dst_start,
                           const dib_info *src_dib, const POINT *src_start,
                           const struct stretch_params *params, int mode,
                           BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_24( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_24( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        do_rop_codes_8( dst_ptr,     *src_ptr,       &codes );
        do_rop_codes_8( dst_ptr + 1, *(src_ptr + 1), &codes );
        do_rop_codes_8( dst_ptr + 2, *(src_ptr + 2), &codes );
        dst_ptr += 3 * params->dst_inc;
        if (err > 0)
        {
            src_ptr += 3 * params->src_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void stretch_row_16(const dib_info *dst_dib, const POINT *dst_start,
                           const dib_info *src_dib, const POINT *src_start,
                           const struct stretch_params *params, int mode,
                           BOOL keep_dst)
{
    WORD *dst_ptr = get_pixel_ptr_16( dst_dib, dst_start->x, dst_start->y );
    WORD *src_ptr = get_pixel_ptr_16( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        do_rop_codes_16( dst_ptr, *src_ptr, &codes );
        dst_ptr += params->dst_inc;
        if (err > 0)
        {
            src_ptr += params->src_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void stretch_row_8(const dib_info *dst_dib, const POINT *dst_start,
                          const dib_info *src_dib, const POINT *src_start,
                          const struct stretch_params *params, int mode,
                          BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_8( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_8( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        do_rop_codes_8( dst_ptr, *src_ptr, &codes );
        dst_ptr += params->dst_inc;
        if (err > 0)
        {
            src_ptr += params->src_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void stretch_row_4(const dib_info *dst_dib, const POINT *dst_start,
                          const dib_info *src_dib, const POINT *src_start,
                          const struct stretch_params *params, int mode,
                          BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_4( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_4( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width, dst_x = dst_start->x, src_x = src_start->x;
    struct rop_codes codes;
    BYTE src_val;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (src_x & 1) src_val = (*src_ptr & 0x0f) | (*src_ptr << 4);
        else src_val = (*src_ptr & 0xf0) | (*src_ptr >> 4);

        do_rop_codes_mask_8( dst_ptr, src_val, &codes, (dst_x & 1) ? 0x0f : 0xf0 );

        if ((dst_x & ~1) != ((dst_x + params->dst_inc) & ~1))
            dst_ptr += params->dst_inc;
        dst_x += params->dst_inc;

        if (err > 0)
        {
            if ((src_x & ~1) != ((src_x + params->src_inc) & ~1))
                src_ptr += params->src_inc;
            src_x += params->src_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void stretch_row_1(const dib_info *dst_dib, const POINT *dst_start,
                          const dib_info *src_dib, const POINT *src_start,
                          const struct stretch_params *params, int mode,
                          BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_1( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_1( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width, dst_x = dst_start->x, src_x = src_start->x;
    struct rop_codes codes;
    BYTE src_val;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        src_val = *src_ptr & pixel_masks_1[src_x % 8] ? 0xff : 0;
        do_rop_codes_mask_8( dst_ptr, src_val, &codes, pixel_masks_1[dst_x % 8] );

        if ((dst_x & ~7) != ((dst_x + params->dst_inc) & ~7))
            dst_ptr += params->dst_inc;
        dst_x += params->dst_inc;

        if (err > 0)
        {
            if ((src_x & ~7) != ((src_x + params->src_inc) & ~7))
                src_ptr += params->src_inc;
            src_x += params->src_inc;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void stretch_row_null(const dib_info *dst_dib, const POINT *dst_start,
                             const dib_info *src_dib, const POINT *src_start,
                             const struct stretch_params *params, int mode,
                             BOOL keep_dst)
{
    FIXME("bit count %d\n", dst_dib->bit_count);
    return;
}

static void shrink_row_32(const dib_info *dst_dib, const POINT *dst_start,
                          const dib_info *src_dib, const POINT *src_start,
                          const struct stretch_params *params, int mode,
                          BOOL keep_dst)
{
    DWORD *dst_ptr = get_pixel_ptr_32( dst_dib, dst_start->x, dst_start->y );
    DWORD *src_ptr = get_pixel_ptr_32( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;
    DWORD init_val = (mode == STRETCH_ANDSCANS) ? ~0u : 0u;
    BOOL new_pix = TRUE;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (new_pix && !keep_dst) *dst_ptr = init_val;
        do_rop_codes_32( dst_ptr, *src_ptr, &codes );
        new_pix = FALSE;
        src_ptr += params->src_inc;
        if (err > 0)
        {
            dst_ptr += params->dst_inc;
            new_pix = TRUE;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void shrink_row_24(const dib_info *dst_dib, const POINT *dst_start,
                          const dib_info *src_dib, const POINT *src_start,
                          const struct stretch_params *params, int mode,
                          BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_24( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_24( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;
    BYTE init_val = (mode == STRETCH_ANDSCANS) ? ~0u : 0u;
    BOOL new_pix = TRUE;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (new_pix && !keep_dst) memset( dst_ptr, init_val, 3 );
        do_rop_codes_8( dst_ptr,      *src_ptr,      &codes );
        do_rop_codes_8( dst_ptr + 1, *(src_ptr + 1), &codes );
        do_rop_codes_8( dst_ptr + 2, *(src_ptr + 2), &codes );
        new_pix = FALSE;
        src_ptr += 3 * params->src_inc;
        if (err > 0)
        {
            dst_ptr += 3 * params->dst_inc;
            new_pix = TRUE;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void shrink_row_16(const dib_info *dst_dib, const POINT *dst_start,
                          const dib_info *src_dib, const POINT *src_start,
                          const struct stretch_params *params, int mode,
                          BOOL keep_dst)
{
    WORD *dst_ptr = get_pixel_ptr_16( dst_dib, dst_start->x, dst_start->y );
    WORD *src_ptr = get_pixel_ptr_16( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;
    WORD init_val = (mode == STRETCH_ANDSCANS) ? ~0u : 0u;
    BOOL new_pix = TRUE;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (new_pix && !keep_dst) *dst_ptr = init_val;
        do_rop_codes_16( dst_ptr, *src_ptr, &codes );
        new_pix = FALSE;
        src_ptr += params->src_inc;
        if (err > 0)
        {
            dst_ptr += params->dst_inc;
            new_pix = TRUE;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void shrink_row_8(const dib_info *dst_dib, const POINT *dst_start,
                         const dib_info *src_dib, const POINT *src_start,
                         const struct stretch_params *params, int mode,
                         BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_8( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_8( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width;
    struct rop_codes codes;
    BYTE init_val = (mode == STRETCH_ANDSCANS) ? ~0u : 0u;
    BOOL new_pix = TRUE;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (new_pix && !keep_dst) *dst_ptr = init_val;
        do_rop_codes_8( dst_ptr, *src_ptr, &codes );
        new_pix = FALSE;
        src_ptr += params->src_inc;
        if (err > 0)
        {
            dst_ptr += params->dst_inc;
            new_pix = TRUE;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void shrink_row_4(const dib_info *dst_dib, const POINT *dst_start,
                         const dib_info *src_dib, const POINT *src_start,
                         const struct stretch_params *params, int mode,
                         BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_8( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_8( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width, dst_x = dst_start->x, src_x = src_start->x;
    struct rop_codes codes;
    BYTE src_val, init_val = (mode == STRETCH_ANDSCANS) ? ~0u : 0u;
    BOOL new_pix = TRUE;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (new_pix && !keep_dst) do_rop_mask_8( dst_ptr, 0, init_val, (dst_x & 1) ? 0xf0 : 0x0f );

        if (src_x & 1) src_val = (*src_ptr & 0x0f) | (*src_ptr << 4);
        else src_val = (*src_ptr & 0xf0) | (*src_ptr >> 4);

        do_rop_codes_mask_8( dst_ptr, src_val, &codes, (dst_x & 1) ? 0xf0 : 0x0f );
        new_pix = FALSE;

        if ((src_x & ~1) != ((src_x + params->src_inc) & ~1))
            src_ptr += params->src_inc;
        src_x += params->src_inc;

        if (err > 0)
        {
            if ((dst_x & ~1) != ((dst_x + params->dst_inc) & ~1))
                dst_ptr += params->dst_inc;
            dst_x += params->dst_inc;
            new_pix = TRUE;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void shrink_row_1(const dib_info *dst_dib, const POINT *dst_start,
                         const dib_info *src_dib, const POINT *src_start,
                         const struct stretch_params *params, int mode,
                         BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_1( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_1( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width, dst_x = dst_start->x, src_x = src_start->x;
    struct rop_codes codes;
    BYTE src_val, init_val = (mode == STRETCH_ANDSCANS) ? ~0u : 0u;
    BOOL new_pix = TRUE;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (new_pix && !keep_dst) do_rop_mask_8( dst_ptr, 0, init_val, pixel_masks_1[dst_x % 8] );
        src_val = *src_ptr & pixel_masks_1[src_x % 8] ? 0xff : 0;
        do_rop_codes_mask_8( dst_ptr, src_val, &codes, pixel_masks_1[dst_x % 8] );
        new_pix = FALSE;

        if ((src_x & ~7) != ((src_x + params->src_inc) & ~7))
            src_ptr += params->src_inc;
        src_x += params->src_inc;

        if (err > 0)
        {
            if ((dst_x & ~7) != ((dst_x + params->dst_inc) & ~7))
                dst_ptr += params->dst_inc;
            dst_x += params->dst_inc;
            new_pix = TRUE;
            err += params->err_add_1;
        }
        else err += params->err_add_2;
    }
}

static void shrink_row_null(const dib_info *dst_dib, const POINT *dst_start,
                            const dib_info *src_dib, const POINT *src_start,
                            const struct stretch_params *params, int mode,
                            BOOL keep_dst)
{
    FIXME("bit count %d\n", dst_dib->bit_count);
    return;
}

const primitive_funcs funcs_8888 =
{
    solid_rects_32,
    solid_line_32,
    pattern_rects_32,
    copy_rect_32,
    blend_rect_8888,
    gradient_rect_8888,
    draw_glyph_8888,
    get_pixel_32,
    colorref_to_pixel_888,
    pixel_to_colorref_888,
    convert_to_8888,
    create_rop_masks_32,
    stretch_row_32,
    shrink_row_32
};

const primitive_funcs funcs_32 =
{
    solid_rects_32,
    solid_line_32,
    pattern_rects_32,
    copy_rect_32,
    blend_rect_32,
    gradient_rect_32,
    draw_glyph_32,
    get_pixel_32,
    colorref_to_pixel_masks,
    pixel_to_colorref_masks,
    convert_to_32,
    create_rop_masks_32,
    stretch_row_32,
    shrink_row_32
};

const primitive_funcs funcs_24 =
{
    solid_rects_24,
    solid_line_24,
    pattern_rects_24,
    copy_rect_24,
    blend_rect_24,
    gradient_rect_24,
    draw_glyph_24,
    get_pixel_24,
    colorref_to_pixel_888,
    pixel_to_colorref_888,
    convert_to_24,
    create_rop_masks_24,
    stretch_row_24,
    shrink_row_24
};

const primitive_funcs funcs_555 =
{
    solid_rects_16,
    solid_line_16,
    pattern_rects_16,
    copy_rect_16,
    blend_rect_555,
    gradient_rect_555,
    draw_glyph_555,
    get_pixel_16,
    colorref_to_pixel_555,
    pixel_to_colorref_555,
    convert_to_555,
    create_rop_masks_16,
    stretch_row_16,
    shrink_row_16
};

const primitive_funcs funcs_16 =
{
    solid_rects_16,
    solid_line_16,
    pattern_rects_16,
    copy_rect_16,
    blend_rect_16,
    gradient_rect_16,
    draw_glyph_16,
    get_pixel_16,
    colorref_to_pixel_masks,
    pixel_to_colorref_masks,
    convert_to_16,
    create_rop_masks_16,
    stretch_row_16,
    shrink_row_16
};

const primitive_funcs funcs_8 =
{
    solid_rects_8,
    solid_line_8,
    pattern_rects_8,
    copy_rect_8,
    blend_rect_8,
    gradient_rect_8,
    draw_glyph_8,
    get_pixel_8,
    colorref_to_pixel_colortable,
    pixel_to_colorref_colortable,
    convert_to_8,
    create_rop_masks_8,
    stretch_row_8,
    shrink_row_8
};

const primitive_funcs funcs_4 =
{
    solid_rects_4,
    solid_line_4,
    pattern_rects_4,
    copy_rect_4,
    blend_rect_4,
    gradient_rect_4,
    draw_glyph_4,
    get_pixel_4,
    colorref_to_pixel_colortable,
    pixel_to_colorref_colortable,
    convert_to_4,
    create_rop_masks_4,
    stretch_row_4,
    shrink_row_4
};

const primitive_funcs funcs_1 =
{
    solid_rects_1,
    solid_line_1,
    pattern_rects_1,
    copy_rect_1,
    blend_rect_1,
    gradient_rect_1,
    draw_glyph_1,
    get_pixel_1,
    colorref_to_pixel_colortable,
    pixel_to_colorref_colortable,
    convert_to_1,
    create_rop_masks_1,
    stretch_row_1,
    shrink_row_1
};

const primitive_funcs funcs_null =
{
    solid_rects_null,
    solid_line_null,
    pattern_rects_null,
    copy_rect_null,
    blend_rect_null,
    gradient_rect_null,
    draw_glyph_null,
    get_pixel_null,
    colorref_to_pixel_null,
    pixel_to_colorref_null,
    convert_to_null,
    create_rop_masks_null,
    stretch_row_null,
    shrink_row_null
};
