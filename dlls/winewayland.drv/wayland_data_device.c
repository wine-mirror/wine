/*
 * Wayland clipboard
 *
 * Copyright 2025 Alexandros Frantzis for Collabora Ltd
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

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

void wayland_data_device_init(void)
{
    struct wayland_data_device *data_device = &process_wayland.data_device;

    TRACE("\n");

    if (data_device->zwlr_data_control_device_v1)
        zwlr_data_control_device_v1_destroy(data_device->zwlr_data_control_device_v1);
    data_device->zwlr_data_control_device_v1 =
        zwlr_data_control_manager_v1_get_data_device(
            process_wayland.zwlr_data_control_manager_v1,
            process_wayland.seat.wl_seat);
}

LRESULT WAYLAND_ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        pthread_mutex_lock(&process_wayland.seat.mutex);
        if (process_wayland.seat.wl_seat && process_wayland.zwlr_data_control_manager_v1)
            wayland_data_device_init();
        pthread_mutex_unlock(&process_wayland.seat.mutex);
        return TRUE;
    }

    return NtUserMessageCall(hwnd, msg, wparam, lparam, NULL, NtUserDefWindowProc, FALSE);
}
