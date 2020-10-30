/*
 *
 * Copyright 2014 Austin English
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

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "rpcproxy.h"

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"
#include "mfreadwrite.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

#include "mf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HINSTANCE mfinstance;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            mfinstance = instance;
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( mfinstance );
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( mfinstance );
}

struct stream_response
{
    struct list entry;
    HRESULT status;
    DWORD stream_index;
    DWORD stream_flags;
    LONGLONG timestamp;
    IMFSample *sample;
};

enum media_stream_state
{
    STREAM_STATE_READY = 0,
    STREAM_STATE_EOS,
};

enum media_source_state
{
    SOURCE_STATE_STOPPED = 0,
    SOURCE_STATE_STARTED,
};

enum media_stream_flags
{
    STREAM_FLAG_SAMPLE_REQUESTED = 0x1, /* Protects from making multiple sample requests. */
    STREAM_FLAG_SELECTED = 0x2,         /* Mirrors descriptor, used to simplify tests when starting the source. */
    STREAM_FLAG_PRESENTED = 0x4,        /* Set if stream was selected last time Start() was called. */
    STREAM_FLAG_REQUESTED_ONCE = 0x8,   /* Used for MF_SOURCE_READER_ANY_STREAM in synchronous mode. */
};

struct media_stream
{
    IMFMediaStream *stream;
    IMFMediaType *current;
    IMFTransform *decoder;
    DWORD id;
    unsigned int index;
    enum media_stream_state state;
    unsigned int flags;
    unsigned int requests;
    unsigned int responses;
};

enum source_reader_async_op
{
    SOURCE_READER_ASYNC_READ,
    SOURCE_READER_ASYNC_SEEK,
    SOURCE_READER_ASYNC_FLUSH,
    SOURCE_READER_ASYNC_SAMPLE_READY,
};

struct source_reader_async_command
{
    IUnknown IUnknown_iface;
    LONG refcount;
    enum source_reader_async_op op;
    union
    {
        struct
        {
            unsigned int flags;
            unsigned int stream_index;
        } read;
        struct
        {
            GUID format;
            PROPVARIANT position;
        } seek;
        struct
        {
            unsigned int stream_index;
        } flush;
        struct
        {
            unsigned int stream_index;
        } sample;
    } u;
};

enum source_reader_flags
{
    SOURCE_READER_FLUSHING = 0x1,
    SOURCE_READER_SEEKING = 0x2,
    SOURCE_READER_SHUTDOWN_ON_RELEASE = 0x4,
};

struct source_reader
{
    IMFSourceReader IMFSourceReader_iface;
    IMFAsyncCallback source_events_callback;
    IMFAsyncCallback stream_events_callback;
    IMFAsyncCallback async_commands_callback;
    LONG refcount;
    IMFMediaSource *source;
    IMFPresentationDescriptor *descriptor;
    DWORD first_audio_stream_index;
    DWORD first_video_stream_index;
    IMFSourceReaderCallback *async_callback;
    unsigned int flags;
    enum media_source_state source_state;
    struct media_stream *streams;
    DWORD stream_count;
    struct list responses;
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE sample_event;
    CONDITION_VARIABLE state_event;
};

static inline struct source_reader *impl_from_IMFSourceReader(IMFSourceReader *iface)
{
    return CONTAINING_RECORD(iface, struct source_reader, IMFSourceReader_iface);
}

static struct source_reader *impl_from_source_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_reader, source_events_callback);
}

static struct source_reader *impl_from_stream_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_reader, stream_events_callback);
}

static struct source_reader *impl_from_async_commands_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct source_reader, async_commands_callback);
}

static struct source_reader_async_command *impl_from_async_command_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct source_reader_async_command, IUnknown_iface);
}

static HRESULT WINAPI source_reader_async_command_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
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

static ULONG WINAPI source_reader_async_command_AddRef(IUnknown *iface)
{
    struct source_reader_async_command *command = impl_from_async_command_IUnknown(iface);
    return InterlockedIncrement(&command->refcount);
}

static ULONG WINAPI source_reader_async_command_Release(IUnknown *iface)
{
    struct source_reader_async_command *command = impl_from_async_command_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&command->refcount);

    if (!refcount)
    {
        if (command->op == SOURCE_READER_ASYNC_SEEK)
            PropVariantClear(&command->u.seek.position);
        heap_free(command);
    }

    return refcount;
}

static const IUnknownVtbl source_reader_async_command_vtbl =
{
    source_reader_async_command_QueryInterface,
    source_reader_async_command_AddRef,
    source_reader_async_command_Release,
};

static HRESULT source_reader_create_async_op(enum source_reader_async_op op, struct source_reader_async_command **ret)
{
    struct source_reader_async_command *command;

    if (!(command = heap_alloc_zero(sizeof(*command))))
        return E_OUTOFMEMORY;

    command->IUnknown_iface.lpVtbl = &source_reader_async_command_vtbl;
    command->op = op;

    *ret = command;

    return S_OK;
}

static HRESULT media_event_get_object(IMFMediaEvent *event, REFIID riid, void **obj)
{
    PROPVARIANT value;
    HRESULT hr;

    PropVariantInit(&value);
    if (FAILED(hr = IMFMediaEvent_GetValue(event, &value)))
    {
        WARN("Failed to get event value, hr %#x.\n", hr);
        return hr;
    }

    if (value.vt != VT_UNKNOWN || !value.u.punkVal)
    {
        WARN("Unexpected value type %d.\n", value.vt);
        PropVariantClear(&value);
        return E_UNEXPECTED;
    }

    hr = IUnknown_QueryInterface(value.u.punkVal, riid, obj);
    PropVariantClear(&value);
    if (FAILED(hr))
    {
        WARN("Unexpected object type.\n");
        return hr;
    }

    return hr;
}

static HRESULT media_stream_get_id(IMFMediaStream *stream, DWORD *id)
{
    IMFStreamDescriptor *sd;
    HRESULT hr;

    if (SUCCEEDED(hr = IMFMediaStream_GetStreamDescriptor(stream, &sd)))
    {
        hr = IMFStreamDescriptor_GetStreamIdentifier(sd, id);
        IMFStreamDescriptor_Release(sd);
    }

    return hr;
}

