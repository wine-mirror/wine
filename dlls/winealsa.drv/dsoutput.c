/*
 * Sample Wine Driver for Advanced Linux Sound System (ALSA)
 *      Based on version <final> of the ALSA API
 *
 * Copyright    2002 Eric Pouech
 *              2002 Marco Pietrobono
 *              2003 Christian Costa : WaveIn support
 *              2006-2007 Maarten Lankhorst
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

/*======================================================================*
 *              Low level dsound output implementation			*
 *======================================================================*/

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "winnls.h"
#include "mmddk.h"

#include "alsa.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#ifdef HAVE_ALSA

WINE_DEFAULT_DEBUG_CHANNEL(wave);

/*======================================================================*
 *                  Low level DSOUND implementation			*
 *======================================================================*/

typedef struct IDsDriverImpl IDsDriverImpl;
typedef struct IDsDriverBufferImpl IDsDriverBufferImpl;

struct IDsDriverImpl
{
    /* IUnknown fields */
    const IDsDriverVtbl *lpVtbl;
    LONG		ref;
    /* IDsDriverImpl fields */
    UINT		wDevID;
    IDsDriverBufferImpl*primary;
};

struct IDsDriverBufferImpl
{
    /* IUnknown fields */
    const IDsDriverBufferVtbl *lpVtbl;
    LONG		      ref;
    /* IDsDriverBufferImpl fields */
    IDsDriverImpl*	      drv;

    CRITICAL_SECTION          mmap_crst;
    LPVOID                    mmap_buffer;
    DWORD                     mmap_buflen_bytes;
    snd_pcm_uframes_t         mmap_buflen_frames;
    snd_pcm_channel_area_t *  mmap_areas;
    snd_async_handler_t *     mmap_async_handler;
    snd_pcm_uframes_t	      mmap_ppos; /* play position */

    /* Do we have a direct hardware buffer - SND_PCM_TYPE_HW? */
    int                       mmap_mode;
};

static void DSDB_CheckXRUN(IDsDriverBufferImpl* pdbi)
{
    WINE_WAVEDEV *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_state_t    state = snd_pcm_state(wwo->pcm);

    if ( state == SND_PCM_STATE_XRUN )
    {
	int            err = snd_pcm_prepare(wwo->pcm);
	TRACE("xrun occurred\n");
	if ( err < 0 )
            ERR("recovery from xrun failed, prepare failed: %s\n", snd_strerror(err));
    }
    else if ( state == SND_PCM_STATE_SUSPENDED )
    {
	int            err = snd_pcm_resume(wwo->pcm);
	TRACE("recovery from suspension occurred\n");
        if (err < 0 && err != -EAGAIN){
            err = snd_pcm_prepare(wwo->pcm);
            if (err < 0)
                ERR("recovery from suspend failed, prepare failed: %s\n", snd_strerror(err));
        }
    }
}

static void DSDB_MMAPCopy(IDsDriverBufferImpl* pdbi, int mul)
{
    WINE_WAVEDEV *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_uframes_t  period_size;
    snd_pcm_sframes_t  avail;
    int err;
    int dir=0;

    const snd_pcm_channel_area_t *areas;
    snd_pcm_uframes_t     ofs;
    snd_pcm_uframes_t     frames;
    snd_pcm_uframes_t     wanted;

    if ( !pdbi->mmap_buffer || !wwo->hw_params || !wwo->pcm)
	return;

    err = snd_pcm_hw_params_get_period_size(wwo->hw_params, &period_size, &dir);
    avail = snd_pcm_avail_update(wwo->pcm);

    DSDB_CheckXRUN(pdbi);

    TRACE("avail=%d, mul=%d\n", (int)avail, mul);

    frames = pdbi->mmap_buflen_frames;

    EnterCriticalSection(&pdbi->mmap_crst);

    /* we want to commit the given number of periods, or the whole lot */
    wanted = mul == 0 ? frames : period_size * 2;

    snd_pcm_mmap_begin(wwo->pcm, &areas, &ofs, &frames);
    if (areas != pdbi->mmap_areas || areas->addr != pdbi->mmap_areas->addr)
        FIXME("Can't access sound driver's buffer directly.\n");

    /* mark our current play position */
    pdbi->mmap_ppos = ofs;

    if (frames > wanted)
        frames = wanted;

    err = snd_pcm_mmap_commit(wwo->pcm, ofs, frames);

    /* Check to make sure we committed all we want to commit. ALSA
     * only gives a contiguous linear region, so we need to check this
     * in case we've reached the end of the buffer, in which case we
     * can wrap around back to the beginning. */
    if (frames < wanted) {
        frames = wanted -= frames;
        snd_pcm_mmap_begin(wwo->pcm, &areas, &ofs, &frames);
        snd_pcm_mmap_commit(wwo->pcm, ofs, frames);
    }

    LeaveCriticalSection(&pdbi->mmap_crst);
}

