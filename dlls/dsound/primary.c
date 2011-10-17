/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
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
 *
 * TODO:
 *      When PrimarySetFormat (via ReopenDevice or PrimaryOpen) fails,
 *       it leaves dsound in unusable (not really open) state.
 */

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/** Calculate how long a fragment length of about 10 ms should be in frames
 *
 * nSamplesPerSec: Frequency rate in samples per second
 * nBlockAlign: Size of a single blockalign
 *
 * Returns:
 * Size in bytes of a single fragment
 */
DWORD DSOUND_fraglen(DWORD nSamplesPerSec, DWORD nBlockAlign)
{
    /* Given a timer delay of 10ms, the fragment size is approximately:
     *     fraglen = (nSamplesPerSec * 10 / 1000) * nBlockAlign
     * ==> fraglen = (nSamplesPerSec / 100) * nBlockSize
     *
     * ALSA uses buffers that are powers of 2. Because of this, fraglen
     * is rounded up to the nearest power of 2:
     */

    if (nSamplesPerSec <= 12800)
        return 128 * nBlockAlign;

    if (nSamplesPerSec <= 25600)
        return 256 * nBlockAlign;

    if (nSamplesPerSec <= 51200)
        return 512 * nBlockAlign;

    return 1024 * nBlockAlign;
}

static void DSOUND_RecalcPrimary(DirectSoundDevice *device)
{
    TRACE("(%p)\n", device);

    device->fraglen = DSOUND_fraglen(device->pwfx->nSamplesPerSec, device->pwfx->nBlockAlign);
    device->helfrags = device->buflen / device->fraglen;
    TRACE("fraglen=%d helfrags=%d\n", device->fraglen, device->helfrags);

    /* calculate the 10ms write lead */
    device->writelead = (device->pwfx->nSamplesPerSec / 100) * device->pwfx->nBlockAlign;
}

HRESULT DSOUND_ReopenDevice(DirectSoundDevice *device, BOOL forcewave)
{
    UINT prebuf_frames;
    REFERENCE_TIME prebuf_rt;
    HRESULT hres;

    TRACE("(%p, %d)\n", device, forcewave);

    if(device->client){
        IAudioClient_Release(device->client);
        device->client = NULL;
    }
    if(device->render){
        IAudioRenderClient_Release(device->render);
        device->render = NULL;
    }
    if(device->clock){
        IAudioClock_Release(device->clock);
        device->clock = NULL;
    }
    if(device->volume){
        IAudioStreamVolume_Release(device->volume);
        device->volume = NULL;
    }

    hres = IMMDevice_Activate(device->mmdevice, &IID_IAudioClient,
            CLSCTX_INPROC_SERVER, NULL, (void **)&device->client);
    if(FAILED(hres)){
        WARN("Activate failed: %08x\n", hres);
        return hres;
    }

    prebuf_frames = device->prebuf * DSOUND_fraglen(device->pwfx->nSamplesPerSec, device->pwfx->nBlockAlign) / device->pwfx->nBlockAlign;
    prebuf_rt = (10000000 * (UINT64)prebuf_frames) / device->pwfx->nSamplesPerSec;

    hres = IAudioClient_Initialize(device->client,
            AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST,
            prebuf_rt, 50000, device->pwfx, NULL);
    if(FAILED(hres)){
        IAudioClient_Release(device->client);
        device->client = NULL;
        WARN("Initialize failed: %08x\n", hres);
        return hres;
    }

    hres = IAudioClient_GetService(device->client, &IID_IAudioRenderClient,
            (void**)&device->render);
    if(FAILED(hres)){
        IAudioClient_Release(device->client);
        device->client = NULL;
        WARN("GetService failed: %08x\n", hres);
        return hres;
    }

    hres = IAudioClient_GetService(device->client, &IID_IAudioClock,
            (void**)&device->clock);
    if(FAILED(hres)){
        IAudioClient_Release(device->client);
        IAudioRenderClient_Release(device->render);
        device->client = NULL;
        device->render = NULL;
        WARN("GetService failed: %08x\n", hres);
        return hres;
    }

    hres = IAudioClient_GetService(device->client, &IID_IAudioStreamVolume,
            (void**)&device->volume);
    if(FAILED(hres)){
        IAudioClient_Release(device->client);
        IAudioRenderClient_Release(device->render);
        IAudioClock_Release(device->clock);
        device->client = NULL;
        device->render = NULL;
        device->clock = NULL;
        WARN("GetService failed: %08x\n", hres);
        return hres;
    }

    return S_OK;
}