static HRESULT WINAPI source_reader_callback_QueryInterface(IMFAsyncCallback *iface,
        REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI source_reader_source_events_callback_AddRef(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_source_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_AddRef(&reader->IMFSourceReader_iface);
}

static ULONG WINAPI source_reader_source_events_callback_Release(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_source_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_Release(&reader->IMFSourceReader_iface);
}

static HRESULT WINAPI source_reader_callback_GetParameters(IMFAsyncCallback *iface,
        DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static void source_reader_queue_response(struct source_reader *reader, struct media_stream *stream, HRESULT status,
        DWORD stream_flags, LONGLONG timestamp, IMFSample *sample)
{
    struct source_reader_async_command *command;
    struct stream_response *response;
    HRESULT hr;

    response = heap_alloc_zero(sizeof(*response));
    response->status = status;
    response->stream_index = stream->index;
    response->stream_flags = stream_flags;
    response->timestamp = timestamp;
    response->sample = sample;
    if (response->sample)
        IMFSample_AddRef(response->sample);

    list_add_tail(&reader->responses, &response->entry);
    stream->responses++;

    if (stream->requests)
    {
        if (reader->async_callback)
        {
            if (SUCCEEDED(source_reader_create_async_op(SOURCE_READER_ASYNC_SAMPLE_READY, &command)))
            {
                command->u.sample.stream_index = stream->index;
                if (FAILED(hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &reader->async_commands_callback,
                        &command->IUnknown_iface)))
                    WARN("Failed to submit async result, hr %#x.\n", hr);
                IUnknown_Release(&command->IUnknown_iface);
            }
        }
        else
            WakeAllConditionVariable(&reader->sample_event);

        stream->requests--;
    }
}

static HRESULT source_reader_request_sample(struct source_reader *reader, struct media_stream *stream)
{
    HRESULT hr = S_OK;

    if (stream->stream && !(stream->flags & STREAM_FLAG_SAMPLE_REQUESTED))
    {
        if (FAILED(hr = IMFMediaStream_RequestSample(stream->stream, NULL)))
            WARN("Sample request failed, hr %#x.\n", hr);
        else
        {
            stream->flags |= (STREAM_FLAG_SAMPLE_REQUESTED | STREAM_FLAG_REQUESTED_ONCE);
        }
    }

    return hr;
}

static HRESULT source_reader_new_stream_handler(struct source_reader *reader, IMFMediaEvent *event)
{
    IMFMediaStream *stream;
    unsigned int i;
    DWORD id = 0;
    HRESULT hr;

    if (FAILED(hr = media_event_get_object(event, &IID_IMFMediaStream, (void **)&stream)))
    {
        WARN("Failed to get stream object, hr %#x.\n", hr);
        return hr;
    }

    TRACE("Got new stream %p.\n", stream);

    if (FAILED(hr = media_stream_get_id(stream, &id)))
    {
        WARN("Unidentified stream %p, hr %#x.\n", stream, hr);
        IMFMediaStream_Release(stream);
        return hr;
    }

    EnterCriticalSection(&reader->cs);

    for (i = 0; i < reader->stream_count; ++i)
    {
        if (id == reader->streams[i].id)
        {
            if (!reader->streams[i].stream)
            {
                reader->streams[i].stream = stream;
                IMFMediaStream_AddRef(reader->streams[i].stream);
                if (FAILED(hr = IMFMediaStream_BeginGetEvent(stream, &reader->stream_events_callback,
                        (IUnknown *)stream)))
                {
                    WARN("Failed to subscribe to stream events, hr %#x.\n", hr);
                }

                if (reader->streams[i].requests)
                    if (FAILED(source_reader_request_sample(reader, &reader->streams[i])))
                        WakeAllConditionVariable(&reader->sample_event);
            }
            break;
        }
    }

    if (i == reader->stream_count)
        WARN("Stream with id %#x was not present in presentation descriptor.\n", id);

    LeaveCriticalSection(&reader->cs);

    IMFMediaStream_Release(stream);

    return hr;
}

static HRESULT source_reader_source_state_handler(struct source_reader *reader, MediaEventType event_type)
{
    EnterCriticalSection(&reader->cs);

    switch (event_type)
    {
        case MESourceStarted:
            reader->source_state = SOURCE_STATE_STARTED;
            break;
        case MESourceStopped:
            reader->source_state = SOURCE_STATE_STOPPED;
            break;
        case MESourceSeeked:
            reader->flags &= ~SOURCE_READER_SEEKING;
            break;
        default:
            WARN("Unhandled event %d.\n", event_type);
    }

    LeaveCriticalSection(&reader->cs);

    WakeAllConditionVariable(&reader->state_event);

    return S_OK;
}

static HRESULT WINAPI source_reader_source_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct source_reader *reader = impl_from_source_callback_IMFAsyncCallback(iface);
    MediaEventType event_type;
    IMFMediaSource *source;
    IMFMediaEvent *event;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, result);

    source = (IMFMediaSource *)IMFAsyncResult_GetStateNoAddRef(result);

    if (FAILED(hr = IMFMediaSource_EndGetEvent(source, result, &event)))
        return hr;

    IMFMediaEvent_GetType(event, &event_type);

    TRACE("Got event %u.\n", event_type);

    switch (event_type)
    {
        case MENewStream:
            hr = source_reader_new_stream_handler(reader, event);
            break;
        case MESourceStarted:
        case MESourcePaused:
        case MESourceStopped:
        case MESourceSeeked:
            hr = source_reader_source_state_handler(reader, event_type);
            break;
        case MEBufferingStarted:
        case MEBufferingStopped:
        case MEConnectStart:
        case MEConnectEnd:
        case MEExtendedType:
        case MESourceCharacteristicsChanged:
        case MESourceMetadataChanged:
        case MEContentProtectionMetadata:
        case MEDeviceThermalStateChanged:
            if (reader->async_callback)
                IMFSourceReaderCallback_OnEvent(reader->async_callback, MF_SOURCE_READER_MEDIASOURCE, event);
            break;
        default:
            ;
    }

    if (FAILED(hr))
        WARN("Failed while handling %d event, hr %#x.\n", event_type, hr);

    IMFMediaEvent_Release(event);

    IMFMediaSource_BeginGetEvent(source, iface, (IUnknown *)source);

    return S_OK;
}

static const IMFAsyncCallbackVtbl source_events_callback_vtbl =
{
    source_reader_callback_QueryInterface,
    source_reader_source_events_callback_AddRef,
    source_reader_source_events_callback_Release,
    source_reader_callback_GetParameters,
    source_reader_source_events_callback_Invoke,
};

static ULONG WINAPI source_reader_stream_events_callback_AddRef(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_stream_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_AddRef(&reader->IMFSourceReader_iface);
}

static ULONG WINAPI source_reader_stream_events_callback_Release(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_stream_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_Release(&reader->IMFSourceReader_iface);
}

static HRESULT source_reader_pull_stream_samples(struct source_reader *reader, struct media_stream *stream)
{
    MFT_OUTPUT_STREAM_INFO stream_info = { 0 };
    MFT_OUTPUT_DATA_BUFFER out_buffer;
    IMFMediaBuffer *buffer;
    LONGLONG timestamp;
    DWORD status;
    HRESULT hr;

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(stream->decoder, 0, &stream_info)))
    {
        WARN("Failed to get output stream info, hr %#x.\n", hr);
        return hr;
    }

    for (;;)
    {
        memset(&out_buffer, 0, sizeof(out_buffer));

        if (!(stream_info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES))
        {
            if (FAILED(hr = MFCreateSample(&out_buffer.pSample)))
                break;

            if (FAILED(hr = MFCreateAlignedMemoryBuffer(stream_info.cbSize, stream_info.cbAlignment, &buffer)))
            {
                IMFSample_Release(out_buffer.pSample);
                break;
            }

            IMFSample_AddBuffer(out_buffer.pSample, buffer);
            IMFMediaBuffer_Release(buffer);
        }

        if (FAILED(hr = IMFTransform_ProcessOutput(stream->decoder, 0, 1, &out_buffer, &status)))
        {
            if (out_buffer.pSample)
                IMFSample_Release(out_buffer.pSample);
            break;
        }

        timestamp = 0;
        if (FAILED(IMFSample_GetSampleTime(out_buffer.pSample, &timestamp)))
            WARN("Sample time wasn't set.\n");

        source_reader_queue_response(reader, stream, S_OK /* FIXME */, 0, timestamp, out_buffer.pSample);
        if (out_buffer.pSample)
            IMFSample_Release(out_buffer.pSample);
        if (out_buffer.pEvents)
            IMFCollection_Release(out_buffer.pEvents);
    }

    return hr;
}

static HRESULT source_reader_process_sample(struct source_reader *reader, struct media_stream *stream,
        IMFSample *sample)
{
    LONGLONG timestamp;
    HRESULT hr;

    if (!stream->decoder)
    {
        timestamp = 0;
        if (FAILED(IMFSample_GetSampleTime(sample, &timestamp)))
            WARN("Sample time wasn't set.\n");

        source_reader_queue_response(reader, stream, S_OK, 0, timestamp, sample);
        return S_OK;
    }

    /* It's assumed that decoder has 1 input and 1 output, both id's are 0. */

    hr = source_reader_pull_stream_samples(reader, stream);
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
        if (FAILED(hr = IMFTransform_ProcessInput(stream->decoder, 0, sample, 0)))
        {
            WARN("Transform failed to process input, hr %#x.\n", hr);
            return hr;
        }

        if ((hr = source_reader_pull_stream_samples(reader, stream)) == MF_E_TRANSFORM_NEED_MORE_INPUT)
            return S_OK;
    }
    else
        WARN("Transform failed to process output, hr %#x.\n", hr);

    return hr;
}

