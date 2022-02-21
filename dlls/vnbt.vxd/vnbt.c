/*
 * VNBT VxD implementation
 *
 * Copyright 2003 Juan Lang
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
#include "winsock2.h"
#include "iphlpapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);

typedef struct _nbtInfo
{
    DWORD ip;
    DWORD winsPrimary;
    DWORD winsSecondary;
    DWORD dnsPrimary;
    DWORD dnsSecondary;
    DWORD unk0;
} nbtInfo;

#define MAX_NBT_ENTRIES 7

typedef struct _nbtTable
{
    DWORD   numEntries;
    nbtInfo table[MAX_NBT_ENTRIES];
    UCHAR   pad[6];
    WORD    nodeType;
    WORD    scopeLen;
    char    scope[254];
} nbtTable;


/***********************************************************************
 *           DeviceIoControl   (VNB.VXD.@)
 */
BOOL WINAPI VNBT_DeviceIoControl(DWORD dwIoControlCode,
                                 LPVOID lpvInBuffer, DWORD cbInBuffer,
                                 LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                                 LPDWORD lpcbBytesReturned,
                                 LPOVERLAPPED lpOverlapped)
{
    DWORD error;

    switch (dwIoControlCode)
    {
    case 116:
        if (lpcbBytesReturned)
            *lpcbBytesReturned = sizeof(nbtTable);
        if (!lpvOutBuffer || cbOutBuffer < sizeof(nbtTable))
            error = ERROR_BUFFER_OVERFLOW;
        else
        {
            nbtTable *info = (nbtTable *)lpvOutBuffer;
            DWORD size = 0;

            memset(info, 0, sizeof(nbtTable));

            error = GetNetworkParams(NULL, &size);
            if (ERROR_BUFFER_OVERFLOW == error)
            {
                PFIXED_INFO fixedInfo = HeapAlloc( GetProcessHeap(), 0, size);

                error = GetNetworkParams(fixedInfo, &size);
                if (NO_ERROR == error)
                {
                    info->nodeType = (WORD)fixedInfo->NodeType;
                    info->scopeLen = min(strlen(fixedInfo->ScopeId) + 1,
                                         sizeof(info->scope) - 2);
                    memcpy(info->scope + 1, fixedInfo->ScopeId,
                           info->scopeLen);
                    info->scope[info->scopeLen + 1] = '\0';
                    {
                        /* convert into L2-encoded version */
                        char *ptr, *lenPtr;

                        for (ptr = info->scope + 1; *ptr &&
                                 ptr - info->scope < sizeof(info->scope); )
                        {
                            for (lenPtr = ptr - 1, *lenPtr = 0;
                                 *ptr && *ptr != '.' &&
                                     ptr - info->scope < sizeof(info->scope);
                                 ptr++)
                                *lenPtr += 1;
                            ptr++;
                        }
                    }
                    /* could set DNS servers here too, but since
                     * ipconfig.exe and winipcfg.exe read these from the
                     * registry, there's no point */
                }
                HeapFree(GetProcessHeap(), 0, fixedInfo);
            }
            size = 0;
            error = GetAdaptersInfo(NULL, &size);
            if (ERROR_BUFFER_OVERFLOW == error)
            {
                PIP_ADAPTER_INFO adapterInfo = HeapAlloc(GetProcessHeap(), 0, size);

                error = GetAdaptersInfo(adapterInfo, &size);
                if (NO_ERROR == error)
                {
                    PIP_ADAPTER_INFO ptr;

                    for (ptr = adapterInfo; ptr && info->numEntries <
                             MAX_NBT_ENTRIES; ptr = ptr->Next)
                    {
                        unsigned long addr;

                        addr = inet_addr(ptr->IpAddressList.IpAddress.String);
                        if (addr != 0 && addr != INADDR_NONE)
                            info->table[info->numEntries].ip = ntohl(addr);
                        addr = inet_addr(ptr->PrimaryWinsServer.IpAddress.String);
                        if (addr != 0 && addr != INADDR_NONE)
                            info->table[info->numEntries].winsPrimary = ntohl(addr);
                        addr = inet_addr(ptr->SecondaryWinsServer.IpAddress.String);
                        if (addr != 0 && addr != INADDR_NONE)
                            info->table[info->numEntries].winsSecondary = ntohl(addr);
                        info->numEntries++;
                    }
                }
                HeapFree(GetProcessHeap(), 0, adapterInfo);
            }
        }
        break;

    case 119:
        /* nbtstat.exe uses this, but the return seems to be a bunch of
         * pointers, so it's not so easy to reverse engineer.  Fall through
         * to unimplemented...
         */
    default:
        FIXME( "Unimplemented control %ld for VxD device VNB\n",
               dwIoControlCode );
        error = ERROR_NOT_SUPPORTED;
        break;
    }
    if (error)
        SetLastError(error);
    return error == NO_ERROR;
}
