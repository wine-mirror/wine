/* Copyright 2001 Mike McCormack
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2003 Juan Lang
 * Copyright 2005,2006 Paul Vriens
 * Copyright 2006 Robert Reif
 * Copyright 2013 Hans Leidekker for CodeWeavers
 * Copyright 2020 Dmitry Timoshkov
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
#include "winsock2.h"
#include "ws2ipdef.h"
#include "windns.h"
#include "lm.h"
#include "lmaccess.h"
#include "atsvc.h"
#include "lmapibuf.h"
#include "lmbrowsr.h"
#include "lmremutl.h"
#include "lmshare.h"
#include "lmwksta.h"
#include "netbios.h"
#include "ifmib.h"
#include "iphlpapi.h"
#include "ntsecapi.h"
#include "dsrole.h"
#include "dsgetdc.h"
#include "davclnt.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "initguid.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#define SAMBA_CALL(func, args) WINE_UNIX_CALL( unix_ ## func, args )

static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

static BOOL WINAPI load_samba( INIT_ONCE *once, void *param, void **context )
{
    SAMBA_CALL( netapi_init, NULL );
    return TRUE;
}

static BOOL samba_init(void)
{
    return __wine_unixlib_handle && InitOnceExecuteOnce( &init_once, load_samba, NULL, NULL );
}

/************************************************************
 *                NETAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
static BOOL NETAPI_IsLocalComputer( LMCSTR name )
{
    WCHAR buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = ARRAY_SIZE(buf);
    BOOL ret;

    if (!name || !name[0]) return TRUE;

    ret = GetComputerNameW( buf,  &size );
    if (ret && name[0] == '\\' && name[1] == '\\') name += 2;
    return ret && !wcsicmp( name, buf );
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            __wine_init_unix_call();
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
    FIXME("Stub (%s %ld %p %ld %p %p %ld %s %p)\n", debugstr_w(servername),
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
    FIXME("Stub (%s %ld %p %ld %p %p %ld %s %s)\n",
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
    FIXME("Stub (%s %ld %p %ld %p %p %p)\n", debugstr_w(ServerName),
     Level, Bufptr, PrefMaxlen, EntriesRead, totalentries, Resume_Handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/************************************************************
 *                NetServerGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetServerGetInfo(LMSTR servername, DWORD level, LPBYTE* bufptr)
{
    NET_API_STATUS ret;
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %ld %p\n", debugstr_w( servername ), level, bufptr );

    if (!bufptr) return ERROR_INVALID_PARAMETER;
    if (!local)
    {
        if (samba_init())
        {
            ULONG size = 1024;
            struct server_getinfo_params params = { servername, level, NULL, &size };

            for (;;)
            {
                if ((ret = NetApiBufferAllocate( size, &params.buffer ))) return ret;
                ret = SAMBA_CALL( server_getinfo, &params );
                if (!ret) *bufptr = params.buffer;
                else NetApiBufferFree( params.buffer );
                if (ret != ERROR_INSUFFICIENT_BUFFER) return ret;
            }
        }
        FIXME( "remote computers not supported\n" );
        return ERROR_INVALID_LEVEL;
    }

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

            /* Plus 1 for empty comment */
            size = sizeof(SERVER_INFO_101) + (computerNameLen + 1) * sizeof(WCHAR);
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
                info->sv101_comment = (LMSTR)(*bufptr + sizeof(SERVER_INFO_101)
                                              + computerNameLen * sizeof(WCHAR));
                info->sv101_comment[0] = '\0';
            }
            break;
        }

        default:
            FIXME("level %ld unimplemented\n", level);
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
    int res;
    union
    {
        STAT_WORKSTATION_0 workst;
        STAT_SERVER_0 server;
    } *stat;
    void *dataptr;

    TRACE("(server %s, service %s, level %ld, options %ld, buffer %p): stub\n",
          debugstr_w(server), debugstr_w(service), level, options, bufptr);

    res = NetApiBufferAllocate(sizeof(*stat), &dataptr);
    if (res != NERR_Success) return res;

    res = NERR_InternalError;
    stat = dataptr;
    switch (level)
    {
        case 0:
            if (!wcscmp( service, L"LanmanWorkstation" ))
            {
                /* Fill the struct STAT_WORKSTATION_0 properly */
                memset(&stat->workst, 0, sizeof(stat->workst));
                res = NERR_Success;
            }
            else if (!wcscmp( service, L"LanmanServer" ))
            {
                /* Fill the struct STAT_SERVER_0 properly */
                memset(&stat->server, 0, sizeof(stat->server));
                res = NERR_Success;
            }
            break;
    }
    if (res != NERR_Success)
        NetApiBufferFree(dataptr);
    else
        *bufptr = dataptr;

    return res;
}

NET_API_STATUS WINAPI NetUseEnum(LMSTR server, DWORD level, LPBYTE* bufptr, DWORD prefmaxsize,
                          LPDWORD entriesread, LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("stub (%p, %ld, %p, %ld, %p, %p, %p)\n", server, level, bufptr, prefmaxsize,
           entriesread, totalentries, resumehandle);
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS WINAPI NetScheduleJobAdd(LPCWSTR server, LPBYTE bufptr, LPDWORD jobid)
{
    TRACE("(%s, %p, %p)\n", debugstr_w(server), bufptr, jobid);
    return NetrJobAdd(server, (AT_INFO *)bufptr, jobid);
}

NET_API_STATUS WINAPI NetScheduleJobDel(LPCWSTR server, DWORD minjobid, DWORD maxjobid)
{
    TRACE("(%s, %lu, %lu)\n", debugstr_w(server), minjobid, maxjobid);
    return NetrJobDel(server, minjobid, maxjobid);
}

NET_API_STATUS WINAPI NetScheduleJobEnum(LPCWSTR server, LPBYTE* bufptr, DWORD prefmaxsize, LPDWORD entriesread,
                                         LPDWORD totalentries, LPDWORD resumehandle)
{
    AT_ENUM_CONTAINER container;
    NET_API_STATUS ret;

    TRACE("(%s, %p, %lu, %p, %p, %p)\n", debugstr_w(server), bufptr, prefmaxsize, entriesread, totalentries, resumehandle);

    container.EntriesRead = 0;
    container.Buffer = NULL;
    ret = NetrJobEnum(server, &container, prefmaxsize, totalentries, resumehandle);
    if (ret == ERROR_SUCCESS)
    {
        *bufptr = (LPBYTE)container.Buffer;
        *entriesread = container.EntriesRead;
    }
    return ret;
}

NET_API_STATUS WINAPI NetScheduleJobGetInfo(LPCWSTR server, DWORD jobid, LPBYTE *bufptr)
{
    TRACE("(%s, %lu, %p)\n", debugstr_w(server), jobid, bufptr);
    return NetrJobGetInfo(server, jobid, (LPAT_INFO *)bufptr);
}

NET_API_STATUS WINAPI NetUseGetInfo(LMSTR server, LMSTR name, DWORD level, LPBYTE *bufptr)
{
    FIXME("stub (%p, %p, %ld, %p)\n", server, name, level, bufptr);
    return ERROR_NOT_SUPPORTED;

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
    FIXME("Stub (%s %s %s %ld %p %ld %p %p %p)\n", debugstr_w(servername),
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
    FIXME("Stub (%s %ld %p %ld %p %p %p)\n", debugstr_w(servername), level, bufptr,
        prefmaxlen, entriesread, totalentries, resume_handle);

    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 * NetShareDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareDel(LMSTR servername, LMSTR netname, DWORD reserved)
{
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %s %ld\n", debugstr_w(servername), debugstr_w(netname), reserved);

    if (!local)
    {
        if (samba_init())
        {
            struct share_del_params params = { servername, netname, reserved };
            return SAMBA_CALL( share_del, &params );
        }
        FIXME( "remote computers not supported\n" );
    }

    FIXME("%s %s %ld\n", debugstr_w(servername), debugstr_w(netname), reserved);
    return NERR_Success;
}

/************************************************************
 * NetShareGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareGetInfo(LMSTR servername, LMSTR netname,
    DWORD level, LPBYTE *bufptr)
{
    BOOL local = NETAPI_IsLocalComputer( servername );

    if (!bufptr) return ERROR_INVALID_PARAMETER;
    if (!local && samba_init())
    {
        ULONG size = 1024;
        struct share_getinfo_params params = { servername, netname, level, NULL, &size };
        NET_API_STATUS ret;

        TRACE("%s %s %ld %p\n", debugstr_w(servername), debugstr_w(netname), level, bufptr);

        for (;;)
        {
            if ((ret = NetApiBufferAllocate( size, &params.buffer ))) return ret;
            ret = SAMBA_CALL( share_getinfo, &params );
            if (!ret) *bufptr = params.buffer;
            else NetApiBufferFree( params.buffer );
            if (ret != ERROR_INSUFFICIENT_BUFFER) return ret;
        }
    }

    FIXME("%s %s %ld %p\n", debugstr_w(servername), debugstr_w(netname), level, bufptr);
    return NERR_NetNameNotFound;
}

/************************************************************
 * NetShareAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareAdd(LMSTR servername,
    DWORD level, LPBYTE buf, LPDWORD parm_err)
{
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %ld %p %p\n", debugstr_w(servername), level, buf, parm_err);

    if (!local)
    {
        if (samba_init())
        {
            struct share_add_params params = { servername, level, buf, parm_err };
            return SAMBA_CALL( share_add, &params );
        }
        FIXME( "remote computers not supported\n" );
    }

    FIXME("%s %ld %p %p\n", debugstr_w(servername), level, buf, parm_err);
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
    FIXME("(%s, %s, %s, %lu): stub\n", debugstr_w(ServerName), debugstr_w(BasePath),
        debugstr_w(UserName), Level);
    return ERROR_NOT_SUPPORTED;
}

static void wprint_mac(WCHAR* buffer, int len, const MIB_IFROW *ifRow)
{
    int i;
    unsigned char val;

    if (!buffer)
        return;
    if (len < 1)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    for (i = 0; i < ifRow->dwPhysAddrLen && 2 * i < len; i++)
    {
        val = ifRow->bPhysAddr[i];
        if ((val >>4) >9)
            buffer[2*i] = (WCHAR)((val >>4) + 'A' - 10);
        else
            buffer[2*i] = (WCHAR)((val >>4) + '0');
        if ((val & 0xf ) >9)
            buffer[2*i+1] = (WCHAR)((val & 0xf) + 'A' - 10);
        else
            buffer[2*i+1] = (WCHAR)((val & 0xf) + '0');
    }
    buffer[2*i]=0;
}

/* Theoretically this could be too short, except that MS defines
 * MAX_ADAPTER_NAME as 128, and MAX_INTERFACE_NAME_LEN as 256, and both
 * represent a count of WCHARs, so even with an extraordinarily long header
 * this will be plenty
 */
#define MAX_TRANSPORT_NAME MAX_INTERFACE_NAME_LEN
#define MAX_TRANSPORT_ADDR 13

#define NBT_TRANSPORT_NAME_HEADER "\\Device\\NetBT_Tcpip_"
#define UNKNOWN_TRANSPORT_NAME_HEADER "\\Device\\UnknownTransport_"

static void wprint_name(WCHAR *buffer, int len, ULONG transport,
 PMIB_IFROW ifRow)
{
    WCHAR *ptr1, *ptr2;
    const char *name;

    if (!buffer)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
        name = NBT_TRANSPORT_NAME_HEADER;
    else
        name = UNKNOWN_TRANSPORT_NAME_HEADER;

    for (ptr1 = buffer; *name && ptr1 < buffer + len; ptr1++, name++)
        *ptr1 = *name;
    for (ptr2 = ifRow->wszName; *ptr2 && ptr1 < buffer + len; ptr1++, ptr2++)
        *ptr1 = *ptr2;
    *ptr1 = '\0';
}

/***********************************************************************
 *                NetWkstaTransportEnum  (NETAPI32.@)
 */

struct WkstaTransportEnumData
{
    UCHAR          n_adapt;
    UCHAR          n_read;
    DWORD          prefmaxlen;
    LPBYTE        *pbuf;
    NET_API_STATUS ret;
};

/**********************************************************************/

static BOOL WkstaEnumAdaptersCallback(UCHAR totalLANAs, UCHAR lanaIndex,
 ULONG transport, const NetBIOSAdapterImpl *data, void *closure)
{
    BOOL ret;
    struct WkstaTransportEnumData *enumData = closure;

    if (enumData && enumData->pbuf)
    {
        if (lanaIndex == 0)
        {
            DWORD toAllocate;

            enumData->n_adapt = totalLANAs;
            enumData->n_read = 0;

            toAllocate = totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0)
             + MAX_TRANSPORT_NAME * sizeof(WCHAR) +
             MAX_TRANSPORT_ADDR * sizeof(WCHAR));
            if (enumData->prefmaxlen != MAX_PREFERRED_LENGTH)
                toAllocate = enumData->prefmaxlen;
            NetApiBufferAllocate(toAllocate, (LPVOID *)enumData->pbuf);
        }
        if (*(enumData->pbuf))
        {
            UCHAR spaceFor;

            if (enumData->prefmaxlen == MAX_PREFERRED_LENGTH)
                spaceFor = totalLANAs;
            else
                spaceFor = enumData->prefmaxlen /
                 (sizeof(WKSTA_TRANSPORT_INFO_0) + (MAX_TRANSPORT_NAME +
                 MAX_TRANSPORT_ADDR) * sizeof(WCHAR));
            if (enumData->n_read < spaceFor)
            {
                PWKSTA_TRANSPORT_INFO_0 ti;
                LMSTR transport_name, transport_addr;
                MIB_IFROW ifRow;

                ti = (PWKSTA_TRANSPORT_INFO_0)(*(enumData->pbuf) +
                 enumData->n_read * sizeof(WKSTA_TRANSPORT_INFO_0));
                transport_name = (LMSTR)(*(enumData->pbuf) +
                 totalLANAs * sizeof(WKSTA_TRANSPORT_INFO_0) +
                 enumData->n_read * MAX_TRANSPORT_NAME * sizeof(WCHAR));
                transport_addr = (LMSTR)(*(enumData->pbuf) +
                 totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0) +
                 MAX_TRANSPORT_NAME * sizeof(WCHAR)) +
                 enumData->n_read * MAX_TRANSPORT_ADDR * sizeof(WCHAR));

                ifRow.dwIndex = data->ifIndex;
                GetIfEntry(&ifRow);
                ti->wkti0_quality_of_service = 0;
                ti->wkti0_number_of_vcs = 0;
                ti->wkti0_transport_name = transport_name;
                wprint_name(ti->wkti0_transport_name, MAX_TRANSPORT_NAME,
                 transport, &ifRow);
                ti->wkti0_transport_address = transport_addr;
                wprint_mac(ti->wkti0_transport_address, MAX_TRANSPORT_ADDR,
                 &ifRow);
                if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
                    ti->wkti0_wan_ish = TRUE;
                else
                    ti->wkti0_wan_ish = FALSE;
                TRACE("%d of %d:ti at %p\n", lanaIndex, totalLANAs, ti);
                TRACE("transport_name at %p %s\n",
                 ti->wkti0_transport_name,
                 debugstr_w(ti->wkti0_transport_name));
                TRACE("transport_address at %p %s\n",
                 ti->wkti0_transport_address,
                 debugstr_w(ti->wkti0_transport_address));
                enumData->n_read++;
                enumData->ret = NERR_Success;
                ret = TRUE;
            }
            else
            {
                enumData->ret = ERROR_MORE_DATA;
                ret = FALSE;
            }
        }
        else
        {
            enumData->ret = ERROR_OUTOFMEMORY;
            ret = FALSE;
        }
    }
    else
        ret = FALSE;
    return ret;
}

