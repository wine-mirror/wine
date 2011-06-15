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
    return (DWORD *)((BYTE*)dib->bits + y * dib->stride + x * 4);
}

static inline DWORD *get_pixel_ptr_24_dword(const dib_info *dib, int x, int y)
{
    return (DWORD *)((BYTE*)dib->bits + y * dib->stride) + x * 3 / 4;
}

static inline BYTE *get_pixel_ptr_24(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits + y * dib->stride + x * 3;
}

static inline WORD *get_pixel_ptr_16(const dib_info *dib, int x, int y)
{
    return (WORD *)((BYTE*)dib->bits + y * dib->stride + x * 2);
}

static inline BYTE *get_pixel_ptr_8(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits + y * dib->stride + x;
}

static inline BYTE *get_pixel_ptr_4(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits + y * dib->stride + x / 2;
}

static inline BYTE *get_pixel_ptr_1(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits + y * dib->stride + x / 8;
}

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

static DWORD colorref_to_pixel_888(const dib_info *dib, COLORREF color)
{
    return ( ((color >> 16) & 0xff) | (color & 0xff00) | ((color << 16) & 0xff0000) );
}

static inline DWORD put_field(DWORD field, int shift, int len)
{
    shift = shift - (8 - len);
    if (len <= 8)
        field &= (((1 << len) - 1) << (8 - len));
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

static DWORD colorref_to_pixel_colortable(const dib_info *dib, COLORREF color)
{
    int i, best_index = 0;
    RGBQUAD rgb;
    DWORD diff, best_diff = 0xffffffff;

    rgb.rgbRed = GetRValue(color);
    rgb.rgbGreen = GetGValue(color);
    rgb.rgbBlue = GetBValue(color);

    for(i = 0; i < dib->color_table_size; i++)
    {
        RGBQUAD *cur = dib->color_table + i;
        diff = (rgb.rgbRed - cur->rgbRed) * (rgb.rgbRed - cur->rgbRed)
            +  (rgb.rgbGreen - cur->rgbGreen) * (rgb.rgbGreen - cur->rgbGreen)
            +  (rgb.rgbBlue - cur->rgbBlue) * (rgb.rgbBlue - cur->rgbBlue);

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

static DWORD colorref_to_pixel_null(const dib_info *dib, COLORREF color)
{
    return 0;
}

static BOOL convert_to_8888(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    DWORD *dst_start = dst->bits, *dst_pixel, src_val;
    int x, y;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top);
        if(src->funcs == &funcs_8888)
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst->bits, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 4);
                    dst_start += dst->stride / 4;
                    src_start += src->stride / 4;
                }
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 8888\n");
            return FALSE;
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
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 8888\n");
            return FALSE;
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
                RGBQUAD rgb;
                src_val = *src_pixel++;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = rgb.rgbRed << 16 | rgb.rgbGreen << 8 | rgb.rgbBlue;
            }
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
                    src_val = *src_pixel++ & 0xf;
                else
                    src_val = (*src_pixel >> 4) & 0xf;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = rgb.rgbRed << 16 | rgb.rgbGreen << 8 | rgb.rgbBlue;
            }
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
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 8888\n", src->bit_count);
        return FALSE;
    }

    return TRUE;
}

static BOOL convert_to_32(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    DWORD *dst_start = dst->bits, *dst_pixel, src_val;
    int x, y;

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
                    *dst_pixel++ = put_field((src_val >> 16) & 0xff, dst->red_shift,   dst->red_len)   |
                                   put_field((src_val >>  8) & 0xff, dst->green_shift, dst->green_len) |
                                   put_field( src_val        & 0xff, dst->blue_shift,  dst->blue_len);
                }
                dst_start += dst->stride / 4;
                src_start += src->stride / 4;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 32\n");
            return FALSE;
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
                dst_start += dst->stride / 4;
                src_start += src->stride / 2;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 8888\n");
            return FALSE;
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
                RGBQUAD rgb;
                src_val = *src_pixel++;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
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
                    src_val = *src_pixel++ & 0xf;
                else
                    src_val = (*src_pixel >> 4) & 0xf;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
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
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 32\n", src->bit_count);
        return FALSE;
    }

    return TRUE;
}

