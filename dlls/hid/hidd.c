/*
 * Human Input Devices
 *
 * Copyright (C) 2006 Kevin Koltzau
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

#include "wine/debug.h"

#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "hidusage.h"
#include "ddk/hidclass.h"
#include "ddk/hidsdi.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static BOOL sync_ioctl(HANDLE file, DWORD code, void *in_buf, DWORD in_len, void *out_buf, DWORD out_len, DWORD *ret_len)
{
    OVERLAPPED ovl = {0};
    BOOL ret;

    ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ret = DeviceIoControl(file, code, in_buf, in_len, out_buf, out_len, ret_len, &ovl);
    if (!ret && GetLastError() == ERROR_IO_PENDING)
        ret = GetOverlappedResult(file, &ovl, ret_len, TRUE);
    CloseHandle(ovl.hEvent);
    return ret;
}

BOOLEAN WINAPI HidD_FreePreparsedData(PHIDP_PREPARSED_DATA PreparsedData)
{
    TRACE("(%p)\n", PreparsedData);
    HeapFree(GetProcessHeap(), 0, PreparsedData);
    return TRUE;
}

BOOLEAN WINAPI HidD_GetAttributes(HANDLE HidDeviceObject, PHIDD_ATTRIBUTES Attr)
{
    HID_COLLECTION_INFORMATION info;
    BOOLEAN ret;

    TRACE("(%p %p)\n", HidDeviceObject, Attr);

    ret = sync_ioctl(HidDeviceObject, IOCTL_HID_GET_COLLECTION_INFORMATION, NULL, 0, &info, sizeof(HID_COLLECTION_INFORMATION), NULL);

    if (ret)
    {
        Attr->VendorID = info.VendorID;
        Attr->ProductID = info.ProductID;
        Attr->VersionNumber = info.VersionNumber;
    }
    return ret;
}

BOOLEAN WINAPI HidD_GetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return sync_ioctl(HidDeviceObject, IOCTL_HID_GET_FEATURE, NULL, 0, ReportBuffer, ReportBufferLength, NULL);
}

void WINAPI HidD_GetHidGuid(LPGUID guid)
{
    TRACE("(%p)\n", guid);
    *guid = GUID_DEVINTERFACE_HID;
}

BOOLEAN WINAPI HidD_GetInputReport(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return sync_ioctl(HidDeviceObject, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, ReportBuffer, ReportBufferLength, NULL);
}

BOOLEAN WINAPI HidD_GetManufacturerString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, Buffer, BufferLength);
    return sync_ioctl(HidDeviceObject, IOCTL_HID_GET_MANUFACTURER_STRING, NULL, 0, Buffer, BufferLength, NULL);
}

BOOLEAN WINAPI HidD_GetNumInputBuffers(HANDLE HidDeviceObject, ULONG *NumberBuffers)
{
    TRACE("(%p %p)\n", HidDeviceObject, NumberBuffers);
    return sync_ioctl(HidDeviceObject, IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS, NULL, 0, NumberBuffers, sizeof(*NumberBuffers), NULL);
}

BOOLEAN WINAPI HidD_SetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return sync_ioctl(HidDeviceObject, IOCTL_HID_SET_FEATURE, ReportBuffer, ReportBufferLength, NULL, 0, NULL);
}

BOOLEAN WINAPI HidD_SetNumInputBuffers(HANDLE HidDeviceObject, ULONG NumberBuffers)
{
    TRACE("(%p %i)\n", HidDeviceObject, NumberBuffers);
    return sync_ioctl(HidDeviceObject, IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS, &NumberBuffers, sizeof(NumberBuffers), NULL, 0, NULL);
}

BOOLEAN WINAPI HidD_GetProductString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, Buffer, BufferLength);
    return sync_ioctl(HidDeviceObject, IOCTL_HID_GET_PRODUCT_STRING, NULL, 0, Buffer, BufferLength, NULL);
}

BOOLEAN WINAPI HidD_GetSerialNumberString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, Buffer, BufferLength);
    return sync_ioctl(HidDeviceObject, IOCTL_HID_GET_SERIALNUMBER_STRING, NULL, 0, Buffer, BufferLength, NULL);
}

BOOLEAN WINAPI HidD_GetPreparsedData(HANDLE HidDeviceObject, PHIDP_PREPARSED_DATA *PreparsedData)
{
    HID_COLLECTION_INFORMATION info;
    PHIDP_PREPARSED_DATA data;

    TRACE("(%p %p)\n", HidDeviceObject, PreparsedData);

    if (!sync_ioctl(HidDeviceObject, IOCTL_HID_GET_COLLECTION_INFORMATION, NULL, 0,
                         &info, sizeof(HID_COLLECTION_INFORMATION), NULL))
        return FALSE;

    if (!(data = HeapAlloc(GetProcessHeap(), 0, info.DescriptorSize))) return FALSE;

    if (!sync_ioctl(HidDeviceObject, IOCTL_HID_GET_COLLECTION_DESCRIPTOR, NULL, 0,
                         data, info.DescriptorSize, NULL))
    {
        HeapFree( GetProcessHeap(), 0, data );
        return FALSE;
    }
    *PreparsedData = data;
    return TRUE;
}

BOOLEAN WINAPI HidD_SetOutputReport(HANDLE HidDeviceObject, void *ReportBuffer, ULONG ReportBufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return sync_ioctl(HidDeviceObject, IOCTL_HID_SET_OUTPUT_REPORT, ReportBuffer, ReportBufferLength, NULL, 0, NULL);
}
