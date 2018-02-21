/*
 * Plug and Play support for hid devices found through SDL2
 *
 * Copyright 2017 CodeWeavers, Aric Stewart
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
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/debug.h"

#include "bus.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef SONAME_LIBSDL2

NTSTATUS WINAPI sdl_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    FIXME("STUB: (%p, %s)\n", driver, debugstr_w(registry_path->Buffer));
    return STATUS_UNSUCCESSFUL;
}

#else

NTSTATUS WINAPI sdl_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    WARN("compiled without SDL support\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBSDL2 */
