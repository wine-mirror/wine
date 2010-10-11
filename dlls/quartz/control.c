/*
 * Filter Seeking and Control Interfaces
 *
 * Copyright 2003 Robert Shearman
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
/* FIXME: critical sections */

#include "quartz_private.h"
#include "control_private.h"

#include "uuids.h"
#include "wine/debug.h"

#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const IMediaSeekingVtbl IMediaSeekingPassThru_Vtbl;

typedef struct PassThruImpl {
    const ISeekingPassThruVtbl *IPassThru_vtbl;
    const IUnknownVtbl *IInner_vtbl;
    const IMediaSeekingVtbl *IMediaSeeking_vtbl;

    LONG ref;
    IUnknown * pUnkOuter;
    IPin * pin;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
    BOOL renderer;
    CRITICAL_SECTION time_cs;
    BOOL timevalid;
    REFERENCE_TIME time_earliest;
} PassThruImpl;

static HRESULT WINAPI SeekInner_QueryInterface(IUnknown * iface,
					  REFIID riid,
					  LPVOID *ppvObj) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    TRACE("(%p)->(%s (%p), %p)\n", This, debugstr_guid(riid), riid, ppvObj);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppvObj = &(This->IInner_vtbl);
        TRACE("   returning IUnknown interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_ISeekingPassThru, riid)) {
        *ppvObj = &(This->IPassThru_vtbl);
        TRACE("   returning ISeekingPassThru interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaSeeking, riid)) {
        *ppvObj = &(This->IMediaSeeking_vtbl);
        TRACE("   returning IMediaSeeking interface (%p)\n", *ppvObj);
    } else {
        *ppvObj = NULL;
        FIXME("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)(*ppvObj));
    return S_OK;
}

static ULONG WINAPI SeekInner_AddRef(IUnknown * iface) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI SeekInner_Release(IUnknown * iface) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (ref == 0)
    {
        This->time_cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->time_cs);
        CoTaskMemFree(This);
    }
    return ref;
}

static const IUnknownVtbl IInner_VTable =
{
    SeekInner_QueryInterface,
    SeekInner_AddRef,
    SeekInner_Release
};

/* Generic functions for aggregation */
static HRESULT SeekOuter_QueryInterface(PassThruImpl *This, REFIID riid, LPVOID *ppv)
{
    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->pUnkOuter)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->pUnkOuter, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
            hr = IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
            IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
}

static ULONG SeekOuter_AddRef(PassThruImpl *This)
{
    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_AddRef(This->pUnkOuter);
    return IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
}

static ULONG SeekOuter_Release(PassThruImpl *This)
{
    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_Release(This->pUnkOuter);
    return IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
}

static HRESULT WINAPI SeekingPassThru_QueryInterface(ISeekingPassThru *iface, REFIID riid, LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return SeekOuter_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI SeekingPassThru_AddRef(ISeekingPassThru *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return SeekOuter_AddRef(This);
}

static ULONG WINAPI SeekingPassThru_Release(ISeekingPassThru *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return SeekOuter_Release(This);
}

static HRESULT WINAPI SeekingPassThru_Init(ISeekingPassThru *iface, BOOL renderer, IPin *pin)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, renderer, pin);

    if (This->pin)
        FIXME("Re-initializing?\n");

    This->renderer = renderer;
    This->pin = pin;

    return S_OK;
}

static const ISeekingPassThruVtbl ISeekingPassThru_Vtbl =
{
    SeekingPassThru_QueryInterface,
    SeekingPassThru_AddRef,
    SeekingPassThru_Release,
    SeekingPassThru_Init
};

HRESULT SeekingPassThru_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    PassThruImpl *fimpl;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    *ppObj = fimpl = CoTaskMemAlloc(sizeof(*fimpl));
    if (!fimpl)
        return E_OUTOFMEMORY;

    fimpl->pUnkOuter = pUnkOuter;
    fimpl->bUnkOuterValid = FALSE;
    fimpl->bAggregatable = FALSE;
    fimpl->IInner_vtbl = &IInner_VTable;
    fimpl->IPassThru_vtbl = &ISeekingPassThru_Vtbl;
    fimpl->IMediaSeeking_vtbl = &IMediaSeekingPassThru_Vtbl;
    fimpl->ref = 1;
    fimpl->pin = NULL;
    fimpl->timevalid = 0;
    InitializeCriticalSection(&fimpl->time_cs);
    fimpl->time_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PassThruImpl.time_cs");
    return S_OK;
}