static HRESULT source_reader_media_sample_handler(struct source_reader *reader, IMFMediaStream *stream,
        IMFMediaEvent *event)
{
    IMFSample *sample;
    unsigned int i;
    DWORD id = 0;
    HRESULT hr;

    TRACE("Got new sample for stream %p.\n", stream);

    if (FAILED(hr = media_event_get_object(event, &IID_IMFSample, (void **)&sample)))
    {
        WARN("Failed to get sample object, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = media_stream_get_id(stream, &id)))
    {
        WARN("Unidentified stream %p, hr %#x.\n", stream, hr);
        IMFSample_Release(sample);
        return hr;
    }

    EnterCriticalSection(&reader->cs);

    for (i = 0; i < reader->stream_count; ++i)
    {
        if (id == reader->streams[i].id)
        {
            /* FIXME: propagate processing errors? */

            reader->streams[i].flags &= ~STREAM_FLAG_SAMPLE_REQUESTED;
            hr = source_reader_process_sample(reader, &reader->streams[i], sample);
            if (reader->streams[i].requests)
                source_reader_request_sample(reader, &reader->streams[i]);

            break;
        }
    }

    if (i == reader->stream_count)
        WARN("Stream with id %#x was not present in presentation descriptor.\n", id);

    LeaveCriticalSection(&reader->cs);

    IMFSample_Release(sample);

    return hr;
}

static HRESULT source_reader_media_stream_state_handler(struct source_reader *reader, IMFMediaStream *stream,
        IMFMediaEvent *event)
{
    MediaEventType event_type;
    LONGLONG timestamp;
    PROPVARIANT value;
    unsigned int i;
    HRESULT hr;
    DWORD id;

    IMFMediaEvent_GetType(event, &event_type);

    if (FAILED(hr = media_stream_get_id(stream, &id)))
    {
        WARN("Unidentified stream %p, hr %#x.\n", stream, hr);
        return hr;
    }

    EnterCriticalSection(&reader->cs);

    for (i = 0; i < reader->stream_count; ++i)
    {
        struct media_stream *stream = &reader->streams[i];

        if (id == stream->id)
        {
            switch (event_type)
            {
                case MEEndOfStream:
                    stream->state = STREAM_STATE_EOS;
                    stream->flags &= ~STREAM_FLAG_SAMPLE_REQUESTED;

                    if (stream->decoder && SUCCEEDED(IMFTransform_ProcessMessage(stream->decoder,
                            MFT_MESSAGE_COMMAND_DRAIN, 0)))
                    {
                        if ((hr = source_reader_pull_stream_samples(reader, stream)) != MF_E_TRANSFORM_NEED_MORE_INPUT)
                            WARN("Failed to pull pending samples, hr %#x.\n", hr);
                    }

                    while (stream->requests)
                        source_reader_queue_response(reader, stream, S_OK, MF_SOURCE_READERF_ENDOFSTREAM, 0, NULL);

                    break;
                case MEStreamSeeked:
                case MEStreamStarted:
                    stream->state = STREAM_STATE_READY;
                    break;
                case MEStreamTick:
                    value.vt = VT_EMPTY;
                    hr = SUCCEEDED(IMFMediaEvent_GetValue(event, &value)) && value.vt == VT_I8 ? S_OK : E_UNEXPECTED;
                    timestamp = SUCCEEDED(hr) ? value.u.hVal.QuadPart : 0;
                    PropVariantClear(&value);

                    source_reader_queue_response(reader, stream, hr, MF_SOURCE_READERF_STREAMTICK, timestamp, NULL);

                    break;
                default:
                    ;
            }

            break;
        }
    }

    LeaveCriticalSection(&reader->cs);

    return S_OK;
}

static HRESULT WINAPI source_reader_stream_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct source_reader *reader = impl_from_stream_callback_IMFAsyncCallback(iface);
    MediaEventType event_type;
    IMFMediaStream *stream;
    IMFMediaEvent *event;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, result);

    stream = (IMFMediaStream *)IMFAsyncResult_GetStateNoAddRef(result);

    if (FAILED(hr = IMFMediaStream_EndGetEvent(stream, result, &event)))
        return hr;

    IMFMediaEvent_GetType(event, &event_type);

    TRACE("Got event %u.\n", event_type);

    switch (event_type)
    {
        case MEMediaSample:
            hr = source_reader_media_sample_handler(reader, stream, event);
            break;
        case MEStreamSeeked:
        case MEStreamStarted:
        case MEStreamTick:
        case MEEndOfStream:
            hr = source_reader_media_stream_state_handler(reader, stream, event);
            break;
        default:
            ;
    }

    if (FAILED(hr))
        WARN("Failed while handling %d event, hr %#x.\n", event_type, hr);

    IMFMediaEvent_Release(event);

    IMFMediaStream_BeginGetEvent(stream, iface, (IUnknown *)stream);

    return S_OK;
}

static const IMFAsyncCallbackVtbl stream_events_callback_vtbl =
{
    source_reader_callback_QueryInterface,
    source_reader_stream_events_callback_AddRef,
    source_reader_stream_events_callback_Release,
    source_reader_callback_GetParameters,
    source_reader_stream_events_callback_Invoke,
};

static ULONG WINAPI source_reader_async_commands_callback_AddRef(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_AddRef(&reader->IMFSourceReader_iface);
}

static ULONG WINAPI source_reader_async_commands_callback_Release(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    return IMFSourceReader_Release(&reader->IMFSourceReader_iface);
}

static struct stream_response * media_stream_detach_response(struct source_reader *reader, struct stream_response *response)
{
    struct media_stream *stream;

    list_remove(&response->entry);

    if (response->stream_index < reader->stream_count)
    {
        stream = &reader->streams[response->stream_index];
        if (stream->responses)
            --stream->responses;
    }

    return response;
}

static struct stream_response *media_stream_pop_response(struct source_reader *reader, struct media_stream *stream)
{
    struct stream_response *response;
    struct list *head;

    if (stream)
    {
        LIST_FOR_EACH_ENTRY(response, &reader->responses, struct stream_response, entry)
        {
            if (response->stream_index == stream->index)
                return media_stream_detach_response(reader, response);
        }
    }
    else
    {
        if ((head = list_head(&reader->responses)))
            return media_stream_detach_response(reader, LIST_ENTRY(head, struct stream_response, entry));
    }

    return NULL;
}

static void source_reader_release_response(struct stream_response *response)
{
    if (response->sample)
        IMFSample_Release(response->sample);
    heap_free(response);
}

static HRESULT source_reader_get_stream_selection(const struct source_reader *reader, DWORD index, BOOL *selected)
{
    IMFStreamDescriptor *sd;

    if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(reader->descriptor, index, selected, &sd)))
        return MF_E_INVALIDSTREAMNUMBER;
    IMFStreamDescriptor_Release(sd);

    return S_OK;
}

static HRESULT source_reader_start_source(struct source_reader *reader)
{
    BOOL selected, selection_changed = FALSE;
    PROPVARIANT position;
    HRESULT hr = S_OK;
    unsigned int i;

    for (i = 0; i < reader->stream_count; ++i)
    {
        source_reader_get_stream_selection(reader, i, &selected);
        if (selected)
            reader->streams[i].flags |= STREAM_FLAG_SELECTED;
        else
            reader->streams[i].flags &= ~STREAM_FLAG_SELECTED;
    }

    if (reader->source_state == SOURCE_STATE_STARTED)
    {
        for (i = 0; i < reader->stream_count; ++i)
        {
            selection_changed = !!(reader->streams[i].flags & STREAM_FLAG_SELECTED) ^
                    !!(reader->streams[i].flags & STREAM_FLAG_PRESENTED);
            if (selection_changed)
                break;
        }
    }

    position.u.hVal.QuadPart = 0;
    if (reader->source_state != SOURCE_STATE_STARTED || selection_changed)
    {
        position.vt = reader->source_state == SOURCE_STATE_STARTED ? VT_EMPTY : VT_I8;

        /* Update cached stream selection if descriptor was accepted. */
        if (SUCCEEDED(hr = IMFMediaSource_Start(reader->source, reader->descriptor, &GUID_NULL, &position)))
        {
            for (i = 0; i < reader->stream_count; ++i)
            {
                if (reader->streams[i].flags & STREAM_FLAG_SELECTED)
                    reader->streams[i].flags |= STREAM_FLAG_PRESENTED;
            }
        }
    }

    return hr;
}

static BOOL source_reader_got_response_for_stream(struct source_reader *reader, struct media_stream *stream)
{
    struct stream_response *response;

    LIST_FOR_EACH_ENTRY(response, &reader->responses, struct stream_response, entry)
    {
        if (response->stream_index == stream->index)
            return TRUE;
    }

    return FALSE;
}

static BOOL source_reader_get_read_result(struct source_reader *reader, struct media_stream *stream, DWORD flags,
        HRESULT *status, DWORD *stream_index, DWORD *stream_flags, LONGLONG *timestamp, IMFSample **sample)
{
    struct stream_response *response = NULL;
    BOOL request_sample = FALSE;

    if ((response = media_stream_pop_response(reader, stream)))
    {
        *status = response->status;
        *stream_index = stream->index;
        *stream_flags = response->stream_flags;
        *timestamp = response->timestamp;
        *sample = response->sample;
        if (*sample)
            IMFSample_AddRef(*sample);

        source_reader_release_response(response);
    }
    else
    {
        *status = S_OK;
        *stream_index = stream->index;
        *timestamp = 0;
        *sample = NULL;

        if (stream->state == STREAM_STATE_EOS)
        {
            *stream_flags = MF_SOURCE_READERF_ENDOFSTREAM;
        }
        else
        {
            request_sample = !(flags & MF_SOURCE_READER_CONTROLF_DRAIN);
            *stream_flags = 0;
        }
    }

    return !request_sample;
}

static HRESULT source_reader_get_first_selected_stream(struct source_reader *reader, unsigned int flags,
        unsigned int *stream_index)
{
    unsigned int i, first_selected = ~0u;
    BOOL selected, stream_drained;

    for (i = 0; i < reader->stream_count; ++i)
    {
        stream_drained = reader->streams[i].state == STREAM_STATE_EOS && !reader->streams[i].responses;
        selected = SUCCEEDED(source_reader_get_stream_selection(reader, i, &selected)) && selected;

        if (selected && !(reader->streams[i].flags & flags))
        {
            if (first_selected == ~0u)
                first_selected = i;

            if (!stream_drained)
            {
                *stream_index = i;
                break;
            }
        }
    }

    /* If all selected streams reached EOS, use first selected. This fallback only applies after reader went through all
       selected streams once. */
    if (i == reader->stream_count && first_selected != ~0u && !flags)
    {
        *stream_index = first_selected;
        i = first_selected;
    }

    return i == reader->stream_count ? MF_E_MEDIA_SOURCE_NO_STREAMS_SELECTED : S_OK;
}

