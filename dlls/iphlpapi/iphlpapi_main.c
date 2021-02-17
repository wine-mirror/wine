/*
 * iphlpapi dll implementation
 *
 * Copyright (C) 2003,2006 Juan Lang
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "winternl.h"
#include "ws2ipdef.h"
#include "windns.h"
#include "iphlpapi.h"
#include "ifenum.h"
#include "ipstats.h"
#include "ipifcons.h"
#include "fltdefs.h"
#include "ifdef.h"
#include "netioapi.h"
#include "tcpestats.h"
#include "ip2string.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

#ifndef INADDR_NONE
#define INADDR_NONE ~0UL
#endif

/******************************************************************
 *    AddIPAddress (IPHLPAPI.@)
 *
 * Add an IP address to an adapter.
 *
 * PARAMS
 *  Address     [In]  IP address to add to the adapter
 *  IpMask      [In]  subnet mask for the IP address
 *  IfIndex     [In]  adapter index to add the address
 *  NTEContext  [Out] Net Table Entry (NTE) context for the IP address
 *  NTEInstance [Out] NTE instance for the IP address
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub. Currently returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI AddIPAddress(IPAddr Address, IPMask IpMask, DWORD IfIndex, PULONG NTEContext, PULONG NTEInstance)
{
  FIXME(":stub\n");
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    AllocateAndGetIfTableFromStack (IPHLPAPI.@)
 *
 * Get table of local interfaces.
 * Like GetIfTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppIfTable [Out] pointer into which the MIB_IFTABLE is
 *                  allocated and returned.
 *  bOrder    [In]  whether to sort the table
 *  heap      [In]  heap from which the table is allocated
 *  flags     [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppIfTable is NULL, whatever
 *  GetIfTable() returns otherwise.
 */
DWORD WINAPI AllocateAndGetIfTableFromStack(PMIB_IFTABLE *ppIfTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIfTable %p, bOrder %d, heap %p, flags 0x%08x\n", ppIfTable,
        bOrder, heap, flags);
  if (!ppIfTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIfTable(*ppIfTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIfTable = HeapAlloc(heap, flags, dwSize);
      ret = GetIfTable(*ppIfTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}


static int IpAddrTableNumericSorter(const void *a, const void *b)
{
  int ret = 0;

  if (a && b)
    ret = ((const MIB_IPADDRROW*)a)->dwAddr - ((const MIB_IPADDRROW*)b)->dwAddr;
  return ret;
}

static int IpAddrTableLoopbackSorter(const void *a, const void *b)
{
  const MIB_IPADDRROW *left = a, *right = b;
  int ret = 0;

  if (isIfIndexLoopback(left->dwIndex))
    ret = 1;
  else if (isIfIndexLoopback(right->dwIndex))
    ret = -1;

  return ret;
}

/******************************************************************
 *    AllocateAndGetIpAddrTableFromStack (IPHLPAPI.@)
 *
 * Get interface-to-IP address mapping table. 
 * Like GetIpAddrTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppIpAddrTable [Out] pointer into which the MIB_IPADDRTABLE is
 *                      allocated and returned.
 *  bOrder        [In]  whether to sort the table
 *  heap          [In]  heap from which the table is allocated
 *  flags         [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppIpAddrTable is NULL, other error codes on
 *  failure, NO_ERROR on success.
 */
DWORD WINAPI AllocateAndGetIpAddrTableFromStack(PMIB_IPADDRTABLE *ppIpAddrTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpAddrTable %p, bOrder %d, heap %p, flags 0x%08x\n",
   ppIpAddrTable, bOrder, heap, flags);
  ret = getIPAddrTable(ppIpAddrTable, heap, flags);
  if (!ret && bOrder)
    qsort((*ppIpAddrTable)->table, (*ppIpAddrTable)->dwNumEntries,
     sizeof(MIB_IPADDRROW), IpAddrTableNumericSorter);
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    CancelIPChangeNotify (IPHLPAPI.@)
 *
 * Cancel a previous notification created by NotifyAddrChange or
 * NotifyRouteChange.
 *
 * PARAMS
 *  overlapped [In]  overlapped structure that notifies the caller
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * FIXME
 *  Stub, returns FALSE.
 */
BOOL WINAPI CancelIPChangeNotify(LPOVERLAPPED overlapped)
{
  FIXME("(overlapped %p): stub\n", overlapped);
  return FALSE;
}


/******************************************************************
 *    CancelMibChangeNotify2 (IPHLPAPI.@)
 */
DWORD WINAPI CancelMibChangeNotify2(HANDLE handle)
{
    FIXME("(handle %p): stub\n", handle);
    return NO_ERROR;
}


/******************************************************************
 *    CreateIpForwardEntry (IPHLPAPI.@)
 *
 * Create a route in the local computer's IP table.
 *
 * PARAMS
 *  pRoute [In] new route information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, always returns NO_ERROR.
 */
DWORD WINAPI CreateIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
  FIXME("(pRoute %p): stub\n", pRoute);
  /* could use SIOCADDRT, not sure I want to */
  return 0;
}


/******************************************************************
 *    CreateIpNetEntry (IPHLPAPI.@)
 *
 * Create entry in the ARP table.
 *
 * PARAMS
 *  pArpEntry [In] new ARP entry
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, always returns NO_ERROR.
 */
DWORD WINAPI CreateIpNetEntry(PMIB_IPNETROW pArpEntry)
{
  FIXME("(pArpEntry %p)\n", pArpEntry);
  /* could use SIOCSARP on systems that support it, not sure I want to */
  return 0;
}


/******************************************************************
 *    CreateProxyArpEntry (IPHLPAPI.@)
 *
 * Create a Proxy ARP (PARP) entry for an IP address.
 *
 * PARAMS
 *  dwAddress [In] IP address for which this computer acts as a proxy. 
 *  dwMask    [In] subnet mask for dwAddress
 *  dwIfIndex [In] interface index
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI CreateProxyArpEntry(DWORD dwAddress, DWORD dwMask, DWORD dwIfIndex)
{
  FIXME("(dwAddress 0x%08x, dwMask 0x%08x, dwIfIndex 0x%08x): stub\n",
   dwAddress, dwMask, dwIfIndex);
  return ERROR_NOT_SUPPORTED;
}

static char *debugstr_ipv6(const struct WS_sockaddr_in6 *sin, char *buf)
{
    const IN6_ADDR *addr = &sin->sin6_addr;
    char *p = buf;
    int i;
    BOOL in_zero = FALSE;

    for (i = 0; i < 7; i++)
    {
        if (!addr->u.Word[i])
        {
            if (i == 0)
                *p++ = ':';
            if (!in_zero)
            {
                *p++ = ':';
                in_zero = TRUE;
            }
        }
        else
        {
            p += sprintf(p, "%x:", ntohs(addr->u.Word[i]));
            in_zero = FALSE;
        }
    }
    sprintf(p, "%x", ntohs(addr->u.Word[7]));
    return buf;
}

static BOOL map_address_6to4( const SOCKADDR_IN6 *addr6, SOCKADDR_IN *addr4 )
{
    ULONG i;

    if (addr6->sin6_family != WS_AF_INET6) return FALSE;

    for (i = 0; i < 5; i++)
        if (addr6->sin6_addr.u.Word[i]) return FALSE;

    if (addr6->sin6_addr.u.Word[5] != 0xffff) return FALSE;

    addr4->sin_family = WS_AF_INET;
    addr4->sin_port   = addr6->sin6_port;
    addr4->sin_addr.S_un.S_addr = addr6->sin6_addr.u.Word[6] << 16 | addr6->sin6_addr.u.Word[7];
    memset( &addr4->sin_zero, 0, sizeof(addr4->sin_zero) );

    return TRUE;
}

static BOOL find_src_address( MIB_IPADDRTABLE *table, const SOCKADDR_IN *dst, SOCKADDR_IN6 *src )
{
    MIB_IPFORWARDROW row;
    DWORD i, j;

    if (GetBestRoute( dst->sin_addr.S_un.S_addr, 0, &row )) return FALSE;

    for (i = 0; i < table->dwNumEntries; i++)
    {
        /* take the first address */
        if (table->table[i].dwIndex == row.dwForwardIfIndex)
        {
            src->sin6_family   = WS_AF_INET6;
            src->sin6_port     = 0;
            src->sin6_flowinfo = 0;
            for (j = 0; j < 5; j++) src->sin6_addr.u.Word[j] = 0;
            src->sin6_addr.u.Word[5] = 0xffff;
            src->sin6_addr.u.Word[6] = table->table[i].dwAddr & 0xffff;
            src->sin6_addr.u.Word[7] = table->table[i].dwAddr >> 16;
            return TRUE;
        }
    }

    return FALSE;
}

/******************************************************************
 *    CreateSortedAddressPairs (IPHLPAPI.@)
 */
DWORD WINAPI CreateSortedAddressPairs( const PSOCKADDR_IN6 src_list, DWORD src_count,
                                       const PSOCKADDR_IN6 dst_list, DWORD dst_count,
                                       DWORD options, PSOCKADDR_IN6_PAIR *pair_list,
                                       DWORD *pair_count )
{
    DWORD i, size, ret;
    SOCKADDR_IN6_PAIR *pairs;
    SOCKADDR_IN6 *ptr;
    SOCKADDR_IN addr4;
    MIB_IPADDRTABLE *table;

    FIXME( "(src_list %p src_count %u dst_list %p dst_count %u options %x pair_list %p pair_count %p): stub\n",
           src_list, src_count, dst_list, dst_count, options, pair_list, pair_count );

    if (src_list || src_count || !dst_list || !pair_list || !pair_count || dst_count > 500)
        return ERROR_INVALID_PARAMETER;

    for (i = 0; i < dst_count; i++)
    {
        if (!map_address_6to4( &dst_list[i], &addr4 ))
        {
            FIXME("only mapped IPv4 addresses are supported\n");
            return ERROR_NOT_SUPPORTED;
        }
    }

    size = dst_count * sizeof(*pairs);
    size += dst_count * sizeof(SOCKADDR_IN6) * 2; /* source address + destination address */
    if (!(pairs = HeapAlloc( GetProcessHeap(), 0, size ))) return ERROR_NOT_ENOUGH_MEMORY;
    ptr = (SOCKADDR_IN6 *)&pairs[dst_count];

    if ((ret = getIPAddrTable( &table, GetProcessHeap(), 0 )))
    {
        HeapFree( GetProcessHeap(), 0, pairs );
        return ret;
    }

    for (i = 0; i < dst_count; i++)
    {
        pairs[i].SourceAddress = ptr++;
        if (!map_address_6to4( &dst_list[i], &addr4 ) ||
            !find_src_address( table, &addr4, pairs[i].SourceAddress ))
        {
            char buf[46];
            FIXME( "source address for %s not found\n", debugstr_ipv6(&dst_list[i], buf) );
            memset( pairs[i].SourceAddress, 0, sizeof(*pairs[i].SourceAddress) );
            pairs[i].SourceAddress->sin6_family = WS_AF_INET6;
        }

        pairs[i].DestinationAddress = ptr++;
        memcpy( pairs[i].DestinationAddress, &dst_list[i], sizeof(*pairs[i].DestinationAddress) );
    }
    *pair_list = pairs;
    *pair_count = dst_count;

    HeapFree( GetProcessHeap(), 0, table );
    return NO_ERROR;
}


/******************************************************************
 *    DeleteIPAddress (IPHLPAPI.@)
 *
 * Delete an IP address added with AddIPAddress().
 *
 * PARAMS
 *  NTEContext [In] NTE context from AddIPAddress();
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI DeleteIPAddress(ULONG NTEContext)
{
  FIXME("(NTEContext %d): stub\n", NTEContext);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    DeleteIpForwardEntry (IPHLPAPI.@)
 *
 * Delete a route.
 *
 * PARAMS
 *  pRoute [In] route to delete
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI DeleteIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
  FIXME("(pRoute %p): stub\n", pRoute);
  /* could use SIOCDELRT, not sure I want to */
  return 0;
}


/******************************************************************
 *    DeleteIpNetEntry (IPHLPAPI.@)
 *
 * Delete an ARP entry.
 *
 * PARAMS
 *  pArpEntry [In] ARP entry to delete
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI DeleteIpNetEntry(PMIB_IPNETROW pArpEntry)
{
  FIXME("(pArpEntry %p): stub\n", pArpEntry);
  /* could use SIOCDARP on systems that support it, not sure I want to */
  return 0;
}


