/*
 * HTTP server driver
 *
 * Copyright 2019 Zebediah Figura
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "wine/debug.h"

static HANDLE directory_obj;
static DEVICE_OBJECT *device_obj;

WINE_DEFAULT_DEBUG_CHANNEL(http);

static NTSTATUS WINAPI dispatch_create(DEVICE_OBJECT *device, IRP *irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI dispatch_close(DEVICE_OBJECT *device, IRP *irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static void WINAPI unload(DRIVER_OBJECT *driver)
{
    IoDeleteDevice(device_obj);
    NtClose(directory_obj);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    static const WCHAR device_nameW[] = {'\\','D','e','v','i','c','e','\\','H','t','t','p','\\','R','e','q','Q','u','e','u','e',0};
    static const WCHAR directory_nameW[] = {'\\','D','e','v','i','c','e','\\','H','t','t','p',0};
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    UNICODE_STRING string;
    NTSTATUS ret;

    TRACE("driver %p, path %s.\n", driver, debugstr_w(path->Buffer));

    RtlInitUnicodeString(&string, directory_nameW);
    attr.ObjectName = &string;
    if ((ret = NtCreateDirectoryObject(&directory_obj, 0, &attr)) && ret != STATUS_OBJECT_NAME_COLLISION)
        ERR("Failed to create \\Device\\Http directory, status %#x.\n", ret);

    RtlInitUnicodeString(&string, device_nameW);
    if ((ret = IoCreateDevice(driver, 0, &string, FILE_DEVICE_UNKNOWN, 0, FALSE, &device_obj)))
    {
        ERR("Failed to create request queue device, status %#x.\n", ret);
        NtClose(directory_obj);
        return ret;
    }

    driver->MajorFunction[IRP_MJ_CREATE] = dispatch_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = dispatch_close;
    driver->DriverUnload = unload;

    return STATUS_SUCCESS;
}
