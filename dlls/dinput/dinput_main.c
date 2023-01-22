/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2002 TransGaming Technologies Inc.
 * Copyright 2007 Vitaliy Margolen
 *
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "devguid.h"
#include "hidusage.h"
#include "initguid.h"
#include "dinputd.h"

#include "dinput_private.h"
#include "device_private.h"

#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

#define INPUT_THREAD_MAX_DEVICES 128

struct input_thread_state
{
    BOOL running;
    UINT events_count;
    UINT devices_count;
    HHOOK mouse_ll_hook;
    HHOOK keyboard_ll_hook;
    HHOOK callwndproc_hook;
    RAWINPUTDEVICE rawinput_devices[2];
    struct dinput_device *devices[INPUT_THREAD_MAX_DEVICES];
    HANDLE events[INPUT_THREAD_MAX_DEVICES];
};

static inline struct dinput_device *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8W_iface );
}

HINSTANCE DINPUT_instance;

static HWND di_em_win;
static HANDLE dinput_thread;
static UINT input_thread_user_count;
static struct input_thread_state *input_thread_state;

static CRITICAL_SECTION dinput_hook_crit;
static CRITICAL_SECTION_DEBUG dinput_critsect_debug =
{
    0, 0, &dinput_hook_crit,
    { &dinput_critsect_debug.ProcessLocksList, &dinput_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dinput_hook_crit") }
};
static CRITICAL_SECTION dinput_hook_crit = { &dinput_critsect_debug, -1, 0, 0, 0, 0 };

static struct list acquired_mouse_list = LIST_INIT( acquired_mouse_list );
static struct list acquired_rawmouse_list = LIST_INIT( acquired_rawmouse_list );
static struct list acquired_keyboard_list = LIST_INIT( acquired_keyboard_list );
static struct list acquired_device_list = LIST_INIT( acquired_device_list );

void dinput_hooks_acquire_device( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    EnterCriticalSection( &dinput_hook_crit );
    if (IsEqualGUID( &impl->guid, &GUID_SysMouse ))
        list_add_tail( impl->use_raw_input ? &acquired_rawmouse_list : &acquired_mouse_list, &impl->entry );
    else if (IsEqualGUID( &impl->guid, &GUID_SysKeyboard ))
        list_add_tail( &acquired_keyboard_list, &impl->entry );
    else
        list_add_tail( &acquired_device_list, &impl->entry );
    LeaveCriticalSection( &dinput_hook_crit );

    SendMessageW( di_em_win, WM_USER + 0x10, 1, 0 );
}

void dinput_hooks_unacquire_device( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    EnterCriticalSection( &dinput_hook_crit );
    list_remove( &impl->entry );
    LeaveCriticalSection( &dinput_hook_crit );

    SendMessageW( di_em_win, WM_USER + 0x10, 1, 0 );
}

static void dinput_device_internal_unacquire( IDirectInputDevice8W *iface, DWORD status )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_ACQUIRED)
    {
        impl->vtbl->unacquire( iface );
        impl->status = status;
        list_remove( &impl->entry );
    }
    LeaveCriticalSection( &impl->crit );
}

static LRESULT CALLBACK input_thread_ll_hook_proc( int code, WPARAM wparam, LPARAM lparam )
{
    struct dinput_device *impl;
    int skip = 0;

    if (code != HC_ACTION) return CallNextHookEx( 0, code, wparam, lparam );

    EnterCriticalSection( &dinput_hook_crit );
    LIST_FOR_EACH_ENTRY( impl, &acquired_mouse_list, struct dinput_device, entry )
    {
        TRACE( "calling dinput_mouse_hook (%p %Ix %Ix)\n", impl, wparam, lparam );
        skip |= dinput_mouse_hook( &impl->IDirectInputDevice8W_iface, wparam, lparam );
    }
    LIST_FOR_EACH_ENTRY( impl, &acquired_keyboard_list, struct dinput_device, entry )
    {
        if (impl->use_raw_input) continue;
        TRACE( "calling dinput_keyboard_hook (%p %Ix %Ix)\n", impl, wparam, lparam );
        skip |= dinput_keyboard_hook( &impl->IDirectInputDevice8W_iface, wparam, lparam );
    }
    LeaveCriticalSection( &dinput_hook_crit );

    return skip ? 1 : CallNextHookEx( 0, code, wparam, lparam );
}

