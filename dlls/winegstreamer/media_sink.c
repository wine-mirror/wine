/* Gstreamer Media Sink
 *
 * Copyright 2023 Ziqing Hui for CodeWeavers
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

#include "gst_private.h"
#include "mferror.h"
#include "mfapi.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

enum async_op
{
    ASYNC_START,
    ASYNC_STOP,
    ASYNC_PAUSE,
    ASYNC_PROCESS,
    ASYNC_FINALIZE,
};

struct async_command
{
    IUnknown IUnknown_iface;
    LONG refcount;

    enum async_op op;

    union
    {
        struct
        {
            IMFSample *sample;
            UINT32 stream_id;
        } process;
        struct
        {
            IMFAsyncResult *result;
        } finalize;
    } u;
};

struct stream_sink
{
    IMFStreamSink IMFStreamSink_iface;
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    LONG refcount;
    DWORD id;

    IMFMediaType *type;
    IMFFinalizableMediaSink *media_sink;
    IMFMediaEventQueue *event_queue;

    struct list entry;
};

struct media_sink
{
    IMFFinalizableMediaSink IMFFinalizableMediaSink_iface;
    IMFMediaEventGenerator IMFMediaEventGenerator_iface;
    IMFClockStateSink IMFClockStateSink_iface;
    IMFAsyncCallback async_callback;
    LONG refcount;
    CRITICAL_SECTION cs;

    enum
    {
        STATE_OPENED,
        STATE_STARTED,
        STATE_STOPPED,
        STATE_PAUSED,
        STATE_FINALIZED,
        STATE_SHUTDOWN,
    } state;

    IMFByteStream *bytestream;
    IMFMediaEventQueue *event_queue;

    struct list stream_sinks;

    wg_muxer_t muxer;
};

static struct stream_sink *impl_from_IMFStreamSink(IMFStreamSink *iface)
{
    return CONTAINING_RECORD(iface, struct stream_sink, IMFStreamSink_iface);
}

static struct stream_sink *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct stream_sink, IMFMediaTypeHandler_iface);
}

static struct media_sink *impl_from_IMFFinalizableMediaSink(IMFFinalizableMediaSink *iface)
{
    return CONTAINING_RECORD(iface, struct media_sink, IMFFinalizableMediaSink_iface);
}

static struct media_sink *impl_from_IMFMediaEventGenerator(IMFMediaEventGenerator *iface)
{
    return CONTAINING_RECORD(iface, struct media_sink, IMFMediaEventGenerator_iface);
}

static struct media_sink *impl_from_IMFClockStateSink(IMFClockStateSink *iface)
{
    return CONTAINING_RECORD(iface, struct media_sink, IMFClockStateSink_iface);
}

static struct media_sink *impl_from_async_callback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_sink, async_callback);
}

static struct async_command *impl_from_async_command_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct async_command, IUnknown_iface);
}

static HRESULT WINAPI async_command_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_command_AddRef(IUnknown *iface)
{
    struct async_command *command = impl_from_async_command_IUnknown(iface);
    return InterlockedIncrement(&command->refcount);
}

static ULONG WINAPI async_command_Release(IUnknown *iface)
{
    struct async_command *command = impl_from_async_command_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&command->refcount);

    if (!refcount)
    {
        if (command->op == ASYNC_PROCESS && command->u.process.sample)
            IMFSample_Release(command->u.process.sample);
        else if (command->op == ASYNC_FINALIZE && command->u.finalize.result)
            IMFAsyncResult_Release(command->u.finalize.result);
        free(command);
    }

    return refcount;
}

static const IUnknownVtbl async_command_vtbl =
{
    async_command_QueryInterface,
    async_command_AddRef,
    async_command_Release,
};

static HRESULT async_command_create(enum async_op op, struct async_command **out)
{
    struct async_command *command;

    if (!(command = calloc(1, sizeof(*command))))
        return E_OUTOFMEMORY;

    command->IUnknown_iface.lpVtbl = &async_command_vtbl;
    command->refcount = 1;
    command->op = op;

    TRACE("Created async command %p.\n", command);
    *out = command;

    return S_OK;
}

static HRESULT WINAPI stream_sink_QueryInterface(IMFStreamSink *iface, REFIID riid, void **obj)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, riid %s, obj %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFStreamSink) ||
            IsEqualGUID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &stream_sink->IMFStreamSink_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaTypeHandler))
    {
        *obj = &stream_sink->IMFMediaTypeHandler_iface;
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

static ULONG WINAPI stream_sink_AddRef(IMFStreamSink *iface)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedIncrement(&stream_sink->refcount);
    TRACE("iface %p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI stream_sink_Release(IMFStreamSink *iface)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);
    ULONG refcount = InterlockedDecrement(&stream_sink->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IMFMediaEventQueue_Release(stream_sink->event_queue);
        IMFFinalizableMediaSink_Release(stream_sink->media_sink);
        if (stream_sink->type)
            IMFMediaType_Release(stream_sink->type);
        free(stream_sink);
    }

    return refcount;
}

static HRESULT WINAPI stream_sink_GetEvent(IMFStreamSink *iface, DWORD flags, IMFMediaEvent **event)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, flags %#lx, event %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(stream_sink->event_queue, flags, event);
}

static HRESULT WINAPI stream_sink_BeginGetEvent(IMFStreamSink *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, callback %p, state %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(stream_sink->event_queue, callback, state);
}

static HRESULT WINAPI stream_sink_EndGetEvent(IMFStreamSink *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, result %p, event %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(stream_sink->event_queue, result, event);
}

static HRESULT WINAPI stream_sink_QueueEvent(IMFStreamSink *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, event_type %lu, ext_type %s, hr %#lx, value %p.\n",
            iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(stream_sink->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI stream_sink_GetMediaSink(IMFStreamSink *iface, IMFMediaSink **ret)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, ret %p.\n", iface, ret);

    IMFMediaSink_AddRef((*ret = (IMFMediaSink *)stream_sink->media_sink));

    return S_OK;
}

static HRESULT WINAPI stream_sink_GetIdentifier(IMFStreamSink *iface, DWORD *identifier)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, identifier %p.\n", iface, identifier);

    *identifier = stream_sink->id;

    return S_OK;
}

static HRESULT WINAPI stream_sink_GetMediaTypeHandler(IMFStreamSink *iface, IMFMediaTypeHandler **handler)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);

    TRACE("iface %p, handler %p.\n", iface, handler);

    IMFMediaTypeHandler_AddRef((*handler = &stream_sink->IMFMediaTypeHandler_iface));

    return S_OK;
}

static HRESULT WINAPI stream_sink_ProcessSample(IMFStreamSink *iface, IMFSample *sample)
{
    struct stream_sink *stream_sink = impl_from_IMFStreamSink(iface);
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(stream_sink->media_sink);
    struct async_command *command;
    HRESULT hr;

    TRACE("iface %p, sample %p.\n", iface, sample);

    EnterCriticalSection(&media_sink->cs);

    if (media_sink->state == STATE_SHUTDOWN)
    {
        LeaveCriticalSection(&media_sink->cs);
        return MF_E_SHUTDOWN;
    }

    if (media_sink->state != STATE_STARTED && media_sink->state != STATE_PAUSED)
    {
        LeaveCriticalSection(&media_sink->cs);
        return MF_E_INVALIDREQUEST;
    }

    if (FAILED(hr = (async_command_create(ASYNC_PROCESS, &command))))
    {
        LeaveCriticalSection(&media_sink->cs);
        return hr;
    }
    IMFSample_AddRef((command->u.process.sample = sample));
    command->u.process.stream_id = stream_sink->id;

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD,
            &media_sink->async_callback, &command->IUnknown_iface);
    IUnknown_Release(&command->IUnknown_iface);

    LeaveCriticalSection(&media_sink->cs);

    return hr;
}

static HRESULT WINAPI stream_sink_PlaceMarker(IMFStreamSink *iface, MFSTREAMSINK_MARKER_TYPE marker_type,
        const PROPVARIANT *marker_value, const PROPVARIANT *context_value)
{
    FIXME("iface %p, marker_type %d, marker_value %p, context_value %p stub!\n",
            iface, marker_type, marker_value, context_value);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_Flush(IMFStreamSink *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static const IMFStreamSinkVtbl stream_sink_vtbl =
{
    stream_sink_QueryInterface,
    stream_sink_AddRef,
    stream_sink_Release,
    stream_sink_GetEvent,
    stream_sink_BeginGetEvent,
    stream_sink_EndGetEvent,
    stream_sink_QueueEvent,
    stream_sink_GetMediaSink,
    stream_sink_GetIdentifier,
    stream_sink_GetMediaTypeHandler,
    stream_sink_ProcessSample,
    stream_sink_PlaceMarker,
    stream_sink_Flush,
};

static HRESULT WINAPI stream_sink_type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid, void **obj)
{
    struct stream_sink *stream_sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_QueryInterface(&stream_sink->IMFStreamSink_iface, riid, obj);
}

static ULONG WINAPI stream_sink_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct stream_sink *stream_sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_AddRef(&stream_sink->IMFStreamSink_iface);
}

static ULONG WINAPI stream_sink_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct stream_sink *stream_sink = impl_from_IMFMediaTypeHandler(iface);
    return IMFStreamSink_Release(&stream_sink->IMFStreamSink_iface);
}

static HRESULT WINAPI stream_sink_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface,
        IMFMediaType *in_type, IMFMediaType **out_type)
{
    FIXME("iface %p, in_type %p, out_type %p.\n", iface, in_type, out_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    FIXME("iface %p, count %p.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    FIXME("iface %p, index %lu, type %p.\n", iface, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType *type)
{
    FIXME("iface %p, type %p.\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI stream_sink_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType **type)
{
    struct stream_sink *stream_sink = impl_from_IMFMediaTypeHandler(iface);

    TRACE("iface %p, type %p.\n", iface, type);

    if (!type)
        return E_POINTER;
    if (!stream_sink->type)
        return MF_E_NOT_INITIALIZED;

    IMFMediaType_AddRef((*type = stream_sink->type));

    return S_OK;
}

static HRESULT WINAPI stream_sink_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    FIXME("iface %p, type %p.\n", iface, type);

    return E_NOTIMPL;
}

static const IMFMediaTypeHandlerVtbl stream_sink_type_handler_vtbl =
{
    stream_sink_type_handler_QueryInterface,
    stream_sink_type_handler_AddRef,
    stream_sink_type_handler_Release,
    stream_sink_type_handler_IsMediaTypeSupported,
    stream_sink_type_handler_GetMediaTypeCount,
    stream_sink_type_handler_GetMediaTypeByIndex,
    stream_sink_type_handler_SetCurrentMediaType,
    stream_sink_type_handler_GetCurrentMediaType,
    stream_sink_type_handler_GetMajorType,
};

static HRESULT stream_sink_create(DWORD stream_sink_id, IMFMediaType *media_type, struct media_sink *media_sink,
        struct stream_sink **out)
{
    struct stream_sink *stream_sink;
    HRESULT hr;

    TRACE("stream_sink_id %#lx, media_type %p, media_sink %p, out %p.\n",
            stream_sink_id, media_type, media_sink, out);

    if (!(stream_sink = calloc(1, sizeof(*stream_sink))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = MFCreateEventQueue(&stream_sink->event_queue)))
    {
        free(stream_sink);
        return hr;
    }

    stream_sink->IMFStreamSink_iface.lpVtbl = &stream_sink_vtbl;
    stream_sink->IMFMediaTypeHandler_iface.lpVtbl = &stream_sink_type_handler_vtbl;
    stream_sink->refcount = 1;
    stream_sink->id = stream_sink_id;
    if (media_type)
        IMFMediaType_AddRef((stream_sink->type = media_type));
    IMFFinalizableMediaSink_AddRef((stream_sink->media_sink = &media_sink->IMFFinalizableMediaSink_iface));

    TRACE("Created stream sink %p.\n", stream_sink);
    *out = stream_sink;

    return S_OK;
}

static struct stream_sink *media_sink_get_stream_sink_by_id(struct media_sink *media_sink, DWORD id)
{
    struct stream_sink *stream_sink;

    LIST_FOR_EACH_ENTRY(stream_sink, &media_sink->stream_sinks, struct stream_sink, entry)
    {
        if (stream_sink->id == id)
            return stream_sink;
    }

    return NULL;
}

static HRESULT media_sink_queue_command(struct media_sink *media_sink, enum async_op op)
{
    struct async_command *command;
    HRESULT hr;

    if (media_sink->state == STATE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    if (FAILED(hr = async_command_create(op, &command)))
        return hr;

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD,
            &media_sink->async_callback, &command->IUnknown_iface);
    IUnknown_Release(&command->IUnknown_iface);

    return hr;
}

static HRESULT media_sink_queue_stream_event(struct media_sink *media_sink, MediaEventType type)
{
    struct stream_sink *stream_sink;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY(stream_sink, &media_sink->stream_sinks, struct stream_sink, entry)
    {
        if (FAILED(hr = IMFMediaEventQueue_QueueEventParamVar(stream_sink->event_queue, type, &GUID_NULL, S_OK, NULL)))
            return hr;
    }

    return S_OK;
}

static HRESULT media_sink_write_stream(struct media_sink *media_sink)
{
    BYTE buffer[1024];
    UINT32 size = sizeof(buffer);
    ULONG written;
    QWORD offset;
    HRESULT hr;

    while (SUCCEEDED(hr = wg_muxer_read_data(media_sink->muxer, buffer, &size, &offset)))
    {
        if (offset != UINT64_MAX && FAILED(hr = IMFByteStream_SetCurrentPosition(media_sink->bytestream, offset)))
            return hr;

        if (FAILED(hr = IMFByteStream_Write(media_sink->bytestream, buffer, size, &written)))
            return hr;

        size = sizeof(buffer);
    }

    return S_OK;
}

static HRESULT media_sink_start(struct media_sink *media_sink)
{
    HRESULT hr;

    if (FAILED(hr = wg_muxer_start(media_sink->muxer)))
        return hr;

    media_sink->state = STATE_STARTED;

    return media_sink_queue_stream_event(media_sink, MEStreamSinkStarted);
}

static HRESULT media_sink_stop(struct media_sink *media_sink)
{
    media_sink->state = STATE_STOPPED;
    return media_sink_queue_stream_event(media_sink, MEStreamSinkStopped);

}

static HRESULT media_sink_pause(struct media_sink *media_sink)
{
    media_sink->state = STATE_PAUSED;
    return media_sink_queue_stream_event(media_sink, MEStreamSinkPaused);
}

static HRESULT media_sink_process(struct media_sink *media_sink, IMFSample *sample, UINT32 stream_id)
{
    wg_muxer_t muxer = media_sink->muxer;
    struct wg_sample *wg_sample;
    LONGLONG time, duration;
    UINT32 value;
    HRESULT hr;

    TRACE("media_sink %p, sample %p, stream_id %u.\n", media_sink, sample, stream_id);

    if (FAILED(hr = media_sink_write_stream(media_sink)))
        WARN("Failed to write output samples to stream, hr %#lx.\n", hr);

    if (FAILED(hr = wg_sample_create_mf(sample, &wg_sample)))
        return hr;

    if (SUCCEEDED(IMFSample_GetSampleTime(sample, &time)))
    {
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_PTS;
        wg_sample->pts = time;
    }
    if (SUCCEEDED(IMFSample_GetSampleDuration(sample, &duration)))
    {
        wg_sample->flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        wg_sample->duration = duration;
    }
    if (SUCCEEDED(IMFSample_GetUINT32(sample, &MFSampleExtension_CleanPoint, &value)) && value)
        wg_sample->flags |= WG_SAMPLE_FLAG_SYNC_POINT;
    if (SUCCEEDED(IMFSample_GetUINT32(sample, &MFSampleExtension_Discontinuity, &value)) && value)
        wg_sample->flags |= WG_SAMPLE_FLAG_DISCONTINUITY;

    hr = wg_muxer_push_sample(muxer, wg_sample, stream_id);
    wg_sample_release(wg_sample);

    return hr;
}

static HRESULT media_sink_begin_finalize(struct media_sink *media_sink, IMFAsyncCallback *callback, IUnknown *state)
{
    struct async_command *command;
    IMFAsyncResult *result;
    HRESULT hr;

    if (media_sink->state == STATE_SHUTDOWN)
        return MF_E_SHUTDOWN;
    if (!callback)
        return S_OK;

    if (FAILED(hr = async_command_create(ASYNC_FINALIZE, &command)))
        return hr;

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
    {
        IUnknown_Release(&command->IUnknown_iface);
        return hr;
    }
    IMFAsyncResult_AddRef((command->u.finalize.result = result));

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD,
            &media_sink->async_callback, &command->IUnknown_iface);
    IUnknown_Release(&command->IUnknown_iface);

    return hr;
}

static HRESULT media_sink_finalize(struct media_sink *media_sink, IMFAsyncResult *result)
{
    HRESULT hr;

    media_sink->state = STATE_FINALIZED;

    hr = wg_muxer_finalize(media_sink->muxer);

    if (SUCCEEDED(hr))
        hr = media_sink_write_stream(media_sink);

    IMFAsyncResult_SetStatus(result, hr);
    MFInvokeCallback(result);

    return hr;
}

static HRESULT WINAPI media_sink_QueryInterface(IMFFinalizableMediaSink *iface, REFIID riid, void **obj)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("iface %p, riid %s, obj %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFFinalizableMediaSink) ||
            IsEqualIID(riid, &IID_IMFMediaSink) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualGUID(riid, &IID_IMFMediaEventGenerator))
    {
        *obj = &media_sink->IMFMediaEventGenerator_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFClockStateSink))
    {
        *obj = &media_sink->IMFClockStateSink_iface;
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

static ULONG WINAPI media_sink_AddRef(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedIncrement(&media_sink->refcount);
    TRACE("iface %p, refcount %lu.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI media_sink_Release(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    ULONG refcount = InterlockedDecrement(&media_sink->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IMFFinalizableMediaSink_Shutdown(iface);
        IMFMediaEventQueue_Release(media_sink->event_queue);
        IMFByteStream_Release(media_sink->bytestream);
        media_sink->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&media_sink->cs);
        wg_muxer_destroy(media_sink->muxer);
        free(media_sink);
    }

    return refcount;
}

static HRESULT WINAPI media_sink_GetCharacteristics(IMFFinalizableMediaSink *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_AddStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
    IMFMediaType *media_type, IMFStreamSink **stream_sink)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    struct stream_sink *object;
    struct wg_format format;
    HRESULT hr;

    TRACE("iface %p, stream_sink_id %#lx, media_type %p, stream_sink %p.\n",
            iface, stream_sink_id, media_type, stream_sink);

    EnterCriticalSection(&media_sink->cs);

    if (media_sink_get_stream_sink_by_id(media_sink, stream_sink_id))
    {
        LeaveCriticalSection(&media_sink->cs);
        return MF_E_STREAMSINK_EXISTS;
    }

    if (FAILED(hr = stream_sink_create(stream_sink_id, media_type, media_sink, &object)))
    {
        WARN("Failed to create stream sink, hr %#lx.\n", hr);
        LeaveCriticalSection(&media_sink->cs);
        return hr;
    }

    mf_media_type_to_wg_format(media_type, &format);
    if (FAILED(hr = wg_muxer_add_stream(media_sink->muxer, stream_sink_id, &format)))
    {
        LeaveCriticalSection(&media_sink->cs);
        IMFStreamSink_Release(&object->IMFStreamSink_iface);
        return hr;
    }

    list_add_tail(&media_sink->stream_sinks, &object->entry);

    LeaveCriticalSection(&media_sink->cs);

    if (stream_sink)
        IMFStreamSink_AddRef((*stream_sink = &object->IMFStreamSink_iface));

    return S_OK;
}

static HRESULT WINAPI media_sink_RemoveStreamSink(IMFFinalizableMediaSink *iface, DWORD stream_sink_id)
{
    FIXME("iface %p, stream_sink_id %#lx stub!\n", iface, stream_sink_id);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetStreamSinkCount(IMFFinalizableMediaSink *iface, DWORD *count)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    EnterCriticalSection(&media_sink->cs);
    *count = list_count(&media_sink->stream_sinks);
    LeaveCriticalSection(&media_sink->cs);

    return S_OK;
}

static HRESULT WINAPI media_sink_GetStreamSinkByIndex(IMFFinalizableMediaSink *iface, DWORD index,
        IMFStreamSink **stream)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    struct stream_sink *stream_sink;
    HRESULT hr = MF_E_INVALIDINDEX;
    DWORD entry_index = 0;

    TRACE("iface %p, index %lu, stream %p stub!\n", iface, index, stream);

    if (!stream)
        return E_POINTER;

    EnterCriticalSection(&media_sink->cs);

    LIST_FOR_EACH_ENTRY(stream_sink, &media_sink->stream_sinks, struct stream_sink, entry)
    {
        if (entry_index++ == index)
        {
            IMFStreamSink_AddRef((*stream = &stream_sink->IMFStreamSink_iface));
            hr = S_OK;
            break;
        }
    }

    LeaveCriticalSection(&media_sink->cs);

    return hr;
}

static HRESULT WINAPI media_sink_GetStreamSinkById(IMFFinalizableMediaSink *iface, DWORD stream_sink_id,
        IMFStreamSink **stream)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    struct stream_sink *stream_sink;
    HRESULT hr;

    TRACE("iface %p, stream_sink_id %#lx, stream %p.\n", iface, stream_sink_id, stream);

    if (!stream)
        return E_POINTER;

    EnterCriticalSection(&media_sink->cs);

    hr = MF_E_INVALIDSTREAMNUMBER;
    if ((stream_sink = media_sink_get_stream_sink_by_id(media_sink, stream_sink_id)))
    {
        IMFStreamSink_AddRef((*stream = &stream_sink->IMFStreamSink_iface));
        hr = S_OK;
    }

    LeaveCriticalSection(&media_sink->cs);

    return hr;
}

static HRESULT WINAPI media_sink_SetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock *clock)
{
    FIXME("iface %p, clock %p stub!\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_GetPresentationClock(IMFFinalizableMediaSink *iface, IMFPresentationClock **clock)
{
    FIXME("iface %p, clock %p stub!\n", iface, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_Shutdown(IMFFinalizableMediaSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    struct stream_sink *stream_sink, *next;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&media_sink->cs);

    if (media_sink->state == STATE_SHUTDOWN)
    {
        LeaveCriticalSection(&media_sink->cs);
        return MF_E_SHUTDOWN;
    }

    LIST_FOR_EACH_ENTRY_SAFE(stream_sink, next, &media_sink->stream_sinks, struct stream_sink, entry)
    {
        list_remove(&stream_sink->entry);
        IMFMediaEventQueue_Shutdown(stream_sink->event_queue);
        IMFStreamSink_Release(&stream_sink->IMFStreamSink_iface);
    }

    IMFMediaEventQueue_Shutdown(media_sink->event_queue);
    IMFByteStream_Close(media_sink->bytestream);

    media_sink->state = STATE_SHUTDOWN;

    LeaveCriticalSection(&media_sink->cs);

    return S_OK;
}

static HRESULT WINAPI media_sink_BeginFinalize(IMFFinalizableMediaSink *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_sink *media_sink = impl_from_IMFFinalizableMediaSink(iface);
    HRESULT hr;

    TRACE("iface %p, callback %p, state %p.\n", iface, callback, state);

    EnterCriticalSection(&media_sink->cs);
    hr =  media_sink_begin_finalize(media_sink, callback, state);
    LeaveCriticalSection(&media_sink->cs);

    return hr;
}

static HRESULT WINAPI media_sink_EndFinalize(IMFFinalizableMediaSink *iface, IMFAsyncResult *result)
{
    TRACE("iface %p, result %p.\n", iface, result);

    return result ? IMFAsyncResult_GetStatus(result) : E_INVALIDARG;
}

static const IMFFinalizableMediaSinkVtbl media_sink_vtbl =
{
    media_sink_QueryInterface,
    media_sink_AddRef,
    media_sink_Release,
    media_sink_GetCharacteristics,
    media_sink_AddStreamSink,
    media_sink_RemoveStreamSink,
    media_sink_GetStreamSinkCount,
    media_sink_GetStreamSinkByIndex,
    media_sink_GetStreamSinkById,
    media_sink_SetPresentationClock,
    media_sink_GetPresentationClock,
    media_sink_Shutdown,
    media_sink_BeginFinalize,
    media_sink_EndFinalize,
};

static HRESULT WINAPI media_sink_event_QueryInterface(IMFMediaEventGenerator *iface, REFIID riid, void **obj)
{
    struct media_sink *media_sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_QueryInterface(&media_sink->IMFFinalizableMediaSink_iface, riid, obj);
}

static ULONG WINAPI media_sink_event_AddRef(IMFMediaEventGenerator *iface)
{
    struct media_sink *media_sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_AddRef(&media_sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI media_sink_event_Release(IMFMediaEventGenerator *iface)
{
    struct media_sink *media_sink = impl_from_IMFMediaEventGenerator(iface);
    return IMFFinalizableMediaSink_Release(&media_sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI media_sink_event_GetEvent(IMFMediaEventGenerator *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_sink *media_sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("iface %p, flags %#lx, event %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(media_sink->event_queue, flags, event);
}

static HRESULT WINAPI media_sink_event_BeginGetEvent(IMFMediaEventGenerator *iface, IMFAsyncCallback *callback,
        IUnknown *state)
{
    struct media_sink *media_sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("iface %p, callback %p, state %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(media_sink->event_queue, callback, state);
}

static HRESULT WINAPI media_sink_event_EndGetEvent(IMFMediaEventGenerator *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct media_sink *media_sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("iface %p, result %p, event %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(media_sink->event_queue, result, event);
}

static HRESULT WINAPI media_sink_event_QueueEvent(IMFMediaEventGenerator *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct media_sink *media_sink = impl_from_IMFMediaEventGenerator(iface);

    TRACE("iface %p, event_type %lu, ext_type %s, hr %#lx, value %p.\n",
            iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(media_sink->event_queue, event_type, ext_type, hr, value);
}

static const IMFMediaEventGeneratorVtbl media_sink_event_vtbl =
{
    media_sink_event_QueryInterface,
    media_sink_event_AddRef,
    media_sink_event_Release,
    media_sink_event_GetEvent,
    media_sink_event_BeginGetEvent,
    media_sink_event_EndGetEvent,
    media_sink_event_QueueEvent,
};

static HRESULT WINAPI media_sink_clock_sink_QueryInterface(IMFClockStateSink *iface, REFIID riid, void **obj)
{
    struct media_sink *media_sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_QueryInterface(&media_sink->IMFFinalizableMediaSink_iface, riid, obj);
}

static ULONG WINAPI media_sink_clock_sink_AddRef(IMFClockStateSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_AddRef(&media_sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI media_sink_clock_sink_Release(IMFClockStateSink *iface)
{
    struct media_sink *media_sink = impl_from_IMFClockStateSink(iface);
    return IMFFinalizableMediaSink_Release(&media_sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI media_sink_clock_sink_OnClockStart(IMFClockStateSink *iface, MFTIME systime, LONGLONG offset)
{
    struct media_sink *media_sink = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("iface %p, systime %s, offset %s.\n", iface, debugstr_time(systime), debugstr_time(offset));

    EnterCriticalSection(&media_sink->cs);

    hr = media_sink_queue_command(media_sink, ASYNC_START);

    LeaveCriticalSection(&media_sink->cs);
    return hr;
}

static HRESULT WINAPI media_sink_clock_sink_OnClockStop(IMFClockStateSink *iface, MFTIME systime)
{
    struct media_sink *media_sink = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("iface %p, systime %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&media_sink->cs);

    hr = media_sink_queue_command(media_sink, ASYNC_STOP);

    LeaveCriticalSection(&media_sink->cs);
    return hr;
}

static HRESULT WINAPI media_sink_clock_sink_OnClockPause(IMFClockStateSink *iface, MFTIME systime)
{
    struct media_sink *media_sink = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("iface %p, systime %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&media_sink->cs);

    hr = media_sink_queue_command(media_sink, ASYNC_PAUSE);

    LeaveCriticalSection(&media_sink->cs);
    return hr;
}

static HRESULT WINAPI media_sink_clock_sink_OnClockRestart(IMFClockStateSink *iface, MFTIME systime)
{
    struct media_sink *media_sink = impl_from_IMFClockStateSink(iface);
    HRESULT hr;

    TRACE("iface %p, systime %s.\n", iface, debugstr_time(systime));

    EnterCriticalSection(&media_sink->cs);

    hr = media_sink_queue_command(media_sink, ASYNC_START);

    LeaveCriticalSection(&media_sink->cs);
    return hr;
}

static HRESULT WINAPI media_sink_clock_sink_OnClockSetRate(IMFClockStateSink *iface, MFTIME systime, float rate)
{
    FIXME("iface %p, systime %s, rate %f stub!\n", iface, debugstr_time(systime), rate);

    return E_NOTIMPL;
}

static const IMFClockStateSinkVtbl media_sink_clock_sink_vtbl =
{
    media_sink_clock_sink_QueryInterface,
    media_sink_clock_sink_AddRef,
    media_sink_clock_sink_Release,
    media_sink_clock_sink_OnClockStart,
    media_sink_clock_sink_OnClockStop,
    media_sink_clock_sink_OnClockPause,
    media_sink_clock_sink_OnClockRestart,
    media_sink_clock_sink_OnClockSetRate,
};

static HRESULT WINAPI media_sink_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, obj %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_sink_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_sink *sink = impl_from_async_callback(iface);
    return IMFFinalizableMediaSink_AddRef(&sink->IMFFinalizableMediaSink_iface);
}

static ULONG WINAPI media_sink_callback_Release(IMFAsyncCallback *iface)
{
    struct media_sink *sink = impl_from_async_callback(iface);
    return IMFFinalizableMediaSink_Release(&sink->IMFFinalizableMediaSink_iface);
}

static HRESULT WINAPI media_sink_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    TRACE("iface %p, flags %p, queue %p.\n", iface, flags, queue);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_sink_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *async_result)
{
    struct media_sink *media_sink = impl_from_async_callback(iface);
    struct async_command *command;
    HRESULT hr = E_FAIL;
    IUnknown *state;

    TRACE("iface %p, async_result %p.\n", iface, async_result);

    EnterCriticalSection(&media_sink->cs);

    if (!(state = IMFAsyncResult_GetStateNoAddRef(async_result)))
    {
        LeaveCriticalSection(&media_sink->cs);
        return hr;
    }

    command = impl_from_async_command_IUnknown(state);
    switch (command->op)
    {
        case ASYNC_START:
            if (FAILED(hr = media_sink_start(media_sink)))
                WARN("Failed to start media sink.\n");
            break;
        case ASYNC_STOP:
            hr = media_sink_stop(media_sink);
            break;
        case ASYNC_PAUSE:
            hr = media_sink_pause(media_sink);
            break;
        case ASYNC_PROCESS:
            if (FAILED(hr = media_sink_process(media_sink, command->u.process.sample, command->u.process.stream_id)))
                WARN("Failed to process sample, hr %#lx.\n", hr);
            break;
        case ASYNC_FINALIZE:
            if (FAILED(hr = media_sink_finalize(media_sink, command->u.finalize.result)))
                WARN("Failed to finalize, hr %#lx.\n", hr);
            break;
        default:
            WARN("Unsupported op %u.\n", command->op);
            break;
    }

    LeaveCriticalSection(&media_sink->cs);

    return hr;
}

static const IMFAsyncCallbackVtbl media_sink_callback_vtbl =
{
    media_sink_callback_QueryInterface,
    media_sink_callback_AddRef,
    media_sink_callback_Release,
    media_sink_callback_GetParameters,
    media_sink_callback_Invoke,
};

static HRESULT media_sink_create(IMFByteStream *bytestream, const char *format, struct media_sink **out)
{
    struct media_sink *media_sink;
    HRESULT hr;

    TRACE("bytestream %p, out %p.\n", bytestream, out);

    if (!bytestream)
        return E_POINTER;

    if (!(media_sink = calloc(1, sizeof(*media_sink))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wg_muxer_create(format, &media_sink->muxer)))
        goto fail;
    if (FAILED(hr = MFCreateEventQueue(&media_sink->event_queue)))
        goto fail;

    media_sink->IMFFinalizableMediaSink_iface.lpVtbl = &media_sink_vtbl;
    media_sink->IMFMediaEventGenerator_iface.lpVtbl = &media_sink_event_vtbl;
    media_sink->IMFClockStateSink_iface.lpVtbl = &media_sink_clock_sink_vtbl;
    media_sink->async_callback.lpVtbl = &media_sink_callback_vtbl;
    media_sink->refcount = 1;
    media_sink->state = STATE_OPENED;
    InitializeCriticalSection(&media_sink->cs);
    media_sink->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");
    IMFByteStream_AddRef((media_sink->bytestream = bytestream));
    list_init(&media_sink->stream_sinks);

    *out = media_sink;
    TRACE("Created media sink %p.\n", media_sink);

    return S_OK;

fail:
    if (media_sink->muxer)
        wg_muxer_destroy(media_sink->muxer);
    free(media_sink);
    return hr;
}

static HRESULT WINAPI sink_class_factory_QueryInterface(IMFSinkClassFactory *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFSinkClassFactory)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFSinkClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_class_factory_AddRef(IMFSinkClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI sink_class_factory_Release(IMFSinkClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI sink_class_factory_create_media_sink(IMFSinkClassFactory *iface, IMFByteStream *bytestream,
        const char *format, IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **out)
{
    IMFFinalizableMediaSink *media_sink_iface;
    struct media_sink *media_sink;
    HRESULT hr;

    TRACE("iface %p, bytestream %p, video_type %p, audio_type %p, out %p.\n",
            iface, bytestream, video_type, audio_type, out);

    if (FAILED(hr = media_sink_create(bytestream, format, &media_sink)))
        return hr;
    media_sink_iface = &media_sink->IMFFinalizableMediaSink_iface;

    if (video_type)
    {
        if (FAILED(hr = IMFFinalizableMediaSink_AddStreamSink(media_sink_iface, 1, video_type, NULL)))
        {
            IMFFinalizableMediaSink_Shutdown(media_sink_iface);
            IMFFinalizableMediaSink_Release(media_sink_iface);
            return hr;
        }
    }
    if (audio_type)
    {
        if (FAILED(hr = IMFFinalizableMediaSink_AddStreamSink(media_sink_iface, 2, audio_type, NULL)))
        {
            IMFFinalizableMediaSink_Shutdown(media_sink_iface);
            IMFFinalizableMediaSink_Release(media_sink_iface);
            return hr;
        }
    }

    *out = (IMFMediaSink *)media_sink_iface;
    return S_OK;
}

static HRESULT WINAPI mp3_sink_class_factory_CreateMediaSink(IMFSinkClassFactory *iface, IMFByteStream *bytestream,
        IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **out)
{
    const char *format = "application/x-id3";

    return sink_class_factory_create_media_sink(iface, bytestream, format, video_type, audio_type, out);
}

static HRESULT WINAPI mpeg4_sink_class_factory_CreateMediaSink(IMFSinkClassFactory *iface, IMFByteStream *bytestream,
        IMFMediaType *video_type, IMFMediaType *audio_type, IMFMediaSink **out)
{
    const char *format = "video/quicktime, variant=iso";

    return sink_class_factory_create_media_sink(iface, bytestream, format, video_type, audio_type, out);
}

static const IMFSinkClassFactoryVtbl mp3_sink_class_factory_vtbl =
{
    sink_class_factory_QueryInterface,
    sink_class_factory_AddRef,
    sink_class_factory_Release,
    mp3_sink_class_factory_CreateMediaSink,
};

static const IMFSinkClassFactoryVtbl mpeg4_sink_class_factory_vtbl =
{
    sink_class_factory_QueryInterface,
    sink_class_factory_AddRef,
    sink_class_factory_Release,
    mpeg4_sink_class_factory_CreateMediaSink,
};

static IMFSinkClassFactory mp3_sink_class_factory = { &mp3_sink_class_factory_vtbl };
static IMFSinkClassFactory mpeg4_sink_class_factory = { &mpeg4_sink_class_factory_vtbl };

HRESULT mp3_sink_class_factory_create(IUnknown *outer, IUnknown **out)
{
    *out = (IUnknown *)&mp3_sink_class_factory;
    return S_OK;
}

HRESULT mpeg4_sink_class_factory_create(IUnknown *outer, IUnknown **out)
{
    *out = (IUnknown *)&mpeg4_sink_class_factory;
    return S_OK;
}
