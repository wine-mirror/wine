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

/*******************************************************************************
 *		IDirectSoundNotify
 */
static HRESULT WINAPI IDirectSoundNotifyImpl_QueryInterface(
	LPDIRECTSOUNDNOTIFY iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	if (This->dsb)
		return IDirectSoundBuffer8_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb, riid, ppobj);
	else if (This->dscb)
		return IDirectSoundCaptureBuffer8_QueryInterface((LPDIRECTSOUNDCAPTUREBUFFER8)This->dscb, riid, ppobj);
	return DSERR_GENERIC;
}

static ULONG WINAPI IDirectSoundNotifyImpl_AddRef(LPDIRECTSOUNDNOTIFY iface) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);
	DWORD ref;

	TRACE("(%p) ref was %ld\n", This, This->ref);

	ref = InterlockedIncrement(&(This->ref));
	return ref;
}

static ULONG WINAPI IDirectSoundNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);
	DWORD ref;

	TRACE("(%p) ref was %ld\n", This, This->ref);

	ref = InterlockedDecrement(&(This->ref));
	if (!ref) {
		if (This->dsb)
			IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->dsb);
		else if (This->dscb)
			IDirectSoundCaptureBuffer8_Release((LPDIRECTSOUNDCAPTUREBUFFER8)This->dscb);
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return ref;
}

static HRESULT WINAPI IDirectSoundNotifyImpl_SetNotificationPositions(
	LPDIRECTSOUNDNOTIFY iface,DWORD howmuch,LPCDSBPOSITIONNOTIFY notify
) {
	ICOM_THIS(IDirectSoundNotifyImpl,iface);
	int	i;

	if (TRACE_ON(dsound)) {
	    TRACE("(%p,0x%08lx,%p)\n",This,howmuch,notify);
	    for (i=0;i<howmuch;i++)
		    TRACE("notify at %ld to 0x%08lx\n",
                            notify[i].dwOffset,(DWORD)notify[i].hEventNotify);
	}
	if (This->dsb) {
		This->dsb->notifies = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->dsb->notifies,(This->dsb->nrofnotifies+howmuch)*sizeof(DSBPOSITIONNOTIFY));
		memcpy(	This->dsb->notifies+This->dsb->nrofnotifies,
			notify,
			howmuch*sizeof(DSBPOSITIONNOTIFY)
		);
		This->dsb->nrofnotifies+=howmuch;
	} else if (This->dscb) {
		TRACE("notifies = %p, nrofnotifies = %d\n", This->dscb->notifies, This->dscb->nrofnotifies);
		This->dscb->notifies = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->dscb->notifies,(This->dscb->nrofnotifies+howmuch)*sizeof(DSBPOSITIONNOTIFY));
		memcpy(	This->dscb->notifies+This->dscb->nrofnotifies,
			notify,
			howmuch*sizeof(DSBPOSITIONNOTIFY)
		);
		This->dscb->nrofnotifies+=howmuch;
		TRACE("notifies = %p, nrofnotifies = %d\n", This->dscb->notifies, This->dscb->nrofnotifies);
	}
	else
		return DSERR_INVALIDPARAM;

	return S_OK;
}

ICOM_VTABLE(IDirectSoundNotify) dsnvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectSoundNotifyImpl_QueryInterface,
	IDirectSoundNotifyImpl_AddRef,
	IDirectSoundNotifyImpl_Release,
	IDirectSoundNotifyImpl_SetNotificationPositions,
};

/*******************************************************************************
 *		IDirectSoundBuffer
 */

