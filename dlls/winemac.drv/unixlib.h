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
    unix_ime_process_text_input,
    unix_ime_using_input_method,
    unix_init,
    unix_notify_icon,
    unix_funcs_count
};

/* FIXME: Use __wine_unix_call when the rest of the stack is ready */
extern NTSTATUS unix_call(enum macdrv_funcs code, void *params) DECLSPEC_HIDDEN;
#define MACDRV_CALL(func, params) unix_call( unix_ ## func, params )

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

/* driver client callbacks exposed with KernelCallbackTable interface */
enum macdrv_client_funcs
{
    client_func_ime_query_char_rect = NtUserDriverCallbackFirst,
    client_func_ime_set_text,
    client_func_last
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