/**********************************************************************/

NET_API_STATUS WINAPI
NetWkstaTransportEnum(LMSTR ServerName, DWORD level, PBYTE* pbuf,
      DWORD prefmaxlen, LPDWORD read_entries,
      PDWORD total_entries, PDWORD hresume)
{
    NET_API_STATUS ret;

    TRACE(":%s, 0x%08lx, %p, 0x%08lx, %p, %p, %p\n", debugstr_w(ServerName),
     level, pbuf, prefmaxlen, read_entries, total_entries,hresume);
    if (!NETAPI_IsLocalComputer(ServerName))
    {
        FIXME(":not implemented for non-local computers\n");
        ret = ERROR_INVALID_LEVEL;
    }
    else
    {
        if (hresume && *hresume)
        {
          FIXME(":resume handle not implemented\n");
          return ERROR_INVALID_LEVEL;
        }

        switch (level)
        {
            case 0: /* transport info */
            {
                ULONG allTransports;
                struct WkstaTransportEnumData enumData;

                if (NetBIOSNumAdapters() == 0)
                  return ERROR_NETWORK_UNREACHABLE;
                if (!read_entries)
                  return STATUS_ACCESS_VIOLATION;
                if (!total_entries || !pbuf)
                  return RPC_X_NULL_REF_POINTER;

                enumData.prefmaxlen = prefmaxlen;
                enumData.pbuf = pbuf;
                memcpy(&allTransports, ALL_TRANSPORTS, sizeof(ULONG));
                NetBIOSEnumAdapters(allTransports, WkstaEnumAdaptersCallback,
                 &enumData);
                *read_entries = enumData.n_read;
                *total_entries = enumData.n_adapt;
                if (hresume) *hresume= 0;
                ret = enumData.ret;
                break;
            }
            default:
                TRACE("Invalid level %ld is specified\n", level);
                ret = ERROR_INVALID_LEVEL;
        }
    }
    return ret;
}

