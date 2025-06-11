/*
 * GStreamer parser backend
 *
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 * Copyright 2010 Aric Stewart for CodeWeavers
 * Copyright 2019-2020 Zebediah Figura
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/audio/audio.h>
#include <gst/tag/tag.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "dshow.h"

#include "unix_private.h"

typedef enum
{
    GST_AUTOPLUG_SELECT_TRY,
    GST_AUTOPLUG_SELECT_EXPOSE,
    GST_AUTOPLUG_SELECT_SKIP,
} GstAutoplugSelectResult;

struct wg_parser;

struct input_cache_chunk
{
    guint64 position;
    uint8_t *data;
};

static BOOL decodebin_parser_init_gst(struct wg_parser *parser);

struct wg_parser
{
    struct wg_parser_stream **streams;
    unsigned int stream_count;

    GstElement *container, *decodebin;
    GstBus *bus;
    GstPad *my_src;

    guint64 file_size, start_offset, next_offset, stop_offset;
    guint64 next_pull_offset;
    gchar *uri;

    pthread_t push_thread;

    pthread_mutex_t mutex;

    pthread_cond_t init_cond;
    bool output_compressed;
    bool no_more_pads, has_duration, error;
    bool err_on, warn_on;

    pthread_cond_t read_cond, read_done_cond;
    struct
    {
        GstBuffer *buffer;
        uint64_t offset;
        uint32_t size;
        bool done;
        GstFlowReturn ret;
    } read_request;

    bool sink_connected;

    gchar *sink_caps;

    struct input_cache_chunk input_cache_chunks[4];
};
static const unsigned int input_cache_chunk_size = 512 << 10;

struct wg_parser_stream
{
    struct wg_parser *parser;
    uint32_t number;

    GstPad *my_sink;
    GstElement *flip, *decodebin;
    GstSegment segment;
    GstCaps *codec_caps;
    GstCaps *current_caps;
    GstCaps *desired_caps;

    pthread_cond_t event_cond, event_empty_cond;
    GstBuffer *buffer;
    GstMapInfo map_info;

    bool flushing, eos, enabled, has_tags, has_buffer, no_more_pads;

    uint64_t duration;
    gchar *tags[WG_PARSER_TAG_COUNT];
};

static struct wg_parser *get_parser(wg_parser_t parser)
{
    return (struct wg_parser *)(ULONG_PTR)parser;
}

static struct wg_parser_stream *get_stream(wg_parser_stream_t stream)
{
    return (struct wg_parser_stream *)(ULONG_PTR)stream;
}

static bool caps_is_compressed(GstCaps *caps)
{
    struct wg_format format;

    if (!caps)
        return false;
    wg_format_from_caps(&format, caps);

    return format.major_type != WG_MAJOR_TYPE_UNKNOWN
            && format.major_type != WG_MAJOR_TYPE_VIDEO
            && format.major_type != WG_MAJOR_TYPE_AUDIO;
}

static NTSTATUS wg_parser_get_stream_count(void *args)
{
    struct wg_parser_get_stream_count_params *params = args;

    params->count = get_parser(params->parser)->stream_count;
    return S_OK;
}

static NTSTATUS wg_parser_get_stream(void *args)
{
    struct wg_parser_get_stream_params *params = args;

    params->stream = (wg_parser_stream_t)(ULONG_PTR)get_parser(params->parser)->streams[params->index];
    return S_OK;
}

static NTSTATUS wg_parser_get_next_read_offset(void *args)
{
    struct wg_parser_get_next_read_offset_params *params = args;
    struct wg_parser *parser = get_parser(params->parser);

    pthread_mutex_lock(&parser->mutex);

    while (parser->sink_connected && !parser->read_request.size)
        pthread_cond_wait(&parser->read_cond, &parser->mutex);

    if (!parser->sink_connected)
    {
        pthread_mutex_unlock(&parser->mutex);
        return VFW_E_WRONG_STATE;
    }

    params->offset = parser->read_request.offset;
    params->size = parser->read_request.size;

    pthread_mutex_unlock(&parser->mutex);
    return S_OK;
}

static NTSTATUS wg_parser_push_data(void *args)
{
    const struct wg_parser_push_data_params *params = args;
    struct wg_parser *parser = get_parser(params->parser);
    const void *data = params->data;
    uint32_t size = params->size;

    pthread_mutex_lock(&parser->mutex);

    if (data)
    {
        if (size)
        {
            GstMapInfo map_info;

            /* Note that we don't allocate the buffer until we have a size.
             * midiparse passes a NULL buffer and a size of UINT_MAX, in an
             * apparent attempt to read the whole input stream at once. */
            if (!parser->read_request.buffer)
                parser->read_request.buffer = gst_buffer_new_and_alloc(size);
            gst_buffer_map(parser->read_request.buffer, &map_info, GST_MAP_WRITE);
            memcpy(map_info.data, data, size);
            gst_buffer_unmap(parser->read_request.buffer, &map_info);
            parser->read_request.ret = GST_FLOW_OK;
        }
        else
        {
            parser->read_request.ret = GST_FLOW_EOS;
        }
    }
    else
    {
        parser->read_request.ret = GST_FLOW_ERROR;
    }
    parser->read_request.done = true;
    parser->read_request.size = 0;

    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&parser->read_done_cond);

    return S_OK;
}

static NTSTATUS wg_parser_stream_get_current_format(void *args)
{
    const struct wg_parser_stream_get_current_format_params *params = args;
    struct wg_parser_stream *stream = get_stream(params->stream);

    if (stream->current_caps)
        wg_format_from_caps(params->format, stream->current_caps);
    else
        memset(params->format, 0, sizeof(*params->format));

    return S_OK;
}

static NTSTATUS wg_parser_stream_get_codec_format(void *args)
{
    struct wg_parser_stream_get_codec_format_params *params = args;
    struct wg_parser_stream *stream = get_stream(params->stream);

    if (caps_is_compressed(stream->codec_caps))
        wg_format_from_caps(params->format, stream->codec_caps);
    else if (stream->current_caps)
        wg_format_from_caps(params->format, stream->current_caps);
    else
        memset(params->format, 0, sizeof(*params->format));

    return S_OK;
}

static NTSTATUS wg_parser_stream_enable(void *args)
{
    const struct wg_parser_stream_enable_params *params = args;
    struct wg_parser_stream *stream = get_stream(params->stream);
    const struct wg_format *format = params->format;
    struct wg_parser *parser = stream->parser;

    pthread_mutex_lock(&parser->mutex);

    stream->desired_caps = wg_format_to_caps(format);
    stream->enabled = true;

    pthread_mutex_unlock(&parser->mutex);

    if (format->major_type == WG_MAJOR_TYPE_VIDEO)
    {
        bool flip = (format->u.video.height < 0);

        gst_util_set_object_arg(G_OBJECT(stream->flip), "method", flip ? "vertical-flip" : "none");
    }

    push_event(stream->my_sink, gst_event_new_reconfigure());
    return S_OK;
}

static NTSTATUS wg_parser_stream_disable(void *args)
{
    struct wg_parser_stream *stream = get_stream(*(wg_parser_stream_t *)args);
    struct wg_parser *parser = stream->parser;

    pthread_mutex_lock(&parser->mutex);
    stream->enabled = false;
    if (stream->desired_caps)
    {
        gst_caps_unref(stream->desired_caps);
        stream->desired_caps = NULL;
    }
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&stream->event_empty_cond);
    return S_OK;
}

static GstBuffer *wait_parser_stream_buffer(struct wg_parser *parser, struct wg_parser_stream *stream)
{
    GstBuffer *buffer = NULL;

    /* Note that we can both have a buffer and stream->eos, in which case we
     * must return the buffer. */

    while (stream->enabled && !(buffer = stream->buffer) && !stream->eos)
        pthread_cond_wait(&stream->event_cond, &parser->mutex);

    return buffer;
}

