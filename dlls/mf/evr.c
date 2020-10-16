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
#include "d3d9.h"
#include "initguid.h"
#include "dxva2api.h"

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
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    IMFGetService IMFGetService_iface;
    LONG refcount;
    unsigned int id;
    struct video_renderer *parent;
    IMFMediaEventQueue *event_queue;
    IMFVideoSampleAllocator *allocator;
    CRITICAL_SECTION cs;
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
    IMFAttributes IMFAttributes_iface;
    LONG refcount;

    IMFMediaEventQueue *event_queue;
    IMFAttributes *attributes;
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

static struct video_renderer *impl_from_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IMFAttributes_iface);
}

static struct video_stream *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_stream, IMFStreamSink_iface);
}

static struct video_stream *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct video_stream, IMFMediaTypeHandler_iface);
}

static struct video_stream *impl_from_stream_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct video_stream, IMFGetService_iface);
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
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaTypeHandler))
    {
        *obj = &stream->IMFMediaTypeHandler_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &stream->IMFGetService_iface;
    }

    if (*obj)
    {
        IUnknown_AddRef((IUnknown *)*obj);
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
        if (stream->event_queue)
            IMFMediaEventQueue_Release(stream->event_queue);
        if (stream->allocator)
            IMFVideoSampleAllocator_Release(stream->allocator);
        DeleteCriticalSection(&stream->cs);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI video_stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(stream->event_queue, flags, event);
}

static HRESULT WINAPI video_stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(stream->event_queue, callback, state);
}

static HRESULT WINAPI video_stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(stream->event_queue, result, event);
}

static HRESULT WINAPI video_stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %d, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI video_stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct video_stream *stream = impl_from_IMFStreamSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, sink);

    EnterCriticalSection(&stream->cs);
    if (!stream->parent)
        hr = MF_E_STREAMSINK_REMOVED;
    else if (!sink)
        hr = E_POINTER;
    else
    {
        *sink = &stream->parent->IMFMediaSink_iface;
        IMFMediaSink_AddRef(*sink);
    }
    LeaveCriticalSection(&stream->cs);

    return hr;
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
    struct video_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    if (!handler)
        return E_POINTER;

    if (!stream->parent)
        return MF_E_STREAMSINK_REMOVED;

    *handler = &stream->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
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

static HRESULT WINAPI video_stream_typehandler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid,
        void **obj)
{
    struct video_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&stream->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI video_stream_typehandler_AddRef(IMFMediaTypeHandler *iface)
{
    struct video_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&stream->IMFStreamSink_iface);
}

static ULONG WINAPI video_stream_typehandler_Release(IMFMediaTypeHandler *iface)
{
    struct video_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&stream->IMFStreamSink_iface);
}

static HRESULT WINAPI video_stream_typehandler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    struct video_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, in_type, out_type);

    if (!in_type)
        return E_POINTER;

    if (!stream->parent)
        return MF_E_INVALIDMEDIATYPE;

    if (SUCCEEDED(hr = IMFTransform_SetInputType(stream->parent->mixer, stream->id, in_type,
            MFT_SET_TYPE_TEST_ONLY)))
    {
        if (out_type) *out_type = NULL;
    }

    return hr;
}

static HRESULT WINAPI video_stream_typehandler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = 0;

    return S_OK;
}

static HRESULT WINAPI video_stream_typehandler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    TRACE("%p, %u, %p.\n", iface, index, type);

    return MF_E_NO_MORE_TYPES;
}

static HRESULT WINAPI video_stream_typehandler_SetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType *type)
{
    struct video_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    if (!stream->parent)
        return MF_E_STREAMSINK_REMOVED;

    hr = IMFTransform_SetInputType(stream->parent->mixer, stream->id, type, 0);
    if (SUCCEEDED(hr) && !stream->id)
        hr = IMFVideoPresenter_ProcessMessage(stream->parent->presenter, MFVP_MESSAGE_INVALIDATEMEDIATYPE, 0);

    return hr;
}

static HRESULT WINAPI video_stream_typehandler_GetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType **type)
{
    struct video_stream *stream = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    if (!stream->parent)
        return MF_E_STREAMSINK_REMOVED;

    return IMFTransform_GetInputCurrentType(stream->parent->mixer, stream->id, type);
}

static HRESULT WINAPI video_stream_typehandler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct video_stream *stream = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, type);

    if (!stream->parent)
        return MF_E_STREAMSINK_REMOVED;

    if (!type)
        return E_POINTER;

    memcpy(type, &MFMediaType_Video, sizeof(*type));
    return S_OK;
}

