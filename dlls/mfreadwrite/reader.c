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
#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "rpcproxy.h"

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mfidl.h"
#include "mfreadwrite.h"
#include "d3d9.h"
#include "initguid.h"
#include "dxva2api.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "mf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32, D3DFMT_A8B8G8R8);

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
    STREAM_FLAG_STOPPED = 0x8,          /* Received MEStreamStopped */
};

struct transform_entry
{
    struct list entry;
    IMFTransform *transform;
    unsigned int min_buffer_size;
    UINT32 pending_flags;
    GUID category;
    BOOL hidden;
    BOOL attributes_initialized;
};

struct media_stream
{
    IMFMediaStream *stream;
    IMFMediaType *current;
    struct list transforms;
    IMFVideoSampleAllocatorEx *allocator;
    IMFTransform *transform_service;
    DWORD id;
    unsigned int index;
    enum media_stream_state state;
    unsigned int flags;
    unsigned int requests;
    unsigned int responses;
    LONGLONG last_sample_ts;
    struct source_reader *reader;
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
        struct
        {
            unsigned int stream_index;
        } sa;
    } u;
};

enum source_reader_flags
{
    SOURCE_READER_FLUSHING = 0x1,
    SOURCE_READER_SEEKING = 0x2,
    SOURCE_READER_SHUTDOWN_ON_RELEASE = 0x4,
    SOURCE_READER_D3D9_DEVICE_MANAGER = 0x8,
    SOURCE_READER_DXGI_DEVICE_MANAGER = 0x10,
    SOURCE_READER_HAS_DEVICE_MANAGER = SOURCE_READER_D3D9_DEVICE_MANAGER | SOURCE_READER_DXGI_DEVICE_MANAGER,
};

struct source_reader
{
    IMFSourceReaderEx IMFSourceReaderEx_iface;
    IMFAsyncCallback source_events_callback;
    IMFAsyncCallback stream_events_callback;
    IMFAsyncCallback async_commands_callback;
    LONG refcount;
    LONG public_refcount;
    IMFMediaSource *source;
    IMFPresentationDescriptor *descriptor;
    IMFSourceReaderCallback *async_callback;
    IMFAttributes *attributes;
    IUnknown *device_manager;
    unsigned int first_audio_stream_index;
    unsigned int first_video_stream_index;
    DWORD stream_count;
    unsigned int flags;
    DWORD queue;
    enum media_source_state source_state;
    struct media_stream *streams;
    struct list responses;
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE sample_event;
    CONDITION_VARIABLE state_event;
    CONDITION_VARIABLE stop_event;
};

static inline struct source_reader *impl_from_IMFSourceReaderEx(IMFSourceReaderEx *iface)
{
    return CONTAINING_RECORD(iface, struct source_reader, IMFSourceReaderEx_iface);
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

static void source_reader_release_responses(struct source_reader *reader, struct media_stream *stream);

static ULONG source_reader_addref(struct source_reader *reader)
{
    return InterlockedIncrement(&reader->refcount);
}

static void transform_entry_destroy(struct transform_entry *entry)
{
    IMFTransform_Release(entry->transform);
    free(entry);
}

static void media_stream_destroy(struct media_stream *stream)
{
    struct transform_entry *entry, *next;

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &stream->transforms, struct transform_entry, entry)
    {
        list_remove(&entry->entry);
        transform_entry_destroy(entry);
    }

    if (stream->transform_service)
        IMFTransform_Release(stream->transform_service);
    if (stream->stream)
        IMFMediaStream_Release(stream->stream);
    if (stream->current)
        IMFMediaType_Release(stream->current);
    if (stream->allocator)
        IMFVideoSampleAllocatorEx_Release(stream->allocator);
}

static ULONG source_reader_release(struct source_reader *reader)
{
    ULONG refcount = InterlockedDecrement(&reader->refcount);
    unsigned int i;

    if (!refcount)
    {
        if (reader->device_manager)
            IUnknown_Release(reader->device_manager);
        if (reader->async_callback)
            IMFSourceReaderCallback_Release(reader->async_callback);
        if (reader->descriptor)
            IMFPresentationDescriptor_Release(reader->descriptor);
        if (reader->attributes)
            IMFAttributes_Release(reader->attributes);
        IMFMediaSource_Release(reader->source);

        for (i = 0; i < reader->stream_count; ++i)
        {
            struct media_stream *stream = &reader->streams[i];
            media_stream_destroy(stream);
        }
        source_reader_release_responses(reader, NULL);
        free(reader->streams);
        DeleteCriticalSection(&reader->cs);
        free(reader);
    }

    return refcount;
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
        free(command);
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

    if (!(command = calloc(1, sizeof(*command))))
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
        WARN("Failed to get event value, hr %#lx.\n", hr);
        return hr;
    }

    if (value.vt != VT_UNKNOWN || !value.punkVal)
    {
        WARN("Unexpected value type %d.\n", value.vt);
        PropVariantClear(&value);
        return E_UNEXPECTED;
    }

    hr = IUnknown_QueryInterface(value.punkVal, riid, obj);
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
    return source_reader_addref(reader);
}

static ULONG WINAPI source_reader_source_events_callback_Release(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_source_callback_IMFAsyncCallback(iface);
    return source_reader_release(reader);
}

static HRESULT WINAPI source_reader_callback_GetParameters(IMFAsyncCallback *iface,
        DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static void source_reader_response_ready(struct source_reader *reader, struct stream_response *response)
{
    struct source_reader_async_command *command;
    struct media_stream *stream = &reader->streams[response->stream_index];
    HRESULT hr;

    if (!stream->requests)
        return;

    if (reader->async_callback)
    {
        if (SUCCEEDED(source_reader_create_async_op(SOURCE_READER_ASYNC_SAMPLE_READY, &command)))
        {
            command->u.sample.stream_index = stream->index;
            if (FAILED(hr = MFPutWorkItem(reader->queue, &reader->async_commands_callback, &command->IUnknown_iface)))
                WARN("Failed to submit async result, hr %#lx.\n", hr);
            IUnknown_Release(&command->IUnknown_iface);
        }
    }
    else
        WakeAllConditionVariable(&reader->sample_event);

    stream->requests--;
}

static void source_reader_copy_sample_buffer(IMFSample *src, IMFSample *dst)
{
    IMFMediaBuffer *buffer;
    LONGLONG time;
    DWORD flags;
    HRESULT hr;

    IMFSample_CopyAllItems(src, (IMFAttributes *)dst);

    IMFSample_SetSampleDuration(dst, 0);
    IMFSample_SetSampleTime(dst, 0);
    IMFSample_SetSampleFlags(dst, 0);

    if (SUCCEEDED(IMFSample_GetSampleDuration(src, &time)))
        IMFSample_SetSampleDuration(dst, time);

    if (SUCCEEDED(IMFSample_GetSampleTime(src, &time)))
        IMFSample_SetSampleTime(dst, time);

    if (SUCCEEDED(IMFSample_GetSampleFlags(src, &flags)))
        IMFSample_SetSampleFlags(dst, flags);

    if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(src, NULL)))
    {
        if (SUCCEEDED(IMFSample_GetBufferByIndex(dst, 0, &buffer)))
        {
            if (FAILED(hr = IMFSample_CopyToBuffer(src, buffer)))
                WARN("Failed to copy a buffer, hr %#lx.\n", hr);
            IMFMediaBuffer_Release(buffer);
        }
    }
}

