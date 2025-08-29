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

#define INPUT_THREAD_NOTIFY     (WM_USER + 0x10)
#define NOTIFY_THREAD_STOP      0
#define NOTIFY_REFRESH_DEVICES  1

struct input_thread_state
{
    BOOL running;
    UINT events_count;
    UINT devices_count;
    UINT devices_capacity;
    HHOOK mouse_ll_hook;
    HHOOK keyboard_ll_hook;
    RAWINPUTDEVICE rawinput_devices[2];
    struct dinput_device **devices;
    HANDLE *events;
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

static struct list acquired_device_list = LIST_INIT( acquired_device_list );

static void unhook_device_window_foreground_changes( struct dinput_device *device )
{
    if (!device->cbt_hook) return;
    UnhookWindowsHookEx( device->cbt_hook );
    device->cbt_hook = NULL;
}

static void dinput_device_internal_unacquire( IDirectInputDevice8W *iface, DWORD status )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p.\n", iface );

    unhook_device_window_foreground_changes( impl );

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
    struct input_thread_state *state = input_thread_state;
    int i, skip = 0;

    if (code != HC_ACTION) return CallNextHookEx( 0, code, wparam, lparam );

    for (i = state->events_count; i < state->devices_count; ++i)
    {
        struct dinput_device *device = state->devices[i];
        if (device->use_raw_input) continue;
        if (device->instance.dwDevType & DIDEVTYPE_HID) continue;
        switch (GET_DIDEVICE_TYPE( device->instance.dwDevType ))
        {
        case DIDEVTYPE_MOUSE:
        case DI8DEVTYPE_MOUSE:
            TRACE( "calling dinput_mouse_hook (%p %Ix %Ix)\n", device, wparam, lparam );
            skip |= dinput_mouse_hook( &device->IDirectInputDevice8W_iface, wparam, lparam );
            break;
        case DIDEVTYPE_KEYBOARD:
        case DI8DEVTYPE_KEYBOARD:
            TRACE( "calling dinput_keyboard_hook (%p %Ix %Ix)\n", device, wparam, lparam );
            skip |= dinput_keyboard_hook( &device->IDirectInputDevice8W_iface, wparam, lparam );
            break;
        }
    }

    return skip ? 1 : CallNextHookEx( 0, code, wparam, lparam );
}

static void handle_foreground_lost( HWND window )
{
    struct dinput_device *impl, *next;

    EnterCriticalSection( &dinput_hook_crit );

    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_device_list, struct dinput_device, entry )
    {
        if (!(impl->dwCoopLevel & DISCL_FOREGROUND) || window != impl->win) continue;
        TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
        dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface, STATUS_UNACQUIRED );
    }

    LeaveCriticalSection( &dinput_hook_crit );
}

static void dinput_unacquire_devices(void)
{
    struct dinput_device *impl, *next;

    EnterCriticalSection( &dinput_hook_crit );

    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_device_list, struct dinput_device, entry )
        dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface, STATUS_UNACQUIRED );

    LeaveCriticalSection( &dinput_hook_crit );
}

static LRESULT CALLBACK cbt_hook_proc( int code, WPARAM wparam, LPARAM lparam )
{
    if (code == HCBT_ACTIVATE && di_em_win)
    {
        CBTACTIVATESTRUCT *data = (CBTACTIVATESTRUCT *)lparam;
        handle_foreground_lost( data->hWndActive );
        SendMessageW( di_em_win, INPUT_THREAD_NOTIFY, NOTIFY_REFRESH_DEVICES, 0 );
    }

    return CallNextHookEx( 0, code, wparam, lparam );
}

static void hook_device_window_foreground_changes( struct dinput_device *device )
{
    DWORD tid, pid;
    if (!(tid = GetWindowThreadProcessId( device->win, &pid ))) return;
    device->cbt_hook = SetWindowsHookExW( WH_CBT, cbt_hook_proc, DINPUT_instance, tid );
}

