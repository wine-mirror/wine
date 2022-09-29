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

#include "winternl.h"
#include "dshow.h"

#include "unix_private.h"

typedef enum
{
    GST_AUTOPLUG_SELECT_TRY,
    GST_AUTOPLUG_SELECT_EXPOSE,
    GST_AUTOPLUG_SELECT_SKIP,
} GstAutoplugSelectResult;

/* GStreamer callbacks may be called on threads not created by Wine, and
 * therefore cannot access the Wine TEB. This means that we must use GStreamer
 * debug logging instead of Wine debug logging. In order to be safe we forbid
 * any use of Wine debug logging in this entire file. */

GST_DEBUG_CATEGORY(wine);
#define GST_CAT_DEFAULT wine

typedef BOOL (*init_gst_cb)(struct wg_parser *parser);

struct wg_parser
{
    init_gst_cb init_gst;

    struct wg_parser_stream **streams;
    unsigned int stream_count;

    GstElement *container, *decodebin;
    GstBus *bus;
    GstPad *my_src, *their_sink;

    guint64 file_size, start_offset, next_offset, stop_offset;
    guint64 next_pull_offset;

    pthread_t push_thread;

    pthread_mutex_t mutex;

    pthread_cond_t init_cond;
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

    bool unlimited_buffering;
};

struct wg_parser_stream
{
    struct wg_parser *parser;

    GstPad *their_src, *post_sink, *post_src, *my_sink;
    GstElement *flip;
    GstSegment segment;
    struct wg_format preferred_format, current_format;

    pthread_cond_t event_cond, event_empty_cond;
    GstBuffer *buffer;
    GstMapInfo map_info;

    bool flushing, eos, enabled, has_caps;

    uint64_t duration;
};

static NTSTATUS wg_parser_get_stream_count(void *args)
{
    struct wg_parser_get_stream_count_params *params = args;

    params->count = params->parser->stream_count;
    return S_OK;
}

static NTSTATUS wg_parser_get_stream(void *args)
{
    struct wg_parser_get_stream_params *params = args;

    params->stream = params->parser->streams[params->index];
    return S_OK;
}

static NTSTATUS wg_parser_get_next_read_offset(void *args)
{
    struct wg_parser_get_next_read_offset_params *params = args;
    struct wg_parser *parser = params->parser;

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
    struct wg_parser *parser = params->parser;
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

static NTSTATUS wg_parser_stream_get_preferred_format(void *args)
{
    const struct wg_parser_stream_get_preferred_format_params *params = args;

    *params->format = params->stream->preferred_format;
    return S_OK;
}

static NTSTATUS wg_parser_stream_enable(void *args)
{
    const struct wg_parser_stream_enable_params *params = args;
    struct wg_parser_stream *stream = params->stream;
    const struct wg_format *format = params->format;
    struct wg_parser *parser = stream->parser;

    pthread_mutex_lock(&parser->mutex);

    stream->current_format = *format;
    stream->enabled = true;

    pthread_mutex_unlock(&parser->mutex);

    if (format->major_type == WG_MAJOR_TYPE_VIDEO)
    {
        bool flip = (format->u.video.height < 0);

        switch (format->u.video.format)
        {
            case WG_VIDEO_FORMAT_BGRA:
            case WG_VIDEO_FORMAT_BGRx:
            case WG_VIDEO_FORMAT_BGR:
            case WG_VIDEO_FORMAT_RGB15:
            case WG_VIDEO_FORMAT_RGB16:
                flip = !flip;
                break;

            case WG_VIDEO_FORMAT_AYUV:
            case WG_VIDEO_FORMAT_I420:
            case WG_VIDEO_FORMAT_NV12:
            case WG_VIDEO_FORMAT_UYVY:
            case WG_VIDEO_FORMAT_YUY2:
            case WG_VIDEO_FORMAT_YV12:
            case WG_VIDEO_FORMAT_YVYU:
            case WG_VIDEO_FORMAT_UNKNOWN:
            case WG_VIDEO_FORMAT_CINEPAK:
                break;
        }

        gst_util_set_object_arg(G_OBJECT(stream->flip), "method", flip ? "vertical-flip" : "none");
    }

    gst_pad_push_event(stream->my_sink, gst_event_new_reconfigure());
    return S_OK;
}

static NTSTATUS wg_parser_stream_disable(void *args)
{
    struct wg_parser_stream *stream = args;
    struct wg_parser *parser = stream->parser;

    pthread_mutex_lock(&parser->mutex);
    stream->enabled = false;
    stream->current_format.major_type = WG_MAJOR_TYPE_UNKNOWN;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&stream->event_empty_cond);
    return S_OK;
}

