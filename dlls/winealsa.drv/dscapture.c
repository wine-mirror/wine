/*
 * Sample Wine Driver for Advanced Linux Sound System (ALSA)
 *      Based on version <final> of the ALSA API
 *
 * Copyright 2007 - Maarten Lankhorst
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
 *              Low level dsound input implementation			*
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
#include "mmddk.h"

#include "alsa.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#ifdef HAVE_ALSA

/* Notify timer checks every 10 ms with a resolution of 2 ms */
#define DS_TIME_DEL 10
#define DS_TIME_RES 2

WINE_DEFAULT_DEBUG_CHANNEL(dsalsa);

typedef struct IDsCaptureDriverBufferImpl IDsCaptureDriverBufferImpl;

typedef struct IDsCaptureDriverImpl
{
    const IDsCaptureDriverVtbl *lpVtbl;
    LONG ref;
    IDsCaptureDriverBufferImpl* capture_buffer;
    UINT wDevID;
} IDsCaptureDriverImpl;

typedef struct IDsCaptureDriverNotifyImpl
{
    const IDsDriverNotifyVtbl *lpVtbl;
    LONG ref;
    IDsCaptureDriverBufferImpl *buffer;
    DSBPOSITIONNOTIFY *notifies;
    DWORD nrofnotifies, playpos;
    UINT timerID;
} IDsCaptureDriverNotifyImpl;

struct IDsCaptureDriverBufferImpl
{
    const IDsCaptureDriverBufferVtbl *lpVtbl;
    LONG ref;
    IDsCaptureDriverImpl *drv;
    IDsCaptureDriverNotifyImpl *notify;

    CRITICAL_SECTION pcm_crst;
    LPBYTE mmap_buffer, presented_buffer;
    DWORD mmap_buflen_bytes, play_looping, mmap_ofs_bytes;

    /* Note: snd_pcm_frames_to_bytes(This->pcm, mmap_buflen_frames) != mmap_buflen_bytes */
    /* The actual buffer may differ in size from the wanted buffer size */

    snd_pcm_t *pcm;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_uframes_t mmap_buflen_frames, mmap_pos;
};

static void Capture_CheckNotify(IDsCaptureDriverNotifyImpl *This, DWORD from, DWORD len)
{
    unsigned i;
    for (i = 0; i < This->nrofnotifies; ++i) {
        LPDSBPOSITIONNOTIFY event = This->notifies + i;
        DWORD offset = event->dwOffset;
        TRACE("checking %d, position %d, event = %p\n", i, offset, event->hEventNotify);

        if (offset == DSBPN_OFFSETSTOP) {
            if (!from && !len) {
                SetEvent(event->hEventNotify);
                TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
                return;
            }
            else return;
        }

        if (offset >= from && offset < (from + len))
        {
            TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
            SetEvent(event->hEventNotify);
        }
    }
}

static void CALLBACK Capture_Notify(UINT timerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)dwUser;
    DWORD last_playpos, playpos;
    PIDSCDRIVERBUFFER iface = (PIDSCDRIVERBUFFER)This;

    /* **** */
    EnterCriticalSection(&This->pcm_crst);

    IDsDriverBuffer_GetPosition(iface, &playpos, NULL);
    last_playpos = This->notify->playpos;
    This->notify->playpos = playpos;

    if (snd_pcm_state(This->pcm) != SND_PCM_STATE_RUNNING || last_playpos == playpos || !This->notify->nrofnotifies || !This->notify->notifies)
        goto done;

    if (playpos < last_playpos)
    {
        Capture_CheckNotify(This->notify, last_playpos, This->mmap_buflen_bytes);
        if (playpos)
            Capture_CheckNotify(This->notify, 0, playpos);
    }
    else Capture_CheckNotify(This->notify, last_playpos, playpos - last_playpos);

done:
    LeaveCriticalSection(&This->pcm_crst);
    /* **** */
}

static HRESULT WINAPI IDsCaptureDriverNotifyImpl_QueryInterface(PIDSDRIVERNOTIFY iface, REFIID riid, LPVOID *ppobj)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsDriverNotify) ) {
        IDsDriverNotify_AddRef(iface);
        *ppobj = This;
        return DS_OK;
    }

    FIXME( "Unknown IID %s\n", debugstr_guid(riid));

    *ppobj = 0;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDsCaptureDriverNotifyImpl_AddRef(PIDSDRIVERNOTIFY iface)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverNotifyImpl_Release(PIDSDRIVERNOTIFY iface)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        This->buffer->notify = NULL;
        if (This->timerID)
        {
            timeKillEvent(This->timerID);
            timeEndPeriod(DS_TIME_RES);
        }
        HeapFree(GetProcessHeap(), 0, This->notifies);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return refCount;
}

