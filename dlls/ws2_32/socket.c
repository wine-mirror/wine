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

#include "ws2_32_private.h"

#define FILE_USE_FILE_POINTER_POSITION ((LONGLONG)-2)

WINE_DEFAULT_DEBUG_CHANNEL(winsock);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define TIMEOUT_INFINITE _I64_MAX

#define u64_from_user_ptr(ptr) ((ULONGLONG)(uintptr_t)(ptr))

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
        .iAddressFamily = AF_INET,
        .iMaxSockAddr = sizeof(struct sockaddr_in),
        .iMinSockAddr = sizeof(struct sockaddr_in),
        .iSocketType = SOCK_STREAM,
        .iProtocol = IPPROTO_TCP,
        .szProtocol = L"TCP/IP",
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0xe70f1aa0, 0xab8b, 0x11cf, {0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1002,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = AF_INET,
        .iMaxSockAddr = sizeof(struct sockaddr_in),
        .iMinSockAddr = sizeof(struct sockaddr_in),
        .iSocketType = SOCK_DGRAM,
        .iProtocol = IPPROTO_UDP,
        .dwMessageSize = 0xffbb,
        .szProtocol = L"UDP/IP",
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO | PFL_HIDDEN,
        .ProviderId = {0xe70f1aa0, 0xab8b, 0x11cf, {0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1003,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = AF_INET,
        .iMaxSockAddr = sizeof(struct sockaddr_in),
        .iMinSockAddr = sizeof(struct sockaddr_in),
        .iSocketType = SOCK_RAW,
        .iProtocol = 0,
        .iProtocolMaxOffset = 255,
        .dwMessageSize = 0x8000,
        .szProtocol = L"MSAFD Tcpip [RAW/IP]",
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_EXPEDITED_DATA | XP1_GRACEFUL_CLOSE
                | XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0xf9eab0c0, 0x26d4, 0x11d0, {0xbb, 0xbf, 0x00, 0xaa, 0x00, 0x6c, 0x34, 0xe4}},
        .dwCatalogEntryId = 1004,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = AF_INET6,
        .iMaxSockAddr = sizeof(struct sockaddr_in6),
        .iMinSockAddr = sizeof(struct sockaddr_in6),
        .iSocketType = SOCK_STREAM,
        .iProtocol = IPPROTO_TCP,
        .szProtocol = L"TCP/IPv6",
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0xf9eab0c0, 0x26d4, 0x11d0, {0xbb, 0xbf, 0x00, 0xaa, 0x00, 0x6c, 0x34, 0xe4}},
        .dwCatalogEntryId = 1005,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = AF_INET6,
        .iMaxSockAddr = sizeof(struct sockaddr_in6),
        .iMinSockAddr = sizeof(struct sockaddr_in6),
        .iSocketType = SOCK_DGRAM,
        .iProtocol = IPPROTO_UDP,
        .dwMessageSize = 0xffbb,
        .szProtocol = L"UDP/IPv6",
    },
    {
        .dwServiceFlags1 = XP1_PARTIAL_MESSAGE | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0x11058240, 0xbe47, 0x11cf, {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1030,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = AF_IPX,
        .iMaxSockAddr = sizeof(struct sockaddr),
        .iMinSockAddr = sizeof(struct sockaddr_ipx),
        .iSocketType = SOCK_DGRAM,
        .iProtocol = NSPROTO_IPX,
        .iProtocolMaxOffset = 255,
        .dwMessageSize = 0x240,
        .szProtocol = L"IPX",
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_PSEUDO_STREAM | XP1_MESSAGE_ORIENTED
                | XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0x11058241, 0xbe47, 0x11cf, {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1031,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = AF_IPX,
        .iMaxSockAddr = sizeof(struct sockaddr),
        .iMinSockAddr = sizeof(struct sockaddr_ipx),
        .iSocketType = SOCK_SEQPACKET,
        .iProtocol = NSPROTO_SPX,
        .dwMessageSize = UINT_MAX,
        .szProtocol = L"SPX",
    },
    {
        .dwServiceFlags1 = XP1_IFS_HANDLES | XP1_GRACEFUL_CLOSE | XP1_PSEUDO_STREAM
                | XP1_MESSAGE_ORIENTED | XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY,
        .dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO,
        .ProviderId = {0x11058241, 0xbe47, 0x11cf, {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}},
        .dwCatalogEntryId = 1033,
        .ProtocolChain.ChainLen = 1,
        .iVersion = 2,
        .iAddressFamily = AF_IPX,
        .iMaxSockAddr = sizeof(struct sockaddr),
        .iMinSockAddr = sizeof(struct sockaddr_ipx),
        .iSocketType = SOCK_SEQPACKET,
        .iProtocol = NSPROTO_SPXII,
        .dwMessageSize = UINT_MAX,
        .szProtocol = L"SPX II",
    },
};

DECLARE_CRITICAL_SECTION(cs_socket_list);

static SOCKET *socket_list;
static unsigned int socket_list_size;

const char *debugstr_sockaddr( const struct sockaddr *a )
{
    if (!a) return "(nil)";
    switch (a->sa_family)
    {
    case AF_INET:
    {
        char buf[16];
        const char *p;
        struct sockaddr_in *sin = (struct sockaddr_in *)a;

        p = inet_ntop( AF_INET, &sin->sin_addr, buf, sizeof(buf) );
        if (!p)
            p = "(unknown IPv4 address)";

        return wine_dbg_sprintf("{ family AF_INET, address %s, port %d }",
                                p, ntohs(sin->sin_port));
    }
    case AF_INET6:
    {
        char buf[46];
        const char *p;
        struct sockaddr_in6 *sin = (struct sockaddr_in6 *)a;

        p = inet_ntop( AF_INET6, &sin->sin6_addr, buf, sizeof(buf) );
        if (!p)
            p = "(unknown IPv6 address)";
        return wine_dbg_sprintf("{ family AF_INET6, address %s, flow label %#lx, port %d, scope %lu }",
                                p, sin->sin6_flowinfo, ntohs(sin->sin6_port), sin->sin6_scope_id );
    }
    case AF_IPX:
    {
        int i;
        char netnum[16], nodenum[16];
        struct sockaddr_ipx *sin = (struct sockaddr_ipx *)a;

        for (i = 0;i < 4; i++) sprintf(netnum + i * 2, "%02X", (unsigned char) sin->sa_netnum[i]);
        for (i = 0;i < 6; i++) sprintf(nodenum + i * 2, "%02X", (unsigned char) sin->sa_nodenum[i]);

        return wine_dbg_sprintf("{ family AF_IPX, address %s.%s, ipx socket %d }",
                                netnum, nodenum, sin->sa_socket);
    }
    case AF_IRDA:
    {
        DWORD addr;

        memcpy( &addr, ((const SOCKADDR_IRDA *)a)->irdaDeviceID, sizeof(addr) );
        addr = ntohl( addr );
        return wine_dbg_sprintf("{ family AF_IRDA, addr %08lx, name %s }",
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
        DEBUG_SOCKLEVEL(SOL_SOCKET);
        switch(optname)
        {
            DEBUG_SOCKOPT(SO_ACCEPTCONN);
            DEBUG_SOCKOPT(SO_BROADCAST);
            DEBUG_SOCKOPT(SO_BSP_STATE);
            DEBUG_SOCKOPT(SO_CONDITIONAL_ACCEPT);
            DEBUG_SOCKOPT(SO_CONNECT_TIME);
            DEBUG_SOCKOPT(SO_DEBUG);
            DEBUG_SOCKOPT(SO_DONTLINGER);
            DEBUG_SOCKOPT(SO_DONTROUTE);
            DEBUG_SOCKOPT(SO_ERROR);
            DEBUG_SOCKOPT(SO_EXCLUSIVEADDRUSE);
            DEBUG_SOCKOPT(SO_GROUP_ID);
            DEBUG_SOCKOPT(SO_GROUP_PRIORITY);
            DEBUG_SOCKOPT(SO_KEEPALIVE);
            DEBUG_SOCKOPT(SO_LINGER);
            DEBUG_SOCKOPT(SO_MAX_MSG_SIZE);
            DEBUG_SOCKOPT(SO_OOBINLINE);
            DEBUG_SOCKOPT(SO_OPENTYPE);
            DEBUG_SOCKOPT(SO_PROTOCOL_INFOA);
            DEBUG_SOCKOPT(SO_PROTOCOL_INFOW);
            DEBUG_SOCKOPT(SO_RCVBUF);
            DEBUG_SOCKOPT(SO_RCVTIMEO);
            DEBUG_SOCKOPT(SO_REUSEADDR);
            DEBUG_SOCKOPT(SO_SNDBUF);
            DEBUG_SOCKOPT(SO_SNDTIMEO);
            DEBUG_SOCKOPT(SO_TYPE);
            DEBUG_SOCKOPT(SO_UPDATE_CONNECT_CONTEXT);
        }
        break;

        DEBUG_SOCKLEVEL(NSPROTO_IPX);
        switch(optname)
        {
            DEBUG_SOCKOPT(IPX_PTYPE);
            DEBUG_SOCKOPT(IPX_FILTERPTYPE);
            DEBUG_SOCKOPT(IPX_DSTYPE);
            DEBUG_SOCKOPT(IPX_RECVHDR);
            DEBUG_SOCKOPT(IPX_MAXSIZE);
            DEBUG_SOCKOPT(IPX_ADDRESS);
            DEBUG_SOCKOPT(IPX_MAX_ADAPTER_NUM);
        }
        break;

        DEBUG_SOCKLEVEL(SOL_IRLMP);
        switch(optname)
        {
            DEBUG_SOCKOPT(IRLMP_ENUMDEVICES);
        }
        break;

        DEBUG_SOCKLEVEL(IPPROTO_TCP);
        switch(optname)
        {
            DEBUG_SOCKOPT(TCP_BSDURGENT);
            DEBUG_SOCKOPT(TCP_EXPEDITED_1122);
            DEBUG_SOCKOPT(TCP_NODELAY);
        }
        break;

        DEBUG_SOCKLEVEL(IPPROTO_IP);
        switch(optname)
        {
            DEBUG_SOCKOPT(IP_ADD_MEMBERSHIP);
            DEBUG_SOCKOPT(IP_DONTFRAGMENT);
            DEBUG_SOCKOPT(IP_DROP_MEMBERSHIP);
            DEBUG_SOCKOPT(IP_HDRINCL);
            DEBUG_SOCKOPT(IP_MULTICAST_IF);
            DEBUG_SOCKOPT(IP_MULTICAST_LOOP);
            DEBUG_SOCKOPT(IP_MULTICAST_TTL);
            DEBUG_SOCKOPT(IP_OPTIONS);
            DEBUG_SOCKOPT(IP_PKTINFO);
            DEBUG_SOCKOPT(IP_RECEIVE_BROADCAST);
            DEBUG_SOCKOPT(IP_RECVTOS);
            DEBUG_SOCKOPT(IP_RECVTTL);
            DEBUG_SOCKOPT(IP_TOS);
            DEBUG_SOCKOPT(IP_TTL);
            DEBUG_SOCKOPT(IP_UNICAST_IF);
        }
        break;

        DEBUG_SOCKLEVEL(IPPROTO_IPV6);
        switch(optname)
        {
            DEBUG_SOCKOPT(IPV6_ADD_MEMBERSHIP);
            DEBUG_SOCKOPT(IPV6_DROP_MEMBERSHIP);
            DEBUG_SOCKOPT(IPV6_HOPLIMIT);
            DEBUG_SOCKOPT(IPV6_MULTICAST_IF);
            DEBUG_SOCKOPT(IPV6_MULTICAST_HOPS);
            DEBUG_SOCKOPT(IPV6_MULTICAST_LOOP);
            DEBUG_SOCKOPT(IPV6_PKTINFO);
            DEBUG_SOCKOPT(IPV6_RECVTCLASS);
            DEBUG_SOCKOPT(IPV6_UNICAST_HOPS);
            DEBUG_SOCKOPT(IPV6_V6ONLY);
            DEBUG_SOCKOPT(IPV6_UNICAST_IF);
            DEBUG_SOCKOPT(IPV6_DONTFRAG);
        }
        break;
    }
#undef DEBUG_SOCKLEVEL
#undef DEBUG_SOCKOPT

    if (!strlevel)
        strlevel = wine_dbg_sprintf("0x%x", level);
    if (!stropt)
        stropt = wine_dbg_sprintf("0x%x", optname);

    return wine_dbg_sprintf("level %s, name %s", strlevel, stropt);
}

static inline const char *debugstr_optval(const char *optval, int optlenval)
{
    if (optval && !IS_INTRESOURCE(optval) && optlenval >= 1 && optlenval <= sizeof(DWORD))
    {
        DWORD value = 0;
        memcpy(&value, optval, optlenval);
        return wine_dbg_sprintf("%p (%lu)", optval, value);
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
    if (!(new_array = realloc( socket_list, new_size * sizeof(*socket_list) )))
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

    if (!socket) return FALSE;

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


static BOOL socket_list_remove( SOCKET socket )
{
    unsigned int i;

    if (!socket) return FALSE;

    EnterCriticalSection(&cs_socket_list);
    for (i = 0; i < socket_list_size; ++i)
    {
        if (socket_list[i] == socket)
        {
            socket_list[i] = 0;
            LeaveCriticalSection( &cs_socket_list );
            return TRUE;
        }
    }
    LeaveCriticalSection(&cs_socket_list);
    return FALSE;
}

static INT WINAPI WSA_DefaultBlockingHook( FARPROC x );

int num_startup;
static FARPROC blocking_hook = (FARPROC)WSA_DefaultBlockingHook;

/* function prototypes */
static int ws_protocol_info(SOCKET s, int unicode, WSAPROTOCOL_INFOW *buffer, int *size);

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


struct per_thread_data *get_per_thread_data(void)
{
    struct per_thread_data *data = NtCurrentTeb()->WinSockData;

    if (!data)
    {
        data = calloc( 1, sizeof(*data) );
        NtCurrentTeb()->WinSockData = data;
    }
    return data;
}

static void free_per_thread_data(void)
{
    struct per_thread_data *data = NtCurrentTeb()->WinSockData;

    if (!data) return;

    CloseHandle( data->sync_event );

    /* delete scratch buffers */
    free( data->he_buffer );
    free( data->se_buffer );
    free( data->pe_buffer );

    free( data );
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

static DWORD wait_event_alertable( HANDLE event )
{
    DWORD ret;

    while ((ret = WaitForSingleObjectEx( event, INFINITE, TRUE )) == WAIT_IO_COMPLETION)
        ;
    return ret;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        return !__wine_init_unix_call();

    case DLL_THREAD_DETACH:
        free_per_thread_data();
    }
    return TRUE;
}


/***********************************************************************
 *      WSAStartup		(WS2_32.115)
 */
int WINAPI WSAStartup( WORD version, WSADATA *data )
{
    TRACE( "version %#x\n", version );

    if (data)
    {
        if (!LOBYTE(version) || LOBYTE(version) > 2
                || (LOBYTE(version) == 2 && HIBYTE(version) > 2))
            data->wVersion = MAKEWORD(2, 2);
        else if (LOBYTE(version) == 1 && HIBYTE(version) > 1)
            data->wVersion = MAKEWORD(1, 1);
        else
            data->wVersion = version;
        data->wHighVersion = MAKEWORD(2, 2);
        strcpy( data->szDescription, "WinSock 2.0" );
        strcpy( data->szSystemStatus, "Running" );
        data->iMaxSockets = (LOBYTE(version) == 1 ? 32767 : 0);
        data->iMaxUdpDg = (LOBYTE(version) == 1 ? 65467 : 0);
        /* don't fill lpVendorInfo */
    }

    if (!LOBYTE(version))
        return WSAVERNOTSUPPORTED;

    if (!data) return WSAEFAULT;

    num_startup++;
    TRACE( "increasing startup count to %d\n", num_startup );
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

static INT WS_DuplicateSocket(BOOL unicode, SOCKET s, DWORD dwProcessId, LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    HANDLE hProcess;
    int size;
    WSAPROTOCOL_INFOW infow;

    TRACE( "unicode %d, socket %#Ix, pid %#lx, info %p\n", unicode, s, dwProcessId, lpProtocolInfo );

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
SOCKET WINAPI accept( SOCKET s, struct sockaddr *addr, int *len )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    ULONG accept_handle;
    HANDLE sync_event;
    SOCKET ret;

    TRACE( "socket %#Ix\n", s );

    if (!(sync_event = get_sync_event())) return INVALID_SOCKET;
    status = NtDeviceIoControlFile( (HANDLE)s, sync_event, NULL, NULL, &io, IOCTL_AFD_WINE_ACCEPT,
                                    NULL, 0, &accept_handle, sizeof(accept_handle) );
    if (status == STATUS_PENDING)
    {
        if (wait_event_alertable( sync_event ) == WAIT_FAILED)
            return SOCKET_ERROR;
        status = io.Status;
    }
    if (status)
    {
        TRACE( "failed, status %#lx\n", status );
        WSASetLastError( NtStatusToWSAError( status ) );
        return INVALID_SOCKET;
    }

    ret = accept_handle;
    if (!socket_list_add( ret ))
    {
        CloseHandle( SOCKET2HANDLE(ret) );
        return INVALID_SOCKET;
    }
    if (addr && len && getpeername( ret, addr, len ))
    {
        closesocket( ret );
        return INVALID_SOCKET;
    }

    TRACE( "returning %#Ix\n", ret );
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

    TRACE( "listener %#Ix, acceptor %#Ix, dest %p, recv_len %lu, local_len %lu, remote_len %lu, ret_len %p, "
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
    TRACE( "status %#lx.\n", status );
    return !status;
}


static BOOL WINAPI WS2_TransmitFile( SOCKET s, HANDLE file, DWORD file_len, DWORD buffer_size,
                                     OVERLAPPED *overlapped, TRANSMIT_FILE_BUFFERS *buffers, DWORD flags )
{
    struct afd_transmit_params params = {{{0}}};
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#Ix, file %p, file_len %lu, buffer_size %lu, overlapped %p, buffers %p, flags %#lx\n",
           s, file, file_len, buffer_size, overlapped, buffers, flags );

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
        overlapped->Internal = STATUS_PENDING;
        overlapped->InternalHigh = 0;
        params.offset.LowPart = overlapped->Offset;
        params.offset.HighPart = overlapped->OffsetHigh;
    }
    else
    {
        if (!(event = get_sync_event())) return -1;
        params.offset.QuadPart = FILE_USE_FILE_POINTER_POSITION;
    }

    params.file = HandleToULong( file );
    params.file_len = file_len;
    params.buffer_size = buffer_size;
    if (buffers)
    {
        params.head_ptr = u64_from_user_ptr(buffers->Head);
        params.head_len = buffers->HeadLength;
        params.tail_ptr = u64_from_user_ptr(buffers->Tail);
        params.tail_len = buffers->TailLength;
    }
    params.flags = flags;

    status = NtDeviceIoControlFile( (HANDLE)s, event, NULL, cvalue, piosb,
                                    IOCTL_AFD_WINE_TRANSMIT, &params, sizeof(params), NULL, 0 );
    if (status == STATUS_PENDING && !overlapped)
    {
        if (WaitForSingleObject( event, INFINITE ) == WAIT_FAILED)
            return FALSE;
        status = piosb->Status;
    }
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return !status;
}


/***********************************************************************
 *     GetAcceptExSockaddrs
 */
static void WINAPI WS2_GetAcceptExSockaddrs( void *buffer, DWORD data_size, DWORD local_size, DWORD remote_size,
                                             struct sockaddr **local_addr, LPINT local_addr_len,
                                             struct sockaddr **remote_addr, LPINT remote_addr_len )
{
    char *cbuf = buffer;

    TRACE( "buffer %p, data_size %lu, local_size %lu, remote_size %lu,"
           " local_addr %p, local_addr_len %p, remote_addr %p, remote_addr_len %p\n",
           buffer, data_size, local_size, remote_size,
           local_addr, local_addr_len, remote_addr, remote_addr_len );

    cbuf += data_size;

    *local_addr_len = *(int *) cbuf;
    *local_addr = (struct sockaddr *)(cbuf + sizeof(int));

    cbuf += local_size;

    *remote_addr_len = *(int *) cbuf;
    *remote_addr = (struct sockaddr *)(cbuf + sizeof(int));
}


static void WINAPI socket_apc( void *apc_user, IO_STATUS_BLOCK *io, ULONG reserved )
{
    LPWSAOVERLAPPED_COMPLETION_ROUTINE func = apc_user;
    func( NtStatusToWSAError( io->Status ), io->Information, (OVERLAPPED *)io, 0 );
}

static int WS2_recv_base( SOCKET s, WSABUF *buffers, DWORD buffer_count, DWORD *ret_size, DWORD *flags,
                          struct sockaddr *addr, int *addr_len, OVERLAPPED *overlapped,
                          LPWSAOVERLAPPED_COMPLETION_ROUTINE completion, WSABUF *control )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    struct afd_recvmsg_params params;
    PIO_APC_ROUTINE apc = NULL;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#Ix, buffers %p, buffer_count %lu, flags %#lx, addr %p, "
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
    piosb->Status = STATUS_PENDING;

    if (completion)
    {
        event = NULL;
        cvalue = completion;
        apc = socket_apc;
    }

    params.control_ptr = u64_from_user_ptr(control);
    params.addr_ptr = u64_from_user_ptr(addr);
    params.addr_len_ptr = u64_from_user_ptr(addr_len);
    params.ws_flags_ptr = u64_from_user_ptr(flags);
    params.force_async = !!overlapped;
    params.count = buffer_count;
    params.buffers_ptr = u64_from_user_ptr(buffers);

    status = NtDeviceIoControlFile( (HANDLE)s, event, apc, cvalue, piosb,
                                    IOCTL_AFD_WINE_RECVMSG, &params, sizeof(params), NULL, 0 );
    if (status == STATUS_PENDING && !overlapped)
    {
        if (wait_event_alertable( event ) == WAIT_FAILED)
            return -1;
        status = piosb->Status;
    }
    if (!status && ret_size) *ret_size = piosb->Information;
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return status ? -1 : 0;
}

static int WS2_sendto( SOCKET s, WSABUF *buffers, DWORD buffer_count, DWORD *ret_size, DWORD flags,
                       const struct sockaddr *addr, int addr_len, OVERLAPPED *overlapped,
                       LPWSAOVERLAPPED_COMPLETION_ROUTINE completion )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    struct afd_sendmsg_params params;
    PIO_APC_ROUTINE apc = NULL;
    HANDLE event = NULL;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#Ix, buffers %p, buffer_count %lu, flags %#lx, addr %p, "
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
    piosb->Status = STATUS_PENDING;

    if (completion)
    {
        event = NULL;
        cvalue = completion;
        apc = socket_apc;
    }

    params.addr_ptr = u64_from_user_ptr( addr );
    params.addr_len = addr_len;
    params.ws_flags = flags;
    params.force_async = !!overlapped;
    params.count = buffer_count;
    params.buffers_ptr = u64_from_user_ptr( buffers );

    status = NtDeviceIoControlFile( (HANDLE)s, event, apc, cvalue, piosb,
                                    IOCTL_AFD_WINE_SENDMSG, &params, sizeof(params), NULL, 0 );
    if (status == STATUS_PENDING && !overlapped)
    {
        if (WaitForSingleObject( event, INFINITE ) == WAIT_FAILED)
            return -1;
        status = piosb->Status;
    }
    if (!status && ret_size) *ret_size = piosb->Information;
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
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
int WINAPI bind( SOCKET s, const struct sockaddr *addr, int len )
{
    struct afd_bind_params *params;
    struct sockaddr *ret_addr;
    IO_STATUS_BLOCK io;
    HANDLE sync_event;
    NTSTATUS status;

    TRACE( "socket %#Ix, addr %s\n", s, debugstr_sockaddr(addr) );

    if (!addr)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    switch (addr->sa_family)
    {
        case AF_INET:
            if (len < sizeof(struct sockaddr_in))
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            break;

        case AF_INET6:
            if (len < sizeof(struct sockaddr_in6))
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            break;

        case AF_IPX:
            if (len < sizeof(struct sockaddr_ipx))
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            break;

        case AF_IRDA:
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

    params = malloc( sizeof(int) + len );
    ret_addr = malloc( len );
    if (!params || !ret_addr)
    {
        free( params );
        free( ret_addr );
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
        status = io.Status;
    }

    if (status)  TRACE( "failed, status %#lx.\n", status );
    else         TRACE( "successfully bound to address %s\n", debugstr_sockaddr( ret_addr ));

    free( params );
    free( ret_addr );

    SetLastError( NtStatusToWSAError( status ) );
    return status ? -1 : 0;
}


/***********************************************************************
 *      closesocket   (ws2_32.3)
 */
int WINAPI closesocket( SOCKET s )
{
    TRACE( "%#Ix\n", s );

    if (!num_startup)
    {
        SetLastError( WSANOTINITIALISED );
        return -1;
    }

    if (!socket_list_remove( s ))
    {
        SetLastError( WSAENOTSOCK );
        return -1;
    }

    CloseHandle( (HANDLE)s );
    return 0;
}


/***********************************************************************
 *      connect   (ws2_32.4)
 */
int WINAPI connect( SOCKET s, const struct sockaddr *addr, int len )
{
    struct afd_connect_params *params;
    IO_STATUS_BLOCK io;
    HANDLE sync_event;
    NTSTATUS status;

    TRACE( "socket %#Ix, addr %s, len %d\n", s, debugstr_sockaddr(addr), len );

    if (!(sync_event = get_sync_event())) return -1;

    if (!(params = malloc( sizeof(*params) + len )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return -1;
    }
    params->addr_len = len;
    params->synchronous = TRUE;
    memcpy( params + 1, addr, len );

    status = NtDeviceIoControlFile( (HANDLE)s, sync_event, NULL, NULL, &io, IOCTL_AFD_WINE_CONNECT,
                                    params, sizeof(*params) + len, NULL, 0 );
    free( params );
    if (status == STATUS_PENDING)
    {
        if (wait_event_alertable( sync_event ) == WAIT_FAILED) return -1;
        status = io.Status;
    }
    if (status)
    {
        /* NtStatusToWSAError() has no mapping for WSAEALREADY */
        SetLastError( status == STATUS_ADDRESS_ALREADY_ASSOCIATED ? WSAEALREADY : NtStatusToWSAError( status ) );
        TRACE( "failed, status %#lx.\n", status );
        return -1;
    }
    return 0;
}


/***********************************************************************
 *              WSAConnect             (WS2_32.30)
 */
int WINAPI WSAConnect( SOCKET s, const struct sockaddr *name, int namelen,
                       LPWSABUF lpCallerData, LPWSABUF lpCalleeData,
                       LPQOS lpSQOS, LPQOS lpGQOS )
{
    if ( lpCallerData || lpCalleeData || lpSQOS || lpGQOS )
        FIXME("unsupported parameters!\n");
    return connect( s, name, namelen );
}


static BOOL WINAPI WS2_ConnectEx( SOCKET s, const struct sockaddr *name, int namelen,
                                  void *send_buffer, DWORD send_len, DWORD *ret_len, OVERLAPPED *overlapped )
{
    struct afd_connect_params *params;
    void *cvalue = NULL;
    NTSTATUS status;

    TRACE( "socket %#Ix, ptr %p %s, length %d, send_buffer %p, send_len %lu, overlapped %p\n",
           s, name, debugstr_sockaddr(name), namelen, send_buffer, send_len, overlapped );

    if (!overlapped)
    {
        SetLastError( WSA_INVALID_PARAMETER );
        return FALSE;
    }

    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    overlapped->Internal = STATUS_PENDING;
    overlapped->InternalHigh = 0;

    if (!(params = malloc( sizeof(*params) + namelen + send_len )))
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
    free( params );
    if (ret_len) *ret_len = overlapped->InternalHigh;
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return !status;
}


/***********************************************************************
 *          WSAConnectByNameA      (WS2_32.@)
 */
BOOL WINAPI WSAConnectByNameA(SOCKET s, const char *node_name, const char *service_name,
                              DWORD *local_addr_len, struct sockaddr *local_addr,
                              DWORD *remote_addr_len, struct sockaddr *remote_addr,
                              const struct timeval *timeout, WSAOVERLAPPED *reserved)
{
    WSAPROTOCOL_INFOA proto_info;
    WSAPOLLFD pollout;
    struct addrinfo *service, hints;
    int ret, proto_len, sockaddr_size, sockname_size, sock_err, int_len;

    TRACE("socket %#Ix, node_name %s, service_name %s, local_addr_len %p, local_addr %p, \
          remote_addr_len %p, remote_addr %p, timeout %p, reserved %p\n",
          s, debugstr_a(node_name), debugstr_a(service_name), local_addr_len, local_addr,
          remote_addr_len, remote_addr, timeout, reserved );

    if (!node_name || !service_name || reserved)
    {
        SetLastError(WSAEINVAL);
        return FALSE;
    }

    if (!s)
    {
        SetLastError(WSAENOTSOCK);
        return FALSE;
    }

    if (timeout)
        FIXME("WSAConnectByName timeout stub\n");

    proto_len = sizeof(WSAPROTOCOL_INFOA);
    ret = getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *)&proto_info, &proto_len);
    if (ret)
        return FALSE;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = proto_info.iSocketType;
    hints.ai_family = proto_info.iAddressFamily;
    hints.ai_protocol = proto_info.iProtocol;
    ret = getaddrinfo(node_name, service_name, &hints, &service);
    if (ret)
        return FALSE;

    if (proto_info.iSocketType != SOCK_STREAM)
    {
        freeaddrinfo(service);
        SetLastError(WSAEFAULT);
        return FALSE;
    }

    switch (proto_info.iAddressFamily)
    {
    case AF_INET:
        sockaddr_size = sizeof(SOCKADDR_IN);
        break;
    case AF_INET6:
        sockaddr_size = sizeof(SOCKADDR_IN6);
        break;
    default:
        freeaddrinfo(service);
        SetLastError(WSAENOTSOCK);
        return FALSE;
    }

    ret = connect(s, service->ai_addr, sockaddr_size);
    if (ret)
    {
        freeaddrinfo(service);
        return FALSE;
    }

    pollout.fd = s;
    pollout.events = POLLWRNORM;
    ret = WSAPoll(&pollout, 1, -1);
    if (ret == SOCKET_ERROR)
    {
        freeaddrinfo(service);
        return FALSE;
    }
    if (pollout.revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        freeaddrinfo(service);
        int_len = sizeof(int);
        ret = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&sock_err, &int_len);
        if (ret == SOCKET_ERROR)
            return FALSE;
        SetLastError(sock_err);
        return FALSE;
    }

    if (remote_addr_len && remote_addr)
    {
        if (*remote_addr_len >= sockaddr_size)
        {
            memcpy(remote_addr, service->ai_addr, sockaddr_size);
            *remote_addr_len = sockaddr_size;
        }
        else
        {
            freeaddrinfo(service);
            SetLastError(WSAEFAULT);
            return FALSE;
        }
    }

    freeaddrinfo(service);

    if (local_addr_len && local_addr)
    {
        if (*local_addr_len >= sockaddr_size)
        {
            sockname_size = sockaddr_size;
            ret = getsockname(s, local_addr, &sockname_size);
            if (ret)
                return FALSE;
            if (proto_info.iAddressFamily == AF_INET6)
                ((SOCKADDR_IN6 *)local_addr)->sin6_port = 0;
            else
                ((SOCKADDR_IN *)local_addr)->sin_port = 0;
            *local_addr_len = sockaddr_size;
        }
        else
        {
            SetLastError(WSAEFAULT);
            return FALSE;
        }
    }

    return TRUE;
}


/***********************************************************************
 *          WSAConnectByNameW      (WS2_32.@)
 */
BOOL WINAPI WSAConnectByNameW(SOCKET s, const WCHAR *node_name, const WCHAR *service_name,
                              DWORD *local_addr_len, struct sockaddr *local_addr,
                              DWORD *remote_addr_len, struct sockaddr *remote_addr,
                              const struct timeval *timeout, WSAOVERLAPPED *reserved)
{
    char *node_nameA, *service_nameA;
    BOOL ret;

    if (!node_name || !service_name)
    {
        SetLastError(WSAEINVAL);
        return FALSE;
    }

    node_nameA = strdupWtoA(node_name);
    service_nameA = strdupWtoA(service_name);
    if (!node_nameA || !service_nameA)
    {
        SetLastError(WSAENOBUFS);
        return FALSE;
    }

    ret = WSAConnectByNameA(s, node_nameA, service_nameA, local_addr_len, local_addr,
                             remote_addr_len, remote_addr, timeout, reserved);
    free(node_nameA);
    free(service_nameA);
    return ret;
}


static BOOL WINAPI WS2_DisconnectEx( SOCKET s, OVERLAPPED *overlapped, DWORD flags, DWORD reserved )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    void *cvalue = NULL;
    int how = SD_SEND;
    HANDLE event = 0;
    NTSTATUS status;

    TRACE( "socket %#Ix, overlapped %p, flags %#lx, reserved %#lx\n", s, overlapped, flags, reserved );

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
    TRACE( "status %#lx.\n", status );
    return !status;
}


/***********************************************************************
 *      getpeername   (ws2_32.5)
 */
int WINAPI getpeername( SOCKET s, struct sockaddr *addr, int *len )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    int safe_len = 0;

    TRACE( "socket %#Ix, addr %p, len %d\n", s, addr, len ? *len : 0 );

    if (!socket_list_find( s ))
    {
        WSASetLastError( WSAENOTSOCK );
        return -1;
    }

    /* Windows checks the validity of the socket before checking len, so
     * let wineserver do the same. Since len being NULL and *len being 0
     * yield the same error, we can substitute in 0 if len is NULL. */
    if (len)
        safe_len = *len;

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io,
                                    IOCTL_AFD_WINE_GETPEERNAME, NULL, 0, addr, safe_len );
    if (!status)
        *len = io.Information;
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return status ? -1 : 0;
}


