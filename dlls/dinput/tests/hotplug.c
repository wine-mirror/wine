/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#define DIRECTINPUT_VERSION 0x0800

#include <stdarg.h>
#include <stddef.h>
#include <limits.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"

#define COBJMACROS
#include "dinput.h"
#include "dinputd.h"
#include "devguid.h"
#include "dbt.h"
#include "unknwn.h"
#include "winstring.h"
#include "cfgmgr32.h"

#include "wine/hid.h"

#include "dinput_test.h"

#include "initguid.h"
#include "roapi.h"
#include "weakreference.h"
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Gaming_Input
#define WIDL_using_Windows_Gaming_Input_Custom
#include "windows.gaming.input.custom.h"
#undef Size

static HRESULT (WINAPI *pRoGetActivationFactory)( HSTRING, REFIID, void** );
static HRESULT (WINAPI *pRoInitialize)( RO_INIT_TYPE );
static HRESULT (WINAPI *pWindowsCreateString)( const WCHAR*, UINT32, HSTRING* );
static HRESULT (WINAPI *pWindowsDeleteString)( HSTRING str );

static BOOL load_combase_functions(void)
{
    HMODULE combase = GetModuleHandleW( L"combase.dll" );

#define LOAD_FUNC(m, f) if (!(p ## f = (void *)GetProcAddress( m, #f ))) goto failed;
    LOAD_FUNC( combase, RoGetActivationFactory );
    LOAD_FUNC( combase, RoInitialize );
    LOAD_FUNC( combase, WindowsCreateString );
    LOAD_FUNC( combase, WindowsDeleteString );
#undef LOAD_FUNC

    return TRUE;

failed:
    win_skip("Failed to load combase.dll functions, skipping tests\n");
    return FALSE;
}

DWORD msg_wait_for_events_( const char *file, int line, DWORD count, HANDLE *events, DWORD timeout )
{
    DWORD ret, end = GetTickCount() + min( timeout, 5000 );
    MSG msg;

    while ((ret = MsgWaitForMultipleObjects( count, events, FALSE, min( timeout, 5000 ), QS_ALLINPUT )) <= count)
    {
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
        if (ret < count) return ret;
        if (timeout >= 5000) continue;
        if (end <= GetTickCount()) timeout = 0;
        else timeout = end - GetTickCount();
    }

    if (timeout >= 5000) ok_(file, line)( 0, "MsgWaitForMultipleObjects returned %#lx\n", ret );
    else ok_(file, line)( ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %#lx\n", ret );
    return ret;
}

#define check_interface( a, b, c ) check_interface_( __LINE__, a, b, c )
static void check_interface_( unsigned int line, void *iface_ptr, REFIID iid, BOOL supported )
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected;
    IUnknown *unk;

    expected = supported ? S_OK : E_NOINTERFACE;
    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_ (__FILE__, line)( hr == expected, "got hr %#lx, expected %#lx.\n", hr, expected );
    if (SUCCEEDED(hr)) IUnknown_Release( unk );
}

static BOOL test_input_lost( DWORD version )
{
#include "psh_hid_macros.h"
    static const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Physical),
                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 6),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    struct hid_device_desc desc =
    {
        .use_report_id = TRUE,
        .caps = { .InputReportByteLength = 1 },
        .attributes = default_attributes,
    };
    static const DIPROPDWORD buffer_size =
    {
        .diph =
        {
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwSize = sizeof(DIPROPDWORD),
            .dwHow = DIPH_DEVICE,
            .dwObj = 0,
        },
        .dwData = UINT_MAX,
    };

    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    DIDEVICEOBJECTDATA objdata[32] = {{0}};
    IDirectInputDevice8W *device = NULL;
    ULONG ref, count, size;
    DIJOYSTATE2 state;
    HRESULT hr;

    winetest_push_context( "%#lx", version );

    cleanup_registry_keys();

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;
    if (FAILED(hr = dinput_test_create_device( version, &devinst, &device ))) goto done;

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, 0, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_BUFFERSIZE, &buffer_size.diph );
    ok( hr == DI_OK, "SetProperty returned %#lx\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );
    size = version < 0x0800 ? sizeof(DIDEVICEOBJECTDATA_DX3) : sizeof(DIDEVICEOBJECTDATA);
    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( count == 0, "got %lu expected 0\n", count );

    hid_device_stop( &desc, 1 );

    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_INPUTLOST, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_INPUTLOST, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DIERR_INPUTLOST, "GetDeviceData returned %#lx\n", hr );
    hr = IDirectInputDevice8_Poll( device );
    ok( hr == DIERR_INPUTLOST, "Poll returned: %#lx\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DIERR_UNPLUGGED, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceData returned %#lx\n", hr );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_NOEFFECT, "Unacquire returned: %#lx\n", hr );

    fill_context( desc.context, ARRAY_SIZE(desc.context) );
    hid_device_start( &desc, 1 );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == S_OK, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == S_OK, "GetDeviceState returned %#lx\n", hr );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

done:
    hid_device_stop( &desc, 1 );
    cleanup_registry_keys();

    winetest_pop_context();
    return device != NULL;
}

static int device_change_count;
static int device_change_expect;
static int device_change_expect_custom;
static HWND device_change_hwnd;
static BOOL device_change_all;

static BOOL all_upper( const WCHAR *str, const WCHAR *end )
{
    while (str++ != end) if (towupper( str[-1] ) != str[-1]) return FALSE;
    return TRUE;
}

static BOOL all_lower( const WCHAR *str, const WCHAR *end )
{
    while (str++ != end) if (towlower( str[-1] ) != str[-1]) return FALSE;
    return TRUE;
}

