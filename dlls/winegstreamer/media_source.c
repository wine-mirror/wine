/* GStreamer Media Source
 *
 * Copyright 2020 Derek Lesho
 * Copyright 2020 Zebediah Figura for CodeWeavers
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

#include "mfapi.h"
#include "mferror.h"

#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct object_context
{
    IUnknown IUnknown_iface;
    LONG refcount;

    IMFAsyncResult *result;
    IMFByteStream *stream;
    UINT64 file_size;
    WCHAR *url;
};

static struct object_context *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct object_context, IUnknown_iface);
}

static HRESULT WINAPI object_context_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI object_context_AddRef(IUnknown *iface)
{
    struct object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&context->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI object_context_Release(IUnknown *iface)
{
    struct object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&context->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IMFAsyncResult_Release(context->result);
        IMFByteStream_Release(context->stream);
        free(context->url);
        free(context);
    }

    return refcount;
}

static const IUnknownVtbl object_context_vtbl =
{
    object_context_QueryInterface,
    object_context_AddRef,
    object_context_Release,
};

static HRESULT object_context_create(DWORD flags, IMFByteStream *stream, const WCHAR *url,
        QWORD file_size, IMFAsyncResult *result, IUnknown **out)
{
    WCHAR *tmp_url = url ? wcsdup(url) : NULL;
    struct object_context *context;

    if (!(context = calloc(1, sizeof(*context))))
    {
        free(tmp_url);
        return E_OUTOFMEMORY;
    }

    context->IUnknown_iface.lpVtbl = &object_context_vtbl;
    context->refcount = 1;
    context->stream = stream;
    IMFByteStream_AddRef(context->stream);
    context->file_size = file_size;
    context->url = tmp_url;
    context->result = result;
    IMFAsyncResult_AddRef(context->result);

    *out = &context->IUnknown_iface;
    return S_OK;
}

struct media_stream
{
    IMFMediaStream IMFMediaStream_iface;
    LONG ref;

    IMFMediaSource *media_source;
    IMFMediaEventQueue *event_queue;
    IMFStreamDescriptor *descriptor;

    wg_parser_stream_t wg_stream;

    IUnknown **token_queue;
    LONG token_queue_count;
    LONG token_queue_cap;

    DWORD stream_id;
    BOOL active;
    BOOL eos;
};

enum source_async_op
{
    SOURCE_ASYNC_START,
    SOURCE_ASYNC_PAUSE,
    SOURCE_ASYNC_STOP,
    SOURCE_ASYNC_REQUEST_SAMPLE,
};

struct source_async_command
{
    IUnknown IUnknown_iface;
    LONG refcount;
    enum source_async_op op;
    union
    {
        struct
        {
            IMFPresentationDescriptor *descriptor;
            GUID format;
            PROPVARIANT position;
        } start;
        struct
        {
            struct media_stream *stream;
            IUnknown *token;
        } request_sample;
    } u;
};

struct media_source
{
    IMFMediaSource IMFMediaSource_iface;
    IMFGetService IMFGetService_iface;
    IMFRateSupport IMFRateSupport_iface;
    IMFRateControl IMFRateControl_iface;
    IMFAsyncCallback async_commands_callback;
    LONG ref;
    DWORD async_commands_queue;
    IMFMediaEventQueue *event_queue;
    IMFByteStream *byte_stream;

    CRITICAL_SECTION cs;

    UINT64 file_size;
    wg_parser_t wg_parser;
    UINT64 duration;

    IMFStreamDescriptor **descriptors;
    struct media_stream **streams;
    ULONG stream_count;

    enum
    {
        SOURCE_OPENING,
        SOURCE_STOPPED,
        SOURCE_PAUSED,
        SOURCE_RUNNING,
        SOURCE_SHUTDOWN,
    } state;
    float rate;

    HANDLE read_thread;
    bool read_thread_shutdown;
};

static inline struct media_stream *impl_from_IMFMediaStream(IMFMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct media_stream, IMFMediaStream_iface);
}

static inline struct media_source *impl_from_IMFMediaSource(IMFMediaSource *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFMediaSource_iface);
}

static inline struct media_source *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFGetService_iface);
}

static inline struct media_source *impl_from_IMFRateSupport(IMFRateSupport *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFRateSupport_iface);
}

static inline struct media_source *impl_from_IMFRateControl(IMFRateControl *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFRateControl_iface);
}

static inline struct media_source *impl_from_async_commands_callback_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, async_commands_callback);
}

static inline struct source_async_command *impl_from_async_command_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct source_async_command, IUnknown_iface);
}

static HRESULT WINAPI source_async_command_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
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

static ULONG WINAPI source_async_command_AddRef(IUnknown *iface)
{
    struct source_async_command *command = impl_from_async_command_IUnknown(iface);
    return InterlockedIncrement(&command->refcount);
}

static ULONG WINAPI source_async_command_Release(IUnknown *iface)
{
    struct source_async_command *command = impl_from_async_command_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&command->refcount);

    if (!refcount)
    {
        if (command->op == SOURCE_ASYNC_START)
        {
            IMFPresentationDescriptor_Release(command->u.start.descriptor);
            PropVariantClear(&command->u.start.position);
        }
        else if (command->op == SOURCE_ASYNC_REQUEST_SAMPLE)
        {
            if (command->u.request_sample.token)
                IUnknown_Release(command->u.request_sample.token);
        }
        free(command);
    }

    return refcount;
}

static const IUnknownVtbl source_async_command_vtbl =
{
    source_async_command_QueryInterface,
    source_async_command_AddRef,
    source_async_command_Release,
};

static HRESULT source_create_async_op(enum source_async_op op, IUnknown **out)
{
    struct source_async_command *command;

    if (!(command = calloc(1, sizeof(*command))))
        return E_OUTOFMEMORY;

    command->IUnknown_iface.lpVtbl = &source_async_command_vtbl;
    command->refcount = 1;
    command->op = op;

    *out = &command->IUnknown_iface;
    return S_OK;
}

static HRESULT WINAPI callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
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

static HRESULT WINAPI callback_GetParameters(IMFAsyncCallback *iface,
        DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static ULONG WINAPI source_async_commands_callback_AddRef(IMFAsyncCallback *iface)
{
    struct media_source *source = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    return IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
}

static ULONG WINAPI source_async_commands_callback_Release(IMFAsyncCallback *iface)
{
    struct media_source *source = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    return IMFMediaSource_Release(&source->IMFMediaSource_iface);
}

static HRESULT stream_descriptor_get_media_type(IMFStreamDescriptor *descriptor, IMFMediaType **media_type)
{
    IMFMediaTypeHandler *handler;
    HRESULT hr;

    if (FAILED(hr = IMFStreamDescriptor_GetMediaTypeHandler(descriptor, &handler)))
        return hr;
    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, media_type);
    IMFMediaTypeHandler_Release(handler);

    return hr;
}

static HRESULT wg_format_from_stream_descriptor(IMFStreamDescriptor *descriptor, struct wg_format *format)
{
    IMFMediaType *media_type;
    HRESULT hr;

    if (FAILED(hr = stream_descriptor_get_media_type(descriptor, &media_type)))
        return hr;
    mf_media_type_to_wg_format(media_type, format);
    IMFMediaType_Release(media_type);

    return hr;
}

static HRESULT stream_descriptor_set_tag(IMFStreamDescriptor *descriptor, wg_parser_stream_t stream,
    const GUID *attr, enum wg_parser_tag tag)
{
    WCHAR *strW;
    HRESULT hr;
    DWORD len;
    char *str;

    if (!(str = wg_parser_stream_get_tag(stream, tag))
            || !(len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0)))
        hr = S_OK;
    else if (!(strW = malloc(len * sizeof(*strW))))
        hr = E_OUTOFMEMORY;
    else
    {
        if (MultiByteToWideChar(CP_UTF8, 0, str, -1, strW, len))
            hr = IMFStreamDescriptor_SetString(descriptor, attr, strW);
        else
            hr = E_FAIL;
        free(strW);
    }

    free(str);
    return hr;
}

static HRESULT init_video_media_types(struct wg_format *format, IMFMediaType *types[6], DWORD *types_count)
{
    /* Try to prefer YUV formats over RGB ones. Most decoders output in the
     * YUV color space, and it's generally much less expensive for
     * videoconvert to do YUV -> YUV transformations. */
    static const enum wg_video_format video_formats[] =
    {
        WG_VIDEO_FORMAT_NV12,
        WG_VIDEO_FORMAT_YV12,
        WG_VIDEO_FORMAT_YUY2,
        WG_VIDEO_FORMAT_I420,
    };
    UINT count = *types_count, i;
    GUID base_subtype;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetGUID(types[0], &MF_MT_SUBTYPE, &base_subtype)))
        return hr;

    for (i = 0; i < ARRAY_SIZE(video_formats); ++i)
    {
        struct wg_format new_format = *format;
        IMFMediaType *new_type;

        new_format.u.video.format = video_formats[i];

        if (!(new_type = mf_media_type_from_wg_format(&new_format)))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        types[count++] = new_type;

        if (video_formats[i] == WG_VIDEO_FORMAT_I420)
        {
            IMFMediaType *iyuv_type;

            if (FAILED(hr = MFCreateMediaType(&iyuv_type)))
                goto done;
            if (FAILED(hr = IMFMediaType_CopyAllItems(new_type, (IMFAttributes *)iyuv_type)))
                goto done;
            if (FAILED(hr = IMFMediaType_SetGUID(iyuv_type, &MF_MT_SUBTYPE, &MFVideoFormat_IYUV)))
                goto done;
            types[count++] = iyuv_type;
        }
    }

