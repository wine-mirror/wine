/*
 * Wine bluetooth APIs
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
 *
 */

#include <stdarg.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>

#include <wine/debug.h>
#include <wine/heap.h>
#include <wine/unixlib.h>

#include "winebth_priv.h"
#include "unixlib.h"

NTSTATUS winebluetooth_init( void )
{
    NTSTATUS status;

    status = __wine_init_unix_call();
    if (status != STATUS_SUCCESS)
        return status;

    return UNIX_BLUETOOTH_CALL( bluetooth_init, NULL );
}

NTSTATUS winebluetooth_shutdown( void )
{
    return UNIX_BLUETOOTH_CALL( bluetooth_shutdown, NULL );
}
