/*
 * Implementation of MediaStream Filter
 *
 * Copyright 2008, 2012 Christian Costa
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
#include "amstream_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

struct enum_pins
{
    IEnumPins IEnumPins_iface;
    LONG refcount;

    IPin **pins;
    unsigned int count, index;
};

static const IEnumPinsVtbl enum_pins_vtbl;

static struct enum_pins *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, struct enum_pins, IEnumPins_iface);
}

static HRESULT WINAPI enum_pins_QueryInterface(IEnumPins *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumPins))
    {
        IEnumPins_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_pins_AddRef(IEnumPins *iface)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    ULONG refcount = InterlockedIncrement(&enum_pins->refcount);
    TRACE("%p increasing refcount to %u.\n", enum_pins, refcount);
    return refcount;
}

static ULONG WINAPI enum_pins_Release(IEnumPins *iface)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    ULONG refcount = InterlockedDecrement(&enum_pins->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %u.\n", enum_pins, refcount);
    if (!refcount)
    {
        for (i = 0; i < enum_pins->count; ++i)
            IPin_Release(enum_pins->pins[i]);
        heap_free(enum_pins->pins);
        heap_free(enum_pins);
    }
    return refcount;
}

static HRESULT WINAPI enum_pins_Next(IEnumPins *iface, ULONG count, IPin **pins, ULONG *ret_count)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    unsigned int i;

    TRACE("iface %p, count %u, pins %p, ret_count %p.\n", iface, count, pins, ret_count);

    if (!pins || (count > 1 && !ret_count))
        return E_POINTER;

    for (i = 0; i < count && enum_pins->index < enum_pins->count; ++i)
    {
        IPin_AddRef(pins[i] = enum_pins->pins[i]);
        enum_pins->index++;
    }

    if (ret_count) *ret_count = i;
    return i == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI enum_pins_Skip(IEnumPins *iface, ULONG count)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);

    TRACE("iface %p, count %u.\n", iface, count);

    enum_pins->index += count;

    return enum_pins->index >= enum_pins->count ? S_FALSE : S_OK;
}

static HRESULT WINAPI enum_pins_Reset(IEnumPins *iface)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);

    TRACE("iface %p.\n", iface);

    enum_pins->index = 0;
    return S_OK;
}

static HRESULT WINAPI enum_pins_Clone(IEnumPins *iface, IEnumPins **out)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    struct enum_pins *object;
    unsigned int i;

    TRACE("iface %p, out %p.\n", iface, out);

    if (!(object = heap_alloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumPins_iface.lpVtbl = &enum_pins_vtbl;
    object->refcount = 1;
    object->count = enum_pins->count;
    object->index = enum_pins->index;
    if (!(object->pins = heap_alloc(enum_pins->count * sizeof(*object->pins))))
    {
        heap_free(object);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < enum_pins->count; ++i)
        IPin_AddRef(object->pins[i] = enum_pins->pins[i]);

    *out = &object->IEnumPins_iface;
    return S_OK;
}

static const IEnumPinsVtbl enum_pins_vtbl =
{
    enum_pins_QueryInterface,
    enum_pins_AddRef,
    enum_pins_Release,
    enum_pins_Next,
    enum_pins_Skip,
    enum_pins_Reset,
    enum_pins_Clone,
};

struct filter
{
    IMediaStreamFilter IMediaStreamFilter_iface;
    LONG refcount;
    CRITICAL_SECTION cs;

    IReferenceClock *clock;
    WCHAR name[128];
    IFilterGraph *graph;
    ULONG nb_streams;
    IAMMediaStream **streams;
};

static inline struct filter *impl_from_IMediaStreamFilter(IMediaStreamFilter *iface)
{
    return CONTAINING_RECORD(iface, struct filter, IMediaStreamFilter_iface);
}

static HRESULT WINAPI filter_QueryInterface(IMediaStreamFilter *iface, REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IMediaFilter) ||
        IsEqualIID(riid, &IID_IBaseFilter) ||
        IsEqualIID(riid, &IID_IMediaStreamFilter))
        *ret_iface = iface;

    if (*ret_iface)
    {
        IMediaStreamFilter_AddRef(*ret_iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI filter_AddRef(IMediaStreamFilter *iface)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    ULONG refcount = InterlockedIncrement(&filter->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI filter_Release(IMediaStreamFilter *iface)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    ULONG refcount = InterlockedDecrement(&filter->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < filter->nb_streams; ++i)
        {
            IAMMediaStream_JoinFilter(filter->streams[i], NULL);
            IAMMediaStream_Release(filter->streams[i]);
        }
        heap_free(filter->streams);
        if (filter->clock)
            IReferenceClock_Release(filter->clock);
        DeleteCriticalSection(&filter->cs);
        heap_free(filter);
    }

    return refcount;
}

static HRESULT WINAPI filter_GetClassID(IMediaStreamFilter *iface, CLSID *clsid)
{
    *clsid = CLSID_MediaStreamFilter;
    return S_OK;
}

static HRESULT WINAPI filter_Stop(IMediaStreamFilter *iface)
{
    FIXME("(%p)->(): Stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_Pause(IMediaStreamFilter *iface)
{
    FIXME("(%p)->(): Stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_Run(IMediaStreamFilter *iface, REFERENCE_TIME start)
{
    FIXME("(%p)->(%s): Stub!\n", iface, wine_dbgstr_longlong(start));

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_GetState(IMediaStreamFilter *iface, DWORD timeout, FILTER_STATE *state)
{
    FIXME("iface %p, timeout %u, state %p, stub!\n", iface, timeout, state);

    *state = State_Stopped;
    return S_OK;
}

static HRESULT WINAPI filter_SetSyncSource(IMediaStreamFilter *iface, IReferenceClock *clock)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, clock %p.\n", iface, clock);

    EnterCriticalSection(&filter->cs);

    if (clock)
        IReferenceClock_AddRef(clock);
    if (filter->clock)
        IReferenceClock_Release(filter->clock);
    filter->clock = clock;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_GetSyncSource(IMediaStreamFilter *iface, IReferenceClock **clock)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, clock %p.\n", iface, clock);

    EnterCriticalSection(&filter->cs);

    if (filter->clock)
        IReferenceClock_AddRef(filter->clock);
    *clock = filter->clock;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_EnumPins(IMediaStreamFilter *iface, IEnumPins **enum_pins)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    struct enum_pins *object;
    unsigned int i;

    TRACE("iface %p, enum_pins %p.\n", iface, enum_pins);

    EnterCriticalSection(&filter->cs);

    if (!enum_pins)
        return E_POINTER;

    if (!(object = heap_alloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumPins_iface.lpVtbl = &enum_pins_vtbl;
    object->refcount = 1;
    object->count = filter->nb_streams;
    object->index = 0;
    if (!(object->pins = heap_alloc(filter->nb_streams * sizeof(*object->pins))))
    {
        heap_free(object);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < filter->nb_streams; ++i)
    {
        if (FAILED(IAMMediaStream_QueryInterface(filter->streams[i], &IID_IPin, (void **)&object->pins[i])))
            WARN("Stream %p does not support IPin.\n", filter->streams[i]);
    }

    LeaveCriticalSection(&filter->cs);

    *enum_pins = &object->IEnumPins_iface;
    return S_OK;
}

static HRESULT WINAPI filter_FindPin(IMediaStreamFilter *iface, const WCHAR *id, IPin **out)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    unsigned int i;
    WCHAR *ret_id;
    IPin *pin;

    TRACE("iface %p, id %s, out %p.\n", iface, debugstr_w(id), out);

    EnterCriticalSection(&filter->cs);

    for (i = 0; i < filter->nb_streams; ++i)
    {
        if (FAILED(IAMMediaStream_QueryInterface(filter->streams[i], &IID_IPin, (void **)&pin)))
        {
            WARN("Stream %p does not support IPin.\n", filter->streams[i]);
            continue;
        }

        if (SUCCEEDED(IPin_QueryId(pin, &ret_id)))
        {
            if (!wcscmp(id, ret_id))
            {
                CoTaskMemFree(ret_id);
                *out = pin;
                LeaveCriticalSection(&filter->cs);
                return S_OK;
            }
            CoTaskMemFree(ret_id);
        }
        IPin_Release(pin);
    }

    LeaveCriticalSection(&filter->cs);

    return VFW_E_NOT_FOUND;
}

static HRESULT WINAPI filter_QueryFilterInfo(IMediaStreamFilter *iface, FILTER_INFO *info)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, info %p.\n", iface, info);

    EnterCriticalSection(&filter->cs);

    wcscpy(info->achName, filter->name);
    if (filter->graph)
        IFilterGraph_AddRef(filter->graph);
    info->pGraph = filter->graph;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_JoinFilterGraph(IMediaStreamFilter *iface,
        IFilterGraph *graph, const WCHAR *name)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, graph %p, name.%s.\n", iface, graph, debugstr_w(name));

    EnterCriticalSection(&filter->cs);

    if (name)
        wcsncpy(filter->name, name, ARRAY_SIZE(filter->name));
    else
        filter->name[0] = 0;
    filter->graph = graph;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_QueryVendorInfo(IMediaStreamFilter *iface, LPWSTR *vendor_info)
{
    WARN("iface %p, vendor_info %p, stub!\n", iface, vendor_info);
    return E_NOTIMPL;
}

/*** IMediaStreamFilter methods ***/

