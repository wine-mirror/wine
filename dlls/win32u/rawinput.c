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
#else
typedef struct
{
    DWORD dwType;
    DWORD dwSize;
    ULONGLONG hDevice;
    ULONGLONG wParam;
} RAWINPUTHEADER64;
#endif

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

static void rawinput_update_device_list( BOOL force )
{
    unsigned int ticks = NtGetTickCount();
    static unsigned int last_check;
    struct device *device, *next;

    TRACE( "\n" );

    if (ticks - last_check <= 2000 && !force) return;
    last_check = ticks;

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

static struct device *find_device_from_handle( HANDLE handle, BOOL refresh )
{
    struct device *device;

    LIST_FOR_EACH_ENTRY( device, &devices, struct device, entry )
        if (device->handle == handle) return device;
    if (!refresh) return NULL;

    rawinput_update_device_list( TRUE );

    LIST_FOR_EACH_ENTRY( device, &devices, struct device, entry )
        if (device->handle == handle) return device;

    return NULL;
}

/**********************************************************************
 *         NtUserGetRawInputDeviceList   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputDeviceList( RAWINPUTDEVICELIST *device_list, UINT *device_count, UINT size )
{
    unsigned int count = 0;
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

    rawinput_update_device_list( FALSE );

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

    if (!(device = find_device_from_handle( handle, TRUE )))
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
    struct user_thread_info *thread_info;
    UINT count;

    TRACE( "data %p, data_size %p, header_size %u\n", data, data_size, header_size );

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (!data_size)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return -1;
    }

    /* with old WOW64 mode we didn't go through the WOW64 thunks, patch the header size here */
    if (NtCurrentTeb()->WowTebOffset) header_size = sizeof(RAWINPUTHEADER64);

    thread_info = get_user_thread_info();
    SERVER_START_REQ( get_rawinput_buffer )
    {
        req->header_size = header_size;
        if ((req->read_data = !!data)) wine_server_set_reply( req, data, *data_size );
        if (!wine_server_call_err( req )) count = reply->count;
        else count = -1;
        *data_size = reply->next_size;
        if (reply->count) thread_info->client_info.message_time = reply->time;
    }
    SERVER_END_REQ;

    return count;
}

/**********************************************************************
 *         NtUserGetRawInputData   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputData( HRAWINPUT handle, UINT command, void *data, UINT *data_size, UINT header_size )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct hardware_msg_data *msg_data;
    RAWINPUT *rawinput = data;
    UINT size = 0;

    TRACE( "handle %p, command %#x, data %p, data_size %p, header_size %u.\n",
           handle, command, data, data_size, header_size );

    if (!(msg_data = thread_info->rawinput) || msg_data->hw_id != (UINT_PTR)handle)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return -1;
    }

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN( "Invalid structure size %u.\n", header_size );
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return -1;
    }
    if (command != RID_HEADER && command != RID_INPUT) goto failed;
    if (command == RID_INPUT) size = msg_data->size - sizeof(*msg_data);

    if (!data)
    {
        *data_size = sizeof(RAWINPUTHEADER) + size;
        return 0;
    }

    if (*data_size < sizeof(RAWINPUTHEADER) + size)
    {
        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
        return -1;
    }

    rawinput->header.dwType  = msg_data->rawinput.type;
    rawinput->header.dwSize  = sizeof(RAWINPUTHEADER) + msg_data->size - sizeof(*msg_data);
    rawinput->header.hDevice = UlongToHandle( msg_data->rawinput.device );
    rawinput->header.wParam  = msg_data->rawinput.wparam;
    if (command == RID_HEADER) return sizeof(RAWINPUTHEADER);

    if (msg_data->rawinput.type == RIM_TYPEMOUSE)
    {
        if (size != sizeof(RAWMOUSE)) goto failed;
        rawinput->data.mouse = *(RAWMOUSE *)(msg_data + 1);
    }
    else if (msg_data->rawinput.type == RIM_TYPEKEYBOARD)
    {
        if (size != sizeof(RAWKEYBOARD)) goto failed;
        rawinput->data.keyboard = *(RAWKEYBOARD *)(msg_data + 1);
    }
    else if (msg_data->rawinput.type == RIM_TYPEHID)
    {
        RAWHID *hid = (RAWHID *)(msg_data + 1);
        if (size < offsetof(RAWHID, bRawData[0])) goto failed;
        if (size != offsetof(RAWHID, bRawData[hid->dwCount * hid->dwSizeHid])) goto failed;
        memcpy( &rawinput->data.hid, msg_data + 1, size );
    }
    else
    {
        FIXME( "Unhandled rawinput type %#x.\n", msg_data->rawinput.type );
        goto failed;
    }

    return rawinput->header.dwSize;

failed:
    WARN( "Invalid command %u or data size %u.\n", command, size );
    RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
    return -1;
}

BOOL process_rawinput_message( MSG *msg, UINT hw_id, const struct hardware_msg_data *msg_data )
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (msg->message == WM_INPUT_DEVICE_CHANGE)
    {
        BOOL refresh = msg->wParam == GIDC_ARRIVAL;
        struct device *device;

        pthread_mutex_lock( &rawinput_mutex );
        if ((device = find_device_from_handle( UlongToHandle( msg_data->rawinput.device ), refresh )))
        {
            if (msg->wParam == GIDC_REMOVAL)
            {
                list_remove( &device->entry );
                NtClose( device->file );
                free( device->data );
                free( device );
            }
        }
        pthread_mutex_unlock( &rawinput_mutex );
    }
    else
    {
        struct hardware_msg_data *tmp;
        if (!(tmp = realloc( thread_info->rawinput, msg_data->size ))) return FALSE;
        memcpy( tmp, msg_data, msg_data->size );
        thread_info->rawinput = tmp;
        msg->lParam = (LPARAM)hw_id;
    }

    msg->pt = point_phys_to_win_dpi( msg->hwnd, msg->pt );
    return TRUE;
}

static void post_device_notifications( const RAWINPUTDEVICE *filter )
{
    ULONG usages = MAKELONG( filter->usUsagePage, filter->usUsage );
    struct device *device;

    LIST_FOR_EACH_ENTRY( device, &devices, struct device, entry )
    {
        switch (device->info.dwType)
        {
        case RIM_TYPEMOUSE:
            if (usages != MAKELONG( HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE )) continue;
            break;
        case RIM_TYPEKEYBOARD:
            if (usages != MAKELONG( HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD )) continue;
            break;
        case RIM_TYPEHID:
            if (usages != MAKELONG( device->info.hid.usUsagePage, device->info.hid.usUsage )) continue;
            break;
        }

        NtUserPostMessage( filter->hwndTarget, WM_INPUT_DEVICE_CHANGE, GIDC_ARRIVAL, (LPARAM)device->handle );
    }
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
        if ((device->dwFlags & RIDEV_DEVNOTIFY) && device->hwndTarget) post_device_notifications( device );
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
               devices[i].usUsage, devices[i].dwFlags, devices[i].hwndTarget );

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
            FIXME( "Unhandled flags %#x for device %u.\n", devices[i].dwFlags, i );
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

    rawinput_update_device_list( TRUE );

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
        server_devices[i].usage = MAKELONG(registered_devices[i].usUsage, registered_devices[i].usUsagePage);
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