static HRESULT DSOUND_PrimaryOpen(DirectSoundDevice *device)
{
	DWORD buflen;
	LPBYTE newbuf;

	TRACE("(%p)\n", device);

	/* on original windows, the buffer it set to a fixed size, no matter what the settings are.
	   on windows this size is always fixed (tested on win-xp) */
	if (!device->buflen)
		device->buflen = ds_hel_buflen;
	buflen = device->buflen;
	buflen -= buflen % device->pwfx->nBlockAlign;
	device->buflen = buflen;

	device->mix_buffer_len = DSOUND_bufpos_to_mixpos(device, device->buflen);
	device->mix_buffer = HeapAlloc(GetProcessHeap(), 0, device->mix_buffer_len);
	if (!device->mix_buffer)
		return DSERR_OUTOFMEMORY;

	if (device->state == STATE_PLAYING) device->state = STATE_STARTING;
	else if (device->state == STATE_STOPPING) device->state = STATE_STOPPED;

    TRACE("desired buflen=%d, old buffer=%p\n", buflen, device->buffer);

    /* reallocate emulated primary buffer */
    if (device->buffer)
        newbuf = HeapReAlloc(GetProcessHeap(),0,device->buffer, buflen);
    else
        newbuf = HeapAlloc(GetProcessHeap(),0, buflen);

    if (!newbuf) {
        ERR("failed to allocate primary buffer\n");
        return DSERR_OUTOFMEMORY;
        /* but the old buffer might still exist and must be re-prepared */
    }

    DSOUND_RecalcPrimary(device);

    device->buffer = newbuf;

    TRACE("fraglen=%d\n", device->fraglen);

	device->mixfunction = mixfunctions[device->pwfx->wBitsPerSample/8 - 1];
	device->normfunction = normfunctions[device->pwfx->wBitsPerSample/8 - 1];
	FillMemory(device->buffer, device->buflen, (device->pwfx->wBitsPerSample == 8) ? 128 : 0);
	FillMemory(device->mix_buffer, device->mix_buffer_len, 0);
	device->last_pos_bytes = device->pwplay = device->pwqueue = device->playpos = device->mixpos = 0;
	return DS_OK;
}


static void DSOUND_PrimaryClose(DirectSoundDevice *device)
{
    HRESULT hr;

    TRACE("(%p)\n", device);

    device->pwqueue = (DWORD)-1; /* resetting queues */

    if(device->client){
        hr = IAudioClient_Stop(device->client);
        if(FAILED(hr))
            WARN("Stop failed: %08x\n", hr);
    }

    /* clear the queue */
    device->pwqueue = 0;
}

HRESULT DSOUND_PrimaryCreate(DirectSoundDevice *device)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n", device);

	device->buflen = ds_hel_buflen;
	err = DSOUND_PrimaryOpen(device);

	if (err != DS_OK) {
		WARN("DSOUND_PrimaryOpen failed\n");
		return err;
	}

	device->state = STATE_STOPPED;
	return DS_OK;
}

