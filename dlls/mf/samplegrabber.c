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

#include "mfidl.h"
#include "mf_private.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum sink_state
{
    SINK_STATE_STOPPED = 0,
    SINK_STATE_PAUSED,
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

#define MAX_SAMPLE_QUEUE_LENGTH 4

struct sample_grabber
{
    IMFMediaSink IMFMediaSink_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFGetService IMFGetService_iface;
    IMFRateSupport IMFRateSupport_iface;
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    IMFAsyncCallback timer_callback;
    LONG refcount;
    IMFSampleGrabberSinkCallback *callback;
    IMFSampleGrabberSinkCallback2 *callback2;
    IMFMediaType *media_type;
    IMFMediaType *current_media_type;
    BOOL is_shut_down;
    IMFMediaEventQueue *event_queue;
    IMFMediaEventQueue *stream_event_queue;
    IMFPresentationClock *clock;
    IMFTimer *timer;
    IMFAttributes *sample_attributes;
    struct list items;
    IUnknown *cancel_key;
    UINT32 ignore_clock;
    UINT64 sample_time_offset;
    float rate;
    enum sink_state state;
    CRITICAL_SECTION cs;
    UINT32 sample_count;
    IMFSample *samples[MAX_SAMPLE_QUEUE_LENGTH];
};

static IMFSampleGrabberSinkCallback *sample_grabber_get_callback(const struct sample_grabber *sink)
{
    return sink->callback2 ? (IMFSampleGrabberSinkCallback *)sink->callback2 : sink->callback;
}

struct sample_grabber_activate_context
{
    IMFMediaType *media_type;
    IMFSampleGrabberSinkCallback *callback;
    BOOL shut_down;
};

static void sample_grabber_free_private(void *user_context)
{
    struct sample_grabber_activate_context *context = user_context;
    IMFMediaType_Release(context->media_type);
    IMFSampleGrabberSinkCallback_Release(context->callback);
    free(context);
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

static struct sample_grabber *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFGetService_iface);
}

static struct sample_grabber *impl_from_IMFRateSupport(IMFRateSupport *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFRateSupport_iface);
}

static struct sample_grabber *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFStreamSink_iface);
}

static struct sample_grabber *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMFMediaTypeHandler_iface);
}

static struct sample_grabber *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, timer_callback);
}

static HRESULT WINAPI sample_grabber_stream_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &grabber->IMFStreamSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaTypeHandler))
    {
        *obj = &grabber->IMFMediaTypeHandler_iface;
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
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);
    return IMFMediaSink_AddRef(&grabber->IMFMediaSink_iface);
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
    free(item);
}

static ULONG WINAPI sample_grabber_stream_Release(IMFStreamSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);
    return IMFMediaSink_Release(&grabber->IMFMediaSink_iface);
}

static HRESULT WINAPI sample_grabber_stream_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_GetEvent(grabber->stream_event_queue, flags, event);
}

static HRESULT WINAPI sample_grabber_stream_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_BeginGetEvent(grabber->stream_event_queue, callback, state);
}

static HRESULT WINAPI sample_grabber_stream_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_EndGetEvent(grabber->stream_event_queue, result, event);
}

static HRESULT WINAPI sample_grabber_stream_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_QueueEventParamVar(grabber->stream_event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI sample_grabber_stream_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **sink)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, sink);

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    *sink = &grabber->IMFMediaSink_iface;
    IMFMediaSink_AddRef(*sink);

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, identifier);

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    *identifier = 0;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    if (!handler)
        return E_POINTER;

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    *handler = &grabber->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

