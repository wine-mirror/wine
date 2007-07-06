/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
 * Copyright 2007 Peter Dons Tychsen
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
#include <stdarg.h>
#include <math.h>	/* Insomnia - pow() function */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan)
{
	double temp;
	TRACE("(%p)\n",volpan);

	TRACE("Vol=%d Pan=%d\n", volpan->lVolume, volpan->lPan);
	/* the AmpFactors are expressed in 16.16 fixed point */
	volpan->dwVolAmpFactor = (ULONG) (pow(2.0, volpan->lVolume / 600.0) * 0xffff);
	/* FIXME: dwPan{Left|Right}AmpFactor */

	/* FIXME: use calculated vol and pan ampfactors */
	temp = (double) (volpan->lVolume - (volpan->lPan > 0 ? volpan->lPan : 0));
	volpan->dwTotalLeftAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 0xffff);
	temp = (double) (volpan->lVolume + (volpan->lPan < 0 ? volpan->lPan : 0));
	volpan->dwTotalRightAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 0xffff);

	TRACE("left = %x, right = %x\n", volpan->dwTotalLeftAmpFactor, volpan->dwTotalRightAmpFactor);
}

void DSOUND_AmpFactorToVolPan(PDSVOLUMEPAN volpan)
{
    double left,right;
    TRACE("(%p)\n",volpan);

    TRACE("left=%x, right=%x\n",volpan->dwTotalLeftAmpFactor,volpan->dwTotalRightAmpFactor);
    if (volpan->dwTotalLeftAmpFactor==0)
        left=-10000;
    else
        left=600 * log(((double)volpan->dwTotalLeftAmpFactor) / 0xffff) / log(2);
    if (volpan->dwTotalRightAmpFactor==0)
        right=-10000;
    else
        right=600 * log(((double)volpan->dwTotalRightAmpFactor) / 0xffff) / log(2);
    if (left<right)
    {
        volpan->lVolume=right;
        volpan->dwVolAmpFactor=volpan->dwTotalRightAmpFactor;
    }
    else
    {
        volpan->lVolume=left;
        volpan->dwVolAmpFactor=volpan->dwTotalLeftAmpFactor;
    }
    if (volpan->lVolume < -10000)
        volpan->lVolume=-10000;
    volpan->lPan=right-left;
    if (volpan->lPan < -10000)
        volpan->lPan=-10000;

    TRACE("Vol=%d Pan=%d\n", volpan->lVolume, volpan->lPan);
}

void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb)
{
	TRACE("(%p)\n",dsb);

	/* calculate the 10ms write lead */
	dsb->writelead = (dsb->freq / 100) * dsb->pwfx->nBlockAlign;
}

/**
 * Check for application callback requests for when the play position
 * reaches certain points.
 *
 * The offsets that will be triggered will be those between the recorded
 * "last played" position for the buffer (i.e. dsb->playpos) and "len" bytes
 * beyond that position.
 */
