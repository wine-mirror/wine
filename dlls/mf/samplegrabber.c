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

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mf_private.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum sink_state
{
    SINK_STATE_STOPPED = 0,
    SINK_STATE_RUNNING,
};

struct sample_grabber;

enum scheduled_item_type
{
    ITEM_TYPE_SAMPLE,
    ITEM_TYPE_MARKER,
};

struct scheduled_item
{
    struct list entry;
    enum scheduled_item_type type;
    union
    {
        IMFSample *sample;
        struct
        {
            MFSTREAMSINK_MARKER_TYPE type;
            PROPVARIANT context;
        } marker;
    } u;
};

struct sample_grabber_stream
{
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    IMFAsyncCallback timer_callback;
    LONG refcount;
    struct sample_grabber *sink;
    IMFMediaEventQueue *event_queue;
    IMFAttributes *sample_attributes;
    enum sink_state state;
    struct list items;
    IUnknown *cancel_key;
    CRITICAL_SECTION cs;
};

struct sample_grabber
{
    IMFMediaSink IMFMediaSink_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    LONG refcount;
    IMFSampleGrabberSinkCallback *callback;
    IMFSampleGrabberSinkCallback2 *callback2;
    IMFMediaType *media_type;
    BOOL is_shut_down;
    struct sample_grabber_stream *stream;
    IMFMediaEventQueue *event_queue;
    IMFPresentationClock *clock;
    IMFTimer *timer;
    UINT32 ignore_clock;
    UINT64 sample_time_offset;
    CRITICAL_SECTION cs;
};

static IMFSampleGrabberSinkCallback *sample_grabber_get_callback(const struct sample_grabber *sink)
{
    return sink->callback2 ? (IMFSampleGrabberSinkCallback *)sink->callback2 : sink->callback;
}

struct sample_grabber_activate_context
{
    IMFMediaType *media_type;
    IMFSampleGrabberSinkCallback *callback;
};

static void sample_grabber_free_private(void *user_context)
{
    struct sample_grabber_activate_context *context = user_context;
    IMFMediaType_Release(context->media_type);
    IMFSampleGrabberSinkCallback_Release(context->callback);
    heap_free(context);
}

static struct sample_grabber *impl_from_IMFMediaSink(IMFMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFMediaSink_iface);
}

static struct sample_grabber *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFClockStateSink_iface);
}

static struct sample_grabber *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFMediaEventGenerator_iface);
}

static struct sample_grabber_stream *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber_stream, IMFStreamSink_iface);
}

static struct sample_grabber_stream *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber_stream, IMFMediaTypeHandler_iface);
}

static struct sample_grabber_stream *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber_stream, timer_callback);
}

static HRESULT WINAPI sample_grabber_stream_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &stream->IMFStreamSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaTypeHandler))
    {
        *obj = &stream->IMFMediaTypeHandler_iface;
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

static ULONG WINAPI sample_grabber_stream_AddRef(IMFStreamSink *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static void stream_release_pending_item(struct scheduled_item *item)
{
    list_remove(&item->entry);
    switch (item->type)
    {
        case ITEM_TYPE_SAMPLE:
            IMFSample_Release(item->u.sample);
            break;
        case ITEM_TYPE_MARKER:
            PropVariantClear(&item->u.marker.context);
            break;
    }
    heap_free(item);
}

static ULONG WINAPI sample_grabber_stream_Release(IMFStreamSink *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);
    struct scheduled_item *item, *next_item;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (stream->sink)
        {
            IMFMediaSink_Release(&stream->sink->IMFMediaSink_iface);
            if (stream->sink->timer && stream->cancel_key)
                IMFTimer_CancelTimer(stream->sink->timer, stream->cancel_key);
        }
        if (stream->cancel_key)
            IUnknown_Release(stream->cancel_key);
        if (stream->event_queue)
        {
            IMFMediaEventQueue_Shutdown(stream->event_queue);
            IMFMediaEventQueue_Release(stream->event_queue);
        }
        if (stream->sample_attributes)
            IMFAttributes_Release(stream->sample_attributes);
        LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &stream->items, struct scheduled_item, entry)
        {
            stream_release_pending_item(item);
        }
        DeleteCriticalSection(&stream->cs);
        heap_free(stream);
    }

    return refcount;
}

