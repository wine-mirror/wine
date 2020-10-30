/* GStreamer Media Source
 *
 * Copyright 2020 Derek Lesho
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

#include "config.h"

#include <gst/gst.h>

#include "gst_private.h"
#include "gst_cbs.h"

#include <assert.h>
#include <stdarg.h>
#include <assert.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "mfapi.h"
#include "mferror.h"
#include "mfidl.h"
#include "mfobjects.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct media_stream
{
    IMFMediaStream IMFMediaStream_iface;
    LONG ref;
    struct media_source *parent_source;
    IMFMediaEventQueue *event_queue;
    IMFStreamDescriptor *descriptor;
    GstElement *appsink;
    GstPad *their_src, *my_sink;
    enum
    {
        STREAM_INACTIVE,
        STREAM_SHUTDOWN,
        STREAM_RUNNING,
    } state;
    DWORD stream_id;
    BOOL eos;
};

enum source_async_op
{
    SOURCE_ASYNC_START,
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
    IMFAsyncCallback async_commands_callback;
    LONG ref;
    DWORD async_commands_queue;
    IMFMediaEventQueue *event_queue;
    IMFByteStream *byte_stream;
    struct media_stream **streams;
    ULONG stream_count;
    IMFPresentationDescriptor *pres_desc;
    GstBus *bus;
    GstElement *container;
    GstElement *decodebin;
    GstPad *my_src, *their_sink;
    enum
    {
        SOURCE_OPENING,
        SOURCE_STOPPED,
        SOURCE_RUNNING,
        SOURCE_SHUTDOWN,
    } state;
    HANDLE no_more_pads_event;
};

static inline struct media_stream *impl_from_IMFMediaStream(IMFMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct media_stream, IMFMediaStream_iface);
}

static inline struct media_source *impl_from_IMFMediaSource(IMFMediaSource *iface)
{
    return CONTAINING_RECORD(iface, struct media_source, IMFMediaSource_iface);
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
            PropVariantClear(&command->u.start.position);
        heap_free(command);
    }

    return refcount;
}

static const IUnknownVtbl source_async_command_vtbl =
{
    source_async_command_QueryInterface,
    source_async_command_AddRef,
    source_async_command_Release,
};

static HRESULT source_create_async_op(enum source_async_op op, struct source_async_command **ret)
{
    struct source_async_command *command;

    if (!(command = heap_alloc_zero(sizeof(*command))))
        return E_OUTOFMEMORY;

    command->IUnknown_iface.lpVtbl = &source_async_command_vtbl;
    command->op = op;

    *ret = command;

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

static IMFStreamDescriptor *stream_descriptor_from_id(IMFPresentationDescriptor *pres_desc, DWORD id, BOOL *selected)
{
    ULONG sd_count;
    IMFStreamDescriptor *ret;
    unsigned int i;

    if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorCount(pres_desc, &sd_count)))
        return NULL;

    for (i = 0; i < sd_count; i++)
    {
        DWORD stream_id;

        if (FAILED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(pres_desc, i, selected, &ret)))
            return NULL;

        if (SUCCEEDED(IMFStreamDescriptor_GetStreamIdentifier(ret, &stream_id)) && stream_id == id)
            return ret;

        IMFStreamDescriptor_Release(ret);
    }
    return NULL;
}

static void start_pipeline(struct media_source *source, struct source_async_command *command)
{
    PROPVARIANT *position = &command->u.start.position;
    BOOL seek_message = source->state != SOURCE_STOPPED && position->vt != VT_EMPTY;
    GstStateChangeReturn ret;
    unsigned int i;

    gst_element_set_state(source->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(source->container, NULL, NULL, -1);
    assert(ret == GST_STATE_CHANGE_SUCCESS);

    /* seek to beginning on stop->play */
    if (source->state == SOURCE_STOPPED && position->vt == VT_EMPTY)
    {
        position->vt = VT_I8;
        position->u.hVal.QuadPart = 0;
    }

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream;
        IMFStreamDescriptor *sd;
        IMFMediaTypeHandler *mth;
        IMFMediaType *current_mt;
        GstCaps *current_caps;
        GstCaps *prev_caps;
        DWORD stream_id;
        BOOL was_active;
        BOOL selected;

        stream = source->streams[i];

        IMFStreamDescriptor_GetStreamIdentifier(stream->descriptor, &stream_id);

        sd = stream_descriptor_from_id(command->u.start.descriptor, stream_id, &selected);
        IMFStreamDescriptor_Release(sd);

        was_active = stream->state != STREAM_INACTIVE;

        stream->state = selected ? STREAM_RUNNING : STREAM_INACTIVE;

        if (selected)
        {
            IMFStreamDescriptor_GetMediaTypeHandler(stream->descriptor, &mth);
            IMFMediaTypeHandler_GetCurrentMediaType(mth, &current_mt);
            current_caps = caps_from_mf_media_type(current_mt);
            g_object_get(stream->appsink, "caps", &prev_caps, NULL);
            if (!prev_caps || !gst_caps_is_equal(prev_caps, current_caps))
            {
                GstEvent *reconfigure_event = gst_event_new_reconfigure();
                g_object_set(stream->appsink, "caps", current_caps, NULL);
                gst_pad_push_event(gst_element_get_static_pad(stream->appsink, "sink"), reconfigure_event);
            }

            gst_caps_unref(current_caps);
            if (prev_caps)
                gst_caps_unref(prev_caps);
            IMFMediaType_Release(current_mt);
            IMFMediaTypeHandler_Release(mth);
        }

        g_object_set(stream->appsink, "drop", !selected, NULL);

        if (position->vt != VT_EMPTY)
        {
            GstEvent *seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, position->u.hVal.QuadPart / 100, GST_SEEK_TYPE_NONE, 0);

            gst_pad_push_event(stream->my_sink, seek_event);

            stream->eos = FALSE;
        }

        if (selected)
        {
            TRACE("Stream %u (%p) selected\n", i, stream);
            IMFMediaEventQueue_QueueEventParamUnk(source->event_queue,
                was_active ? MEUpdatedStream : MENewStream, &GUID_NULL,
                S_OK, (IUnknown*) &stream->IMFMediaStream_iface);

            IMFMediaEventQueue_QueueEventParamVar(stream->event_queue,
                seek_message ? MEStreamSeeked : MEStreamStarted, &GUID_NULL, S_OK, position);
        }
    }

    IMFMediaEventQueue_QueueEventParamVar(source->event_queue,
        seek_message ? MESourceSeeked : MESourceStarted,
        &GUID_NULL, S_OK, position);

    source->state = SOURCE_RUNNING;

    gst_element_set_state(source->container, GST_STATE_PLAYING);
}