static void input_thread_update_device_list( struct input_thread_state *state )
{
    RAWINPUTDEVICE rawinput_keyboard = {.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_KEYBOARD, .dwFlags = RIDEV_REMOVE};
    RAWINPUTDEVICE rawinput_mouse = {.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_MOUSE, .dwFlags = RIDEV_REMOVE};
    UINT count = 0, keyboard_ll_count = 0, mouse_ll_count = 0, capacity;
    struct dinput_device *device;

    EnterCriticalSection( &dinput_hook_crit );

    if ((capacity = list_count( &acquired_device_list )) && capacity > state->devices_capacity)
    {
        struct dinput_device **devices;
        HANDLE *events;

        if (!state->devices_capacity) capacity = max( capacity, 128 );
        else capacity = max( capacity, state->devices_capacity * 3 / 2 );

        if ((events = realloc( state->events, capacity * sizeof(*events) ))) state->events = events;
        if ((devices = realloc( state->devices, capacity * sizeof(*devices) ))) state->devices = devices;
        if (events && devices) state->devices_capacity = capacity;
    }

    LIST_FOR_EACH_ENTRY( device, &acquired_device_list, struct dinput_device, entry )
    {
        unhook_device_window_foreground_changes( device );
        if (device->dwCoopLevel & DISCL_FOREGROUND) hook_device_window_foreground_changes( device );

        if (!device->read_event || !device->vtbl->read) continue;
        state->events[count] = device->read_event;
        dinput_device_internal_addref( (state->devices[count] = device) );
        if (++count >= state->devices_capacity) break;
    }
    state->events_count = count;

    LIST_FOR_EACH_ENTRY( device, &acquired_device_list, struct dinput_device, entry )
    {
        RAWINPUTDEVICE *rawinput_device = NULL;

        if (device->read_event && device->vtbl->read) continue;
        switch (GET_DIDEVICE_TYPE( device->instance.dwDevType ))
        {
        case DIDEVTYPE_MOUSE:
        case DI8DEVTYPE_MOUSE:
            if (device->dwCoopLevel & DISCL_EXCLUSIVE) rawinput_mouse.dwFlags |= RIDEV_CAPTUREMOUSE;
            if (!device->use_raw_input) mouse_ll_count++;
            else rawinput_device = &rawinput_mouse;
            break;
        case DIDEVTYPE_KEYBOARD:
        case DI8DEVTYPE_KEYBOARD:
            if (device->dwCoopLevel & DISCL_EXCLUSIVE) rawinput_keyboard.dwFlags |= RIDEV_NOHOTKEYS;
            if (!device->use_raw_input) keyboard_ll_count++;
            else rawinput_device = &rawinput_keyboard;
            break;
        }

        if (rawinput_device)
        {
            if (device->dwCoopLevel & DISCL_BACKGROUND) rawinput_device->dwFlags |= RIDEV_INPUTSINK;
            if (device->dwCoopLevel & DISCL_EXCLUSIVE) rawinput_device->dwFlags |= RIDEV_NOLEGACY;
            rawinput_device->dwFlags &= ~RIDEV_REMOVE;
            rawinput_device->hwndTarget = di_em_win;
        }

        dinput_device_internal_addref( (state->devices[count] = device) );
        if (++count >= state->devices_capacity) break;
    }
    state->devices_count = count;
    LeaveCriticalSection( &dinput_hook_crit );

    if (keyboard_ll_count && !state->keyboard_ll_hook)
        state->keyboard_ll_hook = SetWindowsHookExW( WH_KEYBOARD_LL, input_thread_ll_hook_proc, DINPUT_instance, 0 );
    else if (!keyboard_ll_count && state->keyboard_ll_hook)
    {
        UnhookWindowsHookEx( state->keyboard_ll_hook );
        state->keyboard_ll_hook = NULL;
    }

    if (mouse_ll_count && !state->mouse_ll_hook)
        state->mouse_ll_hook = SetWindowsHookExW( WH_MOUSE_LL, input_thread_ll_hook_proc, DINPUT_instance, 0 );
    else if (!mouse_ll_count && state->mouse_ll_hook)
    {
        UnhookWindowsHookEx( state->mouse_ll_hook );
        state->mouse_ll_hook = NULL;
    }

    if (!rawinput_mouse.hwndTarget != !state->rawinput_devices[0].hwndTarget &&
        !RegisterRawInputDevices( &rawinput_mouse, 1, sizeof(RAWINPUTDEVICE) ))
        WARN( "Failed to (un)register rawinput mouse device.\n" );
    if (!rawinput_keyboard.hwndTarget != !state->rawinput_devices[1].hwndTarget &&
        !RegisterRawInputDevices( &rawinput_keyboard, 1, sizeof(RAWINPUTDEVICE) ))
        WARN( "Failed to (un)register rawinput mouse device.\n" );

    state->rawinput_devices[0] = rawinput_mouse;
    state->rawinput_devices[1] = rawinput_keyboard;
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
        else if (ri.header.dwType == RIM_TYPEHID)
            WARN( "Unexpected HID rawinput message\n" );
        else
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
                    if (ri.header.dwType != RIM_TYPEMOUSE) break;
                    dinput_mouse_rawinput_hook( &device->IDirectInputDevice8W_iface, wparam, lparam, &ri );
                    break;
                case DIDEVTYPE_KEYBOARD:
                case DI8DEVTYPE_KEYBOARD:
                    if (ri.header.dwType != RIM_TYPEKEYBOARD) break;
                    dinput_keyboard_rawinput_hook( &device->IDirectInputDevice8W_iface, wparam, lparam, &ri );
                    break;
                default: break;
                }
            }
        }
    }

    if (msg == INPUT_THREAD_NOTIFY)
    {
        TRACE( "Processing hook change notification wparam %#Ix, lparam %#Ix.\n", wparam, lparam );

        switch (wparam)
        {
        case NOTIFY_THREAD_STOP:
            state->running = FALSE;
            break;
        case NOTIFY_REFRESH_DEVICES:
            while (state->devices_count--) dinput_device_internal_release( state->devices[state->devices_count] );
            input_thread_update_device_list( state );
            break;
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
    HMODULE this_module = NULL;
    DWORD ret;
    MSG msg;

    SetThreadDescription( GetCurrentThread(), L"wine_dinput_worker" );
    SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );

    di_em_win = CreateWindowW( L"DIEmWin", L"DIEmWin", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, DINPUT_instance, NULL );
    input_thread_state = &state;

    if (!GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (void *)dinput_thread_proc, &this_module ))
        ERR( "Failed to get a handle of dinput to keep it alive: %lu", GetLastError() );

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
        dinput_unacquire_devices();
    }

    while (state.devices_count--) dinput_device_internal_release( state.devices[state.devices_count] );
    if (state.keyboard_ll_hook) UnhookWindowsHookEx( state.keyboard_ll_hook );
    if (state.mouse_ll_hook) UnhookWindowsHookEx( state.mouse_ll_hook );
    free( state.devices );
    free( state.events );

    DestroyWindow( di_em_win );
    di_em_win = NULL;

    if (this_module != NULL) FreeLibraryAndExitThread( this_module, 0 );
    return 0;
}