static NTSTATUS wg_parser_stream_get_buffer(void *args)
{
    const struct wg_parser_stream_get_buffer_params *params = args;
    struct wg_parser_buffer *wg_buffer = params->buffer;
    struct wg_parser_stream *stream = params->stream;
    struct wg_parser *parser = stream->parser;
    GstBuffer *buffer;

    pthread_mutex_lock(&parser->mutex);

    while (!stream->eos && !stream->buffer)
        pthread_cond_wait(&stream->event_cond, &parser->mutex);

    /* Note that we can both have a buffer and stream->eos, in which case we
     * must return the buffer. */
    if ((buffer = stream->buffer))
    {
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

        pthread_mutex_unlock(&parser->mutex);
        return S_OK;
    }

    pthread_mutex_unlock(&parser->mutex);
    return S_FALSE;
}

static NTSTATUS wg_parser_stream_copy_buffer(void *args)
{
    const struct wg_parser_stream_copy_buffer_params *params = args;
    struct wg_parser_stream *stream = params->stream;
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
    struct wg_parser_stream *stream = args;
    struct wg_parser *parser = stream->parser;

    pthread_mutex_lock(&parser->mutex);

    assert(stream->buffer);

    gst_buffer_unmap(stream->buffer, &stream->map_info);
    gst_buffer_unref(stream->buffer);
    stream->buffer = NULL;

    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&stream->event_empty_cond);

    return S_OK;
}

static NTSTATUS wg_parser_stream_get_duration(void *args)
{
    struct wg_parser_stream_get_duration_params *params = args;

    params->duration = params->stream->duration;
    return S_OK;
}

static NTSTATUS wg_parser_stream_seek(void *args)
{
    GstSeekType start_type = GST_SEEK_TYPE_SET, stop_type = GST_SEEK_TYPE_SET;
    const struct wg_parser_stream_seek_params *params = args;
    DWORD start_flags = params->start_flags;
    DWORD stop_flags = params->stop_flags;
    GstSeekFlags flags = 0;

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

    if (!gst_pad_push_event(params->stream->my_sink, gst_event_new_seek(params->rate, GST_FORMAT_TIME,
            flags, start_type, params->start_pos * 100, stop_type, params->stop_pos * 100)))
        GST_ERROR("Failed to seek.\n");

    return S_OK;
}

static NTSTATUS wg_parser_stream_notify_qos(void *args)
{
    const struct wg_parser_stream_notify_qos_params *params = args;
    struct wg_parser_stream *stream = params->stream;
    GstClockTime stream_time;
    GstEvent *event;

    /* We return timestamps in stream time, i.e. relative to the start of the
     * file (or other medium), but gst_event_new_qos() expects the timestamp in
     * running time. */
    stream_time = gst_segment_to_running_time(&stream->segment, GST_FORMAT_TIME, params->timestamp * 100);
    if (stream_time == -1)
    {
        /* This can happen legitimately if the sample falls outside of the
         * segment bounds. GStreamer elements shouldn't present the sample in
         * that case, but DirectShow doesn't care. */
        GST_LOG("Ignoring QoS event.\n");
        return S_OK;
    }

    if (!(event = gst_event_new_qos(params->underflow ? GST_QOS_TYPE_UNDERFLOW : GST_QOS_TYPE_OVERFLOW,
            params->proportion, params->diff * 100, stream_time)))
        GST_ERROR("Failed to create QOS event.\n");
    gst_pad_push_event(stream->my_sink, event);

    return S_OK;
}

