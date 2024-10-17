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
#include <assert.h>
#include <pthread.h>

#ifdef SONAME_LIBDBUS_1
#include <dbus/dbus.h>
#endif

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winternl.h>
#include <winbase.h>
#include <bthsdpdef.h>
#include <bluetoothapis.h>

#include <wine/debug.h>

#include "winebth_priv.h"

#include "unixlib.h"
#include "unixlib_priv.h"
#include "dbus.h"

#ifdef SONAME_LIBDBUS_1

WINE_DEFAULT_DEBUG_CHANNEL( winebth );
WINE_DECLARE_DEBUG_CHANNEL( dbus );

const int bluez_timeout = -1;

#define DBUS_INTERFACE_OBJECTMANAGER "org.freedesktop.DBus.ObjectManager"

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

static const char *bluez_next_dict_entry( DBusMessageIter *iter, DBusMessageIter *variant )
{
    DBusMessageIter sub;
    const char *name;

    if (p_dbus_message_iter_get_arg_type( iter ) != DBUS_TYPE_DICT_ENTRY)
        return NULL;

    p_dbus_message_iter_recurse( iter, &sub );
    p_dbus_message_iter_next( iter );
    p_dbus_message_iter_get_basic( &sub, &name );
    p_dbus_message_iter_next( &sub );
    p_dbus_message_iter_recurse( &sub, variant );
    return name;
}

static inline const char *dbgstr_dbus_connection( DBusConnection *connection )
{
    return wine_dbg_sprintf( "{%p connected=%d}", connection,
                             p_dbus_connection_get_is_connected( connection ) );
}