void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, DWORD playpos, int len)
{
	int			i;
	DWORD			offset;
	LPDSBPOSITIONNOTIFY	event;
	TRACE("(%p,%d)\n",dsb,len);

	if (dsb->nrofnotifies == 0)
		return;

	TRACE("(%p) buflen = %d, playpos = %d, len = %d\n",
		dsb, dsb->buflen, playpos, len);
	for (i = 0; i < dsb->nrofnotifies ; i++) {
		event = dsb->notifies + i;
		offset = event->dwOffset;
		TRACE("checking %d, position %d, event = %p\n",
			i, offset, event->hEventNotify);
		/* DSBPN_OFFSETSTOP has to be the last element. So this is */
		/* OK. [Inside DirectX, p274] */
		/*  */
		/* This also means we can't sort the entries by offset, */
		/* because DSBPN_OFFSETSTOP == -1 */
		if (offset == DSBPN_OFFSETSTOP) {
			if (dsb->state == STATE_STOPPED) {
				SetEvent(event->hEventNotify);
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				return;
			} else
				return;
		}
		if ((playpos + len) >= dsb->buflen) {
			if ((offset < ((playpos + len) % dsb->buflen)) ||
			    (offset >= playpos)) {
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		} else {
			if ((offset >= playpos) && (offset < (playpos + len))) {
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		}
	}
}

/* WAV format info can be found at:
 *
 *    http://www.cwi.nl/ftp/audio/AudioFormats.part2
 *    ftp://ftp.cwi.nl/pub/audio/RIFF-format
 *
 * Import points to remember:
 *    8-bit WAV is unsigned
 *    16-bit WAV is signed
 */
 /* Use the same formulas as pcmconverter.c */
static inline INT16 cvtU8toS16(BYTE b)
{
    return (short)((b+(b << 8))-32768);
}

static inline BYTE cvtS16toU8(INT16 s)
{
    return (s >> 8) ^ (unsigned char)0x80;
}

/**
 * Copy a single frame from the given input buffer to the given output buffer.
 * Translate 8 <-> 16 bits and mono <-> stereo
 */
static inline void cp_fields(const IDirectSoundBufferImpl *dsb, const BYTE *ibuf, BYTE *obuf )
{
	DirectSoundDevice * device = dsb->device;
        INT fl,fr;

        if (dsb->pwfx->wBitsPerSample == 8)  {
                if (device->pwfx->wBitsPerSample == 8 &&
                    device->pwfx->nChannels == dsb->pwfx->nChannels) {
                        /* avoid needless 8->16->8 conversion */
                        *obuf=*ibuf;
                        if (dsb->pwfx->nChannels==2)
                                *(obuf+1)=*(ibuf+1);
                        return;
                }
                fl = cvtU8toS16(*ibuf);
                fr = (dsb->pwfx->nChannels==2 ? cvtU8toS16(*(ibuf + 1)) : fl);
        } else {
                fl = *((const INT16 *)ibuf);
                fr = (dsb->pwfx->nChannels==2 ? *(((const INT16 *)ibuf) + 1)  : fl);
        }

        if (device->pwfx->nChannels == 2) {
                if (device->pwfx->wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        *(obuf + 1) = cvtS16toU8(fr);
                        return;
                }
                if (device->pwfx->wBitsPerSample == 16) {
                        *((INT16 *)obuf) = fl;
                        *(((INT16 *)obuf) + 1) = fr;
                        return;
                }
        }
        if (device->pwfx->nChannels == 1) {
                fl = (fl + fr) >> 1;
                if (device->pwfx->wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        return;
                }
                if (device->pwfx->wBitsPerSample == 16) {
                        *((INT16 *)obuf) = fl;
                        return;
                }
        }
}

/**
 * Mix at most the given amount of data into the given device buffer from the
 * given secondary buffer, starting from the dsb's first currently unmixed
 * frame (buf_mixpos), translating frequency (pitch), stereo/mono and
 * bits-per-sample. The secondary buffer sample is looped if it is not
 * long enough and it is a looping buffer.
 * (Doesn't perform any mixing - this is a straight copy operation).
 *
 * Now with PerfectPitch (tm) technology
 *
 * dsb = the secondary buffer
 * buf = the device buffer
 * len = number of bytes to store in the device buffer
 *
 * Returns: the number of bytes read from the secondary buffer
 *   (ie. len, adjusted for frequency, number of channels and sample size,
 *    and limited by buffer length for non-looping buffers)
 */
static INT DSOUND_MixerNorm(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i, size, ipos, ilen;
	BYTE	*ibp, *obp;
	INT	iAdvance = dsb->pwfx->nBlockAlign;
	INT	oAdvance = dsb->device->pwfx->nBlockAlign;

	ibp = dsb->buffer->memory + dsb->buf_mixpos;
	obp = buf;

	TRACE("(%p, %p, %p), buf_mixpos=%d\n", dsb, ibp, obp, dsb->buf_mixpos);
	/* Check for the best case */
	if ((dsb->freq == dsb->device->pwfx->nSamplesPerSec) &&
	    (dsb->pwfx->wBitsPerSample == dsb->device->pwfx->wBitsPerSample) &&
	    (dsb->pwfx->nChannels == dsb->device->pwfx->nChannels)) {
	        INT bytesleft = dsb->buflen - dsb->buf_mixpos;
		TRACE("(%p) Best case\n", dsb);
	    	if (len <= bytesleft )
			CopyMemory(obp, ibp, len);
		else { /* wrap */
			CopyMemory(obp, ibp, bytesleft);
			CopyMemory(obp + bytesleft, dsb->buffer->memory, len - bytesleft);
		}
		return len;
	}

	/* Check for same sample rate */
	if (dsb->freq == dsb->device->pwfx->nSamplesPerSec) {
		TRACE("(%p) Same sample rate %d = primary %d\n", dsb,
			dsb->freq, dsb->device->pwfx->nSamplesPerSec);
		ilen = 0;
		for (i = 0; i < len; i += oAdvance) {
			cp_fields(dsb, ibp, obp );
			ibp += iAdvance;
			ilen += iAdvance;
			obp += oAdvance;
			if (ibp >= (BYTE *)(dsb->buffer->memory + dsb->buflen))
				ibp = dsb->buffer->memory;	/* wrap */
		}
		return (ilen);
	}

	/* Mix in different sample rates */
	/* */
	/* New PerfectPitch(tm) Technology (c) 1998 Rob Riggs */
	/* Patent Pending :-] */

	/* Patent enhancements (c) 2000 Ove K�ven,
	 * TransGaming Technologies Inc. */

	/* FIXME("(%p) Adjusting frequency: %ld -> %ld (need optimization)\n",
	   dsb, dsb->freq, dsb->device->pwfx->nSamplesPerSec); */

	size = len / oAdvance;
	ilen = 0;
	ipos = dsb->buf_mixpos;
	for (i = 0; i < size; i++) {
                cp_fields(dsb, (dsb->buffer->memory + ipos), obp);
		obp += oAdvance;
		dsb->freqAcc += dsb->freqAdjust;
		if (dsb->freqAcc >= (1<<DSOUND_FREQSHIFT)) {
			ULONG adv = (dsb->freqAcc>>DSOUND_FREQSHIFT) * iAdvance;
			dsb->freqAcc &= (1<<DSOUND_FREQSHIFT)-1;
			ipos += adv; ilen += adv;
			ipos %= dsb->buflen;
		}
	}
	return ilen;
}

static void DSOUND_MixerVol(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i;
	BYTE	*bpc = buf;
	INT16	*bps = (INT16 *) buf;

	TRACE("(%p,%p,%d)\n",dsb,buf,len);
	TRACE("left = %x, right = %x\n", dsb->volpan.dwTotalLeftAmpFactor,
		dsb->volpan.dwTotalRightAmpFactor);

	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->volpan.lPan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->volpan.lVolume == 0)) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return;		/* Nothing to do */

	/* If we end up with some bozo coder using panning or 3D sound */
	/* with a mono primary buffer, it could sound very weird using */
	/* this method. Oh well, tough patooties. */

	switch (dsb->device->pwfx->wBitsPerSample) {
	case 8:
		/* 8-bit WAV is unsigned, but we need to operate */
		/* on signed data for this to work properly */
		switch (dsb->device->pwfx->nChannels) {
		case 1:
			for (i = 0; i < len; i++) {
				INT val = *bpc - 128;
				val = (val * dsb->volpan.dwTotalLeftAmpFactor) >> 16;
				*bpc = val + 128;
				bpc++;
			}
			break;
		case 2:
			for (i = 0; i < len; i+=2) {
				INT val = *bpc - 128;
				val = (val * dsb->volpan.dwTotalLeftAmpFactor) >> 16;
				*bpc++ = val + 128;
				val = *bpc - 128;
				val = (val * dsb->volpan.dwTotalRightAmpFactor) >> 16;
				*bpc = val + 128;
				bpc++;
			}
			break;
		default:
			FIXME("doesn't support %d channels\n", dsb->device->pwfx->nChannels);
			break;
		}
		break;
	case 16:
		/* 16-bit WAV is signed -- much better */
		switch (dsb->device->pwfx->nChannels) {
		case 1:
			for (i = 0; i < len; i += 2) {
				*bps = (*bps * dsb->volpan.dwTotalLeftAmpFactor) >> 16;
				bps++;
			}
			break;
		case 2:
			for (i = 0; i < len; i += 4) {
				*bps = (*bps * dsb->volpan.dwTotalLeftAmpFactor) >> 16;
				bps++;
				*bps = (*bps * dsb->volpan.dwTotalRightAmpFactor) >> 16;
				bps++;
			}
			break;
		default:
			FIXME("doesn't support %d channels\n", dsb->device->pwfx->nChannels);
			break;
		}
		break;
	default:
		FIXME("doesn't support %d bit samples\n", dsb->device->pwfx->wBitsPerSample);
		break;
	}
}