/***********************************************************************
 *      getsockname   (ws2_32.6)
 */
int WINAPI getsockname( SOCKET s, struct sockaddr *addr, int *len )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#Ix, addr %p, len %d\n", s, addr, len ? *len : 0 );

    if (!addr)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_GETSOCKNAME, NULL, 0, addr, *len );
    if (!status)
        *len = io.Information;
    WSASetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return status ? -1 : 0;
}


static int server_getsockopt( SOCKET s, ULONG code, char *optval, int *optlen )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, code, NULL, 0, optval, *optlen );
    if (!status) *optlen = io.Information;
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return status ? -1 : 0;
}


/***********************************************************************
 *		getsockopt		(WS2_32.7)
 */
int WINAPI getsockopt( SOCKET s, int level, int optname, char *optval, int *optlen )
{
    INT ret = 0;

    TRACE( "socket %#Ix, %s, optval %p, optlen %p (%d)\n",
           s, debugstr_sockopt(level, optname), optval, optlen, optlen ? *optlen : 0 );

    if ((level != SOL_SOCKET || optname != SO_OPENTYPE))
    {
        if (!socket_list_find( s ))
        {
            SetLastError( WSAENOTSOCK );
            return SOCKET_ERROR;
        }
        if (!optlen || *optlen <= 0)
        {
            SetLastError( WSAEFAULT );
            return -1;
        }
    }

    switch(level)
    {
    case SOL_SOCKET:
    {
        switch(optname)
        {
        case SO_ACCEPTCONN:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_ACCEPTCONN, optval, optlen );

        case SO_BROADCAST:
            ret = server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_BROADCAST, optval, optlen );
            if (!ret && *optlen < sizeof(DWORD)) *optlen = 1;
            return ret;

        case SO_BSP_STATE:
        {
            CSADDR_INFO *csinfo = (CSADDR_INFO *)optval;
            WSAPROTOCOL_INFOW infow;
            int addr_size;

            if (!ws_protocol_info( s, TRUE, &infow, &addr_size ))
                return -1;

            if (infow.iAddressFamily == AF_INET)
                addr_size = sizeof(struct sockaddr_in);
            else if (infow.iAddressFamily == AF_INET6)
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

            csinfo->LocalAddr.lpSockaddr = (struct sockaddr *)(csinfo + 1);
            csinfo->RemoteAddr.lpSockaddr = (struct sockaddr *)((char *)(csinfo + 1) + addr_size);

            csinfo->LocalAddr.iSockaddrLength = addr_size;
            if (getsockname( s, csinfo->LocalAddr.lpSockaddr, &csinfo->LocalAddr.iSockaddrLength ) < 0)
            {
                csinfo->LocalAddr.lpSockaddr = NULL;
                csinfo->LocalAddr.iSockaddrLength = 0;
            }

            csinfo->RemoteAddr.iSockaddrLength = addr_size;
            if (getpeername( s, csinfo->RemoteAddr.lpSockaddr, &csinfo->RemoteAddr.iSockaddrLength ) < 0)
            {
                csinfo->RemoteAddr.lpSockaddr = NULL;
                csinfo->RemoteAddr.iSockaddrLength = 0;
            }

            csinfo->iSocketType = infow.iSocketType;
            csinfo->iProtocol = infow.iProtocol;
            return 0;
        }

        case SO_CONNECT_TIME:
            if (!optval)
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_CONNECT_TIME, optval, optlen );

        case SO_DEBUG:
            WARN( "returning 0 for SO_DEBUG\n" );
            *(DWORD *)optval = 0;
            SetLastError( 0 );
            return 0;

        case SO_DONTLINGER:
        {
            LINGER linger;
            int len = sizeof(linger);
            BOOL value;
            int ret;

            if (*optlen < 1 || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }

            if (!(ret = getsockopt( s, SOL_SOCKET, SO_LINGER, (char *)&linger, &len )))
            {
                value = !linger.l_onoff;
                memcpy( optval, &value, min( sizeof(BOOL), *optlen ));
                *optlen = *optlen >= sizeof(BOOL) ? sizeof(BOOL) : 1;
            }
            return ret;
        }

        /* As mentioned in setsockopt, Windows ignores this, so we
         * always return true here */
        case SO_DONTROUTE:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *optval = TRUE;
            *optlen = 1;
            SetLastError( ERROR_SUCCESS );
            return 0;

        case SO_ERROR:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_ERROR, optval, optlen );

        case SO_KEEPALIVE:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *optlen = 1;
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_KEEPALIVE, optval, optlen );

        case SO_LINGER:
        {
            WSAPROTOCOL_INFOW info;
            int size;

            /* struct linger and LINGER have different sizes */
            if (*optlen < sizeof(LINGER) || !optval)
            {
                if (optval) memset( optval, 0, *optlen );
                SetLastError( WSAEFAULT );
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

        case SO_MAX_MSG_SIZE:
            if (*optlen < sizeof(int) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            TRACE("getting global SO_MAX_MSG_SIZE = 65507\n");
            *(int *)optval = 65507;
            *optlen = sizeof(int);
            return 0;

        case SO_OOBINLINE:
            ret = server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_OOBINLINE, optval, optlen );
            if (!ret && *optlen < sizeof(DWORD)) *optlen = 1;
            return ret;

        /* SO_OPENTYPE does not require a valid socket handle. */
        case SO_OPENTYPE:
            if (!optlen || *optlen < sizeof(int) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *(int *)optval = get_per_thread_data()->opentype;
            *optlen = sizeof(int);
            TRACE("getting global SO_OPENTYPE = 0x%x\n", *((int*)optval) );
            SetLastError( ERROR_SUCCESS );
            return 0;

        case SO_PROTOCOL_INFOA:
        case SO_PROTOCOL_INFOW:
        {
            int size;
            WSAPROTOCOL_INFOW infow;

            ret = ws_protocol_info(s, optname == SO_PROTOCOL_INFOW, &infow, &size);
            if (ret)
            {
                if (!optval || *optlen < size)
                {
                    *optlen = size;
                    ret = 0;
                    SetLastError(WSAEFAULT);
                }
                else
                    memcpy(optval, &infow, size);
            }
            return ret ? 0 : SOCKET_ERROR;
        }

        case SO_RCVBUF:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                if (optval) memset( optval, 0, *optlen );
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_RCVBUF, optval, optlen );

        case SO_RCVTIMEO:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_RCVTIMEO, optval, optlen );

        case SO_REUSEADDR:
            ret = server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_REUSEADDR, optval, optlen );
            if (!ret && *optlen < sizeof(DWORD)) *optlen = 1;
            return ret;

        case SO_EXCLUSIVEADDRUSE:
            ret = server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_EXCLUSIVEADDRUSE, optval, optlen );
            if (!ret && *optlen < sizeof(DWORD)) *optlen = 1;
            return ret;

        case SO_SNDBUF:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                if (optval) memset( optval, 0, *optlen );
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_SNDBUF, optval, optlen );

        case SO_SNDTIMEO:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_SO_SNDTIMEO, optval, optlen );

        case SO_TYPE:
        {
            WSAPROTOCOL_INFOW info;
            int size;

            if (*optlen < sizeof(int) || !optval)
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
    }/* end case SOL_SOCKET */

    case NSPROTO_IPX:
    {
        struct sockaddr_ipx addr;
        IPX_ADDRESS_DATA *data;
        int namelen;
        switch(optname)
        {
        case IPX_ADDRESS:
            /*
            *  On a Win2000 system with one network card there are usually
            *  three ipx devices one with a speed of 28.8kbps, 10Mbps and 100Mbps.
            *  Using this call you can then retrieve info about this all.
            *  In case of Linux it is a bit different. Usually you have
            *  only "one" device active and further it is not possible to
            *  query things like the linkspeed.
            */
            FIXME("IPX_ADDRESS\n");
            namelen = sizeof(struct sockaddr_ipx);
            memset( &addr, 0, sizeof(struct sockaddr_ipx) );
            getsockname( s, (struct sockaddr *)&addr, &namelen );

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

        case IPX_MAX_ADAPTER_NUM:
            FIXME("IPX_MAX_ADAPTER_NUM\n");
            *(int*)optval = 1; /* As noted under IPX_ADDRESS we have just one card. */
            return 0;

        case IPX_PTYPE:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPX_PTYPE, optval, optlen );

        default:
            FIXME("IPX optname:%x\n", optname);
            return SOCKET_ERROR;
        }/* end switch(optname) */
    } /* end case NSPROTO_IPX */

    case SOL_IRLMP:
        switch(optname)
        {
        case IRLMP_ENUMDEVICES:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IRLMP_ENUMDEVICES, optval, optlen );

        default:
            FIXME("IrDA optname:0x%x\n", optname);
            return SOCKET_ERROR;
        }
        break; /* case SOL_IRLMP */

    /* Levels IPPROTO_TCP and IPPROTO_IP convert directly */
    case IPPROTO_TCP:
        switch(optname)
        {
        case TCP_NODELAY:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *optlen = 1;
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_TCP_NODELAY, optval, optlen );

        case TCP_KEEPALIVE:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *optlen = sizeof(DWORD);
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_TCP_KEEPALIVE, optval, optlen );

        case TCP_KEEPCNT:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *optlen = sizeof(DWORD);
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_TCP_KEEPCNT, optval, optlen );

        case TCP_KEEPINTVL:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *optlen = sizeof(DWORD);
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_TCP_KEEPINTVL, optval, optlen );

        default:
            FIXME( "unrecognized TCP option %#x\n", optname );
            SetLastError( WSAENOPROTOOPT );
            return -1;
        }

    case IPPROTO_IP:
        switch(optname)
        {
        case IP_DONTFRAGMENT:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_DONTFRAGMENT, optval, optlen );

        case IP_HDRINCL:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_HDRINCL, optval, optlen );

        case IP_MULTICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_MULTICAST_IF, optval, optlen );

        case IP_MULTICAST_LOOP:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_MULTICAST_LOOP, optval, optlen );

        case IP_MULTICAST_TTL:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_MULTICAST_TTL, optval, optlen );

        case IP_OPTIONS:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_OPTIONS, optval, optlen );

        case IP_PKTINFO:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_PKTINFO, optval, optlen );

        case IP_RECVTOS:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_RECVTOS, optval, optlen );

        case IP_RECVTTL:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_RECVTTL, optval, optlen );

        case IP_TOS:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_TOS, optval, optlen );

        case IP_TTL:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_TTL, optval, optlen );

        case IP_UNICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IP_UNICAST_IF, optval, optlen );

        default:
            FIXME( "unrecognized IP option %u\n", optname );
            /* fall through */

        case IP_ADD_MEMBERSHIP:
        case IP_DROP_MEMBERSHIP:
            SetLastError( WSAENOPROTOOPT );
            return -1;
        }

    case IPPROTO_IPV6:
        switch(optname)
        {
        case IPV6_DONTFRAG:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            if (*optlen < sizeof(DWORD)) *optlen = 1;
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_DONTFRAG, optval, optlen );

        case IPV6_HOPLIMIT:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_RECVHOPLIMIT, optval, optlen );

        case IPV6_MULTICAST_HOPS:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            if (*optlen < sizeof(DWORD)) *optlen = 1;
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_MULTICAST_HOPS, optval, optlen );

        case IPV6_MULTICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_MULTICAST_IF, optval, optlen );

        case IPV6_MULTICAST_LOOP:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            if (*optlen < sizeof(DWORD)) *optlen = 1;
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_MULTICAST_LOOP, optval, optlen );

        case IPV6_PROTECTION_LEVEL:
            if (*optlen < sizeof(UINT) || !optval)
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            *optlen = sizeof(UINT);
            *optval = PROTECTION_LEVEL_UNRESTRICTED;
            FIXME("IPV6_PROTECTION_LEVEL is ignored!\n");
            return 0;

        case IPV6_PKTINFO:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_RECVPKTINFO, optval, optlen );

        case IPV6_RECVTCLASS:
            if (*optlen < sizeof(DWORD) || !optval)
            {
                if (optlen) *optlen = 0;
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_RECVTCLASS, optval, optlen );

        case IPV6_UNICAST_HOPS:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            if (*optlen < sizeof(DWORD)) *optlen = 1;
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_UNICAST_HOPS, optval, optlen );

        case IPV6_UNICAST_IF:
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_UNICAST_IF, optval, optlen );

        case IPV6_V6ONLY:
            if (*optlen < 1 || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            *optlen = 1;
            return server_getsockopt( s, IOCTL_AFD_WINE_GET_IPV6_V6ONLY, optval, optlen );

        default:
            FIXME( "unrecognized IPv6 option %u\n", optname );
            /* fall through */

        case IPV6_ADD_MEMBERSHIP:
        case IPV6_DROP_MEMBERSHIP:
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
    const char *buf_type, *family;

#define IOCTL_NAME(x) case x: return #x
    switch (code)
    {
        IOCTL_NAME(FIONBIO);
        IOCTL_NAME(FIONREAD);
        IOCTL_NAME(SIOCATMARK);
        /* IOCTL_NAME(SIO_ACQUIRE_PORT_RESERVATION); */
        IOCTL_NAME(SIO_ADDRESS_LIST_CHANGE);
        IOCTL_NAME(SIO_ADDRESS_LIST_QUERY);
        IOCTL_NAME(SIO_ASSOCIATE_HANDLE);
        /* IOCTL_NAME(SIO_ASSOCIATE_PORT_RESERVATION);
        IOCTL_NAME(SIO_BASE_HANDLE);
        IOCTL_NAME(SIO_BSP_HANDLE);
        IOCTL_NAME(SIO_BSP_HANDLE_SELECT);
        IOCTL_NAME(SIO_BSP_HANDLE_POLL);
        IOCTL_NAME(SIO_CHK_QOS); */
        IOCTL_NAME(SIO_ENABLE_CIRCULAR_QUEUEING);
        IOCTL_NAME(SIO_FIND_ROUTE);
        IOCTL_NAME(SIO_FLUSH);
        IOCTL_NAME(SIO_GET_BROADCAST_ADDRESS);
        IOCTL_NAME(SIO_GET_EXTENSION_FUNCTION_POINTER);
        IOCTL_NAME(SIO_GET_GROUP_QOS);
        IOCTL_NAME(SIO_GET_INTERFACE_LIST);
        /* IOCTL_NAME(SIO_GET_INTERFACE_LIST_EX); */
        IOCTL_NAME(SIO_GET_QOS);
        IOCTL_NAME(SIO_IDEAL_SEND_BACKLOG_CHANGE);
        IOCTL_NAME(SIO_IDEAL_SEND_BACKLOG_QUERY);
        IOCTL_NAME(SIO_KEEPALIVE_VALS);
        IOCTL_NAME(SIO_MULTIPOINT_LOOPBACK);
        IOCTL_NAME(SIO_MULTICAST_SCOPE);
        /* IOCTL_NAME(SIO_QUERY_RSS_SCALABILITY_INFO);
        IOCTL_NAME(SIO_QUERY_WFP_ALE_ENDPOINT_HANDLE); */
        IOCTL_NAME(SIO_RCVALL);
        IOCTL_NAME(SIO_RCVALL_IGMPMCAST);
        IOCTL_NAME(SIO_RCVALL_MCAST);
        /* IOCTL_NAME(SIO_RELEASE_PORT_RESERVATION); */
        IOCTL_NAME(SIO_ROUTING_INTERFACE_CHANGE);
        IOCTL_NAME(SIO_ROUTING_INTERFACE_QUERY);
        IOCTL_NAME(SIO_SET_COMPATIBILITY_MODE);
        IOCTL_NAME(SIO_SET_GROUP_QOS);
        IOCTL_NAME(SIO_SET_QOS);
        IOCTL_NAME(SIO_TRANSLATE_HANDLE);
        IOCTL_NAME(SIO_UDP_CONNRESET);
    }
#undef IOCTL_NAME

    /* If this is not a known code split its bits */
    switch(code & 0x18000000)
    {
    case IOC_WS2:
        family = "IOC_WS2";
        break;
    case IOC_PROTOCOL:
        family = "IOC_PROTOCOL";
        break;
    case IOC_VENDOR:
        family = "IOC_VENDOR";
        break;
    default: /* IOC_UNIX */
    {
        BYTE size = (code >> 16) & IOCPARM_MASK;
        char x = (code & 0xff00) >> 8;
        BYTE y = code & 0xff;
        char args[14];

        switch (code & (IOC_VOID | IOC_INOUT))
        {
            case IOC_VOID:
                buf_type = "_IO";
                sprintf(args, "%d, %d", x, y);
                break;
            case IOC_IN:
                buf_type = "_IOW";
                sprintf(args, "'%c', %d, %d", x, y, size);
                break;
            case IOC_OUT:
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

    /* We are different from IOC_UNIX. */
    switch (code & (IOC_VOID | IOC_INOUT))
    {
        case IOC_VOID:
            buf_type = "_WSAIO";
            break;
        case IOC_INOUT:
            buf_type = "_WSAIORW";
            break;
        case IOC_IN:
            buf_type = "_WSAIOW";
            break;
        case IOC_OUT:
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
        status = piosb->Status;
    }
    if (status == STATUS_NOT_SUPPORTED)
    {
        FIXME("Unsupported ioctl %#lx (device=%#lx access=%#lx func=%#lx method=%#lx)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
    }
    else if (status == STATUS_SUCCESS)
        *ret_size = piosb->Information;

    TRACE( "status %#lx.\n", status );
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
    TRACE( "socket %#Ix, %s, in_buff %p, in_size %#lx, out_buff %p,"
           " out_size %#lx, ret_size %p, overlapped %p, completion %p\n",
           s, debugstr_wsaioctl(code), in_buff, in_size, out_buff,
           out_size, ret_size, overlapped, completion );

    if (!ret_size)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    switch (code)
    {
    case FIONBIO:
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

    case FIONREAD:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_FIONREAD, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        if (!ret) *ret_size = sizeof(u_long);
        return ret ? -1 : 0;
    }

    case SIO_IDEAL_SEND_BACKLOG_QUERY:
    {
        DWORD ret;
        WSAPROTOCOL_INFOA proto_info;
        int proto_len, len;
        struct sockaddr addr;

        if (!out_buff || out_size < sizeof(DWORD))
        {
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        proto_len = sizeof(WSAPROTOCOL_INFOA);
        ret = getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *)&proto_info, &proto_len);
        if (ret == SOCKET_ERROR)
            return SOCKET_ERROR;

        if (proto_info.iSocketType != SOCK_STREAM)
        {
            SetLastError( WSAEOPNOTSUPP );
            return SOCKET_ERROR;
        }

        len = sizeof(addr);
        if (getpeername( s, &addr, &len ) == SOCKET_ERROR)
        {
            return SOCKET_ERROR;
        }

        *(DWORD*)out_buff = 0x10000; /* 64k */

        WARN("SIO_IDEAL_SEND_BACKLOG_QUERY Always returning 64k\n");

        SetLastError( ret );
        *ret_size = sizeof(DWORD);
        return 0;
    }

    case SIOCATMARK:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_SIOCATMARK, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        if (!ret) *ret_size = sizeof(u_long);
        return ret ? -1 : 0;
    }

    case SIO_GET_INTERFACE_LIST:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_GET_INTERFACE_LIST, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        if (ret && ret != ERROR_IO_PENDING) *ret_size = 0;
        return ret ? -1 : 0;
    }

    case SIO_ADDRESS_LIST_QUERY:
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
            IP_ADAPTER_INFO *p, *table = malloc( size );
            NTSTATUS status = STATUS_SUCCESS;
            SOCKET_ADDRESS_LIST *sa_list;
            SOCKADDR_IN *sockaddr;
            SOCKET_ADDRESS *sa;
            unsigned int i;
            DWORD ret = 0;
            DWORD num;

            if (!table || GetAdaptersInfo(table, &size))
            {
                free( table );
                SetLastError( WSAEINVAL );
                return -1;
            }

            for (p = table, num = 0; p; p = p->Next)
                if (p->IpAddressList.IpAddress.String[0]) num++;

            total = FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[num]) + num * sizeof(*sockaddr);
            if (total > out_size || !out_buff)
            {
                *ret_size = total;
                free( table );
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

                sockaddr[i].sin_family = AF_INET;
                sockaddr[i].sin_port = 0;
                sockaddr[i].sin_addr.s_addr = inet_addr( p->IpAddressList.IpAddress.String );
                i++;
            }

            free( table );

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

    case SIO_GET_EXTENSION_FUNCTION_POINTER:
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

    case SIO_KEEPALIVE_VALS:
    {
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_KEEPALIVE_VALS, in_buff, in_size,
                                 out_buff, out_size, ret_size, overlapped, completion );
        if (!overlapped || completion) *ret_size = 0;
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case SIO_ROUTING_INTERFACE_QUERY:
    {
        struct sockaddr *daddr = (struct sockaddr *)in_buff;
        struct sockaddr_in *daddr_in = (struct sockaddr_in *)daddr;
        struct sockaddr_in *saddr_in = out_buff;
        MIB_IPFORWARDROW row;
        PMIB_IPADDRTABLE ipAddrTable = NULL;
        DWORD size, i, found_index, ret = 0;
        NTSTATUS status = STATUS_SUCCESS;

        if (!in_buff || in_size < sizeof(struct sockaddr) ||
            !out_buff || out_size < sizeof(struct sockaddr_in))
        {
            SetLastError( WSAEFAULT );
            return -1;
        }
        if (daddr->sa_family != AF_INET)
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
        ipAddrTable = malloc( size );
        if (GetIpAddrTable( ipAddrTable, &size, FALSE ))
        {
            free( ipAddrTable );
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
            ERR( "no matching IP address for interface %lu\n", row.dwForwardIfIndex );
            free( ipAddrTable );
            SetLastError( WSAEFAULT );
            return -1;
        }
        saddr_in->sin_family = AF_INET;
        saddr_in->sin_addr.S_un.S_addr = ipAddrTable->table[found_index].dwAddr;
        saddr_in->sin_port = 0;
        free( ipAddrTable );

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                 NULL, 0, ret_size, overlapped, completion );
        if (!ret) *ret_size = sizeof(struct sockaddr_in);
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case SIO_ADDRESS_LIST_CHANGE:
    {
        int force_async = !!overlapped;
        DWORD ret;

        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_ADDRESS_LIST_CHANGE, &force_async, sizeof(force_async),
                                 out_buff, out_size, ret_size, overlapped, completion );
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case SIO_UDP_CONNRESET:
    {
        NTSTATUS status = STATUS_SUCCESS;
        DWORD ret;

        FIXME( "SIO_UDP_CONNRESET stub\n" );
        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                 NULL, 0, ret_size, overlapped, completion );
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case SIO_ENABLE_CIRCULAR_QUEUEING:
    {
        NTSTATUS status = STATUS_SUCCESS;
        DWORD ret;

        FIXME( "SIO_ENABLE_CIRCULAR_QUEUEING stub\n" );
        ret = server_ioctl_sock( s, IOCTL_AFD_WINE_COMPLETE_ASYNC, &status, sizeof(status),
                                 NULL, 0, ret_size, overlapped, completion );
        SetLastError( ret );
        return ret ? -1 : 0;
    }

    case SIO_BASE_HANDLE:
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
    case LOWORD(FIONBIO): /* Netscape tries to use this */
    case SIO_SET_COMPATIBILITY_MODE:
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
int WINAPI ioctlsocket( SOCKET s, LONG cmd, u_long *argp )
{
    DWORD ret_size;
    return WSAIoctl( s, cmd, argp, sizeof(u_long), argp, sizeof(u_long), &ret_size, NULL, NULL );
}


/***********************************************************************
 *      listen   (ws2_32.13)
 */
int WINAPI listen( SOCKET s, int backlog )
{
    struct afd_listen_params params = {.backlog = backlog};
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#Ix, backlog %d\n", s, backlog );

    status = NtDeviceIoControlFile( SOCKET2HANDLE(s), NULL, NULL, NULL, &io,
            IOCTL_AFD_LISTEN, &params, sizeof(params), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return status ? -1 : 0;
}


/***********************************************************************
 *		recv			(WS2_32.16)
 */
int WINAPI recv( SOCKET s, char *buf, int len, int flags )
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
int WINAPI recvfrom( SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen )
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
static int add_fd_to_set( SOCKET fd, struct fd_set *set )
{
    unsigned int i;

    for (i = 0; i < set->fd_count; ++i)
    {
        if (set->fd_array[i] == fd)
            return 0;
    }

    set->fd_array[set->fd_count++] = fd;
    return 1;
}


/***********************************************************************
 *      select   (ws2_32.18)
 */
int WINAPI select( int count, fd_set *read_ptr, fd_set *write_ptr,
                   fd_set *except_ptr, const struct timeval *timeout)
{
    static const int read_flags = AFD_POLL_READ | AFD_POLL_ACCEPT | AFD_POLL_HUP | AFD_POLL_RESET;
    static const int write_flags = AFD_POLL_WRITE;
    static const int except_flags = AFD_POLL_OOB | AFD_POLL_CONNECT_ERR;

    struct fd_set *read_input = NULL;
    struct afd_poll_params *params;
    unsigned int poll_count = 0;
    ULONG params_size, i, j;
    SOCKET poll_socket = 0;
    IO_STATUS_BLOCK io;
    HANDLE sync_event;
    int ret_count = 0;
    NTSTATUS status;

    TRACE( "read %p, write %p, except %p, timeout %p\n", read_ptr, write_ptr, except_ptr, timeout );

    if (!(sync_event = get_sync_event())) return -1;

    if (read_ptr) poll_count += read_ptr->fd_count;
    if (write_ptr) poll_count += write_ptr->fd_count;
    if (except_ptr) poll_count += except_ptr->fd_count;

    if (!poll_count)
    {
        SetLastError( WSAEINVAL );
        return -1;
    }

    params_size = offsetof( struct afd_poll_params, sockets[poll_count] );
    if (!(params = calloc( params_size, 1 )))
    {
        SetLastError( WSAENOBUFS );
        return -1;
    }

    if (timeout)
        params->timeout = (LONGLONG)timeout->tv_sec * -10000000 + (LONGLONG)timeout->tv_usec * -10;
    else
        params->timeout = TIMEOUT_INFINITE;

    if (read_ptr)
    {
        unsigned int read_size = offsetof( struct fd_set, fd_array[read_ptr->fd_count] );

        if (!(read_input = malloc( read_size )))
        {
            free( params );
            SetLastError( WSAENOBUFS );
            return -1;
        }
        memcpy( read_input, read_ptr, read_size );

        for (i = 0; i < read_ptr->fd_count; ++i)
        {
            params->sockets[params->count].socket = read_ptr->fd_array[i];
            params->sockets[params->count].flags = read_flags;
            ++params->count;
            poll_socket = read_ptr->fd_array[i];
        }
    }

    if (write_ptr)
    {
        for (i = 0; i < write_ptr->fd_count; ++i)
        {
            params->sockets[params->count].socket = write_ptr->fd_array[i];
            params->sockets[params->count].flags = write_flags;
            ++params->count;
            poll_socket = write_ptr->fd_array[i];
        }
    }

    if (except_ptr)
    {
        for (i = 0; i < except_ptr->fd_count; ++i)
        {
            params->sockets[params->count].socket = except_ptr->fd_array[i];
            params->sockets[params->count].flags = except_flags;
            ++params->count;
            poll_socket = except_ptr->fd_array[i];
        }
    }

    assert( params->count == poll_count );

    status = NtDeviceIoControlFile( (HANDLE)poll_socket, sync_event, NULL, NULL, &io,
                                    IOCTL_AFD_POLL, params, params_size, params, params_size );
    if (status == STATUS_PENDING)
    {
        if (wait_event_alertable( sync_event ) == WAIT_FAILED)
        {
            free( read_input );
            free( params );
            return -1;
        }
        status = io.Status;
    }
    if (status == STATUS_TIMEOUT) status = STATUS_SUCCESS;
    if (!status)
    {
        /* pointers may alias, so clear them all first */
        if (read_ptr) read_ptr->fd_count = 0;
        if (write_ptr) write_ptr->fd_count = 0;
        if (except_ptr) except_ptr->fd_count = 0;

        for (i = 0; i < params->count; ++i)
        {
            unsigned int flags = params->sockets[i].flags;
            SOCKET s = params->sockets[i].socket;

            if (read_input)
            {
                for (j = 0; j < read_input->fd_count; ++j)
                {
                    if (read_input->fd_array[j] == s && (flags & (read_flags | AFD_POLL_CLOSE)))
                    {
                        ret_count += add_fd_to_set( s, read_ptr );
                        flags &= ~AFD_POLL_CLOSE;
                    }
                }
            }

            if (flags & AFD_POLL_CLOSE)
                status = STATUS_INVALID_HANDLE;

            if (flags & write_flags)
                ret_count += add_fd_to_set( s, write_ptr );

            if (flags & except_flags)
                ret_count += add_fd_to_set( s, except_ptr );

            if (flags & ~(read_flags | write_flags | except_flags | AFD_POLL_CLOSE))
                FIXME( "not reporting AFD flags %#x\n", flags );
        }
    }

    free( read_input );
    free( params );

    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
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
    if (!(params = calloc( params_size, 1 )))
    {
        SetLastError(WSAENOBUFS);
        return SOCKET_ERROR;
    }

    params->timeout = (timeout >= 0 ? (LONGLONG)timeout * -10000 : TIMEOUT_INFINITE);

    for (i = 0; i < count; ++i)
    {
        unsigned int flags = AFD_POLL_HUP | AFD_POLL_RESET | AFD_POLL_CONNECT_ERR;

        if ((INT_PTR)fds[i].fd < 0 || !socket_list_find( fds[i].fd ))
        {
            fds[i].revents = POLLNVAL;
            continue;
        }

        poll_socket = fds[i].fd;
        params->sockets[params->count].socket = fds[i].fd;

        if (fds[i].events & POLLRDNORM)
            flags |= AFD_POLL_ACCEPT | AFD_POLL_READ;
        if (fds[i].events & POLLRDBAND)
            flags |= AFD_POLL_OOB;
        if (fds[i].events & POLLWRNORM)
            flags |= AFD_POLL_WRITE;
        params->sockets[params->count].flags = flags;
        ++params->count;

        fds[i].revents = 0;
    }

    if (!poll_socket)
    {
        SetLastError( WSAENOTSOCK );
        free( params );
        return -1;
    }

    status = NtDeviceIoControlFile( (HANDLE)poll_socket, sync_event, NULL, NULL, &io, IOCTL_AFD_POLL,
                                    params, params_size, params, params_size );
    if (status == STATUS_PENDING)
    {
        if (wait_event_alertable( sync_event ) == WAIT_FAILED)
        {
            free( params );
            return -1;
        }
        status = io.Status;
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
                        revents |= POLLRDNORM;
                    if (params->sockets[j].flags & AFD_POLL_OOB)
                        revents |= POLLRDBAND;
                    if (params->sockets[j].flags & AFD_POLL_WRITE)
                        revents |= POLLWRNORM;
                    if (params->sockets[j].flags & (AFD_POLL_RESET | AFD_POLL_HUP))
                        revents |= POLLHUP;
                    if (params->sockets[j].flags & (AFD_POLL_RESET | AFD_POLL_CONNECT_ERR))
                        revents |= POLLERR;
                    if (params->sockets[j].flags & AFD_POLL_CLOSE)
                        revents |= POLLNVAL;

                    fds[i].revents = revents & (fds[i].events | POLLHUP | POLLERR | POLLNVAL);

                    if (fds[i].revents)
                        ++ret_count;
                }
            }
        }
    }
    if (status == STATUS_TIMEOUT) status = STATUS_SUCCESS;

    free( params );

    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return status ? -1 : ret_count;
}


