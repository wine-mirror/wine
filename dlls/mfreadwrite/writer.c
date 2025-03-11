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

#include <assert.h>

#define COBJMACROS
#include "mfapi.h"
#include "mfidl.h"
#include "mfreadwrite.h"
#include "pathcch.h"
#include "wine/mfinternal.h"
#include "mf_private.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum writer_state
{
    SINK_WRITER_STATE_INITIAL = 0,
    SINK_WRITER_STATE_WRITING,
};

struct marker_context
{
    IUnknown IUnknown_iface;
    LONG refcount;

    unsigned int marker_type;
    void *user_context;
};

struct pending_item
{
    struct list entry;
    unsigned int marker_type;
    IMFSample *sample;
    LONGLONG timestamp;
    void *user_context;
};

struct stream
{
    IMFStreamSink *stream_sink;
    IMFTransform *encoder;
    MF_SINK_WRITER_STATISTICS stats;
    struct list queue;
};

struct sink_writer
{
    IMFSinkWriter IMFSinkWriter_iface;
    IMFAsyncCallback events_callback;
    LONG refcount;

    struct
    {
        struct stream *items;
        size_t count;
        size_t capacity;
        DWORD next_id;
    } streams;

    IMFPresentationClock *clock;
    IMFMediaSink *sink;
    enum writer_state state;
    HRESULT status;
    MF_SINK_WRITER_STATISTICS stats;

    IMFAttributes *attributes;
    IMFSinkWriterCallback *callback;

    CRITICAL_SECTION cs;
};

static struct sink_writer *impl_from_IMFSinkWriter(IMFSinkWriter *iface)
{
    return CONTAINING_RECORD(iface, struct sink_writer, IMFSinkWriter_iface);
}

static struct sink_writer *impl_from_events_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sink_writer, events_callback);
}

static struct marker_context *impl_from_marker_context_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct marker_context, IUnknown_iface);
}

static HRESULT WINAPI marker_context_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI marker_context_AddRef(IUnknown *iface)
{
    struct marker_context *context = impl_from_marker_context_IUnknown(iface);
    return InterlockedIncrement(&context->refcount);
}

static ULONG WINAPI marker_context_Release(IUnknown *iface)
{
    struct marker_context *context = impl_from_marker_context_IUnknown(iface);
    LONG refcount = InterlockedDecrement(&context->refcount);

    if (!refcount)
        free(context);

    return refcount;
}

static const IUnknownVtbl marker_context_vtbl =
{
    marker_context_QueryInterface,
    marker_context_AddRef,
    marker_context_Release,
};

static struct marker_context *unsafe_impl_from_marker_context_IUnknown(IUnknown *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &marker_context_vtbl);
    return CONTAINING_RECORD(iface, struct marker_context, IUnknown_iface);
}

static HRESULT create_marker_context(unsigned int marker_type, void *user_context,
        IUnknown **ret)
{
    struct marker_context *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IUnknown_iface.lpVtbl = &marker_context_vtbl;
    object->refcount = 1;
    object->marker_type = marker_type;
    object->user_context = user_context;

    *ret = &object->IUnknown_iface;

    return S_OK;
}

static void sink_writer_release_pending_item(struct pending_item *item)
{
    list_remove(&item->entry);
    if (item->sample)
        IMFSample_Release(item->sample);
    free(item);
}

