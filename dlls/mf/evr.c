/*
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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

#include "mf_private.h"
#include "uuids.h"
#include "evr.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum video_renderer_flags
{
    EVR_SHUT_DOWN = 0x1,
    EVR_INIT_SERVICES = 0x2, /* Currently in InitServices() call. */
    EVR_MIXER_INITED_SERVICES = 0x4,
    EVR_PRESENTER_INITED_SERVICES = 0x8,
};

struct video_renderer;

struct video_stream
{
    IMFStreamSink IMFStreamSink_iface;
    LONG refcount;
    unsigned int id;
    struct video_renderer *parent;
};

struct video_renderer
{
    IMFMediaSink IMFMediaSink_iface;
    IMFMediaSinkPreroll IMFMediaSinkPreroll_iface;
    IMFVideoRenderer IMFVideoRenderer_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFGetService IMFGetService_iface;
    IMFTopologyServiceLookup IMFTopologyServiceLookup_iface;
    IMediaEventSink IMediaEventSink_iface;
    LONG refcount;

    IMFMediaEventQueue *event_queue;
    IMFPresentationClock *clock;

    IMFTransform *mixer;
    IMFVideoPresenter *presenter;
    unsigned int flags;

    struct video_stream **streams;
    size_t stream_size;
    size_t stream_count;

    CRITICAL_SECTION cs;
};

static struct video_renderer *impl_from_IMFMediaSink(IMFMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFMediaSink_iface);
}

static struct video_renderer *impl_from_IMFMediaSinkPreroll(IMFMediaSinkPreroll *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFMediaSinkPreroll_iface);
}

static struct video_renderer *impl_from_IMFVideoRenderer(IMFVideoRenderer *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFVideoRenderer_iface);
}

static struct video_renderer *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFMediaEventGenerator_iface);
}

static struct video_renderer *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFClockStateSink_iface);
}

static struct video_renderer *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFGetService_iface);
}

static struct video_renderer *impl_from_IMFTopologyServiceLookup(IMFTopologyServiceLookup *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFTopologyServiceLookup_iface);
}

static struct video_renderer *impl_from_IMediaEventSink(IMediaEventSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMediaEventSink_iface);
}

static struct video_stream *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_stream, IMFStreamSink_iface);
}

static void video_renderer_release_services(struct video_renderer *renderer)
{
    IMFTopologyServiceLookupClient *lookup_client;

    if (renderer->flags & EVR_MIXER_INITED_SERVICES && SUCCEEDED(IMFTransform_QueryInterface(renderer->mixer,
            &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client)))
    {
        IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
        IMFTopologyServiceLookupClient_Release(lookup_client);
        renderer->flags &= ~EVR_MIXER_INITED_SERVICES;
    }

    if (renderer->flags & EVR_PRESENTER_INITED_SERVICES && SUCCEEDED(IMFVideoPresenter_QueryInterface(renderer->presenter,
            &IID_IMFTopologyServiceLookupClient, (void **)&lookup_client)))
    {
        IMFTopologyServiceLookupClient_ReleaseServicePointers(lookup_client);
        IMFTopologyServiceLookupClient_Release(lookup_client);
        renderer->flags &= ~EVR_PRESENTER_INITED_SERVICES;
    }
}

