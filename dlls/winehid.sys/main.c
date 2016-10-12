/*
 * WINE Hid minidriver
 *
 * Copyright 2016 Aric Stewart
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/hidport.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static NTSTATUS WINAPI add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *device)
{
    TRACE("(%p, %p)\n", driver, device);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    HID_MINIDRIVER_REGISTRATION registration;

    TRACE("(%p, %s)\n", driver, debugstr_w(path->Buffer));

    driver->DriverExtension->AddDevice = add_device;

    memset(&registration, 0, sizeof(registration));
    registration.DriverObject = driver;
    registration.RegistryPath = path;
    registration.DeviceExtensionSize = sizeof(HID_DEVICE_EXTENSION);
    registration.DevicesArePolled = FALSE;

    return HidRegisterMinidriver(&registration);
}
