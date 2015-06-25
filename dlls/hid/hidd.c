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

#include "ddk/hidclass.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

BOOLEAN WINAPI HidD_GetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_FEATURE, NULL, 0, ReportBuffer, ReportBufferLength, NULL, NULL);
}

void WINAPI HidD_GetHidGuid(LPGUID guid)
{
    TRACE("(%p)\n", guid);
    *guid = GUID_DEVINTERFACE_HID;
}

BOOLEAN WINAPI HidD_GetManufacturerString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength)
{
    TRACE("(%p %p %u) \n", HidDeviceObject, Buffer, BufferLength);
    return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_MANUFACTURER_STRING, NULL, 0, Buffer, BufferLength, NULL, NULL);
}

BOOLEAN WINAPI HidD_SetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return DeviceIoControl(HidDeviceObject, IOCTL_HID_SET_FEATURE, ReportBuffer, ReportBufferLength, NULL, 0, NULL, NULL);
}

BOOLEAN WINAPI HidD_GetProductString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength)
{
    TRACE("(%p %p %u)\n", HidDeviceObject, Buffer, BufferLength);
    return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_PRODUCT_STRING, NULL, 0, Buffer, BufferLength, NULL, NULL);
}
