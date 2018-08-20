/*
 * Raw Input
 *
 * Copyright 2012 Henri Verbeet
 * Copyright 2018 Zebediah Figura for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/server.h"

#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(rawinput);

/***********************************************************************
 *              GetRawInputDeviceList   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceList(RAWINPUTDEVICELIST *devices, UINT *device_count, UINT size)
{
    TRACE("devices %p, device_count %p, size %u.\n", devices, device_count, size);

    if (size != sizeof(*devices))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0U;
    }

    if (!device_count)
    {
        SetLastError(ERROR_NOACCESS);
        return ~0U;
    }

    if (!devices)
    {
        *device_count = 2;
        return 0;
    }

    if (*device_count < 2)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *device_count = 2;
        return ~0U;
    }

    devices[0].hDevice = WINE_MOUSE_HANDLE;
    devices[0].dwType = RIM_TYPEMOUSE;
    devices[1].hDevice = WINE_KEYBOARD_HANDLE;
    devices[1].dwType = RIM_TYPEKEYBOARD;

    return 2;
}

/***********************************************************************
 *              RegisterRawInputDevices   (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RegisterRawInputDevices(RAWINPUTDEVICE *devices, UINT device_count, UINT size)
{
    struct rawinput_device *d;
    BOOL ret;
    UINT i;

    TRACE("devices %p, device_count %u, size %u.\n", devices, device_count, size);

    if (size != sizeof(*devices))
    {
        WARN("Invalid structure size %u.\n", size);
        return FALSE;
    }

    if (!(d = HeapAlloc( GetProcessHeap(), 0, device_count * sizeof(*d) ))) return FALSE;

    for (i = 0; i < device_count; ++i)
    {
        TRACE("device %u: page %#x, usage %#x, flags %#x, target %p.\n",
                i, devices[i].usUsagePage, devices[i].usUsage,
                devices[i].dwFlags, devices[i].hwndTarget);
        if (devices[i].dwFlags & ~RIDEV_REMOVE)
            FIXME("Unhandled flags %#x for device %u.\n", devices[i].dwFlags, i);

        d[i].usage_page = devices[i].usUsagePage;
        d[i].usage = devices[i].usUsage;
        d[i].flags = devices[i].dwFlags;
        d[i].target = wine_server_user_handle( devices[i].hwndTarget );
    }

    SERVER_START_REQ( update_rawinput_devices )
    {
        wine_server_add_data( req, d, device_count * sizeof(*d) );
        ret = !wine_server_call( req );
    }
    SERVER_END_REQ;

    HeapFree( GetProcessHeap(), 0, d );

    return ret;
}

/***********************************************************************
 *              GetRawInputData   (USER32.@)
 */
UINT WINAPI GetRawInputData(HRAWINPUT rawinput, UINT command, void *data, UINT *data_size, UINT header_size)
{
    RAWINPUT *ri = (RAWINPUT *)rawinput;
    UINT s;

    TRACE("rawinput %p, command %#x, data %p, data_size %p, header_size %u.\n",
            rawinput, command, data, data_size, header_size);

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN("Invalid structure size %u.\n", header_size);
        return ~0U;
    }

    switch (command)
    {
    case RID_INPUT:
        s = ri->header.dwSize;
        break;
    case RID_HEADER:
        s = sizeof(RAWINPUTHEADER);
        break;
    default:
        return ~0U;
    }

    if (!data)
    {
        *data_size = s;
        return 0;
    }

    if (*data_size < s) return ~0U;
    memcpy(data, ri, s);
    return s;
}

/***********************************************************************
 *              GetRawInputBuffer   (USER32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetRawInputBuffer(RAWINPUT *data, UINT *data_size, UINT header_size)
{
    FIXME("data %p, data_size %p, header_size %u stub!\n", data, data_size, header_size);

    return 0;
}

/***********************************************************************
 *              GetRawInputDeviceInfoA   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceInfoA(HANDLE device, UINT command, void *data, UINT *data_size)
{
    UINT ret;

    TRACE("device %p, command %u, data %p, data_size %p.\n",
            device, command, data, data_size);

    ret = GetRawInputDeviceInfoW(device, command, data, data_size);
    if (command == RIDI_DEVICENAME && ret && ret != ~0U)
        ret = WideCharToMultiByte(CP_ACP, 0, data, -1, data, *data_size, NULL, NULL);

    return ret;
}

/***********************************************************************
 *              GetRawInputDeviceInfoW   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceInfoW(HANDLE device, UINT command, void *data, UINT *data_size)
{
    /* FIXME: Most of this is made up. */
    static const WCHAR keyboard_name[] = {'\\','\\','?','\\','W','I','N','E','_','K','E','Y','B','O','A','R','D',0};
    static const WCHAR mouse_name[] = {'\\','\\','?','\\','W','I','N','E','_','M','O','U','S','E',0};
    static const RID_DEVICE_INFO_KEYBOARD keyboard_info = {0, 0, 1, 12, 3, 101};
    static const RID_DEVICE_INFO_MOUSE mouse_info = {1, 5, 0, FALSE};
    const WCHAR *name = NULL;
    RID_DEVICE_INFO *info;
    UINT s;

    TRACE("device %p, command %u, data %p, data_size %p.\n",
            device, command, data, data_size);

    if (!data_size || (device != WINE_MOUSE_HANDLE && device != WINE_KEYBOARD_HANDLE)) return ~0U;

    switch (command)
    {
    case RIDI_DEVICENAME:
        if (device == WINE_MOUSE_HANDLE)
        {
            s = sizeof(mouse_name);
            name = mouse_name;
        }
        else
        {
            s = sizeof(keyboard_name);
            name = keyboard_name;
        }
        break;
    case RIDI_DEVICEINFO:
        s = sizeof(*info);
        break;
    default:
        return ~0U;
    }

    if (!data)
    {
        *data_size = s;
        return 0;
    }

    if (*data_size < s)
    {
        *data_size = s;
        return ~0U;
    }

    if (command == RIDI_DEVICENAME)
    {
        memcpy(data, name, s);
        return s;
    }

    info = data;
    info->cbSize = sizeof(*info);
    if (device == WINE_MOUSE_HANDLE)
    {
        info->dwType = RIM_TYPEMOUSE;
        info->u.mouse = mouse_info;
    }
    else
    {
        info->dwType = RIM_TYPEKEYBOARD;
        info->u.keyboard = keyboard_info;
    }
    return s;
}

/***********************************************************************
 *              GetRegisteredRawInputDevices   (USER32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetRegisteredRawInputDevices(RAWINPUTDEVICE *devices, UINT *device_count, UINT size)
{
    FIXME("devices %p, device_count %p, size %u stub!\n", devices, device_count, size);

    return 0;
}


/***********************************************************************
 *              DefRawInputProc   (USER32.@)
 */
LRESULT WINAPI DefRawInputProc(RAWINPUT **data, INT data_count, UINT header_size)
{
    FIXME("data %p, data_count %d, header_size %u stub!\n", data, data_count, header_size);

    return 0;
}