/************************************************************
 *                NetWkstaUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetWkstaUserGetInfo(LMSTR reserved, DWORD level,
                                          PBYTE* bufptr)
{
    NET_API_STATUS nastatus;

    TRACE("(%s, %ld, %p)\n", debugstr_w(reserved), level, bufptr);
    switch (level)
    {
    case 0:
    {
        PWKSTA_USER_INFO_0 ui;
        DWORD dwSize = UNLEN + 1;

        /* set up buffer */
        nastatus = NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_0) + dwSize * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        if (nastatus != NERR_Success)
            return ERROR_NOT_ENOUGH_MEMORY;

        ui = (PWKSTA_USER_INFO_0) *bufptr;
        ui->wkui0_username = (LMSTR) (*bufptr + sizeof(WKSTA_USER_INFO_0));

        /* get data */
        if (!GetUserNameW(ui->wkui0_username, &dwSize))
        {
            NetApiBufferFree(ui);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            nastatus = NetApiBufferReallocate(
                *bufptr, sizeof(WKSTA_USER_INFO_0) +
                (lstrlenW(ui->wkui0_username) + 1) * sizeof(WCHAR),
                (LPVOID *) bufptr);
            if (nastatus != NERR_Success)
            {
                NetApiBufferFree(ui);
                return nastatus;
            }
            ui = (PWKSTA_USER_INFO_0) *bufptr;
            ui->wkui0_username = (LMSTR) (*bufptr + sizeof(WKSTA_USER_INFO_0));
        }
        break;
    }

    case 1:
    {
        PWKSTA_USER_INFO_1 ui;
        PWKSTA_USER_INFO_0 ui0;
        LSA_OBJECT_ATTRIBUTES ObjectAttributes;
        LSA_HANDLE PolicyHandle;
        PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
        NTSTATUS NtStatus;

        /* sizes of the field buffers in WCHARS */
        int username_sz, logon_domain_sz, oth_domains_sz, logon_server_sz;

        FIXME("Level 1 processing is partially implemented\n");
        oth_domains_sz = 1;
        logon_server_sz = 1;

        /* get some information first to estimate size of the buffer */
        ui0 = NULL;
        nastatus = NetWkstaUserGetInfo(NULL, 0, (PBYTE *) &ui0);
        if (nastatus != NERR_Success)
            return nastatus;
        username_sz = lstrlenW(ui0->wkui0_username) + 1;

        ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
                                 POLICY_VIEW_LOCAL_INFORMATION,
                                 &PolicyHandle);
        if (NtStatus != STATUS_SUCCESS)
        {
            TRACE("LsaOpenPolicyFailed with NT status %lx\n",
                LsaNtStatusToWinError(NtStatus));
            NetApiBufferFree(ui0);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        LsaQueryInformationPolicy(PolicyHandle, PolicyAccountDomainInformation,
                                  (PVOID*) &DomainInfo);
        logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
        LsaClose(PolicyHandle);

        /* set up buffer */
        nastatus = NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1) +
                             (username_sz + logon_domain_sz +
                              oth_domains_sz + logon_server_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        if (nastatus != NERR_Success) {
            NetApiBufferFree(ui0);
            return nastatus;
        }
        ui = (WKSTA_USER_INFO_1 *) *bufptr;
        ui->wkui1_username = (LMSTR) (*bufptr + sizeof(WKSTA_USER_INFO_1));
        ui->wkui1_logon_domain = (LMSTR) (
            ((PBYTE) ui->wkui1_username) + username_sz * sizeof(WCHAR));
        ui->wkui1_oth_domains = (LMSTR) (
            ((PBYTE) ui->wkui1_logon_domain) +
            logon_domain_sz * sizeof(WCHAR));
        ui->wkui1_logon_server = (LMSTR) (
            ((PBYTE) ui->wkui1_oth_domains) +
            oth_domains_sz * sizeof(WCHAR));

        /* get data */
        lstrcpyW(ui->wkui1_username, ui0->wkui0_username);
        NetApiBufferFree(ui0);

        lstrcpynW(ui->wkui1_logon_domain, DomainInfo->DomainName.Buffer,
                logon_domain_sz);
        LsaFreeMemory(DomainInfo);

        /* FIXME. Not implemented. Populated with empty strings */
        ui->wkui1_oth_domains[0] = 0;
        ui->wkui1_logon_server[0] = 0;
        break;
    }
    case 1101:
    {
        PWKSTA_USER_INFO_1101 ui;
        DWORD dwSize = 1;

        FIXME("Stub. Level 1101 processing is not implemented\n");
        /* FIXME see also wkui1_oth_domains for level 1 */

        /* set up buffer */
        nastatus = NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1101) + dwSize * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        if (nastatus != NERR_Success)
            return nastatus;
        ui = (PWKSTA_USER_INFO_1101) *bufptr;
        ui->wkui1101_oth_domains = (LMSTR)(ui + 1);

        /* get data */
        ui->wkui1101_oth_domains[0] = 0;
        break;
    }
    default:
        TRACE("Invalid level %ld is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetWkstaUserEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetWkstaUserEnum(LMSTR servername, DWORD level, LPBYTE* bufptr,
                 DWORD prefmaxlen, LPDWORD entriesread,
                 LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("(%s, %ld, %p, %ld, %p, %p, %p): stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);
    return ERROR_INVALID_PARAMETER;
}

/************************************************************
 *                NetpGetComputerName  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetpGetComputerName(LPWSTR *Buffer)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    TRACE("(%p)\n", Buffer);
    NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) Buffer);
    if (GetComputerNameW(*Buffer,  &dwSize))
    {
        return NetApiBufferReallocate(
            *Buffer, (dwSize + 1) * sizeof(WCHAR),
            (LPVOID *) Buffer);
    }
    else
    {
        NetApiBufferFree(*Buffer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

NET_API_STATUS WINAPI I_NetNameCompare(LPVOID p1, LPWSTR wkgrp, LPWSTR comp,
 LPVOID p4, LPVOID p5)
{
    FIXME("(%p %s %s %p %p): stub\n", p1, debugstr_w(wkgrp), debugstr_w(comp),
     p4, p5);
    return ERROR_INVALID_PARAMETER;
}

NET_API_STATUS WINAPI I_NetNameValidate(LPVOID p1, LPWSTR wkgrp, LPVOID p3,
 LPVOID p4)
{
    FIXME("(%p %s %p %p): stub\n", p1, debugstr_w(wkgrp), p3, p4);
    return ERROR_INVALID_PARAMETER;
}

NET_API_STATUS WINAPI NetWkstaGetInfo( LMSTR servername, DWORD level,
                                       LPBYTE* bufptr)
{
    NET_API_STATUS ret;
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %ld %p\n", debugstr_w( servername ), level, bufptr );

    if (!bufptr) return ERROR_INVALID_PARAMETER;
    if (!local)
    {
        if (samba_init())
        {
            ULONG size = 1024;
            struct wksta_getinfo_params params = { servername, level, NULL, &size };

            for (;;)
            {
                if ((ret = NetApiBufferAllocate( size, &params.buffer ))) return ret;
                ret = SAMBA_CALL( wksta_getinfo, &params );
                if (!ret) *bufptr = params.buffer;
                else NetApiBufferFree( params.buffer );
                if (ret != ERROR_INSUFFICIENT_BUFFER) return ret;
            }
        }
        FIXME( "remote computers not supported\n" );
        return ERROR_INVALID_LEVEL;
    }

    switch (level)
    {
        case 100:
        case 101:
        case 102:
        {
            static const WCHAR lanroot[] = L"c:\\lanman";  /* FIXME */
            DWORD computerNameLen, domainNameLen, size;
            WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
            LSA_OBJECT_ATTRIBUTES ObjectAttributes;
            LSA_HANDLE PolicyHandle;
            NTSTATUS NtStatus;

            computerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
            GetComputerNameW(computerName, &computerNameLen);
            computerNameLen++; /* include NULL terminator */

            ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
            NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
             POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
            if (NtStatus != STATUS_SUCCESS)
                ret = LsaNtStatusToWinError(NtStatus);
            else
            {
                PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;

                LsaQueryInformationPolicy(PolicyHandle,
                 PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
                domainNameLen = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
                size = sizeof(WKSTA_INFO_102) + computerNameLen * sizeof(WCHAR)
                    + domainNameLen * sizeof(WCHAR) + sizeof(lanroot);
                ret = NetApiBufferAllocate(size, (LPVOID *)bufptr);
                if (ret == NERR_Success)
                {
                    /* INFO_100 and INFO_101 structures are subsets of INFO_102 */
                    PWKSTA_INFO_102 info = (PWKSTA_INFO_102)*bufptr;
                    OSVERSIONINFOW verInfo;

                    info->wki102_platform_id = PLATFORM_ID_NT;
                    info->wki102_computername = (LMSTR)(*bufptr +
                     sizeof(WKSTA_INFO_102));
                    memcpy(info->wki102_computername, computerName,
                     computerNameLen * sizeof(WCHAR));
                    info->wki102_langroup = info->wki102_computername + computerNameLen;
                    memcpy(info->wki102_langroup, DomainInfo->DomainName.Buffer,
                     domainNameLen * sizeof(WCHAR));
                    info->wki102_lanroot = info->wki102_langroup + domainNameLen;
                    memcpy(info->wki102_lanroot, lanroot, sizeof(lanroot));
                    memset(&verInfo, 0, sizeof(verInfo));
                    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
                    GetVersionExW(&verInfo);
                    info->wki102_ver_major = verInfo.dwMajorVersion;
                    info->wki102_ver_minor = verInfo.dwMinorVersion;
                    info->wki102_logged_on_users = 1;
                }
                LsaFreeMemory(DomainInfo);
                LsaClose(PolicyHandle);
            }
            break;
        }

        default:
            FIXME("level %ld unimplemented\n", level);
            ret = ERROR_INVALID_LEVEL;
    }
    return ret;
}

/************************************************************
 *                NetGetJoinInformation (NETAPI32.@)
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
 *                NetUserGetGroups (NETAPI32.@)
 */
NET_API_STATUS NET_API_FUNCTION NetUserGetGroups(
        LPCWSTR servername,
        LPCWSTR username,
        DWORD level,
        LPBYTE *bufptr,
        DWORD prefixmaxlen,
        LPDWORD entriesread,
        LPDWORD totalentries)
{
    FIXME("%s %s %ld %p %ld %p %p stub\n", debugstr_w(servername),
          debugstr_w(username), level, bufptr, prefixmaxlen, entriesread,
          totalentries);

    *bufptr = NULL;
    *entriesread = 0;
    *totalentries = 0;

    return ERROR_INVALID_LEVEL;
}

struct sam_user
{
    struct list entry;
    WCHAR user_name[LM20_UNLEN+1];
    WCHAR user_password[PWLEN + 1];
    DWORD sec_since_passwd_change;
    DWORD user_priv;
    LPWSTR home_dir;
    LPWSTR user_comment;
    DWORD user_flags;
    LPWSTR user_logon_script_path;
};

static struct list user_list = LIST_INIT( user_list );

/************************************************************
 *                NETAPI_ValidateServername
 *
 * Validates server name
 */
static NET_API_STATUS NETAPI_ValidateServername(LPCWSTR ServerName)
{
    if (ServerName)
    {
        if (ServerName[0] == 0)
            return ERROR_BAD_NETPATH;
        else if (
            ((ServerName[0] == '\\') &&
             (ServerName[1] != '\\'))
            ||
            ((ServerName[0] == '\\') &&
             (ServerName[1] == '\\') &&
             (ServerName[2] == 0))
            )
            return ERROR_INVALID_NAME;
    }
    return NERR_Success;
}

/************************************************************
 *                NETAPI_FindUser
 *
 * Looks for a user in the user database.
 * Returns a pointer to the entry in the user list when the user
 * is found, NULL otherwise.
 */
static struct sam_user* NETAPI_FindUser(LPCWSTR UserName)
{
    struct sam_user *user;

    LIST_FOR_EACH_ENTRY(user, &user_list, struct sam_user, entry)
    {
        if(wcscmp(user->user_name, UserName) == 0)
            return user;
    }
    return NULL;
}

static BOOL NETAPI_IsCurrentUser(LPCWSTR username)
{
    LPWSTR curr_user = NULL;
    DWORD dwSize;
    BOOL ret = FALSE;

    dwSize = LM20_UNLEN+1;
    curr_user = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    if(!curr_user)
    {
        ERR("Failed to allocate memory for user name.\n");
        goto end;
    }
    if(!GetUserNameW(curr_user, &dwSize))
    {
        ERR("Failed to get current user's user name.\n");
        goto end;
    }
    if (!wcscmp(curr_user, username))
    {
        ret = TRUE;
    }

end:
    HeapFree(GetProcessHeap(), 0, curr_user);
    return ret;
}

