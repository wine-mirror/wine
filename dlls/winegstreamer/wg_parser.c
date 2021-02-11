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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "gst_private.h"
#include "winternl.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

GST_DEBUG_CATEGORY_STATIC(wine);
#define GST_CAT_DEFAULT wine

static enum wg_audio_format wg_audio_format_from_gst(GstAudioFormat format)
{
    switch (format)
    {
        case GST_AUDIO_FORMAT_U8:
            return WG_AUDIO_FORMAT_U8;
        case GST_AUDIO_FORMAT_S16LE:
            return WG_AUDIO_FORMAT_S16LE;
        case GST_AUDIO_FORMAT_S24LE:
            return WG_AUDIO_FORMAT_S24LE;
        case GST_AUDIO_FORMAT_S32LE:
            return WG_AUDIO_FORMAT_S32LE;
        case GST_AUDIO_FORMAT_F32LE:
            return WG_AUDIO_FORMAT_F32LE;
        case GST_AUDIO_FORMAT_F64LE:
            return WG_AUDIO_FORMAT_F64LE;
        default:
            return WG_AUDIO_FORMAT_UNKNOWN;
    }
}

static void wg_format_from_audio_info(struct wg_format *format, const GstAudioInfo *info)
{
    format->major_type = WG_MAJOR_TYPE_AUDIO;
    format->u.audio.format = wg_audio_format_from_gst(GST_AUDIO_INFO_FORMAT(info));
    format->u.audio.channels = GST_AUDIO_INFO_CHANNELS(info);
    format->u.audio.rate = GST_AUDIO_INFO_RATE(info);
}

static enum wg_video_format wg_video_format_from_gst(GstVideoFormat format)
{
    switch (format)
    {
        case GST_VIDEO_FORMAT_BGRA:
            return WG_VIDEO_FORMAT_BGRA;
        case GST_VIDEO_FORMAT_BGRx:
            return WG_VIDEO_FORMAT_BGRx;
        case GST_VIDEO_FORMAT_BGR:
            return WG_VIDEO_FORMAT_BGR;
        case GST_VIDEO_FORMAT_RGB15:
            return WG_VIDEO_FORMAT_RGB15;
        case GST_VIDEO_FORMAT_AYUV:
            return WG_VIDEO_FORMAT_AYUV;
        case GST_VIDEO_FORMAT_I420:
            return WG_VIDEO_FORMAT_I420;
        case GST_VIDEO_FORMAT_NV12:
            return WG_VIDEO_FORMAT_NV12;
        case GST_VIDEO_FORMAT_UYVY:
            return WG_VIDEO_FORMAT_UYVY;
        case GST_VIDEO_FORMAT_YUY2:
            return WG_VIDEO_FORMAT_YUY2;
        case GST_VIDEO_FORMAT_YV12:
            return WG_VIDEO_FORMAT_YV12;
        case GST_VIDEO_FORMAT_YVYU:
            return WG_VIDEO_FORMAT_YVYU;
        default:
            return WG_VIDEO_FORMAT_UNKNOWN;
    }
}

static void wg_format_from_video_info(struct wg_format *format, const GstVideoInfo *info)
{
    format->major_type = WG_MAJOR_TYPE_VIDEO;
    format->u.video.format = wg_video_format_from_gst(GST_VIDEO_INFO_FORMAT(info));
    format->u.video.width = GST_VIDEO_INFO_WIDTH(info);
    format->u.video.height = GST_VIDEO_INFO_HEIGHT(info);
    format->u.video.fps_n = GST_VIDEO_INFO_FPS_N(info);
    format->u.video.fps_d = GST_VIDEO_INFO_FPS_D(info);
}

