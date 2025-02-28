/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "mfsrcsnk_private.h"

#include "wine/list.h"
#include "wine/debug.h"
#include "wine/winedmo.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

#define DEFINE_MF_ASYNC_CALLBACK_(type, name, impl_from, pfx, mem, expr)                           \
    static struct type *impl_from(IMFAsyncCallback *iface)                                         \
    {                                                                                              \
        return CONTAINING_RECORD(iface, struct type, mem);                                         \
    }                                                                                              \
    static HRESULT WINAPI pfx##_QueryInterface(IMFAsyncCallback *iface, REFIID iid, void **out)    \
    {                                                                                              \
        if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IMFAsyncCallback))              \
        {                                                                                          \
            IMFAsyncCallback_AddRef((*out = iface));                                               \
            return S_OK;                                                                           \
        }                                                                                          \
        *out = NULL;                                                                               \
        return E_NOINTERFACE;                                                                      \
    }                                                                                              \
    static ULONG WINAPI pfx##_AddRef(IMFAsyncCallback *iface)                                      \
    {                                                                                              \
        struct type *object = impl_from(iface);                                                    \
        return IUnknown_AddRef((IUnknown *)(expr));                                                \
    }                                                                                              \
    static ULONG WINAPI pfx##_Release(IMFAsyncCallback *iface)                                     \
    {                                                                                              \
        struct type *object = impl_from(iface);                                                    \
        return IUnknown_Release((IUnknown *)(expr));                                               \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue) \
    {                                                                                              \
        return E_NOTIMPL;                                                                          \
    }                                                                                              \
    static HRESULT WINAPI pfx##_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)            \
    {                                                                                              \
        struct type *object = impl_from(iface);                                                    \
        return type##_##name(object, result);                                                      \
    }                                                                                              \
    static const IMFAsyncCallbackVtbl pfx##_vtbl =                                                 \
    {                                                                                              \
            pfx##_QueryInterface,                                                                  \
            pfx##_AddRef,                                                                          \
            pfx##_Release,                                                                         \
            pfx##_GetParameters,                                                                   \
            pfx##_Invoke,                                                                          \
    };

#define DEFINE_MF_ASYNC_CALLBACK(type, name, base_iface)                                           \
    DEFINE_MF_ASYNC_CALLBACK_(type, name, type##_from_##name, type##_##name, name##_iface, &object->base_iface)

struct object_entry
{
    struct list entry;
    IUnknown *object;
};

static void object_entry_destroy(struct object_entry *entry)
{
    if (entry->object) IUnknown_Release( entry->object );
    free(entry);
}

static HRESULT object_entry_create(IUnknown *object, struct object_entry **out)
{
    struct object_entry *entry;

    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;
    if ((entry->object = object)) IUnknown_AddRef( entry->object );

    *out = entry;
    return S_OK;
}

static HRESULT object_queue_push(struct list *queue, IUnknown *object)
{
    struct object_entry *entry;
    HRESULT hr;

    if (SUCCEEDED(hr = object_entry_create(object, &entry)))
        list_add_tail(queue, &entry->entry);

    return hr;
}

static HRESULT object_queue_pop(struct list *queue, IUnknown **object)
{
    struct object_entry *entry;
    struct list *ptr;

    if (!(ptr = list_head(queue))) return E_PENDING;
    entry = LIST_ENTRY(ptr, struct object_entry, entry);

    if ((*object = entry->object)) IUnknown_AddRef(*object);
    list_remove(&entry->entry);
    object_entry_destroy(entry);

    return S_OK;
}

static void object_queue_clear(struct list *queue)
{
    IUnknown *object;
    while (object_queue_pop(queue, &object) != E_PENDING)
        if (object) IUnknown_Release(object);
}

struct async_start_params
{
    IUnknown IUnknown_iface;
    LONG refcount;
    IMFPresentationDescriptor *descriptor;
    GUID format;
    PROPVARIANT position;
};

static struct async_start_params *async_start_params_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct async_start_params, IUnknown_iface);
}

static HRESULT WINAPI async_start_params_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    struct async_start_params *params = async_start_params_from_IUnknown(iface);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(&params->IUnknown_iface);
        *obj = &params->IUnknown_iface;
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_start_params_AddRef(IUnknown *iface)
{
    struct async_start_params *params = async_start_params_from_IUnknown(iface);
    return InterlockedIncrement(&params->refcount);
}

static ULONG WINAPI async_start_params_Release(IUnknown *iface)
{
    struct async_start_params *params = async_start_params_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&params->refcount);

    if (!refcount)
    {
        IMFPresentationDescriptor_Release(params->descriptor);
        PropVariantClear(&params->position);
        free(params);
    }

    return refcount;
}

static const IUnknownVtbl async_start_params_vtbl =
{
    async_start_params_QueryInterface,
    async_start_params_AddRef,
    async_start_params_Release,
};

static HRESULT async_start_params_create(IMFPresentationDescriptor *descriptor, const GUID *time_format,
        const PROPVARIANT *position, IUnknown **out)
{
    struct async_start_params *params;

    if (!(params = calloc(1, sizeof(*params))))
        return E_OUTOFMEMORY;

    params->IUnknown_iface.lpVtbl = &async_start_params_vtbl;
    params->refcount = 1;

    params->descriptor = descriptor;
    IMFPresentationDescriptor_AddRef(descriptor);
    params->format = *time_format;
    PropVariantCopy(&params->position, position);

    *out = &params->IUnknown_iface;
    return S_OK;
}

struct media_stream
{
    IMFMediaStream IMFMediaStream_iface;
    IMFAsyncCallback async_request_iface;
    LONG refcount;

    IMFMediaSource *source;
    IMFMediaEventQueue *queue;
    IMFStreamDescriptor *descriptor;
    struct list samples;
    struct list tokens;

    BOOL active;
    BOOL eos;
};

struct media_source
{
    IMFMediaSource IMFMediaSource_iface;
    IMFGetService IMFGetService_iface;
    IMFRateSupport IMFRateSupport_iface;
    IMFRateControl IMFRateControl_iface;
    IMFAsyncCallback async_create_iface;
    IMFAsyncCallback async_start_iface;
    IMFAsyncCallback async_stop_iface;
    IMFAsyncCallback async_pause_iface;
    IMFAsyncCallback async_read_iface;
    LONG refcount;

    CRITICAL_SECTION cs;
    IMFMediaEventQueue *queue;
    IMFByteStream *stream;
    WCHAR *url;
    float rate;

    struct winedmo_demuxer winedmo_demuxer;
    struct winedmo_stream winedmo_stream;
    UINT64 file_size;
    INT64 duration;
    UINT stream_count;
    WCHAR mime_type[256];

    UINT *stream_map;
    struct media_stream **streams;

    enum
    {
        SOURCE_STOPPED,
        SOURCE_PAUSED,
        SOURCE_RUNNING,
        SOURCE_SHUTDOWN,
    } state;
    UINT pending_reads;
};

static struct media_source *media_source_from_IMFMediaSource(IMFMediaSource *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFMediaSource_iface);
}

static void media_stream_send_sample(struct media_stream *stream, IMFSample *sample, IUnknown *token)
{
    HRESULT hr = S_OK;

    if (!token || SUCCEEDED(hr = IMFSample_SetUnknown(sample, &MFSampleExtension_Token, token)))
        hr = IMFMediaEventQueue_QueueEventParamUnk(stream->queue, MEMediaSample,
                &GUID_NULL, S_OK, (IUnknown *)sample);

    if (FAILED(hr)) ERR("Failed to send stream %p sample, hr %#lx\n", stream, hr);
}

static struct media_stream *media_stream_from_index(struct media_source *source, UINT index)
{
    UINT i;

    for (i = 0; i < source->stream_count; i++)
        if (source->stream_map[i] == index)
            return source->streams[i];

    WARN("Failed to find stream with index %u\n", index);
    return NULL;
}