static HRESULT source_reader_get_stream_read_index(struct source_reader *reader, unsigned int index, unsigned int *stream_index)
{
    BOOL selected;
    HRESULT hr;

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            *stream_index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            *stream_index = reader->first_audio_stream_index;
            break;
        case MF_SOURCE_READER_ANY_STREAM:
            if (reader->async_callback)
            {
                /* Pick first selected stream. */
                hr = source_reader_get_first_selected_stream(reader, 0, stream_index);
            }
            else
            {
                /* Cycle through all selected streams once, next pick first selected. */
                if (FAILED(hr = source_reader_get_first_selected_stream(reader, STREAM_FLAG_REQUESTED_ONCE, stream_index)))
                    hr = source_reader_get_first_selected_stream(reader, 0, stream_index);
            }
            return hr;
        default:
            *stream_index = index;
    }

    /* Can't read from deselected streams. */
    if (SUCCEEDED(hr = source_reader_get_stream_selection(reader, *stream_index, &selected)) && !selected)
        hr = MF_E_INVALIDREQUEST;

    return hr;
}

static void source_reader_release_responses(struct source_reader *reader, struct media_stream *stream)
{
    struct stream_response *ptr, *next;

    LIST_FOR_EACH_ENTRY_SAFE(ptr, next, &reader->responses, struct stream_response, entry)
    {
        if (stream && stream->index != ptr->stream_index &&
                ptr->stream_index != MF_SOURCE_READER_FIRST_VIDEO_STREAM &&
                ptr->stream_index != MF_SOURCE_READER_FIRST_AUDIO_STREAM &&
                ptr->stream_index != MF_SOURCE_READER_ANY_STREAM)
        {
            continue;
        }
        media_stream_detach_response(reader, ptr);
        source_reader_release_response(ptr);
    }
}

static void source_reader_flush_stream(struct source_reader *reader, DWORD stream_index)
{
    struct media_stream *stream = &reader->streams[stream_index];

    source_reader_release_responses(reader, stream);
    if (stream->decoder)
        IMFTransform_ProcessMessage(stream->decoder, MFT_MESSAGE_COMMAND_FLUSH, 0);
    stream->requests = 0;
}

static HRESULT source_reader_flush(struct source_reader *reader, unsigned int index)
{
    unsigned int stream_index;
    HRESULT hr = S_OK;

    if (index == MF_SOURCE_READER_ALL_STREAMS)
    {
        for (stream_index = 0; stream_index < reader->stream_count; ++stream_index)
            source_reader_flush_stream(reader, stream_index);
    }
    else
    {
        switch (index)
        {
            case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
                stream_index = reader->first_video_stream_index;
                break;
            case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
                stream_index = reader->first_audio_stream_index;
                break;
            default:
                stream_index = index;
        }

        if (stream_index < reader->stream_count)
            source_reader_flush_stream(reader, stream_index);
        else
            hr = MF_E_INVALIDSTREAMNUMBER;
    }

    return hr;
}

static HRESULT WINAPI source_reader_async_commands_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct source_reader *reader = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    struct media_stream *stream, stub_stream = { .requests = 1 };
    struct source_reader_async_command *command;
    struct stream_response *response;
    DWORD stream_index, stream_flags;
    BOOL report_sample = FALSE;
    IMFSample *sample = NULL;
    LONGLONG timestamp = 0;
    HRESULT hr, status;
    IUnknown *state;

    if (FAILED(hr = IMFAsyncResult_GetState(result, &state)))
        return hr;

    command = impl_from_async_command_IUnknown(state);

    switch (command->op)
    {
        case SOURCE_READER_ASYNC_READ:
            EnterCriticalSection(&reader->cs);

            if (SUCCEEDED(hr = source_reader_start_source(reader)))
            {
                if (SUCCEEDED(hr = source_reader_get_stream_read_index(reader, command->u.read.stream_index, &stream_index)))
                {
                    stream = &reader->streams[stream_index];

                    if (!(report_sample = source_reader_get_read_result(reader, stream, command->u.read.flags, &status,
                            &stream_index, &stream_flags, &timestamp, &sample)))
                    {
                        stream->requests++;
                        source_reader_request_sample(reader, stream);
                        /* FIXME: set error stream/reader state on request failure */
                    }
                }
                else
                {
                    stub_stream.index = command->u.read.stream_index;
                    source_reader_queue_response(reader, &stub_stream, hr, MF_SOURCE_READERF_ERROR, 0, NULL);
                }
            }

            LeaveCriticalSection(&reader->cs);

            if (report_sample)
                IMFSourceReaderCallback_OnReadSample(reader->async_callback, status, stream_index, stream_flags,
                        timestamp, sample);

            if (sample)
                IMFSample_Release(sample);

            break;

        case SOURCE_READER_ASYNC_SEEK:

            EnterCriticalSection(&reader->cs);
            if (SUCCEEDED(IMFMediaSource_Start(reader->source, reader->descriptor, &command->u.seek.format,
                    &command->u.seek.position)))
            {
                reader->flags |= SOURCE_READER_SEEKING;
            }
            LeaveCriticalSection(&reader->cs);

            break;

        case SOURCE_READER_ASYNC_SAMPLE_READY:

            EnterCriticalSection(&reader->cs);
            response = media_stream_pop_response(reader, NULL);
            LeaveCriticalSection(&reader->cs);

            if (response)
            {
                IMFSourceReaderCallback_OnReadSample(reader->async_callback, response->status, response->stream_index,
                        response->stream_flags, response->timestamp, response->sample);
                source_reader_release_response(response);
            }

            break;
        case SOURCE_READER_ASYNC_FLUSH:
            EnterCriticalSection(&reader->cs);
            source_reader_flush(reader, command->u.flush.stream_index);
            reader->flags &= ~SOURCE_READER_FLUSHING;
            LeaveCriticalSection(&reader->cs);

            IMFSourceReaderCallback_OnFlush(reader->async_callback, command->u.flush.stream_index);
            break;
        default:
            ;
    }

    IUnknown_Release(state);

    return S_OK;
}

static const IMFAsyncCallbackVtbl async_commands_callback_vtbl =
{
    source_reader_callback_QueryInterface,
    source_reader_async_commands_callback_AddRef,
    source_reader_async_commands_callback_Release,
    source_reader_callback_GetParameters,
    source_reader_async_commands_callback_Invoke,
};

static HRESULT WINAPI src_reader_QueryInterface(IMFSourceReader *iface, REFIID riid, void **out)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFSourceReader))
    {
        *out = &reader->IMFSourceReader_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI src_reader_AddRef(IMFSourceReader *iface)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    ULONG refcount = InterlockedIncrement(&reader->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI src_reader_Release(IMFSourceReader *iface)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    ULONG refcount = InterlockedDecrement(&reader->refcount);
    unsigned int i;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (reader->async_callback)
            IMFSourceReaderCallback_Release(reader->async_callback);
        if (reader->flags & SOURCE_READER_SHUTDOWN_ON_RELEASE)
            IMFMediaSource_Shutdown(reader->source);
        if (reader->descriptor)
            IMFPresentationDescriptor_Release(reader->descriptor);
        IMFMediaSource_Release(reader->source);

        for (i = 0; i < reader->stream_count; ++i)
        {
            struct media_stream *stream = &reader->streams[i];

            if (stream->stream)
                IMFMediaStream_Release(stream->stream);
            if (stream->current)
                IMFMediaType_Release(stream->current);
            if (stream->decoder)
                IMFTransform_Release(stream->decoder);
        }
        source_reader_release_responses(reader, NULL);
        heap_free(reader->streams);
        DeleteCriticalSection(&reader->cs);
        heap_free(reader);
    }

    return refcount;
}

static HRESULT WINAPI src_reader_GetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL *selected)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);

    TRACE("%p, %#x, %p.\n", iface, index, selected);

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        default:
            ;
    }

    return source_reader_get_stream_selection(reader, index, selected);
}

