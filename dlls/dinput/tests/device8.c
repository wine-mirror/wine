/*
 * Copyright (c) 2011 Lucas Fialho Zawacki
 * Copyright (c) 2006 Vitaliy Margolen
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
#include "hidusage.h"

#include "dinput_test.h"

#include "wine/hid.h"

#include "initguid.h"

DEFINE_GUID(GUID_keyboard_action_mapping,0x00000001,0x0002,0x0003,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b);

struct enum_data
{
    DWORD version;
    union
    {
        IDirectInput8A *dinput8;
        IDirectInputA *dinput;
    };
};

static void flush_events(void)
{
    int min_timeout = 100, diff = 200;
    DWORD time = GetTickCount() + diff;
    MSG msg;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageA( &msg );
        }
        diff = time - GetTickCount();
    }
}

HWND create_foreground_window_( const char *file, int line, BOOL fullscreen, UINT retries )
{
    for (;;)
    {
        HWND hwnd;
        BOOL ret;

        hwnd = CreateWindowW( L"static", NULL, WS_POPUP | (fullscreen ? 0 : WS_VISIBLE),
                              100, 100, 200, 200, NULL, NULL, NULL, NULL );
        ok_(file, line)( hwnd != NULL, "CreateWindowW failed, error %lu\n", GetLastError() );

        if (fullscreen)
        {
            HMONITOR hmonitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY );
            MONITORINFO mi = {.cbSize = sizeof(MONITORINFO)};

            ok_(file, line)( hmonitor != NULL, "MonitorFromWindow failed, error %lu\n", GetLastError() );
            ret = GetMonitorInfoW( hmonitor, &mi );
            ok_(file, line)( ret, "GetMonitorInfoW failed, error %lu\n", GetLastError() );
            ret = SetWindowPos( hwnd, 0, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                                mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW );
            ok_(file, line)( ret, "SetWindowPos failed, error %lu\n", GetLastError() );
        }
        flush_events();

        if (GetForegroundWindow() == hwnd) return hwnd;
        ok_(file, line)( retries > 0, "failed to create foreground window\n" );
        if (!retries--) return hwnd;

        ret = DestroyWindow( hwnd );
        ok_(file, line)( ret, "DestroyWindow failed, error %lu\n", GetLastError() );
        flush_events();
    }
}

static HRESULT create_dinput_device( DWORD version, const GUID *guid, IDirectInputDevice8W **device )
{
    IDirectInputW *dinput;
    HRESULT hr;
    ULONG ref;

    if (version < 0x800) hr = DirectInputCreateW( instance, version, &dinput, NULL );
    else hr = DirectInput8Create( instance, version, &IID_IDirectInput8W, (void **)&dinput, NULL );
    if (FAILED(hr))
    {
        win_skip( "Failed to instantiate a IDirectInput instance, hr %#lx\n", hr );
        return hr;
    }

    hr = IDirectInput_CreateDevice( dinput, guid, (IDirectInputDeviceW **)device, NULL );
    ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );

    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %ld\n", ref );

    return DI_OK;
}

static HKL activate_keyboard_layout( LANGID langid, HKL *old_hkl )
{
    WCHAR hkl_name[64];
    HKL hkl;

    swprintf( hkl_name, ARRAY_SIZE(hkl_name), L"%08x", langid );
    hkl = LoadKeyboardLayoutW( hkl_name, 0 );
    if (!hkl)
    {
        win_skip( "Unable to load keyboard layout %#x\n", langid );
        *old_hkl = GetKeyboardLayout( 0 );
        return 0;
    }

    *old_hkl = ActivateKeyboardLayout( hkl, 0 );
    ok( !!*old_hkl, "ActivateKeyboardLayout failed, error %lu\n", GetLastError() );

    hkl = GetKeyboardLayout( 0 );
    todo_wine_if( LOWORD(*old_hkl) != langid )
    ok( LOWORD(hkl) == langid, "GetKeyboardLayout returned %p\n", hkl );
    return hkl;
}

static HRESULT direct_input_create( DWORD version, IDirectInputA **out )
{
    HRESULT hr;
    if (version < 0x800) hr = DirectInputCreateA( instance, version, out, NULL );
    else hr = DirectInput8Create( instance, version, &IID_IDirectInput8A, (void **)out, NULL );
    if (FAILED(hr)) win_skip( "Failed to instantiate a IDirectInput instance, hr %#lx\n", hr );
    return hr;
}

#define check_interface( a, b, c ) check_interface_( __LINE__, a, b, c )
static void check_interface_( unsigned int line, void *iface_ptr, REFIID iid, BOOL supported )
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected;
    IUnknown *unk;

    expected = supported ? S_OK : E_NOINTERFACE;
    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == expected, "got hr %#lx, expected %#lx.\n", hr, expected );
    if (SUCCEEDED(hr)) IUnknown_Release( unk );
}

static BOOL CALLBACK check_device_query_interface( const DIDEVICEINSTANCEA *instance, void *context )
{
    struct enum_data *data = context;
    IUnknown *device;
    HRESULT hr;
    LONG ref;

    if (data->version < 0x800)
    {
        hr = IDirectInput_GetDeviceStatus( data->dinput, &instance->guidInstance );
        ok( hr == DI_OK, "GetDeviceStatus returned %#lx\n", hr );
        hr = IDirectInput_GetDeviceStatus( data->dinput, &instance->guidProduct );
        ok( hr == DI_OK, "GetDeviceStatus returned %#lx\n", hr );

        hr = IDirectInput_CreateDevice( data->dinput, &instance->guidProduct, (IDirectInputDeviceA **)&device, NULL );
        ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );
        ref = IUnknown_Release( device );
        ok( ref == 0, "Release returned %ld\n", ref );

        hr = IDirectInput_CreateDevice( data->dinput, &instance->guidInstance, (IDirectInputDeviceA **)&device, NULL );
        ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );
    }
    else
    {
        hr = IDirectInput8_GetDeviceStatus( data->dinput8, &instance->guidInstance );
        ok( hr == DI_OK, "GetDeviceStatus returned %#lx\n", hr );
        hr = IDirectInput8_GetDeviceStatus( data->dinput8, &instance->guidProduct );
        ok( hr == DI_OK, "GetDeviceStatus returned %#lx\n", hr );

        hr = IDirectInput8_CreateDevice( data->dinput8, &instance->guidProduct, (IDirectInputDevice8A **)&device, NULL );
        ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );
        ref = IUnknown_Release( device );
        ok( ref == 0, "Release returned %ld\n", ref );

        hr = IDirectInput8_CreateDevice( data->dinput8, &instance->guidInstance, (IDirectInputDevice8A **)&device, NULL );
        ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );
    }

    todo_wine_if( data->version >= 0x800 )
    check_interface( device, &IID_IDirectInputDeviceA, data->version < 0x800 );
    todo_wine_if( data->version >= 0x800 )
    check_interface( device, &IID_IDirectInputDevice2A, data->version < 0x800 );
    todo_wine_if( data->version >= 0x800 )
    check_interface( device, &IID_IDirectInputDevice7A, data->version < 0x800 );
    todo_wine_if( data->version < 0x800 )
    check_interface( device, &IID_IDirectInputDevice8A, data->version >= 0x800 );

    todo_wine_if( data->version >= 0x800 )
    check_interface( device, &IID_IDirectInputDeviceW, data->version < 0x800 );
    todo_wine_if( data->version >= 0x800 )
    check_interface( device, &IID_IDirectInputDevice2W, data->version < 0x800 );
    todo_wine_if( data->version >= 0x800 )
    check_interface( device, &IID_IDirectInputDevice7W, data->version < 0x800 );
    todo_wine_if( data->version < 0x800 )
    check_interface( device, &IID_IDirectInputDevice8W, data->version >= 0x800 );

    ref = IUnknown_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

    return DIENUM_CONTINUE;
}

static void test_QueryInterface( DWORD version )
{
    struct enum_data data = {.version = version};
    HRESULT hr;
    ULONG ref;

    if (FAILED(hr = direct_input_create( version, &data.dinput ))) return;

    winetest_push_context( "%#lx", version );

    if (version < 0x800)
    {
        hr = IDirectInput_EnumDevices( data.dinput, 0, check_device_query_interface, &data, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned %#lx\n", hr );
    }
    else
    {
        hr = IDirectInput8_EnumDevices( data.dinput8, 0, check_device_query_interface, &data, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned %#lx\n", hr );
    }

    ref = IDirectInput_Release( data.dinput );
    ok( ref == 0, "Release returned %lu\n", ref );

    winetest_pop_context();
}

struct overlapped_state
{
    BYTE keys[4];
    DWORD extra_element;
};

static DIOBJECTDATAFORMAT obj_overlapped_slider_format[] =
{
    {&GUID_Key, 0, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_A ), 0},
    {&GUID_Key, 1, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_S ), 0},
    {&GUID_Key, 2, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_D ), 0},
    {&GUID_Key, 3, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_F ), 0},
    {&GUID_Slider, 0, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION},
};

static const DIDATAFORMAT overlapped_slider_format =
{
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(struct overlapped_state),
    ARRAY_SIZE(obj_overlapped_slider_format),
    obj_overlapped_slider_format,
};

static DIOBJECTDATAFORMAT obj_overlapped_pov_format[] =
{
    {&GUID_Key, 0, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_A ), 0},
    {&GUID_Key, 1, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_S ), 0},
    {&GUID_Key, 2, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_D ), 0},
    {&GUID_Key, 3, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE( DIK_F ), 0},
    {&GUID_POV, 0, DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0},
};

static const DIDATAFORMAT overlapped_pov_format =
{
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(struct overlapped_state),
    ARRAY_SIZE(obj_overlapped_pov_format),
    obj_overlapped_pov_format,
};

void test_overlapped_format( DWORD version )
{
    static const DIPROPDWORD buffer_size =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
        .dwData = 10,
    };
    SIZE_T data_size = version < 0x800 ? sizeof(DIDEVICEOBJECTDATA_DX3) : sizeof(DIDEVICEOBJECTDATA);
    struct overlapped_state state;
    IDirectInputDeviceA *keyboard;
    IDirectInputA *dinput;
    DWORD res, count;
    HANDLE event;
    HRESULT hr;
    HWND hwnd;

    if (FAILED(hr = direct_input_create( version, &dinput ))) return;

    winetest_push_context( "%#lx", version );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!event, "CreateEventW failed, error %lu\n", GetLastError() );
    hwnd = create_foreground_window( FALSE );

    hr = IDirectInput_CreateDevice( dinput, &GUID_SysKeyboard, &keyboard, NULL );
    ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( keyboard, hwnd, DISCL_FOREGROUND|DISCL_EXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetEventNotification( keyboard, event );
    ok( hr == DI_OK, "SetEventNotification returned %#lx\n", hr );

    /* test overlapped slider - default value 0 */
    hr = IDirectInputDevice_SetDataFormat( keyboard, &overlapped_slider_format );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );
    hr = IDirectInputDevice_SetProperty( keyboard, DIPROP_BUFFERSIZE, &buffer_size.diph );
    ok( hr == DI_OK, "SetProperty returned %#lx\n", hr );


    hr = IDirectInputDevice_Acquire( keyboard );
    ok( hr == DI_OK, "Acquire returned %#lx, skipping test_overlapped_format\n", hr );
    if (hr != DI_OK) goto cleanup;

    keybd_event( 0, DIK_F, KEYEVENTF_SCANCODE, 0 );
    res = WaitForSingleObject( event, 100 );
    if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
    {
        keybd_event( 0, DIK_F, KEYEVENTF_SCANCODE, 0 );
        res = WaitForSingleObject( event, 5000 );
    }
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    keybd_event( 0, DIK_F, KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0 );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    count = 10;
    hr = IDirectInputDevice_GetDeviceData( keyboard, data_size, NULL, &count, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( count > 0, "got count %lu\n", count );


    /* press D */
    keybd_event( 0, DIK_D, KEYEVENTF_SCANCODE, 0 );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    count = 10;
    hr = IDirectInputDevice_GetDeviceData( keyboard, data_size, NULL, &count, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( count == 1, "got count %lu\n", count );

    memset( &state, 0xFF, sizeof(state) );
    hr = IDirectInputDevice_GetDeviceState( keyboard, sizeof(state), &state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );

    ok( state.keys[0] == 0x00, "key A should be still up\n" );
    ok( state.keys[1] == 0x00, "key S should be still up\n" );
    ok( state.keys[2] == 0x80, "keydown for D did not register\n" );
    ok( state.keys[3] == 0x00, "key F should be still up\n" );
    ok( state.extra_element == 0, "State struct was not memset to zero\n" );

    /* release D */
    keybd_event( 0, DIK_D, KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0 );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    count = 10;
    hr = IDirectInputDevice_GetDeviceData( keyboard, data_size, NULL, &count, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( count == 1, "got count %lu\n", count );


    hr = IDirectInputDevice_Unacquire( keyboard );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );

    /* test overlapped pov - default value - 0xFFFFFFFF */
    hr = IDirectInputDevice_SetDataFormat( keyboard, &overlapped_pov_format );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );


    hr = IDirectInputDevice_Acquire( keyboard );
    ok( hr == DI_OK, "Acquire returned %#lx, skipping test_overlapped_format\n", hr );
    if (hr != DI_OK) goto cleanup;

    keybd_event( 0, DIK_F, KEYEVENTF_SCANCODE, 0 );
    res = WaitForSingleObject( event, 100 );
    if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
    {
        keybd_event( 0, DIK_F, KEYEVENTF_SCANCODE, 0 );
        res = WaitForSingleObject( event, 5000 );
    }
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    keybd_event( 0, DIK_F, KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0 );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    count = 10;
    hr = IDirectInputDevice_GetDeviceData( keyboard, data_size, NULL, &count, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( count > 0, "got count %lu\n", count );


    /* press D */
    keybd_event( 0, DIK_D, KEYEVENTF_SCANCODE, 0 );
    res = WaitForSingleObject( event, 5000 );
    flaky_wine_if( GetForegroundWindow() != hwnd && version == 0x800 ) /* FIXME: fvwm sometimes steals input focus */
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    count = 10;
    hr = IDirectInputDevice_GetDeviceData( keyboard, data_size, NULL, &count, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    flaky_wine_if( GetForegroundWindow() != hwnd && version == 0x800 ) /* FIXME: fvwm sometimes steals input focus */
    ok( count == 1, "got count %lu\n", count );

    memset( &state, 0xFF, sizeof(state) );
    hr = IDirectInputDevice_GetDeviceState( keyboard, sizeof(state), &state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );

    ok( state.keys[0] == 0xFF, "key state should have been overwritten by the overlapped POV\n" );
    ok( state.keys[1] == 0xFF, "key state should have been overwritten by the overlapped POV\n" );
    ok( state.keys[2] == 0xFF, "key state should have been overwritten by the overlapped POV\n" );
    ok( state.keys[3] == 0xFF, "key state should have been overwritten by the overlapped POV\n" );
    ok( state.extra_element == 0, "State struct was not memset to zero\n" );

    /* release D */
    keybd_event( 0, DIK_D, KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0 );
    res = WaitForSingleObject( event, 5000 );
    flaky_wine_if( GetForegroundWindow() != hwnd && version == 0x800 ) /* FIXME: fvwm sometimes steals input focus */
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    count = 10;
    hr = IDirectInputDevice_GetDeviceData( keyboard, data_size, NULL, &count, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    flaky_wine_if( GetForegroundWindow() != hwnd && version == 0x800 ) /* FIXME: fvwm sometimes steals input focus */
    ok( count == 1, "got count %lu\n", count );


cleanup:
    IUnknown_Release( keyboard );
    IDirectInput_Release( dinput );

    DestroyWindow( hwnd );
    CloseHandle( event );

    winetest_pop_context();
}

static void test_device_input( IDirectInputDevice8A *device, DWORD type, DWORD code, UINT_PTR expected )
{
    HRESULT hr;
    DIDEVICEOBJECTDATA obj_data;
    DWORD res, data_size = 1;
    HANDLE event;
    int i;

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( event != NULL, "CreateEventW failed, error %lu\n", GetLastError() );

    IDirectInputDevice_Unacquire( device );

    hr = IDirectInputDevice8_SetEventNotification( device, event );
    ok( hr == DI_OK, "SetEventNotification returned %#lx\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx\n", hr );

    if (type == INPUT_KEYBOARD)
    {
        keybd_event( 0, code, KEYEVENTF_SCANCODE, 0 );
        res = WaitForSingleObject( event, 100 );
        if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
        {
            keybd_event( 0, code, KEYEVENTF_SCANCODE, 0 );
            res = WaitForSingleObject( event, 5000 );
        }
        ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

        keybd_event( 0, code, KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0 );
        res = WaitForSingleObject( event, 5000 );
        ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );
    }
    if (type == INPUT_MOUSE)
    {
        mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
        res = WaitForSingleObject( event, 100 );
        if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
        {
            mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
            res = WaitForSingleObject( event, 5000 );
        }
        ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

        mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
        res = WaitForSingleObject( event, 5000 );
        ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );
    }

    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(obj_data), &obj_data, &data_size, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( data_size == 1, "got data size %lu, expected 1\n", data_size );
    ok( obj_data.uAppData == expected, "got action uAppData %p, expected %p\n",
        (void *)obj_data.uAppData, (void *)expected );

    /* Check for buffer overflow */
    for (i = 0; i < 17; i++)
    {
        if (type == INPUT_KEYBOARD)
        {
            keybd_event( VK_SPACE, DIK_SPACE, 0, 0 );
            res = WaitForSingleObject( event, 5000 );
            ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

            keybd_event( VK_SPACE, DIK_SPACE, KEYEVENTF_KEYUP, 0 );
            res = WaitForSingleObject( event, 5000 );
            ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );
        }
        if (type == INPUT_MOUSE)
        {
            mouse_event( MOUSEEVENTF_LEFTDOWN, 1, 1, 0, 0 );
            res = WaitForSingleObject( event, 5000 );
            ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

            mouse_event( MOUSEEVENTF_LEFTUP, 1, 1, 0, 0 );
            res = WaitForSingleObject( event, 5000 );
            ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );
        }
    }

    data_size = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(obj_data), &obj_data, &data_size, 0 );
    ok( hr == DI_BUFFEROVERFLOW, "GetDeviceData returned %#lx\n", hr );
    data_size = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(obj_data), &obj_data, &data_size, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( data_size == 1, "got data_size %lu, expected 1\n", data_size );

    /* drain device's queue */
    while (data_size == 1)
    {
        hr = IDirectInputDevice8_GetDeviceData( device, sizeof(obj_data), &obj_data, &data_size, 0 );
        ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
        if (hr != DI_OK) break;
    }

    hr = IDirectInputDevice_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetEventNotification( device, NULL );
    ok( hr == DI_OK, "SetEventNotification returned %#lx\n", hr );

    CloseHandle( event );
}