static GstAutoplugSelectResult autoplug_select_cb(GstElement *bin, GstPad *pad,
        GstCaps *caps, GstElementFactory *fact, gpointer user)
{
    const char *name = gst_element_factory_get_longname(fact);

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
    return GST_AUTOPLUG_SELECT_TRY;
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

static gboolean sink_event_cb(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct wg_parser_stream *stream = gst_pad_get_element_private(pad);
    struct wg_parser *parser = stream->parser;

    GST_LOG("stream %p, type \"%s\".", stream, GST_EVENT_TYPE_NAME(event));

    switch (event->type)
    {
        case GST_EVENT_SEGMENT:
            pthread_mutex_lock(&parser->mutex);
            if (stream->enabled)
            {
                const GstSegment *segment;

                gst_event_parse_segment(event, &segment);

                if (segment->format != GST_FORMAT_TIME)
                {
                    pthread_mutex_unlock(&parser->mutex);
                    GST_FIXME("Unhandled format \"%s\".", gst_format_get_name(segment->format));
                    break;
                }

                gst_segment_copy_into(segment, &stream->segment);
            }
            pthread_mutex_unlock(&parser->mutex);
            break;

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

            if (stream->enabled)
            {
                stream->flushing = true;
                pthread_cond_signal(&stream->event_empty_cond);

                if (stream->buffer)
                {
                    gst_buffer_unmap(stream->buffer, &stream->map_info);
                    gst_buffer_unref(stream->buffer);
                    stream->buffer = NULL;
                }
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
            if (stream->enabled)
                stream->flushing = false;

            pthread_mutex_unlock(&parser->mutex);
            break;
        }

        case GST_EVENT_CAPS:
        {
            GstCaps *caps;

            gst_event_parse_caps(event, &caps);
            pthread_mutex_lock(&parser->mutex);
            wg_format_from_caps(&stream->preferred_format, caps);
            stream->has_caps = true;
            pthread_mutex_unlock(&parser->mutex);
            pthread_cond_signal(&parser->init_cond);
            break;
        }

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

    /* Allow this buffer to be flushed by GStreamer. We are effectively
     * implementing a queue object here. */

    while (stream->enabled && !stream->flushing && stream->buffer)
        pthread_cond_wait(&stream->event_empty_cond, &parser->mutex);

    if (!stream->enabled)
    {
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
        GST_ERROR("Failed to map buffer.\n");
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
            gchar *str;
            gsize i;

            gst_query_parse_caps(query, &filter);

            pthread_mutex_lock(&parser->mutex);
            caps = wg_format_to_caps(&stream->current_format);
            pthread_mutex_unlock(&parser->mutex);

            if (!caps)
                return FALSE;

            /* Clear some fields that shouldn't prevent us from connecting. */
            for (i = 0; i < gst_caps_get_size(caps); ++i)
                gst_structure_remove_fields(gst_caps_get_structure(caps, i),
                        "framerate", "pixel-aspect-ratio", "colorimetry", "chroma-site", NULL);

            str = gst_caps_to_string(caps);
            GST_LOG("Stream caps are \"%s\".", str);
            g_free(str);

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
            struct wg_format format;
            gboolean ret = TRUE;
            GstCaps *caps;

            pthread_mutex_lock(&parser->mutex);

            if (stream->current_format.major_type == WG_MAJOR_TYPE_UNKNOWN)
            {
                pthread_mutex_unlock(&parser->mutex);
                gst_query_set_accept_caps_result(query, TRUE);
                return TRUE;
            }

            gst_query_parse_accept_caps(query, &caps);
            wg_format_from_caps(&format, caps);
            ret = wg_format_compare(&format, &stream->current_format);

            pthread_mutex_unlock(&parser->mutex);

            if (!ret && gst_debug_category_get_threshold(GST_CAT_DEFAULT) >= GST_LEVEL_WARNING)
            {
                gchar *str = gst_caps_to_string(caps);
                GST_WARNING("Rejecting caps \"%s\".", str);
                g_free(str);
            }
            gst_query_set_accept_caps_result(query, ret);
            return TRUE;
        }

        default:
            return gst_pad_query_default (pad, parent, query);
    }
}

GstElement *create_element(const char *name, const char *plugin_set)
{
    GstElement *element;

    if (!(element = gst_element_factory_make(name, NULL)))
        fprintf(stderr, "winegstreamer: failed to create %s, are %u-bit GStreamer \"%s\" plugins installed?\n",
                name, 8 * (unsigned int)sizeof(void *), plugin_set);
    return element;
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
    stream->current_format.major_type = WG_MAJOR_TYPE_UNKNOWN;
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
    if (stream->their_src)
    {
        if (stream->post_sink)
        {
            gst_object_unref(stream->post_src);
            gst_object_unref(stream->post_sink);
            stream->post_src = stream->post_sink = NULL;
        }
        gst_object_unref(stream->their_src);
    }
    gst_object_unref(stream->my_sink);

    pthread_cond_destroy(&stream->event_cond);
    pthread_cond_destroy(&stream->event_empty_cond);

    free(stream);
}

static void pad_added_cb(GstElement *element, GstPad *pad, gpointer user)
{
    struct wg_parser *parser = user;
    struct wg_parser_stream *stream;
    const char *name;
    GstCaps *caps;
    int ret;

    GST_LOG("parser %p, element %p, pad %p.", parser, element, pad);

    if (gst_pad_is_linked(pad))
        return;

    caps = gst_pad_query_caps(pad, NULL);
    name = gst_structure_get_name(gst_caps_get_structure(caps, 0));

    if (!(stream = create_stream(parser)))
        goto out;

    if (!strcmp(name, "video/x-raw"))
    {
        GstElement *deinterlace, *vconv, *flip, *vconv2;

        /* DirectShow can express interlaced video, but downstream filters can't
         * necessarily consume it. In particular, the video renderer can't. */
        if (!(deinterlace = create_element("deinterlace", "good")))
            goto out;

        /* decodebin considers many YUV formats to be "raw", but some quartz
         * filters can't handle those. Also, videoflip can't handle all "raw"
         * formats either. Add a videoconvert to swap color spaces. */
        if (!(vconv = create_element("videoconvert", "base")))
            goto out;

        /* GStreamer outputs RGB video top-down, but DirectShow expects bottom-up. */
        if (!(flip = create_element("videoflip", "good")))
            goto out;

        /* videoflip does not support 15 and 16-bit RGB so add a second videoconvert
         * to do the final conversion. */
        if (!(vconv2 = create_element("videoconvert", "base")))
            goto out;

        /* The bin takes ownership of these elements. */
        gst_bin_add(GST_BIN(parser->container), deinterlace);
        gst_element_sync_state_with_parent(deinterlace);
        gst_bin_add(GST_BIN(parser->container), vconv);
        gst_element_sync_state_with_parent(vconv);
        gst_bin_add(GST_BIN(parser->container), flip);
        gst_element_sync_state_with_parent(flip);
        gst_bin_add(GST_BIN(parser->container), vconv2);
        gst_element_sync_state_with_parent(vconv2);

        gst_element_link(deinterlace, vconv);
        gst_element_link(vconv, flip);
        gst_element_link(flip, vconv2);

        stream->post_sink = gst_element_get_static_pad(deinterlace, "sink");
        stream->post_src = gst_element_get_static_pad(vconv2, "src");
        stream->flip = flip;
    }
    else if (!strcmp(name, "audio/x-raw"))
    {
        GstElement *convert;

        /* Currently our dsound can't handle 64-bit formats or all
         * surround-sound configurations. Native dsound can't always handle
         * 64-bit formats either. Add an audioconvert to allow changing bit
         * depth and channel count. */
        if (!(convert = create_element("audioconvert", "base")))
            goto out;

        gst_bin_add(GST_BIN(parser->container), convert);
        gst_element_sync_state_with_parent(convert);

        stream->post_sink = gst_element_get_static_pad(convert, "sink");
        stream->post_src = gst_element_get_static_pad(convert, "src");
    }

    if (stream->post_sink)
    {
        if ((ret = gst_pad_link(pad, stream->post_sink)) < 0)
        {
            GST_ERROR("Failed to link decodebin source pad to post-processing elements, error %s.",
                    gst_pad_link_get_name(ret));
            gst_object_unref(stream->post_sink);
            stream->post_sink = NULL;
            goto out;
        }

        if ((ret = gst_pad_link(stream->post_src, stream->my_sink)) < 0)
        {
            GST_ERROR("Failed to link post-processing elements to our sink pad, error %s.",
                    gst_pad_link_get_name(ret));
            gst_object_unref(stream->post_src);
            stream->post_src = NULL;
            gst_object_unref(stream->post_sink);
            stream->post_sink = NULL;
            goto out;
        }
    }
    else if ((ret = gst_pad_link(pad, stream->my_sink)) < 0)
    {
        GST_ERROR("Failed to link decodebin source pad to our sink pad, error %s.",
                gst_pad_link_get_name(ret));
        goto out;
    }

    gst_pad_set_active(stream->my_sink, 1);
    gst_object_ref(stream->their_src = pad);
out:
    gst_caps_unref(caps);
}

static void pad_removed_cb(GstElement *element, GstPad *pad, gpointer user)
{
    struct wg_parser *parser = user;
    unsigned int i;
    char *name;

    GST_LOG("parser %p, element %p, pad %p.", parser, element, pad);

    for (i = 0; i < parser->stream_count; ++i)
    {
        struct wg_parser_stream *stream = parser->streams[i];

        if (stream->their_src == pad)
        {
            if (stream->post_sink)
                gst_pad_unlink(stream->their_src, stream->post_sink);
            else
                gst_pad_unlink(stream->their_src, stream->my_sink);
            gst_object_unref(stream->their_src);
            stream->their_src = NULL;
            return;
        }
    }

    name = gst_pad_get_name(pad);
    GST_WARNING("No pin matching pad \"%s\" found.", name);
    g_free(name);
}

static GstFlowReturn src_getrange_cb(GstPad *pad, GstObject *parent,
        guint64 offset, guint size, GstBuffer **buffer)
{
    struct wg_parser *parser = gst_pad_get_element_private(pad);
    GstFlowReturn ret;

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

    gst_pad_push_event(parser->my_src, gst_event_new_eos());

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
        gst_pad_push_event(parser->my_src, flush_event);
        if (thread)
            gst_pad_set_active(parser->my_src, 1);
    }

    parser->next_offset = parser->start_offset = cur;

    /* and prepare to continue streaming */
    if (flags & GST_SEEK_FLAG_FLUSH)
    {
        flush_event = gst_event_new_flush_stop(TRUE);
        gst_event_set_seqnum(flush_event, seqnum);
        gst_pad_push_event(parser->my_src, flush_event);
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

static NTSTATUS wg_parser_connect(void *args)
{
    GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("quartz_src",
            GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
    const struct wg_parser_connect_params *params = args;
    struct wg_parser *parser = params->parser;
    unsigned int i;
    int ret;

    parser->file_size = params->file_size;
    parser->sink_connected = true;

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

    if (!parser->init_gst(parser))
        goto out;

    gst_element_set_state(parser->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(parser->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        GST_ERROR("Failed to play stream.\n");
        goto out;
    }

    pthread_mutex_lock(&parser->mutex);

    while (!parser->no_more_pads && !parser->error)
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

        while (!stream->has_caps && !parser->error)
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
            if (gst_pad_query_duration(stream->their_src, GST_FORMAT_TIME, &duration))
            {
                stream->duration = duration / 100;
                break;
            }

            if (stream->eos)
            {
                stream->duration = 0;
                GST_WARNING("Failed to query duration.\n");
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
    if (parser->their_sink)
    {
        gst_object_unref(parser->their_sink);
        parser->my_src = parser->their_sink = NULL;
    }

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

    pthread_mutex_lock(&parser->mutex);
    parser->sink_connected = false;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&parser->read_cond);

    return E_FAIL;
}

static NTSTATUS wg_parser_disconnect(void *args)
{
    struct wg_parser *parser = args;
    unsigned int i;

    /* Unblock all of our streams. */
    pthread_mutex_lock(&parser->mutex);
    for (i = 0; i < parser->stream_count; ++i)
    {
        parser->streams[i]->flushing = true;
        pthread_cond_signal(&parser->streams[i]->event_empty_cond);
    }
    pthread_mutex_unlock(&parser->mutex);

    gst_element_set_state(parser->container, GST_STATE_NULL);
    gst_object_unref(parser->my_src);
    gst_object_unref(parser->their_sink);
    parser->my_src = parser->their_sink = NULL;

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

    return S_OK;
}

static BOOL decodebin_parser_init_gst(struct wg_parser *parser)
{
    GstElement *element;
    int ret;

    if (!(element = create_element("decodebin", "base")))
        return FALSE;

    gst_bin_add(GST_BIN(parser->container), element);
    parser->decodebin = element;

    if (parser->unlimited_buffering)
    {
        g_object_set(parser->decodebin, "max-size-buffers", G_MAXUINT, NULL);
        g_object_set(parser->decodebin, "max-size-time", G_MAXUINT64, NULL);
        g_object_set(parser->decodebin, "max-size-bytes", G_MAXUINT, NULL);
    }

    g_signal_connect(element, "pad-added", G_CALLBACK(pad_added_cb), parser);
    g_signal_connect(element, "pad-removed", G_CALLBACK(pad_removed_cb), parser);
    g_signal_connect(element, "autoplug-select", G_CALLBACK(autoplug_select_cb), parser);
    g_signal_connect(element, "no-more-pads", G_CALLBACK(no_more_pads_cb), parser);

    parser->their_sink = gst_element_get_static_pad(element, "sink");

    pthread_mutex_lock(&parser->mutex);
    parser->no_more_pads = false;
    pthread_mutex_unlock(&parser->mutex);

    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        GST_ERROR("Failed to link pads, error %d.", ret);
        return FALSE;
    }

    return TRUE;
}

static BOOL avi_parser_init_gst(struct wg_parser *parser)
{
    GstElement *element;
    int ret;

    if (!(element = create_element("avidemux", "good")))
        return FALSE;

    gst_bin_add(GST_BIN(parser->container), element);

    g_signal_connect(element, "pad-added", G_CALLBACK(pad_added_cb), parser);
    g_signal_connect(element, "pad-removed", G_CALLBACK(pad_removed_cb), parser);
    g_signal_connect(element, "no-more-pads", G_CALLBACK(no_more_pads_cb), parser);

    parser->their_sink = gst_element_get_static_pad(element, "sink");

    pthread_mutex_lock(&parser->mutex);
    parser->no_more_pads = false;
    pthread_mutex_unlock(&parser->mutex);

    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        GST_ERROR("Failed to link pads, error %d.", ret);
        return FALSE;
    }

    return TRUE;
}

static BOOL mpeg_audio_parser_init_gst(struct wg_parser *parser)
{
    struct wg_parser_stream *stream;
    GstElement *element;
    int ret;

    if (!(element = create_element("mpegaudioparse", "good")))
        return FALSE;

    gst_bin_add(GST_BIN(parser->container), element);

    parser->their_sink = gst_element_get_static_pad(element, "sink");
    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        GST_ERROR("Failed to link sink pads, error %d.", ret);
        return FALSE;
    }

    if (!(stream = create_stream(parser)))
        return FALSE;

    gst_object_ref(stream->their_src = gst_element_get_static_pad(element, "src"));
    if ((ret = gst_pad_link(stream->their_src, stream->my_sink)) < 0)
    {
        GST_ERROR("Failed to link source pads, error %d.", ret);
        return FALSE;
    }
    gst_pad_set_active(stream->my_sink, 1);

    parser->no_more_pads = true;

    return TRUE;
}

static BOOL wave_parser_init_gst(struct wg_parser *parser)
{
    struct wg_parser_stream *stream;
    GstElement *element;
    int ret;

    if (!(element = create_element("wavparse", "good")))
        return FALSE;

    gst_bin_add(GST_BIN(parser->container), element);

    parser->their_sink = gst_element_get_static_pad(element, "sink");
    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        GST_ERROR("Failed to link sink pads, error %d.", ret);
        return FALSE;
    }

    if (!(stream = create_stream(parser)))
        return FALSE;

    stream->their_src = gst_element_get_static_pad(element, "src");
    gst_object_ref(stream->their_src);
    if ((ret = gst_pad_link(stream->their_src, stream->my_sink)) < 0)
    {
        GST_ERROR("Failed to link source pads, error %d.", ret);
        return FALSE;
    }
    gst_pad_set_active(stream->my_sink, 1);

    parser->no_more_pads = true;

    return TRUE;
}