static HRESULT WINAPI src_reader_SetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL selection)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    HRESULT hr = S_OK;
    BOOL selection_changed = FALSE, selected;
    unsigned int i;

    TRACE("%p, %#x, %d.\n", iface, index, selection);

    selection = !!selection;

    EnterCriticalSection(&reader->cs);

    if (index == MF_SOURCE_READER_ALL_STREAMS)
    {
        for (i = 0; i < reader->stream_count; ++i)
        {
            if (!selection_changed)
            {
                source_reader_get_stream_selection(reader, i, &selected);
                if (selected ^ selection)
                    selection_changed = TRUE;
            }

            if (selection)
                IMFPresentationDescriptor_SelectStream(reader->descriptor, i);
            else
                IMFPresentationDescriptor_DeselectStream(reader->descriptor, i);
        }
    }
    else
    {
        switch (index)
        {
            case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
                index = reader->first_video_stream_index;
                break;
            case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
                index = reader->first_audio_stream_index;
                break;
            default:
                ;
        }

        source_reader_get_stream_selection(reader, index, &selected);
        if (selected ^ selection)
            selection_changed = TRUE;

        if (selection)
            hr = IMFPresentationDescriptor_SelectStream(reader->descriptor, index);
        else
            hr = IMFPresentationDescriptor_DeselectStream(reader->descriptor, index);
    }

    if (SUCCEEDED(hr) && selection_changed)
    {
        for (i = 0; i < reader->stream_count; ++i)
            reader->streams[i].flags &= ~STREAM_FLAG_REQUESTED_ONCE;
    }

    LeaveCriticalSection(&reader->cs);

    return SUCCEEDED(hr) ? S_OK : MF_E_INVALIDSTREAMNUMBER;
}

static HRESULT source_reader_get_native_media_type(struct source_reader *reader, DWORD index, DWORD type_index,
        IMFMediaType **type)
{
    IMFMediaTypeHandler *handler;
    IMFStreamDescriptor *sd;
    IMFMediaType *src_type;
    BOOL selected;
    HRESULT hr;

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        default:
            ;
    }

    if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(reader->descriptor, index, &selected, &sd)))
        return MF_E_INVALIDSTREAMNUMBER;

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
    IMFStreamDescriptor_Release(sd);
    if (FAILED(hr))
        return hr;

    if (type_index == MF_SOURCE_READER_CURRENT_TYPE_INDEX)
        hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &src_type);
    else
        hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, type_index, &src_type);
    IMFMediaTypeHandler_Release(handler);

    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = MFCreateMediaType(type)))
            hr = IMFMediaType_CopyAllItems(src_type, (IMFAttributes *)*type);
        IMFMediaType_Release(src_type);
    }

    return hr;
}

static HRESULT WINAPI src_reader_GetNativeMediaType(IMFSourceReader *iface, DWORD index, DWORD type_index,
            IMFMediaType **type)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);

    TRACE("%p, %#x, %#x, %p.\n", iface, index, type_index, type);

    return source_reader_get_native_media_type(reader, index, type_index, type);
}

static HRESULT WINAPI src_reader_GetCurrentMediaType(IMFSourceReader *iface, DWORD index, IMFMediaType **type)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    HRESULT hr;

    TRACE("%p, %#x, %p.\n", iface, index, type);

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        default:
            ;
    }

    if (index >= reader->stream_count)
        return MF_E_INVALIDSTREAMNUMBER;

    if (FAILED(hr = MFCreateMediaType(type)))
        return hr;

    EnterCriticalSection(&reader->cs);

    hr = IMFMediaType_CopyAllItems(reader->streams[index].current, (IMFAttributes *)*type);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT source_reader_get_source_type_handler(struct source_reader *reader, DWORD index,
        IMFMediaTypeHandler **handler)
{
    IMFStreamDescriptor *sd;
    BOOL selected;
    HRESULT hr;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(reader->descriptor, index, &selected, &sd)))
        return hr;

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, handler);
    IMFStreamDescriptor_Release(sd);

    return hr;
}

static HRESULT source_reader_set_compatible_media_type(struct source_reader *reader, DWORD index, IMFMediaType *type)
{
    IMFMediaTypeHandler *type_handler;
    IMFMediaType *native_type;
    BOOL type_set = FALSE;
    unsigned int i = 0;
    DWORD flags;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_IsEqual(type, reader->streams[index].current, &flags)))
        return hr;

    if (!(flags & MF_MEDIATYPE_EQUAL_MAJOR_TYPES))
        return MF_E_INVALIDMEDIATYPE;

    /* No need for a decoder or type change. */
    if (flags & MF_MEDIATYPE_EQUAL_FORMAT_TYPES)
        return S_OK;

    if (FAILED(hr = source_reader_get_source_type_handler(reader, index, &type_handler)))
        return hr;

    while (!type_set && IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, i++, &native_type) == S_OK)
    {
        static const DWORD compare_flags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES;

        if (SUCCEEDED(IMFMediaType_IsEqual(native_type, type, &flags)) && (flags & compare_flags) == compare_flags)
        {
            if ((type_set = SUCCEEDED(IMFMediaTypeHandler_SetCurrentMediaType(type_handler, native_type))))
                IMFMediaType_CopyAllItems(native_type, (IMFAttributes *)reader->streams[index].current);
        }

        IMFMediaType_Release(native_type);
    }

    IMFMediaTypeHandler_Release(type_handler);

    return type_set ? S_OK : S_FALSE;
}