static const IMFMediaTypeHandlerVtbl video_stream_type_handler_vtbl =
{
    video_stream_typehandler_QueryInterface,
    video_stream_typehandler_AddRef,
    video_stream_typehandler_Release,
    video_stream_typehandler_IsMediaTypeSupported,
    video_stream_typehandler_GetMediaTypeCount,
    video_stream_typehandler_GetMediaTypeByIndex,
    video_stream_typehandler_SetCurrentMediaType,
    video_stream_typehandler_GetCurrentMediaType,
    video_stream_typehandler_GetMajorType,
};

static HRESULT WINAPI video_stream_get_service_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct video_stream *stream = impl_from_stream_IMFGetService(iface);
    return IMFStreamSink_QueryInterface(&stream->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI video_stream_get_service_AddRef(IMFGetService *iface)
{
    struct video_stream *stream = impl_from_stream_IMFGetService(iface);
    return IMFStreamSink_AddRef(&stream->IMFStreamSink_iface);
}

static ULONG WINAPI video_stream_get_service_Release(IMFGetService *iface)
{
    struct video_stream *stream = impl_from_stream_IMFGetService(iface);
    return IMFStreamSink_Release(&stream->IMFStreamSink_iface);
}

static HRESULT WINAPI video_stream_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct video_stream *stream = impl_from_stream_IMFGetService(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(service, &MR_VIDEO_ACCELERATION_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFVideoSampleAllocator))
            return IMFVideoSampleAllocator_QueryInterface(stream->allocator, riid, obj);
        return E_NOINTERFACE;
    }

    FIXME("Unsupported service %s.\n", debugstr_guid(service));

    return E_NOTIMPL;
}

static const IMFGetServiceVtbl video_stream_get_service_vtbl =
{
    video_stream_get_service_QueryInterface,
    video_stream_get_service_AddRef,
    video_stream_get_service_Release,
    video_stream_get_service_GetService,
};

static HRESULT video_renderer_stream_create(struct video_renderer *renderer, unsigned int id,
        struct video_stream **ret)
{
    struct video_stream *stream;
    HRESULT hr;

    if (!(stream = heap_alloc_zero(sizeof(*stream))))
        return E_OUTOFMEMORY;

    stream->IMFStreamSink_iface.lpVtbl = &video_stream_sink_vtbl;
    stream->IMFMediaTypeHandler_iface.lpVtbl = &video_stream_type_handler_vtbl;
    stream->IMFGetService_iface.lpVtbl = &video_stream_get_service_vtbl;
    stream->refcount = 1;
    InitializeCriticalSection(&stream->cs);