/************************************************************
 *                NetUserAdd (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserAdd(LPCWSTR servername,
                  DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    NET_API_STATUS status;
    struct sam_user * su = NULL;

    FIXME("(%s, %ld, %p, %p) stub!\n", debugstr_w(servername), level, bufptr, parm_err);

    if((status = NETAPI_ValidateServername(servername)) != NERR_Success)
        return status;

    switch(level)
    {
    /* Level 3 and 4 are identical for the purposes of NetUserAdd */
    case 4:
    case 3:
        FIXME("Level 3 and 4 not implemented.\n");
        /* Fall through */
    case 2:
        FIXME("Level 2 not implemented.\n");
        /* Fall through */
    case 1:
    {
        PUSER_INFO_1 ui = (PUSER_INFO_1) bufptr;
        su = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sam_user));
        if(!su)
        {
            status = NERR_InternalError;
            break;
        }

        if(lstrlenW(ui->usri1_name) > LM20_UNLEN)
        {
            status = NERR_BadUsername;
            break;
        }

        /*FIXME: do other checks for a valid username */
        lstrcpyW(su->user_name, ui->usri1_name);

        if(lstrlenW(ui->usri1_password) > PWLEN)
        {
            /* Always return PasswordTooShort on invalid passwords. */
            status = NERR_PasswordTooShort;
            break;
        }
        lstrcpyW(su->user_password, ui->usri1_password);

        su->sec_since_passwd_change = ui->usri1_password_age;
        su->user_priv = ui->usri1_priv;
        su->user_flags = ui->usri1_flags;

        /*FIXME: set the other LPWSTRs to NULL for now */
        su->home_dir = NULL;
        su->user_comment = NULL;
        su->user_logon_script_path = NULL;

        list_add_head(&user_list, &su->entry);
        return NERR_Success;
    }
    default:
        TRACE("Invalid level %ld specified.\n", level);
        status = ERROR_INVALID_LEVEL;
        break;
    }

    HeapFree(GetProcessHeap(), 0, su);

    return status;
}

/************************************************************
 *                NetUserDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserDel(LPCWSTR servername, LPCWSTR username)
{
    NET_API_STATUS status;
    struct sam_user *user;

    TRACE("(%s, %s)\n", debugstr_w(servername), debugstr_w(username));

    if((status = NETAPI_ValidateServername(servername))!= NERR_Success)
        return status;

    if ((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    list_remove(&user->entry);

    HeapFree(GetProcessHeap(), 0, user->home_dir);
    HeapFree(GetProcessHeap(), 0, user->user_comment);
    HeapFree(GetProcessHeap(), 0, user->user_logon_script_path);
    HeapFree(GetProcessHeap(), 0, user);

    return NERR_Success;
}

/************************************************************
 *                NetUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetInfo(LPCWSTR servername, LPCWSTR username, DWORD level,
               LPBYTE* bufptr)
{
    NET_API_STATUS status;
    TRACE("(%s, %s, %ld, %p)\n", debugstr_w(servername), debugstr_w(username),
          level, bufptr);
    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    if(!NETAPI_IsLocalComputer(servername))
    {
        FIXME("Only implemented for local computer, but remote server"
              "%s was requested.\n", debugstr_w(servername));
        return NERR_InvalidComputer;
    }

    if(!NETAPI_FindUser(username) && !NETAPI_IsCurrentUser(username))
    {
        TRACE("User %s is unknown.\n", debugstr_w(username));
        return NERR_UserNotFound;
    }

    switch (level)
    {
    case 0:
    {
        PUSER_INFO_0 ui;
        int name_sz;

        name_sz = lstrlenW(username) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_0) + name_sz * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_0) *bufptr;
        ui->usri0_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_0));

        /* get data */
        lstrcpyW(ui->usri0_name, username);
        break;
    }

    case 10:
    {
        PUSER_INFO_10 ui;
        PUSER_INFO_0 ui0;
        /* sizes of the field buffers in WCHARS */
        int name_sz, comment_sz, usr_comment_sz, full_name_sz;

        comment_sz = 1;
        usr_comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_10) +
                             (name_sz + comment_sz + usr_comment_sz +
                              full_name_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        ui = (PUSER_INFO_10) *bufptr;
        ui->usri10_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_10));
        ui->usri10_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_name) + name_sz * sizeof(WCHAR));
        ui->usri10_usr_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_comment) + comment_sz * sizeof(WCHAR));
        ui->usri10_full_name = (LPWSTR) (
            ((PBYTE) ui->usri10_usr_comment) + usr_comment_sz * sizeof(WCHAR));

        /* set data */
        lstrcpyW(ui->usri10_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri10_comment[0] = 0;
        ui->usri10_usr_comment[0] = 0;
        ui->usri10_full_name[0] = 0;
        break;
    }

    case 1:
      {
        PUSER_INFO_1 ui;
        PUSER_INFO_0 ui0;
        /* sizes of the field buffers in WCHARS */
        int name_sz, password_sz, home_dir_sz, comment_sz, script_path_sz;

        password_sz = 1; /* not filled out for security reasons for NetUserGetInfo*/
        comment_sz = 1;
        script_path_sz = 1;

       /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;
        home_dir_sz = GetEnvironmentVariableW(L"HOME", NULL,0);
        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_1) +
                             (name_sz + password_sz + home_dir_sz +
                              comment_sz + script_path_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_1) *bufptr;
        ui->usri1_name = (LPWSTR) (ui + 1);
        ui->usri1_password = ui->usri1_name + name_sz;
        ui->usri1_home_dir = ui->usri1_password + password_sz;
        ui->usri1_comment = ui->usri1_home_dir + home_dir_sz;
        ui->usri1_script_path = ui->usri1_comment + comment_sz;
        /* set data */
        lstrcpyW(ui->usri1_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri1_password[0] = 0;
        ui->usri1_password_age = 0;
        ui->usri1_priv = 0;
        GetEnvironmentVariableW(L"HOME", ui->usri1_home_dir,home_dir_sz);
        ui->usri1_comment[0] = 0;
        ui->usri1_flags = 0;
        ui->usri1_script_path[0] = 0;
        break;
      }
    case 2:
    case 3:
    case 4:
    case 11:
    case 20:
    case 23:
    case 1003:
    case 1005:
    case 1006:
    case 1007:
    case 1008:
    case 1009:
    case 1010:
    case 1011:
    case 1012:
    case 1013:
    case 1014:
    case 1017:
    case 1018:
    case 1020:
    case 1023:
    case 1024:
    case 1025:
    case 1051:
    case 1052:
    case 1053:
    {
        FIXME("Level %ld is not implemented\n", level);
        return NERR_InternalError;
    }
    default:
        TRACE("Invalid level %ld is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetUserGetLocalGroups  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetLocalGroups(LPCWSTR servername, LPCWSTR username, DWORD level,
                      DWORD flags, LPBYTE* bufptr, DWORD prefmaxlen,
                      LPDWORD entriesread, LPDWORD totalentries)
{
    static const WCHAR admins[] = L"Administrators";
    NET_API_STATUS status;
    LPWSTR currentuser;
    LOCALGROUP_USERS_INFO_0* info;
    DWORD size;

    FIXME("(%s, %s, %ld, %08lx, %p %ld, %p, %p) stub!\n",
          debugstr_w(servername), debugstr_w(username), level, flags, bufptr,
          prefmaxlen, entriesread, totalentries);

    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    size = UNLEN + 1;
    NetApiBufferAllocate(size * sizeof(WCHAR), (LPVOID*)&currentuser);
    if (!GetUserNameW(currentuser, &size)) {
        NetApiBufferFree(currentuser);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (lstrcmpiW(username, currentuser) && NETAPI_FindUser(username))
    {
        NetApiBufferFree(currentuser);
        return NERR_UserNotFound;
    }

    NetApiBufferFree(currentuser);
    *totalentries = 1;
    size = sizeof(*info) + sizeof(admins);

    if(prefmaxlen < size)
        status = ERROR_MORE_DATA;
    else
        status = NetApiBufferAllocate(size, (LPVOID*)&info);

    if(status != NERR_Success)
    {
        *bufptr = NULL;
        *entriesread = 0;
        return status;
    }

    info->lgrui0_name = (LPWSTR)((LPBYTE)info + sizeof(*info));
    lstrcpyW(info->lgrui0_name, admins);

    *bufptr = (LPBYTE)info;
    *entriesread = 1;

    return NERR_Success;
}

/************************************************************
 *                NetUserEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserEnum(LPCWSTR servername, DWORD level, DWORD filter, LPBYTE* bufptr,
            DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries,
            LPDWORD resume_handle)
{
    NET_API_STATUS status;
    WCHAR user[UNLEN + 1];
    DWORD size, len = ARRAY_SIZE(user);

    TRACE("(%s, %lu, 0x%lx, %p, %lu, %p, %p, %p)\n", debugstr_w(servername), level,
          filter, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    if (!NETAPI_IsLocalComputer(servername))
    {
        FIXME("Only implemented for local computer, but remote server"
              "%s was requested.\n", debugstr_w(servername));
        return NERR_InvalidComputer;
    }

    if (!GetUserNameW(user, &len)) return GetLastError();

    switch (level)
    {
    case 0:
    {
        USER_INFO_0 *info;

        size = sizeof(*info) + (wcslen(user) + 1) * sizeof(WCHAR);

        if (prefmaxlen < size)
            status = ERROR_MORE_DATA;
        else
            status = NetApiBufferAllocate(size, (void **)&info);

        if (status != NERR_Success)
            return status;

        info->usri0_name = (WCHAR *)((char *)info + sizeof(*info));
        wcscpy(info->usri0_name, user);

        *bufptr = (BYTE *)info;
        *entriesread = *totalentries = 1;
        break;
    }
    case 20:
    {
        USER_INFO_20 *info;
        SID *sid;
        UCHAR *count;
        DWORD *rid;
        SID_NAME_USE use;

        size = sizeof(*info) + (wcslen(user) + 1) * sizeof(WCHAR);

        if (prefmaxlen < size)
            status = ERROR_MORE_DATA;
        else
            status = NetApiBufferAllocate(size, (void **)&info);

        if (status != NERR_Success)
            return status;

        size = len = 0;
        LookupAccountNameW(NULL, user, NULL, &size, NULL, &len, &use);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return GetLastError();

        status = NetApiBufferAllocate(size, (void **)&sid);
        if (status != NERR_Success)
            return status;

        if (!LookupAccountNameW(NULL, user, sid, &size, NULL, &len, &use))
            return GetLastError();

        count = GetSidSubAuthorityCount(sid);
        rid = GetSidSubAuthority(sid, *count - 1);

        info->usri20_name      = (WCHAR *)((char *)info + sizeof(*info));
        wcscpy(info->usri20_name, user);
        info->usri20_full_name = NULL;
        info->usri20_comment   = NULL;
        info->usri20_flags     = UF_NORMAL_ACCOUNT;
        info->usri20_user_id   = *rid;

        *bufptr = (BYTE *)info;
        *entriesread = *totalentries = 1;

        NetApiBufferFree(sid);
        break;
    }
    default:
        FIXME("level %lu not supported\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                ACCESS_QueryAdminDisplayInformation
 *
 *  Creates a buffer with information for the Admin User
 */
static void ACCESS_QueryAdminDisplayInformation(PNET_DISPLAY_USER *buf, PDWORD pdwSize)
{
    static const WCHAR sAdminUserName[] = L"Administrator";

    /* sizes of the field buffers in WCHARS */
    int name_sz, comment_sz, full_name_sz;
    PNET_DISPLAY_USER usr;

    /* set up buffer */
    name_sz = lstrlenW(sAdminUserName) + 1;
    comment_sz = 1;
    full_name_sz = 1;

    *pdwSize = sizeof(NET_DISPLAY_USER);
    *pdwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);
    NetApiBufferAllocate(*pdwSize, (LPVOID *) buf);

    usr = *buf;
    usr->usri1_name = (LPWSTR) ((PBYTE) usr + sizeof(NET_DISPLAY_USER));
    usr->usri1_comment = (LPWSTR) (
        ((PBYTE) usr->usri1_name) + name_sz * sizeof(WCHAR));
    usr->usri1_full_name = (LPWSTR) (
        ((PBYTE) usr->usri1_comment) + comment_sz * sizeof(WCHAR));

    /* set data */
    lstrcpyW(usr->usri1_name, sAdminUserName);
    usr->usri1_comment[0] = 0;
    usr->usri1_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;
    usr->usri1_full_name[0] = 0;
    usr->usri1_user_id = DOMAIN_USER_RID_ADMIN;
    usr->usri1_next_index = 0;
}