static HRESULT source_reader_configure_decoder(struct source_reader *reader, DWORD index, const CLSID *clsid,
        IMFMediaType *input_type, IMFMediaType *output_type)
{
    IMFMediaTypeHandler *type_handler;
    IMFTransform *transform = NULL;
    IMFMediaType *type = NULL;
    DWORD flags;
    HRESULT hr;
    int i = 0;

    if (FAILED(hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void **)&transform)))
    {
        WARN("Failed to create transform object, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = IMFTransform_SetInputType(transform, 0, input_type, 0)))
    {
        WARN("Failed to set decoder input type, hr %#x.\n", hr);
        IMFTransform_Release(transform);
        return hr;
    }

    /* Find the relevant output type. */
    while (IMFTransform_GetOutputAvailableType(transform, 0, i++, &type) == S_OK)
    {
        flags = 0;

        if (SUCCEEDED(IMFMediaType_IsEqual(type, output_type, &flags)))
        {
            if (flags & MF_MEDIATYPE_EQUAL_FORMAT_TYPES)
            {
                if (SUCCEEDED(IMFTransform_SetOutputType(transform, 0, type, 0)))
                {
                    if (SUCCEEDED(source_reader_get_source_type_handler(reader, index, &type_handler)))
                    {
                        IMFMediaTypeHandler_SetCurrentMediaType(type_handler, input_type);
                        IMFMediaTypeHandler_Release(type_handler);
                    }

                    if (FAILED(hr = IMFMediaType_CopyAllItems(type, (IMFAttributes *)reader->streams[index].current)))
                        WARN("Failed to copy attributes, hr %#x.\n", hr);
                    IMFMediaType_Release(type);

                    if (reader->streams[index].decoder)
                        IMFTransform_Release(reader->streams[index].decoder);

                    reader->streams[index].decoder = transform;

                    return S_OK;
                }
            }
        }

        IMFMediaType_Release(type);
    }

    WARN("Failed to find suitable decoder output type.\n");

    IMFTransform_Release(transform);

    return MF_E_TOPO_CODEC_NOT_FOUND;
}

static HRESULT source_reader_create_decoder_for_stream(struct source_reader *reader, DWORD index, IMFMediaType *output_type)
{
    MFT_REGISTER_TYPE_INFO in_type, out_type;
    CLSID *clsids, mft_clsid, category;
    unsigned int i = 0, count;
    IMFMediaType *input_type;
    HRESULT hr;

    /* TODO: should we check if the source type is compressed? */

    if (FAILED(hr = IMFMediaType_GetMajorType(output_type, &out_type.guidMajorType)))
        return hr;

    if (IsEqualGUID(&out_type.guidMajorType, &MFMediaType_Video))
    {
        category = MFT_CATEGORY_VIDEO_DECODER;
    }
    else if (IsEqualGUID(&out_type.guidMajorType, &MFMediaType_Audio))
    {
        category = MFT_CATEGORY_AUDIO_DECODER;
    }
    else
    {
        WARN("Unhandled major type %s.\n", debugstr_guid(&out_type.guidMajorType));
        return MF_E_TOPO_CODEC_NOT_FOUND;
    }

    if (FAILED(hr = IMFMediaType_GetGUID(output_type, &MF_MT_SUBTYPE, &out_type.guidSubtype)))
        return hr;

    in_type.guidMajorType = out_type.guidMajorType;

    while (source_reader_get_native_media_type(reader, index, i++, &input_type) == S_OK)
    {
        if (SUCCEEDED(IMFMediaType_GetGUID(input_type, &MF_MT_SUBTYPE, &in_type.guidSubtype)))
        {
            count = 0;
            if (SUCCEEDED(hr = MFTEnum(category, 0, &in_type, &out_type, NULL, &clsids, &count)) && count)
            {
                mft_clsid = clsids[0];
                CoTaskMemFree(clsids);

                /* TODO: Should we iterate over all of them? */
                if (SUCCEEDED(source_reader_configure_decoder(reader, index, &mft_clsid, input_type, output_type)))
                {
                    IMFMediaType_Release(input_type);
                    return S_OK;
                }

            }
        }

        IMFMediaType_Release(input_type);
    }

    return MF_E_TOPO_CODEC_NOT_FOUND;
}

static HRESULT WINAPI src_reader_SetCurrentMediaType(IMFSourceReader *iface, DWORD index, DWORD *reserved,
        IMFMediaType *type)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    HRESULT hr;

    TRACE("%p, %#x, %p, %p.\n", iface, index, reserved, type);

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        default:
            ;
    }

    if (index >= reader->stream_count)
        return MF_E_INVALIDSTREAMNUMBER;

    /* FIXME: setting the output type while streaming should trigger a flush */

    EnterCriticalSection(&reader->cs);

    if ((hr = source_reader_set_compatible_media_type(reader, index, type)) == S_FALSE)
        hr = source_reader_create_decoder_for_stream(reader, index, type);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT WINAPI src_reader_SetCurrentPosition(IMFSourceReader *iface, REFGUID format, REFPROPVARIANT position)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    struct source_reader_async_command *command;
    unsigned int i, flags;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(format), position);

    if (FAILED(hr = IMFMediaSource_GetCharacteristics(reader->source, &flags)))
        return hr;

    if (!(flags & MFMEDIASOURCE_CAN_SEEK))
        return MF_E_INVALIDREQUEST;

    EnterCriticalSection(&reader->cs);

    /* Check if we got pending requests. */
    for (i = 0; i < reader->stream_count; ++i)
    {
        if (reader->streams[i].requests)
        {
            hr = MF_E_INVALIDREQUEST;
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (reader->async_callback)
        {
            if (SUCCEEDED(hr = source_reader_create_async_op(SOURCE_READER_ASYNC_SEEK, &command)))
            {
                command->u.seek.format = *format;
                PropVariantCopy(&command->u.seek.position, position);

                hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &reader->async_commands_callback,
                        &command->IUnknown_iface);
                IUnknown_Release(&command->IUnknown_iface);
            }
        }
        else
        {
            if (SUCCEEDED(IMFMediaSource_Start(reader->source, reader->descriptor, format, position)))
            {
                reader->flags |= SOURCE_READER_SEEKING;
                while (reader->flags & SOURCE_READER_SEEKING)
                {
                    SleepConditionVariableCS(&reader->state_event, &reader->cs, INFINITE);
                }
            }
        }
    }

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT source_reader_read_sample(struct source_reader *reader, DWORD index, DWORD flags, DWORD *actual_index,
        DWORD *stream_flags, LONGLONG *timestamp, IMFSample **sample)
{
    unsigned int actual_index_tmp;
    struct media_stream *stream;
    LONGLONG timestamp_tmp;
    DWORD stream_index;
    HRESULT hr = S_OK;

    if (!stream_flags || !sample)
        return E_POINTER;

    *sample = NULL;

    if (!timestamp)
        timestamp = &timestamp_tmp;

    if (!actual_index)
        actual_index = &actual_index_tmp;

    if (SUCCEEDED(hr = source_reader_start_source(reader)))
    {
        if (SUCCEEDED(hr = source_reader_get_stream_read_index(reader, index, &stream_index)))
        {
            *actual_index = stream_index;

            stream = &reader->streams[stream_index];

            if (!source_reader_get_read_result(reader, stream, flags, &hr, actual_index, stream_flags,
                   timestamp, sample))
            {
                while (!source_reader_got_response_for_stream(reader, stream) && stream->state != STREAM_STATE_EOS)
                {
                    stream->requests++;
                    if (FAILED(hr = source_reader_request_sample(reader, stream)))
                        WARN("Failed to request a sample, hr %#x.\n", hr);
                    if (stream->stream && !(stream->flags & STREAM_FLAG_SAMPLE_REQUESTED))
                    {
                        *stream_flags = MF_SOURCE_READERF_ERROR;
                        *timestamp = 0;
                        break;
                    }
                    SleepConditionVariableCS(&reader->sample_event, &reader->cs, INFINITE);
                }
                if (SUCCEEDED(hr))
                    source_reader_get_read_result(reader, stream, flags, &hr, actual_index, stream_flags,
                       timestamp, sample);
            }
        }
        else
        {
            *actual_index = index;
            *stream_flags = MF_SOURCE_READERF_ERROR;
            *timestamp = 0;
        }
    }

    TRACE("Stream %u, got sample %p, flags %#x.\n", *actual_index, *sample, *stream_flags);

    return hr;
}

static HRESULT source_reader_read_sample_async(struct source_reader *reader, unsigned int index, unsigned int flags,
        unsigned int *actual_index, unsigned int *stream_flags, LONGLONG *timestamp, IMFSample **sample)
{
    struct source_reader_async_command *command;
    HRESULT hr;

    if (actual_index || stream_flags || timestamp || sample)
        return E_INVALIDARG;

    if (reader->flags & SOURCE_READER_FLUSHING)
        hr = MF_E_NOTACCEPTING;
    else
    {
        if (SUCCEEDED(hr = source_reader_create_async_op(SOURCE_READER_ASYNC_READ, &command)))
        {
            command->u.read.stream_index = index;
            command->u.read.flags = flags;

            hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &reader->async_commands_callback, &command->IUnknown_iface);
            IUnknown_Release(&command->IUnknown_iface);
        }
    }

    return hr;
}

static HRESULT WINAPI src_reader_ReadSample(IMFSourceReader *iface, DWORD index, DWORD flags, DWORD *actual_index,
        DWORD *stream_flags, LONGLONG *timestamp, IMFSample **sample)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    HRESULT hr;

    TRACE("%p, %#x, %#x, %p, %p, %p, %p\n", iface, index, flags, actual_index, stream_flags, timestamp, sample);

    EnterCriticalSection(&reader->cs);

    while (reader->flags & SOURCE_READER_SEEKING)
    {
        SleepConditionVariableCS(&reader->state_event, &reader->cs, INFINITE);
    }

    if (reader->async_callback)
        hr = source_reader_read_sample_async(reader, index, flags, actual_index, stream_flags, timestamp, sample);
    else
        hr = source_reader_read_sample(reader, index, flags, actual_index, stream_flags, timestamp, sample);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT source_reader_flush_async(struct source_reader *reader, unsigned int index)
{
    struct source_reader_async_command *command;
    unsigned int stream_index;
    HRESULT hr;

    if (reader->flags & SOURCE_READER_FLUSHING)
        return MF_E_INVALIDREQUEST;

    switch (index)
    {
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            stream_index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            stream_index = reader->first_audio_stream_index;
            break;
        default:
            stream_index = index;
    }

    reader->flags |= SOURCE_READER_FLUSHING;

    if (stream_index != MF_SOURCE_READER_ALL_STREAMS && stream_index >= reader->stream_count)
        return MF_E_INVALIDSTREAMNUMBER;

    if (FAILED(hr = source_reader_create_async_op(SOURCE_READER_ASYNC_FLUSH, &command)))
        return hr;

    command->u.flush.stream_index = stream_index;

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &reader->async_commands_callback, &command->IUnknown_iface);
    IUnknown_Release(&command->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI src_reader_Flush(IMFSourceReader *iface, DWORD index)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    HRESULT hr;

    TRACE("%p, %#x.\n", iface, index);

    EnterCriticalSection(&reader->cs);

    if (reader->async_callback)
        hr = source_reader_flush_async(reader, index);
    else
        hr = source_reader_flush(reader, index);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT WINAPI src_reader_GetServiceForStream(IMFSourceReader *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    IUnknown *obj = NULL;
    HRESULT hr;

    TRACE("%p, %#x, %s, %s, %p\n", iface, index, debugstr_guid(service), debugstr_guid(riid), object);

    switch (index)
    {
        case MF_SOURCE_READER_MEDIASOURCE:
            obj = (IUnknown *)reader->source;
            break;
        default:
            FIXME("Unsupported index %#x.\n", index);
            return E_NOTIMPL;
    }

    if (IsEqualGUID(service, &GUID_NULL))
    {
        hr = IUnknown_QueryInterface(obj, riid, object);
    }
    else
    {
        IMFGetService *gs;

        hr = IUnknown_QueryInterface(obj, &IID_IMFGetService, (void **)&gs);
        if (SUCCEEDED(hr))
        {
            hr = IMFGetService_GetService(gs, service, riid, object);
            IMFGetService_Release(gs);
        }
    }

    return hr;
}

static HRESULT WINAPI src_reader_GetPresentationAttribute(IMFSourceReader *iface, DWORD index,
        REFGUID guid, PROPVARIANT *value)
{
    struct source_reader *reader = impl_from_IMFSourceReader(iface);
    IMFStreamDescriptor *sd;
    BOOL selected;
    HRESULT hr;

    TRACE("%p, %#x, %s, %p.\n", iface, index, debugstr_guid(guid), value);

    switch (index)
    {
        case MF_SOURCE_READER_MEDIASOURCE:
            if (IsEqualGUID(guid, &MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS))
            {
                DWORD flags;

                if (FAILED(hr = IMFMediaSource_GetCharacteristics(reader->source, &flags)))
                    return hr;

                value->vt = VT_UI4;
                value->u.ulVal = flags;
                return S_OK;
            }
            else
            {
                return IMFPresentationDescriptor_GetItem(reader->descriptor, guid, value);
            }
            break;
        case MF_SOURCE_READER_FIRST_VIDEO_STREAM:
            index = reader->first_video_stream_index;
            break;
        case MF_SOURCE_READER_FIRST_AUDIO_STREAM:
            index = reader->first_audio_stream_index;
            break;
        default:
            ;
    }

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(reader->descriptor, index, &selected, &sd)))
        return hr;

    hr = IMFStreamDescriptor_GetItem(sd, guid, value);
    IMFStreamDescriptor_Release(sd);

    return hr;
}

