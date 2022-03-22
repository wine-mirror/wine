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
    BOOL is_phys;
};

static inline struct device *impl_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    return (struct device *)device->DeviceExtension;
}

struct phys_device
{
    struct device base;
    struct func_device *fdo; /* parent FDO */

    WCHAR instance_id[MAX_PATH];
    WCHAR device_id[MAX_PATH];
};

static inline struct phys_device *pdo_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    return CONTAINING_RECORD( impl, struct phys_device, base );
}

struct func_device
{
    struct device base;
    DEVICE_OBJECT *pdo; /* lower PDO */
    UNICODE_STRING control_iface;
    char devices_buffer[offsetof( DEVICE_RELATIONS, Objects[128] )];
    DEVICE_RELATIONS *devices;
};

static inline struct func_device *fdo_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return CONTAINING_RECORD( impl, struct phys_device, base )->fdo;
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

static NTSTATUS remove_child_device( struct func_device *impl, DEVICE_OBJECT *device )
{
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;
    ULONG i;

    KeAcquireSpinLock( &impl->base.lock, &irql );
    for (i = 0; i < impl->devices->Count; ++i)
        if (impl->devices->Objects[i] == device) break;
    if (i == impl->devices->Count) status = STATUS_NOT_FOUND;
    else impl->devices->Objects[i] = impl->devices->Objects[impl->devices->Count--];
    KeReleaseSpinLock( &impl->base.lock, irql );

    return status;
}

static NTSTATUS append_child_device( struct func_device *impl, DEVICE_OBJECT *device )
{
    NTSTATUS status;
    KIRQL irql;

    KeAcquireSpinLock( &impl->base.lock, &irql );
    if (offsetof( DEVICE_RELATIONS, Objects[impl->devices->Count + 1] ) > sizeof(impl->devices_buffer))
        status = STATUS_NO_MEMORY;
    else
    {
        impl->devices->Objects[impl->devices->Count++] = device;
        status = STATUS_SUCCESS;
    }
    KeReleaseSpinLock( &impl->base.lock, irql );

    return status;
}

static DEVICE_OBJECT *find_child_device( struct func_device *impl, struct bus_device_desc *desc )
{
    DEVICE_OBJECT *device = NULL, **devices;
    WCHAR device_id[MAX_PATH];
    KIRQL irql;
    ULONG i;

    swprintf( device_id, MAX_PATH, L"WINETEST\\VID_%04X&PID_%04X", desc->vid, desc->pid );

    KeAcquireSpinLock( &impl->base.lock, &irql );
    devices = impl->devices->Objects;
    for (i = 0; i < impl->devices->Count; ++i)
    {
        struct phys_device *phys = pdo_from_DEVICE_OBJECT( (device = devices[i]) );
        if (!wcscmp( phys->device_id, device_id )) break;
        else device = NULL;
    }
    KeReleaseSpinLock( &impl->base.lock, irql );

    return device;
}

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

static WCHAR *query_instance_id( DEVICE_OBJECT *device )
{
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    DWORD size = (wcslen( impl->instance_id ) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
        memcpy( dst, impl->instance_id, size );

    return dst;
}

static WCHAR *query_hardware_ids( DEVICE_OBJECT *device )
{
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    DWORD size = (wcslen( impl->device_id ) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size + sizeof(WCHAR) )))
    {
        memcpy( dst, impl->device_id, size );
        dst[size / sizeof(WCHAR)] = 0;
    }

    return dst;
}

static WCHAR *query_compatible_ids( DEVICE_OBJECT *device )
{
    DWORD size = 0;
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size + sizeof(WCHAR) )))
        dst[size / sizeof(WCHAR)] = 0;

    return dst;
}

static WCHAR *query_container_id( DEVICE_OBJECT *device )
{
    static const WCHAR winetest_id[] = L"WINETEST";
    DWORD size = sizeof(winetest_id);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
        memcpy( dst, winetest_id, sizeof(winetest_id) );

    return dst;
}

typedef struct _PNP_BUS_INFORMATION
{
    GUID BusTypeGuid;
    INTERFACE_TYPE LegacyBusType;
    ULONG BusNumber;
} PNP_BUS_INFORMATION, *PPNP_BUS_INFORMATION;

