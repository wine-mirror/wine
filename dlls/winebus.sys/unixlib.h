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
#include <hidusage.h>

#include "wine/unixlib.h"

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
};

extern const unixlib_entry_t __wine_unix_call_funcs[] DECLSPEC_HIDDEN;

#endif /* __WINEBUS_UNIXLIB_H */