struct IMFSourceReaderVtbl srcreader_vtbl =
{
    src_reader_QueryInterface,
    src_reader_AddRef,
    src_reader_Release,
    src_reader_GetStreamSelection,
    src_reader_SetStreamSelection,
    src_reader_GetNativeMediaType,
    src_reader_GetCurrentMediaType,
    src_reader_SetCurrentMediaType,
    src_reader_SetCurrentPosition,
    src_reader_ReadSample,
    src_reader_Flush,
    src_reader_GetServiceForStream,
    src_reader_GetPresentationAttribute
};

static DWORD reader_get_first_stream_index(IMFPresentationDescriptor *descriptor, const GUID *major)
{
    unsigned int count, i;
    BOOL selected;
    HRESULT hr;
    GUID guid;

    if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorCount(descriptor, &count)))
        return MF_SOURCE_READER_INVALID_STREAM_INDEX;

    for (i = 0; i < count; ++i)
    {
        IMFMediaTypeHandler *handler;
        IMFStreamDescriptor *sd;

        if (SUCCEEDED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(descriptor, i, &selected, &sd)))
        {
            hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
            IMFStreamDescriptor_Release(sd);
            if (SUCCEEDED(hr))
            {
                hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
                IMFMediaTypeHandler_Release(handler);
                if (FAILED(hr))
                {
                    WARN("Failed to get stream major type, hr %#x.\n", hr);
                    continue;
                }

                if (IsEqualGUID(&guid, major))
                {
                    return i;
                }
            }
        }
    }

    return MF_SOURCE_READER_INVALID_STREAM_INDEX;
}

static HRESULT create_source_reader_from_source(IMFMediaSource *source, IMFAttributes *attributes,
        BOOL shutdown_on_release, REFIID riid, void **out)
{
    struct source_reader *object;
    unsigned int i;
    HRESULT hr;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSourceReader_iface.lpVtbl = &srcreader_vtbl;
    object->source_events_callback.lpVtbl = &source_events_callback_vtbl;
    object->stream_events_callback.lpVtbl = &stream_events_callback_vtbl;
    object->async_commands_callback.lpVtbl = &async_commands_callback_vtbl;
    object->refcount = 1;
    list_init(&object->responses);
    if (shutdown_on_release)
        object->flags |= SOURCE_READER_SHUTDOWN_ON_RELEASE;
    object->source = source;
    IMFMediaSource_AddRef(object->source);
    InitializeCriticalSection(&object->cs);
    InitializeConditionVariable(&object->sample_event);
    InitializeConditionVariable(&object->state_event);

    if (FAILED(hr = IMFMediaSource_CreatePresentationDescriptor(object->source, &object->descriptor)))
        goto failed;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(object->descriptor, &object->stream_count)))
        goto failed;

    if (!(object->streams = heap_alloc_zero(object->stream_count * sizeof(*object->streams))))
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }

    /* Set initial current media types. */
    for (i = 0; i < object->stream_count; ++i)
    {
        IMFMediaTypeHandler *handler;
        IMFStreamDescriptor *sd;
        IMFMediaType *src_type;
        BOOL selected;

        if (FAILED(hr = MFCreateMediaType(&object->streams[i].current)))
            break;

        if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(object->descriptor, i, &selected, &sd)))
            break;

        if (FAILED(hr = IMFStreamDescriptor_GetStreamIdentifier(sd, &object->streams[i].id)))
            WARN("Failed to get stream identifier, hr %#x.\n", hr);

        hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
        IMFStreamDescriptor_Release(sd);
        if (FAILED(hr))
            break;

        hr = IMFMediaTypeHandler_GetMediaTypeByIndex(handler, 0, &src_type);
        IMFMediaTypeHandler_Release(handler);
        if (FAILED(hr))
            break;

        hr = IMFMediaType_CopyAllItems(src_type, (IMFAttributes *)object->streams[i].current);
        IMFMediaType_Release(src_type);
        if (FAILED(hr))
            break;

        object->streams[i].index = i;
    }

    if (FAILED(hr))
        goto failed;

    /* At least one major type has to be set. */
    object->first_audio_stream_index = reader_get_first_stream_index(object->descriptor, &MFMediaType_Audio);
    object->first_video_stream_index = reader_get_first_stream_index(object->descriptor, &MFMediaType_Video);

    if (object->first_audio_stream_index == MF_SOURCE_READER_INVALID_STREAM_INDEX &&
            object->first_video_stream_index == MF_SOURCE_READER_INVALID_STREAM_INDEX)
    {
        hr = MF_E_ATTRIBUTENOTFOUND;
    }

    if (FAILED(hr = IMFMediaSource_BeginGetEvent(object->source, &object->source_events_callback,
            (IUnknown *)object->source)))
    {
        goto failed;
    }

    if (attributes)
    {
        IMFAttributes_GetUnknown(attributes, &MF_SOURCE_READER_ASYNC_CALLBACK, &IID_IMFSourceReaderCallback,
                (void **)&object->async_callback);
        if (object->async_callback)
            TRACE("Using async callback %p.\n", object->async_callback);
    }

    hr = IMFSourceReader_QueryInterface(&object->IMFSourceReader_iface, riid, out);

failed:
    IMFSourceReader_Release(&object->IMFSourceReader_iface);
    return hr;
}

static HRESULT bytestream_get_url_hint(IMFByteStream *stream, WCHAR const **url)
{
    static const unsigned char asfmagic[]  = {0x30,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11,0xa6,0xd9,0x00,0xaa,0x00,0x62,0xce,0x6c};
    static const unsigned char wavmagic[]  = { 'R', 'I', 'F', 'F',0x00,0x00,0x00,0x00, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' '};
    static const unsigned char wavmask[]   = {0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    static const unsigned char isommagic[] = {0x00,0x00,0x00,0x00, 'f', 't', 'y', 'p', 'i', 's', 'o', 'm',0x00,0x00,0x00,0x00};
    static const unsigned char mp42magic[] = {0x00,0x00,0x00,0x00, 'f', 't', 'y', 'p', 'm', 'p', '4', '2',0x00,0x00,0x00,0x00};
    static const unsigned char mp4mask[]   = {0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00};
    static const struct stream_content_url_hint
    {
        const unsigned char *magic;
        const WCHAR *url;
        const unsigned char *mask;
    }
    url_hints[] =
    {
        { asfmagic,  L".asf" },
        { wavmagic,  L".wav", wavmask },
        { isommagic, L".mp4", mp4mask },
        { mp42magic, L".mp4", mp4mask },
    };
    unsigned char buffer[4 * sizeof(unsigned int)], pattern[4 * sizeof(unsigned int)];
    unsigned int i, j, length = 0, caps = 0;
    IMFAttributes *attributes;
    QWORD position;
    HRESULT hr;

    *url = NULL;

    if (SUCCEEDED(IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes)))
    {
        IMFAttributes_GetStringLength(attributes, &MF_BYTESTREAM_CONTENT_TYPE, &length);
        IMFAttributes_Release(attributes);
    }

    if (length)
        return S_OK;

    if (FAILED(hr = IMFByteStream_GetCapabilities(stream, &caps)))
        return hr;

    if (!(caps & MFBYTESTREAM_IS_SEEKABLE))
        return S_OK;

    if (FAILED(hr = IMFByteStream_GetCurrentPosition(stream, &position)))
        return hr;

    hr = IMFByteStream_Read(stream, buffer, sizeof(buffer), &length);
    IMFByteStream_SetCurrentPosition(stream, position);
    if (FAILED(hr))
        return hr;

    if (length < sizeof(buffer))
        return S_OK;

    for (i = 0; i < ARRAY_SIZE(url_hints); ++i)
    {
        memcpy(pattern, buffer, sizeof(buffer));
        if (url_hints[i].mask)
        {
            unsigned int *mask = (unsigned int *)url_hints[i].mask;
            unsigned int *data = (unsigned int *)pattern;

            for (j = 0; j < sizeof(buffer) / sizeof(unsigned int); ++j)
                data[j] &= mask[j];

        }
        if (!memcmp(pattern, url_hints[i].magic, sizeof(pattern)))
        {
            *url = url_hints[i].url;
            break;
        }
    }

    if (*url)
        TRACE("Stream type guessed as %s from %s.\n", debugstr_w(*url), debugstr_an((char *)buffer, length));
    else
        WARN("Unrecognized content type %s.\n", debugstr_an((char *)buffer, length));

    return S_OK;
}

