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

typedef int(*enum_func)(DEVICE_OBJECT *device, void *context);

/* Buses */
NTSTATUS udev_driver_init(void) DECLSPEC_HIDDEN;
NTSTATUS iohid_driver_init(void) DECLSPEC_HIDDEN;
NTSTATUS sdl_driver_init(void) DECLSPEC_HIDDEN;
void udev_driver_unload( void ) DECLSPEC_HIDDEN;
void iohid_driver_unload( void ) DECLSPEC_HIDDEN;
void sdl_driver_unload( void ) DECLSPEC_HIDDEN;

/* Native device function table */
typedef struct
{
    void (*free_device)(DEVICE_OBJECT *device);
    int (*compare_platform_device)(DEVICE_OBJECT *device, void *platform_dev);
    NTSTATUS (*get_reportdescriptor)(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length);
    NTSTATUS (*get_string)(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length);
    NTSTATUS (*begin_report_processing)(DEVICE_OBJECT *device);
    NTSTATUS (*set_output_report)(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written);
    NTSTATUS (*get_feature_report)(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *read);
    NTSTATUS (*set_feature_report)(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written);
} platform_vtbl;

void *get_platform_private(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;

/* HID Plug and Play Bus */
DEVICE_OBJECT *bus_create_hid_device(const WCHAR *busidW, WORD vid, WORD pid,
                                     WORD input, DWORD version, DWORD uid, const WCHAR *serialW, BOOL is_gamepad,
                                     const platform_vtbl *vtbl, DWORD platform_data_size) DECLSPEC_HIDDEN;
DEVICE_OBJECT *bus_find_hid_device(const platform_vtbl *vtbl, void *platform_dev) DECLSPEC_HIDDEN;
void bus_unlink_hid_device(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;
void bus_remove_hid_device(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;
void process_hid_report(DEVICE_OBJECT *device, BYTE *report, DWORD length) DECLSPEC_HIDDEN;
DEVICE_OBJECT* bus_enumerate_hid_devices(const platform_vtbl *vtbl, enum_func function, void* context) DECLSPEC_HIDDEN;

/* General Bus Functions */
DWORD check_bus_option(const UNICODE_STRING *option, DWORD default_value) DECLSPEC_HIDDEN;
BOOL is_xbox_gamepad(WORD vid, WORD pid) DECLSPEC_HIDDEN;

extern HANDLE driver_key DECLSPEC_HIDDEN;
extern DEVICE_OBJECT *bus_pdo DECLSPEC_HIDDEN;
