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

static const WCHAR output_name[] = {'A','V','I',' ','O','u','t',0};

typedef struct {
    BaseOutputPin pin;
    IQualityControl IQualityControl_iface;
} AviMuxOut;

typedef struct {
    BaseFilter filter;
    IConfigAviMux IConfigAviMux_iface;
    IConfigInterleaving IConfigInterleaving_iface;
    IMediaSeeking IMediaSeeking_iface;
    IPersistMediaPropertyBag IPersistMediaPropertyBag_iface;
    ISpecifyPropertyPages ISpecifyPropertyPages_iface;

    AviMuxOut *out;
} AviMux;

static inline AviMux* impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AviMux, filter);
}

static IPin* WINAPI AviMux_GetPin(BaseFilter *iface, int pos)
{
    AviMux *This = impl_from_BaseFilter(iface);
    FIXME("(%p)->(%d)\n", This, pos);

    if(pos == 0) {
        IPin_AddRef(&This->out->pin.pin.IPin_iface);
        return &This->out->pin.pin.IPin_iface;
    }

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
        if(This->out->pin.pin.pConnectedTo) {
            IPin_Disconnect(This->out->pin.pin.pConnectedTo);
            IPin_Disconnect(&This->out->pin.pin.IPin_iface);
        }
        BaseOutputPinImpl_Release(&This->out->pin.pin.IPin_iface);

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
    TRACE("(%p)->(%p)\n", This, ppEnum);
    return BaseFilterImpl_EnumPins(iface, ppEnum);
}

static HRESULT WINAPI AviMux_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    AviMux *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(Id), ppPin);

    if(!lstrcmpiW(Id, output_name)) {
        IPin_AddRef(&This->out->pin.pin.IPin_iface);
        *ppPin = &This->out->pin.pin.IPin_iface;
        return S_OK;
    }

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

static HRESULT WINAPI AviMuxOut_AttemptConnection(BasePin *base,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p))\n", base, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BaseOutputPinImpl_AttemptConnection(base, pReceivePin, pmt);
}

static LONG WINAPI AviMuxOut_GetMediaTypeVersion(BasePin *base)
{
    FIXME("(%p)\n", base);
    return 0;
}

static HRESULT WINAPI AviMuxOut_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    TRACE("(%p)->(%d %p)\n", base, iPosition, amt);

    if(iPosition < 0)
        return E_INVALIDARG;
    if(iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

    amt->majortype = MEDIATYPE_Stream;
    amt->subtype = MEDIASUBTYPE_Avi;
    amt->bFixedSizeSamples = TRUE;
    amt->bTemporalCompression = FALSE;
    amt->lSampleSize = 1;
    amt->formattype = GUID_NULL;
    amt->pUnk = NULL;
    amt->cbFormat = 0;
    amt->pbFormat = NULL;
    return S_OK;
}

static const BasePinFuncTable AviMuxOut_BaseFuncTable = {
    NULL,
    AviMuxOut_AttemptConnection,
    AviMuxOut_GetMediaTypeVersion,
    AviMuxOut_GetMediaType
};

static HRESULT WINAPI AviMuxOut_DecideBufferSize(BaseOutputPin *base,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    FIXME("(%p)->(%p %p)\n", base, pAlloc, ppropInputRequest);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxOut_DecideAllocator(BaseOutputPin *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    FIXME("(%p)->(%p %p)\n", base, pPin, pAlloc);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxOut_BreakConnect(BaseOutputPin *base)
{
    FIXME("(%p)\n", base);
    return E_NOTIMPL;
}

static const BaseOutputPinFuncTable AviMuxOut_BaseOutputFuncTable = {
    AviMuxOut_DecideBufferSize,
    AviMuxOut_DecideAllocator,
    AviMuxOut_BreakConnect
};

static inline AviMux* impl_from_out_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    IBaseFilter *bf = bp->pinInfo.pFilter;

    return impl_from_IBaseFilter(bf);
}

static HRESULT WINAPI AviMuxOut_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_out_IPin(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IQualityControl))
        *ppv = &This->out->IQualityControl_iface;
    else {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AviMuxOut_AddRef(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxOut_Release(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxOut_Connect(IPin *iface,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p))\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BaseOutputPinImpl_Connect(iface, pReceivePin, pmt);
}

static HRESULT WINAPI AviMuxOut_ReceiveConnection(IPin *iface,
        IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p)\n", This, pConnector, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BaseOutputPinImpl_ReceiveConnection(iface, pConnector, pmt);
}

static HRESULT WINAPI AviMuxOut_Disconnect(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)\n", This);
    return BaseOutputPinImpl_Disconnect(iface);
}

static HRESULT WINAPI AviMuxOut_ConnectedTo(IPin *iface, IPin **pPin)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pPin);
    return BasePinImpl_ConnectedTo(iface, pPin);
}

static HRESULT WINAPI AviMuxOut_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pmt);
    return BasePinImpl_ConnectionMediaType(iface, pmt);
}

static HRESULT WINAPI AviMuxOut_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pInfo);
    return BasePinImpl_QueryPinInfo(iface, pInfo);
}

