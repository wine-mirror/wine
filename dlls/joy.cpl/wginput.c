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

struct device
{
    struct list entry;
    IRawGameController *device;
};

static CRITICAL_SECTION state_cs;
static CRITICAL_SECTION_DEBUG state_cs_debug =
{
    0, 0, &state_cs,
    { &state_cs_debug.ProcessLocksList, &state_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": state_cs") }
};
static CRITICAL_SECTION state_cs = { &state_cs_debug, -1, 0, 0, 0, 0 };

static struct list devices = LIST_INIT( devices );
static IRawGameController *device_selected;

static HWND dialog_hwnd;
static HANDLE state_event;

static void set_selected_device( IRawGameController *device )
{
    IRawGameController *previous;

    EnterCriticalSection( &state_cs );

    if ((previous = device_selected)) IRawGameController_Release( previous );
    if ((device_selected = device)) IRawGameController_AddRef( device );

    LeaveCriticalSection( &state_cs );
}

static void clear_devices(void)
{
    struct device *entry, *next;

    set_selected_device( NULL );

    LIST_FOR_EACH_ENTRY_SAFE( entry, next, &devices, struct device, entry )
    {
        list_remove( &entry->entry );
        IRawGameController_Release( entry->device );
        free( entry );
    }
}

static DWORD WINAPI input_thread_proc( void *param )
{
    HANDLE stop_event = param;

    while (WaitForSingleObject( stop_event, 20 ) == WAIT_TIMEOUT)
    {
    }

    return 0;
}

static void handle_wgi_devices_change( HWND hwnd )
{
    IRawGameController *device;
    struct list *entry;
    int i;

    set_selected_device( NULL );

    i = SendDlgItemMessageW( hwnd, IDC_WGI_DEVICES, CB_GETCURSEL, 0, 0 );
    if (i < 0) return;

    entry = list_head( &devices );
    while (i-- && entry) entry = list_next( &devices, entry );
    if (!entry) return;

    device = LIST_ENTRY( entry, struct device, entry )->device;
    set_selected_device( device );
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

            swprintf( buffer, ARRAY_SIZE(buffer), L"RawGameController %04x:%04x", vid, pid );
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
