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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "iphlpapi.h"
#include "ifenum.h"
#include "ipstats.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

#ifndef INADDR_NONE
#define INADDR_NONE ~0UL
#endif

static int resolver_initialised;

/* call res_init() just once because of a bug in Mac OS X 10.4 */
static void initialise_resolver(void)
{
    if (!resolver_initialised)
    {
        res_init();
        resolver_initialised = 1;
    }
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls( hinstDLL );
      break;

    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}

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


static int IpAddrTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((const MIB_IPADDRROW*)a)->dwAddr - ((const MIB_IPADDRROW*)b)->dwAddr;
  else
    ret = 0;
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
     sizeof(MIB_IPADDRROW), IpAddrTableSorter);
  TRACE("returning %d\n", ret);
  return ret;
}


static int IpForwardTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
   const MIB_IPFORWARDROW* rowA = (const MIB_IPFORWARDROW*)a;
   const MIB_IPFORWARDROW* rowB = (const MIB_IPFORWARDROW*)b;

    ret = rowA->dwForwardDest - rowB->dwForwardDest;
    if (ret == 0) {
      ret = rowA->dwForwardProto - rowB->dwForwardProto;
      if (ret == 0) {
        ret = rowA->dwForwardPolicy - rowB->dwForwardPolicy;
        if (ret == 0)
          ret = rowA->dwForwardNextHop - rowB->dwForwardNextHop;
      }
    }
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpForwardTableFromStack (IPHLPAPI.@)
 *
 * Get the route table.
 * Like GetIpForwardTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppIpForwardTable [Out] pointer into which the MIB_IPFORWARDTABLE is
 *                         allocated and returned.
 *  bOrder           [In]  whether to sort the table
 *  heap             [In]  heap from which the table is allocated
 *  flags            [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppIfTable is NULL, other error codes
 *  on failure, NO_ERROR on success.
 */
DWORD WINAPI AllocateAndGetIpForwardTableFromStack(PMIB_IPFORWARDTABLE *
 ppIpForwardTable, BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpForwardTable %p, bOrder %d, heap %p, flags 0x%08x\n",
   ppIpForwardTable, bOrder, heap, flags);
  ret = getRouteTable(ppIpForwardTable, heap, flags);
  if (!ret && bOrder)
    qsort((*ppIpForwardTable)->table, (*ppIpForwardTable)->dwNumEntries,
     sizeof(MIB_IPFORWARDROW), IpForwardTableSorter);
  TRACE("returning %d\n", ret);
  return ret;
}


static int IpNetTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((const MIB_IPNETROW*)a)->dwAddr - ((const MIB_IPNETROW*)b)->dwAddr;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpNetTableFromStack (IPHLPAPI.@)
 *
 * Get the IP-to-physical address mapping table.
 * Like GetIpNetTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppIpNetTable [Out] pointer into which the MIB_IPNETTABLE is
 *                     allocated and returned.
 *  bOrder       [In]  whether to sort the table
 *  heap         [In]  heap from which the table is allocated
 *  flags        [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppIpNetTable is NULL, other error codes
 *  on failure, NO_ERROR on success.
 */
DWORD WINAPI AllocateAndGetIpNetTableFromStack(PMIB_IPNETTABLE *ppIpNetTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpNetTable %p, bOrder %d, heap %p, flags 0x%08x\n",
   ppIpNetTable, bOrder, heap, flags);
  ret = getArpTable(ppIpNetTable, heap, flags);
  if (!ret && bOrder)
    qsort((*ppIpNetTable)->table, (*ppIpNetTable)->dwNumEntries,
     sizeof(MIB_IPADDRROW), IpNetTableSorter);
  TRACE("returning %d\n", ret);
  return ret;
}


