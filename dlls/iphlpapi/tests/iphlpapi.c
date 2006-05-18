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
#include "windef.h"
#include "winbase.h"
#include "iphlpapi.h"
#include "iprtrmib.h"
#include "wine/test.h"
#include <stdio.h>
#include <stdlib.h>

static HMODULE hLibrary = NULL;

typedef DWORD (WINAPI *GetNumberOfInterfacesFunc)(PDWORD);
typedef DWORD (WINAPI *GetIpAddrTableFunc)(PMIB_IPADDRTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetIfEntryFunc)(PMIB_IFROW);
typedef DWORD (WINAPI *GetFriendlyIfIndexFunc)(DWORD);
typedef DWORD (WINAPI *GetIfTableFunc)(PMIB_IFTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetIpForwardTableFunc)(PMIB_IPFORWARDTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetIpNetTableFunc)(PMIB_IPNETTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetInterfaceInfoFunc)(PIP_INTERFACE_INFO,PULONG);
typedef DWORD (WINAPI *GetAdaptersInfoFunc)(PIP_ADAPTER_INFO,PULONG);
typedef DWORD (WINAPI *GetNetworkParamsFunc)(PFIXED_INFO,PULONG);
typedef DWORD (WINAPI *GetIcmpStatisticsFunc)(PMIB_ICMP);
typedef DWORD (WINAPI *GetIpStatisticsFunc)(PMIB_IPSTATS);
typedef DWORD (WINAPI *GetTcpStatisticsFunc)(PMIB_TCPSTATS);
typedef DWORD (WINAPI *GetUdpStatisticsFunc)(PMIB_UDPSTATS);
typedef DWORD (WINAPI *GetTcpTableFunc)(PMIB_TCPTABLE,PDWORD,BOOL);
typedef DWORD (WINAPI *GetUdpTableFunc)(PMIB_UDPTABLE,PDWORD,BOOL);

static GetNumberOfInterfacesFunc gGetNumberOfInterfaces = NULL;
static GetIpAddrTableFunc gGetIpAddrTable = NULL;
static GetIfEntryFunc gGetIfEntry = NULL;
static GetFriendlyIfIndexFunc gGetFriendlyIfIndex = NULL;
static GetIfTableFunc gGetIfTable = NULL;
static GetIpForwardTableFunc gGetIpForwardTable = NULL;
static GetIpNetTableFunc gGetIpNetTable = NULL;
static GetInterfaceInfoFunc gGetInterfaceInfo = NULL;
static GetAdaptersInfoFunc gGetAdaptersInfo = NULL;
static GetNetworkParamsFunc gGetNetworkParams = NULL;
static GetIcmpStatisticsFunc gGetIcmpStatistics = NULL;
static GetIpStatisticsFunc gGetIpStatistics = NULL;
static GetTcpStatisticsFunc gGetTcpStatistics = NULL;
static GetUdpStatisticsFunc gGetUdpStatistics = NULL;
static GetTcpTableFunc gGetTcpTable = NULL;
static GetUdpTableFunc gGetUdpTable = NULL;

static void loadIPHlpApi(void)
{
  hLibrary = LoadLibraryA("iphlpapi.dll");
  if (hLibrary) {
    gGetNumberOfInterfaces = (GetNumberOfInterfacesFunc)GetProcAddress(
     hLibrary, "GetNumberOfInterfaces");
    gGetIpAddrTable = (GetIpAddrTableFunc)GetProcAddress(
     hLibrary, "GetIpAddrTable");
    gGetIfEntry = (GetIfEntryFunc)GetProcAddress(
     hLibrary, "GetIfEntry");
    gGetFriendlyIfIndex = (GetFriendlyIfIndexFunc)GetProcAddress(
     hLibrary, "GetFriendlyIfIndex");
    gGetIfTable = (GetIfTableFunc)GetProcAddress(
     hLibrary, "GetIfTable");
    gGetIpForwardTable = (GetIpForwardTableFunc)GetProcAddress(
     hLibrary, "GetIpForwardTable");
    gGetIpNetTable = (GetIpNetTableFunc)GetProcAddress(
     hLibrary, "GetIpNetTable");
    gGetInterfaceInfo = (GetInterfaceInfoFunc)GetProcAddress(
     hLibrary, "GetInterfaceInfo");
    gGetAdaptersInfo = (GetAdaptersInfoFunc)GetProcAddress(
     hLibrary, "GetAdaptersInfo");
    gGetNetworkParams = (GetNetworkParamsFunc)GetProcAddress(
     hLibrary, "GetNetworkParams");
    gGetIcmpStatistics = (GetIcmpStatisticsFunc)GetProcAddress(
     hLibrary, "GetIcmpStatistics");
    gGetIpStatistics = (GetIpStatisticsFunc)GetProcAddress(
     hLibrary, "GetIpStatistics");
    gGetTcpStatistics = (GetTcpStatisticsFunc)GetProcAddress(
     hLibrary, "GetTcpStatistics");
    gGetUdpStatistics = (GetUdpStatisticsFunc)GetProcAddress(
     hLibrary, "GetUdpStatistics");
    gGetTcpTable = (GetTcpTableFunc)GetProcAddress(
     hLibrary, "GetTcpTable");
    gGetUdpTable = (GetUdpTableFunc)GetProcAddress(
     hLibrary, "GetUdpTable");
  }
}

