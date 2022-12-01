/*
 * DBus devices support
 *
 * Copyright 2006, 2011 Alexandre Julliard
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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef SONAME_LIBDBUS_1
# include <dbus/dbus.h>
#endif

#include "mountmgr.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "dhcpcsdk.h"
#include "unixlib.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#ifdef SONAME_LIBDBUS_1

#define DBUS_FUNCS               \
    DO_FUNC(dbus_bus_add_match); \
    DO_FUNC(dbus_bus_get); \
    DO_FUNC(dbus_bus_get_private); \
    DO_FUNC(dbus_bus_remove_match); \
    DO_FUNC(dbus_connection_add_filter); \
    DO_FUNC(dbus_connection_read_write_dispatch); \
    DO_FUNC(dbus_connection_remove_filter); \
    DO_FUNC(dbus_connection_send_with_reply_and_block); \
    DO_FUNC(dbus_error_free); \
    DO_FUNC(dbus_error_init); \
    DO_FUNC(dbus_error_is_set); \
    DO_FUNC(dbus_free_string_array); \
    DO_FUNC(dbus_message_get_args); \
    DO_FUNC(dbus_message_get_interface); \
    DO_FUNC(dbus_message_get_member); \
    DO_FUNC(dbus_message_get_path); \
    DO_FUNC(dbus_message_get_type); \
    DO_FUNC(dbus_message_is_signal); \
    DO_FUNC(dbus_message_iter_append_basic); \
    DO_FUNC(dbus_message_iter_get_arg_type); \
    DO_FUNC(dbus_message_iter_get_basic); \
    DO_FUNC(dbus_message_iter_get_fixed_array); \
    DO_FUNC(dbus_message_iter_init); \
    DO_FUNC(dbus_message_iter_init_append); \
    DO_FUNC(dbus_message_iter_next); \
    DO_FUNC(dbus_message_iter_recurse); \
    DO_FUNC(dbus_message_new_method_call); \
    DO_FUNC(dbus_message_unref)

#define DO_FUNC(f) static typeof(f) * p_##f
DBUS_FUNCS;
#undef DO_FUNC

static int udisks_timeout = -1;
static DBusConnection *connection;

static inline int starts_with( const char *str, const char *prefix )
{
    return !strncmp( str, prefix, strlen(prefix) );
}

static GUID *parse_uuid( GUID *guid, const char *str )
{
    /* standard uuid format */
    if (strlen(str) == 36 && str[8] == '-' && str[13] == '-' && str[18] == '-' && str[23] == '-')
    {
        int i;
        unsigned char *out = guid->Data4;

        if (sscanf( str, "%x-%hx-%hx-", (int *)&guid->Data1, &guid->Data2, &guid->Data3 ) != 3) return NULL;
        for (i = 19; i < 36; i++)
        {
            unsigned char val;

            if (i == 23) continue;

            if      (str[i] >= '0' && str[i] <= '9') val = str[i] - '0';
            else if (str[i] >= 'a' && str[i] <= 'f') val = str[i] - 'a' + 10;
            else if (str[i] >= 'A' && str[i] <= 'F') val = str[i] - 'A' + 10;
            else return NULL;
            val <<= 4;
            i++;

            if      (str[i] >= '0' && str[i] <= '9') val += str[i] - '0';
            else if (str[i] >= 'a' && str[i] <= 'f') val += str[i] - 'a' + 10;
            else if (str[i] >= 'A' && str[i] <= 'F') val += str[i] - 'A' + 10;
            else return NULL;
            *out++ = val;
        }
        return guid;
    }

    /* check for xxxx-xxxx format (FAT serial number) */
    if (strlen(str) == 9 && str[4] == '-')
    {
        memset( guid, 0, sizeof(*guid) );
        if (sscanf( str, "%hx-%hx", &guid->Data2, &guid->Data3 ) == 2) return guid;
    }
    return NULL;
}

static BOOL load_dbus_functions(void)
{
    void *handle;

    if (!(handle = dlopen( SONAME_LIBDBUS_1, RTLD_NOW )))
        goto failed;

#define DO_FUNC(f) if (!(p_##f = dlsym( handle, #f ))) goto failed
    DBUS_FUNCS;
#undef DO_FUNC
    return TRUE;

failed:
    WARN( "failed to load DBUS support: %s\n", dlerror() );
    return FALSE;
}