static int TcpTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    const MIB_TCPROW* rowA = a;
    const MIB_TCPROW* rowB = b;

    ret = ntohl (rowA->dwLocalAddr) - ntohl (rowB->dwLocalAddr);
    if (ret == 0) {
       ret = ntohs ((unsigned short)rowA->dwLocalPort) -
          ntohs ((unsigned short)rowB->dwLocalPort);
      if (ret == 0) {
         ret = ntohl (rowA->dwRemoteAddr) - ntohl (rowB->dwRemoteAddr);
        if (ret == 0)
           ret = ntohs ((unsigned short)rowA->dwRemotePort) -
              ntohs ((unsigned short)rowB->dwRemotePort);
      }
    }
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    AllocateAndGetTcpTableFromStack (IPHLPAPI.@)
 *
 * Get the TCP connection table.
 * Like GetTcpTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppTcpTable [Out] pointer into which the MIB_TCPTABLE is
 *                   allocated and returned.
 *  bOrder     [In]  whether to sort the table
 *  heap       [In]  heap from which the table is allocated
 *  flags      [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppTcpTable is NULL, whatever GetTcpTable()
 *  returns otherwise.
 */
DWORD WINAPI AllocateAndGetTcpTableFromStack(PMIB_TCPTABLE *ppTcpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppTcpTable %p, bOrder %d, heap %p, flags 0x%08x\n",
   ppTcpTable, bOrder, heap, flags);

  *ppTcpTable = NULL;
  ret = getTcpTable(ppTcpTable, 0, heap, flags);
  if (!ret && bOrder)
    qsort((*ppTcpTable)->table, (*ppTcpTable)->dwNumEntries,
     sizeof(MIB_TCPROW), TcpTableSorter);
  TRACE("returning %d\n", ret);
  return ret;
}