done:
    *types_count = count;
    return hr;
}

static HRESULT init_audio_media_types(struct wg_format *format, IMFMediaType *types[6], DWORD *types_count)
{
    /* Expose at least one PCM and one floating point type for the
       consumer to pick from. */
    static const enum wg_audio_format audio_types[] =
    {
        WG_AUDIO_FORMAT_S16LE,
        WG_AUDIO_FORMAT_F32LE,
    };
    UINT count = *types_count, i;

    for (i = 0; i < ARRAY_SIZE(audio_types); i++)
    {
        struct wg_format new_format = *format;
        if (new_format.u.audio.format == audio_types[i])
            continue;
        new_format.u.audio.format = audio_types[i];
        if ((types[count] = mf_media_type_from_wg_format(&new_format)))
            count++;
    }

    *types_count = count;
    return S_OK;
}

static HRESULT stream_descriptor_create(UINT32 id, struct wg_format *format, IMFStreamDescriptor **out)
{
    IMFStreamDescriptor *descriptor;
    IMFMediaTypeHandler *handler;
    IMFMediaType *types[6];
    DWORD count = 0;
    HRESULT hr;

    if (!(types[0] = mf_media_type_from_wg_format(format)))
        return MF_E_INVALIDMEDIATYPE;
    count = 1;

    if (format->major_type == WG_MAJOR_TYPE_VIDEO)
    {
        if (FAILED(hr = init_video_media_types(format, types, &count)))
            goto done;
    }
    else if (format->major_type == WG_MAJOR_TYPE_AUDIO)
    {
        if (FAILED(hr = init_audio_media_types(format, types, &count)))
            goto done;
    }

    assert(count <= ARRAY_SIZE(types));

    if (FAILED(hr = MFCreateStreamDescriptor(id, count, types, &descriptor)))
        goto done;

    if (FAILED(hr = IMFStreamDescriptor_GetMediaTypeHandler(descriptor, &handler)))
        IMFStreamDescriptor_Release(descriptor);
    else
    {
        hr = IMFMediaTypeHandler_SetCurrentMediaType(handler, types[0]);
        IMFMediaTypeHandler_Release(handler);
    }

done:
    while (count--)
        IMFMediaType_Release(types[count]);
    *out = SUCCEEDED(hr) ? descriptor : NULL;
    return hr;
}

static BOOL enqueue_token(struct media_stream *stream, IUnknown *token)
{
    if (stream->token_queue_count == stream->token_queue_cap)
    {
        IUnknown **buf;
        stream->token_queue_cap = stream->token_queue_cap * 2 + 1;
        buf = realloc(stream->token_queue, stream->token_queue_cap * sizeof(*buf));
        if (buf)
            stream->token_queue = buf;
        else
        {
            stream->token_queue_cap = stream->token_queue_count;
            return FALSE;
        }
    }
    stream->token_queue[stream->token_queue_count++] = token;
    return TRUE;
}

static void flush_token_queue(struct media_stream *stream, BOOL send)
{
    struct media_source *source = impl_from_IMFMediaSource(stream->media_source);
    LONG i;

    for (i = 0; i < stream->token_queue_count; i++)
    {
        if (send)
        {
            IUnknown *op;
            HRESULT hr;

            if (SUCCEEDED(hr = source_create_async_op(SOURCE_ASYNC_REQUEST_SAMPLE, &op)))
            {
                struct source_async_command *command = impl_from_async_command_IUnknown(op);
                command->u.request_sample.stream = stream;
                command->u.request_sample.token = stream->token_queue[i];

                hr = MFPutWorkItem(source->async_commands_queue, &source->async_commands_callback, op);
                IUnknown_Release(op);
            }
            if (FAILED(hr))
                WARN("Could not enqueue sample request, hr %#lx\n", hr);
        }
        else if (stream->token_queue[i])
            IUnknown_Release(stream->token_queue[i]);
    }
    free(stream->token_queue);
    stream->token_queue = NULL;
    stream->token_queue_count = 0;
    stream->token_queue_cap = 0;
}