static void dispatch_end_of_presentation(struct media_source *source)
{
    PROPVARIANT empty = {.vt = VT_EMPTY};
    unsigned int i;

    /* A stream has ended, check whether all have */
    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];

        if (stream->state != STREAM_INACTIVE && !stream->eos)
            return;
    }

    IMFMediaEventQueue_QueueEventParamVar(source->event_queue, MEEndOfPresentation, &GUID_NULL, S_OK, &empty);
}

static void wait_on_sample(struct media_stream *stream, IUnknown *token)
{
    PROPVARIANT empty_var = {.vt = VT_EMPTY};
    GstSample *gst_sample;
    GstBuffer *buffer;
    IMFSample *sample;

    TRACE("%p, %p\n", stream, token);

    g_signal_emit_by_name(stream->appsink, "pull-sample", &gst_sample);
    if (gst_sample)
    {
        buffer = gst_sample_get_buffer(gst_sample);

        TRACE("PTS = %llu\n", (unsigned long long int) GST_BUFFER_PTS(buffer));

        sample = mf_sample_from_gst_buffer(buffer);
        gst_sample_unref(gst_sample);

        if (token)
            IMFSample_SetUnknown(sample, &MFSampleExtension_Token, token);

        IMFMediaEventQueue_QueueEventParamUnk(stream->event_queue, MEMediaSample, &GUID_NULL, S_OK, (IUnknown *)sample);
        IMFSample_Release(sample);
    }
    else
    {
        g_object_get(stream->appsink, "eos", &stream->eos, NULL);
        if (stream->eos)
        {
            if (token)
                IUnknown_Release(token);
            IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, MEEndOfStream, &GUID_NULL, S_OK, &empty_var);
            dispatch_end_of_presentation(stream->parent_source);
        }
    }
}

static HRESULT WINAPI source_async_commands_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_source *source = impl_from_async_commands_callback_IMFAsyncCallback(iface);
    struct source_async_command *command;
    IUnknown *state;
    HRESULT hr;

    if (source->state == SOURCE_SHUTDOWN)
        return S_OK;

    if (FAILED(hr = IMFAsyncResult_GetState(result, &state)))
        return hr;

    command = impl_from_async_command_IUnknown(state);
    switch (command->op)
    {
        case SOURCE_ASYNC_START:
            start_pipeline(source, command);
            break;
        case SOURCE_ASYNC_REQUEST_SAMPLE:
            wait_on_sample(command->u.request_sample.stream, command->u.request_sample.token);
            break;
    }

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

GstFlowReturn bytestream_wrapper_pull(GstPad *pad, GstObject *parent, guint64 ofs, guint len,
        GstBuffer **buf)
{
    struct media_source *source = gst_pad_get_element_private(pad);
    IMFByteStream *byte_stream = source->byte_stream;
    ULONG bytes_read;
    GstMapInfo info;
    BOOL is_eof;
    HRESULT hr;

    TRACE("requesting %u bytes at %s from source %p into buffer %p\n", len, wine_dbgstr_longlong(ofs), source, *buf);

    if (ofs != GST_BUFFER_OFFSET_NONE)
    {
        if (FAILED(IMFByteStream_SetCurrentPosition(byte_stream, ofs)))
            return GST_FLOW_ERROR;
    }

    if (FAILED(IMFByteStream_IsEndOfStream(byte_stream, &is_eof)))
        return GST_FLOW_ERROR;
    if (is_eof)
        return GST_FLOW_EOS;

    if (!(*buf))
        *buf = gst_buffer_new_and_alloc(len);
    gst_buffer_map(*buf, &info, GST_MAP_WRITE);
    hr = IMFByteStream_Read(byte_stream, info.data, len, &bytes_read);
    gst_buffer_unmap(*buf, &info);

    gst_buffer_set_size(*buf, bytes_read);

    if (FAILED(hr))
        return GST_FLOW_ERROR;
    return GST_FLOW_OK;
}

static gboolean bytestream_query(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct media_source *source = gst_pad_get_element_private(pad);
    GstFormat format;
    QWORD bytestream_len;

    TRACE("GStreamer queries source %p for %s\n", source, GST_QUERY_TYPE_NAME(query));

    if (FAILED(IMFByteStream_GetLength(source->byte_stream, &bytestream_len)))
        return FALSE;

    switch (GST_QUERY_TYPE(query))
    {
        case GST_QUERY_DURATION:
        {
            gst_query_parse_duration(query, &format, NULL);
            if (format == GST_FORMAT_PERCENT)
            {
                gst_query_set_duration(query, GST_FORMAT_PERCENT, GST_FORMAT_PERCENT_MAX);
                return TRUE;
            }
            else if (format == GST_FORMAT_BYTES)
            {
                QWORD length;
                IMFByteStream_GetLength(source->byte_stream, &length);
                gst_query_set_duration(query, GST_FORMAT_BYTES, length);
                return TRUE;
            }
            return FALSE;
        }
        case GST_QUERY_SEEKING:
        {
            gst_query_parse_seeking (query, &format, NULL, NULL, NULL);
            if (format != GST_FORMAT_BYTES)
            {
                WARN("Cannot seek using format \"%s\".\n", gst_format_get_name(format));
                return FALSE;
            }
            gst_query_set_seeking(query, GST_FORMAT_BYTES, 1, 0, bytestream_len);
            return TRUE;
        }
        case GST_QUERY_SCHEDULING:
        {
            gst_query_set_scheduling(query, GST_SCHEDULING_FLAG_SEEKABLE, 1, -1, 0);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PULL);
            return TRUE;
        }
        default:
        {
            WARN("Unhandled query type %s\n", GST_QUERY_TYPE_NAME(query));
            return FALSE;
        }
    }
}

