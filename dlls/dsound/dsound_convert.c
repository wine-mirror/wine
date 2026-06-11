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


#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

static void get8(const IDirectSoundBufferImpl *dsb, BYTE *base, float *dst, unsigned samples, DWORD channel)
{
    DWORD channels = dsb->pwfx->nChannels;
    const BYTE *buf = base + channel;
    int i;

    for (i = 0; i < samples; ++i)
        dst[i] = (buf[i * channels] - 0x80) / (float)0x80;
}

static void get16(const IDirectSoundBufferImpl *dsb, BYTE *base, float *dst, unsigned samples, DWORD channel)
{
    DWORD channels = dsb->pwfx->nChannels;
    const BYTE *buf = base + 2 * channel;
    const SHORT *sbuf = (const SHORT*)(buf);
    int i;

    for (i = 0; i < samples; ++i)
        dst[i] = sbuf[i * channels] / (float)0x8000;
}

static void get24(const IDirectSoundBufferImpl *dsb, BYTE *base, float *dst, unsigned samples, DWORD channel)
{
    DWORD channels = dsb->pwfx->nChannels;
    const BYTE *buf = base + 3 * channel;
    int i;

    for (i = 0; i < samples; ++i) {
        /* The next expression deliberately has an overflow for buf[2] >= 0x80,
           this is how negative values are made.
         */
        LONG sample =
                (buf[i * channels * 3 + 0] << 8) |
                (buf[i * channels * 3 + 1] << 16) |
                (buf[i * channels * 3 + 2] << 24);
        dst[i] = sample / (float)0x80000000U;
    }
}

static void get32(const IDirectSoundBufferImpl *dsb, BYTE *base, float *dst, unsigned samples, DWORD channel)
{
    DWORD channels = dsb->pwfx->nChannels;
    const BYTE *buf = base + 4 * channel;
    const LONG *sbuf = (const LONG*)(buf);
    int i;

    for (i = 0; i < samples; ++i)
        dst[i] = sbuf[i * channels] / (float)0x80000000U;
}

const bitsgetfunc getbpp[4] = {get8, get16, get24, get32};

void getieee32(const IDirectSoundBufferImpl *dsb, BYTE *base, float *dst, unsigned samples, DWORD channel)
{
    DWORD channels = dsb->pwfx->nChannels;
    const BYTE *buf = base + 4 * channel;
    const float *sbuf = (const float*)(buf);
    int i;

    for (i = 0; i < samples; ++i)
        /* The value will be clipped later, when put into some non-float buffer */
        dst[i] = sbuf[i * channels];
}

void putieee32(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    BYTE *buf = (BYTE *)dsb->device->tmp_buffer;
    float *fbuf = (float*)(buf + pos + sizeof(float) * channel);
    *fbuf = value;
}

void putieee32_sum(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    BYTE *buf = (BYTE *)dsb->device->tmp_buffer;
    float *fbuf = (float*)(buf + pos + sizeof(float) * channel);
    *fbuf += value;
}

void put_mono2stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    dsb->put_aux(dsb, pos, 0, value);
    dsb->put_aux(dsb, pos, 1, value);
}

void put_mono2quad(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    dsb->put_aux(dsb, pos, 0, value);
    dsb->put_aux(dsb, pos, 1, value);
    dsb->put_aux(dsb, pos, 2, value);
    dsb->put_aux(dsb, pos, 3, value);
}

void put_stereo2quad(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    if (channel == 0) { /* Left */
        dsb->put_aux(dsb, pos, 0, value); /* Front left */
        dsb->put_aux(dsb, pos, 2, value); /* Back left */
    } else if (channel == 1) { /* Right */
        dsb->put_aux(dsb, pos, 1, value); /* Front right */
        dsb->put_aux(dsb, pos, 3, value); /* Back right */
    }
}

void put_mono2surround51(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    dsb->put_aux(dsb, pos, 0, value);
    dsb->put_aux(dsb, pos, 1, value);
    dsb->put_aux(dsb, pos, 2, value);
    dsb->put_aux(dsb, pos, 3, value);
    dsb->put_aux(dsb, pos, 4, value);
    dsb->put_aux(dsb, pos, 5, value);
}

void put_stereo2surround51(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    if (channel == 0) { /* Left */
        dsb->put_aux(dsb, pos, 0, value); /* Front left */
        dsb->put_aux(dsb, pos, 4, value); /* Back left */

        dsb->put_aux(dsb, pos, 2, 0.0f); /* Mute front centre */
        dsb->put_aux(dsb, pos, 3, 0.0f); /* Mute LFE */
    } else if (channel == 1) { /* Right */
        dsb->put_aux(dsb, pos, 1, value); /* Front right */
        dsb->put_aux(dsb, pos, 5, value); /* Back right */
    }
}

void put_mono(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    /* XXX: does Windows include LFE into the mix? */
    dsb->put_aux(dsb, pos, 0, value);
}

void put_mono2surround71(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    dsb->put_aux(dsb, pos, 2, value); /* Front centre */

    dsb->put_aux(dsb, pos, 0, 0.0f); /* Mute front left */
    dsb->put_aux(dsb, pos, 1, 0.0f); /* Mute front right */
    dsb->put_aux(dsb, pos, 3, 0.0f); /* Mute LFE */
    dsb->put_aux(dsb, pos, 4, 0.0f); /* Mute back left */
    dsb->put_aux(dsb, pos, 5, 0.0f); /* Mute back right */
    dsb->put_aux(dsb, pos, 6, 0.0f); /* Mute side left */
    dsb->put_aux(dsb, pos, 7, 0.0f); /* Mute side right */
}

