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
    unix_funcs_count
};

/* FIXME: Use __wine_unix_call when the rest of the stack is ready */
extern NTSTATUS unix_call(enum macdrv_funcs code, void *params) DECLSPEC_HIDDEN;
#define MACDRV_CALL(func, params) unix_call( unix_ ## func, params )

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