static HRESULT WINAPI IDirectSoundBufferImpl_SetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPWAVEFORMATEX wfex
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);

	TRACE("(%p,%p)\n",This,wfex);
	/* This method is not available on secondary buffers */
	return DSERR_INVALIDCALL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LONG vol
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	LONG oldVol;

	TRACE("(%p,%ld)\n",This,vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME))
		return DSERR_CONTROLUNAVAIL;

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN))
		return DSERR_INVALIDPARAM;

	/* **** */
	EnterCriticalSection(&(This->lock));

	if (This->dsbd.dwFlags & DSBCAPS_CTRL3D) {
		oldVol = This->ds3db->lVolume;
		This->ds3db->lVolume = vol;
	} else {
		oldVol = This->volpan.lVolume;
		This->volpan.lVolume = vol;
		if (vol != oldVol) DSOUND_RecalcVolPan(&(This->volpan));
	}

	if (vol != oldVol) {
		if (This->hwbuf) {
			IDsDriverBuffer_SetVolumePan(This->hwbuf, &(This->volpan));
		}
		else DSOUND_ForceRemix(This);
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG vol
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,vol);

	if (vol == NULL)
		return DSERR_INVALIDPARAM;

	if (This->dsbd.dwFlags & DSBCAPS_CTRL3D)
		*vol = This->ds3db->lVolume;
	else
		*vol = This->volpan.lVolume;
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,DWORD freq
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD oldFreq;

	TRACE("(%p,%ld)\n",This,freq);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY))
		return DSERR_CONTROLUNAVAIL;

	if (freq == DSBFREQUENCY_ORIGINAL)
		freq = This->wfx.nSamplesPerSec;

	if ((freq < DSBFREQUENCY_MIN) || (freq > DSBFREQUENCY_MAX))
		return DSERR_INVALIDPARAM;

	/* **** */
	EnterCriticalSection(&(This->lock));

	oldFreq = This->freq;
	This->freq = freq;
	if (freq != oldFreq) {
		This->freqAdjust = (freq << DSOUND_FREQSHIFT) / This->dsound->wfx.nSamplesPerSec;
		This->nAvgBytesPerSec = freq * This->wfx.nBlockAlign;
		DSOUND_RecalcFormat(This);
		if (!This->hwbuf) DSOUND_ForceRemix(This);
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Play(
	LPDIRECTSOUNDBUFFER8 iface,DWORD reserved1,DWORD reserved2,DWORD flags
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%08lx,%08lx,%08lx)\n",
		This,reserved1,reserved2,flags
	);

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->playflags = flags;
	if (This->state == STATE_STOPPED) {
		This->leadin = TRUE;
		This->startpos = This->buf_mixpos;
		This->state = STATE_STARTING;
	} else if (This->state == STATE_STOPPING)
		This->state = STATE_PLAYING;
	if (This->hwbuf) {
		IDsDriverBuffer_Play(This->hwbuf, 0, 0, This->playflags);
		This->state = STATE_PLAYING;
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Stop(LPDIRECTSOUNDBUFFER8 iface)
{
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p)\n",This);

	/* **** */
	EnterCriticalSection(&(This->lock));

	if (This->state == STATE_PLAYING)
		This->state = STATE_STOPPING;
	else if (This->state == STATE_STARTING)
		This->state = STATE_STOPPED;
	if (This->hwbuf) {
		IDsDriverBuffer_Stop(This->hwbuf);
		This->state = STATE_STOPPED;
	}
	DSOUND_CheckEvent(This, 0);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static DWORD WINAPI IDirectSoundBufferImpl_AddRef(LPDIRECTSOUNDBUFFER8 iface) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD ref;

	TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

	ref = InterlockedIncrement(&(This->ref));
	if (!ref) {
		FIXME("thread-safety alert! AddRef-ing with a zero refcount!\n");
	}
	return ref;
}
static DWORD WINAPI IDirectSoundBufferImpl_Release(LPDIRECTSOUNDBUFFER8 iface) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	int	i;
	DWORD ref;

	TRACE("(%p) ref was %ld, thread is %04lx\n",This, This->ref, GetCurrentThreadId());

	ref = InterlockedDecrement(&(This->ref));
	if (ref) return ref;

	RtlAcquireResourceExclusive(&(This->dsound->lock), TRUE);
	for (i=0;i<This->dsound->nrofbuffers;i++)
		if (This->dsound->buffers[i] == This)
			break;

	if (i < This->dsound->nrofbuffers) {
		/* Put the last buffer of the list in the (now empty) position */
		This->dsound->buffers[i] = This->dsound->buffers[This->dsound->nrofbuffers - 1];
		This->dsound->nrofbuffers--;
		This->dsound->buffers = HeapReAlloc(GetProcessHeap(),0,This->dsound->buffers,sizeof(LPDIRECTSOUNDBUFFER8)*This->dsound->nrofbuffers);
		TRACE("buffer count is now %d\n", This->dsound->nrofbuffers);
		IDirectSound_Release((LPDIRECTSOUND)This->dsound);
	}
	RtlReleaseResource(&(This->dsound->lock));

	DeleteCriticalSection(&(This->lock));
	if (This->hwbuf) {
		IDsDriverBuffer_Release(This->hwbuf);
		if (This->dsound->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY)
			HeapFree(GetProcessHeap(),0,This->buffer);
	}
	else if (!This->parent)
		HeapFree(GetProcessHeap(),0,This->buffer);
	if (This->ds3db) {
		DeleteCriticalSection(&This->ds3db->lock);
		HeapFree(GetProcessHeap(), 0, This->ds3db);
	}
	if (This->iks) {
		HeapFree(GetProcessHeap(), 0, This->iks);
	}
	if (This->parent)
		IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->parent);

	HeapFree(GetProcessHeap(),0,This);

	return 0;
}