static HRESULT sample_grabber_report_sample(struct sample_grabber *grabber, IMFSample *sample, BOOL *sample_delivered)
{
    LONGLONG sample_time, sample_duration = 0;
    IMFMediaBuffer *buffer;
    DWORD flags, size;
    GUID major_type;
    BYTE *data;
    HRESULT hr;

    *sample_delivered = FALSE;

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
            *sample_delivered = TRUE;

            if (grabber->callback2)
            {
                hr = IMFSample_CopyAllItems(sample, grabber->sample_attributes);
                if (SUCCEEDED(hr))
                    hr = IMFSampleGrabberSinkCallback2_OnProcessSampleEx(grabber->callback2, &major_type, flags,
                            sample_time, sample_duration, data, size, grabber->sample_attributes);
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

static HRESULT stream_schedule_sample(struct sample_grabber *grabber, struct scheduled_item *item)
{
    LONGLONG sampletime;
    HRESULT hr;

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    if (FAILED(hr = IMFSample_GetSampleTime(item->u.sample, &sampletime)))
        return hr;

    if (grabber->cancel_key)
    {
        IUnknown_Release(grabber->cancel_key);
        grabber->cancel_key = NULL;
    }

    if (FAILED(hr = IMFTimer_SetTimer(grabber->timer, 0, sampletime - grabber->sample_time_offset,
            &grabber->timer_callback, NULL, &grabber->cancel_key)))
    {
        grabber->cancel_key = NULL;
    }

    return hr;
}

static HRESULT stream_queue_sample(struct sample_grabber *grabber, IMFSample *sample)
{
    struct scheduled_item *item;
    LONGLONG sampletime;
    HRESULT hr;

    if (FAILED(hr = IMFSample_GetSampleTime(sample, &sampletime)))
        return hr;

    if (!(item = calloc(1, sizeof(*item))))
        return E_OUTOFMEMORY;

    item->type = ITEM_TYPE_SAMPLE;
    item->u.sample = sample;
    IMFSample_AddRef(item->u.sample);
    list_init(&item->entry);
    if (list_empty(&grabber->items))
        hr = stream_schedule_sample(grabber, item);

    if (SUCCEEDED(hr))
        list_add_tail(&grabber->items, &item->entry);
    else
        stream_release_pending_item(item);

    return hr;
}

static void sample_grabber_stream_request_sample(struct sample_grabber *grabber)
{
    IMFStreamSink_QueueEvent(&grabber->IMFStreamSink_iface, MEStreamSinkRequestSample, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI sample_grabber_stream_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);
    BOOL sample_delivered;
    LONGLONG sampletime;
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, sample);

    if (!sample)
        return S_OK;

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_STREAMSINK_REMOVED;
    else if (grabber->state == SINK_STATE_RUNNING || (grabber->state == SINK_STATE_PAUSED && grabber->ignore_clock))
    {
        hr = IMFSample_GetSampleTime(sample, &sampletime);

        if (SUCCEEDED(hr))
        {
            if (grabber->ignore_clock)
            {
                /* OnProcessSample() could return error code, which has to be propagated but isn't a blocker.
                   Use additional flag indicating that user callback was called at all. */
                hr = sample_grabber_report_sample(grabber, sample, &sample_delivered);
                if (sample_delivered)
                    sample_grabber_stream_request_sample(grabber);
            }
            else
                hr = stream_queue_sample(grabber, sample);
        }
    }
    else if (grabber->state == SINK_STATE_PAUSED)
    {
        if (grabber->sample_count < MAX_SAMPLE_QUEUE_LENGTH)
        {
            IMFSample_AddRef(sample);
            grabber->samples[grabber->sample_count++] = sample;
        }
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static void sample_grabber_stream_report_marker(struct sample_grabber *grabber, const PROPVARIANT *context,
        HRESULT hr)
{
    IMFStreamSink_QueueEvent(&grabber->IMFStreamSink_iface, MEStreamSinkMarker, &GUID_NULL, hr, context);
}

static HRESULT stream_place_marker(struct sample_grabber *grabber, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *context_value)
{
    struct scheduled_item *item;
    HRESULT hr = S_OK;

    if (list_empty(&grabber->items))
    {
        sample_grabber_stream_report_marker(grabber, context_value, S_OK);
        return S_OK;
    }

    if (!(item = calloc(1, sizeof(*item))))
        return E_OUTOFMEMORY;

    item->type = ITEM_TYPE_MARKER;
    item->u.marker.type = marker_type;
    list_init(&item->entry);
    PropVariantInit(&item->u.marker.context);
    if (context_value)
        hr = PropVariantCopy(&item->u.marker.context, context_value);
    if (SUCCEEDED(hr))
        list_add_tail(&grabber->items, &item->entry);
    else
        stream_release_pending_item(item);

    return hr;
}

static HRESULT WINAPI sample_grabber_stream_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_STREAMSINK_REMOVED;
    else if (grabber->state == SINK_STATE_RUNNING)
        hr = stream_place_marker(grabber, marker_type, context_value);

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_stream_Flush(IMFStreamSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFStreamSink(iface);
    struct scheduled_item *item, *next_item;
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_STREAMSINK_REMOVED;
    else
    {
        LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &grabber->items, struct scheduled_item, entry)
        {
            /* Samples are discarded, markers are processed immediately. */
            if (item->type == ITEM_TYPE_MARKER)
                sample_grabber_stream_report_marker(grabber, &item->u.marker.context, E_ABORT);

            stream_release_pending_item(item);
        }
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
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
    struct sample_grabber *grabber = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&grabber->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_stream_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&grabber->IMFStreamSink_iface);
}

static ULONG WINAPI sample_grabber_stream_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&grabber->IMFStreamSink_iface);
}