static void test_mouse_keyboard(void)
{
    HRESULT hr;
    HWND hwnd, di_hwnd = INVALID_HANDLE_VALUE;
    IDirectInput8A *di = NULL;
    IDirectInputDevice8A *di_mouse, *di_keyboard;
    UINT raw_devices_count;
    RAWINPUTDEVICE raw_devices[3];

    hr = CoCreateInstance(&CLSID_DirectInput8, 0, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (LPVOID*)&di);
    if (hr == DIERR_OLDDIRECTINPUTVERSION ||
        hr == DIERR_BETADIRECTINPUTVERSION ||
        hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("test_mouse_keyboard requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8Create failed: %#lx\n", hr);

    hr = IDirectInput8_Initialize(di, GetModuleHandleA(NULL), DIRECTINPUT_VERSION);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_BETADIRECTINPUTVERSION)
    {
        win_skip("test_mouse_keyboard requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "IDirectInput8_Initialize failed: %#lx\n", hr);

    hwnd = create_foreground_window( TRUE );

    hr = IDirectInput8_CreateDevice(di, &GUID_SysMouse, &di_mouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput8_CreateDevice failed: %#lx\n", hr);
    hr = IDirectInputDevice8_SetDataFormat(di_mouse, &c_dfDIMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetDataFormat failed: %#lx\n", hr);

    hr = IDirectInput8_CreateDevice(di, &GUID_SysKeyboard, &di_keyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput8_CreateDevice failed: %#lx\n", hr);
    hr = IDirectInputDevice8_SetDataFormat(di_keyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetDataFormat failed: %#lx\n", hr);

    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(raw_devices_count == 0, "Unexpected raw devices registered: %d\n", raw_devices_count);

    hr = IDirectInputDevice8_Acquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    memset(raw_devices, 0, sizeof(raw_devices));
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(hr == 1, "GetRegisteredRawInputDevices returned %ld, raw_devices_count: %d\n", hr, raw_devices_count);
    ok(raw_devices[0].usUsagePage == HID_USAGE_PAGE_GENERIC, "got usUsagePage: %x\n", raw_devices[0].usUsagePage);
    ok(raw_devices[0].usUsage == HID_USAGE_GENERIC_KEYBOARD, "got usUsage: %x\n", raw_devices[0].usUsage);
    todo_wine
    ok(raw_devices[0].dwFlags == RIDEV_INPUTSINK, "Unexpected raw device flags: %#lx\n", raw_devices[0].dwFlags);
    ok(raw_devices[0].hwndTarget != NULL, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);
    hr = IDirectInputDevice8_Unacquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(raw_devices_count == 0, "Unexpected raw devices registered: %d\n", raw_devices_count);

    if (raw_devices[0].hwndTarget != NULL)
    {
        WCHAR str[16];
        int i;

        di_hwnd = raw_devices[0].hwndTarget;
        i = GetClassNameW(di_hwnd, str, ARRAY_SIZE(str));
        ok(i == lstrlenW(L"DIEmWin"), "GetClassName returned incorrect length\n");
        ok(!lstrcmpW(L"DIEmWin", str), "GetClassName returned incorrect name for this window's class\n");

        i = GetWindowTextW(di_hwnd, str, ARRAY_SIZE(str));
        ok(i == lstrlenW(L"DIEmWin"), "GetClassName returned incorrect length\n");
        ok(!lstrcmpW(L"DIEmWin", str), "GetClassName returned incorrect name for this window's class\n");
    }

    hr = IDirectInputDevice8_Acquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    memset(raw_devices, 0, sizeof(raw_devices));
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(hr == 1, "GetRegisteredRawInputDevices returned %ld, raw_devices_count: %d\n", hr, raw_devices_count);
    ok(raw_devices[0].usUsagePage == HID_USAGE_PAGE_GENERIC, "got usUsagePage: %x\n", raw_devices[0].usUsagePage);
    ok(raw_devices[0].usUsage == HID_USAGE_GENERIC_MOUSE, "got usUsage: %x\n", raw_devices[0].usUsage);
    ok(raw_devices[0].dwFlags == RIDEV_INPUTSINK, "Unexpected raw device flags: %#lx\n", raw_devices[0].dwFlags);
    ok(raw_devices[0].hwndTarget == di_hwnd, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);
    hr = IDirectInputDevice8_Unacquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(raw_devices_count == 0, "Unexpected raw devices registered: %d\n", raw_devices_count);

    if (raw_devices[0].hwndTarget != NULL)
        di_hwnd = raw_devices[0].hwndTarget;

    /* expect dinput8 to take over any activated raw input devices */
    raw_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
    raw_devices[0].dwFlags = 0;
    raw_devices[0].hwndTarget = hwnd;
    raw_devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    raw_devices[1].dwFlags = 0;
    raw_devices[1].hwndTarget = hwnd;
    raw_devices[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[2].usUsage = HID_USAGE_GENERIC_MOUSE;
    raw_devices[2].dwFlags = 0;
    raw_devices[2].hwndTarget = hwnd;
    raw_devices_count = ARRAY_SIZE(raw_devices);
    hr = RegisterRawInputDevices(raw_devices, raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(hr == TRUE, "RegisterRawInputDevices failed\n");

    hr = IDirectInputDevice8_Acquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    hr = IDirectInputDevice8_Acquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    memset(raw_devices, 0, sizeof(raw_devices));
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(hr == 3, "GetRegisteredRawInputDevices returned %ld, raw_devices_count: %d\n", hr, raw_devices_count);
    ok(raw_devices[0].usUsagePage == HID_USAGE_PAGE_GENERIC, "got usUsagePage: %x\n", raw_devices[0].usUsagePage);
    ok(raw_devices[0].usUsage == HID_USAGE_GENERIC_MOUSE, "got usUsage: %x\n", raw_devices[0].usUsage);
    ok(raw_devices[0].dwFlags == RIDEV_INPUTSINK, "Unexpected raw device flags: %#lx\n", raw_devices[0].dwFlags);
    ok(raw_devices[0].hwndTarget == di_hwnd, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);
    ok(raw_devices[1].usUsagePage == HID_USAGE_PAGE_GENERIC, "got usUsagePage: %x\n", raw_devices[1].usUsagePage);
    ok(raw_devices[1].usUsage == HID_USAGE_GENERIC_GAMEPAD, "got usUsage: %x\n", raw_devices[1].usUsage);
    ok(raw_devices[1].dwFlags == 0, "Unexpected raw device flags: %#lx\n", raw_devices[1].dwFlags);
    ok(raw_devices[1].hwndTarget == hwnd, "Unexpected raw device target: %p\n", raw_devices[1].hwndTarget);
    ok(raw_devices[2].usUsagePage == HID_USAGE_PAGE_GENERIC, "got usUsagePage: %x\n", raw_devices[1].usUsagePage);
    ok(raw_devices[2].usUsage == HID_USAGE_GENERIC_KEYBOARD, "got usUsage: %x\n", raw_devices[1].usUsage);
    ok(raw_devices[2].hwndTarget == di_hwnd, "Unexpected raw device target: %p\n", raw_devices[1].hwndTarget);
    hr = IDirectInputDevice8_Unacquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    hr = IDirectInputDevice8_Unacquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(raw_devices_count == 1, "Unexpected raw devices registered: %d\n", raw_devices_count);

    IDirectInputDevice8_SetCooperativeLevel(di_mouse, hwnd, DISCL_FOREGROUND|DISCL_EXCLUSIVE);
    IDirectInputDevice8_SetCooperativeLevel(di_keyboard, hwnd, DISCL_FOREGROUND|DISCL_EXCLUSIVE);

    hr = IDirectInputDevice8_Acquire(di_keyboard);
    flaky_wine_if( GetForegroundWindow() != hwnd ) /* FIXME: fvwm sometimes steals input focus */
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    hr = IDirectInputDevice8_Acquire(di_mouse);
    flaky_wine_if( GetForegroundWindow() != hwnd ) /* FIXME: fvwm sometimes steals input focus */
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    memset(raw_devices, 0, sizeof(raw_devices));
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    flaky_wine_if( GetForegroundWindow() != hwnd ) /* FIXME: fvwm sometimes steals input focus */
    ok(hr == 3, "GetRegisteredRawInputDevices returned %ld, raw_devices_count: %d\n", hr, raw_devices_count);
    flaky_wine_if( GetForegroundWindow() != hwnd ) /* FIXME: fvwm sometimes steals input focus */
    ok(raw_devices[0].dwFlags == (RIDEV_CAPTUREMOUSE|RIDEV_NOLEGACY), "Unexpected raw device flags: %#lx\n", raw_devices[0].dwFlags);
    flaky_wine_if( GetForegroundWindow() != hwnd ) /* FIXME: fvwm sometimes steals input focus */
    ok(raw_devices[2].dwFlags == (RIDEV_NOHOTKEYS|RIDEV_NOLEGACY), "Unexpected raw device flags: %#lx\n", raw_devices[1].dwFlags);
    hr = IDirectInputDevice8_Unacquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);
    hr = IDirectInputDevice8_Unacquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);

    raw_devices_count = ARRAY_SIZE(raw_devices);
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(hr == 1, "GetRegisteredRawInputDevices returned %ld, raw_devices_count: %d\n", hr, raw_devices_count);
    ok(raw_devices[0].usUsagePage == HID_USAGE_PAGE_GENERIC, "got usUsagePage: %x\n", raw_devices[0].usUsagePage);
    ok(raw_devices[0].usUsage == HID_USAGE_GENERIC_GAMEPAD, "got usUsage: %x\n", raw_devices[0].usUsage);
    ok(raw_devices[0].dwFlags == 0, "Unexpected raw device flags: %#lx\n", raw_devices[0].dwFlags);
    ok(raw_devices[0].hwndTarget == hwnd, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);

    raw_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
    raw_devices[0].dwFlags = RIDEV_REMOVE;
    raw_devices[0].hwndTarget = 0;
    raw_devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    raw_devices[1].dwFlags = RIDEV_REMOVE;
    raw_devices[1].hwndTarget = 0;
    raw_devices[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[2].usUsage = HID_USAGE_GENERIC_MOUSE;
    raw_devices[2].dwFlags = RIDEV_REMOVE;
    raw_devices[2].hwndTarget = 0;
    raw_devices_count = ARRAY_SIZE(raw_devices);
    hr = RegisterRawInputDevices(raw_devices, raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(hr == TRUE, "RegisterRawInputDevices failed\n");

    IDirectInputDevice8_Release(di_mouse);
    IDirectInputDevice8_Release(di_keyboard);
    IDirectInput8_Release(di);

    DestroyWindow(hwnd);
}

static void test_keyboard_events(void)
{
    HRESULT hr;
    HWND hwnd = INVALID_HANDLE_VALUE;
    IDirectInput8A *di;
    IDirectInputDevice8A *di_keyboard;
    DIPROPDWORD dp;
    DIDEVICEOBJECTDATA obj_data[10];
    DWORD data_size;
    BYTE kbdata[256];

    hr = CoCreateInstance(&CLSID_DirectInput8, 0, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (LPVOID*)&di);
    if (hr == DIERR_OLDDIRECTINPUTVERSION ||
        hr == DIERR_BETADIRECTINPUTVERSION ||
        hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("test_keyboard_events requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8Create failed: %#lx\n", hr);

    hr = IDirectInput8_Initialize(di, GetModuleHandleA(NULL), DIRECTINPUT_VERSION);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_BETADIRECTINPUTVERSION)
    {
        win_skip("test_keyboard_events requires dinput8\n");
        IDirectInput8_Release(di);
        return;
    }
    ok(SUCCEEDED(hr), "IDirectInput8_Initialize failed: %#lx\n", hr);

    hwnd = create_foreground_window( FALSE );

    hr = IDirectInput8_CreateDevice(di, &GUID_SysKeyboard, &di_keyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput8_CreateDevice failed: %#lx\n", hr);
    hr = IDirectInputDevice8_SetCooperativeLevel(di_keyboard, hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    ok(SUCCEEDED(hr), "IDirectInput8_SetCooperativeLevel failed: %#lx\n", hr);
    hr = IDirectInputDevice8_SetDataFormat(di_keyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetDataFormat failed: %#lx\n", hr);
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwObj = 0;
    dp.diph.dwHow = DIPH_DEVICE;
    dp.dwData = ARRAY_SIZE(obj_data);
    IDirectInputDevice8_SetProperty(di_keyboard, DIPROP_BUFFERSIZE, &(dp.diph));

    hr = IDirectInputDevice8_Acquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %#lx\n", hr);

    /* Test injecting keyboard events with both VK and scancode given. */
    keybd_event(VK_SPACE, DIK_SPACE, 0, 0);
    flush_events();
    IDirectInputDevice8_Poll(di_keyboard);
    data_size = ARRAY_SIZE(obj_data);
    hr = IDirectInputDevice8_GetDeviceData(di_keyboard, sizeof(DIDEVICEOBJECTDATA), obj_data, &data_size, 0);
    ok(SUCCEEDED(hr), "Failed to get data hr=%#lx\n", hr);
    ok(data_size == 1, "Expected 1 element, received %ld\n", data_size);

    hr = IDirectInputDevice8_GetDeviceState(di_keyboard, sizeof(kbdata), kbdata);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_GetDeviceState failed: %#lx\n", hr);
    ok(kbdata[DIK_SPACE], "Expected DIK_SPACE key state down\n");

    keybd_event(VK_SPACE, DIK_SPACE, KEYEVENTF_KEYUP, 0);
    flush_events();
    IDirectInputDevice8_Poll(di_keyboard);
    data_size = ARRAY_SIZE(obj_data);
    hr = IDirectInputDevice8_GetDeviceData(di_keyboard, sizeof(DIDEVICEOBJECTDATA), obj_data, &data_size, 0);
    ok(SUCCEEDED(hr), "Failed to get data hr=%#lx\n", hr);
    ok(data_size == 1, "Expected 1 element, received %ld\n", data_size);

    /* Test injecting keyboard events with scancode=0.
     * Windows DInput ignores the VK, sets scancode 0 to be pressed, and GetDeviceData returns no elements. */
    keybd_event(VK_SPACE, 0, 0, 0);
    flush_events();
    IDirectInputDevice8_Poll(di_keyboard);
    data_size = ARRAY_SIZE(obj_data);
    hr = IDirectInputDevice8_GetDeviceData(di_keyboard, sizeof(DIDEVICEOBJECTDATA), obj_data, &data_size, 0);
    ok(SUCCEEDED(hr), "Failed to get data hr=%#lx\n", hr);
    ok(data_size == 0, "Expected 0 elements, received %ld\n", data_size);

    hr = IDirectInputDevice8_GetDeviceState(di_keyboard, sizeof(kbdata), kbdata);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_GetDeviceState failed: %#lx\n", hr);
    todo_wine
    ok(kbdata[0], "Expected key 0 state down\n");

    keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
    flush_events();
    IDirectInputDevice8_Poll(di_keyboard);
    data_size = ARRAY_SIZE(obj_data);
    hr = IDirectInputDevice8_GetDeviceData(di_keyboard, sizeof(DIDEVICEOBJECTDATA), obj_data, &data_size, 0);
    ok(SUCCEEDED(hr), "Failed to get data hr=%#lx\n", hr);
    ok(data_size == 0, "Expected 0 elements, received %ld\n", data_size);

    hr = IDirectInputDevice8_Unacquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Unacquire failed: %#lx\n", hr);

    IDirectInputDevice8_Release(di_keyboard);
    IDirectInput8_Release(di);

    DestroyWindow(hwnd);
}

static void test_appdata_property(void)
{
    HRESULT hr;
    IDirectInputDevice8A *di_keyboard;
    IDirectInput8A *pDI = NULL;
    HWND hwnd;
    DIPROPDWORD dw;
    DIPROPPOINTER dp;

    hr = CoCreateInstance(&CLSID_DirectInput8, 0, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (LPVOID*)&pDI);
    if (hr == DIERR_OLDDIRECTINPUTVERSION ||
        hr == DIERR_BETADIRECTINPUTVERSION ||
        hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("DIPROP_APPDATA requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8 Create failed: hr=%#lx\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInput8_Initialize(pDI, instance, DIRECTINPUT_VERSION);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_BETADIRECTINPUTVERSION)
    {
        win_skip("DIPROP_APPDATA requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8 Initialize failed: hr=%#lx\n", hr);
    if (FAILED(hr)) return;

    hwnd = create_foreground_window( TRUE );

    hr = IDirectInput8_CreateDevice(pDI, &GUID_SysKeyboard, &di_keyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput8_CreateDevice failed: %#lx\n", hr);

    hr = IDirectInputDevice8_SetDataFormat(di_keyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetDataFormat failed: %#lx\n", hr);

    dw.diph.dwSize = sizeof(DIPROPDWORD);
    dw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dw.diph.dwObj = 0;
    dw.diph.dwHow = DIPH_DEVICE;
    dw.dwData = 32;
    hr = IDirectInputDevice8_SetProperty(di_keyboard, DIPROP_BUFFERSIZE, &(dw.diph));
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetProperty failed hr=%#lx\n", hr);

    /* the default value */
    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_A, -1);

    dp.diph.dwHow = DIPH_DEVICE;
    dp.diph.dwObj = 0;
    dp.uData = 1;
    hr = IDirectInputDevice8_SetProperty(di_keyboard, DIPROP_APPDATA, &(dp.diph));
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice8_SetProperty APPDATA for the device should be invalid hr=%#lx\n", hr);

    dp.diph.dwSize = sizeof(dp);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwHow = DIPH_BYUSAGE;
    dp.diph.dwObj = 2;
    dp.uData = 2;
    hr = IDirectInputDevice8_SetProperty(di_keyboard, DIPROP_APPDATA, &(dp.diph));
    ok(hr == DIERR_UNSUPPORTED, "IDirectInputDevice8_SetProperty APPDATA by usage should be unsupported hr=%#lx\n", hr);

    dp.diph.dwHow = DIPH_BYID;
    dp.diph.dwObj = DIDFT_MAKEINSTANCE(DIK_SPACE) | DIDFT_PSHBUTTON;
    dp.uData = 3;
    hr = IDirectInputDevice8_SetProperty(di_keyboard, DIPROP_APPDATA, &(dp.diph));
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetProperty failed hr=%#lx\n", hr);

    dp.diph.dwHow = DIPH_BYOFFSET;
    dp.diph.dwObj = DIK_A;
    dp.uData = 4;
    hr = IDirectInputDevice8_SetProperty(di_keyboard, DIPROP_APPDATA, &(dp.diph));
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetProperty failed hr=%#lx\n", hr);

    dp.diph.dwHow = DIPH_BYOFFSET;
    dp.diph.dwObj = DIK_B;
    dp.uData = 5;
    hr = IDirectInputDevice8_SetProperty(di_keyboard, DIPROP_APPDATA, &(dp.diph));
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetProperty failed hr=%#lx\n", hr);

    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_SPACE, 3);
    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_A, 4);
    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_B, 5);
    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_C, -1);

    /* setting data format resets APPDATA */
    hr = IDirectInputDevice8_SetDataFormat(di_keyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetDataFormat failed: %#lx\n", hr);

    test_device_input(di_keyboard, INPUT_KEYBOARD, VK_SPACE, -1);
    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_A, -1);
    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_B, -1);
    test_device_input(di_keyboard, INPUT_KEYBOARD, DIK_C, -1);

    DestroyWindow(hwnd);
    IDirectInputDevice_Release(di_keyboard);
    IDirectInput_Release(pDI);
}

struct check_objects_todos
{
    BOOL offset;
    BOOL type;
    BOOL name;
};

struct check_objects_params
{
    UINT index;
    UINT stop_count;
    UINT expect_count;
    const DIDEVICEOBJECTINSTANCEW *expect_objs;
    const struct check_objects_todos *todo_objs;
    BOOL todo_extra;
};

static BOOL CALLBACK check_objects( const DIDEVICEOBJECTINSTANCEW *obj, void *args )
{
    struct check_objects_params *params = args;
    const DIDEVICEOBJECTINSTANCEW *exp = params->expect_objs + params->index;
    const struct check_objects_todos *todo = params->todo_objs + params->index;

    todo_wine_if( params->todo_extra && params->index >= params->expect_count )
    ok( params->index < params->expect_count, "unexpected extra object\n" );
    if (params->index >= params->expect_count) return DIENUM_CONTINUE;

    winetest_push_context( "obj[%d]", params->index );

    check_member( *obj, *exp, "%lu", dwSize );
    check_member_guid( *obj, *exp, guidType );
    todo_wine_if( todo->offset )
    check_member( *obj, *exp, "%#lx", dwOfs );
    todo_wine_if( todo->type )
    check_member( *obj, *exp, "%#lx", dwType );
    check_member( *obj, *exp, "%#lx", dwFlags );
    if (!localized) todo_wine_if( todo->name ) check_member_wstr( *obj, *exp, tszName );
    check_member( *obj, *exp, "%lu", dwFFMaxForce );
    check_member( *obj, *exp, "%lu", dwFFForceResolution );
    check_member( *obj, *exp, "%u", wCollectionNumber );
    check_member( *obj, *exp, "%u", wDesignatorIndex );
    check_member( *obj, *exp, "%#04x", wUsagePage );
    check_member( *obj, *exp, "%#04x", wUsage );
    check_member( *obj, *exp, "%#lx", dwDimension );
    check_member( *obj, *exp, "%#04x", wExponent );
    check_member( *obj, *exp, "%u", wReportId );

    winetest_pop_context();

    params->index++;
    if (params->stop_count && params->index == params->stop_count) return DIENUM_STOP;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK check_object_count( const DIDEVICEOBJECTINSTANCEW *obj, void *args )
{
    DWORD *count = args;
    *count = *count + 1;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK check_object_count_bad_retval( const DIDEVICEOBJECTINSTANCEW *obj, void *args )
{
    DWORD *count = args;
    *count = *count + 1;
    return -1; /* Invalid, but should CONTINUE. Only explicit DIENUM_STOP will stop enumeration. */
}

static void test_sys_mouse( DWORD version )
{
    const DIDEVCAPS expect_caps =
    {
        .dwSize = sizeof(DIDEVCAPS),
        .dwFlags = DIDC_ATTACHED | DIDC_EMULATED,
        .dwDevType = version < 0x800 ? (DIDEVTYPEMOUSE_UNKNOWN << 8) | DIDEVTYPE_MOUSE
                                     : (DI8DEVTYPEMOUSE_UNKNOWN << 8) | DI8DEVTYPE_MOUSE,
        .dwAxes = 3,
        .dwButtons = 5,
    };
    const DIDEVICEINSTANCEW expect_devinst =
    {
        .dwSize = sizeof(DIDEVICEINSTANCEW),
        .guidInstance = GUID_SysMouse,
        .guidProduct = GUID_SysMouse,
        .dwDevType = version < 0x800 ? (DIDEVTYPEMOUSE_UNKNOWN << 8) | DIDEVTYPE_MOUSE
                                     : (DI8DEVTYPEMOUSE_UNKNOWN << 8) | DI8DEVTYPE_MOUSE,
        .tszInstanceName = L"Mouse",
        .tszProductName = L"Mouse",
        .guidFFDriver = GUID_NULL,
    };
    const DIDEVICEOBJECTINSTANCEW expect_objects[] =
    {
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_XAxis,
            .dwOfs = 0x0,
            .dwType = DIDFT_RELAXIS|DIDFT_MAKEINSTANCE(0),
            .dwFlags = DIDOI_ASPECTPOSITION,
            .tszName = L"X-axis",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_YAxis,
            .dwOfs = 0x4,
            .dwType = DIDFT_RELAXIS|DIDFT_MAKEINSTANCE(1),
            .dwFlags = DIDOI_ASPECTPOSITION,
            .tszName = L"Y-axis",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_ZAxis,
            .dwOfs = 0x8,
            .dwType = DIDFT_RELAXIS|DIDFT_MAKEINSTANCE(2),
            .dwFlags = DIDOI_ASPECTPOSITION,
            .tszName = L"Wheel",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0xc,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(3),
            .tszName = L"Button 0",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0xd,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(4),
            .tszName = L"Button 1",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0xe,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(5),
            .tszName = L"Button 2",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0xf,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(6),
            .tszName = L"Button 3",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0x10,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(7),
            .tszName = L"Button 4",
        },
    };
    struct check_objects_todos todo_objects[ARRAY_SIZE(expect_objects)] = {{0}};
    struct check_objects_params check_objects_params =
    {
        .expect_count = ARRAY_SIZE(expect_objects),
        .expect_objs = expect_objects,
        .todo_objs = todo_objects,
        .todo_extra = TRUE,
    };
    DIPROPGUIDANDPATH prop_guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPSTRING prop_string =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPSTRING),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPRANGE prop_range =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPRANGE),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIDEVICEOBJECTINSTANCEW objinst = {0};
    DIDEVICEOBJECTDATA objdata = {0};
    DIDEVICEINSTANCEW devinst = {0};
    BOOL old_localized = localized;
    IDirectInputDevice8W *device;
    HWND hwnd, tmp_hwnd, child;
    DIDEVCAPS caps = {0};
    DIMOUSESTATE state;
    ULONG res, ref;
    HANDLE event;
    DWORD count;
    HRESULT hr;
    GUID guid;
    int i;

    if (FAILED(create_dinput_device( version, &GUID_SysMouse, &device ))) return;

    localized = LOWORD( GetKeyboardLayout( 0 ) ) != 0x0409;
    winetest_push_context( "%#lx", version );

    hr = IDirectInputDevice8_Initialize( device, instance, version, &GUID_SysMouseEm );
    ok( hr == DI_OK, "Initialize returned %#lx\n", hr );
    guid = GUID_SysMouseEm;
    memset( &devinst, 0, sizeof(devinst) );
    devinst.dwSize = sizeof(DIDEVICEINSTANCEW);
    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    ok( IsEqualGUID( &guid, &GUID_SysMouseEm ), "got %s expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_SysMouseEm ) );

    hr = IDirectInputDevice8_Initialize( device, instance, version, &GUID_SysMouse );
    ok( hr == DI_OK, "Initialize returned %#lx\n", hr );

    memset( &devinst, 0, sizeof(devinst) );
    devinst.dwSize = sizeof(DIDEVICEINSTANCEW);
    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    check_member( devinst, expect_devinst, "%lu", dwSize );
    check_member_guid( devinst, expect_devinst, guidInstance );
    check_member_guid( devinst, expect_devinst, guidProduct );
    todo_wine
    check_member( devinst, expect_devinst, "%#lx", dwDevType );
    if (!localized) check_member_wstr( devinst, expect_devinst, tszInstanceName );
    if (!localized) todo_wine check_member_wstr( devinst, expect_devinst, tszProductName );
    check_member_guid( devinst, expect_devinst, guidFFDriver );
    check_member( devinst, expect_devinst, "%04x", wUsagePage );
    check_member( devinst, expect_devinst, "%04x", wUsage );

    devinst.dwSize = sizeof(DIDEVICEINSTANCE_DX3W);
    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    check_member_guid( devinst, expect_devinst, guidInstance );
    check_member_guid( devinst, expect_devinst, guidProduct );
    todo_wine
    check_member( devinst, expect_devinst, "%#lx", dwDevType );
    if (!localized) check_member_wstr( devinst, expect_devinst, tszInstanceName );
    if (!localized) todo_wine check_member_wstr( devinst, expect_devinst, tszProductName );

    caps.dwSize = sizeof(DIDEVCAPS);
    hr = IDirectInputDevice8_GetCapabilities( device, &caps );
    ok( hr == DI_OK, "GetCapabilities returned %#lx\n", hr );
    check_member( caps, expect_caps, "%lu", dwSize );
    check_member( caps, expect_caps, "%#lx", dwFlags );
    todo_wine
    check_member( caps, expect_caps, "%#lx", dwDevType );
    check_member( caps, expect_caps, "%lu", dwAxes );
    ok( caps.dwButtons == 3 || caps.dwButtons == 5, "got dwButtons %ld, expected 3 or 5\n", caps.dwButtons );
    check_member( caps, expect_caps, "%lu", dwPOVs );
    check_member( caps, expect_caps, "%lu", dwFFSamplePeriod );
    check_member( caps, expect_caps, "%lu", dwFFMinTimeResolution );
    todo_wine
    check_member( caps, expect_caps, "%lu", dwFirmwareRevision );
    todo_wine
    check_member( caps, expect_caps, "%lu", dwHardwareRevision );
    check_member( caps, expect_caps, "%lu", dwFFDriverVersion );

    prop_dword.diph.dwHow = DIPH_BYOFFSET;
    prop_dword.diph.dwObj = DIMOFS_X;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_DEADZONE, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_DEADZONE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_SATURATION, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_SATURATION returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_CALIBRATIONMODE, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_CALIBRATIONMODE returned %#lx\n", hr );
    prop_range.diph.dwHow = DIPH_BYOFFSET;
    prop_range.diph.dwObj = DIMOFS_X;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_RANGE, &prop_range.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_RANGE returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_AXISMODE, &prop_dword.diph );
    todo_wine
    ok( hr == DI_OK, "GetProperty DIPROP_AXISMODE returned %#lx\n", hr );
    todo_wine
    ok( prop_dword.dwData == DIPROPAXISMODE_ABS, "got %lu expected %u\n", prop_dword.dwData, DIPROPAXISMODE_ABS );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_BUFFERSIZE, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_BUFFERSIZE returned %#lx\n", hr );
    ok( prop_dword.dwData == 0, "got %#lx expected %#x\n", prop_dword.dwData, 0 );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    ok( prop_dword.dwData == 10000, "got %lu expected %u\n", prop_dword.dwData, 10000 );

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIMouse2 );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_AXISMODE, &prop_dword.diph );
    todo_wine
    ok( hr == DI_OK, "GetProperty DIPROP_AXISMODE returned %#lx\n", hr );
    todo_wine
    ok( prop_dword.dwData == DIPROPAXISMODE_REL, "got %lu expected %u\n", prop_dword.dwData, DIPROPAXISMODE_ABS );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_VIDPID, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_VIDPID returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &prop_guid_path.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_GUIDANDPATH returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_INSTANCENAME, &prop_string.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_INSTANCENAME returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_PRODUCTNAME, &prop_string.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_PRODUCTNAME returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_TYPENAME, &prop_string.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_TYPENAME returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_USERNAME, &prop_string.diph );
    if (version < 0x0800)
        ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_USERNAME returned %#lx\n", hr );
    else
    {
        ok( hr == DI_NOEFFECT, "GetProperty DIPROP_USERNAME returned %#lx\n", hr );
        ok( !wcscmp( prop_string.wsz, L"" ), "got user %s\n", debugstr_w(prop_string.wsz) );
    }

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_JOYSTICKID, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_VIDPID returned %#lx\n", hr );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_CALIBRATION, &prop_dword.diph );
    ok( hr == DIERR_INVALIDPARAM, "GetProperty DIPROP_CALIBRATION returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_AUTOCENTER returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_DEADZONE, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_DEADZONE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_SCANCODE, &prop_dword.diph );
    ok( hr == (version < 0x800 ? DIERR_UNSUPPORTED : DIERR_INVALIDPARAM),
        "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_BYUSAGE;
    prop_dword.diph.dwObj = MAKELONG( HID_USAGE_GENERIC_X, HID_USAGE_PAGE_GENERIC );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_BYID;
    prop_dword.diph.dwObj = DIDFT_MAKEINSTANCE(1) | DIDFT_RELAXIS;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    ok( prop_dword.dwData == 1, "got %ld expected 1\n", prop_dword.dwData );
    prop_dword.diph.dwHow = DIPH_BYOFFSET;
    prop_dword.diph.dwObj = DIMOFS_X;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    ok( prop_dword.dwData == 1, "got %ld expected 1\n", prop_dword.dwData );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_DEADZONE, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_DEADZONE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_SATURATION, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_SATURATION returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_CALIBRATIONMODE, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_CALIBRATIONMODE returned %#lx\n", hr );
    prop_dword.diph.dwObj = DIMOFS_Z;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    ok( prop_dword.dwData == WHEEL_DELTA, "got %ld expected %ld\n", prop_dword.dwData, (DWORD)WHEEL_DELTA );
    prop_range.lMin = 0xdeadbeef;
    prop_range.lMax = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_RANGE, &prop_range.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_RANGE returned %#lx\n", hr );
    ok( prop_range.lMin == DIPROPRANGE_NOMIN, "got %ld expected %ld\n", prop_range.lMin, DIPROPRANGE_NOMIN );
    ok( prop_range.lMax == DIPROPRANGE_NOMAX, "got %ld expected %ld\n", prop_range.lMax, DIPROPRANGE_NOMAX );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_LOGICALRANGE, &prop_range.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_LOGICALRANGE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_PHYSICALRANGE, &prop_range.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_PHYSICALRANGE returned %#lx\n", hr );

    prop_string.diph.dwHow = DIPH_BYOFFSET;
    prop_string.diph.dwObj = DIMOFS_X;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_KEYNAME, &prop_string.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_KEYNAME returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DIERR_INVALIDPARAM, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    prop_dword.dwData = 1000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );

    res = 0;
    hr = IDirectInputDevice8_EnumObjects( device, check_object_count, &res, DIDFT_AXIS | DIDFT_PSHBUTTON );
    ok( hr == DI_OK, "EnumObjects returned %#lx\n", hr );
    ok( res == 3 + caps.dwButtons, "got %lu expected %lu\n", res, 3 + caps.dwButtons );
    check_objects_params.expect_count = 3 + caps.dwButtons;
    hr = IDirectInputDevice8_EnumObjects( device, check_objects, &check_objects_params, DIDFT_ALL );
    ok( hr == DI_OK, "EnumObjects returned %#lx\n", hr );
    ok( check_objects_params.index >= check_objects_params.expect_count, "missing %u objects\n",
        check_objects_params.expect_count - check_objects_params.index );

    res = 0;
    hr = IDirectInputDevice8_EnumObjects( device, check_object_count_bad_retval, &res, DIDFT_AXIS );
    ok( hr == DI_OK, "EnumObjects returned %#lx\n", hr );
    ok( res == 3, "got %lu expected 3\n", res );

    objinst.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW);
    res = MAKELONG( HID_USAGE_GENERIC_X, HID_USAGE_PAGE_GENERIC );
    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, res, DIPH_BYUSAGE );
    ok( hr == DIERR_UNSUPPORTED, "GetObjectInfo returned: %#lx\n", hr );

    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, DIMOFS_Y, DIPH_BYOFFSET );
    ok( hr == DI_OK, "GetObjectInfo returned: %#lx\n", hr );

    check_member( objinst, expect_objects[1], "%lu", dwSize );
    check_member_guid( objinst, expect_objects[1], guidType );
    check_member( objinst, expect_objects[1], "%#lx", dwOfs );
    check_member( objinst, expect_objects[1], "%#lx", dwType );
    check_member( objinst, expect_objects[1], "%#lx", dwFlags );
    if (!localized) check_member_wstr( objinst, expect_objects[1], tszName );
    check_member( objinst, expect_objects[1], "%lu", dwFFMaxForce );
    check_member( objinst, expect_objects[1], "%lu", dwFFForceResolution );
    check_member( objinst, expect_objects[1], "%u", wCollectionNumber );
    check_member( objinst, expect_objects[1], "%u", wDesignatorIndex );
    check_member( objinst, expect_objects[1], "%#04x", wUsagePage );
    check_member( objinst, expect_objects[1], "%#04x", wUsage );
    check_member( objinst, expect_objects[1], "%#lx", dwDimension );
    check_member( objinst, expect_objects[1], "%#04x", wExponent );
    check_member( objinst, expect_objects[1], "%u", wReportId );

    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, 7, DIPH_BYOFFSET );
    ok( hr == DIERR_NOTFOUND, "GetObjectInfo returned: %#lx\n", hr );
    res = DIDFT_TGLBUTTON | DIDFT_MAKEINSTANCE( 3 );
    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, res, DIPH_BYID );
    ok( hr == DIERR_NOTFOUND, "GetObjectInfo returned: %#lx\n", hr );
    res = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 3 );
    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, res, DIPH_BYID );
    ok( hr == DI_OK, "GetObjectInfo returned: %#lx\n", hr );

    check_member( objinst, expect_objects[3], "%lu", dwSize );
    check_member_guid( objinst, expect_objects[3], guidType );
    check_member( objinst, expect_objects[3], "%#lx", dwOfs );
    check_member( objinst, expect_objects[3], "%#lx", dwType );
    check_member( objinst, expect_objects[3], "%#lx", dwFlags );
    if (!localized) check_member_wstr( objinst, expect_objects[3], tszName );
    check_member( objinst, expect_objects[3], "%lu", dwFFMaxForce );
    check_member( objinst, expect_objects[3], "%lu", dwFFForceResolution );
    check_member( objinst, expect_objects[3], "%u", wCollectionNumber );
    check_member( objinst, expect_objects[3], "%u", wDesignatorIndex );
    check_member( objinst, expect_objects[3], "%#04x", wUsagePage );
    check_member( objinst, expect_objects[3], "%#04x", wUsage );
    check_member( objinst, expect_objects[3], "%#lx", dwDimension );
    check_member( objinst, expect_objects[3], "%#04x", wExponent );
    check_member( objinst, expect_objects[3], "%u", wReportId );


    hwnd = create_foreground_window( TRUE );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_FOREGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_FOREGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_BACKGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_BACKGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_FOREGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_FOREGROUND|DISCL_EXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND|DISCL_EXCLUSIVE );
    ok( hr == DIERR_UNSUPPORTED, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );

    child = CreateWindowW( L"static", L"static", WS_CHILD | WS_VISIBLE,
                           10, 10, 50, 50, hwnd, NULL, NULL, NULL );
    ok( !!child, "CreateWindowW failed, error %lu\n", GetLastError() );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_FOREGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_FOREGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_BACKGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_BACKGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );

    DestroyWindow( child );


    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!event, "CreateEventW failed, error %lu\n", GetLastError() );
    hr = IDirectInputDevice8_SetEventNotification( device, event );
    ok( hr == DI_OK, "SetEventNotification returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );

    prop_dword.dwData = 5;
    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&prop_dword );
    ok( hr == DI_OK, "SetProperty returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIMouse );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_NOEFFECT, "Unacquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx, skipping test_sys_mouse\n", hr );
    if (hr != DI_OK) goto cleanup;
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_NOEFFECT, "Acquire returned %#lx\n", hr );

    mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
    res = WaitForSingleObject( event, 100 );
    if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
    {
        mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
        res = WaitForSingleObject( event, 5000 );
    }
    ok( !res, "WaitForSingleObject returned %#lx\n", res );

    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(objdata), &objdata, &count, 0 );
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( count == 1, "got count %lu\n", count );

    mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
    res = WaitForSingleObject( event, 5000 );
    ok( !res, "WaitForSingleObject returned %#lx\n", res );

    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );
    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(objdata), &objdata, &count, 0 );
    ok( hr == (version < 0x800 ? DI_OK : DIERR_NOTACQUIRED), "GetDeviceData returned %#lx\n", hr );
    ok( count == 1, "got count %lu\n", count );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx, skipping test_sys_mouse\n", hr );
    if (hr != DI_OK) goto cleanup;

    mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
    res = WaitForSingleObject( event, 100 );
    if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
    {
        mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
        res = WaitForSingleObject( event, 5000 );
    }
    ok( !res, "WaitForSingleObject returned %#lx\n", res );

    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx, skipping test_sys_mouse\n", hr );
    if (hr != DI_OK) goto cleanup;

    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(objdata), &objdata, &count, 0 );
    ok( hr == (version < 0x800 ? DI_OK : DI_BUFFEROVERFLOW), "GetDeviceData returned %#lx\n", hr );
    ok( count == 1, "got count %lu\n", count );

    mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
    res = WaitForSingleObject( event, 100 );
    if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
    {
        mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
        res = WaitForSingleObject( event, 5000 );
    }
    ok( !res, "WaitForSingleObject returned %#lx\n", res );

    for (i = 0; i < 2; i++)
    {
        mouse_event( MOUSEEVENTF_MOVE, 10 + i, 10 + i, 0, 0 );
        res = WaitForSingleObject( event, 5000 );
        ok( !res, "WaitForSingleObject returned %#lx\n", res );
    }

    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(objdata), &objdata, &count, 0 );
    ok( hr == (version < 0x800 ? DI_OK : DI_BUFFEROVERFLOW), "GetDeviceData returned %#lx\n", hr );
    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, sizeof(objdata), &objdata, &count, 0 );

    flaky_wine_if (hr == DIERR_NOTACQUIRED)
    ok( hr == DI_OK, "GetDeviceData returned %#lx\n", hr );
    ok( count == 1, "got count %lu\n", count );

    if (hr != DIERR_NOTACQUIRED)
    {
        hr = IDirectInputDevice8_Unacquire( device );
        ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );
    }

    tmp_hwnd = create_foreground_window( FALSE );

    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState  returned %#lx\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DIERR_OTHERAPPHASPRIO, "Acquire returned %#lx\n", hr );

    DestroyWindow( tmp_hwnd );