static BOOL convert_to_24(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = dst->bits, *dst_pixel;
    DWORD src_val;
    int x, y;

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
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 24\n");
            return FALSE;
        }
        break;
    }

    case 24:
    {
        BYTE *src_start = get_pixel_ptr_24(src, src_rect->left, src_rect->top);

        if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
            memcpy(dst->bits, src_start, (src_rect->bottom - src_rect->top) * src->stride);
        else
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 3);
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
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 24\n");
            return FALSE;
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
                RGBQUAD rgb;
                src_val = *src_pixel++;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = rgb.rgbBlue;
                *dst_pixel++ = rgb.rgbGreen;
                *dst_pixel++ = rgb.rgbRed;
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
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                if(x & 1)
                    src_val = *src_pixel++ & 0xf;
                else
                    src_val = (*src_pixel >> 4) & 0xf;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = rgb.rgbBlue;
                *dst_pixel++ = rgb.rgbGreen;
                *dst_pixel++ = rgb.rgbRed;
            }
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
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 24\n", src->bit_count);
        return FALSE;
    }

    return TRUE;
}

static BOOL convert_to_555(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    WORD *dst_start = dst->bits, *dst_pixel;
    INT x, y;
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
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 555\n");
            return FALSE;
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
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 16:
    {
        WORD *src_start = get_pixel_ptr_16(src, src_rect->left, src_rect->top);
        if(src->funcs == &funcs_555)
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst->bits, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left) * 2);
                    dst_start += dst->stride / 2;
                    src_start += src->stride / 2;
                }
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 555\n");
            return FALSE;
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
                RGBQUAD rgb;
                src_val = *src_pixel++;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = ((rgb.rgbRed   << 7) & 0x7c00) |
                               ((rgb.rgbGreen << 2) & 0x03e0) |
                               ((rgb.rgbBlue  >> 3) & 0x001f);
            }
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
                    src_val = *src_pixel++ & 0xf;
                else
                    src_val = (*src_pixel >> 4) & 0xf;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = ((rgb.rgbRed   << 7) & 0x7c00) |
                               ((rgb.rgbGreen << 2) & 0x03e0) |
                               ((rgb.rgbBlue  >> 3) & 0x001f);
            }
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
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 555\n", src->bit_count);
        return FALSE;

    }
    return TRUE;
}

static BOOL convert_to_16(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    WORD *dst_start = dst->bits, *dst_pixel;
    INT x, y;
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
                    *dst_pixel++ = put_field((src_val >> 16) & 0xff, dst->red_shift,   dst->red_len)   |
                                   put_field((src_val >>  8) & 0xff, dst->green_shift, dst->green_len) |
                                   put_field( src_val        & 0xff, dst->blue_shift,  dst->blue_len);
                }
                dst_start += dst->stride / 2;
                src_start += src->stride / 4;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 16\n");
            return FALSE;
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
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 16\n");
            return FALSE;
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
                RGBQUAD rgb;
                src_val = *src_pixel++;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
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
                    src_val = *src_pixel++ & 0xf;
                else
                    src_val = (*src_pixel >> 4) & 0xf;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = put_field(rgb.rgbRed,   dst->red_shift,   dst->red_len) |
                               put_field(rgb.rgbGreen, dst->green_shift, dst->green_len) |
                               put_field(rgb.rgbBlue,  dst->blue_shift,  dst->blue_len);
            }
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
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 16\n", src->bit_count);
        return FALSE;

    }
    return TRUE;
}

static inline BOOL color_tables_match(const dib_info *d1, const dib_info *d2)
{
    assert(d1->color_table_size && d2->color_table_size);

    if(d1->color_table_size != d2->color_table_size) return FALSE;
    return !memcmp(d1->color_table, d2->color_table, d1->color_table_size * sizeof(d1->color_table[0]));
}