static gboolean bytestream_pad_mode_activate(GstPad *pad, GstObject *parent, GstPadMode mode, gboolean activate)
{
    struct media_source *source = gst_pad_get_element_private(pad);

    TRACE("%s source pad for mediasource %p in %s mode.\n",
            activate ? "Activating" : "Deactivating", source, gst_pad_mode_get_name(mode));

    return mode == GST_PAD_MODE_PULL;
}

static gboolean bytestream_pad_event_process(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct media_source *source = gst_pad_get_element_private(pad);

    TRACE("source %p, type \"%s\".\n", source, GST_EVENT_TYPE_NAME(event));

    switch (event->type) {
        /* the seek event should fail in pull mode */
        case GST_EVENT_SEEK:
            return FALSE;
        default:
            WARN("Ignoring \"%s\" event.\n", GST_EVENT_TYPE_NAME(event));
        case GST_EVENT_TAG:
        case GST_EVENT_QOS:
        case GST_EVENT_RECONFIGURE:
            return gst_pad_event_default(pad, parent, event);
    }
    return TRUE;
}

GstBusSyncReply bus_watch(GstBus *bus, GstMessage *message, gpointer user)
{
    struct media_source *source = user;
    gchar *dbg_info = NULL;
    GError *err = NULL;

    TRACE("source %p message type %s\n", source, GST_MESSAGE_TYPE_NAME(message));

    switch (message->type)
    {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(message, &err, &dbg_info);
            ERR("%s: %s\n", GST_OBJECT_NAME(message->src), err->message);
            ERR("%s\n", dbg_info);
            g_error_free(err);
            g_free(dbg_info);
            break;
        case GST_MESSAGE_WARNING:
            gst_message_parse_warning(message, &err, &dbg_info);
            WARN("%s: %s\n", GST_OBJECT_NAME(message->src), err->message);
            WARN("%s\n", dbg_info);
            g_error_free(err);
            g_free(dbg_info);
            break;
        default:
            break;
    }

    gst_message_unref(message);
    return GST_BUS_DROP;
}

static HRESULT WINAPI media_stream_QueryInterface(IMFMediaStream *iface, REFIID riid, void **out)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("(%p)->(%s %p)\n", stream, debugstr_guid(riid), out);

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

    TRACE("(%p) ref=%u\n", stream, ref);

    return ref;
}

static ULONG WINAPI media_stream_Release(IMFMediaStream *iface)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    ULONG ref = InterlockedDecrement(&stream->ref);

    TRACE("(%p) ref=%u\n", stream, ref);

    if (!ref)
    {
        if (stream->event_queue)
            IMFMediaEventQueue_Release(stream->event_queue);
        heap_free(stream);
    }

    return ref;
}

static HRESULT WINAPI media_stream_GetEvent(IMFMediaStream *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("(%p)->(%#x, %p)\n", stream, flags, event);

    return IMFMediaEventQueue_GetEvent(stream->event_queue, flags, event);
}