static HRESULT source_reader_queue_response(struct source_reader *reader, struct media_stream *stream, HRESULT status,
        DWORD stream_flags, LONGLONG timestamp, IMFSample *sample)
{
    struct stream_response *response;

    if (!(response = calloc(1, sizeof(*response))))
        return E_OUTOFMEMORY;

    response->status = status;
    response->stream_index = stream->index;
    response->stream_flags = stream_flags;
    response->timestamp = timestamp;
    response->sample = sample;
    if (response->sample)
        IMFSample_AddRef(response->sample);

    list_add_tail(&reader->responses, &response->entry);
    stream->responses++;

    source_reader_response_ready(reader, response);

    stream->last_sample_ts = timestamp;

    return S_OK;
}

static HRESULT source_reader_queue_sample(struct source_reader *reader, struct media_stream *stream,
        UINT flags, IMFSample *sample)
{
    LONGLONG timestamp = 0;

    if (FAILED(IMFSample_GetSampleTime(sample, &timestamp)))
        WARN("Sample time wasn't set.\n");

    return source_reader_queue_response(reader, stream, S_OK, flags, timestamp, sample);
}

static HRESULT source_reader_request_sample(struct source_reader *reader, struct media_stream *stream)
{
    HRESULT hr = S_OK;

    if (stream->stream && !(stream->flags & STREAM_FLAG_SAMPLE_REQUESTED))
    {
        if (FAILED(hr = IMFMediaStream_RequestSample(stream->stream, NULL)))
            WARN("Sample request failed, hr %#lx.\n", hr);
        else
        {
            stream->flags |= STREAM_FLAG_SAMPLE_REQUESTED;
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
        WARN("Failed to get stream object, hr %#lx.\n", hr);
        return hr;
    }

    TRACE("Got new stream %p.\n", stream);

    if (FAILED(hr = media_stream_get_id(stream, &id)))
    {
        WARN("Unidentified stream %p, hr %#lx.\n", stream, hr);
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
                    WARN("Failed to subscribe to stream events, hr %#lx.\n", hr);
                }

                if (reader->streams[i].requests)
                    if (FAILED(source_reader_request_sample(reader, &reader->streams[i])))
                        WakeAllConditionVariable(&reader->sample_event);
            }
            break;
        }
    }

    if (i == reader->stream_count)
        WARN("Stream with id %#lx was not present in presentation descriptor.\n", id);

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
            reader->flags &= ~SOURCE_READER_SEEKING;
            break;
        case MESourceStopped:
            reader->source_state = SOURCE_STATE_STOPPED;
            reader->flags &= ~SOURCE_READER_SEEKING;
            break;
        case MESourceSeeked:
            reader->flags &= ~SOURCE_READER_SEEKING;
            break;
        default:
            WARN("Unhandled event %ld.\n", event_type);
    }

    LeaveCriticalSection(&reader->cs);

    WakeAllConditionVariable(&reader->state_event);
    if (event_type == MESourceStopped)
        WakeAllConditionVariable(&reader->stop_event);

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

    TRACE("Got event %lu.\n", event_type);

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
        WARN("Failed while handling %ld event, hr %#lx.\n", event_type, hr);

    IMFMediaEvent_Release(event);

    if (event_type != MESourceStopped)
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
    return source_reader_addref(reader);
}

static ULONG WINAPI source_reader_stream_events_callback_Release(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_stream_callback_IMFAsyncCallback(iface);
    return source_reader_release(reader);
}

static HRESULT source_reader_allocate_stream_sample(MFT_OUTPUT_STREAM_INFO *info, IMFSample **out)
{
    IMFMediaBuffer *buffer;
    IMFSample *sample;
    HRESULT hr;

    *out = NULL;
    if (FAILED(hr = MFCreateSample(&sample)))
        return hr;
    if (SUCCEEDED(hr = MFCreateAlignedMemoryBuffer(info->cbSize, info->cbAlignment, &buffer)))
    {
        if (SUCCEEDED(hr = IMFSample_AddBuffer(sample, buffer)))
        {
            *out = sample;
            IMFSample_AddRef(sample);
        }
        IMFMediaBuffer_Release(buffer);
    }

    IMFSample_Release(sample);
    return hr;
}

static void media_type_try_copy_attr(IMFMediaType *dst, IMFMediaType *src, const GUID *attr, HRESULT *hr)
{
    PROPVARIANT value;

    PropVariantInit(&value);
    if (SUCCEEDED(*hr) && FAILED(IMFMediaType_GetItem(dst, attr, NULL))
            && SUCCEEDED(IMFMediaType_GetItem(src, attr, &value)))
        *hr = IMFMediaType_SetItem(dst, attr, &value);
    PropVariantClear(&value);
}

/* update a media type with additional attributes reported by upstream element */
/* also present in mf/topology_loader.c pipeline */
static HRESULT update_media_type_from_upstream(IMFMediaType *media_type, IMFMediaType *upstream_type)
{
    HRESULT hr = S_OK;

    /* propagate common video attributes */
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_FRAME_SIZE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_FRAME_RATE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_DEFAULT_STRIDE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_ROTATION, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_FIXED_SIZE_SAMPLES, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_PIXEL_ASPECT_RATIO, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, &hr);

    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_CHROMA_SITING, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_INTERLACE_MODE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_TRANSFER_FUNCTION, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_PRIMARIES, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_YUV_MATRIX, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_LIGHTING, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_VIDEO_NOMINAL_RANGE, &hr);

    /* propagate common audio attributes */
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_NUM_CHANNELS, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_CHANNEL_MASK, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_SAMPLES_PER_BLOCK, &hr);
    media_type_try_copy_attr(media_type, upstream_type, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, &hr);

    return hr;
}

static HRESULT source_reader_pull_transform_samples(struct source_reader *reader, struct media_stream *stream,
        struct transform_entry *entry);
static HRESULT source_reader_push_transform_samples(struct source_reader *reader, struct media_stream *stream,
        struct transform_entry *entry, IMFSample *sample)
{
    HRESULT hr;

    do
    {
        if (FAILED(hr = source_reader_pull_transform_samples(reader, stream, entry))
                && hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
            return hr;
        if (SUCCEEDED(hr = IMFTransform_ProcessInput(entry->transform, 0, sample, 0)))
            return source_reader_pull_transform_samples(reader, stream, entry);
    }
    while (hr == MF_E_NOTACCEPTING);

    return hr;
}

/* update the transform output type while keeping subtype which matches the desired type */
static HRESULT set_matching_transform_output_type(IMFTransform *transform, IMFMediaType *old_output_type)
{
    IMFMediaType *new_output_type;
    GUID subtype, desired;
    UINT i = 0;
    HRESULT hr;

    IMFMediaType_GetGUID(old_output_type, &MF_MT_SUBTYPE, &desired);

    /* find an available output type matching the desired subtype */
    while (SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, i++, &new_output_type)))
    {
        IMFMediaType_GetGUID(new_output_type, &MF_MT_SUBTYPE, &subtype);
        if (IsEqualGUID(&subtype, &desired) && SUCCEEDED(hr = IMFTransform_SetOutputType(transform, 0, new_output_type, 0)))
        {
            IMFMediaType_Release(new_output_type);
            return S_OK;
        }
        IMFMediaType_Release(new_output_type);
    }

    return hr;
}

