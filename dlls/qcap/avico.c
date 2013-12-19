/*
 * Copyright 2013 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    BaseFilter filter;
} AVICompressor;

static inline AVICompressor *impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AVICompressor, filter);
}

static inline AVICompressor *impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static HRESULT WINAPI AVICompressor_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);

    if(IsEqualIID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    }else if(IsEqualIID(riid, &IID_IPersist)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    }else if(IsEqualIID(riid, &IID_IMediaFilter)) {
        TRACE("(%p)->(IID_IMediaFilter %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    }else if(IsEqualIID(riid, &IID_IBaseFilter)) {
        TRACE("(%p)->(IID_IBaseFilter %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    }else {
        FIXME("no interface for %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;

}

static ULONG WINAPI AVICompressor_Release(IBaseFilter *iface)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    ULONG ref = BaseFilterImpl_Release(&This->filter.IBaseFilter_iface);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI AVICompressor_Stop(IBaseFilter *iface)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AVICompressor_Pause(IBaseFilter *iface)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AVICompressor_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_longlong(tStart));
    return E_NOTIMPL;
}

static HRESULT WINAPI AVICompressor_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(Id), ppPin);
    return VFW_E_NOT_FOUND;
}

static HRESULT WINAPI AVICompressor_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p)\n", This, pInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI AVICompressor_QueryVendorInfo(IBaseFilter *iface, LPWSTR *pVendorInfo)
{
    AVICompressor *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p)\n", This, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl AVICompressorVtbl = {
    AVICompressor_QueryInterface,
    BaseFilterImpl_AddRef,
    AVICompressor_Release,
    BaseFilterImpl_GetClassID,
    AVICompressor_Stop,
    AVICompressor_Pause,
    AVICompressor_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    AVICompressor_FindPin,
    AVICompressor_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    AVICompressor_QueryVendorInfo
};

static IPin* WINAPI AVICompressor_GetPin(BaseFilter *iface, int pos)
{
    AVICompressor *This = impl_from_BaseFilter(iface);
    FIXME("(%p)->(%d)\n", This, pos);
    return NULL;
}

static LONG WINAPI AVICompressor_GetPinCount(BaseFilter *iface)
{
    AVICompressor *This = impl_from_BaseFilter(iface);
    FIXME("(%p)\n", This);
    return 0;
}

static const BaseFilterFuncTable filter_func_table = {
    AVICompressor_GetPin,
    AVICompressor_GetPinCount
};

IUnknown* WINAPI QCAP_createAVICompressor(IUnknown *outer, HRESULT *phr)
{
    AVICompressor *compressor;

    TRACE("\n");

    compressor = heap_alloc_zero(sizeof(*compressor));
    if(!compressor) {
        *phr = E_NOINTERFACE;
        return NULL;
    }

    BaseFilter_Init(&compressor->filter, &AVICompressorVtbl, &CLSID_AVICo,
            (DWORD_PTR)(__FILE__ ": AVICompressor.csFilter"), &filter_func_table);

    *phr = S_OK;
    return (IUnknown*)&compressor->filter.IBaseFilter_iface;
}