static HRESULT media_source_send_sample(struct media_source *source, UINT index, IMFSample *sample)
{
    struct media_stream *stream;
    IUnknown *token;
    HRESULT hr;

    if (!(stream = media_stream_from_index(source, index)) || !stream->active)
        return S_FALSE;

    if (SUCCEEDED(hr = object_queue_pop(&stream->tokens, &token)))
    {
        media_stream_send_sample(stream, sample, token);
        if (token) IUnknown_Release(token);
        return S_OK;
    }

    if (FAILED(hr = object_queue_push(&stream->samples, (IUnknown *)sample)))
        return hr;

    return S_FALSE;
}

static void queue_media_event_object(IMFMediaEventQueue *queue, MediaEventType type, IUnknown *object)
{
    HRESULT hr;
    if (FAILED(hr = IMFMediaEventQueue_QueueEventParamUnk(queue, type, &GUID_NULL, S_OK, object)))
        ERR("Failed to queue event of type %#lx to queue %p, hr %#lx\n", type, queue, hr);
}

static void queue_media_event_value(IMFMediaEventQueue *queue, MediaEventType type, const PROPVARIANT *value)
{
    HRESULT hr;
    if (FAILED(hr = IMFMediaEventQueue_QueueEventParamVar(queue, type, &GUID_NULL, S_OK, value)))
        ERR("Failed to queue event of type %#lx to queue %p, hr %#lx\n", type, queue, hr);
}

static void queue_media_source_read(struct media_source *source)
{
    HRESULT hr;

    if (FAILED(hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &source->async_read_iface, NULL)))
        ERR("Failed to queue async read for source %p, hr %#lx\n", source, hr);
    source->pending_reads++;
}

static void media_stream_start(struct media_stream *stream, UINT index, const PROPVARIANT *position)
{
    struct media_source *source = media_source_from_IMFMediaSource(stream->source);
    BOOL starting = source->state == SOURCE_STOPPED, seeking = !starting && position->vt != VT_EMPTY;
    BOOL was_active = !starting && stream->active;

    TRACE("stream %p, index %u, position %s\n", stream, index, debugstr_propvar(position));

    if (position->vt != VT_EMPTY)
    {
        object_queue_clear(&stream->tokens);
        object_queue_clear(&stream->samples);
    }

    if (stream->active)
    {
        queue_media_event_object(source->queue, was_active ? MEUpdatedStream : MENewStream,
                (IUnknown *)&stream->IMFMediaStream_iface);
        queue_media_event_value(stream->queue, seeking ? MEStreamSeeked : MEStreamStarted, position);
    }

    if (position->vt == VT_EMPTY && stream->active)
    {
        struct list samples = LIST_INIT(samples);
        IMFSample *sample;
        UINT token_count;

        list_move_head(&samples, &stream->samples);
        while (object_queue_pop(&samples, (IUnknown **)&sample) != E_PENDING)
        {
            media_source_send_sample(source, index, sample);
            IMFSample_Release(sample);
        }

        token_count = list_count(&stream->tokens);
        while (source->pending_reads < token_count)
            queue_media_source_read(source);
    }
}

static HRESULT media_source_start(struct media_source *source, IMFPresentationDescriptor *descriptor,
        GUID *format, PROPVARIANT *position)
{
    BOOL starting = source->state == SOURCE_STOPPED, seeking = !starting && position->vt != VT_EMPTY;
    NTSTATUS status;
    DWORD i, count;
    HRESULT hr;

    TRACE("source %p, descriptor %p, format %s, position %s\n", source, descriptor,
            debugstr_guid(format), debugstr_propvar(position));

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    if (source->state == SOURCE_STOPPED && position->vt == VT_EMPTY)
    {
        position->vt = VT_I8;
        position->hVal.QuadPart = 0;
    }

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        if (position->vt != VT_EMPTY)
            stream->eos = FALSE;
        stream->active = FALSE;
    }

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(descriptor, &count)))
        WARN("Failed to get presentation descriptor stream count, hr %#lx\n", hr);

    for (i = 0; i < count; i++)
    {
        IMFStreamDescriptor *stream_descriptor;
        BOOL selected;
        DWORD id;

        if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(descriptor, i,
                &selected, &stream_descriptor)))
            break;

        if (FAILED(hr = IMFStreamDescriptor_GetStreamIdentifier(stream_descriptor, &id)))
            WARN("Failed to get stream descriptor id, hr %#lx\n", hr);
        else if (id > source->stream_count)
            WARN("Invalid stream descriptor id %lu, hr %#lx\n", id, hr);
        else
        {
            struct media_stream *stream = source->streams[id - 1];
            stream->active = selected;
        }

        IMFStreamDescriptor_Release(stream_descriptor);
    }

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        media_stream_start(stream, i, position);
    }

    if (position->vt == VT_I8 && (status = winedmo_demuxer_seek(source->winedmo_demuxer, position->hVal.QuadPart)))
        WARN("Failed to seek to %I64d, status %#lx\n", position->hVal.QuadPart, status);
    source->state = SOURCE_RUNNING;

    queue_media_event_value(source->queue, seeking ? MESourceSeeked : MESourceStarted, position);
    return hr;
}

static HRESULT media_source_async_start(struct media_source *source, IMFAsyncResult *result)
{
    struct async_start_params *params;
    IUnknown *state;
    HRESULT hr;

    if (!(state = IMFAsyncResult_GetStateNoAddRef(result))) return E_INVALIDARG;
    params = async_start_params_from_IUnknown(state);

    EnterCriticalSection(&source->cs);

    if (FAILED(hr = media_source_start(source, params->descriptor, &params->format, &params->position)))
        WARN("Failed to start source %p, hr %#lx\n", source, hr);

    LeaveCriticalSection(&source->cs);

    return hr;
}

DEFINE_MF_ASYNC_CALLBACK(media_source, async_start, IMFMediaSource_iface)

static void media_stream_stop(struct media_stream *stream)
{
    TRACE("stream %p\n", stream);

    object_queue_clear(&stream->tokens);
    object_queue_clear(&stream->samples);

    if (stream->active)
        queue_media_event_value(stream->queue, MEStreamStopped, NULL);
}

static HRESULT media_source_stop(struct media_source *source)
{
    unsigned int i;

    TRACE("source %p\n", source);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        media_stream_stop(stream);
    }

    source->state = SOURCE_STOPPED;
    queue_media_event_value(source->queue, MESourceStopped, NULL);
    return S_OK;
}

static HRESULT media_source_async_stop(struct media_source *source, IMFAsyncResult *result)
{
    HRESULT hr;

    EnterCriticalSection(&source->cs);

    if (FAILED(hr = media_source_stop(source)))
        WARN("Failed to stop source %p, hr %#lx\n", source, hr);

    LeaveCriticalSection(&source->cs);

    return hr;
}

DEFINE_MF_ASYNC_CALLBACK(media_source, async_stop, IMFMediaSource_iface)

static void media_stream_pause(struct media_stream *stream)
{
    TRACE("stream %p\n", stream);

    if (stream->active)
        queue_media_event_value(stream->queue, MEStreamPaused, NULL);
}

static HRESULT media_source_pause(struct media_source *source)
{
    unsigned int i;

    TRACE("source %p\n", source);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        media_stream_pause(stream);
    }

    source->state = SOURCE_PAUSED;
    queue_media_event_value(source->queue, MESourcePaused, NULL);
    return S_OK;
}

static HRESULT media_source_async_pause(struct media_source *source, IMFAsyncResult *result)
{
    HRESULT hr;

    EnterCriticalSection(&source->cs);

    if (FAILED(hr = media_source_pause(source)))
        WARN("Failed to pause source %p, hr %#lx\n", source, hr);

    LeaveCriticalSection(&source->cs);

    return hr;
}

DEFINE_MF_ASYNC_CALLBACK(media_source, async_pause, IMFMediaSource_iface)

