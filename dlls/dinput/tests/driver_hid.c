/*
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

#if 0
#pragma makedep testdll
#endif

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

#define WINE_DRIVER_TEST
#include "initguid.h"
#include "driver_hid.h"

static DRIVER_OBJECT *expect_driver;

struct hid_device
{
    DEVICE_OBJECT *expect_bus_pdo;
    DEVICE_OBJECT *expect_hid_fdo;
    struct hid_device *expect_hid_ext;
    UNICODE_STRING control_symlink;
};

static void check_device( DEVICE_OBJECT *device )
{
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;
    struct hid_device *impl = ext->MiniDeviceExtension;

    ok( device == impl->expect_hid_fdo, "got device %p\n", device );
    ok( device->DriverObject == expect_driver, "got DriverObject %p\n", device->DriverObject );
    if (!device->NextDevice) ok( device == impl->expect_hid_fdo, "got device %p\n", device );
    else ok( device->NextDevice == impl->expect_hid_fdo, "got NextDevice %p\n", device->NextDevice );
    ok( !device->AttachedDevice, "got AttachedDevice %p\n", device->AttachedDevice );

    ok( ext->MiniDeviceExtension == impl->expect_hid_ext, "got MiniDeviceExtension %p\n", ext->MiniDeviceExtension );
    ok( ext->PhysicalDeviceObject == impl->expect_bus_pdo, "got PhysicalDeviceObject %p\n", ext->PhysicalDeviceObject );
    ok( ext->NextDeviceObject == impl->expect_bus_pdo, "got NextDeviceObject %p\n", ext->NextDeviceObject );
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

static NTSTATUS WINAPI driver_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;
    struct hid_device *impl = ext->MiniDeviceExtension;
    ULONG code = stack->MinorFunction;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_pnp(code) );

    switch (code)
    {
    case IRP_MN_START_DEVICE:
        IoSetDeviceInterfaceState( &impl->control_symlink, TRUE );
        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_REMOVE_DEVICE:
        IoSetDeviceInterfaceState( &impl->control_symlink, FALSE );
        RtlFreeUnicodeString( &impl->control_symlink );
        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
        irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
    {
        DEVICE_RELATION_TYPE type = stack->Parameters.QueryDeviceRelations.Type;
        switch (type)
        {
        case BusRelations:
        case EjectionRelations:
            if (winetest_debug > 1) trace( "IRP_MN_QUERY_DEVICE_RELATIONS type %u not handled\n", type );
            break;
        case RemovalRelations:
            ok( !irp->IoStatus.Information, "got unexpected RemovalRelations relations\n" );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information,
                                                              1, &ext->PhysicalDeviceObject );
            if (!irp->IoStatus.Information) irp->IoStatus.Status = STATUS_NO_MEMORY;
            else irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        default: ok( 0, "got unexpected IRP_MN_QUERY_DEVICE_RELATIONS type %#x\n", type ); break;
        }
        break;
    }
    }

    IoSkipCurrentIrpStackLocation( irp );
    return IoCallDriver( ext->NextDeviceObject, irp );
}

static NTSTATUS WINAPI driver_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;
    const ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );
    check_device( device );

    IoSkipCurrentIrpStackLocation( irp );
    return IoCallDriver( ext->PhysicalDeviceObject, irp );
}

static NTSTATUS (WINAPI *hidclass_driver_ioctl)( DEVICE_OBJECT *device, IRP *irp );
static NTSTATUS WINAPI driver_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );

    switch (code)
    {
    case IOCTL_WINETEST_HID_SET_EXPECT:
    case IOCTL_WINETEST_HID_WAIT_EXPECT:
    case IOCTL_WINETEST_HID_SEND_INPUT:
    case IOCTL_WINETEST_HID_SET_CONTEXT:
    case IOCTL_WINETEST_HID_WAIT_INPUT:
        IoSkipCurrentIrpStackLocation( irp );
        return IoCallDriver( ext->PhysicalDeviceObject, irp );

    case IOCTL_WINETEST_REMOVE_DEVICE:
    case IOCTL_WINETEST_CREATE_DEVICE:
        ok( 0, "unexpected call\n" );
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_NOT_SUPPORTED;
    }

    return hidclass_driver_ioctl( device, irp );
}

static NTSTATUS WINAPI driver_add_device( DRIVER_OBJECT *driver, DEVICE_OBJECT *device )
{
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;
    struct hid_device *impl = ext->MiniDeviceExtension;
    DEVICE_OBJECT *bus_pdo = ext->PhysicalDeviceObject;
    NTSTATUS status;

    if (winetest_debug > 1) trace( "%s: driver %p, device %p\n", __func__, driver, device );

    impl->expect_hid_fdo = device;
    impl->expect_bus_pdo = ext->PhysicalDeviceObject;
    impl->expect_hid_ext = ext->MiniDeviceExtension;

    todo_wine
    ok( impl->expect_bus_pdo->AttachedDevice == device, "got AttachedDevice %p\n", bus_pdo->AttachedDevice );
    ok( driver == expect_driver, "got driver %p\n", driver );
    check_device( device );

    status = IoRegisterDeviceInterface( ext->PhysicalDeviceObject, &control_class, NULL, &impl->control_symlink );
    ok( !status, "IoRegisterDeviceInterface returned %#lx\n", status );

    if (winetest_debug > 1) trace( "Created HID FDO %p for Bus PDO %p\n", device, ext->PhysicalDeviceObject );

    device->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create( DEVICE_OBJECT *device, IRP *irp )
{
    if (winetest_debug > 1) trace( "%s: device %p\n", __func__, device );
    ok( 0, "unexpected call\n" );
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close( DEVICE_OBJECT *device, IRP *irp )
{
    if (winetest_debug > 1) trace( "%s: device %p\n", __func__, device );
    ok( 0, "unexpected call\n" );
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
    HID_MINIDRIVER_REGISTRATION params =
    {
        .Revision = HID_REVISION,
        .DriverObject = driver,
        .DeviceExtensionSize = sizeof(struct hid_device),
        .RegistryPath = registry,
    };
    NTSTATUS status;

    expect_driver = driver;

    if ((status = winetest_init())) return status;
    if (winetest_debug > 1) trace( "%s: driver %p\n", __func__, driver );

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = driver_internal_ioctl;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    status = HidRegisterMinidriver( &params );
    ok( !status, "got %#lx\n", status );

    hidclass_driver_ioctl = driver->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;

    return STATUS_SUCCESS;
}