/***********************************************************************
 *		send			(WS2_32.19)
 */
int WINAPI send( SOCKET s, const char *buf, int len, int flags )
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
    return shutdown( s, SD_SEND );
}


/***********************************************************************
 *		WSASendTo		(WS2_32.74)
 */
INT WINAPI WSASendTo( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                      LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                      const struct sockaddr *to, int tolen,
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
int WINAPI sendto( SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen )
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
    TRACE( "status %#lx.\n", status );
    return status ? -1 : 0;
}


/***********************************************************************
 *		setsockopt		(WS2_32.21)
 */
int WINAPI setsockopt( SOCKET s, int level, int optname, const char *optval, int optlen )
{
    DWORD value = 0;

    TRACE( "socket %#Ix, %s, optval %s, optlen %d\n",
           s, debugstr_sockopt(level, optname), debugstr_optval(optval, optlen), optlen );

    /* some broken apps pass the value directly instead of a pointer to it */
    if(optlen && IS_INTRESOURCE(optval))
    {
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    switch(level)
    {
    case SOL_SOCKET:
        switch(optname)
        {
        case SO_BROADCAST:
            if (!optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_BROADCAST, (char *)&value, sizeof(value) );
        case SO_DONTLINGER:
        {
            LINGER linger;

            if (!optval)
            {
                SetLastError( WSAEFAULT );
                return -1;
            }

            linger.l_onoff  = !*(const BOOL *)optval;
            linger.l_linger = 0;
            return setsockopt( s, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger) );
        }

        case SO_ERROR:
            FIXME( "SO_ERROR, stub.\n" );
            SetLastError( ERROR_SUCCESS );
            return 0;

        case SO_KEEPALIVE:
            if (optlen <= 0 || !optval)
            {
                SetLastError( optlen ? WSAENOBUFS : WSAEFAULT );
                return SOCKET_ERROR;
            }
            value = *optval;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_KEEPALIVE, (char *)&value, sizeof(value) );

        case SO_LINGER:
            if (optlen < sizeof(LINGER) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_LINGER, optval, optlen );

        case SO_OOBINLINE:
            if (!optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_OOBINLINE, (char *)&value, sizeof(value) );

        case SO_RCVBUF:
            if (optlen < 0) optlen = 4;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_RCVBUF, optval, optlen );

        case SO_RCVTIMEO:
            if (optlen < 0) optlen = 4;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_RCVTIMEO, optval, optlen );

        case SO_REUSEADDR:
            if (!optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_REUSEADDR, (char *)&value, sizeof(value) );

        case SO_EXCLUSIVEADDRUSE:
            if (!optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_EXCLUSIVEADDRUSE, (char *)&value, sizeof(value) );

        case SO_SNDBUF:
            if (optlen < 0) optlen = 4;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_SNDBUF, optval, optlen );

        case SO_SNDTIMEO:
            if (optlen < 0) optlen = 4;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_SO_SNDTIMEO, optval, optlen );

        /* SO_DEBUG is a privileged operation, ignore it. */
        case SO_DEBUG:
            TRACE("Ignoring SO_DEBUG\n");
            return 0;

        /* For some reason the game GrandPrixLegends does set SO_DONTROUTE on its
         * socket. According to MSDN, this option is silently ignored.*/
        case SO_DONTROUTE:
            TRACE( "Ignoring SO_DONTROUTE.\n" );
            if (optlen <= 0)
            {
                SetLastError( optlen ? WSAENOBUFS : WSAEFAULT );
                return -1;
            }
            SetLastError( ERROR_SUCCESS );
            return 0;

        /* After a ConnectEx call succeeds, the socket can't be used with half of the
         * normal winsock functions on windows. We don't have that problem. */
        case SO_UPDATE_CONNECT_CONTEXT:
            TRACE("Ignoring SO_UPDATE_CONNECT_CONTEXT, since our sockets are normal\n");
            return 0;

        /* After a AcceptEx call succeeds, the socket can't be used with half of the
         * normal winsock functions on windows. We don't have that problem. */
        case SO_UPDATE_ACCEPT_CONTEXT:
            TRACE("Ignoring SO_UPDATE_ACCEPT_CONTEXT, since our sockets are normal\n");
            return 0;

        /* SO_OPENTYPE does not require a valid socket handle. */
        case SO_OPENTYPE:
            if (!optlen || optlen < sizeof(int) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            get_per_thread_data()->opentype = *(const int *)optval;
            TRACE("setting global SO_OPENTYPE = 0x%x\n", *((const int*)optval) );
            SetLastError( ERROR_SUCCESS );
            return 0;

        case SO_RANDOMIZE_PORT:
            FIXME("Ignoring SO_RANDOMIZE_PORT\n");
            return 0;

        case SO_PORT_SCALABILITY:
            FIXME("Ignoring SO_PORT_SCALABILITY\n");
            return 0;

        case SO_REUSE_UNICASTPORT:
            FIXME("Ignoring SO_REUSE_UNICASTPORT\n");
            return 0;

        case SO_REUSE_MULTICASTPORT:
            FIXME("Ignoring SO_REUSE_MULTICASTPORT\n");
            return 0;

        default:
            TRACE("Unknown SOL_SOCKET optname: 0x%08x\n", optname);
            /* fall through */

        case SO_ACCEPTCONN:
        case SO_TYPE:
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        }
        break; /* case SOL_SOCKET */

    case NSPROTO_IPX:
        switch(optname)
        {
        case IPX_PTYPE:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPX_PTYPE, optval, optlen );

        case IPX_FILTERPTYPE:
            /* Sets the receive filter packet type, at the moment we don't support it */
            FIXME("IPX_FILTERPTYPE: %x\n", *optval);
            /* Returning 0 is better for now than returning a SOCKET_ERROR */
            return 0;

        default:
            FIXME("opt_name:%x\n", optname);
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        }
        break; /* case NSPROTO_IPX */

    case IPPROTO_TCP:
        if (optlen < 0)
        {
            SetLastError(WSAENOBUFS);
            return SOCKET_ERROR;
        }

        switch(optname)
        {
        case TCP_NODELAY:
            if (optlen <= 0 || !optval)
            {
                SetLastError( optlen && optval ? WSAENOBUFS : WSAEFAULT );
                return SOCKET_ERROR;
            }
            value = *optval;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_TCP_NODELAY, (char*)&value, sizeof(value) );

        case TCP_KEEPALIVE:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            value = *(DWORD*)optval;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_TCP_KEEPALIVE, (char*)&value, sizeof(value) );

        case TCP_KEEPCNT:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            value = *(DWORD*)optval;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_TCP_KEEPCNT, (char*)&value, sizeof(value) );

        case TCP_KEEPINTVL:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            value = *(DWORD*)optval;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_TCP_KEEPINTVL, (char*)&value, sizeof(value) );

        default:
            FIXME("Unknown IPPROTO_TCP optname 0x%08x\n", optname);
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        }
        break;

    case IPPROTO_IP:
        if (optlen < 0)
        {
            SetLastError(WSAENOBUFS);
            return SOCKET_ERROR;
        }

        switch(optname)
        {
        case IP_ADD_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_ADD_MEMBERSHIP, optval, optlen );

        case IP_ADD_SOURCE_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_ADD_SOURCE_MEMBERSHIP, optval, optlen );

        case IP_BLOCK_SOURCE:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_BLOCK_SOURCE, optval, optlen );

        case IP_DONTFRAGMENT:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_DONTFRAGMENT, optval, optlen );

        case IP_DROP_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_DROP_MEMBERSHIP, optval, optlen );

        case IP_DROP_SOURCE_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_DROP_SOURCE_MEMBERSHIP, optval, optlen );

        case IP_HDRINCL:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_HDRINCL, optval, optlen );

        case IP_MULTICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_MULTICAST_IF, optval, optlen );

        case IP_MULTICAST_LOOP:
            if (!optlen)
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_MULTICAST_LOOP, optval, optlen );

        case IP_MULTICAST_TTL:
            if (!optlen)
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_MULTICAST_TTL, optval, optlen );

        case IP_OPTIONS:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_OPTIONS, optval, optlen );

        case IP_PKTINFO:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_PKTINFO, optval, optlen );

        case IP_RECVTOS:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_RECVTOS, optval, optlen );

        case IP_RECVTTL:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_RECVTTL, optval, optlen );

        case IP_TOS:
            if (!optlen || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            if (value > 0xff)
            {
                SetLastError(WSAEINVAL);
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_TOS, optval, optlen );

        case IP_TTL:
            if (!optlen)
            {
                SetLastError( WSAEFAULT );
                return -1;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_TTL, optval, optlen );

        case IP_UNBLOCK_SOURCE:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_UNBLOCK_SOURCE, optval, optlen );

        case IP_UNICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IP_UNICAST_IF, optval, optlen );

        default:
            FIXME("Unknown IPPROTO_IP optname 0x%08x\n", optname);
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        }
        break;

    case IPPROTO_IPV6:
        if (optlen < 0)
        {
            SetLastError( WSAENOBUFS );
            return SOCKET_ERROR;
        }

        switch(optname)
        {
        case IPV6_ADD_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_ADD_MEMBERSHIP, optval, optlen );

        case IPV6_DONTFRAG:
            if (!optlen || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_DONTFRAG, (char *)&value, sizeof(value) );

        case IPV6_DROP_MEMBERSHIP:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_DROP_MEMBERSHIP, optval, optlen );

        case IPV6_HOPLIMIT:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_RECVHOPLIMIT, optval, optlen );

        case IPV6_MULTICAST_HOPS:
            if (!optlen || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_MULTICAST_HOPS, (char *)&value, sizeof(value) );

        case IPV6_MULTICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_MULTICAST_IF, optval, optlen );

        case IPV6_MULTICAST_LOOP:
            if (!optlen || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            value = !!value;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_MULTICAST_LOOP, (char *)&value, sizeof(value) );

        case IPV6_PKTINFO:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_RECVPKTINFO, optval, optlen );

        case IPV6_PROTECTION_LEVEL:
            FIXME("IPV6_PROTECTION_LEVEL is ignored!\n");
            return 0;

        case IPV6_RECVTCLASS:
            if (optlen < sizeof(DWORD) || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_RECVTCLASS, optval, optlen );

        case IPV6_UNICAST_HOPS:
            if (!optlen || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            memcpy( &value, optval, min( optlen, sizeof(value) ));
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_UNICAST_HOPS, (char *)&value, sizeof(value) );

        case IPV6_UNICAST_IF:
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_UNICAST_IF, optval, optlen );

        case IPV6_V6ONLY:
            if (!optlen || !optval)
            {
                SetLastError( WSAEFAULT );
                return SOCKET_ERROR;
            }
            value = *optval;
            return server_setsockopt( s, IOCTL_AFD_WINE_SET_IPV6_V6ONLY, (char *)&value, sizeof(value) );

        default:
            FIXME("Unknown IPPROTO_IPV6 optname 0x%08x\n", optname);
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        }
        break;

    default:
        WARN("Unknown level: 0x%08x\n", level);
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    } /* end switch(level) */
}


