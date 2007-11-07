/* DirectSound format conversion and mixing routines
 *
 * Copyright 2007 Maarten Lankhorst
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

/* 8 bits is unsigned, the rest is signed.
 * First I tried to reuse existing stuff from alsa-lib, after that
 * didn't work, I gave up and just went for individual hacks.
 *
 * 24 bit is expensive to do, due to unaligned access.
 * In dlls/winex11.drv/dib_convert.c convert_888_to_0888_asis there is a way
 * around it, but I'm happy current code works, maybe something for later.
 *
 * The ^ 0x80 flips the signed bit, this is the conversion from
 * signed (-128.. 0.. 127) to unsigned (0...255)
 * This is only temporary: All 8 bit data should be converted to signed.
 * then when fed to the sound card, it should be converted to unsigned again.
 *
 * Sound is LITTLE endian
 */

#include "config.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

typedef struct {
    uint8_t byte[3];
} int24_struct;

#ifdef WORDS_BIGENDIAN
#define le16(x) RtlUshortByteSwap((x))
#define le32(x) RtlUlongByteSwap((x))
#else
#define le16(x) (x)
#define le32(x) (x)
#endif

static void convert_8_to_8 (const void *src, void *dst)
{
    uint8_t *dest = dst;
    *dest = *(uint8_t*)src;
}

static void convert_8_to_16 (const void *src, void *dst)
{
    uint16_t dest = *(uint8_t*)src, *dest16 = dst;
    *dest16 = le16(dest * 257 - 32768);
}

static void convert_8_to_24 (const void *src, void *dst)
{
    uint8_t dest = *(uint8_t*)src;
    int24_struct *dest24 = dst;
    dest24->byte[0] = dest;
    dest24->byte[1] = dest;
    dest24->byte[2] = dest - 0x80;
}

static void convert_8_to_32 (const void *src, void *dst)
{
    uint32_t dest = *(uint8_t*)src, *dest32 = dst;
    *dest32 = le32(dest * 16843009 - 2147483648U);
}

static void convert_16_to_8 (const void *src, void *dst)
{
    uint8_t *dst8 = dst;
    *dst8 = (le16(*(uint16_t*)src)) / 256;
    *dst8 -= 0x80;
}

static void convert_16_to_16 (const void *src, void *dst)
{
    uint16_t *dest = dst;
    *dest = *(uint16_t*)src;
}

static void convert_16_to_24 (const void *src, void *dst)
{
    uint16_t dest = le16(*(uint16_t*)src);
    int24_struct *dest24 = dst;

    dest24->byte[0] = dest / 256;
    dest24->byte[1] = dest;
    dest24->byte[2] = dest / 256;
}

static void convert_16_to_32 (const void *src, void *dst)
{
    uint32_t dest = *(uint16_t*)src, *dest32 = dst;
    *dest32 = dest * 65537;
}

static void convert_24_to_8 (const void *src, void *dst)
{
    uint8_t *dst8 = dst;
    *dst8 = ((int24_struct*)src)->byte[2];
}

static void convert_24_to_16 (const void *src, void *dst)
{
    uint16_t *dest16 = dst;
    const int24_struct *source = src;
    *dest16 = le16(source->byte[2] * 256 + source->byte[1]);
}

static void convert_24_to_24 (const void *src, void *dst)
{
    int24_struct *dest24 = dst;
    const int24_struct *src24 = src;
    *dest24 = *src24;
}

static void convert_24_to_32 (const void *src, void *dst)
{
    uint32_t *dest32 = dst;
    const int24_struct *source = src;
    *dest32 = le32(source->byte[2] * 16777217 + source->byte[1] * 65536 + source->byte[0] * 256);
}

static void convert_32_to_8 (const void *src, void *dst)
{
    uint8_t *dst8 = dst;
    *dst8 = (le32(*(uint32_t*)src) / 16777216);
    *dst8 -= 0x80;
}