static NTSTATUS wg_parser_stream_get_buffer(void *args)
{
    const struct wg_parser_stream_get_buffer_params *params = args;
    struct wg_parser_buffer *wg_buffer = params->buffer;
    struct wg_parser_stream *stream = get_stream(params->stream);
    struct wg_parser *parser = get_parser(params->parser);
    GstBuffer *buffer;
    unsigned int i;

    pthread_mutex_lock(&parser->mutex);

    if (stream)
        buffer = wait_parser_stream_buffer(parser, stream);
    else
    {
        /* Find the earliest buffer by PTS.
         *
         * Native seems to behave similarly to this with the wm async reader, although our
         * unit tests show that it's not entirely consistent—some frames are received
         * slightly out of order. It's possible that one stream is being manually offset
         * to account for decoding latency.
         *
         * The behaviour with the wm sync reader, when stream 0 is requested, seems
         * consistent with this hypothesis, but with a much larger offset—the video
         * stream seems to be "behind" by about 150 ms.
         *
         * The main reason for doing this is that the video and audio stream probably
         * don't have quite the same "frame rate", and we don't want to force one stream
         * to decode faster just to keep up with the other. Delivering samples in PTS
         * order should avoid that problem. */
        GstBuffer *earliest = NULL;

        for (i = 0; i < parser->stream_count; ++i)
        {
            if (!(buffer = wait_parser_stream_buffer(parser, parser->streams[i])))
                continue;
            /* invalid PTS is GST_CLOCK_TIME_NONE == (guint64)-1, so this will prefer valid timestamps. */
            if (!earliest || GST_BUFFER_PTS(buffer) < GST_BUFFER_PTS(earliest))
            {
                stream = parser->streams[i];
                earliest = buffer;
            }
        }

        buffer = earliest;
    }

    if (!buffer)
    {
        pthread_mutex_unlock(&parser->mutex);
        return S_FALSE;
    }

    /* FIXME: Should we use gst_segment_to_stream_time_full()? Under what
     * circumstances is the stream time not equal to the buffer PTS? Note
     * that this will need modification to wg_parser_stream_notify_qos() as
     * well. */

    if ((wg_buffer->has_pts = GST_BUFFER_PTS_IS_VALID(buffer)))
        wg_buffer->pts = GST_BUFFER_PTS(buffer) / 100;
    if ((wg_buffer->has_duration = GST_BUFFER_DURATION_IS_VALID(buffer)))
        wg_buffer->duration = GST_BUFFER_DURATION(buffer) / 100;
    wg_buffer->discontinuity = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT);
    wg_buffer->preroll = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_LIVE);
    wg_buffer->delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    wg_buffer->size = gst_buffer_get_size(buffer);
    wg_buffer->stream = stream->number;

    pthread_mutex_unlock(&parser->mutex);
    return S_OK;
}

static NTSTATUS wg_parser_stream_copy_buffer(void *args)
{
    const struct wg_parser_stream_copy_buffer_params *params = args;
    struct wg_parser_stream *stream = get_stream(params->stream);
    struct wg_parser *parser = stream->parser;
    uint32_t offset = params->offset;
    uint32_t size = params->size;

    pthread_mutex_lock(&parser->mutex);

    if (!stream->buffer)
    {
        pthread_mutex_unlock(&parser->mutex);
        return VFW_E_WRONG_STATE;
    }

    assert(offset < stream->map_info.size);
    assert(offset + size <= stream->map_info.size);
    memcpy(params->data, stream->map_info.data + offset, size);

    pthread_mutex_unlock(&parser->mutex);
    return S_OK;
}

static NTSTATUS wg_parser_stream_release_buffer(void *args)
{
    struct wg_parser_stream *stream = get_stream(*(wg_parser_stream_t *)args);
    struct wg_parser *parser = stream->parser;

    pthread_mutex_lock(&parser->mutex);

    if (stream->buffer)
    {
        gst_buffer_unmap(stream->buffer, &stream->map_info);
        gst_buffer_unref(stream->buffer);
        stream->buffer = NULL;
    }

    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&stream->event_empty_cond);

    return S_OK;
}

static NTSTATUS wg_parser_stream_get_duration(void *args)
{
    struct wg_parser_stream_get_duration_params *params = args;

    params->duration = get_stream(params->stream)->duration;
    return S_OK;
}

static NTSTATUS wg_parser_stream_get_tag(void *args)
{
    struct wg_parser_stream_get_tag_params *params = args;
    struct wg_parser_stream *stream = get_stream(params->stream);
    uint32_t len;

    if (params->tag >= WG_PARSER_TAG_COUNT)
        return STATUS_INVALID_PARAMETER;
    if (!stream->tags[params->tag])
        return STATUS_NOT_FOUND;
    if ((len = strlen(stream->tags[params->tag]) + 1) > *params->size)
    {
        *params->size = len;
        return STATUS_BUFFER_TOO_SMALL;
    }
    memcpy(params->buffer, stream->tags[params->tag], len);
    return STATUS_SUCCESS;
}

