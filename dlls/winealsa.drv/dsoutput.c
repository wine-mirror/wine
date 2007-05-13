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
WINE_DECLARE_DEBUG_CHANNEL(waveloop);

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

    LPVOID                    mmap_buffer;
    DWORD                     mmap_buflen_bytes;
    snd_pcm_uframes_t         mmap_buflen_frames;
    snd_pcm_channel_area_t *  mmap_areas;
    snd_pcm_uframes_t	      mmap_ppos; /* play position */
    snd_pcm_uframes_t         mmap_wpos; /* write position */
    HANDLE                    mmap_thread;
    ALSA_MSG_RING             mmap_ring; /* sync object */
};

static void DSDB_CheckXRUN(IDsDriverBufferImpl* pdbi)
{
    WINE_WAVEDEV *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_state_t    state = snd_pcm_state(wwo->pcm);
    snd_pcm_avail_update(wwo->pcm);

    if ( state == SND_PCM_STATE_XRUN )
    {
	int err = snd_pcm_prepare(wwo->pcm);
	WARN_(waveloop)("xrun occurred\n");
	if ( err < 0 )
            ERR("recovery from xrun failed, prepare failed: %s\n", snd_strerror(err));
    }
    else if ( state == SND_PCM_STATE_SUSPENDED )
    {
	int err = snd_pcm_resume(wwo->pcm);
	TRACE_(waveloop)("recovery from suspension occurred\n");
        if (err < 0 && err != -EAGAIN){
            err = snd_pcm_prepare(wwo->pcm);
            if (err < 0)
                ERR("recovery from suspend failed, prepare failed: %s\n", snd_strerror(err));
        }
    } else if ( state != SND_PCM_STATE_RUNNING ) {
        WARN_(waveloop)("Unhandled state: %d\n", state);
    }
}

/**
 * The helper thread for DirectSound
 *
 * Basically it does an infinite loop until it is told to die
 * 
 * snd_pcm_wait() is a call that polls the sound buffer and waits
 * until there is at least 1 period free before it returns.
 *
 * We then commit the buffer filled by the owner of this
 * IDSDriverBuffer */
static DWORD CALLBACK DBSB_MMAPLoop(LPVOID data)
{
    IDsDriverBufferImpl* pdbi = (IDsDriverBufferImpl*)data;
    WINE_WAVEDEV *wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_uframes_t frames, wanted, ofs;
    const snd_pcm_channel_area_t *areas;
    int state = WINE_WS_STOPPED;
    snd_pcm_state_t alsastate;

    TRACE_(waveloop)("0x%8p\n", data);
    TRACE("0x%8p, framelength: %lu, area: %8p\n", data, pdbi->mmap_buflen_frames, pdbi->mmap_areas);

    do {
        enum win_wm_message msg;
        DWORD param;
        HANDLE hEvent;
        int err;
        snd_pcm_format_t format;

        if (state != WINE_WS_PLAYING)
        {
            ALSA_WaitRingMessage(&pdbi->mmap_ring, 1000000);
            ALSA_RetrieveRingMessage(&pdbi->mmap_ring, &msg, &param, &hEvent);
        }
        else
        while (!ALSA_RetrieveRingMessage(&pdbi->mmap_ring, &msg, &param, &hEvent))
        {
            snd_pcm_wait(wwo->pcm, -1);
            DSDB_CheckXRUN(pdbi);

            wanted = frames = pdbi->mmap_buflen_frames;

            err = snd_pcm_mmap_begin(wwo->pcm, &areas, &ofs, &frames);
            snd_pcm_mmap_commit(wwo->pcm, ofs, frames);

            /* mark our current play position */
            pdbi->mmap_ppos = ofs;
            TRACE_(waveloop)("Updated position to %lx [%d, 0x%8p, %lu]\n", ofs, err, areas, frames);
            /* Check to make sure we committed all we want to commit. ALSA
             * only gives a contiguous linear region, so we need to check this
             * in case we've reached the end of the buffer, in which case we
             * can wrap around back to the beginning. */
            if (frames < wanted) {
                frames = wanted - frames;
                snd_pcm_mmap_begin(wwo->pcm, &areas, &ofs, &frames);
                snd_pcm_mmap_commit(wwo->pcm, ofs, frames);
            }
            wanted = 0;
            snd_pcm_mmap_begin(wwo->pcm, &areas, &ofs, &wanted);
            snd_pcm_mmap_commit(wwo->pcm, ofs, wanted);
            pdbi->mmap_wpos = ofs;
        }

        switch (msg) {
        case WINE_WM_STARTING:
            if (state == WINE_WS_PLAYING)
                break;

            alsastate = snd_pcm_state(wwo->pcm);
            if ( alsastate == SND_PCM_STATE_SETUP )
            {
                err = snd_pcm_prepare(wwo->pcm);
                alsastate = snd_pcm_state(wwo->pcm);
            }

            snd_pcm_avail_update(wwo->pcm);

            /* Rewind, and initialise */
            frames = 0;
            snd_pcm_mmap_begin(wwo->pcm, &areas, &ofs, &frames);
            snd_pcm_mmap_commit(wwo->pcm, ofs, frames);
            snd_pcm_rewind(wwo->pcm, ofs);
            snd_pcm_hw_params_get_format(wwo->hw_params, &format);
            snd_pcm_format_set_silence(format, pdbi->mmap_buffer, pdbi->mmap_buflen_frames);
            pdbi->mmap_ppos = 0;
            snd_pcm_hw_params_get_period_size(wwo->hw_params, &wanted, NULL);
            pdbi->mmap_wpos = 2*wanted;
            if ( alsastate == SND_PCM_STATE_PREPARED )
            {
                err = snd_pcm_start(wwo->pcm);
                TRACE("Starting 0x%8p err: %d\n", wwo->pcm, err);
            }
            state = WINE_WS_PLAYING;
            break;

        case WINE_WM_STOPPING:
            state = WINE_WS_STOPPED;
            snd_pcm_drop(wwo->pcm);
            break;

        case WINE_WM_CLOSING:
            if (wwo->pcm != NULL)
                snd_pcm_drop(wwo->pcm);
            pdbi->mmap_thread = NULL;
            SetEvent(hEvent);
            goto out;
        default:
            ERR("Unhandled event %s\n", ALSA_getCmdString(msg));
            break;
        }
        if (hEvent != INVALID_HANDLE_VALUE)
            SetEvent(hEvent);
    } while (1);
out:
    TRACE_(waveloop)("Destroyed MMAP thread\n");
    TRACE("Destroyed MMAP thread\n");
    return 0;
}

