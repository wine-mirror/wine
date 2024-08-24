/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "mfsrcsnk_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum wave_sink_flags
{
    SINK_SHUT_DOWN = 0x1,
    SINK_HEADER_WRITTEN = 0x2,
    SINK_DATA_CHUNK_STARTED = 0x4,
    SINK_DATA_FINALIZED = 0x8,
};

struct wave_sink
{
    IMFFinalizableMediaSink IMFFinalizableMediaSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    IMFStreamSink IMFStreamSink_iface;
    LONG refcount;

    IMFMediaEventQueue *event_queue;
    IMFMediaEventQueue *stream_event_queue;
    IMFPresentationClock *clock;

    WAVEFORMATEX *fmt;
    IMFByteStream *bytestream;
    QWORD data_size_offset;
    QWORD riff_size_offset;
    DWORD data_length;
    DWORD full_length;

    unsigned int flags;
    CRITICAL_SECTION cs;
};

static struct wave_sink *impl_from_IMFFinalizableMediaSink(IMFFinalizableMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFFinalizableMediaSink_iface);
}

static struct wave_sink *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFMediaEventGenerator_iface);
}

static struct wave_sink *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFStreamSink_iface);
}

static struct wave_sink *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFClockStateSink_iface);
}

static struct wave_sink *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct wave_sink, IMFMediaTypeHandler_iface);
}

static HRESULT WINAPI wave_sink_QueryInterface(IMFFinalizableMediaSink *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFFinalizableMediaSink) ||
            IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &sink->IMFMediaEventGenerator_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &sink->IMFClockStateSink_iface;
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

static ULONG WINAPI wave_sink_AddRef(IMFFinalizableMediaSink *iface)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&sink->refcount);
    TRACE("%p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI wave_sink_Release(IMFFinalizableMediaSink *iface)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&sink->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (sink->event_queue)
            IMFMediaEventQueue_Release(sink->event_queue);
        if (sink->stream_event_queue)
            IMFMediaEventQueue_Release(sink->stream_event_queue);
        IMFByteStream_Release(sink->bytestream);
        CoTaskMemFree(sink->fmt);
        DeleteCriticalSection(&sink->cs);
        free(sink);
    }

    return refcount;
}

static HRESULT WINAPI wave_sink_GetCharacteristics(IMFFinalizableMediaSink *iface, DWORD *flags)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %p.\n", iface, flags);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    *flags = MEDIASINK_FIXED_STREAMS | MEDIASINK_RATELESS;

    return S_OK;
}

static HRESULT WINAPI wave_sink_AddStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %#lx, %p, %p.\n", iface, stream_sink_id, media_type, stream_sink);

    return sink->flags & SINK_SHUT_DOWN ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI wave_sink_RemoveStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %#lx.\n", iface, stream_sink_id);

    return sink->flags & SINK_SHUT_DOWN ? MF_E_SHUTDOWN : MF_E_STREAMSINKS_FIXED;
}

