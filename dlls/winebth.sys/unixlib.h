/*
 * Unix interface definitions
 *
 * Copyright 2024 Vibhav Pant
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

#ifndef __WINE_WINEBTH_UNIXLIB_H
#define __WINE_WINEBTH_UNIXLIB_H

#include <stdarg.h>

#include <wine/unixlib.h>

enum bluetoothapis_funcs
{
    unix_bluetooth_init,
    unix_bluetooth_shutdown,

    unix_funcs_count
};

#define UNIX_BLUETOOTH_CALL( func, params ) WINE_UNIX_CALL( unix_##func, params )

#endif /* __WINE_WINEBTH_UNIXLIB_H */
