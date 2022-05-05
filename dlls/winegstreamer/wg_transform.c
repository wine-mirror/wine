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

struct wg_transform
{
    GstElement *container;
    GstPad *my_src, *my_sink;
    GstPad *their_sink, *their_src;
    GstSegment segment;
    GstBufferList *input;
    guint input_max_length;
    GstAtomicQueue *output_queue;
    GstBuffer *output_buffer;
};

static GstFlowReturn transform_sink_chain_cb(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    struct wg_transform *transform = gst_pad_get_element_private(pad);

    GST_LOG("transform %p, buffer %p.", transform, buffer);

    gst_atomic_queue_push(transform->output_queue, buffer);

    return GST_FLOW_OK;
}

NTSTATUS wg_transform_destroy(void *args)
{
    struct wg_transform *transform = args;
    GstBuffer *buffer;

    if (transform->input)
        gst_buffer_list_unref(transform->input);

    gst_element_set_state(transform->container, GST_STATE_NULL);

    if (transform->output_buffer)
        gst_buffer_unref(transform->output_buffer);
    while ((buffer = gst_atomic_queue_pop(transform->output_queue)))
        gst_buffer_unref(buffer);

    g_object_unref(transform->their_sink);
    g_object_unref(transform->their_src);
    g_object_unref(transform->container);
    g_object_unref(transform->my_sink);
    g_object_unref(transform->my_src);
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

NTSTATUS wg_transform_create(void *args)
{
    struct wg_transform_create_params *params = args;
    GstCaps *raw_caps = NULL, *src_caps = NULL, *sink_caps = NULL;
    struct wg_format output_format = *params->output_format;
    struct wg_format input_format = *params->input_format;
    GstElement *first = NULL, *last = NULL, *element;
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
    if (!(transform->input = gst_buffer_list_new()))
        goto out;
    if (!(transform->output_queue = gst_atomic_queue_new(8)))
        goto out;
    transform->input_max_length = 1;

    if (!(src_caps = wg_format_to_caps(&input_format)))
        goto out;
    if (!(template = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps)))
        goto out;
    transform->my_src = gst_pad_new_from_template(template, "src");
    g_object_unref(template);
    if (!transform->my_src)
        goto out;

    if (!(sink_caps = wg_format_to_caps(&output_format)))
        goto out;
    if (!(template = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps)))
        goto out;
    transform->my_sink = gst_pad_new_from_template(template, "sink");
    g_object_unref(template);
    if (!transform->my_sink)
        goto out;

    gst_pad_set_element_private(transform->my_sink, transform);
    gst_pad_set_chain_function(transform->my_sink, transform_sink_chain_cb);

    /* Since we append conversion elements, we don't want to filter decoders
     * based on the actual output caps now. Matching decoders with the
     * raw output media type should be enough.
     */
    media_type = gst_structure_get_name(gst_caps_get_structure(sink_caps, 0));
    if (!(raw_caps = gst_caps_new_empty_simple(media_type)))
        goto out;

    switch (input_format.major_type)
    {
        case WG_MAJOR_TYPE_H264:
            /* Call of Duty: Black Ops 3 doesn't care about the ProcessInput/ProcessOutput
             * return values, it calls them in a specific order and expects the decoder
             * transform to be able to queue its input buffers. We need to use a buffer list
             * to match its expectations.
             */
            transform->input_max_length = 16;
            if (!(element = create_element("h264parse", "base"))
                    || !transform_append_element(transform, element, &first, &last))
                goto out;
            /* fallthrough */
        case WG_MAJOR_TYPE_WMA:
            if (!(element = transform_find_element(GST_ELEMENT_FACTORY_TYPE_DECODER, src_caps, raw_caps))
                    || !transform_append_element(transform, element, &first, &last))
            {
                gst_caps_unref(raw_caps);
                goto out;
            }
            break;

        case WG_MAJOR_TYPE_MPEG1_AUDIO:
        case WG_MAJOR_TYPE_AUDIO:
        case WG_MAJOR_TYPE_VIDEO:
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
            break;

        case WG_MAJOR_TYPE_MPEG1_AUDIO:
        case WG_MAJOR_TYPE_H264:
        case WG_MAJOR_TYPE_WMA:
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

    gst_caps_unref(sink_caps);
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
    if (sink_caps)
        gst_caps_unref(sink_caps);
    if (transform->my_src)
        gst_object_unref(transform->my_src);
    if (src_caps)
        gst_caps_unref(src_caps);
    if (transform->output_queue)
        gst_atomic_queue_unref(transform->output_queue);
    if (transform->input)
        gst_buffer_list_unref(transform->input);
    if (transform->container)
    {
        gst_element_set_state(transform->container, GST_STATE_NULL);
        gst_object_unref(transform->container);
    }
    free(transform);
    GST_ERROR("Failed to create winegstreamer transform.");
    return status;
}

