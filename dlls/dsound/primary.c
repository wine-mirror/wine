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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

void DSOUND_RecalcPrimary(IDirectSoundImpl *This)
{
	DWORD sw;
	TRACE("(%p)\n",This);

	sw = This->wfx.nChannels * (This->wfx.wBitsPerSample / 8);
	if (This->hwbuf) {
		DWORD fraglen;
		/* let fragment size approximate the timer delay */
		fraglen = (This->wfx.nSamplesPerSec * DS_TIME_DEL / 1000) * sw;
		/* reduce fragment size until an integer number of them fits in the buffer */
		/* (FIXME: this may or may not be a good idea) */
		while (This->buflen % fraglen) fraglen -= sw;
		This->fraglen = fraglen;
		TRACE("fraglen=%ld\n", This->fraglen);
	}
	/* calculate the 10ms write lead */
	This->writelead = (This->wfx.nSamplesPerSec / 100) * sw;
}

static HRESULT DSOUND_PrimaryOpen(IDirectSoundImpl *This)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n",This);

	/* are we using waveOut stuff? */
	if (!This->hwbuf) {
		LPBYTE newbuf;
		DWORD buflen;
		HRESULT merr = DS_OK;
		/* Start in pause mode, to allow buffers to get filled */
		waveOutPause(This->hwo);
		if (This->state == STATE_PLAYING) This->state = STATE_STARTING;
		else if (This->state == STATE_STOPPING) This->state = STATE_STOPPED;
		/* use fragments of 10ms (1/100s) each (which should get us within
		 * the documented write cursor lead of 10-15ms) */
		buflen = ((This->wfx.nAvgBytesPerSec / 100) & ~3) * DS_HEL_FRAGS;
		TRACE("desired buflen=%ld, old buffer=%p\n", buflen, This->buffer);
		/* reallocate emulated primary buffer */
		newbuf = (LPBYTE)HeapReAlloc(GetProcessHeap(),0,This->buffer,buflen);
		if (newbuf == NULL) {
			ERR("failed to allocate primary buffer\n");
			merr = DSERR_OUTOFMEMORY;
			/* but the old buffer might still exist and must be re-prepared */
		} else {
			This->buffer = newbuf;
			This->buflen = buflen;
		}
		if (This->buffer) {
			unsigned c;

			This->fraglen = This->buflen / DS_HEL_FRAGS;

			/* prepare fragment headers */
			for (c=0; c<DS_HEL_FRAGS; c++) {
				This->pwave[c]->lpData = This->buffer + c*This->fraglen;
				This->pwave[c]->dwBufferLength = This->fraglen;
				This->pwave[c]->dwUser = (DWORD)This;
				This->pwave[c]->dwFlags = 0;
				This->pwave[c]->dwLoops = 0;
				err = mmErr(waveOutPrepareHeader(This->hwo,This->pwave[c],sizeof(WAVEHDR)));
				if (err != DS_OK) {
					while (c--)
						waveOutUnprepareHeader(This->hwo,This->pwave[c],sizeof(WAVEHDR));
					break;
				}
			}

			This->pwplay = 0;
			This->pwwrite = 0;
			This->pwqueue = 0;
			This->playpos = 0;
			This->mixpos = 0;
			memset(This->buffer, (This->wfx.wBitsPerSample == 16) ? 0 : 128, This->buflen);
			TRACE("fraglen=%ld\n", This->fraglen);
			DSOUND_WaveQueue(This, (DWORD)-1);
		}
		if ((err == DS_OK) && (merr != DS_OK))
			err = merr;
	}
	return err;
}


static void DSOUND_PrimaryClose(IDirectSoundImpl *This)
{
	TRACE("(%p)\n",This);

	/* are we using waveOut stuff? */
	if (!This->hwbuf) {
		unsigned c;

		This->pwqueue = (DWORD)-1; /* resetting queues */
		waveOutReset(This->hwo);
		for (c=0; c<DS_HEL_FRAGS; c++)
			waveOutUnprepareHeader(This->hwo, This->pwave[c], sizeof(WAVEHDR));
		This->pwqueue = 0;
	}
}