static PNP_BUS_INFORMATION *query_bus_information( DEVICE_OBJECT *device )
{
    DWORD size = sizeof(PNP_BUS_INFORMATION);
    PNP_BUS_INFORMATION *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
    {
        memset( &dst->BusTypeGuid, 0, sizeof(dst->BusTypeGuid) );
        dst->LegacyBusType = PNPBus;
        dst->BusNumber = 0;
    }

    return dst;
}

static WCHAR *query_device_text( DEVICE_OBJECT *device )
{
    static const WCHAR device_text[] = L"Wine Test HID device";
    DWORD size = sizeof(device_text);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
        memcpy( dst, device_text, size );

    return dst;
}

static NTSTATUS pdo_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    struct func_device *fdo = fdo_from_DEVICE_OBJECT( device );
    ULONG code = stack->MinorFunction;
    PNP_DEVICE_STATE state;
    NTSTATUS status;
    KIRQL irql;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_pnp(code) );

    switch (code)
    {
    case IRP_MN_START_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_REMOVE_DEVICE:
        state = (code == IRP_MN_START_DEVICE || code == IRP_MN_CANCEL_REMOVE_DEVICE) ? 0 : PNP_DEVICE_REMOVED;
        KeAcquireSpinLock( &impl->base.lock, &irql );
        impl->base.state = state;
        KeReleaseSpinLock( &impl->base.lock, irql );
        if (code != IRP_MN_REMOVE_DEVICE) status = STATUS_SUCCESS;
        else
        {
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest( irp, IO_NO_INCREMENT );
            if (remove_child_device( fdo, device ))
            {
                IoDeleteDevice( device );
                if (winetest_debug > 1) trace( "Deleted Bus PDO %p\n", device );
            }
            return STATUS_SUCCESS;
        }
        break;
    case IRP_MN_QUERY_CAPABILITIES:
    {
        DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
        caps->Removable = 1;
        caps->SilentInstall = 1;
        caps->SurpriseRemovalOK = 1;
        /* caps->RawDeviceOK = 1; */
        status = STATUS_SUCCESS;
        break;
    }
    case IRP_MN_QUERY_ID:
    {
        BUS_QUERY_ID_TYPE type = stack->Parameters.QueryId.IdType;
        switch (type)
        {
        case BusQueryDeviceID:
        case BusQueryHardwareIDs:
            irp->IoStatus.Information = (ULONG_PTR)query_hardware_ids( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case BusQueryInstanceID:
            irp->IoStatus.Information = (ULONG_PTR)query_instance_id( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case BusQueryCompatibleIDs:
            irp->IoStatus.Information = (ULONG_PTR)query_compatible_ids( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case BusQueryContainerID:
            irp->IoStatus.Information = (ULONG_PTR)query_container_id( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        default:
            ok( 0, "IRP_MN_QUERY_ID type %u, not implemented!\n", type );
            status = STATUS_NOT_SUPPORTED;
            break;
        }
        break;
    }
    case IRP_MN_QUERY_BUS_INFORMATION:
        irp->IoStatus.Information = (ULONG_PTR)query_bus_information( device );
        if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
        else status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_DEVICE_TEXT:
        irp->IoStatus.Information = (ULONG_PTR)query_device_text( device );
        if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
        else status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        irp->IoStatus.Information = impl->base.state;
        status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    {
        DEVICE_RELATION_TYPE type = stack->Parameters.QueryDeviceRelations.Type;
        switch (type)
        {
        case BusRelations:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS BusRelations\n" );
            ok( irp->IoStatus.Information, "got unexpected BusRelations relations\n" );
            status = irp->IoStatus.Status;
            break;
        case EjectionRelations:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS EjectionRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected EjectionRelations relations\n" );
            status = irp->IoStatus.Status;
            break;
        case RemovalRelations:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS RemovalRelations\n" );
            ok( irp->IoStatus.Information, "got unexpected RemovalRelations relations\n" );
            status = irp->IoStatus.Status;
            break;
        case TargetDeviceRelation:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS TargetDeviceRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected TargetDeviceRelations relations\n" );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information, 1, &device );
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
    default:
        if (winetest_debug > 1) trace( "pdo_pnp code %#lx %s, not implemented!\n", code, debugstr_pnp(code) );
        status = irp->IoStatus.Status;
        break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS create_child_pdo( DEVICE_OBJECT *device, struct bus_device_desc *desc )
{
    static ULONG index;

    struct func_device *fdo = fdo_from_DEVICE_OBJECT( device );
    struct phys_device *impl;
    UNICODE_STRING name_str;
    DEVICE_OBJECT *child;
    WCHAR name[MAX_PATH];
    NTSTATUS status;

    swprintf( name, MAX_PATH, L"\\Device\\WINETEST#%p&%p&%u", device->DriverObject, device, index++ );
    RtlInitUnicodeString( &name_str, name );

    if ((status = IoCreateDevice( device->DriverObject, sizeof(struct phys_device), &name_str, 0, 0, FALSE, &child )))
    {
        ok( 0, "Failed to create gamepad device, status %#lx\n", status );
        return status;
    }

    impl = pdo_from_DEVICE_OBJECT( child );
    KeInitializeSpinLock( &impl->base.lock );
    swprintf( impl->device_id, MAX_PATH, L"WINETEST\\VID_%04X&PID_%04X", desc->vid, desc->pid );
    swprintf( impl->instance_id, MAX_PATH, L"0&0000&0" );
    impl->base.is_phys = TRUE;
    impl->fdo = fdo;

    if (winetest_debug > 1) trace( "Created Bus PDO %p for Bus FDO %p\n", child, device );

    append_child_device( fdo, child );
    IoInvalidateDeviceRelations( fdo->pdo, BusRelations );
    return STATUS_SUCCESS;
}

static NTSTATUS fdo_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    struct func_device *impl = fdo_from_DEVICE_OBJECT( device );
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    char relations_buffer[sizeof(impl->devices_buffer)];
    DEVICE_RELATIONS *relations = (void *)relations_buffer;
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
        if (code == IRP_MN_REMOVE_DEVICE) memcpy( relations, impl->devices, sizeof(relations_buffer) );
        impl->devices->Count = 0;
        KeReleaseSpinLock( &impl->base.lock, irql );
        IoSetDeviceInterfaceState( &impl->control_iface, state != PNP_DEVICE_REMOVED );
        if (code != IRP_MN_REMOVE_DEVICE) status = STATUS_SUCCESS;
        else
        {
            while (relations->Count--) IoDeleteDevice( relations->Objects[relations->Count] );
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
            KeAcquireSpinLock( &impl->base.lock, &irql );
            memcpy( relations, impl->devices, sizeof(relations_buffer) );
            KeReleaseSpinLock( &impl->base.lock, irql );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information,
                                                              relations->Count, relations->Objects );
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
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return pdo_pnp( device, irp );
    return fdo_pnp( device, irp );
}

static NTSTATUS WINAPI pdo_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );
    ok( 0, "unexpected call\n" );

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS WINAPI fdo_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );
    ok( 0, "unexpected call\n" );

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS WINAPI driver_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return pdo_internal_ioctl( device, irp );
    return fdo_internal_ioctl( device, irp );
}

static NTSTATUS WINAPI pdo_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );
    ok( 0, "unexpected call\n" );

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS WINAPI fdo_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    struct func_device *impl = fdo_from_DEVICE_OBJECT( device );
    NTSTATUS status;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );

    switch (code)
    {
    case IOCTL_WINETEST_CREATE_DEVICE:
        if (in_size < sizeof(struct bus_device_desc)) status = STATUS_INVALID_PARAMETER;
        else status = create_child_pdo( device, irp->AssociatedIrp.SystemBuffer );
        break;
    case IOCTL_WINETEST_REMOVE_DEVICE:
        if ((device = find_child_device( impl, irp->AssociatedIrp.SystemBuffer )) &&
            !remove_child_device( impl, device ))
            IoInvalidateDeviceRelations( impl->pdo, BusRelations );
        status = STATUS_SUCCESS;
        break;
    default:
        ok( 0, "unexpected call\n" );
        status = irp->IoStatus.Status;
        break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS WINAPI driver_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return pdo_ioctl( device, irp );
    return fdo_ioctl( device, irp );
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
    impl->devices = (void *)impl->devices_buffer;
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
