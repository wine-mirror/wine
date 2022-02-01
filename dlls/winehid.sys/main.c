/*
 * WINE Hid minidriver
 *
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "ddk/hidport.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static NTSTATUS WINAPI internal_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS status = irp->IoStatus.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    ULONG code = irpsp->Parameters.DeviceIoControl.IoControlCode;

    switch (code)
    {
    case IOCTL_GET_PHYSICAL_DESCRIPTOR:
    case IOCTL_HID_ACTIVATE_DEVICE:
    case IOCTL_HID_DEACTIVATE_DEVICE:
    case IOCTL_HID_GET_INDEXED_STRING:
    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
    case IOCTL_HID_GET_STRING:
    case IOCTL_HID_GET_INPUT_REPORT:
    case IOCTL_HID_READ_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
    case IOCTL_HID_WRITE_REPORT:
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_SET_FEATURE:
        /* All these are handled by the lower level driver */
        IoSkipCurrentIrpStackLocation(irp);
        return IoCallDriver(((HID_DEVICE_EXTENSION *)device->DeviceExtension)->NextDeviceObject, irp);

    default:
        FIXME("Unsupported ioctl %#lx (device=%lx access=%lx func=%lx method=%lx)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
    }
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(ext->NextDeviceObject, irp);
}

static NTSTATUS WINAPI add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *device)
{
    TRACE("(%p, %p)\n", driver, device);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    HID_MINIDRIVER_REGISTRATION registration;

    TRACE("(%p, %s)\n", driver, debugstr_w(path->Buffer));

    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = internal_ioctl;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->DriverExtension->AddDevice = add_device;

    memset(&registration, 0, sizeof(registration));
    registration.DriverObject = driver;
    registration.RegistryPath = path;
    registration.DeviceExtensionSize = sizeof(HID_DEVICE_EXTENSION);
    registration.DevicesArePolled = FALSE;

    return HidRegisterMinidriver(&registration);
}