/* update the transform output type while keeping subtype which matches the old output type */
static HRESULT transform_entry_update_output_type(struct transform_entry *entry, IMFMediaType *old_output_type)
{
    HRESULT hr;

    if (SUCCEEDED(hr = set_matching_transform_output_type(entry->transform, old_output_type)))
        entry->pending_flags |= MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED;

    return hr;
}

/* update the transform input type while keeping an output type which matches the current output subtype */
static HRESULT transform_entry_update_input_type(struct transform_entry *entry, IMFMediaType *input_type)
{
    IMFMediaType *old_output_type, *new_output_type;
    HRESULT hr;

    if (FAILED(hr = IMFTransform_GetOutputCurrentType(entry->transform, 0, &old_output_type)))
        return hr;
    if (FAILED(hr = IMFTransform_SetInputType(entry->transform, 0, input_type, 0)))
        return hr;

    /* check if transform output type is still valid or if we need to update it as well */
    if (FAILED(hr = IMFTransform_GetOutputCurrentType(entry->transform, 0, &new_output_type)))
        hr = transform_entry_update_output_type(entry, old_output_type);
    else
        IMFMediaType_Release(new_output_type);

    IMFMediaType_Release(old_output_type);
    return hr;
}

static void transform_entry_initialize_attributes(struct source_reader *reader, struct transform_entry *entry)
{
    IMFAttributes *attributes;

    if (SUCCEEDED(IMFTransform_GetAttributes(entry->transform, &attributes)))
    {
        if (FAILED(IMFAttributes_GetItem(attributes, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, NULL)))
            IMFAttributes_SetUINT32(attributes, &MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT, 6);

        IMFAttributes_Release(attributes);
    }

    if (SUCCEEDED(IMFTransform_GetOutputStreamAttributes(entry->transform, 0, &attributes)))
    {
        UINT32 shared, shared_without_mutex, bind_flags;

        if (SUCCEEDED(IMFAttributes_GetUINT32(reader->attributes, &MF_SA_D3D11_SHARED, &shared)))
            IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_SHARED, shared);
        if (SUCCEEDED(IMFAttributes_GetUINT32(reader->attributes, &MF_SA_D3D11_SHARED_WITHOUT_MUTEX, &shared_without_mutex)))
            IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_SHARED_WITHOUT_MUTEX, shared_without_mutex);
        if (SUCCEEDED(IMFAttributes_GetUINT32(reader->attributes, &MF_SOURCE_READER_D3D11_BIND_FLAGS, &bind_flags)))
            IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_BINDFLAGS, bind_flags);
        else if ((reader->flags & SOURCE_READER_DXGI_DEVICE_MANAGER) && FAILED(IMFAttributes_GetItem(attributes, &MF_SA_D3D11_BINDFLAGS, NULL)))
            IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_BINDFLAGS, 1024);

        IMFAttributes_Release(attributes);
    }
}

static HRESULT source_reader_pull_transform_samples(struct source_reader *reader, struct media_stream *stream,
        struct transform_entry *entry)
{
    MFT_OUTPUT_STREAM_INFO stream_info = {0};
    struct transform_entry *next = NULL;
    struct list *ptr;
    DWORD status;
    HRESULT hr;

    if ((ptr = list_next(&stream->transforms, &entry->entry)))
        next = LIST_ENTRY(ptr, struct transform_entry, entry);

    if (!entry->attributes_initialized)
    {
        transform_entry_initialize_attributes(reader, entry);
        entry->attributes_initialized = TRUE;
    }

    if (FAILED(hr = IMFTransform_GetOutputStreamInfo(entry->transform, 0, &stream_info)))
        return hr;
    stream_info.cbSize = max(stream_info.cbSize, entry->min_buffer_size);