static void DSDB_PCMCallback(snd_async_handler_t *ahandler)
{
    int periods;
    /* snd_pcm_t *               handle = snd_async_handler_get_pcm(ahandler); */
    IDsDriverBufferImpl*      pdbi = snd_async_handler_get_callback_private(ahandler);
    TRACE("callback called\n");

    /* Commit another block (the entire buffer if it's a direct hw buffer) */
    periods = pdbi->mmap_mode == SND_PCM_TYPE_HW ? 0 : 1;
    DSDB_MMAPCopy(pdbi, periods);
}

/**
 * Allocate the memory-mapped buffer for direct sound, and set up the
 * callback.
 */
static int DSDB_CreateMMAP(IDsDriverBufferImpl* pdbi)
{
    WINE_WAVEDEV *            wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_format_t          format;
    snd_pcm_uframes_t         frames;
    snd_pcm_uframes_t         ofs;
    snd_pcm_uframes_t         avail;
    unsigned int              channels;
    unsigned int              bits_per_sample;
    unsigned int              bits_per_frame;
    int                       err;

    err = snd_pcm_hw_params_get_format(wwo->hw_params, &format);
    err = snd_pcm_hw_params_get_buffer_size(wwo->hw_params, &frames);
    err = snd_pcm_hw_params_get_channels(wwo->hw_params, &channels);
    bits_per_sample = snd_pcm_format_physical_width(format);
    bits_per_frame = bits_per_sample * channels;
    pdbi->mmap_mode = snd_pcm_type(wwo->pcm);

    if (pdbi->mmap_mode == SND_PCM_TYPE_HW) {
        TRACE("mmap'd buffer is a hardware buffer.\n");
    }
    else {
        TRACE("mmap'd buffer is an ALSA emulation of hardware buffer.\n");
    }

    if (TRACE_ON(wave))
	ALSA_TraceParameters(wwo->hw_params, NULL, FALSE);

    TRACE("format=%s  frames=%ld  channels=%d  bits_per_sample=%d  bits_per_frame=%d\n",
          snd_pcm_format_name(format), frames, channels, bits_per_sample, bits_per_frame);

    pdbi->mmap_buflen_frames = frames;
    pdbi->mmap_buflen_bytes = snd_pcm_frames_to_bytes( wwo->pcm, frames );

    avail = snd_pcm_avail_update(wwo->pcm);
    if (avail < 0)
    {
	ERR("No buffer is available: %s.\n", snd_strerror(avail));
	return DSERR_GENERIC;
    }
    err = snd_pcm_mmap_begin(wwo->pcm, (const snd_pcm_channel_area_t **)&pdbi->mmap_areas, &ofs, &avail);
    if ( err < 0 )
    {
	ERR("Can't map sound device for direct access: %s\n", snd_strerror(err));
	return DSERR_GENERIC;
    }
    avail = 0;/* We don't have any data to commit yet */
    err = snd_pcm_mmap_commit(wwo->pcm, ofs, avail);
    if (ofs > 0)
	err = snd_pcm_rewind(wwo->pcm, ofs);
    pdbi->mmap_buffer = pdbi->mmap_areas->addr;

    snd_pcm_format_set_silence(format, pdbi->mmap_buffer, frames );

    TRACE("created mmap buffer of %ld frames (%d bytes) at %p\n",
        frames, pdbi->mmap_buflen_bytes, pdbi->mmap_buffer);

    err = snd_async_add_pcm_handler(&pdbi->mmap_async_handler, wwo->pcm, DSDB_PCMCallback, pdbi);
    if ( err < 0 )
    {
	ERR("add_pcm_handler failed. reason: %s\n", snd_strerror(err));
	return DSERR_GENERIC;
    }

    InitializeCriticalSection(&pdbi->mmap_crst);
    pdbi->mmap_crst.DebugInfo->Spare[0] = (DWORD_PTR)"WINEALSA_mmap_crst";

    return DS_OK;
}

