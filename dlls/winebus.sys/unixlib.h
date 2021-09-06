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
#include <ddk/wdm.h>
#include <ddk/hidclass.h>
#include <hidusage.h>

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

struct device_desc
{
    const WCHAR *busid;
    DWORD vid;
    DWORD pid;
    DWORD version;
    DWORD input;
    DWORD uid;
    WCHAR serial[256];
    BOOL is_gamepad;
};

struct sdl_bus_options
{
    BOOL map_controllers;
};

struct udev_bus_options
{
    BOOL disable_hidraw;
    BOOL disable_input;
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
};

struct bus_event
{
    enum bus_event_type type;
    struct list entry;

    union
    {
        struct
        {
            const WCHAR *bus_id;
            void *context;
        } device_removed;

        struct
        {
            struct unix_device *device;
            struct device_desc desc;
        } device_created;
    };
};

struct device_create_params
{
    struct device_desc desc;
    struct unix_device *device;
};

struct device_compare_params
{
    struct unix_device *iface;
    void *context;
};

struct device_start_params
{
    struct unix_device *iface;
    DEVICE_OBJECT *device;
};

struct device_descriptor_params
{
    struct unix_device *iface;
    BYTE *buffer;
    DWORD length;
    DWORD *out_length;
};

struct device_string_params
{
    struct unix_device *iface;
    DWORD index;
    WCHAR *buffer;
    DWORD length;
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
    device_compare,
    device_start,
    device_get_report_descriptor,
    device_get_string,
    device_set_output_report,
    device_get_feature_report,
    device_set_feature_report,
};

extern const unixlib_entry_t __wine_unix_call_funcs[] DECLSPEC_HIDDEN;

static inline const char *debugstr_device_desc(struct device_desc *desc)
{
    if (!desc) return "(null)";
    return wine_dbg_sprintf("{busid %s, vid %04x, pid %04x, version %04x, input %d, uid %08x, serial %s, is_gamepad %u}",
                            debugstr_w(desc->busid), desc->vid, desc->pid, desc->version,
                            desc->input, desc->uid, debugstr_w(desc->serial), desc->is_gamepad);
}

#endif /* __WINEBUS_UNIXLIB_H */
