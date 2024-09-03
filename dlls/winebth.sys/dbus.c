/*
 * Support for communicating with BlueZ over DBus.
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

#include "config.h"

#include <stdlib.h>
#include <dlfcn.h>

#ifdef SONAME_LIBDBUS_1
#include <dbus/dbus.h>
#endif

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winternl.h>
#include <winbase.h>

#include <wine/debug.h>

#include "unixlib.h"
#include "unixlib_priv.h"
#include "dbus.h"

#ifdef SONAME_LIBDBUS_1

WINE_DEFAULT_DEBUG_CHANNEL( winebth );
WINE_DECLARE_DEBUG_CHANNEL( dbus );

const int bluez_timeout = -1;

#define BLUEZ_DEST "org.bluez"
#define BLUEZ_INTERFACE_ADAPTER "org.bluez.Adapter1"

#define DO_FUNC( f ) typeof( f ) (*p_##f)
DBUS_FUNCS;
#undef DO_FUNC

static BOOL load_dbus_functions( void )
{
    void *handle = dlopen( SONAME_LIBDBUS_1, RTLD_NOW );

    if (handle == NULL) goto failed;

#define DO_FUNC( f )                                                                               \
    if (!( p_##f = dlsym( handle, #f ) ))                                                          \
    {                                                                                              \
    ERR( "failed to load symbol %s: %s\n", #f, dlerror() );                                        \
        goto failed;                                                                               \
    }
    DBUS_FUNCS;
#undef DO_FUNC
    return TRUE;

failed:
    WARN( "failed to load DBus support: %s\n", dlerror() );
    return FALSE;
}

static inline const char *dbgstr_dbus_connection( DBusConnection *connection )
{
    return wine_dbg_sprintf( "{%p connected=%d}", connection,
                             p_dbus_connection_get_is_connected( connection ) );
}

void *bluez_dbus_init( void )
{
    DBusError error;
    DBusConnection *connection;

    if (!load_dbus_functions()) return NULL;

    p_dbus_threads_init_default();
    p_dbus_error_init ( &error );

    connection = p_dbus_bus_get_private ( DBUS_BUS_SYSTEM, &error );
    if (!connection)
    {
        ERR( "Failed to get system dbus connection: %s: %s\n", debugstr_a( error.name ), debugstr_a( error.message ) );
        p_dbus_error_free( &error );
        return NULL;
    }

    return connection;
}

void bluez_dbus_close( void *connection )
{
    TRACE_(dbus)( "(%s)\n", dbgstr_dbus_connection( connection ) );

    p_dbus_connection_flush( connection );
    p_dbus_connection_close( connection );
}

void bluez_dbus_free( void *connection )
{
    TRACE_(dbus)( "(%s)\n", dbgstr_dbus_connection( connection ) );

    p_dbus_connection_unref( connection );
}
#else

void *bluez_dbus_init( void ) { return NULL; }
void bluez_dbus_close( void *connection ) {}
void bluez_dbus_free( void *connection ) {}

#endif /* SONAME_LIBDBUS_1 */
