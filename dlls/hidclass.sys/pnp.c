/*
 * WINE HID Pseudo-Plug and Play support
 *
 * Copyright 2015 Aric Stewart
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

#define NONAMELESSUNION
#include <unistd.h>
#include <stdarg.h>
#include "hid.h"
#include "ddk/hidtypes.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

typedef struct _NATIVE_DEVICE {
    struct list entry;

    DWORD vidpid;
    DEVICE_OBJECT *PDO;
    DEVICE_OBJECT *FDO;
    HID_MINIDRIVER_REGISTRATION *minidriver;

} NATIVE_DEVICE;

static struct list tracked_devices = LIST_INIT(tracked_devices);

static NTSTATUS WINAPI internalComplete(DEVICE_OBJECT *deviceObject, IRP *irp,
    void *context )
{
    SetEvent(irp->UserEvent);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS SendDeviceIRP(DEVICE_OBJECT* device, IRP *irp)
{
    NTSTATUS status;
    IO_STACK_LOCATION *irpsp;
    HANDLE event = CreateEventA(NULL, FALSE, FALSE, NULL);

    irp->UserEvent = event;
    irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->CompletionRoutine = internalComplete;
    irpsp->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR;

    IoCallDriver(device, irp);

    if (irp->IoStatus.u.Status == STATUS_PENDING)
        WaitForSingleObject(event, INFINITE);

    status = irp->IoStatus.u.Status;
    IoCompleteRequest(irp, IO_NO_INCREMENT );
    CloseHandle(event);
    return status;
}

static NTSTATUS PNP_SendPnPIRP(DEVICE_OBJECT *device, UCHAR minor)
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;

    IRP *irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status);

    irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->MinorFunction = minor;

    irpsp->Parameters.StartDevice.AllocatedResources = NULL;
    irpsp->Parameters.StartDevice.AllocatedResourcesTranslated = NULL;

    return SendDeviceIRP(device, irp);
}

NTSTATUS WINAPI PNP_AddDevice(DRIVER_OBJECT *driver, DEVICE_OBJECT *PDO)
{
    DEVICE_OBJECT *device = NULL;
    NTSTATUS status;
    minidriver *minidriver;
    HID_DEVICE_ATTRIBUTES attr;
    BASE_DEVICE_EXTENSION *ext = NULL;
    WCHAR serial[256];
    WCHAR interface[256];
    DWORD index = HID_STRING_ID_ISERIALNUMBER;
    NATIVE_DEVICE *tracked_device, *ptr;
    INT interface_index = 1;
    HID_DESCRIPTOR descriptor;
    BYTE *reportDescriptor;
    INT i;

    static const WCHAR ig_fmtW[] = {'I','G','_','%','i',0};
    static const WCHAR im_fmtW[] = {'I','M','_','%','i',0};


    TRACE("PDO add device(%p)\n", PDO);
    minidriver = find_minidriver(driver);

    status = HID_CreateDevice(PDO, &minidriver->minidriver, &device);
    if (status != STATUS_SUCCESS)
    {
        ERR("Failed to create HID object (%x)\n",status);
        return status;
    }

    TRACE("Created device %p\n",device);
    status = minidriver->AddDevice(minidriver->minidriver.DriverObject, device);
    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver AddDevice failed (%x)\n",status);
        HID_DeleteDevice(&minidriver->minidriver, device);
        return status;
    }

    status = PNP_SendPnPIRP(device, IRP_MN_START_DEVICE);
    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver IRP_MN_START_DEVICE failed (%x)\n",status);
        HID_DeleteDevice(&minidriver->minidriver, device);
        return status;
    }

    status = call_minidriver(IOCTL_HID_GET_DEVICE_ATTRIBUTES, device,
        NULL, 0, &attr, sizeof(attr));

    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver failed to get Attributes(%x)\n",status);
        PNP_SendPnPIRP(device, IRP_MN_REMOVE_DEVICE);
        HID_DeleteDevice(&minidriver->minidriver, device);
        return status;
    }

    ext = device->DeviceExtension;
    ext->information.VendorID = attr.VendorID;
    ext->information.ProductID = attr.ProductID;
    ext->information.VersionNumber = attr.VersionNumber;
    ext->information.Polled = minidriver->minidriver.DevicesArePolled;

    tracked_device = HeapAlloc(GetProcessHeap(), 0, sizeof(*tracked_device));
    tracked_device->vidpid = MAKELONG(attr.VendorID, attr.ProductID);
    tracked_device->PDO = PDO;
    tracked_device->FDO = device;
    tracked_device->minidriver = &minidriver->minidriver;

    LIST_FOR_EACH_ENTRY(ptr, &tracked_devices, NATIVE_DEVICE, entry)
        if (ptr->vidpid == tracked_device->vidpid) interface_index++;

    list_add_tail(&tracked_devices, &tracked_device->entry);

    status = call_minidriver(IOCTL_HID_GET_DEVICE_DESCRIPTOR, device, NULL, 0,
        &descriptor, sizeof(descriptor));
    if (status != STATUS_SUCCESS)
    {
        ERR("Cannot get Device Descriptor(%x)\n",status);
        PNP_SendPnPIRP(device, IRP_MN_REMOVE_DEVICE);
        HID_DeleteDevice(&minidriver->minidriver, device);
        return status;
    }
    for (i = 0; i < descriptor.bNumDescriptors; i++)
        if (descriptor.DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE)
            break;

    if (i >= descriptor.bNumDescriptors)
    {
        ERR("No Report Descriptor found in reply\n");
        PNP_SendPnPIRP(device, IRP_MN_REMOVE_DEVICE);
        HID_DeleteDevice(&minidriver->minidriver, device);
        return status;
    }

    reportDescriptor = HeapAlloc(GetProcessHeap(), 0, descriptor.DescriptorList[i].wReportLength);
    status = call_minidriver(IOCTL_HID_GET_REPORT_DESCRIPTOR, device, NULL, 0,
        reportDescriptor, descriptor.DescriptorList[i].wReportLength);
    if (status != STATUS_SUCCESS)
    {
        ERR("Cannot get Report Descriptor(%x)\n",status);
        HID_DeleteDevice(&minidriver->minidriver, device);
        HeapFree(GetProcessHeap(), 0, reportDescriptor);
        return status;
    }

    ext->preparseData = ParseDescriptor(reportDescriptor, descriptor.DescriptorList[0].wReportLength);
    ext->information.DescriptorSize = ext->preparseData->dwSize;
    HeapFree(GetProcessHeap(), 0, reportDescriptor);

    serial[0] = 0;
    status = call_minidriver(IOCTL_HID_GET_STRING, device,
                             &index, sizeof(DWORD), serial, sizeof(serial));

    if (serial[0] == 0)
    {
        static const WCHAR wZeroSerial[]= {'0','0','0','0',0};
        lstrcpyW(serial, wZeroSerial);
    }

    if (ext->preparseData->caps.UsagePage == HID_USAGE_PAGE_GENERIC &&
        (ext->preparseData->caps.Usage == HID_USAGE_GENERIC_GAMEPAD ||
         ext->preparseData->caps.Usage == HID_USAGE_GENERIC_JOYSTICK))
        sprintfW(interface, ig_fmtW, interface_index);
    else
        sprintfW(interface, im_fmtW, interface_index);

    HID_LinkDevice(device, serial, interface);

    ext->poll_interval = DEFAULT_POLL_INTERVAL;
    InitializeListHead(&ext->irp_queue);

    ext->ring_buffer = RingBuffer_Create(sizeof(HID_XFER_PACKET) + ext->preparseData->caps.InputReportByteLength);

    HID_StartDeviceThread(device);

    return STATUS_SUCCESS;
}

void PNP_CleanupPNP(DRIVER_OBJECT *driver)
{
    NATIVE_DEVICE *tracked_device, *ptr;

    LIST_FOR_EACH_ENTRY_SAFE(tracked_device, ptr, &tracked_devices,
        NATIVE_DEVICE, entry)
    {
        if (tracked_device->minidriver->DriverObject == driver)
        {
            list_remove(&tracked_device->entry);
            PNP_SendPnPIRP(tracked_device->FDO, IRP_MN_REMOVE_DEVICE);
            HID_DeleteDevice(tracked_device->minidriver, tracked_device->FDO);
            HeapFree(GetProcessHeap(), 0, tracked_device);
        }
    }
}
