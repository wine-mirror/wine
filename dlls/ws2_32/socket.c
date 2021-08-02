/*
 * based on Windows Sockets 1.1 specs
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 2001 Stefan Leichter
 * Copyright (C) 2004 Hans Leidekker
 * Copyright (C) 2005 Marcus Meissner
 * Copyright (C) 2006-2008 Kai Blin
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
 *
 * NOTE: If you make any changes to fix a particular app, make sure
 * they don't break something else like Netscape or telnet and ftp
 * clients and servers (www.winsite.com got a lot of those).
 */

#include "config.h"
#include "wine/port.h"

#include "ws2_32_private.h"

#if defined(linux) && !defined(IP_UNICAST_IF)
#define IP_UNICAST_IF 50
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)  || defined(__DragonFly__)
# define sipx_network    sipx_addr.x_net
# define sipx_node       sipx_addr.x_host.c_host
#endif  /* __FreeBSD__ */

#define FILE_USE_FILE_POINTER_POSITION ((LONGLONG)-2)

WINE_DEFAULT_DEBUG_CHANNEL(winsock);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const WSAPROTOCOL_INFOW supported_protocols[] =
{
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_EXPEDITED_DATA | XP1_GRACEFUL_CLOSE
                | XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0xe70f1aa0, 0xab8b, 0x11cf, {0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1001,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = WS_AF_INET,
        .iMaxSockAddr = sizeof(struct WS_sockaddr_in),
        .iMinSockAddr = sizeof(struct WS_sockaddr_in),
        .iSocketType = WS_SOCK_STREAM,
        .iProtocol = WS_IPPROTO_TCP,
        .szProtocol = {'T','C','P','/','I','P',0},
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0xe70f1aa0, 0xab8b, 0x11cf, {0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1002,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = WS_AF_INET,
        .iMaxSockAddr = sizeof(struct WS_sockaddr_in),
        .iMinSockAddr = sizeof(struct WS_sockaddr_in),
        .iSocketType = WS_SOCK_DGRAM,
        .iProtocol = WS_IPPROTO_UDP,
        .dwMessageSize = 0xffbb,
        .szProtocol = {'U','D','P','/','I','P',0},
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_EXPEDITED_DATA | XP1_GRACEFUL_CLOSE
                | XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0xf9eab0c0, 0x26d4, 0x11d0, {0xbb, 0xbf, 0x00, 0xaa, 0x00, 0x6c, 0x34, 0xe4}},
        .dwCatalogEntryId = 1004,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = WS_AF_INET6,
        .iMaxSockAddr = sizeof(struct WS_sockaddr_in6),
        .iMinSockAddr = sizeof(struct WS_sockaddr_in6),
        .iSocketType = WS_SOCK_STREAM,
        .iProtocol = WS_IPPROTO_TCP,
        .szProtocol = {'T','C','P','/','I','P','v','6',0},
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0xf9eab0c0, 0x26d4, 0x11d0, {0xbb, 0xbf, 0x00, 0xaa, 0x00, 0x6c, 0x34, 0xe4}},
        .dwCatalogEntryId = 1005,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = WS_AF_INET6,
        .iMaxSockAddr = sizeof(struct WS_sockaddr_in6),
        .iMinSockAddr = sizeof(struct WS_sockaddr_in6),
        .iSocketType = WS_SOCK_DGRAM,
        .iProtocol = WS_IPPROTO_UDP,
        .dwMessageSize = 0xffbb,
        .szProtocol = {'U','D','P','/','I','P','v','6',0},
    },
    {
        .dwServiceFlags1 = XP1_PARTIAL_MESSAGE | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0x11058240, 0xbe47, 0x11cf, {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1030,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = WS_AF_IPX,
        .iMaxSockAddr = sizeof(struct WS_sockaddr),
        .iMinSockAddr = sizeof(struct WS_sockaddr_ipx),
        .iSocketType = WS_SOCK_DGRAM,
        .iProtocol = WS_NSPROTO_IPX,
        .iProtocolMaxOffset = 255,
        .dwMessageSize = 0x240,
        .szProtocol = {'I','P','X',0},
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_PSEUDO_STREAM | XP1_MESSAGE_ORIENTED
                | XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0x11058241, 0xbe47, 0x11cf, {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1031,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = WS_AF_IPX,
        .iMaxSockAddr = sizeof(struct WS_sockaddr),
        .iMinSockAddr = sizeof(struct WS_sockaddr_ipx),
        .iSocketType = WS_SOCK_SEQPACKET,
        .iProtocol = WS_NSPROTO_SPX,
        .dwMessageSize = UINT_MAX,
        .szProtocol = {'S','P','X',0},
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_GRACEFUL_CLOSE | XP1_PSEUDO_STREAM
                | XP1_MESSAGE_ORIENTED | XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0x11058241, 0xbe47, 0x11cf, {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1033,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = WS_AF_IPX,
        .iMaxSockAddr = sizeof(struct WS_sockaddr),
        .iMinSockAddr = sizeof(struct WS_sockaddr_ipx),
        .iSocketType = WS_SOCK_SEQPACKET,
        .iProtocol = WS_NSPROTO_SPXII,
        .dwMessageSize = UINT_MAX,
        .szProtocol = {'S','P','X',' ','I','I',0},
    },
};

DECLARE_CRITICAL_SECTION(cs_socket_list);

static SOCKET *socket_list;
static unsigned int socket_list_size;

const char *debugstr_sockaddr( const struct WS_sockaddr *a )
{
    if (!a) return "(nil)";
    switch (a->sa_family)
    {
    case WS_AF_INET:
    {
        char buf[16];
        const char *p;
        struct WS_sockaddr_in *sin = (struct WS_sockaddr_in *)a;

        p = WS_inet_ntop( WS_AF_INET, &sin->sin_addr, buf, sizeof(buf) );
        if (!p)
            p = "(unknown IPv4 address)";

        return wine_dbg_sprintf("{ family AF_INET, address %s, port %d }",
                                p, ntohs(sin->sin_port));
    }
    case WS_AF_INET6:
    {
        char buf[46];
        const char *p;
        struct WS_sockaddr_in6 *sin = (struct WS_sockaddr_in6 *)a;

        p = WS_inet_ntop( WS_AF_INET6, &sin->sin6_addr, buf, sizeof(buf) );
        if (!p)
            p = "(unknown IPv6 address)";
        return wine_dbg_sprintf("{ family AF_INET6, address %s, flow label %#x, port %d, scope %u }",
                                p, sin->sin6_flowinfo, ntohs(sin->sin6_port), sin->sin6_scope_id );
    }
    case WS_AF_IPX:
    {
        int i;
        char netnum[16], nodenum[16];
        struct WS_sockaddr_ipx *sin = (struct WS_sockaddr_ipx *)a;

        for (i = 0;i < 4; i++) sprintf(netnum + i * 2, "%02X", (unsigned char) sin->sa_netnum[i]);
        for (i = 0;i < 6; i++) sprintf(nodenum + i * 2, "%02X", (unsigned char) sin->sa_nodenum[i]);

        return wine_dbg_sprintf("{ family AF_IPX, address %s.%s, ipx socket %d }",
                                netnum, nodenum, sin->sa_socket);
    }
    case WS_AF_IRDA:
    {
        DWORD addr;

        memcpy( &addr, ((const SOCKADDR_IRDA *)a)->irdaDeviceID, sizeof(addr) );
        addr = ntohl( addr );
        return wine_dbg_sprintf("{ family AF_IRDA, addr %08x, name %s }",
                                addr,
                                ((const SOCKADDR_IRDA *)a)->irdaServiceName);
    }
    default:
        return wine_dbg_sprintf("{ family %d }", a->sa_family);
    }
}

static inline const char *debugstr_sockopt(int level, int optname)
{
    const char *stropt = NULL, *strlevel = NULL;

#define DEBUG_SOCKLEVEL(x) case (x): strlevel = #x
#define DEBUG_SOCKOPT(x) case (x): stropt = #x; break

    switch(level)
    {
        DEBUG_SOCKLEVEL(WS_SOL_SOCKET);
        switch(optname)
        {
            DEBUG_SOCKOPT(WS_SO_ACCEPTCONN);
            DEBUG_SOCKOPT(WS_SO_BROADCAST);
            DEBUG_SOCKOPT(WS_SO_BSP_STATE);
            DEBUG_SOCKOPT(WS_SO_CONDITIONAL_ACCEPT);
            DEBUG_SOCKOPT(WS_SO_CONNECT_TIME);
            DEBUG_SOCKOPT(WS_SO_DEBUG);
            DEBUG_SOCKOPT(WS_SO_DONTLINGER);
            DEBUG_SOCKOPT(WS_SO_DONTROUTE);
            DEBUG_SOCKOPT(WS_SO_ERROR);
            DEBUG_SOCKOPT(WS_SO_EXCLUSIVEADDRUSE);
            DEBUG_SOCKOPT(WS_SO_GROUP_ID);
            DEBUG_SOCKOPT(WS_SO_GROUP_PRIORITY);
            DEBUG_SOCKOPT(WS_SO_KEEPALIVE);
            DEBUG_SOCKOPT(WS_SO_LINGER);
            DEBUG_SOCKOPT(WS_SO_MAX_MSG_SIZE);
            DEBUG_SOCKOPT(WS_SO_OOBINLINE);
            DEBUG_SOCKOPT(WS_SO_OPENTYPE);
            DEBUG_SOCKOPT(WS_SO_PROTOCOL_INFOA);
            DEBUG_SOCKOPT(WS_SO_PROTOCOL_INFOW);
            DEBUG_SOCKOPT(WS_SO_RCVBUF);
            DEBUG_SOCKOPT(WS_SO_RCVTIMEO);
            DEBUG_SOCKOPT(WS_SO_REUSEADDR);
            DEBUG_SOCKOPT(WS_SO_SNDBUF);
            DEBUG_SOCKOPT(WS_SO_SNDTIMEO);
            DEBUG_SOCKOPT(WS_SO_TYPE);
            DEBUG_SOCKOPT(WS_SO_UPDATE_CONNECT_CONTEXT);
        }
        break;

        DEBUG_SOCKLEVEL(WS_NSPROTO_IPX);
        switch(optname)
        {
            DEBUG_SOCKOPT(WS_IPX_PTYPE);
            DEBUG_SOCKOPT(WS_IPX_FILTERPTYPE);
            DEBUG_SOCKOPT(WS_IPX_DSTYPE);
            DEBUG_SOCKOPT(WS_IPX_RECVHDR);
            DEBUG_SOCKOPT(WS_IPX_MAXSIZE);
            DEBUG_SOCKOPT(WS_IPX_ADDRESS);
            DEBUG_SOCKOPT(WS_IPX_MAX_ADAPTER_NUM);
        }
        break;

        DEBUG_SOCKLEVEL(WS_SOL_IRLMP);
        switch(optname)
        {
            DEBUG_SOCKOPT(WS_IRLMP_ENUMDEVICES);
        }
        break;

        DEBUG_SOCKLEVEL(WS_IPPROTO_TCP);
        switch(optname)
        {
            DEBUG_SOCKOPT(WS_TCP_BSDURGENT);
            DEBUG_SOCKOPT(WS_TCP_EXPEDITED_1122);
            DEBUG_SOCKOPT(WS_TCP_NODELAY);
        }
        break;

        DEBUG_SOCKLEVEL(WS_IPPROTO_IP);
        switch(optname)
        {
            DEBUG_SOCKOPT(WS_IP_ADD_MEMBERSHIP);
            DEBUG_SOCKOPT(WS_IP_DONTFRAGMENT);
            DEBUG_SOCKOPT(WS_IP_DROP_MEMBERSHIP);
            DEBUG_SOCKOPT(WS_IP_HDRINCL);
            DEBUG_SOCKOPT(WS_IP_MULTICAST_IF);
            DEBUG_SOCKOPT(WS_IP_MULTICAST_LOOP);
            DEBUG_SOCKOPT(WS_IP_MULTICAST_TTL);
            DEBUG_SOCKOPT(WS_IP_OPTIONS);
            DEBUG_SOCKOPT(WS_IP_PKTINFO);
            DEBUG_SOCKOPT(WS_IP_RECEIVE_BROADCAST);
            DEBUG_SOCKOPT(WS_IP_TOS);
            DEBUG_SOCKOPT(WS_IP_TTL);
            DEBUG_SOCKOPT(WS_IP_UNICAST_IF);
        }
        break;

        DEBUG_SOCKLEVEL(WS_IPPROTO_IPV6);
        switch(optname)
        {
            DEBUG_SOCKOPT(WS_IPV6_ADD_MEMBERSHIP);
            DEBUG_SOCKOPT(WS_IPV6_DROP_MEMBERSHIP);
            DEBUG_SOCKOPT(WS_IPV6_MULTICAST_IF);
            DEBUG_SOCKOPT(WS_IPV6_MULTICAST_HOPS);
            DEBUG_SOCKOPT(WS_IPV6_MULTICAST_LOOP);
            DEBUG_SOCKOPT(WS_IPV6_UNICAST_HOPS);
            DEBUG_SOCKOPT(WS_IPV6_V6ONLY);
            DEBUG_SOCKOPT(WS_IPV6_UNICAST_IF);
            DEBUG_SOCKOPT(WS_IPV6_DONTFRAG);
        }
        break;
    }
#undef DEBUG_SOCKLEVEL
#undef DEBUG_SOCKOPT

    if (!strlevel)
        strlevel = wine_dbg_sprintf("WS_0x%x", level);
    if (!stropt)
        stropt = wine_dbg_sprintf("WS_0x%x", optname);

    return wine_dbg_sprintf("level %s, name %s", strlevel + 3, stropt + 3);
}

static inline const char *debugstr_optval(const char *optval, int optlenval)
{
    if (optval && !IS_INTRESOURCE(optval) && optlenval >= 1 && optlenval <= sizeof(DWORD))
    {
        DWORD value = 0;
        memcpy(&value, optval, optlenval);
        return wine_dbg_sprintf("%p (%u)", optval, value);
    }
    return wine_dbg_sprintf("%p", optval);
}

/* HANDLE<->SOCKET conversion (SOCKET is UINT_PTR). */
#define SOCKET2HANDLE(s) ((HANDLE)(s))
#define HANDLE2SOCKET(h) ((SOCKET)(h))

static BOOL socket_list_add(SOCKET socket)
{
    unsigned int i, new_size;
    SOCKET *new_array;

    EnterCriticalSection(&cs_socket_list);
    for (i = 0; i < socket_list_size; ++i)
    {
        if (!socket_list[i])
        {
            socket_list[i] = socket;
            LeaveCriticalSection(&cs_socket_list);
            return TRUE;
        }
    }
    new_size = max(socket_list_size * 2, 8);
    if (!(new_array = heap_realloc(socket_list, new_size * sizeof(*socket_list))))
    {
        LeaveCriticalSection(&cs_socket_list);
        return FALSE;
    }
    socket_list = new_array;
    memset(socket_list + socket_list_size, 0, (new_size - socket_list_size) * sizeof(*socket_list));
    socket_list[socket_list_size] = socket;
    socket_list_size = new_size;
    LeaveCriticalSection(&cs_socket_list);
    return TRUE;
}


static BOOL socket_list_find( SOCKET socket )
{
    unsigned int i;

    EnterCriticalSection( &cs_socket_list );
    for (i = 0; i < socket_list_size; ++i)
    {
        if (socket_list[i] == socket)
        {
            LeaveCriticalSection( &cs_socket_list );
            return TRUE;
        }
    }
    LeaveCriticalSection( &cs_socket_list );
    return FALSE;
}


static void socket_list_remove(SOCKET socket)
{
    unsigned int i;

    EnterCriticalSection(&cs_socket_list);
    for (i = 0; i < socket_list_size; ++i)
    {
        if (socket_list[i] == socket)
        {
            socket_list[i] = 0;
            break;
        }
    }
    LeaveCriticalSection(&cs_socket_list);
}

#define WS_MAX_SOCKETS_PER_PROCESS      128     /* reasonable guess */
#define WS_MAX_UDP_DATAGRAM             1024
static INT WINAPI WSA_DefaultBlockingHook( FARPROC x );

int num_startup;
static FARPROC blocking_hook = (FARPROC)WSA_DefaultBlockingHook;

/* function prototypes */
static int ws_protocol_info(SOCKET s, int unicode, WSAPROTOCOL_INFOW *buffer, int *size);

#define MAP_OPTION(opt) { WS_##opt, opt }

static const int ws_sock_map[][2] =
{
    MAP_OPTION( SO_DEBUG ),
    MAP_OPTION( SO_ACCEPTCONN ),
    MAP_OPTION( SO_REUSEADDR ),
    MAP_OPTION( SO_KEEPALIVE ),
    MAP_OPTION( SO_DONTROUTE ),
    MAP_OPTION( SO_BROADCAST ),
    MAP_OPTION( SO_LINGER ),
    MAP_OPTION( SO_OOBINLINE ),
    MAP_OPTION( SO_SNDBUF ),
    MAP_OPTION( SO_RCVBUF ),
    MAP_OPTION( SO_ERROR ),
    MAP_OPTION( SO_TYPE ),
#ifdef SO_RCVTIMEO
    MAP_OPTION( SO_RCVTIMEO ),
#endif
#ifdef SO_SNDTIMEO
    MAP_OPTION( SO_SNDTIMEO ),
#endif
};

static const int ws_tcp_map[][2] =
{
#ifdef TCP_NODELAY
    MAP_OPTION( TCP_NODELAY ),
#endif
};

static const int ws_ip_map[][2] =
{
    MAP_OPTION( IP_MULTICAST_IF ),
    MAP_OPTION( IP_MULTICAST_TTL ),
    MAP_OPTION( IP_MULTICAST_LOOP ),
    MAP_OPTION( IP_ADD_MEMBERSHIP ),
    MAP_OPTION( IP_DROP_MEMBERSHIP ),
    MAP_OPTION( IP_ADD_SOURCE_MEMBERSHIP ),
    MAP_OPTION( IP_DROP_SOURCE_MEMBERSHIP ),
    MAP_OPTION( IP_BLOCK_SOURCE ),
    MAP_OPTION( IP_UNBLOCK_SOURCE ),
    MAP_OPTION( IP_OPTIONS ),
#ifdef IP_HDRINCL
    MAP_OPTION( IP_HDRINCL ),
#endif
    MAP_OPTION( IP_TOS ),
    MAP_OPTION( IP_TTL ),
#if defined(IP_PKTINFO)
    MAP_OPTION( IP_PKTINFO ),
#elif defined(IP_RECVDSTADDR)
    { WS_IP_PKTINFO, IP_RECVDSTADDR },
#endif
#ifdef IP_UNICAST_IF
    MAP_OPTION( IP_UNICAST_IF ),
#endif
};

static const int ws_ipv6_map[][2] =
{
#ifdef IPV6_ADD_MEMBERSHIP
    MAP_OPTION( IPV6_ADD_MEMBERSHIP ),
#endif
#ifdef IPV6_DROP_MEMBERSHIP
    MAP_OPTION( IPV6_DROP_MEMBERSHIP ),
#endif
    MAP_OPTION( IPV6_MULTICAST_IF ),
    MAP_OPTION( IPV6_MULTICAST_HOPS ),
    MAP_OPTION( IPV6_MULTICAST_LOOP ),
    MAP_OPTION( IPV6_UNICAST_HOPS ),
    MAP_OPTION( IPV6_V6ONLY ),
#ifdef IPV6_UNICAST_IF
    MAP_OPTION( IPV6_UNICAST_IF ),
#endif
};

static const int ws_socktype_map[][2] =
{
    MAP_OPTION( SOCK_DGRAM ),
    MAP_OPTION( SOCK_STREAM ),
    MAP_OPTION( SOCK_RAW ),
    {FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO},
};

UINT sock_get_error( int err )
{
	switch(err)
    {
	case EINTR:		return WSAEINTR;
	case EPERM:
	case EACCES:		return WSAEACCES;
	case EFAULT:		return WSAEFAULT;
	case EINVAL:		return WSAEINVAL;
	case EMFILE:		return WSAEMFILE;
	case EINPROGRESS:
	case EWOULDBLOCK:	return WSAEWOULDBLOCK;
	case EALREADY:		return WSAEALREADY;
	case EBADF:
	case ENOTSOCK:		return WSAENOTSOCK;
	case EDESTADDRREQ:	return WSAEDESTADDRREQ;
	case EMSGSIZE:		return WSAEMSGSIZE;
	case EPROTOTYPE:	return WSAEPROTOTYPE;
	case ENOPROTOOPT:	return WSAENOPROTOOPT;
	case EPROTONOSUPPORT:	return WSAEPROTONOSUPPORT;
	case ESOCKTNOSUPPORT:	return WSAESOCKTNOSUPPORT;
	case EOPNOTSUPP:	return WSAEOPNOTSUPP;
	case EPFNOSUPPORT:	return WSAEPFNOSUPPORT;
	case EAFNOSUPPORT:	return WSAEAFNOSUPPORT;
	case EADDRINUSE:	return WSAEADDRINUSE;
	case EADDRNOTAVAIL:	return WSAEADDRNOTAVAIL;
	case ENETDOWN:		return WSAENETDOWN;
	case ENETUNREACH:	return WSAENETUNREACH;
	case ENETRESET:		return WSAENETRESET;
	case ECONNABORTED:	return WSAECONNABORTED;
	case EPIPE:
	case ECONNRESET:	return WSAECONNRESET;
	case ENOBUFS:		return WSAENOBUFS;
	case EISCONN:		return WSAEISCONN;
	case ENOTCONN:		return WSAENOTCONN;
	case ESHUTDOWN:		return WSAESHUTDOWN;
	case ETOOMANYREFS:	return WSAETOOMANYREFS;
	case ETIMEDOUT:		return WSAETIMEDOUT;
	case ECONNREFUSED:	return WSAECONNREFUSED;
	case ELOOP:		return WSAELOOP;
	case ENAMETOOLONG:	return WSAENAMETOOLONG;
	case EHOSTDOWN:		return WSAEHOSTDOWN;
	case EHOSTUNREACH:	return WSAEHOSTUNREACH;
	case ENOTEMPTY:		return WSAENOTEMPTY;
#ifdef EPROCLIM
	case EPROCLIM:		return WSAEPROCLIM;
#endif
#ifdef EUSERS
	case EUSERS:		return WSAEUSERS;
#endif
#ifdef EDQUOT
	case EDQUOT:		return WSAEDQUOT;
#endif
#ifdef ESTALE
	case ESTALE:		return WSAESTALE;
#endif
#ifdef EREMOTE
	case EREMOTE:		return WSAEREMOTE;
#endif

	/* just in case we ever get here and there are no problems */
	case 0:			return 0;
	default:
		WARN("Unknown errno %d!\n", err);
		return WSAEOPNOTSUPP;
    }
}

static UINT wsaErrno(void)
{
    int	loc_errno = errno;
    WARN("errno %d, (%s).\n", loc_errno, strerror(loc_errno));

    return sock_get_error( loc_errno );
}

static DWORD NtStatusToWSAError( NTSTATUS status )
{
    static const struct
    {
        NTSTATUS status;
        DWORD error;
    }
    errors[] =
    {
        {STATUS_PENDING,                    ERROR_IO_PENDING},

        {STATUS_BUFFER_OVERFLOW,            WSAEMSGSIZE},

        {STATUS_NOT_IMPLEMENTED,            WSAEOPNOTSUPP},
        {STATUS_ACCESS_VIOLATION,           WSAEFAULT},
        {STATUS_PAGEFILE_QUOTA,             WSAENOBUFS},
        {STATUS_INVALID_HANDLE,             WSAENOTSOCK},
        {STATUS_NO_SUCH_DEVICE,             WSAENETDOWN},
        {STATUS_NO_SUCH_FILE,               WSAENETDOWN},
        {STATUS_NO_MEMORY,                  WSAENOBUFS},
        {STATUS_CONFLICTING_ADDRESSES,      WSAENOBUFS},
        {STATUS_ACCESS_DENIED,              WSAEACCES},
        {STATUS_BUFFER_TOO_SMALL,           WSAEFAULT},
        {STATUS_OBJECT_TYPE_MISMATCH,       WSAENOTSOCK},
        {STATUS_OBJECT_NAME_NOT_FOUND,      WSAENETDOWN},
        {STATUS_OBJECT_PATH_NOT_FOUND,      WSAENETDOWN},
        {STATUS_SHARING_VIOLATION,          WSAEADDRINUSE},
        {STATUS_QUOTA_EXCEEDED,             WSAENOBUFS},
        {STATUS_TOO_MANY_PAGING_FILES,      WSAENOBUFS},
        {STATUS_INSUFFICIENT_RESOURCES,     WSAENOBUFS},
        {STATUS_WORKING_SET_QUOTA,          WSAENOBUFS},
        {STATUS_DEVICE_NOT_READY,           WSAEWOULDBLOCK},
        {STATUS_PIPE_DISCONNECTED,          WSAESHUTDOWN},
        {STATUS_IO_TIMEOUT,                 WSAETIMEDOUT},
        {STATUS_NOT_SUPPORTED,              WSAEOPNOTSUPP},
        {STATUS_REMOTE_NOT_LISTENING,       WSAECONNREFUSED},
        {STATUS_BAD_NETWORK_PATH,           WSAENETUNREACH},
        {STATUS_NETWORK_BUSY,               WSAENETDOWN},
        {STATUS_INVALID_NETWORK_RESPONSE,   WSAENETDOWN},
        {STATUS_UNEXPECTED_NETWORK_ERROR,   WSAENETDOWN},
        {STATUS_REQUEST_NOT_ACCEPTED,       WSAEWOULDBLOCK},
        {STATUS_CANCELLED,                  ERROR_OPERATION_ABORTED},
        {STATUS_COMMITMENT_LIMIT,           WSAENOBUFS},
        {STATUS_LOCAL_DISCONNECT,           WSAECONNABORTED},
        {STATUS_REMOTE_DISCONNECT,          WSAECONNRESET},
        {STATUS_REMOTE_RESOURCES,           WSAENOBUFS},
        {STATUS_LINK_FAILED,                WSAECONNRESET},
        {STATUS_LINK_TIMEOUT,               WSAETIMEDOUT},
        {STATUS_INVALID_CONNECTION,         WSAENOTCONN},
        {STATUS_INVALID_ADDRESS,            WSAEADDRNOTAVAIL},
        {STATUS_INVALID_BUFFER_SIZE,        WSAEMSGSIZE},
        {STATUS_INVALID_ADDRESS_COMPONENT,  WSAEADDRNOTAVAIL},
        {STATUS_TOO_MANY_ADDRESSES,         WSAENOBUFS},
        {STATUS_ADDRESS_ALREADY_EXISTS,     WSAEADDRINUSE},
        {STATUS_CONNECTION_DISCONNECTED,    WSAECONNRESET},
        {STATUS_CONNECTION_RESET,           WSAECONNRESET},
        {STATUS_TRANSACTION_ABORTED,        WSAECONNABORTED},
        {STATUS_CONNECTION_REFUSED,         WSAECONNREFUSED},
        {STATUS_GRACEFUL_DISCONNECT,        WSAEDISCON},
        {STATUS_CONNECTION_ACTIVE,          WSAEISCONN},
        {STATUS_NETWORK_UNREACHABLE,        WSAENETUNREACH},
        {STATUS_HOST_UNREACHABLE,           WSAEHOSTUNREACH},
        {STATUS_PROTOCOL_UNREACHABLE,       WSAENETUNREACH},
        {STATUS_PORT_UNREACHABLE,           WSAECONNRESET},
        {STATUS_REQUEST_ABORTED,            WSAEINTR},
        {STATUS_CONNECTION_ABORTED,         WSAECONNABORTED},
        {STATUS_DATATYPE_MISALIGNMENT_ERROR,WSAEFAULT},
        {STATUS_HOST_DOWN,                  WSAEHOSTDOWN},
        {0x80070000 | ERROR_IO_INCOMPLETE,  ERROR_IO_INCOMPLETE},
        {0xc0010000 | ERROR_IO_INCOMPLETE,  ERROR_IO_INCOMPLETE},
        {0xc0070000 | ERROR_IO_INCOMPLETE,  ERROR_IO_INCOMPLETE},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(errors); ++i)
    {
        if (errors[i].status == status)
            return errors[i].error;
    }

    return NT_SUCCESS(status) ? RtlNtStatusToDosErrorNoTeb(status) : WSAEINVAL;
}

/* set last error code from NT status without mapping WSA errors */
static inline unsigned int set_error( unsigned int err )
{
    if (err)
    {
        err = NtStatusToWSAError( err );
        SetLastError( err );
    }
    return err;
}

static inline int get_sock_fd( SOCKET s, DWORD access, unsigned int *options )
{
    int fd;
    if (set_error( wine_server_handle_to_fd( SOCKET2HANDLE(s), access, &fd, options ) ))
        return -1;
    return fd;
}

static inline void release_sock_fd( SOCKET s, int fd )
{
    close( fd );
}

struct per_thread_data *get_per_thread_data(void)
{
    struct per_thread_data * ptb = NtCurrentTeb()->WinSockData;
    /* lazy initialization */
    if (!ptb)
    {
        ptb = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ptb) );
        NtCurrentTeb()->WinSockData = ptb;
    }
    return ptb;
}

