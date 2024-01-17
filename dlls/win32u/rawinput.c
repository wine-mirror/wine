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

#if 0
#pragma makedep unix
#endif

#include <stdbool.h>
#include <pthread.h>

#include "win32u_private.h"
#include "ntuser_private.h"
#define WIN32_NO_STATUS
#include "winioctl.h"
#include "ddk/hidclass.h"
#include "wine/hid.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rawinput);

#define WINE_MOUSE_HANDLE       ((HANDLE)1)
#define WINE_KEYBOARD_HANDLE    ((HANDLE)2)

#ifdef _WIN64
typedef RAWINPUTHEADER RAWINPUTHEADER64;
typedef RAWINPUT RAWINPUT64;
#else
typedef struct
{
    DWORD dwType;
    DWORD dwSize;
    ULONGLONG hDevice;
    ULONGLONG wParam;
} RAWINPUTHEADER64;

typedef struct
{
    RAWINPUTHEADER64 header;
    union
    {
        RAWMOUSE    mouse;
        RAWKEYBOARD keyboard;
        RAWHID      hid;
    } data;
} RAWINPUT64;
#endif

static struct rawinput_thread_data *get_rawinput_thread_data(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct rawinput_thread_data *data = thread_info->rawinput;
    if (data) return data;
    data = thread_info->rawinput = calloc( 1, RAWINPUT_BUFFER_SIZE + sizeof(struct user_thread_info) );
    return data;
}

static bool rawinput_from_hardware_message( RAWINPUT *rawinput, const struct hardware_msg_data *msg_data )
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
        rawinput->data.keyboard.Flags    = (msg_data->flags & KEYEVENTF_KEYUP) ? RI_KEY_BREAK : RI_KEY_MAKE;
        if (msg_data->flags & KEYEVENTF_EXTENDEDKEY)
            rawinput->data.keyboard.Flags |= RI_KEY_E0;
        rawinput->data.keyboard.Reserved = 0;

        switch (msg_data->rawinput.kbd.vkey)
        {
        case VK_LSHIFT:
        case VK_RSHIFT:
            rawinput->data.keyboard.VKey = VK_SHIFT;
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
        if (size > rawinput->header.dwSize - sizeof(*rawinput)) return false;

        rawinput->header.dwSize  = FIELD_OFFSET( RAWINPUT, data.hid.bRawData ) + size;
        rawinput->header.hDevice = ULongToHandle( msg_data->rawinput.hid.device );
        rawinput->header.wParam  = 0;

        rawinput->data.hid.dwCount = msg_data->rawinput.hid.count;
        rawinput->data.hid.dwSizeHid = msg_data->rawinput.hid.length;
        memcpy( rawinput->data.hid.bRawData, msg_data + 1, size );
    }
    else
    {
        FIXME( "Unhandled rawinput type %#x.\n", msg_data->rawinput.type );
        return false;
    }

    return true;
}

struct device
{
    HANDLE file;
    HANDLE handle;
    struct list entry;
    WCHAR path[MAX_PATH];
    RID_DEVICE_INFO info;
    struct hid_preparsed_data *data;
};