static HRESULT WINAPI sample_grabber_stream_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(stream->event_queue, flags, event);
}

static HRESULT WINAPI sample_grabber_stream_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(stream->event_queue, callback, state);
}

static HRESULT WINAPI sample_grabber_stream_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(stream->event_queue, result, event);
}

static HRESULT WINAPI sample_grabber_stream_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI sample_grabber_stream_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, sink);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    *sink = &stream->sink->IMFMediaSink_iface;
    IMFMediaSink_AddRef(*sink);

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, identifier);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    *identifier = 0;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    if (!handler)
        return E_POINTER;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    *handler = &stream->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

static HRESULT sample_grabber_report_sample(struct sample_grabber *grabber, IMFSample *sample)
{
    LONGLONG sample_time, sample_duration = 0;
    IMFMediaBuffer *buffer;
    DWORD flags, size;
    GUID major_type;
    BYTE *data;
    HRESULT hr;

    hr = IMFMediaType_GetMajorType(grabber->media_type, &major_type);

    if (SUCCEEDED(hr))
        hr = IMFSample_GetSampleTime(sample, &sample_time);

    if (FAILED(IMFSample_GetSampleDuration(sample, &sample_duration)))
        sample_duration = 0;

    if (SUCCEEDED(hr))
        hr = IMFSample_GetSampleFlags(sample, &flags);

    if (SUCCEEDED(hr))
    {
        if (FAILED(IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
            return E_UNEXPECTED;

        if (SUCCEEDED(hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &size)))
        {
            if (grabber->callback2)
            {
                hr = IMFSample_CopyAllItems(sample, grabber->stream->sample_attributes);
                if (SUCCEEDED(hr))
                    hr = IMFSampleGrabberSinkCallback2_OnProcessSampleEx(grabber->callback2, &major_type, flags,
                            sample_time, sample_duration, data, size, grabber->stream->sample_attributes);
            }
            else
                hr = IMFSampleGrabberSinkCallback_OnProcessSample(grabber->callback, &major_type, flags, sample_time,
                            sample_duration, data, size);
            IMFMediaBuffer_Unlock(buffer);
        }

        IMFMediaBuffer_Release(buffer);
    }

    return hr;
}

static HRESULT stream_schedule_sample(struct sample_grabber_stream *stream, struct scheduled_item *item)
{
    LONGLONG sampletime;
    HRESULT hr;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    if (FAILED(hr = IMFSample_GetSampleTime(item->u.sample, &sampletime)))
        return hr;

    if (stream->cancel_key)
    {
        IUnknown_Release(stream->cancel_key);
        stream->cancel_key = NULL;
    }

    if (FAILED(hr = IMFTimer_SetTimer(stream->sink->timer, 0, sampletime - stream->sink->sample_time_offset,
            &stream->timer_callback, NULL, &stream->cancel_key)))
    {
        stream->cancel_key = NULL;
    }

    return hr;
}

static HRESULT stream_queue_sample(struct sample_grabber_stream *stream, IMFSample *sample)
{
    struct scheduled_item *item;
    LONGLONG sampletime;
    HRESULT hr;

    if (FAILED(hr = IMFSample_GetSampleTime(sample, &sampletime)))
        return hr;

    if (!(item = heap_alloc_zero(sizeof(*item))))
        return E_OUTOFMEMORY;

    item->type = ITEM_TYPE_SAMPLE;
    item->u.sample = sample;
    IMFSample_AddRef(item->u.sample);
    list_init(&item->entry);
    if (list_empty(&stream->items))
        hr = stream_schedule_sample(stream, item);

    if (SUCCEEDED(hr))
        list_add_tail(&stream->items, &item->entry);
    else
        stream_release_pending_item(item);

    return hr;
}

static HRESULT WINAPI sample_grabber_stream_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);
    LONGLONG sampletime;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, sample);

    if (!sample)
        return S_OK;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    EnterCriticalSection(&stream->cs);

    if (stream->state == SINK_STATE_RUNNING)
    {
        hr = IMFSample_GetSampleTime(sample, &sampletime);

        if (SUCCEEDED(hr))
        {
            if (stream->sink->ignore_clock)
                hr = sample_grabber_report_sample(stream->sink, sample);
            else
                hr = stream_queue_sample(stream, sample);
        }
    }

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static void sample_grabber_stream_report_marker(struct sample_grabber_stream *stream, const PROPVARIANT *context,
        HRESULT hr)
{
    IMFStreamSink_QueueEvent(&stream->IMFStreamSink_iface, MEStreamSinkMarker, &GUID_NULL, hr, context);
}

