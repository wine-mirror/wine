/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2020 Zebediah Figura
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

#include "driver.h"

static const GUID control_class = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc0}};
static UNICODE_STRING control_symlink;

static DEVICE_OBJECT *bus_fdo, *bus_pdo;

static NTSTATUS fdo_pnp(IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret;

    switch (stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            irp->IoStatus.Status = IoSetDeviceInterfaceState(&control_symlink, TRUE);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            IoSetDeviceInterfaceState(&control_symlink, FALSE);
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            ret = IoCallDriver(bus_pdo, irp);
            IoDetachDevice(bus_pdo);
            IoDeleteDevice(bus_fdo);
            RtlFreeUnicodeString(&control_symlink);
            return ret;
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(bus_pdo, irp);
}

static NTSTATUS pdo_pnp(DEVICE_OBJECT *device_obj, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret = irp->IoStatus.Status;

    switch (stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
        case IRP_MN_SURPRISE_REMOVAL:
            ret = STATUS_SUCCESS;
            break;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    if (device == bus_fdo)
        return fdo_pnp(irp);
    return pdo_pnp(device, irp);
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *pdo)
{
    NTSTATUS ret;

    if ((ret = IoCreateDevice(driver, 0, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &bus_fdo)))
        return ret;

    if ((ret = IoRegisterDeviceInterface(pdo, &control_class, NULL, &control_symlink)))
    {
        IoDeleteDevice(bus_fdo);
        return ret;
    }

    IoAttachDeviceToDeviceStack(bus_fdo, pdo);
    bus_pdo = pdo;
    bus_fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *registry)
{
    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    return STATUS_SUCCESS;
}
