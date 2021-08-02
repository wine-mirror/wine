/*
 * iphlpapi dll test
 *
 * Copyright (C) 2003 Juan Lang
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

/*
 * Some observations that an automated test can't produce:
 * An adapter index is a key for an adapter.  That is, if an index is returned
 * from one API, that same index may be used successfully in another API, as
 * long as the adapter remains present.
 * If the adapter is removed and reinserted, however, the index may change (and
 * indeed it does change on Win2K).
 *
 * The Name field of the IP_ADAPTER_INDEX_MAP entries returned by
 * GetInterfaceInfo is declared as a wide string, but the bytes are actually
 * an ASCII string on some versions of the IP helper API under Win9x.  This was
 * apparently an MS bug, it's corrected in later versions.
 *
 * The DomainName field of FIXED_INFO isn't NULL-terminated on Win98.
 */

#include <stdarg.h>
#include "winsock2.h"
#include "windef.h"
#include "winbase.h"
#include "ws2tcpip.h"
#include "windns.h"
#include "iphlpapi.h"
#include "icmpapi.h"
#include "iprtrmib.h"
#include "netioapi.h"
#include "wine/test.h"
#include <stdio.h>
#include <stdlib.h>

#define ICMP_MINLEN 8 /* copied from dlls/iphlpapi/ip_icmp.h file */

static HMODULE hLibrary = NULL;

static DWORD (WINAPI *pAllocateAndGetTcpExTableFromStack)(void**,BOOL,HANDLE,DWORD,DWORD);
static DWORD (WINAPI *pGetTcp6Table)(PMIB_TCP6TABLE,PDWORD,BOOL);
static DWORD (WINAPI *pGetUdp6Table)(PMIB_UDP6TABLE,PDWORD,BOOL);
static DWORD (WINAPI *pGetUnicastIpAddressEntry)(MIB_UNICASTIPADDRESS_ROW*);
static DWORD (WINAPI *pGetUnicastIpAddressTable)(ADDRESS_FAMILY,MIB_UNICASTIPADDRESS_TABLE**);
static DWORD (WINAPI *pGetExtendedTcpTable)(PVOID,PDWORD,BOOL,ULONG,TCP_TABLE_CLASS,ULONG);
static DWORD (WINAPI *pGetExtendedUdpTable)(PVOID,PDWORD,BOOL,ULONG,UDP_TABLE_CLASS,ULONG);
static DWORD (WINAPI *pCreateSortedAddressPairs)(const PSOCKADDR_IN6,ULONG,const PSOCKADDR_IN6,ULONG,ULONG,
                                                 PSOCKADDR_IN6_PAIR*,ULONG*);
static DWORD (WINAPI *pConvertLengthToIpv4Mask)(ULONG,ULONG*);
static DWORD (WINAPI *pParseNetworkString)(const WCHAR*,DWORD,NET_ADDRESS_INFO*,USHORT*,BYTE*);
static DWORD (WINAPI *pNotifyUnicastIpAddressChange)(ADDRESS_FAMILY, PUNICAST_IPADDRESS_CHANGE_CALLBACK,
                                                PVOID, BOOLEAN, HANDLE *);
static DWORD (WINAPI *pCancelMibChangeNotify2)(HANDLE);

DWORD WINAPI ConvertGuidToStringA( const GUID *, char *, DWORD );
DWORD WINAPI ConvertGuidToStringW( const GUID *, WCHAR *, DWORD );
DWORD WINAPI ConvertStringToGuidW( const WCHAR *, GUID * );

static void loadIPHlpApi(void)
{
  hLibrary = LoadLibraryA("iphlpapi.dll");
  if (hLibrary) {
    pAllocateAndGetTcpExTableFromStack = (void *)GetProcAddress(hLibrary, "AllocateAndGetTcpExTableFromStack");
    pGetTcp6Table = (void *)GetProcAddress(hLibrary, "GetTcp6Table");
    pGetUdp6Table = (void *)GetProcAddress(hLibrary, "GetUdp6Table");
    pGetUnicastIpAddressEntry = (void *)GetProcAddress(hLibrary, "GetUnicastIpAddressEntry");
    pGetUnicastIpAddressTable = (void *)GetProcAddress(hLibrary, "GetUnicastIpAddressTable");
    pGetExtendedTcpTable = (void *)GetProcAddress(hLibrary, "GetExtendedTcpTable");
    pGetExtendedUdpTable = (void *)GetProcAddress(hLibrary, "GetExtendedUdpTable");
    pCreateSortedAddressPairs = (void *)GetProcAddress(hLibrary, "CreateSortedAddressPairs");
    pConvertLengthToIpv4Mask = (void *)GetProcAddress(hLibrary, "ConvertLengthToIpv4Mask");
    pParseNetworkString = (void *)GetProcAddress(hLibrary, "ParseNetworkString");
    pNotifyUnicastIpAddressChange = (void *)GetProcAddress(hLibrary, "NotifyUnicastIpAddressChange");
    pCancelMibChangeNotify2 = (void *)GetProcAddress(hLibrary, "CancelMibChangeNotify2");
  }
}

static void freeIPHlpApi(void)
{
    FreeLibrary(hLibrary);
}

/* replacement for inet_ntoa */
static const char *ntoa( DWORD ip )
{
    static char buffers[4][16];
    static int i = -1;

    ip = htonl(ip);
    i = (i + 1) % ARRAY_SIZE(buffers);
    sprintf( buffers[i], "%u.%u.%u.%u", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff );
    return buffers[i];
}

static const char *ntoa6( IN6_ADDR *ip )
{
    static char buffers[4][40];
    static int i = -1;
    unsigned short *p = ip->u.Word;

    i = (i + 1) % ARRAY_SIZE(buffers);
    sprintf( buffers[i], "%x:%x:%x:%x:%x:%x:%x:%x",
             htons(p[0]), htons(p[1]), htons(p[2]), htons(p[3]), htons(p[4]), htons(p[5]), htons(p[6]), htons(p[7]) );
    return buffers[i];
}

/*
still-to-be-tested 98-only functions:
GetUniDirectionalAdapterInfo
*/
static void testWin98OnlyFunctions(void)
{
}

static void testGetNumberOfInterfaces(void)
{
    DWORD apiReturn, numInterfaces;

    /* Crashes on Vista */
    if (0) {
        apiReturn = GetNumberOfInterfaces(NULL);
        if (apiReturn == ERROR_NOT_SUPPORTED)
            return;
        ok(apiReturn == ERROR_INVALID_PARAMETER,
           "GetNumberOfInterfaces(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
           apiReturn);
    }

    apiReturn = GetNumberOfInterfaces(&numInterfaces);
    ok(apiReturn == NO_ERROR,
       "GetNumberOfInterfaces returned %d, expected 0\n", apiReturn);
}

