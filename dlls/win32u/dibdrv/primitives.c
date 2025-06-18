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

#if 0
#pragma makedep unix
#endif

#include <assert.h>

#include "ntgdi_private.h"
#include "dibdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

/* Bayer matrices for dithering */

static const BYTE bayer_4x4[4][4] =
{
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};

static const BYTE bayer_8x8[8][8] =
{
    {   0,  32,   8,  40,   2,  34,  10,  42 },
    {  48,  16,  56,  24,  50,  18,  58,  26 },
    {  12,  44,   4,  36,  14,  46,   6,  38 },
    {  60,  28,  52,  20,  62,  30,  54,  22 },
    {   3,  35,  11,  43,   1,  33,   9,  41 },
    {  51,  19,  59,  27,  49,  17,  57,  25 },
    {  15,  47,   7,  39,  13,  45,   5,  37 },
    {  63,  31,  55,  23,  61,  29,  53,  21 }
};

static const BYTE bayer_16x16[16][16] =
{
    {   0, 128,  32, 160,   8, 136,  40, 168,   2, 130,  34, 162,  10, 138,  42, 170 },
    { 192,  64, 224,  96, 200,  72, 232, 104, 194,  66, 226,  98, 202,  74, 234, 106 },
    {  48, 176,  16, 144,  56, 184,  24, 152,  50, 178,  18, 146,  58, 186,  26, 154 },
    { 240, 112, 208,  80, 248, 120, 216,  88, 242, 114, 210,  82, 250, 122, 218,  90 },
    {  12, 140,  44, 172,   4, 132,  36, 164,  14, 142,  46, 174,   6, 134,  38, 166 },
    { 204,  76, 236, 108, 196,  68, 228, 100, 206,  78, 238, 110, 198,  70, 230, 102 },
    {  60, 188,  28, 156,  52, 180,  20, 148,  62, 190,  30, 158,  54, 182,  22, 150 },
    { 252, 124, 220,  92, 244, 116, 212,  84, 254, 126, 222,  94, 246, 118, 214,  86 },
    {   3, 131,  35, 163,  11, 139,  43, 171,   1, 129,  33, 161,   9, 137,  41, 169 },
    { 195,  67, 227,  99, 203,  75, 235, 107, 193,  65, 225,  97, 201,  73, 233, 105 },
    {  51, 179,  19, 147,  59, 187,  27, 155,  49, 177,  17, 145,  57, 185,  25, 153 },
    { 243, 115, 211,  83, 251, 123, 219,  91, 241, 113, 209,  81, 249, 121, 217,  89 },
    {  15, 143,  47, 175,   7, 135,  39, 167,  13, 141,  45, 173,   5, 133,  37, 165 },
    { 207,  79, 239, 111, 199,  71, 231, 103, 205,  77, 237, 109, 197,  69, 229, 101 },
    {  63, 191,  31, 159,  55, 183,  23, 151,  61, 189,  29, 157,  53, 181,  21, 149 },
    { 255, 127, 223,  95, 247, 119, 215,  87, 253, 125, 221,  93, 245, 117, 213,  85 },
};

static inline DWORD *get_pixel_ptr_32(const dib_info *dib, int x, int y)
{
    return (DWORD *)((BYTE*)dib->bits.ptr + (dib->rect.top + y) * dib->stride + (dib->rect.left + x) * 4);
}

static inline DWORD *get_pixel_ptr_24_dword(const dib_info *dib, int x, int y)
{
    return (DWORD *)((BYTE*)dib->bits.ptr + (dib->rect.top + y) * dib->stride) + (dib->rect.left + x) * 3 / 4;
}

static inline BYTE *get_pixel_ptr_24(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + (dib->rect.top + y) * dib->stride + (dib->rect.left + x) * 3;
}

static inline WORD *get_pixel_ptr_16(const dib_info *dib, int x, int y)
{
    return (WORD *)((BYTE*)dib->bits.ptr + (dib->rect.top + y) * dib->stride + (dib->rect.left + x) * 2);
}

static inline BYTE *get_pixel_ptr_8(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + (dib->rect.top + y) * dib->stride + dib->rect.left + x;
}

static inline BYTE *get_pixel_ptr_4(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + (dib->rect.top + y) * dib->stride + (dib->rect.left + x) / 2;
}

static inline BYTE *get_pixel_ptr_1(const dib_info *dib, int x, int y)
{
    return (BYTE*)dib->bits.ptr + (dib->rect.top + y) * dib->stride + (dib->rect.left + x) / 8;
}

static const BYTE pixel_masks_4[2] = {0xf0, 0x0f};
static const BYTE pixel_masks_1[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static const BYTE edge_masks_1[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

#define FILTER_DIBINDEX(rgbquad,other_val) \
    (HIWORD( *(DWORD *)(&rgbquad) ) == 0x10ff ? LOWORD( *(DWORD *)(&rgbquad) ) : (other_val))

#define ROPS_WITHOUT_COPY( _d, _s )                                     \
case R2_BLACK:        LOOP( (_d) = 0 ) break;                           \
case R2_NOTMERGEPEN:  LOOP( (_d) = ~((_d) | (_s)) ) break;              \
case R2_MASKNOTPEN:   LOOP( (_d) &= ~(_s) ) break;                      \
case R2_NOTCOPYPEN:   LOOP( (_d) = ~(_s) ) break;                       \
case R2_MASKPENNOT:   LOOP( (_d) = (~(_d) & (_s)) ) break;              \
case R2_NOT:          LOOP( (_d) = ~(_d) ) break;                       \
case R2_XORPEN:       LOOP( (_d) ^= (_s) ) break;                       \
case R2_NOTMASKPEN:   LOOP( (_d) = ~((_d) & (_s)) ) break;              \
case R2_MASKPEN:      LOOP( (_d) &= (_s) ) break;                       \
case R2_NOTXORPEN:    LOOP( (_d) = ~((_d) ^ (_s)) ) break;              \
case R2_NOP:          break;                                            \
case R2_MERGENOTPEN:  LOOP( (_d) = ((_d) | ~(_s)) ) break;              \
case R2_MERGEPENNOT:  LOOP( (_d) = (~(_d) | (_s)) ) break;              \
case R2_MERGEPEN:     LOOP( (_d) |= (_s) ) break;                       \
case R2_WHITE:        LOOP( (_d) = ~0 ) break;

#define ROPS_ALL( _d, _s )                                              \
case R2_COPYPEN:      LOOP( (_d) = (_s) ) break;                        \
ROPS_WITHOUT_COPY( (_d), (_s) )

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

static inline void memset_32( DWORD *start, DWORD val, DWORD size )
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    DWORD dummy;
    __asm__ __volatile__( "cld; rep; stosl"
                          : "=c" (dummy), "=D" (dummy)
                          : "a" (val), "0" (size), "1" (start) );
#else
    while (size--) *start++ = val;
#endif
}

static inline void memset_16( WORD *start, WORD val, DWORD size )
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    DWORD dummy;
    __asm__ __volatile__( "cld; rep; stosw"
                          : "=c" (dummy), "=D" (dummy)
                          : "a" (val), "0" (size), "1" (start) );
#else
    while (size--) *start++ = val;
#endif
}

static void solid_rects_32(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    DWORD *ptr, *start;
    int x, y, i;

    for(i = 0; i < num; i++, rc++)
    {
        assert( !IsRectEmpty( rc ));

        start = get_pixel_ptr_32(dib, rc->left, rc->top);
        if (and)
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
                for(x = rc->left, ptr = start; x < rc->right; x++)
                    do_rop_32(ptr++, and, xor);
        else
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
                memset_32( start, xor, rc->right - rc->left );
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
        int left = dib->rect.left + rc->left;
        int right = dib->rect.left + rc->right;

        assert( !IsRectEmpty( rc ));

        if ((left & ~3) == (right & ~3)) /* Special case for lines that start and end in the same DWORD triplet */
        {
            byte_start = get_pixel_ptr_24(dib, rc->left, rc->top);
            for(y = rc->top; y < rc->bottom; y++, byte_start += dib->stride)
            {
                for(x = left, byte_ptr = byte_start; x < right; x++)
                {
                    do_rop_8(byte_ptr++, and_masks[0] & 0xff, xor_masks[0] & 0xff);
                    do_rop_8(byte_ptr++, and_masks[1] & 0xff, xor_masks[1] & 0xff);
                    do_rop_8(byte_ptr++, and_masks[2] & 0xff, xor_masks[2] & 0xff);
                }
            }
        }
        else if (and)
        {
            start = get_pixel_ptr_24_dword(dib, rc->left, rc->top);
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
            {
                ptr = start;

                switch(left & 3)
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

                for(x = (left + 3) & ~3; x < (right & ~3); x += 4)
                {
                    do_rop_32(ptr++, and_masks[0], xor_masks[0]);
                    do_rop_32(ptr++, and_masks[1], xor_masks[1]);
                    do_rop_32(ptr++, and_masks[2], xor_masks[2]);
                }

                switch(right & 3)
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
        else
        {
            start = get_pixel_ptr_24_dword(dib, rc->left, rc->top);
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
            {
                ptr = start;

                switch(left & 3)
                {
                case 1:
                    do_rop_32(ptr++, 0x00ffffff, xor_masks[0] & 0xff000000);
                    *ptr++ = xor_masks[1];
                    *ptr++ = xor_masks[2];
                    break;
                case 2:
                    do_rop_32(ptr++, 0x0000ffff, xor_masks[1] & 0xffff0000);
                    *ptr++ = xor_masks[2];
                    break;
                case 3:
                    do_rop_32(ptr++, 0x000000ff, xor_masks[2] & 0xffffff00);
                    break;
                }

                for(x = (left + 3) & ~3; x < (right & ~3); x += 4)
                {
                    *ptr++ = xor_masks[0];
                    *ptr++ = xor_masks[1];
                    *ptr++ = xor_masks[2];
                }

                switch(right & 3)
                {
                case 1:
                    do_rop_32(ptr, 0xff000000, xor_masks[0] & 0x00ffffff);
                    break;
                case 2:
                    *ptr++ = xor_masks[0];
                    do_rop_32(ptr, 0xffff0000, xor_masks[1] & 0x0000ffff);
                    break;
                case 3:
                    *ptr++ = xor_masks[0];
                    *ptr++ = xor_masks[1];
                    do_rop_32(ptr, 0xffffff00, xor_masks[2] & 0x000000ff);
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
        assert( !IsRectEmpty( rc ));

        start = get_pixel_ptr_16(dib, rc->left, rc->top);
        if (and)
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 2)
                for(x = rc->left, ptr = start; x < rc->right; x++)
                    do_rop_16(ptr++, and, xor);
        else
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 2)
                memset_16( start, xor, rc->right - rc->left );
    }
}

static void solid_rects_8(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    BYTE *ptr, *start;
    int x, y, i;

    for(i = 0; i < num; i++, rc++)
    {
        assert( !IsRectEmpty( rc ));

        start = get_pixel_ptr_8(dib, rc->left, rc->top);
        if (and)
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
                for(x = rc->left, ptr = start; x < rc->right; x++)
                    do_rop_8(ptr++, and, xor);
        else
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
                memset( start, xor, rc->right - rc->left );
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
        int left = dib->rect.left + rc->left;
        int right = dib->rect.left + rc->right;

        assert( !IsRectEmpty( rc ));

        start = get_pixel_ptr_4(dib, rc->left, rc->top);
        if (and)
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                ptr = start;
                if(left & 1) /* upper nibble untouched */
                    do_rop_8(ptr++, byte_and | 0xf0, byte_xor & 0x0f);

                for(x = (left + 1) & ~1; x < (right & ~1); x += 2)
                    do_rop_8(ptr++, byte_and, byte_xor);

                if(right & 1) /* lower nibble untouched */
                    do_rop_8(ptr, byte_and | 0x0f, byte_xor & 0xf0);
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                unsigned int byte_len = (right - ((left + 1) & ~1)) / 2;

                ptr = start;
                if(left & 1) /* upper nibble untouched */
                    do_rop_8(ptr++, 0xf0, byte_xor & 0x0f);

                memset( ptr, byte_xor, byte_len );

                if(right & 1) /* lower nibble untouched */
                    do_rop_8(ptr + byte_len, 0x0f, byte_xor & 0xf0);
            }
        }
    }
}

static void solid_rects_1(const dib_info *dib, int num, const RECT *rc, DWORD and, DWORD xor)
{
    BYTE *ptr, *start;
    int x, y, i;
    BYTE byte_and = (and & 1) ? 0xff : 0;
    BYTE byte_xor = (xor & 1) ? 0xff : 0;

    for(i = 0; i < num; i++, rc++)
    {
        int left = dib->rect.left + rc->left;
        int right = dib->rect.left + rc->right;

        assert( !IsRectEmpty( rc ));

        start = get_pixel_ptr_1(dib, rc->left, rc->top);

        if ((left & ~7) == (right & ~7)) /* Special case for lines that start and end in the same byte */
        {
            BYTE mask = edge_masks_1[left & 7] & ~edge_masks_1[right & 7];

            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                do_rop_8(start, byte_and | ~mask, byte_xor & mask);
            }
        }
        else if (and)
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                ptr = start;

                if(left & 7)
                    do_rop_8(ptr++, byte_and | ~edge_masks_1[left & 7], byte_xor & edge_masks_1[left & 7]);

                for(x = (left + 7) & ~7; x < (right & ~7); x += 8)
                    do_rop_8(ptr++, byte_and, byte_xor);

                if(right & 7)
                    /* this is inverted wrt start mask */
                    do_rop_8(ptr, byte_and | edge_masks_1[right & 7], byte_xor & ~edge_masks_1[right & 7]);
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                unsigned int byte_len = (right - ((left + 7) & ~7)) / 8;

                ptr = start;

                if(left & 7)
                    do_rop_8(ptr++, ~edge_masks_1[left & 7], byte_xor & edge_masks_1[left & 7]);

                memset( ptr, byte_xor, byte_len );

                if(right & 7)
                    do_rop_8(ptr + byte_len, edge_masks_1[right & 7], byte_xor & ~edge_masks_1[right & 7]);
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
    int x = dib->rect.left + start->x;

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
    int x = dib->rect.left + start->x;

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
                             const dib_info *brush, const rop_mask_bits *bits)
{
    DWORD *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i, len, brush_x;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);
        start = get_pixel_ptr_32(dib, rc->left, rc->top);
        start_xor = (DWORD*)bits->xor + offset.y * brush->stride / 4;

        if (bits->and)
        {
            start_and = (DWORD*)bits->and + offset.y * brush->stride / 4;

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
                    start_and = bits->and;
                    start_xor = bits->xor;
                    offset.y = 0;
                }
                else
                {
                    start_and += brush->stride / 4;
                    start_xor += brush->stride / 4;
                }
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 4)
            {
                for (x = rc->left, brush_x = offset.x; x < rc->right; x += len)
                {
                    len = min( rc->right - x, brush->width - brush_x );
                    memcpy( start + x - rc->left, start_xor + brush_x, len * 4 );
                    brush_x = 0;
                }

                start_xor += brush->stride / 4;
                offset.y++;
                if(offset.y == brush->height)
                {
                    start_xor = bits->xor;
                    offset.y = 0;
                }
            }
        }
    }
}

static void pattern_rects_24(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                             const dib_info *brush, const rop_mask_bits *bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i, len, brush_x;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_24(dib, rc->left, rc->top);
        start_xor = (BYTE*)bits->xor + offset.y * brush->stride;

        if (bits->and)
        {
            start_and = (BYTE*)bits->and + offset.y * brush->stride;
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
                    start_and = bits->and;
                    start_xor = bits->xor;
                    offset.y = 0;
                }
                else
                {
                    start_and += brush->stride;
                    start_xor += brush->stride;
                }
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                for (x = rc->left, brush_x = offset.x; x < rc->right; x += len)
                {
                    len = min( rc->right - x, brush->width - brush_x );
                    memcpy( start + (x - rc->left) * 3, start_xor + brush_x * 3, len * 3 );
                    brush_x = 0;
                }

                start_xor += brush->stride;
                offset.y++;
                if(offset.y == brush->height)
                {
                    start_xor = bits->xor;
                    offset.y = 0;
                }
            }
        }
    }
}

static void pattern_rects_16(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                             const dib_info *brush, const rop_mask_bits *bits)
{
    WORD *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i, len, brush_x;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_16(dib, rc->left, rc->top);
        start_xor = (WORD*)bits->xor + offset.y * brush->stride / 2;

        if (bits->and)
        {
            start_and = (WORD*)bits->and + offset.y * brush->stride / 2;
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
                    start_and = bits->and;
                    start_xor = bits->xor;
                    offset.y = 0;
                }
                else
                {
                    start_and += brush->stride / 2;
                    start_xor += brush->stride / 2;
                }
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride / 2)
            {
                for (x = rc->left, brush_x = offset.x; x < rc->right; x += len)
                {
                    len = min( rc->right - x, brush->width - brush_x );
                    memcpy( start + x - rc->left, start_xor + brush_x, len * 2 );
                    brush_x = 0;
                }

                start_xor += brush->stride / 2;
                offset.y++;
                if(offset.y == brush->height)
                {
                    start_xor = bits->xor;
                    offset.y = 0;
                }
            }
        }
    }
}

static void pattern_rects_8(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                            const dib_info *brush, const rop_mask_bits *bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i, len, brush_x;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);

        start = get_pixel_ptr_8(dib, rc->left, rc->top);
        start_xor = (BYTE*)bits->xor + offset.y * brush->stride;

        if (bits->and)
        {
            start_and = (BYTE*)bits->and + offset.y * brush->stride;
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
                    start_and = bits->and;
                    start_xor = bits->xor;
                    offset.y = 0;
                }
                else
                {
                    start_and += brush->stride;
                    start_xor += brush->stride;
                }
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                for (x = rc->left, brush_x = offset.x; x < rc->right; x += len)
                {
                    len = min( rc->right - x, brush->width - brush_x );
                    memcpy( start + x - rc->left, start_xor + brush_x, len );
                    brush_x = 0;
                }

                start_xor += brush->stride;
                offset.y++;
                if(offset.y == brush->height)
                {
                    start_xor = bits->xor;
                    offset.y = 0;
                }
            }
        }
    }
}

static void pattern_rects_4(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                            const dib_info *brush, const rop_mask_bits *bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i, left, right;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);
        left = dib->rect.left + rc->left;
        right = dib->rect.left + rc->right;

        start = get_pixel_ptr_4(dib, rc->left, rc->top);
        start_xor = (BYTE*)bits->xor + offset.y * brush->stride;

        if (bits->and)
        {
            start_and = (BYTE*)bits->and + offset.y * brush->stride;
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                INT brush_x = offset.x;
                BYTE byte_and, byte_xor;

                and_ptr = start_and + brush_x / 2;
                xor_ptr = start_xor + brush_x / 2;

                for(x = left, ptr = start; x < right; x++)
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
                    start_and = bits->and;
                    start_xor = bits->xor;
                    offset.y = 0;
                }
                else
                {
                    start_and += brush->stride;
                    start_xor += brush->stride;
                }
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                INT brush_x = offset.x;
                BYTE byte_xor;

                xor_ptr = start_xor + brush_x / 2;

                for(x = left, ptr = start; x < right; x++)
                {
                    /* FIXME: Two pixels at a time */
                    if(x & 1) /* lower dst nibble */
                    {
                        if(brush_x & 1) /* lower pat nibble */
                            byte_xor = *xor_ptr++ & 0x0f;
                        else /* upper pat nibble */
                            byte_xor = (*xor_ptr >> 4) & 0x0f;
                        do_rop_8(ptr, 0xf0, byte_xor);
                    }
                    else /* upper dst nibble */
                    {
                        if(brush_x & 1) /* lower pat nibble */
                            byte_xor = (*xor_ptr++ << 4) & 0xf0;
                        else /* upper pat nibble */
                            byte_xor = *xor_ptr & 0xf0;
                        do_rop_8(ptr, 0x0f, byte_xor);
                    }

                    if(x & 1) ptr++;

                    if(++brush_x == brush->width)
                    {
                        brush_x = 0;
                        xor_ptr = start_xor;
                    }
                }

                start_xor += brush->stride;
                offset.y++;
                if(offset.y == brush->height)
                {
                    start_xor = bits->xor;
                    offset.y = 0;
                }
            }
        }
    }
}

