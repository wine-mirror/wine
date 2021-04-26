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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
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
        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.s.ListEntry);
        if (!IoSetCancelRoutine(irp, NULL))
        {
            /* cancel routine is already cleared, meaning that it was called. let it handle completion. */
            InitializeListHead(&irp->Tail.Overlay.s.ListEntry);
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

    RemoveEntryList(&irp->Tail.Overlay.s.ListEntry);

    KeReleaseSpinLock(&ext->u.pdo.irp_queue_lock, old_irql);

    irp->IoStatus.u.Status = STATUS_CANCELLED;
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

static void HID_Device_processQueue(DEVICE_OBJECT *device)
{
    IRP *irp;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    UINT buffer_size = RingBuffer_GetBufferSize(ext->u.pdo.ring_buffer);
    HID_XFER_PACKET *packet;

    packet = HeapAlloc(GetProcessHeap(), 0, buffer_size);

    while((irp = pop_irp_from_queue(ext)))
    {
        int ptr;
        ptr = PtrToUlong( irp->Tail.Overlay.OriginalFileObject->FsContext );

        RingBuffer_Read(ext->u.pdo.ring_buffer, ptr, packet, &buffer_size);
        if (buffer_size)
        {
            NTSTATUS rc;
            ULONG out_length;
            IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
            packet->reportBuffer = (BYTE *)packet + sizeof(*packet);
            TRACE_(hid_report)("Processing Request (%i)\n",ptr);
            rc = copy_packet_into_buffer(packet, irp->AssociatedIrp.SystemBuffer, irpsp->Parameters.Read.Length, &out_length);
            irp->IoStatus.u.Status = rc;
            irp->IoStatus.Information = out_length;
        }
        else
        {
            irp->IoStatus.Information = 0;
            irp->IoStatus.u.Status = STATUS_UNSUCCESSFUL;
        }
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
    HeapFree(GetProcessHeap(), 0, packet);
}

static DWORD CALLBACK hid_device_thread(void *args)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)args;

    IRP *irp;
    IO_STATUS_BLOCK irp_status;
    HID_XFER_PACKET *packet;
    DWORD rc;

    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    USHORT report_size = ext->u.pdo.preparsed_data->caps.InputReportByteLength;

    packet = HeapAlloc(GetProcessHeap(), 0, sizeof(*packet) + report_size);
    packet->reportBuffer = (BYTE *)packet + sizeof(*packet);

    if (ext->u.pdo.information.Polled)
    {
        while(1)
        {
            KEVENT event;

            KeInitializeEvent(&event, NotificationEvent, FALSE);

            packet->reportBufferLen = report_size;
            packet->reportId = 0;

            irp = IoBuildDeviceIoControlRequest(IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo,
                    NULL, 0, packet, sizeof(*packet), TRUE, &event, &irp_status);

            if (IoCallDriver(ext->u.pdo.parent_fdo, irp) == STATUS_PENDING)
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            if (irp_status.u.Status == STATUS_SUCCESS)
            {
                RingBuffer_Write(ext->u.pdo.ring_buffer, packet);
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
            KEVENT event;

            KeInitializeEvent(&event, NotificationEvent, FALSE);

            irp = IoBuildDeviceIoControlRequest(IOCTL_HID_READ_REPORT, ext->u.pdo.parent_fdo,
                    NULL, 0, packet->reportBuffer, report_size, TRUE, &event, &irp_status);

            if (IoCallDriver(ext->u.pdo.parent_fdo, irp) == STATUS_PENDING)
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            rc = WaitForSingleObject(ext->u.pdo.halt_event, 0);
            if (rc == WAIT_OBJECT_0)
                exit_now = TRUE;

            if (!exit_now && irp_status.u.Status == STATUS_SUCCESS)
            {
                packet->reportBufferLen = irp_status.Information;
                if (ext->u.pdo.preparsed_data->reports[0].reportID)
                    packet->reportId = packet->reportBuffer[0];
                else
                    packet->reportId = 0;
                RingBuffer_Write(ext->u.pdo.ring_buffer, packet);
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

static NTSTATUS handle_IOCTL_HID_GET_COLLECTION_INFORMATION(IRP *irp, BASE_DEVICE_EXTENSION *ext)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <  sizeof(HID_COLLECTION_INFORMATION))
    {
        irp->IoStatus.u.Status = STATUS_BUFFER_OVERFLOW;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy(irp->AssociatedIrp.SystemBuffer, &ext->u.pdo.information, sizeof(HID_COLLECTION_INFORMATION));
        irp->IoStatus.Information = sizeof(HID_COLLECTION_INFORMATION);
        irp->IoStatus.u.Status = STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS handle_IOCTL_HID_GET_COLLECTION_DESCRIPTOR(IRP *irp, BASE_DEVICE_EXTENSION *ext)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < data->dwSize)
    {
        irp->IoStatus.u.Status = STATUS_INVALID_BUFFER_SIZE;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy(irp->UserBuffer, data, data->dwSize);
        irp->IoStatus.Information = data->dwSize;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS handle_minidriver_string(BASE_DEVICE_EXTENSION *ext, IRP *irp, SHORT index)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    WCHAR buffer[127];
    NTSTATUS status;
    ULONG InputBuffer;

    InputBuffer = MAKELONG(index, 0);
    status = call_minidriver(IOCTL_HID_GET_STRING, ext->u.pdo.parent_fdo,
            ULongToPtr(InputBuffer), sizeof(InputBuffer), buffer, sizeof(buffer));

    if (status == STATUS_SUCCESS)
    {
        WCHAR *out_buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
        int length = irpsp->Parameters.DeviceIoControl.OutputBufferLength/sizeof(WCHAR);
        TRACE("got string %s from minidriver\n",debugstr_w(buffer));
        lstrcpynW(out_buffer, buffer, length);
        irp->IoStatus.Information = (lstrlenW(buffer)+1) * sizeof(WCHAR);
    }
    irp->IoStatus.u.Status = status;

    return STATUS_SUCCESS;
}

static NTSTATUS HID_get_feature(BASE_DEVICE_EXTENSION *ext, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    HID_XFER_PACKET *packet;
    DWORD len;
    NTSTATUS rc = STATUS_SUCCESS;
    BYTE *out_buffer;

    irp->IoStatus.Information = 0;

    out_buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
    TRACE_(hid_report)("Device %p Buffer length %i Buffer %p\n", ext, irpsp->Parameters.DeviceIoControl.OutputBufferLength, out_buffer);

    len = sizeof(*packet) + irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    packet = HeapAlloc(GetProcessHeap(), 0, len);
    packet->reportBufferLen = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    packet->reportBuffer = ((BYTE*)packet) + sizeof(*packet);
    packet->reportId = out_buffer[0];

    TRACE_(hid_report)("(id %i, len %i buffer %p)\n", packet->reportId, packet->reportBufferLen, packet->reportBuffer);

    rc = call_minidriver(IOCTL_HID_GET_FEATURE, ext->u.pdo.parent_fdo, NULL, 0, packet, sizeof(*packet));

    irp->IoStatus.u.Status = rc;
    if (irp->IoStatus.u.Status == STATUS_SUCCESS)
    {
        irp->IoStatus.Information = packet->reportBufferLen;
        memcpy(out_buffer, packet->reportBuffer, packet->reportBufferLen);
    }
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)("Result 0x%x get %li bytes\n", rc, irp->IoStatus.Information);

    HeapFree(GetProcessHeap(), 0, packet);

    return rc;
}

static NTSTATUS HID_set_to_device(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;
    HID_XFER_PACKET packet;
    ULONG max_len;
    NTSTATUS rc;

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

    rc = call_minidriver(irpsp->Parameters.DeviceIoControl.IoControlCode,
            ext->u.pdo.parent_fdo, NULL, 0, &packet, sizeof(packet));

    irp->IoStatus.u.Status = rc;
    if (irp->IoStatus.u.Status == STATUS_SUCCESS)
        irp->IoStatus.Information = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)("Result 0x%x set %li bytes\n", rc, irp->IoStatus.Information);

    return rc;
}

NTSTATUS WINAPI pdo_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS rc = STATUS_SUCCESS;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    irp->IoStatus.Information = 0;

    TRACE("device %p ioctl(%x)\n", device, irpsp->Parameters.DeviceIoControl.IoControlCode);

    switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_HID_GET_POLL_FREQUENCY_MSEC:
            TRACE("IOCTL_HID_GET_POLL_FREQUENCY_MSEC\n");
            if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                irp->IoStatus.u.Status = STATUS_BUFFER_OVERFLOW;
                irp->IoStatus.Information = 0;
                break;
            }
            *(ULONG *)irp->AssociatedIrp.SystemBuffer = ext->u.pdo.poll_interval;
            irp->IoStatus.Information = sizeof(ULONG);
            irp->IoStatus.u.Status = STATUS_SUCCESS;
            break;
        case IOCTL_HID_SET_POLL_FREQUENCY_MSEC:
        {
            ULONG poll_interval;
            TRACE("IOCTL_HID_SET_POLL_FREQUENCY_MSEC\n");
            if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
            {
                irp->IoStatus.u.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            poll_interval = *(ULONG *)irp->AssociatedIrp.SystemBuffer;
            if (poll_interval <= MAX_POLL_INTERVAL_MSEC)
            {
                ext->u.pdo.poll_interval = poll_interval;
                irp->IoStatus.u.Status = STATUS_SUCCESS;
            }
            else
                irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
            break;
        }
        case IOCTL_HID_GET_PRODUCT_STRING:
        {
            rc = handle_minidriver_string(ext, irp, HID_STRING_ID_IPRODUCT);
            break;
        }
        case IOCTL_HID_GET_SERIALNUMBER_STRING:
        {
            rc = handle_minidriver_string(ext, irp, HID_STRING_ID_ISERIALNUMBER);
            break;
        }
        case IOCTL_HID_GET_MANUFACTURER_STRING:
        {
            rc = handle_minidriver_string(ext, irp, HID_STRING_ID_IMANUFACTURER);
            break;
        }
        case IOCTL_HID_GET_COLLECTION_INFORMATION:
        {
            rc = handle_IOCTL_HID_GET_COLLECTION_INFORMATION(irp, ext);
            break;
        }
        case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
        {
            rc = handle_IOCTL_HID_GET_COLLECTION_DESCRIPTOR(irp, ext);
            break;
        }
        case IOCTL_HID_GET_INPUT_REPORT:
        {
            HID_XFER_PACKET *packet;
            UINT packet_size = sizeof(*packet) + irpsp->Parameters.DeviceIoControl.OutputBufferLength;
            BYTE *buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
            ULONG out_length;

            packet = HeapAlloc(GetProcessHeap(), 0, packet_size);

            if (ext->u.pdo.preparsed_data->reports[0].reportID)
                packet->reportId = buffer[0];
            else
                packet->reportId = 0;
            packet->reportBuffer = (BYTE *)packet + sizeof(*packet);
            packet->reportBufferLen = irpsp->Parameters.DeviceIoControl.OutputBufferLength - 1;

            rc = call_minidriver(IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo, NULL, 0, packet, sizeof(*packet));
            if (rc == STATUS_SUCCESS)
            {
                rc = copy_packet_into_buffer(packet, buffer, irpsp->Parameters.DeviceIoControl.OutputBufferLength, &out_length);
                irp->IoStatus.Information = out_length;
            }
            else
                irp->IoStatus.Information = 0;
            irp->IoStatus.u.Status = rc;
            HeapFree(GetProcessHeap(), 0, packet);
            break;
        }
        case IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS:
        {
            irp->IoStatus.Information = 0;

            if (irpsp->Parameters.DeviceIoControl.InputBufferLength != sizeof(ULONG))
            {
                irp->IoStatus.u.Status = rc = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                rc = RingBuffer_SetSize(ext->u.pdo.ring_buffer, *(ULONG *)irp->AssociatedIrp.SystemBuffer);
                irp->IoStatus.u.Status = rc;
            }
            break;
        }
        case IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS:
        {
            if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                irp->IoStatus.u.Status = rc = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                *(ULONG *)irp->AssociatedIrp.SystemBuffer = RingBuffer_GetSize(ext->u.pdo.ring_buffer);
                rc = irp->IoStatus.u.Status = STATUS_SUCCESS;
            }
            break;
        }
        case IOCTL_HID_GET_FEATURE:
            rc = HID_get_feature(ext, irp);
            break;
        case IOCTL_HID_SET_FEATURE:
        case IOCTL_HID_SET_OUTPUT_REPORT:
            rc = HID_set_to_device(device, irp);
            break;
        default:
        {
            ULONG code = irpsp->Parameters.DeviceIoControl.IoControlCode;
            FIXME("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
            irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
            rc = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    if (rc != STATUS_PENDING)
        IoCompleteRequest( irp, IO_NO_INCREMENT );

    return rc;
}

NTSTATUS WINAPI pdo_read(DEVICE_OBJECT *device, IRP *irp)
{
    HID_XFER_PACKET *packet;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    UINT buffer_size = RingBuffer_GetBufferSize(ext->u.pdo.ring_buffer);
    NTSTATUS rc = STATUS_SUCCESS;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    int ptr = -1;

    packet = HeapAlloc(GetProcessHeap(), 0, buffer_size);
    ptr = PtrToUlong( irp->Tail.Overlay.OriginalFileObject->FsContext );

    irp->IoStatus.Information = 0;
    RingBuffer_ReadNew(ext->u.pdo.ring_buffer, ptr, packet, &buffer_size);

    if (buffer_size)
    {
        IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
        NTSTATUS rc;
        ULONG out_length;
        packet->reportBuffer = (BYTE *)packet + sizeof(*packet);
        TRACE_(hid_report)("Got Packet %p %i\n", packet->reportBuffer, packet->reportBufferLen);

        rc = copy_packet_into_buffer(packet, irp->AssociatedIrp.SystemBuffer, irpsp->Parameters.Read.Length, &out_length);
        irp->IoStatus.Information = out_length;
        irp->IoStatus.u.Status = rc;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
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
                InitializeListHead(&irp->Tail.Overlay.s.ListEntry);
                KeReleaseSpinLock(&ext->u.pdo.irp_queue_lock, old_irql);
                return STATUS_CANCELLED;
            }

            InsertTailList(&ext->u.pdo.irp_queue, &irp->Tail.Overlay.s.ListEntry);
            IoMarkIrpPending(irp);

            KeReleaseSpinLock(&ext->u.pdo.irp_queue_lock, old_irql);
            rc = STATUS_PENDING;
        }
        else
        {
            HID_XFER_PACKET packet;
            TRACE("No packet, but opportunistic reads enabled\n");
            packet.reportId = ((BYTE*)irp->AssociatedIrp.SystemBuffer)[0];
            packet.reportBuffer = &((BYTE*)irp->AssociatedIrp.SystemBuffer)[1];
            packet.reportBufferLen = irpsp->Parameters.Read.Length - 1;
            rc = call_minidriver(IOCTL_HID_GET_INPUT_REPORT, ext->u.pdo.parent_fdo, NULL, 0, &packet, sizeof(packet));

            if (rc == STATUS_SUCCESS)
            {
                ((BYTE*)irp->AssociatedIrp.SystemBuffer)[0] = packet.reportId;
                irp->IoStatus.Information = packet.reportBufferLen + 1;
                irp->IoStatus.u.Status = rc;
            }
            IoCompleteRequest(irp, IO_NO_INCREMENT);
        }
    }
    HeapFree(GetProcessHeap(), 0, packet);

    return rc;
}