void input_thread_start(void)
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

void dinput_hooks_acquire_device( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    EnterCriticalSection( &dinput_hook_crit );
    /* start the input thread now if it wasn't started already */
    if (!dinput_thread) input_thread_start();
    list_add_tail( &acquired_device_list, &impl->entry );
    LeaveCriticalSection( &dinput_hook_crit );

    SendMessageW( di_em_win, INPUT_THREAD_NOTIFY, NOTIFY_REFRESH_DEVICES, 0 );
}

void dinput_hooks_unacquire_device( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    EnterCriticalSection( &dinput_hook_crit );
    list_remove( &impl->entry );
    LeaveCriticalSection( &dinput_hook_crit );

    SendMessageW( di_em_win, INPUT_THREAD_NOTIFY, NOTIFY_REFRESH_DEVICES, 0 );
}

void input_thread_add_user(void)
{
    /* we cannot start the input thread here because some games create dinput objects from their DllMain, and
     * starting the thread will wait for it to initialize, which requires the loader lock to be released.
     */
    EnterCriticalSection( &dinput_hook_crit );
    input_thread_user_count++;
    LeaveCriticalSection( &dinput_hook_crit );
}

void input_thread_remove_user(void)
{
    EnterCriticalSection( &dinput_hook_crit );
    if (!--input_thread_user_count && dinput_thread)
    {
        TRACE( "Stopping input thread.\n" );

        SendMessageW( di_em_win, INPUT_THREAD_NOTIFY, NOTIFY_THREAD_STOP, 0 );
        WaitForSingleObject( dinput_thread, INFINITE );
        CloseHandle( dinput_thread );
        dinput_thread = NULL;
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