static void free_per_thread_data(void)
{
    struct per_thread_data * ptb = NtCurrentTeb()->WinSockData;

    if (!ptb) return;

    CloseHandle( ptb->sync_event );

    /* delete scratch buffers */
    HeapFree( GetProcessHeap(), 0, ptb->he_buffer );
    HeapFree( GetProcessHeap(), 0, ptb->se_buffer );
    HeapFree( GetProcessHeap(), 0, ptb->pe_buffer );

    HeapFree( GetProcessHeap(), 0, ptb );
    NtCurrentTeb()->WinSockData = NULL;
}

static HANDLE get_sync_event(void)
{
    struct per_thread_data *data;

    if (!(data = get_per_thread_data())) return NULL;
    if (!data->sync_event)
        data->sync_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    return data->sync_event;
}

/***********************************************************************
 *		DllMain (WS2_32.init)
 */
BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    if (reason == DLL_THREAD_DETACH)
        free_per_thread_data();
    return TRUE;
}

/***********************************************************************
 *          convert_sockopt()
 *
 * Converts socket flags from Windows format.
 * Return 1 if converted, 0 if not (error).
 */
static int convert_sockopt(INT *level, INT *optname)
{
  unsigned int i;
  switch (*level)
  {
     case WS_SOL_SOCKET:
        *level = SOL_SOCKET;
        for(i = 0; i < ARRAY_SIZE(ws_sock_map); i++) {
            if( ws_sock_map[i][0] == *optname )
            {
                *optname = ws_sock_map[i][1];
                return 1;
            }
        }
        FIXME("Unknown SOL_SOCKET optname 0x%x\n", *optname);
        break;
     case WS_IPPROTO_TCP:
        *level = IPPROTO_TCP;
        for(i = 0; i < ARRAY_SIZE(ws_tcp_map); i++) {
            if ( ws_tcp_map[i][0] == *optname )
            {
                *optname = ws_tcp_map[i][1];
                return 1;
            }
        }
        FIXME("Unknown IPPROTO_TCP optname 0x%x\n", *optname);
	break;
     case WS_IPPROTO_IP:
        *level = IPPROTO_IP;
        for(i = 0; i < ARRAY_SIZE(ws_ip_map); i++) {
            if (ws_ip_map[i][0] == *optname )
            {
                *optname = ws_ip_map[i][1];
                return 1;
            }
        }
	FIXME("Unknown IPPROTO_IP optname 0x%x\n", *optname);
	break;
     case WS_IPPROTO_IPV6:
        *level = IPPROTO_IPV6;
        for(i = 0; i < ARRAY_SIZE(ws_ipv6_map); i++) {
            if (ws_ipv6_map[i][0] == *optname )
            {
                *optname = ws_ipv6_map[i][1];
                return 1;
            }
        }
	FIXME("Unknown IPPROTO_IPV6 optname 0x%x\n", *optname);
	break;
     default: FIXME("Unimplemented or unknown socket level\n");
  }
  return 0;
}

int
convert_socktype_w2u(int windowssocktype) {
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ws_socktype_map); i++)
    	if (ws_socktype_map[i][0] == windowssocktype)
	    return ws_socktype_map[i][1];
    FIXME("unhandled Windows socket type %d\n", windowssocktype);
    return -1;
}

int
convert_socktype_u2w(int unixsocktype) {
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ws_socktype_map); i++)
    	if (ws_socktype_map[i][1] == unixsocktype)
	    return ws_socktype_map[i][0];
    FIXME("unhandled UNIX socket type %d\n", unixsocktype);
    return -1;
}

static int set_ipx_packettype(int sock, int ptype)
{
#ifdef HAS_IPX
    int fd = get_sock_fd( sock, 0, NULL ), ret = 0;
    TRACE("trying to set IPX_PTYPE: %d (fd: %d)\n", ptype, fd);

    if (fd == -1) return SOCKET_ERROR;

    /* We try to set the ipx type on ipx socket level. */
#ifdef SOL_IPX
    if(setsockopt(fd, SOL_IPX, IPX_TYPE, &ptype, sizeof(ptype)) == -1)
    {
        ERR("IPX: could not set ipx option type; expect weird behaviour\n");
        ret = SOCKET_ERROR;
    }
#else
    {
        struct ipx val;
        /* Should we retrieve val using a getsockopt call and then
         * set the modified one? */
        val.ipx_pt = ptype;
        setsockopt(fd, 0, SO_DEFAULT_HEADERS, &val, sizeof(struct ipx));
    }
#endif
    release_sock_fd( sock, fd );
    return ret;
#else
    WARN("IPX support is not enabled, can't set packet type\n");
    return SOCKET_ERROR;
#endif
}

/* ----------------------------------- API -----
 *
 * Init / cleanup / error checking.
 */

/***********************************************************************
 *      WSAStartup		(WS2_32.115)
 */
int WINAPI WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData)
{
    TRACE("verReq=%x\n", wVersionRequested);

    if (LOBYTE(wVersionRequested) < 1)
        return WSAVERNOTSUPPORTED;

    if (!lpWSAData) return WSAEINVAL;

    num_startup++;

    /* that's the whole of the negotiation for now */
    lpWSAData->wVersion = wVersionRequested;
    /* return winsock information */
    lpWSAData->wHighVersion = 0x0202;
    strcpy(lpWSAData->szDescription, "WinSock 2.0" );
    strcpy(lpWSAData->szSystemStatus, "Running" );
    lpWSAData->iMaxSockets = WS_MAX_SOCKETS_PER_PROCESS;
    lpWSAData->iMaxUdpDg = WS_MAX_UDP_DATAGRAM;
    /* don't do anything with lpWSAData->lpVendorInfo */
    /* (some apps don't allocate the space for this field) */

    TRACE("succeeded starts: %d\n", num_startup);
    return 0;
}


/***********************************************************************
 *      WSACleanup			(WS2_32.116)
 */
INT WINAPI WSACleanup(void)
{
    TRACE("decreasing startup count from %d\n", num_startup);
    if (num_startup)
    {
        if (!--num_startup)
        {
            unsigned int i;

            for (i = 0; i < socket_list_size; ++i)
                CloseHandle(SOCKET2HANDLE(socket_list[i]));
            memset(socket_list, 0, socket_list_size * sizeof(*socket_list));
        }
        return 0;
    }
    SetLastError(WSANOTINITIALISED);
    return SOCKET_ERROR;
}


/***********************************************************************
 *      WSAGetLastError		(WS2_32.111)
 */
INT WINAPI WSAGetLastError(void)
{
	return GetLastError();
}

/***********************************************************************
 *      WSASetLastError		(WS2_32.112)
 */
