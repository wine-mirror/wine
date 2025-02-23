/*
 * Support for communicating with BlueZ over DBus.
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


#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

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
#include <wine/winebth.h>

#include <wine/debug.h>

#include "winebth_priv.h"

#include "unixlib.h"
#include "unixlib_priv.h"
#include "dbus.h"

#ifdef SONAME_LIBDBUS_1

/* BlueZ is the userspace Bluetooth management daemon for Linux systems, which provides an
 * object-based API for interacting with Bluetooth adapters and devices on top of DBus. Every DBus
 * object has one or more "interfaces", with each interface have its own set of methods and signals
 * that we can call or listen for, respectively. Objects are identified by their "path", which
 * provide a way to organize objects under a hierarchy. For instance, a bluetooth adapter/radio will
 * have a path of the form "/org/bluez/hciX", with any remote devices discovered by or bonded to it
 * having the path "/org/bluez/hciX/XX:XX:XX:XX:XX:XX". Similarly, any LE GATT services,
 * characteristics or descriptors specific to this remote device are represented as objects under
 * the device's path. Among BlueZ's own interfaces, the other important interfaces we rely on are:
 *
 * org.freedesktop.DBus.Properties: Allows us to read and (optionally) modify properties specific
 * to an object's interface using the "Get" and "Set" methods. Additionally, some objects may
 * broadcast a "PropertiesChanged" signal if any of their properties change, which gets used by
 * bluez_filter to let the driver know to appropriately update the equivalent PDO. The "GetAll"
 * method provides a convenient way to get all properties for an interface.
 *
 * org.freedesktop.DBus.ObjectManager: This contains the "GetManagedObjects" method, that returns a
 * list of all known objects under the object's path namespace, with the interfaces they implement
 * (and any properties those interfaces might have). This is used to build a list of initially known
 * devices in bluez_build_initial_device_lists. The "InterfacesAdded" and "InterfacesRemoved"
 * signals are used to detect when a bluetooth adapter or device has been addeed/removed in
 * bluez_filter, which we relay to the bluetooth driver to add/remove the corresponding PDO.
 */

WINE_DEFAULT_DEBUG_CHANNEL( winebth );
WINE_DECLARE_DEBUG_CHANNEL( dbus );

const int bluez_timeout = -1;

#define DBUS_INTERFACE_OBJECTMANAGER "org.freedesktop.DBus.ObjectManager"
#define DBUS_OBJECTMANAGER_SIGNAL_INTERFACESADDED "InterfacesAdded"
#define DBUS_OBJECTMANAGER_SIGNAL_INTERFACESREMOVED "InterfacesRemoved"
#define DBUS_PROPERTIES_SIGNAL_PROPERTIESCHANGED "PropertiesChanged"

#define DBUS_INTERFACES_ADDED_SIGNATURE                                                            \
    DBUS_TYPE_OBJECT_PATH_AS_STRING                                                                \
    DBUS_TYPE_ARRAY_AS_STRING                                                                      \
    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING                                                           \
    DBUS_TYPE_STRING_AS_STRING                                                                     \
    DBUS_TYPE_ARRAY_AS_STRING                                                                      \
    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING    \
    DBUS_DICT_ENTRY_END_CHAR_AS_STRING                                                             \
    DBUS_DICT_ENTRY_END_CHAR_AS_STRING

#define DBUS_INTERFACES_REMOVED_SIGNATURE                                                          \
    DBUS_TYPE_OBJECT_PATH_AS_STRING                                                                \
    DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING

#define DBUS_PROPERTIES_CHANGED_SIGNATURE                                                          \
    DBUS_TYPE_STRING_AS_STRING                                                                     \
    DBUS_TYPE_ARRAY_AS_STRING                                                                      \
    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING                                                           \
    DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING                                         \
    DBUS_DICT_ENTRY_END_CHAR_AS_STRING                                                             \
    DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING

#define BLUEZ_DEST "org.bluez"
#define BLUEZ_INTERFACE_ADAPTER "org.bluez.Adapter1"
#define BLUEZ_INTERFACE_DEVICE  "org.bluez.Device1"

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