static HRESULT WINAPI AviMuxOut_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, pPinDir);
    return BasePinImpl_QueryDirection(iface, pPinDir);
}

static HRESULT WINAPI AviMuxOut_QueryId(IPin *iface, LPWSTR *Id)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, Id);
    return BasePinImpl_QueryId(iface, Id);
}

static HRESULT WINAPI AviMuxOut_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(AM_MEDIA_TYPE(%p))\n", This, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BasePinImpl_QueryAccept(iface, pmt);
}

static HRESULT WINAPI AviMuxOut_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(%p)\n", This, ppEnum);
    return BasePinImpl_EnumMediaTypes(iface, ppEnum);
}

static HRESULT WINAPI AviMuxOut_QueryInternalConnections(
        IPin *iface, IPin **apPin, ULONG *nPin)
{
    AviMux *This = impl_from_out_IPin(iface);
    FIXME("(%p)->(%p %p)\n", This, apPin, nPin);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxOut_EndOfStream(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)\n", This);
    return BaseOutputPinImpl_EndOfStream(iface);
}

static HRESULT WINAPI AviMuxOut_BeginFlush(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)\n", This);
    return BaseOutputPinImpl_BeginFlush(iface);
}

static HRESULT WINAPI AviMuxOut_EndFlush(IPin *iface)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)\n", This);
    return BaseOutputPinImpl_EndFlush(iface);
}

static HRESULT WINAPI AviMuxOut_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    AviMux *This = impl_from_out_IPin(iface);
    TRACE("(%p)->(0x%x%08x 0x%x%08x %lf)\n", This, (ULONG)(tStart >> 32),
            (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);
    return BasePinImpl_NewSegment(iface, tStart, tStop, dRate);
}

static const IPinVtbl AviMuxOut_PinVtbl = {
    AviMuxOut_QueryInterface,
    AviMuxOut_AddRef,
    AviMuxOut_Release,
    AviMuxOut_Connect,
    AviMuxOut_ReceiveConnection,
    AviMuxOut_Disconnect,
    AviMuxOut_ConnectedTo,
    AviMuxOut_ConnectionMediaType,
    AviMuxOut_QueryPinInfo,
    AviMuxOut_QueryDirection,
    AviMuxOut_QueryId,
    AviMuxOut_QueryAccept,
    AviMuxOut_EnumMediaTypes,
    AviMuxOut_QueryInternalConnections,
    AviMuxOut_EndOfStream,
    AviMuxOut_BeginFlush,
    AviMuxOut_EndFlush,
    AviMuxOut_NewSegment
};

static inline AviMux* impl_from_out_IQualityControl(IQualityControl *iface)
{
    AviMuxOut *amo = CONTAINING_RECORD(iface, AviMuxOut, IQualityControl_iface);
    return impl_from_IBaseFilter(amo->pin.pin.pinInfo.pFilter);
}

static HRESULT WINAPI AviMuxOut_QualityControl_QueryInterface(
        IQualityControl *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    return IPin_QueryInterface(&This->out->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxOut_QualityControl_AddRef(IQualityControl *iface)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxOut_QualityControl_Release(IQualityControl *iface)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxOut_QualityControl_Notify(IQualityControl *iface,
        IBaseFilter *pSelf, Quality q)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    FIXME("(%p)->(%p Quality)\n", This, pSelf);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxOut_QualityControl_SetSink(
        IQualityControl *iface, IQualityControl *piqc)
{
    AviMux *This = impl_from_out_IQualityControl(iface);
    FIXME("(%p)->(%p)\n", This, piqc);
    return E_NOTIMPL;
}

static const IQualityControlVtbl AviMuxOut_QualityControlVtbl = {
    AviMuxOut_QualityControl_QueryInterface,
    AviMuxOut_QualityControl_AddRef,
    AviMuxOut_QualityControl_Release,
    AviMuxOut_QualityControl_Notify,
    AviMuxOut_QualityControl_SetSink
};

IUnknown* WINAPI QCAP_createAVIMux(IUnknown *pUnkOuter, HRESULT *phr)
{
    AviMux *avimux;
    PIN_INFO info;
    HRESULT hr;

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

    info.dir = PINDIR_OUTPUT;
    info.pFilter = &avimux->filter.IBaseFilter_iface;
    lstrcpyW(info.achName, output_name);
    hr = BaseOutputPin_Construct(&AviMuxOut_PinVtbl, sizeof(AviMuxOut), &info,
            &AviMuxOut_BaseFuncTable, &AviMuxOut_BaseOutputFuncTable,
            &avimux->filter.csFilter, (IPin**)&avimux->out);
    if(FAILED(hr)) {
        BaseFilterImpl_Release(&avimux->filter.IBaseFilter_iface);
        HeapFree(GetProcessHeap(), 0, avimux);
        *phr = hr;
        return NULL;
    }
    avimux->out->IQualityControl_iface.lpVtbl = &AviMuxOut_QualityControlVtbl;

    ObjectRefCount(TRUE);
    *phr = S_OK;
    return (IUnknown*)&avimux->filter.IBaseFilter_iface;
}