static void convert_32_to_16 (const void *src, void *dst)
{
    uint16_t *dest16 = dst;
    *dest16 = le16(le32(*(uint32_t*)src) / 65536);
}

static void convert_32_to_24 (const void *src, void *dst)
{
    uint32_t dest = le32(*(uint32_t*)dst);
    int24_struct *dest24 = dst;

    dest24->byte[0] = dest / 256;
    dest24->byte[1] = dest / 65536;
    dest24->byte[2] = dest / 16777216;
}

static void convert_32_to_32 (const void *src, void *dst)
{
    uint32_t *dest = dst;
    *dest = *(uint32_t*)src;
}

const bitsconvertfunc convertbpp[4][4] = {
    { convert_8_to_8, convert_8_to_16, convert_8_to_24, convert_8_to_32 },
    { convert_16_to_8, convert_16_to_16, convert_16_to_24, convert_16_to_32 },
    { convert_24_to_8, convert_24_to_16, convert_24_to_24, convert_24_to_32 },
    { convert_32_to_8, convert_32_to_16, convert_32_to_24, convert_32_to_32 },
};

static void mix8(int8_t *src, int32_t *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    while (len--)
        /* 8-bit WAV is unsigned, it's here converted to signed, normalize function will convert it back again */
        *(dst++) += (int8_t)((uint8_t)*(src++) - (uint8_t)0x80);
}

static void mix16(int16_t *src, int32_t *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 2;
    while (len--)
    {
        *dst += le16(*src);
        ++dst; ++src;
    }
}

static void mix24(int24_struct *src, int32_t *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 3;
    while (len--)
    {
        uint32_t field;
        field = ((unsigned)src->byte[2] << 16) + ((unsigned)src->byte[1] << 8) + (unsigned)src->byte[0];
        if (src->byte[2] & 0x80)
            field |= 0xFF000000U;
        *(dst++) += field;
        ++src;
    }
}

static void mix32(int32_t *src, int64_t *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
        *(dst++) += le32(*(src++));
}

const mixfunc mixfunctions[4] = {
    (mixfunc)mix8,
    (mixfunc)mix16,
    (mixfunc)mix24,
    (mixfunc)mix32
};

static void norm8(int32_t *src, int8_t *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    while (len--)
    {
        *dst = (*src) + 0x80;
        if (*src < -0x80)
            *dst = 0;
        else if (*src > 0x7f)
            *dst = 0xff;
        ++dst;
        ++src;
    }
}

static void norm16(int32_t *src, int16_t *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 2;
    while (len--)
    {
        *dst = le16(*src);
        if (*src <= -0x8000)
            *dst = le16(0x8000);
        else if (*src > 0x7fff)
            *dst = le16(0x7fff);
        ++dst;
        ++src;
    }
}

static void norm24(int32_t *src, int24_struct *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 3;
    while (len--)
    {
        if (*src <= -0x800000)
        {
            dst->byte[0] = 0;
            dst->byte[1] = 0;
            dst->byte[2] = 0x80;
        }
        else if (*src > 0x7fffff)
        {
            dst->byte[0] = 0xff;
            dst->byte[1] = 0xff;
            dst->byte[2] = 0x7f;
        }
        else
        {
            dst->byte[0] = *src;
            dst->byte[1] = *src >> 8;
            dst->byte[2] = *src >> 16;
        }
        ++dst;
        ++src;
    }
}

static void norm32(int64_t *src, int32_t *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
    {
        *dst = le32(*src);
        if (*src <= -(int64_t)0x80000000)
            *dst = le32(0x80000000);
        else if (*src > 0x7fffffff)
            *dst = le32(0x7fffffff);
        ++dst;
        ++src;
    }
}

const normfunc normfunctions[4] = {
    (normfunc)norm8,
    (normfunc)norm16,
    (normfunc)norm24,
    (normfunc)norm32,
};