static const union device_change_event
{
    struct
    {
        USHORT Version;
        USHORT Size;
        GUID Event;
        FILE_OBJECT *FileObject;
        LONG NameBufferOffset;
        UCHAR CustomDataBuffer[256];
    };
    TARGET_DEVICE_CUSTOM_NOTIFICATION notif;
} device_change_events[] =
{
    {.Version = 1, .Size = sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION),
     .Event = {0x00000001,0xdead,0xbeef}, .NameBufferOffset = -1},
    {.Version = 1, .Size = offsetof(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer[sizeof(GUID)]),
     .Event = {0x00000002,0xdead,0xbeef}, .NameBufferOffset = -1,
     .CustomDataBuffer = {0x00,0x00,0x00,0x03,0xad,0xde,0xef,0xbe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {.Version = 1, .Size = offsetof(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer[23 * sizeof(WCHAR)]),
     .Event = {0x00000003,0xdead,0xbeef}, .NameBufferOffset = 0,
     .CustomDataBuffer = {'W',0,'i',0,'n',0,'e',0,' ',0,'i',0,'s',0,' ',0,'n',0,'o',0,'t',0,' ',0,
                          'a',0,'n',0,' ',0,'e',0,'m',0,'u',0,'l',0,'a',0,'t',0,'o',0,'r',0}},
    {.Version = 1, .Size = offsetof(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer[sizeof(GUID) + 23 * sizeof(WCHAR)]),
     .Event = {0x00000004,0xdead,0xbeef}, .NameBufferOffset = sizeof(GUID),
     .CustomDataBuffer = {0x00,0x00,0x00,0x03,0xad,0xde,0xef,0xbe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                          'W',0,'i',0,'n',0,'e',0,' ',0,'i',0,'s',0,' ',0,'n',0,'o',0,'t',0,' ',0,
                          'a',0,'n',0,' ',0,'e',0,'m',0,'u',0,'l',0,'a',0,'t',0,'o',0,'r',0}},
    /* DBT_DEVICEREMOVECOMPLETE event that is sent when device is removed */
    {.Version = 1, .Size = offsetof(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer[0]), .NameBufferOffset = -1},
};
static const union device_change_event *device_change_expect_event;
static HANDLE device_change_expect_handle;
static HDEVNOTIFY handle_devnotify;
static HCMNOTIFICATION handle_cmnotify;

static LRESULT CALLBACK devnotify_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_DEVICECHANGE)
    {
        DEV_BROADCAST_HDR *header = (DEV_BROADCAST_HDR *)lparam;
        DEV_BROADCAST_DEVICEINTERFACE_W *iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)lparam;
        const WCHAR *upper_end, *name_end, *expect_prefix;
        GUID expect_guid;

        if (device_change_expect_custom)
        {
            todo_wine_if( device_change_expect_custom == 1 )
            ok( header->dbch_devicetype == DBT_DEVTYP_HANDLE ||
                broken( device_change_expect_custom == 1 && header->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE ), /* w8 32bit */
                "got type %#lx\n", header->dbch_devicetype);
            ok( wparam == (device_change_expect_custom > 1 ? DBT_CUSTOMEVENT : DBT_DEVICEREMOVECOMPLETE),
                "got wparam %#Ix\n", wparam );
        }
        if (header->dbch_devicetype == DBT_DEVTYP_HANDLE)
        {
            DEV_BROADCAST_HANDLE *handle = (DEV_BROADCAST_HANDLE *)header;
            UINT size = handle->dbch_size - offsetof(DEV_BROADCAST_HANDLE, dbch_data);

            winetest_push_context( "%Id", device_change_expect_event - device_change_events );
            ok( !!device_change_expect_event, "unexpected event\n" );
            ok( device_change_expect_event->Size - offsetof(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer[0]) ==
                handle->dbch_size - 2 * sizeof(WCHAR) - offsetof(DEV_BROADCAST_HANDLE, dbch_data[0]),
                "got dbch_size %#lx\n", handle->dbch_size );
            ok( !handle->dbch_reserved, "got dbch_reserved %lu\n", handle->dbch_reserved );
            ok( handle->dbch_handle == device_change_expect_handle, "got dbch_handle %p\n", handle->dbch_handle );
            ok( handle->dbch_hdevnotify == handle_devnotify, "got dbch_hdevnotify %p\n", handle->dbch_hdevnotify );
            ok( IsEqualGUID( &handle->dbch_eventguid, &device_change_expect_event->Event ),
                "got dbch_eventguid %s\n", debugstr_guid(&handle->dbch_eventguid) );
            ok( handle->dbch_nameoffset == device_change_expect_event->NameBufferOffset,
                "got dbch_nameoffset %ld\n", handle->dbch_nameoffset );
            ok( !memcmp( handle->dbch_data, device_change_expect_event->CustomDataBuffer, size ),
                "got unexpected dbch_data\n" );
            winetest_pop_context();

            device_change_expect_custom--;
            device_change_expect_event++;
            return DefWindowProcW( hwnd, msg, wparam, lparam );
        }

        if (device_change_all && (device_change_count == 0 || device_change_count == 3))
        {
            expect_guid = control_class;
            expect_prefix = L"\\\\?\\WINETEST#";
        }
        else
        {
            expect_guid = GUID_DEVINTERFACE_HID;
            expect_prefix = L"\\\\?\\HID#";
        }

        ok( hwnd == device_change_hwnd, "got hwnd %p\n", hwnd );
        ok( header->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE, "got dbch_devicetype %lu\n",
            header->dbch_devicetype );

        winetest_push_context( "%u", device_change_count );

        ok( IsEqualGUID( &iface->dbcc_classguid, &expect_guid ), "got dbch_classguid %s\n",
            debugstr_guid( &iface->dbcc_classguid ) );
        ok( iface->dbcc_size >= offsetof( DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name[wcslen( iface->dbcc_name ) + 1] ),
            "got dbcc_size %lu\n", iface->dbcc_size );
        ok( !wcsncmp( iface->dbcc_name, expect_prefix, wcslen( expect_prefix ) ),
            "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );

        upper_end = wcschr( iface->dbcc_name + wcslen( expect_prefix ), '#' );
        name_end = iface->dbcc_name + wcslen( iface->dbcc_name ) + 1;
        ok( !!upper_end, "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );
        ok( all_upper( iface->dbcc_name, upper_end ), "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );
        ok( all_lower( upper_end, name_end ), "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );

        if (device_change_count++ >= device_change_expect / 2)
            ok( wparam == DBT_DEVICEREMOVECOMPLETE, "got wparam %#Ix\n", wparam );
        else
            ok( wparam == DBT_DEVICEARRIVAL, "got wparam %#Ix\n", wparam );

        winetest_pop_context();
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

struct cm_notify_callback_data
{
    HCMNOTIFICATION hnotify;
    DWORD device_change_expect;
    DWORD device_change_expect_custom;
    BOOL device_change_all;

    DWORD device_change_count;
    HANDLE device_change_sem;

    BOOL received_deviceremovalcomplete;
};

static CALLBACK DWORD cm_notify_callback( HCMNOTIFICATION hnotify, void *ctx, CM_NOTIFY_ACTION action,
                                          CM_NOTIFY_EVENT_DATA *data, DWORD size )
{
    struct cm_notify_callback_data *cb_data = ctx;

    switch (action)
    {
    case CM_NOTIFY_ACTION_DEVICECUSTOMEVENT:
    {
        const union device_change_event *event;
        SIZE_T idx;

        ok( hnotify == handle_cmnotify, "%p != %p\n", hnotify, handle_cmnotify );
        ok( !!cb_data->device_change_expect_custom, "unexpected event\n" );
        idx = (ARRAY_SIZE(device_change_events) + 1) - cb_data->device_change_expect_custom;
        event = &device_change_events[idx];

        winetest_push_context( "%Id", idx );
        ok( size == offsetof( CM_NOTIFY_EVENT_DATA, u.DeviceHandle.Data[data->u.DeviceHandle.DataSize] ),
            "got %#lx\n", size );
        ok( IsEqualGUID( &data->u.DeviceHandle.EventGuid, &event->Event ), "got DeviceHandle.EventGuid %s\n",
            debugstr_guid( &data->u.DeviceHandle.EventGuid ) );
        ok( data->u.DeviceHandle.NameOffset == event->NameBufferOffset, "got DeviceHandle.NameOffset %ld\n",
            data->u.DeviceHandle.NameOffset );
        ok( data->u.DeviceHandle.DataSize == event->Size - offsetof( TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer[0] ),
            "got DeviceHandle.DataSize %ld\n", data->u.DeviceHandle.DataSize );
        ok( !memcmp( data->u.DeviceHandle.Data, event->CustomDataBuffer, data->u.DeviceHandle.DataSize ),
            "Unexpected DeviceHandle.Data\n" );
        winetest_pop_context();

        cb_data->device_change_expect_custom--;
        cb_data->device_change_count++;
        ReleaseSemaphore( cb_data->device_change_sem, 1, NULL );
        break;
    }
    case CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE:
        ok( size == sizeof(*data), "got %#lx\n", size );
        ok( cb_data->device_change_expect_custom, "Unexpected CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE\n" );
        cb_data->received_deviceremovalcomplete = TRUE;
        cb_data->device_change_count++;
        ReleaseSemaphore( cb_data->device_change_sem, 1, NULL );
        break;
    case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
    case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
    {
        const WCHAR *expect_prefix, *name = data->u.DeviceInterface.SymbolicLink, *upper_end, *name_end;
        GUID expect_guid;

        ok( hnotify == cb_data->hnotify, "%p != %p\n", hnotify, cb_data->hnotify );
        ok( size == offsetof( CM_NOTIFY_EVENT_DATA, u.DeviceInterface.SymbolicLink[wcslen( name ) + 1] ), "got %#lx\n", size );
        if (cb_data->device_change_all && (!cb_data->device_change_count || cb_data->device_change_count == 3))
        {
            expect_guid = control_class;
            expect_prefix = L"\\\\?\\WINETEST#";
        }
        else
        {
            expect_guid = GUID_DEVINTERFACE_HID;
            expect_prefix = L"\\\\?\\HID#";
        }

        winetest_push_context( "%lu", cb_data->device_change_count );
        ok( IsEqualGUID( &data->u.DeviceInterface.ClassGuid, &expect_guid ), "got u.DeviceInterface.ClassGuid %s\n",
            debugstr_guid( &data->u.DeviceInterface.ClassGuid ) );
        ok( !wcsncmp( name, expect_prefix, wcslen( expect_prefix ) ), "got u.DeviceInterface.SymbolicLink %s\n",
            debugstr_w( name ) );
        upper_end = wcschr( name + wcslen( expect_prefix ), '#' );
        name_end = name + wcslen( name ) + 1;
        ok( !!upper_end, "got u.DeviceInterface.SymbolicLink %s\n", debugstr_w( name ) );
        ok( all_upper( name, upper_end ), "got u.DeviceInterface.SymbolicLink %s\n", debugstr_w( name ) );
        ok( all_lower( upper_end, name_end ), "got u.DeviceInterface.SymbolicLink %s\n", debugstr_w( name ) );

        if (cb_data->device_change_count++ >= cb_data->device_change_expect / 2)
            ok( action == CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL, "got action %d\n", action );
        else if (cb_data->device_change_expect_custom)
            ok( action == CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL, "got action %d\n", action );
        else
            ok( action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL, "got action %d\n", action );

        winetest_pop_context();
        ReleaseSemaphore( cb_data->device_change_sem, 1, NULL );
        break;
    }
    default:
        ok( 0, "Unexpected CM_NOTIFY_ACTION %d\n", action );
        break;
    }

    return ERROR_SUCCESS;
}

static void test_RegisterDeviceNotification(void)
{
    DEV_BROADCAST_DEVICEINTERFACE_A iface_filter_a =
    {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_A),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
        .dbcc_classguid = GUID_DEVINTERFACE_HID,
    };
    DEV_BROADCAST_HANDLE handle_filter_a =
    {
        .dbch_size = sizeof(DEV_BROADCAST_HANDLE),
        .dbch_devicetype = DBT_DEVTYP_HANDLE,
    };
    WNDCLASSEXW class =
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .hInstance = GetModuleHandleW( NULL ),
        .lpszClassName = L"devnotify",
        .lpfnWndProc = devnotify_wndproc,
    };
    CM_NOTIFY_FILTER cm_iface_filter =
    {.cbSize = sizeof(CM_NOTIFY_FILTER),
     .Flags = 0,
     .FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE,
     .Reserved = 0,
     .u = {.DeviceInterface = {.ClassGuid = GUID_DEVINTERFACE_HID}}};
    CM_NOTIFY_FILTER cm_all_ifaces_filter =
    {
        .cbSize = sizeof(CM_NOTIFY_FILTER),
        .Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES,
        .FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE,
        .Reserved = 0,
    };
    CM_NOTIFY_FILTER cm_handle_filter =
    {
        .cbSize = sizeof(CM_NOTIFY_FILTER),
        .Flags = 0,
        .FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE,
        .Reserved = 0,
    };
    char buffer[1024] = {0};
    DEV_BROADCAST_HDR *header = (DEV_BROADCAST_HDR *)buffer;
    struct cm_notify_callback_data cm_ctx = {0};
    HANDLE hwnd, thread, stop_event;
    HDEVNOTIFY devnotify;
    DWORD i, ret;
    MSG msg;

    RegisterClassExW( &class );

    hwnd = CreateWindowW( class.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    devnotify = RegisterDeviceNotificationA( NULL, NULL, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    devnotify = RegisterDeviceNotificationA( (HWND)0xdeadbeef, NULL, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    devnotify = RegisterDeviceNotificationA( hwnd, NULL, 2 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_OEM) );
    header->dbch_size = sizeof(DEV_BROADCAST_OEM);
    header->dbch_devicetype = DBT_DEVTYP_OEM;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_DEVNODE) );
    header->dbch_size = sizeof(DEV_BROADCAST_DEVNODE);
    header->dbch_devicetype = DBT_DEVTYP_DEVNODE;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_VOLUME) );
    header->dbch_size = sizeof(DEV_BROADCAST_VOLUME);
    header->dbch_devicetype = DBT_DEVTYP_VOLUME;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_PORT_A) );
    header->dbch_size = sizeof(DEV_BROADCAST_PORT_A);
    header->dbch_devicetype = DBT_DEVTYP_PORT;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_NET) );
    header->dbch_size = sizeof(DEV_BROADCAST_NET);
    header->dbch_devicetype = DBT_DEVTYP_NET;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %lu\n", GetLastError() );

    devnotify = RegisterDeviceNotificationA( hwnd, &iface_filter_a, DEVICE_NOTIFY_WINDOW_HANDLE );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    cm_ctx.device_change_sem = CreateSemaphoreW( NULL, 0, 2 + ARRAY_SIZE(device_change_events), NULL );
    ok( !!cm_ctx.device_change_sem, "CreateSemaphoreW failed, error %lu\n", GetLastError() );

    cm_ctx.device_change_all = FALSE;
    cm_ctx.device_change_expect = 2;
    cm_ctx.device_change_count = 0;

    ret = CM_Register_Notification( &cm_iface_filter, &cm_ctx, cm_notify_callback, &cm_ctx.hnotify );
    todo_wine ok( !ret, "CM_Register_Notification failed, error %lu\n", ret );

    device_change_count = 0;
    device_change_expect = 2;
    device_change_hwnd = hwnd;
    device_change_all = FALSE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );

    while (device_change_count < device_change_expect)
    {
        ret = MsgWaitForMultipleObjects( 0, NULL, FALSE, 5000, QS_ALLINPUT );
        ok( !ret, "MsgWaitForMultipleObjects returned %#lx\n", ret );
        if (ret) break;
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (device_change_count == device_change_expect / 2) SetEvent( stop_event );
    }
    for (i = 0; i < cm_ctx.device_change_expect; i++)
    {
        ret = WaitForSingleObject( cm_ctx.device_change_sem, 100 );
        todo_wine ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    }
    todo_wine ok( cm_ctx.device_change_count == cm_ctx.device_change_expect, "%lu != %lu\n",
                  cm_ctx.device_change_count, cm_ctx.device_change_expect );

    ret = WaitForSingleObject( thread, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );
    CM_Unregister_Notification( cm_ctx.hnotify );

    memcpy( buffer, &iface_filter_a, sizeof(iface_filter_a) );
    strcpy( ((DEV_BROADCAST_DEVICEINTERFACE_A *)buffer)->dbcc_name, "device name" );
    ((DEV_BROADCAST_DEVICEINTERFACE_A *)buffer)->dbcc_size += strlen( "device name" ) + 1;
    devnotify = RegisterDeviceNotificationA( hwnd, buffer, DEVICE_NOTIFY_WINDOW_HANDLE );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    cm_ctx.device_change_all = FALSE;
    cm_ctx.device_change_expect = 2;
    cm_ctx.device_change_count = 0;

    ret = CM_Register_Notification( &cm_iface_filter, &cm_ctx, cm_notify_callback, &cm_ctx.hnotify );
    todo_wine ok( !ret, "CM_Register_Notification failed, error %lu\n", ret );

    device_change_count = 0;
    device_change_expect = 2;
    device_change_hwnd = hwnd;
    device_change_all = FALSE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );

    while (device_change_count < device_change_expect)
    {
        ret = MsgWaitForMultipleObjects( 0, NULL, FALSE, 5000, QS_ALLINPUT );
        ok( !ret, "MsgWaitForMultipleObjects returned %#lx\n", ret );
        if (ret) break;
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (device_change_count == device_change_expect / 2) SetEvent( stop_event );
    }
    for (i = 0; i < cm_ctx.device_change_expect; i++)
    {
        ret = WaitForSingleObject( cm_ctx.device_change_sem, 100 );
        todo_wine ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    }
    todo_wine ok( cm_ctx.device_change_count == cm_ctx.device_change_expect, "%lu != %lu\n",
                  cm_ctx.device_change_count, cm_ctx.device_change_expect );

    ret = WaitForSingleObject( thread, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );
    CM_Unregister_Notification( cm_ctx.hnotify );

    devnotify = RegisterDeviceNotificationA( hwnd, &iface_filter_a, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    cm_ctx.device_change_all = TRUE;
    cm_ctx.device_change_expect = 4;
    cm_ctx.device_change_count = 0;
    ret = CM_Register_Notification( &cm_all_ifaces_filter, &cm_ctx, cm_notify_callback,
                                    &cm_ctx.hnotify );
    todo_wine ok( !ret, "CM_Register_Notification failed, error %lu\n", ret );

    device_change_count = 0;
    device_change_expect = 4;
    device_change_hwnd = hwnd;
    device_change_all = TRUE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );

    while (device_change_count < device_change_expect)
    {
        ret = MsgWaitForMultipleObjects( 0, NULL, FALSE, 5000, QS_ALLINPUT );
        ok( !ret, "MsgWaitForMultipleObjects returned %#lx\n", ret );
        if (ret) break;
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (device_change_count == device_change_expect / 2) SetEvent( stop_event );
    }
    for (i = 0; i < cm_ctx.device_change_expect; i++)
    {
        ret = WaitForSingleObject( cm_ctx.device_change_sem, 100 );
        todo_wine ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    }
    todo_wine ok( cm_ctx.device_change_count == cm_ctx.device_change_expect, "%lu != %lu\n",
                  cm_ctx.device_change_count, cm_ctx.device_change_expect );

    ret = WaitForSingleObject( thread, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );
    CM_Unregister_Notification( cm_ctx.hnotify );

    devnotify = RegisterDeviceNotificationA( hwnd, &iface_filter_a, DEVICE_NOTIFY_WINDOW_HANDLE );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    cm_ctx.device_change_all = FALSE;
    cm_ctx.device_change_expect = 2 + ARRAY_SIZE(device_change_events);
    cm_ctx.device_change_count = 0;
    cm_iface_filter.Flags = 0;
    ret = CM_Register_Notification( &cm_iface_filter, &cm_ctx, cm_notify_callback, &cm_ctx.hnotify );
    todo_wine ok( !ret, "CM_Register_Notification failed, error %lu\n", ret );

    device_change_count = 0;
    device_change_expect = 2;
    device_change_hwnd = hwnd;
    device_change_all = FALSE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );

    while (device_change_count < device_change_expect)
    {
        ret = MsgWaitForMultipleObjects( 0, NULL, FALSE, 5000, QS_ALLINPUT );
        ok( !ret, "MsgWaitForMultipleObjects returned %#lx\n", ret );
        if (ret) break;
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (device_change_count == 1 && !device_change_expect_event)
        {
            WCHAR device_path[MAX_PATH];
            HANDLE file;
            UINT i;

            ret = WaitForSingleObject( cm_ctx.device_change_sem, 100 );
            todo_wine ok( !ret, "WaitForSingleObject returned %#lx\n", ret );

            swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", LOWORD(EXPECT_VIDPID), HIWORD(EXPECT_VIDPID) );
            ret = find_hid_device_path( device_path );
            ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );

            file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
            ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

            handle_filter_a.dbch_handle = file;
            handle_devnotify = RegisterDeviceNotificationA( hwnd, &handle_filter_a, DEVICE_NOTIFY_WINDOW_HANDLE );
            ok( !!handle_devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );

            cm_handle_filter.u.DeviceHandle.hTarget = file;
            ret = CM_Register_Notification( &cm_handle_filter, &cm_ctx, cm_notify_callback, &handle_cmnotify );
            todo_wine ok( !ret, "CM_Register_Notification failed, error %lu\n", ret );

            device_change_expect_handle = file;
            device_change_expect_event = device_change_events;
            cm_ctx.device_change_expect_custom = ARRAY_SIZE(device_change_events) + 1;
            for (i = 0; i < ARRAY_SIZE(device_change_events) - 1; i++)
            {
                ret = sync_ioctl( file, IOCTL_WINETEST_DEVICE_CHANGE, (BYTE *)&device_change_events[i].notif,
                                  device_change_events[i].notif.Size, NULL, 0, 5000 );
                ok( ret, "IOCTL_WINETEST_DEVICE_CHANGE failed, last error %lu\n", GetLastError() );
                device_change_expect_custom++;
            }
            device_change_expect_custom++; /* extra DBT_DEVICEREMOVECOMPLETE event that is sent when device is removed */

            CloseHandle( file );
        }
        if (device_change_count == 1 && device_change_expect_custom == 1) SetEvent( stop_event );
    }
    for (i = 1; i < cm_ctx.device_change_expect; i++)
    {
        ret = WaitForSingleObject( cm_ctx.device_change_sem, 100 );
        todo_wine ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    }
    todo_wine ok( cm_ctx.device_change_count == cm_ctx.device_change_expect, "%lu != %lu\n",
                  cm_ctx.device_change_count, cm_ctx.device_change_expect );
    todo_wine ok( cm_ctx.received_deviceremovalcomplete, "%d != 0\n", cm_ctx.received_deviceremovalcomplete );

    ret = WaitForSingleObject( thread, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    CloseHandle( thread );
    CloseHandle( stop_event );

    if (handle_devnotify) UnregisterDeviceNotification( handle_devnotify );
    if (handle_cmnotify) CM_Unregister_Notification( handle_cmnotify );
    UnregisterDeviceNotification( devnotify );
    CM_Unregister_Notification( cm_ctx.hnotify );
    device_change_expect_event = NULL;
    handle_devnotify = 0;
    CloseHandle( cm_ctx.device_change_sem );

    DestroyWindow( hwnd );
    UnregisterClassW( class.lpszClassName, class.hInstance );
}