/******************************************************************
 *    DeleteProxyArpEntry (IPHLPAPI.@)
 *
 * Delete a Proxy ARP entry.
 *
 * PARAMS
 *  dwAddress [In] IP address for which this computer acts as a proxy. 
 *  dwMask    [In] subnet mask for dwAddress
 *  dwIfIndex [In] interface index
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI DeleteProxyArpEntry(DWORD dwAddress, DWORD dwMask, DWORD dwIfIndex)
{
  FIXME("(dwAddress 0x%08x, dwMask 0x%08x, dwIfIndex 0x%08x): stub\n",
   dwAddress, dwMask, dwIfIndex);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    EnableRouter (IPHLPAPI.@)
 *
 * Turn on ip forwarding.
 *
 * PARAMS
 *  pHandle     [In/Out]
 *  pOverlapped [In/Out] hEvent member should contain a valid handle.
 *
 * RETURNS
 *  Success: ERROR_IO_PENDING
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI EnableRouter(HANDLE * pHandle, OVERLAPPED * pOverlapped)
{
  FIXME("(pHandle %p, pOverlapped %p): stub\n", pHandle, pOverlapped);
  /* could echo "1" > /proc/net/sys/net/ipv4/ip_forward, not sure I want to
     could map EACCESS to ERROR_ACCESS_DENIED, I suppose
   */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    FlushIpNetTable (IPHLPAPI.@)
 *
 * Delete all ARP entries of an interface
 *
 * PARAMS
 *  dwIfIndex [In] interface index
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI FlushIpNetTable(DWORD dwIfIndex)
{
  FIXME("(dwIfIndex 0x%08x): stub\n", dwIfIndex);
  /* this flushes the arp cache of the given index */
  return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    FreeMibTable (IPHLPAPI.@)
 *
 * Free buffer allocated by network functions
 *
 * PARAMS
 *  ptr     [In] pointer to the buffer to free
 *
 */
void WINAPI FreeMibTable(void *ptr)
{
  TRACE("(%p)\n", ptr);
  HeapFree(GetProcessHeap(), 0, ptr);
}

/******************************************************************
 *    GetAdapterIndex (IPHLPAPI.@)
 *
 * Get interface index from its name.
 *
 * PARAMS
 *  AdapterName [In]  unicode string with the adapter name
 *  IfIndex     [Out] returns found interface index
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetAdapterIndex(LPWSTR AdapterName, PULONG IfIndex)
{
  char adapterName[MAX_ADAPTER_NAME];
  unsigned int i;
  DWORD ret;

  TRACE("(AdapterName %p, IfIndex %p)\n", AdapterName, IfIndex);
  /* The adapter name is guaranteed not to have any unicode characters, so
   * this translation is never lossy */
  for (i = 0; i < sizeof(adapterName) - 1 && AdapterName[i]; i++)
    adapterName[i] = (char)AdapterName[i];
  adapterName[i] = '\0';
  ret = getInterfaceIndexByName(adapterName, IfIndex);
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetAdaptersInfo (IPHLPAPI.@)
 *
 * Get information about adapters.
 *
 * PARAMS
 *  pAdapterInfo [Out] buffer for adapter infos
 *  pOutBufLen   [In]  length of output buffer
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen)
{
  DWORD ret;

  TRACE("pAdapterInfo %p, pOutBufLen %p\n", pAdapterInfo, pOutBufLen);
  if (!pOutBufLen)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numNonLoopbackInterfaces = get_interface_indices( TRUE, NULL );

    if (numNonLoopbackInterfaces > 0) {
      DWORD numIPAddresses = getNumIPAddresses();
      ULONG size;

      /* This may slightly overestimate the amount of space needed, because
       * the IP addresses include the loopback address, but it's easier
       * to make sure there's more than enough space than to make sure there's
       * precisely enough space.
       */
      size = sizeof(IP_ADAPTER_INFO) * numNonLoopbackInterfaces;
      size += numIPAddresses  * sizeof(IP_ADDR_STRING); 
      if (!pAdapterInfo || *pOutBufLen < size) {
        *pOutBufLen = size;
        ret = ERROR_BUFFER_OVERFLOW;
      }
      else {
        InterfaceIndexTable *table = NULL;
        PMIB_IPADDRTABLE ipAddrTable = NULL;
        PMIB_IPFORWARDTABLE routeTable = NULL;

        ret = getIPAddrTable(&ipAddrTable, GetProcessHeap(), 0);
        if (!ret)
          ret = AllocateAndGetIpForwardTableFromStack(&routeTable, FALSE, GetProcessHeap(), 0);
        if (!ret)
          get_interface_indices( TRUE, &table );
        if (table) {
          size = sizeof(IP_ADAPTER_INFO) * table->numIndexes;
          size += ipAddrTable->dwNumEntries * sizeof(IP_ADDR_STRING); 
          if (*pOutBufLen < size) {
            *pOutBufLen = size;
            ret = ERROR_INSUFFICIENT_BUFFER;
          }
          else {
            DWORD ndx;
            HKEY hKey;
            BOOL winsEnabled = FALSE;
            IP_ADDRESS_STRING primaryWINS, secondaryWINS;
            PIP_ADDR_STRING nextIPAddr = (PIP_ADDR_STRING)((LPBYTE)pAdapterInfo
             + numNonLoopbackInterfaces * sizeof(IP_ADAPTER_INFO));

            memset(pAdapterInfo, 0, size);
            /* @@ Wine registry key: HKCU\Software\Wine\Network */
            if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Network",
             &hKey) == ERROR_SUCCESS) {
              DWORD size = sizeof(primaryWINS.String);
              unsigned long addr;

              RegQueryValueExA(hKey, "WinsServer", NULL, NULL,
               (LPBYTE)primaryWINS.String, &size);
              addr = inet_addr(primaryWINS.String);
              if (addr != INADDR_NONE && addr != INADDR_ANY)
                winsEnabled = TRUE;
              size = sizeof(secondaryWINS.String);
              RegQueryValueExA(hKey, "BackupWinsServer", NULL, NULL,
               (LPBYTE)secondaryWINS.String, &size);
              addr = inet_addr(secondaryWINS.String);
              if (addr != INADDR_NONE && addr != INADDR_ANY)
                winsEnabled = TRUE;
              RegCloseKey(hKey);
            }
            for (ndx = 0; ndx < table->numIndexes; ndx++) {
              PIP_ADAPTER_INFO ptr = &pAdapterInfo[ndx];
              DWORD i;
              PIP_ADDR_STRING currentIPAddr = &ptr->IpAddressList;
              BOOL firstIPAddr = TRUE;

              /* on Win98 this is left empty, but whatever */
              getInterfaceNameByIndex(table->indexes[ndx], ptr->AdapterName);
              getInterfaceNameByIndex(table->indexes[ndx], ptr->Description);
              ptr->AddressLength = sizeof(ptr->Address);
              getInterfacePhysicalByIndex(table->indexes[ndx],
               &ptr->AddressLength, ptr->Address, &ptr->Type);
              ptr->Index = table->indexes[ndx];
              for (i = 0; i < ipAddrTable->dwNumEntries; i++) {
                if (ipAddrTable->table[i].dwIndex == ptr->Index) {
                  if (firstIPAddr) {
                    RtlIpv4AddressToStringA((IN_ADDR *)&ipAddrTable->table[i].dwAddr,
                     ptr->IpAddressList.IpAddress.String);
                    RtlIpv4AddressToStringA((IN_ADDR *)&ipAddrTable->table[i].dwMask,
                     ptr->IpAddressList.IpMask.String);
                    firstIPAddr = FALSE;
                  }
                  else {
                    currentIPAddr->Next = nextIPAddr;
                    currentIPAddr = nextIPAddr;
                    RtlIpv4AddressToStringA((IN_ADDR *)&ipAddrTable->table[i].dwAddr,
                     currentIPAddr->IpAddress.String);
                    RtlIpv4AddressToStringA((IN_ADDR *)&ipAddrTable->table[i].dwMask,
                     currentIPAddr->IpMask.String);
                    nextIPAddr++;
                  }
                }
              }
              /* If no IP was found it probably means that the interface is not
               * configured. In this case we have to return a zeroed IP and mask. */
              if (firstIPAddr) {
                strcpy(ptr->IpAddressList.IpAddress.String, "0.0.0.0");
                strcpy(ptr->IpAddressList.IpMask.String, "0.0.0.0");
              }
              /* Find first router through this interface, which we'll assume
               * is the default gateway for this adapter */
              strcpy(ptr->GatewayList.IpAddress.String, "0.0.0.0");
              strcpy(ptr->GatewayList.IpMask.String, "255.255.255.255");
              for (i = 0; i < routeTable->dwNumEntries; i++)
                if (routeTable->table[i].dwForwardIfIndex == ptr->Index
                 && routeTable->table[i].u1.ForwardType ==
                 MIB_IPROUTE_TYPE_INDIRECT)
                {
                  RtlIpv4AddressToStringA((IN_ADDR *)&routeTable->table[i].dwForwardNextHop,
                   ptr->GatewayList.IpAddress.String);
                  RtlIpv4AddressToStringA((IN_ADDR *)&routeTable->table[i].dwForwardMask,
                   ptr->GatewayList.IpMask.String);
                }
              if (winsEnabled) {
                ptr->HaveWins = TRUE;
                memcpy(ptr->PrimaryWinsServer.IpAddress.String,
                 primaryWINS.String, sizeof(primaryWINS.String));
                memcpy(ptr->SecondaryWinsServer.IpAddress.String,
                 secondaryWINS.String, sizeof(secondaryWINS.String));
              }
              if (ndx < table->numIndexes - 1)
                ptr->Next = &pAdapterInfo[ndx + 1];
              else
                ptr->Next = NULL;

              ptr->DhcpEnabled = TRUE;
            }
            ret = NO_ERROR;
          }
          HeapFree(GetProcessHeap(), 0, table);
        }
        else
          ret = ERROR_OUTOFMEMORY;
        HeapFree(GetProcessHeap(), 0, routeTable);
        HeapFree(GetProcessHeap(), 0, ipAddrTable);
      }
    }
    else
      ret = ERROR_NO_DATA;
  }
  TRACE("returning %d\n", ret);
  return ret;
}

static DWORD typeFromMibType(DWORD mib_type)
{
    switch (mib_type)
    {
    case MIB_IF_TYPE_ETHERNET:  return IF_TYPE_ETHERNET_CSMACD;
    case MIB_IF_TYPE_TOKENRING: return IF_TYPE_ISO88025_TOKENRING;
    case MIB_IF_TYPE_PPP:       return IF_TYPE_PPP;
    case MIB_IF_TYPE_LOOPBACK:  return IF_TYPE_SOFTWARE_LOOPBACK;
    default:                    return IF_TYPE_OTHER;
    }
}

static NET_IF_CONNECTION_TYPE connectionTypeFromMibType(DWORD mib_type)
{
    switch (mib_type)
    {
    case MIB_IF_TYPE_PPP:       return NET_IF_CONNECTION_DEMAND;
    case MIB_IF_TYPE_SLIP:      return NET_IF_CONNECTION_DEMAND;
    default:                    return NET_IF_CONNECTION_DEDICATED;
    }
}

static ULONG v4addressesFromIndex(IF_INDEX index, DWORD **addrs, ULONG *num_addrs, DWORD **masks)
{
    ULONG ret, i, j;
    MIB_IPADDRTABLE *at;

    *num_addrs = 0;
    if ((ret = getIPAddrTable(&at, GetProcessHeap(), 0))) return ret;
    for (i = 0; i < at->dwNumEntries; i++)
    {
        if (at->table[i].dwIndex == index) (*num_addrs)++;
    }
    if (!(*addrs = HeapAlloc(GetProcessHeap(), 0, *num_addrs * sizeof(DWORD))))
    {
        HeapFree(GetProcessHeap(), 0, at);
        return ERROR_OUTOFMEMORY;
    }
    if (!(*masks = HeapAlloc(GetProcessHeap(), 0, *num_addrs * sizeof(DWORD))))
    {
        HeapFree(GetProcessHeap(), 0, *addrs);
        HeapFree(GetProcessHeap(), 0, at);
        return ERROR_OUTOFMEMORY;
    }
    for (i = 0, j = 0; i < at->dwNumEntries; i++)
    {
        if (at->table[i].dwIndex == index)
        {
            (*addrs)[j] = at->table[i].dwAddr;
            (*masks)[j] = at->table[i].dwMask;
            j++;
        }
    }
    HeapFree(GetProcessHeap(), 0, at);
    return ERROR_SUCCESS;
}

static char *debugstr_ipv4(const in_addr_t *in_addr, char *buf)
{
    const BYTE *addrp;
    char *p = buf;

    for (addrp = (const BYTE *)in_addr;
     addrp - (const BYTE *)in_addr < sizeof(*in_addr);
     addrp++)
    {
        if (addrp == (const BYTE *)in_addr + sizeof(*in_addr) - 1)
            sprintf(p, "%d", *addrp);
        else
            p += sprintf(p, "%d.", *addrp);
    }
    return buf;
}

static ULONG count_v4_gateways(DWORD index, PMIB_IPFORWARDTABLE routeTable)
{
    DWORD i, num_gateways = 0;

    for (i = 0; i < routeTable->dwNumEntries; i++)
    {
        if (routeTable->table[i].dwForwardIfIndex == index &&
            routeTable->table[i].u1.ForwardType == MIB_IPROUTE_TYPE_INDIRECT)
            num_gateways++;
    }
    return num_gateways;
}

