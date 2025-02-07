/*
 * winegstreamer Unix library interface
 *
 * Copyright 2020-2021 Zebediah Figura for CodeWeavers
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

/* GStreamer callbacks may be called on threads not created by Wine, and
 * therefore cannot access the Wine TEB. This means that we must use GStreamer
 * debug logging instead of Wine debug logging. In order to be safe we forbid
 * any use of Wine debug logging in this entire file. */

GST_DEBUG_CATEGORY(wine);

static UINT thread_count;

GstStreamType stream_type_from_caps(GstCaps *caps)
{
    const gchar *media_type;

    if (!caps || !gst_caps_get_size(caps))
        return GST_STREAM_TYPE_UNKNOWN;

    media_type = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    if (g_str_has_prefix(media_type, "video/")
            || g_str_has_prefix(media_type, "image/"))
        return GST_STREAM_TYPE_VIDEO;
    if (g_str_has_prefix(media_type, "audio/"))
        return GST_STREAM_TYPE_AUDIO;
    if (g_str_has_prefix(media_type, "text/")
            || g_str_has_prefix(media_type, "subpicture/")
            || g_str_has_prefix(media_type, "closedcaption/"))
        return GST_STREAM_TYPE_TEXT;

    return GST_STREAM_TYPE_UNKNOWN;
}

GstElement *create_element(const char *name, const char *plugin_set)
{
    GstElement *element;

    if (!(element = gst_element_factory_make(name, NULL)))
        fprintf(stderr, "winegstreamer: failed to create %s, are %u-bit GStreamer \"%s\" plugins installed?\n",
                name, 8 * (unsigned int)sizeof(void *), plugin_set);
    return element;
}

GstElement *factory_create_element(GstElementFactory *factory)
{
    GstElement *element;

    if ((element = gst_element_factory_create(factory, NULL)))
        GST_INFO("Created element %"GST_PTR_FORMAT" from factory %"GST_PTR_FORMAT".",
                element, factory);
    else
        GST_WARNING("Failed to create element from factory %"GST_PTR_FORMAT".", factory);

    return element;
}

GList *find_element_factories(GstElementFactoryListType type, GstRank min_rank,
        GstCaps *element_sink_caps, GstCaps *element_src_caps)
{
    GList *tmp, *factories = NULL;

    if (!(factories = gst_element_factory_list_get_elements(type, min_rank)))
        goto done;

    if (element_sink_caps)
    {
        tmp = gst_element_factory_list_filter(factories, element_sink_caps, GST_PAD_SINK, FALSE);
        gst_plugin_feature_list_free(factories);
        if (!(factories = tmp))
            goto done;
    }

    if (element_src_caps)
    {
        tmp = gst_element_factory_list_filter(factories, element_src_caps, GST_PAD_SRC, FALSE);
        gst_plugin_feature_list_free(factories);
        if (!(factories = tmp))
            goto done;
    }

    factories = g_list_sort(factories, gst_plugin_feature_rank_compare_func);

done:
    if (!factories)
        GST_WARNING("Failed to find any element factory matching "
                "type %"G_GUINT64_FORMAT"x, caps %"GST_PTR_FORMAT" / %"GST_PTR_FORMAT".",
                type, element_sink_caps, element_src_caps);

    return factories;
}

GstElement *find_element(GstElementFactoryListType type, GstCaps *element_sink_caps, GstCaps *element_src_caps)
{
    GstElement *element = NULL;
    GList *tmp, *transforms;
    const gchar *name;

    if (!(transforms = find_element_factories(type, GST_RANK_MARGINAL, element_sink_caps, element_src_caps)))
        return NULL;

    for (tmp = transforms; tmp != NULL && element == NULL; tmp = tmp->next)
    {
        name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(tmp->data));

        if (!strcmp(name, "vaapidecodebin"))
        {
            /* vaapidecodebin adds asynchronicity which breaks wg_transform synchronous drain / flush
             * requirements. Ignore it and use VA-API decoders directly instead.
             */
            GST_WARNING("Ignoring vaapidecodebin decoder.");
            continue;
        }

        element = factory_create_element(GST_ELEMENT_FACTORY(tmp->data));
    }

    gst_plugin_feature_list_free(transforms);

    if (!element)
        GST_WARNING("Failed to create element matching caps %"GST_PTR_FORMAT" / %"GST_PTR_FORMAT".",
                element_sink_caps, element_src_caps);

    return element;
}