static HRESULT WINAPI media_stream_BeginGetEvent(IMFMediaStream *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("(%p)->(%p, %p)\n", stream, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(stream->event_queue, callback, state);
}

static HRESULT WINAPI media_stream_EndGetEvent(IMFMediaStream *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("(%p)->(%p, %p)\n", stream, result, event);

    return IMFMediaEventQueue_EndGetEvent(stream->event_queue, result, event);
}

static HRESULT WINAPI media_stream_QueueEvent(IMFMediaStream *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("(%p)->(%d, %s, %#x, %p)\n", stream, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(stream->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI media_stream_GetMediaSource(IMFMediaStream *iface, IMFMediaSource **source)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    FIXME("stub (%p)->(%p)\n", stream, source);

    if (stream->state == STREAM_SHUTDOWN)
        return MF_E_SHUTDOWN;

    return E_NOTIMPL;
}

static HRESULT WINAPI media_stream_GetStreamDescriptor(IMFMediaStream* iface, IMFStreamDescriptor **descriptor)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);

    TRACE("(%p)->(%p)\n", stream, descriptor);

    if (stream->state == STREAM_SHUTDOWN)
        return MF_E_SHUTDOWN;

    IMFStreamDescriptor_AddRef(stream->descriptor);
    *descriptor = stream->descriptor;

    return S_OK;
}

static HRESULT WINAPI media_stream_RequestSample(IMFMediaStream *iface, IUnknown *token)
{
    struct media_stream *stream = impl_from_IMFMediaStream(iface);
    struct source_async_command *command;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, token);

    if (stream->state == STREAM_SHUTDOWN)
        return MF_E_SHUTDOWN;

    if (stream->state == STREAM_INACTIVE)
    {
        WARN("Stream isn't active\n");
        return MF_E_MEDIA_SOURCE_WRONGSTATE;
    }

    if (stream->eos)
    {
        return MF_E_END_OF_STREAM;
    }

    if (SUCCEEDED(hr = source_create_async_op(SOURCE_ASYNC_REQUEST_SAMPLE, &command)))
    {
        command->u.request_sample.stream = stream;
        if (token)
            IUnknown_AddRef(token);
        command->u.request_sample.token = token;

        /* Once pause support is added, this will need to put into a stream queue, and synchronization will need to be added*/
        hr = MFPutWorkItem(stream->parent_source->async_commands_queue, &stream->parent_source->async_commands_callback, &command->IUnknown_iface);
    }

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

/* Setup a chain of elements which should hopefully allow transformations to any IMFMediaType
   the user throws at us through gstreamer's caps negotiation. */
static HRESULT media_stream_connect_to_sink(struct media_stream *stream)
{
    GstCaps *source_caps = gst_pad_query_caps(stream->their_src, NULL);
    const gchar *stream_type;

    if (!source_caps)
        return E_FAIL;

    stream_type = gst_structure_get_name(gst_caps_get_structure(source_caps, 0));
    gst_caps_unref(source_caps);

    if (!strcmp(stream_type, "video/x-raw"))
    {
        GstElement *videoconvert = gst_element_factory_make("videoconvert", NULL);

        gst_bin_add(GST_BIN(stream->parent_source->container), videoconvert);

        stream->my_sink = gst_element_get_static_pad(videoconvert, "sink");

        if (!gst_element_link(videoconvert, stream->appsink))
            return E_FAIL;

        gst_element_sync_state_with_parent(videoconvert);
    }
    else if (!strcmp(stream_type, "audio/x-raw"))
    {
        GstElement *audioconvert = gst_element_factory_make("audioconvert", NULL);

        gst_bin_add(GST_BIN(stream->parent_source->container), audioconvert);

        stream->my_sink = gst_element_get_static_pad(audioconvert, "sink");

        if (!gst_element_link(audioconvert, stream->appsink))
            return E_FAIL;

        gst_element_sync_state_with_parent(audioconvert);
    }
    else
    {
        stream->my_sink = gst_element_get_static_pad(stream->appsink, "sink");
    }

    if (gst_pad_link(stream->their_src, stream->my_sink) != GST_PAD_LINK_OK)
        return E_FAIL;

    return S_OK;
}

static HRESULT new_media_stream(struct media_source *source, GstPad *pad, DWORD stream_id, struct media_stream **out_stream)
{
    struct media_stream *object = heap_alloc_zero(sizeof(*object));
    HRESULT hr;

    TRACE("(%p %p)->(%p)\n", source, pad, out_stream);

    object->IMFMediaStream_iface.lpVtbl = &media_stream_vtbl;
    object->ref = 1;

    IMFMediaSource_AddRef(&source->IMFMediaSource_iface);
    object->parent_source = source;
    object->their_src = pad;
    object->stream_id = stream_id;

    object->state = STREAM_INACTIVE;
    object->eos = FALSE;

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto fail;

    if (!(object->appsink = gst_element_factory_make("appsink", NULL)))
    {
        hr = E_OUTOFMEMORY;
        goto fail;
    }
    gst_bin_add(GST_BIN(object->parent_source->container), object->appsink);

    g_object_set(object->appsink, "sync", FALSE, NULL);
    g_object_set(object->appsink, "max-buffers", 5, NULL);

    if (FAILED(hr = media_stream_connect_to_sink(object)))
        goto fail;

    gst_element_sync_state_with_parent(object->appsink);

    TRACE("->(%p)\n", object);
    *out_stream = object;

    return S_OK;

fail:
    WARN("Failed to construct media stream, hr %#x.\n", hr);

    IMFMediaStream_Release(&object->IMFMediaStream_iface);
    return hr;
}

static HRESULT media_stream_init_desc(struct media_stream *stream)
{
    GstCaps *current_caps = gst_pad_get_current_caps(stream->their_src);
    IMFMediaTypeHandler *type_handler;
    IMFMediaType **stream_types = NULL;
    IMFMediaType *stream_type = NULL;
    DWORD type_count = 0;
    const gchar *major_type;
    unsigned int i;
    HRESULT hr;

    major_type = gst_structure_get_name(gst_caps_get_structure(current_caps, 0));

    if (!strcmp(major_type, "video/x-raw"))
    {
        /* These are the most common native output types of decoders:
            https://docs.microsoft.com/en-us/windows/win32/medfound/mft-decoder-expose-output-types-in-native-order */
        static const GUID *const video_types[] =
        {
            &MFVideoFormat_NV12,
            &MFVideoFormat_YV12,
            &MFVideoFormat_YUY2,
            &MFVideoFormat_IYUV,
            &MFVideoFormat_I420,
        };

        IMFMediaType *base_type = mf_media_type_from_caps(current_caps);
        GUID base_subtype;

        IMFMediaType_GetGUID(base_type, &MF_MT_SUBTYPE, &base_subtype);

        stream_types = heap_alloc( sizeof(IMFMediaType *) * ARRAY_SIZE(video_types) + 1);

        stream_types[0] = base_type;
        type_count = 1;

        for (i = 0; i < ARRAY_SIZE(video_types); i++)
        {
            IMFMediaType *new_type;

            if (IsEqualGUID(&base_subtype, video_types[i]))
                continue;

            if (FAILED(hr = MFCreateMediaType(&new_type)))
                goto done;
            stream_types[type_count++] = new_type;

            if (FAILED(hr = IMFMediaType_CopyAllItems(base_type, (IMFAttributes *) new_type)))
                goto done;
            if (FAILED(hr = IMFMediaType_SetGUID(new_type, &MF_MT_SUBTYPE, video_types[i])))
                goto done;
        }
    }
    else
    {
        stream_type = mf_media_type_from_caps(current_caps);
        if (stream_type)
        {
            stream_types = &stream_type;
            type_count = 1;
        }
    }

    if (!type_count)
    {
        ERR("Failed to establish an IMFMediaType from any of the possible stream caps!\n");
        return E_FAIL;
    }

    if (FAILED(hr = MFCreateStreamDescriptor(stream->stream_id, type_count, stream_types, &stream->descriptor)))
        goto done;

    if (FAILED(hr = IMFStreamDescriptor_GetMediaTypeHandler(stream->descriptor, &type_handler)))
        goto done;

    if (FAILED(hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, stream_types[0])))
        goto done;

done:
    gst_caps_unref(current_caps);
    if (type_handler)
        IMFMediaTypeHandler_Release(type_handler);
    for (i = 0; i < type_count; i++)
        IMFMediaType_Release(stream_types[i]);
    if (stream_types != &stream_type)
        heap_free(stream_types);
    return hr;
}

static HRESULT WINAPI media_source_QueryInterface(IMFMediaSource *iface, REFIID riid, void **out)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("(%p)->(%s %p)\n", source, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaSource) ||
        IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &source->IMFMediaSource_iface;
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

static ULONG WINAPI media_source_AddRef(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    ULONG ref = InterlockedIncrement(&source->ref);

    TRACE("(%p) ref=%u\n", source, ref);

    return ref;
}

static ULONG WINAPI media_source_Release(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    ULONG ref = InterlockedDecrement(&source->ref);

    TRACE("(%p) ref=%u\n", source, ref);

    if (!ref)
    {
        IMFMediaSource_Shutdown(&source->IMFMediaSource_iface);
        IMFMediaEventQueue_Release(source->event_queue);
        heap_free(source);
    }

    return ref;
}

