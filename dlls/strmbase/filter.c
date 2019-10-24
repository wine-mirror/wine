/*
 * Generic Implementation of IBaseFilter Interface
 *
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static inline struct strmbase_filter *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_filter, IUnknown_inner);
}

static HRESULT WINAPI filter_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct strmbase_filter *filter = impl_from_IUnknown(iface);
    HRESULT hr;

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    *out = NULL;

    if (filter->ops->filter_query_interface
            && SUCCEEDED(hr = filter->ops->filter_query_interface(filter, iid, out)))
    {
        return hr;
    }

    if (IsEqualIID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualIID(iid, &IID_IPersist)
            || IsEqualIID(iid, &IID_IMediaFilter)
            || IsEqualIID(iid, &IID_IBaseFilter))
    {
        *out = &filter->IBaseFilter_iface;
    }
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI filter_inner_AddRef(IUnknown *iface)
{
    struct strmbase_filter *filter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&filter->refcount);

    TRACE("%p increasing refcount to %u.\n", filter, refcount);

    return refcount;
}

static ULONG WINAPI filter_inner_Release(IUnknown *iface)
{
    struct strmbase_filter *filter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&filter->refcount);

    TRACE("%p decreasing refcount to %u.\n", filter, refcount);

    if (!refcount)
        filter->ops->filter_destroy(filter);

    return refcount;
}

static const IUnknownVtbl filter_inner_vtbl =
{
    filter_inner_QueryInterface,
    filter_inner_AddRef,
    filter_inner_Release,
};

static inline struct strmbase_filter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_filter, IBaseFilter_iface);
}

HRESULT WINAPI BaseFilterImpl_QueryInterface(IBaseFilter *iface, REFIID iid, void **out)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(filter->outer_unk, iid, out);
}

ULONG WINAPI BaseFilterImpl_AddRef(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_AddRef(filter->outer_unk);
}

ULONG WINAPI BaseFilterImpl_Release(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_Release(filter->outer_unk);
}

HRESULT WINAPI BaseFilterImpl_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    struct strmbase_filter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, pClsid);

    *pClsid = This->clsid;

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_Stop(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->csFilter);
    filter->state = State_Stopped;
    LeaveCriticalSection(&filter->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_Pause(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->csFilter);
    filter->state = State_Paused;
    LeaveCriticalSection(&filter->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_Run(IBaseFilter *iface, REFERENCE_TIME start)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p, start %s.\n", iface, debugstr_time(start));

    EnterCriticalSection(&filter->csFilter);
    filter->state = State_Running;
    LeaveCriticalSection(&filter->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState )
{
    struct strmbase_filter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%d, %p)\n", This, dwMilliSecsTimeout, pState);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    struct strmbase_filter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, pClock);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        This->pClock = pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    struct strmbase_filter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, ppClock);

    EnterCriticalSection(&This->csFilter);
    {
        *ppClock = This->pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_EnumPins(IBaseFilter *iface, IEnumPins **enum_pins)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p, enum_pins %p.\n", iface, enum_pins);

    return enum_pins_create(filter, enum_pins);
}

HRESULT WINAPI BaseFilterImpl_FindPin(IBaseFilter *iface, const WCHAR *id, IPin **ret)
{
    struct strmbase_filter *This = impl_from_IBaseFilter(iface);
    unsigned int i;
    PIN_INFO info;
    HRESULT hr;
    IPin *pin;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(id), ret);

    for (i = 0; (pin = This->ops->filter_get_pin(This, i)); ++i)
    {
        hr = IPin_QueryPinInfo(pin, &info);
        if (FAILED(hr))
            return hr;

        if (info.pFilter) IBaseFilter_Release(info.pFilter);

        if (!lstrcmpW(id, info.achName))
        {
            IPin_AddRef(*ret = pin);
            return S_OK;
        }
    }

    return VFW_E_NOT_FOUND;
}

HRESULT WINAPI BaseFilterImpl_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    struct strmbase_filter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, pInfo);

    lstrcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName )
{
    struct strmbase_filter *This = impl_from_IBaseFilter(iface);

    TRACE("(%p)->(%p, %s)\n", This, pGraph, debugstr_w(pName));

    EnterCriticalSection(&This->csFilter);
    {
        if (pName)
            lstrcpyW(This->filterInfo.achName, pName);
        else
            *This->filterInfo.achName = '\0';
        This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    TRACE("(%p)->(%p)\n", iface, pVendorInfo);
    return E_NOTIMPL;
}

VOID WINAPI BaseFilterImpl_IncrementPinVersion(struct strmbase_filter *filter)
{
    InterlockedIncrement(&filter->pin_version);
}

void strmbase_filter_init(struct strmbase_filter *filter, const IBaseFilterVtbl *vtbl, IUnknown *outer,
        const CLSID *clsid, const struct strmbase_filter_ops *ops)
{
    memset(filter, 0, sizeof(*filter));

    filter->IBaseFilter_iface.lpVtbl = vtbl;
    filter->IUnknown_inner.lpVtbl = &filter_inner_vtbl;
    filter->outer_unk = outer ? outer : &filter->IUnknown_inner;
    filter->refcount = 1;

    InitializeCriticalSection(&filter->csFilter);
    if (filter->csFilter.DebugInfo != (RTL_CRITICAL_SECTION_DEBUG *)-1)
        filter->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": strmbase_filter.csFilter");
    filter->clsid = *clsid;
    filter->pin_version = 1;
    filter->ops = ops;
}

void strmbase_filter_cleanup(struct strmbase_filter *This)
{
    if (This->pClock)
        IReferenceClock_Release(This->pClock);

    This->IBaseFilter_iface.lpVtbl = NULL;
    DeleteCriticalSection(&This->csFilter);
}