static HRESULT WINAPI IDsCaptureDriverNotifyImpl_SetNotificationPositions(PIDSDRIVERNOTIFY iface, DWORD howmuch, LPCDSBPOSITIONNOTIFY notify)
{
    DWORD len = howmuch * sizeof(DSBPOSITIONNOTIFY);
    unsigned i;
    LPVOID notifies;
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    TRACE("(%p,0x%08x,%p)\n",This,howmuch,notify);

    if (!notify) {
        WARN("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(dsalsa))
        for (i=0;i<howmuch; ++i)
            TRACE("notify at %d to %p\n", notify[i].dwOffset, notify[i].hEventNotify);

    /* **** */
    EnterCriticalSection(&This->buffer->pcm_crst);

    /* Make an internal copy of the caller-supplied array.
     * Replace the existing copy if one is already present. */
    if (This->notifies)
        notifies = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->notifies, len);
    else
        notifies = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);

    if (!notifies)
    {
        LeaveCriticalSection(&This->buffer->pcm_crst);
        /* **** */
        return DSERR_OUTOFMEMORY;
    }
    This->notifies = notifies;
    memcpy(This->notifies, notify, len);
    This->nrofnotifies = howmuch;
    IDsDriverBuffer_GetPosition((PIDSCDRIVERBUFFER)This->buffer, &This->playpos, NULL);

    if (!This->timerID)
    {
        timeBeginPeriod(DS_TIME_RES);
        This->timerID = timeSetEvent(DS_TIME_DEL, DS_TIME_RES, Capture_Notify, (DWORD_PTR)This->buffer, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    }

    LeaveCriticalSection(&This->buffer->pcm_crst);
    /* **** */

    return S_OK;
}

static const IDsDriverNotifyVtbl dscdnvt =
{
    IDsCaptureDriverNotifyImpl_QueryInterface,
    IDsCaptureDriverNotifyImpl_AddRef,
    IDsCaptureDriverNotifyImpl_Release,
    IDsCaptureDriverNotifyImpl_SetNotificationPositions,
};

#if 0
/** Convert the position an application sees into a position ALSA sees */
static snd_pcm_uframes_t fakepos_to_realpos(const IDsCaptureDriverBufferImpl* This, DWORD fakepos)
{
    snd_pcm_uframes_t realpos;
    if (fakepos < This->mmap_ofs_bytes)
        realpos = This->mmap_buflen_bytes + fakepos - This->mmap_ofs_bytes;
    else realpos = fakepos - This->mmap_ofs_bytes;
    return snd_pcm_bytes_to_frames(This->pcm, realpos) % This->mmap_buflen_frames;
}
#endif

/** Convert the position ALSA sees into a position an application sees */
static DWORD realpos_to_fakepos(const IDsCaptureDriverBufferImpl* This, snd_pcm_uframes_t realpos)
{
    DWORD realposb = snd_pcm_frames_to_bytes(This->pcm, realpos);
    return (realposb + This->mmap_ofs_bytes) % This->mmap_buflen_bytes;
}

/** Raw copy data, with buffer wrap around */
static void CopyDataWrap(const IDsCaptureDriverBufferImpl* This, LPBYTE dest, DWORD fromwhere, DWORD copylen, DWORD buflen)
{
    DWORD remainder = buflen - fromwhere;
    if (remainder >= copylen)
    {
        CopyMemory(dest, This->mmap_buffer + fromwhere, copylen);
    }
    else
    {
        CopyMemory(dest, This->mmap_buffer + fromwhere, remainder);
        copylen -= remainder;
        CopyMemory(dest, This->mmap_buffer, copylen);
    }
}

/** Copy data from the mmap buffer to backbuffer, taking into account all wraparounds that may occur */
static void CopyData(const IDsCaptureDriverBufferImpl* This, snd_pcm_uframes_t fromwhere, snd_pcm_uframes_t len)
{
    DWORD dlen = snd_pcm_frames_to_bytes(This->pcm, len) % This->mmap_buflen_bytes;

    /* Backbuffer */
    DWORD ofs = realpos_to_fakepos(This, fromwhere);
    DWORD remainder = This->mmap_buflen_bytes - ofs;

    /* MMAP buffer */
    DWORD realbuflen = snd_pcm_frames_to_bytes(This->pcm, This->mmap_buflen_frames);
    DWORD realofs = snd_pcm_frames_to_bytes(This->pcm, fromwhere);

    if (remainder >= dlen)
    {
       CopyDataWrap(This, This->presented_buffer + ofs, realofs, dlen, realbuflen);
    }
    else
    {
       CopyDataWrap(This, This->presented_buffer + ofs, realofs, remainder, realbuflen);
       dlen -= remainder;
       CopyDataWrap(This, This->presented_buffer, (realofs+remainder)%realbuflen, dlen, realbuflen);
    }
}

/** Fill buffers, for starting and stopping
 * Alsa won't start playing until everything is filled up
 * This also updates mmap_pos
 *
 * Returns: Amount of periods in use so snd_pcm_avail_update
 * doesn't have to be called up to 4x in GetPosition()
 */
static snd_pcm_uframes_t CommitAll(IDsCaptureDriverBufferImpl *This, DWORD forced)
{
    const snd_pcm_channel_area_t *areas;
    snd_pcm_uframes_t used;
    const snd_pcm_uframes_t commitahead = This->mmap_buflen_frames;

    used = This->mmap_buflen_frames - snd_pcm_avail_update(This->pcm);
    TRACE("%p needs to commit to %lu, used: %lu\n", This, commitahead, used);
    if (used < commitahead && (forced || This->play_looping))
    {
        snd_pcm_uframes_t done, putin = commitahead - used;
        snd_pcm_mmap_begin(This->pcm, &areas, &This->mmap_pos, &putin);
        CopyData(This, This->mmap_pos, putin);
        done = snd_pcm_mmap_commit(This->pcm, This->mmap_pos, putin);
        This->mmap_pos += done;
        used += done;
        putin = commitahead - used;

        if (This->mmap_pos == This->mmap_buflen_frames && (snd_pcm_sframes_t)putin > 0 && This->play_looping)
        {
            snd_pcm_mmap_begin(This->pcm, &areas, &This->mmap_pos, &putin);
            This->mmap_ofs_bytes += snd_pcm_frames_to_bytes(This->pcm, This->mmap_buflen_frames);
            This->mmap_ofs_bytes %= This->mmap_buflen_bytes;
            CopyData(This, This->mmap_pos, putin);
            done = snd_pcm_mmap_commit(This->pcm, This->mmap_pos, putin);
            This->mmap_pos += done;
            used += done;
        }
    }

    if (This->mmap_pos == This->mmap_buflen_frames)
    {
        This->mmap_ofs_bytes += snd_pcm_frames_to_bytes(This->pcm, This->mmap_buflen_frames);
        This->mmap_ofs_bytes %= This->mmap_buflen_bytes;
        This->mmap_pos = 0;
    }

    return used;
}

static void CheckXRUN(IDsCaptureDriverBufferImpl* This)
{
    snd_pcm_state_t state = snd_pcm_state(This->pcm);
    snd_pcm_sframes_t delay;
    int err;

    snd_pcm_hwsync(This->pcm);
    snd_pcm_delay(This->pcm, &delay);
    if ( state == SND_PCM_STATE_XRUN )
    {
        err = snd_pcm_prepare(This->pcm);
        CommitAll(This, FALSE);
        snd_pcm_start(This->pcm);
        WARN("xrun occurred\n");
        if ( err < 0 )
            ERR("recovery from xrun failed, prepare failed: %s\n", snd_strerror(err));
    }
    else if ( state == SND_PCM_STATE_SUSPENDED )
    {
        int err = snd_pcm_resume(This->pcm);
        TRACE("recovery from suspension occurred\n");
        if (err < 0 && err != -EAGAIN){
            err = snd_pcm_prepare(This->pcm);
            if (err < 0)
                ERR("recovery from suspend failed, prepare failed: %s\n", snd_strerror(err));
        }
    }
    else if ( state != SND_PCM_STATE_RUNNING)
    {
        WARN("Unhandled state: %d\n", state);
    }
}

/**
 * Allocate the memory-mapped buffer for direct sound, and set up the
 * callback.
 */
static int CreateMMAP(IDsCaptureDriverBufferImpl* pdbi)
{
    snd_pcm_t *pcm = pdbi->pcm;
    snd_pcm_format_t format;
    snd_pcm_uframes_t frames, ofs, avail, psize, boundary;
    unsigned int channels, bits_per_sample, bits_per_frame;
    int err, mmap_mode;
    const snd_pcm_channel_area_t *areas;
    snd_pcm_hw_params_t *hw_params = pdbi->hw_params;
    snd_pcm_sw_params_t *sw_params = pdbi->sw_params;

    mmap_mode = snd_pcm_type(pcm);

    if (mmap_mode == SND_PCM_TYPE_HW)
        TRACE("mmap'd buffer is a direct hardware buffer.\n");
    else if (mmap_mode == SND_PCM_TYPE_DMIX)
        TRACE("mmap'd buffer is an ALSA dmix buffer\n");
    else
        TRACE("mmap'd buffer is an ALSA type %d buffer\n", mmap_mode);

    err = snd_pcm_hw_params_get_period_size(hw_params, &psize, NULL);
    err = snd_pcm_hw_params_get_format(hw_params, &format);
    err = snd_pcm_hw_params_get_buffer_size(hw_params, &frames);
    err = snd_pcm_hw_params_get_channels(hw_params, &channels);
    bits_per_sample = snd_pcm_format_physical_width(format);
    bits_per_frame = bits_per_sample * channels;

    if (TRACE_ON(dsalsa))
        ALSA_TraceParameters(hw_params, NULL, FALSE);

    TRACE("format=%s  frames=%ld  channels=%d  bits_per_sample=%d  bits_per_frame=%d\n",
          snd_pcm_format_name(format), frames, channels, bits_per_sample, bits_per_frame);

    pdbi->mmap_buflen_frames = frames;
    snd_pcm_sw_params_current(pcm, sw_params);
    snd_pcm_sw_params_set_start_threshold(pcm, sw_params, 0);
    snd_pcm_sw_params_get_boundary(sw_params, &boundary);
    snd_pcm_sw_params_set_stop_threshold(pcm, sw_params, boundary);
    snd_pcm_sw_params_set_silence_threshold(pcm, sw_params, INT_MAX);
    snd_pcm_sw_params_set_silence_size(pcm, sw_params, 0);
    snd_pcm_sw_params_set_avail_min(pcm, sw_params, 0);
    snd_pcm_sw_params_set_xrun_mode(pcm, sw_params, SND_PCM_XRUN_NONE);
    err = snd_pcm_sw_params(pcm, sw_params);

    avail = snd_pcm_avail_update(pcm);
    if (avail < 0)
    {
        ERR("No buffer is available: %s.\n", snd_strerror(avail));
        return DSERR_GENERIC;
    }
    err = snd_pcm_mmap_begin(pcm, &areas, &ofs, &avail);
    if ( err < 0 )
    {
        ERR("Can't map sound device for direct access: %s\n", snd_strerror(err));
        return DSERR_GENERIC;
    }
    err = snd_pcm_mmap_commit(pcm, ofs, 0);
    pdbi->mmap_buffer = areas->addr;

    TRACE("created mmap buffer of %ld frames (%d bytes) at %p\n",
        frames, pdbi->mmap_buflen_bytes, pdbi->mmap_buffer);

    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_QueryInterface(PIDSCDRIVERBUFFER iface, REFIID riid, LPVOID *ppobj)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsCaptureDriverBuffer) ) {
        IDsCaptureDriverBuffer_AddRef(iface);
        *ppobj = (LPVOID)iface;
        return DS_OK;
    }

    if ( IsEqualGUID( &IID_IDsDriverNotify, riid ) ) {
        if (!This->notify)
        {
            This->notify = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDsCaptureDriverNotifyImpl));
            if (!This->notify)
                return DSERR_OUTOFMEMORY;
            This->notify->lpVtbl = &dscdnvt;
            This->notify->buffer = This;

            /* Keep a lock on IDsDriverNotify for ourself, so it is destroyed when the buffer is */
            IDsDriverNotify_AddRef((PIDSDRIVERNOTIFY)This->notify);
        }
        IDsDriverNotify_AddRef((PIDSDRIVERNOTIFY)This->notify);
        *ppobj = (LPVOID)This->notify;
        return DS_OK;
    }

    if ( IsEqualGUID( &IID_IDsDriverPropertySet, riid ) ) {
        FIXME("Unsupported interface IID_IDsDriverPropertySet\n");
        return E_FAIL;
    }

    FIXME("(): Unknown interface %s\n", debugstr_guid(riid));
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsCaptureDriverBufferImpl_AddRef(PIDSCDRIVERBUFFER iface)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverBufferImpl_Release(PIDSCDRIVERBUFFER iface)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

    if (refCount)
        return refCount;

    EnterCriticalSection(&This->pcm_crst);
    if (This->notify)
        IDsDriverNotify_Release((PIDSDRIVERNOTIFY)This->notify);
    TRACE("mmap buffer %p destroyed\n", This->mmap_buffer);

    This->drv->capture_buffer = NULL;
    This->pcm_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->pcm_crst);

    snd_pcm_drop(This->pcm);
    snd_pcm_close(This->pcm);
    This->pcm = NULL;
    HeapFree(GetProcessHeap(), 0, This->presented_buffer);
    HeapFree(GetProcessHeap(), 0, This->sw_params);
    HeapFree(GetProcessHeap(), 0, This->hw_params);
    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Lock(PIDSCDRIVERBUFFER iface, LPVOID*ppvAudio1,LPDWORD pdwLen1,LPVOID*ppvAudio2,LPDWORD pdwLen2, DWORD dwWritePosition,DWORD dwWriteLen, DWORD dwFlags)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    TRACE("(%p,%p,%p,%p,%p,%d,%d,0x%08x)\n", This, ppvAudio1, pdwLen1, ppvAudio2, pdwLen2, dwWritePosition, dwWriteLen, dwFlags);

    if (ppvAudio1)
        *ppvAudio1 = (LPBYTE)This->presented_buffer + dwWritePosition;

    if (dwWritePosition + dwWriteLen < This->mmap_buflen_bytes) {
        if (pdwLen1)
            *pdwLen1 = dwWriteLen;
        if (ppvAudio2)
            *ppvAudio2 = 0;
        if (pdwLen2)
            *pdwLen2 = 0;
    } else {
        if (pdwLen1)
            *pdwLen1 = This->mmap_buflen_bytes - dwWritePosition;
        if (ppvAudio2)
            *ppvAudio2 = This->presented_buffer;
        if (pdwLen2)
            *pdwLen2 = dwWriteLen - (This->mmap_buflen_bytes - dwWritePosition);
    }
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Unlock(PIDSCDRIVERBUFFER iface, LPVOID pvAudio1,DWORD dwLen1, LPVOID pvAudio2,DWORD dwLen2)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    TRACE("(%p,%p,%d,%p,%d)\n",This,pvAudio1,dwLen1,pvAudio2,dwLen2);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_SetFormat(PIDSCDRIVERBUFFER iface, LPWAVEFORMATEX pwfx)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    WINE_WAVEDEV *wwi = &WInDev[This->drv->wDevID];
    snd_pcm_t *pcm = NULL;
    snd_pcm_hw_params_t *hw_params = This->hw_params;
    snd_pcm_format_t format = -1;
    snd_pcm_uframes_t buffer_size;
    DWORD rate = pwfx->nSamplesPerSec;
    int err=0;

    TRACE("(%p, %p)\n", iface, pwfx);

    switch (pwfx->wBitsPerSample)
    {
        case  8: format = SND_PCM_FORMAT_U8; break;
        case 16: format = SND_PCM_FORMAT_S16_LE; break;
        case 24: format = SND_PCM_FORMAT_S24_LE; break;
        case 32: format = SND_PCM_FORMAT_S32_LE; break;
        default: FIXME("Unsupported bpp: %d\n", pwfx->wBitsPerSample); return DSERR_GENERIC;
    }

    /* **** */
    EnterCriticalSection(&This->pcm_crst);

    err = snd_pcm_open(&pcm, wwi->pcmname, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);

    if (err < 0)
    {
        if (errno != EBUSY || !This->pcm)
        {
            /* **** */
            LeaveCriticalSection(&This->pcm_crst);
            WARN("Cannot open sound device: %s\n", snd_strerror(err));
            return DSERR_GENERIC;
        }
        snd_pcm_drop(This->pcm);
        snd_pcm_close(This->pcm);
        This->pcm = NULL;
        err = snd_pcm_open(&pcm, wwi->pcmname, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
        if (err < 0)
        {
            /* **** */
            LeaveCriticalSection(&This->pcm_crst);
            WARN("Cannot open sound device: %s\n", snd_strerror(err));
            return DSERR_BUFFERLOST;
        }
    }

    /* Set some defaults */
    snd_pcm_hw_params_any(pcm, hw_params);
    err = snd_pcm_hw_params_set_channels(pcm, hw_params, pwfx->nChannels);
    if (err < 0) { WARN("Could not set channels to %d\n", pwfx->nChannels); goto err; }

    err = snd_pcm_hw_params_set_format(pcm, hw_params, format);
    if (err < 0) { WARN("Could not set format to %d bpp\n", pwfx->wBitsPerSample); goto err; }

    err = snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, NULL);
    if (err < 0) { rate = pwfx->nSamplesPerSec; WARN("Could not set rate\n"); goto err; }

    if (!ALSA_NearMatch(rate, pwfx->nSamplesPerSec))
    {
        WARN("Could not set sound rate to %d, but instead to %d\n", pwfx->nSamplesPerSec, rate);
        pwfx->nSamplesPerSec = rate;
        pwfx->nAvgBytesPerSec = rate * pwfx->nBlockAlign;
        /* Let DirectSound detect this */
    }

    snd_pcm_hw_params_set_periods_integer(pcm, hw_params);
    buffer_size = This->mmap_buflen_bytes / pwfx->nBlockAlign;
    snd_pcm_hw_params_set_buffer_size_near(pcm, hw_params, &buffer_size);
    buffer_size = 5000;
    snd_pcm_hw_params_set_period_time_near(pcm, hw_params, (unsigned int*)&buffer_size, NULL);
    err = snd_pcm_hw_params(pcm, hw_params);
    err = snd_pcm_sw_params(pcm, This->sw_params);
    snd_pcm_prepare(pcm);

    if (This->pcm)
    {
        snd_pcm_drop(This->pcm);
        snd_pcm_close(This->pcm);
    }
    This->pcm = pcm;

    snd_pcm_prepare(This->pcm);
    CreateMMAP(This);

    /* **** */
    LeaveCriticalSection(&This->pcm_crst);
    return S_OK;

    err:
    if (err < 0)
        WARN("Failed to apply changes: %s\n", snd_strerror(err));

    if (!This->pcm)
        This->pcm = pcm;
    else
        snd_pcm_close(pcm);

    if (This->pcm)
        snd_pcm_hw_params_current(This->pcm, This->hw_params);

    /* **** */
    LeaveCriticalSection(&This->pcm_crst);
    return DSERR_BADFORMAT;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_GetPosition(PIDSCDRIVERBUFFER iface, LPDWORD lpdwCappos, LPDWORD lpdwReadpos)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    snd_pcm_uframes_t hw_pptr, hw_wptr;

    EnterCriticalSection(&This->pcm_crst);

    if (!This->pcm)
    {
        FIXME("Bad pointer for pcm: %p\n", This->pcm);
        LeaveCriticalSection(&This->pcm_crst);
        return DSERR_GENERIC;
    }

    if (snd_pcm_state(This->pcm) != SND_PCM_STATE_RUNNING)
    {
        hw_pptr = This->mmap_pos;
        CheckXRUN(This);
    }
    else
    {
        /* FIXME: Unused at the moment */
        snd_pcm_uframes_t used = CommitAll(This, FALSE);

        if (This->mmap_pos > used)
            hw_pptr = This->mmap_pos - used;
        else
            hw_pptr = This->mmap_buflen_frames - used + This->mmap_pos;
    }
    hw_wptr = This->mmap_pos;

    if (lpdwCappos)
        *lpdwCappos = realpos_to_fakepos(This, hw_pptr);
    if (lpdwReadpos)
        *lpdwReadpos = realpos_to_fakepos(This, hw_wptr);

    LeaveCriticalSection(&This->pcm_crst);

    TRACE("hw_pptr=0x%08x, hw_wptr=0x%08x playpos=%d, writepos=%d\n", (unsigned int)hw_pptr, (unsigned int)hw_wptr, lpdwCappos?*lpdwCappos:-1, lpdwReadpos?*lpdwReadpos:-1);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_GetStatus(PIDSCDRIVERBUFFER iface, LPDWORD lpdwStatus)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    snd_pcm_state_t state;
    TRACE("(%p,%p)\n",iface,lpdwStatus);

    state = snd_pcm_state(This->pcm);
    switch (state)
    {
    case SND_PCM_STATE_XRUN:
    case SND_PCM_STATE_SUSPENDED:
    case SND_PCM_STATE_RUNNING:
        *lpdwStatus = DSCBSTATUS_CAPTURING | (This->play_looping ? DSCBSTATUS_LOOPING : 0);
        break;
    default:
        *lpdwStatus = 0;
        break;
    }

    TRACE("State: %d, flags: 0x%08x\n", state, *lpdwStatus);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Start(PIDSCDRIVERBUFFER iface, DWORD dwFlags)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    TRACE("(%p,%x)\n",iface,dwFlags);

    /* **** */
    EnterCriticalSection(&This->pcm_crst);
    snd_pcm_start(This->pcm);
    This->play_looping = !!(dwFlags & DSCBSTART_LOOPING);
    if (!This->play_looping)
        /* Not well supported because of the difference in ALSA size and DSOUND's notion of size
         * what it does right now is fill the buffer once.. ALSA size */
        FIXME("Non-looping buffers are not properly supported!\n");
    CommitAll(This, TRUE);

    if (This->notify && This->notify->nrofnotifies && This->notify->notifies)
    {
        DWORD playpos = realpos_to_fakepos(This, This->mmap_pos);
        if (playpos)
            Capture_CheckNotify(This->notify, 0, playpos);
        This->notify->playpos = playpos;
    }

    /* **** */
    LeaveCriticalSection(&This->pcm_crst);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Stop(PIDSCDRIVERBUFFER iface)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    TRACE("(%p)\n",iface);

    /* **** */
    EnterCriticalSection(&This->pcm_crst);
    This->play_looping = FALSE;
    snd_pcm_drop(This->pcm);
    snd_pcm_prepare(This->pcm);

    if (This->notify && This->notify->notifies && This->notify->nrofnotifies)
        Capture_CheckNotify(This->notify, 0, 0);

    /* **** */
    LeaveCriticalSection(&This->pcm_crst);
    return DS_OK;
}

