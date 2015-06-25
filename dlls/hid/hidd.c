/*
 * Human Input Devices
 *
 * Copyright (C) 2006 Kevin Koltzau
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

#include "ddk/hidclass.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

BOOLEAN WINAPI HidD_GetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
{
    static int count = 0;
    if (!count++)
        FIXME("(%p %p %u) stub\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return FALSE;
}

void WINAPI HidD_GetHidGuid(LPGUID guid)
{
    TRACE("(%p)\n", guid);
    *guid = GUID_DEVINTERFACE_HID;
}

BOOLEAN WINAPI HidD_GetManufacturerString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength)
{
    FIXME("(%p %p %u) stub\n", HidDeviceObject, Buffer, BufferLength);
    return FALSE;
}

BOOLEAN WINAPI HidD_SetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength)
{
    FIXME("(%p %p %u) stub\n", HidDeviceObject, ReportBuffer, ReportBufferLength);
    return FALSE;
}

BOOLEAN WINAPI HidD_GetProductString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength)
{
    FIXME("(%p %p %u) stub\n", HidDeviceObject, Buffer, BufferLength);
    return FALSE;
}
