/*
 * GStreamer transform backend
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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
#include "mferror.h"

#include "unix_private.h"

#include "wine/list.h"

GST_DEBUG_CATEGORY_EXTERN(wine);
#define GST_CAT_DEFAULT wine

struct wg_transform_sample
{
    struct list entry;
    GstSample *sample;
};

struct wg_transform
{
    GstElement *container;
    GstPad *my_src, *my_sink;
    GstPad *their_sink, *their_src;
    pthread_mutex_t mutex;
    struct list samples;
    GstCaps *sink_caps;
};

static GstCaps *wg_format_to_caps_xwma(const struct wg_encoded_format *format)
{
    GstBuffer *buffer;
    GstCaps *caps;

    if (format->encoded_type == WG_ENCODED_TYPE_WMA)
    {
        caps = gst_caps_new_empty_simple("audio/x-wma");
        if (format->u.xwma.version)
            gst_caps_set_simple(caps, "wmaversion", G_TYPE_INT, format->u.xwma.version, NULL);
    }
    else
    {
        caps = gst_caps_new_empty_simple("audio/x-xma");
        if (format->u.xwma.version)
            gst_caps_set_simple(caps, "xmaversion", G_TYPE_INT, format->u.xwma.version, NULL);
    }

    if (format->u.xwma.bitrate)
        gst_caps_set_simple(caps, "bitrate", G_TYPE_INT, format->u.xwma.bitrate, NULL);
    if (format->u.xwma.rate)
        gst_caps_set_simple(caps, "rate", G_TYPE_INT, format->u.xwma.rate, NULL);
    if (format->u.xwma.depth)
        gst_caps_set_simple(caps, "depth", G_TYPE_INT, format->u.xwma.depth, NULL);
    if (format->u.xwma.channels)
        gst_caps_set_simple(caps, "channels", G_TYPE_INT, format->u.xwma.channels, NULL);
    if (format->u.xwma.block_align)
        gst_caps_set_simple(caps, "block_align", G_TYPE_INT, format->u.xwma.block_align, NULL);

    if (format->u.xwma.codec_data_len)
    {
        buffer = gst_buffer_new_and_alloc(format->u.xwma.codec_data_len);
        gst_buffer_fill(buffer, 0, format->u.xwma.codec_data, format->u.xwma.codec_data_len);
        gst_caps_set_simple(caps, "codec_data", GST_TYPE_BUFFER, buffer, NULL);
        gst_buffer_unref(buffer);
    }

    return caps;
}

static GstCaps *wg_format_to_caps_aac(const struct wg_encoded_format *format)
{
    const char *profile, *level, *stream_format;
    GstBuffer *buffer;
    GstCaps *caps;

    caps = gst_caps_new_empty_simple("audio/mpeg");
    gst_caps_set_simple(caps, "mpegversion", G_TYPE_INT, 4, NULL);

    switch (format->u.aac.payload_type)
    {
        case 0: stream_format = "raw"; break;
        case 1: stream_format = "adts"; break;
        case 2: stream_format = "adif"; break;
        case 3: stream_format = "loas"; break;
        default: stream_format = "raw"; break;
    }
    if (stream_format)
        gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, stream_format, NULL);

    switch (format->u.aac.profile_level_indication)
    {
        case 0x29: profile = "lc"; level = "2";  break;
        case 0x2A: profile = "lc"; level = "4"; break;
        case 0x2B: profile = "lc"; level = "5"; break;
        default:
            GST_FIXME("Unrecognized profile-level-indication %u\n", format->u.aac.profile_level_indication);
            /* fallthrough */
        case 0x00: case 0xFE: profile = level = NULL; break; /* unspecified */
    }
    if (profile)
        gst_caps_set_simple(caps, "profile", G_TYPE_STRING, profile, NULL);
    if (level)
        gst_caps_set_simple(caps, "level", G_TYPE_STRING, level, NULL);

    if (format->u.aac.codec_data_len)
    {
        buffer = gst_buffer_new_and_alloc(format->u.aac.codec_data_len);
        gst_buffer_fill(buffer, 0, format->u.aac.codec_data, format->u.aac.codec_data_len);
        gst_caps_set_simple(caps, "codec_data", GST_TYPE_BUFFER, buffer, NULL);
        gst_buffer_unref(buffer);
    }

    return caps;
}