cleanup:
    CloseHandle( event );
    DestroyWindow( hwnd );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

    winetest_pop_context();
    localized = old_localized;
}

static UINT mouse_move_count;

static LRESULT CALLBACK mouse_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_MOUSEMOVE)
    {
        ok( wparam == 0, "got wparam %#Ix\n", wparam );
        ok( LOWORD(lparam) - 107 <= 2, "got LOWORD(lparam) %u\n", LOWORD(lparam) );
        ok( HIWORD(lparam) - 107 <= 2, "got HIWORD(lparam) %u\n", HIWORD(lparam) );
        mouse_move_count++;
    }
    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

static void test_hid_mouse(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_MOUSE),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_POINTER),
            COLLECTION(1, Physical),
                REPORT_ID(1, 1),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                LOGICAL_MINIMUM(2, -10),
                LOGICAL_MAXIMUM(2, +10),
                INPUT(1, Data|Var|Rel),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    const HID_DEVICE_ATTRIBUTES attributes =
    {
        .Size = sizeof(HID_DEVICE_ATTRIBUTES),
        .VendorID = LOWORD(EXPECT_VIDPID),
        .ProductID = 0x0002,
        .VersionNumber = 0x0100,
    };
    struct hid_device_desc desc =
    {
        .caps = { .InputReportByteLength = 6 },
        .attributes = attributes,
        .use_report_id = 1,
    };
    struct hid_expect single_move[] =
    {
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_buf = {1,0x00,0x08,0x00,0x08,0x00},
        },
    };

    WCHAR device_path[MAX_PATH];
    HANDLE file;
    POINT pos;
    DWORD res;
    HWND hwnd;
    BOOL ret;

    winetest_push_context( "mouse ");

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start_( &desc, 1, 5000 /* needs a long timeout on Win7 */ )) goto done;

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );


    /* windows doesn't let us open HID mouse devices */

    file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    todo_wine
    ok( file == INVALID_HANDLE_VALUE, "CreateFileW succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError() );
    CloseHandle( file );

    file = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                        NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );


    /* check basic mouse input injection to window message */

    SetCursorPos( 100, 100 );
    hwnd = create_foreground_window( TRUE );
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)mouse_wndproc );

    GetCursorPos( &pos );
    ok( pos.x == 100, "got x %lu\n", pos.x );
    ok( pos.y == 100, "got y %lu\n", pos.y );

    mouse_move_count = 0;
    bus_send_hid_input( file, &desc, single_move, sizeof(single_move) );
    bus_wait_hid_input( file, &desc, 100 );

    res = MsgWaitForMultipleObjects( 0, NULL, FALSE, 500, QS_MOUSEMOVE );
    todo_wine
    ok( !res, "MsgWaitForMultipleObjects returned %#lx\n", res );
    msg_wait_for_events( 0, NULL, 5 ); /* process pending messages */
    todo_wine
    ok( mouse_move_count >= 1, "got mouse_move_count %u\n", mouse_move_count );

    GetCursorPos( &pos );
    todo_wine
    ok( (ULONG)pos.x - 107 <= 2, "got x %lu\n", pos.x );
    todo_wine
    ok( (ULONG)pos.y - 107 <= 2, "got y %lu\n", pos.y );

    DestroyWindow( hwnd );


    CloseHandle( file );

