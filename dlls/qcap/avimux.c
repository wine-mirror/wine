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

#define MAX_PIN_NO 128

typedef struct {
    BaseOutputPin pin;
    IQualityControl IQualityControl_iface;
} AviMuxOut;

typedef struct {
    BaseInputPin pin;
    IAMStreamControl IAMStreamControl_iface;
    IMemInputPin IMemInputPin_iface;
    IPropertyBag IPropertyBag_iface;
    IQualityControl IQualityControl_iface;
} AviMuxIn;

typedef struct {
    BaseFilter filter;
    IConfigAviMux IConfigAviMux_iface;
    IConfigInterleaving IConfigInterleaving_iface;
    IMediaSeeking IMediaSeeking_iface;
    IPersistMediaPropertyBag IPersistMediaPropertyBag_iface;
    ISpecifyPropertyPages ISpecifyPropertyPages_iface;

    AviMuxOut *out;
    int input_pin_no;
    AviMuxIn *in[MAX_PIN_NO-1];
} AviMux;

static HRESULT create_input_pin(AviMux*);

static inline AviMux* impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AviMux, filter);
}

static IPin* WINAPI AviMux_GetPin(BaseFilter *iface, int pos)
{
    AviMux *This = impl_from_BaseFilter(iface);

    TRACE("(%p)->(%d)\n", This, pos);

    if(pos == 0) {
        IPin_AddRef(&This->out->pin.pin.IPin_iface);
        return &This->out->pin.pin.IPin_iface;
    }else if(pos>0 && pos<=This->input_pin_no) {
        IPin_AddRef(&This->in[pos-1]->pin.pin.IPin_iface);
        return &This->in[pos-1]->pin.pin.IPin_iface;
    }

    return NULL;
}

static LONG WINAPI AviMux_GetPinCount(BaseFilter *iface)
{
    AviMux *This = impl_from_BaseFilter(iface);
    TRACE("(%p)\n", This);
    return This->input_pin_no+1;
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
        int i;

        BaseOutputPinImpl_Release(&This->out->pin.pin.IPin_iface);

        for(i=0; i<This->input_pin_no; i++)
            BaseInputPinImpl_Release(&This->in[i]->pin.pin.IPin_iface);

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
    int i;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(Id), ppPin);

    if(!Id || !ppPin)
        return E_POINTER;

    if(!lstrcmpiW(Id, This->out->pin.pin.pinInfo.achName)) {
        IPin_AddRef(&This->out->pin.pin.IPin_iface);
        *ppPin = &This->out->pin.pin.IPin_iface;
        return S_OK;
    }

    for(i=0; i<This->input_pin_no; i++) {
        if(lstrcmpiW(Id, This->in[i]->pin.pin.pinInfo.achName))
            continue;

        IPin_AddRef(&This->in[i]->pin.pin.IPin_iface);
        *ppPin = &This->in[i]->pin.pin.IPin_iface;
        return S_OK;
    }

    return VFW_E_NOT_FOUND;
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
    PIN_DIRECTION dir;
    HRESULT hr;

    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p))\n", base, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    hr = IPin_QueryDirection(pReceivePin, &dir);
    if(hr==S_OK && dir!=PINDIR_INPUT)
        return VFW_E_INVALID_DIRECTION;

    return BaseOutputPinImpl_AttemptConnection(base, pReceivePin, pmt);
}