static HRESULT media_stream_start(struct media_stream *stream, BOOL active, BOOL seeking, const PROPVARIANT *position)
{
    struct media_source *source = impl_from_IMFMediaSource(stream->media_source);
    struct wg_format format;
    HRESULT hr;

    TRACE("source %p, stream %p\n", source, stream);

    if (FAILED(hr = wg_format_from_stream_descriptor(stream->descriptor, &format)))
        WARN("Failed to get wg_format from stream descriptor, hr %#lx\n", hr);
    wg_parser_stream_enable(stream->wg_stream, &format);

    if (FAILED(hr = IMFMediaEventQueue_QueueEventParamUnk(source->event_queue, active ? MEUpdatedStream : MENewStream,
            &GUID_NULL, S_OK, (IUnknown *)&stream->IMFMediaStream_iface)))
        WARN("Failed to send source stream event, hr %#lx\n", hr);
    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, seeking ? MEStreamSeeked : MEStreamStarted,
            &GUID_NULL, S_OK, position);
}

static HRESULT media_source_start(struct media_source *source, IMFPresentationDescriptor *descriptor,
        GUID *format, PROPVARIANT *position)
{
    BOOL starting = source->state == SOURCE_STOPPED, seek_message = !starting && position->vt != VT_EMPTY;
    IMFStreamDescriptor **descriptors;
    DWORD i, count;
    HRESULT hr;

    TRACE("source %p, descriptor %p, format %s, position %s\n", source, descriptor,
            debugstr_guid(format), wine_dbgstr_variant((VARIANT *)position));

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    /* seek to beginning on stop->play */
    if (source->state == SOURCE_STOPPED && position->vt == VT_EMPTY)
    {
        position->vt = VT_I8;
        position->hVal.QuadPart = 0;
    }

    if (!(descriptors = calloc(source->stream_count, sizeof(*descriptors))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(descriptor, &count)))
        WARN("Failed to get presentation descriptor stream count, hr %#lx\n", hr);

    for (i = 0; i < count; i++)
    {
        IMFStreamDescriptor *stream_descriptor;
        BOOL selected;
        DWORD id;

        if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(descriptor, i,
                &selected, &stream_descriptor)))
            WARN("Failed to get presentation stream descriptor, hr %#lx\n", hr);
        else
        {
            if (FAILED(hr = IMFStreamDescriptor_GetStreamIdentifier(stream_descriptor, &id)))
                WARN("Failed to get stream descriptor id, hr %#lx\n", hr);
            else if (id >= source->stream_count)
                WARN("Invalid stream descriptor id %lu, hr %#lx\n", id, hr);
            else if (selected)
                IMFStreamDescriptor_AddRef((descriptors[id] = stream_descriptor));

            IMFStreamDescriptor_Release(stream_descriptor);
        }
    }

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        BOOL was_active = !starting && stream->active;

        if (position->vt != VT_EMPTY)
            stream->eos = FALSE;

        if (!(stream->active = !!descriptors[i]))
            wg_parser_stream_disable(stream->wg_stream);
        else
        {
            if (FAILED(hr = media_stream_start(stream, was_active, seek_message, position)))
                WARN("Failed to start media stream, hr %#lx\n", hr);
            IMFStreamDescriptor_Release(descriptors[i]);
        }
    }

    free(descriptors);

    source->state = SOURCE_RUNNING;

    if (position->vt == VT_I8)
        wg_parser_stream_seek(source->streams[0]->wg_stream, 1.0, position->hVal.QuadPart, 0,
                AM_SEEKING_AbsolutePositioning, AM_SEEKING_NoPositioning);

    for (i = 0; i < source->stream_count; i++)
        flush_token_queue(source->streams[i], position->vt == VT_EMPTY);

    return IMFMediaEventQueue_QueueEventParamVar(source->event_queue,
            seek_message ? MESourceSeeked : MESourceStarted, &GUID_NULL, S_OK, position);
}

static HRESULT media_source_pause(struct media_source *source)
{
    unsigned int i;
    HRESULT hr;

    TRACE("source %p\n", source);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        if (stream->active && FAILED(hr = IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, MEStreamPaused,
                    &GUID_NULL, S_OK, NULL)))
            WARN("Failed to queue MEStreamPaused event, hr %#lx\n", hr);
    }

    source->state = SOURCE_PAUSED;
    return IMFMediaEventQueue_QueueEventParamVar(source->event_queue, MESourcePaused, &GUID_NULL, S_OK, NULL);
}

static HRESULT media_source_stop(struct media_source *source)
{
    unsigned int i;
    HRESULT hr;

    TRACE("source %p\n", source);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        if (stream->active && FAILED(hr = IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, MEStreamStopped,
                    &GUID_NULL, S_OK, NULL)))
            WARN("Failed to queue MEStreamStopped event, hr %#lx\n", hr);
    }

    source->state = SOURCE_STOPPED;

    for (i = 0; i < source->stream_count; i++)
        flush_token_queue(source->streams[i], FALSE);

    return IMFMediaEventQueue_QueueEventParamVar(source->event_queue, MESourceStopped, &GUID_NULL, S_OK, NULL);
}

static HRESULT media_stream_send_sample(struct media_stream *stream, const struct wg_parser_buffer *wg_buffer, IUnknown *token)
{
    IMFSample *sample = NULL;
    IMFMediaBuffer *buffer;
    HRESULT hr;
    BYTE *data;

    if (FAILED(hr = MFCreateMemoryBuffer(wg_buffer->size, &buffer)))
        return hr;
    if (FAILED(hr = IMFMediaBuffer_SetCurrentLength(buffer, wg_buffer->size)))
        goto out;
    if (FAILED(hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL)))
        goto out;

    if (!wg_parser_stream_copy_buffer(stream->wg_stream, data, 0, wg_buffer->size))
    {
        hr = S_FALSE;
        wg_parser_stream_release_buffer(stream->wg_stream);
        IMFMediaBuffer_Unlock(buffer);
        goto out;
    }
    wg_parser_stream_release_buffer(stream->wg_stream);

    if (FAILED(hr = IMFMediaBuffer_Unlock(buffer)))
        goto out;

    if (FAILED(hr = MFCreateSample(&sample)))
        goto out;
    if (FAILED(hr = IMFSample_AddBuffer(sample, buffer)))
        goto out;
    if (FAILED(hr = IMFSample_SetSampleTime(sample, wg_buffer->pts)))
        goto out;
    if (FAILED(hr = IMFSample_SetSampleDuration(sample, wg_buffer->duration)))
        goto out;
    if (token && FAILED(hr = IMFSample_SetUnknown(sample, &MFSampleExtension_Token, token)))
        goto out;

    hr = IMFMediaEventQueue_QueueEventParamUnk(stream->event_queue, MEMediaSample,
            &GUID_NULL, S_OK, (IUnknown *)sample);

