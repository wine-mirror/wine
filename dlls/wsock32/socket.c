/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
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
 * WSCNTL_TCPIP_QUERY_INFO option is partially implemeted based
 * on observing the behaviour of WsControl with an app in
 * Windows 98.  It is not fully implemented, and there could
 * be (are?) errors due to incorrect assumptions made.
 *
 *
 * WsControl returns WSCTL_SUCCESS on success.
 * STATUS_BUFFER_TOO_SMALL is returned if the output buffer length
 * (*pcbResponseInfoLen) is too small, otherwise errors return -1.
 *
 * It doesn't seem to generate errors that can be retrieved by
 * WSAGetLastError().
 *
 */

DWORD WINAPI WsControl(DWORD protocoll,
                       DWORD action,
                       LPVOID pRequestInfo,
                       LPDWORD pcbRequestInfoLen,
                       LPVOID pResponseInfo,
                       LPDWORD pcbResponseInfoLen)
{

   /* Get the command structure into a pointer we can use,
      rather than void */
   TDIObjectID *pcommand = (TDIObjectID *)pRequestInfo;

   TRACE ("   WsControl TOI_ID=>0x%lx<, {TEI_ENTITY=0x%lx, TEI_INSTANCE=0x%lx}, TOI_CLASS=0x%lx, TOI_TYPE=0x%lx\n",
          pcommand->toi_id, pcommand->toi_entity.tei_entity, pcommand->toi_entity.tei_instance,
          pcommand->toi_class, pcommand->toi_type );



   switch (action)
   {
      case WSCNTL_TCPIP_QUERY_INFO:
      {
         switch (pcommand->toi_id)
         {
            /*
               ENTITY_LIST_ID seems to get number of adapters in the system.
               (almost like an index to be used when calling other WsControl options)
            */
            case ENTITY_LIST_ID:
            {
               TDIEntityID *baseptr = pResponseInfo;
               DWORD numInt, i, ipAddrTableSize;
               PMIB_IPADDRTABLE table;

               if (pcommand->toi_class != INFO_CLASS_GENERIC &&
                   pcommand->toi_type != INFO_TYPE_PROVIDER)
               {
                  FIXME ("Unexpected Option for ENTITY_LIST_ID request -> toi_class=0x%lx, toi_type=0x%lx\n",
                       pcommand->toi_class, pcommand->toi_type);
                  return (WSAEOPNOTSUPP);
               }

               GetNumberOfInterfaces(&numInt);

               if (*pcbResponseInfoLen < sizeof(TDIEntityID)*(numInt*2) )
               {
                  return (STATUS_BUFFER_TOO_SMALL);
               }

               /* expect a 1:1 correspondence between interfaces and IP
                  addresses, so use the cheaper (less memory allocated)
                  GetIpAddrTable rather than GetIfTable */
               ipAddrTableSize = 0;
               GetIpAddrTable(NULL, &ipAddrTableSize, FALSE);
               table = (PMIB_IPADDRTABLE)calloc(1, ipAddrTableSize);
               if (!table) return -1; /* FIXME: better error code */
               GetIpAddrTable(table, &ipAddrTableSize, FALSE);

               /* 0 it out first */
               memset(baseptr, 0, sizeof(TDIEntityID)*(table->dwNumEntries*2));

               for (i=0; i<table->dwNumEntries; i++)
               {
                  /* tei_instance is an network interface identifier.
                     I'm not quite sure what the difference is between tei_entity values of
                     CL_NL_ENTITY and IF_ENTITY */
                  baseptr->tei_entity = CL_NL_ENTITY;  baseptr->tei_instance = table->table[i].dwIndex; baseptr++;
                  baseptr->tei_entity = IF_ENTITY;     baseptr->tei_instance = table->table[i].dwIndex; baseptr++;
               }

               /* Calculate size of out buffer */
               *pcbResponseInfoLen = sizeof(TDIEntityID)*(table->dwNumEntries*2);
               free(table);

               break;
            }


            /* ENTITY_TYPE_ID is used to obtain simple information about a
               network card, such as MAC Address, description, interface type,
               number of network addresses, etc. */
            case ENTITY_TYPE_ID:  /* ALSO: IP_MIB_STATS_ID */
            {
               if (pcommand->toi_class == INFO_CLASS_GENERIC && pcommand->toi_type == INFO_TYPE_PROVIDER)
               {
                  if (pcommand->toi_entity.tei_entity == IF_ENTITY)
                  {
                     * ((ULONG *)pResponseInfo) = IF_MIB;

                     /* Calculate size of out buffer */
                     *pcbResponseInfoLen = sizeof (ULONG);

                  }
                  else if (pcommand->toi_entity.tei_entity == CL_NL_ENTITY)
                  {
                     * ((ULONG *)pResponseInfo) = CL_NL_IP;

                     /* Calculate size of out buffer */
                     *pcbResponseInfoLen = sizeof (ULONG);
                  }
               }
               else if (pcommand->toi_class == INFO_CLASS_PROTOCOL &&
                        pcommand->toi_type == INFO_TYPE_PROVIDER)
               {
                  if (pcommand->toi_entity.tei_entity == IF_ENTITY)
                  {
                     MIB_IFROW row;
                     DWORD index = pcommand->toi_entity.tei_instance, ret;
                     DWORD size = sizeof(row) - sizeof(row.wszName) -
                      sizeof(row.bDescr);

                     if (*pcbResponseInfoLen < size)
                     {
                        return (STATUS_BUFFER_TOO_SMALL);
                     }
                     row.dwIndex = index;
                     ret = GetIfEntry(&row);
                     if (ret != NO_ERROR)
                     {
                       ERR ("Error retrieving data for interface index %lu\n", index);
                       return -1; /* FIXME: better error code */
                     }
                     size = sizeof(row) - sizeof(row.wszName) -
                      sizeof(row.bDescr) + row.dwDescrLen;
                     if (*pcbResponseInfoLen < size)
                     {
                        return (STATUS_BUFFER_TOO_SMALL);
                     }
                     memcpy(pResponseInfo, &row.dwIndex, size);
                     *pcbResponseInfoLen = size;
                  }
                  else if (pcommand->toi_entity.tei_entity == CL_NL_ENTITY)
                  {
                     /* This case is used to obtain general statistics about the
                        network */

                     if (*pcbResponseInfoLen < sizeof(MIB_IPSTATS))
                     {
                        return (STATUS_BUFFER_TOO_SMALL);
                     }
                     GetIpStatistics((PMIB_IPSTATS)pResponseInfo);

                     /* Calculate size of out buffer */
                     *pcbResponseInfoLen = sizeof(MIB_IPSTATS);
                  }
               }
               else
               {
                  FIXME ("Unexpected Option for ENTITY_TYPE_ID request -> toi_class=0x%lx, toi_type=0x%lx\n",
                       pcommand->toi_class, pcommand->toi_type);

                  return (WSAEOPNOTSUPP);
               }

               break;
            }


            /* IP_MIB_ADDRTABLE_ENTRY_ID is used to obtain more detailed information about a
               particular network adapter */
            case IP_MIB_ADDRTABLE_ENTRY_ID:
            {
               DWORD index = pcommand->toi_entity.tei_instance;
               PMIB_IPADDRROW baseIPInfo = (PMIB_IPADDRROW) pResponseInfo;
               PMIB_IPADDRTABLE table;
               DWORD tableSize, i;

               if (*pcbResponseInfoLen < sizeof(MIB_IPADDRROW))
               {
                  return (STATUS_BUFFER_TOO_SMALL);
               }

               /* overkill, get entire table, because there isn't an
                  exported function that gets just one entry, and don't
                  necessarily want our own private export. */
               tableSize = 0;
               GetIpAddrTable(NULL, &tableSize, FALSE);
               table = (PMIB_IPADDRTABLE)calloc(1, tableSize);
               if (!table) return -1; /* FIXME: better error code */
               GetIpAddrTable(table, &tableSize, FALSE);
               for (i = 0; i < table->dwNumEntries; i++)
               {
                    if (table->table[i].dwIndex == index)
                    {
                       memcpy(baseIPInfo, &table->table[i],
                        sizeof(MIB_IPADDRROW));
                       break;
                    }
               }
               free(table);

               /************************************************************************/

               /* Calculate size of out buffer */
               *pcbResponseInfoLen = sizeof(MIB_IPADDRROW);
               break;
            }


            /* This call returns the routing table.
             * No official documentation found, even the name of the command is unknown.
             * Work is based on
             * http://www.cyberport.com/~tangent/programming/winsock/articles/wscontrol.html
             * and testings done with winipcfg.exe, route.exe and ipconfig.exe.
             * pcommand->toi_entity.tei_instance seems to be the interface number
             * but route.exe outputs only the information for the last interface
             * if only the routes for the pcommand->toi_entity.tei_instance
             * interface are returned. */
            case IP_MIB_ROUTETABLE_ENTRY_ID:  /* FIXME: not real name. Value is 0x101 */
            {
                DWORD routeTableSize, numRoutes, ndx;
                PMIB_IPFORWARDTABLE table;
                IPRouteEntry *winRouteTable  = (IPRouteEntry *) pResponseInfo;

                GetIpForwardTable(NULL, &routeTableSize, FALSE);
                numRoutes = min(routeTableSize - sizeof(MIB_IPFORWARDTABLE), 0)
                 / sizeof(MIB_IPFORWARDROW) + 1;
                if (*pcbResponseInfoLen < sizeof(IPRouteEntry) * numRoutes)
                {
                    return (STATUS_BUFFER_TOO_SMALL);
                }
                table = (PMIB_IPFORWARDTABLE)calloc(1, routeTableSize);
                if (!table) return -1; /* FIXME: better return value */
                GetIpForwardTable(table, &routeTableSize, FALSE);

                memset(pResponseInfo, 0, sizeof(IPRouteEntry) * numRoutes);
                for (ndx = 0; ndx < table->dwNumEntries; ndx++)
                {
                    winRouteTable->ire_addr =
                     table->table[ndx].dwForwardDest;
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

                free(table);
                break;
            }


            default:
            {
               FIXME ("Command ID Not Supported -> toi_id=0x%lx, toi_entity={tei_entity=0x%lx, tei_instance=0x%lx}, toi_class=0x%lx, toi_type=0x%lx\n",
                       pcommand->toi_id, pcommand->toi_entity.tei_entity, pcommand->toi_entity.tei_instance,
                       pcommand->toi_class, pcommand->toi_type);

               return (WSAEOPNOTSUPP);
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

         FIXME("(ICMP_ECHO) to 0x%08x stub \n", addr);
         break;
      }

      default:
      {
         FIXME("Protocoll Not Supported -> protocoll=0x%lx, action=0x%lx, Request=%p, RequestLen=%p, Response=%p, ResponseLen=%p\n",
	       protocoll, action, pRequestInfo, pcbRequestInfoLen, pResponseInfo, pcbResponseInfoLen);

         return (WSAEOPNOTSUPP);
      }
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
    FIXME("(WSARecvEx) partial packet return value not set \n");
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