    if (FAILED(hr = MFCreateEventQueue(&stream->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateVideoSampleAllocator(&IID_IMFVideoSampleAllocator, (void **)&stream->allocator)))
        goto failed;

    stream->parent = renderer;
    IMFMediaSink_AddRef(&stream->parent->IMFMediaSink_iface);
    stream->id = id;

    *ret = stream;

    return S_OK;

failed:

    IMFStreamSink_Release(&stream->IMFStreamSink_iface);

    return hr;
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
    else if (IsEqualIID(riid, &IID_IMFAttributes))
    {
        *obj = &renderer->IMFAttributes_iface;
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
        if (renderer->attributes)
            IMFAttributes_Release(renderer->attributes);
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

static HRESULT video_renderer_add_stream(struct video_renderer *renderer, unsigned int id,
        IMFStreamSink **stream_sink)
{
    struct video_stream *stream;
    HRESULT hr;

    if (!mf_array_reserve((void **)&renderer->streams, &renderer->stream_size, renderer->stream_count + 1,
            sizeof(*renderer->streams)))
    {
        return E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr = video_renderer_stream_create(renderer, id, &stream)))
    {
        if (stream_sink)
        {
            *stream_sink = &stream->IMFStreamSink_iface;
            IMFStreamSink_AddRef(*stream_sink);
        }
        renderer->streams[renderer->stream_count++] = stream;
    }

    return hr;
}

static HRESULT WINAPI video_renderer_sink_AddStreamSink(IMFMediaSink *iface, DWORD id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct video_renderer *renderer = impl_from_IMFMediaSink(iface);
    HRESULT hr;

    TRACE("%p, %#x, %p, %p.\n", iface, id, media_type, stream_sink);

    /* Rely on mixer for stream id validation. */

    EnterCriticalSection(&renderer->cs);
    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (SUCCEEDED(hr = IMFTransform_AddInputStreams(renderer->mixer, 1, &id)))
    {
        if (FAILED(hr = video_renderer_add_stream(renderer, id, stream_sink)))
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
        struct video_stream *stream = renderer->streams[i];

        EnterCriticalSection(&stream->cs);
        stream->parent = NULL;
        LeaveCriticalSection(&stream->cs);

        IMFMediaEventQueue_Shutdown(stream->event_queue);
        IMFStreamSink_Release(&stream->IMFStreamSink_iface);
        IMFMediaSink_Release(iface);
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

static HRESULT video_renderer_create_mixer(IMFAttributes *attributes, IMFTransform **out)
{
    unsigned int flags = 0;
    IMFActivate *activate;
    CLSID clsid;
    HRESULT hr;

    if (attributes && SUCCEEDED(hr = IMFAttributes_GetUnknown(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_MIXER_ACTIVATE,
            &IID_IMFActivate, (void **)&activate)))
    {
        IMFAttributes_GetUINT32(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_MIXER_FLAGS, &flags);
        hr = IMFActivate_ActivateObject(activate, &IID_IMFTransform, (void **)out);
        IMFActivate_Release(activate);
        if (SUCCEEDED(hr) || !(flags & MF_ACTIVATE_CUSTOM_MIXER_ALLOWFAIL))
            return hr;
    }

    if (!attributes || FAILED(IMFAttributes_GetGUID(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_MIXER_CLSID, &clsid)))
        memcpy(&clsid, &CLSID_MFVideoMixer9, sizeof(clsid));

    return CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)out);
}

static HRESULT video_renderer_create_presenter(IMFAttributes *attributes, IMFVideoPresenter **out)
{
    unsigned int flags = 0;
    IMFActivate *activate;
    CLSID clsid;
    HRESULT hr;

    if (attributes && SUCCEEDED(IMFAttributes_GetUnknown(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_ACTIVATE,
            &IID_IMFActivate, (void **)&activate)))
    {
        IMFAttributes_GetUINT32(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_FLAGS, &flags);
        hr = IMFActivate_ActivateObject(activate, &IID_IMFVideoPresenter, (void **)out);
        IMFActivate_Release(activate);
        if (SUCCEEDED(hr) || !(flags & MF_ACTIVATE_CUSTOM_PRESENTER_ALLOWFAIL))
            return hr;
    }

    if (!attributes || FAILED(IMFAttributes_GetGUID(attributes, &MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_CLSID, &clsid)))
        memcpy(&clsid, &CLSID_MFVideoPresenter9, sizeof(clsid));

    return CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFVideoPresenter, (void **)out);
}

static HRESULT video_renderer_configure_mixer(struct video_renderer *renderer)
{
    IMFTopologyServiceLookupClient *lookup_client;
    IMFAttributes *attributes;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFTransform_QueryInterface(renderer->mixer, &IID_IMFTopologyServiceLookupClient,
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

    if (SUCCEEDED(hr))
    {
        unsigned int input_count, output_count;
        unsigned int *ids, *oids;
        size_t i;

        /* Create stream sinks for inputs that mixer already has by default. */
        if (SUCCEEDED(IMFTransform_GetStreamCount(renderer->mixer, &input_count, &output_count)))
        {
            ids = heap_calloc(input_count, sizeof(*ids));
            oids = heap_calloc(output_count, sizeof(*oids));

            if (ids && oids)
            {
                if (SUCCEEDED(IMFTransform_GetStreamIDs(renderer->mixer, input_count, ids, output_count, oids)))
                {
                    for (i = 0; i < input_count; ++i)
                    {
                        video_renderer_add_stream(renderer, ids[i], NULL);
                    }
                }

            }

            heap_free(ids);
            heap_free(oids);
        }
    }

    /* Set device manager that presenter should have created. */
    if (SUCCEEDED(IMFTransform_QueryInterface(renderer->mixer, &IID_IMFAttributes, (void **)&attributes)))
    {
        IDirect3DDeviceManager9 *device_manager;
        unsigned int value;

        if (SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &value)) && value)
        {
            if (SUCCEEDED(MFGetService((IUnknown *)renderer->presenter, &MR_VIDEO_ACCELERATION_SERVICE,
                    &IID_IDirect3DDeviceManager9, (void **)&device_manager)))
            {
                IMFTransform_ProcessMessage(renderer->mixer, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)device_manager);
                IDirect3DDeviceManager9_Release(device_manager);
            }
        }
        IMFAttributes_Release(attributes);
    }

    return hr;
}

