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
#include "setupapi.h"

#include "wine/debug.h"
#include "ddk/hidsdi.h"
#include "ddk/hidtypes.h"
#include "ddk/wdm.h"

#include "initguid.h"
#include "devguid.h"
#include "ntddmou.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);
WINE_DECLARE_DEBUG_CHANNEL(hid_report);

NTSTATUS HID_CreateDevice(DEVICE_OBJECT *native_device, HID_MINIDRIVER_REGISTRATION *driver, DEVICE_OBJECT **device)
{
    WCHAR dev_name[255];
    UNICODE_STRING nameW;
    NTSTATUS status;
    BASE_DEVICE_EXTENSION *ext;

    swprintf(dev_name, ARRAY_SIZE(dev_name), L"\\Device\\HID#%p&%p", driver->DriverObject, native_device);
    RtlInitUnicodeString( &nameW, dev_name );

    TRACE("Create base hid device %s\n", debugstr_w(dev_name));

    status = IoCreateDevice(driver->DriverObject, driver->DeviceExtensionSize + sizeof(BASE_DEVICE_EXTENSION), &nameW, 0, 0, FALSE, device);
    if (status)
    {
        FIXME( "failed to create device error %x\n", status );
        return status;
    }

    ext = (*device)->DeviceExtension;

    ext->deviceExtension.MiniDeviceExtension = ext + 1;
    ext->deviceExtension.PhysicalDeviceObject = *device;
    ext->deviceExtension.NextDeviceObject = native_device;
    ext->device_name = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(dev_name) + 1) * sizeof(WCHAR));
    lstrcpyW(ext->device_name, dev_name);
    ext->link_name.Buffer = NULL;

    IoAttachDeviceToDeviceStack(*device, native_device);

    return STATUS_SUCCESS;
}

NTSTATUS HID_LinkDevice(DEVICE_OBJECT *device)
{
    WCHAR device_instance_id[MAX_DEVICE_ID_LEN];
    SP_DEVINFO_DATA Data;
    UNICODE_STRING nameW;
    NTSTATUS status;
    HDEVINFO devinfo;
    GUID hidGuid;
    BASE_DEVICE_EXTENSION *ext;

    HidD_GetHidGuid(&hidGuid);
    ext = device->DeviceExtension;

    RtlInitUnicodeString( &nameW, ext->device_name);

    lstrcpyW(device_instance_id, ext->device_id);
    lstrcatW(device_instance_id, L"\\");
    lstrcatW(device_instance_id, ext->instance_id);

    devinfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_HIDCLASS, NULL);
    if (devinfo == INVALID_HANDLE_VALUE)
    {
        FIXME( "failed to get ClassDevs %x\n", GetLastError());
        return STATUS_UNSUCCESSFUL;
    }
    Data.cbSize = sizeof(Data);
    if (SetupDiCreateDeviceInfoW(devinfo, device_instance_id, &GUID_DEVCLASS_HIDCLASS, NULL, NULL, DICD_INHERIT_CLASSDRVS, &Data))
    {
        if (!SetupDiRegisterDeviceInfo(devinfo, &Data, 0, NULL, NULL, NULL))
        {
            FIXME( "failed to register device info %x\n", GetLastError());
            goto error;
        }
    }
    else if (GetLastError() != ERROR_DEVINST_ALREADY_EXISTS)
    {
        FIXME( "failed to create device info %x\n", GetLastError());
        goto error;
    }
    SetupDiDestroyDeviceInfoList(devinfo);

    status = IoRegisterDeviceInterface(device, &hidGuid, NULL, &ext->link_name);
    if (status != STATUS_SUCCESS)
    {
        FIXME( "failed to register device interface %x\n", status );
        return status;
    }

    /* FIXME: This should probably be done in mouhid.sys. */
    if (ext->preparseData->caps.UsagePage == HID_USAGE_PAGE_GENERIC
            && ext->preparseData->caps.Usage == HID_USAGE_GENERIC_MOUSE)
    {
        if (!IoRegisterDeviceInterface(device, &GUID_DEVINTERFACE_MOUSE, NULL, &ext->mouse_link_name))
            ext->is_mouse = TRUE;
    }

    return STATUS_SUCCESS;

