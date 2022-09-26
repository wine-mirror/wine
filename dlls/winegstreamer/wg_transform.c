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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "mferror.h"

#include "unix_private.h"

GST_DEBUG_CATEGORY_EXTERN(wine);
#define GST_CAT_DEFAULT wine

#define GST_SAMPLE_FLAG_WG_CAPS_CHANGED (GST_MINI_OBJECT_FLAG_LAST << 0)

struct wg_transform
{
    GstElement *container;
    GstAllocator *allocator;
    GstPad *my_src, *my_sink;
    GstPad *their_sink, *their_src;
    GstSegment segment;
    GstQuery *drain_query;

    guint input_max_length;
    GstAtomicQueue *input_queue;

    guint output_plane_align;
    struct wg_sample *output_wg_sample;
    GstAtomicQueue *output_queue;
    GstSample *output_sample;
    bool output_caps_changed;
    GstCaps *output_caps;
};

static bool is_caps_video(GstCaps *caps)
{
    const gchar *media_type;

    if (!caps || !gst_caps_get_size(caps))
        return false;

    media_type = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    return g_str_has_prefix(media_type, "video/");
}

static void align_video_info_planes(gsize plane_align, GstVideoInfo *info, GstVideoAlignment *align)
{
    gst_video_alignment_reset(align);

    align->padding_right = ((plane_align + 1) - (info->width & plane_align)) & plane_align;
    align->padding_bottom = ((plane_align + 1) - (info->height & plane_align)) & plane_align;
    align->stride_align[0] = plane_align;
    align->stride_align[1] = plane_align;
    align->stride_align[2] = plane_align;
    align->stride_align[3] = plane_align;

    gst_video_info_align(info, align);
}

static GstFlowReturn transform_sink_chain_cb(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    struct wg_transform *transform = gst_pad_get_element_private(pad);
    GstSample *sample;

    GST_LOG("transform %p, buffer %p.", transform, buffer);

    if (!(sample = gst_sample_new(buffer, transform->output_caps, NULL, NULL)))
    {
        GST_ERROR("Failed to allocate transform %p output sample.", transform);
        gst_buffer_unref(buffer);
        return GST_FLOW_ERROR;
    }

    if (transform->output_caps_changed)
        GST_MINI_OBJECT_FLAG_SET(sample, GST_SAMPLE_FLAG_WG_CAPS_CHANGED);
    transform->output_caps_changed = false;

    gst_atomic_queue_push(transform->output_queue, sample);
    gst_buffer_unref(buffer);
    return GST_FLOW_OK;
}

static gboolean transform_sink_query_cb(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct wg_transform *transform = gst_pad_get_element_private(pad);

    GST_LOG("transform %p, type \"%s\".", transform, gst_query_type_get_name(query->type));

    switch (query->type)
    {
        case GST_QUERY_ALLOCATION:
        {
            gsize plane_align = transform->output_plane_align;
            GstStructure *config, *params;
            GstVideoAlignment align;
            gboolean needs_pool;
            GstBufferPool *pool;
            GstVideoInfo info;
            GstCaps *caps;

            gst_query_parse_allocation(query, &caps, &needs_pool);
            if (!is_caps_video(caps) || !needs_pool)
                break;

            if (!gst_video_info_from_caps(&info, caps)
                    || !(pool = gst_video_buffer_pool_new()))
                break;

            align_video_info_planes(plane_align, &info, &align);

            if ((params = gst_structure_new("video-meta",
                    "padding-top", G_TYPE_UINT, align.padding_top,
                    "padding-bottom", G_TYPE_UINT, align.padding_bottom,
                    "padding-left", G_TYPE_UINT, align.padding_left,
                    "padding-right", G_TYPE_UINT, align.padding_right,
                    NULL)))
                gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, params);

            if (!(config = gst_buffer_pool_get_config(pool)))
                GST_ERROR("Failed to get pool %p config.", pool);
            else
            {
                gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
                gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT);
                gst_buffer_pool_config_set_video_alignment(config, &align);

                gst_buffer_pool_config_set_params(config, caps,
                        info.size, 0, 0);
                gst_buffer_pool_config_set_allocator(config, transform->allocator, NULL);
                if (!gst_buffer_pool_set_config(pool, config))
                    GST_ERROR("Failed to set pool %p config.", pool);
            }

            /* Prevent pool reconfiguration, we don't want another alignment. */
            if (!gst_buffer_pool_set_active(pool, true))
                GST_ERROR("Pool %p failed to activate.", pool);

            gst_query_add_allocation_pool(query, pool, info.size, 0, 0);
            gst_query_add_allocation_param(query, transform->allocator, NULL);

            GST_INFO("Proposing pool %p, buffer size %#zx, allocator %p, for query %p.",
                    pool, info.size, transform->allocator, query);

            g_object_unref(pool);
            return true;
        }

        case GST_QUERY_CAPS:
        {
            GstCaps *caps, *filter, *temp;
            gchar *str;

            gst_query_parse_caps(query, &filter);
            caps = gst_caps_ref(transform->output_caps);

            if (filter)
            {
                temp = gst_caps_intersect(caps, filter);
                gst_caps_unref(caps);
                caps = temp;
            }

            str = gst_caps_to_string(caps);
            GST_INFO("Returning caps %s", str);
            g_free(str);

            gst_query_set_caps_result(query, caps);
            gst_caps_unref(caps);
            return true;
        }

        default:
            GST_WARNING("Ignoring \"%s\" query.", gst_query_type_get_name(query->type));
            break;
    }

    return gst_pad_query_default(pad, parent, query);
}

