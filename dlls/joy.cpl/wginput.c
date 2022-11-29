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
 *
 */

#include "stdarg.h"
#include "stddef.h"

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "winstring.h"
#include "roapi.h"

#include "initguid.h"
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#define WIDL_using_Windows_Foundation_Numerics
#include "windows.foundation.h"
#define WIDL_using_Windows_Devices_Power
#define WIDL_using_Windows_Gaming_Input
#include "windows.gaming.input.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "joy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

struct iface
{
    struct list entry;
    IGameController *iface;
};

struct device
{
    struct list entry;
    IRawGameController *device;
};

struct raw_controller_state
{
    UINT64 timestamp;
    double axes[6];
    boolean buttons[32];
    GameControllerSwitchPosition switches[4];
};

struct device_state
{
    const GUID *iid;
    union
    {
        struct raw_controller_state raw_controller;
        GamepadReading gamepad;
    };
};

static CRITICAL_SECTION state_cs;
static CRITICAL_SECTION_DEBUG state_cs_debug =
{
    0, 0, &state_cs,
    { &state_cs_debug.ProcessLocksList, &state_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": state_cs") }
};
static CRITICAL_SECTION state_cs = { &state_cs_debug, -1, 0, 0, 0, 0 };

static struct device_state device_state;
static struct list devices = LIST_INIT( devices );
static struct list ifaces = LIST_INIT( ifaces );
static IGameController *iface_selected;

static HWND dialog_hwnd;
static HANDLE state_event;

static void set_device_state( struct device_state *state )
{
    BOOL modified;

    EnterCriticalSection( &state_cs );
    modified = memcmp( &device_state, state, sizeof(*state) );
    device_state = *state;
    LeaveCriticalSection( &state_cs );

    if (modified) SendMessageW( dialog_hwnd, WM_USER, 0, 0 );
}

static void set_selected_interface( IGameController *iface )
{
    IGameController *previous;

    EnterCriticalSection( &state_cs );

    if ((previous = iface_selected)) IGameController_Release( previous );
    if ((iface_selected = iface)) IGameController_AddRef( iface );

    LeaveCriticalSection( &state_cs );
}

static IGameController *get_selected_interface(void)
{
    IGameController *iface;

    EnterCriticalSection( &state_cs );
    iface = iface_selected;
    if (iface) IGameController_AddRef( iface );
    LeaveCriticalSection( &state_cs );

    return iface;
}

static void clear_interfaces(void)
{
    struct iface *entry, *next;

    set_selected_interface( NULL );

    LIST_FOR_EACH_ENTRY_SAFE( entry, next, &ifaces, struct iface, entry )
    {
        list_remove( &entry->entry );
        IGameController_Release( entry->iface );
        free( entry );
    }
}

static void clear_devices(void)
{
    struct device *entry, *next;

    LIST_FOR_EACH_ENTRY_SAFE( entry, next, &devices, struct device, entry )
    {
        list_remove( &entry->entry );
        IRawGameController_Release( entry->device );
        free( entry );
    }
}

static DWORD WINAPI input_thread_proc( void *param )
{
    union
    {
        struct raw_controller_state raw_controller;
        GamepadReading gamepad;
    } previous = {0};

    HANDLE stop_event = param;

    while (WaitForSingleObject( stop_event, 20 ) == WAIT_TIMEOUT)
    {
        IGameController *iface;

        if (!(iface = get_selected_interface()))
            memset( &previous, 0, sizeof(previous) );
        else
        {
            IRawGameController *raw_controller;
            IGamepad *gamepad;

            if (SUCCEEDED(IGameController_QueryInterface( iface, &IID_IRawGameController, (void **)&raw_controller )))
            {
                struct device_state state = {.iid = &IID_IRawGameController};
                struct raw_controller_state *current = &state.raw_controller;
                IRawGameController_GetCurrentReading( raw_controller, ARRAY_SIZE(current->buttons), current->buttons,
                                                      ARRAY_SIZE(current->switches), current->switches,
                                                      ARRAY_SIZE(current->axes), current->axes, &current->timestamp );
                IRawGameController_Release( raw_controller );
                set_device_state( &state );
            }

            if (SUCCEEDED(IGameController_QueryInterface( iface, &IID_IGamepad, (void **)&gamepad )))
            {
                struct device_state state = {.iid = &IID_IGamepad};
                IGamepad_GetCurrentReading( gamepad, &state.gamepad );
                IGamepad_Release( gamepad );
                set_device_state( &state );
            }

            IGameController_Release( iface );
        }
    }

    return 0;
}

