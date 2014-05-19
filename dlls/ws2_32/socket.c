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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif

#if defined(__EMX__)
# include <sys/so_ioctl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MSG_H
# include <sys/msg.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_LINUX_FILTER_H
# include <linux/filter.h>
#endif

#ifdef HAVE_NETIPX_IPX_H
# include <netipx/ipx.h>
# define HAS_IPX
#elif defined(HAVE_LINUX_IPX_H)
# ifdef HAVE_ASM_TYPES_H
#  include <asm/types.h>
# endif
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/ipx.h>
# define HAS_IPX
#endif

#ifdef HAVE_LINUX_IRDA_H
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/irda.h>
# define HAS_IRDA
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "winsock2.h"
#include "mswsock.h"
#include "ws2tcpip.h"
#include "ws2spi.h"
#include "wsipx.h"
#include "wshisotp.h"
#include "mstcpip.h"
#include "af_irda.h"
#include "winnt.h"
#define USE_WC_PREFIX   /* For CMSG_DATA */
#include "iphlpapi.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/unicode.h"

#ifdef HAS_IPX
# include "wsnwlink.h"
#endif

#if defined(linux) && !defined(IP_UNICAST_IF)
#define IP_UNICAST_IF 50
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)  || defined(__DragonFly__)
# define sipx_network    sipx_addr.x_net
# define sipx_node       sipx_addr.x_host.c_host
#endif  /* __FreeBSD__ */

#ifndef INADDR_NONE
#define INADDR_NONE ~0UL
#endif

WINE_DEFAULT_DEBUG_CHANNEL(winsock);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

/* names of the protocols */
static const WCHAR NameIpxW[]   = {'I', 'P', 'X', '\0'};
static const WCHAR NameSpxW[]   = {'S', 'P', 'X', '\0'};
static const WCHAR NameSpxIIW[] = {'S', 'P', 'X', ' ', 'I', 'I', '\0'};
static const WCHAR NameTcpW[]   = {'T', 'C', 'P', '/', 'I', 'P', '\0'};
static const WCHAR NameUdpW[]   = {'U', 'D', 'P', '/', 'I', 'P', '\0'};

/* Taken from Win2k */
static const GUID ProviderIdIP = { 0xe70f1aa0, 0xab8b, 0x11cf,
                                   { 0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } };
static const GUID ProviderIdIPX = { 0x11058240, 0xbe47, 0x11cf,
                                    { 0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } };
static const GUID ProviderIdSPX = { 0x11058241, 0xbe47, 0x11cf,
                                    { 0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } };

static const INT valid_protocols[] =
{
    WS_IPPROTO_TCP,
    WS_IPPROTO_UDP,
    WS_NSPROTO_IPX,
    WS_NSPROTO_SPX,
    WS_NSPROTO_SPXII,
    0
};

#define IS_IPX_PROTO(X) ((X) >= WS_NSPROTO_IPX && (X) <= WS_NSPROTO_IPX + 255)

#if defined(IP_UNICAST_IF) && defined(SO_ATTACH_FILTER)
# define LINUX_BOUND_IF
struct interface_filter {
    struct sock_filter iface_memaddr;
    struct sock_filter iface_rule;
    struct sock_filter ip_memaddr;
    struct sock_filter ip_rule;
    struct sock_filter return_keep;
    struct sock_filter return_dump;
};
# define FILTER_JUMP_DUMP(here)  (u_char)(offsetof(struct interface_filter, return_dump) \
                                 -offsetof(struct interface_filter, here)-sizeof(struct sock_filter)) \
                                 /sizeof(struct sock_filter)
# define FILTER_JUMP_KEEP(here)  (u_char)(offsetof(struct interface_filter, return_keep) \
                                 -offsetof(struct interface_filter, here)-sizeof(struct sock_filter)) \
                                 /sizeof(struct sock_filter)
# define FILTER_JUMP_NEXT()      (u_char)(0)
# define SKF_NET_DESTIP          16 /* offset in the network header to the destination IP */
static struct interface_filter generic_interface_filter = {
    /* This filter rule allows incoming packets on the specified interface, which works for all
     * remotely generated packets and for locally generated broadcast packets. */
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, SKF_AD_OFF+SKF_AD_IFINDEX),
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xdeadbeef, FILTER_JUMP_KEEP(iface_rule), FILTER_JUMP_NEXT()),
    /* This rule allows locally generated packets targeted at the specific IP address of the chosen
     * adapter (local packets not destined for the broadcast address do not have IFINDEX set) */
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, SKF_NET_OFF+SKF_NET_DESTIP),
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xdeadbeef, FILTER_JUMP_KEEP(ip_rule), FILTER_JUMP_DUMP(ip_rule)),
    BPF_STMT(BPF_RET+BPF_K, (u_int)-1), /* keep packet */
    BPF_STMT(BPF_RET+BPF_K, 0)          /* dump packet */
};
#endif /* LINUX_BOUND_IF */

/*
 * The actual definition of WSASendTo, wrapped in a different function name
 * so that internal calls from ws2_32 itself will not trigger programs like
 * Garena, which hooks WSASendTo/WSARecvFrom calls.
 */
static int WS2_sendto( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                       LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                       const struct WS_sockaddr *to, int tolen,
                       LPWSAOVERLAPPED lpOverlapped,
                       LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine );

/*
 * Internal fundamental receive function, essentially WSARecvFrom with an
 * additional parameter to support message control headers.
 */
static int WS2_recv_base( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                          LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
                          struct WS_sockaddr *lpFrom,
                          LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped,
                          LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
                          LPWSABUF lpControlBuffer );

/* critical section to protect some non-reentrant net function */
static CRITICAL_SECTION csWSgetXXXbyYYY;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csWSgetXXXbyYYY,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csWSgetXXXbyYYY") }
};
static CRITICAL_SECTION csWSgetXXXbyYYY = { &critsect_debug, -1, 0, 0, 0, 0 };

union generic_unix_sockaddr
{
    struct sockaddr addr;
    char data[128];  /* should be big enough for all families */
};