static HRESULT sample_grabber_stream_is_media_type_supported(struct sample_grabber *grabber, IMFMediaType *in_type)
{
    const DWORD supported_flags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES |
            MF_MEDIATYPE_EQUAL_FORMAT_DATA;
    DWORD flags;

    if (grabber->is_shut_down)
        return MF_E_STREAMSINK_REMOVED;

    if (!in_type)
        return E_POINTER;

    if (IMFMediaType_IsEqual(grabber->media_type, in_type, &flags) == S_OK)
        return S_OK;

    return (flags & supported_flags) == supported_flags ? S_OK : MF_E_INVALIDMEDIATYPE;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    struct sample_grabber *grabber = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p, %p.\n", iface, in_type, out_type);

    return sample_grabber_stream_is_media_type_supported(grabber, in_type);
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
    TRACE("%p, %lu, %p.\n", iface, index, media_type);

    if (!media_type)
        return E_POINTER;

    return MF_E_NO_MORE_TYPES;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType *media_type)
{
    struct sample_grabber *grabber = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, media_type);

    if (FAILED(hr = sample_grabber_stream_is_media_type_supported(grabber, media_type)))
        return hr;

    IMFMediaType_Release(grabber->current_media_type);
    grabber->current_media_type = media_type;
    IMFMediaType_AddRef(grabber->current_media_type);

    return S_OK;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType **media_type)
{
    struct sample_grabber *grabber = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, media_type);

    if (!media_type)
        return E_POINTER;

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
    {
        hr = MF_E_STREAMSINK_REMOVED;
    }
    else
    {
        *media_type = grabber->current_media_type;
        IMFMediaType_AddRef(*media_type);
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static HRESULT WINAPI sample_grabber_stream_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct sample_grabber *grabber = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_STREAMSINK_REMOVED;
    else
        hr = IMFMediaType_GetMajorType(grabber->current_media_type, type);

    LeaveCriticalSection(&grabber->cs);

    return hr;
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
    struct sample_grabber *grabber = impl_from_IMFAsyncCallback(iface);
    return IMFStreamSink_AddRef(&grabber->IMFStreamSink_iface);
}

static ULONG WINAPI sample_grabber_stream_timer_callback_Release(IMFAsyncCallback *iface)
{
    struct sample_grabber *grabber = impl_from_IMFAsyncCallback(iface);
    return IMFStreamSink_Release(&grabber->IMFStreamSink_iface);
}