static void freeIPHlpApi(void)
{
  if (hLibrary) {
    gGetNumberOfInterfaces = NULL;
    gGetIpAddrTable = NULL;
    gGetIfEntry = NULL;
    gGetFriendlyIfIndex = NULL;
    gGetIfTable = NULL;
    gGetIpForwardTable = NULL;
    gGetIpNetTable = NULL;
    gGetInterfaceInfo = NULL;
    gGetAdaptersInfo = NULL;
    gGetNetworkParams = NULL;
    gGetIcmpStatistics = NULL;
    gGetIpStatistics = NULL;
    gGetTcpStatistics = NULL;
    gGetUdpStatistics = NULL;
    gGetTcpTable = NULL;
    gGetUdpTable = NULL;
    FreeLibrary(hLibrary);
    hLibrary = NULL;
  }
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
  if (gGetNumberOfInterfaces) {
    DWORD apiReturn = gGetNumberOfInterfaces(NULL), numInterfaces;

    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER, 
     "GetNumberOfInterfaces(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetNumberOfInterfaces(&numInterfaces);
    ok(apiReturn == NO_ERROR,
     "GetNumberOfInterfaces returned %ld, expected 0\n", apiReturn);
  }
}

static void testGetIfEntry(DWORD index)
{
  if (gGetIfEntry) {
    DWORD apiReturn;
    MIB_IFROW row;

    memset(&row, 0, sizeof(row));
    apiReturn = gGetIfEntry(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfEntry(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    row.dwIndex = -1; /* hope that's always bogus! */
    apiReturn = gGetIfEntry(&row);
    ok(apiReturn == ERROR_INVALID_DATA,
     "GetIfEntry(bogus row) returned %ld, expected ERROR_INVALID_DATA\n",
     apiReturn);
    row.dwIndex = index;
    apiReturn = gGetIfEntry(&row);
    ok(apiReturn == NO_ERROR, 
     "GetIfEntry returned %ld, expected NO_ERROR\n", apiReturn);
  }
}

static void testGetIpAddrTable(void)
{
  if (gGetIpAddrTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIpAddrTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpAddrTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpAddrTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpAddrTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPADDRTABLE buf = (PMIB_IPADDRTABLE)malloc(dwSize);

      apiReturn = gGetIpAddrTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIpAddrTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
       apiReturn);
      if (apiReturn == NO_ERROR && buf->dwNumEntries)
        testGetIfEntry(buf->table[0].dwIndex);
      free(buf);
    }
  }
}

static void testGetIfTable(void)
{
  if (gGetIfTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIfTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIfTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIfTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IFTABLE buf = (PMIB_IFTABLE)malloc(dwSize);

      apiReturn = gGetIfTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIfTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n\n",
       apiReturn);
      free(buf);
    }
  }
}

static void testGetIpForwardTable(void)
{
  if (gGetIpForwardTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIpForwardTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpForwardTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpForwardTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpForwardTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPFORWARDTABLE buf = (PMIB_IPFORWARDTABLE)malloc(dwSize);

      apiReturn = gGetIpForwardTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIpForwardTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
       apiReturn);
      free(buf);
    }
  }
}

static void testGetIpNetTable(void)
{
  if (gGetIpNetTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIpNetTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpNetTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpNetTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpNetTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_NO_DATA or ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_NO_DATA)
      ; /* empty ARP table's okay */
    else if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPNETTABLE buf = (PMIB_IPNETTABLE)malloc(dwSize);

      apiReturn = gGetIpNetTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIpNetTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
       apiReturn);
      free(buf);
    }
  }
}