static HRESULT create_media_buffer_sample(UINT buffer_size, IMFSample **sample, IMediaBuffer **media_buffer)
{
    IMFMediaBuffer *buffer;
    HRESULT hr;

    if (FAILED(hr = MFCreateSample(sample)))
        return hr;
    if (SUCCEEDED(hr = MFCreateMemoryBuffer( buffer_size, &buffer )))
    {
        if (SUCCEEDED(hr = IMFSample_AddBuffer(*sample, buffer)))
            hr = MFCreateLegacyMediaBufferOnMFMediaBuffer(*sample, buffer, 0, media_buffer);
        IMFMediaBuffer_Release(buffer);
    }
    if (FAILED(hr))
    {
        IMFSample_Release(*sample);
        *sample = NULL;
    }
    return hr;
}

static HRESULT demuxer_read_sample(struct winedmo_demuxer demuxer, UINT *index, IMFSample **out)
{
    UINT buffer_size = 0x1000;
    IMFSample *sample;
    HRESULT hr;

    do
    {
        DMO_OUTPUT_DATA_BUFFER output = {0};
        NTSTATUS status;

        if (FAILED(hr = create_media_buffer_sample(buffer_size, &sample, &output.pBuffer)))
            return hr;
        if ((status = winedmo_demuxer_read(demuxer, index, &output, &buffer_size)))
        {
            if (status == STATUS_BUFFER_TOO_SMALL) hr = E_PENDING;
            else if (status == STATUS_END_OF_FILE) hr = MF_E_END_OF_STREAM;
            else hr = HRESULT_FROM_NT(status);
            IMFSample_Release(sample);
            sample = NULL;
        }
        IMediaBuffer_Release(output.pBuffer);
    }
    while (hr == E_PENDING);

    *out = sample;
    return hr;
}

static HRESULT media_source_send_eos(struct media_source *source, struct media_stream *stream)
{
    PROPVARIANT empty = {.vt = VT_EMPTY};
    UINT i;

    if (stream->active && !stream->eos && list_empty(&stream->samples))
    {
        queue_media_event_value(stream->queue, MEEndOfStream, &empty);
        stream->eos = TRUE;
    }

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *other = source->streams[i];
        if (other->active && !other->eos) return S_OK;
    }

    queue_media_event_value(source->queue, MEEndOfPresentation, &empty);
    source->state = SOURCE_STOPPED;
    return S_OK;
}

static HRESULT media_source_read(struct media_source *source)
{
    IMFSample *sample;
    UINT i, index;
    HRESULT hr;

    if (source->state != SOURCE_RUNNING)
        return S_OK;

    if (FAILED(hr = demuxer_read_sample(source->winedmo_demuxer, &index, &sample)) && hr != MF_E_END_OF_STREAM)
    {
        WARN("Failed to read stream %u data, hr %#lx\n", index, hr);
        return hr;
    }

    if (hr == MF_E_END_OF_STREAM)
    {
        for (i = 0; i < source->stream_count; i++)
            media_source_send_eos(source, source->streams[i]);
        return S_OK;
    }

    if ((hr = media_source_send_sample(source, index, sample)) == S_FALSE)
        queue_media_source_read(source);
    IMFSample_Release(sample);

    return hr;
}

static HRESULT media_source_async_read(struct media_source *source, IMFAsyncResult *result)
{
    HRESULT hr;

    EnterCriticalSection(&source->cs);
    source->pending_reads--;

    if (FAILED(hr = media_source_read(source)))
        WARN("Failed to request sample, hr %#lx\n", hr);

    LeaveCriticalSection(&source->cs);

    return S_OK;
}

DEFINE_MF_ASYNC_CALLBACK(media_source, async_read, IMFMediaSource_iface)

static struct media_stream *media_stream_from_IMFMediaStream(IMFMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct media_stream, IMFMediaStream_iface);
}

static HRESULT WINAPI media_stream_QueryInterface(IMFMediaStream *iface, REFIID riid, void **out)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);

    TRACE("stream %p, riid %s, out %p\n", stream, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IMFMediaStream))
    {
        IMFMediaStream_AddRef(&stream->IMFMediaStream_iface);
        *out = &stream->IMFMediaStream_iface;
        return S_OK;
    }

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_stream_AddRef(IMFMediaStream *iface)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);
    TRACE("stream %p, refcount %ld\n", stream, refcount);
    return refcount;
}

static ULONG WINAPI media_stream_Release(IMFMediaStream *iface)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("stream %p, refcount %ld\n", stream, refcount);

    if (!refcount)
    {
        stream->active = FALSE;
        media_stream_stop(stream);
        IMFMediaSource_Release(stream->source);
        IMFStreamDescriptor_Release(stream->descriptor);
        IMFMediaEventQueue_Release(stream->queue);
        free(stream);
    }

    return refcount;
}

static HRESULT WINAPI media_stream_GetEvent(IMFMediaStream *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    TRACE("stream %p, flags %#lx, event %p\n", stream, flags, event);
    return IMFMediaEventQueue_GetEvent(stream->queue, flags, event);
}

static HRESULT WINAPI media_stream_BeginGetEvent(IMFMediaStream *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    TRACE("stream %p, callback %p, state %p\n", stream, callback, state);
    return IMFMediaEventQueue_BeginGetEvent(stream->queue, callback, state);
}

static HRESULT WINAPI media_stream_EndGetEvent(IMFMediaStream *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    TRACE("stream %p, result %p, event %p\n", stream, result, event);
    return IMFMediaEventQueue_EndGetEvent(stream->queue, result, event);
}

static HRESULT WINAPI media_stream_QueueEvent(IMFMediaStream *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    TRACE("stream %p, event_type %#lx, ext_type %s, hr %#lx, value %p\n", stream, event_type, debugstr_guid(ext_type), hr, value);
    return IMFMediaEventQueue_QueueEventParamVar(stream->queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI media_stream_GetMediaSource(IMFMediaStream *iface, IMFMediaSource **out)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    struct media_source *source = media_source_from_IMFMediaSource(stream->source);
    HRESULT hr = S_OK;

    TRACE("stream %p, out %p\n", stream, out);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
        *out = &source->IMFMediaSource_iface;
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_stream_GetStreamDescriptor(IMFMediaStream* iface, IMFStreamDescriptor **descriptor)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    struct media_source *source = media_source_from_IMFMediaSource(stream->source);
    HRESULT hr = S_OK;

    TRACE("stream %p, descriptor %p\n", stream, descriptor);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        IMFStreamDescriptor_AddRef(stream->descriptor);
        *descriptor = stream->descriptor;
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT media_stream_async_request(struct media_stream *stream, IMFAsyncResult *result)
{
    struct media_source *source = media_source_from_IMFMediaSource(stream->source);
    IUnknown *token = IMFAsyncResult_GetStateNoAddRef(result);
    IMFSample *sample;
    HRESULT hr = S_OK;

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (source->state == SOURCE_RUNNING && SUCCEEDED(hr = object_queue_pop(&stream->samples, (IUnknown **)&sample)))
    {
        media_stream_send_sample(stream, sample, token);
        IMFSample_Release(sample);
    }
    else if (SUCCEEDED(hr = object_queue_push(&stream->tokens, token)) && source->state == SOURCE_RUNNING)
        queue_media_source_read(source);

    LeaveCriticalSection(&source->cs);

    return hr;
}

DEFINE_MF_ASYNC_CALLBACK(media_stream, async_request, IMFMediaStream_iface)

static HRESULT WINAPI media_stream_RequestSample(IMFMediaStream *iface, IUnknown *token)
{
    struct media_stream *stream = media_stream_from_IMFMediaStream(iface);
    struct media_source *source = media_source_from_IMFMediaSource(stream->source);
    HRESULT hr;

    TRACE("stream %p, token %p\n", stream, token);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!stream->active)
        hr = MF_E_MEDIA_SOURCE_WRONGSTATE;
    else if (stream->eos)
        hr = MF_E_END_OF_STREAM;
    else
        hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &stream->async_request_iface, token);

    LeaveCriticalSection(&source->cs);

    return hr;
}

