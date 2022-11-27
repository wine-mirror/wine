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

#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "xinput.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "joy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

struct device_state
{
    XINPUT_CAPABILITIES caps;
    XINPUT_STATE state;
    DWORD status;
};

static CRITICAL_SECTION state_cs;
static CRITICAL_SECTION_DEBUG state_cs_debug =
{
    0, 0, &state_cs,
    { &state_cs_debug.ProcessLocksList, &state_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": state_cs") }
};
static CRITICAL_SECTION state_cs = { &state_cs_debug, -1, 0, 0, 0, 0 };

static struct device_state devices_state[XUSER_MAX_COUNT] =
{
    {.status = ERROR_DEVICE_NOT_CONNECTED},
    {.status = ERROR_DEVICE_NOT_CONNECTED},
    {.status = ERROR_DEVICE_NOT_CONNECTED},
    {.status = ERROR_DEVICE_NOT_CONNECTED},
};
static HWND dialog_hwnd;

static void set_device_state( DWORD index, struct device_state *state )
{
    BOOL modified;

    EnterCriticalSection( &state_cs );
    modified = memcmp( devices_state + index, state, sizeof(*state) );
    devices_state[index] = *state;
    LeaveCriticalSection( &state_cs );

    if (modified) SendMessageW( dialog_hwnd, WM_USER, index, 0 );
}

static void get_device_state( DWORD index, struct device_state *state )
{
    EnterCriticalSection( &state_cs );
    *state = devices_state[index];
    LeaveCriticalSection( &state_cs );
}

static DWORD WINAPI input_thread_proc( void *param )
{
    HANDLE thread_stop = param;
    DWORD i;

    while (WaitForSingleObject( thread_stop, 20 ) == WAIT_TIMEOUT)
    {
        for (i = 0; i < ARRAY_SIZE(devices_state); ++i)
        {
            struct device_state state = {0};
            state.status = XInputGetCapabilities( i, 0, &state.caps );
            if (!state.status) state.status = XInputGetState( i, &state.state );
            set_device_state( i, &state );
        }
    }

    return 0;
}

static void create_user_view( HWND hwnd, DWORD index )
{
    HWND parent;

    parent = GetDlgItem( hwnd, IDC_XI_USER_0 + index );

    ShowWindow( parent, SW_HIDE );
}

static void update_user_view( HWND hwnd, DWORD index )
{
    struct device_state state;
    HWND parent;

    get_device_state( index, &state );

    parent = GetDlgItem( hwnd, IDC_XI_NO_USER_0 + index );
    ShowWindow( parent, state.status ? SW_SHOW : SW_HIDE );

    parent = GetDlgItem( hwnd, IDC_XI_USER_0 + index );
    ShowWindow( parent, state.status ? SW_HIDE : SW_SHOW );
}

extern INT_PTR CALLBACK test_xi_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    static HANDLE thread, thread_stop;

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    switch (msg)
    {
    case WM_INITDIALOG:
        create_user_view( hwnd, 0 );
        create_user_view( hwnd, 1 );
        create_user_view( hwnd, 2 );
        create_user_view( hwnd, 3 );
        return TRUE;

    case WM_COMMAND:
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR *)lparam)->code)
        {
        case PSN_SETACTIVE:
            dialog_hwnd = hwnd;
            thread_stop = CreateEventW( NULL, FALSE, FALSE, NULL );
            thread = CreateThread( NULL, 0, input_thread_proc, (void *)thread_stop, 0, NULL );
            break;

        case PSN_RESET:
        case PSN_KILLACTIVE:
            SetEvent( thread_stop );
            MsgWaitForMultipleObjects( 1, &thread, FALSE, INFINITE, 0 );
            CloseHandle( thread_stop );
            CloseHandle( thread );
            dialog_hwnd = 0;
            break;
        }
        return TRUE;

    case WM_USER:
        update_user_view( hwnd, wparam );
        return TRUE;
    }

    return FALSE;
}