static gboolean transform_sink_event_cb(GstPad *pad, GstObject *parent, GstEvent *event)
{
    struct wg_transform *transform = gst_pad_get_element_private(pad);

    GST_LOG("transform %p, type \"%s\".", transform, GST_EVENT_TYPE_NAME(event));

    switch (event->type)
    {
        case GST_EVENT_CAPS:
        {
            GstCaps *caps;

            gst_event_parse_caps(event, &caps);

            transform->output_caps_changed = transform->output_caps_changed
                    || !gst_caps_is_always_compatible(transform->output_caps, caps);

            gst_caps_unref(transform->output_caps);
            transform->output_caps = gst_caps_ref(caps);
            break;
        }
        default:
            GST_WARNING("Ignoring \"%s\" event.", GST_EVENT_TYPE_NAME(event));
            break;
    }

    gst_event_unref(event);
    return TRUE;
}

NTSTATUS wg_transform_destroy(void *args)
{
    struct wg_transform *transform = args;
    GstSample *sample;
    GstBuffer *buffer;

    while ((buffer = gst_atomic_queue_pop(transform->input_queue)))
        gst_buffer_unref(buffer);
    gst_atomic_queue_unref(transform->input_queue);

    gst_element_set_state(transform->container, GST_STATE_NULL);

    if (transform->output_sample)
        gst_sample_unref(transform->output_sample);
    while ((sample = gst_atomic_queue_pop(transform->output_queue)))
        gst_sample_unref(sample);

    wg_allocator_destroy(transform->allocator);
    g_object_unref(transform->their_sink);
    g_object_unref(transform->their_src);
    g_object_unref(transform->container);
    g_object_unref(transform->my_sink);
    g_object_unref(transform->my_src);
    gst_query_unref(transform->drain_query);
    gst_caps_unref(transform->output_caps);
    gst_atomic_queue_unref(transform->output_queue);
    free(transform);

    return STATUS_SUCCESS;
}

static GstElement *transform_find_element(GstElementFactoryListType type, GstCaps *src_caps, GstCaps *sink_caps)
{
    GstElement *element = NULL;
    GList *tmp, *transforms;
    const gchar *name;

    if (!(transforms = gst_element_factory_list_get_elements(type, GST_RANK_MARGINAL)))
        goto done;

    tmp = gst_element_factory_list_filter(transforms, src_caps, GST_PAD_SINK, FALSE);
    gst_plugin_feature_list_free(transforms);
    if (!(transforms = tmp))
        goto done;

    tmp = gst_element_factory_list_filter(transforms, sink_caps, GST_PAD_SRC, FALSE);
    gst_plugin_feature_list_free(transforms);
    if (!(transforms = tmp))
        goto done;

    transforms = g_list_sort(transforms, gst_plugin_feature_rank_compare_func);
    for (tmp = transforms; tmp != NULL && element == NULL; tmp = tmp->next)
    {
        name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(tmp->data));
        if (!(element = gst_element_factory_create(GST_ELEMENT_FACTORY(tmp->data), NULL)))
            GST_WARNING("Failed to create %s element.", name);
    }
    gst_plugin_feature_list_free(transforms);