static GstCaps *wg_format_to_caps_h264(const struct wg_encoded_format *format)
{
    const char *profile, *level;
    GstCaps *caps;

    caps = gst_caps_new_empty_simple("video/x-h264");
    gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "byte-stream", NULL);
    gst_caps_set_simple(caps, "alignment", G_TYPE_STRING, "au", NULL);

    if (format->u.h264.width)
        gst_caps_set_simple(caps, "width", G_TYPE_INT, format->u.h264.width, NULL);
    if (format->u.h264.height)
        gst_caps_set_simple(caps, "height", G_TYPE_INT, format->u.h264.height, NULL);
    if (format->u.h264.fps_n || format->u.h264.fps_d)
        gst_caps_set_simple(caps, "framerate", GST_TYPE_FRACTION, format->u.h264.fps_n, format->u.h264.fps_d, NULL);

    switch (format->u.h264.profile)
    {
        case /* eAVEncH264VProfile_Main */ 77:  profile = "main"; break;
        case /* eAVEncH264VProfile_High */ 100: profile = "high"; break;
        case /* eAVEncH264VProfile_444 */  244: profile = "high-4:4:4"; break;
        default:
            GST_ERROR("Unrecognized H.264 profile attribute %u.", format->u.h264.profile);
            /* fallthrough */
        case 0: profile = NULL;
    }
    if (profile)
        gst_caps_set_simple(caps, "profile", G_TYPE_STRING, profile, NULL);

    switch (format->u.h264.level)
    {
        case /* eAVEncH264VLevel1 */   10: level = "1";   break;
        case /* eAVEncH264VLevel1_1 */ 11: level = "1.1"; break;
        case /* eAVEncH264VLevel1_2 */ 12: level = "1.2"; break;
        case /* eAVEncH264VLevel1_3 */ 13: level = "1.3"; break;
        case /* eAVEncH264VLevel2 */   20: level = "2";   break;
        case /* eAVEncH264VLevel2_1 */ 21: level = "2.1"; break;
        case /* eAVEncH264VLevel2_2 */ 22: level = "2.2"; break;
        case /* eAVEncH264VLevel3 */   30: level = "3";   break;
        case /* eAVEncH264VLevel3_1 */ 31: level = "3.1"; break;
        case /* eAVEncH264VLevel3_2 */ 32: level = "3.2"; break;
        case /* eAVEncH264VLevel4 */   40: level = "4";   break;
        case /* eAVEncH264VLevel4_1 */ 41: level = "4.1"; break;
        case /* eAVEncH264VLevel4_2 */ 42: level = "4.2"; break;
        case /* eAVEncH264VLevel5 */   50: level = "5";   break;
        case /* eAVEncH264VLevel5_1 */ 51: level = "5.1"; break;
        case /* eAVEncH264VLevel5_2 */ 52: level = "5.2"; break;
        default:
            GST_ERROR("Unrecognized H.264 level attribute %u.", format->u.h264.level);
            /* fallthrough */
        case 0: level = NULL;
    }
    if (level)
        gst_caps_set_simple(caps, "level", G_TYPE_STRING, level, NULL);

    return caps;
}

static GstCaps *wg_encoded_format_to_caps(const struct wg_encoded_format *format)
{
    switch (format->encoded_type)
    {
        case WG_ENCODED_TYPE_UNKNOWN:
            return NULL;
        case WG_ENCODED_TYPE_WMA:
        case WG_ENCODED_TYPE_XMA:
            return wg_format_to_caps_xwma(format);
        case WG_ENCODED_TYPE_AAC:
            return wg_format_to_caps_aac(format);
        case WG_ENCODED_TYPE_H264:
            return wg_format_to_caps_h264(format);
    }
    assert(0);
    return NULL;
}

static GstFlowReturn transform_sink_chain_cb(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    struct wg_transform *transform = gst_pad_get_element_private(pad);
    struct wg_transform_sample *sample;

    GST_INFO("transform %p, buffer %p.", transform, buffer);

    if (!(sample = malloc(sizeof(*sample))))
        GST_ERROR("Failed to allocate transform sample entry");
    else
    {
        pthread_mutex_lock(&transform->mutex);
        if (!(sample->sample = gst_sample_new(buffer, transform->sink_caps, NULL, NULL)))
            GST_ERROR("Failed to allocate transform sample");
        list_add_tail(&transform->samples, &sample->entry);
        pthread_mutex_unlock(&transform->mutex);
    }

    gst_buffer_unref(buffer);
    return GST_FLOW_OK;
}