    while (SUCCEEDED(hr))
    {
        MFT_OUTPUT_DATA_BUFFER out_buffer = {0};
        IMFMediaType *media_type;

        if (!(stream_info.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
                && FAILED(hr = source_reader_allocate_stream_sample(&stream_info, &out_buffer.pSample)))
            break;

        if (SUCCEEDED(hr = IMFTransform_ProcessOutput(entry->transform, 0, 1, &out_buffer, &status)))
        {
            /* propagate upstream type to the transform input type */
            if ((entry->pending_flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
                    && SUCCEEDED(hr = IMFTransform_GetOutputCurrentType(entry->transform, 0, &media_type)))
            {
                if (!next)
                    hr = IMFMediaType_CopyAllItems(media_type, (IMFAttributes *)stream->current);
                else
                    hr = transform_entry_update_input_type(next, media_type);
                IMFMediaType_Release(media_type);
            }

            if (FAILED(hr))
                source_reader_queue_response(reader, stream, hr, MF_SOURCE_READERF_ERROR, 0, NULL);
            else if (next)
                hr = source_reader_push_transform_samples(reader, stream, next, out_buffer.pSample);
            else
                hr = source_reader_queue_sample(reader, stream, entry->pending_flags, out_buffer.pSample);

            entry->pending_flags = 0;
        }

        if (hr == MF_E_TRANSFORM_STREAM_CHANGE && SUCCEEDED(hr = IMFTransform_GetOutputCurrentType(entry->transform, 0, &media_type)))
        {
            hr = transform_entry_update_output_type(entry, media_type);
            IMFMediaType_Release(media_type);

            if (SUCCEEDED(hr))
            {
                hr = IMFTransform_GetOutputStreamInfo(entry->transform, 0, &stream_info);
                stream_info.cbSize = max(stream_info.cbSize, entry->min_buffer_size);
            }
        }

        if (out_buffer.pSample)
            IMFSample_Release(out_buffer.pSample);
        if (out_buffer.pEvents)
            IMFCollection_Release(out_buffer.pEvents);
    }

    return hr;
}

static HRESULT source_reader_drain_transform_samples(struct source_reader *reader, struct media_stream *stream,
        struct transform_entry *entry)
{
    struct transform_entry *next = NULL;
    struct list *ptr;
    HRESULT hr;

    if ((ptr = list_next(&stream->transforms, &entry->entry)))
        next = LIST_ENTRY(ptr, struct transform_entry, entry);

    if (FAILED(hr = IMFTransform_ProcessMessage(entry->transform, MFT_MESSAGE_COMMAND_DRAIN, 0)))
        WARN("Failed to drain transform %p, hr %#lx\n", entry->transform, hr);
    if (FAILED(hr = source_reader_pull_transform_samples(reader, stream, entry))
            && hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
        WARN("Failed to pull pending samples, hr %#lx.\n", hr);

    return next ? source_reader_drain_transform_samples(reader, stream, next) : S_OK;
}

static HRESULT source_reader_flush_transform_samples(struct source_reader *reader, struct media_stream *stream,
        struct transform_entry *entry)
{
    struct transform_entry *next = NULL;
    struct list *ptr;
    HRESULT hr;

    if ((ptr = list_next(&stream->transforms, &entry->entry)))
        next = LIST_ENTRY(ptr, struct transform_entry, entry);

    if (FAILED(hr = IMFTransform_ProcessMessage(entry->transform, MFT_MESSAGE_COMMAND_FLUSH, 0)))
        WARN("Failed to flush transform %p, hr %#lx\n", entry->transform, hr);

    return next ? source_reader_flush_transform_samples(reader, stream, next) : S_OK;
}

static HRESULT source_reader_notify_transform(struct source_reader *reader, struct media_stream *stream,
        struct transform_entry *entry, UINT message)
{
    struct transform_entry *next = NULL;
    struct list *ptr;
    HRESULT hr;

    if ((ptr = list_next(&stream->transforms, &entry->entry)))
        next = LIST_ENTRY(ptr, struct transform_entry, entry);

    if (FAILED(hr = IMFTransform_ProcessMessage(entry->transform, message, 0)))
        WARN("Failed to notify transform %p message %#x, hr %#lx\n", entry->transform, message, hr);

    return next ? source_reader_notify_transform(reader, stream, next, message) : S_OK;
}

static HRESULT source_reader_process_sample(struct source_reader *reader, struct media_stream *stream,
        IMFSample *sample)
{
    struct transform_entry *entry;
    struct list *ptr;
    HRESULT hr;

    if (!(ptr = list_head(&stream->transforms)))
        return source_reader_queue_sample(reader, stream, 0, sample);
    entry = LIST_ENTRY(ptr, struct transform_entry, entry);

    /* It's assumed that decoder has 1 input and 1 output, both id's are 0. */
    if (SUCCEEDED(hr = source_reader_push_transform_samples(reader, stream, entry, sample))
            || hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        hr = stream->requests ? source_reader_request_sample(reader, stream) : S_OK;
    else
        WARN("Transform failed to process output, hr %#lx.\n", hr);

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
        WARN("Failed to get sample object, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = media_stream_get_id(stream, &id)))
    {
        WARN("Unidentified stream %p, hr %#lx.\n", stream, hr);
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
            break;
        }
    }

    if (i == reader->stream_count)
        WARN("Stream with id %#lx was not present in presentation descriptor.\n", id);

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
    struct list *ptr;
    unsigned int i;
    HRESULT hr;
    DWORD id;

    IMFMediaEvent_GetType(event, &event_type);

    if (FAILED(hr = media_stream_get_id(stream, &id)))
    {
        WARN("Unidentified stream %p, hr %#lx.\n", stream, hr);
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

                    if ((ptr = list_head(&stream->transforms)))
                    {
                        struct transform_entry *entry = LIST_ENTRY(ptr, struct transform_entry, entry);
                        if (FAILED(hr = source_reader_drain_transform_samples(reader, stream, entry)))
                            WARN("Failed to drain pending samples, hr %#lx.\n", hr);
                    }

                    while (stream->requests)
                        source_reader_queue_response(reader, stream, S_OK, MF_SOURCE_READERF_ENDOFSTREAM, 0, NULL);

                    break;
                case MEStreamSeeked:
                case MEStreamStarted:
                    stream->state = STREAM_STATE_READY;

                    if ((ptr = list_head(&stream->transforms)))
                    {
                        struct transform_entry *entry = LIST_ENTRY(ptr, struct transform_entry, entry);
                        if (FAILED(hr = source_reader_notify_transform(reader, stream, entry, MFT_MESSAGE_NOTIFY_START_OF_STREAM)))
                            WARN("Failed to notify transforms of stream start, hr %#lx.\n", hr);
                    }
                    break;
                case MEStreamStopped:
                    stream->flags |= STREAM_FLAG_STOPPED;
                    break;
                case MEStreamTick:
                    value.vt = VT_EMPTY;
                    hr = SUCCEEDED(IMFMediaEvent_GetValue(event, &value)) && value.vt == VT_I8 ? S_OK : E_UNEXPECTED;
                    timestamp = SUCCEEDED(hr) ? value.hVal.QuadPart : 0;
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

    if (event_type == MEStreamStopped)
        WakeAllConditionVariable(&reader->stop_event);

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

    TRACE("Got event %lu.\n", event_type);

    switch (event_type)
    {
        case MEMediaSample:
            hr = source_reader_media_sample_handler(reader, stream, event);
            break;
        case MEStreamSeeked:
        case MEStreamStarted:
        case MEStreamStopped:
        case MEStreamTick:
        case MEEndOfStream:
            hr = source_reader_media_stream_state_handler(reader, stream, event);
            break;
        default:
            ;
    }

    if (FAILED(hr))
        WARN("Failed while handling %ld event, hr %#lx.\n", event_type, hr);

    IMFMediaEvent_Release(event);

    if (event_type != MEStreamStopped)
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
    return source_reader_addref(reader);
}

static ULONG WINAPI source_reader_async_commands_callback_Release(IMFAsyncCallback *iface)
{
    struct source_reader *reader = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    return source_reader_release(reader);
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
    IMFSample *sample;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY(response, &reader->responses, struct stream_response, entry)
    {
        if (stream && response->stream_index != stream->index)
            continue;

        if (!stream) stream = &reader->streams[response->stream_index];

        if (response->sample && stream->allocator)
        {
            /* Return allocation error to the caller, while keeping original response sample in for later. */
            if (SUCCEEDED(hr = IMFVideoSampleAllocatorEx_AllocateSample(stream->allocator, &sample)))
            {
                source_reader_copy_sample_buffer(response->sample, sample);
                IMFSample_Release(response->sample);
                response->sample = sample;
            }
            else
            {
                if (!(response = calloc(1, sizeof(*response))))
                    return NULL;

                response->status = hr;
                response->stream_flags = MF_SOURCE_READERF_ERROR;
                return response;
            }
        }

        return media_stream_detach_response(reader, response);
    }

    return NULL;
}

static void source_reader_release_response(struct stream_response *response)
{
    if (response->sample)
        IMFSample_Release(response->sample);
    free(response);
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

    position.hVal.QuadPart = 0;
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

static HRESULT source_reader_get_next_selected_stream(struct source_reader *reader, DWORD *stream_index)
{
    unsigned int i, first_selected = ~0u;
    BOOL selected, stream_drained;
    LONGLONG min_ts = MAXLONGLONG;

    for (i = 0; i < reader->stream_count; ++i)
    {
        stream_drained = reader->streams[i].state == STREAM_STATE_EOS && !reader->streams[i].responses;
        selected = SUCCEEDED(source_reader_get_stream_selection(reader, i, &selected)) && selected;

        if (selected)
        {
            if (first_selected == ~0u)
                first_selected = i;

            /* Pick the stream whose last sample had the lowest timestamp. */
            if (!stream_drained && reader->streams[i].last_sample_ts < min_ts)
            {
                min_ts = reader->streams[i].last_sample_ts;
                *stream_index = i;
            }
        }
    }

    /* If all selected streams reached EOS, use first selected. */
    if (first_selected != ~0u)
    {
        if (min_ts == MAXLONGLONG)
            *stream_index = first_selected;
    }

    return first_selected == ~0u ? MF_E_MEDIA_SOURCE_NO_STREAMS_SELECTED : S_OK;
}

static HRESULT source_reader_get_stream_read_index(struct source_reader *reader, unsigned int index, DWORD *stream_index)
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
            return source_reader_get_next_selected_stream(reader, stream_index);
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
    struct list *ptr;
    HRESULT hr;

    source_reader_release_responses(reader, stream);

    if ((ptr = list_head(&stream->transforms)))
    {
        struct transform_entry *entry = LIST_ENTRY(ptr, struct transform_entry, entry);
        if (FAILED(hr = source_reader_flush_transform_samples(reader, stream, entry)))
            WARN("Failed to drain pending samples, hr %#lx.\n", hr);
    }

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
            stream = &reader->streams[command->u.sample.stream_index];
            response = media_stream_pop_response(reader, stream);
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

static HRESULT WINAPI src_reader_QueryInterface(IMFSourceReaderEx *iface, REFIID riid, void **out)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IMFSourceReader)
            || IsEqualGUID(riid, &IID_IMFSourceReaderEx))
    {
        *out = &reader->IMFSourceReaderEx_iface;
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

static ULONG WINAPI src_reader_AddRef(IMFSourceReaderEx *iface)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    ULONG refcount = InterlockedIncrement(&reader->public_refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static BOOL source_reader_is_source_stopped(const struct source_reader *reader)
{
    unsigned int i;

    if (reader->source_state != SOURCE_STATE_STOPPED)
        return FALSE;

    for (i = 0; i < reader->stream_count; ++i)
    {
        if (reader->streams[i].stream && !(reader->streams[i].flags & STREAM_FLAG_STOPPED))
            return FALSE;
    }

    return TRUE;
}

static ULONG WINAPI src_reader_Release(IMFSourceReaderEx *iface)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    ULONG refcount = InterlockedDecrement(&reader->public_refcount);
    unsigned int i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (reader->flags & SOURCE_READER_SHUTDOWN_ON_RELEASE)
            IMFMediaSource_Shutdown(reader->source);
        else if (SUCCEEDED(IMFMediaSource_Stop(reader->source)))
        {
            EnterCriticalSection(&reader->cs);

            while (!source_reader_is_source_stopped(reader))
            {
                SleepConditionVariableCS(&reader->stop_event, &reader->cs, INFINITE);
            }

            LeaveCriticalSection(&reader->cs);
        }

        for (i = 0; i < reader->stream_count; ++i)
        {
            struct media_stream *stream = &reader->streams[i];
            IMFVideoSampleAllocatorCallback *callback;

            if (!stream->allocator)
                continue;

            if (SUCCEEDED(IMFVideoSampleAllocatorEx_QueryInterface(stream->allocator, &IID_IMFVideoSampleAllocatorCallback,
                    (void **)&callback)))
            {
                IMFVideoSampleAllocatorCallback_SetCallback(callback, NULL);
                IMFVideoSampleAllocatorCallback_Release(callback);
            }
        }

        MFUnlockWorkQueue(reader->queue);
        source_reader_release(reader);
    }

    return refcount;
}

static HRESULT WINAPI src_reader_GetStreamSelection(IMFSourceReaderEx *iface, DWORD index, BOOL *selected)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);

    TRACE("%p, %#lx, %p.\n", iface, index, selected);

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

static HRESULT WINAPI src_reader_SetStreamSelection(IMFSourceReaderEx *iface, DWORD index, BOOL selection)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    HRESULT hr = S_OK;
    BOOL selection_changed = FALSE, selected;
    unsigned int i;

    TRACE("%p, %#lx, %d.\n", iface, index, selection);

    selection = !!selection;

    EnterCriticalSection(&reader->cs);

    if (index == MF_SOURCE_READER_ALL_STREAMS)
    {
        for (i = 0; i < reader->stream_count; ++i)
        {
            if (!selection_changed)
            {
                source_reader_get_stream_selection(reader, i, &selected);
                selection_changed = !!(selected ^ selection);
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
        selection_changed = !!(selected ^ selection);

        if (selection)
            hr = IMFPresentationDescriptor_SelectStream(reader->descriptor, index);
        else
            hr = IMFPresentationDescriptor_DeselectStream(reader->descriptor, index);
    }

    if (selection_changed)
    {
        for (i = 0; i < reader->stream_count; ++i)
        {
            reader->streams[i].last_sample_ts = 0;
        }
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

static HRESULT WINAPI src_reader_GetNativeMediaType(IMFSourceReaderEx *iface, DWORD index, DWORD type_index,
            IMFMediaType **type)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);

    TRACE("%p, %#lx, %#lx, %p.\n", iface, index, type_index, type);

    return source_reader_get_native_media_type(reader, index, type_index, type);
}

static HRESULT WINAPI src_reader_GetCurrentMediaType(IMFSourceReaderEx *iface, DWORD index, IMFMediaType **type)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    HRESULT hr;

    TRACE("%p, %#lx, %p.\n", iface, index, type);

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
    struct media_stream *stream = &reader->streams[index];
    struct transform_entry *entry, *next;
    IMFMediaTypeHandler *type_handler;
    IMFMediaType *native_type;
    BOOL type_set = FALSE;
    unsigned int i = 0;
    DWORD flags;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_IsEqual(type, stream->current, &flags)))
        return hr;

    if (!(flags & MF_MEDIATYPE_EQUAL_MAJOR_TYPES))
        return MF_E_INVALIDMEDIATYPE;

    /* No need for a decoder or type change. */
    if (flags & MF_MEDIATYPE_EQUAL_FORMAT_DATA)
        return S_OK;

    if (stream->transform_service)
    {
        IMFTransform_Release(stream->transform_service);
        stream->transform_service = NULL;
    }
    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &stream->transforms, struct transform_entry, entry)
    {
        list_remove(&entry->entry);
        transform_entry_destroy(entry);
    }