DWORD DSOUND_CalcPlayPosition(IDirectSoundBufferImpl *This,
			      DWORD state, DWORD pplay, DWORD pwrite, DWORD pmix, DWORD bmix)
{
	DWORD bplay;

	TRACE("primary playpos=%ld, mixpos=%ld\n", pplay, pmix);
	TRACE("this mixpos=%ld, time=%ld\n", bmix, GetTickCount());

	/* the actual primary play position (pplay) is always behind last mixed (pmix),
	 * unless the computer is too slow or something */
	/* we need to know how far away we are from there */
#if 0 /* we'll never fill the primary entirely */
	if (pmix == pplay) {
		if ((state == STATE_PLAYING) || (state == STATE_STOPPING)) {
			/* wow, the software mixer is really doing well,
			 * seems the entire primary buffer is filled! */
			pmix += This->dsound->buflen;
		}
		/* else: the primary buffer is not playing, so probably empty */
	}
#endif
	if (pmix < pplay) pmix += This->dsound->buflen; /* wraparound */
	pmix -= pplay;
	/* detect buffer underrun */
	if (pwrite < pplay) pwrite += This->dsound->buflen; /* wraparound */
	pwrite -= pplay;
	if (pmix > (ds_snd_queue_max * This->dsound->fraglen + pwrite + This->dsound->writelead)) {
		WARN("detected an underrun: primary queue was %ld\n",pmix);
		pmix = 0;
	}
	/* divide the offset by its sample size */
	pmix /= This->dsound->wfx.nBlockAlign;
	TRACE("primary back-samples=%ld\n",pmix);
	/* adjust for our frequency */
	pmix = (pmix * This->freqAdjust) >> DSOUND_FREQSHIFT;
	/* multiply by our own sample size */
	pmix *= This->wfx.nBlockAlign;
	TRACE("this back-offset=%ld\n", pmix);
	/* subtract from our last mixed position */
	bplay = bmix;
	while (bplay < pmix) bplay += This->buflen; /* wraparound */
	bplay -= pmix;
	if (This->leadin && ((bplay < This->startpos) || (bplay > bmix))) {
		/* seems we haven't started playing yet */
		TRACE("this still in lead-in phase\n");
		bplay = This->startpos;
	}
	/* return the result */
	return bplay;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD playpos,LPDWORD writepos
) {
	HRESULT	hres;
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p,%p)\n",This,playpos,writepos);
	if (This->hwbuf) {
		hres=IDsDriverBuffer_GetPosition(This->hwbuf,playpos,writepos);
		if (hres)
		    return hres;
	}
	else {
		if (playpos && (This->state != STATE_PLAYING)) {
			/* we haven't been merged into the primary buffer (yet) */
			*playpos = This->buf_mixpos;
		}
		else if (playpos) {
			DWORD pplay, pwrite, lplay, splay, pstate;
			/* let's get this exact; first, recursively call GetPosition on the primary */
			EnterCriticalSection(&(This->dsound->mixlock));
			DSOUND_PrimaryGetPosition(This->dsound, &pplay, &pwrite);
			/* detect HEL mode underrun */
			pstate = This->dsound->state;
			if (!(This->dsound->hwbuf || This->dsound->pwqueue)) {
				TRACE("detected an underrun\n");
				/* pplay = ? */
				if (pstate == STATE_PLAYING)
					pstate = STATE_STARTING;
				else if (pstate == STATE_STOPPING)
					pstate = STATE_STOPPED;
			}
			/* get data for ourselves while we still have the lock */
			pstate &= This->state;
			lplay = This->primary_mixpos;
			splay = This->buf_mixpos;
			if ((This->dsbd.dwFlags & DSBCAPS_GETCURRENTPOSITION2) || This->dsound->hwbuf) {
				/* calculate play position using this */
				*playpos = DSOUND_CalcPlayPosition(This, pstate, pplay, pwrite, lplay, splay);
			} else {
				/* (unless the app isn't using GETCURRENTPOSITION2) */
				/* don't know exactly how this should be handled...
				 * the docs says that play cursor is reported as directly
				 * behind write cursor, hmm... */
				/* let's just do what might work for Half-Life */
				DWORD wp;
				wp = (This->dsound->pwplay + ds_hel_margin) * This->dsound->fraglen;
				while (wp >= This->dsound->buflen)
					wp -= This->dsound->buflen;
				*playpos = DSOUND_CalcPlayPosition(This, pstate, wp, pwrite, lplay, splay);
			}
			LeaveCriticalSection(&(This->dsound->mixlock));
		}
		if (writepos) *writepos = This->buf_mixpos;
	}
	if (writepos) {
		if (This->state != STATE_STOPPED)
			/* apply the documented 10ms lead to writepos */
			*writepos += This->writelead;
		while (*writepos >= This->buflen) *writepos -= This->buflen;
	}
	if (playpos) This->last_playpos = *playpos;
	TRACE("playpos = %ld, writepos = %ld (%p, time=%ld)\n", playpos?*playpos:0, writepos?*writepos:0, This, GetTickCount());
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD status
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p), thread is %04lx\n",This,status,GetCurrentThreadId());

	if (status == NULL)
		return DSERR_INVALIDPARAM;

	*status = 0;
	if ((This->state == STATE_STARTING) || (This->state == STATE_PLAYING)) {
		*status |= DSBSTATUS_PLAYING;
		if (This->playflags & DSBPLAY_LOOPING)
			*status |= DSBSTATUS_LOOPING;
	}

	TRACE("status=%lx\n", *status);
	return DS_OK;
}


