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
    unix_create_desktop,
    unix_init,
    unix_systray_clear,
    unix_systray_dock,
    unix_systray_hide,
    unix_systray_init,
    unix_tablet_attach_queue,
    unix_tablet_get_packet,
    unix_tablet_info,
    unix_tablet_load_info,
    unix_xim_preedit_state,
    unix_xim_reset,
    unix_funcs_count,
};

/* FIXME: Use __wine_unix_call when the rest of the stack is ready */
extern NTSTATUS (CDECL *x11drv_unix_call)( enum x11drv_funcs code, void *params ) DECLSPEC_HIDDEN;
#define X11DRV_CALL(func, params) x11drv_unix_call( unix_ ## func, params )

/* x11drv_create_desktop params */
struct create_desktop_params
{
    UINT width;
    UINT height;
};

/* x11drv_init params */
struct init_params
{
    NTSTATUS (WINAPI *pNtWaitForMultipleObjects)(ULONG,const HANDLE*,BOOLEAN,BOOLEAN,const LARGE_INTEGER*);
    WNDPROC foreign_window_proc;
    BOOL show_systray;
    NTSTATUS (CDECL *unix_call)( enum x11drv_funcs code, void *params );
};

struct systray_dock_params
{
    UINT64 event_handle;
    void *icon;
    int cx;
    int cy;
    BOOL *layered;
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
    client_func_callback = NtUserDriverCallbackFirst,
    client_func_dnd_enter_event,
    client_func_dnd_position_event,
    client_func_dnd_post_drop,
    client_func_ime_set_composition_string,
    client_func_ime_set_result,
    client_func_systray_change_owner,
    client_func_last
};

C_ASSERT( client_func_last <= NtUserDriverCallbackLast + 1 );

/* simplified interface for client callbacks requiring only a single UINT parameter */
enum client_callback
{
    client_dnd_drop_event,
    client_dnd_leave_event,
    client_ime_get_cursor_pos,
    client_ime_set_composition_status,
    client_ime_set_cursor_pos,
    client_ime_set_open_status,
    client_ime_update_association,
    client_funcs_count
};

/* x11drv_callback params */
struct client_callback_params
{
    UINT id;
    UINT arg;
};

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
    HWND  hwnd;
    POINT point;
    DWORD effect;
};

struct systray_change_owner_params
{
    UINT64 event_handle;
};