static HRESULT stream_place_marker(struct sample_grabber_stream *stream, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *context_value)
{
    struct scheduled_item *item;
    HRESULT hr;

    if (list_empty(&stream->items))
    {
        sample_grabber_stream_report_marker(stream, context_value, S_OK);
        return S_OK;
    }

    if (!(item = heap_alloc_zero(sizeof(*item))))
        return E_OUTOFMEMORY;

    item->type = ITEM_TYPE_MARKER;
    item->u.marker.type = marker_type;
    list_init(&item->entry);
    hr = PropVariantCopy(&item->u.marker.context, context_value);
    if (SUCCEEDED(hr))
        list_add_tail(&stream->items, &item->entry);
    else
        stream_release_pending_item(item);

    return hr;
}

static HRESULT WINAPI sample_grabber_stream_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    EnterCriticalSection(&stream->cs);

    if (stream->state == SINK_STATE_RUNNING)
        hr = stream_place_marker(stream, marker_type, context_value);

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_stream_Flush(IMFStreamSink *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFStreamSink(iface);
    struct scheduled_item *item, *next_item;

    TRACE("%p.\n", iface);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    EnterCriticalSection(&stream->cs);

    LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &stream->items, struct scheduled_item, entry)
    {
        /* Samples are discarded, markers are processed immediately. */
        switch (item->type)
        {
            case ITEM_TYPE_SAMPLE:
                break;
            case ITEM_TYPE_MARKER:
                sample_grabber_stream_report_marker(stream, &item->u.marker.context, E_ABORT);
                break;
        }

        stream_release_pending_item(item);
    }

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static const IMFStreamSinkVtbl sample_grabber_stream_vtbl =
{
    sample_grabber_stream_QueryInterface,
    sample_grabber_stream_AddRef,
    sample_grabber_stream_Release,
    sample_grabber_stream_GetEvent,
    sample_grabber_stream_BeginGetEvent,
    sample_grabber_stream_EndGetEvent,
    sample_grabber_stream_QueueEvent,
    sample_grabber_stream_GetMediaSink,
    sample_grabber_stream_GetIdentifier,
    sample_grabber_stream_GetMediaTypeHandler,
    sample_grabber_stream_ProcessSample,
    sample_grabber_stream_PlaceMarker,
    sample_grabber_stream_Flush,
};

static HRESULT WINAPI sample_grabber_stream_type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid,
        void **obj)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&stream->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_stream_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&stream->IMFStreamSink_iface);
}

static ULONG WINAPI sample_grabber_stream_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&stream->IMFStreamSink_iface);
}

static HRESULT WINAPI sample_grabber_stream_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);
    const DWORD supported_flags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES;
    DWORD flags;

    TRACE("%p, %p, %p.\n", iface, in_type, out_type);

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    if (!in_type)
        return E_POINTER;

    if (IMFMediaType_IsEqual(stream->sink->media_type, in_type, &flags) == S_OK)
        return S_OK;

    return (flags & supported_flags) == supported_flags ? S_OK : MF_E_INVALIDMEDIATYPE;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = 0;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **media_type)
{
    TRACE("%p, %u, %p.\n", iface, index, media_type);

    if (!media_type)
        return E_POINTER;

    return MF_E_NO_MORE_TYPES;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType *media_type)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, media_type);

    if (!media_type)
        return E_POINTER;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    IMFMediaType_Release(stream->sink->media_type);
    stream->sink->media_type = media_type;
    IMFMediaType_AddRef(stream->sink->media_type);

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType **media_type)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, media_type);

    if (!media_type)
        return E_POINTER;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    *media_type = stream->sink->media_type;
    IMFMediaType_AddRef(*media_type);

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct sample_grabber_stream *stream = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    if (!stream->sink)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaType_GetMajorType(stream->sink->media_type, type);
}