static const IMFMediaStreamVtbl media_stream_vtbl =
{
    media_stream_QueryInterface,
    media_stream_AddRef,
    media_stream_Release,
    media_stream_GetEvent,
    media_stream_BeginGetEvent,
    media_stream_EndGetEvent,
    media_stream_QueueEvent,
    media_stream_GetMediaSource,
    media_stream_GetStreamDescriptor,
    media_stream_RequestSample,
};

static HRESULT media_stream_create(IMFMediaSource *source, IMFStreamDescriptor *descriptor, struct media_stream **out)
{
    struct media_stream *object;
    HRESULT hr;

    TRACE("source %p, descriptor %p, out %p\n", source, descriptor, out);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaStream_iface.lpVtbl = &media_stream_vtbl;
    object->async_request_iface.lpVtbl = &media_stream_async_request_vtbl;
    object->refcount = 1;

    if (FAILED(hr = MFCreateEventQueue(&object->queue)))
    {
        free(object);
        return hr;
    }

    IMFMediaSource_AddRef((object->source = source));
    IMFStreamDescriptor_AddRef((object->descriptor = descriptor));

    list_init(&object->tokens);
    list_init(&object->samples);

    TRACE("Created stream object %p\n", object);

    *out = object;
    return S_OK;
}

static struct media_source *media_source_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFGetService_iface);
}

static HRESULT WINAPI media_source_IMFGetService_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct media_source *source = media_source_from_IMFGetService(iface);
    return IMFMediaSource_QueryInterface(&source->IMFMediaSource_iface, riid, obj);
}

static ULONG WINAPI media_source_IMFGetService_AddRef(IMFGetService *iface)
{
    struct media_source *source = media_source_from_IMFGetService(iface);
    return IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
}

static ULONG WINAPI media_source_IMFGetService_Release(IMFGetService *iface)
{
    struct media_source *source = media_source_from_IMFGetService(iface);
    return IMFMediaSource_Release(&source->IMFMediaSource_iface);
}

static HRESULT WINAPI media_source_IMFGetService_GetService(IMFGetService *iface, REFGUID service,
        REFIID riid, void **obj)
{
    struct media_source *source = media_source_from_IMFGetService(iface);

    TRACE("source %p, service %s, riid %s, obj %p\n", source, debugstr_guid(service), debugstr_guid(riid), obj);

    if (IsEqualGUID(service, &MF_RATE_CONTROL_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFRateSupport))
        {
            IMFRateSupport_AddRef(&source->IMFRateSupport_iface);
            *obj = &source->IMFRateSupport_iface;
            return S_OK;
        }
        if (IsEqualIID(riid, &IID_IMFRateControl))
        {
            IMFRateControl_AddRef(&source->IMFRateControl_iface);
            *obj = &source->IMFRateControl_iface;
            return S_OK;
        }
    }

    FIXME("Unsupported service %s / riid %s\n", debugstr_guid(service), debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static const IMFGetServiceVtbl media_source_IMFGetService_vtbl =
{
    media_source_IMFGetService_QueryInterface,
    media_source_IMFGetService_AddRef,
    media_source_IMFGetService_Release,
    media_source_IMFGetService_GetService,
};

static struct media_source *media_source_from_IMFRateSupport(IMFRateSupport *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFRateSupport_iface);
}

static HRESULT WINAPI media_source_IMFRateSupport_QueryInterface(IMFRateSupport *iface, REFIID riid, void **obj)
{
    struct media_source *source = media_source_from_IMFRateSupport(iface);
    return IMFMediaSource_QueryInterface(&source->IMFMediaSource_iface, riid, obj);
}

static ULONG WINAPI media_source_IMFRateSupport_AddRef(IMFRateSupport *iface)
{
    struct media_source *source = media_source_from_IMFRateSupport(iface);
    return IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
}

static ULONG WINAPI media_source_IMFRateSupport_Release(IMFRateSupport *iface)
{
    struct media_source *source = media_source_from_IMFRateSupport(iface);
    return IMFMediaSource_Release(&source->IMFMediaSource_iface);
}

static HRESULT WINAPI media_source_IMFRateSupport_GetSlowestRate(IMFRateSupport *iface,
        MFRATE_DIRECTION direction, BOOL thin, float *rate)
{
    struct media_source *source = media_source_from_IMFRateSupport(iface);
    TRACE("source %p, direction %d, thin %d, rate %p\n", source, direction, thin, rate);
    *rate = 0.0f;
    return S_OK;
}

static HRESULT WINAPI media_source_IMFRateSupport_GetFastestRate(IMFRateSupport *iface,
        MFRATE_DIRECTION direction, BOOL thin, float *rate)
{
    struct media_source *source = media_source_from_IMFRateSupport(iface);
    TRACE("source %p, direction %d, thin %d, rate %p\n", source, direction, thin, rate);
    *rate = direction == MFRATE_FORWARD ? 1e6f : -1e6f;
    return S_OK;
}

static HRESULT WINAPI media_source_IMFRateSupport_IsRateSupported(IMFRateSupport *iface, BOOL thin,
        float rate, float *nearest_rate)
{
    struct media_source *source = media_source_from_IMFRateSupport(iface);
    TRACE("source %p, thin %d, rate %f, nearest_rate %p\n", source, thin, rate, nearest_rate);
    if (nearest_rate) *nearest_rate = rate;
    return rate >= -1e6f && rate <= 1e6f ? S_OK : MF_E_UNSUPPORTED_RATE;
}

static const IMFRateSupportVtbl media_source_IMFRateSupport_vtbl =
{
    media_source_IMFRateSupport_QueryInterface,
    media_source_IMFRateSupport_AddRef,
    media_source_IMFRateSupport_Release,
    media_source_IMFRateSupport_GetSlowestRate,
    media_source_IMFRateSupport_GetFastestRate,
    media_source_IMFRateSupport_IsRateSupported,
};

static struct media_source *media_source_from_IMFRateControl(IMFRateControl *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFRateControl_iface);
}

static HRESULT WINAPI media_source_IMFRateControl_QueryInterface(IMFRateControl *iface, REFIID riid, void **obj)
{
    struct media_source *source = media_source_from_IMFRateControl(iface);
    return IMFMediaSource_QueryInterface(&source->IMFMediaSource_iface, riid, obj);
}

static ULONG WINAPI media_source_IMFRateControl_AddRef(IMFRateControl *iface)
{
    struct media_source *source = media_source_from_IMFRateControl(iface);
    return IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
}

static ULONG WINAPI media_source_IMFRateControl_Release(IMFRateControl *iface)
{
    struct media_source *source = media_source_from_IMFRateControl(iface);
    return IMFMediaSource_Release(&source->IMFMediaSource_iface);
}