static inline const char *debugstr_sockaddr( const struct WS_sockaddr *a )
{
    if (!a) return "(nil)";
    switch (a->sa_family)
    {
    case WS_AF_INET:
        return wine_dbg_sprintf("{ family AF_INET, address %s, port %d }",
                                inet_ntoa(((const struct sockaddr_in *)a)->sin_addr),
                                ntohs(((const struct sockaddr_in *)a)->sin_port));
    case WS_AF_INET6:
    {
        char buf[46];
        const char *p;
        struct WS_sockaddr_in6 *sin = (struct WS_sockaddr_in6 *)a;

        p = WS_inet_ntop( WS_AF_INET6, &sin->sin6_addr, buf, sizeof(buf) );
        if (!p)
            p = "(unknown IPv6 address)";
        return wine_dbg_sprintf("{ family AF_INET6, address %s, port %d }",
                                p, ntohs(sin->sin6_port));
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

/* HANDLE<->SOCKET conversion (SOCKET is UINT_PTR). */
#define SOCKET2HANDLE(s) ((HANDLE)(s))
#define HANDLE2SOCKET(h) ((SOCKET)(h))

/****************************************************************
 * Async IO declarations
 ****************************************************************/

typedef struct ws2_async
{
    HANDLE                              hSocket;
    int                                 type;
    LPWSAOVERLAPPED                     user_overlapped;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE  completion_func;
    IO_STATUS_BLOCK                     local_iosb;
    struct WS_sockaddr                  *addr;
    union
    {
        int val;     /* for send operations */
        int *ptr;    /* for recv operations */
    }                                   addrlen;
    DWORD                               flags;
    DWORD                              *lpFlags;
    WSABUF                             *control;
    unsigned int                        n_iovecs;
    unsigned int                        first_iovec;
    struct iovec                        iovec[1];
} ws2_async;

typedef struct ws2_accept_async
{
    HANDLE              listen_socket;
    HANDLE              accept_socket;
    LPOVERLAPPED        user_overlapped;
    ULONG_PTR           cvalue;
    PVOID               buf;      /* buffer to write data to */
    int                 data_len;
    int                 local_len;
    int                 remote_len;
    struct ws2_async    *read;
} ws2_accept_async;

/****************************************************************/

/* ----------------------------------- internal data */

/* ws_... struct conversion flags */

typedef struct          /* WSAAsyncSelect() control struct */
{
  HANDLE      service, event, sock;
  HWND        hWnd;
  UINT        uMsg;
  LONG        lEvent;
} ws_select_info;

#define WS_MAX_SOCKETS_PER_PROCESS      128     /* reasonable guess */
#define WS_MAX_UDP_DATAGRAM             1024
static INT WINAPI WSA_DefaultBlockingHook( FARPROC x );

/* hostent's, servent's and protent's are stored in one buffer per thread,
 * as documented on MSDN for the functions that return any of the buffers */
struct per_thread_data
{
    int opentype;
    struct WS_hostent *he_buffer;
    struct WS_servent *se_buffer;
    struct WS_protoent *pe_buffer;
    int he_len;
    int se_len;
    int pe_len;
    char ntoa_buffer[16]; /* 4*3 digits + 3 '.' + 1 '\0' */
};

/* internal: routing description information */
struct route {
    struct in_addr addr;
    IF_INDEX interface;
    DWORD metric;
};

static INT num_startup;          /* reference counter */
static FARPROC blocking_hook = (FARPROC)WSA_DefaultBlockingHook;

/* function prototypes */
static struct WS_hostent *WS_create_he(char *name, int aliases, int aliases_size, int addresses, int address_length);
static struct WS_hostent *WS_dup_he(const struct hostent* p_he);
static struct WS_protoent *WS_dup_pe(const struct protoent* p_pe);
static struct WS_servent *WS_dup_se(const struct servent* p_se);
static int ws_protocol_info(SOCKET s, int unicode, WSAPROTOCOL_INFOW *buffer, int *size);

int WSAIOCTL_GetInterfaceCount(void);
int WSAIOCTL_GetInterfaceName(int intNumber, char *intName);

static void WS_AddCompletion( SOCKET sock, ULONG_PTR CompletionValue, NTSTATUS CompletionStatus, ULONG Information );

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
    MAP_OPTION( IP_OPTIONS ),
#ifdef IP_HDRINCL
    MAP_OPTION( IP_HDRINCL ),
#endif
    MAP_OPTION( IP_TOS ),
    MAP_OPTION( IP_TTL ),
#ifdef IP_PKTINFO
    MAP_OPTION( IP_PKTINFO ),
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

static const int ws_af_map[][2] =
{
    MAP_OPTION( AF_UNSPEC ),
    MAP_OPTION( AF_INET ),
    MAP_OPTION( AF_INET6 ),
#ifdef HAS_IPX
    MAP_OPTION( AF_IPX ),
#endif
#ifdef AF_IRDA
    MAP_OPTION( AF_IRDA ),
#endif
    {FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO},
};

static const int ws_socktype_map[][2] =
{
    MAP_OPTION( SOCK_DGRAM ),
    MAP_OPTION( SOCK_STREAM ),
    MAP_OPTION( SOCK_RAW ),
    {FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO},
};

static const int ws_proto_map[][2] =
{
    MAP_OPTION( IPPROTO_IP ),
    MAP_OPTION( IPPROTO_TCP ),
    MAP_OPTION( IPPROTO_UDP ),
    MAP_OPTION( IPPROTO_ICMP ),
    MAP_OPTION( IPPROTO_IGMP ),
    MAP_OPTION( IPPROTO_RAW ),
    {FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO},
};

static const int ws_aiflag_map[][2] =
{
    MAP_OPTION( AI_PASSIVE ),
    MAP_OPTION( AI_CANONNAME ),
    MAP_OPTION( AI_NUMERICHOST ),
#ifdef AI_NUMERICSERV
    MAP_OPTION( AI_NUMERICSERV ),
#endif
    MAP_OPTION( AI_V4MAPPED ),
    MAP_OPTION( AI_ADDRCONFIG ),
};

static const int ws_niflag_map[][2] =
{
    MAP_OPTION( NI_NOFQDN ),
    MAP_OPTION( NI_NUMERICHOST ),
    MAP_OPTION( NI_NAMEREQD ),
    MAP_OPTION( NI_NUMERICSERV ),
    MAP_OPTION( NI_DGRAM ),
};

static const int ws_eai_map[][2] =
{
    MAP_OPTION( EAI_AGAIN ),
    MAP_OPTION( EAI_BADFLAGS ),
    MAP_OPTION( EAI_FAIL ),
    MAP_OPTION( EAI_FAMILY ),
    MAP_OPTION( EAI_MEMORY ),
/* Note: EAI_NODATA is deprecated, but still 
 * used by Windows and Linux... We map the newer
 * EAI_NONAME to EAI_NODATA for now until Windows
 * changes too.
 */
#ifdef EAI_NODATA
    MAP_OPTION( EAI_NODATA ),
#endif
#ifdef EAI_NONAME
    { WS_EAI_NODATA, EAI_NONAME },
#endif

    MAP_OPTION( EAI_SERVICE ),
    MAP_OPTION( EAI_SOCKTYPE ),
    { 0, 0 }
};

static const char magic_loopback_addr[] = {127, 12, 34, 56};

#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
static inline WSACMSGHDR *fill_control_message(int level, int type, WSACMSGHDR *current, ULONG *maxsize, void *data, int len)
{
    ULONG msgsize = sizeof(WSACMSGHDR) + WSA_CMSG_ALIGN(len);
    char *ptr = (char *) current + sizeof(WSACMSGHDR);

    /* Make sure there is at least enough room for this entry */
    if (msgsize > *maxsize)
        return NULL;
    *maxsize -= msgsize;
    /* Fill in the entry */
    current->cmsg_len = sizeof(WSACMSGHDR) + len;
    current->cmsg_level = level;
    current->cmsg_type = type;
    memcpy(ptr, data, len);
    /* Return the pointer to where next entry should go */
    return (WSACMSGHDR *) (ptr + WSA_CMSG_ALIGN(len));
}

static inline int convert_control_headers(struct msghdr *hdr, WSABUF *control)
{
#ifdef IP_PKTINFO
    WSACMSGHDR *cmsg_win = (WSACMSGHDR *) control->buf, *ptr;
    ULONG ctlsize = control->len;
    struct cmsghdr *cmsg_unix;

    ptr = cmsg_win;
    /* Loop over all the headers, converting as appropriate */
    for (cmsg_unix = CMSG_FIRSTHDR(hdr); cmsg_unix != NULL; cmsg_unix = CMSG_NXTHDR(hdr, cmsg_unix))
    {
        switch(cmsg_unix->cmsg_level)
        {
            case IPPROTO_IP:
                switch(cmsg_unix->cmsg_type)
                {
                    case IP_PKTINFO:
                    {
                        /* Convert the Unix IP_PKTINFO structure to the Windows version */
                        struct in_pktinfo *data_unix = (struct in_pktinfo *) CMSG_DATA(cmsg_unix);
                        struct WS_in_pktinfo data_win;

                        memcpy(&data_win.ipi_addr,&data_unix->ipi_addr.s_addr,4); /* 4 bytes = 32 address bits */
                        data_win.ipi_ifindex = data_unix->ipi_ifindex;
                        ptr = fill_control_message(WS_IPPROTO_IP, WS_IP_PKTINFO, ptr, &ctlsize,
                                                   (void*)&data_win, sizeof(data_win));
                        if (!ptr) goto error;
                    }   break;
                    default:
                        FIXME("Unhandled IPPROTO_IP message header type %d\n", cmsg_unix->cmsg_type);
                        break;
                }
                break;
            default:
                FIXME("Unhandled message header level %d\n", cmsg_unix->cmsg_level);
                break;
        }
    }

error:
    /* Set the length of the returned control headers */
    control->len = (ptr == NULL ? 0 : (char*)ptr - (char*)cmsg_win);
    return (ptr != NULL);
#else /* IP_PKTINFO */
    control->len = 0;
    return 1;
#endif /* IP_PKTINFO */
}
#endif /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

/* ----------------------------------- error handling */

static NTSTATUS sock_get_ntstatus( int err )
{
    switch ( err )
    {
        case EBADF:             return STATUS_INVALID_HANDLE;
        case EBUSY:             return STATUS_DEVICE_BUSY;
        case EPERM:
        case EACCES:            return STATUS_ACCESS_DENIED;
        case EFAULT:            return STATUS_NO_MEMORY;
        case EINVAL:            return STATUS_INVALID_PARAMETER;
        case ENFILE:
        case EMFILE:            return STATUS_TOO_MANY_OPENED_FILES;
        case EWOULDBLOCK:       return STATUS_CANT_WAIT;
        case EINPROGRESS:       return STATUS_PENDING;
        case EALREADY:          return STATUS_NETWORK_BUSY;
        case ENOTSOCK:          return STATUS_OBJECT_TYPE_MISMATCH;
        case EDESTADDRREQ:      return STATUS_INVALID_PARAMETER;
        case EMSGSIZE:          return STATUS_BUFFER_OVERFLOW;
        case EPROTONOSUPPORT:
        case ESOCKTNOSUPPORT:
        case EPFNOSUPPORT:
        case EAFNOSUPPORT:
        case EPROTOTYPE:        return STATUS_NOT_SUPPORTED;
        case ENOPROTOOPT:       return STATUS_INVALID_PARAMETER;
        case EOPNOTSUPP:        return STATUS_NOT_SUPPORTED;
        case EADDRINUSE:        return STATUS_ADDRESS_ALREADY_ASSOCIATED;
        case EADDRNOTAVAIL:     return STATUS_INVALID_PARAMETER;
        case ECONNREFUSED:      return STATUS_CONNECTION_REFUSED;
        case ESHUTDOWN:         return STATUS_PIPE_DISCONNECTED;
        case ENOTCONN:          return STATUS_CONNECTION_DISCONNECTED;
        case ETIMEDOUT:         return STATUS_IO_TIMEOUT;
        case ENETUNREACH:       return STATUS_NETWORK_UNREACHABLE;
        case ENETDOWN:          return STATUS_NETWORK_BUSY;
        case EPIPE:
        case ECONNRESET:        return STATUS_CONNECTION_RESET;
        case ECONNABORTED:      return STATUS_CONNECTION_ABORTED;

        case 0:                 return STATUS_SUCCESS;
        default:
            WARN("Unknown errno %d!\n", err);
            return STATUS_UNSUCCESSFUL;
    }
}

static UINT sock_get_error( int err )
{
	switch(err)
    {
	case EINTR:		return WSAEINTR;
	case EBADF:		return WSAEBADF;
	case EPERM:
	case EACCES:		return WSAEACCES;
	case EFAULT:		return WSAEFAULT;
	case EINVAL:		return WSAEINVAL;
	case EMFILE:		return WSAEMFILE;
	case EWOULDBLOCK:	return WSAEWOULDBLOCK;
	case EINPROGRESS:	return WSAEINPROGRESS;
	case EALREADY:		return WSAEALREADY;
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

/* most ws2 overlapped functions return an ntstatus-based error code */
static NTSTATUS wsaErrStatus(void)
{
    int	loc_errno = errno;
    WARN("errno %d, (%s).\n", loc_errno, strerror(loc_errno));

    return sock_get_ntstatus(loc_errno);
}

static UINT wsaHerrno(int loc_errno)
{
    WARN("h_errno %d.\n", loc_errno);

    switch(loc_errno)
    {
	case HOST_NOT_FOUND:	return WSAHOST_NOT_FOUND;
	case TRY_AGAIN:		return WSATRY_AGAIN;
	case NO_RECOVERY:	return WSANO_RECOVERY;
	case NO_DATA:		return WSANO_DATA;
	case ENOBUFS:		return WSAENOBUFS;

	case 0:			return 0;
	default:
		WARN("Unknown h_errno %d!\n", loc_errno);
		return WSAEOPNOTSUPP;
    }
}

static inline DWORD NtStatusToWSAError( const DWORD status )
{
    /* We only need to cover the status codes set by server async request handling */
    DWORD wserr;
    switch ( status )
    {
    case STATUS_SUCCESS:                    wserr = 0;                     break;
    case STATUS_PENDING:                    wserr = WSA_IO_PENDING;        break;
    case STATUS_OBJECT_TYPE_MISMATCH:       wserr = WSAENOTSOCK;           break;
    case STATUS_INVALID_HANDLE:             wserr = WSAEBADF;              break;
    case STATUS_INVALID_PARAMETER:          wserr = WSAEINVAL;             break;
    case STATUS_PIPE_DISCONNECTED:          wserr = WSAESHUTDOWN;          break;
    case STATUS_NETWORK_BUSY:               wserr = WSAEALREADY;           break;
    case STATUS_NETWORK_UNREACHABLE:        wserr = WSAENETUNREACH;        break;
    case STATUS_CONNECTION_REFUSED:         wserr = WSAECONNREFUSED;       break;
    case STATUS_CONNECTION_DISCONNECTED:    wserr = WSAENOTCONN;           break;
    case STATUS_CONNECTION_RESET:           wserr = WSAECONNRESET;         break;
    case STATUS_CONNECTION_ABORTED:         wserr = WSAECONNABORTED;       break;
    case STATUS_CANCELLED:                  wserr = WSA_OPERATION_ABORTED; break;
    case STATUS_ADDRESS_ALREADY_ASSOCIATED: wserr = WSAEADDRINUSE;         break;
    case STATUS_IO_TIMEOUT:
    case STATUS_TIMEOUT:                    wserr = WSAETIMEDOUT;          break;
    case STATUS_NO_MEMORY:                  wserr = WSAEFAULT;             break;
    case STATUS_ACCESS_DENIED:              wserr = WSAEACCES;             break;
    case STATUS_TOO_MANY_OPENED_FILES:      wserr = WSAEMFILE;             break;
    case STATUS_CANT_WAIT:                  wserr = WSAEWOULDBLOCK;        break;
    case STATUS_BUFFER_OVERFLOW:            wserr = WSAEMSGSIZE;           break;
    case STATUS_NOT_SUPPORTED:              wserr = WSAEOPNOTSUPP;         break;
    case STATUS_HOST_UNREACHABLE:           wserr = WSAEHOSTUNREACH;       break;

    default:
        wserr = RtlNtStatusToDosError( status );
        FIXME( "Status code %08x converted to DOS error code %x\n", status, wserr );
    }
    return wserr;
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
    wine_server_release_fd( SOCKET2HANDLE(s), fd );
}

static void _enable_event( HANDLE s, unsigned int event,
                           unsigned int sstate, unsigned int cstate )
{
    SERVER_START_REQ( enable_socket_event )
    {
        req->handle = wine_server_obj_handle( s );
        req->mask   = event;
        req->sstate = sstate;
        req->cstate = cstate;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

static NTSTATUS _is_blocking(SOCKET s, BOOL *ret)
{
    NTSTATUS status;
    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = wine_server_obj_handle( SOCKET2HANDLE(s) );
        req->service = FALSE;
        req->c_event = 0;
        status = wine_server_call( req );
        *ret = (reply->state & FD_WINE_NONBLOCKING) == 0;
    }
    SERVER_END_REQ;
    return status;
}

static unsigned int _get_sock_mask(SOCKET s)
{
    unsigned int ret;
    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = wine_server_obj_handle( SOCKET2HANDLE(s) );
        req->service = FALSE;
        req->c_event = 0;
        wine_server_call( req );
        ret = reply->mask;
    }
    SERVER_END_REQ;
    return ret;
}

static void _sync_sock_state(SOCKET s)
{
    BOOL dummy;
    /* do a dummy wineserver request in order to let
       the wineserver run through its select loop once */
    (void)_is_blocking(s, &dummy);
}

static int _get_sock_error(SOCKET s, unsigned int bit)
{
    int events[FD_MAX_EVENTS];

    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = wine_server_obj_handle( SOCKET2HANDLE(s) );
        req->service = FALSE;
        req->c_event = 0;
        wine_server_set_reply( req, events, sizeof(events) );
        wine_server_call( req );
    }
    SERVER_END_REQ;
    return events[bit];
}

static struct per_thread_data *get_per_thread_data(void)
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

    /* delete scratch buffers */
    HeapFree( GetProcessHeap(), 0, ptb->he_buffer );
    HeapFree( GetProcessHeap(), 0, ptb->se_buffer );
    HeapFree( GetProcessHeap(), 0, ptb->pe_buffer );
    ptb->he_buffer = NULL;
    ptb->se_buffer = NULL;
    ptb->pe_buffer = NULL;

    HeapFree( GetProcessHeap(), 0, ptb );
    NtCurrentTeb()->WinSockData = NULL;
}

/***********************************************************************
 *		DllMain (WS2_32.init)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hInstDLL, fdwReason, fImpLoad);
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        if (fImpLoad) break;
        free_per_thread_data();
        DeleteCriticalSection(&csWSgetXXXbyYYY);
        break;
    case DLL_THREAD_DETACH:
        free_per_thread_data();
        break;
    }
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
        for(i=0; i<sizeof(ws_sock_map)/sizeof(ws_sock_map[0]); i++) {
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
        for(i=0; i<sizeof(ws_tcp_map)/sizeof(ws_tcp_map[0]); i++) {
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
        for(i=0; i<sizeof(ws_ip_map)/sizeof(ws_ip_map[0]); i++) {
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
        for(i=0; i<sizeof(ws_ipv6_map)/sizeof(ws_ipv6_map[0]); i++) {
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

/* ----------------------------------- Per-thread info (or per-process?) */

static char *strdup_lower(const char *str)
{
    int i;
    char *ret = HeapAlloc( GetProcessHeap(), 0, strlen(str) + 1 );

    if (ret)
    {
        for (i = 0; str[i]; i++) ret[i] = tolower(str[i]);
        ret[i] = 0;
    }
    else SetLastError(WSAENOBUFS);
    return ret;
}

static inline int sock_error_p(int s)
{
    unsigned int optval;
    socklen_t optlen = sizeof(optval);
    getsockopt(s, SOL_SOCKET, SO_ERROR, (void *) &optval, &optlen);
    if (optval) WARN("\t[%i] error: %d\n", s, optval);
    return optval != 0;
}

/* Utility: get the SO_RCVTIMEO or SO_SNDTIMEO socket option
 * from an fd and return the value converted to milli seconds
 * or -1 if there is an infinite time out */
static inline int get_rcvsnd_timeo( int fd, int optname)
{
  struct timeval tv;
  socklen_t len = sizeof(tv);
  int ret = getsockopt(fd, SOL_SOCKET, optname, &tv, &len);
  if( ret >= 0)
      ret = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  if( ret <= 0 ) /* tv == {0,0} means infinite time out */
      return -1;
  return ret;
}

/* macro wrappers for portability */
#ifdef SO_RCVTIMEO
#define GET_RCVTIMEO(fd) get_rcvsnd_timeo( (fd), SO_RCVTIMEO)
#else
#define GET_RCVTIMEO(fd) (-1)
#endif

#ifdef SO_SNDTIMEO
#define GET_SNDTIMEO(fd) get_rcvsnd_timeo( (fd), SO_SNDTIMEO)
#else
#define GET_SNDTIMEO(fd) (-1)
#endif

/* utility: given an fd, will block until one of the events occurs */
static inline int do_block( int fd, int events, int timeout )
{
  struct pollfd pfd;
  int ret;

  pfd.fd = fd;
  pfd.events = events;

  while ((ret = poll(&pfd, 1, timeout)) < 0)
  {
      if (errno != EINTR)
          return -1;
  }
  if( ret == 0 )
      return 0;
  return pfd.revents;
}

static int
convert_af_w2u(int windowsaf) {
    unsigned int i;

    for (i=0;i<sizeof(ws_af_map)/sizeof(ws_af_map[0]);i++)
    	if (ws_af_map[i][0] == windowsaf)
	    return ws_af_map[i][1];
    FIXME("unhandled Windows address family %d\n", windowsaf);
    return -1;
}

static int
convert_af_u2w(int unixaf) {
    unsigned int i;

    for (i=0;i<sizeof(ws_af_map)/sizeof(ws_af_map[0]);i++)
    	if (ws_af_map[i][1] == unixaf)
	    return ws_af_map[i][0];
    FIXME("unhandled UNIX address family %d\n", unixaf);
    return -1;
}

static int
convert_proto_w2u(int windowsproto) {
    unsigned int i;

    for (i=0;i<sizeof(ws_proto_map)/sizeof(ws_proto_map[0]);i++)
    	if (ws_proto_map[i][0] == windowsproto)
	    return ws_proto_map[i][1];

    /* check for extended IPX */
    if (IS_IPX_PROTO(windowsproto))
      return windowsproto;

    FIXME("unhandled Windows socket protocol %d\n", windowsproto);
    return -1;
}

static int
convert_proto_u2w(int unixproto) {
    unsigned int i;

    for (i=0;i<sizeof(ws_proto_map)/sizeof(ws_proto_map[0]);i++)
    	if (ws_proto_map[i][1] == unixproto)
	    return ws_proto_map[i][0];

    /* if value is inside IPX range just return it - the kernel simply
     * echoes the value used in the socket() function */
    if (IS_IPX_PROTO(unixproto))
      return unixproto;

    FIXME("unhandled UNIX socket protocol %d\n", unixproto);
    return -1;
}

static int
convert_socktype_w2u(int windowssocktype) {
    unsigned int i;

    for (i=0;i<sizeof(ws_socktype_map)/sizeof(ws_socktype_map[0]);i++)
    	if (ws_socktype_map[i][0] == windowssocktype)
	    return ws_socktype_map[i][1];
    FIXME("unhandled Windows socket type %d\n", windowssocktype);
    return -1;
}

static int
convert_socktype_u2w(int unixsocktype) {
    unsigned int i;

    for (i=0;i<sizeof(ws_socktype_map)/sizeof(ws_socktype_map[0]);i++)
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

    TRACE("succeeded\n");
    return 0;
}


/***********************************************************************
 *      WSACleanup			(WS2_32.116)
 */
INT WINAPI WSACleanup(void)
{
    if (num_startup) {
        num_startup--;
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

static struct WS_hostent *check_buffer_he(int size)
{
    struct per_thread_data * ptb = get_per_thread_data();
    if (ptb->he_buffer)
    {
        if (ptb->he_len >= size ) return ptb->he_buffer;
        HeapFree( GetProcessHeap(), 0, ptb->he_buffer );
    }
    ptb->he_buffer = HeapAlloc( GetProcessHeap(), 0, (ptb->he_len = size) );
    if (!ptb->he_buffer) SetLastError(WSAENOBUFS);
    return ptb->he_buffer;
}

static struct WS_servent *check_buffer_se(int size)
{
    struct per_thread_data * ptb = get_per_thread_data();
    if (ptb->se_buffer)
    {
        if (ptb->se_len >= size ) return ptb->se_buffer;
        HeapFree( GetProcessHeap(), 0, ptb->se_buffer );
    }
    ptb->se_buffer = HeapAlloc( GetProcessHeap(), 0, (ptb->se_len = size) );
    if (!ptb->se_buffer) SetLastError(WSAENOBUFS);
    return ptb->se_buffer;
}

static struct WS_protoent *check_buffer_pe(int size)
{
    struct per_thread_data * ptb = get_per_thread_data();
    if (ptb->pe_buffer)
    {
        if (ptb->pe_len >= size ) return ptb->pe_buffer;
        HeapFree( GetProcessHeap(), 0, ptb->pe_buffer );
    }
    ptb->pe_buffer = HeapAlloc( GetProcessHeap(), 0, (ptb->pe_len = size) );
    if (!ptb->pe_buffer) SetLastError(WSAENOBUFS);
    return ptb->pe_buffer;
}

/* ----------------------------------- i/o APIs */

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

static inline BOOL supported_protocol(int protocol)
{
    int i;
    for (i = 0; i < sizeof(valid_protocols) / sizeof(valid_protocols[0]); i++)
        if (protocol == valid_protocols[i])
            return TRUE;
    return FALSE;
}

/**********************************************************************/

/* Returns the length of the converted address if successful, 0 if it was too small to
 * start with.
 */
static unsigned int ws_sockaddr_ws2u(const struct WS_sockaddr* wsaddr, int wsaddrlen,
                                     union generic_unix_sockaddr *uaddr)
{
    unsigned int uaddrlen = 0;

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
        if (wsaddrlen >= sizeof(struct WS_sockaddr_in6_old)) {
            uaddrlen = sizeof(struct sockaddr_in6);
            memset( uaddr, 0, uaddrlen );
            uin6->sin6_family   = AF_INET6;
            uin6->sin6_port     = win6->sin6_port;
            uin6->sin6_flowinfo = win6->sin6_flowinfo;
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
            if (wsaddrlen >= sizeof(struct WS_sockaddr_in6)) uin6->sin6_scope_id = win6->sin6_scope_id;
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

static BOOL is_sockaddr_bound(const struct sockaddr *uaddr, int uaddrlen)
{
    switch (uaddr->sa_family)
    {
#ifdef HAS_IPX
        case AF_IPX:
        {
            static const struct sockaddr_ipx emptyAddr;
            struct sockaddr_ipx *ipx = (struct sockaddr_ipx*) uaddr;
            return ipx->sipx_port
            || memcmp(&ipx->sipx_network, &emptyAddr.sipx_network, sizeof(emptyAddr.sipx_network))
            || memcmp(&ipx->sipx_node, &emptyAddr.sipx_node, sizeof(emptyAddr.sipx_node));
        }
#endif
        case AF_INET6:
        {
            static const struct sockaddr_in6 emptyAddr;
            const struct sockaddr_in6 *in6 = (const struct sockaddr_in6*) uaddr;
            return in6->sin6_port || memcmp(&in6->sin6_addr, &emptyAddr.sin6_addr, sizeof(struct in6_addr));
        }
        case AF_INET:
        {
            static const struct sockaddr_in emptyAddr;
            const struct sockaddr_in *in = (const struct sockaddr_in*) uaddr;
            return in->sin_port || memcmp(&in->sin_addr, &emptyAddr.sin_addr, sizeof(struct in_addr));
        }
        case AF_UNSPEC:
            return FALSE;
        default:
            FIXME("unknown address family %d\n", uaddr->sa_family);
            return TRUE;
    }
}

/* Returns 0 if successful, -1 if the buffer is too small */
static int ws_sockaddr_u2ws(const struct sockaddr* uaddr, struct WS_sockaddr* wsaddr, int* wsaddrlen)
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
        struct WS_sockaddr_in6_old* win6old = (struct WS_sockaddr_in6_old*)wsaddr;

        if (*wsaddrlen < sizeof(struct WS_sockaddr_in6_old))
            return -1;
        win6old->sin6_family   = WS_AF_INET6;
        win6old->sin6_port     = uin6->sin6_port;
        win6old->sin6_flowinfo = uin6->sin6_flowinfo;
        memcpy(&win6old->sin6_addr,&uin6->sin6_addr,16); /* 16 bytes = 128 address bits */
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
        if (*wsaddrlen >= sizeof(struct WS_sockaddr_in6)) {
            struct WS_sockaddr_in6* win6 = (struct WS_sockaddr_in6*)wsaddr;
            win6->sin6_scope_id = uin6->sin6_scope_id;
            *wsaddrlen = sizeof(struct WS_sockaddr_in6);
        }
        else
            *wsaddrlen = sizeof(struct WS_sockaddr_in6_old);
#else
        *wsaddrlen = sizeof(struct WS_sockaddr_in6_old);
#endif
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

/*****************************************************************************
 *          WS_EnterSingleProtocolW [internal]
 *
 *    enters the protocol information of one given protocol into the given
 *    buffer.
 *
 * RETURNS
 *    TRUE if a protocol was entered into the buffer.
 *
 * BUGS
 *    - only implemented for IPX, SPX, SPXII, TCP, UDP
 *    - there is no check that the operating system supports the returned
 *      protocols
 */
static BOOL WS_EnterSingleProtocolW( INT protocol, WSAPROTOCOL_INFOW* info )
{
    memset( info, 0, sizeof(WSAPROTOCOL_INFOW) );
    info->iProtocol = protocol;

    switch (protocol)
    {
    case WS_IPPROTO_TCP:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_EXPEDITED_DATA |
                                XP1_GRACEFUL_CLOSE | XP1_GUARANTEED_ORDER |
                                XP1_GUARANTEED_DELIVERY;
        info->ProviderId = ProviderIdIP;
        info->dwCatalogEntryId = 0x3e9;
        info->dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_INET;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x10;
        info->iSocketType = WS_SOCK_STREAM;
        strcpyW( info->szProtocol, NameTcpW );
        break;

    case WS_IPPROTO_UDP:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST |
                                XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED |
                                XP1_CONNECTIONLESS;
        info->ProviderId = ProviderIdIP;
        info->dwCatalogEntryId = 0x3ea;
        info->dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_INET;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x10;
        info->iSocketType = WS_SOCK_DGRAM;
        info->dwMessageSize = 0xffbb;
        strcpyW( info->szProtocol, NameUdpW );
        break;

    case WS_NSPROTO_IPX:
        info->dwServiceFlags1 = XP1_PARTIAL_MESSAGE | XP1_SUPPORT_BROADCAST |
                                XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED |
                                XP1_CONNECTIONLESS;
        info->ProviderId = ProviderIdIPX;
        info->dwCatalogEntryId = 0x406;
        info->dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = WS_SOCK_DGRAM;
        info->iProtocolMaxOffset = 0xff;
        info->dwMessageSize = 0x240;
        strcpyW( info->szProtocol, NameIpxW );
        break;

    case WS_NSPROTO_SPX:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_PSEUDO_STREAM |
                                XP1_MESSAGE_ORIENTED | XP1_GUARANTEED_ORDER |
                                XP1_GUARANTEED_DELIVERY;
        info->ProviderId = ProviderIdSPX;
        info->dwCatalogEntryId = 0x407;
        info->dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = WS_SOCK_SEQPACKET;
        info->dwMessageSize = 0xffffffff;
        strcpyW( info->szProtocol, NameSpxW );
        break;

    case WS_NSPROTO_SPXII:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_GRACEFUL_CLOSE |
                                XP1_PSEUDO_STREAM | XP1_MESSAGE_ORIENTED |
                                XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY;
        info->ProviderId = ProviderIdSPX;
        info->dwCatalogEntryId = 0x409;
        info->dwProviderFlags = PFL_MATCHES_PROTOCOL_ZERO;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = WS_SOCK_SEQPACKET;
        info->dwMessageSize = 0xffffffff;
        strcpyW( info->szProtocol, NameSpxIIW );
        break;

    default:
        FIXME("unknown Protocol <0x%08x>\n", protocol);
        return FALSE;
    }
    return TRUE;
}

/*****************************************************************************
 *          WS_EnterSingleProtocolA [internal]
 *
 *    see function WS_EnterSingleProtocolW
 *
 */
static BOOL WS_EnterSingleProtocolA( INT protocol, WSAPROTOCOL_INFOA* info )
{
    WSAPROTOCOL_INFOW infow;
    INT ret;
    memset( info, 0, sizeof(WSAPROTOCOL_INFOA) );

    ret = WS_EnterSingleProtocolW( protocol, &infow );
    if (ret)
    {
        /* convert the structure from W to A */
        memcpy( info, &infow, FIELD_OFFSET( WSAPROTOCOL_INFOA, szProtocol ) );
        WideCharToMultiByte( CP_ACP, 0, infow.szProtocol, -1,
                             info->szProtocol, WSAPROTOCOL_LEN+1, NULL, NULL );
    }

    return ret;
}

static INT WS_EnumProtocols( BOOL unicode, const INT *protocols, LPWSAPROTOCOL_INFOW buffer, LPDWORD len )
{
    INT i = 0, items = 0;
    DWORD size = 0;
    union _info
    {
      LPWSAPROTOCOL_INFOA a;
      LPWSAPROTOCOL_INFOW w;
    } info;
    info.w = buffer;

    if (!protocols) protocols = valid_protocols;

    while (protocols[i])
    {
        if(supported_protocol(protocols[i++]))
            items++;
    }

    size = items * (unicode ? sizeof(WSAPROTOCOL_INFOW) : sizeof(WSAPROTOCOL_INFOA));

    TRACE("unicode %d, protocols %p, buffer %p, length %p %d, items %d, required %d\n",
          unicode, protocols, buffer, len, len ? *len : 0, items, size);

    if (*len < size || !buffer)
    {
        *len = size;
        WSASetLastError(WSAENOBUFS);
        return SOCKET_ERROR;
    }

    for (i = items = 0; protocols[i]; i++)
    {
        if (unicode)
        {
            if (WS_EnterSingleProtocolW( protocols[i], &info.w[items] ))
                items++;
        }
        else
        {
            if (WS_EnterSingleProtocolA( protocols[i], &info.a[items] ))
                items++;
        }
    }
    return items;
}

static BOOL ws_protocol_info(SOCKET s, int unicode, WSAPROTOCOL_INFOW *buffer, int *size)
{
    NTSTATUS status;

    *size = unicode ? sizeof(WSAPROTOCOL_INFOW) : sizeof(WSAPROTOCOL_INFOA);
    memset(buffer, 0, *size);

    SERVER_START_REQ( get_socket_info )
    {
        req->handle  = wine_server_obj_handle( SOCKET2HANDLE(s) );
        status = wine_server_call( req );
        if (!status)
        {
            buffer->iAddressFamily = convert_af_u2w(reply->family);
            buffer->iSocketType = convert_socktype_u2w(reply->type);
            buffer->iProtocol = convert_proto_u2w(reply->protocol);
        }
    }
    SERVER_END_REQ;

    if (status)
    {
        unsigned int err = NtStatusToWSAError( status );
        SetLastError( err == WSAEBADF ? WSAENOTSOCK : err );
        return FALSE;
    }

    if (unicode)
        WS_EnterSingleProtocolW( buffer->iProtocol, buffer);
    else
        WS_EnterSingleProtocolA( buffer->iProtocol, (WSAPROTOCOL_INFOA *)buffer);

    return TRUE;
}

/**************************************************************************
 * Functions for handling overlapped I/O
 **************************************************************************/

/* user APC called upon async completion */
static void WINAPI ws2_async_apc( void *arg, IO_STATUS_BLOCK *iosb, ULONG reserved )
{
    ws2_async *wsa = arg;

    if (wsa->completion_func) wsa->completion_func( NtStatusToWSAError(iosb->u.Status),
                                                    iosb->Information, wsa->user_overlapped,
                                                    wsa->flags );
    HeapFree( GetProcessHeap(), 0, wsa );
}

/***********************************************************************
 *              WS2_recv                (INTERNAL)
 *
 * Workhorse for both synchronous and asynchronous recv() operations.
 */
static int WS2_recv( int fd, struct ws2_async *wsa )
{
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    char pktbuf[512];
#endif
    struct msghdr hdr;
    union generic_unix_sockaddr unix_sockaddr;
    int n;

    hdr.msg_name = NULL;

    if (wsa->addr)
    {
        hdr.msg_namelen = sizeof(unix_sockaddr);
        hdr.msg_name = &unix_sockaddr;
    }
    else
        hdr.msg_namelen = 0;

    hdr.msg_iov = wsa->iovec + wsa->first_iovec;
    hdr.msg_iovlen = wsa->n_iovecs - wsa->first_iovec;
#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    hdr.msg_accrights = NULL;
    hdr.msg_accrightslen = 0;
#else
    hdr.msg_control = pktbuf;
    hdr.msg_controllen = sizeof(pktbuf);
    hdr.msg_flags = 0;
#endif

    while ((n = recvmsg(fd, &hdr, wsa->flags)) == -1)
    {
        if (errno != EINTR)
            return -1;
    }

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    if (wsa->control)
    {
        ERR("Message control headers cannot be properly supported on this system.\n");
        wsa->control->len = 0;
    }
#else
    if (wsa->control && !convert_control_headers(&hdr, wsa->control))
    {
        WARN("Application passed insufficient room for control headers.\n");
        *wsa->lpFlags |= WS_MSG_CTRUNC;
        errno = EMSGSIZE;
        return -1;
    }
#endif

    /* if this socket is connected and lpFrom is not NULL, Linux doesn't give us
     * msg_name and msg_namelen from recvmsg, but it does set msg_namelen to zero.
     *
     * quoting linux 2.6 net/ipv4/tcp.c:
     *  "According to UNIX98, msg_name/msg_namelen are ignored
     *  on connected socket. I was just happy when found this 8) --ANK"
     *
     * likewise MSDN says that lpFrom and lpFromlen are ignored for
     * connection-oriented sockets, so don't try to update lpFrom.
     */
    if (wsa->addr && hdr.msg_namelen)
        ws_sockaddr_u2ws( &unix_sockaddr.addr, wsa->addr, wsa->addrlen.ptr );

    return n;
}

/***********************************************************************
 *              WS2_async_recv          (INTERNAL)
 *
 * Handler for overlapped recv() operations.
 */
static NTSTATUS WS2_async_recv( void* user, IO_STATUS_BLOCK* iosb, NTSTATUS status, void **apc)
{
    ws2_async* wsa = user;
    int result = 0, fd;

    switch (status)
    {
    case STATUS_ALERTED:
        if ((status = wine_server_handle_to_fd( wsa->hSocket, FILE_READ_DATA, &fd, NULL ) ))
            break;

        result = WS2_recv( fd, wsa );
        wine_server_release_fd( wsa->hSocket, fd );
        if (result >= 0)
        {
            status = STATUS_SUCCESS;
            _enable_event( wsa->hSocket, FD_READ, 0, 0 );
        }
        else
        {
            if (errno == EAGAIN)
            {
                status = STATUS_PENDING;
                _enable_event( wsa->hSocket, FD_READ, 0, 0 );
            }
            else
            {
                result = 0;
                status = wsaErrStatus();
            }
        }
        break;
    }
    if (status != STATUS_PENDING)
    {
        iosb->u.Status = status;
        iosb->Information = result;
        *apc = ws2_async_apc;
    }
    return status;
}

/* user APC called upon async accept completion */
static void WINAPI ws2_async_accept_apc( void *arg, IO_STATUS_BLOCK *iosb, ULONG reserved )
{
    struct ws2_accept_async *wsa = arg;

    HeapFree( GetProcessHeap(), 0, wsa->read );
    HeapFree( GetProcessHeap(), 0, wsa );
}

/***********************************************************************
 *              WS2_async_accept_recv            (INTERNAL)
 *
 * This function is used to finish the read part of an accept request. It is
 * needed to place the completion on the correct socket (listener).
 */
static NTSTATUS WS2_async_accept_recv( void *arg, IO_STATUS_BLOCK *iosb, NTSTATUS status, void **apc )
{
    void *junk;
    struct ws2_accept_async *wsa = arg;

    status = WS2_async_recv( wsa->read, iosb, status, &junk );
    if (status == STATUS_PENDING)
        return status;

    if (wsa->user_overlapped->hEvent)
        SetEvent(wsa->user_overlapped->hEvent);
    if (wsa->cvalue)
        WS_AddCompletion( HANDLE2SOCKET(wsa->listen_socket), wsa->cvalue, iosb->u.Status, iosb->Information );

    *apc = ws2_async_accept_apc;
    return status;
}

/***********************************************************************
 *              WS2_async_accept                (INTERNAL)
 *
 * This is the function called to satisfy the AcceptEx callback
 */
static NTSTATUS WS2_async_accept( void *arg, IO_STATUS_BLOCK *iosb, NTSTATUS status, void **apc )
{
    struct ws2_accept_async *wsa = arg;
    int len;
    char *addr;

    TRACE("status: 0x%x listen: %p, accept: %p\n", status, wsa->listen_socket, wsa->accept_socket);

    if (status == STATUS_ALERTED)
    {
        SERVER_START_REQ( accept_into_socket )
        {
            req->lhandle = wine_server_obj_handle( wsa->listen_socket );
            req->ahandle = wine_server_obj_handle( wsa->accept_socket );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;

        if (status == STATUS_CANT_WAIT)
            return STATUS_PENDING;

        if (status == STATUS_INVALID_HANDLE)
        {
            FIXME("AcceptEx accepting socket closed but request was not cancelled\n");
            status = STATUS_CANCELLED;
        }
    }
    else if (status == STATUS_HANDLES_CLOSED)
        status = STATUS_CANCELLED;  /* strange windows behavior */

    if (status != STATUS_SUCCESS)
        goto finish;

    /* WS2 Spec says size param is extra 16 bytes long...what do we put in it? */
    addr = ((char *)wsa->buf) + wsa->data_len;
    len = wsa->local_len - sizeof(int);
    WS_getsockname(HANDLE2SOCKET(wsa->accept_socket),
                   (struct WS_sockaddr *)(addr + sizeof(int)), &len);
    *(int *)addr = len;

    addr += wsa->local_len;
    len = wsa->remote_len - sizeof(int);
    WS_getpeername(HANDLE2SOCKET(wsa->accept_socket),
                   (struct WS_sockaddr *)(addr + sizeof(int)), &len);
    *(int *)addr = len;

    if (!wsa->read)
        goto finish;

    SERVER_START_REQ( register_async )
    {
        req->type           = ASYNC_TYPE_READ;
        req->async.handle   = wine_server_obj_handle( wsa->accept_socket );
        req->async.callback = wine_server_client_ptr( WS2_async_accept_recv );
        req->async.iosb     = wine_server_client_ptr( iosb );
        req->async.arg      = wine_server_client_ptr( wsa );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING)
        goto finish;

    /* The APC has finished but no completion should be sent for the operation yet, additional processing
     * needs to be performed by WS2_async_accept_recv() first. */
    return STATUS_MORE_PROCESSING_REQUIRED;

finish:
    iosb->u.Status = status;
    iosb->Information = 0;

    if (wsa->user_overlapped->hEvent)
        SetEvent(wsa->user_overlapped->hEvent);

    *apc = ws2_async_accept_apc;
    return status;
}

/***********************************************************************
 *              WS2_send                (INTERNAL)
 *
 * Workhorse for both synchronous and asynchronous send() operations.
 */
static int WS2_send( int fd, struct ws2_async *wsa )
{
    struct msghdr hdr;
    union generic_unix_sockaddr unix_addr;
    int n, ret;

    hdr.msg_name = NULL;
    hdr.msg_namelen = 0;

    if (wsa->addr)
    {
        hdr.msg_name = &unix_addr;
        hdr.msg_namelen = ws_sockaddr_ws2u( wsa->addr, wsa->addrlen.val, &unix_addr );
        if ( !hdr.msg_namelen )
        {
            errno = EFAULT;
            return -1;
        }

#if defined(HAS_IPX) && defined(SOL_IPX)
        if(wsa->addr->sa_family == WS_AF_IPX)
        {
            struct sockaddr_ipx* uipx = (struct sockaddr_ipx*)hdr.msg_name;
            int val=0;
            socklen_t len = sizeof(int);

            /* The packet type is stored at the ipx socket level; At least the linux kernel seems
             *  to do something with it in case hdr.msg_name is NULL. Nonetheless can we use it to store
             *  the packet type and then we can retrieve it using getsockopt. After that we can set the
             *  ipx type in the sockaddr_opx structure with the stored value.
             */
            if(getsockopt(fd, SOL_IPX, IPX_TYPE, &val, &len) != -1)
                uipx->sipx_type = val;
        }
#endif
    }

    hdr.msg_iov = wsa->iovec + wsa->first_iovec;
    hdr.msg_iovlen = wsa->n_iovecs - wsa->first_iovec;
#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    hdr.msg_accrights = NULL;
    hdr.msg_accrightslen = 0;
#else
    hdr.msg_control = NULL;
    hdr.msg_controllen = 0;
    hdr.msg_flags = 0;
#endif

    while ((ret = sendmsg(fd, &hdr, wsa->flags)) == -1)
    {
        if (errno != EINTR)
            return -1;
    }

    n = ret;
    while (wsa->first_iovec < wsa->n_iovecs && wsa->iovec[wsa->first_iovec].iov_len <= n)
        n -= wsa->iovec[wsa->first_iovec++].iov_len;
    if (wsa->first_iovec < wsa->n_iovecs)
    {
        wsa->iovec[wsa->first_iovec].iov_base = (char*)wsa->iovec[wsa->first_iovec].iov_base + n;
        wsa->iovec[wsa->first_iovec].iov_len -= n;
    }
    return ret;
}

/***********************************************************************
 *              WS2_async_send          (INTERNAL)
 *
 * Handler for overlapped send() operations.
 */
static NTSTATUS WS2_async_send(void* user, IO_STATUS_BLOCK* iosb, NTSTATUS status, void **apc)
{
    ws2_async* wsa = user;
    int result = 0, fd;

    switch (status)
    {
    case STATUS_ALERTED:
        if ( wsa->n_iovecs <= wsa->first_iovec )
        {
            /* Nothing to do */
            status = STATUS_SUCCESS;
            break;
        }
        if ((status = wine_server_handle_to_fd( wsa->hSocket, FILE_WRITE_DATA, &fd, NULL ) ))
            break;

        /* check to see if the data is ready (non-blocking) */
        result = WS2_send( fd, wsa );
        wine_server_release_fd( wsa->hSocket, fd );

        if (result >= 0)
        {
            if (wsa->first_iovec < wsa->n_iovecs)
                status = STATUS_PENDING;
            else
                status = STATUS_SUCCESS;

            iosb->Information += result;
        }
        else if (errno == EAGAIN)
        {
            status = STATUS_PENDING;
        }
        else
        {
            status = wsaErrStatus();
        }
        break;
    }
    if (status != STATUS_PENDING)
    {
        iosb->u.Status = status;
        *apc = ws2_async_apc;
    }
    return status;
}

/***********************************************************************
 *              WS2_async_shutdown      (INTERNAL)
 *
 * Handler for shutdown() operations on overlapped sockets.
 */
static NTSTATUS WS2_async_shutdown( void* user, PIO_STATUS_BLOCK iosb, NTSTATUS status, void **apc )
{
    ws2_async* wsa = user;
    int fd, err = 1;

    switch (status)
    {
    case STATUS_ALERTED:
        if ((status = wine_server_handle_to_fd( wsa->hSocket, 0, &fd, NULL ) ))
            break;

        switch ( wsa->type )
        {
        case ASYNC_TYPE_READ:   err = shutdown( fd, 0 );  break;
        case ASYNC_TYPE_WRITE:  err = shutdown( fd, 1 );  break;
        }
        status = err ? wsaErrStatus() : STATUS_SUCCESS;
        wine_server_release_fd( wsa->hSocket, fd );
        break;
    }
    iosb->u.Status = status;
    iosb->Information = 0;
    *apc = ws2_async_apc;
    return status;
}

/***********************************************************************
 *  WS2_register_async_shutdown         (INTERNAL)
 *
 * Helper function for WS_shutdown() on overlapped sockets.
 */
static int WS2_register_async_shutdown( SOCKET s, int type )
{
    struct ws2_async *wsa;
    NTSTATUS status;

    TRACE("s %ld type %d\n", s, type);

    wsa = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( struct ws2_async, iovec[1] ));
    if ( !wsa )
        return WSAEFAULT;

    wsa->hSocket         = SOCKET2HANDLE(s);
    wsa->type            = type;
    wsa->completion_func = NULL;

    SERVER_START_REQ( register_async )
    {
        req->type   = type;
        req->async.handle   = wine_server_obj_handle( wsa->hSocket );
        req->async.callback = wine_server_client_ptr( WS2_async_shutdown );
        req->async.iosb     = wine_server_client_ptr( &wsa->local_iosb );
        req->async.arg      = wine_server_client_ptr( wsa );
        req->async.cvalue   = 0;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING)
    {
        HeapFree( GetProcessHeap(), 0, wsa );
        return NtStatusToWSAError( status );
    }
    return 0;
}

/***********************************************************************
 *		accept		(WS2_32.1)
 */
SOCKET WINAPI WS_accept(SOCKET s, struct WS_sockaddr *addr, int *addrlen32)
{
    NTSTATUS status;
    SOCKET as;
    BOOL is_blocking;

    TRACE("socket %04lx\n", s );
    status = _is_blocking(s, &is_blocking);
    if (status)
    {
        set_error(status);
        return INVALID_SOCKET;
    }

    do {
        /* try accepting first (if there is a deferred connection) */
        SERVER_START_REQ( accept_socket )
        {
            req->lhandle    = wine_server_obj_handle( SOCKET2HANDLE(s) );
            req->access     = GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE;
            req->attributes = OBJ_INHERIT;
            status = wine_server_call( req );
            as = HANDLE2SOCKET( wine_server_ptr_handle( reply->handle ));
        }
        SERVER_END_REQ;
        if (!status)
        {
            if (addr && WS_getpeername(as, addr, addrlen32))
            {
                WS_closesocket(as);
                return SOCKET_ERROR;
            }
            return as;
        }
        if (is_blocking && status == STATUS_CANT_WAIT)
        {
            int fd = get_sock_fd( s, FILE_READ_DATA, NULL );
            /* block here */
            do_block(fd, POLLIN, -1);
            _sync_sock_state(s); /* let wineserver notice connection */
            release_sock_fd( s, fd );
        }
    } while (is_blocking && status == STATUS_CANT_WAIT);

    set_error(status);
    return INVALID_SOCKET;
}

/***********************************************************************
 *     AcceptEx
 */
static BOOL WINAPI WS2_AcceptEx(SOCKET listener, SOCKET acceptor, PVOID dest, DWORD dest_len,
                         DWORD local_addr_len, DWORD rem_addr_len, LPDWORD received,
                         LPOVERLAPPED overlapped)
{
    DWORD status;
    struct ws2_accept_async *wsa;
    int fd;
    ULONG_PTR cvalue = (overlapped && ((ULONG_PTR)overlapped->hEvent & 1) == 0) ? (ULONG_PTR)overlapped : 0;

    TRACE("(%lx, %lx, %p, %d, %d, %d, %p, %p)\n", listener, acceptor, dest, dest_len, local_addr_len,
                                                  rem_addr_len, received, overlapped);

    if (!dest)
    {
        SetLastError(WSAEINVAL);
        return FALSE;
    }

    if (!overlapped)
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return FALSE;
    }

    if (!rem_addr_len)
    {
        SetLastError(WSAEFAULT);
        return FALSE;
    }

    fd = get_sock_fd( listener, FILE_READ_DATA, NULL );
    if (fd == -1)
    {
        SetLastError(WSAENOTSOCK);
        return FALSE;
    }
    release_sock_fd( listener, fd );

    fd = get_sock_fd( acceptor, FILE_READ_DATA, NULL );
    if (fd == -1)
    {
        SetLastError(WSAENOTSOCK);
        return FALSE;
    }
    release_sock_fd( acceptor, fd );

    wsa = HeapAlloc( GetProcessHeap(), 0, sizeof(*wsa) );
    if(!wsa)
    {
        SetLastError(WSAEFAULT);
        return FALSE;
    }

    wsa->listen_socket   = SOCKET2HANDLE(listener);
    wsa->accept_socket   = SOCKET2HANDLE(acceptor);
    wsa->user_overlapped = overlapped;
    wsa->cvalue          = cvalue;
    wsa->buf             = dest;
    wsa->data_len        = dest_len;
    wsa->local_len       = local_addr_len;
    wsa->remote_len      = rem_addr_len;
    wsa->read            = NULL;

    if (wsa->data_len)
    {
        /* set up a read request if we need it */
        wsa->read = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET(struct ws2_async, iovec[1]) );
        if (!wsa->read)
        {
            HeapFree( GetProcessHeap(), 0, wsa );
            SetLastError(WSAEFAULT);
            return FALSE;
        }

        wsa->read->hSocket     = wsa->accept_socket;
        wsa->read->flags       = 0;
        wsa->read->lpFlags     = &wsa->read->flags;
        wsa->read->addr        = NULL;
        wsa->read->addrlen.ptr = NULL;
        wsa->read->control     = NULL;
        wsa->read->n_iovecs    = 1;
        wsa->read->first_iovec = 0;
        wsa->read->iovec[0].iov_base = wsa->buf;
        wsa->read->iovec[0].iov_len  = wsa->data_len;
    }

    SERVER_START_REQ( register_async )
    {
        req->type           = ASYNC_TYPE_READ;
        req->async.handle   = wine_server_obj_handle( SOCKET2HANDLE(listener) );
        req->async.callback = wine_server_client_ptr( WS2_async_accept );
        req->async.iosb     = wine_server_client_ptr( overlapped );
        req->async.arg      = wine_server_client_ptr( wsa );
        req->async.cvalue   = cvalue;
        /* We don't set event since we may also have to read */
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if(status != STATUS_PENDING)
    {
        HeapFree( GetProcessHeap(), 0, wsa->read );
        HeapFree( GetProcessHeap(), 0, wsa );
    }

    SetLastError( NtStatusToWSAError(status) );
    return FALSE;
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
 *               interface_bind         (INTERNAL)
 *
 * Take bind() calls on any name corresponding to a local network adapter and restrict the given socket to
 * operating only on the specified interface.  This restriction consists of two components:
 *  1) An outgoing packet restriction suggesting the egress interface for all packets.
 *  2) An incoming packet restriction dropping packets not meant for the interface.
 * If the function succeeds in placing these restrictions (returns TRUE) then the name for the bind() may
 * safely be changed to INADDR_ANY, permitting the transmission and receipt of broadcast packets on the
 * socket. This behavior is only relevant to UDP sockets and is needed for applications that expect to be able
 * to receive broadcast packets on a socket that is bound to a specific network interface.
 */
static BOOL interface_bind( SOCKET s, int fd, struct sockaddr *addr )
{
    struct sockaddr_in *in_sock = (struct sockaddr_in *) addr;
    unsigned int sock_type = 0;
    socklen_t optlen = sizeof(sock_type);
    in_addr_t bind_addr = in_sock->sin_addr.s_addr;
    PIP_ADAPTER_INFO adapters = NULL, adapter;
    BOOL ret = FALSE;
    DWORD adap_size;
    int enable = 1;

    if (bind_addr == htonl(INADDR_ANY) || bind_addr == htonl(INADDR_LOOPBACK))
        return FALSE; /* Not binding to a network adapter, special interface binding unnecessary. */
    if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &sock_type, &optlen) == -1 || sock_type != SOCK_DGRAM)
        return FALSE; /* Special interface binding is only necessary for UDP datagrams. */
    if (GetAdaptersInfo(NULL, &adap_size) != ERROR_BUFFER_OVERFLOW)
        goto cleanup;
    adapters = HeapAlloc(GetProcessHeap(), 0, adap_size);
    if (adapters == NULL || GetAdaptersInfo(adapters, &adap_size) != NO_ERROR)
        goto cleanup;
    /* Search the IPv4 adapter list for the appropriate binding interface */
    for (adapter = adapters; adapter != NULL; adapter = adapter->Next)
    {
        in_addr_t adapter_addr = (in_addr_t) inet_addr(adapter->IpAddressList.IpAddress.String);

        if (bind_addr == adapter_addr)
        {
#if defined(IP_BOUND_IF)
            /* IP_BOUND_IF sets both the incoming and outgoing restriction at once */
            if (setsockopt(fd, IPPROTO_IP, IP_BOUND_IF, &adapter->Index, sizeof(adapter->Index)) != 0)
                goto cleanup;
            ret = TRUE;
#elif defined(LINUX_BOUND_IF)
            in_addr_t ifindex = (in_addr_t) htonl(adapter->Index);
            struct interface_filter specific_interface_filter;
            struct sock_fprog filter_prog;

            if (setsockopt(fd, IPPROTO_IP, IP_UNICAST_IF, &ifindex, sizeof(ifindex)) != 0)
                goto cleanup; /* Failed to suggest egress interface */
            specific_interface_filter = generic_interface_filter;
            specific_interface_filter.iface_rule.k = adapter->Index;
            specific_interface_filter.ip_rule.k = htonl(adapter_addr);
            filter_prog.len = sizeof(generic_interface_filter)/sizeof(struct sock_filter);
            filter_prog.filter = (struct sock_filter *) &specific_interface_filter;
            if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter_prog, sizeof(filter_prog)) != 0)
                goto cleanup; /* Failed to specify incoming packet filter */
            ret = TRUE;
#else
            FIXME("Broadcast packets on interface-bound sockets are not currently supported on this platform, "
                  "receiving broadcast packets will not work on socket %04lx.\n", s);
#endif
            break;
        }
    }
    /* Will soon be switching to INADDR_ANY: permit address reuse */
    if (ret && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == 0)
        TRACE("Socket %04lx bound to interface index %d\n", s, adapter->Index);
    else
        ret = FALSE;

cleanup:
    if(!ret)
        ERR("Failed to bind to interface, receiving broadcast packets will not work on socket %04lx.\n", s);
    HeapFree(GetProcessHeap(), 0, adapters);
    return ret;
}

/***********************************************************************
 *		bind			(WS2_32.2)
 */
int WINAPI WS_bind(SOCKET s, const struct WS_sockaddr* name, int namelen)
{
    int fd = get_sock_fd( s, 0, NULL );
    int res = SOCKET_ERROR;

    TRACE("socket %04lx, ptr %p %s, length %d\n", s, name, debugstr_sockaddr(name), namelen);

    if (fd != -1)
    {
        if (!name || (name->sa_family && !supported_pf(name->sa_family)))
        {
            SetLastError(WSAEAFNOSUPPORT);
        }
        else
        {
            union generic_unix_sockaddr uaddr;
            unsigned int uaddrlen = ws_sockaddr_ws2u(name, namelen, &uaddr);
            if (!uaddrlen)
            {
                SetLastError(WSAEFAULT);
            }
            else
            {
#ifdef IPV6_V6ONLY
                const struct sockaddr_in6 *in6 = (const struct sockaddr_in6*) &uaddr;
                if (name->sa_family == WS_AF_INET6 &&
                    !memcmp(&in6->sin6_addr, &in6addr_any, sizeof(struct in6_addr)))
                {
                    int enable = 1;
                    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &enable, sizeof(enable)) == -1)
                    {
                        release_sock_fd( s, fd );
                        SetLastError(WSAEAFNOSUPPORT);
                        return SOCKET_ERROR;
                    }
                }
#endif
                if (name->sa_family == WS_AF_INET)
                {
                    struct sockaddr_in *in4 = (struct sockaddr_in*) &uaddr;
                    if (memcmp(&in4->sin_addr, magic_loopback_addr, 4) == 0)
                    {
                        /* Trying to bind to the default host interface, using
                         * INADDR_ANY instead*/
                        WARN("Trying to bind to magic IP address, using "
                             "INADDR_ANY instead.\n");
                        in4->sin_addr.s_addr = htonl(INADDR_ANY);
                    }
                    else if (interface_bind(s, fd, &uaddr.addr))
                        in4->sin_addr.s_addr = htonl(INADDR_ANY);
                }
                if (bind(fd, &uaddr.addr, uaddrlen) < 0)
                {
                    int loc_errno = errno;
                    WARN("\tfailure - errno = %i\n", errno);
                    errno = loc_errno;
                    switch (errno)
                    {
                    case EBADF:
                        SetLastError(WSAENOTSOCK);
                        break;
                    case EADDRNOTAVAIL:
                        SetLastError(WSAEINVAL);
                        break;
                    case EADDRINUSE:
                    {
                        int optval = 0;
                        socklen_t optlen = sizeof(optval);
                        /* Windows >= 2003 will return different results depending on
                         * SO_REUSEADDR, WSAEACCES may be returned representing that
                         * the socket hijacking protection prevented the bind */
                        if (!getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, &optlen) && optval)
                        {
                            SetLastError(WSAEACCES);
                            break;
                        }
                        /* fall through */
                    }
                    default:
                        SetLastError(wsaErrno());
                        break;
                    }
                }
                else
                {
                    res=0; /* success */
                }
            }
        }
        release_sock_fd( s, fd );
    }
    return res;
}