done:
    if (element)
    {
        GST_DEBUG("Created %s element %p.", name, element);
    }
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
    bool success = false;

    if (!gst_bin_add(GST_BIN(transform->container), element) ||
            (*last && !gst_element_link(*last, element)))
    {
        GST_ERROR("Failed to link %s element.", name);
    }
    else
    {
        GST_DEBUG("Linked %s element %p.", name, element);
        if (!*first)
            *first = element;
        *last = element;
        success = true;
    }

    g_free(name);
    return success;
}

static struct wg_sample *transform_request_sample(gsize size, void *context)
{
    struct wg_transform *transform = context;

    GST_LOG("size %#zx, context %p", size, transform);

    return InterlockedExchangePointer((void **)&transform->output_wg_sample, NULL);
}

NTSTATUS wg_transform_create(void *args)
{
    struct wg_transform_create_params *params = args;
    struct wg_format output_format = *params->output_format;
    struct wg_format input_format = *params->input_format;
    GstElement *first = NULL, *last = NULL, *element;
    GstCaps *raw_caps = NULL, *src_caps = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    GstPadTemplate *template = NULL;
    struct wg_transform *transform;
    const gchar *media_type;
    GstEvent *event;

    if (!init_gstreamer())
        return STATUS_UNSUCCESSFUL;

    if (!(transform = calloc(1, sizeof(*transform))))
        return STATUS_NO_MEMORY;
    if (!(transform->container = gst_bin_new("wg_transform")))
        goto out;
    if (!(transform->input_queue = gst_atomic_queue_new(8)))
        goto out;
    if (!(transform->output_queue = gst_atomic_queue_new(8)))
        goto out;
    if (!(transform->drain_query = gst_query_new_drain()))
        goto out;
    if (!(transform->allocator = wg_allocator_create(transform_request_sample, transform)))
        goto out;
    transform->input_max_length = 1;
    transform->output_plane_align = 0;

    if (!(src_caps = wg_format_to_caps(&input_format)))
        goto out;
    if (!(template = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps)))
        goto out;
    transform->my_src = gst_pad_new_from_template(template, "src");
    g_object_unref(template);
    if (!transform->my_src)
        goto out;

    if (!(transform->output_caps = wg_format_to_caps(&output_format)))
        goto out;
    if (!(template = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, transform->output_caps)))
        goto out;
    transform->my_sink = gst_pad_new_from_template(template, "sink");
    g_object_unref(template);
    if (!transform->my_sink)
        goto out;

    gst_pad_set_element_private(transform->my_sink, transform);
    gst_pad_set_event_function(transform->my_sink, transform_sink_event_cb);
    gst_pad_set_query_function(transform->my_sink, transform_sink_query_cb);
    gst_pad_set_chain_function(transform->my_sink, transform_sink_chain_cb);

    /* Since we append conversion elements, we don't want to filter decoders
     * based on the actual output caps now. Matching decoders with the
     * raw output media type should be enough.
     */
    media_type = gst_structure_get_name(gst_caps_get_structure(transform->output_caps, 0));
    if (!(raw_caps = gst_caps_new_empty_simple(media_type)))
        goto out;

    switch (input_format.major_type)
    {
        case WG_MAJOR_TYPE_VIDEO_H264:
            /* Call of Duty: Black Ops 3 doesn't care about the ProcessInput/ProcessOutput
             * return values, it calls them in a specific order and expects the decoder
             * transform to be able to queue its input buffers. We need to use a buffer list
             * to match its expectations.
             */
            transform->input_max_length = 16;
            transform->output_plane_align = 15;
            if (!(element = create_element("h264parse", "base"))
                    || !transform_append_element(transform, element, &first, &last))
                goto out;
            /* fallthrough */
        case WG_MAJOR_TYPE_AUDIO_MPEG1:
        case WG_MAJOR_TYPE_AUDIO_WMA:
        case WG_MAJOR_TYPE_VIDEO_CINEPAK:
            if (!(element = transform_find_element(GST_ELEMENT_FACTORY_TYPE_DECODER, src_caps, raw_caps))
                    || !transform_append_element(transform, element, &first, &last))
            {
                gst_caps_unref(raw_caps);
                goto out;
            }
            break;

        case WG_MAJOR_TYPE_AUDIO:
        case WG_MAJOR_TYPE_VIDEO:
            break;
        case WG_MAJOR_TYPE_UNKNOWN:
            GST_FIXME("Format %u not implemented!", input_format.major_type);
            gst_caps_unref(raw_caps);
            goto out;
    }

    gst_caps_unref(raw_caps);

    switch (output_format.major_type)
    {
        case WG_MAJOR_TYPE_AUDIO:
            /* The MF audio decoder transforms allow decoding to various formats
             * as well as resampling the audio at the same time, whereas
             * GStreamer decoder plugins usually only support decoding to a
             * single format and at the original rate.
             *
             * The WMA decoder transform also has output samples interleaved on
             * Windows, whereas GStreamer avdec_wmav2 output uses
             * non-interleaved format.
             */
            if (!(element = create_element("audioconvert", "base"))
                    || !transform_append_element(transform, element, &first, &last))
                goto out;
            if (!(element = create_element("audioresample", "base"))
                    || !transform_append_element(transform, element, &first, &last))
                goto out;
            break;

        case WG_MAJOR_TYPE_VIDEO:
            if (!(element = create_element("videoconvert", "base"))
                    || !transform_append_element(transform, element, &first, &last))
                goto out;
            /* Let GStreamer choose a default number of threads. */
            gst_util_set_object_arg(G_OBJECT(element), "n-threads", "0");
            break;

        case WG_MAJOR_TYPE_AUDIO_MPEG1:
        case WG_MAJOR_TYPE_AUDIO_WMA:
        case WG_MAJOR_TYPE_VIDEO_CINEPAK:
        case WG_MAJOR_TYPE_VIDEO_H264:
        case WG_MAJOR_TYPE_UNKNOWN:
            GST_FIXME("Format %u not implemented!", output_format.major_type);
            goto out;
    }

    if (!(transform->their_sink = gst_element_get_static_pad(first, "sink")))
        goto out;
    if (!(transform->their_src = gst_element_get_static_pad(last, "src")))
        goto out;
    if (gst_pad_link(transform->my_src, transform->their_sink) < 0)
        goto out;
    if (gst_pad_link(transform->their_src, transform->my_sink) < 0)
        goto out;
    if (!gst_pad_set_active(transform->my_sink, 1))
        goto out;
    if (!gst_pad_set_active(transform->my_src, 1))
        goto out;

    gst_element_set_state(transform->container, GST_STATE_PAUSED);
    if (!gst_element_get_state(transform->container, NULL, NULL, -1))
        goto out;

    if (!(event = gst_event_new_stream_start("stream"))
            || !gst_pad_push_event(transform->my_src, event))
        goto out;
    if (!(event = gst_event_new_caps(src_caps))
            || !gst_pad_push_event(transform->my_src, event))
        goto out;

    /* We need to use GST_FORMAT_TIME here because it's the only format
     * some elements such avdec_wmav2 correctly support. */
    gst_segment_init(&transform->segment, GST_FORMAT_TIME);
    transform->segment.start = 0;
    transform->segment.stop = -1;
    if (!(event = gst_event_new_segment(&transform->segment))
            || !gst_pad_push_event(transform->my_src, event))
        goto out;

    gst_caps_unref(src_caps);

    GST_INFO("Created winegstreamer transform %p.", transform);
    params->transform = transform;
    return STATUS_SUCCESS;

