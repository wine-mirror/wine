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

static inline BaseFilter *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, BaseFilter, IUnknown_inner);
}

static HRESULT WINAPI filter_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    BaseFilter *filter = impl_from_IUnknown(iface);
    HRESULT hr;

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    *out = NULL;

    if (filter->pFuncsTable->filter_query_interface
            && SUCCEEDED(hr = filter->pFuncsTable->filter_query_interface(filter, iid, out)))
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
    BaseFilter *filter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&filter->refcount);

    TRACE("%p increasing refcount to %u.\n", filter, refcount);

    return refcount;
}

static ULONG WINAPI filter_inner_Release(IUnknown *iface)
{
    BaseFilter *filter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&filter->refcount);

    TRACE("%p decreasing refcount to %u.\n", filter, refcount);

    if (!refcount)
        filter->pFuncsTable->filter_destroy(filter);

    return refcount;
}

static const IUnknownVtbl filter_inner_vtbl =
{
    filter_inner_QueryInterface,
    filter_inner_AddRef,
    filter_inner_Release,
};

static inline BaseFilter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
}

HRESULT WINAPI BaseFilterImpl_QueryInterface(IBaseFilter *iface, REFIID iid, void **out)
{
    BaseFilter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(filter->outer_unk, iid, out);
}

ULONG WINAPI BaseFilterImpl_AddRef(IBaseFilter *iface)
{
    BaseFilter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_AddRef(filter->outer_unk);
}

ULONG WINAPI BaseFilterImpl_Release(IBaseFilter *iface)
{
    BaseFilter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_Release(filter->outer_unk);
}

HRESULT WINAPI BaseFilterImpl_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    BaseFilter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, pClsid);

    *pClsid = This->clsid;

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_Stop(IBaseFilter *iface)
{
    BaseFilter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->csFilter);
    filter->state = State_Stopped;
    LeaveCriticalSection(&filter->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_Pause(IBaseFilter *iface)
{
    BaseFilter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->csFilter);
    filter->state = State_Paused;
    LeaveCriticalSection(&filter->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_Run(IBaseFilter *iface, REFERENCE_TIME start)
{
    BaseFilter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p, start %s.\n", iface, wine_dbgstr_longlong(start));

    EnterCriticalSection(&filter->csFilter);
    filter->state = State_Running;
    LeaveCriticalSection(&filter->csFilter);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState )
{
    BaseFilter *This = impl_from_IBaseFilter(iface);
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
    BaseFilter *This = impl_from_IBaseFilter(iface);
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
    BaseFilter *This = impl_from_IBaseFilter(iface);
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
    BaseFilter *filter = impl_from_IBaseFilter(iface);

    TRACE("iface %p, enum_pins %p.\n", iface, enum_pins);

    return enum_pins_create(filter, enum_pins);
}

HRESULT WINAPI BaseFilterImpl_FindPin(IBaseFilter *iface, const WCHAR *id, IPin **ret)
{
    BaseFilter *This = impl_from_IBaseFilter(iface);
    unsigned int i;
    PIN_INFO info;
    HRESULT hr;
    IPin *pin;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(id), ret);

    for (i = 0; (pin = This->pFuncsTable->filter_get_pin(This, i)); ++i)
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
    BaseFilter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, pInfo);

    lstrcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);

    return S_OK;
}

HRESULT WINAPI BaseFilterImpl_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName )
{
    BaseFilter *This = impl_from_IBaseFilter(iface);

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

VOID WINAPI BaseFilterImpl_IncrementPinVersion(BaseFilter *filter)
{
    InterlockedIncrement(&filter->pin_version);
}

void strmbase_filter_init(BaseFilter *filter, const IBaseFilterVtbl *vtbl, IUnknown *outer,
        const CLSID *clsid, DWORD_PTR debug_info, const BaseFilterFuncTable *func_table)
{
    memset(filter, 0, sizeof(*filter));

    filter->IBaseFilter_iface.lpVtbl = vtbl;
    filter->IUnknown_inner.lpVtbl = &filter_inner_vtbl;
    filter->outer_unk = outer ? outer : &filter->IUnknown_inner;
    filter->refcount = 1;

    InitializeCriticalSection(&filter->csFilter);
    filter->clsid = *clsid;
    filter->csFilter.DebugInfo->Spare[0] = debug_info;
    filter->pin_version = 1;
    filter->pFuncsTable = func_table;
}

void strmbase_filter_cleanup(BaseFilter *This)
{
    if (This->pClock)
        IReferenceClock_Release(This->pClock);

    This->IBaseFilter_iface.lpVtbl = NULL;
    This->csFilter.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->csFilter);
}
