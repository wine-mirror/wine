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

#include "wine/hid.h"

#include "dinput_test.h"

#include "initguid.h"
#include "roapi.h"
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Gaming_Input
#include "windows.gaming.input.h"

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

    winetest_push_context( "%#lx", version );

    GetCurrentDirectoryW( ARRAY_SIZE(cwd), cwd );
    GetTempPathW( ARRAY_SIZE(tempdir), tempdir );
    SetCurrentDirectoryW( tempdir );

    cleanup_registry_keys();
    if (!dinput_driver_start( report_desc, sizeof(report_desc), &hid_caps, NULL, 0 )) goto done;
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

    pnp_driver_stop();

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

    dinput_driver_start( report_desc, sizeof(report_desc), &hid_caps, NULL, 0 );

    hr = IDirectInputDevice8_Acquire( device );
    todo_wine
    ok( hr == DIERR_UNPLUGGED, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    todo_wine
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#lx\n", hr );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

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
        MsgWaitForMultipleObjects( 0, NULL, FALSE, INFINITE, QS_ALLINPUT );
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
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

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
        MsgWaitForMultipleObjects( 0, NULL, FALSE, INFINITE, QS_ALLINPUT );
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
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );

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
        MsgWaitForMultipleObjects( 0, NULL, FALSE, INFINITE, QS_ALLINPUT );
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

struct controller_handler
{
    IEventHandler_RawGameController IEventHandler_RawGameController_iface;
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

    trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
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

    trace( "iface %p, sender %p, controller %p\n", iface, sender, controller );

    ok( sender == NULL, "got sender %p\n", sender );
    impl->invoked = TRUE;

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

static LRESULT CALLBACK windows_gaming_input_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_DEVICECHANGE)
    {
        winetest_push_context( "%u", device_change_count );
        if (device_change_count++ >= device_change_expect / 2)
        {
            ok( wparam == DBT_DEVICEREMOVECOMPLETE, "got wparam %#Ix\n", wparam );
            ok( controller_added.invoked, "controller added handler not invoked\n" );
            ok( controller_removed.invoked, "controller removed handler not invoked\n" );
        }
        else
        {
            ok( wparam == DBT_DEVICEARRIVAL, "got wparam %#Ix\n", wparam );
            ok( !controller_added.invoked, "controller added handler not invoked\n" );
            ok( !controller_removed.invoked, "controller removed handler invoked\n" );
        }
        winetest_pop_context();
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void test_windows_gaming_input(void)
{
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
    EventRegistrationToken controller_removed_token;
    IVectorView_RawGameController *controller_view;
    EventRegistrationToken controller_added_token;
    IRawGameControllerStatics *statics;
    HANDLE hwnd, thread, stop_event;
    HDEVNOTIFY devnotify;
    BOOL removed;
    HSTRING str;
    UINT32 size;
    HRESULT hr;
    MSG msg;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == RPC_E_CHANGED_MODE, "RoInitialize failed, hr %#lx\n", hr );

    hr = WindowsCreateString( class_name, wcslen( class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr );

    hr = RoGetActivationFactory( str, &IID_IRawGameControllerStatics, (void **)&statics );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "RoGetActivationFactory failed, hr %#lx\n", hr );
    WindowsDeleteString( str );

    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        RoUninitialize();
        return;
    }

    hr = IRawGameControllerStatics_add_RawGameControllerAdded( statics, &controller_added.IEventHandler_RawGameController_iface,
                                                               &controller_added_token );
    ok( hr == S_OK, "add_RawGameControllerAdded returned %#lx\n", hr );
    todo_wine
    ok( controller_added_token.value, "got token %I64u\n", controller_added_token.value );
    if (!controller_added_token.value) goto done;

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
    device_change_hwnd = hwnd;
    device_change_all = FALSE;
    stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stop_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread = CreateThread( NULL, 0, dinput_test_device_thread, stop_event, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );

    removed = FALSE;
    while (device_change_count < device_change_expect)
    {
        MsgWaitForMultipleObjects( 0, NULL, FALSE, 50, QS_ALLINPUT );
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            ok( msg.message != WM_DEVICECHANGE, "got WM_DEVICECHANGE\n" );
            DispatchMessageW( &msg );
        }
        if (controller_added.invoked && !removed)
        {
            ok( !controller_removed.invoked, "controller removed handler invoked\n" );
            removed = TRUE;

            hr = IVectorView_RawGameController_get_Size( controller_view, &size );
            ok( hr == S_OK, "get_Size returned %#lx\n", hr );
            ok( size == 0, "got size %u\n", size );

            IVectorView_RawGameController_Release( controller_view );
            hr = IRawGameControllerStatics_get_RawGameControllers( statics, &controller_view );
            ok( hr == S_OK, "get_RawGameControllers returned %#lx\n", hr );

            hr = IVectorView_RawGameController_get_Size( controller_view, &size );
            ok( hr == S_OK, "get_Size returned %#lx\n", hr );
            ok( size == 1, "got size %u\n", size );

            SetEvent( stop_event );
        }
    }

    ok( controller_added.invoked, "controller added handler not invoked\n" );
    ok( controller_removed.invoked, "controller removed handler not invoked\n" );

    hr = IVectorView_RawGameController_get_Size( controller_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 1, "got size %u\n", size );

    IVectorView_RawGameController_Release( controller_view );
    hr = IRawGameControllerStatics_get_RawGameControllers( statics, &controller_view );
    ok( hr == S_OK, "get_RawGameControllers returned %#lx\n", hr );

    hr = IVectorView_RawGameController_get_Size( controller_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 0, "got size %u\n", size );

    hr = IRawGameControllerStatics_remove_RawGameControllerAdded( statics, controller_added_token );
    ok( hr == S_OK, "remove_RawGameControllerAdded returned %#lx\n", hr );
    hr = IRawGameControllerStatics_remove_RawGameControllerRemoved( statics, controller_removed_token );
    ok( hr == S_OK, "remove_RawGameControllerRemoved returned %#lx\n", hr );
    hr = IRawGameControllerStatics_remove_RawGameControllerRemoved( statics, controller_removed_token );
    ok( hr == S_OK, "remove_RawGameControllerRemoved returned %#lx\n", hr );

    IVectorView_RawGameController_Release( controller_view );
    IRawGameControllerStatics_Release( statics );

    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( stop_event );

    UnregisterDeviceNotification( devnotify );

    DestroyWindow( hwnd );
    UnregisterClassW( class.lpszClassName, class.hInstance );

done:
    RoUninitialize();
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
        test_windows_gaming_input();
    }
    CoUninitialize();

    dinput_test_exit();
}