static HRESULT WINAPI filter_AddMediaStream(IMediaStreamFilter *iface, IAMMediaStream *pAMMediaStream)
{
    struct filter *This = impl_from_IMediaStreamFilter(iface);
    IAMMediaStream** streams;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, pAMMediaStream);

    streams = CoTaskMemRealloc(This->streams, (This->nb_streams + 1) * sizeof(IAMMediaStream*));
    if (!streams)
        return E_OUTOFMEMORY;
    This->streams = streams;

    hr = IAMMediaStream_JoinFilter(pAMMediaStream, iface);
    if (FAILED(hr))
        return hr;

    This->streams[This->nb_streams] = pAMMediaStream;
    This->nb_streams++;

    IAMMediaStream_AddRef(pAMMediaStream);

    return S_OK;
}

static HRESULT WINAPI filter_GetMediaStream(IMediaStreamFilter *iface, REFMSPID idPurpose, IMediaStream **ppMediaStream)
{
    struct filter *This = impl_from_IMediaStreamFilter(iface);
    MSPID purpose_id;
    unsigned int i;

    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(idPurpose), ppMediaStream);

    for (i = 0; i < This->nb_streams; i++)
    {
        IAMMediaStream_GetInformation(This->streams[i], &purpose_id, NULL);
        if (IsEqualIID(&purpose_id, idPurpose))
        {
            *ppMediaStream = (IMediaStream *)This->streams[i];
            IMediaStream_AddRef(*ppMediaStream);
            return S_OK;
        }
    }

    return MS_E_NOSTREAM;
}