void put_stereo2surround71(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    if (channel == 0) { /* Left */
        dsb->put_aux(dsb, pos, 0, value); /* Front left */

        dsb->put_aux(dsb, pos, 2, 0.0f); /* Mute front centre */
        dsb->put_aux(dsb, pos, 3, 0.0f); /* Mute LFE */
        dsb->put_aux(dsb, pos, 4, 0.0f); /* Mute back left */
        dsb->put_aux(dsb, pos, 5, 0.0f); /* Mute back right */
        dsb->put_aux(dsb, pos, 6, 0.0f); /* Mute side left */
        dsb->put_aux(dsb, pos, 7, 0.0f); /* Mute side right */
    } else if (channel == 1) { /* Right */
        dsb->put_aux(dsb, pos, 1, value); /* Front right */
    }
}

void put_quad2surround71(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    if (channel == 0) { /* Front left */
        dsb->put_aux(dsb, pos, 0, value); /* Front left */

        dsb->put_aux(dsb, pos, 2, 0.0f); /* Mute front center */
        dsb->put_aux(dsb, pos, 3, 0.0f); /* Mute LFE */
        dsb->put_aux(dsb, pos, 6, 0.0f); /* Mute side left */
        dsb->put_aux(dsb, pos, 7, 0.0f); /* Mute side right */
    } else if (channel == 1) { /* Front right */
        dsb->put_aux(dsb, pos, 1, value); /* Front right */
    } else if (channel == 2) { /* Rear left */
        dsb->put_aux(dsb, pos, 4, value); /* Rear left */
    } else if (channel == 3) { /* Rear right */
        dsb->put_aux(dsb, pos, 5, value); /* Rear right */
    }
}

void put_surround512surround71(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    if (channel == 0) { /* Front left */
        dsb->put_aux(dsb, pos, 0, value); /* Front left */

        dsb->put_aux(dsb, pos, 6, 0.0f); /* Mute side left */
        dsb->put_aux(dsb, pos, 7, 0.0f); /* Mute side right */
    } else if (channel == 1) { /* Front right */
        dsb->put_aux(dsb, pos, 1, value); /* Front right */
    } else if (channel == 2) { /* Front center */
        dsb->put_aux(dsb, pos, 2, value); /* Front center */
    } else if (channel == 3) { /* LFE */
        dsb->put_aux(dsb, pos, 3, value); /* LFE */
    } else if (channel == 4) { /* Rear left */
        dsb->put_aux(dsb, pos, 4, value); /* Rear left */
    } else if (channel == 5) { /* Rear right */
        dsb->put_aux(dsb, pos, 5, value); /* Rear right */
    }
}

void put_surround512stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    /* based on analyzing a recording of a dsound downmix */
    switch(channel){

    case 4: /* surround left */
        value *= 0.24f;
        dsb->put_aux(dsb, pos, 0, value);
        break;

    case 0: /* front left */
        value *= 1.0f;
        dsb->put_aux(dsb, pos, 0, value);
        break;

    case 5: /* surround right */
        value *= 0.24f;
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 1: /* front right */
        value *= 1.0f;
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 2: /* centre */
        value *= 0.7;
        dsb->put_aux(dsb, pos, 0, value);
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 3:
        /* LFE is totally ignored in dsound when downmixing to 2 channels */
        break;
    }
}

void put_surround712stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    /* based on analyzing a recording of a dsound downmix */
    switch(channel){

    case 6: /* back left */
        value *= 0.24f;
        dsb->put_aux(dsb, pos, 0, value);
        break;

    case 4: /* surround left */
        value *= 0.24f;
        dsb->put_aux(dsb, pos, 0, value);
        break;

    case 0: /* front left */
        value *= 1.0f;
        dsb->put_aux(dsb, pos, 0, value);
        break;

    case 7: /* back right */
        value *= 0.24f;
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 5: /* surround right */
        value *= 0.24f;
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 1: /* front right */
        value *= 1.0f;
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 2: /* centre */
        value *= 0.7;
        dsb->put_aux(dsb, pos, 0, value);
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 3:
        /* LFE is totally ignored in dsound when downmixing to 2 channels */
        break;
    }
}

void put_quad2stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value)
{
    /* based on pulseaudio's downmix algorithm */
    switch(channel){

    case 2: /* back left */
        value *= 0.1f; /* (1/9) / (sum of left volumes) */
        dsb->put_aux(dsb, pos, 0, value);
        break;

    case 0: /* front left */
        value *= 0.9f; /* 1 / (sum of left volumes) */
        dsb->put_aux(dsb, pos, 0, value);
        break;

    case 3: /* back right */
        value *= 0.1f; /* (1/9) / (sum of right volumes) */
        dsb->put_aux(dsb, pos, 1, value);
        break;

    case 1: /* front right */
        value *= 0.9f; /* 1 / (sum of right volumes) */
        dsb->put_aux(dsb, pos, 1, value);
        break;
    }
}

void mixieee32(float *src, float *dst, unsigned samples)
{
    TRACE("%p - %p %d\n", src, dst, samples);
    while (samples--)
        *(dst++) += *(src++);
}
