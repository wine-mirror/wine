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
#include "shlobj.h"
#include "wine/unixlib.h"

enum x11drv_funcs
{
    unix_init,
    unix_tablet_get_packet,
    unix_tablet_info,
    unix_tablet_load_info,
    unix_funcs_count,
};

#define X11DRV_CALL(func, params) WINE_UNIX_CALL( unix_ ## func, params )

/* x11drv_tablet_info params */
struct tablet_info_params
{
    UINT category;
    UINT index;
    void *output;
};