struct controller_handler
{
    IEventHandler_RawGameController IEventHandler_RawGameController_iface;
    HANDLE event;
    BOOL invoked;
};

static inline struct controller_handler *impl_from_IEventHandler_RawGameController( IEventHandler_RawGameController *iface )
{
    return CONTAINING_RECORD( iface, struct controller_handler, IEventHandler_RawGameController_iface );
}

static HRESULT WINAPI controller_handler_QueryInterface( IEventHandler_RawGameController *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IEventHandler_RawGameController ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (winetest_debug > 1) trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI controller_handler_AddRef( IEventHandler_RawGameController *iface )
{
    return 2;
}

static ULONG WINAPI controller_handler_Release( IEventHandler_RawGameController *iface )
{
    return 1;
}

static HRESULT WINAPI controller_handler_Invoke( IEventHandler_RawGameController *iface,
                                                 IInspectable *sender, IRawGameController *controller )
{
    struct controller_handler *impl = impl_from_IEventHandler_RawGameController( iface );

    if (winetest_debug > 1) trace( "iface %p, sender %p, controller %p\n", iface, sender, controller );

    ok( sender == NULL, "got sender %p\n", sender );
    impl->invoked = TRUE;
    SetEvent( impl->event );

    return S_OK;
}

static const IEventHandler_RawGameControllerVtbl controller_handler_vtbl =
{
    controller_handler_QueryInterface,
    controller_handler_AddRef,
    controller_handler_Release,
    controller_handler_Invoke,
};

static struct controller_handler controller_removed = {{&controller_handler_vtbl}};
static struct controller_handler controller_added = {{&controller_handler_vtbl}};

DEFINE_GUID( IID_IGameControllerImpl, 0x06e58977, 0x7684, 0x4dc5, 0xba, 0xd1, 0xcd, 0xa5, 0x2a, 0x4a, 0xa0, 0x6d );
typedef IInspectable IGameControllerImpl;

#define DEFINE_IINSPECTABLE_OUTER( pfx, iface_type, impl_type, outer_iface )                       \
    static inline impl_type *impl_from_##iface_type( iface_type *iface )                           \
    {                                                                                              \
        return CONTAINING_RECORD( iface, impl_type, iface_type##_iface );                          \
    }                                                                                              \
    static HRESULT WINAPI pfx##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_QueryInterface( (IInspectable *)impl->outer_iface, iid, out );         \
    }                                                                                              \
    static ULONG WINAPI pfx##_AddRef( iface_type *iface )                                          \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_AddRef( (IInspectable *)impl->outer_iface );                           \
    }                                                                                              \
    static ULONG WINAPI pfx##_Release( iface_type *iface )                                         \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_Release( (IInspectable *)impl->outer_iface );                          \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetIids( iface_type *iface, ULONG *iid_count, IID **iids )         \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_GetIids( (IInspectable *)impl->outer_iface, iid_count, iids );         \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetRuntimeClassName( iface_type *iface, HSTRING *class_name )      \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_GetRuntimeClassName( (IInspectable *)impl->outer_iface, class_name );  \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetTrustLevel( iface_type *iface, TrustLevel *trust_level )        \
    {                                                                                              \
        impl_type *impl = impl_from_##iface_type( iface );                                         \
        return IInspectable_GetTrustLevel( (IInspectable *)impl->outer_iface, trust_level );       \
    }

