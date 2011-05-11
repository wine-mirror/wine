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

#include "gdi_private.h"
#include "dibdrv.h"

static inline DWORD *get_pixel_ptr_32(const dib_info *dib, int x, int y)
{
    return (DWORD *)((BYTE*)dib->bits + y * dib->stride + x * 4);
}

static inline void do_rop_32(DWORD *ptr, DWORD and, DWORD xor)
{
    *ptr = (*ptr & and) ^ xor;
}

static void solid_rects_32(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    DWORD *ptr, *start;
    int x, y, i;

    for(i = 0; i < num; i++, rc++)
    {
        start = ptr = get_pixel_ptr_32(dib, rc->left, rc->top);
        for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
            for(x = rc->left, ptr = start; x < rc->right; x++)
                do_rop_32(ptr++, and, xor);
    }
}

static void solid_rects_null(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
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

static DWORD colorref_to_pixel_null(const dib_info *dib, COLORREF color)
{
    return 0;
}

const primitive_funcs funcs_8888 =
{
    solid_rects_32,
    colorref_to_pixel_888
};

const primitive_funcs funcs_32 =
{
    solid_rects_32,
    colorref_to_pixel_masks
};

const primitive_funcs funcs_null =
{
    solid_rects_null,
    colorref_to_pixel_null
};