static const char *udisks_next_dict_entry( DBusMessageIter *iter, DBusMessageIter *variant )
{
    DBusMessageIter sub;
    const char *name;

    if (p_dbus_message_iter_get_arg_type( iter ) != DBUS_TYPE_DICT_ENTRY) return NULL;
    p_dbus_message_iter_recurse( iter, &sub );
    p_dbus_message_iter_next( iter );
    p_dbus_message_iter_get_basic( &sub, &name );
    p_dbus_message_iter_next( &sub );
    p_dbus_message_iter_recurse( &sub, variant );
    return name;
}

static enum device_type udisks_parse_media_compatibility( DBusMessageIter *iter )
{
    DBusMessageIter media;
    enum device_type drive_type = DEVICE_UNKNOWN;

    p_dbus_message_iter_recurse( iter, &media );
    while (p_dbus_message_iter_get_arg_type( &media ) == DBUS_TYPE_STRING)
    {
        const char *media_type;
        p_dbus_message_iter_get_basic( &media, &media_type );
        if (starts_with( media_type, "optical_dvd" ))
            drive_type = DEVICE_DVD;
        if (starts_with( media_type, "floppy" ))
            drive_type = DEVICE_FLOPPY;
        else if (starts_with( media_type, "optical_" ) && drive_type == DEVICE_UNKNOWN)
            drive_type = DEVICE_CDROM;
        p_dbus_message_iter_next( &media );
    }
    return drive_type;
}

/* UDisks callback for new device */
static void udisks_new_device( const char *udi )
{
    static const char *dev_name = "org.freedesktop.UDisks.Device";
    DBusMessage *request, *reply;
    DBusMessageIter iter, variant;
    DBusError error;
    const char *device = NULL;
    const char *mount_point = NULL;
    const char *type = NULL;
    GUID guid, *guid_ptr = NULL;
    int removable = FALSE;
    enum device_type drive_type = DEVICE_UNKNOWN;

    request = p_dbus_message_new_method_call( "org.freedesktop.UDisks", udi,
                                              "org.freedesktop.DBus.Properties", "GetAll" );
    if (!request) return;

    p_dbus_message_iter_init_append( request, &iter );
    p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &dev_name );

    p_dbus_error_init( &error );
    reply = p_dbus_connection_send_with_reply_and_block( connection, request, -1, &error );
    p_dbus_message_unref( request );
    if (!reply)
    {
        WARN( "failed: %s\n", error.message );
        p_dbus_error_free( &error );
        return;
    }
    p_dbus_error_free( &error );

    p_dbus_message_iter_init( reply, &iter );
    if (p_dbus_message_iter_get_arg_type( &iter ) == DBUS_TYPE_ARRAY)
    {
        const char *name;

        p_dbus_message_iter_recurse( &iter, &iter );
        while ((name = udisks_next_dict_entry( &iter, &variant )))
        {
            if (!strcmp( name, "DeviceFile" ))
                p_dbus_message_iter_get_basic( &variant, &device );
            else if (!strcmp( name, "DeviceIsRemovable" ))
                p_dbus_message_iter_get_basic( &variant, &removable );
            else if (!strcmp( name, "IdType" ))
                p_dbus_message_iter_get_basic( &variant, &type );
            else if (!strcmp( name, "DriveMediaCompatibility" ))
                drive_type = udisks_parse_media_compatibility( &variant );
            else if (!strcmp( name, "DeviceMountPaths" ))
            {
                DBusMessageIter paths;
                p_dbus_message_iter_recurse( &variant, &paths );
                if (p_dbus_message_iter_get_arg_type( &paths ) == DBUS_TYPE_STRING)
                    p_dbus_message_iter_get_basic( &paths, &mount_point );
            }
            else if (!strcmp( name, "IdUuid" ))
            {
                char *uuid_str;
                p_dbus_message_iter_get_basic( &variant, &uuid_str );
                guid_ptr = parse_uuid( &guid, uuid_str );
            }
        }
    }

    TRACE( "udi %s device %s mount point %s uuid %s type %s removable %u\n",
           debugstr_a(udi), debugstr_a(device), debugstr_a(mount_point),
           debugstr_guid(guid_ptr), debugstr_a(type), removable );

    if (type)
    {
        if (!strcmp( type, "iso9660" ))
        {
            removable = TRUE;
            drive_type = DEVICE_CDROM;
        }
        else if (!strcmp( type, "udf" ))
        {
            removable = TRUE;
            drive_type = DEVICE_DVD;
        }
    }

    if (device)
    {
        if (removable) queue_device_op( ADD_DOS_DEVICE, udi, device, mount_point, drive_type, guid_ptr, NULL, NULL );
        else if (guid_ptr) queue_device_op( ADD_VOLUME, udi, device, mount_point, DEVICE_HARDDISK_VOL, guid_ptr, NULL, NULL );
    }

    p_dbus_message_unref( reply );
}