static void testGetIfEntry(DWORD index)
{
    DWORD apiReturn;
    MIB_IFROW row;

    memset(&row, 0, sizeof(row));
    apiReturn = GetIfEntry(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIfEntry is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfEntry(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    row.dwIndex = -1; /* hope that's always bogus! */
    apiReturn = GetIfEntry(&row);
    ok(apiReturn == ERROR_INVALID_DATA ||
     apiReturn == ERROR_FILE_NOT_FOUND /* Vista */,
     "GetIfEntry(bogus row) returned %d, expected ERROR_INVALID_DATA or ERROR_FILE_NOT_FOUND\n",
     apiReturn);
    row.dwIndex = index;
    apiReturn = GetIfEntry(&row);
    ok(apiReturn == NO_ERROR, 
     "GetIfEntry returned %d, expected NO_ERROR\n", apiReturn);
}

static void testGetIpAddrTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetIpAddrTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIpAddrTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpAddrTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIpAddrTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetIpAddrTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IPADDRTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

        apiReturn = GetIpAddrTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetIpAddrTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
           apiReturn);
        if (apiReturn == NO_ERROR && buf->dwNumEntries)
        {
            int i;
            testGetIfEntry(buf->table[0].dwIndex);
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                ok (buf->table[i].wType != 0, "Test[%d]: expected wType > 0\n", i);
                ok (buf->table[i].dwBCastAddr == 1, "Test[%d]: got %08x\n", i, buf->table[i].dwBCastAddr);
                ok (buf->table[i].dwReasmSize == 0xffff, "Test[%d]: got %08x\n", i, buf->table[i].dwReasmSize);
                trace("Entry[%d]: addr %s, dwIndex %u, wType 0x%x\n", i,
                      ntoa(buf->table[i].dwAddr), buf->table[i].dwIndex, buf->table[i].wType);
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testGetIfTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetIfTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIfTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIfTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIfTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetIfTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IFTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

        apiReturn = GetIfTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetIfTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n\n",
           apiReturn);

        if (apiReturn == NO_ERROR)
        {
            char descr[MAX_INTERFACE_NAME_LEN];
            WCHAR name[MAX_INTERFACE_NAME_LEN];
            DWORD i, index;

            if (winetest_debug > 1) trace( "interface table: %u entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                MIB_IFROW *row = &buf->table[i];
                MIB_IF_ROW2 row2;
                GUID *guid;

                if (winetest_debug > 1)
                {
                    trace( "%u: '%s' type %u mtu %u speed %u\n",
                           row->dwIndex, debugstr_w(row->wszName), row->dwType, row->dwMtu, row->dwSpeed );
                    trace( "        in: bytes %u upkts %u nupkts %u disc %u err %u unk %u\n",
                           row->dwInOctets, row->dwInUcastPkts, row->dwInNUcastPkts,
                           row->dwInDiscards, row->dwInErrors, row->dwInUnknownProtos );
                    trace( "        out: bytes %u upkts %u nupkts %u disc %u err %u\n",
                           row->dwOutOctets, row->dwOutUcastPkts, row->dwOutNUcastPkts,
                           row->dwOutDiscards, row->dwOutErrors );
                }
                apiReturn = GetAdapterIndex( row->wszName, &index );
                ok( !apiReturn, "got %d\n", apiReturn );
                ok( index == row->dwIndex ||
                    broken( index != row->dwIndex && index ), /* Win8 can have identical guids for two different ifaces */
                    "got %d vs %d\n", index, row->dwIndex );
                memset( &row2, 0, sizeof(row2) );
                row2.InterfaceIndex = row->dwIndex;
                GetIfEntry2( &row2 );
                WideCharToMultiByte( CP_ACP, 0, row2.Description, -1, descr, sizeof(descr), NULL, NULL );
                ok( !strcmp( (char *)row->bDescr, descr ), "got %s vs %s\n", row->bDescr, descr );
                guid = &row2.InterfaceGuid;
                swprintf( name, ARRAY_SIZE(name), L"\\DEVICE\\TCPIP_{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                          guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1],
                          guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5],
                          guid->Data4[6], guid->Data4[7]);
                ok( !wcscmp( row->wszName, name ), "got %s vs %s\n", debugstr_w( row->wszName ), debugstr_w( name ) );
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testGetIpForwardTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetIpForwardTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIpForwardTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpForwardTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIpForwardTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetIpForwardTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IPFORWARDTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

        apiReturn = GetIpForwardTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetIpForwardTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR)
        {
            DWORD i;

            trace( "IP forward table: %u entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                if (!U1(buf->table[i]).dwForwardDest) /* Default route */
                {
                    todo_wine
                    ok (U1(buf->table[i]).dwForwardProto == MIB_IPPROTO_NETMGMT,
                            "Unexpected dwForwardProto %d\n", U1(buf->table[i]).dwForwardProto);
                    ok (U1(buf->table[i]).dwForwardType == MIB_IPROUTE_TYPE_INDIRECT,
                        "Unexpected dwForwardType %d\n",  U1(buf->table[i]).dwForwardType);
                }
                else
                {
                    /* In general we should get MIB_IPPROTO_LOCAL but does not work
                     * for Vista, 2008 and 7. */
                    ok (U1(buf->table[i]).dwForwardProto == MIB_IPPROTO_LOCAL ||
                        broken(U1(buf->table[i]).dwForwardProto == MIB_IPPROTO_NETMGMT),
                        "Unexpected dwForwardProto %d\n", U1(buf->table[i]).dwForwardProto);
                    /* The forward type varies depending on the address and gateway
                     * value so it is not worth testing in this case. */
                }

                trace( "%u: dest %s mask %s gw %s if %u type %u proto %u\n", i,
                       ntoa( buf->table[i].dwForwardDest ), ntoa( buf->table[i].dwForwardMask ),
                       ntoa( buf->table[i].dwForwardNextHop ), buf->table[i].dwForwardIfIndex,
                       U1(buf->table[i]).dwForwardType, U1(buf->table[i]).dwForwardProto );
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testGetIpNetTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetIpNetTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIpNetTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpNetTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIpNetTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetIpNetTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_NO_DATA or ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_NO_DATA)
        ; /* empty ARP table's okay */
    else if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IPNETTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

        apiReturn = GetIpNetTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR ||
           apiReturn == ERROR_NO_DATA, /* empty ARP table's okay */
           "GetIpNetTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i, j;

            trace( "IP net table: %u entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                trace( "%u: idx %u type %u addr %s phys",
                       i, buf->table[i].dwIndex, U(buf->table[i]).dwType, ntoa( buf->table[i].dwAddr ));
                for (j = 0; j < buf->table[i].dwPhysAddrLen; j++)
                    printf( " %02x", buf->table[i].bPhysAddr[j] );
                printf( "\n" );
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testGetIcmpStatistics(void)
{
    DWORD apiReturn;
    MIB_ICMP stats;

    /* Crashes on Vista */
    if (0) {
        apiReturn = GetIcmpStatistics(NULL);
        if (apiReturn == ERROR_NOT_SUPPORTED)
            return;
        ok(apiReturn == ERROR_INVALID_PARAMETER,
           "GetIcmpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
           apiReturn);
    }

    apiReturn = GetIcmpStatistics(&stats);
    if (apiReturn == ERROR_NOT_SUPPORTED)
    {
        skip("GetIcmpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == NO_ERROR,
       "GetIcmpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "ICMP stats:          %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:          %8u %8u\n", stats.stats.icmpInStats.dwMsgs, stats.stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:        %8u %8u\n", stats.stats.icmpInStats.dwErrors, stats.stats.icmpOutStats.dwErrors );
        trace( "    dwDestUnreachs:  %8u %8u\n", stats.stats.icmpInStats.dwDestUnreachs, stats.stats.icmpOutStats.dwDestUnreachs );
        trace( "    dwTimeExcds:     %8u %8u\n", stats.stats.icmpInStats.dwTimeExcds, stats.stats.icmpOutStats.dwTimeExcds );
        trace( "    dwParmProbs:     %8u %8u\n", stats.stats.icmpInStats.dwParmProbs, stats.stats.icmpOutStats.dwParmProbs );
        trace( "    dwSrcQuenchs:    %8u %8u\n", stats.stats.icmpInStats.dwSrcQuenchs, stats.stats.icmpOutStats.dwSrcQuenchs );
        trace( "    dwRedirects:     %8u %8u\n", stats.stats.icmpInStats.dwRedirects, stats.stats.icmpOutStats.dwRedirects );
        trace( "    dwEchos:         %8u %8u\n", stats.stats.icmpInStats.dwEchos, stats.stats.icmpOutStats.dwEchos );
        trace( "    dwEchoReps:      %8u %8u\n", stats.stats.icmpInStats.dwEchoReps, stats.stats.icmpOutStats.dwEchoReps );
        trace( "    dwTimestamps:    %8u %8u\n", stats.stats.icmpInStats.dwTimestamps, stats.stats.icmpOutStats.dwTimestamps );
        trace( "    dwTimestampReps: %8u %8u\n", stats.stats.icmpInStats.dwTimestampReps, stats.stats.icmpOutStats.dwTimestampReps );
        trace( "    dwAddrMasks:     %8u %8u\n", stats.stats.icmpInStats.dwAddrMasks, stats.stats.icmpOutStats.dwAddrMasks );
        trace( "    dwAddrMaskReps:  %8u %8u\n", stats.stats.icmpInStats.dwAddrMaskReps, stats.stats.icmpOutStats.dwAddrMaskReps );
    }
}

static void testGetIpStatistics(void)
{
    DWORD apiReturn;
    MIB_IPSTATS stats;

    apiReturn = GetIpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
       "GetIpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP stats:\n" );
        trace( "    dwForwarding:      %u\n", U(stats).dwForwarding );
        trace( "    dwDefaultTTL:      %u\n", stats.dwDefaultTTL );
        trace( "    dwInReceives:      %u\n", stats.dwInReceives );
        trace( "    dwInHdrErrors:     %u\n", stats.dwInHdrErrors );
        trace( "    dwInAddrErrors:    %u\n", stats.dwInAddrErrors );
        trace( "    dwForwDatagrams:   %u\n", stats.dwForwDatagrams );
        trace( "    dwInUnknownProtos: %u\n", stats.dwInUnknownProtos );
        trace( "    dwInDiscards:      %u\n", stats.dwInDiscards );
        trace( "    dwInDelivers:      %u\n", stats.dwInDelivers );
        trace( "    dwOutRequests:     %u\n", stats.dwOutRequests );
        trace( "    dwRoutingDiscards: %u\n", stats.dwRoutingDiscards );
        trace( "    dwOutDiscards:     %u\n", stats.dwOutDiscards );
        trace( "    dwOutNoRoutes:     %u\n", stats.dwOutNoRoutes );
        trace( "    dwReasmTimeout:    %u\n", stats.dwReasmTimeout );
        trace( "    dwReasmReqds:      %u\n", stats.dwReasmReqds );
        trace( "    dwReasmOks:        %u\n", stats.dwReasmOks );
        trace( "    dwReasmFails:      %u\n", stats.dwReasmFails );
        trace( "    dwFragOks:         %u\n", stats.dwFragOks );
        trace( "    dwFragFails:       %u\n", stats.dwFragFails );
        trace( "    dwFragCreates:     %u\n", stats.dwFragCreates );
        trace( "    dwNumIf:           %u\n", stats.dwNumIf );
        trace( "    dwNumAddr:         %u\n", stats.dwNumAddr );
        trace( "    dwNumRoutes:       %u\n", stats.dwNumRoutes );
    }
}

static void testGetTcpStatistics(void)
{
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    apiReturn = GetTcpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetTcpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetTcpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetTcpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
       "GetTcpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP stats:\n" );
        trace( "    dwRtoAlgorithm: %u\n", U(stats).dwRtoAlgorithm );
        trace( "    dwRtoMin:       %u\n", stats.dwRtoMin );
        trace( "    dwRtoMax:       %u\n", stats.dwRtoMax );
        trace( "    dwMaxConn:      %u\n", stats.dwMaxConn );
        trace( "    dwActiveOpens:  %u\n", stats.dwActiveOpens );
        trace( "    dwPassiveOpens: %u\n", stats.dwPassiveOpens );
        trace( "    dwAttemptFails: %u\n", stats.dwAttemptFails );
        trace( "    dwEstabResets:  %u\n", stats.dwEstabResets );
        trace( "    dwCurrEstab:    %u\n", stats.dwCurrEstab );
        trace( "    dwInSegs:       %u\n", stats.dwInSegs );
        trace( "    dwOutSegs:      %u\n", stats.dwOutSegs );
        trace( "    dwRetransSegs:  %u\n", stats.dwRetransSegs );
        trace( "    dwInErrs:       %u\n", stats.dwInErrs );
        trace( "    dwOutRsts:      %u\n", stats.dwOutRsts );
        trace( "    dwNumConns:     %u\n", stats.dwNumConns );
    }
}

static void testGetUdpStatistics(void)
{
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    apiReturn = GetUdpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetUdpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetUdpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetUdpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
       "GetUdpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP stats:\n" );
        trace( "    dwInDatagrams:  %u\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %u\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %u\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %u\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %u\n", stats.dwNumAddrs );
    }
}

static void testGetIcmpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_ICMP_EX stats;

    /* Crashes on Vista */
    if (1) {
        apiReturn = GetIcmpStatisticsEx(NULL, AF_INET);
        ok(apiReturn == ERROR_INVALID_PARAMETER,
         "GetIcmpStatisticsEx(NULL, AF_INET) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);
    }

    apiReturn = GetIcmpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIcmpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetIcmpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetIcmpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        INT i;
        trace( "ICMP IPv4 Ex stats:           %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:              %8u %8u\n", stats.icmpInStats.dwMsgs, stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:            %8u %8u\n", stats.icmpInStats.dwErrors, stats.icmpOutStats.dwErrors );
        for (i = 0; i < 256; i++)
            trace( "    rgdwTypeCount[%3i]: %8u %8u\n", i, stats.icmpInStats.rgdwTypeCount[i], stats.icmpOutStats.rgdwTypeCount[i] );
    }

    apiReturn = GetIcmpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetIcmpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        INT i;
        trace( "ICMP IPv6 Ex stats:           %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:              %8u %8u\n", stats.icmpInStats.dwMsgs, stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:            %8u %8u\n", stats.icmpInStats.dwErrors, stats.icmpOutStats.dwErrors );
        for (i = 0; i < 256; i++)
            trace( "    rgdwTypeCount[%3i]: %8u %8u\n", i, stats.icmpInStats.rgdwTypeCount[i], stats.icmpOutStats.rgdwTypeCount[i] );
    }
}