static HRESULT WINAPI media_source_IMFRateControl_SetRate(IMFRateControl *iface, BOOL thin, float rate)
{
    struct media_source *source = media_source_from_IMFRateControl(iface);
    HRESULT hr;

    FIXME("source %p, thin %d, rate %f, stub!\n", source, thin, rate);

    if (rate < 0.0f)
        return MF_E_REVERSE_UNSUPPORTED;
    if (thin)
        return MF_E_THINNING_UNSUPPORTED;

    if (FAILED(hr = IMFRateSupport_IsRateSupported(&source->IMFRateSupport_iface, thin, rate, NULL)))
        return hr;

    EnterCriticalSection(&source->cs);
    source->rate = rate;
    LeaveCriticalSection(&source->cs);

    return IMFMediaEventQueue_QueueEventParamVar(source->queue, MESourceRateChanged, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI media_source_IMFRateControl_GetRate(IMFRateControl *iface, BOOL *thin, float *rate)
{
    struct media_source *source = media_source_from_IMFRateControl(iface);

    TRACE("source %p, thin %p, rate %p\n", source, thin, rate);

    if (thin)
        *thin = FALSE;

    EnterCriticalSection(&source->cs);
    *rate = source->rate;
    LeaveCriticalSection(&source->cs);

    return S_OK;
}

static const IMFRateControlVtbl media_source_IMFRateControl_vtbl =
{
    media_source_IMFRateControl_QueryInterface,
    media_source_IMFRateControl_AddRef,
    media_source_IMFRateControl_Release,
    media_source_IMFRateControl_SetRate,
    media_source_IMFRateControl_GetRate,
};

static HRESULT WINAPI media_source_QueryInterface(IMFMediaSource *iface, REFIID riid, void **out)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);

    TRACE("source %p, riid %s, out %p\n", source, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown)
            || IsEqualIID(riid, &IID_IMFMediaEventGenerator)
            || IsEqualIID(riid, &IID_IMFMediaSource))
    {
        IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
        *out = &source->IMFMediaSource_iface;
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IMFGetService))
    {
        IMFGetService_AddRef(&source->IMFGetService_iface);
        *out = &source->IMFGetService_iface;
        return S_OK;
    }

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_source_AddRef(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    ULONG refcount = InterlockedIncrement(&source->refcount);
    TRACE("source %p, refcount %ld\n", source, refcount);
    return refcount;
}

static ULONG WINAPI media_source_Release(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    ULONG refcount = InterlockedDecrement(&source->refcount);

    TRACE("source %p, refcount %ld\n", source, refcount);

    if (!refcount)
    {
        IMFMediaSource_Shutdown(iface);

        winedmo_demuxer_destroy(&source->winedmo_demuxer);
        free(source->stream_map);
        free(source->streams);

        IMFMediaEventQueue_Release(source->queue);
        IMFByteStream_Release(source->stream);
        free(source->url);

        source->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&source->cs);

        free(source);
    }

    return refcount;
}

static HRESULT WINAPI media_source_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, flags %#lx, event %p\n", source, flags, event);
    return IMFMediaEventQueue_GetEvent(source->queue, flags, event);
}

static HRESULT WINAPI media_source_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, callback %p, state %p\n", source, callback, state);
    return IMFMediaEventQueue_BeginGetEvent(source->queue, callback, state);
}

static HRESULT WINAPI media_source_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result,
        IMFMediaEvent **event)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, result %p, event %p\n", source, result, event);
    return IMFMediaEventQueue_EndGetEvent(source->queue, result, event);
}

static HRESULT WINAPI media_source_QueueEvent(IMFMediaSource *iface, MediaEventType event_type,
        REFGUID ext_type, HRESULT hr, const PROPVARIANT *value)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    TRACE("source %p, event_type %#lx, ext_type %s, hr %#lx, value %p\n", source, event_type,
            debugstr_guid(ext_type), hr, debugstr_propvar(value));
    return IMFMediaEventQueue_QueueEventParamVar(source->queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI media_source_GetCharacteristics(IMFMediaSource *iface, DWORD *characteristics)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    HRESULT hr;

    TRACE("source %p, characteristics %p\n", source, characteristics);

    EnterCriticalSection(&source->cs);
    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        *characteristics = MFMEDIASOURCE_CAN_SEEK | MFMEDIASOURCE_CAN_PAUSE;
        hr = S_OK;
    }
    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT media_source_create_presentation_descriptor(struct media_source *source, IMFPresentationDescriptor **descriptor)
{
    IMFStreamDescriptor **descriptors;
    HRESULT hr;
    UINT i;

    if (!(descriptors = malloc(source->stream_count * sizeof(*descriptors))))
        return E_OUTOFMEMORY;
    for (i = 0; i < source->stream_count; ++i)
        descriptors[i] = source->streams[i]->descriptor;
    hr = MFCreatePresentationDescriptor(source->stream_count, descriptors, descriptor);
    free(descriptors);

    return hr;
}

static HRESULT WINAPI media_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **descriptor)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    HRESULT hr;
    UINT i;

    TRACE("source %p, descriptor %p\n", source, descriptor);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (SUCCEEDED(hr = media_source_create_presentation_descriptor(source, descriptor)))
    {
        if (FAILED(hr = IMFPresentationDescriptor_SetString(*descriptor, &MF_PD_MIME_TYPE, source->mime_type)))
            WARN("Failed to set presentation descriptor MF_PD_MIME_TYPE, hr %#lx\n", hr);
        if (FAILED(hr = IMFPresentationDescriptor_SetUINT64(*descriptor, &MF_PD_TOTAL_FILE_SIZE, source->file_size)))
            WARN("Failed to set presentation descriptor MF_PD_TOTAL_FILE_SIZE, hr %#lx\n", hr);
        if (FAILED(hr = IMFPresentationDescriptor_SetUINT64(*descriptor, &MF_PD_DURATION, source->duration)))
            WARN("Failed to set presentation descriptor MF_PD_DURATION, hr %#lx\n", hr);

        for (i = 0; i < source->stream_count; ++i)
        {
            if (!source->streams[i]->active)
                continue;
            if (FAILED(hr = IMFPresentationDescriptor_SelectStream(*descriptor, i)))
                WARN("Failed to select stream %u, hr %#lx\n", i, hr);
        }

        hr = S_OK;
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Start(IMFMediaSource *iface, IMFPresentationDescriptor *descriptor, const GUID *format,
        const PROPVARIANT *position)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    IUnknown *op;
    HRESULT hr;

    TRACE("source %p, descriptor %p, format %s, position %s\n", source, descriptor,
            debugstr_guid(format), debugstr_propvar(position));

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!IsEqualIID(format, &GUID_NULL))
        hr = MF_E_UNSUPPORTED_TIME_FORMAT;
    else if (SUCCEEDED(hr = async_start_params_create(descriptor, format, position, &op)))
    {
        hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &source->async_start_iface, op);
        IUnknown_Release(op);
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Stop(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    HRESULT hr;

    TRACE("source %p\n", source);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &source->async_stop_iface, NULL);

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Pause(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);
    HRESULT hr;

    TRACE("source %p\n", source);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (source->state != SOURCE_RUNNING)
        hr = MF_E_INVALID_STATE_TRANSITION;
    else
        hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &source->async_pause_iface, NULL);

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Shutdown(IMFMediaSource *iface)
{
    struct media_source *source = media_source_from_IMFMediaSource(iface);

    TRACE("source %p\n", source);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
    {
        LeaveCriticalSection(&source->cs);
        return MF_E_SHUTDOWN;
    }
    source->state = SOURCE_SHUTDOWN;

    IMFMediaEventQueue_Shutdown(source->queue);
    IMFByteStream_Close(source->stream);

    while (source->stream_count--)
    {
        struct media_stream *stream = source->streams[source->stream_count];
        IMFMediaEventQueue_Shutdown(stream->queue);
        IMFMediaStream_Release(&stream->IMFMediaStream_iface);
    }

    LeaveCriticalSection(&source->cs);

    return S_OK;
}

static const IMFMediaSourceVtbl media_source_vtbl =
{
    media_source_QueryInterface,
    media_source_AddRef,
    media_source_Release,
    media_source_GetEvent,
    media_source_BeginGetEvent,
    media_source_EndGetEvent,
    media_source_QueueEvent,
    media_source_GetCharacteristics,
    media_source_CreatePresentationDescriptor,
    media_source_Start,
    media_source_Stop,
    media_source_Pause,
    media_source_Shutdown,
};