static void init_gstreamer_once(void)
{
    char arg0[] = "wine";
    char arg1[] = "--gst-disable-registry-fork";
    char *args[] = {arg0, arg1, NULL};
    int argc = ARRAY_SIZE(args) - 1;
    char **argv = args;
    GError *err;

    if (!gst_init_check(&argc, &argv, &err))
    {
        fprintf(stderr, "winegstreamer: failed to initialize GStreamer: %s\n", err->message);
        g_error_free(err);
        return;
    }

    GST_DEBUG_CATEGORY_INIT(wine, "WINE", GST_DEBUG_FG_RED, "Wine GStreamer support");

    GST_INFO("GStreamer library version %s; wine built with %d.%d.%d.",
            gst_version_string(), GST_VERSION_MAJOR, GST_VERSION_MINOR, GST_VERSION_MICRO);
}

bool init_gstreamer(void)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;

    return !pthread_once(&init_once, init_gstreamer_once);
}

static NTSTATUS wg_parser_create(void *args)
{
    static const init_gst_cb init_funcs[] =
    {
        [WG_PARSER_DECODEBIN] = decodebin_parser_init_gst,
        [WG_PARSER_AVIDEMUX] = avi_parser_init_gst,
        [WG_PARSER_MPEGAUDIOPARSE] = mpeg_audio_parser_init_gst,
        [WG_PARSER_WAVPARSE] = wave_parser_init_gst,
    };

    struct wg_parser_create_params *params = args;
    struct wg_parser *parser;

    if (!init_gstreamer())
        return E_FAIL;

    if (!(parser = calloc(1, sizeof(*parser))))
        return E_OUTOFMEMORY;

    pthread_mutex_init(&parser->mutex, NULL);
    pthread_cond_init(&parser->init_cond, NULL);
    pthread_cond_init(&parser->read_cond, NULL);
    pthread_cond_init(&parser->read_done_cond, NULL);
    parser->init_gst = init_funcs[params->type];
    parser->unlimited_buffering = params->unlimited_buffering;
    parser->err_on = params->err_on;
    parser->warn_on = params->warn_on;
    GST_DEBUG("Created winegstreamer parser %p.", parser);
    params->parser = parser;
    return S_OK;
}