HRESULT DSOUND_PrimaryDestroy(DirectSoundDevice *device)
{
	TRACE("(%p)\n", device);

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	DSOUND_PrimaryClose(device);
	HeapFree(GetProcessHeap(),0,device->pwfx);
	device->pwfx=NULL;

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

HRESULT DSOUND_PrimaryPlay(DirectSoundDevice *device)
{
    HRESULT hr;

    TRACE("(%p)\n", device);

    hr = IAudioClient_Start(device->client);
    if(FAILED(hr)){
        WARN("Start failed: %08x\n", hr);
        return hr;
    }

    return DS_OK;
}

HRESULT DSOUND_PrimaryStop(DirectSoundDevice *device)
{
    HRESULT hr;

    TRACE("(%p)\n", device);

    hr = IAudioClient_Stop(device->client);
    if(FAILED(hr)){
        WARN("Stop failed: %08x\n", hr);
        return hr;
    }

    return DS_OK;
}

HRESULT DSOUND_PrimaryGetPosition(DirectSoundDevice *device, LPDWORD playpos, LPDWORD writepos)
{
	TRACE("(%p,%p,%p)\n", device, playpos, writepos);

	TRACE("pwplay=%i, pwqueue=%i\n", device->pwplay, device->pwqueue);

	/* check if playpos was requested */
	if (playpos)
		/* use the cached play position */
		*playpos = device->pwplay * device->fraglen;

	/* check if writepos was requested */
	if (writepos)
		/* the writepos is the first non-queued position */
		*writepos = ((device->pwplay + device->pwqueue) % device->helfrags) * device->fraglen;

	TRACE("playpos = %d, writepos = %d (%p, time=%d)\n", playpos?*playpos:-1, writepos?*writepos:-1, device, GetTickCount());
	return DS_OK;
}

static DWORD DSOUND_GetFormatSize(LPCWAVEFORMATEX wfex)
{
	if (wfex->wFormatTag == WAVE_FORMAT_PCM)
		return sizeof(WAVEFORMATEX);
	else
		return sizeof(WAVEFORMATEX) + wfex->cbSize;
}

LPWAVEFORMATEX DSOUND_CopyFormat(LPCWAVEFORMATEX wfex)
{
	DWORD size = DSOUND_GetFormatSize(wfex);
	LPWAVEFORMATEX pwfx = HeapAlloc(GetProcessHeap(),0,size);
	if (pwfx == NULL) {
		WARN("out of memory\n");
	} else if (wfex->wFormatTag != WAVE_FORMAT_PCM) {
		CopyMemory(pwfx, wfex, size);
	} else {
		CopyMemory(pwfx, wfex, sizeof(PCMWAVEFORMAT));
		pwfx->cbSize=0;
		if (pwfx->nBlockAlign != pwfx->nChannels * pwfx->wBitsPerSample/8) {
			WARN("Fixing bad nBlockAlign (%u)\n", pwfx->nBlockAlign);
			pwfx->nBlockAlign  = pwfx->nChannels * pwfx->wBitsPerSample/8;
		}
		if (pwfx->nAvgBytesPerSec != pwfx->nSamplesPerSec * pwfx->nBlockAlign) {
			WARN("Fixing bad nAvgBytesPerSec (%u)\n", pwfx->nAvgBytesPerSec);
			pwfx->nAvgBytesPerSec  = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		}
	}
	return pwfx;
}

HRESULT primarybuffer_SetFormat(DirectSoundDevice *device, LPCWAVEFORMATEX passed_fmt)
{
	HRESULT err = DSERR_BUFFERLOST;
	int i;
	WAVEFORMATEX *old_fmt;
	WAVEFORMATEXTENSIBLE *fmtex;
	BOOL forced = (device->priolevel == DSSCL_WRITEPRIMARY);

	TRACE("(%p,%p)\n", device, passed_fmt);

	if (device->priolevel == DSSCL_NORMAL) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	/* Let's be pedantic! */
	if (passed_fmt == NULL) {
		WARN("invalid parameter: passed_fmt==NULL!\n");
		return DSERR_INVALIDPARAM;
	}
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%d,"
			  "bytespersec=%d,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		  passed_fmt->wFormatTag, passed_fmt->nChannels, passed_fmt->nSamplesPerSec,
		  passed_fmt->nAvgBytesPerSec, passed_fmt->nBlockAlign,
		  passed_fmt->wBitsPerSample, passed_fmt->cbSize);

	/* **** */
	RtlAcquireResourceExclusive(&(device->buffer_list_lock), TRUE);
	EnterCriticalSection(&(device->mixlock));

	old_fmt = device->pwfx;
	device->pwfx = DSOUND_CopyFormat(passed_fmt);
	fmtex = (WAVEFORMATEXTENSIBLE *)device->pwfx;
	if (device->pwfx == NULL) {
		device->pwfx = old_fmt;
		old_fmt = NULL;
		err = DSERR_OUTOFMEMORY;
		goto done;
	}

	DSOUND_PrimaryClose(device);

	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	/* requested format failed, so try others */
	if(device->pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT){
		device->pwfx->wFormatTag = WAVE_FORMAT_PCM;
		device->pwfx->wBitsPerSample = 32;
		device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
		device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);

		err = DSOUND_ReopenDevice(device, FALSE);
		if(SUCCEEDED(err))
			goto opened;
	}

	if(device->pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
			 IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)){
		fmtex->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		device->pwfx->wBitsPerSample = 32;
		device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
		device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);

		err = DSOUND_ReopenDevice(device, FALSE);
		if(SUCCEEDED(err))
			goto opened;
	}

	device->pwfx->wBitsPerSample = 32;
	device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
	device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);
	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	device->pwfx->wBitsPerSample = 16;
	device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
	device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);
	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	device->pwfx->wBitsPerSample = 8;
	device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
	device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);
	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	device->pwfx->nChannels = (passed_fmt->nChannels == 2) ? 1 : 2;
	device->pwfx->wBitsPerSample = passed_fmt->wBitsPerSample;
	device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
	device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);
	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	device->pwfx->wBitsPerSample = 32;
	device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
	device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);
	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	device->pwfx->wBitsPerSample = 16;
	device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
	device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);
	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	device->pwfx->wBitsPerSample = 8;
	device->pwfx->nAvgBytesPerSec = passed_fmt->nSamplesPerSec * device->pwfx->nBlockAlign;
	device->pwfx->nBlockAlign = passed_fmt->nChannels * (device->pwfx->wBitsPerSample / 8);
	err = DSOUND_ReopenDevice(device, FALSE);
	if(SUCCEEDED(err))
		goto opened;

	WARN("No formats could be opened\n");
	goto done;

