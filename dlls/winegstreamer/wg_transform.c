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
    int dummy;
};

NTSTATUS wg_transform_destroy(void *args)
{
    struct wg_transform *transform = args;

    free(transform);
    return STATUS_SUCCESS;
}

NTSTATUS wg_transform_create(void *args)
{
    struct wg_transform_create_params *params = args;
    struct wg_transform *transform;
    NTSTATUS status;

    if (!init_gstreamer())
        return STATUS_UNSUCCESSFUL;

    status = STATUS_NO_MEMORY;

    if (!(transform = calloc(1, sizeof(*transform))))
        goto done;

    status = STATUS_SUCCESS;

done:
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