NTSTATUS wg_transform_push_data(void *args)
{
    struct wg_transform_push_data_params *params = args;
    struct wg_transform *transform = params->transform;
    struct wg_sample *sample = params->sample;
    GstBuffer *buffer;
    guint length;

    length = gst_buffer_list_length(transform->input);
    if (length >= transform->input_max_length)
    {
        GST_INFO("Refusing %u bytes, %u buffers already queued", sample->size, length);
        params->result = MF_E_NOTACCEPTING;
        return STATUS_SUCCESS;
    }

    if (!(buffer = gst_buffer_new_and_alloc(sample->size)))
    {
        GST_ERROR("Failed to allocate input buffer");
        return STATUS_NO_MEMORY;
    }
    gst_buffer_fill(buffer, 0, sample->data, sample->size);
    if (sample->flags & WG_SAMPLE_FLAG_HAS_PTS)
        GST_BUFFER_PTS(buffer) = sample->pts * 100;
    if (sample->flags & WG_SAMPLE_FLAG_HAS_DURATION)
        GST_BUFFER_DURATION(buffer) = sample->duration * 100;
    if (!(sample->flags & WG_SAMPLE_FLAG_SYNC_POINT))
        GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    gst_buffer_list_insert(transform->input, -1, buffer);

    GST_INFO("Copied %u bytes from sample %p to input buffer list", sample->size, sample);
    params->result = S_OK;
    return STATUS_SUCCESS;
}

static NTSTATUS read_transform_output_data(GstBuffer *buffer, struct wg_sample *sample)
{
    GstMapInfo info;

    if (!gst_buffer_map(buffer, &info, GST_MAP_READ))
    {
        GST_ERROR("Failed to map buffer %p", buffer);
        return STATUS_UNSUCCESSFUL;
    }

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

    if (GST_BUFFER_PTS_IS_VALID(buffer))
    {
        sample->flags |= WG_SAMPLE_FLAG_HAS_PTS;
        sample->pts = GST_BUFFER_PTS(buffer) / 100;
    }
    if (GST_BUFFER_DURATION_IS_VALID(buffer))
    {
        GstClockTime duration = GST_BUFFER_DURATION(buffer) / 100;

        duration = (duration * sample->size) / info.size;
        GST_BUFFER_DURATION(buffer) -= duration * 100;
        if (GST_BUFFER_PTS_IS_VALID(buffer))
            GST_BUFFER_PTS(buffer) += duration * 100;

        sample->flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        sample->duration = duration;
    }
    if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))
        sample->flags |= WG_SAMPLE_FLAG_SYNC_POINT;

    GST_INFO("Copied %u bytes, sample %p, flags %#x", sample->size, sample, sample->flags);
    return STATUS_SUCCESS;
}

NTSTATUS wg_transform_read_data(void *args)
{
    struct wg_transform_read_data_params *params = args;
    struct wg_transform *transform = params->transform;
    struct wg_sample *sample = params->sample;
    GstBufferList *input = transform->input;
    GstFlowReturn ret;
    NTSTATUS status;

    if (!gst_buffer_list_length(transform->input))
        GST_DEBUG("Not input buffer queued");
    else if (!(transform->input = gst_buffer_list_new()))
    {
        GST_ERROR("Failed to allocate new input queue");
        return STATUS_NO_MEMORY;
    }
    else if ((ret = gst_pad_push_list(transform->my_src, input)))
    {
        GST_ERROR("Failed to push transform input, error %d", ret);
        return STATUS_UNSUCCESSFUL;
    }

    if (!transform->output_buffer && !(transform->output_buffer = gst_atomic_queue_pop(transform->output_queue)))
    {
        sample->size = 0;
        params->result = MF_E_TRANSFORM_NEED_MORE_INPUT;
        GST_INFO("Cannot read %u bytes, no output available", sample->max_size);
        return STATUS_SUCCESS;
    }

    if ((status = read_transform_output_data(transform->output_buffer, sample)))
    {
        sample->size = 0;
        return status;
    }

    if (!(sample->flags & WG_SAMPLE_FLAG_INCOMPLETE))
    {
        gst_buffer_unref(transform->output_buffer);
        transform->output_buffer = NULL;
    }

    params->result = S_OK;
    return STATUS_SUCCESS;
}