/**
 * Make sure the device's tmp_buffer is at least the given size. Return a
 * pointer to it.
 */
static LPBYTE DSOUND_tmpbuffer(DirectSoundDevice *device, DWORD len)
{
    TRACE("(%p,%d)\n", device, len);

    if (len > device->tmp_buffer_len) {
        if (device->tmp_buffer)
            device->tmp_buffer = HeapReAlloc(GetProcessHeap(), 0, device->tmp_buffer, len);
        else
            device->tmp_buffer = HeapAlloc(GetProcessHeap(), 0, len);

        device->tmp_buffer_len = len;
    }

    return device->tmp_buffer;
}

/**
 * Mix (at most) the given number of bytes into the given position of the
 * device buffer, from the secondary buffer "dsb" (starting at the current
 * mix position for that buffer).
 *
 * Returns the number of bytes actually mixed into the device buffer. This
 * will match fraglen unless the end of the secondary buffer is reached
 * (and it is not looping).
 *
 * dsb  = the secondary buffer to mix from
 * writepos = position (offset) in device buffer to write at
 * fraglen = number of bytes to mix
 */
static DWORD DSOUND_MixInBuffer(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD fraglen)
{
	INT	i, len, ilen, field, todo;
	BYTE	*buf, *ibuf;

	TRACE("(%p,%d,%d)\n",dsb,writepos,fraglen);

	len = fraglen;
	if (!(dsb->playflags & DSBPLAY_LOOPING)) {
		/* This buffer is not looping, so make sure the requested
		 * length will not take us past the end of the buffer */
		int secondary_remainder = dsb->buflen - dsb->buf_mixpos;
		int adjusted_remainder = MulDiv(dsb->device->pwfx->nAvgBytesPerSec, secondary_remainder, dsb->nAvgBytesPerSec);
		assert(adjusted_remainder >= 0);
			/* The adjusted remainder must be at least one sample,
			 * otherwise we will never reach the end of the
			 * secondary buffer, as there will perpetually be a
			 * fractional remainder */
		TRACE("secondary_remainder = %d, adjusted_remainder = %d, len = %d\n", secondary_remainder, adjusted_remainder, len);
		if (adjusted_remainder < len) {
			TRACE("clipping len to remainder of secondary buffer\n");
			len = adjusted_remainder;
		}
		if (len == 0)
			return 0;
	}

	if (len % dsb->device->pwfx->nBlockAlign) {
		INT nBlockAlign = dsb->device->pwfx->nBlockAlign;
		ERR("length not a multiple of block size, len = %d, block size = %d\n", len, nBlockAlign);
		len -= len % nBlockAlign; /* data alignment */
	}

	/* Create temp buffer to hold actual resulting data */
	if ((buf = ibuf = DSOUND_tmpbuffer(dsb->device, len)) == NULL)
		return 0;

	TRACE("MixInBuffer (%p) len = %d, dest = %d\n", dsb, len, writepos);

	/* first, copy the data from the DirectSoundBuffer into the temporary
	   buffer, translating frequency/bits-per-sample/number-of-channels
	   to match the device settings */
	ilen = DSOUND_MixerNorm(dsb, ibuf, len);

	/* then apply the correct volume, if necessary */
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		DSOUND_MixerVol(dsb, ibuf, len);

	/* Now mix the temporary buffer into the devices main buffer */
	if (dsb->device->pwfx->wBitsPerSample == 8) {
		BYTE	*obuf = dsb->device->buffer + writepos;

		if ((writepos + len) <= dsb->device->buflen)
			todo = len;
		else
			todo = dsb->device->buflen - writepos;

		for (i = 0; i < todo; i++) {
			/* 8-bit WAV is unsigned */
			field = (*ibuf++ - 128);
			field += (*obuf - 128);
			if (field > 127) field = 127;
			else if (field < -128) field = -128;
			*obuf++ = field + 128;
		}
 
		if (todo < len) {
			todo = len - todo;
			obuf = dsb->device->buffer;

			for (i = 0; i < todo; i++) {
				/* 8-bit WAV is unsigned */
				field = (*ibuf++ - 128);
				field += (*obuf - 128);
				if (field > 127) field = 127;
				else if (field < -128) field = -128;
				*obuf++ = field + 128;
			}
		}
        } else {
		INT16	*ibufs, *obufs;

		ibufs = (INT16 *) ibuf;
		obufs = (INT16 *)(dsb->device->buffer + writepos);

		if ((writepos + len) <= dsb->device->buflen)
			todo = len / 2;
		else
			todo = (dsb->device->buflen - writepos) / 2;

		for (i = 0; i < todo; i++) {
			/* 16-bit WAV is signed */
			field = *ibufs++;
			field += *obufs;
			if (field > 32767) field = 32767;
			else if (field < -32768) field = -32768;
			*obufs++ = field;
		}

		if (todo < (len / 2)) {
			todo = (len / 2) - todo;
			obufs = (INT16 *)dsb->device->buffer;

			for (i = 0; i < todo; i++) {
				/* 16-bit WAV is signed */
				field = *ibufs++;
				field += *obufs;
				if (field > 32767) field = 32767;
				else if (field < -32768) field = -32768;
				*obufs++ = field;
			}
		}
        }

	if (dsb->leadin && (dsb->startpos > dsb->buf_mixpos) && (dsb->startpos <= dsb->buf_mixpos + ilen)) {
		/* HACK... leadin should be reset when the PLAY position reaches the startpos,
		 * not the MIX position... but if the sound buffer is bigger than our prebuffering
		 * (which must be the case for the streaming buffers that need this hack anyway)
		 * plus DS_HEL_MARGIN or equivalent, then this ought to work anyway. */
		dsb->leadin = FALSE;
	}

	dsb->buf_mixpos += ilen;

	if (dsb->buf_mixpos >= dsb->buflen) {
		if (dsb->playflags & DSBPLAY_LOOPING) {
			/* wrap */
			dsb->buf_mixpos %= dsb->buflen;
			if (dsb->leadin && (dsb->startpos <= dsb->buf_mixpos))
				dsb->leadin = FALSE; /* HACK: see above */
		} else if (dsb->buf_mixpos > dsb->buflen) {
			ERR("Mixpos (%u) past buflen (%u), capping...\n", dsb->buf_mixpos, dsb->buflen);
			dsb->buf_mixpos = dsb->buflen;
		}
	}

	return len;
}