static HRESULT WINAPI filter_EnumMediaStreams(IMediaStreamFilter *iface, LONG Index, IMediaStream **ppMediaStream)
{
    FIXME("(%p)->(%d,%p): Stub!\n", iface, Index, ppMediaStream);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_SupportSeeking(IMediaStreamFilter *iface, BOOL bRenderer)
{
    FIXME("(%p)->(%d): Stub!\n", iface, bRenderer);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_ReferenceTimeToStreamTime(IMediaStreamFilter *iface, REFERENCE_TIME *pTime)
{
    FIXME("(%p)->(%p): Stub!\n", iface, pTime);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_GetCurrentStreamTime(IMediaStreamFilter *iface, REFERENCE_TIME *pCurrentStreamTime)
{
    FIXME("(%p)->(%p): Stub!\n", iface, pCurrentStreamTime);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_WaitUntil(IMediaStreamFilter *iface, REFERENCE_TIME WaitStreamTime)
{
    FIXME("(%p)->(%s): Stub!\n", iface, wine_dbgstr_longlong(WaitStreamTime));

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_Flush(IMediaStreamFilter *iface, BOOL bCancelEOS)
{
    FIXME("(%p)->(%d): Stub!\n", iface, bCancelEOS);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_EndOfStream(IMediaStreamFilter *iface)
{
    FIXME("(%p)->(): Stub!\n",  iface);

    return E_NOTIMPL;
}

static const IMediaStreamFilterVtbl filter_vtbl =
{
    filter_QueryInterface,
    filter_AddRef,
    filter_Release,
    filter_GetClassID,
    filter_Stop,
    filter_Pause,
    filter_Run,
    filter_GetState,
    filter_SetSyncSource,
    filter_GetSyncSource,
    filter_EnumPins,
    filter_FindPin,
    filter_QueryFilterInfo,
    filter_JoinFilterGraph,
    filter_QueryVendorInfo,
    filter_AddMediaStream,
    filter_GetMediaStream,
    filter_EnumMediaStreams,
    filter_SupportSeeking,
    filter_ReferenceTimeToStreamTime,
    filter_GetCurrentStreamTime,
    filter_WaitUntil,
    filter_Flush,
    filter_EndOfStream
};

HRESULT filter_create(IUnknown *outer, void **out)
{
    struct filter *object;

    TRACE("outer %p, out %p.\n", outer, out);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMediaStreamFilter_iface.lpVtbl = &filter_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MediaStreamFilter.cs");

    TRACE("Created media stream filter %p.\n", object);
    *out = &object->IMediaStreamFilter_iface;
    return S_OK;
}