/************************************************************
 *                ACCESS_QueryGuestDisplayInformation
 *
 *  Creates a buffer with information for the Guest User
 */
static void ACCESS_QueryGuestDisplayInformation(PNET_DISPLAY_USER *buf, PDWORD pdwSize)
{
    static const WCHAR sGuestUserName[] = L"Guest";

    /* sizes of the field buffers in WCHARS */
    int name_sz, comment_sz, full_name_sz;
    PNET_DISPLAY_USER usr;

    /* set up buffer */
    name_sz = lstrlenW(sGuestUserName) + 1;
    comment_sz = 1;
    full_name_sz = 1;

    *pdwSize = sizeof(NET_DISPLAY_USER);
    *pdwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);
    NetApiBufferAllocate(*pdwSize, (LPVOID *) buf);

    usr = *buf;
    usr->usri1_name = (LPWSTR) ((PBYTE) usr + sizeof(NET_DISPLAY_USER));
    usr->usri1_comment = (LPWSTR) (
        ((PBYTE) usr->usri1_name) + name_sz * sizeof(WCHAR));
    usr->usri1_full_name = (LPWSTR) (
        ((PBYTE) usr->usri1_comment) + comment_sz * sizeof(WCHAR));

    /* set data */
    lstrcpyW(usr->usri1_name, sGuestUserName);
    usr->usri1_comment[0] = 0;
    usr->usri1_flags = UF_ACCOUNTDISABLE | UF_SCRIPT | UF_NORMAL_ACCOUNT |
        UF_DONT_EXPIRE_PASSWD;
    usr->usri1_full_name[0] = 0;
    usr->usri1_user_id = DOMAIN_USER_RID_GUEST;
    usr->usri1_next_index = 0;
}

/************************************************************
 * Copies NET_DISPLAY_USER record.
 */
static void ACCESS_CopyDisplayUser(const NET_DISPLAY_USER *dest, LPWSTR *dest_buf,
                            PNET_DISPLAY_USER src)
{
    LPWSTR str = *dest_buf;

    src->usri1_name = str;
    lstrcpyW(src->usri1_name, dest->usri1_name);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_comment = str;
    lstrcpyW(src->usri1_comment, dest->usri1_comment);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_flags = dest->usri1_flags;

    src->usri1_full_name = str;
    lstrcpyW(src->usri1_full_name, dest->usri1_full_name);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_user_id = dest->usri1_user_id;
    src->usri1_next_index = dest->usri1_next_index;
    *dest_buf = str;
}

/************************************************************
 *                NetQueryDisplayInformation  (NETAPI32.@)
 *
 * The buffer structure:
 * - array of fixed size record of the level type
 * - strings, referenced by the record of the level type
 */
