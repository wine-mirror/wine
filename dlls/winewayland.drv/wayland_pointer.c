/*
 * Wayland pointer handling
 *
 * Copyright (c) 2020 Alexandros Frantzis for Collabora Ltd
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

#include "waylanddrv.h"

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
}

static void pointer_handle_enter(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *wl_surface,
                                 wl_fixed_t sx, wl_fixed_t sy)
{
}

static void pointer_handle_leave(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *wl_surface)
{
}

static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t serial, uint32_t time, uint32_t button,
                                  uint32_t state)
{
}

static void pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
                                uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static void pointer_handle_frame(void *data, struct wl_pointer *wl_pointer)
{
}

static void pointer_handle_axis_source(void *data, struct wl_pointer *wl_pointer,
                                       uint32_t axis_source)
{
}

static void pointer_handle_axis_stop(void *data, struct wl_pointer *wl_pointer,
                                     uint32_t time, uint32_t axis)
{
}

static void pointer_handle_axis_discrete(void *data, struct wl_pointer *wl_pointer,
                                         uint32_t axis, int32_t discrete)
{
}

static const struct wl_pointer_listener pointer_listener =
{
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
    pointer_handle_frame,
    pointer_handle_axis_source,
    pointer_handle_axis_stop,
    pointer_handle_axis_discrete
};

void wayland_pointer_init(struct wl_pointer *wl_pointer)
{
    process_wayland.wl_pointer = wl_pointer;
    wl_pointer_add_listener(process_wayland.wl_pointer, &pointer_listener, NULL);
}

void wayland_pointer_deinit(void)
{
    wl_pointer_release(process_wayland.wl_pointer);
    process_wayland.wl_pointer = NULL;
}
