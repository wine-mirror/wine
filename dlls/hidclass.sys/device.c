/*
 * HIDClass device functions
 *
 * Copyright (C) 2015 Aric Stewart
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
#include "hid.h"
#include "winreg.h"
#include "winuser.h"

#include "ddk/hidsdi.h"
#include "ddk/hidtypes.h"
#include "ddk/wdm.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static void WINAPI read_cancel_routine(DEVICE_OBJECT *device, IRP *irp)
{
    struct hid_queue *queue = irp->Tail.Overlay.OriginalFileObject->FsContext;
    KIRQL irql;

    TRACE("cancel %p IRP on device %p\n", irp, device);

    IoReleaseCancelSpinLock(irp->CancelIrql);

    KeAcquireSpinLock( &queue->lock, &irql );

    RemoveEntryList(&irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock( &queue->lock, irql );

    irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

static struct hid_report *hid_report_create( HID_XFER_PACKET *packet, ULONG length )
{
    struct hid_report *report;

    if (!(report = malloc( offsetof( struct hid_report, buffer[length] ) )))
        return NULL;
    report->ref = 1;
    report->length = length;
    memcpy( report->buffer, packet->reportBuffer, packet->reportBufferLen );
    memset( report->buffer + packet->reportBufferLen, 0, length - packet->reportBufferLen );

    return report;
}

static void hid_report_incref( struct hid_report *report )
{
    InterlockedIncrement( &report->ref );
}

static void hid_report_decref( struct hid_report *report )
{
    if (!report) return;
    if (InterlockedDecrement( &report->ref ) == 0) free( report );
}

static struct hid_queue *hid_queue_create( void )
{
    struct hid_queue *queue;

    if (!(queue = calloc( 1, sizeof(struct hid_queue) ))) return NULL;
    InitializeListHead( &queue->irp_queue );
    KeInitializeSpinLock( &queue->lock );
    list_init( &queue->entry );
    queue->length = 32;
    queue->read_idx = 0;
    queue->write_idx = 0;

    return queue;
}

static IRP *hid_queue_pop_irp( struct hid_queue *queue )
{
    LIST_ENTRY *entry;
    IRP *irp = NULL;
    KIRQL irql;

    KeAcquireSpinLock( &queue->lock, &irql );

    while (!irp && (entry = RemoveHeadList( &queue->irp_queue )) != &queue->irp_queue)
    {
        irp = CONTAINING_RECORD( entry, IRP, Tail.Overlay.ListEntry );
        if (!IoSetCancelRoutine( irp, NULL ))
        {
            /* cancel routine is already cleared, meaning that it was called. let it handle completion. */
            InitializeListHead( &irp->Tail.Overlay.ListEntry );
            irp = NULL;
        }
    }

    KeReleaseSpinLock( &queue->lock, irql );
    return irp;
}

void hid_queue_remove_pending_irps( struct hid_queue *queue )
{
    IRP *irp;

    while ((irp = hid_queue_pop_irp( queue )))
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
}

void hid_queue_destroy( struct hid_queue *queue )
{
    hid_queue_remove_pending_irps( queue );
    while (queue->length--) hid_report_decref( queue->reports[queue->length] );
    list_remove( &queue->entry );
    free( queue );
}

static NTSTATUS hid_queue_resize( struct hid_queue *queue, ULONG length )
{
    struct hid_report *old_reports[512];
    LONG old_length = queue->length;
    KIRQL irql;

    if (length < 2 || length > 512) return STATUS_INVALID_PARAMETER;
    if (length == queue->length) return STATUS_SUCCESS;

    KeAcquireSpinLock( &queue->lock, &irql );
    memcpy( old_reports, queue->reports, old_length * sizeof(void *) );
    memset( queue->reports, 0, old_length * sizeof(void *) );
    queue->length = length;
    queue->write_idx = 0;
    queue->read_idx = 0;
    KeReleaseSpinLock( &queue->lock, irql );

    while (old_length--) hid_report_decref( old_reports[old_length] );
    return STATUS_SUCCESS;
}