static RAWINPUTDEVICE *registered_devices;
static unsigned int registered_device_count;
static struct list devices = LIST_INIT( devices );
static pthread_mutex_t rawinput_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct device *add_device( HKEY key, DWORD type )
{
    static const WCHAR symbolic_linkW[] = {'S','y','m','b','o','l','i','c','L','i','n','k',0};
    char value_buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[MAX_PATH * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *value = (KEY_VALUE_PARTIAL_INFORMATION *)value_buffer;
    static const RID_DEVICE_INFO_KEYBOARD keyboard_info = {0, 0, 1, 12, 3, 101};
    static const RID_DEVICE_INFO_MOUSE mouse_info = {1, 5, 0, FALSE};
    struct hid_preparsed_data *preparsed = NULL;
    HID_COLLECTION_INFORMATION hid_info;
    IO_STATUS_BLOCK io = {{0}};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    struct device *device;
    RID_DEVICE_INFO info;
    unsigned int status;
    UINT32 handle;
    void *buffer;
    SIZE_T size;
    HANDLE file;
    WCHAR *path;
    unsigned int i = 0;

    if (!query_reg_value( key, symbolic_linkW, value, sizeof(value_buffer) - sizeof(WCHAR) ))
    {
        ERR( "failed to get symbolic link value\n" );
        return NULL;
    }
    memset( value->Data + value->DataLength, 0, sizeof(WCHAR) );

    /* upper case everything until the instance ID */
    for (path = (WCHAR *)value->Data; *path && i < 2; path++)
    {
        if (*path == '#') ++i;
        *path = towupper( *path );
    }
    path = (WCHAR *)value->Data;

    /* path is in DOS format and begins with \\?\ prefix */
    path[1] = '?';

    RtlInitUnicodeString( &string, path );
    InitializeObjectAttributes( &attr, &string, OBJ_CASE_INSENSITIVE, NULL, NULL );
    if ((status = NtOpenFile( &file, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &attr, &io,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT )))
    {
        WARN( "Failed to open device file %s, status %#x.\n", debugstr_w(path), status );
        return NULL;
    }

    path[1] = '\\';

    status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io, IOCTL_HID_GET_WINE_RAWINPUT_HANDLE,
                                    NULL, 0, &handle, sizeof(handle) );
    if (status)
    {
        ERR( "Failed to get raw input handle, status %#x.\n", status );
        goto fail;
    }

    LIST_FOR_EACH_ENTRY( device, &devices, struct device, entry )
    {
        if (device->handle == UlongToHandle( handle ))
        {
            TRACE( "Ignoring already added device %#x / %s.\n", handle, debugstr_w(path) );
            goto fail;
        }
    }

    memset( &info, 0, sizeof(info) );
    info.cbSize = sizeof(info);
    info.dwType = type;

    switch (type)
    {
    case RIM_TYPEHID:
        status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
                                        IOCTL_HID_GET_COLLECTION_INFORMATION,
                                        NULL, 0, &hid_info, sizeof(hid_info) );
        if (status)
        {
            ERR( "Failed to get collection information, status %#x.\n", status );
            goto fail;
        }

        info.hid.dwVendorId = hid_info.VendorID;
        info.hid.dwProductId = hid_info.ProductID;
        info.hid.dwVersionNumber = hid_info.VersionNumber;

        if (!(preparsed = malloc( hid_info.DescriptorSize )))
        {
            ERR( "Failed to allocate memory.\n" );
            goto fail;
        }

        /* NtDeviceIoControlFile checks that the output buffer is writable using ntdll virtual
         * memory protection information, we need an NtAllocateVirtualMemory allocated buffer.
         */
        buffer = NULL;
        size = hid_info.DescriptorSize;
        if (!(status = NtAllocateVirtualMemory( GetCurrentProcess(), &buffer, 0, &size,
                                                MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE )))
        {
            size = 0;
            status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
                                            IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                            NULL, 0, buffer, hid_info.DescriptorSize );
            if (!status) memcpy( preparsed, buffer, hid_info.DescriptorSize );
            NtFreeVirtualMemory( GetCurrentProcess(), &buffer, &size, MEM_RELEASE );
        }

        if (status)
        {
            ERR( "Failed to get collection descriptor, status %#x.\n", status );
            goto fail;
        }

        info.hid.usUsagePage = preparsed->usage_page;
        info.hid.usUsage = preparsed->usage;
        break;

    case RIM_TYPEMOUSE:
        info.mouse = mouse_info;
        break;

    case RIM_TYPEKEYBOARD:
        info.keyboard = keyboard_info;
        break;
    }

    if (!(device = calloc( 1, sizeof(*device) )))
    {
        ERR( "Failed to allocate memory.\n" );
        goto fail;
    }

    TRACE( "Adding device %#x / %s.\n", handle, debugstr_w(path) );
    wcscpy( device->path, path );
    device->file = file;
    device->handle = ULongToHandle(handle);
    device->info = info;
    device->data = preparsed;
    list_add_tail( &devices, &device->entry );

    return device;

fail:
    free( preparsed );
    NtClose( file );
    return NULL;
}