static HRESULT create_source_reader_from_stream(IMFByteStream *stream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    IPropertyStore *props = NULL;
    IMFSourceResolver *resolver;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    const WCHAR *url;
    HRESULT hr;

    /* If stream does not have content type set, try to guess from starting byte sequence. */
    if (FAILED(hr = bytestream_get_url_hint(stream, &url)))
        return hr;

    if (FAILED(hr = MFCreateSourceResolver(&resolver)))
        return hr;

    if (attributes)
        IMFAttributes_GetUnknown(attributes, &MF_SOURCE_READER_MEDIASOURCE_CONFIG, &IID_IPropertyStore,
                (void **)&props);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, url, MF_RESOLUTION_MEDIASOURCE, props,
            &obj_type, (IUnknown **)&source);
    IMFSourceResolver_Release(resolver);
    if (props)
        IPropertyStore_Release(props);
    if (FAILED(hr))
        return hr;

    hr = create_source_reader_from_source(source, attributes, TRUE, riid, out);
    IMFMediaSource_Release(source);
    return hr;
}

static HRESULT create_source_reader_from_url(const WCHAR *url, IMFAttributes *attributes, REFIID riid, void **out)
{
    IPropertyStore *props = NULL;
    IMFSourceResolver *resolver;
    IUnknown *object = NULL;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    HRESULT hr;

    if (FAILED(hr = MFCreateSourceResolver(&resolver)))
        return hr;

    if (attributes)
        IMFAttributes_GetUnknown(attributes, &MF_SOURCE_READER_MEDIASOURCE_CONFIG, &IID_IPropertyStore,
                (void **)&props);

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, url, MF_RESOLUTION_MEDIASOURCE, props, &obj_type,
            &object);
    if (SUCCEEDED(hr))
    {
        switch (obj_type)
        {
            case MF_OBJECT_BYTESTREAM:
                hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, (IMFByteStream *)object, NULL,
                        MF_RESOLUTION_MEDIASOURCE, props, &obj_type, (IUnknown **)&source);
                break;
            case MF_OBJECT_MEDIASOURCE:
                source = (IMFMediaSource *)object;
                IMFMediaSource_AddRef(source);
                break;
            default:
                WARN("Unknown object type %d.\n", obj_type);
                hr = E_UNEXPECTED;
        }
        IUnknown_Release(object);
    }

    IMFSourceResolver_Release(resolver);
    if (props)
        IPropertyStore_Release(props);
    if (FAILED(hr))
        return hr;

    hr = create_source_reader_from_source(source, attributes, TRUE, riid, out);
    IMFMediaSource_Release(source);
    return hr;
}

static HRESULT create_source_reader_from_object(IUnknown *unk, IMFAttributes *attributes, REFIID riid, void **out)
{
    IMFMediaSource *source = NULL;
    IMFByteStream *stream = NULL;
    HRESULT hr;

    hr = IUnknown_QueryInterface(unk, &IID_IMFMediaSource, (void **)&source);
    if (FAILED(hr))
        hr = IUnknown_QueryInterface(unk, &IID_IMFByteStream, (void **)&stream);

    if (source)
    {
        UINT32 disconnect = 0;

        if (attributes)
            IMFAttributes_GetUINT32(attributes, &MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, &disconnect);
        hr = create_source_reader_from_source(source, attributes, !disconnect, riid, out);
    }
    else if (stream)
        hr = create_source_reader_from_stream(stream, attributes, riid, out);

    if (source)
        IMFMediaSource_Release(source);
    if (stream)
        IMFByteStream_Release(stream);

    return hr;
}

/***********************************************************************
 *      MFCreateSourceReaderFromByteStream (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromByteStream(IMFByteStream *stream, IMFAttributes *attributes,
        IMFSourceReader **reader)
{
    TRACE("%p, %p, %p.\n", stream, attributes, reader);

    return create_source_reader_from_object((IUnknown *)stream, attributes, &IID_IMFSourceReader, (void **)reader);
}

/***********************************************************************
 *      MFCreateSourceReaderFromMediaSource (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromMediaSource(IMFMediaSource *source, IMFAttributes *attributes,
        IMFSourceReader **reader)
{
    TRACE("%p, %p, %p.\n", source, attributes, reader);

    return create_source_reader_from_object((IUnknown *)source, attributes, &IID_IMFSourceReader, (void **)reader);
}

/***********************************************************************
 *      MFCreateSourceReaderFromURL (mfreadwrite.@)
 */
HRESULT WINAPI MFCreateSourceReaderFromURL(const WCHAR *url, IMFAttributes *attributes, IMFSourceReader **reader)
{
    TRACE("%s, %p, %p.\n", debugstr_w(url), attributes, reader);

    return create_source_reader_from_url(url, attributes, &IID_IMFSourceReader, (void **)reader);
}

static HRESULT WINAPI readwrite_factory_QueryInterface(IMFReadWriteClassFactory *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IMFReadWriteClassFactory) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFReadWriteClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI readwrite_factory_AddRef(IMFReadWriteClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI readwrite_factory_Release(IMFReadWriteClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI readwrite_factory_CreateInstanceFromURL(IMFReadWriteClassFactory *iface, REFCLSID clsid,
        const WCHAR *url, IMFAttributes *attributes, REFIID riid, void **out)
{
    TRACE("%s, %s, %p, %s, %p.\n", debugstr_guid(clsid), debugstr_w(url), attributes, debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFSourceReader))
    {
        return create_source_reader_from_url(url, attributes, &IID_IMFSourceReader, out);
    }

    FIXME("Unsupported %s.\n", debugstr_guid(clsid));

    return E_NOTIMPL;
}

static HRESULT WINAPI readwrite_factory_CreateInstanceFromObject(IMFReadWriteClassFactory *iface, REFCLSID clsid,
        IUnknown *unk, IMFAttributes *attributes, REFIID riid, void **out)
{
    HRESULT hr;

    TRACE("%s, %p, %p, %s, %p.\n", debugstr_guid(clsid), unk, attributes, debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFSourceReader))
    {
        return create_source_reader_from_object(unk, attributes, riid, out);
    }
    else if (IsEqualGUID(clsid, &CLSID_MFSinkWriter))
    {
        IMFByteStream *stream = NULL;
        IMFMediaSink *sink = NULL;

        hr = IUnknown_QueryInterface(unk, &IID_IMFByteStream, (void **)&stream);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(unk, &IID_IMFMediaSink, (void **)&sink);

        if (stream)
            hr = create_sink_writer_from_stream(stream, attributes, riid, out);
        else if (sink)
            hr = create_sink_writer_from_sink(sink, attributes, riid, out);

        if (sink)
            IMFMediaSink_Release(sink);
        if (stream)
            IMFByteStream_Release(stream);

        return hr;
    }
    else
    {
        WARN("Unsupported class %s.\n", debugstr_guid(clsid));
        *out = NULL;
        return E_FAIL;
    }
}

static const IMFReadWriteClassFactoryVtbl readwrite_factory_vtbl =
{
    readwrite_factory_QueryInterface,
    readwrite_factory_AddRef,
    readwrite_factory_Release,
    readwrite_factory_CreateInstanceFromURL,
    readwrite_factory_CreateInstanceFromObject,
};

static IMFReadWriteClassFactory readwrite_factory = { &readwrite_factory_vtbl };

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    TRACE("%s, %p.\n", debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("interface %s not implemented\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), out);

    *out = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IMFReadWriteClassFactory_QueryInterface(&readwrite_factory, riid, out);
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("%d.\n", dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl classfactoryvtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer,
};

static IClassFactory classfactory = { &classfactoryvtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    TRACE("%s, %s, %p.\n", debugstr_guid(clsid), debugstr_guid(riid), out);

    if (IsEqualGUID(clsid, &CLSID_MFReadWriteClassFactory))
        return IClassFactory_QueryInterface(&classfactory, riid, out);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    *out = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