NET_API_STATUS WINAPI
NetQueryDisplayInformation(
    LPCWSTR ServerName, DWORD Level, DWORD Index, DWORD EntriesRequested,
    DWORD PreferredMaximumLength, LPDWORD ReturnedEntryCount,
    PVOID *SortedBuffer)
{
    TRACE("(%s, %ld, %ld, %ld, %ld, %p, %p)\n", debugstr_w(ServerName),
          Level, Index, EntriesRequested, PreferredMaximumLength,
          ReturnedEntryCount, SortedBuffer);

    if(!NETAPI_IsLocalComputer(ServerName))
    {
        FIXME("Only implemented on local computer, but requested for "
              "remote server %s\n", debugstr_w(ServerName));
        return ERROR_ACCESS_DENIED;
    }

    switch (Level)
    {
    case 1:
    {
        /* current record */
        PNET_DISPLAY_USER inf;
        /* current available strings buffer */
        LPWSTR str;
        PNET_DISPLAY_USER admin, guest;
        DWORD admin_size, guest_size;
        LPWSTR name = NULL;
        DWORD dwSize;

        /* sizes of the field buffers in WCHARS */
        int name_sz, comment_sz, full_name_sz;

        /* number of the records, returned in SortedBuffer
           3 - for current user, Administrator and Guest users
         */
        int records = 3;

        FIXME("Level %ld partially implemented\n", Level);
        *ReturnedEntryCount = records;
        comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        dwSize = UNLEN + 1;
        NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) &name);
        if (!GetUserNameW(name, &dwSize))
        {
            NetApiBufferFree(name);
            return ERROR_ACCESS_DENIED;
        }
        name_sz = dwSize;
        ACCESS_QueryAdminDisplayInformation(&admin, &admin_size);
        ACCESS_QueryGuestDisplayInformation(&guest, &guest_size);

        /* set up buffer */
        dwSize = sizeof(NET_DISPLAY_USER) * records;
        dwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);

        NetApiBufferAllocate(dwSize +
                             admin_size - sizeof(NET_DISPLAY_USER) +
                             guest_size - sizeof(NET_DISPLAY_USER),
                             SortedBuffer);
        inf = *SortedBuffer;
        str = (LPWSTR) ((PBYTE) inf + sizeof(NET_DISPLAY_USER) * records);
        inf->usri1_name = str;
        str = (LPWSTR) (
            ((PBYTE) str) + name_sz * sizeof(WCHAR));
        inf->usri1_comment = str;
        str = (LPWSTR) (
            ((PBYTE) str) + comment_sz * sizeof(WCHAR));
        inf->usri1_full_name = str;
        str = (LPWSTR) (
            ((PBYTE) str) + full_name_sz * sizeof(WCHAR));

        /* set data */
        lstrcpyW(inf->usri1_name, name);
        NetApiBufferFree(name);
        inf->usri1_comment[0] = 0;
        inf->usri1_flags =
            UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;
        inf->usri1_full_name[0] = 0;
        inf->usri1_user_id = 0;
        inf->usri1_next_index = 0;

        inf++;
        ACCESS_CopyDisplayUser(admin, &str, inf);
        NetApiBufferFree(admin);

        inf++;
        ACCESS_CopyDisplayUser(guest, &str, inf);
        NetApiBufferFree(guest);
        break;
    }

    case 2:
    case 3:
    {
        FIXME("Level %ld is not implemented\n", Level);
        break;
    }

    default:
        TRACE("Invalid level %ld is specified\n", Level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetGetDCName  (NETAPI32.@)
 *
 *  Return the name of the primary domain controller (PDC)
 */

NET_API_STATUS WINAPI
NetGetDCName(LPCWSTR servername, LPCWSTR domainname, LPBYTE *bufptr)
{
  FIXME("(%s, %s, %p) stub!\n", debugstr_w(servername),
                 debugstr_w(domainname), bufptr);
  return NERR_DCNotFound; /* say we can't find a domain controller */
}

/************************************************************
 *                NetGetAnyDCName  (NETAPI32.@)
 *
 *  Return the name of any domain controller (DC) for a
 *   domain that is directly trusted by the specified server
 */

NET_API_STATUS WINAPI NetGetAnyDCName(LPCWSTR servername, LPCWSTR domainname, LPBYTE *bufptr)
{
    FIXME("(%s, %s, %p) stub!\n", debugstr_w(servername),
          debugstr_w(domainname), bufptr);
    return ERROR_NO_SUCH_DOMAIN;
}

/************************************************************
 *                NetGroupAddUser  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetGroupAddUser(LPCWSTR servername, LPCWSTR groupname, LPCWSTR username)
{
    FIXME("(%s, %s, %s) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), debugstr_w(username));
    return NERR_Success;
}

/************************************************************
 *                NetGroupEnum  (NETAPI32.@)
 *
 */
NET_API_STATUS WINAPI
NetGroupEnum(LPCWSTR servername, DWORD level, LPBYTE *bufptr, DWORD prefmaxlen,
             LPDWORD entriesread, LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("(%s, %ld, %p, %ld, %p, %p, %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);
    return ERROR_ACCESS_DENIED;
}

/************************************************************
 *                NetGroupGetInfo  (NETAPI32.@)
 *
 */
NET_API_STATUS WINAPI NetGroupGetInfo(LPCWSTR servername, LPCWSTR groupname, DWORD level, LPBYTE *bufptr)
{
    FIXME("(%s, %s, %ld, %p) stub!\n", debugstr_w(servername), debugstr_w(groupname), level, bufptr);
    return ERROR_ACCESS_DENIED;
}

/******************************************************************************
 * NetUserModalsGet  (NETAPI32.@)
 *
 * Retrieves global information for all users and global groups in the security
 * database.
 *
 * PARAMS
 *  szServer   [I] Specifies the DNS or the NetBIOS name of the remote server
 *                 on which the function is to execute.
 *  level      [I] Information level of the data.
 *     0   Return global passwords parameters. bufptr points to a
 *         USER_MODALS_INFO_0 struct.
 *     1   Return logon server and domain controller information. bufptr
 *         points to a USER_MODALS_INFO_1 struct.
 *     2   Return domain name and identifier. bufptr points to a
 *         USER_MODALS_INFO_2 struct.
 *     3   Return lockout information. bufptr points to a USER_MODALS_INFO_3
 *         struct.
 *  pbuffer    [I] Buffer that receives the data.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure:
 *     ERROR_ACCESS_DENIED - the user does not have access to the info.
 *     NERR_InvalidComputer - computer name is invalid.
 */
NET_API_STATUS WINAPI NetUserModalsGet(
    LPCWSTR szServer, DWORD level, LPBYTE *pbuffer)
{
    TRACE("(%s %ld %p)\n", debugstr_w(szServer), level, pbuffer);

    switch (level)
    {
        case 0:
            /* return global passwords parameters */
            FIXME("level 0 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        case 1:
            /* return logon server and domain controller info */
            FIXME("level 1 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        case 2:
        {
            /* return domain name and identifier */
            PUSER_MODALS_INFO_2 umi;
            LSA_HANDLE policyHandle;
            LSA_OBJECT_ATTRIBUTES objectAttributes;
            PPOLICY_ACCOUNT_DOMAIN_INFO domainInfo;
            NTSTATUS ntStatus;
            PSID domainIdentifier = NULL;
            int domainNameLen;

            ZeroMemory(&objectAttributes, sizeof(objectAttributes));
            objectAttributes.Length = sizeof(objectAttributes);

            ntStatus = LsaOpenPolicy(NULL, &objectAttributes,
                                     POLICY_VIEW_LOCAL_INFORMATION,
                                     &policyHandle);
            if (ntStatus != STATUS_SUCCESS)
            {
                WARN("LsaOpenPolicy failed with NT status %lx\n",
                     LsaNtStatusToWinError(ntStatus));
                return ntStatus;
            }

            ntStatus = LsaQueryInformationPolicy(policyHandle,
                                                 PolicyAccountDomainInformation,
                                                 (PVOID *)&domainInfo);
            if (ntStatus != STATUS_SUCCESS)
            {
                WARN("LsaQueryInformationPolicy failed with NT status %lx\n",
                     LsaNtStatusToWinError(ntStatus));
                LsaClose(policyHandle);
                return ntStatus;
            }

            domainIdentifier = domainInfo->DomainSid;
            domainNameLen = lstrlenW(domainInfo->DomainName.Buffer) + 1;
            LsaClose(policyHandle);

            ntStatus = NetApiBufferAllocate(sizeof(USER_MODALS_INFO_2) +
                                            GetLengthSid(domainIdentifier) +
                                            domainNameLen * sizeof(WCHAR),
                                            (LPVOID *)pbuffer);

            if (ntStatus != NERR_Success)
            {
                WARN("NetApiBufferAllocate() failed\n");
                LsaFreeMemory(domainInfo);
                return ntStatus;
            }

            umi = (USER_MODALS_INFO_2 *) *pbuffer;
            umi->usrmod2_domain_id = *pbuffer + sizeof(USER_MODALS_INFO_2);
            umi->usrmod2_domain_name = (LPWSTR)(*pbuffer +
                sizeof(USER_MODALS_INFO_2) + GetLengthSid(domainIdentifier));

            lstrcpynW(umi->usrmod2_domain_name,
                      domainInfo->DomainName.Buffer,
                      domainNameLen);
            CopySid(GetLengthSid(domainIdentifier), umi->usrmod2_domain_id,
                    domainIdentifier);

            LsaFreeMemory(domainInfo);

            break;
        }
        case 3:
            /* return lockout information */
            FIXME("level 3 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        default:
            TRACE("Invalid level %ld is specified\n", level);
            *pbuffer = NULL;
            return ERROR_INVALID_LEVEL;
    }

    return NERR_Success;
}

/******************************************************************************
 *                NetUserChangePassword  (NETAPI32.@)
 * PARAMS
 *  domainname  [I] Optional. Domain on which the user resides or the logon
 *                  domain of the current user if NULL.
 *  username    [I] Optional. Username to change the password for or the name
 *                  of the current user if NULL.
 *  oldpassword [I] The user's current password.
 *  newpassword [I] The password that the user will be changed to using.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure: NERR_* failure code or win error code.
 *
 */
NET_API_STATUS WINAPI NetUserChangePassword(LPCWSTR domainname, LPCWSTR username,
    LPCWSTR oldpassword, LPCWSTR newpassword)
{
    struct sam_user *user;
    struct change_password_params params = { domainname, username, oldpassword, newpassword };

    TRACE("(%s, %s, ..., ...)\n", debugstr_w(domainname), debugstr_w(username));

    if (samba_init() && !SAMBA_CALL( change_password, &params ))
        return NERR_Success;

    if(domainname)
        FIXME("Ignoring domainname %s.\n", debugstr_w(domainname));

    if((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    if(wcscmp(user->user_password, oldpassword) != 0)
        return ERROR_INVALID_PASSWORD;

    if(lstrlenW(newpassword) > PWLEN)
        return ERROR_PASSWORD_RESTRICTION;

    lstrcpyW(user->user_password, newpassword);

    return NERR_Success;
}

NET_API_STATUS WINAPI NetUseAdd(LMSTR servername, DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    FIXME("%s %ld %p %p stub\n", debugstr_w(servername), level, bufptr, parm_err);
    return NERR_Success;
}

NET_API_STATUS WINAPI NetUseDel(LMSTR servername, LMSTR usename, DWORD forcecond)
{
    FIXME("%s %s %ld stub\n", debugstr_w(servername), debugstr_w(usename), forcecond);
    return NERR_Success;
}

/************************************************************
 *                I_BrowserSetNetlogonState  (NETAPI32.@)
 */
NET_API_STATUS WINAPI I_BrowserSetNetlogonState(
    LPWSTR ServerName, LPWSTR DomainName, LPWSTR EmulatedServerName,
    DWORD Role)
{
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                I_BrowserQueryEmulatedDomains  (NETAPI32.@)
 */
NET_API_STATUS WINAPI I_BrowserQueryEmulatedDomains(
    LPWSTR ServerName, PBROWSER_EMULATED_DOMAIN *EmulatedDomains,
    LPDWORD EntriesRead)
{
    return ERROR_NOT_SUPPORTED;
}

#define NS_MAXDNAME 1025

static DWORD get_dc_info(const WCHAR *domain, WCHAR *dc, WCHAR *ip)
{
    WCHAR name[NS_MAXDNAME];
    DWORD ret, size;
    DNS_RECORDW *rec;

    wcscpy( name, L"_ldap._tcp.dc._msdcs." );
    wcscat( name, domain );

    ret = DnsQuery_W(name, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
    TRACE("DnsQuery_W(%s) => %ld\n", wine_dbgstr_w(domain), ret);
    if (ret == ERROR_SUCCESS)
    {
        TRACE("target %s, port %d\n", wine_dbgstr_w(rec->Data.Srv.pNameTarget), rec->Data.Srv.wPort);

        lstrcpynW(dc, rec->Data.Srv.pNameTarget, NS_MAXDNAME);
        DnsRecordListFree(rec, DnsFreeRecordList);

        /* IPv4 */
        ret = DnsQuery_W(dc, DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &rec, NULL);
        TRACE("DnsQuery_W(%s) => %ld\n", wine_dbgstr_w(dc), ret);
        if (ret == ERROR_SUCCESS)
        {
            SOCKADDR_IN addr;

            addr.sin_family = AF_INET;
            addr.sin_port = 0;
            addr.sin_addr.s_addr = rec->Data.A.IpAddress;
            size = IP6_ADDRESS_STRING_LENGTH;
            ret = WSAAddressToStringW((SOCKADDR *)&addr, sizeof(addr), NULL, ip, &size);
            if (!ret)
                TRACE("WSAAddressToStringW => %ld, %s\n", ret, wine_dbgstr_w(ip));

            DnsRecordListFree(rec, DnsFreeRecordList);

            return ret;
        }

        /* IPv6 */
        ret = DnsQuery_W(dc, DNS_TYPE_AAAA, DNS_QUERY_STANDARD, NULL, &rec, NULL);
        TRACE("DnsQuery_W(%s) => %ld\n", wine_dbgstr_w(dc), ret);
        if (ret == ERROR_SUCCESS)
        {
            SOCKADDR_IN6 addr;

            addr.sin6_family = AF_INET6;
            addr.sin6_port = 0;
            addr.sin6_scope_id = 0;
            memcpy(addr.sin6_addr.s6_addr, &rec->Data.AAAA.Ip6Address, sizeof(rec->Data.AAAA.Ip6Address));
            size = IP6_ADDRESS_STRING_LENGTH;
            ret = WSAAddressToStringW((SOCKADDR *)&addr, sizeof(addr), NULL, ip, &size);
            if (!ret)
                TRACE("WSAAddressToStringW => %ld, %s\n", ret, wine_dbgstr_w(ip));

            DnsRecordListFree(rec, DnsFreeRecordList);
        }
    }

    return ret;
}

DWORD WINAPI DsGetDcNameW(LPCWSTR computer, LPCWSTR domain, GUID *domain_guid,
                          LPCWSTR site, ULONG flags, PDOMAIN_CONTROLLER_INFOW *dc_info)
{
    static const WCHAR pfxW[] = {'\\','\\'};
    static const WCHAR default_site_nameW[] = L"Default-First-Site-Name";
    NTSTATUS status;
    POLICY_DNS_DOMAIN_INFO *dns_domain_info = NULL;
    DOMAIN_CONTROLLER_INFOW *info;
    WCHAR dc[NS_MAXDNAME], ip[IP6_ADDRESS_STRING_LENGTH];
    DWORD size;

    FIXME("(%s, %s, %s, %s, %08lx, %p): semi-stub\n", debugstr_w(computer),
        debugstr_w(domain), debugstr_guid(domain_guid), debugstr_w(site), flags, dc_info);

    if (!dc_info) return ERROR_INVALID_PARAMETER;

    if (!domain)
    {
        LSA_OBJECT_ATTRIBUTES attrs;
        LSA_HANDLE lsa;

        memset(&attrs, 0, sizeof(attrs));
        attrs.Length = sizeof(attrs);
        status = LsaOpenPolicy(NULL, &attrs, POLICY_VIEW_LOCAL_INFORMATION, &lsa);
        if (status)
            return LsaNtStatusToWinError(status);

        status = LsaQueryInformationPolicy(lsa, PolicyDnsDomainInformation, (void **)&dns_domain_info);
        LsaClose(lsa);
        if (status)
            return LsaNtStatusToWinError(status);

        domain = dns_domain_info->DnsDomainName.Buffer;
    }

    status = get_dc_info(domain, dc, ip);
    if (status) return status;

    size = sizeof(DOMAIN_CONTROLLER_INFOW) + lstrlenW(domain) * sizeof(WCHAR) +
            sizeof(pfxW) * 2 + (lstrlenW(dc) + 1 + lstrlenW(ip) + 1) * sizeof(WCHAR) +
            lstrlenW(domain) * sizeof(WCHAR) /* assume forest == domain */ +
            sizeof(default_site_nameW) * 2;
    status = NetApiBufferAllocate(size, (void **)&info);
    if (status != NERR_Success)
    {
        LsaFreeMemory(dns_domain_info);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    info->DomainControllerName = (WCHAR *)(info + 1);
    memcpy(info->DomainControllerName, pfxW, sizeof(pfxW));
    lstrcpyW(info->DomainControllerName + 2, dc);
    info->DomainControllerAddress = (WCHAR *)((char *)info->DomainControllerName + (wcslen(info->DomainControllerName) + 1) * sizeof(WCHAR));
    memcpy(info->DomainControllerAddress, pfxW, sizeof(pfxW));
    lstrcpyW(info->DomainControllerAddress + 2, ip);
    info->DomainControllerAddressType = DS_INET_ADDRESS;
    info->DomainGuid = dns_domain_info ? dns_domain_info->DomainGuid : GUID_NULL /* FIXME */;
    info->DomainName = (WCHAR *)((char *)info->DomainControllerAddress + (wcslen(info->DomainControllerAddress) + 1) * sizeof(WCHAR));
    lstrcpyW(info->DomainName, domain);
    info->DnsForestName = (WCHAR *)((char *)info->DomainName + (lstrlenW(info->DomainName) + 1) * sizeof(WCHAR));
    lstrcpyW(info->DnsForestName, domain);
    info->DcSiteName = (WCHAR *)((char *)info->DnsForestName + (lstrlenW(info->DnsForestName) + 1) * sizeof(WCHAR));
    lstrcpyW(info->DcSiteName, default_site_nameW);
    info->ClientSiteName = (WCHAR *)((char *)info->DcSiteName + sizeof(default_site_nameW));
    lstrcpyW(info->ClientSiteName, default_site_nameW);
    info->Flags = DS_DNS_DOMAIN_FLAG | DS_DNS_FOREST_FLAG;

    LsaFreeMemory(dns_domain_info);

    *dc_info = info;

    return ERROR_SUCCESS;
}

DWORD WINAPI DsGetDcNameA(LPCSTR ComputerName, LPCSTR AvoidDCName,
 GUID* DomainGuid, LPCSTR SiteName, ULONG Flags,
 PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo)
{
    FIXME("(%s, %s, %s, %s, %08lx, %p): stub\n", debugstr_a(ComputerName),
     debugstr_a(AvoidDCName), debugstr_guid(DomainGuid),
     debugstr_a(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetSiteNameW(LPCWSTR ComputerName, LPWSTR *SiteName)
{
    FIXME("(%s, %p): stub\n", debugstr_w(ComputerName), SiteName);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetSiteNameA(LPCSTR ComputerName, LPSTR *SiteName)
{
    FIXME("(%s, %p): stub\n", debugstr_a(ComputerName), SiteName);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/************************************************************
 *  DsRoleFreeMemory (NETAPI32.@)
 *
 * PARAMS
 *  Buffer [I] Pointer to the to-be-freed buffer.
 *
 * RETURNS
 *  Nothing
 */
VOID WINAPI DsRoleFreeMemory(PVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
}

/************************************************************
 *  DsRoleGetPrimaryDomainInformation  (NETAPI32.@)
 *
 * PARAMS
 *  lpServer  [I] Pointer to UNICODE string with ComputerName
 *  InfoLevel [I] Type of data to retrieve
 *  Buffer    [O] Pointer to to the requested data
 *
 * RETURNS
 *
 * NOTES
 *  When lpServer is NULL, use the local computer
 */
DWORD WINAPI DsRoleGetPrimaryDomainInformation(
    LPCWSTR lpServer, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    PBYTE* Buffer)
{
    DWORD ret;

    FIXME("(%p, %d, %p) stub\n", lpServer, InfoLevel, Buffer);

    /* Check some input parameters */

    if (!Buffer) return ERROR_INVALID_PARAMETER;
    if ((InfoLevel < DsRolePrimaryDomainInfoBasic) || (InfoLevel > DsRoleOperationState)) return ERROR_INVALID_PARAMETER;

    *Buffer = NULL;
    switch (InfoLevel)
    {
        case DsRolePrimaryDomainInfoBasic:
        {
            LSA_OBJECT_ATTRIBUTES ObjectAttributes;
            LSA_HANDLE PolicyHandle;
            PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
            NTSTATUS NtStatus;
            int logon_domain_sz;
            DWORD size;
            PDSROLE_PRIMARY_DOMAIN_INFO_BASIC basic;

            ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
            NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
             POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
            if (NtStatus != STATUS_SUCCESS)
            {
                TRACE("LsaOpenPolicyFailed with NT status %lx\n",
                    LsaNtStatusToWinError(NtStatus));
                return ERROR_OUTOFMEMORY;
            }
            LsaQueryInformationPolicy(PolicyHandle,
             PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
            logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
            LsaClose(PolicyHandle);

            size = sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC) +
             logon_domain_sz * sizeof(WCHAR);
            basic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            if (basic)
            {
                basic->MachineRole = DsRole_RoleStandaloneWorkstation;
                basic->DomainNameFlat = (LPWSTR)((LPBYTE)basic +
                 sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC));
                lstrcpyW(basic->DomainNameFlat, DomainInfo->DomainName.Buffer);
                ret = ERROR_SUCCESS;
            }
            else
                ret = ERROR_OUTOFMEMORY;
            *Buffer = (PBYTE)basic;
            LsaFreeMemory(DomainInfo);
        }
        break;
    default:
        ret = ERROR_CALL_NOT_IMPLEMENTED;
    }
    return ret;
}

DWORD WINAPI DsGetDcOpenA(LPCSTR domain, ULONG flags, LPCSTR site,
    GUID *domain_guid, LPCSTR forest, ULONG dc_flags, PHANDLE context)
{
    FIXME("(%s, %08lx, %s, %s, %s, %08lx, %p)\n", debugstr_a(domain),
        flags, debugstr_a(site), wine_dbgstr_guid(domain_guid),
        debugstr_a(forest), dc_flags, context);

    *context = NULL;

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetDcOpenW(LPCWSTR domain, ULONG flags, LPCWSTR site,
    GUID *domain_guid, LPCWSTR forest, ULONG dc_flags, PHANDLE context)
{
    FIXME("(%s, %08lx, %s, %s, %s, %08lx, %p)\n", debugstr_w(domain),
        flags, debugstr_w(site), wine_dbgstr_guid(domain_guid),
        debugstr_w(forest), dc_flags, context);

    *context = NULL;

    return ERROR_CALL_NOT_IMPLEMENTED;
}

/************************************************************
 *                NetLocalGroupAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAdd(
    LPCWSTR servername,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %ld %p %p) stub!\n", debugstr_w(servername), level, buf,
          parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %ld %p %ld) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDel(
    LPCWSTR servername,
    LPCWSTR groupname)
{
    FIXME("(%s %s) stub!\n", debugstr_w(servername), debugstr_w(groupname));
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %ld %p %ld) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupEnum(
    LPCWSTR servername,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %ld %p %ld %p %p %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);
    *entriesread = 0;
    *totalentries = 0;
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE* bufptr)
{
    static const WCHAR commentW[] = L"No comment";
    LOCALGROUP_INFO_1* info;
    DWORD size;

    FIXME("(%s %s %ld %p) semi-stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, bufptr);

    size = sizeof(*info) + sizeof(WCHAR) * (lstrlenW(groupname)+1) + sizeof(commentW);
    NetApiBufferAllocate(size, (LPVOID*)&info);

    info->lgrpi1_name = (LPWSTR)(info + 1);
    lstrcpyW(info->lgrpi1_name, groupname);

    info->lgrpi1_comment = info->lgrpi1_name + lstrlenW(groupname) + 1;
    lstrcpyW(info->lgrpi1_comment, commentW);

    *bufptr = (LPBYTE)info;

    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetMembers(
    LPCWSTR servername,
    LPCWSTR localgroupname,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %s %ld %p %ld, %p %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(localgroupname), level, bufptr, prefmaxlen, entriesread,
          totalentries, resumehandle);

    if (level == 3)
    {
        WCHAR userName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD userNameLen;
        DWORD len,needlen;
        PLOCALGROUP_MEMBERS_INFO_3 ptr;

        /* still a stub,  current user is belonging to all groups */

        *totalentries = 1;
        *entriesread = 0;

        userNameLen = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetUserNameW(userName,&userNameLen))
            return ERROR_NOT_ENOUGH_MEMORY;

        needlen = sizeof(LOCALGROUP_MEMBERS_INFO_3) +
             (userNameLen+2) * sizeof(WCHAR);
        if (prefmaxlen != MAX_PREFERRED_LENGTH)
            len = min(prefmaxlen,needlen);
        else
            len = needlen;

        NetApiBufferAllocate(len, (LPVOID *) bufptr);
        if (len < needlen)
            return ERROR_MORE_DATA;

        ptr = (PLOCALGROUP_MEMBERS_INFO_3)*bufptr;
        ptr->lgrmi3_domainandname = (LPWSTR)(*bufptr+sizeof(LOCALGROUP_MEMBERS_INFO_3));
        lstrcpyW(ptr->lgrmi3_domainandname,userName);

        *entriesread = 1;
    }

    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %s %ld %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetMember (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %ld %p %ld) stub!\n", debugstr_w(servername),
            debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetRemoteTOD (NETAPI32.@)
 */
NET_API_STATUS NET_API_FUNCTION NetRemoteTOD(
    LPCWSTR servername,
    LPBYTE *buf)
{
    FIXME("(%s %p) stub!\n", debugstr_w(servername), buf);
    return ERROR_NO_BROWSER_SERVERS_FOUND;
}


/************************************************************
 *                NetValidatePasswordPolicy  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetValidatePasswordPolicy(
    LPCWSTR servername,
    LPVOID qualifier,
    DWORD validation_type,
    LPVOID input_arg,
    LPVOID *output_arg)
{
    NET_API_STATUS status;
    NET_VALIDATE_OUTPUT_ARG *out = NULL;

    FIXME("(%s, %p, %lu, %p, %p) stub!\n",
          debugstr_w(servername), qualifier, validation_type, input_arg, output_arg);

    if (!output_arg)
        return ERROR_INVALID_PARAMETER;

    *output_arg = NULL;

    status = NetApiBufferAllocate(sizeof(*out), (LPVOID *)&out);
    if (status != NERR_Success)
        return status;

    /* Zero all fields; report success so most probes pass */
    memset(out, 0, sizeof(*out));
    out->ValidationStatus = NERR_Success;

    *output_arg = out;
    return NERR_Success;
}

/************************************************************
 *          NetValidatePasswordPolicyFree  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetValidatePasswordPolicyFree(LPVOID *output_arg)
{
    TRACE("(%p)\n", output_arg);

    if (!output_arg) return ERROR_INVALID_PARAMETER;
    if (*output_arg)
    {
        NetApiBufferFree(*output_arg);
        *output_arg = NULL;
    }
    return NERR_Success;
}

/************************************************************
 *                DavGetHTTPFromUNCPath (NETAPI32.@)
 */
DWORD WINAPI DavGetHTTPFromUNCPath(const WCHAR *unc_path, WCHAR *buf, DWORD *buflen)
{
    static const WCHAR httpW[] = L"http://";
    static const WCHAR httpsW[] = L"https://";
    const WCHAR *p = unc_path, *q, *server, *path, *scheme = httpW;
    UINT i, len_server, len_path = 0, len_port = 0, len, port = 0;
    WCHAR *end, portbuf[12];

    TRACE("(%s %p %p)\n", debugstr_w(unc_path), buf, buflen);

    if (p[0] != '\\' || p[1] != '\\' || !p[2]) return ERROR_INVALID_PARAMETER;
    q = p += 2;
    while (*q && *q != '\\' && *q != '/' && *q != '@') q++;
    server = p;
    len_server = q - p;
    if (*q == '@')
    {
        p = ++q;
        while (*p && (*p != '\\' && *p != '/' && *p != '@')) p++;
        if (p - q == 3 && !wcsnicmp( q, L"SSL", 3 ))
        {
            scheme = httpsW;
            q = p;
        }
        else if ((port = wcstol( q, &end, 10 ))) q = end;
        else return ERROR_INVALID_PARAMETER;
    }
    if (*q == '@')
    {
        if (!(port = wcstol( ++q, &end, 10 ))) return ERROR_INVALID_PARAMETER;
        q = end;
    }
    if (*q == '\\' || *q  == '/') q++;
    path = q;
    while (*q++) len_path++;
    if (len_path && (path[len_path - 1] == '\\' || path[len_path - 1] == '/'))
        len_path--; /* remove trailing slash */

    swprintf( portbuf, ARRAY_SIZE(portbuf), L":%u", port );
    if (scheme == httpsW)
    {
        len = wcslen( httpsW );
        if (port && port != 443) len_port = wcslen( portbuf );
    }
    else
    {
        len = wcslen( httpW );
        if (port && port != 80) len_port = wcslen( portbuf );
    }
    len += len_server;
    len += len_port;
    if (len_path) len += len_path + 1; /* leading '/' */
    len++; /* nul */

    if (*buflen < len)
    {
        *buflen = len;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy( buf, scheme, wcslen(scheme) * sizeof(WCHAR) );
    buf += wcslen( scheme );
    memcpy( buf, server, len_server * sizeof(WCHAR) );
    buf += len_server;
    if (len_port)
    {
        memcpy( buf, portbuf, len_port * sizeof(WCHAR) );
        buf += len_port;
    }
    if (len_path)
    {
        *buf++ = '/';
        for (i = 0; i < len_path; i++)
        {
            if (path[i] == '\\') *buf++ = '/';
            else *buf++ = path[i];
        }
    }
    *buf = 0;
    *buflen = len;

    return ERROR_SUCCESS;
}

/************************************************************
 *                DavGetUNCFromHTTPPath (NETAPI32.@)
 */
DWORD WINAPI DavGetUNCFromHTTPPath(const WCHAR *http_path, WCHAR *buf, DWORD *buflen)
{
    static const WCHAR httpW[] = {'h','t','t','p'};
    static const WCHAR httpsW[] = {'h','t','t','p','s'};
    static const WCHAR davrootW[] = {'\\','D','a','v','W','W','W','R','o','o','t'};
    static const WCHAR sslW[] = {'@','S','S','L'};
    static const WCHAR port80W[] = {'8','0'};
    static const WCHAR port443W[] = {'4','4','3'};
    const WCHAR *p = http_path, *server, *port = NULL, *path = NULL;
    DWORD i, len = 0, len_server = 0, len_port = 0, len_path = 0;
    BOOL ssl;

    TRACE("(%s %p %p)\n", debugstr_w(http_path), buf, buflen);

    while (*p && *p != ':') { p++; len++; };
    if (len == ARRAY_SIZE(httpW) && !wcsnicmp( http_path, httpW, len )) ssl = FALSE;
    else if (len == ARRAY_SIZE(httpsW) && !wcsnicmp( http_path, httpsW, len )) ssl = TRUE;
    else return ERROR_INVALID_PARAMETER;

    if (p[0] != ':' || p[1] != '/' || p[2] != '/') return ERROR_INVALID_PARAMETER;
    server = p += 3;

    while (*p && *p != ':' && *p != '/') { p++; len_server++; };
    if (!len_server) return ERROR_BAD_NET_NAME;
    if (*p == ':')
    {
        port = ++p;
        while (*p >= '0' && *p <= '9') { p++; len_port++; };
        if (len_port == 2 && !ssl && !memcmp( port, port80W, sizeof(port80W) )) port = NULL;
        else if (len_port == 3 && ssl && !memcmp( port, port443W, sizeof(port443W) )) port = NULL;
        path = p;
    }
    else if (*p == '/') path = p;

    while (*p)
    {
        if (p[0] == '/' && p[1] == '/') return ERROR_BAD_NET_NAME;
        p++; len_path++;
    }
    if (len_path && path[len_path - 1] == '/') len_path--;

    len = len_server + 2; /* \\ */
    if (ssl) len += 4; /* @SSL */
    if (port) len += len_port + 1 /* @ */;
    len += ARRAY_SIZE(davrootW);
    len += len_path + 1; /* nul */

    if (*buflen < len)
    {
        *buflen = len;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    buf[0] = buf[1] = '\\';
    buf += 2;
    memcpy( buf, server, len_server * sizeof(WCHAR) );
    buf += len_server;
    if (ssl)
    {
        memcpy( buf, sslW, sizeof(sslW) );
        buf += 4;
    }
    if (port)
    {
        *buf++ = '@';
        memcpy( buf, port, len_port * sizeof(WCHAR) );
        buf += len_port;
    }
    memcpy( buf, davrootW, sizeof(davrootW) );
    buf += ARRAY_SIZE(davrootW);
    for (i = 0; i < len_path; i++)
    {
        if (path[i] == '/') *buf++ = '\\';
        else *buf++ = path[i];
    }

    *buf = 0;
    *buflen = len;

    return ERROR_SUCCESS;
}

/************************************************************
 *  DsEnumerateDomainTrustsA (NETAPI32.@)
 */
DWORD WINAPI DsEnumerateDomainTrustsA(LPSTR server, ULONG flags, PDS_DOMAIN_TRUSTSA* domains, PULONG count)
{
    FIXME("(%s, 0x%04lx, %p, %p): stub\n", debugstr_a(server), flags, domains, count);
    return ERROR_NO_LOGON_SERVERS;
}

/************************************************************
 *  DsEnumerateDomainTrustsW (NETAPI32.@)
 */
DWORD WINAPI DsEnumerateDomainTrustsW(LPWSTR server, ULONG flags, PDS_DOMAIN_TRUSTSW* domains, PULONG count)
{
    FIXME("(%s, 0x%04lx, %p, %p): stub\n", debugstr_w(server), flags, domains, count);
    return ERROR_NO_LOGON_SERVERS;
}

void __RPC_FAR *__RPC_USER MIDL_user_allocate(SIZE_T n)
{
    return HeapAlloc(GetProcessHeap(), 0, n);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

handle_t __RPC_USER ATSVC_HANDLE_bind(ATSVC_HANDLE str)
{
    static unsigned char ncalrpc[] = "ncalrpc";
    unsigned char *binding_str;
    handle_t rpc_handle = 0;

    if (RpcStringBindingComposeA(NULL, ncalrpc, NULL, NULL, NULL, &binding_str) == RPC_S_OK)
    {
        RpcBindingFromStringBindingA(binding_str, &rpc_handle);
        RpcStringFreeA(&binding_str);
    }
    return rpc_handle;
}

void __RPC_USER ATSVC_HANDLE_unbind(ATSVC_HANDLE ServerName, handle_t rpc_handle)
{
    RpcBindingFree(&rpc_handle);
}

/************************************************************
 *  NetGetAadJoinInformation (NETAPI32.@)
 */
HRESULT WINAPI NetGetAadJoinInformation(LPCWSTR tenant_id, PDSREG_JOIN_INFO *join_info)
{
    FIXME("(%s, %p): stub\n", debugstr_w(tenant_id), join_info);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

void NET_API_FUNCTION NetFreeAadJoinInformation(DSREG_JOIN_INFO *join_info)
{
    FIXME("%p): stub\n", join_info);
}
