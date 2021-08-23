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

typedef int(*enum_func)(DEVICE_OBJECT *device, void *context);

/* Native device function table */
typedef struct
{
    void (*free_device)(DEVICE_OBJECT *device);
    int (*compare_platform_device)(DEVICE_OBJECT *device, void *platform_dev);
    NTSTATUS (*start_device)(DEVICE_OBJECT *device);
    NTSTATUS (*get_reportdescriptor)(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length);
    NTSTATUS (*get_string)(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length);
    void (*set_output_report)(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
    void (*get_feature_report)(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
    void (*set_feature_report)(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
} platform_vtbl;

struct unix_device *get_unix_device(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;

/* HID Plug and Play Bus */
DEVICE_OBJECT *bus_create_hid_device(const WCHAR *busidW, WORD vid, WORD pid, WORD input,
                                     DWORD version, DWORD uid, const WCHAR *serialW, BOOL is_gamepad,
                                     const platform_vtbl *vtbl, struct unix_device *unix_device) DECLSPEC_HIDDEN;
DEVICE_OBJECT *bus_find_hid_device(const WCHAR *bus_id, void *platform_dev) DECLSPEC_HIDDEN;
void bus_unlink_hid_device(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;
void process_hid_report(DEVICE_OBJECT *device, BYTE *report, DWORD length) DECLSPEC_HIDDEN;
DEVICE_OBJECT *bus_enumerate_hid_devices(const WCHAR *bus_id, enum_func function, void *context) DECLSPEC_HIDDEN;

/* General Bus Functions */
BOOL is_xbox_gamepad(WORD vid, WORD pid) DECLSPEC_HIDDEN;

extern HANDLE driver_key DECLSPEC_HIDDEN;
extern DEVICE_OBJECT *bus_pdo DECLSPEC_HIDDEN;

struct hid_descriptor
{
    BYTE  *data;
    SIZE_T size;
    SIZE_T max_size;
};

extern BOOL hid_descriptor_append(struct hid_descriptor *desc, const BYTE *buffer, SIZE_T size) DECLSPEC_HIDDEN;
extern BOOL hid_descriptor_begin(struct hid_descriptor *desc, USAGE usage_page, USAGE usage) DECLSPEC_HIDDEN;
extern BOOL hid_descriptor_end(struct hid_descriptor *desc) DECLSPEC_HIDDEN;
extern void hid_descriptor_free(struct hid_descriptor *desc) DECLSPEC_HIDDEN;

extern BOOL hid_descriptor_add_buttons(struct hid_descriptor *desc, USAGE usage_page,
                                       USAGE usage_min, USAGE usage_max) DECLSPEC_HIDDEN;
extern BOOL hid_descriptor_add_padding(struct hid_descriptor *desc, BYTE bitcount) DECLSPEC_HIDDEN;
extern BOOL hid_descriptor_add_hatswitch(struct hid_descriptor *desc, INT count) DECLSPEC_HIDDEN;
extern BOOL hid_descriptor_add_axes(struct hid_descriptor *desc, BYTE count, USAGE usage_page,
                                    const USAGE *usages, BOOL rel, INT size, LONG min, LONG max) DECLSPEC_HIDDEN;

extern BOOL hid_descriptor_add_haptics(struct hid_descriptor *desc) DECLSPEC_HIDDEN;