static HRESULT WINAPI sink_writer_QueryInterface(IMFSinkWriter *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFSinkWriter) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSinkWriter_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_writer_AddRef(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    ULONG refcount = InterlockedIncrement(&writer->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    return refcount;
}

static void sink_writer_drop_pending_items(struct stream *stream)
{
    struct pending_item *item, *next;

    LIST_FOR_EACH_ENTRY_SAFE(item, next, &stream->queue, struct pending_item, entry)
    {
        sink_writer_release_pending_item(item);
    }
}

static ULONG WINAPI sink_writer_Release(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    ULONG refcount = InterlockedDecrement(&writer->refcount);
    unsigned int i;

    TRACE("%p, %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (writer->clock)
            IMFPresentationClock_Release(writer->clock);
        if (writer->sink)
            IMFMediaSink_Release(writer->sink);
        if (writer->attributes)
            IMFAttributes_Release(writer->attributes);
        if (writer->callback)
            IMFSinkWriterCallback_Release(writer->callback);
        for (i = 0; i < writer->streams.count; ++i)
        {
            struct stream *stream = &writer->streams.items[i];

            if (stream->stream_sink)
                IMFStreamSink_Release(stream->stream_sink);
            if (stream->encoder)
                IMFTransform_Release(stream->encoder);
            sink_writer_drop_pending_items(stream);
        }
        DeleteCriticalSection(&writer->cs);
        free(writer);
    }

    return refcount;
}

static HRESULT sink_writer_add_stream(struct sink_writer *writer, IMFStreamSink *stream_sink, DWORD *index)
{
    struct stream *stream;
    DWORD id = 0;
    HRESULT hr;

    if (!mf_array_reserve((void **)&writer->streams.items, &writer->streams.capacity, writer->streams.count + 1,
            sizeof(*writer->streams.items)))
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = IMFStreamSink_GetIdentifier(stream_sink, &id))) return hr;

    *index = writer->streams.count++;

    stream = &writer->streams.items[*index];
    memset(stream, 0, sizeof(*stream));
    stream->stream_sink = stream_sink;
    IMFStreamSink_AddRef(stream_sink);
    stream->stats.cb = sizeof(stream->stats);
    list_init(&stream->queue);

    writer->streams.next_id = max(writer->streams.next_id, id);

    return hr;
}

static HRESULT WINAPI sink_writer_AddStream(IMFSinkWriter *iface, IMFMediaType *media_type, DWORD *index)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    HRESULT hr = MF_E_INVALIDREQUEST;
    IMFStreamSink *stream_sink;
    DWORD id;

    TRACE("%p, %p, %p.\n", iface, media_type, index);

    if (!media_type)
        return E_INVALIDARG;

    if (!index)
        return E_POINTER;

    EnterCriticalSection(&writer->cs);

    if (writer->state == SINK_WRITER_STATE_INITIAL)
    {
        id = writer->streams.next_id + 1;
        if (SUCCEEDED(hr = IMFMediaSink_AddStreamSink(writer->sink, id, media_type, &stream_sink)))
        {
            if (FAILED(hr = sink_writer_add_stream(writer, stream_sink, index)))
                IMFMediaSink_RemoveStreamSink(writer->sink, id);
        }
    }

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static HRESULT WINAPI sink_writer_SetInputMediaType(IMFSinkWriter *iface, DWORD index, IMFMediaType *type,
        IMFAttributes *parameters)
{
    FIXME("%p, %lu, %p, %p.\n", iface, index, type, parameters);

    return E_NOTIMPL;
}

static HRESULT sink_writer_set_presentation_clock(struct sink_writer *writer)
{
    IMFPresentationTimeSource *time_source = NULL;
    HRESULT hr;

    if (FAILED(hr = MFCreatePresentationClock(&writer->clock))) return hr;

    if (FAILED(IMFMediaSink_QueryInterface(writer->sink, &IID_IMFPresentationTimeSource, (void **)&time_source)))
        hr = MFCreateSystemTimeSource(&time_source);

    if (SUCCEEDED(hr = IMFPresentationClock_SetTimeSource(writer->clock, time_source)))
        hr = IMFMediaSink_SetPresentationClock(writer->sink, writer->clock);

    if (time_source)
        IMFPresentationTimeSource_Release(time_source);

    return hr;
}

