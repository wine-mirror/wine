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

#include "wine/debug.h"
#include "ddk/hidsdi.h"
#include "ddk/hidtypes.h"
#include "ddk/wdm.h"

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

static NTSTATUS copy_packet_into_buffer(HID_XFER_PACKET *packet, BYTE* buffer, ULONG buffer_length, ULONG *out_length)
{
    BOOL zero_id = (packet->reportId == 0);

    *out_length = 0;

    if ((zero_id && buffer_length > packet->reportBufferLen) ||
        (!zero_id && buffer_length >= packet->reportBufferLen))
    {
        if (packet->reportId != 0)
        {
            memcpy(buffer, packet->reportBuffer, packet->reportBufferLen);
            *out_length = packet->reportBufferLen;
        }
        else
        {
            buffer[0] = 0;
            memcpy(&buffer[1], packet->reportBuffer, packet->reportBufferLen);
            *out_length = packet->reportBufferLen + 1;
        }
        return STATUS_SUCCESS;
    }
    else
        return STATUS_BUFFER_OVERFLOW;
}

static void hid_device_send_input(DEVICE_OBJECT *device, HID_XFER_PACKET *packet)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    RAWINPUT *rawinput;
    UCHAR *report, id;
    ULONG data_size;
    INPUT input;

    data_size = offsetof(RAWINPUT, data.hid.bRawData) + packet->reportBufferLen;
    if (!(id = ext->u.pdo.preparsed_data->reports[0].reportID)) data_size += 1;

    if (!(rawinput = malloc(data_size)))
    {
        ERR("Failed to allocate rawinput data!\n");
        return;
    }

    rawinput->header.dwType = RIM_TYPEHID;
    rawinput->header.dwSize = data_size;
    rawinput->header.hDevice = ULongToHandle(ext->u.pdo.rawinput_handle);
    rawinput->header.wParam = RIM_INPUT;
    rawinput->data.hid.dwCount = 1;
    rawinput->data.hid.dwSizeHid = data_size - offsetof(RAWINPUT, data.hid.bRawData);

    report = rawinput->data.hid.bRawData;
    if (!id) *report++ = 0;
    memcpy(report, packet->reportBuffer, packet->reportBufferLen);

    input.type = INPUT_HARDWARE;
    input.hi.uMsg = WM_INPUT;
    input.hi.wParamH = 0;
    input.hi.wParamL = 0;
    __wine_send_input(0, &input, rawinput);

    free(rawinput);
}