error:
    SetupDiDestroyDeviceInfoList(devinfo);
    return STATUS_UNSUCCESSFUL;
}

static IRP *pop_irp_from_queue(BASE_DEVICE_EXTENSION *ext)
{
    LIST_ENTRY *entry;
    KIRQL old_irql;
    IRP *irp = NULL;

    KeAcquireSpinLock(&ext->irp_queue_lock, &old_irql);

    while (!irp && (entry = RemoveHeadList(&ext->irp_queue)) != &ext->irp_queue)
    {
        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.s.ListEntry);
        if (!IoSetCancelRoutine(irp, NULL))
        {
            /* cancel routine is already cleared, meaning that it was called. let it handle completion. */
            InitializeListHead(&irp->Tail.Overlay.s.ListEntry);
            irp = NULL;
        }
    }

    KeReleaseSpinLock(&ext->irp_queue_lock, old_irql);
    return irp;
}

static void WINAPI read_cancel_routine(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext;
    KIRQL old_irql;

    TRACE("cancel %p IRP on device %p\n", irp, device);

    ext = device->DeviceExtension;

    IoReleaseCancelSpinLock(irp->CancelIrql);

    KeAcquireSpinLock(&ext->irp_queue_lock, &old_irql);

    RemoveEntryList(&irp->Tail.Overlay.s.ListEntry);

    KeReleaseSpinLock(&ext->irp_queue_lock, old_irql);

    irp->IoStatus.u.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

void HID_DeleteDevice(DEVICE_OBJECT *device)
{
    BASE_DEVICE_EXTENSION *ext;
    IRP *irp;

    ext = device->DeviceExtension;

    if (ext->thread)
    {
        SetEvent(ext->halt_event);
        WaitForSingleObject(ext->thread, INFINITE);
    }
    CloseHandle(ext->halt_event);

    HeapFree(GetProcessHeap(), 0, ext->preparseData);
    if (ext->ring_buffer)
        RingBuffer_Destroy(ext->ring_buffer);

    while((irp = pop_irp_from_queue(ext)))
    {
        irp->IoStatus.u.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    TRACE("Delete device(%p) %s\n", device, debugstr_w(ext->device_name));
    HeapFree(GetProcessHeap(), 0, ext->device_name);
    RtlFreeUnicodeString(&ext->link_name);

    IoDetachDevice(ext->deviceExtension.NextDeviceObject);
    IoDeleteDevice(device);
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
    UINT buffer_size = RingBuffer_GetBufferSize(ext->ring_buffer);
    HID_XFER_PACKET *packet;

    packet = HeapAlloc(GetProcessHeap(), 0, buffer_size);

    while((irp = pop_irp_from_queue(ext)))
    {
        int ptr;
        ptr = PtrToUlong( irp->Tail.Overlay.OriginalFileObject->FsContext );

        RingBuffer_Read(ext->ring_buffer, ptr, packet, &buffer_size);
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

static NTSTATUS WINAPI read_Completion(DEVICE_OBJECT *deviceObject, IRP *irp, void *context)
{
    HANDLE event = context;
    SetEvent(event);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static DWORD CALLBACK hid_device_thread(void *args)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)args;

    IRP *irp;
    IO_STATUS_BLOCK irp_status;
    HID_XFER_PACKET *packet;
    DWORD rc;
    HANDLE events[2];
    NTSTATUS ntrc;

    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    events[0] = CreateEventA(NULL, TRUE, FALSE, NULL);
    events[1] = ext->halt_event;

    packet = HeapAlloc(GetProcessHeap(), 0, sizeof(*packet) + ext->preparseData->caps.InputReportByteLength);
    packet->reportBuffer = (BYTE *)packet + sizeof(*packet);

    if (ext->information.Polled)
    {
        while(1)
        {
            ResetEvent(events[0]);

            packet->reportBufferLen = ext->preparseData->caps.InputReportByteLength;
            packet->reportId = 0;

            irp = IoBuildDeviceIoControlRequest(IOCTL_HID_GET_INPUT_REPORT,
                device, NULL, 0, packet, sizeof(*packet), TRUE, NULL,
                &irp_status);

            IoSetCompletionRoutine(irp, read_Completion, events[0], TRUE, TRUE, TRUE);
            ntrc = IoCallDriver(device, irp);

            if (ntrc == STATUS_PENDING)
                WaitForMultipleObjects(2, events, FALSE, INFINITE);

            if (irp->IoStatus.u.Status == STATUS_SUCCESS)
            {
                RingBuffer_Write(ext->ring_buffer, packet);
                HID_Device_processQueue(device);
            }

            IoCompleteRequest(irp, IO_NO_INCREMENT );

            rc = WaitForSingleObject(ext->halt_event, ext->poll_interval ? ext->poll_interval : DEFAULT_POLL_INTERVAL);

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
            ResetEvent(events[0]);

            irp = IoBuildDeviceIoControlRequest(IOCTL_HID_READ_REPORT,
                device, NULL, 0, packet->reportBuffer,
                ext->preparseData->caps.InputReportByteLength, TRUE, NULL,
                &irp_status);

            IoSetCompletionRoutine(irp, read_Completion, events[0], TRUE, TRUE, TRUE);
            ntrc = IoCallDriver(device, irp);

            if (ntrc == STATUS_PENDING)
            {
                WaitForMultipleObjects(2, events, FALSE, INFINITE);
            }

            rc = WaitForSingleObject(ext->halt_event, 0);
            if (rc == WAIT_OBJECT_0)
                exit_now = TRUE;

            if (!exit_now && irp->IoStatus.u.Status == STATUS_SUCCESS)
            {
                packet->reportBufferLen = irp->IoStatus.Information;
                if (ext->preparseData->reports[0].reportID)
                    packet->reportId = packet->reportBuffer[0];
                else
                    packet->reportId = 0;
                RingBuffer_Write(ext->ring_buffer, packet);
                HID_Device_processQueue(device);
            }

            IoCompleteRequest(irp, IO_NO_INCREMENT );

            if (exit_now)
                break;
        }
    }

    /* FIXME: releasing packet requires IRP cancellation support */
    CloseHandle(events[0]);

    TRACE("Device thread exiting\n");
    return 1;
}

void HID_StartDeviceThread(DEVICE_OBJECT *device)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    ext->halt_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ext->thread = CreateThread(NULL, 0, hid_device_thread, device, 0, NULL);
}

static NTSTATUS handle_IOCTL_HID_GET_COLLECTION_INFORMATION(IRP *irp, BASE_DEVICE_EXTENSION *base)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <  sizeof(HID_COLLECTION_INFORMATION))
    {
        irp->IoStatus.u.Status = STATUS_BUFFER_OVERFLOW;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy(irp->AssociatedIrp.SystemBuffer, &base->information, sizeof(HID_COLLECTION_INFORMATION));
        irp->IoStatus.Information = sizeof(HID_COLLECTION_INFORMATION);
        irp->IoStatus.u.Status = STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS handle_IOCTL_HID_GET_COLLECTION_DESCRIPTOR(IRP *irp, BASE_DEVICE_EXTENSION *base)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <  base->preparseData->dwSize)
    {
        irp->IoStatus.u.Status = STATUS_INVALID_BUFFER_SIZE;
        irp->IoStatus.Information = 0;
    }
    else
    {
        memcpy(irp->UserBuffer, base->preparseData, base->preparseData->dwSize);
        irp->IoStatus.Information = base->preparseData->dwSize;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS handle_minidriver_string(DEVICE_OBJECT *device, IRP *irp, SHORT index)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    WCHAR buffer[127];
    NTSTATUS status;
    ULONG InputBuffer;

    InputBuffer = MAKELONG(index, 0);
    status = call_minidriver(IOCTL_HID_GET_STRING, device, ULongToPtr(InputBuffer), sizeof(InputBuffer), buffer, sizeof(buffer));

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

static NTSTATUS HID_get_feature(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    HID_XFER_PACKET *packet;
    DWORD len;
    NTSTATUS rc = STATUS_SUCCESS;
    BYTE *out_buffer;

    irp->IoStatus.Information = 0;

    out_buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
    TRACE_(hid_report)("Device %p Buffer length %i Buffer %p\n", device, irpsp->Parameters.DeviceIoControl.OutputBufferLength, out_buffer);

    len = sizeof(*packet) + irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    packet = HeapAlloc(GetProcessHeap(), 0, len);
    packet->reportBufferLen = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    packet->reportBuffer = ((BYTE*)packet) + sizeof(*packet);
    packet->reportId = out_buffer[0];

    TRACE_(hid_report)("(id %i, len %i buffer %p)\n", packet->reportId, packet->reportBufferLen, packet->reportBuffer);

    rc = call_minidriver(IOCTL_HID_GET_FEATURE, device, NULL, 0, packet, sizeof(*packet));

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
            max_len = ext->preparseData->caps.FeatureReportByteLength;
        else
            max_len = ext->preparseData->caps.OutputReportByteLength;
    }
    else
    {
        packet.reportBuffer = irp->AssociatedIrp.SystemBuffer;
        packet.reportBufferLen = irpsp->Parameters.DeviceIoControl.InputBufferLength;
        if (irpsp->Parameters.DeviceIoControl.IoControlCode == IOCTL_HID_SET_FEATURE)
            max_len = (ext->preparseData->reports[ext->preparseData->reportIdx[HidP_Feature][packet.reportId]].bitSize + 7) / 8;
        else
            max_len = (ext->preparseData->reports[ext->preparseData->reportIdx[HidP_Output][packet.reportId]].bitSize + 7) / 8;
    }
    if (packet.reportBufferLen > max_len)
        packet.reportBufferLen = max_len;

    TRACE_(hid_report)("(id %i, len %i buffer %p)\n", packet.reportId, packet.reportBufferLen, packet.reportBuffer);

    rc = call_minidriver(irpsp->Parameters.DeviceIoControl.IoControlCode,
        device, NULL, 0, &packet, sizeof(packet));

    irp->IoStatus.u.Status = rc;
    if (irp->IoStatus.u.Status == STATUS_SUCCESS)
        irp->IoStatus.Information = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)("Result 0x%x set %li bytes\n", rc, irp->IoStatus.Information);

    return rc;
}