static HRESULT WINAPI video_stream_sink_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFStreamSink_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI video_stream_sink_AddRef(IMFStreamSink *iface)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_stream_sink_Release(IMFStreamSink *iface)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    if (!refcount)
    {
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI video_stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    FIXME("%p, %#x, %p.\n", iface, flags, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    FIXME("%p, %p, %p.\n", iface, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    FIXME("%p, %p, %p.\n", iface, result, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID exttype, HRESULT hr_status, const PROPVARIANT *value)
{
    FIXME("%p, %d, %s, %#x, %p.\n", iface, event_type, debugstr_guid(exttype), hr_status, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, sink);

    if (!stream->parent)
        return MF_E_STREAMSINK_REMOVED;

    if (!sink)
        return E_POINTER;

    /* FIXME: not entirely safe if sink is being shut down. */
    *sink = &stream->parent->IMFMediaSink_iface;
    IMFMediaSink_AddRef(*sink);

    return S_OK;
}

static HRESULT WINAPI video_stream_sink_GetIdentifier(IMFStreamSink *iface, DWORD *id)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, id);

    if (!stream->parent)
        return MF_E_STREAMSINK_REMOVED;

    if (!id)
        return E_INVALIDARG;

    *id = stream->id;

    return S_OK;
}

static HRESULT WINAPI video_stream_sink_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    FIXME("%p, %p.\n", iface, handler);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_stream_sink_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    FIXME("%p, %p.\n", iface, sample);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_stream_sink_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    FIXME("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_stream_sink_Flush(IMFStreamSink *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl video_stream_sink_vtbl =
{
    video_stream_sink_QueryInterface,
    video_stream_sink_AddRef,
    video_stream_sink_Release,
    video_stream_sink_GetEvent,
    video_stream_sink_BeginGetEvent,
    video_stream_sink_EndGetEvent,
    video_stream_sink_QueueEvent,
    video_stream_sink_GetMediaSink,
    video_stream_sink_GetIdentifier,
    video_stream_sink_GetMediaTypeHandler,
    video_stream_sink_ProcessSample,
    video_stream_sink_PlaceMarker,
    video_stream_sink_Flush,
};

static HRESULT video_renderer_stream_create(struct video_renderer *renderer, unsigned int id,
        struct video_stream **ret)
{
    struct video_stream *stream;

    if (!(stream = heap_alloc_zero(sizeof(*stream))))
        return E_OUTOFMEMORY;

    stream->IMFStreamSink_iface.lpVtbl = &video_stream_sink_vtbl;
    stream->refcount = 1;
    stream->parent = renderer;
    IMFMediaSink_AddRef(&stream->parent->IMFMediaSink_iface);
    stream->id = id;

    *ret = stream;

    return S_OK;
}

static HRESULT WINAPI video_renderer_sink_QueryInterface(IMFMediaSink *iface, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &renderer->IMFMediaSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaSinkPreroll))
    {
        *obj = &renderer->IMFMediaSinkPreroll_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoRenderer))
    {
        *obj = &renderer->IMFVideoRenderer_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &renderer->IMFMediaEventGenerator_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &renderer->IMFClockStateSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &renderer->IMFGetService_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI video_renderer_sink_AddRef(IMFMediaSink *iface)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&renderer->refcount);
    TRACE("%p, refcount %u.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI video_renderer_sink_Release(IMFMediaSink *iface)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&renderer->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (renderer->event_queue)
            IMFMediaEventQueue_Release(renderer->event_queue);
        if (renderer->mixer)
            IMFTransform_Release(renderer->mixer);
        if (renderer->presenter)
            IMFVideoPresenter_Release(renderer->presenter);
        if (renderer->clock)
            IMFPresentationClock_Release(renderer->clock);
        DeleteCriticalSection(&renderer->cs);
        heap_free(renderer);
    }

    return refcount;
}

static HRESULT WINAPI video_renderer_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *flags)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (renderer->flags & EVR_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_CLOCK_REQUIRED | MEDIASINK_CAN_PREROLL;

    return S_OK;
}