static HRESULT media_type_from_mf_video_format( const MFVIDEOFORMAT *format, IMFMediaType **media_type )
{
    HRESULT hr;

    TRACE("format %p, media_type %p\n", format, media_type);

    if (FAILED(hr = MFCreateVideoMediaType( format, (IMFVideoMediaType **)media_type )) ||
        format->dwSize <= sizeof(*format))
        return hr;

    if (FAILED(IMFMediaType_GetItem(*media_type, &MF_MT_VIDEO_ROTATION, NULL)))
        IMFMediaType_SetUINT32(*media_type, &MF_MT_VIDEO_ROTATION, MFVideoRotationFormat_0);

    return hr;
}

static HRESULT media_type_from_winedmo_format( GUID major, union winedmo_format *format, IMFMediaType **media_type )
{
    TRACE("major %p, format %p, media_type %p\n", &major, format, media_type);

    if (IsEqualGUID( &major, &MFMediaType_Video ))
        return media_type_from_mf_video_format( &format->video, media_type );
    if (IsEqualGUID( &major, &MFMediaType_Audio ))
        return MFCreateAudioMediaType( &format->audio, (IMFAudioMediaType **)media_type );

    FIXME( "Unsupported major type %s\n", debugstr_guid( &major ) );
    return E_NOTIMPL;
}

static HRESULT get_stream_media_type(struct winedmo_demuxer demuxer, UINT index, GUID *major, IMFMediaType **media_type)
{
    union winedmo_format *format;
    NTSTATUS status;
    HRESULT hr;

    TRACE("demuxer %p, index %u, media_type %p\n", &demuxer, index, media_type);

    if ((status = winedmo_demuxer_stream_type(demuxer, index, major, &format)))
    {
        WARN("Failed to get stream %u type, status %#lx\n", index, status);
        return HRESULT_FROM_NT(status);
    }

    hr = media_type ? media_type_from_winedmo_format(*major, format, media_type) : S_OK;
    free(format);
    return hr;
}

static void media_source_init_stream_map(struct media_source *source, UINT stream_count)
{
    int i, n = 0;
    GUID major;

    TRACE("source %p, stream_count %d\n", source, stream_count);

    if (wcscmp(source->mime_type, L"video/mp4"))
    {
        for (i = stream_count - 1; i >= 0; i--)
        {
            TRACE("mapping source %p stream %u to demuxer stream %u\n", source, i, i);
            source->stream_map[i] = i;
        }
        return;
    }

    for (i = stream_count - 1; i >= 0; i--)
    {
        if (FAILED(get_stream_media_type(source->winedmo_demuxer, i, &major, NULL)))
            continue;
        if (IsEqualGUID(&major, &MFMediaType_Audio))
        {
            TRACE("mapping source %p stream %u to demuxer stream %u\n", source, n, i);
            source->stream_map[n++] = i;
        }
    }
    for (i = stream_count - 1; i >= 0; i--)
    {
        if (FAILED(get_stream_media_type(source->winedmo_demuxer, i, &major, NULL)))
            continue;
        if (IsEqualGUID(&major, &MFMediaType_Video))
        {
            TRACE("mapping source %p stream %u to demuxer stream %u\n", source, n, i);
            source->stream_map[n++] = i;
        }
    }
    for (i = stream_count - 1; i >= 0; i--)
    {
        if (FAILED(get_stream_media_type(source->winedmo_demuxer, i, &major, NULL)))
            major = GUID_NULL;
        if (!IsEqualGUID(&major, &MFMediaType_Audio) && !IsEqualGUID(&major, &MFMediaType_Video))
        {
            TRACE("mapping source %p stream %u to demuxer stream %u\n", source, n, i);
            source->stream_map[n++] = i;
        }
    }
}

static HRESULT normalize_mp4_language_code(struct media_source *source, WCHAR *buffer)
{
    static const WCHAR mapping[] = L"aar:aa,abk:ab,ave:ae,afr:af,aka:ak,amh:am,arg:an,"
                                    "ara:ar,asm:as,ava:av,aym:ay,aze:az,bak:ba,bel:be,"
                                    "bul:bg,bih:bh,bis:bi,bam:bm,ben:bn,bod:bo,tib:bo,"
                                    "bre:br,bos:bs,cat:ca,che:ce,cha:ch,cos:co,cre:cr,"
                                    "ces:cs,cze:cs,chu:cu,chv:cv,cym:cy,wel:cy,dan:da,"
                                    "deu:de,ger:de,div:dv,dzo:dz,ewe:ee,ell:el,gre:el,"
                                    "eng:en,epo:eo,spa:es,est:et,eus:eu,baq:eu,fas:fa,"
                                    "per:fa,ful:ff,fin:fi,fij:fj,fao:fo,fra:fr,fre:fr,"
                                    "fry:fy,gle:ga,gla:gd,glg:gl,grn:gn,guj:gu,glv:gv,"
                                    "hau:ha,heb:he,hin:hi,hmo:ho,hrv:hr,hat:ht,hun:hu,"
                                    "hye:hy,arm:hy,her:hz,ina:ia,ind:id,ile:ie,ibo:ig,"
                                    "iii:ii,ipk:ik,ido:io,isl:is,ice:is,ita:it,iku:iu,"
                                    "jpn:ja,jav:jv,kat:ka,geo:ka,kon:kg,kik:ki,kua:kj,"
                                    "kaz:kk,kal:kl,khm:km,kan:kn,kor:ko,kau:kr,kas:ks,"
                                    "kur:ku,kom:kv,cor:kw,kir:ky,lat:la,ltz:lb,lug:lg,"
                                    "lim:li,lin:ln,lao:lo,lit:lt,lub:lu,lav:lv,mlg:mg,"
                                    "mah:mh,mri:mi,mao:mi,mkd:mk,mac:mk,mal:ml,mon:mn,"
                                    "mar:mr,msa:ms,may:ms,mlt:mt,mya:my,bur:my,nau:na,"
                                    "nob:nb,nde:nd,nep:ne,ndo:ng,nld:nl,dut:nl,nno:nn,"
                                    "nor:no,nbl:nr,nav:nv,nya:ny,oci:oc,oji:oj,orm:om,"
                                    "ori:or,oss:os,pan:pa,pli:pi,pol:pl,pus:ps,por:pt,"
                                    "que:qu,roh:rm,run:rn,ron:ro,rum:ro,rus:ru,kin:rw,"
                                    "san:sa,srd:sc,snd:sd,sme:se,sag:sg,sin:si,slk:sk,"
                                    "slo:sk,slv:sl,smo:sm,sna:sn,som:so,sqi:sq,alb:sq,"
                                    "srp:sr,ssw:ss,sot:st,sun:su,swe:sv,swa:sw,tam:ta,"
                                    "tel:te,tgk:tg,tha:th,tir:ti,tuk:tk,tgl:tl,tsn:tn,"
                                    "ton:to,tur:tr,tso:ts,tat:tt,twi:tw,tah:ty,uig:ug,"
                                    "ukr:uk,urd:ur,uzb:uz,ven:ve,vie:vi,vol:vo,wln:wa,"
                                    "wol:wo,xho:xh,yid:yi,yor:yo,zha:za,zho:zh,chi:zh,"
                                    "zul:zu";
    const WCHAR *tmp;
    TRACE("source %p, buffer %s\n", source, debugstr_w(buffer));

    if (wcslen(buffer) != 3) return S_OK;
    if ((tmp = wcsstr(mapping, buffer)) && tmp[3] == ':') lstrcpynW(buffer, tmp + 4, 3);
    if (!tmp) return E_UNEXPECTED;
    return S_OK;
}

