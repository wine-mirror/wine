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

/* Busses */
NTSTATUS WINAPI udev_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path) DECLSPEC_HIDDEN;

/* Native device function table */
typedef struct
{
    int (*compare_platform_device)(DEVICE_OBJECT *device, void *platform_dev);
} platform_vtbl;

void *get_platform_private(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;

/* HID Plug and Play Bus */
NTSTATUS WINAPI common_pnp_dispatch(DEVICE_OBJECT *device, IRP *irp) DECLSPEC_HIDDEN;
DEVICE_OBJECT *bus_create_hid_device(DRIVER_OBJECT *driver, const WCHAR *busidW, WORD vid, WORD pid,
                                     DWORD version, DWORD uid, const WCHAR *serialW, BOOL is_gamepad,
                                     const GUID *class, const platform_vtbl *vtbl, DWORD platform_data_size) DECLSPEC_HIDDEN;
DEVICE_OBJECT *bus_find_hid_device(const platform_vtbl *vtbl, void *platform_dev) DECLSPEC_HIDDEN;
void bus_remove_hid_device(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;
NTSTATUS WINAPI hid_internal_dispatch(DEVICE_OBJECT *device, IRP *irp) DECLSPEC_HIDDEN;