NTSTATUS WINAPI HID_Device_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS rc = STATUS_SUCCESS;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *extension = device->DeviceExtension;

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
            *((ULONG*)irp->AssociatedIrp.SystemBuffer) = extension->poll_interval;
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
                extension->poll_interval = poll_interval;
                irp->IoStatus.u.Status = STATUS_SUCCESS;
            }
            else
                irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
            break;
        }
        case IOCTL_HID_GET_PRODUCT_STRING:
        {
            rc = handle_minidriver_string(device, irp, HID_STRING_ID_IPRODUCT);
            break;
        }
        case IOCTL_HID_GET_SERIALNUMBER_STRING:
        {
            rc = handle_minidriver_string(device, irp, HID_STRING_ID_ISERIALNUMBER);
            break;
        }
        case IOCTL_HID_GET_MANUFACTURER_STRING:
        {
            rc = handle_minidriver_string(device, irp, HID_STRING_ID_IMANUFACTURER);
            break;
        }
        case IOCTL_HID_GET_COLLECTION_INFORMATION:
        {
            rc = handle_IOCTL_HID_GET_COLLECTION_INFORMATION(irp, extension);
            break;
        }
        case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
        {
            rc = handle_IOCTL_HID_GET_COLLECTION_DESCRIPTOR(irp, extension);
            break;
        }
        case IOCTL_HID_GET_INPUT_REPORT:
        {
            HID_XFER_PACKET *packet;
            UINT packet_size = sizeof(*packet) + irpsp->Parameters.DeviceIoControl.OutputBufferLength;
            BYTE *buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
            ULONG out_length;

            packet = HeapAlloc(GetProcessHeap(), 0, packet_size);

            if (extension->preparseData->reports[0].reportID)
                packet->reportId = buffer[0];
            else
                packet->reportId = 0;
            packet->reportBuffer = (BYTE *)packet + sizeof(*packet);
            packet->reportBufferLen = irpsp->Parameters.DeviceIoControl.OutputBufferLength - 1;

            rc = call_minidriver(IOCTL_HID_GET_INPUT_REPORT, device, NULL, 0, packet, sizeof(*packet));
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
                rc = RingBuffer_SetSize(extension->ring_buffer, *(ULONG*)irp->AssociatedIrp.SystemBuffer);
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
                *(ULONG*)irp->AssociatedIrp.SystemBuffer = RingBuffer_GetSize(extension->ring_buffer);
                rc = irp->IoStatus.u.Status = STATUS_SUCCESS;
            }
            break;
        }
        case IOCTL_HID_GET_FEATURE:
            rc = HID_get_feature(device, irp);
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

