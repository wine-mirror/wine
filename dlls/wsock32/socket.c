/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 2003 Juan Lang.
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
#include "winsock2.h"
#include "winnt.h"
#include "wscontrol.h"
#include "iphlpapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

/* internal remapper function for the IP_ constants */
static INT _remap_optname(INT level, INT optname)
{
  TRACE("level=%d, optname=%d\n", level, optname);
  if (level == IPPROTO_IP) {
    switch (optname) {       /***** from value *****/
      case 2: return 9;      /* IP_MULTICAST_IF    */
      case 3: return 10;     /* IP_MULTICAST_TTL   */
      case 4: return 11;     /* IP_MULTICAST_LOOP  */
      case 5: return 12;     /* IP_ADD_MEMBERSHIP  */
      case 6: return 13;     /* IP_DROP_MEMBERSHIP */
      case 7: return 4;      /* IP_TTL             */
      case 8: return 3;      /* IP_TOS             */
      case 9: return 14;     /* IP_DONTFRAGMENT    */
      default: FIXME("Unknown optname %d, can't remap!\n", optname); return optname; 
    }
  } else {
    /* don't need to do anything */
    return optname;
  }
}

/***********************************************************************
 *		setsockopt		(WSOCK32.21)
 *
 * We have these forwarders because, for reasons unknown to us mere mortals,
 * the values of the IP_ constants changed between winsock.h and winsock2.h.
 * So, we need to remap them here.
 */
INT WINAPI WS1_setsockopt(SOCKET s, INT level, INT optname, char *optval, INT optlen)
{
  return setsockopt(s, level, _remap_optname(level, optname), optval, optlen);
}

/***********************************************************************
 *		getsockopt		(WSOCK32.7)
 */
INT WINAPI WS1_getsockopt(SOCKET s, INT level, INT optname, char *optval, INT *optlen)
{
  return getsockopt(s, level, _remap_optname(level, optname), optval, optlen);
}

/***********************************************************************
 *		WsControl (WSOCK32.1001)
 *
 * WsControl seems to be an undocumented Win95 function. A lot of
 * discussion about WsControl can be found on the net, e.g.
 * Subject:      Re: WSOCK32.DLL WsControl Exported Function
 * From:         "Peter Rindfuss" <rindfuss-s@medea.wz-berlin.de>
 * Date:         1997/08/17
 *
 * The WSCNTL_TCPIP_QUERY_INFO option is partially implemented based
 * on observing the behaviour of WsControl with an app in
 * Windows 98.  It is not fully implemented, and there could
 * be (are?) errors due to incorrect assumptions made.
 *
 *
 * WsControl returns WSCTL_SUCCESS on success.
 * ERROR_LOCK_VIOLATION is returned if the output buffer length
 * (*pcbResponseInfoLen) is too small.  This is an unusual error code, but
 * it matches Win98's behavior.  Other errors come from winerror.h, not from
 * winsock.h.  Again, this is to match Win98 behavior.
 *
 */

