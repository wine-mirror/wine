/*
 * WINE Platform native bus driver
 *
 * Copyright 2016 Aric Stewart
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

NTSTATUS WINAPI common_pnp_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS status = irp->IoStatus.u.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);

    switch (irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            TRACE("IRP_MN_QUERY_DEVICE_RELATIONS\n");
            break;
        case IRP_MN_QUERY_ID:
            TRACE("IRP_MN_QUERY_ID\n");
            break;
        case IRP_MN_QUERY_CAPABILITIES:
            TRACE("IRP_MN_QUERY_CAPABILITIES\n");
            break;
        default:
            FIXME("Unhandled function %08x\n", irpsp->MinorFunction);
            break;
    }

    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    static const WCHAR udevW[] = {'\\','D','r','i','v','e','r','\\','U','D','E','V',0};
    static UNICODE_STRING udev = {sizeof(udevW) - sizeof(WCHAR), sizeof(udevW), (WCHAR *)udevW};

    TRACE( "(%p, %s)\n", driver, debugstr_w(path->Buffer) );

    IoCreateDriver(&udev, udev_driver_init);

    return STATUS_SUCCESS;
}