void WINAPI WSASetLastError(INT iError) {
    SetLastError(iError);
}

static inline BOOL supported_pf(int pf)
{
    switch (pf)
    {
    case WS_AF_INET:
    case WS_AF_INET6:
        return TRUE;
#ifdef HAS_IPX
    case WS_AF_IPX:
        return TRUE;
#endif
#ifdef HAS_IRDA
    case WS_AF_IRDA:
        return TRUE;
#endif
    default:
        return FALSE;
    }
}

/**********************************************************************/

/* Returns the length of the converted address if successful, 0 if it was too
 * small to start with or unknown family or invalid address buffer.
 */
unsigned int ws_sockaddr_ws2u( const struct WS_sockaddr *wsaddr, int wsaddrlen,
                               union generic_unix_sockaddr *uaddr )
{
    unsigned int uaddrlen = 0;

    if (!wsaddr)
        return 0;

    switch (wsaddr->sa_family)
    {
#ifdef HAS_IPX
    case WS_AF_IPX:
        {
            const struct WS_sockaddr_ipx* wsipx=(const struct WS_sockaddr_ipx*)wsaddr;
            struct sockaddr_ipx* uipx = (struct sockaddr_ipx *)uaddr;

            if (wsaddrlen<sizeof(struct WS_sockaddr_ipx))
                return 0;

            uaddrlen = sizeof(struct sockaddr_ipx);
            memset( uaddr, 0, uaddrlen );
            uipx->sipx_family=AF_IPX;
            uipx->sipx_port=wsipx->sa_socket;
            /* copy sa_netnum and sa_nodenum to sipx_network and sipx_node
             * in one go
             */
            memcpy(&uipx->sipx_network,wsipx->sa_netnum,sizeof(uipx->sipx_network)+sizeof(uipx->sipx_node));
#ifdef IPX_FRAME_NONE
            uipx->sipx_type=IPX_FRAME_NONE;
#endif
            break;
        }
#endif
    case WS_AF_INET6: {
        struct sockaddr_in6* uin6 = (struct sockaddr_in6 *)uaddr;
        const struct WS_sockaddr_in6* win6 = (const struct WS_sockaddr_in6*)wsaddr;

        /* Note: Windows has 2 versions of the sockaddr_in6 struct, one with
         * scope_id, one without.
         */
        if (wsaddrlen >= sizeof(struct WS_sockaddr_in6)) {
            uaddrlen = sizeof(struct sockaddr_in6);
            memset( uaddr, 0, uaddrlen );
            uin6->sin6_family   = AF_INET6;
            uin6->sin6_port     = win6->sin6_port;
            uin6->sin6_flowinfo = win6->sin6_flowinfo;
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
            uin6->sin6_scope_id = win6->sin6_scope_id;
#endif
            memcpy(&uin6->sin6_addr,&win6->sin6_addr,16); /* 16 bytes = 128 address bits */
            break;
        }
        FIXME("bad size %d for WS_sockaddr_in6\n",wsaddrlen);
        return 0;
    }
    case WS_AF_INET: {
        struct sockaddr_in* uin = (struct sockaddr_in *)uaddr;
        const struct WS_sockaddr_in* win = (const struct WS_sockaddr_in*)wsaddr;

        if (wsaddrlen<sizeof(struct WS_sockaddr_in))
            return 0;
        uaddrlen = sizeof(struct sockaddr_in);
        memset( uaddr, 0, uaddrlen );
        uin->sin_family = AF_INET;
        uin->sin_port   = win->sin_port;
        memcpy(&uin->sin_addr,&win->sin_addr,4); /* 4 bytes = 32 address bits */
        break;
    }
#ifdef HAS_IRDA
    case WS_AF_IRDA: {
        struct sockaddr_irda *uin = (struct sockaddr_irda *)uaddr;
        const SOCKADDR_IRDA *win = (const SOCKADDR_IRDA *)wsaddr;

        if (wsaddrlen < sizeof(SOCKADDR_IRDA))
            return 0;
        uaddrlen = sizeof(struct sockaddr_irda);
        memset( uaddr, 0, uaddrlen );
        uin->sir_family = AF_IRDA;
        if (!strncmp( win->irdaServiceName, "LSAP-SEL", strlen( "LSAP-SEL" ) ))
        {
            unsigned int lsap_sel = 0;

            sscanf( win->irdaServiceName, "LSAP-SEL%u", &lsap_sel );
            uin->sir_lsap_sel = lsap_sel;
        }
        else
        {
            uin->sir_lsap_sel = LSAP_ANY;
            memcpy( uin->sir_name, win->irdaServiceName, 25 );
        }
        memcpy( &uin->sir_addr, win->irdaDeviceID, sizeof(uin->sir_addr) );
        break;
    }
#endif
    case WS_AF_UNSPEC: {
        /* Try to determine the needed space by the passed windows sockaddr space */
        switch (wsaddrlen) {
        default: /* likely an ipv4 address */
        case sizeof(struct WS_sockaddr_in):
            uaddrlen = sizeof(struct sockaddr_in);
            break;
#ifdef HAS_IPX
        case sizeof(struct WS_sockaddr_ipx):
            uaddrlen = sizeof(struct sockaddr_ipx);
            break;
#endif
#ifdef HAS_IRDA
        case sizeof(SOCKADDR_IRDA):
            uaddrlen = sizeof(struct sockaddr_irda);
            break;
#endif
        case sizeof(struct WS_sockaddr_in6):
        case sizeof(struct WS_sockaddr_in6_old):
            uaddrlen = sizeof(struct sockaddr_in6);
            break;
        }
        memset( uaddr, 0, uaddrlen );
        break;
    }
    default:
        FIXME("Unknown address family %d, return NULL.\n", wsaddr->sa_family);
        return 0;
    }
    return uaddrlen;
}

/* Returns 0 if successful, -1 if the buffer is too small */
int ws_sockaddr_u2ws(const struct sockaddr *uaddr, struct WS_sockaddr *wsaddr, int *wsaddrlen)
{
    int res;

    switch(uaddr->sa_family)
    {
#ifdef HAS_IPX
    case AF_IPX:
        {
            const struct sockaddr_ipx* uipx=(const struct sockaddr_ipx*)uaddr;
            struct WS_sockaddr_ipx* wsipx=(struct WS_sockaddr_ipx*)wsaddr;

            res=-1;
            switch (*wsaddrlen) /* how much can we copy? */
            {
            default:
                res=0; /* enough */
                *wsaddrlen = sizeof(*wsipx);
                wsipx->sa_socket=uipx->sipx_port;
                /* fall through */
            case 13:
            case 12:
                memcpy(wsipx->sa_nodenum,uipx->sipx_node,sizeof(wsipx->sa_nodenum));
                /* fall through */
            case 11:
            case 10:
            case 9:
            case 8:
            case 7:
            case 6:
                memcpy(wsipx->sa_netnum,&uipx->sipx_network,sizeof(wsipx->sa_netnum));
                /* fall through */
            case 5:
            case 4:
            case 3:
            case 2:
                wsipx->sa_family=WS_AF_IPX;
                /* fall through */
            case 1:
            case 0:
                /* way too small */
                break;
            }
        }
        break;
#endif
#ifdef HAS_IRDA
    case AF_IRDA: {
        const struct sockaddr_irda *uin = (const struct sockaddr_irda *)uaddr;
        SOCKADDR_IRDA *win = (SOCKADDR_IRDA *)wsaddr;

        if (*wsaddrlen < sizeof(SOCKADDR_IRDA))
            return -1;
        win->irdaAddressFamily = WS_AF_IRDA;
        memcpy( win->irdaDeviceID, &uin->sir_addr, sizeof(win->irdaDeviceID) );
        if (uin->sir_lsap_sel != LSAP_ANY)
            sprintf( win->irdaServiceName, "LSAP-SEL%u", uin->sir_lsap_sel );
        else
            memcpy( win->irdaServiceName, uin->sir_name,
                    sizeof(win->irdaServiceName) );
        return 0;
    }
#endif
    case AF_INET6: {
        const struct sockaddr_in6* uin6 = (const struct sockaddr_in6*)uaddr;
        struct WS_sockaddr_in6 *win6 = (struct WS_sockaddr_in6 *)wsaddr;

        if (*wsaddrlen < sizeof(struct WS_sockaddr_in6))
            return -1;
        win6->sin6_family   = WS_AF_INET6;
        win6->sin6_port     = uin6->sin6_port;
        win6->sin6_flowinfo = uin6->sin6_flowinfo;
        memcpy(&win6->sin6_addr, &uin6->sin6_addr, 16); /* 16 bytes = 128 address bits */
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
        win6->sin6_scope_id = uin6->sin6_scope_id;
#else
        win6->sin6_scope_id = 0;
#endif
        *wsaddrlen = sizeof(struct WS_sockaddr_in6);
        return 0;
    }
    case AF_INET: {
        const struct sockaddr_in* uin = (const struct sockaddr_in*)uaddr;
        struct WS_sockaddr_in* win = (struct WS_sockaddr_in*)wsaddr;

        if (*wsaddrlen < sizeof(struct WS_sockaddr_in))
            return -1;
        win->sin_family = WS_AF_INET;
        win->sin_port   = uin->sin_port;
        memcpy(&win->sin_addr,&uin->sin_addr,4); /* 4 bytes = 32 address bits */
        memset(win->sin_zero, 0, 8); /* Make sure the null padding is null */
        *wsaddrlen = sizeof(struct WS_sockaddr_in);
        return 0;
    }
    case AF_UNSPEC: {
        memset(wsaddr,0,*wsaddrlen);
        return 0;
    }
    default:
        FIXME("Unknown address family %d\n", uaddr->sa_family);
        return -1;
    }
    return res;
}

static INT WS_DuplicateSocket(BOOL unicode, SOCKET s, DWORD dwProcessId, LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    HANDLE hProcess;
    int size;
    WSAPROTOCOL_INFOW infow;

    TRACE("(unicode %d, socket %04lx, processid %x, buffer %p)\n",
          unicode, s, dwProcessId, lpProtocolInfo);

    if (!ws_protocol_info(s, unicode, &infow, &size))
        return SOCKET_ERROR;

    if (!(hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId)))
    {
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    if (!lpProtocolInfo)
    {
        CloseHandle(hProcess);
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    /* I don't know what the real Windoze does next, this is a hack */
    /* ...we could duplicate and then use ConvertToGlobalHandle on the duplicate, then let
     * the target use the global duplicate, or we could copy a reference to us to the structure
     * and let the target duplicate it from us, but let's do it as simple as possible */
    memcpy(lpProtocolInfo, &infow, size);
    DuplicateHandle(GetCurrentProcess(), SOCKET2HANDLE(s),
                    hProcess, (LPHANDLE)&lpProtocolInfo->dwServiceFlags3,
                    0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(hProcess);
    lpProtocolInfo->dwServiceFlags4 = 0xff00ff00; /* magic */
    return 0;
}

static BOOL ws_protocol_info(SOCKET s, int unicode, WSAPROTOCOL_INFOW *buffer, int *size)
{
    struct afd_get_info_params params;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    unsigned int i;

    *size = unicode ? sizeof(WSAPROTOCOL_INFOW) : sizeof(WSAPROTOCOL_INFOA);
    memset(buffer, 0, *size);

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io,
                                    IOCTL_AFD_WINE_GET_INFO, NULL, 0, &params, sizeof(params) );
    if (status)
    {
        SetLastError( NtStatusToWSAError( status ) );
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(supported_protocols); ++i)
    {
        const WSAPROTOCOL_INFOW *info = &supported_protocols[i];
        if (params.family == info->iAddressFamily &&
            params.type == info->iSocketType &&
            params.protocol >= info->iProtocol &&
            params.protocol <= info->iProtocol + info->iProtocolMaxOffset)
        {
            if (unicode)
                *buffer = *info;
            else
            {
                WSAPROTOCOL_INFOA *bufferA = (WSAPROTOCOL_INFOA *)buffer;
                memcpy( bufferA, info, offsetof( WSAPROTOCOL_INFOW, szProtocol ) );
                WideCharToMultiByte( CP_ACP, 0, info->szProtocol, -1,
                                     bufferA->szProtocol, sizeof(bufferA->szProtocol), NULL, NULL );
            }
            buffer->iProtocol = params.protocol;
            return TRUE;
        }
    }
    FIXME( "Could not fill protocol information for family %d, type %d, protocol %d.\n",
            params.family, params.type, params.protocol );
    return TRUE;
}



/***********************************************************************
 *		accept		(WS2_32.1)
 */
SOCKET WINAPI WS_accept( SOCKET s, struct WS_sockaddr *addr, int *len )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    obj_handle_t accept_handle;
    HANDLE sync_event;
    SOCKET ret;

    TRACE("%#lx\n", s);

    if (!(sync_event = get_sync_event())) return INVALID_SOCKET;
    status = NtDeviceIoControlFile( (HANDLE)s, sync_event, NULL, NULL, &io, IOCTL_AFD_WINE_ACCEPT,
                                    NULL, 0, &accept_handle, sizeof(accept_handle) );
    if (status == STATUS_PENDING)
    {
        if (WaitForSingleObject( sync_event, INFINITE ) == WAIT_FAILED)
            return SOCKET_ERROR;
        status = io.u.Status;
    }
    if (status)
    {
        WARN("failed; status %#x\n", status);
        WSASetLastError( NtStatusToWSAError( status ) );
        return INVALID_SOCKET;
    }

    ret = HANDLE2SOCKET(wine_server_ptr_handle( accept_handle ));
    if (!socket_list_add( ret ))
    {
        CloseHandle( SOCKET2HANDLE(ret) );
        return INVALID_SOCKET;
    }
    if (addr && len && WS_getpeername( ret, addr, len ))
    {
        WS_closesocket( ret );
        return INVALID_SOCKET;
    }

    TRACE("returning %#lx\n", ret);
    return ret;
}

/***********************************************************************
 *     AcceptEx
 */
static BOOL WINAPI WS2_AcceptEx( SOCKET listener, SOCKET acceptor, void *dest, DWORD recv_len,
                                 DWORD local_len, DWORD remote_len, DWORD *ret_len, OVERLAPPED *overlapped)
{
    struct afd_accept_into_params params =
    {
        .accept_handle = acceptor,
        .recv_len = recv_len,
        .local_len = local_len,
    };
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "listener %#lx, acceptor %#lx, dest %p, recv_len %u, local_len %u, remote_len %u, ret_len %p, "
           "overlapped %p\n", listener, acceptor, dest, recv_len, local_len, remote_len, ret_len, overlapped );

    if (!overlapped)
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return FALSE;
    }

    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    overlapped->Internal = STATUS_PENDING;
    overlapped->InternalHigh = 0;

    if (!dest)
    {
        SetLastError(WSAEINVAL);
        return FALSE;
    }

    if (!remote_len)
    {
        SetLastError(WSAEFAULT);
        return FALSE;
    }

    status = NtDeviceIoControlFile( SOCKET2HANDLE(listener), overlapped->hEvent, NULL, cvalue,
                                    (IO_STATUS_BLOCK *)overlapped, IOCTL_AFD_WINE_ACCEPT_INTO, &params, sizeof(params),
                                    dest, recv_len + local_len + remote_len );

    if (ret_len) *ret_len = overlapped->InternalHigh;
    WSASetLastError( NtStatusToWSAError(status) );
    return !status;
}


static BOOL WINAPI WS2_TransmitFile( SOCKET s, HANDLE file, DWORD file_len, DWORD buffer_size,
                                     OVERLAPPED *overlapped, TRANSMIT_FILE_BUFFERS *buffers, DWORD flags )
{
    struct afd_transmit_params params = {0};
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#lx, file %p, file_len %u, buffer_size %u, overlapped %p, buffers %p, flags %#x\n",
           s, file, file_len, buffer_size, overlapped, buffers, flags );

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
        overlapped->Internal = STATUS_PENDING;
        overlapped->InternalHigh = 0;
        params.offset.u.LowPart = overlapped->u.s.Offset;
        params.offset.u.HighPart = overlapped->u.s.OffsetHigh;
    }
    else
    {
        if (!(event = get_sync_event())) return -1;
        params.offset.QuadPart = FILE_USE_FILE_POINTER_POSITION;
    }

    params.file = file;
    params.file_len = file_len;
    params.buffer_size = buffer_size;
    if (buffers) params.buffers = *buffers;
    params.flags = flags;

    status = NtDeviceIoControlFile( (HANDLE)s, event, NULL, cvalue, piosb,
                                    IOCTL_AFD_WINE_TRANSMIT, &params, sizeof(params), NULL, 0 );
    if (status == STATUS_PENDING && !overlapped)
    {
        if (WaitForSingleObject( event, INFINITE ) == WAIT_FAILED)
            return FALSE;
        status = piosb->u.Status;
    }
    SetLastError( NtStatusToWSAError( status ) );
    return !status;
}


/***********************************************************************
 *     GetAcceptExSockaddrs
 */
static void WINAPI WS2_GetAcceptExSockaddrs(PVOID buffer, DWORD data_size, DWORD local_size, DWORD remote_size,
                                     struct WS_sockaddr **local_addr, LPINT local_addr_len,
                                     struct WS_sockaddr **remote_addr, LPINT remote_addr_len)
{
    char *cbuf = buffer;
    TRACE("(%p, %d, %d, %d, %p, %p, %p, %p)\n", buffer, data_size, local_size, remote_size, local_addr,
                                                local_addr_len, remote_addr, remote_addr_len );
    cbuf += data_size;

    *local_addr_len = *(int *) cbuf;
    *local_addr = (struct WS_sockaddr *)(cbuf + sizeof(int));

    cbuf += local_size;

    *remote_addr_len = *(int *) cbuf;
    *remote_addr = (struct WS_sockaddr *)(cbuf + sizeof(int));
}


