/*
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

#include "ntuser.h"
#include "wine/unixlib.h"

enum x11drv_funcs
{
    unix_init,
    unix_tablet_attach_queue,
    unix_tablet_get_packet,
    unix_tablet_info,
    unix_tablet_load_info,
    unix_funcs_count,
};

#define X11DRV_CALL(func, params) WINE_UNIX_CALL( unix_ ## func, params )

/* x11drv_init params */
struct init_params
{
    WNDPROC foreign_window_proc;
};

/* x11drv_tablet_info params */
struct tablet_info_params
{
    UINT category;
    UINT index;
    void *output;
};

/* x11drv_xim_preedit_state params */
struct xim_preedit_state_params
{
    HWND hwnd;
    BOOL open;
};

/* driver client callbacks exposed with KernelCallbackTable interface */
enum x11drv_client_funcs
{
    client_func_dnd_enter_event = NtUserDriverCallbackFirst,
    client_func_dnd_position_event,
    client_func_dnd_post_drop,
    client_func_dnd_drop_event,
    client_func_dnd_leave_event,
    client_func_last
};

C_ASSERT( client_func_last <= NtUserDriverCallbackLast + 1 );

/* x11drv_dnd_enter_event and x11drv_dnd_post_drop params */
struct format_entry
{
    UINT format;
    UINT size;
    char data[1];
};

/* x11drv_dnd_position_event params */
struct dnd_position_event_params
{
    ULONG hwnd;
    POINT point;
    DWORD effect;
};