static gboolean transform_sink_event_cb(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct wg_transform *transform = gst_pad_get_element_private(pad);

    GST_INFO("transform %p, type \"%s\".", transform, GST_EVENT_TYPE_NAME(event));

    switch (event->type)
    {
    case GST_EVENT_CAPS:
    {
        GstCaps *caps;
        gchar *str;

        gst_event_parse_caps(event, &caps);
        str = gst_caps_to_string(caps);
        GST_WARNING("Got caps \"%s\".", str);
        g_free(str);

        pthread_mutex_lock(&transform->mutex);
        gst_caps_unref(transform->sink_caps);
        transform->sink_caps = gst_caps_ref(caps);
        pthread_mutex_unlock(&transform->mutex);
        break;
    }
    default:
        GST_WARNING("Ignoring \"%s\" event.", GST_EVENT_TYPE_NAME(event));
    }

    gst_event_unref(event);
    return TRUE;
}

NTSTATUS wg_transform_destroy(void *args)
{
    struct wg_transform *transform = args;
    struct wg_transform_sample *sample, *next;

    if (transform->container)
        gst_element_set_state(transform->container, GST_STATE_NULL);

    if (transform->their_src && transform->my_sink)
        gst_pad_unlink(transform->their_src, transform->my_sink);
    if (transform->their_sink && transform->my_src)
        gst_pad_unlink(transform->my_src, transform->their_sink);

    if (transform->their_sink)
        g_object_unref(transform->their_sink);
    if (transform->their_src)
        g_object_unref(transform->their_src);

    if (transform->container)
        g_object_unref(transform->container);

    if (transform->my_sink)
        g_object_unref(transform->my_sink);
    if (transform->my_src)
        g_object_unref(transform->my_src);

    LIST_FOR_EACH_ENTRY_SAFE(sample, next, &transform->samples, struct wg_transform_sample, entry)
    {
        gst_sample_unref(sample->sample);
        list_remove(&sample->entry);
        free(sample);
    }

    free(transform);
    return S_OK;
}

static GstElement *try_create_transform(GstCaps *src_caps, GstCaps *sink_caps)
{
    GstElement *element = NULL;
    GList *tmp, *transforms;
    gchar *type;

    transforms = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_ANY,
            GST_RANK_MARGINAL);

    tmp = gst_element_factory_list_filter(transforms, src_caps, GST_PAD_SINK, FALSE);
    gst_plugin_feature_list_free(transforms);
    transforms = tmp;

    tmp = gst_element_factory_list_filter(transforms, sink_caps, GST_PAD_SRC, FALSE);
    gst_plugin_feature_list_free(transforms);
    transforms = tmp;

    transforms = g_list_sort(transforms, gst_plugin_feature_rank_compare_func);
    for (tmp = transforms; tmp != NULL && element == NULL; tmp = tmp->next)
    {
        type = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(tmp->data));
        element = gst_element_factory_create(GST_ELEMENT_FACTORY(tmp->data), NULL);
        if (!element)
            GST_WARNING("Failed to create %s element.", type);
    }
    gst_plugin_feature_list_free(transforms);

    if (element)
        GST_INFO("Created %s element %p.", type, element);
    else
    {
        gchar *src_str = gst_caps_to_string(src_caps), *sink_str = gst_caps_to_string(sink_caps);
        GST_WARNING("Failed to create transform matching caps %s / %s.", src_str, sink_str);
        g_free(sink_str);
        g_free(src_str);
    }

    return element;
}

static bool transform_append_element(struct wg_transform *transform, GstElement *element,
        GstElement **first, GstElement **last)
{
    gchar *name = gst_element_get_name(element);

    if (!gst_bin_add(GST_BIN(transform->container), element))
    {
        GST_ERROR("Failed to add %s element to bin.", name);
        g_free(name);
        return false;
    }

    if (*last && !gst_element_link(*last, element))
    {
        GST_ERROR("Failed to link %s element.", name);
        g_free(name);
        return false;
    }

    GST_INFO("Created %s element %p.", name, element);
    g_free(name);

    if (!*first)
        *first = element;

    *last = element;
    return true;
}