/***********************************************************************
 *      shutdown   (ws2_32.22)
 */
int WINAPI shutdown( SOCKET s, int how )
{
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    TRACE( "socket %#Ix, how %u\n", s, how );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io,
                                    IOCTL_AFD_WINE_SHUTDOWN, &how, sizeof(how), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
    return status ? -1 : 0;
}


/***********************************************************************
 *		socket		(WS2_32.23)
 */
SOCKET WINAPI socket( int af, int type, int protocol )
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

    TRACE( "socket %#Ix, event %p, events %p\n", s, event, ret_events );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_GET_EVENTS,
                                    event, 0, &params, sizeof(params) );
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
            {
                if (params.flags & AFD_POLL_RESET)
                    ret_events->iErrorCode[FD_CLOSE_BIT] = WSAECONNABORTED;
            }
        }
    }
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
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

    TRACE( "socket %#Ix, event %p, mask %#lx\n", s, event, mask );

    params.event = event;
    params.mask = afd_poll_flag_from_win32( mask );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_EVENT_SELECT,
                                    &params, sizeof(params), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
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

    TRACE( "socket %#Ix, overlapped %p, transfer %p, wait %d, flags %p\n",
           s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags );

    if ( lpOverlapped == NULL )
    {
        ERR( "Invalid pointer\n" );
        SetLastError(WSA_INVALID_PARAMETER);
        return FALSE;
    }

    if (!socket_list_find( s ))
    {
        SetLastError( WSAENOTSOCK );
        return FALSE;
    }

    /* Paired with the write-release in set_async_iosb() in ntdll; see the
     * latter for details. */
    status = ReadAcquire( (LONG *)&lpOverlapped->Internal );
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
        /* We don't need to give this load acquire semantics; the wait above
         * already guarantees that the IOSB and output buffer are filled. */
        status = lpOverlapped->Internal;
    }

    if ( lpcbTransfer )
        *lpcbTransfer = lpOverlapped->InternalHigh;

    if ( lpdwFlags )
        *lpdwFlags = lpOverlapped->Offset;

    SetLastError( NtStatusToWSAError(status) );
    TRACE( "status %#lx.\n", status );
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

    TRACE( "socket %#Ix, window %p, message %#x, mask %#lx\n", s, window, message, mask );

    params.handle = s;
    params.window = HandleToULong( window );
    params.message = message;
    params.mask = afd_poll_flag_from_win32( mask );

    status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_WINE_MESSAGE_SELECT,
                                    &params, sizeof(params), NULL, 0 );
    SetLastError( NtStatusToWSAError( status ) );
    TRACE( "status %#lx.\n", status );
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
    struct afd_create_params create_params;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string = RTL_CONSTANT_STRING(L"\\Device\\Afd");
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    SOCKET ret;
    DWORD err;

   /*
      FIXME: The "advanced" parameters of WSASocketW (lpProtocolInfo,
      g, dwFlags except WSA_FLAG_OVERLAPPED) are ignored.
   */

    TRACE( "family %d, type %d, protocol %d, info %p, group %u, flags %#lx\n",
           af, type, protocol, lpProtocolInfo, g, flags );

    if (!num_startup)
    {
        WARN( "not initialised\n" );
        SetLastError( WSANOTINITIALISED );
        return -1;
    }

    /* hack for WSADuplicateSocket */
    if (lpProtocolInfo && lpProtocolInfo->dwServiceFlags4 == 0xff00ff00)
    {
        ret = lpProtocolInfo->dwServiceFlags3;
        TRACE( "got duplicate %#Ix\n", ret );
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

    InitializeObjectAttributes(&attr, &string, (flags & WSA_FLAG_NO_HANDLE_INHERIT) ? 0 : OBJ_INHERIT, NULL, NULL);
    if ((status = NtOpenFile(&handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &attr,
            &io, 0, (flags & WSA_FLAG_OVERLAPPED) ? 0 : FILE_SYNCHRONOUS_IO_NONALERT)))
    {
        WARN( "failed to create socket, status %#lx\n", status );
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
        WARN( "failed to initialize socket, status %#lx\n", status );
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
    TRACE( "created %#Ix\n", ret );

    if (!socket_list_add(ret))
    {
        CloseHandle(handle);
        return INVALID_SOCKET;
    }
    WSASetLastError(0);
    return ret;
}

/***********************************************************************
 *      WSAJoinLeaf          (WS2_32.58)
 *
 */
SOCKET WINAPI WSAJoinLeaf( SOCKET s, const struct sockaddr *addr, int addrlen, WSABUF *caller_data,
                           WSABUF *callee_data, QOS *socket_qos, QOS *group_qos, DWORD flags)
{
    FIXME("stub.\n");
    return INVALID_SOCKET;
}

/***********************************************************************
 *      __WSAFDIsSet			(WS2_32.151)
 */
int WINAPI __WSAFDIsSet( SOCKET s, fd_set *set )
{
  int i = set->fd_count, ret = 0;

  while (i--)
      if (set->fd_array[i] == s)
      {
          ret = 1;
          break;
      }

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
    if (!dwBufferCount)
    {
        SetLastError( WSAEINVAL );
        return -1;
    }

    return WS2_recv_base(s, lpBuffers, dwBufferCount, NumberOfBytesReceived, lpFlags,
                       NULL, NULL, lpOverlapped, lpCompletionRoutine, NULL);
}


/***********************************************************************
 *              WSARecvFrom             (WS2_32.69)
 */
INT WINAPI WSARecvFrom( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                        DWORD *lpNumberOfBytesRecvd, DWORD *lpFlags, struct sockaddr *lpFrom,
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
SOCKET WINAPI WSAAccept( SOCKET s, struct sockaddr *addr, int *addrlen,
                         LPCONDITIONPROC callback, DWORD_PTR context )
{
    int ret = 0, size;
    WSABUF caller_id, caller_data, callee_id, callee_data;
    struct sockaddr src_addr, dst_addr;
    GROUP group;
    SOCKET cs;

    TRACE( "socket %#Ix, addr %p, addrlen %p, callback %p, context %#Ix\n",
           s, addr, addrlen, callback, context );

    cs = accept( s, addr, addrlen );
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
        getpeername( cs, &src_addr, &size );
        caller_id.buf = (char *)&src_addr;
        caller_id.len = size;
    }
    caller_data.buf = NULL;
    caller_data.len = 0;

    size = sizeof(dst_addr);
    getsockname( cs, &dst_addr, &size );

    callee_id.buf = (char *)&dst_addr;
    callee_id.len = sizeof(dst_addr);

    ret = (*callback)( &caller_id, &caller_data, NULL, NULL,
                       &callee_id, &callee_data, &group, context );
    TRACE( "callback returned %d.\n", ret );
    switch (ret)
    {
    case CF_ACCEPT:
        return cs;

    case CF_DEFER:
    {
        ULONG server_handle = cs;
        IO_STATUS_BLOCK io;
        NTSTATUS status;

        status = NtDeviceIoControlFile( (HANDLE)s, NULL, NULL, NULL, &io, IOCTL_AFD_WINE_DEFER,
                                        &server_handle, sizeof(server_handle), NULL, 0 );
        closesocket( cs );
        SetLastError( status ? RtlNtStatusToDosError( status ) : WSATRY_AGAIN );
        TRACE( "status %#lx.\n", status );
        return -1;
    }

    case CF_REJECT:
        closesocket( cs );
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
BOOL WINAPI WSAGetQOSByName( SOCKET s, WSABUF *name, QOS *qos )
{
    FIXME( "socket %#Ix, name %p, qos %p, stub!\n", s, name, qos );
    return FALSE;
}


/***********************************************************************
 *              WSARecvDisconnect                           (WS2_32.68)
 */
int WINAPI WSARecvDisconnect( SOCKET s, WSABUF *data )
{
    TRACE( "socket %#Ix, data %p\n", s, data );

    return shutdown( s, SD_RECEIVE );
}


static BOOL protocol_matches_filter( const int *filter, unsigned int index )
{
    if (supported_protocols[index].dwProviderFlags & PFL_HIDDEN) return FALSE;
    if (!filter) return TRUE;
    while (*filter)
    {
        if (supported_protocols[index].iProtocol == *filter++) return TRUE;
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
        if (protocol_matches_filter( filter, i ))
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
        if (protocol_matches_filter( filter, i ))
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
        if (protocol_matches_filter( filter, i ))
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
        if (protocol_matches_filter( filter, i ))
            protocols[count++] = supported_protocols[i];
    }
    return count;
}
