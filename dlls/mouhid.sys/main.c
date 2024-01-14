/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ntuser.h"

#include "ddk/wdm.h"
#include "ddk/hidport.h"
#include "ddk/hidpddi.h"
#include "ddk/hidtypes.h"

#include "wine/hid.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

struct device
{
    LONG removed;
    DEVICE_OBJECT *bus_device;
};

static inline struct device *impl_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    return (struct device *)device->DeviceExtension;
}

static NTSTATUS WINAPI driver_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    struct device *impl = impl_from_DEVICE_OBJECT( device );

    if (InterlockedOr( &impl->removed, FALSE ))
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        irp->IoStatus.Information = 0;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }

    TRACE( "device %p, irp %p, code %#lx, bus_device %p.\n", device, irp, code, impl->bus_device );

    IoSkipCurrentIrpStackLocation( irp );
    return IoCallDriver( impl->bus_device, irp );
}

static NTSTATUS WINAPI set_event_completion( DEVICE_OBJECT *device, IRP *irp, void *context )
{
    if (irp->PendingReturned) KeSetEvent( (KEVENT *)context, IO_NO_INCREMENT, FALSE );
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS WINAPI driver_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    UCHAR code = stack->MinorFunction;
    NTSTATUS status;
    KEVENT event;

    TRACE( "device %p, irp %p, code %#x, bus_device %p.\n", device, irp, code, impl->bus_device );

    switch (stack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        KeInitializeEvent( &event, NotificationEvent, FALSE );
        IoCopyCurrentIrpStackLocationToNext( irp );
        IoSetCompletionRoutine( irp, set_event_completion, &event, TRUE, TRUE, TRUE );

        status = IoCallDriver( impl->bus_device, irp );
        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
            status = irp->IoStatus.Status;
        }

        if (status) irp->IoStatus.Status = status;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return status;

    case IRP_MN_SURPRISE_REMOVAL:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
        IoSkipCurrentIrpStackLocation( irp );
        status = IoCallDriver( impl->bus_device, irp );
        IoDetachDevice( impl->bus_device );
        IoDeleteDevice( device );
        return status;

    default:
        IoSkipCurrentIrpStackLocation( irp );
        return IoCallDriver( impl->bus_device, irp );
    }

    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI add_device( DRIVER_OBJECT *driver, DEVICE_OBJECT *bus_device )
{
    struct device *impl;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    TRACE( "driver %p, bus_device %p.\n", driver, bus_device );

    if ((status = IoCreateDevice( driver, sizeof(struct device), NULL, FILE_DEVICE_BUS_EXTENDER,
                                  0, FALSE, &device )))
    {
        ERR( "failed to create bus FDO, status %#lx.\n", status );
        return status;
    }

    impl = device->DeviceExtension;
    impl->bus_device = bus_device;

    IoAttachDeviceToDeviceStack( device, bus_device );
    device->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload( DRIVER_OBJECT *driver )
{
    TRACE( "driver %p\n", driver );
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    TRACE( "driver %p, path %s.\n", driver, debugstr_w(path->Buffer) );

    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->DriverExtension->AddDevice = add_device;
    driver->DriverUnload = driver_unload;

    return STATUS_SUCCESS;
}
