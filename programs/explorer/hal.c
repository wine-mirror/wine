/*
 * HAL devices support
 *
 * Copyright 2006 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "dbt.h"
#include "excpt.h"

#include "wine/library.h"
#include "wine/list.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#ifdef HAVE_LIBHAL

#include <dbus/dbus.h>
#include <hal/libhal.h>

struct dos_drive
{
    struct list entry;
    char *udi;
    int   drive;
};

static struct list drives_list = LIST_INIT(drives_list);


#define DBUS_FUNCS \
    DO_FUNC(dbus_bus_get); \
    DO_FUNC(dbus_connection_close); \
    DO_FUNC(dbus_connection_read_write_dispatch); \
    DO_FUNC(dbus_error_init); \
    DO_FUNC(dbus_error_free); \
    DO_FUNC(dbus_error_is_set)

#define HAL_FUNCS \
    DO_FUNC(libhal_ctx_free); \
    DO_FUNC(libhal_ctx_init); \
    DO_FUNC(libhal_ctx_new); \
    DO_FUNC(libhal_ctx_set_dbus_connection); \
    DO_FUNC(libhal_ctx_set_device_added); \
    DO_FUNC(libhal_ctx_set_device_property_modified); \
    DO_FUNC(libhal_ctx_set_device_removed); \
    DO_FUNC(libhal_ctx_shutdown); \
    DO_FUNC(libhal_device_get_property_bool); \
    DO_FUNC(libhal_device_get_property_string); \
    DO_FUNC(libhal_device_add_property_watch); \
    DO_FUNC(libhal_device_remove_property_watch); \
    DO_FUNC(libhal_free_string); \
    DO_FUNC(libhal_free_string_array); \
    DO_FUNC(libhal_get_all_devices)

#define DO_FUNC(f) static typeof(f) * p_##f
DBUS_FUNCS;
HAL_FUNCS;
#undef DO_FUNC

static BOOL load_functions(void)
{
    void *dbus_handle, *hal_handle;
    char error[128];

    if (!(dbus_handle = wine_dlopen(SONAME_LIBDBUS_1, RTLD_NOW, error, sizeof(error)))) goto failed;
    if (!(hal_handle = wine_dlopen(SONAME_LIBHAL, RTLD_NOW, error, sizeof(error)))) goto failed;

#define DO_FUNC(f) if (!(p_##f = wine_dlsym( dbus_handle, #f, error, sizeof(error) ))) goto failed
    DBUS_FUNCS;
#undef DO_FUNC

#define DO_FUNC(f) if (!(p_##f = wine_dlsym( hal_handle, #f, error, sizeof(error) ))) goto failed
    HAL_FUNCS;
#undef DO_FUNC

    return TRUE;

failed:
    WINE_WARN( "failed to load HAL support: %s\n", error );
    return FALSE;
}

static WINE_EXCEPTION_FILTER(assert_fault)
{
    if (GetExceptionCode() == EXCEPTION_WINE_ASSERTION) return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/* send notification about a change to a given drive */
static void send_notify( int drive, int code )
{
    DWORD_PTR result;
    DEV_BROADCAST_VOLUME info;

    info.dbcv_size       = sizeof(info);
    info.dbcv_devicetype = DBT_DEVTYP_VOLUME;
    info.dbcv_reserved   = 0;
    info.dbcv_unitmask   = 1 << drive;
    info.dbcv_flags      = DBTF_MEDIA;
    SendMessageTimeoutW( HWND_BROADCAST, WM_DEVICECHANGE, code, (LPARAM)&info,
                         SMTO_ABORTIFHUNG, 0, &result );
}

static char *get_dosdevices_path(void)
{
    const char *config_dir = wine_get_config_dir();
    size_t len = strlen(config_dir) + sizeof("/dosdevices/a::");
    char *path = HeapAlloc( GetProcessHeap(), 0, len );
    if (path)
    {
        strcpy( path, config_dir );
        strcat( path, "/dosdevices/a::" );
    }
    return path;
}