HRESULT DSOUND_PrimaryCreate(IDirectSoundImpl *This)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n",This);

	This->buflen = This->wfx.nAvgBytesPerSec;

	/* FIXME: verify that hardware capabilities (DSCAPS_PRIMARY flags) match */

	if (This->driver) {
		err = IDsDriver_CreateSoundBuffer(This->driver,&(This->wfx),
						  DSBCAPS_PRIMARYBUFFER,0,
						  &(This->buflen),&(This->buffer),
						  (LPVOID*)&(This->hwbuf));
		if (err != DS_OK) {
			WARN("IDsDriver_CreateSoundBuffer failed\n");
			return err;
		}
	}
	if (!This->hwbuf) {
		/* Allocate memory for HEL buffer headers */
		unsigned c;
		for (c=0; c<DS_HEL_FRAGS; c++) {
			This->pwave[c] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEHDR));
			if (!This->pwave[c]) {
				/* Argh, out of memory */
				while (c--) {
					HeapFree(GetProcessHeap(),0,This->pwave[c]);
				}
				WARN("out of memory\n");
				return DSERR_OUTOFMEMORY;
			}
		}
	}

	err = DSOUND_PrimaryOpen(This);

	if (err != DS_OK) {
		WARN("DSOUND_PrimaryOpen failed\n");
		return err;
	}

	/* calculate fragment size and write lead */
	DSOUND_RecalcPrimary(This);
	This->state = STATE_STOPPED;
	return DS_OK;
}

HRESULT DSOUND_PrimaryDestroy(IDirectSoundImpl *This)
{
	TRACE("(%p)\n",This);

	DSOUND_PrimaryClose(This);
	if (This->hwbuf) {
		if (IDsDriverBuffer_Release(This->hwbuf) == 0)
			This->hwbuf = 0;
	} else {
		unsigned c;
		for (c=0; c<DS_HEL_FRAGS; c++) {
			HeapFree(GetProcessHeap(),0,This->pwave[c]);
		}
	}
	return DS_OK;
}

HRESULT DSOUND_PrimaryPlay(IDirectSoundImpl *This)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n",This);

	if (This->hwbuf) {
		err = IDsDriverBuffer_Play(This->hwbuf, 0, 0, DSBPLAY_LOOPING);
		if (err != DS_OK)
			WARN("IDsDriverBuffer_Play failed\n");
	} else {
		err = mmErr(waveOutRestart(This->hwo));
		if (err != DS_OK)
			WARN("waveOutRestart failed\n");
	}

	return err;
}

HRESULT DSOUND_PrimaryStop(IDirectSoundImpl *This)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n",This);

	if (This->hwbuf) {
		err = IDsDriverBuffer_Stop(This->hwbuf);
		if (err == DSERR_BUFFERLOST) {
			DWORD flags = CALLBACK_FUNCTION;
			if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
				flags |= WAVE_DIRECTSOUND;
			/* Wine-only: the driver wants us to reopen the device */
			/* FIXME: check for errors */
			IDsDriverBuffer_Release(This->hwbuf);
			waveOutClose(This->hwo);
			This->hwo = 0;
			err = mmErr(waveOutOpen(&(This->hwo), This->drvdesc.dnDevNode,
						&(This->wfx), (DWORD)DSOUND_callback, (DWORD)This,
						flags));
			if (err == DS_OK) {
				err = IDsDriver_CreateSoundBuffer(This->driver,&(This->wfx),
								  DSBCAPS_PRIMARYBUFFER,0,
								  &(This->buflen),&(This->buffer),
								  (LPVOID)&(This->hwbuf));
				if (err != DS_OK)
					WARN("IDsDriver_CreateSoundBuffer failed\n");
			} else {
				WARN("waveOutOpen failed\n");
			}
		} else if (err != DS_OK) {
			WARN("IDsDriverBuffer_Stop failed\n");
		}
	} else {
		err = mmErr(waveOutPause(This->hwo));
		if (err != DS_OK)
			WARN("waveOutPause failed\n");
	}
	return err;
}