static NTSTATUS wg_parser_stream_seek(void *args)
{
    GstSeekType start_type = GST_SEEK_TYPE_SET, stop_type = GST_SEEK_TYPE_SET;
    const struct wg_parser_stream_seek_params *params = args;
    DWORD start_flags = params->start_flags;
    DWORD stop_flags = params->stop_flags;
    const struct wg_parser_stream *stream;
    GstSeekFlags flags = 0;

    stream = get_stream(params->stream);

    if (start_flags & AM_SEEKING_SeekToKeyFrame)
        flags |= GST_SEEK_FLAG_KEY_UNIT;
    if (start_flags & AM_SEEKING_Segment)
        flags |= GST_SEEK_FLAG_SEGMENT;
    if (!(start_flags & AM_SEEKING_NoFlush))
        flags |= GST_SEEK_FLAG_FLUSH;

    if ((start_flags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
        start_type = GST_SEEK_TYPE_NONE;
    if ((stop_flags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
        stop_type = GST_SEEK_TYPE_NONE;

    if (!push_event(stream->my_sink, gst_event_new_seek(params->rate, GST_FORMAT_TIME,
            flags, start_type, params->start_pos * 100, stop_type,
            params->stop_pos == stream->duration ? -1 : params->stop_pos * 100)))
        GST_ERROR("Failed to seek.");

    return S_OK;
}

static NTSTATUS wg_parser_stream_notify_qos(void *args)
{
    const struct wg_parser_stream_notify_qos_params *params = args;
    struct wg_parser_stream *stream = get_stream(params->stream);
    GstClockTimeDiff diff = params->diff * 100;
    GstClockTime stream_time;
    GstEvent *event;

    /* We return timestamps in stream time, i.e. relative to the start of the
     * file (or other medium), but gst_event_new_qos() expects the timestamp in
     * running time. */
    stream_time = gst_segment_to_running_time(&stream->segment, GST_FORMAT_TIME, params->timestamp * 100);
    if (diff < (GstClockTimeDiff)-stream_time)
        diff = -stream_time;
    if (stream_time == -1)
    {
        /* This can happen legitimately if the sample falls outside of the
         * segment bounds. GStreamer elements shouldn't present the sample in
         * that case, but DirectShow doesn't care. */
        GST_LOG("Ignoring QoS event.");
        return S_OK;
    }
    event = gst_event_new_qos(params->underflow ? GST_QOS_TYPE_UNDERFLOW : GST_QOS_TYPE_OVERFLOW, params->proportion, diff, stream_time);
    push_event(stream->my_sink, event);

    return S_OK;
}

static bool parser_no_more_pads(struct wg_parser *parser)
{
    unsigned int i;

    for (i = 0; i < parser->stream_count; ++i)
    {
        if (!parser->streams[i]->no_more_pads)
            return false;
    }

    return parser->no_more_pads;
}

static gboolean autoplug_continue_cb(GstElement * decodebin, GstPad *pad, GstCaps * caps, gpointer user)
{
    return !caps_is_compressed(caps);
}

static GstAutoplugSelectResult autoplug_select_cb(GstElement *bin, GstPad *pad,
        GstCaps *caps, GstElementFactory *fact, gpointer user)
{
    struct wg_parser *parser = user;
    const char *name = gst_element_factory_get_longname(fact);
    const char *klass = gst_element_factory_get_klass(fact);

    GST_INFO("Using \"%s\".", name);

    if (strstr(name, "Player protection"))
    {
        GST_WARNING("Blacklisted a/52 decoder because it only works in Totem.");
        return GST_AUTOPLUG_SELECT_SKIP;
    }
    if (!strcmp(name, "Fluendo Hardware Accelerated Video Decoder"))
    {
        GST_WARNING("Disabled video acceleration since it breaks in wine.");
        return GST_AUTOPLUG_SELECT_SKIP;
    }

    if (!parser->sink_caps && strstr(klass, GST_ELEMENT_FACTORY_KLASS_DEMUXER))
        parser->sink_caps = g_strdup(gst_structure_get_name(gst_caps_get_structure(caps, 0)));

    return GST_AUTOPLUG_SELECT_TRY;
}

static gboolean autoplug_query_cb(GstElement *bin, GstPad *child,
        GstElement *pad, GstQuery *query, gpointer user)
{
    GstCapsFeatures *features;
    GstCaps *filter, *result;
    GstStructure *structure;
    guint i;

    GST_INFO("Query %"GST_PTR_FORMAT, query);

    if (query->type == GST_QUERY_CAPS)
    {
        result = gst_caps_new_empty();
        gst_query_parse_caps(query, &filter);
        for (i = 0; i < gst_caps_get_size(filter); i++)
        {
            if (!(features = gst_caps_get_features(filter, i))
                    || gst_caps_features_contains(features, GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY))
            {
                structure = gst_caps_get_structure(filter, i);
                gst_caps_append_structure(result, gst_structure_copy(structure));
            }
        }

        GST_INFO("Result %"GST_PTR_FORMAT, result);
        gst_query_set_caps_result(query, result);

        return TRUE;
    }

    return FALSE;
}

static void no_more_pads_cb(GstElement *element, gpointer user)
{
    struct wg_parser *parser = user;

    GST_DEBUG("parser %p.", parser);

    pthread_mutex_lock(&parser->mutex);
    parser->no_more_pads = true;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&parser->init_cond);
}

static void deep_element_added_cb(GstBin *self, GstBin *sub_bin, GstElement *element, gpointer user)
{
    if (element)
        set_max_threads(element);
}

static gboolean sink_event_cb(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct wg_parser_stream *stream = gst_pad_get_element_private(pad);
    struct wg_parser *parser = stream->parser;

    GST_LOG("stream %p, type \"%s\".", stream, GST_EVENT_TYPE_NAME(event));

    switch (event->type)
    {
        case GST_EVENT_SEGMENT:
        {
            const GstSegment *segment;

            gst_event_parse_segment(event, &segment);
            if (segment->format != GST_FORMAT_TIME)
            {
                GST_FIXME("Unhandled format \"%s\".", gst_format_get_name(segment->format));
                break;
            }
            pthread_mutex_lock(&parser->mutex);
            gst_segment_copy_into(segment, &stream->segment);
            pthread_mutex_unlock(&parser->mutex);
            break;
        }

        /* decodebin collects EOS and sends them only when all streams are EOS.
         * In place it sends stream-group-done notifications for individual
         * streams. This is mainly meant to accommodate chained OGGs. However,
         * Windows is generally capable of reading from arbitrary streams while
         * ignoring others, and should still send EOS in that case.
         * Therefore translate stream-group-done back to EOS. */
        case GST_EVENT_STREAM_GROUP_DONE:
        case GST_EVENT_EOS:
            pthread_mutex_lock(&parser->mutex);
            stream->eos = true;
            if (stream->enabled)
                pthread_cond_signal(&stream->event_cond);
            else
                pthread_cond_signal(&parser->init_cond);
            pthread_mutex_unlock(&parser->mutex);
            break;

        case GST_EVENT_FLUSH_START:
            pthread_mutex_lock(&parser->mutex);

            stream->flushing = true;
            pthread_cond_signal(&stream->event_empty_cond);

            if (stream->buffer)
            {
                gst_buffer_unmap(stream->buffer, &stream->map_info);
                gst_buffer_unref(stream->buffer);
                stream->buffer = NULL;
            }

            pthread_mutex_unlock(&parser->mutex);
            break;

        case GST_EVENT_FLUSH_STOP:
        {
            gboolean reset_time;

            gst_event_parse_flush_stop(event, &reset_time);

            if (reset_time)
                gst_segment_init(&stream->segment, GST_FORMAT_UNDEFINED);

            pthread_mutex_lock(&parser->mutex);

            stream->eos = false;
            stream->flushing = false;

            pthread_mutex_unlock(&parser->mutex);
            break;
        }

        case GST_EVENT_CAPS:
        {
            GstCaps *caps;

            gst_event_parse_caps(event, &caps);
            pthread_mutex_lock(&parser->mutex);
            stream->current_caps = gst_caps_ref(caps);
            pthread_mutex_unlock(&parser->mutex);
            pthread_cond_signal(&parser->init_cond);
            break;
        }

        case GST_EVENT_TAG:
            pthread_mutex_lock(&parser->mutex);
            stream->has_tags = true;
            pthread_cond_signal(&parser->init_cond);
            pthread_mutex_unlock(&parser->mutex);
            break;

        default:
            GST_WARNING("Ignoring \"%s\" event.", GST_EVENT_TYPE_NAME(event));
    }
    gst_event_unref(event);
    return TRUE;
}

static GstFlowReturn sink_chain_cb(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    struct wg_parser_stream *stream = gst_pad_get_element_private(pad);
    struct wg_parser *parser = stream->parser;

    GST_LOG("stream %p, buffer %p.", stream, buffer);

    pthread_mutex_lock(&parser->mutex);

    if (!stream->has_buffer)
    {
        stream->has_buffer = true;
        pthread_cond_signal(&parser->init_cond);
    }

    /* Allow this buffer to be flushed by GStreamer. We are effectively
     * implementing a queue object here. */

    while (stream->enabled && !stream->flushing && stream->buffer)
        pthread_cond_wait(&stream->event_empty_cond, &parser->mutex);

    if (!stream->enabled)
    {
        GST_LOG("Stream is disabled; discarding buffer.");
        pthread_mutex_unlock(&parser->mutex);
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }

    if (stream->flushing)
    {
        pthread_mutex_unlock(&parser->mutex);
        GST_DEBUG("Stream is flushing; discarding buffer.");
        gst_buffer_unref(buffer);
        return GST_FLOW_FLUSHING;
    }

    if (!gst_buffer_map(buffer, &stream->map_info, GST_MAP_READ))
    {
        pthread_mutex_unlock(&parser->mutex);
        GST_ERROR("Failed to map buffer.");
        gst_buffer_unref(buffer);
        return GST_FLOW_ERROR;
    }

    stream->buffer = buffer;

    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&stream->event_cond);

    /* The chain callback is given a reference to the buffer. Transfer that
     * reference to the stream object, which will release it in
     * wg_parser_stream_release_buffer(). */

    GST_LOG("Buffer queued.");
    return GST_FLOW_OK;
}

static gboolean sink_query_cb(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct wg_parser_stream *stream = gst_pad_get_element_private(pad);
    struct wg_parser *parser = stream->parser;

    GST_LOG("stream %p, type \"%s\".", stream, gst_query_type_get_name(query->type));

    switch (query->type)
    {
        case GST_QUERY_CAPS:
        {
            GstCaps *caps, *filter, *temp;
            gsize i;

            gst_query_parse_caps(query, &filter);

            pthread_mutex_lock(&parser->mutex);
            if (!stream->desired_caps || !(caps = gst_caps_copy(stream->desired_caps)))
            {
                pthread_mutex_unlock(&parser->mutex);
                return FALSE;
            }
            pthread_mutex_unlock(&parser->mutex);

            /* Clear some fields that shouldn't prevent us from connecting. */
            for (i = 0; i < gst_caps_get_size(caps); ++i)
                gst_structure_remove_fields(gst_caps_get_structure(caps, i),
                        "framerate", "pixel-aspect-ratio", NULL);

            GST_LOG("Stream caps are \"%" GST_PTR_FORMAT "\".", caps);

            if (filter)
            {
                temp = gst_caps_intersect(caps, filter);
                gst_caps_unref(caps);
                caps = temp;
            }

            gst_query_set_caps_result(query, caps);
            gst_caps_unref(caps);
            return TRUE;
        }

        case GST_QUERY_ACCEPT_CAPS:
        {
            struct wg_format format, current_format;
            gboolean ret = TRUE;
            GstCaps *caps;

            pthread_mutex_lock(&parser->mutex);

            if (!stream->desired_caps)
            {
                pthread_mutex_unlock(&parser->mutex);
                gst_query_set_accept_caps_result(query, TRUE);
                return TRUE;
            }

            gst_query_parse_accept_caps(query, &caps);
            wg_format_from_caps(&format, caps);
            wg_format_from_caps(&current_format, stream->desired_caps);
            ret = wg_format_compare(&format, &current_format);

            pthread_mutex_unlock(&parser->mutex);

            if (!ret)
                GST_WARNING("Rejecting caps \"%" GST_PTR_FORMAT "\".", caps);
            gst_query_set_accept_caps_result(query, ret);
            return TRUE;
        }

        default:
            return gst_pad_query_default (pad, parent, query);
    }
}

static struct wg_parser_stream *create_stream(struct wg_parser *parser)
{
    struct wg_parser_stream *stream, **new_array;
    char pad_name[19];

    if (!(new_array = realloc(parser->streams, (parser->stream_count + 1) * sizeof(*parser->streams))))
        return NULL;
    parser->streams = new_array;

    if (!(stream = calloc(1, sizeof(*stream))))
        return NULL;

    gst_segment_init(&stream->segment, GST_FORMAT_UNDEFINED);

    stream->parser = parser;
    stream->number = parser->stream_count;
    stream->no_more_pads = true;
    pthread_cond_init(&stream->event_cond, NULL);
    pthread_cond_init(&stream->event_empty_cond, NULL);

    sprintf(pad_name, "qz_sink_%u", parser->stream_count);
    stream->my_sink = gst_pad_new(pad_name, GST_PAD_SINK);
    gst_pad_set_element_private(stream->my_sink, stream);
    gst_pad_set_chain_function(stream->my_sink, sink_chain_cb);
    gst_pad_set_event_function(stream->my_sink, sink_event_cb);
    gst_pad_set_query_function(stream->my_sink, sink_query_cb);

    parser->streams[parser->stream_count++] = stream;
    return stream;
}

static void free_stream(struct wg_parser_stream *stream)
{
    unsigned int i;

    gst_object_unref(stream->my_sink);

    if (stream->buffer)
    {
        gst_buffer_unmap(stream->buffer, &stream->map_info);
        gst_buffer_unref(stream->buffer);
        stream->buffer = NULL;
    }

    pthread_cond_destroy(&stream->event_cond);
    pthread_cond_destroy(&stream->event_empty_cond);

    for (i = 0; i < ARRAY_SIZE(stream->tags); ++i)
    {
        if (stream->tags[i])
            g_free(stream->tags[i]);
    }
    free(stream);
}

static bool stream_create_post_processing_elements(GstPad *pad, struct wg_parser_stream *stream)
{
    GstElement *element = NULL, *first = NULL, *last = NULL;
    struct wg_parser *parser = stream->parser;
    const char *name;
    GstCaps *caps;

    caps = gst_pad_query_caps(pad, NULL);
    name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    gst_caps_unref(caps);

    if (!strcmp(name, "video/x-raw"))
    {
        /* DirectShow can express interlaced video, but downstream filters can't
         * necessarily consume it. In particular, the video renderer can't. */
        if (!(element = create_element("deinterlace", "good"))
                || !append_element(parser->container, element, &first, &last))
            return false;

        /* decodebin considers many YUV formats to be "raw", but some quartz
         * filters can't handle those. Also, videoflip can't handle all "raw"
         * formats either. Add a videoconvert to swap color spaces. */
        if (!(element = create_element("videoconvert", "base"))
                || !append_element(parser->container, element, &first, &last))
            return false;

        /* Let GStreamer choose a default number of threads. */
        gst_util_set_object_arg(G_OBJECT(element), "n-threads", "0");

        /* GStreamer outputs RGB video top-down, but DirectShow expects bottom-up. */
        if (!(element = create_element("videoflip", "good"))
                || !append_element(parser->container, element, &first, &last))
            return false;
        stream->flip = element;

        /* videoflip does not support 15 and 16-bit RGB so add a second videoconvert
         * to do the final conversion. */
        if (!(element = create_element("videoconvert", "base"))
                || !append_element(parser->container, element, &first, &last))
            return false;

        /* Let GStreamer choose a default number of threads. */
        gst_util_set_object_arg(G_OBJECT(element), "n-threads", "0");

        if (!link_src_to_element(pad, first) || !link_element_to_sink(last, stream->my_sink))
            return false;
    }
    else if (!strcmp(name, "audio/x-raw"))
    {
        /* Currently our dsound can't handle 64-bit formats or all
         * surround-sound configurations. Native dsound can't always handle
         * 64-bit formats either. Add an audioconvert to allow changing bit
         * depth and channel count. */
        if (!(element = create_element("audioconvert", "base"))
                || !append_element(parser->container, element, &first, &last))
            return false;

        if (!link_src_to_element(pad, first) || !link_element_to_sink(last, stream->my_sink))
            return false;
    }
    else
    {
        return link_src_to_sink(pad, stream->my_sink);
    }

    return true;
}

static void stream_decodebin_no_more_pads_cb(GstElement *element, gpointer user)
{
    struct wg_parser_stream *stream = user;
    struct wg_parser *parser = stream->parser;

    GST_DEBUG("stream %p, parser %p, element %p.", stream, parser, element);

    pthread_mutex_lock(&parser->mutex);
    stream->no_more_pads = true;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&parser->init_cond);
}

static void stream_decodebin_pad_added_cb(GstElement *element, GstPad *pad, gpointer user)
{
    struct wg_parser_stream *stream = user;
    struct wg_parser *parser = stream->parser;

    GST_LOG("stream %p, parser %p, element %p, pad %p.", stream, parser, element, pad);

    if (gst_pad_is_linked(pad))
        return;

    if (!stream_create_post_processing_elements(pad, stream))
        return;
    gst_pad_set_active(stream->my_sink, 1);
}

static bool stream_decodebin_create(struct wg_parser_stream *stream)
{
    struct wg_parser *parser = stream->parser;

    GST_LOG("stream %p, parser %p.", stream, parser);

    if (!(stream->decodebin = create_element("decodebin", "base")))
        return false;
    gst_bin_add(GST_BIN(parser->container), stream->decodebin);

    g_signal_connect(stream->decodebin, "pad-added", G_CALLBACK(stream_decodebin_pad_added_cb), stream);
    g_signal_connect(stream->decodebin, "autoplug-select", G_CALLBACK(autoplug_select_cb), stream);
    g_signal_connect(stream->decodebin, "no-more-pads", G_CALLBACK(stream_decodebin_no_more_pads_cb), stream);

    pthread_mutex_lock(&parser->mutex);
    stream->no_more_pads = false;
    pthread_mutex_unlock(&parser->mutex);
    gst_element_sync_state_with_parent(stream->decodebin);

    GST_LOG("Created stream decodebin %p for %u.", stream->decodebin, stream->number);

    return true;
}

static void pad_added_cb(GstElement *element, GstPad *pad, gpointer user)
{
    struct wg_parser_stream *stream;
    struct wg_parser *parser = user;

    GST_LOG("parser %p, element %p, pad %p.", parser, element, pad);

    if (gst_pad_is_linked(pad))
        return;

    if (!(stream = create_stream(parser)))
        return;
    stream->codec_caps = gst_pad_query_caps(pad, NULL);

    /* For compressed stream, create an extra decodebin to decode it. */
    if (!parser->output_compressed && caps_is_compressed(stream->codec_caps))
    {
        if (!stream_decodebin_create(stream))
        {
            GST_ERROR("Failed to create decodebin for stream %u.", stream->number);
            return;
        }

        if (!link_src_to_element(pad, stream->decodebin))
            GST_ERROR("Failed to link pad %p to stream decodebin %p for stream %u.",
                    pad, stream->decodebin, stream->number);

        return;
    }

    if (!stream_create_post_processing_elements(pad, stream))
        return;
    gst_pad_set_active(stream->my_sink, 1);
}

static void pad_removed_cb(GstElement *element, GstPad *pad, gpointer user)
{
    struct wg_parser *parser = user;
    bool done = false;
    unsigned int i;
    char *name;

    GST_LOG("parser %p, element %p, pad %p.", parser, element, pad);

    for (i = 0; i < parser->stream_count; ++i)
    {
        struct wg_parser_stream *stream = parser->streams[i];
        GstPad *stream_decodebin_sink_peer = NULL;
        GstPad *stream_decodebin_sink = NULL;

        if (stream->decodebin)
        {
            stream_decodebin_sink = gst_element_get_static_pad(stream->decodebin, "sink");
            stream_decodebin_sink_peer = gst_pad_get_peer(stream_decodebin_sink);
        }

        if (stream_decodebin_sink_peer == pad)
        {
            gst_pad_unlink(pad, stream_decodebin_sink);
            done = true;
        }

        if (stream_decodebin_sink_peer)
            gst_object_unref(stream_decodebin_sink_peer);
        if (stream_decodebin_sink)
            gst_object_unref(stream_decodebin_sink);

        if (done)
            return;
    }

    name = gst_pad_get_name(pad);
    GST_WARNING("No pin matching pad \"%s\" found.", name);
    g_free(name);
}

static GstFlowReturn issue_read_request(struct wg_parser *parser, guint64 offset, guint size, GstBuffer **buffer)
{
    GstFlowReturn ret;

    pthread_mutex_lock(&parser->mutex);

    assert(!parser->read_request.size);
    parser->read_request.buffer = *buffer;
    parser->read_request.offset = offset;
    parser->read_request.size = size;
    parser->read_request.done = false;
    pthread_cond_signal(&parser->read_cond);

    /* Note that we don't unblock this wait on GST_EVENT_FLUSH_START. We expect
     * the upstream pin to flush if necessary. We should never be blocked on
     * read_thread() not running. */

    while (!parser->read_request.done)
        pthread_cond_wait(&parser->read_done_cond, &parser->mutex);

    *buffer = parser->read_request.buffer;
    ret = parser->read_request.ret;

    pthread_mutex_unlock(&parser->mutex);

    GST_LOG("Request returned %s.", gst_flow_get_name(ret));

    return ret;
}

static struct input_cache_chunk * get_cache_entry(struct wg_parser *parser, guint64 position)
{
    struct input_cache_chunk chunk;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(parser->input_cache_chunks); i++)
    {
        chunk = parser->input_cache_chunks[i];

        if (chunk.data && position == chunk.position)
        {
            if (i != 0)
            {
                memmove(&parser->input_cache_chunks[1], &parser->input_cache_chunks[0], i * sizeof(chunk));
                parser->input_cache_chunks[0] = chunk;
            }

            return &parser->input_cache_chunks[0];
        }
    }

    return NULL;
}