static NTSTATUS hid_queue_push_irp( struct hid_queue *queue, IRP *irp )
{
    KIRQL irql;

    KeAcquireSpinLock( &queue->lock, &irql );

    IoSetCancelRoutine( irp, read_cancel_routine );
    if (irp->Cancel && !IoSetCancelRoutine( irp, NULL ))
    {
        /* IRP was canceled before we set cancel routine */
        InitializeListHead( &irp->Tail.Overlay.ListEntry );
        KeReleaseSpinLock( &queue->lock, irql );
        return STATUS_CANCELLED;
    }

    InsertTailList( &queue->irp_queue, &irp->Tail.Overlay.ListEntry );
    irp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending( irp );

    KeReleaseSpinLock( &queue->lock, irql );
    return STATUS_PENDING;
}

static void hid_queue_push_report( struct hid_queue *queue, struct hid_report *report )
{
    ULONG i = queue->write_idx, next = i + 1;
    struct hid_report *prev;
    KIRQL irql;

    if (next >= queue->length) next = 0;
    hid_report_incref( report );

    KeAcquireSpinLock( &queue->lock, &irql );
    prev = queue->reports[i];
    queue->reports[i] = report;
    if (next == queue->read_idx) queue->read_idx = next + 1;
    if (queue->read_idx >= queue->length) queue->read_idx = 0;
    KeReleaseSpinLock( &queue->lock, irql );

    hid_report_decref( prev );
    queue->write_idx = next;
}

static struct hid_report *hid_queue_pop_report( struct hid_queue *queue )
{
    ULONG i = queue->read_idx, next = i + 1;
    struct hid_report *report;
    KIRQL irql;

    if (next >= queue->length) next = 0;

    KeAcquireSpinLock( &queue->lock, &irql );
    report = queue->reports[i];
    queue->reports[i] = NULL;
    if (i != queue->write_idx) queue->read_idx = next;
    KeReleaseSpinLock( &queue->lock, irql );

    return report;
}

static void hid_device_queue_input( DEVICE_OBJECT *device, HID_XFER_PACKET *packet )
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    HIDP_COLLECTION_DESC *desc = ext->u.pdo.device_desc.CollectionDesc;
    const BOOL polled = ext->u.pdo.information.Polled;
    ULONG size, report_len = polled ? packet->reportBufferLen : desc->InputLength;
    struct hid_report *last_report, *report;
    struct hid_queue *queue;
    LIST_ENTRY completed, *entry;
    RAWINPUT *rawinput;
    KIRQL irql;
    IRP *irp;

    if (IsEqualGUID( ext->class_guid, &GUID_DEVINTERFACE_HID ))
    {
        size = offsetof( RAWINPUT, data.hid.bRawData[report_len] );
        if (!(rawinput = malloc( size ))) ERR( "Failed to allocate rawinput data!\n" );
        else
        {
            INPUT input;

            rawinput->header.dwType = RIM_TYPEHID;
            rawinput->header.dwSize = size;
            rawinput->header.hDevice = ULongToHandle( ext->u.pdo.rawinput_handle );
            rawinput->header.wParam = RIM_INPUT;
            rawinput->data.hid.dwCount = 1;
            rawinput->data.hid.dwSizeHid = report_len;
            memcpy( rawinput->data.hid.bRawData, packet->reportBuffer, packet->reportBufferLen );
            memset( rawinput->data.hid.bRawData + packet->reportBufferLen, 0, report_len - packet->reportBufferLen );

            input.type = INPUT_HARDWARE;
            input.hi.uMsg = WM_INPUT;
            input.hi.wParamH = 0;
            input.hi.wParamL = 0;
            __wine_send_input( 0, &input, rawinput );

            free( rawinput );
        }
    }

    if (!(last_report = hid_report_create( packet, report_len )))
    {
        ERR( "Failed to allocate hid_report!\n" );
        return;
    }

    InitializeListHead( &completed );

    KeAcquireSpinLock( &ext->u.pdo.queues_lock, &irql );
    LIST_FOR_EACH_ENTRY( queue, &ext->u.pdo.queues, struct hid_queue, entry )
    {
        if (!polled) hid_queue_push_report( queue, last_report );

        do
        {
            if (!(irp = hid_queue_pop_irp( queue ))) break;
            if (!(report = hid_queue_pop_report( queue ))) hid_report_incref( (report = last_report) );

            memcpy( irp->AssociatedIrp.SystemBuffer, report->buffer, report->length );
            irp->IoStatus.Information = report->length;
            irp->IoStatus.Status = STATUS_SUCCESS;
            hid_report_decref( report );

            InsertTailList( &completed, &irp->Tail.Overlay.ListEntry );
        }
        while (polled);
    }
    KeReleaseSpinLock( &ext->u.pdo.queues_lock, irql );

    while ((entry = RemoveHeadList( &completed )) != &completed)
    {
        irp = CONTAINING_RECORD( entry, IRP, Tail.Overlay.ListEntry );
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }

    hid_report_decref( last_report );
}

