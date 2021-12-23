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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
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
#include "wine/hid.h"

#include "user_private.h"

#include "initguid.h"
#include "devpkey.h"
#include "ntddmou.h"
#include "ntddkbd.h"

WINE_DEFAULT_DEBUG_CHANNEL(rawinput);

DEFINE_DEVPROPKEY(DEVPROPKEY_HID_HANDLE, 0xbc62e415, 0xf4fe, 0x405c, 0x8e, 0xda, 0x63, 0x6f, 0xb5, 0x9f, 0x08, 0x98, 2);

struct device
{
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail;
    HANDLE file;
    HANDLE handle;
    RID_DEVICE_INFO info;
    PHIDP_PREPARSED_DATA data;
};

static struct device *rawinput_devices;
static unsigned int rawinput_devices_count, rawinput_devices_max;

static CRITICAL_SECTION rawinput_devices_cs;
static CRITICAL_SECTION_DEBUG rawinput_devices_cs_debug =
{
    0, 0, &rawinput_devices_cs,
    { &rawinput_devices_cs_debug.ProcessLocksList, &rawinput_devices_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": rawinput_devices_cs") }
};
static CRITICAL_SECTION rawinput_devices_cs = { &rawinput_devices_cs_debug, -1, 0, 0, 0, 0 };