done:
    hid_device_stop( &desc, 1 );

    winetest_pop_context();
}

static UINT pointer_enter_count;
static UINT pointer_up_count;
static HANDLE touchdown_event, touchleave_event;
static WPARAM pointer_wparam[16];
static WPARAM pointer_lparam[16];
static UINT pointer_count;

static LRESULT CALLBACK touch_screen_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_POINTERENTER)
    {
        pointer_wparam[pointer_count] = wparam;
        pointer_lparam[pointer_count] = lparam;
        pointer_count++;
        pointer_enter_count++;
    }
    if (msg == WM_POINTERDOWN) ReleaseSemaphore( touchdown_event, 1, NULL );

    if (msg == WM_POINTERUP)
    {
        pointer_wparam[pointer_count] = wparam;
        pointer_lparam[pointer_count] = lparam;
        pointer_count++;
        pointer_up_count++;
    }
    if (msg == WM_POINTERLEAVE) ReleaseSemaphore( touchleave_event, 1, NULL );

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void test_hid_touch_screen(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_DIGITIZER),
        USAGE(1, HID_USAGE_DIGITIZER_TOUCH_SCREEN),
        COLLECTION(1, Application),
            REPORT_ID(1, 1),

            USAGE(1, HID_USAGE_DIGITIZER_CONTACT_COUNT),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_COUNT(1, 1),
            REPORT_SIZE(1, 8),
            INPUT(1, Data|Var|Abs),

            USAGE_PAGE(1, HID_USAGE_PAGE_DIGITIZER),
            USAGE(1, HID_USAGE_DIGITIZER_FINGER),
            COLLECTION(1, Logical),
                USAGE(1, HID_USAGE_DIGITIZER_TIP_SWITCH),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),

                USAGE(1, HID_USAGE_DIGITIZER_CONTACT_ID),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_X),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE(1, HID_USAGE_GENERIC_Y),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_DIGITIZER),
            USAGE(1, HID_USAGE_DIGITIZER_FINGER),
            COLLECTION(1, Logical),
                USAGE(1, HID_USAGE_DIGITIZER_TIP_SWITCH),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),

                USAGE(1, HID_USAGE_DIGITIZER_CONTACT_ID),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_X),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE(1, HID_USAGE_GENERIC_Y),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_DIGITIZER),
            USAGE(1, HID_USAGE_DIGITIZER_CONTACT_COUNT_MAX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 2),
            REPORT_COUNT(1, 1),
            REPORT_SIZE(1, 8),
            FEATURE(1, Data|Var|Abs),
        END_COLLECTION
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    const HID_DEVICE_ATTRIBUTES attributes =
    {
        .Size = sizeof(HID_DEVICE_ATTRIBUTES),
        .VendorID = LOWORD(EXPECT_VIDPID),
        .ProductID = 0x0004,
        .VersionNumber = 0x0100,
    };
    struct hid_device_desc desc =
    {
        .caps = { .InputReportByteLength = 10, .FeatureReportByteLength = 2 },
        .attributes = attributes,
        .use_report_id = 1,
    };
    struct hid_expect touch_single =
    {
        .code = IOCTL_HID_READ_REPORT,
        .report_buf = {1, 1, 0x01,0x02,0x08,0x10},
    };
    struct hid_expect touch_multiple =
    {
        .code = IOCTL_HID_READ_REPORT,
        .report_buf = {1, 2, 0x01,0x02,0x08,0x10, 0x01,0x03,0x18,0x20},
    };
    struct hid_expect touch_release =
    {
        .code = IOCTL_HID_READ_REPORT,
        .report_buf = {1, 0},
    };
    struct hid_expect expect_max_count =
    {
        .code = IOCTL_HID_GET_FEATURE,
        .report_id = 1,
        .report_len = 2,
        .report_buf = {1,0x02},
    };

    RAWINPUTDEVICE rawdevice = {.usUsagePage = HID_USAGE_PAGE_DIGITIZER, .usUsage = HID_USAGE_DIGITIZER_TOUCH_SCREEN};
    UINT rawbuffer_count, rawbuffer_size, expect_flags, id, width, height;
    WCHAR device_path[MAX_PATH];
    char rawbuffer[1024];
    RAWINPUT *rawinput;
    HANDLE file;
    DWORD res;
    HWND hwnd;
    BOOL ret;

    width = GetSystemMetrics( SM_CXVIRTUALSCREEN );
    height = GetSystemMetrics( SM_CYVIRTUALSCREEN );

    touchdown_event = CreateSemaphoreW( NULL, 0, LONG_MAX, NULL );
    ok( !!touchdown_event, "CreateSemaphoreW failed, error %lu\n", GetLastError() );
    touchleave_event = CreateSemaphoreW( NULL, 0, LONG_MAX, NULL );
    ok( !!touchleave_event, "CreateSemaphoreW failed, error %lu\n", GetLastError() );

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    desc.expect_size = sizeof(expect_max_count);
    memcpy( desc.expect, &expect_max_count, sizeof(expect_max_count) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );


    /* windows doesn't let us open HID touch_screen devices */

    file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    /* except on Win7 which doesn't seem to support HID touch screen */
    if (broken(sizeof(void *) == 4 && file != INVALID_HANDLE_VALUE))
    {
        win_skip( "HID touchscreen unsupported, skipping tests\n" );
        CloseHandle( file );
        goto done;
    }

    todo_wine
    ok( file == INVALID_HANDLE_VALUE, "CreateFileW succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_SHARING_VIOLATION, "got error %lu\n", GetLastError() );
    CloseHandle( file );


    file = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                        NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );


    /* check basic touch_screen input injection to window message */

    SetCursorPos( 0, 0 );

    hwnd = create_foreground_window( TRUE );
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)touch_screen_wndproc );


    /* a single touch is automatically released if we don't send continuous updates */

    pointer_enter_count = pointer_up_count = pointer_count = 0;
    memset( pointer_wparam, 0, sizeof(pointer_wparam) );
    memset( pointer_lparam, 0, sizeof(pointer_lparam) );
    bus_send_hid_input( file, &desc, &touch_single, sizeof(touch_single) );

    res = MsgWaitForMultipleObjects( 0, NULL, FALSE, 500, QS_POINTER );
    ok( !res, "MsgWaitForMultipleObjects returned %#lx\n", res );

    res = msg_wait_for_events( 1, &touchdown_event, 10 );
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    res = msg_wait_for_events( 1, &touchleave_event, 500 );
    todo_wine
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    ok( pointer_enter_count == 1, "got pointer_enter_count %u\n", pointer_enter_count );
    todo_wine
    ok( pointer_up_count == 1, "got pointer_up_count %u\n", pointer_up_count );

    expect_flags = POINTER_MESSAGE_FLAG_PRIMARY | POINTER_MESSAGE_FLAG_CONFIDENCE |
                   POINTER_MESSAGE_FLAG_FIRSTBUTTON | POINTER_MESSAGE_FLAG_NEW |
                   POINTER_MESSAGE_FLAG_INRANGE | POINTER_MESSAGE_FLAG_INCONTACT;
    todo_wine /* missing POINTER_MESSAGE_FLAG_FIRSTBUTTON */
    ok( HIWORD( pointer_wparam[0] ) == expect_flags, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_wparam[0] ) > 0, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_lparam[0] ) * 128 / width == 0x08, "got lparam %#Ix\n", pointer_lparam[0] );
    ok( HIWORD( pointer_lparam[0] ) * 128 / height == 0x10, "got lparam %#Ix\n", pointer_lparam[0] );
    id = LOWORD( pointer_wparam[0] );

    expect_flags = POINTER_MESSAGE_FLAG_PRIMARY | POINTER_MESSAGE_FLAG_CONFIDENCE;
    todo_wine
    ok( HIWORD( pointer_wparam[1] ) == expect_flags ||
        broken(HIWORD( pointer_wparam[1] ) == (expect_flags & ~POINTER_MESSAGE_FLAG_CONFIDENCE)), /* Win8 32bit */
        "got wparam %#Ix\n", pointer_wparam[1] );
    todo_wine
    ok( LOWORD( pointer_wparam[1] ) == id, "got wparam %#Ix\n", pointer_wparam[1] );
    todo_wine
    ok( LOWORD( pointer_lparam[1] ) * 128 / width == 0x08, "got lparam %#Ix\n", pointer_lparam[1] );
    todo_wine
    ok( HIWORD( pointer_lparam[1] ) * 128 / height == 0x10, "got lparam %#Ix\n", pointer_lparam[1] );


    /* test that we receive HID rawinput type with the touchscreen */

    rawdevice.dwFlags = RIDEV_INPUTSINK;
    rawdevice.hwndTarget = hwnd;
    ret = RegisterRawInputDevices( &rawdevice, 1, sizeof(RAWINPUTDEVICE) );
    ok( ret, "RegisterRawInputDevices failed, error %lu\n", GetLastError() );

    res = GetQueueStatus( QS_RAWINPUT );
    ok( res == 0, "got res %#lx\n", res );

    bus_send_hid_input( file, &desc, &touch_multiple, sizeof(touch_multiple) );
    bus_wait_hid_input( file, &desc, 5000 );
    bus_send_hid_input( file, &desc, &touch_release, sizeof(touch_release) );
    bus_wait_hid_input( file, &desc, 5000 );

    res = MsgWaitForMultipleObjects( 0, NULL, FALSE, 500, QS_POINTER );
    ok( !res, "MsgWaitForMultipleObjects returned %#lx\n", res );

    res = GetQueueStatus( QS_RAWINPUT );
    ok( LOWORD(res) == QS_RAWINPUT, "got res %#lx\n", res );
    ok( HIWORD(res) == QS_RAWINPUT, "got res %#lx\n", res );

    memset( rawbuffer, 0, sizeof(rawbuffer) );
    rawinput = (RAWINPUT *)rawbuffer;
    rawbuffer_size = sizeof(rawbuffer);
    rawbuffer_count = GetRawInputBuffer( rawinput, &rawbuffer_size, sizeof(RAWINPUTHEADER) );
    ok( rawbuffer_count == 2, "got rawbuffer_count %u\n", rawbuffer_count );
    ok( rawbuffer_size == sizeof(rawbuffer), "got rawbuffer_size %u\n", rawbuffer_size );

    rawinput = (RAWINPUT *)rawbuffer;
    ok( rawinput->header.dwType == RIM_TYPEHID, "got dwType %lu\n", rawinput->header.dwType );
    ok( rawinput->header.dwSize == offsetof(RAWINPUT, data.hid.bRawData[desc.caps.InputReportByteLength * rawinput->data.hid.dwCount]),
        "got header.dwSize %lu\n", rawinput->header.dwSize );
    ok( rawinput->header.hDevice != 0, "got hDevice %p\n", rawinput->header.hDevice );
    ok( rawinput->header.wParam == 0, "got wParam %#Ix\n", rawinput->header.wParam );
    ok( rawinput->data.hid.dwSizeHid == desc.caps.InputReportByteLength, "got dwSizeHid %lu\n", rawinput->data.hid.dwSizeHid );
    ok( rawinput->data.hid.dwCount == 1, "got dwCount %lu\n", rawinput->data.hid.dwCount );
    ok( !memcmp( rawinput->data.hid.bRawData, touch_multiple.report_buf, desc.caps.InputReportByteLength ),
        "got unexpected report data\n" );

    res = GetQueueStatus( QS_RAWINPUT );
    ok( LOWORD(res) == 0, "got res %#lx\n", res );
    ok( HIWORD(res) == 0, "got res %#lx\n", res );

    rawdevice.dwFlags = RIDEV_REMOVE;
    rawdevice.hwndTarget = 0;
    ret = RegisterRawInputDevices( &rawdevice, 1, sizeof(RAWINPUTDEVICE) );
    ok( ret, "RegisterRawInputDevices failed, error %lu\n", GetLastError() );


    /* test that we don't receive mouse rawinput type with the touchscreen */

    memset( rawbuffer, 0, sizeof(rawbuffer) );
    rawdevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawdevice.usUsage = HID_USAGE_GENERIC_MOUSE;
    rawdevice.dwFlags = RIDEV_INPUTSINK;
    rawdevice.hwndTarget = hwnd;
    ret = RegisterRawInputDevices( &rawdevice, 1, sizeof(RAWINPUTDEVICE) );
    ok( ret, "RegisterRawInputDevices failed, error %lu\n", GetLastError() );

    bus_send_hid_input( file, &desc, &touch_multiple, sizeof(touch_multiple) );
    res = MsgWaitForMultipleObjects( 0, NULL, FALSE, 500, QS_POINTER );
    ok( !res, "MsgWaitForMultipleObjects returned %#lx\n", res );
    bus_send_hid_input( file, &desc, &touch_release, sizeof(touch_release) );
    res = MsgWaitForMultipleObjects( 0, NULL, FALSE, 500, QS_POINTER );
    ok( !res, "MsgWaitForMultipleObjects returned %#lx\n", res );

    rawinput = (RAWINPUT *)rawbuffer;
    rawbuffer_size = sizeof(rawbuffer);
    rawbuffer_count = GetRawInputBuffer( rawinput, &rawbuffer_size, sizeof(RAWINPUTHEADER) );
    ok( rawbuffer_count == 0, "got rawbuffer_count %u\n", rawbuffer_count );
    ok( rawbuffer_size == 0, "got rawbuffer_size %u\n", rawbuffer_size );

    rawdevice.dwFlags = RIDEV_REMOVE;
    rawdevice.hwndTarget = 0;
    ret = RegisterRawInputDevices( &rawdevice, 1, sizeof(RAWINPUTDEVICE) );
    ok( ret, "RegisterRawInputDevices failed, error %lu\n", GetLastError() );


    DestroyWindow( hwnd );

    CloseHandle( file );
    hid_device_stop( &desc, 1 );


    /* create a polled device instead so updates are sent continuously */

    desc.is_polled = TRUE;
    desc.input_size = sizeof(touch_release);
    memcpy( desc.input, &touch_release, sizeof(touch_release) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );

    file = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                        NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );

    hwnd = create_foreground_window( TRUE );
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)touch_screen_wndproc );


    /* now the touch is continuously updated */

    pointer_enter_count = pointer_up_count = pointer_count = 0;
    memset( pointer_wparam, 0, sizeof(pointer_wparam) );
    memset( pointer_lparam, 0, sizeof(pointer_lparam) );
    bus_send_hid_input( file, &desc, &touch_single, sizeof(touch_single) );

    res = msg_wait_for_events( 1, &touchdown_event, 1000 );
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    res = msg_wait_for_events( 1, &touchleave_event, 10 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx\n", res );
    ok( pointer_enter_count == 1, "got pointer_enter_count %u\n", pointer_enter_count );
    ok( pointer_up_count == 0, "got pointer_up_count %u\n", pointer_up_count );

    expect_flags = POINTER_MESSAGE_FLAG_PRIMARY | POINTER_MESSAGE_FLAG_CONFIDENCE |
                   POINTER_MESSAGE_FLAG_FIRSTBUTTON | POINTER_MESSAGE_FLAG_NEW |
                   POINTER_MESSAGE_FLAG_INRANGE | POINTER_MESSAGE_FLAG_INCONTACT;
    todo_wine /* missing POINTER_MESSAGE_FLAG_FIRSTBUTTON */
    ok( HIWORD( pointer_wparam[0] ) == expect_flags, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_wparam[0] ) > 0, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_lparam[0] ) * 128 / width == 0x08, "got lparam %#Ix\n", pointer_lparam[0] );
    ok( HIWORD( pointer_lparam[0] ) * 128 / height == 0x10, "got lparam %#Ix\n", pointer_lparam[0] );
    ok( pointer_wparam[1] == 0, "got wparam %#Ix\n", pointer_wparam[1] );
    ok( pointer_lparam[1] == 0, "got lparam %#Ix\n", pointer_lparam[1] );
    id = LOWORD( pointer_wparam[0] );


    pointer_enter_count = pointer_up_count = pointer_count = 0;
    memset( pointer_wparam, 0, sizeof(pointer_wparam) );
    memset( pointer_lparam, 0, sizeof(pointer_lparam) );
    bus_send_hid_input( file, &desc, &touch_release, sizeof(touch_release) );

    res = msg_wait_for_events( 1, &touchleave_event, 1000 );
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    res = msg_wait_for_events( 1, &touchdown_event, 10 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx\n", res );
    ok( pointer_enter_count == 0, "got pointer_enter_count %u\n", pointer_enter_count );
    ok( pointer_up_count == 1, "got pointer_up_count %u\n", pointer_up_count );

    expect_flags = POINTER_MESSAGE_FLAG_PRIMARY | POINTER_MESSAGE_FLAG_CONFIDENCE;
    ok( HIWORD( pointer_wparam[0] ) == expect_flags ||
        broken(HIWORD( pointer_wparam[0] ) == (expect_flags & ~POINTER_MESSAGE_FLAG_CONFIDENCE)), /* Win8 32bit */
        "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_wparam[0] ) == id, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_lparam[0] ) * 128 / width == 0x08, "got lparam %#Ix\n", pointer_lparam[0] );
    ok( HIWORD( pointer_lparam[0] ) * 128 / height == 0x10, "got lparam %#Ix\n", pointer_lparam[0] );
    ok( pointer_wparam[1] == 0, "got wparam %#Ix\n", pointer_wparam[1] );
    ok( pointer_lparam[1] == 0, "got lparam %#Ix\n", pointer_lparam[1] );


    pointer_enter_count = pointer_up_count = pointer_count = 0;
    memset( pointer_wparam, 0, sizeof(pointer_wparam) );
    memset( pointer_lparam, 0, sizeof(pointer_lparam) );
    bus_send_hid_input( file, &desc, &touch_multiple, sizeof(touch_multiple) );

    res = msg_wait_for_events( 1, &touchdown_event, 1000 );
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    res = msg_wait_for_events( 1, &touchdown_event, 1000 );
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    res = msg_wait_for_events( 1, &touchleave_event, 10 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx\n", res );
    ok( pointer_enter_count == 2, "got pointer_enter_count %u\n", pointer_enter_count );
    ok( pointer_up_count == 0, "got pointer_up_count %u\n", pointer_up_count );

    expect_flags = POINTER_MESSAGE_FLAG_PRIMARY | POINTER_MESSAGE_FLAG_CONFIDENCE |
                   POINTER_MESSAGE_FLAG_FIRSTBUTTON | POINTER_MESSAGE_FLAG_NEW |
                   POINTER_MESSAGE_FLAG_INRANGE | POINTER_MESSAGE_FLAG_INCONTACT;
    todo_wine /* missing POINTER_MESSAGE_FLAG_FIRSTBUTTON */
    ok( HIWORD( pointer_wparam[0] ) == expect_flags, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_wparam[0] ) > 0, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_lparam[0] ) * 128 / width == 0x08, "got lparam %#Ix\n", pointer_lparam[0] );
    ok( HIWORD( pointer_lparam[0] ) * 128 / height == 0x10, "got lparam %#Ix\n", pointer_lparam[0] );
    id = LOWORD( pointer_wparam[0] );

    expect_flags = POINTER_MESSAGE_FLAG_CONFIDENCE | POINTER_MESSAGE_FLAG_FIRSTBUTTON |
                   POINTER_MESSAGE_FLAG_NEW | POINTER_MESSAGE_FLAG_INRANGE | POINTER_MESSAGE_FLAG_INCONTACT;
    todo_wine /* missing POINTER_MESSAGE_FLAG_FIRSTBUTTON */
    ok( HIWORD( pointer_wparam[1] ) == expect_flags ||
        broken(HIWORD( pointer_wparam[1] ) == (expect_flags & ~POINTER_MESSAGE_FLAG_CONFIDENCE)), /* Win8 32bit */
        "got wparam %#Ix\n", pointer_wparam[1] );
    ok( LOWORD( pointer_wparam[1] ) == id + 1, "got wparam %#Ix\n", pointer_wparam[1] );
    ok( LOWORD( pointer_lparam[1] ) * 128 / width == 0x18, "got lparam %#Ix\n", pointer_lparam[1] );
    ok( HIWORD( pointer_lparam[1] ) * 128 / height == 0x20, "got lparam %#Ix\n", pointer_lparam[1] );


    pointer_enter_count = pointer_up_count = pointer_count = 0;
    memset( pointer_wparam, 0, sizeof(pointer_wparam) );
    memset( pointer_lparam, 0, sizeof(pointer_lparam) );
    bus_send_hid_input( file, &desc, &touch_release, sizeof(touch_release) );

    res = msg_wait_for_events( 1, &touchleave_event, 1000 );
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    res = msg_wait_for_events( 1, &touchleave_event, 1000 );
    ok( res == 0, "WaitForSingleObject returned %#lx\n", res );
    res = msg_wait_for_events( 1, &touchdown_event, 10 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx\n", res );
    ok( pointer_enter_count == 0, "got pointer_enter_count %u\n", pointer_enter_count );
    ok( pointer_up_count == 2, "got pointer_up_count %u\n", pointer_up_count );

    expect_flags = POINTER_MESSAGE_FLAG_PRIMARY | POINTER_MESSAGE_FLAG_CONFIDENCE;
    ok( HIWORD( pointer_wparam[0] ) == expect_flags ||
        broken(HIWORD( pointer_wparam[0] ) == (expect_flags & ~POINTER_MESSAGE_FLAG_CONFIDENCE)), /* Win8 32bit */
        "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_wparam[0] ) == id, "got wparam %#Ix\n", pointer_wparam[0] );
    ok( LOWORD( pointer_lparam[0] ) * 128 / width == 0x08, "got lparam %#Ix\n", pointer_lparam[0] );
    ok( HIWORD( pointer_lparam[0] ) * 128 / height == 0x10, "got lparam %#Ix\n", pointer_lparam[0] );

    expect_flags = POINTER_MESSAGE_FLAG_CONFIDENCE;
    ok( HIWORD( pointer_wparam[1] ) == expect_flags ||
        broken(HIWORD( pointer_wparam[1] ) == (expect_flags & ~POINTER_MESSAGE_FLAG_CONFIDENCE)), /* Win8 32bit */
        "got wparam %#Ix\n", pointer_wparam[1] );
    ok( LOWORD( pointer_wparam[1] ) == id + 1, "got wparam %#Ix\n", pointer_wparam[1] );
    ok( LOWORD( pointer_lparam[1] ) * 128 / width == 0x18, "got lparam %#Ix\n", pointer_lparam[1] );
    ok( HIWORD( pointer_lparam[1] ) * 128 / height == 0x20, "got lparam %#Ix\n", pointer_lparam[1] );


    DestroyWindow( hwnd );

    CloseHandle( file );

done:
    hid_device_stop( &desc, 1 );

    CloseHandle( touchdown_event );
    CloseHandle( touchleave_event );
}

static void test_dik_codes( IDirectInputDevice8W *device, HANDLE event, HWND hwnd, DWORD version )
{
    static const struct key2dik
    {
        BYTE key, dik, todo;
    }
    key2dik_en[] =
    {
        {'Q',DIK_Q}, {'W',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Y',DIK_Y},
        {'[',DIK_LBRACKET}, {']',DIK_RBRACKET}, {'.',DIK_PERIOD}
    },
    key2dik_fr[] =
    {
        {'A',DIK_Q}, {'Z',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Y',DIK_Y},
        {'^',DIK_LBRACKET}, {'$',DIK_RBRACKET}, {':',DIK_PERIOD}
    },
    key2dik_de[] =
    {
        {'Q',DIK_Q}, {'W',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Z',DIK_Y},
        {'\xfc',DIK_LBRACKET,1}, {'+',DIK_RBRACKET}, {'.',DIK_PERIOD}
    },
    key2dik_ja[] =
    {
        {'Q',DIK_Q}, {'W',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Y',DIK_Y},
        {'@',DIK_AT}, {']',DIK_RBRACKET}, {'.',DIK_PERIOD}
    };
    static const struct
    {
        LANGID langid;
        const struct key2dik *map;
        DWORD type;
    } tests[] =
    {
        { MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), key2dik_en, DIDEVTYPEKEYBOARD_PCENH },
        { MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH), key2dik_fr, DIDEVTYPEKEYBOARD_PCENH },
        { MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), key2dik_de, DIDEVTYPEKEYBOARD_PCENH },
        { MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN), key2dik_ja, DIDEVTYPEKEYBOARD_JAPAN106 }
    };
    DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
    const struct key2dik *map;
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_BYOFFSET,
        },
    };
    BYTE key_state[256];
    HKL hkl, old_hkl;
    HRESULT hr;
    ULONG res;
    UINT vkey, scan, i, j;

    hr = IDirectInputDevice_SetDataFormat( device, &c_dfDIKeyboard );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );
    hr = IDirectInputDevice_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice_GetCapabilities( device, &caps );
    ok( hr == DI_OK, "GetDeviceInstance returned %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (tests[i].type != GET_DIDEVICE_SUBTYPE( caps.dwDevType ))
        {
            skip( "keyboard type %#x doesn't match for lang %#x\n",
                  GET_DIDEVICE_SUBTYPE( caps.dwDevType ), tests[i].langid );
            continue;
        }

        winetest_push_context( "lang %#x", tests[i].langid );

        hkl = activate_keyboard_layout( tests[i].langid, &old_hkl );
        if (LOWORD(old_hkl) != MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT) ||
            LOWORD(hkl) != tests[i].langid) goto skip_key_tests;

        map = tests[i].map;
        for (j = 0; j < ARRAY_SIZE(key2dik_en); j++)
        {
            winetest_push_context( "key %#x, dik %#x", map[j].key, map[j].dik );

            vkey = VkKeyScanExW( map[j].key, hkl );
            todo_wine_if( map[j].todo )
            ok( vkey != 0xffff, "VkKeyScanExW failed\n" );

            vkey = LOBYTE(vkey);
            res = MapVirtualKeyExA( vkey, MAPVK_VK_TO_CHAR, hkl ) & 0xff;
            todo_wine_if( map[j].todo )
            ok( res == map[j].key, "MapVirtualKeyExA failed\n" );

            scan = MapVirtualKeyExA( vkey, MAPVK_VK_TO_VSC, hkl );
            todo_wine_if( map[j].todo )
            ok( scan, "MapVirtualKeyExA failed\n" );

            prop_dword.diph.dwObj = map[j].dik;
            hr = IDirectInputDevice8_GetProperty( device, DIPROP_SCANCODE, &prop_dword.diph );
            if (version < 0x0800)
                ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );
            else
            {
                ok( hr == DI_OK, "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );
                ok( prop_dword.dwData == scan, "got %#lx expected %#x\n", prop_dword.dwData, scan );
            }

            keybd_event( vkey, scan, 0, 0 );
            res = WaitForSingleObject( event, 100 );
            if (i == 0 && j == 0 && res == WAIT_TIMEOUT) /* Acquire is asynchronous */
            {
                keybd_event( vkey, scan, 0, 0 );
                res = WaitForSingleObject( event, 5000 );
            }
            ok( !res, "WaitForSingleObject returned %#lx\n", res );

            hr = IDirectInputDevice_GetDeviceState( device, sizeof(key_state), key_state );
            ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );

            todo_wine_if( map[j].todo )
            ok( key_state[map[j].dik] == 0x80, "got state %#x\n", key_state[map[j].dik] );

            keybd_event( vkey, scan, KEYEVENTF_KEYUP, 0 );
            res = WaitForSingleObject( event, 5000 );
            flaky_wine_if( GetForegroundWindow() != hwnd && version == 0x800 ) /* FIXME: fvwm sometimes steals input focus */
            ok( !res, "WaitForSingleObject returned %#lx\n", res );

            winetest_pop_context();
        }

    skip_key_tests:
        ActivateKeyboardLayout( old_hkl, 0 );

        winetest_pop_context();
    }

    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );
}

