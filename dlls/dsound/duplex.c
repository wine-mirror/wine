/*              DirectSoundFullDuplex
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
 * Copyright 2005 Robert Reif
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

#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "mmddk.h"
#include "winternl.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/*****************************************************************************
 * IDirectSoundFullDuplex implementation structure
 */
typedef struct IDirectSoundFullDuplexImpl
{
    IDirectSoundFullDuplex IDirectSoundFullDuplex_iface;
    LONG                   ref, numIfaces;
    /* IDirectSoundFullDuplexImpl fields */
    IDirectSound8                    *renderer_device;
    IDirectSoundCapture              *capture_device;

    LPUNKNOWN                         pUnknown;
    LPDIRECTSOUND8                    pDS8;
    LPDIRECTSOUNDCAPTURE              pDSC;
} IDirectSoundFullDuplexImpl;

typedef struct IDirectSoundFullDuplex_IUnknown {
    const IUnknownVtbl         *lpVtbl;
    LONG                        ref;
    IDirectSoundFullDuplexImpl *pdsfd;
} IDirectSoundFullDuplex_IUnknown;

typedef struct IDirectSoundFullDuplex_IDirectSound8 {
    const IDirectSound8Vtbl    *lpVtbl;
    LONG                        ref;
    IDirectSoundFullDuplexImpl *pdsfd;
} IDirectSoundFullDuplex_IDirectSound8;

typedef struct IDirectSoundFullDuplex_IDirectSoundCapture {
    const IDirectSoundCaptureVtbl *lpVtbl;
    LONG                           ref;
    IDirectSoundFullDuplexImpl    *pdsfd;
} IDirectSoundFullDuplex_IDirectSoundCapture;

static void fullduplex_destroy(IDirectSoundFullDuplexImpl *This)
{
    if (This->capture_device)
        IDirectSoundCapture_Release(This->capture_device);
    if (This->renderer_device)
        IDirectSound_Release(This->renderer_device);
    HeapFree(GetProcessHeap(), 0, This);
    TRACE("(%p) released\n", This);
}

/*******************************************************************************
 * IUnknown
 */
static HRESULT WINAPI IDirectSoundFullDuplex_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundFullDuplex_IUnknown *This = (IDirectSoundFullDuplex_IUnknown *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return IDirectSoundFullDuplex_QueryInterface((LPDIRECTSOUNDFULLDUPLEX)This->pdsfd, riid, ppobj);
}

