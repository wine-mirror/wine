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


/* FIXME: This hack is fixing a problem in WsControl.  When we call socket(),
 * it will call into ws2_32's WSOCK32_socket (because of the redirection in
 * our own .spec file).
 * The problem is that socket() is predefined in a linux system header that
 * we are including, which is different from the WINE definition.
 * (cdecl vs. stdapi).  The result is stack corruption.
 * Furthermore WsControl uses Unix macros and types. This forces us to include
 * the Unix headers which then conflict with the winsock headers. This forces
 * us to use USE_WS_PREFIX but then ioctlsocket is called WS_ioctlsocket,
 * which causes link problems. The correct solution is to implement
 * WsControl using calls to WSAIoctl. Then we should no longer need to use the
 * Unix headers. This would also have the advantage of reducing code
 * duplication.
 * Until that happens we need this ugly hack.
 */
#define USE_WS_PREFIX

#define socket  linux_socket
#define recv    linux_recv
/* */

#define setsockopt linux_setsockopt
#define getsockopt linux_getsockopt

#include "config.h"

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "winsock2.h"
#include "winnt.h"
#include "wscontrol.h"

/* FIXME: The rest of the socket() cdecl<->stdapi stack corruption problem
 * discussed above.
 */
#undef socket
#undef recv
extern SOCKET WINAPI socket(INT af, INT type, INT protocol);
extern SOCKET WINAPI recv(SOCKET,char*,int,int);
/* Plus some missing prototypes, due to the WS_ prefixing */
extern int    WINAPI closesocket(SOCKET);
extern int    WINAPI ioctlsocket(SOCKET,long,u_long*);
/* */

/* for the get/setsockopt forwarders */
#undef setsockopt
#undef getsockopt
extern int WINAPI setsockopt(SOCKET s, INT level, INT optname ,char* optval, INT optlen);
extern int WINAPI getsockopt(SOCKET s, INT level, INT optname ,char* optval, INT* optlen);

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