static void WINAPI socket_apc( void *apc_user, IO_STATUS_BLOCK *io, ULONG reserved )
{
    LPWSAOVERLAPPED_COMPLETION_ROUTINE func = apc_user;
    func( NtStatusToWSAError( io->u.Status ), io->Information, (OVERLAPPED *)io, 0 );
}

static int WS2_recv_base( SOCKET s, WSABUF *buffers, DWORD buffer_count, DWORD *ret_size, DWORD *flags,
                          struct WS_sockaddr *addr, int *addr_len, OVERLAPPED *overlapped,
                          LPWSAOVERLAPPED_COMPLETION_ROUTINE completion, WSABUF *control )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    struct afd_recvmsg_params params;
    PIO_APC_ROUTINE apc = NULL;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#lx, buffers %p, buffer_count %u, flags %#x, addr %p, "
           "addr_len %d, overlapped %p, completion %p, control %p\n",
           s, buffers, buffer_count, *flags, addr, addr_len ? *addr_len : -1, overlapped, completion, control );

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
    }
    else
    {
        if (!(event = get_sync_event())) return -1;
    }
    piosb->u.Status = STATUS_PENDING;

    if (completion)
    {
        event = NULL;
        cvalue = completion;
        apc = socket_apc;
    }

    params.control = control;
    params.addr = addr;
    params.addr_len = addr_len;
    params.ws_flags = flags;
    params.force_async = !!overlapped;
    params.count = buffer_count;
    params.buffers = buffers;

    status = NtDeviceIoControlFile( (HANDLE)s, event, apc, cvalue, piosb,
                                    IOCTL_AFD_WINE_RECVMSG, &params, sizeof(params), NULL, 0 );
    if (status == STATUS_PENDING && !overlapped)
    {
        if (WaitForSingleObject( event, INFINITE ) == WAIT_FAILED)
            return -1;
        status = piosb->u.Status;
    }
    if (!status && ret_size) *ret_size = piosb->Information;
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}

static int WS2_sendto( SOCKET s, WSABUF *buffers, DWORD buffer_count, DWORD *ret_size, DWORD flags,
                       const struct WS_sockaddr *addr, int addr_len, OVERLAPPED *overlapped,
                       LPWSAOVERLAPPED_COMPLETION_ROUTINE completion )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    struct afd_sendmsg_params params;
    PIO_APC_ROUTINE apc = NULL;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#lx, buffers %p, buffer_count %u, flags %#x, addr %p, "
           "addr_len %d, overlapped %p, completion %p\n",
           s, buffers, buffer_count, flags, addr, addr_len, overlapped, completion );

    if (!socket_list_find( s ))
    {
        SetLastError( WSAENOTSOCK );
        return -1;
    }

    if (!overlapped && !ret_size)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
    }
    else
    {
        if (!(event = get_sync_event())) return -1;
    }
    piosb->u.Status = STATUS_PENDING;

    if (completion)
    {
        event = NULL;
        cvalue = completion;
        apc = socket_apc;
    }

    params.addr = addr;
    params.addr_len = addr_len;
    params.ws_flags = flags;
    params.force_async = !!overlapped;
    params.count = buffer_count;
    params.buffers = buffers;

    status = NtDeviceIoControlFile( (HANDLE)s, event, apc, cvalue, piosb,
                                    IOCTL_AFD_WINE_SENDMSG, &params, sizeof(params), NULL, 0 );
    if (status == STATUS_PENDING && !overlapped)
    {
        if (WaitForSingleObject( event, INFINITE ) == WAIT_FAILED)
            return -1;
        status = piosb->u.Status;
    }
    if (!status && ret_size) *ret_size = piosb->Information;
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *     WSASendMsg
 */
int WINAPI WSASendMsg( SOCKET s, LPWSAMSG msg, DWORD dwFlags, LPDWORD lpNumberOfBytesSent,
                       LPWSAOVERLAPPED lpOverlapped,
                       LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    if (!msg)
    {
        SetLastError( WSAEFAULT );
        return SOCKET_ERROR;
    }

    return WS2_sendto( s, msg->lpBuffers, msg->dwBufferCount, lpNumberOfBytesSent,
                       dwFlags, msg->name, msg->namelen,
                       lpOverlapped, lpCompletionRoutine );
}

/***********************************************************************
 *     WSARecvMsg
 *
 * Perform a receive operation that is capable of returning message
 * control headers.  It is important to note that the WSAMSG parameter
 * must remain valid throughout the operation, even when an overlapped
 * receive is performed.
 */
static int WINAPI WS2_WSARecvMsg( SOCKET s, LPWSAMSG msg, LPDWORD lpNumberOfBytesRecvd,
                                  LPWSAOVERLAPPED lpOverlapped,
                                  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    if (!msg)
    {
        SetLastError( WSAEFAULT );
        return SOCKET_ERROR;
    }

    return WS2_recv_base( s, msg->lpBuffers, msg->dwBufferCount, lpNumberOfBytesRecvd,
                          &msg->dwFlags, msg->name, &msg->namelen,
                          lpOverlapped, lpCompletionRoutine, &msg->Control );
}


/***********************************************************************
 *      bind   (ws2_32.2)
 */
int WINAPI WS_bind( SOCKET s, const struct WS_sockaddr *addr, int len )
{
    struct afd_bind_params *params;
    struct WS_sockaddr *ret_addr;
    IO_STATUS_BLOCK io;
    HANDLE sync_event;
    NTSTATUS status;

    TRACE( "socket %#lx, addr %s\n", s, debugstr_sockaddr(addr) );

    if (!addr)
    {
        SetLastError( WSAEAFNOSUPPORT );
        return -1;
    }

    switch (addr->sa_family)
    {
        case WS_AF_INET:
            if (len < sizeof(struct WS_sockaddr_in))
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            break;

        case WS_AF_INET6:
            if (len < sizeof(struct WS_sockaddr_in6))
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            break;

        case WS_AF_IPX:
            if (len < sizeof(struct WS_sockaddr_ipx))
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            break;

        case WS_AF_IRDA:
            if (len < sizeof(SOCKADDR_IRDA))
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            break;

        default:
            FIXME( "unknown protocol %u\n", addr->sa_family );
            SetLastError( WSAEAFNOSUPPORT );
            return -1;
    }

    if (!(sync_event = get_sync_event())) return -1;

    params = HeapAlloc( GetProcessHeap(), 0, sizeof(int) + len );
    ret_addr = HeapAlloc( GetProcessHeap(), 0, len );
    if (!params || !ret_addr)
    {
        HeapFree( GetProcessHeap(), 0, params );
        HeapFree( GetProcessHeap(), 0, ret_addr );
        SetLastError( WSAENOBUFS );
        return -1;
    }
    params->unknown = 0;
    memcpy( &params->addr, addr, len );

    status = NtDeviceIoControlFile( (HANDLE)s, sync_event, NULL, NULL, &io, IOCTL_AFD_BIND,
                                    params, sizeof(int) + len, ret_addr, len );
    if (status == STATUS_PENDING)
    {
        if (WaitForSingleObject( sync_event, INFINITE ) == WAIT_FAILED)
            return -1;
        status = io.u.Status;
    }

    HeapFree( GetProcessHeap(), 0, params );
    HeapFree( GetProcessHeap(), 0, ret_addr );

    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *		closesocket		(WS2_32.3)
 */
int WINAPI WS_closesocket(SOCKET s)
{
    int res = SOCKET_ERROR, fd;
    if (num_startup)
    {
        fd = get_sock_fd(s, FILE_READ_DATA, NULL);
        if (fd >= 0)
        {
            release_sock_fd(s, fd);
            socket_list_remove(s);
            if (CloseHandle(SOCKET2HANDLE(s)))
                res = 0;
        }
    }
    else
        SetLastError(WSANOTINITIALISED);
    TRACE("(socket %04lx) -> %d\n", s, res);
    return res;
}


/***********************************************************************
 *      connect   (ws2_32.4)
 */
int WINAPI WS_connect( SOCKET s, const struct WS_sockaddr *addr, int len )
{
    struct afd_connect_params *params;
    IO_STATUS_BLOCK io;
    HANDLE sync_event;
    NTSTATUS status;

    TRACE( "socket %#lx, addr %s, len %d\n", s, debugstr_sockaddr(addr), len );

    if (!(sync_event = get_sync_event())) return -1;

    if (!(params = HeapAlloc( GetProcessHeap(), 0, sizeof(*params) + len )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return -1;
    }
    params->addr_len = len;
    params->synchronous = TRUE;
    memcpy( params + 1, addr, len );

    status = NtDeviceIoControlFile( (HANDLE)s, sync_event, NULL, NULL, &io, IOCTL_AFD_WINE_CONNECT,
                                    params, sizeof(*params) + len, NULL, 0 );
    HeapFree( GetProcessHeap(), 0, params );
    if (status == STATUS_PENDING)
    {
        if (WaitForSingleObject( sync_event, INFINITE ) == WAIT_FAILED) return -1;
        status = io.u.Status;
    }
    if (status)
    {
        /* NtStatusToWSAError() has no mapping for WSAEALREADY */
        SetLastError( status == STATUS_ADDRESS_ALREADY_ASSOCIATED ? WSAEALREADY : NtStatusToWSAError( status ) );
        return -1;
    }
    return 0;
}


/***********************************************************************
 *              WSAConnect             (WS2_32.30)
 */
int WINAPI WSAConnect( SOCKET s, const struct WS_sockaddr* name, int namelen,
                       LPWSABUF lpCallerData, LPWSABUF lpCalleeData,
                       LPQOS lpSQOS, LPQOS lpGQOS )
{
    if ( lpCallerData || lpCalleeData || lpSQOS || lpGQOS )
        FIXME("unsupported parameters!\n");
    return WS_connect( s, name, namelen );
}


static BOOL WINAPI WS2_ConnectEx( SOCKET s, const struct WS_sockaddr *name, int namelen,
                                  void *send_buffer, DWORD send_len, DWORD *ret_len, OVERLAPPED *overlapped )
{
    struct afd_connect_params *params;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#lx, ptr %p %s, length %d, send_buffer %p, send_len %u, overlapped %p\n",
           s, name, debugstr_sockaddr(name), namelen, send_buffer, send_len, overlapped );

    if (!overlapped)
    {
        SetLastError( WSA_INVALID_PARAMETER );
        return FALSE;
    }

    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    overlapped->Internal = STATUS_PENDING;
    overlapped->InternalHigh = 0;

    if (!(params = HeapAlloc( GetProcessHeap(), 0, sizeof(*params) + namelen + send_len )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return SOCKET_ERROR;
    }
    params->addr_len = namelen;
    params->synchronous = FALSE;
    memcpy( params + 1, name, namelen );
    memcpy( (char *)(params + 1) + namelen, send_buffer, send_len );

    status = NtDeviceIoControlFile( SOCKET2HANDLE(s), overlapped->hEvent, NULL, cvalue,
                                    (IO_STATUS_BLOCK *)overlapped, IOCTL_AFD_WINE_CONNECT,
                                    params, sizeof(*params) + namelen + send_len, NULL, 0 );
    HeapFree( GetProcessHeap(), 0, params );
    if (ret_len) *ret_len = overlapped->InternalHigh;
    SetLastError( NtStatusToWSAError( status ) );
    return !status;
}


static BOOL WINAPI WS2_DisconnectEx( SOCKET s, OVERLAPPED *overlapped, DWORD flags, DWORD reserved )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    void *cvalue = NULL;
    int how = SD_SEND;
    HANDLE event = 0;
    NTSTATUS status;

    TRACE( "socket %#lx, overlapped %p, flags %#x, reserved %#x\n", s, overlapped, flags, reserved );

    if (flags & TF_REUSE_SOCKET)
        FIXME( "Reusing socket not supported yet\n" );

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
        overlapped->Internal = STATUS_PENDING;
        overlapped->InternalHigh = 0;
    }

    status = NtDeviceIoControlFile( (HANDLE)s, event, NULL, cvalue, piosb,
                                    IOCTL_AFD_WINE_SHUTDOWN, &how, sizeof(how), NULL, 0 );
    if (!status && overlapped) status = STATUS_PENDING;
    SetLastError( NtStatusToWSAError( status ) );
    return !status;
}


/***********************************************************************
 *      getpeername   (ws2_32.5)
 */
int WINAPI WS_getpeername( SOCKET s, struct WS_sockaddr *addr, int *len )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#lx, addr %p, len %d\n", s, addr, len ? *len : 0 );

    if (!socket_list_find( s ))
    {
        WSASetLastError( WSAENOTSOCK );
        return -1;
    }

    if (!len)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io,
                                    IOCTL_AFD_WINE_GETPEERNAME, NULL, 0, addr, *len );
    if (!status)
        *len = io.Information;
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *      getsockname   (ws2_32.6)
 */
int WINAPI WS_getsockname( SOCKET s, struct WS_sockaddr *addr, int *len )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#lx, addr %p, len %d\n", s, addr, len ? *len : 0 );

    if (!addr)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_GETSOCKNAME, NULL, 0, addr, *len );
    if (!status)
        *len = io.Information;
    WSASetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


static int server_getsockopt( SOCKET s, ULONG code, char *optval, int *optlen )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, code, NULL, 0, optval, *optlen );
    if (!status) *optlen = io.Information;
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *		getsockopt		(WS2_32.7)
 */
INT WINAPI WS_getsockopt(SOCKET s, INT level,
                                  INT optname, char *optval, INT *optlen)
{
    int fd;
    INT ret = 0;

    TRACE("(socket %04lx, %s, optval %s, optlen %p (%d))\n", s,
          debugstr_sockopt(level, optname), debugstr_optval(optval, 0),
          optlen, optlen ? *optlen : 0);

    switch(level)
    {
    case WS_SOL_SOCKET:
    {
        switch(optname)
        {
        case WS_SO_ACCEPTCONN:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_ACCEPTCONN, optval, optlen );

        case WS_SO_BROADCAST:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_BROADCAST, optval, optlen );

        case WS_SO_BSP_STATE:
        {
            CSADDR_INFO *csinfo = (CSADDR_INFO *)optval;
            WSAPROTOCOL_INFOW infow;
            int addr_size;

            if (!ws_protocol_info( s, TRUE, &infow, &addr_size ))
                return -1;

            if (infow.iAddressFamily == WS_AF_INET)
                addr_size = sizeof(struct sockaddr_in);
            else if (infow.iAddressFamily == WS_AF_INET6)
                addr_size = sizeof(struct sockaddr_in6);
            else
            {
                FIXME( "family %d is unsupported for SO_BSP_STATE\n", infow.iAddressFamily );
                SetLastError( WSAEAFNOSUPPORT );
                return -1;
            }

            if (*optlen < sizeof(CSADDR_INFO) + addr_size * 2)
            {
                ret = 0;
                SetLastError( WSAEFAULT );
                return -1;
            }

            csinfo->LocalAddr.lpSockaddr = (struct WS_sockaddr *)(csinfo + 1);
            csinfo->RemoteAddr.lpSockaddr = (struct WS_sockaddr *)((char *)(csinfo + 1) + addr_size);

            csinfo->LocalAddr.iSockaddrLength = addr_size;
            if (WS_getsockname( s, csinfo->LocalAddr.lpSockaddr, &csinfo->LocalAddr.iSockaddrLength ) < 0)
            {
                csinfo->LocalAddr.lpSockaddr = NULL;
                csinfo->LocalAddr.iSockaddrLength = 0;
            }

            csinfo->RemoteAddr.iSockaddrLength = addr_size;
            if (WS_getpeername( s, csinfo->RemoteAddr.lpSockaddr, &csinfo->RemoteAddr.iSockaddrLength ) < 0)
            {
                csinfo->RemoteAddr.lpSockaddr = NULL;
                csinfo->RemoteAddr.iSockaddrLength = 0;
            }

            csinfo->iSocketType = infow.iSocketType;
            csinfo->iProtocol = infow.iProtocol;
            return 0;
        }

        case WS_SO_DEBUG:
            WARN( "returning 0 for SO_DEBUG\n" );
            *(DWORD *)optval = 0;
            SetLastError( 0 );
            return 0;

        case WS_SO_DONTLINGER:
        {
            struct WS_linger linger;
            int len = sizeof(linger);
            int ret;

            if (!optlen || *optlen < sizeof(BOOL)|| !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }

            if (!(ret = WS_getsockopt( s, WS_SOL_SOCKET, WS_SO_LINGER, (char *)&linger, &len )))
            {
                *(BOOL *)optval = !linger.l_onoff;
                *optlen = sizeof(BOOL);
            }
            return ret;
        }

        case WS_SO_CONNECT_TIME:
        {
            static int pretendtime = 0;
            struct WS_sockaddr addr;
            int len = sizeof(addr);

            if (!optlen || *optlen < sizeof(DWORD) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            if (WS_getpeername(s, &addr, &len) == SOCKET_ERROR)
                *(DWORD *)optval = ~0u;
            else
            {
                if (!pretendtime) FIXME("WS_SO_CONNECT_TIME - faking results\n");
                *(DWORD *)optval = pretendtime++;
            }
            *optlen = sizeof(DWORD);
            return ret;
        }
        /* As mentioned in setsockopt, Windows ignores this, so we
         * always return true here */
        case WS_SO_DONTROUTE:
            if (!optlen || *optlen < sizeof(BOOL) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            *(BOOL *)optval = TRUE;
            *optlen = sizeof(BOOL);
            return 0;

        case WS_SO_ERROR:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_ERROR, optval, optlen );

        case WS_SO_KEEPALIVE:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_KEEPALIVE, optval, optlen );

        case WS_SO_LINGER:
        {
            WSAPROTOCOL_INFOW info;
            int size;

            /* struct linger and LINGER have different sizes */
            if (!optlen || *optlen < sizeof(LINGER) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }

            if (!ws_protocol_info( s, TRUE, &info, &size ))
                return -1;

            if (info.iSocketType == SOCK_DGRAM)
            {
                SetLastError( WSAENOPROTOOPT );
                return -1;
            }

            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_LINGER, optval, optlen );
        }

        case WS_SO_MAX_MSG_SIZE:
            if (!optlen || *optlen < sizeof(int) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            TRACE("getting global SO_MAX_MSG_SIZE = 65507\n");
            *(int *)optval = 65507;
            *optlen = sizeof(int);
            return 0;

        case WS_SO_OOBINLINE:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_OOBINLINE, optval, optlen );

        /* SO_OPENTYPE does not require a valid socket handle. */
        case WS_SO_OPENTYPE:
            if (!optlen || *optlen < sizeof(int) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            *(int *)optval = get_per_thread_data()->opentype;
            *optlen = sizeof(int);
            TRACE("getting global SO_OPENTYPE = 0x%x\n", *((int*)optval) );
            return 0;
        case WS_SO_PROTOCOL_INFOA:
        case WS_SO_PROTOCOL_INFOW:
        {
            int size;
            WSAPROTOCOL_INFOW infow;

            ret = ws_protocol_info(s, optname == WS_SO_PROTOCOL_INFOW, &infow, &size);
            if (ret)
            {
                if (!optlen || !optval || *optlen < size)
                {
                    if(optlen) *optlen = size;
                    ret = 0;
                    SetLastError(WSAEFAULT);
                }
                else
                    memcpy(optval, &infow, size);
            }
            return ret ? 0 : SOCKET_ERROR;
        }

        case WS_SO_RCVBUF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_RCVBUF, optval, optlen );

        case WS_SO_RCVTIMEO:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_RCVTIMEO, optval, optlen );

        case WS_SO_REUSEADDR:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_REUSEADDR, optval, optlen );

        case WS_SO_SNDBUF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_SNDBUF, optval, optlen );

        case WS_SO_SNDTIMEO:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_SNDTIMEO, optval, optlen );

        case WS_SO_TYPE:
        {
            WSAPROTOCOL_INFOW info;
            int size;

            if (!optlen || *optlen < sizeof(int) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }

            if (!ws_protocol_info( s, TRUE, &info, &size ))
                return -1;

            *(int *)optval = info.iSocketType;
            return 0;
        }

        default:
            TRACE("Unknown SOL_SOCKET optname: 0x%08x\n", optname);
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        } /* end switch(optname) */
    }/* end case WS_SOL_SOCKET */
