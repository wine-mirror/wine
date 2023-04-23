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

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "wingdi.h"
#include "mmreg.h"
#include "wine/debug.h"
#include "dsound.h"
#include "ks.h"
#include "ksmedia.h"
#include "dsound_private.h"
#include "fir.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan)
{
	double temp;
	TRACE("(%p)\n",volpan);

	TRACE("Vol=%ld Pan=%ld\n", volpan->lVolume, volpan->lPan);
	/* the AmpFactors are expressed in 16.16 fixed point */

	/* FIXME: use calculated vol and pan ampfactors */
	temp = (double) (volpan->lVolume - (volpan->lPan > 0 ? volpan->lPan : 0));
	volpan->dwTotalAmpFactor[0] = (ULONG) (pow(2.0, temp / 600.0) * 0xffff);
	temp = (double) (volpan->lVolume + (volpan->lPan < 0 ? volpan->lPan : 0));
	volpan->dwTotalAmpFactor[1] = (ULONG) (pow(2.0, temp / 600.0) * 0xffff);

	TRACE("left = %lx, right = %lx\n", volpan->dwTotalAmpFactor[0], volpan->dwTotalAmpFactor[1]);
}

void DSOUND_AmpFactorToVolPan(PDSVOLUMEPAN volpan)
{
    double left,right;
    TRACE("(%p)\n",volpan);

    TRACE("left=%lx, right=%lx\n",volpan->dwTotalAmpFactor[0],volpan->dwTotalAmpFactor[1]);
    if (volpan->dwTotalAmpFactor[0]==0)
        left=-10000;
    else
        left=600 * log(((double)volpan->dwTotalAmpFactor[0]) / 0xffff) / log(2);
    if (volpan->dwTotalAmpFactor[1]==0)
        right=-10000;
    else
        right=600 * log(((double)volpan->dwTotalAmpFactor[1]) / 0xffff) / log(2);
    if (left<right)
        volpan->lVolume=right;
    else
        volpan->lVolume=left;
    if (volpan->lVolume < -10000)
        volpan->lVolume=-10000;
    volpan->lPan=right-left;
    if (volpan->lPan < -10000)
        volpan->lPan=-10000;

    TRACE("Vol=%ld Pan=%ld\n", volpan->lVolume, volpan->lPan);
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
	LONG64 oldFreqAdjustDen = dsb->freqAdjustDen;
	WAVEFORMATEXTENSIBLE *pwfxe;
	BOOL ieee = FALSE;

	TRACE("(%p)\n",dsb);

	pwfxe = (WAVEFORMATEXTENSIBLE *) dsb->pwfx;
	dsb->freqAdjustNum = dsb->freq;
	dsb->freqAdjustDen = dsb->device->pwfx->nSamplesPerSec;

	if ((pwfxe->Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) || ((pwfxe->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	    && (IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))))
		ieee = TRUE;

	/**
	 * Recalculate FIR step and gain.
	 *
	 * firstep says how many points of the FIR exist per one
	 * sample in the secondary buffer. firgain specifies what
	 * to multiply the FIR output by in order to attenuate it correctly.
	 */
	if (dsb->freqAdjustNum / dsb->freqAdjustDen > 0) {
		/**
		 * Yes, round it a bit to make sure that the
		 * linear interpolation factor never changes.
		 */
		dsb->firstep = fir_step * dsb->freqAdjustDen / dsb->freqAdjustNum;
	} else {
		dsb->firstep = fir_step;
	}
	dsb->firgain = (float)dsb->firstep / fir_step;

	/* calculate the 10ms write lead */
	dsb->writelead = (dsb->freq / 100) * dsb->pwfx->nBlockAlign;
	dsb->maxwritelead = (DSBFREQUENCY_MAX / 100) * dsb->pwfx->nBlockAlign;

	if (oldFreqAdjustDen)
		dsb->freqAccNum = (dsb->freqAccNum * dsb->freqAdjustDen + oldFreqAdjustDen / 2) / oldFreqAdjustDen;

	dsb->get_aux = ieee ? getbpp[4] : getbpp[dsb->pwfx->wBitsPerSample/8 - 1];
	dsb->put_aux = putieee32;

	dsb->get = dsb->get_aux;
	dsb->put = dsb->put_aux;

	if (ichannels == ochannels)
	{
		dsb->mix_channels = ichannels;
		if (ichannels > 32) {
			FIXME("Copying %lu channels is unsupported, limiting to first 32\n", ichannels);
			dsb->mix_channels = 32;
		}
	}
	else if (ichannels == 1)
	{
		dsb->mix_channels = 1;

		if (ochannels == 2)
			dsb->put = put_mono2stereo;
		else if (ochannels == 4)
			dsb->put = put_mono2quad;
		else if (ochannels == 6)
			dsb->put = put_mono2surround51;
	}
	else if (ochannels == 1)
	{
		dsb->mix_channels = 1;
		dsb->get = get_mono;
	}
	else if (ichannels == 2 && ochannels == 4)
	{
		dsb->mix_channels = 2;
		dsb->put = put_stereo2quad;
	}
	else if (ichannels == 2 && ochannels == 6)
	{
		dsb->mix_channels = 2;
		dsb->put = put_stereo2surround51;
	}
	else if (ichannels == 6 && ochannels == 2)
	{
		dsb->mix_channels = 6;
		dsb->put = put_surround512stereo;
		dsb->put_aux = putieee32_sum;
	}
	else if (ichannels == 8 && ochannels == 2)
	{
		dsb->mix_channels = 8;
		dsb->put = put_surround712stereo;
		dsb->put_aux = putieee32_sum;
	}
	else if (ichannels == 4 && ochannels == 2)
	{
		dsb->mix_channels = 4;
		dsb->put = put_quad2stereo;
		dsb->put_aux = putieee32_sum;
	}
	else
	{
		if (ichannels > 2)
			FIXME("Conversion from %lu to %lu channels is not implemented, falling back to stereo\n", ichannels, ochannels);
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
    int first, left, right, check;

    if(dsb->nrofnotifies == 0)
        return;

    if(dsb->state == STATE_STOPPED){
        TRACE("Stopped...\n");
        /* DSBPN_OFFSETSTOP notifies are always at the start of the sorted array */
        for(left = 0; left < dsb->nrofnotifies; ++left){
            if(dsb->notifies[left].dwOffset != DSBPN_OFFSETSTOP)
                break;

            TRACE("Signalling %p\n", dsb->notifies[left].hEventNotify);
            SetEvent(dsb->notifies[left].hEventNotify);
        }
        return;
    }

    for(first = 0; first < dsb->nrofnotifies && dsb->notifies[first].dwOffset == DSBPN_OFFSETSTOP; ++first)
        ;

    if(first == dsb->nrofnotifies)
        return;

    check = left = first;
    right = dsb->nrofnotifies - 1;

    /* find leftmost notify that is greater than playpos */
    while(left != right){
        check = left + (right - left) / 2;
        if(dsb->notifies[check].dwOffset < playpos)
            left = check + 1;
        else if(dsb->notifies[check].dwOffset > playpos)
            right = check;
        else{
            left = check;
            break;
        }
    }

    TRACE("Not stopped: first notify: %u (%lu), left notify: %u (%lu), range: [%lu,%lu)\n",
            first, dsb->notifies[first].dwOffset,
            left, dsb->notifies[left].dwOffset,
            playpos, (playpos + len) % dsb->buflen);

    /* send notifications in range */
    if(dsb->notifies[left].dwOffset >= playpos){
        for(check = left; check < dsb->nrofnotifies; ++check){
            if(dsb->notifies[check].dwOffset >= playpos + len)
                break;

            TRACE("Signalling %p (%lu)\n", dsb->notifies[check].hEventNotify, dsb->notifies[check].dwOffset);
            SetEvent(dsb->notifies[check].hEventNotify);
        }
    }

    if(playpos + len > dsb->buflen){
        for(check = first; check < left; ++check){
            if(dsb->notifies[check].dwOffset >= (playpos + len) % dsb->buflen)
                break;

            TRACE("Signalling %p (%lu)\n", dsb->notifies[check].hEventNotify, dsb->notifies[check].dwOffset);
            SetEvent(dsb->notifies[check].hEventNotify);
        }
    }
}

static inline float get_current_sample(const IDirectSoundBufferImpl *dsb,
        BYTE *buffer, DWORD buflen, DWORD mixpos, DWORD channel)
{
    if (mixpos >= buflen && !(dsb->playflags & DSBPLAY_LOOPING))
        return 0.0f;
    return dsb->get(dsb, buffer + (mixpos % buflen), channel);
}

static UINT cp_fields_noresample(IDirectSoundBufferImpl *dsb, UINT count)
{
    UINT istride = dsb->pwfx->nBlockAlign;
    UINT ostride = dsb->device->pwfx->nChannels * sizeof(float);
    UINT committed_samples = 0;
    DWORD channel, i;

    if (!secondarybuffer_is_audible(dsb))
        return count;

    if(dsb->use_committed) {
        committed_samples = (dsb->writelead - dsb->committed_mixpos) / istride;
        committed_samples = committed_samples <= count ? committed_samples : count;
    }

    for (i = 0; i < committed_samples; i++)
        for (channel = 0; channel < dsb->mix_channels; channel++)
            dsb->put(dsb, i * ostride, channel, get_current_sample(dsb, dsb->committedbuff,
                dsb->writelead, dsb->committed_mixpos + i * istride, channel));

    for (; i < count; i++)
        for (channel = 0; channel < dsb->mix_channels; channel++)
            dsb->put(dsb, i * ostride, channel, get_current_sample(dsb, dsb->buffer->memory,
                dsb->buflen, dsb->sec_mixpos + i * istride, channel));

    return count;
}

static UINT cp_fields_resample(IDirectSoundBufferImpl *dsb, UINT count, LONG64 *freqAccNum)
{
    UINT i, channel;
    UINT istride = dsb->pwfx->nBlockAlign;
    UINT ostride = dsb->device->pwfx->nChannels * sizeof(float);
    UINT committed_samples = 0;

    LONG64 freqAcc_start = *freqAccNum;
    LONG64 freqAcc_end = freqAcc_start + count * dsb->freqAdjustNum;
    UINT dsbfirstep = dsb->firstep;
    UINT channels = dsb->mix_channels;
    UINT max_ipos = (freqAcc_start + count * dsb->freqAdjustNum) / dsb->freqAdjustDen;

    UINT fir_cachesize = (fir_len + dsbfirstep - 2) / dsbfirstep;
    UINT required_input = max_ipos + fir_cachesize;
    float *intermediate, *fir_copy, *itmp;

    DWORD len = required_input * channels;
    len += fir_cachesize;
    len *= sizeof(float);

    *freqAccNum = freqAcc_end % dsb->freqAdjustDen;

    if (!secondarybuffer_is_audible(dsb))
        return max_ipos;

    if (!dsb->device->cp_buffer) {
        dsb->device->cp_buffer = malloc(len);
        dsb->device->cp_buffer_len = len;
    } else if (len > dsb->device->cp_buffer_len) {
        dsb->device->cp_buffer = realloc(dsb->device->cp_buffer, len);
        dsb->device->cp_buffer_len = len;
    }

    fir_copy = dsb->device->cp_buffer;
    intermediate = fir_copy + fir_cachesize;

    if(dsb->use_committed) {
        committed_samples = (dsb->writelead - dsb->committed_mixpos) / istride;
        committed_samples = committed_samples <= required_input ? committed_samples : required_input;
    }

    /* Important: this buffer MUST be non-interleaved
     * if you want -msse3 to have any effect.
     * This is good for CPU cache effects, too.
     */
    itmp = intermediate;
    for (channel = 0; channel < channels; channel++) {
        for (i = 0; i < committed_samples; i++)
            *(itmp++) = get_current_sample(dsb, dsb->committedbuff,
                dsb->writelead, dsb->committed_mixpos + i * istride, channel);
        for (; i < required_input; i++)
            *(itmp++) = get_current_sample(dsb, dsb->buffer->memory,
                    dsb->buflen, dsb->sec_mixpos + i * istride, channel);
    }

    for(i = 0; i < count; ++i) {
        UINT int_fir_steps = (freqAcc_start + i * dsb->freqAdjustNum) * dsbfirstep / dsb->freqAdjustDen;
        float total_fir_steps = (freqAcc_start + i * dsb->freqAdjustNum) * dsbfirstep / (float)dsb->freqAdjustDen;
        UINT ipos = int_fir_steps / dsbfirstep;

        UINT idx = (ipos + 1) * dsbfirstep - int_fir_steps - 1;
        float rem = int_fir_steps + 1.0 - total_fir_steps;

        int fir_used = 0;
        while (idx < fir_len - 1) {
            fir_copy[fir_used++] = fir[idx] * (1.0 - rem) + fir[idx + 1] * rem;
            idx += dsb->firstep;
        }

        assert(fir_used <= fir_cachesize);
        assert(ipos + fir_used <= required_input);

        for (channel = 0; channel < dsb->mix_channels; channel++) {
            int j;
            float sum = 0.0;
            float* cache = &intermediate[channel * required_input + ipos];
            for (j = 0; j < fir_used; j++)
                sum += fir_copy[j] * cache[j];
            dsb->put(dsb, i * ostride, channel, sum * dsb->firgain);
        }
    }

    return max_ipos;
}

static void cp_fields(IDirectSoundBufferImpl *dsb, UINT count, LONG64 *freqAccNum)
{
    DWORD ipos, adv;

    if (dsb->freqAdjustNum == dsb->freqAdjustDen)
        adv = cp_fields_noresample(dsb, count); /* *freqAccNum is unmodified */
    else
        adv = cp_fields_resample(dsb, count, freqAccNum);

    ipos = dsb->sec_mixpos + adv * dsb->pwfx->nBlockAlign;
    if (ipos >= dsb->buflen) {
        if (dsb->playflags & DSBPLAY_LOOPING)
            ipos %= dsb->buflen;
        else {
            ipos = 0;
            dsb->state = STATE_STOPPED;
        }
    }

    dsb->sec_mixpos = ipos;

    if(dsb->use_committed) {
        dsb->committed_mixpos += adv * dsb->pwfx->nBlockAlign;
        if(dsb->committed_mixpos >= dsb->writelead)
            dsb->use_committed = FALSE;
    }
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
static void DSOUND_MixToTemporary(IDirectSoundBufferImpl *dsb, DWORD frames)
{
	UINT size_bytes = frames * sizeof(float) * dsb->device->pwfx->nChannels;
	HRESULT hr;
	int i;

	if (dsb->device->tmp_buffer_len < size_bytes || !dsb->device->tmp_buffer)
	{
		dsb->device->tmp_buffer_len = size_bytes;
		dsb->device->tmp_buffer = realloc(dsb->device->tmp_buffer, size_bytes);
	}
	if(dsb->put_aux == putieee32_sum)
		memset(dsb->device->tmp_buffer, 0, dsb->device->tmp_buffer_len);

	cp_fields(dsb, frames, &dsb->freqAccNum);

	if (size_bytes > 0) {
		for (i = 0; i < dsb->num_filters; i++) {
			if (dsb->filters[i].inplace) {
				hr = IMediaObjectInPlace_Process(dsb->filters[i].inplace, size_bytes, (BYTE*)dsb->device->tmp_buffer, 0, DMO_INPLACE_NORMAL);

				if (FAILED(hr))
					WARN("IMediaObjectInPlace_Process failed for filter %u\n", i);
			} else
				WARN("filter %u has no inplace object - unsupported\n", i);
		}
	}
}

static void DSOUND_MixerVol(const IDirectSoundBufferImpl *dsb, INT frames)
{
	INT	i;
	float vols[DS_MAX_CHANNELS];
	UINT channels = dsb->device->pwfx->nChannels, chan;

	TRACE("(%p,%d)\n",dsb,frames);
	TRACE("left = %lx, right = %lx\n", dsb->volpan.dwTotalAmpFactor[0],
		dsb->volpan.dwTotalAmpFactor[1]);

	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->volpan.lPan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->volpan.lVolume == 0)) &&
	     !(dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return; /* Nothing to do */

	if (channels > DS_MAX_CHANNELS)
	{
		FIXME("There is no support for %u channels\n", channels);
		return;
	}

	for (i = 0; i < channels; ++i)
		vols[i] = dsb->volpan.dwTotalAmpFactor[i] / ((float)0xFFFF);

	for(i = 0; i < frames; ++i){
		for(chan = 0; chan < channels; ++chan){
			dsb->device->tmp_buffer[i * channels + chan] *= vols[chan];
		}
	}
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
 * fraglen = number of bytes to mix
 */
static DWORD DSOUND_MixInBuffer(IDirectSoundBufferImpl *dsb, float *mix_buffer, DWORD frames)
{
	float *ibuf;
	DWORD oldpos;

	TRACE("sec_mixpos=%ld/%ld\n", dsb->sec_mixpos, dsb->buflen);
	TRACE("(%p, frames=%ld)\n",dsb,frames);

	/* Resample buffer to temporary buffer specifically allocated for this purpose, if needed */
	oldpos = dsb->sec_mixpos;
	DSOUND_MixToTemporary(dsb, frames);
	ibuf = dsb->device->tmp_buffer;

	if (secondarybuffer_is_audible(dsb)) {
		/* Apply volume if needed */
		DSOUND_MixerVol(dsb, frames);

		mixieee32(ibuf, mix_buffer, frames * dsb->device->pwfx->nChannels);
	}

	/* check for notification positions */
	if (dsb->dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY &&
	    dsb->state != STATE_STARTING) {
		INT ilen = DSOUND_BufPtrDiff(dsb->buflen, dsb->sec_mixpos, oldpos);
		DSOUND_CheckEvent(dsb, oldpos, ilen);
	}

	return frames;
}

/**
 * Mix some frames from the given secondary buffer "dsb" into the device
 * primary buffer.
 *
 * dsb = the secondary buffer
 * playpos = the current play position in the device buffer (primary buffer)
 * frames = the maximum number of frames in the primary buffer to mix, from the
 *          current writepos.
 *
 * Returns: the number of frames beyond the writepos that were mixed.
 */
static DWORD DSOUND_MixOne(IDirectSoundBufferImpl *dsb, float *mix_buffer, DWORD frames)
{
	DWORD primary_done = 0;

	TRACE("(%p, frames=%ld)\n",dsb,frames);
	TRACE("looping=%ld, leadin=%ld\n", dsb->playflags, dsb->leadin);

	/* If leading in, only mix about 20 ms, and 'skip' mixing the rest, for more fluid pointer advancement */
	/* FIXME: Is this needed? */
	if (dsb->leadin && dsb->state == STATE_STARTING) {
		if (frames > 2 * dsb->device->frag_frames) {
			primary_done = frames - 2 * dsb->device->frag_frames;
			frames = 2 * dsb->device->frag_frames;
			dsb->sec_mixpos += primary_done *
				dsb->pwfx->nBlockAlign * dsb->freqAdjustNum / dsb->freqAdjustDen;
		}
	}

	dsb->leadin = FALSE;

	TRACE("frames (primary) = %li\n", frames);

	/* First try to mix to the end of the buffer if possible
	 * Theoretically it would allow for better optimization
	*/
	primary_done += DSOUND_MixInBuffer(dsb, mix_buffer, frames);

	TRACE("total mixed data=%ld\n", primary_done);

	/* Report back the total prebuffered amount for this buffer */
	return primary_done;
}

/**
 * For a DirectSoundDevice, go through all the currently playing buffers and
 * mix them in to the device buffer.
 *
 * frames = the maximum amount to mix into the primary buffer
 * all_stopped = reports back if all buffers have stopped
 *
 * Returns:  the length beyond the writepos that was mixed to.
 */

static void DSOUND_MixToPrimary(const DirectSoundDevice *device, float *mix_buffer, DWORD frames, BOOL *all_stopped)
{
	INT i;
	IDirectSoundBufferImpl	*dsb;

	/* unless we find a running buffer, all have stopped */
	*all_stopped = TRUE;

	TRACE("(frames %ld)\n", frames);
	for (i = 0; i < device->nrofbuffers; i++) {
		dsb = device->buffers[i];

		TRACE("MixToPrimary for %p, state=%ld\n", dsb, dsb->state);

		if (dsb->buflen && dsb->state) {
			TRACE("Checking %p, frames=%ld\n", dsb, frames);
			AcquireSRWLockShared(&dsb->lock);
			if (dsb->state != STATE_STOPPED) {

				/* if the buffer was starting, it must be playing now */
				if (dsb->state == STATE_STARTING)
					dsb->state = STATE_PLAYING;

				/* mix next buffer into the main buffer */
				DSOUND_MixOne(dsb, mix_buffer, frames);

				*all_stopped = FALSE;
			}
			ReleaseSRWLockShared(&dsb->lock);
		}
	}
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

static void DSOUND_WaveQueue(DirectSoundDevice *device, LPBYTE pos, DWORD bytes)
{
	BYTE *buffer;
	HRESULT hr;

	TRACE("(%p)\n", device);

	hr = IAudioRenderClient_GetBuffer(device->render, bytes / device->pwfx->nBlockAlign, &buffer);
	if(FAILED(hr)){
		WARN("GetBuffer failed: %08lx\n", hr);
		return;
	}

	memcpy(buffer, pos, bytes);

	hr = IAudioRenderClient_ReleaseBuffer(device->render, bytes / device->pwfx->nBlockAlign, 0);
	if(FAILED(hr)) {
		ERR("ReleaseBuffer failed: %08lx\n", hr);
		IAudioRenderClient_ReleaseBuffer(device->render, 0, 0);
		return;
	}

	device->pad += bytes;
}

/**
 * Perform mixing for a Direct Sound device. That is, go through all the
 * secondary buffers (the sound bites currently playing) and mix them in
 * to the primary buffer (the device buffer).
 *
 * The mixing procedure goes:
 *
 * secondary->buffer (secondary format)
 *   =[Resample]=> device->tmp_buffer (float format)
 *   =[Volume]=> device->tmp_buffer (float format)
 *   =[Reformat]=> device->buffer (device format, skipped on float)
 */
static void DSOUND_PerformMix(DirectSoundDevice *device)
{
	DWORD block, pad_bytes, frames;
	UINT32 pad_frames;
	HRESULT hr;

	TRACE("(%p)\n", device);

	/* **** */
	EnterCriticalSection(&device->mixlock);

	hr = IAudioClient_GetCurrentPadding(device->client, &pad_frames);
	if(FAILED(hr)){
		WARN("GetCurrentPadding failed: %08lx\n", hr);
		LeaveCriticalSection(&device->mixlock);
		return;
	}
	block = device->pwfx->nBlockAlign;
	pad_bytes = pad_frames * block;
	device->playpos += device->pad - pad_bytes;
	device->playpos %= device->buflen;
	device->pad = pad_bytes;

	frames = device->ac_frames - pad_frames;
	if(!frames){
		/* nothing to do! */
		LeaveCriticalSection(&device->mixlock);
		return;
	}
	if (frames > device->frag_frames * 3)
		frames = device->frag_frames * 3;

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		BOOL all_stopped = FALSE;
		int nfiller;
		void *buffer = NULL;

		/* the sound of silence */
		nfiller = device->pwfx->wBitsPerSample == 8 ? 128 : 0;

		/* check for underrun. underrun occurs when the write position passes the mix position
		 * also wipe out just-played sound data */
		if (!pad_frames)
			WARN("Probable buffer underrun\n");

		hr = IAudioRenderClient_GetBuffer(device->render, frames, (BYTE **)&buffer);
		if(FAILED(hr)){
			WARN("GetBuffer failed: %08lx\n", hr);
			LeaveCriticalSection(&device->mixlock);
			return;
		}

		memset(buffer, nfiller, frames * block);

		if (!device->normfunction)
			DSOUND_MixToPrimary(device, buffer, frames, &all_stopped);
		else {
			memset(device->buffer, nfiller, device->buflen);

			/* do the mixing */
			DSOUND_MixToPrimary(device, (float*)device->buffer, frames, &all_stopped);

			device->normfunction(device->buffer, buffer, frames * device->pwfx->nChannels);
		}

		hr = IAudioRenderClient_ReleaseBuffer(device->render, frames, 0);
		if(FAILED(hr))
			ERR("ReleaseBuffer failed: %08lx\n", hr);

		device->pad += frames * block;
	} else if (!device->stopped) {
                DWORD writepos = (device->playpos + pad_bytes) % device->buflen;
                DWORD bytes = frames * block;

		if (bytes > device->buflen)
			bytes = device->buflen;
		if (writepos + bytes > device->buflen) {
			DSOUND_WaveQueue(device, device->buffer + writepos, device->buflen - writepos);
			DSOUND_WaveQueue(device, device->buffer, writepos + bytes - device->buflen);
		} else
			DSOUND_WaveQueue(device, device->buffer + writepos, bytes);
	}

	LeaveCriticalSection(&(device->mixlock));
	/* **** */
}

DWORD CALLBACK DSOUND_mixthread(void *p)
{
	DirectSoundDevice *dev = p;
	TRACE("(%p)\n", dev);
	SetThreadDescription(GetCurrentThread(), L"wine_dsound_mixer");

	while (dev->ref) {
		DWORD ret;

		/*
		 * Some audio drivers are retarded and won't fire after being
		 * stopped, add a timeout to handle this.
		 */
		ret = WaitForSingleObject(dev->sleepev, dev->sleeptime);
		if (ret == WAIT_FAILED)
			WARN("wait returned error %lu %08lx!\n", GetLastError(), GetLastError());
		else if (ret != WAIT_OBJECT_0)
			WARN("wait returned %08lx!\n", ret);
		if (!dev->ref)
			break;

		AcquireSRWLockShared(&dev->buffer_list_lock);
		DSOUND_PerformMix(dev);
		ReleaseSRWLockShared(&dev->buffer_list_lock);
	}
	return 0;
}
