/*
 * Plug and Play test driver
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#include "wine/list.h"

#include "initguid.h"
#include "driver_hid.h"

typedef ULONG PNP_DEVICE_STATE;
#define PNP_DEVICE_REMOVED 8

struct device
{
    KSPIN_LOCK lock;
    PNP_DEVICE_STATE state;
};

static inline struct device *impl_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    return (struct device *)device->DeviceExtension;
}

struct func_device
{
    struct device base;
    DEVICE_OBJECT *pdo; /* lower PDO */
    UNICODE_STRING control_iface;
};

static inline struct func_device *fdo_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    return CONTAINING_RECORD( impl, struct func_device, base );
}

#ifdef __ASM_USE_FASTCALL_WRAPPER
extern void *WINAPI wrap_fastcall_func1( void *func, const void *a );
__ASM_STDCALL_FUNC( wrap_fastcall_func1, 8,
                    "popl %ecx\n\t"
                    "popl %eax\n\t"
                    "xchgl (%esp),%ecx\n\t"
                    "jmp *%eax" );
#define call_fastcall_func1( func, a ) wrap_fastcall_func1( func, a )
#else
#define call_fastcall_func1( func, a ) func( a )
#endif

static ULONG_PTR get_device_relations( DEVICE_OBJECT *device, DEVICE_RELATIONS *previous,
                                       ULONG count, DEVICE_OBJECT **devices )
{
    DEVICE_RELATIONS *relations;
    ULONG new_count = count;

    if (previous) new_count += previous->Count;
    if (!(relations = ExAllocatePool( PagedPool, offsetof( DEVICE_RELATIONS, Objects[new_count] ) )))
    {
        ok( 0, "Failed to allocate memory\n" );
        return (ULONG_PTR)previous;
    }

    if (!previous) relations->Count = 0;
    else
    {
        memcpy( relations, previous, offsetof( DEVICE_RELATIONS, Objects[previous->Count] ) );
        ExFreePool( previous );
    }

    while (count--)
    {
        call_fastcall_func1( ObfReferenceObject, *devices );
        relations->Objects[relations->Count++] = *devices++;
    }

    return (ULONG_PTR)relations;
}

static NTSTATUS fdo_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    struct func_device *impl = fdo_from_DEVICE_OBJECT( device );
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->MinorFunction;
    PNP_DEVICE_STATE state;
    NTSTATUS status;
    KIRQL irql;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_pnp(code) );

    switch (code)
    {
    case IRP_MN_START_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_REMOVE_DEVICE:
        state = (code == IRP_MN_START_DEVICE || code == IRP_MN_CANCEL_REMOVE_DEVICE) ? 0 : PNP_DEVICE_REMOVED;
        KeAcquireSpinLock( &impl->base.lock, &irql );
        impl->base.state = state;
        KeReleaseSpinLock( &impl->base.lock, irql );
        IoSetDeviceInterfaceState( &impl->control_iface, state != PNP_DEVICE_REMOVED );
        if (code != IRP_MN_REMOVE_DEVICE) status = STATUS_SUCCESS;
        else
        {
            IoSkipCurrentIrpStackLocation( irp );
            status = IoCallDriver( impl->pdo, irp );
            IoDetachDevice( impl->pdo );
            RtlFreeUnicodeString( &impl->control_iface );
            IoDeleteDevice( device );
            if (winetest_debug > 1) trace( "Deleted Bus FDO %p from PDO %p, status %#lx\n", impl, impl->pdo, status );
            return status;
        }
        break;
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        KeAcquireSpinLock( &impl->base.lock, &irql );
        irp->IoStatus.Information = impl->base.state;
        KeReleaseSpinLock( &impl->base.lock, irql );
        status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    {
        DEVICE_RELATION_TYPE type = stack->Parameters.QueryDeviceRelations.Type;
        switch (type)
        {
        case BusRelations:
            if (winetest_debug > 1) trace( "fdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS BusRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected BusRelations relations\n" );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information, 0, NULL );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case RemovalRelations:
            if (winetest_debug > 1) trace( "fdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS RemovalRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected RemovalRelations relations\n" );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information,
                                                              1, &impl->pdo );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        default:
            ok( 0, "got unexpected IRP_MN_QUERY_DEVICE_RELATIONS type %#x\n", type );
            status = irp->IoStatus.Status;
            break;
        }
        break;
    }
    case IRP_MN_QUERY_CAPABILITIES:
    {
        DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
        caps->EjectSupported = TRUE;
        caps->Removable = TRUE;
        caps->SilentInstall = TRUE;
        caps->SurpriseRemovalOK = TRUE;
        status = STATUS_SUCCESS;
        break;
    }
    default:
        if (winetest_debug > 1) trace( "fdo_pnp code %#lx %s, not implemented!\n", code, debugstr_pnp(code) );
        status = irp->IoStatus.Status;
        break;
    }

    irp->IoStatus.Status = status;
    IoSkipCurrentIrpStackLocation( irp );
    return IoCallDriver( impl->pdo, irp );
}

static NTSTATUS WINAPI driver_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    return fdo_pnp( device, irp );
}

static NTSTATUS WINAPI driver_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );
    ok( 0, "unexpected call\n" );

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS WINAPI driver_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );
    ok( 0, "unexpected call\n" );

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS WINAPI driver_add_device( DRIVER_OBJECT *driver, DEVICE_OBJECT *device )
{
    struct func_device *impl;
    DEVICE_OBJECT *child;
    NTSTATUS status;

    if (winetest_debug > 1) trace( "%s: driver %p, device %p\n", __func__, driver, device );

    if ((status = IoCreateDevice( driver, sizeof(struct func_device), NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &child )))
    {
        ok( 0, "Failed to create FDO, status %#lx.\n", status );
        return status;
    }

    impl = (struct func_device *)child->DeviceExtension;
    KeInitializeSpinLock( &impl->base.lock );
    impl->pdo = device;

    status = IoRegisterDeviceInterface( impl->pdo, &control_class, NULL, &impl->control_iface );
    ok( !status, "IoRegisterDeviceInterface returned %#lx\n", status );

    if (winetest_debug > 1) trace( "Created Bus FDO %p for PDO %p\n", child, device );

    IoAttachDeviceToDeviceStack( child, device );
    child->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create( DEVICE_OBJECT *device, IRP *irp )
{
    if (winetest_debug > 1) trace( "%s: device %p, irp %p\n", __func__, device, irp );

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close( DEVICE_OBJECT *device, IRP *irp )
{
    if (winetest_debug > 1) trace( "%s: device %p, irp %p\n", __func__, device, irp );

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload( DRIVER_OBJECT *driver )
{
    if (winetest_debug > 1) trace( "%s: driver %p\n", __func__, driver );
    winetest_cleanup();
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *registry )
{
    NTSTATUS status;

    if ((status = winetest_init())) return status;
    if (winetest_debug > 1) trace( "%s: driver %p\n", __func__, driver );

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = driver_internal_ioctl;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    return STATUS_SUCCESS;
}
