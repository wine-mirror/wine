/*
 * iphlpapi dll implementation
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls( hinstDLL );
      interfaceMapInit();
      break;

    case DLL_PROCESS_DETACH:
      interfaceMapFree();
      break;
  }
  return TRUE;
}

/******************************************************************
 *    AddIPAddress (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  Address [In]
 *  IpMask [In]
 *  IfIndex [In]
 *  NTEContext [In/Out]
 *  NTEInstance [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI AddIPAddress(IPAddr Address, IPMask IpMask, DWORD IfIndex, PULONG NTEContext, PULONG NTEInstance)
{
  FIXME(":stub\n");
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    AllocateAndGetIfTableFromStack (IPHLPAPI.@)
 *
 * NOTES
 * Like GetIfTable, but allocates the returned table from heap.
 *
 * PARAMS
 *
 *  ppIfTable [Out] -- pointer into which the MIB_IFTABLE is
 *   allocated and returned.
 *  bOrder [In] -- passed to GetIfTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS -- ERROR_INVALID_PARAMETER if ppIfTable is NULL, whatever
 *  GetIfTable returns otherwise
 *
 */
DWORD WINAPI AllocateAndGetIfTableFromStack(PMIB_IFTABLE *ppIfTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIfTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n", ppIfTable,
   (DWORD)bOrder, (DWORD)heap, flags);
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
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpAddrTableFromStack (IPHLPAPI.@)
 *
 * NOTES
 * Like GetIpAddrTable, but allocates the returned table from heap.
 *
 * PARAMS
 *
 *  ppIpAddrTable [Out]
 *  bOrder [In] -- passed to GetIpAddrTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 * ERROR_INVALID_PARAMETER if ppIpAddrTable is NULL, whatever GetIpAddrTable
 * returns otherwise.
 *
 */
