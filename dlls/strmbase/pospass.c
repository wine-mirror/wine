/*
 * Filter Seeking and Control Interfaces
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

typedef struct PassThruImpl {
    ISeekingPassThru ISeekingPassThru_iface;
    IMediaSeeking IMediaSeeking_iface;
    IMediaPosition IMediaPosition_iface;

    IUnknown * outer_unk;
    IPin * pin;
    BOOL renderer;
    CRITICAL_SECTION time_cs;
    BOOL timevalid;
    REFERENCE_TIME time_earliest;
} PassThruImpl;

static inline PassThruImpl *impl_from_ISeekingPassThru(ISeekingPassThru *iface)
{
    return CONTAINING_RECORD(iface, PassThruImpl, ISeekingPassThru_iface);
}

static inline PassThruImpl *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, PassThruImpl, IMediaSeeking_iface);
}

static inline PassThruImpl *impl_from_IMediaPosition(IMediaPosition *iface)
{
    return CONTAINING_RECORD(iface, PassThruImpl, IMediaPosition_iface);
}

static HRESULT WINAPI SeekingPassThru_QueryInterface(ISeekingPassThru *iface, REFIID iid, void **out)
{
    PassThruImpl *passthrough = impl_from_ISeekingPassThru(iface);

    return IUnknown_QueryInterface(passthrough->outer_unk, iid, out);
}

static ULONG WINAPI SeekingPassThru_AddRef(ISeekingPassThru *iface)
{
    PassThruImpl *passthrough = impl_from_ISeekingPassThru(iface);

    return IUnknown_AddRef(passthrough->outer_unk);
}

static ULONG WINAPI SeekingPassThru_Release(ISeekingPassThru *iface)
{
    PassThruImpl *passthrough = impl_from_ISeekingPassThru(iface);

    return IUnknown_Release(passthrough->outer_unk);
}

static HRESULT WINAPI SeekingPassThru_Init(ISeekingPassThru *iface, BOOL renderer, IPin *pin)
{
    PassThruImpl *This = impl_from_ISeekingPassThru(iface);

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

HRESULT WINAPI CreatePosPassThru(IUnknown* pUnkOuter, BOOL bRenderer, IPin *pPin, IUnknown **ppPassThru)
{
    HRESULT hr;
    ISeekingPassThru *passthru;

    hr = CoCreateInstance(&CLSID_SeekingPassThru, pUnkOuter, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)ppPassThru);
    if (FAILED(hr))
        return hr;

    IUnknown_QueryInterface(*ppPassThru, &IID_ISeekingPassThru, (void**)&passthru);
    hr = ISeekingPassThru_Init(passthru, bRenderer, pPin);
    ISeekingPassThru_Release(passthru);

    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    PassThruImpl *passthrough = impl_from_IMediaSeeking(iface);

    return IUnknown_QueryInterface(passthrough->outer_unk, iid, out);
}

static ULONG WINAPI MediaSeekingPassThru_AddRef(IMediaSeeking *iface)
{
    PassThruImpl *passthrough = impl_from_IMediaSeeking(iface);

    return IUnknown_AddRef(passthrough->outer_unk);
}

static ULONG WINAPI MediaSeekingPassThru_Release(IMediaSeeking *iface)
{
    PassThruImpl *passthrough = impl_from_IMediaSeeking(iface);

    return IUnknown_Release(passthrough->outer_unk);
}

static HRESULT get_connected(PassThruImpl *This, REFIID riid, LPVOID *ppvObj) {
    HRESULT hr;
    IPin *pin;
    *ppvObj = NULL;
    hr = IPin_ConnectedTo(This->pin, &pin);
    if (FAILED(hr))
        return VFW_E_NOT_CONNECTED;
    hr = IPin_QueryInterface(pin, riid, ppvObj);
    IPin_Release(pin);
    if (FAILED(hr))
        hr = E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, debugstr_guid(pFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, debugstr_guid(pFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, debugstr_guid(pFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pDuration);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pStop);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
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
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%s,%x%08x,%s)\n", iface, This, pTarget, debugstr_guid(pTargetFormat), (DWORD)(Source>>32), (DWORD)Source, debugstr_guid(pSourceFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%x,%p,%x)\n", iface, This, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_SetPositions(seek, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
        IMediaSeeking_Release(seek);
    } else if (hr == VFW_E_NOT_CONNECTED)
        hr = S_OK;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pCurrent, pStop);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%p)\n", iface, This, pEarliest, pLatest);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%e)\n", iface, This, dRate);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, dRate);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
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
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p)\n", pPreroll);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetPreroll(seek, pPreroll);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
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

static HRESULT WINAPI MediaPositionPassThru_QueryInterface(IMediaPosition *iface, REFIID iid, void **out)
{
    PassThruImpl *passthrough = impl_from_IMediaPosition(iface);

    return IUnknown_QueryInterface(passthrough->outer_unk, iid, out);
}

static ULONG WINAPI MediaPositionPassThru_AddRef(IMediaPosition *iface)
{
    PassThruImpl *passthrough = impl_from_IMediaPosition(iface);

    return IUnknown_AddRef(passthrough->outer_unk);
}

static ULONG WINAPI MediaPositionPassThru_Release(IMediaPosition *iface)
{
    PassThruImpl *passthrough = impl_from_IMediaPosition(iface);

    return IUnknown_Release(passthrough->outer_unk);
}

static HRESULT WINAPI MediaPositionPassThru_GetTypeInfoCount(IMediaPosition *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI MediaPositionPassThru_GetTypeInfo(IMediaPosition *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#x, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IMediaPosition_tid, typeinfo);
}

static HRESULT WINAPI MediaPositionPassThru_GetIDsOfNames(IMediaPosition *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#x, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaPosition_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_Invoke(IMediaPosition *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %d, iid %s, lcid %#x, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaPosition_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_Duration(IMediaPosition *iface, REFTIME *plength)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", plength);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_Duration(pos, plength);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_CurrentPosition(IMediaPosition *iface, REFTIME llTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("iface %p, time %.16e.\n", iface, llTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_CurrentPosition(pos, llTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_CurrentPosition(IMediaPosition *iface, REFTIME *pllTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pllTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_CurrentPosition(pos, pllTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_StopTime(IMediaPosition *iface, REFTIME *pllTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pllTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_StopTime(pos, pllTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_StopTime(IMediaPosition *iface, REFTIME llTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("iface %p, time %.16e.\n", iface, llTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_StopTime(pos, llTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_PrerollTime(IMediaPosition *iface, REFTIME *pllTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pllTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_PrerollTime(pos, pllTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_PrerollTime(IMediaPosition *iface, REFTIME llTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("iface %p, time %.16e.\n", iface, llTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_PrerollTime(pos, llTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_Rate(IMediaPosition *iface, double dRate)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%f)\n", dRate);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_Rate(pos, dRate);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_Rate(IMediaPosition *iface, double *pdRate)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pdRate);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_Rate(pos, pdRate);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_CanSeekForward(IMediaPosition *iface, LONG *pCanSeekForward)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pCanSeekForward);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_CanSeekForward(pos, pCanSeekForward);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_CanSeekBackward(IMediaPosition *iface, LONG *pCanSeekBackward)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pCanSeekBackward);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_CanSeekBackward(pos, pCanSeekBackward);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static const IMediaPositionVtbl IMediaPositionPassThru_Vtbl =
{
    MediaPositionPassThru_QueryInterface,
    MediaPositionPassThru_AddRef,
    MediaPositionPassThru_Release,
    MediaPositionPassThru_GetTypeInfoCount,
    MediaPositionPassThru_GetTypeInfo,
    MediaPositionPassThru_GetIDsOfNames,
    MediaPositionPassThru_Invoke,
    MediaPositionPassThru_get_Duration,
    MediaPositionPassThru_put_CurrentPosition,
    MediaPositionPassThru_get_CurrentPosition,
    MediaPositionPassThru_get_StopTime,
    MediaPositionPassThru_put_StopTime,
    MediaPositionPassThru_get_PrerollTime,
    MediaPositionPassThru_put_PrerollTime,
    MediaPositionPassThru_put_Rate,
    MediaPositionPassThru_get_Rate,
    MediaPositionPassThru_CanSeekForward,
    MediaPositionPassThru_CanSeekBackward
};

void strmbase_passthrough_init(PassThruImpl *passthrough, IUnknown *outer)
{
    memset(passthrough, 0, sizeof(*passthrough));

    passthrough->outer_unk = outer;
    passthrough->IMediaPosition_iface.lpVtbl = &IMediaPositionPassThru_Vtbl;
    passthrough->IMediaSeeking_iface.lpVtbl = &IMediaSeekingPassThru_Vtbl;
    passthrough->ISeekingPassThru_iface.lpVtbl = &ISeekingPassThru_Vtbl;
    InitializeCriticalSection(&passthrough->time_cs);
    passthrough->time_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PassThruImpl.time_cs" );
}

void strmbase_passthrough_cleanup(PassThruImpl *passthrough)
{
    passthrough->time_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&passthrough->time_cs);
}

struct seeking_passthrough
{
    PassThruImpl passthrough;

    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;
};

static struct seeking_passthrough *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct seeking_passthrough, IUnknown_inner);
}

static HRESULT WINAPI seeking_passthrough_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);

    TRACE("passthrough %p, iid %s, out %p.\n", passthrough, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IMediaPosition))
        *out = &passthrough->passthrough.IMediaPosition_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &passthrough->passthrough.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_ISeekingPassThru))
        *out = &passthrough->passthrough.ISeekingPassThru_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI seeking_passthrough_AddRef(IUnknown *iface)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&passthrough->refcount);

    TRACE("%p increasing refcount to %u.\n", passthrough, refcount);
    return refcount;
}

static ULONG WINAPI seeking_passthrough_Release(IUnknown *iface)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&passthrough->refcount);

    TRACE("%p decreasing refcount to %u.\n", passthrough, refcount);
    if (!refcount)
    {
        strmbase_passthrough_cleanup(&passthrough->passthrough);
        heap_free(passthrough);
    }
    return refcount;
}

static const IUnknownVtbl seeking_passthrough_vtbl =
{
    seeking_passthrough_QueryInterface,
    seeking_passthrough_AddRef,
    seeking_passthrough_Release,
};

HRESULT WINAPI PosPassThru_Construct(IUnknown *outer, void **out)
{
    struct seeking_passthrough *object;

    TRACE("outer %p, out %p.\n", outer, out);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IUnknown_inner.lpVtbl = &seeking_passthrough_vtbl;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;

    strmbase_passthrough_init(&object->passthrough, object->outer_unk);

    TRACE("Created seeking passthrough %p.\n", object);
    *out = &object->IUnknown_inner;
    return S_OK;
}

HRESULT WINAPI RendererPosPassThru_RegisterMediaTime(IUnknown *iface, REFERENCE_TIME start)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);

    EnterCriticalSection(&passthrough->passthrough.time_cs);
    passthrough->passthrough.time_earliest = start;
    passthrough->passthrough.timevalid = TRUE;
    LeaveCriticalSection(&passthrough->passthrough.time_cs);
    return S_OK;
}

HRESULT WINAPI RendererPosPassThru_ResetMediaTime(IUnknown *iface)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);

    EnterCriticalSection(&passthrough->passthrough.time_cs);
    passthrough->passthrough.timevalid = FALSE;
    LeaveCriticalSection(&passthrough->passthrough.time_cs);
    return S_OK;
}

HRESULT WINAPI RendererPosPassThru_EOS(IUnknown *iface)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);
    REFERENCE_TIME time;
    HRESULT hr;

    hr = IMediaSeeking_GetStopPosition(&passthrough->passthrough.IMediaSeeking_iface, &time);
    EnterCriticalSection(&passthrough->passthrough.time_cs);
    if (SUCCEEDED(hr))
    {
        passthrough->passthrough.timevalid = TRUE;
        passthrough->passthrough.time_earliest = time;
    }
    else
        passthrough->passthrough.timevalid = FALSE;
    LeaveCriticalSection(&passthrough->passthrough.time_cs);
    return hr;
}
