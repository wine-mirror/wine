/*
 * Plug and Play support for hid devices found through udev
 *
 * Copyright 2016 CodeWeavers, Aric Stewart
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
#include <stdarg.h>

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/debug.h"

#include "bus.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef HAVE_UDEV

static DRIVER_OBJECT *udev_driver_obj = NULL;

NTSTATUS WINAPI udev_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    TRACE("(%p, %s)\n", driver, debugstr_w(registry_path->Buffer));

    udev_driver_obj = driver;
    driver->MajorFunction[IRP_MJ_PNP] = common_pnp_dispatch;

    return STATUS_SUCCESS;
}

#else

NTSTATUS WINAPI udev_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    WARN("Wine was compiled without UDEV support\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_UDEV */