out:
    if (transform->their_sink)
        gst_object_unref(transform->their_sink);
    if (transform->their_src)
        gst_object_unref(transform->their_src);
    if (transform->my_sink)
        gst_object_unref(transform->my_sink);
    if (transform->output_caps)
        gst_caps_unref(transform->output_caps);
    if (transform->my_src)
        gst_object_unref(transform->my_src);
    if (src_caps)
        gst_caps_unref(src_caps);
    if (transform->allocator)
        wg_allocator_destroy(transform->allocator);
    if (transform->drain_query)
        gst_query_unref(transform->drain_query);
    if (transform->output_queue)
        gst_atomic_queue_unref(transform->output_queue);
    if (transform->input_queue)
        gst_atomic_queue_unref(transform->input_queue);
    if (transform->container)
    {
        gst_element_set_state(transform->container, GST_STATE_NULL);
        gst_object_unref(transform->container);
    }
    free(transform);
    GST_ERROR("Failed to create winegstreamer transform.");
    return status;
}

NTSTATUS wg_transform_set_output_format(void *args)
{
    struct wg_transform_set_output_format_params *params = args;
    struct wg_transform *transform = params->transform;
    const struct wg_format *format = params->format;
    GstSample *sample;
    GstCaps *caps;
    gchar *str;

    if (!(caps = wg_format_to_caps(format)))
    {
        GST_ERROR("Failed to convert format %p to caps.", format);
        return STATUS_UNSUCCESSFUL;
    }

    if (gst_caps_is_always_compatible(transform->output_caps, caps))
    {
        gst_caps_unref(caps);
        return STATUS_SUCCESS;
    }

    if (!gst_pad_peer_query(transform->my_src, transform->drain_query))
    {
        GST_ERROR("Failed to drain transform %p.", transform);
        return STATUS_UNSUCCESSFUL;
    }

    gst_caps_unref(transform->output_caps);
    transform->output_caps = caps;

    if (!gst_pad_push_event(transform->my_sink, gst_event_new_reconfigure()))
    {
        GST_ERROR("Failed to reconfigure transform %p.", transform);
        return STATUS_UNSUCCESSFUL;
    }

    str = gst_caps_to_string(caps);
    GST_INFO("Configured new caps %s.", str);
    g_free(str);

    /* Ideally and to be fully compatible with native transform, the queued
     * output buffers will need to be converted to the new output format and
     * kept queued.
     */
    if (transform->output_sample)
        gst_sample_unref(transform->output_sample);
    while ((sample = gst_atomic_queue_pop(transform->output_queue)))
        gst_sample_unref(sample);
    transform->output_sample = NULL;

    return STATUS_SUCCESS;
}

