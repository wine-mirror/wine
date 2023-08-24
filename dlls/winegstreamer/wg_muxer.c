/*
 * GStreamer muxer backend
 *
 * Copyright 2023 Ziqing Hui for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "unix_private.h"

struct wg_muxer
{
    GstElement *container, *muxer;
    GstPad *my_sink;
};

static struct wg_muxer *get_muxer(wg_muxer_t muxer)
{
    return (struct wg_muxer *)(ULONG_PTR)muxer;
}

static gboolean muxer_sink_query_cb(GstPad *pad, GstObject *parent, GstQuery *query)
{
    struct wg_muxer *muxer = gst_pad_get_element_private(pad);

    GST_DEBUG("pad %p, parent %p, query %p, muxer %p, type \"%s\".",
            pad, parent, query, muxer, gst_query_type_get_name(query->type));

    switch (query->type)
    {
        case GST_QUERY_SEEKING:
            gst_query_set_seeking(query, GST_FORMAT_BYTES, true, 0, -1);
            return true;
        default:
            GST_WARNING("Ignoring \"%s\" query.", gst_query_type_get_name(query->type));
            return gst_pad_query_default(pad, parent, query);
    }
}

NTSTATUS wg_muxer_create(void *args)
{
    struct wg_muxer_create_params *params = args;
    GstElement *first = NULL, *last = NULL;
    GstPadTemplate *template = NULL;
    GstCaps *sink_caps = NULL;
    NTSTATUS status = E_FAIL;
    struct wg_muxer *muxer;

    /* Create wg_muxer object. */
    if (!(muxer = calloc(1, sizeof(*muxer))))
        return E_OUTOFMEMORY;
    if (!(muxer->container = gst_bin_new("wg_muxer")))
        goto out;

    /* Create sink pad. */
    if (!(sink_caps = gst_caps_from_string(params->format)))
    {
        GST_ERROR("Failed to get caps from format string: \"%s\".", params->format);
        goto out;
    }
    if (!(template = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps)))
        goto out;
    muxer->my_sink = gst_pad_new_from_template(template, "wg_muxer_sink");
    if (!muxer->my_sink)
        goto out;
    gst_pad_set_element_private(muxer->my_sink, muxer);
    gst_pad_set_query_function(muxer->my_sink, muxer_sink_query_cb);

    /* Create gstreamer muxer element. */
    if (!(muxer->muxer = find_element(GST_ELEMENT_FACTORY_TYPE_MUXER | GST_ELEMENT_FACTORY_TYPE_FORMATTER,
            NULL, sink_caps)))
        goto out;
    if (!append_element(muxer->container, muxer->muxer, &first, &last))
        goto out;

    /* Link muxer to sink pad. */
    if (!link_element_to_sink(muxer->muxer, muxer->my_sink))
        goto out;
    if (!gst_pad_set_active(muxer->my_sink, 1))
        goto out;

    /* Set to pause state. */
    gst_element_set_state(muxer->container, GST_STATE_PAUSED);
    if (!gst_element_get_state(muxer->container, NULL, NULL, -1))
        goto out;

    gst_object_unref(template);
    gst_caps_unref(sink_caps);

    GST_INFO("Created winegstreamer muxer %p.", muxer);
    params->muxer = (wg_transform_t)(ULONG_PTR)muxer;

    return S_OK;

out:
    if (muxer->my_sink)
        gst_object_unref(muxer->my_sink);
    if (template)
        gst_object_unref(template);
    if (sink_caps)
        gst_caps_unref(sink_caps);
    if (muxer->container)
    {
        gst_element_set_state(muxer->container, GST_STATE_NULL);
        gst_object_unref(muxer->container);
    }
    free(muxer);

    return status;
}

NTSTATUS wg_muxer_destroy(void *args)
{
    struct wg_muxer *muxer = get_muxer(*(wg_muxer_t *)args);

    gst_object_unref(muxer->my_sink);
    gst_element_set_state(muxer->container, GST_STATE_NULL);
    gst_object_unref(muxer->container);
    free(muxer);

    return S_OK;
}
