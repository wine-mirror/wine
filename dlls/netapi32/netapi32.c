/* Copyright 2001 Mike McCormack
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2003 Juan Lang
 * Copyright 2006 Paul Vriens
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

#include "wine/debug.h"
#include "lm.h"
#include "lmat.h"
#include "netbios.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

BOOL NETAPI_IsLocalComputer(LMCSTR ServerName);

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            NetBIOSInit();
            NetBTInit();
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            NetBIOSShutdown();
            break;
    }

    return TRUE;
}

/************************************************************
 *                NetServerEnum (NETAPI32.@)
 */
NET_API_STATUS  WINAPI NetServerEnum(
  LMCSTR servername,
  DWORD level,
  LPBYTE* bufptr,
  DWORD prefmaxlen,
  LPDWORD entriesread,
  LPDWORD totalentries,
  DWORD servertype,
  LMCSTR domain,
  LPDWORD resume_handle
)
{
    FIXME("Stub (%s %d %p %d %p %p %d %s %p)\n", debugstr_w(servername),
     level, bufptr, prefmaxlen, entriesread, totalentries, servertype,
     debugstr_w(domain), resume_handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/************************************************************
 *                NetServerEnumEx (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetServerEnumEx(
    LMCSTR ServerName,
    DWORD Level,
    LPBYTE *Bufptr,
    DWORD PrefMaxlen,
    LPDWORD EntriesRead,
    LPDWORD totalentries,
    DWORD servertype,
    LMCSTR domain,
    LMCSTR FirstNameToReturn)
{
    FIXME("Stub (%s %d %p %d %p %p %d %s %s)\n",
           debugstr_w(ServerName), Level, Bufptr, PrefMaxlen, EntriesRead, totalentries,
           servertype, debugstr_w(domain), debugstr_w(FirstNameToReturn));

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/************************************************************
 *                NetServerDiskEnum (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetServerDiskEnum(
    LMCSTR ServerName,
    DWORD Level,
    LPBYTE *Bufptr,
    DWORD PrefMaxlen,
    LPDWORD EntriesRead,
    LPDWORD totalentries,
    LPDWORD Resume_Handle)
{
    FIXME("Stub (%s %d %p %d %p %p %p)\n", debugstr_w(ServerName),
     Level, Bufptr, PrefMaxlen, EntriesRead, totalentries, Resume_Handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/************************************************************
 *                NetServerGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetServerGetInfo(LMSTR servername, DWORD level, LPBYTE* bufptr)
{
    NET_API_STATUS ret;

    TRACE("%s %d %p\n", debugstr_w( servername ), level, bufptr );
    if (servername)
    {
        if (!NETAPI_IsLocalComputer(servername))
        {
            FIXME("remote computers not supported\n");
            return ERROR_INVALID_LEVEL;
        }
    }
    if (!bufptr) return ERROR_INVALID_PARAMETER;

    switch (level)
    {
        case 100:
        case 101:
        {
            DWORD computerNameLen, size;
            WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];

            computerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
            GetComputerNameW(computerName, &computerNameLen);
            computerNameLen++; /* include NULL terminator */

            size = sizeof(SERVER_INFO_101) + computerNameLen * sizeof(WCHAR);
            ret = NetApiBufferAllocate(size, (LPVOID *)bufptr);
            if (ret == NERR_Success)
            {
                /* INFO_100 structure is a subset of INFO_101 */
                PSERVER_INFO_101 info = (PSERVER_INFO_101)*bufptr;
                OSVERSIONINFOW verInfo;

                info->sv101_platform_id = PLATFORM_ID_NT;
                info->sv101_name = (LMSTR)(*bufptr + sizeof(SERVER_INFO_101));
                memcpy(info->sv101_name, computerName,
                       computerNameLen * sizeof(WCHAR));
                verInfo.dwOSVersionInfoSize = sizeof(verInfo);
                GetVersionExW(&verInfo);
                info->sv101_version_major = verInfo.dwMajorVersion;
                info->sv101_version_minor = verInfo.dwMinorVersion;
                 /* Use generic type as no wine equivalent of DC / Server */
                info->sv101_type = SV_TYPE_NT;
                info->sv101_comment = NULL;
            }
            break;
        }

        default:
            FIXME("level %d unimplemented\n", level);
            ret = ERROR_INVALID_LEVEL;
    }
    return ret;
}