static void dinput_unacquire_window_devices( HWND window )
{
    struct dinput_device *impl, *next;

    EnterCriticalSection( &dinput_hook_crit );

    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_device_list, struct dinput_device, entry )
    {
        if (!window || window == impl->win)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface, STATUS_UNACQUIRED );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_mouse_list, struct dinput_device, entry )
    {
        if (!window || window == impl->win)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface, STATUS_UNACQUIRED );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_rawmouse_list, struct dinput_device, entry )
    {
        if (!window || window == impl->win)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface, STATUS_UNACQUIRED );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_keyboard_list, struct dinput_device, entry )
    {
        if (!window || window == impl->win)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface, STATUS_UNACQUIRED );
        }
    }

    LeaveCriticalSection( &dinput_hook_crit );
}

static LRESULT CALLBACK callwndproc_proc( int code, WPARAM wparam, LPARAM lparam )
{
    CWPSTRUCT *msg = (CWPSTRUCT *)lparam;

    if (code != HC_ACTION || (msg->message != WM_KILLFOCUS &&
        msg->message != WM_ACTIVATEAPP && msg->message != WM_ACTIVATE))
        return CallNextHookEx( 0, code, wparam, lparam );

    if (msg->hwnd != GetForegroundWindow()) dinput_unacquire_window_devices( msg->hwnd );

    return CallNextHookEx( 0, code, wparam, lparam );
}

static void input_thread_update_device_list( struct input_thread_state *state )
{
    RAWINPUTDEVICE rawinput_mouse = {.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_MOUSE, .dwFlags = RIDEV_REMOVE};
    UINT count = 0, keyboard_count = 0, mouse_count = 0, foreground_count = 0;
    struct dinput_device *device;

    EnterCriticalSection( &dinput_hook_crit );
    LIST_FOR_EACH_ENTRY( device, &acquired_device_list, struct dinput_device, entry )
    {
        if (device->dwCoopLevel & DISCL_FOREGROUND) foreground_count++;
        if (!device->read_event || !device->vtbl->read) continue;
        state->events[count] = device->read_event;
        dinput_device_internal_addref( (state->devices[count] = device) );
        if (++count >= INPUT_THREAD_MAX_DEVICES) break;
    }
    state->events_count = count;

    LIST_FOR_EACH_ENTRY( device, &acquired_rawmouse_list, struct dinput_device, entry )
    {
        if (device->dwCoopLevel & DISCL_FOREGROUND) foreground_count++;
        if (device->dwCoopLevel & DISCL_BACKGROUND) rawinput_mouse.dwFlags |= RIDEV_INPUTSINK;
        if (device->dwCoopLevel & DISCL_EXCLUSIVE) rawinput_mouse.dwFlags |= RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE;
        rawinput_mouse.dwFlags &= ~RIDEV_REMOVE;
        rawinput_mouse.hwndTarget = di_em_win;
        dinput_device_internal_addref( (state->devices[count] = device) );
        if (++count >= INPUT_THREAD_MAX_DEVICES) break;
    }
    LIST_FOR_EACH_ENTRY( device, &acquired_mouse_list, struct dinput_device, entry )
    {
        if (device->dwCoopLevel & DISCL_FOREGROUND) foreground_count++;
        mouse_count++;
    }
    LIST_FOR_EACH_ENTRY( device, &acquired_keyboard_list, struct dinput_device, entry )
    {
        if (device->dwCoopLevel & DISCL_FOREGROUND) foreground_count++;
        keyboard_count++;
    }
    state->devices_count = count;
    LeaveCriticalSection( &dinput_hook_crit );

    if (foreground_count && !state->callwndproc_hook)
        state->callwndproc_hook = SetWindowsHookExW( WH_CALLWNDPROC, callwndproc_proc, DINPUT_instance, GetCurrentThreadId() );
    else if (!foreground_count && state->callwndproc_hook)
    {
        UnhookWindowsHookEx( state->callwndproc_hook );
        state->callwndproc_hook = NULL;
    }

    if (keyboard_count && !state->keyboard_ll_hook)
        state->keyboard_ll_hook = SetWindowsHookExW( WH_KEYBOARD_LL, input_thread_ll_hook_proc, DINPUT_instance, 0 );
    else if (!keyboard_count && state->keyboard_ll_hook)
    {
        UnhookWindowsHookEx( state->keyboard_ll_hook );
        state->keyboard_ll_hook = NULL;
    }

    if (mouse_count && !state->mouse_ll_hook)
        state->mouse_ll_hook = SetWindowsHookExW( WH_MOUSE_LL, input_thread_ll_hook_proc, DINPUT_instance, 0 );
    else if (!mouse_count && state->mouse_ll_hook)
    {
        UnhookWindowsHookEx( state->mouse_ll_hook );
        state->mouse_ll_hook = NULL;
    }

    if (!rawinput_mouse.hwndTarget != !state->rawinput_devices[0].hwndTarget &&
        !RegisterRawInputDevices( &rawinput_mouse, 1, sizeof(RAWINPUTDEVICE) ))
        WARN( "Failed to (un)register rawinput mouse device.\n" );

    state->rawinput_devices[0] = rawinput_mouse;
}

