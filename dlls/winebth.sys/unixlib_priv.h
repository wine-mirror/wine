/*
 * Bluetoothapis Unix interface
 *
 * Copyright 2024 Vibhav Pant
 * Copyright 2025 Vibhav Pant
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

#ifndef __WINE_WINEBTH_UNIXLIB_PRIV_H
#define __WINE_WINEBTH_UNIXLIB_PRIV_H

#include <config.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>

#include <wine/list.h>
#include <wine/rbtree.h>

#include "unixlib.h"

struct unix_name
{
    char *str;
    SIZE_T refcnt;

    struct wine_rb_entry entry;
};

extern struct unix_name *unix_name_get_or_create( const char *str );
extern void unix_name_free( struct unix_name *name );

extern void *bluez_dbus_init( void );
extern void bluez_dbus_close( void *connection );
extern void bluez_dbus_free( void *connection );
extern NTSTATUS bluez_dbus_loop( void *connection, void *watcher_ctx, struct winebluetooth_event *result );
extern NTSTATUS bluez_adapter_set_prop( void *connection,
                                        struct bluetooth_adapter_set_prop_params *params );
extern NTSTATUS bluez_adapter_start_discovery( void *connection, const char *adapter_path );
extern NTSTATUS bluez_adapter_stop_discovery( void *connection, const char *adapter_path );
extern NTSTATUS bluez_auth_agent_start( void *connection );
extern NTSTATUS bluez_auth_agent_stop( void *connection );
extern NTSTATUS bluez_watcher_init( void *connection, void **ctx );
extern void bluez_watcher_close( void *connection, void *ctx );
#endif /* __WINE_WINEBTH_UNIXLIB_PRIV_H */