    if (FAILED(hr = source_reader_get_source_type_handler(reader, index, &type_handler)))
        return hr;

    while (!type_set && IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, i++, &native_type) == S_OK)
    {
        static const DWORD compare_flags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA;

        if (SUCCEEDED(IMFMediaType_IsEqual(native_type, type, &flags)) && (flags & compare_flags) == compare_flags)
        {
            if ((type_set = SUCCEEDED(IMFMediaTypeHandler_SetCurrentMediaType(type_handler, native_type))))
                IMFMediaType_CopyAllItems(native_type, (IMFAttributes *)stream->current);
        }

        IMFMediaType_Release(native_type);
    }

    IMFMediaTypeHandler_Release(type_handler);

    return type_set ? S_OK : S_FALSE;
}

static HRESULT source_reader_create_sample_allocator_attributes(const struct source_reader *reader,
        IMFAttributes **attributes)
{
    UINT32 shared = 0, shared_without_mutex = 0;
    HRESULT hr;

    if (FAILED(hr = MFCreateAttributes(attributes, 1)))
        return hr;

    IMFAttributes_GetUINT32(reader->attributes, &MF_SA_D3D11_SHARED, &shared);
    IMFAttributes_GetUINT32(reader->attributes, &MF_SA_D3D11_SHARED_WITHOUT_MUTEX, &shared_without_mutex);

    if (shared_without_mutex)
        hr = IMFAttributes_SetUINT32(*attributes, &MF_SA_D3D11_SHARED_WITHOUT_MUTEX, TRUE);
    else if (shared)
        hr = IMFAttributes_SetUINT32(*attributes, &MF_SA_D3D11_SHARED, TRUE);

    return hr;
}