static void media_source_init_descriptors(struct media_source *source)
{
    UINT i, last_audio = -1, last_video = -1, first_audio = -1, first_video = -1;

    TRACE("source %p\n", source);

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        UINT exclude = -1;
        WCHAR buffer[512];
        NTSTATUS status;
        GUID major;

        if (FAILED(status = winedmo_demuxer_stream_lang(source->winedmo_demuxer, source->stream_map[i], buffer, ARRAY_SIZE(buffer)))
                || (!wcscmp(source->mime_type, L"video/mp4") && FAILED(normalize_mp4_language_code(source, buffer)))
                || FAILED(IMFStreamDescriptor_SetString(stream->descriptor, &MF_SD_LANGUAGE, buffer)))
            WARN("Failed to set stream descriptor language, status %#lx\n", status);
        if (FAILED(status = winedmo_demuxer_stream_name(source->winedmo_demuxer, source->stream_map[i], buffer, ARRAY_SIZE(buffer)))
                || FAILED(IMFStreamDescriptor_SetString(stream->descriptor, &MF_SD_STREAM_NAME, buffer)))
            WARN("Failed to set stream descriptor name, status %#lx\n", status);

        if (FAILED(get_stream_media_type(source->winedmo_demuxer, source->stream_map[i], &major, NULL)))
            continue;

        if (IsEqualGUID(&major, &MFMediaType_Audio))
        {
            if (first_audio == -1)
                first_audio = i;
            exclude = last_audio;
            last_audio = i;
        }
        else if (IsEqualGUID(&major, &MFMediaType_Video))
        {
            if (first_video == -1)
                first_video = i;
            exclude = last_video;
            last_video = i;
        }

        if (exclude != -1)
        {
            if (FAILED(IMFStreamDescriptor_SetUINT32(source->streams[exclude]->descriptor, &MF_SD_MUTUALLY_EXCLUSIVE, 1)))
                WARN("Failed to set stream %u MF_SD_MUTUALLY_EXCLUSIVE\n", exclude);
            else if (FAILED(IMFStreamDescriptor_SetUINT32(stream->descriptor, &MF_SD_MUTUALLY_EXCLUSIVE, 1)))
                WARN("Failed to set stream %u MF_SD_MUTUALLY_EXCLUSIVE\n", i);
        }
    }

    if (!wcscmp(source->mime_type, L"video/mp4"))
    {
        if (last_audio != -1)
            source->streams[last_audio]->active = TRUE;
        if (last_video != -1)
            source->streams[last_video]->active = TRUE;
    }
    else
    {
        if (first_audio != -1)
            source->streams[first_audio]->active = TRUE;
        if (first_video != -1)
            source->streams[first_video]->active = TRUE;
    }
}

static HRESULT stream_descriptor_create(UINT32 id, IMFMediaType *media_type, IMFStreamDescriptor **out)
{
    IMFStreamDescriptor *descriptor;
    IMFMediaTypeHandler *handler;
    HRESULT hr;

    TRACE("id %d, media_type %p, out %p\n", id, media_type, out);

    *out = NULL;
    if (FAILED(hr = MFCreateStreamDescriptor(id, 1, &media_type, &descriptor)))
        return hr;

    if (FAILED(hr = IMFStreamDescriptor_GetMediaTypeHandler(descriptor, &handler)))
        IMFStreamDescriptor_Release(descriptor);
    else
    {
        if (SUCCEEDED(hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, media_type)))
            *out = descriptor;
        IMFMediaTypeHandler_Release(handler);
    }

    return hr;
}

static NTSTATUS CDECL media_source_seek_cb( struct winedmo_stream *stream, UINT64 *pos )
{
    struct media_source *source = CONTAINING_RECORD(stream, struct media_source, winedmo_stream);
    TRACE("stream %p, pos %p\n", stream, pos);

    if (FAILED(IMFByteStream_Seek(source->stream, msoBegin, *pos, 0, pos)))
        return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL media_source_read_cb(struct winedmo_stream *stream, BYTE *buffer, ULONG *size)
{
    struct media_source *source = CONTAINING_RECORD(stream, struct media_source, winedmo_stream);
    TRACE("stream %p, buffer %p, size %p\n", stream, buffer, size);

    if (FAILED(IMFByteStream_Read(source->stream, buffer, *size, size)))
        return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;
}

static HRESULT media_source_async_create(struct media_source *source, IMFAsyncResult *result)
{
    IUnknown *state = IMFAsyncResult_GetStateNoAddRef(result);
    UINT i, stream_count;
    NTSTATUS status;
    HRESULT hr;

    TRACE("source %p, result %p\n", source, result);

    if (FAILED(hr = IMFByteStream_GetLength(source->stream, &source->file_size)))
    {
        WARN("Failed to get byte stream length, hr %#lx\n", hr);
        source->file_size = -1;
    }
    if (FAILED(hr = IMFByteStream_SetCurrentPosition(source->stream, 0)))
    {
        WARN("Failed to set byte stream position, hr %#lx\n", hr);
        hr = S_OK;
    }

    source->winedmo_stream.p_seek = media_source_seek_cb;
    source->winedmo_stream.p_read = media_source_read_cb;

    if ((status = winedmo_demuxer_create(source->url, &source->winedmo_stream, source->file_size, &source->duration,
            &stream_count, source->mime_type, &source->winedmo_demuxer)))
    {
        WARN("Failed to create demuxer, status %#lx\n", status);
        hr = HRESULT_FROM_NT(status);
        goto done;
    }

    if (!(source->stream_map = calloc(stream_count, sizeof(*source->stream_map)))
            || !(source->streams = calloc(stream_count, sizeof(*source->streams))))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    media_source_init_stream_map(source, stream_count);

    for (i = 0; SUCCEEDED(hr) && i < stream_count; ++i)
    {
        IMFStreamDescriptor *descriptor;
        IMFMediaType *media_type;
        GUID major;

        if (FAILED(hr = get_stream_media_type(source->winedmo_demuxer, source->stream_map[i], &major, &media_type)))
            goto done;
        if (SUCCEEDED(hr = stream_descriptor_create(i + 1, media_type, &descriptor)))
        {
            if (SUCCEEDED(hr = media_stream_create(&source->IMFMediaSource_iface, descriptor, &source->streams[i])))
                source->stream_count++;
            IMFStreamDescriptor_Release(descriptor);
        }
        IMFMediaType_Release(media_type);
    }

    media_source_init_descriptors(source);

done:
    IMFAsyncResult_SetStatus(result, hr);
    return MFInvokeCallback((IMFAsyncResult *)state);
}

DEFINE_MF_ASYNC_CALLBACK(media_source, async_create, IMFMediaSource_iface)

static WCHAR *get_byte_stream_url(IMFByteStream *stream, const WCHAR *url)
{
    IMFAttributes *attributes;
    WCHAR buffer[MAX_PATH];
    UINT32 size;
    HRESULT hr;

    TRACE("stream %p, url %s\n", stream, debugstr_w(url));

    if (SUCCEEDED(hr = IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes)))
    {
        if (FAILED(hr = IMFAttributes_GetString(attributes, &MF_BYTESTREAM_ORIGIN_NAME,
                buffer, ARRAY_SIZE(buffer), &size)))
            WARN("Failed to get MF_BYTESTREAM_ORIGIN_NAME got size %#x, hr %#lx\n", size, hr);
        else
            url = buffer;
        IMFAttributes_Release(attributes);
    }

    return url ? wcsdup(url) : NULL;
}