out:
    if (sample)
        IMFSample_Release(sample);
    IMFMediaBuffer_Release(buffer);
    return hr;
}

static HRESULT media_stream_send_eos(struct media_source *source, struct media_stream *stream)
{
    PROPVARIANT empty = {.vt = VT_EMPTY};
    HRESULT hr;
    UINT i;

    TRACE("source %p, stream %p\n", source, stream);

    stream->eos = TRUE;
    if (FAILED(hr = IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, MEEndOfStream, &GUID_NULL, S_OK, &empty)))
        WARN("Failed to queue MEEndOfStream event, hr %#lx\n", hr);

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        if (stream->active && !stream->eos)
            return S_OK;
    }

    if (FAILED(hr = IMFMediaEventQueue_QueueEventParamVar(source->event_queue, MEEndOfPresentation, &GUID_NULL, S_OK, &empty)))
        WARN("Failed to queue MEEndOfPresentation event, hr %#lx\n", hr);
    return S_OK;
}

static bool stream_get_buffer(struct media_stream *stream, struct wg_parser_buffer *buffer)
{
    struct media_source *source = impl_from_IMFMediaSource(stream->media_source);
    wg_parser_stream_t wg_stream = stream->wg_stream;
    wg_parser_t wg_parser = source->wg_parser;
    bool ret;

    LeaveCriticalSection(&source->cs);
    ret = wg_parser_stream_get_buffer(wg_parser, wg_stream, buffer);
    EnterCriticalSection(&source->cs);

    return ret;
}

static HRESULT wait_on_sample(struct media_stream *stream, IUnknown *token)
{
    struct media_source *source = impl_from_IMFMediaSource(stream->media_source);
    struct wg_parser_buffer buffer;

    TRACE("%p, %p\n", stream, token);

    while (stream_get_buffer(stream, &buffer))
    {
        HRESULT hr = media_stream_send_sample(stream, &buffer, token);
        if (hr != S_FALSE)
            return hr;
    }

    if (source->state == SOURCE_SHUTDOWN)
        return S_OK;

    return media_stream_send_eos(source, stream);
}

static HRESULT WINAPI source_async_commands_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_source *source = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    struct source_async_command *command;
    IUnknown *state;
    HRESULT hr;

    if (FAILED(hr = IMFAsyncResult_GetState(result, &state)))
        return hr;

    EnterCriticalSection(&source->cs);

    command = impl_from_async_command_IUnknown(state);
    switch (command->op)
    {
        case SOURCE_ASYNC_START:
        {
            IMFPresentationDescriptor *descriptor = command->u.start.descriptor;
            GUID format = command->u.start.format;
            PROPVARIANT position = command->u.start.position;

            if (FAILED(hr = media_source_start(source, descriptor, &format, &position)))
                WARN("Failed to start source %p, hr %#lx\n", source, hr);
            break;
        }
        case SOURCE_ASYNC_PAUSE:
            if (FAILED(hr = media_source_pause(source)))
                WARN("Failed to pause source %p, hr %#lx\n", source, hr);
            break;
        case SOURCE_ASYNC_STOP:
            if (FAILED(hr = media_source_stop(source)))
                WARN("Failed to stop source %p, hr %#lx\n", source, hr);
            break;
        case SOURCE_ASYNC_REQUEST_SAMPLE:
            if (source->state == SOURCE_PAUSED)
                enqueue_token(command->u.request_sample.stream, command->u.request_sample.token);
            else if (source->state == SOURCE_RUNNING)
            {
                if (FAILED(hr = wait_on_sample(command->u.request_sample.stream, command->u.request_sample.token)))
                    WARN("Failed to request sample, hr %#lx\n", hr);
            }
            break;
    }

    LeaveCriticalSection(&source->cs);

    IUnknown_Release(state);

    return S_OK;
}

static const IMFAsyncCallbackVtbl source_async_commands_callback_vtbl =
{
    callback_QueryInterface,
    source_async_commands_callback_AddRef,
    source_async_commands_callback_Release,
    callback_GetParameters,
    source_async_commands_Invoke,
};

static DWORD CALLBACK read_thread(void *arg)
{
    struct media_source *source = arg;
    IMFByteStream *byte_stream = source->byte_stream;
    size_t buffer_size = 4096;
    QWORD file_size;
    void *data;

    if (!(data = malloc(buffer_size)))
        return 0;

    IMFByteStream_GetLength(byte_stream, &file_size);

    TRACE("Starting read thread for media source %p.\n", source);

    while (!source->read_thread_shutdown)
    {
        uint64_t offset;
        ULONG ret_size;
        uint32_t size;
        HRESULT hr;

        if (!wg_parser_get_next_read_offset(source->wg_parser, &offset, &size))
            continue;

        if (offset >= file_size)
            size = 0;
        else if (offset + size >= file_size)
            size = file_size - offset;

        /* Some IMFByteStreams (including the standard file-based stream) return
         * an error when reading past the file size. */
        if (!size)
        {
            wg_parser_push_data(source->wg_parser, data, 0);
            continue;
        }

        if (!array_reserve(&data, &buffer_size, size, 1))
        {
            free(data);
            return 0;
        }

        ret_size = 0;

        if (SUCCEEDED(hr = IMFByteStream_SetCurrentPosition(byte_stream, offset)))
            hr = IMFByteStream_Read(byte_stream, data, size, &ret_size);
        if (FAILED(hr))
            ERR("Failed to read %u bytes at offset %I64u, hr %#lx.\n", size, offset, hr);
        else if (ret_size != size)
            ERR("Unexpected short read: requested %u bytes, got %lu.\n", size, ret_size);
        wg_parser_push_data(source->wg_parser, SUCCEEDED(hr) ? data : NULL, ret_size);
    }

    free(data);
    TRACE("Media source is shutting down; exiting.\n");
    return 0;
}

static HRESULT WINAPI media_stream_QueryInterface(IMFMediaStream *iface, REFIID riid, void **out)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaStream) ||
        IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &stream->IMFMediaStream_iface;
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

static ULONG WINAPI media_stream_AddRef(IMFMediaStream *iface)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);
    ULONG ref = InterlockedIncrement(&stream->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI media_stream_Release(IMFMediaStream *iface)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);
    ULONG ref = InterlockedDecrement(&stream->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        IMFMediaSource_Release(stream->media_source);
        IMFStreamDescriptor_Release(stream->descriptor);
        IMFMediaEventQueue_Release(stream->event_queue);
        flush_token_queue(stream, FALSE);
        free(stream);
    }

    return ref;
}