/**
 * Calculate the distance between two buffer offsets, taking wraparound
 * into account.
 */
static inline DWORD DSOUND_BufPtrDiff(DWORD buflen, DWORD ptr1, DWORD ptr2)
{
	if (ptr1 >= ptr2) {
		return ptr1 - ptr2;
	} else {
		return buflen + ptr1 - ptr2;
	}
}

/**
 * Mix some frames from the given secondary buffer "dsb" into the device
 * primary buffer.
 *
 * dsb = the secondary buffer
 * playpos = the current play position in the device buffer (primary buffer)
 * writepos = the current safe-to-write position in the device buffer
 * mixlen = the maximum number of bytes in the primary buffer to mix, from the
 *          current writepos.
 *
 * Returns: the number of bytes beyond the writepos that were mixed.
 */
static DWORD DSOUND_MixOne(IDirectSoundBufferImpl *dsb, DWORD playpos, DWORD writepos, DWORD mixlen)
{
	/* The buffer's primary_mixpos may be before or after the the device
	 * buffer's mixpos, but both must be ahead of writepos. */
	DWORD primary_done;

	TRACE("(%p,%d,%d,%d)\n",dsb,playpos,writepos,mixlen);
	TRACE("writepos=%d, buf_mixpos=%d, primary_mixpos=%d, mixlen=%d\n", writepos, dsb->buf_mixpos, dsb->primary_mixpos, mixlen);
	TRACE("looping=%d, startpos=%d, leadin=%d, buflen=%d\n", dsb->playflags, dsb->startpos, dsb->leadin, dsb->buflen);

	/* calculate how much pre-buffering has already been done for this buffer */
	primary_done = DSOUND_BufPtrDiff(dsb->device->buflen, dsb->primary_mixpos, writepos);

	/* sanity */
	if(mixlen < primary_done)
	{
		/* Should *NEVER* happen */
		ERR("Fatal error. Under/Overflow? primary_done=%d, mixpos=%d, primary_mixpos=%d, writepos=%d, playpos=%d\n", primary_done,dsb->buf_mixpos,dsb->primary_mixpos, writepos, playpos);
		return 0;
	}

	/* take into acount already mixed data */
	mixlen = mixlen - primary_done;

	TRACE("mixlen (primary) = %i\n", mixlen);

	/* clip to valid length */
	mixlen = (dsb->buflen < mixlen) ? dsb->buflen : mixlen;

	TRACE("primary_done=%d, mixlen (buffer)=%d\n", primary_done, mixlen);

	/* mix more data */
	mixlen = DSOUND_MixInBuffer(dsb, dsb->primary_mixpos, mixlen);

	/* check for notification positions */
	if (dsb->dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY &&
	    dsb->state != STATE_STARTING) {
		DSOUND_CheckEvent(dsb, writepos, mixlen);
	}

	/* increase mix position */
	dsb->primary_mixpos += mixlen;
	dsb->primary_mixpos %= dsb->device->buflen;

	TRACE("new primary_mixpos=%d, mixed data len=%d, buffer left = %d\n",
		dsb->primary_mixpos, mixlen, (dsb->buflen - dsb->buf_mixpos));

	/* re-calculate the primary done */
	primary_done = DSOUND_BufPtrDiff(dsb->device->buflen, dsb->primary_mixpos, writepos);

	/* check if buffer should be considered complete */
	if (((dsb->buflen - dsb->buf_mixpos) < dsb->writelead) &&
	    !(dsb->playflags & DSBPLAY_LOOPING)) {

		TRACE("Buffer reached end. Stopped\n");

		dsb->state = STATE_STOPPED;
		dsb->buf_mixpos = 0;
		dsb->leadin = FALSE;
	}

	/* Report back the total prebuffered amount for this buffer */
	return primary_done;
}