static const WCHAR device_classesW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','C','o','n','t','r','o','l',
    '\\','D','e','v','i','c','e','C','l','a','s','s','e','s','\\',0
};
static const WCHAR guid_devinterface_hidW[] =
{
    '{','4','d','1','e','5','5','b','2','-','f','1','6','f','-','1','1','c','f',
    '-','8','8','c','b','-','0','0','1','1','1','1','0','0','0','0','3','0','}',0
};
static const WCHAR guid_devinterface_keyboardW[] =
{
    '{','8','8','4','b','9','6','c','3','-','5','6','e','f','-','1','1','d','1',
    '-','b','c','8','c','-','0','0','a','0','c','9','1','4','0','5','d','d','}',0
};
static const WCHAR guid_devinterface_mouseW[] =
{
    '{','3','7','8','d','e','4','4','c','-','5','6','e','f','-','1','1','d','1',
    '-','b','c','8','c','-','0','0','a','0','c','9','1','4','0','5','d','d','}',0
};

static void enumerate_devices( DWORD type, const WCHAR *class )
{
    WCHAR buffer[1024];
    KEY_NODE_INFORMATION *subkey_info = (void *)buffer;
    HKEY class_key, device_key, iface_key;
    unsigned int i, j;
    DWORD size;

    wcscpy( buffer, device_classesW );
    wcscat( buffer, class );
    if (!(class_key = reg_open_key( NULL, buffer, wcslen( buffer ) * sizeof(WCHAR) )))
        return;

    for (i = 0; !NtEnumerateKey( class_key, i, KeyNodeInformation, buffer, sizeof(buffer), &size ); ++i)
    {
        if (!(device_key = reg_open_key( class_key, subkey_info->Name, subkey_info->NameLength )))
        {
            ERR( "failed to open %s\n", debugstr_wn(subkey_info->Name, subkey_info->NameLength / sizeof(WCHAR)) );
            continue;
        }

        for (j = 0; !NtEnumerateKey( device_key, j, KeyNodeInformation, buffer, sizeof(buffer), &size ); ++j)
        {
            if (!(iface_key = reg_open_key( device_key, subkey_info->Name, subkey_info->NameLength )))
            {
                ERR( "failed to open %s\n", debugstr_wn(subkey_info->Name, subkey_info->NameLength / sizeof(WCHAR)) );
                continue;
            }

            add_device( iface_key, type );
            NtClose( iface_key );
        }

        NtClose( device_key );
    }

    NtClose( class_key );
}

static void rawinput_update_device_list(void)
{
    struct device *device, *next;

    TRACE( "\n" );

    LIST_FOR_EACH_ENTRY_SAFE( device, next, &devices, struct device, entry )
    {
        list_remove( &device->entry );
        NtClose( device->file );
        free( device->data );
        free( device );
    }

    enumerate_devices( RIM_TYPEMOUSE, guid_devinterface_mouseW );
    enumerate_devices( RIM_TYPEKEYBOARD, guid_devinterface_keyboardW );
    enumerate_devices( RIM_TYPEHID, guid_devinterface_hidW );
}

static struct device *find_device_from_handle( HANDLE handle )
{
    struct device *device;

    LIST_FOR_EACH_ENTRY( device, &devices, struct device, entry )
        if (device->handle == handle) return device;

    rawinput_update_device_list();

    LIST_FOR_EACH_ENTRY( device, &devices, struct device, entry )
        if (device->handle == handle) return device;

    return NULL;
}

BOOL rawinput_device_get_usages( HANDLE handle, USAGE *usage_page, USAGE *usage )
{
    struct device *device;

    pthread_mutex_lock( &rawinput_mutex );

    if (!(device = find_device_from_handle( handle )) || device->info.dwType != RIM_TYPEHID)
        *usage_page = *usage = 0;
    else
    {
        *usage_page = device->info.hid.usUsagePage;
        *usage = device->info.hid.usUsage;
    }

    pthread_mutex_unlock( &rawinput_mutex );

    return *usage_page || *usage;
}