static HRESULT WINAPI media_stream_GetEvent(IMFMediaStream *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(stream->event_queue, flags, event);
}

static HRESULT WINAPI media_stream_BeginGetEvent(IMFMediaStream *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(stream->event_queue, callback, state);
}

static HRESULT WINAPI media_stream_EndGetEvent(IMFMediaStream *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("%p, %p, %p.\n", stream, result, event);

    return IMFMediaEventQueue_EndGetEvent(stream->event_queue, result, event);
}

static HRESULT WINAPI media_stream_QueueEvent(IMFMediaStream *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI media_stream_GetMediaSource(IMFMediaStream *iface, IMFMediaSource **out)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);
    struct media_source *source = impl_from_IMFMediaSource(stream->media_source);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, out);

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
    struct media_stream *stream = impl_from_IMFMediaStream(iface);
    struct media_source *source = impl_from_IMFMediaSource(stream->media_source);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, descriptor);

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

static HRESULT WINAPI media_stream_RequestSample(IMFMediaStream *iface, IUnknown *token)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);
    struct media_source *source = impl_from_IMFMediaSource(stream->media_source);
    IUnknown *op;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, token);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!stream->active)
        hr = MF_E_MEDIA_SOURCE_WRONGSTATE;
    else if (stream->eos)
        hr = MF_E_END_OF_STREAM;
    else if (SUCCEEDED(hr = source_create_async_op(SOURCE_ASYNC_REQUEST_SAMPLE, &op)))
    {
        struct source_async_command *command = impl_from_async_command_IUnknown(op);
        command->u.request_sample.stream = stream;
        if (token)
            IUnknown_AddRef(token);
        command->u.request_sample.token = token;

        hr = MFPutWorkItem(source->async_commands_queue, &source->async_commands_callback, op);
        IUnknown_Release(op);
    }

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
    media_stream_RequestSample
};

static HRESULT media_stream_create(IMFMediaSource *source, IMFStreamDescriptor *descriptor,
        wg_parser_stream_t wg_stream, struct media_stream **out)
{
    struct media_stream *object;
    HRESULT hr;

    TRACE("source %p, descriptor %p, wg_stream %#I64x.\n", source, descriptor, wg_stream);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaStream_iface.lpVtbl = &media_stream_vtbl;
    object->ref = 1;

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
    {
        free(object);
        return hr;
    }

    IMFMediaSource_AddRef(source);
    object->media_source = source;
    IMFStreamDescriptor_AddRef(descriptor);
    object->descriptor = descriptor;

    object->active = TRUE;
    object->eos = FALSE;
    object->wg_stream = wg_stream;

    TRACE("Created stream object %p.\n", object);

    *out = object;
    return S_OK;
}

static HRESULT WINAPI media_source_get_service_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct media_source *source = impl_from_IMFGetService(iface);
    return IMFMediaSource_QueryInterface(&source->IMFMediaSource_iface, riid, obj);
}

static ULONG WINAPI media_source_get_service_AddRef(IMFGetService *iface)
{
    struct media_source *source = impl_from_IMFGetService(iface);
    return IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
}

static ULONG WINAPI media_source_get_service_Release(IMFGetService *iface)
{
    struct media_source *source = impl_from_IMFGetService(iface);
    return IMFMediaSource_Release(&source->IMFMediaSource_iface);
}

static HRESULT WINAPI media_source_get_service_GetService(IMFGetService *iface, REFGUID service, REFIID riid, void **obj)
{
    struct media_source *source = impl_from_IMFGetService(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(service, &MF_RATE_CONTROL_SERVICE))
    {
        if (IsEqualIID(riid, &IID_IMFRateSupport))
        {
            *obj = &source->IMFRateSupport_iface;
        }
        else if (IsEqualIID(riid, &IID_IMFRateControl))
        {
            *obj = &source->IMFRateControl_iface;
        }
    }
    else
        FIXME("Unsupported service %s.\n", debugstr_guid(service));

    if (*obj)
        IUnknown_AddRef((IUnknown *)*obj);

    return *obj ? S_OK : E_NOINTERFACE;
}

static const IMFGetServiceVtbl media_source_get_service_vtbl =
{
    media_source_get_service_QueryInterface,
    media_source_get_service_AddRef,
    media_source_get_service_Release,
    media_source_get_service_GetService,
};

static HRESULT WINAPI media_source_rate_support_QueryInterface(IMFRateSupport *iface, REFIID riid, void **obj)
{
    struct media_source *source = impl_from_IMFRateSupport(iface);
    return IMFMediaSource_QueryInterface(&source->IMFMediaSource_iface, riid, obj);
}

static ULONG WINAPI media_source_rate_support_AddRef(IMFRateSupport *iface)
{
    struct media_source *source = impl_from_IMFRateSupport(iface);
    return IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
}

static ULONG WINAPI media_source_rate_support_Release(IMFRateSupport *iface)
{
    struct media_source *source = impl_from_IMFRateSupport(iface);
    return IMFMediaSource_Release(&source->IMFMediaSource_iface);
}