static HRESULT WINAPI video_renderer_sink_AddStreamSink(IMFMediaSink *iface, DWORD id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    struct video_stream *stream;
    HRESULT hr;

    TRACE("%p, %#x, %p, %p.\n", iface, id, media_type, stream_sink);

    /* Rely on mixer for stream id validation. */

    EnterCriticalSection(&renderer->cs);
    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (SUCCEEDED(hr = IMFTransform_AddInputStreams(renderer->mixer, 1, &id)))
    {
        if (mf_array_reserve((void **)&renderer->streams, &renderer->stream_size, renderer->stream_count + 1,
                sizeof(*renderer->streams)))
        {
            if (SUCCEEDED(hr = video_renderer_stream_create(renderer, id, &stream)))
            {
                *stream_sink = &stream->IMFStreamSink_iface;
                IMFStreamSink_AddRef(*stream_sink);
                renderer->streams[renderer->stream_count++] = stream;
            }
        }
        else
            hr = E_OUTOFMEMORY;

        if (FAILED(hr))
            IMFTransform_DeleteInputStream(renderer->mixer, id);

    }
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI video_renderer_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD id)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr;
    size_t i;

    TRACE("%p, %#x.\n", iface, id);

    /* Rely on mixer for stream id validation. */

    EnterCriticalSection(&renderer->cs);
    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (SUCCEEDED(hr = IMFTransform_DeleteInputStream(renderer->mixer, id)))
    {
        for (i = 0; i < renderer->stream_count; ++i)
        {
            if (renderer->streams[i]->id == id)
            {
                IMFStreamSink_Release(&renderer->streams[i]->IMFStreamSink_iface);
                renderer->streams[i] = NULL;
                if (i < renderer->stream_count - 1)
                {
                    memmove(&renderer->streams[i], &renderer->streams[i+1],
                            (renderer->stream_count - i - 1) * sizeof(*renderer->streams));
                }
                renderer->stream_count--;
                break;
            }
        }
    }
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI video_renderer_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    EnterCriticalSection(&renderer->cs);
    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!count)
        hr = E_POINTER;
    else
        *count = renderer->stream_count;
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI video_renderer_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, stream);

    EnterCriticalSection(&renderer->cs);
    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!stream)
        hr = E_POINTER;
    else if (index >= renderer->stream_count)
        hr = E_INVALIDARG;
    else
    {
        *stream = &renderer->streams[index]->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI video_renderer_sink_GetStreamSinkById(IMFMediaSink *iface, DWORD id,
        IMFStreamSink **stream)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;
    size_t i;

    TRACE("%p, %#x, %p.\n", iface, id, stream);

    EnterCriticalSection(&renderer->cs);
    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!stream)
        hr = E_POINTER;
    else
    {
        for (i = 0; i < renderer->stream_count; ++i)
        {
            if (renderer->streams[i]->id == id)
                break;
        }

        if (i == renderer->stream_count)
            hr = MF_E_INVALIDSTREAMNUMBER;
        else
        {
            *stream = &renderer->streams[i]->IMFStreamSink_iface;
            IMFStreamSink_AddRef(*stream);
        }

    }
    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static void video_renderer_set_presentation_clock(struct video_renderer *renderer, IMFPresentationClock *clock)
{
    if (renderer->clock)
    {
        IMFPresentationClock_RemoveClockStateSink(renderer->clock, &renderer->IMFClockStateSink_iface);
        IMFPresentationClock_Release(renderer->clock);
    }
    renderer->clock = clock;
    if (renderer->clock)
    {
        IMFPresentationClock_AddRef(renderer->clock);
        IMFPresentationClock_AddClockStateSink(renderer->clock, &renderer->IMFClockStateSink_iface);
    }
}