static BOOL convert_to_8(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = dst->bits, *dst_pixel;
    INT x, y;
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
                    *dst_pixel++ = colorref_to_pixel_colortable(dst, ((src_val >> 16) & 0x0000ff) |
                                                                     ( src_val        & 0x00ff00) |
                                                                     ((src_val << 16) & 0xff0000) );
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 8\n");
            return FALSE;
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

                *dst_pixel++ = colorref_to_pixel_colortable(dst, ( rgb.rgbRed         & 0x0000ff) |
                                                                 ((rgb.rgbGreen << 8) & 0x00ff00) |
                                                                 ((rgb.rgbBlue << 16) & 0xff0000));
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
                    *dst_pixel++ = colorref_to_pixel_colortable(dst, ((src_val >>  7) & 0x0000f8) | ((src_val >> 12) & 0x000007) |
                                                                     ((src_val <<  6) & 0x00f800) | ((src_val <<  1) & 0x000700) |
                                                                     ((src_val << 19) & 0xf80000) | ((src_val << 14) & 0x070000) );
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 8\n");
            return FALSE;
        }
        break;
    }

    case 8:
    {
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;

        if(color_tables_match(dst, src))
        {
            if(src->stride > 0 && dst->stride > 0 && src_rect->left == 0 && src_rect->right == src->width)
                memcpy(dst->bits, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, src_rect->right - src_rect->left);
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
                    src_val = *src_pixel++;
                    if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                    rgb = src->color_table[src_val];
                    *dst_pixel++ = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
                }
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
                    src_val = *src_pixel++ & 0xf;
                else
                    src_val = (*src_pixel >> 4) & 0xf;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                *dst_pixel++ = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
            }
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
                *dst_pixel++ = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 8\n", src->bit_count);
        return FALSE;

    }
    return TRUE;
}

static BOOL convert_to_4(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = dst->bits, *dst_pixel, dst_val;
    INT x, y;
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
                    dst_val = colorref_to_pixel_colortable(dst, ((src_val >> 16) & 0x0000ff) |
                                                                ( src_val        & 0x00ff00) |
                                                                ((src_val << 16) & 0xff0000) );
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 4\n");
            return FALSE;
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

                dst_val = colorref_to_pixel_colortable(dst, ( rgb.rgbRed         & 0x0000ff) |
                                                            ((rgb.rgbGreen << 8) & 0x00ff00) |
                                                            ((rgb.rgbBlue << 16) & 0xff0000));

                if((x - src_rect->left) & 1)
                {
                    *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                    dst_pixel++;
                }
                else
                    *dst_pixel = (dst_val << 4) & 0xf0;
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
                    dst_val = colorref_to_pixel_colortable(dst, ((src_val >>  7) & 0x0000f8) | ((src_val >> 12) & 0x000007) |
                                                                ((src_val <<  6) & 0x00f800) | ((src_val <<  1) & 0x000700) |
                                                                ((src_val << 19) & 0xf80000) | ((src_val << 14) & 0x070000) );
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 4\n");
            return FALSE;
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
                RGBQUAD rgb;
                src_val = *src_pixel++;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                dst_val = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
                if((x - src_rect->left) & 1)
                {
                    *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                    dst_pixel++;
                }
                else
                    *dst_pixel = (dst_val << 4) & 0xf0;
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
                memcpy(dst->bits, src_start, (src_rect->bottom - src_rect->top) * src->stride);
            else
            {
                for(y = src_rect->top; y < src_rect->bottom; y++)
                {
                    memcpy(dst_start, src_start, (src_rect->right - src_rect->left + 1) / 2);
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
                        src_val = *src_pixel++ & 0xf;
                    else
                        src_val = (*src_pixel >> 4) & 0xf;
                    if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                    rgb = src->color_table[src_val];
                    dst_val = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
                    if((x - src_rect->left) & 1)
                    {
                        *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                        dst_pixel++;
                    }
                    else
                        *dst_pixel = (dst_val << 4) & 0xf0;
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
                dst_val = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
                if((x - src_rect->left) & 1)
                {
                    *dst_pixel = (dst_val & 0x0f) | (*dst_pixel & 0xf0);
                    dst_pixel++;
                }
                else
                    *dst_pixel = (dst_val << 4) & 0xf0;
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 4\n", src->bit_count);
        return FALSE;

    }

    return TRUE;
}

static BOOL convert_to_1(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    BYTE *dst_start = dst->bits, *dst_pixel, dst_val;
    INT x, y;
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
                    dst_val = colorref_to_pixel_colortable(dst, ((src_val >> 16) & 0x0000ff) |
                                                                ( src_val        & 0x00ff00) |
                                                                ((src_val << 16) & 0xff0000) ) ? 0xff : 0;

                    if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                dst_start += dst->stride;
                src_start += src->stride / 4;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 32 -> 1\n");
            return FALSE;
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
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
            {
                RGBQUAD rgb;
                rgb.rgbBlue  = *src_pixel++;
                rgb.rgbGreen = *src_pixel++;
                rgb.rgbRed   = *src_pixel++;

                dst_val = colorref_to_pixel_colortable(dst, ( rgb.rgbRed         & 0x0000ff) |
                                                            ((rgb.rgbGreen << 8) & 0x00ff00) |
                                                            ((rgb.rgbBlue << 16) & 0xff0000)) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
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
                    dst_val = colorref_to_pixel_colortable(dst, ((src_val >>  7) & 0x0000f8) | ((src_val >> 12) & 0x000007) |
                                                                ((src_val <<  6) & 0x00f800) | ((src_val <<  1) & 0x000700) |
                                                                ((src_val << 19) & 0xf80000) | ((src_val << 14) & 0x070000) ) ? 0xff : 0;

                    if(bit_pos == 0) *dst_pixel = 0;
                    *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                    if(++bit_pos == 8)
                    {
                        dst_pixel++;
                        bit_pos = 0;
                    }
                }
                dst_start += dst->stride;
                src_start += src->stride / 2;
            }
        }
        else
        {
            FIXME("Unsupported conversion: 16 -> 1\n");
            return FALSE;
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
                RGBQUAD rgb;
                src_val = *src_pixel++;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                dst_val = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue)) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
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
                    src_val = *src_pixel++ & 0xf;
                else
                    src_val = (*src_pixel >> 4) & 0xf;
                if(src_val >= src->color_table_size) src_val = src->color_table_size - 1;
                rgb = src->color_table[src_val];
                dst_val = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue)) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
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
                dst_val = colorref_to_pixel_colortable(dst, RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue)) ? 0xff : 0;

                if(bit_pos == 0) *dst_pixel = 0;
                *dst_pixel = (*dst_pixel & ~pixel_masks_1[bit_pos]) | (dst_val & pixel_masks_1[bit_pos]);

                if(++bit_pos == 8)
                {
                    dst_pixel++;
                    bit_pos = 0;
                }
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    default:
        FIXME("Unsupported conversion: %d -> 1\n", src->bit_count);
        return FALSE;

    }
    return TRUE;
}