static HRESULT media_source_create(const WCHAR *url, IMFByteStream *stream, IMFMediaSource **out)
{
    struct media_source *source;
    HRESULT hr;

    TRACE("url %s, stream %p, out %p\n", debugstr_w(url), stream, out);

    if (!(source = calloc(1, sizeof(*source))))
        return E_OUTOFMEMORY;
    source->IMFMediaSource_iface.lpVtbl = &media_source_vtbl;
    source->IMFGetService_iface.lpVtbl = &media_source_IMFGetService_vtbl;
    source->IMFRateSupport_iface.lpVtbl = &media_source_IMFRateSupport_vtbl;
    source->IMFRateControl_iface.lpVtbl = &media_source_IMFRateControl_vtbl;
    source->async_create_iface.lpVtbl = &media_source_async_create_vtbl;
    source->async_start_iface.lpVtbl = &media_source_async_start_vtbl;
    source->async_stop_iface.lpVtbl = &media_source_async_stop_vtbl;
    source->async_pause_iface.lpVtbl = &media_source_async_pause_vtbl;
    source->async_read_iface.lpVtbl = &media_source_async_read_vtbl;
    source->refcount = 1;

    if (FAILED(hr = MFCreateEventQueue(&source->queue)))
    {
        free(source);
        return hr;
    }

    source->url = get_byte_stream_url(stream, url);
    IMFByteStream_AddRef((source->stream = stream));

    source->rate = 1.0f;
    InitializeCriticalSectionEx(&source->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    source->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");

    *out = &source->IMFMediaSource_iface;
    TRACE("created source %p\n", source);
    return S_OK;
}

struct byte_stream_handler
{
    IMFByteStreamHandler IMFByteStreamHandler_iface;
    LONG refcount;
};

static struct byte_stream_handler *byte_stream_handler_from_IMFByteStreamHandler(IMFByteStreamHandler *iface)
{
    return CONTAINING_RECORD(iface, struct byte_stream_handler, IMFByteStreamHandler_iface);
}

static HRESULT WINAPI byte_stream_handler_QueryInterface(IMFByteStreamHandler *iface, REFIID riid, void **out)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);

    TRACE("handler %p, riid %s, out %p\n", handler, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown)
            || IsEqualIID(riid, &IID_IMFByteStreamHandler))
    {
        IMFByteStreamHandler_AddRef(&handler->IMFByteStreamHandler_iface);
        *out = &handler->IMFByteStreamHandler_iface;
        return S_OK;
    }

    WARN("Unsupported %s\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI byte_stream_handler_AddRef(IMFByteStreamHandler *iface)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);
    TRACE("handler %p, refcount %ld\n", handler, refcount);
    return refcount;
}

static ULONG WINAPI byte_stream_handler_Release(IMFByteStreamHandler *iface)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);
    TRACE("handler %p, refcount %ld\n", handler, refcount);
    if (!refcount)
        free(handler);
    return refcount;
}

static HRESULT WINAPI byte_stream_handler_BeginCreateObject(IMFByteStreamHandler *iface, IMFByteStream *stream, const WCHAR *url, DWORD flags,
        IPropertyStore *props, IUnknown **cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    IMFAsyncResult *result;
    IMFMediaSource *source;
    HRESULT hr;
    DWORD caps;

    TRACE("handler %p, stream %p, url %s, flags %#lx, props %p, cookie %p, callback %p, state %p\n",
            handler, stream, debugstr_w(url), flags, props, cookie, callback, state);

    if (cookie)
        *cookie = NULL;
    if (!stream)
        return E_INVALIDARG;
    if (flags != MF_RESOLUTION_MEDIASOURCE)
        FIXME("Unimplemented flags %#lx\n", flags);

    if (FAILED(hr = IMFByteStream_GetCapabilities(stream, &caps)))
        return hr;
    if (!(caps & MFBYTESTREAM_IS_SEEKABLE))
    {
        FIXME("Non-seekable bytestreams not supported\n");
        return MF_E_BYTESTREAM_NOT_SEEKABLE;
    }

    if (FAILED(hr = media_source_create(url, stream, &source)))
        return hr;
    if (SUCCEEDED(hr = MFCreateAsyncResult((IUnknown *)source, callback, state, &result)))
    {
        struct media_source *media_source = media_source_from_IMFMediaSource(source);
        hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_IO, &media_source->async_create_iface, (IUnknown *)result);
        IMFAsyncResult_Release(result);
    }
    IMFMediaSource_Release(source);

    return hr;
}

static HRESULT WINAPI byte_stream_handler_EndCreateObject(IMFByteStreamHandler *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *type, IUnknown **object)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    HRESULT hr;

    TRACE("handler %p, result %p, type %p, object %p\n", handler, result, type, object);

    *object = NULL;
    *type = MF_OBJECT_INVALID;

    if (SUCCEEDED(hr = IMFAsyncResult_GetStatus(result)))
    {
        hr = IMFAsyncResult_GetObject(result, object);
        *type = MF_OBJECT_MEDIASOURCE;
    }

    return hr;
}

static HRESULT WINAPI byte_stream_handler_CancelObjectCreation(IMFByteStreamHandler *iface, IUnknown *cookie)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    FIXME("handler %p, cookie %p, stub!\n", handler, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI byte_stream_handler_GetMaxNumberOfBytesRequiredForResolution(IMFByteStreamHandler *iface, QWORD *bytes)
{
    struct byte_stream_handler *handler = byte_stream_handler_from_IMFByteStreamHandler(iface);
    FIXME("handler %p, bytes %p, stub!\n", handler, bytes);
    return E_NOTIMPL;
}

static const IMFByteStreamHandlerVtbl byte_stream_handler_vtbl =
{
    byte_stream_handler_QueryInterface,
    byte_stream_handler_AddRef,
    byte_stream_handler_Release,
    byte_stream_handler_BeginCreateObject,
    byte_stream_handler_EndCreateObject,
    byte_stream_handler_CancelObjectCreation,
    byte_stream_handler_GetMaxNumberOfBytesRequiredForResolution,
};

static HRESULT byte_stream_plugin_create(IUnknown *outer, REFIID riid, void **out)
{
    struct byte_stream_handler *handler;
    HRESULT hr;

    TRACE("outer %p, riid %s, out %p\n", outer, debugstr_guid(riid), out);

    if (outer)
        return CLASS_E_NOAGGREGATION;
    if (!(handler = calloc(1, sizeof(*handler))))
        return E_OUTOFMEMORY;
    handler->IMFByteStreamHandler_iface.lpVtbl = &byte_stream_handler_vtbl;
    handler->refcount = 1;
    TRACE("created %p\n", handler);

    hr = IMFByteStreamHandler_QueryInterface(&handler->IMFByteStreamHandler_iface, riid, out);
    IMFByteStreamHandler_Release(&handler->IMFByteStreamHandler_iface);
    return hr;
}

static BOOL use_gst_byte_stream_handler(void)
{
    BOOL result;
    DWORD size = sizeof(result);

    /* @@ Wine registry key: HKCU\Software\Wine\MediaFoundation */
    if (!RegGetValueW( HKEY_CURRENT_USER, L"Software\\Wine\\MediaFoundation", L"DisableGstByteStreamHandler",
                       RRF_RT_REG_DWORD, NULL, &result, &size ))
        return !result;

    return TRUE;
}

static HRESULT WINAPI asf_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/x-ms-asf")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl asf_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    asf_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory asf_byte_stream_plugin_factory = {&asf_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI avi_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/avi")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl avi_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    avi_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory avi_byte_stream_plugin_factory = {&avi_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI mpeg4_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("video/mp4")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl mpeg4_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    mpeg4_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory mpeg4_byte_stream_plugin_factory = {&mpeg4_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI wav_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("audio/wav")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl wav_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    wav_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory wav_byte_stream_plugin_factory = {&wav_byte_stream_plugin_factory_vtbl};

static HRESULT WINAPI mp3_byte_stream_plugin_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    NTSTATUS status;

    if ((status = winedmo_demuxer_check("audio/mp3")) || use_gst_byte_stream_handler())
    {
        static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618,0x5e5a,0x468a,{0x9f,0x15,0xd8,0x27,0xa9,0xa0,0x81,0x62}};
        if (status) WARN("Unsupported demuxer, status %#lx.\n", status);
        return CoCreateInstance(&CLSID_GStreamerByteStreamHandler, outer, CLSCTX_INPROC_SERVER, riid, out);
    }

    return byte_stream_plugin_create(outer, riid, out);
}

static const IClassFactoryVtbl mp3_byte_stream_plugin_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    mp3_byte_stream_plugin_factory_CreateInstance,
    class_factory_LockServer,
};

IClassFactory mp3_byte_stream_plugin_factory = {&mp3_byte_stream_plugin_factory_vtbl};