static GstFlowReturn read_cached_chunk(struct wg_parser *parser, guint64 chunk_position, unsigned int chunk_offset, GstBuffer *buffer, guint64 buffer_offset)
{
    struct input_cache_chunk *chunk;
    GstBuffer *chunk_buffer;
    void *chunk_data;
    GstFlowReturn ret;

    if ((chunk = get_cache_entry(parser, chunk_position)))
    {
        if (!!gst_buffer_fill(buffer, buffer_offset, chunk->data + chunk_offset, input_cache_chunk_size - chunk_offset))
            return GST_FLOW_OK;
        else
            return GST_FLOW_ERROR;
    }

    chunk = &parser->input_cache_chunks[ ARRAY_SIZE(parser->input_cache_chunks) - 1 ];

    if (!(chunk_data = chunk->data))
        chunk_data = malloc(input_cache_chunk_size);

    chunk_buffer = gst_buffer_new_wrapped_full(0, chunk_data, input_cache_chunk_size, 0, input_cache_chunk_size, NULL, NULL);
    ret = issue_read_request(parser, chunk_position, input_cache_chunk_size, &chunk_buffer);
    gst_buffer_unref(chunk_buffer);

    if (ret != GST_FLOW_OK)
    {
        if (!chunk->data)
            free(chunk_data);
        return ret;
    }

    memmove(&parser->input_cache_chunks[1], &parser->input_cache_chunks[0], (ARRAY_SIZE(parser->input_cache_chunks) - 1) * sizeof(*chunk));
    parser->input_cache_chunks[0].data = chunk_data;
    parser->input_cache_chunks[0].position = chunk_position;

    chunk = &parser->input_cache_chunks[0];
    if (!!gst_buffer_fill(buffer, buffer_offset, chunk->data + chunk_offset, input_cache_chunk_size - chunk_offset))
        return GST_FLOW_OK;
    else
        return GST_FLOW_ERROR;
}