static LRESULT WINAPI di_em_win_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct input_thread_state *state = input_thread_state;
    int rim = GET_RAWINPUT_CODE_WPARAM( wparam );
    UINT i, size = sizeof(RAWINPUT);
    RAWINPUT ri;

    TRACE( "%p %d %Ix %Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_INPUT && (rim == RIM_INPUT || rim == RIM_INPUTSINK))
    {
        size = GetRawInputData( (HRAWINPUT)lparam, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER) );
        if (size == (UINT)-1 || size < sizeof(RAWINPUTHEADER))
            WARN( "Unable to read raw input data\n" );
        else if (ri.header.dwType == RIM_TYPEMOUSE)
        {
            for (i = state->events_count; i < state->devices_count; ++i)
            {
                struct dinput_device *device = state->devices[i];
                if (!device->use_raw_input) continue;
                if (device->instance.dwDevType & DIDEVTYPE_HID) continue;
                switch (GET_DIDEVICE_TYPE( device->instance.dwDevType ))
                {
                case DIDEVTYPE_MOUSE:
                case DI8DEVTYPE_MOUSE:
                    dinput_mouse_rawinput_hook( &device->IDirectInputDevice8W_iface, wparam, lparam, &ri );
                    break;
                default: break;
                }
            }
        }
    }

    if (msg == WM_USER + 0x10)
    {
        TRACE( "Processing hook change notification wparam %#Ix, lparam %#Ix.\n", wparam, lparam );

        if (!wparam) state->running = FALSE;
        else
        {
            while (state->devices_count--) dinput_device_internal_release( state->devices[state->devices_count] );
            input_thread_update_device_list( state );
        }

        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void register_di_em_win_class(void)
{
    WNDCLASSEXW class;

    memset(&class, 0, sizeof(class));
    class.cbSize = sizeof(class);
    class.lpfnWndProc = di_em_win_wndproc;
    class.hInstance = DINPUT_instance;
    class.lpszClassName = L"DIEmWin";

    if (!RegisterClassExW( &class ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        WARN( "Unable to register message window class\n" );
}

static void unregister_di_em_win_class(void)
{
    if (!UnregisterClassW( L"DIEmWin", NULL ) && GetLastError() != ERROR_CLASS_DOES_NOT_EXIST)
        WARN( "Unable to unregister message window class\n" );
}

static DWORD WINAPI dinput_thread_proc( void *params )
{
    struct input_thread_state state = {.running = TRUE};
    struct dinput_device *device;
    HANDLE start_event = params;
    DWORD ret;
    MSG msg;

    di_em_win = CreateWindowW( L"DIEmWin", L"DIEmWin", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, DINPUT_instance, NULL );
    input_thread_state = &state;
    SetEvent( start_event );

    while (state.running && (ret = MsgWaitForMultipleObjectsEx( state.events_count, state.events, INFINITE, QS_ALLINPUT, 0 )) <= state.events_count)
    {
        if (ret < state.events_count)
        {
            if ((device = state.devices[ret]) && FAILED( device->vtbl->read( &device->IDirectInputDevice8W_iface ) ))
            {
                EnterCriticalSection( &dinput_hook_crit );
                dinput_device_internal_unacquire( &device->IDirectInputDevice8W_iface, STATUS_UNPLUGGED );
                LeaveCriticalSection( &dinput_hook_crit );

                state.events[ret] = state.events[--state.events_count];
                state.devices[ret] = state.devices[state.events_count];
                state.devices[state.events_count] = state.devices[--state.devices_count];
                dinput_device_internal_release( device );
            }
        }

        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (state.running)
    {
        ERR( "Unexpected termination, ret %#lx\n", ret );
        dinput_unacquire_window_devices( 0 );
    }

    while (state.devices_count--) dinput_device_internal_release( state.devices[state.devices_count] );
    if (state.callwndproc_hook) UnhookWindowsHookEx( state.callwndproc_hook );
    if (state.keyboard_ll_hook) UnhookWindowsHookEx( state.keyboard_ll_hook );
    if (state.mouse_ll_hook) UnhookWindowsHookEx( state.mouse_ll_hook );
    DestroyWindow( di_em_win );
    di_em_win = NULL;
    return 0;
}

void input_thread_add_user(void)
{
    EnterCriticalSection( &dinput_hook_crit );
    if (!input_thread_user_count++)
    {
        HANDLE start_event;

        TRACE( "Starting input thread.\n" );

        if (!(start_event = CreateEventW( NULL, FALSE, FALSE, NULL )))
            ERR( "Failed to create start event, error %lu\n", GetLastError() );
        else if (!(dinput_thread = CreateThread( NULL, 0, dinput_thread_proc, start_event, 0, NULL )))
            ERR( "Failed to create internal thread, error %lu\n", GetLastError() );
        else
            WaitForSingleObject( start_event, INFINITE );

        CloseHandle( start_event );
    }
    LeaveCriticalSection( &dinput_hook_crit );
}

void input_thread_remove_user(void)
{
    EnterCriticalSection( &dinput_hook_crit );
    if (!--input_thread_user_count)
    {
        TRACE( "Stopping input thread.\n" );

        SendMessageW( di_em_win, WM_USER + 0x10, 0, 0 );
        WaitForSingleObject( dinput_thread, INFINITE );
        CloseHandle( dinput_thread );
    }
    LeaveCriticalSection( &dinput_hook_crit );
}

void check_dinput_events(void)
{
    /* Windows does not do that, but our current implementation of winex11
     * requires periodic event polling to forward events to the wineserver.
     *
     * We have to call this function from multiple places, because:
     * - some games do not explicitly poll for mouse events
     *   (for example Culpa Innata)
     * - some games only poll the device, and neither keyboard nor mouse
     *   (for example Civilization: Call to Power 2)
     * - some games do not explicitly poll for keyboard events
     *   (for example Morrowind in its key binding page)
     */
    MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, 0);
}

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, void *reserved )
{
    TRACE( "inst %p, reason %lu, reserved %p.\n", inst, reason, reserved );

    switch(reason)
    {
      case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        DINPUT_instance = inst;
        register_di_em_win_class();
        break;
      case DLL_PROCESS_DETACH:
        if (reserved) break;
        unregister_di_em_win_class();
        break;
    }
    return TRUE;
}