/* UDisks callback for removed device */
static void udisks_removed_device( const char *udi )
{
    TRACE( "removed %s\n", wine_dbgstr_a(udi) );
    queue_device_op( REMOVE_DEVICE, udi, NULL, NULL, 0, NULL, NULL, NULL );
}

/* UDisks callback for changed device */
static void udisks_changed_device( const char *udi )
{
    udisks_new_device( udi );
}

static BOOL udisks_enumerate_devices(void)
{
    DBusMessage *request, *reply;
    DBusError error;
    char **paths;
    int i, count;

    request = p_dbus_message_new_method_call( "org.freedesktop.UDisks", "/org/freedesktop/UDisks",
                                              "org.freedesktop.UDisks", "EnumerateDevices" );
    if (!request) return FALSE;

    p_dbus_error_init( &error );
    reply = p_dbus_connection_send_with_reply_and_block( connection, request, udisks_timeout, &error );
    p_dbus_message_unref( request );
    if (!reply)
    {
        WARN( "failed: %s\n", error.message );
        p_dbus_error_free( &error );
        return FALSE;
    }
    p_dbus_error_free( &error );

    if (p_dbus_message_get_args( reply, &error, DBUS_TYPE_ARRAY,
                                 DBUS_TYPE_OBJECT_PATH, &paths, &count, DBUS_TYPE_INVALID ))
    {
        for (i = 0; i < count; i++) udisks_new_device( paths[i] );
        p_dbus_free_string_array( paths );
    }
    else WARN( "unexpected args in EnumerateDevices reply\n" );

    p_dbus_message_unref( reply );
    return TRUE;
}

/* to make things easier, UDisks2 stores strings as array of bytes instead of strings... */
static const char *udisks2_string_from_array( DBusMessageIter *iter )
{
    DBusMessageIter string;
    const char *array;
    int size;

    p_dbus_message_iter_recurse( iter, &string );
    p_dbus_message_iter_get_fixed_array( &string, &array, &size );
    return array;
}

/* find the drive entry in the dictionary and get its parameters */
static void udisks2_get_drive_info( const char *drive_name, DBusMessageIter *dict,
                                    enum device_type *drive_type, int *removable, const char **serial )
{
    DBusMessageIter iter, drive, variant;
    const char *name;

    p_dbus_message_iter_recurse( dict, &iter );
    while ((name = udisks_next_dict_entry( &iter, &drive )))
    {
        if (strcmp( name, drive_name )) continue;
        while ((name = udisks_next_dict_entry( &drive, &iter )))
        {
            if (strcmp( name, "org.freedesktop.UDisks2.Drive" )) continue;
            while ((name = udisks_next_dict_entry( &iter, &variant )))
            {
                if (!strcmp( name, "Removable" ))
                    p_dbus_message_iter_get_basic( &variant, removable );
                else if (!strcmp( name, "MediaCompatibility" ))
                    *drive_type = udisks_parse_media_compatibility( &variant );
                else if (!strcmp( name, "Id" ))
                    p_dbus_message_iter_get_basic( &variant, serial );
            }
        }
    }
}