static HRESULT source_reader_setup_sample_allocator(struct source_reader *reader, unsigned int index)
{
    struct media_stream *stream = &reader->streams[index];
    IMFAttributes *attributes = NULL;
    GUID major = { 0 };
    HRESULT hr;

    IMFMediaType_GetMajorType(stream->current, &major);
    if (!IsEqualGUID(&major, &MFMediaType_Video))
        return S_OK;

    if (!(reader->flags & SOURCE_READER_HAS_DEVICE_MANAGER))
        return S_OK;

    if (!stream->allocator)
    {
        if (FAILED(hr = MFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocatorEx, (void **)&stream->allocator)))
        {
            WARN("Failed to create sample allocator, hr %#lx.\n", hr);
            return hr;
        }
    }

    IMFVideoSampleAllocatorEx_UninitializeSampleAllocator(stream->allocator);
    if (FAILED(hr = IMFVideoSampleAllocatorEx_SetDirectXManager(stream->allocator, reader->device_manager)))
    {
        WARN("Failed to set device manager, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = source_reader_create_sample_allocator_attributes(reader, &attributes)))
        WARN("Failed to create allocator attributes, hr %#lx.\n", hr);

    if (FAILED(hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(stream->allocator, 2, 8,
            attributes, stream->current)))
    {
        WARN("Failed to initialize sample allocator, hr %#lx.\n", hr);
    }

    if (attributes)
        IMFAttributes_Release(attributes);

    return hr;
}

static BOOL source_reader_allow_video_processor(struct source_reader *reader, BOOL *advanced)
{
    UINT32 value;

    *advanced = FALSE;
    if (!reader->attributes)
        return FALSE;

    if (SUCCEEDED(IMFAttributes_GetUINT32(reader->attributes, &MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, &value)))
        *advanced = value;
    if (SUCCEEDED(IMFAttributes_GetUINT32(reader->attributes, &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, &value)))
        return value || *advanced;

    return *advanced;
}

static HRESULT source_reader_create_transform(struct source_reader *reader, BOOL decoder, BOOL allow_processor,
        IMFMediaType *input_type, IMFMediaType *output_type, struct transform_entry **out)
{
    MFT_REGISTER_TYPE_INFO in_type, out_type;
    struct transform_entry *entry;
    IMFActivate **activates;
    GUID category;
    IMFTransform *transform;
    UINT i, count;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetMajorType(input_type, &in_type.guidMajorType))
            || FAILED(hr = IMFMediaType_GetGUID(input_type, &MF_MT_SUBTYPE, &in_type.guidSubtype)))
        return hr;
    if (FAILED(hr = IMFMediaType_GetMajorType(output_type, &out_type.guidMajorType))
            || FAILED(hr = IMFMediaType_GetGUID(output_type, &MF_MT_SUBTYPE, &out_type.guidSubtype)))
        return hr;

    if (IsEqualGUID(&out_type.guidMajorType, &MFMediaType_Video))
        category = decoder ? MFT_CATEGORY_VIDEO_DECODER : MFT_CATEGORY_VIDEO_PROCESSOR;
    else if (IsEqualGUID(&out_type.guidMajorType, &MFMediaType_Audio))
        category = decoder ? MFT_CATEGORY_AUDIO_DECODER : MFT_CATEGORY_AUDIO_EFFECT;
    else
        return MF_E_TOPO_CODEC_NOT_FOUND;

    if (!(entry = calloc(1, sizeof(*entry))))
        return E_OUTOFMEMORY;
    list_init(&entry->entry);
    entry->category = category;

    if (IsEqualGUID(&out_type.guidMajorType, &MFMediaType_Audio)
            && SUCCEEDED(IMFMediaType_GetUINT32(output_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &entry->min_buffer_size)))
    {
        UINT32 bytes_per_second;

        if (SUCCEEDED(IMFMediaType_GetUINT32(output_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &bytes_per_second)))
            entry->min_buffer_size = max(entry->min_buffer_size, bytes_per_second);
    }

    if (IsEqualGUID(&out_type.guidMajorType, &MFMediaType_Video) && IsEqualGUID(&out_type.guidSubtype, &MFVideoFormat_ABGR32)
            && IsEqualGUID(&category, &MFT_CATEGORY_VIDEO_PROCESSOR))
    {
        /* The video processor isn't registered for MFVideoFormat_ABGR32, and native even only supports that format when
         * D3D-enabled, we still want to instantiate a video processor in such case, so fixup the subtype for MFTEnumEx.
         */
        WARN("Fixing up MFVideoFormat_ABGR32 subtype for the video processor\n");
        out_type.guidSubtype = MFVideoFormat_RGB32;
    }


    count = 0;
    if (SUCCEEDED(hr = MFTEnumEx(category, 0, &in_type, allow_processor ? NULL : &out_type, &activates, &count)))
    {
        if (!count)
        {
            free(entry);
            return MF_E_TOPO_CODEC_NOT_FOUND;
        }

        for (i = 0; i < count; i++)
        {
            IMFAttributes *attributes;
            IMFMediaType *media_type;

            if (FAILED(hr = IMFActivate_ActivateObject(activates[i], &IID_IMFTransform, (void **)&transform)))
                continue;

            if (!reader->device_manager || FAILED(IMFTransform_GetAttributes(transform, &attributes)))
                entry->attributes_initialized = TRUE;
            else
            {
                UINT32 d3d_aware = FALSE;

                if (reader->flags & SOURCE_READER_DXGI_DEVICE_MANAGER)
                {
                    if (SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D11_AWARE, &d3d_aware)) && d3d_aware)
                        IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)reader->device_manager);
                }
                else if (reader->flags & SOURCE_READER_D3D9_DEVICE_MANAGER)
                {
                    if (SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_SA_D3D_AWARE, &d3d_aware)) && d3d_aware)
                        IMFTransform_ProcessMessage(transform, MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)reader->device_manager);
                }

                entry->attributes_initialized = !d3d_aware;
                IMFAttributes_Release(attributes);
            }

            if (SUCCEEDED(hr = IMFTransform_SetInputType(transform, 0, input_type, 0))
                    && SUCCEEDED(hr = IMFTransform_GetInputCurrentType(transform, 0, &media_type)))
            {
                if (SUCCEEDED(hr = update_media_type_from_upstream(output_type, media_type))
                        && FAILED(hr = IMFTransform_SetOutputType(transform, 0, output_type, 0))
                        && FAILED(hr = set_matching_transform_output_type(transform, output_type)) && allow_processor
                        && SUCCEEDED(hr = IMFTransform_GetOutputAvailableType(transform, 0, 0, &media_type)))
                {
                    struct transform_entry *converter;

                    if (SUCCEEDED(hr = IMFTransform_SetOutputType(transform, 0, media_type, 0))
                            && SUCCEEDED(hr = update_media_type_from_upstream(output_type, media_type))
                            && SUCCEEDED(hr = source_reader_create_transform(reader, FALSE, FALSE, media_type, output_type, &converter)))
                        list_add_tail(&entry->entry, &converter->entry);

                    IMFMediaType_Release(media_type);
                }

                if (SUCCEEDED(hr))
                {
                    entry->transform = transform;
                    *out = entry;
                    break;
                }
            }

            IMFTransform_Release(transform);
        }

        for (i = 0; i < count; ++i)
            IMFActivate_Release(activates[i]);
        CoTaskMemFree(activates);
    }

    if (FAILED(hr))
        free(entry);
    return hr;
}