#ifdef HAS_IPX
    case WS_NSPROTO_IPX:
    {
        struct WS_sockaddr_ipx addr;
        IPX_ADDRESS_DATA *data;
        int namelen;
        switch(optname)
        {
        case WS_IPX_PTYPE:
            if ((fd = get_sock_fd( s, 0, NULL )) == -1) return SOCKET_ERROR;
#ifdef SOL_IPX
            if(getsockopt(fd, SOL_IPX, IPX_TYPE, optval, (socklen_t *)optlen) == -1)
            {
                ret = SOCKET_ERROR;
            }
#else
            {
                struct ipx val;
                socklen_t len=sizeof(struct ipx);
                if(getsockopt(fd, 0, SO_DEFAULT_HEADERS, &val, &len) == -1 )
                    ret = SOCKET_ERROR;
                else
                    *optval = (int)val.ipx_pt;
            }
#endif
            TRACE("ptype: %d (fd: %d)\n", *(int*)optval, fd);
            release_sock_fd( s, fd );
            return ret;

        case WS_IPX_ADDRESS:
            /*
            *  On a Win2000 system with one network card there are usually
            *  three ipx devices one with a speed of 28.8kbps, 10Mbps and 100Mbps.
            *  Using this call you can then retrieve info about this all.
            *  In case of Linux it is a bit different. Usually you have
            *  only "one" device active and further it is not possible to
            *  query things like the linkspeed.
            */
            FIXME("IPX_ADDRESS\n");
            namelen = sizeof(struct WS_sockaddr_ipx);
            memset(&addr, 0, sizeof(struct WS_sockaddr_ipx));
            WS_getsockname(s, (struct WS_sockaddr*)&addr, &namelen);

            data = (IPX_ADDRESS_DATA*)optval;
                    memcpy(data->nodenum,addr.sa_nodenum,sizeof(data->nodenum));
                    memcpy(data->netnum,addr.sa_netnum,sizeof(data->netnum));
            data->adapternum = 0;
            data->wan = FALSE; /* We are not on a wan for now .. */
            data->status = FALSE; /* Since we are not on a wan, the wan link isn't up */
            data->maxpkt = 1467; /* This value is the default one, at least on Win2k/WinXP */
            data->linkspeed = 100000; /* Set the line speed in 100bit/s to 10 Mbit;
                                       * note 1MB = 1000kB in this case */
            return 0;

        case WS_IPX_MAX_ADAPTER_NUM:
            FIXME("IPX_MAX_ADAPTER_NUM\n");
            *(int*)optval = 1; /* As noted under IPX_ADDRESS we have just one card. */
            return 0;

        default:
            FIXME("IPX optname:%x\n", optname);
            return SOCKET_ERROR;
        }/* end switch(optname) */
    } /* end case WS_NSPROTO_IPX */
#endif

#ifdef HAS_IRDA
#define MAX_IRDA_DEVICES 10

    case WS_SOL_IRLMP:
        switch(optname)
        {
        case WS_IRLMP_ENUMDEVICES:
        {
            char buf[sizeof(struct irda_device_list) +
                     (MAX_IRDA_DEVICES - 1) * sizeof(struct irda_device_info)];
            int res;
            socklen_t len = sizeof(buf);

            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;
            res = getsockopt( fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &len );
            release_sock_fd( s, fd );
            if (res < 0)
            {
                SetLastError(wsaErrno());
                return SOCKET_ERROR;
            }
            else
            {
                struct irda_device_list *src = (struct irda_device_list *)buf;
                DEVICELIST *dst = (DEVICELIST *)optval;
                INT needed = sizeof(DEVICELIST);
                unsigned int i;

                if (src->len > 0)
                    needed += (src->len - 1) * sizeof(IRDA_DEVICE_INFO);
                if (*optlen < needed)
                {
                    SetLastError(WSAEFAULT);
                    return SOCKET_ERROR;
                }
                *optlen = needed;
                TRACE("IRLMP_ENUMDEVICES: %d devices found:\n", src->len);
                dst->numDevice = src->len;
                for (i = 0; i < src->len; i++)
                {
                    TRACE("saddr = %08x, daddr = %08x, info = %s, hints = %02x%02x\n",
                          src->dev[i].saddr, src->dev[i].daddr,
                          src->dev[i].info, src->dev[i].hints[0],
                          src->dev[i].hints[1]);
                    memcpy( dst->Device[i].irdaDeviceID,
                            &src->dev[i].daddr,
                            sizeof(dst->Device[i].irdaDeviceID) ) ;
                    memcpy( dst->Device[i].irdaDeviceName,
                            src->dev[i].info,
                            sizeof(dst->Device[i].irdaDeviceName) ) ;
                    memcpy( &dst->Device[i].irdaDeviceHints1,
                            &src->dev[i].hints[0],
                            sizeof(dst->Device[i].irdaDeviceHints1) ) ;
                    memcpy( &dst->Device[i].irdaDeviceHints2,
                            &src->dev[i].hints[1],
                            sizeof(dst->Device[i].irdaDeviceHints2) ) ;
                    dst->Device[i].irdaCharSet = src->dev[i].charset;
                }
                return 0;
            }
        }
        default:
            FIXME("IrDA optname:0x%x\n", optname);
            return SOCKET_ERROR;
        }
        break; /* case WS_SOL_IRLMP */
#undef MAX_IRDA_DEVICES
#endif

    /* Levels WS_IPPROTO_TCP and WS_IPPROTO_IP convert directly */
    case WS_IPPROTO_TCP:
        switch(optname)
        {
        case WS_TCP_NODELAY:
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;
            convert_sockopt(&level, &optname);
            if (getsockopt(fd, level, optname, optval, (socklen_t *)optlen) != 0 )
            {
                SetLastError(wsaErrno());
                ret = SOCKET_ERROR;
            }
            release_sock_fd( s, fd );
            return ret;
        }
        FIXME("Unknown IPPROTO_TCP optname 0x%08x\n", optname);
        return SOCKET_ERROR;

    case WS_IPPROTO_IP:
        switch(optname)
        {
        case WS_IP_DONTFRAGMENT:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_DONTFRAGMENT, optval, optlen );

        case WS_IP_HDRINCL:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_HDRINCL, optval, optlen );

        case WS_IP_MULTICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_MULTICAST_IF, optval, optlen );

        case WS_IP_MULTICAST_LOOP:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_MULTICAST_LOOP, optval, optlen );

        case WS_IP_MULTICAST_TTL:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_MULTICAST_TTL, optval, optlen );

        case WS_IP_OPTIONS:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_OPTIONS, optval, optlen );

        case WS_IP_PKTINFO:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_PKTINFO, optval, optlen );

        case WS_IP_TOS:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_TOS, optval, optlen );

        case WS_IP_TTL:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_TTL, optval, optlen );

        case WS_IP_UNICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_UNICAST_IF, optval, optlen );

        default:
            FIXME( "unrecognized IP option %u\n", optname );
            /* fall through */

        case WS_IP_ADD_MEMBERSHIP:
        case WS_IP_DROP_MEMBERSHIP:
            SetLastError( WSAENOPROTOOPT );
            return -1;
        }

    case WS_IPPROTO_IPV6:
        switch(optname)
        {
        case WS_IPV6_DONTFRAG:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_DONTFRAG, optval, optlen );

        case WS_IPV6_MULTICAST_HOPS:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_MULTICAST_HOPS, optval, optlen );

        case WS_IPV6_MULTICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_MULTICAST_IF, optval, optlen );

        case WS_IPV6_MULTICAST_LOOP:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_MULTICAST_LOOP, optval, optlen );

        case WS_IPV6_UNICAST_HOPS:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_UNICAST_HOPS, optval, optlen );

        case WS_IPV6_UNICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_UNICAST_IF, optval, optlen );

        case WS_IPV6_V6ONLY:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_V6ONLY, optval, optlen );

        default:
            FIXME( "unrecognized IPv6 option %u\n", optname );
            /* fall through */

        case WS_IPV6_ADD_MEMBERSHIP:
        case WS_IPV6_DROP_MEMBERSHIP:
            SetLastError( WSAENOPROTOOPT );
            return -1;
        }

    default:
        WARN("Unknown level: 0x%08x\n", level);
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    } /* end switch(level) */
}


static const char *debugstr_wsaioctl(DWORD code)
{
    const char *name = NULL, *buf_type, *family;

#define IOCTL_NAME(x) case x: name = #x; break
    switch (code)
    {
        IOCTL_NAME(WS_FIONBIO);
        IOCTL_NAME(WS_FIONREAD);
        IOCTL_NAME(WS_SIOCATMARK);
        /* IOCTL_NAME(WS_SIO_ACQUIRE_PORT_RESERVATION); */
        IOCTL_NAME(WS_SIO_ADDRESS_LIST_CHANGE);
        IOCTL_NAME(WS_SIO_ADDRESS_LIST_QUERY);
        IOCTL_NAME(WS_SIO_ASSOCIATE_HANDLE);
        /* IOCTL_NAME(WS_SIO_ASSOCIATE_PORT_RESERVATION);
        IOCTL_NAME(WS_SIO_BASE_HANDLE);
        IOCTL_NAME(WS_SIO_BSP_HANDLE);
        IOCTL_NAME(WS_SIO_BSP_HANDLE_SELECT);
        IOCTL_NAME(WS_SIO_BSP_HANDLE_POLL);
        IOCTL_NAME(WS_SIO_CHK_QOS); */
        IOCTL_NAME(WS_SIO_ENABLE_CIRCULAR_QUEUEING);
        IOCTL_NAME(WS_SIO_FIND_ROUTE);
        IOCTL_NAME(WS_SIO_FLUSH);
        IOCTL_NAME(WS_SIO_GET_BROADCAST_ADDRESS);
        IOCTL_NAME(WS_SIO_GET_EXTENSION_FUNCTION_POINTER);
        IOCTL_NAME(WS_SIO_GET_GROUP_QOS);
        IOCTL_NAME(WS_SIO_GET_INTERFACE_LIST);
        /* IOCTL_NAME(WS_SIO_GET_INTERFACE_LIST_EX); */
        IOCTL_NAME(WS_SIO_GET_QOS);
        IOCTL_NAME(WS_SIO_IDEAL_SEND_BACKLOG_CHANGE);
        IOCTL_NAME(WS_SIO_IDEAL_SEND_BACKLOG_QUERY);
        IOCTL_NAME(WS_SIO_KEEPALIVE_VALS);
        IOCTL_NAME(WS_SIO_MULTIPOINT_LOOPBACK);
        IOCTL_NAME(WS_SIO_MULTICAST_SCOPE);
        /* IOCTL_NAME(WS_SIO_QUERY_RSS_SCALABILITY_INFO);
        IOCTL_NAME(WS_SIO_QUERY_WFP_ALE_ENDPOINT_HANDLE); */
        IOCTL_NAME(WS_SIO_RCVALL);
        IOCTL_NAME(WS_SIO_RCVALL_IGMPMCAST);
        IOCTL_NAME(WS_SIO_RCVALL_MCAST);
        /* IOCTL_NAME(WS_SIO_RELEASE_PORT_RESERVATION); */
        IOCTL_NAME(WS_SIO_ROUTING_INTERFACE_CHANGE);
        IOCTL_NAME(WS_SIO_ROUTING_INTERFACE_QUERY);
        IOCTL_NAME(WS_SIO_SET_COMPATIBILITY_MODE);
        IOCTL_NAME(WS_SIO_SET_GROUP_QOS);
        IOCTL_NAME(WS_SIO_SET_QOS);
        IOCTL_NAME(WS_SIO_TRANSLATE_HANDLE);
        IOCTL_NAME(WS_SIO_UDP_CONNRESET);
    }
#undef IOCTL_NAME

    if (name)
        return name + 3;

    /* If this is not a known code split its bits */
    switch(code & 0x18000000)
    {
    case WS_IOC_WS2:
        family = "IOC_WS2";
        break;
    case WS_IOC_PROTOCOL:
        family = "IOC_PROTOCOL";
        break;
    case WS_IOC_VENDOR:
        family = "IOC_VENDOR";
        break;
    default: /* WS_IOC_UNIX */
    {
        BYTE size = (code >> 16) & WS_IOCPARM_MASK;
        char x = (code & 0xff00) >> 8;
        BYTE y = code & 0xff;
        char args[14];

        switch (code & (WS_IOC_VOID|WS_IOC_INOUT))
        {
            case WS_IOC_VOID:
                buf_type = "_IO";
                sprintf(args, "%d, %d", x, y);
                break;
            case WS_IOC_IN:
                buf_type = "_IOW";
                sprintf(args, "'%c', %d, %d", x, y, size);
                break;
            case WS_IOC_OUT:
                buf_type = "_IOR";
                sprintf(args, "'%c', %d, %d", x, y, size);
                break;
            default:
                buf_type = "?";
                sprintf(args, "'%c', %d, %d", x, y, size);
                break;
        }
        return wine_dbg_sprintf("%s(%s)", buf_type, args);
    }
    }

    /* We are different from WS_IOC_UNIX. */
    switch (code & (WS_IOC_VOID|WS_IOC_INOUT))
    {
        case WS_IOC_VOID:
            buf_type = "_WSAIO";
            break;
        case WS_IOC_INOUT:
            buf_type = "_WSAIORW";
            break;
        case WS_IOC_IN:
            buf_type = "_WSAIOW";
            break;
        case WS_IOC_OUT:
            buf_type = "_WSAIOR";
            break;
        default:
            buf_type = "?";
            break;
    }

    return wine_dbg_sprintf("%s(%s, %d)", buf_type, family,
                            (USHORT)(code & 0xffff));
}

/* do an ioctl call through the server */
static DWORD server_ioctl_sock( SOCKET s, DWORD code, LPVOID in_buff, DWORD in_size,
                                LPVOID out_buff, DWORD out_size, LPDWORD ret_size,
                                LPWSAOVERLAPPED overlapped,
                                LPWSAOVERLAPPED_COMPLETION_ROUTINE completion )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    HANDLE handle = SOCKET2HANDLE( s );
    PIO_APC_ROUTINE apc = NULL;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
    }
    else
    {
        if (!(event = get_sync_event())) return GetLastError();
    }

    if (completion)
    {
        event = NULL;
        cvalue = completion;
        apc = socket_apc;
    }

    status = NtDeviceIoControlFile( handle, event, apc, cvalue, piosb, code,
                                    in_buff, in_size, out_buff, out_size );
    if (status == STATUS_PENDING && !overlapped)
    {
        if (WaitForSingleObject( event, INFINITE ) == WAIT_FAILED)
            return -1;
        status = piosb->u.Status;
    }
    if (status == STATUS_NOT_SUPPORTED)
    {
        FIXME("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
    }
    else if (status == STATUS_SUCCESS)
        *ret_size = piosb->Information;

    return NtStatusToWSAError( status );
}


/**********************************************************************
 *              WSAIoctl                (WS2_32.50)
 *
 */