/**
 * For a DirectSoundDevice, go through all the currently playing buffers and
 * mix them in to the device buffer.
 *
 * playpos = the current play position in the primary buffer
 * writepos = the current safe-to-write position in the primary buffer
 * mixlen = the maximum amount to mix into the primary buffer
 *          (beyond the current writepos)
 * recover = true if the sound device may have been reset and the write
 *           position in the device buffer changed
 * all_stopped = reports back if all buffers have stopped
 *
 * Returns:  the length beyond the writepos that was mixed to.
 */

static DWORD DSOUND_MixToPrimary(const DirectSoundDevice *device, DWORD playpos, DWORD writepos,
                                 DWORD mixlen, BOOL recover, BOOL *all_stopped)
{
	INT i, len;
	DWORD minlen = 0;
	IDirectSoundBufferImpl	*dsb;

	/* unless we find a running buffer, all have stopped */
	*all_stopped = TRUE;

	TRACE("(%d,%d,%d,%d)\n", playpos, writepos, mixlen, recover);
	for (i = 0; i < device->nrofbuffers; i++) {
		dsb = device->buffers[i];

		TRACE("MixToPrimary for %p, state=%d\n", dsb, dsb->state);

		if (dsb->buflen && dsb->state && !dsb->hwbuf) {
			TRACE("Checking %p, mixlen=%d\n", dsb, mixlen);
			EnterCriticalSection(&(dsb->lock));

			/* if buffer is stopping it is stopped now */
			if (dsb->state == STATE_STOPPING) {
				dsb->state = STATE_STOPPED;
				DSOUND_CheckEvent(dsb, 0, 0);
			} else {

				/* if recovering, reset the mix position */
				if ((dsb->state == STATE_STARTING) || recover) {
					dsb->primary_mixpos = writepos;
				}

				/* mix next buffer into the main buffer */
				len = DSOUND_MixOne(dsb, playpos, writepos, mixlen);

				/* if the buffer was starting, it must be playing now */
				if (dsb->state == STATE_STARTING)
					dsb->state = STATE_PLAYING;

				/* check if min-len should be initialized */
				if(minlen == 0) minlen = len;

			        /* record the minimum length mixed from all buffers */
				/* we only want to return the length which *all* buffers have mixed */
			        if(len != 0) minlen = (len < minlen) ? len : minlen;
			}

			if(dsb->state != STATE_STOPPED){
				*all_stopped = FALSE;
			}

			LeaveCriticalSection(&(dsb->lock));
		}
	}

	TRACE("Mixed at least %d from all buffers\n", minlen);

	return minlen;
}

