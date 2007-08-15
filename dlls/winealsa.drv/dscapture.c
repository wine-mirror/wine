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

WINE_DEFAULT_DEBUG_CHANNEL(dsalsa);

typedef struct IDsCaptureDriverImpl IDsCaptureDriverImpl;
typedef struct IDsCaptureDriverBufferImpl IDsCaptureDriverBufferImpl;

struct IDsCaptureDriverImpl
{
    /* IUnknown fields */
    const IDsCaptureDriverVtbl *lpVtbl;
    LONG ref;

    /* IDsCaptureDriverImpl fields */
    IDsCaptureDriverBufferImpl* capture_buffer;
    UINT wDevID;
};

struct IDsCaptureDriverBufferImpl
{
    const IDsCaptureDriverBufferVtbl *lpVtbl;
    LONG ref;
    IDsCaptureDriverImpl* drv;

    CRITICAL_SECTION pcm_crst;
    LPBYTE mmap_buffer, presented_buffer;
    DWORD mmap_buflen_bytes, play_looping, mmap_ofs_bytes;
};

static HRESULT WINAPI IDsCaptureDriverBufferImpl_QueryInterface(PIDSCDRIVERBUFFER iface, REFIID riid, LPVOID *ppobj)
{
    /* IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface; */
    FIXME("(): stub!\n");
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

    TRACE("mmap buffer %p destroyed\n", This->mmap_buffer);

    This->drv->capture_buffer = NULL;
    This->pcm_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->pcm_crst);

    HeapFree(GetProcessHeap(), 0, This->presented_buffer);
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
    FIXME("stub!\n");
    return DSERR_BADFORMAT;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_GetPosition(PIDSCDRIVERBUFFER iface, LPDWORD lpdwCappos, LPDWORD lpdwReadpos)
{
    FIXME("stub!\n");
    return DSERR_GENERIC;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_GetStatus(PIDSCDRIVERBUFFER iface, LPDWORD lpdwStatus)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    snd_pcm_state_t state;
    TRACE("(%p,%p)\n",iface,lpdwStatus);

    state = 0;
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
    This->play_looping = !!(dwFlags & DSCBSTART_LOOPING);
    if (!This->play_looping)
        /* Not well supported because of the difference in ALSA size and DSOUND's notion of size
         * what it does right now is fill the buffer once.. ALSA size */
        FIXME("Non-looping buffers are not properly supported!\n");
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

    (*ippdsdb)->presented_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdwcbBufferSize);
    if (!(*ippdsdb)->presented_buffer)
    {
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
    HeapFree(GetProcessHeap(), 0, *ippdsdb);
    *ippdsdb = NULL;
    return err;
}
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