HRESULT DSOUND_PrimaryGetPosition(IDirectSoundImpl *This, LPDWORD playpos, LPDWORD writepos)
{
	TRACE("(%p,%p,%p)\n",This,playpos,writepos);

	if (This->hwbuf) {
		HRESULT err=IDsDriverBuffer_GetPosition(This->hwbuf,playpos,writepos);
		if (err) {
			WARN("IDsDriverBuffer_GetPosition failed\n");
			return err;
		}
	}
	else {
		if (playpos) {
			MMTIME mtime;
			mtime.wType = TIME_BYTES;
			waveOutGetPosition(This->hwo, &mtime, sizeof(mtime));
			mtime.u.cb = mtime.u.cb % This->buflen;
			*playpos = mtime.u.cb;
		}
		if (writepos) {
			/* the writepos should only be used by apps with WRITEPRIMARY priority,
			 * in which case our software mixer is disabled anyway */
			*writepos = (This->pwplay + ds_hel_margin) * This->fraglen;
			while (*writepos >= This->buflen)
				*writepos -= This->buflen;
		}
	}
	TRACE("playpos = %ld, writepos = %ld (%p, time=%ld)\n", playpos?*playpos:0, writepos?*writepos:0, This, GetTickCount());
	return DS_OK;
}


/*******************************************************************************
 *		IDirectSoundBuffer
 */
/* This sets this format for the <em>Primary Buffer Only</em> */
/* See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 120 */
static HRESULT WINAPI PrimaryBufferImpl_SetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPWAVEFORMATEX wfex
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	IDirectSoundImpl* dsound = This->dsound;
	IDirectSoundBufferImpl** dsb;
	HRESULT err = DS_OK;
	int			i;
	TRACE("(%p,%p)\n",This,wfex);

	if (This->dsound->priolevel == DSSCL_NORMAL) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	/* Let's be pedantic! */
	if (wfex == NULL) {
		WARN("invalid parameter: wfex==NULL!\n");
		return DSERR_INVALIDPARAM;
	}
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
	      "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
	      wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
	      wfex->nAvgBytesPerSec, wfex->nBlockAlign,
	      wfex->wBitsPerSample, wfex->cbSize);

	if ((wfex->wFormatTag != WAVE_FORMAT_PCM) ||
	    (wfex->nChannels < 1) || (wfex->nChannels > 2) ||
	    (wfex->nSamplesPerSec < 1) ||
	    ((wfex->wBitsPerSample != 8) && (wfex->wBitsPerSample != 16))) {
		WARN("invalid paramemer: unsupported format!\n");
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	RtlAcquireResourceExclusive(&(dsound->lock), TRUE);

	if (dsound->wfx.nSamplesPerSec != wfex->nSamplesPerSec) {
		dsb = dsound->buffers;
		for (i = 0; i < dsound->nrofbuffers; i++, dsb++) {
			/* **** */
			EnterCriticalSection(&((*dsb)->lock));

			(*dsb)->freqAdjust = ((*dsb)->freq << DSOUND_FREQSHIFT) /
				wfex->nSamplesPerSec;

			LeaveCriticalSection(&((*dsb)->lock));
			/* **** */
		}
	}

	dsound->wfx.nSamplesPerSec = wfex->nSamplesPerSec;
	dsound->wfx.nChannels = wfex->nChannels;
	dsound->wfx.wBitsPerSample = wfex->wBitsPerSample;
	dsound->wfx.nBlockAlign = dsound->wfx.wBitsPerSample / 8 * dsound->wfx.nChannels;
	dsound->wfx.nAvgBytesPerSec =
		dsound->wfx.nSamplesPerSec * dsound->wfx.nBlockAlign;

	if (dsound->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMSETFORMAT) {
		DWORD flags = CALLBACK_FUNCTION;
		if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
			flags |= WAVE_DIRECTSOUND;
		/* FIXME: check for errors */
		DSOUND_PrimaryClose(dsound);
		waveOutClose(dsound->hwo);
		dsound->hwo = 0;
                err = mmErr(waveOutOpen(&(dsound->hwo), dsound->drvdesc.dnDevNode,
                                        &(dsound->wfx), (DWORD)DSOUND_callback, (DWORD)dsound,
                                        flags));
                if (err == DS_OK) {
                    err = DSOUND_PrimaryOpen(dsound);
		    if (err != DS_OK) {
			    WARN("DSOUND_PrimaryOpen failed\n");
			    RtlReleaseResource(&(dsound->lock));
			    return err;
		    }
		} else {
			WARN("waveOutOpen failed\n");
			RtlReleaseResource(&(dsound->lock));
			return err;
		}
	}
	if (dsound->hwbuf) {
		err = IDsDriverBuffer_SetFormat(dsound->hwbuf, &(dsound->wfx));
		if (err == DSERR_BUFFERLOST) {
			/* Wine-only: the driver wants us to recreate the HW buffer */
			IDsDriverBuffer_Release(dsound->hwbuf);
			err = IDsDriver_CreateSoundBuffer(dsound->driver,&(dsound->wfx),
							  DSBCAPS_PRIMARYBUFFER,0,
							  &(dsound->buflen),&(dsound->buffer),
							  (LPVOID)&(dsound->hwbuf));
			if (err != DS_OK) {
				WARN("IDsDriver_CreateSoundBuffer failed\n");
				RtlReleaseResource(&(dsound->lock));
				return err;
			}
			if (dsound->state == STATE_PLAYING) dsound->state = STATE_STARTING;
			else if (dsound->state == STATE_STOPPING) dsound->state = STATE_STOPPED;
		} else {
			WARN("IDsDriverBuffer_SetFormat failed\n");
			RtlReleaseResource(&(dsound->lock));
			return err;
		}
                /* FIXME: should we set err back to DS_OK in all cases ? */
	}
	DSOUND_RecalcPrimary(dsound);

	RtlReleaseResource(&(dsound->lock));
	/* **** */

	return err;
}