static NTSTATUS wg_parser_destroy(void *args)
{
    struct wg_parser *parser = args;

    if (parser->bus)
    {
        gst_bus_set_sync_handler(parser->bus, NULL, NULL, NULL);
        gst_object_unref(parser->bus);
    }

    pthread_mutex_destroy(&parser->mutex);
    pthread_cond_destroy(&parser->init_cond);
    pthread_cond_destroy(&parser->read_cond);
    pthread_cond_destroy(&parser->read_done_cond);

    free(parser);
    return S_OK;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
#define X(name) [unix_ ## name] = name
    X(wg_parser_create),
    X(wg_parser_destroy),

    X(wg_parser_connect),
    X(wg_parser_disconnect),

    X(wg_parser_get_next_read_offset),
    X(wg_parser_push_data),

    X(wg_parser_get_stream_count),
    X(wg_parser_get_stream),

    X(wg_parser_stream_get_preferred_format),
    X(wg_parser_stream_enable),
    X(wg_parser_stream_disable),

    X(wg_parser_stream_get_buffer),
    X(wg_parser_stream_copy_buffer),
    X(wg_parser_stream_release_buffer),
    X(wg_parser_stream_notify_qos),

    X(wg_parser_stream_get_duration),
    X(wg_parser_stream_seek),

    X(wg_transform_create),
    X(wg_transform_destroy),
    X(wg_transform_set_output_format),

    X(wg_transform_push_data),
    X(wg_transform_read_data),
};
