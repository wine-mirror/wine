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
    PHIDP_PREPARSED_DATA preparsed;

    FILE_OBJECT dummy_file;
    IO_STATUS_BLOCK io;
    ULONG report_len;
    char *report_buf;

    ULONG caps_count;
    ULONG contact_max;
    HIDP_VALUE_CAPS *id_caps;
    HIDP_VALUE_CAPS *x_caps;
    HIDP_VALUE_CAPS *y_caps;
};

static inline struct device *impl_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    return (struct device *)device->DeviceExtension;
}

static inline LONG sign_extend( ULONG value, const HIDP_VALUE_CAPS *caps )
{
    UINT sign = 1 << (caps->BitSize - 1);
    if (sign <= 1 || caps->LogicalMin >= 0) return value;
    return value - ((value & sign) << 1);
}

static inline LONG scale_value( ULONG value, const HIDP_VALUE_CAPS *caps, LONG min, LONG max )
{
    LONG tmp = sign_extend( value, caps );
    if (caps->LogicalMin > caps->LogicalMax) return 0;
    if (caps->LogicalMin > tmp || caps->LogicalMax < tmp) return 0;
    return min + MulDiv( tmp - caps->LogicalMin, max - min, caps->LogicalMax - caps->LogicalMin );
}

static NTSTATUS WINAPI read_completion( DEVICE_OBJECT *device, IRP *irp, void *context );

static NTSTATUS start_device_read( DEVICE_OBJECT *device )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    IO_STACK_LOCATION *stack;
    NTSTATUS status;
    IRP *irp;

    TRACE( "device %p\n", device );

    irp = IoBuildAsynchronousFsdRequest( IRP_MJ_READ, device, impl->report_buf,
                                         impl->report_len, NULL, &impl->io );
    if (!irp) return STATUS_NO_MEMORY;
    irp->Tail.Overlay.OriginalFileObject = &impl->dummy_file;
    stack = IoGetNextIrpStackLocation( irp );
    stack->FileObject = &impl->dummy_file;

    TRACE( "created irp %p\n", irp );

    IoSetCompletionRoutine( irp, read_completion, device, TRUE, TRUE, TRUE );
    if ((status = IoCallDriver( impl->bus_device, irp )) && status != STATUS_PENDING) return status;
    return STATUS_SUCCESS;
}

static void process_hid_report( struct device *impl, char *report_buf, UINT report_len )
{
    ULONG contact_count;
    NTSTATUS status;

    TRACE( "impl %p, report_buf %p, report_len %u\n", impl, report_buf, report_len );

    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_DIGITIZER, 0, HID_USAGE_DIGITIZER_CONTACT_COUNT,
                                 &contact_count, impl->preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return;
    if (contact_count > impl->contact_max)
    {
        WARN( "got %lu contacts, capping to %lu.\n", contact_count, impl->contact_max );
        contact_count = impl->contact_max;
    }

    FIXME( "Not implemented\n" );
}

static NTSTATUS WINAPI read_completion( DEVICE_OBJECT *device, IRP *irp, void *context )
{
    struct device *impl = impl_from_DEVICE_OBJECT( context );
    NTSTATUS status;

    TRACE( "device %p, irp %p, context %p\n", device, irp, context );

    if (irp->IoStatus.Status)
        WARN( "device read failed with status %#lx, stopping\n", irp->IoStatus.Status );
    else
    {
        process_hid_report( impl, impl->report_buf, impl->report_len );

        if (!InterlockedOr( &impl->removed, FALSE ) && (status = start_device_read( context )))
            ERR( "Failed to start next read, status %#lx\n", status );
    }

    if (irp->PendingReturned) IoMarkIrpPending( irp );
    return STATUS_SUCCESS;
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

static NTSTATUS call_hid_device( DEVICE_OBJECT *device, DWORD major, DWORD code, void *in_buf,
                                 DWORD in_len, void *out_buf, DWORD out_len )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    IO_STACK_LOCATION *stack;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    KEVENT event;
    IRP *irp;

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    if (major == IRP_MJ_DEVICE_CONTROL)
        irp = IoBuildDeviceIoControlRequest( code, device, in_buf, in_len, out_buf, out_len,
                                             FALSE, &event, &io );
    else
        irp = IoBuildSynchronousFsdRequest( code, device, out_buf, out_len, NULL, &event, &io );
    if (!irp) return STATUS_NO_MEMORY;

    irp->Tail.Overlay.OriginalFileObject = &impl->dummy_file;
    if (code == IRP_MJ_CREATE) irp->Flags |= IRP_CREATE_OPERATION;
    if (code == IRP_MJ_CLOSE) irp->Flags |= IRP_CLOSE_OPERATION;
    stack = IoGetNextIrpStackLocation( irp );
    stack->FileObject = &impl->dummy_file;

    status = IoCallDriver( impl->bus_device, irp );
    if (status == STATUS_PENDING) KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
    return io.Status;
}

