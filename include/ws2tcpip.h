/*
 * Copyright (C) 2001 Francois Gouget
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

#ifndef __WS2TCPIP__
#define __WS2TCPIP__

#ifdef USE_WS_PREFIX
#define WS(x)    WS_##x
#else
#define WS(x)    x
#endif

/* FIXME: This gets defined by some Unix (Linux) header and messes things */
#undef s6_addr

typedef struct WS(in_addr6)
{
   u_char s6_addr[16];   /* IPv6 address */
} IN6_ADDR, *PIN6_ADDR, *LPIN6_ADDR;

typedef struct WS(sockaddr_in6)
{
   short   sin6_family;            /* AF_INET6 */
   u_short sin6_port;              /* Transport level port number */
   u_long  sin6_flowinfo;          /* IPv6 flow information */
   struct  WS(in_addr6) sin6_addr; /* IPv6 address */
} SOCKADDR_IN6,*PSOCKADDR_IN6, *LPSOCKADDR_IN6;

typedef union sockaddr_gen
{
   struct WS(sockaddr) Address;
   struct WS(sockaddr_in)  AddressIn;
   struct WS(sockaddr_in6) AddressIn6;
} WS(sockaddr_gen);

/* Structure to keep interface specific information */
typedef struct _INTERFACE_INFO
{
    u_long            iiFlags;             /* Interface flags */
    WS(sockaddr_gen)  iiAddress;           /* Interface address */
    WS(sockaddr_gen)  iiBroadcastAddress;  /* Broadcast address */
    WS(sockaddr_gen)  iiNetmask;           /* Network mask */
} INTERFACE_INFO, * LPINTERFACE_INFO;

/* Possible flags for the  iiFlags - bitmask  */
#ifndef USE_WS_PREFIX
#define IFF_UP                0x00000001 /* Interface is up */
#define IFF_BROADCAST         0x00000002 /* Broadcast is  supported */
#define IFF_LOOPBACK          0x00000004 /* this is loopback interface */
#define IFF_POINTTOPOINT      0x00000008 /* this is point-to-point interface */
#define IFF_MULTICAST         0x00000010 /* multicast is supported */
#else
#define WS_IFF_UP             0x00000001 /* Interface is up */
#define WS_IFF_BROADCAST      0x00000002 /* Broadcast is  supported */
#define WS_IFF_LOOPBACK       0x00000004 /* this is loopback interface */
#define WS_IFF_POINTTOPOINT   0x00000008 /* this is point-to-point interface */
#define WS_IFF_MULTICAST      0x00000010 /* multicast is supported */
#endif /* USE_WS_PREFIX */

#ifndef USE_WS_PREFIX
#define IP_OPTIONS             1
#define IP_HDRINCL             2
#define IP_TOS                 3
#define IP_TTL                 4
#define IP_MULTICAST_IF        9
#define IP_MULTICAST_TTL       10
#define IP_MULTICAST_LOOP      11
#define IP_ADD_MEMBERSHIP      12
#define IP_DROP_MEMBERSHIP     13
#define IP_DONTFRAGMENT        14
#else
#define WS_IP_OPTIONS          1
#define WS_IP_HDRINCL          2
#define WS_IP_TOS              3
#define WS_IP_TTL              4
#define WS_IP_MULTICAST_IF     9
#define WS_IP_MULTICAST_TTL    10
#define WS_IP_MULTICAST_LOOP   11
#define WS_IP_ADD_MEMBERSHIP   12
#define WS_IP_DROP_MEMBERSHIP  13
#define WS_IP_DONTFRAGMENT     14
#endif /* USE_WS_PREFIX */

#endif /* __WS2TCPIP__ */