static void udisks2_add_device( const char *udi, DBusMessageIter *dict, DBusMessageIter *block )
{
    DBusMessageIter iter, variant, paths, string;
    const char *device = NULL;
    const char *mount_point = NULL;
    const char *type = NULL;
    const char *drive = NULL;
    const char *id = NULL;
    GUID guid, *guid_ptr = NULL;
    const char *iface, *name;
    int removable = FALSE;
    enum device_type drive_type = DEVICE_UNKNOWN;

    while ((iface = udisks_next_dict_entry( block, &iter )))
    {
        if (!strcmp( iface, "org.freedesktop.UDisks2.Filesystem" ))
        {
            while ((name = udisks_next_dict_entry( &iter, &variant )))
            {
                if (!strcmp( name, "MountPoints" ))
                {
                    p_dbus_message_iter_recurse( &variant, &paths );
                    if (p_dbus_message_iter_get_arg_type( &paths ) == DBUS_TYPE_ARRAY)
                    {
                        p_dbus_message_iter_recurse( &variant, &string );
                        mount_point = udisks2_string_from_array( &string );
                    }
                }
            }
        }
        if (!strcmp( iface, "org.freedesktop.UDisks2.Block" ))
        {
            while ((name = udisks_next_dict_entry( &iter, &variant )))
            {
                if (!strcmp( name, "Device" ))
                    device = udisks2_string_from_array( &variant );
                else if (!strcmp( name, "IdType" ))
                    p_dbus_message_iter_get_basic( &variant, &type );
                else if (!strcmp( name, "Drive" ))
                {
                    p_dbus_message_iter_get_basic( &variant, &drive );
                    udisks2_get_drive_info( drive, dict, &drive_type, &removable, &id );
                }
                else if (!strcmp( name, "IdUUID" ))
                {
                    const char *uuid_str;
                    if (p_dbus_message_iter_get_arg_type( &variant ) == DBUS_TYPE_ARRAY)
                        uuid_str = udisks2_string_from_array( &variant );
                    else
                        p_dbus_message_iter_get_basic( &variant, &uuid_str );
                    guid_ptr = parse_uuid( &guid, uuid_str );
                }
            }
        }
    }

    TRACE( "udi %s device %s mount point %s uuid %s type %s removable %u\n",
           debugstr_a(udi), debugstr_a(device), debugstr_a(mount_point),
           debugstr_guid(guid_ptr), debugstr_a(type), removable );

    if (type)
    {
        if (!strcmp( type, "iso9660" ))
        {
            removable = TRUE;
            drive_type = DEVICE_CDROM;
        }
        else if (!strcmp( type, "udf" ))
        {
            removable = TRUE;
            drive_type = DEVICE_DVD;
        }
    }
    if (device)
    {
        if (removable) queue_device_op( ADD_DOS_DEVICE, udi, device, mount_point, drive_type, guid_ptr, id, NULL );
        else if (guid_ptr) queue_device_op( ADD_VOLUME, udi, device, mount_point, DEVICE_HARDDISK_VOL, guid_ptr, id, NULL );
    }
}

/* UDisks2 is almost, but not quite, entirely unlike UDisks.
 * It would have been easy to make it backwards compatible, but where would be the fun in that?
 */
static BOOL udisks2_add_devices( const char *changed )
{
    DBusMessage *request, *reply;
    DBusMessageIter dict, iter, block;
    DBusError error;
    const char *udi;

    request = p_dbus_message_new_method_call( "org.freedesktop.UDisks2", "/org/freedesktop/UDisks2",
                                              "org.freedesktop.DBus.ObjectManager", "GetManagedObjects" );
    if (!request) return FALSE;

    p_dbus_error_init( &error );
    reply = p_dbus_connection_send_with_reply_and_block( connection, request, udisks_timeout, &error );
    p_dbus_message_unref( request );
    if (!reply)
    {
        WARN( "failed: %s\n", error.message );
        p_dbus_error_free( &error );
        return FALSE;
    }
    p_dbus_error_free( &error );

    p_dbus_message_iter_init( reply, &dict );
    if (p_dbus_message_iter_get_arg_type( &dict ) == DBUS_TYPE_ARRAY)
    {
        p_dbus_message_iter_recurse( &dict, &iter );
        while ((udi = udisks_next_dict_entry( &iter, &block )))
        {
            if (!starts_with( udi, "/org/freedesktop/UDisks2/block_devices/" )) continue;
            if (changed && strcmp( changed, udi )) continue;
            udisks2_add_device( udi, &dict, &block );
        }
    }
    else WARN( "unexpected args in GetManagedObjects reply\n" );

    p_dbus_message_unref( reply );
    return TRUE;
}