DWORD WINAPI WsControl(DWORD protocol,
                       DWORD action,
                       LPVOID pRequestInfo,
                       LPDWORD pcbRequestInfoLen,
                       LPVOID pResponseInfo,
                       LPDWORD pcbResponseInfoLen)
{

   /* Get the command structure into a pointer we can use,
      rather than void */
   TDIObjectID *pcommand = pRequestInfo;

   /* validate input parameters.  Error codes are from winerror.h, not from
    * winsock.h.  pcbResponseInfoLen is apparently allowed to be NULL for some
    * commands, since winipcfg.exe fails if we ensure it's non-NULL in every
    * case.
    */
   if (protocol != IPPROTO_TCP) return ERROR_INVALID_PARAMETER;
   if (!pcommand) return ERROR_INVALID_PARAMETER;
   if (!pcbRequestInfoLen) return ERROR_INVALID_ACCESS;
   if (*pcbRequestInfoLen < sizeof(TDIObjectID)) return ERROR_INVALID_ACCESS;
   if (!pResponseInfo) return ERROR_INVALID_PARAMETER;
   if (pcommand->toi_type != INFO_TYPE_PROVIDER) return ERROR_INVALID_PARAMETER;

   TRACE ("   WsControl TOI_ID=>0x%lx<, {TEI_ENTITY=0x%lx, TEI_INSTANCE=0x%lx}, TOI_CLASS=0x%lx, TOI_TYPE=0x%lx\n",
      pcommand->toi_id, pcommand->toi_entity.tei_entity,
      pcommand->toi_entity.tei_instance,
      pcommand->toi_class, pcommand->toi_type );

   switch (action)
   {
   case WSCNTL_TCPIP_QUERY_INFO:
   {
      if (pcommand->toi_class != INFO_CLASS_GENERIC &&
       pcommand->toi_class != INFO_CLASS_PROTOCOL)
      {
         ERR("Unexpected class %ld for WSCNTL_TCPIP_QUERY_INFO\n",
          pcommand->toi_class);
         return ERROR_BAD_ENVIRONMENT;
      }

      switch (pcommand->toi_id)
      {
         /* ENTITY_LIST_ID gets the list of "entity IDs", where an entity
            may represent an interface, or a datagram service, or address
            translation, or other fun things.  Typically an entity ID represents
            a class of service, which is further queried for what type it is.
            Different types will then have more specific queries defined.
         */
         case ENTITY_LIST_ID:
         {
            TDIEntityID *baseptr = pResponseInfo;
            DWORD numInt, i, ifTable, spaceNeeded;
            PMIB_IFTABLE table;

            if (!pcbResponseInfoLen)
               return ERROR_BAD_ENVIRONMENT;
            if (pcommand->toi_class != INFO_CLASS_GENERIC)
            {
               FIXME ("Unexpected Option for ENTITY_LIST_ID request -> toi_class=0x%lx\n",
                    pcommand->toi_class);
               return (ERROR_BAD_ENVIRONMENT);
            }

            GetNumberOfInterfaces(&numInt);
            spaceNeeded = sizeof(TDIEntityID) * (numInt * 2 + 3);

            if (*pcbResponseInfoLen < spaceNeeded)
               return (ERROR_LOCK_VIOLATION);

            ifTable = 0;
            GetIfTable(NULL, &ifTable, FALSE);
            table = HeapAlloc( GetProcessHeap(), 0, ifTable );
            if (!table)
               return ERROR_NOT_ENOUGH_MEMORY;
            GetIfTable(table, &ifTable, FALSE);

            spaceNeeded = sizeof(TDIEntityID) * (table->dwNumEntries + 4);
            if (*pcbResponseInfoLen < spaceNeeded)
            {
               HeapFree( GetProcessHeap(), 0, table );
               return ERROR_LOCK_VIOLATION;
            }

            memset(baseptr, 0, spaceNeeded);

            for (i = 0; i < table->dwNumEntries; i++)
            {
               /* Return IF_GENERIC and CL_NL_ENTITY on every interface, and
                * AT_ENTITY, CL_TL_ENTITY, and CO_TL_ENTITY on the first
                * interface.  MS returns them only on the loopback interface,
                * but it doesn't seem to matter.
                */
               if (i == 0)
               {
                  baseptr->tei_entity = CO_TL_ENTITY;
                  baseptr->tei_instance = table->table[i].dwIndex;
                  baseptr++;
                  baseptr->tei_entity = CL_TL_ENTITY;
                  baseptr->tei_instance = table->table[i].dwIndex;
                  baseptr++;
                  baseptr->tei_entity = AT_ENTITY;
                  baseptr->tei_instance = table->table[i].dwIndex;
                  baseptr++;
               }
               baseptr->tei_entity = CL_NL_ENTITY;
               baseptr->tei_instance = table->table[i].dwIndex;
               baseptr++;
               baseptr->tei_entity = IF_GENERIC;
               baseptr->tei_instance = table->table[i].dwIndex;
               baseptr++;
            }

            *pcbResponseInfoLen = spaceNeeded;
            HeapFree( GetProcessHeap(), 0, table );
            break;
         }

         /* Returns MIB-II statistics for an interface */
         case ENTITY_TYPE_ID:
            switch (pcommand->toi_entity.tei_entity)
            {
            case IF_GENERIC:
               if (pcommand->toi_class == INFO_CLASS_GENERIC)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  *((ULONG *)pResponseInfo) = IF_MIB;
                  *pcbResponseInfoLen = sizeof(ULONG);
               }
               else if (pcommand->toi_class == INFO_CLASS_PROTOCOL)
               {
                  MIB_IFROW row;
                  DWORD index = pcommand->toi_entity.tei_instance, ret;
                  DWORD size = sizeof(row) - sizeof(row.wszName) -
                   sizeof(row.bDescr);

                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  if (*pcbResponseInfoLen < size)
                     return (ERROR_LOCK_VIOLATION);
                  row.dwIndex = index;
                  ret = GetIfEntry(&row);
                  if (ret != NO_ERROR)
                  {
                     /* FIXME: Win98's arp.exe insists on querying index 1 for
                      * its MIB-II stats, regardless of the tei_instances
                      * returned in the ENTITY_LIST query above.  If the query
                      * fails, arp.exe fails.  So, I do this hack return value
                      * if index is 1 and the query failed just to get arp.exe
                      * to continue.
                      */
                     if (index == 1)
                        return NO_ERROR;
                     ERR ("Error retrieving data for interface index %lu\n",
                      index);
                     return ret;
                  }
                  size = sizeof(row) - sizeof(row.wszName) -
                   sizeof(row.bDescr) + row.dwDescrLen;
                  if (*pcbResponseInfoLen < size)
                     return (ERROR_LOCK_VIOLATION);
                  memcpy(pResponseInfo, &row.dwIndex, size);
                  *pcbResponseInfoLen = size;
               }
               break;

            /* Returns address-translation related data.  In our case, this is
             * ARP.
             */
            case AT_ENTITY:
               if (pcommand->toi_class == INFO_CLASS_GENERIC)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  *((ULONG *)pResponseInfo) = AT_ARP;
                  *pcbResponseInfoLen = sizeof(ULONG);
               }
               else if (pcommand->toi_class == INFO_CLASS_PROTOCOL)
               {
                  PMIB_IPNETTABLE table;
                  DWORD size;
                  PULONG output = pResponseInfo;

                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  if (*pcbResponseInfoLen < sizeof(ULONG) * 2)
                     return (ERROR_LOCK_VIOLATION);
                  GetIpNetTable(NULL, &size, FALSE);
                  table = HeapAlloc( GetProcessHeap(), 0, size );
                  if (!table)
                     return ERROR_NOT_ENOUGH_MEMORY;
                  GetIpNetTable(table, &size, FALSE);
                  /* FIXME: I don't understand the meaning of the ARP output
                   * very well, but it seems to indicate how many ARP entries
                   * exist.  I don't know whether this should reflect the
                   * number per interface, as I'm only testing with a single
                   * interface.  So, I lie and say all ARP entries exist on
                   * a single interface--the first one that appears in the
                   * ARP table.
                   */
                  *(output++) = table->dwNumEntries;
                  *output = table->table[0].dwIndex;
                  HeapFree( GetProcessHeap(), 0, table );
                  *pcbResponseInfoLen = sizeof(ULONG) * 2;
               }
               break;

            /* Returns connectionless network layer statistics--in our case,
             * this is IP.
             */
            case CL_NL_ENTITY:
               if (pcommand->toi_class == INFO_CLASS_GENERIC)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  *((ULONG *)pResponseInfo) = CL_NL_IP;
                  *pcbResponseInfoLen = sizeof(ULONG);
               }
               else if (pcommand->toi_class == INFO_CLASS_PROTOCOL)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  if (*pcbResponseInfoLen < sizeof(MIB_IPSTATS))
                     return ERROR_LOCK_VIOLATION;
                  GetIpStatistics(pResponseInfo);

                  *pcbResponseInfoLen = sizeof(MIB_IPSTATS);
               }
               break;

            /* Returns connectionless transport layer statistics--in our case,
             * this is UDP.
             */
            case CL_TL_ENTITY:
               if (pcommand->toi_class == INFO_CLASS_GENERIC)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  *((ULONG *)pResponseInfo) = CL_TL_UDP;
                  *pcbResponseInfoLen = sizeof(ULONG);
               }
               else if (pcommand->toi_class == INFO_CLASS_PROTOCOL)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  if (*pcbResponseInfoLen < sizeof(MIB_UDPSTATS))
                     return ERROR_LOCK_VIOLATION;
                  GetUdpStatistics(pResponseInfo);
                  *pcbResponseInfoLen = sizeof(MIB_UDPSTATS);
               }
               break;

            /* Returns connection-oriented transport layer statistics--in our
             * case, this is TCP.
             */
            case CO_TL_ENTITY:
               if (pcommand->toi_class == INFO_CLASS_GENERIC)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  *((ULONG *)pResponseInfo) = CO_TL_TCP;
                  *pcbResponseInfoLen = sizeof(ULONG);
               }
               else if (pcommand->toi_class == INFO_CLASS_PROTOCOL)
               {
                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  if (*pcbResponseInfoLen < sizeof(MIB_TCPSTATS))
                     return ERROR_LOCK_VIOLATION;
                  GetTcpStatistics(pResponseInfo);
                  *pcbResponseInfoLen = sizeof(MIB_TCPSTATS);
               }
               break;

            default:
               ERR("Unknown entity %ld for ENTITY_TYPE_ID query\n",
                pcommand->toi_entity.tei_entity);
         }
         break;

         /* This call returns the IP address, subnet mask, and broadcast
          * address for an interface.  If there are multiple IP addresses for
          * the interface with the given index, returns the "first" one.
          */
         case IP_MIB_ADDRTABLE_ENTRY_ID:
         {
            DWORD index = pcommand->toi_entity.tei_instance;
            PMIB_IPADDRROW baseIPInfo = pResponseInfo;
            PMIB_IPADDRTABLE table;
            DWORD tableSize, i;

            if (!pcbResponseInfoLen)
               return ERROR_BAD_ENVIRONMENT;
            if (*pcbResponseInfoLen < sizeof(MIB_IPADDRROW))
               return (ERROR_LOCK_VIOLATION);

            /* get entire table, because there isn't an exported function that
               gets just one entry. */
            tableSize = 0;
            GetIpAddrTable(NULL, &tableSize, FALSE);
            table = HeapAlloc( GetProcessHeap(), 0, tableSize );
            if (!table)
               return ERROR_NOT_ENOUGH_MEMORY;
            GetIpAddrTable(table, &tableSize, FALSE);
            for (i = 0; i < table->dwNumEntries; i++)
            {
               if (table->table[i].dwIndex == index)
               {
                  TRACE("Found IP info for tei_instance 0x%lx:\n", index);
                  TRACE("IP 0x%08lx, mask 0x%08lx\n", table->table[i].dwAddr,
                   table->table[i].dwMask);
                  *baseIPInfo = table->table[i];
                  break;
               }
            }
            HeapFree( GetProcessHeap(), 0, table );

            *pcbResponseInfoLen = sizeof(MIB_IPADDRROW);
            break;
         }

         case IP_MIB_TABLE_ENTRY_ID:
         {
            switch (pcommand->toi_entity.tei_entity)
            {
            /* This call returns the routing table.
             * No official documentation found, even the name of the command is unknown.
             * Work is based on
             * http://www.cyberport.com/~tangent/programming/winsock/articles/wscontrol.html
             * and testings done with winipcfg.exe, route.exe and ipconfig.exe.
             * pcommand->toi_entity.tei_instance seems to be the interface number
             * but route.exe outputs only the information for the last interface
             * if only the routes for the pcommand->toi_entity.tei_instance
             * interface are returned. */
               case CL_NL_ENTITY:
               {
                  DWORD routeTableSize, numRoutes, ndx, ret;
                  PMIB_IPFORWARDTABLE table;
                  IPRouteEntry *winRouteTable  = pResponseInfo;

                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  ret = GetIpForwardTable(NULL, &routeTableSize, FALSE);
                  if (ret != ERROR_INSUFFICIENT_BUFFER)
                      return ret;
                  numRoutes = (routeTableSize - sizeof(MIB_IPFORWARDTABLE))
                   / sizeof(MIB_IPFORWARDROW) + 1;
                  if (*pcbResponseInfoLen < sizeof(IPRouteEntry) * numRoutes)
                     return (ERROR_LOCK_VIOLATION);
                  table = HeapAlloc( GetProcessHeap(), 0, routeTableSize );
                  if (!table)
                     return ERROR_NOT_ENOUGH_MEMORY;
                  ret = GetIpForwardTable(table, &routeTableSize, FALSE);
                  if (ret != NO_ERROR) {
                     HeapFree( GetProcessHeap(), 0, table );
                     return ret;
                  }

                  memset(pResponseInfo, 0, sizeof(IPRouteEntry) * numRoutes);
                  for (ndx = 0; ndx < table->dwNumEntries; ndx++)
                  {
                     winRouteTable->ire_addr = table->table[ndx].dwForwardDest;
                     winRouteTable->ire_index =
                      table->table[ndx].dwForwardIfIndex;
                     winRouteTable->ire_metric =
                      table->table[ndx].dwForwardMetric1;
                     /* winRouteTable->ire_option4 =
                     winRouteTable->ire_option5 =
                     winRouteTable->ire_option6 = */
                     winRouteTable->ire_gw = table->table[ndx].dwForwardNextHop;
                     /* winRouteTable->ire_option8 =
                     winRouteTable->ire_option9 =
                     winRouteTable->ire_option10 = */
                     winRouteTable->ire_mask = table->table[ndx].dwForwardMask;
                     /* winRouteTable->ire_option12 = */
                     winRouteTable++;
                  }

                  /* calculate the length of the data in the output buffer */
                  *pcbResponseInfoLen = sizeof(IPRouteEntry) *
                   table->dwNumEntries;

                  HeapFree( GetProcessHeap(), 0, table );
               }
               break;

               case AT_ARP:
               {
                  DWORD arpTableSize, numEntries, ret;
                  PMIB_IPNETTABLE table;

                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  ret = GetIpNetTable(NULL, &arpTableSize, FALSE);
                  if (ret != ERROR_INSUFFICIENT_BUFFER)
                      return ret;
                  numEntries = (arpTableSize - sizeof(MIB_IPNETTABLE))
                   / sizeof(MIB_IPNETROW) + 1;
                  if (*pcbResponseInfoLen < sizeof(MIB_IPNETROW) * numEntries)
                     return (ERROR_LOCK_VIOLATION);
                  table = HeapAlloc( GetProcessHeap(), 0, arpTableSize );
                  if (!table)
                     return ERROR_NOT_ENOUGH_MEMORY;
                  ret = GetIpNetTable(table, &arpTableSize, FALSE);
                  if (ret != NO_ERROR) {
                     HeapFree( GetProcessHeap(), 0, table );
                     return ret;
                  }
                  if (*pcbResponseInfoLen < sizeof(MIB_IPNETROW) *
                   table->dwNumEntries)
                  {
                     HeapFree( GetProcessHeap(), 0, table );
                     return ERROR_LOCK_VIOLATION;
                  }
                  memcpy(pResponseInfo, table->table, sizeof(MIB_IPNETROW) *
                   table->dwNumEntries);

                  /* calculate the length of the data in the output buffer */
                  *pcbResponseInfoLen = sizeof(MIB_IPNETROW) *
                   table->dwNumEntries;

                  HeapFree( GetProcessHeap(), 0, table );
               }
               break;

               case CO_TL_ENTITY:
               {
                  DWORD tcpTableSize, numEntries, ret;
                  PMIB_TCPTABLE table;
                  DWORD i;

                  if (!pcbResponseInfoLen)
                     return ERROR_BAD_ENVIRONMENT;
                  ret = GetTcpTable(NULL, &tcpTableSize, FALSE);
                  if (ret != ERROR_INSUFFICIENT_BUFFER)
                      return ret;
                  numEntries = (tcpTableSize - sizeof(MIB_TCPTABLE))
                   / sizeof(MIB_TCPROW) + 1;
                  if (*pcbResponseInfoLen < sizeof(MIB_TCPROW) * numEntries)
                     return (ERROR_LOCK_VIOLATION);
                  table = HeapAlloc( GetProcessHeap(), 0, tcpTableSize );
                  if (!table)
                     return ERROR_NOT_ENOUGH_MEMORY;
                  ret = GetTcpTable(table, &tcpTableSize, FALSE);
                  if (ret != NO_ERROR) {
                     HeapFree( GetProcessHeap(), 0, table );
                     return ret;
                  }
                  if (*pcbResponseInfoLen < sizeof(MIB_TCPROW) *
                   table->dwNumEntries)
                  {
                     HeapFree( GetProcessHeap(), 0, table );
                     return ERROR_LOCK_VIOLATION;
                  }
                  for (i = 0; i < table->dwNumEntries; i++)
                  {
                     USHORT sPort;

                     sPort = ntohs((USHORT)table->table[i].dwLocalPort);
                     table->table[i].dwLocalPort = (DWORD)sPort;
                     sPort = ntohs((USHORT)table->table[i].dwRemotePort);
                     table->table[i].dwRemotePort = (DWORD)sPort;
                  }
                  memcpy(pResponseInfo, table->table, sizeof(MIB_TCPROW) *
                   table->dwNumEntries);

                  /* calculate the length of the data in the output buffer */
                  *pcbResponseInfoLen = sizeof(MIB_TCPROW) *
                   table->dwNumEntries;

                  HeapFree( GetProcessHeap(), 0, table );
               }
               break;

               default:
               {
                  FIXME ("Command ID Not Supported -> toi_id=0x%lx, toi_entity={tei_entity=0x%lx, tei_instance=0x%lx}, toi_class=0x%lx\n",
                     pcommand->toi_id, pcommand->toi_entity.tei_entity,
                     pcommand->toi_entity.tei_instance, pcommand->toi_class);

                  return (ERROR_BAD_ENVIRONMENT);
               }
            }
         }
         break;


         default:
         {
            FIXME ("Command ID Not Supported -> toi_id=0x%lx, toi_entity={tei_entity=0x%lx, tei_instance=0x%lx}, toi_class=0x%lx\n",
               pcommand->toi_id, pcommand->toi_entity.tei_entity,
               pcommand->toi_entity.tei_instance, pcommand->toi_class);

            return (ERROR_BAD_ENVIRONMENT);
         }
      }

      break;
   }

   case WSCNTL_TCPIP_ICMP_ECHO:
   {
      unsigned int addr = *(unsigned int*)pRequestInfo;
#if 0
         int timeout= *(unsigned int*)(inbuf+4);
         short x1 = *(unsigned short*)(inbuf+8);
         short sendbufsize = *(unsigned short*)(inbuf+10);
         char x2 = *(unsigned char*)(inbuf+12);
         char ttl = *(unsigned char*)(inbuf+13);
         char service = *(unsigned char*)(inbuf+14);
         char type= *(unsigned char*)(inbuf+15); /* 0x2: don't fragment*/
#endif

      FIXME("(ICMP_ECHO) to 0x%08x stub\n", addr);
      break;
   }

   default:
      FIXME("Protocol Not Supported -> protocol=0x%lx, action=0x%lx, Request=%p, RequestLen=%p, Response=%p, ResponseLen=%p\n",
       protocol, action, pRequestInfo, pcbRequestInfoLen, pResponseInfo, pcbResponseInfoLen);

      return (WSAEOPNOTSUPP);

   }

   return (WSCTL_SUCCESS);
}



/***********************************************************************
 *		WSARecvEx			(WSOCK32.1107)
 *
 * WSARecvEx is a Microsoft specific extension to winsock that is identical to recv
 * except that has an in/out argument call flags that has the value MSG_PARTIAL ored
 * into the flags parameter when a partial packet is read. This only applies to
 * sockets using the datagram protocol. This method does not seem to be implemented
 * correctly by microsoft as the winsock implementation does not set the MSG_PARTIAL
 * flag when a fragmented packet arrives.
 */
INT WINAPI WSARecvEx(SOCKET s, char *buf, INT len, INT *flags)
{
    FIXME("(WSARecvEx) partial packet return value not set\n");
    return recv(s, buf, len, *flags);
}


/***********************************************************************
 *       s_perror         (WSOCK32.1108)
 */
void WINAPI s_perror(LPCSTR message)
{
    FIXME("(%s): stub\n",message);
    return;
}
