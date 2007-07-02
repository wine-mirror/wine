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
#include "excpt.h"

#include "wine/library.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#ifdef HAVE_LIBHAL

#include <dbus/dbus.h>
#include <hal/libhal.h>

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

#ifndef SONAME_LIBHAL
#define SONAME_LIBHAL "libhal" SONAME_EXT
#endif

static BOOL load_functions(void)
{
    void *hal_handle;
    char error[128];

    /* Load libhal with RTLD_GLOBAL so that the dbus symbols are available.
     * We can't load libdbus directly since libhal may have been built against a
     * different version but with the same soname. Binary compatibility is for wimps. */

    if (!(hal_handle = wine_dlopen(SONAME_LIBHAL, RTLD_NOW|RTLD_GLOBAL, error, sizeof(error))))
        goto failed;

#define DO_FUNC(f) if (!(p_##f = wine_dlsym( RTLD_DEFAULT, #f, error, sizeof(error) ))) goto failed
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

    WINE_TRACE( "removed %s\n", wine_dbgstr_a(udi) );

    if (remove_dos_device( udi ))
    {
        p_dbus_error_init( &error );
        p_libhal_device_remove_property_watch( ctx, udi, &error );
        p_dbus_error_free( &error );
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
    WINE_TRACE( "Skipping, HAL support not compiled in\n" );
}

#endif  /* HAVE_LIBHAL */