opened:
	err = DSOUND_PrimaryOpen(device);
	if (err != DS_OK) {
		WARN("DSOUND_PrimaryOpen failed\n");
		goto done;
	}

	if (passed_fmt->nSamplesPerSec/100 != device->pwfx->nSamplesPerSec/100 && forced && device->buffer)
	{
		DSOUND_PrimaryClose(device);
		device->pwfx->nSamplesPerSec = passed_fmt->nSamplesPerSec;
		err = DSOUND_ReopenDevice(device, TRUE);
		if (FAILED(err))
			WARN("DSOUND_ReopenDevice(2) failed: %08x\n", err);
		else if (FAILED((err = DSOUND_PrimaryOpen(device))))
			WARN("DSOUND_PrimaryOpen(2) failed: %08x\n", err);
	}

	device->mix_buffer_len = DSOUND_bufpos_to_mixpos(device, device->buflen);
	device->mix_buffer = HeapReAlloc(GetProcessHeap(), 0, device->mix_buffer, device->mix_buffer_len);
	FillMemory(device->mix_buffer, device->mix_buffer_len, 0);
	device->mixfunction = mixfunctions[device->pwfx->wBitsPerSample/8 - 1];
	device->normfunction = normfunctions[device->pwfx->wBitsPerSample/8 - 1];

	if (old_fmt->nSamplesPerSec != device->pwfx->nSamplesPerSec ||
			old_fmt->wBitsPerSample != device->pwfx->wBitsPerSample ||
			old_fmt->nChannels != device->pwfx->nChannels) {
		IDirectSoundBufferImpl** dsb = device->buffers;
		for (i = 0; i < device->nrofbuffers; i++, dsb++) {
			/* **** */
			RtlAcquireResourceExclusive(&(*dsb)->lock, TRUE);

			(*dsb)->freqAdjust = ((DWORD64)(*dsb)->freq << DSOUND_FREQSHIFT) / device->pwfx->nSamplesPerSec;
			DSOUND_RecalcFormat((*dsb));
			DSOUND_MixToTemporary((*dsb), 0, (*dsb)->buflen, FALSE);
			(*dsb)->primary_mixpos = 0;

			RtlReleaseResource(&(*dsb)->lock);
			/* **** */
		}
	}

done:
	LeaveCriticalSection(&(device->mixlock));
	RtlReleaseResource(&(device->buffer_list_lock));
	/* **** */

	HeapFree(GetProcessHeap(), 0, old_fmt);
	return err;
}

/*******************************************************************************
 *		PrimaryBuffer
 */
static inline IDirectSoundBufferImpl *impl_from_IDirectSoundBuffer(IDirectSoundBuffer *iface)
{
    /* IDirectSoundBuffer and IDirectSoundBuffer8 use the same iface. */
    return CONTAINING_RECORD(iface, IDirectSoundBufferImpl, IDirectSoundBuffer8_iface);
}

/* This sets this format for the <em>Primary Buffer Only</em> */
/* See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 120 */
static HRESULT WINAPI PrimaryBufferImpl_SetFormat(
    LPDIRECTSOUNDBUFFER iface,
    LPCWAVEFORMATEX wfex)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    TRACE("(%p,%p)\n", iface, wfex);
    return primarybuffer_SetFormat(This->device, wfex);
}