static void testGetIpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_IPSTATS stats;

    apiReturn = GetIpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatisticsEx(NULL, AF_INET) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetIpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetIpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetIpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP IPv4 Ex stats:\n" );
        trace( "    dwForwarding:      %u\n", U(stats).dwForwarding );
        trace( "    dwDefaultTTL:      %u\n", stats.dwDefaultTTL );
        trace( "    dwInReceives:      %u\n", stats.dwInReceives );
        trace( "    dwInHdrErrors:     %u\n", stats.dwInHdrErrors );
        trace( "    dwInAddrErrors:    %u\n", stats.dwInAddrErrors );
        trace( "    dwForwDatagrams:   %u\n", stats.dwForwDatagrams );
        trace( "    dwInUnknownProtos: %u\n", stats.dwInUnknownProtos );
        trace( "    dwInDiscards:      %u\n", stats.dwInDiscards );
        trace( "    dwInDelivers:      %u\n", stats.dwInDelivers );
        trace( "    dwOutRequests:     %u\n", stats.dwOutRequests );
        trace( "    dwRoutingDiscards: %u\n", stats.dwRoutingDiscards );
        trace( "    dwOutDiscards:     %u\n", stats.dwOutDiscards );
        trace( "    dwOutNoRoutes:     %u\n", stats.dwOutNoRoutes );
        trace( "    dwReasmTimeout:    %u\n", stats.dwReasmTimeout );
        trace( "    dwReasmReqds:      %u\n", stats.dwReasmReqds );
        trace( "    dwReasmOks:        %u\n", stats.dwReasmOks );
        trace( "    dwReasmFails:      %u\n", stats.dwReasmFails );
        trace( "    dwFragOks:         %u\n", stats.dwFragOks );
        trace( "    dwFragFails:       %u\n", stats.dwFragFails );
        trace( "    dwFragCreates:     %u\n", stats.dwFragCreates );
        trace( "    dwNumIf:           %u\n", stats.dwNumIf );
        trace( "    dwNumAddr:         %u\n", stats.dwNumAddr );
        trace( "    dwNumRoutes:       %u\n", stats.dwNumRoutes );
    }

    apiReturn = GetIpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetIpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP IPv6 Ex stats:\n" );
        trace( "    dwForwarding:      %u\n", U(stats).dwForwarding );
        trace( "    dwDefaultTTL:      %u\n", stats.dwDefaultTTL );
        trace( "    dwInReceives:      %u\n", stats.dwInReceives );
        trace( "    dwInHdrErrors:     %u\n", stats.dwInHdrErrors );
        trace( "    dwInAddrErrors:    %u\n", stats.dwInAddrErrors );
        trace( "    dwForwDatagrams:   %u\n", stats.dwForwDatagrams );
        trace( "    dwInUnknownProtos: %u\n", stats.dwInUnknownProtos );
        trace( "    dwInDiscards:      %u\n", stats.dwInDiscards );
        trace( "    dwInDelivers:      %u\n", stats.dwInDelivers );
        trace( "    dwOutRequests:     %u\n", stats.dwOutRequests );
        trace( "    dwRoutingDiscards: %u\n", stats.dwRoutingDiscards );
        trace( "    dwOutDiscards:     %u\n", stats.dwOutDiscards );
        trace( "    dwOutNoRoutes:     %u\n", stats.dwOutNoRoutes );
        trace( "    dwReasmTimeout:    %u\n", stats.dwReasmTimeout );
        trace( "    dwReasmReqds:      %u\n", stats.dwReasmReqds );
        trace( "    dwReasmOks:        %u\n", stats.dwReasmOks );
        trace( "    dwReasmFails:      %u\n", stats.dwReasmFails );
        trace( "    dwFragOks:         %u\n", stats.dwFragOks );
        trace( "    dwFragFails:       %u\n", stats.dwFragFails );
        trace( "    dwFragCreates:     %u\n", stats.dwFragCreates );
        trace( "    dwNumIf:           %u\n", stats.dwNumIf );
        trace( "    dwNumAddr:         %u\n", stats.dwNumAddr );
        trace( "    dwNumRoutes:       %u\n", stats.dwNumRoutes );
    }
}

static void testGetTcpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    apiReturn = GetTcpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetTcpStatisticsEx(NULL, AF_INET); returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetTcpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER || apiReturn == ERROR_NOT_SUPPORTED,
       "GetTcpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetTcpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetTcpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP IPv4 Ex stats:\n" );
        trace( "    dwRtoAlgorithm: %u\n", U(stats).dwRtoAlgorithm );
        trace( "    dwRtoMin:       %u\n", stats.dwRtoMin );
        trace( "    dwRtoMax:       %u\n", stats.dwRtoMax );
        trace( "    dwMaxConn:      %u\n", stats.dwMaxConn );
        trace( "    dwActiveOpens:  %u\n", stats.dwActiveOpens );
        trace( "    dwPassiveOpens: %u\n", stats.dwPassiveOpens );
        trace( "    dwAttemptFails: %u\n", stats.dwAttemptFails );
        trace( "    dwEstabResets:  %u\n", stats.dwEstabResets );
        trace( "    dwCurrEstab:    %u\n", stats.dwCurrEstab );
        trace( "    dwInSegs:       %u\n", stats.dwInSegs );
        trace( "    dwOutSegs:      %u\n", stats.dwOutSegs );
        trace( "    dwRetransSegs:  %u\n", stats.dwRetransSegs );
        trace( "    dwInErrs:       %u\n", stats.dwInErrs );
        trace( "    dwOutRsts:      %u\n", stats.dwOutRsts );
        trace( "    dwNumConns:     %u\n", stats.dwNumConns );
    }

    apiReturn = GetTcpStatisticsEx(&stats, AF_INET6);
    todo_wine ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
                 "GetTcpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP IPv6 Ex stats:\n" );
        trace( "    dwRtoAlgorithm: %u\n", U(stats).dwRtoAlgorithm );
        trace( "    dwRtoMin:       %u\n", stats.dwRtoMin );
        trace( "    dwRtoMax:       %u\n", stats.dwRtoMax );
        trace( "    dwMaxConn:      %u\n", stats.dwMaxConn );
        trace( "    dwActiveOpens:  %u\n", stats.dwActiveOpens );
        trace( "    dwPassiveOpens: %u\n", stats.dwPassiveOpens );
        trace( "    dwAttemptFails: %u\n", stats.dwAttemptFails );
        trace( "    dwEstabResets:  %u\n", stats.dwEstabResets );
        trace( "    dwCurrEstab:    %u\n", stats.dwCurrEstab );
        trace( "    dwInSegs:       %u\n", stats.dwInSegs );
        trace( "    dwOutSegs:      %u\n", stats.dwOutSegs );
        trace( "    dwRetransSegs:  %u\n", stats.dwRetransSegs );
        trace( "    dwInErrs:       %u\n", stats.dwInErrs );
        trace( "    dwOutRsts:      %u\n", stats.dwOutRsts );
        trace( "    dwNumConns:     %u\n", stats.dwNumConns );
    }
}

static void testGetUdpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    apiReturn = GetUdpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetUdpStatisticsEx(NULL, AF_INET); returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetUdpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER || apiReturn == ERROR_NOT_SUPPORTED,
       "GetUdpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetUdpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetUdpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP IPv4 Ex stats:\n" );
        trace( "    dwInDatagrams:  %u\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %u\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %u\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %u\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %u\n", stats.dwNumAddrs );
    }

    apiReturn = GetUdpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetUdpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP IPv6 Ex stats:\n" );
        trace( "    dwInDatagrams:  %u\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %u\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %u\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %u\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %u\n", stats.dwNumAddrs );
    }
}

static void testGetTcpTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetTcpTable(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetTcpTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetTcpTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_TCPTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

        apiReturn = GetTcpTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetTcpTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i;
            trace( "TCP table: %u entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                trace( "%u: local %s:%u remote %s:%u state %u\n", i,
                       ntoa(buf->table[i].dwLocalAddr), ntohs(buf->table[i].dwLocalPort),
                       ntoa(buf->table[i].dwRemoteAddr), ntohs(buf->table[i].dwRemotePort),
                       U(buf->table[i]).dwState );
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testGetUdpTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetUdpTable(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetUdpTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetUdpTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_UDPTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

        apiReturn = GetUdpTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetUdpTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i;
            trace( "UDP table: %u entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
                trace( "%u: %s:%u\n",
                       i, ntoa( buf->table[i].dwLocalAddr ), ntohs(buf->table[i].dwLocalPort) );
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testSetTcpEntry(void)
{
    DWORD ret;
    MIB_TCPROW row;

    memset(&row, 0, sizeof(row));
    if(0) /* This test crashes in OS >= VISTA */
    {
        ret = SetTcpEntry(NULL);
        ok( ret == ERROR_INVALID_PARAMETER, "got %u, expected %u\n", ret, ERROR_INVALID_PARAMETER);
    }

    ret = SetTcpEntry(&row);
    if (ret == ERROR_NETWORK_ACCESS_DENIED)
    {
        win_skip("SetTcpEntry failed with access error. Skipping test.\n");
        return;
    }
    todo_wine ok( ret == ERROR_INVALID_PARAMETER, "got %u, expected %u\n", ret, ERROR_INVALID_PARAMETER);

    U(row).dwState = MIB_TCP_STATE_DELETE_TCB;
    ret = SetTcpEntry(&row);
    todo_wine ok( ret == ERROR_MR_MID_NOT_FOUND || broken(ret == ERROR_INVALID_PARAMETER),
       "got %u, expected %u\n", ret, ERROR_MR_MID_NOT_FOUND);
}

static void testIcmpSendEcho(void)
{
    HANDLE icmp;
    char senddata[32], replydata[sizeof(senddata) + sizeof(ICMP_ECHO_REPLY)];
    DWORD ret, error, replysz = sizeof(replydata);
    IPAddr address;
    ICMP_ECHO_REPLY *reply;
    INT i;

    memset(senddata, 0, sizeof(senddata));

    address = htonl(INADDR_LOOPBACK);
    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(INVALID_HANDLE_VALUE, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INVALID_HANDLE) /* <= 2003 */,
        "expected 87, got %d\n", error);

    icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        if (error == ERROR_ACCESS_DENIED)
        {
            skip ("ICMP is not available.\n");
            return;
        }
    }
    ok (icmp != INVALID_HANDLE_VALUE, "IcmpCreateFile failed unexpectedly with error %d\n", GetLastError());

    address = 0;
    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_NETNAME
        || broken(error == IP_BAD_DESTINATION) /* <= 2003 */,
        "expected 1214, got %d\n", error);

    address = htonl(INADDR_LOOPBACK);
    if (0) /* crashes in XP */
    {
        ret = IcmpSendEcho(icmp, address, NULL, sizeof(senddata), NULL, replydata, replysz, 1000);
        ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    }

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, NULL, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, NULL, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER, "expected 87, got %d\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, 0, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %d\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, NULL, 0, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %d\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata) - 1;
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %d\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY);
    ret = IcmpSendEcho(icmp, address, senddata, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY) + ICMP_MINLEN;
    ret = IcmpSendEcho(icmp, address, senddata, ICMP_MINLEN, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY) + ICMP_MINLEN;
    ret = IcmpSendEcho(icmp, address, senddata, ICMP_MINLEN + 1, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %d\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, ICMP_MINLEN, NULL, replydata, replysz - 1, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %d\n", error);

    /* in windows >= vista the timeout can't be invalid */
    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 0);
    error = GetLastError();
    if (!ret) ok(error == ERROR_INVALID_PARAMETER, "expected 87, got %d\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, -1);
    error = GetLastError();
    if (!ret) ok(error == ERROR_INVALID_PARAMETER, "expected 87, got %d\n", error);

    /* real ping test */
    SetLastError(0xdeadbeef);
    address = htonl(INADDR_LOOPBACK);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    if (!ret)
    {
        skip ("Failed to ping with error %d, is lo interface down?.\n", error);
    }
    else if (winetest_debug > 1)
    {
        PICMP_ECHO_REPLY pong = (PICMP_ECHO_REPLY) replydata;
        trace ("send addr  : %s\n", ntoa(address));
        trace ("reply addr : %s\n", ntoa(pong->Address));
        trace ("reply size : %u\n", replysz);
        trace ("roundtrip  : %u ms\n", pong->RoundTripTime);
        trace ("status     : %u\n", pong->Status);
        trace ("recv size  : %u\n", pong->DataSize);
        trace ("ttl        : %u\n", pong->Options.Ttl);
        trace ("flags      : 0x%x\n", pong->Options.Flags);
    }

    /* check reply data */
    SetLastError(0xdeadbeef);
    address = htonl(INADDR_LOOPBACK);
    for (i = 0; i < ARRAY_SIZE(senddata); i++) senddata[i] = i & 0xff;
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    reply = (ICMP_ECHO_REPLY *)replydata;
    ok(ret, "IcmpSendEcho failed unexpectedly\n");
    ok(error == NO_ERROR, "Expect last error:0x%08x, got:0x%08x\n", NO_ERROR, error);
    ok(INADDR_LOOPBACK == ntohl(reply->Address), "Address mismatch, expect:%s, got: %s\n", ntoa(INADDR_LOOPBACK),
       ntoa(reply->Address));
    ok(reply->Status == IP_SUCCESS, "Expect status:0x%08x, got:0x%08x\n", IP_SUCCESS, reply->Status);
    ok(reply->DataSize == sizeof(senddata), "Got size:%d\n", reply->DataSize);
    ok(!memcmp(senddata, reply->Data, min(sizeof(senddata), reply->DataSize)), "Data mismatch\n");

    IcmpCloseHandle(icmp);
}

