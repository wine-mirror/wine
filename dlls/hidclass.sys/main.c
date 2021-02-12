/*
 * WINE Hid device services
 *
 * Copyright 2015 Aric Stewart
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

#define NONAMELESSUNION
#include <unistd.h>
#include <stdarg.h>
#include "hid.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static struct list minidriver_list = LIST_INIT(minidriver_list);

minidriver* find_minidriver(DRIVER_OBJECT *driver)
{
    minidriver *md;
    LIST_FOR_EACH_ENTRY(md, &minidriver_list, minidriver, entry)
    {
        if (md->minidriver.DriverObject == driver)
            return md;
    }
    return NULL;
}

static VOID WINAPI UnloadDriver(DRIVER_OBJECT *driver)
{
    minidriver *md;

    TRACE("Driver Unload\n");
    md = find_minidriver(driver);
    if (md)
    {
        if (md->DriverUnload)
            md->DriverUnload(md->minidriver.DriverObject);
        list_remove(&md->entry);
        HeapFree( GetProcessHeap(), 0, md );
    }
}

NTSTATUS WINAPI HidRegisterMinidriver(HID_MINIDRIVER_REGISTRATION *registration)
{
    minidriver *driver;
    driver = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*driver));

    if (!driver)
        return STATUS_NO_MEMORY;

    driver->DriverUnload = registration->DriverObject->DriverUnload;
    registration->DriverObject->DriverUnload = UnloadDriver;

    registration->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HID_Device_ioctl;
    registration->DriverObject->MajorFunction[IRP_MJ_READ] = HID_Device_read;
    registration->DriverObject->MajorFunction[IRP_MJ_WRITE] = HID_Device_write;
    registration->DriverObject->MajorFunction[IRP_MJ_CREATE] = HID_Device_create;
    registration->DriverObject->MajorFunction[IRP_MJ_CLOSE] = HID_Device_close;

    driver->PNPDispatch = registration->DriverObject->MajorFunction[IRP_MJ_PNP];
    registration->DriverObject->MajorFunction[IRP_MJ_PNP] = HID_PNP_Dispatch;

    driver->AddDevice = registration->DriverObject->DriverExtension->AddDevice;
    registration->DriverObject->DriverExtension->AddDevice = PNP_AddDevice;

    driver->minidriver = *registration;
    list_add_tail(&minidriver_list, &driver->entry);

    return STATUS_SUCCESS;
}

NTSTATUS call_minidriver(ULONG code, DEVICE_OBJECT *device, void *in_buff, ULONG in_size, void *out_buff, ULONG out_size)
{
    IRP *irp;
    IO_STATUS_BLOCK io;
    KEVENT event;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(code, device, in_buff, in_size,
        out_buff, out_size, TRUE, &event, &io);

    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    return io.u.Status;
}