static NTSTATUS bluez_dbus_error_to_ntstatus( const DBusError *error )
{

#define DBUS_ERROR_CASE(n, s) if (p_dbus_error_has_name( error, (n)) ) return (s)

    DBUS_ERROR_CASE( "org.bluez.Error.Failed", STATUS_INTERNAL_ERROR);
    DBUS_ERROR_CASE( "org.bluez.Error.NotReady", STATUS_DEVICE_NOT_READY );
    DBUS_ERROR_CASE( "org.bluez.Error.NotAuthorized", STATUS_ACCESS_DENIED );
    DBUS_ERROR_CASE( "org.bluez.Error.InvalidArguments", STATUS_INVALID_PARAMETER );
    DBUS_ERROR_CASE( "org.bluez.Error.AlreadyExists", STATUS_NO_MORE_ENTRIES );
    DBUS_ERROR_CASE( "org.bluez.Error.AuthenticationCanceled", STATUS_CANCELLED );
    DBUS_ERROR_CASE( "org.bluez.Error.AuthenticationFailed", STATUS_INTERNAL_ERROR );
    DBUS_ERROR_CASE( "org.bluez.Error.AuthenticationRejected", STATUS_INTERNAL_ERROR );
    DBUS_ERROR_CASE( "org.bluez.Error.AuthenticationTimeout", STATUS_TIMEOUT );
    DBUS_ERROR_CASE( "org.bluez.Error.ConnectionAttemptFailed", STATUS_DEVICE_NOT_CONNECTED);
    DBUS_ERROR_CASE( "org.bluez.Error.NotConnected", STATUS_DEVICE_NOT_CONNECTED );
    DBUS_ERROR_CASE( "org.bluez.Error.InProgress", STATUS_OPERATION_IN_PROGRESS );
    DBUS_ERROR_CASE( DBUS_ERROR_UNKNOWN_OBJECT, STATUS_INVALID_PARAMETER );
    DBUS_ERROR_CASE( DBUS_ERROR_NO_MEMORY, STATUS_NO_MEMORY );
    DBUS_ERROR_CASE( DBUS_ERROR_NOT_SUPPORTED, STATUS_NOT_SUPPORTED );
    DBUS_ERROR_CASE( DBUS_ERROR_ACCESS_DENIED, STATUS_ACCESS_DENIED );
    return STATUS_INTERNAL_ERROR;
#undef DBUS_ERROR_CASE
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

static const char *dbgstr_dbus_message( DBusMessage *message )
{
    const char *interface;
    const char *member;
    const char *path;
    const char *sender;
    const char *signature;
    int type;

    interface = p_dbus_message_get_interface( message );
    member = p_dbus_message_get_member( message );
    path = p_dbus_message_get_path( message );
    sender = p_dbus_message_get_sender( message );
    type = p_dbus_message_get_type( message );
    signature = p_dbus_message_get_signature( message );

    switch (type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
        return wine_dbg_sprintf( "{method_call sender=%s interface=%s member=%s path=%s signature=%s}",
                                 debugstr_a( sender ), debugstr_a( interface ), debugstr_a( member ),
                                 debugstr_a( path ), debugstr_a( signature ) );
    case DBUS_MESSAGE_TYPE_SIGNAL:
        return wine_dbg_sprintf( "{signal sender=%s interface=%s member=%s path=%s signature=%s}",
                                 debugstr_a( sender ), debugstr_a( interface ), debugstr_a( member ),
                                 debugstr_a( path ), debugstr_a( signature ) );
    default:
        return wine_dbg_sprintf( "%p", message );
    }
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


static void parse_mac_address( const char *addr_str, BYTE dest[6] )
{
    int addr[6], i;

    sscanf( addr_str, "%x:%x:%x:%x:%x:%x", &addr[0], &addr[1], &addr[2], &addr[3], &addr[4],
            &addr[5] );
    for (i = 0 ; i < 6; i++)
        dest[i] = addr[i];
}

static void bluez_dbus_wait_for_reply_callback( DBusPendingCall *pending_call, void *wait )
{
    sem_post( wait );
}

static NTSTATUS bluez_dbus_pending_call_wait( DBusPendingCall *pending_call, DBusMessage **reply, DBusError *error )
{
    sem_t wait;

    sem_init( &wait, 0, 0 );
    if (!p_dbus_pending_call_set_notify( pending_call, bluez_dbus_wait_for_reply_callback, &wait, NULL ))
    {
        sem_destroy( &wait );
        p_dbus_pending_call_unref( pending_call );
        return STATUS_NO_MEMORY;
    }
    for (;;)
    {
        int ret = sem_wait( &wait );
        if (!ret)
        {
            *reply = p_dbus_pending_call_steal_reply( pending_call );
            if (p_dbus_set_error_from_message( error, *reply ))
            {
                p_dbus_message_unref( *reply );
                *reply = NULL;
            }
            p_dbus_pending_call_unref( pending_call );
            sem_destroy( &wait );
            return STATUS_SUCCESS;
        }
        if (errno == EINTR)
            continue;

        ERR( "Failed to wait for DBus method reply: %s\n", debugstr_a( strerror( errno ) ) );
        sem_destroy( &wait );
        p_dbus_pending_call_cancel( pending_call );
        p_dbus_pending_call_unref( pending_call );
        return STATUS_INTERNAL_ERROR;
    }
}

/* Like dbus_connection_send_with_reply_and_block, but it does not acquire a lock on the connection, instead relying on
 * the main loop in bluez_dbus_loop. This is faster than send_with_reply_and_block.
 * This takes ownership of the request, so there is no need to unref it. */
static NTSTATUS bluez_dbus_send_and_wait_for_reply( DBusConnection *connection, DBusMessage *request, DBusMessage **reply,
                                                    DBusError *error )
{
    DBusPendingCall *pending_call;
    dbus_bool_t success;

    success = p_dbus_connection_send_with_reply( connection, request, &pending_call, bluez_timeout );
    p_dbus_message_unref( request );

    if (!success)
        return STATUS_NO_MEMORY;
    return bluez_dbus_pending_call_wait( pending_call, reply, error );
}

NTSTATUS bluez_adapter_set_prop( void *connection, struct bluetooth_adapter_set_prop_params *params )
{
    DBusMessage *request, *reply;
    DBusMessageIter iter, sub_iter;
    DBusError error;
    DBusBasicValue val;
    int val_type;
    static const char *adapter_iface = BLUEZ_INTERFACE_ADAPTER;
    const char *prop_name;
    NTSTATUS status;

    TRACE( "(%p, %p)\n", connection, params );

    switch (params->prop_flag)
    {
    case LOCAL_RADIO_CONNECTABLE:
        prop_name = "Connectable";
        val.bool_val = params->prop->boolean;
        val_type = DBUS_TYPE_BOOLEAN;
        break;
    case LOCAL_RADIO_DISCOVERABLE:
        prop_name = "Discoverable";
        val.bool_val = params->prop->boolean;
        val_type = DBUS_TYPE_BOOLEAN;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    TRACE( "Setting property %s for adapter %s\n", debugstr_a( prop_name ), debugstr_a( params->adapter->str ) );
    request = p_dbus_message_new_method_call( BLUEZ_DEST, params->adapter->str,
                                              DBUS_INTERFACE_PROPERTIES, "Set" );
    if (!request) return STATUS_NO_MEMORY;

    p_dbus_message_iter_init_append( request, &iter );
    if (!p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &adapter_iface ))
    {
        p_dbus_message_unref( request );
        return STATUS_NO_MEMORY;
    }
    if (!p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &prop_name ))
    {
        p_dbus_message_unref( request );
        return STATUS_NO_MEMORY;
    }
    if (!p_dbus_message_iter_open_container( &iter, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING, &sub_iter ))
    {
        p_dbus_message_unref( request );
        return STATUS_NO_MEMORY;
    }
    if (!p_dbus_message_iter_append_basic( &sub_iter, val_type, &val ))
    {
        p_dbus_message_iter_abandon_container( &iter, &sub_iter );
        p_dbus_message_unref( request );
        return STATUS_NO_MEMORY;
    }
    if (!p_dbus_message_iter_close_container( &iter, &sub_iter ))
    {
        p_dbus_message_unref( request );
        return STATUS_NO_MEMORY;
    }

    p_dbus_error_init( &error );
    status = bluez_dbus_send_and_wait_for_reply( connection, request, &reply, &error );
    if (status)
    {
        p_dbus_error_free( &error );
        return status;
    }
    if (!reply)
    {
        ERR( "Failed to set property %s for adapter %s: %s: %s\n", debugstr_a( prop_name ),
             debugstr_a( params->adapter->str ), debugstr_a( error.name ), debugstr_a( error.message ) );
        status = bluez_dbus_error_to_ntstatus( &error );
        p_dbus_error_free( &error );
        return status;
    }
    p_dbus_error_free( &error );
    p_dbus_message_unref( reply );

    return STATUS_SUCCESS;
}

static void bluez_radio_prop_from_dict_entry( const char *prop_name, DBusMessageIter *variant,
                                              struct winebluetooth_radio_properties *props,
                                              winebluetooth_radio_props_mask_t *props_mask,
                                              winebluetooth_radio_props_mask_t wanted_props_mask )
{
    TRACE_(dbus)( "(%s, %p, %p, %p, %#x)\n", debugstr_a( prop_name ), variant, props, props_mask,
                  wanted_props_mask );

    if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_ADDRESS &&
        !strcmp( prop_name, "Address" ) &&
        p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_STRING)
    {
        const char *addr_str;
        p_dbus_message_iter_get_basic( variant, &addr_str );
        parse_mac_address( addr_str, props->address.rgBytes );
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_ADDRESS;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_CLASS &&
             !strcmp( prop_name, "Class" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_UINT32)
    {
        dbus_uint32_t class;
        p_dbus_message_iter_get_basic( variant, &class );
        props->class = class;
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_CLASS;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_MANUFACTURER &&
             !strcmp( prop_name, "Manufacturer" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_UINT16)
    {
        dbus_uint16_t manufacturer;
        p_dbus_message_iter_get_basic( variant, &manufacturer );
        props->manufacturer = manufacturer;
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_MANUFACTURER;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_CONNECTABLE &&
             !strcmp( prop_name, "Connectable" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BOOLEAN)
    {
        dbus_bool_t connectable;
        p_dbus_message_iter_get_basic( variant, &connectable );
        props->connectable = connectable != 0;
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_CONNECTABLE;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERABLE &&
             !strcmp( prop_name, "Discoverable" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BOOLEAN)
    {
        dbus_bool_t discoverable;
        p_dbus_message_iter_get_basic( variant, &discoverable );
        props->discoverable = discoverable != 0;
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERABLE;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERING &&
             !strcmp( prop_name, "Discovering") &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BOOLEAN)
    {
        dbus_bool_t discovering;
        p_dbus_message_iter_get_basic( variant, &discovering );
        props->discovering = discovering != 0;
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERING;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_PAIRABLE &&
             !strcmp( prop_name, "Pairable" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BOOLEAN)
    {
        dbus_bool_t pairable;
        p_dbus_message_iter_get_basic( variant, &pairable );
        props->pairable = pairable != 0;
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_PAIRABLE;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_RADIO_PROPERTY_VERSION &&
             !strcmp( prop_name, "Version" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BYTE)
    {
        p_dbus_message_iter_get_basic( variant, &props->version );
        *props_mask |= WINEBLUETOOTH_RADIO_PROPERTY_VERSION;
    }
}

static void bluez_device_prop_from_dict_entry( const char *prop_name, DBusMessageIter *variant,
                                               struct winebluetooth_device_properties *props,
                                               winebluetooth_device_props_mask_t *props_mask,
                                               winebluetooth_device_props_mask_t wanted_props_mask )
{
    TRACE_( dbus )( "(%s, %p, %p, %p, %#x)\n", debugstr_a( prop_name ), variant, props, props_mask,
                    wanted_props_mask );


    if (wanted_props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_NAME &&
        !strcmp( prop_name, "Name" ) &&
        p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_STRING)
    {
        const char *name_str;
        SIZE_T len;

        p_dbus_message_iter_get_basic( variant, &name_str );
        len = strlen( name_str );
        memcpy( props->name, name_str, min( len + 1, ARRAYSIZE( props->name ) ) );
        props->name[ARRAYSIZE( props->name ) - 1] = '\0';
        *props_mask |= WINEBLUETOOTH_DEVICE_PROPERTY_NAME;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_ADDRESS &&
        !strcmp( prop_name, "Address" ) &&
        p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_STRING)
    {
        const char *addr_str;

        p_dbus_message_iter_get_basic( variant, &addr_str );
        parse_mac_address( addr_str, props->address.rgBytes );
        *props_mask |= WINEBLUETOOTH_DEVICE_PROPERTY_ADDRESS;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_CONNECTED &&
             !strcmp( prop_name, "Connected" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BOOLEAN)
    {
        dbus_bool_t connected;

        p_dbus_message_iter_get_basic( variant, &connected );
        props->connected = !!connected;
        *props_mask |= WINEBLUETOOTH_DEVICE_PROPERTY_CONNECTED;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_PAIRED &&
             !strcmp( prop_name, "Paired" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BOOLEAN)
    {
        dbus_bool_t paired;

        p_dbus_message_iter_get_basic( variant, &paired );
        props->paired = !!paired;
        *props_mask |= WINEBLUETOOTH_DEVICE_PROPERTY_PAIRED;
    }
    else if (wanted_props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_LEGACY_PAIRING &&
             !strcmp( prop_name, "LegacyPairing" ) &&
             p_dbus_message_iter_get_arg_type( variant ) == DBUS_TYPE_BOOLEAN)
    {
        dbus_bool_t legacy;

        p_dbus_message_iter_get_basic( variant, &legacy );
        props->legacy_pairing = !!legacy;
        *props_mask |= WINEBLUETOOTH_DEVICE_PROPERTY_LEGACY_PAIRING;
    }
}

static NTSTATUS bluez_adapter_get_props_async( void *connection, const char *radio_object_path,
                                               DBusPendingCall **call )
{
    DBusMessage *request;
    static const char *adapter_iface = BLUEZ_INTERFACE_ADAPTER;
    dbus_bool_t success;

    request = p_dbus_message_new_method_call( BLUEZ_DEST, radio_object_path,
                                              DBUS_INTERFACE_PROPERTIES, "GetAll" );
    if (!request) return STATUS_NO_MEMORY;

    p_dbus_message_append_args( request, DBUS_TYPE_STRING, &adapter_iface, DBUS_TYPE_INVALID );

    success = p_dbus_connection_send_with_reply( connection, request, call, bluez_timeout );
    p_dbus_message_unref( request );
    if (!success)
        return STATUS_NO_MEMORY;
    if (!*call)
        return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

struct bluez_watcher_ctx
{
    void *init_device_list_call;

    /* struct bluez_init_entry */
    struct list initial_radio_list;
    /* struct bluez_init_entry */
    struct list initial_device_list;

    /* struct bluez_watcher_event */
    struct list event_list;
};

struct bluez_init_entry
{
    union {
        struct winebluetooth_watcher_event_radio_added radio;
        struct winebluetooth_watcher_event_device_added device;
    } object;
    struct list entry;
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
        WARN( "Failed to get system dbus connection: %s: %s\n", debugstr_a( error.name ), debugstr_a( error.message ) );
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

    /* Some DBus signals, like PropertiesChanged in org.freedesktop.DBus.Properties, require us to
     * perform an additional call to get the complete state of the object (in this instance, call
     * Get/GetAll to get the values of invalidated properties). The event is queued out only once
     * this call completes. */
    DBusPendingCall *pending_call;
};

static BOOL bluez_event_list_queue_new_event_with_call(
    struct list *event_list, enum winebluetooth_watcher_event_type event_type,
    union winebluetooth_watcher_event_data event, DBusPendingCall *call,
    DBusPendingCallNotifyFunction callback )
{
    struct bluez_watcher_event *event_entry;

    if (!(event_entry = calloc(1, sizeof( *event_entry ) )))
    {
        ERR( "Could not allocate memory for DBus event.\n" );
        return FALSE;
    }

    event_entry->event_type = event_type;
    event_entry->event = event;
    event_entry->pending_call = call;
    if (call && callback)
        p_dbus_pending_call_set_notify( call, callback, &event_entry->event, NULL );
    list_add_tail( event_list, &event_entry->entry );

    return TRUE;
}

static BOOL bluez_event_list_queue_new_event( struct list *event_list,
                                              enum winebluetooth_watcher_event_type event_type,
                                              union winebluetooth_watcher_event_data event )
{
    return bluez_event_list_queue_new_event_with_call( event_list, event_type, event, NULL, NULL );
}

static void bluez_filter_radio_props_changed_callback( DBusPendingCall *call, void *user_data )
{
    union winebluetooth_watcher_event_data *event = user_data;
    struct winebluetooth_watcher_event_radio_props_changed *changed = &event->radio_props_changed;
    const struct unix_name *radio = (struct unix_name *)event->radio_props_changed.radio.handle;
    DBusMessage *reply;
    DBusMessageIter dict, prop_iter, variant;
    const char *prop_name;
    DBusError error;

    TRACE( "call %p, radio %s\n", call, debugstr_a( radio->str ) );

    reply = p_dbus_pending_call_steal_reply( call );
    p_dbus_error_init( &error );
    if (p_dbus_set_error_from_message( &error, reply ))
    {
        ERR( "Failed to get adapter properties for %s: %s: %s\n", debugstr_a( radio->str ),
             debugstr_a( error.name ), debugstr_a( error.message ) );
        p_dbus_error_free( &error );
        p_dbus_message_unref( reply );
        return;
    }
    p_dbus_error_free( &error );

    p_dbus_message_iter_init( reply, &dict );
    p_dbus_message_iter_recurse( &dict, &prop_iter );
    while((prop_name = bluez_next_dict_entry( &prop_iter, &variant )))
    {
        bluez_radio_prop_from_dict_entry( prop_name, &variant, &changed->props,
                                          &changed->changed_props_mask, changed->invalid_props_mask );
    }
    changed->invalid_props_mask &= ~changed->changed_props_mask;
    p_dbus_message_unref( reply );
}

struct bluez_object_property_masks
{
    const char *prop_name;
    UINT16 mask;
};

static UINT16 bluez_dbus_get_invalidated_properties_from_iter(
    DBusMessageIter *invalid_prop_iter, const struct bluez_object_property_masks *prop_masks,
    SIZE_T len )
{
    UINT16 mask = 0;

    while (p_dbus_message_iter_has_next( invalid_prop_iter ))
    {
        const char *prop_name;
        SIZE_T i;

        assert( p_dbus_message_iter_get_arg_type( invalid_prop_iter ) == DBUS_TYPE_STRING );
        p_dbus_message_iter_get_basic( invalid_prop_iter, &prop_name );
        for (i = 0; i < len; i++)
        {
            if (strcmp( prop_masks[i].prop_name, prop_name ) == 0)
                mask |= prop_masks[i].mask;
        }

        p_dbus_message_iter_next( invalid_prop_iter );
    }

    return mask;
}

static DBusHandlerResult bluez_filter( DBusConnection *conn, DBusMessage *msg, void *user_data )
{
    struct list *event_list;

    if (TRACE_ON( dbus ))
        TRACE_( dbus )( "(%s, %s, %p)\n", dbgstr_dbus_connection( conn ), dbgstr_dbus_message( msg ), user_data );

    event_list = &((struct bluez_watcher_ctx *)user_data)->event_list;

    if (p_dbus_message_is_signal( msg, DBUS_INTERFACE_OBJECTMANAGER, DBUS_OBJECTMANAGER_SIGNAL_INTERFACESADDED )
        && p_dbus_message_has_signature( msg, DBUS_INTERFACES_ADDED_SIGNATURE ))
    {
        DBusMessageIter iter, ifaces_iter;
        const char *object_path;

        p_dbus_message_iter_init( msg, &iter );
        p_dbus_message_iter_get_basic( &iter, &object_path );
        p_dbus_message_iter_next( &iter );
        p_dbus_message_iter_recurse( &iter, &ifaces_iter );
        while (p_dbus_message_iter_has_next( &ifaces_iter ))
        {
            DBusMessageIter iface_entry;
            const char *iface_name;

            p_dbus_message_iter_recurse( &ifaces_iter, &iface_entry );
            p_dbus_message_iter_get_basic( &iface_entry, &iface_name );
            if (!strcmp( iface_name, BLUEZ_INTERFACE_ADAPTER ))
            {
                struct winebluetooth_watcher_event_radio_added radio_added = {0};
                struct unix_name *radio;
                DBusMessageIter props_iter, variant;
                const char *prop_name;

                p_dbus_message_iter_next( &iface_entry );
                p_dbus_message_iter_recurse( &iface_entry, &props_iter );

                while((prop_name = bluez_next_dict_entry( &props_iter, &variant )))
                {
                    bluez_radio_prop_from_dict_entry( prop_name, &variant, &radio_added.props,
                                                      &radio_added.props_mask,
                                                      WINEBLUETOOTH_RADIO_ALL_PROPERTIES );
                }

                radio = unix_name_get_or_create( object_path );
                radio_added.radio.handle = (UINT_PTR)radio;
                if (!radio_added.radio.handle)
                {
                    ERR( "failed to allocate memory for adapter path %s\n", debugstr_a( object_path ) );
                    break;
                }
                else
                {
                    union winebluetooth_watcher_event_data event = { .radio_added = radio_added };
                    TRACE( "New BlueZ org.bluez.Adapter1 object added at %s: %p\n",
                           debugstr_a( object_path ), radio );
                    if (!bluez_event_list_queue_new_event(
                            event_list, BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_ADDED, event ))
                        unix_name_free( radio );
                }
            }
            else if (!strcmp( iface_name, BLUEZ_INTERFACE_DEVICE ))
            {
                struct winebluetooth_watcher_event_device_added device_added = {0};
                struct unix_name *device_name, *radio_name = NULL;
                DBusMessageIter props_iter, variant;
                const char *prop_name;

                device_name = unix_name_get_or_create( object_path );
                device_added.device.handle = (UINT_PTR)device_name;
                if (!device_name)
                {
                    ERR("Failed to allocate memory for device path %s\n", debugstr_a( object_path ));
                    break;
                }
                p_dbus_message_iter_next( &iface_entry );
                p_dbus_message_iter_recurse( &iface_entry, &props_iter );

                while((prop_name = bluez_next_dict_entry( &props_iter, &variant )))
                {
                    if (!strcmp( prop_name, "Adapter" ) &&
                        p_dbus_message_iter_get_arg_type( &variant ) == DBUS_TYPE_OBJECT_PATH)
                    {
                        const char *path;

                        p_dbus_message_iter_get_basic( &variant, &path );
                        radio_name = unix_name_get_or_create( path );
                        if (!radio_name)
                        {
                            unix_name_free( device_name );
                            ERR("Failed to allocate memory for radio path %s\n", debugstr_a( path ));
                            break;
                        }
                        device_added.radio.handle = (UINT_PTR)radio_name;
                    }
                    else
                        bluez_device_prop_from_dict_entry( prop_name, &variant, &device_added.props,
                                                           &device_added.known_props_mask,
                                                           WINEBLUETOOTH_DEVICE_ALL_PROPERTIES );
                }

                if (!radio_name)
                {
                    unix_name_free( device_name );
                    ERR( "Could not find the associated adapter for device %s\n", debugstr_a( object_path ) );
                    break;
                }
                else
                {
                    union winebluetooth_watcher_event_data event = { .device_added = device_added };
                    TRACE( "New BlueZ org.bluez.Device1 object added at %s: %p\n", debugstr_a( object_path ),
                           device_name );
                    if (!bluez_event_list_queue_new_event( event_list, BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_ADDED, event ))
                    {
                        unix_name_free( device_name );
                        unix_name_free( radio_name );
                    }
                }
            }
            p_dbus_message_iter_next( &ifaces_iter );
        }
    }
    else if (p_dbus_message_is_signal( msg, DBUS_INTERFACE_OBJECTMANAGER, DBUS_OBJECTMANAGER_SIGNAL_INTERFACESREMOVED )
             && p_dbus_message_has_signature( msg, DBUS_INTERFACES_REMOVED_SIGNATURE ))
    {
        const char *object_path;
        char **interfaces;
        int n_interfaces, i;
        DBusError error;
        dbus_bool_t success;

        p_dbus_error_init( &error );
        success = p_dbus_message_get_args( msg, &error, DBUS_TYPE_OBJECT_PATH, &object_path,
                                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &interfaces,
                                           &n_interfaces, DBUS_TYPE_INVALID );
        if (!success)
        {
            ERR( "error getting arguments from message: %s: %s\n", debugstr_a( error.name ),
                 debugstr_a( error.message ) );
            p_dbus_error_free( &error );
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        p_dbus_error_free( &error );
        for (i = 0; i < n_interfaces; i++)
        {
            if (!strcmp( interfaces[i], BLUEZ_INTERFACE_ADAPTER ))
            {
                winebluetooth_radio_t radio;
                struct unix_name *radio_name;
                union winebluetooth_watcher_event_data event;

                radio_name = unix_name_get_or_create( object_path );
                if (!radio_name)
                {
                    ERR( "failed to allocate memory for adapter path %s\n", object_path );
                    continue;
                }
                radio.handle = (UINT_PTR)radio_name;
                event.radio_removed = radio;
                if (!bluez_event_list_queue_new_event(
                        event_list, BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_REMOVED, event ))
                    unix_name_free( radio_name );
            }
            else if (!strcmp( interfaces[i], BLUEZ_INTERFACE_DEVICE ))
            {
                struct unix_name *device;
                union winebluetooth_watcher_event_data event;

                device = unix_name_get_or_create( object_path );
                if (!device)
                {
                    ERR( "Failed to allocate memory for adapter path %s\n", object_path );
                    continue;
                }
                event.device_removed.device.handle = (UINT_PTR)device;
                if (!bluez_event_list_queue_new_event( event_list, BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_REMOVED,
                                                       event ))
                    unix_name_free( device );
            }
        }
        p_dbus_free_string_array( interfaces );
    }
    else if (p_dbus_message_is_signal( msg, DBUS_INTERFACE_PROPERTIES, DBUS_PROPERTIES_SIGNAL_PROPERTIESCHANGED ) &&
             p_dbus_message_has_signature( msg, DBUS_PROPERTIES_CHANGED_SIGNATURE ))
    {
        DBusMessageIter iter;
        const char *iface;

        p_dbus_message_iter_init( msg, &iter );
        p_dbus_message_iter_get_basic( &iter, &iface );

        if (!strcmp( iface, BLUEZ_INTERFACE_ADAPTER ))
        {
            struct winebluetooth_watcher_event_radio_props_changed props_changed = {0};
            struct unix_name *radio;
            DBusMessageIter changed_props_iter, invalid_props_iter, variant;
            const char *prop_name;
            const static struct bluez_object_property_masks radio_prop_masks[] = {
                { "Name", WINEBLUETOOTH_RADIO_PROPERTY_NAME },
                { "Address", WINEBLUETOOTH_RADIO_PROPERTY_ADDRESS },
                { "Discoverable", WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERABLE },
                { "Connectable", WINEBLUETOOTH_RADIO_PROPERTY_CONNECTABLE },
                { "Class", WINEBLUETOOTH_RADIO_PROPERTY_CLASS },
                { "Manufacturer", WINEBLUETOOTH_RADIO_PROPERTY_MANUFACTURER },
                { "Version", WINEBLUETOOTH_RADIO_PROPERTY_VERSION },
                { "Discovering", WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERING },
                { "Pairable", WINEBLUETOOTH_RADIO_PROPERTY_PAIRABLE },
            };
            const char *object_path;

            p_dbus_message_iter_next( &iter );
            p_dbus_message_iter_recurse( &iter, &changed_props_iter );
            while ((prop_name = bluez_next_dict_entry( &changed_props_iter, &variant )))
            {
                bluez_radio_prop_from_dict_entry( prop_name, &variant, &props_changed.props,
                                                  &props_changed.changed_props_mask,
                                                  WINEBLUETOOTH_RADIO_ALL_PROPERTIES );
            }

            p_dbus_message_iter_next( &iter );
            p_dbus_message_iter_recurse( &iter, &invalid_props_iter );
            props_changed.invalid_props_mask = bluez_dbus_get_invalidated_properties_from_iter(
                &invalid_props_iter, radio_prop_masks, ARRAY_SIZE( radio_prop_masks ) );
            if (!props_changed.changed_props_mask && !props_changed.invalid_props_mask)
                /* No properties that are of any interest to us have changed or been invalidated,
                 * no need to generate an event. */
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

            object_path = p_dbus_message_get_path( msg );
            radio = unix_name_get_or_create( object_path );
            if (!radio)
            {
                ERR( "failed to allocate memory for adapter path %s\n", debugstr_a( object_path ) );
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
            props_changed.radio.handle = (UINT_PTR)radio;
            TRACE( "Properties changed for radio %s, changed %#x, invalid %#x\n",
                   debugstr_a( radio->str ), props_changed.changed_props_mask,
                  props_changed.invalid_props_mask );
            if (props_changed.invalid_props_mask != 0)
            {
                DBusPendingCall *pending_call = NULL;
                union winebluetooth_watcher_event_data event = { .radio_props_changed = props_changed };
                NTSTATUS status = bluez_adapter_get_props_async( conn, radio->str, &pending_call );

                if (status != STATUS_SUCCESS)
                {
                    ERR( "Failed to create async call to get adapter properties: %#x\n",
                         (int)status );
                    unix_name_free( radio );
                    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                }

                if (!bluez_event_list_queue_new_event_with_call( event_list,
                                                                BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_PROPERTIES_CHANGED,
                                                                event, pending_call,
                                                                bluez_filter_radio_props_changed_callback ))
                {
                    unix_name_free( radio );
                    p_dbus_pending_call_cancel( pending_call );
                    p_dbus_pending_call_unref( pending_call );
                    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                }
            }
            else
            {
                union winebluetooth_watcher_event_data event = { .radio_props_changed = props_changed };
                if (!bluez_event_list_queue_new_event( event_list,
                                                      BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_PROPERTIES_CHANGED,
                                                      event ))
                {
                    unix_name_free( radio );
                    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                }
            }
        }
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static const char BLUEZ_MATCH_OBJECTMANAGER[] = "type='signal',"
                                                "interface='org.freedesktop.DBus.ObjectManager',"
                                                "sender='"BLUEZ_DEST"',"
                                                "path='/'";
static const char BLUEZ_MATCH_PROPERTIES[] = "type='signal',"
                                             "interface='"DBUS_INTERFACE_PROPERTIES"',"
                                             "member='PropertiesChanged',"
                                             "sender='"BLUEZ_DEST"',";

static const char *BLUEZ_MATCH_RULES[] = { BLUEZ_MATCH_OBJECTMANAGER, BLUEZ_MATCH_PROPERTIES };

/* Free up the watcher alongside any remaining events and initial devices and other associated resources. */
static void bluez_watcher_free( struct bluez_watcher_ctx *watcher )
{
    struct bluez_watcher_event *event1, *event2;
    struct bluez_init_entry *entry1, *entry2;

    if (watcher->init_device_list_call)
    {
        p_dbus_pending_call_cancel( watcher->init_device_list_call );
        p_dbus_pending_call_unref( watcher->init_device_list_call );
    }

    LIST_FOR_EACH_ENTRY_SAFE( entry1, entry2, &watcher->initial_radio_list, struct bluez_init_entry, entry )
    {
        list_remove( &entry1->entry );
        unix_name_free( (struct unix_name *)entry1->object.radio.radio.handle );
        free( entry1 );
    }

    LIST_FOR_EACH_ENTRY_SAFE( event1, event2, &watcher->event_list, struct bluez_watcher_event, entry )
    {
        list_remove( &event1->entry );
        switch (event1->event_type)
        {
        case BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_ADDED:
            unix_name_free( (struct unix_name *)event1->event.radio_added.radio.handle );
            break;
        case BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_REMOVED:
            unix_name_free( (struct unix_name *)event1->event.radio_removed.handle );
            break;
        case BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_PROPERTIES_CHANGED:
            unix_name_free( (struct unix_name *)event1->event.radio_props_changed.radio.handle );
            break;
        case BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_ADDED:
            unix_name_free( (struct unix_name *)event1->event.device_added.radio.handle );
            unix_name_free( (struct unix_name *)event1->event.device_added.device.handle );
            break;
        case BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_REMOVED:
            unix_name_free( (struct unix_name *)event1->event.device_removed.device.handle );
            break;
        }
        free( event1 );
    }

    free( watcher );
}

NTSTATUS bluez_watcher_init( void *connection, void **ctx )
{
    DBusError err;
    NTSTATUS status;
    DBusPendingCall *call;
    struct bluez_watcher_ctx *watcher_ctx =
        calloc( 1, sizeof( struct bluez_watcher_ctx ) );
    SIZE_T i;

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
    list_init( &watcher_ctx->initial_device_list );
    list_init( &watcher_ctx->event_list );

    /* The bluez_dbus_loop thread will free up the watcher when the disconnect message is processed (i.e,
     * dbus_connection_read_write_dispatch returns false). Using a free-function with dbus_connection_add_filter
     * is racy as the filter is removed from a different thread. */
    if (!p_dbus_connection_add_filter( connection, bluez_filter, watcher_ctx, NULL ))
    {
        p_dbus_pending_call_cancel( call );
        p_dbus_pending_call_unref( call );
        free( watcher_ctx );
        ERR( "Could not add DBus filter\n" );
        return STATUS_NO_MEMORY;
    }
    p_dbus_error_init( &err );
    for (i = 0; i < ARRAY_SIZE( BLUEZ_MATCH_RULES ); i++)
    {
        TRACE( "Adding DBus match rule %s\n", debugstr_a( BLUEZ_MATCH_RULES[i] ) );

        p_dbus_bus_add_match( connection, BLUEZ_MATCH_RULES[i], &err );
        if (p_dbus_error_is_set( &err ))
        {
            NTSTATUS status = bluez_dbus_error_to_ntstatus( &err );
            ERR( "Could not add DBus match %s: %s: %s\n", debugstr_a( BLUEZ_MATCH_RULES[i] ), debugstr_a( err.name ),
                 debugstr_a( err.message ) );
            p_dbus_pending_call_cancel( call );
            p_dbus_pending_call_unref( call );
            p_dbus_error_free( &err );
            free( watcher_ctx );
            return status;
        }
    }
    p_dbus_error_free( &err );
    *ctx = watcher_ctx;
    TRACE( "ctx=%p\n", ctx );
    return STATUS_SUCCESS;
}

void bluez_watcher_close( void *connection, void *ctx )
{
    SIZE_T i;
    for (i = 0; i < ARRAY_SIZE( BLUEZ_MATCH_RULES ); i++)
    {
        DBusError error;

        p_dbus_error_init( &error );
        p_dbus_bus_remove_match( connection, BLUEZ_MATCH_RULES[i], &error );
        if (p_dbus_error_is_set( &error ))
            ERR( "Could not remove DBus match %s: %s: %s", BLUEZ_MATCH_RULES[i],
                 debugstr_a( error.name ), debugstr_a( error.message ) );
        p_dbus_error_free( &error );
    }
    p_dbus_connection_remove_filter( connection, bluez_filter, ctx );
}

static NTSTATUS bluez_build_initial_device_lists( DBusMessage *reply, struct list *adapter_list,
                                                  struct list *device_list )
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
                const char *prop_name;
                DBusMessageIter variant;
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
                while ((prop_name = bluez_next_dict_entry( &prop_iter, &variant )))
                {
                    bluez_radio_prop_from_dict_entry(
                        prop_name, &variant, &init_device->object.radio.props,
                        &init_device->object.radio.props_mask, WINEBLUETOOTH_RADIO_ALL_PROPERTIES );
                }
                init_device->object.radio.radio.handle = (UINT_PTR)radio_name;
                list_add_tail( adapter_list, &init_device->entry );
                TRACE( "Found BlueZ org.bluez.Adapter1 object %s: %p\n",
                       debugstr_a( radio_name->str ), radio_name );
                break;
            }
            else if (!strcmp( iface, BLUEZ_INTERFACE_DEVICE ))
            {
                const char *prop_name;
                DBusMessageIter variant;
                struct bluez_init_entry *init_device;
                struct unix_name *device_name, *radio_name = NULL;

                init_device = calloc( 1, sizeof( *init_device ) );
                if (!init_device)
                {
                    status = STATUS_NO_MEMORY;
                    goto done;
                }
                device_name = unix_name_get_or_create( path );
                if (!device_name)
                {
                    free( init_device );
                    status = STATUS_NO_MEMORY;
                    goto done;
                }
                init_device->object.device.device.handle = (UINT_PTR)device_name;

                while((prop_name = bluez_next_dict_entry( &prop_iter, &variant )))
                {
                    if (!strcmp( prop_name, "Adapter" ) &&
                        p_dbus_message_iter_get_arg_type( &variant ) == DBUS_TYPE_OBJECT_PATH)
                    {
                        const char *path;
                        p_dbus_message_iter_get_basic( &variant, &path );
                        radio_name = unix_name_get_or_create( path );
                        if (!radio_name)
                        {
                            unix_name_free( device_name );
                            free( init_device );
                            status = STATUS_NO_MEMORY;
                            goto done;
                        }
                        init_device->object.device.radio.handle = (UINT_PTR)radio_name;
                    }
                    else
                        bluez_device_prop_from_dict_entry( prop_name, &variant, &init_device->object.device.props,
                                                           &init_device->object.device.known_props_mask,
                                                           WINEBLUETOOTH_DEVICE_ALL_PROPERTIES );
                }
                if (!init_device->object.device.radio.handle)
                {
                    unix_name_free( device_name );
                    free( init_device );
                    ERR( "Could not find the associated adapter for device %s\n", debugstr_a( path ) );
                    break;
                }
                list_add_tail( device_list, &init_device->entry );
                TRACE( "Found BlueZ org.bluez.Device1 object %s: %p\n", debugstr_a( path ), device_name );
                break;
            }
        }
    }

    TRACE( "Initial device list: radios: %d, devices: %d\n", list_count( adapter_list ), list_count( device_list ) );
 done:
    return status;
}

static BOOL bluez_watcher_event_queue_ready( struct bluez_watcher_ctx *ctx, struct winebluetooth_watcher_event *event )
{
    if (!list_empty( &ctx->initial_radio_list ))
    {
        struct bluez_init_entry *radio;

        radio = LIST_ENTRY( list_head( &ctx->initial_radio_list ), struct bluez_init_entry, entry );
        event->event_type = BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_ADDED;
        event->event_data.radio_added = radio->object.radio;
        list_remove( &radio->entry );
        free( radio );
        return TRUE;
    }
    if (!list_empty( &ctx->initial_device_list ))
    {
        struct bluez_init_entry *device;

        device = LIST_ENTRY( list_head( &ctx->initial_device_list ), struct bluez_init_entry, entry );
        event->event_type = BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_ADDED;
        event->event_data.device_added = device->object.device;
        list_remove( &device->entry );
        free( device );
        return TRUE;
    }
    if (!list_empty( &ctx->event_list ))
    {
        struct bluez_watcher_event *watcher_event =
            LIST_ENTRY( list_head( &ctx->event_list ), struct bluez_watcher_event, entry );

        if (watcher_event->pending_call && !p_dbus_pending_call_get_completed( watcher_event->pending_call ))
            return FALSE;

        event->event_type = watcher_event->event_type;
        event->event_data = watcher_event->event;
        list_remove( &watcher_event->entry );
        if (watcher_event->pending_call)
            p_dbus_pending_call_unref( watcher_event->pending_call );
        free( watcher_event );
        return TRUE;
    }
    return FALSE;
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
        if (bluez_watcher_event_queue_ready( watcher_ctx, &result->data.watcher_event ))
        {
            result->status = WINEBLUETOOTH_EVENT_WATCHER_EVENT;
            p_dbus_connection_unref( connection );
            return STATUS_PENDING;
        }
        else if (!p_dbus_connection_read_write_dispatch( connection, 100 ))
        {
            bluez_watcher_free( watcher_ctx );
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
                WARN( "Error getting object list from BlueZ: '%s': '%s'\n", error.name,
                      error.message );
                p_dbus_error_free( &error );
                p_dbus_message_unref( reply );
                p_dbus_connection_unref( connection );
                return STATUS_NO_MEMORY;
            }
            status = bluez_build_initial_device_lists( reply, &watcher_ctx->initial_radio_list,
                                                       &watcher_ctx->initial_device_list );
            p_dbus_message_unref( reply );
            if (status != STATUS_SUCCESS)
            {
                WARN( "Error building initial bluetooth devices list: %#x\n", (int)status );
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
void bluez_watcher_close( void *connection, void *ctx ) {}
NTSTATUS bluez_dbus_loop( void *c, void *watcher, struct winebluetooth_event *result )
{
    return STATUS_NOT_SUPPORTED;
}
NTSTATUS bluez_adapter_set_prop( void *connection, struct bluetooth_adapter_set_prop_params *params )
{
    return STATUS_NOT_SUPPORTED;
}

#endif /* SONAME_LIBDBUS_1 */
