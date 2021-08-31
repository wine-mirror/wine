/*
 * WAYLANDDRV initialization code
 *
 * Copyright 2020 Alexandre Frantzis for Collabora Ltd
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "waylanddrv.h"

static NTSTATUS waylanddrv_unix_init(void *arg)
{
    if (!wayland_process_init()) return STATUS_UNSUCCESSFUL;

    return 0;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    waylanddrv_unix_init,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == waylanddrv_unix_func_count);

#ifdef _WIN64

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    waylanddrv_unix_init,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_wow64_funcs) == waylanddrv_unix_func_count);

#endif /* _WIN64 */
