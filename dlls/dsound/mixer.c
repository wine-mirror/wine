/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
 * Copyright 2007 Peter Dons Tychsen
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

#include <assert.h>
#include <stdarg.h>
#include <math.h>	/* Insomnia - pow() function */

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "wingdi.h"
#include "mmreg.h"
#include "winternl.h"
#include "wine/debug.h"
#include "dsound.h"
#include "ks.h"
#include "ksmedia.h"
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

/** Convert a primary buffer position to a pointer position for device->mix_buffer
 * device: DirectSoundDevice for which to calculate
 * pos: Primary buffer position to converts
 * Returns: Offset for mix_buffer
 */
DWORD DSOUND_bufpos_to_mixpos(const DirectSoundDevice* device, DWORD pos)
{
    DWORD ret = pos * 32 / device->pwfx->wBitsPerSample;
    if (device->pwfx->wBitsPerSample == 32)
        ret *= 2;
    return ret;
}

/**
 * Recalculate the size for temporary buffer, and new writelead
 * Should be called when one of the following things occur:
 * - Primary buffer format is changed
 * - This buffer format (frequency) is changed
 */
void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb)
{
	DWORD ichannels = dsb->pwfx->nChannels;
	DWORD ochannels = dsb->device->pwfx->nChannels;
	WAVEFORMATEXTENSIBLE *pwfxe;
	BOOL ieee = FALSE;

	TRACE("(%p)\n",dsb);

	pwfxe = (WAVEFORMATEXTENSIBLE *) dsb->pwfx;

	if ((pwfxe->Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) || ((pwfxe->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	    && (IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))))
		ieee = TRUE;

	/* calculate the 10ms write lead */
	dsb->writelead = (dsb->freq / 100) * dsb->pwfx->nBlockAlign;

	dsb->freqAcc = 0;

	dsb->get_aux = ieee ? getbpp[4] : getbpp[dsb->pwfx->wBitsPerSample/8 - 1];
	dsb->put_aux = putbpp[dsb->device->pwfx->wBitsPerSample/8 - 1];

	dsb->get = dsb->get_aux;
	dsb->put = dsb->put_aux;

	if (ichannels == ochannels)
	{
		dsb->mix_channels = ichannels;
		if (ichannels > 32) {
			FIXME("Copying %u channels is unsupported, limiting to first 32\n", ichannels);
			dsb->mix_channels = 32;
		}
	}
	else if (ichannels == 1)
	{
		dsb->mix_channels = 1;
		dsb->put = put_mono2stereo;
	}
	else if (ochannels == 1)
	{
		dsb->mix_channels = 1;
		dsb->get = get_mono;
	}
	else
	{
		if (ichannels > 2)
			FIXME("Conversion from %u to %u channels is not implemented, falling back to stereo\n", ichannels, ochannels);
		dsb->mix_channels = 2;
	}
}

/**
 * Check for application callback requests for when the play position
 * reaches certain points.
 *
 * The offsets that will be triggered will be those between the recorded
 * "last played" position for the buffer (i.e. dsb->playpos) and "len" bytes
 * beyond that position.
 */