static void wg_format_from_caps_audio_mpeg(struct wg_format *format, const GstCaps *caps)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint layer, channels, rate;

    if (!gst_structure_get_int(structure, "layer", &layer))
    {
        GST_WARNING("Missing \"layer\" value.");
        return;
    }
    if (!gst_structure_get_int(structure, "channels", &channels))
    {
        GST_WARNING("Missing \"channels\" value.");
        return;
    }
    if (!gst_structure_get_int(structure, "rate", &rate))
    {
        GST_WARNING("Missing \"rate\" value.");
        return;
    }

    format->major_type = WG_MAJOR_TYPE_AUDIO;

    if (layer == 1)
        format->u.audio.format = WG_AUDIO_FORMAT_MPEG1_LAYER1;
    else if (layer == 2)
        format->u.audio.format = WG_AUDIO_FORMAT_MPEG1_LAYER2;
    else if (layer == 3)
        format->u.audio.format = WG_AUDIO_FORMAT_MPEG1_LAYER3;

    format->u.audio.channels = channels;
    format->u.audio.rate = rate;
}

static void wg_format_from_caps_video_cinepak(struct wg_format *format, const GstCaps *caps)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint width, height, fps_n, fps_d;

    if (!gst_structure_get_int(structure, "width", &width))
    {
        GST_WARNING("Missing \"width\" value.");
        return;
    }
    if (!gst_structure_get_int(structure, "height", &height))
    {
        GST_WARNING("Missing \"height\" value.");
        return;
    }
    if (!gst_structure_get_fraction(structure, "framerate", &fps_n, &fps_d))
    {
        fps_n = 0;
        fps_d = 1;
    }

    format->major_type = WG_MAJOR_TYPE_VIDEO;
    format->u.video.format = WG_VIDEO_FORMAT_CINEPAK;
    format->u.video.width = width;
    format->u.video.height = height;
    format->u.video.fps_n = fps_n;
    format->u.video.fps_d = fps_d;
}

static void wg_format_from_caps(struct wg_format *format, const GstCaps *caps)
{
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const char *name = gst_structure_get_name(structure);

    memset(format, 0, sizeof(*format));

    if (!strcmp(name, "audio/x-raw"))
    {
        GstAudioInfo info;

        if (gst_audio_info_from_caps(&info, caps))
            wg_format_from_audio_info(format, &info);
    }
    else if (!strcmp(name, "video/x-raw"))
    {
        GstVideoInfo info;

        if (gst_video_info_from_caps(&info, caps))
            wg_format_from_video_info(format, &info);
    }
    else if (!strcmp(name, "audio/mpeg"))
    {
        wg_format_from_caps_audio_mpeg(format, caps);
    }
    else if (!strcmp(name, "video/x-cinepak"))
    {
        wg_format_from_caps_video_cinepak(format, caps);
    }
    else
    {
        gchar *str = gst_caps_to_string(caps);

        GST_FIXME("Unhandled caps %s.", str);
        g_free(str);
    }
}

static GstAudioFormat wg_audio_format_to_gst(enum wg_audio_format format)
{
    switch (format)
    {
        case WG_AUDIO_FORMAT_U8:    return GST_AUDIO_FORMAT_U8;
        case WG_AUDIO_FORMAT_S16LE: return GST_AUDIO_FORMAT_S16LE;
        case WG_AUDIO_FORMAT_S24LE: return GST_AUDIO_FORMAT_S24LE;
        case WG_AUDIO_FORMAT_S32LE: return GST_AUDIO_FORMAT_S32LE;
        case WG_AUDIO_FORMAT_F32LE: return GST_AUDIO_FORMAT_F32LE;
        case WG_AUDIO_FORMAT_F64LE: return GST_AUDIO_FORMAT_F64LE;
        default: return GST_AUDIO_FORMAT_UNKNOWN;
    }
}

static GstCaps *wg_format_to_caps_audio(const struct wg_format *format)
{
    GstAudioFormat audio_format;
    GstAudioInfo info;

    if ((audio_format = wg_audio_format_to_gst(format->u.audio.format)) == GST_AUDIO_FORMAT_UNKNOWN)
        return NULL;

    gst_audio_info_set_format(&info, audio_format, format->u.audio.rate, format->u.audio.channels, NULL);
    return gst_audio_info_to_caps(&info);
}