static HRESULT WINAPI IDirectSoundBufferImpl_GetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPWAVEFORMATEX lpwf,DWORD wfsize,LPDWORD wfwritten
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p,%ld,%p)\n",This,lpwf,wfsize,wfwritten);

	if (wfsize>sizeof(This->wfx))
		wfsize = sizeof(This->wfx);
	if (lpwf) {	/* NULL is valid */
		memcpy(lpwf,&(This->wfx),wfsize);
		if (wfwritten)
			*wfwritten = wfsize;
	} else
		if (wfwritten)
			*wfwritten = sizeof(This->wfx);
		else
			return DSERR_INVALIDPARAM;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Lock(
	LPDIRECTSOUNDBUFFER8 iface,DWORD writecursor,DWORD writebytes,LPVOID lplpaudioptr1,LPDWORD audiobytes1,LPVOID lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);

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

	if (flags & DSBLOCK_FROMWRITECURSOR) {
		DWORD writepos;
		/* GetCurrentPosition does too much magic to duplicate here */
		IDirectSoundBufferImpl_GetCurrentPosition(iface, NULL, &writepos);
		writecursor += writepos;
	}
	while (writecursor >= This->buflen)
		writecursor -= This->buflen;
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = This->buflen;
	if (writebytes > This->buflen)
		writebytes = This->buflen;

	assert(audiobytes1!=audiobytes2);
	assert(lplpaudioptr1!=lplpaudioptr2);

	EnterCriticalSection(&(This->lock));

	if ((writebytes == This->buflen) &&
	    ((This->state == STATE_STARTING) ||
	     (This->state == STATE_PLAYING)))
		/* some games, like Half-Life, try to be clever (not) and
		 * keep one secondary buffer, and mix sounds into it itself,
		 * locking the entire buffer every time... so we can just forget
		 * about tracking the last-written-to-position... */
		This->probably_valid_to = (DWORD)-1;
	else
		This->probably_valid_to = writecursor;

	if (!(This->dsound->drvdesc.dwFlags & DSDDESC_DONTNEEDSECONDARYLOCK) && This->hwbuf) {
		IDsDriverBuffer_Lock(This->hwbuf,
				     lplpaudioptr1, audiobytes1,
				     lplpaudioptr2, audiobytes2,
				     writecursor, writebytes,
				     0);
	}
	else {
		BOOL remix = FALSE;
		if (writecursor+writebytes <= This->buflen) {
			*(LPBYTE*)lplpaudioptr1 = This->buffer+writecursor;
			*audiobytes1 = writebytes;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = NULL;
			if (audiobytes2)
				*audiobytes2 = 0;
			TRACE("->%ld.0\n",writebytes);
		} else {
			*(LPBYTE*)lplpaudioptr1 = This->buffer+writecursor;
			*audiobytes1 = This->buflen-writecursor;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = This->buffer;
			if (audiobytes2)
				*audiobytes2 = writebytes-(This->buflen-writecursor);
			TRACE("->%ld.%ld\n",*audiobytes1,audiobytes2?*audiobytes2:0);
		}
		if (This->state == STATE_PLAYING) {
			/* if the segment between playpos and buf_mixpos is touched,
			 * we need to cancel some mixing */
			/* we'll assume that the app always calls GetCurrentPosition before
			 * locking a playing buffer, so that last_playpos is up-to-date */
			if (This->buf_mixpos >= This->last_playpos) {
				if (This->buf_mixpos > writecursor &&
				    This->last_playpos < writecursor+writebytes)
					remix = TRUE;
			}
			else {
				if (This->buf_mixpos > writecursor ||
				    This->last_playpos < writecursor+writebytes)
					remix = TRUE;
			}
			if (remix) {
				TRACE("locking prebuffered region, ouch\n");
				DSOUND_MixCancelAt(This, writecursor);
			}
		}
	}

	LeaveCriticalSection(&(This->lock));
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,DWORD newpos
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%ld)\n",This,newpos);

	/* **** */
	EnterCriticalSection(&(This->lock));

	while (newpos >= This->buflen)
		newpos -= This->buflen;
	This->buf_mixpos = newpos;
	if (This->hwbuf)
		IDsDriverBuffer_SetPosition(This->hwbuf, This->buf_mixpos);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER8 iface,LONG pan
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	LONG oldPan;

	TRACE("(%p,%ld)\n",This,pan);

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT))
		return DSERR_INVALIDPARAM;

	/* You cannot use both pan and 3D controls */
	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (This->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return DSERR_CONTROLUNAVAIL;

	/* **** */
	EnterCriticalSection(&(This->lock));

	oldPan = This->volpan.lPan;
	This->volpan.lPan = pan;

	if (pan != oldPan) {
		DSOUND_RecalcVolPan(&(This->volpan));

		if (This->hwbuf) {
			IDsDriverBuffer_SetVolumePan(This->hwbuf, &(This->volpan));
		}
		else DSOUND_ForceRemix(This);
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetPan(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG pan
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,pan);

	if (pan == NULL)
		return DSERR_INVALIDPARAM;

	*pan = This->volpan.lPan;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Unlock(
	LPDIRECTSOUNDBUFFER8 iface,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD probably_valid_to;

	TRACE("(%p,%p,%ld,%p,%ld)\n", This,p1,x1,p2,x2);

#if 0
	/* Preprocess 3D buffers... */

	/* This is highly experimental and liable to break things */
	if (This->dsbd.dwFlags & DSBCAPS_CTRL3D)
		DSOUND_Create3DBuffer(This);
#endif

	if (!(This->dsound->drvdesc.dwFlags & DSDDESC_DONTNEEDSECONDARYLOCK) && This->hwbuf) {
		IDsDriverBuffer_Unlock(This->hwbuf, p1, x1, p2, x2);
	}

	if (p2) probably_valid_to = (((LPBYTE)p2)-This->buffer) + x2;
	else probably_valid_to = (((LPBYTE)p1)-This->buffer) + x1;
	while (probably_valid_to >= This->buflen)
		probably_valid_to -= This->buflen;
	if ((probably_valid_to == 0) && ((x1+x2) == This->buflen) &&
	    ((This->state == STATE_STARTING) ||
	     (This->state == STATE_PLAYING)))
		/* see IDirectSoundBufferImpl_Lock */
		probably_valid_to = (DWORD)-1;
	This->probably_valid_to = probably_valid_to;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Restore(
	LPDIRECTSOUNDBUFFER8 iface
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD freq
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	TRACE("(%p,%p)\n",This,freq);

	if (freq == NULL)
		return DSERR_INVALIDPARAM;

	*freq = This->freq;
	TRACE("-> %ld\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetFX(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwEffectsCount,LPDSEFFECTDESC pDSFXDesc,LPDWORD pdwResultCodes
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD u;

	FIXME("(%p,%lu,%p,%p): stub\n",This,dwEffectsCount,pDSFXDesc,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_AcquireResources(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwFlags,DWORD dwEffectsCount,LPDWORD pdwResultCodes
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	DWORD u;

	FIXME("(%p,%08lu,%lu,%p): stub\n",This,dwFlags,dwEffectsCount,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetObjectInPath(
	LPDIRECTSOUNDBUFFER8 iface,REFGUID rguidObject,DWORD dwIndex,REFGUID rguidInterface,LPVOID* ppObject
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);

	FIXME("(%p,%s,%lu,%s,%p): stub\n",This,debugstr_guid(rguidObject),dwIndex,debugstr_guid(rguidInterface),ppObject);

	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER8 iface,LPDIRECTSOUND8 dsound,LPDSBUFFERDESC dbsd
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
	FIXME("(%p,%p,%p):stub\n",This,dsound,dbsd);
	DPRINTF("Re-Init!!!\n");
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCaps(
	LPDIRECTSOUNDBUFFER8 iface,LPDSBCAPS caps
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);
  	TRACE("(%p)->(%p)\n",This,caps);

	if (caps == NULL || caps->dwSize!=sizeof(*caps))
		return DSERR_INVALIDPARAM;

	caps->dwFlags = This->dsbd.dwFlags;
	if (This->hwbuf) caps->dwFlags |= DSBCAPS_LOCHARDWARE;
	else caps->dwFlags |= DSBCAPS_LOCSOFTWARE;

	caps->dwBufferBytes = This->buflen;

	/* This value represents the speed of the "unlock" command.
	   As unlock is quite fast (it does not do anything), I put
	   4096 ko/s = 4 Mo / s */
	/* FIXME: hwbuf speed */
	caps->dwUnlockTransferRate = 4096;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER8 iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectSoundBufferImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if ( IsEqualGUID( &IID_IDirectSoundNotify, riid ) ) {
		IDirectSoundNotifyImpl	*dsn;

		dsn = (IDirectSoundNotifyImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*dsn));
		dsn->ref = 1;
		dsn->dsb = This;
		dsn->dscb = 0;
		IDirectSoundBuffer8_AddRef(iface);
		ICOM_VTBL(dsn) = &dsnvt;
		*ppobj = (LPVOID)dsn;
		return S_OK;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
		if (!This->ds3db)
			IDirectSound3DBufferImpl_Create(This, &This->ds3db);
		*ppobj = This->ds3db;
		if (*ppobj) {
			IDirectSound3DBuffer_AddRef((LPDIRECTSOUND3DBUFFER)*ppobj);
			return S_OK;
		}
		return E_FAIL;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		ERR("app requested IDirectSound3DListener on secondary buffer\n");
		*ppobj = NULL;
		return E_FAIL;
	}

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
		if (!This->iks)
			IKsPropertySetImpl_Create(This, &This->iks);
		*ppobj = This->iks;
		if (*ppobj) {
			IKsPropertySet_AddRef((LPKSPROPERTYSET)*ppobj);
			return S_OK;
		}
		return E_FAIL;
	}

	if ( IsEqualGUID( &IID_IDirectSoundBuffer8, riid ) ) {
	    IDirectSoundBuffer8_AddRef(iface);
	    *ppobj = This;
	    return NO_ERROR;
	}

	FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );

	*ppobj = NULL;

	return E_NOINTERFACE;
}

static ICOM_VTABLE(IDirectSoundBuffer8) dsbvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectSoundBufferImpl_QueryInterface,
	IDirectSoundBufferImpl_AddRef,
	IDirectSoundBufferImpl_Release,
	IDirectSoundBufferImpl_GetCaps,
	IDirectSoundBufferImpl_GetCurrentPosition,
	IDirectSoundBufferImpl_GetFormat,
	IDirectSoundBufferImpl_GetVolume,
	IDirectSoundBufferImpl_GetPan,
	IDirectSoundBufferImpl_GetFrequency,
	IDirectSoundBufferImpl_GetStatus,
	IDirectSoundBufferImpl_Initialize,
	IDirectSoundBufferImpl_Lock,
	IDirectSoundBufferImpl_Play,
	IDirectSoundBufferImpl_SetCurrentPosition,
	IDirectSoundBufferImpl_SetFormat,
	IDirectSoundBufferImpl_SetVolume,
	IDirectSoundBufferImpl_SetPan,
	IDirectSoundBufferImpl_SetFrequency,
	IDirectSoundBufferImpl_Stop,
	IDirectSoundBufferImpl_Unlock,
	IDirectSoundBufferImpl_Restore,
	IDirectSoundBufferImpl_SetFX,
	IDirectSoundBufferImpl_AcquireResources,
	IDirectSoundBufferImpl_GetObjectInPath
};

HRESULT WINAPI SecondaryBuffer_Create(
	IDirectSoundImpl *This,
	IDirectSoundBufferImpl **pdsb,
	LPDSBUFFERDESC dsbd)
{
	IDirectSoundBufferImpl *dsb;
	LPWAVEFORMATEX wfex = dsbd->lpwfxFormat;
	HRESULT err = DS_OK;
	DWORD capf = 0;
	int use_hw;

	if (dsbd->dwBufferBytes < DSBSIZE_MIN || dsbd->dwBufferBytes > DSBSIZE_MAX) {
		ERR("invalid sound buffer size %ld\n", dsbd->dwBufferBytes);
		return DSERR_INVALIDPARAM; /* FIXME: which error? */
	}

	dsb = (IDirectSoundBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));
	dsb->ref = 1;
	dsb->dsound = This;
	dsb->parent = NULL;
	ICOM_VTBL(dsb) = &dsbvt;

	memcpy(&dsb->dsbd, dsbd, sizeof(*dsbd));
	if (wfex)
		memcpy(&dsb->wfx, wfex, sizeof(dsb->wfx));

	TRACE("Created buffer at %p\n", dsb);

	dsb->buflen = dsbd->dwBufferBytes;
	dsb->freq = dsbd->lpwfxFormat->nSamplesPerSec;

	/* Check necessary hardware mixing capabilities */
	if (wfex->nChannels==2) capf |= DSCAPS_SECONDARYSTEREO;
	else capf |= DSCAPS_SECONDARYMONO;
	if (wfex->wBitsPerSample==16) capf |= DSCAPS_SECONDARY16BIT;
	else capf |= DSCAPS_SECONDARY8BIT;
	use_hw = (This->drvcaps.dwFlags & capf) == capf;

	/* FIXME: check hardware sample rate mixing capabilities */
	/* FIXME: check app hints for software/hardware buffer (STATIC, LOCHARDWARE, etc) */
	/* FIXME: check whether any hardware buffers are left */
	/* FIXME: handle DSDHEAP_CREATEHEAP for hardware buffers */

	/* Allocate system memory if applicable */
	if ((This->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) || !use_hw) {
		dsb->buffer = (LPBYTE)HeapAlloc(GetProcessHeap(),0,dsb->buflen);
		if (dsb->buffer == NULL)
			err = DSERR_OUTOFMEMORY;
	}

	/* Allocate the hardware buffer */
	if (use_hw && (err == DS_OK)) {
		err = IDsDriver_CreateSoundBuffer(This->driver,wfex,dsbd->dwFlags,0,
						  &(dsb->buflen),&(dsb->buffer),
						  (LPVOID*)&(dsb->hwbuf));
	}

	if (err != DS_OK) {
		if (dsb->buffer)
			HeapFree(GetProcessHeap(),0,dsb->buffer);
		HeapFree(GetProcessHeap(),0,dsb);
		dsb = NULL;
		return err;
	}
	/* calculate fragment size and write lead */
	DSOUND_RecalcFormat(dsb);

	/* It's not necessary to initialize values to zero since */
	/* we allocated this structure with HEAP_ZERO_MEMORY... */
	dsb->playpos = 0;
	dsb->buf_mixpos = 0;
	dsb->state = STATE_STOPPED;

	dsb->freqAdjust = (dsb->freq << DSOUND_FREQSHIFT) /
		This->wfx.nSamplesPerSec;
	dsb->nAvgBytesPerSec = dsb->freq *
		dsbd->lpwfxFormat->nBlockAlign;

	if (dsbd->dwFlags & DSBCAPS_CTRL3D) {
		IDirectSound3DBufferImpl_Create(dsb, &dsb->ds3db);
	}
	else
		DSOUND_RecalcVolPan(&(dsb->volpan));

	InitializeCriticalSection(&(dsb->lock));

	/* register buffer */
	RtlAcquireResourceExclusive(&(This->lock), TRUE);
	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		IDirectSoundBufferImpl **newbuffers = (IDirectSoundBufferImpl**)HeapReAlloc(GetProcessHeap(),0,This->buffers,sizeof(IDirectSoundBufferImpl*)*(This->nrofbuffers+1));
		if (newbuffers) {
			This->buffers = newbuffers;
			This->buffers[This->nrofbuffers] = dsb;
			This->nrofbuffers++;
			TRACE("buffer count is now %d\n", This->nrofbuffers);
		} else {
			ERR("out of memory for buffer list! Current buffer count is %d\n", This->nrofbuffers);
			err = DSERR_OUTOFMEMORY;
		}
	}
	RtlReleaseResource(&(This->lock));
	IDirectSound8_AddRef((LPDIRECTSOUND8)This);

	if (err != DS_OK) {
		/* oops... */
		IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)dsb);
		*pdsb = NULL;
		return err;
	}

	*pdsb = dsb;
	return S_OK;
}