void DSOUND_CheckEvent(const IDirectSoundBufferImpl *dsb, DWORD playpos, int len)
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
		/* Windows does not seem to enforce this, and some apps rely */
		/* on that, so we can't stop there. */
		/*  */
		/* This also means we can't sort the entries by offset, */
		/* because DSBPN_OFFSETSTOP == -1 */
		if (offset == DSBPN_OFFSETSTOP) {
			if (dsb->state == STATE_STOPPED) {
				SetEvent(event->hEventNotify);
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
			}
                        continue;
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

static inline float get_current_sample(const IDirectSoundBufferImpl *dsb,
        DWORD mixpos, DWORD channel)
{
    if (mixpos >= dsb->buflen && !(dsb->playflags & DSBPLAY_LOOPING))
        return 0.0f;
    return dsb->get(dsb, mixpos % dsb->buflen, channel);
}

/**
 * Copy frames from the given input buffer to the given output buffer.
 * Translate 8 <-> 16 bits and mono <-> stereo
 */
static inline void cp_fields(IDirectSoundBufferImpl *dsb,
        UINT ostride, UINT count, float *freqAcc)
{
    DWORD ipos = dsb->sec_mixpos;
    UINT istride = dsb->pwfx->nBlockAlign, i;
    DWORD opos = 0;

    for (i = 0; i < count; ++i){
        DWORD channel;
        for (channel = 0; channel < dsb->mix_channels; channel++)
            dsb->put(dsb, opos, channel,
                get_current_sample(dsb, ipos, channel));
        *freqAcc += dsb->freqAdjust;
        ipos += ((DWORD)*freqAcc) * istride;
        *freqAcc -= truncf(*freqAcc);
        opos += ostride;
    }

    if (ipos >= dsb->buflen) {
        if (dsb->playflags & DSBPLAY_LOOPING)
            ipos %= dsb->buflen;
        else {
            ipos = 0;
            dsb->state = STATE_STOPPED;
        }
    }

    dsb->sec_mixpos = ipos;
}

/**
 * Calculate the distance between two buffer offsets, taking wraparound
 * into account.
 */
static inline DWORD DSOUND_BufPtrDiff(DWORD buflen, DWORD ptr1, DWORD ptr2)
{
/* If these asserts fail, the problem is not here, but in the underlying code */
	assert(ptr1 < buflen);
	assert(ptr2 < buflen);
	if (ptr1 >= ptr2) {
		return ptr1 - ptr2;
	} else {
		return buflen + ptr1 - ptr2;
	}
}
/**
 * Mix at most the given amount of data into the allocated temporary buffer
 * of the given secondary buffer, starting from the dsb's first currently
 * unsampled frame (writepos), translating frequency (pitch), stereo/mono
 * and bits-per-sample so that it is ideal for the primary buffer.
 * Doesn't perform any mixing - this is a straight copy/convert operation.
 *
 * dsb = the secondary buffer
 * writepos = Starting position of changed buffer
 * len = number of bytes to resample from writepos
 *
 * NOTE: writepos + len <= buflen. When called by mixer, MixOne makes sure of this.
 */
static void DSOUND_MixToTemporary(IDirectSoundBufferImpl *dsb, DWORD tmp_len)
{
	INT	oAdvance = dsb->device->pwfx->nBlockAlign;
	INT	size = tmp_len / oAdvance;

	if (dsb->device->tmp_buffer_len < tmp_len || !dsb->device->tmp_buffer)
	{
		dsb->device->tmp_buffer_len = tmp_len;
		if (dsb->device->tmp_buffer)
			dsb->device->tmp_buffer = HeapReAlloc(GetProcessHeap(), 0, dsb->device->tmp_buffer, tmp_len);
		else
			dsb->device->tmp_buffer = HeapAlloc(GetProcessHeap(), 0, tmp_len);
	}

	cp_fields(dsb, oAdvance, size, &dsb->freqAcc);
}

/** Apply volume to the given soundbuffer from (primary) position writepos and length len
 * Returns: NULL if no volume needs to be applied
 * or else a memory handle that holds 'len' volume adjusted buffer */
static LPBYTE DSOUND_MixerVol(const IDirectSoundBufferImpl *dsb, INT len)
{
	INT	i;
	BYTE	*bpc;
	INT16	*bps, *mems;
	DWORD vLeft, vRight;
	INT nChannels = dsb->device->pwfx->nChannels;
	LPBYTE mem = dsb->device->tmp_buffer;

	TRACE("(%p,%d)\n",dsb,len);
	TRACE("left = %x, right = %x\n", dsb->volpan.dwTotalLeftAmpFactor,
		dsb->volpan.dwTotalRightAmpFactor);

	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->volpan.lPan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->volpan.lVolume == 0)) &&
	     !(dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return NULL; /* Nothing to do */

	if (nChannels != 1 && nChannels != 2)
	{
		FIXME("There is no support for %d channels\n", nChannels);
		return NULL;
	}

	if (dsb->device->pwfx->wBitsPerSample != 8 && dsb->device->pwfx->wBitsPerSample != 16)
	{
		FIXME("There is no support for %d bpp\n", dsb->device->pwfx->wBitsPerSample);
		return NULL;
	}

	assert(dsb->device->tmp_buffer_len >= len && dsb->device->tmp_buffer);

	bpc = dsb->device->tmp_buffer;
	bps = (INT16 *)bpc;
	mems = (INT16 *)mem;
	vLeft = dsb->volpan.dwTotalLeftAmpFactor;
	if (nChannels > 1)
		vRight = dsb->volpan.dwTotalRightAmpFactor;
	else
		vRight = vLeft;

	switch (dsb->device->pwfx->wBitsPerSample) {
	case 8:
		/* 8-bit WAV is unsigned, but we need to operate */
		/* on signed data for this to work properly */
		for (i = 0; i < len-1; i+=2) {
			*(bpc++) = (((*(mem++) - 128) * vLeft) >> 16) + 128;
			*(bpc++) = (((*(mem++) - 128) * vRight) >> 16) + 128;
		}
		if (len % 2 == 1 && nChannels == 1)
			*(bpc++) = (((*(mem++) - 128) * vLeft) >> 16) + 128;
		break;
	case 16:
		/* 16-bit WAV is signed -- much better */
		for (i = 0; i < len-3; i += 4) {
			*(bps++) = (*(mems++) * vLeft) >> 16;
			*(bps++) = (*(mems++) * vRight) >> 16;
		}
		if (len % 4 == 2 && nChannels == 1)
			*(bps++) = ((INT)*(mems++) * vLeft) >> 16;
		break;
	}
	return dsb->device->tmp_buffer;
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
	INT len = fraglen;
	BYTE *ibuf, *volbuf;
	DWORD oldpos, mixbufpos;

	TRACE("sec_mixpos=%d/%d\n", dsb->sec_mixpos, dsb->buflen);
	TRACE("(%p,%d,%d)\n",dsb,writepos,fraglen);

	if (len % dsb->device->pwfx->nBlockAlign) {
		INT nBlockAlign = dsb->device->pwfx->nBlockAlign;
		ERR("length not a multiple of block size, len = %d, block size = %d\n", len, nBlockAlign);
		len -= len % nBlockAlign; /* data alignment */
	}

	/* Resample buffer to temporary buffer specifically allocated for this purpose, if needed */
	oldpos = dsb->sec_mixpos;

	DSOUND_MixToTemporary(dsb, len);
	ibuf = dsb->device->tmp_buffer;

	/* Apply volume if needed */
	volbuf = DSOUND_MixerVol(dsb, len);
	if (volbuf)
		ibuf = volbuf;

	mixbufpos = DSOUND_bufpos_to_mixpos(dsb->device, writepos);
	/* Now mix the temporary buffer into the devices main buffer */
	if ((writepos + len) <= dsb->device->buflen)
		dsb->device->mixfunction(ibuf, dsb->device->mix_buffer + mixbufpos, len);
	else
	{
		DWORD todo = dsb->device->buflen - writepos;
		dsb->device->mixfunction(ibuf, dsb->device->mix_buffer + mixbufpos, todo);
		dsb->device->mixfunction(ibuf + todo, dsb->device->mix_buffer, len - todo);
	}

	/* check for notification positions */
	if (dsb->dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY &&
	    dsb->state != STATE_STARTING) {
		INT ilen = DSOUND_BufPtrDiff(dsb->buflen, dsb->sec_mixpos, oldpos);
		DSOUND_CheckEvent(dsb, oldpos, ilen);
	}

	/* increase mix position */
	dsb->primary_mixpos += len;
	dsb->primary_mixpos %= dsb->device->buflen;

	return len;
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
static DWORD DSOUND_MixOne(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD mixlen)
{
	/* The buffer's primary_mixpos may be before or after the device
	 * buffer's mixpos, but both must be ahead of writepos. */
	DWORD primary_done;

	TRACE("(%p,%d,%d)\n",dsb,writepos,mixlen);
	TRACE("writepos=%d, primary_mixpos=%d, mixlen=%d\n", writepos, dsb->primary_mixpos, mixlen);
	TRACE("looping=%d, leadin=%d\n", dsb->playflags, dsb->leadin);

	/* If leading in, only mix about 20 ms, and 'skip' mixing the rest, for more fluid pointer advancement */
	if (dsb->leadin && dsb->state == STATE_STARTING)
	{
		if (mixlen > 2 * dsb->device->fraglen)
		{
			dsb->primary_mixpos += mixlen - 2 * dsb->device->fraglen;
			dsb->primary_mixpos %= dsb->device->buflen;
		}
	}
	dsb->leadin = FALSE;

	/* calculate how much pre-buffering has already been done for this buffer */
	primary_done = DSOUND_BufPtrDiff(dsb->device->buflen, dsb->primary_mixpos, writepos);

	/* sanity */
	if(mixlen < primary_done)
	{
		/* Should *NEVER* happen */
		ERR("Fatal error. Under/Overflow? primary_done=%d, mixpos=%d/%d, primary_mixpos=%d, writepos=%d, mixlen=%d\n", primary_done,dsb->sec_mixpos, dsb->buflen, dsb->primary_mixpos, writepos, mixlen);
		dsb->primary_mixpos = writepos + mixlen;
		dsb->primary_mixpos %= dsb->device->buflen;
		return mixlen;
	}

	/* take into account already mixed data */
	mixlen -= primary_done;

	TRACE("primary_done=%d, mixlen (primary) = %i\n", primary_done, mixlen);

	if (!mixlen)
		return primary_done;

	/* First try to mix to the end of the buffer if possible
	 * Theoretically it would allow for better optimization
	*/
	DSOUND_MixInBuffer(dsb, dsb->primary_mixpos, mixlen);

	/* re-calculate the primary done */
	primary_done = DSOUND_BufPtrDiff(dsb->device->buflen, dsb->primary_mixpos, writepos);

	TRACE("new primary_mixpos=%d, total mixed data=%d\n", dsb->primary_mixpos, primary_done);

	/* Report back the total prebuffered amount for this buffer */
	return primary_done;
}

/**
 * For a DirectSoundDevice, go through all the currently playing buffers and
 * mix them in to the device buffer.
 *
 * writepos = the current safe-to-write position in the primary buffer
 * mixlen = the maximum amount to mix into the primary buffer
 *          (beyond the current writepos)
 * recover = true if the sound device may have been reset and the write
 *           position in the device buffer changed
 * all_stopped = reports back if all buffers have stopped
 *
 * Returns:  the length beyond the writepos that was mixed to.
 */

static DWORD DSOUND_MixToPrimary(const DirectSoundDevice *device, DWORD writepos, DWORD mixlen, BOOL recover, BOOL *all_stopped)
{
	INT i, len;
	DWORD minlen = 0;
	IDirectSoundBufferImpl	*dsb;

	/* unless we find a running buffer, all have stopped */
	*all_stopped = TRUE;

	TRACE("(%d,%d,%d)\n", writepos, mixlen, recover);
	for (i = 0; i < device->nrofbuffers; i++) {
		dsb = device->buffers[i];

		TRACE("MixToPrimary for %p, state=%d\n", dsb, dsb->state);

		if (dsb->buflen && dsb->state) {
			TRACE("Checking %p, mixlen=%d\n", dsb, mixlen);
			RtlAcquireResourceShared(&dsb->lock, TRUE);
			/* if buffer is stopping it is stopped now */
			if (dsb->state == STATE_STOPPING) {
				dsb->state = STATE_STOPPED;
				DSOUND_CheckEvent(dsb, 0, 0);
			} else if (dsb->state != STATE_STOPPED) {

				/* if recovering, reset the mix position */
				if ((dsb->state == STATE_STARTING) || recover) {
					dsb->primary_mixpos = writepos;
				}

				/* if the buffer was starting, it must be playing now */
				if (dsb->state == STATE_STARTING)
					dsb->state = STATE_PLAYING;

				/* mix next buffer into the main buffer */
				len = DSOUND_MixOne(dsb, writepos, mixlen);

				if (!minlen) minlen = len;

				/* record the minimum length mixed from all buffers */
				/* we only want to return the length which *all* buffers have mixed */
				else if (len) minlen = (len < minlen) ? len : minlen;

				*all_stopped = FALSE;
			}
			RtlReleaseResource(&dsb->lock);
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
	DWORD prebuf_frames, buf_offs_bytes, wave_fragpos;
	int prebuf_frags;
	BYTE *buffer;
	HRESULT hr;

	TRACE("(%p)\n", device);

	/* calculate the current wave frag position */
	wave_fragpos = (device->pwplay + device->pwqueue) % device->helfrags;

	/* calculate the current wave write position */
	buf_offs_bytes = wave_fragpos * device->fraglen;

	TRACE("wave_fragpos = %i, buf_offs_bytes = %i, pwqueue = %i, prebuf = %i\n",
		wave_fragpos, buf_offs_bytes, device->pwqueue, device->prebuf);

	if (!force)
	{
		/* check remaining prebuffered frags */
		prebuf_frags = device->mixpos / device->fraglen;
		if (prebuf_frags == device->helfrags)
			--prebuf_frags;
		TRACE("wave_fragpos = %d, mixpos_frags = %d\n", wave_fragpos, prebuf_frags);
		if (prebuf_frags < wave_fragpos)
			prebuf_frags += device->helfrags;
		prebuf_frags -= wave_fragpos;
		TRACE("wanted prebuf_frags = %d\n", prebuf_frags);
	}
	else
		/* buffer the maximum amount of frags */
		prebuf_frags = device->prebuf;

	/* limit to the queue we have left */
	if ((prebuf_frags + device->pwqueue) > device->prebuf)
		prebuf_frags = device->prebuf - device->pwqueue;

	TRACE("prebuf_frags = %i\n", prebuf_frags);

	if(!prebuf_frags)
		return;

	/* adjust queue */
	device->pwqueue += prebuf_frags;

	prebuf_frames = ((prebuf_frags + wave_fragpos > device->helfrags) ?
			(device->helfrags - wave_fragpos) :
			(prebuf_frags)) * device->fraglen / device->pwfx->nBlockAlign;

	hr = IAudioRenderClient_GetBuffer(device->render, prebuf_frames, &buffer);
	if(FAILED(hr)){
		WARN("GetBuffer failed: %08x\n", hr);
		return;
	}

	memcpy(buffer, device->buffer + buf_offs_bytes,
			prebuf_frames * device->pwfx->nBlockAlign);

	hr = IAudioRenderClient_ReleaseBuffer(device->render, prebuf_frames, 0);
	if(FAILED(hr)){
		WARN("ReleaseBuffer failed: %08x\n", hr);
		return;
	}

	/* check if anything wrapped */
	prebuf_frags = prebuf_frags + wave_fragpos - device->helfrags;
	if(prebuf_frags > 0){
		prebuf_frames = prebuf_frags * device->fraglen / device->pwfx->nBlockAlign;

		hr = IAudioRenderClient_GetBuffer(device->render, prebuf_frames, &buffer);
		if(FAILED(hr)){
			WARN("GetBuffer failed: %08x\n", hr);
			return;
		}

		memcpy(buffer, device->buffer, prebuf_frames * device->pwfx->nBlockAlign);

		hr = IAudioRenderClient_ReleaseBuffer(device->render, prebuf_frames, 0);
		if(FAILED(hr)){
			WARN("ReleaseBuffer failed: %08x\n", hr);
			return;
		}
	}

	TRACE("queue now = %i\n", device->pwqueue);
}

/**
 * Perform mixing for a Direct Sound device. That is, go through all the
 * secondary buffers (the sound bites currently playing) and mix them in
 * to the primary buffer (the device buffer).
 */
static void DSOUND_PerformMix(DirectSoundDevice *device)
{
	UINT64 clock_pos, clock_freq, pos_bytes;
	UINT delta_frags;
	HRESULT hr;

	TRACE("(%p)\n", device);

	/* **** */
	EnterCriticalSection(&device->mixlock);

	hr = IAudioClock_GetFrequency(device->clock, &clock_freq);
	if(FAILED(hr)){
		WARN("GetFrequency failed: %08x\n", hr);
        LeaveCriticalSection(&device->mixlock);
		return;
	}

	hr = IAudioClock_GetPosition(device->clock, &clock_pos, NULL);
	if(FAILED(hr)){
		WARN("GetCurrentPadding failed: %08x\n", hr);
        LeaveCriticalSection(&device->mixlock);
		return;
	}

	pos_bytes = (clock_pos * device->pwfx->nSamplesPerSec * device->pwfx->nBlockAlign) / clock_freq;

	delta_frags = (pos_bytes - device->last_pos_bytes) / device->fraglen;
	if(delta_frags > 0){
		device->pwplay += delta_frags;
		device->pwplay %= device->helfrags;
		device->pwqueue -= delta_frags;
		device->last_pos_bytes = pos_bytes - (pos_bytes % device->fraglen);
	}

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		BOOL recover = FALSE, all_stopped = FALSE;
		DWORD playpos, writepos, writelead, maxq, frag, prebuff_max, prebuff_left, size1, size2, mixplaypos, mixplaypos2;
		LPVOID buf1, buf2;
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

		mixplaypos = DSOUND_bufpos_to_mixpos(device, device->playpos);
		mixplaypos2 = DSOUND_bufpos_to_mixpos(device, playpos);

		/* calc maximum prebuff */
		prebuff_max = (device->prebuf * device->fraglen);
		if (playpos + prebuff_max >= device->helfrags * device->fraglen)
			prebuff_max += device->buflen - device->helfrags * device->fraglen;

		/* check how close we are to an underrun. It occurs when the writepos overtakes the mixpos */
		prebuff_left = DSOUND_BufPtrDiff(device->buflen, device->mixpos, playpos);
		writelead = DSOUND_BufPtrDiff(device->buflen, writepos, playpos);

		/* check for underrun. underrun occurs when the write position passes the mix position
		 * also wipe out just-played sound data */
		if((prebuff_left > prebuff_max) || (device->state == STATE_STOPPED) || (device->state == STATE_STARTING)){
			if (device->state == STATE_STOPPING || device->state == STATE_PLAYING)
				WARN("Probable buffer underrun\n");
			else TRACE("Buffer starting or buffer underrun\n");

			/* recover mixing for all buffers */
			recover = TRUE;

			/* reset mix position to write position */
			device->mixpos = writepos;

			ZeroMemory(device->mix_buffer, device->mix_buffer_len);
			ZeroMemory(device->buffer, device->buflen);
		} else if (playpos < device->playpos) {
			buf1 = device->buffer + device->playpos;
			buf2 = device->buffer;
			size1 = device->buflen - device->playpos;
			size2 = playpos;
			FillMemory(device->mix_buffer + mixplaypos, device->mix_buffer_len - mixplaypos, 0);
			FillMemory(device->mix_buffer, mixplaypos2, 0);
			FillMemory(buf1, size1, nfiller);
			if (playpos && (!buf2 || !size2))
				FIXME("%d: (%d, %d)=>(%d, %d) There should be an additional buffer here!!\n", __LINE__, device->playpos, device->mixpos, playpos, writepos);
			FillMemory(buf2, size2, nfiller);
		} else {
			buf1 = device->buffer + device->playpos;
			buf2 = NULL;
			size1 = playpos - device->playpos;
			size2 = 0;
			FillMemory(device->mix_buffer + mixplaypos, mixplaypos2 - mixplaypos, 0);
			FillMemory(buf1, size1, nfiller);
		}
		device->playpos = playpos;

		/* find the maximum we can prebuffer from current write position */
		maxq = (writelead < prebuff_max) ? (prebuff_max - writelead) : 0;

		TRACE("prebuff_left = %d, prebuff_max = %dx%d=%d, writelead=%d\n",
			prebuff_left, device->prebuf, device->fraglen, prebuff_max, writelead);

		/* do the mixing */
		frag = DSOUND_MixToPrimary(device, writepos, maxq, recover, &all_stopped);

		if (frag + writepos > device->buflen)
		{
			DWORD todo = device->buflen - writepos;
			device->normfunction(device->mix_buffer + DSOUND_bufpos_to_mixpos(device, writepos), device->buffer + writepos, todo);
			device->normfunction(device->mix_buffer, device->buffer, frag - todo);
		}
		else
			device->normfunction(device->mix_buffer + DSOUND_bufpos_to_mixpos(device, writepos), device->buffer + writepos, frag);

		/* update the mix position, taking wrap-around into account */
		device->mixpos = writepos + frag;
		device->mixpos %= device->buflen;

		/* update prebuff left */
		prebuff_left = DSOUND_BufPtrDiff(device->buflen, device->mixpos, playpos);

		/* check if have a whole fragment */
		if (prebuff_left >= device->fraglen){

			/* update the wave queue */
			DSOUND_WaveQueue(device, FALSE);

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

		DSOUND_WaveQueue(device, TRUE);

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

	RtlAcquireResourceShared(&(device->buffer_list_lock), TRUE);

	if (device->ref)
		DSOUND_PerformMix(device);

	RtlReleaseResource(&(device->buffer_list_lock));

	end_time = GetTickCount();
	TRACE("completed processing at %d, duration = %d\n", end_time, end_time - start_time);
}