/**
 * Allocate the memory-mapped buffer for direct sound, and set up the
 * callback.
 */
static int DSDB_CreateMMAP(IDsDriverBufferImpl* pdbi)
{
    WINE_WAVEDEV *     wwo = &(WOutDev[pdbi->drv->wDevID]);
    snd_pcm_format_t   format;
    snd_pcm_uframes_t  frames, ofs, avail, psize;
    unsigned int       channels, bits_per_sample, bits_per_frame;
    int                err, mmap_mode;

    mmap_mode = snd_pcm_type(wwo->pcm);

    ALSA_InitRingMessage(&pdbi->mmap_ring);

    if (mmap_mode == SND_PCM_TYPE_HW) {
        TRACE("mmap'd buffer is a hardware buffer.\n");
    }
    else {
#if 0 /* Horribly hack, shouldn't be used */
        err = snd_pcm_hw_params_get_period_size(wwo->hw_params, &psize, NULL);
        /* Set only a buffer of 2 period sizes, to decrease latency */
        if (err >= 0)
            err = snd_pcm_hw_params_set_buffer_size_near(wwo->pcm, wwo->hw_params, &psize);

        psize *= 2;
        if (err < 0) {
            ERR("Errno %d (%s) occurred when setting buffer size\n", err, strerror(errno));
        }
#endif
        TRACE("mmap'd buffer is an ALSA emulation of hardware buffer.\n");
    }

    err = snd_pcm_hw_params_get_period_size(wwo->hw_params, &psize, NULL);
    err = snd_pcm_hw_params_get_format(wwo->hw_params, &format);
    err = snd_pcm_hw_params_get_buffer_size(wwo->hw_params, &frames);
    err = snd_pcm_hw_params_get_channels(wwo->hw_params, &channels);
    bits_per_sample = snd_pcm_format_physical_width(format);
    bits_per_frame = bits_per_sample * channels;

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
    pdbi->mmap_thread = NULL;

    TRACE("created mmap buffer of %ld frames (%d bytes) at %p\n",
        frames, pdbi->mmap_buflen_bytes, pdbi->mmap_buffer);

    return DS_OK;
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

    if (This->mmap_thread != NULL)
        ALSA_AddRingMessage(&This->mmap_ring, WINE_WM_CLOSING, 0, 1);

    TRACE("mmap buffer %p destroyed\n", This->mmap_buffer);
    ALSA_DestroyRingMessage(&This->mmap_ring);

    if (This == This->drv->primary)
	This->drv->primary = NULL;
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
    FIXME("(%p,%d): stub\n",iface,dwNewPos);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_GetPosition(PIDSDRIVERBUFFER iface,
						      LPDWORD lpdwPlay, LPDWORD lpdwWrite)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    WINE_WAVEDEV *      wwo = &(WOutDev[This->drv->wDevID]);
    snd_pcm_uframes_t   hw_pptr, hw_wptr;
    snd_pcm_state_t     state;

    if (wwo->hw_params == NULL || wwo->pcm == NULL) return DSERR_GENERIC;

    state = snd_pcm_state(wwo->pcm);
    if (state == SND_PCM_STATE_RUNNING)
    {
        hw_pptr = This->mmap_ppos;
        hw_wptr = This->mmap_wpos;
    }
    else
        hw_pptr = hw_wptr = 0;

    if (lpdwPlay)
	*lpdwPlay = snd_pcm_frames_to_bytes(wwo->pcm, hw_pptr) % This->mmap_buflen_bytes;
    if (lpdwWrite)
	*lpdwWrite = snd_pcm_frames_to_bytes(wwo->pcm, hw_wptr) % This->mmap_buflen_bytes;

    TRACE_(waveloop)("hw_pptr=0x%08x, hw_wptr=0x%08x playpos=%d, writepos=%d\n", (unsigned int)hw_pptr, (unsigned int)hw_wptr, lpdwPlay?*lpdwPlay:-1, lpdwWrite?*lpdwWrite:-1);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;

    TRACE("(%p,%x,%x,%x)\n",iface,dwRes1,dwRes2,dwFlags);

    if (This->mmap_thread == NULL)
        This->mmap_thread = CreateThread(NULL, 0, DBSB_MMAPLoop, (LPVOID)(DWORD)This, 0, NULL);

    if (This->mmap_thread == NULL)
        ERR("Cannot create sound thread, don't expect any sound at all\n");
    else
        ALSA_AddRingMessage(&This->mmap_ring, WINE_WM_STARTING, 0, 1);

    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Stop(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;

    TRACE("(%p)\n",iface);

    if (This->mmap_thread != NULL)
        ALSA_AddRingMessage(&This->mmap_ring, WINE_WM_STOPPING, 0, 1);

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