static HIDP_REPORT_IDS *find_report_with_type_and_id( BASE_DEVICE_EXTENSION *ext, BYTE type, BYTE id, BOOL any_id )
{
    HIDP_REPORT_IDS *report, *reports = ext->u.pdo.device_desc.ReportIDs;
    ULONG report_count = ext->u.pdo.device_desc.ReportIDsLength;

    for (report = reports; report != reports + report_count; report++)
    {
        if (!any_id && report->ReportID && report->ReportID != id) continue;
        if (type == HidP_Input && report->InputLength) return report;
        if (type == HidP_Output && report->OutputLength) return report;
        if (type == HidP_Feature && report->FeatureLength) return report;
    }

    return NULL;
}

static DWORD CALLBACK hid_device_thread(void *args)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)args;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    HIDP_COLLECTION_DESC *desc = ext->u.pdo.device_desc.CollectionDesc;
    BOOL polled = ext->u.pdo.information.Polled;
    HIDP_REPORT_IDS *report;
    HID_XFER_PACKET *packet;
    ULONG report_id = 0;
    IO_STATUS_BLOCK io;
    BYTE *buffer;
    DWORD res;

    packet = malloc( sizeof(*packet) + desc->InputLength );
    buffer = (BYTE *)(packet + 1);

    report = find_report_with_type_and_id( ext, HidP_Input, 0, TRUE );
    if (!report) WARN("no input report found.\n");
    else report_id = report->ReportID;

    do
    {
        packet->reportId = buffer[0] = report_id;
        packet->reportBuffer = buffer;
        packet->reportBufferLen = desc->InputLength;

        if (!report_id)
        {
            packet->reportBuffer++;
            packet->reportBufferLen--;
        }

        call_minidriver( IOCTL_HID_READ_REPORT, ext->u.pdo.parent_fdo, NULL, 0,
                         packet->reportBuffer, packet->reportBufferLen, &io );

        if (io.Status == STATUS_SUCCESS)
        {
            if (!report_id) io.Information++;
            if (!(report = find_report_with_type_and_id( ext, HidP_Input, buffer[0], FALSE )))
                WARN( "dropping unknown input id %u\n", buffer[0] );
            else if (!polled && io.Information < report->InputLength)
                WARN( "dropping short report, len %Iu expected %u\n", io.Information, report->InputLength );
            else
            {
                packet->reportId = buffer[0];
                packet->reportBuffer = buffer;
                packet->reportBufferLen = io.Information;
                hid_device_queue_input( device, packet );
            }
        }

        res = WaitForSingleObject(ext->u.pdo.halt_event, polled ? ext->u.pdo.poll_interval : 0);
    } while (res == WAIT_TIMEOUT);

    TRACE( "device thread exiting, res %#lx\n", res );
    return 1;
}

void HID_StartDeviceThread(DEVICE_OBJECT *device)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    ext->u.pdo.halt_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ext->u.pdo.thread = CreateThread(NULL, 0, hid_device_thread, device, 0, NULL);
}

static void handle_IOCTL_HID_GET_COLLECTION_INFORMATION( IRP *irp, BASE_DEVICE_EXTENSION *ext )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <  sizeof(HID_COLLECTION_INFORMATION))
    {
        irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy(irp->AssociatedIrp.SystemBuffer, &ext->u.pdo.information, sizeof(HID_COLLECTION_INFORMATION));
        irp->IoStatus.Information = sizeof(HID_COLLECTION_INFORMATION);
        irp->IoStatus.Status = STATUS_SUCCESS;
    }
}