/*
still-to-be-tested NT4-onward functions:
CreateIpForwardEntry
DeleteIpForwardEntry
CreateIpNetEntry
DeleteIpNetEntry
GetFriendlyIfIndex
GetRTTAndHopCount
SetIfEntry
SetIpForwardEntry
SetIpNetEntry
SetIpStatistics
SetIpTTL
*/
static void testWinNT4Functions(void)
{
  testGetNumberOfInterfaces();
  testGetIpAddrTable();
  testGetIfTable();
  testGetIpForwardTable();
  testGetIpNetTable();
  testGetIcmpStatistics();
  testGetIpStatistics();
  testGetTcpStatistics();
  testGetUdpStatistics();
  testGetIcmpStatisticsEx();
  testGetIpStatisticsEx();
  testGetTcpStatisticsEx();
  testGetUdpStatisticsEx();
  testGetTcpTable();
  testGetUdpTable();
  testSetTcpEntry();
  testIcmpSendEcho();
}

static void testGetInterfaceInfo(void)
{
    DWORD apiReturn;
    ULONG len = 0, i;

    apiReturn = GetInterfaceInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetInterfaceInfo is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetInterfaceInfo returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetInterfaceInfo(NULL, &len);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetInterfaceInfo returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PIP_INTERFACE_INFO buf = HeapAlloc(GetProcessHeap(), 0, len);

        apiReturn = GetInterfaceInfo(buf, &len);
        ok(apiReturn == NO_ERROR,
           "GetInterfaceInfo(buf, &dwSize) returned %d, expected NO_ERROR\n",
           apiReturn);

        for (i = 0; i < buf->NumAdapters; i++)
        {
            MIB_IFROW row = { .dwIndex = buf->Adapter[i].Index };
            GetIfEntry( &row );
            ok( !wcscmp( buf->Adapter[i].Name, row.wszName ), "got %s vs %s\n",
                debugstr_w( buf->Adapter[i].Name ), debugstr_w( row.wszName ) );
todo_wine_if( row.dwType == IF_TYPE_SOFTWARE_LOOPBACK)
            ok( row.dwType != IF_TYPE_SOFTWARE_LOOPBACK, "got loopback\n" );
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testGetAdaptersInfo(void)
{
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = GetAdaptersInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetAdaptersInfo is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetAdaptersInfo returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetAdaptersInfo(NULL, &len);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_BUFFER_OVERFLOW,
       "GetAdaptersInfo returned %d, expected ERROR_NO_DATA or ERROR_BUFFER_OVERFLOW\n",
       apiReturn);
    if (apiReturn == ERROR_NO_DATA)
        ; /* no adapter's, that's okay */
    else if (apiReturn == ERROR_BUFFER_OVERFLOW) {
        PIP_ADAPTER_INFO ptr, buf = HeapAlloc(GetProcessHeap(), 0, len);
        NET_LUID luid;
        GUID guid;
        char AdapterName[ARRAY_SIZE(ptr->AdapterName)];

        apiReturn = GetAdaptersInfo(buf, &len);
        ok(apiReturn == NO_ERROR,
           "GetAdaptersInfo(buf, &dwSize) returned %d, expected NO_ERROR\n",
           apiReturn);
        ptr = buf;
        while (ptr) {
            ConvertInterfaceIndexToLuid(ptr->Index, &luid);
            ConvertInterfaceLuidToGuid(&luid, &guid);
            sprintf(AdapterName, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                    guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                    guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
                    guid.Data4[6], guid.Data4[7]);
            ok(!strcmp(ptr->AdapterName, AdapterName), "expected '%s' got '%s'\n", ptr->AdapterName, AdapterName);
            ok(ptr->IpAddressList.IpAddress.String[0], "A valid IP address must be present\n");
            ok(ptr->IpAddressList.IpMask.String[0], "A valid mask must be present\n");
            ok(ptr->GatewayList.IpAddress.String[0], "A valid IP address must be present\n");
            ok(ptr->GatewayList.IpMask.String[0], "A valid mask must be present\n");
            trace("adapter '%s', address %s/%s gateway %s/%s\n", ptr->AdapterName,
                  ptr->IpAddressList.IpAddress.String, ptr->IpAddressList.IpMask.String,
                  ptr->GatewayList.IpAddress.String, ptr->GatewayList.IpMask.String);
            ptr = ptr->Next;
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void testGetNetworkParams(void)
{
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = GetNetworkParams(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetNetworkParams is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetNetworkParams returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetNetworkParams(NULL, &len);
    ok(apiReturn == ERROR_BUFFER_OVERFLOW,
       "GetNetworkParams returned %d, expected ERROR_BUFFER_OVERFLOW\n",
       apiReturn);
    if (apiReturn == ERROR_BUFFER_OVERFLOW) {
        PFIXED_INFO buf = HeapAlloc(GetProcessHeap(), 0, len);

        apiReturn = GetNetworkParams(buf, &len);
        ok(apiReturn == NO_ERROR,
           "GetNetworkParams(buf, &dwSize) returned %d, expected NO_ERROR\n",
           apiReturn);
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

/*
still-to-be-tested 98-onward functions:
GetBestInterface
GetBestRoute
IpReleaseAddress
IpRenewAddress
*/
static DWORD CALLBACK testWin98Functions(void *p)
{
  testGetInterfaceInfo();
  testGetAdaptersInfo();
  testGetNetworkParams();
  return 0;
}

static void testGetPerAdapterInfo(void)
{
    DWORD ret, needed;
    void *buffer;

    ret = GetPerAdapterInfo(1, NULL, NULL);
    if (ret == ERROR_NOT_SUPPORTED) {
      skip("GetPerAdapterInfo is not supported\n");
      return;
    }
    ok( ret == ERROR_INVALID_PARAMETER, "got %u instead of ERROR_INVALID_PARAMETER\n", ret );
    needed = 0xdeadbeef;
    ret = GetPerAdapterInfo(1, NULL, &needed);
    if (ret == ERROR_NO_DATA) return;  /* no such adapter */
    ok( ret == ERROR_BUFFER_OVERFLOW, "got %u instead of ERROR_BUFFER_OVERFLOW\n", ret );
    ok( needed != 0xdeadbeef, "needed not set\n" );
    buffer = HeapAlloc( GetProcessHeap(), 0, needed );
    ret = GetPerAdapterInfo(1, buffer, &needed);
    ok( ret == NO_ERROR, "got %u instead of NO_ERROR\n", ret );
    HeapFree( GetProcessHeap(), 0, buffer );
}

static void testNotifyAddrChange(void)
{
    DWORD ret, bytes;
    OVERLAPPED overlapped;
    HANDLE handle;
    BOOL success;

    handle = NULL;
    ZeroMemory(&overlapped, sizeof(overlapped));
    ret = NotifyAddrChange(&handle, &overlapped);
    ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %d, expected ERROR_IO_PENDING\n", ret);
    ret = GetLastError();
    todo_wine ok(ret == ERROR_IO_PENDING, "GetLastError returned %d, expected ERROR_IO_PENDING\n", ret);
    success = CancelIPChangeNotify(&overlapped);
    todo_wine ok(success == TRUE, "CancelIPChangeNotify returned FALSE, expected TRUE\n");

    ZeroMemory(&overlapped, sizeof(overlapped));
    success = CancelIPChangeNotify(&overlapped);
    ok(success == FALSE, "CancelIPChangeNotify returned TRUE, expected FALSE\n");

    handle = NULL;
    ZeroMemory(&overlapped, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    ret = NotifyAddrChange(&handle, &overlapped);
    ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %d, expected ERROR_IO_PENDING\n", ret);
    todo_wine ok(handle != INVALID_HANDLE_VALUE, "NotifyAddrChange returned invalid file handle\n");
    success = GetOverlappedResult(handle, &overlapped, &bytes, FALSE);
    ok(success == FALSE, "GetOverlappedResult returned TRUE, expected FALSE\n");
    ret = GetLastError();
    ok(ret == ERROR_IO_INCOMPLETE, "GetLastError returned %d, expected ERROR_IO_INCOMPLETE\n", ret);
    success = CancelIPChangeNotify(&overlapped);
    todo_wine ok(success == TRUE, "CancelIPChangeNotify returned FALSE, expected TRUE\n");

    if (winetest_interactive)
    {
        handle = NULL;
        ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        trace("Testing asynchronous ipv4 address change notification. Please "
              "change the ipv4 address of one of your network interfaces\n");
        ret = NotifyAddrChange(&handle, &overlapped);
        ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %d, expected NO_ERROR\n", ret);
        success = GetOverlappedResult(handle, &overlapped, &bytes, TRUE);
        ok(success == TRUE, "GetOverlappedResult returned FALSE, expected TRUE\n");
    }

    /* test synchronous functionality */
    if (winetest_interactive)
    {
        trace("Testing synchronous ipv4 address change notification. Please "
              "change the ipv4 address of one of your network interfaces\n");
        ret = NotifyAddrChange(NULL, NULL);
        todo_wine ok(ret == NO_ERROR, "NotifyAddrChange returned %d, expected NO_ERROR\n", ret);
    }
}

/*
still-to-be-tested 2K-onward functions:
AddIPAddress
CreateProxyArpEntry
DeleteIPAddress
DeleteProxyArpEntry
EnableRouter
FlushIpNetTable
GetAdapterIndex
NotifyRouteChange + CancelIPChangeNotify
SendARP
UnenableRouter
*/
static void testWin2KFunctions(void)
{
    testGetPerAdapterInfo();
    testNotifyAddrChange();
}

static void test_GetAdaptersAddresses(void)
{
    BOOL dns_eligible_found = FALSE;
    ULONG ret, size, osize, i;
    IP_ADAPTER_ADDRESSES *aa, *ptr;
    IP_ADAPTER_UNICAST_ADDRESS *ua;

    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", ret);

    /* size should be ignored and overwritten if buffer is NULL */
    size = 0x7fffffff;
    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    ok(ret == ERROR_BUFFER_OVERFLOW, "expected ERROR_BUFFER_OVERFLOW, got %u\n", ret);
    if (ret != ERROR_BUFFER_OVERFLOW) return;

    ptr = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, ptr, &size);
    ok(!ret, "expected ERROR_SUCCESS got %u\n", ret);
    HeapFree(GetProcessHeap(), 0, ptr);

    /* higher size must not be changed to lower size */
    size *= 2;
    osize = size;
    ptr = HeapAlloc(GetProcessHeap(), 0, osize);
    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, ptr, &osize);
    ok(!ret, "expected ERROR_SUCCESS got %u\n", ret);
    ok(osize == size, "expected %d, got %d\n", size, osize);

    for (aa = ptr; !ret && aa; aa = aa->Next)
    {
        char temp[128], buf[39];
        IP_ADAPTER_PREFIX *prefix;
        DWORD status;
        GUID guid;

        ok(S(U(*aa)).Length == sizeof(IP_ADAPTER_ADDRESSES_LH) ||
           S(U(*aa)).Length == sizeof(IP_ADAPTER_ADDRESSES_XP),
           "Unknown structure size of %u bytes\n", S(U(*aa)).Length);
        ok(aa->DnsSuffix != NULL, "DnsSuffix is not a valid pointer\n");
        ok(aa->Description != NULL, "Description is not a valid pointer\n");
        ok(aa->FriendlyName != NULL, "FriendlyName is not a valid pointer\n");

        for (i = 0; i < aa->PhysicalAddressLength; i++)
            sprintf(temp + i * 3, "%02X-", aa->PhysicalAddress[i]);
        temp[i ? i * 3 - 1 : 0] = '\0';
        trace("idx %u name %s %s dns %s descr %s phys %s mtu %u flags %08x type %u\n",
              S(U(*aa)).IfIndex, aa->AdapterName,
              wine_dbgstr_w(aa->FriendlyName), wine_dbgstr_w(aa->DnsSuffix),
              wine_dbgstr_w(aa->Description), temp, aa->Mtu, aa->Flags, aa->IfType );
        ua = aa->FirstUnicastAddress;
        while (ua)
        {
            ok(S(U(*ua)).Length == sizeof(IP_ADAPTER_UNICAST_ADDRESS_LH) ||
               S(U(*ua)).Length == sizeof(IP_ADAPTER_UNICAST_ADDRESS_XP),
               "Unknown structure size of %u bytes\n", S(U(*ua)).Length);
            ok(ua->PrefixOrigin != IpPrefixOriginOther,
               "bad address config value %d\n", ua->PrefixOrigin);
            ok(ua->SuffixOrigin != IpSuffixOriginOther,
               "bad address config value %d\n", ua->PrefixOrigin);
            /* Address configured manually or from DHCP server? */
            if (ua->PrefixOrigin == IpPrefixOriginManual ||
                ua->PrefixOrigin == IpPrefixOriginDhcp)
            {
                ok(ua->ValidLifetime, "expected non-zero value\n");
                ok(ua->PreferredLifetime, "expected non-zero value\n");
                ok(ua->LeaseLifetime, "expected non-zero\n");
            }
            /* Is the address ok in the network (not duplicated)? */
            ok(ua->DadState != IpDadStateInvalid && ua->DadState != IpDadStateDuplicate,
               "bad address duplication value %d\n", ua->DadState);
            trace("  flags %08x origin %u/%u state %u lifetime %u/%u/%u prefix %u\n",
                  S(U(*ua)).Flags, ua->PrefixOrigin, ua->SuffixOrigin, ua->DadState,
                  ua->ValidLifetime, ua->PreferredLifetime, ua->LeaseLifetime,
                  S(U(*ua)).Length < sizeof(IP_ADAPTER_UNICAST_ADDRESS_LH) ? 0 : ua->OnLinkPrefixLength);

            if (ua->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE)
                dns_eligible_found = TRUE;

            ua = ua->Next;
        }
        for (i = 0, temp[0] = '\0'; i < ARRAY_SIZE(aa->ZoneIndices); i++)
            sprintf(temp + strlen(temp), "%d ", aa->ZoneIndices[i]);
        trace("status %u index %u zone %s\n", aa->OperStatus, aa->Ipv6IfIndex, temp );
        prefix = aa->FirstPrefix;
        while (prefix)
        {
            trace( "  prefix %u/%u flags %08x\n", prefix->Address.iSockaddrLength,
                   prefix->PrefixLength, S(U(*prefix)).Flags );
            prefix = prefix->Next;
        }

        if (S(U(*aa)).Length < sizeof(IP_ADAPTER_ADDRESSES_LH)) continue;
        trace("speed %s/%s metrics %u/%u guid %s type %u/%u\n",
              wine_dbgstr_longlong(aa->TransmitLinkSpeed),
              wine_dbgstr_longlong(aa->ReceiveLinkSpeed),
              aa->Ipv4Metric, aa->Ipv6Metric, wine_dbgstr_guid((GUID*) &aa->NetworkGuid),
              aa->ConnectionType, aa->TunnelType);

        status = ConvertInterfaceLuidToGuid(&aa->Luid, &guid);
        ok(!status, "got %u\n", status);
        sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
                guid.Data4[6], guid.Data4[7]);
        ok(!strcasecmp(aa->AdapterName, buf), "expected '%s' got '%s'\n", aa->AdapterName, buf);
    }
    ok(dns_eligible_found, "Did not find any dns eligible addresses.\n");
    HeapFree(GetProcessHeap(), 0, ptr);
}

static void test_GetExtendedTcpTable(void)
{
    DWORD ret, size;
    MIB_TCPTABLE *table;
    MIB_TCPTABLE_OWNER_PID *table_pid;
    MIB_TCPTABLE_OWNER_MODULE *table_module;

    if (!pGetExtendedTcpTable)
    {
        win_skip("GetExtendedTcpTable not available\n");
        return;
    }
    ret = pGetExtendedTcpTable( NULL, NULL, TRUE, AF_INET, TCP_TABLE_BASIC_ALL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_BASIC_ALL, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table, &size, TRUE, AF_INET, TCP_TABLE_BASIC_ALL, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_BASIC_LISTENER, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table, &size, TRUE, AF_INET, TCP_TABLE_BASIC_LISTENER, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_pid = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_pid, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_pid );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_pid = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_pid, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_pid );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_module = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_module, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_module );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_module = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_module, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_module );
}

static void test_AllocateAndGetTcpExTableFromStack(void)
{
    DWORD ret;
    MIB_TCPTABLE_OWNER_PID *table_ex = NULL;

    if (!pAllocateAndGetTcpExTableFromStack)
    {
        skip("AllocateAndGetTcpExTableFromStack not available\n");
        return;
    }

    if (0)
    {
        /* crashes on native */
        ret = pAllocateAndGetTcpExTableFromStack( NULL, FALSE, INVALID_HANDLE_VALUE, 0, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
        ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, INVALID_HANDLE_VALUE, 0, AF_INET );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
        ret = pAllocateAndGetTcpExTableFromStack( NULL, FALSE, GetProcessHeap(), 0, AF_INET );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    }

    ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, GetProcessHeap(), 0, 0 );
    ok( ret == ERROR_INVALID_PARAMETER || broken(ret == ERROR_NOT_SUPPORTED) /* win2k */, "got %u\n", ret );

    ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, GetProcessHeap(), 0, AF_INET );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );

    if (ret == NO_ERROR && winetest_debug > 1)
    {
        DWORD i;
        trace( "AllocateAndGetTcpExTableFromStack table: %u entries\n", table_ex->dwNumEntries );
        for (i = 0; i < table_ex->dwNumEntries; i++)
        {
          char remote_ip[16];

          strcpy(remote_ip, ntoa(table_ex->table[i].dwRemoteAddr));
          trace( "%u: local %s:%u remote %s:%u state %u pid %u\n", i,
                 ntoa(table_ex->table[i].dwLocalAddr), ntohs(table_ex->table[i].dwLocalPort),
                 remote_ip, ntohs(table_ex->table[i].dwRemotePort),
                 U(table_ex->table[i]).dwState, table_ex->table[i].dwOwningPid );
        }
    }
    HeapFree(GetProcessHeap(), 0, table_ex);

    ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, GetProcessHeap(), 0, AF_INET6 );
    ok( ret == ERROR_NOT_SUPPORTED, "got %u\n", ret );
}