static void wg_sample_free_notify(void *arg)
{
    struct wg_sample *sample = arg;
    GST_DEBUG("Releasing wg_sample %p", sample);
    InterlockedDecrement(&sample->refcount);
}

NTSTATUS wg_transform_push_data(void *args)
{
    struct wg_transform_push_data_params *params = args;
    struct wg_transform *transform = params->transform;
    struct wg_sample *sample = params->sample;
    GstBuffer *buffer;
    guint length;

    length = gst_atomic_queue_length(transform->input_queue);
    if (length >= transform->input_max_length)
    {
        GST_INFO("Refusing %u bytes, %u buffers already queued", sample->size, length);
        params->result = MF_E_NOTACCEPTING;
        return STATUS_SUCCESS;
    }

    if (!(buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, sample->data, sample->max_size,
            0, sample->size, sample, wg_sample_free_notify)))
    {
        GST_ERROR("Failed to allocate input buffer");
        return STATUS_NO_MEMORY;
    }
    else
    {
        InterlockedIncrement(&sample->refcount);
        GST_INFO("Wrapped %u/%u bytes from sample %p to buffer %p", sample->size, sample->max_size, sample, buffer);
    }

    if (sample->flags & WG_SAMPLE_FLAG_HAS_PTS)
        GST_BUFFER_PTS(buffer) = sample->pts * 100;
    if (sample->flags & WG_SAMPLE_FLAG_HAS_DURATION)
        GST_BUFFER_DURATION(buffer) = sample->duration * 100;
    if (!(sample->flags & WG_SAMPLE_FLAG_SYNC_POINT))
        GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    gst_atomic_queue_push(transform->input_queue, buffer);

    params->result = S_OK;
    return STATUS_SUCCESS;
}

static bool copy_video_buffer(GstBuffer *buffer, GstCaps *caps, gsize plane_align,
        struct wg_sample *sample, gsize *total_size)
{
    GstVideoFrame src_frame, dst_frame;
    GstVideoInfo src_info, dst_info;
    GstVideoAlignment align;
    GstBuffer *dst_buffer;
    bool ret = false;

    if (!gst_video_info_from_caps(&src_info, caps))
    {
        GST_ERROR("Failed to get video info from caps.");
        return false;
    }

    dst_info = src_info;
    align_video_info_planes(plane_align, &dst_info, &align);

    if (sample->max_size < dst_info.size)
    {
        GST_ERROR("Output buffer is too small.");
        return false;
    }

    if (!(dst_buffer = gst_buffer_new_wrapped_full(0, sample->data, sample->max_size,
            0, sample->max_size, 0, NULL)))
    {
        GST_ERROR("Failed to wrap wg_sample into GstBuffer");
        return false;
    }
    gst_buffer_set_size(dst_buffer, dst_info.size);
    *total_size = sample->size = dst_info.size;