static void handle_IOCTL_HID_GET_COLLECTION_DESCRIPTOR( IRP *irp, BASE_DEVICE_EXTENSION *ext )
{
    HIDP_COLLECTION_DESC *desc = ext->u.pdo.device_desc.CollectionDesc;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < desc->PreparsedDataLength)
    {
        irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy( irp->UserBuffer, desc->PreparsedData, desc->PreparsedDataLength );
        irp->IoStatus.Information = desc->PreparsedDataLength;
        irp->IoStatus.Status = STATUS_SUCCESS;
    }
}

struct device_strings
{
    const WCHAR *id;
    const WCHAR *product;
};

static const struct device_strings device_strings[] =
{
    /* Microsoft controllers */
    { .id = L"VID_045E&PID_028E", .product = L"Controller (XBOX 360 For Windows)" },
    { .id = L"VID_045E&PID_028F", .product = L"Controller (XBOX 360 For Windows)" },
    { .id = L"VID_045E&PID_02D1", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_02DD", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_02E3", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_02EA", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_02FD", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_0719", .product = L"Controller (XBOX 360 For Windows)" },
    { .id = L"VID_045E&PID_0B00", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_0B05", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_0B12", .product = L"Controller (Xbox One For Windows)" },
    { .id = L"VID_045E&PID_0B13", .product = L"Controller (Xbox One For Windows)" },
    /* Sony controllers */
    { .id = L"VID_054C&PID_05C4", .product = L"Wireless Controller" },
    { .id = L"VID_054C&PID_09CC", .product = L"Wireless Controller" },
    { .id = L"VID_054C&PID_0BA0", .product = L"Wireless Controller" },
    { .id = L"VID_054C&PID_0CE6", .product = L"Wireless Controller" },
};

static const WCHAR *find_product_string( const WCHAR *device_id )
{
    const WCHAR *match_id = wcsrchr( device_id, '\\' ) + 1;
    DWORD i;

    for (i = 0; i < ARRAY_SIZE(device_strings); ++i)
        if (!wcsnicmp( device_strings[i].id, match_id, 17 ))
            return device_strings[i].product;

    return NULL;
}

static void handle_minidriver_string( BASE_DEVICE_EXTENSION *ext, IRP *irp, ULONG index )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    WCHAR *output_buf = MmGetSystemAddressForMdlSafe( irp->MdlAddress, NormalPagePriority );
    ULONG output_len = stack->Parameters.DeviceIoControl.OutputBufferLength;
    const WCHAR *str = NULL;

    if (index == HID_STRING_ID_IPRODUCT) str = find_product_string( ext->device_id );

    if (!str) call_minidriver( IOCTL_HID_GET_STRING, ext->u.pdo.parent_fdo, ULongToPtr( index ),
                               sizeof(index), output_buf, output_len, &irp->IoStatus );
    else
    {
        irp->IoStatus.Information = (wcslen( str ) + 1) * sizeof(WCHAR);
        if (irp->IoStatus.Information > output_len)
            irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            memcpy( output_buf, str, irp->IoStatus.Information );
            irp->IoStatus.Status = STATUS_SUCCESS;
        }
    }
}