static NTSTATUS initialize_device( DEVICE_OBJECT *device )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    HID_COLLECTION_INFORMATION info;
    USHORT usage, count, report_len;
    HIDP_VALUE_CAPS value_caps;
    char *report_buf;
    NTSTATUS status;
    HIDP_CAPS caps;

    if ((status = call_hid_device( device, IRP_MJ_CREATE, 0, NULL, 0, NULL, 0 ))) return status;
    if ((status = call_hid_device( device, IRP_MJ_DEVICE_CONTROL, IOCTL_HID_GET_COLLECTION_INFORMATION,
                                   &info, 0, &info, sizeof(info) )))
        return status;
    if (!(impl->preparsed = malloc( info.DescriptorSize ))) return STATUS_NO_MEMORY;
    if ((status = call_hid_device( device, IRP_MJ_DEVICE_CONTROL, IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                   NULL, 0, impl->preparsed, info.DescriptorSize )))
        return status;

    status = HidP_GetCaps( impl->preparsed, &caps );
    if (status != HIDP_STATUS_SUCCESS) return status;

    count = 1;
    usage = HID_USAGE_DIGITIZER_CONTACT_COUNT_MAX;
    status = HidP_GetSpecificValueCaps( HidP_Feature, HID_USAGE_PAGE_DIGITIZER, 0, usage,
                                        &value_caps, &count, impl->preparsed );
    if (status == HIDP_STATUS_SUCCESS)
    {
        report_len = caps.FeatureReportByteLength;
        if (!(report_buf = malloc( report_len ))) return STATUS_NO_MEMORY;
        report_buf[0] = value_caps.ReportID;

        status = call_hid_device( device, IRP_MJ_DEVICE_CONTROL, IOCTL_HID_GET_FEATURE, NULL, 0, report_buf, report_len );
        if (!status) status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_DIGITIZER, 0, usage,
                                                  &impl->contact_max, impl->preparsed, report_buf, report_len );
        free( report_buf );
        if (status && status != HIDP_STATUS_SUCCESS) return status;

        count = 1;
        usage = HID_USAGE_DIGITIZER_CONTACT_COUNT;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_DIGITIZER, 0, usage,
                                            &value_caps, &count, impl->preparsed );
        if (status != HIDP_STATUS_SUCCESS) goto failed;

        count = caps.NumberLinkCollectionNodes;
        usage = HID_USAGE_DIGITIZER_CONTACT_ID;
        if (!(impl->id_caps = malloc( sizeof(*impl->id_caps) * caps.NumberLinkCollectionNodes ))) return STATUS_NO_MEMORY;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_DIGITIZER, 0, usage,
                                            impl->id_caps, &count, impl->preparsed );
        if (status != HIDP_STATUS_SUCCESS) goto failed;
        impl->caps_count = count;

        count = impl->caps_count;
        usage = HID_USAGE_GENERIC_X;
        if (!(impl->x_caps = malloc( sizeof(*impl->x_caps) * impl->caps_count ))) return STATUS_NO_MEMORY;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, usage,
                                            impl->x_caps, &count, impl->preparsed );
        if (status != HIDP_STATUS_SUCCESS) goto failed;

        count = impl->caps_count;
        usage = HID_USAGE_GENERIC_Y;
        if (!(impl->y_caps = malloc( sizeof(*impl->y_caps) * impl->caps_count ))) return STATUS_NO_MEMORY;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, usage,
                                            impl->y_caps, &count, impl->preparsed );
        if (status != HIDP_STATUS_SUCCESS) goto failed;

        impl->report_len = caps.InputReportByteLength;
        if (!(impl->report_buf = malloc( impl->report_len ))) return STATUS_NO_MEMORY;
        impl->report_buf[0] = value_caps.ReportID;

        return STATUS_SUCCESS;
    }

failed:
    ERR( "%#x usage not found, unsupported device\n", usage );
    return STATUS_NOT_SUPPORTED;
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
        if (!status) status = initialize_device( device );
        if (!status) status = start_device_read( device );

        if (status) irp->IoStatus.Status = status;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return status;

    case IRP_MN_SURPRISE_REMOVAL:
        status = STATUS_SUCCESS;
        if (InterlockedExchange( &impl->removed, TRUE )) break;
        call_hid_device( device, IRP_MJ_CLOSE, 0, NULL, 0, NULL, 0 );
        break;

    case IRP_MN_REMOVE_DEVICE:
        IoSkipCurrentIrpStackLocation( irp );
        status = IoCallDriver( impl->bus_device, irp );
        IoDetachDevice( impl->bus_device );
        free( impl->id_caps );
        free( impl->x_caps );
        free( impl->y_caps );
        free( impl->report_buf );
        free( impl->preparsed );
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