static HRESULT WINAPI media_source_rate_support_GetSlowestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction, BOOL thin, float *rate)
{
    TRACE("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    *rate = 0.0f;

    return S_OK;
}

static HRESULT WINAPI media_source_rate_support_GetFastestRate(IMFRateSupport *iface, MFRATE_DIRECTION direction, BOOL thin, float *rate)
{
    TRACE("%p, %d, %d, %p.\n", iface, direction, thin, rate);

    *rate = direction == MFRATE_FORWARD ? 1e6f : -1e6f;

    return S_OK;
}

static HRESULT WINAPI media_source_rate_support_IsRateSupported(IMFRateSupport *iface, BOOL thin, float rate,
        float *nearest_rate)
{
    TRACE("%p, %d, %f, %p.\n", iface, thin, rate, nearest_rate);

    if (nearest_rate)
        *nearest_rate = rate;

    return rate >= -1e6f && rate <= 1e6f ? S_OK : MF_E_UNSUPPORTED_RATE;
}

static const IMFRateSupportVtbl media_source_rate_support_vtbl =
{
    media_source_rate_support_QueryInterface,
    media_source_rate_support_AddRef,
    media_source_rate_support_Release,
    media_source_rate_support_GetSlowestRate,
    media_source_rate_support_GetFastestRate,
    media_source_rate_support_IsRateSupported,
};

static HRESULT WINAPI media_source_rate_control_QueryInterface(IMFRateControl *iface, REFIID riid, void **obj)
{
    struct media_source *source = impl_from_IMFRateControl(iface);
    return IMFMediaSource_QueryInterface(&source->IMFMediaSource_iface, riid, obj);
}

static ULONG WINAPI media_source_rate_control_AddRef(IMFRateControl *iface)
{
    struct media_source *source = impl_from_IMFRateControl(iface);
    return IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
}

static ULONG WINAPI media_source_rate_control_Release(IMFRateControl *iface)
{
    struct media_source *source = impl_from_IMFRateControl(iface);
    return IMFMediaSource_Release(&source->IMFMediaSource_iface);
}

static HRESULT WINAPI media_source_rate_control_SetRate(IMFRateControl *iface, BOOL thin, float rate)
{
    struct media_source *source = impl_from_IMFRateControl(iface);
    HRESULT hr;

    FIXME("%p, %d, %f.\n", iface, thin, rate);

    if (rate < 0.0f)
        return MF_E_REVERSE_UNSUPPORTED;

    if (thin)
        return MF_E_THINNING_UNSUPPORTED;

    if (FAILED(hr = IMFRateSupport_IsRateSupported(&source->IMFRateSupport_iface, thin, rate, NULL)))
        return hr;

    EnterCriticalSection(&source->cs);
    source->rate = rate;
    LeaveCriticalSection(&source->cs);

    return IMFMediaEventQueue_QueueEventParamVar(source->event_queue, MESourceRateChanged, &GUID_NULL, S_OK, NULL);
}

static HRESULT WINAPI media_source_rate_control_GetRate(IMFRateControl *iface, BOOL *thin, float *rate)
{
    struct media_source *source = impl_from_IMFRateControl(iface);

    TRACE("%p, %p, %p.\n", iface, thin, rate);

    if (thin)
        *thin = FALSE;

    EnterCriticalSection(&source->cs);
    *rate = source->rate;
    LeaveCriticalSection(&source->cs);

    return S_OK;
}

static const IMFRateControlVtbl media_source_rate_control_vtbl =
{
    media_source_rate_control_QueryInterface,
    media_source_rate_control_AddRef,
    media_source_rate_control_Release,
    media_source_rate_control_SetRate,
    media_source_rate_control_GetRate,
};

static HRESULT WINAPI media_source_QueryInterface(IMFMediaSource *iface, REFIID riid, void **out)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaSource) ||
        IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &source->IMFMediaSource_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *out = &source->IMFGetService_iface;
    }
    else
    {
        FIXME("%s, %p.\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI media_source_AddRef(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    ULONG ref = InterlockedIncrement(&source->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI media_source_Release(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    ULONG ref = InterlockedDecrement(&source->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        IMFMediaSource_Shutdown(iface);
        IMFMediaEventQueue_Release(source->event_queue);
        IMFByteStream_Release(source->byte_stream);
        wg_parser_destroy(source->wg_parser);
        source->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&source->cs);
        free(source);
    }

    return ref;
}

static HRESULT WINAPI media_source_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("%p, %#lx, %p.\n", iface, flags, event);

    return IMFMediaEventQueue_GetEvent(source->event_queue, flags, event);
}

static HRESULT WINAPI media_source_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("%p, %p, %p.\n", iface, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(source->event_queue, callback, state);
}

static HRESULT WINAPI media_source_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("%p, %p, %p.\n", iface, result, event);

    return IMFMediaEventQueue_EndGetEvent(source->event_queue, result, event);
}

