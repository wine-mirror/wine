/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#ifndef __WINEBUS_UNIXLIB_H
#define __WINEBUS_UNIXLIB_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <ddk/hidclass.h>
#include <hidusage.h>

#include "wine/debug.h"
#include "wine/list.h"

struct device_desc
{
    UINT vid;
    UINT pid;
    UINT version;
    UINT input;
    UINT uid;
    BOOL is_gamepad;

    WCHAR manufacturer[MAX_PATH];
    WCHAR product[MAX_PATH];
    WCHAR serialnumber[MAX_PATH];
};

struct sdl_bus_options
{
    BOOL split_controllers;
    BOOL map_controllers;
    /* freed after bus_init */
    UINT mappings_count;
    char **mappings;
};

struct udev_bus_options
{
    BOOL disable_hidraw;
    BOOL disable_input;
    BOOL disable_udevd;
};

struct iohid_bus_options
{
};

struct unix_device;

enum bus_event_type
{
    BUS_EVENT_TYPE_NONE,
    BUS_EVENT_TYPE_DEVICE_REMOVED,
    BUS_EVENT_TYPE_DEVICE_CREATED,
    BUS_EVENT_TYPE_INPUT_REPORT,
};

struct bus_event
{
    enum bus_event_type type;
    struct list entry;

    struct unix_device *device;
    union
    {
        struct
        {
            struct device_desc desc;
        } device_created;

        struct
        {
            USHORT length;
            BYTE buffer[1];
        } input_report;
    };
};

struct device_create_params
{
    struct device_desc desc;
    struct unix_device *device;
};

struct device_descriptor_params
{
    struct unix_device *iface;
    BYTE *buffer;
    UINT length;
    UINT *out_length;
};

struct device_report_params
{
    struct unix_device *iface;
    HID_XFER_PACKET *packet;
    IO_STATUS_BLOCK *io;
};

enum unix_funcs
{
    sdl_init,
    sdl_wait,
    sdl_stop,
    udev_init,
    udev_wait,
    udev_stop,
    iohid_init,
    iohid_wait,
    iohid_stop,
    mouse_create,
    keyboard_create,
    device_remove,
    device_start,
    device_get_report_descriptor,
    device_set_output_report,
    device_get_feature_report,
    device_set_feature_report,
};

static inline const char *debugstr_device_desc(struct device_desc *desc)
{
    if (!desc) return "(null)";
    return wine_dbg_sprintf("{vid %04x, pid %04x, version %04x, input %d, uid %08x, is_gamepad %u}",
                            desc->vid, desc->pid, desc->version, desc->input, desc->uid, desc->is_gamepad);
}

#endif /* __WINEBUS_UNIXLIB_H */