INT WINAPI WSAIoctl(SOCKET s, DWORD code, LPVOID in_buff, DWORD in_size, LPVOID out_buff,
                    DWORD out_size, LPDWORD ret_size, LPWSAOVERLAPPED overlapped,
                    LPWSAOVERLAPPED_COMPLETION_ROUTINE completion )
{
    TRACE("%04lx, %s, %p, %d, %p, %d, %p, %p, %p\n",
          s, debugstr_wsaioctl(code), in_buff, in_size, out_buff, out_size, ret_size, overlapped, completion);

    if (!ret_size)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    switch (code)
    {
    case WS_FIONBIO:
    {
        DWORD ret;

        if (IS_INTRESOURCE( in_buff ))
        {
            SetLastError( WSAEFAULT );
            return -1;
        }

        /* Explicitly ignore the output buffer; WeChat tries to pass an address
         * without write access. */
        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_FIONBIO, in_buff, in_size,
                                 NULL, 0, ret_size, overlapped, completion );
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case WS_FIONREAD:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_FIONREAD, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        if (!ret) *ret_size = sizeof(WS_u_long);
        return ret ? -1 : 0;
    }

    case WS_SIOCATMARK:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_SIOCATMARK, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        if (!ret) *ret_size = sizeof(WS_u_long);
        return ret ? -1 : 0;
    }

    case WS_SIO_GET_INTERFACE_LIST:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_GET_INTERFACE_LIST, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        if (ret && ret != ERROR_IO_PENDING) *ret_size = 0;
        return ret ? -1 : 0;
    }

    case WS_SIO_ADDRESS_LIST_QUERY:
    {
        DWORD size, total;

        TRACE("-> SIO_ADDRESS_LIST_QUERY request\n");

        if (out_size && out_size < FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[0]))
        {
            *ret_size = 0;
            SetLastError(WSAEINVAL);
            return SOCKET_ERROR;
        }

        if (GetAdaptersInfo(NULL, &size) == ERROR_BUFFER_OVERFLOW)
        {
            IP_ADAPTER_INFO *p, *table = HeapAlloc(GetProcessHeap(), 0, size);
            NTSTATUS status = STATUS_SUCCESS;
            SOCKET_ADDRESS_LIST *sa_list;
            SOCKADDR_IN *sockaddr;
            SOCKET_ADDRESS *sa;
            unsigned int i;
            DWORD ret = 0;
            DWORD num;

            if (!table || GetAdaptersInfo(table, &size))
            {
                HeapFree(GetProcessHeap(), 0, table);
                SetLastError( WSAEINVAL );
                return -1;
            }

            for (p = table, num = 0; p; p = p->Next)
                if (p->IpAddressList.IpAddress.String[0]) num++;

            total = FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[num]) + num * sizeof(*sockaddr);
            if (total > out_size || !out_buff)
            {
                *ret_size = total;
                HeapFree(GetProcessHeap(), 0, table);
                SetLastError( WSAEFAULT );
                return -1;
            }

            sa_list = out_buff;
            sa = sa_list->Address;
            sockaddr = (SOCKADDR_IN *)&sa[num];
            sa_list->iAddressCount = num;

            for (p = table, i = 0; p; p = p->Next)
            {
                if (!p->IpAddressList.IpAddress.String[0]) continue;

                sa[i].lpSockaddr = (SOCKADDR *)&sockaddr[i];
                sa[i].iSockaddrLength = sizeof(SOCKADDR);

                sockaddr[i].sin_family = WS_AF_INET;
                sockaddr[i].sin_port = 0;
                sockaddr[i].sin_addr.WS_s_addr = inet_addr(p->IpAddressList.IpAddress.String);
                i++;
            }

            HeapFree(GetProcessHeap(), 0, table);

            ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                     NULL, 0, ret_size, overlapped, completion );
            *ret_size = total;
            SetLastError( ret );
            return ret ? -1 : 0;
        }
        else
        {
            WARN("unable to get IP address list\n");
            SetLastError( WSAEINVAL );
            return -1;
        }
    }

    case WS_SIO_GET_EXTENSION_FUNCTION_POINTER:
    {
#define EXTENSION_FUNCTION(x, y) { x, y, #y },
        static const struct
        {
            GUID guid;
            void *func_ptr;
            const char *name;
        } guid_funcs[] = {
            EXTENSION_FUNCTION(WSAID_CONNECTEX, WS2_ConnectEx)
            EXTENSION_FUNCTION(WSAID_DISCONNECTEX, WS2_DisconnectEx)
            EXTENSION_FUNCTION(WSAID_ACCEPTEX, WS2_AcceptEx)
            EXTENSION_FUNCTION(WSAID_GETACCEPTEXSOCKADDRS, WS2_GetAcceptExSockaddrs)
            EXTENSION_FUNCTION(WSAID_TRANSMITFILE, WS2_TransmitFile)
            /* EXTENSION_FUNCTION(WSAID_TRANSMITPACKETS, WS2_TransmitPackets) */
            EXTENSION_FUNCTION(WSAID_WSARECVMSG, WS2_WSARecvMsg)
            EXTENSION_FUNCTION(WSAID_WSASENDMSG, WSASendMsg)
        };
#undef EXTENSION_FUNCTION
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(guid_funcs); i++)
        {
            if (IsEqualGUID(&guid_funcs[i].guid, in_buff))
            {
                NTSTATUS status = STATUS_SUCCESS;
                DWORD ret = 0;

                TRACE( "returning %s\n", guid_funcs[i].name );
                *(void **)out_buff = guid_funcs[i].func_ptr;

                ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                         NULL, 0, ret_size, overlapped, completion );
                *ret_size = sizeof(void *);
                SetLastError( ret );
                return ret ? -1 : 0;
            }
        }

        FIXME("SIO_GET_EXTENSION_FUNCTION_POINTER %s: stub\n", debugstr_guid(in_buff));
        SetLastError( WSAEINVAL );
        return -1;
    }

    case WS_SIO_KEEPALIVE_VALS:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_KEEPALIVE_VALS, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        if (!overlapped || completion) *ret_size = 0;
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case WS_SIO_ROUTING_INTERFACE_QUERY:
    {
        struct WS_sockaddr *daddr = (struct WS_sockaddr *)in_buff;
        struct WS_sockaddr_in *daddr_in = (struct WS_sockaddr_in *)daddr;
        struct WS_sockaddr_in *saddr_in = out_buff;
        MIB_IPFORWARDROW row;
        PMIB_IPADDRTABLE ipAddrTable = NULL;
        DWORD size, i, found_index, ret = 0;
        NTSTATUS status = STATUS_SUCCESS;

        TRACE( "-> WS_SIO_ROUTING_INTERFACE_QUERY request\n" );

        if (!in_buff || in_size < sizeof(struct WS_sockaddr) ||
            !out_buff || out_size < sizeof(struct WS_sockaddr_in))
        {
            SetLastError( WSAEFAULT );
            return -1;
        }
        if (daddr->sa_family != WS_AF_INET)
        {
            FIXME("unsupported address family %d\n", daddr->sa_family);
            SetLastError( WSAEAFNOSUPPORT );
            return -1;
        }
        if (GetBestRoute( daddr_in->sin_addr.S_un.S_addr, 0, &row ) != NOERROR ||
            GetIpAddrTable( NULL, &size, FALSE ) != ERROR_INSUFFICIENT_BUFFER)
        {
            SetLastError( WSAEFAULT );
            return -1;
        }
        ipAddrTable = HeapAlloc( GetProcessHeap(), 0, size );
        if (GetIpAddrTable( ipAddrTable, &size, FALSE ))
        {
            HeapFree( GetProcessHeap(), 0, ipAddrTable );
            SetLastError( WSAEFAULT );
            return -1;
        }
        for (i = 0, found_index = ipAddrTable->dwNumEntries;
             i < ipAddrTable->dwNumEntries; i++)
        {
            if (ipAddrTable->table[i].dwIndex == row.dwForwardIfIndex)
                found_index = i;
        }
        if (found_index == ipAddrTable->dwNumEntries)
        {
            ERR("no matching IP address for interface %d\n",
                row.dwForwardIfIndex);
            HeapFree( GetProcessHeap(), 0, ipAddrTable );
            SetLastError( WSAEFAULT );
            return -1;
        }
        saddr_in->sin_family = WS_AF_INET;
        saddr_in->sin_addr.S_un.S_addr = ipAddrTable->table[found_index].dwAddr;
        saddr_in->sin_port = 0;
        HeapFree( GetProcessHeap(), 0, ipAddrTable );

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                 NULL, 0, ret_size, overlapped, completion );
        if (!ret) *ret_size = sizeof(struct WS_sockaddr_in);
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case WS_SIO_ADDRESS_LIST_CHANGE:
    {
        int force_async = !!overlapped;
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_ADDRESS_LIST_CHANGE, &force_async, sizeof(force_async),
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case WS_SIO_UDP_CONNRESET:
    {
        NTSTATUS status = STATUS_SUCCESS;
        DWORD ret;

        FIXME( "WS_SIO_UDP_CONNRESET stub\n" );
        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                 NULL, 0, ret_size, overlapped, completion );
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case WS_SIO_BASE_HANDLE:
    {
        NTSTATUS status;
        DWORD ret;

        if (overlapped)
        {
            status = STATUS_NOT_SUPPORTED;
        }
        else
        {
            status = STATUS_SUCCESS;
            *(SOCKET *)out_buff = s;
        }
        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                 NULL, 0, ret_size, overlapped, completion );
        if (overlapped) ret = ERROR_IO_PENDING;
        if (!ret) *ret_size = sizeof(SOCKET);
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    default:
        FIXME( "unimplemented ioctl %s\n", debugstr_wsaioctl( code ) );
        /* fall through */
    case LOWORD(WS_FIONBIO): /* Netscape tries to use this */
    case WS_SIO_SET_COMPATIBILITY_MODE:
    {
        NTSTATUS status = STATUS_NOT_SUPPORTED;

        server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                           NULL, 0, ret_size, overlapped, completion );
        if (overlapped)
        {
            SetLastError( ERROR_IO_PENDING );
        }
        else
        {
            *ret_size = 0;
            SetLastError( WSAEOPNOTSUPP );
        }
        return -1;
    }
    }
}


/***********************************************************************
 *		ioctlsocket		(WS2_32.10)
 */
int WINAPI WS_ioctlsocket(SOCKET s, LONG cmd, WS_u_long *argp)
{
    DWORD ret_size;
    return WSAIoctl( s, cmd, argp, sizeof(WS_u_long), argp, sizeof(WS_u_long), &ret_size, NULL, NULL );
}


/***********************************************************************
 *      listen   (ws2_32.13)
 */
