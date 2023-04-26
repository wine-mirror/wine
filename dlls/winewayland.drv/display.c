/*
 * WAYLAND display device functions
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "waylanddrv.h"

#include "wine/debug.h"

#include "ntuser.h"

#include <stdlib.h>

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

static BOOL force_display_devices_refresh;

void wayland_init_display_devices(BOOL force)
{
    UINT32 num_path, num_mode;

    TRACE("force=%d\n", force);

    if (force) force_display_devices_refresh = TRUE;
    /* Trigger refresh in win32u */
    NtUserGetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &num_path, &num_mode);
}

struct output_info
{
    int x, y;
    struct wayland_output_state *output;
};

static int output_info_cmp_primary_x_y(const void *va, const void *vb)
{
    const struct output_info *a = va;
    const struct output_info *b = vb;
    BOOL a_is_primary = a->x == 0 && a->y == 0;
    BOOL b_is_primary = b->x == 0 && b->y == 0;

    if (a_is_primary && !b_is_primary) return -1;
    if (!a_is_primary && b_is_primary) return 1;
    if (a->x < b->x) return -1;
    if (a->x > b->x) return 1;
    if (a->y < b->y) return -1;
    if (a->y > b->y) return 1;
    return strcmp(a->output->name, b->output->name);
}

static inline BOOL output_info_overlap(struct output_info *a, struct output_info *b)
{
    return b->x < a->x + a->output->current_mode->width &&
           b->x + b->output->current_mode->width > a->x &&
           b->y < a->y + a->output->current_mode->height &&
           b->y + b->output->current_mode->height > a->y;
}

/* Map a point to one of the four quadrants of our 2d coordinate space:
 * 0: bottom right (x >= 0, y >= 0)
 * 1: top right (x >= 0, y < 0)
 * 2: bottom left (x < 0, y >= 0)
 * 3: top left (x < 0, y < 0) */
static inline int point_to_quadrant(int x, int y)
{
    return (x < 0) * 2 + (y < 0);
}

/* Decide which of two outputs to keep stationary in order
 * to resolve an overlap. */
static struct output_info *output_info_get_overlap_anchor(struct output_info *a,
                                                          struct output_info *b)
{
    /* Preferences for the direction of growth in each quadrant, with a
     * lower value signifying a higher preference. */
    static const int quadrant_prefs[4][4] =
    {
        {0, 1, 2, 3}, /* quadrant 0 */
        {3, 0, 2, 1}, /* quadrant 1 */
        {2, 3, 0, 1}, /* quadrant 2 */
        {3, 2, 1, 0}, /* quadrant 3 */
    };
    int qa = point_to_quadrant(a->output->logical_x, a->output->logical_y);
    int qb = point_to_quadrant(b->output->logical_x, b->output->logical_y);
    /* Direction of growth if a is the anchor. */
    int qab = point_to_quadrant(b->output->logical_x - a->output->logical_x,
                                b->output->logical_y - a->output->logical_y);
    /* Direction of growth if b is the anchor. */
    int qba = point_to_quadrant(a->output->logical_x - b->output->logical_x,
                                a->output->logical_y - b->output->logical_y);

    /* If the two output origins are in different quadrants, use the output
     * in the lower valued quadrant as the anchor (so effectively outputs
     * grow/move away from quadrant 0). */
    if (qa != qb) return (qa < qb) ? a : b;

    /* If the outputs are in the same quadrant, use the preference for the
     * direction of growth in that quadrant to select the anchor. Again the
     * intended effect is to grow/move outputs away from the origin. */
    return (quadrant_prefs[qa][qab] < quadrant_prefs[qa][qba]) ? a : b;
}

static BOOL output_info_array_resolve_overlaps(struct wl_array *output_info_array)
{
    struct output_info *a, *b;
    BOOL found_overlap = FALSE;

    wl_array_for_each(a, output_info_array)
    {
        wl_array_for_each(b, output_info_array)
        {
            struct output_info *anchor, *move;
            BOOL x_use_end, y_use_end;
            double rel_x, rel_y;

            /* Break if we reach the same output in the inner loop, so that we
             * don't process output pairs twice (since order doesn't matter for
             * our algorithm.) */
            if (a == b) break;

            if (!output_info_overlap(a, b)) continue;
            found_overlap = TRUE;

            /* Decide which output to move to resolve the overlap. */
            anchor = output_info_get_overlap_anchor(a, b);
            move = anchor == a ? b : a;

            /* Move the selected output on the X axis to resolve the overlap,
             * while maintaining the same relative positioning of the outputs as
             * the one they have in logical space. Use either the start or end
             * of the moved output as the point to maintain the relative
             * position of, depending on whether the anchor is before or after
             * the moved output on the axis. */
            x_use_end = move->output->logical_x < anchor->output->logical_x;
            rel_x = (move->output->logical_x - anchor->output->logical_x +
                     (x_use_end ? move->output->logical_w : 0)) /
                    (double)anchor->output->logical_w;
            move->x = anchor->x + anchor->output->current_mode->width * rel_x -
                      (x_use_end ? move->output->current_mode->width : 0);

            /* Similarly for the Y axis. */
            y_use_end = move->output->logical_y < anchor->output->logical_y;
            rel_y = (move->output->logical_y - anchor->output->logical_y +
                     (y_use_end ? move->output->logical_h : 0)) /
                    (double)anchor->output->logical_h;
            move->y = anchor->y + anchor->output->current_mode->height * rel_y -
                      (y_use_end ? move->output->current_mode->height : 0);
        }
    }

    return found_overlap;
}