static HRESULT WINAPI wave_sink_GetStreamSinkCount(IMFFinalizableMediaSink *iface, DWORD *count)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_SHUTDOWN;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI wave_sink_GetStreamSinkByIndex(IMFFinalizableMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p.\n", iface, index, stream);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (index)
        hr = MF_E_INVALIDINDEX;
    else
    {
       *stream = &sink->IMFStreamSink_iface;
       IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_GetStreamSinkById(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %#lx, %p.\n", iface, stream_sink_id, stream);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (stream_sink_id != 1)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else
    {
        *stream = &sink->IMFStreamSink_iface;
        IMFStreamSink_AddRef(*stream);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static void wave_sink_set_presentation_clock(struct wave_sink *sink, IMFPresentationClock *clock)
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

static HRESULT WINAPI wave_sink_SetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock *clock)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        wave_sink_set_presentation_clock(sink, clock);

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_GetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock **clock)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, clock);

    if (!clock)
        return E_POINTER;

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (sink->clock)
    {
        *clock = sink->clock;
        IMFPresentationClock_AddRef(*clock);
    }
    else
        hr = MF_E_NO_CLOCK;

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_Shutdown(IMFFinalizableMediaSink *iface)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        sink->flags |= SINK_SHUT_DOWN;
        IMFMediaEventQueue_Shutdown(sink->event_queue);
        IMFMediaEventQueue_Shutdown(sink->stream_event_queue);
        wave_sink_set_presentation_clock(sink, NULL);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static void wave_sink_write_raw(struct wave_sink *sink, const void *data, DWORD length, HRESULT *hr)
{
    DWORD written_length;

    if (FAILED(*hr)) return;
    if (SUCCEEDED(*hr = IMFByteStream_Write(sink->bytestream, data, length, &written_length)))
        sink->full_length += length;
}

static void wave_sink_write_pad(struct wave_sink *sink, DWORD size, HRESULT *hr)
{
    DWORD i, len = size / 4, zero = 0;

    for (i = 0; i < len; ++i)
        wave_sink_write_raw(sink, &zero, 4, hr);
    if ((len = size % 4))
        wave_sink_write_raw(sink, &zero, len, hr);
}

static void wave_sink_write_junk(struct wave_sink *sink, DWORD size, HRESULT *hr)
{
    wave_sink_write_raw(sink, "JUNK", 4, hr);
    wave_sink_write_raw(sink, &size, 4, hr);
    wave_sink_write_pad(sink, size, hr);
}

static HRESULT wave_sink_write_header(struct wave_sink *sink)
{
    HRESULT hr = S_OK;
    DWORD size = 0;

    wave_sink_write_raw(sink, "RIFF", 4, &hr);
    if (SUCCEEDED(hr))
        hr = IMFByteStream_GetCurrentPosition(sink->bytestream, &sink->riff_size_offset);
    wave_sink_write_raw(sink, &size, sizeof(size), &hr);
    wave_sink_write_raw(sink, "WAVE", 4, &hr);
    wave_sink_write_junk(sink, 28, &hr);

    /* Format chunk */
    wave_sink_write_raw(sink, "fmt ", 4, &hr);
    size = sizeof(*sink->fmt);
    wave_sink_write_raw(sink, &size, sizeof(size), &hr);
    wave_sink_write_raw(sink, sink->fmt, size, &hr);

    sink->flags |= SINK_HEADER_WRITTEN;

    return hr;
}

static HRESULT wave_sink_start_data_chunk(struct wave_sink *sink)
{
    HRESULT hr = S_OK;

    wave_sink_write_raw(sink, "data", 4, &hr);
    if (SUCCEEDED(hr))
        hr = IMFByteStream_GetCurrentPosition(sink->bytestream, &sink->data_size_offset);
    wave_sink_write_pad(sink, 4, &hr);
    sink->flags |= SINK_DATA_CHUNK_STARTED;

    return hr;
}

static HRESULT wave_sink_write_data(struct wave_sink *sink, const BYTE *data, DWORD length)
{
    HRESULT hr = S_OK;

    wave_sink_write_raw(sink, data, length, &hr);
    if (SUCCEEDED(hr))
        sink->data_length += length;

    return hr;
}

static void wave_sink_write_at(struct wave_sink *sink, const void *data, DWORD length, QWORD offset, HRESULT *hr)
{
    QWORD position;

    if (FAILED(*hr)) return;

    if (FAILED(*hr = IMFByteStream_GetCurrentPosition(sink->bytestream, &position))) return;
    if (FAILED(*hr = IMFByteStream_SetCurrentPosition(sink->bytestream, offset))) return;
    wave_sink_write_raw(sink, data, length, hr);
    IMFByteStream_SetCurrentPosition(sink->bytestream, position);
}

static HRESULT WINAPI wave_sink_BeginFinalize(IMFFinalizableMediaSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct wave_sink *sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr = S_OK, status;
    IMFAsyncResult *result;
    DWORD size;

    TRACE("%p, %p, %p.\n", iface, callback, state);

    EnterCriticalSection(&sink->cs);

    if (!(sink->flags & SINK_DATA_FINALIZED))
    {
        size = sink->full_length - 8 /* RIFF chunk header size */;
        wave_sink_write_at(sink, &size, 4, sink->riff_size_offset, &hr);
        wave_sink_write_at(sink, &sink->data_length, 4, sink->data_size_offset, &hr);
        sink->flags |= SINK_DATA_FINALIZED;
        status = hr;
    }
    else
        status = E_INVALIDARG;

    if (callback)
    {
        if (SUCCEEDED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        {
            IMFAsyncResult_SetStatus(result, status);
            hr = MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_STANDARD, result);
            IMFAsyncResult_Release(result);
        }
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_sink_EndFinalize(IMFFinalizableMediaSink *iface, IMFAsyncResult *result)
{
    TRACE("%p, %p.\n", iface, result);

    return result ? IMFAsyncResult_GetStatus(result) : E_INVALIDARG;
}

static const IMFFinalizableMediaSinkVtbl wave_sink_vtbl =
{
    wave_sink_QueryInterface,
    wave_sink_AddRef,
    wave_sink_Release,
    wave_sink_GetCharacteristics,
    wave_sink_AddStreamSink,
    wave_sink_RemoveStreamSink,
    wave_sink_GetStreamSinkCount,
    wave_sink_GetStreamSinkByIndex,
    wave_sink_GetStreamSinkById,
    wave_sink_SetPresentationClock,
    wave_sink_GetPresentationClock,
    wave_sink_Shutdown,
    wave_sink_BeginFinalize,
    wave_sink_EndFinalize,
};

static HRESULT WINAPI wave_sink_events_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_QueryInterface(&sink->IMFFinalizableMediaSink_iface, riid, obj);
}

static ULONG WINAPI wave_sink_events_AddRef(IMFMediaEventGenerator *iface)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_AddRef(&sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI wave_sink_events_Release(IMFMediaEventGenerator *iface)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_Release(&sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI wave_sink_events_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(sink->event_queue, flags, event);
}

static HRESULT WINAPI wave_sink_events_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(sink->event_queue, callback, state);
}

static HRESULT WINAPI wave_sink_events_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(sink->event_queue, result, event);
}

static HRESULT WINAPI wave_sink_events_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct wave_sink *sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(sink->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl wave_sink_events_vtbl =
{
    wave_sink_events_QueryInterface,
    wave_sink_events_AddRef,
    wave_sink_events_Release,
    wave_sink_events_GetEvent,
    wave_sink_events_BeginGetEvent,
    wave_sink_events_EndGetEvent,
    wave_sink_events_QueueEvent,
};

static HRESULT WINAPI wave_stream_sink_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

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
    else
    {
        WARN("Unsupported %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI wave_stream_sink_AddRef(IMFStreamSink *iface)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);
    return IMFFinalizableMediaSink_AddRef(&sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI wave_stream_sink_Release(IMFStreamSink *iface)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);
    return IMFFinalizableMediaSink_Release(&sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI wave_stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_GetEvent(sink->stream_event_queue, flags, event);
}

static HRESULT WINAPI wave_stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_BeginGetEvent(sink->stream_event_queue, callback, state);
}

static HRESULT WINAPI wave_stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_EndGetEvent(sink->stream_event_queue, result, event);
}

static HRESULT WINAPI wave_stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI wave_stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **ret)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, ret);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *ret = (IMFMediaSink *)&sink->IMFFinalizableMediaSink_iface;
    IMFMediaSink_AddRef(*ret);

    return S_OK;
}