/************************************************************
 *                NetStatisticsGet  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetStatisticsGet(LMSTR server, LMSTR service,
                                       DWORD level, DWORD options,
                                       LPBYTE *bufptr)
{
    TRACE("(%p, %p, %d, %d, %p)\n", server, service, level, options, bufptr);
    return NERR_InternalError;
}

NET_API_STATUS WINAPI NetUseEnum(LMSTR server, DWORD level, LPBYTE* bufptr, DWORD prefmaxsize,
                          LPDWORD entriesread, LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("stub (%p, %d, %p, %d, %p, %p, %p)\n", server, level, bufptr, prefmaxsize,
           entriesread, totalentries, resumehandle);
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS WINAPI NetScheduleJobAdd(LPCWSTR server, LPBYTE bufptr, LPDWORD jobid)
{
    FIXME("stub (%s, %p, %p)\n", debugstr_w(server), bufptr, jobid);
    return NERR_Success;
}

NET_API_STATUS WINAPI NetScheduleJobEnum(LPCWSTR server, LPBYTE* bufptr, DWORD prefmaxsize, LPDWORD entriesread,
                                         LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("stub (%s, %p, %d, %p, %p, %p)\n", debugstr_w(server), bufptr, prefmaxsize, entriesread, totalentries, resumehandle);
    *entriesread = 0;
    *totalentries = 0;
    return NERR_Success;
}

NET_API_STATUS WINAPI NetUseGetInfo(LMSTR server, LMSTR name, DWORD level, LPBYTE *bufptr)
{
    FIXME("stub (%p, %p, %d, %p)\n", server, name, level, bufptr);
    return ERROR_NOT_SUPPORTED;

}

/************************************************************
 *                NetApiBufferAllocate  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferAllocate(DWORD ByteCount, LPVOID* Buffer)
{
    TRACE("(%d, %p)\n", ByteCount, Buffer);

    if (Buffer == NULL) return ERROR_INVALID_PARAMETER;
    *Buffer = HeapAlloc(GetProcessHeap(), 0, ByteCount);
    if (*Buffer)
        return NERR_Success;
    else
        return GetLastError();
}

/************************************************************
 *                NetApiBufferFree  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferFree(LPVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
    return NERR_Success;
}

/************************************************************
 *                NetApiBufferReallocate  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferReallocate(LPVOID OldBuffer, DWORD NewByteCount,
                                             LPVOID* NewBuffer)
{
    TRACE("(%p, %d, %p)\n", OldBuffer, NewByteCount, NewBuffer);
    if (NewByteCount)
    {
        if (OldBuffer)
            *NewBuffer = HeapReAlloc(GetProcessHeap(), 0, OldBuffer, NewByteCount);
        else
            *NewBuffer = HeapAlloc(GetProcessHeap(), 0, NewByteCount);
	return *NewBuffer ? NERR_Success : GetLastError();
    }
    else
    {
	if (!HeapFree(GetProcessHeap(), 0, OldBuffer)) return GetLastError();
	*NewBuffer = 0;
	return NERR_Success;
    }
}

/************************************************************
 *                NetApiBufferSize  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferSize(LPVOID Buffer, LPDWORD ByteCount)
{
    DWORD dw;

    TRACE("(%p, %p)\n", Buffer, ByteCount);
    if (Buffer == NULL)
        return ERROR_INVALID_PARAMETER;
    dw = HeapSize(GetProcessHeap(), 0, Buffer);
    TRACE("size: %d\n", dw);
    if (dw != 0xFFFFFFFF)
        *ByteCount = dw;
    else
        *ByteCount = 0;

    return NERR_Success;
}

/************************************************************
 * NetSessionEnum  (NETAPI32.@)
 *
 * PARAMS
 *   servername    [I]   Pointer to a string with the name of the server
 *   UncClientName [I]   Pointer to a string with the name of the session
 *   username      [I]   Pointer to a string with the name of the user
 *   level         [I]   Data information level
 *   bufptr        [O]   Buffer to the data
 *   prefmaxlen    [I]   Preferred maximum length of the data
 *   entriesread   [O]   Pointer to the number of entries enumerated
 *   totalentries  [O]   Pointer to the possible number of entries
 *   resume_handle [I/O] Pointer to a handle for subsequent searches
 *
 * RETURNS
 *   If successful, the function returns NERR_Success
 *   On failure it returns:
 *     ERROR_ACCESS_DENIED         User has no access to the requested information
 *     ERROR_INVALID_LEVEL         Value of 'level' is not correct
 *     ERROR_INVALID_PARAMETER     Wrong parameter
 *     ERROR_MORE_DATA             Need a larger buffer
 *     ERROR_NOT_ENOUGH_MEMORY     Not enough memory
 *     NERR_ClientNameNotFound     A session does not exist on a given computer
 *     NERR_InvalidComputer        Invalid computer name
 *     NERR_UserNotFound           User name could not be found.
 */