static LONG WINAPI AviMuxOut_GetMediaTypeVersion(BasePin *base)
{
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

static HRESULT WINAPI AviMuxOut_DecideAllocator(BaseOutputPin *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    ALLOCATOR_PROPERTIES req, actual;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", base, pPin, pAlloc);

    hr = BaseOutputPinImpl_InitAllocator(base, pAlloc);
    if(FAILED(hr))
        return hr;

    hr = IMemInputPin_GetAllocatorRequirements(pPin, &req);
    if(FAILED(hr))
        req.cbAlign = 1;
    req.cBuffers = 32;
    req.cbBuffer = 0;
    req.cbPrefix = 0;

    hr = IMemAllocator_SetProperties(*pAlloc, &req, &actual);
    if(FAILED(hr))
        return hr;

    return IMemInputPin_NotifyAllocator(pPin, *pAlloc, TRUE);
}

static HRESULT WINAPI AviMuxOut_BreakConnect(BaseOutputPin *base)
{
    FIXME("(%p)\n", base);
    return E_NOTIMPL;
}

static const BaseOutputPinFuncTable AviMuxOut_BaseOutputFuncTable = {
    NULL,
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
    HRESULT hr;
    int i;

    TRACE("(%p)->(%p AM_MEDIA_TYPE(%p))\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    hr = BaseOutputPinImpl_Connect(iface, pReceivePin, pmt);
    if(FAILED(hr))
        return hr;

    for(i=0; i<This->input_pin_no; i++) {
        if(!This->in[i]->pin.pin.pConnectedTo)
            continue;

        hr = IFilterGraph_Reconnect(This->filter.filterInfo.pGraph, &This->in[i]->pin.pin.IPin_iface);
        if(FAILED(hr)) {
            BaseOutputPinImpl_Disconnect(iface);
            break;
        }
    }

    if(hr == S_OK)
        IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
    return hr;
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
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = BaseOutputPinImpl_Disconnect(iface);
    if(hr == S_OK)
        IBaseFilter_Release(&This->filter.IBaseFilter_iface);
    return hr;
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

static HRESULT WINAPI AviMuxIn_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *pmt)
{
    FIXME("(%p:%s)->(AM_MEDIA_TYPE(%p))\n", base, debugstr_w(base->pinInfo.achName), pmt);
    dump_AM_MEDIA_TYPE(pmt);

    if(IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio) &&
            IsEqualIID(&pmt->formattype, &FORMAT_WaveFormatEx))
        return S_OK;
    if(IsEqualIID(&pmt->majortype, &MEDIATYPE_Interleaved) &&
            IsEqualIID(&pmt->formattype, &FORMAT_DvInfo))
        return S_OK;
    if(IsEqualIID(&pmt->majortype, &MEDIATYPE_Video) &&
            (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo) ||
             IsEqualIID(&pmt->formattype, &FORMAT_DvInfo)))
        return S_OK;
    return S_FALSE;
}

static LONG WINAPI AviMuxIn_GetMediaTypeVersion(BasePin *base)
{
    return 0;
}

static HRESULT WINAPI AviMuxIn_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    return S_FALSE;
}

static const BasePinFuncTable AviMuxIn_BaseFuncTable = {
    AviMuxIn_CheckMediaType,
    NULL,
    AviMuxIn_GetMediaTypeVersion,
    AviMuxIn_GetMediaType
};

static HRESULT WINAPI AviMuxIn_Receive(BaseInputPin *base, IMediaSample *pSample)
{
    FIXME("(%p:%s)->(%p)\n", base, debugstr_w(base->pin.pinInfo.achName), pSample);
    return E_NOTIMPL;
}

static const BaseInputPinFuncTable AviMuxIn_BaseInputFuncTable = {
    AviMuxIn_Receive
};

static inline AviMux* impl_from_in_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    IBaseFilter *bf = bp->pinInfo.pFilter;

    return impl_from_IBaseFilter(bf);
}

static inline AviMuxIn* AviMuxIn_from_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    BaseInputPin *bip = CONTAINING_RECORD(bp, BaseInputPin, pin);
    return CONTAINING_RECORD(bip, AviMuxIn, pin);
}

