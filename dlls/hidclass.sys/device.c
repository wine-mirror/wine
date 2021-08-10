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
WINE_DECLARE_DEBUG_CHANNEL(hid_report);

IRP *pop_irp_from_queue(BASE_DEVICE_EXTENSION *ext)
{
    LIST_ENTRY *entry;
    KIRQL old_irql;
    IRP *irp = NULL;

    KeAcquireSpinLock(&ext->u.pdo.irp_queue_lock, &old_irql);

    while (!irp && (entry = RemoveHeadList(&ext->u.pdo.irp_queue)) != &ext->u.pdo.irp_queue)
    {
        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
        if (!IoSetCancelRoutine(irp, NULL))
        {
            /* cancel routine is already cleared, meaning that it was called. let it handle completion. */
            InitializeListHead(&irp->Tail.Overlay.ListEntry);
            irp = NULL;
        }
    }

    KeReleaseSpinLock(&ext->u.pdo.irp_queue_lock, old_irql);
    return irp;
}

static void WINAPI read_cancel_routine(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext;
    KIRQL old_irql;

    TRACE("cancel %p IRP on device %p\n", irp, device);

    ext = device->DeviceExtension;

    IoReleaseCancelSpinLock(irp->CancelIrql);

    KeAcquireSpinLock(&ext->u.pdo.irp_queue_lock, &old_irql);

    RemoveEntryList(&irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&ext->u.pdo.irp_queue_lock, old_irql);

    irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

static struct hid_report *hid_report_create( HID_XFER_PACKET *packet )
{
    struct hid_report *report;

    if (!(report = malloc( offsetof( struct hid_report, buffer[packet->reportBufferLen] ) )))
        return NULL;
    report->ref = 1;
    report->length = packet->reportBufferLen;
    memcpy( report->buffer, packet->reportBuffer, report->length );

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

static struct hid_report_queue *hid_report_queue_create( void )
{
    struct hid_report_queue *queue;

    if (!(queue = calloc( 1, sizeof(struct hid_report_queue) ))) return NULL;
    KeInitializeSpinLock( &queue->lock );
    list_init( &queue->entry );
    queue->length = 32;
    queue->read_idx = 0;
    queue->write_idx = 0;

    return queue;
}

static void hid_report_queue_destroy( struct hid_report_queue *queue )
{
    while (queue->length--) hid_report_decref( queue->reports[queue->length] );
    free( queue );
}

static NTSTATUS hid_report_queue_resize( struct hid_report_queue *queue, ULONG length )
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

static void hid_report_queue_push( struct hid_report_queue *queue, struct hid_report *report )
{
    ULONG i = queue->write_idx, next = i + 1;
    struct hid_report *prev;
    KIRQL irql;

    if (next >= queue->length) next = 0;
    hid_report_incref( report );

    KeAcquireSpinLock( &queue->lock, &irql );
    prev = queue->reports[i];
    queue->reports[i] = report;
    if (next != queue->read_idx) queue->write_idx = next;
    KeReleaseSpinLock( &queue->lock, irql );

    hid_report_decref( prev );
}

static struct hid_report *hid_report_queue_pop( struct hid_report_queue *queue )
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
    struct hid_preparsed_data *preparsed = ext->u.pdo.preparsed_data;
    struct hid_report *last_report, *report;
    struct hid_report_queue *queue;
    RAWINPUT *rawinput;
    ULONG size;
    KIRQL irql;
    IRP *irp;

    size = offsetof( RAWINPUT, data.hid.bRawData[packet->reportBufferLen] );
    if (!(rawinput = malloc( size ))) ERR( "Failed to allocate rawinput data!\n" );
    else
    {
        INPUT input;

        rawinput->header.dwType = RIM_TYPEHID;
        rawinput->header.dwSize = size;
        rawinput->header.hDevice = ULongToHandle( ext->u.pdo.rawinput_handle );
        rawinput->header.wParam = RIM_INPUT;
        rawinput->data.hid.dwCount = 1;
        rawinput->data.hid.dwSizeHid = packet->reportBufferLen;
        memcpy( rawinput->data.hid.bRawData, packet->reportBuffer, packet->reportBufferLen );

        input.type = INPUT_HARDWARE;
        input.hi.uMsg = WM_INPUT;
        input.hi.wParamH = 0;
        input.hi.wParamL = 0;
        __wine_send_input( 0, &input, rawinput );

        free( rawinput );
    }

    if (!(last_report = hid_report_create( packet )))
    {
        ERR( "Failed to allocate hid_report!\n" );
        return;
    }

    KeAcquireSpinLock( &ext->u.pdo.report_queues_lock, &irql );
    LIST_FOR_EACH_ENTRY( queue, &ext->u.pdo.report_queues, struct hid_report_queue, entry )
    hid_report_queue_push( queue, last_report );
    KeReleaseSpinLock( &ext->u.pdo.report_queues_lock, irql );

    while ((irp = pop_irp_from_queue( ext )))
    {
        queue = irp->Tail.Overlay.OriginalFileObject->FsContext;

        if (!(report = hid_report_queue_pop( queue ))) hid_report_incref( (report = last_report) );
        memcpy( irp->AssociatedIrp.SystemBuffer, report->buffer, preparsed->caps.InputReportByteLength );
        irp->IoStatus.Information = report->length;
        irp->IoStatus.Status = STATUS_SUCCESS;
        hid_report_decref( report );

        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }

    hid_report_decref( last_report );
}

static DWORD CALLBACK hid_device_thread(void *args)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)args;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    struct hid_preparsed_data *preparsed = ext->u.pdo.preparsed_data;
    BYTE report_id = HID_INPUT_VALUE_CAPS( preparsed )->report_id;
    ULONG buffer_len = preparsed->caps.InputReportByteLength;
    IO_STATUS_BLOCK io;
    HID_XFER_PACKET *packet;
    BYTE *buffer;
    DWORD rc;

    packet = malloc( sizeof(*packet) + buffer_len );
    buffer = (BYTE *)(packet + 1);
    packet->reportBuffer = buffer;

    if (ext->u.pdo.information.Polled)
    {
        while(1)
        {
            packet->reportId = buffer[0] = report_id;
            packet->reportBufferLen = buffer_len;

            if (!report_id)
            {
                packet->reportBuffer++;
                packet->reportBufferLen--;
            }

            call_minidriver( IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo, NULL, 0, packet,
                             sizeof(*packet), &io );

            if (io.Status == STATUS_SUCCESS)
            {
                if (!report_id) io.Information++;
                packet->reportId = buffer[0];
                packet->reportBuffer = buffer;
                packet->reportBufferLen = io.Information;

                hid_device_queue_input( device, packet );
            }

            rc = WaitForSingleObject(ext->u.pdo.halt_event,
                    ext->u.pdo.poll_interval ? ext->u.pdo.poll_interval : DEFAULT_POLL_INTERVAL);

            if (rc == WAIT_OBJECT_0)
                break;
            else if (rc != WAIT_TIMEOUT)
                ERR("Wait returned unexpected value %x\n",rc);
        }
    }
    else
    {
        INT exit_now = FALSE;

        while(1)
        {
            packet->reportId = buffer[0] = report_id;
            packet->reportBufferLen = buffer_len;

            if (!report_id)
            {
                packet->reportBuffer++;
                packet->reportBufferLen--;
            }

            call_minidriver( IOCTL_HID_READ_REPORT, ext->u.pdo.parent_fdo, NULL, 0,
                             packet->reportBuffer, packet->reportBufferLen, &io );

            rc = WaitForSingleObject(ext->u.pdo.halt_event, 0);
            if (rc == WAIT_OBJECT_0)
                exit_now = TRUE;

            if (!exit_now && io.Status == STATUS_SUCCESS)
            {
                if (!report_id) io.Information++;
                packet->reportId = buffer[0];
                packet->reportBuffer = buffer;
                packet->reportBufferLen = io.Information;

                hid_device_queue_input( device, packet );
            }

            if (exit_now)
                break;
        }
    }

    TRACE("Device thread exiting\n");
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
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    struct hid_preparsed_data *preparsed = ext->u.pdo.preparsed_data;

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < preparsed->size)
    {
        irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy( irp->UserBuffer, preparsed, preparsed->size );
        irp->IoStatus.Information = preparsed->size;
        irp->IoStatus.Status = STATUS_SUCCESS;
    }
}

