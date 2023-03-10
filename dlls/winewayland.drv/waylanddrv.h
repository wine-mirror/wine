/*
 * Wayland driver
 *
 * Copyright 2020 Alexandros Frantzis for Collabora Ltd
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

#ifndef __WINE_WAYLANDDRV_H
#define __WINE_WAYLANDDRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <wayland-client.h>

#include "windef.h"
#include "winbase.h"
#include "wine/gdi_driver.h"
#include "wine/rbtree.h"

#include "unixlib.h"

/**********************************************************************
 *          Globals
 */

extern struct wl_display *process_wl_display DECLSPEC_HIDDEN;
extern struct wayland *process_wayland DECLSPEC_HIDDEN;

/**********************************************************************
 *          Definitions for wayland types
 */

struct wayland
{
    struct wl_event_queue *wl_event_queue;
    struct wl_registry *wl_registry;
    struct wl_list output_list;
};

struct wayland_output_mode
{
    struct rb_entry entry;
    int32_t width;
    int32_t height;
    int32_t refresh;
};

struct wayland_output
{
    struct wl_list link;
    struct wl_output *wl_output;
    struct rb_tree modes;
    struct wayland_output_mode *current_mode;
    char name[20];
    uint32_t global_id;
};

/**********************************************************************
 *          Wayland initialization
 */

BOOL wayland_process_init(void) DECLSPEC_HIDDEN;
void wayland_init_display_devices(void) DECLSPEC_HIDDEN;

/**********************************************************************
 *          Wayland output
 */

BOOL wayland_output_create(uint32_t id, uint32_t version) DECLSPEC_HIDDEN;
void wayland_output_destroy(struct wayland_output *output) DECLSPEC_HIDDEN;

/**********************************************************************
 *          USER driver functions
 */

BOOL WAYLAND_UpdateDisplayDevices(const struct gdi_device_manager *device_manager,
                                  BOOL force, void *param) DECLSPEC_HIDDEN;

#endif /* __WINE_WAYLANDDRV_H */