static void pattern_rects_1(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                            const dib_info *brush, const rop_mask_bits *bits)
{
    BYTE *ptr, *start, *start_and, *and_ptr, *start_xor, *xor_ptr;
    int x, y, i, left, right;
    POINT offset;

    for(i = 0; i < num; i++, rc++)
    {
        offset = calc_brush_offset(rc, brush, origin);
        left = dib->rect.left + rc->left;
        right = dib->rect.left + rc->right;

        start = get_pixel_ptr_1(dib, rc->left, rc->top);
        start_xor = (BYTE*)bits->xor + offset.y * brush->stride;

        if (bits->and)
        {
            start_and = (BYTE*)bits->and + offset.y * brush->stride;
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                INT brush_x = offset.x;
                BYTE byte_and, byte_xor;

                and_ptr = start_and + brush_x / 8;
                xor_ptr = start_xor + brush_x / 8;

                for(x = left, ptr = start; x < right; x++)
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
                    start_and = bits->and;
                    start_xor = bits->xor;
                    offset.y = 0;
                }
                else
                {
                    start_and += brush->stride;
                    start_xor += brush->stride;
                }
            }
        }
        else
        {
            for(y = rc->top; y < rc->bottom; y++, start += dib->stride)
            {
                INT brush_x = offset.x;

                xor_ptr = start_xor + brush_x / 8;

                for(x = left, ptr = start; x < right; x++)
                {
                    BYTE byte_xor = (*xor_ptr & pixel_masks_1[brush_x % 8]) ? 0xff : 0;
                    byte_xor &= pixel_masks_1[x % 8];

                    do_rop_8(ptr, ~pixel_masks_1[x % 8], byte_xor);

                    if((x & 7) == 7) ptr++;
                    if((brush_x & 7) == 7) xor_ptr++;

                    if(++brush_x == brush->width)
                    {
                        brush_x = 0;
                        xor_ptr = start_xor;
                    }
                }

                start_xor += brush->stride;
                offset.y++;
                if(offset.y == brush->height)
                {
                    start_xor = bits->xor;
                    offset.y = 0;
                }
            }
        }
    }
}

static void pattern_rects_null(const dib_info *dib, int num, const RECT *rc, const POINT *origin,
                               const dib_info *brush, const rop_mask_bits *bits)
{
    return;
}

static inline void copy_rect_bits_32( DWORD *dst_start, const DWORD *src_start, const SIZE *size,
                                      int dst_stride, int src_stride, int rop2 )
{
    const DWORD *src;
    DWORD *dst;
    int x, y;

#define LOOP( op )                                                                     \
    for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride)   \
        for (x = 0, src = src_start, dst = dst_start; x < size->cx; x++, src++, dst++) \
            op;

    switch (rop2)
    {
        ROPS_WITHOUT_COPY( dst[0], src[0] )
    }
#undef LOOP
}

static inline void copy_rect_bits_rev_32( DWORD *dst_start, const DWORD *src_start, const SIZE *size,
                                          int dst_stride, int src_stride, int rop2 )
{
    const DWORD *src;
    DWORD *dst;
    int x, y;

    src_start += size->cx - 1;
    dst_start += size->cx - 1;

#define LOOP( op )                                                                     \
    for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride)   \
        for (x = 0, src = src_start, dst = dst_start; x < size->cx; x++, src--, dst--) \
            op;

    switch (rop2)
    {
        ROPS_WITHOUT_COPY( dst[0], src[0] )
    }
#undef LOOP
}

static void copy_rect_32(const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    DWORD *dst_start, *src_start;
    int y, dst_stride, src_stride;
    SIZE size;

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

    size.cx = rc->right - rc->left;
    size.cy = rc->bottom - rc->top;

    if (overlap & OVERLAP_RIGHT)
        copy_rect_bits_rev_32( dst_start, src_start, &size, dst_stride, src_stride, rop2 );
    else
        copy_rect_bits_32( dst_start, src_start, &size, dst_stride, src_stride, rop2 );
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
    int left = dst->rect.left + rc->left;
    int right = dst->rect.left + rc->right;
    int org_x = src->rect.left + origin->x;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_4(dst, rc->left, rc->bottom - 1);
        src_start = get_pixel_ptr_4(src, origin->x, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride;
        src_stride = -src->stride;
    }
    else
    {
        dst_start = get_pixel_ptr_4(dst, rc->left, rc->top);
        src_start = get_pixel_ptr_4(src, origin->x, origin->y);
        dst_stride = dst->stride;
        src_stride = src->stride;
    }

    if (rop2 == R2_COPYPEN && (left & 1) == 0 && (org_x & 1) == 0 && (right & 1) == 0)
    {
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (right - left) / 2 );
        return;
    }

    get_rop_codes( rop2, &codes );
    for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
    {
        if (overlap & OVERLAP_RIGHT)
            do_rop_codes_line_rev_4( dst_start, left & 1, src_start, org_x & 1, &codes, right - left );
        else
            do_rop_codes_line_4( dst_start, left & 1, src_start, org_x & 1, &codes, right - left );
    }
}

static inline void copy_rect_bits_partial_1( BYTE *dst_start, int dst_x, const BYTE *src_start, int src_x,
                                             const SIZE *size, int dst_stride, int src_stride, int rop2 )
{
    const BYTE *src;
    BYTE *dst, src_val, mask;
    int dst_end = dst_x + size->cx, y;
    int off = (src_x & 7) - (dst_x & 7);
    struct rop_codes codes;

    get_rop_codes( rop2, &codes );

    src_start += src_x / 8;
    dst_start += dst_x / 8;

    for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride)
    {
        dst = dst_start;
        src = src_start;
        if (off == 0)
            src_val = src[0];
        else if (off > 0)
        {
            src_val = src[0] << off;
            if ((dst_end & 7) + off > 8)
                src_val |= (src[1] >> (8 - off));
        }
        else
            src_val = src[0] >> -off;

        mask = edge_masks_1[dst_x & 7];
        if (dst_end & 7)
            mask &= ~edge_masks_1[dst_end & 7];
        do_rop_codes_mask_8( dst, src_val, &codes, mask );
    }
}

static inline void copy_rect_bits_align_1( BYTE *dst_start, int dst_x, const BYTE *src_start, int src_x,
                                           const SIZE *size, int dst_stride, int src_stride, int rop2 )
{
    const BYTE *src;
    BYTE *dst, mask;
    int y, i, full_bytes, dst_end = dst_x + size->cx;
    struct rop_codes codes;

    get_rop_codes( rop2, &codes );

    src_start += src_x / 8;
    dst_start += dst_x / 8;
    full_bytes = (dst_end - ((dst_x + 7) & ~7)) / 8;

    switch( rop2 )
    {
    case R2_COPYPEN:
        for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride)
        {
            dst = dst_start;
            src = src_start;
            if (dst_x & 7)
            {
                mask = edge_masks_1[dst_x & 7];
                do_rop_codes_mask_8( dst, src[0], &codes, mask );
                src++;
                dst++;
            }
            memmove( dst, src, full_bytes );
            src += full_bytes;
            dst += full_bytes;
            if (dst_end & 7)
            {
                mask = ~edge_masks_1[dst_end & 7];
                do_rop_codes_mask_8( dst, src[0], &codes, mask );
            }
        }
        break;

#define LOOP( op )                                                      \
        for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride) \
        {                                                               \
            dst = dst_start;                                            \
            src = src_start;                                            \
            if (dst_x & 7)                                              \
            {                                                           \
                mask = edge_masks_1[dst_x & 7];                         \
                do_rop_codes_mask_8( dst, src[0], &codes, mask );       \
                src++;                                                  \
                dst++;                                                  \
            }                                                           \
            for (i = 0; i < full_bytes; i++, src++, dst++)              \
                op;                                                     \
            if (dst_end & 7)                                            \
            {                                                           \
                mask = ~edge_masks_1[dst_end & 7];                      \
                do_rop_codes_mask_8( dst, src[0], &codes, mask );       \
            }                                                           \
        }

        ROPS_WITHOUT_COPY( dst[0], src[0] );
    }
#undef LOOP
}

static inline void copy_rect_bits_shl_1( BYTE *dst_start, int dst_x, const BYTE *src_start, int src_x,
                                         const SIZE *size, int dst_stride, int src_stride, int rop2 )
{
    const BYTE *src;
    BYTE *dst, mask, src_val;
    int y, i, full_bytes, dst_end = dst_x + size->cx;
    int off = (src_x & 7) - (dst_x & 7);
    struct rop_codes codes;

    get_rop_codes( rop2, &codes );

    src_start += src_x / 8;
    dst_start += dst_x / 8;
    full_bytes = (dst_end - ((dst_x + 7) & ~7)) / 8;

#define LOOP( op )                                                      \
    for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride) \
    {                                                                   \
        dst = dst_start;                                                \
        src = src_start;                                                \
        if (dst_x & 7)                                                  \
        {                                                               \
            src_val = (src[0] << off) | (src[1] >> (8 - off));          \
            mask = edge_masks_1[dst_x & 7];                             \
            do_rop_codes_mask_8( dst, src_val, &codes, mask );          \
            src++;                                                      \
            dst++;                                                      \
        }                                                               \
        for (i = 0; i < full_bytes; i++, src++, dst++)                  \
            op;                                                         \
        if (dst_end & 7)                                                \
        {                                                               \
            src_val = src[0] << off;                                    \
            if ((dst_end & 7) + off > 8)                                \
                src_val |= (src[1] >> (8 - off));                       \
            mask = ~edge_masks_1[dst_end & 7];                          \
            do_rop_codes_mask_8( dst, src_val, &codes, mask );          \
        }                                                               \
    }

    switch( rop2 )
    {
        ROPS_ALL( dst[0], ((src[0] << off) | (src[1] >> (8 - off))) );
    }
#undef LOOP
}

static inline void copy_rect_bits_shr_1( BYTE *dst_start, int dst_x, const BYTE *src_start, int src_x,
                                         const SIZE *size, int dst_stride, int src_stride, int rop2 )
{
    const BYTE *src;
    BYTE *dst, mask, src_val, last_src;
    int y, i, full_bytes, dst_end = dst_x + size->cx;
    int off = (src_x & 7) - (dst_x & 7);
    struct rop_codes codes;

    get_rop_codes( rop2, &codes );

    src_start += src_x / 8;
    dst_start += dst_x / 8;
    full_bytes = (dst_end - ((dst_x + 7) & ~7)) / 8;

#define LOOP( op )                                                      \
    for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride) \
    {                                                                   \
        dst = dst_start;                                                \
        src = src_start;                                                \
        last_src = 0;                                                   \
        if (dst_x & 7)                                                  \
        {                                                               \
            last_src = src[0];                                          \
            mask = edge_masks_1[dst_x & 7];                             \
            do_rop_codes_mask_8( dst, src[0] >> -off, &codes, mask );   \
            src++;                                                      \
            dst++;                                                      \
        }                                                               \
        for (i = 0; i < full_bytes; i++, src++, dst++)                  \
        {                                                               \
            src_val = (last_src << (8 + off)) | (src[0] >> -off);       \
            last_src = src[0];                                          \
            op;                                                         \
        }                                                               \
        if (dst_end & 7)                                                \
        {                                                               \
            src_val = last_src << (8 + off);                            \
            if ((dst_end & 7) + off > 0)                                \
                src_val |= (src[0] >> -off);                            \
            mask = ~edge_masks_1[dst_end & 7];                          \
            do_rop_codes_mask_8( dst, src_val, &codes, mask );          \
        }                                                               \
    }

    switch( rop2 )
    {
        ROPS_ALL( dst[0], src_val )
    }
#undef LOOP
}

static inline void copy_rect_bits_rev_align_1( BYTE *dst_start, int dst_x, const BYTE *src_start, int src_x,
                                               const SIZE *size, int dst_stride, int src_stride, int rop2 )
{
    const BYTE *src;
    BYTE *dst, mask;
    int y, i, full_bytes, dst_end = dst_x + size->cx, src_end = src_x + size->cx;
    struct rop_codes codes;

    get_rop_codes( rop2, &codes );

    src_start += (src_end - 1) / 8;
    dst_start += (dst_end - 1) / 8;
    full_bytes = (dst_end - ((dst_x + 7) & ~7)) / 8;

    switch( rop2 )
    {
    case R2_COPYPEN:
        for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride)
        {
            dst = dst_start;
            src = src_start;
            if (dst_end & 7)
            {
                mask = ~edge_masks_1[dst_end & 7];
                do_rop_codes_mask_8( dst, src[0], &codes, mask );
                src--;
                dst--;
            }
            memmove( dst, src, full_bytes );
            src -= full_bytes;
            dst -= full_bytes;
            if (dst_x & 7)
            {
                mask = edge_masks_1[dst_x & 7];
                do_rop_codes_mask_8( dst, src[0], &codes, mask );
            }
        }
        break;

#define LOOP( op )                                                      \
        for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride) \
        {                                                               \
            dst = dst_start;                                            \
            src = src_start;                                            \
            if (dst_end & 7)                                            \
            {                                                           \
                mask = ~edge_masks_1[dst_end & 7];                      \
                do_rop_codes_mask_8( dst, src[0], &codes, mask );       \
                src--;                                                  \
                dst--;                                                  \
            }                                                           \
            for (i = 0; i < full_bytes; i++, src--, dst--)              \
                op;                                                     \
            if (dst_x & 7)                                              \
            {                                                           \
                mask = edge_masks_1[dst_x & 7];                         \
                do_rop_codes_mask_8( dst, src[0], &codes, mask );       \
            }                                                           \
        }

        ROPS_WITHOUT_COPY( dst[0], src[0] );
    }
#undef LOOP
}

static inline void copy_rect_bits_rev_shl_1( BYTE *dst_start, int dst_x, const BYTE *src_start, int src_x,
                                             const SIZE *size, int dst_stride, int src_stride, int rop2 )
{
    const BYTE *src;
    BYTE *dst, mask, src_val, last_src;
    int y, i, full_bytes, dst_end = dst_x + size->cx, src_end = src_x + size->cx;
    int off = ((src_end - 1) & 7) - ((dst_end - 1) & 7);
    struct rop_codes codes;

    get_rop_codes( rop2, &codes );

    src_start += (src_end - 1) / 8;
    dst_start += (dst_end - 1) / 8;
    full_bytes = (dst_end - ((dst_x + 7) & ~7)) / 8;

#define LOOP( op )                                                      \
    for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride) \
    {                                                                   \
        dst = dst_start;                                                \
        src = src_start;                                                \
        last_src = 0;                                                   \
        if (dst_end & 7)                                                \
        {                                                               \
            last_src = src[0];                                          \
            mask = ~edge_masks_1[dst_end & 7];                          \
            do_rop_codes_mask_8( dst, src[0] << off, &codes, mask );    \
            src--;                                                      \
            dst--;                                                      \
        }                                                               \
        for (i = 0; i < full_bytes; i++, src--, dst--)                  \
        {                                                               \
            src_val = (src[0] << off) | (last_src >> (8 - off));        \
            last_src = src[0];                                          \
            op;                                                         \
        }                                                               \
        if (dst_x & 7)                                                  \
        {                                                               \
            src_val = last_src >> (8 - off);                            \
            if ((dst_x & 7) + off < 8)                                  \
                src_val |= (src[0] << off);                             \
            mask = edge_masks_1[dst_x & 7];                             \
            do_rop_codes_mask_8( dst, src_val, &codes, mask );          \
        }                                                               \
    }

    switch( rop2 )
    {
        ROPS_ALL( dst[0], src_val );
    }
#undef LOOP
}

static inline void copy_rect_bits_rev_shr_1( BYTE *dst_start, int dst_x, const BYTE *src_start, int src_x,
                                             const SIZE *size, int dst_stride, int src_stride, int rop2 )
{
    const BYTE *src;
    BYTE *dst, mask, src_val;
    int y, i, full_bytes, dst_end = dst_x + size->cx, src_end = src_x + size->cx;
    int off = ((src_end - 1) & 7) - ((dst_end - 1) & 7);
    struct rop_codes codes;

    get_rop_codes( rop2, &codes );

    src_start += (src_end - 1) / 8;
    dst_start += (dst_end - 1) / 8;
    full_bytes = (dst_end - ((dst_x + 7) & ~7)) / 8;

#define LOOP( op )                                                      \
    for (y = 0; y < size->cy; y++, dst_start += dst_stride, src_start += src_stride) \
    {                                                                   \
        dst = dst_start;                                                \
        src = src_start;                                                \
        if (dst_end & 7)                                                \
        {                                                               \
            mask = edge_masks_1[dst_x & 7];                             \
            do_rop_codes_mask_8( dst, (src[-1] << (8 + off)) | (src[0] >> -off), &codes, mask ); \
            src--;                                                      \
            dst--;                                                      \
        }                                                               \
        for (i = 0; i < full_bytes; i++, src--, dst--)                  \
            op;                                                         \
        if (dst_x & 7)                                                  \
        {                                                               \
            src_val = src[0] >> -off;                                   \
            if ((dst_x & 7) + off < 0)                                  \
                src_val |= (src[-1] << (8 + off));                      \
            mask = edge_masks_1[dst_x & 7];                             \
            do_rop_codes_mask_8( dst, src_val, &codes, mask );          \
        }                                                               \
    }

    switch( rop2 )
    {
        ROPS_ALL( dst[0], (src[-1] << (8 + off)) | (src[0] >> -off) );
    }
#undef LOOP
}

static void copy_rect_1(const dib_info *dst, const RECT *rc,
                        const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    BYTE *dst_start, *src_start;
    int y, dst_stride, src_stride;
    int left = dst->rect.left + rc->left;
    int right = dst->rect.left + rc->right;
    int org_x = src->rect.left + origin->x;
    SIZE size;

    if (overlap & OVERLAP_BELOW)
    {
        dst_start = get_pixel_ptr_1(dst, rc->left, rc->bottom - 1);
        src_start = get_pixel_ptr_1(src, origin->x, origin->y + rc->bottom - rc->top - 1);
        dst_stride = -dst->stride;
        src_stride = -src->stride;
    }
    else
    {
        dst_start = get_pixel_ptr_1(dst, rc->left, rc->top);
        src_start = get_pixel_ptr_1(src, origin->x, origin->y);
        dst_stride = dst->stride;
        src_stride = src->stride;
    }

    if (rop2 == R2_COPYPEN && (left & 7) == 0 && (org_x & 7) == 0 && (right & 7) == 0)
    {
        for (y = rc->top; y < rc->bottom; y++, dst_start += dst_stride, src_start += src_stride)
            memmove( dst_start, src_start, (right - left) / 8 );
        return;
    }

    size.cx = right - left;
    size.cy = rc->bottom - rc->top;

    /* Special case starting and finishing in same byte, neither on byte boundary */
    if ((left & 7) && (right & 7) && (left & ~7) == (right & ~7))
        copy_rect_bits_partial_1( dst_start, left & 7, src_start, org_x & 7, &size, dst_stride, src_stride, rop2 );
    else if (overlap & OVERLAP_RIGHT)
    {
        int off = ((org_x + right - left - 1) & 7) - ((right - 1) & 7);

        if (off == 0)
            copy_rect_bits_rev_align_1( dst_start, left & 7, src_start, org_x & 7, &size, dst_stride, src_stride, rop2 );
        else if (off > 0)
            copy_rect_bits_rev_shl_1( dst_start, left & 7, src_start, org_x & 7, &size, dst_stride, src_stride, rop2 );
        else
            copy_rect_bits_rev_shr_1( dst_start, left & 7, src_start, org_x & 7, &size, dst_stride, src_stride, rop2 );
    }
    else
    {
        int off = (org_x & 7) - (left & 7);

        if (off == 0)
            copy_rect_bits_align_1( dst_start, left & 7, src_start, org_x & 7, &size, dst_stride, src_stride, rop2 );
        else if (off > 0)
            copy_rect_bits_shl_1( dst_start, left & 7, src_start, org_x & 7, &size, dst_stride, src_stride, rop2 );
        else
            copy_rect_bits_shr_1( dst_start, left & 7, src_start, org_x & 7, &size, dst_stride, src_stride, rop2 );
    }
}

