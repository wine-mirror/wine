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
#include "shlobj.h"
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
    UINT64 dnd_enter_event_callback;
    UINT64 dnd_position_event_callback;
    UINT64 dnd_post_drop_callback;
    UINT64 dnd_drop_event_callback;
    UINT64 dnd_leave_event_callback;
    UINT64 foreign_window_proc;
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

struct format_entry
{
    UINT format;
    UINT size;
    char data[1];
};

/* x11drv_dnd_enter_event params */
struct dnd_enter_event_params
{
    struct dispatch_callback_params dispatch;
    struct format_entry entries[];
};

C_ASSERT(sizeof(struct dnd_enter_event_params) == offsetof(struct dnd_enter_event_params, entries[0]));

/* x11drv_dnd_position_event params */
struct dnd_position_event_params
{
    struct dispatch_callback_params dispatch;
    ULONG hwnd;
    POINT point;
    DWORD effect;
};

/* x11drv_dnd_drop_event params */
struct dnd_drop_event_params
{
    struct dispatch_callback_params dispatch;
    ULONG hwnd;
};

/* x11drv_dnd_post_drop params */
struct dnd_post_drop_params
{
    struct dispatch_callback_params dispatch;
    UINT32 __pad;
    DROPFILES drop;
    char data[];
};

C_ASSERT(sizeof(struct dnd_post_drop_params) == offsetof(struct dnd_post_drop_params, data[0]));
