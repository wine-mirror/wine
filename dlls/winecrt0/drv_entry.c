/*
 * Default entry point for a native driver
 *
 * Copyright 2005 Alexandre Julliard
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

#include <stdarg.h>
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "crt0_private.h"

struct _DRIVER_OBJECT;
extern NTSTATUS WINAPI DriverEntry( struct _DRIVER_OBJECT *obj, UNICODE_STRING *path );

NTSTATUS DECLSPEC_HIDDEN WINAPI __wine_spec_drv_entry( struct _DRIVER_OBJECT *obj, UNICODE_STRING *path )
{
    BOOL needs_init = (__wine_spec_init_state != CONSTRUCTORS_DONE);

    if (needs_init) _init( __wine_main_argc, __wine_main_argv, NULL );
    return DriverEntry( obj, path );
    /* there is no detach routine so we can't call destructors */
}