static HRESULT WINAPI AviMuxIn_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);

    TRACE("(%p:%s)->(%s %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin))
        *ppv = &avimuxin->pin.pin.IPin_iface;
    else if(IsEqualIID(riid, &IID_IAMStreamControl))
        *ppv = &avimuxin->IAMStreamControl_iface;
    else if(IsEqualIID(riid, &IID_IMemInputPin))
        *ppv = &avimuxin->IMemInputPin_iface;
    else if(IsEqualIID(riid, &IID_IPropertyBag))
        *ppv = &avimuxin->IPropertyBag_iface;
    else if(IsEqualIID(riid, &IID_IQualityControl))
        *ppv = &avimuxin->IQualityControl_iface;
    else {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AviMuxIn_AddRef(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_Release(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_Connect(IPin *iface,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p AM_MEDIA_TYPE(%p))\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BaseInputPinImpl_Connect(iface, pReceivePin, pmt);
}

static HRESULT WINAPI AviMuxIn_ReceiveConnection(IPin *iface,
        IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    HRESULT hr;

    TRACE("(%p:%s)->(%p AM_MEDIA_TYPE(%p))\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), pConnector, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    if(!pmt)
        return E_POINTER;

    hr = BaseInputPinImpl_ReceiveConnection(iface, pConnector, pmt);
    if(FAILED(hr))
        return hr;

    return create_input_pin(This);
}

static HRESULT WINAPI AviMuxIn_Disconnect(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return BasePinImpl_Disconnect(iface);
}

static HRESULT WINAPI AviMuxIn_ConnectedTo(IPin *iface, IPin **pPin)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pPin);
    return BasePinImpl_ConnectedTo(iface, pPin);
}

static HRESULT WINAPI AviMuxIn_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pmt);
    return BasePinImpl_ConnectionMediaType(iface, pmt);
}

static HRESULT WINAPI AviMuxIn_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pInfo);
    return BasePinImpl_QueryPinInfo(iface, pInfo);
}

static HRESULT WINAPI AviMuxIn_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pPinDir);
    return BasePinImpl_QueryDirection(iface, pPinDir);
}

static HRESULT WINAPI AviMuxIn_QueryId(IPin *iface, LPWSTR *Id)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), Id);
    return BasePinImpl_QueryId(iface, Id);
}

static HRESULT WINAPI AviMuxIn_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(AM_MEDIA_TYPE(%p))\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), pmt);
    dump_AM_MEDIA_TYPE(pmt);
    return BasePinImpl_QueryAccept(iface, pmt);
}

static HRESULT WINAPI AviMuxIn_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), ppEnum);
    return BasePinImpl_EnumMediaTypes(iface, ppEnum);
}

static HRESULT WINAPI AviMuxIn_QueryInternalConnections(
        IPin *iface, IPin **apPin, ULONG *nPin)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(%p %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), apPin, nPin);
    return BasePinImpl_QueryInternalConnections(iface, apPin, nPin);
}

static HRESULT WINAPI AviMuxIn_EndOfStream(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return BaseInputPinImpl_EndOfStream(iface);
}

static HRESULT WINAPI AviMuxIn_BeginFlush(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return BaseInputPinImpl_BeginFlush(iface);
}

static HRESULT WINAPI AviMuxIn_EndFlush(IPin *iface)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return BaseInputPinImpl_EndFlush(iface);
}

static HRESULT WINAPI AviMuxIn_NewSegment(IPin *iface, REFERENCE_TIME tStart,
        REFERENCE_TIME tStop, double dRate)
{
    AviMux *This = impl_from_in_IPin(iface);
    AviMuxIn *avimuxin = AviMuxIn_from_IPin(iface);
    TRACE("(%p:%s)->(0x%x%08x 0x%x%08x %lf)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
         (ULONG)(tStart >> 32), (ULONG)tStart, (ULONG)(tStop >> 32), (ULONG)tStop, dRate);
    return BasePinImpl_NewSegment(iface, tStart, tStop, dRate);
}

static const IPinVtbl AviMuxIn_PinVtbl = {
    AviMuxIn_QueryInterface,
    AviMuxIn_AddRef,
    AviMuxIn_Release,
    AviMuxIn_Connect,
    AviMuxIn_ReceiveConnection,
    AviMuxIn_Disconnect,
    AviMuxIn_ConnectedTo,
    AviMuxIn_ConnectionMediaType,
    AviMuxIn_QueryPinInfo,
    AviMuxIn_QueryDirection,
    AviMuxIn_QueryId,
    AviMuxIn_QueryAccept,
    AviMuxIn_EnumMediaTypes,
    AviMuxIn_QueryInternalConnections,
    AviMuxIn_EndOfStream,
    AviMuxIn_BeginFlush,
    AviMuxIn_EndFlush,
    AviMuxIn_NewSegment
};