/***********************************************************************
 *		closesocket		(WS2_32.3)
 */
int WINAPI WS_closesocket(SOCKET s)
{
    TRACE("socket %04lx\n", s);
    if (CloseHandle(SOCKET2HANDLE(s))) return 0;
    return SOCKET_ERROR;
}

static int do_connect(int fd, const struct WS_sockaddr* name, int namelen)
{
    union generic_unix_sockaddr uaddr;
    unsigned int uaddrlen = ws_sockaddr_ws2u(name, namelen, &uaddr);

    if (!uaddrlen)
        return WSAEFAULT;

    if (name->sa_family == WS_AF_INET)
    {
        struct sockaddr_in *in4 = (struct sockaddr_in*) &uaddr;
        if (memcmp(&in4->sin_addr, magic_loopback_addr, 4) == 0)
        {
            /* Trying to connect to magic replace-loopback address,
                * assuming we really want to connect to localhost */
            TRACE("Trying to connect to magic IP address, using "
                    "INADDR_LOOPBACK instead.\n");
            in4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        }
    }

    if (connect(fd, &uaddr.addr, uaddrlen) == 0)
        return 0;

    return wsaErrno();
}

/***********************************************************************
 *		connect		(WS2_32.4)
 */
int WINAPI WS_connect(SOCKET s, const struct WS_sockaddr* name, int namelen)
{
    int fd = get_sock_fd( s, FILE_READ_DATA, NULL );

    TRACE("socket %04lx, ptr %p %s, length %d\n", s, name, debugstr_sockaddr(name), namelen);

    if (fd != -1)
    {
        NTSTATUS status;
        BOOL is_blocking;
        int ret = do_connect(fd, name, namelen);
        if (ret == 0)
            goto connect_success;

        if (ret == WSAEINPROGRESS)
        {
            /* tell wineserver that a connection is in progress */
            _enable_event(SOCKET2HANDLE(s), FD_CONNECT|FD_READ|FD_WRITE,
                          FD_CONNECT,
                          FD_WINE_CONNECTED|FD_WINE_LISTENING);
            status = _is_blocking( s, &is_blocking );
            if (status)
            {
                release_sock_fd( s, fd );
                set_error( status );
                return SOCKET_ERROR;
            }
            if (is_blocking)
            {
                int result;
                /* block here */
                do_block(fd, POLLIN | POLLOUT, -1);
                _sync_sock_state(s); /* let wineserver notice connection */
                /* retrieve any error codes from it */
                result = _get_sock_error(s, FD_CONNECT_BIT);
                if (result)
                    SetLastError(NtStatusToWSAError(result));
                else
                {
                    goto connect_success;
                }
            }
            else
            {
                SetLastError(WSAEWOULDBLOCK);
            }
        }
        else
        {
            SetLastError(ret);
        }
        release_sock_fd( s, fd );
    }
    return SOCKET_ERROR;

connect_success:
    release_sock_fd( s, fd );
    _enable_event(SOCKET2HANDLE(s), FD_CONNECT|FD_READ|FD_WRITE,
                  FD_WINE_CONNECTED|FD_READ|FD_WRITE,
                  FD_CONNECT|FD_WINE_LISTENING);
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

/***********************************************************************
 *             ConnectEx
 */
static BOOL WINAPI WS2_ConnectEx(SOCKET s, const struct WS_sockaddr* name, int namelen,
                          PVOID sendBuf, DWORD sendBufLen, LPDWORD sent, LPOVERLAPPED ov)
{
    int fd, ret, status;
    union generic_unix_sockaddr uaddr;
    socklen_t uaddrlen = sizeof(uaddr);

    if (!ov)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    fd = get_sock_fd( s, FILE_READ_DATA, NULL );
    if (fd == -1)
    {
        SetLastError( WSAENOTSOCK );
        return FALSE;
    }

    TRACE("socket %04lx, ptr %p %s, length %d, sendptr %p, len %d, ov %p\n",
          s, name, debugstr_sockaddr(name), namelen, sendBuf, sendBufLen, ov);

    if (getsockname(fd, &uaddr.addr, &uaddrlen) != 0)
    {
        SetLastError(wsaErrno());
        return FALSE;
    }
    else if (!is_sockaddr_bound(&uaddr.addr, uaddrlen))
    {
        SetLastError(WSAEINVAL);
        return FALSE;
    }

    ret = do_connect(fd, name, namelen);
    if (ret == 0)
    {
        WSABUF wsabuf;

        _enable_event(SOCKET2HANDLE(s), FD_CONNECT|FD_READ|FD_WRITE,
                            FD_WINE_CONNECTED|FD_READ|FD_WRITE,
                            FD_CONNECT|FD_WINE_LISTENING);

        wsabuf.len = sendBufLen;
        wsabuf.buf = (char*) sendBuf;

        /* WSASend takes care of completion if need be */
        if (WSASend(s, &wsabuf, sendBuf ? 1 : 0, sent, 0, ov, NULL) != SOCKET_ERROR)
            goto connection_success;
    }
    else if (ret == WSAEINPROGRESS)
    {
        struct ws2_async *wsa;
        ULONG_PTR cvalue = (((ULONG_PTR)ov->hEvent & 1) == 0) ? (ULONG_PTR)ov : 0;

        _enable_event(SOCKET2HANDLE(s), FD_CONNECT|FD_READ|FD_WRITE,
                      FD_CONNECT,
                      FD_WINE_CONNECTED|FD_WINE_LISTENING);

        /* Indirectly call WSASend */
        if (!(wsa = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( struct ws2_async, iovec[1] ))))
        {
            SetLastError(WSAEFAULT);
        }
        else
        {
            IO_STATUS_BLOCK *iosb = (IO_STATUS_BLOCK *)ov;
            iosb->u.Status = STATUS_PENDING;
            iosb->Information = 0;

            wsa->hSocket     = SOCKET2HANDLE(s);
            wsa->addr        = NULL;
            wsa->addrlen.val = 0;
            wsa->flags       = 0;
            wsa->lpFlags     = &wsa->flags;
            wsa->control     = NULL;
            wsa->n_iovecs    = sendBuf ? 1 : 0;
            wsa->first_iovec = 0;
            wsa->completion_func = NULL;
            wsa->iovec[0].iov_base = sendBuf;
            wsa->iovec[0].iov_len  = sendBufLen;

            SERVER_START_REQ( register_async )
            {
                req->type           = ASYNC_TYPE_WRITE;
                req->async.handle   = wine_server_obj_handle( wsa->hSocket );
                req->async.callback = wine_server_client_ptr( WS2_async_send );
                req->async.iosb     = wine_server_client_ptr( iosb );
                req->async.arg      = wine_server_client_ptr( wsa );
                req->async.event    = wine_server_obj_handle( ov->hEvent );
                req->async.cvalue   = cvalue;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;

            if (status != STATUS_PENDING) HeapFree(GetProcessHeap(), 0, wsa);

            /* If the connect already failed */
            if (status == STATUS_PIPE_DISCONNECTED)
                status = _get_sock_error(s, FD_CONNECT_BIT);
            SetLastError( NtStatusToWSAError(status) );
        }
    }
    else
    {
        SetLastError(ret);
    }

    release_sock_fd( s, fd );
    return FALSE;

connection_success:
    release_sock_fd( s, fd );
    return TRUE;
}


/***********************************************************************
 *		getpeername		(WS2_32.5)
 */
int WINAPI WS_getpeername(SOCKET s, struct WS_sockaddr *name, int *namelen)
{
    int fd;
    int res;

    TRACE("socket: %04lx, ptr %p, len %08x\n", s, name, namelen ? *namelen : 0);

    fd = get_sock_fd( s, 0, NULL );
    res = SOCKET_ERROR;

    if (fd != -1)
    {
        union generic_unix_sockaddr uaddr;
        socklen_t uaddrlen = sizeof(uaddr);

        if (getpeername(fd, &uaddr.addr, &uaddrlen) == 0)
        {
            if (!name || !namelen)
                SetLastError(WSAEFAULT);
            else if (ws_sockaddr_u2ws(&uaddr.addr, name, namelen) != 0)
                /* The buffer was too small */
                SetLastError(WSAEFAULT);
            else
                res = 0;
        }
        else
            SetLastError(wsaErrno());
        release_sock_fd( s, fd );
    }
    return res;
}

/***********************************************************************
 *		getsockname		(WS2_32.6)
 */
int WINAPI WS_getsockname(SOCKET s, struct WS_sockaddr *name, int *namelen)
{
    int fd;
    int res;

    TRACE("socket: %04lx, ptr %p, len %08x\n", s, name, namelen ? *namelen : 0);

    /* Check if what we've received is valid. Should we use IsBadReadPtr? */
    if( (name == NULL) || (namelen == NULL) )
    {
        SetLastError( WSAEFAULT );
        return SOCKET_ERROR;
    }

    fd = get_sock_fd( s, 0, NULL );
    res = SOCKET_ERROR;

    if (fd != -1)
    {
        union generic_unix_sockaddr uaddr;
        socklen_t uaddrlen = sizeof(uaddr);

        if (getsockname(fd, &uaddr.addr, &uaddrlen) != 0)
        {
            SetLastError(wsaErrno());
        }
        else if (!is_sockaddr_bound(&uaddr.addr, uaddrlen))
        {
            SetLastError(WSAEINVAL);
        }
        else if (ws_sockaddr_u2ws(&uaddr.addr, name, namelen) != 0)
        {
            /* The buffer was too small */
            SetLastError(WSAEFAULT);
        }
        else
        {
            res=0;
            TRACE("=> %s\n", debugstr_sockaddr(name));
        }
        release_sock_fd( s, fd );
    }
    return res;
}

/***********************************************************************
 *		getsockopt		(WS2_32.7)
 */
INT WINAPI WS_getsockopt(SOCKET s, INT level,
                                  INT optname, char *optval, INT *optlen)
{
    int fd;
    INT ret = 0;

    TRACE("socket: %04lx, level 0x%x, name 0x%x, ptr %p, len %d\n",
          s, level, optname, optval, optlen ? *optlen : 0);

    switch(level)
    {
    case WS_SOL_SOCKET:
    {
        switch(optname)
        {
        /* Handle common cases. The special cases are below, sorted
         * alphabetically */
        case WS_SO_BROADCAST:
        case WS_SO_DEBUG:
        case WS_SO_ERROR:
        case WS_SO_KEEPALIVE:
        case WS_SO_OOBINLINE:
        case WS_SO_RCVBUF:
        case WS_SO_REUSEADDR:
        case WS_SO_SNDBUF:
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;
            convert_sockopt(&level, &optname);
            if (getsockopt(fd, level, optname, optval, (socklen_t *)optlen) != 0 )
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            release_sock_fd( s, fd );
            return ret;
        case WS_SO_ACCEPTCONN:
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;
            if (getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, optval, (socklen_t *)optlen) != 0 )
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            else
            {
                /* BSD returns != 0 while Windows return exact == 1 */
                if (*(int *)optval) *(int *)optval = 1;
            }
            release_sock_fd( s, fd );
            return ret;
        case WS_SO_DONTLINGER:
        {
            struct linger lingval;
            socklen_t len = sizeof(struct linger);

            if (!optlen || *optlen < sizeof(BOOL)|| !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;

            if (getsockopt(fd, SOL_SOCKET, SO_LINGER, &lingval, &len) != 0 )
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            else
            {
                *(BOOL *)optval = !lingval.l_onoff;
                *optlen = sizeof(BOOL);
            }

            release_sock_fd( s, fd );
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

        case WS_SO_LINGER:
        {
            struct linger lingval;
            int so_type;
            socklen_t len = sizeof(struct linger), slen = sizeof(int);

            /* struct linger and LINGER have different sizes */
            if (!optlen || *optlen < sizeof(LINGER) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;

            if ((getsockopt(fd, SOL_SOCKET, SO_TYPE, &so_type, &slen) == 0 && so_type == SOCK_DGRAM))
            {
                SetLastError(WSAENOPROTOOPT);
                ret = SOCKET_ERROR;
            }
            else if (getsockopt(fd, SOL_SOCKET, SO_LINGER, &lingval, &len) != 0)
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            else
            {
                ((LINGER *)optval)->l_onoff = lingval.l_onoff;
                ((LINGER *)optval)->l_linger = lingval.l_linger;
                *optlen = sizeof(struct linger);
            }

            release_sock_fd( s, fd );
            return ret;
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
#ifdef SO_RCVTIMEO
        case WS_SO_RCVTIMEO:
#endif
#ifdef SO_SNDTIMEO
        case WS_SO_SNDTIMEO:
#endif
#if defined(SO_RCVTIMEO) || defined(SO_SNDTIMEO)
        {
            struct timeval tv;
            socklen_t len = sizeof(struct timeval);

            if (!optlen || *optlen < sizeof(int)|| !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;

            convert_sockopt(&level, &optname);
            if (getsockopt(fd, level, optname, &tv, &len) != 0 )
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            else
            {
                *(int *)optval = tv.tv_sec * 1000 + tv.tv_usec / 1000;
                *optlen = sizeof(int);
            }

            release_sock_fd( s, fd );
            return ret;
        }
#endif
        case WS_SO_TYPE:
        {
            if (!optlen || *optlen < sizeof(int) || !optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;

            if (getsockopt(fd, SOL_SOCKET, SO_TYPE, optval, (socklen_t *)optlen) != 0 )
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            else
                (*(int *)optval) = convert_socktype_u2w(*(int *)optval);

            release_sock_fd( s, fd );
            return ret;
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
        case IPX_PTYPE:
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

        case IPX_MAX_ADAPTER_NUM:
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
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
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
        case WS_IP_ADD_MEMBERSHIP:
        case WS_IP_DROP_MEMBERSHIP:
#ifdef IP_HDRINCL
        case WS_IP_HDRINCL:
#endif
        case WS_IP_MULTICAST_IF:
        case WS_IP_MULTICAST_LOOP:
        case WS_IP_MULTICAST_TTL:
        case WS_IP_OPTIONS:
#ifdef IP_PKTINFO
        case WS_IP_PKTINFO:
#endif
        case WS_IP_TOS:
        case WS_IP_TTL:
#ifdef IP_UNICAST_IF
        case WS_IP_UNICAST_IF:
#endif
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;
            convert_sockopt(&level, &optname);
            if (getsockopt(fd, level, optname, optval, (socklen_t *)optlen) != 0 )
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            release_sock_fd( s, fd );
            return ret;
        case WS_IP_DONTFRAGMENT:
            FIXME("WS_IP_DONTFRAGMENT is always false!\n");
            *(BOOL*)optval = FALSE;
            return 0;
        }
        FIXME("Unknown IPPROTO_IP optname 0x%08x\n", optname);
        return SOCKET_ERROR;

    case WS_IPPROTO_IPV6:
        switch(optname)
        {
#ifdef IPV6_ADD_MEMBERSHIP
        case WS_IPV6_ADD_MEMBERSHIP:
#endif
#ifdef IPV6_DROP_MEMBERSHIP
        case WS_IPV6_DROP_MEMBERSHIP:
#endif
        case WS_IPV6_MULTICAST_IF:
        case WS_IPV6_MULTICAST_HOPS:
        case WS_IPV6_MULTICAST_LOOP:
        case WS_IPV6_UNICAST_HOPS:
        case WS_IPV6_V6ONLY:
#ifdef IPV6_UNICAST_IF
        case WS_IPV6_UNICAST_IF:
#endif
            if ( (fd = get_sock_fd( s, 0, NULL )) == -1)
                return SOCKET_ERROR;
            convert_sockopt(&level, &optname);
            if (getsockopt(fd, level, optname, optval, (socklen_t *)optlen) != 0 )
            {
                SetLastError((errno == EBADF) ? WSAENOTSOCK : wsaErrno());
                ret = SOCKET_ERROR;
            }
            release_sock_fd( s, fd );
            return ret;
        case WS_IPV6_DONTFRAG:
            FIXME("WS_IPV6_DONTFRAG is always false!\n");
            *(BOOL*)optval = FALSE;
            return 0;
        }
        FIXME("Unknown IPPROTO_IPV6 optname 0x%08x\n", optname);
        return SOCKET_ERROR;

    default:
        WARN("Unknown level: 0x%08x\n", level);
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    } /* end switch(level) */
}

/***********************************************************************
 *		htonl			(WS2_32.8)
 */
WS_u_long WINAPI WS_htonl(WS_u_long hostlong)
{
    return htonl(hostlong);
}


/***********************************************************************
 *		htons			(WS2_32.9)
 */
WS_u_short WINAPI WS_htons(WS_u_short hostshort)
{
    return htons(hostshort);
}

/***********************************************************************
 *		WSAHtonl		(WS2_32.46)
 *  From MSDN description of error codes, this function should also
 *  check if WinSock has been initialized and the socket is a valid
 *  socket. But why? This function only translates a host byte order
 *  u_long into a network byte order u_long...
 */
int WINAPI WSAHtonl(SOCKET s, WS_u_long hostlong, WS_u_long *lpnetlong)
{
    if (lpnetlong)
    {
        *lpnetlong = htonl(hostlong);
        return 0;
    }
    WSASetLastError(WSAEFAULT);
    return SOCKET_ERROR;
}

/***********************************************************************
 *		WSAHtons		(WS2_32.47)
 *  From MSDN description of error codes, this function should also
 *  check if WinSock has been initialized and the socket is a valid
 *  socket. But why? This function only translates a host byte order
 *  u_short into a network byte order u_short...
 */
int WINAPI WSAHtons(SOCKET s, WS_u_short hostshort, WS_u_short *lpnetshort)
{

    if (lpnetshort)
    {
        *lpnetshort = htons(hostshort);
        return 0;
    }
    WSASetLastError(WSAEFAULT);
    return SOCKET_ERROR;
}


/***********************************************************************
 *		inet_addr		(WS2_32.11)
 */
WS_u_long WINAPI WS_inet_addr(const char *cp)
{
    if (!cp) return INADDR_NONE;
    return inet_addr(cp);
}


/***********************************************************************
 *		ntohl			(WS2_32.14)
 */
WS_u_long WINAPI WS_ntohl(WS_u_long netlong)
{
    return ntohl(netlong);
}


/***********************************************************************
 *		ntohs			(WS2_32.15)
 */
WS_u_short WINAPI WS_ntohs(WS_u_short netshort)
{
    return ntohs(netshort);
}


/***********************************************************************
 *		inet_ntoa		(WS2_32.12)
 */
char* WINAPI WS_inet_ntoa(struct WS_in_addr in)
{
    char* s = inet_ntoa(*((struct in_addr*)&in));
    if( s )
    {
        struct per_thread_data *data = get_per_thread_data();
        strcpy(data->ntoa_buffer, s);
        return data->ntoa_buffer;
    }
    SetLastError(wsaErrno());
    return NULL;
}

static const char *debugstr_wsaioctl(DWORD ioctl)
{
    const char *buf_type, *family;

    switch(ioctl & 0x18000000)
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
        BYTE size = (ioctl >> 16) & WS_IOCPARM_MASK;
        char x = (ioctl & 0xff00) >> 8;
        BYTE y = ioctl & 0xff;
        char args[14];

        switch (ioctl & (WS_IOC_VOID|WS_IOC_INOUT))
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
    switch (ioctl & (WS_IOC_VOID|WS_IOC_INOUT))
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
                            (USHORT)(ioctl & 0xffff));
}

/* do an ioctl call through the server */
static DWORD server_ioctl_sock( SOCKET s, DWORD code, LPVOID in_buff, DWORD in_size,
                                LPVOID out_buff, DWORD out_size, LPDWORD ret_size,
                                LPWSAOVERLAPPED overlapped,
                                LPWSAOVERLAPPED_COMPLETION_ROUTINE completion )
{
    HANDLE event = overlapped ? overlapped->hEvent : 0;
    HANDLE handle = SOCKET2HANDLE( s );
    struct ws2_async *wsa;
    NTSTATUS status;
    PIO_STATUS_BLOCK io;

    if (!(wsa = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*wsa) )))
        return WSA_NOT_ENOUGH_MEMORY;
    wsa->hSocket           = handle;
    wsa->user_overlapped   = overlapped;
    wsa->completion_func   = completion;
    io = (overlapped ? (PIO_STATUS_BLOCK)overlapped : &wsa->local_iosb);

    status = NtDeviceIoControlFile( handle, event, (PIO_APC_ROUTINE)ws2_async_apc, wsa, io, code,
                                    in_buff, in_size, out_buff, out_size );
    if (status == STATUS_NOT_SUPPORTED)
    {
        FIXME("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
    }
    else if (status == STATUS_SUCCESS)
        *ret_size = io->Information; /* "Information" is the size written to the output buffer */

    if (status != STATUS_PENDING) RtlFreeHeap( GetProcessHeap(), 0, wsa );

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
    int fd;
    DWORD status = 0, total = 0;

    TRACE("%ld, %s, %p, %d, %p, %d, %p, %p, %p\n",
          s, debugstr_wsaioctl(code), in_buff, in_size, out_buff, out_size, ret_size, overlapped, completion);

    switch (code)
    {
    case WS_FIONBIO:
        if (in_size != sizeof(WS_u_long) || IS_INTRESOURCE(in_buff))
        {
            WSASetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }
        TRACE("-> FIONBIO (%x)\n", *(WS_u_long*)in_buff);
        if (_get_sock_mask(s))
        {
            /* AsyncSelect()'ed sockets are always nonblocking */
            if (!*(WS_u_long *)in_buff) status = WSAEINVAL;
            break;
        }
        if (*(WS_u_long *)in_buff)
            _enable_event(SOCKET2HANDLE(s), 0, FD_WINE_NONBLOCKING, 0);
        else
            _enable_event(SOCKET2HANDLE(s), 0, 0, FD_WINE_NONBLOCKING);
        break;

    case WS_FIONREAD:
    {
        if (out_size != sizeof(WS_u_long) || IS_INTRESOURCE(out_buff))
        {
            WSASetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }
        if ((fd = get_sock_fd( s, 0, NULL )) == -1) return SOCKET_ERROR;
        if (ioctl(fd, FIONREAD, out_buff ) == -1)
            status = (errno == EBADF) ? WSAENOTSOCK : wsaErrno();
        release_sock_fd( s, fd );
        break;
    }

    case WS_SIOCATMARK:
    {
        unsigned int oob = 0, atmark = 0;
        socklen_t oobsize = sizeof(int);
        if (out_size != sizeof(WS_u_long) || IS_INTRESOURCE(out_buff))
        {
            WSASetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }
        if ((fd = get_sock_fd( s, 0, NULL )) == -1) return SOCKET_ERROR;
        /* SO_OOBINLINE sockets must always return TRUE to SIOCATMARK */
        if ((getsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &oob, &oobsize ) == -1)
           || (!oob && ioctl(fd, SIOCATMARK, &atmark ) == -1))
            status = (errno == EBADF) ? WSAENOTSOCK : wsaErrno();
        else
        {
            /* The SIOCATMARK value read from ioctl() is reversed
             * because BSD returns TRUE if it's in the OOB mark
             * while Windows returns TRUE if there are NO OOB bytes.
             */
            (*(WS_u_long *) out_buff) = oob | !atmark;
        }

        release_sock_fd( s, fd );
        break;
    }

    case WS_FIOASYNC:
        WARN("Warning: WS1.1 shouldn't be using async I/O\n");
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;

   case WS_SIO_GET_INTERFACE_LIST:
       {
           INTERFACE_INFO* intArray = out_buff;
           DWORD size, numInt = 0, apiReturn;

           TRACE("-> SIO_GET_INTERFACE_LIST request\n");

           if (!out_buff || !ret_size)
           {
               WSASetLastError(WSAEFAULT);
               return SOCKET_ERROR;
           }

           fd = get_sock_fd( s, 0, NULL );
           if (fd == -1) return SOCKET_ERROR;

           apiReturn = GetAdaptersInfo(NULL, &size);
           if (apiReturn == ERROR_BUFFER_OVERFLOW)
           {
               PIP_ADAPTER_INFO table = HeapAlloc(GetProcessHeap(),0,size);

               if (table)
               {
                  if (GetAdaptersInfo(table, &size) == NO_ERROR)
                  {
                     PIP_ADAPTER_INFO ptr;

                     for (ptr = table, numInt = 0; ptr; ptr = ptr->Next)
                     {
                        unsigned int addr, mask, bcast;
                        struct ifreq ifInfo;

                        /* Skip interfaces without an IPv4 address. */
                        if (ptr->IpAddressList.IpAddress.String[0] == '\0')
                            continue;

                        if ((numInt + 1)*sizeof(INTERFACE_INFO)/sizeof(IP_ADAPTER_INFO) > out_size)
                        {
                            WARN("Buffer too small = %u, out_size = %u\n", numInt + 1, out_size);
                            status = WSAEFAULT;
                            break;
                        }

                        /* Socket Status Flags */
                        lstrcpynA(ifInfo.ifr_name, ptr->AdapterName, IFNAMSIZ);
                        if (ioctl(fd, SIOCGIFFLAGS, &ifInfo) < 0)
                        {
                           ERR("Error obtaining status flags for socket!\n");
                           status = WSAEINVAL;
                           break;
                        }
                        else
                        {
                           /* set flags; the values of IFF_* are not the same
                              under Linux and Windows, therefore must generate
                              new flags */
                           intArray->iiFlags = 0;
                           if (ifInfo.ifr_flags & IFF_BROADCAST)
                              intArray->iiFlags |= WS_IFF_BROADCAST;
#ifdef IFF_POINTOPOINT
                           if (ifInfo.ifr_flags & IFF_POINTOPOINT)
                              intArray->iiFlags |= WS_IFF_POINTTOPOINT;
#endif
                           if (ifInfo.ifr_flags & IFF_LOOPBACK)
                              intArray->iiFlags |= WS_IFF_LOOPBACK;
                           if (ifInfo.ifr_flags & IFF_UP)
                              intArray->iiFlags |= WS_IFF_UP;
                           if (ifInfo.ifr_flags & IFF_MULTICAST)
                              intArray->iiFlags |= WS_IFF_MULTICAST;
                        }

                        addr = inet_addr(ptr->IpAddressList.IpAddress.String);
                        mask = inet_addr(ptr->IpAddressList.IpMask.String);
                        bcast = addr | ~mask;
                        intArray->iiAddress.AddressIn.sin_family = AF_INET;
                        intArray->iiAddress.AddressIn.sin_port = 0;
                        intArray->iiAddress.AddressIn.sin_addr.WS_s_addr =
                         addr;
                        intArray->iiNetmask.AddressIn.sin_family = AF_INET;
                        intArray->iiNetmask.AddressIn.sin_port = 0;
                        intArray->iiNetmask.AddressIn.sin_addr.WS_s_addr =
                         mask;
                        intArray->iiBroadcastAddress.AddressIn.sin_family =
                         AF_INET;
                        intArray->iiBroadcastAddress.AddressIn.sin_port = 0;
                        intArray->iiBroadcastAddress.AddressIn.sin_addr.
                         WS_s_addr = bcast;
                        intArray++;
                        numInt++;
                     }
                  }
                  else
                  {
                     ERR("Unable to get interface table!\n");
                     status = WSAEINVAL;
                  }
                  HeapFree(GetProcessHeap(),0,table);
               }
               else status = WSAEINVAL;
           }
           else if (apiReturn != ERROR_NO_DATA)
           {
               ERR("Unable to get interface table!\n");
               status = WSAEINVAL;
           }
           /* Calculate the size of the array being returned */
           total = sizeof(INTERFACE_INFO) * numInt;
           release_sock_fd( s, fd );
           break;
       }

   case WS_SIO_ADDRESS_LIST_QUERY:
   {
        DWORD size;

        TRACE("-> SIO_ADDRESS_LIST_QUERY request\n");

        if (!ret_size)
        {
            WSASetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        if (GetAdaptersInfo(NULL, &size) == ERROR_BUFFER_OVERFLOW)
        {
            IP_ADAPTER_INFO *p, *table = HeapAlloc(GetProcessHeap(), 0, size);
            DWORD num;

            if (!table || GetAdaptersInfo(table, &size))
            {
                HeapFree(GetProcessHeap(), 0, table);
                status = WSAEINVAL;
                break;
            }

            for (p = table, num = 0; p; p = p->Next)
                if (p->IpAddressList.IpAddress.String[0]) num++;

            total = sizeof(SOCKET_ADDRESS_LIST) + sizeof(SOCKET_ADDRESS) * (num - 1);
            total += sizeof(SOCKADDR) * num;

            if (total > out_size)
            {
                HeapFree(GetProcessHeap(), 0, table);
                status = WSAEFAULT;
                break;
            }

            if (out_buff)
            {
                unsigned int i;
                SOCKET_ADDRESS *sa;
                SOCKET_ADDRESS_LIST *sa_list = out_buff;
                SOCKADDR_IN *sockaddr;

                sa = sa_list->Address;
                sockaddr = (SOCKADDR_IN *)((char *)sa + num * sizeof(SOCKET_ADDRESS));
                sa_list->iAddressCount = num;

                for (p = table, i = 0; p; p = p->Next)
                {
                    if (!p->IpAddressList.IpAddress.String[0]) continue;

                    sa[i].lpSockaddr = (SOCKADDR *)&sockaddr[i];
                    sa[i].iSockaddrLength = sizeof(SOCKADDR);

                    sockaddr[i].sin_family = AF_INET;
                    sockaddr[i].sin_port = 0;
                    sockaddr[i].sin_addr.WS_s_addr = inet_addr(p->IpAddressList.IpAddress.String);
                    i++;
                }
            }

            HeapFree(GetProcessHeap(), 0, table);
        }
        else
        {
            WARN("unable to get IP address list\n");
            status = WSAEINVAL;
        }
        break;
   }

   case WS_SIO_FLUSH:
	FIXME("SIO_FLUSH: stub.\n");
	break;

   case WS_SIO_GET_EXTENSION_FUNCTION_POINTER:
   {
        static const GUID connectex_guid = WSAID_CONNECTEX;
        static const GUID disconnectex_guid = WSAID_DISCONNECTEX;
        static const GUID acceptex_guid = WSAID_ACCEPTEX;
        static const GUID getaccepexsockaddrs_guid = WSAID_GETACCEPTEXSOCKADDRS;
        static const GUID transmitfile_guid = WSAID_TRANSMITFILE;
        static const GUID transmitpackets_guid = WSAID_TRANSMITPACKETS;
        static const GUID wsarecvmsg_guid = WSAID_WSARECVMSG;
        static const GUID wsasendmsg_guid = WSAID_WSASENDMSG;

        if ( IsEqualGUID(&connectex_guid, in_buff) )
        {
            *(LPFN_CONNECTEX *)out_buff = WS2_ConnectEx;
            break;
        }
        else if ( IsEqualGUID(&disconnectex_guid, in_buff) )
        {
            FIXME("SIO_GET_EXTENSION_FUNCTION_POINTER: unimplemented DisconnectEx\n");
        }
        else if ( IsEqualGUID(&acceptex_guid, in_buff) )
        {
            *(LPFN_ACCEPTEX *)out_buff = WS2_AcceptEx;
            break;
        }
        else if ( IsEqualGUID(&getaccepexsockaddrs_guid, in_buff) )
        {
            *(LPFN_GETACCEPTEXSOCKADDRS *)out_buff = WS2_GetAcceptExSockaddrs;
            break;
        }
        else if ( IsEqualGUID(&transmitfile_guid, in_buff) )
        {
            FIXME("SIO_GET_EXTENSION_FUNCTION_POINTER: unimplemented TransmitFile\n");
        }
        else if ( IsEqualGUID(&transmitpackets_guid, in_buff) )
        {
            FIXME("SIO_GET_EXTENSION_FUNCTION_POINTER: unimplemented TransmitPackets\n");
        }
        else if ( IsEqualGUID(&wsarecvmsg_guid, in_buff) )
        {
            *(LPFN_WSARECVMSG *)out_buff = WS2_WSARecvMsg;
            break;
        }
        else if ( IsEqualGUID(&wsasendmsg_guid, in_buff) )
        {
            *(LPFN_WSASENDMSG *)out_buff = WSASendMsg;
            break;
        }
        else
            FIXME("SIO_GET_EXTENSION_FUNCTION_POINTER %s: stub\n", debugstr_guid(in_buff));

        status = WSAEOPNOTSUPP;
        break;
   }
   case WS_SIO_KEEPALIVE_VALS:
   {
        struct tcp_keepalive *k;
        int keepalive, keepidle, keepintvl;

        if (!in_buff || in_size < sizeof(struct tcp_keepalive))
        {
            WSASetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        k = in_buff;
        keepalive = k->onoff ? 1 : 0;
        keepidle = max( 1, (k->keepalivetime + 500) / 1000 );
        keepintvl = max( 1, (k->keepaliveinterval + 500) / 1000 );

        TRACE("onoff: %d, keepalivetime: %d, keepaliveinterval: %d\n", keepalive, keepidle, keepintvl);

        fd = get_sock_fd(s, 0, NULL);
        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(int)) == -1)
            status = WSAEINVAL;
#if defined(TCP_KEEPIDLE) && defined(TCP_KEEPINTVL)
        /* these values need to be set only if SO_KEEPALIVE is enabled */
        else if(keepalive)
        {
            if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(int)) == -1)
                status = WSAEINVAL;
            else if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof(int)) == -1)
                status = WSAEINVAL;
        }
#else
        else
            FIXME("ignoring keepalive interval and timeout\n");
#endif
        release_sock_fd(s, fd);
        break;
   }
   case WS_SIO_ROUTING_INTERFACE_QUERY:
   {
       struct WS_sockaddr *daddr = (struct WS_sockaddr *)in_buff;
       struct WS_sockaddr_in *daddr_in = (struct WS_sockaddr_in *)daddr;
       struct WS_sockaddr_in *saddr_in = out_buff;
       MIB_IPFORWARDROW row;
       PMIB_IPADDRTABLE ipAddrTable = NULL;
       DWORD size, i, found_index;

       TRACE("-> WS_SIO_ROUTING_INTERFACE_QUERY request\n");

       if (!in_buff || in_size < sizeof(struct WS_sockaddr) ||
           !out_buff || out_size < sizeof(struct WS_sockaddr_in) || !ret_size)
       {
           WSASetLastError(WSAEFAULT);
           return SOCKET_ERROR;
       }
       if (daddr->sa_family != AF_INET)
       {
           FIXME("unsupported address family %d\n", daddr->sa_family);
           status = WSAEAFNOSUPPORT;
           break;
       }
       if (GetBestRoute(daddr_in->sin_addr.S_un.S_addr, 0, &row) != NOERROR ||
           GetIpAddrTable(NULL, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER)
       {
           status = WSAEFAULT;
           break;
       }
       ipAddrTable = HeapAlloc(GetProcessHeap(), 0, size);
       if (GetIpAddrTable(ipAddrTable, &size, FALSE))
       {
           HeapFree(GetProcessHeap(), 0, ipAddrTable);
           status = WSAEFAULT;
           break;
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
           HeapFree(GetProcessHeap(), 0, ipAddrTable);
           status = WSAEFAULT;
           break;
       }
       saddr_in->sin_family = AF_INET;
       saddr_in->sin_addr.S_un.S_addr = ipAddrTable->table[found_index].dwAddr;
       saddr_in->sin_port = 0;
       total = sizeof(struct WS_sockaddr_in);
       HeapFree(GetProcessHeap(), 0, ipAddrTable);
       break;
   }
   case WS_SIO_SET_COMPATIBILITY_MODE:
       TRACE("WS_SIO_SET_COMPATIBILITY_MODE ignored\n");
       status = WSAEOPNOTSUPP;
       break;
   case WS_SIO_UDP_CONNRESET:
       FIXME("WS_SIO_UDP_CONNRESET stub\n");
       break;
    case 0x667e: /* Netscape tries hard to use bogus ioctl 0x667e */
        WSASetLastError(WSAEOPNOTSUPP);
        return SOCKET_ERROR;
    default:
        status = WSAEOPNOTSUPP;
        break;
    }

    if (status == WSAEOPNOTSUPP)
    {
        status = server_ioctl_sock(s, code, in_buff, in_size, out_buff, out_size, &total,
                                   overlapped, completion);
        if (status != WSAEOPNOTSUPP)
        {
            if (status == 0 || status == WSA_IO_PENDING)
                TRACE("-> %s request\n", debugstr_wsaioctl(code));
            else
                ERR("-> %s request failed with status 0x%x\n", debugstr_wsaioctl(code), status);

            /* overlapped and completion operations will be handled by the server */
            completion = NULL;
            overlapped = NULL;
        }
        else
            FIXME("unsupported WS_IOCTL cmd (%s)\n", debugstr_wsaioctl(code));
    }

    if (completion)
    {
        FIXME( "completion routine %p not supported\n", completion );
    }
    else if (overlapped)
    {
        ULONG_PTR cvalue = (overlapped && ((ULONG_PTR)overlapped->hEvent & 1) == 0) ? (ULONG_PTR)overlapped : 0;
        overlapped->Internal = status;
        overlapped->InternalHigh = total;
        if (overlapped->hEvent) NtSetEvent( overlapped->hEvent, NULL );
        if (cvalue) WS_AddCompletion( HANDLE2SOCKET(s), cvalue, status, total );
    }

    if (!status)
    {
        if (ret_size) *ret_size = total;
        return 0;
    }
    SetLastError( status );
    return SOCKET_ERROR;
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
 *		listen		(WS2_32.13)
 */
int WINAPI WS_listen(SOCKET s, int backlog)
{
    int fd = get_sock_fd( s, FILE_READ_DATA, NULL ), ret = SOCKET_ERROR;

    TRACE("socket %04lx, backlog %d\n", s, backlog);
    if (fd != -1)
    {
        union generic_unix_sockaddr uaddr;
        socklen_t uaddrlen = sizeof(uaddr);

        if (getsockname(fd, &uaddr.addr, &uaddrlen) != 0)
        {
            SetLastError(wsaErrno());
        }
        else if (!is_sockaddr_bound(&uaddr.addr, uaddrlen))
        {
            SetLastError(WSAEINVAL);
        }
        else if (listen(fd, backlog) == 0)
        {
            _enable_event(SOCKET2HANDLE(s), FD_ACCEPT,
                          FD_WINE_LISTENING,
                          FD_CONNECT|FD_WINE_CONNECTED);
            ret = 0;
        }
        else
            SetLastError(wsaErrno());
        release_sock_fd( s, fd );
    }
    else
        SetLastError(WSAENOTSOCK);
    return ret;
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

/* allocate a poll array for the corresponding fd sets */
static struct pollfd *fd_sets_to_poll( const WS_fd_set *readfds, const WS_fd_set *writefds,
                                       const WS_fd_set *exceptfds, int *count_ptr )
{
    unsigned int i, j = 0, count = 0;
    struct pollfd *fds;

    if (readfds) count += readfds->fd_count;
    if (writefds) count += writefds->fd_count;
    if (exceptfds) count += exceptfds->fd_count;
    *count_ptr = count;
    if (!count)
    {
        SetLastError(WSAEINVAL);
        return NULL;
    }
    if (!(fds = HeapAlloc( GetProcessHeap(), 0, count * sizeof(fds[0]))))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    if (readfds)
        for (i = 0; i < readfds->fd_count; i++, j++)
        {
            fds[j].fd = get_sock_fd( readfds->fd_array[i], FILE_READ_DATA, NULL );
            if (fds[j].fd == -1) goto failed;
            fds[j].events = POLLIN;
            fds[j].revents = 0;
        }
    if (writefds)
        for (i = 0; i < writefds->fd_count; i++, j++)
        {
            fds[j].fd = get_sock_fd( writefds->fd_array[i], FILE_WRITE_DATA, NULL );
            if (fds[j].fd == -1) goto failed;
            fds[j].events = POLLOUT;
            fds[j].revents = 0;
        }
    if (exceptfds)
        for (i = 0; i < exceptfds->fd_count; i++, j++)
        {
            fds[j].fd = get_sock_fd( exceptfds->fd_array[i], 0, NULL );
            if (fds[j].fd == -1) goto failed;
            fds[j].events = POLLHUP;
            fds[j].revents = 0;
        }
    return fds;

failed:
    count = j;
    j = 0;
    if (readfds)
        for (i = 0; i < readfds->fd_count && j < count; i++, j++)
            release_sock_fd( readfds->fd_array[i], fds[j].fd );
    if (writefds)
        for (i = 0; i < writefds->fd_count && j < count; i++, j++)
            release_sock_fd( writefds->fd_array[i], fds[j].fd );
    if (exceptfds)
        for (i = 0; i < exceptfds->fd_count && j < count; i++, j++)
            release_sock_fd( exceptfds->fd_array[i], fds[j].fd );
    HeapFree( GetProcessHeap(), 0, fds );
    return NULL;
}

/* release the file descriptor obtained in fd_sets_to_poll */
/* must be called with the original fd_set arrays, before calling get_poll_results */
static void release_poll_fds( const WS_fd_set *readfds, const WS_fd_set *writefds,
                              const WS_fd_set *exceptfds, struct pollfd *fds )
{
    unsigned int i, j = 0;

    if (readfds)
    {
        for (i = 0; i < readfds->fd_count; i++, j++)
            if (fds[j].fd != -1) release_sock_fd( readfds->fd_array[i], fds[j].fd );
    }
    if (writefds)
    {
        for (i = 0; i < writefds->fd_count; i++, j++)
            if (fds[j].fd != -1) release_sock_fd( writefds->fd_array[i], fds[j].fd );
    }
    if (exceptfds)
    {
        for (i = 0; i < exceptfds->fd_count; i++, j++)
            if (fds[j].fd != -1)
            {
                /* make sure we have a real error before releasing the fd */
                if (!sock_error_p( fds[j].fd )) fds[j].revents = 0;
                release_sock_fd( exceptfds->fd_array[i], fds[j].fd );
            }
    }
}

/* map the poll results back into the Windows fd sets */
static int get_poll_results( WS_fd_set *readfds, WS_fd_set *writefds, WS_fd_set *exceptfds,
                             const struct pollfd *fds )
{
    unsigned int i, j = 0, k, total = 0;

    if (readfds)
    {
        for (i = k = 0; i < readfds->fd_count; i++, j++)
            if (fds[j].revents) readfds->fd_array[k++] = readfds->fd_array[i];
        readfds->fd_count = k;
        total += k;
    }
    if (writefds)
    {
        for (i = k = 0; i < writefds->fd_count; i++, j++)
            if ((fds[j].revents & POLLOUT) && !(fds[j].revents & POLLHUP))
                writefds->fd_array[k++] = writefds->fd_array[i];
        writefds->fd_count = k;
        total += k;
    }
    if (exceptfds)
    {
        for (i = k = 0; i < exceptfds->fd_count; i++, j++)
            if (fds[j].revents) exceptfds->fd_array[k++] = exceptfds->fd_array[i];
        exceptfds->fd_count = k;
        total += k;
    }
    return total;
}


/***********************************************************************
 *		select			(WS2_32.18)
 */
int WINAPI WS_select(int nfds, WS_fd_set *ws_readfds,
                     WS_fd_set *ws_writefds, WS_fd_set *ws_exceptfds,
                     const struct WS_timeval* ws_timeout)
{
    struct pollfd *pollfds;
    struct timeval tv1, tv2;
    int torig = 0;
    int count, ret, timeout = -1;

    TRACE("read %p, write %p, excp %p timeout %p\n",
          ws_readfds, ws_writefds, ws_exceptfds, ws_timeout);

    if (!(pollfds = fd_sets_to_poll( ws_readfds, ws_writefds, ws_exceptfds, &count )))
        return SOCKET_ERROR;

    if (ws_timeout)
    {
        torig = (ws_timeout->tv_sec * 1000) + (ws_timeout->tv_usec + 999) / 1000;
        timeout = torig;
        gettimeofday( &tv1, 0 );
    }

    while ((ret = poll( pollfds, count, timeout )) < 0)
    {
        if (errno == EINTR)
        {
            if (!ws_timeout) continue;
            gettimeofday( &tv2, 0 );

            tv2.tv_sec  -= tv1.tv_sec;
            tv2.tv_usec -= tv1.tv_usec;
            if (tv2.tv_usec < 0)
            {
                tv2.tv_usec += 1000000;
                tv2.tv_sec  -= 1;
            }

            timeout = torig - (tv2.tv_sec * 1000) - (tv2.tv_usec + 999) / 1000;
            if (timeout <= 0) break;
        } else break;
    }
    release_poll_fds( ws_readfds, ws_writefds, ws_exceptfds, pollfds );

    if (ret == -1) SetLastError(wsaErrno());
    else ret = get_poll_results( ws_readfds, ws_writefds, ws_exceptfds, pollfds );
    HeapFree( GetProcessHeap(), 0, pollfds );
    return ret;
}

/* helper to send completion messages for client-only i/o operation case */
static void WS_AddCompletion( SOCKET sock, ULONG_PTR CompletionValue, NTSTATUS CompletionStatus,
                              ULONG Information )
{
    SERVER_START_REQ( add_fd_completion )
    {
        req->handle      = wine_server_obj_handle( SOCKET2HANDLE(sock) );
        req->cvalue      = CompletionValue;
        req->status      = CompletionStatus;
        req->information = Information;
        wine_server_call( req );
    }
    SERVER_END_REQ;
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


static int WS2_sendto( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                       LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
                       const struct WS_sockaddr *to, int tolen,
                       LPWSAOVERLAPPED lpOverlapped,
                       LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    unsigned int i, options;
    int n, fd, err, overlapped;
    struct ws2_async *wsa = NULL, localwsa;
    int totalLength = 0;
    DWORD bytes_sent;
    BOOL is_blocking;

    TRACE("socket %04lx, wsabuf %p, nbufs %d, flags %d, to %p, tolen %d, ovl %p, func %p\n",
          s, lpBuffers, dwBufferCount, dwFlags,
          to, tolen, lpOverlapped, lpCompletionRoutine);

    fd = get_sock_fd( s, FILE_WRITE_DATA, &options );
    TRACE( "fd=%d, options=%x\n", fd, options );

    if ( fd == -1 ) return SOCKET_ERROR;

    if (!lpOverlapped && !lpNumberOfBytesSent)
    {
        err = WSAEFAULT;
        goto error;
    }

    overlapped = (lpOverlapped || lpCompletionRoutine) &&
        !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));
    if (overlapped || dwBufferCount > 1)
    {
        if (!(wsa = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET(struct ws2_async, iovec[dwBufferCount]) )))
        {
            err = WSAEFAULT;
            goto error;
        }
    }
    else
        wsa = &localwsa;

    wsa->hSocket     = SOCKET2HANDLE(s);
    wsa->addr        = (struct WS_sockaddr *)to;
    wsa->addrlen.val = tolen;
    wsa->flags       = dwFlags;
    wsa->lpFlags     = &wsa->flags;
    wsa->control     = NULL;
    wsa->n_iovecs    = dwBufferCount;
    wsa->first_iovec = 0;
    for ( i = 0; i < dwBufferCount; i++ )
    {
        wsa->iovec[i].iov_base = lpBuffers[i].buf;
        wsa->iovec[i].iov_len  = lpBuffers[i].len;
        totalLength += lpBuffers[i].len;
    }

    n = WS2_send( fd, wsa );
    if (n == -1 && errno != EAGAIN)
    {
        err = wsaErrno();
        goto error;
    }

    if (overlapped)
    {
        IO_STATUS_BLOCK *iosb = lpOverlapped ? (IO_STATUS_BLOCK *)lpOverlapped : &wsa->local_iosb;
        ULONG_PTR cvalue = (lpOverlapped && ((ULONG_PTR)lpOverlapped->hEvent & 1) == 0) ? (ULONG_PTR)lpOverlapped : 0;

        wsa->user_overlapped = lpOverlapped;
        wsa->completion_func = lpCompletionRoutine;
        release_sock_fd( s, fd );

        if (n == -1 || n < totalLength)
        {
            iosb->u.Status = STATUS_PENDING;
            iosb->Information = n == -1 ? 0 : n;

            SERVER_START_REQ( register_async )
            {
                req->type           = ASYNC_TYPE_WRITE;
                req->async.handle   = wine_server_obj_handle( wsa->hSocket );
                req->async.callback = wine_server_client_ptr( WS2_async_send );
                req->async.iosb     = wine_server_client_ptr( iosb );
                req->async.arg      = wine_server_client_ptr( wsa );
                req->async.event    = wine_server_obj_handle( lpCompletionRoutine ? 0 : lpOverlapped->hEvent );
                req->async.cvalue   = cvalue;
                err = wine_server_call( req );
            }
            SERVER_END_REQ;

            /* Enable the event only after starting the async. The server will deliver it as soon as
               the async is done. */
            _enable_event(SOCKET2HANDLE(s), FD_WRITE, 0, 0);

            if (err != STATUS_PENDING) HeapFree( GetProcessHeap(), 0, wsa );
            WSASetLastError( NtStatusToWSAError( err ));
            return SOCKET_ERROR;
        }

        iosb->u.Status = STATUS_SUCCESS;
        iosb->Information = n;
        if (lpNumberOfBytesSent) *lpNumberOfBytesSent = n;
        if (!wsa->completion_func)
        {
            if (cvalue) WS_AddCompletion( s, cvalue, STATUS_SUCCESS, n );
            if (lpOverlapped->hEvent) SetEvent( lpOverlapped->hEvent );
            HeapFree( GetProcessHeap(), 0, wsa );
        }
        else NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)ws2_async_apc,
                               (ULONG_PTR)wsa, (ULONG_PTR)iosb, 0 );
        WSASetLastError(0);
        return 0;
    }

    if ((err = _is_blocking( s, &is_blocking )))
    {
        err = NtStatusToWSAError( err );
        goto error;
    }

    if ( is_blocking )
    {
        /* On a blocking non-overlapped stream socket,
         * sending blocks until the entire buffer is sent. */
        DWORD timeout_start = GetTickCount();

        bytes_sent = n == -1 ? 0 : n;

        while (wsa->first_iovec < wsa->n_iovecs)
        {
            struct pollfd pfd;
            int timeout = GET_SNDTIMEO(fd);

            if (timeout != -1)
            {
                timeout -= GetTickCount() - timeout_start;
                if (timeout < 0) timeout = 0;
            }

            pfd.fd = fd;
            pfd.events = POLLOUT;

            if (!timeout || !poll( &pfd, 1, timeout ))
            {
                err = WSAETIMEDOUT;
                goto error; /* msdn says a timeout in send is fatal */
            }

            n = WS2_send( fd, wsa );
            if (n == -1 && errno != EAGAIN)
            {
                err = wsaErrno();
                goto error;
            }

            if (n >= 0)
                bytes_sent += n;
        }
    }
    else  /* non-blocking */
    {
        if (n < totalLength)
            _enable_event(SOCKET2HANDLE(s), FD_WRITE, 0, 0);
        if (n == -1)
        {
            err = WSAEWOULDBLOCK;
            goto error;
        }
        bytes_sent = n;
    }

    TRACE(" -> %i bytes\n", bytes_sent);

    if (lpNumberOfBytesSent) *lpNumberOfBytesSent = bytes_sent;
    if (wsa != &localwsa) HeapFree( GetProcessHeap(), 0, wsa );
    release_sock_fd( s, fd );
    WSASetLastError(0);
    return 0;

error:
    if (wsa != &localwsa) HeapFree( GetProcessHeap(), 0, wsa );
    release_sock_fd( s, fd );
    WARN(" -> ERROR %d\n", err);
    WSASetLastError(err);
    return SOCKET_ERROR;
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

/***********************************************************************
 *		setsockopt		(WS2_32.21)
 */
int WINAPI WS_setsockopt(SOCKET s, int level, int optname,
                         const char *optval, int optlen)
{
    int fd;
    int woptval;
    struct linger linger;
    struct timeval tval;

    TRACE("socket: %04lx, level 0x%x, name 0x%x, ptr %p, len %d\n",
          s, level, optname, optval, optlen);

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
        /* Some options need some conversion before they can be sent to
         * setsockopt. The conversions are done here, then they will fall though
         * to the general case. Special options that are not passed to
         * setsockopt follow below that.*/

        case WS_SO_DONTLINGER:
            if (!optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            linger.l_onoff  = *(const int*)optval == 0;
            linger.l_linger = 0;
            level = SOL_SOCKET;
            optname = SO_LINGER;
            optval = (char*)&linger;
            optlen = sizeof(struct linger);
            break;

        case WS_SO_LINGER:
            if (!optval)
            {
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }
            linger.l_onoff  = ((LINGER*)optval)->l_onoff;
            linger.l_linger  = ((LINGER*)optval)->l_linger;
            level = SOL_SOCKET;
            optname = SO_LINGER;
            optval = (char*)&linger;
            optlen = sizeof(struct linger);
            break;

        case WS_SO_RCVBUF:
            if (*(const int*)optval < 2048)
            {
                WARN("SO_RCVBF for %d bytes is too small: ignored\n", *(const int*)optval );
                return 0;
            }
            /* Fall through */

        /* The options listed here don't need any special handling. Thanks to
         * the conversion happening above, options from there will fall through
         * to this, too.*/
        case WS_SO_ACCEPTCONN:
        case WS_SO_BROADCAST:
        case WS_SO_ERROR:
        case WS_SO_KEEPALIVE:
        case WS_SO_OOBINLINE:
        /* BSD socket SO_REUSEADDR is not 100% compatible to winsock semantics.
         * however, using it the BSD way fixes bug 8513 and seems to be what
         * most programmers assume, anyway */
        case WS_SO_REUSEADDR:
        case WS_SO_SNDBUF:
        case WS_SO_TYPE:
            convert_sockopt(&level, &optname);
            break;

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

#ifdef SO_RCVTIMEO
        case WS_SO_RCVTIMEO:
#endif
#ifdef SO_SNDTIMEO
        case WS_SO_SNDTIMEO:
#endif
#if defined(SO_RCVTIMEO) || defined(SO_SNDTIMEO)
            if (optval && optlen == sizeof(UINT32)) {
                /* WinSock passes milliseconds instead of struct timeval */
                tval.tv_usec = (*(const UINT32*)optval % 1000) * 1000;
                tval.tv_sec = *(const UINT32*)optval / 1000;
                /* min of 500 milliseconds */
                if (tval.tv_sec == 0 && tval.tv_usec && tval.tv_usec < 500000)
                    tval.tv_usec = 500000;
                optlen = sizeof(struct timeval);
                optval = (char*)&tval;
            } else if (optlen == sizeof(struct timeval)) {
                WARN("SO_SND/RCVTIMEO for %d bytes: assuming unixism\n", optlen);
            } else {
                WARN("SO_SND/RCVTIMEO for %d bytes is weird: ignored\n", optlen);
                return 0;
            }
            convert_sockopt(&level, &optname);
            break;
#endif

        default:
            TRACE("Unknown SOL_SOCKET optname: 0x%08x\n", optname);
            SetLastError(WSAENOPROTOOPT);
            return SOCKET_ERROR;
        }
        break; /* case WS_SOL_SOCKET */

#ifdef HAS_IPX
    case WS_NSPROTO_IPX:
        switch(optname)
        {
        case IPX_PTYPE:
            return set_ipx_packettype(s, *(int*)optval);

        case IPX_FILTERPTYPE:
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
        case WS_IP_DROP_MEMBERSHIP:
#ifdef IP_HDRINCL
        case WS_IP_HDRINCL:
#endif
        case WS_IP_MULTICAST_IF:
        case WS_IP_MULTICAST_LOOP:
        case WS_IP_MULTICAST_TTL:
        case WS_IP_OPTIONS:
#ifdef IP_PKTINFO
        case WS_IP_PKTINFO:
#endif
        case WS_IP_TOS:
        case WS_IP_TTL:
#ifdef IP_UNICAST_IF
        case WS_IP_UNICAST_IF:
#endif
            convert_sockopt(&level, &optname);
            break;
        case WS_IP_DONTFRAGMENT:
            FIXME("IP_DONTFRAGMENT is silently ignored!\n");
            return 0;
        default:
            FIXME("Unknown IPPROTO_IP optname 0x%08x\n", optname);
            return SOCKET_ERROR;
        }
        break;

    case WS_IPPROTO_IPV6:
        switch(optname)
        {
#ifdef IPV6_ADD_MEMBERSHIP
        case WS_IPV6_ADD_MEMBERSHIP:
#endif
#ifdef IPV6_DROP_MEMBERSHIP
        case WS_IPV6_DROP_MEMBERSHIP:
#endif
        case WS_IPV6_MULTICAST_IF:
        case WS_IPV6_MULTICAST_HOPS:
        case WS_IPV6_MULTICAST_LOOP:
        case WS_IPV6_UNICAST_HOPS:
        case WS_IPV6_V6ONLY:
#ifdef IPV6_UNICAST_IF
        case WS_IPV6_UNICAST_IF:
#endif
            convert_sockopt(&level, &optname);
            break;
        case WS_IPV6_DONTFRAG:
            FIXME("IPV6_DONTFRAG is silently ignored!\n");
            return 0;
        case WS_IPV6_PROTECTION_LEVEL:
            FIXME("IPV6_PROTECTION_LEVEL is ignored!\n");
            return 0;
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
 *		shutdown		(WS2_32.22)
 */
int WINAPI WS_shutdown(SOCKET s, int how)
{
    int fd, err = WSAENOTSOCK;
    unsigned int options, clear_flags = 0;

    fd = get_sock_fd( s, 0, &options );
    TRACE("socket %04lx, how %i %x\n", s, how, options );

    if (fd == -1)
        return SOCKET_ERROR;

    switch( how )
    {
    case SD_RECEIVE: /* drop receives */
        clear_flags |= FD_READ;
        break;
    case SD_SEND: /* drop sends */
        clear_flags |= FD_WRITE;
        break;
    case SD_BOTH: /* drop all */
        clear_flags |= FD_READ|FD_WRITE;
        /*fall through */
    default:
        clear_flags |= FD_WINE_LISTENING;
    }

    if (!(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)))
    {
        switch ( how )
        {
        case SD_RECEIVE:
            err = WS2_register_async_shutdown( s, ASYNC_TYPE_READ );
            break;
        case SD_SEND:
            err = WS2_register_async_shutdown( s, ASYNC_TYPE_WRITE );
            break;
        case SD_BOTH:
        default:
            err = WS2_register_async_shutdown( s, ASYNC_TYPE_READ );
            if (!err) err = WS2_register_async_shutdown( s, ASYNC_TYPE_WRITE );
            break;
        }
        if (err) goto error;
    }
    else /* non-overlapped mode */
    {
        if ( shutdown( fd, how ) )
        {
            err = wsaErrno();
            goto error;
        }
    }

    release_sock_fd( s, fd );
    _enable_event( SOCKET2HANDLE(s), 0, 0, clear_flags );
    if ( how > 1) WSAAsyncSelect( s, 0, 0, 0 );
    return 0;

error:
    release_sock_fd( s, fd );
    _enable_event( SOCKET2HANDLE(s), 0, 0, clear_flags );
    WSASetLastError( err );
    return SOCKET_ERROR;
}

/***********************************************************************
 *		socket		(WS2_32.23)
 */
SOCKET WINAPI WS_socket(int af, int type, int protocol)
{
    TRACE("af=%d type=%d protocol=%d\n", af, type, protocol);

    return WSASocketA( af, type, protocol, NULL, 0,
                       get_per_thread_data()->opentype ? 0 : WSA_FLAG_OVERLAPPED );
}


/***********************************************************************
 *		gethostbyaddr		(WS2_32.51)
 */
struct WS_hostent* WINAPI WS_gethostbyaddr(const char *addr, int len, int type)
{
    struct WS_hostent *retval = NULL;
    struct hostent* host;
    int unixtype = convert_af_w2u(type);
    const char *paddr = addr;
    unsigned long loopback;
#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
    char *extrabuf;
    int ebufsize = 1024;
    struct hostent hostentry;
    int locerr = ENOBUFS;
#endif

    /* convert back the magic loopback address if necessary */
    if (unixtype == AF_INET && len == 4 && !memcmp(addr, magic_loopback_addr, 4))
    {
        loopback = htonl(INADDR_LOOPBACK);
        paddr = (char*) &loopback;
    }

#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
    host = NULL;
    extrabuf=HeapAlloc(GetProcessHeap(),0,ebufsize) ;
    while(extrabuf) {
        int res = gethostbyaddr_r(paddr, len, unixtype,
                                  &hostentry, extrabuf, ebufsize, &host, &locerr);
        if (res != ERANGE) break;
        ebufsize *=2;
        extrabuf=HeapReAlloc(GetProcessHeap(),0,extrabuf,ebufsize) ;
    }
    if (host) retval = WS_dup_he(host);
    else SetLastError((locerr < 0) ? wsaErrno() : wsaHerrno(locerr));
    HeapFree(GetProcessHeap(),0,extrabuf);
#else
    EnterCriticalSection( &csWSgetXXXbyYYY );
    host = gethostbyaddr(paddr, len, unixtype);
    if (host) retval = WS_dup_he(host);
    else SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno(h_errno));
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    TRACE("ptr %p, len %d, type %d ret %p\n", addr, len, type, retval);
    return retval;
}

/***********************************************************************
 *		WS_get_local_ips		(INTERNAL)
 *
 * Returns the list of local IP addresses by going through the network
 * adapters and using the local routing table to sort the addresses
 * from highest routing priority to lowest routing priority. This
 * functionality is inferred from the description for obtaining local
 * IP addresses given in the Knowledge Base Article Q160215.
 *
 * Please note that the returned hostent is only freed when the thread
 * closes and is replaced if another hostent is requested.
 */
static struct WS_hostent* WS_get_local_ips( char *hostname )
{
    int last_metric, numroutes = 0, i, j;
    DWORD n;
    PIP_ADAPTER_INFO adapters = NULL, k;
    struct WS_hostent *hostlist = NULL;
    PMIB_IPFORWARDTABLE routes = NULL;
    struct route *route_addrs = NULL;
    DWORD adap_size, route_size;

    /* Obtain the size of the adapter list and routing table, also allocate memory */
    if (GetAdaptersInfo(NULL, &adap_size) != ERROR_BUFFER_OVERFLOW)
        return NULL;
    if (GetIpForwardTable(NULL, &route_size, FALSE) != ERROR_INSUFFICIENT_BUFFER)
        return NULL;
    adapters = HeapAlloc(GetProcessHeap(), 0, adap_size);
    routes = HeapAlloc(GetProcessHeap(), 0, route_size);
    route_addrs = HeapAlloc(GetProcessHeap(), 0, 0); /* HeapReAlloc doesn't work on NULL */
    if (adapters == NULL || routes == NULL || route_addrs == NULL)
        goto cleanup;
    /* Obtain the adapter list and the full routing table */
    if (GetAdaptersInfo(adapters, &adap_size) != NO_ERROR)
        goto cleanup;
    if (GetIpForwardTable(routes, &route_size, FALSE) != NO_ERROR)
        goto cleanup;
    /* Store the interface associated with each route */
    for (n = 0; n < routes->dwNumEntries; n++)
    {
        IF_INDEX ifindex;
        DWORD ifmetric;
        BOOL exists = FALSE;

        if (routes->table[n].u1.ForwardType != MIB_IPROUTE_TYPE_DIRECT)
            continue;
        ifindex = routes->table[n].dwForwardIfIndex;
        ifmetric = routes->table[n].dwForwardMetric1;
        /* Only store the lowest valued metric for an interface */
        for (j = 0; j < numroutes; j++)
        {
            if (route_addrs[j].interface == ifindex)
            {
                if (route_addrs[j].metric > ifmetric)
                    route_addrs[j].metric = ifmetric;
                exists = TRUE;
            }
        }
        if (exists)
            continue;
        route_addrs = HeapReAlloc(GetProcessHeap(), 0, route_addrs, (numroutes+1)*sizeof(struct route));
        if (route_addrs == NULL)
            goto cleanup; /* Memory allocation error, fail gracefully */
        route_addrs[numroutes].interface = ifindex;
        route_addrs[numroutes].metric = ifmetric;
        /* If no IP is found in the next step (for whatever reason)
         * then fall back to the magic loopback address.
         */
        memcpy(&(route_addrs[numroutes].addr.s_addr), magic_loopback_addr, 4);
        numroutes++;
    }
   if (numroutes == 0)
       goto cleanup; /* No routes, fall back to the Magic IP */
    /* Find the IP address associated with each found interface */
    for (i = 0; i < numroutes; i++)
    {
        for (k = adapters; k != NULL; k = k->Next)
        {
            char *ip = k->IpAddressList.IpAddress.String;

            if (route_addrs[i].interface == k->Index)
                route_addrs[i].addr.s_addr = (in_addr_t) inet_addr(ip);
        }
    }
    /* Allocate a hostent and enough memory for all the IPs,
     * including the NULL at the end of the list.
     */
    hostlist = WS_create_he(hostname, 1, 0, numroutes+1, sizeof(struct in_addr));
    if (hostlist == NULL)
        goto cleanup; /* Failed to allocate a hostent for the list of IPs */
    hostlist->h_addr_list[numroutes] = NULL; /* NULL-terminate the address list */
    hostlist->h_aliases[0] = NULL; /* NULL-terminate the alias list */
    hostlist->h_addrtype = AF_INET;
    hostlist->h_length = sizeof(struct in_addr); /* = 4 */
    /* Reorder the entries when placing them in the host list, Windows expects
     * the IP list in order from highest priority to lowest (the critical thing
     * is that most applications expect the first IP to be the default route).
     */
    last_metric = -1;
    for (i = 0; i < numroutes; i++)
    {
       struct in_addr addr;
       int metric = 0xFFFF;

       memcpy(&addr, magic_loopback_addr, 4);
       for (j = 0; j < numroutes; j++)
       {
           int this_metric = route_addrs[j].metric;

           if (this_metric > last_metric && this_metric < metric)
           {
               addr = route_addrs[j].addr;
               metric = this_metric;
           }
       }
       last_metric = metric;
       (*(struct in_addr *) hostlist->h_addr_list[i]) = addr;
    }

    /* Cleanup all allocated memory except the address list,
     * the address list is used by the calling app.
     */
cleanup:
    HeapFree(GetProcessHeap(), 0, route_addrs);
    HeapFree(GetProcessHeap(), 0, adapters);
    HeapFree(GetProcessHeap(), 0, routes);
    return hostlist;
}

/***********************************************************************
 *		gethostbyname		(WS2_32.52)
 */
struct WS_hostent* WINAPI WS_gethostbyname(const char* name)
{
    struct WS_hostent *retval = NULL;
    struct hostent*     host;
#ifdef  HAVE_LINUX_GETHOSTBYNAME_R_6
    char *extrabuf;
    int ebufsize=1024;
    struct hostent hostentry;
    int locerr = ENOBUFS;
#endif
    char hostname[100];
    if(!num_startup) {
        SetLastError(WSANOTINITIALISED);
        return NULL;
    }
    if( gethostname( hostname, 100) == -1) {
        SetLastError( WSAENOBUFS); /* appropriate ? */
        return retval;
    }
    if( !name || !name[0]) {
        name = hostname;
    }
    /* If the hostname of the local machine is requested then return the
     * complete list of local IP addresses */
    if(strcmp(name, hostname) == 0)
        retval = WS_get_local_ips(hostname);
    /* If any other hostname was requested (or the routing table lookup failed)
     * then return the IP found by the host OS */
    if(retval == NULL)
    {
#ifdef  HAVE_LINUX_GETHOSTBYNAME_R_6
        host = NULL;
        extrabuf=HeapAlloc(GetProcessHeap(),0,ebufsize) ;
        while(extrabuf) {
            int res = gethostbyname_r(name, &hostentry, extrabuf, ebufsize, &host, &locerr);
            if( res != ERANGE) break;
            ebufsize *=2;
            extrabuf=HeapReAlloc(GetProcessHeap(),0,extrabuf,ebufsize) ;
        }
        if (!host) SetLastError((locerr < 0) ? wsaErrno() : wsaHerrno(locerr));
#else
        EnterCriticalSection( &csWSgetXXXbyYYY );
        host = gethostbyname(name);
        if (!host) SetLastError((h_errno < 0) ? wsaErrno() : wsaHerrno(h_errno));
#endif
        if (host) retval = WS_dup_he(host);
#ifdef  HAVE_LINUX_GETHOSTBYNAME_R_6
        HeapFree(GetProcessHeap(),0,extrabuf);
#else
        LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    }
    if (retval && retval->h_addr_list[0][0] == 127 &&
        strcmp(name, "localhost") != 0)
    {
        /* hostname != "localhost" but has loopback address. replace by our
         * special address.*/
        memcpy(retval->h_addr_list[0], magic_loopback_addr, 4);
    }
    TRACE( "%s ret %p\n", debugstr_a(name), retval );
    return retval;
}


/***********************************************************************
 *		getprotobyname		(WS2_32.53)
 */
struct WS_protoent* WINAPI WS_getprotobyname(const char* name)
{
    struct WS_protoent* retval = NULL;
#ifdef HAVE_GETPROTOBYNAME
    struct protoent*     proto;
    EnterCriticalSection( &csWSgetXXXbyYYY );
    if( (proto = getprotobyname(name)) != NULL )
    {
        retval = WS_dup_pe(proto);
    }
    else {
        MESSAGE("protocol %s not found; You might want to add "
                "this to /etc/protocols\n", debugstr_a(name) );
        SetLastError(WSANO_DATA);
    }
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    TRACE( "%s ret %p\n", debugstr_a(name), retval );
    return retval;
}


/***********************************************************************
 *		getprotobynumber	(WS2_32.54)
 */
struct WS_protoent* WINAPI WS_getprotobynumber(int number)
{
    struct WS_protoent* retval = NULL;
#ifdef HAVE_GETPROTOBYNUMBER
    struct protoent*     proto;
    EnterCriticalSection( &csWSgetXXXbyYYY );
    if( (proto = getprotobynumber(number)) != NULL )
    {
        retval = WS_dup_pe(proto);
    }
    else {
        MESSAGE("protocol number %d not found; You might want to add "
                "this to /etc/protocols\n", number );
        SetLastError(WSANO_DATA);
    }
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    TRACE("%i ret %p\n", number, retval);
    return retval;
}


/***********************************************************************
 *		getservbyname		(WS2_32.55)
 */
struct WS_servent* WINAPI WS_getservbyname(const char *name, const char *proto)
{
    struct WS_servent* retval = NULL;
    struct servent*     serv;
    char *name_str;
    char *proto_str = NULL;

    if (!(name_str = strdup_lower(name))) return NULL;

    if (proto && *proto)
    {
        if (!(proto_str = strdup_lower(proto)))
        {
            HeapFree( GetProcessHeap(), 0, name_str );
            return NULL;
        }
    }

    EnterCriticalSection( &csWSgetXXXbyYYY );
    serv = getservbyname(name_str, proto_str);
    if( serv != NULL )
    {
        retval = WS_dup_se(serv);
    }
    else SetLastError(WSANO_DATA);
    LeaveCriticalSection( &csWSgetXXXbyYYY );
    HeapFree( GetProcessHeap(), 0, proto_str );
    HeapFree( GetProcessHeap(), 0, name_str );
    TRACE( "%s, %s ret %p\n", debugstr_a(name), debugstr_a(proto), retval );
    return retval;
}

/***********************************************************************
 *		freeaddrinfo		(WS2_32.@)
 */
void WINAPI WS_freeaddrinfo(struct WS_addrinfo *res)
{
    while (res) {
        struct WS_addrinfo *next;

        HeapFree(GetProcessHeap(),0,res->ai_canonname);
        HeapFree(GetProcessHeap(),0,res->ai_addr);
        next = res->ai_next;
        HeapFree(GetProcessHeap(),0,res);
        res = next;
    }
}

/* helper functions for getaddrinfo()/getnameinfo() */
static int convert_aiflag_w2u(int winflags) {
    unsigned int i;
    int unixflags = 0;

    for (i=0;i<sizeof(ws_aiflag_map)/sizeof(ws_aiflag_map[0]);i++)
        if (ws_aiflag_map[i][0] & winflags) {
            unixflags |= ws_aiflag_map[i][1];
            winflags &= ~ws_aiflag_map[i][0];
        }
    if (winflags)
        FIXME("Unhandled windows AI_xxx flags %x\n", winflags);
    return unixflags;
}

static int convert_niflag_w2u(int winflags) {
    unsigned int i;
    int unixflags = 0;

    for (i=0;i<sizeof(ws_niflag_map)/sizeof(ws_niflag_map[0]);i++)
        if (ws_niflag_map[i][0] & winflags) {
            unixflags |= ws_niflag_map[i][1];
            winflags &= ~ws_niflag_map[i][0];
        }
    if (winflags)
        FIXME("Unhandled windows NI_xxx flags %x\n", winflags);
    return unixflags;
}

static int convert_aiflag_u2w(int unixflags) {
    unsigned int i;
    int winflags = 0;

    for (i=0;i<sizeof(ws_aiflag_map)/sizeof(ws_aiflag_map[0]);i++)
        if (ws_aiflag_map[i][1] & unixflags) {
            winflags |= ws_aiflag_map[i][0];
            unixflags &= ~ws_aiflag_map[i][1];
        }
    if (unixflags) /* will warn usually */
        WARN("Unhandled UNIX AI_xxx flags %x\n", unixflags);
    return winflags;
}

static int convert_eai_u2w(int unixret) {
    int i;

    if (!unixret) return 0;

    for (i=0;ws_eai_map[i][0];i++)
        if (ws_eai_map[i][1] == unixret)
            return ws_eai_map[i][0];

    if (unixret == EAI_SYSTEM)
        /* There are broken versions of glibc which return EAI_SYSTEM
         * and set errno to 0 instead of returning EAI_NONAME.
         */
        return errno ? sock_get_error( errno ) : WS_EAI_NONAME;

    FIXME("Unhandled unix EAI_xxx ret %d\n", unixret);
    return unixret;
}

static char *get_hostname(void)
{
    char *ret;
    DWORD size = 0;

    GetComputerNameExA( ComputerNamePhysicalDnsHostname, NULL, &size );
    if (GetLastError() != ERROR_MORE_DATA) return NULL;
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    if (!GetComputerNameExA( ComputerNamePhysicalDnsHostname, ret, &size ))
    {
        HeapFree( GetProcessHeap(), 0, ret );
        return NULL;
    }
    return ret;
}

/***********************************************************************
 *		getaddrinfo		(WS2_32.@)
 */
int WINAPI WS_getaddrinfo(LPCSTR nodename, LPCSTR servname, const struct WS_addrinfo *hints, struct WS_addrinfo **res)
{
#ifdef HAVE_GETADDRINFO
    struct addrinfo *unixaires = NULL;
    int   result;
    struct addrinfo unixhints, *punixhints = NULL;
    char *hostname = NULL;
    const char *node;

    *res = NULL;
    if (!nodename && !servname) return WSAHOST_NOT_FOUND;

    if (!nodename)
        node = NULL;
    else if (!nodename[0])
    {
        node = hostname = get_hostname();
        if (!node) return WSA_NOT_ENOUGH_MEMORY;
    }
    else
        node = nodename;

    /* servname tweak required by OSX and BSD kernels */
    if (servname && !servname[0]) servname = "0";

    if (hints) {
        punixhints = &unixhints;

        memset(&unixhints, 0, sizeof(unixhints));
        punixhints->ai_flags = convert_aiflag_w2u(hints->ai_flags);

        /* zero is a wildcard, no need to convert */
        if (hints->ai_family)
            punixhints->ai_family = convert_af_w2u(hints->ai_family);
        if (hints->ai_socktype)
            punixhints->ai_socktype = convert_socktype_w2u(hints->ai_socktype);
        if (hints->ai_protocol)
            punixhints->ai_protocol = max(convert_proto_w2u(hints->ai_protocol), 0);

        if (punixhints->ai_socktype < 0)
        {
            WSASetLastError(WSAESOCKTNOSUPPORT);
            HeapFree(GetProcessHeap(), 0, hostname);
            return SOCKET_ERROR;
        }

        /* windows allows invalid combinations of socket type and protocol, unix does not.
         * fix the parameters here to make getaddrinfo call always work */
        if (punixhints->ai_protocol == IPPROTO_TCP &&
            punixhints->ai_socktype != SOCK_STREAM && punixhints->ai_socktype != SOCK_SEQPACKET)
            punixhints->ai_socktype = 0;

        else if (punixhints->ai_protocol == IPPROTO_UDP && punixhints->ai_socktype != SOCK_DGRAM)
            punixhints->ai_socktype = 0;

        else if (IS_IPX_PROTO(punixhints->ai_protocol) && punixhints->ai_socktype != SOCK_DGRAM)
            punixhints->ai_socktype = 0;
    }

    /* getaddrinfo(3) is thread safe, no need to wrap in CS */
    result = getaddrinfo(node, servname, punixhints, &unixaires);

    TRACE("%s, %s %p -> %p %d\n", debugstr_a(nodename), debugstr_a(servname), hints, res, result);
    HeapFree(GetProcessHeap(), 0, hostname);

    if (!result) {
        struct addrinfo *xuai = unixaires;
        struct WS_addrinfo **xai = res;

        *xai = NULL;
        while (xuai) {
            struct WS_addrinfo *ai = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(struct WS_addrinfo));
            SIZE_T len;

            if (!ai)
                goto outofmem;

            *xai = ai;xai = &ai->ai_next;
            ai->ai_flags    = convert_aiflag_u2w(xuai->ai_flags);
            ai->ai_family   = convert_af_u2w(xuai->ai_family);
            /* copy whatever was sent in the hints */
            if(hints) {
                ai->ai_socktype = hints->ai_socktype;
                ai->ai_protocol = hints->ai_protocol;
            } else {
                ai->ai_socktype = convert_socktype_u2w(xuai->ai_socktype);
                ai->ai_protocol = convert_proto_u2w(xuai->ai_protocol);
            }
            if (xuai->ai_canonname) {
                TRACE("canon name - %s\n",debugstr_a(xuai->ai_canonname));
                ai->ai_canonname = HeapAlloc(GetProcessHeap(),0,strlen(xuai->ai_canonname)+1);
                if (!ai->ai_canonname)
                    goto outofmem;
                strcpy(ai->ai_canonname,xuai->ai_canonname);
            }
            len = xuai->ai_addrlen;
            ai->ai_addr = HeapAlloc(GetProcessHeap(),0,len);
            if (!ai->ai_addr)
                goto outofmem;
            ai->ai_addrlen = len;
            do {
                int winlen = ai->ai_addrlen;

                if (!ws_sockaddr_u2ws(xuai->ai_addr, ai->ai_addr, &winlen)) {
                    ai->ai_addrlen = winlen;
                    break;
                }
                len = 2*len;
                ai->ai_addr = HeapReAlloc(GetProcessHeap(),0,ai->ai_addr,len);
                if (!ai->ai_addr)
                    goto outofmem;
                ai->ai_addrlen = len;
            } while (1);
            xuai = xuai->ai_next;
        }
        freeaddrinfo(unixaires);

        if (TRACE_ON(winsock))
        {
            struct WS_addrinfo *ai = *res;
            while (ai)
            {
                TRACE("=> %p, flags %#x, family %d, type %d, protocol %d, len %ld, name %s, addr %s\n",
                      ai, ai->ai_flags, ai->ai_family, ai->ai_socktype, ai->ai_protocol, ai->ai_addrlen,
                      ai->ai_canonname, debugstr_sockaddr(ai->ai_addr));
                ai = ai->ai_next;
            }
        }
    } else
        result = convert_eai_u2w(result);

    return result;

outofmem:
    if (*res) WS_freeaddrinfo(*res);
    if (unixaires) freeaddrinfo(unixaires);
    return WSA_NOT_ENOUGH_MEMORY;
#else
    FIXME("getaddrinfo() failed, not found during buildtime.\n");
    return EAI_FAIL;
#endif
}

