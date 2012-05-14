/* DirectSound format conversion and mixing routines
 *
 * Copyright 2007 Maarten Lankhorst
 * Copyright 2011 Owen Rudge for CodeWeavers
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

#include <stdarg.h>
#include <math.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

#ifdef WORDS_BIGENDIAN
#define le16(x) RtlUshortByteSwap((x))
#define le32(x) RtlUlongByteSwap((x))
#else
#define le16(x) (x)
#define le32(x) (x)
#endif

static float get8(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel)
{
    const BYTE* buf = dsb->buffer->memory;
    buf += pos + channel;
    return (buf[0] - 0x80) / (float)0x80;
}

static float get16(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel)
{
    const BYTE* buf = dsb->buffer->memory;
    const SHORT *sbuf = (const SHORT*)(buf + pos + 2 * channel);
    SHORT sample = (SHORT)le16(*sbuf);
    return sample / (float)0x8000;
}

static float get24(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel)
{
    LONG sample;
    const BYTE* buf = dsb->buffer->memory;
    buf += pos + 3 * channel;
    /* The next expression deliberately has an overflow for buf[2] >= 0x80,
       this is how negative values are made.
     */
    sample = (buf[0] << 8) | (buf[1] << 16) | (buf[2] << 24);
    return sample / (float)0x80000000U;
}

static float get32(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel)
{
    const BYTE* buf = dsb->buffer->memory;
    const LONG *sbuf = (const LONG*)(buf + pos + 4 * channel);
    LONG sample = le32(*sbuf);
    return sample / (float)0x80000000U;
}

static float getieee32(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel)
{
    const BYTE* buf = dsb->buffer->memory;
    const float *sbuf = (const float*)(buf + pos + 4 * channel);
    /* The value will be clipped later, when put into some non-float buffer */
    return *sbuf;
}

const bitsgetfunc getbpp[5] = {get8, get16, get24, get32, getieee32};

float get_mono(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel)
{
    DWORD channels = dsb->pwfx->nChannels;
    DWORD c;
    float val = 0;
    /* XXX: does Windows include LFE into the mix? */
    for (c = 0; c < channels; c++)
        val += dsb->get_aux(dsb, pos, c);
    val /= channels;
    return val;
}

static inline unsigned char f_to_8(float value)
{
    if(value <= -1.f)
        return 0;
    if(value >= 1.f * 0x7f / 0x80)
        return 0xFF;
    return lrintf((value + 1.f) * 0x80);
}

static void put8(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    BYTE* buf = dsb->device->tmp_buffer;
    buf += pos + channel;
    *buf = f_to_8(value);
}

static inline SHORT f_to_16(float value)
{
    if(value <= -1.f)
        return 0x8000;
    if(value >= 1.f * 0x7FFF / 0x8000)
        return 0x7FFF;
    return le16(lrintf(value * 0x8000));
}

static void put16(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    BYTE* buf = dsb->device->tmp_buffer;
    SHORT *sbuf = (SHORT*)(buf + pos + 2 * channel);
    *sbuf = f_to_16(value);
}

static LONG f_to_24(float value)
{
    if(value <= -1.f)
        return 0x80000000;
    if(value >= 1.f * 0x7FFFFF / 0x800000)
        return 0x7FFFFF00;
    return lrintf(value * 0x80000000U);
}

static void put24(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    BYTE* buf = dsb->device->tmp_buffer;
    LONG t;
    buf += pos + 3 * channel;
    t = f_to_24(value);
    buf[0] = (t >> 8) & 0xFF;
    buf[1] = (t >> 16) & 0xFF;
    buf[2] = (t >> 24) & 0xFF;
}

static inline LONG f_to_32(float value)
{
    if(value <= -1.f)
        return 0x80000000;
    if(value >= 1.f * 0x7FFFFFFF / 0x80000000U)  /* this rounds to 1.f */
        return 0x7FFFFFFF;
    return le32(lrintf(value * 0x80000000U));
}

static void put32(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    BYTE* buf = dsb->device->tmp_buffer;
    LONG *sbuf = (LONG*)(buf + pos + 4 * channel);
    *sbuf = f_to_32(value);
}

const bitsputfunc putbpp[4] = {put8, put16, put24, put32};

void put_mono2stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    dsb->put_aux(dsb, pos, 0, value);
    dsb->put_aux(dsb, pos, 1, value);
}

static void mix8(signed char *src, float *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    while (len--)
        /* 8-bit WAV is unsigned, it's here converted to signed, normalize function will convert it back again */
        *(dst++) += ((signed char)((BYTE)*(src++) - (BYTE)0x80)) / (float)0x80;
}

static void mix16(SHORT *src, float *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 2;
    while (len--)
    {
        *dst += le16(*src) / (float)0x8000U;
        ++dst; ++src;
    }
}

static void mix24(BYTE *src, float *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 3;
    while (len--)
    {
        DWORD field;
        field = ((DWORD)src[2] << 16) + ((DWORD)src[1] << 8) + (DWORD)src[0];
        if (src[2] & 0x80)
            field |= 0xFF000000U;
        *(dst++) += field / (float)0x800000U;
        src += 3;
    }
}

static void mix32(INT *src, float *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
        *(dst++) += le32(*(src++)) / (float)0x80000000U;
}

static void mixieee32(float *src, float *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
        *(dst++) += *(src++);
}

const mixfunc mixfunctions[5] = {
    (mixfunc)mix8,
    (mixfunc)mix16,
    (mixfunc)mix24,
    (mixfunc)mix32,
    (mixfunc)mixieee32
};

static void norm8(float *src, unsigned char *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    while (len--)
    {
        *dst = f_to_8(*src);
        ++dst;
        ++src;
    }
}

static void norm16(float *src, SHORT *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 2;
    while (len--)
    {
        *dst = f_to_16(*src);
        ++dst;
        ++src;
    }
}

static void norm24(float *src, BYTE *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 3;
    while (len--)
    {
        LONG t = f_to_24(*src);
        dst[0] = (t >> 8) & 0xFF;
        dst[1] = (t >> 16) & 0xFF;
        dst[2] = t >> 24;
        dst += 3;
        ++src;
    }
}

static void norm32(float *src, INT *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
    {
        *dst = f_to_32(*src);
        ++dst;
        ++src;
    }
}

static void normieee32(float *src, float *dst, unsigned len)
{
    TRACE("%p - %p %d\n", src, dst, len);
    len /= 4;
    while (len--)
    {
        if(*src > 1)
            *dst = 1;
        else if(*src < -1)
            *dst = -1;
        else
            *dst = *src;
        ++dst;
        ++src;
    }
}

const normfunc normfunctions[5] = {
    (normfunc)norm8,
    (normfunc)norm16,
    (normfunc)norm24,
    (normfunc)norm32,
    (normfunc)normieee32
};