static void copy_rect_null(const dib_info *dst, const RECT *rc,
                           const dib_info *src, const POINT *origin, int rop2, int overlap)
{
    return;
}

static DWORD get_pixel_32(const dib_info *dib, int x, int y)
{
    DWORD *ptr = get_pixel_ptr_32( dib, x, y );
    return *ptr;
}

static DWORD get_pixel_24(const dib_info *dib, int x, int y)
{
    BYTE *ptr = get_pixel_ptr_24( dib, x, y );
    return ptr[0] | ((DWORD)ptr[1] << 8) | ((DWORD)ptr[2] << 16);
}

static DWORD get_pixel_16(const dib_info *dib, int x, int y)
{
    WORD *ptr = get_pixel_ptr_16( dib, x, y );
    return *ptr;
}

static DWORD get_pixel_8(const dib_info *dib, int x, int y)
{
    BYTE *ptr = get_pixel_ptr_8( dib, x, y );
    return *ptr;
}

static DWORD get_pixel_4(const dib_info *dib, int x, int y)
{
    BYTE *ptr = get_pixel_ptr_4( dib, x, y );

    if ((dib->rect.left + x) & 1)
        return *ptr & 0x0f;
    else
        return (*ptr >> 4) & 0x0f;
}

static DWORD get_pixel_1(const dib_info *dib, int x, int y)
{
    BYTE *ptr = get_pixel_ptr_1( dib, x, y );
    return (*ptr & pixel_masks_1[(dib->rect.left + x) & 7]) ? 1 : 0;
}

static DWORD get_pixel_null(const dib_info *dib, int x, int y)
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

static DWORD rgb_to_pixel_masks(const dib_info *dib, DWORD r, DWORD g, DWORD b)
{
    return put_field(r, dib->red_shift,   dib->red_len) |
           put_field(g, dib->green_shift, dib->green_len) |
           put_field(b, dib->blue_shift,  dib->blue_len);
}

static DWORD rgbquad_to_pixel_masks(const dib_info *dib, RGBQUAD rgb)
{
    return rgb_to_pixel_masks(dib, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
}

static DWORD colorref_to_pixel_masks(const dib_info *dib, COLORREF colour)
{
    return rgb_to_pixel_masks(dib, GetRValue(colour), GetGValue(colour), GetBValue(colour));
}

static DWORD colorref_to_pixel_555(const dib_info *dib, COLORREF color)
{
    return ( ((color >> 19) & 0x1f) | ((color >> 6) & 0x03e0) | ((color << 7) & 0x7c00) );
}

static DWORD rgb_to_pixel_colortable(const dib_info *dib, BYTE r, BYTE g, BYTE b)
{
    const RGBQUAD *color_table = get_dib_color_table( dib );
    int size = dib->color_table ? dib->color_table_size : 1 << dib->bit_count;
    int i, best_index = 0;
    DWORD diff, best_diff = 0xffffffff;

    for(i = 0; i < size; i++)
    {
        const RGBQUAD *cur = color_table + i;
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

static DWORD rgb_to_pixel_mono(const dib_info *dib, BOOL dither, int x, int y,
                               DWORD src_pixel, DWORD bg_pixel, BYTE r, BYTE g, BYTE b)
{
    DWORD ret;

    if (dib->color_table_size != 1)
    {
        if (dither)
        {
            if (((30 * r + 59 * g + 11 * b) / 100 + bayer_16x16[y % 16][x % 16]) > 255) r = g = b = 255;
            else r = g = b = 0;
        }
        ret = rgb_to_pixel_colortable( dib, r, g, b );
    }
    else ret = (src_pixel == bg_pixel);  /* only match raw pixel value */

    return ret ? 0xff : 0;
}

static DWORD rgbquad_to_pixel_colortable(const dib_info *dib, RGBQUAD rgb)
{
    return rgb_to_pixel_colortable( dib, rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue );
}

static DWORD colorref_to_pixel_colortable(const dib_info *dib, COLORREF color)
{
    return rgb_to_pixel_colortable( dib, GetRValue(color), GetGValue(color), GetBValue(color) );
}

static DWORD colorref_to_pixel_null(const dib_info *dib, COLORREF color)
{
    return 0;
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
    const RGBQUAD *color_table = get_dib_color_table( dib );

    if (!dib->color_table || pixel < dib->color_table_size)
    {
        RGBQUAD quad = color_table[pixel];
        return RGB( quad.rgbRed, quad.rgbGreen, quad.rgbBlue );
    }
    return 0;
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

static void convert_to_8888(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
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
            if (src->stride > 0 && src->stride == dst->stride && !pad_size)
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        DWORD dst_colors[256], i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = color_table[i].rgbRed << 16 | color_table[i].rgbGreen << 8 |
                color_table[i].rgbBlue;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
                *dst_pixel++ = dst_colors[*src_pixel++];

            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        DWORD dst_colors[16], i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = color_table[i].rgbRed << 16 | color_table[i].rgbGreen << 8 |
                color_table[i].rgbBlue;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 1;
            src_pixel = src_start;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                if (pos & 1)
                    dst_start[x] = dst_colors[*src_pixel++ & 0xf];
                else
                    dst_start[x] = dst_colors[*src_pixel >> 4];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        DWORD dst_colors[2], i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = color_table[i].rgbRed << 16 | color_table[i].rgbGreen << 8 |
                color_table[i].rgbBlue;

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            for(x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                dst_start[x] = dst_colors[src_val];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_32(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst, src_val >> 16, src_val >> 8, src_val);
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 4;
                src_start += src->stride / 4;
            }
        }
        else if(bit_fields_match(src, dst))
        {
            if (src->stride > 0 && src->stride == dst->stride && !pad_size)
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      get_field(src_val, src->red_shift, src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift, src->blue_len));
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
            for(x = src_rect->left; x < src_rect->right; x++, src_pixel += 3)
                *dst_pixel++ = rgb_to_pixel_masks(dst, src_pixel[2], src_pixel[1], src_pixel[0]);
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      ((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07),
                                                      ((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07),
                                                      ((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07));
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 3) & 0xf8) |
                                                      (((src_val >> src->green_shift) >> 2) & 0x07),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07));
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 2) & 0xfc) |
                                                      (((src_val >> src->green_shift) >> 4) & 0x03),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07));
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      get_field(src_val, src->red_shift,   src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift,  src->blue_len));
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        DWORD dst_colors[256], i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
                *dst_pixel++ = dst_colors[*src_pixel++];

            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        DWORD dst_colors[16], i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 1;
            src_pixel = src_start;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                if (pos & 1)
                    dst_start[x] = dst_colors[*src_pixel++ & 0xf];
                else
                    dst_start[x] = dst_colors[*src_pixel >> 4];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        DWORD dst_colors[2], i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                dst_start[x] = dst_colors[src_val];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_24(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
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

        if (src->stride > 0 && src->stride == dst->stride && !pad_size)
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                RGBQUAD rgb = color_table[*src_pixel++];
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 1;
            src_pixel = src_start;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                RGBQUAD rgb;
                if (pos & 1)
                    rgb = color_table[*src_pixel++ & 0xf];
                else
                    rgb = color_table[*src_pixel >> 4];
                dst_start[x * 3] = rgb.rgbBlue;
                dst_start[x * 3 + 1] = rgb.rgbGreen;
                dst_start[x * 3 + 2] = rgb.rgbRed;
            }
            if(pad_size) memset(dst_start + x * 3, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                RGBQUAD rgb;
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                rgb = color_table[src_val];
                dst_start[x * 3] = rgb.rgbBlue;
                dst_start[x * 3 + 1] = rgb.rgbGreen;
                dst_start[x * 3 + 2] = rgb.rgbRed;
            }
            if(pad_size) memset(dst_start + x * 3, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_555(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
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
            if (src->stride > 0 && src->stride == dst->stride && !pad_size)
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        WORD dst_colors[256];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = ((color_table[i].rgbRed   << 7) & 0x7c00) |
                            ((color_table[i].rgbGreen << 2) & 0x03e0) |
                            ((color_table[i].rgbBlue  >> 3) & 0x001f);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
                *dst_pixel++ = dst_colors[*src_pixel++];

            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        WORD dst_colors[16];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = ((color_table[i].rgbRed   << 7) & 0x7c00) |
                            ((color_table[i].rgbGreen << 2) & 0x03e0) |
                            ((color_table[i].rgbBlue  >> 3) & 0x001f);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 1;
            src_pixel = src_start;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                if (pos & 1)
                    dst_start[x] = dst_colors[*src_pixel++ & 0xf];
                else
                    dst_start[x] = dst_colors[*src_pixel >> 4];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        WORD dst_colors[2];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = ((color_table[i].rgbRed   << 7) & 0x7c00) |
                            ((color_table[i].rgbGreen << 2) & 0x03e0) |
                            ((color_table[i].rgbBlue  >> 3) & 0x001f);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                dst_start[x] = dst_colors[src_val];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_16(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst, src_val >> 16, src_val >> 8, src_val);
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      src_val >> src->red_shift,
                                                      src_val >> src->green_shift,
                                                      src_val >> src->blue_shift);
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      get_field(src_val, src->red_shift, src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift, src->blue_len ));
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
            for(x = src_rect->left; x < src_rect->right; x++, src_pixel += 3)
                *dst_pixel++ = rgb_to_pixel_masks(dst, src_pixel[2], src_pixel[1], src_pixel[0]);
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      ((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07),
                                                      ((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07),
                                                      ((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07));
                }
                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride / 2;
                src_start += src->stride / 2;
            }
        }
        else if(bit_fields_match(src, dst))
        {
            if (src->stride > 0 && src->stride == dst->stride && !pad_size)
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 3) & 0xf8) |
                                                      (((src_val >> src->green_shift) >> 2) & 0x07),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07));
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                      (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                      (((src_val >> src->green_shift) << 2) & 0xfc) |
                                                      (((src_val >> src->green_shift) >> 4) & 0x03),
                                                      (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                      (((src_val >> src->blue_shift)  >> 2) & 0x07));
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
                    *dst_pixel++ = rgb_to_pixel_masks(dst,
                                                      get_field(src_val, src->red_shift,   src->red_len),
                                                      get_field(src_val, src->green_shift, src->green_len),
                                                      get_field(src_val, src->blue_shift,  src->blue_len));
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        WORD dst_colors[256];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
                *dst_pixel++ = dst_colors[*src_pixel++];

            if(pad_size) memset(dst_pixel, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 4:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        WORD dst_colors[16];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 1;
            src_pixel = src_start;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                if (pos & 1)
                    dst_start[x] = dst_colors[*src_pixel++ & 0xf];
                else
                    dst_start[x] = dst_colors[*src_pixel >> 4];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        WORD dst_colors[2];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                dst_start[x] = dst_colors[src_val];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        break;
    }
    }
}

static inline BOOL color_tables_match(const dib_info *d1, const dib_info *d2)
{
    if (!d1->color_table || !d2->color_table) return (!d1->color_table && !d2->color_table);
    return !memcmp(d1->color_table, d2->color_table, (1 << d1->bit_count) * sizeof(d1->color_table[0]));
}

/*
 * To translate an RGB value into a colour table index Windows uses the 5 msbs of each component.
 * We thus create a lookup table with 32^3 entries.
 */
struct rgb_lookup_colortable_ctx
{
    const dib_info *dib;
    BYTE map[32 * 32 * 32];
    BYTE valid[32 * 32 * 32 / 8];
};

static void rgb_lookup_colortable_init(const dib_info *dib, struct rgb_lookup_colortable_ctx *ctx)
{
    ctx->dib = dib;
    memset(ctx->valid, 0, sizeof(ctx->valid));
}

static inline BYTE rgb_lookup_colortable(struct rgb_lookup_colortable_ctx *ctx, DWORD r, DWORD g, DWORD b)
{
    DWORD pos = ((r & 0xf8) >> 3) | ((g & 0xf8) << 2) | ((b & 0xf8) << 7);

    if (!(ctx->valid[pos / 8] & pixel_masks_1[pos & 7]))
    {
        ctx->valid[pos / 8] |= pixel_masks_1[pos & 7];
        ctx->map[pos] = rgb_to_pixel_colortable(ctx->dib, (r & 0xf8) | 4, (g & 0xf8) | 4, (b & 0xf8) | 4);
    }
    return ctx->map[pos];
}

static void convert_to_8(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
{
    BYTE *dst_start = get_pixel_ptr_8(dst, 0, 0), *dst_pixel;
    INT x, y, pad_size = ((dst->width + 3) & ~3) - (src_rect->right - src_rect->left);
    struct rgb_lookup_colortable_ctx lookup_ctx;
    DWORD src_val;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;

        rgb_lookup_colortable_init(dst, &lookup_ctx);
        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx, src_val >> 16, src_val >> 8, src_val );
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
                    *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx,
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
                    *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx,
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

        rgb_lookup_colortable_init(dst, &lookup_ctx);
        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++, src_pixel += 3)
            {
                *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx, src_pixel[2], src_pixel[1], src_pixel[0] );
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
        rgb_lookup_colortable_init(dst, &lookup_ctx);
        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx,
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
                    *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx,
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
                    *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx,
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
                    *dst_pixel++ = rgb_lookup_colortable(&lookup_ctx,
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
            if (src->stride > 0 && src->stride == dst->stride && !pad_size)
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
            const RGBQUAD *color_table = get_dib_color_table( src );
            BYTE dst_colors[256];
            int i;

            for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
                dst_colors[i] = rgbquad_to_pixel_colortable(dst, color_table[i]);

            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++)
                    *dst_pixel++ = dst_colors[*src_pixel++];

                if(pad_size) memset(dst_pixel, 0, pad_size);
                dst_start += dst->stride;
                src_start += src->stride;
            }
        }
        break;
    }

    case 4:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        BYTE dst_colors[16];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_colortable(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 1;
            src_pixel = src_start;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                if (pos & 1)
                    dst_start[x] = dst_colors[*src_pixel++ & 0xf];
                else
                    dst_start[x] = dst_colors[*src_pixel >> 4];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }

    case 1:
    {
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        BYTE dst_colors[2];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = FILTER_DIBINDEX(color_table[i], rgbquad_to_pixel_colortable(dst, color_table[i]));

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            for (x = 0; x < src_rect->right - src_rect->left; x++, pos++)
            {
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                dst_start[x] = dst_colors[src_val];
            }
            if(pad_size) memset(dst_start + x, 0, pad_size);
            dst_start += dst->stride;
            src_start += src->stride;
        }
        break;
    }
    }
}

