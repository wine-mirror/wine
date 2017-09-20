/*
 * Copyright 2017 Fabian Maurer
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

#define COBJMACROS

#include "config.h"
#include "wine/debug.h"

#include <stdio.h>

#include "evr_private.h"
#include "d3d9.h"
#include "wine/strmbase.h"

#include "initguid.h"
#include "dxva2api.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

typedef struct
{
    BaseFilter filter;

    IUnknown IUnknown_inner;
} evr_filter;

static inline evr_filter *impl_from_inner_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, evr_filter, IUnknown_inner);
}

static HRESULT WINAPI inner_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    evr_filter *This = impl_from_inner_IUnknown(iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppv);

    *ppv = NULL;

     if (IsEqualIID(riid, &IID_IUnknown))
         *ppv = &This->IUnknown_inner;

     else if (IsEqualIID(riid, &IID_IAMCertifiedOutputProtection))
         FIXME("No interface for IID_IAMCertifiedOutputProtection\n");
     else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
         FIXME("No interface for IID_IAMFilterMiscFlags\n");
     else if (IsEqualIID(riid, &IID_IBaseFilter))
         *ppv =&This->filter.IBaseFilter_iface;
     else if (IsEqualIID(riid, &IID_IMediaFilter))
         *ppv =&This->filter.IBaseFilter_iface;
     else if (IsEqualIID(riid, &IID_IPersist))
         *ppv =&This->filter.IBaseFilter_iface;
     else if (IsEqualIID(riid, &IID_IKsPropertySet))
         FIXME("No interface for IID_IKsPropertySet\n");
     else if (IsEqualIID(riid, &IID_IMediaEventSink))
         FIXME("No interface for IID_IMediaEventSink\n");
     else if (IsEqualIID(riid, &IID_IMediaSeeking))
         FIXME("No interface for IID_IMediaSeeking\n");
     else if (IsEqualIID(riid, &IID_IQualityControl))
         FIXME("No interface for IID_IQualityControl\n");
     else if (IsEqualIID(riid, &IID_IQualProp))
         FIXME("No interface for IID_IQualProp\n");

     else if (IsEqualIID(riid, &IID_IEVRFilterConfig))
         FIXME("No interface for IID_IEVRFilterConfig\n");
     else if (IsEqualIID(riid, &IID_IMFGetService))
         FIXME("No interface for IID_IMFGetService\n");
     else if (IsEqualIID(riid, &IID_IMFVideoPositionMapper))
         FIXME("No interface for IID_IMFVideoPositionMapper\n");
     else if (IsEqualIID(riid, &IID_IMFVideoRenderer))
         FIXME("No interface for IID_IMFVideoRenderer\n");

     else if (IsEqualIID(riid, &IID_IMemInputPin))
         FIXME("No interface for IID_IMemInputPin\n");
     else if (IsEqualIID(riid, &IID_IPin))
         FIXME("No interface for IID_IPin\n");
     else if (IsEqualIID(riid, &IID_IQualityControl))
         FIXME("No interface for IID_IQualityControl\n");

     else if (IsEqualIID(riid, &IID_IDirectXVideoMemoryConfiguration))
         FIXME("No interface for IID_IDirectXVideoMemoryConfiguration\n");

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI inner_AddRef(IUnknown *iface)
{
    evr_filter *This = impl_from_inner_IUnknown(iface);
    ULONG ref = BaseFilterImpl_AddRef(&This->filter.IBaseFilter_iface);

    TRACE("(%p, %p)->(): new ref %d\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI inner_Release(IUnknown *iface)
{
    evr_filter *This = impl_from_inner_IUnknown(iface);
    ULONG ref = BaseFilterImpl_Release(&This->filter.IBaseFilter_iface);

    TRACE("(%p, %p)->(): new ref %d\n", iface, This, ref);

    if (!ref)
        CoTaskMemFree(This);

    return ref;
}

static const IUnknownVtbl evr_inner_vtbl =
{
    inner_QueryInterface,
    inner_AddRef,
    inner_Release
};

static inline evr_filter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, evr_filter, filter);
}

static HRESULT WINAPI filter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    evr_filter *This = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(&This->IUnknown_inner, riid, ppv);
}

static ULONG WINAPI filter_AddRef(IBaseFilter *iface)
{
    evr_filter *This = impl_from_IBaseFilter(iface);
    LONG ret;

    ret = IUnknown_AddRef(&This->IUnknown_inner);

    TRACE("(%p)->AddRef from %d\n", iface, ret - 1);

    return ret;
}

static ULONG WINAPI filter_Release(IBaseFilter *iface)
{
    evr_filter *This = impl_from_IBaseFilter(iface);
    LONG ret;

    ret = IUnknown_Release(&This->IUnknown_inner);

    TRACE("(%p)->Release from %d\n", iface, ret + 1);

    return ret;
}

static const IBaseFilterVtbl basefilter_vtbl =
{
    filter_QueryInterface,
    filter_AddRef,
    filter_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    BaseRendererImpl_Pause,
    BaseRendererImpl_Run,
    BaseRendererImpl_GetState,
    BaseRendererImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseRendererImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static IPin* WINAPI filter_GetPin(BaseFilter *iface, int position)
{
    FIXME("(%p, %d): stub!\n", iface, position);
    return NULL;
}

static LONG WINAPI filter_GetPinCount(BaseFilter *iface)
{
    FIXME("(%p): stub!\n", iface);
    return 0;
}

static const BaseFilterFuncTable basefilter_functable =
{
    filter_GetPin,
    filter_GetPinCount,
};

HRESULT evr_filter_create(IUnknown *outer_unk, void **ppv)
{
    evr_filter *object;

    TRACE("(%p, %p)\n", outer_unk, ppv);

    *ppv = NULL;

    if(outer_unk != NULL)
    {
        FIXME("Aggregation yet unsupported!\n");
        return E_NOINTERFACE;
    }

    object = CoTaskMemAlloc(sizeof(evr_filter));
    if (!object)
        return E_OUTOFMEMORY;

    BaseFilter_Init(&object->filter, &basefilter_vtbl, &CLSID_EnhancedVideoRenderer,
                    (DWORD_PTR)(__FILE__ ": EVR.csFilter"), &basefilter_functable);

    object->IUnknown_inner.lpVtbl = &evr_inner_vtbl;

    *ppv = &object->IUnknown_inner;

    return S_OK;
}