static inline AviMuxIn* AviMuxIn_from_IAMStreamControl(IAMStreamControl *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, IAMStreamControl_iface);
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_QueryInterface(
        IAMStreamControl *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_AMStreamControl_AddRef(IAMStreamControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_AMStreamControl_Release(IAMStreamControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_StartAt(IAMStreamControl *iface,
        const REFERENCE_TIME *ptStart, DWORD dwCookie)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p %x)\n", This,
            debugstr_w(avimuxin->pin.pin.pinInfo.achName), ptStart, dwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_StopAt(IAMStreamControl *iface,
        const REFERENCE_TIME *ptStop, BOOL bSendExtra, DWORD dwCookie)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p %x %x)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            ptStop, bSendExtra, dwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_AMStreamControl_GetInfo(
        IAMStreamControl *iface, AM_STREAM_INFO *pInfo)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IAMStreamControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pInfo);
    return E_NOTIMPL;
}

static const IAMStreamControlVtbl AviMuxIn_AMStreamControlVtbl = {
    AviMuxIn_AMStreamControl_QueryInterface,
    AviMuxIn_AMStreamControl_AddRef,
    AviMuxIn_AMStreamControl_Release,
    AviMuxIn_AMStreamControl_StartAt,
    AviMuxIn_AMStreamControl_StopAt,
    AviMuxIn_AMStreamControl_GetInfo
};

static inline AviMuxIn* AviMuxIn_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, IMemInputPin_iface);
}

static HRESULT WINAPI AviMuxIn_MemInputPin_QueryInterface(
        IMemInputPin *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_MemInputPin_AddRef(IMemInputPin *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_MemInputPin_Release(IMemInputPin *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_MemInputPin_GetAllocator(
        IMemInputPin *iface, IMemAllocator **ppAllocator)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), ppAllocator);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_NotifyAllocator(
        IMemInputPin *iface, IMemAllocator *pAllocator, BOOL bReadOnly)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p %x)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            pAllocator, bReadOnly);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_GetAllocatorRequirements(
        IMemInputPin *iface, ALLOCATOR_PROPERTIES *pProps)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pProps);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_Receive(
        IMemInputPin *iface, IMediaSample *pSample)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pSample);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p %d %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            pSamples, nSamples, nSamplesProcessed);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_MemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IMemInputPin(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName));
    return E_NOTIMPL;
}

static const IMemInputPinVtbl AviMuxIn_MemInputPinVtbl = {
    AviMuxIn_MemInputPin_QueryInterface,
    AviMuxIn_MemInputPin_AddRef,
    AviMuxIn_MemInputPin_Release,
    AviMuxIn_MemInputPin_GetAllocator,
    AviMuxIn_MemInputPin_NotifyAllocator,
    AviMuxIn_MemInputPin_GetAllocatorRequirements,
    AviMuxIn_MemInputPin_Receive,
    AviMuxIn_MemInputPin_ReceiveMultiple,
    AviMuxIn_MemInputPin_ReceiveCanBlock
};

static inline AviMuxIn* AviMuxIn_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, IPropertyBag_iface);
}

static HRESULT WINAPI AviMuxIn_PropertyBag_QueryInterface(
        IPropertyBag *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_PropertyBag_AddRef(IPropertyBag *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_PropertyBag_Release(IPropertyBag *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_PropertyBag_Read(IPropertyBag *iface,
        LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%s %p %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            debugstr_w(pszPropName), pVar, pErrorLog);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_PropertyBag_Write(IPropertyBag *iface,
        LPCOLESTR pszPropName, VARIANT *pVar)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IPropertyBag(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%s %p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName),
            debugstr_w(pszPropName), pVar);
    return E_NOTIMPL;
}

static const IPropertyBagVtbl AviMuxIn_PropertyBagVtbl = {
    AviMuxIn_PropertyBag_QueryInterface,
    AviMuxIn_PropertyBag_AddRef,
    AviMuxIn_PropertyBag_Release,
    AviMuxIn_PropertyBag_Read,
    AviMuxIn_PropertyBag_Write
};

static inline AviMuxIn* AviMuxIn_from_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, AviMuxIn, IQualityControl_iface);
}

