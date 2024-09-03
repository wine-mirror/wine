/*
 * winebluetooth Unix interface
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

#if 0
#pragma makedep unix
#endif

#include <config.h>

#include <stdarg.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <wine/debug.h>

#include "unixlib.h"
#include "unixlib_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL( winebth );

static void *dbus_connection;

static NTSTATUS bluetooth_init ( void *params )
{
    dbus_connection = bluez_dbus_init();
    TRACE("dbus_connection=%p\n", dbus_connection);

    return dbus_connection ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
}

static NTSTATUS bluetooth_shutdown( void *params )
{
    if (!dbus_connection) return STATUS_NOT_SUPPORTED;

    bluez_dbus_close( dbus_connection );
    bluez_dbus_free( dbus_connection );
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] = {
    bluetooth_init,
    bluetooth_shutdown,
};

C_ASSERT( ARRAYSIZE( __wine_unix_call_funcs ) == unix_funcs_count );
