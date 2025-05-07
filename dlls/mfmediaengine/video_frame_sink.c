/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include <float.h>
#include <assert.h>
#include <stdbool.h>

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"

#include "mediaengine_private.h"

#include "initguid.h"
#include "evr.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum sink_state
{
    SINK_STATE_STOPPED = 0,
    SINK_STATE_PAUSED,
    SINK_STATE_RUNNING,
};

static inline const char *debugstr_time(LONGLONG time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

enum video_frame_sink_flags
{
    FLAGS_FIRST_FRAME = 0x1,
};

struct sample_queue
{
    IMFSample *samples[2];
    unsigned int used;
    unsigned int front;
    unsigned int back;
    IMFSample *presentation_sample;
    bool sample_presented;
};

struct video_frame_sink
{
    IMFMediaSink IMFMediaSink_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    IMFGetService IMFGetService_iface;
    LONG refcount;
    IUnknown *device_manager;
    IMFMediaType *media_type;
    IMFMediaType *current_media_type;
    bool is_shut_down;
    IMFMediaEventQueue *event_queue;
    IMFMediaEventQueue *stream_event_queue;
    IMFPresentationClock *clock;
    IMFAsyncCallback *callback;
    float rate;
    enum sink_state state;
    unsigned int flags;
    struct sample_queue queue;
    bool sample_request_pending;
    bool eos;
    CRITICAL_SECTION cs;
};

static void video_frame_sink_sample_queue_set_presentation(struct video_frame_sink *sink, IMFSample *sample)
{
    struct sample_queue *queue = &sink->queue;

    if (queue->presentation_sample)
        IMFSample_Release(queue->presentation_sample);

    queue->presentation_sample = sample;
    queue->sample_presented = FALSE;
}

static BOOL video_frame_sink_sample_queue_pop(struct video_frame_sink *sink, IMFSample **sample)
{
    struct sample_queue *queue = &sink->queue;

    if (queue->used)
    {
        *sample = queue->samples[queue->front];
        queue->front = (queue->front + 1) % ARRAY_SIZE(queue->samples);
        queue->used--;
    }
    else
        *sample = NULL;

    return *sample != NULL;
}

static void video_frame_sink_sample_queue_push(struct video_frame_sink *sink, IMFSample *sample,
        BOOL at_front)
{
    struct sample_queue *queue = &sink->queue;
    IMFSample *old_sample;
    unsigned int idx;

    /* if queue is full, we drop the oldest sample to make room for the new one */
    if (queue->used == ARRAY_SIZE(queue->samples))
    {
        video_frame_sink_sample_queue_pop(sink, &old_sample);
        IMFSample_Release(old_sample);
    }

    if (at_front)
        idx = queue->front = (ARRAY_SIZE(queue->samples) + queue->front - 1) % ARRAY_SIZE(queue->samples);
    else
        idx = queue->back = (queue->back + 1) % ARRAY_SIZE(queue->samples);
    queue->samples[idx] = sample;
    queue->used++;
}

static void video_frame_sink_sample_queue_flush(struct video_frame_sink *sink)
{
    struct sample_queue *queue = &sink->queue;
    IMFSample *sample;

    while (video_frame_sink_sample_queue_pop(sink, &sample))
        IMFSample_Release(sample);

    queue->used = 0;
    queue->front = 0;
    queue->back = ARRAY_SIZE(queue->samples) - 1;

    if (queue->presentation_sample)
    {
        IMFSample_Release(queue->presentation_sample);
        queue->presentation_sample = NULL;
        queue->sample_presented = FALSE;
    }
}

static void video_frame_sink_sample_queue_free(struct video_frame_sink *sink)
{
    struct sample_queue *queue = &sink->queue;
    IMFSample *sample;

    while (video_frame_sink_sample_queue_pop(sink, &sample))
        IMFSample_Release(sample);

    if (queue->presentation_sample)
        IMFSample_Release(queue->presentation_sample);
}

static void video_frame_sink_set_flag(struct video_frame_sink *sink, unsigned int mask, bool value)
{
    if (value)
        sink->flags |= mask;
    else
        sink->flags &= ~mask;
}

static struct video_frame_sink *impl_from_IMFMediaSink(IMFMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_frame_sink, IMFMediaSink_iface);
}