static struct WS_addrinfoW *addrinfo_AtoW(const struct WS_addrinfo *ai)
{
    struct WS_addrinfoW *ret;

    if (!(ret = HeapAlloc(GetProcessHeap(), 0, sizeof(struct WS_addrinfoW)))) return NULL;
    ret->ai_flags     = ai->ai_flags;
    ret->ai_family    = ai->ai_family;
    ret->ai_socktype  = ai->ai_socktype;
    ret->ai_protocol  = ai->ai_protocol;
    ret->ai_addrlen   = ai->ai_addrlen;
    ret->ai_canonname = NULL;
    ret->ai_addr      = NULL;
    ret->ai_next      = NULL;
    if (ai->ai_canonname)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, ai->ai_canonname, -1, NULL, 0);
        if (!(ret->ai_canonname = HeapAlloc(GetProcessHeap(), 0, len)))
        {
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        MultiByteToWideChar(CP_ACP, 0, ai->ai_canonname, -1, ret->ai_canonname, len);
    }
    if (ai->ai_addr)
    {
        if (!(ret->ai_addr = HeapAlloc(GetProcessHeap(), 0, ai->ai_addrlen)))
        {
            HeapFree(GetProcessHeap(), 0, ret->ai_canonname);
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        memcpy(ret->ai_addr, ai->ai_addr, ai->ai_addrlen);
    }
    return ret;
}

