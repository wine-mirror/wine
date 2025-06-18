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

enum android_funcs
{
    unix_dispatch_ioctl,
    unix_init,
    unix_java_init,
    unix_java_uninit,
    unix_register_window,
    unix_funcs_count
};

#define ANDROID_CALL(func, params) WINE_UNIX_CALL( unix_ ## func, params )

/* android_init params */
struct init_params
{
    PNTAPCFUNC register_window_callback;
    UINT64 start_device_callback;
};


/* android_ioctl params */
struct ioctl_params
{
    struct _IRP *irp;
    DWORD client_id;
};


/* android_register_window params */
struct register_window_params
{
    UINT_PTR arg1;
    UINT_PTR arg2;
    UINT_PTR arg3;
};