static struct video_frame_sink *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_frame_sink, IMFClockStateSink_iface);
}

static struct video_frame_sink *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct video_frame_sink, IMFMediaEventGenerator_iface);
}

static struct video_frame_sink *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct video_frame_sink, IMFStreamSink_iface);
}

static struct video_frame_sink *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct video_frame_sink, IMFMediaTypeHandler_iface);
}

static struct video_frame_sink *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct video_frame_sink, IMFGetService_iface);
}

static HRESULT WINAPI video_frame_sink_stream_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &sink->IMFStreamSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaTypeHandler))
    {
        *obj = &sink->IMFMediaTypeHandler_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &sink->IMFGetService_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI video_frame_sink_stream_AddRef(IMFStreamSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);
    return IMFMediaSink_AddRef(&sink->IMFMediaSink_iface);
}

static ULONG WINAPI video_frame_sink_stream_Release(IMFStreamSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);
    return IMFMediaSink_Release(&sink->IMFMediaSink_iface);
}

static HRESULT WINAPI video_frame_sink_stream_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_GetEvent(sink->stream_event_queue, flags, event);
}

static HRESULT WINAPI video_frame_sink_stream_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_BeginGetEvent(sink->stream_event_queue, callback, state);
}

static HRESULT WINAPI video_frame_sink_stream_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_EndGetEvent(sink->stream_event_queue, result, event);
}

static HRESULT WINAPI video_frame_sink_stream_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI video_frame_sink_stream_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **ret)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, ret);

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    *ret = &sink->IMFMediaSink_iface;
    IMFMediaSink_AddRef(*ret);

    return S_OK;
}

static HRESULT WINAPI video_frame_sink_stream_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, identifier);

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    *identifier = 0;

    return S_OK;
}

static HRESULT WINAPI video_frame_sink_stream_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    if (!handler)
        return E_POINTER;

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    *handler = &sink->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

/* must be called with critical section held */
static void video_frame_sink_stream_request_sample(struct video_frame_sink *sink)
{
    if (sink->sample_request_pending || sink->eos)
        return;

    IMFStreamSink_QueueEvent(&sink->IMFStreamSink_iface, MEStreamSinkRequestSample, &GUID_NULL, S_OK, NULL);
    sink->sample_request_pending = TRUE;
}

static void video_frame_sink_notify(struct video_frame_sink *sink, unsigned int event)
{
    IMFAsyncResult *result;

    if (FAILED(MFCreateAsyncResult(NULL, sink->callback, NULL, &result)))
        return;

    IMFAsyncResult_SetStatus(result, event);
    MFInvokeCallback(result);
    IMFAsyncResult_Release(result);
}

