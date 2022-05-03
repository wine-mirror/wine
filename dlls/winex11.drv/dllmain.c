/*
 * winex11.drv entry points
 *
 * Copyright 2022 Jacek Caban for CodeWeavers
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

#include "x11drv_dll.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);


HMODULE x11drv_module = 0;
static unixlib_handle_t x11drv_handle;
NTSTATUS (CDECL *x11drv_unix_call)( enum x11drv_funcs code, void *params );

/**************************************************************************
 *		wait_clipboard_mutex
 *
 * Make sure that there's only one clipboard thread per window station.
 */
static BOOL wait_clipboard_mutex(void)
{
    static const WCHAR prefix[] = {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_'};
    WCHAR buffer[MAX_PATH + ARRAY_SIZE( prefix )];
    HANDLE mutex;

    memcpy( buffer, prefix, sizeof(prefix) );
    if (!GetUserObjectInformationW( GetProcessWindowStation(), UOI_NAME,
                                    buffer + ARRAY_SIZE( prefix ),
                                    sizeof(buffer) - sizeof(prefix), NULL ))
    {
        ERR( "failed to get winstation name\n" );
        return FALSE;
    }
    mutex = CreateMutexW( NULL, TRUE, buffer );
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        TRACE( "waiting for mutex %s\n", debugstr_w( buffer ));
        WaitForSingleObject( mutex, INFINITE );
    }
    return TRUE;
}


/**************************************************************************
 *		clipboard_wndproc
 *
 * Window procedure for the clipboard manager.
 */
static LRESULT CALLBACK clipboard_wndproc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    struct clipboard_message_params params;

    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CLIPBOARDUPDATE:
    case WM_RENDERFORMAT:
    case WM_TIMER:
    case WM_DESTROYCLIPBOARD:
        params.hwnd   = hwnd;
        params.msg    = msg;
        params.wparam = wp;
        params.lparam = lp;
        return X11DRV_CALL( clipboard_message, &params );
    }

    return DefWindowProcW( hwnd, msg, wp, lp );
}


/**************************************************************************
 *		clipboard_thread
 *
 * Thread running inside the desktop process to manage the clipboard
 */
static DWORD WINAPI clipboard_thread( void *arg )
{
    static const WCHAR clipboard_classname[] = {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_','m','a','n','a','g','e','r',0};
    WNDCLASSW class;
    MSG msg;

    if (!wait_clipboard_mutex()) return 0;

    memset( &class, 0, sizeof(class) );
    class.lpfnWndProc   = clipboard_wndproc;
    class.lpszClassName = clipboard_classname;

    if (!RegisterClassW( &class ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR( "could not register clipboard window class err %u\n", GetLastError() );
        return 0;
    }
    if (!CreateWindowW( clipboard_classname, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, NULL ))
    {
        ERR( "failed to create clipboard window err %u\n", GetLastError() );
        return 0;
    }

    while (GetMessageW( &msg, 0, 0, 0 )) DispatchMessageW( &msg );
    return 0;
}


static NTSTATUS x11drv_clipboard_init( UINT arg )
{
    DWORD id;
    HANDLE thread = CreateThread( NULL, 0, clipboard_thread, NULL, 0, &id );

    if (thread) CloseHandle( thread );
    else ERR( "failed to create clipboard thread\n" );
    return 0;
}


static NTSTATUS WINAPI x11drv_is_system_module( void *arg, ULONG size )
{
    HMODULE module;
    unsigned int i;

    static const WCHAR cursor_modules[][16] =
    {
        { 'u','s','e','r','3','2','.','d','l','l',0 },
        { 'c','o','m','c','t','l','3','2','.','d','l','l',0 },
        { 'o','l','e','3','2','.','d','l','l',0 },
        { 'r','i','c','h','e','d','2','0','.','d','l','l',0 }
    };

    if (!(module = GetModuleHandleW( arg ))) return system_module_none;

    for (i = 0; i < ARRAYSIZE(cursor_modules); i++)
    {
        if (GetModuleHandleW( cursor_modules[i] ) == module) return i;
    }

    return system_module_none;
}


static NTSTATUS x11drv_load_icon( UINT id )
{
    return HandleToUlong( LoadIconW( NULL, UlongToPtr( id )));
}


typedef NTSTATUS (*callback_func)( UINT arg );
static const callback_func callback_funcs[] =
{
    x11drv_clipboard_init,
    x11drv_dnd_drop_event,
    x11drv_dnd_leave_event,
    x11drv_ime_get_cursor_pos,
    x11drv_ime_set_composition_status,
    x11drv_ime_set_cursor_pos,
    x11drv_ime_set_open_status,
    x11drv_ime_update_association,
    x11drv_load_icon,
};

C_ASSERT( ARRAYSIZE(callback_funcs) == client_funcs_count );

static NTSTATUS WINAPI x11drv_callback( void *arg, ULONG size )
{
    struct client_callback_params *params = arg;
    return callback_funcs[params->id]( params->arg );
}

typedef NTSTATUS (WINAPI *kernel_callback)( void *params, ULONG size );
static const kernel_callback kernel_callbacks[] =
{
    x11drv_callback,
    x11drv_dnd_enter_event,
    x11drv_dnd_position_event,
    x11drv_dnd_post_drop,
    x11drv_ime_set_composition_string,
    x11drv_ime_set_result,
    x11drv_is_system_module,
    x11drv_systray_change_owner,
};

C_ASSERT( NtUserDriverCallbackFirst + ARRAYSIZE(kernel_callbacks) == client_func_last );


BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    void **callback_table;
    struct init_params params =
    {
        NtWaitForMultipleObjects,
        foreign_window_proc,
    };

    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls( instance );
    x11drv_module = instance;
    if (NtQueryVirtualMemory( GetCurrentProcess(), instance, MemoryWineUnixFuncs,
                              &x11drv_handle, sizeof(x11drv_handle), NULL ))
        return FALSE;

    if (__wine_unix_call( x11drv_handle, unix_init, &params )) return FALSE;

    callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
    memcpy( callback_table + NtUserDriverCallbackFirst, kernel_callbacks, sizeof(kernel_callbacks) );

    show_systray = params.show_systray;
    x11drv_unix_call = params.unix_call;
    return TRUE;
}


/***********************************************************************
 *           wine_create_desktop (winex11.@)
 */
BOOL CDECL wine_create_desktop( UINT width, UINT height )
{
    struct create_desktop_params params = { .width = width, .height = height };
    return X11DRV_CALL( create_desktop, &params );
}

/***********************************************************************
 *           AttachEventQueueToTablet (winex11.@)
 */
int CDECL X11DRV_AttachEventQueueToTablet( HWND owner )
{
    return X11DRV_CALL( tablet_attach_queue, owner );
}

/***********************************************************************
 *           GetCurrentPacket (winex11.@)
 */
int CDECL X11DRV_GetCurrentPacket( void *packet )
{
    return X11DRV_CALL( tablet_get_packet, packet );
}

/***********************************************************************
 *           LoadTabletInfo (winex11.@)
 */
BOOL CDECL X11DRV_LoadTabletInfo( HWND hwnd )
{
    return X11DRV_CALL( tablet_load_info, hwnd );
}

/***********************************************************************
 *          WTInfoW (winex11.@)
 */
UINT CDECL X11DRV_WTInfoW( UINT category, UINT index, void *output )
{
    struct tablet_info_params params;
    params.category = category;
    params.index = index;
    params.output = output;
    return X11DRV_CALL( tablet_info, &params );
}