NTSTATUS WINAPI HID_Device_read(DEVICE_OBJECT *device, IRP *irp)
{
    HID_XFER_PACKET *packet;
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    UINT buffer_size = RingBuffer_GetBufferSize(ext->ring_buffer);
    NTSTATUS rc = STATUS_SUCCESS;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    int ptr = -1;

    packet = HeapAlloc(GetProcessHeap(), 0, buffer_size);
    ptr = PtrToUlong( irp->Tail.Overlay.OriginalFileObject->FsContext );

    irp->IoStatus.Information = 0;
    RingBuffer_ReadNew(ext->ring_buffer, ptr, packet, &buffer_size);

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
        BASE_DEVICE_EXTENSION *extension = device->DeviceExtension;
        if (extension->poll_interval)
        {
            KIRQL old_irql;
            TRACE_(hid_report)("Queue irp\n");

            KeAcquireSpinLock(&ext->irp_queue_lock, &old_irql);

            IoSetCancelRoutine(irp, read_cancel_routine);
            if (irp->Cancel && !IoSetCancelRoutine(irp, NULL))
            {
                /* IRP was canceled before we set cancel routine */
                InitializeListHead(&irp->Tail.Overlay.s.ListEntry);
                KeReleaseSpinLock(&ext->irp_queue_lock, old_irql);
                return STATUS_CANCELLED;
            }

            InsertTailList(&ext->irp_queue, &irp->Tail.Overlay.s.ListEntry);
            IoMarkIrpPending(irp);

            KeReleaseSpinLock(&ext->irp_queue_lock, old_irql);
            rc = STATUS_PENDING;
        }
        else
        {
            HID_XFER_PACKET packet;
            TRACE("No packet, but opportunistic reads enabled\n");
            packet.reportId = ((BYTE*)irp->AssociatedIrp.SystemBuffer)[0];
            packet.reportBuffer = &((BYTE*)irp->AssociatedIrp.SystemBuffer)[1];
            packet.reportBufferLen = irpsp->Parameters.Read.Length - 1;
            rc = call_minidriver(IOCTL_HID_GET_INPUT_REPORT, device, NULL, 0, &packet, sizeof(packet));

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

NTSTATUS WINAPI HID_Device_write(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
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
        max_len = ext->preparseData->caps.OutputReportByteLength;
    }
    else
    {
        packet.reportBuffer = irp->AssociatedIrp.SystemBuffer;
        packet.reportBufferLen = irpsp->Parameters.Write.Length;
        max_len = (ext->preparseData->reports[ext->preparseData->reportIdx[HidP_Output][packet.reportId]].bitSize + 7) / 8;
    }
    if (packet.reportBufferLen > max_len)
        packet.reportBufferLen = max_len;

    TRACE_(hid_report)("(id %i, len %i buffer %p)\n", packet.reportId, packet.reportBufferLen, packet.reportBuffer);

    rc = call_minidriver(IOCTL_HID_WRITE_REPORT, device, NULL, 0, &packet, sizeof(packet));

    irp->IoStatus.u.Status = rc;
    if (irp->IoStatus.u.Status == STATUS_SUCCESS)
        irp->IoStatus.Information = irpsp->Parameters.Write.Length;
    else
        irp->IoStatus.Information = 0;

    TRACE_(hid_report)("Result 0x%x wrote %li bytes\n", rc, irp->IoStatus.Information);

    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return rc;
}

NTSTATUS WINAPI HID_Device_create(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    TRACE("Open handle on device %p\n", device);
    irp->Tail.Overlay.OriginalFileObject->FsContext = UlongToPtr(RingBuffer_AddPointer(ext->ring_buffer));
    irp->IoStatus.u.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI HID_Device_close(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    int ptr = PtrToUlong(irp->Tail.Overlay.OriginalFileObject->FsContext);
    TRACE("Close handle on device %p\n", device);
    RingBuffer_RemovePointer(ext->ring_buffer, ptr);
    irp->IoStatus.u.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}