/**
 * Add buffers to the emulated wave device system.
 *
 * device = The current dsound playback device
 * force = If TRUE, the function will buffer up as many frags as possible,
 *         even though and will ignore the actual state of the primary buffer.
 *
 * Returns:  None
 */

static void DSOUND_WaveQueue(DirectSoundDevice *device, BOOL force)
{
	DWORD prebuf_frags, wave_writepos, wave_fragpos, i;
	TRACE("(%p)\n", device);

	/* calculate the current wave frag position */
	wave_fragpos = (device->pwplay + device->pwqueue) % DS_HEL_FRAGS;

	/* calculte the current wave write position */
	wave_writepos = wave_fragpos * device->fraglen;

	TRACE("wave_fragpos = %i, wave_writepos = %i, pwqueue = %i, prebuf = %i\n",
		wave_fragpos, wave_writepos, device->pwqueue, device->prebuf);

	if(force == FALSE){
		/* check remaining prebuffered frags */
		prebuf_frags = DSOUND_BufPtrDiff(device->buflen, device->mixpos, wave_writepos);
		prebuf_frags = prebuf_frags / device->fraglen;
	}
	else{
		/* buffer the maximum amount of frags */
		prebuf_frags = device->prebuf;
	}

	/* limit to the queue we have left */
	if((prebuf_frags + device->pwqueue) > device->prebuf)
		prebuf_frags = device->prebuf - device->pwqueue;

	TRACE("prebuf_frags = %i\n", prebuf_frags);

	/* adjust queue */
	device->pwqueue += prebuf_frags;

	/* get out of CS when calling the wave system */
	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	/* queue up the new buffers */
	for(i=0; i<prebuf_frags; i++){
		TRACE("queueing wave buffer %i\n", wave_fragpos);
		waveOutWrite(device->hwo, device->pwave[wave_fragpos], sizeof(WAVEHDR));
		wave_fragpos++;
		wave_fragpos %= DS_HEL_FRAGS;
	}

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	TRACE("queue now = %i\n", device->pwqueue);
}