static DWORD mask_v4_to_prefix(DWORD m)
{
#ifdef HAVE___BUILTIN_POPCOUNT
    return __builtin_popcount(m);
#else
    m -= m >> 1 & 0x55555555;
    m = (m & 0x33333333) + (m >> 2 & 0x33333333);
    return ((m + (m >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

static DWORD mask_v6_to_prefix(SOCKET_ADDRESS *m)
{
    const IN6_ADDR *mask = &((struct WS_sockaddr_in6 *)m->lpSockaddr)->sin6_addr;
    DWORD ret = 0, i;

    for (i = 0; i < 8; i++)
        ret += mask_v4_to_prefix(mask->u.Word[i]);
    return ret;
}

static PMIB_IPFORWARDROW findIPv4Gateway(DWORD index,
                                         PMIB_IPFORWARDTABLE routeTable)
{
    DWORD i;
    PMIB_IPFORWARDROW row = NULL;

    for (i = 0; !row && i < routeTable->dwNumEntries; i++)
    {
        if (routeTable->table[i].dwForwardIfIndex == index &&
            routeTable->table[i].u1.ForwardType == MIB_IPROUTE_TYPE_INDIRECT)
            row = &routeTable->table[i];
    }
    return row;
}

static void fill_unicast_addr_data(IP_ADAPTER_ADDRESSES *aa, IP_ADAPTER_UNICAST_ADDRESS *ua)
{
    /* Actually this information should be read somewhere from the system
     * but it doesn't matter much for the bugs found so far.
     * This information is required for DirectPlay8 games. */
    if (aa->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
    {
        ua->PrefixOrigin = IpPrefixOriginDhcp;
        ua->SuffixOrigin = IpSuffixOriginDhcp;
    }
    else
    {
        ua->PrefixOrigin = IpPrefixOriginManual;
        ua->SuffixOrigin = IpSuffixOriginManual;
    }

    /* The address is not duplicated in the network */
    ua->DadState = IpDadStatePreferred;

    /* Some address life time values, required even for non-dhcp addresses */
    ua->ValidLifetime = 60000;
    ua->PreferredLifetime = 60000;
    ua->LeaseLifetime = 60000;
}

static ULONG adapterAddressesFromIndex(ULONG family, ULONG flags, IF_INDEX index,
                                       IP_ADAPTER_ADDRESSES *aa, ULONG *size)
{
    ULONG ret = ERROR_SUCCESS, i, j, num_v4addrs = 0, num_v4_gateways = 0, num_v6addrs = 0, total_size;
    DWORD *v4addrs = NULL, *v4masks = NULL;
    SOCKET_ADDRESS *v6addrs = NULL, *v6masks = NULL;
    PMIB_IPFORWARDTABLE routeTable = NULL;
    BOOL output_gateways;

    if ((flags & GAA_FLAG_INCLUDE_ALL_GATEWAYS) || !(flags & GAA_FLAG_SKIP_UNICAST))
    {
        ret = AllocateAndGetIpForwardTableFromStack(&routeTable, FALSE, GetProcessHeap(), 0);
        if (ret) return ret;
        num_v4_gateways = count_v4_gateways(index, routeTable);
    }
    output_gateways = (flags & GAA_FLAG_INCLUDE_ALL_GATEWAYS) && (family == WS_AF_INET || family == WS_AF_UNSPEC);

    if (family == WS_AF_INET)
    {
        ret = v4addressesFromIndex(index, &v4addrs, &num_v4addrs, &v4masks);
    }
    else if (family == WS_AF_INET6)
    {
        ret = v6addressesFromIndex(index, &v6addrs, &num_v6addrs, &v6masks);
    }
    else if (family == WS_AF_UNSPEC)
    {
        ret = v4addressesFromIndex(index, &v4addrs, &num_v4addrs, &v4masks);
        if (!ret) ret = v6addressesFromIndex(index, &v6addrs, &num_v6addrs, &v6masks);
    }
    else
    {
        FIXME("address family %u unsupported\n", family);
        ret = ERROR_NO_DATA;
    }
    if (ret)
    {
        HeapFree(GetProcessHeap(), 0, v4addrs);
        HeapFree(GetProcessHeap(), 0, v4masks);
        HeapFree(GetProcessHeap(), 0, v6addrs);
        HeapFree(GetProcessHeap(), 0, v6masks);
        HeapFree(GetProcessHeap(), 0, routeTable);
        return ret;
    }

    total_size = sizeof(IP_ADAPTER_ADDRESSES);
    total_size += 39; /* "{00000000-0000-0000-0000-000000000000}" */
    total_size += IF_NAMESIZE * sizeof(WCHAR);
    if (!(flags & GAA_FLAG_SKIP_FRIENDLY_NAME))
        total_size += IF_NAMESIZE * sizeof(WCHAR);
    if (flags & GAA_FLAG_INCLUDE_PREFIX)
    {
        total_size += sizeof(IP_ADAPTER_PREFIX) * num_v4addrs;
        total_size += sizeof(IP_ADAPTER_PREFIX) * num_v6addrs;
        total_size += sizeof(struct sockaddr_in) * num_v4addrs;
        for (i = 0; i < num_v6addrs; i++)
            total_size += v6masks[i].iSockaddrLength;
    }
    total_size += sizeof(IP_ADAPTER_UNICAST_ADDRESS) * num_v4addrs;
    total_size += sizeof(struct sockaddr_in) * num_v4addrs;
    if (output_gateways)
        total_size += (sizeof(IP_ADAPTER_GATEWAY_ADDRESS) + sizeof(SOCKADDR_IN)) * num_v4_gateways;
    total_size += sizeof(IP_ADAPTER_UNICAST_ADDRESS) * num_v6addrs;
    total_size += sizeof(SOCKET_ADDRESS) * num_v6addrs;
    for (i = 0; i < num_v6addrs; i++)
        total_size += v6addrs[i].iSockaddrLength;

    if (aa && *size >= total_size)
    {
        char name[IF_NAMESIZE], *ptr = (char *)aa + sizeof(IP_ADAPTER_ADDRESSES), *src;
        WCHAR *dst;
        DWORD buflen, type;
        INTERNAL_IF_OPER_STATUS status;
        NET_LUID luid;
        GUID guid;

        memset(aa, 0, sizeof(IP_ADAPTER_ADDRESSES));
        aa->u.s.Length  = sizeof(IP_ADAPTER_ADDRESSES);
        aa->u.s.IfIndex = index;

        ConvertInterfaceIndexToLuid(index, &luid);
        ConvertInterfaceLuidToGuid(&luid, &guid);
        sprintf(ptr, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
                guid.Data4[6], guid.Data4[7]);
        aa->AdapterName = ptr;
        ptr += 39;

        getInterfaceNameByIndex(index, name);
        if (!(flags & GAA_FLAG_SKIP_FRIENDLY_NAME))
        {
            aa->FriendlyName = (WCHAR *)ptr;
            for (src = name, dst = (WCHAR *)ptr; *src; src++, dst++)
                *dst = *src;
            *dst++ = 0;
            ptr = (char *)dst;
        }
        aa->Description = (WCHAR *)ptr;
        for (src = name, dst = (WCHAR *)ptr; *src; src++, dst++)
            *dst = *src;
        *dst++ = 0;
        ptr = (char *)dst;

        TRACE("%s: %d IPv4 addresses, %d IPv6 addresses:\n", name, num_v4addrs,
              num_v6addrs);

        buflen = MAX_INTERFACE_PHYSADDR;
        getInterfacePhysicalByIndex(index, &buflen, aa->PhysicalAddress, &type);
        aa->PhysicalAddressLength = buflen;
        aa->IfType = typeFromMibType(type);
        aa->ConnectionType = connectionTypeFromMibType(type);
        aa->Luid.Info.NetLuidIndex = index;
        aa->Luid.Info.IfType = aa->IfType;

        if (output_gateways && num_v4_gateways)
        {
            PMIB_IPFORWARDROW adapterRow;

            if ((adapterRow = findIPv4Gateway(index, routeTable)))
            {
                PIP_ADAPTER_GATEWAY_ADDRESS gw;
                PSOCKADDR_IN sin;

                gw = (PIP_ADAPTER_GATEWAY_ADDRESS)ptr;
                aa->FirstGatewayAddress = gw;

                gw->u.s.Length = sizeof(IP_ADAPTER_GATEWAY_ADDRESS);
                ptr += sizeof(IP_ADAPTER_GATEWAY_ADDRESS);
                sin = (PSOCKADDR_IN)ptr;
                sin->sin_family = WS_AF_INET;
                sin->sin_port = 0;
                memcpy(&sin->sin_addr, &adapterRow->dwForwardNextHop,
                       sizeof(DWORD));
                gw->Address.lpSockaddr = (LPSOCKADDR)sin;
                gw->Address.iSockaddrLength = sizeof(SOCKADDR_IN);
                gw->Next = NULL;
                ptr += sizeof(SOCKADDR_IN);
            }
        }
        if (num_v4addrs && !(flags & GAA_FLAG_SKIP_UNICAST))
        {
            IP_ADAPTER_UNICAST_ADDRESS *ua;
            struct WS_sockaddr_in *sa;
            aa->u1.s1.Ipv4Enabled = TRUE;
            ua = aa->FirstUnicastAddress = (IP_ADAPTER_UNICAST_ADDRESS *)ptr;
            for (i = 0; i < num_v4addrs; i++)
            {
                char addr_buf[16];

                memset(ua, 0, sizeof(IP_ADAPTER_UNICAST_ADDRESS));
                ua->u.s.Length              = sizeof(IP_ADAPTER_UNICAST_ADDRESS);
                ua->Address.iSockaddrLength = sizeof(struct sockaddr_in);
                ua->Address.lpSockaddr      = (SOCKADDR *)((char *)ua + ua->u.s.Length);
                if (num_v4_gateways)
                    ua->u.s.Flags |= IP_ADAPTER_ADDRESS_DNS_ELIGIBLE;

                sa = (struct WS_sockaddr_in *)ua->Address.lpSockaddr;
                sa->sin_family           = WS_AF_INET;
                sa->sin_addr.S_un.S_addr = v4addrs[i];
                sa->sin_port             = 0;
                TRACE("IPv4 %d/%d: %s\n", i + 1, num_v4addrs,
                      debugstr_ipv4(&sa->sin_addr.S_un.S_addr, addr_buf));
                fill_unicast_addr_data(aa, ua);

                ua->OnLinkPrefixLength = mask_v4_to_prefix(v4masks[i]);

                ptr += ua->u.s.Length + ua->Address.iSockaddrLength;
                if (i < num_v4addrs - 1)
                {
                    ua->Next = (IP_ADAPTER_UNICAST_ADDRESS *)ptr;
                    ua = ua->Next;
                }
            }
        }
        if (num_v6addrs && !(flags & GAA_FLAG_SKIP_UNICAST))
        {
            IP_ADAPTER_UNICAST_ADDRESS *ua;
            struct WS_sockaddr_in6 *sa;

            aa->u1.s1.Ipv6Enabled = TRUE;
            if (aa->FirstUnicastAddress)
            {
                for (ua = aa->FirstUnicastAddress; ua->Next; ua = ua->Next)
                    ;
                ua->Next = (IP_ADAPTER_UNICAST_ADDRESS *)ptr;
                ua = (IP_ADAPTER_UNICAST_ADDRESS *)ptr;
            }
            else
                ua = aa->FirstUnicastAddress = (IP_ADAPTER_UNICAST_ADDRESS *)ptr;
            for (i = 0; i < num_v6addrs; i++)
            {
                char addr_buf[46];

                memset(ua, 0, sizeof(IP_ADAPTER_UNICAST_ADDRESS));
                ua->u.s.Length              = sizeof(IP_ADAPTER_UNICAST_ADDRESS);
                ua->Address.iSockaddrLength = v6addrs[i].iSockaddrLength;
                ua->Address.lpSockaddr      = (SOCKADDR *)((char *)ua + ua->u.s.Length);

                sa = (struct WS_sockaddr_in6 *)ua->Address.lpSockaddr;
                memcpy(sa, v6addrs[i].lpSockaddr, sizeof(*sa));
                TRACE("IPv6 %d/%d: %s\n", i + 1, num_v6addrs,
                      debugstr_ipv6(sa, addr_buf));
                fill_unicast_addr_data(aa, ua);

                ua->OnLinkPrefixLength = mask_v6_to_prefix(&v6masks[i]);

                ptr += ua->u.s.Length + ua->Address.iSockaddrLength;
                if (i < num_v6addrs - 1)
                {
                    ua->Next = (IP_ADAPTER_UNICAST_ADDRESS *)ptr;
                    ua = ua->Next;
                }
            }
        }
        if (num_v4addrs && (flags & GAA_FLAG_INCLUDE_PREFIX))
        {
            IP_ADAPTER_PREFIX *prefix;

            prefix = aa->FirstPrefix = (IP_ADAPTER_PREFIX *)ptr;
            for (i = 0; i < num_v4addrs; i++)
            {
                char addr_buf[16];
                struct WS_sockaddr_in *sa;

                prefix->u.s.Length = sizeof(*prefix);
                prefix->u.s.Flags  = 0;
                prefix->Next       = NULL;
                prefix->Address.iSockaddrLength = sizeof(struct sockaddr_in);
                prefix->Address.lpSockaddr      = (SOCKADDR *)((char *)prefix + prefix->u.s.Length);

                sa = (struct WS_sockaddr_in *)prefix->Address.lpSockaddr;
                sa->sin_family           = WS_AF_INET;
                sa->sin_addr.S_un.S_addr = v4addrs[i] & v4masks[i];
                sa->sin_port             = 0;

                prefix->PrefixLength = mask_v4_to_prefix(v4masks[i]);

                TRACE("IPv4 network: %s/%u\n",
                      debugstr_ipv4((const in_addr_t *)&sa->sin_addr.S_un.S_addr, addr_buf),
                      prefix->PrefixLength);

                ptr += prefix->u.s.Length + prefix->Address.iSockaddrLength;
                if (i < num_v4addrs - 1)
                {
                    prefix->Next = (IP_ADAPTER_PREFIX *)ptr;
                    prefix = prefix->Next;
                }
            }
        }
        if (num_v6addrs && (flags & GAA_FLAG_INCLUDE_PREFIX))
        {
            IP_ADAPTER_PREFIX *prefix;

            if (aa->FirstPrefix)
            {
                for (prefix = aa->FirstPrefix; prefix->Next; prefix = prefix->Next)
                    ;
                prefix->Next = (IP_ADAPTER_PREFIX *)ptr;
                prefix = (IP_ADAPTER_PREFIX *)ptr;
            }
            else
                prefix = aa->FirstPrefix = (IP_ADAPTER_PREFIX *)ptr;
            for (i = 0; i < num_v6addrs; i++)
            {
                char addr_buf[46];
                struct WS_sockaddr_in6 *sa;
                const IN6_ADDR *addr, *mask;

                prefix->u.s.Length = sizeof(*prefix);
                prefix->u.s.Flags  = 0;
                prefix->Next       = NULL;
                prefix->Address.iSockaddrLength = sizeof(struct sockaddr_in6);
                prefix->Address.lpSockaddr      = (SOCKADDR *)((char *)prefix + prefix->u.s.Length);

                sa = (struct WS_sockaddr_in6 *)prefix->Address.lpSockaddr;
                sa->sin6_family   = WS_AF_INET6;
                sa->sin6_port     = 0;
                sa->sin6_flowinfo = 0;
                addr = &((struct WS_sockaddr_in6 *)v6addrs[i].lpSockaddr)->sin6_addr;
                mask = &((struct WS_sockaddr_in6 *)v6masks[i].lpSockaddr)->sin6_addr;
                for (j = 0; j < 8; j++) sa->sin6_addr.u.Word[j] = addr->u.Word[j] & mask->u.Word[j];
                sa->sin6_scope_id = 0;

                prefix->PrefixLength = mask_v6_to_prefix(&v6masks[i]);

                TRACE("IPv6 network: %s/%u\n", debugstr_ipv6(sa, addr_buf), prefix->PrefixLength);

                ptr += prefix->u.s.Length + prefix->Address.iSockaddrLength;
                if (i < num_v6addrs - 1)
                {
                    prefix->Next = (IP_ADAPTER_PREFIX *)ptr;
                    prefix = prefix->Next;
                }
            }
        }

        getInterfaceMtuByName(name, &aa->Mtu);

        getInterfaceStatusByName(name, &status);
        if (status == MIB_IF_OPER_STATUS_OPERATIONAL) aa->OperStatus = IfOperStatusUp;
        else if (status == MIB_IF_OPER_STATUS_NON_OPERATIONAL) aa->OperStatus = IfOperStatusDown;
        else aa->OperStatus = IfOperStatusUnknown;
    }
    *size = total_size;
    HeapFree(GetProcessHeap(), 0, routeTable);
    HeapFree(GetProcessHeap(), 0, v6addrs);
    HeapFree(GetProcessHeap(), 0, v6masks);
    HeapFree(GetProcessHeap(), 0, v4addrs);
    HeapFree(GetProcessHeap(), 0, v4masks);
    return ERROR_SUCCESS;
}

static void sockaddr_in_to_WS_storage( SOCKADDR_STORAGE *dst, const struct sockaddr_in *src )
{
    SOCKADDR_IN *s = (SOCKADDR_IN *)dst;

    s->sin_family = WS_AF_INET;
    s->sin_port = src->sin_port;
    memcpy( &s->sin_addr, &src->sin_addr, sizeof(IN_ADDR) );
    memset( (char *)s + FIELD_OFFSET( SOCKADDR_IN, sin_zero ), 0,
            sizeof(SOCKADDR_STORAGE) - FIELD_OFFSET( SOCKADDR_IN, sin_zero) );
}

#if defined(HAVE_STRUCT___RES_STATE__U__EXT_NSCOUNT6) || \
    (defined(HAVE___RES_GET_STATE) && defined(HAVE___RES_GETSERVERS)) || \
    defined(HAVE_RES_GETSERVERS)
static void sockaddr_in6_to_WS_storage( SOCKADDR_STORAGE *dst, const struct sockaddr_in6 *src )
{
    SOCKADDR_IN6 *s = (SOCKADDR_IN6 *)dst;

    s->sin6_family = WS_AF_INET6;
    s->sin6_port = src->sin6_port;
    s->sin6_flowinfo = src->sin6_flowinfo;
    memcpy( &s->sin6_addr, &src->sin6_addr, sizeof(IN6_ADDR) );
    s->sin6_scope_id = src->sin6_scope_id;
    memset( (char *)s + sizeof(SOCKADDR_IN6), 0,
                    sizeof(SOCKADDR_STORAGE) - sizeof(SOCKADDR_IN6) );
}
#endif

#ifdef HAVE_STRUCT___RES_STATE
/* call res_init() just once because of a bug in Mac OS X 10.4 */
/* Call once per thread on systems that have per-thread _res. */

static CRITICAL_SECTION res_init_cs;
static CRITICAL_SECTION_DEBUG res_init_cs_debug = {
    0, 0, &res_init_cs,
    { &res_init_cs_debug.ProcessLocksList, &res_init_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": res_init_cs") }
};
static CRITICAL_SECTION res_init_cs = { &res_init_cs_debug, -1, 0, 0, 0, 0 };

static void initialise_resolver(void)
{
    EnterCriticalSection(&res_init_cs);
    if ((_res.options & RES_INIT) == 0)
        res_init();
    LeaveCriticalSection(&res_init_cs);
}

#ifdef HAVE_RES_GETSERVERS
static int get_dns_servers( SOCKADDR_STORAGE *servers, int num, BOOL ip4_only )
{
    struct __res_state *state = &_res;
    int i, found = 0, total;
    SOCKADDR_STORAGE *addr = servers;
    union res_sockaddr_union *buf;

    initialise_resolver();

    total = res_getservers( state, NULL, 0 );

    if ((!servers || !num) && !ip4_only) return total;

    buf = HeapAlloc( GetProcessHeap(), 0, total * sizeof(union res_sockaddr_union) );
    total = res_getservers( state, buf, total );

    for (i = 0; i < total; i++)
    {
        if (buf[i].sin6.sin6_family == AF_INET6 && ip4_only) continue;
        if (buf[i].sin.sin_family != AF_INET && buf[i].sin6.sin6_family != AF_INET6) continue;

        found++;
        if (!servers || !num) continue;

        if (buf[i].sin6.sin6_family == AF_INET6)
        {
            sockaddr_in6_to_WS_storage( addr, &buf[i].sin6 );
        }
        else
        {
            sockaddr_in_to_WS_storage( addr, &buf[i].sin );
        }
        if (++addr >= servers + num) break;
    }

    HeapFree( GetProcessHeap(), 0, buf );
    return found;
}
#else

static int get_dns_servers( SOCKADDR_STORAGE *servers, int num, BOOL ip4_only )
{
    int i, ip6_count = 0;
    SOCKADDR_STORAGE *addr;

    initialise_resolver();

#ifdef HAVE_STRUCT___RES_STATE__U__EXT_NSCOUNT6
    ip6_count = _res._u._ext.nscount6;
#endif

    if (!servers || !num)
    {
        num = _res.nscount;
        if (ip4_only) num -= ip6_count;
        return num;
    }

    for (i = 0, addr = servers; addr < (servers + num) && i < _res.nscount; i++)
    {
#ifdef HAVE_STRUCT___RES_STATE__U__EXT_NSCOUNT6
        if (_res._u._ext.nsaddrs[i] && _res._u._ext.nsaddrs[i]->sin6_family == AF_INET6)
        {
            if (ip4_only) continue;
            sockaddr_in6_to_WS_storage( addr, _res._u._ext.nsaddrs[i] );
        }
        else
#endif
        {
            sockaddr_in_to_WS_storage( addr, _res.nsaddr_list + i );
        }
        addr++;
    }
    return addr - servers;
}
#endif
#elif defined(HAVE___RES_GET_STATE) && defined(HAVE___RES_GETSERVERS)

static int get_dns_servers( SOCKADDR_STORAGE *servers, int num, BOOL ip4_only )
{
    extern struct res_state *__res_get_state( void );
    extern int __res_getservers( struct res_state *, struct sockaddr_storage *, int );
    struct res_state *state = __res_get_state();
    int i, found = 0, total = __res_getservers( state, NULL, 0 );
    SOCKADDR_STORAGE *addr = servers;
    struct sockaddr_storage *buf;

    if ((!servers || !num) && !ip4_only) return total;

    buf = HeapAlloc( GetProcessHeap(), 0, total * sizeof(struct sockaddr_storage) );
    total = __res_getservers( state, buf, total );

    for (i = 0; i < total; i++)
    {
        if (buf[i].ss_family == AF_INET6 && ip4_only) continue;
        if (buf[i].ss_family != AF_INET && buf[i].ss_family != AF_INET6) continue;

        found++;
        if (!servers || !num) continue;

        if (buf[i].ss_family == AF_INET6)
        {
            sockaddr_in6_to_WS_storage( addr, (struct sockaddr_in6 *)(buf + i) );
        }
        else
        {
            sockaddr_in_to_WS_storage( addr, (struct sockaddr_in *)(buf + i) );
        }
        if (++addr >= servers + num) break;
    }

    HeapFree( GetProcessHeap(), 0, buf );
    return found;
}
#else

static int get_dns_servers( SOCKADDR_STORAGE *servers, int num, BOOL ip4_only )
{
    FIXME("Unimplemented on this system\n");
    return 0;
}
#endif

static ULONG get_dns_server_addresses(PIP_ADAPTER_DNS_SERVER_ADDRESS address, ULONG *len)
{
    int num = get_dns_servers( NULL, 0, FALSE );
    DWORD size;

    size = num * (sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS) + sizeof(SOCKADDR_STORAGE));
    if (!address || *len < size)
    {
        *len = size;
        return ERROR_BUFFER_OVERFLOW;
    }
    *len = size;
    if (num > 0)
    {
        PIP_ADAPTER_DNS_SERVER_ADDRESS addr = address;
        SOCKADDR_STORAGE *sock_addrs = (SOCKADDR_STORAGE *)(address + num);
        int i;

        get_dns_servers( sock_addrs, num, FALSE );

        for (i = 0; i < num; i++, addr = addr->Next)
        {
            addr->u.s.Length = sizeof(*addr);
            if (sock_addrs[i].ss_family == WS_AF_INET6)
                addr->Address.iSockaddrLength = sizeof(SOCKADDR_IN6);
            else
                addr->Address.iSockaddrLength = sizeof(SOCKADDR_IN);
            addr->Address.lpSockaddr = (SOCKADDR *)(sock_addrs + i);
            if (i == num - 1)
                addr->Next = NULL;
            else
                addr->Next = addr + 1;
        }
    }
    return ERROR_SUCCESS;
}

#ifdef HAVE_STRUCT___RES_STATE
static BOOL is_ip_address_string(const char *str)
{
    struct in_addr in;
    int ret;

    ret = inet_aton(str, &in);
    return ret != 0;
}
#endif

static ULONG get_dns_suffix(WCHAR *suffix, ULONG *len)
{
    ULONG size;
    const char *found_suffix = "";
    /* Always return a NULL-terminated string, even if it's empty. */

#ifdef HAVE_STRUCT___RES_STATE
    {
        ULONG i;
        initialise_resolver();
        for (i = 0; !*found_suffix && i < MAXDNSRCH + 1 && _res.dnsrch[i]; i++)
        {
            /* This uses a heuristic to select a DNS suffix:
             * the first, non-IP address string is selected.
             */
            if (!is_ip_address_string(_res.dnsrch[i]))
                found_suffix = _res.dnsrch[i];
        }
    }
#endif

    size = MultiByteToWideChar( CP_UNIXCP, 0, found_suffix, -1, NULL, 0 ) * sizeof(WCHAR);
    if (!suffix || *len < size)
    {
        *len = size;
        return ERROR_BUFFER_OVERFLOW;
    }
    *len = MultiByteToWideChar( CP_UNIXCP, 0, found_suffix, -1, suffix, *len / sizeof(WCHAR) ) * sizeof(WCHAR);
    return ERROR_SUCCESS;
}

ULONG WINAPI DECLSPEC_HOTPATCH GetAdaptersAddresses(ULONG family, ULONG flags, PVOID reserved,
                                                    PIP_ADAPTER_ADDRESSES aa, PULONG buflen)
{
    InterfaceIndexTable *table;
    ULONG i, size, dns_server_size = 0, dns_suffix_size, total_size, ret = ERROR_NO_DATA;

    TRACE("(%d, %08x, %p, %p, %p)\n", family, flags, reserved, aa, buflen);

    if (!buflen) return ERROR_INVALID_PARAMETER;

    get_interface_indices( FALSE, &table );
    if (!table || !table->numIndexes)
    {
        HeapFree(GetProcessHeap(), 0, table);
        return ERROR_NO_DATA;
    }
    total_size = 0;
    for (i = 0; i < table->numIndexes; i++)
    {
        size = 0;
        if ((ret = adapterAddressesFromIndex(family, flags, table->indexes[i], NULL, &size)))
        {
            HeapFree(GetProcessHeap(), 0, table);
            return ret;
        }
        total_size += size;
    }
    if (!(flags & GAA_FLAG_SKIP_DNS_SERVER))
    {
        /* Since DNS servers aren't really per adapter, get enough space for a
         * single copy of them.
         */
        get_dns_server_addresses(NULL, &dns_server_size);
        total_size += dns_server_size;
    }
    /* Since DNS suffix also isn't really per adapter, get enough space for a
     * single copy of it.
     */
    get_dns_suffix(NULL, &dns_suffix_size);
    total_size += dns_suffix_size;
    if (aa && *buflen >= total_size)
    {
        ULONG bytes_left = size = total_size;
        PIP_ADAPTER_ADDRESSES first_aa = aa;
        PIP_ADAPTER_DNS_SERVER_ADDRESS firstDns;
        WCHAR *dnsSuffix;

        for (i = 0; i < table->numIndexes; i++)
        {
            if ((ret = adapterAddressesFromIndex(family, flags, table->indexes[i], aa, &size)))
            {
                HeapFree(GetProcessHeap(), 0, table);
                return ret;
            }
            if (i < table->numIndexes - 1)
            {
                aa->Next = (IP_ADAPTER_ADDRESSES *)((char *)aa + size);
                aa = aa->Next;
                size = bytes_left -= size;
            }
        }
        if (dns_server_size)
        {
            firstDns = (PIP_ADAPTER_DNS_SERVER_ADDRESS)((BYTE *)first_aa + total_size - dns_server_size - dns_suffix_size);
            get_dns_server_addresses(firstDns, &dns_server_size);
            for (aa = first_aa; aa; aa = aa->Next)
            {
                if (aa->IfType != IF_TYPE_SOFTWARE_LOOPBACK && aa->OperStatus == IfOperStatusUp)
                    aa->FirstDnsServerAddress = firstDns;
            }
        }
        aa = first_aa;
        dnsSuffix = (WCHAR *)((BYTE *)aa + total_size - dns_suffix_size);
        get_dns_suffix(dnsSuffix, &dns_suffix_size);
        for (; aa; aa = aa->Next)
        {
            if (aa->IfType != IF_TYPE_SOFTWARE_LOOPBACK && aa->OperStatus == IfOperStatusUp)
                aa->DnsSuffix = dnsSuffix;
            else
                aa->DnsSuffix = dnsSuffix + dns_suffix_size / sizeof(WCHAR) - 1;
        }
        ret = ERROR_SUCCESS;
    }
    else
    {
        ret = ERROR_BUFFER_OVERFLOW;
        *buflen = total_size;
    }

    TRACE("num adapters %u\n", table->numIndexes);
    HeapFree(GetProcessHeap(), 0, table);
    return ret;
}

/******************************************************************
 *    GetBestInterface (IPHLPAPI.@)
 *
 * Get the interface, with the best route for the given IP address.
 *
 * PARAMS
 *  dwDestAddr     [In]  IP address to search the interface for
 *  pdwBestIfIndex [Out] found best interface
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetBestInterface(IPAddr dwDestAddr, PDWORD pdwBestIfIndex)
{
    struct WS_sockaddr_in sa_in;
    memset(&sa_in, 0, sizeof(sa_in));
    sa_in.sin_family = WS_AF_INET;
    sa_in.sin_addr.S_un.S_addr = dwDestAddr;
    return GetBestInterfaceEx((struct WS_sockaddr *)&sa_in, pdwBestIfIndex);
}

/******************************************************************
 *    GetBestInterfaceEx (IPHLPAPI.@)
 *
 * Get the interface, with the best route for the given IP address.
 *
 * PARAMS
 *  dwDestAddr     [In]  IP address to search the interface for
 *  pdwBestIfIndex [Out] found best interface
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetBestInterfaceEx(struct WS_sockaddr *pDestAddr, PDWORD pdwBestIfIndex)
{
  DWORD ret;

  TRACE("pDestAddr %p, pdwBestIfIndex %p\n", pDestAddr, pdwBestIfIndex);
  if (!pDestAddr || !pdwBestIfIndex)
    ret = ERROR_INVALID_PARAMETER;
  else {
    MIB_IPFORWARDROW ipRow;

    if (pDestAddr->sa_family == WS_AF_INET) {
      ret = GetBestRoute(((struct WS_sockaddr_in *)pDestAddr)->sin_addr.S_un.S_addr, 0, &ipRow);
      if (ret == ERROR_SUCCESS)
        *pdwBestIfIndex = ipRow.dwForwardIfIndex;
    } else {
      FIXME("address family %d not supported\n", pDestAddr->sa_family);
      ret = ERROR_NOT_SUPPORTED;
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetBestRoute (IPHLPAPI.@)
 *
 * Get the best route for the given IP address.
 *
 * PARAMS
 *  dwDestAddr   [In]  IP address to search the best route for
 *  dwSourceAddr [In]  optional source IP address
 *  pBestRoute   [Out] found best route
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetBestRoute(DWORD dwDestAddr, DWORD dwSourceAddr, PMIB_IPFORWARDROW pBestRoute)
{
  PMIB_IPFORWARDTABLE table;
  DWORD ret;

  TRACE("dwDestAddr 0x%08x, dwSourceAddr 0x%08x, pBestRoute %p\n", dwDestAddr,
   dwSourceAddr, pBestRoute);
  if (!pBestRoute)
    return ERROR_INVALID_PARAMETER;

  ret = AllocateAndGetIpForwardTableFromStack(&table, FALSE, GetProcessHeap(), 0);
  if (!ret) {
    DWORD ndx, matchedBits, matchedNdx = table->dwNumEntries;

    for (ndx = 0, matchedBits = 0; ndx < table->dwNumEntries; ndx++) {
      if (table->table[ndx].u1.ForwardType != MIB_IPROUTE_TYPE_INVALID &&
       (dwDestAddr & table->table[ndx].dwForwardMask) ==
       (table->table[ndx].dwForwardDest & table->table[ndx].dwForwardMask)) {
        DWORD numShifts, mask;

        for (numShifts = 0, mask = table->table[ndx].dwForwardMask;
         mask && mask & 1; mask >>= 1, numShifts++)
          ;
        if (numShifts > matchedBits) {
          matchedBits = numShifts;
          matchedNdx = ndx;
        }
        else if (!matchedBits) {
          matchedNdx = ndx;
        }
      }
    }
    if (matchedNdx < table->dwNumEntries) {
      memcpy(pBestRoute, &table->table[matchedNdx], sizeof(MIB_IPFORWARDROW));
      ret = ERROR_SUCCESS;
    }
    else {
      /* No route matches, which can happen if there's no default route. */
      ret = ERROR_HOST_UNREACHABLE;
    }
    HeapFree(GetProcessHeap(), 0, table);
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetFriendlyIfIndex (IPHLPAPI.@)
 *
 * Get a "friendly" version of IfIndex, which is one that doesn't
 * have the top byte set.  Doesn't validate whether IfIndex is a valid
 * adapter index.
 *
 * PARAMS
 *  IfIndex [In] interface index to get the friendly one for
 *
 * RETURNS
 *  A friendly version of IfIndex.
 */
DWORD WINAPI GetFriendlyIfIndex(DWORD IfIndex)
{
  /* windows doesn't validate these, either, just makes sure the top byte is
     cleared.  I assume my ifenum module never gives an index with the top
     byte set. */
  TRACE("returning %d\n", IfIndex);
  return IfIndex;
}


/******************************************************************
 *    GetIfEntry (IPHLPAPI.@)
 *
 * Get information about an interface.
 *
 * PARAMS
 *  pIfRow [In/Out] In:  dwIndex of MIB_IFROW selects the interface.
 *                  Out: interface information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetIfEntry(PMIB_IFROW pIfRow)
{
  DWORD ret;
  char nameBuf[MAX_ADAPTER_NAME];
  char *name;

  TRACE("pIfRow %p\n", pIfRow);
  if (!pIfRow)
    return ERROR_INVALID_PARAMETER;

  name = getInterfaceNameByIndex(pIfRow->dwIndex, nameBuf);
  if (name) {
    ret = getInterfaceEntryByName(name, pIfRow);
    if (ret == NO_ERROR)
      ret = getInterfaceStatsByName(name, pIfRow);
  }
  else
    ret = ERROR_INVALID_DATA;
  TRACE("returning %d\n", ret);
  return ret;
}

/******************************************************************
 *    GetIfEntry2 (IPHLPAPI.@)
 */
DWORD WINAPI GetIfEntry2( MIB_IF_ROW2 *row2 )
{
    DWORD ret, len = ARRAY_SIZE(row2->Description);
    char buf[MAX_ADAPTER_NAME], *name;
    MIB_IFROW row;

    TRACE("%p\n", row2);

    if (!row2 || (!(name = getInterfaceNameByIndex( row2->InterfaceIndex, buf )) &&
                  !(name = getInterfaceNameByIndex( row2->InterfaceLuid.Info.NetLuidIndex, buf ))))
    {
        return ERROR_INVALID_PARAMETER;
    }
    if ((ret = getInterfaceEntryByName( name, &row ))) return ret;
    if ((ret = getInterfaceStatsByName( name, &row ))) return ret;

    memset( row2, 0, sizeof(*row2) );
    row2->InterfaceLuid.Info.Reserved     = 0;
    row2->InterfaceLuid.Info.NetLuidIndex = row.dwIndex;
    row2->InterfaceLuid.Info.IfType       = row.dwType;
    row2->InterfaceIndex                  = row.dwIndex;
    ConvertInterfaceLuidToGuid( &row2->InterfaceLuid, &row2->InterfaceGuid );
    row2->Type                            = row.dwType;
    row2->Mtu                             = row.dwMtu;
    MultiByteToWideChar( CP_UNIXCP, 0, (const char *)row.bDescr, -1, row2->Description, len );
    row2->PhysicalAddressLength           = row.dwPhysAddrLen;
    memcpy( &row2->PhysicalAddress, &row.bPhysAddr, row.dwPhysAddrLen );
    memcpy( &row2->PermanentPhysicalAddress, &row.bPhysAddr, row.dwPhysAddrLen );
    row2->OperStatus                      = IfOperStatusUp;
    row2->AdminStatus                     = NET_IF_ADMIN_STATUS_UP;
    row2->MediaConnectState               = MediaConnectStateConnected;
    row2->ConnectionType                  = NET_IF_CONNECTION_DEDICATED;

    /* stats */
    row2->InOctets        = row.dwInOctets;
    row2->InUcastPkts     = row.dwInUcastPkts;
    row2->InNUcastPkts    = row.dwInNUcastPkts;
    row2->InDiscards      = row.dwInDiscards;
    row2->InErrors        = row.dwInErrors;
    row2->InUnknownProtos = row.dwInUnknownProtos;
    row2->OutOctets       = row.dwOutOctets;
    row2->OutUcastPkts    = row.dwOutUcastPkts;
    row2->OutNUcastPkts   = row.dwOutNUcastPkts;
    row2->OutDiscards     = row.dwOutDiscards;
    row2->OutErrors       = row.dwOutErrors;

    return NO_ERROR;
}

static int IfTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((const MIB_IFROW*)a)->dwIndex - ((const MIB_IFROW*)b)->dwIndex;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIfTable (IPHLPAPI.@)
 *
 * Get a table of local interfaces.
 *
 * PARAMS
 *  pIfTable [Out]    buffer for local interfaces table
 *  pdwSize  [In/Out] length of output buffer
 *  bOrder   [In]     whether to sort the table
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pdwSize is less than required, the function will return
 *  ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 *  size.
 *  If bOrder is true, the returned table will be sorted by interface index.
 */
DWORD WINAPI GetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIfTable %p, pdwSize %p, bOrder %d\n", pIfTable, pdwSize, bOrder);

  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numInterfaces = get_interface_indices( FALSE, NULL );
    ULONG size = sizeof(MIB_IFTABLE);

    if (numInterfaces > 1)
      size += (numInterfaces - 1) * sizeof(MIB_IFROW);
    if (!pIfTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table;
      get_interface_indices( FALSE, &table );

      if (table) {
        size = sizeof(MIB_IFTABLE);
        if (table->numIndexes > 1)
          size += (table->numIndexes - 1) * sizeof(MIB_IFROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx;

          *pdwSize = size;
          pIfTable->dwNumEntries = 0;
          for (ndx = 0; ndx < table->numIndexes; ndx++) {
            pIfTable->table[ndx].dwIndex = table->indexes[ndx];
            GetIfEntry(&pIfTable->table[ndx]);
            pIfTable->dwNumEntries++;
          }
          if (bOrder)
            qsort(pIfTable->table, pIfTable->dwNumEntries, sizeof(MIB_IFROW),
             IfTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}

/******************************************************************
 *    GetIfTable2Ex (IPHLPAPI.@)
 */
DWORD WINAPI GetIfTable2Ex( MIB_IF_TABLE_LEVEL level, MIB_IF_TABLE2 **table )
{
    DWORD i, nb_interfaces, size = sizeof(MIB_IF_TABLE2);
    InterfaceIndexTable *index_table;
    MIB_IF_TABLE2 *ret;

    TRACE( "level %u, table %p\n", level, table );

    if (!table || level > MibIfTableNormalWithoutStatistics)
        return ERROR_INVALID_PARAMETER;

    if (level != MibIfTableNormal)
        FIXME("level %u not fully supported\n", level);

    if ((nb_interfaces = get_interface_indices( FALSE, NULL )) > 1)
        size += (nb_interfaces - 1) * sizeof(MIB_IF_ROW2);

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, size ))) return ERROR_OUTOFMEMORY;

    get_interface_indices( FALSE, &index_table );
    if (!index_table)
    {
        HeapFree( GetProcessHeap(), 0, ret );
        return ERROR_OUTOFMEMORY;
    }

    ret->NumEntries = 0;
    for (i = 0; i < index_table->numIndexes; i++)
    {
        ret->Table[i].InterfaceIndex = index_table->indexes[i];
        GetIfEntry2( &ret->Table[i] );
        ret->NumEntries++;
    }

    HeapFree( GetProcessHeap(), 0, index_table );
    *table = ret;
    return NO_ERROR;
}

/******************************************************************
 *    GetIfTable2 (IPHLPAPI.@)
 */
DWORD WINAPI GetIfTable2( MIB_IF_TABLE2 **table )
{
    TRACE( "table %p\n", table );
    return GetIfTable2Ex(MibIfTableNormal, table);
}

/******************************************************************
 *    GetInterfaceInfo (IPHLPAPI.@)
 *
 * Get a list of network interface adapters.
 *
 * PARAMS
 *  pIfTable    [Out] buffer for interface adapters
 *  dwOutBufLen [Out] if buffer is too small, returns required size
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * BUGS
 *  MSDN states this should return non-loopback interfaces only.
 */
DWORD WINAPI GetInterfaceInfo(PIP_INTERFACE_INFO pIfTable, PULONG dwOutBufLen)
{
  DWORD ret;

  TRACE("pIfTable %p, dwOutBufLen %p\n", pIfTable, dwOutBufLen);
  if (!dwOutBufLen)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numInterfaces = get_interface_indices( FALSE, NULL );
    ULONG size = sizeof(IP_INTERFACE_INFO);

    if (numInterfaces > 1)
      size += (numInterfaces - 1) * sizeof(IP_ADAPTER_INDEX_MAP);
    if (!pIfTable || *dwOutBufLen < size) {
      *dwOutBufLen = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table;
      get_interface_indices( FALSE, &table );

      if (table) {
        size = sizeof(IP_INTERFACE_INFO);
        if (table->numIndexes > 1)
          size += (table->numIndexes - 1) * sizeof(IP_ADAPTER_INDEX_MAP);
        if (*dwOutBufLen < size) {
          *dwOutBufLen = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx;
          char nameBuf[MAX_ADAPTER_NAME];

          *dwOutBufLen = size;
          pIfTable->NumAdapters = 0;
          for (ndx = 0; ndx < table->numIndexes; ndx++) {
            const char *walker, *name;
            WCHAR *assigner;

            pIfTable->Adapter[ndx].Index = table->indexes[ndx];
            name = getInterfaceNameByIndex(table->indexes[ndx], nameBuf);
            for (walker = name, assigner = pIfTable->Adapter[ndx].Name;
             walker && *walker &&
             assigner - pIfTable->Adapter[ndx].Name < MAX_ADAPTER_NAME - 1;
             walker++, assigner++)
              *assigner = *walker;
            *assigner = 0;
            pIfTable->NumAdapters++;
          }
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetIpAddrTable (IPHLPAPI.@)
 *
 * Get interface-to-IP address mapping table. 
 *
 * PARAMS
 *  pIpAddrTable [Out]    buffer for mapping table
 *  pdwSize      [In/Out] length of output buffer
 *  bOrder       [In]     whether to sort the table
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pdwSize is less than required, the function will return
 *  ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 *  size.
 *  If bOrder is true, the returned table will be sorted by the next hop and
 *  an assortment of arbitrary parameters.
 */
DWORD WINAPI GetIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIpAddrTable %p, pdwSize %p, bOrder %d\n", pIpAddrTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    PMIB_IPADDRTABLE table;

    ret = getIPAddrTable(&table, GetProcessHeap(), 0);
    if (ret == NO_ERROR)
    {
      ULONG size = FIELD_OFFSET(MIB_IPADDRTABLE, table[table->dwNumEntries]);

      if (!pIpAddrTable || *pdwSize < size) {
        *pdwSize = size;
        ret = ERROR_INSUFFICIENT_BUFFER;
      }
      else {
        *pdwSize = size;
        memcpy(pIpAddrTable, table, size);
        /* sort by numeric IP value */
        if (bOrder)
          qsort(pIpAddrTable->table, pIpAddrTable->dwNumEntries,
           sizeof(MIB_IPADDRROW), IpAddrTableNumericSorter);
        /* sort ensuring loopback interfaces are in the end */
        else
          qsort(pIpAddrTable->table, pIpAddrTable->dwNumEntries,
           sizeof(MIB_IPADDRROW), IpAddrTableLoopbackSorter);
        ret = NO_ERROR;
      }
      HeapFree(GetProcessHeap(), 0, table);
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetIpForwardTable (IPHLPAPI.@)
 *
 * Get the route table.
 *
 * PARAMS
 *  pIpForwardTable [Out]    buffer for route table
 *  pdwSize         [In/Out] length of output buffer
 *  bOrder          [In]     whether to sort the table
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pdwSize is less than required, the function will return
 *  ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 *  size.
 *  If bOrder is true, the returned table will be sorted by the next hop and
 *  an assortment of arbitrary parameters.
 */
DWORD WINAPI GetIpForwardTable(PMIB_IPFORWARDTABLE pIpForwardTable, PULONG pdwSize, BOOL bOrder)
{
    DWORD ret;
    PMIB_IPFORWARDTABLE table;

    TRACE("pIpForwardTable %p, pdwSize %p, bOrder %d\n", pIpForwardTable, pdwSize, bOrder);

    if (!pdwSize) return ERROR_INVALID_PARAMETER;

    ret = AllocateAndGetIpForwardTableFromStack(&table, bOrder, GetProcessHeap(), 0);
    if (!ret) {
        DWORD size = FIELD_OFFSET( MIB_IPFORWARDTABLE, table[table->dwNumEntries] );
        if (!pIpForwardTable || *pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          *pdwSize = size;
          memcpy(pIpForwardTable, table, size);
        }
        HeapFree(GetProcessHeap(), 0, table);
    }
    TRACE("returning %d\n", ret);
    return ret;
}


/******************************************************************
 *    GetIpNetTable (IPHLPAPI.@)
 *
 * Get the IP-to-physical address mapping table.
 *
 * PARAMS
 *  pIpNetTable [Out]    buffer for mapping table
 *  pdwSize     [In/Out] length of output buffer
 *  bOrder      [In]     whether to sort the table
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pdwSize is less than required, the function will return
 *  ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 *  size.
 *  If bOrder is true, the returned table will be sorted by IP address.
 */
DWORD WINAPI GetIpNetTable(PMIB_IPNETTABLE pIpNetTable, PULONG pdwSize, BOOL bOrder)
{
    DWORD ret;
    PMIB_IPNETTABLE table;

    TRACE("pIpNetTable %p, pdwSize %p, bOrder %d\n", pIpNetTable, pdwSize, bOrder);

    if (!pdwSize) return ERROR_INVALID_PARAMETER;

    ret = AllocateAndGetIpNetTableFromStack( &table, bOrder, GetProcessHeap(), 0 );
    if (!ret) {
        DWORD size = FIELD_OFFSET( MIB_IPNETTABLE, table[table->dwNumEntries] );
        if (!pIpNetTable || *pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          *pdwSize = size;
          memcpy(pIpNetTable, table, size);
        }
        HeapFree(GetProcessHeap(), 0, table);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* Gets the DNS server list into the list beginning at list.  Assumes that
 * a single server address may be placed at list if *len is at least
 * sizeof(IP_ADDR_STRING) long.  Otherwise, list->Next is set to firstDynamic,
 * and assumes that all remaining DNS servers are contiguously located
 * beginning at firstDynamic.  On input, *len is assumed to be the total number
 * of bytes available for all DNS servers, and is ignored if list is NULL.
 * On return, *len is set to the total number of bytes required for all DNS
 * servers.
 * Returns ERROR_BUFFER_OVERFLOW if *len is insufficient,
 * ERROR_SUCCESS otherwise.
 */
static DWORD get_dns_server_list(PIP_ADDR_STRING list,
 PIP_ADDR_STRING firstDynamic, DWORD *len)
{
  DWORD size;
  int num = get_dns_servers( NULL, 0, TRUE );

  size = num * sizeof(IP_ADDR_STRING);
  if (!list || *len < size) {
    *len = size;
    return ERROR_BUFFER_OVERFLOW;
  }
  *len = size;
  if (num > 0) {
    PIP_ADDR_STRING ptr;
    int i;
    SOCKADDR_STORAGE *addr = HeapAlloc( GetProcessHeap(), 0, num * sizeof(SOCKADDR_STORAGE) );

    get_dns_servers( addr, num, TRUE );

    for (i = 0, ptr = list; i < num; i++, ptr = ptr->Next) {
        RtlIpv4AddressToStringA((IN_ADDR *)&((struct sockaddr_in *)(addr + i))->sin_addr.s_addr,
       ptr->IpAddress.String);
      if (i == num - 1)
        ptr->Next = NULL;
      else if (i == 0)
        ptr->Next = firstDynamic;
      else
        ptr->Next = (PIP_ADDR_STRING)((PBYTE)ptr + sizeof(IP_ADDR_STRING));
    }
    HeapFree( GetProcessHeap(), 0, addr );
  }
  return ERROR_SUCCESS;
}

/******************************************************************
 *    GetNetworkParams (IPHLPAPI.@)
 *
 * Get the network parameters for the local computer.
 *
 * PARAMS
 *  pFixedInfo [Out]    buffer for network parameters
 *  pOutBufLen [In/Out] length of output buffer
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pOutBufLen is less than required, the function will return
 *  ERROR_INSUFFICIENT_BUFFER, and pOutBufLen will be set to the required byte
 *  size.
 */
DWORD WINAPI GetNetworkParams(PFIXED_INFO pFixedInfo, PULONG pOutBufLen)
{
  DWORD ret, size, serverListSize;
  LONG regReturn;
  HKEY hKey;

  TRACE("pFixedInfo %p, pOutBufLen %p\n", pFixedInfo, pOutBufLen);
  if (!pOutBufLen)
    return ERROR_INVALID_PARAMETER;

  get_dns_server_list(NULL, NULL, &serverListSize);
  size = sizeof(FIXED_INFO) + serverListSize - sizeof(IP_ADDR_STRING);
  if (!pFixedInfo || *pOutBufLen < size) {
    *pOutBufLen = size;
    return ERROR_BUFFER_OVERFLOW;
  }

  memset(pFixedInfo, 0, size);
  size = sizeof(pFixedInfo->HostName);
  GetComputerNameExA(ComputerNameDnsHostname, pFixedInfo->HostName, &size);
  size = sizeof(pFixedInfo->DomainName);
  GetComputerNameExA(ComputerNameDnsDomain, pFixedInfo->DomainName, &size);
  get_dns_server_list(&pFixedInfo->DnsServerList,
   (PIP_ADDR_STRING)((BYTE *)pFixedInfo + sizeof(FIXED_INFO)),
   &serverListSize);
  /* Assume the first DNS server in the list is the "current" DNS server: */
  pFixedInfo->CurrentDnsServer = &pFixedInfo->DnsServerList;
  pFixedInfo->NodeType = HYBRID_NODETYPE;
  regReturn = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
   "SYSTEM\\CurrentControlSet\\Services\\VxD\\MSTCP", 0, KEY_READ, &hKey);
  if (regReturn != ERROR_SUCCESS)
    regReturn = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
     "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", 0, KEY_READ,
     &hKey);
  if (regReturn == ERROR_SUCCESS)
  {
    DWORD size = sizeof(pFixedInfo->ScopeId);

    RegQueryValueExA(hKey, "ScopeID", NULL, NULL, (LPBYTE)pFixedInfo->ScopeId, &size);
    RegCloseKey(hKey);
  }

  /* FIXME: can check whether routing's enabled in /proc/sys/net/ipv4/ip_forward
     I suppose could also check for a listener on port 53 to set EnableDns */
  ret = NO_ERROR;
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetNumberOfInterfaces (IPHLPAPI.@)
 *
 * Get the number of interfaces.
 *
 * PARAMS
 *  pdwNumIf [Out] number of interfaces
 *
 * RETURNS
 *  NO_ERROR on success, ERROR_INVALID_PARAMETER if pdwNumIf is NULL.
 */
DWORD WINAPI GetNumberOfInterfaces(PDWORD pdwNumIf)
{
  DWORD ret;

  TRACE("pdwNumIf %p\n", pdwNumIf);
  if (!pdwNumIf)
    ret = ERROR_INVALID_PARAMETER;
  else {
    *pdwNumIf = get_interface_indices( FALSE, NULL );
    ret = NO_ERROR;
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetPerAdapterInfo (IPHLPAPI.@)
 *
 * Get information about an adapter corresponding to an interface.
 *
 * PARAMS
 *  IfIndex         [In]     interface info
 *  pPerAdapterInfo [Out]    buffer for per adapter info
 *  pOutBufLen      [In/Out] length of output buffer
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetPerAdapterInfo(ULONG IfIndex, PIP_PER_ADAPTER_INFO pPerAdapterInfo, PULONG pOutBufLen)
{
  ULONG bytesNeeded = sizeof(IP_PER_ADAPTER_INFO), serverListSize = 0;
  DWORD ret = NO_ERROR;

  TRACE("(IfIndex %d, pPerAdapterInfo %p, pOutBufLen %p)\n", IfIndex, pPerAdapterInfo, pOutBufLen);

  if (!pOutBufLen) return ERROR_INVALID_PARAMETER;

  if (!isIfIndexLoopback(IfIndex)) {
    get_dns_server_list(NULL, NULL, &serverListSize);
    if (serverListSize > sizeof(IP_ADDR_STRING))
      bytesNeeded += serverListSize - sizeof(IP_ADDR_STRING);
  }
  if (!pPerAdapterInfo || *pOutBufLen < bytesNeeded)
  {
    *pOutBufLen = bytesNeeded;
    return ERROR_BUFFER_OVERFLOW;
  }

  memset(pPerAdapterInfo, 0, bytesNeeded);
  if (!isIfIndexLoopback(IfIndex)) {
    ret = get_dns_server_list(&pPerAdapterInfo->DnsServerList,
     (PIP_ADDR_STRING)((PBYTE)pPerAdapterInfo + sizeof(IP_PER_ADAPTER_INFO)),
     &serverListSize);
    /* Assume the first DNS server in the list is the "current" DNS server: */
    pPerAdapterInfo->CurrentDnsServer = &pPerAdapterInfo->DnsServerList;
  }
  return ret;
}


/******************************************************************
 *    GetRTTAndHopCount (IPHLPAPI.@)
 *
 * Get round-trip time (RTT) and hop count.
 *
 * PARAMS
 *
 *  DestIpAddress [In]  destination address to get the info for
 *  HopCount      [Out] retrieved hop count
 *  MaxHops       [In]  maximum hops to search for the destination
 *  RTT           [Out] RTT in milliseconds
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * FIXME
 *  Stub, returns FALSE.
 */
BOOL WINAPI GetRTTAndHopCount(IPAddr DestIpAddress, PULONG HopCount, ULONG MaxHops, PULONG RTT)
{
  FIXME("(DestIpAddress 0x%08x, HopCount %p, MaxHops %d, RTT %p): stub\n",
   DestIpAddress, HopCount, MaxHops, RTT);
  return FALSE;
}


/******************************************************************
 *    GetTcpTable (IPHLPAPI.@)
 *
 * Get the table of active TCP connections.
 *
 * PARAMS
 *  pTcpTable [Out]    buffer for TCP connections table
 *  pdwSize   [In/Out] length of output buffer
 *  bOrder    [In]     whether to order the table
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pdwSize is less than required, the function will return 
 *  ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to 
 *  the required byte size.
 *  If bOrder is true, the returned table will be sorted, first by
 *  local address and port number, then by remote address and port
 *  number.
 */
DWORD WINAPI GetTcpTable(PMIB_TCPTABLE pTcpTable, PDWORD pdwSize, BOOL bOrder)
{
    TRACE("pTcpTable %p, pdwSize %p, bOrder %d\n", pTcpTable, pdwSize, bOrder);
    return GetExtendedTcpTable(pTcpTable, pdwSize, bOrder, WS_AF_INET, TCP_TABLE_BASIC_ALL, 0);
}

/******************************************************************
 *    GetExtendedTcpTable (IPHLPAPI.@)
 */
DWORD WINAPI GetExtendedTcpTable(PVOID pTcpTable, PDWORD pdwSize, BOOL bOrder,
                                 ULONG ulAf, TCP_TABLE_CLASS TableClass, ULONG Reserved)
{
    DWORD ret, size;
    void *table;

    TRACE("pTcpTable %p, pdwSize %p, bOrder %d, ulAf %u, TableClass %u, Reserved %u\n",
           pTcpTable, pdwSize, bOrder, ulAf, TableClass, Reserved);

    if (!pdwSize) return ERROR_INVALID_PARAMETER;

    if (TableClass >= TCP_TABLE_OWNER_MODULE_LISTENER)
        FIXME("module classes not fully supported\n");

    switch (ulAf)
    {
        case WS_AF_INET:
            ret = build_tcp_table(TableClass, &table, bOrder, GetProcessHeap(), 0, &size);
            break;

        case WS_AF_INET6:
            ret = build_tcp6_table(TableClass, &table, bOrder, GetProcessHeap(), 0, &size);
            break;

        default:
            FIXME("ulAf = %u not supported\n", ulAf);
            ret = ERROR_NOT_SUPPORTED;
    }

    if (ret)
        return ret;

    if (!pTcpTable || *pdwSize < size)
    {
        *pdwSize = size;
        ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        *pdwSize = size;
        memcpy(pTcpTable, table, size);
    }
    HeapFree(GetProcessHeap(), 0, table);
    return ret;
}

/******************************************************************
 *    GetUdpTable (IPHLPAPI.@)
 *
 * Get a table of active UDP connections.
 *
 * PARAMS
 *  pUdpTable [Out]    buffer for UDP connections table
 *  pdwSize   [In/Out] length of output buffer
 *  bOrder    [In]     whether to order the table
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pdwSize is less than required, the function will return 
 *  ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the
 *  required byte size.
 *  If bOrder is true, the returned table will be sorted, first by
 *  local address, then by local port number.
 */
DWORD WINAPI GetUdpTable(PMIB_UDPTABLE pUdpTable, PDWORD pdwSize, BOOL bOrder)
{
    return GetExtendedUdpTable(pUdpTable, pdwSize, bOrder, WS_AF_INET, UDP_TABLE_BASIC, 0);
}

/******************************************************************
 *    GetUdp6Table (IPHLPAPI.@)
 */
DWORD WINAPI GetUdp6Table(PMIB_UDP6TABLE pUdpTable, PDWORD pdwSize, BOOL bOrder)
{
    return GetExtendedUdpTable(pUdpTable, pdwSize, bOrder, WS_AF_INET6, UDP_TABLE_BASIC, 0);
}

/******************************************************************
 *    GetExtendedUdpTable (IPHLPAPI.@)
 */
DWORD WINAPI GetExtendedUdpTable(PVOID pUdpTable, PDWORD pdwSize, BOOL bOrder,
                                 ULONG ulAf, UDP_TABLE_CLASS TableClass, ULONG Reserved)
{
    DWORD ret, size;
    void *table;

    TRACE("pUdpTable %p, pdwSize %p, bOrder %d, ulAf %u, TableClass %u, Reserved %u\n",
           pUdpTable, pdwSize, bOrder, ulAf, TableClass, Reserved);

    if (!pdwSize) return ERROR_INVALID_PARAMETER;

    if (TableClass == UDP_TABLE_OWNER_MODULE)
        FIXME("UDP_TABLE_OWNER_MODULE not fully supported\n");

    switch (ulAf)
    {
        case WS_AF_INET:
            ret = build_udp_table(TableClass, &table, bOrder, GetProcessHeap(), 0, &size);
            break;

        case WS_AF_INET6:
            ret = build_udp6_table(TableClass, &table, bOrder, GetProcessHeap(), 0, &size);
            break;

        default:
            FIXME("ulAf = %u not supported\n", ulAf);
            ret = ERROR_NOT_SUPPORTED;
    }

    if (ret)
        return ret;

    if (!pUdpTable || *pdwSize < size)
    {
        *pdwSize = size;
        ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        *pdwSize = size;
        memcpy(pUdpTable, table, size);
    }
    HeapFree(GetProcessHeap(), 0, table);
    return ret;
}

DWORD WINAPI GetUnicastIpAddressEntry(MIB_UNICASTIPADDRESS_ROW *row)
{
    IP_ADAPTER_ADDRESSES *aa, *ptr;
    ULONG size = 0;
    DWORD ret;

    TRACE("%p\n", row);

    if (!row)
        return ERROR_INVALID_PARAMETER;

    ret = GetAdaptersAddresses(row->Address.si_family, 0, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW)
        return ret;
    if (!(ptr = HeapAlloc(GetProcessHeap(), 0, size)))
        return ERROR_OUTOFMEMORY;
    if ((ret = GetAdaptersAddresses(row->Address.si_family, 0, NULL, ptr, &size)))
    {
        HeapFree(GetProcessHeap(), 0, ptr);
        return ret;
    }

    ret = ERROR_FILE_NOT_FOUND;
    for (aa = ptr; aa; aa = aa->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *ua;

        if (aa->u.s.IfIndex != row->InterfaceIndex &&
            memcmp(&aa->Luid, &row->InterfaceLuid, sizeof(row->InterfaceLuid)))
            continue;
        ret = ERROR_NOT_FOUND;

        ua = aa->FirstUnicastAddress;
        while (ua)
        {
            SOCKADDR_INET *uaaddr = (SOCKADDR_INET *)ua->Address.lpSockaddr;

            if ((row->Address.si_family == WS_AF_INET6 &&
                 !memcmp(&row->Address.Ipv6.sin6_addr, &uaaddr->Ipv6.sin6_addr, sizeof(uaaddr->Ipv6.sin6_addr))) ||
                (row->Address.si_family == WS_AF_INET &&
                 row->Address.Ipv4.sin_addr.S_un.S_addr == uaaddr->Ipv4.sin_addr.S_un.S_addr))
            {
                memcpy(&row->InterfaceLuid, &aa->Luid, sizeof(aa->Luid));
                row->InterfaceIndex     = aa->u.s.IfIndex;
                row->PrefixOrigin       = ua->PrefixOrigin;
                row->SuffixOrigin       = ua->SuffixOrigin;
                row->ValidLifetime      = ua->ValidLifetime;
                row->PreferredLifetime  = ua->PreferredLifetime;
                row->OnLinkPrefixLength = ua->OnLinkPrefixLength;
                row->SkipAsSource       = 0;
                row->DadState           = ua->DadState;
                if (row->Address.si_family == WS_AF_INET6)
                    row->ScopeId.u.Value  = row->Address.Ipv6.sin6_scope_id;
                else
                    row->ScopeId.u.Value  = 0;
                NtQuerySystemTime(&row->CreationTimeStamp);
                HeapFree(GetProcessHeap(), 0, ptr);
                return NO_ERROR;
            }
            ua = ua->Next;
        }
    }
    HeapFree(GetProcessHeap(), 0, ptr);

    return ret;
}

DWORD WINAPI GetUnicastIpAddressTable(ADDRESS_FAMILY family, MIB_UNICASTIPADDRESS_TABLE **table)
{
    IP_ADAPTER_ADDRESSES *aa, *ptr;
    MIB_UNICASTIPADDRESS_TABLE *data;
    DWORD ret, count = 0;
    ULONG size, flags;

    TRACE("%u, %p\n", family, table);

    if (!table || (family != WS_AF_INET && family != WS_AF_INET6 && family != WS_AF_UNSPEC))
        return ERROR_INVALID_PARAMETER;

    flags = GAA_FLAG_SKIP_ANYCAST |
            GAA_FLAG_SKIP_MULTICAST |
            GAA_FLAG_SKIP_DNS_SERVER |
            GAA_FLAG_SKIP_FRIENDLY_NAME;

    ret = GetAdaptersAddresses(family, flags, NULL, NULL, &size);
    if (ret != ERROR_BUFFER_OVERFLOW)
        return ret;
    if (!(ptr = HeapAlloc(GetProcessHeap(), 0, size)))
        return ERROR_OUTOFMEMORY;
    if ((ret = GetAdaptersAddresses(family, flags, NULL, ptr, &size)))
    {
        HeapFree(GetProcessHeap(), 0, ptr);
        return ret;
    }

    for (aa = ptr; aa; aa = aa->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *ua = aa->FirstUnicastAddress;
        while (ua)
        {
            count++;
            ua = ua->Next;
        }
    }

    if (!(data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) + (count - 1) * sizeof(data->Table[0]))))
    {
        HeapFree(GetProcessHeap(), 0, ptr);
        return ERROR_OUTOFMEMORY;
    }

    data->NumEntries = 0;
    for (aa = ptr; aa; aa = aa->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *ua = aa->FirstUnicastAddress;
        while (ua)
        {
            MIB_UNICASTIPADDRESS_ROW *row = &data->Table[data->NumEntries];
            memcpy(&row->Address, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
            memcpy(&row->InterfaceLuid, &aa->Luid, sizeof(aa->Luid));
            row->InterfaceIndex     = aa->u.s.IfIndex;
            row->PrefixOrigin       = ua->PrefixOrigin;
            row->SuffixOrigin       = ua->SuffixOrigin;
            row->ValidLifetime      = ua->ValidLifetime;
            row->PreferredLifetime  = ua->PreferredLifetime;
            row->OnLinkPrefixLength = ua->OnLinkPrefixLength;
            row->SkipAsSource       = 0;
            row->DadState           = ua->DadState;
            if (row->Address.si_family == WS_AF_INET6)
                row->ScopeId.u.Value  = row->Address.Ipv6.sin6_scope_id;
            else
                row->ScopeId.u.Value  = 0;
            NtQuerySystemTime(&row->CreationTimeStamp);

            data->NumEntries++;
            ua = ua->Next;
        }
    }

    HeapFree(GetProcessHeap(), 0, ptr);

    *table = data;
    return ret;
}

/******************************************************************
 *    GetUniDirectionalAdapterInfo (IPHLPAPI.@)
 *
 * This is a Win98-only function to get information on "unidirectional"
 * adapters.  Since this is pretty nonsensical in other contexts, it
 * never returns anything.
 *
 * PARAMS
 *  pIPIfInfo   [Out] buffer for adapter infos
 *  dwOutBufLen [Out] length of the output buffer
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI GetUniDirectionalAdapterInfo(PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS pIPIfInfo, PULONG dwOutBufLen)
{
  TRACE("pIPIfInfo %p, dwOutBufLen %p\n", pIPIfInfo, dwOutBufLen);
  /* a unidirectional adapter?? not bloody likely! */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    IpReleaseAddress (IPHLPAPI.@)
 *
 * Release an IP obtained through DHCP,
 *
 * PARAMS
 *  AdapterInfo [In] adapter to release IP address
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  Since GetAdaptersInfo never returns adapters that have DHCP enabled,
 *  this function does nothing.
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI IpReleaseAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo)
{
  FIXME("Stub AdapterInfo %p\n", AdapterInfo);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    IpRenewAddress (IPHLPAPI.@)
 *
 * Renew an IP obtained through DHCP.
 *
 * PARAMS
 *  AdapterInfo [In] adapter to renew IP address
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  Since GetAdaptersInfo never returns adapters that have DHCP enabled,
 *  this function does nothing.
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI IpRenewAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo)
{
  FIXME("Stub AdapterInfo %p\n", AdapterInfo);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    NotifyAddrChange (IPHLPAPI.@)
 *
 * Notify caller whenever the ip-interface map is changed.
 *
 * PARAMS
 *  Handle     [Out] handle usable in asynchronous notification
 *  overlapped [In]  overlapped structure that notifies the caller
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI NotifyAddrChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
  FIXME("(Handle %p, overlapped %p): stub\n", Handle, overlapped);
  if (Handle) *Handle = INVALID_HANDLE_VALUE;
  if (overlapped) ((IO_STATUS_BLOCK *) overlapped)->u.Status = STATUS_PENDING;
  return ERROR_IO_PENDING;
}


/******************************************************************
 *    NotifyIpInterfaceChange (IPHLPAPI.@)
 */
DWORD WINAPI NotifyIpInterfaceChange(ADDRESS_FAMILY family, PIPINTERFACE_CHANGE_CALLBACK callback,
                                     PVOID context, BOOLEAN init_notify, PHANDLE handle)
{
    FIXME("(family %d, callback %p, context %p, init_notify %d, handle %p): stub\n",
          family, callback, context, init_notify, handle);
    if (handle) *handle = NULL;
    return NO_ERROR;
}

/******************************************************************
 *    NotifyRouteChange2 (IPHLPAPI.@)
 */
DWORD WINAPI NotifyRouteChange2(ADDRESS_FAMILY family, PIPFORWARD_CHANGE_CALLBACK callback, VOID* context,
                                BOOLEAN init_notify, HANDLE* handle)
{
    FIXME("(family %d, callback %p, context %p, init_notify %d, handle %p): stub\n",
        family, callback, context, init_notify, handle);
    if (handle) *handle = NULL;
    return NO_ERROR;
}


/******************************************************************
 *    NotifyRouteChange (IPHLPAPI.@)
 *
 * Notify caller whenever the ip routing table is changed.
 *
 * PARAMS
 *  Handle     [Out] handle usable in asynchronous notification
 *  overlapped [In]  overlapped structure that notifies the caller
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI NotifyRouteChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
  FIXME("(Handle %p, overlapped %p): stub\n", Handle, overlapped);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    NotifyUnicastIpAddressChange (IPHLPAPI.@)
 */
DWORD WINAPI NotifyUnicastIpAddressChange(ADDRESS_FAMILY family, PUNICAST_IPADDRESS_CHANGE_CALLBACK callback,
                                          PVOID context, BOOLEAN init_notify, PHANDLE handle)
{
    FIXME("(family %d, callback %p, context %p, init_notify %d, handle %p): semi-stub\n",
          family, callback, context, init_notify, handle);
    if (handle) *handle = NULL;

    if (init_notify)
        callback(context, NULL, MibInitialNotification);

    return NO_ERROR;
}

/******************************************************************
 *    SendARP (IPHLPAPI.@)
 *
 * Send an ARP request.
 *
 * PARAMS
 *  DestIP     [In]     attempt to obtain this IP
 *  SrcIP      [In]     optional sender IP address
 *  pMacAddr   [Out]    buffer for the mac address
 *  PhyAddrLen [In/Out] length of the output buffer
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI SendARP(IPAddr DestIP, IPAddr SrcIP, PULONG pMacAddr, PULONG PhyAddrLen)
{
  FIXME("(DestIP 0x%08x, SrcIP 0x%08x, pMacAddr %p, PhyAddrLen %p): stub\n",
   DestIP, SrcIP, pMacAddr, PhyAddrLen);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SetIfEntry (IPHLPAPI.@)
 *
 * Set the administrative status of an interface.
 *
 * PARAMS
 *  pIfRow [In] dwAdminStatus member specifies the new status.
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI SetIfEntry(PMIB_IFROW pIfRow)
{
  FIXME("(pIfRow %p): stub\n", pIfRow);
  /* this is supposed to set an interface administratively up or down.
     Could do SIOCSIFFLAGS and set/clear IFF_UP, but, not sure I want to, and
     this sort of down is indistinguishable from other sorts of down (e.g. no
     link). */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SetIpForwardEntry (IPHLPAPI.@)
 *
 * Modify an existing route.
 *
 * PARAMS
 *  pRoute [In] route with the new information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
  FIXME("(pRoute %p): stub\n", pRoute);
  /* this is to add a route entry, how's it distinguishable from
     CreateIpForwardEntry?
     could use SIOCADDRT, not sure I want to */
  return 0;
}


/******************************************************************
 *    SetIpNetEntry (IPHLPAPI.@)
 *
 * Modify an existing ARP entry.
 *
 * PARAMS
 *  pArpEntry [In] ARP entry with the new information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetIpNetEntry(PMIB_IPNETROW pArpEntry)
{
  FIXME("(pArpEntry %p): stub\n", pArpEntry);
  /* same as CreateIpNetEntry here, could use SIOCSARP, not sure I want to */
  return 0;
}


/******************************************************************
 *    SetIpStatistics (IPHLPAPI.@)
 *
 * Toggle IP forwarding and det the default TTL value.
 *
 * PARAMS
 *  pIpStats [In] IP statistics with the new information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetIpStatistics(PMIB_IPSTATS pIpStats)
{
  FIXME("(pIpStats %p): stub\n", pIpStats);
  return 0;
}


/******************************************************************
 *    SetIpTTL (IPHLPAPI.@)
 *
 * Set the default TTL value.
 *
 * PARAMS
 *  nTTL [In] new TTL value
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetIpTTL(UINT nTTL)
{
  FIXME("(nTTL %d): stub\n", nTTL);
  /* could echo nTTL > /proc/net/sys/net/ipv4/ip_default_ttl, not sure I
     want to.  Could map EACCESS to ERROR_ACCESS_DENIED, I suppose */
  return 0;
}


/******************************************************************
 *    SetTcpEntry (IPHLPAPI.@)
 *
 * Set the state of a TCP connection.
 *
 * PARAMS
 *  pTcpRow [In] specifies connection with new state
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetTcpEntry(PMIB_TCPROW pTcpRow)
{
  FIXME("(pTcpRow %p): stub\n", pTcpRow);
  return 0;
}

/******************************************************************
 *    SetPerTcpConnectionEStats (IPHLPAPI.@)
 */
DWORD WINAPI SetPerTcpConnectionEStats(PMIB_TCPROW row, TCP_ESTATS_TYPE state, PBYTE rw,
                                       ULONG version, ULONG size, ULONG offset)
{
  FIXME("(row %p, state %d, rw %p, version %u, size %u, offset %u): stub\n",
        row, state, rw, version, size, offset);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    UnenableRouter (IPHLPAPI.@)
 *
 * Decrement the IP-forwarding reference count. Turn off IP-forwarding
 * if it reaches zero.
 *
 * PARAMS
 *  pOverlapped     [In/Out] should be the same as in EnableRouter()
 *  lpdwEnableCount [Out]    optional, receives reference count
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI UnenableRouter(OVERLAPPED * pOverlapped, LPDWORD lpdwEnableCount)
{
  FIXME("(pOverlapped %p, lpdwEnableCount %p): stub\n", pOverlapped,
   lpdwEnableCount);
  /* could echo "0" > /proc/net/sys/net/ipv4/ip_forward, not sure I want to
     could map EACCESS to ERROR_ACCESS_DENIED, I suppose
   */
  return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    PfCreateInterface (IPHLPAPI.@)
 */
DWORD WINAPI PfCreateInterface(DWORD dwName, PFFORWARD_ACTION inAction, PFFORWARD_ACTION outAction,
        BOOL bUseLog, BOOL bMustBeUnique, INTERFACE_HANDLE *ppInterface)
{
    FIXME("(%d %d %d %x %x %p) stub\n", dwName, inAction, outAction, bUseLog, bMustBeUnique, ppInterface);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *    PfUnBindInterface (IPHLPAPI.@)
 */
DWORD WINAPI PfUnBindInterface(INTERFACE_HANDLE interface)
{
    FIXME("(%p) stub\n", interface);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *   PfDeleteInterface(IPHLPAPI.@)
 */
DWORD WINAPI PfDeleteInterface(INTERFACE_HANDLE interface)
{
    FIXME("(%p) stub\n", interface);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *   PfBindInterfaceToIPAddress(IPHLPAPI.@)
 */
DWORD WINAPI PfBindInterfaceToIPAddress(INTERFACE_HANDLE interface, PFADDRESSTYPE type, PBYTE ip)
{
    FIXME("(%p %d %p) stub\n", interface, type, ip);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 *    GetTcpTable2 (IPHLPAPI.@)
 */
ULONG WINAPI GetTcpTable2(PMIB_TCPTABLE2 table, PULONG size, BOOL order)
{
    FIXME("pTcpTable2 %p, pdwSize %p, bOrder %d: stub\n", table, size, order);
    return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    GetTcp6Table (IPHLPAPI.@)
 */
ULONG WINAPI GetTcp6Table(PMIB_TCP6TABLE table, PULONG size, BOOL order)
{
    TRACE("(table %p, size %p, order %d)\n", table, size, order);
    return GetExtendedTcpTable(table, size, order, WS_AF_INET6, TCP_TABLE_BASIC_ALL, 0);
}

/******************************************************************
 *    GetTcp6Table2 (IPHLPAPI.@)
 */
ULONG WINAPI GetTcp6Table2(PMIB_TCP6TABLE2 table, PULONG size, BOOL order)
{
    FIXME("pTcp6Table2 %p, size %p, order %d: stub\n", table, size, order);
    return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    ConvertInterfaceGuidToLuid (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceGuidToLuid(const GUID *guid, NET_LUID *luid)
{
    DWORD ret;
    MIB_IFROW row;

    TRACE("(%s %p)\n", debugstr_guid(guid), luid);

    if (!guid || !luid) return ERROR_INVALID_PARAMETER;

    row.dwIndex = guid->Data1;
    if ((ret = GetIfEntry( &row ))) return ret;

    luid->Info.Reserved     = 0;
    luid->Info.NetLuidIndex = guid->Data1;
    luid->Info.IfType       = row.dwType;
    return NO_ERROR;
}

/******************************************************************
 *    ConvertInterfaceIndexToLuid (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceIndexToLuid(NET_IFINDEX index, NET_LUID *luid)
{
    MIB_IFROW row;

    TRACE("(%u %p)\n", index, luid);

    if (!luid) return ERROR_INVALID_PARAMETER;
    memset( luid, 0, sizeof(*luid) );

    row.dwIndex = index;
    if (GetIfEntry( &row )) return ERROR_FILE_NOT_FOUND;

    luid->Info.Reserved     = 0;
    luid->Info.NetLuidIndex = index;
    luid->Info.IfType       = row.dwType;
    return NO_ERROR;
}

/******************************************************************
 *    ConvertInterfaceLuidToGuid (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceLuidToGuid(const NET_LUID *luid, GUID *guid)
{
    DWORD ret;
    MIB_IFROW row;

    TRACE("(%p %p)\n", luid, guid);

    if (!luid || !guid) return ERROR_INVALID_PARAMETER;

    row.dwIndex = luid->Info.NetLuidIndex;
    if ((ret = GetIfEntry( &row ))) return ret;

    memset( guid, 0, sizeof(*guid) );
    guid->Data1 = luid->Info.NetLuidIndex;
    memcpy( guid->Data4+2, "NetDev", 6 );
    return NO_ERROR;
}

/******************************************************************
 *    ConvertInterfaceLuidToIndex (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceLuidToIndex(const NET_LUID *luid, NET_IFINDEX *index)
{
    DWORD ret;
    MIB_IFROW row;

    TRACE("(%p %p)\n", luid, index);

    if (!luid || !index) return ERROR_INVALID_PARAMETER;

    row.dwIndex = luid->Info.NetLuidIndex;
    if ((ret = GetIfEntry( &row ))) return ret;

    *index = luid->Info.NetLuidIndex;
    return NO_ERROR;
}

/******************************************************************
 *    ConvertInterfaceLuidToNameA (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceLuidToNameA(const NET_LUID *luid, char *name, SIZE_T len)
{
    DWORD ret;
    MIB_IFROW row;

    TRACE("(%p %p %u)\n", luid, name, (DWORD)len);

    if (!luid) return ERROR_INVALID_PARAMETER;

    row.dwIndex = luid->Info.NetLuidIndex;
    if ((ret = GetIfEntry( &row ))) return ret;

    if (!name || len < WideCharToMultiByte( CP_UNIXCP, 0, row.wszName, -1, NULL, 0, NULL, NULL ))
        return ERROR_NOT_ENOUGH_MEMORY;

    WideCharToMultiByte( CP_UNIXCP, 0, row.wszName, -1, name, len, NULL, NULL );
    return NO_ERROR;
}

/******************************************************************
 *    ConvertInterfaceLuidToNameW (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceLuidToNameW(const NET_LUID *luid, WCHAR *name, SIZE_T len)
{
    DWORD ret;
    MIB_IFROW row;

    TRACE("(%p %p %u)\n", luid, name, (DWORD)len);

    if (!luid || !name) return ERROR_INVALID_PARAMETER;

    row.dwIndex = luid->Info.NetLuidIndex;
    if ((ret = GetIfEntry( &row ))) return ret;

    if (len < strlenW( row.wszName ) + 1) return ERROR_NOT_ENOUGH_MEMORY;
    strcpyW( name, row.wszName );
    return NO_ERROR;
}

/******************************************************************
 *    ConvertInterfaceNameToLuidA (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceNameToLuidA(const char *name, NET_LUID *luid)
{
    DWORD ret;
    IF_INDEX index;
    MIB_IFROW row;

    TRACE("(%s %p)\n", debugstr_a(name), luid);

    if ((ret = getInterfaceIndexByName( name, &index ))) return ERROR_INVALID_NAME;
    if (!luid) return ERROR_INVALID_PARAMETER;

    row.dwIndex = index;
    if ((ret = GetIfEntry( &row ))) return ret;

    luid->Info.Reserved     = 0;
    luid->Info.NetLuidIndex = index;
    luid->Info.IfType       = row.dwType;
    return NO_ERROR;
}

/******************************************************************
 *    ConvertInterfaceNameToLuidW (IPHLPAPI.@)
 */
DWORD WINAPI ConvertInterfaceNameToLuidW(const WCHAR *name, NET_LUID *luid)
{
    DWORD ret;
    IF_INDEX index;
    MIB_IFROW row;
    char nameA[IF_MAX_STRING_SIZE + 1];

    TRACE("(%s %p)\n", debugstr_w(name), luid);

    if (!luid) return ERROR_INVALID_PARAMETER;
    memset( luid, 0, sizeof(*luid) );

    if (!WideCharToMultiByte( CP_UNIXCP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL ))
        return ERROR_INVALID_NAME;

    if ((ret = getInterfaceIndexByName( nameA, &index ))) return ret;

    row.dwIndex = index;
    if ((ret = GetIfEntry( &row ))) return ret;

    luid->Info.Reserved     = 0;
    luid->Info.NetLuidIndex = index;
    luid->Info.IfType       = row.dwType;
    return NO_ERROR;
}

/******************************************************************
 *    ConvertLengthToIpv4Mask (IPHLPAPI.@)
 */
DWORD WINAPI ConvertLengthToIpv4Mask(ULONG mask_len, ULONG *mask)
{
    if (mask_len > 32)
    {
        *mask = INADDR_NONE;
        return ERROR_INVALID_PARAMETER;
    }

    if (mask_len == 0)
        *mask = 0;
    else
        *mask = htonl(~0u << (32 - mask_len));

    return NO_ERROR;
}

/******************************************************************
 *    if_nametoindex (IPHLPAPI.@)
 */
IF_INDEX WINAPI IPHLP_if_nametoindex(const char *name)
{
    IF_INDEX idx;

    TRACE("(%s)\n", name);
    if (getInterfaceIndexByName(name, &idx) == NO_ERROR)
        return idx;

    return 0;
}

/******************************************************************
 *    if_indextoname (IPHLPAPI.@)
 */
PCHAR WINAPI IPHLP_if_indextoname(NET_IFINDEX index, PCHAR name)
{
    TRACE("(%u, %p)\n", index, name);

    return getInterfaceNameByIndex(index, name);
}

/******************************************************************
 *    GetIpForwardTable2 (IPHLPAPI.@)
 */
DWORD WINAPI GetIpForwardTable2(ADDRESS_FAMILY family, PMIB_IPFORWARD_TABLE2 *table)
{
    static int once;

    if (!once++) FIXME("(%u %p): stub\n", family, table);
    return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    GetIpNetTable2 (IPHLPAPI.@)
 */
DWORD WINAPI GetIpNetTable2(ADDRESS_FAMILY family, PMIB_IPNET_TABLE2 *table)
{
    static int once;

    if (!once++) FIXME("(%u %p): stub\n", family, table);
    return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    GetIpInterfaceTable (IPHLPAPI.@)
 */
DWORD WINAPI GetIpInterfaceTable(ADDRESS_FAMILY family, PMIB_IPINTERFACE_TABLE *table)
{
    FIXME("(%u %p): stub\n", family, table);
    return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    GetBestRoute2 (IPHLPAPI.@)
 */
DWORD WINAPI GetBestRoute2(NET_LUID *luid, NET_IFINDEX index,
                           const SOCKADDR_INET *source, const SOCKADDR_INET *destination,
                           ULONG options, PMIB_IPFORWARD_ROW2 bestroute,
                           SOCKADDR_INET *bestaddress)
{
    static int once;

    if (!once++)
        FIXME("(%p, %d, %p, %p, 0x%08x, %p, %p): stub\n", luid, index, source,
                destination, options, bestroute, bestaddress);

    if (!destination || !bestroute || !bestaddress)
        return ERROR_INVALID_PARAMETER;

    return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    ParseNetworkString (IPHLPAPI.@)
 */
DWORD WINAPI ParseNetworkString(const WCHAR *str, DWORD type,
                                NET_ADDRESS_INFO *info, USHORT *port, BYTE *prefix_len)
{
    IN_ADDR temp_addr4;
    IN6_ADDR temp_addr6;
    ULONG temp_scope;
    USHORT temp_port = 0;
    NTSTATUS status;

    TRACE("(%s, %d, %p, %p, %p)\n", debugstr_w(str), type, info, port, prefix_len);

    if (!str)
        return ERROR_INVALID_PARAMETER;

    if (type & NET_STRING_IPV4_ADDRESS)
    {
        status = RtlIpv4StringToAddressExW(str, TRUE, &temp_addr4, &temp_port);
        if (SUCCEEDED(status) && !temp_port)
        {
            if (info)
            {
                info->Format = NET_ADDRESS_IPV4;
                info->u.Ipv4Address.sin_addr = temp_addr4;
                info->u.Ipv4Address.sin_port = 0;
            }
            if (port) *port = 0;
            if (prefix_len) *prefix_len = 255;
            return ERROR_SUCCESS;
        }
    }
    if (type & NET_STRING_IPV4_SERVICE)
    {
        status = RtlIpv4StringToAddressExW(str, TRUE, &temp_addr4, &temp_port);
        if (SUCCEEDED(status) && temp_port)
        {
            if (info)
            {
                info->Format = NET_ADDRESS_IPV4;
                info->u.Ipv4Address.sin_addr = temp_addr4;
                info->u.Ipv4Address.sin_port = temp_port;
            }
            if (port) *port = ntohs(temp_port);
            if (prefix_len) *prefix_len = 255;
            return ERROR_SUCCESS;
        }
    }
    if (type & NET_STRING_IPV6_ADDRESS)
    {
        status = RtlIpv6StringToAddressExW(str, &temp_addr6, &temp_scope, &temp_port);
        if (SUCCEEDED(status) && !temp_port)
        {
            if (info)
            {
                info->Format = NET_ADDRESS_IPV6;
                info->u.Ipv6Address.sin6_addr = temp_addr6;
                info->u.Ipv6Address.sin6_scope_id = temp_scope;
                info->u.Ipv6Address.sin6_port = 0;
            }
            if (port) *port = 0;
            if (prefix_len) *prefix_len = 255;
            return ERROR_SUCCESS;
        }
    }
    if (type & NET_STRING_IPV6_SERVICE)
    {
        status = RtlIpv6StringToAddressExW(str, &temp_addr6, &temp_scope, &temp_port);
        if (SUCCEEDED(status) && temp_port)
        {
            if (info)
            {
                info->Format = NET_ADDRESS_IPV6;
                info->u.Ipv6Address.sin6_addr = temp_addr6;
                info->u.Ipv6Address.sin6_scope_id = temp_scope;
                info->u.Ipv6Address.sin6_port = temp_port;
            }
            if (port) *port = ntohs(temp_port);
            if (prefix_len) *prefix_len = 255;
            return ERROR_SUCCESS;
        }
    }

    if (info) info->Format = NET_ADDRESS_FORMAT_UNSPECIFIED;

    if (type & ~(NET_STRING_IPV4_ADDRESS|NET_STRING_IPV4_SERVICE|NET_STRING_IPV6_ADDRESS|NET_STRING_IPV6_SERVICE))
    {
        FIXME("Unimplemented type 0x%x\n", type);
        return ERROR_NOT_SUPPORTED;
    }

    return ERROR_INVALID_PARAMETER;
}
