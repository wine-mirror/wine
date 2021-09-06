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

#ifndef __WINEBUS_UNIX_PRIVATE_H
#define __WINEBUS_UNIX_PRIVATE_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winternl.h>

#include "unixlib.h"

#include "wine/list.h"

struct unix_device_vtbl
{
    void (*destroy)(struct unix_device *iface);
    int (*compare)(struct unix_device *iface, void *platform_dev);
    NTSTATUS (*start)(struct unix_device *iface, DEVICE_OBJECT *device);
    NTSTATUS (*get_report_descriptor)(struct unix_device *iface, BYTE *buffer, DWORD length, DWORD *out_length);
    NTSTATUS (*get_string)(struct unix_device *iface, DWORD index, WCHAR *buffer, DWORD length);
    void (*set_output_report)(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
    void (*get_feature_report)(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
    void (*set_feature_report)(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
};

struct unix_device
{
    const struct unix_device_vtbl *vtbl;
    struct list entry;
};

extern NTSTATUS sdl_bus_init(void *) DECLSPEC_HIDDEN;
extern NTSTATUS sdl_bus_wait(void *) DECLSPEC_HIDDEN;
extern NTSTATUS sdl_bus_stop(void *) DECLSPEC_HIDDEN;

extern NTSTATUS udev_bus_init(void *) DECLSPEC_HIDDEN;
extern NTSTATUS udev_bus_wait(void *) DECLSPEC_HIDDEN;
extern NTSTATUS udev_bus_stop(void *) DECLSPEC_HIDDEN;

extern NTSTATUS iohid_bus_init(void *) DECLSPEC_HIDDEN;
extern NTSTATUS iohid_bus_wait(void *) DECLSPEC_HIDDEN;
extern NTSTATUS iohid_bus_stop(void *) DECLSPEC_HIDDEN;

extern void bus_event_queue_destroy(struct list *queue) DECLSPEC_HIDDEN;
extern BOOL bus_event_queue_device_removed(struct list *queue, const WCHAR *bus_id, void *context) DECLSPEC_HIDDEN;
extern BOOL bus_event_queue_device_created(struct list *queue, struct unix_device *device, struct device_desc *desc) DECLSPEC_HIDDEN;
extern BOOL bus_event_queue_pop(struct list *queue, struct bus_event *event) DECLSPEC_HIDDEN;

struct hid_descriptor
{
    BYTE *data;
    SIZE_T size;
    SIZE_T max_size;
};

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

#endif /* __WINEBUS_UNIX_PRIVATE_H */