static HRESULT WINAPI sink_writer_BeginWriting(IMFSinkWriter *iface)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    HRESULT hr = S_OK;
    unsigned int i;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&writer->cs);

    if (!writer->streams.count)
        hr = MF_E_INVALIDREQUEST;
    else if (writer->state != SINK_WRITER_STATE_INITIAL)
        hr = MF_E_INVALIDREQUEST;
    else if (SUCCEEDED(hr = sink_writer_set_presentation_clock(writer)))
    {
        for (i = 0; i < writer->streams.count; ++i)
        {
            struct stream *stream = &writer->streams.items[i];

            if (FAILED(hr = IMFStreamSink_BeginGetEvent(stream->stream_sink, &writer->events_callback,
                    (IUnknown *)stream->stream_sink)))
            {
                WARN("Failed to subscribe to events for steam %u, hr %#lx.\n", i, hr);
            }

            if (stream->encoder)
                IMFTransform_ProcessMessage(stream->encoder, MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
        }

        if (SUCCEEDED(hr))
            hr = IMFPresentationClock_Start(writer->clock, 0);

        writer->state = SINK_WRITER_STATE_WRITING;
    }

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static struct stream * sink_writer_get_stream(const struct sink_writer *writer, DWORD index)
{
    if (index >= writer->streams.count) return NULL;
    return &writer->streams.items[index];
}

static HRESULT sink_writer_get_buffer_length(IMFSample *sample, LONGLONG *timestamp, DWORD *length)
{
    IMFMediaBuffer *buffer;
    HRESULT hr;

    *timestamp = 0;
    *length = 0;

    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer))) return hr;
    hr = IMFMediaBuffer_GetCurrentLength(buffer, length);
    IMFMediaBuffer_Release(buffer);
    if (FAILED(hr)) return hr;
    if (!*length) return E_INVALIDARG;
    IMFSample_GetSampleTime(sample, timestamp);

    return hr;
}

static HRESULT sink_writer_place_marker(struct sink_writer *writer, struct stream *stream, unsigned int marker_type,
        LONGLONG timestamp, void *user_context)
{
    PROPVARIANT value, context;
    IUnknown *context_obj;
    HRESULT hr;

    if (FAILED(hr = create_marker_context(marker_type, user_context, &context_obj))) return hr;

    context.vt = VT_UNKNOWN;
    context.punkVal = context_obj;
    value.vt = VT_I8;
    value.hVal.QuadPart = timestamp;
    hr = IMFStreamSink_PlaceMarker(stream->stream_sink, marker_type, marker_type == MFSTREAMSINK_MARKER_TICK ? &value : NULL,
            &context);
    IUnknown_Release(context_obj);
    return hr;
}

static HRESULT sink_writer_queue_marker(struct sink_writer *writer, struct stream *stream, unsigned int marker_type,
        LONGLONG timestamp, void *user_context)
{
    struct pending_item *item;

    if (list_empty(&stream->queue))
        return sink_writer_place_marker(writer, stream, marker_type, timestamp, user_context);

    if (!(item = calloc(1, sizeof(*item))))
        return E_OUTOFMEMORY;

    item->marker_type = marker_type;
    item->timestamp = timestamp;
    list_add_tail(&stream->queue, &item->entry);

    return S_OK;
}

static HRESULT sink_writer_send_stream_tick(struct sink_writer *writer, unsigned int index, LONGLONG timestamp)
{
    struct stream *stream;

    if (!(stream = sink_writer_get_stream(writer, index))) return MF_E_INVALIDSTREAMNUMBER;

    writer->stats.llLastStreamTickReceived = timestamp;
    writer->stats.qwNumStreamTicksReceived++;
    stream->stats.llLastStreamTickReceived = timestamp;
    stream->stats.qwNumStreamTicksReceived++;

    return sink_writer_queue_marker(writer, stream, MFSTREAMSINK_MARKER_TICK, timestamp, NULL);
}

static HRESULT sink_writer_notify_end_of_segment(struct sink_writer *writer, unsigned int index)
{
    struct stream *stream;

    if (!(stream = sink_writer_get_stream(writer, index))) return MF_E_INVALIDSTREAMNUMBER;

    return sink_writer_queue_marker(writer, stream, MFSTREAMSINK_MARKER_ENDOFSEGMENT, 0, NULL);
}

static HRESULT sink_writer_flush(struct sink_writer *writer, unsigned int index)
{
    struct stream *stream;

    if (!(stream = sink_writer_get_stream(writer, index))) return MF_E_INVALIDSTREAMNUMBER;

    sink_writer_drop_pending_items(stream);

    IMFStreamSink_Flush(stream->stream_sink);

    if (stream->encoder)
        IMFTransform_ProcessMessage(stream->encoder, MFT_MESSAGE_COMMAND_FLUSH, 0);

    return sink_writer_place_marker(writer, stream, MFSTREAMSINK_MARKER_ENDOFSEGMENT, 0, NULL);
}