static const IMFMediaTypeHandlerVtbl sample_grabber_stream_type_handler_vtbl =
{
    sample_grabber_stream_type_handler_QueryInterface,
    sample_grabber_stream_type_handler_AddRef,
    sample_grabber_stream_type_handler_Release,
    sample_grabber_stream_type_handler_IsMediaTypeSupported,
    sample_grabber_stream_type_handler_GetMediaTypeCount,
    sample_grabber_stream_type_handler_GetMediaTypeByIndex,
    sample_grabber_stream_type_handler_SetCurrentMediaType,
    sample_grabber_stream_type_handler_GetCurrentMediaType,
    sample_grabber_stream_type_handler_GetMajorType,
};

static HRESULT WINAPI sample_grabber_stream_timer_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid,
        void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sample_grabber_stream_timer_callback_AddRef(IMFAsyncCallback *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFAsyncCallback(iface);
    return IMFStreamSink_AddRef(&stream->IMFStreamSink_iface);
}

static ULONG WINAPI sample_grabber_stream_timer_callback_Release(IMFAsyncCallback *iface)
{
    struct sample_grabber_stream *stream = impl_from_IMFAsyncCallback(iface);
    return IMFStreamSink_Release(&stream->IMFStreamSink_iface);
}

static HRESULT WINAPI sample_grabber_stream_timer_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags,
        DWORD *queue)
{
    return E_NOTIMPL;
}

static struct scheduled_item *stream_get_next_item(struct sample_grabber_stream *stream)
{
    struct scheduled_item *item = NULL;
    struct list *e;

    if ((e = list_head(&stream->items)))
        item = LIST_ENTRY(e, struct scheduled_item, entry);

    return item;
}