/* find or create a DOS drive for the corresponding device */
static int add_drive( const char *device, const char *type )
{
    char *path, *p;
    char in_use[26];
    struct stat dev_st, drive_st;
    int drive, first, last, avail = 0;

    if (stat( device, &dev_st ) == -1 || !S_ISBLK( dev_st.st_mode )) return -1;

    if (!(path = get_dosdevices_path())) return -1;
    p = path + strlen(path) - 3;

    memset( in_use, 0, sizeof(in_use) );

    first = 2;
    last = 26;
    if (type && !strcmp( type, "floppy" ))
    {
        first = 0;
        last = 2;
    }

    while (avail != -1)
    {
        avail = -1;
        for (drive = first; drive < last; drive++)
        {
            if (in_use[drive]) continue;  /* already checked */
            *p = 'a' + drive;
            if (stat( path, &drive_st ) == -1)
            {
                if (lstat( path, &drive_st ) == -1 && errno == ENOENT)  /* this is a candidate */
                {
                    if (avail == -1)
                    {
                        p[2] = 0;
                        /* if mount point symlink doesn't exist either, it's available */
                        if (lstat( path, &drive_st ) == -1 && errno == ENOENT) avail = drive;
                        p[2] = ':';
                    }
                }
                else in_use[drive] = 1;
            }
            else
            {
                in_use[drive] = 1;
                if (!S_ISBLK( drive_st.st_mode )) continue;
                if (dev_st.st_rdev == drive_st.st_rdev) goto done;
            }
        }
        if (avail != -1)
        {
            /* try to use the one we found */
            drive = avail;
            *p = 'a' + drive;
            if (symlink( device, path ) != -1) goto done;
            /* failed, retry the search */
        }
    }
    drive = -1;

done:
    HeapFree( GetProcessHeap(), 0, path );
    return drive;
}

static void set_mount_point( struct dos_drive *drive, const char *mount_point )
{
    char *path, *p;
    struct stat path_st, mnt_st;

    if (drive->drive == -1) return;
    if (!(path = get_dosdevices_path())) return;
    p = path + strlen(path) - 3;
    *p = 'a' + drive->drive;
    p[2] = 0;

    if (mount_point[0])
    {
        /* try to avoid unlinking if already set correctly */
        if (stat( path, &path_st ) == -1 || stat( mount_point, &mnt_st ) == -1 ||
            path_st.st_dev != mnt_st.st_dev || path_st.st_ino != mnt_st.st_ino)
        {
            unlink( path );
            symlink( mount_point, path );
        }
    }
    else unlink( path );

    HeapFree( GetProcessHeap(), 0, path );
}

static void add_dos_device( const char *udi, const char *device,
                            const char *mount_point, const char *type )
{
    struct dos_drive *drive;

    /* first check if it already exists */
    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        if (!strcmp( udi, drive->udi )) goto found;
    }

    if (!(drive = HeapAlloc( GetProcessHeap(), 0, sizeof(*drive) ))) return;
    if (!(drive->udi = HeapAlloc( GetProcessHeap(), 0, strlen(udi)+1 )))
    {
        HeapFree( GetProcessHeap(), 0, drive );
        return;
    }
    strcpy( drive->udi, udi );
    list_add_tail( &drives_list, &drive->entry );

found:
    drive->drive = add_drive( device, type );
    if (drive->drive != -1)
    {
        HKEY hkey;

        set_mount_point( drive, mount_point );

        WINE_TRACE( "added device %c: udi %s for %s on %s type %s\n",
                    'a' + drive->drive, wine_dbgstr_a(udi), wine_dbgstr_a(device),
                    wine_dbgstr_a(mount_point), wine_dbgstr_a(type) );

        /* hack: force the drive type in the registry */
        if (!RegCreateKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Drives", &hkey ))
        {
            char name[3] = "a:";
            name[0] += drive->drive;
            if (!type || strcmp( type, "cdrom" )) type = "floppy";  /* FIXME: default to floppy */
            RegSetValueExA( hkey, name, 0, REG_SZ, (const BYTE *)type, strlen(type) + 1 );
            RegCloseKey( hkey );
        }

        send_notify( drive->drive, DBT_DEVICEARRIVAL );
    }
}

static void remove_dos_device( struct dos_drive *drive )
{
    HKEY hkey;

    if (drive->drive != -1)
    {
        set_mount_point( drive, "" );

        /* clear the registry key too */
        if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Drives", &hkey ))
        {
            char name[3] = "a:";
            name[0] += drive->drive;
            RegDeleteValueA( hkey, name );
            RegCloseKey( hkey );
        }

        send_notify( drive->drive, DBT_DEVICEREMOVECOMPLETE );
    }

    list_remove( &drive->entry );
    HeapFree( GetProcessHeap(), 0, drive->udi );
    HeapFree( GetProcessHeap(), 0, drive );
}