static void handle_minidriver_string( BASE_DEVICE_EXTENSION *ext, IRP *irp, SHORT index )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    WCHAR buffer[127];
    ULONG InputBuffer;

    InputBuffer = MAKELONG(index, 0);

    call_minidriver( IOCTL_HID_GET_STRING, ext->u.pdo.parent_fdo, ULongToPtr( InputBuffer ),
                     sizeof(InputBuffer), buffer, sizeof(buffer), &irp->IoStatus );

    if (irp->IoStatus.Status == STATUS_SUCCESS)
    {
        WCHAR *out_buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
        int length = irpsp->Parameters.DeviceIoControl.OutputBufferLength/sizeof(WCHAR);
        TRACE("got string %s from minidriver\n",debugstr_w(buffer));
        lstrcpynW(out_buffer, buffer, length);
        irp->IoStatus.Information = (lstrlenW(buffer)+1) * sizeof(WCHAR);
    }
}

static void hid_device_xfer_report( BASE_DEVICE_EXTENSION *ext, ULONG code, IRP *irp )
{
    struct hid_preparsed_data *preparsed = ext->u.pdo.preparsed_data;
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    struct hid_value_caps *caps = NULL, *caps_end = NULL;
    ULONG report_len = 0, buffer_len = 0;
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

    switch (code)
    {
    case IOCTL_HID_GET_INPUT_REPORT:
        report_len = preparsed->caps.InputReportByteLength;
        caps = HID_INPUT_VALUE_CAPS( preparsed );
        caps_end = caps + preparsed->value_caps_count[HidP_Input];
        break;
    case IOCTL_HID_SET_OUTPUT_REPORT:
    case IOCTL_HID_WRITE_REPORT:
        report_len = preparsed->caps.OutputReportByteLength;
        caps = HID_OUTPUT_VALUE_CAPS( preparsed );
        caps_end = caps + preparsed->value_caps_count[HidP_Output];
        break;
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_SET_FEATURE:
        report_len = preparsed->caps.FeatureReportByteLength;
        caps = HID_FEATURE_VALUE_CAPS( preparsed );
        caps_end = caps + preparsed->value_caps_count[HidP_Feature];
        break;
    }

    if (!buffer || !buffer_len)
    {
        irp->IoStatus.Status = STATUS_INVALID_USER_BUFFER;
        return;
    }
    if (buffer_len < report_len)
    {
        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    for (; caps != caps_end; ++caps) if (!caps->report_id || caps->report_id == buffer[0]) break;
    if (caps == caps_end)
    {
        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    packet.reportId = buffer[0];
    packet.reportBuffer = buffer;
    packet.reportBufferLen = buffer_len;

    if (!caps->report_id)
    {
        packet.reportId = 0;
        packet.reportBuffer++;
        packet.reportBufferLen--;
    }

    switch (code)
    {
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_GET_INPUT_REPORT:
        call_minidriver( code, ext->u.pdo.parent_fdo, NULL, 0, &packet, sizeof(packet), &irp->IoStatus );
        break;
    case IOCTL_HID_SET_FEATURE:
    case IOCTL_HID_SET_OUTPUT_REPORT:
    case IOCTL_HID_WRITE_REPORT:
        call_minidriver( code, ext->u.pdo.parent_fdo, NULL, sizeof(packet), &packet, 0, &irp->IoStatus );
        if (code == IOCTL_HID_WRITE_REPORT && packet.reportId) irp->IoStatus.Information--;
        break;
    }
}

NTSTATUS WINAPI pdo_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    struct hid_report_queue *queue = irp->Tail.Overlay.OriginalFileObject->FsContext;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    NTSTATUS status;
    BOOL removed;
    ULONG code;
    KIRQL irql;

    irp->IoStatus.Information = 0;

    TRACE("device %p ioctl(%x)\n", device, irpsp->Parameters.DeviceIoControl.IoControlCode);

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
            if (poll_interval <= MAX_POLL_INTERVAL_MSEC)
            {
                ext->u.pdo.poll_interval = poll_interval;
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            else
                irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
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
                irp->IoStatus.Status = hid_report_queue_resize( queue, *(ULONG *)irp->AssociatedIrp.SystemBuffer );
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
            FIXME("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
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
    struct hid_report_queue *queue = irp->Tail.Overlay.OriginalFileObject->FsContext;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    struct hid_preparsed_data *preparsed = ext->u.pdo.preparsed_data;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    BYTE report_id = HID_INPUT_VALUE_CAPS( preparsed )->report_id;
    struct hid_report *report;
    NTSTATUS status;
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

    if (irpsp->Parameters.Read.Length < preparsed->caps.InputReportByteLength)
    {
        irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_INVALID_BUFFER_SIZE;
    }

    irp->IoStatus.Information = 0;
    if ((report = hid_report_queue_pop( queue )))
    {
        memcpy( irp->AssociatedIrp.SystemBuffer, report->buffer, preparsed->caps.InputReportByteLength );
        irp->IoStatus.Information = report->length;
        irp->IoStatus.Status = STATUS_SUCCESS;
        hid_report_decref( report );
    }
    else
    {
        if (ext->u.pdo.poll_interval)
        {
            KIRQL old_irql;
            TRACE_(hid_report)("Queue irp\n");

            KeAcquireSpinLock(&ext->u.pdo.irp_queue_lock, &old_irql);

            IoSetCancelRoutine(irp, read_cancel_routine);
            if (irp->Cancel && !IoSetCancelRoutine(irp, NULL))
            {
                /* IRP was canceled before we set cancel routine */
                InitializeListHead(&irp->Tail.Overlay.ListEntry);
                KeReleaseSpinLock(&ext->u.pdo.irp_queue_lock, old_irql);
                return STATUS_CANCELLED;
            }

            InsertTailList(&ext->u.pdo.irp_queue, &irp->Tail.Overlay.ListEntry);
            irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(irp);

            KeReleaseSpinLock(&ext->u.pdo.irp_queue_lock, old_irql);
        }
        else
        {
            HID_XFER_PACKET packet;
            BYTE *buffer = irp->AssociatedIrp.SystemBuffer;
            ULONG buffer_len = irpsp->Parameters.Read.Length;

            TRACE("No packet, but opportunistic reads enabled\n");

            packet.reportId = buffer[0];
            packet.reportBuffer = buffer;
            packet.reportBufferLen = buffer_len;

            if (!report_id)
            {
                packet.reportId = 0;
                packet.reportBuffer++;
                packet.reportBufferLen--;
            }

            call_minidriver( IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo, NULL, 0, &packet,
                             sizeof(packet), &irp->IoStatus );
        }
    }

    status = irp->IoStatus.Status;
    if (status != STATUS_PENDING) IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
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
    struct hid_report_queue *queue;
    KIRQL irql;

    TRACE("Open handle on device %p\n", device);

    if (!(queue = hid_report_queue_create())) irp->IoStatus.Status = STATUS_NO_MEMORY;
    else
    {
        KeAcquireSpinLock( &ext->u.pdo.report_queues_lock, &irql );
        list_add_tail( &ext->u.pdo.report_queues, &queue->entry );
        KeReleaseSpinLock( &ext->u.pdo.report_queues_lock, irql );

        irp->Tail.Overlay.OriginalFileObject->FsContext = queue;
        irp->IoStatus.Status = STATUS_SUCCESS;
    }

    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI pdo_close(DEVICE_OBJECT *device, IRP *irp)
{
    struct hid_report_queue *queue = irp->Tail.Overlay.OriginalFileObject->FsContext;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    KIRQL irql;

    TRACE("Close handle on device %p\n", device);

    if (queue)
    {
        KeAcquireSpinLock( &ext->u.pdo.report_queues_lock, &irql );
        list_remove( &queue->entry );
        KeReleaseSpinLock( &ext->u.pdo.report_queues_lock, irql );
        hid_report_queue_destroy( queue );
    }

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}