static DBusHandlerResult udisks_filter( DBusConnection *ctx, DBusMessage *msg, void *user_data )
{
    char *path;
    DBusError error;

    p_dbus_error_init( &error );

    /* udisks signals */
    if (p_dbus_message_is_signal( msg, "org.freedesktop.UDisks", "DeviceAdded" ) &&
        p_dbus_message_get_args( msg, &error, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID ))
    {
        udisks_new_device( path );
    }
    else if (p_dbus_message_is_signal( msg, "org.freedesktop.UDisks", "DeviceRemoved" ) &&
             p_dbus_message_get_args( msg, &error, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID ))
    {
        udisks_removed_device( path );
    }
    else if (p_dbus_message_is_signal( msg, "org.freedesktop.UDisks", "DeviceChanged" ) &&
             p_dbus_message_get_args( msg, &error, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID ))
    {
        udisks_changed_device( path );
    }
    /* udisks2 signals */
    else if (p_dbus_message_is_signal( msg, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded" ) &&
             p_dbus_message_get_args( msg, &error, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID ))
    {
        TRACE( "added %s\n", wine_dbgstr_a(path) );
        udisks2_add_devices( path );
    }
    else if (p_dbus_message_is_signal( msg, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved" ) &&
             p_dbus_message_get_args( msg, &error, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID ))
    {
        udisks_removed_device( path );
    }
    else if (p_dbus_message_is_signal( msg, "org.freedesktop.DBus.Properties", "PropertiesChanged" ))
    {
        const char *udi = p_dbus_message_get_path( msg );
        TRACE( "changed %s\n", wine_dbgstr_a(udi) );
        udisks2_add_devices( udi );
    }
    else TRACE( "ignoring message type=%d path=%s interface=%s method=%s\n",
                p_dbus_message_get_type( msg ), p_dbus_message_get_path( msg ),
                p_dbus_message_get_interface( msg ), p_dbus_message_get_member( msg ) );

    p_dbus_error_free( &error );
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void run_dbus_loop(void)
{
    static const char udisks_match[] = "type='signal',"
                                       "interface='org.freedesktop.UDisks',"
                                       "sender='org.freedesktop.UDisks'";
    static const char udisks2_match_interfaces[] = "type='signal',"
                                                   "interface='org.freedesktop.DBus.ObjectManager',"
                                                   "path='/org/freedesktop/UDisks2'";
    static const char udisks2_match_properties[] = "type='signal',"
                                                   "interface='org.freedesktop.DBus.Properties'";


    DBusError error;

    if (!load_dbus_functions()) return;

    p_dbus_error_init( &error );
    if (!(connection = p_dbus_bus_get( DBUS_BUS_SYSTEM, &error )))
    {
        WARN( "failed to get system dbus connection: %s\n", error.message );
        p_dbus_error_free( &error );
        return;
    }

    /* first try UDisks2 */

    p_dbus_connection_add_filter( connection, udisks_filter, NULL, NULL );
    p_dbus_bus_add_match( connection, udisks2_match_interfaces, &error );
    p_dbus_bus_add_match( connection, udisks2_match_properties, &error );
    if (udisks2_add_devices( NULL )) goto found;
    p_dbus_bus_remove_match( connection, udisks2_match_interfaces, &error );
    p_dbus_bus_remove_match( connection, udisks2_match_properties, &error );

    /* then try UDisks */

    p_dbus_bus_add_match( connection, udisks_match, &error );
    if (udisks_enumerate_devices()) goto found;
    p_dbus_bus_remove_match( connection, udisks_match, &error );
    p_dbus_connection_remove_filter( connection, udisks_filter, NULL );

found:
    while (p_dbus_connection_read_write_dispatch( connection, -1 )) /* nothing */ ;
}

#if !defined(__APPLE__)

/* The udisks dispatch loop will block all threads using the same connection, so we'll
   use a private connection. Multiple threads can make methods calls at the same time
   on the same connection, according to the documentation.
 */
static DBusConnection *dhcp_connection;
static DBusConnection *get_dhcp_connection(void)
{
    if (!dhcp_connection)
    {
        DBusError error;
        p_dbus_error_init( &error );
        if (!(dhcp_connection = p_dbus_bus_get_private( DBUS_BUS_SYSTEM, &error )))
        {
            WARN( "failed to get system dbus connection: %s\n", error.message );
            p_dbus_error_free( &error );
        }
    }
    return dhcp_connection;
}

static DBusMessage *device_by_iface_request( const char *iface )
{
    DBusMessage *request, *reply;
    DBusMessageIter iter;
    DBusError error;
    DBusConnection *connection = get_dhcp_connection();

    if (!connection) return NULL;

    request = p_dbus_message_new_method_call( "org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
                                              "org.freedesktop.NetworkManager", "GetDeviceByIpIface" );
    if (!request) return NULL;

    p_dbus_message_iter_init_append( request, &iter );
    p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &iface );

    p_dbus_error_init( &error );
    reply = p_dbus_connection_send_with_reply_and_block( connection, request, -1, &error );
    p_dbus_message_unref( request );
    if (!reply)
    {
        WARN( "failed: %s\n", error.message );
        p_dbus_error_free( &error );
        return NULL;
    }

    p_dbus_error_free( &error );
    return reply;
}

static DBusMessage *dhcp4_config_request( const char *iface )
{
    static const char *device = "org.freedesktop.NetworkManager.Device";
    static const char *dhcp4_config = "Dhcp4Config";
    DBusMessage *request, *reply;
    DBusMessageIter iter;
    DBusError error;
    const char *path = NULL;

    if (!(reply = device_by_iface_request( iface ))) return NULL;

    p_dbus_message_iter_init( reply, &iter );
    if (p_dbus_message_iter_get_arg_type( &iter ) == DBUS_TYPE_OBJECT_PATH) p_dbus_message_iter_get_basic( &iter, &path );
    if (!path)
    {
        p_dbus_message_unref( reply );
        return NULL;
    }

    request = p_dbus_message_new_method_call( "org.freedesktop.NetworkManager", path,
                                              "org.freedesktop.DBus.Properties", "Get" );
    p_dbus_message_unref( reply );
    if (!request) return NULL;

    p_dbus_message_iter_init_append( request, &iter );
    p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &device );
    p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &dhcp4_config );

    p_dbus_error_init( &error );
    reply = p_dbus_connection_send_with_reply_and_block( get_dhcp_connection(), request, -1, &error );
    p_dbus_message_unref( request );
    if (!reply)
    {
        WARN( "failed: %s\n", error.message );
        p_dbus_error_free( &error );
        return NULL;
    }

    p_dbus_error_free( &error );
    return reply;
}

