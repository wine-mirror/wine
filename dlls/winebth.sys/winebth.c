/*
 * Bluetooth bus driver
 *
 * Copyright 2024 Vibhav Pant
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
#include <assert.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <winioctl.h>
#include <ddk/wdm.h>

#include <wine/debug.h>
#include <wine/list.h>

#include "winebth_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL( winebth );

static DRIVER_OBJECT *driver_obj;

static DEVICE_OBJECT *bus_fdo, *bus_pdo;

static NTSTATUS WINAPI fdo_pnp( DEVICE_OBJECT *device_obj, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );

    TRACE( "irp %p, minor function %s.\n", irp, debugstr_minor_function_code( stack->MinorFunction ) );

    switch (stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IRP_MN_SURPRISE_REMOVAL:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IRP_MN_REMOVE_DEVICE:
        {
            NTSTATUS ret;
            IoSkipCurrentIrpStackLocation( irp );
            ret = IoCallDriver( bus_pdo, irp );
            IoDetachDevice( bus_pdo );
            IoDeleteDevice( bus_fdo );
            return ret;
        }
        default:
            FIXME( "Unhandled minor function %s.\n", debugstr_minor_function_code( stack->MinorFunction ) );
    }

    IoSkipCurrentIrpStackLocation( irp );
    return IoCallDriver( bus_pdo, irp );
}

static NTSTATUS WINAPI pdo_pnp( DEVICE_OBJECT *device_obj, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret = irp->IoStatus.Status;

    TRACE( "device_obj %p, irp %p, minor function %s\n", device_obj, irp, debugstr_minor_function_code( stack->MinorFunction ) );
    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
            caps->Removable = TRUE;
            caps->SurpriseRemovalOK = TRUE;
            caps->RawDeviceOK = TRUE;
            ret = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_START_DEVICE:
            ret = STATUS_SUCCESS;
            break;
        case IRP_MN_REMOVE_DEVICE:
            IoDeleteDevice( device_obj );
        case IRP_MN_SURPRISE_REMOVAL:
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest( irp, IO_NO_INCREMENT );
            return STATUS_SUCCESS;
        default:
            FIXME( "Unhandled minor function %s.\n", debugstr_minor_function_code( stack->MinorFunction ) );
            break;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return ret;
}

static NTSTATUS WINAPI bluetooth_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    if (device == bus_fdo)
        return fdo_pnp( device, irp );
    return pdo_pnp( device, irp );
}

static NTSTATUS WINAPI driver_add_device( DRIVER_OBJECT *driver, DEVICE_OBJECT *pdo )
{
    NTSTATUS ret;

    TRACE( "(%p, %p)\n", driver, pdo );
    ret = IoCreateDevice( driver, 0, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &bus_fdo );
    if (ret != STATUS_SUCCESS)
    {
        ERR( "failed to create FDO: %#lx\n", ret );
        return ret;
    }

    IoAttachDeviceToDeviceStack( bus_fdo, pdo );
    bus_pdo = pdo;
    bus_fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload( DRIVER_OBJECT *driver ) {}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    TRACE( "(%p, %s)\n", driver, debugstr_w( path->Buffer ) );

    driver->DriverExtension->AddDevice = driver_add_device;
    driver_obj = driver;

    driver->MajorFunction[IRP_MJ_PNP] = bluetooth_pnp;
    driver->DriverUnload = driver_unload;

    return STATUS_SUCCESS;
}