static void convert_to_4(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        BYTE dst_colors[256];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_colortable(dst, color_table[i]);

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left; x < src_rect->right; x++)
            {
                dst_val = dst_colors[*src_pixel++];
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

        if(color_tables_match(dst, src) && ((src->rect.left + src_rect->left) & 1) == 0)
        {
            if (src->stride > 0 && src->stride == dst->stride && !pad_size)
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
            const RGBQUAD *color_table = get_dib_color_table( src );
            BYTE dst_colors[16];
            int i;

            for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
                dst_colors[i] = rgbquad_to_pixel_colortable(dst, color_table[i]);

            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                int pos = (src->rect.left + src_rect->left) & 1;
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left; x < src_rect->right; x++, pos++)
                {
                    if(pos & 1)
                        dst_val = dst_colors[*src_pixel++ & 0xf];
                    else
                        dst_val = dst_colors[*src_pixel >> 4];
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        BYTE dst_colors[2];
        int i;

        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = FILTER_DIBINDEX(color_table[i], rgbquad_to_pixel_colortable(dst, color_table[i]));

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            dst_pixel = dst_start;
            for(x = src_rect->left; x < src_rect->right; x++, pos++)
            {
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                dst_val = dst_colors[src_val];
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

static void convert_to_1(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
{
    BYTE *dst_start = get_pixel_ptr_1(dst, 0, 0), *dst_pixel, dst_val;
    INT x, y, pad_size = ((dst->width + 31) & ~31) / 8 - (src_rect->right - src_rect->left + 7) / 8;
    RGBQUAD rgb, bg_entry = *get_dib_color_table( dst ); /* entry 0 is the background color */
    DWORD src_val;
    int bit_pos;

    switch(src->bit_count)
    {
    case 32:
    {
        DWORD *src_start = get_pixel_ptr_32(src, src_rect->left, src_rect->top), *src_pixel;
        DWORD bg_pixel = FILTER_DIBINDEX(bg_entry, rgbquad_to_pixel_masks(src, bg_entry));

        if(src->funcs == &funcs_8888)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                src_val >> 16, src_val >> 8, src_val);
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
                    dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                src_val >> src->red_shift,
                                                src_val >> src->green_shift,
                                                src_val >> src->blue_shift);

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
                    dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                get_field(src_val, src->red_shift, src->red_len),
                                                get_field(src_val, src->green_shift, src->green_len),
                                                get_field(src_val, src->blue_shift, src->blue_len));

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
        DWORD bg_pixel = FILTER_DIBINDEX(bg_entry, RGB(bg_entry.rgbRed, bg_entry.rgbGreen, bg_entry.rgbBlue));

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++, src_pixel += 3)
            {
                dst_val = rgb_to_pixel_mono(dst, dither, x, y, RGB(src_pixel[2], src_pixel[1], src_pixel[0]),
                                            bg_pixel, src_pixel[2], src_pixel[1], src_pixel[0]);

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
        DWORD bg_pixel = FILTER_DIBINDEX(bg_entry, rgbquad_to_pixel_masks(src, bg_entry));

        if(src->funcs == &funcs_555)
        {
            for(y = src_rect->top; y < src_rect->bottom; y++)
            {
                dst_pixel = dst_start;
                src_pixel = src_start;
                for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
                {
                    src_val = *src_pixel++;
                    dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                ((src_val >> 7) & 0xf8) | ((src_val >> 12) & 0x07),
                                                ((src_val >> 2) & 0xf8) | ((src_val >>  7) & 0x07),
                                                ((src_val << 3) & 0xf8) | ((src_val >>  2) & 0x07));

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
                    dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                (((src_val >> src->green_shift) << 3) & 0xf8) |
                                                (((src_val >> src->green_shift) >> 2) & 0x07),
                                                (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                (((src_val >> src->blue_shift)  >> 2) & 0x07));
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
                    dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                (((src_val >> src->red_shift)   << 3) & 0xf8) |
                                                (((src_val >> src->red_shift)   >> 2) & 0x07),
                                                (((src_val >> src->green_shift) << 2) & 0xfc) |
                                                (((src_val >> src->green_shift) >> 4) & 0x03),
                                                (((src_val >> src->blue_shift)  << 3) & 0xf8) |
                                                (((src_val >> src->blue_shift)  >> 2) & 0x07));
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
                    dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                get_field(src_val, src->red_shift, src->red_len),
                                                get_field(src_val, src->green_shift, src->green_len),
                                                get_field(src_val, src->blue_shift, src->blue_len));
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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_8(src, src_rect->left, src_rect->top), *src_pixel;
        DWORD bg_pixel = FILTER_DIBINDEX(bg_entry, rgbquad_to_pixel_colortable(src, bg_entry));

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++)
            {
                BYTE src_val = *src_pixel++;
                rgb = color_table[src_val];
                dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                            rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);

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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_4(src, src_rect->left, src_rect->top), *src_pixel;
        DWORD bg_pixel = FILTER_DIBINDEX(bg_entry, rgbquad_to_pixel_colortable(src, bg_entry));

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 1;
            dst_pixel = dst_start;
            src_pixel = src_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++, pos++)
            {
                src_val = (pos & 1) ? *src_pixel++ & 0xf : *src_pixel >> 4;
                rgb = color_table[src_val];
                dst_val = rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                            rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);

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
        const RGBQUAD *color_table = get_dib_color_table( src );
        BYTE *src_start = get_pixel_ptr_1(src, src_rect->left, src_rect->top);
        DWORD bg_pixel = FILTER_DIBINDEX(bg_entry, rgbquad_to_pixel_colortable(src, bg_entry));

        for(y = src_rect->top; y < src_rect->bottom; y++)
        {
            int pos = (src->rect.left + src_rect->left) & 7;
            dst_pixel = dst_start;
            for(x = src_rect->left, bit_pos = 0; x < src_rect->right; x++, pos++)
            {
                src_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                rgb = color_table[src_val];
                dst_val = FILTER_DIBINDEX(rgb, rgb_to_pixel_mono(dst, dither, x, y, src_val, bg_pixel,
                                                                 rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
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

static void convert_to_null(dib_info *dst, const dib_info *src, const RECT *src_rect, BOOL dither)
{
}

static inline BYTE blend_color(BYTE dst, BYTE src, DWORD alpha)
{
    return (src * alpha + dst * (255 - alpha) + 127) / 255;
}

static inline DWORD blend_argb_constant_alpha( DWORD dst, DWORD src, DWORD alpha )
{
    return (blend_color( dst, src, alpha ) |
            blend_color( dst >> 8, src >> 8, alpha ) << 8 |
            blend_color( dst >> 16, src >> 16, alpha ) << 16 |
            blend_color( dst >> 24, src >> 24, alpha ) << 24);
}

static inline DWORD blend_argb_no_src_alpha( DWORD dst, DWORD src, DWORD alpha )
{
    return (blend_color( dst, src, alpha ) |
            blend_color( dst >> 8, src >> 8, alpha ) << 8 |
            blend_color( dst >> 16, src >> 16, alpha ) << 16 |
            blend_color( dst >> 24, 255, alpha ) << 24);
}

static inline DWORD blend_argb( DWORD dst, DWORD src )
{
    BYTE b = (BYTE)src;
    BYTE g = (BYTE)(src >> 8);
    BYTE r = (BYTE)(src >> 16);
    DWORD alpha  = (BYTE)(src >> 24);
    return ((b     + ((BYTE)dst         * (255 - alpha) + 127) / 255) |
            (g     + ((BYTE)(dst >> 8)  * (255 - alpha) + 127) / 255) << 8 |
            (r     + ((BYTE)(dst >> 16) * (255 - alpha) + 127) / 255) << 16 |
            (alpha + ((BYTE)(dst >> 24) * (255 - alpha) + 127) / 255) << 24);
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

static void blend_rects_8888(const dib_info *dst, int num, const RECT *rc,
                             const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    int i, x, y;

    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        DWORD *dst_ptr = get_pixel_ptr_32( dst, rc->left, rc->top );

        if (blend.AlphaFormat & AC_SRC_ALPHA)
        {
            if (blend.SourceConstantAlpha == 255)
                for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
                    for (x = 0; x < rc->right - rc->left; x++)
                        dst_ptr[x] = blend_argb( dst_ptr[x], src_ptr[x] );
            else
                for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
                    for (x = 0; x < rc->right - rc->left; x++)
                        dst_ptr[x] = blend_argb_alpha( dst_ptr[x], src_ptr[x], blend.SourceConstantAlpha );
        }
        else if (src->compression == BI_RGB)
            for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
                for (x = 0; x < rc->right - rc->left; x++)
                    dst_ptr[x] = blend_argb_constant_alpha( dst_ptr[x], src_ptr[x], blend.SourceConstantAlpha );
        else
            for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 4, src_ptr += src->stride / 4)
                for (x = 0; x < rc->right - rc->left; x++)
                    dst_ptr[x] = blend_argb_no_src_alpha( dst_ptr[x], src_ptr[x], blend.SourceConstantAlpha );
    }
}

static void blend_rects_32(const dib_info *dst, int num, const RECT *rc,
                           const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    int i, x, y;

    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        DWORD *dst_ptr = get_pixel_ptr_32( dst, rc->left, rc->top );

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
                    dst_ptr[x] = rgb_to_pixel_masks( dst, val >> 16, val >> 8, val );
                }
            }
        }
    }
}

static void blend_rects_24(const dib_info *dst, int num, const RECT *rc,
                           const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    int i, x, y;

    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        BYTE *dst_ptr = get_pixel_ptr_24( dst, rc->left, rc->top );

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
}

static void blend_rects_555(const dib_info *dst, int num, const RECT *rc,
                            const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    int i, x, y;

    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        WORD *dst_ptr = get_pixel_ptr_16( dst, rc->left, rc->top );

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
}

static void blend_rects_16(const dib_info *dst, int num, const RECT *rc,
                           const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    int i, x, y;

    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        WORD *dst_ptr = get_pixel_ptr_16( dst, rc->left, rc->top );

        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride / 2, src_ptr += src->stride / 4)
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                DWORD val = blend_rgb( get_field( dst_ptr[x], dst->red_shift, dst->red_len ),
                                       get_field( dst_ptr[x], dst->green_shift, dst->green_len ),
                                       get_field( dst_ptr[x], dst->blue_shift, dst->blue_len ),
                                       src_ptr[x], blend );
                dst_ptr[x] = rgb_to_pixel_masks( dst, val >> 16, val >> 8, val );
            }
        }
    }
}

static void blend_rects_8(const dib_info *dst, int num, const RECT *rc,
                          const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    const RGBQUAD *color_table = get_dib_color_table( dst );
    struct rgb_lookup_colortable_ctx lookup_ctx;
    int i, x, y;

    rgb_lookup_colortable_init( dst, &lookup_ctx );
    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        BYTE *dst_ptr = get_pixel_ptr_8( dst, rc->left, rc->top );

        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride, src_ptr += src->stride / 4)
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                RGBQUAD rgb = color_table[dst_ptr[x]];
                DWORD val = blend_rgb( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue, src_ptr[x], blend );
                dst_ptr[x] = rgb_lookup_colortable( &lookup_ctx, val >> 16, val >> 8, val );
            }
        }
    }
}

static void blend_rects_4(const dib_info *dst, int num, const RECT *rc,
                          const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    const RGBQUAD *color_table = get_dib_color_table( dst );
    struct rgb_lookup_colortable_ctx lookup_ctx;
    int i, j, x, y;

    rgb_lookup_colortable_init( dst, &lookup_ctx );
    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        BYTE *dst_ptr = get_pixel_ptr_4( dst, rc->left, rc->top );

        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride, src_ptr += src->stride / 4)
        {
            for (j = 0, x = (dst->rect.left + rc->left) & 1; j < rc->right - rc->left; j++, x++)
            {
                DWORD val = ((x & 1) ? dst_ptr[x / 2] : (dst_ptr[x / 2] >> 4)) & 0x0f;
                RGBQUAD rgb = color_table[val];
                val = blend_rgb( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue, src_ptr[j], blend );
                val = rgb_lookup_colortable( &lookup_ctx, val >> 16, val >> 8, val );
                if (x & 1)
                    dst_ptr[x / 2] = val | (dst_ptr[x / 2] & 0xf0);
                else
                    dst_ptr[x / 2] = (val << 4) | (dst_ptr[x / 2] & 0x0f);
            }
        }
    }
}

static void blend_rects_1(const dib_info *dst, int num, const RECT *rc,
                          const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
{
    const RGBQUAD *color_table = get_dib_color_table( dst );
    int i, j, x, y;

    for (i = 0; i < num; i++, rc++)
    {
        DWORD *src_ptr = get_pixel_ptr_32( src, rc->left + offset->x, rc->top + offset->y );
        BYTE *dst_ptr = get_pixel_ptr_1( dst, rc->left, rc->top );

        for (y = rc->top; y < rc->bottom; y++, dst_ptr += dst->stride, src_ptr += src->stride / 4)
        {
            for (j = 0, x = (dst->rect.left + rc->left) & 7; j < rc->right - rc->left; j++, x++)
            {
                DWORD val = (dst_ptr[x / 8] & pixel_masks_1[x % 8]) ? 1 : 0;
                RGBQUAD rgb = color_table[val];
                val = blend_rgb( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue, src_ptr[j], blend );
                val = rgb_to_pixel_colortable(dst, val >> 16, val >> 8, val) ? 0xff : 0;
                dst_ptr[x / 8] = (dst_ptr[x / 8] & ~pixel_masks_1[x % 8]) | (val & pixel_masks_1[x % 8]);
            }
        }
    }
}

static void blend_rects_null(const dib_info *dst, int num, const RECT *rc,
                             const dib_info *src, const POINT *offset, BLENDFUNCTION blend)
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
    int r = (v[0].Red   * (len - pos) + v[1].Red   * pos) / len / 128 + bayer_4x4[y % 4][x % 4];
    int g = (v[0].Green * (len - pos) + v[1].Green * pos) / len / 128 + bayer_4x4[y % 4][x % 4];
    int b = (v[0].Blue  * (len - pos) + v[1].Blue  * pos) / len / 128 + bayer_4x4[y % 4][x % 4];
    r = min( 31, max( 0, r / 16 ));
    g = min( 31, max( 0, g / 16 ));
    b = min( 31, max( 0, b / 16 ));
    return (r << 10) | (g << 5) | b;
}

static inline BYTE gradient_rgb_8( const dib_info *dib, const TRIVERTEX *v,
                                   unsigned int pos, unsigned int len, unsigned int x, unsigned int y )
{
    BYTE r = ((v[0].Red   * (len - pos) + v[1].Red   * pos) / len / 128 + bayer_16x16[y % 16][x % 16]) / 256;
    BYTE g = ((v[0].Green * (len - pos) + v[1].Green * pos) / len / 128 + bayer_16x16[y % 16][x % 16]) / 256;
    BYTE b = ((v[0].Blue  * (len - pos) + v[1].Blue  * pos) / len / 128 + bayer_16x16[y % 16][x % 16]) / 256;
    return rgb_to_pixel_colortable( dib, r * 127, g * 127, b * 127 );
}

/* compute the left/right triangle limit for row y */
static inline void triangle_coords( const TRIVERTEX *v, const RECT *rc, int y, int *left, int *right )
{
    int x1, x2;

    if (y < v[1].y) x1 = edge_coord( y, v[0].x, v[0].y, v[1].x, v[1].y );
    else x1 = edge_coord( y, v[1].x, v[1].y, v[2].x, v[2].y );

    x2 = edge_coord( y, v[0].x, v[0].y, v[2].x, v[2].y );

    *left  = max( rc->left, min( x1, x2 ) );
    *right = min( rc->right, max( x1, x2 ) );
}

/* compute the matrix determinant for triangular barycentric coordinates (constant across the triangle) */
static inline int triangle_det( const TRIVERTEX *v )
{
    return (v[2].y - v[1].y) * (v[2].x - v[0].x) - (v[2].x - v[1].x) * (v[2].y - v[0].y);
}

/* compute the barycentric weights for a given point inside the triangle */
static inline void triangle_weights( const TRIVERTEX *v, int x, int y, INT64 *l1, INT64 *l2 )
{
    *l1 = (v[1].y - v[2].y) * (x - v[2].x) - (v[1].x - v[2].x) * (y - v[2].y);
    *l2 = (v[2].y - v[0].y) * (x - v[2].x) - (v[2].x - v[0].x) * (y - v[2].y);
}

static inline DWORD gradient_triangle_8888( const TRIVERTEX *v, int x, int y, int det )
{
    INT64 l1, l2;
    BYTE r, g, b, a;

    triangle_weights( v, x, y, &l1, &l2 );
    r = (v[0].Red   * l1 + v[1].Red   * l2 + v[2].Red   * (det - l1 - l2)) / det / 256;
    g = (v[0].Green * l1 + v[1].Green * l2 + v[2].Green * (det - l1 - l2)) / det / 256;
    b = (v[0].Blue  * l1 + v[1].Blue  * l2 + v[2].Blue  * (det - l1 - l2)) / det / 256;
    a = (v[0].Alpha * l1 + v[1].Alpha * l2 + v[2].Alpha * (det - l1 - l2)) / det / 256;
    return a << 24 | r << 16 | g << 8 | b;
}

static inline DWORD gradient_triangle_24( const TRIVERTEX *v, int x, int y, int det )
{
    INT64 l1, l2;
    BYTE r, g, b;

    triangle_weights( v, x, y, &l1, &l2 );
    r = (v[0].Red   * l1 + v[1].Red   * l2 + v[2].Red   * (det - l1 - l2)) / det / 256;
    g = (v[0].Green * l1 + v[1].Green * l2 + v[2].Green * (det - l1 - l2)) / det / 256;
    b = (v[0].Blue  * l1 + v[1].Blue  * l2 + v[2].Blue  * (det - l1 - l2)) / det / 256;
    return r << 16 | g << 8 | b;
}

static inline DWORD gradient_triangle_555( const TRIVERTEX *v, int x, int y, int det )
{
    INT64 l1, l2;
    int r, g, b;

    triangle_weights( v, x, y, &l1, &l2 );
    r = (v[0].Red   * l1 + v[1].Red   * l2 + v[2].Red   * (det - l1 - l2)) / det / 128 + bayer_4x4[y % 4][x % 4];
    g = (v[0].Green * l1 + v[1].Green * l2 + v[2].Green * (det - l1 - l2)) / det / 128 + bayer_4x4[y % 4][x % 4];
    b = (v[0].Blue  * l1 + v[1].Blue  * l2 + v[2].Blue  * (det - l1 - l2)) / det / 128 + bayer_4x4[y % 4][x % 4];
    r = min( 31, max( 0, r / 16 ));
    g = min( 31, max( 0, g / 16 ));
    b = min( 31, max( 0, b / 16 ));
    return (r << 10) | (g << 5) | b;
}

static inline DWORD gradient_triangle_8( const dib_info *dib, const TRIVERTEX *v, int x, int y, int det )
{
    INT64 l1, l2;
    BYTE r, g, b;

    triangle_weights( v, x, y, &l1, &l2 );
    r = ((v[0].Red   * l1 + v[1].Red   * l2 + v[2].Red   * (det - l1 - l2)) / det / 128 + bayer_16x16[y % 16][x % 16]) / 256;
    g = ((v[0].Green * l1 + v[1].Green * l2 + v[2].Green * (det - l1 - l2)) / det / 128 + bayer_16x16[y % 16][x % 16]) / 256;
    b = ((v[0].Blue  * l1 + v[1].Blue  * l2 + v[2].Blue  * (det - l1 - l2)) / det / 128 + bayer_16x16[y % 16][x % 16]) / 256;
    return rgb_to_pixel_colortable( dib, r * 127, g * 127, b * 127 );
}