static HRESULT WINAPI media_source_GetEvent(IMFMediaSource *iface, DWORD flags, IMFMediaEvent **event)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("(%p)->(%#x, %p)\n", source, flags, event);

    return IMFMediaEventQueue_GetEvent(source->event_queue, flags, event);
}

static HRESULT WINAPI media_source_BeginGetEvent(IMFMediaSource *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("(%p)->(%p, %p)\n", source, callback, state);

    return IMFMediaEventQueue_BeginGetEvent(source->event_queue, callback, state);
}

static HRESULT WINAPI media_source_EndGetEvent(IMFMediaSource *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("(%p)->(%p, %p)\n", source, result, event);

    return IMFMediaEventQueue_EndGetEvent(source->event_queue, result, event);
}

static HRESULT WINAPI media_source_QueueEvent(IMFMediaSource *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("(%p)->(%d, %s, %#x, %p)\n", source, event_type, debugstr_guid(ext_type), hr, value);

    return IMFMediaEventQueue_QueueEventParamVar(source->event_queue, event_type, ext_type, hr, value);
}

static HRESULT WINAPI media_source_GetCharacteristics(IMFMediaSource *iface, DWORD *characteristics)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    FIXME("(%p)->(%p): stub\n", source, characteristics);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    return E_NOTIMPL;
}

static HRESULT WINAPI media_source_CreatePresentationDescriptor(IMFMediaSource *iface, IMFPresentationDescriptor **descriptor)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    TRACE("(%p)->(%p)\n", source, descriptor);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    return IMFPresentationDescriptor_Clone(source->pres_desc, descriptor);
}

static HRESULT WINAPI media_source_Start(IMFMediaSource *iface, IMFPresentationDescriptor *descriptor,
                                     const GUID *time_format, const PROPVARIANT *position)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    struct source_async_command *command;
    HRESULT hr;

    TRACE("(%p)->(%p, %p, %p)\n", source, descriptor, time_format, position);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    if (!(IsEqualIID(time_format, &GUID_NULL)))
        return MF_E_UNSUPPORTED_TIME_FORMAT;

    if (SUCCEEDED(hr = source_create_async_op(SOURCE_ASYNC_START, &command)))
    {
        command->u.start.descriptor = descriptor;
        command->u.start.format = *time_format;
        PropVariantCopy(&command->u.start.position, position);

        hr = MFPutWorkItem(source->async_commands_queue, &source->async_commands_callback, &command->IUnknown_iface);
    }

    return hr;
}

static HRESULT WINAPI media_source_Stop(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    FIXME("(%p): stub\n", source);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    return E_NOTIMPL;
}

static HRESULT WINAPI media_source_Pause(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);

    FIXME("(%p): stub\n", source);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    return E_NOTIMPL;
}

static HRESULT WINAPI media_source_Shutdown(IMFMediaSource *iface)
{
    struct media_source *source = impl_from_IMFMediaSource(iface);
    unsigned int i;

    TRACE("(%p)\n", source);

    if (source->state == SOURCE_SHUTDOWN)
        return MF_E_SHUTDOWN;

    source->state = SOURCE_SHUTDOWN;

    if (source->container)
    {
        gst_element_set_state(source->container, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(source->container));
    }

    if (source->my_src)
        gst_object_unref(GST_OBJECT(source->my_src));
    if (source->their_sink)
        gst_object_unref(GST_OBJECT(source->their_sink));

    if (source->pres_desc)
        IMFPresentationDescriptor_Release(source->pres_desc);
    if (source->event_queue)
        IMFMediaEventQueue_Shutdown(source->event_queue);
    if (source->byte_stream)
        IMFByteStream_Release(source->byte_stream);

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];

        stream->state = STREAM_SHUTDOWN;

        if (stream->my_sink)
            gst_object_unref(GST_OBJECT(stream->my_sink));
        if (stream->event_queue)
            IMFMediaEventQueue_Shutdown(stream->event_queue);
        if (stream->descriptor)
            IMFStreamDescriptor_Release(stream->descriptor);
        if (stream->parent_source)
            IMFMediaSource_Release(&stream->parent_source->IMFMediaSource_iface);

        IMFMediaStream_Release(&stream->IMFMediaStream_iface);
    }

    if (source->stream_count)
        heap_free(source->streams);

    if (source->no_more_pads_event)
        CloseHandle(source->no_more_pads_event);

    if (source->async_commands_queue)
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

static void stream_added(GstElement *element, GstPad *pad, gpointer user)
{
    struct media_source *source = user;
    struct media_stream **new_stream_array;
    struct media_stream *stream;

    if (gst_pad_get_direction(pad) != GST_PAD_SRC)
        return;

    if (FAILED(new_media_stream(source, pad, source->stream_count, &stream)))
        return;

    if (!(new_stream_array = heap_realloc(source->streams, (source->stream_count + 1) * (sizeof(*new_stream_array)))))
    {
        ERR("Failed to add stream to source\n");
        IMFMediaStream_Release(&stream->IMFMediaStream_iface);
        return;
    }

    source->streams = new_stream_array;
    source->streams[source->stream_count++] = stream;
}

static void stream_removed(GstElement *element, GstPad *pad, gpointer user)
{
    struct media_source *source = user;
    unsigned int i;

    for (i = 0; i < source->stream_count; i++)
    {
        struct media_stream *stream = source->streams[i];
        if (stream->their_src != pad)
            continue;
        stream->their_src = NULL;
        stream->state = STREAM_INACTIVE;
    }
}

static void no_more_pads(GstElement *element, gpointer user)
{
    struct media_source *source = user;

    SetEvent(source->no_more_pads_event);
}