static const IDsCaptureDriverBufferVtbl dsdbvt =
{
    IDsCaptureDriverBufferImpl_QueryInterface,
    IDsCaptureDriverBufferImpl_AddRef,
    IDsCaptureDriverBufferImpl_Release,
    IDsCaptureDriverBufferImpl_Lock,
    IDsCaptureDriverBufferImpl_Unlock,
    IDsCaptureDriverBufferImpl_SetFormat,
    IDsCaptureDriverBufferImpl_GetPosition,
    IDsCaptureDriverBufferImpl_GetStatus,
    IDsCaptureDriverBufferImpl_Start,
    IDsCaptureDriverBufferImpl_Stop
};

static HRESULT WINAPI IDsCaptureDriverImpl_QueryInterface(PIDSCDRIVER iface, REFIID riid, LPVOID *ppobj)
{
    /* IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface; */
    FIXME("(%p): stub!\n",iface);
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsCaptureDriverImpl_AddRef(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverImpl_Release(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

    if (refCount)
        return refCount;

    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI IDsCaptureDriverImpl_GetDriverDesc(PIDSCDRIVER iface, PDSDRIVERDESC pDesc)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pDesc);
    memcpy(pDesc, &(WInDev[This->wDevID].ds_desc), sizeof(DSDRIVERDESC));
    pDesc->dwFlags		= 0;
    pDesc->dnDevNode		= WInDev[This->wDevID].waveDesc.dnDevNode;
    pDesc->wVxdId		= 0;
    pDesc->wReserved		= 0;
    pDesc->ulDeviceNum		= This->wDevID;
    pDesc->dwHeapType		= DSDHEAP_NOHEAP;
    pDesc->pvDirectDrawHeap	= NULL;
    pDesc->dwMemStartAddress	= 0xDEAD0000;
    pDesc->dwMemEndAddress	= 0xDEAF0000;
    pDesc->dwMemAllocExtra	= 0;
    pDesc->pvReserved1		= NULL;
    pDesc->pvReserved2		= NULL;
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_Open(PIDSCDRIVER iface)
{
    HRESULT hr = S_OK;
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    int err=0;
    snd_pcm_t *pcm = NULL;
    snd_pcm_hw_params_t *hw_params;

    /* While this is not really needed, it is a good idea to do this,
     * to see if sound can be initialized */

    hw_params = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_hw_params_sizeof());
    if (!hw_params)
    {
        hr = DSERR_OUTOFMEMORY;
        WARN("--> %08x\n", hr);
        return hr;
    }

    err = snd_pcm_open(&pcm, WOutDev[This->wDevID].pcmname, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (err < 0) goto err;
    err = snd_pcm_hw_params_any(pcm, hw_params);
    if (err < 0) goto err;
    err = snd_pcm_hw_params_set_access (pcm, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    if (err < 0) goto err;

    TRACE("Success\n");
    snd_pcm_close(pcm);
    HeapFree(GetProcessHeap(), 0, hw_params);
    return hr;

    err:
    hr = DSERR_GENERIC;
    WARN("Failed to open device: %s\n", snd_strerror(err));
    if (pcm)
        snd_pcm_close(pcm);
    HeapFree(GetProcessHeap(), 0, hw_params);
    WARN("--> %08x\n", hr);
    return hr;
}

static HRESULT WINAPI IDsCaptureDriverImpl_Close(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p) stub, harmless\n",This);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_GetCaps(PIDSCDRIVER iface, PDSCDRIVERCAPS pCaps)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    WINE_WAVEDEV *wwi = &WInDev[This->wDevID];
    TRACE("(%p,%p)\n",iface,pCaps);
    pCaps->dwSize = sizeof(DSCDRIVERCAPS);
    pCaps->dwFlags = wwi->ds_caps.dwFlags;
    pCaps->dwFormats = wwi->incaps.dwFormats;
    pCaps->dwChannels = wwi->incaps.wChannels;
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_CreateCaptureBuffer(PIDSCDRIVER iface,
						      LPWAVEFORMATEX pwfx,
						      DWORD dwFlags, DWORD dwCardAddress,
						      LPDWORD pdwcbBufferSize,
						      LPBYTE *ppbBuffer,
						      LPVOID *ppvObj)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    IDsCaptureDriverBufferImpl** ippdsdb = (IDsCaptureDriverBufferImpl**)ppvObj;
    HRESULT err;

    TRACE("(%p,%p,%x,%x)\n",iface,pwfx,dwFlags,dwCardAddress);

    if (This->capture_buffer)
        return DSERR_ALLOCATED;

    This->capture_buffer = *ippdsdb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDsCaptureDriverBufferImpl));
    if (*ippdsdb == NULL)
        return DSERR_OUTOFMEMORY;

    (*ippdsdb)->hw_params = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_hw_params_sizeof());
    (*ippdsdb)->sw_params = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_sw_params_sizeof());
    (*ippdsdb)->presented_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdwcbBufferSize);
    if (!(*ippdsdb)->hw_params || !(*ippdsdb)->sw_params || !(*ippdsdb)->presented_buffer)
    {
        HeapFree(GetProcessHeap(), 0, (*ippdsdb)->sw_params);
        HeapFree(GetProcessHeap(), 0, (*ippdsdb)->hw_params);
        HeapFree(GetProcessHeap(), 0, (*ippdsdb)->presented_buffer);
        return DSERR_OUTOFMEMORY;
    }
    (*ippdsdb)->lpVtbl = &dsdbvt;
    (*ippdsdb)->ref = 1;
    (*ippdsdb)->drv = This;
    (*ippdsdb)->mmap_buflen_bytes = *pdwcbBufferSize;
    InitializeCriticalSection(&(*ippdsdb)->pcm_crst);
    (*ippdsdb)->pcm_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ALSA_DSCAPTURE.pcm_crst");

    /* SetFormat initialises pcm */
    err = IDsDriverBuffer_SetFormat((IDsDriverBuffer*)*ppvObj, pwfx);
    if (FAILED(err))
    {
        WARN("Error occurred: %08x\n", err);
        goto err;
    }
    *ppbBuffer = (*ippdsdb)->presented_buffer;

    /* buffer is ready to go */
    TRACE("buffer created at %p\n", *ippdsdb);
    return err;

    err:
    HeapFree(GetProcessHeap(), 0, (*ippdsdb)->presented_buffer);
    HeapFree(GetProcessHeap(), 0, (*ippdsdb)->sw_params);
    HeapFree(GetProcessHeap(), 0, (*ippdsdb)->hw_params);
    HeapFree(GetProcessHeap(), 0, *ippdsdb);
    *ippdsdb = NULL;
    return err;
}