static BOOL gradient_rect_8888( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    DWORD *ptr = get_pixel_ptr_32( dib, rc->left, rc->top );
    int x, y, left, right, det;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (x = 0; x < rc->right - rc->left; x++)
            ptr[x] = gradient_rgb_8888( v, rc->left + x - v[0].x, v[1].x - v[0].x );

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride / 4)
            memcpy( ptr + dib->stride / 4, ptr, (rc->right - rc->left) * 4 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 4)
        {
            DWORD val = gradient_rgb_8888( v, y - v[0].y, v[1].y - v[0].y );
            memset_32( ptr, val, rc->right - rc->left );
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 4)
        {
            triangle_coords( v, rc, y, &left, &right );
            for (x = left; x < right; x++) ptr[x - rc->left] = gradient_triangle_8888( v, x, y, det );
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_32( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    DWORD *ptr = get_pixel_ptr_32( dib, rc->left, rc->top );
    int x, y, left, right, det;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        if (dib->red_len == 8 && dib->green_len == 8 && dib->blue_len == 8)
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                DWORD val = gradient_rgb_24( v, rc->left + x - v[0].x, v[1].x - v[0].x );
                ptr[x] = ((( val        & 0xff) << dib->blue_shift) |
                          (((val >> 8)  & 0xff) << dib->green_shift) |
                          (((val >> 16) & 0xff) << dib->red_shift));
            }
        }
        else
        {
            for (x = 0; x < rc->right - rc->left; x++)
            {
                DWORD val = gradient_rgb_24( v, rc->left + x - v[0].x, v[1].x - v[0].x );
                ptr[x] = rgb_to_pixel_masks( dib, val >> 16, val >> 8, val );
            }
        }

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride / 4)
            memcpy( ptr + dib->stride / 4, ptr, (rc->right - rc->left) * 4 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 4)
        {
            DWORD val = gradient_rgb_24( v, y - v[0].y, v[1].y - v[0].y );
            if (dib->red_len == 8 && dib->green_len == 8 && dib->blue_len == 8)
                val = ((( val        & 0xff) << dib->blue_shift) |
                       (((val >> 8)  & 0xff) << dib->green_shift) |
                       (((val >> 16) & 0xff) << dib->red_shift));
            else
                val = rgb_to_pixel_masks( dib, val >> 16, val >> 8, val );

            memset_32( ptr, val, rc->right - rc->left );
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 4)
        {
            triangle_coords( v, rc, y, &left, &right );

            if (dib->red_len == 8 && dib->green_len == 8 && dib->blue_len == 8)
                for (x = left; x < right; x++)
                {
                    DWORD val = gradient_triangle_24( v, x, y, det );
                    ptr[x - rc->left] = ((( val        & 0xff) << dib->blue_shift) |
                                         (((val >> 8)  & 0xff) << dib->green_shift) |
                                         (((val >> 16) & 0xff) << dib->red_shift));
                }
            else
                for (x = left; x < right; x++)
                {
                    DWORD val = gradient_triangle_24( v, x, y, det );
                    ptr[x - rc->left] = rgb_to_pixel_masks( dib, val >> 16, val >> 8, val );
                }
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_24( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_24( dib, rc->left, rc->top );
    int x, y, left, right, det;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (x = 0; x < rc->right - rc->left; x++)
        {
            DWORD val = gradient_rgb_24( v, rc->left + x - v[0].x, v[1].x - v[0].x );
            ptr[x * 3]     = val;
            ptr[x * 3 + 1] = val >> 8;
            ptr[x * 3 + 2] = val >> 16;
        }

        for (y = rc->top + 1; y < rc->bottom; y++, ptr += dib->stride)
            memcpy( ptr + dib->stride, ptr, (rc->right - rc->left) * 3 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            DWORD val = gradient_rgb_24( v, y - v[0].y, v[1].y - v[0].y );
            for (x = 0; x < rc->right - rc->left; x++)
            {
                ptr[x * 3]     = val;
                ptr[x * 3 + 1] = val >> 8;
                ptr[x * 3 + 2] = val >> 16;
            }
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            triangle_coords( v, rc, y, &left, &right );
            for (x = left; x < right; x++)
            {
                DWORD val = gradient_triangle_24( v, x, y, det );
                ptr[(x - rc->left) * 3]     = val;
                ptr[(x - rc->left) * 3 + 1] = val >> 8;
                ptr[(x - rc->left) * 3 + 2] = val >> 16;
            }
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_555( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    WORD *ptr = get_pixel_ptr_16( dib, rc->left, rc->top );
    int x, y, left, right, det;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (y = rc->top; y < min( rc->top + 4, rc->bottom ); y++, ptr += dib->stride / 2)
            for (x = rc->left; x < rc->right; x++)
                ptr[x - rc->left] = gradient_rgb_555( v, x - v[0].x, v[1].x - v[0].x, x, y );
        for ( ; y < rc->bottom; y++, ptr += dib->stride / 2)
            memcpy( ptr, ptr - dib->stride * 2, (rc->right - rc->left) * 2 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 2)
        {
            WORD values[4];
            for (x = 0; x < 4; x++) values[x] = gradient_rgb_555( v, y - v[0].y, v[1].y - v[0].y, x, y );
            for (x = rc->left; x < rc->right; x++) ptr[x - rc->left] = values[x % 4];
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 2)
        {
            triangle_coords( v, rc, y, &left, &right );
            for (x = left; x < right; x++) ptr[x - rc->left] = gradient_triangle_555( v, x, y, det );
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_16( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    WORD *ptr = get_pixel_ptr_16( dib, rc->left, rc->top );
    int x, y, left, right, det;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (y = rc->top; y < min( rc->top + 4, rc->bottom ); y++, ptr += dib->stride / 2)
            for (x = rc->left; x < rc->right; x++)
            {
                WORD val = gradient_rgb_555( v, x - v[0].x, v[1].x - v[0].x, x, y );
                ptr[x - rc->left] = rgb_to_pixel_masks( dib,
                                                        ((val >> 7) & 0xf8) | ((val >> 12) & 0x07),
                                                        ((val >> 2) & 0xf8) | ((val >> 7)  & 0x07),
                                                        ((val << 3) & 0xf8) | ((val >> 2)  & 0x07) );
            }
        for ( ; y < rc->bottom; y++, ptr += dib->stride / 2)
            memcpy( ptr, ptr - dib->stride * 2, (rc->right - rc->left) * 2 );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 2)
        {
            WORD values[4];
            for (x = 0; x < 4; x++)
            {
                WORD val = gradient_rgb_555( v, y - v[0].y, v[1].y - v[0].y, x, y );
                values[x] = rgb_to_pixel_masks( dib,
                                                ((val >> 7) & 0xf8) | ((val >> 12) & 0x07),
                                                ((val >> 2) & 0xf8) | ((val >> 7)  & 0x07),
                                                ((val << 3) & 0xf8) | ((val >> 2)  & 0x07) );
            }
            for (x = rc->left; x < rc->right; x++) ptr[x - rc->left] = values[x % 4];
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride / 2)
        {
            triangle_coords( v, rc, y, &left, &right );
            for (x = left; x < right; x++)
            {
                WORD val = gradient_triangle_555( v, x, y, det );
                ptr[x - rc->left] = rgb_to_pixel_masks( dib,
                                                        ((val >> 7) & 0xf8) | ((val >> 12) & 0x07),
                                                        ((val >> 2) & 0xf8) | ((val >> 7)  & 0x07),
                                                        ((val << 3) & 0xf8) | ((val >> 2)  & 0x07) );
            }
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_8( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_8( dib, rc->left, rc->top );
    int x, y, left, right, det;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (y = rc->top; y < min( rc->top + 16, rc->bottom ); y++, ptr += dib->stride)
            for (x = rc->left; x < rc->right; x++)
                ptr[x - rc->left] = gradient_rgb_8( dib, v, x - v[0].x, v[1].x - v[0].x, x, y );
        for ( ; y < rc->bottom; y++, ptr += dib->stride)
            memcpy( ptr, ptr - dib->stride * 16, rc->right - rc->left );
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            BYTE values[16];
            for (x = 0; x < 16; x++)
                values[x] = gradient_rgb_8( dib, v, y - v[0].y, v[1].y - v[0].y, x, y );
            for (x = rc->left; x < rc->right; x++) ptr[x - rc->left] = values[x % 16];
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            triangle_coords( v, rc, y, &left, &right );
            for (x = left; x < right; x++) ptr[x - rc->left] = gradient_triangle_8( dib, v, x, y, det );
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_4( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_4( dib, rc->left, rc->top );
    int x, y, left, right, det, pos;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (y = rc->top; y < min( rc->top + 16, rc->bottom ); y++, ptr += dib->stride)
        {
            for (x = rc->left, pos = (dib->rect.left + rc->left) & 1; x < rc->right; x++, pos++)
            {
                BYTE val = gradient_rgb_8( dib, v, x - v[0].x, v[1].x - v[0].x, x, y );
                if (pos & 1)
                    ptr[pos / 2] = val | (ptr[pos / 2] & 0xf0);
                else
                    ptr[pos / 2] = (val << 4) | (ptr[pos / 2] & 0x0f);
            }
        }
        for ( ; y < rc->bottom; y++, ptr += dib->stride)
        {
            x = rc->left;
            pos = (dib->rect.left + rc->left) & 1;
            if (pos)
            {
                ptr[0] = (ptr[-16 * dib->stride] & 0x0f) | (ptr[0] & 0xf0);
                pos++;
                x++;
            }
            for (; x < rc->right - 1; x += 2, pos += 2) ptr[pos / 2] = ptr[pos / 2 - 16 * dib->stride];
            if (x < rc->right)
                ptr[pos / 2] = (ptr[pos / 2] & 0x0f) | (ptr[pos / 2 - 16 * dib->stride] & 0xf0);
        }
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            BYTE values[16];
            for (x = 0; x < 16; x++)
                values[x] = gradient_rgb_8( dib, v, y - v[0].y, v[1].y - v[0].y, x, y );
            for (x = rc->left, pos = (dib->rect.left + rc->left) & 1; x < rc->right; x++, pos++)
                if (pos & 1)
                    ptr[pos / 2] = values[x % 16] | (ptr[pos / 2] & 0xf0);
                else
                    ptr[pos / 2] = (values[x % 16] << 4) | (ptr[pos / 2] & 0x0f);
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            triangle_coords( v, rc, y, &left, &right );
            for (x = left, pos = left - rc->left + ((dib->rect.left + rc->left) & 1); x < right; x++, pos++)
            {
                BYTE val = gradient_triangle_8( dib, v, x, y, det );
                if (pos & 1)
                    ptr[pos / 2] = val | (ptr[pos / 2] & 0xf0);
                else
                    ptr[pos / 2] = (val << 4) | (ptr[pos / 2] & 0x0f);
            }
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_1( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    BYTE *ptr = get_pixel_ptr_1( dib, rc->left, rc->top );
    int x, y, left, right, det, pos;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
        for (y = rc->top; y < min( rc->top + 16, rc->bottom ); y++, ptr += dib->stride)
        {
            for (x = rc->left, pos = (dib->rect.left + rc->left) & 7; x < rc->right; x++, pos++)
            {
                BYTE val = gradient_rgb_8( dib, v, x - v[0].x, v[1].x - v[0].x, x, y ) ? 0xff : 0;
                ptr[pos / 8] = (ptr[pos / 8] & ~pixel_masks_1[pos % 8]) | (val & pixel_masks_1[pos % 8]);
            }
        }
        for ( ; y < rc->bottom; y++, ptr += dib->stride)
            for (x = rc->left, pos = (dib->rect.left + rc->left) & 7; x < rc->right; x++, pos++)
                ptr[pos / 8] = (ptr[pos / 8] & ~pixel_masks_1[pos % 8]) |
                               (ptr[pos / 8 - 16 * dib->stride] & pixel_masks_1[pos % 8]);
        break;

    case GRADIENT_FILL_RECT_V:
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            BYTE values[16];
            for (x = 0; x < 16; x++)
                values[x] = gradient_rgb_8( dib, v, y - v[0].y, v[1].y - v[0].y, x, y ) ? 0xff : 0;
            for (x = rc->left, pos = (dib->rect.left + rc->left) & 7; x < rc->right; x++, pos++)
                ptr[pos / 8] = (ptr[pos / 8] & ~pixel_masks_1[pos % 8]) |
                               (values[x % 16] & pixel_masks_1[pos % 8]);
        }
        break;

    case GRADIENT_FILL_TRIANGLE:
        if (!(det = triangle_det( v ))) return FALSE;
        for (y = rc->top; y < rc->bottom; y++, ptr += dib->stride)
        {
            triangle_coords( v, rc, y, &left, &right );
            for (x = left, pos = left - rc->left + ((dib->rect.left + rc->left) & 7); x < right; x++, pos++)
            {
                BYTE val = gradient_triangle_8( dib, v, x, y, det ) ? 0xff : 0;
                ptr[pos / 8] = (ptr[pos / 8] & ~pixel_masks_1[pos % 8]) | (val & pixel_masks_1[pos % 8]);
            }
        }
        break;
    }
    return TRUE;
}

static BOOL gradient_rect_null( const dib_info *dib, const RECT *rc, const TRIVERTEX *v, int mode )
{
    return TRUE;
}

static void mask_rect_32( const dib_info *dst, const RECT *rc,
                          const dib_info *src, const POINT *origin, int rop2 )
{
    DWORD *dst_start = get_pixel_ptr_32(dst, rc->left, rc->top), dst_colors[256];
    DWORD src_val, bit_val, i, full, pos;
    int x, y, origin_end = origin->x + rc->right - rc->left;
    const RGBQUAD *color_table = get_dib_color_table( src );
    BYTE *src_start = get_pixel_ptr_1(src, origin->x, origin->y);

    if (dst->funcs == &funcs_8888)
        for (i = 0; i < 2; i++)
            dst_colors[i] = color_table[i].rgbRed << 16 | color_table[i].rgbGreen << 8 |
                color_table[i].rgbBlue;
    else
        for (i = 0; i < 2; i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

    /* Creating a BYTE-sized table so we don't need to mask the lsb of bit_val */
    for (i = 2; i < ARRAY_SIZE(dst_colors); i++)
        dst_colors[i] = dst_colors[i & 1];

    /* Special case starting and finishing in same byte, neither on byte boundary */
    if ((origin->x & 7) && (origin_end & 7) && (origin->x & ~7) == (origin_end & ~7))
    {
        struct rop_codes codes;

        get_rop_codes( rop2, &codes );

        for (y = rc->top; y < rc->bottom; y++)
        {
            pos = origin->x & 7;
            for (x = 0; x < rc->right - rc->left; x++, pos++)
            {
                bit_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                do_rop_codes_32( dst_start + x, dst_colors[bit_val], &codes );
            }
            dst_start += dst->stride / 4;
            src_start += src->stride;
        }
        return;
    }

    full = ((rc->right - rc->left) - ((8 - (origin->x & 7)) & 7)) / 8;

#define LOOP( op )                                                      \
    for (y = rc->top; y < rc->bottom; y++)                              \
    {                                                                   \
        pos = origin->x & 7;                                            \
        src_val = src_start[pos / 8];                                   \
        x = 0;                                                          \
        switch (pos & 7)                                                \
        {                                                               \
        case 1: bit_val = src_val >> 6; op; x++;                        \
            /* fall through */                                          \
        case 2: bit_val = src_val >> 5; op; x++;                        \
            /* fall through */                                          \
        case 3: bit_val = src_val >> 4; op; x++;                        \
            /* fall through */                                          \
        case 4: bit_val = src_val >> 3; op; x++;                        \
            /* fall through */                                          \
        case 5: bit_val = src_val >> 2; op; x++;                        \
            /* fall through */                                          \
        case 6: bit_val = src_val >> 1; op; x++;                        \
            /* fall through */                                          \
        case 7: bit_val = src_val; op; x++;                             \
            pos = (pos + 7) & ~7;                                       \
        }                                                               \
        for (i = 0; i < full; i++, pos += 8)                            \
        {                                                               \
            src_val = src_start[pos / 8];                               \
            bit_val = src_val >> 7; op; x++;                            \
            bit_val = src_val >> 6; op; x++;                            \
            bit_val = src_val >> 5; op; x++;                            \
            bit_val = src_val >> 4; op; x++;                            \
            bit_val = src_val >> 3; op; x++;                            \
            bit_val = src_val >> 2; op; x++;                            \
            bit_val = src_val >> 1; op; x++;                            \
            bit_val = src_val; op; x++;                                 \
        }                                                               \
        if (origin_end & 7)                                             \
        {                                                               \
            src_val = src_start[pos / 8];                               \
            x += (origin_end & 7) - 1;                                  \
            switch (origin_end & 7)                                     \
            {                                                           \
            case 7: bit_val = src_val >> 1; op; x--;                    \
                /* fall through */                                      \
            case 6: bit_val = src_val >> 2; op; x--;                    \
                /* fall through */                                      \
            case 5: bit_val = src_val >> 3; op; x--;                    \
                /* fall through */                                      \
            case 4: bit_val = src_val >> 4; op; x--;                    \
                /* fall through */                                      \
            case 3: bit_val = src_val >> 5; op; x--;                    \
                /* fall through */                                      \
            case 2: bit_val = src_val >> 6; op; x--;                    \
                /* fall through */                                      \
            case 1: bit_val = src_val >> 7; op;                         \
            }                                                           \
        }                                                               \
        dst_start += dst->stride / 4;                                   \
        src_start += src->stride;                                       \
    }

    switch (rop2)
    {
        ROPS_ALL( dst_start[x], dst_colors[bit_val] )
    }
#undef LOOP
}

static void mask_rect_24( const dib_info *dst, const RECT *rc,
                          const dib_info *src, const POINT *origin, int rop2 )
{
    BYTE *dst_start = get_pixel_ptr_24(dst, rc->left, rc->top);
    DWORD src_val, bit_val, i, full, pos;
    struct rop_codes codes;
    int x, y, origin_end = origin->x + rc->right - rc->left;
    const RGBQUAD *color_table = get_dib_color_table( src );
    BYTE *src_start = get_pixel_ptr_1(src, origin->x, origin->y);
    RGBQUAD rgb;

    get_rop_codes( rop2, &codes );

    /* Special case starting and finishing in same byte, neither on byte boundary */
    if ((origin->x & 7) && (origin_end & 7) && (origin->x & ~7) == (origin_end & ~7))
    {
        for (y = rc->top; y < rc->bottom; y++)
        {
            pos = origin->x & 7;
            for (x = 0; x < rc->right - rc->left; x++, pos++)
            {
                bit_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        return;
    }

    full = ((rc->right - rc->left) - ((8 - (origin->x & 7)) & 7)) / 8;

    for (y = rc->top; y < rc->bottom; y++)
    {
        pos = origin->x & 7;
        src_val = src_start[pos / 8];
        x = 0;

        switch (pos & 7)
        {
        case 1:
            bit_val = (src_val >> 6) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
            /* fall through */
        case 2:
            bit_val = (src_val >> 5) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
            /* fall through */
        case 3:
            bit_val = (src_val >> 4) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
            /* fall through */
        case 4:
            bit_val = (src_val >> 3) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
            /* fall through */
        case 5:
            bit_val = (src_val >> 2) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
            /* fall through */
        case 6:
            bit_val = (src_val >> 1) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
            /* fall through */
        case 7:
            bit_val = src_val & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
            pos = (pos + 7) & ~7;
        }

        for (i = 0; i < full; i++, pos += 8)
        {
            src_val = src_start[pos / 8];

            bit_val = (src_val >> 7) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;

            bit_val = (src_val >> 6) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;

            bit_val = (src_val >> 5) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;

            bit_val = (src_val >> 4) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;

            bit_val = (src_val >> 3) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;

            bit_val = (src_val >> 2) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;

            bit_val = (src_val >> 1) & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;

            bit_val = src_val & 1;
            rgb = color_table[bit_val];
            do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
            do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
            do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            x++;
        }

        if (origin_end & 7)
        {
            src_val = src_start[pos / 8];
            x += (origin_end & 7) - 1;

            switch (origin_end & 7)
            {
            case 7:
                bit_val = (src_val >> 1) & 1;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
                x--;
                /* fall through */
            case 6:
                bit_val = (src_val >> 2) & 1;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
                x--;
                /* fall through */
            case 5:
                bit_val = (src_val >> 3) & 1;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
                x--;
                /* fall through */
            case 4:
                bit_val = (src_val >> 4) & 1;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
                x--;
                /* fall through */
            case 3:
                bit_val = (src_val >> 5) & 1;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
                x--;
                /* fall through */
            case 2:
                bit_val = (src_val >> 6) & 1;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
                x--;
                /* fall through */
            case 1:
                bit_val = (src_val >> 7) & 1;
                rgb = color_table[bit_val];
                do_rop_codes_8( dst_start + x * 3, rgb.rgbBlue, &codes );
                do_rop_codes_8( dst_start + x * 3 + 1, rgb.rgbGreen, &codes );
                do_rop_codes_8( dst_start + x * 3 + 2, rgb.rgbRed, &codes );
            }
        }

        dst_start += dst->stride;
        src_start += src->stride;
    }
}

static void mask_rect_16( const dib_info *dst, const RECT *rc,
                          const dib_info *src, const POINT *origin, int rop2 )
{
    WORD *dst_start = get_pixel_ptr_16(dst, rc->left, rc->top), dst_colors[2];
    DWORD src_val, bit_val, i, full, pos;
    struct rop_codes codes;
    int x, y, origin_end = origin->x + rc->right - rc->left;
    const RGBQUAD *color_table = get_dib_color_table( src );
    BYTE *src_start = get_pixel_ptr_1(src, origin->x, origin->y);

    get_rop_codes( rop2, &codes );

    if (dst->funcs == &funcs_555)
        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = ((color_table[i].rgbRed   << 7) & 0x7c00) |
                            ((color_table[i].rgbGreen << 2) & 0x03e0) |
                            ((color_table[i].rgbBlue  >> 3) & 0x001f);
    else
        for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
            dst_colors[i] = rgbquad_to_pixel_masks(dst, color_table[i]);

    /* Special case starting and finishing in same byte, neither on byte boundary */
    if ((origin->x & 7) && (origin_end & 7) && (origin->x & ~7) == (origin_end & ~7))
    {
        for (y = rc->top; y < rc->bottom; y++)
        {
            pos = origin->x & 7;
            for (x = 0; x < rc->right - rc->left; x++, pos++)
            {
                bit_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                do_rop_codes_16( dst_start + x, dst_colors[bit_val], &codes );
            }
            dst_start += dst->stride / 2;
            src_start += src->stride;
        }
        return;
    }

    full = ((rc->right - rc->left) - ((8 - (origin->x & 7)) & 7)) / 8;

    for (y = rc->top; y < rc->bottom; y++)
    {
        pos = origin->x & 7;
        src_val = src_start[pos / 8];
        x = 0;

        switch (pos & 7)
        {
        case 1:
            bit_val = (src_val >> 6) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 2:
            bit_val = (src_val >> 5) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 3:
            bit_val = (src_val >> 4) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 4:
            bit_val = (src_val >> 3) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 5:
            bit_val = (src_val >> 2) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 6:
            bit_val = (src_val >> 1) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 7:
            bit_val = src_val & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            pos = (pos + 7) & ~7;
        }

        for (i = 0; i < full; i++, pos += 8)
        {
            src_val = src_start[pos / 8];

            bit_val = (src_val >> 7) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 6) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 5) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 4) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 3) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 2) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 1) & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = src_val & 1;
            do_rop_codes_16( dst_start + x++, dst_colors[bit_val], &codes );
        }

        if (origin_end & 7)
        {
            src_val = src_start[pos / 8];
            x += (origin_end & 7) - 1;

            switch (origin_end & 7)
            {
            case 7:
                bit_val = (src_val >> 1) & 1;
                do_rop_codes_16( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 6:
                bit_val = (src_val >> 2) & 1;
                do_rop_codes_16( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 5:
                bit_val = (src_val >> 3) & 1;
                do_rop_codes_16( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 4:
                bit_val = (src_val >> 4) & 1;
                do_rop_codes_16( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 3:
                bit_val = (src_val >> 5) & 1;
                do_rop_codes_16( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 2:
                bit_val = (src_val >> 6) & 1;
                do_rop_codes_16( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 1:
                bit_val = (src_val >> 7) & 1;
                do_rop_codes_16( dst_start + x, dst_colors[bit_val], &codes );
            }
        }

        dst_start += dst->stride / 2;
        src_start += src->stride;
    }
}

static void mask_rect_8( const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, int rop2 )
{
    BYTE *dst_start = get_pixel_ptr_8(dst, rc->left, rc->top), dst_colors[2];
    DWORD src_val, bit_val, i, full, pos;
    struct rop_codes codes;
    int x, y, origin_end = origin->x + rc->right - rc->left;
    const RGBQUAD *color_table = get_dib_color_table( src );
    BYTE *src_start = get_pixel_ptr_1(src, origin->x, origin->y);

    get_rop_codes( rop2, &codes );

    for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
        dst_colors[i] = FILTER_DIBINDEX(color_table[i], rgbquad_to_pixel_colortable(dst, color_table[i]));

    /* Special case starting and finishing in same byte, neither on byte boundary */
    if ((origin->x & 7) && (origin_end & 7) && (origin->x & ~7) == (origin_end & ~7))
    {
        for (y = rc->top; y < rc->bottom; y++)
        {
            pos = origin->x & 7;
            for (x = 0; x < rc->right - rc->left; x++, pos++)
            {
                bit_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
                do_rop_codes_8( dst_start + x, dst_colors[bit_val], &codes );
            }
            dst_start += dst->stride;
            src_start += src->stride;
        }
        return;
    }

    full = ((rc->right - rc->left) - ((8 - (origin->x & 7)) & 7)) / 8;

    for (y = rc->top; y < rc->bottom; y++)
    {
        pos = origin->x & 7;
        src_val = src_start[pos / 8];
        x = 0;

        switch (pos & 7)
        {
        case 1:
            bit_val = (src_val >> 6) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 2:
            bit_val = (src_val >> 5) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 3:
            bit_val = (src_val >> 4) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 4:
            bit_val = (src_val >> 3) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 5:
            bit_val = (src_val >> 2) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 6:
            bit_val = (src_val >> 1) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            /* fall through */
        case 7:
            bit_val = src_val & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            pos = (pos + 7) & ~7;
        }

        for (i = 0; i < full; i++, pos += 8)
        {
            src_val = src_start[pos / 8];

            bit_val = (src_val >> 7) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 6) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 5) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 4) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 3) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 2) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = (src_val >> 1) & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
            bit_val = src_val & 1;
            do_rop_codes_8( dst_start + x++, dst_colors[bit_val], &codes );
        }

        if (origin_end & 7)
        {
            src_val = src_start[pos / 8];
            x += (origin_end & 7) - 1;

            switch (origin_end & 7)
            {
            case 7:
                bit_val = (src_val >> 1) & 1;
                do_rop_codes_8( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 6:
                bit_val = (src_val >> 2) & 1;
                do_rop_codes_8( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 5:
                bit_val = (src_val >> 3) & 1;
                do_rop_codes_8( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 4:
                bit_val = (src_val >> 4) & 1;
                do_rop_codes_8( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 3:
                bit_val = (src_val >> 5) & 1;
                do_rop_codes_8( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 2:
                bit_val = (src_val >> 6) & 1;
                do_rop_codes_8( dst_start + x--, dst_colors[bit_val], &codes );
                /* fall through */
            case 1:
                bit_val = (src_val >> 7) & 1;
                do_rop_codes_8( dst_start + x, dst_colors[bit_val], &codes );
            }
        }

        dst_start += dst->stride;
        src_start += src->stride;
    }
}

static void mask_rect_4( const dib_info *dst, const RECT *rc,
                         const dib_info *src, const POINT *origin, int rop2 )
{
    BYTE *dst_start = get_pixel_ptr_4(dst, rc->left, rc->top), dst_colors[2], *dst_ptr;
    DWORD bit_val, i, pos;
    struct rop_codes codes;
    int x, y;
    int left = dst->rect.left + rc->left;
    int right = dst->rect.left + rc->right;
    const RGBQUAD *color_table = get_dib_color_table( src );
    BYTE *src_start = get_pixel_ptr_1(src, origin->x, origin->y);

    get_rop_codes( rop2, &codes );

    for (i = 0; i < ARRAY_SIZE(dst_colors); i++)
    {
        dst_colors[i] = FILTER_DIBINDEX(color_table[i],rgbquad_to_pixel_colortable(dst, color_table[i]));
        /* Set high nibble to match so we don't need to shift it later. */
        dst_colors[i] |= dst_colors[i] << 4;
    }

    for (y = rc->top; y < rc->bottom; y++)
    {
        pos = origin->x & 7;

        for (x = left, dst_ptr = dst_start; x < right; x++, pos++)
        {
            bit_val = (src_start[pos / 8] & pixel_masks_1[pos % 8]) ? 1 : 0;
            if (x & 1)
                do_rop_codes_mask_8( dst_ptr++, dst_colors[bit_val], &codes, 0x0f );
            else
                do_rop_codes_mask_8( dst_ptr, dst_colors[bit_val], &codes, 0xf0 );
        }
        dst_start += dst->stride;
        src_start += src->stride;
    }
}

static void mask_rect_null( const dib_info *dst, const RECT *rc,
                            const dib_info *src, const POINT *origin, int rop2 )
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
            dst_ptr[x] = rgb_to_pixel_masks( dib, val >> 16, val >> 8, val );
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
            dst_ptr[x] = rgb_to_pixel_masks( dib, val >> 16, val >> 8, val );
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
    BYTE *dst_ptr = get_pixel_ptr_4( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y, pos;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0, pos = (dib->rect.left + rect->left) & 1; x < rect->right - rect->left; x++, pos++)
        {
            /* no antialiasing, glyph should only contain 0 or 16. */
            if (glyph_ptr[x] >= 16)
            {
                if (pos & 1)
                    dst_ptr[pos / 2] = text_pixel | (dst_ptr[pos / 2] & 0xf0);
                else
                    dst_ptr[pos / 2] = (text_pixel << 4) | (dst_ptr[pos / 2] & 0x0f);
            }
        }
        dst_ptr += dib->stride;
        glyph_ptr += glyph->stride;
    }
}

static void draw_glyph_1( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                          const POINT *origin, DWORD text_pixel, const struct intensity_range *ranges )
{
    BYTE *dst_ptr = get_pixel_ptr_1( dib, rect->left, rect->top );
    const BYTE *glyph_ptr = get_pixel_ptr_8( glyph, origin->x, origin->y );
    int x, y, pos;
    BYTE text = (text_pixel & 1) ? 0xff : 0;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0, pos = (dib->rect.left + rect->left) & 7; x < rect->right - rect->left; x++, pos++)
        {
            /* no antialiasing, glyph should only contain 0 or 16. */
            if (glyph_ptr[x] >= 16)
                dst_ptr[pos / 8] = (dst_ptr[pos / 8] & ~pixel_masks_1[pos % 8]) |
                                   (text & pixel_masks_1[pos % 8]);
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
static inline BYTE blend_color_gamma( BYTE dst, BYTE text, BYTE alpha,
                                      const struct font_gamma_ramp *gamma_ramp )
{
    if (alpha == 0) return dst;
    if (alpha == 255) return text;
    if (dst == text) return dst;

    return gamma_ramp->encode[ blend_color( gamma_ramp->decode[dst],
                                            gamma_ramp->decode[text],
                                            alpha ) ];
}

static inline DWORD blend_subpixel( BYTE r, BYTE g, BYTE b, DWORD text, DWORD alpha,
                                    const struct font_gamma_ramp *gamma_ramp )
{
    if (gamma_ramp != NULL && gamma_ramp->gamma != 1000)
    {
        return blend_color_gamma( r, text >> 16, (BYTE)(alpha >> 16), gamma_ramp ) << 16 |
               blend_color_gamma( g, text >> 8,  (BYTE)(alpha >> 8),  gamma_ramp ) << 8  |
               blend_color_gamma( b, text,       (BYTE) alpha,        gamma_ramp );
    }
    return blend_color( r, text >> 16, (BYTE)(alpha >> 16) ) << 16 |
           blend_color( g, text >> 8,  (BYTE)(alpha >> 8) )  << 8  |
           blend_color( b, text,       (BYTE) alpha );
}

static void draw_subpixel_glyph_8888( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                                      const POINT *origin, DWORD text_pixel,
                                      const struct font_gamma_ramp *gamma_ramp )
{
    DWORD *dst_ptr = get_pixel_ptr_32( dib, rect->left, rect->top );
    const DWORD *glyph_ptr = get_pixel_ptr_32( glyph, origin->x, origin->y );
    int x, y;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] == 0) continue;
            dst_ptr[x] = blend_subpixel( dst_ptr[x] >> 16, dst_ptr[x] >> 8, dst_ptr[x],
                                         text_pixel, glyph_ptr[x], gamma_ramp );
        }
        dst_ptr += dib->stride / 4;
        glyph_ptr += glyph->stride / 4;
    }
}

static void draw_subpixel_glyph_32( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                                    const POINT *origin, DWORD text_pixel,
                                    const struct font_gamma_ramp *gamma_ramp )
{
    DWORD *dst_ptr = get_pixel_ptr_32( dib, rect->left, rect->top );
    const DWORD *glyph_ptr = get_pixel_ptr_32( glyph, origin->x, origin->y );
    int x, y;
    DWORD text, val;

    text = get_field( text_pixel, dib->red_shift,   dib->red_len ) << 16 |
           get_field( text_pixel, dib->green_shift, dib->green_len ) << 8 |
           get_field( text_pixel, dib->blue_shift,  dib->blue_len );

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] == 0) continue;
            val = blend_subpixel( get_field(dst_ptr[x], dib->red_shift,   dib->red_len),
                                  get_field(dst_ptr[x], dib->green_shift, dib->green_len),
                                  get_field(dst_ptr[x], dib->blue_shift,  dib->blue_len),
                                  text, glyph_ptr[x], gamma_ramp );
            dst_ptr[x] = rgb_to_pixel_masks( dib, val >> 16, val >> 8, val );
        }
        dst_ptr += dib->stride / 4;
        glyph_ptr += glyph->stride / 4;
    }
}