static GstVideoFormat wg_video_format_to_gst(enum wg_video_format format)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_BGRA:  return GST_VIDEO_FORMAT_BGRA;
        case WG_VIDEO_FORMAT_BGRx:  return GST_VIDEO_FORMAT_BGRx;
        case WG_VIDEO_FORMAT_BGR:   return GST_VIDEO_FORMAT_BGR;
        case WG_VIDEO_FORMAT_RGB15: return GST_VIDEO_FORMAT_RGB15;
        case WG_VIDEO_FORMAT_RGB16: return GST_VIDEO_FORMAT_RGB16;
        case WG_VIDEO_FORMAT_AYUV:  return GST_VIDEO_FORMAT_AYUV;
        case WG_VIDEO_FORMAT_I420:  return GST_VIDEO_FORMAT_I420;
        case WG_VIDEO_FORMAT_NV12:  return GST_VIDEO_FORMAT_NV12;
        case WG_VIDEO_FORMAT_UYVY:  return GST_VIDEO_FORMAT_UYVY;
        case WG_VIDEO_FORMAT_YUY2:  return GST_VIDEO_FORMAT_YUY2;
        case WG_VIDEO_FORMAT_YV12:  return GST_VIDEO_FORMAT_YV12;
        case WG_VIDEO_FORMAT_YVYU:  return GST_VIDEO_FORMAT_YVYU;
        default: return GST_VIDEO_FORMAT_UNKNOWN;
    }
}

static GstCaps *wg_format_to_caps_video(const struct wg_format *format)
{
    GstVideoFormat video_format;
    GstVideoInfo info;
    unsigned int i;
    GstCaps *caps;

    if ((video_format = wg_video_format_to_gst(format->u.video.format)) == GST_VIDEO_FORMAT_UNKNOWN)
        return NULL;

    gst_video_info_set_format(&info, video_format, format->u.video.width, format->u.video.height);
    if ((caps = gst_video_info_to_caps(&info)))
    {
        /* Clear some fields that shouldn't prevent us from connecting. */
        for (i = 0; i < gst_caps_get_size(caps); ++i)
        {
            gst_structure_remove_fields(gst_caps_get_structure(caps, i),
                    "framerate", "pixel-aspect-ratio", "colorimetry", "chroma-site", NULL);
        }
    }
    return caps;
}

static GstCaps *wg_format_to_caps(const struct wg_format *format)
{
    switch (format->major_type)
    {
        case WG_MAJOR_TYPE_UNKNOWN:
            return NULL;
        case WG_MAJOR_TYPE_AUDIO:
            return wg_format_to_caps_audio(format);
        case WG_MAJOR_TYPE_VIDEO:
            return wg_format_to_caps_video(format);
    }
    assert(0);
    return NULL;
}

static bool wg_format_compare(const struct wg_format *a, const struct wg_format *b)
{
    if (a->major_type != b->major_type)
        return false;

    switch (a->major_type)
    {
        case WG_MAJOR_TYPE_UNKNOWN:
            return false;

        case WG_MAJOR_TYPE_AUDIO:
            return a->u.audio.format == b->u.audio.format
                    && a->u.audio.channels == b->u.audio.channels
                    && a->u.audio.rate == b->u.audio.rate;

        case WG_MAJOR_TYPE_VIDEO:
            return a->u.video.format == b->u.video.format
                    && a->u.video.width == b->u.video.width
                    && a->u.video.height == b->u.video.height
                    && a->u.video.fps_d * b->u.video.fps_n == a->u.video.fps_n * b->u.video.fps_d;
    }

    assert(0);
    return false;
}

static GstAutoplugSelectResult autoplug_blacklist(GstElement *bin, GstPad *pad, GstCaps *caps, GstElementFactory *fact, gpointer user)
{
    const char *name = gst_element_factory_get_longname(fact);

    GST_TRACE("Using \"%s\".", name);

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

static void no_more_pads(GstElement *element, gpointer user)
{
    struct wg_parser *parser = user;

    GST_DEBUG("parser %p.", parser);

    pthread_mutex_lock(&parser->mutex);
    parser->no_more_pads = true;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&parser->init_cond);
}