static GstFlowReturn read_input_cache(struct wg_parser *parser, guint64 offset, guint size, GstBuffer **buffer)
{
    unsigned int i, chunk_count, chunk_offset, buffer_offset = 0;
    GstBuffer *working_buffer;
    guint64 chunk_position;
    GstFlowReturn ret;

    working_buffer = *buffer;
    if (!working_buffer)
        working_buffer = gst_buffer_new_and_alloc(size);

    chunk_position = offset - (offset % input_cache_chunk_size);
    chunk_count = (offset + size + input_cache_chunk_size - chunk_position - 1) / input_cache_chunk_size;
    chunk_offset = offset - chunk_position;

    for (i = 0; i < chunk_count; i++)
    {
        if ((ret = read_cached_chunk(parser, chunk_position, chunk_offset, working_buffer, buffer_offset)) != GST_FLOW_OK)
        {
            if (!*buffer)
                gst_buffer_unref(working_buffer);
            return ret;
        }

        chunk_position += input_cache_chunk_size;
        buffer_offset += input_cache_chunk_size - chunk_offset;
        chunk_offset = 0;
    }

    *buffer = working_buffer;
    return GST_FLOW_OK;
}

static GstFlowReturn src_getrange_cb(GstPad *pad, GstObject *parent,
        guint64 offset, guint size, GstBuffer **buffer)
{
    struct wg_parser *parser = gst_pad_get_element_private(pad);

    GST_LOG("pad %p, offset %" G_GINT64_MODIFIER "u, size %u, buffer %p.", pad, offset, size, *buffer);

    if (offset == GST_BUFFER_OFFSET_NONE)
        offset = parser->next_pull_offset;
    parser->next_pull_offset = offset + size;

    if (!size)
    {
        /* asfreader occasionally asks for zero bytes. gst_buffer_map() will
         * return NULL in this case. Avoid confusing the read thread by asking
         * it for zero bytes. */
        if (!*buffer)
            *buffer = gst_buffer_new_and_alloc(0);
        gst_buffer_set_size(*buffer, 0);
        GST_LOG("Returning empty buffer.");
        return GST_FLOW_OK;
    }

    if (size >= input_cache_chunk_size || sizeof(void*) == 4)
        return issue_read_request(parser, offset, size, buffer);

    if (offset >= parser->file_size)
        return GST_FLOW_EOS;

    if ((offset + size) >= parser->file_size)
        size = parser->file_size - offset;

    return read_input_cache(parser, offset, size, buffer);
}

static gboolean src_query_cb(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct wg_parser *parser = gst_pad_get_element_private(pad);
    GstFormat format;

    GST_LOG("parser %p, type %s.", parser, GST_QUERY_TYPE_NAME(query));

    switch (GST_QUERY_TYPE(query))
    {
        case GST_QUERY_DURATION:
            gst_query_parse_duration(query, &format, NULL);
            if (format == GST_FORMAT_PERCENT)
            {
                gst_query_set_duration(query, GST_FORMAT_PERCENT, GST_FORMAT_PERCENT_MAX);
                return TRUE;
            }
            else if (format == GST_FORMAT_BYTES)
            {
                gst_query_set_duration(query, GST_FORMAT_BYTES, parser->file_size);
                return TRUE;
            }
            return FALSE;

        case GST_QUERY_SEEKING:
            gst_query_parse_seeking (query, &format, NULL, NULL, NULL);
            if (format != GST_FORMAT_BYTES)
            {
                GST_WARNING("Cannot seek using format \"%s\".", gst_format_get_name(format));
                return FALSE;
            }
            gst_query_set_seeking(query, GST_FORMAT_BYTES, 1, 0, parser->file_size);
            return TRUE;

        case GST_QUERY_SCHEDULING:
            gst_query_set_scheduling(query, GST_SCHEDULING_FLAG_SEEKABLE, 1, -1, 0);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PUSH);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PULL);
            return TRUE;

        case GST_QUERY_URI:
            if (parser->uri)
            {
                gst_query_set_uri(query, parser->uri);
                GST_LOG_OBJECT(pad, "Responding with %" GST_PTR_FORMAT, query);
                return TRUE;
            }
            return FALSE;

        default:
            GST_WARNING("Unhandled query type %s.", GST_QUERY_TYPE_NAME(query));
            return FALSE;
    }
}

static void *push_data(void *arg)
{
    struct wg_parser *parser = arg;
    GstBuffer *buffer;
    guint max_size;

    GST_DEBUG("Starting push thread.");

    if (!(buffer = gst_buffer_new_allocate(NULL, 16384, NULL)))
    {
        GST_ERROR("Failed to allocate memory.");
        return NULL;
    }

    max_size = parser->stop_offset ? parser->stop_offset : parser->file_size;

    for (;;)
    {
        ULONG size;
        int ret;

        if (parser->next_offset >= max_size)
            break;
        size = min(16384, max_size - parser->next_offset);

        if ((ret = src_getrange_cb(parser->my_src, NULL, parser->next_offset, size, &buffer)) < 0)
        {
            GST_ERROR("Failed to read data, ret %s.", gst_flow_get_name(ret));
            break;
        }

        parser->next_offset += size;

        buffer->duration = buffer->pts = -1;
        if ((ret = gst_pad_push(parser->my_src, buffer)) < 0)
        {
            GST_ERROR("Failed to push data, ret %s.", gst_flow_get_name(ret));
            break;
        }
    }

    gst_buffer_unref(buffer);

    push_event(parser->my_src, gst_event_new_eos());

    GST_DEBUG("Stopping push thread.");

    return NULL;
}