static HRESULT WINAPI wave_stream_sink_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, identifier);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *identifier = 1;

    return S_OK;
}

static HRESULT WINAPI wave_stream_sink_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);

    TRACE("%p, %p.\n", iface, handler);

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    *handler = &sink->IMFMediaTypeHandler_iface;
    IMFMediaTypeHandler_AddRef(*handler);

    return S_OK;
}

static HRESULT WINAPI wave_stream_sink_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    struct wave_sink *sink = impl_from_IMFStreamSink(iface);
    IMFMediaBuffer *buffer = NULL;
    DWORD max_length, length;
    HRESULT hr = S_OK;
    BYTE *data;

    TRACE("%p, %p.\n", iface, sample);

    EnterCriticalSection(&sink->cs);

    if (sink->flags & SINK_SHUT_DOWN)
        hr = MF_E_STREAMSINK_REMOVED;
    else
    {
        if (!(sink->flags & SINK_HEADER_WRITTEN))
            hr = wave_sink_write_header(sink);

        if (SUCCEEDED(hr) && !(sink->flags & SINK_DATA_CHUNK_STARTED))
            hr = wave_sink_start_data_chunk(sink);

        if (SUCCEEDED(hr))
            hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer);

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr = IMFMediaBuffer_Lock(buffer, &data, &max_length, &length)))
            {
                hr = wave_sink_write_data(sink, data, length);
                IMFMediaBuffer_Unlock(buffer);
            }
        }

        if (buffer)
            IMFMediaBuffer_Release(buffer);
    }

    LeaveCriticalSection(&sink->cs);

    return hr;
}

