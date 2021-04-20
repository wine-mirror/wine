/*
 * HID Plug and Play test driver
 *
 * Copyright 2021 Zebediah Figura
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
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "hidusage.h"
#include "ddk/hidpi.h"
#include "ddk/hidport.h"

#include "wine/list.h"

#include "driver.h"
#include "utils.h"

static UNICODE_STRING control_symlink;

static unsigned int got_start_device;

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    if (winetest_debug > 1) trace("pnp %#x\n", stack->MinorFunction);

    switch (stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            ++got_start_device;
            IoSetDeviceInterfaceState(&control_symlink, TRUE);
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            IoSetDeviceInterfaceState(&control_symlink, FALSE);
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(ext->NextDeviceObject, irp);
}


static NTSTATUS WINAPI driver_power(DEVICE_OBJECT *device, IRP *irp)
{
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    /* We do not expect power IRPs as part of normal operation. */
    ok(0, "unexpected call\n");

    PoStartNextPowerIrp(irp);
    IoSkipCurrentIrpStackLocation(irp);
    return PoCallDriver(ext->NextDeviceObject, irp);
}

static const unsigned char report_descriptor[] =
{
    0x05, HID_USAGE_PAGE_GENERIC,
    0x09, HID_USAGE_GENERIC_JOYSTICK,
    0xa1, 0x01, /* application collection */
    0x05, HID_USAGE_PAGE_GENERIC,
    0x09, HID_USAGE_GENERIC_X,
    0x09, HID_USAGE_GENERIC_Y,
    0x15, 0x80, /* logical minimum -128 */
    0x25, 0x7f, /* logical maximum 127 */
    0x75, 0x08, /* report size */
    0x95, 0x02, /* report count */
    0x81, 0x02, /* input, variable */
    0xc0, /* end collection */
};

static NTSTATUS WINAPI driver_internal_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    const ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
    const ULONG out_size = stack->Parameters.DeviceIoControl.OutputBufferLength;
    const ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS ret;

    if (winetest_debug > 1) trace("ioctl %#x\n", code);

    ok(got_start_device, "expected IRP_MN_START_DEVICE before any ioctls\n");

    irp->IoStatus.Information = 0;

    switch (code)
    {
        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            HID_DESCRIPTOR *desc = irp->UserBuffer;

            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == sizeof(*desc), "got output size %u\n", out_size);

            if (out_size == sizeof(*desc))
            {
                ok(!desc->bLength, "got size %u\n", desc->bLength);

                desc->bLength = sizeof(*desc);
                desc->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
                desc->bcdHID = HID_REVISION;
                desc->bCountry = 0;
                desc->bNumDescriptors = 1;
                desc->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
                desc->DescriptorList[0].wReportLength = sizeof(report_descriptor);
                irp->IoStatus.Information = sizeof(*desc);
            }
            ret = STATUS_SUCCESS;
            break;
        }

        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == sizeof(report_descriptor), "got output size %u\n", out_size);

            if (out_size == sizeof(report_descriptor))
            {
                memcpy(irp->UserBuffer, report_descriptor, sizeof(report_descriptor));
                irp->IoStatus.Information = sizeof(report_descriptor);
            }
            ret = STATUS_SUCCESS;
            break;

        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            HID_DEVICE_ATTRIBUTES *attr = irp->UserBuffer;

            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == sizeof(*attr), "got output size %u\n", out_size);

            if (out_size == sizeof(*attr))
            {
                ok(!attr->Size, "got size %u\n", attr->Size);

                attr->Size = sizeof(*attr);
                attr->VendorID = 0x1209;
                attr->ProductID = 0x0001;
                attr->VersionNumber = 0xface;
                irp->IoStatus.Information = sizeof(*attr);
            }
            ret = STATUS_SUCCESS;
            break;
        }

        case IOCTL_HID_READ_REPORT:
            ok(!in_size, "got input size %u\n", in_size);
            todo_wine ok(out_size == 2, "got output size %u\n", out_size);

            ret = STATUS_NOT_IMPLEMENTED;
            break;

        case IOCTL_HID_GET_STRING:
            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == 128, "got output size %u\n", out_size);

            ret = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            ok(0, "unexpected ioctl %#x\n", code);
            ret = STATUS_NOT_IMPLEMENTED;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static NTSTATUS WINAPI driver_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    ok(0, "unexpected call\n");
    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(ext->NextDeviceObject, irp);
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *fdo)
{
    HID_DEVICE_EXTENSION *ext = fdo->DeviceExtension;
    NTSTATUS ret;

    /* We should be given the FDO, not the PDO. */
    ok(!!ext->PhysicalDeviceObject, "expected non-NULL pdo\n");
    ok(ext->NextDeviceObject == ext->PhysicalDeviceObject, "got pdo %p, next %p\n",
            ext->PhysicalDeviceObject, ext->NextDeviceObject);
    todo_wine ok(ext->NextDeviceObject->AttachedDevice == fdo, "wrong attached device\n");

    ret = IoRegisterDeviceInterface(ext->PhysicalDeviceObject, &control_class, NULL, &control_symlink);
    ok(!ret, "got %#x\n", ret);

    fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    ok(0, "unexpected call\n");
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    ok(0, "unexpected call\n");
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    winetest_cleanup();
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *registry)
{
    HID_MINIDRIVER_REGISTRATION params =
    {
        .Revision = HID_REVISION,
        .DriverObject = driver,
        .RegistryPath = registry,
    };
    NTSTATUS ret;

    if ((ret = winetest_init()))
        return ret;

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_POWER] = driver_power;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = driver_internal_ioctl;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    ret = HidRegisterMinidriver(&params);
    ok(!ret, "got %#x\n", ret);

    return STATUS_SUCCESS;
}