static void handle_wgi_interface_change( HWND hwnd )
{
    IGameController *iface;
    struct list *entry;
    int i;

    set_selected_interface( NULL );

    i = SendDlgItemMessageW( hwnd, IDC_WGI_INTERFACE, CB_GETCURSEL, 0, 0 );
    if (i < 0) return;

    entry = list_head( &ifaces );
    while (i-- && entry) entry = list_next( &ifaces, entry );
    if (!entry) return;

    iface = LIST_ENTRY( entry, struct iface, entry )->iface;
    set_selected_interface( iface );
}

static HRESULT check_gamepad_interface( IRawGameController *device, IGameController **iface )
{
    const WCHAR *class_name = RuntimeClass_Windows_Gaming_Input_Gamepad;
    IGameController *controller;
    IGamepadStatics2 *statics;
    IGamepad *gamepad = NULL;
    HSTRING str;

    WindowsCreateString( class_name, wcslen( class_name ), &str );
    RoGetActivationFactory( str, &IID_IGamepadStatics2, (void **)&statics );
    WindowsDeleteString( str );
    if (!statics) return E_NOINTERFACE;

    if (SUCCEEDED(IRawGameController_QueryInterface( device, &IID_IGameController, (void **)&controller )))
    {
        IGamepadStatics2_FromGameController( statics, controller, &gamepad );
        IGameController_Release( controller );
    }

    IGamepadStatics2_Release( statics );
    if (!gamepad) return E_NOINTERFACE;

    IGamepad_QueryInterface( gamepad, &IID_IGameController, (void **)iface );
    IGamepad_Release( gamepad );
    return S_OK;
}

static HRESULT check_racing_wheel_interface( IRawGameController *device, IGameController **iface )
{
    const WCHAR *class_name = RuntimeClass_Windows_Gaming_Input_RacingWheel;
    IRacingWheelStatics2 *statics;
    IGameController *controller;
    IRacingWheel *wheel = NULL;
    HSTRING str;

    WindowsCreateString( class_name, wcslen( class_name ), &str );
    RoGetActivationFactory( str, &IID_IRacingWheelStatics2, (void **)&statics );
    WindowsDeleteString( str );
    if (!statics) return E_NOINTERFACE;

    if (SUCCEEDED(IRawGameController_QueryInterface( device, &IID_IGameController, (void **)&controller )))
    {
        IRacingWheelStatics2_FromGameController( statics, controller, &wheel );
        IGameController_Release( controller );
    }

    IRacingWheelStatics2_Release( statics );
    if (!wheel) return E_NOINTERFACE;

    IRacingWheel_QueryInterface( wheel, &IID_IGameController, (void **)iface );
    IRacingWheel_Release( wheel );
    return S_OK;
}

static void update_wgi_interface( HWND hwnd, IRawGameController *device )
{
    IGameController *controller;
    struct iface *iface;

    clear_interfaces();

    if (SUCCEEDED(IRawGameController_QueryInterface( device, &IID_IGameController,
                                                     (void **)&controller )))
    {
        if (!(iface = calloc( 1, sizeof(*iface)))) goto done;
        list_add_tail( &ifaces, &iface->entry );
        iface->iface = controller;
    }
    if (SUCCEEDED(check_gamepad_interface( device, &controller )))
    {
        if (!(iface = calloc( 1, sizeof(*iface)))) goto done;
        list_add_tail( &ifaces, &iface->entry );
        iface->iface = controller;
    }
    if (SUCCEEDED(check_racing_wheel_interface( device, &controller )))
    {
        if (!(iface = calloc( 1, sizeof(*iface)))) goto done;
        list_add_tail( &ifaces, &iface->entry );
        iface->iface = controller;
    }
    controller = NULL;

    SendDlgItemMessageW( hwnd, IDC_WGI_INTERFACE, CB_RESETCONTENT, 0, 0 );

    LIST_FOR_EACH_ENTRY( iface, &ifaces, struct iface, entry )
    {
        HSTRING name;

        if (FAILED(IGameController_GetRuntimeClassName( iface->iface, &name ))) continue;

        SendDlgItemMessageW( hwnd, IDC_WGI_INTERFACE, CB_ADDSTRING, 0,
                             (LPARAM)(wcsrchr( WindowsGetStringRawBuffer( name, NULL ), '.' ) + 1) );

        WindowsDeleteString( name );
    }

done:
    if (controller) IGameController_Release( controller );

    SendDlgItemMessageW( hwnd, IDC_WGI_INTERFACE, CB_SETCURSEL, 0, 0 );
}

static void handle_wgi_devices_change( HWND hwnd )
{
    IRawGameController *device;
    struct list *entry;
    int i;

    i = SendDlgItemMessageW( hwnd, IDC_WGI_DEVICES, CB_GETCURSEL, 0, 0 );
    if (i < 0) return;

    entry = list_head( &devices );
    while (i-- && entry) entry = list_next( &devices, entry );
    if (!entry) return;

    device = LIST_ENTRY( entry, struct device, entry )->device;
    update_wgi_interface( hwnd, device );
}