static HRESULT WINAPI video_frame_sink_stream_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);
    LONGLONG sampletime;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, sample);

    if (!sample)
        return S_OK;

    EnterCriticalSection(&sink->cs);

    sink->sample_request_pending = FALSE;

    if (sink->is_shut_down)
    {
        hr = MF_E_STREAMSINK_REMOVED;
    }
    else if (sink->state == SINK_STATE_RUNNING || sink->state == SINK_STATE_PAUSED)
    {
        hr = IMFSample_GetSampleTime(sample, &sampletime);

        if (SUCCEEDED(hr))
        {
            if (!(sink->flags & FLAGS_FIRST_FRAME))
            {
                video_frame_sink_notify(sink, MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY);
                video_frame_sink_set_flag(sink, FLAGS_FIRST_FRAME, TRUE);
            }
            /* else TODO: send MEQualityNotify event */

            IMFSample_AddRef(sample);
            video_frame_sink_sample_queue_push(sink, sample, FALSE);

            if (sink->queue.used != ARRAY_SIZE(sink->queue.samples))
                video_frame_sink_stream_request_sample(sink);
        }
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_stream_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
    {
        hr = MF_E_STREAMSINK_REMOVED;
    }
    else if (sink->state == SINK_STATE_RUNNING)
    {
        video_frame_sink_sample_queue_flush(sink);
        hr = IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkMarker,
                &GUID_NULL, S_OK, context_value);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_stream_Flush(IMFStreamSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFStreamSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
        hr = MF_E_STREAMSINK_REMOVED;
    else
        video_frame_sink_sample_queue_flush(sink);

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static const IMFStreamSinkVtbl video_frame_sink_stream_vtbl =
{
    video_frame_sink_stream_QueryInterface,
    video_frame_sink_stream_AddRef,
    video_frame_sink_stream_Release,
    video_frame_sink_stream_GetEvent,
    video_frame_sink_stream_BeginGetEvent,
    video_frame_sink_stream_EndGetEvent,
    video_frame_sink_stream_QueueEvent,
    video_frame_sink_stream_GetMediaSink,
    video_frame_sink_stream_GetIdentifier,
    video_frame_sink_stream_GetMediaTypeHandler,
    video_frame_sink_stream_ProcessSample,
    video_frame_sink_stream_PlaceMarker,
    video_frame_sink_stream_Flush,
};

static HRESULT WINAPI video_frame_sink_stream_type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid,
        void **obj)
{
    struct video_frame_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&sink->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI video_frame_sink_stream_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct video_frame_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&sink->IMFStreamSink_iface);
}

static ULONG WINAPI video_frame_sink_stream_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct video_frame_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&sink->IMFStreamSink_iface);
}

static HRESULT video_frame_sink_stream_is_media_type_supported(struct video_frame_sink *sink, IMFMediaType *in_type)
{
    const DWORD supported_flags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES |
            MF_MEDIATYPE_EQUAL_FORMAT_DATA;
    DWORD flags;

    if (sink->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    if (!in_type)
        return E_POINTER;

    if (IMFMediaType_IsEqual(sink->media_type, in_type, &flags) == S_OK)
        return S_OK;

    return (flags & supported_flags) == supported_flags ? S_OK : MF_E_INVALIDMEDIATYPE;
}

static HRESULT WINAPI video_frame_sink_stream_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    struct video_frame_sink *sink = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p, %p.\n", iface, in_type, out_type);

    return video_frame_sink_stream_is_media_type_supported(sink, in_type);
}

static HRESULT WINAPI video_frame_sink_stream_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = 0;

    return S_OK;
}

static HRESULT WINAPI video_frame_sink_stream_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **media_type)
{
    TRACE("%p, %lu, %p.\n", iface, index, media_type);

    if (!media_type)
        return E_POINTER;

    return MF_E_NO_MORE_TYPES;
}

static HRESULT WINAPI video_frame_sink_stream_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType *media_type)
{
    struct video_frame_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, media_type);

    if (FAILED(hr = video_frame_sink_stream_is_media_type_supported(sink, media_type)))
        return hr;

    IMFMediaType_Release(sink->current_media_type);
    sink->current_media_type = media_type;
    IMFMediaType_AddRef(sink->current_media_type);

    return S_OK;
}