static BOOL array_reserve(void **elements, unsigned int *capacity, unsigned int count, unsigned int size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

static struct device *add_device(HDEVINFO set, SP_DEVICE_INTERFACE_DATA *iface)
{
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail;
    struct device *device = NULL;
    UINT32 handle;
    HANDLE file;
    WCHAR *pos;
    DWORD i, size, type;

    SetupDiGetDeviceInterfaceDetailW(set, iface, NULL, 0, &size, &device_data);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ERR("Failed to get device path, error %#x.\n", GetLastError());
        return FALSE;
    }

    if (!SetupDiGetDevicePropertyW(set, &device_data, &DEVPROPKEY_HID_HANDLE, &type, (BYTE *)&handle, sizeof(handle), NULL, 0) ||
        type != DEVPROP_TYPE_UINT32)
    {
        ERR("Failed to get device handle, error %#x.\n", GetLastError());
        return NULL;
    }

    if (!(detail = malloc(size)))
    {
        ERR("Failed to allocate memory.\n");
        return FALSE;
    }
    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    SetupDiGetDeviceInterfaceDetailW(set, iface, detail, size, NULL, NULL);

    /* upper case everything but the GUID */
    for (pos = detail->DevicePath; *pos && *pos != '{'; pos++) *pos = towupper(*pos);

    file = CreateFileW(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
    {
        ERR("Failed to open device file %s, error %u.\n", debugstr_w(detail->DevicePath), GetLastError());
        free(detail);
        return NULL;
    }

    for (i = 0; i < rawinput_devices_count && !device; ++i)
        if (rawinput_devices[i].handle == UlongToHandle(handle))
            device = rawinput_devices + i;

    if (device)
    {
        TRACE("Updating device %x / %s.\n", handle, debugstr_w(detail->DevicePath));
        HidD_FreePreparsedData(device->data);
        CloseHandle(device->file);
        free(device->detail);
    }
    else if (array_reserve((void **)&rawinput_devices, &rawinput_devices_max,
                           rawinput_devices_count + 1, sizeof(*rawinput_devices)))
    {
        device = &rawinput_devices[rawinput_devices_count++];
        TRACE("Adding device %x / %s.\n", handle, debugstr_w(detail->DevicePath));
    }
    else
    {
        ERR("Failed to allocate memory.\n");
        CloseHandle(file);
        free(detail);
        return NULL;
    }

    device->detail = detail;
    device->file = file;
    device->handle = ULongToHandle(handle);
    device->info.cbSize = sizeof(RID_DEVICE_INFO);
    device->data = NULL;

    return device;
}

void rawinput_update_device_list(void)
{
    SP_DEVICE_INTERFACE_DATA iface = { sizeof(iface) };
    struct device *device;
    HIDD_ATTRIBUTES attr;
    HIDP_CAPS caps;
    GUID hid_guid;
    HDEVINFO set;
    DWORD idx;

    TRACE("\n");

    HidD_GetHidGuid(&hid_guid);

    EnterCriticalSection(&rawinput_devices_cs);

    /* destroy previous list */
    for (idx = 0; idx < rawinput_devices_count; ++idx)
    {
        HidD_FreePreparsedData(rawinput_devices[idx].data);
        CloseHandle(rawinput_devices[idx].file);
        free(rawinput_devices[idx].detail);
    }
    rawinput_devices_count = 0;

    set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    for (idx = 0; SetupDiEnumDeviceInterfaces(set, NULL, &hid_guid, idx, &iface); ++idx)
    {
        if (!(device = add_device(set, &iface)))
            continue;

        attr.Size = sizeof(HIDD_ATTRIBUTES);
        if (!HidD_GetAttributes(device->file, &attr))
            WARN("Failed to get attributes.\n");

        device->info.dwType = RIM_TYPEHID;
        device->info.hid.dwVendorId = attr.VendorID;
        device->info.hid.dwProductId = attr.ProductID;
        device->info.hid.dwVersionNumber = attr.VersionNumber;

        if (!HidD_GetPreparsedData(device->file, &device->data))
            WARN("Failed to get preparsed data.\n");

        if (!HidP_GetCaps(device->data, &caps))
            WARN("Failed to get caps.\n");

        device->info.hid.usUsagePage = caps.UsagePage;
        device->info.hid.usUsage = caps.Usage;
    }

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MOUSE, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    for (idx = 0; SetupDiEnumDeviceInterfaces(set, NULL, &GUID_DEVINTERFACE_MOUSE, idx, &iface); ++idx)
    {
        static const RID_DEVICE_INFO_MOUSE mouse_info = {1, 5, 0, FALSE};

        if (!(device = add_device(set, &iface)))
            continue;

        device->info.dwType = RIM_TYPEMOUSE;
        device->info.mouse = mouse_info;
    }

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_KEYBOARD, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    for (idx = 0; SetupDiEnumDeviceInterfaces(set, NULL, &GUID_DEVINTERFACE_KEYBOARD, idx, &iface); ++idx)
    {
        static const RID_DEVICE_INFO_KEYBOARD keyboard_info = {0, 0, 1, 12, 3, 101};

        if (!(device = add_device(set, &iface)))
            continue;

        device->info.dwType = RIM_TYPEKEYBOARD;
        device->info.keyboard = keyboard_info;
    }

    SetupDiDestroyDeviceInfoList(set);

    LeaveCriticalSection(&rawinput_devices_cs);
}


static struct device *find_device_from_handle(HANDLE handle)
{
    UINT i;
    for (i = 0; i < rawinput_devices_count; ++i)
        if (rawinput_devices[i].handle == handle)
            return rawinput_devices + i;
    rawinput_update_device_list();
    for (i = 0; i < rawinput_devices_count; ++i)
        if (rawinput_devices[i].handle == handle)
            return rawinput_devices + i;
    return NULL;
}


BOOL rawinput_device_get_usages(HANDLE handle, USAGE *usage_page, USAGE *usage)
{
    struct device *device;

    *usage_page = *usage = 0;

    if (!(device = find_device_from_handle(handle))) return FALSE;
    if (device->info.dwType != RIM_TYPEHID) return FALSE;

    *usage_page = device->info.hid.usUsagePage;
    *usage = device->info.hid.usUsage;
    return TRUE;
}


struct rawinput_thread_data *rawinput_thread_data(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct rawinput_thread_data *data = thread_info->rawinput;
    if (data) return data;
    data = thread_info->rawinput = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                              RAWINPUT_BUFFER_SIZE + sizeof(struct user_thread_info) );
    return data;
}