static BOOL convert_to_null(dib_info *dst, const dib_info *src, const RECT *src_rect)
{
    return FALSE;
}

const primitive_funcs funcs_8888 =
{
    solid_rects_32,
    pattern_rects_32,
    colorref_to_pixel_888,
    convert_to_8888
};

const primitive_funcs funcs_32 =
{
    solid_rects_32,
    pattern_rects_32,
    colorref_to_pixel_masks,
    convert_to_32
};

const primitive_funcs funcs_24 =
{
    solid_rects_24,
    pattern_rects_24,
    colorref_to_pixel_888,
    convert_to_24
};

const primitive_funcs funcs_555 =
{
    solid_rects_16,
    pattern_rects_16,
    colorref_to_pixel_555,
    convert_to_555
};

const primitive_funcs funcs_16 =
{
    solid_rects_16,
    pattern_rects_16,
    colorref_to_pixel_masks,
    convert_to_16
};

const primitive_funcs funcs_8 =
{
    solid_rects_8,
    pattern_rects_8,
    colorref_to_pixel_colortable,
    convert_to_8
};

const primitive_funcs funcs_4 =
{
    solid_rects_4,
    pattern_rects_4,
    colorref_to_pixel_colortable,
    convert_to_4
};

const primitive_funcs funcs_1 =
{
    solid_rects_1,
    pattern_rects_1,
    colorref_to_pixel_colortable,
    convert_to_1
};

const primitive_funcs funcs_null =
{
    solid_rects_null,
    pattern_rects_null,
    colorref_to_pixel_null,
    convert_to_null
};