static HRESULT WINAPI video_frame_sink_stream_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType **media_type)
{
    struct video_frame_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, media_type);

    if (!media_type)
        return E_POINTER;

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
    {
        hr = MF_E_STREAMSINK_REMOVED;
    }
    else
    {
        *media_type = sink->current_media_type;
        IMFMediaType_AddRef(*media_type);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_stream_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct video_frame_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
        hr = MF_E_STREAMSINK_REMOVED;
    else
        hr = IMFMediaType_GetMajorType(sink->current_media_type, type);

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static const IMFMediaTypeHandlerVtbl video_frame_sink_stream_type_handler_vtbl =
{
    video_frame_sink_stream_type_handler_QueryInterface,
    video_frame_sink_stream_type_handler_AddRef,
    video_frame_sink_stream_type_handler_Release,
    video_frame_sink_stream_type_handler_IsMediaTypeSupported,
    video_frame_sink_stream_type_handler_GetMediaTypeCount,
    video_frame_sink_stream_type_handler_GetMediaTypeByIndex,
    video_frame_sink_stream_type_handler_SetCurrentMediaType,
    video_frame_sink_stream_type_handler_GetCurrentMediaType,
    video_frame_sink_stream_type_handler_GetMajorType,
};

static HRESULT WINAPI video_frame_sink_stream_get_service_QueryInterface(IMFGetService *iface, REFIID riid,
        void **obj)
{
    struct video_frame_sink *sink = impl_from_IMFGetService(iface);
    return IMFStreamSink_QueryInterface(&sink->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI video_frame_sink_stream_get_service_AddRef(IMFGetService *iface)
{
    struct video_frame_sink *sink = impl_from_IMFGetService(iface);
    return IMFStreamSink_AddRef(&sink->IMFStreamSink_iface);
}

static ULONG WINAPI video_frame_sink_stream_get_service_Release(IMFGetService *iface)
{
    struct video_frame_sink *sink = impl_from_IMFGetService(iface);
    return IMFStreamSink_Release(&sink->IMFStreamSink_iface);
}

static HRESULT WINAPI video_frame_sink_stream_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid,
        void **obj)
{
    struct video_frame_sink *sink = impl_from_IMFGetService(iface);

    if (IsEqualGUID(service, &MR_VIDEO_ACCELERATION_SERVICE))
    {
        if (sink->device_manager)
            return IUnknown_QueryInterface(sink->device_manager, riid, obj);
        return E_NOINTERFACE;
    }

    FIXME("Unsupported service %s, riid %s.\n", debugstr_guid(service), debugstr_guid(riid));
    return MF_E_UNSUPPORTED_SERVICE;
}

static const IMFGetServiceVtbl video_frame_sink_stream_get_service_vtbl =
{
    video_frame_sink_stream_get_service_QueryInterface,
    video_frame_sink_stream_get_service_AddRef,
    video_frame_sink_stream_get_service_Release,
    video_frame_sink_stream_get_service_GetService,
};

static HRESULT WINAPI video_frame_sink_QueryInterface(IMFMediaSink *iface, REFIID riid, void **obj)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &sink->IMFMediaSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &sink->IMFClockStateSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &sink->IMFMediaEventGenerator_iface;
    }
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI video_frame_sink_AddRef(IMFMediaSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&sink->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_frame_sink_Release(IMFMediaSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&sink->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (sink->current_media_type)
            IMFMediaType_Release(sink->current_media_type);
        IMFMediaType_Release(sink->media_type);
        if (sink->device_manager)
            IUnknown_Release(sink->device_manager);
        if (sink->event_queue)
            IMFMediaEventQueue_Release(sink->event_queue);
        if (sink->clock)
            IMFPresentationClock_Release(sink->clock);
        if (sink->callback)
            IMFAsyncCallback_Release(sink->callback);
        if (sink->stream_event_queue)
        {
            IMFMediaEventQueue_Shutdown(sink->stream_event_queue);
            IMFMediaEventQueue_Release(sink->stream_event_queue);
        }
        video_frame_sink_sample_queue_free(sink);
        DeleteCriticalSection(&sink->cs);
        free(sink);
    }

    return refcount;
}

static HRESULT WINAPI video_frame_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *flags)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (sink->is_shut_down)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS | MEDIASINK_RATELESS;

    return S_OK;
}