BOOL rawinput_from_hardware_message(RAWINPUT *rawinput, const struct hardware_msg_data *msg_data)
{
    SIZE_T size;

    rawinput->header.dwType = msg_data->rawinput.type;
    if (msg_data->rawinput.type == RIM_TYPEMOUSE)
    {
        static const unsigned int button_flags[] =
        {
            0,                              /* MOUSEEVENTF_MOVE */
            RI_MOUSE_LEFT_BUTTON_DOWN,      /* MOUSEEVENTF_LEFTDOWN */
            RI_MOUSE_LEFT_BUTTON_UP,        /* MOUSEEVENTF_LEFTUP */
            RI_MOUSE_RIGHT_BUTTON_DOWN,     /* MOUSEEVENTF_RIGHTDOWN */
            RI_MOUSE_RIGHT_BUTTON_UP,       /* MOUSEEVENTF_RIGHTUP */
            RI_MOUSE_MIDDLE_BUTTON_DOWN,    /* MOUSEEVENTF_MIDDLEDOWN */
            RI_MOUSE_MIDDLE_BUTTON_UP,      /* MOUSEEVENTF_MIDDLEUP */
        };
        unsigned int i;

        rawinput->header.dwSize  = FIELD_OFFSET(RAWINPUT, data) + sizeof(RAWMOUSE);
        rawinput->header.hDevice = WINE_MOUSE_HANDLE;
        rawinput->header.wParam  = 0;

        rawinput->data.mouse.usFlags           = MOUSE_MOVE_RELATIVE;
        rawinput->data.mouse.usButtonFlags = 0;
        rawinput->data.mouse.usButtonData  = 0;
        for (i = 1; i < ARRAY_SIZE(button_flags); ++i)
        {
            if (msg_data->flags & (1 << i))
                rawinput->data.mouse.usButtonFlags |= button_flags[i];
        }
        if (msg_data->flags & MOUSEEVENTF_WHEEL)
        {
            rawinput->data.mouse.usButtonFlags |= RI_MOUSE_WHEEL;
            rawinput->data.mouse.usButtonData   = msg_data->rawinput.mouse.data;
        }
        if (msg_data->flags & MOUSEEVENTF_HWHEEL)
        {
            rawinput->data.mouse.usButtonFlags |= RI_MOUSE_HORIZONTAL_WHEEL;
            rawinput->data.mouse.usButtonData   = msg_data->rawinput.mouse.data;
        }
        if (msg_data->flags & MOUSEEVENTF_XDOWN)
        {
            if (msg_data->rawinput.mouse.data == XBUTTON1)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_4_DOWN;
            else if (msg_data->rawinput.mouse.data == XBUTTON2)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_5_DOWN;
        }
        if (msg_data->flags & MOUSEEVENTF_XUP)
        {
            if (msg_data->rawinput.mouse.data == XBUTTON1)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_4_UP;
            else if (msg_data->rawinput.mouse.data == XBUTTON2)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_5_UP;
        }

        rawinput->data.mouse.ulRawButtons       = 0;
        rawinput->data.mouse.lLastX             = msg_data->rawinput.mouse.x;
        rawinput->data.mouse.lLastY             = msg_data->rawinput.mouse.y;
        rawinput->data.mouse.ulExtraInformation = msg_data->info;
    }
    else if (msg_data->rawinput.type == RIM_TYPEKEYBOARD)
    {
        rawinput->header.dwSize  = FIELD_OFFSET(RAWINPUT, data) + sizeof(RAWKEYBOARD);
        rawinput->header.hDevice = WINE_KEYBOARD_HANDLE;
        rawinput->header.wParam  = 0;

        rawinput->data.keyboard.MakeCode = msg_data->rawinput.kbd.scan;
        rawinput->data.keyboard.Flags    = msg_data->flags & KEYEVENTF_KEYUP ? RI_KEY_BREAK : RI_KEY_MAKE;
        if (msg_data->flags & KEYEVENTF_EXTENDEDKEY) rawinput->data.keyboard.Flags |= RI_KEY_E0;
        rawinput->data.keyboard.Reserved = 0;

        switch (msg_data->rawinput.kbd.vkey)
        {
        case VK_LSHIFT:
        case VK_RSHIFT:
            rawinput->data.keyboard.VKey   = VK_SHIFT;
            rawinput->data.keyboard.Flags &= ~RI_KEY_E0;
            break;
        case VK_LCONTROL:
        case VK_RCONTROL:
            rawinput->data.keyboard.VKey = VK_CONTROL;
            break;
        case VK_LMENU:
        case VK_RMENU:
            rawinput->data.keyboard.VKey = VK_MENU;
            break;
        default:
            rawinput->data.keyboard.VKey = msg_data->rawinput.kbd.vkey;
            break;
        }

        rawinput->data.keyboard.Message          = msg_data->rawinput.kbd.message;
        rawinput->data.keyboard.ExtraInformation = msg_data->info;
    }
    else if (msg_data->rawinput.type == RIM_TYPEHID)
    {
        size = msg_data->size - sizeof(*msg_data);
        if (size > rawinput->header.dwSize - sizeof(*rawinput)) return FALSE;

        rawinput->header.dwSize  = FIELD_OFFSET( RAWINPUT, data.hid.bRawData ) + size;
        rawinput->header.hDevice = ULongToHandle( msg_data->rawinput.hid.device );
        rawinput->header.wParam  = 0;

        rawinput->data.hid.dwCount = msg_data->rawinput.hid.count;
        rawinput->data.hid.dwSizeHid = msg_data->rawinput.hid.length;
        memcpy( rawinput->data.hid.bRawData, msg_data + 1, size );
    }
    else
    {
        FIXME("Unhandled rawinput type %#x.\n", msg_data->rawinput.type);
        return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *              GetRawInputDeviceList   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceList(RAWINPUTDEVICELIST *devices, UINT *device_count, UINT size)
{
    static UINT last_check;
    UINT i, ticks = GetTickCount();

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

    if (ticks - last_check > 2000)
    {
        last_check = ticks;
        rawinput_update_device_list();
    }

    if (!devices)
    {
        *device_count = rawinput_devices_count;
        return 0;
    }

    if (*device_count < rawinput_devices_count)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *device_count = rawinput_devices_count;
        return ~0U;
    }

    for (i = 0; i < rawinput_devices_count; ++i)
    {
        devices[i].hDevice = rawinput_devices[i].handle;
        devices[i].dwType = rawinput_devices[i].info.dwType;
    }

    return rawinput_devices_count;
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
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < device_count; ++i)
    {
        if ((devices[i].dwFlags & RIDEV_INPUTSINK) &&
            (devices[i].hwndTarget == NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if ((devices[i].dwFlags & RIDEV_REMOVE) &&
            (devices[i].hwndTarget != NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    if (!(d = HeapAlloc( GetProcessHeap(), 0, device_count * sizeof(*d) ))) return FALSE;

    for (i = 0; i < device_count; ++i)
    {
        TRACE("device %u: page %#x, usage %#x, flags %#x, target %p.\n",
                i, devices[i].usUsagePage, devices[i].usUsage,
                devices[i].dwFlags, devices[i].hwndTarget);
        if (devices[i].dwFlags & ~(RIDEV_REMOVE|RIDEV_NOLEGACY|RIDEV_INPUTSINK|RIDEV_DEVNOTIFY))
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
    struct rawinput_thread_data *thread_data = rawinput_thread_data();
    UINT size;

    TRACE("rawinput %p, command %#x, data %p, data_size %p, header_size %u.\n",
            rawinput, command, data, data_size, header_size);

    if (!rawinput || thread_data->hw_id != (UINT_PTR)rawinput)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return ~0U;
    }

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN("Invalid structure size %u.\n", header_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0U;
    }

    switch (command)
    {
    case RID_INPUT:
        size = thread_data->buffer->header.dwSize;
        break;
    case RID_HEADER:
        size = sizeof(RAWINPUTHEADER);
        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0U;
    }

    if (!data)
    {
        *data_size = size;
        return 0;
    }

    if (*data_size < size)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return ~0U;
    }
    memcpy(data, thread_data->buffer, size);
    return size;
}

#ifdef _WIN64
typedef RAWINPUT RAWINPUT64;
#else
typedef struct
{
    RAWINPUTHEADER header;
    char pad[8];
    union {
        RAWMOUSE    mouse;
        RAWKEYBOARD keyboard;
        RAWHID      hid;
    } data;
} RAWINPUT64;
#endif

/***********************************************************************
 *              GetRawInputBuffer   (USER32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetRawInputBuffer(RAWINPUT *data, UINT *data_size, UINT header_size)
{
    struct hardware_msg_data *msg_data;
    struct rawinput_thread_data *thread_data;
    RAWINPUT *rawinput;
    UINT count = 0, remaining, rawinput_size, next_size, overhead;
    BOOL is_wow64;
    int i;

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
        rawinput_size = sizeof(RAWINPUT64);
    else
        rawinput_size = sizeof(RAWINPUT);
    overhead = rawinput_size - sizeof(RAWINPUT);

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN("Invalid structure size %u.\n", header_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0U;
    }

    if (!data_size)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0U;
    }

    if (!data)
    {
        TRACE("data %p, data_size %p (%u), header_size %u\n", data, data_size, *data_size, header_size);
        SERVER_START_REQ( get_rawinput_buffer )
        {
            req->rawinput_size = rawinput_size;
            req->buffer_size = 0;
            if (wine_server_call( req )) return ~0U;
            *data_size = reply->next_size;
        }
        SERVER_END_REQ;
        return 0;
    }

    if (!(thread_data = rawinput_thread_data())) return ~0U;
    rawinput = thread_data->buffer;

    /* first RAWINPUT block in the buffer is used for WM_INPUT message data */
    msg_data = (struct hardware_msg_data *)NEXTRAWINPUTBLOCK(rawinput);
    SERVER_START_REQ( get_rawinput_buffer )
    {
        req->rawinput_size = rawinput_size;
        req->buffer_size = *data_size;
        wine_server_set_reply( req, msg_data, RAWINPUT_BUFFER_SIZE - rawinput->header.dwSize );
        if (wine_server_call( req )) return ~0U;
        next_size = reply->next_size;
        count = reply->count;
    }
    SERVER_END_REQ;

    remaining = *data_size;
    for (i = 0; i < count; ++i)
    {
        data->header.dwSize = remaining;
        if (!rawinput_from_hardware_message(data, msg_data)) break;
        if (overhead) memmove((char *)&data->data + overhead, &data->data,
                              data->header.dwSize - sizeof(RAWINPUTHEADER));
        data->header.dwSize += overhead;
        remaining -= data->header.dwSize;
        data = NEXTRAWINPUTBLOCK(data);
        msg_data = (struct hardware_msg_data *)((char *)msg_data + msg_data->size);
    }

    if (count == 0 && next_size == 0) *data_size = 0;
    else if (next_size == 0) next_size = rawinput_size;

    if (next_size && *data_size <= next_size)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *data_size = next_size;
        count = ~0U;
    }

    if (count) TRACE("data %p, data_size %p (%u), header_size %u, count %u\n", data, data_size, *data_size, header_size, count);
    return count;
}

/***********************************************************************
 *              GetRawInputDeviceInfoA   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceInfoA(HANDLE device, UINT command, void *data, UINT *data_size)
{
    TRACE("device %p, command %#x, data %p, data_size %p.\n",
            device, command, data, data_size);

    /* RIDI_DEVICENAME data_size is in chars, not bytes */
    if (command == RIDI_DEVICENAME)
    {
        WCHAR *nameW;
        UINT ret, nameW_sz;

        if (!data_size) return ~0U;

        nameW_sz = *data_size;

        if (data && nameW_sz > 0)
            nameW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * nameW_sz);
        else
            nameW = NULL;

        ret = GetRawInputDeviceInfoW(device, command, nameW, &nameW_sz);

        if (ret && ret != ~0U)
            WideCharToMultiByte(CP_ACP, 0, nameW, -1, data, *data_size, NULL, NULL);

        *data_size = nameW_sz;

        HeapFree(GetProcessHeap(), 0, nameW);

        return ret;
    }

    return GetRawInputDeviceInfoW(device, command, data, data_size);
}

