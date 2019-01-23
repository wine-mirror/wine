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
#include "winreg.h"
#include "winuser.h"
#include "setupapi.h"
#include "ddk/hidsdi.h"
#include "wine/debug.h"
#include "wine/server.h"

#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(rawinput);

struct hid_device
{
    WCHAR *path;
    HANDLE file;
    RID_DEVICE_INFO_HID info;
    PHIDP_PREPARSED_DATA data;
};

static struct hid_device *hid_devices;
static unsigned int hid_devices_count, hid_devices_max;

static CRITICAL_SECTION hid_devices_cs;
static CRITICAL_SECTION_DEBUG hid_devices_cs_debug =
{
    0, 0, &hid_devices_cs,
    { &hid_devices_cs_debug.ProcessLocksList, &hid_devices_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": hid_devices_cs") }
};
static CRITICAL_SECTION hid_devices_cs = { &hid_devices_cs_debug, -1, 0, 0, 0, 0 };

static void find_hid_devices(void)
{
    static ULONGLONG last_check;

    SP_DEVICE_INTERFACE_DATA iface = { sizeof(iface) };
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail;
    DWORD detail_size, needed;
    HIDD_ATTRIBUTES attr;
    DWORD idx, didx;
    HIDP_CAPS caps;
    GUID hid_guid;
    HDEVINFO set;
    HANDLE file;
    WCHAR *path;

    if (GetTickCount64() - last_check < 2000)
        return;
    last_check = GetTickCount64();

    HidD_GetHidGuid(&hid_guid);

    set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    detail_size = sizeof(*detail) + (MAX_PATH * sizeof(WCHAR));
    if (!(detail = heap_alloc(detail_size)))
        return;
    detail->cbSize = sizeof(*detail);

    EnterCriticalSection(&hid_devices_cs);

    /* destroy previous list */
    for (didx = 0; didx < hid_devices_count; ++didx)
    {
        CloseHandle(hid_devices[didx].file);
        heap_free(hid_devices[didx].path);
    }

    didx = 0;
    for (idx = 0; SetupDiEnumDeviceInterfaces(set, NULL, &hid_guid, idx, &iface); ++idx)
    {
        if (!SetupDiGetDeviceInterfaceDetailW(set, &iface, detail, detail_size, &needed, NULL))
        {
            if (!(detail = heap_realloc(detail, needed)))
            {
                ERR("Failed to allocate memory.\n");
                goto done;
            }
            detail_size = needed;

            SetupDiGetDeviceInterfaceDetailW(set, &iface, detail, detail_size, NULL, NULL);
        }

        if (!(path = heap_strdupW(detail->DevicePath)))
        {
            ERR("Failed to allocate memory.\n");
            goto done;
        }

        file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
        if (file == INVALID_HANDLE_VALUE)
        {
            ERR("Failed to open device file %s, error %u.\n", debugstr_w(path), GetLastError());
            heap_free(path);
            continue;
        }

        if (didx >= hid_devices_max)
        {
            if (hid_devices)
            {
                hid_devices_max *= 2;
                hid_devices = heap_realloc(hid_devices,
                    hid_devices_max * sizeof(hid_devices[0]));
            }
            else
            {
                hid_devices_max = 8;
                hid_devices = heap_alloc(hid_devices_max * sizeof(hid_devices[0]));
            }
            if (!hid_devices)
            {
                ERR("Failed to allocate memory.\n");
                goto done;
            }
        }

        TRACE("Found HID device %s.\n", debugstr_w(path));

        hid_devices[didx].path = path;
        hid_devices[didx].file = file;

        attr.Size = sizeof(HIDD_ATTRIBUTES);
        if (!HidD_GetAttributes(file, &attr))
            WARN_(rawinput)("Failed to get attributes.\n");
        hid_devices[didx].info.dwVendorId = attr.VendorID;
        hid_devices[didx].info.dwProductId = attr.ProductID;
        hid_devices[didx].info.dwVersionNumber = attr.VersionNumber;

        if (!HidD_GetPreparsedData(file, &hid_devices[didx].data))
            WARN_(rawinput)("Failed to get preparsed data.\n");

        if (!HidP_GetCaps(hid_devices[didx].data, &caps))
            WARN_(rawinput)("Failed to get caps.\n");

        hid_devices[didx].info.usUsagePage = caps.UsagePage;
        hid_devices[didx].info.usUsage = caps.Usage;

        didx++;
    }
    hid_devices_count = didx;

done:
    LeaveCriticalSection(&hid_devices_cs);
    SetupDiDestroyDeviceInfoList(set);
    heap_free(detail);
}

/***********************************************************************
 *              GetRawInputDeviceList   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceList(RAWINPUTDEVICELIST *devices, UINT *device_count, UINT size)
{
    UINT i;

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

    find_hid_devices();

    if (!devices)
    {
        *device_count = 2 + hid_devices_count;
        return 0;
    }

    if (*device_count < 2 + hid_devices_count)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *device_count = 2 + hid_devices_count;
        return ~0U;
    }

    devices[0].hDevice = WINE_MOUSE_HANDLE;
    devices[0].dwType = RIM_TYPEMOUSE;
    devices[1].hDevice = WINE_KEYBOARD_HANDLE;
    devices[1].dwType = RIM_TYPEKEYBOARD;

    for (i = 0; i < hid_devices_count; ++i)
    {
        devices[2 + i].hDevice = &hid_devices[i];
        devices[2 + i].dwType = RIM_TYPEHID;
    }

    return 2 + hid_devices_count;
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

    if (!ri)
        return ~0U;

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

    TRACE("device %p, command %#x, data %p, data_size %p.\n",
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
    struct hid_device *hid_device;
    const WCHAR *name = NULL;
    RID_DEVICE_INFO *info;
    UINT s;

    TRACE("device %p, command %#x, data %p, data_size %p.\n",
            device, command, data, data_size);

    if (!data_size) return ~0U;

    switch (command)
    {
    case RIDI_DEVICENAME:
        if (device == WINE_MOUSE_HANDLE)
        {
            s = sizeof(mouse_name);
            name = mouse_name;
        }
        else if (device == WINE_KEYBOARD_HANDLE)
        {
            s = sizeof(keyboard_name);
            name = keyboard_name;
        }
        else
        {
            hid_device = device;
            s = (strlenW(hid_device->path) + 1) * sizeof(WCHAR);
            name = hid_device->path;
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
    else if (device == WINE_KEYBOARD_HANDLE)
    {
        info->dwType = RIM_TYPEKEYBOARD;
        info->u.keyboard = keyboard_info;
    }
    else
    {
        hid_device = device;
        info->dwType = RIM_TYPEHID;
        info->u.hid = hid_device->info;
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