static const IDsCaptureDriverVtbl dscdvt =
{
    IDsCaptureDriverImpl_QueryInterface,
    IDsCaptureDriverImpl_AddRef,
    IDsCaptureDriverImpl_Release,
    IDsCaptureDriverImpl_GetDriverDesc,
    IDsCaptureDriverImpl_Open,
    IDsCaptureDriverImpl_Close,
    IDsCaptureDriverImpl_GetCaps,
    IDsCaptureDriverImpl_CreateCaptureBuffer
};

/**************************************************************************
 *                              widDsCreate                     [internal]
 */
DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv)
{
    IDsCaptureDriverImpl** idrv = (IDsCaptureDriverImpl**)drv;
    TRACE("(%d,%p)\n",wDevID,drv);

    if (!(WInDev[wDevID].dwSupport & WAVECAPS_DIRECTSOUND))
    {
        WARN("Hardware accelerated capture not supported, falling back to wavein\n");
        return MMSYSERR_NOTSUPPORTED;
    }

    *idrv = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsCaptureDriverImpl));
    if (!*idrv)
        return MMSYSERR_NOMEM;
    (*idrv)->lpVtbl	= &dscdvt;
    (*idrv)->ref	= 1;

    (*idrv)->wDevID	= wDevID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *                              widDsDesc                       [internal]
 */
DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memcpy(desc, &(WInDev[wDevID].ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

#endif /* HAVE_ALSA */
