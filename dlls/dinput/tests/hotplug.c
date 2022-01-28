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

#include "wine/hid.h"

#include "dinput_test.h"

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
#include "pop_hid_macros.h"

    static const HIDP_CAPS hid_caps =
    {
        .InputReportByteLength = 1,
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
    WCHAR cwd[MAX_PATH], tempdir[MAX_PATH];
    IDirectInputDevice8W *device = NULL;
    ULONG ref, count, size;
    DIJOYSTATE2 state;
    HRESULT hr;

    winetest_push_context( "%#x", version );

    GetCurrentDirectoryW( ARRAY_SIZE(cwd), cwd );
    GetTempPathW( ARRAY_SIZE(tempdir), tempdir );
    SetCurrentDirectoryW( tempdir );

    cleanup_registry_keys();
    if (!dinput_driver_start( report_desc, sizeof(report_desc), &hid_caps, NULL, 0 )) goto done;
    if (FAILED(hr = dinput_test_create_device( version, &devinst, &device ))) goto done;

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned %#x\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, 0, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#x\n", hr );
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_BUFFERSIZE, &buffer_size.diph );
    ok( hr == DI_OK, "SetProperty returned %#x\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DI_OK, "GetDeviceState returned %#x\n", hr );
    size = version < 0x0800 ? sizeof(DIDEVICEOBJECTDATA_DX3) : sizeof(DIDEVICEOBJECTDATA);
    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DI_OK, "GetDeviceData returned %#x\n", hr );
    ok( count == 0, "got %u expected %u\n", count, 0 );

    pnp_driver_stop();

    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_INPUTLOST, "GetDeviceState returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_INPUTLOST, "GetDeviceState returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DIERR_INPUTLOST, "GetDeviceData returned %#x\n", hr );
    hr = IDirectInputDevice8_Poll( device );
    ok( hr == DIERR_INPUTLOST, "Poll returned: %#x\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DIERR_UNPLUGGED, "Acquire returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceData returned %#x\n", hr );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_NOEFFECT, "Unacquire returned: %#x\n", hr );

    dinput_driver_start( report_desc, sizeof(report_desc), &hid_caps, NULL, 0 );

    hr = IDirectInputDevice8_Acquire( device );
    todo_wine
    ok( hr == DIERR_UNPLUGGED, "Acquire returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    todo_wine
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#x\n", hr );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %d\n", ref );

done:
    pnp_driver_stop();
    cleanup_registry_keys();
    SetCurrentDirectoryW( cwd );

    winetest_pop_context();
    return device != NULL;
}

static int device_change_count;
static int device_change_expect;
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

static LRESULT CALLBACK devnotify_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_DEVICECHANGE)
    {
        DEV_BROADCAST_HDR *header = (DEV_BROADCAST_HDR *)lparam;
        DEV_BROADCAST_DEVICEINTERFACE_W *iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)lparam;
        const WCHAR *upper_end, *name_end, *expect_prefix;
        GUID expect_guid;

        if (device_change_all && (device_change_count == 0 || device_change_count == 3))
        {
            expect_guid = control_class;
            expect_prefix = L"\\\\?\\ROOT#";
        }
        else
        {
            expect_guid = GUID_DEVINTERFACE_HID;
            expect_prefix = L"\\\\?\\HID#";
        }

        ok( hwnd == device_change_hwnd, "got hwnd %p\n", hwnd );
        ok( header->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE, "got dbch_devicetype %u\n",
            header->dbch_devicetype );

        winetest_push_context( "%u", device_change_count );

        ok( IsEqualGUID( &iface->dbcc_classguid, &expect_guid ), "got dbch_classguid %s\n",
            debugstr_guid( &iface->dbcc_classguid ) );
        ok( iface->dbcc_size >= offsetof( DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name[wcslen( iface->dbcc_name ) + 1] ),
            "got dbcc_size %u\n", iface->dbcc_size );
        ok( !wcsncmp( iface->dbcc_name, expect_prefix, wcslen( expect_prefix ) ),
            "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );

        upper_end = wcschr( iface->dbcc_name + wcslen( expect_prefix ), '#' );
        name_end = iface->dbcc_name + wcslen( iface->dbcc_name ) + 1;
        ok( !!upper_end, "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );
        ok( all_upper( iface->dbcc_name, upper_end ), "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );
        ok( all_lower( upper_end, name_end ), "got dbcc_name %s\n", debugstr_w(iface->dbcc_name) );

        if (device_change_count++ >= device_change_expect / 2)
            ok( wparam == DBT_DEVICEREMOVECOMPLETE, "got wparam %#x\n", (DWORD)wparam );
        else
            ok( wparam == DBT_DEVICEARRIVAL, "got wparam %#x\n", (DWORD)wparam );

        winetest_pop_context();
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void test_RegisterDeviceNotification(void)
{
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
        .lpfnWndProc = devnotify_wndproc,
    };
    char buffer[1024] = {0};
    DEV_BROADCAST_HDR *header = (DEV_BROADCAST_HDR *)buffer;
    HANDLE hwnd, thread, stop_event;
    HDEVNOTIFY devnotify;
    MSG msg;

    RegisterClassExW( &class );

    hwnd = CreateWindowW( class.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    devnotify = RegisterDeviceNotificationA( NULL, NULL, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    devnotify = RegisterDeviceNotificationA( (HWND)0xdeadbeef, NULL, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    devnotify = RegisterDeviceNotificationA( hwnd, NULL, 2 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_OEM) );
    header->dbch_size = sizeof(DEV_BROADCAST_OEM);
    header->dbch_devicetype = DBT_DEVTYP_OEM;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_DEVNODE) );
    header->dbch_size = sizeof(DEV_BROADCAST_DEVNODE);
    header->dbch_devicetype = DBT_DEVTYP_DEVNODE;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_VOLUME) );
    header->dbch_size = sizeof(DEV_BROADCAST_VOLUME);
    header->dbch_devicetype = DBT_DEVTYP_VOLUME;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_PORT_A) );
    header->dbch_size = sizeof(DEV_BROADCAST_PORT_A);
    header->dbch_devicetype = DBT_DEVTYP_PORT;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( header, 0, sizeof(DEV_BROADCAST_NET) );
    header->dbch_size = sizeof(DEV_BROADCAST_NET);
    header->dbch_devicetype = DBT_DEVTYP_NET;
    devnotify = RegisterDeviceNotificationA( hwnd, header, 0 );
    ok( !devnotify, "RegisterDeviceNotificationA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_SERVICE_SPECIFIC_ERROR,
        "got error %u\n", GetLastError() );

    devnotify = RegisterDeviceNotificationA( hwnd, &iface_filter_a, DEVICE_NOTIFY_WINDOW_HANDLE );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %u\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    device_change_count = 0;
    device_change_expect = 2;
    device_change_hwnd = hwnd;
    device_change_all = FALSE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %u\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %u\n", GetLastError() );

    while (device_change_count < device_change_expect)
    {
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (device_change_count == device_change_expect / 2) SetEvent( stop_event );
    }

    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );

    memcpy( buffer, &iface_filter_a, sizeof(iface_filter_a) );
    strcpy( ((DEV_BROADCAST_DEVICEINTERFACE_A *)buffer)->dbcc_name, "device name" );
    ((DEV_BROADCAST_DEVICEINTERFACE_A *)buffer)->dbcc_size += strlen( "device name" ) + 1;
    devnotify = RegisterDeviceNotificationA( hwnd, buffer, DEVICE_NOTIFY_WINDOW_HANDLE );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %u\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    device_change_count = 0;
    device_change_expect = 2;
    device_change_hwnd = hwnd;
    device_change_all = FALSE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %u\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %u\n", GetLastError() );

    while (device_change_count < device_change_expect)
    {
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (device_change_count == device_change_expect / 2) SetEvent( stop_event );
    }

    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );

    devnotify = RegisterDeviceNotificationA( hwnd, &iface_filter_a, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %u\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

    device_change_count = 0;
    device_change_expect = 4;
    device_change_hwnd = hwnd;
    device_change_all = TRUE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %u\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %u\n", GetLastError() );

    while (device_change_count < device_change_expect)
    {
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (device_change_count == device_change_expect / 2) SetEvent( stop_event );
    }

    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );

    DestroyWindow( hwnd );
    UnregisterClassW( class.lpszClassName, class.hInstance );
}

START_TEST( hotplug )
{
    if (!dinput_test_init()) return;

    CoInitialize( NULL );
    if (test_input_lost( 0x500 ))
    {
        test_input_lost( 0x700 );
        test_input_lost( 0x800 );

        test_RegisterDeviceNotification();
    }
    CoUninitialize();

    dinput_test_exit();
}