/***********************************************************************
 *              GetRawInputDeviceInfoW   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceInfoW(HANDLE handle, UINT command, void *data, UINT *data_size)
{
    struct hid_preparsed_data *preparsed;
    RID_DEVICE_INFO info;
    struct device *device;
    DWORD len, data_len;

    TRACE("handle %p, command %#x, data %p, data_size %p.\n",
            handle, command, data, data_size);

    if (!data_size)
    {
        SetLastError(ERROR_NOACCESS);
        return ~0U;
    }
    if (!(device = find_device_from_handle(handle)))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return ~0U;
    }

    data_len = *data_size;
    switch (command)
    {
    case RIDI_DEVICENAME:
        if ((len = wcslen(device->detail->DevicePath) + 1) <= data_len && data)
            memcpy(data, device->detail->DevicePath, len * sizeof(WCHAR));
        *data_size = len;
        break;

    case RIDI_DEVICEINFO:
        if ((len = sizeof(info)) <= data_len && data)
            memcpy(data, &device->info, len);
        *data_size = len;
        break;

    case RIDI_PREPARSEDDATA:
        if (!(preparsed = (struct hid_preparsed_data *)device->data)) len = 0;
        else len = preparsed->caps_size + FIELD_OFFSET(struct hid_preparsed_data, value_caps[0]) +
                   preparsed->number_link_collection_nodes * sizeof(struct hid_collection_node);

        if (device->data && len <= data_len && data)
            memcpy(data, device->data, len);
        *data_size = len;
        break;

    default:
        FIXME("command %#x not supported\n", command);
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0U;
    }

    if (!data)
        return 0;

    if (data_len < len)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return ~0U;
    }

    return *data_size;
}

static int __cdecl compare_raw_input_devices(const void *ap, const void *bp)
{
    const RAWINPUTDEVICE a = *(const RAWINPUTDEVICE *)ap;
    const RAWINPUTDEVICE b = *(const RAWINPUTDEVICE *)bp;

    if (a.usUsagePage != b.usUsagePage) return a.usUsagePage - b.usUsagePage;
    if (a.usUsage != b.usUsage) return a.usUsage - b.usUsage;
    return 0;
}

/***********************************************************************
 *              GetRegisteredRawInputDevices   (USER32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetRegisteredRawInputDevices(RAWINPUTDEVICE *devices, UINT *device_count, UINT size)
{
    struct rawinput_device *buffer = NULL;
    unsigned int i, status, count = ~0U, buffer_size;

    TRACE("devices %p, device_count %p, size %u\n", devices, device_count, size);

    if (size != sizeof(RAWINPUTDEVICE) || !device_count || (devices && !*device_count))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ~0U;
    }

    buffer_size = *device_count * sizeof(*buffer);
    if (devices && !(buffer = HeapAlloc(GetProcessHeap(), 0, buffer_size)))
        return ~0U;

    SERVER_START_REQ(get_rawinput_devices)
    {
        if (buffer) wine_server_set_reply(req, buffer, buffer_size);
        status = wine_server_call_err(req);
        *device_count = reply->device_count;
    }
    SERVER_END_REQ;

    if (buffer && !status)
    {
        for (i = 0, count = *device_count; i < count; ++i)
        {
            devices[i].usUsagePage = buffer[i].usage_page;
            devices[i].usUsage = buffer[i].usage;
            devices[i].dwFlags = buffer[i].flags;
            devices[i].hwndTarget = wine_server_ptr_handle(buffer[i].target);
        }

        qsort(devices, count, sizeof(*devices), compare_raw_input_devices);
    }

    if (buffer) HeapFree(GetProcessHeap(), 0, buffer);
    else count = 0;
    return count;
}


/***********************************************************************
 *              DefRawInputProc   (USER32.@)
 */
LRESULT WINAPI DefRawInputProc(RAWINPUT **data, INT data_count, UINT header_size)
{
    FIXME("data %p, data_count %d, header_size %u stub!\n", data, data_count, header_size);

    return 0;
}