bool append_element(GstElement *container, GstElement *element, GstElement **first, GstElement **last)
{
    gchar *name = gst_element_get_name(element);
    bool success = false;

    if (!gst_bin_add(GST_BIN(container), element) ||
            !gst_element_sync_state_with_parent(element) ||
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

bool link_src_to_sink(GstPad *src_pad, GstPad *sink_pad)
{
    GstPadLinkReturn ret;

    if ((ret = gst_pad_link(src_pad, sink_pad)) != GST_PAD_LINK_OK)
    {
        GST_ERROR("Failed to link src pad %"GST_PTR_FORMAT" to sink pad %"GST_PTR_FORMAT", reason: %s",
                src_pad, sink_pad, gst_pad_link_get_name(ret));
        return false;
    }

    return true;
}

bool link_src_to_element(GstPad *src_pad, GstElement *element)
{
    GstPadLinkReturn ret;
    GstPad *sink_pad;

    if (!(sink_pad = gst_element_get_compatible_pad(element, src_pad, NULL)))
    {
        GST_ERROR("Failed to find sink pad compatible to %"GST_PTR_FORMAT" on %"GST_PTR_FORMAT".",
                src_pad, element);
        return false;
    }
    ret = link_src_to_sink(src_pad, sink_pad);
    gst_object_unref(sink_pad);
    return ret;
}

bool link_element_to_sink(GstElement *element, GstPad *sink_pad)
{
    GstPadLinkReturn ret;
    GstPad *src_pad;

    if (!(src_pad = gst_element_get_compatible_pad(element, sink_pad, NULL)))
    {
        GST_ERROR("Failed to find src pad compatible to %"GST_PTR_FORMAT" on %"GST_PTR_FORMAT".",
                sink_pad, element);
        return false;
    }
    ret = link_src_to_sink(src_pad, sink_pad);
    gst_object_unref(src_pad);
    return ret;
}

bool push_event(GstPad *pad, GstEvent *event)
{
    if (!gst_pad_push_event(pad, event))
    {
        const gchar *type_name = gst_event_type_get_name(GST_EVENT_TYPE(event));
        gchar *pad_name = gst_pad_get_name(pad);

        GST_ERROR("Failed to push %s event %p to pad %s.", type_name, event, pad_name);
        g_free(pad_name);
        return false;
    }
    return true;
}

static ULONG popcount(ULONG val)
{
#if HAVE___BUILTIN_POPCOUNT
    return __builtin_popcount(val);
#else
    val -= val >> 1 & 0x55555555;
    val = (val & 0x33333333) + (val >> 2 & 0x33333333);
    return ((val + (val >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

NTSTATUS wg_init_gstreamer(void *arg)
{
    struct wg_init_gstreamer_params *params = arg;
    char arg0[] = "wine";
    char arg1[] = "--gst-disable-registry-fork";
    char *args[] = {arg0, arg1, NULL};
    int argc = ARRAY_SIZE(args) - 1;
    char **argv = args;
    GError *err;
    DWORD_PTR process_mask;

    if (params->trace_on)
        setenv("GST_DEBUG", "WINE:9,4", FALSE);
    if (params->warn_on)
        setenv("GST_DEBUG", "3", FALSE);
    if (params->err_on)
        setenv("GST_DEBUG", "1", FALSE);
    setenv("GST_DEBUG_NO_COLOR", "1", FALSE);

    /* GStreamer installs a temporary SEGV handler when it loads plugins
     * to initialize its registry calling exit(-1) when any fault is caught.
     * We need to make sure any signal reaches our signal handlers to catch
     * and handle them, or eventually propagate the exceptions to the user.
     */
    gst_segtrap_set_enabled(false);

    if (!gst_init_check(&argc, &argv, &err))
    {
        fprintf(stderr, "winegstreamer: failed to initialize GStreamer: %s\n", err->message);
        g_error_free(err);
        return STATUS_UNSUCCESSFUL;
    }

    if (!NtQueryInformationProcess(GetCurrentProcess(),
            ProcessAffinityMask, &process_mask, sizeof(process_mask), NULL))
        thread_count = popcount(process_mask);
    else
        thread_count = 0;

    GST_DEBUG_CATEGORY_INIT(wine, "WINE", GST_DEBUG_FG_RED, "Wine GStreamer support");

    GST_INFO("GStreamer library version %s; wine built with %d.%d.%d.",
            gst_version_string(), GST_VERSION_MAJOR, GST_VERSION_MINOR, GST_VERSION_MICRO);

    if (!GST_ELEMENT_REGISTER(winegstreamerstepper, NULL))
        GST_ERROR("Failed to register the stepper element");

    return STATUS_SUCCESS;
}

static bool element_has_property(const GstElement *element, const gchar *property)
{
    return !!g_object_class_find_property(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(element)), property);
}

void set_max_threads(GstElement *element)
{
    const char *shortname = NULL;
    GstElementFactory *factory = gst_element_get_factory(element);

    if (factory)
        shortname = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));

    /* By default, GStreamer will use the result of sysconf(_SC_NPROCESSORS_CONF) to determine the number
     * of decoder threads to be used by libva. This has two issues:
     * 1. It can return an inaccurate result (for example, on the Steam Deck this returns 16); and
     * 2. It disregards process affinity
     *
     * Both of these scenarios result in more threads being allocated than logical cores made available, meaning
     * they provide little (or possibly detrimental) performance benefit and for 4K video can occupy 32MB
     * of RAM each (w * h * bpp).
     *
     * So we will instead explictly set 'max-threads' to the minimum of thread_count (process affinity at time of
     * initialization) or 16 (4 for 32-bit processors).
     */

    if (shortname && strstr(shortname, "avdec_") && element_has_property(element, "max-threads"))
    {
        gint32 max_threads = MIN(thread_count, sizeof(void *) == 4 ? 4 : 16);
        GST_DEBUG("%s found, setting max-threads to %d.", shortname, max_threads);
        g_object_set(element, "max-threads", max_threads, NULL);
    }
}
