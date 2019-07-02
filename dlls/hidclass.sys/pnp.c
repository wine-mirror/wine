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
#include "ddk/wdm.h"
#include "regstr.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static const WCHAR device_enumeratorW[] = {'H','I','D',0};
static const WCHAR separator_W[] = {'\\',0};

static NTSTATUS WINAPI internalComplete(DEVICE_OBJECT *deviceObject, IRP *irp,
    void *context)
{
    HANDLE event = context;
    SetEvent(event);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS get_device_id(DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR *id)
{
    NTSTATUS status;
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    HANDLE event;
    IRP *irp;

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status);
    if (irp == NULL)
        return STATUS_NO_MEMORY;

    event = CreateEventA(NULL, FALSE, FALSE, NULL);
    irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->MinorFunction = IRP_MN_QUERY_ID;
    irpsp->Parameters.QueryId.IdType = type;

    IoSetCompletionRoutine(irp, internalComplete, event, TRUE, TRUE, TRUE);
    status = IoCallDriver(device, irp);
    if (status == STATUS_PENDING)
        WaitForSingleObject(event, INFINITE);

    lstrcpyW(id, (WCHAR *)irp->IoStatus.Information);
    ExFreePool( (WCHAR *)irp->IoStatus.Information );
    status = irp->IoStatus.u.Status;
    IoCompleteRequest(irp, IO_NO_INCREMENT );
    CloseHandle(event);

    return status;
}

NTSTATUS WINAPI PNP_AddDevice(DRIVER_OBJECT *driver, DEVICE_OBJECT *PDO)
{
    WCHAR device_id[MAX_DEVICE_ID_LEN], instance_id[MAX_DEVICE_ID_LEN];
    hid_device *hiddev;
    DEVICE_OBJECT *device = NULL;
    NTSTATUS status;
    minidriver *minidriver;
    HID_DEVICE_ATTRIBUTES attr;
    BASE_DEVICE_EXTENSION *ext = NULL;
    HID_DESCRIPTOR descriptor;
    BYTE *reportDescriptor;
    INT i;

    if ((status = get_device_id(PDO, BusQueryDeviceID, device_id)))
    {
        ERR("Failed to get PDO device id, status %#x.\n", status);
        return status;
    }

    if ((status = get_device_id(PDO, BusQueryInstanceID, instance_id)))
    {
        ERR("Failed to get PDO instance id, status %#x.\n", status);
        return status;
    }

    TRACE("Adding device to PDO %p, id %s\\%s.\n", PDO, debugstr_w(device_id), debugstr_w(instance_id));
    minidriver = find_minidriver(driver);

    hiddev = HeapAlloc(GetProcessHeap(), 0, sizeof(*hiddev));
    if (!hiddev)
        return STATUS_NO_MEMORY;

    status = HID_CreateDevice(PDO, &minidriver->minidriver, &hiddev->device);
    if (status != STATUS_SUCCESS)
    {
        ERR("Failed to create HID object (%x)\n",status);
        HeapFree(GetProcessHeap(), 0, hiddev);
        return status;
    }
    device = hiddev->device;

    ext = device->DeviceExtension;
    InitializeListHead(&ext->irp_queue);
    KeInitializeSpinLock(&ext->irp_queue_lock);

    TRACE("Created device %p\n",device);
    status = minidriver->AddDevice(minidriver->minidriver.DriverObject, device);
    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver AddDevice failed (%x)\n",status);
        HID_DeleteDevice(device);
        return status;
    }

    status = call_minidriver(IOCTL_HID_GET_DEVICE_ATTRIBUTES, device,
        NULL, 0, &attr, sizeof(attr));

    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver failed to get Attributes(%x)\n",status);
        HID_DeleteDevice(device);
        return status;
    }

    ext->information.VendorID = attr.VendorID;
    ext->information.ProductID = attr.ProductID;
    ext->information.VersionNumber = attr.VersionNumber;
    ext->information.Polled = minidriver->minidriver.DevicesArePolled;

    status = call_minidriver(IOCTL_HID_GET_DEVICE_DESCRIPTOR, device, NULL, 0,
        &descriptor, sizeof(descriptor));
    if (status != STATUS_SUCCESS)
    {
        ERR("Cannot get Device Descriptor(%x)\n",status);
        HID_DeleteDevice(device);
        return status;
    }
    for (i = 0; i < descriptor.bNumDescriptors; i++)
        if (descriptor.DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE)
            break;

    if (i >= descriptor.bNumDescriptors)
    {
        ERR("No Report Descriptor found in reply\n");
        HID_DeleteDevice(device);
        return status;
    }

    reportDescriptor = HeapAlloc(GetProcessHeap(), 0, descriptor.DescriptorList[i].wReportLength);
    status = call_minidriver(IOCTL_HID_GET_REPORT_DESCRIPTOR, device, NULL, 0,
        reportDescriptor, descriptor.DescriptorList[i].wReportLength);
    if (status != STATUS_SUCCESS)
    {
        ERR("Cannot get Report Descriptor(%x)\n",status);
        HID_DeleteDevice(device);
        HeapFree(GetProcessHeap(), 0, reportDescriptor);
        return status;
    }

    ext->preparseData = ParseDescriptor(reportDescriptor, descriptor.DescriptorList[0].wReportLength);

    HeapFree(GetProcessHeap(), 0, reportDescriptor);
    if (!ext->preparseData)
    {
        ERR("Cannot parse Report Descriptor\n");
        HID_DeleteDevice(device);
        return STATUS_NOT_SUPPORTED;
    }

    list_add_tail(&(minidriver->device_list), &hiddev->entry);

    ext->information.DescriptorSize = ext->preparseData->dwSize;

    lstrcpyW(ext->instance_id, instance_id);

    lstrcpyW(ext->device_id, device_enumeratorW);
    lstrcatW(ext->device_id, separator_W);
    lstrcatW(ext->device_id, wcschr(device_id, '\\') + 1);

    HID_LinkDevice(device);

    ext->poll_interval = DEFAULT_POLL_INTERVAL;

    ext->ring_buffer = RingBuffer_Create(sizeof(HID_XFER_PACKET) + ext->preparseData->caps.InputReportByteLength);

    HID_StartDeviceThread(device);

    return STATUS_SUCCESS;
}

