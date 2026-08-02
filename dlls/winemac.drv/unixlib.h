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

enum macdrv_funcs
{
    unix_init,
    unix_quit_result,
    unix_funcs_count
};

#define MACDRV_CALL(func, params) WINE_UNIX_CALL(unix_ ## func, params)

/* macdrv_init params */
struct localized_string
{
    UINT id;
    UINT len;
    UINT64 str;
};

struct init_params
{
    struct localized_string *strings;
    UINT64 app_icon_callback;
    UINT64 app_quit_request_callback;
};

/* macdrv_quit_result params */
struct quit_result_params
{
    int result;
};

/* macdrv_app_icon result */
struct app_icon_entry
{
    UINT32 width;
    UINT32 height;
    UINT32 size;
    UINT32 icon;
    UINT64 png;
};

/* macdrv_app_quit_request params */
struct app_quit_request_params
{
    struct dispatch_callback_params dispatch;
    UINT flags;
};

/* macdrv_dnd_query_exited params */
struct dnd_query_exited_params
{
    struct dispatch_callback_params dispatch;
    UINT32 hwnd;
};

static inline void *param_ptr(UINT64 param)
{
    return (void *)(UINT_PTR)param;
}