static NTSTATUS bluez_get_objects_async( DBusConnection *connection, DBusPendingCall **call )
{
    DBusMessage *request;
    dbus_bool_t success;

    TRACE( "Getting managed objects under '/' at service '%s'\n", BLUEZ_DEST );
    request = p_dbus_message_new_method_call(
        BLUEZ_DEST, "/", DBUS_INTERFACE_OBJECTMANAGER, "GetManagedObjects" );
    if (!request)
    {
        return STATUS_NO_MEMORY;
    }

    success = p_dbus_connection_send_with_reply( connection, request, call, -1 );
    p_dbus_message_unref( request );
    if (!success)
        return STATUS_NO_MEMORY;

    if (*call == NULL)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

#define DBUS_OBJECTMANAGER_METHOD_GETMANAGEDOBJECTS_RETURN_SIGNATURE                               \
    DBUS_TYPE_ARRAY_AS_STRING                                                                      \
    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING                                                           \
    DBUS_TYPE_OBJECT_PATH_AS_STRING                                                                \
    DBUS_TYPE_ARRAY_AS_STRING                                                                      \
    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING                                                           \
    DBUS_TYPE_STRING_AS_STRING                                                                     \
    DBUS_TYPE_ARRAY_AS_STRING                                                                      \
    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING                                                           \
    DBUS_TYPE_STRING_AS_STRING                                                                     \
    DBUS_TYPE_VARIANT_AS_STRING                                                                    \
    DBUS_DICT_ENTRY_END_CHAR_AS_STRING                                                             \
    DBUS_DICT_ENTRY_END_CHAR_AS_STRING                                                             \
    DBUS_DICT_ENTRY_END_CHAR_AS_STRING

struct bluez_watcher_ctx
{
    void *init_device_list_call;

    /* struct bluez_init_entry */
    struct list initial_radio_list;
};

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

struct bluez_watcher_event
{
    struct list entry;
    enum winebluetooth_watcher_event_type event_type;
    union winebluetooth_watcher_event_data event;
};

NTSTATUS bluez_watcher_init( void *connection, void **ctx )
{
    NTSTATUS status;
    DBusPendingCall *call;
    struct bluez_watcher_ctx *watcher_ctx =
        calloc( 1, sizeof( struct bluez_watcher_ctx ) );

    if (watcher_ctx == NULL) return STATUS_NO_MEMORY;
    status = bluez_get_objects_async( connection, &call );
    if (status != STATUS_SUCCESS)
    {
        free( watcher_ctx );
        ERR( "could not create async GetManagedObjects call: %#x\n", (int)status);
        return status;
    }
    watcher_ctx->init_device_list_call = call;
    list_init( &watcher_ctx->initial_radio_list );

    *ctx = watcher_ctx;
    TRACE( "ctx=%p\n", ctx );
    return STATUS_SUCCESS;
}

struct bluez_init_entry
{
    union {
        struct winebluetooth_watcher_event_radio_added radio;
    } object;
    struct list entry;
};

static NTSTATUS bluez_build_initial_device_lists( DBusMessage *reply, struct list *adapter_list )
{
    DBusMessageIter dict, paths_iter, iface_iter, prop_iter;
    const char *path;
    NTSTATUS status = STATUS_SUCCESS;

    if (!p_dbus_message_has_signature( reply,
                                       DBUS_OBJECTMANAGER_METHOD_GETMANAGEDOBJECTS_RETURN_SIGNATURE ))
    {
        ERR( "Unexpected signature in GetManagedObjects reply: %s\n",
             debugstr_a( p_dbus_message_get_signature( reply ) ) );
        return STATUS_INTERNAL_ERROR;
    }

    p_dbus_message_iter_init( reply, &dict );
    p_dbus_message_iter_recurse( &dict, &paths_iter );
    while((path = bluez_next_dict_entry( &paths_iter, &iface_iter )))
    {
        const char *iface;
        while ((iface = bluez_next_dict_entry ( &iface_iter, &prop_iter )))
        {
            if (!strcmp( iface, BLUEZ_INTERFACE_ADAPTER ))
            {
                struct bluez_init_entry *init_device = calloc( 1, sizeof( *init_device ) );
                struct unix_name *radio_name;

                if (!init_device)
                {
                    status = STATUS_NO_MEMORY;
                    goto done;
                }
                radio_name = unix_name_get_or_create( path );
                if (!radio_name)
                {
                    free( init_device );
                    status = STATUS_NO_MEMORY;
                    goto done;
                }
                init_device->object.radio.radio.handle = (UINT_PTR)radio_name;
                list_add_tail( adapter_list, &init_device->entry );
                TRACE( "Found BlueZ org.bluez.Adapter1 object %s: %p\n",
                       debugstr_a( radio_name->str ), radio_name );
                break;
            }
        }
    }

    TRACE( "Initial device list: radios: %d\n", list_count( adapter_list ) );
 done:
    return status;
}

static struct bluez_init_entry *bluez_init_entries_list_pop( struct list *list )
{
    struct list *entry = list_head( list );
    struct bluez_init_entry *device = LIST_ENTRY( entry, struct bluez_init_entry, entry );

    list_remove( entry );
    return device;
}

NTSTATUS bluez_dbus_loop( void *c, void *watcher,
                          struct winebluetooth_event *result )
{
    DBusConnection *connection;
    struct bluez_watcher_ctx *watcher_ctx = watcher;

    TRACE( "(%p, %p, %p)\n", c, watcher, result );
    connection = p_dbus_connection_ref( c );

    while (TRUE)
    {
        if (!list_empty( &watcher_ctx->initial_radio_list ))
        {
            struct bluez_init_entry *radio =
                bluez_init_entries_list_pop( &watcher_ctx->initial_radio_list );
            struct winebluetooth_watcher_event *watcher_event = &result->data.watcher_event;

            result->status = WINEBLUETOOTH_EVENT_WATCHER_EVENT;
            watcher_event->event_type = BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_ADDED;
            watcher_event->event_data.radio_added = radio->object.radio;
            free( radio );
            p_dbus_connection_unref( connection );
            return STATUS_PENDING;
        }
        else if (!p_dbus_connection_read_write_dispatch( connection, 100 ))
        {
            p_dbus_connection_unref( connection );
            TRACE( "Disconnected from DBus\n" );
            return STATUS_SUCCESS;
        }

        if (watcher_ctx->init_device_list_call != NULL
            && p_dbus_pending_call_get_completed( watcher_ctx->init_device_list_call ))
        {
            DBusMessage *reply = p_dbus_pending_call_steal_reply( watcher_ctx->init_device_list_call );
            DBusError error;
            NTSTATUS status;

            p_dbus_pending_call_unref( watcher_ctx->init_device_list_call );
            watcher_ctx->init_device_list_call = NULL;

            p_dbus_error_init( &error );
            if (p_dbus_set_error_from_message( &error, reply ))
            {
                ERR( "Error getting object list from BlueZ: '%s': '%s'\n", error.name,
                     error.message );
                p_dbus_error_free( &error );
                p_dbus_message_unref( reply );
                p_dbus_connection_unref( connection );
                return STATUS_NO_MEMORY;
            }
            status = bluez_build_initial_device_lists( reply, &watcher_ctx->initial_radio_list );
            p_dbus_message_unref( reply );
            if (status != STATUS_SUCCESS)
            {
                ERR( "Error building initial bluetooth devices list: %#x\n", (int)status );
                p_dbus_connection_unref( connection );
                return status;
            }
        }
    }
}
#else

void *bluez_dbus_init( void ) { return NULL; }
void bluez_dbus_close( void *connection ) {}
void bluez_dbus_free( void *connection ) {}
NTSTATUS bluez_watcher_init( void *connection, void **ctx ) { return STATUS_NOT_SUPPORTED; }
NTSTATUS bluez_dbus_loop( void *c, void *watcher, struct winebluetooth_event *result )
{
    return STATUS_NOT_SUPPORTED;
}

#endif /* SONAME_LIBDBUS_1 */