static void test_scan_codes( IDirectInputDevice8W *device, HANDLE event, HWND hwnd, DWORD version )
{
    static const struct dik2scan
    {
        BYTE dik; BOOL found; DWORD result;
    }
    dik2scan_en[] =
    {
        { DIK_NUMLOCK, TRUE, 0x451DE1 },
        { DIK_PAUSE, TRUE, 0x45 },
        { DIK_CIRCUMFLEX, TRUE, 0x10E0 },
        { DIK_KANA, FALSE, 0 },
    },
    dik2scan_ja[] =
    {
        { DIK_NUMLOCK, TRUE, 0x451DE1 },
        { DIK_PAUSE, TRUE, 0x45 },
        { DIK_CIRCUMFLEX, TRUE, 0x0d },
        { DIK_KANA, TRUE, 0x70 },
    };
    static const struct
    {
        LANGID langid;
        const struct dik2scan *map;
        DWORD type;
    } tests[] =
    {
        { MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), dik2scan_en, DIDEVTYPEKEYBOARD_PCENH },
        { MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN), dik2scan_ja, DIDEVTYPEKEYBOARD_JAPAN106 },
    };
    DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
    const struct dik2scan *map;
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_BYOFFSET,
        },
    };
    HKL hkl, old_hkl;
    HRESULT hr;
    UINT i, j;

    hr = IDirectInputDevice_SetDataFormat( device, &c_dfDIKeyboard );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );
    hr = IDirectInputDevice_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice_GetCapabilities( device, &caps );
    ok( hr == DI_OK, "GetDeviceInstance returned %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (tests[i].type != GET_DIDEVICE_SUBTYPE( caps.dwDevType ))
        {
            skip( "keyboard type %#x doesn't match for lang %#x\n",
                  GET_DIDEVICE_SUBTYPE( caps.dwDevType ), tests[i].langid );
            continue;
        }

        winetest_push_context( "lang %#x", tests[i].langid );

        hkl = activate_keyboard_layout( tests[i].langid, &old_hkl );
        if (LOWORD(old_hkl) != MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT) ||
            LOWORD(hkl) != tests[i].langid) goto skip_key_tests;

        map = tests[i].map;
        for (j = 0; j < ARRAY_SIZE(dik2scan_en); j++)
        {
            winetest_push_context( "dik %#x", map[j].dik );

            prop_dword.diph.dwObj = map[j].dik;
            hr = IDirectInputDevice8_GetProperty( device, DIPROP_SCANCODE, &prop_dword.diph );

            if (!map[j].found)
                todo_wine ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );
            else if (version < 0x0800)
                ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );
            else
            {
                ok( hr == DI_OK, "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );
                ok( prop_dword.dwData == map[j].result, "got %#lx expected %#lx\n",
                    prop_dword.dwData, map[j].result );
            }

            winetest_pop_context();
        }

    skip_key_tests:
        ActivateKeyboardLayout( old_hkl, 0 );

        winetest_pop_context();
    }

    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );
}