static void draw_subpixel_glyph_24( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                                    const POINT *origin, DWORD text_pixel,
                                    const struct font_gamma_ramp *gamma_ramp )
{
    BYTE *dst_ptr = get_pixel_ptr_24( dib, rect->left, rect->top );
    const DWORD *glyph_ptr = get_pixel_ptr_32( glyph, origin->x, origin->y );
    int x, y;
    DWORD val;

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] == 0) continue;
            val = blend_subpixel( dst_ptr[x * 3 + 2], dst_ptr[x * 3 + 1], dst_ptr[x * 3],
                                  text_pixel, glyph_ptr[x], gamma_ramp );
            dst_ptr[x * 3]     = val;
            dst_ptr[x * 3 + 1] = val >> 8;
            dst_ptr[x * 3 + 2] = val >> 16;
        }
        dst_ptr += dib->stride;
        glyph_ptr += glyph->stride / 4;
    }
}

static void draw_subpixel_glyph_555( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                                     const POINT *origin, DWORD text_pixel,
                                     const struct font_gamma_ramp *gamma_ramp )
{
    WORD *dst_ptr = get_pixel_ptr_16( dib, rect->left, rect->top );
    const DWORD *glyph_ptr = get_pixel_ptr_32( glyph, origin->x, origin->y );
    int x, y;
    DWORD text, val;

    text = ((text_pixel << 9) & 0xf80000) | ((text_pixel << 4) & 0x070000) |
           ((text_pixel << 6) & 0x00f800) | ((text_pixel << 1) & 0x000700) |
           ((text_pixel << 3) & 0x0000f8) | ((text_pixel >> 2) & 0x000007);

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] == 0) continue;
            val = blend_subpixel( ((dst_ptr[x] >> 7) & 0xf8) | ((dst_ptr[x] >> 12) & 0x07),
                                  ((dst_ptr[x] >> 2) & 0xf8) | ((dst_ptr[x] >>  7) & 0x07),
                                  ((dst_ptr[x] << 3) & 0xf8) | ((dst_ptr[x] >>  2) & 0x07),
                                  text, glyph_ptr[x], NULL );
            dst_ptr[x] = ((val >> 9) & 0x7c00) | ((val >> 6) & 0x03e0) | ((val >> 3) & 0x001f);
        }
        dst_ptr += dib->stride / 2;
        glyph_ptr += glyph->stride / 4;
    }
}

static void draw_subpixel_glyph_16( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                                    const POINT *origin, DWORD text_pixel,
                                    const struct font_gamma_ramp *gamma_ramp )
{
    WORD *dst_ptr = get_pixel_ptr_16( dib, rect->left, rect->top );
    const DWORD *glyph_ptr = get_pixel_ptr_32( glyph, origin->x, origin->y );
    int x, y;
    DWORD text, val;

    text = get_field( text_pixel, dib->red_shift,   dib->red_len ) << 16 |
           get_field( text_pixel, dib->green_shift, dib->green_len ) << 8 |
           get_field( text_pixel, dib->blue_shift,  dib->blue_len );

    for (y = rect->top; y < rect->bottom; y++)
    {
        for (x = 0; x < rect->right - rect->left; x++)
        {
            if (glyph_ptr[x] == 0) continue;
            val = blend_subpixel( get_field(dst_ptr[x], dib->red_shift,   dib->red_len),
                                  get_field(dst_ptr[x], dib->green_shift, dib->green_len),
                                  get_field(dst_ptr[x], dib->blue_shift,  dib->blue_len),
                                  text, glyph_ptr[x], NULL );
            dst_ptr[x] = rgb_to_pixel_masks( dib, val >> 16, val >> 8, val );
        }
        dst_ptr += dib->stride / 2;
        glyph_ptr += glyph->stride / 4;
    }
}

static void draw_subpixel_glyph_null( const dib_info *dib, const RECT *rect, const dib_info *glyph,
                                      const POINT *origin, DWORD text_pixel,
                                      const struct font_gamma_ramp *gamma_ramp )
{
    return;
}

static void create_rop_masks_32(const dib_info *dib, const BYTE *hatch_ptr,
                                const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    DWORD *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    for(y = 0; y < 8; y++, hatch_ptr++)
    {
        for(x = 0; x < 8; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x])
            {
                and_bits[x] = fg->and;
                xor_bits[x] = fg->xor;
            }
            else
            {
                and_bits[x] = bg->and;
                xor_bits[x] = bg->xor;
            }
        }
        and_bits += dib->stride / 4;
        xor_bits += dib->stride / 4;
    }
}

static void create_rop_masks_24(const dib_info *dib, const BYTE *hatch_ptr,
                                const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    DWORD mask_start = 0, mask_offset;
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    for(y = 0; y < 8; y++, hatch_ptr++)
    {
        mask_offset = mask_start;
        for(x = 0; x < 8; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x])
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
        }
        mask_start += dib->stride;
    }
}

static void create_rop_masks_16(const dib_info *dib, const BYTE *hatch_ptr,
                                const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    WORD *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    for(y = 0; y < 8; y++, hatch_ptr++)
    {
        for(x = 0; x < 8; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x])
            {
                and_bits[x] = fg->and;
                xor_bits[x] = fg->xor;
            }
            else
            {
                and_bits[x] = bg->and;
                xor_bits[x] = bg->xor;
            }
        }
        and_bits += dib->stride / 2;
        xor_bits += dib->stride / 2;
    }
}

static void create_rop_masks_8(const dib_info *dib, const BYTE *hatch_ptr,
                               const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    for(y = 0; y < 8; y++, hatch_ptr++)
    {
        for(x = 0; x < 8; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x])
            {
                and_bits[x] = fg->and;
                xor_bits[x] = fg->xor;
            }
            else
            {
                and_bits[x] = bg->and;
                xor_bits[x] = bg->xor;
            }
        }
        and_bits += dib->stride;
        xor_bits += dib->stride;
    }
}

static void create_rop_masks_4(const dib_info *dib, const BYTE *hatch_ptr,
                               const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    DWORD mask_offset;
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    const rop_mask *rop_mask;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    for(y = 0; y < 8; y++, hatch_ptr++)
    {
        for(x = mask_offset = 0; x < 8; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x])
                rop_mask = fg;
            else
                rop_mask = bg;

            if(x & 1)
            {
                and_bits[mask_offset] |= (rop_mask->and & 0x0f);
                xor_bits[mask_offset] |= (rop_mask->xor & 0x0f);
                mask_offset++;
            }
            else
            {
                and_bits[mask_offset] = (rop_mask->and << 4) & 0xf0;
                xor_bits[mask_offset] = (rop_mask->xor << 4) & 0xf0;
            }
        }
        and_bits += dib->stride;
        xor_bits += dib->stride;
    }
}

static void create_rop_masks_1(const dib_info *dib, const BYTE *hatch_ptr,
                               const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    rop_mask rop_mask;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    for(y = 0; y < 8; y++, hatch_ptr++)
    {
        *and_bits = *xor_bits = 0;
        for(x = 0; x < 8; x++)
        {
            if(*hatch_ptr & pixel_masks_1[x])
            {
                rop_mask.and = (fg->and & 1) ? 0xff : 0;
                rop_mask.xor = (fg->xor & 1) ? 0xff : 0;
            }
            else
            {
                rop_mask.and = (bg->and & 1) ? 0xff : 0;
                rop_mask.xor = (bg->xor & 1) ? 0xff : 0;
            }
            *and_bits |= (rop_mask.and & pixel_masks_1[x]);
            *xor_bits |= (rop_mask.xor & pixel_masks_1[x]);
        }
        and_bits += dib->stride;
        xor_bits += dib->stride;
    }
}

static void create_rop_masks_null(const dib_info *dib, const BYTE *hatch_ptr,
                                  const rop_mask *fg, const rop_mask *bg, rop_mask_bits *bits)
{
}

static void create_dither_masks_8(const dib_info *dib, int rop2, COLORREF color, rop_mask_bits *bits)
{
    /* mapping between RGB triples and the default color table */
    static const BYTE mapping[27] =
    {
        0,   /* 000000 -> 000000 */
        4,   /* 00007f -> 000080 */
        252, /* 0000ff -> 0000ff */
        2,   /* 007f00 -> 008000 */
        6,   /* 007f7f -> 008080 */
        224, /* 007fff -> 0080c0 */
        250, /* 00ff00 -> 00ff00 */
        184, /* 00ff7f -> 00e080 */
        254, /* 00ffff -> 00ffff */
        1,   /* 7f0000 -> 800000 */
        5,   /* 7f007f -> 800080 */
        196, /* 7f00ff -> 8000c0 */
        3,   /* 7f7f00 -> 808000 */
        248, /* 7f7f7f -> 808080 */
        228, /* 7f7fff -> 8080c0 */
        60,  /* 7fff00 -> 80e000 */
        188, /* 7fff7f -> 80e080 */
        244, /* 7fffff -> 80c0c0 */
        249, /* ff0000 -> ff0000 */
        135, /* ff007f -> e00080 */
        253, /* ff00ff -> ff00ff */
        39,  /* ff7f00 -> e08000 */
        167, /* ff7f7f -> e08080 */
        231, /* ff7fff -> e080c0 */
        251, /* ffff00 -> ffff00 */
        191, /* ffff7f -> e0e080 */
        255  /* ffffff -> ffffff */
    };

    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    struct rop_codes codes;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    get_rop_codes( rop2, &codes );

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            DWORD r = ((GetRValue(color) + 1) / 2 + bayer_8x8[y][x]) / 64;
            DWORD g = ((GetGValue(color) + 1) / 2 + bayer_8x8[y][x]) / 64;
            DWORD b = ((GetBValue(color) + 1) / 2 + bayer_8x8[y][x]) / 64;
            DWORD pixel = mapping[r * 9 + g * 3 + b];
            and_bits[x] = (pixel & codes.a1) ^ codes.a2;
            xor_bits[x] = (pixel & codes.x1) ^ codes.x2;
        }
        and_bits += dib->stride;
        xor_bits += dib->stride;
    }
}

