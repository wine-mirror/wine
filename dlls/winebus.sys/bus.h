/*
 * Copyright 2016 Aric Stewart
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

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <ddk/wdm.h>
#include <ddk/hidclass.h>
#include <hidusage.h>

#include "unixlib.h"

struct unix_device *get_unix_device(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;

/* HID Plug and Play Bus */
DEVICE_OBJECT *bus_create_hid_device(struct device_desc *desc, struct unix_device *unix_device) DECLSPEC_HIDDEN;
DEVICE_OBJECT *bus_find_hid_device(const WCHAR *bus_id, void *platform_dev) DECLSPEC_HIDDEN;
void process_hid_report(DEVICE_OBJECT *device, BYTE *report, DWORD length) DECLSPEC_HIDDEN;

/* General Bus Functions */
BOOL is_xbox_gamepad(WORD vid, WORD pid) DECLSPEC_HIDDEN;

extern HANDLE driver_key DECLSPEC_HIDDEN;
extern DEVICE_OBJECT *bus_pdo DECLSPEC_HIDDEN;
