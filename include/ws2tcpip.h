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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WS2TCPIP__
#define __WS2TCPIP__

#include <winsock2.h>
/* FIXME: #include <ws2ipdef.h> */
#include <limits.h>

#ifdef USE_WS_PREFIX
#define WS(x)    WS_##x
#else
#define WS(x)    x
#endif

/* FIXME: This gets defined by some Unix (Linux) header and messes things */
#undef s6_addr

/* for addrinfo calls */
typedef struct WS(addrinfo)
{
    int                ai_flags;
    int                ai_family;
    int                ai_socktype;
    int                ai_protocol;
    size_t             ai_addrlen;
    char *             ai_canonname;
    struct WS(sockaddr)*   ai_addr;
    struct WS(addrinfo)*   ai_next;
} ADDRINFOA, *PADDRINFOA;

typedef struct WS(addrinfoW)
{
    int                ai_flags;
    int                ai_family;
    int                ai_socktype;
    int                ai_protocol;
    size_t             ai_addrlen;
    PWSTR              ai_canonname;
    struct WS(sockaddr)*   ai_addr;
    struct WS(addrinfoW)*   ai_next;
} ADDRINFOW, *PADDRINFOW;

typedef int WS(socklen_t);

typedef ADDRINFOA ADDRINFO, *LPADDRINFO;

/*
 * Multicast group information
 */

struct WS(ip_mreq)
{
    struct WS(in_addr) imr_multiaddr;
    struct WS(in_addr) imr_interface;
};

struct WS(ip_mreq_source) {
    struct WS(in_addr) imr_multiaddr;
    struct WS(in_addr) imr_sourceaddr;
    struct WS(in_addr) imr_interface;
};

struct WS(ip_msfilter) {
    struct WS(in_addr) imsf_multiaddr;
    struct WS(in_addr) imsf_interface;
    WS(u_long)         imsf_fmode;
    WS(u_long)         imsf_numsrc;
    struct WS(in_addr) imsf_slist[1];
};

typedef struct WS(in_addr6)
{
   WS(u_char) s6_addr[16]; /* IPv6 address */
} IN6_ADDR, *PIN6_ADDR, *LPIN6_ADDR;

typedef struct WS(sockaddr_in6)
{
   short   sin6_family;            /* AF_INET6 */
   WS(u_short) sin6_port;          /* Transport level port number */
   WS(u_long) sin6_flowinfo;       /* IPv6 flow information */
   struct  WS(in_addr6) sin6_addr; /* IPv6 address */
   WS(u_long) sin6_scope_id;       /* IPv6 scope id */
} SOCKADDR_IN6,*PSOCKADDR_IN6, *LPSOCKADDR_IN6;

typedef struct WS(sockaddr_in6_old)
{
   short   sin6_family;            /* AF_INET6 */
   WS(u_short) sin6_port;              /* Transport level port number */
   WS(u_long) sin6_flowinfo;          /* IPv6 flow information */
   struct  WS(in_addr6) sin6_addr; /* IPv6 address */
} SOCKADDR_IN6_OLD,*PSOCKADDR_IN6_OLD, *LPSOCKADDR_IN6_OLD;

typedef union sockaddr_gen
{
   struct WS(sockaddr) Address;
   struct WS(sockaddr_in)  AddressIn;
   struct WS(sockaddr_in6_old) AddressIn6;
} WS(sockaddr_gen);