static ULONG WINAPI IDirectSoundFullDuplex_IUnknown_AddRef(
    LPUNKNOWN iface)
{
    IDirectSoundFullDuplex_IUnknown *This = (IDirectSoundFullDuplex_IUnknown *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundFullDuplex_IUnknown_Release(
    LPUNKNOWN iface)
{
    IDirectSoundFullDuplex_IUnknown *This = (IDirectSoundFullDuplex_IUnknown *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        This->pdsfd->pUnknown = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static const IUnknownVtbl DirectSoundFullDuplex_Unknown_Vtbl =
{
    IDirectSoundFullDuplex_IUnknown_QueryInterface,
    IDirectSoundFullDuplex_IUnknown_AddRef,
    IDirectSoundFullDuplex_IUnknown_Release
};

static HRESULT IDirectSoundFullDuplex_IUnknown_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPUNKNOWN * ppunk)
{
    IDirectSoundFullDuplex_IUnknown * pdsfdunk;
    TRACE("(%p,%p)\n",pdsfd,ppunk);

    if (pdsfd == NULL) {
        ERR("invalid parameter: pdsfd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppunk == NULL) {
        ERR("invalid parameter: ppunk == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    pdsfdunk = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsfdunk));
    if (pdsfdunk == NULL) {
        WARN("out of memory\n");
        *ppunk = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsfdunk->lpVtbl = &DirectSoundFullDuplex_Unknown_Vtbl;
    pdsfdunk->ref = 0;
    pdsfdunk->pdsfd = (IDirectSoundFullDuplexImpl *)pdsfd;

    *ppunk = (LPUNKNOWN)pdsfdunk;

    return DS_OK;
}

/*******************************************************************************
 * IDirectSoundFullDuplex_IDirectSound8
 */
static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_QueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return IDirectSoundFullDuplex_QueryInterface(&This->pdsfd->IDirectSoundFullDuplex_iface, riid, ppobj);
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSound8_AddRef(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSound8_Release(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        This->pdsfd->pDS8 = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_CreateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return IDirectSound8_CreateSoundBuffer(This->pdsfd->renderer_device,dsbd,ppdsb,lpunk);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_GetCaps(
    LPDIRECTSOUND8 iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return IDirectSound8_GetCaps(This->pdsfd->renderer_device, lpDSCaps);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_DuplicateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return IDirectSound8_DuplicateSoundBuffer(This->pdsfd->renderer_device,psb,ppdsb);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_SetCooperativeLevel(
    LPDIRECTSOUND8 iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,%p,%s)\n",This,hwnd,dumpCooperativeLevel(level));
    return IDirectSound8_SetCooperativeLevel(This->pdsfd->renderer_device,hwnd,level);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_Compact(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p)\n", This);
    return IDirectSound8_Compact(This->pdsfd->renderer_device);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_GetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return IDirectSound8_GetSpeakerConfig(This->pdsfd->renderer_device,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_SetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    DWORD config)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p,0x%08x)\n",This,config);
    return IDirectSound8_SetSpeakerConfig(This->pdsfd->renderer_device,config);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_Initialize(
    LPDIRECTSOUND8 iface,
    LPCGUID lpcGuid)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return IDirectSound8_Initialize(This->pdsfd->renderer_device,lpcGuid);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSound8_VerifyCertification(
    LPDIRECTSOUND8 iface,
    DWORD *cert)
{
    IDirectSoundFullDuplex_IDirectSound8 *This = (IDirectSoundFullDuplex_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, cert);
    return IDirectSound8_VerifyCertification(This->pdsfd->renderer_device,cert);
}

static const IDirectSound8Vtbl DirectSoundFullDuplex_DirectSound8_Vtbl =
{
    IDirectSoundFullDuplex_IDirectSound8_QueryInterface,
    IDirectSoundFullDuplex_IDirectSound8_AddRef,
    IDirectSoundFullDuplex_IDirectSound8_Release,
    IDirectSoundFullDuplex_IDirectSound8_CreateSoundBuffer,
    IDirectSoundFullDuplex_IDirectSound8_GetCaps,
    IDirectSoundFullDuplex_IDirectSound8_DuplicateSoundBuffer,
    IDirectSoundFullDuplex_IDirectSound8_SetCooperativeLevel,
    IDirectSoundFullDuplex_IDirectSound8_Compact,
    IDirectSoundFullDuplex_IDirectSound8_GetSpeakerConfig,
    IDirectSoundFullDuplex_IDirectSound8_SetSpeakerConfig,
    IDirectSoundFullDuplex_IDirectSound8_Initialize,
    IDirectSoundFullDuplex_IDirectSound8_VerifyCertification
};

static HRESULT IDirectSoundFullDuplex_IDirectSound8_Create(
    IDirectSoundFullDuplexImpl *pdsfd,
    LPDIRECTSOUND8 * ppds8)
{
    IDirectSoundFullDuplex_IDirectSound8 * pdsfdds8;
    TRACE("(%p,%p)\n",pdsfd,ppds8);

    if (pdsfd == NULL) {
        ERR("invalid parameter: pdsfd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppds8 == NULL) {
        ERR("invalid parameter: ppds8 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pdsfd->renderer_device == NULL) {
        WARN("not initialized\n");
        *ppds8 = NULL;
        return DSERR_UNINITIALIZED;
    }

    pdsfdds8 = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsfdds8));
    if (pdsfdds8 == NULL) {
        WARN("out of memory\n");
        *ppds8 = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsfdds8->lpVtbl = &DirectSoundFullDuplex_DirectSound8_Vtbl;
    pdsfdds8->ref = 0;
    pdsfdds8->pdsfd = pdsfd;

    *ppds8 = (LPDIRECTSOUND8)pdsfdds8;

    return DS_OK;
}

/*******************************************************************************
 * IDirectSoundFullDuplex_IDirectSoundCapture
 */
static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_QueryInterface(
    LPDIRECTSOUNDCAPTURE iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return IDirectSoundFullDuplex_QueryInterface(&This->pdsfd->IDirectSoundFullDuplex_iface, riid, ppobj);
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_AddRef(
    LPDIRECTSOUNDCAPTURE iface)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_Release(
    LPDIRECTSOUNDCAPTURE iface)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);
    if (!ref) {
        This->pdsfd->pDSC = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_CreateCaptureBuffer(
    LPDIRECTSOUNDCAPTURE iface,
    LPCDSCBUFFERDESC lpcDSCBufferDesc,
    LPDIRECTSOUNDCAPTUREBUFFER* lplpDSCaptureBuffer,
    LPUNKNOWN pUnk)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,lpcDSCBufferDesc,lplpDSCaptureBuffer,pUnk);
    return IDirectSoundCapture_CreateCaptureBuffer(This->pdsfd->capture_device,lpcDSCBufferDesc,lplpDSCaptureBuffer,pUnk);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_GetCaps(
    LPDIRECTSOUNDCAPTURE iface,
    LPDSCCAPS lpDSCCaps)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p,%p)\n",This,lpDSCCaps);
    return IDirectSoundCapture_GetCaps(This->pdsfd->capture_device, lpDSCCaps);
}

static HRESULT WINAPI IDirectSoundFullDuplex_IDirectSoundCapture_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID)
{
    IDirectSoundFullDuplex_IDirectSoundCapture *This = (IDirectSoundFullDuplex_IDirectSoundCapture *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGUID));
    return IDirectSoundCapture_Initialize(This->pdsfd->capture_device,lpcGUID);
}

static const IDirectSoundCaptureVtbl DirectSoundFullDuplex_DirectSoundCapture_Vtbl =
{
    IDirectSoundFullDuplex_IDirectSoundCapture_QueryInterface,
    IDirectSoundFullDuplex_IDirectSoundCapture_AddRef,
    IDirectSoundFullDuplex_IDirectSoundCapture_Release,
    IDirectSoundFullDuplex_IDirectSoundCapture_CreateCaptureBuffer,
    IDirectSoundFullDuplex_IDirectSoundCapture_GetCaps,
    IDirectSoundFullDuplex_IDirectSoundCapture_Initialize
};

static HRESULT IDirectSoundFullDuplex_IDirectSoundCapture_Create(
    IDirectSoundFullDuplexImpl *pdsfd,
    LPDIRECTSOUNDCAPTURE8 * ppdsc8)
{
    IDirectSoundFullDuplex_IDirectSoundCapture * pdsfddsc;
    TRACE("(%p,%p)\n",pdsfd,ppdsc8);

    if (pdsfd == NULL) {
        ERR("invalid parameter: pdsfd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppdsc8 == NULL) {
        ERR("invalid parameter: ppdsc8 == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pdsfd->capture_device == NULL) {
        WARN("not initialized\n");
        *ppdsc8 = NULL;
        return DSERR_UNINITIALIZED;
    }

    pdsfddsc = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsfddsc));
    if (pdsfddsc == NULL) {
        WARN("out of memory\n");
        *ppdsc8 = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsfddsc->lpVtbl = &DirectSoundFullDuplex_DirectSoundCapture_Vtbl;
    pdsfddsc->ref = 0;
    pdsfddsc->pdsfd = pdsfd;

    *ppdsc8 = (LPDIRECTSOUNDCAPTURE)pdsfddsc;

    return DS_OK;
}

/***************************************************************************
 * IDirectSoundFullDuplex implementation
 */
static inline IDirectSoundFullDuplexImpl *impl_from_IDirectSoundFullDuplex(IDirectSoundFullDuplex *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundFullDuplexImpl, IDirectSoundFullDuplex_iface);
}

static ULONG WINAPI IDirectSoundFullDuplexImpl_AddRef(IDirectSoundFullDuplex *iface)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    ULONG ref = InterlockedIncrement(&(This->ref));

    TRACE("(%p) ref=%d\n", This, ref);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);
    return ref;
}

static HRESULT WINAPI IDirectSoundFullDuplexImpl_QueryInterface(IDirectSoundFullDuplex *iface,
        REFIID riid, void **ppv)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppv);

    if (ppv == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)) {
        if (!This->pUnknown) {
            IDirectSoundFullDuplex_IUnknown_Create(iface, &This->pUnknown);
            if (!This->pUnknown) {
                WARN("IDirectSoundFullDuplex_IUnknown_Create() failed\n");
                *ppv = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSoundFullDuplex_IUnknown_AddRef(This->pUnknown);
        *ppv = This->pUnknown;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSoundFullDuplex)) {
        IDirectSoundFullDuplexImpl_AddRef(iface);
        *ppv = &This->IDirectSoundFullDuplex_iface;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound)
               || IsEqualIID(riid, &IID_IDirectSound8)) {
        if (!This->pDS8) {
            IDirectSoundFullDuplex_IDirectSound8_Create(This, &This->pDS8);
            if (!This->pDS8) {
                WARN("IDirectSoundFullDuplex_IDirectSound8_Create() failed\n");
                *ppv = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSoundFullDuplex_IDirectSound8_AddRef(This->pDS8);
        *ppv = This->pDS8;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSoundCapture)) {
        if (!This->pDSC) {
            IDirectSoundFullDuplex_IDirectSoundCapture_Create(This, &This->pDSC);
            if (!This->pDSC) {
                WARN("IDirectSoundFullDuplex_IDirectSoundCapture_Create() failed\n");
                *ppv = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSoundFullDuplex_IDirectSoundCapture_AddRef(This->pDSC);
        *ppv = This->pDSC;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectSoundFullDuplexImpl_Release(IDirectSoundFullDuplex *iface)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    ULONG ref = InterlockedDecrement(&(This->ref));

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        fullduplex_destroy(This);
    return ref;
}

static HRESULT WINAPI IDirectSoundFullDuplexImpl_Initialize(IDirectSoundFullDuplex *iface,
        const GUID *capture_dev, const GUID *render_dev, const DSCBUFFERDESC *cbufdesc,
        const DSBUFFERDESC *bufdesc, HWND hwnd, DWORD level, IDirectSoundCaptureBuffer8 **dscb8,
        IDirectSoundBuffer8 **dsb8)
{
    IDirectSoundFullDuplexImpl *This = impl_from_IDirectSoundFullDuplex(iface);
    HRESULT hr;

    TRACE("(%p,%s,%s,%p,%p,%p,%x,%p,%p)\n", This, debugstr_guid(capture_dev),
            debugstr_guid(render_dev), cbufdesc, bufdesc, hwnd, level, dscb8, dsb8);

    if (!dscb8 || !dsb8)
        return E_INVALIDARG;

    *dscb8 = NULL;
    *dsb8 = NULL;

    if (This->renderer_device != NULL || This->capture_device != NULL) {
        WARN("already initialized\n");
        return DSERR_ALREADYINITIALIZED;
    }

    hr = DSOUND_Create8(&IID_IDirectSound8, (void **)&This->renderer_device);
    if (SUCCEEDED(hr))
        hr = IDirectSound_Initialize(This->renderer_device, render_dev);
    if (hr != DS_OK) {
        WARN("DirectSoundDevice_Initialize() failed\n");
        goto error;
    }

    IDirectSound8_SetCooperativeLevel(This->renderer_device, hwnd, level);

    hr = IDirectSound8_CreateSoundBuffer(This->renderer_device, bufdesc,
        (IDirectSoundBuffer**)dsb8, NULL);
    if (hr != DS_OK) {
        WARN("IDirectSoundBufferImpl_Create() failed\n");
        goto error;
    }

    hr = DSOUND_CaptureCreate8(&IID_IDirectSoundCapture8, (void**)&This->capture_device);
    if (SUCCEEDED(hr))
        hr = IDirectSoundCapture_Initialize(This->capture_device, capture_dev);
    if (hr != DS_OK) {
        WARN("DirectSoundCaptureDevice_Initialize() failed\n");
        goto error;
    }

    hr = IDirectSoundCapture_CreateCaptureBuffer(This->capture_device, cbufdesc,
            (IDirectSoundCaptureBuffer**)dscb8, NULL);
    if (hr != DS_OK) {
        WARN("IDirectSoundCaptureBufferImpl_Create() failed\n");
        goto error;
    }

    return DS_OK;

error:
    if (*dsb8) {
        IDirectSoundBuffer8_Release(*dsb8);
        *dsb8 = NULL;
    }
    if (This->renderer_device) {
        IDirectSound8_Release(This->renderer_device);
        This->renderer_device = NULL;
    }
    if (*dscb8) {
        IDirectSoundCaptureBuffer8_Release(*dscb8);
        *dscb8 = NULL;
    }
    if (This->capture_device) {
        IDirectSoundCapture_Release(This->capture_device);
        This->capture_device = NULL;
    }
    return hr;
}

static const IDirectSoundFullDuplexVtbl dsfd_vtbl =
{
    /* IUnknown methods */
    IDirectSoundFullDuplexImpl_QueryInterface,
    IDirectSoundFullDuplexImpl_AddRef,
    IDirectSoundFullDuplexImpl_Release,

    /* IDirectSoundFullDuplex methods */
    IDirectSoundFullDuplexImpl_Initialize
};

HRESULT DSOUND_FullDuplexCreate(REFIID riid, void **ppv)
{
    IDirectSoundFullDuplexImpl *obj;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));
    if (!obj) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    setup_dsound_options();

    obj->IDirectSoundFullDuplex_iface.lpVtbl = &dsfd_vtbl;
    obj->ref = 1;
    obj->numIfaces = 1;

    hr = IUnknown_QueryInterface((IUnknown*)obj, riid, ppv);
    IUnknown_Release((IUnknown*)obj);

    return hr;
}

/***************************************************************************
 * DirectSoundFullDuplexCreate [DSOUND.10]
 *
 * Create and initialize a DirectSoundFullDuplex interface.
 *
 * PARAMS
 *    capture_dev         [I] Address of sound capture device GUID.
 *    render_dev          [I] Address of sound render device GUID.
 *    cbufdesc            [I] Address of capture buffer description.
 *    bufdesc             [I] Address of  render buffer description.
 *    hwnd                [I] Handle to application window.
 *    level               [I] Cooperative level.
 *    dsfd                [O] Address where full duplex interface returned.
 *    dscb8               [0] Address where capture buffer interface returned.
 *    dsb8                [0] Address where render buffer interface returned.
 *    outer_unk           [I] Must be NULL.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_NOAGGREGATION, DSERR_ALLOCATED, DSERR_INVALIDPARAM,
 *             DSERR_OUTOFMEMORY DSERR_INVALIDCALL DSERR_NODRIVER
 */
HRESULT WINAPI DirectSoundFullDuplexCreate(const GUID *capture_dev, const GUID *render_dev,
        const DSCBUFFERDESC *cbufdesc, const DSBUFFERDESC *bufdesc, HWND hwnd, DWORD level,
        IDirectSoundFullDuplex **dsfd, IDirectSoundCaptureBuffer8 **dscb8,
        IDirectSoundBuffer8 **dsb8, IUnknown *outer_unk)
{
    HRESULT hr;

    TRACE("(%s,%s,%p,%p,%p,%x,%p,%p,%p,%p)\n", debugstr_guid(capture_dev),
            debugstr_guid(render_dev), cbufdesc, bufdesc, hwnd, level, dsfd, dscb8, dsb8,
            outer_unk);

    if (!dsfd)
        return DSERR_INVALIDPARAM;
    if (outer_unk) {
        *dsfd = NULL;
        return DSERR_NOAGGREGATION;
    }

    hr = DSOUND_FullDuplexCreate(&IID_IDirectSoundFullDuplex, (void**)dsfd);
    if (hr == DS_OK) {
        hr = IDirectSoundFullDuplex_Initialize(*dsfd, capture_dev, render_dev, cbufdesc, bufdesc,
                hwnd, level, dscb8, dsb8);
        if (hr != DS_OK) {
            IDirectSoundFullDuplex_Release(*dsfd);
            *dsfd = NULL;
            WARN("IDirectSoundFullDuplexImpl_Initialize failed\n");
        }
    }

    return hr;
}