static void create_dither_masks_4(const dib_info *dib, int rop2, COLORREF color, rop_mask_bits *bits)
{
    /* mapping between RGB triples and the default color table */
    static const BYTE mapping[27] =
    {
        0,  /* 000000 -> 000000 */
        4,  /* 00007f -> 000080 */
        12, /* 0000ff -> 0000ff */
        2,  /* 007f00 -> 008000 */
        6,  /* 007f7f -> 008080 */
        6,  /* 007fff -> 008080 */
        10, /* 00ff00 -> 00ff00 */
        6,  /* 00ff7f -> 008080 */
        14, /* 00ffff -> 00ffff */
        1,  /* 7f0000 -> 800000 */
        5,  /* 7f007f -> 800080 */
        5,  /* 7f00ff -> 800080 */
        3,  /* 7f7f00 -> 808000 */
        7,  /* 7f7f7f -> 808080 */
        8,  /* 7f7fff -> c0c0c0 */
        3,  /* 7fff00 -> 808000 */
        8,  /* 7fff7f -> c0c0c0 */
        8,  /* 7fffff -> c0c0c0 */
        9,  /* ff0000 -> ff0000 */
        5,  /* ff007f -> 800080 */
        13, /* ff00ff -> ff00ff */
        3,  /* ff7f00 -> 808000 */
        8,  /* ff7f7f -> c0c0c0 */
        8,  /* ff7fff -> c0c0c0 */
        11, /* ffff00 -> ffff00 */
        8,  /* ffff7f -> c0c0c0 */
        15  /* ffffff -> ffffff */
    };

    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    struct rop_codes codes;
    int x, y;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    get_rop_codes( rop2, &codes );

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            DWORD r = ((GetRValue(color) + 1) / 2 + bayer_8x8[y][x]) / 64;
            DWORD g = ((GetGValue(color) + 1) / 2 + bayer_8x8[y][x]) / 64;
            DWORD b = ((GetBValue(color) + 1) / 2 + bayer_8x8[y][x]) / 64;
            DWORD pixel = mapping[r * 9 + g * 3 + b];
            if (x & 1)
            {
                and_bits[x / 2] |= (pixel & codes.a1) ^ codes.a2;
                xor_bits[x / 2] |= (pixel & codes.x1) ^ codes.x2;
            }
            else
            {
                and_bits[x / 2] = ((pixel & codes.a1) ^ codes.a2) << 4;
                xor_bits[x / 2] = ((pixel & codes.x1) ^ codes.x2) << 4;
            }
        }
        and_bits += dib->stride;
        xor_bits += dib->stride;
    }
}

static void create_dither_masks_1(const dib_info *dib, int rop2, COLORREF color, rop_mask_bits *bits)
{
    BYTE *and_bits = bits->and, *xor_bits = bits->xor;
    struct rop_codes codes;
    rop_mask rop_mask;
    int x, y, grey = (30 * GetRValue(color) + 59 * GetGValue(color) + 11 * GetBValue(color) + 200) / 400;

    /* masks are always 8x8 */
    assert( dib->width == 8 );
    assert( dib->height == 8 );

    get_rop_codes( rop2, &codes );

    for (y = 0; y < 8; y++)
    {
        *and_bits = *xor_bits = 0;
        for (x = 0; x < 8; x++)
        {
            if (grey + bayer_8x8[y][x] > 63)
            {
                rop_mask.and = (0xff & codes.a1) ^ codes.a2;
                rop_mask.xor = (0xff & codes.x1) ^ codes.x2;
            }
            else
            {
                rop_mask.and = (0x00 & codes.a1) ^ codes.a2;
                rop_mask.xor = (0x00 & codes.x1) ^ codes.x2;
            }
            *and_bits |= (rop_mask.and & pixel_masks_1[x]);
            *xor_bits |= (rop_mask.xor & pixel_masks_1[x]);
        }
        and_bits += dib->stride;
        xor_bits += dib->stride;
    }
}