static HRESULT WINAPI media_source_QueueEvent(IMFMediaSource *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("%p, %lu, %s, %#lx, %p.\n", iface, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(source->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI media_source_GetCharacteristics(IMFMediaSource *iface, DWORD *characteristics)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, characteristics);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else
        *characteristics = MFMEDIASOURCE_CAN_SEEK | MFMEDIASOURCE_CAN_PAUSE;

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **descriptor)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    HRESULT hr;
    UINT i;

    TRACE("%p, %p.\n", iface, descriptor);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (SUCCEEDED(hr = MFCreatePresentationDescriptor(source->stream_count, source->descriptors, descriptor)))
    {
        if (FAILED(hr = IMFPresentationDescriptor_SetUINT64(*descriptor, &MF_PD_DURATION, source->duration)))
            WARN("Failed to set presentation descriptor MF_PD_DURATION, hr %#lx\n", hr);

        for (i = 0; i < source->stream_count; ++i)
        {
            if (FAILED(hr = IMFPresentationDescriptor_SelectStream(*descriptor, i)))
                WARN("Failed to select stream %u, hr %#lx\n", i, hr);
        }

        hr = S_OK;
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Start(IMFMediaSource *iface, IMFPresentationDescriptor *descriptor,
                                     const GUID *time_format, const PROPVARIANT *position)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    IUnknown *op;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, descriptor, time_format, position);

    if (!time_format)
        time_format = &GUID_NULL;

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (!(IsEqualIID(time_format, &GUID_NULL)))
        hr = MF_E_UNSUPPORTED_TIME_FORMAT;
    else if (SUCCEEDED(hr = source_create_async_op(SOURCE_ASYNC_START, &op)))
    {
        struct source_async_command *command = impl_from_async_command_IUnknown(op);
        command->u.start.descriptor = descriptor;
        IMFPresentationDescriptor_AddRef(descriptor);
        command->u.start.format = *time_format;
        PropVariantCopy(&command->u.start.position, position);

        hr = MFPutWorkItem(source->async_commands_queue, &source->async_commands_callback, op);
        IUnknown_Release(op);
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Stop(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    IUnknown *op;
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (SUCCEEDED(hr = source_create_async_op(SOURCE_ASYNC_STOP, &op)))
    {
        hr = MFPutWorkItem(source->async_commands_queue, &source->async_commands_callback, op);
        IUnknown_Release(op);
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Pause(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    IUnknown *op;
    HRESULT hr;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
        hr = MF_E_SHUTDOWN;
    else if (source->state != SOURCE_RUNNING)
        hr = MF_E_INVALID_STATE_TRANSITION;
    else if (SUCCEEDED(hr = source_create_async_op(SOURCE_ASYNC_PAUSE, &op)))
    {
        hr = MFPutWorkItem(source->async_commands_queue, &source->async_commands_callback, op);
        IUnknown_Release(op);
    }

    LeaveCriticalSection(&source->cs);

    return hr;
}

static HRESULT WINAPI media_source_Shutdown(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&source->cs);

    if (source->state == SOURCE_SHUTDOWN)
    {
        LeaveCriticalSection(&source->cs);
        return MF_E_SHUTDOWN;
    }

    source->state = SOURCE_SHUTDOWN;

    wg_parser_disconnect(source->wg_parser);

    source->read_thread_shutdown = true;
    WaitForSingleObject(source->read_thread, INFINITE);
    CloseHandle(source->read_thread);

    IMFMediaEventQueue_QueueEventParamVar(source->event_queue, MEError, &GUID_NULL, MF_E_SHUTDOWN, NULL);
    IMFMediaEventQueue_Shutdown(source->event_queue);
    IMFByteStream_Close(source->byte_stream);

    while (source->stream_count--)
    {
        struct media_stream *stream = source->streams[source->stream_count];
        IMFStreamDescriptor_Release(source->descriptors[source->stream_count]);
        IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, MEError, &GUID_NULL, MF_E_SHUTDOWN, NULL);
        IMFMediaEventQueue_Shutdown(stream->event_queue);
        IMFMediaStream_Release(&stream->IMFMediaStream_iface);
    }
    free(source->descriptors);
    free(source->streams);

    LeaveCriticalSection(&source->cs);

    MFUnlockWorkQueue(source->async_commands_queue);

    return S_OK;
}

static const IMFMediaSourceVtbl IMFMediaSource_vtbl =
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

static void media_source_init_descriptors(struct media_source *source)
{
    HRESULT hr = S_OK;
    UINT i;

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        IMFStreamDescriptor *descriptor = stream->descriptor;

        if (FAILED(hr = stream_descriptor_set_tag(descriptor, stream->wg_stream,
                &MF_SD_LANGUAGE, WG_PARSER_TAG_LANGUAGE)))
            WARN("Failed to set stream descriptor language, hr %#lx\n", hr);
        if (FAILED(hr = stream_descriptor_set_tag(descriptor, stream->wg_stream,
                &MF_SD_STREAM_NAME, WG_PARSER_TAG_NAME)))
            WARN("Failed to set stream descriptor name, hr %#lx\n", hr);
    }
}

static HRESULT media_source_create(struct object_context *context, IMFMediaSource **out)
{
    unsigned int stream_count = UINT_MAX;
    struct media_source *object;
    wg_parser_t parser;
    unsigned int i;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaSource_iface.lpVtbl = &IMFMediaSource_vtbl;
    object->IMFGetService_iface.lpVtbl = &media_source_get_service_vtbl;
    object->IMFRateSupport_iface.lpVtbl = &media_source_rate_support_vtbl;
    object->IMFRateControl_iface.lpVtbl = &media_source_rate_control_vtbl;
    object->async_commands_callback.lpVtbl = &source_async_commands_callback_vtbl;
    object->ref = 1;
    object->byte_stream = context->stream;
    IMFByteStream_AddRef(context->stream);
    object->file_size = context->file_size;
    object->rate = 1.0f;
    InitializeCriticalSectionEx(&object->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto fail;

    if (FAILED(hr = MFAllocateWorkQueue(&object->async_commands_queue)))
        goto fail;

    if (!(parser = wg_parser_create(FALSE)))
    {
        hr = E_OUTOFMEMORY;
        goto fail;
    }
    object->wg_parser = parser;

    object->read_thread = CreateThread(NULL, 0, read_thread, object, 0, NULL);

    object->state = SOURCE_OPENING;

    if (FAILED(hr = wg_parser_connect(parser, object->file_size, context->url)))
        goto fail;

    stream_count = wg_parser_get_stream_count(parser);

    if (!(object->descriptors = calloc(stream_count, sizeof(*object->descriptors)))
            || !(object->streams = calloc(stream_count, sizeof(*object->streams))))
    {
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    for (i = 0; i < stream_count; ++i)
    {
        wg_parser_stream_t wg_stream = wg_parser_get_stream(object->wg_parser, i);
        IMFStreamDescriptor *descriptor;
        struct media_stream *stream;
        struct wg_format format;

        wg_parser_stream_get_current_format(wg_stream, &format);
        if (FAILED(hr = stream_descriptor_create(i, &format, &descriptor)))
            goto fail;
        if (FAILED(hr = media_stream_create(&object->IMFMediaSource_iface, descriptor, wg_stream, &stream)))
        {
            IMFStreamDescriptor_Release(descriptor);
            goto fail;
        }

        object->duration = max(object->duration, wg_parser_stream_get_duration(wg_stream));
        IMFStreamDescriptor_AddRef(descriptor);
        object->descriptors[i] = descriptor;
        object->streams[i] = stream;
        object->stream_count++;
    }

    media_source_init_descriptors(object);
    object->state = SOURCE_STOPPED;

    *out = &object->IMFMediaSource_iface;
    TRACE("Created IMFMediaSource %p\n", *out);
    return S_OK;

fail:
    WARN("Failed to construct MFMediaSource, hr %#lx.\n", hr);

    while (object->streams && object->stream_count--)
    {
        struct media_stream *stream = object->streams[object->stream_count];
        IMFStreamDescriptor_Release(object->descriptors[object->stream_count]);
        IMFMediaStream_Release(&stream->IMFMediaStream_iface);
    }
    free(object->descriptors);
    free(object->streams);

    if (stream_count != UINT_MAX)
        wg_parser_disconnect(object->wg_parser);
    if (object->read_thread)
    {
        object->read_thread_shutdown = true;
        WaitForSingleObject(object->read_thread, INFINITE);
        CloseHandle(object->read_thread);
    }
    if (object->wg_parser)
        wg_parser_destroy(object->wg_parser);
    if (object->async_commands_queue)
        MFUnlockWorkQueue(object->async_commands_queue);
    if (object->event_queue)
        IMFMediaEventQueue_Release(object->event_queue);
    IMFByteStream_Release(object->byte_stream);
    free(object);
    return hr;
}

struct result_entry
{
    struct list entry;
    IMFAsyncResult *result;
    MF_OBJECT_TYPE type;
    IUnknown *object;
};

static HRESULT result_entry_create(IMFAsyncResult *result, MF_OBJECT_TYPE type,
        IUnknown *object, struct result_entry **out)
{
    struct result_entry *entry;

    if (!(entry = malloc(sizeof(*entry))))
        return E_OUTOFMEMORY;

    entry->result = result;
    IMFAsyncResult_AddRef(entry->result);
    entry->object = object;
    IUnknown_AddRef(entry->object);
    entry->type = type;

    *out = entry;
    return S_OK;
}

static void result_entry_destroy(struct result_entry *entry)
{
    IMFAsyncResult_Release(entry->result);
    IUnknown_Release(entry->object);
    free(entry);
}

struct stream_handler
{
    IMFByteStreamHandler IMFByteStreamHandler_iface;
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;
    struct list results;
    CRITICAL_SECTION cs;
};

static struct result_entry *handler_find_result_entry(struct stream_handler *handler, IMFAsyncResult *result)
{
    struct result_entry *entry;

    EnterCriticalSection(&handler->cs);
    LIST_FOR_EACH_ENTRY(entry, &handler->results, struct result_entry, entry)
    {
        if (result == entry->result)
        {
            list_remove(&entry->entry);
            LeaveCriticalSection(&handler->cs);
            return entry;
        }
    }
    LeaveCriticalSection(&handler->cs);

    return NULL;
}

static struct stream_handler *impl_from_IMFByteStreamHandler(IMFByteStreamHandler *iface)
{
    return CONTAINING_RECORD(iface, struct stream_handler, IMFByteStreamHandler_iface);
}

static struct stream_handler *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct stream_handler, IMFAsyncCallback_iface);
}

static HRESULT WINAPI stream_handler_QueryInterface(IMFByteStreamHandler *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFByteStreamHandler) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFByteStreamHandler_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI stream_handler_AddRef(IMFByteStreamHandler *iface)
{
    struct stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);

    TRACE("%p, refcount %lu.\n", handler, refcount);

    return refcount;
}