struct custom_controller
{
    IGameControllerImpl IGameControllerImpl_iface;
    IGameControllerInputSink IGameControllerInputSink_iface;
    IHidGameControllerInputSink IHidGameControllerInputSink_iface;
    IGameController *IGameController_outer;
    LONG ref;

    BOOL initialize_called;
    BOOL on_input_resumed_called;
    BOOL on_input_suspended_called;
    BOOL raw_game_controller_queried;
};

static inline struct custom_controller *impl_from_IGameControllerImpl( IGameControllerImpl *iface )
{
    return CONTAINING_RECORD( iface, struct custom_controller, IGameControllerImpl_iface );
}

static HRESULT WINAPI controller_QueryInterface( IGameControllerImpl *iface, REFIID iid, void **out )
{
    struct custom_controller *impl = impl_from_IGameControllerImpl( iface );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IGameControllerImpl ))
    {
        IInspectable_AddRef( (*out = &impl->IGameControllerImpl_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGameControllerInputSink ))
    {
        IInspectable_AddRef( (*out = &impl->IGameControllerInputSink_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IHidGameControllerInputSink ))
    {
        IInspectable_AddRef( (*out = &impl->IHidGameControllerInputSink_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IRawGameController ))
    {
        impl->raw_game_controller_queried = TRUE;
        *out = NULL;
        return E_NOINTERFACE;
    }

    ok( 0, "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI controller_AddRef( IGameControllerImpl *iface )
{
    struct custom_controller *impl = impl_from_IGameControllerImpl( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI controller_Release( IGameControllerImpl *iface )
{
    struct custom_controller *impl = impl_from_IGameControllerImpl( iface );
    return InterlockedDecrement( &impl->ref );
}

static HRESULT WINAPI controller_GetIids( IGameControllerImpl *iface, ULONG *iid_count, IID **iids )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_GetRuntimeClassName( IGameControllerImpl *iface, HSTRING *class_name )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_GetTrustLevel( IGameControllerImpl *iface, TrustLevel *trust_level )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_Initialize( IGameControllerImpl *iface, IGameController *outer, IGameControllerProvider *provider )
{
    struct custom_controller *impl = impl_from_IGameControllerImpl( iface );

    ok( !impl->initialize_called, "Initialize already called\n" );
    impl->initialize_called = TRUE;

    check_interface( outer, &IID_IUnknown, TRUE );
    check_interface( outer, &IID_IInspectable, TRUE );
    check_interface( outer, &IID_IAgileObject, TRUE );
    check_interface( outer, &IID_IWeakReferenceSource, TRUE );
    check_interface( outer, &IID_IGameController, TRUE );
    impl->IGameController_outer = outer;

    check_interface( provider, &IID_IUnknown, TRUE );
    check_interface( provider, &IID_IInspectable, TRUE );
    check_interface( provider, &IID_IAgileObject, TRUE );
    check_interface( provider, &IID_IWeakReferenceSource, TRUE );
    check_interface( provider, &IID_IGameControllerProvider, TRUE );
    check_interface( provider, &IID_IHidGameControllerProvider, TRUE );

    return S_OK;
}

static const void *controller_vtbl[] =
{
    controller_QueryInterface,
    controller_AddRef,
    controller_Release,
    /* IInspectable methods */
    controller_GetIids,
    controller_GetRuntimeClassName,
    controller_GetTrustLevel,
    /* IGameControllerImpl methods */
    controller_Initialize,
};

DEFINE_IINSPECTABLE_OUTER( input_sink, IGameControllerInputSink, struct custom_controller, IGameController_outer )

static HRESULT WINAPI input_sink_OnInputResumed( IGameControllerInputSink *iface, UINT64 timestamp )
{
    struct custom_controller *impl = impl_from_IGameControllerInputSink( iface );

    if (winetest_debug > 1) trace( "iface %p, timestamp %I64u\n", iface, timestamp );

    ok( !controller_added.invoked, "controller added handler invoked\n" );
    ok( !impl->on_input_resumed_called, "OnInputResumed already called\n" );
    impl->on_input_resumed_called = TRUE;

    return S_OK;
}

static HRESULT WINAPI input_sink_OnInputSuspended( IGameControllerInputSink *iface, UINT64 timestamp )
{
    struct custom_controller *impl = impl_from_IGameControllerInputSink( iface );

    if (winetest_debug > 1) trace( "iface %p, timestamp %I64u\n", iface, timestamp );

    ok( !controller_removed.invoked, "controller removed handler invoked\n" );
    ok( !impl->on_input_suspended_called, "OnInputSuspended already called\n" );
    impl->on_input_suspended_called = TRUE;

    return S_OK;
}

static const struct IGameControllerInputSinkVtbl input_sink_vtbl =
{
    input_sink_QueryInterface,
    input_sink_AddRef,
    input_sink_Release,
    /* IInspectable methods */
    input_sink_GetIids,
    input_sink_GetRuntimeClassName,
    input_sink_GetTrustLevel,
    /* IGameControllerInputSink methods */
    input_sink_OnInputResumed,
    input_sink_OnInputSuspended,
};

DEFINE_IINSPECTABLE_OUTER( hid_sink, IHidGameControllerInputSink, struct custom_controller, IGameController_outer )

static HRESULT WINAPI hid_sink_OnInputReportReceived( IHidGameControllerInputSink *iface, UINT64 timestamp, BYTE id, UINT32 report_len, BYTE *report_buf )
{
    ok( 0, "unexpected call\n" );
    return S_OK;
}

static const struct IHidGameControllerInputSinkVtbl hid_sink_vtbl =
{
    hid_sink_QueryInterface,
    hid_sink_AddRef,
    hid_sink_Release,
    /* IInspectable methods */
    hid_sink_GetIids,
    hid_sink_GetRuntimeClassName,
    hid_sink_GetTrustLevel,
    /* IGameControllerInputSink methods */
    hid_sink_OnInputReportReceived,
};

static struct custom_controller custom_controller =
{
    {(IInspectableVtbl *)controller_vtbl},
    {&input_sink_vtbl},
    {&hid_sink_vtbl},
};

struct custom_factory
{
    ICustomGameControllerFactory ICustomGameControllerFactory_iface;
    BOOL create_controller_called;
    BOOL create_controller;
    BOOL on_game_controller_added_called;
    HANDLE added_event;
    BOOL on_game_controller_removed_called;
    HANDLE removed_event;
};

static inline struct custom_factory *impl_from_ICustomGameControllerFactory( ICustomGameControllerFactory *iface )
{
    return CONTAINING_RECORD( iface, struct custom_factory, ICustomGameControllerFactory_iface );
}

static HRESULT WINAPI custom_factory_QueryInterface( ICustomGameControllerFactory *iface, REFIID iid, void **out )
{
    struct custom_factory *impl = impl_from_ICustomGameControllerFactory( iface );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICustomGameControllerFactory ))
    {
        IInspectable_AddRef( (*out = &impl->ICustomGameControllerFactory_iface) );
        return S_OK;
    }

    ok( 0, "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI custom_factory_AddRef( ICustomGameControllerFactory *iface )
{
    return 2;
}

static ULONG WINAPI custom_factory_Release( ICustomGameControllerFactory *iface )
{
    return 1;
}

static HRESULT WINAPI custom_factory_GetIids( ICustomGameControllerFactory *iface, ULONG *iid_count, IID **iids )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI custom_factory_GetRuntimeClassName( ICustomGameControllerFactory *iface, HSTRING *class_name )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI custom_factory_GetTrustLevel( ICustomGameControllerFactory *iface, TrustLevel *trust_level )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI custom_factory_CreateGameController( ICustomGameControllerFactory *iface, IGameControllerProvider *provider,
                                                           IInspectable **value )
{
    struct custom_factory *impl = impl_from_ICustomGameControllerFactory( iface );

    if (winetest_debug > 1) trace( "iface %p, provider %p, value %p\n", iface, provider, value );

    ok( !controller_added.invoked, "controller added handler invoked\n" );
    ok( !impl->create_controller_called, "unexpected call\n" );
    impl->create_controller_called = TRUE;
    if (!impl->create_controller) return E_NOTIMPL;

    check_interface( provider, &IID_IUnknown, TRUE );
    check_interface( provider, &IID_IInspectable, TRUE );
    check_interface( provider, &IID_IAgileObject, TRUE );
    check_interface( provider, &IID_IGameControllerProvider, TRUE );
    check_interface( provider, &IID_IHidGameControllerProvider, TRUE );
    check_interface( provider, &IID_IXusbGameControllerProvider, FALSE );
    check_interface( provider, &IID_IGameControllerInputSink, FALSE );
    custom_controller.ref = 1;

    *value = &custom_controller.IGameControllerImpl_iface;
    return S_OK;
}

static HRESULT WINAPI custom_factory_OnGameControllerAdded( ICustomGameControllerFactory *iface, IGameController *value )
{
    struct custom_factory *impl = impl_from_ICustomGameControllerFactory( iface );

    if (winetest_debug > 1) trace( "iface %p, value %p\n", iface, value );

    ok( controller_added.invoked, "controller added handler not invoked\n" );
    ok( impl->create_controller_called, "CreateGameController not called\n" );
    ok( impl->create_controller, "unexpected call\n" );
    ok( custom_controller.initialize_called, "Initialize not called\n" );
    ok( custom_controller.on_input_resumed_called, "OnInputResumed not called\n" );
    ok( !custom_controller.on_input_suspended_called, "OnInputSuspended not called\n" );
    ok( !impl->on_game_controller_added_called, "OnGameControllerAdded already called\n" );
    impl->on_game_controller_added_called = TRUE;
    SetEvent( impl->added_event );

    return S_OK;
}

static HRESULT WINAPI custom_factory_OnGameControllerRemoved( ICustomGameControllerFactory *iface, IGameController *value )
{
    struct custom_factory *impl = impl_from_ICustomGameControllerFactory( iface );

    if (winetest_debug > 1) trace( "iface %p, value %p\n", iface, value );

    ok( controller_removed.invoked, "controller removed handler invoked\n" );
    ok( custom_controller.on_input_suspended_called, "OnInputSuspended not called\n" );
    ok( impl->create_controller, "unexpected call\n" );
    ok( impl->on_game_controller_added_called, "OnGameControllerAdded already called\n" );
    ok( !impl->on_game_controller_removed_called, "OnGameControllerRemoved already called\n" );
    impl->on_game_controller_removed_called = TRUE;
    SetEvent( impl->removed_event );

    return S_OK;
}

static const struct ICustomGameControllerFactoryVtbl custom_factory_vtbl =
{
    custom_factory_QueryInterface,
    custom_factory_AddRef,
    custom_factory_Release,
    /* IInspectable methods */
    custom_factory_GetIids,
    custom_factory_GetRuntimeClassName,
    custom_factory_GetTrustLevel,
    /* ICustomGameControllerFactory methods */
    custom_factory_CreateGameController,
    custom_factory_OnGameControllerAdded,
    custom_factory_OnGameControllerRemoved,
};

static struct custom_factory custom_factory = {{&custom_factory_vtbl}};

static LRESULT CALLBACK windows_gaming_input_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_DEVICECHANGE)
    {
        winetest_push_context( "%u", device_change_count );
        if (device_change_count++ >= device_change_expect / 2)
        {
            ok( wparam == DBT_DEVICEREMOVECOMPLETE, "got wparam %#Ix\n", wparam );
            ok( controller_added.invoked, "controller added handler not invoked\n" );
        }
        else
        {
            ok( wparam == DBT_DEVICEARRIVAL, "got wparam %#Ix\n", wparam );
            todo_wine /* Wine currently listens to WINEXINPUT device arrival, which is received earlier than HID */
            ok( !controller_added.invoked, "controller added handler not invoked\n" );
            ok( !controller_removed.invoked, "controller removed handler invoked\n" );
        }
        winetest_pop_context();
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void test_windows_gaming_input(void)
{
    static const WCHAR *manager_class_name = RuntimeClass_Windows_Gaming_Input_Custom_GameControllerFactoryManager;
    static const WCHAR *class_name = RuntimeClass_Windows_Gaming_Input_RawGameController;
    DEV_BROADCAST_DEVICEINTERFACE_A iface_filter_a =
    {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_A),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
        .dbcc_classguid = GUID_DEVINTERFACE_HID,
    };
    WNDCLASSEXW class =
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .hInstance = GetModuleHandleW( NULL ),
        .lpszClassName = L"devnotify",
        .lpfnWndProc = windows_gaming_input_wndproc,
    };
    IGameControllerFactoryManagerStatics2 *manager_statics2;
    IRawGameController *raw_controller, *tmp_raw_controller;
    IGameController *game_controller, *tmp_game_controller;
    IGameControllerFactoryManagerStatics *manager_statics;
    EventRegistrationToken controller_removed_token;
    IVectorView_RawGameController *controller_view;
    EventRegistrationToken controller_added_token;
    IIterable_RawGameController *iterable;
    IIterator_RawGameController *iterator;
    IRawGameControllerStatics *statics;
    HANDLE hwnd, thread, stop_event;
    IInspectable *tmp_inspectable;
    HDEVNOTIFY devnotify;
    boolean boolret;
    HSTRING str;
    UINT32 size;
    HRESULT hr;
    DWORD res;
    MSG msg;

    if (!load_combase_functions()) return;

    hr = pRoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == RPC_E_CHANGED_MODE, "RoInitialize failed, hr %#lx\n", hr );

    hr = pWindowsCreateString( class_name, wcslen( class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IRawGameControllerStatics, (void **)&statics );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "RoGetActivationFactory failed, hr %#lx\n", hr );
    pWindowsDeleteString( str );

    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        return;
    }

    hr = pWindowsCreateString( manager_class_name, wcslen( manager_class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IGameControllerFactoryManagerStatics, (void **)&manager_statics );
    ok( hr == S_OK, "RoGetActivationFactory failed, hr %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IGameControllerFactoryManagerStatics2, (void **)&manager_statics2 );
    ok( hr == S_OK, "RoGetActivationFactory failed, hr %#lx\n", hr );
    pWindowsDeleteString( str );

    controller_added.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!controller_added.event, "CreateEventW failed, error %lu\n", GetLastError() );
    controller_removed.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!controller_removed.event, "CreateEventW failed, error %lu\n", GetLastError() );

    hr = IGameControllerFactoryManagerStatics_RegisterCustomFactoryForHardwareId( manager_statics, &custom_factory.ICustomGameControllerFactory_iface,
                                                                                  LOWORD(EXPECT_VIDPID), HIWORD(EXPECT_VIDPID) );
    todo_wine
    ok( hr == S_OK, "RegisterCustomFactoryForHardwareId returned %#lx\n", hr );

    hr = IRawGameControllerStatics_add_RawGameControllerAdded( statics, &controller_added.IEventHandler_RawGameController_iface,
                                                               &controller_added_token );
    ok( hr == S_OK, "add_RawGameControllerAdded returned %#lx\n", hr );
    ok( controller_added_token.value, "got token %I64u\n", controller_added_token.value );

    hr = IRawGameControllerStatics_add_RawGameControllerRemoved( statics, &controller_removed.IEventHandler_RawGameController_iface,
                                                                 &controller_removed_token );
    ok( hr == S_OK, "add_RawGameControllerAdded returned %#lx\n", hr );

    hr = IRawGameControllerStatics_get_RawGameControllers( statics, &controller_view );
    ok( hr == S_OK, "get_RawGameControllers returned %#lx\n", hr );

    RegisterClassExW( &class );

    hwnd = CreateWindowW( class.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );

    devnotify = RegisterDeviceNotificationA( hwnd, &iface_filter_a, DEVICE_NOTIFY_WINDOW_HANDLE );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    device_change_count = 0;
    device_change_expect = 2;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );

    msg_wait_for_events( 1, &controller_added.event, 5000 );

    ok( controller_added.invoked, "controller added handler not invoked\n" );
    ok( !controller_removed.invoked, "controller removed handler invoked\n" );
    todo_wine
    ok( custom_factory.create_controller_called, "CreateGameController not called\n" );

    hr = IVectorView_RawGameController_get_Size( controller_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 0, "got size %u\n", size );

    IVectorView_RawGameController_Release( controller_view );
    hr = IRawGameControllerStatics_get_RawGameControllers( statics, &controller_view );
    ok( hr == S_OK, "get_RawGameControllers returned %#lx\n", hr );

    hr = IVectorView_RawGameController_get_Size( controller_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 1, "got size %u\n", size );
    hr = IVectorView_RawGameController_GetAt( controller_view, 0, &raw_controller );
    ok( hr == S_OK, "GetAt returned %#lx\n", hr );
    hr = IRawGameController_QueryInterface( raw_controller, &IID_IGameController, (void **)&game_controller );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    hr = IGameControllerFactoryManagerStatics2_TryGetFactoryControllerFromGameController( manager_statics2,
            &custom_factory.ICustomGameControllerFactory_iface, game_controller, &tmp_game_controller );
    ok( hr == S_OK, "TryGetFactoryControllerFromGameController returned %#lx\n", hr );
    ok( !tmp_game_controller, "got controller %p\n", tmp_game_controller );

    hr = IRawGameControllerStatics_FromGameController( statics, game_controller, &tmp_raw_controller );
    ok( hr == S_OK, "FromGameController returned %#lx\n", hr );
    ok( tmp_raw_controller == raw_controller, "got controller %p\n", tmp_raw_controller );
    IRawGameController_Release( tmp_raw_controller );

    IGameController_Release( game_controller );
    IRawGameController_Release( raw_controller );

    SetEvent( stop_event );
    msg_wait_for_events( 1, &controller_removed.event, 5000 );

    ok( controller_added.invoked, "controller added handler not invoked\n" );
    ok( controller_removed.invoked, "controller removed handler not invoked\n" );

    hr = IVectorView_RawGameController_get_Size( controller_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 1, "got size %u\n", size );

    hr = IVectorView_RawGameController_QueryInterface( controller_view, &IID_IIterable_RawGameController,
                                                       (void **)&iterable );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IIterable_RawGameController_First( iterable, &iterator );
    ok( hr == S_OK, "First returned %#lx\n", hr );
    IIterable_RawGameController_Release( iterable );

    hr = IIterator_RawGameController_get_HasCurrent( iterator, &boolret );
    ok( hr == S_OK, "First returned %#lx\n", hr );
    ok( boolret == TRUE, "got HasCurrent %u\n", boolret );
    hr = IIterator_RawGameController_MoveNext( iterator, &boolret );
    ok( hr == S_OK, "First returned %#lx\n", hr );
    ok( boolret == FALSE, "got MoveNext %u\n", boolret );
    hr = IIterator_RawGameController_get_HasCurrent( iterator, &boolret );
    ok( hr == S_OK, "First returned %#lx\n", hr );
    ok( boolret == FALSE, "got MoveNext %u\n", boolret );
    hr = IIterator_RawGameController_MoveNext( iterator, &boolret );
    ok( hr == S_OK, "First returned %#lx\n", hr );
    ok( boolret == FALSE, "got MoveNext %u\n", boolret );
    IIterator_RawGameController_Release( iterator );

    IVectorView_RawGameController_Release( controller_view );
    hr = IRawGameControllerStatics_get_RawGameControllers( statics, &controller_view );
    ok( hr == S_OK, "get_RawGameControllers returned %#lx\n", hr );

    hr = IVectorView_RawGameController_get_Size( controller_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 0, "got size %u\n", size );

    IVectorView_RawGameController_Release( controller_view );

    res = WaitForSingleObject( thread, 5000 );
    ok( !res, "WaitForSingleObject returned %#lx\n", res );
    CloseHandle( thread );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    device_change_count = 0;
    device_change_expect = 2;
    custom_factory.create_controller = TRUE;
    custom_factory.create_controller_called = FALSE;
    ResetEvent( controller_added.event );
    controller_added.invoked = FALSE;
    ResetEvent( controller_removed.event );
    controller_removed.invoked = FALSE;
    ResetEvent( stop_event );

    custom_factory.added_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!custom_factory.added_event, "CreateEventW failed, error %lu\n", GetLastError() );
    custom_factory.removed_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!custom_factory.removed_event, "CreateEventW failed, error %lu\n", GetLastError() );

    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );
    msg_wait_for_events( 1, &controller_added.event, 5000 );
    res = msg_wait_for_events( 1, &custom_factory.added_event, 500 );
    todo_wine
    ok( !res, "msg_wait_for_events returned %#lx\n", res );
    hr = IRawGameControllerStatics_get_RawGameControllers( statics, &controller_view );
    ok( hr == S_OK, "get_RawGameControllers returned %#lx\n", hr );
    hr = IVectorView_RawGameController_GetAt( controller_view, 0, &raw_controller );
    ok( hr == S_OK, "GetAt returned %#lx\n", hr );
    hr = IRawGameController_QueryInterface( raw_controller, &IID_IGameController, (void **)&game_controller );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    ok( game_controller != custom_controller.IGameController_outer, "got controller %p\n", game_controller );

    hr = IGameControllerFactoryManagerStatics2_TryGetFactoryControllerFromGameController( manager_statics2,
            &custom_factory.ICustomGameControllerFactory_iface, game_controller, &tmp_game_controller );
    ok( hr == S_OK, "TryGetFactoryControllerFromGameController returned %#lx\n", hr );
    ok( tmp_game_controller == custom_controller.IGameController_outer, "got controller %p\n", tmp_game_controller );
    if (!tmp_game_controller) goto next;
    hr = IGameController_QueryInterface( tmp_game_controller, &IID_IInspectable, (void **)&tmp_inspectable );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    ok( tmp_inspectable == (void *)tmp_game_controller, "got inspectable %p\n", tmp_inspectable );

    check_interface( tmp_inspectable, &IID_IUnknown, TRUE );
    check_interface( tmp_inspectable, &IID_IInspectable, TRUE );
    check_interface( tmp_inspectable, &IID_IAgileObject, TRUE );
    check_interface( tmp_inspectable, &IID_IWeakReferenceSource, TRUE );
    check_interface( tmp_inspectable, &IID_IGameController, TRUE );
    check_interface( tmp_inspectable, &IID_IGameControllerBatteryInfo, TRUE );
    check_interface( tmp_inspectable, &IID_IGameControllerInputSink, TRUE );
    check_interface( tmp_inspectable, &IID_IHidGameControllerInputSink, TRUE );
    check_interface( tmp_inspectable, &IID_IGameControllerImpl, TRUE );

    check_interface( tmp_inspectable, &IID_IRawGameController, FALSE );
    check_interface( tmp_inspectable, &IID_IGameControllerProvider, FALSE );
    IInspectable_Release( tmp_inspectable );
    ok( custom_controller.raw_game_controller_queried, "IRawGameController not queried\n" );

    IGameController_Release( tmp_game_controller );

    hr = IRawGameControllerStatics_FromGameController( statics, custom_controller.IGameController_outer, &tmp_raw_controller );
    ok( hr == S_OK, "FromGameController returned %#lx\n", hr );
    todo_wine
    ok( tmp_raw_controller == raw_controller, "got controller %p\n", tmp_raw_controller );
    if (tmp_raw_controller) IRawGameController_Release( tmp_raw_controller );