static gboolean activate_push(GstPad *pad, gboolean activate)
{
    struct wg_parser *parser = gst_pad_get_element_private(pad);

    if (!activate)
    {
        if (parser->push_thread)
        {
            pthread_join(parser->push_thread, NULL);
            parser->push_thread = 0;
        }
    }
    else if (!parser->push_thread)
    {
        int ret;

        if ((ret = pthread_create(&parser->push_thread, NULL, push_data, parser)))
        {
            GST_ERROR("Failed to create push thread: %s", strerror(errno));
            parser->push_thread = 0;
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean src_activate_mode_cb(GstPad *pad, GstObject *parent, GstPadMode mode, gboolean activate)
{
    struct wg_parser *parser = gst_pad_get_element_private(pad);

    GST_DEBUG("%s source pad for parser %p in %s mode.",
            activate ? "Activating" : "Deactivating", parser, gst_pad_mode_get_name(mode));

    switch (mode)
    {
        case GST_PAD_MODE_PULL:
            return TRUE;
        case GST_PAD_MODE_PUSH:
            return activate_push(pad, activate);
        case GST_PAD_MODE_NONE:
            break;
    }
    return FALSE;
}

static GstBusSyncReply bus_handler_cb(GstBus *bus, GstMessage *msg, gpointer user)
{
    struct wg_parser *parser = user;
    gchar *dbg_info = NULL;
    GError *err = NULL;

    GST_DEBUG("parser %p, message type %s.", parser, GST_MESSAGE_TYPE_NAME(msg));

    switch (msg->type)
    {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &dbg_info);
        if (parser->err_on)
        {
            fprintf(stderr, "winegstreamer error: %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            fprintf(stderr, "winegstreamer error: %s: %s\n", GST_OBJECT_NAME(msg->src), dbg_info);
        }
        g_error_free(err);
        g_free(dbg_info);
        pthread_mutex_lock(&parser->mutex);
        parser->error = true;
        pthread_mutex_unlock(&parser->mutex);
        pthread_cond_signal(&parser->init_cond);
        break;

    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(msg, &err, &dbg_info);
        if (parser->warn_on)
        {
            fprintf(stderr, "winegstreamer warning: %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            fprintf(stderr, "winegstreamer warning: %s: %s\n", GST_OBJECT_NAME(msg->src), dbg_info);
        }
        g_error_free(err);
        g_free(dbg_info);
        break;

    case GST_MESSAGE_DURATION_CHANGED:
        pthread_mutex_lock(&parser->mutex);
        parser->has_duration = true;
        pthread_mutex_unlock(&parser->mutex);
        pthread_cond_signal(&parser->init_cond);
        break;

    default:
        break;
    }
    gst_message_unref(msg);
    return GST_BUS_DROP;
}

static gboolean src_perform_seek(struct wg_parser *parser, GstEvent *event)
{
    BOOL thread = !!parser->push_thread;
    GstSeekType cur_type, stop_type;
    GstFormat seek_format;
    GstEvent *flush_event;
    GstSeekFlags flags;
    gint64 cur, stop;
    guint32 seqnum;
    gdouble rate;

    gst_event_parse_seek(event, &rate, &seek_format, &flags,
                         &cur_type, &cur, &stop_type, &stop);

    if (seek_format != GST_FORMAT_BYTES)
    {
        GST_FIXME("Unhandled format \"%s\".", gst_format_get_name(seek_format));
        return FALSE;
    }

    seqnum = gst_event_get_seqnum(event);

    /* send flush start */
    if (flags & GST_SEEK_FLAG_FLUSH)
    {
        flush_event = gst_event_new_flush_start();
        gst_event_set_seqnum(flush_event, seqnum);
        push_event(parser->my_src, flush_event);
        if (thread)
            gst_pad_set_active(parser->my_src, 1);
    }

    parser->next_offset = parser->start_offset = cur;

    /* and prepare to continue streaming */
    if (flags & GST_SEEK_FLAG_FLUSH)
    {
        flush_event = gst_event_new_flush_stop(TRUE);
        gst_event_set_seqnum(flush_event, seqnum);
        push_event(parser->my_src, flush_event);
        if (thread)
            gst_pad_set_active(parser->my_src, 1);
    }

    return TRUE;
}

static gboolean src_event_cb(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct wg_parser *parser = gst_pad_get_element_private(pad);
    gboolean ret = TRUE;

    GST_LOG("parser %p, type \"%s\".", parser, GST_EVENT_TYPE_NAME(event));

    switch (event->type)
    {
        case GST_EVENT_SEEK:
            ret = src_perform_seek(parser, event);
            break;

        case GST_EVENT_FLUSH_START:
        case GST_EVENT_FLUSH_STOP:
        case GST_EVENT_QOS:
        case GST_EVENT_RECONFIGURE:
            break;

        default:
            GST_WARNING("Ignoring \"%s\" event.", GST_EVENT_TYPE_NAME(event));
            ret = FALSE;
            break;
    }
    gst_event_unref(event);
    return ret;
}

static void query_tags(struct wg_parser_stream *stream)
{
    GstPad *peer = gst_pad_get_peer(stream->my_sink);
    const gchar *struct_name;
    GstEvent *tag_event;
    guint i, j;

    stream->tags[WG_PARSER_TAG_NAME]     = NULL;
    stream->tags[WG_PARSER_TAG_LANGUAGE] = NULL;

    i = 0;
    while ((tag_event = gst_pad_get_sticky_event(peer, GST_EVENT_TAG, i++)))
    {
        GstTagList *tag_list;

        gst_event_parse_tag(tag_event, &tag_list);

        if (!stream->tags[WG_PARSER_TAG_NAME])
        {
            /* Extract stream name from Quick Time demuxer private tag where it puts unrecognized chunks. */
            const GValue *val;
            GstSample *sample;
            GstBuffer *buf;
            gsize size;
            guint tag_count = gst_tag_list_get_tag_size(tag_list, "private-qt-tag");

            for (j = 0; j < tag_count; ++j)
            {
                if (!(val = gst_tag_list_get_value_index(tag_list, "private-qt-tag", j)))
                    continue;
                if (!GST_VALUE_HOLDS_SAMPLE(val) || !(sample = gst_value_get_sample(val)))
                    continue;
                struct_name = gst_structure_get_name(gst_sample_get_info(sample));
                if (!struct_name || strcmp(struct_name, "application/x-gst-qt-name-tag"))
                    continue;
                if (!(buf = gst_sample_get_buffer(sample)))
                    continue;
                if ((size = gst_buffer_get_size(buf)) < 8)
                    continue;
                size -= 8;
                if (!(stream->tags[WG_PARSER_TAG_NAME] = g_malloc(size + 1)))
                    continue;
                if (gst_buffer_extract(buf, 8, stream->tags[WG_PARSER_TAG_NAME], size) != size)
                {
                    g_free(stream->tags[WG_PARSER_TAG_NAME]);
                    stream->tags[WG_PARSER_TAG_NAME] = NULL;
                    continue;
                }
                stream->tags[WG_PARSER_TAG_NAME][size] = 0;
            }
        }

        if (!stream->tags[WG_PARSER_TAG_LANGUAGE])
        {
            gchar *lang_code = NULL;

            gst_tag_list_get_string(tag_list, GST_TAG_LANGUAGE_CODE, &lang_code);
            if (stream->parser->sink_caps && !strcmp(stream->parser->sink_caps, "video/quicktime"))
            {
                /* For QuickTime media, we convert the language tags to ISO 639-1. */
                const gchar *lang_code_iso_639_1 = lang_code ? gst_tag_get_language_code_iso_639_1(lang_code) : NULL;
                stream->tags[WG_PARSER_TAG_LANGUAGE] = lang_code_iso_639_1 ? g_strdup(lang_code_iso_639_1) : NULL;
                g_free(lang_code);
            }
            else
                stream->tags[WG_PARSER_TAG_LANGUAGE] = lang_code;
        }

        gst_event_unref(tag_event);
    }
    gst_object_unref(peer);
}

static NTSTATUS wg_parser_connect(void *args)
{
    GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("quartz_src",
            GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
    const struct wg_parser_connect_params *params = args;
    struct wg_parser *parser = get_parser(params->parser);
    const WCHAR *uri = params->uri;
    unsigned int i;
    int ret;

    parser->file_size = params->file_size;
    parser->sink_connected = true;
    if (uri)
    {
        parser->uri = malloc(wcslen(uri) * 3 + 1);
        ntdll_wcstoumbs(uri, wcslen(uri) + 1, parser->uri, wcslen(uri) * 3 + 1, FALSE);
    }
    else
    {
        parser->uri = NULL;
    }

    if (!parser->bus)
    {
        parser->bus = gst_bus_new();
        gst_bus_set_sync_handler(parser->bus, bus_handler_cb, parser, NULL);
    }

    parser->container = gst_bin_new(NULL);
    gst_element_set_bus(parser->container, parser->bus);

    parser->my_src = gst_pad_new_from_static_template(&src_template, "quartz-src");
    gst_pad_set_getrange_function(parser->my_src, src_getrange_cb);
    gst_pad_set_query_function(parser->my_src, src_query_cb);
    gst_pad_set_activatemode_function(parser->my_src, src_activate_mode_cb);
    gst_pad_set_event_function(parser->my_src, src_event_cb);
    gst_pad_set_element_private(parser->my_src, parser);

    parser->start_offset = parser->next_offset = parser->stop_offset = 0;
    parser->next_pull_offset = 0;
    parser->error = false;

    if (!decodebin_parser_init_gst(parser))
        goto out;

    gst_element_set_state(parser->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(parser->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        GST_ERROR("Failed to play stream.");
        goto out;
    }

    pthread_mutex_lock(&parser->mutex);

    while (!parser_no_more_pads(parser) && !parser->error)
        pthread_cond_wait(&parser->init_cond, &parser->mutex);
    if (parser->error)
    {
        pthread_mutex_unlock(&parser->mutex);
        goto out;
    }

    for (i = 0; i < parser->stream_count; ++i)
    {
        struct wg_parser_stream *stream = parser->streams[i];
        gint64 duration;

        /* If we received a buffer, waiting for tags or caps does not make sense anymore. */
        while ((!stream->current_caps || !stream->has_tags) && !parser->error && !stream->has_buffer)
            pthread_cond_wait(&parser->init_cond, &parser->mutex);

        /* GStreamer doesn't actually provide any guarantees about when duration
         * is available, even for seekable streams. It's basically built for
         * applications that don't care, e.g. movie players that can display
         * a duration once it's available, and update it visually if a better
         * estimate is found. This doesn't really match well with DirectShow or
         * Media Foundation, which both expect duration to be available
         * immediately on connecting, so we have to use some complex heuristics
         * to try to actually get a usable duration.
         *
         * Some elements (avidemux, wavparse, qtdemux) record duration almost
         * immediately, before fixing caps. Such elements don't send
         * duration-changed messages. Therefore always try querying duration
         * after caps have been found.
         *
         * Some elements (mpegaudioparse) send duration-changed. In the case of
         * a mp3 stream without seek tables it will not be sent immediately, but
         * only after enough frames have been parsed to form an estimate. They
         * may send it multiple times with increasingly accurate estimates, but
         * unfortunately we have no way of knowing whether another estimate will
         * be sent, so we always take the first one. We assume that if the
         * duration is not immediately available then the element will always
         * send duration-changed.
         */

        for (;;)
        {
            if (parser->error)
            {
                pthread_mutex_unlock(&parser->mutex);
                goto out;
            }
            if (gst_pad_peer_query_duration(stream->my_sink, GST_FORMAT_TIME, &duration))
            {
                stream->duration = duration / 100;
                break;
            }

            if (stream->eos)
            {
                stream->duration = 0;
                GST_WARNING("Failed to query duration.");
                break;
            }

            /* Elements based on GstBaseParse send duration-changed before
             * actually updating the duration in GStreamer versions prior
             * to 1.17.1. See <gstreamer.git:d28e0b4147fe7073b2>. So after
             * receiving duration-changed we have to continue polling until
             * the query succeeds. */
            if (parser->has_duration)
            {
                pthread_mutex_unlock(&parser->mutex);
                g_usleep(10000);
                pthread_mutex_lock(&parser->mutex);
            }
            else
            {
                pthread_cond_wait(&parser->init_cond, &parser->mutex);
            }
        }

        query_tags(stream);

        /* Now that we're fully initialized, enable the stream so that further
         * samples get queued instead of being discarded. We don't actually need
         * the samples (in particular, the frontend should seek before
         * attempting to read anything), but we don't want to waste CPU time
         * trying to decode them. */
        stream->enabled = true;
    }

    pthread_mutex_unlock(&parser->mutex);

    parser->next_offset = 0;
    return S_OK;

out:
    if (parser->container)
        gst_element_set_state(parser->container, GST_STATE_NULL);
    if (parser->my_src)
        gst_object_unref(parser->my_src);

    for (i = 0; i < parser->stream_count; ++i)
        free_stream(parser->streams[i]);
    parser->stream_count = 0;
    free(parser->streams);
    parser->streams = NULL;

    if (parser->container)
    {
        gst_element_set_bus(parser->container, NULL);
        gst_object_unref(parser->container);
        parser->container = NULL;
    }

    g_free(parser->sink_caps);
    parser->sink_caps = NULL;

    pthread_mutex_lock(&parser->mutex);
    parser->sink_connected = false;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&parser->read_cond);

    return E_FAIL;
}

static NTSTATUS wg_parser_disconnect(void *args)
{
    struct wg_parser *parser = get_parser(*(wg_parser_t *)args);
    unsigned int i;

    /* Unblock all of our streams. */
    pthread_mutex_lock(&parser->mutex);
    for (i = 0; i < parser->stream_count; ++i)
    {
        parser->streams[i]->flushing = true;
        parser->streams[i]->eos = true;
        pthread_cond_signal(&parser->streams[i]->event_empty_cond);
        pthread_cond_signal(&parser->streams[i]->event_cond);
    }
    pthread_mutex_unlock(&parser->mutex);

    gst_element_set_state(parser->container, GST_STATE_NULL);
    gst_object_unref(parser->my_src);
    parser->my_src = NULL;

    pthread_mutex_lock(&parser->mutex);
    parser->sink_connected = false;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&parser->read_cond);

    for (i = 0; i < parser->stream_count; ++i)
        free_stream(parser->streams[i]);

    parser->stream_count = 0;
    free(parser->streams);
    parser->streams = NULL;

    gst_element_set_bus(parser->container, NULL);
    gst_object_unref(parser->container);
    parser->container = NULL;

    g_free(parser->sink_caps);
    parser->sink_caps = NULL;

    for (i = 0; i < ARRAY_SIZE(parser->input_cache_chunks); i++)
    {
        free(parser->input_cache_chunks[i].data);
        parser->input_cache_chunks[i].data = NULL;
    }

    return S_OK;
}

static BOOL decodebin_parser_init_gst(struct wg_parser *parser)
{
    GstElement *element;

    if (!(element = create_element("decodebin", "base")))
        return FALSE;

    gst_bin_add(GST_BIN(parser->container), element);
    parser->decodebin = element;

    g_signal_connect(element, "pad-added", G_CALLBACK(pad_added_cb), parser);
    g_signal_connect(element, "pad-removed", G_CALLBACK(pad_removed_cb), parser);
    g_signal_connect(element, "autoplug-continue", G_CALLBACK(autoplug_continue_cb), parser);
    g_signal_connect(element, "autoplug-select", G_CALLBACK(autoplug_select_cb), parser);
    g_signal_connect(element, "autoplug-query", G_CALLBACK(autoplug_query_cb), parser);
    g_signal_connect(element, "no-more-pads", G_CALLBACK(no_more_pads_cb), parser);
    g_signal_connect(element, "deep-element-added", G_CALLBACK(deep_element_added_cb), parser);

    pthread_mutex_lock(&parser->mutex);
    parser->no_more_pads = false;
    pthread_mutex_unlock(&parser->mutex);

    if (!link_src_to_element(parser->my_src, element))
        return FALSE;

    return TRUE;
}


static NTSTATUS wg_parser_create(void *args)
{
    struct wg_parser_create_params *params = args;
    struct wg_parser *parser;

    if (!(parser = calloc(1, sizeof(*parser))))
        return E_OUTOFMEMORY;

    pthread_mutex_init(&parser->mutex, NULL);
    pthread_cond_init(&parser->init_cond, NULL);
    pthread_cond_init(&parser->read_cond, NULL);
    pthread_cond_init(&parser->read_done_cond, NULL);
    parser->output_compressed = params->output_compressed;
    parser->err_on = params->err_on;
    parser->warn_on = params->warn_on;
    GST_DEBUG("Created winegstreamer parser %p.", parser);
    params->parser = (wg_parser_t)(ULONG_PTR)parser;
    return S_OK;
}

static NTSTATUS wg_parser_destroy(void *args)
{
    struct wg_parser *parser = get_parser(*(wg_parser_t *)args);

    if (parser->bus)
    {
        gst_bus_set_sync_handler(parser->bus, NULL, NULL, NULL);
        gst_object_unref(parser->bus);
    }

    pthread_mutex_destroy(&parser->mutex);
    pthread_cond_destroy(&parser->init_cond);
    pthread_cond_destroy(&parser->read_cond);
    pthread_cond_destroy(&parser->read_done_cond);

    free(parser->uri);
    free(parser);
    return S_OK;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
#define X(name) [unix_ ## name] = name
    X(wg_init_gstreamer),

    X(wg_parser_create),
    X(wg_parser_destroy),

    X(wg_parser_connect),
    X(wg_parser_disconnect),

    X(wg_parser_get_next_read_offset),
    X(wg_parser_push_data),

    X(wg_parser_get_stream_count),
    X(wg_parser_get_stream),

    X(wg_parser_stream_get_current_format),
    X(wg_parser_stream_get_codec_format),
    X(wg_parser_stream_enable),
    X(wg_parser_stream_disable),

    X(wg_parser_stream_get_buffer),
    X(wg_parser_stream_copy_buffer),
    X(wg_parser_stream_release_buffer),
    X(wg_parser_stream_notify_qos),

    X(wg_parser_stream_get_duration),
    X(wg_parser_stream_get_tag),
    X(wg_parser_stream_seek),

    X(wg_transform_create),
    X(wg_transform_destroy),
    X(wg_transform_get_output_type),
    X(wg_transform_set_output_type),

    X(wg_transform_push_data),
    X(wg_transform_read_data),
    X(wg_transform_get_status),
    X(wg_transform_drain),
    X(wg_transform_flush),
    X(wg_transform_notify_qos),

    X(wg_muxer_create),
    X(wg_muxer_destroy),
    X(wg_muxer_add_stream),
    X(wg_muxer_start),
    X(wg_muxer_push_sample),
    X(wg_muxer_read_data),
    X(wg_muxer_finalize),
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_wg_funcs_count);

#ifdef _WIN64

typedef ULONG PTR32;

struct wg_media_type32
{
    GUID major;
    UINT32 format_size;
    PTR32 format;
};

static NTSTATUS wow64_wg_parser_connect(void *args)
{
    struct
    {
        wg_parser_t parser;
        PTR32 uri;
        UINT64 file_size;
    } *params32 = args;
    struct wg_parser_connect_params params =
    {
        .parser = params32->parser,
        .uri = ULongToPtr(params32->uri),
        .file_size = params32->file_size,
    };

    return wg_parser_connect(&params);
}

static NTSTATUS wow64_wg_parser_push_data(void *args) {
    struct
    {
        wg_parser_t parser;
        PTR32 data;
        UINT32 size;
    } *params32 = args;
    struct wg_parser_push_data_params params =
    {
        .parser = params32->parser,
        .data = ULongToPtr(params32->data),
        .size = params32->size,
    };

    return wg_parser_push_data(&params);
}

static NTSTATUS wow64_wg_parser_stream_get_current_format(void *args)
{
    struct
    {
        wg_parser_stream_t stream;
        PTR32 format;
    } *params32 = args;
    struct wg_parser_stream_get_current_format_params params =
    {
        .stream = params32->stream,
        .format = ULongToPtr(params32->format),
    };

    return wg_parser_stream_get_current_format(&params);
}

static NTSTATUS wow64_wg_parser_stream_get_codec_format(void *args)
{
    struct
    {
        wg_parser_stream_t stream;
        PTR32 format;
    } *params32 = args;
    struct wg_parser_stream_get_codec_format_params params =
    {
        .stream = params32->stream,
        .format = ULongToPtr(params32->format),
    };

    return wg_parser_stream_get_codec_format(&params);
}

static NTSTATUS wow64_wg_parser_stream_enable(void *args)
{
    struct
    {
        wg_parser_stream_t stream;
        PTR32 format;
    } *params32 = args;
    struct wg_parser_stream_enable_params params =
    {
        .stream = params32->stream,
        .format = ULongToPtr(params32->format),
    };

    return wg_parser_stream_enable(&params);
}

static NTSTATUS wow64_wg_parser_stream_get_buffer(void *args)
{
    struct
    {
        wg_parser_t parser;
        wg_parser_stream_t stream;
        PTR32 buffer;
    } *params32 = args;
    struct wg_parser_stream_get_buffer_params params =
    {
        .parser = params32->parser,
        .stream = params32->stream,
        .buffer = ULongToPtr(params32->buffer),
    };
    return wg_parser_stream_get_buffer(&params);
}

static NTSTATUS wow64_wg_parser_stream_copy_buffer(void *args)
{
    struct
    {
        wg_parser_stream_t stream;
        PTR32 data;
        UINT32 offset;
        UINT32 size;
    } *params32 = args;
    struct wg_parser_stream_copy_buffer_params params =
    {
        .stream = params32->stream,
        .data = ULongToPtr(params32->data),
        .offset = params32->offset,
        .size = params32->size,
    };
    return wg_parser_stream_copy_buffer(&params);
}

static NTSTATUS wow64_wg_parser_stream_get_tag(void *args)
{
    struct
    {
        wg_parser_stream_t stream;
        wg_parser_tag tag;
        PTR32 buffer;
        PTR32 size;
    } *params32 = args;
    struct wg_parser_stream_get_tag_params params =
    {
        .stream = params32->stream,
        .tag = params32->tag,
        .buffer = ULongToPtr(params32->buffer),
        .size = ULongToPtr(params32->size),
    };

    return wg_parser_stream_get_tag(&params);
}

NTSTATUS wow64_wg_transform_create(void *args)
{
    struct
    {
        wg_transform_t transform;
        struct wg_media_type32 input_type;
        struct wg_media_type32 output_type;
        struct wg_transform_attrs attrs;
    } *params32 = args;
    struct wg_transform_create_params params =
    {
        .input_type =
        {
            .major = params32->input_type.major,
            .format_size = params32->input_type.format_size,
            .u.format = ULongToPtr(params32->input_type.format),
        },
        .output_type =
        {
            .major = params32->output_type.major,
            .format_size = params32->output_type.format_size,
            .u.format = ULongToPtr(params32->output_type.format),
        },
        .attrs = params32->attrs,
    };
    NTSTATUS ret;

    ret = wg_transform_create(&params);
    params32->transform = params.transform;
    return ret;
}

NTSTATUS wow64_wg_transform_get_output_type(void *args)
{
    struct
    {
        wg_transform_t transform;
        struct wg_media_type32 media_type;
    } *params32 = args;
    struct wg_transform_get_output_type_params params =
    {
        .transform = params32->transform,
        .media_type =
        {
            .major = params32->media_type.major,
            .format_size = params32->media_type.format_size,
            .u.format = ULongToPtr(params32->media_type.format),
        },
    };
    NTSTATUS status;

    status = wg_transform_get_output_type(&params);
    params32->media_type.major = params.media_type.major;
    params32->media_type.format_size = params.media_type.format_size;
    return status;
}

NTSTATUS wow64_wg_transform_set_output_type(void *args)
{
    struct
    {
        wg_transform_t transform;
        struct wg_media_type32 media_type;
    } *params32 = args;
    struct wg_transform_set_output_type_params params =
    {
        .transform = params32->transform,
        .media_type =
        {
            .major = params32->media_type.major,
            .format_size = params32->media_type.format_size,
            .u.format = ULongToPtr(params32->media_type.format),
        },
    };
    return wg_transform_set_output_type(&params);
}

NTSTATUS wow64_wg_transform_push_data(void *args)
{
    struct
    {
        wg_transform_t transform;
        PTR32 sample;
        HRESULT result;
    } *params32 = args;
    struct wg_transform_push_data_params params =
    {
        .transform = params32->transform,
        .sample = ULongToPtr(params32->sample),
    };
    NTSTATUS ret;

    ret = wg_transform_push_data(&params);
    params32->result = params.result;
    return ret;
}

NTSTATUS wow64_wg_transform_read_data(void *args)
{
    struct
    {
        wg_transform_t transform;
        PTR32 sample;
        HRESULT result;
    } *params32 = args;
    struct wg_transform_read_data_params params =
    {
        .transform = params32->transform,
        .sample = ULongToPtr(params32->sample),
    };
    NTSTATUS ret;

    ret = wg_transform_read_data(&params);
    params32->result = params.result;
    return ret;
}

NTSTATUS wow64_wg_muxer_create(void *args)
{
    struct
    {
        wg_muxer_t muxer;
        PTR32 format;
    } *params32 = args;
    struct wg_muxer_create_params params =
    {
        .format = ULongToPtr(params32->format),
    };
    NTSTATUS ret;

    ret = wg_muxer_create(&params);
    params32->muxer = params.muxer;
    return ret;
}

NTSTATUS wow64_wg_muxer_add_stream(void *args)
{
    struct
    {
        wg_muxer_t muxer;
        UINT32 stream_id;
        PTR32 format;
    } *params32 = args;
    struct wg_muxer_add_stream_params params =
    {
        .muxer = params32->muxer,
        .stream_id = params32->stream_id,
        .format = ULongToPtr(params32->format),
    };
    return wg_muxer_add_stream(&params);
}

NTSTATUS wow64_wg_muxer_push_sample(void *args)
{
    struct
    {
        wg_muxer_t muxer;
        PTR32 sample;
        UINT32 stream_id;
    } *params32 = args;
    struct wg_muxer_push_sample_params params =
    {
        .muxer = params32->muxer,
        .sample = ULongToPtr(params32->sample),
        .stream_id = params32->stream_id,
    };
    return wg_muxer_push_sample(&params);
}

NTSTATUS wow64_wg_muxer_read_data(void *args)
{
    struct
    {
        wg_muxer_t muxer;
        PTR32 buffer;
        UINT32 size;
        UINT64 offset;
    } *params32 = args;
    struct wg_muxer_read_data_params params =
    {
        .muxer = params32->muxer,
        .buffer = ULongToPtr(params32->buffer),
        .size = params32->size,
        .offset = params32->offset,
    };
    NTSTATUS ret;

    ret = wg_muxer_read_data(&params);
    params32->size = params.size;
    params32->offset = params.offset;
    return ret;
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
#define X64(name) [unix_ ## name] = wow64_ ## name
    X(wg_init_gstreamer),

    X(wg_parser_create),
    X(wg_parser_destroy),

    X64(wg_parser_connect),
    X(wg_parser_disconnect),

    X(wg_parser_get_next_read_offset),
    X64(wg_parser_push_data),

    X(wg_parser_get_stream_count),
    X(wg_parser_get_stream),

    X64(wg_parser_stream_get_current_format),
    X64(wg_parser_stream_get_codec_format),
    X64(wg_parser_stream_enable),
    X(wg_parser_stream_disable),

    X64(wg_parser_stream_get_buffer),
    X64(wg_parser_stream_copy_buffer),
    X(wg_parser_stream_release_buffer),
    X(wg_parser_stream_notify_qos),

    X(wg_parser_stream_get_duration),
    X64(wg_parser_stream_get_tag),
    X(wg_parser_stream_seek),

    X64(wg_transform_create),
    X(wg_transform_destroy),
    X64(wg_transform_get_output_type),
    X64(wg_transform_set_output_type),

    X64(wg_transform_push_data),
    X64(wg_transform_read_data),
    X(wg_transform_get_status),
    X(wg_transform_drain),
    X(wg_transform_flush),
    X(wg_transform_notify_qos),

    X64(wg_muxer_create),
    X(wg_muxer_destroy),
    X64(wg_muxer_add_stream),
    X(wg_muxer_start),
    X64(wg_muxer_push_sample),
    X64(wg_muxer_read_data),
    X(wg_muxer_finalize),
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_wg_funcs_count);

#endif  /* _WIN64 */