static HRESULT WINAPI video_renderer_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        video_renderer_set_presentation_clock(renderer, clock);

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI video_renderer_sink_GetPresentationClock(IMFMediaSink *iface, IMFPresentationClock **clock)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    if (!clock)
        return E_POINTER;

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (renderer->clock)
    {
        *clock = renderer->clock;
        IMFPresentationClock_AddRef(*clock);
    }
    else
        hr = MF_E_NO_CLOCK;

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static HRESULT WINAPI video_renderer_sink_Shutdown(IMFMediaSink *iface)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    size_t i;

    TRACE("%p.\n", iface);

    if (renderer->flags & EVR_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&renderer->cs);
    renderer->flags |= EVR_SHUT_DOWN;
    /* Detach streams from the sink. */
    for (i = 0; i < renderer->stream_count; ++i)
    {
        IMFMediaSink_Release(&renderer->streams[i]->parent->IMFMediaSink_iface);
        renderer->streams[i]->parent = NULL;
        IMFStreamSink_Release(&renderer->streams[i]->IMFStreamSink_iface);
        renderer->streams[i] = NULL;
    }
    heap_free(renderer->streams);
    renderer->stream_count = 0;
    renderer->stream_size = 0;
    IMFMediaEventQueue_Shutdown(renderer->event_queue);
    video_renderer_set_presentation_clock(renderer, NULL);
    video_renderer_release_services(renderer);
    LeaveCriticalSection(&renderer->cs);

    return S_OK;
}

static const IMFMediaSinkVtbl video_renderer_sink_vtbl =
{
    video_renderer_sink_QueryInterface,
    video_renderer_sink_AddRef,
    video_renderer_sink_Release,
    video_renderer_sink_GetCharacteristics,
    video_renderer_sink_AddStreamSink,
    video_renderer_sink_RemoveStreamSink,
    video_renderer_sink_GetStreamSinkCount,
    video_renderer_sink_GetStreamSinkByIndex,
    video_renderer_sink_GetStreamSinkById,
    video_renderer_sink_SetPresentationClock,
    video_renderer_sink_GetPresentationClock,
    video_renderer_sink_Shutdown,
};