static void DSDB_DestroyMMAP(IDsDriverBufferImpl* pdbi)
{
    TRACE("mmap buffer %p destroyed\n", pdbi->mmap_buffer);
    pdbi->mmap_areas = NULL;
    pdbi->mmap_buffer = NULL;
}


static HRESULT WINAPI IDsDriverBufferImpl_QueryInterface(PIDSDRIVERBUFFER iface, REFIID riid, LPVOID *ppobj)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    FIXME("(): stub!\n");
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsDriverBufferImpl_AddRef(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverBufferImpl_Release(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

    if (refCount)
	return refCount;
    if (This == This->drv->primary)
	This->drv->primary = NULL;
    DSDB_DestroyMMAP(This);
    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI IDsDriverBufferImpl_Lock(PIDSDRIVERBUFFER iface,
					       LPVOID*ppvAudio1,LPDWORD pdwLen1,
					       LPVOID*ppvAudio2,LPDWORD pdwLen2,
					       DWORD dwWritePosition,DWORD dwWriteLen,
					       DWORD dwFlags)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_Unlock(PIDSDRIVERBUFFER iface,
						 LPVOID pvAudio1,DWORD dwLen1,
						 LPVOID pvAudio2,DWORD dwLen2)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFormat(PIDSDRIVERBUFFER iface,
						    LPWAVEFORMATEX pwfx)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p,%p)\n",iface,pwfx);
    return DSERR_BUFFERLOST;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFrequency(PIDSDRIVERBUFFER iface, DWORD dwFreq)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p,%d): stub\n",iface,dwFreq);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetVolumePan(PIDSDRIVERBUFFER iface, PDSVOLUMEPAN pVolPan)
{
    DWORD vol;
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    TRACE("(%p,%p)\n",iface,pVolPan);
    vol = pVolPan->dwTotalLeftAmpFactor | (pVolPan->dwTotalRightAmpFactor << 16);

    if (wodSetVolume(This->drv->wDevID, vol) != MMSYSERR_NOERROR) {
	WARN("wodSetVolume failed\n");
	return DSERR_INVALIDPARAM;
    }

    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetPosition(PIDSDRIVERBUFFER iface, DWORD dwNewPos)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%d): stub\n",iface,dwNewPos);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_GetPosition(PIDSDRIVERBUFFER iface,
						      LPDWORD lpdwPlay, LPDWORD lpdwWrite)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    WINE_WAVEDEV *      wwo = &(WOutDev[This->drv->wDevID]);
    snd_pcm_uframes_t   hw_ptr;
    snd_pcm_uframes_t   period_size;
    snd_pcm_state_t     state;
    int dir;
    int err;

    if (wwo->hw_params == NULL) return DSERR_GENERIC;

    dir=0;
    err = snd_pcm_hw_params_get_period_size(wwo->hw_params, &period_size, &dir);

    if (wwo->pcm == NULL) return DSERR_GENERIC;
    /** we need to track down buffer underruns */
    DSDB_CheckXRUN(This);

    hw_ptr = This->mmap_ppos;

    state = snd_pcm_state(wwo->pcm);
    if (state != SND_PCM_STATE_RUNNING)
      hw_ptr = 0;

    if (lpdwPlay)
	*lpdwPlay = snd_pcm_frames_to_bytes(wwo->pcm, hw_ptr) % This->mmap_buflen_bytes;
    if (lpdwWrite)
	*lpdwWrite = snd_pcm_frames_to_bytes(wwo->pcm, hw_ptr + period_size * 2) % This->mmap_buflen_bytes;

    TRACE("hw_ptr=0x%08x, playpos=%d, writepos=%d\n", (unsigned int)hw_ptr, lpdwPlay?*lpdwPlay:-1, lpdwWrite?*lpdwWrite:-1);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    WINE_WAVEDEV *       wwo = &(WOutDev[This->drv->wDevID]);
    snd_pcm_state_t      state;
    int                  err;

    TRACE("(%p,%x,%x,%x)\n",iface,dwRes1,dwRes2,dwFlags);

    if (wwo->pcm == NULL) return DSERR_GENERIC;

    state = snd_pcm_state(wwo->pcm);
    if ( state == SND_PCM_STATE_SETUP )
    {
	err = snd_pcm_prepare(wwo->pcm);
        state = snd_pcm_state(wwo->pcm);
    }
    if ( state == SND_PCM_STATE_PREPARED )
    {
	/* If we have a direct hardware buffer, we can commit the whole lot
	 * immediately (periods = 0), otherwise we prime the queue with only
	 * 2 periods.
	 *
	 * Why 2? We want a small number so that we don't get ahead of the
	 * DirectSound mixer. But we don't want to ever let the buffer get
	 * completely empty - having 2 periods gives us time to commit another
	 * period when the first expires.
	 *
	 * The potential for buffer underrun is high, but that's the reality
	 * of using a translated buffer (the whole point of DirectSound is
	 * to provide direct access to the hardware).
         *
         * A better implementation would use the buffer Lock() and Unlock()
         * methods to determine how far ahead we can commit, and to rewind if
         * necessary.
	 */
	int periods = This->mmap_mode == SND_PCM_TYPE_HW ? 0 : 2;

	DSDB_MMAPCopy(This, periods);
	err = snd_pcm_start(wwo->pcm);
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Stop(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    WINE_WAVEDEV *    wwo = &(WOutDev[This->drv->wDevID]);
    int               err;
    DWORD             play;
    DWORD             write;

    TRACE("(%p)\n",iface);

    if (wwo->pcm == NULL) return DSERR_GENERIC;

    /* ring buffer wrap up detection */
    IDsDriverBufferImpl_GetPosition(iface, &play, &write);
    if ( play > write)
    {
	TRACE("writepos wrapper up\n");
	return DS_OK;
    }

    if ( ( err = snd_pcm_drop(wwo->pcm)) < 0 )
    {
	ERR("error while stopping pcm: %s\n", snd_strerror(err));
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static const IDsDriverBufferVtbl dsdbvt =
{
    IDsDriverBufferImpl_QueryInterface,
    IDsDriverBufferImpl_AddRef,
    IDsDriverBufferImpl_Release,
    IDsDriverBufferImpl_Lock,
    IDsDriverBufferImpl_Unlock,
    IDsDriverBufferImpl_SetFormat,
    IDsDriverBufferImpl_SetFrequency,
    IDsDriverBufferImpl_SetVolumePan,
    IDsDriverBufferImpl_SetPosition,
    IDsDriverBufferImpl_GetPosition,
    IDsDriverBufferImpl_Play,
    IDsDriverBufferImpl_Stop
};

static HRESULT WINAPI IDsDriverImpl_QueryInterface(PIDSDRIVER iface, REFIID riid, LPVOID *ppobj)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    FIXME("(%p): stub!\n",iface);
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsDriverImpl_AddRef(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverImpl_Release(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

    if (refCount)
	return refCount;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI IDsDriverImpl_GetDriverDesc(PIDSDRIVER iface, PDSDRIVERDESC pDesc)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pDesc);
    memcpy(pDesc, &(WOutDev[This->wDevID].ds_desc), sizeof(DSDRIVERDESC));
    pDesc->dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT |
	DSDDESC_USESYSTEMMEMORY | DSDDESC_DONTNEEDPRIMARYLOCK;
    pDesc->dnDevNode		= WOutDev[This->wDevID].waveDesc.dnDevNode;
    pDesc->wVxdId		= 0;
    pDesc->wReserved		= 0;
    pDesc->ulDeviceNum		= This->wDevID;
    pDesc->dwHeapType		= DSDHEAP_NOHEAP;
    pDesc->pvDirectDrawHeap	= NULL;
    pDesc->dwMemStartAddress	= 0;
    pDesc->dwMemEndAddress	= 0;
    pDesc->dwMemAllocExtra	= 0;
    pDesc->pvReserved1		= NULL;
    pDesc->pvReserved2		= NULL;
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Open(PIDSDRIVER iface)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Close(PIDSDRIVER iface)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p)\n",iface);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_GetCaps(PIDSDRIVER iface, PDSDRIVERCAPS pCaps)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pCaps);
    memcpy(pCaps, &(WOutDev[This->wDevID].ds_caps), sizeof(DSDRIVERCAPS));
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_CreateSoundBuffer(PIDSDRIVER iface,
						      LPWAVEFORMATEX pwfx,
						      DWORD dwFlags, DWORD dwCardAddress,
						      LPDWORD pdwcbBufferSize,
						      LPBYTE *ppbBuffer,
						      LPVOID *ppvObj)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    IDsDriverBufferImpl** ippdsdb = (IDsDriverBufferImpl**)ppvObj;
    int err;

    TRACE("(%p,%p,%x,%x)\n",iface,pwfx,dwFlags,dwCardAddress);
    /* we only support primary buffers */
    if (!(dwFlags & DSBCAPS_PRIMARYBUFFER))
	return DSERR_UNSUPPORTED;
    if (This->primary)
	return DSERR_ALLOCATED;
    if (dwFlags & (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN))
	return DSERR_CONTROLUNAVAIL;

    *ippdsdb = HeapAlloc(GetProcessHeap(),0,sizeof(IDsDriverBufferImpl));
    if (*ippdsdb == NULL)
	return DSERR_OUTOFMEMORY;
    (*ippdsdb)->lpVtbl  = &dsdbvt;
    (*ippdsdb)->ref	= 1;
    (*ippdsdb)->drv	= This;

    err = DSDB_CreateMMAP((*ippdsdb));
    if ( err != DS_OK )
     {
	HeapFree(GetProcessHeap(), 0, *ippdsdb);
	*ippdsdb = NULL;
	return err;
     }
    *ppbBuffer = (*ippdsdb)->mmap_buffer;
    *pdwcbBufferSize = (*ippdsdb)->mmap_buflen_bytes;

    This->primary = *ippdsdb;

    /* buffer is ready to go */
    TRACE("buffer created at %p\n", *ippdsdb);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_DuplicateSoundBuffer(PIDSDRIVER iface,
							 PIDSDRIVERBUFFER pBuffer,
							 LPVOID *ppvObj)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%p): stub\n",iface,pBuffer);
    return DSERR_INVALIDCALL;
}

static const IDsDriverVtbl dsdvt =
{
    IDsDriverImpl_QueryInterface,
    IDsDriverImpl_AddRef,
    IDsDriverImpl_Release,
    IDsDriverImpl_GetDriverDesc,
    IDsDriverImpl_Open,
    IDsDriverImpl_Close,
    IDsDriverImpl_GetCaps,
    IDsDriverImpl_CreateSoundBuffer,
    IDsDriverImpl_DuplicateSoundBuffer
};

DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv)
{
    IDsDriverImpl** idrv = (IDsDriverImpl**)drv;

    TRACE("driver created\n");

    /* the HAL isn't much better than the HEL if we can't do mmap() */
    if (!(WOutDev[wDevID].outcaps.dwSupport & WAVECAPS_DIRECTSOUND)) {
	ERR("DirectSound flag not set\n");
	MESSAGE("This sound card's driver does not support direct access\n");
	MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
	return MMSYSERR_NOTSUPPORTED;
    }

    *idrv = HeapAlloc(GetProcessHeap(),0,sizeof(IDsDriverImpl));
    if (!*idrv)
	return MMSYSERR_NOMEM;
    (*idrv)->lpVtbl	= &dsdvt;
    (*idrv)->ref	= 1;

    (*idrv)->wDevID	= wDevID;
    (*idrv)->primary	= NULL;
    return MMSYSERR_NOERROR;
}

DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memcpy(desc, &(WOutDev[wDevID].ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

#endif /* HAVE_ALSA */