static void HID_Device_processQueue(DEVICE_OBJECT *device)
{
    IRP *irp;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    UINT buffer_size = RingBuffer_GetBufferSize(ext->u.pdo.ring_buffer);
    HID_XFER_PACKET *packet;

    packet = malloc(buffer_size);

    while((irp = pop_irp_from_queue(ext)))
    {
        int ptr;
        ptr = PtrToUlong( irp->Tail.Overlay.OriginalFileObject->FsContext );

        RingBuffer_Read(ext->u.pdo.ring_buffer, ptr, packet, &buffer_size);
        if (buffer_size)
        {
            ULONG out_length;
            IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
            packet->reportBuffer = (BYTE *)packet + sizeof(*packet);
            TRACE_(hid_report)("Processing Request (%i)\n",ptr);
            irp->IoStatus.Status = copy_packet_into_buffer( packet, irp->AssociatedIrp.SystemBuffer, irpsp->Parameters.Read.Length, &out_length );
            irp->IoStatus.Information = out_length;
        }
        else
        {
            irp->IoStatus.Information = 0;
            irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
    free(packet);
}

static DWORD CALLBACK hid_device_thread(void *args)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)args;
    HID_XFER_PACKET *packet;
    IO_STATUS_BLOCK io;
    DWORD rc;

    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    USHORT report_size = ext->u.pdo.preparsed_data->caps.InputReportByteLength;

    packet = malloc(sizeof(*packet) + report_size);
    packet->reportBuffer = (BYTE *)packet + sizeof(*packet);

    if (ext->u.pdo.information.Polled)
    {
        while(1)
        {
            packet->reportBufferLen = report_size;
            packet->reportId = 0;

            call_minidriver( IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo, NULL, 0, packet,
                             sizeof(*packet), &io );

            if (io.Status == STATUS_SUCCESS)
            {
                RingBuffer_Write(ext->u.pdo.ring_buffer, packet);
                hid_device_send_input(device, packet);
                HID_Device_processQueue(device);
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
            call_minidriver( IOCTL_HID_READ_REPORT, ext->u.pdo.parent_fdo, NULL, 0,
                             packet->reportBuffer, report_size, &io );

            rc = WaitForSingleObject(ext->u.pdo.halt_event, 0);
            if (rc == WAIT_OBJECT_0)
                exit_now = TRUE;

            if (!exit_now && io.Status == STATUS_SUCCESS)
            {
                packet->reportBufferLen = io.Information;
                if (ext->u.pdo.preparsed_data->reports[0].reportID)
                    packet->reportId = packet->reportBuffer[0];
                else
                    packet->reportId = 0;
                RingBuffer_Write(ext->u.pdo.ring_buffer, packet);
                hid_device_send_input(device, packet);
                HID_Device_processQueue(device);
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
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < data->dwSize)
    {
        irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy(irp->UserBuffer, data, data->dwSize);
        irp->IoStatus.Information = data->dwSize;
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

static void HID_get_feature( BASE_DEVICE_EXTENSION *ext, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    HID_XFER_PACKET *packet;
    DWORD len;
    BYTE *out_buffer;

    irp->IoStatus.Information = 0;

    out_buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
    TRACE_(hid_report)("Device %p Buffer length %i Buffer %p\n", ext, irpsp->Parameters.DeviceIoControl.OutputBufferLength, out_buffer);

    if (!irpsp->Parameters.DeviceIoControl.OutputBufferLength || !out_buffer)
    {
        irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return;
    }

    len = sizeof(*packet) + irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    packet = malloc(len);
    packet->reportBufferLen = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    packet->reportBuffer = ((BYTE*)packet) + sizeof(*packet);
    packet->reportId = out_buffer[0];

    TRACE_(hid_report)("(id %i, len %i buffer %p)\n", packet->reportId, packet->reportBufferLen, packet->reportBuffer);

    call_minidriver( IOCTL_HID_GET_FEATURE, ext->u.pdo.parent_fdo, NULL, 0, packet, sizeof(*packet),
                     &irp->IoStatus );

    if (irp->IoStatus.Status == STATUS_SUCCESS)
    {
        irp->IoStatus.Information = packet->reportBufferLen;
        memcpy(out_buffer, packet->reportBuffer, packet->reportBufferLen);
    }
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)( "Result 0x%x get %li bytes\n", irp->IoStatus.Status, irp->IoStatus.Information );

    free(packet);
}

static void HID_set_to_device( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;
    HID_XFER_PACKET packet;
    ULONG max_len;

    TRACE_(hid_report)("Device %p Buffer length %i Buffer %p\n", device, irpsp->Parameters.DeviceIoControl.InputBufferLength, irp->AssociatedIrp.SystemBuffer);
    packet.reportId = ((BYTE*)irp->AssociatedIrp.SystemBuffer)[0];
    if (packet.reportId == 0)
    {
        packet.reportBuffer = &((BYTE*)irp->AssociatedIrp.SystemBuffer)[1];
        packet.reportBufferLen = irpsp->Parameters.DeviceIoControl.InputBufferLength - 1;
        if (irpsp->Parameters.DeviceIoControl.IoControlCode == IOCTL_HID_SET_FEATURE)
            max_len = data->caps.FeatureReportByteLength;
        else
            max_len = data->caps.OutputReportByteLength;
    }
    else
    {
        packet.reportBuffer = irp->AssociatedIrp.SystemBuffer;
        packet.reportBufferLen = irpsp->Parameters.DeviceIoControl.InputBufferLength;
        if (irpsp->Parameters.DeviceIoControl.IoControlCode == IOCTL_HID_SET_FEATURE)
            max_len = data->reports[data->reportIdx[HidP_Feature][packet.reportId]].bitSize;
        else
            max_len = data->reports[data->reportIdx[HidP_Output][packet.reportId]].bitSize;
        max_len = (max_len + 7) / 8;
    }
    if (packet.reportBufferLen > max_len)
        packet.reportBufferLen = max_len;

    TRACE_(hid_report)("(id %i, len %i buffer %p)\n", packet.reportId, packet.reportBufferLen, packet.reportBuffer);

    call_minidriver( irpsp->Parameters.DeviceIoControl.IoControlCode, ext->u.pdo.parent_fdo, NULL,
                     0, &packet, sizeof(packet), &irp->IoStatus );

    if (irp->IoStatus.Status == STATUS_SUCCESS)
        irp->IoStatus.Information = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)( "Result 0x%x set %li bytes\n", irp->IoStatus.Status, irp->IoStatus.Information );
}

NTSTATUS WINAPI pdo_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;
    NTSTATUS status;
    BOOL removed;
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

    switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
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
        case IOCTL_HID_GET_INPUT_REPORT:
        {
            HID_XFER_PACKET *packet;
            ULONG buffer_len = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
            UINT packet_size = sizeof(*packet) + buffer_len;
            BYTE *buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
            ULONG out_length;

            if (!buffer)
            {
                irp->IoStatus.Status = STATUS_INVALID_USER_BUFFER;
                break;
            }
            if (buffer_len < data->caps.InputReportByteLength)
            {
                irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                break;
            }

            packet = malloc(packet_size);

            if (ext->u.pdo.preparsed_data->reports[0].reportID)
                packet->reportId = buffer[0];
            else
                packet->reportId = 0;
            packet->reportBuffer = (BYTE *)packet + sizeof(*packet);
            packet->reportBufferLen = buffer_len - 1;

            call_minidriver( IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo, NULL, 0, packet,
                             sizeof(*packet), &irp->IoStatus );

            if (irp->IoStatus.Status == STATUS_SUCCESS)
                irp->IoStatus.Status = copy_packet_into_buffer( packet, buffer, buffer_len, &out_length );
            free(packet);
            break;
        }
        case IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS:
        {
            irp->IoStatus.Information = 0;

            if (irpsp->Parameters.DeviceIoControl.InputBufferLength != sizeof(ULONG))
                irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            else
                irp->IoStatus.Status = RingBuffer_SetSize( ext->u.pdo.ring_buffer, *(ULONG *)irp->AssociatedIrp.SystemBuffer );
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
                *(ULONG *)irp->AssociatedIrp.SystemBuffer = RingBuffer_GetSize(ext->u.pdo.ring_buffer);
                irp->IoStatus.Information = sizeof(ULONG);
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            break;
        }
        case IOCTL_HID_GET_FEATURE:
            HID_get_feature( ext, irp );
            break;
        case IOCTL_HID_SET_FEATURE:
        case IOCTL_HID_SET_OUTPUT_REPORT:
            HID_set_to_device( device, irp );
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
    HID_XFER_PACKET *packet;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;
    UINT buffer_size = RingBuffer_GetBufferSize(ext->u.pdo.ring_buffer);
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS status;
    int ptr = -1;
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

    if (irpsp->Parameters.Read.Length < data->caps.InputReportByteLength)
    {
        irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_INVALID_BUFFER_SIZE;
    }

    packet = malloc(buffer_size);
    ptr = PtrToUlong( irp->Tail.Overlay.OriginalFileObject->FsContext );

    irp->IoStatus.Information = 0;
    RingBuffer_ReadNew(ext->u.pdo.ring_buffer, ptr, packet, &buffer_size);

    if (buffer_size)
    {
        IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
        ULONG out_length;
        packet->reportBuffer = (BYTE *)packet + sizeof(*packet);
        TRACE_(hid_report)("Got Packet %p %i\n", packet->reportBuffer, packet->reportBufferLen);

        irp->IoStatus.Status = copy_packet_into_buffer( packet, irp->AssociatedIrp.SystemBuffer, irpsp->Parameters.Read.Length, &out_length );
        irp->IoStatus.Information = out_length;
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
            TRACE("No packet, but opportunistic reads enabled\n");
            packet.reportId = ((BYTE*)irp->AssociatedIrp.SystemBuffer)[0];
            packet.reportBuffer = &((BYTE*)irp->AssociatedIrp.SystemBuffer)[1];
            packet.reportBufferLen = irpsp->Parameters.Read.Length - 1;

            call_minidriver( IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo, NULL, 0, &packet,
                             sizeof(packet), &irp->IoStatus );

            if (irp->IoStatus.Status == STATUS_SUCCESS)
            {
                ((BYTE*)irp->AssociatedIrp.SystemBuffer)[0] = packet.reportId;
                irp->IoStatus.Information = packet.reportBufferLen + 1;
            }
        }
    }
    free(packet);

    status = irp->IoStatus.Status;
    if (status != STATUS_PENDING) IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS WINAPI pdo_write(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;
    HID_XFER_PACKET packet;
    NTSTATUS status;
    ULONG max_len;
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

    if (!irpsp->Parameters.Write.Length)
    {
        irp->IoStatus.Status = STATUS_INVALID_USER_BUFFER;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_INVALID_USER_BUFFER;
    }

    if (irpsp->Parameters.Write.Length < data->caps.OutputReportByteLength)
    {
        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_INVALID_PARAMETER;
    }

    irp->IoStatus.Information = 0;

    TRACE_(hid_report)("Device %p Buffer length %i Buffer %p\n", device, irpsp->Parameters.Write.Length, irp->AssociatedIrp.SystemBuffer);
    packet.reportId = ((BYTE*)irp->AssociatedIrp.SystemBuffer)[0];
    if (packet.reportId == 0)
    {
        packet.reportBuffer = &((BYTE*)irp->AssociatedIrp.SystemBuffer)[1];
        packet.reportBufferLen = irpsp->Parameters.Write.Length - 1;
        max_len = data->caps.OutputReportByteLength;
    }
    else
    {
        packet.reportBuffer = irp->AssociatedIrp.SystemBuffer;
        packet.reportBufferLen = irpsp->Parameters.Write.Length;
        max_len = (data->reports[data->reportIdx[HidP_Output][packet.reportId]].bitSize + 7) / 8;
    }
    if (packet.reportBufferLen > max_len)
        packet.reportBufferLen = max_len;

    TRACE_(hid_report)("(id %i, len %i buffer %p)\n", packet.reportId, packet.reportBufferLen, packet.reportBuffer);

    call_minidriver( IOCTL_HID_WRITE_REPORT, ext->u.pdo.parent_fdo, NULL, 0, &packet,
                     sizeof(packet), &irp->IoStatus );

    if (irp->IoStatus.Status == STATUS_SUCCESS)
        irp->IoStatus.Information = irpsp->Parameters.Write.Length;
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)( "Result 0x%x wrote %li bytes\n", irp->IoStatus.Status, irp->IoStatus.Information );

    status = irp->IoStatus.Status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS WINAPI pdo_create(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    TRACE("Open handle on device %p\n", device);
    irp->Tail.Overlay.OriginalFileObject->FsContext = UlongToPtr(RingBuffer_AddPointer(ext->u.pdo.ring_buffer));
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI pdo_close(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    int ptr = PtrToUlong(irp->Tail.Overlay.OriginalFileObject->FsContext);
    TRACE("Close handle on device %p\n", device);
    RingBuffer_RemovePointer(ext->u.pdo.ring_buffer, ptr);
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}
