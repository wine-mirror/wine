/*
 * Copyright (C) 2013 Piotr Caban for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    BaseFilter filter;
    IConfigAviMux IConfigAviMux_iface;
    IConfigInterleaving IConfigInterleaving_iface;
    IMediaSeeking IMediaSeeking_iface;
    IPersistMediaPropertyBag IPersistMediaPropertyBag_iface;
    ISpecifyPropertyPages ISpecifyPropertyPages_iface;
} AviMux;

static inline AviMux* impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AviMux, filter);
}

static IPin* WINAPI AviMux_GetPin(BaseFilter *iface, int pos)
{
    AviMux *This = impl_from_BaseFilter(iface);
    FIXME("(%p)->(%d)\n", This, pos);
    return NULL;
}

static LONG WINAPI AviMux_GetPinCount(BaseFilter *iface)
{
    AviMux *This = impl_from_BaseFilter(iface);
    FIXME("(%p)\n", This);
    return 1;
}

static const BaseFilterFuncTable filter_func_table = {
    AviMux_GetPin,
    AviMux_GetPinCount
};

static inline AviMux* impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static HRESULT WINAPI AviMux_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IBaseFilter(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPersist) ||
            IsEqualIID(riid, &IID_IMediaFilter) || IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = &This->filter.IBaseFilter_iface;
    else if(IsEqualIID(riid, &IID_IConfigAviMux))
        *ppv = &This->IConfigAviMux_iface;
    else if(IsEqualIID(riid, &IID_IConfigInterleaving))
        *ppv = &This->IConfigInterleaving_iface;
    else if(IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &This->IMediaSeeking_iface;
    else if(IsEqualIID(riid, &IID_IPersistMediaPropertyBag))
        *ppv = &This->IPersistMediaPropertyBag_iface;
    else if(IsEqualIID(riid, &IID_ISpecifyPropertyPages))
        *ppv = &This->ISpecifyPropertyPages_iface;
    else {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AviMux_Release(IBaseFilter *iface)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    ULONG ref = BaseFilterImpl_Release(iface);

    TRACE("(%p) new refcount: %u\n", This, ref);

    if(!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        ObjectRefCount(FALSE);
    }
    return ref;
}

static HRESULT WINAPI AviMux_Stop(IBaseFilter *iface)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_Pause(IBaseFilter *iface)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(0x%x%08x)\n", This, (ULONG)(tStart >> 32), (ULONG)tStart);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_EnumPins(IBaseFilter *iface, IEnumPins **ppEnum)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(Id), ppPin);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p)\n", This, pInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMux_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p)\n", This, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl AviMuxVtbl = {
    AviMux_QueryInterface,
    BaseFilterImpl_AddRef,
    AviMux_Release,
    BaseFilterImpl_GetClassID,
    AviMux_Stop,
    AviMux_Pause,
    AviMux_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    AviMux_EnumPins,
    AviMux_FindPin,
    AviMux_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    AviMux_QueryVendorInfo
};

static inline AviMux* impl_from_IConfigAviMux(IConfigAviMux *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IConfigAviMux_iface);
}

static HRESULT WINAPI ConfigAviMux_QueryInterface(
        IConfigAviMux *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI ConfigAviMux_AddRef(IConfigAviMux *iface)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI ConfigAviMux_Release(IConfigAviMux *iface)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI ConfigAviMux_SetMasterStream(IConfigAviMux *iface, LONG iStream)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%d)\n", This, iStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigAviMux_GetMasterStream(IConfigAviMux *iface, LONG *pStream)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%p)\n", This, pStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigAviMux_SetOutputCompatibilityIndex(
        IConfigAviMux *iface, BOOL fOldIndex)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%x)\n", This, fOldIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigAviMux_GetOutputCompatibilityIndex(
        IConfigAviMux *iface, BOOL *pfOldIndex)
{
    AviMux *This = impl_from_IConfigAviMux(iface);
    FIXME("(%p)->(%p)\n", This, pfOldIndex);
    return E_NOTIMPL;
}

static const IConfigAviMuxVtbl ConfigAviMuxVtbl = {
    ConfigAviMux_QueryInterface,
    ConfigAviMux_AddRef,
    ConfigAviMux_Release,
    ConfigAviMux_SetMasterStream,
    ConfigAviMux_GetMasterStream,
    ConfigAviMux_SetOutputCompatibilityIndex,
    ConfigAviMux_GetOutputCompatibilityIndex
};

static inline AviMux* impl_from_IConfigInterleaving(IConfigInterleaving *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IConfigInterleaving_iface);
}

static HRESULT WINAPI ConfigInterleaving_QueryInterface(
        IConfigInterleaving *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI ConfigInterleaving_AddRef(IConfigInterleaving *iface)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI ConfigInterleaving_Release(IConfigInterleaving *iface)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI ConfigInterleaving_put_Mode(
        IConfigInterleaving *iface, InterleavingMode mode)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    FIXME("(%p)->(%d)\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigInterleaving_get_Mode(
        IConfigInterleaving *iface, InterleavingMode *pMode)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    FIXME("(%p)->(%p)\n", This, pMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigInterleaving_put_Interleaving(IConfigInterleaving *iface,
        const REFERENCE_TIME *prtInterleave, const REFERENCE_TIME *prtPreroll)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    FIXME("(%p)->(%p %p)\n", This, prtInterleave, prtPreroll);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigInterleaving_get_Interleaving(IConfigInterleaving *iface,
        REFERENCE_TIME *prtInterleave, REFERENCE_TIME *prtPreroll)
{
    AviMux *This = impl_from_IConfigInterleaving(iface);
    FIXME("(%p)->(%p %p)\n", This, prtInterleave, prtPreroll);
    return E_NOTIMPL;
}

static const IConfigInterleavingVtbl ConfigInterleavingVtbl = {
    ConfigInterleaving_QueryInterface,
    ConfigInterleaving_AddRef,
    ConfigInterleaving_Release,
    ConfigInterleaving_put_Mode,
    ConfigInterleaving_get_Mode,
    ConfigInterleaving_put_Interleaving,
    ConfigInterleaving_get_Interleaving
};

static inline AviMux* impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IMediaSeeking_iface);
}

static HRESULT WINAPI MediaSeeking_QueryInterface(
        IMediaSeeking *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI MediaSeeking_AddRef(IMediaSeeking *iface)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI MediaSeeking_Release(IMediaSeeking *iface)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI MediaSeeking_GetCapabilities(
        IMediaSeeking *iface, DWORD *pCapabilities)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pCapabilities);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_CheckCapabilities(
        IMediaSeeking *iface, DWORD *pCapabilities)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pCapabilities);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_IsFormatSupported(
        IMediaSeeking *iface, const GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_guid(pFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_QueryPreferredFormat(
        IMediaSeeking *iface, GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetTimeFormat(
        IMediaSeeking *iface, GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_IsUsingTimeFormat(
        IMediaSeeking *iface, const GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_guid(pFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetTimeFormat(
        IMediaSeeking *iface, const GUID *pFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, debugstr_guid(pFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetDuration(
        IMediaSeeking *iface, LONGLONG *pDuration)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pDuration);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetStopPosition(
        IMediaSeeking *iface, LONGLONG *pStop)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pStop);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetCurrentPosition(
        IMediaSeeking *iface, LONGLONG *pCurrent)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pCurrent);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *pTarget,
        const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %s %s %s)\n", This, pTarget, debugstr_guid(pTargetFormat),
            wine_dbgstr_longlong(Source), debugstr_guid(pSourceFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetPositions(IMediaSeeking *iface, LONGLONG *pCurrent,
        DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %x %p %x)\n", This, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetPositions(IMediaSeeking *iface,
        LONGLONG *pCurrent, LONGLONG *pStop)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %p)\n", This, pCurrent, pStop);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetAvailable(IMediaSeeking *iface,
        LONGLONG *pEarliest, LONGLONG *pLatest)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p %p)\n", This, pEarliest, pLatest);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_SetRate(IMediaSeeking *iface, double dRate)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%lf)\n", This, dRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetRate(IMediaSeeking *iface, double *pdRate)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pdRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeeking_GetPreroll(IMediaSeeking *iface, LONGLONG *pllPreroll)
{
    AviMux *This = impl_from_IMediaSeeking(iface);
    FIXME("(%p)->(%p)\n", This, pllPreroll);
    return E_NOTIMPL;
}

static const IMediaSeekingVtbl MediaSeekingVtbl = {
    MediaSeeking_QueryInterface,
    MediaSeeking_AddRef,
    MediaSeeking_Release,
    MediaSeeking_GetCapabilities,
    MediaSeeking_CheckCapabilities,
    MediaSeeking_IsFormatSupported,
    MediaSeeking_QueryPreferredFormat,
    MediaSeeking_GetTimeFormat,
    MediaSeeking_IsUsingTimeFormat,
    MediaSeeking_SetTimeFormat,
    MediaSeeking_GetDuration,
    MediaSeeking_GetStopPosition,
    MediaSeeking_GetCurrentPosition,
    MediaSeeking_ConvertTimeFormat,
    MediaSeeking_SetPositions,
    MediaSeeking_GetPositions,
    MediaSeeking_GetAvailable,
    MediaSeeking_SetRate,
    MediaSeeking_GetRate,
    MediaSeeking_GetPreroll
};

static inline AviMux* impl_from_IPersistMediaPropertyBag(IPersistMediaPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AviMux, IPersistMediaPropertyBag_iface);
}

static HRESULT WINAPI PersistMediaPropertyBag_QueryInterface(
        IPersistMediaPropertyBag *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI PersistMediaPropertyBag_AddRef(IPersistMediaPropertyBag *iface)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI PersistMediaPropertyBag_Release(IPersistMediaPropertyBag *iface)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI PersistMediaPropertyBag_GetClassID(
        IPersistMediaPropertyBag *iface, CLSID *pClassID)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI PersistMediaPropertyBag_InitNew(IPersistMediaPropertyBag *iface)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMediaPropertyBag_Load(IPersistMediaPropertyBag *iface,
        IMediaPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMediaPropertyBag_Save(IPersistMediaPropertyBag *iface,
        IMediaPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    AviMux *This = impl_from_IPersistMediaPropertyBag(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IPersistMediaPropertyBagVtbl PersistMediaPropertyBagVtbl = {
    PersistMediaPropertyBag_QueryInterface,
    PersistMediaPropertyBag_AddRef,
    PersistMediaPropertyBag_Release,
    PersistMediaPropertyBag_GetClassID,
    PersistMediaPropertyBag_InitNew,
    PersistMediaPropertyBag_Load,
    PersistMediaPropertyBag_Save
};

static inline AviMux* impl_from_ISpecifyPropertyPages(ISpecifyPropertyPages *iface)
{
    return CONTAINING_RECORD(iface, AviMux, ISpecifyPropertyPages_iface);
}

static HRESULT WINAPI SpecifyPropertyPages_QueryInterface(
        ISpecifyPropertyPages *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI SpecifyPropertyPages_AddRef(ISpecifyPropertyPages *iface)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI SpecifyPropertyPages_Release(ISpecifyPropertyPages *iface)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI SpecifyPropertyPages_GetPages(
        ISpecifyPropertyPages *iface, CAUUID *pPages)
{
    AviMux *This = impl_from_ISpecifyPropertyPages(iface);
    FIXME("(%p)->(%p)\n", This, pPages);
    return E_NOTIMPL;
}

static const ISpecifyPropertyPagesVtbl SpecifyPropertyPagesVtbl = {
    SpecifyPropertyPages_QueryInterface,
    SpecifyPropertyPages_AddRef,
    SpecifyPropertyPages_Release,
    SpecifyPropertyPages_GetPages
};

IUnknown* WINAPI QCAP_createAVIMux(IUnknown *pUnkOuter, HRESULT *phr)
{
    AviMux *avimux;

    TRACE("(%p)\n", pUnkOuter);

    if(pUnkOuter) {
        *phr = CLASS_E_NOAGGREGATION;
        return NULL;
    }

    avimux = HeapAlloc(GetProcessHeap(), 0, sizeof(AviMux));
    if(!avimux) {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }

    BaseFilter_Init(&avimux->filter, &AviMuxVtbl, &CLSID_AviDest,
            (DWORD_PTR)(__FILE__ ": AviMux.csFilter"), &filter_func_table);
    avimux->IConfigAviMux_iface.lpVtbl = &ConfigAviMuxVtbl;
    avimux->IConfigInterleaving_iface.lpVtbl = &ConfigInterleavingVtbl;
    avimux->IMediaSeeking_iface.lpVtbl = &MediaSeekingVtbl;
    avimux->IPersistMediaPropertyBag_iface.lpVtbl = &PersistMediaPropertyBagVtbl;
    avimux->ISpecifyPropertyPages_iface.lpVtbl = &SpecifyPropertyPagesVtbl;

    ObjectRefCount(TRUE);
    *phr = S_OK;
    return (IUnknown*)&avimux->filter.IBaseFilter_iface;
}
