/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_WINEDMO_UNIXLIB_H
#define __WINE_WINEDMO_UNIXLIB_H

#include <stddef.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"

#include "wine/unixlib.h"
#include "wine/winedmo.h"

struct process_attach_params
{
    UINT64 seek_callback;
    UINT64 read_callback;
};

struct stream_context
{
    UINT64 placeholder;
};

struct seek_callback_params
{
    struct dispatch_callback_params dispatch;
    UINT64 context;
    INT64 offset;
};

struct read_callback_params
{
    struct dispatch_callback_params dispatch;
    UINT64 context;
    INT32 size;
};


struct demuxer_check_params
{
    char mime_type[256];
};

struct demuxer_create_params
{
    struct stream_context *context;
    struct winedmo_demuxer demuxer;
};

struct demuxer_destroy_params
{
    struct winedmo_demuxer demuxer;
    struct stream_context *context;
};


enum unix_funcs
{
    unix_process_attach,

    unix_demuxer_check,
    unix_demuxer_create,
    unix_demuxer_destroy,

    unix_funcs_count,
};

#define UNIX_CALL( func, params ) (__wine_unixlib_handle ? WINE_UNIX_CALL( unix_##func, params ) : STATUS_PROCEDURE_NOT_FOUND)

#endif /* __WINE_WINEDMO_UNIXLIB_H */
