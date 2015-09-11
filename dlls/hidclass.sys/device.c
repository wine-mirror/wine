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

#include "config.h"

#include <stdarg.h>
#define NONAMELESSUNION
#include "hid.h"
#include "wine/unicode.h"
#include "winreg.h"
#include "winuser.h"
#include "setupapi.h"

#include "wine/debug.h"
#include "ddk/hidsdi.h"
#include "ddk/hidtypes.h"

#include "initguid.h"
#include "devguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static const WCHAR device_name_fmtW[] = {'\\','D','e','v','i','c','e',
    '\\','H','I','D','#','%','p','&','%','p',0};
static const WCHAR device_regname_fmtW[] = {'H','I','D','\\',
    'v','i','d','_','%','0','4','x','&','p','i','d','_','%',
    '0','4','x','&','%','s','\\','%','i','&','%','s',0};
static const WCHAR device_link_fmtW[] = {'\\','?','?','\\','h','i','d','#',
    'v','i','d','_','%','0','4','x','&','p','i','d','_','%',
    '0','4','x','&','%','s','#','%','i','&','%','s','#','%','s',0};
/* GUID_DEVINTERFACE_HID */
static const WCHAR class_guid[] = {'{','4','D','1','E','5','5','B','2',
    '-','F','1','6','F','-','1','1','C','F','-','8','8','C','B','-','0','0',
    '1','1','1','1','0','0','0','0','3','0','}',0};


NTSTATUS HID_CreateDevice(DEVICE_OBJECT *native_device, HID_MINIDRIVER_REGISTRATION *driver, DEVICE_OBJECT **device)
{
    WCHAR dev_name[255];
    UNICODE_STRING nameW;
    NTSTATUS status;
    BASE_DEVICE_EXTENSION *ext;

    sprintfW(dev_name, device_name_fmtW, driver->DriverObject, native_device);
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
    ext->link_name = NULL;

    return S_OK;
}

NTSTATUS HID_LinkDevice(DEVICE_OBJECT *device, LPCWSTR serial, LPCWSTR index)
{
    WCHAR regname[255];
    WCHAR dev_link[255];
    SP_DEVINFO_DATA Data;
    UNICODE_STRING nameW, linkW;
    NTSTATUS status;
    HDEVINFO devinfo;
    GUID hidGuid;
    BASE_DEVICE_EXTENSION *ext;

    HidD_GetHidGuid(&hidGuid);
    ext = device->DeviceExtension;

    sprintfW(dev_link, device_link_fmtW, ext->information.VendorID,
        ext->information.ProductID, index, ext->information.VersionNumber, serial,
        class_guid);
    struprW(dev_link);

    RtlInitUnicodeString( &nameW, ext->device_name);
    RtlInitUnicodeString( &linkW, dev_link );

    TRACE("Create link %s\n", debugstr_w(dev_link));

    ext->link_name = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (lstrlenW(dev_link) + 1));
    lstrcpyW(ext->link_name, dev_link);

    status = IoCreateSymbolicLink( &linkW, &nameW );
    if (status)
    {
        FIXME( "failed to create link error %x\n", status );
        return status;
    }

    sprintfW(regname, device_regname_fmtW, ext->information.VendorID, ext->information.ProductID, index, ext->information.VersionNumber, serial);

    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_HIDCLASS, NULL, NULL, DIGCF_DEVICEINTERFACE);
    if (!devinfo)
    {
        FIXME( "failed to get ClassDevs %x\n", GetLastError());
        return GetLastError();
    }
    Data.cbSize = sizeof(Data);
    if (!SetupDiCreateDeviceInfoW(devinfo, regname, &GUID_DEVCLASS_HIDCLASS, NULL, NULL, DICD_INHERIT_CLASSDRVS, &Data))
    {
        if (GetLastError() == ERROR_DEVINST_ALREADY_EXISTS)
        {
            SetupDiDestroyDeviceInfoList(devinfo);
            return ERROR_SUCCESS;
        }
        FIXME( "failed to Create Device Info %x\n", GetLastError());
        return GetLastError();
    }
    if (!SetupDiRegisterDeviceInfo( devinfo, &Data, 0, NULL, NULL, NULL ))
    {
        FIXME( "failed to Register Device Info %x\n", GetLastError());
        return GetLastError();
    }
    if (!SetupDiCreateDeviceInterfaceW( devinfo, &Data,  &hidGuid, NULL, 0, NULL))
    {
        FIXME( "failed to Create Device Interface %x\n", GetLastError());
        return GetLastError();
    }
    SetupDiDestroyDeviceInfoList(devinfo);

    return S_OK;
}

void HID_DeleteDevice(HID_MINIDRIVER_REGISTRATION *driver, DEVICE_OBJECT *device)
{
    NTSTATUS status;
    BASE_DEVICE_EXTENSION *ext;
    UNICODE_STRING linkW;
    LIST_ENTRY *entry;
    IRP *irp;

    ext = device->DeviceExtension;

    if (ext->link_name)
    {
        TRACE("Delete link %s\n", debugstr_w(ext->link_name));
        RtlInitUnicodeString(&linkW, ext->link_name);

        status = IoDeleteSymbolicLink(&linkW);
        if (status != STATUS_SUCCESS)
            ERR("Delete Symbolic Link failed (%x)\n",status);
    }

    if (ext->thread)
    {
        SetEvent(ext->halt_event);
        WaitForSingleObject(ext->thread, INFINITE);
    }
    CloseHandle(ext->halt_event);

    HeapFree(GetProcessHeap(), 0, ext->preparseData);
    if (ext->ring_buffer)
        RingBuffer_Destroy(ext->ring_buffer);

    entry = RemoveHeadList(&ext->irp_queue);
    while(entry != &ext->irp_queue)
    {
        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
        irp->IoStatus.u.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        entry = RemoveHeadList(&ext->irp_queue);
    }

    TRACE("Delete device(%p) %s\n", device, debugstr_w(ext->device_name));
    HeapFree(GetProcessHeap(), 0, ext->device_name);
    HeapFree(GetProcessHeap(), 0, ext->link_name);

    IoDeleteDevice(device);
}