static void test_GetExtendedUdpTable(void)
{
    DWORD ret, size;
    MIB_UDPTABLE *table;
    MIB_UDPTABLE_OWNER_PID *table_pid;
    MIB_UDPTABLE_OWNER_MODULE *table_module;

    if (!pGetExtendedUdpTable)
    {
        win_skip("GetExtendedUdpTable not available\n");
        return;
    }
    ret = pGetExtendedUdpTable( NULL, NULL, TRUE, AF_INET, UDP_TABLE_BASIC, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    size = 0;
    ret = pGetExtendedUdpTable( NULL, &size, TRUE, AF_INET, UDP_TABLE_BASIC, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedUdpTable( table, &size, TRUE, AF_INET, UDP_TABLE_BASIC, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table );

    size = 0;
    ret = pGetExtendedUdpTable( NULL, &size, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_pid = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedUdpTable( table_pid, &size, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_pid );

    size = 0;
    ret = pGetExtendedUdpTable( NULL, &size, TRUE, AF_INET, UDP_TABLE_OWNER_MODULE, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_module = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedUdpTable( table_module, &size, TRUE, AF_INET, UDP_TABLE_OWNER_MODULE, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_module );
}

static void test_CreateSortedAddressPairs(void)
{
    SOCKADDR_IN6 dst[2];
    SOCKADDR_IN6_PAIR *pair;
    ULONG pair_count;
    DWORD ret;

    if (!pCreateSortedAddressPairs)
    {
        win_skip( "CreateSortedAddressPairs not available\n" );
        return;
    }

    memset( dst, 0, sizeof(dst) );
    dst[0].sin6_family = AF_INET6;
    dst[0].sin6_addr.u.Word[5] = 0xffff;
    dst[0].sin6_addr.u.Word[6] = 0x0808;
    dst[0].sin6_addr.u.Word[7] = 0x0808;

    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 1, 0, NULL, &pair_count );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    ok( pair_count == 0xdeadbeef, "got %u\n", pair_count );

    pair = (SOCKADDR_IN6_PAIR *)0xdeadbeef;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, NULL, 1, 0, &pair, &pair_count );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    ok( pair == (SOCKADDR_IN6_PAIR *)0xdeadbeef, "got %p\n", pair );
    ok( pair_count == 0xdeadbeef, "got %u\n", pair_count );

    pair = NULL;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 1, 0, &pair, &pair_count );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( pair != NULL, "pair not set\n" );
    ok( pair_count >= 1, "got %u\n", pair_count );
    ok( pair[0].SourceAddress != NULL, "src address not set\n" );
    ok( pair[0].DestinationAddress != NULL, "dst address not set\n" );
    FreeMibTable( pair );

    dst[1].sin6_family = AF_INET6;
    dst[1].sin6_addr.u.Word[5] = 0xffff;
    dst[1].sin6_addr.u.Word[6] = 0x0404;
    dst[1].sin6_addr.u.Word[7] = 0x0808;

    pair = NULL;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 2, 0, &pair, &pair_count );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( pair != NULL, "pair not set\n" );
    ok( pair_count >= 2, "got %u\n", pair_count );
    ok( pair[0].SourceAddress != NULL, "src address not set\n" );
    ok( pair[0].DestinationAddress != NULL, "dst address not set\n" );
    ok( pair[1].SourceAddress != NULL, "src address not set\n" );
    ok( pair[1].DestinationAddress != NULL, "dst address not set\n" );
    FreeMibTable( pair );
}