static DBusMessage *dhcp4_config_options_request( const char *unix_name )
{
    static const char *dhcp4_config = "org.freedesktop.NetworkManager.DHCP4Config";
    static const char *options = "Options";
    DBusMessage *request, *reply;
    DBusMessageIter iter, sub;
    DBusError error;
    const char *path = NULL;

    if (!(reply = dhcp4_config_request( unix_name ))) return NULL;

    p_dbus_message_iter_init( reply, &iter );
    if (p_dbus_message_iter_get_arg_type( &iter ) == DBUS_TYPE_VARIANT)
    {
        p_dbus_message_iter_recurse( &iter, &sub );
        p_dbus_message_iter_get_basic( &sub, &path );
    }
    if (!path)
    {
        p_dbus_message_unref( reply );
        return NULL;
    }

    request = p_dbus_message_new_method_call( "org.freedesktop.NetworkManager", path,
                                              "org.freedesktop.DBus.Properties", "Get" );
    p_dbus_message_unref( reply );
    if (!request) return NULL;

    p_dbus_message_iter_init_append( request, &iter );
    p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &dhcp4_config );
    p_dbus_message_iter_append_basic( &iter, DBUS_TYPE_STRING, &options );

    p_dbus_error_init( &error );
    reply = p_dbus_connection_send_with_reply_and_block( get_dhcp_connection(), request, -1, &error );
    p_dbus_message_unref( request );
    if (!reply)
    {
        p_dbus_error_free( &error );
        return NULL;
    }

    p_dbus_error_free( &error );
    return reply;
}

