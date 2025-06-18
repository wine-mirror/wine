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
#include <ddk/hidpi.h>
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
    BOOL is_hidraw;
    BOOL is_bluetooth;

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

enum bus_event_type
{
    BUS_EVENT_TYPE_NONE,
    BUS_EVENT_TYPE_DEVICE_REMOVED,
    BUS_EVENT_TYPE_DEVICE_CREATED,
    BUS_EVENT_TYPE_INPUT_REPORT,
};

struct bus_event
{
    UINT type;
    UINT64 device;
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
    UINT64 device;
};

struct device_remove_params
{
    UINT64 device;
};

struct device_start_params
{
    UINT64 device;
};

struct device_descriptor_params
{
    UINT64 device;
    BYTE *buffer;
    UINT length;
    UINT *out_length;
};

struct device_report_params
{
    UINT64 device;
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
    unix_funcs_count,
};

static inline const char *debugstr_device_desc(struct device_desc *desc)
{
    if (!desc) return "(null)";
    return wine_dbg_sprintf("{vid %04x, pid %04x, version %04x, input %d, uid %08x, is_gamepad %u, is_hidraw %u, is_bluetooth %u}",
                            desc->vid, desc->pid, desc->version, desc->input, desc->uid,
                            desc->is_gamepad, desc->is_hidraw, desc->is_bluetooth);
}

static inline BOOL is_xbox_gamepad(WORD vid, WORD pid)
{
    if (vid != 0x045e) return FALSE;
    if (pid == 0x0202) return TRUE; /* Xbox Controller */
    if (pid == 0x0285) return TRUE; /* Xbox Controller S */
    if (pid == 0x0289) return TRUE; /* Xbox Controller S */
    if (pid == 0x028e) return TRUE; /* Xbox360 Controller */
    if (pid == 0x028f) return TRUE; /* Xbox360 Wireless Controller */
    if (pid == 0x02d1) return TRUE; /* Xbox One Controller */
    if (pid == 0x02dd) return TRUE; /* Xbox One Controller (Covert Forces/Firmware 2015) */
    if (pid == 0x02e0) return TRUE; /* Xbox One X Controller */
    if (pid == 0x02e3) return TRUE; /* Xbox One Elite Controller */
    if (pid == 0x02e6) return TRUE; /* Wireless XBox Controller Dongle */
    if (pid == 0x02ea) return TRUE; /* Xbox One S Controller */
    if (pid == 0x02fd) return TRUE; /* Xbox One S Controller (Firmware 2017) */
    if (pid == 0x0b00) return TRUE; /* Xbox Elite 2 */
    if (pid == 0x0b05) return TRUE; /* Xbox Elite 2 Wireless */
    if (pid == 0x0b12) return TRUE; /* Xbox Series */
    if (pid == 0x0b13) return TRUE; /* Xbox Series Wireless */
    if (pid == 0x0719) return TRUE; /* Xbox 360 Wireless Adapter */
    return FALSE;
}

static inline BOOL is_dualshock4_gamepad(WORD vid, WORD pid)
{
    if (vid != 0x054c) return FALSE;
    if (pid == 0x05c4) return TRUE; /* DualShock 4 [CUH-ZCT1x] */
    if (pid == 0x09cc) return TRUE; /* DualShock 4 [CUH-ZCT2x] */
    if (pid == 0x0ba0) return TRUE; /* Dualshock 4 Wireless Adaptor */
    return FALSE;
}

static inline BOOL is_dualsense_gamepad(WORD vid, WORD pid)
{
    if (vid != 0x054c) return FALSE;
    if (pid == 0x0ce6) return TRUE; /* DualSense */
    if (pid == 0x0df2) return TRUE; /* DualSense Edge */
    return FALSE;
}

#endif /* __WINEBUS_UNIXLIB_H */