    if (!gst_video_frame_map(&src_frame, &src_info, buffer, GST_MAP_READ))
        GST_ERROR("Failed to map source frame.");
    else
    {
        if (!gst_video_frame_map(&dst_frame, &dst_info, dst_buffer, GST_MAP_WRITE))
            GST_ERROR("Failed to map destination frame.");
        else
        {
            if (!(ret = gst_video_frame_copy(&dst_frame, &src_frame)))
                GST_ERROR("Failed to copy video frame.");
            gst_video_frame_unmap(&dst_frame);
        }
        gst_video_frame_unmap(&src_frame);
    }

    gst_buffer_unref(dst_buffer);
    return ret;
}

static bool copy_buffer(GstBuffer *buffer, GstCaps *caps, struct wg_sample *sample,
        gsize *total_size)
{
    GstMapInfo info;

    if (!gst_buffer_map(buffer, &info, GST_MAP_READ))
        return false;

    if (sample->max_size >= info.size)
        sample->size = info.size;
    else
    {
        sample->flags |= WG_SAMPLE_FLAG_INCOMPLETE;
        sample->size = sample->max_size;
    }

    memcpy(sample->data, info.data, sample->size);
    gst_buffer_unmap(buffer, &info);

    if (sample->flags & WG_SAMPLE_FLAG_INCOMPLETE)
        gst_buffer_resize(buffer, sample->size, -1);

    *total_size = info.size;
    return true;
}

static NTSTATUS read_transform_output_data(GstBuffer *buffer, GstCaps *caps, gsize plane_align,
        struct wg_sample *sample)
{
    bool ret, needs_copy;
    gsize total_size;
    GstMapInfo info;

    if (!gst_buffer_map(buffer, &info, GST_MAP_READ))
    {
        GST_ERROR("Failed to map buffer %p", buffer);
        sample->size = 0;
        return STATUS_UNSUCCESSFUL;
    }
    needs_copy = info.data != sample->data;
    gst_buffer_unmap(buffer, &info);

    if ((ret = !needs_copy))
        total_size = sample->size = info.size;
    else if (is_caps_video(caps))
        ret = copy_video_buffer(buffer, caps, plane_align, sample, &total_size);
    else
        ret = copy_buffer(buffer, caps, sample, &total_size);

    if (!ret)
    {
        GST_ERROR("Failed to copy buffer %p", buffer);
        sample->size = 0;
        return STATUS_UNSUCCESSFUL;
    }

    if (GST_BUFFER_PTS_IS_VALID(buffer))
    {
        sample->flags |= WG_SAMPLE_FLAG_HAS_PTS;
        sample->pts = GST_BUFFER_PTS(buffer) / 100;
    }
    if (GST_BUFFER_DURATION_IS_VALID(buffer))
    {
        GstClockTime duration = GST_BUFFER_DURATION(buffer) / 100;

        duration = (duration * sample->size) / total_size;
        GST_BUFFER_DURATION(buffer) -= duration * 100;
        if (GST_BUFFER_PTS_IS_VALID(buffer))
            GST_BUFFER_PTS(buffer) += duration * 100;

        sample->flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        sample->duration = duration;
    }
    if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))
        sample->flags |= WG_SAMPLE_FLAG_SYNC_POINT;

    if (needs_copy)
    {
        if (is_caps_video(caps))
            GST_WARNING("Copied %u bytes, sample %p, flags %#x", sample->size, sample, sample->flags);
        else
            GST_INFO("Copied %u bytes, sample %p, flags %#x", sample->size, sample, sample->flags);
    }
    else if (sample->flags & WG_SAMPLE_FLAG_INCOMPLETE)
        GST_ERROR("Partial read %u bytes, sample %p, flags %#x", sample->size, sample, sample->flags);
    else
        GST_INFO("Read %u bytes, sample %p, flags %#x", sample->size, sample, sample->flags);

    return STATUS_SUCCESS;
}