static struct WS_addrinfoW *addrinfo_list_AtoW(const struct WS_addrinfo *info)
{
    struct WS_addrinfoW *ret, *infoW;

    if (!(ret = infoW = addrinfo_AtoW(info))) return NULL;
    while (info->ai_next)
    {
        if (!(infoW->ai_next = addrinfo_AtoW(info->ai_next)))
        {
            FreeAddrInfoW(ret);
            return NULL;
        }
        infoW = infoW->ai_next;
        info = info->ai_next;
    }
    return ret;
}

static struct WS_addrinfo *addrinfo_WtoA(const struct WS_addrinfoW *ai)
{
    struct WS_addrinfo *ret;

    if (!(ret = HeapAlloc(GetProcessHeap(), 0, sizeof(struct WS_addrinfo)))) return NULL;
    ret->ai_flags     = ai->ai_flags;
    ret->ai_family    = ai->ai_family;
    ret->ai_socktype  = ai->ai_socktype;
    ret->ai_protocol  = ai->ai_protocol;
    ret->ai_addrlen   = ai->ai_addrlen;
    ret->ai_canonname = NULL;
    ret->ai_addr      = NULL;
    ret->ai_next      = NULL;
    if (ai->ai_canonname)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, ai->ai_canonname, -1, NULL, 0, NULL, NULL);
        if (!(ret->ai_canonname = HeapAlloc(GetProcessHeap(), 0, len)))
        {
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        WideCharToMultiByte(CP_ACP, 0, ai->ai_canonname, -1, ret->ai_canonname, len, NULL, NULL);
    }
    if (ai->ai_addr)
    {
        if (!(ret->ai_addr = HeapAlloc(GetProcessHeap(), 0, sizeof(struct WS_sockaddr))))
        {
            HeapFree(GetProcessHeap(), 0, ret->ai_canonname);
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        memcpy(ret->ai_addr, ai->ai_addr, sizeof(struct WS_sockaddr));
    }
    return ret;
}