static HRESULT WINAPI AviMuxIn_QualityControl_QueryInterface(
        IQualityControl *iface, REFIID riid, void **ppv)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    return IPin_QueryInterface(&avimuxin->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI AviMuxIn_QualityControl_AddRef(IQualityControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI AviMuxIn_QualityControl_Release(IQualityControl *iface)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI AviMuxIn_QualityControl_Notify(IQualityControl *iface,
        IBaseFilter *pSelf, Quality q)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p Quality)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), pSelf);
    return E_NOTIMPL;
}

static HRESULT WINAPI AviMuxIn_QualityControl_SetSink(
        IQualityControl *iface, IQualityControl *piqc)
{
    AviMuxIn *avimuxin = AviMuxIn_from_IQualityControl(iface);
    AviMux *This = impl_from_in_IPin(&avimuxin->pin.pin.IPin_iface);
    FIXME("(%p:%s)->(%p)\n", This, debugstr_w(avimuxin->pin.pin.pinInfo.achName), piqc);
    return E_NOTIMPL;
}

static const IQualityControlVtbl AviMuxIn_QualityControlVtbl = {
    AviMuxIn_QualityControl_QueryInterface,
    AviMuxIn_QualityControl_AddRef,
    AviMuxIn_QualityControl_Release,
    AviMuxIn_QualityControl_Notify,
    AviMuxIn_QualityControl_SetSink
};

static HRESULT create_input_pin(AviMux *avimux)
{
    static const WCHAR name[] = {'I','n','p','u','t',' ','0','0',0};
    PIN_INFO info;
    HRESULT hr;

    if(avimux->input_pin_no >= MAX_PIN_NO-1)
        return E_FAIL;

    info.dir = PINDIR_INPUT;
    info.pFilter = &avimux->filter.IBaseFilter_iface;
    memcpy(info.achName, name, sizeof(name));
    info.achName[7] = '0' + (avimux->input_pin_no+1) % 10;
    info.achName[6] = '0' + (avimux->input_pin_no+1) / 10;

    hr = BaseInputPin_Construct(&AviMuxIn_PinVtbl, sizeof(AviMuxIn), &info,
            &AviMuxIn_BaseFuncTable, &AviMuxIn_BaseInputFuncTable,
            &avimux->filter.csFilter, NULL, (IPin**)&avimux->in[avimux->input_pin_no]);
    if(FAILED(hr))
        return hr;
    avimux->in[avimux->input_pin_no]->IAMStreamControl_iface.lpVtbl = &AviMuxIn_AMStreamControlVtbl;
    avimux->in[avimux->input_pin_no]->IMemInputPin_iface.lpVtbl = &AviMuxIn_MemInputPinVtbl;
    avimux->in[avimux->input_pin_no]->IPropertyBag_iface.lpVtbl = &AviMuxIn_PropertyBagVtbl;
    avimux->in[avimux->input_pin_no]->IQualityControl_iface.lpVtbl = &AviMuxIn_QualityControlVtbl;

    avimux->input_pin_no++;
    return S_OK;
}

IUnknown* WINAPI QCAP_createAVIMux(IUnknown *pUnkOuter, HRESULT *phr)
{
    static const WCHAR output_name[] = {'A','V','I',' ','O','u','t',0};

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
    avimux->input_pin_no = 0;

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

    hr = create_input_pin(avimux);
    if(FAILED(hr)) {
        BaseOutputPinImpl_Release(&avimux->out->pin.pin.IPin_iface);
        BaseFilterImpl_Release(&avimux->filter.IBaseFilter_iface);
        HeapFree(GetProcessHeap(), 0, avimux);
        *phr = hr;
        return NULL;
    }

    ObjectRefCount(TRUE);
    *phr = S_OK;
    return (IUnknown*)&avimux->filter.IBaseFilter_iface;
}