/**********************************************************************
 *         NtUserGetRawInputDeviceList   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputDeviceList( RAWINPUTDEVICELIST *device_list, UINT *device_count, UINT size )
{
    unsigned int count = 0, ticks = NtGetTickCount();
    static unsigned int last_check;
    struct device *device;

    TRACE( "device_list %p, device_count %p, size %u.\n", device_list, device_count, size );

    if (size != sizeof(*device_list))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (!device_count)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return ~0u;
    }

    pthread_mutex_lock( &rawinput_mutex );

    if (ticks - last_check > 2000)
    {
        last_check = ticks;
        rawinput_update_device_list();
    }

    LIST_FOR_EACH_ENTRY( device, &devices, struct device, entry )
    {
        if (*device_count < ++count || !device_list) continue;
        device_list->hDevice = device->handle;
        device_list->dwType = device->info.dwType;
        device_list++;
    }

    pthread_mutex_unlock( &rawinput_mutex );

    if (!device_list)
    {
        *device_count = count;
        return 0;
    }

    if (*device_count < count)
    {
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        *device_count = count;
        return ~0u;
    }

    return count;
}

/**********************************************************************
 *         NtUserGetRawInputDeviceInfo   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputDeviceInfo( HANDLE handle, UINT command, void *data, UINT *data_size )
{
    const struct hid_preparsed_data *preparsed;
    struct device *device;
    RID_DEVICE_INFO info;
    DWORD len, data_len;

    TRACE( "handle %p, command %#x, data %p, data_size %p.\n", handle, command, data, data_size );

    if (!data_size)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return ~0u;
    }
    if (command != RIDI_DEVICENAME && command != RIDI_DEVICEINFO && command != RIDI_PREPARSEDDATA)
    {
        FIXME( "Command %#x not implemented!\n", command );
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    pthread_mutex_lock( &rawinput_mutex );

    if (!(device = find_device_from_handle( handle )))
    {
        pthread_mutex_unlock( &rawinput_mutex );
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return ~0u;
    }

    len = data_len = *data_size;
    switch (command)
    {
    case RIDI_DEVICENAME:
        if ((len = wcslen( device->path ) + 1) <= data_len && data)
            memcpy( data, device->path, len * sizeof(WCHAR) );
        *data_size = len;
        break;

    case RIDI_DEVICEINFO:
        if ((len = sizeof(info)) <= data_len && data)
            memcpy( data, &device->info, len );
        *data_size = len;
        break;

    case RIDI_PREPARSEDDATA:
        if (!(preparsed = device->data))
            len = 0;
        else
            len = preparsed->caps_size + FIELD_OFFSET(struct hid_preparsed_data, value_caps[0]) +
                  preparsed->number_link_collection_nodes * sizeof(struct hid_collection_node);

        if (preparsed && len <= data_len && data)
            memcpy( data, preparsed, len );
        *data_size = len;
        break;
    }

    pthread_mutex_unlock( &rawinput_mutex );

    if (!data)
        return 0;

    if (data_len < len)
    {
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        return ~0u;
    }

    return *data_size;
}

/**********************************************************************
 *         NtUserGetRawInputBuffer   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputBuffer( RAWINPUT *data, UINT *data_size, UINT header_size )
{
    unsigned int count = 0, remaining, rawinput_size, next_size, overhead;
    struct rawinput_thread_data *thread_data;
    struct hardware_msg_data *msg_data;
    RAWINPUT *rawinput;
    int i;

    if (NtCurrentTeb()->WowTebOffset)
        rawinput_size = sizeof(RAWINPUT64);
    else
        rawinput_size = sizeof(RAWINPUT);
    overhead = rawinput_size - sizeof(RAWINPUT);

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN( "Invalid structure size %u.\n", header_size );
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (!data_size)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (!data)
    {
        TRACE( "data %p, data_size %p (%u), header_size %u\n", data, data_size, *data_size, header_size );
        SERVER_START_REQ( get_rawinput_buffer )
        {
            req->rawinput_size = rawinput_size;
            req->buffer_size = 0;
            if (wine_server_call( req )) return ~0u;
            *data_size = reply->next_size;
        }
        SERVER_END_REQ;
        return 0;
    }

    if (!(thread_data = get_rawinput_thread_data())) return ~0u;
    rawinput = thread_data->buffer;

    /* first RAWINPUT block in the buffer is used for WM_INPUT message data */
    msg_data = (struct hardware_msg_data *)NEXTRAWINPUTBLOCK(rawinput);
    SERVER_START_REQ( get_rawinput_buffer )
    {
        req->rawinput_size = rawinput_size;
        req->buffer_size = *data_size;
        wine_server_set_reply( req, msg_data, RAWINPUT_BUFFER_SIZE - rawinput->header.dwSize );
        if (wine_server_call( req )) return ~0u;
        next_size = reply->next_size;
        count = reply->count;
    }
    SERVER_END_REQ;

    remaining = *data_size;
    for (i = 0; i < count; ++i)
    {
        data->header.dwSize = remaining;
        if (!rawinput_from_hardware_message( data, msg_data )) break;
        if (overhead)
        {
            /* Under WoW64, GetRawInputBuffer always gives 64-bit RAWINPUT structs. */
            RAWINPUT64 *ri64 = (RAWINPUT64 *)data;
            memmove( (char *)&data->data + overhead, &data->data,
                     data->header.dwSize - sizeof(RAWINPUTHEADER) );
            ri64->header.dwSize += overhead;

            /* Need to copy wParam before hDevice so it's not overwritten. */
            ri64->header.wParam = data->header.wParam;
#ifdef _WIN64
            ri64->header.hDevice = data->header.hDevice;
#else
            ri64->header.hDevice = HandleToULong(data->header.hDevice);
#endif
        }
        remaining -= data->header.dwSize;
        data = NEXTRAWINPUTBLOCK(data);
        msg_data = (struct hardware_msg_data *)((char *)msg_data + msg_data->size);
    }

    if (!next_size)
    {
        if (!count)
            *data_size = 0;
        else
            next_size = rawinput_size;
    }

    if (next_size && *data_size <= next_size)
    {
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        *data_size = next_size;
        count = ~0u;
    }

    TRACE( "data %p, data_size %p (%u), header_size %u, count %u\n",
           data, data_size, *data_size, header_size, count );
    return count;
}