static GstFlowReturn queue_stream_event(struct wg_parser_stream *stream, const struct wg_parser_event *event)
{
    struct wg_parser *parser = stream->parser;

    /* Unlike request_buffer_src() [q.v.], we need to watch for GStreamer
     * flushes here. The difference is that we can be blocked by the streaming
     * thread not running (or itself flushing on the DirectShow side).
     * request_buffer_src() can only be blocked by the upstream source, and that
     * is solved by flushing the upstream source. */

    pthread_mutex_lock(&parser->mutex);
    while (!stream->flushing && stream->event.type != WG_PARSER_EVENT_NONE)
        pthread_cond_wait(&stream->event_empty_cond, &parser->mutex);
    if (stream->flushing)
    {
        pthread_mutex_unlock(&parser->mutex);
        GST_DEBUG("Filter is flushing; discarding event.");
        return GST_FLOW_FLUSHING;
    }
    stream->event = *event;
    pthread_mutex_unlock(&parser->mutex);
    pthread_cond_signal(&stream->event_cond);
    GST_LOG("Event queued.");
    return GST_FLOW_OK;
}

static gboolean event_sink(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct wg_parser_stream *stream = gst_pad_get_element_private(pad);
    struct wg_parser *parser = stream->parser;

    GST_LOG("stream %p, type \"%s\".", stream, GST_EVENT_TYPE_NAME(event));

    switch (event->type)
    {
        case GST_EVENT_SEGMENT:
            if (stream->enabled)
            {
                struct wg_parser_event stream_event;
                const GstSegment *segment;

                gst_event_parse_segment(event, &segment);

                if (segment->format != GST_FORMAT_TIME)
                {
                    GST_FIXME("Unhandled format \"%s\".", gst_format_get_name(segment->format));
                    break;
                }

                stream_event.type = WG_PARSER_EVENT_SEGMENT;
                stream_event.u.segment.position = segment->position / 100;
                stream_event.u.segment.stop = segment->stop / 100;
                stream_event.u.segment.rate = segment->rate * segment->applied_rate;
                queue_stream_event(stream, &stream_event);
            }
            break;

        case GST_EVENT_EOS:
            if (stream->enabled)
            {
                struct wg_parser_event stream_event;

                stream_event.type = WG_PARSER_EVENT_EOS;
                queue_stream_event(stream, &stream_event);
            }
            else
            {
                pthread_mutex_lock(&parser->mutex);
                stream->eos = true;
                pthread_mutex_unlock(&parser->mutex);
                pthread_cond_signal(&parser->init_cond);
            }
            break;

        case GST_EVENT_FLUSH_START:
            if (stream->enabled)
            {
                pthread_mutex_lock(&parser->mutex);

                stream->flushing = true;
                pthread_cond_signal(&stream->event_empty_cond);

                switch (stream->event.type)
                {
                    case WG_PARSER_EVENT_NONE:
                    case WG_PARSER_EVENT_EOS:
                    case WG_PARSER_EVENT_SEGMENT:
                        break;

                    case WG_PARSER_EVENT_BUFFER:
                        gst_buffer_unref(stream->event.u.buffer);
                        break;
                }
                stream->event.type = WG_PARSER_EVENT_NONE;

                pthread_mutex_unlock(&parser->mutex);
            }
            break;

        case GST_EVENT_FLUSH_STOP:
            if (stream->enabled)
            {
                pthread_mutex_lock(&parser->mutex);
                stream->flushing = false;
                pthread_mutex_unlock(&parser->mutex);
            }
            break;

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

static GstFlowReturn got_data_sink(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    struct wg_parser_stream *stream = gst_pad_get_element_private(pad);
    struct wg_parser_event stream_event;
    GstFlowReturn ret;

    GST_LOG("stream %p, buffer %p.", stream, buffer);

    if (!stream->enabled)
    {
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }

    stream_event.type = WG_PARSER_EVENT_BUFFER;
    stream_event.u.buffer = buffer;
    /* Transfer our reference to the buffer to the object. */
    if ((ret = queue_stream_event(stream, &stream_event)) != GST_FLOW_OK)
        gst_buffer_unref(buffer);
    return ret;
}

static gboolean query_sink(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct wg_parser_stream *stream = gst_pad_get_element_private(pad);

    GST_LOG("stream %p, type \"%s\".", stream, gst_query_type_get_name(query->type));

    switch (query->type)
    {
        case GST_QUERY_CAPS:
        {
            GstCaps *caps, *filter, *temp;

            gst_query_parse_caps(query, &filter);

            if (stream->enabled)
                caps = wg_format_to_caps(&stream->current_format);
            else
                caps = gst_caps_new_any();
            if (!caps)
                return FALSE;

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

            if (!stream->enabled)
            {
                gst_query_set_accept_caps_result(query, TRUE);
                return TRUE;
            }

            gst_query_parse_accept_caps(query, &caps);
            wg_format_from_caps(&format, caps);
            ret = wg_format_compare(&format, &stream->current_format);
            if (!ret && WARN_ON(gstreamer))
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

static struct wg_parser_stream *create_stream(struct wg_parser *parser)
{
    struct wg_parser_stream *stream, **new_array;
    char pad_name[19];

    if (!(new_array = realloc(parser->streams, (parser->stream_count + 1) * sizeof(*parser->streams))))
        return NULL;
    parser->streams = new_array;

    if (!(stream = calloc(1, sizeof(*stream))))
        return NULL;

    stream->parser = parser;
    pthread_cond_init(&stream->event_cond, NULL);
    pthread_cond_init(&stream->event_empty_cond, NULL);

    sprintf(pad_name, "qz_sink_%u", parser->stream_count);
    stream->my_sink = gst_pad_new(pad_name, GST_PAD_SINK);
    gst_pad_set_element_private(stream->my_sink, stream);
    gst_pad_set_chain_function(stream->my_sink, got_data_sink);
    gst_pad_set_event_function(stream->my_sink, event_sink);
    gst_pad_set_query_function(stream->my_sink, query_sink);

    parser->streams[parser->stream_count++] = stream;
    return stream;
}

static void init_new_decoded_pad(GstElement *element, GstPad *pad, struct wg_parser *parser)
{
    struct wg_parser_stream *stream;
    const char *name;
    GstCaps *caps;
    int ret;

    caps = gst_caps_make_writable(gst_pad_query_caps(pad, NULL));
    name = gst_structure_get_name(gst_caps_get_structure(caps, 0));

    if (!(stream = create_stream(parser)))
        goto out;

    if (!strcmp(name, "video/x-raw"))
    {
        GstElement *deinterlace, *vconv, *flip, *vconv2;

        /* DirectShow can express interlaced video, but downstream filters can't
         * necessarily consume it. In particular, the video renderer can't. */
        if (!(deinterlace = gst_element_factory_make("deinterlace", NULL)))
        {
            fprintf(stderr, "winegstreamer: failed to create deinterlace, are %u-bit GStreamer \"good\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

        /* decodebin considers many YUV formats to be "raw", but some quartz
         * filters can't handle those. Also, videoflip can't handle all "raw"
         * formats either. Add a videoconvert to swap color spaces. */
        if (!(vconv = gst_element_factory_make("videoconvert", NULL)))
        {
            fprintf(stderr, "winegstreamer: failed to create videoconvert, are %u-bit GStreamer \"base\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

        /* GStreamer outputs RGB video top-down, but DirectShow expects bottom-up. */
        if (!(flip = gst_element_factory_make("videoflip", NULL)))
        {
            fprintf(stderr, "winegstreamer: failed to create videoflip, are %u-bit GStreamer \"good\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

        /* videoflip does not support 15 and 16-bit RGB so add a second videoconvert
         * to do the final conversion. */
        if (!(vconv2 = gst_element_factory_make("videoconvert", NULL)))
        {
            fprintf(stderr, "winegstreamer: failed to create videoconvert, are %u-bit GStreamer \"base\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

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
        if (!(convert = gst_element_factory_make("audioconvert", NULL)))
        {
            fprintf(stderr, "winegstreamer: failed to create audioconvert, are %u-bit GStreamer \"base\" plugins installed?\n",
                    8 * (int)sizeof(void *));
            goto out;
        }

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

static void existing_new_pad(GstElement *element, GstPad *pad, gpointer user)
{
    struct wg_parser *parser = user;

    GST_LOG("parser %p, element %p, pad %p.", parser, element, pad);

    if (gst_pad_is_linked(pad))
        return;

    init_new_decoded_pad(element, pad, parser);
}

static void removed_decoded_pad(GstElement *element, GstPad *pad, gpointer user)
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

static BOOL decodebin_parser_init_gst(struct wg_parser *parser)
{
    GstElement *element = gst_element_factory_make("decodebin", NULL);
    int ret;

    if (!element)
    {
        ERR("Failed to create decodebin; are %u-bit GStreamer \"base\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(parser->container), element);

    g_signal_connect(element, "pad-added", G_CALLBACK(existing_new_pad), parser);
    g_signal_connect(element, "pad-removed", G_CALLBACK(removed_decoded_pad), parser);
    g_signal_connect(element, "autoplug-select", G_CALLBACK(autoplug_blacklist), parser);
    g_signal_connect(element, "no-more-pads", G_CALLBACK(no_more_pads), parser);

    parser->their_sink = gst_element_get_static_pad(element, "sink");

    pthread_mutex_lock(&parser->mutex);
    parser->no_more_pads = parser->error = false;
    pthread_mutex_unlock(&parser->mutex);

    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        ERR("Failed to link pads, error %d.\n", ret);
        return FALSE;
    }

    gst_element_set_state(parser->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(parser->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    pthread_mutex_lock(&parser->mutex);
    while (!parser->no_more_pads && !parser->error)
        pthread_cond_wait(&parser->init_cond, &parser->mutex);
    if (parser->error)
    {
        pthread_mutex_unlock(&parser->mutex);
        return FALSE;
    }
    pthread_mutex_unlock(&parser->mutex);

    return TRUE;
}

static BOOL avi_parser_init_gst(struct wg_parser *parser)
{
    GstElement *element = gst_element_factory_make("avidemux", NULL);
    int ret;

    if (!element)
    {
        ERR("Failed to create avidemux; are %u-bit GStreamer \"good\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(parser->container), element);

    g_signal_connect(element, "pad-added", G_CALLBACK(existing_new_pad), parser);
    g_signal_connect(element, "pad-removed", G_CALLBACK(removed_decoded_pad), parser);
    g_signal_connect(element, "no-more-pads", G_CALLBACK(no_more_pads), parser);

    parser->their_sink = gst_element_get_static_pad(element, "sink");

    pthread_mutex_lock(&parser->mutex);
    parser->no_more_pads = parser->error = false;
    pthread_mutex_unlock(&parser->mutex);

    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        ERR("Failed to link pads, error %d.\n", ret);
        return FALSE;
    }

    gst_element_set_state(parser->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(parser->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    pthread_mutex_lock(&parser->mutex);
    while (!parser->no_more_pads && !parser->error)
        pthread_cond_wait(&parser->init_cond, &parser->mutex);
    if (parser->error)
    {
        pthread_mutex_unlock(&parser->mutex);
        return FALSE;
    }
    pthread_mutex_unlock(&parser->mutex);

    return TRUE;
}

static BOOL mpeg_audio_parser_init_gst(struct wg_parser *parser)
{
    struct wg_parser_stream *stream;
    GstElement *element;
    int ret;

    if (!(element = gst_element_factory_make("mpegaudioparse", NULL)))
    {
        ERR("Failed to create mpegaudioparse; are %u-bit GStreamer \"good\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(parser->container), element);

    parser->their_sink = gst_element_get_static_pad(element, "sink");
    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        ERR("Failed to link sink pads, error %d.\n", ret);
        return FALSE;
    }

    if (!(stream = create_stream(parser)))
        return FALSE;

    gst_object_ref(stream->their_src = gst_element_get_static_pad(element, "src"));
    if ((ret = gst_pad_link(stream->their_src, stream->my_sink)) < 0)
    {
        ERR("Failed to link source pads, error %d.\n", ret);
        return FALSE;
    }

    gst_pad_set_active(stream->my_sink, 1);
    gst_element_set_state(parser->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(parser->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    pthread_mutex_lock(&parser->mutex);
    while (!parser->has_duration && !parser->error && !stream->eos)
        pthread_cond_wait(&parser->init_cond, &parser->mutex);
    if (parser->error)
    {
        pthread_mutex_unlock(&parser->mutex);
        return FALSE;
    }
    pthread_mutex_unlock(&parser->mutex);
    return TRUE;
}

static BOOL wave_parser_init_gst(struct wg_parser *parser)
{
    struct wg_parser_stream *stream;
    GstElement *element;
    int ret;

    if (!(element = gst_element_factory_make("wavparse", NULL)))
    {
        ERR("Failed to create wavparse; are %u-bit GStreamer \"good\" plugins installed?\n",
                8 * (int)sizeof(void*));
        return FALSE;
    }

    gst_bin_add(GST_BIN(parser->container), element);

    parser->their_sink = gst_element_get_static_pad(element, "sink");
    if ((ret = gst_pad_link(parser->my_src, parser->their_sink)) < 0)
    {
        ERR("Failed to link sink pads, error %d.\n", ret);
        return FALSE;
    }

    if (!(stream = create_stream(parser)))
        return FALSE;

    stream->their_src = gst_element_get_static_pad(element, "src");
    gst_object_ref(stream->their_src);
    if ((ret = gst_pad_link(stream->their_src, stream->my_sink)) < 0)
    {
        ERR("Failed to link source pads, error %d.\n", ret);
        return FALSE;
    }

    gst_pad_set_active(stream->my_sink, 1);
    gst_element_set_state(parser->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(parser->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        ERR("Failed to play stream.\n");
        return FALSE;
    }

    return TRUE;
}

static struct wg_parser *wg_parser_create(void)
{
    struct wg_parser *parser;

    if (!(parser = calloc(1, sizeof(*parser))))
        return NULL;

    pthread_mutex_init(&parser->mutex, NULL);
    pthread_cond_init(&parser->init_cond, NULL);
    pthread_cond_init(&parser->read_cond, NULL);
    pthread_cond_init(&parser->read_done_cond, NULL);
    parser->flushing = true;

    TRACE("Created winegstreamer parser %p.\n", parser);
    return parser;
}

static struct wg_parser * CDECL wg_decodebin_parser_create(void)
{
    struct wg_parser *parser;

    if ((parser = wg_parser_create()))
        parser->init_gst = decodebin_parser_init_gst;
    return parser;
}

static struct wg_parser * CDECL wg_avi_parser_create(void)
{
    struct wg_parser *parser;

    if ((parser = wg_parser_create()))
        parser->init_gst = avi_parser_init_gst;
    return parser;
}

static struct wg_parser * CDECL wg_mpeg_audio_parser_create(void)
{
    struct wg_parser *parser;

    if ((parser = wg_parser_create()))
        parser->init_gst = mpeg_audio_parser_init_gst;
    return parser;
}

static struct wg_parser * CDECL wg_wave_parser_create(void)
{
    struct wg_parser *parser;

    if ((parser = wg_parser_create()))
        parser->init_gst = wave_parser_init_gst;
    return parser;
}

static void CDECL wg_parser_destroy(struct wg_parser *parser)
{
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
}

static const struct unix_funcs funcs =
{
    wg_decodebin_parser_create,
    wg_avi_parser_create,
    wg_mpeg_audio_parser_create,
    wg_wave_parser_create,
    wg_parser_destroy,
};

NTSTATUS CDECL __wine_init_unix_lib(HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        char arg0[] = "wine";
        char arg1[] = "--gst-disable-registry-fork";
        char *args[] = {arg0, arg1, NULL};
        int argc = ARRAY_SIZE(args) - 1;
        char **argv = args;
        GError *err;

        if (!gst_init_check(&argc, &argv, &err))
        {
            ERR("Failed to initialize GStreamer: %s\n", debugstr_a(err->message));
            g_error_free(err);
            return STATUS_UNSUCCESSFUL;
        }
        TRACE("GStreamer library version %s; wine built with %d.%d.%d.\n",
                gst_version_string(), GST_VERSION_MAJOR, GST_VERSION_MINOR, GST_VERSION_MICRO);

        GST_DEBUG_CATEGORY_INIT(wine, "WINE", GST_DEBUG_FG_RED, "Wine GStreamer support");

        *(const struct unix_funcs **)ptr_out = &funcs;
    }
    return STATUS_SUCCESS;
}