static HRESULT video_renderer_configure_presenter(struct video_renderer *renderer)
{
    IMFTopologyServiceLookupClient *lookup_client;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFVideoPresenter_QueryInterface(renderer->presenter, &IID_IMFTopologyServiceLookupClient,
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

    return hr;
}

static HRESULT video_renderer_initialize(struct video_renderer *renderer, IMFTransform *mixer,
        IMFVideoPresenter *presenter)
{
    HRESULT hr;

    if (renderer->mixer)
    {
        IMFTransform_Release(renderer->mixer);
        renderer->mixer = NULL;
    }

    if (renderer->presenter)
    {
        IMFVideoPresenter_Release(renderer->presenter);
        renderer->presenter = NULL;
    }

    renderer->mixer = mixer;
    IMFTransform_AddRef(renderer->mixer);

    renderer->presenter = presenter;
    IMFVideoPresenter_AddRef(renderer->presenter);

    if (SUCCEEDED(hr = video_renderer_configure_mixer(renderer)))
        hr = video_renderer_configure_presenter(renderer);

    return hr;
}

static HRESULT WINAPI video_renderer_InitializeRenderer(IMFVideoRenderer *iface, IMFTransform *mixer,
        IMFVideoPresenter *presenter)
{
    struct video_renderer *renderer = impl_from_IMFVideoRenderer(iface);
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, mixer, presenter);

    if (mixer)
        IMFTransform_AddRef(mixer);
    else if (FAILED(hr = video_renderer_create_mixer(NULL, &mixer)))
    {
        WARN("Failed to create default mixer object, hr %#x.\n", hr);
        return hr;
    }

    if (presenter)
        IMFVideoPresenter_AddRef(presenter);
    else if (FAILED(hr = video_renderer_create_presenter(NULL, &presenter)))
    {
        WARN("Failed to create default presenter, hr %#x.\n", hr);
        IMFTransform_Release(mixer);
        return hr;
    }

    EnterCriticalSection(&renderer->cs);

    if (renderer->flags & EVR_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        /* FIXME: check clock state */
        /* FIXME: check that streams are not initialized */

        hr = video_renderer_initialize(renderer, mixer, presenter);
    }

    LeaveCriticalSection(&renderer->cs);

    IMFTransform_Release(mixer);
    IMFVideoPresenter_Release(presenter);

    return hr;
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

static HRESULT WINAPI video_renderer_attributes_QueryInterface(IMFAttributes *iface, REFIID riid, void **obj)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);
    return IMFMediaSink_QueryInterface(&renderer->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_renderer_attributes_AddRef(IMFAttributes *iface)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);
    return IMFMediaSink_AddRef(&renderer->IMFMediaSink_iface);
}

static ULONG WINAPI video_renderer_attributes_Release(IMFAttributes *iface)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);
    return IMFMediaSink_Release(&renderer->IMFMediaSink_iface);
}

static HRESULT WINAPI video_renderer_attributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(renderer->attributes, key, type);
}

static HRESULT WINAPI video_renderer_attributes_CompareItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value,
        BOOL *result)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(renderer->attributes, key, value, result);
}

static HRESULT WINAPI video_renderer_attributes_Compare(IMFAttributes *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return IMFAttributes_Compare(renderer->attributes, theirs, type, result);
}

static HRESULT WINAPI video_renderer_attributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_GetStringLength(IMFAttributes *iface, REFGUID key,
        UINT32 *length)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(renderer->attributes, key, length);
}

static HRESULT WINAPI video_renderer_attributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(renderer->attributes, key, value, size, length);
}

static HRESULT WINAPI video_renderer_attributes_GetAllocatedString(IMFAttributes *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(renderer->attributes, key, value, length);
}

static HRESULT WINAPI video_renderer_attributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(renderer->attributes, key, size);
}

static HRESULT WINAPI video_renderer_attributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
         UINT32 bufsize, UINT32 *blobsize)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(renderer->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI video_renderer_attributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key,
        UINT8 **buf, UINT32 *size)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(renderer->attributes, key, buf, size);
}

static HRESULT WINAPI video_renderer_attributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **out)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), out);

    return IMFAttributes_GetUnknown(renderer->attributes, key, riid, out);
}

static HRESULT WINAPI video_renderer_attributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s.\n", iface, debugstr_guid(key));

    return IMFAttributes_DeleteItem(renderer->attributes, key);
}

static HRESULT WINAPI video_renderer_attributes_DeleteAllItems(IMFAttributes *iface)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(renderer->attributes);
}

static HRESULT WINAPI video_renderer_attributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_SetString(IMFAttributes *iface, REFGUID key,
        const WCHAR *value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(renderer->attributes, key, value);
}

