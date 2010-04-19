/*
 * Copyright 2010 Maarten Lankhorst for Codeweavers
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

#define NONAMELESSUNION
#define CINTERFACE
#define COBJMACROS
#include "config.h"

#include <stdarg.h>
#ifdef HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined(HAVE_OPENAL_AL_H)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "dshow.h"
#include "dsound.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"

#include "mmdevapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

#ifdef HAVE_OPENAL

typedef struct ACRender ACRender;
typedef struct ACCapture ACCapture;
typedef struct ACSession ACSession;
typedef struct ASVolume ASVolume;
typedef struct AClock AClock;

typedef struct ACImpl {
    const IAudioClientVtbl *lpVtbl;
    LONG ref;

    MMDevice *parent;
    CRITICAL_SECTION *crst;
    DWORD init;
} ACImpl;

static const IAudioClientVtbl ACImpl_Vtbl;

HRESULT AudioClient_Create(MMDevice *parent, IAudioClient **ppv)
{
    ACImpl *This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    *ppv = (IAudioClient*)This;
    if (!*ppv)
        return E_OUTOFMEMORY;
    This->crst = &parent->crst;
    This->lpVtbl = &ACImpl_Vtbl;
    This->ref = 1;
    This->parent = parent;
    return S_OK;
}

static void AudioClient_Destroy(ACImpl *This)
{
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI AC_QueryInterface(IAudioClient *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IAudioClient))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AC_AddRef(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI AC_Release(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        AudioClient_Destroy(This);
    return ref;
}

static HRESULT WINAPI AC_Initialize(IAudioClient *iface, AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration, REFERENCE_TIME period, const WAVEFORMATEX *pwfx, const GUID *sessionguid)
{
    ACImpl *This = (ACImpl*)iface;

    TRACE("(%p)->(%x,%x,%u,%u,%p,%s)\n", This, mode, flags, (int)duration, (int)period, pwfx, debugstr_guid(sessionguid));

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_GetBufferSize(IAudioClient *iface, UINT32 *frames)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)->(%p)\n", This, frames);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!frames)
        return E_POINTER;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_GetStreamLatency(IAudioClient *iface, REFERENCE_TIME *latency)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)->(%p)\n", This, latency);

    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;

    return IAudioClient_GetDevicePeriod(iface, latency, NULL);
}

static HRESULT WINAPI AC_GetCurrentPadding(IAudioClient *iface, UINT32 *numpad)
{
    ACImpl *This = (ACImpl*)iface;

    TRACE("(%p)->(%p)\n", This, numpad);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!numpad)
        return E_POINTER;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_IsFormatSupported(IAudioClient *iface, AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *pwfx, WAVEFORMATEX **outpwfx)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)->(%x,%p,%p)\n", This, mode, pwfx, outpwfx);
    if (!pwfx)
        return E_POINTER;

    if (mode == AUDCLNT_SHAREMODE_SHARED && !outpwfx)
        return E_POINTER;
    if (mode != AUDCLNT_SHAREMODE_SHARED
        && mode != AUDCLNT_SHAREMODE_EXCLUSIVE) {
        WARN("Unknown mode %x\n", mode);
        return E_INVALIDARG;
    }
    if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE
        && pwfx->wFormatTag != WAVE_FORMAT_PCM)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    if (pwfx->nSamplesPerSec < 8000
        || pwfx->nSamplesPerSec > 192000)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE
        || !IsEqualIID(&((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
        if (pwfx->wBitsPerSample > 16)
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    if (outpwfx)
        *outpwfx = NULL;
    return S_OK;
}

static HRESULT WINAPI AC_GetMixFormat(IAudioClient *iface, WAVEFORMATEX **pwfx)
{
    ACImpl *This = (ACImpl*)iface;

    TRACE("(%p)->(%p)\n", This, pwfx);
    if (!pwfx)
        return E_POINTER;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_GetDevicePeriod(IAudioClient *iface, REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    ACImpl *This = (ACImpl*)iface;

    TRACE("(%p)->(%p)\n", This, minperiod);
    if (!defperiod && !minperiod)
        return E_POINTER;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_Start(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;

    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_Stop(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_Reset(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_SetEventHandle(IAudioClient *iface, HANDLE handle)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!handle)
        return E_INVALIDARG;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AC_GetService(IAudioClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = (ACImpl*)iface;
    HRESULT hr = S_OK;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (FAILED(hr))
        return hr;

    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("stub %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static const IAudioClientVtbl ACImpl_Vtbl =
{
    AC_QueryInterface,
    AC_AddRef,
    AC_Release,
    AC_Initialize,
    AC_GetBufferSize,
    AC_GetStreamLatency,
    AC_GetCurrentPadding,
    AC_IsFormatSupported,
    AC_GetMixFormat,
    AC_GetDevicePeriod,
    AC_Start,
    AC_Stop,
    AC_Reset,
    AC_SetEventHandle,
    AC_GetService
};

#endif