/* Structure to keep interface specific information */
typedef struct _INTERFACE_INFO
{
    WS(u_long)        iiFlags;             /* Interface flags */
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
#define IP_OPTIONS                      1
#define IP_HDRINCL                      2
#define IP_TOS                          3
#define IP_TTL                          4
#define IP_MULTICAST_IF                 9
#define IP_MULTICAST_TTL                10
#define IP_MULTICAST_LOOP               11
#define IP_ADD_MEMBERSHIP               12
#define IP_DROP_MEMBERSHIP              13
#define IP_DONTFRAGMENT                 14
#define IP_ADD_SOURCE_MEMBERSHIP        15
#define IP_DROP_SOURCE_MEMBERSHIP       16
#define IP_BLOCK_SOURCE                 17
#define IP_UNBLOCK_SOURCE               18
#define IP_PKTINFO                      19
#define IP_RECEIVE_BROADCAST            22
#else
#define WS_IP_OPTIONS                   1
#define WS_IP_HDRINCL                   2
#define WS_IP_TOS                       3
#define WS_IP_TTL                       4
#define WS_IP_MULTICAST_IF              9
#define WS_IP_MULTICAST_TTL             10
#define WS_IP_MULTICAST_LOOP            11
#define WS_IP_ADD_MEMBERSHIP            12
#define WS_IP_DROP_MEMBERSHIP           13
#define WS_IP_DONTFRAGMENT              14
#define WS_IP_ADD_SOURCE_MEMBERSHIP     15
#define WS_IP_DROP_SOURCE_MEMBERSHIP    16
#define WS_IP_BLOCK_SOURCE              17
#define WS_IP_UNBLOCK_SOURCE            18
#define WS_IP_PKTINFO                   19
#define WS_IP_RECEIVE_BROADCAST         22
#endif /* USE_WS_PREFIX */

/* Possible Windows flags for getaddrinfo() */
#ifndef USE_WS_PREFIX
# define AI_PASSIVE     0x0001
# define AI_CANONNAME   0x0002
# define AI_NUMERICHOST 0x0004
/* getaddrinfo error codes */
# define EAI_AGAIN	WSATRY_AGAIN
# define EAI_BADFLAGS	WSAEINVAL
# define EAI_FAIL	WSANO_RECOVERY
# define EAI_FAMILY	WSAEAFNOSUPPORT
# define EAI_MEMORY	WSA_NOT_ENOUGH_MEMORY
# define EAI_NODATA	EAI_NONAME
# define EAI_NONAME	WSAHOST_NOT_FOUND
# define EAI_SERVICE	WSATYPE_NOT_FOUND
# define EAI_SOCKTYPE	WSAESOCKTNOSUPPORT
#else
# define WS_AI_PASSIVE     0x0001
# define WS_AI_CANONNAME   0x0002
# define WS_AI_NUMERICHOST 0x0004
/* getaddrinfo error codes */
# define WS_EAI_AGAIN	WSATRY_AGAIN
# define WS_EAI_BADFLAGS	WSAEINVAL
# define WS_EAI_FAIL	WSANO_RECOVERY
# define WS_EAI_FAMILY	WSAEAFNOSUPPORT
# define WS_EAI_MEMORY	WSA_NOT_ENOUGH_MEMORY
# define WS_EAI_NODATA	WS_EAI_NONAME
# define WS_EAI_NONAME	WSAHOST_NOT_FOUND
# define WS_EAI_SERVICE	WSATYPE_NOT_FOUND
# define WS_EAI_SOCKTYPE	WSAESOCKTNOSUPPORT
#endif

/* Possible Windows flags for getnameinfo() */
#ifndef USE_WS_PREFIX
# define NI_NOFQDN          0x01
# define NI_NUMERICHOST     0x02
# define NI_NAMEREQD        0x04
# define NI_NUMERICSERV     0x08
# define NI_DGRAM           0x10
#else
# define WS_NI_NOFQDN       0x01
# define WS_NI_NUMERICHOST  0x02
# define WS_NI_NAMEREQD     0x04
# define WS_NI_NUMERICSERV  0x08
# define WS_NI_DGRAM        0x10
#endif


#ifdef __cplusplus
extern "C" {
#endif

void WINAPI WS(freeaddrinfo)(LPADDRINFO);
#define     FreeAddrInfoA WS(freeaddrinfo)
void WINAPI FreeAddrInfoW(PADDRINFOW);
#define     FreeAddrInfo WINELIB_NAME_AW(FreeAddrInfo)
int WINAPI  WS(getaddrinfo)(const char*,const char*,const struct WS(addrinfo)*,struct WS(addrinfo)**);
#define     GetAddrInfoA WS(getaddrinfo)
int WINAPI  GetAddrInfoW(PCWSTR,PCWSTR,const ADDRINFOW*,PADDRINFOW*);
#define     GetAddrInfo WINELIB_NAME_AW(GetAddrInfo)
int WINAPI  WS(getnameinfo)(const SOCKADDR*,WS(socklen_t),PCHAR,DWORD,PCHAR,DWORD,INT);
#define     GetNameInfoA WS(getnameinfo)
INT WINAPI  GetNameInfoW(const SOCKADDR*,WS(socklen_t),PWCHAR,DWORD,PWCHAR,DWORD,INT);
#define     GetNameInfo WINELIB_NAME_AW(GetNameInfo)

/*
 * Ws2tcpip Function Typedefs
 *
 * Remember to keep this section in sync with the
 * prototypes above.
 */
#if INCL_WINSOCK_API_TYPEDEFS

typedef void (WINAPI *LPFN_FREEADDRINFO)(LPADDRINFO);
#define LPFN_FREEADDRINFOA LPFN_FREEADDRINFO
typedef void (WINAPI *LPFN_FREEADDRINFOW)(PADDRINFOW);
#define LPFN_FREEADDRINFOT WINELIB_NAME_AW(LPFN_FREEADDRINFO)
typedef int (WINAPI *LPFN_GETADDRINFO)(const char*,const char*,const struct WS(addrinfo)*,struct WS(addrinfo)**);
#define LPFN_GETADDRINFOA LPFN_GETADDRINFO
typedef int (WINAPI *LPFN_GETADDRINFOW)(PCWSTR,PCWSTR,const ADDRINFOW*,PADDRINFOW*);
#define LPFN_GETADDRINFOT WINELIB_NAME_AW(LPFN_GETADDRINFO)
typedef int (WINAPI *LPFN_GETNAMEINFO)(const struct sockaddr*,socklen_t,char*,DWORD,char*,DWORD,int);
#define LPFN_GETNAMEINFOA LPFN_GETNAMEINFO
typedef int (WINAPI *LPFN_GETNAMEINFOW)(const SOCKADDR*,socklen_t,PWCHAR,DWORD,PWCHAR,DWORD,INT);
#define LPFN_GETNAMEINFOT WINELIB_NAME_AW(LPFN_GETNAMEINFO)

#endif

#ifdef __cplusplus
}
#endif

#endif /* __WS2TCPIP__ */
