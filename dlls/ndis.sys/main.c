/*
 * ndis.sys
 *
 * Copyright 2014 Austin English
 * Copyright 2016 André Hentschel
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/ndis.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ndis);

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    TRACE("(%p, %s)\n", driver, debugstr_w(path->Buffer));

    return STATUS_SUCCESS;
}

NDIS_STATUS WINAPI NdisAllocateMemoryWithTag(void **address, UINT length, ULONG tag)
{
    FIXME("(%p, %u, %u): stub\n", address, length, tag);
    return NDIS_STATUS_FAILURE;
}

void WINAPI NdisAllocateSpinLock(NDIS_SPIN_LOCK *lock)
{
    FIXME("(%p): stub\n", lock);
}

void WINAPI NdisRegisterProtocol(NDIS_STATUS *status, NDIS_HANDLE *handle,
                                 NDIS_PROTOCOL_CHARACTERISTICS *prot, UINT len)
{
    FIXME("(%p, %p, %p, %u): stub\n", status, handle, prot, len);
    *status = NDIS_STATUS_FAILURE;
}

CCHAR WINAPI NdisSystemProcessorCount(void)
{
    SYSTEM_INFO si;

    TRACE("()\n");
    GetSystemInfo(&si);

    return si.dwNumberOfProcessors;
}