static ULONG WINAPI stream_handler_Release(IMFByteStreamHandler *iface)
{
    struct stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);
    struct result_entry *result, *next;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        LIST_FOR_EACH_ENTRY_SAFE(result, next, &handler->results, struct result_entry, entry)
            result_entry_destroy(result);
        DeleteCriticalSection(&handler->cs);
        free(handler);
    }

    return refcount;
}

static HRESULT WINAPI stream_handler_BeginCreateObject(IMFByteStreamHandler *iface, IMFByteStream *stream, const WCHAR *url, DWORD flags,
        IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    IMFAsyncResult *result;
    IUnknown *context;
    QWORD file_size;
    HRESULT hr;
    DWORD caps;

    TRACE("%p, %s, %#lx, %p, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, cancel_cookie, callback, state);

    if (cancel_cookie)
        *cancel_cookie = NULL;

    if (!stream)
        return E_INVALIDARG;
    if (flags != MF_RESOLUTION_MEDIASOURCE)
        FIXME("Unimplemented flags %#lx\n", flags);

    if (FAILED(hr = IMFByteStream_GetCapabilities(stream, &caps)))
        return hr;
    if (!(caps & MFBYTESTREAM_IS_SEEKABLE))
    {
        FIXME("Non-seekable bytestreams not supported.\n");
        return MF_E_BYTESTREAM_NOT_SEEKABLE;
    }
    if (FAILED(hr = IMFByteStream_GetLength(stream, &file_size)))
    {
        FIXME("Failed to get byte stream length, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;
    if (FAILED(hr = object_context_create(flags, stream, url, file_size, result, &context)))
    {
        IMFAsyncResult_Release(result);
        return hr;
    }

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_IO, &handler->IMFAsyncCallback_iface, context);
    IUnknown_Release(context);

    if (SUCCEEDED(hr) && cancel_cookie)
    {
        *cancel_cookie = (IUnknown *)result;
        IUnknown_AddRef(*cancel_cookie);
    }

    IMFAsyncResult_Release(result);

    return hr;
}

static HRESULT WINAPI stream_handler_EndCreateObject(IMFByteStreamHandler *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *type, IUnknown **object)
{
    struct stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    struct result_entry *entry;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, result, type, object);

    if (!(entry = handler_find_result_entry(handler, result)))
    {
        *type = MF_OBJECT_INVALID;
        *object = NULL;
        return MF_E_UNEXPECTED;
    }

    hr = IMFAsyncResult_GetStatus(entry->result);
    *type = entry->type;
    *object = entry->object;
    IUnknown_AddRef(*object);
    result_entry_destroy(entry);
    return hr;
}

static HRESULT WINAPI stream_handler_CancelObjectCreation(IMFByteStreamHandler *iface, IUnknown *cookie)
{
    struct stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    IMFAsyncResult *result = (IMFAsyncResult *)cookie;
    struct result_entry *entry;

    TRACE("%p, %p.\n", iface, cookie);

    if (!(entry = handler_find_result_entry(handler, result)))
        return MF_E_UNEXPECTED;

    result_entry_destroy(entry);
    return S_OK;
}

static HRESULT WINAPI stream_handler_GetMaxNumberOfBytesRequiredForResolution(IMFByteStreamHandler *iface, QWORD *bytes)
{
    FIXME("stub (%p %p)\n", iface, bytes);
    return E_NOTIMPL;
}

static const IMFByteStreamHandlerVtbl stream_handler_vtbl =
{
    stream_handler_QueryInterface,
    stream_handler_AddRef,
    stream_handler_Release,
    stream_handler_BeginCreateObject,
    stream_handler_EndCreateObject,
    stream_handler_CancelObjectCreation,
    stream_handler_GetMaxNumberOfBytesRequiredForResolution,
};

static HRESULT WINAPI stream_handler_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
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

static ULONG WINAPI stream_handler_callback_AddRef(IMFAsyncCallback *iface)
{
    struct stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFByteStreamHandler_AddRef(&handler->IMFByteStreamHandler_iface);
}

static ULONG WINAPI stream_handler_callback_Release(IMFAsyncCallback *iface)
{
    struct stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFByteStreamHandler_Release(&handler->IMFByteStreamHandler_iface);
}

static HRESULT WINAPI stream_handler_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_handler_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    IUnknown *object, *state = IMFAsyncResult_GetStateNoAddRef(result);
    struct object_context *context;
    struct result_entry *entry;
    HRESULT hr;

    if (!state || !(context = impl_from_IUnknown(state)))
        return E_INVALIDARG;

    if (FAILED(hr = media_source_create(context, (IMFMediaSource **)&object)))
        WARN("Failed to create media source, hr %#lx\n", hr);
    else
    {
        if (FAILED(hr = result_entry_create(context->result, MF_OBJECT_MEDIASOURCE, object, &entry)))
            WARN("Failed to create handler result, hr %#lx\n", hr);
        else
        {
            EnterCriticalSection(&handler->cs);
            list_add_tail(&handler->results, &entry->entry);
            LeaveCriticalSection(&handler->cs);
        }

        IUnknown_Release(object);
    }

    IMFAsyncResult_SetStatus(context->result, hr);
    MFInvokeCallback(context->result);

    return S_OK;
}

static const IMFAsyncCallbackVtbl stream_handler_callback_vtbl =
{
    stream_handler_callback_QueryInterface,
    stream_handler_callback_AddRef,
    stream_handler_callback_Release,
    stream_handler_callback_GetParameters,
    stream_handler_callback_Invoke,
};

HRESULT gstreamer_byte_stream_handler_create(REFIID riid, void **obj)
{
    struct stream_handler *handler;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (!(handler = calloc(1, sizeof(*handler))))
        return E_OUTOFMEMORY;

    list_init(&handler->results);
    InitializeCriticalSection(&handler->cs);

    handler->IMFByteStreamHandler_iface.lpVtbl = &stream_handler_vtbl;
    handler->IMFAsyncCallback_iface.lpVtbl = &stream_handler_callback_vtbl;
    handler->refcount = 1;

    hr = IMFByteStreamHandler_QueryInterface(&handler->IMFByteStreamHandler_iface, riid, obj);
    IMFByteStreamHandler_Release(&handler->IMFByteStreamHandler_iface);

    return hr;
}