static HRESULT sink_writer_process_sample(struct sink_writer *writer, struct stream *stream)
{
    struct pending_item *item, *next;
    LONGLONG timestamp;
    IMFSample *sample;
    HRESULT hr;

    if (list_empty(&stream->queue)) return S_OK;
    item = LIST_ENTRY(list_head(&stream->queue), struct pending_item, entry);
    if (!item->sample) return S_OK;

    IMFSample_AddRef((sample = item->sample));
    sink_writer_release_pending_item(item);

    writer->stats.dwNumOutstandingSinkSampleRequests--;
    stream->stats.dwNumOutstandingSinkSampleRequests--;

    if (FAILED(hr = IMFSample_GetSampleTime(sample, &timestamp)))
    {
        IMFSample_Release(sample);
        writer->status = hr;
        return S_OK;
    }

    writer->stats.llLastTimestampProcessed = timestamp;
    stream->stats.llLastTimestampProcessed = timestamp;

    hr = IMFStreamSink_ProcessSample(stream->stream_sink, sample);
    IMFSample_Release(sample);
    if (FAILED(hr)) return hr;

    LIST_FOR_EACH_ENTRY_SAFE(item, next, &stream->queue, struct pending_item, entry)
    {
        if (item->sample) break;
        sink_writer_place_marker(writer, stream, item->marker_type, item->timestamp, item->user_context);
        sink_writer_release_pending_item(item);
    }

    return hr;
}

static HRESULT sink_writer_encode_sample(struct sink_writer *writer, struct stream *stream, IMFSample *sample)
{
    struct pending_item *item;

    /* FIXME: call the encoder, queue its output */

    if (!(item = calloc(1, sizeof(*item))))
        return E_OUTOFMEMORY;

    item->sample = sample;
    IMFSample_AddRef(item->sample);
    list_add_tail(&stream->queue, &item->entry);

    return S_OK;
}

static HRESULT sink_writer_write_sample(struct sink_writer *writer, struct stream *stream, IMFSample *sample)
{
    LONGLONG timestamp;
    DWORD length;
    HRESULT hr;

    if (FAILED(hr = sink_writer_get_buffer_length(sample, &timestamp, &length))) return hr;

    stream->stats.llLastTimestampReceived = timestamp;
    stream->stats.qwNumSamplesReceived++;
    stream->stats.dwByteCountQueued += length;

    writer->stats.llLastTimestampReceived = timestamp;
    writer->stats.qwNumSamplesReceived++;
    writer->stats.dwByteCountQueued += length;

    if (FAILED(hr = sink_writer_encode_sample(writer, stream, sample))) return hr;

    if (stream->stats.dwNumOutstandingSinkSampleRequests)
        hr = sink_writer_process_sample(writer, stream);

    return hr;
}