/***********************************************************************
 *		GetAddrInfoW		(WS2_32.@)
 */
int WINAPI GetAddrInfoW(LPCWSTR nodename, LPCWSTR servname, const ADDRINFOW *hints, PADDRINFOW *res)
{
    int ret, len;
    char *nodenameA = NULL, *servnameA = NULL;
    struct WS_addrinfo *resA, *hintsA = NULL;

    *res = NULL;
    if (nodename)
    {
        len = WideCharToMultiByte(CP_ACP, 0, nodename, -1, NULL, 0, NULL, NULL);
        if (!(nodenameA = HeapAlloc(GetProcessHeap(), 0, len))) return EAI_MEMORY;
        WideCharToMultiByte(CP_ACP, 0, nodename, -1, nodenameA, len, NULL, NULL);
    }
    if (servname)
    {
        len = WideCharToMultiByte(CP_ACP, 0, servname, -1, NULL, 0, NULL, NULL);
        if (!(servnameA = HeapAlloc(GetProcessHeap(), 0, len)))
        {
            HeapFree(GetProcessHeap(), 0, nodenameA);
            return EAI_MEMORY;
        }
        WideCharToMultiByte(CP_ACP, 0, servname, -1, servnameA, len, NULL, NULL);
    }

    if (hints) hintsA = addrinfo_WtoA(hints);
    ret = WS_getaddrinfo(nodenameA, servnameA, hintsA, &resA);
    WS_freeaddrinfo(hintsA);

    if (!ret)
    {
        *res = addrinfo_list_AtoW(resA);
        WS_freeaddrinfo(resA);
    }

    HeapFree(GetProcessHeap(), 0, nodenameA);
    HeapFree(GetProcessHeap(), 0, servnameA);
    return ret;
}