/**********************************************************************
 *         NtUserGetRawInputData   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputData( HRAWINPUT rawinput, UINT command, void *data, UINT *data_size, UINT header_size )
{
    struct rawinput_thread_data *thread_data;
    UINT size;

    TRACE( "rawinput %p, command %#x, data %p, data_size %p, header_size %u.\n",
           rawinput, command, data, data_size, header_size );

    if (!(thread_data = get_rawinput_thread_data()))
    {
        RtlSetLastWin32Error( ERROR_OUTOFMEMORY );
        return ~0u;
    }

    if (!rawinput || thread_data->hw_id != (UINT_PTR)rawinput)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return ~0u;
    }

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN( "Invalid structure size %u.\n", header_size );
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return ~0u;
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
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (!data)
    {
        *data_size = size;
        return 0;
    }

    if (*data_size < size)
    {
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        return ~0u;
    }
    memcpy( data, thread_data->buffer, size );
    return size;
}

BOOL process_rawinput_message( MSG *msg, UINT hw_id, const struct hardware_msg_data *msg_data )
{
    struct rawinput_thread_data *thread_data;

    if (!(thread_data = get_rawinput_thread_data()))
        return FALSE;

    if (msg->message == WM_INPUT_DEVICE_CHANGE)
    {
        pthread_mutex_lock( &rawinput_mutex );
        rawinput_update_device_list();
        pthread_mutex_unlock( &rawinput_mutex );
    }
    else
    {
        thread_data->buffer->header.dwSize = RAWINPUT_BUFFER_SIZE;
        if (!rawinput_from_hardware_message( thread_data->buffer, msg_data )) return FALSE;
        thread_data->hw_id = hw_id;
        msg->lParam = (LPARAM)hw_id;
    }

    msg->pt = point_phys_to_win_dpi( msg->hwnd, msg->pt );
    return TRUE;
}

static void register_rawinput_device( const RAWINPUTDEVICE *device )
{
    RAWINPUTDEVICE *pos, *end;

    for (pos = registered_devices, end = pos + registered_device_count; pos != end; pos++)
    {
        if (pos->usUsagePage < device->usUsagePage) continue;
        if (pos->usUsagePage > device->usUsagePage) break;
        if (pos->usUsage >= device->usUsage) break;
    }

    if (device->dwFlags & RIDEV_REMOVE)
    {
        if (pos != end && pos->usUsagePage == device->usUsagePage && pos->usUsage == device->usUsage)
        {
            memmove( pos, pos + 1, (char *)end - (char *)(pos + 1) );
            registered_device_count--;
        }
    }
    else
    {
        if (pos == end || pos->usUsagePage != device->usUsagePage || pos->usUsage != device->usUsage)
        {
            memmove( pos + 1, pos, (char *)end - (char *)pos );
            registered_device_count++;
        }
        *pos = *device;
    }
}

/**********************************************************************
 *         NtUserRegisterRawInputDevices   (win32u.@)
 */