static HRESULT WINAPI sample_grabber_stream_timer_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags,
        DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI sample_grabber_stream_timer_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct sample_grabber *grabber = impl_from_IMFAsyncCallback(iface);
    BOOL sample_reported = FALSE, sample_delivered = FALSE;
    struct scheduled_item *item, *item2;
    HRESULT hr;

    EnterCriticalSection(&grabber->cs);

    LIST_FOR_EACH_ENTRY_SAFE(item, item2, &grabber->items, struct scheduled_item, entry)
    {
        if (item->type == ITEM_TYPE_MARKER)
        {
            sample_grabber_stream_report_marker(grabber, &item->u.marker.context, S_OK);
            stream_release_pending_item(item);
        }
        else if (item->type == ITEM_TYPE_SAMPLE)
        {
            if (!sample_reported)
            {
                if (FAILED(hr = sample_grabber_report_sample(grabber, item->u.sample, &sample_delivered)))
                    WARN("Failed to report a sample, hr %#lx.\n", hr);
                stream_release_pending_item(item);
                sample_reported = TRUE;
            }
            else
            {
                if (FAILED(hr = stream_schedule_sample(grabber, item)))
                    WARN("Failed to schedule a sample, hr %#lx.\n", hr);
                break;
            }
        }
    }
    if (sample_delivered)
        sample_grabber_stream_request_sample(grabber);

    LeaveCriticalSection(&grabber->cs);

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
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &grabber->IMFGetService_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFRateSupport))
    {
        *obj = &grabber->IMFRateSupport_iface;
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

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static void sample_grabber_release_pending_items(struct sample_grabber *grabber)
{
    struct scheduled_item *item, *next_item;

    LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &grabber->items, struct scheduled_item, entry)
    {
        stream_release_pending_item(item);
    }
}

static void release_samples(struct sample_grabber *grabber)
{
    unsigned int i;

    for (i = 0; i < MAX_SAMPLE_QUEUE_LENGTH; ++i)
    {
        if (grabber->samples[i])
        {
            IMFSample_Release(grabber->samples[i]);
            grabber->samples[i] = NULL;
        }
    }
}

