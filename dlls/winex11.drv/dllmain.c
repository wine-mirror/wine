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


HMODULE x11drv_module = 0;
static unixlib_handle_t x11drv_handle;
NTSTATUS (CDECL *x11drv_unix_call)( enum x11drv_funcs code, void *params );


typedef NTSTATUS (*callback_func)( UINT arg );
static const callback_func callback_funcs[] =
{
    x11drv_dnd_drop_event,
    x11drv_dnd_leave_event,
    x11drv_ime_get_cursor_pos,
    x11drv_ime_set_composition_status,
    x11drv_ime_set_cursor_pos,
    x11drv_ime_set_open_status,
    x11drv_ime_update_association,
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
