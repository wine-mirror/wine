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
    unix_dnd_get_data,
    unix_dnd_get_formats,
    unix_dnd_have_format,
    unix_dnd_release,
    unix_dnd_retain,
    unix_ime_clear,
    unix_ime_process_text_input,
    unix_ime_using_input_method,
    unix_init,
    unix_notify_icon,
    unix_quit_result,
    unix_funcs_count
};

extern unixlib_handle_t macdrv_handle DECLSPEC_HIDDEN;
#define MACDRV_CALL(func, params) __wine_unix_call(macdrv_handle, unix_ ## func, params)

/* macdrv_dnd_get_data params */
struct dnd_get_data_params
{
    UINT64 handle;
    UINT format;
    size_t size;
    void *data;
};

/* macdrv_dnd_get_formats params */
struct dnd_get_formats_params
{
    UINT64 handle;
    UINT formats[64];
};

/* macdrv_dnd_have_format params */
struct dnd_have_format_params
{
    UINT64 handle;
    UINT format;
};

/* macdrv_ime_process_text_input params */
struct process_text_input_params
{
    UINT vkey;
    UINT scan;
    UINT repeat;
    const BYTE *key_state;
    void *himc;
    int *done;
};

/* macdrv_init params */
struct localized_string
{
    UINT id;
    UINT len;
    const WCHAR *str;
};

struct init_params
{
    struct localized_string *strings;
};

/* macdrv_notify_icon params */
struct notify_icon_params
{
    DWORD msg;
    struct _NOTIFYICONDATAW *data;
};

/* macdrv_quit_result params */
struct quit_result_params
{
    int result;
};

/* driver client callbacks exposed with KernelCallbackTable interface */
enum macdrv_client_funcs
{
    client_func_app_icon = NtUserDriverCallbackFirst,
    client_func_app_quit_request,
    client_func_dnd_query_drag,
    client_func_dnd_query_drop,
    client_func_dnd_query_exited,
    client_func_ime_query_char_rect,
    client_func_ime_set_text,
    client_func_last
};

/* macdrv_app_icon result */
struct app_icon_entry
{
    UINT32 width;
    UINT32 height;
    UINT32 size;
    void *png;
    HICON icon;
};

struct app_icon_result
{
    UINT32 count;
    struct app_icon_entry entries[64];
};

/* macdrv_app_icon params */
struct app_icon_params
{
    struct app_icon_result *result; /* FIXME: Use NtCallbackReturn instead */
};

/* macdrv_app_quit_request params */
struct app_quit_request_params
{
    UINT flags;
};

/* macdrv_dnd_query_drag params */
struct dnd_query_drag_params
{
    HWND hwnd;
    UINT32 effect;
    INT32 x;
    INT32 y;
    UINT64 handle;
};

/* macdrv_dnd_query_drop params */
struct dnd_query_drop_params
{
    HWND hwnd;
    UINT32 effect;
    INT32 x;
    INT32 y;
    UINT64 handle;
};

/* macdrv_dnd_query_exited params */
struct dnd_query_exited_params
{
    HWND hwnd;
};

/* macdrv_ime_query_char_rect result */
struct ime_query_char_rect_result
{
    RECT rect;
    UINT32 location;
    UINT32 length;
};

/* macdrv_ime_query_char_rect params */
struct ime_query_char_rect_params
{
    HWND hwnd;
    void *data;
    UINT32 location;
    UINT32 length;
    struct ime_query_char_rect_result *result; /* FIXME: Use NtCallbackReturn instead */
};

/* macdrv_ime_set_text params */
struct ime_set_text_params
{
    HWND hwnd;
    void *data;
    UINT32 cursor_pos;
    UINT32 complete;
    WCHAR text[1];
};

C_ASSERT(client_func_last <= NtUserDriverCallbackLast + 1);