#define check_member_str_( file, line, val, exp, member )                                         \
    ok_(file, line)( !strcmp( (val).member, (exp).member ), "got " #member " %s\n",               \
                     debugstr_a((val).member) )
#define check_member_str( val, exp, member )                                                      \
    check_member_str_( __FILE__, __LINE__, val, exp, member )

#define check_diactionA( a, b ) check_diactionA_( __LINE__, a, b )
static void check_diactionA_( int line, const DIACTIONA *actual, const DIACTIONA *expected )
{
    check_member_( __FILE__, line, *actual, *expected, "%#Ix", uAppData );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwSemantic );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwFlags );
    if (actual->lptszActionName && expected->lptszActionName)
        check_member_str_( __FILE__, line, *actual, *expected, lptszActionName );
    else
        check_member_( __FILE__, line, *actual, *expected, "%p", lptszActionName );
    check_member_guid_( __FILE__, line, *actual, *expected, guidInstance );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwObjID );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwHow );
}

#define check_diactionformatA( a, b ) check_diactionformatA_( __LINE__, a, b )
static void check_diactionformatA_( int line, const DIACTIONFORMATA *actual, const DIACTIONFORMATA *expected )
{
    DWORD i;
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwSize );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwActionSize );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwDataSize );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwNumActions );
    for (i = 0; i < min( actual->dwNumActions, expected->dwNumActions ); ++i)
    {
        winetest_push_context( "action[%lu]", i );
        check_diactionA_( line, actual->rgoAction + i, expected->rgoAction + i );
        winetest_pop_context();
        if (expected->dwActionSize != sizeof(DIACTIONA)) break;
        if (actual->dwActionSize != sizeof(DIACTIONA)) break;
    }
    check_member_guid_( __FILE__, line, *actual, *expected, guidActionMap );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwGenre );
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwBufferSize );
    check_member_( __FILE__, line, *actual, *expected, "%+ld", lAxisMin );
    check_member_( __FILE__, line, *actual, *expected, "%+ld", lAxisMax );
    check_member_( __FILE__, line, *actual, *expected, "%p", hInstString );
    check_member_( __FILE__, line, *actual, *expected, "%ld", ftTimeStamp.dwLowDateTime );
    check_member_( __FILE__, line, *actual, *expected, "%ld", ftTimeStamp.dwHighDateTime );
    todo_wine
    check_member_( __FILE__, line, *actual, *expected, "%#lx", dwCRC );
    check_member_str_( __FILE__, line, *actual, *expected, tszActionMap );
}