NET_API_STATUS WINAPI NetSessionEnum(LMSTR servername, LMSTR UncClientName,
    LMSTR username, DWORD level, LPBYTE* bufptr, DWORD prefmaxlen, LPDWORD entriesread,
    LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("Stub (%s %s %s %d %p %d %p %p %p)\n", debugstr_w(servername),
        debugstr_w(UncClientName), debugstr_w(username),
        level, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

    return NERR_Success;
}

/************************************************************
 * NetShareEnum  (NETAPI32.@)
 *
 * PARAMS
 *   servername    [I]   Pointer to a string with the name of the server
 *   level         [I]   Data information level
 *   bufptr        [O]   Buffer to the data
 *   prefmaxlen    [I]   Preferred maximum length of the data
 *   entriesread   [O]   Pointer to the number of entries enumerated
 *   totalentries  [O]   Pointer to the possible number of entries
 *   resume_handle [I/O] Pointer to a handle for subsequent searches
 *
 * RETURNS
 *   If successful, the function returns NERR_Success
 *   On failure it returns a system error code (FIXME: find out which)
 *
 */
NET_API_STATUS WINAPI NetShareEnum( LMSTR servername, DWORD level, LPBYTE* bufptr,
    DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("Stub (%s %d %p %d %p %p %p)\n", debugstr_w(servername), level, bufptr,
        prefmaxlen, entriesread, totalentries, resume_handle);

    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 * NetShareDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareDel(LMSTR servername, LMSTR netname, DWORD reserved)
{
    FIXME("Stub (%s %s %d)\n", debugstr_w(servername), debugstr_w(netname), reserved);
    return NERR_Success;
}

/************************************************************
 * NetShareGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareGetInfo(LMSTR servername, LMSTR netname,
    DWORD level, LPBYTE *bufptr)
{
    FIXME("Stub (%s %s %d %p)\n", debugstr_w(servername),
        debugstr_w(netname),level, bufptr);
    return NERR_NetNameNotFound;
}

/************************************************************
 * NetShareAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareAdd(LMSTR servername,
    DWORD level, LPBYTE buf, LPDWORD parm_err)
{
    FIXME("Stub (%s %d %p %p)\n", debugstr_w(servername), level, buf, parm_err);
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                NetFileEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetFileEnum(
    LPWSTR ServerName, LPWSTR BasePath, LPWSTR UserName,
    DWORD Level, LPBYTE* BufPtr, DWORD PrefMaxLen,
    LPDWORD EntriesRead, LPDWORD TotalEntries, PDWORD_PTR ResumeHandle)
{
    FIXME("(%s, %s, %s, %u): stub\n", debugstr_w(ServerName), debugstr_w(BasePath),
        debugstr_w(UserName), Level);
    return ERROR_NOT_SUPPORTED;
}