static ULONG WINAPI sample_grabber_sink_Release(IMFMediaSink *iface)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&grabber->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (grabber->callback)
            IMFSampleGrabberSinkCallback_Release(grabber->callback);
        if (grabber->callback2)
            IMFSampleGrabberSinkCallback2_Release(grabber->callback2);
        if (grabber->current_media_type)
            IMFMediaType_Release(grabber->current_media_type);
        IMFMediaType_Release(grabber->media_type);
        if (grabber->event_queue)
            IMFMediaEventQueue_Release(grabber->event_queue);
        if (grabber->clock)
            IMFPresentationClock_Release(grabber->clock);
        if (grabber->timer)
            IMFTimer_Release(grabber->timer);
        if (grabber->cancel_key)
            IUnknown_Release(grabber->cancel_key);
        if (grabber->stream_event_queue)
        {
            IMFMediaEventQueue_Shutdown(grabber->stream_event_queue);
            IMFMediaEventQueue_Release(grabber->stream_event_queue);
        }
        if (grabber->sample_attributes)
            IMFAttributes_Release(grabber->sample_attributes);
        sample_grabber_release_pending_items(grabber);
        release_samples(grabber);
        DeleteCriticalSection(&grabber->cs);
        free(grabber);
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

    TRACE("%p, %#lx, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return grabber->is_shut_down ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI sample_grabber_sink_RemoveStreamSink(IMFMediaSink *iface, DWORD stream_sink_id)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);

    TRACE("%p, %#lx.\n", iface, stream_sink_id);

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

    TRACE("%p, %lu, %p.\n", iface, index, stream);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (index > 0)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = &grabber->IMFStreamSink_iface;
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

    TRACE("%p, %#lx, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id > 0)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = &grabber->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&grabber->cs);

    return hr;
}

static void sample_grabber_cancel_timer(struct sample_grabber *grabber)
{
    if (grabber->timer && grabber->cancel_key)
    {
        IMFTimer_CancelTimer(grabber->timer, grabber->cancel_key);
        IUnknown_Release(grabber->cancel_key);
        grabber->cancel_key = NULL;
    }
}

static HRESULT sample_grabber_set_presentation_clock(struct sample_grabber *grabber, IMFPresentationClock *clock)
{
    HRESULT hr;

    if (FAILED(hr = IMFSampleGrabberSinkCallback_OnSetPresentationClock(sample_grabber_get_callback(grabber), clock)))
        return hr;

    if (grabber->clock)
    {
        sample_grabber_cancel_timer(grabber);
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

    return hr;
}

static HRESULT WINAPI sample_grabber_sink_SetPresentationClock(IMFMediaSink *iface, IMFPresentationClock *clock)
{
    struct sample_grabber *grabber = impl_from_IMFMediaSink(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
    {
        hr = MF_E_SHUTDOWN;
    }
    else
    {
        hr = sample_grabber_set_presentation_clock(grabber, clock);
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

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else
    {
        grabber->is_shut_down = TRUE;
        sample_grabber_release_pending_items(grabber);
        if (SUCCEEDED(hr = sample_grabber_set_presentation_clock(grabber, NULL)))
            hr = IMFSampleGrabberSinkCallback_OnShutdown(sample_grabber_get_callback(grabber));
        if (SUCCEEDED(hr))
        {
            IMFMediaType_Release(grabber->current_media_type);
            grabber->current_media_type = NULL;
            IMFMediaEventQueue_Shutdown(grabber->stream_event_queue);
            IMFMediaEventQueue_Shutdown(grabber->event_queue);
        }
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

static HRESULT sample_grabber_set_state(struct sample_grabber *grabber, enum sink_state state,
                                        MFTIME systime, LONGLONG offset)
{
    static const DWORD events[] =
    {
        [SINK_STATE_STOPPED] = MEStreamSinkStopped,
        [SINK_STATE_PAUSED]  = MEStreamSinkPaused,
        [SINK_STATE_RUNNING] = MEStreamSinkStarted,
    };
    BOOL do_callback = FALSE;
    HRESULT hr = S_OK;
    unsigned int i;

    EnterCriticalSection(&grabber->cs);

    if (!grabber->is_shut_down)
    {
        if (state == SINK_STATE_PAUSED && grabber->state == SINK_STATE_STOPPED)
            hr = MF_E_INVALID_STATE_TRANSITION;
        else
        {
            if (state == SINK_STATE_STOPPED)
            {
                sample_grabber_cancel_timer(grabber);
                release_samples(grabber);
                grabber->sample_count = MAX_SAMPLE_QUEUE_LENGTH;
                sample_grabber_release_pending_items(grabber);
            }

            if (state == SINK_STATE_RUNNING && grabber->state != SINK_STATE_RUNNING)
            {
                /* Every transition to running state sends a bunch requests to build up initial queue. */
                for (i = 0; i < grabber->sample_count; ++i)
                {
                    if (grabber->state == SINK_STATE_PAUSED && offset == PRESENTATION_CURRENT_POSITION)
                    {
                        assert(grabber->samples[i]);
                        stream_queue_sample(grabber, grabber->samples[i]);
                    }
                    else
                    {
                        sample_grabber_stream_request_sample(grabber);
                    }
                }
                release_samples(grabber);
                grabber->sample_count = 0;
            }

            do_callback = state != grabber->state || state != SINK_STATE_PAUSED;
            if (do_callback)
            {
                if (grabber->rate == 0.0f && state == SINK_STATE_RUNNING)
                    IMFStreamSink_QueueEvent(&grabber->IMFStreamSink_iface, MEStreamSinkScrubSampleComplete,
                            &GUID_NULL, S_OK, NULL);

                IMFStreamSink_QueueEvent(&grabber->IMFStreamSink_iface, events[state], &GUID_NULL, S_OK, NULL);
            }
            grabber->state = state;
        }
    }

    LeaveCriticalSection(&grabber->cs);

    if (do_callback)
    {
        switch (state)
        {
        case SINK_STATE_STOPPED:
            hr = IMFSampleGrabberSinkCallback_OnClockStop(sample_grabber_get_callback(grabber), systime);
            break;
        case SINK_STATE_PAUSED:
            hr = IMFSampleGrabberSinkCallback_OnClockPause(sample_grabber_get_callback(grabber), systime);
            break;
        case SINK_STATE_RUNNING:
            hr = IMFSampleGrabberSinkCallback_OnClockStart(sample_grabber_get_callback(grabber), systime, offset);
            break;
        }
    }

    return hr;
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    return sample_grabber_set_state(grabber, SINK_STATE_RUNNING, systime, offset);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return sample_grabber_set_state(grabber, SINK_STATE_STOPPED, systime, 0);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return sample_grabber_set_state(grabber, SINK_STATE_PAUSED, systime, 0);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return sample_grabber_set_state(grabber, SINK_STATE_RUNNING, systime, PRESENTATION_CURRENT_POSITION);
}

static HRESULT WINAPI sample_grabber_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    struct sample_grabber *grabber = impl_from_IMFClockStateSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    EnterCriticalSection(&grabber->cs);

    if (grabber->is_shut_down)
        hr = MF_E_SHUTDOWN;
    else
    {
        IMFStreamSink_QueueEvent(&grabber->IMFStreamSink_iface, MEStreamSinkRateChanged, &GUID_NULL, S_OK, NULL);
        grabber->rate = rate;
    }

    LeaveCriticalSection(&grabber->cs);

    if (SUCCEEDED(hr))
        hr = IMFSampleGrabberSinkCallback_OnClockSetRate(sample_grabber_get_callback(grabber), systime, rate);

    return hr;
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

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

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

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

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

static HRESULT WINAPI sample_grabber_getservice_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFGetService(iface);
    return IMFMediaSink_QueryInterface(&grabber->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_getservice_AddRef(IMFGetService *iface)
{
    struct sample_grabber *grabber = impl_from_IMFGetService(iface);
    return IMFMediaSink_AddRef(&grabber->IMFMediaSink_iface);
}

static ULONG WINAPI sample_grabber_getservice_Release(IMFGetService *iface)
{
    struct sample_grabber *grabber = impl_from_IMFGetService(iface);
    return IMFMediaSink_Release(&grabber->IMFMediaSink_iface);
}

static HRESULT WINAPI sample_grabber_getservice_GetService(IMFGetService *iface, REFGUID service,
        REFIID riid, void **obj)
{
    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(service, &MF_RATE_CONTROL_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFRateSupport))
            return IMFGetService_QueryInterface(iface, riid, obj);

        return E_NOINTERFACE;
    }

    FIXME("Unsupported service %s, riid %s.\n", debugstr_guid(service), debugstr_guid(riid));

    return MF_E_UNSUPPORTED_SERVICE;
}

static const IMFGetServiceVtbl sample_grabber_getservice_vtbl =
{
    sample_grabber_getservice_QueryInterface,
    sample_grabber_getservice_AddRef,
    sample_grabber_getservice_Release,
    sample_grabber_getservice_GetService,
};

static HRESULT WINAPI sample_grabber_rate_support_QueryInterface(IMFRateSupport *iface, REFIID riid, void **obj)
{
    struct sample_grabber *grabber = impl_from_IMFRateSupport(iface);
    return IMFMediaSink_QueryInterface(&grabber->IMFMediaSink_iface, riid, obj);
}

static ULONG WINAPI sample_grabber_rate_support_AddRef(IMFRateSupport *iface)
{
    struct sample_grabber *grabber = impl_from_IMFRateSupport(iface);
    return IMFMediaSink_AddRef(&grabber->IMFMediaSink_iface);
}

static ULONG WINAPI sample_grabber_rate_support_Release(IMFRateSupport *iface)
{
    struct sample_grabber *grabber = impl_from_IMFRateSupport(iface);
    return IMFMediaSink_Release(&grabber->IMFMediaSink_iface);
}

static HRESULT WINAPI sample_grabber_rate_support_GetSlowestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    TRACE("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    *rate = 0.0f;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_rate_support_GetFastestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction,
        BOOL thin, float *rate)
{
    TRACE("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    *rate = direction == MFRATE_REVERSE ? -FLT_MAX : FLT_MAX;

    return S_OK;
}

static HRESULT WINAPI sample_grabber_rate_support_IsRateSupported(IMFRateSupport *iface, BOOL thin, float rate,
        float *ret_rate)
{
    TRACE("%p, %d, %f, %p.\n", iface, thin, rate, ret_rate);

    if (ret_rate)
        *ret_rate = rate;

    return S_OK;
}

static const IMFRateSupportVtbl sample_grabber_rate_support_vtbl =
{
    sample_grabber_rate_support_QueryInterface,
    sample_grabber_rate_support_AddRef,
    sample_grabber_rate_support_Release,
    sample_grabber_rate_support_GetSlowestRate,
    sample_grabber_rate_support_GetFastestRate,
    sample_grabber_rate_support_IsRateSupported,
};

static HRESULT sample_grabber_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    struct sample_grabber_activate_context *context = user_context;
    struct sample_grabber *object;
    HRESULT hr;
    GUID guid;

    TRACE("%p, %p, %p.\n", attributes, user_context, obj);

    if (context->shut_down)
        return MF_E_SHUTDOWN;

    /* At least major type is required. */
    if (FAILED(IMFMediaType_GetMajorType(context->media_type, &guid)))
        return MF_E_INVALIDMEDIATYPE;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaSink_iface.lpVtbl = &sample_grabber_sink_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &sample_grabber_clock_sink_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &sample_grabber_sink_events_vtbl;
    object->IMFGetService_iface.lpVtbl = &sample_grabber_getservice_vtbl;
    object->IMFRateSupport_iface.lpVtbl = &sample_grabber_rate_support_vtbl;
    object->IMFStreamSink_iface.lpVtbl = &sample_grabber_stream_vtbl;
    object->IMFMediaTypeHandler_iface.lpVtbl = &sample_grabber_stream_type_handler_vtbl;
    object->timer_callback.lpVtbl = &sample_grabber_stream_timer_callback_vtbl;
    object->sample_count = MAX_SAMPLE_QUEUE_LENGTH;
    object->refcount = 1;
    object->rate = 1.0f;
    if (FAILED(IMFSampleGrabberSinkCallback_QueryInterface(context->callback, &IID_IMFSampleGrabberSinkCallback2,
            (void **)&object->callback2)))
    {
        object->callback = context->callback;
        IMFSampleGrabberSinkCallback_AddRef(object->callback);
    }
    object->media_type = context->media_type;
    IMFMediaType_AddRef(object->media_type);
    object->current_media_type = context->media_type;
    IMFMediaType_AddRef(object->current_media_type);
    IMFAttributes_GetUINT32(attributes, &MF_SAMPLEGRABBERSINK_IGNORE_CLOCK, &object->ignore_clock);
    IMFAttributes_GetUINT64(attributes, &MF_SAMPLEGRABBERSINK_SAMPLE_TIME_OFFSET, &object->sample_time_offset);
    list_init(&object->items);
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->stream_event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateAttributes(&object->sample_attributes, 0)))
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

static void sample_grabber_shutdown_object(void *user_context, IUnknown *obj)
{
    struct sample_grabber_activate_context *context = user_context;
    context->shut_down = TRUE;
}

static const struct activate_funcs sample_grabber_activate_funcs =
{
    sample_grabber_create_object,
    sample_grabber_shutdown_object,
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

    if (!(context = calloc(1, sizeof(*context))))
        return E_OUTOFMEMORY;

    context->media_type = media_type;
    IMFMediaType_AddRef(context->media_type);
    context->callback = callback;
    IMFSampleGrabberSinkCallback_AddRef(context->callback);

    if (FAILED(hr = create_activation_object(context, &sample_grabber_activate_funcs, activate)))
        sample_grabber_free_private(context);

    return hr;
}