/* HAL callback for new device */
static void new_device( LibHalContext *ctx, const char *udi )
{
    DBusError error;
    char *parent, *mount_point, *device, *type;

    p_dbus_error_init( &error );

    if (!(device = p_libhal_device_get_property_string( ctx, udi, "block.device", &error )))
        goto done;

    if (!(mount_point = p_libhal_device_get_property_string( ctx, udi, "volume.mount_point", &error )))
        goto done;

    if (!(parent = p_libhal_device_get_property_string( ctx, udi, "info.parent", &error )))
        goto done;

    if (!p_libhal_device_get_property_bool( ctx, parent, "storage.removable", &error ))
        goto done;

    if (!(type = p_libhal_device_get_property_string( ctx, parent, "storage.drive_type", &error )))
        p_dbus_error_free( &error );  /* ignore error */

    add_dos_device( udi, device, mount_point, type );

    if (type) p_libhal_free_string( type );
    p_libhal_free_string( parent );
    p_libhal_free_string( device );
    p_libhal_free_string( mount_point );

    /* add property watch for mount point */
    p_libhal_device_add_property_watch( ctx, udi, &error );

done:
    p_dbus_error_free( &error );
}

/* HAL callback for removed device */
static void removed_device( LibHalContext *ctx, const char *udi )
{
    DBusError error;
    struct dos_drive *drive;

    WINE_TRACE( "removed %s\n", wine_dbgstr_a(udi) );

    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        if (strcmp( udi, drive->udi )) continue;
        p_dbus_error_init( &error );
        p_libhal_device_remove_property_watch( ctx, udi, &error );
        remove_dos_device( drive );
        p_dbus_error_free( &error );
        return;
    }
}

/* HAL callback for property changes */
static void property_modified (LibHalContext *ctx, const char *udi,
                               const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
    WINE_TRACE( "udi %s key %s %s\n", wine_dbgstr_a(udi), wine_dbgstr_a(key),
                is_added ? "added" : is_removed ? "removed" : "modified" );

    if (!strcmp( key, "volume.mount_point" )) new_device( ctx, udi );
}


static DWORD WINAPI hal_thread( void *arg )
{
    DBusError error;
    DBusConnection *dbc;
    LibHalContext *ctx;
    int i, num;
    char **list;

    if (!(ctx = p_libhal_ctx_new())) return 1;

    p_dbus_error_init( &error );
    if (!(dbc = p_dbus_bus_get( DBUS_BUS_SYSTEM, &error )))
    {
        WINE_WARN( "failed to get system dbus connection: %s\n", error.message );
        p_dbus_error_free( &error );
        return 1;
    }

    p_libhal_ctx_set_dbus_connection( ctx, dbc );
    p_libhal_ctx_set_device_added( ctx, new_device );
    p_libhal_ctx_set_device_removed( ctx, removed_device );
    p_libhal_ctx_set_device_property_modified( ctx, property_modified );

    if (!p_libhal_ctx_init( ctx, &error ))
    {
        WINE_WARN( "HAL context init failed: %s\n", error.message );
        p_dbus_error_free( &error );
        return 1;
    }

    /* retrieve all existing devices */
    if (!(list = p_libhal_get_all_devices( ctx, &num, &error ))) p_dbus_error_free( &error );
    else
    {
        for (i = 0; i < num; i++) new_device( ctx, list[i] );
        p_libhal_free_string_array( list );
    }

    __TRY
    {
        while (p_dbus_connection_read_write_dispatch( dbc, -1 )) /* nothing */ ;
    }
    __EXCEPT( assert_fault )
    {
        WINE_WARN( "dbus assertion failure, disabling HAL support\n" );
        return 1;
    }
    __ENDTRY;

    p_libhal_ctx_shutdown( ctx, &error );
    p_dbus_error_free( &error );  /* just in case */
    p_dbus_connection_close( dbc );
    p_libhal_ctx_free( ctx );
    return 0;
}

void initialize_hal(void)
{
    HANDLE handle;

    if (!load_functions()) return;
    if (!(handle = CreateThread( NULL, 0, hal_thread, NULL, 0, NULL ))) return;
    CloseHandle( handle );
}

#else  /* HAVE_LIBHAL */

void initialize_hal(void)
{
    WINE_WARN( "HAL support not compiled in\n" );
}

#endif  /* HAVE_LIBHAL */