int WINAPI WS_listen( SOCKET s, int backlog )
{
    struct afd_listen_params params = {.backlog = backlog};
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#lx, backlog %d\n", s, backlog );

    status = NtDeviceIoControlFile( SOCKET2HANDLE(s), NULL, NULL, NULL, &io,
            IOCTL_AFD_LISTEN, &params, sizeof(params), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *		recv			(WS2_32.16)
 */
int WINAPI WS_recv(SOCKET s, char *buf, int len, int flags)
{
    DWORD n, dwFlags = flags;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = buf;

    if ( WS2_recv_base(s, &wsabuf, 1, &n, &dwFlags, NULL, NULL, NULL, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}

/***********************************************************************
 *		recvfrom		(WS2_32.17)
 */
int WINAPI WS_recvfrom(SOCKET s, char *buf, INT len, int flags,
                       struct WS_sockaddr *from, int *fromlen)
{
    DWORD n, dwFlags = flags;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = buf;

    if ( WS2_recv_base(s, &wsabuf, 1, &n, &dwFlags, from, fromlen, NULL, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}


/* as FD_SET(), but returns 1 if the fd was added, 0 otherwise */
static int add_fd_to_set( SOCKET fd, struct WS_fd_set *set )
{
    unsigned int i;

    for (i = 0; i < set->fd_count; ++i)
    {
        if (set->fd_array[i] == fd)
            return 0;
    }

    if (set->fd_count < WS_FD_SETSIZE)
    {
        set->fd_array[set->fd_count++] = fd;
        return 1;
    }

    return 0;
}


/***********************************************************************
 *      select   (ws2_32.18)
 */
int WINAPI WS_select( int count, WS_fd_set *read_ptr, WS_fd_set *write_ptr,
                      WS_fd_set *except_ptr, const struct WS_timeval *timeout)
{
    char buffer[offsetof( struct afd_poll_params, sockets[WS_FD_SETSIZE * 3] )] = {0};
    struct afd_poll_params *params = (struct afd_poll_params *)buffer;
    struct WS_fd_set read, write, except;
    ULONG params_size, i, j;
    SOCKET poll_socket = 0;
    IO_STATUS_BLOCK io;
    HANDLE sync_event;
    int ret_count = 0;
    NTSTATUS status;

    TRACE( "read %p, write %p, except %p, timeout %p\n", read_ptr, write_ptr, except_ptr, timeout );

    WS_FD_ZERO( &read );
    WS_FD_ZERO( &write );
    WS_FD_ZERO( &except );
    if (read_ptr) read = *read_ptr;
    if (write_ptr) write = *write_ptr;
    if (except_ptr) except = *except_ptr;

    if (!(sync_event = get_sync_event())) return -1;

    if (timeout)
        params->timeout = timeout->tv_sec * -10000000 + timeout->tv_usec * -10;
    else
        params->timeout = TIMEOUT_INFINITE;

    for (i = 0; i < read.fd_count; ++i)
    {
        params->sockets[params->count].socket = read.fd_array[i];
        params->sockets[params->count].flags = AFD_POLL_READ | AFD_POLL_ACCEPT | AFD_POLL_HUP;
        ++params->count;
        poll_socket = read.fd_array[i];
    }

    for (i = 0; i < write.fd_count; ++i)
    {
        params->sockets[params->count].socket = write.fd_array[i];
        params->sockets[params->count].flags = AFD_POLL_WRITE;
        ++params->count;
        poll_socket = write.fd_array[i];
    }

    for (i = 0; i < except.fd_count; ++i)
    {
        params->sockets[params->count].socket = except.fd_array[i];
        params->sockets[params->count].flags = AFD_POLL_OOB | AFD_POLL_CONNECT_ERR;
        ++params->count;
        poll_socket = except.fd_array[i];
    }

    if (!params->count)
    {
        SetLastError( WSAEINVAL );
        return -1;
    }

    params_size = offsetof( struct afd_poll_params, sockets[params->count] );

    status = NtDeviceIoControlFile( (HANDLE)poll_socket, sync_event, NULL, NULL, &io,
                                    IOCTL_AFD_POLL, params, params_size, params, params_size );
    if (status == STATUS_PENDING)
    {
        if (WaitForSingleObject( sync_event, INFINITE ) == WAIT_FAILED)
            return -1;
        status = io.u.Status;
    }
    if (status == STATUS_TIMEOUT) status = STATUS_SUCCESS;
    if (!status)
    {
        /* pointers may alias, so clear them all first */
        if (read_ptr) WS_FD_ZERO( read_ptr );
        if (write_ptr) WS_FD_ZERO( write_ptr );
        if (except_ptr) WS_FD_ZERO( except_ptr );

        for (i = 0; i < params->count; ++i)
        {
            unsigned int flags = params->sockets[i].flags;
            SOCKET s = params->sockets[i].socket;

            for (j = 0; j < read.fd_count; ++j)
            {
                if (read.fd_array[j] == s
                        && (flags & (AFD_POLL_READ | AFD_POLL_ACCEPT | AFD_POLL_HUP | AFD_POLL_CLOSE)))
                {
                    ret_count += add_fd_to_set( s, read_ptr );
                    flags &= ~AFD_POLL_CLOSE;
                }
            }

            if (flags & AFD_POLL_CLOSE)
                status = STATUS_INVALID_HANDLE;

            for (j = 0; j < write.fd_count; ++j)
            {
                if (write.fd_array[j] == s && (flags & AFD_POLL_WRITE))
                    ret_count += add_fd_to_set( s, write_ptr );
            }

            for (j = 0; j < except.fd_count; ++j)
            {
                if (except.fd_array[j] == s && (flags & (AFD_POLL_OOB | AFD_POLL_CONNECT_ERR)))
                    ret_count += add_fd_to_set( s, except_ptr );
            }
        }
    }

    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : ret_count;
}


static unsigned int afd_poll_flag_to_win32( unsigned int flags )
{
    static const unsigned int map[] =
    {
        FD_READ,    /* READ */
        FD_OOB,     /* OOB */
        FD_WRITE,   /* WRITE */
        FD_CLOSE,   /* HUP */
        FD_CLOSE,   /* RESET */
        0,          /* CLOSE */
        FD_CONNECT, /* CONNECT */
        FD_ACCEPT,  /* ACCEPT */
        FD_CONNECT, /* CONNECT_ERR */
    };

    unsigned int i, ret = 0;

    for (i = 0; i < ARRAY_SIZE(map); ++i)
    {
        if (flags & (1 << i)) ret |= map[i];
    }

    return ret;
}


/***********************************************************************
 *      WSAPoll   (ws2_32.@)
 */
int WINAPI WSAPoll( WSAPOLLFD *fds, ULONG count, int timeout )
{
    struct afd_poll_params *params;
    ULONG params_size, i, j;
    SOCKET poll_socket = 0;
    IO_STATUS_BLOCK io;
    HANDLE sync_event;
    int ret_count = 0;
    NTSTATUS status;

    if (!count)
    {
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }
    if (!fds)
    {
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    if (!(sync_event = get_sync_event())) return -1;

    params_size = offsetof( struct afd_poll_params, sockets[count] );
    if (!(params = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, params_size )))
    {
        SetLastError(WSAENOBUFS);
        return SOCKET_ERROR;
    }

    params->timeout = (timeout >= 0 ? timeout * -10000 : TIMEOUT_INFINITE);

    for (i = 0; i < count; ++i)
    {
        unsigned int flags = AFD_POLL_HUP | AFD_POLL_RESET | AFD_POLL_CONNECT_ERR;

        if ((INT_PTR)fds[i].fd < 0 || !socket_list_find( fds[i].fd ))
        {
            fds[i].revents = WS_POLLNVAL;
            continue;
        }

        poll_socket = fds[i].fd;
        params->sockets[params->count].socket = fds[i].fd;

        if (fds[i].events & WS_POLLRDNORM)
            flags |= AFD_POLL_ACCEPT | AFD_POLL_READ;
        if (fds[i].events & WS_POLLRDBAND)
            flags |= AFD_POLL_OOB;
        if (fds[i].events & WS_POLLWRNORM)
            flags |= AFD_POLL_WRITE;
        params->sockets[params->count].flags = flags;
        ++params->count;

        fds[i].revents = 0;
    }

    if (!poll_socket)
    {
        SetLastError( WSAENOTSOCK );
        HeapFree( GetProcessHeap(), 0, params );
        return -1;
    }

    status = NtDeviceIoControlFile( (HANDLE)poll_socket, sync_event, NULL, NULL, &io, IOCTL_AFD_POLL,
                                    params, params_size, params, params_size );
    if (status == STATUS_PENDING)
    {
        if (WaitForSingleObject( sync_event, INFINITE ) == WAIT_FAILED)
        {
            HeapFree( GetProcessHeap(), 0, params );
            return -1;
        }
        status = io.u.Status;
    }
    if (!status)
    {
        for (i = 0; i < count; ++i)
        {
            for (j = 0; j < params->count; ++j)
            {
                if (fds[i].fd == params->sockets[j].socket)
                {
                    unsigned int revents = 0;

                    if (params->sockets[j].flags & (AFD_POLL_ACCEPT | AFD_POLL_READ))
                        revents |= WS_POLLRDNORM;
                    if (params->sockets[j].flags & AFD_POLL_OOB)
                        revents |= WS_POLLRDBAND;
                    if (params->sockets[j].flags & AFD_POLL_WRITE)
                        revents |= WS_POLLWRNORM;
                    if (params->sockets[j].flags & AFD_POLL_HUP)
                        revents |= WS_POLLHUP;
                    if (params->sockets[j].flags & (AFD_POLL_RESET | AFD_POLL_CONNECT_ERR))
                        revents |= WS_POLLERR;
                    if (params->sockets[j].flags & AFD_POLL_CLOSE)
                        revents |= WS_POLLNVAL;

                    fds[i].revents = revents & (fds[i].events | WS_POLLHUP | WS_POLLERR | WS_POLLNVAL);

                    if (fds[i].revents)
                        ++ret_count;
                }
            }
        }
    }
    if (status == STATUS_TIMEOUT) status = STATUS_SUCCESS;

    HeapFree( GetProcessHeap(), 0, params );

    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : ret_count;
}


/***********************************************************************
 *		send			(WS2_32.19)
 */
int WINAPI WS_send(SOCKET s, const char *buf, int len, int flags)
{
    DWORD n;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = (char*) buf;

    if ( WS2_sendto( s, &wsabuf, 1, &n, flags, NULL, 0, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}

/***********************************************************************
 *		WSASend			(WS2_32.72)
 */
INT WINAPI WSASend( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                    LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                    LPWSAOVERLAPPED lpOverlapped,
                    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    return WS2_sendto( s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags,
                      NULL, 0, lpOverlapped, lpCompletionRoutine );
}

/***********************************************************************
 *              WSASendDisconnect       (WS2_32.73)
 */
INT WINAPI WSASendDisconnect( SOCKET s, LPWSABUF lpBuffers )
{
    return WS_shutdown( s, SD_SEND );
}


/***********************************************************************
 *		WSASendTo		(WS2_32.74)
 */
INT WINAPI WSASendTo( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                      LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                      const struct WS_sockaddr *to, int tolen,
                      LPWSAOVERLAPPED lpOverlapped,
                      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    /* Garena hooks WSASendTo(), so we need a wrapper */
    return WS2_sendto( s, lpBuffers, dwBufferCount,
                lpNumberOfBytesSent, dwFlags,
                to, tolen,
                lpOverlapped, lpCompletionRoutine );
}

/***********************************************************************
 *		sendto		(WS2_32.20)
 */
int WINAPI WS_sendto(SOCKET s, const char *buf, int len, int flags,
                              const struct WS_sockaddr *to, int tolen)
{
    DWORD n;
    WSABUF wsabuf;

    wsabuf.len = len;
    wsabuf.buf = (char*) buf;

    if ( WS2_sendto(s, &wsabuf, 1, &n, flags, to, tolen, NULL, NULL) == SOCKET_ERROR )
        return SOCKET_ERROR;
    else
        return n;
}


static int server_setsockopt( SOCKET s, ULONG code, const char *optval, int optlen )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, code, (void *)optval, optlen, NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *		setsockopt		(WS2_32.21)
 */
int WINAPI WS_setsockopt(SOCKET s, int level, int optname,
                         const char *optval, int optlen)
{
    int fd;
    int woptval;

    TRACE("(socket %04lx, %s, optval %s, optlen %d)\n", s,
          debugstr_sockopt(level, optname), debugstr_optval(optval, optlen),
          optlen);

    /* some broken apps pass the value directly instead of a pointer to it */
    if(optlen && IS_INTRESOURCE(optval))
    {
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    switch(level)
    {
    case WS_SOL_SOCKET:
        switch(optname)
        {
        case WS_SO_BROADCAST:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_BROADCAST, optval, optlen );

        case WS_SO_DONTLINGER:
        {
            struct WS_linger linger;

            if (!optval)
            {
                SetLastError( WSAEFAULT );
                return -1;
            }

            linger.l_onoff  = !*(const BOOL *)optval;
            linger.l_linger = 0;
            return WS_setsockopt( s, WS_SOL_SOCKET, WS_SO_LINGER, (char *)&linger, sizeof(linger) );
        }

        case WS_SO_ERROR:
            FIXME( "SO_ERROR, stub!\n" );
            SetLastError( WSAENOPROTOOPT );
            return -1;

        case WS_SO_KEEPALIVE:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_KEEPALIVE, optval, optlen );

        case WS_SO_LINGER:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_LINGER, optval, optlen );

        case WS_SO_OOBINLINE:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_OOBINLINE, optval, optlen );

        case WS_SO_RCVBUF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_RCVBUF, optval, optlen );

        case WS_SO_RCVTIMEO:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_RCVTIMEO, optval, optlen );

        case WS_SO_REUSEADDR:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_REUSEADDR, optval, optlen );

        case WS_SO_SNDBUF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_SNDBUF, optval, optlen );

        case WS_SO_SNDTIMEO:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_SNDTIMEO, optval, optlen );

        /* SO_DEBUG is a privileged operation, ignore it. */
        case WS_SO_DEBUG:
            TRACE("Ignoring SO_DEBUG\n");
            return 0;

        /* For some reason the game GrandPrixLegends does set SO_DONTROUTE on its
         * socket. According to MSDN, this option is silently ignored.*/
        case WS_SO_DONTROUTE:
            TRACE("Ignoring SO_DONTROUTE\n");
            return 0;

        /* Stops two sockets from being bound to the same port. Always happens
         * on unix systems, so just drop it. */
        case WS_SO_EXCLUSIVEADDRUSE:
            TRACE("Ignoring SO_EXCLUSIVEADDRUSE, is always set.\n");
            return 0;

        /* After a ConnectEx call succeeds, the socket can't be used with half of the
         * normal winsock functions on windows. We don't have that problem. */
        case WS_SO_UPDATE_CONNECT_CONTEXT:
            TRACE("Ignoring SO_UPDATE_CONNECT_CONTEXT, since our sockets are normal\n");
            return 0;

        /* After a AcceptEx call succeeds, the socket can't be used with half of the
         * normal winsock functions on windows. We don't have that problem. */
        case WS_SO_UPDATE_ACCEPT_CONTEXT:
            TRACE("Ignoring SO_UPDATE_ACCEPT_CONTEXT, since our sockets are normal\n");
            return 0;

        /* SO_OPENTYPE does not require a valid socket handle. */
        case WS_SO_OPENTYPE:
            if (!optlen || optlen < sizeof(int) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            get_per_thread_data()->opentype = *(const int *)optval;
            TRACE("setting global SO_OPENTYPE = 0x%x\n", *((const int*)optval) );
            return 0;

        case WS_SO_RANDOMIZE_PORT:
            FIXME("Ignoring WS_SO_RANDOMIZE_PORT\n");
            return 0;

        case WS_SO_PORT_SCALABILITY:
            FIXME("Ignoring WS_SO_PORT_SCALABILITY\n");
            return 0;

        case WS_SO_REUSE_UNICASTPORT:
            FIXME("Ignoring WS_SO_REUSE_UNICASTPORT\n");
            return 0;

        case WS_SO_REUSE_MULTICASTPORT:
            FIXME("Ignoring WS_SO_REUSE_MULTICASTPORT\n");
            return 0;

        default:
            TRACE("Unknown SOL_SOCKET optname: 0x%08x\n", optname);
            /* fall through */

        case WS_SO_ACCEPTCONN:
        case WS_SO_TYPE:
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        }
        break; /* case WS_SOL_SOCKET */

#ifdef HAS_IPX
    case WS_NSPROTO_IPX:
        switch(optname)
        {
        case WS_IPX_PTYPE:
            return set_ipx_packettype(s, *(int*)optval);

        case WS_IPX_FILTERPTYPE:
            /* Sets the receive filter packet type, at the moment we don't support it */
            FIXME("IPX_FILTERPTYPE: %x\n", *optval);
            /* Returning 0 is better for now than returning a SOCKET_ERROR */
            return 0;

        default:
            FIXME("opt_name:%x\n", optname);
            return SOCKET_ERROR;
        }
        break; /* case WS_NSPROTO_IPX */
#endif

    /* Levels WS_IPPROTO_TCP and WS_IPPROTO_IP convert directly */
    case WS_IPPROTO_TCP:
        switch(optname)
        {
        case WS_TCP_NODELAY:
            convert_sockopt(&level, &optname);
            break;
        default:
            FIXME("Unknown IPPROTO_TCP optname 0x%08x\n", optname);
            return SOCKET_ERROR;
        }
        break;

    case WS_IPPROTO_IP:
        switch(optname)
        {
        case WS_IP_ADD_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_ADD_MEMBERSHIP, optval, optlen );

        case WS_IP_ADD_SOURCE_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_ADD_SOURCE_MEMBERSHIP, optval, optlen );

        case WS_IP_BLOCK_SOURCE:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_BLOCK_SOURCE, optval, optlen );

        case WS_IP_DONTFRAGMENT:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_DONTFRAGMENT, optval, optlen );

        case WS_IP_DROP_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_DROP_MEMBERSHIP, optval, optlen );

        case WS_IP_DROP_SOURCE_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_DROP_SOURCE_MEMBERSHIP, optval, optlen );

        case WS_IP_HDRINCL:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_HDRINCL, optval, optlen );

        case WS_IP_MULTICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_MULTICAST_IF, optval, optlen );

        case WS_IP_MULTICAST_LOOP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_MULTICAST_LOOP, optval, optlen );

        case WS_IP_MULTICAST_TTL:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_MULTICAST_TTL, optval, optlen );

        case WS_IP_OPTIONS:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_OPTIONS, optval, optlen );

        case WS_IP_PKTINFO:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_PKTINFO, optval, optlen );

        case WS_IP_TOS:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_TOS, optval, optlen );

        case WS_IP_TTL:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_TTL, optval, optlen );

        case WS_IP_UNBLOCK_SOURCE:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_UNBLOCK_SOURCE, optval, optlen );

        case WS_IP_UNICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_UNICAST_IF, optval, optlen );

        default:
            FIXME("Unknown IPPROTO_IP optname 0x%08x\n", optname);
            return SOCKET_ERROR;
        }
        break;

    case WS_IPPROTO_IPV6:
        switch(optname)
        {
        case WS_IPV6_ADD_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_ADD_MEMBERSHIP, optval, optlen );

        case WS_IPV6_DONTFRAG:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_DONTFRAG, optval, optlen );

        case WS_IPV6_DROP_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_DROP_MEMBERSHIP, optval, optlen );

        case WS_IPV6_MULTICAST_HOPS:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_MULTICAST_HOPS, optval, optlen );

        case WS_IPV6_MULTICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_MULTICAST_IF, optval, optlen );

        case WS_IPV6_MULTICAST_LOOP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_MULTICAST_LOOP, optval, optlen );

        case WS_IPV6_PROTECTION_LEVEL:
            FIXME("IPV6_PROTECTION_LEVEL is ignored!\n");
            return 0;

        case WS_IPV6_UNICAST_HOPS:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_UNICAST_HOPS, optval, optlen );

        case WS_IPV6_UNICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_UNICAST_IF, optval, optlen );

        case WS_IPV6_V6ONLY:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_V6ONLY, optval, optlen );

        default:
            FIXME("Unknown IPPROTO_IPV6 optname 0x%08x\n", optname);
            return SOCKET_ERROR;
        }
        break;

    default:
        WARN("Unknown level: 0x%08x\n", level);
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    } /* end switch(level) */

    /* avoid endianness issues if argument is a 16-bit int */
    if (optval && optlen < sizeof(int))
    {
        woptval= *((const INT16 *) optval);
        optval= (char*) &woptval;
        woptval&= (1 << optlen * 8) - 1;
        optlen=sizeof(int);
    }
    fd = get_sock_fd( s, 0, NULL );
    if (fd == -1) return SOCKET_ERROR;

    if (setsockopt(fd, level, optname, optval, optlen) == 0)
    {
#ifdef __APPLE__
        if (level == SOL_SOCKET && optname == SO_REUSEADDR &&
            setsockopt(fd, level, SO_REUSEPORT, optval, optlen) != 0)
        {
            SetLastError(wsaErrno());
            release_sock_fd( s, fd );
            return SOCKET_ERROR;
        }
#endif
        release_sock_fd( s, fd );
        return 0;
    }
    TRACE("Setting socket error, %d\n", wsaErrno());
    SetLastError(wsaErrno());
    release_sock_fd( s, fd );

    return SOCKET_ERROR;
}


/***********************************************************************
 *      shutdown   (ws2_32.22)
 */
int WINAPI WS_shutdown( SOCKET s, int how )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#lx, how %u\n", s, how );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io,
                                    IOCTL_AFD_WINE_SHUTDOWN, &how, sizeof(how), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *		socket		(WS2_32.23)
 */
SOCKET WINAPI WS_socket(int af, int type, int protocol)
{
    TRACE("af=%d type=%d protocol=%d\n", af, type, protocol);

    return WSASocketW( af, type, protocol, NULL, 0,
                       get_per_thread_data()->opentype ? 0 : WSA_FLAG_OVERLAPPED );
}


/***********************************************************************
 *      WSAEnumNetworkEvents   (ws2_32.@)
 */
int WINAPI WSAEnumNetworkEvents( SOCKET s, WSAEVENT event, WSANETWORKEVENTS *ret_events )
{
    struct afd_get_events_params params;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#lx, event %p, events %p\n", s, event, ret_events );

    ResetEvent( event );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_GET_EVENTS,
                                    NULL, 0, &params, sizeof(params) );
    if (!status)
    {
        ret_events->lNetworkEvents = afd_poll_flag_to_win32( params.flags );

        if (ret_events->lNetworkEvents & FD_READ)
            ret_events->iErrorCode[FD_READ_BIT] = NtStatusToWSAError( params.status[AFD_POLL_BIT_READ] );

        if (ret_events->lNetworkEvents & FD_WRITE)
            ret_events->iErrorCode[FD_WRITE_BIT] = NtStatusToWSAError( params.status[AFD_POLL_BIT_WRITE] );

        if (ret_events->lNetworkEvents & FD_OOB)
            ret_events->iErrorCode[FD_OOB_BIT] = NtStatusToWSAError( params.status[AFD_POLL_BIT_OOB] );

        if (ret_events->lNetworkEvents & FD_ACCEPT)
            ret_events->iErrorCode[FD_ACCEPT_BIT] = NtStatusToWSAError( params.status[AFD_POLL_BIT_ACCEPT] );

        if (ret_events->lNetworkEvents & FD_CONNECT)
            ret_events->iErrorCode[FD_CONNECT_BIT] = NtStatusToWSAError( params.status[AFD_POLL_BIT_CONNECT_ERR] );

        if (ret_events->lNetworkEvents & FD_CLOSE)
        {
            if (!(ret_events->iErrorCode[FD_CLOSE_BIT] = NtStatusToWSAError( params.status[AFD_POLL_BIT_HUP] )))
                ret_events->iErrorCode[FD_CLOSE_BIT] = NtStatusToWSAError( params.status[AFD_POLL_BIT_RESET] );
        }
    }
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


static unsigned int afd_poll_flag_from_win32( unsigned int flags )
{
    static const unsigned int map[] =
    {
        AFD_POLL_READ,
        AFD_POLL_WRITE,
        AFD_POLL_OOB,
        AFD_POLL_ACCEPT,
        AFD_POLL_CONNECT | AFD_POLL_CONNECT_ERR,
        AFD_POLL_RESET | AFD_POLL_HUP,
    };

    unsigned int i, ret = 0;

    for (i = 0; i < ARRAY_SIZE(map); ++i)
    {
        if (flags & (1 << i)) ret |= map[i];
    }

    return ret;
}


/***********************************************************************
 *      WSAEventSelect   (ws2_32.@)
 */