static HRESULT media_source_constructor(IMFByteStream *bytestream, struct media_source **out_media_source)
{
    GstStaticPadTemplate src_template =
        GST_STATIC_PAD_TEMPLATE("mf_src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

    struct media_source *object = heap_alloc_zero(sizeof(*object));
    IMFStreamDescriptor **descriptors = NULL;
    unsigned int i;
    HRESULT hr;
    int ret;

    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaSource_iface.lpVtbl = &IMFMediaSource_vtbl;
    object->async_commands_callback.lpVtbl = &source_async_commands_callback_vtbl;
    object->ref = 1;
    object->byte_stream = bytestream;
    IMFByteStream_AddRef(bytestream);
    object->no_more_pads_event = CreateEventA(NULL, FALSE, FALSE, NULL);

    if (FAILED(hr = MFCreateEventQueue(&object->event_queue)))
        goto fail;

    if (FAILED(hr = MFAllocateWorkQueue(&object->async_commands_queue)))
        goto fail;

    object->container = gst_bin_new(NULL);
    object->bus = gst_bus_new();
    gst_bus_set_sync_handler(object->bus, mf_src_bus_watch_wrapper, object, NULL);
    gst_element_set_bus(object->container, object->bus);

    object->my_src = gst_pad_new_from_static_template(&src_template, "mf-src");
    gst_pad_set_element_private(object->my_src, object);
    gst_pad_set_getrange_function(object->my_src, bytestream_wrapper_pull_wrapper);
    gst_pad_set_query_function(object->my_src, bytestream_query_wrapper);
    gst_pad_set_activatemode_function(object->my_src, bytestream_pad_mode_activate_wrapper);
    gst_pad_set_event_function(object->my_src, bytestream_pad_event_process_wrapper);

    if (!(object->decodebin = gst_element_factory_make("decodebin", NULL)))
    {
        WARN("Failed to create decodebin for source\n");
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    /* In Media Foundation, sources may read from any media source stream
       without fear of blocking due to buffering limits on another.  Trailmakers,
       a Unity3D engine game does this by only reading from the audio stream once,
       and never deselecting this.  These properties replicate that behavior.

       Note that with most elements, this causes excessive memory use, however
       this is also what occurs on Windows.
    */
    g_object_set(object->decodebin, "max-size-buffers", 0, NULL);
    g_object_set(object->decodebin, "max-size-time", G_GUINT64_CONSTANT(0), NULL);
    g_object_set(object->decodebin, "max-size-bytes", 0, NULL);

    gst_bin_add(GST_BIN(object->container), object->decodebin);

    g_signal_connect(object->decodebin, "pad-added", G_CALLBACK(mf_src_stream_added_wrapper), object);
    g_signal_connect(object->decodebin, "pad-removed", G_CALLBACK(mf_src_stream_removed_wrapper), object);
    g_signal_connect(object->decodebin, "no-more-pads", G_CALLBACK(mf_src_no_more_pads_wrapper), object);

    object->their_sink = gst_element_get_static_pad(object->decodebin, "sink");

    if ((ret = gst_pad_link(object->my_src, object->their_sink)) < 0)
    {
        WARN("Failed to link our bytestream pad to the demuxer input, error %d.\n", ret);
        hr = E_FAIL;
        goto fail;
    }

    object->state = SOURCE_OPENING;

    gst_element_set_state(object->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(object->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play source, error %d.\n", ret);
        hr = E_FAIL;
        goto fail;
    }

    WaitForSingleObject(object->no_more_pads_event, INFINITE);
    for (i = 0; i < object->stream_count; i++)
    {
        GstSample *preroll;
        g_signal_emit_by_name(object->streams[i]->appsink, "pull-preroll", &preroll);
        if (FAILED(hr = media_stream_init_desc(object->streams[i])))
        {
            ERR("Failed to finish initialization of media stream %p, hr %x.\n", object->streams[i], hr);
            IMFMediaStream_Release(&object->streams[i]->IMFMediaStream_iface);
            goto fail;
        }
        gst_sample_unref(preroll);
    }

    /* init presentation descriptor */

    descriptors = heap_alloc(object->stream_count * sizeof(IMFStreamDescriptor*));
    for (i = 0; i < object->stream_count; i++)
    {
        IMFMediaStream_GetStreamDescriptor(&object->streams[i]->IMFMediaStream_iface, &descriptors[i]);
    }

    if (FAILED(hr = MFCreatePresentationDescriptor(object->stream_count, descriptors, &object->pres_desc)))
        goto fail;

    for (i = 0; i < object->stream_count; i++)
    {
        IMFPresentationDescriptor_SelectStream(object->pres_desc, i);
        IMFStreamDescriptor_Release(descriptors[i]);
    }
    heap_free(descriptors);
    descriptors = NULL;

    object->state = SOURCE_STOPPED;

    *out_media_source = object;
    return S_OK;

    fail:
    WARN("Failed to construct MFMediaSource, hr %#x.\n", hr);

    heap_free(descriptors);
    IMFMediaSource_Release(&object->IMFMediaSource_iface);
    return hr;
}

struct winegstreamer_stream_handler_result
{
    struct list entry;
    IMFAsyncResult *result;
    MF_OBJECT_TYPE obj_type;
    IUnknown *object;
};

struct winegstreamer_stream_handler
{
    IMFByteStreamHandler IMFByteStreamHandler_iface;
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;
    struct list results;
    CRITICAL_SECTION cs;
};

static struct winegstreamer_stream_handler *impl_from_IMFByteStreamHandler(IMFByteStreamHandler *iface)
{
    return CONTAINING_RECORD(iface, struct winegstreamer_stream_handler, IMFByteStreamHandler_iface);
}

static struct winegstreamer_stream_handler *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct winegstreamer_stream_handler, IMFAsyncCallback_iface);
}

static HRESULT WINAPI winegstreamer_stream_handler_QueryInterface(IMFByteStreamHandler *iface, REFIID riid, void **obj)
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

static ULONG WINAPI winegstreamer_stream_handler_AddRef(IMFByteStreamHandler *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);

    TRACE("%p, refcount %u.\n", handler, refcount);

    return refcount;
}

static ULONG WINAPI winegstreamer_stream_handler_Release(IMFByteStreamHandler *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFByteStreamHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);
    struct winegstreamer_stream_handler_result *result, *next;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        LIST_FOR_EACH_ENTRY_SAFE(result, next, &handler->results, struct winegstreamer_stream_handler_result, entry)
        {
            list_remove(&result->entry);
            IMFAsyncResult_Release(result->result);
            if (result->object)
                IUnknown_Release(result->object);
            heap_free(result);
        }
        DeleteCriticalSection(&handler->cs);
        heap_free(handler);
    }

    return refcount;
}