NTSTATUS WINAPI pdo_write(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    const WINE_HIDP_PREPARSED_DATA *data = ext->u.pdo.preparsed_data;
    HID_XFER_PACKET packet;
    ULONG max_len;
    NTSTATUS rc;

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

    rc = call_minidriver(IOCTL_HID_WRITE_REPORT, ext->u.pdo.parent_fdo, NULL, 0, &packet, sizeof(packet));

    irp->IoStatus.u.Status = rc;
    if (irp->IoStatus.u.Status == STATUS_SUCCESS)
        irp->IoStatus.Information = irpsp->Parameters.Write.Length;
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)("Result 0x%x wrote %li bytes\n", rc, irp->IoStatus.Information);

    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return rc;
}

NTSTATUS WINAPI pdo_create(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    TRACE("Open handle on device %p\n", device);
    irp->Tail.Overlay.OriginalFileObject->FsContext = UlongToPtr(RingBuffer_AddPointer(ext->u.pdo.ring_buffer));
    irp->IoStatus.u.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI pdo_close(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    int ptr = PtrToUlong(irp->Tail.Overlay.OriginalFileObject->FsContext);
    TRACE("Close handle on device %p\n", device);
    RingBuffer_RemovePointer(ext->u.pdo.ring_buffer, ptr);
    irp->IoStatus.u.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}