/***********************************************************************
 *      FreeAddrInfoW        (WS2_32.@)
 */
void WINAPI FreeAddrInfoW(PADDRINFOW ai)
{
    while (ai)
    {
        ADDRINFOW *next;
        HeapFree(GetProcessHeap(), 0, ai->ai_canonname);
        HeapFree(GetProcessHeap(), 0, ai->ai_addr);
        next = ai->ai_next;
        HeapFree(GetProcessHeap(), 0, ai);
        ai = next;
    }
}

int WINAPI WS_getnameinfo(const SOCKADDR *sa, WS_socklen_t salen, PCHAR host,
                          DWORD hostlen, PCHAR serv, DWORD servlen, INT flags)
{
#ifdef HAVE_GETNAMEINFO
    int ret;
    union generic_unix_sockaddr sa_u;
    unsigned int size;

    TRACE("%s %d %p %d %p %d %d\n", debugstr_sockaddr(sa), salen, host, hostlen,
          serv, servlen, flags);

    size = ws_sockaddr_ws2u(sa, salen, &sa_u);
    if (!size)
    {
        WSASetLastError(WSAEFAULT);
        return WSA_NOT_ENOUGH_MEMORY;
    }
    ret = getnameinfo(&sa_u.addr, size, host, hostlen, serv, servlen, convert_niflag_w2u(flags));
    return convert_eai_u2w(ret);
#else
    FIXME("getnameinfo() failed, not found during buildtime.\n");
    return EAI_FAIL;
#endif
}

int WINAPI GetNameInfoW(const SOCKADDR *sa, WS_socklen_t salen, PWCHAR host,
                        DWORD hostlen, PWCHAR serv, DWORD servlen, INT flags)
{
    int ret;
    char *hostA = NULL, *servA = NULL;

    if (host && (!(hostA = HeapAlloc(GetProcessHeap(), 0, hostlen)))) return EAI_MEMORY;
    if (serv && (!(servA = HeapAlloc(GetProcessHeap(), 0, servlen))))
    {
        HeapFree(GetProcessHeap(), 0, hostA);
        return EAI_MEMORY;
    }

    ret = WS_getnameinfo(sa, salen, hostA, hostlen, servA, servlen, flags);
    if (!ret)
    {
        if (host) MultiByteToWideChar(CP_ACP, 0, hostA, -1, host, hostlen);
        if (serv) MultiByteToWideChar(CP_ACP, 0, servA, -1, serv, servlen);
    }

    HeapFree(GetProcessHeap(), 0, hostA);
    HeapFree(GetProcessHeap(), 0, servA);
    return ret;
}

/***********************************************************************
 *		getservbyport		(WS2_32.56)
 */
struct WS_servent* WINAPI WS_getservbyport(int port, const char *proto)
{
    struct WS_servent* retval = NULL;
#ifdef HAVE_GETSERVBYPORT
    struct servent*     serv;
    char *proto_str = NULL;

    if (proto && *proto)
    {
        if (!(proto_str = strdup_lower(proto))) return NULL;
    }
    EnterCriticalSection( &csWSgetXXXbyYYY );
    if( (serv = getservbyport(port, proto_str)) != NULL ) {
        retval = WS_dup_se(serv);
    }
    else SetLastError(WSANO_DATA);
    LeaveCriticalSection( &csWSgetXXXbyYYY );
    HeapFree( GetProcessHeap(), 0, proto_str );
#endif
    TRACE("%d (i.e. port %d), %s ret %p\n", port, (int)ntohl(port), debugstr_a(proto), retval);
    return retval;
}


/***********************************************************************
 *              gethostname           (WS2_32.57)
 */
int WINAPI WS_gethostname(char *name, int namelen)
{
    TRACE("name %p, len %d\n", name, namelen);

    if (gethostname(name, namelen) == 0)
    {
        TRACE("<- '%s'\n", name);
        return 0;
    }
    SetLastError((errno == EINVAL) ? WSAEFAULT : wsaErrno());
    TRACE("<- ERROR !\n");
    return SOCKET_ERROR;
}


/* ------------------------------------- Windows sockets extensions -- *
 *								       *
 * ------------------------------------------------------------------- */

/***********************************************************************
 *		WSAEnumNetworkEvents (WS2_32.36)
 */