static void test_sys_keyboard( DWORD version )
{
    const DIDEVCAPS expect_caps =
    {
        .dwSize = sizeof(DIDEVCAPS),
        .dwFlags = DIDC_ATTACHED | DIDC_EMULATED,
        .dwDevType = version < 0x800 ? (DIDEVTYPEKEYBOARD_PCENH << 8) | DIDEVTYPE_KEYBOARD
                                     : (DI8DEVTYPEKEYBOARD_PCENH << 8) | DI8DEVTYPE_KEYBOARD,
        .dwButtons = 128,
    };
    const DIDEVICEINSTANCEW expect_devinst =
    {
        .dwSize = sizeof(DIDEVICEINSTANCEW),
        .guidInstance = GUID_SysKeyboard,
        .guidProduct = GUID_SysKeyboard,
        .dwDevType = version < 0x800 ? (DIDEVTYPEKEYBOARD_PCENH << 8) | DIDEVTYPE_KEYBOARD
                                     : (DI8DEVTYPEKEYBOARD_PCENH << 8) | DI8DEVTYPE_KEYBOARD,
        .tszInstanceName = L"Keyboard",
        .tszProductName = L"Keyboard",
        .guidFFDriver = GUID_NULL,
    };

    DIDEVICEOBJECTINSTANCEW expect_objects[] =
    {
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Key,
            .dwOfs = DIK_ESCAPE,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(DIK_ESCAPE),
            .tszName = L"Esc",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Key,
            .dwOfs = DIK_1,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(DIK_1),
            .tszName = L"1",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Key,
            .dwOfs = DIK_2,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(DIK_2),
            .tszName = L"2",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Key,
            .dwOfs = DIK_3,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(DIK_3),
            .tszName = L"3",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Key,
            .dwOfs = DIK_4,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(DIK_4),
            .tszName = L"4",
        },
    };
    struct check_objects_todos todo_objects[ARRAY_SIZE(expect_objects)] = {{0}};
    struct check_objects_params check_objects_params =
    {
        .stop_count = ARRAY_SIZE(expect_objects),
        .expect_count = ARRAY_SIZE(expect_objects),
        .expect_objs = expect_objects,
        .todo_objs = todo_objects,
    };
    DIPROPGUIDANDPATH prop_guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPSTRING prop_string =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPSTRING),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPRANGE prop_range =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPRANGE),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };

    LONG key_state[6], zero_state[6] = {0};
    DIOBJECTDATAFORMAT obj_data_format[] =
    {
        {&GUID_Key, sizeof(LONG) * 0, DIDFT_MAKEINSTANCE( DIK_Q ) | DIDFT_BUTTON, 0},
        {&GUID_Key, sizeof(LONG) * 1, DIDFT_MAKEINSTANCE( DIK_W ) | DIDFT_BUTTON, 0},
        {&GUID_Key, sizeof(LONG) * 2, DIDFT_MAKEINSTANCE( DIK_E ) | DIDFT_BUTTON, 0},
        {&GUID_Key, sizeof(LONG) * 4, DIDFT_MAKEINSTANCE( DIK_R ) | DIDFT_BUTTON, 0},
    };
    DIDATAFORMAT data_format =
    {
        sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT),  DIDF_RELAXIS,
        sizeof(key_state), ARRAY_SIZE(obj_data_format), obj_data_format,
    };

    DIDEVICEOBJECTINSTANCEW objinst = {0};
    DIDEVICEINSTANCEW devinst = {0};
    BOOL old_localized = localized;
    IDirectInputDevice8W *device;
    DIDEVCAPS caps = {0};
    BYTE full_state[256];
    HKL hkl, old_hkl;
    HWND hwnd, child;
    ULONG res, ref;
    HANDLE event;
    HRESULT hr;
    GUID guid;

    if (FAILED(create_dinput_device( version, &GUID_SysKeyboard, &device ))) return;

    localized = TRUE; /* Skip name tests, Wine sometimes succeeds depending on the host key names */
    winetest_push_context( "%#lx", version );

    hr = IDirectInputDevice8_Initialize( device, instance, version, &GUID_SysKeyboardEm );
    ok( hr == DI_OK, "Initialize returned %#lx\n", hr );
    guid = GUID_SysKeyboardEm;
    memset( &devinst, 0, sizeof(devinst) );
    devinst.dwSize = sizeof(DIDEVICEINSTANCEW);
    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    ok( IsEqualGUID( &guid, &GUID_SysKeyboardEm ), "got %s expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_SysKeyboardEm ) );

    hr = IDirectInputDevice8_Initialize( device, instance, version, &GUID_SysKeyboard );
    ok( hr == DI_OK, "Initialize returned %#lx\n", hr );

    memset( &devinst, 0, sizeof(devinst) );
    devinst.dwSize = sizeof(DIDEVICEINSTANCEW);
    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    check_member( devinst, expect_devinst, "%lu", dwSize );
    check_member_guid( devinst, expect_devinst, guidInstance );
    check_member_guid( devinst, expect_devinst, guidProduct );
    check_member( devinst, expect_devinst, "%#lx", dwDevType );
    if (!localized) check_member_wstr( devinst, expect_devinst, tszInstanceName );
    if (!localized) todo_wine check_member_wstr( devinst, expect_devinst, tszProductName );
    check_member_guid( devinst, expect_devinst, guidFFDriver );
    check_member( devinst, expect_devinst, "%04x", wUsagePage );
    check_member( devinst, expect_devinst, "%04x", wUsage );

    devinst.dwSize = sizeof(DIDEVICEINSTANCE_DX3W);
    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    check_member_guid( devinst, expect_devinst, guidInstance );
    check_member_guid( devinst, expect_devinst, guidProduct );
    check_member( devinst, expect_devinst, "%#lx", dwDevType );
    if (!localized) check_member_wstr( devinst, expect_devinst, tszInstanceName );
    if (!localized) todo_wine check_member_wstr( devinst, expect_devinst, tszProductName );

    caps.dwSize = sizeof(DIDEVCAPS);
    hr = IDirectInputDevice8_GetCapabilities( device, &caps );
    ok( hr == DI_OK, "GetCapabilities returned %#lx\n", hr );
    check_member( caps, expect_caps, "%lu", dwSize );
    check_member( caps, expect_caps, "%#lx", dwFlags );
    check_member( caps, expect_caps, "%#lx", dwDevType );
    check_member( caps, expect_caps, "%lu", dwAxes );
    todo_wine
    check_member( caps, expect_caps, "%lu", dwButtons );
    check_member( caps, expect_caps, "%lu", dwPOVs );
    check_member( caps, expect_caps, "%lu", dwFFSamplePeriod );
    check_member( caps, expect_caps, "%lu", dwFFMinTimeResolution );
    todo_wine
    check_member( caps, expect_caps, "%lu", dwFirmwareRevision );
    todo_wine
    check_member( caps, expect_caps, "%lu", dwHardwareRevision );
    check_member( caps, expect_caps, "%lu", dwFFDriverVersion );

    prop_dword.diph.dwHow = DIPH_BYOFFSET;
    prop_dword.diph.dwObj = 1;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_DEADZONE, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_DEADZONE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_SATURATION, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_SATURATION returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_CALIBRATIONMODE, &prop_dword.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_CALIBRATIONMODE returned %#lx\n", hr );
    prop_range.diph.dwHow = DIPH_BYOFFSET;
    prop_range.diph.dwObj = 1;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_RANGE, &prop_range.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_RANGE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_LOGICALRANGE, &prop_range.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_LOGICALRANGE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_PHYSICALRANGE, &prop_range.diph );
    ok( hr == DIERR_NOTFOUND, "GetProperty DIPROP_PHYSICALRANGE returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_AXISMODE, &prop_dword.diph );
    todo_wine
    ok( hr == DI_OK, "GetProperty DIPROP_AXISMODE returned %#lx\n", hr );
    todo_wine
    ok( prop_dword.dwData == DIPROPAXISMODE_ABS, "got %lu expected %u\n", prop_dword.dwData, DIPROPAXISMODE_ABS );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_BUFFERSIZE, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_BUFFERSIZE returned %#lx\n", hr );
    ok( prop_dword.dwData == 0, "got %#lx expected %#x\n", prop_dword.dwData, 0 );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    ok( prop_dword.dwData == 10000, "got %lu expected %u\n", prop_dword.dwData, 10000 );

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIKeyboard );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_AXISMODE, &prop_dword.diph );
    todo_wine
    ok( hr == DI_OK, "GetProperty DIPROP_AXISMODE returned %#lx\n", hr );
    todo_wine
    ok( prop_dword.dwData == DIPROPAXISMODE_REL, "got %lu expected %u\n", prop_dword.dwData, DIPROPAXISMODE_ABS );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_VIDPID, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_VIDPID returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &prop_guid_path.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_GUIDANDPATH returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_INSTANCENAME, &prop_string.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_INSTANCENAME returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_PRODUCTNAME, &prop_string.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_PRODUCTNAME returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_TYPENAME, &prop_string.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_TYPENAME returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_USERNAME, &prop_string.diph );
    if (version < 0x0800)
        ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_USERNAME returned %#lx\n", hr );
    else
    {
        ok( hr == DI_NOEFFECT, "GetProperty DIPROP_USERNAME returned %#lx\n", hr );
        ok( !wcscmp( prop_string.wsz, L"" ), "got user %s\n", debugstr_w(prop_string.wsz) );
    }

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_JOYSTICKID, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_VIDPID returned %#lx\n", hr );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_CALIBRATION, &prop_dword.diph );
    ok( hr == DIERR_INVALIDPARAM, "GetProperty DIPROP_CALIBRATION returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_AUTOCENTER returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_DEADZONE, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_DEADZONE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_SCANCODE, &prop_dword.diph );
    ok( hr == (version < 0x800 ? DIERR_UNSUPPORTED : DIERR_INVALIDPARAM),
        "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_BYUSAGE;
    prop_dword.diph.dwObj = MAKELONG( HID_USAGE_KEYBOARD_LCTRL, HID_USAGE_PAGE_KEYBOARD );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_SCANCODE, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_SCANCODE returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_BYOFFSET;
    prop_dword.diph.dwObj = 1;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GRANULARITY, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_GRANULARITY returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_DEADZONE, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_DEADZONE returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_SATURATION, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_SATURATION returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_CALIBRATIONMODE, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_CALIBRATIONMODE returned %#lx\n", hr );
    prop_range.diph.dwHow = DIPH_BYOFFSET;
    prop_range.diph.dwObj = 1;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_RANGE, &prop_range.diph );
    ok( hr == DIERR_UNSUPPORTED, "GetProperty DIPROP_RANGE returned %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DIERR_INVALIDPARAM, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    prop_dword.dwData = 1000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );

    res = 0;
    hr = IDirectInputDevice8_EnumObjects( device, check_object_count, &res, DIDFT_AXIS | DIDFT_PSHBUTTON );
    ok( hr == DI_OK, "EnumObjects returned %#lx\n", hr );
    if (!localized) todo_wine ok( res == 127, "got %lu expected %u\n", res, 127 );
    hr = IDirectInputDevice8_EnumObjects( device, check_objects, &check_objects_params, DIDFT_ALL );
    ok( hr == DI_OK, "EnumObjects returned %#lx\n", hr );
    ok( check_objects_params.index >= check_objects_params.expect_count, "missing %u objects\n",
        check_objects_params.expect_count - check_objects_params.index );

    objinst.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW);
    res = MAKELONG( HID_USAGE_KEYBOARD_LCTRL, HID_USAGE_PAGE_KEYBOARD );
    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, res, DIPH_BYUSAGE );
    ok( hr == DIERR_UNSUPPORTED, "GetObjectInfo returned: %#lx\n", hr );

    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, 2, DIPH_BYOFFSET );
    ok( hr == DI_OK, "GetObjectInfo returned: %#lx\n", hr );

    check_member( objinst, expect_objects[1], "%lu", dwSize );
    check_member_guid( objinst, expect_objects[1], guidType );
    check_member( objinst, expect_objects[1], "%#lx", dwOfs );
    check_member( objinst, expect_objects[1], "%#lx", dwType );
    check_member( objinst, expect_objects[1], "%#lx", dwFlags );
    if (!localized) check_member_wstr( objinst, expect_objects[1], tszName );
    check_member( objinst, expect_objects[1], "%lu", dwFFMaxForce );
    check_member( objinst, expect_objects[1], "%lu", dwFFForceResolution );
    check_member( objinst, expect_objects[1], "%u", wCollectionNumber );
    check_member( objinst, expect_objects[1], "%u", wDesignatorIndex );
    check_member( objinst, expect_objects[1], "%#04x", wUsagePage );
    check_member( objinst, expect_objects[1], "%#04x", wUsage );
    check_member( objinst, expect_objects[1], "%#lx", dwDimension );
    check_member( objinst, expect_objects[1], "%#04x", wExponent );
    check_member( objinst, expect_objects[1], "%u", wReportId );

    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, 423, DIPH_BYOFFSET );
    ok( hr == DIERR_NOTFOUND, "GetObjectInfo returned: %#lx\n", hr );
    res = DIDFT_TGLBUTTON | DIDFT_MAKEINSTANCE( 3 );
    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, res, DIPH_BYID );
    ok( hr == DIERR_NOTFOUND, "GetObjectInfo returned: %#lx\n", hr );
    res = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 3 );
    hr = IDirectInputDevice8_GetObjectInfo( device, &objinst, res, DIPH_BYID );
    ok( hr == DI_OK, "GetObjectInfo returned: %#lx\n", hr );

    check_member( objinst, expect_objects[2], "%lu", dwSize );
    check_member_guid( objinst, expect_objects[2], guidType );
    check_member( objinst, expect_objects[2], "%#lx", dwOfs );
    check_member( objinst, expect_objects[2], "%#lx", dwType );
    check_member( objinst, expect_objects[2], "%#lx", dwFlags );
    if (!localized) check_member_wstr( objinst, expect_objects[2], tszName );
    check_member( objinst, expect_objects[2], "%lu", dwFFMaxForce );
    check_member( objinst, expect_objects[2], "%lu", dwFFForceResolution );
    check_member( objinst, expect_objects[2], "%u", wCollectionNumber );
    check_member( objinst, expect_objects[2], "%u", wDesignatorIndex );
    check_member( objinst, expect_objects[2], "%#04x", wUsagePage );
    check_member( objinst, expect_objects[2], "%#04x", wUsage );
    check_member( objinst, expect_objects[2], "%#lx", dwDimension );
    check_member( objinst, expect_objects[2], "%#04x", wExponent );
    check_member( objinst, expect_objects[2], "%u", wReportId );


    hwnd = create_foreground_window( FALSE );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_FOREGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_FOREGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_BACKGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_BACKGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_FOREGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_FOREGROUND|DISCL_EXCLUSIVE );
    todo_wine_if( version == 0x500 )
    ok( hr == (version == 0x500 ? DIERR_INVALIDPARAM : DI_OK), "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND|DISCL_EXCLUSIVE );
    todo_wine_if( version == 0x500 )
    ok( hr == (version == 0x500 ? DIERR_INVALIDPARAM : DIERR_UNSUPPORTED), "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );

    child = CreateWindowW( L"static", L"static", WS_CHILD | WS_VISIBLE,
                           10, 10, 50, 50, hwnd, NULL, NULL, NULL );
    ok( !!child, "CreateWindowW failed, error %lu\n", GetLastError() );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_FOREGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_FOREGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_BACKGROUND );
    ok( hr == DIERR_INVALIDPARAM, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_BACKGROUND|DISCL_EXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, child, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
    ok( hr == E_HANDLE, "SetCooperativeLevel returned %#lx\n", hr );

    DestroyWindow( child );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!event, "CreateEventW failed, error %lu\n", GetLastError() );

    hkl = activate_keyboard_layout( MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &old_hkl );
    if (LOWORD(hkl) != MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT)) goto skip_key_tests;

    hr = IDirectInputDevice8_SetEventNotification( device, event );
    ok( hr == DI_OK, "SetEventNotification returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIKeyboard );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, 10, full_state );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(full_state), full_state );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_NOEFFECT, "Unacquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_NOEFFECT, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, 10, full_state );
    ok( hr == DIERR_INVALIDPARAM, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(full_state), full_state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetDataFormat( device, &data_format );
    ok( hr == DI_OK, "SetDataFormat returned %#lx\n", hr );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(key_state), key_state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(full_state), full_state );
    ok( hr == DIERR_INVALIDPARAM, "GetDeviceState returned %#lx\n", hr );

    memset( key_state, 0x56, sizeof(key_state) );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(key_state), key_state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );
    ok( !memcmp( key_state, zero_state, sizeof(key_state) ), "got non zero state\n" );

    keybd_event( 'Q', 0, 0, 0 );
    res = WaitForSingleObject( event, 100 );
    if (res == WAIT_TIMEOUT) /* Acquire is asynchronous */
    {
        keybd_event( 'Q', 0, 0, 0 );
        res = WaitForSingleObject( event, 5000 );
    }
    ok( !res, "WaitForSingleObject returned %#lx\n", res );

    memset( key_state, 0xcd, sizeof(key_state) );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(key_state), key_state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );
    flaky_wine_if( version < 0x800 && key_state[0] == 0 )
    ok( key_state[0] == (version < 0x800 ? 0x80 : 0), "got key_state[0] %lu\n", key_state[0] );

    /* unacquiring should reset the device state */
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(key_state), key_state );
    ok( hr == DI_OK, "GetDeviceState returned %#lx\n", hr );
    ok( !memcmp( key_state, zero_state, sizeof(key_state) ), "got non zero state\n" );

    keybd_event( 'Q', 0, KEYEVENTF_KEYUP, 0 );

    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned %#lx\n", hr );