static void sample_grabber_stream_request_sample(struct sample_grabber_stream *stream)
{
    IMFStreamSink_QueueEvent(&stream->IMFStreamSink_iface, MEStreamSinkRequestSample, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI sample_grabber_stream_timer_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct sample_grabber_stream *stream = impl_from_IMFAsyncCallback(iface);
    struct scheduled_item *item;
    HRESULT hr;

    EnterCriticalSection(&stream->cs);

    /* Report and schedule next. */
    if (stream->sink && (item = stream_get_next_item(stream)))
    {
        while (item)
        {
            switch (item->type)
            {
                case ITEM_TYPE_SAMPLE:
                    if (FAILED(hr = sample_grabber_report_sample(stream->sink, item->u.sample)))
                        WARN("Failed to report a sample, hr %#x.\n", hr);
                    stream_release_pending_item(item);
                    item = stream_get_next_item(stream);
                    if (item && item->type == ITEM_TYPE_SAMPLE)
                    {
                        if (FAILED(hr = stream_schedule_sample(stream, item)))
                            WARN("Failed to schedule a sample, hr %#x.\n", hr);
                        sample_grabber_stream_request_sample(stream);
                        item = NULL;
                    }
                    break;
                case ITEM_TYPE_MARKER:
                    sample_grabber_stream_report_marker(stream, &item->u.marker.context, S_OK);
                    stream_release_pending_item(item);
                    item = stream_get_next_item(stream);
                    break;
            }
        }
    }

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl sample_grabber_stream_timer_callback_vtbl =
{
    sample_grabber_stream_timer_callback_QueryInterface,
    sample_grabber_stream_timer_callback_AddRef,
    sample_grabber_stream_timer_callback_Release,
    sample_grabber_stream_timer_callback_GetParameters,
    sample_grabber_stream_timer_callback_Invoke,
};

static HRESULT WINAPI sample_grabber_sink_QueryInterface(IMFMediaSink *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &grabber->IMFMediaSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &grabber->IMFClockStateSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &grabber->IMFMediaEventGenerator_iface;
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

static ULONG WINAPI sample_grabber_sink_AddRef(IMFMediaSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&grabber->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sample_grabber_sink_Release(IMFMediaSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&grabber->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (grabber->callback)
            IMFSampleGrabberSinkCallback_Release(grabber->callback);
        if (grabber->callback2)
            IMFSampleGrabberSinkCallback2_Release(grabber->callback2);
        IMFMediaType_Release(grabber->media_type);
        if (grabber->event_queue)
        {
            IMFMediaEventQueue_Shutdown(grabber->event_queue);
            IMFMediaEventQueue_Release(grabber->event_queue);
        }
        if (grabber->clock)
            IMFPresentationClock_Release(grabber->clock);
        if (grabber->timer)
            IMFTimer_Release(grabber->timer);
        DeleteCriticalSection(&grabber->cs);
        heap_free(grabber);
    }

    return refcount;
}

static HRESULT WINAPI sample_grabber_sink_GetCharacteristics(IMFMediaSink *iface, DWORD *flags)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS;
    if (grabber->ignore_clock)
        *flags |= MEDIASINK_RATELESS;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_sink_AddStreamSink(IMFMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return grabber->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI sample_grabber_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#x.\n", iface, stream_sink_id);

    return grabber->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI sample_grabber_sink_GetStreamSinkCount(IMFMediaSink *iface, DWORD *count)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_sink_GetStreamSinkByIndex(IMFMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, stream);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (index > 0)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = &grabber->stream->IMFStreamSink_iface;
       IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_sink_GetStreamSinkById(IMFMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#x, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id > 0)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = &grabber->stream->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&grabber->cs);

    if (SUCCEEDED(hr = IMFSampleGrabberSinkCallback_OnSetPresentationClock(sample_grabber_get_callback(grabber),
            clock)))
    {
        if (grabber->clock)
        {
            IMFPresentationClock_RemoveClockStateSink(grabber->clock, &grabber->IMFClockStateSink_iface);
            IMFPresentationClock_Release(grabber->clock);
            if (grabber->timer)
            {
                IMFTimer_Release(grabber->timer);
                grabber->timer = NULL;
            }
        }
        grabber->clock = clock;
        if (grabber->clock)
        {
            IMFPresentationClock_AddRef(grabber->clock);
            IMFPresentationClock_AddClockStateSink(grabber->clock, &grabber->IMFClockStateSink_iface);
            if (FAILED(IMFPresentationClock_QueryInterface(grabber->clock, &IID_IMFTimer, (void **)&grabber->timer)))
            {
                WARN("Failed to get IMFTimer interface.\n");
                grabber->timer = NULL;
            }
        }
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_sink_GetPresentationClock(IMFMediaSink *iface, IMFPresentationClock **clock)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    if (!clock)
        return E_POINTER;

    EnterCriticalSection(&grabber->cs);

    if (grabber->clock)
    {
        *clock = grabber->clock;
        IMFPresentationClock_AddRef(*clock);
    }
    else
        hr = MF_E_NO_CLOCK;

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_sink_Shutdown(IMFMediaSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr;

    TRACE("%p.\n", iface);

    if (grabber->is_shut_down)
        return MF_E_SHUTDOWN;

    EnterCriticalSection(&grabber->cs);
    grabber->is_shut_down = TRUE;
    if (SUCCEEDED(hr = IMFSampleGrabberSinkCallback_OnShutdown(sample_grabber_get_callback(grabber))))
    {
        IMFMediaSink_Release(&grabber->stream->sink->IMFMediaSink_iface);
        EnterCriticalSection(&grabber->stream->cs);
        grabber->stream->sink = NULL;
        LeaveCriticalSection(&grabber->stream->cs);
        IMFStreamSink_Release(&grabber->stream->IMFStreamSink_iface);
        grabber->stream = NULL;
    }
    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static const IMFMediaSinkVtbl sample_grabber_sink_vtbl =
{
    sample_grabber_sink_QueryInterface,
    sample_grabber_sink_AddRef,
    sample_grabber_sink_Release,
    sample_grabber_sink_GetCharacteristics,
    sample_grabber_sink_AddStreamSink,
    sample_grabber_sink_RemoveStreamSink,
    sample_grabber_sink_GetStreamSinkCount,
    sample_grabber_sink_GetStreamSinkByIndex,
    sample_grabber_sink_GetStreamSinkById,
    sample_grabber_sink_SetPresentationClock,
    sample_grabber_sink_GetPresentationClock,
    sample_grabber_sink_Shutdown,
};

static HRESULT WINAPI sample_grabber_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_QueryInterface(&grabber->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_AddRef(&grabber->IMFMediaSink_iface);
}

static ULONG WINAPI sample_grabber_clock_sink_Release(IMFClockStateSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);
    return IMFMediaSink_Release(&grabber->IMFMediaSink_iface);
}

static void sample_grabber_set_state(struct sample_grabber *grabber, enum sink_state state)
{
    static const DWORD events[] =
    {
        MEStreamSinkStopped, /* SINK_STATE_STOPPED */
        MEStreamSinkStarted, /* SINK_STATE_RUNNING */
    };
    BOOL set_state = FALSE;
    unsigned int i;

    EnterCriticalSection(&grabber->cs);

    if (grabber->stream)
    {
        switch (grabber->stream->state)
        {
            case SINK_STATE_STOPPED:
                set_state = state == SINK_STATE_RUNNING;
                break;
            case SINK_STATE_RUNNING:
                set_state = state == SINK_STATE_STOPPED;
                break;
            default:
                ;
        }

        if (set_state)
        {
            grabber->stream->state = state;
            if (state == SINK_STATE_RUNNING)
            {
                /* Every transition to running state sends a bunch requests to build up initial queue. */
                for (i = 0; i < 4; ++i)
                    sample_grabber_stream_request_sample(grabber->stream);
            }
            IMFStreamSink_QueueEvent(&grabber->stream->IMFStreamSink_iface, events[state], &GUID_NULL, S_OK, NULL);
        }
    }

    LeaveCriticalSection(&grabber->cs);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s, %s.\n", iface, wine_dbgstr_longlong(systime), wine_dbgstr_longlong(offset));

    sample_grabber_set_state(grabber, SINK_STATE_RUNNING);

    return IMFSampleGrabberSinkCallback_OnClockStart(sample_grabber_get_callback(grabber), systime, offset);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(systime));

    sample_grabber_set_state(grabber, SINK_STATE_STOPPED);

    return IMFSampleGrabberSinkCallback_OnClockStop(sample_grabber_get_callback(grabber), systime);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(systime));

    return IMFSampleGrabberSinkCallback_OnClockPause(sample_grabber_get_callback(grabber), systime);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(systime));

    sample_grabber_set_state(grabber, SINK_STATE_RUNNING);

    return IMFSampleGrabberSinkCallback_OnClockRestart(sample_grabber_get_callback(grabber), systime);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s, %f.\n", iface, wine_dbgstr_longlong(systime), rate);

    return IMFSampleGrabberSinkCallback_OnClockSetRate(sample_grabber_get_callback(grabber), systime, rate);
}

static HRESULT WINAPI sample_grabber_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_QueryInterface(&grabber->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_AddRef(&grabber->IMFMediaSink_iface);
}

static ULONG WINAPI sample_grabber_events_Release(IMFMediaEventGenerator *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);
    return IMFMediaSink_Release(&grabber->IMFMediaSink_iface);
}

static HRESULT WINAPI sample_grabber_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#x, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(grabber->event_queue, flags, event);
}

static HRESULT WINAPI sample_grabber_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(grabber->event_queue, callback, state);
}