int WINAPI WSAEventSelect( SOCKET s, WSAEVENT event, LONG mask )
{
    struct afd_event_select_params params;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#lx, event %p, mask %#x\n", s, event, mask );

    params.event = event;
    params.mask = afd_poll_flag_from_win32( mask );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_EVENT_SELECT,
                                    &params, sizeof(params), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/**********************************************************************
 *      WSAGetOverlappedResult (WS2_32.40)
 */
BOOL WINAPI WSAGetOverlappedResult( SOCKET s, LPWSAOVERLAPPED lpOverlapped,
                                    LPDWORD lpcbTransfer, BOOL fWait,
                                    LPDWORD lpdwFlags )
{
    NTSTATUS status;

    TRACE( "socket %04lx ovl %p trans %p, wait %d flags %p\n",
           s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags );

    if ( lpOverlapped == NULL )
    {
        ERR( "Invalid pointer\n" );
        SetLastError(WSA_INVALID_PARAMETER);
        return FALSE;
    }

    status = lpOverlapped->Internal;
    if (status == STATUS_PENDING)
    {
        if (!fWait)
        {
            SetLastError( WSA_IO_INCOMPLETE );
            return FALSE;
        }

        if (WaitForSingleObject( lpOverlapped->hEvent ? lpOverlapped->hEvent : SOCKET2HANDLE(s),
                                 INFINITE ) == WAIT_FAILED)
            return FALSE;
        status = lpOverlapped->Internal;
    }

    if ( lpcbTransfer )
        *lpcbTransfer = lpOverlapped->InternalHigh;

    if ( lpdwFlags )
        *lpdwFlags = lpOverlapped->u.s.Offset;

    SetLastError( NtStatusToWSAError(status) );
    return NT_SUCCESS( status );
}


/***********************************************************************
 *      WSAAsyncSelect   (ws2_32.@)
 */
int WINAPI WSAAsyncSelect( SOCKET s, HWND window, UINT message, LONG mask )
{
    struct afd_message_select_params params;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#lx, window %p, message %#x, mask %#x\n", s, window, message, mask );

    params.handle = wine_server_obj_handle( (HANDLE)s );
    params.window = wine_server_user_handle( window );
    params.message = message;
    params.mask = afd_poll_flag_from_win32( mask );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_WINE_MESSAGE_SELECT,
                                    &params, sizeof(params), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *      WSACreateEvent          (WS2_32.31)
 *
 */
WSAEVENT WINAPI WSACreateEvent(void)
{
    /* Create a manual-reset event, with initial state: unsignaled */
    TRACE("\n");

    return CreateEventW(NULL, TRUE, FALSE, NULL);
}

/***********************************************************************
 *      WSACloseEvent          (WS2_32.29)
 *
 */
BOOL WINAPI WSACloseEvent(WSAEVENT event)
{
    TRACE ("event=%p\n", event);

    return CloseHandle(event);
}

/***********************************************************************
 *      WSASocketA          (WS2_32.78)
 *
 */
SOCKET WINAPI WSASocketA(int af, int type, int protocol,
                         LPWSAPROTOCOL_INFOA lpProtocolInfo,
                         GROUP g, DWORD dwFlags)
{
    INT len;
    WSAPROTOCOL_INFOW info;

    TRACE("af=%d type=%d protocol=%d protocol_info=%p group=%d flags=0x%x\n",
          af, type, protocol, lpProtocolInfo, g, dwFlags);

    if (!lpProtocolInfo) return WSASocketW(af, type, protocol, NULL, g, dwFlags);

    memcpy(&info, lpProtocolInfo, FIELD_OFFSET(WSAPROTOCOL_INFOW, szProtocol));
    len = MultiByteToWideChar(CP_ACP, 0, lpProtocolInfo->szProtocol, -1,
                              info.szProtocol, WSAPROTOCOL_LEN + 1);

    if (!len)
    {
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    return WSASocketW(af, type, protocol, &info, g, dwFlags);
}

/***********************************************************************
 *      WSASocketW          (WS2_32.79)
 *
 */
SOCKET WINAPI WSASocketW(int af, int type, int protocol,
                         LPWSAPROTOCOL_INFOW lpProtocolInfo,
                         GROUP g, DWORD flags)
{
    static const WCHAR afdW[] = {'\\','D','e','v','i','c','e','\\','A','f','d',0};
    struct afd_create_params create_params;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    SOCKET ret;
    DWORD err;

   /*
      FIXME: The "advanced" parameters of WSASocketW (lpProtocolInfo,
      g, dwFlags except WSA_FLAG_OVERLAPPED) are ignored.
   */

   TRACE("af=%d type=%d protocol=%d protocol_info=%p group=%d flags=0x%x\n",
         af, type, protocol, lpProtocolInfo, g, flags );

    if (!num_startup)
    {
        err = WSANOTINITIALISED;
        goto done;
    }

    /* hack for WSADuplicateSocket */
    if (lpProtocolInfo && lpProtocolInfo->dwServiceFlags4 == 0xff00ff00)
    {
        ret = lpProtocolInfo->dwServiceFlags3;
        TRACE("\tgot duplicate %04lx\n", ret);
        if (!socket_list_add(ret))
        {
            CloseHandle(SOCKET2HANDLE(ret));
            return INVALID_SOCKET;
        }
        return ret;
    }

    if (lpProtocolInfo)
    {
        if (af == FROM_PROTOCOL_INFO || !af)
            af = lpProtocolInfo->iAddressFamily;
        if (type == FROM_PROTOCOL_INFO || !type)
            type = lpProtocolInfo->iSocketType;
        if (protocol == FROM_PROTOCOL_INFO || !protocol)
            protocol = lpProtocolInfo->iProtocol;
    }

    if (!af && !protocol)
    {
        WSASetLastError(WSAEINVAL);
        return INVALID_SOCKET;
    }

    if (!af && lpProtocolInfo)
    {
        WSASetLastError(WSAEAFNOSUPPORT);
        return INVALID_SOCKET;
    }

    if (!af || !type || !protocol)
    {
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(supported_protocols); ++i)
        {
            const WSAPROTOCOL_INFOW *info = &supported_protocols[i];

            if (af && af != info->iAddressFamily) continue;
            if (type && type != info->iSocketType) continue;
            if (protocol && (protocol < info->iProtocol ||
                             protocol > info->iProtocol + info->iProtocolMaxOffset)) continue;
            if (!protocol && !(info->dwProviderFlags & PFL_MATCHES_PROTOCOL_ZERO)) continue;

            if (!af) af = supported_protocols[i].iAddressFamily;
            if (!type) type = supported_protocols[i].iSocketType;
            if (!protocol) protocol = supported_protocols[i].iProtocol;
            break;
        }
    }

    RtlInitUnicodeString(&string, afdW);
    InitializeObjectAttributes(&attr, &string, (flags & WSA_FLAG_NO_HANDLE_INHERIT) ? 0 : OBJ_INHERIT, NULL, NULL);
    if ((status = NtOpenFile(&handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &attr,
            &io, 0, (flags & WSA_FLAG_OVERLAPPED) ? 0 : FILE_SYNCHRONOUS_IO_NONALERT)))
    {
        WARN("Failed to create socket, status %#x.\n", status);
        WSASetLastError(NtStatusToWSAError(status));
        return INVALID_SOCKET;
    }

    create_params.family = af;
    create_params.type = type;
    create_params.protocol = protocol;
    create_params.flags = flags & ~(WSA_FLAG_NO_HANDLE_INHERIT | WSA_FLAG_OVERLAPPED);
    if ((status = NtDeviceIoControlFile(handle, NULL, NULL, NULL, &io,
            IOCTL_AFD_WINE_CREATE, &create_params, sizeof(create_params), NULL, 0)))
    {
        WARN("Failed to initialize socket, status %#x.\n", status);
        err = RtlNtStatusToDosError( status );
        if (err == WSAEACCES) /* raw socket denied */
        {
            if (type == SOCK_RAW)
                ERR_(winediag)("Failed to create a socket of type SOCK_RAW, this requires special permissions.\n");
            else
                ERR_(winediag)("Failed to create socket, this requires special permissions.\n");
        }
        WSASetLastError(err);
        NtClose(handle);
        return INVALID_SOCKET;
    }

    ret = HANDLE2SOCKET(handle);
    TRACE("\tcreated %04lx\n", ret );

    if (!socket_list_add(ret))
    {
        CloseHandle(handle);
        return INVALID_SOCKET;
    }
    return ret;

done:
    WARN("\t\tfailed, error %d!\n", err);
    SetLastError(err);
    return INVALID_SOCKET;
}

/***********************************************************************
 *      WSAJoinLeaf          (WS2_32.58)
 *
 */
SOCKET WINAPI WSAJoinLeaf(
        SOCKET s,
        const struct WS_sockaddr *addr,
        int addrlen,
        LPWSABUF lpCallerData,
        LPWSABUF lpCalleeData,
        LPQOS lpSQOS,
        LPQOS lpGQOS,
        DWORD dwFlags)
{
    FIXME("stub.\n");
    return INVALID_SOCKET;
}

/***********************************************************************
 *      __WSAFDIsSet			(WS2_32.151)
 */
int WINAPI __WSAFDIsSet(SOCKET s, WS_fd_set *set)
{
  int i = set->fd_count, ret = 0;

  while (i--)
      if (set->fd_array[i] == s)
      {
          ret = 1;
          break;
      }

  TRACE("(socket %04lx, fd_set %p, count %i) <- %d\n", s, set, set->fd_count, ret);
  return ret;
}

/***********************************************************************
 *      WSAIsBlocking			(WS2_32.114)
 */
BOOL WINAPI WSAIsBlocking(void)
{
  /* By default WinSock should set all its sockets to non-blocking mode
   * and poll in PeekMessage loop when processing "blocking" ones. This
   * function is supposed to tell if the program is in this loop. Our
   * blocking calls are truly blocking so we always return FALSE.
   *
   * Note: It is allowed to call this function without prior WSAStartup().
   */

  TRACE("\n");
  return FALSE;
}

/***********************************************************************
 *      WSACancelBlockingCall		(WS2_32.113)
 */
INT WINAPI WSACancelBlockingCall(void)
{
    TRACE("\n");
    return 0;
}

static INT WINAPI WSA_DefaultBlockingHook( FARPROC x )
{
    FIXME("How was this called?\n");
    return x();
}


/***********************************************************************
 *      WSASetBlockingHook (WS2_32.109)
 */
FARPROC WINAPI WSASetBlockingHook(FARPROC lpBlockFunc)
{
  FARPROC prev = blocking_hook;
  blocking_hook = lpBlockFunc;
  TRACE("hook %p\n", lpBlockFunc);
  return prev;
}


/***********************************************************************
 *      WSAUnhookBlockingHook (WS2_32.110)
 */
INT WINAPI WSAUnhookBlockingHook(void)
{
    blocking_hook = (FARPROC)WSA_DefaultBlockingHook;
    return 0;
}


/***********************************************************************
 *		WSARecv			(WS2_32.67)
 */
int WINAPI WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                   LPDWORD NumberOfBytesReceived, LPDWORD lpFlags,
                   LPWSAOVERLAPPED lpOverlapped,
                   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return WS2_recv_base(s, lpBuffers, dwBufferCount, NumberOfBytesReceived, lpFlags,
                       NULL, NULL, lpOverlapped, lpCompletionRoutine, NULL);
}


/***********************************************************************
 *              WSARecvFrom             (WS2_32.69)
 */
INT WINAPI WSARecvFrom( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                        LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct WS_sockaddr *lpFrom,
                        LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped,
                        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )

{
    /* Garena hooks WSARecvFrom(), so we need a wrapper */
    return WS2_recv_base( s, lpBuffers, dwBufferCount,
                lpNumberOfBytesRecvd, lpFlags,
                lpFrom, lpFromlen,
                lpOverlapped, lpCompletionRoutine, NULL );
}


/***********************************************************************
 *      WSAAccept   (ws2_32.@)
 */
SOCKET WINAPI WSAAccept( SOCKET s, struct WS_sockaddr *addr, int *addrlen,
                         LPCONDITIONPROC callback, DWORD_PTR context )
{
    int ret = 0, size;
    WSABUF caller_id, caller_data, callee_id, callee_data;
    struct WS_sockaddr src_addr, dst_addr;
    GROUP group;
    SOCKET cs;

    TRACE( "socket %#lx, addr %p, addrlen %p, callback %p, context %#lx\n",
           s, addr, addrlen, callback, context );

    cs = WS_accept(s, addr, addrlen);
    if (cs == SOCKET_ERROR) return SOCKET_ERROR;
    if (!callback) return cs;

    if (addr && addrlen)
    {
        caller_id.buf = (char *)addr;
        caller_id.len = *addrlen;
    }
    else
    {
        size = sizeof(src_addr);
        WS_getpeername( cs, &src_addr, &size );
        caller_id.buf = (char *)&src_addr;
        caller_id.len = size;
    }
    caller_data.buf = NULL;
    caller_data.len = 0;

    size = sizeof(dst_addr);
    WS_getsockname( cs, &dst_addr, &size );

    callee_id.buf = (char *)&dst_addr;
    callee_id.len = sizeof(dst_addr);

    ret = (*callback)( &caller_id, &caller_data, NULL, NULL,
                       &callee_id, &callee_data, &group, context );

    switch (ret)
    {
    case CF_ACCEPT:
        return cs;

    case CF_DEFER:
    {
        obj_handle_t server_handle = cs;
        IO_STATUS_BLOCK io;
        NTSTATUS status;

        status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_WINE_DEFER,
                                        &server_handle, sizeof(server_handle), NULL, 0 );
        SetLastError( status ? RtlNtStatusToDosError( status ) : WSATRY_AGAIN );
        return -1;
    }

    case CF_REJECT:
        WS_closesocket( cs );
        SetLastError( WSAECONNREFUSED );
        return SOCKET_ERROR;

    default:
        FIXME( "Unknown return type from Condition function\n" );
        SetLastError( WSAENOTSOCK );
        return SOCKET_ERROR;
    }
}


/***********************************************************************
 *              WSADuplicateSocketA                      (WS2_32.32)
 */
int WINAPI WSADuplicateSocketA( SOCKET s, DWORD dwProcessId, LPWSAPROTOCOL_INFOA lpProtocolInfo )
{
    return WS_DuplicateSocket(FALSE, s, dwProcessId, (LPWSAPROTOCOL_INFOW) lpProtocolInfo);
}

/***********************************************************************
 *              WSADuplicateSocketW                      (WS2_32.33)
 */
int WINAPI WSADuplicateSocketW( SOCKET s, DWORD dwProcessId, LPWSAPROTOCOL_INFOW lpProtocolInfo )
{
    return WS_DuplicateSocket(TRUE, s, dwProcessId, lpProtocolInfo);
}


/***********************************************************************
 *              WSAGetQOSByName                             (WS2_32.41)
 */
BOOL WINAPI WSAGetQOSByName( SOCKET s, LPWSABUF lpQOSName, LPQOS lpQOS )
{
    FIXME( "(0x%04lx %p %p) Stub!\n", s, lpQOSName, lpQOS );
    return FALSE;
}


/***********************************************************************
 *              WSARecvDisconnect                           (WS2_32.68)
 */
INT WINAPI WSARecvDisconnect( SOCKET s, LPWSABUF disconnectdata )
{
    TRACE( "(%04lx %p)\n", s, disconnectdata );

    return WS_shutdown( s, SD_RECEIVE );
}


static BOOL protocol_matches_filter( const int *filter, int protocol )
{
    if (!filter) return TRUE;
    while (*filter)
    {
        if (protocol == *filter++) return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *          WSAEnumProtocolsA       [WS2_32.@]
 *
 *    see function WSAEnumProtocolsW
 */
int WINAPI WSAEnumProtocolsA( int *filter, WSAPROTOCOL_INFOA *protocols, DWORD *size )
{
    DWORD i, count = 0;

    TRACE("filter %p, protocols %p, size %p\n", filter, protocols, size);

    for (i = 0; i < ARRAY_SIZE(supported_protocols); ++i)
    {
        if (protocol_matches_filter( filter, supported_protocols[i].iProtocol ))
            ++count;
    }

    if (!protocols || *size < count * sizeof(WSAPROTOCOL_INFOA))
    {
        *size = count * sizeof(WSAPROTOCOL_INFOA);
        WSASetLastError( WSAENOBUFS );
        return SOCKET_ERROR;
    }

    count = 0;
    for (i = 0; i < ARRAY_SIZE(supported_protocols); ++i)
    {
        if (protocol_matches_filter( filter, supported_protocols[i].iProtocol ))
        {
            memcpy( &protocols[count], &supported_protocols[i], offsetof( WSAPROTOCOL_INFOW, szProtocol ) );
            WideCharToMultiByte( CP_ACP, 0, supported_protocols[i].szProtocol, -1,
                                 protocols[count].szProtocol, sizeof(protocols[count].szProtocol), NULL, NULL );
            ++count;
        }
    }
    return count;
}

/*****************************************************************************
 *          WSAEnumProtocolsW       [WS2_32.@]
 *
 * Retrieves information about specified set of active network protocols.
 *
 * PARAMS
 *  protocols [I]   Pointer to null-terminated array of protocol id's. NULL
 *                  retrieves information on all available protocols.
 *  buffer    [I]   Pointer to a buffer to be filled with WSAPROTOCOL_INFO
 *                  structures.
 *  len       [I/O] Pointer to a variable specifying buffer size. On output
 *                  the variable holds the number of bytes needed when the
 *                  specified size is too small.
 *
 * RETURNS
 *  Success: number of WSAPROTOCOL_INFO structures in buffer.
 *  Failure: SOCKET_ERROR
 *
 * NOTES
 *  NT4SP5 does not return SPX if protocols == NULL
 *
 * BUGS
 *  - NT4SP5 returns in addition these list of NETBIOS protocols
 *    (address family 17), each entry two times one for socket type 2 and 5
 *
 *    iProtocol   szProtocol
 *    0x80000000  \Device\NwlnkNb
 *    0xfffffffa  \Device\NetBT_CBENT7
 *    0xfffffffb  \Device\Nbf_CBENT7
 *    0xfffffffc  \Device\NetBT_NdisWan5
 *    0xfffffffd  \Device\NetBT_El9202
 *    0xfffffffe  \Device\Nbf_El9202
 *    0xffffffff  \Device\Nbf_NdisWan4
 *
 *  - there is no check that the operating system supports the returned
 *    protocols
 */
int WINAPI WSAEnumProtocolsW( int *filter, WSAPROTOCOL_INFOW *protocols, DWORD *size )
{
    DWORD i, count = 0;

    TRACE("filter %p, protocols %p, size %p\n", filter, protocols, size);

    for (i = 0; i < ARRAY_SIZE(supported_protocols); ++i)
    {
        if (protocol_matches_filter( filter, supported_protocols[i].iProtocol ))
            ++count;
    }

    if (!protocols || *size < count * sizeof(WSAPROTOCOL_INFOW))
    {
        *size = count * sizeof(WSAPROTOCOL_INFOW);
        WSASetLastError( WSAENOBUFS );
        return SOCKET_ERROR;
    }

    count = 0;
    for (i = 0; i < ARRAY_SIZE(supported_protocols); ++i)
    {
        if (protocol_matches_filter( filter, supported_protocols[i].iProtocol ))
            protocols[count++] = supported_protocols[i];
    }
    return count;
}