static HRESULT WINAPI video_renderer_attributes_SetBlob(IMFAttributes *iface, REFGUID key,
        const UINT8 *buf, UINT32 size)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p, %u.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(renderer->attributes, key, buf, size);
}

static HRESULT WINAPI video_renderer_attributes_SetUnknown(IMFAttributes *iface, REFGUID key,
        IUnknown *unknown)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(renderer->attributes, key, unknown);
}

static HRESULT WINAPI video_renderer_attributes_LockStore(IMFAttributes *iface)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(renderer->attributes);
}

static HRESULT WINAPI video_renderer_attributes_UnlockStore(IMFAttributes *iface)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(renderer->attributes);
}

static HRESULT WINAPI video_renderer_attributes_GetCount(IMFAttributes *iface, UINT32 *count)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(renderer->attributes, count);
}

static HRESULT WINAPI video_renderer_attributes_GetItemByIndex(IMFAttributes *iface, UINT32 index,
        GUID *key, PROPVARIANT *value)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(renderer->attributes, index, key, value);
}

static HRESULT WINAPI video_renderer_attributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    struct video_renderer *renderer = impl_from_IMFAttributes(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(renderer->attributes, dest);
}

static const IMFAttributesVtbl video_renderer_attributes_vtbl =
{
    video_renderer_attributes_QueryInterface,
    video_renderer_attributes_AddRef,
    video_renderer_attributes_Release,
    video_renderer_attributes_GetItem,
    video_renderer_attributes_GetItemType,
    video_renderer_attributes_CompareItem,
    video_renderer_attributes_Compare,
    video_renderer_attributes_GetUINT32,
    video_renderer_attributes_GetUINT64,
    video_renderer_attributes_GetDouble,
    video_renderer_attributes_GetGUID,
    video_renderer_attributes_GetStringLength,
    video_renderer_attributes_GetString,
    video_renderer_attributes_GetAllocatedString,
    video_renderer_attributes_GetBlobSize,
    video_renderer_attributes_GetBlob,
    video_renderer_attributes_GetAllocatedBlob,
    video_renderer_attributes_GetUnknown,
    video_renderer_attributes_SetItem,
    video_renderer_attributes_DeleteItem,
    video_renderer_attributes_DeleteAllItems,
    video_renderer_attributes_SetUINT32,
    video_renderer_attributes_SetUINT64,
    video_renderer_attributes_SetDouble,
    video_renderer_attributes_SetGUID,
    video_renderer_attributes_SetString,
    video_renderer_attributes_SetBlob,
    video_renderer_attributes_SetUnknown,
    video_renderer_attributes_LockStore,
    video_renderer_attributes_UnlockStore,
    video_renderer_attributes_GetCount,
    video_renderer_attributes_GetItemByIndex,
    video_renderer_attributes_CopyAllItems,
};

static HRESULT evr_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct video_renderer *object;
    IMFVideoPresenter *presenter;
    IMFTransform *mixer;
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
    object->IMFAttributes_iface.lpVtbl = &video_renderer_attributes_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
        goto failed;

    /* Create mixer and presenter. */
    if (FAILED(hr = video_renderer_create_mixer(attributes, &mixer)))
        goto failed;

    if (FAILED(hr = video_renderer_create_presenter(attributes, &presenter)))
        goto failed;

    if (FAILED(hr = video_renderer_initialize(object, mixer, presenter)))
        goto failed;

    IMFTransform_Release(mixer);
    IMFVideoPresenter_Release(presenter);

    /* Default attributes */
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_ForceBob, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_AllowDropToBob, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_ForceThrottle, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_AllowDropToThrottle, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_ForceHalfInterlace, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_AllowDropToHalfInterlace, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_ForceScaling, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_AllowScaling, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_ForceBatching, 0);
    IMFAttributes_SetUINT32(object->attributes, &EVRConfig_AllowBatching, 0);

    *obj = (IUnknown *)&object->IMFMediaSink_iface;

    return S_OK;

failed:

    if (mixer)
        IMFTransform_Release(mixer);

    if (presenter)
        IMFVideoPresenter_Release(presenter);

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

/***********************************************************************
 *      MFCreateVideoRenderer (mf.@)
 */
HRESULT WINAPI MFCreateVideoRenderer(REFIID riid, void **renderer)
{
    IUnknown *obj;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), renderer);

    *renderer = NULL;

    if (SUCCEEDED(hr = evr_create_object(NULL, NULL, &obj)))
    {
        hr = IUnknown_QueryInterface(obj, riid, renderer);
        IUnknown_Release(obj);
    }

    return hr;
}