static HRESULT WINAPI video_frame_sink_AddStreamSink(IMFMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#lx, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return sink->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI video_frame_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#lx.\n", iface, stream_sink_id);

    return sink->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI video_frame_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (sink->is_shut_down)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI video_frame_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, stream);

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (index > 0)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = &sink->IMFStreamSink_iface;
       IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_GetStreamSinkById(IMFMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#lx, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id > 0)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = &sink->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static void video_frame_sink_set_presentation_clock(struct video_frame_sink *sink, IMFPresentationClock *clock)
{
    if (sink->clock)
    {
        IMFPresentationClock_RemoveClockStateSink(sink->clock, &sink->IMFClockStateSink_iface);
        IMFPresentationClock_Release(sink->clock);
    }
    sink->clock = clock;
    if (sink->clock)
    {
        IMFPresentationClock_AddRef(sink->clock);
        IMFPresentationClock_AddClockStateSink(sink->clock, &sink->IMFClockStateSink_iface);
    }
}

static HRESULT WINAPI video_frame_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
    {
        hr = MF_E_SHUTDOWN;
    }
    else
    {
        video_frame_sink_set_presentation_clock(sink, clock);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_GetPresentationClock(IMFMediaSink *iface, IMFPresentationClock **clock)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    if (!clock)
        return E_POINTER;

    EnterCriticalSection(&sink->cs);

    if (sink->clock)
    {
        *clock = sink->clock;
        IMFPresentationClock_AddRef(*clock);
    }
    else
        hr = MF_E_NO_CLOCK;

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_Shutdown(IMFMediaSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else
    {
        sink->is_shut_down = TRUE;
        video_frame_sink_set_presentation_clock(sink, NULL);
        IMFMediaType_Release(sink->current_media_type);
        sink->current_media_type = NULL;
        IMFAsyncCallback_Release(sink->callback);
        sink->callback = NULL;
        IMFMediaEventQueue_Shutdown(sink->stream_event_queue);
        IMFMediaEventQueue_Shutdown(sink->event_queue);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static const IMFMediaSinkVtbl video_frame_sink_vtbl =
{
    video_frame_sink_QueryInterface,
    video_frame_sink_AddRef,
    video_frame_sink_Release,
    video_frame_sink_GetCharacteristics,
    video_frame_sink_AddStreamSink,
    video_frame_sink_RemoveStreamSink,
    video_frame_sink_GetStreamSinkCount,
    video_frame_sink_GetStreamSinkByIndex,
    video_frame_sink_GetStreamSinkById,
    video_frame_sink_SetPresentationClock,
    video_frame_sink_GetPresentationClock,
    video_frame_sink_Shutdown,
};

static HRESULT WINAPI video_frame_sink_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_QueryInterface(&sink->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_frame_sink_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_AddRef(&sink->IMFMediaSink_iface);
}

static ULONG WINAPI video_frame_sink_clock_sink_Release(IMFClockStateSink *iface)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_Release(&sink->IMFMediaSink_iface);
}

static HRESULT video_frame_sink_set_state(struct video_frame_sink *sink, enum sink_state state,
        MFTIME systime, LONGLONG offset)
{
    static const DWORD events[] =
    {
        [SINK_STATE_STOPPED] = MEStreamSinkStopped,
        [SINK_STATE_PAUSED] = MEStreamSinkPaused,
        [SINK_STATE_RUNNING] = MEStreamSinkStarted,
    };
    HRESULT hr = S_OK;

    EnterCriticalSection(&sink->cs);

    if (!sink->is_shut_down)
    {
        if (state == SINK_STATE_PAUSED && sink->state == SINK_STATE_STOPPED)
        {
            hr = MF_E_INVALID_STATE_TRANSITION;
        }
        else
        {
            if (state == SINK_STATE_STOPPED)
            {
                video_frame_sink_sample_queue_flush(sink);
                video_frame_sink_set_flag(sink, FLAGS_FIRST_FRAME, FALSE);
            }

            if (state == SINK_STATE_RUNNING && sink->state != SINK_STATE_RUNNING)
                video_frame_sink_stream_request_sample(sink);

            if (state != sink->state || state != SINK_STATE_PAUSED)
            {
                if (sink->rate == 0.0f && state == SINK_STATE_RUNNING)
                    IMFStreamSink_QueueEvent(&sink->IMFStreamSink_iface, MEStreamSinkScrubSampleComplete,
                            &GUID_NULL, S_OK, NULL);

                IMFStreamSink_QueueEvent(&sink->IMFStreamSink_iface, events[state], &GUID_NULL, S_OK, NULL);
            }
            sink->state = state;
        }
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    return video_frame_sink_set_state(sink, SINK_STATE_RUNNING, systime, offset);
}

static HRESULT WINAPI video_frame_sink_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return video_frame_sink_set_state(sink, SINK_STATE_STOPPED, systime, 0);
}

static HRESULT WINAPI video_frame_sink_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return video_frame_sink_set_state(sink, SINK_STATE_PAUSED, systime, 0);
}

static HRESULT WINAPI video_frame_sink_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return video_frame_sink_set_state(sink, SINK_STATE_RUNNING, systime, PRESENTATION_CURRENT_POSITION);
}

static HRESULT WINAPI video_frame_sink_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    struct video_frame_sink *sink = impl_from_IMFClockStateSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    EnterCriticalSection(&sink->cs);

    if (sink->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else
    {
        IMFStreamSink_QueueEvent(&sink->IMFStreamSink_iface, MEStreamSinkRateChanged, &GUID_NULL, S_OK, NULL);
        sink->rate = rate;
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI video_frame_sink_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct video_frame_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_QueryInterface(&sink->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI video_frame_sink_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct video_frame_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_AddRef(&sink->IMFMediaSink_iface);
}

static ULONG WINAPI video_frame_sink_events_Release(IMFMediaEventGenerator *iface)
{
    struct video_frame_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_Release(&sink->IMFMediaSink_iface);
}

static HRESULT WINAPI video_frame_sink_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct video_frame_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(sink->event_queue, flags, event);
}

static HRESULT WINAPI video_frame_sink_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct video_frame_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(sink->event_queue, callback, state);
}

static HRESULT WINAPI video_frame_sink_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct video_frame_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(sink->event_queue, result, event);
}

