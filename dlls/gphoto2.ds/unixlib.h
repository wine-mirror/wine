/*
 * Unix library interface for gphoto
 *
 * Copyright 2000 Corel Corporation
 * Copyright 2006 Marcus Meissner
 * Copyright 2021 Alexandre Julliard
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

#include <stdarg.h>
#include "windef.h"
#include "winternl.h"
#include "wine/unixlib.h"
#include "twain.h"

struct load_file_list_params
{
    const char   *root;
    unsigned int *count;
};

struct get_file_name_params
{
    unsigned int  idx;
    unsigned int  size;
    char         *buffer;
};

struct open_file_params
{
    unsigned int  idx;
    BOOL          preview;
    UINT64       *handle;
    unsigned int *size;
};

struct get_file_data_params
{
    UINT64       handle;
    void        *data;
    unsigned int size;
};

struct close_file_params
{
    UINT64       handle;
};

enum gphoto2_funcs
{
    unix_get_identity,
    unix_open_ds,
    unix_close_ds,
    unix_load_file_list,
    unix_get_file_name,
    unix_open_file,
    unix_get_file_data,
    unix_close_file,
};

#define GPHOTO2_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )
