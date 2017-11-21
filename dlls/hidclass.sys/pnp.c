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
#include "regstr.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static const WCHAR device_enumeratorW[] = {'H','I','D',0};
static const WCHAR separator_W[] = {'\\',0};
static const WCHAR device_deviceid_fmtW[] = {'%','s','\\',
    'v','i','d','_','%','0','4','x','&','p','i','d','_','%', '0','4','x',0};

static NTSTATUS WINAPI internalComplete(DEVICE_OBJECT *deviceObject, IRP *irp,
    void *context)
{
    HANDLE event = context;
    SetEvent(event);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS get_device_id(DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR **id)
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

    *id = (WCHAR*)irp->IoStatus.Information;
    status = irp->IoStatus.u.Status;
    IoCompleteRequest(irp, IO_NO_INCREMENT );
    CloseHandle(event);

    return status;
}

NTSTATUS WINAPI PNP_AddDevice(DRIVER_OBJECT *driver, DEVICE_OBJECT *PDO)
{
    DEVICE_OBJECT *device = NULL;
    NTSTATUS status;
    minidriver *minidriver;
    HID_DEVICE_ATTRIBUTES attr;
    BASE_DEVICE_EXTENSION *ext = NULL;
    HID_DESCRIPTOR descriptor;
    BYTE *reportDescriptor;
    INT i;
    WCHAR *PDO_id;
    WCHAR *id_ptr;

    status = get_device_id(PDO, BusQueryInstanceID, &PDO_id);
    if (status != STATUS_SUCCESS)
    {
        ERR("Failed to get PDO id(%x)\n",status);
        return status;
    }

    TRACE("PDO add device(%p:%s)\n", PDO, debugstr_w(PDO_id));
    minidriver = find_minidriver(driver);

    status = HID_CreateDevice(PDO, &minidriver->minidriver, &device);
    if (status != STATUS_SUCCESS)
    {
        ERR("Failed to create HID object (%x)\n",status);
        HeapFree(GetProcessHeap(), 0, PDO_id);
        return status;
    }

    ext = device->DeviceExtension;
    InitializeListHead(&ext->irp_queue);

    TRACE("Created device %p\n",device);
    status = minidriver->AddDevice(minidriver->minidriver.DriverObject, device);
    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver AddDevice failed (%x)\n",status);
        HeapFree(GetProcessHeap(), 0, PDO_id);
        HID_DeleteDevice(&minidriver->minidriver, device);
        return status;
    }

    status = call_minidriver(IOCTL_HID_GET_DEVICE_ATTRIBUTES, device,
        NULL, 0, &attr, sizeof(attr));

    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver failed to get Attributes(%x)\n",status);
        HID_DeleteDevice(&minidriver->minidriver, device);
        HeapFree(GetProcessHeap(), 0, PDO_id);
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
        HID_DeleteDevice(&minidriver->minidriver, device);
        HeapFree(GetProcessHeap(), 0, PDO_id);
        return status;
    }
    for (i = 0; i < descriptor.bNumDescriptors; i++)
        if (descriptor.DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE)
            break;

    if (i >= descriptor.bNumDescriptors)
    {
        ERR("No Report Descriptor found in reply\n");
        HID_DeleteDevice(&minidriver->minidriver, device);
        HeapFree(GetProcessHeap(), 0, PDO_id);
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
        HeapFree(GetProcessHeap(), 0, PDO_id);
        return status;
    }

    ext->preparseData = ParseDescriptor(reportDescriptor, descriptor.DescriptorList[0].wReportLength);

    HeapFree(GetProcessHeap(), 0, reportDescriptor);
    if (!ext->preparseData)
    {
        ERR("Cannot parse Report Descriptor\n");
        HID_DeleteDevice(&minidriver->minidriver, device);
        HeapFree(GetProcessHeap(), 0, PDO_id);
        return STATUS_NOT_SUPPORTED;
    }

    ext->information.DescriptorSize = ext->preparseData->dwSize;

    lstrcpyW(ext->instance_id, device_enumeratorW);
    strcatW(ext->instance_id, separator_W);
    /* Skip the original enumerator */
    id_ptr = strchrW(PDO_id, '\\');
    id_ptr++;
    strcatW(ext->instance_id, id_ptr);
    HeapFree(GetProcessHeap(), 0, PDO_id);

    sprintfW(ext->device_id, device_deviceid_fmtW, device_enumeratorW, ext->information.VendorID, ext->information.ProductID);

    HID_LinkDevice(device);

    ext->poll_interval = DEFAULT_POLL_INTERVAL;

    ext->ring_buffer = RingBuffer_Create(sizeof(HID_XFER_PACKET) + ext->preparseData->caps.InputReportByteLength);

    HID_StartDeviceThread(device);

    return STATUS_SUCCESS;
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
            WCHAR *id = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR)*REGSTR_VAL_MAX_HCID_LEN);
            TRACE("IRP_MN_QUERY_ID[%i]\n", irpsp->Parameters.QueryId.IdType);
            switch (irpsp->Parameters.QueryId.IdType)
            {
                case BusQueryHardwareIDs:
                case BusQueryCompatibleIDs:
                {
                    WCHAR *ptr;
                    ptr = id;
                    /* Instance ID */
                    strcpyW(ptr, ext->instance_id);
                    ptr += lstrlenW(ext->instance_id) + 1;
                    /* Device ID */
                    strcpyW(ptr, ext->device_id);
                    ptr += lstrlenW(ext->device_id) + 1;
                    /* Bus ID */
                    strcpyW(ptr, device_enumeratorW);
                    ptr += lstrlenW(device_enumeratorW) + 1;
                    *ptr = 0;
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    rc = STATUS_SUCCESS;
                    break;
                }
                case BusQueryDeviceID:
                    strcpyW(id, ext->device_id);
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    rc = STATUS_SUCCESS;
                    break;
                case BusQueryInstanceID:
                    strcpyW(id, ext->instance_id);
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
        case IRP_MN_REMOVE_DEVICE:
        {
            rc = minidriver->PNPDispatch(device, irp);
            HID_DeleteDevice(&minidriver->minidriver, device);
            return rc;
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