skip_key_tests:
    ActivateKeyboardLayout( old_hkl, 0 );

    test_dik_codes( device, event, hwnd, version );
    test_scan_codes( device, event, hwnd, version );

    CloseHandle( event );
    DestroyWindow( hwnd );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

    winetest_pop_context();
    localized = old_localized;
}

static void test_sys_keyboard_action_format(void)
{
    DIACTIONA actions[] =
    {
        {.uAppData = 0x1, .dwSemantic = 0x810004c8, .dwFlags = DIA_APPNOMAP},
        {.uAppData = 0x1, .dwSemantic = 0x81000448, .dwFlags = 0},
    };
    const DIACTIONA expect_actions[ARRAY_SIZE(actions)] =
    {
        {.uAppData = 0x1, .dwSemantic = 0x810004c8, .dwFlags = 0, .guidInstance = GUID_SysKeyboard, .dwObjID = 0xc804, .dwHow = DIAH_DEFAULT},
        {.uAppData = 0x1, .dwSemantic = 0x81000448, .dwFlags = 0, .guidInstance = GUID_SysKeyboard, .dwObjID = 0x4804, .dwHow = DIAH_DEFAULT},
    };
    DIACTIONFORMATA action_format =
    {
        .dwSize = sizeof(DIACTIONFORMATA),
        .dwActionSize = sizeof(DIACTIONA),
        .dwNumActions = ARRAY_SIZE(actions),
        .dwDataSize = ARRAY_SIZE(actions) * 4,
        .rgoAction = actions,
        .dwGenre = 0x1000000,
        .guidActionMap = GUID_keyboard_action_mapping,
    };
    const DIACTIONFORMATA expect_action_format =
    {
        .dwSize = sizeof(DIACTIONFORMATA),
        .dwActionSize = sizeof(DIACTIONA),
        .dwNumActions = ARRAY_SIZE(expect_actions),
        .dwDataSize = ARRAY_SIZE(expect_actions) * 4,
        .rgoAction = (DIACTIONA *)expect_actions,
        .dwGenre = 0x1000000,
        .guidActionMap = GUID_keyboard_action_mapping,
        .dwCRC = 0x68e7e227,
    };
    IDirectInputDevice8A *device;
    IDirectInput8A *dinput;
    HRESULT hr;
    LONG ref;

    hr = DirectInput8Create( instance, 0x800, &IID_IDirectInput8A, (void **)&dinput, NULL );
    ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );
    hr = IDirectInput_CreateDevice( dinput, &GUID_SysKeyboard, &device, NULL );
    ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );
    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %ld\n", ref );

    hr = IDirectInputDevice8_BuildActionMap( device, &action_format, NULL, 2 );
    ok( hr == DI_OK, "BuildActionMap returned %#lx\n", hr );
    check_diactionformatA(&action_format, &expect_action_format);

    hr = IDirectInputDevice8_SetActionMap( device, &action_format, NULL, 2 );
    ok( hr == DI_SETTINGSNOTSAVED, "SetActionMap returned %#lx\n", hr );
    hr = IDirectInputDevice8_SetActionMap( device, &action_format, NULL, 0 );
    ok( hr == DI_OK, "SetActionMap returned %#lx\n", hr );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );
}

static UINT key_down_count;
static UINT key_up_count;

static LRESULT CALLBACK keyboard_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    BOOL us_kbd = (GetKeyboardLayout(0) == (HKL)(ULONG_PTR)0x04090409);

    if (msg == WM_KEYDOWN)
    {
        key_down_count++;
        if (us_kbd) ok( wparam == 'A', "got wparam %#Ix\n", wparam );
        ok( lparam == 0x1e0001, "got lparam %#Ix\n", lparam );
    }
    if (msg == WM_KEYUP)
    {
        key_up_count++;
        if (us_kbd) ok( wparam == 'A', "got wparam %#Ix\n", wparam );
        ok( lparam == 0xc01e0001, "got lparam %#Ix\n", lparam );
    }
    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

static void test_hid_keyboard(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_KEYBOARD),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_KEYBOARD),
            COLLECTION(1, Physical),
                REPORT_ID(1, 1),

                USAGE_PAGE(1, HID_USAGE_PAGE_KEYBOARD),
                USAGE_MINIMUM(1, HID_USAGE_KEYBOARD_aA),
                USAGE_MAXIMUM(1, HID_USAGE_KEYBOARD_zZ),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 32),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    const HID_DEVICE_ATTRIBUTES attributes =
    {
        .Size = sizeof(HID_DEVICE_ATTRIBUTES),
        .VendorID = LOWORD(EXPECT_VIDPID),
        .ProductID = 0x0003,
        .VersionNumber = 0x0100,
    };
    struct hid_device_desc desc =
    {
        .caps = { .InputReportByteLength = 5 },
        .attributes = attributes,
        .use_report_id = 1,
    };
    struct hid_expect key_press_release[] =
    {
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_buf = {1,0x01,0x00,0x00,0x00},
        },
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_buf = {1,0x00,0x00,0x00,0x00},
        },
    };

    WCHAR device_path[MAX_PATH];
    HANDLE file;
    DWORD res;
    HWND hwnd;
    BOOL ret;

    winetest_push_context( "keyboard" );

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start_( &desc, 1, 5000 /* needs a long timeout on Win7 */ )) goto done;

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );


    /* windows doesn't let us open HID keyboard devices */

    file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    todo_wine
    ok( file == INVALID_HANDLE_VALUE, "CreateFileW succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError() );
    CloseHandle( file );

    file = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                        NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );


    /* check basic keyboard input injection to window message */

    hwnd = create_foreground_window( FALSE );
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)keyboard_wndproc );

    key_down_count = 0;
    key_up_count = 0;
    bus_send_hid_input( file, &desc, &key_press_release[0], sizeof(key_press_release[0]) );

    res = MsgWaitForMultipleObjects( 0, NULL, FALSE, 500, QS_KEY );
    todo_wine
    ok( !res, "MsgWaitForMultipleObjects returned %#lx\n", res );
    msg_wait_for_events( 0, NULL, 5 ); /* process pending messages */
    todo_wine
    ok( key_down_count == 1, "got key_down_count %u\n", key_down_count );
    ok( key_up_count == 0, "got key_up_count %u\n", key_up_count );

    bus_send_hid_input( file, &desc, &key_press_release[1], sizeof(key_press_release[1]) );

    res = MsgWaitForMultipleObjects( 0, NULL, FALSE, 500, QS_KEY );
    todo_wine
    ok( !res, "MsgWaitForMultipleObjects returned %#lx\n", res );
    msg_wait_for_events( 0, NULL, 5 ); /* process pending messages */
    todo_wine
    ok( key_down_count == 1, "got key_down_count %u\n", key_down_count );
    todo_wine
    ok( key_up_count == 1, "got key_up_count %u\n", key_up_count );

    DestroyWindow( hwnd );


    CloseHandle( file );

done:
    hid_device_stop( &desc, 1 );

    winetest_pop_context();
}

START_TEST(device8)
{
    dinput_test_init();

    test_QueryInterface( 0x300 );
    test_QueryInterface( 0x500 );
    test_QueryInterface( 0x700 );
    test_QueryInterface( 0x800 );

    test_overlapped_format( 0x700 );
    test_overlapped_format( 0x800 );

    test_sys_mouse( 0x500 );
    test_sys_mouse( 0x700 );
    test_sys_mouse( 0x800 );

    test_sys_keyboard( 0x500 );
    test_sys_keyboard( 0x700 );
    test_sys_keyboard( 0x800 );

    test_sys_keyboard_action_format();

    test_mouse_keyboard();
    test_keyboard_events();
    test_appdata_property();

    if (!bus_device_start()) goto done;

    test_hid_mouse();
    test_hid_keyboard();
    test_hid_touch_screen();

done:
    bus_device_stop();
    dinput_test_exit();
}
