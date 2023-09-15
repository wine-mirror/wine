/*
 * Unix library interface
 *
 * Copyright 2021 Zebediah Figura for CodeWeavers
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

#include "wine/unixlib.h"

struct server_getinfo_params
{
    const WCHAR *server;
    DWORD level;
    void *buffer;
    ULONG *size;
};

struct share_add_params
{
    const WCHAR *server;
    DWORD level;
    const BYTE *info;
    DWORD *err;
};

struct share_del_params
{
    const WCHAR *server;
    const WCHAR *share;
    DWORD reserved;
};

struct wksta_getinfo_params
{
    const WCHAR *server;
    DWORD level;
    void *buffer;
    ULONG *size;
};

struct change_password_params
{
    const WCHAR *domain;
    const WCHAR *user;
    const WCHAR *old;
    const WCHAR *new;
};

enum samba_funcs
{
    unix_netapi_init,
    unix_server_getinfo,
    unix_share_add,
    unix_share_del,
    unix_wksta_getinfo,
    unix_change_password,
    unix_funcs_count
};