static HRESULT source_reader_create_decoder_for_stream(struct source_reader *reader, DWORD index, IMFMediaType *output_type)
{
    BOOL enable_advanced = FALSE, allow_processor = TRUE;
    struct media_stream *stream = &reader->streams[index];
    IMFMediaType *input_type;
    unsigned int i = 0;
    GUID major;
    HRESULT hr;

    if (SUCCEEDED(IMFMediaType_GetMajorType(output_type, &major)) && IsEqualGUID(&major, &MFMediaType_Video))
        allow_processor = source_reader_allow_video_processor(reader, &enable_advanced);

    while (SUCCEEDED(hr = source_reader_get_native_media_type(reader, index, i++, &input_type)))
    {
        struct transform_entry *entry;

        /* first, try to append a single processor, then try again with a decoder and a processor */
        if ((allow_processor && SUCCEEDED(hr = source_reader_create_transform(reader, FALSE, FALSE, input_type, output_type, &entry)))
                || SUCCEEDED(hr = source_reader_create_transform(reader, TRUE, allow_processor, input_type, output_type, &entry)))
        {
            struct list *ptr = list_head(&entry->entry);
            struct transform_entry *service = ptr ? LIST_ENTRY(ptr, struct transform_entry, entry) : entry;
            IMFMediaTypeHandler *type_handler;

            if (enable_advanced)
            {
                /* when advanced video processing is enabled, converters are exposed as stream transform service */
                stream->transform_service = service->transform;
                IMFTransform_AddRef(stream->transform_service);
            }
            else
            {
                /* when advanced video processing is disabled, only decoders are exposed as stream transform service */
                if (IsEqualGUID(&entry->category, &MFT_CATEGORY_AUDIO_DECODER)
                        || IsEqualGUID(&entry->category, &MFT_CATEGORY_VIDEO_DECODER))
                {
                    stream->transform_service = entry->transform;
                    IMFTransform_AddRef(stream->transform_service);

                    /* converters are hidden from the stream transforms */
                    if (service != entry)
                        service->hidden = TRUE;
                }
                else
                {
                    /* converters are hidden from the stream transforms */
                    entry->hidden = TRUE;
                }
            }

            /* move any additional transforms that have been created */
            list_move_head(&stream->transforms, &entry->entry);
            list_add_head(&stream->transforms, &entry->entry);

            if (SUCCEEDED(source_reader_get_source_type_handler(reader, index, &type_handler)))
            {
                if (FAILED(hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, input_type)))
                    WARN("Failed to set current input media type, hr %#lx\n", hr);
                IMFMediaTypeHandler_Release(type_handler);
            }

            if (FAILED(hr = IMFTransform_GetOutputCurrentType(service->transform, 0, &output_type)))
                WARN("Failed to get decoder output media type, hr %#lx\n", hr);
            else
            {
                IMFMediaType_CopyAllItems(output_type, (IMFAttributes *)stream->current);
                IMFMediaType_Release(output_type);
            }

            IMFMediaType_Release(input_type);
            return S_OK;
        }

        IMFMediaType_Release(input_type);
    }

    return MF_E_TOPO_CODEC_NOT_FOUND;
}

static HRESULT WINAPI src_reader_SetCurrentMediaType(IMFSourceReaderEx *iface, DWORD index, DWORD *reserved,
        IMFMediaType *type)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    HRESULT hr;

    TRACE("%p, %#lx, %p, %p.\n", iface, index, reserved, type);

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

    hr = source_reader_set_compatible_media_type(reader, index, type);
    if (hr == S_FALSE)
        hr = source_reader_create_decoder_for_stream(reader, index, type);
    if (SUCCEEDED(hr))
        hr = source_reader_setup_sample_allocator(reader, index);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT WINAPI src_reader_SetCurrentPosition(IMFSourceReaderEx *iface, REFGUID format, REFPROPVARIANT position)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    struct source_reader_async_command *command;
    unsigned int i;
    DWORD flags;
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
        for (i = 0; i < reader->stream_count; ++i)
        {
            reader->streams[i].last_sample_ts = 0;
        }

        if (reader->async_callback)
        {
            if (SUCCEEDED(hr = source_reader_create_async_op(SOURCE_READER_ASYNC_SEEK, &command)))
            {
                command->u.seek.format = *format;
                PropVariantCopy(&command->u.seek.position, position);

                hr = MFPutWorkItem(reader->queue, &reader->async_commands_callback, &command->IUnknown_iface);
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
    struct media_stream *stream;
    DWORD actual_index_tmp;
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
                        WARN("Failed to request a sample, hr %#lx.\n", hr);
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

    TRACE("Stream %lu, got sample %p, flags %#lx.\n", *actual_index, *sample, *stream_flags);

    return hr;
}

static HRESULT source_reader_read_sample_async(struct source_reader *reader, unsigned int index, unsigned int flags,
        DWORD *actual_index, DWORD *stream_flags, LONGLONG *timestamp, IMFSample **sample)
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

            hr = MFPutWorkItem(reader->queue, &reader->async_commands_callback, &command->IUnknown_iface);
            IUnknown_Release(&command->IUnknown_iface);
        }
    }

    return hr;
}

static HRESULT WINAPI src_reader_ReadSample(IMFSourceReaderEx *iface, DWORD index, DWORD flags, DWORD *actual_index,
        DWORD *stream_flags, LONGLONG *timestamp, IMFSample **sample)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    HRESULT hr;

    TRACE("%p, %#lx, %#lx, %p, %p, %p, %p\n", iface, index, flags, actual_index, stream_flags, timestamp, sample);

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

    hr = MFPutWorkItem(reader->queue, &reader->async_commands_callback, &command->IUnknown_iface);
    IUnknown_Release(&command->IUnknown_iface);

    return hr;
}

static HRESULT WINAPI src_reader_Flush(IMFSourceReaderEx *iface, DWORD index)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    HRESULT hr;

    TRACE("%p, %#lx.\n", iface, index);

    EnterCriticalSection(&reader->cs);

    if (reader->async_callback)
        hr = source_reader_flush_async(reader, index);
    else
        hr = source_reader_flush(reader, index);

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static HRESULT WINAPI src_reader_GetServiceForStream(IMFSourceReaderEx *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    IUnknown *obj = NULL;
    HRESULT hr = S_OK;

    TRACE("%p, %#lx, %s, %s, %p\n", iface, index, debugstr_guid(service), debugstr_guid(riid), object);

    EnterCriticalSection(&reader->cs);

    switch (index)
    {
        case MF_SOURCE_READER_MEDIASOURCE:
            obj = (IUnknown *)reader->source;
            break;
        default:
            if (index == MF_SOURCE_READER_FIRST_VIDEO_STREAM)
                index = reader->first_video_stream_index;
            else if (index == MF_SOURCE_READER_FIRST_AUDIO_STREAM)
                index = reader->first_audio_stream_index;

            if (index >= reader->stream_count)
                hr = MF_E_INVALIDSTREAMNUMBER;
            else if (!(obj = (IUnknown *)reader->streams[index].transform_service))
                hr = E_NOINTERFACE;
            break;
    }

    if (obj)
        IUnknown_AddRef(obj);

    LeaveCriticalSection(&reader->cs);

    if (obj)
    {
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
    }

    if (obj)
        IUnknown_Release(obj);

    return hr;
}

