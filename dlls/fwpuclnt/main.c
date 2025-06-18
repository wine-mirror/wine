/*
 * Implementation of Windows Filtering Platform (WFP) management functions
 *
 * Copyright 2009 Paul Chitescu
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

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fwpuclnt);

/***********************************************************************
 *   FwpmFreeMemory0 (FWPUCLNT.@)
 *
 */

void WINAPI FwpmFreeMemory0(void** mem)
{
    FIXME("(%p) stub, mem=%p\n", mem, (mem ? *mem : NULL));
}

/***********************************************************************
 *   FwpmEngineOpen0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmEngineOpen0(LPCWSTR serverName, UINT32 authService, void* authIdentity, void* session, LPHANDLE engineHandle)
{
    FIXME("(%s, 0x%X, %p, %p, %p) stub\n", debugstr_w(serverName), authService, authIdentity, session, engineHandle);
    *engineHandle = NULL;
    return RPC_S_CANNOT_SUPPORT;
}

/***********************************************************************
 *   FwpmEngineClose0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmEngineClose0(HANDLE engineHandle)
{
    FIXME("(%p) stub\n", engineHandle);
    return RPC_S_CANNOT_SUPPORT;
}


/***********************************************************************
 *   FwpmSubLayerAdd0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmSubLayerAdd0(HANDLE engineHandle, void* subLayer, PSECURITY_DESCRIPTOR security)
{
    FIXME("(%p, %p, %p) stub\n", engineHandle, subLayer, security);
    return RPC_S_CANNOT_SUPPORT;
}

/***********************************************************************
 *   FwpmSubLayerGetByKey0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmSubLayerGetByKey0(HANDLE engineHandle, LPCGUID key, void** subLayer)
{
    FIXME("(%p, %s, %p) stub\n", engineHandle, debugstr_guid(key), subLayer);
    *subLayer = NULL;
    return RPC_S_CANNOT_SUPPORT;
}

/***********************************************************************
 *   FwpmFilterCreateEnumHandle0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmFilterCreateEnumHandle0(HANDLE engineHandle, void* enumTemplate, LPHANDLE enumHandle)
{
    FIXME("(%p, %p, %p) stub\n", engineHandle, enumTemplate, enumHandle);
    *enumHandle = NULL;
    return RPC_S_CANNOT_SUPPORT;
}

/***********************************************************************
 *   FwpmFilterDestroyEnumHandle0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmFilterDestroyEnumHandle0(HANDLE engineHandle, HANDLE enumHandle)
{
    FIXME("(%p, %p) stub\n", engineHandle, enumHandle);
    return RPC_S_CANNOT_SUPPORT;
}

/***********************************************************************
 *   FwpmFilterEnum0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmFilterEnum0(HANDLE engineHandle, HANDLE enumHandle, UINT32 nEntriesRequested, void*** entries, UINT32* nEntriesReturned)
{
    FIXME("(%p, %p, %u, %p, %p) stub\n", engineHandle, enumHandle, nEntriesRequested, entries, nEntriesReturned);
    *entries = NULL;
    *nEntriesReturned = 0;
    return RPC_S_CANNOT_SUPPORT;
}

/***********************************************************************
 *   FwpmFilterAdd0 (FWPUCLNT.@)
 *
 */

DWORD WINAPI FwpmFilterAdd0(HANDLE engineHandle, void* filter, PSECURITY_DESCRIPTOR security, UINT64* pFilterId)
{
    FIXME("(%p, %p, %p, %p) stub\n", engineHandle, filter, security, pFilterId);
    return RPC_S_CANNOT_SUPPORT;
}