static HRESULT WINAPI PrimaryBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER iface,LONG vol
) {
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	HRESULT hr;
	float lvol, rvol;

	TRACE("(%p,%d)\n", iface, vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN)) {
		WARN("invalid parameter: vol = %d\n", vol);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&device->mixlock);

	hr = IAudioStreamVolume_GetChannelVolume(device->volume, 0, &lvol);
	if(FAILED(hr)){
		LeaveCriticalSection(&device->mixlock);
		WARN("GetChannelVolume failed: %08x\n", hr);
		return hr;
	}

	if(device->pwfx->nChannels > 1){
		hr = IAudioStreamVolume_GetChannelVolume(device->volume, 1, &rvol);
		if(FAILED(hr)){
			LeaveCriticalSection(&device->mixlock);
			WARN("GetChannelVolume failed: %08x\n", hr);
			return hr;
		}
	}else
		rvol = 1;

	device->volpan.dwTotalLeftAmpFactor = ((UINT16)(lvol * (DWORD)0xFFFF));
	device->volpan.dwTotalRightAmpFactor = ((UINT16)(rvol * (DWORD)0xFFFF));

	DSOUND_AmpFactorToVolPan(&device->volpan);
	if (vol != device->volpan.lVolume) {
		device->volpan.lVolume=vol;
		DSOUND_RecalcVolPan(&device->volpan);
		lvol = (float)((DWORD)(device->volpan.dwTotalLeftAmpFactor & 0xFFFF) / (float)0xFFFF);
		hr = IAudioStreamVolume_SetChannelVolume(device->volume, 0, lvol);
		if(FAILED(hr)){
			LeaveCriticalSection(&device->mixlock);
			WARN("SetChannelVolume failed: %08x\n", hr);
			return hr;
		}

		if(device->pwfx->nChannels > 1){
			rvol = (float)((DWORD)(device->volpan.dwTotalRightAmpFactor & 0xFFFF) / (float)0xFFFF);
			hr = IAudioStreamVolume_SetChannelVolume(device->volume, 1, rvol);
			if(FAILED(hr)){
				LeaveCriticalSection(&device->mixlock);
				WARN("SetChannelVolume failed: %08x\n", hr);
				return hr;
			}
		}
	}

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER iface,LPLONG vol
) {
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	float lvol, rvol;
	HRESULT hr;
	TRACE("(%p,%p)\n", iface, vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (vol == NULL) {
		WARN("invalid parameter: vol = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	EnterCriticalSection(&device->mixlock);

	hr = IAudioStreamVolume_GetChannelVolume(device->volume, 0, &lvol);
	if(FAILED(hr)){
		LeaveCriticalSection(&device->mixlock);
		WARN("GetChannelVolume failed: %08x\n", hr);
		return hr;
	}

	if(device->pwfx->nChannels > 1){
		hr = IAudioStreamVolume_GetChannelVolume(device->volume, 1, &rvol);
		if(FAILED(hr)){
			LeaveCriticalSection(&device->mixlock);
			WARN("GetChannelVolume failed: %08x\n", hr);
			return hr;
		}
	}else
		rvol = 1;

	device->volpan.dwTotalLeftAmpFactor = ((UINT16)(lvol * (DWORD)0xFFFF));
	device->volpan.dwTotalRightAmpFactor = ((UINT16)(rvol * (DWORD)0xFFFF));

	DSOUND_AmpFactorToVolPan(&device->volpan);
	*vol = device->volpan.lVolume;

	LeaveCriticalSection(&device->mixlock);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetFrequency(
	LPDIRECTSOUNDBUFFER iface,DWORD freq
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	TRACE("(%p,%d)\n",This,freq);

	/* You cannot set the frequency of the primary buffer */
	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_Play(
	LPDIRECTSOUNDBUFFER iface,DWORD reserved1,DWORD reserved2,DWORD flags
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%08x,%08x,%08x)\n", iface, reserved1, reserved2, flags);

	if (!(flags & DSBPLAY_LOOPING)) {
		WARN("invalid parameter: flags = %08x\n", flags);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	if (device->state == STATE_STOPPED)
		device->state = STATE_STARTING;
	else if (device->state == STATE_STOPPING)
		device->state = STATE_PLAYING;

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Stop(LPDIRECTSOUNDBUFFER iface)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p)\n", iface);

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	if (device->state == STATE_PLAYING)
		device->state = STATE_STOPPING;
	else if (device->state == STATE_STARTING)
		device->state = STATE_STOPPED;

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

static ULONG WINAPI PrimaryBufferImpl_AddRef(LPDIRECTSOUNDBUFFER iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);
    return ref;
}

void primarybuffer_destroy(IDirectSoundBufferImpl *This)
{
    This->device->primary = NULL;
    HeapFree(GetProcessHeap(), 0, This);
    TRACE("(%p) released\n", This);
}

static ULONG WINAPI PrimaryBufferImpl_Release(LPDIRECTSOUNDBUFFER iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    DWORD ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        primarybuffer_destroy(This);
    return ref;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER iface,LPDWORD playpos,LPDWORD writepos
) {
	HRESULT	hres;
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p,%p)\n", iface, playpos, writepos);

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	hres = DSOUND_PrimaryGetPosition(device, playpos, writepos);
	if (hres != DS_OK) {
		WARN("DSOUND_PrimaryGetPosition failed\n");
		LeaveCriticalSection(&(device->mixlock));
		return hres;
	}
	if (writepos) {
		if (device->state != STATE_STOPPED)
			/* apply the documented 10ms lead to writepos */
			*writepos += device->writelead;
		while (*writepos >= device->buflen) *writepos -= device->buflen;
	}

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	TRACE("playpos = %d, writepos = %d (%p, time=%d)\n", playpos?*playpos:0, writepos?*writepos:0, device, GetTickCount());
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER iface,LPDWORD status
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p)\n", iface, status);

	if (status == NULL) {
		WARN("invalid parameter: status == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*status = 0;
	if ((device->state == STATE_STARTING) ||
	    (device->state == STATE_PLAYING))
		*status |= DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;

	TRACE("status=%x\n", *status);
	return DS_OK;
}


static HRESULT WINAPI PrimaryBufferImpl_GetFormat(
    LPDIRECTSOUNDBUFFER iface,
    LPWAVEFORMATEX lpwf,
    DWORD wfsize,
    LPDWORD wfwritten)
{
    DWORD size;
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    DirectSoundDevice *device = This->device;
    TRACE("(%p,%p,%d,%p)\n", iface, lpwf, wfsize, wfwritten);

    size = sizeof(WAVEFORMATEX) + device->pwfx->cbSize;

    if (lpwf) {	/* NULL is valid */
        if (wfsize >= size) {
            CopyMemory(lpwf,device->pwfx,size);
            if (wfwritten)
                *wfwritten = size;
        } else {
            WARN("invalid parameter: wfsize too small\n");
            if (wfwritten)
                *wfwritten = 0;
            return DSERR_INVALIDPARAM;
        }
    } else {
        if (wfwritten)
            *wfwritten = sizeof(WAVEFORMATEX) + device->pwfx->cbSize;
        else {
            WARN("invalid parameter: wfwritten == NULL\n");
            return DSERR_INVALIDPARAM;
        }
    }

    return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Lock(
	LPDIRECTSOUNDBUFFER iface,DWORD writecursor,DWORD writebytes,LPVOID *lplpaudioptr1,LPDWORD audiobytes1,LPVOID *lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {
	HRESULT hres;
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%d,%d,%p,%p,%p,%p,0x%08x) at %d\n",
		iface,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags,
		GetTickCount()
	);

        if (!audiobytes1)
            return DSERR_INVALIDPARAM;

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

        /* when this flag is set, writecursor is meaningless and must be calculated */
	if (flags & DSBLOCK_FROMWRITECURSOR) {
		/* GetCurrentPosition does too much magic to duplicate here */
		hres = IDirectSoundBuffer_GetCurrentPosition(iface, NULL, &writecursor);
		if (hres != DS_OK) {
			WARN("IDirectSoundBuffer_GetCurrentPosition failed\n");
			return hres;
		}
	}

        /* when this flag is set, writebytes is meaningless and must be set */
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = device->buflen;

        if (writecursor >= device->buflen) {
                WARN("Invalid parameter, writecursor: %u >= buflen: %u\n",
		     writecursor, device->buflen);
                return DSERR_INVALIDPARAM;
        }

        if (writebytes > device->buflen) {
                WARN("Invalid parameter, writebytes: %u > buflen: %u\n",
		     writebytes, device->buflen);
                return DSERR_INVALIDPARAM;
        }

	if (writecursor+writebytes <= device->buflen) {
		*(LPBYTE*)lplpaudioptr1 = device->buffer+writecursor;
		*audiobytes1 = writebytes;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = NULL;
		if (audiobytes2)
			*audiobytes2 = 0;
		TRACE("->%d.0\n",writebytes);
	} else {
		*(LPBYTE*)lplpaudioptr1 = device->buffer+writecursor;
		*audiobytes1 = device->buflen-writecursor;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = device->buffer;
		if (audiobytes2)
			*audiobytes2 = writebytes-(device->buflen-writecursor);
		TRACE("->%d.%d\n",*audiobytes1,audiobytes2?*audiobytes2:0);
	}
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER iface,DWORD newpos
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	TRACE("(%p,%d)\n",This,newpos);

	/* You cannot set the position of the primary buffer */
	WARN("invalid call\n");
	return DSERR_INVALIDCALL;
}

static HRESULT WINAPI PrimaryBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER iface,LONG pan
) {
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	float lvol, rvol;
	HRESULT hr;
	TRACE("(%p,%d)\n", iface, pan);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT)) {
		WARN("invalid parameter: pan = %d\n", pan);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&device->mixlock);

	hr = IAudioStreamVolume_GetChannelVolume(device->volume, 0, &lvol);
	if(FAILED(hr)){
		LeaveCriticalSection(&device->mixlock);
		WARN("GetChannelVolume failed: %08x\n", hr);
		return hr;
	}

	if(device->pwfx->nChannels > 1){
		hr = IAudioStreamVolume_GetChannelVolume(device->volume, 1, &rvol);
		if(FAILED(hr)){
			LeaveCriticalSection(&device->mixlock);
			WARN("GetChannelVolume failed: %08x\n", hr);
			return hr;
		}
	}else
		rvol = 1;

	device->volpan.dwTotalLeftAmpFactor = ((UINT16)(lvol * (DWORD)0xFFFF));
	device->volpan.dwTotalRightAmpFactor = ((UINT16)(rvol * (DWORD)0xFFFF));

	DSOUND_AmpFactorToVolPan(&device->volpan);
	if (pan != device->volpan.lPan) {
		device->volpan.lPan=pan;
		DSOUND_RecalcVolPan(&device->volpan);

		lvol = (float)((DWORD)(device->volpan.dwTotalLeftAmpFactor & 0xFFFF) / (float)0xFFFF);
		hr = IAudioStreamVolume_SetChannelVolume(device->volume, 0, lvol);
		if(FAILED(hr)){
			LeaveCriticalSection(&device->mixlock);
			WARN("SetChannelVolume failed: %08x\n", hr);
			return hr;
		}

		if(device->pwfx->nChannels > 1){
			rvol = (float)((DWORD)(device->volpan.dwTotalRightAmpFactor & 0xFFFF) / (float)0xFFFF);
			hr = IAudioStreamVolume_SetChannelVolume(device->volume, 1, rvol);
			if(FAILED(hr)){
				LeaveCriticalSection(&device->mixlock);
				WARN("SetChannelVolume failed: %08x\n", hr);
				return hr;
			}
		}
	}

	LeaveCriticalSection(&device->mixlock);
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetPan(
	LPDIRECTSOUNDBUFFER iface,LPLONG pan
) {
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	float lvol, rvol;
	HRESULT hr;
	TRACE("(%p,%p)\n", iface, pan);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (pan == NULL) {
		WARN("invalid parameter: pan == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	EnterCriticalSection(&device->mixlock);

	hr = IAudioStreamVolume_GetChannelVolume(device->volume, 0, &lvol);
	if(FAILED(hr)){
		LeaveCriticalSection(&device->mixlock);
		WARN("GetChannelVolume failed: %08x\n", hr);
		return hr;
	}

	if(device->pwfx->nChannels > 1){
		hr = IAudioStreamVolume_GetChannelVolume(device->volume, 1, &rvol);
		if(FAILED(hr)){
			LeaveCriticalSection(&device->mixlock);
			WARN("GetChannelVolume failed: %08x\n", hr);
			return hr;
		}
	}else
		rvol = 1;

	device->volpan.dwTotalLeftAmpFactor = ((UINT16)(lvol * (DWORD)0xFFFF));
	device->volpan.dwTotalRightAmpFactor = ((UINT16)(rvol * (DWORD)0xFFFF));

	DSOUND_AmpFactorToVolPan(&device->volpan);
	*pan = device->volpan.lPan;

	LeaveCriticalSection(&device->mixlock);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Unlock(
	LPDIRECTSOUNDBUFFER iface,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p,%d,%p,%d)\n", iface, p1, x1, p2, x2);

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

    if((p1 && ((BYTE*)p1 < device->buffer ||
                    (BYTE*)p1 >= device->buffer + device->buflen)) ||
            (p2 && ((BYTE*)p2 < device->buffer ||
                    (BYTE*)p2 >= device->buffer + device->buflen)))
        return DSERR_INVALIDPARAM;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Restore(
	LPDIRECTSOUNDBUFFER iface
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetFrequency(
	LPDIRECTSOUNDBUFFER iface,LPDWORD freq
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p)\n", iface, freq);

	if (freq == NULL) {
		WARN("invalid parameter: freq == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	*freq = device->pwfx->nSamplesPerSec;
	TRACE("-> %d\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER iface,LPDIRECTSOUND dsound,LPCDSBUFFERDESC dbsd
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	WARN("(%p) already initialized\n", This);
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCaps(
	LPDIRECTSOUNDBUFFER iface,LPDSBCAPS caps
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
  	TRACE("(%p,%p)\n", iface, caps);

	if (caps == NULL) {
		WARN("invalid parameter: caps == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (caps->dwSize < sizeof(*caps)) {
		WARN("invalid parameter: caps->dwSize = %d\n", caps->dwSize);
		return DSERR_INVALIDPARAM;
	}

	caps->dwFlags = This->dsbd.dwFlags;
	caps->dwBufferBytes = device->buflen;

	/* Windows reports these as zero */
	caps->dwUnlockTransferRate = 0;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER iface,REFIID riid,LPVOID *ppobj
) {
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	*ppobj = NULL;	/* assume failure */

	if ( IsEqualGUID(riid, &IID_IUnknown) ||
	     IsEqualGUID(riid, &IID_IDirectSoundBuffer) ) {
		IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)This);
		*ppobj = This;
		return S_OK;
	}

	/* DirectSoundBuffer and DirectSoundBuffer8 are different and */
	/* a primary buffer can't have a DirectSoundBuffer8 interface */
	if ( IsEqualGUID( &IID_IDirectSoundBuffer8, riid ) ) {
		WARN("app requested DirectSoundBuffer8 on primary buffer\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSoundNotify, riid ) ) {
		ERR("app requested IDirectSoundNotify on primary buffer\n");
		/* FIXME: should we support this? */
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
		ERR("app requested IDirectSound3DBuffer on primary buffer\n");
		return E_NOINTERFACE;
	}

        if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		if (!device->listener)
			IDirectSound3DListenerImpl_Create(device, &device->listener);
		if (device->listener) {
			*ppobj = device->listener;
			IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)*ppobj);
			return S_OK;
		}

		WARN("IID_IDirectSound3DListener failed\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
		FIXME("app requested IKsPropertySet on primary buffer\n");
		return E_NOINTERFACE;
	}

	FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );
	return E_NOINTERFACE;
}

static const IDirectSoundBufferVtbl dspbvt =
{
	PrimaryBufferImpl_QueryInterface,
	PrimaryBufferImpl_AddRef,
	PrimaryBufferImpl_Release,
	PrimaryBufferImpl_GetCaps,
	PrimaryBufferImpl_GetCurrentPosition,
	PrimaryBufferImpl_GetFormat,
	PrimaryBufferImpl_GetVolume,
	PrimaryBufferImpl_GetPan,
        PrimaryBufferImpl_GetFrequency,
	PrimaryBufferImpl_GetStatus,
	PrimaryBufferImpl_Initialize,
	PrimaryBufferImpl_Lock,
	PrimaryBufferImpl_Play,
	PrimaryBufferImpl_SetCurrentPosition,
	PrimaryBufferImpl_SetFormat,
	PrimaryBufferImpl_SetVolume,
	PrimaryBufferImpl_SetPan,
	PrimaryBufferImpl_SetFrequency,
	PrimaryBufferImpl_Stop,
	PrimaryBufferImpl_Unlock,
	PrimaryBufferImpl_Restore
};

HRESULT primarybuffer_create(DirectSoundDevice *device, IDirectSoundBufferImpl **ppdsb,
	const DSBUFFERDESC *dsbd)
{
	IDirectSoundBufferImpl *dsb;
	TRACE("%p,%p,%p)\n",device,ppdsb,dsbd);

	if (dsbd->lpwfxFormat) {
		WARN("invalid parameter: dsbd->lpwfxFormat != NULL\n");
		*ppdsb = NULL;
		return DSERR_INVALIDPARAM;
	}

	dsb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));

	if (dsb == NULL) {
		WARN("out of memory\n");
		*ppdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

        dsb->ref = 1;
        dsb->numIfaces = 1;
	dsb->device = device;
	dsb->IDirectSoundBuffer8_iface.lpVtbl = (IDirectSoundBuffer8Vtbl *)&dspbvt;
	dsb->dsbd = *dsbd;

	TRACE("Created primary buffer at %p\n", dsb);
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%d,"
		"bytespersec=%d,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		device->pwfx->wFormatTag, device->pwfx->nChannels,
                device->pwfx->nSamplesPerSec, device->pwfx->nAvgBytesPerSec,
                device->pwfx->nBlockAlign, device->pwfx->wBitsPerSample,
                device->pwfx->cbSize);

	*ppdsb = dsb;
	return S_OK;
}