NTSTATUS PNP_RemoveDevice(minidriver *minidriver, DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    hid_device *hiddev;
    NTSTATUS rc = STATUS_NOT_SUPPORTED;

    rc = IoSetDeviceInterfaceState(&ext->link_name, FALSE);
    if (rc)
    {
        FIXME("failed to disable interface %x\n", rc);
        return rc;
    }

    if (ext->is_mouse)
        IoSetDeviceInterfaceState(&ext->mouse_link_name, FALSE);

    if (irp)
        rc = minidriver->PNPDispatch(device, irp);
    HID_DeleteDevice(device);
    LIST_FOR_EACH_ENTRY(hiddev,  &minidriver->device_list, hid_device, entry)
    {
        if (hiddev->device == device)
        {
            list_remove(&hiddev->entry);
            HeapFree(GetProcessHeap(), 0, hiddev);
            break;
        }
    }
    return rc;
}

NTSTATUS WINAPI HID_PNP_Dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS rc = STATUS_NOT_SUPPORTED;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    minidriver *minidriver = find_minidriver(device->DriverObject);

    TRACE("%p, %p\n", device, irp);

    switch(irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
        {
            BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
            WCHAR *id = ExAllocatePool(PagedPool, sizeof(WCHAR) * REGSTR_VAL_MAX_HCID_LEN);
            TRACE("IRP_MN_QUERY_ID[%i]\n", irpsp->Parameters.QueryId.IdType);
            switch (irpsp->Parameters.QueryId.IdType)
            {
                case BusQueryHardwareIDs:
                case BusQueryCompatibleIDs:
                {
                    WCHAR *ptr;
                    ptr = id;
                    /* Device instance ID */
                    lstrcpyW(ptr, ext->device_id);
                    ptr += lstrlenW(ext->device_id);
                    lstrcpyW(ptr, separator_W);
                    ptr += 1;
                    lstrcpyW(ptr, ext->instance_id);
                    ptr += lstrlenW(ext->instance_id) + 1;
                    /* Device ID */
                    lstrcpyW(ptr, ext->device_id);
                    ptr += lstrlenW(ext->device_id) + 1;
                    /* Bus ID */
                    lstrcpyW(ptr, device_enumeratorW);
                    ptr += lstrlenW(device_enumeratorW) + 1;
                    *ptr = 0;
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    rc = STATUS_SUCCESS;
                    break;
                }
                case BusQueryDeviceID:
                    lstrcpyW(id, ext->device_id);
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    rc = STATUS_SUCCESS;
                    break;
                case BusQueryInstanceID:
                    lstrcpyW(id, ext->instance_id);
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    rc = STATUS_SUCCESS;
                    break;
                case BusQueryDeviceSerialNumber:
                    FIXME("BusQueryDeviceSerialNumber not implemented\n");
                    HeapFree(GetProcessHeap(), 0, id);
                    break;
            }
            break;
        }
        case IRP_MN_START_DEVICE:
        {
            BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

            rc = minidriver->PNPDispatch(device, irp);

            IoSetDeviceInterfaceState(&ext->link_name, TRUE);
            if (ext->is_mouse)
                IoSetDeviceInterfaceState(&ext->mouse_link_name, TRUE);
            return rc;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            return PNP_RemoveDevice(minidriver, device, irp);
        }
        default:
        {
            /* Forward IRP to the minidriver */
            return minidriver->PNPDispatch(device, irp);
        }
    }

    irp->IoStatus.u.Status = rc;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return rc;
}