/**
 * Perform mixing for a Direct Sound device. That is, go through all the
 * secondary buffers (the sound bites currently playing) and mix them in
 * to the primary buffer (the device buffer).
 */
static void DSOUND_PerformMix(DirectSoundDevice *device)
{

	TRACE("(%p)\n", device);

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		BOOL recover = FALSE, all_stopped = FALSE;
		DWORD playpos, writepos, writelead, maxq, frag, prebuff_max, prebuff_left, size1, size2;
		LPVOID buf1, buf2;
		BOOL lock = (device->hwbuf && !(device->drvdesc.dwFlags & DSDDESC_DONTNEEDPRIMARYLOCK));
		int nfiller;

		/* the sound of silence */
		nfiller = device->pwfx->wBitsPerSample == 8 ? 128 : 0;

		/* get the position in the primary buffer */
		if (DSOUND_PrimaryGetPosition(device, &playpos, &writepos) != 0){
			LeaveCriticalSection(&(device->mixlock));
			return;
		}

		TRACE("primary playpos=%d, writepos=%d, clrpos=%d, mixpos=%d, buflen=%d\n",
		      playpos,writepos,device->playpos,device->mixpos,device->buflen);
		assert(device->playpos < device->buflen);

		/* wipe out just-played sound data */
		if (playpos < device->playpos) {
			buf1 = device->buffer + device->playpos;
			buf2 = device->buffer;
			size1 = device->buflen - device->playpos;
			size2 = playpos;
			if (lock)
				IDsDriverBuffer_Lock(device->hwbuf, &buf1, &size1, &buf2, &size2, device->playpos, size1+size2, 0);
			FillMemory(buf1, size1, nfiller);
			if (playpos && (!buf2 || !size2))
				FIXME("%d: (%d, %d)=>(%d, %d) There should be an additional buffer here!!\n", __LINE__, device->playpos, device->mixpos, playpos, writepos);
			FillMemory(buf2, size2, nfiller);
			if (lock)
				IDsDriverBuffer_Unlock(device->hwbuf, buf1, size1, buf2, size2);
		} else {
			buf1 = device->buffer + device->playpos;
			buf2 = NULL;
			size1 = playpos - device->playpos;
			size2 = 0;
			if (lock)
				IDsDriverBuffer_Lock(device->hwbuf, &buf1, &size1, &buf2, &size2, device->playpos, size1+size2, 0);
			FillMemory(buf1, size1, nfiller);
			if (buf2 && size2)
			{
				FIXME("%d: There should be no additional buffer here!!\n", __LINE__);
				FillMemory(buf2, size2, nfiller);
			}
			if (lock)
				IDsDriverBuffer_Unlock(device->hwbuf, buf1, size1, buf2, size2);
		}
		device->playpos = playpos;

		/* calc maximum prebuff */
		prebuff_max = (device->prebuf * device->fraglen);

		/* check how close we are to an underrun. It occurs when the writepos overtakes the mixpos */
		prebuff_left = DSOUND_BufPtrDiff(device->buflen, device->mixpos, playpos);

		writelead = DSOUND_BufPtrDiff(device->buflen, writepos, playpos);

		/* find the maximum we can prebuffer from current write position */
		maxq = prebuff_max - prebuff_left;
		maxq = (writelead < prebuff_max) ? (prebuff_max - writelead) : 0;

		TRACE("prebuff_left = %d, prebuff_max = %dx%d=%d, writelead=%d\n",
			prebuff_left, device->prebuf, device->fraglen, prebuff_max, writelead);

		/* check for underrun. underrun occurs when the write position passes the mix position */
		if((prebuff_left > prebuff_max) || (device->state == STATE_STOPPED) || (device->state == STATE_STARTING)){
			TRACE("Buffer starting or buffer underrun\n");

			/* recover mixing for all buffers */
			recover = TRUE;

			/* reset mix position to write position */
			device->mixpos = writepos;
		}

		if (lock)
			IDsDriverBuffer_Lock(device->hwbuf, &buf1, &size1, &buf2, &size2, device->mixpos, maxq, 0);

		/* do the mixing */
		frag = DSOUND_MixToPrimary(device, playpos, writepos, maxq, recover, &all_stopped);

		/* update the mix position, taking wrap-around into acount */
		device->mixpos = writepos + frag;
		device->mixpos %= device->buflen;

		if (lock)
		{
			DWORD frag2 = (frag > size1 ? frag - size1 : 0);
			frag -= frag2;
			if (frag2 > size2)
			{
				FIXME("Buffering too much! (%d, %d, %d, %d)\n", maxq, frag, size2, frag2 - size2);
				frag2 = size2;
			}
			IDsDriverBuffer_Unlock(device->hwbuf, buf1, frag, buf2, frag2);
		}

		/* update prebuff left */
		prebuff_left = DSOUND_BufPtrDiff(device->buflen, device->mixpos, playpos);

		/* check if have a whole fragment */
		if (prebuff_left >= device->fraglen){

			/* update the wave queue if using wave system */
			if(device->hwbuf == NULL){
				DSOUND_WaveQueue(device,TRUE);
			}

			/* buffers are full. start playing if applicable */
			if(device->state == STATE_STARTING){
				TRACE("started primary buffer\n");
				if(DSOUND_PrimaryPlay(device) != DS_OK){
					WARN("DSOUND_PrimaryPlay failed\n");
				}
				else{
					/* we are playing now */
					device->state = STATE_PLAYING;
				}
			}

			/* buffers are full. start stopping if applicable */
			if(device->state == STATE_STOPPED){
				TRACE("restarting primary buffer\n");
				if(DSOUND_PrimaryPlay(device) != DS_OK){
					WARN("DSOUND_PrimaryPlay failed\n");
				}
				else{
					/* start stopping again. as soon as there is no more data, it will stop */
					device->state = STATE_STOPPING;
				}
			}
		}

		/* if device was stopping, its for sure stopped when all buffers have stopped */
		else if((all_stopped == TRUE) && (device->state == STATE_STOPPING)){
			TRACE("All buffers have stopped. Stopping primary buffer\n");
			device->state = STATE_STOPPED;

			/* stop the primary buffer now */
			DSOUND_PrimaryStop(device);
		}

	} else {

		/* update the wave queue if using wave system */
		if(device->hwbuf == NULL){
			DSOUND_WaveQueue(device, TRUE);
		}

		/* in the DSSCL_WRITEPRIMARY mode, the app is totally in charge... */
		if (device->state == STATE_STARTING) {
			if (DSOUND_PrimaryPlay(device) != DS_OK)
				WARN("DSOUND_PrimaryPlay failed\n");
			else
				device->state = STATE_PLAYING;
		}
		else if (device->state == STATE_STOPPING) {
			if (DSOUND_PrimaryStop(device) != DS_OK)
				WARN("DSOUND_PrimaryStop failed\n");
			else
				device->state = STATE_STOPPED;
		}
	}

	LeaveCriticalSection(&(device->mixlock));
	/* **** */
}