struct create_object_context
{
    IUnknown IUnknown_iface;
    LONG refcount;

    IPropertyStore *props;
    IMFByteStream *stream;
    WCHAR *url;
    DWORD flags;
};

static struct create_object_context *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct create_object_context, IUnknown_iface);
}

static HRESULT WINAPI create_object_context_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
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

static ULONG WINAPI create_object_context_AddRef(IUnknown *iface)
{
    struct create_object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&context->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI create_object_context_Release(IUnknown *iface)
{
    struct create_object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&context->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (context->props)
            IPropertyStore_Release(context->props);
        if (context->stream)
            IMFByteStream_Release(context->stream);
        heap_free(context->url);
        heap_free(context);
    }

    return refcount;
}

static const IUnknownVtbl create_object_context_vtbl =
{
    create_object_context_QueryInterface,
    create_object_context_AddRef,
    create_object_context_Release,
};

static WCHAR *heap_strdupW(const WCHAR *str)
{
    WCHAR *ret = NULL;

    if (str)
    {
        unsigned int size;

        size = (lstrlenW(str) + 1) * sizeof(WCHAR);
        ret = heap_alloc(size);
        if (ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static HRESULT WINAPI winegstreamer_stream_handler_BeginCreateObject(IMFByteStreamHandler *iface, IMFByteStream *stream, const WCHAR *url, DWORD flags,
        IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct winegstreamer_stream_handler *this = impl_from_IMFByteStreamHandler(iface);
    struct create_object_context *context;
    IMFAsyncResult *caller, *item;
    HRESULT hr;

    TRACE("%p, %s, %#x, %p, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, cancel_cookie, callback, state);

    if (cancel_cookie)
        *cancel_cookie = NULL;

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &caller)))
        return hr;

    context = heap_alloc(sizeof(*context));
    if (!context)
    {
        IMFAsyncResult_Release(caller);
        return E_OUTOFMEMORY;
    }

    context->IUnknown_iface.lpVtbl = &create_object_context_vtbl;
    context->refcount = 1;
    context->props = props;
    if (context->props)
        IPropertyStore_AddRef(context->props);
    context->flags = flags;
    context->stream = stream;
    if (context->stream)
        IMFByteStream_AddRef(context->stream);
    if (url)
        context->url = heap_strdupW(url);
    if (!context->stream)
    {
        IMFAsyncResult_Release(caller);
        IUnknown_Release(&context->IUnknown_iface);
        return E_OUTOFMEMORY;
    }

    hr = MFCreateAsyncResult(&context->IUnknown_iface, &this->IMFAsyncCallback_iface, (IUnknown *)caller, &item);
    IUnknown_Release(&context->IUnknown_iface);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_IO, item)))
        {
            if (cancel_cookie)
            {
                *cancel_cookie = (IUnknown *)caller;
                IUnknown_AddRef(*cancel_cookie);
            }
        }

        IMFAsyncResult_Release(item);
    }
    IMFAsyncResult_Release(caller);

    return hr;
}

static HRESULT WINAPI winegstreamer_stream_handler_EndCreateObject(IMFByteStreamHandler *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct winegstreamer_stream_handler *this = impl_from_IMFByteStreamHandler(iface);
    struct winegstreamer_stream_handler_result *found = NULL, *cur;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    EnterCriticalSection(&this->cs);

    LIST_FOR_EACH_ENTRY(cur, &this->results, struct winegstreamer_stream_handler_result, entry)
    {
        if (result == cur->result)
        {
            list_remove(&cur->entry);
            found = cur;
            break;
        }
    }

    LeaveCriticalSection(&this->cs);

    if (found)
    {
        *obj_type = found->obj_type;
        *object = found->object;
        hr = IMFAsyncResult_GetStatus(found->result);
        IMFAsyncResult_Release(found->result);
        heap_free(found);
    }
    else
    {
        *obj_type = MF_OBJECT_INVALID;
        *object = NULL;
        hr = MF_E_UNEXPECTED;
    }

    return hr;
}

static HRESULT WINAPI winegstreamer_stream_handler_CancelObjectCreation(IMFByteStreamHandler *iface, IUnknown *cancel_cookie)
{
    struct winegstreamer_stream_handler *this = impl_from_IMFByteStreamHandler(iface);
    struct winegstreamer_stream_handler_result *found = NULL, *cur;

    TRACE("%p, %p.\n", iface, cancel_cookie);

    EnterCriticalSection(&this->cs);

    LIST_FOR_EACH_ENTRY(cur, &this->results, struct winegstreamer_stream_handler_result, entry)
    {
        if (cancel_cookie == (IUnknown *)cur->result)
        {
            list_remove(&cur->entry);
            found = cur;
            break;
        }
    }

    LeaveCriticalSection(&this->cs);

    if (found)
    {
        IMFAsyncResult_Release(found->result);
        if (found->object)
            IUnknown_Release(found->object);
        heap_free(found);
    }

    return found ? S_OK : MF_E_UNEXPECTED;
}

static HRESULT WINAPI winegstreamer_stream_handler_GetMaxNumberOfBytesRequiredForResolution(IMFByteStreamHandler *iface, QWORD *bytes)
{
    FIXME("stub (%p %p)\n", iface, bytes);
    return E_NOTIMPL;
}

static const IMFByteStreamHandlerVtbl winegstreamer_stream_handler_vtbl =
{
    winegstreamer_stream_handler_QueryInterface,
    winegstreamer_stream_handler_AddRef,
    winegstreamer_stream_handler_Release,
    winegstreamer_stream_handler_BeginCreateObject,
    winegstreamer_stream_handler_EndCreateObject,
    winegstreamer_stream_handler_CancelObjectCreation,
    winegstreamer_stream_handler_GetMaxNumberOfBytesRequiredForResolution,
};

static HRESULT WINAPI winegstreamer_stream_handler_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
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

static ULONG WINAPI winegstreamer_stream_handler_callback_AddRef(IMFAsyncCallback *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFByteStreamHandler_AddRef(&handler->IMFByteStreamHandler_iface);
}