static int UdpTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    const MIB_UDPROW* rowA = (const MIB_UDPROW*)a;
    const MIB_UDPROW* rowB = (const MIB_UDPROW*)b;

    ret = rowA->dwLocalAddr - rowB->dwLocalAddr;
    if (ret == 0)
      ret = rowA->dwLocalPort - rowB->dwLocalPort;
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    AllocateAndGetUdpTableFromStack (IPHLPAPI.@)
 *
 * Get the UDP listener table.
 * Like GetUdpTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppUdpTable [Out] pointer into which the MIB_UDPTABLE is
 *                   allocated and returned.
 *  bOrder     [In]  whether to sort the table
 *  heap       [In]  heap from which the table is allocated
 *  flags      [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppUdpTable is NULL, whatever GetUdpTable()
 *  returns otherwise.
 */
DWORD WINAPI AllocateAndGetUdpTableFromStack(PMIB_UDPTABLE *ppUdpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppUdpTable %p, bOrder %d, heap %p, flags 0x%08x\n",
   ppUdpTable, bOrder, heap, flags);
  ret = getUdpTable(ppUdpTable, heap, flags);
  if (!ret && bOrder)
    qsort((*ppUdpTable)->table, (*ppUdpTable)->dwNumEntries,
     sizeof(MIB_UDPROW), UdpTableSorter);
  TRACE("returning %d\n", ret);
  return ret;
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
  return (DWORD) 0;
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
  return (DWORD) 0;
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
  return (DWORD) 0;
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
  return (DWORD) 0;
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
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI GetAdapterIndex(LPWSTR AdapterName, PULONG IfIndex)
{
  FIXME("(AdapterName %p, IfIndex %p): stub\n", AdapterName, IfIndex);
  /* FIXME: implement using getInterfaceIndexByName */
  return ERROR_NOT_SUPPORTED;
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
    DWORD numNonLoopbackInterfaces = getNumNonLoopbackInterfaces();

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

        ret = getIPAddrTable(&ipAddrTable, GetProcessHeap(), 0);
        if (!ret)
          table = getNonLoopbackInterfaceIndexTable();
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
              DWORD addrLen = sizeof(ptr->Address), type, i;
              PIP_ADDR_STRING currentIPAddr = &ptr->IpAddressList;
              BOOL firstIPAddr = TRUE;

              /* on Win98 this is left empty, but whatever */
              getInterfaceNameByIndex(table->indexes[ndx], ptr->AdapterName);
              getInterfacePhysicalByIndex(table->indexes[ndx], &addrLen,
               ptr->Address, &type);
              /* MS defines address length and type as UINT in some places and
                 DWORD in others, **sigh**.  Don't want to assume that PUINT and
                 PDWORD are equiv (64-bit?) */
              ptr->AddressLength = addrLen;
              ptr->Type = type;
              ptr->Index = table->indexes[ndx];
              for (i = 0; i < ipAddrTable->dwNumEntries; i++) {
                if (ipAddrTable->table[i].dwIndex == ptr->Index) {
                  if (firstIPAddr) {
                    toIPAddressString(ipAddrTable->table[i].dwAddr,
                     ptr->IpAddressList.IpAddress.String);
                    toIPAddressString(ipAddrTable->table[i].dwMask,
                     ptr->IpAddressList.IpMask.String);
                    firstIPAddr = FALSE;
                  }
                  else {
                    currentIPAddr->Next = nextIPAddr;
                    currentIPAddr = nextIPAddr;
                    toIPAddressString(ipAddrTable->table[i].dwAddr,
                     currentIPAddr->IpAddress.String);
                    toIPAddressString(ipAddrTable->table[i].dwMask,
                     currentIPAddr->IpMask.String);
                    nextIPAddr++;
                  }
                }
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
            }
            ret = NO_ERROR;
          }
          HeapFree(GetProcessHeap(), 0, table);
        }
        else
          ret = ERROR_OUTOFMEMORY;
        HeapFree(GetProcessHeap(), 0, ipAddrTable);
      }
    }
    else
      ret = ERROR_NO_DATA;
  }
  TRACE("returning %d\n", ret);
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
  DWORD ret;

  TRACE("dwDestAddr 0x%08lx, pdwBestIfIndex %p\n", dwDestAddr, pdwBestIfIndex);
  if (!pdwBestIfIndex)
    ret = ERROR_INVALID_PARAMETER;
  else {
    MIB_IPFORWARDROW ipRow;

    ret = GetBestRoute(dwDestAddr, 0, &ipRow);
    if (ret == ERROR_SUCCESS)
      *pdwBestIfIndex = ipRow.dwForwardIfIndex;
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

  AllocateAndGetIpForwardTableFromStack(&table, FALSE, GetProcessHeap(), 0);
  if (table) {
    DWORD ndx, matchedBits, matchedNdx = 0;

    for (ndx = 0, matchedBits = 0; ndx < table->dwNumEntries; ndx++) {
      if (table->table[ndx].dwForwardType != MIB_IPROUTE_TYPE_INVALID &&
       (dwDestAddr & table->table[ndx].dwForwardMask) ==
       (table->table[ndx].dwForwardDest & table->table[ndx].dwForwardMask)) {
        DWORD numShifts, mask;

        for (numShifts = 0, mask = table->table[ndx].dwForwardMask;
         mask && !(mask & 1); mask >>= 1, numShifts++)
          ;
        if (numShifts > matchedBits) {
          matchedBits = numShifts;
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
  else
    ret = ERROR_OUTOFMEMORY;
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
 *    GetIcmpStatistics (IPHLPAPI.@)
 *
 * Get the ICMP statistics for the local computer.
 *
 * PARAMS
 *  pStats [Out] buffer for ICMP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetIcmpStatistics(PMIB_ICMP pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getICMPStats(pStats);
  TRACE("returning %d\n", ret);
  return ret;
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

  TRACE("pIfTable %p, pdwSize %p, bOrder %d\n", pdwSize, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numInterfaces = getNumInterfaces();
    ULONG size = sizeof(MIB_IFTABLE) + (numInterfaces - 1) * sizeof(MIB_IFROW);

    if (!pIfTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table = getInterfaceIndexTable();

      if (table) {
        size = sizeof(MIB_IFTABLE) + (table->numIndexes - 1) *
         sizeof(MIB_IFROW);
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
    DWORD numInterfaces = getNumInterfaces();
    ULONG size = sizeof(IP_INTERFACE_INFO) + (numInterfaces - 1) *
     sizeof(IP_ADAPTER_INDEX_MAP);

    if (!pIfTable || *dwOutBufLen < size) {
      *dwOutBufLen = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table = getInterfaceIndexTable();

      if (table) {
        size = sizeof(IP_INTERFACE_INFO) + (table->numIndexes - 1) *
         sizeof(IP_ADAPTER_INDEX_MAP);
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
      ULONG size = sizeof(MIB_IPADDRTABLE) + (table->dwNumEntries - 1) *
       sizeof(MIB_IPADDRROW);

      if (!pIpAddrTable || *pdwSize < size) {
        *pdwSize = size;
        ret = ERROR_INSUFFICIENT_BUFFER;
      }
      else {
        *pdwSize = size;
        memcpy(pIpAddrTable, table, sizeof(MIB_IPADDRTABLE) +
         (table->dwNumEntries - 1) * sizeof(MIB_IPADDRROW));
        if (bOrder)
          qsort(pIpAddrTable->table, pIpAddrTable->dwNumEntries,
           sizeof(MIB_IPADDRROW), IpAddrTableSorter);
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

  TRACE("pIpForwardTable %p, pdwSize %p, bOrder %d\n", pIpForwardTable,
   pdwSize, (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numRoutes = getNumRoutes();
    ULONG sizeNeeded = sizeof(MIB_IPFORWARDTABLE) + (numRoutes - 1) *
     sizeof(MIB_IPFORWARDROW);

    if (!pIpForwardTable || *pdwSize < sizeNeeded) {
      *pdwSize = sizeNeeded;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_IPFORWARDTABLE table;

      ret = getRouteTable(&table, GetProcessHeap(), 0);
      if (!ret) {
        sizeNeeded = sizeof(MIB_IPFORWARDTABLE) + (table->dwNumEntries - 1) *
         sizeof(MIB_IPFORWARDROW);
        if (*pdwSize < sizeNeeded) {
          *pdwSize = sizeNeeded;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          *pdwSize = sizeNeeded;
          memcpy(pIpForwardTable, table, sizeNeeded);
          if (bOrder)
            qsort(pIpForwardTable->table, pIpForwardTable->dwNumEntries,
             sizeof(MIB_IPFORWARDROW), IpForwardTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
    }
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

  TRACE("pIpNetTable %p, pdwSize %p, bOrder %d\n", pIpNetTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumArpEntries();
    ULONG size = sizeof(MIB_IPNETTABLE) + (numEntries - 1) *
     sizeof(MIB_IPNETROW);

    if (!pIpNetTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_IPNETTABLE table;

      ret = getArpTable(&table, GetProcessHeap(), 0);
      if (!ret) {
        size = sizeof(MIB_IPNETTABLE) + (table->dwNumEntries - 1) *
         sizeof(MIB_IPNETROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          *pdwSize = size;
          memcpy(pIpNetTable, table, size);
          if (bOrder)
            qsort(pIpNetTable->table, pIpNetTable->dwNumEntries,
             sizeof(MIB_IPNETROW), IpNetTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetIpStatistics (IPHLPAPI.@)
 *
 * Get the IP statistics for the local computer.
 *
 * PARAMS
 *  pStats [Out] buffer for IP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetIpStatistics(PMIB_IPSTATS pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getIPStats(pStats);
  TRACE("returning %d\n", ret);
  return ret;
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
  DWORD ret, size;
  LONG regReturn;
  HKEY hKey;

  TRACE("pFixedInfo %p, pOutBufLen %p\n", pFixedInfo, pOutBufLen);
  if (!pOutBufLen)
    return ERROR_INVALID_PARAMETER;

  initialise_resolver();
  size = sizeof(FIXED_INFO) + (_res.nscount > 0 ? (_res.nscount  - 1) *
   sizeof(IP_ADDR_STRING) : 0);
  if (!pFixedInfo || *pOutBufLen < size) {
    *pOutBufLen = size;
    return ERROR_BUFFER_OVERFLOW;
  }

  memset(pFixedInfo, 0, size);
  size = sizeof(pFixedInfo->HostName);
  GetComputerNameExA(ComputerNameDnsHostname, pFixedInfo->HostName, &size);
  size = sizeof(pFixedInfo->DomainName);
  GetComputerNameExA(ComputerNameDnsDomain, pFixedInfo->DomainName, &size);
  if (_res.nscount > 0) {
    PIP_ADDR_STRING ptr;
    int i;

    for (i = 0, ptr = &pFixedInfo->DnsServerList; i < _res.nscount && ptr;
     i++, ptr = ptr->Next) {
      toIPAddressString(_res.nsaddr_list[i].sin_addr.s_addr,
       ptr->IpAddress.String);
      if (i == _res.nscount - 1)
        ptr->Next = NULL;
      else if (i == 0)
        ptr->Next = (PIP_ADDR_STRING)((LPBYTE)pFixedInfo + sizeof(FIXED_INFO));
      else
        ptr->Next = (PIP_ADDR_STRING)((PBYTE)ptr + sizeof(IP_ADDR_STRING));
    }
  }
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
    *pdwNumIf = getNumInterfaces();
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
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI GetPerAdapterInfo(ULONG IfIndex, PIP_PER_ADAPTER_INFO pPerAdapterInfo, PULONG pOutBufLen)
{
  TRACE("(IfIndex %d, pPerAdapterInfo %p, pOutBufLen %p)\n", IfIndex,
   pPerAdapterInfo, pOutBufLen);
  return ERROR_NOT_SUPPORTED;
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
  FIXME("(DestIpAddress 0x%08lx, HopCount %p, MaxHops %d, RTT %p): stub\n",
   DestIpAddress, HopCount, MaxHops, RTT);
  return FALSE;
}


/******************************************************************
 *    GetTcpStatistics (IPHLPAPI.@)
 *
 * Get the TCP statistics for the local computer.
 *
 * PARAMS
 *  pStats [Out] buffer for TCP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetTcpStatistics(PMIB_TCPSTATS pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getTCPStats(pStats);
  TRACE("returning %d\n", ret);
  return ret;
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
  DWORD ret;

  TRACE("pTcpTable %p, pdwSize %p, bOrder %d\n", pTcpTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumTcpEntries();
    DWORD size = sizeof(MIB_TCPTABLE) + (numEntries - 1) * sizeof(MIB_TCPROW);

    if (!pTcpTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      ret = getTcpTable(&pTcpTable, numEntries, 0, 0);
      if (!ret) {
        size = sizeof(MIB_TCPTABLE) + (pTcpTable->dwNumEntries - 1) *
         sizeof(MIB_TCPROW);
        *pdwSize = size;

          if (bOrder)
             qsort(pTcpTable->table, pTcpTable->dwNumEntries,
                   sizeof(MIB_TCPROW), TcpTableSorter);
          ret = NO_ERROR;
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetUdpStatistics (IPHLPAPI.@)
 *
 * Get the UDP statistics for the local computer.
 *
 * PARAMS
 *  pStats [Out] buffer for UDP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetUdpStatistics(PMIB_UDPSTATS pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getUDPStats(pStats);
  TRACE("returning %d\n", ret);
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
  DWORD ret;

  TRACE("pUdpTable %p, pdwSize %p, bOrder %d\n", pUdpTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumUdpEntries();
    DWORD size = sizeof(MIB_UDPTABLE) + (numEntries - 1) * sizeof(MIB_UDPROW);

    if (!pUdpTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_UDPTABLE table;

      ret = getUdpTable(&table, GetProcessHeap(), 0);
      if (!ret) {
        size = sizeof(MIB_UDPTABLE) + (table->dwNumEntries - 1) *
         sizeof(MIB_UDPROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          *pdwSize = size;
          memcpy(pUdpTable, table, size);
          if (bOrder)
            qsort(pUdpTable->table, pUdpTable->dwNumEntries,
             sizeof(MIB_UDPROW), UdpTableSorter);
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
 * Release an IP optained through DHCP,
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
  TRACE("AdapterInfo %p\n", AdapterInfo);
  /* not a stub, never going to support this (and I never mark an adapter as
     DHCP enabled, see GetAdaptersInfo, so this should never get called) */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    IpRenewAddress (IPHLPAPI.@)
 *
 * Renew an IP optained through DHCP.
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
  TRACE("AdapterInfo %p\n", AdapterInfo);
  /* not a stub, never going to support this (and I never mark an adapter as
     DHCP enabled, see GetAdaptersInfo, so this should never get called) */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    NotifyAddrChange (IPHLPAPI.@)
 *
 * Notify caller whenever the ip-interface map is changed.
 *
 * PARAMS
 *  Handle     [Out] handle useable in asynchronus notification
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
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    NotifyRouteChange (IPHLPAPI.@)
 *
 * Notify caller whenever the ip routing table is changed.
 *
 * PARAMS
 *  Handle     [Out] handle useable in asynchronus notification
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
  FIXME("(DestIP 0x%08lx, SrcIP 0x%08lx, pMacAddr %p, PhyAddrLen %p): stub\n",
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
  return (DWORD) 0;
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
  return (DWORD) 0;
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
  return (DWORD) 0;
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
  return (DWORD) 0;
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
  return (DWORD) 0;
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