static HRESULT WINAPI PrimaryBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LONG vol
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	IDirectSoundImpl* dsound = This->dsound;
	LONG oldVol;

	TRACE("(%p,%ld)\n",This,vol);

	/* I'm not sure if we need this for primary buffer */
	if (!(This->dsound->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN)) {
		WARN("invalid parameter: vol = %ld\n", vol);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(dsound->mixlock));

	oldVol = dsound->volpan.lVolume;
	dsound->volpan.lVolume = vol;
	DSOUND_RecalcVolPan(&dsound->volpan);

	if (vol != oldVol) {
		if (dsound->hwbuf) {
			HRESULT hres;
			hres = IDsDriverBuffer_SetVolumePan(dsound->hwbuf, &(dsound->volpan));
			if (hres != DS_OK) {
				LeaveCriticalSection(&(dsound->mixlock));
				WARN("IDsDriverBuffer_SetVolumePan failed\n");
				return hres;
			}
		}
		else {
#if 0 /* should we really do this? */
			/* the DS volume ranges from 0 (max, 0dB attenuation) to -10000 (min, 100dB attenuation) */
			/* the MM volume ranges from 0 to 0xffff in an unspecified logarithmic scale */
			WORD cvol = 0xffff + vol*6 + vol/2;
			DWORD vol = cvol | ((DWORD)cvol << 16)
			waveOutSetVolume(dsound->hwo, vol);
#endif
		}
	}

	LeaveCriticalSection(&(dsound->mixlock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG vol
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%p)\n",This,vol);

	if (!(This->dsound->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (vol == NULL) {
		WARN("invalid parameter: vol = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*vol = This->dsound->volpan.lVolume;
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,DWORD freq
) {
	ICOM_THIS(PrimaryBufferImpl,iface);

	TRACE("(%p,%ld)\n",This,freq);

	/* You cannot set the frequency of the primary buffer */
	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_Play(
	LPDIRECTSOUNDBUFFER8 iface,DWORD reserved1,DWORD reserved2,DWORD flags
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	IDirectSoundImpl* dsound = This->dsound;

	TRACE("(%p,%08lx,%08lx,%08lx)\n",
		This,reserved1,reserved2,flags
	);

	if (!(flags & DSBPLAY_LOOPING)) {
		WARN("invalid parameter: flags = %08lx\n", flags);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(dsound->mixlock));

	if (dsound->state == STATE_STOPPED)
		dsound->state = STATE_STARTING;
	else if (dsound->state == STATE_STOPPING)
		dsound->state = STATE_PLAYING;

	LeaveCriticalSection(&(dsound->mixlock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Stop(LPDIRECTSOUNDBUFFER8 iface)
{
	ICOM_THIS(PrimaryBufferImpl,iface);
	IDirectSoundImpl* dsound = This->dsound;

	TRACE("(%p)\n",This);

	/* **** */
	EnterCriticalSection(&(dsound->mixlock));

	if (dsound->state == STATE_PLAYING)
		dsound->state = STATE_STOPPING;
	else if (dsound->state == STATE_STARTING)
		dsound->state = STATE_STOPPED;

	LeaveCriticalSection(&(dsound->mixlock));
	/* **** */

	return DS_OK;
}

static DWORD WINAPI PrimaryBufferImpl_AddRef(LPDIRECTSOUNDBUFFER8 iface) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	DWORD ref;

	TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());
	ref = InterlockedIncrement(&(This->ref));

	return ref;
}

static DWORD WINAPI PrimaryBufferImpl_Release(LPDIRECTSOUNDBUFFER8 iface) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	DWORD ref;

	TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());
	ref = InterlockedDecrement(&(This->ref));

	if (ref == -1) {
		This->dsound->primary = NULL;
		HeapFree(GetProcessHeap(),0,This);
	}

	return ref;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD playpos,LPDWORD writepos
) {
	HRESULT	hres;
	ICOM_THIS(PrimaryBufferImpl,iface);
	IDirectSoundImpl* dsound = This->dsound;

	TRACE("(%p,%p,%p)\n",This,playpos,writepos);
	hres = DSOUND_PrimaryGetPosition(dsound, playpos, writepos);
	if (hres != DS_OK) {
		WARN("DSOUND_PrimaryGetPosition failed\n");
		return hres;
	}
	if (writepos) {
		if (dsound->state != STATE_STOPPED)
			/* apply the documented 10ms lead to writepos */
			*writepos += dsound->writelead;
		while (*writepos >= dsound->buflen) *writepos -= dsound->buflen;
	}
	TRACE("playpos = %ld, writepos = %ld (%p, time=%ld)\n", playpos?*playpos:0, writepos?*writepos:0, This, GetTickCount());
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD status
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%p), thread is %04lx\n",This,status,GetCurrentThreadId());

	if (status == NULL) {
		WARN("invalid parameter: status == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*status = 0;
	if ((This->dsound->state == STATE_STARTING) ||
	    (This->dsound->state == STATE_PLAYING))
		*status |= DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;

	TRACE("status=%lx\n", *status);
	return DS_OK;
}


static HRESULT WINAPI PrimaryBufferImpl_GetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPWAVEFORMATEX lpwf,DWORD wfsize,LPDWORD wfwritten
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%p,%ld,%p)\n",This,lpwf,wfsize,wfwritten);

	if (wfsize>sizeof(This->dsound->wfx))
		wfsize = sizeof(This->dsound->wfx);
	if (lpwf) {	/* NULL is valid */
		memcpy(lpwf,&(This->dsound->wfx),wfsize);
		if (wfwritten)
			*wfwritten = wfsize;
	} else {
		if (wfwritten)
			*wfwritten = sizeof(This->dsound->wfx);
		else {
			WARN("invalid parameter: wfwritten == NULL\n");
			return DSERR_INVALIDPARAM;
		}
	}

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Lock(
	LPDIRECTSOUNDBUFFER8 iface,DWORD writecursor,DWORD writebytes,LPVOID lplpaudioptr1,LPDWORD audiobytes1,LPVOID lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	IDirectSoundImpl* dsound = This->dsound;

	TRACE("(%p,%ld,%ld,%p,%p,%p,%p,0x%08lx) at %ld\n",
		This,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags,
		GetTickCount()
	);

	if (dsound->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	if (flags & DSBLOCK_FROMWRITECURSOR) {
		DWORD writepos;
		HRESULT hres;
		/* GetCurrentPosition does too much magic to duplicate here */
		hres = IDirectSoundBuffer_GetCurrentPosition(iface, NULL, &writepos);
		if (hres != DS_OK) {
			WARN("IDirectSoundBuffer_GetCurrentPosition failed\n");
			return hres;
		}
		writecursor += writepos;
	}
	while (writecursor >= dsound->buflen)
		writecursor -= dsound->buflen;
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = dsound->buflen;
	if (writebytes > dsound->buflen)
		writebytes = dsound->buflen;

	assert(audiobytes1!=audiobytes2);
	assert(lplpaudioptr1!=lplpaudioptr2);

	if (!(dsound->drvdesc.dwFlags & DSDDESC_DONTNEEDPRIMARYLOCK) && dsound->hwbuf) {
		HRESULT hres;
		hres = IDsDriverBuffer_Lock(dsound->hwbuf,
					    lplpaudioptr1, audiobytes1,
					    lplpaudioptr2, audiobytes2,
					    writecursor, writebytes,
					    0);
		if (hres != DS_OK) {
			WARN("IDsDriverBuffer_Lock failed\n");
			return hres;
		}
	} else {
		if (writecursor+writebytes <= dsound->buflen) {
			*(LPBYTE*)lplpaudioptr1 = dsound->buffer+writecursor;
			*audiobytes1 = writebytes;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = NULL;
			if (audiobytes2)
				*audiobytes2 = 0;
			TRACE("->%ld.0\n",writebytes);
		} else {
			*(LPBYTE*)lplpaudioptr1 = dsound->buffer+writecursor;
			*audiobytes1 = dsound->buflen-writecursor;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = dsound->buffer;
			if (audiobytes2)
				*audiobytes2 = writebytes-(dsound->buflen-writecursor);
			TRACE("->%ld.%ld\n",*audiobytes1,audiobytes2?*audiobytes2:0);
		}
	}
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,DWORD newpos
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%ld)\n",This,newpos);

	/* You cannot set the position of the primary buffer */
	WARN("invalid call\n");
	return DSERR_INVALIDCALL;
}

static HRESULT WINAPI PrimaryBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER8 iface,LONG pan
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%ld)\n",This,pan);

	/* You cannot set the pan of the primary buffer */
	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_GetPan(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG pan
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%p)\n",This,pan);

	if (pan == NULL) {
		WARN("invalid parameter: pan == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*pan = This->dsound->volpan.lPan;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Unlock(
	LPDIRECTSOUNDBUFFER8 iface,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	IDirectSoundImpl* dsound = This->dsound;

	TRACE("(%p,%p,%ld,%p,%ld):stub\n", This,p1,x1,p2,x2);

	if (dsound->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	if (!(dsound->drvdesc.dwFlags & DSDDESC_DONTNEEDPRIMARYLOCK) && dsound->hwbuf) {
		HRESULT	hres;
		
		hres = IDsDriverBuffer_Unlock(dsound->hwbuf, p1, x1, p2, x2);
		if (hres != DS_OK) {
			WARN("IDsDriverBuffer_Unlock failed\n");
			return hres;
		}
	}

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Restore(
	LPDIRECTSOUNDBUFFER8 iface
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD freq
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%p)\n",This,freq);

	if (freq == NULL) {
		WARN("invalid parameter: freq == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (!(This->dsound->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	*freq = This->dsound->wfx.nSamplesPerSec;
	TRACE("-> %ld\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetFX(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwEffectsCount,LPDSEFFECTDESC pDSFXDesc,LPDWORD pdwResultCodes
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	DWORD u;

	FIXME("(%p,%lu,%p,%p): stub\n",This,dwEffectsCount,pDSFXDesc,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_AcquireResources(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwFlags,DWORD dwEffectsCount,LPDWORD pdwResultCodes
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	DWORD u;

	FIXME("(%p,%08lu,%lu,%p): stub\n",This,dwFlags,dwEffectsCount,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_GetObjectInPath(
	LPDIRECTSOUNDBUFFER8 iface,REFGUID rguidObject,DWORD dwIndex,REFGUID rguidInterface,LPVOID* ppObject
) {
	ICOM_THIS(PrimaryBufferImpl,iface);

	FIXME("(%p,%s,%lu,%s,%p): stub\n",This,debugstr_guid(rguidObject),dwIndex,debugstr_guid(rguidInterface),ppObject);

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER8 iface,LPDIRECTSOUND8 dsound,LPDSBUFFERDESC dbsd
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	FIXME("(%p,%p,%p):stub\n",This,dsound,dbsd);
	DPRINTF("Re-Init!!!\n");
	WARN("already initialized\n");
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCaps(
	LPDIRECTSOUNDBUFFER8 iface,LPDSBCAPS caps
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
  	TRACE("(%p)->(%p)\n",This,caps);

	if (caps == NULL) {
		WARN("invalid parameter: caps == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (caps->dwSize < sizeof(*caps)) {
		WARN("invalid parameter: caps->dwSize = %ld: < %d\n", caps->dwSize, sizeof(*caps));
		return DSERR_INVALIDPARAM;
	}

	caps->dwFlags = This->dsound->dsbd.dwFlags;
	if (This->dsound->hwbuf) caps->dwFlags |= DSBCAPS_LOCHARDWARE;
	else caps->dwFlags |= DSBCAPS_LOCSOFTWARE;

	caps->dwBufferBytes = This->dsound->buflen;

	/* This value represents the speed of the "unlock" command.
	   As unlock is quite fast (it does not do anything), I put
	   4096 ko/s = 4 Mo / s */
	/* FIXME: hwbuf speed */
	caps->dwUnlockTransferRate = 4096;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER8 iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(PrimaryBufferImpl,iface);
	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

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
		if (This->dsound->dsbd.dwFlags & DSBCAPS_CTRL3D) {
			if (!This->dsound->listener)
				IDirectSound3DListenerImpl_Create(This, &This->dsound->listener);
	
			*ppobj = This->dsound->listener;
	
			if (This->dsound->listener) {
				IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)*ppobj);
				return S_OK;
			}
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

static ICOM_VTABLE(IDirectSoundBuffer8) dspbvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	PrimaryBufferImpl_Restore,
	PrimaryBufferImpl_SetFX,
	PrimaryBufferImpl_AcquireResources,
	PrimaryBufferImpl_GetObjectInPath
};

HRESULT WINAPI PrimaryBuffer_Create(
	IDirectSoundImpl *This,
	PrimaryBufferImpl **pdsb,
	LPDSBUFFERDESC dsbd)
{
	PrimaryBufferImpl *dsb;

	TRACE("%p,%p,%p)\n",This,pdsb,dsbd);

	if (dsbd->lpwfxFormat) {
		WARN("invalid parameter: dsbd->lpwfxFormat != NULL\n");
		*pdsb = NULL;
		return DSERR_INVALIDPARAM;
	}

	dsb = (PrimaryBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));

	if (dsb == NULL) {
		WARN("out of memory\n");
		*pdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

	dsb->ref = -1;
	dsb->dsound = This;
	dsb->lpVtbl = &dspbvt;

	memcpy(&This->dsbd, dsbd, sizeof(*dsbd));

	TRACE("Created primary buffer at %p\n", dsb);
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
		"bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		This->wfx.wFormatTag, This->wfx.nChannels, This->wfx.nSamplesPerSec,
		This->wfx.nAvgBytesPerSec, This->wfx.nBlockAlign,
		This->wfx.wBitsPerSample, This->wfx.cbSize);

	*pdsb = dsb;
	return S_OK;
}