static DWORD get_interface_index(void)
{
    DWORD size = 0, ret = 0;
    IP_ADAPTER_ADDRESSES *buf, *aa;

    if (GetAdaptersAddresses( AF_UNSPEC, 0, NULL, NULL, &size ) != ERROR_BUFFER_OVERFLOW)
        return 0;

    buf = HeapAlloc( GetProcessHeap(), 0, size );
    GetAdaptersAddresses( AF_UNSPEC, 0, NULL, buf, &size );
    for (aa = buf; aa; aa = aa->Next)
    {
        if (aa->IfType == IF_TYPE_ETHERNET_CSMACD)
        {
            ret = aa->IfIndex;
            break;
        }
    }
    HeapFree( GetProcessHeap(), 0, buf );
    return ret;
}

static void convert_luid_to_name( NET_LUID *luid, WCHAR *expect_nameW, int len )
{
    struct
    {
        const WCHAR *prefix;
        DWORD type;
    } prefixes[] =
    {
        { L"other", IF_TYPE_OTHER },
        { L"ethernet", IF_TYPE_ETHERNET_CSMACD },
        { L"tokenring", IF_TYPE_ISO88025_TOKENRING },
        { L"ppp", IF_TYPE_PPP },
        { L"loopback", IF_TYPE_SOFTWARE_LOOPBACK },
        { L"atm", IF_TYPE_ATM },
        { L"wireless", IF_TYPE_IEEE80211 },
        { L"tunnel", IF_TYPE_TUNNEL },
        { L"ieee1394", IF_TYPE_IEEE1394 }
    };
    DWORD i;
    const WCHAR *prefix = NULL;

    for (i = 0; i < ARRAY_SIZE(prefixes); i++)
    {
        if (prefixes[i].type == luid->Info.IfType)
        {
            prefix = prefixes[i].prefix;
            break;
        }
    }
    if (prefix)
        swprintf( expect_nameW, len, L"%s_%d", prefix, luid->Info.NetLuidIndex );
    else
        swprintf( expect_nameW, len, L"iftype%d_%d", luid->Info.IfType, luid->Info.NetLuidIndex );
}