BOOL WINAPI NtUserRegisterRawInputDevices( const RAWINPUTDEVICE *devices, UINT device_count, UINT device_size )
{
    struct rawinput_device *server_devices;
    RAWINPUTDEVICE *new_registered_devices;
    SIZE_T size;
    BOOL ret;
    UINT i;

    TRACE( "devices %p, device_count %u, device_size %u.\n", devices, device_count, device_size );

    if (device_size != sizeof(RAWINPUTDEVICE))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (i = 0; i < device_count; ++i)
    {
        TRACE( "device %u: page %#x, usage %#x, flags %#x, target %p.\n", i, devices[i].usUsagePage,
               devices[i].usUsage, (int)devices[i].dwFlags, devices[i].hwndTarget );

        if ((devices[i].dwFlags & RIDEV_INPUTSINK) && !devices[i].hwndTarget)
        {
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if ((devices[i].dwFlags & RIDEV_REMOVE) && devices[i].hwndTarget)
        {
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if (devices[i].dwFlags & ~(RIDEV_REMOVE|RIDEV_NOLEGACY|RIDEV_INPUTSINK|RIDEV_DEVNOTIFY))
            FIXME( "Unhandled flags %#x for device %u.\n", (int)devices[i].dwFlags, i );
    }

    pthread_mutex_lock( &rawinput_mutex );

    if (!registered_device_count && !device_count)
    {
        pthread_mutex_unlock( &rawinput_mutex );
        return TRUE;
    }

    size = (SIZE_T)device_size * (registered_device_count + device_count);
    if (!(new_registered_devices = realloc( registered_devices, size )))
    {
        pthread_mutex_unlock( &rawinput_mutex );
        RtlSetLastWin32Error( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    registered_devices = new_registered_devices;
    for (i = 0; i < device_count; ++i) register_rawinput_device( devices + i );

    if (!(device_count = registered_device_count)) server_devices = NULL;
    else if (!(server_devices = malloc( device_count * sizeof(*server_devices) )))
    {
        pthread_mutex_unlock( &rawinput_mutex );
        RtlSetLastWin32Error( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    for (i = 0; i < device_count; ++i)
    {
        server_devices[i].usage_page = registered_devices[i].usUsagePage;
        server_devices[i].usage = registered_devices[i].usUsage;
        server_devices[i].flags = registered_devices[i].dwFlags;
        server_devices[i].target = wine_server_user_handle( registered_devices[i].hwndTarget );
    }

    SERVER_START_REQ( update_rawinput_devices )
    {
        wine_server_add_data( req, server_devices, device_count * sizeof(*server_devices) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    free( server_devices );

    pthread_mutex_unlock( &rawinput_mutex );

    return ret;
}

/**********************************************************************
 *         NtUserGetRegisteredRawInputDevices   (win32u.@)
 */
UINT WINAPI NtUserGetRegisteredRawInputDevices( RAWINPUTDEVICE *devices, UINT *device_count, UINT device_size )
{
    SIZE_T size, capacity;

    TRACE( "devices %p, device_count %p, device_size %u\n", devices, device_count, device_size );

    if (device_size != sizeof(RAWINPUTDEVICE) || !device_count || (devices && !*device_count))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    pthread_mutex_lock( &rawinput_mutex );

    capacity = *device_count * device_size;
    *device_count = registered_device_count;
    size = (SIZE_T)device_size * *device_count;
    if (devices && capacity >= size) memcpy( devices, registered_devices, size );

    pthread_mutex_unlock( &rawinput_mutex );

    if (!devices) return 0;

    if (capacity < size)
    {
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        return ~0u;
    }

    return *device_count;
}