static HRESULT WINAPI sample_grabber_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(grabber->event_queue, result, event);
}

static HRESULT WINAPI sample_grabber_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct sample_grabber *grabber = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %u, %s, %#x, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(grabber->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl sample_grabber_sink_events_vtbl =
{
    sample_grabber_events_QueryInterface,
    sample_grabber_events_AddRef,
    sample_grabber_events_Release,
    sample_grabber_events_GetEvent,
    sample_grabber_events_BeginGetEvent,
    sample_grabber_events_EndGetEvent,
    sample_grabber_events_QueueEvent,
};

static const IMFClockStateSinkVtbl sample_grabber_clock_sink_vtbl =
{
    sample_grabber_clock_sink_QueryInterface,
    sample_grabber_clock_sink_AddRef,
    sample_grabber_clock_sink_Release,
    sample_grabber_clock_sink_OnClockStart,
    sample_grabber_clock_sink_OnClockStop,
    sample_grabber_clock_sink_OnClockPause,
    sample_grabber_clock_sink_OnClockRestart,
    sample_grabber_clock_sink_OnClockSetRate,
};

static HRESULT sample_grabber_create_stream(struct sample_grabber *sink, struct sample_grabber_stream **stream)
{
    struct sample_grabber_stream *object;
    HRESULT hr;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFStreamSink_iface.lpVtbl = &sample_grabber_stream_vtbl;
    object->IMFMediaTypeHandler_iface.lpVtbl = &sample_grabber_stream_type_handler_vtbl;
    object->timer_callback.lpVtbl = &sample_grabber_stream_timer_callback_vtbl;
    object->refcount = 1;
    object->sink = sink;
    IMFMediaSink_AddRef(&object->sink->IMFMediaSink_iface);
    list_init(&object->items);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateAttributes(&object->sample_attributes, 0)))
        goto failed;

    *stream = object;

    return S_OK;