NTSTATUS wg_transform_create(void *args)
{
    struct wg_transform_create_params *params = args;
    struct wg_encoded_format input_format = *params->input_format;
    struct wg_format output_format = *params->output_format;
    GstElement *first = NULL, *last = NULL, *element;
    GstCaps *raw_caps, *src_caps, *sink_caps;
    struct wg_transform *transform;
    GstPadTemplate *template;
    const gchar *media_type;
    GstSegment *segment;
    int i, ret;

    if (!init_gstreamer())
        return E_FAIL;

    if (!(transform = calloc(1, sizeof(*transform))))
        return E_OUTOFMEMORY;

    list_init(&transform->samples);

    src_caps = wg_encoded_format_to_caps(&input_format);
    assert(src_caps);
    sink_caps = wg_format_to_caps(&output_format);
    assert(sink_caps);
    media_type = gst_structure_get_name(gst_caps_get_structure(sink_caps, 0));
    raw_caps = gst_caps_new_empty_simple(media_type);
    assert(raw_caps);

    transform->sink_caps = gst_caps_copy(sink_caps);
    transform->container = gst_bin_new("wg_transform");
    assert(transform->container);

    if (!(element = try_create_transform(src_caps, raw_caps)) ||
            !transform_append_element(transform, element, &first, &last))
        goto failed;

    switch (output_format.major_type)
    {
    case WG_MAJOR_TYPE_AUDIO:
        if (!(element = create_element("audioconvert", "base")) ||
                !transform_append_element(transform, element, &first, &last))
            goto failed;
        if (!(element = create_element("audioresample", "base")) ||
                !transform_append_element(transform, element, &first, &last))
            goto failed;
        break;
    case WG_MAJOR_TYPE_VIDEO:
        if (!(element = create_element("videoconvert", "base")) ||
                !transform_append_element(transform, element, &first, &last))
            goto failed;
        for (i = 0; i < gst_caps_get_size(sink_caps); ++i)
            gst_structure_remove_fields(gst_caps_get_structure(sink_caps, i),
                    "width", "height", NULL);
        break;
    default:
        assert(0);
        break;
    }

    if (!(transform->their_sink = gst_element_get_static_pad(first, "sink")))
    {
        GST_ERROR("Failed to find target sink pad.");
        goto failed;
    }
    if (!(transform->their_src = gst_element_get_static_pad(last, "src")))
    {
        GST_ERROR("Failed to find target src pad.");
        goto failed;
    }

    template = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
    assert(template);
    transform->my_src = gst_pad_new_from_template(template, "src");
    g_object_unref(template);
    assert(transform->my_src);

    template = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
    assert(template);
    transform->my_sink = gst_pad_new_from_template(template, "sink");
    g_object_unref(template);
    assert(transform->my_sink);

    gst_pad_set_element_private(transform->my_sink, transform);
    gst_pad_set_event_function(transform->my_sink, transform_sink_event_cb);
    gst_pad_set_chain_function(transform->my_sink, transform_sink_chain_cb);

    if ((ret = gst_pad_link(transform->my_src, transform->their_sink)) < 0)
    {
        GST_ERROR("Failed to link sink pads, error %d.", ret);
        goto failed;
    }
    if ((ret = gst_pad_link(transform->their_src, transform->my_sink)) < 0)
    {
        GST_ERROR("Failed to link source pads, error %d.", ret);
        goto failed;
    }

    if (!(ret = gst_pad_set_active(transform->my_sink, 1)))
        GST_WARNING("Failed to activate my_sink.");
    if (!(ret = gst_pad_set_active(transform->my_src, 1)))
        GST_WARNING("Failed to activate my_src.");

    gst_element_set_state(transform->container, GST_STATE_PAUSED);
    ret = gst_element_get_state(transform->container, NULL, NULL, -1);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        GST_ERROR("Failed to play stream.\n");
        goto failed;
    }

    if (!gst_pad_push_event(transform->my_src, gst_event_new_stream_start("stream")))
    {
        GST_ERROR("Failed to send stream-start.");
        goto failed;
    }

    if (!gst_pad_push_event(transform->my_src, gst_event_new_caps(src_caps)) ||
        !gst_pad_has_current_caps(transform->their_sink))
    {
        GST_ERROR("Failed to set stream caps.");
        goto failed;
    }

    segment = gst_segment_new();
    gst_segment_init(segment, GST_FORMAT_TIME);
    segment->start = 0;
    segment->stop = -1;
    ret = gst_pad_push_event(transform->my_src, gst_event_new_segment(segment));
    gst_segment_free(segment);
    if (!ret)
    {
        GST_ERROR("Failed to start new segment.");
        goto failed;
    }

    GST_INFO("Created winegstreamer transform %p.", transform);
    params->transform = transform;

