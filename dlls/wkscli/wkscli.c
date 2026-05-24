/*
 * Copyright 2026 Alistair Leslie-Hughes
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
#include "windef.h"
#include "winbase.h"

#include "lm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wkscli);

/************************************************************
 *                NetGetJoinInformation (wkscli.@)
 */
NET_API_STATUS NET_API_FUNCTION NetGetJoinInformation(
    LPCWSTR Server,
    LPWSTR *Name,
    PNETSETUP_JOIN_STATUS type)
{
    static const WCHAR workgroupW[] = L"Workgroup";

    FIXME("Semi-stub %s %p %p\n", wine_dbgstr_w(Server), Name, type);

    if (!Name || !type)
        return ERROR_INVALID_PARAMETER;

    NetApiBufferAllocate(sizeof(workgroupW), (LPVOID *)Name);
    lstrcpyW(*Name, workgroupW);
    *type = NetSetupWorkgroupName;

    return NERR_Success;
}

/************************************************************
 *                NetUseAdd (wkscli.@)
 */
NET_API_STATUS WINAPI NetUseAdd(LMSTR servername, DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    FIXME("%s %ld %p %p stub\n", debugstr_w(servername), level, bufptr, parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetUseDel (wkscli.@)
 */
NET_API_STATUS WINAPI NetUseDel(LMSTR servername, LMSTR usename, DWORD forcecond)
{
    FIXME("%s %s %ld stub\n", debugstr_w(servername), debugstr_w(usename), forcecond);
    return NERR_Success;
}

/************************************************************
 *                NetUseEnum (wkscli.@)
 */
NET_API_STATUS WINAPI NetUseEnum(LMSTR server, DWORD level, LPBYTE* bufptr, DWORD prefmaxsize,
                                  LPDWORD entriesread, LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("stub (%p, %ld, %p, %ld, %p, %p, %p)\n", server, level, bufptr, prefmaxsize,
           entriesread, totalentries, resumehandle);
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                NetUseGetInfo (wkscli.@)
 */
NET_API_STATUS WINAPI NetUseGetInfo(LMSTR server, LMSTR name, DWORD level, LPBYTE *bufptr)
{
    FIXME("stub (%p, %p, %ld, %p)\n", server, name, level, bufptr);
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                NetWkstaTransportEnum (wkscli.@)
 */
NET_API_STATUS WINAPI NetWkstaTransportEnum(LMSTR ServerName, DWORD level, PBYTE* pbuf,
      DWORD prefmaxlen, LPDWORD read_entries,
      PDWORD total_entries, PDWORD hresume)
{
    FIXME(":%s, 0x%08lx, %p, 0x%08lx, %p, %p, %p\n", debugstr_w(ServerName),
            level, pbuf, prefmaxlen, read_entries, total_entries,hresume);
    return ERROR_NOT_SUPPORTED;
}