static HRESULT WINAPI video_renderer_preroll_QueryInterface(IMFMediaSinkPreroll *iface, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_renderer_preroll_AddRef(IMFMediaSinkPreroll *iface)
{
    struct video_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_preroll_Release(IMFMediaSinkPreroll *iface)
{
    struct video_renderer *renderer = impl_from_IMFMediaSinkPreroll(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_preroll_NotifyPreroll(IMFMediaSinkPreroll *iface, MFTIME start_time)
{
    FIXME("%p, %s.\n", iface, debugstr_time(start_time));

    return E_NOTIMPL;
}

static const IMFMediaSinkPrerollVtbl video_renderer_preroll_vtbl =
{
    video_renderer_preroll_QueryInterface,
    video_renderer_preroll_AddRef,
    video_renderer_preroll_Release,
    video_renderer_preroll_NotifyPreroll,
};

static HRESULT WINAPI video_renderer_QueryInterface(IMFVideoRenderer *iface, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFVideoRenderer(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_renderer_AddRef(IMFVideoRenderer *iface)
{
    struct video_renderer *renderer = impl_from_IMFVideoRenderer(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_Release(IMFVideoRenderer *iface)
{
    struct video_renderer *renderer = impl_from_IMFVideoRenderer(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_InitializeRenderer(IMFVideoRenderer *iface, IMFTransform *mixer,
        IMFVideoPresenter *presenter)
{
    FIXME("%p, %p, %p.\n", iface, mixer, presenter);

    return E_NOTIMPL;
}

static const IMFVideoRendererVtbl video_renderer_vtbl =
{
    video_renderer_QueryInterface,
    video_renderer_AddRef,
    video_renderer_Release,
    video_renderer_InitializeRenderer,
};

static HRESULT WINAPI video_renderer_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_renderer_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct video_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_events_Release(IMFMediaEventGenerator *iface)
{
    struct video_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct video_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(renderer->event_queue, flags, event);
}

static HRESULT WINAPI video_renderer_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct video_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(renderer->event_queue, callback, state);
}

static HRESULT WINAPI video_renderer_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct video_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(renderer->event_queue, result, event);
}

static HRESULT WINAPI video_renderer_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct video_renderer *renderer = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(renderer->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl video_renderer_events_vtbl =
{
    video_renderer_events_QueryInterface,
    video_renderer_events_AddRef,
    video_renderer_events_Release,
    video_renderer_events_GetEvent,
    video_renderer_events_BeginGetEvent,
    video_renderer_events_EndGetEvent,
    video_renderer_events_QueueEvent,
};

static HRESULT WINAPI video_renderer_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_renderer_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct video_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_clock_sink_Release(IMFClockStateSink *iface)
{
    struct video_renderer *renderer = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    FIXME("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_renderer_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_renderer_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_renderer_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    FIXME("%p, %s.\n", iface, debugstr_time(systime));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_renderer_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl video_renderer_clock_sink_vtbl =
{
    video_renderer_clock_sink_QueryInterface,
    video_renderer_clock_sink_AddRef,
    video_renderer_clock_sink_Release,
    video_renderer_clock_sink_OnClockStart,
    video_renderer_clock_sink_OnClockStop,
    video_renderer_clock_sink_OnClockPause,
    video_renderer_clock_sink_OnClockRestart,
    video_renderer_clock_sink_OnClockSetRate,
};

static HRESULT WINAPI video_renderer_get_service_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFGetService(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_renderer_get_service_AddRef(IMFGetService *iface)
{
    struct video_renderer *renderer = impl_from_IMFGetService(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_get_service_Release(IMFGetService *iface)
{
    struct video_renderer *renderer = impl_from_IMFGetService(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFGetService(iface);
    HRESULT hr = E_NOINTERFACE;
    IMFGetService *gs = NULL;

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(service, &MR_VIDEO_MIXER_SERVICE))
    {
        hr = IMFTransform_QueryInterface(renderer->mixer, &IID_IMFGetService, (void **)&gs);
    }
    else if (IsEqualGUID(service, &MR_VIDEO_RENDER_SERVICE))
    {
        hr = IMFVideoPresenter_QueryInterface(renderer->presenter, &IID_IMFGetService, (void **)&gs);
    }
    else
    {
        FIXME("Unsupported service %s.\n", debugstr_guid(service));
    }

    if (gs)
    {
        hr = IMFGetService_GetService(gs, service, riid, obj);
        IMFGetService_Release(gs);
    }

    return hr;
}

static const IMFGetServiceVtbl video_renderer_get_service_vtbl =
{
    video_renderer_get_service_QueryInterface,
    video_renderer_get_service_AddRef,
    video_renderer_get_service_Release,
    video_renderer_get_service_GetService,
};

static HRESULT WINAPI video_renderer_service_lookup_QueryInterface(IMFTopologyServiceLookup *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFTopologyServiceLookup) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFTopologyServiceLookup_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI video_renderer_service_lookup_AddRef(IMFTopologyServiceLookup *iface)
{
    struct video_renderer *renderer = impl_from_IMFTopologyServiceLookup(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_service_lookup_Release(IMFTopologyServiceLookup *iface)
{
    struct video_renderer *renderer = impl_from_IMFTopologyServiceLookup(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_service_lookup_LookupService(IMFTopologyServiceLookup *iface,
        MF_SERVICE_LOOKUP_TYPE lookup_type, DWORD index, REFGUID service, REFIID riid,
        void **objects, DWORD *num_objects)
{
    struct video_renderer *renderer = impl_from_IMFTopologyServiceLookup(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %u, %s, %s, %p, %p.\n", iface, lookup_type, index, debugstr_guid(service), debugstr_guid(riid),
            objects, num_objects);

    EnterCriticalSection(&renderer->cs);

    if (!(renderer->flags & EVR_INIT_SERVICES))
        hr = MF_E_NOTACCEPTING;
    else if (IsEqualGUID(service, &MR_VIDEO_RENDER_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMediaEventSink))
        {
            *objects = &renderer->IMediaEventSink_iface;
            IUnknown_AddRef((IUnknown *)*objects);
        }
        else
        {
            FIXME("Unsupported interface %s for render service.\n", debugstr_guid(riid));
            hr = E_NOINTERFACE;
        }
    }
    else if (IsEqualGUID(service, &MR_VIDEO_MIXER_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFTransform))
        {
            *objects = renderer->mixer;
            IUnknown_AddRef((IUnknown *)*objects);
        }
        else
        {
            FIXME("Unsupported interface %s for mixer service.\n", debugstr_guid(riid));
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        WARN("Unsupported service %s.\n", debugstr_guid(service));
        hr = MF_E_UNSUPPORTED_SERVICE;
    }

    LeaveCriticalSection(&renderer->cs);

    return hr;
}

static const IMFTopologyServiceLookupVtbl video_renderer_service_lookup_vtbl =
{
    video_renderer_service_lookup_QueryInterface,
    video_renderer_service_lookup_AddRef,
    video_renderer_service_lookup_Release,
    video_renderer_service_lookup_LookupService,
};

static HRESULT WINAPI video_renderer_event_sink_QueryInterface(IMediaEventSink *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMediaEventSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMediaEventSink_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI video_renderer_event_sink_AddRef(IMediaEventSink *iface)
{
    struct video_renderer *renderer = impl_from_IMediaEventSink(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_event_sink_Release(IMediaEventSink *iface)
{
    struct video_renderer *renderer = impl_from_IMediaEventSink(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_event_sink_Notify(IMediaEventSink *iface, LONG event, LONG_PTR param1, LONG_PTR param2)
{
    FIXME("%p, %d, %ld, %ld.\n", iface, event, param1, param2);

    return E_NOTIMPL;
}

static const IMediaEventSinkVtbl media_event_sink_vtbl =
{
    video_renderer_event_sink_QueryInterface,
    video_renderer_event_sink_AddRef,
    video_renderer_event_sink_Release,
    video_renderer_event_sink_Notify,
};

static HRESULT video_renderer_create_mixer(struct video_renderer *renderer, IMFAttributes *attributes,
        IMFTransform **out)
{
    IMFTopologyServiceLookupClient *lookup_client;
    unsigned int flags = 0;
    IMFActivate *activate;
    CLSID clsid;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFAttributes_GetUnknown(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_MIXER_ACTIVATE,
            &IID_IMFActivate, (void **)&activate)))
    {
        IMFAttributes_GetUINT32(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_MIXER_FLAGS, &flags);
        hr = IMFActivate_ActivateObject(activate, &IID_IMFTransform, (void **)out);
        IMFActivate_Release(activate);
        if (FAILED(hr) && !(flags & MF_ACTIVATE_CUSTOM_MIXER_ALLOWFAIL))
            return hr;
    }

    /* Activation object failed, use class activation. */
    if (FAILED(hr))
    {
        if (FAILED(IMFAttributes_GetGUID(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_MIXER_CLSID, &clsid)))
            memcpy(&clsid, &CLSID_MFVideoMixer9, sizeof(clsid));
        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)out);
    }

    if (FAILED(hr))
    {
        WARN("Failed to create a mixer object, hr %#x.\n", hr);
        return hr;
    }

    if (SUCCEEDED(hr = IMFTransform_QueryInterface(*out, &IID_IMFTopologyServiceLookupClient,
            (void **)&lookup_client)))
    {
        renderer->flags |= EVR_INIT_SERVICES;
        if (SUCCEEDED(hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client,
                &renderer->IMFTopologyServiceLookup_iface)))
        {
            renderer->flags |= EVR_MIXER_INITED_SERVICES;
        }
        renderer->flags &= ~EVR_INIT_SERVICES;
        IMFTopologyServiceLookupClient_Release(lookup_client);
    }

    return hr;
}

static HRESULT video_renderer_create_presenter(struct video_renderer *renderer, IMFAttributes *attributes,
        IMFVideoPresenter **out)
{
    IMFTopologyServiceLookupClient *lookup_client;
    unsigned int flags = 0;
    IMFActivate *activate;
    CLSID clsid;
    HRESULT hr;

    if (SUCCEEDED(IMFAttributes_GetUnknown(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_ACTIVATE,
            &IID_IMFActivate, (void **)&activate)))
    {
        IMFAttributes_GetUINT32(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_FLAGS, &flags);
        hr = IMFActivate_ActivateObject(activate, &IID_IMFVideoPresenter, (void **)out);
        IMFActivate_Release(activate);
        if (FAILED(hr) && !(flags & MF_ACTIVATE_CUSTOM_PRESENTER_ALLOWFAIL))
            return hr;
    }

    if (FAILED(IMFAttributes_GetGUID(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_CLSID, &clsid)))
        memcpy(&clsid, &CLSID_MFVideoPresenter9, sizeof(clsid));

    if (SUCCEEDED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFVideoPresenter, (void **)out)))
    {
        if (SUCCEEDED(hr = IMFVideoPresenter_QueryInterface(*out, &IID_IMFTopologyServiceLookupClient,
                (void **)&lookup_client)))
        {
            renderer->flags |= EVR_INIT_SERVICES;
            if (SUCCEEDED(hr = IMFTopologyServiceLookupClient_InitServicePointers(lookup_client,
                    &renderer->IMFTopologyServiceLookup_iface)))
            {
                renderer->flags |= EVR_PRESENTER_INITED_SERVICES;
            }
            renderer->flags &= ~EVR_INIT_SERVICES;
            IMFTopologyServiceLookupClient_Release(lookup_client);
        }
    }

    return hr;
}

static HRESULT evr_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct video_renderer *object;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", attributes, user_context, obj);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaSink_iface.lpVtbl = &video_renderer_sink_vtbl;
    object->IMFMediaSinkPreroll_iface.lpVtbl = &video_renderer_preroll_vtbl;
    object->IMFVideoRenderer_iface.lpVtbl = &video_renderer_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &video_renderer_events_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &video_renderer_clock_sink_vtbl;
    object->IMFGetService_iface.lpVtbl = &video_renderer_get_service_vtbl;
    object->IMFTopologyServiceLookup_iface.lpVtbl = &video_renderer_service_lookup_vtbl;
    object->IMediaEventSink_iface.lpVtbl = &media_event_sink_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    /* Create mixer and presenter. */
    if (FAILED(hr = video_renderer_create_mixer(object, attributes, &object->mixer)))
        goto failed;

    if (FAILED(hr = video_renderer_create_presenter(object, attributes, &object->presenter)))
        goto failed;

    *obj = (IUnknown *)&object->IMFMediaSink_iface;

    return S_OK;

failed:

    video_renderer_release_services(object);
    IMFMediaSink_Release(&object->IMFMediaSink_iface);

    return hr;
}

static void evr_shutdown_object(void *user_context, IUnknown *obj)
{
    IMFMediaSink *sink;

    if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IMFMediaSink, (void **)&sink)))
    {
        IMFMediaSink_Shutdown(sink);
        IMFMediaSink_Release(sink);
    }
}

static const struct activate_funcs evr_activate_funcs =
{
    .create_object = evr_create_object,
    .shutdown_object = evr_shutdown_object,
};

/***********************************************************************
 *      MFCreateVideoRendererActivate (mf.@)
 */
HRESULT WINAPI MFCreateVideoRendererActivate(HWND hwnd, IMFActivate **activate)
{
    HRESULT hr;

    TRACE("%p, %p.\n", hwnd, activate);

    if (!activate)
        return E_POINTER;

    hr = create_activation_object(NULL, &evr_activate_funcs, activate);
    if (SUCCEEDED(hr))
        IMFActivate_SetUINT64(*activate, &MF_ACTIVATE_VIDEO_WINDOW, (ULONG_PTR)hwnd);

    return hr;
}