failed:
    IMFStreamSink_Release(&object->IMFStreamSink_iface);

    return hr;
}

static HRESULT sample_grabber_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct sample_grabber_activate_context *context = user_context;
    struct sample_grabber *object;
    HRESULT hr;
    GUID guid;

    TRACE("%p, %p, %p.\n", attributes, user_context, obj);

    /* At least major type is required. */
    if (FAILED(IMFMediaType_GetMajorType(context->media_type, &guid)))
        return MF_E_INVALIDMEDIATYPE;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaSink_iface.lpVtbl = &sample_grabber_sink_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &sample_grabber_clock_sink_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &sample_grabber_sink_events_vtbl;
    object->refcount = 1;
    if (FAILED(IMFSampleGrabberSinkCallback_QueryInterface(context->callback, &IID_IMFSampleGrabberSinkCallback2,
            (void **)&object->callback2)))
    {
        object->callback = context->callback;
        IMFSampleGrabberSinkCallback_AddRef(object->callback);
    }
    object->media_type = context->media_type;
    IMFMediaType_AddRef(object->media_type);
    IMFAttributes_GetUINT32(attributes, &MF_SAMPLEGRABBERSINK_IGNORE_CLOCK, &object->ignore_clock);
    IMFAttributes_GetUINT64(attributes, &MF_SAMPLEGRABBERSINK_SAMPLE_TIME_OFFSET, &object->sample_time_offset);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = sample_grabber_create_stream(object, &object->stream)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    *obj = (IUnknown *)&object->IMFMediaSink_iface;

    TRACE("Created %p.\n", *obj);

    return S_OK;

failed:

    IMFMediaSink_Release(&object->IMFMediaSink_iface);

    return hr;
}

static const struct activate_funcs sample_grabber_activate_funcs =
{
    sample_grabber_create_object,
    sample_grabber_free_private,
};

/***********************************************************************
 *      MFCreateSampleGrabberSinkActivate (mf.@)
 */
HRESULT WINAPI MFCreateSampleGrabberSinkActivate(IMFMediaType *media_type, IMFSampleGrabberSinkCallback *callback,
        IMFActivate **activate)
{
    struct sample_grabber_activate_context *context;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", media_type, callback, activate);

    if (!media_type || !callback || !activate)
        return E_POINTER;

    context = heap_alloc_zero(sizeof(*context));
    if (!context)
        return E_OUTOFMEMORY;

    context->media_type = media_type;
    IMFMediaType_AddRef(context->media_type);
    context->callback = callback;
    IMFSampleGrabberSinkCallback_AddRef(context->callback);

    if (FAILED(hr = create_activation_object(context, &sample_grabber_activate_funcs, activate)))
        sample_grabber_free_private(context);

    return hr;
}
