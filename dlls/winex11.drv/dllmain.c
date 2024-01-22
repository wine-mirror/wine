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


static const KERNEL_CALLBACK_PROC kernel_callbacks[] =
{
    x11drv_dnd_enter_event,
    x11drv_dnd_position_event,
    x11drv_dnd_post_drop,
    x11drv_dnd_drop_event,
    x11drv_dnd_leave_event,
};

C_ASSERT( NtUserDriverCallbackFirst + ARRAYSIZE(kernel_callbacks) == client_func_last );


BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    KERNEL_CALLBACK_PROC *callback_table;
    struct init_params params =
    {
        foreign_window_proc,
    };

    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls( instance );
    x11drv_module = instance;
    if (__wine_init_unix_call()) return FALSE;
    if (X11DRV_CALL( init, &params )) return FALSE;

    callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
    memcpy( callback_table + NtUserDriverCallbackFirst, kernel_callbacks, sizeof(kernel_callbacks) );
    return TRUE;
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