void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD_PTR dwUser,
                           DWORD_PTR dw1, DWORD_PTR dw2)
{
        DirectSoundDevice * device = (DirectSoundDevice*)dwUser;
	DWORD start_time =  GetTickCount();
        DWORD end_time;
	TRACE("(%d,%d,0x%lx,0x%lx,0x%lx)\n",timerID,msg,dwUser,dw1,dw2);
        TRACE("entering at %d\n", start_time);

	if (DSOUND_renderer[device->drvdesc.dnDevNode] != device) {
		ERR("dsound died without killing us?\n");
		timeKillEvent(timerID);
		timeEndPeriod(DS_TIME_RES);
		return;
	}

	RtlAcquireResourceShared(&(device->buffer_list_lock), TRUE);

	if (device->ref)
		DSOUND_PerformMix(device);

	RtlReleaseResource(&(device->buffer_list_lock));

	end_time = GetTickCount();
	TRACE("completed processing at %d, duration = %d\n", end_time, end_time - start_time);
}

void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        DirectSoundDevice * device = (DirectSoundDevice*)dwUser;
	TRACE("(%p,%x,%x,%x,%x)\n",hwo,msg,dwUser,dw1,dw2);
	TRACE("entering at %d, msg=%08x(%s)\n", GetTickCount(), msg,
		msg==MM_WOM_DONE ? "MM_WOM_DONE" : msg==MM_WOM_CLOSE ? "MM_WOM_CLOSE" : 
		msg==MM_WOM_OPEN ? "MM_WOM_OPEN" : "UNKNOWN");

	/* check if packet completed from wave driver */
	if (msg == MM_WOM_DONE) {

		/* **** */
		EnterCriticalSection(&(device->mixlock));

		TRACE("done playing primary pos=%d\n", device->pwplay * device->fraglen);

		/* update playpos */
		device->pwplay++;
		device->pwplay %= DS_HEL_FRAGS;

		/* sanity */
		if(device->pwqueue == 0){
			ERR("Wave queue corrupted!\n");
		}

		/* update queue */
		device->pwqueue--;

		LeaveCriticalSection(&(device->mixlock));
		/* **** */
	}
	TRACE("completed\n");
}