static void hid_device_xfer_report( BASE_DEVICE_EXTENSION *ext, ULONG code, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG offset = 0, report_len = 0, buffer_len = 0;
    HIDP_REPORT_IDS *report = NULL;
    HID_XFER_PACKET packet;
    BYTE *buffer = NULL;

    switch (code)
    {
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_GET_INPUT_REPORT:
        buffer_len = stack->Parameters.DeviceIoControl.OutputBufferLength;
        buffer = MmGetSystemAddressForMdlSafe( irp->MdlAddress, NormalPagePriority );
        break;
    case IOCTL_HID_SET_FEATURE:
    case IOCTL_HID_SET_OUTPUT_REPORT:
        buffer_len = stack->Parameters.DeviceIoControl.InputBufferLength;
        buffer = irp->AssociatedIrp.SystemBuffer;
        break;
    case IOCTL_HID_WRITE_REPORT:
        buffer_len = stack->Parameters.Write.Length;
        buffer = irp->AssociatedIrp.SystemBuffer;
        break;
    }
    if (!buffer || !buffer_len)
    {
        irp->IoStatus.Status = STATUS_INVALID_USER_BUFFER;
        return;
    }

    switch (code)
    {
    case IOCTL_HID_GET_INPUT_REPORT:
        report = find_report_with_type_and_id( ext, HidP_Input, buffer[0], FALSE );
        if (report) report_len = report->InputLength;
        break;
    case IOCTL_HID_SET_OUTPUT_REPORT:
    case IOCTL_HID_WRITE_REPORT:
        report = find_report_with_type_and_id( ext, HidP_Output, buffer[0], FALSE );
        if (report) report_len = report->OutputLength;
        break;
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_SET_FEATURE:
        report = find_report_with_type_and_id( ext, HidP_Feature, buffer[0], FALSE );
        if (report) report_len = report->FeatureLength;
        break;
    }

    if (!report || buffer_len < report_len)
    {
        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    if (!report->ReportID) offset = 1;
    packet.reportId = report->ReportID;
    packet.reportBuffer = buffer + offset;

    switch (code)
    {
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_GET_INPUT_REPORT:
        packet.reportBufferLen = buffer_len - offset;
        call_minidriver( code, ext->u.pdo.parent_fdo, NULL, 0, &packet, sizeof(packet), &irp->IoStatus );
        break;
    case IOCTL_HID_SET_FEATURE:
    case IOCTL_HID_SET_OUTPUT_REPORT:
    case IOCTL_HID_WRITE_REPORT:
        packet.reportBufferLen = report_len - offset;
        call_minidriver( code, ext->u.pdo.parent_fdo, NULL, sizeof(packet), &packet, 0, &irp->IoStatus );
        if (code == IOCTL_HID_WRITE_REPORT && packet.reportId) irp->IoStatus.Information--;
        break;
    }
}

NTSTATUS WINAPI pdo_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    struct hid_queue *queue = irp->Tail.Overlay.OriginalFileObject->FsContext;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    NTSTATUS status;
    BOOL removed;
    ULONG code;
    KIRQL irql;

    irp->IoStatus.Information = 0;

    TRACE( "device %p code %#lx\n", device, irpsp->Parameters.DeviceIoControl.IoControlCode );

    KeAcquireSpinLock(&ext->u.pdo.lock, &irql);
    removed = ext->u.pdo.removed;
    KeReleaseSpinLock(&ext->u.pdo.lock, irql);

    if (removed)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    switch ((code = irpsp->Parameters.DeviceIoControl.IoControlCode))
    {
        case IOCTL_HID_GET_POLL_FREQUENCY_MSEC:
            TRACE("IOCTL_HID_GET_POLL_FREQUENCY_MSEC\n");
            if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                irp->IoStatus.Information = 0;
                break;
            }
            *(ULONG *)irp->AssociatedIrp.SystemBuffer = ext->u.pdo.poll_interval;
            irp->IoStatus.Information = sizeof(ULONG);
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IOCTL_HID_SET_POLL_FREQUENCY_MSEC:
        {
            ULONG poll_interval;
            TRACE("IOCTL_HID_SET_POLL_FREQUENCY_MSEC\n");
            if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            poll_interval = *(ULONG *)irp->AssociatedIrp.SystemBuffer;
            if (poll_interval) ext->u.pdo.poll_interval = min(poll_interval, MAX_POLL_INTERVAL_MSEC);
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_HID_GET_PRODUCT_STRING:
        {
            handle_minidriver_string( ext, irp, HID_STRING_ID_IPRODUCT );
            break;
        }
        case IOCTL_HID_GET_SERIALNUMBER_STRING:
        {
            handle_minidriver_string( ext, irp, HID_STRING_ID_ISERIALNUMBER );
            break;
        }
        case IOCTL_HID_GET_MANUFACTURER_STRING:
        {
            handle_minidriver_string( ext, irp, HID_STRING_ID_IMANUFACTURER );
            break;
        }
        case IOCTL_HID_GET_COLLECTION_INFORMATION:
        {
            handle_IOCTL_HID_GET_COLLECTION_INFORMATION( irp, ext );
            break;
        }
        case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
        {
            handle_IOCTL_HID_GET_COLLECTION_DESCRIPTOR( irp, ext );
            break;
        }
        case IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS:
        {
            irp->IoStatus.Information = 0;

            if (irpsp->Parameters.DeviceIoControl.InputBufferLength != sizeof(ULONG))
                irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            else
                irp->IoStatus.Status = hid_queue_resize( queue, *(ULONG *)irp->AssociatedIrp.SystemBuffer );
            break;
        }
        case IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS:
        {
            if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                irp->IoStatus.Information = 0;
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                *(ULONG *)irp->AssociatedIrp.SystemBuffer = queue->length;
                irp->IoStatus.Information = sizeof(ULONG);
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            break;
        }
        case IOCTL_HID_GET_FEATURE:
        case IOCTL_HID_SET_FEATURE:
        case IOCTL_HID_GET_INPUT_REPORT:
        case IOCTL_HID_SET_OUTPUT_REPORT:
            hid_device_xfer_report( ext, code, irp );
            break;
        default:
        {
            ULONG code = irpsp->Parameters.DeviceIoControl.IoControlCode;
            FIXME( "Unsupported ioctl %#lx (device=%lx access=%lx func=%lx method=%lx)\n", code,
                   code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3 );
            irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            break;
        }
    }

    status = irp->IoStatus.Status;
    if (status != STATUS_PENDING) IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS WINAPI pdo_read(DEVICE_OBJECT *device, IRP *irp)
{
    struct hid_queue *queue = irp->Tail.Overlay.OriginalFileObject->FsContext;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    HIDP_COLLECTION_DESC *desc = ext->u.pdo.device_desc.CollectionDesc;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    struct hid_report *report;
    BOOL removed;
    KIRQL irql;

    KeAcquireSpinLock(&ext->u.pdo.lock, &irql);
    removed = ext->u.pdo.removed;
    KeReleaseSpinLock(&ext->u.pdo.lock, irql);

    if (removed)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    if (irpsp->Parameters.Read.Length < desc->InputLength)
    {
        irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_INVALID_BUFFER_SIZE;
    }

    irp->IoStatus.Information = 0;
    if ((report = hid_queue_pop_report( queue )))
    {
        memcpy( irp->AssociatedIrp.SystemBuffer, report->buffer, report->length );
        irp->IoStatus.Information = report->length;
        irp->IoStatus.Status = STATUS_SUCCESS;
        hid_report_decref( report );

        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    return hid_queue_push_irp( queue, irp );

}

NTSTATUS WINAPI pdo_write(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    NTSTATUS status;

    hid_device_xfer_report( ext, IOCTL_HID_WRITE_REPORT, irp );

    status = irp->IoStatus.Status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS WINAPI pdo_create(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    struct hid_queue *queue;
    BOOL removed;
    KIRQL irql;

    TRACE("Open handle on device %p\n", device);

    KeAcquireSpinLock( &ext->u.pdo.lock, &irql );
    removed = ext->u.pdo.removed;
    KeReleaseSpinLock( &ext->u.pdo.lock, irql );

    if (removed)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }

    if (!(queue = hid_queue_create())) irp->IoStatus.Status = STATUS_NO_MEMORY;
    else
    {
        KeAcquireSpinLock( &ext->u.pdo.queues_lock, &irql );
        list_add_tail( &ext->u.pdo.queues, &queue->entry );
        KeReleaseSpinLock( &ext->u.pdo.queues_lock, irql );

        irp->Tail.Overlay.OriginalFileObject->FsContext = queue;
        irp->IoStatus.Status = STATUS_SUCCESS;
    }

    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI pdo_close(DEVICE_OBJECT *device, IRP *irp)
{
    struct hid_queue *queue = irp->Tail.Overlay.OriginalFileObject->FsContext;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    BOOL removed;
    KIRQL irql;

    TRACE("Close handle on device %p\n", device);

    KeAcquireSpinLock( &ext->u.pdo.lock, &irql );
    removed = ext->u.pdo.removed;
    KeReleaseSpinLock( &ext->u.pdo.lock, irql );

    if (removed)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }

    if (queue)
    {
        KeAcquireSpinLock( &ext->u.pdo.queues_lock, &irql );
        list_remove( &queue->entry );
        KeReleaseSpinLock( &ext->u.pdo.queues_lock, irql );
        hid_queue_destroy( queue );
    }

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}