/* internal remapper function for the IP_ constants */
static INT _remap_optname(INT level, INT optname)
{
  TRACE("level=%d, optname=%d\n", level, optname);
  if (level == WS_IPPROTO_IP) {
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
               int numInt = 0, i;

               if (pcommand->toi_class != INFO_CLASS_GENERIC &&
                   pcommand->toi_type != INFO_TYPE_PROVIDER)
               {
                  FIXME ("Unexpected Option for ENTITY_LIST_ID request -> toi_class=0x%lx, toi_type=0x%lx\n",
                       pcommand->toi_class, pcommand->toi_type);
                  return (WSAEOPNOTSUPP);
               }

               numInt = WSCNTL_GetEntryCount(WSCNTL_COUNT_INTERFACES);
               if (numInt < 0)
               {
                  ERR ("Unable to open /proc filesystem to determine number of network interfaces!\n");
                  return (-1);
               }

               if (*pcbResponseInfoLen < sizeof(TDIEntityID)*(numInt*2) )
               {
                  return (STATUS_BUFFER_TOO_SMALL);
               }

               /* 0 it out first */
               memset(baseptr, 0, sizeof(TDIEntityID)*(numInt*2));

               for (i=0; i<numInt; i++)
               {
                  /* tei_instance is an network interface identifier.
                     I'm not quite sure what the difference is between tei_entity values of
                     CL_NL_ENTITY and IF_ENTITY */
                  baseptr->tei_entity = CL_NL_ENTITY;  baseptr->tei_instance = i; baseptr++;
                  baseptr->tei_entity = IF_ENTITY;     baseptr->tei_instance = i; baseptr++;
               }

               /* Calculate size of out buffer */
               *pcbResponseInfoLen = sizeof(TDIEntityID)*(numInt*2);

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
                     /* In this case, we are requesting specific information about a
                        a particular network adapter. (MAC Address, speed, data transmitted/received,
                        etc.)
                     */
                     IFEntry *IntInfo = (IFEntry *) pResponseInfo;
                     char ifName[512];
#if defined(SIOCGIFHWADDR) || defined(SIOCGENADDR)
                     struct ifreq ifInfo;
#endif
                     SOCKET sock;


                     if (!WSCNTL_GetInterfaceName(pcommand->toi_entity.tei_instance, ifName))
                     {
                        ERR ("Unable to parse /proc filesystem!\n");
                        return (-1);
                     }

                     /* Get a socket so that we can use ioctl */
                     if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
                     {
                        ERR ("Error creating socket!\n");
                        return (-1);
                     }

                     /* 0 out return structure first */
                     memset (IntInfo, 0, sizeof(IFEntry));

                     /* Interface ID */
                     IntInfo->if_index = pcommand->toi_entity.tei_instance;

                     /* MAC Address - Let's try to do this in a cross-platform way... */
#if defined(SIOCGIFHWADDR) /* Linux */
                        strcpy(ifInfo.ifr_name, ifName);
                        if (ioctlsocket(sock, SIOCGIFHWADDR, (ULONG*)&ifInfo) < 0)
                        {
                           ERR ("Error obtaining MAC Address!\n");
                           closesocket(sock);
                           return (-1);
                        }
                        else
                        {
                           /* FIXME: Is it correct to assume size of 6? */
                           memcpy(IntInfo->if_physaddr, ifInfo.ifr_hwaddr.sa_data, 6);
                           IntInfo->if_physaddrlen=6;
                        }
#elif defined(SIOCGENADDR) /* Solaris */
                        if (ioctlsocket(sock, SIOCGENADDR, (ULONG*)&ifInfo) < 0)
                        {
                           ERR ("Error obtaining MAC Address!\n");
                           closesocket(sock);
                           return (-1);
                        }
                        else
                        {
                           /* FIXME: Is it correct to assume size of 6? */
                           memcpy(IntInfo->if_physaddr, ifInfo.ifr_enaddr, 6);
                           IntInfo->if_physaddrlen=6;
                        }
#else
                        memset (IntInfo->if_physaddr, 0, 6);
                        ERR ("Unable to determine MAC Address on your platform!\n");
#endif


                     /* Interface name and length */
                     strcpy (IntInfo->if_descr, ifName);
                     IntInfo->if_descrlen= strlen (IntInfo->if_descr);

                     /* Obtain bytes transmitted/received for interface */
                     if ( (WSCNTL_GetTransRecvStat(pcommand->toi_entity.tei_instance,
                           &IntInfo->if_inoctets, &IntInfo->if_outoctets)) < 0)
                     {
                        ERR ("Error obtaining transmit/receive stats for the network interface!\n");
                        closesocket(sock);
                        return (-1);
                     }


                     /* FIXME: How should the below be properly calculated? ******************/
                     IntInfo->if_type =  0x6; /* Ethernet (?) */
                     IntInfo->if_speed = 1000000; /* Speed of interface (bits per second?) */
                     /************************************************************************/

                     closesocket(sock);
                     *pcbResponseInfoLen = sizeof (IFEntry) + IntInfo->if_descrlen;
                  }
                  else if (pcommand->toi_entity.tei_entity == CL_NL_ENTITY)
                  {
                     IPSNMPInfo *infoStruc = (IPSNMPInfo *) pResponseInfo;
                     int numInt, numRoutes;

                     /* This case is used to obtain general statistics about the
                        network */

                     if (*pcbResponseInfoLen < sizeof(IPSNMPInfo) )
                     {
                        return (STATUS_BUFFER_TOO_SMALL);
                     }
                     else
                     {
                        /* 0 it out first */
                        memset(infoStruc, 0, sizeof(IPSNMPInfo));

                        /* Get the number of interfaces */
                        numInt = WSCNTL_GetEntryCount(WSCNTL_COUNT_INTERFACES);
                        if (numInt < 0)
                        {
                           ERR ("Unable to open /proc filesystem to determine number of network interfaces!\n");
                           return (-1);
                        }
                        /* Get the number of routes */
                        numRoutes = WSCNTL_GetEntryCount(WSCNTL_COUNT_ROUTES);
                        if (numRoutes < 0)
                        {
                           ERR ("Unable to open /proc filesystem to determine number of network routes!\n");
                           return (-1);
                        }

                        infoStruc->ipsi_numif           = numInt; /* # of interfaces */
                        infoStruc->ipsi_numaddr         = numInt; /* # of addresses */
                        infoStruc->ipsi_numroutes       = numRoutes; /* # of routes */

                        /* FIXME: How should the below be properly calculated? ******************/
                        infoStruc->ipsi_forwarding      = 0x0;
                        infoStruc->ipsi_defaultttl      = 0x0;
                        infoStruc->ipsi_inreceives      = 0x0;
                        infoStruc->ipsi_inhdrerrors     = 0x0;
                        infoStruc->ipsi_inaddrerrors    = 0x0;
                        infoStruc->ipsi_forwdatagrams   = 0x0;
                        infoStruc->ipsi_inunknownprotos = 0x0;
                        infoStruc->ipsi_indiscards      = 0x0;
                        infoStruc->ipsi_indelivers      = 0x0;
                        infoStruc->ipsi_outrequests     = 0x0;
                        infoStruc->ipsi_routingdiscards = 0x0;
                        infoStruc->ipsi_outdiscards     = 0x0;
                        infoStruc->ipsi_outnoroutes     = 0x0;
                        infoStruc->ipsi_reasmtimeout    = 0x0;
                        infoStruc->ipsi_reasmreqds      = 0x0;
                        infoStruc->ipsi_reasmoks        = 0x0;
                        infoStruc->ipsi_reasmfails      = 0x0;
                        infoStruc->ipsi_fragoks         = 0x0;
                        infoStruc->ipsi_fragfails       = 0x0;
                        infoStruc->ipsi_fragcreates     = 0x0;
                        /************************************************************************/

                        /* Calculate size of out buffer */
                        *pcbResponseInfoLen = sizeof(IPSNMPInfo);
                     }
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
               IPAddrEntry *baseIPInfo = (IPAddrEntry *) pResponseInfo;
               char ifName[IFNAMSIZ+1];
               struct ifreq ifInfo;
               SOCKET sock;

               if (*pcbResponseInfoLen < sizeof(IPAddrEntry))
               {
                  return (STATUS_BUFFER_TOO_SMALL);
               }

               if (!WSCNTL_GetInterfaceName(pcommand->toi_entity.tei_instance, ifName))
               {
                  ERR ("Unable to parse /proc filesystem!\n");
                  return (-1);
               }


               /* Get a socket so we can use ioctl */
               if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
               {
                  ERR ("Error creating socket!\n");
                  return (-1);
               }

               /* 0 it out first */
               memset(baseIPInfo, 0, sizeof(IPAddrEntry) );

               /* Interface Id */
               baseIPInfo->iae_index     = pcommand->toi_entity.tei_instance;

               /* IP Address */
               strcpy (ifInfo.ifr_name, ifName);
	       ifInfo.ifr_addr.sa_family = AF_INET;
	       if (ioctlsocket(sock, SIOCGIFADDR, (ULONG*)&ifInfo) < 0)
               {
                  baseIPInfo->iae_addr = 0x0;
               }
               else
               {
                  struct WS_sockaddr_in* ipTemp = (struct WS_sockaddr_in*)&ifInfo.ifr_addr;
                  baseIPInfo->iae_addr = ipTemp->sin_addr.S_un.S_addr;
               }

               /* Broadcast Address */
               strcpy (ifInfo.ifr_name, ifName);
	       if (ioctlsocket(sock, SIOCGIFBRDADDR, (ULONG *)&ifInfo) < 0)
               {
                  baseIPInfo->iae_bcastaddr = 0x0;
               }
	       else
               {
                  struct WS_sockaddr_in* ipTemp = (struct WS_sockaddr_in*)&ifInfo.ifr_broadaddr;
                  baseIPInfo->iae_bcastaddr = ipTemp->sin_addr.S_un.S_addr;
               }

               /* Subnet Mask */
	       strcpy(ifInfo.ifr_name, ifName);
	       if (ioctlsocket(sock, SIOCGIFNETMASK, (ULONG *)&ifInfo) < 0)
               {
                  baseIPInfo->iae_mask = 0x0;
	       }
               else
	       {
                  /* Trying to avoid some compile problems across platforms.
                     (Linux, FreeBSD, Solaris...) */
                  #ifndef ifr_netmask
                     #ifndef ifr_addr
                        baseIPInfo->iae_mask = 0;
                        ERR ("Unable to determine Netmask on your platform!\n");
                     #else
                        struct WS_sockaddr_in* ipTemp = (struct WS_sockaddr_in*)&ifInfo.ifr_addr;
                        baseIPInfo->iae_mask = ipTemp->sin_addr.S_un.S_addr;
                     #endif
                  #else
                     struct WS_sockaddr_in* ipTemp = (struct WS_sockaddr_in*)&ifInfo.ifr_netmask;
                     baseIPInfo->iae_mask = ipTemp->sin_addr.S_un.S_addr;
                  #endif
               }

               /* FIXME: How should the below be properly calculated? ******************/
               baseIPInfo->iae_reasmsize = 0x0;
               baseIPInfo->iae_context   = 0x0;
               baseIPInfo->iae_pad       = 0x0;
               /************************************************************************/

               /* Calculate size of out buffer */
               *pcbResponseInfoLen = sizeof(IPAddrEntry);
               closesocket(sock);
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
	    case IP_MIB_ROUTETABLE_ENTRY_ID:	/* FIXME: not real name. Value is 0x101 */
	    {
		int numRoutes, foundRoutes;
		wscntl_routeentry *routeTable, *routePtr;	/* route table */

                IPRouteEntry *winRouteTable  = (IPRouteEntry *) pResponseInfo;

		/* Get the number of routes */
		numRoutes = WSCNTL_GetEntryCount(WSCNTL_COUNT_ROUTES);
		if (numRoutes < 0)
		{
		    ERR ("Unable to open /proc filesystem to determine number of network routes!\n");
		    return (-1);
		}

		if (*pcbResponseInfoLen < (sizeof(IPRouteEntry) * numRoutes))
		{
		    return (STATUS_BUFFER_TOO_SMALL);
		}

		/* malloc space for the routeTable */
		routeTable = (wscntl_routeentry *) malloc(sizeof(wscntl_routeentry) * numRoutes);
		if (!routeTable)
	       	{
		    ERR ("couldn't malloc space for routeTable!\n");
		}

		/* get the route table */
		foundRoutes = WSCNTL_GetRouteTable(numRoutes, routeTable);
		if (foundRoutes < 0)
		{
		    ERR ("Unable to open /proc filesystem to parse the route entries!\n");
		    free(routeTable);
		    return -1;
		}
		routePtr = routeTable;

                /* first 0 out the output buffer */
                memset(winRouteTable, 0, *pcbResponseInfoLen);

		/* calculate the length of the data in the output buffer */
                *pcbResponseInfoLen = sizeof(IPRouteEntry) * foundRoutes;

		for ( ; foundRoutes > 0; foundRoutes--)
		{
		    winRouteTable->ire_addr = routePtr->wre_dest;
		    winRouteTable->ire_index = routePtr->wre_intf;
		    winRouteTable->ire_metric = routePtr->wre_metric;
		    /* winRouteTable->ire_option4 =
		    winRouteTable->ire_option5 =
		    winRouteTable->ire_option6 = */
		    winRouteTable->ire_gw = routePtr->wre_gw;
		    /* winRouteTable->ire_option8 =
		    winRouteTable->ire_option9 =
		    winRouteTable->ire_option10 = */
		    winRouteTable->ire_mask = routePtr->wre_mask;
		    /* winRouteTable->ire_option12 = */

		    winRouteTable++;
		    routePtr++;
		}

		free(routeTable);
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



/*
  Helper function for WsControl - Get count of the number of interfaces
  or routes by parsing /proc filesystem.
*/
int WSCNTL_GetEntryCount(const int entrytype)
{
   char *filename;
   int 	fd;
   char buf[512];  /* Size optimized for a typical workstation */
   char	*ptr;
   int  count;
   int	chrread;


   switch (entrytype)
   {
       case WSCNTL_COUNT_INTERFACES:
       {
	   filename = PROCFS_NETDEV_FILE;
	   count = -2;	/* two haeder lines */
	   break;
       };

       case WSCNTL_COUNT_ROUTES:
       {
	   filename = PROCFS_ROUTE_FILE;
	   count = -1;	/* one haeder line */
	   break;
       };

       default:
       {
	   return -1;
       };
   }

   /* open /proc filesystem file */
   fd = open(filename, O_RDONLY);
   if (fd < 0) {
       return -1;
   }

   /* read the file and count the EOL's */
   while ((chrread = read(fd, buf, sizeof(buf))) != 0)
   {
       ptr = buf;
       if (chrread < 0)
       {
	   if (errno == EINTR)
	   {
	       continue;	/* read interupted by a signal, try to read again */
	   }
	   else
	   {
	       close(fd);
	       return -1;
	   }
       }
       while ((ptr = memchr(ptr, '\n', chrread - (int) (ptr -  buf))) > 0)
       {
	   count++;
	   ptr++;
       }
   }

   close(fd);
   return count;
}


/*
   Helper function for WsControl - Get name of device from interface number
   by parsing /proc filesystem.
*/
int WSCNTL_GetInterfaceName(int intNumber, char *intName)
{
   FILE *procfs;
   char buf[512]; /* Size doesn't matter, something big */
   int  i;

   /* Open /proc filesystem file for network devices */
   procfs = fopen(PROCFS_NETDEV_FILE, "r");
   if (!procfs)
   {
      /* If we can't open the file, return an error */
      return (-1);
   }

   /* Omit first two lines, they are only headers */
   fgets(buf, sizeof(buf), procfs);
   fgets(buf, sizeof(buf), procfs);

   for (i=0; i<intNumber; i++)
   {
      /* Skip the lines that don't interest us. */
      fgets(buf, sizeof(buf), procfs);
   }
   fgets(buf, sizeof(buf), procfs); /* This is the line we want */


   /* Parse out the line, grabbing only the name of the device
      to the intName variable

      The Line comes in like this: (we only care about the device name)
      lo:   21970 377 0 0 0 0 0 0 21970 377 0 0 0 0 0 0
   */
   i=0;
   while (isspace(buf[i])) /* Skip initial space(s) */
   {
      i++;
   }

   while (buf[i])
   {
      if (isspace(buf[i]))
      {
         break;
      }

      if (buf[i] == ':')  /* FIXME: Not sure if this block (alias detection) works properly */
      {
         /* This interface could be an alias... */
         int hold = i;
         char *dotname = intName;
         *intName++ = buf[i++];

         while (isdigit(buf[i]))
         {
            *intName++ = buf[i++];
         }

         if (buf[i] != ':')
         {
            /* ... It wasn't, so back up */
            i = hold;
            intName = dotname;
         }

         if (buf[i] == '\0')
         {
            fclose(procfs);
            return(FALSE);
         }

         i++;
         break;
      }

      *intName++ = buf[i++];
   }
   *intName++ = '\0';

   fclose(procfs);
   return(TRUE);
}


/*
   Helper function for WsControl - This function returns the bytes (octets) transmitted
   and received for the supplied interface number from the /proc fs.
*/
int WSCNTL_GetTransRecvStat(int intNumber, unsigned long *transBytes, unsigned long *recvBytes)
{
   FILE *procfs;
   char buf[512], result[512]; /* Size doesn't matter, something big */
   int  i, bufPos, resultPos;

   /* Open /proc filesystem file for network devices */
   procfs = fopen(PROCFS_NETDEV_FILE, "r");
   if (!procfs)
   {
      /* If we can't open the file, return an error */
      return (-1);
   }

   /* Omit first two lines, they are only headers */
   fgets(buf, sizeof(buf), procfs);
   fgets(buf, sizeof(buf), procfs);

   for (i=0; i<intNumber; i++)
   {
      /* Skip the lines that don't interest us. */
      fgets(buf, sizeof(buf), procfs);
   }
   fgets(buf, sizeof(buf), procfs); /* This is the line we want */



   /* Parse out the line, grabbing the number of bytes transmitted
      and received on the interface.

      The Line comes in like this: (we care about columns 2 and 10)
      lo:   21970 377 0 0 0 0 0 0 21970 377 0 0 0 0 0 0
   */

   /* Start at character 0 in the buffer */
   bufPos=0;

   /* Skip initial space(s) */
   while (isspace(buf[bufPos]))
      bufPos++;


   /* Skip the name and its trailing spaces (if any) */
   while (buf[bufPos])
   {
      if (isspace(buf[bufPos]))
         break;

      if (buf[bufPos] == ':') /* Could be an alias */
      {
         int hold = bufPos;

         while(isdigit (buf[bufPos]))
            bufPos++;
         if (buf[bufPos] != ':')
            bufPos = hold;
         if (buf[bufPos] == '\0')
         {
            fclose(procfs);
            return(FALSE);
         }

         bufPos++;
         break;
      }

      bufPos++;
   }
   while (isspace(buf[bufPos]))
      bufPos++;


   /* This column (#2) is the number of bytes received. */
   resultPos = 0;
   while (!isspace(buf[bufPos]))
   {
      result[resultPos] = buf[bufPos];
      result[resultPos+1]='\0';
      resultPos++; bufPos++;
   }
   *recvBytes = strtoul (result, NULL, 10); /* convert string to unsigned long, using base 10 */


   /* Skip columns #3 to #9 (Don't need them) */
   for  (i=0; i<7; i++)
   {
      while (isspace(buf[bufPos]))
         bufPos++;
      while (!isspace(buf[bufPos]))
         bufPos++;
   }


   /* This column (#10) is the number of bytes transmitted */
   while (isspace(buf[bufPos]))
       bufPos++;

   resultPos = 0;
   while (!isspace(buf[bufPos]))
   {
      result[resultPos] = buf[bufPos];
      result[resultPos+1]='\0';
      resultPos++; bufPos++;
   }
   *transBytes = strtoul (result, NULL, 10); /* convert string to unsigned long, using base 10 */


   fclose(procfs);
   return(TRUE);
}


/* Parse the procfs route file and put the datas into routeTable.
 * Return value is the number of found routes */
int WSCNTL_GetRouteTable(int numRoutes, wscntl_routeentry *routeTable)
{
    int nrIntf;		/* total number of interfaces */
    char buf[256];	/* temporary buffer */
    char *ptr;		/* pointer to temporary buffer */
    FILE *file;		/* file handle for procfs route file */
    int foundRoutes = 0;	/* number of found routes */
    typedef struct interface_t {
	char intfName[IFNAMSIZ+1];	/* the name of the interface */
	int intfNameLen;	/* length of interface name */
    } interface_t;
    interface_t *interface;
    int intfNr;		/* the interface number */

    wscntl_routeentry *routePtr = routeTable;

    /* get the number of interfaces */
    nrIntf = WSCNTL_GetEntryCount(WSCNTL_COUNT_INTERFACES);
    if (nrIntf < 0)
    {
	ERR ("Unable to open /proc filesystem to determine number of network interfaces!\n");
	return (-1);
    }

    /* malloc space for the interface struct array */
    interface = (interface_t *) malloc(sizeof(interface_t) * nrIntf);
    if (!routeTable)
    {
	ERR ("couldn't malloc space for interface!\n");
    }

    for (intfNr = 0; intfNr < nrIntf; intfNr++) {
	if (WSCNTL_GetInterfaceName(intfNr, interface[intfNr].intfName) < 0)
	{
	    ERR ("Unable to open /proc filesystem to determine the name of network interfaces!\n");
	    free(interface);
	    return (-1);
	}
	interface[intfNr].intfNameLen = strlen(interface[intfNr].intfName);
    }

    /* Open /proc filesystem file for routes */
    file = fopen(PROCFS_ROUTE_FILE, "r");
    if (!file)
    {
       	/* If we can't open the file, return an error */
	free(interface);
	return (-1);
    }

    /* skip the header line */
    fgets(buf, sizeof(buf), file);

    /* parse the rest of the file and put the matching entries into routeTable.
       Format of procfs route entry:
       Iface Destination Gateway Flags RefCnt Use Metric Mask  MTU Window IRTT
       lo 0000007F 00000000 0001 0 0 0 000000FF 0 0 0
    */
    while (fgets(buf, sizeof(buf), file)) {
	intfNr = 0;
	/* find the interface of the route */
	while ((strncmp(buf, interface[intfNr].intfName, interface[intfNr].intfNameLen) != 0)
		&& (intfNr < nrIntf))
	{
	    intfNr++;
	}
	if (intfNr < nrIntf) {
	    foundRoutes++;
	    if (foundRoutes > numRoutes) {
		/* output buffer is to small */
		ERR("buffer to small to fit all routes found into it!\n");
		free(interface);
		fclose(file);
		return -1;
	    }
	    ptr = buf;
	    ptr += interface[intfNr].intfNameLen;
	    routePtr->wre_intf = intfNr;
	    routePtr->wre_dest = strtoul(ptr, &ptr, 16);	/* destination */
	    routePtr->wre_gw = strtoul(ptr, &ptr, 16);	/* gateway */
	    strtoul(ptr, &ptr, 16);	/* Flags; unused */
	    strtoul(ptr, &ptr, 16);	/* RefCnt; unused */
	    strtoul(ptr, &ptr, 16);	/* Use; unused */
	    routePtr->wre_metric = strtoul(ptr, &ptr, 16);	/* metric */
	    routePtr->wre_mask = strtoul(ptr, &ptr, 16);	/* mask */
	    /* strtoul(ptr, &ptr, 16);	MTU; unused */
	    /* strtoul(ptr, &ptr, 16);	Window; unused */
	    /* strtoul(ptr, &ptr, 16);	IRTT; unused */

	    routePtr++;
	}
	else
	{
	    /* this should never happen */
	    WARN("Skipping route with unknown interface\n");
	}
    }

    free(interface);
    fclose(file);
    return foundRoutes;
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