next:
    IGameController_Release( game_controller );
    IRawGameController_Release( raw_controller );
    SetEvent( stop_event );
    res = msg_wait_for_events( 1, &custom_factory.removed_event, 500 );
    todo_wine
    ok( !res, "msg_wait_for_events returned %#lx\n", res );
    msg_wait_for_events( 1, &controller_removed.event, 5000 );

    hr = IRawGameControllerStatics_remove_RawGameControllerAdded( statics, controller_added_token );
    ok( hr == S_OK, "remove_RawGameControllerAdded returned %#lx\n", hr );
    hr = IRawGameControllerStatics_remove_RawGameControllerRemoved( statics, controller_removed_token );
    ok( hr == S_OK, "remove_RawGameControllerRemoved returned %#lx\n", hr );
    hr = IRawGameControllerStatics_remove_RawGameControllerRemoved( statics, controller_removed_token );
    ok( hr == S_OK, "remove_RawGameControllerRemoved returned %#lx\n", hr );

    IVectorView_RawGameController_Release( controller_view );

    IGameControllerFactoryManagerStatics2_Release( manager_statics2 );
    IGameControllerFactoryManagerStatics_Release( manager_statics );
    IRawGameControllerStatics_Release( statics );
    res = WaitForSingleObject( thread, 5000 );
    ok( !res, "WaitForSingleObject returned %#lx\n", res );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );

    DestroyWindow( hwnd );
    UnregisterClassW( class.lpszClassName, class.hInstance );

    CloseHandle( custom_factory.added_event );
    CloseHandle( custom_factory.removed_event );
    CloseHandle( controller_added.event );
    CloseHandle( controller_removed.event );
}

START_TEST( hotplug )
{
    dinput_test_init();
    if (!bus_device_start()) goto done;

    if (test_input_lost( 0x500 ))
    {
        test_input_lost( 0x700 );
        test_input_lost( 0x800 );

        test_RegisterDeviceNotification();
        test_windows_gaming_input();
    }

done:
    bus_device_stop();
    dinput_test_exit();
}