static const char *dhcp4_config_option_next_dict_entry( DBusMessageIter *iter, DBusMessageIter *variant )
{
    DBusMessageIter sub;
    const char *name;

    if (p_dbus_message_iter_get_arg_type( iter ) != DBUS_TYPE_DICT_ENTRY) return NULL;
    p_dbus_message_iter_recurse( iter, &sub );
    p_dbus_message_iter_next( iter );
    p_dbus_message_iter_get_basic( &sub, &name );
    p_dbus_message_iter_next( &sub );
    p_dbus_message_iter_recurse( &sub, variant );
    return name;
}

static DBusMessage *dhcp4_config_option_request( const char *unix_name, const char *option, const char **value )
{
    DBusMessage *reply;
    DBusMessageIter iter, variant;
    const char *name;

    if (!(reply = dhcp4_config_options_request( unix_name ))) return NULL;

    *value = NULL;
    p_dbus_message_iter_init( reply, &iter );
    if (p_dbus_message_iter_get_arg_type( &iter ) == DBUS_TYPE_VARIANT)
    {
        p_dbus_message_iter_recurse( &iter, &iter );
        if (p_dbus_message_iter_get_arg_type( &iter ) == DBUS_TYPE_ARRAY)
        {
            p_dbus_message_iter_recurse( &iter, &iter );
            while ((name = dhcp4_config_option_next_dict_entry( &iter, &variant )))
            {
                if (!strcmp( name, option ))
                {
                    p_dbus_message_iter_get_basic( &variant, value );
                    break;
                }
            }
        }
    }

    return reply;
}

static const char *map_option( unsigned option )
{
    switch (option)
    {
    case OPTION_SUBNET_MASK:         return "subnet_mask";
    case OPTION_ROUTER_ADDRESS:      return "next_server";
    case OPTION_HOST_NAME:           return "host_name";
    case OPTION_DOMAIN_NAME:         return "domain_name";
    case OPTION_BROADCAST_ADDRESS:   return "broadcast_address";
    case OPTION_MSFT_IE_PROXY:       return "wpad";
    default:
        FIXME( "unhandled option %u\n", option );
        return "";
    }
}

NTSTATUS dhcp_request( void *args )
{
    const struct dhcp_request_params *params = args;
    DBusMessage *reply;
    const char *value;
    ULONG ret = 0;

    params->req->offset = params->req->size = 0;
    *params->ret_size = 0;

    if (!(reply = dhcp4_config_option_request( params->unix_name, map_option(params->req->id), &value ))) return STATUS_SUCCESS;

    switch (params->req->id)
    {
    case OPTION_SUBNET_MASK:
    case OPTION_ROUTER_ADDRESS:
    case OPTION_BROADCAST_ADDRESS:
    {
        IN_ADDR *ptr = (IN_ADDR *)(params->buffer + params->offset);
        if (value && params->size >= sizeof(IN_ADDR))
        {
            ptr->S_un.S_addr = inet_addr( value );
            params->req->offset = params->offset;
            params->req->size   = sizeof(*ptr);
            TRACE( "returning %08x\n", *(unsigned int*)ptr );
        }
        ret = sizeof(*ptr);
        break;
    }
    case OPTION_HOST_NAME:
    case OPTION_DOMAIN_NAME:
    case OPTION_MSFT_IE_PROXY:
    {
        char *ptr = params->buffer + params->offset;
        int len = value ? strlen( value ) : 0;
        if (len && params->size >= len)
        {
            memcpy( ptr, value, len );
            params->req->offset = params->offset;
            params->req->size   = len;
            TRACE( "returning %s\n", debugstr_an(ptr, len) );
        }
        ret = len;
        break;
    }
    default:
        FIXME( "option %u not supported\n", (unsigned int)params->req->id );
        break;
    }

    p_dbus_message_unref( reply );
    *params->ret_size = ret;
    return STATUS_SUCCESS;
}
#endif

#else  /* SONAME_LIBDBUS_1 */

void run_dbus_loop(void)
{
    TRACE( "Skipping, DBUS support not compiled in\n" );
}

#endif  /* SONAME_LIBDBUS_1 */