static void create_dither_masks_null(const dib_info *dib, int rop2, COLORREF color, rop_mask_bits *bits)
{
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

    if (mode == STRETCH_DELETESCANS || !keep_dst)
    {
        for (width = params->length; width; width--)
        {
            *dst_ptr = *src_ptr;
            dst_ptr += params->dst_inc;
            if (err > 0)
            {
                src_ptr += params->src_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
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

    if (mode == STRETCH_DELETESCANS || !keep_dst)
    {
        for (width = params->length; width; width--)
        {
            dst_ptr[0] = src_ptr[0];
            dst_ptr[1] = src_ptr[1];
            dst_ptr[2] = src_ptr[2];
            dst_ptr += 3 * params->dst_inc;
            if (err > 0)
            {
                src_ptr += 3 * params->src_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
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

    if (mode == STRETCH_DELETESCANS || !keep_dst)
    {
        for (width = params->length; width; width--)
        {
            *dst_ptr = *src_ptr;
            dst_ptr += params->dst_inc;
            if (err > 0)
            {
                src_ptr += params->src_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
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

    if (mode == STRETCH_DELETESCANS || !keep_dst)
    {
        for (width = params->length; width; width--)
        {
            *dst_ptr = *src_ptr;
            dst_ptr += params->dst_inc;
            if (err > 0)
            {
                src_ptr += params->src_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
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
}

static void stretch_row_4(const dib_info *dst_dib, const POINT *dst_start,
                          const dib_info *src_dib, const POINT *src_start,
                          const struct stretch_params *params, int mode,
                          BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_4( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_4( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width, dst_x = dst_dib->rect.left + dst_start->x, src_x = src_dib->rect.left + src_start->x;
    struct rop_codes codes;
    BYTE src_val;

    if (!keep_dst) mode = STRETCH_DELETESCANS;
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
    int width, dst_x = dst_dib->rect.left + dst_start->x, src_x = src_dib->rect.left + src_start->x;
    struct rop_codes codes;
    BYTE src_val;

    if (!keep_dst) mode = STRETCH_DELETESCANS;
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

    if (mode == STRETCH_DELETESCANS)
    {
        for (width = params->length; width; width--)
        {
            *dst_ptr = *src_ptr;
            src_ptr += params->src_inc;
            if (err > 0)
            {
                dst_ptr += params->dst_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
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

    if (mode == STRETCH_DELETESCANS)
    {
        for (width = params->length; width; width--)
        {
            dst_ptr[0] = src_ptr[0];
            dst_ptr[1] = src_ptr[1];
            dst_ptr[2] = src_ptr[2];
            src_ptr += 3 * params->src_inc;
            if (err > 0)
            {
                dst_ptr += 3 * params->dst_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
        struct rop_codes codes;
        BYTE init_val = (mode == STRETCH_ANDSCANS) ? 0xff : 0;
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

    if (mode == STRETCH_DELETESCANS)
    {
        for (width = params->length; width; width--)
        {
            *dst_ptr = *src_ptr;
            src_ptr += params->src_inc;
            if (err > 0)
            {
                dst_ptr += params->dst_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
        struct rop_codes codes;
        WORD init_val = (mode == STRETCH_ANDSCANS) ? 0xffff : 0;
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

    if (mode == STRETCH_DELETESCANS)
    {
        for (width = params->length; width; width--)
        {
            *dst_ptr = *src_ptr;
            src_ptr += params->src_inc;
            if (err > 0)
            {
                dst_ptr += params->dst_inc;
                err += params->err_add_1;
            }
            else err += params->err_add_2;
        }
    }
    else
    {
        struct rop_codes codes;
        BYTE init_val = (mode == STRETCH_ANDSCANS) ? 0xff : 0;
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
}

static void shrink_row_4(const dib_info *dst_dib, const POINT *dst_start,
                         const dib_info *src_dib, const POINT *src_start,
                         const struct stretch_params *params, int mode,
                         BOOL keep_dst)
{
    BYTE *dst_ptr = get_pixel_ptr_4( dst_dib, dst_start->x, dst_start->y );
    BYTE *src_ptr = get_pixel_ptr_4( src_dib, src_start->x, src_start->y );
    int err = params->err_start;
    int width, dst_x = dst_dib->rect.left + dst_start->x, src_x = src_dib->rect.left + src_start->x;
    struct rop_codes codes;
    BYTE src_val, init_val = (mode == STRETCH_ANDSCANS) ? 0xff : 0;
    BOOL new_pix = TRUE;

    rop_codes_from_stretch_mode( mode, &codes );
    for (width = params->length; width; width--)
    {
        if (new_pix && !keep_dst) do_rop_mask_8( dst_ptr, 0, init_val, (dst_x & 1) ? 0x0f : 0xf0 );

        if (src_x & 1) src_val = (*src_ptr & 0x0f) | (*src_ptr << 4);
        else src_val = (*src_ptr & 0xf0) | (*src_ptr >> 4);

        do_rop_codes_mask_8( dst_ptr, src_val, &codes, (dst_x & 1) ? 0x0f : 0xf0 );
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
    int width, dst_x = dst_dib->rect.left + dst_start->x, src_x = src_dib->rect.left + src_start->x;
    struct rop_codes codes;
    BYTE src_val, init_val = (mode == STRETCH_ANDSCANS) ? 0xff : 0;
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

static float clampf( float value, float min, float max )
{
    return max( min, min( value, max ) );
}

static int clamp( int value, int min, int max )
{
    return max( min, min( value, max ) );
}

static BYTE linear_interpolate( BYTE start, BYTE end, float delta )
{
    return start + (end - start) * delta + 0.5f;
}

static BYTE bilinear_interpolate( BYTE c00, BYTE c01, BYTE c10, BYTE c11, float dx, float dy )
{
    return linear_interpolate( linear_interpolate( c00, c01, dx ),
                               linear_interpolate( c10, c11, dx ), dy );
}

static void calc_halftone_params( const struct bitblt_coords *dst, const struct bitblt_coords *src,
                                  RECT *dst_rect, RECT *src_rect, int *src_start_x,
                                  int *src_start_y, float *src_inc_x, float *src_inc_y )
{
    int src_width, src_height, dst_width, dst_height;
    BOOL mirrored_x, mirrored_y;

    get_bounding_rect( src_rect, src->x, src->y, src->width, src->height );
    get_bounding_rect( dst_rect, dst->x, dst->y, dst->width, dst->height );
    intersect_rect( src_rect, &src->visrect, src_rect );
    intersect_rect( dst_rect, &dst->visrect, dst_rect );
    OffsetRect( dst_rect, -dst_rect->left, -dst_rect->top );

    src_width = src_rect->right - src_rect->left;
    src_height = src_rect->bottom - src_rect->top;
    dst_width = dst_rect->right - dst_rect->left;
    dst_height = dst_rect->bottom - dst_rect->top;

    mirrored_x = (dst->width < 0) != (src->width < 0);
    mirrored_y = (dst->height < 0) != (src->height < 0);
    *src_start_x = mirrored_x ? src_rect->right - 1 : src_rect->left;
    *src_start_y = mirrored_y ? src_rect->bottom - 1 : src_rect->top;
    *src_inc_x = mirrored_x ? -(float)src_width / dst_width : (float)src_width / dst_width;
    *src_inc_y = mirrored_y ? -(float)src_height / dst_height : (float)src_height / dst_height;
}

static void halftone_888( const dib_info *dst_dib, const struct bitblt_coords *dst,
                          const dib_info *src_dib, const struct bitblt_coords *src )
{
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1;
    DWORD *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    BYTE c00_r, c01_r, c10_r, c11_r;
    BYTE c00_g, c01_g, c10_g, c11_g;
    BYTE c00_b, c01_b, c10_b, c11_b;
    RECT dst_rect, src_rect;
    BYTE r, g, b;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    dst_ptr = get_pixel_ptr_32( dst_dib, dst_rect.left, dst_rect.top );
    for (dst_y = 0; dst_y < dst_rect.bottom - dst_rect.top; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = get_pixel_ptr_32( src_dib, 0, y0 );
        src_ptr_dy = (y1 - y0) * src_dib->stride / 4;
        for (dst_x = 0; dst_x < dst_rect.right - dst_rect.left; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + x0;
            c01_ptr = src_ptr + x1;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            c00_r = (*c00_ptr >> 16) & 0xff;
            c01_r = (*c01_ptr >> 16) & 0xff;
            c10_r = (*c10_ptr >> 16) & 0xff;
            c11_r = (*c11_ptr >> 16) & 0xff;
            c00_g = (*c00_ptr >> 8) & 0xff;
            c01_g = (*c01_ptr >> 8) & 0xff;
            c10_g = (*c10_ptr >> 8) & 0xff;
            c11_g = (*c11_ptr >> 8) & 0xff;
            c00_b = *c00_ptr & 0xff;
            c01_b = *c01_ptr & 0xff;
            c10_b = *c10_ptr & 0xff;
            c11_b = *c11_ptr & 0xff;
            r = bilinear_interpolate( c00_r, c01_r, c10_r, c11_r, dx, dy );
            g = bilinear_interpolate( c00_g, c01_g, c10_g, c11_g, dx, dy );
            b = bilinear_interpolate( c00_b, c01_b, c10_b, c11_b, dx, dy );
            dst_ptr[dst_x] = ((r << 16) & 0xff0000) | ((g << 8) & 0x00ff00) | (b & 0x0000ff);

            float_x += src_inc_x;
        }

        dst_ptr += dst_dib->stride / 4;
        float_y += src_inc_y;
    }
}

static void halftone_32( const dib_info *dst_dib, const struct bitblt_coords *dst,
                         const dib_info *src_dib, const struct bitblt_coords *src )
{
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1;
    DWORD *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    BYTE c00_r, c01_r, c10_r, c11_r;
    BYTE c00_g, c01_g, c10_g, c11_g;
    BYTE c00_b, c01_b, c10_b, c11_b;
    RECT dst_rect, src_rect;
    BYTE r, g, b;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    dst_ptr = get_pixel_ptr_32( dst_dib, dst_rect.left, dst_rect.top );
    for (dst_y = 0; dst_y < dst_rect.bottom - dst_rect.top; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = get_pixel_ptr_32( src_dib, 0, y0 );
        src_ptr_dy = (y1 - y0) * src_dib->stride / 4;
        for (dst_x = 0; dst_x < dst_rect.right - dst_rect.left; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + x0;
            c01_ptr = src_ptr + x1;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            c00_r = get_field( *c00_ptr, src_dib->red_shift, src_dib->red_len );
            c01_r = get_field( *c01_ptr, src_dib->red_shift, src_dib->red_len );
            c10_r = get_field( *c10_ptr, src_dib->red_shift, src_dib->red_len );
            c11_r = get_field( *c11_ptr, src_dib->red_shift, src_dib->red_len );
            c00_g = get_field( *c00_ptr, src_dib->green_shift, src_dib->green_len );
            c01_g = get_field( *c01_ptr, src_dib->green_shift, src_dib->green_len );
            c10_g = get_field( *c10_ptr, src_dib->green_shift, src_dib->green_len );
            c11_g = get_field( *c11_ptr, src_dib->green_shift, src_dib->green_len );
            c00_b = get_field( *c00_ptr, src_dib->blue_shift, src_dib->blue_len );
            c01_b = get_field( *c01_ptr, src_dib->blue_shift, src_dib->blue_len );
            c10_b = get_field( *c10_ptr, src_dib->blue_shift, src_dib->blue_len );
            c11_b = get_field( *c11_ptr, src_dib->blue_shift, src_dib->blue_len );
            r = bilinear_interpolate( c00_r, c01_r, c10_r, c11_r, dx, dy );
            g = bilinear_interpolate( c00_g, c01_g, c10_g, c11_g, dx, dy );
            b = bilinear_interpolate( c00_b, c01_b, c10_b, c11_b, dx, dy );
            dst_ptr[dst_x] = rgb_to_pixel_masks( dst_dib, r, g, b );

            float_x += src_inc_x;
        }

        dst_ptr += dst_dib->stride / 4;
        float_y += src_inc_y;
    }
}

static void halftone_24( const dib_info *dst_dib, const struct bitblt_coords *dst,
                         const dib_info *src_dib, const struct bitblt_coords *src )
{
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1;
    BYTE *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    BYTE c00_r, c01_r, c10_r, c11_r;
    BYTE c00_g, c01_g, c10_g, c11_g;
    BYTE c00_b, c01_b, c10_b, c11_b;
    RECT dst_rect, src_rect;
    BYTE r, g, b;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    dst_ptr = get_pixel_ptr_24( dst_dib, dst_rect.left, dst_rect.top );
    for (dst_y = 0; dst_y < dst_rect.bottom - dst_rect.top; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = get_pixel_ptr_24( src_dib, 0, y0 );
        src_ptr_dy = (y1 - y0) * src_dib->stride;
        for (dst_x = 0; dst_x < dst_rect.right - dst_rect.left; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + x0 * 3;
            c01_ptr = src_ptr + x1 * 3;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            c00_b = c00_ptr[0];
            c01_b = c01_ptr[0];
            c10_b = c10_ptr[0];
            c11_b = c11_ptr[0];
            c00_g = c00_ptr[1];
            c01_g = c01_ptr[1];
            c10_g = c10_ptr[1];
            c11_g = c11_ptr[1];
            c00_r = c00_ptr[2];
            c01_r = c01_ptr[2];
            c10_r = c10_ptr[2];
            c11_r = c11_ptr[2];
            r = bilinear_interpolate( c00_r, c01_r, c10_r, c11_r, dx, dy );
            g = bilinear_interpolate( c00_g, c01_g, c10_g, c11_g, dx, dy );
            b = bilinear_interpolate( c00_b, c01_b, c10_b, c11_b, dx, dy );
            dst_ptr[dst_x * 3] = b;
            dst_ptr[dst_x * 3 + 1] = g;
            dst_ptr[dst_x * 3 + 2] = r;

            float_x += src_inc_x;
        }

        dst_ptr += dst_dib->stride;
        float_y += src_inc_y;
    }
}

static void halftone_555( const dib_info *dst_dib, const struct bitblt_coords *dst,
                          const dib_info *src_dib, const struct bitblt_coords *src )
{
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1;
    WORD *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    BYTE c00_r, c01_r, c10_r, c11_r;
    BYTE c00_g, c01_g, c10_g, c11_g;
    BYTE c00_b, c01_b, c10_b, c11_b;
    RECT dst_rect, src_rect;
    BYTE r, g, b;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    dst_ptr = get_pixel_ptr_16( dst_dib, dst_rect.left, dst_rect.top );
    for (dst_y = 0; dst_y < dst_rect.bottom - dst_rect.top; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = get_pixel_ptr_16( src_dib, 0, y0 );
        src_ptr_dy = (y1 - y0) * src_dib->stride / 2;
        for (dst_x = 0; dst_x < dst_rect.right - dst_rect.left; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + x0;
            c01_ptr = src_ptr + x1;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            c00_r = ((*c00_ptr >> 7) & 0xf8) | ((*c00_ptr >> 12) & 0x07);
            c01_r = ((*c01_ptr >> 7) & 0xf8) | ((*c01_ptr >> 12) & 0x07);
            c10_r = ((*c10_ptr >> 7) & 0xf8) | ((*c10_ptr >> 12) & 0x07);
            c11_r = ((*c11_ptr >> 7) & 0xf8) | ((*c11_ptr >> 12) & 0x07);
            c00_g = ((*c00_ptr >> 2) & 0xf8) | ((*c00_ptr >>  7) & 0x07);
            c01_g = ((*c01_ptr >> 2) & 0xf8) | ((*c01_ptr >>  7) & 0x07);
            c10_g = ((*c10_ptr >> 2) & 0xf8) | ((*c10_ptr >>  7) & 0x07);
            c11_g = ((*c11_ptr >> 2) & 0xf8) | ((*c11_ptr >>  7) & 0x07);
            c00_b = ((*c00_ptr << 3) & 0xf8) | ((*c00_ptr >>  2) & 0x07);
            c01_b = ((*c01_ptr << 3) & 0xf8) | ((*c01_ptr >>  2) & 0x07);
            c10_b = ((*c10_ptr << 3) & 0xf8) | ((*c10_ptr >>  2) & 0x07);
            c11_b = ((*c11_ptr << 3) & 0xf8) | ((*c11_ptr >>  2) & 0x07);
            r = bilinear_interpolate( c00_r, c01_r, c10_r, c11_r, dx, dy );
            g = bilinear_interpolate( c00_g, c01_g, c10_g, c11_g, dx, dy );
            b = bilinear_interpolate( c00_b, c01_b, c10_b, c11_b, dx, dy );
            dst_ptr[dst_x] = ((r << 7) & 0x7c00) | ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f);

            float_x += src_inc_x;
        }

        dst_ptr += dst_dib->stride / 2;
        float_y += src_inc_y;
    }
}

static void halftone_16( const dib_info *dst_dib, const struct bitblt_coords *dst,
                         const dib_info *src_dib, const struct bitblt_coords *src )
{
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1;
    WORD *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    BYTE c00_r, c01_r, c10_r, c11_r;
    BYTE c00_g, c01_g, c10_g, c11_g;
    BYTE c00_b, c01_b, c10_b, c11_b;
    RECT dst_rect, src_rect;
    BYTE r, g, b;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    dst_ptr = get_pixel_ptr_16( dst_dib, dst_rect.left, dst_rect.top );
    for (dst_y = 0; dst_y < dst_rect.bottom - dst_rect.top; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = get_pixel_ptr_16( src_dib, 0, y0 );
        src_ptr_dy = (y1 - y0) * src_dib->stride / 2;
        for (dst_x = 0; dst_x < dst_rect.right - dst_rect.left; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + x0;
            c01_ptr = src_ptr + x1;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            c00_r = get_field( *c00_ptr, src_dib->red_shift, src_dib->red_len );
            c01_r = get_field( *c01_ptr, src_dib->red_shift, src_dib->red_len );
            c10_r = get_field( *c10_ptr, src_dib->red_shift, src_dib->red_len );
            c11_r = get_field( *c11_ptr, src_dib->red_shift, src_dib->red_len );
            c00_g = get_field( *c00_ptr, src_dib->green_shift, src_dib->green_len );
            c01_g = get_field( *c01_ptr, src_dib->green_shift, src_dib->green_len );
            c10_g = get_field( *c10_ptr, src_dib->green_shift, src_dib->green_len );
            c11_g = get_field( *c11_ptr, src_dib->green_shift, src_dib->green_len );
            c00_b = get_field( *c00_ptr, src_dib->blue_shift, src_dib->blue_len );
            c01_b = get_field( *c01_ptr, src_dib->blue_shift, src_dib->blue_len );
            c10_b = get_field( *c10_ptr, src_dib->blue_shift, src_dib->blue_len );
            c11_b = get_field( *c11_ptr, src_dib->blue_shift, src_dib->blue_len );
            r = bilinear_interpolate( c00_r, c01_r, c10_r, c11_r, dx, dy );
            g = bilinear_interpolate( c00_g, c01_g, c10_g, c11_g, dx, dy );
            b = bilinear_interpolate( c00_b, c01_b, c10_b, c11_b, dx, dy );
            dst_ptr[dst_x] = rgb_to_pixel_masks( dst_dib, r, g, b );

            float_x += src_inc_x;
        }

        dst_ptr += dst_dib->stride / 2;
        float_y += src_inc_y;
    }
}

static void halftone_8( const dib_info *dst_dib, const struct bitblt_coords *dst,
                        const dib_info *src_dib, const struct bitblt_coords *src )
{
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1;
    BYTE *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    RGBQUAD c00_rgb, c01_rgb, c10_rgb, c11_rgb, zero_rgb = {0};
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    const RGBQUAD *src_clr_table;
    RECT dst_rect, src_rect;
    BYTE r, g, b;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    src_clr_table = get_dib_color_table( src_dib );
    dst_ptr = get_pixel_ptr_8( dst_dib, dst_rect.left, dst_rect.top );
    for (dst_y = 0; dst_y < dst_rect.bottom - dst_rect.top; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = get_pixel_ptr_8( src_dib, 0, y0 );
        src_ptr_dy = (y1 - y0) * src_dib->stride;
        for (dst_x = 0; dst_x < dst_rect.right - dst_rect.left; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + x0;
            c01_ptr = src_ptr + x1;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            if (src_clr_table)
            {
                c00_rgb = *c00_ptr < src_dib->color_table_size ? src_clr_table[*c00_ptr] : zero_rgb;
                c01_rgb = *c01_ptr < src_dib->color_table_size ? src_clr_table[*c01_ptr] : zero_rgb;
                c10_rgb = *c10_ptr < src_dib->color_table_size ? src_clr_table[*c10_ptr] : zero_rgb;
                c11_rgb = *c11_ptr < src_dib->color_table_size ? src_clr_table[*c11_ptr] : zero_rgb;
                r = bilinear_interpolate( c00_rgb.rgbRed, c01_rgb.rgbRed, c10_rgb.rgbRed, c11_rgb.rgbRed, dx, dy );
                g = bilinear_interpolate( c00_rgb.rgbGreen, c01_rgb.rgbGreen, c10_rgb.rgbGreen, c11_rgb.rgbGreen, dx, dy );
                b = bilinear_interpolate( c00_rgb.rgbBlue, c01_rgb.rgbBlue, c10_rgb.rgbBlue, c11_rgb.rgbBlue, dx, dy );
            }
            else
            {
                r = 0;
                g = 0;
                b = 0;
            }
            dst_ptr[dst_x] = rgb_to_pixel_colortable( dst_dib, r, g, b );

            float_x += src_inc_x;
        }

        dst_ptr += dst_dib->stride;
        float_y += src_inc_y;
    }
}

static void halftone_4( const dib_info *dst_dib, const struct bitblt_coords *dst,
                        const dib_info *src_dib, const struct bitblt_coords *src )
{
    BYTE *dst_col_ptr, *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1;
    RGBQUAD c00_rgb, c01_rgb, c10_rgb, c11_rgb, zero_rgb = {0};
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    BYTE r, g, b, val, c00, c01, c10, c11;
    const RGBQUAD *src_clr_table;
    RECT dst_rect, src_rect;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    src_clr_table = get_dib_color_table( src_dib );
    dst_col_ptr = (BYTE *)dst_dib->bits.ptr + (dst_dib->rect.top + dst_rect.top) * dst_dib->stride;
    for (dst_y = 0; dst_y < dst_rect.bottom - dst_rect.top; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = (BYTE *)src_dib->bits.ptr + (src_dib->rect.top + y0) * src_dib->stride;
        src_ptr_dy = (y1 - y0) * src_dib->stride;
        for (dst_x = dst_rect.left; dst_x < dst_rect.right; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + (src_dib->rect.left + x0) / 2;
            c01_ptr = src_ptr + (src_dib->rect.left + x1) / 2;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            if ((src_dib->rect.left + x0) & 1)
            {
                c00 = *c00_ptr & 0x0f;
                c10 = *c10_ptr & 0x0f;
            }
            else
            {
                c00 = (*c00_ptr >> 4) & 0x0f;
                c10 = (*c10_ptr >> 4) & 0x0f;
            }
            if ((src_dib->rect.left + x1) & 1)
            {
                c01 = *c01_ptr & 0x0f;
                c11 = *c11_ptr & 0x0f;
            }
            else
            {
                c01 = (*c01_ptr >> 4) & 0x0f;
                c11 = (*c11_ptr >> 4) & 0x0f;
            }

            if (src_clr_table)
            {
                c00_rgb = c00 < src_dib->color_table_size ? src_clr_table[c00] : zero_rgb;
                c01_rgb = c01 < src_dib->color_table_size ? src_clr_table[c01] : zero_rgb;
                c10_rgb = c10 < src_dib->color_table_size ? src_clr_table[c10] : zero_rgb;
                c11_rgb = c11 < src_dib->color_table_size ? src_clr_table[c11] : zero_rgb;
                r = bilinear_interpolate( c00_rgb.rgbRed, c01_rgb.rgbRed, c10_rgb.rgbRed, c11_rgb.rgbRed, dx, dy );
                g = bilinear_interpolate( c00_rgb.rgbGreen, c01_rgb.rgbGreen, c10_rgb.rgbGreen, c11_rgb.rgbGreen, dx, dy );
                b = bilinear_interpolate( c00_rgb.rgbBlue, c01_rgb.rgbBlue, c10_rgb.rgbBlue, c11_rgb.rgbBlue, dx, dy );
            }
            else
            {
                r = 0;
                g = 0;
                b = 0;
            }

            dst_ptr = dst_col_ptr + (dst_dib->rect.left + dst_x) / 2;
            val = rgb_to_pixel_colortable( dst_dib, r, g, b );
            if ((dst_x + dst_dib->rect.left) & 1)
                *dst_ptr = (val & 0x0f) | (*dst_ptr & 0xf0);
            else
                *dst_ptr = (val << 4) & 0xf0;

            float_x += src_inc_x;
        }

        dst_col_ptr += dst_dib->stride;
        float_y += src_inc_y;
    }
}

static void halftone_1( const dib_info *dst_dib, const struct bitblt_coords *dst,
                        const dib_info *src_dib, const struct bitblt_coords *src )
{
    int src_start_x, src_start_y, src_ptr_dy, dst_x, dst_y, x0, x1, y0, y1, bit_pos;
    BYTE *dst_col_ptr, *dst_ptr, *src_ptr, *c00_ptr, *c01_ptr, *c10_ptr, *c11_ptr;
    RGBQUAD c00_rgb, c01_rgb, c10_rgb, c11_rgb, zero_rgb = {0};
    float src_inc_x, src_inc_y, float_x, float_y, dx, dy;
    BYTE r, g, b, val, c00, c01, c10, c11;
    const RGBQUAD *src_clr_table;
    RECT dst_rect, src_rect;
    RGBQUAD bg_entry;
    DWORD bg_pixel;

    calc_halftone_params( dst, src, &dst_rect, &src_rect, &src_start_x, &src_start_y, &src_inc_x,
                          &src_inc_y );

    float_y = src_start_y;
    bg_entry = *get_dib_color_table( dst_dib );
    src_clr_table = get_dib_color_table( src_dib );
    dst_col_ptr = (BYTE *)dst_dib->bits.ptr + (dst_dib->rect.top + dst_rect.top) * dst_dib->stride;
    for (dst_y = dst_rect.top; dst_y < dst_rect.bottom; ++dst_y)
    {
        float_y = clampf( float_y, src_rect.top, src_rect.bottom - 1 );
        y0 = float_y;
        y1 = clamp( y0 + 1, src_rect.top, src_rect.bottom - 1 );
        dy = float_y - y0;

        float_x = src_start_x;
        src_ptr = (BYTE *)src_dib->bits.ptr + (src_dib->rect.top + y0) * src_dib->stride;
        src_ptr_dy = (y1 - y0) * src_dib->stride;
        for (dst_x = dst_rect.left; dst_x < dst_rect.right; ++dst_x)
        {
            float_x = clampf( float_x, src_rect.left, src_rect.right - 1 );
            x0 = float_x;
            x1 = clamp( x0 + 1, src_rect.left, src_rect.right - 1 );
            dx = float_x - x0;

            c00_ptr = src_ptr + (src_dib->rect.left + x0) / 8;
            c01_ptr = src_ptr + (src_dib->rect.left + x1) / 8;
            c10_ptr = c00_ptr + src_ptr_dy;
            c11_ptr = c01_ptr + src_ptr_dy;
            c00 = (*c00_ptr & pixel_masks_1[(src_dib->rect.left + x0) & 7]) ? 1 : 0;
            c01 = (*c01_ptr & pixel_masks_1[(src_dib->rect.left + x1) & 7]) ? 1 : 0;
            c10 = (*c10_ptr & pixel_masks_1[(src_dib->rect.left + x0) & 7]) ? 1 : 0;
            c11 = (*c11_ptr & pixel_masks_1[(src_dib->rect.left + x1) & 7]) ? 1 : 0;

            if (src_clr_table)
            {
                c00_rgb = c00 < src_dib->color_table_size ? src_clr_table[c00] : zero_rgb;
                c01_rgb = c01 < src_dib->color_table_size ? src_clr_table[c01] : zero_rgb;
                c10_rgb = c10 < src_dib->color_table_size ? src_clr_table[c10] : zero_rgb;
                c11_rgb = c11 < src_dib->color_table_size ? src_clr_table[c11] : zero_rgb;
                r = bilinear_interpolate( c00_rgb.rgbRed, c01_rgb.rgbRed, c10_rgb.rgbRed, c11_rgb.rgbRed, dx, dy );
                g = bilinear_interpolate( c00_rgb.rgbGreen, c01_rgb.rgbGreen, c10_rgb.rgbGreen, c11_rgb.rgbGreen, dx, dy );
                b = bilinear_interpolate( c00_rgb.rgbBlue, c01_rgb.rgbBlue, c10_rgb.rgbBlue, c11_rgb.rgbBlue, dx, dy );
            }
            else
            {
                r = 0;
                g = 0;
                b = 0;
            }

            dst_ptr = dst_col_ptr + (dst_dib->rect.left + dst_x) / 8;
            bit_pos = (dst_x + dst_dib->rect.left) & 7;
            bg_pixel = FILTER_DIBINDEX(bg_entry, RGB(bg_entry.rgbRed, bg_entry.rgbGreen, bg_entry.rgbBlue));
            val = rgb_to_pixel_mono( dst_dib, FALSE, dst_x, dst_y, RGB(r, g, b), bg_pixel, r, g, b );
            if (bit_pos == 0)
                *dst_ptr = 0;
            *dst_ptr = (*dst_ptr & ~pixel_masks_1[bit_pos]) | (val & pixel_masks_1[bit_pos]);

            float_x += src_inc_x;
        }

        dst_col_ptr += dst_dib->stride;
        float_y += src_inc_y;
    }
}

static void halftone_null( const dib_info *dst_dib, const struct bitblt_coords *dst,
                           const dib_info *src_dib, const struct bitblt_coords *src )
{}

const primitive_funcs funcs_8888 =
{
    solid_rects_32,
    solid_line_32,
    pattern_rects_32,
    copy_rect_32,
    blend_rects_8888,
    gradient_rect_8888,
    mask_rect_32,
    draw_glyph_8888,
    draw_subpixel_glyph_8888,
    get_pixel_32,
    colorref_to_pixel_888,
    pixel_to_colorref_888,
    convert_to_8888,
    create_rop_masks_32,
    create_dither_masks_null,
    stretch_row_32,
    shrink_row_32,
    halftone_888
};

const primitive_funcs funcs_32 =
{
    solid_rects_32,
    solid_line_32,
    pattern_rects_32,
    copy_rect_32,
    blend_rects_32,
    gradient_rect_32,
    mask_rect_32,
    draw_glyph_32,
    draw_subpixel_glyph_32,
    get_pixel_32,
    colorref_to_pixel_masks,
    pixel_to_colorref_masks,
    convert_to_32,
    create_rop_masks_32,
    create_dither_masks_null,
    stretch_row_32,
    shrink_row_32,
    halftone_32
};

const primitive_funcs funcs_24 =
{
    solid_rects_24,
    solid_line_24,
    pattern_rects_24,
    copy_rect_24,
    blend_rects_24,
    gradient_rect_24,
    mask_rect_24,
    draw_glyph_24,
    draw_subpixel_glyph_24,
    get_pixel_24,
    colorref_to_pixel_888,
    pixel_to_colorref_888,
    convert_to_24,
    create_rop_masks_24,
    create_dither_masks_null,
    stretch_row_24,
    shrink_row_24,
    halftone_24
};

const primitive_funcs funcs_555 =
{
    solid_rects_16,
    solid_line_16,
    pattern_rects_16,
    copy_rect_16,
    blend_rects_555,
    gradient_rect_555,
    mask_rect_16,
    draw_glyph_555,
    draw_subpixel_glyph_555,
    get_pixel_16,
    colorref_to_pixel_555,
    pixel_to_colorref_555,
    convert_to_555,
    create_rop_masks_16,
    create_dither_masks_null,
    stretch_row_16,
    shrink_row_16,
    halftone_555
};

const primitive_funcs funcs_16 =
{
    solid_rects_16,
    solid_line_16,
    pattern_rects_16,
    copy_rect_16,
    blend_rects_16,
    gradient_rect_16,
    mask_rect_16,
    draw_glyph_16,
    draw_subpixel_glyph_16,
    get_pixel_16,
    colorref_to_pixel_masks,
    pixel_to_colorref_masks,
    convert_to_16,
    create_rop_masks_16,
    create_dither_masks_null,
    stretch_row_16,
    shrink_row_16,
    halftone_16
};

const primitive_funcs funcs_8 =
{
    solid_rects_8,
    solid_line_8,
    pattern_rects_8,
    copy_rect_8,
    blend_rects_8,
    gradient_rect_8,
    mask_rect_8,
    draw_glyph_8,
    draw_subpixel_glyph_null,
    get_pixel_8,
    colorref_to_pixel_colortable,
    pixel_to_colorref_colortable,
    convert_to_8,
    create_rop_masks_8,
    create_dither_masks_8,
    stretch_row_8,
    shrink_row_8,
    halftone_8
};

const primitive_funcs funcs_4 =
{
    solid_rects_4,
    solid_line_4,
    pattern_rects_4,
    copy_rect_4,
    blend_rects_4,
    gradient_rect_4,
    mask_rect_4,
    draw_glyph_4,
    draw_subpixel_glyph_null,
    get_pixel_4,
    colorref_to_pixel_colortable,
    pixel_to_colorref_colortable,
    convert_to_4,
    create_rop_masks_4,
    create_dither_masks_4,
    stretch_row_4,
    shrink_row_4,
    halftone_4
};

const primitive_funcs funcs_1 =
{
    solid_rects_1,
    solid_line_1,
    pattern_rects_1,
    copy_rect_1,
    blend_rects_1,
    gradient_rect_1,
    mask_rect_null,
    draw_glyph_1,
    draw_subpixel_glyph_null,
    get_pixel_1,
    colorref_to_pixel_colortable,
    pixel_to_colorref_colortable,
    convert_to_1,
    create_rop_masks_1,
    create_dither_masks_1,
    stretch_row_1,
    shrink_row_1,
    halftone_1
};

const primitive_funcs funcs_null =
{
    solid_rects_null,
    solid_line_null,
    pattern_rects_null,
    copy_rect_null,
    blend_rects_null,
    gradient_rect_null,
    mask_rect_null,
    draw_glyph_null,
    draw_subpixel_glyph_null,
    get_pixel_null,
    colorref_to_pixel_null,
    pixel_to_colorref_null,
    convert_to_null,
    create_rop_masks_null,
    create_dither_masks_null,
    stretch_row_null,
    shrink_row_null,
    halftone_null
};