static HRESULT WINAPI video_frame_sink_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct video_frame_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(sink->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl video_frame_sink_events_vtbl =
{
    video_frame_sink_events_QueryInterface,
    video_frame_sink_events_AddRef,
    video_frame_sink_events_Release,
    video_frame_sink_events_GetEvent,
    video_frame_sink_events_BeginGetEvent,
    video_frame_sink_events_EndGetEvent,
    video_frame_sink_events_QueueEvent,
};

static const IMFClockStateSinkVtbl video_frame_sink_clock_sink_vtbl =
{
    video_frame_sink_clock_sink_QueryInterface,
    video_frame_sink_clock_sink_AddRef,
    video_frame_sink_clock_sink_Release,
    video_frame_sink_clock_sink_OnClockStart,
    video_frame_sink_clock_sink_OnClockStop,
    video_frame_sink_clock_sink_OnClockPause,
    video_frame_sink_clock_sink_OnClockRestart,
    video_frame_sink_clock_sink_OnClockSetRate,
};

HRESULT create_video_frame_sink(IMFMediaType *media_type, IUnknown *device_manager, IMFAsyncCallback *events_callback, struct video_frame_sink **sink)
{
    struct video_frame_sink *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaSink_iface.lpVtbl = &video_frame_sink_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &video_frame_sink_clock_sink_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &video_frame_sink_events_vtbl;
    object->IMFStreamSink_iface.lpVtbl = &video_frame_sink_stream_vtbl;
    object->IMFMediaTypeHandler_iface.lpVtbl = &video_frame_sink_stream_type_handler_vtbl;
    object->IMFGetService_iface.lpVtbl = &video_frame_sink_stream_get_service_vtbl;
    object->refcount = 1;
    object->rate = 1.0f;
    object->queue.back = ARRAY_SIZE(object->queue.samples) - 1;
    if ((object->device_manager = device_manager))
        IUnknown_AddRef(object->device_manager);
    object->media_type = media_type;
    IMFAsyncCallback_AddRef(object->callback = events_callback);
    IMFMediaType_AddRef(object->media_type);
    object->current_media_type = media_type;
    IMFMediaType_AddRef(object->current_media_type);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->stream_event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    *sink = object;

    return S_OK;

failed:

    IMFMediaSink_Release(&object->IMFMediaSink_iface);

    return hr;
}

HRESULT video_frame_sink_query_iface(struct video_frame_sink *sink, REFIID riid, void **obj)
{
    return IMFStreamSink_QueryInterface(&sink->IMFStreamSink_iface, riid, obj);
}

int video_frame_sink_get_sample(struct video_frame_sink *sink, IMFSample **ret)
{
    *ret = NULL;

    if (sink)
    {
        EnterCriticalSection(&sink->cs);

        if (sink->queue.presentation_sample)
        {
            IMFSample_AddRef(*ret = sink->queue.presentation_sample);
            sink->queue.sample_presented = TRUE;
        }

        LeaveCriticalSection(&sink->cs);
    }

    return !!*ret;
}

static HRESULT sample_get_pts(IMFSample *sample, MFTIME clocktime, LONGLONG *pts)
{
    HRESULT hr = S_FALSE;
    LONGLONG sample_pts;

    if (sample)
    {
        if (SUCCEEDED(hr = IMFSample_GetSampleTime(sample, &sample_pts)))
        {
            hr = (clocktime >= sample_pts) ? S_OK : S_FALSE;
            if (hr == S_OK)
                *pts = sample_pts;
        }
        else
            WARN("Failed to get sample time, hr %#lx.\n", hr);
    }
    return hr;
}

/*
 * This function selects the queued sample with the greatest PTS that is also below the supplied value of clocktime.
 * If no queued sample has a PTS below the supplied value of clocktime, S_FALSE is returned.
 * Otherwise S_TRUE is returned, the PTS of the selected sample is provided, and the selected sample will now be returned
 * by subsequent calls to video_frame_sink_get_sample.
 * Queued samples with a PTS lower than the PTS of the selected sample will be silently dropped.
 */
HRESULT video_frame_sink_get_pts(struct video_frame_sink *sink, MFTIME clocktime, LONGLONG *pts)
{
    HRESULT hr = S_FALSE;

    *pts = MINLONGLONG;
    if (sink)
    {
        IMFSample *sample;
        bool transfer_sample = FALSE;
        EnterCriticalSection(&sink->cs);
        while (video_frame_sink_sample_queue_pop(sink, &sample))
        {
            if (sample_get_pts(sample, clocktime, pts) == S_OK)
            {
                video_frame_sink_sample_queue_set_presentation(sink, sample);
                transfer_sample = TRUE;
                hr = S_OK;
            }
            else
            {
                video_frame_sink_sample_queue_push(sink, sample, TRUE);
                break;
            }
        }

        if (transfer_sample)
            video_frame_sink_stream_request_sample(sink);
        else if (!sink->queue.sample_presented)
            hr = sample_get_pts(sink->queue.presentation_sample, clocktime, pts);

        LeaveCriticalSection(&sink->cs);
    }

    return hr;
}

void video_frame_sink_notify_end_of_presentation_segment(struct video_frame_sink *sink)
{
    sink->eos = TRUE;
}

ULONG video_frame_sink_release(struct video_frame_sink *sink)
{
    return video_frame_sink_Release(&sink->IMFMediaSink_iface);
}
