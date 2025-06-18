/*
 * WINE ct-api wrapper
 *
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
#include "winbase.h"
#include "winternl.h"
#include "wine/unixlib.h"

typedef unsigned char IU8;
typedef unsigned short IU16;

typedef signed char IS8;
typedef signed short IS16;

struct ct_init_params
{
    IU16 ctn;
    IU16 pn;
};

struct ct_data_params
{
    IU16  ctn;
    IU8  *dad;
    IU8  *sad;
    IU16  lenc;
    IU8  *command;
    IU16 *lenr;
    IU8  *response;
};

struct ct_close_params
{
    IU16 ctn;
};

enum ctapi_funcs
{
    unix_attach,
    unix_detach,
    unix_ct_init,
    unix_ct_data,
    unix_ct_close,
    unix_funcs_count
};