static HRESULT WINAPI wave_stream_sink_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    FIXME("%p, %d, %p, %p.\n", iface, marker_type, marker_value, context_value);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_stream_sink_Flush(IMFStreamSink *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl wave_stream_sink_vtbl =
{
    wave_stream_sink_QueryInterface,
    wave_stream_sink_AddRef,
    wave_stream_sink_Release,
    wave_stream_sink_GetEvent,
    wave_stream_sink_BeginGetEvent,
    wave_stream_sink_EndGetEvent,
    wave_stream_sink_QueueEvent,
    wave_stream_sink_GetMediaSink,
    wave_stream_sink_GetIdentifier,
    wave_stream_sink_GetMediaTypeHandler,
    wave_stream_sink_ProcessSample,
    wave_stream_sink_PlaceMarker,
    wave_stream_sink_Flush,
};

static HRESULT WINAPI wave_sink_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_QueryInterface(&sink->IMFFinalizableMediaSink_iface, riid, obj);
}

static ULONG WINAPI wave_sink_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_AddRef(&sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI wave_sink_clock_sink_Release(IMFClockStateSink *iface)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_Release(&sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkStarted, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkStopped, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkPaused, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    struct wave_sink *sink = impl_from_IMFClockStateSink(iface);

    TRACE("%p, %s.\n", iface, debugstr_time(systime));

    return IMFMediaEventQueue_QueueEventParamVar(sink->stream_event_queue, MEStreamSinkStarted, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI wave_sink_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    FIXME("%p, %s, %f.\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl wave_sink_clock_sink_vtbl =
{
    wave_sink_clock_sink_QueryInterface,
    wave_sink_clock_sink_AddRef,
    wave_sink_clock_sink_Release,
    wave_sink_clock_sink_OnClockStart,
    wave_sink_clock_sink_OnClockStop,
    wave_sink_clock_sink_OnClockPause,
    wave_sink_clock_sink_OnClockRestart,
    wave_sink_clock_sink_OnClockSetRate,
};

static HRESULT WINAPI wave_sink_type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid,
        void **obj)
{
    struct wave_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&sink->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI wave_sink_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct wave_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&sink->IMFStreamSink_iface);
}

static ULONG WINAPI wave_sink_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct wave_sink *sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&sink->IMFStreamSink_iface);
}

static HRESULT WINAPI wave_sink_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    FIXME("%p, %p, %p.\n", iface, in_type, out_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_sink_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    TRACE("%p, %p.\n", iface, count);

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI wave_sink_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **media_type)
{
    FIXME("%p, %lu, %p.\n", iface, index, media_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_sink_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType *media_type)
{
    FIXME("%p, %p.\n", iface, media_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_sink_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface,
        IMFMediaType **media_type)
{
    FIXME("%p, %p.\n", iface, media_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI wave_sink_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct wave_sink *sink = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    if (sink->flags & SINK_SHUT_DOWN)
        return MF_E_STREAMSINK_REMOVED;

    memcpy(type, &MFMediaType_Audio, sizeof(*type));
    return S_OK;
}

static const IMFMediaTypeHandlerVtbl wave_sink_type_handler_vtbl =
{
    wave_sink_type_handler_QueryInterface,
    wave_sink_type_handler_AddRef,
    wave_sink_type_handler_Release,
    wave_sink_type_handler_IsMediaTypeSupported,
    wave_sink_type_handler_GetMediaTypeCount,
    wave_sink_type_handler_GetMediaTypeByIndex,
    wave_sink_type_handler_SetCurrentMediaType,
    wave_sink_type_handler_GetCurrentMediaType,
    wave_sink_type_handler_GetMajorType,
};

/***********************************************************************
 *      MFCreateWAVEMediaSink (mfsrcsnk.@)
 */
HRESULT WINAPI MFCreateWAVEMediaSink(IMFByteStream *bytestream, IMFMediaType *media_type,
        IMFMediaSink **sink)
{
    struct wave_sink *object;
    DWORD flags = 0;
    UINT32 size;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", bytestream, media_type, sink);

    if (!bytestream || !media_type || !sink)
        return E_POINTER;

    if (FAILED(hr = IMFByteStream_GetCapabilities(bytestream, &flags))) return hr;
    if (!(flags & MFBYTESTREAM_IS_WRITABLE)) return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    /* FIXME: do basic media type validation */

    if (FAILED(hr = MFCreateWaveFormatExFromMFMediaType(media_type, &object->fmt, &size, 0)))
    {
        WARN("Failed to produce WAVEFORMATEX representation, hr %#lx.\n", hr);
        goto failed;
    }

    /* Update derived fields. */
    object->fmt->nAvgBytesPerSec = object->fmt->nSamplesPerSec * object->fmt->nChannels * object->fmt->wBitsPerSample / 8;
    object->fmt->nBlockAlign = object->fmt->nChannels * object->fmt->wBitsPerSample / 8;

    object->IMFFinalizableMediaSink_iface.lpVtbl = &wave_sink_vtbl;
    object->IMFMediaEventGenerator_iface.lpVtbl = &wave_sink_events_vtbl;
    object->IMFStreamSink_iface.lpVtbl = &wave_stream_sink_vtbl;
    object->IMFClockStateSink_iface.lpVtbl = &wave_sink_clock_sink_vtbl;
    object->IMFMediaTypeHandler_iface.lpVtbl = &wave_sink_type_handler_vtbl;
    object->refcount = 1;
    IMFByteStream_AddRef((object->bytestream = bytestream));
    InitializeCriticalSection(&object->cs);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto failed;

    if (FAILED(hr = MFCreateEventQueue(&object->stream_event_queue)))
        goto failed;

    *sink = (IMFMediaSink *)&object->IMFFinalizableMediaSink_iface;

    return S_OK;

failed:

    IMFFinalizableMediaSink_Release(&object->IMFFinalizableMediaSink_iface);

    return hr;
}

static HRESULT WINAPI sink_class_factory_CreateMediaSink(IMFSinkClassFactory *iface, IMFByteStream *stream,
        IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **sink)
{
    TRACE("%p, %p, %p, %p.\n", stream, video_type, audio_type, sink);

    return MFCreateWAVEMediaSink(stream, audio_type, sink);
}

static const IMFSinkClassFactoryVtbl wave_sink_factory_vtbl =
{
    sink_class_factory_QueryInterface,
    sink_class_factory_AddRef,
    sink_class_factory_Release,
    sink_class_factory_CreateMediaSink,
};

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **out)
{
    struct sink_class_factory *factory = sink_class_factory_from_IClassFactory(iface);

    TRACE("iface %p, outer %p, riid %s, out %p stub!.\n", iface, outer, debugstr_guid(riid), out);

    *out = NULL;
    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IMFSinkClassFactory_QueryInterface(&factory->IMFSinkClassFactory_iface, riid, out);
}

static const IClassFactoryVtbl wave_sink_class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

struct sink_class_factory wave_sink_factory =
{
    { &wave_sink_class_factory_vtbl },
    { &wave_sink_factory_vtbl },
};

IClassFactory *wave_sink_class_factory = &wave_sink_factory.IClassFactory_iface;