static ULONG WINAPI winegstreamer_stream_handler_callback_Release(IMFAsyncCallback *iface)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFByteStreamHandler_Release(&handler->IMFByteStreamHandler_iface);
}

static HRESULT WINAPI winegstreamer_stream_handler_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT winegstreamer_stream_handler_create_object(struct winegstreamer_stream_handler *This, WCHAR *url, IMFByteStream *stream, DWORD flags,
                                            IPropertyStore *props, IUnknown **out_object, MF_OBJECT_TYPE *out_obj_type)
{
    TRACE("(%p %s %p %u %p %p %p)\n", This, debugstr_w(url), stream, flags, props, out_object, out_obj_type);

    if (flags & MF_RESOLUTION_MEDIASOURCE)
    {
        HRESULT hr;
        struct media_source *new_source;

        if (FAILED(hr = media_source_constructor(stream, &new_source)))
            return hr;

        TRACE("->(%p)\n", new_source);

        *out_object = (IUnknown*)&new_source->IMFMediaSource_iface;
        *out_obj_type = MF_OBJECT_MEDIASOURCE;

        return S_OK;
    }
    else
    {
        FIXME("flags = %08x\n", flags);
        return E_NOTIMPL;
    }
}

static HRESULT WINAPI winegstreamer_stream_handler_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct winegstreamer_stream_handler *handler = impl_from_IMFAsyncCallback(iface);
    struct winegstreamer_stream_handler_result *handler_result;
    MF_OBJECT_TYPE obj_type = MF_OBJECT_INVALID;
    IUnknown *object = NULL, *context_object;
    struct create_object_context *context;
    IMFAsyncResult *caller;
    HRESULT hr;

    caller = (IMFAsyncResult *)IMFAsyncResult_GetStateNoAddRef(result);

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &context_object)))
    {
        WARN("Expected context set for callee result.\n");
        return hr;
    }

    context = impl_from_IUnknown(context_object);

    hr = winegstreamer_stream_handler_create_object(handler, context->url, context->stream, context->flags, context->props, &object, &obj_type);

    handler_result = heap_alloc(sizeof(*handler_result));
    if (handler_result)
    {
        handler_result->result = caller;
        IMFAsyncResult_AddRef(handler_result->result);
        handler_result->obj_type = obj_type;
        handler_result->object = object;

        EnterCriticalSection(&handler->cs);
        list_add_tail(&handler->results, &handler_result->entry);
        LeaveCriticalSection(&handler->cs);
    }
    else
    {
        if (object)
            IUnknown_Release(object);
        hr = E_OUTOFMEMORY;
    }

    IUnknown_Release(&context->IUnknown_iface);

    IMFAsyncResult_SetStatus(caller, hr);
    MFInvokeCallback(caller);

    return S_OK;
}

static const IMFAsyncCallbackVtbl winegstreamer_stream_handler_callback_vtbl =
{
    winegstreamer_stream_handler_callback_QueryInterface,
    winegstreamer_stream_handler_callback_AddRef,
    winegstreamer_stream_handler_callback_Release,
    winegstreamer_stream_handler_callback_GetParameters,
    winegstreamer_stream_handler_callback_Invoke,
};

HRESULT winegstreamer_stream_handler_create(REFIID riid, void **obj)
{
    struct winegstreamer_stream_handler *this;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    this = heap_alloc_zero(sizeof(*this));
    if (!this)
        return E_OUTOFMEMORY;

    list_init(&this->results);
    InitializeCriticalSection(&this->cs);

    this->IMFByteStreamHandler_iface.lpVtbl = &winegstreamer_stream_handler_vtbl;
    this->IMFAsyncCallback_iface.lpVtbl = &winegstreamer_stream_handler_callback_vtbl;
    this->refcount = 1;

    hr = IMFByteStreamHandler_QueryInterface(&this->IMFByteStreamHandler_iface, riid, obj);
    IMFByteStreamHandler_Release(&this->IMFByteStreamHandler_iface);

    return hr;
}

/* helper for callback forwarding */
void perform_cb_media_source(struct cb_data *cbdata)
{
    switch(cbdata->type)
    {
    case BYTESTREAM_WRAPPER_PULL:
        {
            struct getrange_data *data = &cbdata->u.getrange_data;
            cbdata->u.getrange_data.ret = bytestream_wrapper_pull(data->pad, data->parent,
                    data->ofs, data->len, data->buf);
            break;
        }
    case BYTESTREAM_QUERY:
        {
            struct query_function_data *data = &cbdata->u.query_function_data;
            cbdata->u.query_function_data.ret = bytestream_query(data->pad, data->parent, data->query);
            break;
        }
    case BYTESTREAM_PAD_MODE_ACTIVATE:
        {
            struct activate_mode_data *data = &cbdata->u.activate_mode_data;
            cbdata->u.activate_mode_data.ret = bytestream_pad_mode_activate(data->pad, data->parent, data->mode, data->activate);
            break;
        }
    case BYTESTREAM_PAD_EVENT_PROCESS:
        {
            struct event_src_data *data = &cbdata->u.event_src_data;
            cbdata->u.event_src_data.ret = bytestream_pad_event_process(data->pad, data->parent, data->event);
            break;
        }
    case MF_SRC_BUS_WATCH:
        {
            struct watch_bus_data *data = &cbdata->u.watch_bus_data;
            cbdata->u.watch_bus_data.ret = bus_watch(data->bus, data->msg, data->user);
            break;
        }
    case MF_SRC_STREAM_ADDED:
        {
            struct pad_added_data *data = &cbdata->u.pad_added_data;
            stream_added(data->element, data->pad, data->user);
            break;
        }
    case MF_SRC_STREAM_REMOVED:
        {
            struct pad_removed_data *data = &cbdata->u.pad_removed_data;
            stream_removed(data->element, data->pad, data->user);
            break;
        }
    case MF_SRC_NO_MORE_PADS:
        {
            struct no_more_pads_data *data = &cbdata->u.no_more_pads_data;
            no_more_pads(data->element, data->user);
            break;
        }
    default:
        {
            assert(0);
        }
    }
}