DWORD WINAPI AllocateAndGetIpAddrTableFromStack(PMIB_IPADDRTABLE *ppIpAddrTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpAddrTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppIpAddrTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppIpAddrTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIpAddrTable(*ppIpAddrTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIpAddrTable = HeapAlloc(heap, flags, dwSize);
      ret = GetIpAddrTable(*ppIpAddrTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpForwardTableFromStack (IPHLPAPI.@)
 *
 * NOTES
 * Like GetIpForwardTable, but allocates the returned table from heap.
 *
 * PARAMS
 *
 *  ppIpForwardTable [Out] -- pointer into which the MIB_IPFORWARDTABLE is
 *   allocated and returned.
 *  bOrder [In] -- passed to GetIfTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS -- ERROR_INVALID_PARAMETER if ppIfTable is NULL, whatever
 *  GetIpForwardTable returns otherwise
 *
 */
DWORD WINAPI AllocateAndGetIpForwardTableFromStack(PMIB_IPFORWARDTABLE *
 ppIpForwardTable, BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpForwardTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppIpForwardTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppIpForwardTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIpForwardTable(*ppIpForwardTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIpForwardTable = HeapAlloc(heap, flags, dwSize);
      ret = GetIpForwardTable(*ppIpForwardTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpNetTableFromStack (IPHLPAPI.@)
 *
 * NOTES
 * Like GetIpNetTable, but allocates the returned table from heap.
 *
 * PARAMS
 *
 *  ppIpNetTable [Out]
 *  bOrder [In] -- passed to GetIpNetTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 * ERROR_INVALID_PARAMETER if ppIpNetTable is NULL, whatever GetIpNetTable
 * returns otherwise.
 *
 */
DWORD WINAPI AllocateAndGetIpNetTableFromStack(PMIB_IPNETTABLE *ppIpNetTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpNetTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppIpNetTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppIpNetTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIpNetTable(*ppIpNetTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIpNetTable = HeapAlloc(heap, flags, dwSize);
      ret = GetIpNetTable(*ppIpNetTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetTcpTableFromStack (IPHLPAPI.@)
 *
 * NOTES
 * Like GetTcpTable, but allocates the returned table from heap.
 *
 * PARAMS
 *
 *  ppTcpTable [Out]
 *  bOrder [In] -- passed to GetTcpTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 * ERROR_INVALID_PARAMETER if ppTcpTable is NULL, whatever GetTcpTable
 * returns otherwise.
 *
 */
DWORD WINAPI AllocateAndGetTcpTableFromStack(PMIB_TCPTABLE *ppTcpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppTcpTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppTcpTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppTcpTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetTcpTable(*ppTcpTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppTcpTable = HeapAlloc(heap, flags, dwSize);
      ret = GetTcpTable(*ppTcpTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetUdpTableFromStack (IPHLPAPI.@)
 *
 * NOTES
 * Like GetUdpTable, but allocates the returned table from heap.
 *
 * PARAMS
 *
 *  ppUdpTable [Out]
 *  bOrder [In] -- passed to GetUdpTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 * ERROR_INVALID_PARAMETER if ppUdpTable is NULL, whatever GetUdpTable
 * returns otherwise.
 *
 */
DWORD WINAPI AllocateAndGetUdpTableFromStack(PMIB_UDPTABLE *ppUdpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppUdpTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppUdpTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppUdpTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetUdpTable(*ppUdpTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppUdpTable = HeapAlloc(heap, flags, dwSize);
      ret = GetUdpTable(*ppUdpTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    CreateIpForwardEntry (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pRoute [In/Out]
 *
 * RETURNS
 * 0
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pArpEntry [In/Out]
 *
 * RETURNS
 * 0
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  dwAddress [In]
 *  dwMask [In]
 *  dwIfIndex [In]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI CreateProxyArpEntry(DWORD dwAddress, DWORD dwMask, DWORD dwIfIndex)
{
  FIXME("(dwAddress 0x%08lx, dwMask 0x%08lx, dwIfIndex 0x%08lx): stub\n",
   dwAddress, dwMask, dwIfIndex);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    DeleteIPAddress (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  NTEContext [In]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI DeleteIPAddress(ULONG NTEContext)
{
  FIXME("(NTEContext %ld): stub\n", NTEContext);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    DeleteIpForwardEntry (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pRoute [In/Out]
 *
 * RETURNS
 * 0
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pArpEntry [In/Out]
 *
 * RETURNS
 * 0
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  dwAddress [In]
 *  dwMask [In]
 *  dwIfIndex [In]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI DeleteProxyArpEntry(DWORD dwAddress, DWORD dwMask, DWORD dwIfIndex)
{
  FIXME("(dwAddress 0x%08lx, dwMask 0x%08lx, dwIfIndex 0x%08lx): stub\n",
   dwAddress, dwMask, dwIfIndex);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    EnableRouter (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pHandle [In/Out]
 *  pOverlapped [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI EnableRouter(HANDLE * pHandle, OVERLAPPED * pOverlapped)
{
  FIXME("(pHandle %p, pOverlapped %p): stub\n", pHandle, pOverlapped);
  /* could echo "1" > /proc/net/sys/net/ipv4/ip_forward, not sure I want to
     could map EACCESS to ERROR_ACCESS_DENIED, I suppose
     marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    FlushIpNetTable (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  dwIfIndex [In]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI FlushIpNetTable(DWORD dwIfIndex)
{
  FIXME("(dwIfIndex 0x%08lx): stub\n", dwIfIndex);
  /* this flushes the arp cache of the given index
     marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    GetAdapterIndex (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  AdapterName [In/Out]
 *  IfIndex [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI GetAdapterIndex(LPWSTR AdapterName, PULONG IfIndex)
{
  FIXME("(AdapterName %p, IfIndex %p): stub\n", AdapterName, IfIndex);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    GetAdaptersInfo (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pAdapterInfo [In/Out]
 *  pOutBufLen [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
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
      /* this calculation assumes only one address in the IP_ADDR_STRING lists.
         that's okay, because:
         - we don't get multiple addresses per adapter anyway
         - we don't know about per-adapter gateways
         - DHCP and WINS servers can have max one entry per list */
      ULONG size = sizeof(IP_ADAPTER_INFO) * numNonLoopbackInterfaces;

      if (!pAdapterInfo || *pOutBufLen < size) {
        *pOutBufLen = size;
        ret = ERROR_BUFFER_OVERFLOW;
      }
      else {
        InterfaceIndexTable *table = getNonLoopbackInterfaceIndexTable();

        if (table) {
          size = sizeof(IP_ADAPTER_INFO) * table->numIndexes;
          if (*pOutBufLen < size) {
            *pOutBufLen = size;
            ret = ERROR_INSUFFICIENT_BUFFER;
          }
          else {
            DWORD ndx;
            HKEY hKey;
            BOOL winsEnabled = FALSE;
            IP_ADDRESS_STRING primaryWINS, secondaryWINS;

            memset(pAdapterInfo, 0, size);
            /* @@ Wine registry key: HKCU\Software\Wine\Network */
            if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Network", &hKey) == ERROR_SUCCESS) {
              DWORD size = sizeof(primaryWINS.String);
              unsigned long addr;

              RegQueryValueExA(hKey, "WinsServer", NULL, NULL,
               primaryWINS.String, &size);
              addr = inet_addr(primaryWINS.String);
              if (addr != INADDR_NONE && addr != INADDR_ANY)
                winsEnabled = TRUE;
              size = sizeof(secondaryWINS.String);
              RegQueryValueExA(hKey, "BackupWinsServer", NULL, NULL,
               secondaryWINS.String, &size);
              addr = inet_addr(secondaryWINS.String);
              if (addr != INADDR_NONE && addr != INADDR_ANY)
                winsEnabled = TRUE;
              RegCloseKey(hKey);
            }
            for (ndx = 0; ndx < table->numIndexes; ndx++) {
              PIP_ADAPTER_INFO ptr = &pAdapterInfo[ndx];
              DWORD addrLen = sizeof(ptr->Address), type;

              /* on Win98 this is left empty, but whatever */
              lstrcpynA(ptr->AdapterName,
               getInterfaceNameByIndex(table->indexes[ndx]),
               MAX_ADAPTER_NAME_LENGTH+1);
              getInterfacePhysicalByIndex(table->indexes[ndx], &addrLen,
               ptr->Address, &type);
              /* MS defines address length and type as UINT in some places and
                 DWORD in others, **sigh**.  Don't want to assume that PUINT and
                 PDWORD are equiv (64-bit?) */
              ptr->AddressLength = addrLen;
              ptr->Type = type;
              ptr->Index = table->indexes[ndx];
              toIPAddressString(getInterfaceIPAddrByIndex(table->indexes[ndx]),
               ptr->IpAddressList.IpAddress.String);
              toIPAddressString(getInterfaceMaskByIndex(table->indexes[ndx]),
               ptr->IpAddressList.IpMask.String);
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
      }
    }
    else
      ret = ERROR_NO_DATA;
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetBestInterface (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  dwDestAddr [In]
 *  pdwBestIfIndex [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
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
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetBestRoute (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  dwDestAddr [In]
 *  dwSourceAddr [In]
 *  OUT [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetBestRoute(DWORD dwDestAddr, DWORD dwSourceAddr, PMIB_IPFORWARDROW pBestRoute)
{
  PMIB_IPFORWARDTABLE table;
  DWORD ret;

  TRACE("dwDestAddr 0x%08lx, dwSourceAddr 0x%08lx, pBestRoute %p\n", dwDestAddr,
   dwSourceAddr, pBestRoute);
  if (!pBestRoute)
    return ERROR_INVALID_PARAMETER;

  AllocateAndGetIpForwardTableFromStack(&table, FALSE, GetProcessHeap(), 0);
  if (table) {
    DWORD ndx, matchedBits, matchedNdx = 0;

    for (ndx = 0, matchedBits = 0; ndx < table->dwNumEntries; ndx++) {
      if ((dwDestAddr & table->table[ndx].dwForwardMask) ==
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
    memcpy(pBestRoute, &table->table[matchedNdx], sizeof(MIB_IPFORWARDROW));
    HeapFree(GetProcessHeap(), 0, table);
    ret = ERROR_SUCCESS;
  }
  else
    ret = ERROR_OUTOFMEMORY;
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetFriendlyIfIndex (IPHLPAPI.@)
 *
 * NOTES
 * Returns a "friendly" version if IfIndex, which is one that doesn't
 * have the top byte set.  Doesn't validate whether IfIndex is a valid
 * adapter index.
 *
 * PARAMS
 *
 *  IfIndex [In]
 *
 * RETURNS
 * A friendly version of IfIndex.
 *
 */
DWORD WINAPI GetFriendlyIfIndex(DWORD IfIndex)
{
  /* windows doesn't validate these, either, just makes sure the top byte is
     cleared.  I assume my ifenum module never gives an index with the top
     byte set. */
  TRACE("returning %ld\n", IfIndex);
  return IfIndex;
}


/******************************************************************
 *    GetIcmpStatistics (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIcmpStatistics(PMIB_ICMP pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getICMPStats(pStats);
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetIfEntry (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pIfRow [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIfEntry(PMIB_IFROW pIfRow)
{
  DWORD ret;
  const char *name;

  TRACE("pIfRow %p\n", pIfRow);
  if (!pIfRow)
    return ERROR_INVALID_PARAMETER;

  name = getInterfaceNameByIndex(pIfRow->dwIndex);
  if (name) {
    ret = getInterfaceEntryByName(name, pIfRow);
    if (ret == NO_ERROR)
      ret = getInterfaceStatsByName(name, pIfRow);
  }
  else
    ret = ERROR_INVALID_DATA;
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IfTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((PMIB_IFROW)a)->dwIndex - ((PMIB_IFROW)b)->dwIndex;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIfTable (IPHLPAPI.@)
 *
 * NOTES
 * Returns a table of local interfaces, *pdwSize is the size in bytes of
 * pIfTable.  If this is less than required, the function will return
 * ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 * size.
 * If bOrder is true, the returned table will be sorted by interface index.
 *
 * PARAMS
 *
 *  pIfTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 * NO_ERROR on success, something else on failure.
 *
 */
DWORD WINAPI GetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIfTable %p, pdwSize %p, bOrder %ld\n", pdwSize, pdwSize,
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
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetInterfaceInfo (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pIfTable [In/Out]
 *  dwOutBufLen [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
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

          pIfTable->NumAdapters = 0;
          for (ndx = 0; ndx < table->numIndexes; ndx++) {
            const char *walker, *name;
            WCHAR *assigner;

            pIfTable->Adapter[ndx].Index = table->indexes[ndx];
            name = getInterfaceNameByIndex(table->indexes[ndx]);
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
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IpAddrTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((PMIB_IPADDRROW)a)->dwAddr - ((PMIB_IPADDRROW)b)->dwAddr;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIpAddrTable (IPHLPAPI.@)
 *
 * NOTES
 * Returns the route table.  On input, *pdwSize is the size in bytes of
 * pIpForwardTable.  If this is less than required, the function will return
 * ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 * size.
 * If bOrder is true, the returned table will be sorted by the next hop and
 * an assortment of arbitrary parameters.
 *
 * PARAMS
 *
 *  pIpAddrTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 * NO_ERROR on success, something else on failure.
 *
 */
DWORD WINAPI GetIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIpAddrTable %p, pdwSize %p, bOrder %ld\n", pIpAddrTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numInterfaces = getNumInterfaces();
    ULONG size = sizeof(MIB_IPADDRTABLE) + (numInterfaces - 1) *
     sizeof(MIB_IPADDRROW);

    if (!pIpAddrTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table = getInterfaceIndexTable();

      if (table) {
        size = sizeof(MIB_IPADDRTABLE) + (table->numIndexes - 1) *
         sizeof(MIB_IPADDRROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx, bcast;

          pIpAddrTable->dwNumEntries = 0;
          for (ndx = 0; ndx < table->numIndexes; ndx++) {
            pIpAddrTable->table[ndx].dwIndex = table->indexes[ndx];
            pIpAddrTable->table[ndx].dwAddr =
             getInterfaceIPAddrByIndex(table->indexes[ndx]);
            pIpAddrTable->table[ndx].dwMask =
             getInterfaceMaskByIndex(table->indexes[ndx]);
            /* the dwBCastAddr member isn't the broadcast address, it indicates
             * whether the interface uses the 1's broadcast address (1) or the
             * 0's broadcast address (0).
             */
            bcast = getInterfaceBCastAddrByIndex(table->indexes[ndx]);
            pIpAddrTable->table[ndx].dwBCastAddr =
             (bcast & pIpAddrTable->table[ndx].dwMask) ? 1 : 0;
            /* FIXME: hardcoded reasm size, not sure where to get it */
            pIpAddrTable->table[ndx].dwReasmSize = 65535;
            pIpAddrTable->table[ndx].unused1 = 0;
            pIpAddrTable->table[ndx].wType = 0; /* aka unused2 */
            pIpAddrTable->dwNumEntries++;
          }
          if (bOrder)
            qsort(pIpAddrTable->table, pIpAddrTable->dwNumEntries,
             sizeof(MIB_IPADDRROW), IpAddrTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IpForwardTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    PMIB_IPFORWARDROW rowA = (PMIB_IPFORWARDROW)a, rowB = (PMIB_IPFORWARDROW)b;

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
 *    GetIpForwardTable (IPHLPAPI.@)
 *
 * NOTES
 * Returns the route table.  On input, *pdwSize is the size in bytes of
 * pIpForwardTable.  If this is less than required, the function will return
 * ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 * size.
 * If bOrder is true, the returned table will be sorted by the next hop and
 * an assortment of arbitrary parameters.
 *
 * PARAMS
 *
 *  pIpForwardTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 * NO_ERROR on success, something else on failure.
 *
 */
DWORD WINAPI GetIpForwardTable(PMIB_IPFORWARDTABLE pIpForwardTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIpForwardTable %p, pdwSize %p, bOrder %ld\n", pIpForwardTable,
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
      RouteTable *table = getRouteTable();
      if (table) {
        sizeNeeded = sizeof(MIB_IPFORWARDTABLE) + (table->numRoutes - 1) *
         sizeof(MIB_IPFORWARDROW);
        if (*pdwSize < sizeNeeded) {
          *pdwSize = sizeNeeded;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx;

          pIpForwardTable->dwNumEntries = table->numRoutes;
          for (ndx = 0; ndx < numRoutes; ndx++) {
            pIpForwardTable->table[ndx].dwForwardIfIndex =
             table->routes[ndx].ifIndex;
            pIpForwardTable->table[ndx].dwForwardDest =
             table->routes[ndx].dest;
            pIpForwardTable->table[ndx].dwForwardMask =
             table->routes[ndx].mask;
            pIpForwardTable->table[ndx].dwForwardPolicy = 0;
            pIpForwardTable->table[ndx].dwForwardNextHop =
             table->routes[ndx].gateway;
            /* FIXME: this type is appropriate for local interfaces; may not
               always be appropriate */
            pIpForwardTable->table[ndx].dwForwardType = MIB_IPROUTE_TYPE_DIRECT;
            /* FIXME: other protos might be appropriate, e.g. the default route
               is typically set with MIB_IPPROTO_NETMGMT instead */
            pIpForwardTable->table[ndx].dwForwardProto = MIB_IPPROTO_LOCAL;
            /* punt on age and AS */
            pIpForwardTable->table[ndx].dwForwardAge = 0;
            pIpForwardTable->table[ndx].dwForwardNextHopAS = 0;
            pIpForwardTable->table[ndx].dwForwardMetric1 =
             table->routes[ndx].metric;
            /* rest of the metrics are 0.. */
            pIpForwardTable->table[ndx].dwForwardMetric2 = 0;
            pIpForwardTable->table[ndx].dwForwardMetric3 = 0;
            pIpForwardTable->table[ndx].dwForwardMetric4 = 0;
            pIpForwardTable->table[ndx].dwForwardMetric5 = 0;
          }
          if (bOrder)
            qsort(pIpForwardTable->table, pIpForwardTable->dwNumEntries,
             sizeof(MIB_IPFORWARDROW), IpForwardTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IpNetTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((PMIB_IPNETROW)a)->dwAddr - ((PMIB_IPNETROW)b)->dwAddr;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIpNetTable (IPHLPAPI.@)
 *
 * Returns the ARP cache.  On input, *pdwSize is the size in bytes of
 * pIpNetTable.  If this is less than required, the function will return
 * ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to the required byte
 * size.
 * If bOrder is true, the returned table will be sorted by IP address.
 *
 * PARAMS
 *
 *  pIpNetTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 * NO_ERROR on success, something else on failure.
 *
 */
DWORD WINAPI GetIpNetTable(PMIB_IPNETTABLE pIpNetTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIpNetTable %p, pdwSize %p, bOrder %ld\n", pIpNetTable, pdwSize,
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
      PMIB_IPNETTABLE table = getArpTable();

      if (table) {
        size = sizeof(MIB_IPNETTABLE) + (table->dwNumEntries - 1) *
         sizeof(MIB_IPNETROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          memcpy(pIpNetTable, table, size);
          if (bOrder)
            qsort(pIpNetTable->table, pIpNetTable->dwNumEntries,
             sizeof(MIB_IPNETROW), IpNetTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetIpStatistics (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIpStatistics(PMIB_IPSTATS pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getIPStats(pStats);
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetNetworkParams (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pFixedInfo [In/Out]
 *  pOutBufLen [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetNetworkParams(PFIXED_INFO pFixedInfo, PULONG pOutBufLen)
{
  DWORD ret, size;
  LONG regReturn;
  HKEY hKey;

  TRACE("pFixedInfo %p, pOutBufLen %p\n", pFixedInfo, pOutBufLen);
  if (!pOutBufLen)
    return ERROR_INVALID_PARAMETER;

  res_init();
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

    RegQueryValueExA(hKey, "ScopeID", NULL, NULL, pFixedInfo->ScopeId, &size);
    RegCloseKey(hKey);
  }

  /* FIXME: can check whether routing's enabled in /proc/sys/net/ipv4/ip_forward
     I suppose could also check for a listener on port 53 to set EnableDns */
  ret = NO_ERROR;
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetNumberOfInterfaces (IPHLPAPI.@)
 *
 * NOTES
 * Returns the number of interfaces in *pdwNumIf.
 *
 * PARAMS
 *
 *  pdwNumIf [In/Out]
 *
 * RETURNS
 * NO_ERROR on success, ERROR_INVALID_PARAMETER if pdwNumIf is NULL.
 *
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
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetPerAdapterInfo (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  IfIndex [In]
 *  pPerAdapterInfo [In/Out]
 *  pOutBufLen [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 */
DWORD WINAPI GetPerAdapterInfo(ULONG IfIndex, PIP_PER_ADAPTER_INFO pPerAdapterInfo, PULONG pOutBufLen)
{
  TRACE("(IfIndex %ld, pPerAdapterInfo %p, pOutBufLen %p)\n", IfIndex,
   pPerAdapterInfo, pOutBufLen);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    GetRTTAndHopCount (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  DestIpAddress [In]
 *  HopCount [In/Out]
 *  MaxHops [In]
 *  RTT [In/Out]
 *
 * RETURNS
 * FALSE
 *
 */
BOOL WINAPI GetRTTAndHopCount(IPAddr DestIpAddress, PULONG HopCount, ULONG MaxHops, PULONG RTT)
{
  FIXME("(DestIpAddress 0x%08lx, HopCount %p, MaxHops %ld, RTT %p): stub\n",
   DestIpAddress, HopCount, MaxHops, RTT);
  return FALSE;
}


/******************************************************************
 *    GetTcpStatistics (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetTcpStatistics(PMIB_TCPSTATS pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getTCPStats(pStats);
  TRACE("returning %ld\n", ret);
  return ret;
}


static int TcpTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    PMIB_TCPROW rowA = (PMIB_TCPROW)a, rowB = (PMIB_TCPROW)b;

    ret = rowA->dwLocalAddr - rowB->dwLocalAddr;
    if (ret == 0) {
      ret = rowA->dwLocalPort - rowB->dwLocalPort;
      if (ret == 0) {
        ret = rowA->dwRemoteAddr - rowB->dwRemoteAddr;
        if (ret == 0)
          ret = rowA->dwRemotePort - rowB->dwRemotePort;
      }
    }
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetTcpTable (IPHLPAPI.@)
 *
 * Returns a table of active TCP connections.  On input, *pdwSize
 * is the size in bytes of pTcpTable.  If this is less than required,
 * the function will return ERROR_INSUFFICIENT_BUFFER, and *pdwSize
 * will be set to the required byte size.
 * If bOrder is true, the returned table will be sorted, first by
 * local address and port number, then by remote address and port
 * number.
 *
 * PARAMS
 *
 *  pTcpTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 * NO_ERROR on success, something else on failure.
 *
 */
DWORD WINAPI GetTcpTable(PMIB_TCPTABLE pTcpTable, PDWORD pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pTcpTable %p, pdwSize %p, bOrder %ld\n", pTcpTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumTcpEntries();
    ULONG size = sizeof(MIB_TCPTABLE) + (numEntries - 1) * sizeof(MIB_TCPROW);

    if (!pTcpTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_TCPTABLE table = getTcpTable();

      if (table) {
        size = sizeof(MIB_TCPTABLE) + (table->dwNumEntries - 1) *
         sizeof(MIB_TCPROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          memcpy(pTcpTable, table, size);
          if (bOrder)
            qsort(pTcpTable->table, pTcpTable->dwNumEntries,
             sizeof(MIB_TCPROW), TcpTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetUdpStatistics (IPHLPAPI.@)
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetUdpStatistics(PMIB_UDPSTATS pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getUDPStats(pStats);
  TRACE("returning %ld\n", ret);
  return ret;
}


static int UdpTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    PMIB_UDPROW rowA = (PMIB_UDPROW)a, rowB = (PMIB_UDPROW)b;

    ret = rowA->dwLocalAddr - rowB->dwLocalAddr;
    if (ret == 0)
      ret = rowA->dwLocalPort - rowB->dwLocalPort;
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetUdpTable (IPHLPAPI.@)
 *
 * Returns a table of active UDP connections.  On input, *pdwSize
 * is the size in bytes of pUdpTable.  If this is less than required,
 * the function will return ERROR_INSUFFICIENT_BUFFER, and *pdwSize
 * will be set to the required byte size.
 * If bOrder is true, the returned table will be sorted, first by
 * local address, then by local port number.
 *
 * PARAMS
 *
 *  pUdpTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 * NO_ERROR on success, something else on failure.
 *
 */
DWORD WINAPI GetUdpTable(PMIB_UDPTABLE pUdpTable, PDWORD pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pUdpTable %p, pdwSize %p, bOrder %ld\n", pUdpTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumUdpEntries();
    ULONG size = sizeof(MIB_UDPTABLE) + (numEntries - 1) * sizeof(MIB_UDPROW);

    if (!pUdpTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_UDPTABLE table = getUdpTable();

      if (table) {
        size = sizeof(MIB_UDPTABLE) + (table->dwNumEntries - 1) *
         sizeof(MIB_UDPROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
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
  TRACE("returning %ld\n", ret);
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
 *
 *  pIPIfInfo [In/Out]
 *  dwOutBufLen [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
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
 * NOTES
 * Since GetAdaptersInfo never returns adapters that have DHCP enabled,
 * this function does nothing.
 *
 * PARAMS
 *
 *  AdapterInfo [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
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
 * NOTES
 * Since GetAdaptersInfo never returns adapters that have DHCP enabled,
 * this function does nothing.
 *
 * PARAMS
 *
 *  AdapterInfo [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  Handle [In/Out]
 *  overlapped [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI NotifyAddrChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
  FIXME("(Handle %p, overlapped %p): stub\n", Handle, overlapped);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    NotifyRouteChange (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  Handle [In/Out]
 *  overlapped [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI NotifyRouteChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
  FIXME("(Handle %p, overlapped %p): stub\n", Handle, overlapped);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SendARP (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  DestIP [In]
 *  SrcIP [In]
 *  pMacAddr [In/Out]
 *  PhyAddrLen [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI SendARP(IPAddr DestIP, IPAddr SrcIP, PULONG pMacAddr, PULONG PhyAddrLen)
{
  FIXME("(DestIP 0x%08lx, SrcIP 0x%08lx, pMacAddr %p, PhyAddrLen %p): stub\n",
   DestIP, SrcIP, pMacAddr, PhyAddrLen);
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SetIfEntry (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pIfRow [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI SetIfEntry(PMIB_IFROW pIfRow)
{
  FIXME("(pIfRow %p): stub\n", pIfRow);
  /* this is supposed to set an administratively interface up or down.
     Could do SIOCSIFFLAGS and set/clear IFF_UP, but, not sure I want to, and
     this sort of down is indistinguishable from other sorts of down (e.g. no
     link). */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SetIpForwardEntry (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pRoute [In/Out]
 *
 * RETURNS
 * 0
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pArpEntry [In/Out]
 *
 * RETURNS
 * 0
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pIpStats [In/Out]
 *
 * RETURNS
 * 0
 *
 */
DWORD WINAPI SetIpStatistics(PMIB_IPSTATS pIpStats)
{
  FIXME("(pIpStats %p): stub\n", pIpStats);
  return (DWORD) 0;
}


/******************************************************************
 *    SetIpTTL (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  nTTL [In]
 *
 * RETURNS
 * 0
 *
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
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pTcpRow [In/Out]
 *
 * RETURNS
 * 0
 *
 */
DWORD WINAPI SetTcpEntry(PMIB_TCPROW pTcpRow)
{
  FIXME("(pTcpRow %p): stub\n", pTcpRow);
  return (DWORD) 0;
}


/******************************************************************
 *    UnenableRouter (IPHLPAPI.@)
 *
 * NOTES
 * Stub
 *
 * PARAMS
 *
 *  pOverlapped [In/Out]
 *  lpdwEnableCount [In/Out]
 *
 * RETURNS
 * ERROR_NOT_SUPPORTED
 *
 */
DWORD WINAPI UnenableRouter(OVERLAPPED * pOverlapped, LPDWORD lpdwEnableCount)
{
  FIXME("(pOverlapped %p, lpdwEnableCount %p): stub\n", pOverlapped,
   lpdwEnableCount);
  /* could echo "0" > /proc/net/sys/net/ipv4/ip_forward, not sure I want to
     could map EACCESS to ERROR_ACCESS_DENIED, I suppose
     marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}