static HRESULT WINAPI MediaSeekingPassThru_QueryInterface(IMediaSeeking *iface, REFIID riid, LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return SeekOuter_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaSeekingPassThru_AddRef(IMediaSeeking *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_AddRef(This);
}

static ULONG WINAPI MediaSeekingPassThru_Release(IMediaSeeking *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_Release(This);
}

static HRESULT get_connected(PassThruImpl *This, IMediaSeeking **seek) {
    HRESULT hr;
    IPin *pin;
    *seek = NULL;
    hr = IPin_ConnectedTo(This->pin, &pin);
    if (FAILED(hr))
        return VFW_E_NOT_CONNECTED;
    hr = IPin_QueryInterface(pin, &IID_IMediaSeeking, (void**)seek);
    IPin_Release(pin);
    if (FAILED(hr))
        hr = E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetCapabilities(seek, pCapabilities);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_CheckCapabilities(seek, pCapabilities);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, qzdebugstr_guid(pFormat));
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_IsFormatSupported(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_QueryPreferredFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetTimeFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, qzdebugstr_guid(pFormat));
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_IsUsingTimeFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, qzdebugstr_guid(pFormat));
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_SetTimeFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pDuration);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetDuration(seek, pDuration);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pStop);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetStopPosition(seek, pStop);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr = S_OK;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCurrent);
    if (!pCurrent)
        return E_POINTER;
    EnterCriticalSection(&This->time_cs);
    if (This->timevalid)
        *pCurrent = This->time_earliest;
    else
        hr = E_FAIL;
    LeaveCriticalSection(&This->time_cs);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_ConvertTimeFormat(iface, pCurrent, NULL, *pCurrent, &TIME_FORMAT_MEDIA_TIME);
        return hr;
    }
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetCurrentPosition(seek, pCurrent);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%s,%x%08x,%s)\n", iface, This, pTarget, debugstr_guid(pTargetFormat), (DWORD)(Source>>32), (DWORD)Source, debugstr_guid(pSourceFormat));
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_ConvertTimeFormat(seek, pTarget, pTargetFormat, Source, pSourceFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%x,%p,%x)\n", iface, This, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_SetPositions(seek, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
        IMediaSeeking_Release(seek);
    } else if (hr == VFW_E_NOT_CONNECTED)
        hr = S_OK;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pCurrent, pStop);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetPositions(seek, pCurrent, pStop);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%p)\n", iface, This, pEarliest, pLatest);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetAvailable(seek, pEarliest, pLatest);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_SetRate(IMediaSeeking * iface, double dRate)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%e)\n", iface, This, dRate);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_SetRate(seek, dRate);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetRate(IMediaSeeking * iface, double * dRate)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, dRate);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetRate(seek, dRate);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p)\n", pPreroll);
    hr = get_connected(This, &seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetPreroll(seek, pPreroll);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

void MediaSeekingPassThru_RegisterMediaTime(IUnknown *iface, REFERENCE_TIME start) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    EnterCriticalSection(&This->time_cs);
    This->time_earliest = start;
    This->timevalid = 1;
    LeaveCriticalSection(&This->time_cs);
}

void MediaSeekingPassThru_ResetMediaTime(IUnknown *iface) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    EnterCriticalSection(&This->time_cs);
    This->timevalid = 0;
    LeaveCriticalSection(&This->time_cs);
}

void MediaSeekingPassThru_EOS(IUnknown *iface) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    REFERENCE_TIME time;
    HRESULT hr;
    hr = IMediaSeeking_GetStopPosition((IMediaSeeking*)&This->IMediaSeeking_vtbl, &time);
    EnterCriticalSection(&This->time_cs);
    if (SUCCEEDED(hr)) {
        This->timevalid = 1;
        This->time_earliest = time;
    } else
        This->timevalid = 0;
    LeaveCriticalSection(&This->time_cs);
}

static const IMediaSeekingVtbl IMediaSeekingPassThru_Vtbl =
{
    MediaSeekingPassThru_QueryInterface,
    MediaSeekingPassThru_AddRef,
    MediaSeekingPassThru_Release,
    MediaSeekingPassThru_GetCapabilities,
    MediaSeekingPassThru_CheckCapabilities,
    MediaSeekingPassThru_IsFormatSupported,
    MediaSeekingPassThru_QueryPreferredFormat,
    MediaSeekingPassThru_GetTimeFormat,
    MediaSeekingPassThru_IsUsingTimeFormat,
    MediaSeekingPassThru_SetTimeFormat,
    MediaSeekingPassThru_GetDuration,
    MediaSeekingPassThru_GetStopPosition,
    MediaSeekingPassThru_GetCurrentPosition,
    MediaSeekingPassThru_ConvertTimeFormat,
    MediaSeekingPassThru_SetPositions,
    MediaSeekingPassThru_GetPositions,
    MediaSeekingPassThru_GetAvailable,
    MediaSeekingPassThru_SetRate,
    MediaSeekingPassThru_GetRate,
    MediaSeekingPassThru_GetPreroll
};
