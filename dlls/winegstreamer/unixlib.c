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

NTSTATUS wg_init_gstreamer(void *arg)
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
        return STATUS_UNSUCCESSFUL;
    }

    GST_DEBUG_CATEGORY_INIT(wine, "WINE", GST_DEBUG_FG_RED, "Wine GStreamer support");

    GST_INFO("GStreamer library version %s; wine built with %d.%d.%d.",
            gst_version_string(), GST_VERSION_MAJOR, GST_VERSION_MINOR, GST_VERSION_MICRO);
    return STATUS_SUCCESS;
}