static bool get_transform_output(struct wg_transform *transform, struct wg_sample *sample)
{
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *input_buffer;

    /* Provide the sample for transform_request_sample to pick it up */
    InterlockedIncrement(&sample->refcount);
    InterlockedExchangePointer((void **)&transform->output_wg_sample, sample);

    while (!(transform->output_sample = gst_atomic_queue_pop(transform->output_queue)))
    {
        if (!(input_buffer = gst_atomic_queue_pop(transform->input_queue)))
            break;

        if ((ret = gst_pad_push(transform->my_src, input_buffer)))
        {
            GST_ERROR("Failed to push transform input, error %d", ret);
            break;
        }
    }

    /* Remove the sample so transform_request_sample cannot use it */
    if (InterlockedExchangePointer((void **)&transform->output_wg_sample, NULL))
        InterlockedDecrement(&sample->refcount);

    return ret == GST_FLOW_OK;
}

NTSTATUS wg_transform_read_data(void *args)
{
    struct wg_transform_read_data_params *params = args;
    struct wg_transform *transform = params->transform;
    struct wg_sample *sample = params->sample;
    struct wg_format *format = params->format;
    GstBuffer *output_buffer;
    GstCaps *output_caps;
    bool discard_data;
    NTSTATUS status;

    if (!transform->output_sample && !get_transform_output(transform, sample))
    {
        wg_allocator_release_sample(transform->allocator, sample, false);
        return STATUS_UNSUCCESSFUL;
    }

    if (!transform->output_sample)
    {
        sample->size = 0;
        params->result = MF_E_TRANSFORM_NEED_MORE_INPUT;
        GST_INFO("Cannot read %u bytes, no output available", sample->max_size);
        wg_allocator_release_sample(transform->allocator, sample, false);
        return STATUS_SUCCESS;
    }

    output_buffer = gst_sample_get_buffer(transform->output_sample);
    output_caps = gst_sample_get_caps(transform->output_sample);

    if (GST_MINI_OBJECT_FLAG_IS_SET(transform->output_sample, GST_SAMPLE_FLAG_WG_CAPS_CHANGED))
    {
        GST_MINI_OBJECT_FLAG_UNSET(transform->output_sample, GST_SAMPLE_FLAG_WG_CAPS_CHANGED);

        if (format)
        {
            gsize plane_align = transform->output_plane_align;
            GstVideoAlignment align;
            GstVideoInfo info;

            wg_format_from_caps(format, output_caps);

            if (format->major_type == WG_MAJOR_TYPE_VIDEO
                    && gst_video_info_from_caps(&info, output_caps))
            {
                align_video_info_planes(plane_align, &info, &align);

                GST_INFO("Returning video alignment left %u, top %u, right %u, bottom %u.", align.padding_left,
                        align.padding_top, align.padding_right, align.padding_bottom);

                format->u.video.padding.left = align.padding_left;
                format->u.video.width += format->u.video.padding.left;
                format->u.video.padding.right = align.padding_right;
                format->u.video.width += format->u.video.padding.right;
                format->u.video.padding.top = align.padding_top;
                format->u.video.height += format->u.video.padding.top;
                format->u.video.padding.bottom = align.padding_bottom;
                format->u.video.height += format->u.video.padding.bottom;
            }
        }

        params->result = MF_E_TRANSFORM_STREAM_CHANGE;
        GST_INFO("Format changed detected, returning no output");
        wg_allocator_release_sample(transform->allocator, sample, false);
        return STATUS_SUCCESS;
    }

    if ((status = read_transform_output_data(output_buffer, output_caps,
                transform->output_plane_align, sample)))
    {
        wg_allocator_release_sample(transform->allocator, sample, false);
        return status;
    }

    if (sample->flags & WG_SAMPLE_FLAG_INCOMPLETE)
        discard_data = false;
    else
    {
        /* Taint the buffer memory to make sure it cannot be reused by the buffer pool,
         * for the pool to always requests new memory from the allocator, and so we can
         * then always provide output sample memory to achieve zero-copy.
         *
         * However, some decoder keep a reference on the buffer they passed downstream,
         * to re-use it later. In this case, it will not be possible to do zero-copy,
         * and we should copy the data back to the buffer and leave it unchanged.
         *
         * Some other plugins make assumptions that the returned buffer will always have
         * at least one memory attached, we cannot just remove it and need to replace the
         * memory instead.
         */
        if ((discard_data = gst_buffer_is_writable(output_buffer)))
            gst_buffer_replace_all_memory(output_buffer, gst_allocator_alloc(NULL, 0, NULL));

        gst_sample_unref(transform->output_sample);
        transform->output_sample = NULL;
    }

    params->result = S_OK;
    wg_allocator_release_sample(transform->allocator, sample, discard_data);
    return STATUS_SUCCESS;
}