static HRESULT WINAPI src_reader_GetPresentationAttribute(IMFSourceReaderEx *iface, DWORD index,
        REFGUID guid, PROPVARIANT *value)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    IMFStreamDescriptor *sd;
    BOOL selected;
    HRESULT hr;

    TRACE("%p, %#lx, %s, %p.\n", iface, index, debugstr_guid(guid), value);

    switch (index)
    {
        case MF_SOURCE_READER_MEDIASOURCE:
            if (IsEqualGUID(guid, &MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS))
            {
                DWORD flags;

                if (FAILED(hr = IMFMediaSource_GetCharacteristics(reader->source, &flags)))
                    return hr;

                value->vt = VT_UI4;
                value->ulVal = flags;
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

static HRESULT WINAPI src_reader_SetNativeMediaType(IMFSourceReaderEx *iface, DWORD stream_index,
        IMFMediaType *media_type, DWORD *stream_flags)
{
    FIXME("%p, %#lx, %p, %p.\n", iface, stream_index, media_type, stream_flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_AddTransformForStream(IMFSourceReaderEx *iface, DWORD stream_index,
        IUnknown *transform)
{
    FIXME("%p, %#lx, %p.\n", iface, stream_index, transform);

    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_RemoveAllTransformsForStream(IMFSourceReaderEx *iface, DWORD stream_index)
{
    FIXME("%p, %#lx.\n", iface, stream_index);

    return E_NOTIMPL;
}

static struct transform_entry *get_transform_at_index(struct media_stream *stream, UINT index)
{
    struct transform_entry *entry;

    LIST_FOR_EACH_ENTRY(entry, &stream->transforms, struct transform_entry, entry)
        if (!entry->hidden && !index--)
            return entry;

    return NULL;
}

static HRESULT WINAPI src_reader_GetTransformForStream(IMFSourceReaderEx *iface, DWORD stream_index,
        DWORD transform_index, GUID *category, IMFTransform **transform)
{
    struct source_reader *reader = impl_from_IMFSourceReaderEx(iface);
    struct transform_entry *entry;
    HRESULT hr;

    TRACE("%p, %#lx, %#lx, %p, %p.\n", iface, stream_index, transform_index, category, transform);

    if (!transform)
        return E_POINTER;

    EnterCriticalSection(&reader->cs);

    if (stream_index == MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        stream_index = reader->first_video_stream_index;
    else if (stream_index == MF_SOURCE_READER_FIRST_AUDIO_STREAM)
        stream_index = reader->first_audio_stream_index;

    if (stream_index >= reader->stream_count)
        hr = MF_E_INVALIDSTREAMNUMBER;
    else if (!(entry = get_transform_at_index(&reader->streams[stream_index], transform_index)))
        hr = MF_E_INVALIDINDEX;
    else
    {
        if (category)
            *category = entry->category;
        *transform = entry->transform;
        IMFTransform_AddRef(*transform);
        hr = S_OK;
    }

    LeaveCriticalSection(&reader->cs);

    return hr;
}

static const IMFSourceReaderExVtbl srcreader_vtbl =
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
    src_reader_GetPresentationAttribute,
    src_reader_SetNativeMediaType,
    src_reader_AddTransformForStream,
    src_reader_RemoveAllTransformsForStream,
    src_reader_GetTransformForStream,
};

static DWORD reader_get_first_stream_index(IMFPresentationDescriptor *descriptor, const GUID *major)
{
    DWORD count, i;
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
                    WARN("Failed to get stream major type, hr %#lx.\n", hr);
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

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSourceReaderEx_iface.lpVtbl = &srcreader_vtbl;
    object->source_events_callback.lpVtbl = &source_events_callback_vtbl;
    object->stream_events_callback.lpVtbl = &stream_events_callback_vtbl;
    object->async_commands_callback.lpVtbl = &async_commands_callback_vtbl;
    object->public_refcount = 1;
    object->refcount = 1;
    list_init(&object->responses);
    if (shutdown_on_release)
        object->flags |= SOURCE_READER_SHUTDOWN_ON_RELEASE;
    object->source = source;
    IMFMediaSource_AddRef(object->source);
    InitializeCriticalSection(&object->cs);
    InitializeConditionVariable(&object->sample_event);
    InitializeConditionVariable(&object->state_event);
    InitializeConditionVariable(&object->stop_event);

    if (FAILED(hr = IMFMediaSource_CreatePresentationDescriptor(object->source, &object->descriptor)))
        goto failed;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(object->descriptor, &object->stream_count)))
        goto failed;

    if (!(object->streams = calloc(object->stream_count, sizeof(*object->streams))))
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

        list_init(&object->streams[i].transforms);

        if (FAILED(hr = MFCreateMediaType(&object->streams[i].current)))
            break;

        if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(object->descriptor, i, &selected, &sd)))
            break;

        if (FAILED(hr = IMFStreamDescriptor_GetStreamIdentifier(sd, &object->streams[i].id)))
            WARN("Failed to get stream identifier, hr %#lx.\n", hr);

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

        object->streams[i].reader = object;
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
        object->attributes = attributes;
        IMFAttributes_AddRef(object->attributes);

        IMFAttributes_GetUnknown(attributes, &MF_SOURCE_READER_ASYNC_CALLBACK, &IID_IMFSourceReaderCallback,
                (void **)&object->async_callback);
        if (object->async_callback)
            TRACE("Using async callback %p.\n", object->async_callback);

        IMFAttributes_GetUnknown(attributes, &MF_SOURCE_READER_D3D_MANAGER, &IID_IUnknown, (void **)&object->device_manager);
        if (object->device_manager)
        {
            IUnknown *unk = NULL;

            if (SUCCEEDED(IUnknown_QueryInterface(object->device_manager, &IID_IMFDXGIDeviceManager, (void **)&unk)))
                object->flags |= SOURCE_READER_DXGI_DEVICE_MANAGER;
            else if (SUCCEEDED(IUnknown_QueryInterface(object->device_manager, &IID_IDirect3DDeviceManager9, (void **)&unk)))
                object->flags |= SOURCE_READER_D3D9_DEVICE_MANAGER;

            if (!(object->flags & (SOURCE_READER_HAS_DEVICE_MANAGER)))
            {
                WARN("Unknown device manager.\n");
                IUnknown_Release(object->device_manager);
                object->device_manager = NULL;
            }

            if (unk)
                IUnknown_Release(unk);
        }
    }

    if (FAILED(hr = MFLockSharedWorkQueue(L"", 0, NULL, &object->queue)))
        WARN("Failed to acquired shared queue, hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
        hr = IMFSourceReaderEx_QueryInterface(&object->IMFSourceReaderEx_iface, riid, out);

failed:
    IMFSourceReaderEx_Release(&object->IMFSourceReaderEx_iface);
    return hr;
}

static HRESULT create_source_reader_from_stream(IMFByteStream *stream, IMFAttributes *attributes,
        REFIID riid, void **out)
{
    IPropertyStore *props = NULL;
    IMFSourceResolver *resolver;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    HRESULT hr;

    if (FAILED(hr = MFCreateSourceResolver(&resolver)))
        return hr;

    if (attributes)
        IMFAttributes_GetUnknown(attributes, &MF_SOURCE_READER_MEDIASOURCE_CONFIG, &IID_IPropertyStore,
                (void **)&props);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE
            | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE, props, &obj_type, (IUnknown **)&source);
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
    else if (IsEqualGUID(clsid, &CLSID_MFSinkWriter))
    {
        return create_sink_writer_from_url(url, NULL, attributes, riid, out);
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
            hr = create_sink_writer_from_url(NULL, stream, attributes, riid, out);
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

static const IClassFactoryVtbl classfactoryvtbl =
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