static void output_info_array_arrange_physical_coords(struct wl_array *output_info_array)
{
    struct output_info *info;
    size_t num_outputs = output_info_array->size / sizeof(struct output_info);
    int steps = 0;

    /* Set the initial physical pixel coordinates. */
    wl_array_for_each(info, output_info_array)
    {
        info->x = info->output->logical_x;
        info->y = info->output->logical_y;
    }

    /* Try to iteratively resolve overlaps, but be defensive and set an upper
     * iteration bound to ensure we avoid infinite loops. */
    while (output_info_array_resolve_overlaps(output_info_array) &&
           ++steps < num_outputs)
        continue;

    /* Now that we have our physical pixel coordinates, sort from physical left
     * to right, but ensure the primary output is first. */
    qsort(output_info_array->data, num_outputs, sizeof(struct output_info),
          output_info_cmp_primary_x_y);
}

static void wayland_add_device_gpu(const struct gdi_device_manager *device_manager,
                                   void *param)
{
    static const WCHAR wayland_gpuW[] = {'W','a','y','l','a','n','d','G','P','U',0};
    struct gdi_gpu gpu = {0};
    lstrcpyW(gpu.name, wayland_gpuW);

    TRACE("id=0x%s name=%s\n",
          wine_dbgstr_longlong(gpu.id), wine_dbgstr_w(gpu.name));

    device_manager->add_gpu(&gpu, param);
}

static void wayland_add_device_adapter(const struct gdi_device_manager *device_manager,
                                       void *param, INT output_id)
{
    struct gdi_adapter adapter;
    adapter.id = output_id;
    adapter.state_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
    if (output_id == 0)
        adapter.state_flags |= DISPLAY_DEVICE_PRIMARY_DEVICE;

    TRACE("id=0x%s state_flags=0x%x\n",
          wine_dbgstr_longlong(adapter.id), (UINT)adapter.state_flags);

    device_manager->add_adapter(&adapter, param);
}

static void wayland_add_device_monitor(const struct gdi_device_manager *device_manager,
                                       void *param, struct output_info *output_info)
{
    struct gdi_monitor monitor = {0};

    SetRect(&monitor.rc_monitor, output_info->x, output_info->y,
            output_info->x + output_info->output->current_mode->width,
            output_info->y + output_info->output->current_mode->height);

    /* We don't have a direct way to get the work area in Wayland. */
    monitor.rc_work = monitor.rc_monitor;

    monitor.state_flags = DISPLAY_DEVICE_ATTACHED | DISPLAY_DEVICE_ACTIVE;

    TRACE("name=%s rc_monitor=rc_work=%s state_flags=0x%x\n",
          output_info->output->name, wine_dbgstr_rect(&monitor.rc_monitor),
          (UINT)monitor.state_flags);

    device_manager->add_monitor(&monitor, param);
}

static void populate_devmode(struct wayland_output_mode *output_mode, DEVMODEW *mode)
{
    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    mode->dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmDisplayFlags = 0;
    mode->dmBitsPerPel = 32;
    mode->dmPelsWidth = output_mode->width;
    mode->dmPelsHeight = output_mode->height;
    mode->dmDisplayFrequency = output_mode->refresh / 1000;
}

static void wayland_add_device_modes(const struct gdi_device_manager *device_manager,
                                     void *param, struct output_info *output_info)
{
    struct wayland_output_mode *output_mode;

    RB_FOR_EACH_ENTRY(output_mode, &output_info->output->modes,
                      struct wayland_output_mode, entry)
    {
        DEVMODEW mode = {.dmSize = sizeof(mode)};
        BOOL mode_is_current = output_mode == output_info->output->current_mode;
        populate_devmode(output_mode, &mode);
        if (mode_is_current)
        {
            mode.dmFields |= DM_POSITION;
            mode.dmPosition.x = output_info->x;
            mode.dmPosition.y = output_info->y;
        }
        device_manager->add_mode(&mode, mode_is_current, param);
    }
}

/***********************************************************************
 *      UpdateDisplayDevices (WAYLAND.@)
 */
BOOL WAYLAND_UpdateDisplayDevices(const struct gdi_device_manager *device_manager,
                                  BOOL force, void *param)
{
    struct wayland_output *output;
    INT output_id = 0;
    struct wl_array output_info_array;
    struct output_info *output_info;

    if (!force && !force_display_devices_refresh) return TRUE;

    TRACE("force=%d force_refresh=%d\n", force, force_display_devices_refresh);

    force_display_devices_refresh = FALSE;

    wl_array_init(&output_info_array);

    pthread_mutex_lock(&process_wayland.output_mutex);

    wl_list_for_each(output, &process_wayland.output_list, link)
    {
        if (!output->current.current_mode) continue;
        output_info = wl_array_add(&output_info_array, sizeof(*output_info));
        if (output_info) output_info->output = &output->current;
        else ERR("Failed to allocate space for output_info\n");
    }

    output_info_array_arrange_physical_coords(&output_info_array);

    /* Populate GDI devices. */
    wayland_add_device_gpu(device_manager, param);

    wl_array_for_each(output_info, &output_info_array)
    {
        wayland_add_device_adapter(device_manager, param, output_id);
        wayland_add_device_monitor(device_manager, param, output_info);
        wayland_add_device_modes(device_manager, param, output_info);
        output_id++;
    }

    wl_array_release(&output_info_array);

    pthread_mutex_unlock(&process_wayland.output_mutex);

    return TRUE;
}
