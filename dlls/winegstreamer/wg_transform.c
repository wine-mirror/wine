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
#include "dshow.h"

#include "unix_private.h"

GST_DEBUG_CATEGORY_EXTERN(wine);
#define GST_CAT_DEFAULT wine

struct wg_transform
{
    GstElement *container;
    GstPad *my_src, *my_sink;
};

static GstFlowReturn transform_sink_chain_cb(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    struct wg_transform *transform = gst_pad_get_element_private(pad);

    GST_INFO("transform %p, buffer %p.", transform, buffer);

    gst_buffer_unref(buffer);

    return GST_FLOW_OK;
}

NTSTATUS wg_transform_destroy(void *args)
{
    struct wg_transform *transform = args;

    gst_element_set_state(transform->container, GST_STATE_NULL);
    g_object_unref(transform->container);
    g_object_unref(transform->my_sink);
    g_object_unref(transform->my_src);
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

    if (!init_gstreamer())
        return STATUS_UNSUCCESSFUL;

    if (!(transform = calloc(1, sizeof(*transform))))
        return STATUS_NO_MEMORY;
    if (!(transform->container = gst_bin_new("wg_transform")))
        goto out;

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
        case WG_MAJOR_TYPE_WMA:
            if (!(element = transform_find_element(GST_ELEMENT_FACTORY_TYPE_DECODER, src_caps, raw_caps))
                    || !transform_append_element(transform, element, &first, &last))
            {
                gst_caps_unref(raw_caps);
                goto out;
            }
            break;

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
        case WG_MAJOR_TYPE_WMA:
        case WG_MAJOR_TYPE_UNKNOWN:
            GST_FIXME("Format %u not implemented!", output_format.major_type);
            goto out;
    }

    gst_element_set_state(transform->container, GST_STATE_PAUSED);
    if (!gst_element_get_state(transform->container, NULL, NULL, -1))
        goto out;

    gst_caps_unref(sink_caps);
    gst_caps_unref(src_caps);

    GST_INFO("Created winegstreamer transform %p.", transform);
    params->transform = transform;
    return STATUS_SUCCESS;

out:
    if (transform->my_sink)
        gst_object_unref(transform->my_sink);
    if (sink_caps)
        gst_caps_unref(sink_caps);
    if (transform->my_src)
        gst_object_unref(transform->my_src);
    if (src_caps)
        gst_caps_unref(src_caps);
    if (transform->container)
        gst_object_unref(transform->container);
    free(transform);
    GST_ERROR("Failed to create winegstreamer transform.");
    return status;
}