static void test_interface_identifier_conversion(void)
{
    DWORD ret, i;
    NET_LUID luid;
    GUID guid;
    SIZE_T len;
    WCHAR nameW[IF_MAX_STRING_SIZE + 1];
    WCHAR alias[IF_MAX_STRING_SIZE + 1];
    WCHAR expect_nameW[IF_MAX_STRING_SIZE + 1];
    char nameA[IF_MAX_STRING_SIZE + 1], *name;
    char expect_nameA[IF_MAX_STRING_SIZE + 1];
    NET_IFINDEX index;
    MIB_IF_TABLE2 *table;

    ret = GetIfTable2( &table );
    ok( !ret, "got %d\n", ret );

    for (i = 0; i < table->NumEntries; i++)
    {
        MIB_IF_ROW2 *row = table->Table + i;

        /* ConvertInterfaceIndexToLuid */
        ret = ConvertInterfaceIndexToLuid( 0, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        memset( &luid, 0xff, sizeof(luid) );
        ret = ConvertInterfaceIndexToLuid( 0, &luid );
        ok( ret == ERROR_FILE_NOT_FOUND, "got %u\n", ret );
        ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
        ok( !luid.Info.NetLuidIndex, "got %u\n", luid.Info.NetLuidIndex );
        ok( !luid.Info.IfType, "got %u\n", luid.Info.IfType );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceIndexToLuid( row->InterfaceIndex, &luid );
        ok( !ret, "got %u\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceLuidToIndex */
        ret = ConvertInterfaceLuidToIndex( NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToIndex( NULL, &index );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToIndex( &luid, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToIndex( &luid, &index );
        ok( !ret, "got %u\n", ret );
        ok( index == row->InterfaceIndex, "mismatch\n" );

        /* ConvertInterfaceLuidToGuid */
        ret = ConvertInterfaceLuidToGuid( NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        memset( &guid, 0xff, sizeof(guid) );
        ret = ConvertInterfaceLuidToGuid( NULL, &guid );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
        ok( guid.Data1 == 0xffffffff, "got %x\n", guid.Data1 );

        ret = ConvertInterfaceLuidToGuid( &luid, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        memset( &guid, 0, sizeof(guid) );
        ret = ConvertInterfaceLuidToGuid( &luid, &guid );
        ok( !ret, "got %u\n", ret );
        ok( IsEqualGUID( &guid, &row->InterfaceGuid ), "mismatch\n" );

        /* ConvertInterfaceGuidToLuid */
        ret = ConvertInterfaceGuidToLuid( NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        luid.Info.NetLuidIndex = 1;
        ret = ConvertInterfaceGuidToLuid( NULL, &luid );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
        ok( luid.Info.NetLuidIndex == 1, "got %u\n", luid.Info.NetLuidIndex );

        ret = ConvertInterfaceGuidToLuid( &guid, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceGuidToLuid( &guid, &luid );
        ok( !ret, "got %u\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value ||
            broken( luid.Value != row->InterfaceLuid.Value), /* Win8 can have identical guids for two different ifaces */
            "mismatch\n" );
        if (luid.Value != row->InterfaceLuid.Value) continue;

        /* ConvertInterfaceLuidToNameW */
        ret = ConvertInterfaceLuidToNameW( NULL, NULL, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToNameW( &luid, NULL, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToNameW( NULL, nameW, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToNameW( &luid, nameW, 0 );
        ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %u\n", ret );

        nameW[0] = 0;
        len = ARRAY_SIZE(nameW);
        ret = ConvertInterfaceLuidToNameW( &luid, nameW, len );
        ok( !ret, "got %u\n", ret );
        convert_luid_to_name( &luid, expect_nameW, len );
        ok( !wcscmp( nameW, expect_nameW ), "got %s vs %s\n", debugstr_w( nameW ), debugstr_w( expect_nameW ) );

        /* ConvertInterfaceLuidToNameA */
        ret = ConvertInterfaceLuidToNameA( NULL, NULL, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToNameA( &luid, NULL, 0 );
        ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %u\n", ret );

        ret = ConvertInterfaceLuidToNameA( NULL, nameA, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        ret = ConvertInterfaceLuidToNameA( &luid, nameA, 0 );
        ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %u\n", ret );

        nameA[0] = 0;
        len = ARRAY_SIZE(nameA);
        ret = ConvertInterfaceLuidToNameA( &luid, nameA, len );
        ok( !ret, "got %u\n", ret );
        ok( nameA[0], "name not set\n" );

        /* ConvertInterfaceNameToLuidW */
        ret = ConvertInterfaceNameToLuidW( NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidW( NULL, &luid );
        ok( ret == ERROR_INVALID_NAME, "got %u\n", ret );
        ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
        ok( luid.Info.NetLuidIndex != 0xdead, "index not set\n" );
        ok( !luid.Info.IfType, "got %u\n", luid.Info.IfType );

        ret = ConvertInterfaceNameToLuidW( nameW, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidW( nameW, &luid );
        ok( !ret, "got %u\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceNameToLuidA */
        ret = ConvertInterfaceNameToLuidA( NULL, NULL );
        ok( ret == ERROR_INVALID_NAME, "got %u\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidA( NULL, &luid );
        ok( ret == ERROR_INVALID_NAME, "got %u\n", ret );
        ok( luid.Info.Reserved == 0xdead, "reserved set\n" );
        ok( luid.Info.NetLuidIndex == 0xdead, "index set\n" );
        ok( luid.Info.IfType == 0xdead, "type set\n" );

        ret = ConvertInterfaceNameToLuidA( nameA, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidA( nameA, &luid );
        ok( !ret, "got %u\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceAliasToLuid */
        ret = ConvertInterfaceAliasToLuid( row->Alias, &luid );
        ok( !ret, "got %u\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceLuidToAlias */
        ret = ConvertInterfaceLuidToAlias( &row->InterfaceLuid, alias, ARRAY_SIZE(alias) );
        ok( !ret, "got %u\n", ret );
        ok( !wcscmp( alias, row->Alias ), "got %s vs %s\n", wine_dbgstr_w( alias ), wine_dbgstr_w( row->Alias ) );

        index = if_nametoindex( NULL );
        ok( !index, "Got unexpected index %u\n", index );
        index = if_nametoindex( nameA );
        ok( index == row->InterfaceIndex, "Got index %u for %s, expected %u\n", index, nameA, row->InterfaceIndex );
        /* Wargaming.net Game Center passes a GUID-like string. */
        index = if_nametoindex( "{00000001-0000-0000-0000-000000000000}" );
        ok( !index, "Got unexpected index %u\n", index );
        index = if_nametoindex( wine_dbgstr_guid( &guid ) );
        ok( !index, "Got unexpected index %u for input %s\n", index, wine_dbgstr_guid( &guid ) );

        name = if_indextoname( 0, NULL );
        ok( name == NULL, "got %s\n", name );

        name = if_indextoname( 0, nameA );
        ok( name == NULL, "got %p\n", name );

        name = if_indextoname( ~0u, nameA );
        ok( name == NULL, "got %p\n", name );

        nameA[0] = 0;
        name = if_indextoname( row->InterfaceIndex, nameA );
        ConvertInterfaceLuidToNameA( &row->InterfaceLuid, expect_nameA, ARRAY_SIZE(expect_nameA) );
        ok( name == nameA, "mismatch\n" );
        ok( !strcmp( nameA, expect_nameA ), "mismatch\n" );
    }
    FreeMibTable( table );
}

static void test_GetIfEntry2(void)
{
    DWORD ret;
    MIB_IF_ROW2 row;
    NET_IFINDEX index;

    if (!(index = get_interface_index()))
    {
        skip( "no suitable interface found\n" );
        return;
    }

    ret = GetIfEntry2( NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    ret = GetIfEntry2( &row );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = index;
    ret = GetIfEntry2( &row );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( row.InterfaceIndex == index, "got %u\n", index );
}

static void test_GetIfTable2(void)
{
    DWORD ret;
    MIB_IF_TABLE2 *table;

    table = NULL;
    ret = GetIfTable2( &table );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( table != NULL, "table not set\n" );
    FreeMibTable( table );
}

static void test_GetIfTable2Ex(void)
{
    DWORD ret;
    MIB_IF_TABLE2 *table;

    table = NULL;
    ret = GetIfTable2Ex( MibIfTableNormal, &table );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( table != NULL, "table not set\n" );
    FreeMibTable( table );

    table = NULL;
    ret = GetIfTable2Ex( MibIfTableRaw, &table );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( table != NULL, "table not set\n" );
    FreeMibTable( table );

    table = NULL;
    ret = GetIfTable2Ex( MibIfTableNormalWithoutStatistics, &table );
    ok( ret == NO_ERROR || broken(ret == ERROR_INVALID_PARAMETER), "got %u\n", ret );
    ok( table != NULL || broken(!table), "table not set\n" );
    FreeMibTable( table );

    table = NULL;
    ret = GetIfTable2Ex( 3, &table );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    ok( !table, "table should not be set\n" );
    FreeMibTable( table );
}

static void test_GetUnicastIpAddressEntry(void)
{
    IP_ADAPTER_ADDRESSES *aa, *ptr;
    MIB_UNICASTIPADDRESS_ROW row;
    DWORD ret, size;

    if (!pGetUnicastIpAddressEntry)
    {
        win_skip( "GetUnicastIpAddressEntry not available\n" );
        return;
    }

    ret = pGetUnicastIpAddressEntry( NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    row.Address.Ipv4.sin_family = AF_INET;
    row.Address.Ipv4.sin_port = 0;
    row.Address.Ipv4.sin_addr.S_un.S_addr = 0x01020304;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_FILE_NOT_FOUND, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = 123;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = get_interface_index();
    row.Address.Ipv4.sin_family = AF_INET;
    row.Address.Ipv4.sin_port = 0;
    row.Address.Ipv4.sin_addr.S_un.S_addr = 0x01020304;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_NOT_FOUND, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = 123;
    row.Address.Ipv4.sin_family = AF_INET;
    row.Address.Ipv4.sin_port = 0;
    row.Address.Ipv4.sin_addr.S_un.S_addr = 0x01020304;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_FILE_NOT_FOUND, "got %u\n", ret );

    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, NULL, &size);
    ok(ret == ERROR_BUFFER_OVERFLOW, "expected ERROR_BUFFER_OVERFLOW, got %u\n", ret);
    if (ret != ERROR_BUFFER_OVERFLOW) return;

    ptr = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, ptr, &size);
    ok(!ret, "expected ERROR_SUCCESS got %u\n", ret);

    for (aa = ptr; !ret && aa; aa = aa->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *ua;

        ua = aa->FirstUnicastAddress;
        while (ua)
        {
            /* test with luid */
            memset( &row, 0, sizeof(row) );
            memcpy(&row.InterfaceLuid, &aa->Luid, sizeof(aa->Luid));
            memcpy(&row.Address, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
            ret = pGetUnicastIpAddressEntry( &row );
            ok( ret == NO_ERROR, "got %u\n", ret );

            /* test with index */
            memset( &row, 0, sizeof(row) );
            row.InterfaceIndex = S(U(*aa)).IfIndex;
            memcpy(&row.Address, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
            ret = pGetUnicastIpAddressEntry( &row );
            ok( ret == NO_ERROR, "got %u\n", ret );
            if (ret == NO_ERROR)
            {
                ok(row.InterfaceLuid.Info.Reserved == aa->Luid.Info.Reserved, "Expected %d, got %d\n",
                    aa->Luid.Info.Reserved, row.InterfaceLuid.Info.Reserved);
                ok(row.InterfaceLuid.Info.NetLuidIndex == aa->Luid.Info.NetLuidIndex, "Expected %d, got %d\n",
                    aa->Luid.Info.NetLuidIndex, row.InterfaceLuid.Info.NetLuidIndex);
                ok(row.InterfaceLuid.Info.IfType == aa->Luid.Info.IfType, "Expected %d, got %d\n",
                    aa->Luid.Info.IfType, row.InterfaceLuid.Info.IfType);
                ok(row.InterfaceIndex == S(U(*aa)).IfIndex, "Expected %d, got %d\n",
                    S(U(*aa)).IfIndex, row.InterfaceIndex);
                ok(row.PrefixOrigin == ua->PrefixOrigin, "Expected %d, got %d\n",
                    ua->PrefixOrigin, row.PrefixOrigin);
                ok(row.SuffixOrigin == ua->SuffixOrigin, "Expected %d, got %d\n",
                    ua->SuffixOrigin, row.SuffixOrigin);
                ok(row.ValidLifetime == ua->ValidLifetime, "Expected %d, got %d\n",
                    ua->ValidLifetime, row.ValidLifetime);
                ok(row.PreferredLifetime == ua->PreferredLifetime, "Expected %d, got %d\n",
                    ua->PreferredLifetime, row.PreferredLifetime);
                ok(row.OnLinkPrefixLength == ua->OnLinkPrefixLength, "Expected %d, got %d\n",
                    ua->OnLinkPrefixLength, row.OnLinkPrefixLength);
                ok(row.SkipAsSource == 0, "Expected 0, got %d\n", row.SkipAsSource);
                ok(row.DadState == ua->DadState, "Expected %d, got %d\n", ua->DadState, row.DadState);
                if (row.Address.si_family == AF_INET6)
                    ok(row.ScopeId.Value == row.Address.Ipv6.sin6_scope_id, "Expected %d, got %d\n",
                        row.Address.Ipv6.sin6_scope_id, row.ScopeId.Value);
                ok(row.CreationTimeStamp.QuadPart, "CreationTimeStamp is 0\n");
            }
            ua = ua->Next;
        }
    }
    HeapFree(GetProcessHeap(), 0, ptr);
}

static void test_GetUnicastIpAddressTable(void)
{
    MIB_UNICASTIPADDRESS_TABLE *table;
    DWORD ret;
    ULONG i;

    if (!pGetUnicastIpAddressTable)
    {
        win_skip( "GetUnicastIpAddressTable not available\n" );
        return;
    }

    ret = pGetUnicastIpAddressTable(AF_UNSPEC, NULL);
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pGetUnicastIpAddressTable(AF_BAN, &table);
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pGetUnicastIpAddressTable(AF_INET, &table);
    ok( ret == NO_ERROR, "got %u\n", ret );
    trace("GetUnicastIpAddressTable(AF_INET): NumEntries %u\n", table->NumEntries);
    FreeMibTable( table );

    ret = pGetUnicastIpAddressTable(AF_INET6, &table);
    ok( ret == NO_ERROR, "got %u\n", ret );
    trace("GetUnicastIpAddressTable(AF_INET6): NumEntries %u\n", table->NumEntries);
    FreeMibTable( table );

    ret = pGetUnicastIpAddressTable(AF_UNSPEC, &table);
    ok( ret == NO_ERROR, "got %u\n", ret );
    trace("GetUnicastIpAddressTable(AF_UNSPEC): NumEntries %u\n", table->NumEntries);
    for (i = 0; i < table->NumEntries && winetest_debug > 1; i++)
    {
        trace("Index %u:\n", i);
        trace("Address.si_family:               %u\n", table->Table[i].Address.si_family);
        trace("InterfaceLuid.Info.Reserved:     %u\n", table->Table[i].InterfaceLuid.Info.Reserved);
        trace("InterfaceLuid.Info.NetLuidIndex: %u\n", table->Table[i].InterfaceLuid.Info.NetLuidIndex);
        trace("InterfaceLuid.Info.IfType:       %u\n", table->Table[i].InterfaceLuid.Info.IfType);
        trace("InterfaceIndex:                  %u\n", table->Table[i].InterfaceIndex);
        trace("PrefixOrigin:                    %u\n", table->Table[i].PrefixOrigin);
        trace("SuffixOrigin:                    %u\n", table->Table[i].SuffixOrigin);
        trace("ValidLifetime:                   %u seconds\n", table->Table[i].ValidLifetime);
        trace("PreferredLifetime:               %u seconds\n", table->Table[i].PreferredLifetime);
        trace("OnLinkPrefixLength:              %u\n", table->Table[i].OnLinkPrefixLength);
        trace("SkipAsSource:                    %u\n", table->Table[i].SkipAsSource);
        trace("DadState:                        %u\n", table->Table[i].DadState);
        trace("ScopeId.Value:                   %u\n", table->Table[i].ScopeId.Value);
        trace("CreationTimeStamp:               %08x%08x\n", table->Table[i].CreationTimeStamp.HighPart, table->Table[i].CreationTimeStamp.LowPart);
    }

    FreeMibTable( table );
}

static void test_ConvertLengthToIpv4Mask(void)
{
    DWORD ret;
    DWORD n;
    ULONG mask;
    ULONG expected;

    if (!pConvertLengthToIpv4Mask)
    {
        win_skip( "ConvertLengthToIpv4Mask not available\n" );
        return;
    }

    for (n = 0; n <= 32; n++)
    {
        mask = 0xdeadbeef;
        if (n > 0)
            expected = htonl( ~0u << (32 - n) );
        else
            expected = 0;

        ret = pConvertLengthToIpv4Mask( n, &mask );
        ok( ret == NO_ERROR, "ConvertLengthToIpv4Mask returned 0x%08x, expected 0x%08x\n", ret, NO_ERROR );
        ok( mask == expected, "ConvertLengthToIpv4Mask mask value 0x%08x, expected 0x%08x\n", mask, expected );
    }

    /* Testing for out of range. In this case both mask and return are changed to indicate error. */
    mask = 0xdeadbeef;
    ret = pConvertLengthToIpv4Mask( 33, &mask );
    ok( ret == ERROR_INVALID_PARAMETER, "ConvertLengthToIpv4Mask returned 0x%08x, expected 0x%08x\n", ret, ERROR_INVALID_PARAMETER );
    ok( mask == INADDR_NONE, "ConvertLengthToIpv4Mask mask value 0x%08x, expected 0x%08x\n", mask, INADDR_NONE );
}

static void test_GetTcp6Table(void)
{
    DWORD ret;
    ULONG size = 0;
    PMIB_TCP6TABLE buf;

    if (!pGetTcp6Table)
    {
        win_skip("GetTcp6Table not available\n");
        return;
    }

    ret = pGetTcp6Table(NULL, &size, FALSE);
    if (ret == ERROR_NOT_SUPPORTED)
    {
        skip("GetTcp6Table is not supported\n");
        return;
    }
    ok(ret == ERROR_INSUFFICIENT_BUFFER,
       "GetTcp6Table(NULL, &size, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n", ret);
    if (ret != ERROR_INSUFFICIENT_BUFFER) return;

    buf = HeapAlloc(GetProcessHeap(), 0, size);

    ret = pGetTcp6Table(buf, &size, FALSE);
    ok(ret == NO_ERROR,
       "GetTcp6Table(buf, &size, FALSE) returned %d, expected NO_ERROR\n", ret);

    if (ret == NO_ERROR && winetest_debug > 1)
    {
        DWORD i;
        trace("TCP6 table: %u entries\n", buf->dwNumEntries);
        for (i = 0; i < buf->dwNumEntries; i++)
        {
            trace("%u: local %s%%%u:%u remote %s%%%u:%u state %u\n", i,
                  ntoa6(&buf->table[i].LocalAddr), ntohs(buf->table[i].dwLocalScopeId),
                  ntohs(buf->table[i].dwLocalPort), ntoa6(&buf->table[i].RemoteAddr),
                  ntohs(buf->table[i].dwRemoteScopeId), ntohs(buf->table[i].dwRemotePort),
                  buf->table[i].State);
        }
    }

    HeapFree(GetProcessHeap(), 0, buf);
}

static void test_GetUdp6Table(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    if (!pGetUdp6Table) {
        win_skip("GetUdp6Table not available\n");
        return;
    }

    apiReturn = pGetUdp6Table(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetUdp6Table is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetUdp6Table(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_UDP6TABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

        apiReturn = pGetUdp6Table(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetUdp6Table(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i;
            trace( "UDP6 table: %u entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
                trace( "%u: %s%%%u:%u\n",
                       i, ntoa6(&buf->table[i].dwLocalAddr), ntohs(buf->table[i].dwLocalScopeId), ntohs(buf->table[i].dwLocalPort) );
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
}

static void test_ParseNetworkString(void)
{
    struct
    {
        char str[32];
        IN_ADDR addr;
        DWORD ret;
    }
    ipv4_address_tests[] =
    {
        {"1.2.3.4",               {{{1, 2, 3, 4}}}},
        {"1.2.3.4a",              {}, ERROR_INVALID_PARAMETER},
        {"1.2.3.0x4a",            {}, ERROR_INVALID_PARAMETER},
        {"1.2.3",                 {}, ERROR_INVALID_PARAMETER},
        {"a1.2.3.4",              {}, ERROR_INVALID_PARAMETER},
        {"0xdeadbeef",            {}, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:22",            {}, ERROR_INVALID_PARAMETER},
        {"::1",                   {}, ERROR_INVALID_PARAMETER},
        {"winehq.org",            {}, ERROR_INVALID_PARAMETER},
    };
    struct
    {
        char str[32];
        IN_ADDR addr;
        DWORD port;
        DWORD ret;
    }
    ipv4_service_tests[] =
    {
        {"1.2.3.4:22",            {{{1, 2, 3, 4}}}, 22},
        {"winehq.org:22",         {}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4",               {}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:0",             {}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:65536",         {}, 0, ERROR_INVALID_PARAMETER},
    };
    WCHAR wstr[IP6_ADDRESS_STRING_BUFFER_LENGTH] = {'1','2','7','.','0','.','0','.','1',':','2','2',0};
    NET_ADDRESS_INFO info;
    USHORT port;
    BYTE prefix_len;
    DWORD ret;
    int i;

    if (!pParseNetworkString)
    {
        win_skip("ParseNetworkString not available\n");
        return;
    }

    ret = pParseNetworkString(wstr, -1, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "expected success, got %d\n", ret);

    ret = pParseNetworkString(NULL, NET_STRING_IPV4_SERVICE, &info, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", ret);

    for (i = 0; i < ARRAY_SIZE(ipv4_address_tests); i++)
    {
        MultiByteToWideChar(CP_ACP, 0, ipv4_address_tests[i].str, sizeof(ipv4_address_tests[i].str),
                            wstr, ARRAY_SIZE(wstr));
        memset(&info, 0x99, sizeof(info));
        port = 0x9999;
        prefix_len = 0x99;

        ret = pParseNetworkString(wstr, NET_STRING_IPV4_ADDRESS, &info, &port, &prefix_len);

        ok(ret == ipv4_address_tests[i].ret,
           "%s gave error %d\n", ipv4_address_tests[i].str, ret);
        ok(info.Format == ret ? NET_ADDRESS_FORMAT_UNSPECIFIED : NET_ADDRESS_IPV4,
           "%s gave format %d\n", ipv4_address_tests[i].str, info.Format);
        ok(info.Ipv4Address.sin_addr.S_un.S_addr == (ret ? 0x99999999 : ipv4_address_tests[i].addr.S_un.S_addr),
           "%s gave address %d.%d.%d.%d\n", ipv4_address_tests[i].str,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b1, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b2,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b3, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b4);
        ok(info.Ipv4Address.sin_port == (ret ? 0x9999 : 0),
           "%s gave port %d\n", ipv4_service_tests[i].str, ntohs(info.Ipv4Address.sin_port));
        ok(port == (ret ? 0x9999 : 0),
           "%s gave port %d\n", ipv4_service_tests[i].str, port);
        ok(prefix_len == (ret ? 0x99 : 255),
           "%s gave prefix length %d\n", ipv4_service_tests[i].str, prefix_len);
    }

    for (i = 0; i < ARRAY_SIZE(ipv4_service_tests); i++)
    {
        MultiByteToWideChar(CP_ACP, 0, ipv4_service_tests[i].str, sizeof(ipv4_service_tests[i].str),
                            wstr, ARRAY_SIZE(wstr));
        memset(&info, 0x99, sizeof(info));
        port = 0x9999;
        prefix_len = 0x99;

        ret = pParseNetworkString(wstr, NET_STRING_IPV4_SERVICE, &info, &port, &prefix_len);

        ok(ret == ipv4_service_tests[i].ret,
           "%s gave error %d\n", ipv4_service_tests[i].str, ret);
        ok(info.Format == ret ? NET_ADDRESS_FORMAT_UNSPECIFIED : NET_ADDRESS_IPV4,
           "%s gave format %d\n", ipv4_address_tests[i].str, info.Format);
        ok(info.Ipv4Address.sin_addr.S_un.S_addr == (ret ? 0x99999999 : ipv4_service_tests[i].addr.S_un.S_addr),
           "%s gave address %d.%d.%d.%d\n", ipv4_service_tests[i].str,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b1, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b2,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b3, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b4);
        ok(ntohs(info.Ipv4Address.sin_port) == (ret ? 0x9999 : ipv4_service_tests[i].port),
           "%s gave port %d\n", ipv4_service_tests[i].str, ntohs(info.Ipv4Address.sin_port));
        ok(port == (ret ? 0x9999 : ipv4_service_tests[i].port),
           "%s gave port %d\n", ipv4_service_tests[i].str, port);
        ok(prefix_len == (ret ? 0x99 : 255),
           "%s gave prefix length %d\n", ipv4_service_tests[i].str, prefix_len);
    }
}

static void WINAPI test_ipaddtess_change_callback(PVOID context, PMIB_UNICASTIPADDRESS_ROW row,
                                                 MIB_NOTIFICATION_TYPE notification_type)
{
    BOOL *callback_called = context;

    *callback_called = TRUE;

    ok(notification_type == MibInitialNotification, "Unexpected notification_type %#x.\n",
            notification_type);
    ok(!row, "Unexpected row %p.\n", row);
}

static void test_NotifyUnicastIpAddressChange(void)
{
    BOOL callback_called;
    HANDLE handle;
    DWORD ret;

    if (!pNotifyUnicastIpAddressChange)
    {
        win_skip("NotifyUnicastIpAddressChange not available.\n");
        return;
    }

    callback_called = FALSE;
    ret = pNotifyUnicastIpAddressChange(AF_INET, test_ipaddtess_change_callback,
            &callback_called, TRUE, &handle);
    ok(ret == NO_ERROR, "Unexpected ret %#x.\n", ret);
    ok(callback_called, "Callback was not called.\n");

    ret = pCancelMibChangeNotify2(handle);
    ok(ret == NO_ERROR, "Unexpected ret %#x.\n", ret);
    ok(!CloseHandle(handle), "CloseHandle() succeeded.\n");
}

static void test_ConvertGuidToString( void )
{
    DWORD err;
    char bufA[39];
    WCHAR bufW[39];
    GUID guid = { 0xa, 0xb, 0xc, { 0xd, 0, 0xe, 0xf } }, guid2;

    err = ConvertGuidToStringA( &guid, bufA, 38 );
    ok( err, "got %d\n", err );
    err = ConvertGuidToStringA( &guid, bufA, 39 );
    ok( !err, "got %d\n", err );
    ok( !strcmp( bufA, "{0000000A-000B-000C-0D00-0E0F00000000}" ), "got %s\n", bufA );

    err = ConvertGuidToStringW( &guid, bufW, 38 );
    ok( err, "got %d\n", err );
    err = ConvertGuidToStringW( &guid, bufW, 39 );
    ok( !err, "got %d\n", err );
    ok( !wcscmp( bufW, L"{0000000A-000B-000C-0D00-0E0F00000000}" ), "got %s\n", debugstr_w( bufW ) );

    err = ConvertStringToGuidW( bufW, &guid2 );
    ok( !err, "got %d\n", err );
    ok( IsEqualGUID( &guid, &guid2 ), "guid mismatch\n" );

    err = ConvertStringToGuidW( L"foo", &guid2 );
    ok( err == ERROR_INVALID_PARAMETER, "got %d\n", err );
}

START_TEST(iphlpapi)
{

  loadIPHlpApi();
  if (hLibrary) {
    HANDLE thread;

    testWin98OnlyFunctions();
    testWinNT4Functions();

    /* run testGetXXXX in two threads at once to make sure we don't crash in that case */
    thread = CreateThread(NULL, 0, testWin98Functions, NULL, 0, NULL);
    testWin98Functions(NULL);
    WaitForSingleObject(thread, INFINITE);

    testWin2KFunctions();
    test_GetAdaptersAddresses();
    test_GetExtendedTcpTable();
    test_GetExtendedUdpTable();
    test_AllocateAndGetTcpExTableFromStack();
    test_CreateSortedAddressPairs();
    test_interface_identifier_conversion();
    test_GetIfEntry2();
    test_GetIfTable2();
    test_GetIfTable2Ex();
    test_GetUnicastIpAddressEntry();
    test_GetUnicastIpAddressTable();
    test_ConvertLengthToIpv4Mask();
    test_GetTcp6Table();
    test_GetUdp6Table();
    test_ParseNetworkString();
    test_NotifyUnicastIpAddressChange();
    test_ConvertGuidToString();
    freeIPHlpApi();
  }
}
