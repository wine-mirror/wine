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

    if (transform->my_sink)
        g_object_unref(transform->my_sink);
    if (transform->my_src)
        g_object_unref(transform->my_src);

    free(transform);
    return STATUS_SUCCESS;
}

NTSTATUS wg_transform_create(void *args)
{
    struct wg_transform_create_params *params = args;
    struct wg_format output_format = *params->output_format;
    struct wg_format input_format = *params->input_format;
    GstCaps *src_caps = NULL, *sink_caps = NULL;
    GstPadTemplate *template = NULL;
    struct wg_transform *transform;
    NTSTATUS status;

    if (!init_gstreamer())
        return STATUS_UNSUCCESSFUL;

    status = STATUS_NO_MEMORY;

    if (!(transform = calloc(1, sizeof(*transform))))
        goto done;

    if (!(src_caps = wg_format_to_caps(&input_format)))
        goto done;
    if (!(sink_caps = wg_format_to_caps(&output_format)))
        goto done;

    if (!(template = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps)))
        goto done;
    if (!(transform->my_src = gst_pad_new_from_template(template, "src")))
        goto done;
    g_object_unref(template);
    template = NULL;

    if (!(template = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps)))
        goto done;
    if (!(transform->my_sink = gst_pad_new_from_template(template, "sink")))
        goto done;
    g_object_unref(template);
    template = NULL;

    gst_pad_set_element_private(transform->my_sink, transform);
    gst_pad_set_chain_function(transform->my_sink, transform_sink_chain_cb);

    status = STATUS_SUCCESS;

done:
    if (template)
        g_object_unref(template);
    if (sink_caps)
        gst_caps_unref(sink_caps);
    if (src_caps)
        gst_caps_unref(src_caps);

    if (status)
    {
        GST_ERROR("Failed to create winegstreamer transform.");
        if (transform)
            wg_transform_destroy(transform);
    }
    else
    {
        GST_INFO("Created winegstreamer transform %p.", transform);
        params->transform = transform;
    }

    return status;
}