int WINAPI WSAEnumNetworkEvents(SOCKET s, WSAEVENT hEvent, LPWSANETWORKEVENTS lpEvent)
{
    int ret;
    int i;
    int errors[FD_MAX_EVENTS];

    TRACE("%08lx, hEvent %p, lpEvent %p\n", s, hEvent, lpEvent );

    SERVER_START_REQ( get_socket_event )
    {
        req->handle  = wine_server_obj_handle( SOCKET2HANDLE(s) );
        req->service = TRUE;
        req->c_event = wine_server_obj_handle( hEvent );
        wine_server_set_reply( req, errors, sizeof(errors) );
        if (!(ret = wine_server_call(req))) lpEvent->lNetworkEvents = reply->pmask & reply->mask;
    }
    SERVER_END_REQ;
    if (!ret)
    {
        for (i = 0; i < FD_MAX_EVENTS; i++)
            lpEvent->iErrorCode[i] = NtStatusToWSAError(errors[i]);
        return 0;
    }
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/***********************************************************************
 *		WSAEventSelect (WS2_32.39)
 */
int WINAPI WSAEventSelect(SOCKET s, WSAEVENT hEvent, LONG lEvent)
{
    int ret;

    TRACE("%08lx, hEvent %p, event %08x\n", s, hEvent, lEvent);

    SERVER_START_REQ( set_socket_event )
    {
        req->handle = wine_server_obj_handle( SOCKET2HANDLE(s) );
        req->mask   = lEvent;
        req->event  = wine_server_obj_handle( hEvent );
        req->window = 0;
        req->msg    = 0;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (!ret) return 0;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
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
        WSASetLastError(WSA_INVALID_PARAMETER);
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

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *      WSAAsyncSelect			(WS2_32.101)
 */
INT WINAPI WSAAsyncSelect(SOCKET s, HWND hWnd, UINT uMsg, LONG lEvent)
{
    int ret;

    TRACE("%lx, hWnd %p, uMsg %08x, event %08x\n", s, hWnd, uMsg, lEvent);

    SERVER_START_REQ( set_socket_event )
    {
        req->handle = wine_server_obj_handle( SOCKET2HANDLE(s) );
        req->mask   = lEvent;
        req->event  = 0;
        req->window = wine_server_user_handle( hWnd );
        req->msg    = uMsg;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (!ret) return 0;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
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
        WSASetLastError( WSAEINVAL);
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
                         GROUP g, DWORD dwFlags)
{
    SOCKET ret;
    DWORD err;
    int unixaf, unixtype, ipxptype = -1;

   /*
      FIXME: The "advanced" parameters of WSASocketW (lpProtocolInfo,
      g, dwFlags except WSA_FLAG_OVERLAPPED) are ignored.
   */

   TRACE("af=%d type=%d protocol=%d protocol_info=%p group=%d flags=0x%x\n",
         af, type, protocol, lpProtocolInfo, g, dwFlags );

    if (!num_startup)
    {
        err = WSANOTINITIALISED;
        goto done;
    }

    /* hack for WSADuplicateSocket */
    if (lpProtocolInfo && lpProtocolInfo->dwServiceFlags4 == 0xff00ff00) {
      ret = lpProtocolInfo->dwServiceFlags3;
      TRACE("\tgot duplicate %04lx\n", ret);
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

    if (!type && (af || protocol))
    {
        int autoproto = protocol;
        WSAPROTOCOL_INFOW infow;

        /* default to the first valid protocol */
        if (!autoproto)
            autoproto = valid_protocols[0];
        else if(IS_IPX_PROTO(autoproto))
            autoproto = WS_NSPROTO_IPX;

        if (WS_EnterSingleProtocolW(autoproto, &infow))
        {
            type = infow.iSocketType;

            /* after win2003 it's no longer possible to pass AF_UNSPEC
               using the protocol info struct */
            if (!lpProtocolInfo && af == WS_AF_UNSPEC)
                af = infow.iAddressFamily;
        }
    }

    /*
       Windows has an extension to the IPX protocol that allows one to create sockets
       and set the IPX packet type at the same time, to do that a caller will use
       a protocol like NSPROTO_IPX + <PACKET TYPE>
    */
    if (IS_IPX_PROTO(protocol))
        ipxptype = protocol - WS_NSPROTO_IPX;

    /* convert the socket family, type and protocol */
    unixaf = convert_af_w2u(af);
    unixtype = convert_socktype_w2u(type);
    protocol = convert_proto_w2u(protocol);
    if (unixaf == AF_UNSPEC) unixaf = -1;

    /* filter invalid parameters */
    if (protocol < 0)
    {
        /* the type could not be converted */
        if (type && unixtype < 0)
        {
            err = WSAESOCKTNOSUPPORT;
            goto done;
        }

        err = WSAEPROTONOSUPPORT;
        goto done;
    }
    if (unixaf < 0)
    {
        /* both family and protocol can't be invalid */
        if (protocol <= 0)
        {
            err = WSAEINVAL;
            goto done;
        }

        /* family could not be converted and neither socket type */
        if (unixtype < 0 && af >= 0)
        {

            err = WSAESOCKTNOSUPPORT;
            goto done;
        }

        err = WSAEAFNOSUPPORT;
        goto done;
    }

    SERVER_START_REQ( create_socket )
    {
        req->family     = unixaf;
        req->type       = unixtype;
        req->protocol   = protocol;
        req->access     = GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE;
        req->attributes = OBJ_INHERIT;
        req->flags      = dwFlags;
        set_error( wine_server_call( req ) );
        ret = HANDLE2SOCKET( wine_server_ptr_handle( reply->handle ));
    }
    SERVER_END_REQ;
    if (ret)
    {
        TRACE("\tcreated %04lx\n", ret );
        if (ipxptype > 0)
            set_ipx_packettype(ret, ipxptype);
       return ret;
    }

    err = GetLastError();
    if (err == WSAEACCES) /* raw socket denied */
    {
        if (type == SOCK_RAW)
            ERR_(winediag)("Failed to create a socket of type SOCK_RAW, this requires special permissions.\n");
        else
            ERR_(winediag)("Failed to create socket, this requires special permissions.\n");
    }
    else
    {
        /* invalid combination of valid parameters, like SOCK_STREAM + IPPROTO_UDP */
        if (err == WSAEINVAL)
            err = WSAESOCKTNOSUPPORT;
        else if (err == WSAEOPNOTSUPP)
            err = WSAEPROTONOSUPPORT;
    }

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
  int i = set->fd_count;

  TRACE("(%ld,%p(%i))\n", s, set, i);

  while (i--)
      if (set->fd_array[i] == s) return 1;
  return 0;
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


/* ----------------------------------- end of API stuff */

/* ----------------------------------- helper functions -
 *
 * TODO: Merge WS_dup_..() stuff into one function that
 * would operate with a generic structure containing internal
 * pointers (via a template of some kind).
 */

static int list_size(char** l, int item_size)
{
  int i,j = 0;
  if(l)
  { for(i=0;l[i];i++)
	j += (item_size) ? item_size : strlen(l[i]) + 1;
    j += (i + 1) * sizeof(char*); }
  return j;
}

static int list_dup(char** l_src, char** l_to, int item_size)
{
   char *p;
   int i;

   for (i = 0; l_src[i]; i++) ;
   p = (char *)(l_to + i + 1);
   for (i = 0; l_src[i]; i++)
   {
       int count = ( item_size ) ? item_size : strlen(l_src[i]) + 1;
       memcpy(p, l_src[i], count);
       l_to[i] = p;
       p += count;
   }
   l_to[i] = NULL;
   return p - (char *)l_to;
}

/* ----- hostent */

/* create a hostent entry
 *
 * Creates the entry with enough memory for the name, aliases
 * addresses, and the address pointers.  Also copies the name
 * and sets up all the pointers.
 *
 * NOTE: The alias and address lists must be allocated with room
 * for the NULL item terminating the list.  This is true even if
 * the list has no items ("aliases" and "addresses" must be
 * at least "1", a truly empty list is invalid).
 */
static struct WS_hostent *WS_create_he(char *name, int aliases, int aliases_size, int addresses, int address_length)
{
    struct WS_hostent *p_to;
    char *p;
    int size = (sizeof(struct WS_hostent) +
                strlen(name) + 1 +
                sizeof(char *) * aliases +
                aliases_size +
                sizeof(char *) * addresses +
                address_length * (addresses - 1)), i;

    if (!(p_to = check_buffer_he(size))) return NULL;
    memset(p_to, 0, size);

    /* Use the memory in the same way winsock does.
     * First set the pointer for aliases, second set the pointers for addresses.
     * Third fill the addresses indexes, fourth jump aliases names size.
     * Fifth fill the hostname.
     * NOTE: This method is valid for OS version's >= XP.
     */
    p = (char *)(p_to + 1);
    p_to->h_aliases = (char **)p;
    p += sizeof(char *)*aliases;

    p_to->h_addr_list = (char **)p;
    p += sizeof(char *)*addresses;

    for (i = 0, addresses--; i < addresses; i++, p += address_length)
        p_to->h_addr_list[i] = p;

    /* NOTE: h_aliases must be filled in manually because we don't know each string
     * size, leave these pointers NULL (already set to NULL by memset earlier).
     */
    p += aliases_size;

    p_to->h_name = p;
    strcpy(p, name);

    return p_to;
}

/* duplicate hostent entry
 * and handle all Win16/Win32 dependent things (struct size, ...) *correctly*.
 * Ditto for protoent and servent.
 */
static struct WS_hostent *WS_dup_he(const struct hostent* p_he)
{
    int i, addresses = 0, alias_size = 0;
    struct WS_hostent *p_to;
    char *p;

    for( i = 0; p_he->h_aliases[i]; i++) alias_size += strlen(p_he->h_aliases[i]) + 1;
    while (p_he->h_addr_list[addresses]) addresses++;

    p_to = WS_create_he(p_he->h_name, i + 1, alias_size, addresses + 1, p_he->h_length);

    if (!p_to) return NULL;
    p_to->h_addrtype = convert_af_u2w(p_he->h_addrtype);
    p_to->h_length = p_he->h_length;

    for(i = 0, p = p_to->h_addr_list[0]; p_he->h_addr_list[i]; i++, p += p_to->h_length)
        memcpy(p, p_he->h_addr_list[i], p_to->h_length);

    /* Fill the aliases after the IP data */
    for(i = 0; p_he->h_aliases[i]; i++)
    {
        p_to->h_aliases[i] = p;
        strcpy(p, p_he->h_aliases[i]);
        p += strlen(p) + 1;
    }

    return p_to;
}

/* ----- protoent */

static struct WS_protoent *WS_dup_pe(const struct protoent* p_pe)
{
    char *p;
    struct WS_protoent *p_to;

    int size = (sizeof(*p_pe) +
                strlen(p_pe->p_name) + 1 +
                list_size(p_pe->p_aliases, 0));

    if (!(p_to = check_buffer_pe(size))) return NULL;
    p_to->p_proto = p_pe->p_proto;

    p = (char *)(p_to + 1);
    p_to->p_name = p;
    strcpy(p, p_pe->p_name);
    p += strlen(p) + 1;

    p_to->p_aliases = (char **)p;
    list_dup(p_pe->p_aliases, p_to->p_aliases, 0);
    return p_to;
}

/* ----- servent */

static struct WS_servent *WS_dup_se(const struct servent* p_se)
{
    char *p;
    struct WS_servent *p_to;

    int size = (sizeof(*p_se) +
                strlen(p_se->s_proto) + 1 +
                strlen(p_se->s_name) + 1 +
                list_size(p_se->s_aliases, 0));

    if (!(p_to = check_buffer_se(size))) return NULL;
    p_to->s_port = p_se->s_port;

    p = (char *)(p_to + 1);
    p_to->s_name = p;
    strcpy(p, p_se->s_name);
    p += strlen(p) + 1;

    p_to->s_proto = p;
    strcpy(p, p_se->s_proto);
    p += strlen(p) + 1;

    p_to->s_aliases = (char **)p;
    list_dup(p_se->s_aliases, p_to->s_aliases, 0);
    return p_to;
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

static int WS2_recv_base( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                          LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
                          struct WS_sockaddr *lpFrom,
                          LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped,
                          LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
                          LPWSABUF lpControlBuffer )
{
    unsigned int i, options;
    int n, fd, err, overlapped;
    struct ws2_async *wsa, localwsa;
    BOOL is_blocking;
    DWORD timeout_start = GetTickCount();
    ULONG_PTR cvalue = (lpOverlapped && ((ULONG_PTR)lpOverlapped->hEvent & 1) == 0) ? (ULONG_PTR)lpOverlapped : 0;

    TRACE("socket %04lx, wsabuf %p, nbufs %d, flags %d, from %p, fromlen %d, ovl %p, func %p\n",
          s, lpBuffers, dwBufferCount, *lpFlags, lpFrom,
          (lpFromlen ? *lpFromlen : -1),
          lpOverlapped, lpCompletionRoutine);

    fd = get_sock_fd( s, FILE_READ_DATA, &options );
    TRACE( "fd=%d, options=%x\n", fd, options );

    if (fd == -1) return SOCKET_ERROR;

    overlapped = (lpOverlapped || lpCompletionRoutine) &&
        !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));
    if (overlapped || dwBufferCount > 1)
    {
        if (!(wsa = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET(struct ws2_async, iovec[dwBufferCount]) )))
        {
            err = WSAEFAULT;
            goto error;
        }
    }
    else
        wsa = &localwsa;

    wsa->hSocket     = SOCKET2HANDLE(s);
    wsa->flags       = *lpFlags;
    wsa->lpFlags     = lpFlags;
    wsa->addr        = lpFrom;
    wsa->addrlen.ptr = lpFromlen;
    wsa->control     = lpControlBuffer;
    wsa->n_iovecs    = dwBufferCount;
    wsa->first_iovec = 0;
    for (i = 0; i < dwBufferCount; i++)
    {
        /* check buffer first to trigger write watches */
        if (IsBadWritePtr( lpBuffers[i].buf, lpBuffers[i].len ))
        {
            err = WSAEFAULT;
            goto error;
        }
        wsa->iovec[i].iov_base = lpBuffers[i].buf;
        wsa->iovec[i].iov_len  = lpBuffers[i].len;
    }

    for (;;)
    {
        n = WS2_recv( fd, wsa );
        if (n == -1)
        {
            if (errno != EAGAIN)
            {
                int loc_errno = errno;
                err = wsaErrno();
                if (cvalue) WS_AddCompletion( s, cvalue, sock_get_ntstatus(loc_errno), 0 );
                goto error;
            }
        }
        else if (lpNumberOfBytesRecvd) *lpNumberOfBytesRecvd = n;

        if (overlapped)
        {
            IO_STATUS_BLOCK *iosb = lpOverlapped ? (IO_STATUS_BLOCK *)lpOverlapped : &wsa->local_iosb;

            wsa->user_overlapped = lpOverlapped;
            wsa->completion_func = lpCompletionRoutine;
            release_sock_fd( s, fd );

            if (n == -1)
            {
                iosb->u.Status = STATUS_PENDING;
                iosb->Information = 0;

                SERVER_START_REQ( register_async )
                {
                    req->type           = ASYNC_TYPE_READ;
                    req->async.handle   = wine_server_obj_handle( wsa->hSocket );
                    req->async.callback = wine_server_client_ptr( WS2_async_recv );
                    req->async.iosb     = wine_server_client_ptr( iosb );
                    req->async.arg      = wine_server_client_ptr( wsa );
                    req->async.event    = wine_server_obj_handle( lpCompletionRoutine ? 0 : lpOverlapped->hEvent );
                    req->async.cvalue   = cvalue;
                    err = wine_server_call( req );
                }
                SERVER_END_REQ;

                if (err != STATUS_PENDING) HeapFree( GetProcessHeap(), 0, wsa );
                WSASetLastError( NtStatusToWSAError( err ));
                return SOCKET_ERROR;
            }

            iosb->u.Status = STATUS_SUCCESS;
            iosb->Information = n;
            if (!wsa->completion_func)
            {
                if (cvalue) WS_AddCompletion( s, cvalue, STATUS_SUCCESS, n );
                if (lpOverlapped->hEvent) SetEvent( lpOverlapped->hEvent );
                HeapFree( GetProcessHeap(), 0, wsa );
            }
            else NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)ws2_async_apc,
                                   (ULONG_PTR)wsa, (ULONG_PTR)iosb, 0 );
            _enable_event(SOCKET2HANDLE(s), FD_READ, 0, 0);
            return 0;
        }

        if (n != -1) break;

        if ((err = _is_blocking( s, &is_blocking )))
        {
            err = NtStatusToWSAError( err );
            goto error;
        }

        if ( is_blocking )
        {
            struct pollfd pfd;
            int timeout = GET_RCVTIMEO(fd);
            if (timeout != -1)
            {
                timeout -= GetTickCount() - timeout_start;
                if (timeout < 0) timeout = 0;
            }

            pfd.fd = fd;
            pfd.events = POLLIN;
            if (*lpFlags & WS_MSG_OOB) pfd.events |= POLLPRI;

            if (!timeout || !poll( &pfd, 1, timeout ))
            {
                err = WSAETIMEDOUT;
                /* a timeout is not fatal */
                _enable_event(SOCKET2HANDLE(s), FD_READ, 0, 0);
                goto error;
            }
        }
        else
        {
            _enable_event(SOCKET2HANDLE(s), FD_READ, 0, 0);
            err = WSAEWOULDBLOCK;
            goto error;
        }
    }

    TRACE(" -> %i bytes\n", n);
    if (wsa != &localwsa) HeapFree( GetProcessHeap(), 0, wsa );
    release_sock_fd( s, fd );
    _enable_event(SOCKET2HANDLE(s), FD_READ, 0, 0);

    return 0;

error:
    if (wsa != &localwsa) HeapFree( GetProcessHeap(), 0, wsa );
    release_sock_fd( s, fd );
    WARN(" -> ERROR %d\n", err);
    WSASetLastError( err );
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSARecvFrom             (WS2_32.69)
 */
INT WINAPI WSARecvFrom( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                        LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct WS_sockaddr *lpFrom,
                        LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped,
                        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )

{
    return WS2_recv_base( s, lpBuffers, dwBufferCount,
                lpNumberOfBytesRecvd, lpFlags,
                lpFrom, lpFromlen,
                lpOverlapped, lpCompletionRoutine, NULL );
}

/***********************************************************************
 *              WSCInstallProvider             (WS2_32.88)
 */
INT WINAPI WSCInstallProvider( const LPGUID lpProviderId,
                               LPCWSTR lpszProviderDllPath,
                               const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
                               DWORD dwNumberOfEntries,
                               LPINT lpErrno )
{
    FIXME("(%s, %s, %p, %d, %p): stub !\n", debugstr_guid(lpProviderId),
          debugstr_w(lpszProviderDllPath), lpProtocolInfoList,
          dwNumberOfEntries, lpErrno);
    *lpErrno = 0;
    return 0;
}


/***********************************************************************
 *              WSCDeinstallProvider             (WS2_32.83)
 */
INT WINAPI WSCDeinstallProvider(LPGUID lpProviderId, LPINT lpErrno)
{
    FIXME("(%s, %p): stub !\n", debugstr_guid(lpProviderId), lpErrno);
    *lpErrno = 0;
    return 0;
}


/***********************************************************************
 *              WSAAccept                        (WS2_32.26)
 */
SOCKET WINAPI WSAAccept( SOCKET s, struct WS_sockaddr *addr, LPINT addrlen,
               LPCONDITIONPROC lpfnCondition, DWORD_PTR dwCallbackData)
{

       int ret = 0, size;
       WSABUF CallerId, CallerData, CalleeId, CalleeData;
       /*        QOS SQOS, GQOS; */
       GROUP g;
       SOCKET cs;
       SOCKADDR src_addr, dst_addr;

       TRACE("Socket %04lx, sockaddr %p, addrlen %p, fnCondition %p, dwCallbackData %ld\n",
               s, addr, addrlen, lpfnCondition, dwCallbackData);

       cs = WS_accept(s, addr, addrlen);
       if (cs == SOCKET_ERROR) return SOCKET_ERROR;
       if (!lpfnCondition) return cs;

       if (addr && addrlen)
       {
           CallerId.buf = (char *)addr;
           CallerId.len = *addrlen;
       }
       else
       {
           size = sizeof(src_addr);
           WS_getpeername(cs, &src_addr, &size);
           CallerId.buf = (char *)&src_addr;
           CallerId.len = size;
       }
       CallerData.buf = NULL;
       CallerData.len = 0;

       size = sizeof(dst_addr);
       WS_getsockname(cs, &dst_addr, &size);

       CalleeId.buf = (char *)&dst_addr;
       CalleeId.len = sizeof(dst_addr);

       ret = (*lpfnCondition)(&CallerId, &CallerData, NULL, NULL,
                       &CalleeId, &CalleeData, &g, dwCallbackData);

       switch (ret)
       {
               case CF_ACCEPT:
                       return cs;
               case CF_DEFER:
                       SERVER_START_REQ( set_socket_deferred )
                       {
                           req->handle = wine_server_obj_handle( SOCKET2HANDLE(s) );
                           req->deferred = wine_server_obj_handle( SOCKET2HANDLE(cs) );
                           if ( !wine_server_call_err ( req ) )
                           {
                               SetLastError( WSATRY_AGAIN );
                               WS_closesocket( cs );
                           }
                       }
                       SERVER_END_REQ;
                       return SOCKET_ERROR;
               case CF_REJECT:
                       WS_closesocket(cs);
                       SetLastError(WSAECONNREFUSED);
                       return SOCKET_ERROR;
               default:
                       FIXME("Unknown return type from Condition function\n");
                       SetLastError(WSAENOTSOCK);
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
 *              WSAInstallServiceClassA                  (WS2_32.48)
 */
int WINAPI WSAInstallServiceClassA(LPWSASERVICECLASSINFOA info)
{
    FIXME("Request to install service %s\n",debugstr_a(info->lpszServiceClassName));
    WSASetLastError(WSAEACCES);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSAInstallServiceClassW                  (WS2_32.49)
 */
int WINAPI WSAInstallServiceClassW(LPWSASERVICECLASSINFOW info)
{
    FIXME("Request to install service %s\n",debugstr_w(info->lpszServiceClassName));
    WSASetLastError(WSAEACCES);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSARemoveServiceClass                    (WS2_32.70)
 */
int WINAPI WSARemoveServiceClass(LPGUID info)
{
    FIXME("Request to remove service %p\n",info);
    WSASetLastError(WSATYPE_NOT_FOUND);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              inet_ntop                      (WS2_32.@)
 */
PCSTR WINAPI WS_inet_ntop( INT family, PVOID addr, PSTR buffer, SIZE_T len )
{
#ifdef HAVE_INET_NTOP
    struct WS_in6_addr *in6;
    struct WS_in_addr  *in;
    PCSTR pdst;

    TRACE("family %d, addr (%p), buffer (%p), len %ld\n", family, addr, buffer, len);
    if (!buffer)
    {
        WSASetLastError( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    switch (family)
    {
    case WS_AF_INET:
    {
        in = addr;
        pdst = inet_ntop( AF_INET, &in->WS_s_addr, buffer, len );
        break;
    }
    case WS_AF_INET6:
    {
        in6 = addr;
        pdst = inet_ntop( AF_INET6, in6->WS_s6_addr, buffer, len );
        break;
    }
    default:
        WSASetLastError( WSAEAFNOSUPPORT );
        return NULL;
    }

    if (!pdst) WSASetLastError( STATUS_INVALID_PARAMETER );
    return pdst;
#else
    FIXME( "not supported on this platform\n" );
    WSASetLastError( WSAEAFNOSUPPORT );
    return NULL;
#endif
}

/***********************************************************************
 *              WSAStringToAddressA                      (WS2_32.80)
 */
INT WINAPI WSAStringToAddressA(LPSTR AddressString,
                               INT AddressFamily,
                               LPWSAPROTOCOL_INFOA lpProtocolInfo,
                               LPSOCKADDR lpAddress,
                               LPINT lpAddressLength)
{
    INT res=0;
    LPSTR workBuffer=NULL,ptrPort;

    TRACE( "(%s, %x, %p, %p, %p)\n", debugstr_a(AddressString), AddressFamily,
           lpProtocolInfo, lpAddress, lpAddressLength );

    if (!lpAddressLength || !lpAddress) return SOCKET_ERROR;

    if (!AddressString)
    {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    if (lpProtocolInfo)
        FIXME("ProtocolInfo not implemented.\n");

    workBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                            strlen(AddressString) + 1);
    if (!workBuffer)
    {
        WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
        return SOCKET_ERROR;
    }

    strcpy(workBuffer, AddressString);

    switch(AddressFamily)
    {
    case WS_AF_INET:
    {
        struct in_addr inetaddr;

        /* If lpAddressLength is too small, tell caller the size we need */
        if (*lpAddressLength < sizeof(SOCKADDR_IN))
        {
            *lpAddressLength = sizeof(SOCKADDR_IN);
            res = WSAEFAULT;
            break;
        }
        *lpAddressLength = sizeof(SOCKADDR_IN);
        memset(lpAddress, 0, sizeof(SOCKADDR_IN));

        ((LPSOCKADDR_IN)lpAddress)->sin_family = WS_AF_INET;

        ptrPort = strchr(workBuffer, ':');
        if(ptrPort)
        {
            ((LPSOCKADDR_IN)lpAddress)->sin_port = htons(atoi(ptrPort+1));
            *ptrPort = '\0';
        }
        else
        {
            ((LPSOCKADDR_IN)lpAddress)->sin_port = 0;
        }

        if(inet_aton(workBuffer, &inetaddr) > 0)
        {
            ((LPSOCKADDR_IN)lpAddress)->sin_addr.WS_s_addr = inetaddr.s_addr;
            res = 0;
        }
        else
            res = WSAEINVAL;

        break;

    }
    case WS_AF_INET6:
    {
        struct in6_addr inetaddr;
        /* If lpAddressLength is too small, tell caller the size we need */
        if (*lpAddressLength < sizeof(SOCKADDR_IN6))
        {
            *lpAddressLength = sizeof(SOCKADDR_IN6);
            res = WSAEFAULT;
            break;
        }
#ifdef HAVE_INET_PTON
        *lpAddressLength = sizeof(SOCKADDR_IN6);
        memset(lpAddress, 0, sizeof(SOCKADDR_IN6));

        ((LPSOCKADDR_IN6)lpAddress)->sin6_family = WS_AF_INET6;

        /* This one is a bit tricky. An IPv6 address contains colons, so the
         * check from IPv4 doesn't work like that. However, IPv6 addresses that
         * contain a port are written with braces like [fd12:3456:7890::1]:12345
         * so what we will do is to look for ']', check if the next char is a
         * colon, and if it is, parse the port as in IPv4. */

        ptrPort = strchr(workBuffer, ']');
        if(ptrPort && *(++ptrPort) == ':')
        {
            ((LPSOCKADDR_IN6)lpAddress)->sin6_port = htons(atoi(ptrPort+1));
            *ptrPort = '\0';
        }
        else
        {
            ((LPSOCKADDR_IN6)lpAddress)->sin6_port = 0;
        }

        if(inet_pton(AF_INET6, workBuffer, &inetaddr) > 0)
        {
            memcpy(&((LPSOCKADDR_IN6)lpAddress)->sin6_addr, &inetaddr,
                    sizeof(struct in6_addr));
            res = 0;
        }
        else
#endif /* HAVE_INET_PTON */
            res = WSAEINVAL;

        break;
    }
    default:
        /* According to MSDN, only AF_INET and AF_INET6 are supported. */
        TRACE("Unsupported address family specified: %d.\n", AddressFamily);
        res = WSAEINVAL;
    }

    HeapFree(GetProcessHeap(), 0, workBuffer);

    if (!res) return 0;
    WSASetLastError(res);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSAStringToAddressW                      (WS2_32.81)
 *
 * FIXME: Does anybody know if this function allows using Hebrew/Arabic/Chinese... digits?
 * If this should be the case, it would be required to map these digits
 * to Unicode digits (0-9) using FoldString first.
 */
INT WINAPI WSAStringToAddressW(LPWSTR AddressString,
                               INT AddressFamily,
                               LPWSAPROTOCOL_INFOW lpProtocolInfo,
                               LPSOCKADDR lpAddress,
                               LPINT lpAddressLength)
{
    INT sBuffer,res=0;
    LPSTR workBuffer=NULL;
    WSAPROTOCOL_INFOA infoA;
    LPWSAPROTOCOL_INFOA lpProtoInfoA = NULL;

    TRACE( "(%s, %x, %p, %p, %p)\n", debugstr_w(AddressString), AddressFamily, lpProtocolInfo,
           lpAddress, lpAddressLength );

    if (!lpAddressLength || !lpAddress) return SOCKET_ERROR;

    /* if ProtocolInfo is available - convert to ANSI variant */
    if (lpProtocolInfo)
    {
        lpProtoInfoA = &infoA;
        memcpy( lpProtoInfoA, lpProtocolInfo, FIELD_OFFSET( WSAPROTOCOL_INFOA, szProtocol ) );

        if (!WideCharToMultiByte( CP_ACP, 0, lpProtocolInfo->szProtocol, -1,
                                  lpProtoInfoA->szProtocol, WSAPROTOCOL_LEN+1, NULL, NULL ))
        {
            WSASetLastError( WSAEINVAL);
            return SOCKET_ERROR;
        }
    }

    if (AddressString)
    {
        /* Translate AddressString to ANSI code page - assumes that only
           standard digits 0-9 are used with this API call */
        sBuffer = WideCharToMultiByte( CP_ACP, 0, AddressString, -1, NULL, 0, NULL, NULL );
        workBuffer = HeapAlloc( GetProcessHeap(), 0, sBuffer );

        if (workBuffer)
        {
            WideCharToMultiByte( CP_ACP, 0, AddressString, -1, workBuffer, sBuffer, NULL, NULL );
            res = WSAStringToAddressA(workBuffer,AddressFamily,lpProtoInfoA,
                                      lpAddress,lpAddressLength);
            HeapFree( GetProcessHeap(), 0, workBuffer );
            return res;
        }
        else
            res = WSA_NOT_ENOUGH_MEMORY;
    }
    else
        res = WSAEINVAL;

    WSASetLastError(res);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSAAddressToStringA                      (WS2_32.27)
 *
 *  See WSAAddressToStringW
 */
INT WINAPI WSAAddressToStringA( LPSOCKADDR sockaddr, DWORD len,
                                LPWSAPROTOCOL_INFOA info, LPSTR string,
                                LPDWORD lenstr )
{
    DWORD size;
    CHAR buffer[54]; /* 32 digits + 7':' + '[' + '%" + 5 digits + ']:' + 5 digits + '\0' */
    CHAR *p;

    TRACE( "(%p, %d, %p, %p, %p)\n", sockaddr, len, info, string, lenstr );

    if (!sockaddr) return SOCKET_ERROR;
    if (!string || !lenstr) return SOCKET_ERROR;

    switch(sockaddr->sa_family)
    {
    case WS_AF_INET:
        if (len < sizeof(SOCKADDR_IN)) return SOCKET_ERROR;
        sprintf( buffer, "%u.%u.%u.%u:%u",
               (unsigned int)(ntohl( ((SOCKADDR_IN *)sockaddr)->sin_addr.WS_s_addr ) >> 24 & 0xff),
               (unsigned int)(ntohl( ((SOCKADDR_IN *)sockaddr)->sin_addr.WS_s_addr ) >> 16 & 0xff),
               (unsigned int)(ntohl( ((SOCKADDR_IN *)sockaddr)->sin_addr.WS_s_addr ) >> 8 & 0xff),
               (unsigned int)(ntohl( ((SOCKADDR_IN *)sockaddr)->sin_addr.WS_s_addr ) & 0xff),
               ntohs( ((SOCKADDR_IN *)sockaddr)->sin_port ) );

        p = strchr( buffer, ':' );
        if (!((SOCKADDR_IN *)sockaddr)->sin_port) *p = 0;
        break;

    case WS_AF_INET6:
    {
        struct WS_sockaddr_in6 *sockaddr6 = (LPSOCKADDR_IN6) sockaddr;

        buffer[0] = 0;
        if (len < sizeof(SOCKADDR_IN6)) return SOCKET_ERROR;
        if ((sockaddr6->sin6_port))
            strcpy(buffer, "[");
        if (!WS_inet_ntop(WS_AF_INET6, &sockaddr6->sin6_addr, buffer+strlen(buffer), sizeof(buffer)))
        {
            WSASetLastError(WSAEINVAL);
            return SOCKET_ERROR;
        }
        if ((sockaddr6->sin6_scope_id))
            sprintf(buffer+strlen(buffer), "%%%u", sockaddr6->sin6_scope_id);
        if ((sockaddr6->sin6_port))
            sprintf(buffer+strlen(buffer), "]:%u", ntohs(sockaddr6->sin6_port));
        break;
    }

    default:
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    size = strlen( buffer ) + 1;

    if (*lenstr <  size)
    {
        *lenstr = size;
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    TRACE("=> %s,%u bytes\n", debugstr_a(buffer), size);
    *lenstr = size;
    strcpy( string, buffer );
    return 0;
}

/***********************************************************************
 *              WSAAddressToStringW                      (WS2_32.28)
 *
 * Convert a sockaddr address into a readable address string. 
 *
 * PARAMS
 *  sockaddr [I]    Pointer to a sockaddr structure.
 *  len      [I]    Size of the sockaddr structure.
 *  info     [I]    Pointer to a WSAPROTOCOL_INFOW structure (optional).
 *  string   [I/O]  Pointer to a buffer to receive the address string.
 *  lenstr   [I/O]  Size of the receive buffer in WCHARs.
 *
 * RETURNS
 *  Success: 0
 *  Failure: SOCKET_ERROR
 *
 * NOTES
 *  The 'info' parameter is ignored.
 */
INT WINAPI WSAAddressToStringW( LPSOCKADDR sockaddr, DWORD len,
                                LPWSAPROTOCOL_INFOW info, LPWSTR string,
                                LPDWORD lenstr )
{
    INT   ret;
    DWORD size;
    WCHAR buffer[54]; /* 32 digits + 7':' + '[' + '%" + 5 digits + ']:' + 5 digits + '\0' */
    CHAR bufAddr[54];

    TRACE( "(%p, %d, %p, %p, %p)\n", sockaddr, len, info, string, lenstr );

    size = *lenstr;
    ret = WSAAddressToStringA(sockaddr, len, NULL, bufAddr, &size);

    if (ret) return ret;

    MultiByteToWideChar( CP_ACP, 0, bufAddr, size, buffer, sizeof( buffer )/sizeof(WCHAR));

    if (*lenstr <  size)
    {
        *lenstr = size;
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    TRACE("=> %s,%u bytes\n", debugstr_w(buffer), size);
    *lenstr = size;
    lstrcpyW( string, buffer );
    return 0;
}

/***********************************************************************
 *              WSAEnumNameSpaceProvidersA                  (WS2_32.34)
 */
INT WINAPI WSAEnumNameSpaceProvidersA( LPDWORD len, LPWSANAMESPACE_INFOA buffer )
{
    FIXME( "(%p %p) Stub!\n", len, buffer );
    return 0;
}

/***********************************************************************
 *              WSAEnumNameSpaceProvidersW                  (WS2_32.35)
 */
INT WINAPI WSAEnumNameSpaceProvidersW( LPDWORD len, LPWSANAMESPACE_INFOW buffer )
{
    FIXME( "(%p %p) Stub!\n", len, buffer );
    return 0;
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
 *              WSAGetServiceClassInfoA                     (WS2_32.42)
 */
INT WINAPI WSAGetServiceClassInfoA( LPGUID provider, LPGUID service, LPDWORD len,
                                    LPWSASERVICECLASSINFOA info )
{
    FIXME( "(%s %s %p %p) Stub!\n", debugstr_guid(provider), debugstr_guid(service),
           len, info );
    WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    return SOCKET_ERROR; 
}

/***********************************************************************
 *              WSAGetServiceClassInfoW                     (WS2_32.43)
 */
INT WINAPI WSAGetServiceClassInfoW( LPGUID provider, LPGUID service, LPDWORD len,
                                    LPWSASERVICECLASSINFOW info )
{
    FIXME( "(%s %s %p %p) Stub!\n", debugstr_guid(provider), debugstr_guid(service),
           len, info );
    WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSAGetServiceClassNameByClassIdA            (WS2_32.44)
 */
INT WINAPI WSAGetServiceClassNameByClassIdA( LPGUID class, LPSTR service, LPDWORD len )
{
    FIXME( "(%s %p %p) Stub!\n", debugstr_guid(class), service, len );
    WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSAGetServiceClassNameByClassIdW            (WS2_32.45)
 */
INT WINAPI WSAGetServiceClassNameByClassIdW( LPGUID class, LPWSTR service, LPDWORD len )
{
    FIXME( "(%s %p %p) Stub!\n", debugstr_guid(class), service, len );
    WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSALookupServiceBeginA                       (WS2_32.59)
 */
INT WINAPI WSALookupServiceBeginA( LPWSAQUERYSETA lpqsRestrictions,
                                   DWORD dwControlFlags,
                                   LPHANDLE lphLookup)
{
    FIXME("(%p 0x%08x %p) Stub!\n", lpqsRestrictions, dwControlFlags,
            lphLookup);
    WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSALookupServiceBeginW                       (WS2_32.60)
 */
INT WINAPI WSALookupServiceBeginW( LPWSAQUERYSETW lpqsRestrictions,
                                   DWORD dwControlFlags,
                                   LPHANDLE lphLookup)
{
    FIXME("(%p 0x%08x %p) Stub!\n", lpqsRestrictions, dwControlFlags,
            lphLookup);
    WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSALookupServiceEnd                          (WS2_32.61)
 */
INT WINAPI WSALookupServiceEnd( HANDLE lookup )
{
    FIXME("(%p) Stub!\n", lookup );
    return 0;
}

/***********************************************************************
 *              WSALookupServiceNextA                       (WS2_32.62)
 */
INT WINAPI WSALookupServiceNextA( HANDLE lookup, DWORD flags, LPDWORD len, LPWSAQUERYSETA results )
{
    FIXME( "(%p 0x%08x %p %p) Stub!\n", lookup, flags, len, results );
    WSASetLastError(WSA_E_NO_MORE);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSALookupServiceNextW                       (WS2_32.63)
 */
INT WINAPI WSALookupServiceNextW( HANDLE lookup, DWORD flags, LPDWORD len, LPWSAQUERYSETW results )
{
    FIXME( "(%p 0x%08x %p %p) Stub!\n", lookup, flags, len, results );
    WSASetLastError(WSA_E_NO_MORE);
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSANtohl                                   (WS2_32.64)
 */
INT WINAPI WSANtohl( SOCKET s, WS_u_long netlong, WS_u_long* lphostlong )
{
    TRACE( "(0x%04lx 0x%08x %p)\n", s, netlong, lphostlong );

    if (!lphostlong) return WSAEFAULT;

    *lphostlong = ntohl( netlong );
    return 0;
}

/***********************************************************************
 *              WSANtohs                                   (WS2_32.65)
 */
INT WINAPI WSANtohs( SOCKET s, WS_u_short netshort, WS_u_short* lphostshort )
{
    TRACE( "(0x%04lx 0x%08x %p)\n", s, netshort, lphostshort );

    if (!lphostshort) return WSAEFAULT;

    *lphostshort = ntohs( netshort );
    return 0;
}

/***********************************************************************
 *              WSAProviderConfigChange                     (WS2_32.66)
 */
INT WINAPI WSAProviderConfigChange( LPHANDLE handle, LPWSAOVERLAPPED overlapped,
                                    LPWSAOVERLAPPED_COMPLETION_ROUTINE completion )
{
    FIXME( "(%p %p %p) Stub!\n", handle, overlapped, completion );
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSARecvDisconnect                           (WS2_32.68)
 */
INT WINAPI WSARecvDisconnect( SOCKET s, LPWSABUF disconnectdata )
{
    TRACE( "(0x%04lx %p)\n", s, disconnectdata );

    return WS_shutdown( s, SD_RECEIVE );
}

/***********************************************************************
 *              WSASetServiceA                              (WS2_32.76)
 */
INT WINAPI WSASetServiceA( LPWSAQUERYSETA query, WSAESETSERVICEOP operation, DWORD flags )
{
    FIXME( "(%p 0x%08x 0x%08x) Stub!\n", query, operation, flags );
    return 0;
}

/***********************************************************************
 *              WSASetServiceW                              (WS2_32.77)
 */
INT WINAPI WSASetServiceW( LPWSAQUERYSETW query, WSAESETSERVICEOP operation, DWORD flags )
{
    FIXME( "(%p 0x%08x 0x%08x) Stub!\n", query, operation, flags );
    return 0;
}

/***********************************************************************
 *              WSCEnableNSProvider                         (WS2_32.84)
 */
INT WINAPI WSCEnableNSProvider( LPGUID provider, BOOL enable )
{
    FIXME( "(%s 0x%08x) Stub!\n", debugstr_guid(provider), enable );
    return 0;
}

/***********************************************************************
 *              WSCGetProviderPath                          (WS2_32.86)
 */
INT WINAPI WSCGetProviderPath( LPGUID provider, LPWSTR path, LPINT len, LPINT errcode )
{
    FIXME( "(%s %p %p %p) Stub!\n", debugstr_guid(provider), path, len, errcode );

    if (!errcode || !provider || !len) return WSAEFAULT;

    *errcode = WSAEINVAL;
    return SOCKET_ERROR;
}

/***********************************************************************
 *              WSCInstallNameSpace                         (WS2_32.87)
 */
INT WINAPI WSCInstallNameSpace( LPWSTR identifier, LPWSTR path, DWORD namespace,
                                DWORD version, LPGUID provider )
{
    FIXME( "(%s %s 0x%08x 0x%08x %s) Stub!\n", debugstr_w(identifier), debugstr_w(path),
           namespace, version, debugstr_guid(provider) );
    return 0;
}

/***********************************************************************
 *              WSCUnInstallNameSpace                       (WS2_32.89)
 */
INT WINAPI WSCUnInstallNameSpace( LPGUID lpProviderId )
{
    FIXME("(%p) Stub!\n", lpProviderId);
    return NO_ERROR;
}

/***********************************************************************
 *              WSCWriteProviderOrder                       (WS2_32.91)
 */
INT WINAPI WSCWriteProviderOrder( LPDWORD entry, DWORD number )
{
    FIXME("(%p 0x%08x) Stub!\n", entry, number);
    return 0;
}

/***********************************************************************
 *              WSANSPIoctl                       (WS2_32.91)
 */
INT WINAPI WSANSPIoctl( HANDLE hLookup, DWORD dwControlCode, LPVOID lpvInBuffer,
                        DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                        LPDWORD lpcbBytesReturned, LPWSACOMPLETION lpCompletion )
{
    FIXME("(%p, 0x%08x, %p, 0x%08x, %p, 0x%08x, %p, %p) Stub!\n", hLookup, dwControlCode,
    lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpCompletion);
    WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    return SOCKET_ERROR;
}

/*****************************************************************************
 *          WSAEnumProtocolsA       [WS2_32.@]
 *
 *    see function WSAEnumProtocolsW
 */
INT WINAPI WSAEnumProtocolsA( LPINT protocols, LPWSAPROTOCOL_INFOA buffer, LPDWORD len )
{
    return WS_EnumProtocols( FALSE, protocols, (LPWSAPROTOCOL_INFOW) buffer, len);
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
INT WINAPI WSAEnumProtocolsW( LPINT protocols, LPWSAPROTOCOL_INFOW buffer, LPDWORD len )
{
    return WS_EnumProtocols( TRUE, protocols, buffer, len);
}

/*****************************************************************************
 *          WSCEnumProtocols        [WS2_32.@]
 *
 * PARAMS
 *  protocols [I]   Null-terminated array of iProtocol values.
 *  buffer    [O]   Buffer of WSAPROTOCOL_INFOW structures.
 *  len       [I/O] Size of buffer on input/output.
 *  errno     [O]   Error code.
 *
 * RETURNS
 *  Success: number of protocols to be reported on.
 *  Failure: SOCKET_ERROR. error is in errno.
 *
 * BUGS
 *  Doesn't supply info on layered protocols.
 *
 */
INT WINAPI WSCEnumProtocols( LPINT protocols, LPWSAPROTOCOL_INFOW buffer, LPDWORD len, LPINT err )
{
    INT ret = WSAEnumProtocolsW( protocols, buffer, len );

    if (ret == SOCKET_ERROR) *err = WSAENOBUFS;

    return ret;
}