static void testGetIcmpStatistics(void)
{
  if (gGetIcmpStatistics) {
    DWORD apiReturn;
    MIB_ICMP stats;

    apiReturn = gGetIcmpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIcmpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIcmpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
     "GetIcmpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
  }
}

static void testGetIpStatistics(void)
{
  if (gGetIpStatistics) {
    DWORD apiReturn;
    MIB_IPSTATS stats;

    apiReturn = gGetIpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
      "GetIpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
  }
}

static void testGetTcpStatistics(void)
{
  if (gGetTcpStatistics) {
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    apiReturn = gGetTcpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetTcpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetTcpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
      "GetTcpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
  }
}

static void testGetUdpStatistics(void)
{
  if (gGetUdpStatistics) {
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    apiReturn = gGetUdpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetUdpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetUdpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
     "GetUdpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
  }
}

static void testGetTcpTable(void)
{
  if (gGetTcpTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetTcpTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetTcpTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetTcpTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetTcpTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_TCPTABLE buf = (PMIB_TCPTABLE)malloc(dwSize);

      apiReturn = gGetTcpTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetTcpTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
       apiReturn);
      free(buf);
    }
  }
}

static void testGetUdpTable(void)
{
  if (gGetUdpTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetUdpTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetUdpTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetUdpTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetUdpTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn != ERROR_INSUFFICIENT_BUFFER) {
      PMIB_UDPTABLE buf = (PMIB_UDPTABLE)malloc(dwSize);

      apiReturn = gGetUdpTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetUdpTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
       apiReturn);
      free(buf);
    }
  }
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
SetTcpEntry
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
  testGetTcpTable();
  testGetUdpTable();
}

static void testGetInterfaceInfo(void)
{
  if (gGetInterfaceInfo) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = gGetInterfaceInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetInterfaceInfo returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetInterfaceInfo(NULL, &len);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetInterfaceInfo returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PIP_INTERFACE_INFO buf = (PIP_INTERFACE_INFO)malloc(len);

      apiReturn = gGetInterfaceInfo(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetInterfaceInfo(buf, &dwSize) returned %ld, expected NO_ERROR\n",
       apiReturn);
      free(buf);
    }
  }
}

static void testGetAdaptersInfo(void)
{
  if (gGetAdaptersInfo) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = gGetAdaptersInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetAdaptersInfo returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetAdaptersInfo(NULL, &len);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_BUFFER_OVERFLOW,
     "GetAdaptersInfo returned %ld, expected ERROR_NO_DATA or ERROR_BUFFER_OVERFLOW\n",
     apiReturn);
    if (apiReturn == ERROR_NO_DATA)
      ; /* no adapter's, that's okay */
    else if (apiReturn == ERROR_BUFFER_OVERFLOW) {
      PIP_ADAPTER_INFO buf = (PIP_ADAPTER_INFO)malloc(len);

      apiReturn = gGetAdaptersInfo(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetAdaptersInfo(buf, &dwSize) returned %ld, expected NO_ERROR\n",
       apiReturn);
      free(buf);
    }
  }
}

static void testGetNetworkParams(void)
{
  if (gGetNetworkParams) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = gGetNetworkParams(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED)
      return;
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetNetworkParams returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetNetworkParams(NULL, &len);
    ok(apiReturn == ERROR_BUFFER_OVERFLOW,
     "GetNetworkParams returned %ld, expected ERROR_BUFFER_OVERFLOW\n",
     apiReturn);
    if (apiReturn == ERROR_BUFFER_OVERFLOW) {
      PFIXED_INFO buf = (PFIXED_INFO)malloc(len);

      apiReturn = gGetNetworkParams(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetNetworkParams(buf, &dwSize) returned %ld, expected NO_ERROR\n",
       apiReturn);
      free(buf);
    }
  }
}

/*
still-to-be-tested 98-onward functions:
GetBestInterface
GetBestRoute
IpReleaseAddress
IpRenewAddress
*/
static void testWin98Functions(void)
{
  testGetInterfaceInfo();
  testGetAdaptersInfo();
  testGetNetworkParams();
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
GetPerAdapterInfo
NotifyAddrChange
NotifyRouteChange
SendARP
UnenableRouter
*/
static void testWin2KFunctions(void)
{
}

START_TEST(iphlpapi)
{

  loadIPHlpApi();
  if (hLibrary) {
    testWin98OnlyFunctions();
    testWinNT4Functions();
    testWin98Functions();
    testWin2KFunctions();
    freeIPHlpApi();
  }
}