failed:
    gst_caps_unref(raw_caps);
    gst_caps_unref(src_caps);
    gst_caps_unref(sink_caps);

    if (params->transform)
        return S_OK;

    wg_transform_destroy(transform);
    return E_FAIL;
}

NTSTATUS wg_transform_push_data(void *args)
{
    struct wg_transform_push_data_params *params = args;
    struct wg_transform *transform = params->transform;
    GstBuffer *buffer;
    GstFlowReturn ret;

    buffer = gst_buffer_new_and_alloc(params->size);
    gst_buffer_fill(buffer, 0, params->data, params->size);

    ret = gst_pad_push(transform->my_src, buffer);
    if (ret)
    {
        GST_ERROR("Failed to push buffer %d", ret);
        return MF_E_NOTACCEPTING;
    }

    GST_INFO("Pushed %u bytes", params->size);
    return S_OK;
}

NTSTATUS wg_transform_read_data(void *args)
{
    struct wg_transform_read_data_params *params = args;
    struct wg_transform *transform = params->transform;
    struct wg_sample *read_sample = params->sample;
    struct wg_transform_sample *transform_sample;
    struct wg_format buffer_format;
    bool broken_timestamp = false;
    GstBuffer *buffer;
    struct list *head;
    GstMapInfo info;
    GstCaps *caps;

    pthread_mutex_lock(&transform->mutex);
    if (!(head = list_head(&transform->samples)))
    {
        pthread_mutex_unlock(&transform->mutex);
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    transform_sample = LIST_ENTRY(head, struct wg_transform_sample, entry);
    buffer = gst_sample_get_buffer(transform_sample->sample);

    if (read_sample->format)
    {
        if (!(caps = gst_sample_get_caps(transform_sample->sample)))
            caps = transform->sink_caps;
        wg_format_from_caps(&buffer_format, caps);
        if (!wg_format_compare(read_sample->format, &buffer_format))
        {
            *read_sample->format = buffer_format;
            read_sample->size = gst_buffer_get_size(buffer);
            pthread_mutex_unlock(&transform->mutex);
            return MF_E_TRANSFORM_STREAM_CHANGE;
        }

        if (buffer_format.major_type == WG_MAJOR_TYPE_VIDEO
                && buffer_format.u.video.fps_n <= 1
                && buffer_format.u.video.fps_d <= 1)
            broken_timestamp = true;
    }

    gst_buffer_map(buffer, &info, GST_MAP_READ);
    if (read_sample->size > info.size)
        read_sample->size = info.size;
    memcpy(read_sample->data, info.data, read_sample->size);
    gst_buffer_unmap(buffer, &info);

    if (buffer->pts != GST_CLOCK_TIME_NONE && !broken_timestamp)
    {
        read_sample->flags |= WG_SAMPLE_FLAG_HAS_PTS;
        read_sample->pts = buffer->pts / 100;
    }
    if (buffer->duration != GST_CLOCK_TIME_NONE && !broken_timestamp)
    {
        read_sample->flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        read_sample->duration = buffer->duration / 100;
    }

    if (info.size > read_sample->size)
    {
        read_sample->flags |= WG_SAMPLE_FLAG_INCOMPLETE;
        gst_buffer_resize(buffer, read_sample->size, -1);
    }
    else
    {
        gst_sample_unref(transform_sample->sample);
        list_remove(&transform_sample->entry);
        free(transform_sample);
    }
    pthread_mutex_unlock(&transform->mutex);

    GST_INFO("Read %u bytes, flags %#x", read_sample->size, read_sample->flags);
    return S_OK;
}