static void update_wgi_devices( HWND hwnd )
{
    static const WCHAR *class_name = RuntimeClass_Windows_Gaming_Input_RawGameController;
    IVectorView_RawGameController *controllers;
    IRawGameControllerStatics *statics;
    struct device *entry;
    HSTRING str;
    UINT size;

    clear_devices();

    WindowsCreateString( class_name, wcslen( class_name ), &str );
    RoGetActivationFactory( str, &IID_IRawGameControllerStatics, (void **)&statics );
    WindowsDeleteString( str );

    IRawGameControllerStatics_get_RawGameControllers( statics, &controllers );
    IVectorView_RawGameController_get_Size( controllers, &size );

    while (size--)
    {
        struct device *entry;
        if (!(entry = calloc( 1, sizeof(struct device) ))) break;
        IVectorView_RawGameController_GetAt( controllers, size, &entry->device );
        list_add_tail( &devices, &entry->entry );
    }

    IVectorView_RawGameController_Release( controllers );
    IRawGameControllerStatics_Release( statics );

    SendDlgItemMessageW( hwnd, IDC_WGI_DEVICES, CB_RESETCONTENT, 0, 0 );

    LIST_FOR_EACH_ENTRY( entry, &devices, struct device, entry )
    {
        IRawGameController2 *controller2;

        if (SUCCEEDED(IRawGameController_QueryInterface( entry->device, &IID_IRawGameController2,
                                                         (void **)&controller2 )))
        {
            HSTRING name;

            IRawGameController2_get_DisplayName( controller2, &name );
            SendDlgItemMessageW( hwnd, IDC_WGI_DEVICES, CB_ADDSTRING, 0,
                                 (LPARAM)WindowsGetStringRawBuffer( name, NULL ) );
            WindowsDeleteString( name );

            IRawGameController2_Release( controller2 );
        }
        else
        {
            UINT16 vid = -1, pid = -1;
            WCHAR buffer[256];

            IRawGameController_get_HardwareVendorId( entry->device, &vid );
            IRawGameController_get_HardwareProductId( entry->device, &pid );

            swprintf( buffer, ARRAY_SIZE(buffer), L"%04x:%04x", vid, pid );
            SendDlgItemMessageW( hwnd, IDC_WGI_DEVICES, CB_ADDSTRING, 0, (LPARAM)buffer );
        }
    }
}

static void create_device_views( HWND hwnd )
{
}

extern INT_PTR CALLBACK test_wgi_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    static HANDLE thread, thread_stop;

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    switch (msg)
    {
    case WM_INITDIALOG:
        create_device_views( hwnd );
        return TRUE;

    case WM_COMMAND:
        switch (wparam)
        {
        case MAKEWPARAM( IDC_WGI_DEVICES, CBN_SELCHANGE ):
            handle_wgi_devices_change( hwnd );
            handle_wgi_interface_change( hwnd );
            break;

        case MAKEWPARAM( IDC_WGI_INTERFACE, CBN_SELCHANGE ):
            handle_wgi_interface_change( hwnd );
            break;
        }
        return TRUE;


    case WM_NOTIFY:
        switch (((NMHDR *)lparam)->code)
        {
        case PSN_SETACTIVE:
            RoInitialize( RO_INIT_MULTITHREADED );

            dialog_hwnd = hwnd;
            state_event = CreateEventW( NULL, FALSE, FALSE, NULL );
            thread_stop = CreateEventW( NULL, FALSE, FALSE, NULL );

            update_wgi_devices( hwnd );

            SendDlgItemMessageW( hwnd, IDC_WGI_DEVICES, CB_SETCURSEL, 0, 0 );
            handle_wgi_devices_change( hwnd );

            SendDlgItemMessageW( hwnd, IDC_WGI_INTERFACE, CB_SETCURSEL, 0, 0 );
            handle_wgi_interface_change( hwnd );

            thread_stop = CreateEventW( NULL, FALSE, FALSE, NULL );
            thread = CreateThread( NULL, 0, input_thread_proc, (void *)thread_stop, 0, NULL );
            break;

        case PSN_RESET:
        case PSN_KILLACTIVE:
            SetEvent( thread_stop );
            MsgWaitForMultipleObjects( 1, &thread, FALSE, INFINITE, 0 );
            CloseHandle( state_event );
            CloseHandle( thread_stop );
            CloseHandle( thread );

            clear_devices();

            RoUninitialize();
            break;
        }
        return TRUE;

    case WM_USER:
        return TRUE;
    }

    return FALSE;
}
