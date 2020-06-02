/*
 * WSK (Winsock Kernel) driver library.
 *
 * Copyright 2020 Paul Gofman <pgofman@codeweavers.com> for Codeweavers
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
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winioctl.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/wsk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(netio);

NTSTATUS WINAPI WskCaptureProviderNPI(WSK_REGISTRATION *wsk_registration, ULONG wait_timeout,
        WSK_PROVIDER_NPI *wsk_provider_npi)
{
    FIXME("wsk_registration %p, wait_timeout %u, wsk_provider_npi %p stub.\n",
            wsk_registration, wait_timeout, wsk_provider_npi);

    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI WskReleaseProviderNPI(WSK_REGISTRATION *wsk_registration)
{
    FIXME("wsk_registration %p stub.\n", wsk_registration);
}

NTSTATUS WINAPI WskRegister(WSK_CLIENT_NPI *wsk_client_npi, WSK_REGISTRATION *wsk_registration)
{
    FIXME("wsk_client_npi %p, wsk_registration %p stub.\n", wsk_client_npi, wsk_registration);

    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI WskDeregister(WSK_REGISTRATION *wsk_registration)
{
    FIXME("wsk_registration %p stub.\n", wsk_registration);
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    TRACE("driver %p.\n", driver);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    TRACE("driver %p, path %s.\n", driver, debugstr_w(path->Buffer));

    driver->DriverUnload = driver_unload;
    return STATUS_SUCCESS;
}