static HRESULT WINAPI sink_writer_WriteSample(IMFSinkWriter *iface, DWORD index, IMFSample *sample)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    struct stream *stream;
    LONGLONG timestamp;
    DWORD length;
    HRESULT hr;

    TRACE("%p, %lu, %p.\n", iface, index, sample);

    if (!sample)
        return E_INVALIDARG;

    EnterCriticalSection(&writer->cs);

    if (writer->state != SINK_WRITER_STATE_WRITING)
        hr = MF_E_INVALIDREQUEST;
    else if (!(stream = sink_writer_get_stream(writer, index)))
    {
        hr = MF_E_INVALIDSTREAMNUMBER;
    }
    else if (SUCCEEDED(hr = sink_writer_get_buffer_length(sample, &timestamp, &length)))
    {
        stream->stats.llLastTimestampReceived = timestamp;
        stream->stats.qwNumSamplesReceived++;
        stream->stats.dwByteCountQueued += length;

        writer->stats.llLastTimestampReceived = timestamp;
        writer->stats.qwNumSamplesReceived++;
        writer->stats.dwByteCountQueued += length;

        hr = sink_writer_write_sample(writer, stream, sample);
    }

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static HRESULT WINAPI sink_writer_SendStreamTick(IMFSinkWriter *iface, DWORD index, LONGLONG timestamp)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    HRESULT hr;

    TRACE("%p, %lu, %s.\n", iface, index, wine_dbgstr_longlong(timestamp));

    EnterCriticalSection(&writer->cs);

    if (writer->state != SINK_WRITER_STATE_WRITING)
        hr = MF_E_INVALIDREQUEST;
    else
        hr = sink_writer_send_stream_tick(writer, index, timestamp);

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static HRESULT WINAPI sink_writer_PlaceMarker(IMFSinkWriter *iface, DWORD index, void *user_context)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    struct stream *stream;
    HRESULT hr;

    TRACE("%p, %lu, %p.\n", iface, index, user_context);

    EnterCriticalSection(&writer->cs);

    if (!writer->callback)
        hr = MF_E_INVALIDREQUEST;
    else if (writer->state != SINK_WRITER_STATE_WRITING)
        hr = MF_E_INVALIDREQUEST;
    else if (!(stream = sink_writer_get_stream(writer, index)))
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
        hr = sink_writer_queue_marker(writer, stream, MFSTREAMSINK_MARKER_DEFAULT, 0, user_context);

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static HRESULT WINAPI sink_writer_NotifyEndOfSegment(IMFSinkWriter *iface, DWORD index)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    HRESULT hr = S_OK;
    unsigned int i;

    TRACE("%p, %lu.\n", iface, index);

    EnterCriticalSection(&writer->cs);

    if (writer->state != SINK_WRITER_STATE_WRITING)
        hr = MF_E_INVALIDREQUEST;
    else if (index == MF_SINK_WRITER_ALL_STREAMS)
    {
        for (i = 0; i < writer->streams.count; ++i)
        {
            if (FAILED(hr = sink_writer_notify_end_of_segment(writer, index)))
            {
                WARN("Failed to place a marker for stream %u.\n", i);
                break;
            }
        }
    }
    else
        hr = sink_writer_notify_end_of_segment(writer, index);

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static HRESULT WINAPI sink_writer_Flush(IMFSinkWriter *iface, DWORD index)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    HRESULT hr = S_OK;
    unsigned int i;

    TRACE("%p, %lu.\n", iface, index);

    EnterCriticalSection(&writer->cs);

    if (writer->state != SINK_WRITER_STATE_WRITING)
        hr = MF_E_INVALIDREQUEST;
    else if (index == MF_SINK_WRITER_ALL_STREAMS)
    {
        for (i = 0; i < writer->streams.count; ++i)
        {
            if (FAILED(hr = sink_writer_flush(writer, i)))
            {
                WARN("Failed to flush stream %u.\n", i);
                break;
            }
        }
    }
    else
        hr = sink_writer_flush(writer, index);

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static HRESULT WINAPI sink_writer_Finalize(IMFSinkWriter *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT sink_writer_get_service(void *object, REFGUID service, REFIID riid, void **ret)
{
    IUnknown *iface = object;
    IMFGetService *gs;
    HRESULT hr;

    if (!iface) return MF_E_UNSUPPORTED_SERVICE;

    if (IsEqualGUID(service, &GUID_NULL))
        return IUnknown_QueryInterface(iface, riid, ret);

    if (FAILED(hr = IUnknown_QueryInterface(iface, &IID_IMFGetService, (void **)&gs)))
        return hr;

    hr = IMFGetService_GetService(gs, service, riid, ret);
    IMFGetService_Release(gs);
    return hr;
}

static HRESULT WINAPI sink_writer_GetServiceForStream(IMFSinkWriter *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    HRESULT hr = E_UNEXPECTED;
    struct stream *stream;

    TRACE("%p, %lu, %s, %s, %p.\n", iface, index, debugstr_guid(service), debugstr_guid(riid), object);

    EnterCriticalSection(&writer->cs);

    if (index == MF_SINK_WRITER_MEDIASINK)
        hr = sink_writer_get_service(writer->sink, service, riid, object);
    else if ((stream = sink_writer_get_stream(writer, index)))
    {
        if (stream->encoder)
            hr = sink_writer_get_service(stream->encoder, service, riid, object);
        if (FAILED(hr))
            hr = sink_writer_get_service(stream->stream_sink, service, riid, object);
    }
    else
        hr = MF_E_INVALIDSTREAMNUMBER;

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static HRESULT WINAPI sink_writer_GetStatistics(IMFSinkWriter *iface, DWORD index, MF_SINK_WRITER_STATISTICS *stats)
{
    struct sink_writer *writer = impl_from_IMFSinkWriter(iface);
    struct stream *stream;
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, stats);

    if (!stats)
        return E_POINTER;

    if (stats->cb != sizeof(*stats))
        return E_INVALIDARG;

    EnterCriticalSection(&writer->cs);

    if (FAILED(writer->status))
        hr = writer->status;
    else if (index == MF_SINK_WRITER_ALL_STREAMS)
        *stats = writer->stats;
    else if ((stream = sink_writer_get_stream(writer, index)))
        *stats = stream->stats;
    else
        hr = MF_E_INVALIDSTREAMNUMBER;

    LeaveCriticalSection(&writer->cs);

    return hr;
}

static const IMFSinkWriterVtbl sink_writer_vtbl =
{
    sink_writer_QueryInterface,
    sink_writer_AddRef,
    sink_writer_Release,
    sink_writer_AddStream,
    sink_writer_SetInputMediaType,
    sink_writer_BeginWriting,
    sink_writer_WriteSample,
    sink_writer_SendStreamTick,
    sink_writer_PlaceMarker,
    sink_writer_NotifyEndOfSegment,
    sink_writer_Flush,
    sink_writer_Finalize,
    sink_writer_GetServiceForStream,
    sink_writer_GetStatistics,
};

static HRESULT WINAPI sink_writer_callback_QueryInterface(IMFAsyncCallback *iface,
        REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_writer_events_callback_AddRef(IMFAsyncCallback *iface)
{
    struct sink_writer *writer = impl_from_events_callback_IMFAsyncCallback(iface);
    return IMFSinkWriter_AddRef(&writer->IMFSinkWriter_iface);
}

static ULONG WINAPI sink_writer_events_callback_Release(IMFAsyncCallback *iface)
{
    struct sink_writer *writer = impl_from_events_callback_IMFAsyncCallback(iface);
    return IMFSinkWriter_Release(&writer->IMFSinkWriter_iface);
}

static HRESULT WINAPI sink_writer_callback_GetParameters(IMFAsyncCallback *iface,
        DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static struct stream *sink_writer_get_stream_for_stream_sink(struct sink_writer *writer,
        IMFStreamSink *stream_sink, DWORD *index)
{
    unsigned int i;

    for (i = 0; i < writer->streams.count; ++i)
    {
        if (writer->streams.items[i].stream_sink == stream_sink)
        {
            *index = i;
            return &writer->streams.items[i];
        }
    }

    return NULL;
}

static HRESULT WINAPI sink_writer_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct sink_writer *writer = impl_from_events_callback_IMFAsyncCallback(iface);
    struct marker_context *context;
    IMFStreamSink *stream_sink;
    MediaEventType event_type;
    struct stream *stream;
    IMFMediaEvent *event;
    LONGLONG timestamp;
    HRESULT status, hr;
    PROPVARIANT value;
    DWORD index;

    TRACE("%p, %p.\n", iface, result);

    stream_sink = (IMFStreamSink *)IMFAsyncResult_GetStateNoAddRef(result);

    if (FAILED(hr = IMFStreamSink_EndGetEvent(stream_sink, result, &event)))
        return hr;

    IMFMediaEvent_GetType(event, &event_type);
    IMFMediaEvent_GetStatus(event, &status);

    TRACE("Got event %lu.\n", event_type);

    PropVariantInit(&value);

    EnterCriticalSection(&writer->cs);

    if (writer->status == S_OK && FAILED(status))
        writer->status = status;

    if (writer->status == S_OK && (stream = sink_writer_get_stream_for_stream_sink(writer, stream_sink, &index)))
    {
        switch (event_type)
        {
            case MEStreamSinkRequestSample:

                timestamp = MFGetSystemTime();

                writer->stats.llLastSinkSampleRequest = timestamp;
                writer->stats.dwNumOutstandingSinkSampleRequests++;
                stream->stats.llLastSinkSampleRequest = timestamp;
                stream->stats.dwNumOutstandingSinkSampleRequests++;

                sink_writer_process_sample(writer, stream);

                break;

            case MEStreamSinkMarker:
                if (FAILED(hr = IMFMediaEvent_GetValue(event, &value))) break;
                if (value.vt != VT_UNKNOWN || !(context = unsafe_impl_from_marker_context_IUnknown(value.punkVal))) break;

                /* This relies on the fact that default marker type is only used for PlaceMarker(). */
                if (context->marker_type == MFSTREAMSINK_MARKER_DEFAULT)
                    IMFSinkWriterCallback_OnMarker(writer->callback, index, context->user_context);

                break;

            default:
                ;
        }
    }

    LeaveCriticalSection(&writer->cs);

    PropVariantClear(&value);

    IMFMediaEvent_Release(event);

    IMFStreamSink_BeginGetEvent(stream_sink, iface, (IUnknown *)stream_sink);

    return S_OK;
}

static const IMFAsyncCallbackVtbl sink_writer_events_callback_vtbl =
{
    sink_writer_callback_QueryInterface,
    sink_writer_events_callback_AddRef,
    sink_writer_events_callback_Release,
    sink_writer_callback_GetParameters,
    sink_writer_events_callback_Invoke,
};

static HRESULT sink_writer_initialize_existing_streams(struct sink_writer *writer, IMFMediaSink *sink)
{
    IMFStreamSink *stream_sink;
    DWORD count = 0, i, index;
    HRESULT hr;

    if (FAILED(hr = IMFMediaSink_GetStreamSinkCount(sink, &count))) return hr;
    if (!count) return S_OK;

    for (i = 0; i < count; ++i)
    {
        if (FAILED(hr = IMFMediaSink_GetStreamSinkByIndex(sink, i, &stream_sink))) break;
        hr = sink_writer_add_stream(writer, stream_sink, &index);
        IMFStreamSink_Release(stream_sink);
        if (FAILED(hr)) break;
    }

    return hr;
}

HRESULT create_sink_writer_from_sink(IMFMediaSink *sink, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    struct sink_writer *object;
    HRESULT hr;

    *out = NULL;

    if (!sink)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFSinkWriter_iface.lpVtbl = &sink_writer_vtbl;
    object->events_callback.lpVtbl = &sink_writer_events_callback_vtbl;
    object->refcount = 1;
    object->sink = sink;
    IMFMediaSink_AddRef(sink);
    object->stats.cb = sizeof(object->stats);
    InitializeCriticalSection(&object->cs);

    if (attributes)
    {
        IMFAttributes_AddRef((object->attributes = attributes));
        IMFAttributes_GetUnknown(attributes, &MF_SINK_WRITER_ASYNC_CALLBACK,
                &IID_IMFSinkWriterCallback, (void **)&object->callback);
    }

    if (FAILED(hr = sink_writer_initialize_existing_streams(object, sink)))
    {
        IMFSinkWriter_Release(&object->IMFSinkWriter_iface);
        return hr;
    }

    hr = IMFSinkWriter_QueryInterface(&object->IMFSinkWriter_iface, riid, out);
    IMFSinkWriter_Release(&object->IMFSinkWriter_iface);
    return hr;
}

/***********************************************************************
 *      MFCreateSinkWriterFromMediaSink (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSinkWriterFromMediaSink(IMFMediaSink *sink, IMFAttributes *attributes, IMFSinkWriter **writer)
{
    TRACE("%p, %p, %p.\n", sink, attributes, writer);

    if (!writer)
        return E_INVALIDARG;

    return create_sink_writer_from_sink(sink, attributes, &IID_IMFSinkWriter, (void **)writer);
}

static HRESULT sink_writer_get_sink_factory_class(const WCHAR *url, IMFAttributes *attributes, CLSID *clsid)
{
    static const struct extension_map
    {
        const WCHAR *ext;
        const GUID *guid;
    } ext_map[] =
    {
        { L".mp4", &MFTranscodeContainerType_MPEG4 },
        { L".mp3", &MFTranscodeContainerType_MP3 },
        { L".wav", &MFTranscodeContainerType_WAVE },
        { L".avi", &MFTranscodeContainerType_AVI },
    };
    static const struct
    {
        const GUID *container;
        const CLSID *clsid;
    } class_map[] =
    {
        { &MFTranscodeContainerType_MPEG4, &CLSID_MFMPEG4SinkClassFactory },
        { &MFTranscodeContainerType_MP3, &CLSID_MFMP3SinkClassFactory },
        { &MFTranscodeContainerType_WAVE, &CLSID_MFWAVESinkClassFactory },
        { &MFTranscodeContainerType_AVI, &CLSID_MFAVISinkClassFactory },
    };
    const WCHAR *extension;
    GUID container;
    unsigned int i;

    if (url)
    {
        if (!attributes || FAILED(IMFAttributes_GetGUID(attributes, &MF_TRANSCODE_CONTAINERTYPE, &container)))
        {
            const struct extension_map *map = NULL;

            if (FAILED(PathCchFindExtension(url, PATHCCH_MAX_CCH, &extension))) return E_INVALIDARG;
            if (!extension || !*extension) return E_INVALIDARG;

            for (i = 0; i < ARRAY_SIZE(ext_map); ++i)
            {
                map = &ext_map[i];

                if (!wcsicmp(map->ext, extension))
                    break;
            }

            if (!map)
            {
                WARN("Couldn't find container type for extension %s.\n", debugstr_w(extension));
                return E_INVALIDARG;
            }

            container = *map->guid;
        }
    }
    else
    {
        if (!attributes) return E_INVALIDARG;
        if (FAILED(IMFAttributes_GetGUID(attributes, &MF_TRANSCODE_CONTAINERTYPE, &container))) return E_INVALIDARG;
    }

    for (i = 0; i < ARRAY_SIZE(class_map); ++i)
    {
        if (IsEqualGUID(&container, class_map[i].container))
        {
            *clsid = *class_map[i].clsid;
            return S_OK;
        }
    }

    WARN("Couldn't find factory class for container %s.\n", debugstr_guid(&container));
    return E_INVALIDARG;
}

HRESULT create_sink_writer_from_url(const WCHAR *url, IMFByteStream *bytestream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    IMFSinkClassFactory *factory;
    IMFMediaSink *sink;
    CLSID clsid;
    HRESULT hr;

    *out = NULL;

    if (!url && !bytestream)
        return E_INVALIDARG;

    if (FAILED(hr = sink_writer_get_sink_factory_class(url, attributes, &clsid))) return hr;

    if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFSinkClassFactory, (void **)&factory)))
    {
        WARN("Failed to create a sink factory, hr %#lx.\n", hr);
        return hr;
    }

    if (bytestream)
        IMFByteStream_AddRef(bytestream);
    else if (FAILED(hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_DELETE_IF_EXIST, 0, url, &bytestream)))
    {
        WARN("Failed to create output file stream, hr %#lx.\n", hr);
        IMFSinkClassFactory_Release(factory);
        return hr;
    }

    hr = IMFSinkClassFactory_CreateMediaSink(factory, bytestream, NULL, NULL, &sink);
    IMFSinkClassFactory_Release(factory);
    IMFByteStream_Release(bytestream);
    if (FAILED(hr))
    {
        WARN("Failed to create a sink, hr %#lx.\n", hr);
        return hr;
    }

    hr = create_sink_writer_from_sink(sink, attributes, riid, out);
    IMFMediaSink_Release(sink);

    return hr;
}

/***********************************************************************
 *      MFCreateSinkWriterFromURL (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSinkWriterFromURL(const WCHAR *url, IMFByteStream *bytestream, IMFAttributes *attributes,
        IMFSinkWriter **writer)
{
    TRACE("%s, %p, %p, %p.\n", debugstr_w(url), bytestream, attributes, writer);

    if (!writer)
        return E_INVALIDARG;

    return create_sink_writer_from_url(url, bytestream, attributes, &IID_IMFSinkWriter, (void **)writer);
}
