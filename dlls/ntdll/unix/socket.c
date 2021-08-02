/*
 * Windows sockets
 *
 * Copyright 2021 Zebediah Figura for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_IFADDRS_H
# include <ifaddrs.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif

#ifdef HAVE_NETIPX_IPX_H
# include <netipx/ipx.h>
#elif defined(HAVE_LINUX_IPX_H)
# ifdef HAVE_ASM_TYPES_H
#  include <asm/types.h>
# endif
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/ipx.h>
#endif
#if defined(SOL_IPX) || defined(SO_DEFAULT_HEADERS)
# define HAS_IPX
#endif

#ifdef HAVE_LINUX_IRDA_H
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/irda.h>
# define HAS_IRDA
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winioctl.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "mswsock.h"
#include "mstcpip.h"
#include "ws2tcpip.h"
#include "wsipx.h"
#include "af_irda.h"
#include "wine/afd.h"

#include "unix_private.h"

#if !defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
/* TCP_KEEPALIVE is the Mac OS name for TCP_KEEPIDLE */
#define TCP_KEEPIDLE TCP_KEEPALIVE
#endif

#if defined(linux) && !defined(IP_UNICAST_IF)
#define IP_UNICAST_IF 50
#endif

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

#define FILE_USE_FILE_POINTER_POSITION ((LONGLONG)-2)

static async_data_t server_async( HANDLE handle, struct async_fileio *user, HANDLE event,
                                  PIO_APC_ROUTINE apc, void *apc_context, IO_STATUS_BLOCK *io )
{
    async_data_t async;
    async.handle      = wine_server_obj_handle( handle );
    async.user        = wine_server_client_ptr( user );
    async.iosb        = wine_server_client_ptr( io );
    async.event       = wine_server_obj_handle( event );
    async.apc         = wine_server_client_ptr( apc );
    async.apc_context = wine_server_client_ptr( apc_context );
    return async;
}

static NTSTATUS wait_async( HANDLE handle, BOOL alertable )
{
    return NtWaitForSingleObject( handle, alertable, NULL );
}

union unix_sockaddr
{
    struct sockaddr addr;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
#ifdef HAS_IPX
    struct sockaddr_ipx ipx;
#endif
#ifdef HAS_IRDA
    struct sockaddr_irda irda;
#endif
};

struct async_recv_ioctl
{
    struct async_fileio io;
    WSABUF *control;
    struct WS_sockaddr *addr;
    int *addr_len;
    DWORD *ret_flags;
    int unix_flags;
    unsigned int count;
    struct iovec iov[1];
};

struct async_send_ioctl
{
    struct async_fileio io;
    const struct WS_sockaddr *addr;
    int addr_len;
    int unix_flags;
    unsigned int sent_len;
    unsigned int count;
    unsigned int iov_cursor;
    struct iovec iov[1];
};

struct async_transmit_ioctl
{
    struct async_fileio io;
    HANDLE file;
    char *buffer;
    unsigned int buffer_size;   /* allocated size of buffer */
    unsigned int read_len;      /* amount of valid data currently in the buffer */
    unsigned int head_cursor;   /* amount of header data already sent */
    unsigned int file_cursor;   /* amount of file data already sent */
    unsigned int buffer_cursor; /* amount of data currently in the buffer already sent */
    unsigned int tail_cursor;   /* amount of tail data already sent */
    unsigned int file_len;      /* total file length to send */
    DWORD flags;
    TRANSMIT_FILE_BUFFERS buffers;
    LARGE_INTEGER offset;
};

static NTSTATUS sock_errno_to_status( int err )
{
    switch (err)
    {
        case EBADF:             return STATUS_INVALID_HANDLE;
        case EBUSY:             return STATUS_DEVICE_BUSY;
        case EPERM:
        case EACCES:            return STATUS_ACCESS_DENIED;
        case EFAULT:            return STATUS_ACCESS_VIOLATION;
        case EINVAL:            return STATUS_INVALID_PARAMETER;
        case ENFILE:
        case EMFILE:            return STATUS_TOO_MANY_OPENED_FILES;
        case EINPROGRESS:
        case EWOULDBLOCK:       return STATUS_DEVICE_NOT_READY;
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
        case EADDRINUSE:        return STATUS_SHARING_VIOLATION;
        case EADDRNOTAVAIL:     return STATUS_INVALID_PARAMETER;
        case ECONNREFUSED:      return STATUS_CONNECTION_REFUSED;
        case ESHUTDOWN:         return STATUS_PIPE_DISCONNECTED;
        case ENOTCONN:          return STATUS_INVALID_CONNECTION;
        case ETIMEDOUT:         return STATUS_IO_TIMEOUT;
        case ENETUNREACH:       return STATUS_NETWORK_UNREACHABLE;
        case EHOSTUNREACH:      return STATUS_HOST_UNREACHABLE;
        case ENETDOWN:          return STATUS_NETWORK_BUSY;
        case EPIPE:
        case ECONNRESET:        return STATUS_CONNECTION_RESET;
        case ECONNABORTED:      return STATUS_CONNECTION_ABORTED;
        case EISCONN:           return STATUS_CONNECTION_ACTIVE;

        case 0:                 return STATUS_SUCCESS;
        default:
            FIXME( "unknown errno %d\n", err );
            return STATUS_UNSUCCESSFUL;
    }
}

static socklen_t sockaddr_to_unix( const struct WS_sockaddr *wsaddr, int wsaddrlen, union unix_sockaddr *uaddr )
{
    memset( uaddr, 0, sizeof(*uaddr) );

    switch (wsaddr->sa_family)
    {
    case WS_AF_INET:
    {
        struct WS_sockaddr_in win = {0};

        if (wsaddrlen < sizeof(win)) return 0;
        memcpy( &win, wsaddr, sizeof(win) );
        uaddr->in.sin_family = AF_INET;
        uaddr->in.sin_port = win.sin_port;
        memcpy( &uaddr->in.sin_addr, &win.sin_addr, sizeof(win.sin_addr) );
        return sizeof(uaddr->in);
    }

    case WS_AF_INET6:
    {
        struct WS_sockaddr_in6 win = {0};

        if (wsaddrlen < sizeof(win)) return 0;
        memcpy( &win, wsaddr, sizeof(win) );
        uaddr->in6.sin6_family = AF_INET6;
        uaddr->in6.sin6_port = win.sin6_port;
        uaddr->in6.sin6_flowinfo = win.sin6_flowinfo;
        memcpy( &uaddr->in6.sin6_addr, &win.sin6_addr, sizeof(win.sin6_addr) );
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
        uaddr->in6.sin6_scope_id = win.sin6_scope_id;
#endif
        return sizeof(uaddr->in6);
    }

#ifdef HAS_IPX
    case WS_AF_IPX:
    {
        struct WS_sockaddr_ipx win = {0};

        if (wsaddrlen < sizeof(win)) return 0;
        memcpy( &win, wsaddr, sizeof(win) );
        uaddr->ipx.sipx_family = AF_IPX;
        memcpy( &uaddr->ipx.sipx_network, win.sa_netnum, sizeof(win.sa_netnum) );
        memcpy( &uaddr->ipx.sipx_node, win.sa_nodenum, sizeof(win.sa_nodenum) );
        uaddr->ipx.sipx_port = win.sa_socket;
        return sizeof(uaddr->ipx);
    }
#endif

#ifdef HAS_IRDA
    case WS_AF_IRDA:
    {
        SOCKADDR_IRDA win = {0};
        unsigned int lsap_sel;

        if (wsaddrlen < sizeof(win)) return 0;
        memcpy( &win, wsaddr, sizeof(win) );
        uaddr->irda.sir_family = AF_IRDA;
        if (sscanf( win.irdaServiceName, "LSAP-SEL%u", &lsap_sel ) == 1)
            uaddr->irda.sir_lsap_sel = lsap_sel;
        else
        {
            uaddr->irda.sir_lsap_sel = LSAP_ANY;
            memcpy( uaddr->irda.sir_name, win.irdaServiceName, sizeof(win.irdaServiceName) );
        }
        memcpy( &uaddr->irda.sir_addr, win.irdaDeviceID, sizeof(win.irdaDeviceID) );
        return sizeof(uaddr->irda);
    }
#endif

    case WS_AF_UNSPEC:
        switch (wsaddrlen)
        {
        default: /* likely an ipv4 address */
        case sizeof(struct WS_sockaddr_in):
            return sizeof(uaddr->in);

#ifdef HAS_IPX
        case sizeof(struct WS_sockaddr_ipx):
            return sizeof(uaddr->ipx);
#endif

#ifdef HAS_IRDA
        case sizeof(SOCKADDR_IRDA):
            return sizeof(uaddr->irda);
#endif

        case sizeof(struct WS_sockaddr_in6):
            return sizeof(uaddr->in6);
        }

    default:
        FIXME( "unknown address family %u\n", wsaddr->sa_family );
        return 0;
    }
}

static int sockaddr_from_unix( const union unix_sockaddr *uaddr, struct WS_sockaddr *wsaddr, socklen_t wsaddrlen )
{
    memset( wsaddr, 0, wsaddrlen );

    switch (uaddr->addr.sa_family)
    {
    case AF_INET:
    {
        struct WS_sockaddr_in win = {0};

        if (wsaddrlen < sizeof(win)) return -1;
        win.sin_family = WS_AF_INET;
        win.sin_port = uaddr->in.sin_port;
        memcpy( &win.sin_addr, &uaddr->in.sin_addr, sizeof(win.sin_addr) );
        memcpy( wsaddr, &win, sizeof(win) );
        return sizeof(win);
    }

    case AF_INET6:
    {
        struct WS_sockaddr_in6 win = {0};

        if (wsaddrlen < sizeof(win)) return -1;
        win.sin6_family = WS_AF_INET6;
        win.sin6_port = uaddr->in6.sin6_port;
        win.sin6_flowinfo = uaddr->in6.sin6_flowinfo;
        memcpy( &win.sin6_addr, &uaddr->in6.sin6_addr, sizeof(win.sin6_addr) );
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
        win.sin6_scope_id = uaddr->in6.sin6_scope_id;
#endif
        memcpy( wsaddr, &win, sizeof(win) );
        return sizeof(win);
    }

#ifdef HAS_IPX
    case AF_IPX:
    {
        struct WS_sockaddr_ipx win = {0};

        if (wsaddrlen < sizeof(win)) return -1;
        win.sa_family = WS_AF_IPX;
        memcpy( win.sa_netnum, &uaddr->ipx.sipx_network, sizeof(win.sa_netnum) );
        memcpy( win.sa_nodenum, &uaddr->ipx.sipx_node, sizeof(win.sa_nodenum) );
        win.sa_socket = uaddr->ipx.sipx_port;
        memcpy( wsaddr, &win, sizeof(win) );
        return sizeof(win);
    }
#endif

#ifdef HAS_IRDA
    case AF_IRDA:
    {
        SOCKADDR_IRDA win;

        if (wsaddrlen < sizeof(win)) return -1;
        win.irdaAddressFamily = WS_AF_IRDA;
        memcpy( win.irdaDeviceID, &uaddr->irda.sir_addr, sizeof(win.irdaDeviceID) );
        if (uaddr->irda.sir_lsap_sel != LSAP_ANY)
            snprintf( win.irdaServiceName, sizeof(win.irdaServiceName), "LSAP-SEL%u", uaddr->irda.sir_lsap_sel );
        else
            memcpy( win.irdaServiceName, uaddr->irda.sir_name, sizeof(win.irdaServiceName) );
        memcpy( wsaddr, &win, sizeof(win) );
        return sizeof(win);
    }
#endif

    case AF_UNSPEC:
        return 0;

    default:
        FIXME( "unknown address family %d\n", uaddr->addr.sa_family );
        return -1;
    }
}

#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
#if defined(IP_PKTINFO) || defined(IP_RECVDSTADDR)
static WSACMSGHDR *fill_control_message( int level, int type, WSACMSGHDR *current, ULONG *maxsize, void *data, int len )
{
    ULONG msgsize = sizeof(WSACMSGHDR) + WSA_CMSG_ALIGN(len);
    char *ptr = (char *) current + sizeof(WSACMSGHDR);

    if (msgsize > *maxsize)
        return NULL;
    *maxsize -= msgsize;
    current->cmsg_len = sizeof(WSACMSGHDR) + len;
    current->cmsg_level = level;
    current->cmsg_type = type;
    memcpy(ptr, data, len);
    return (WSACMSGHDR *)(ptr + WSA_CMSG_ALIGN(len));
}
#endif /* defined(IP_PKTINFO) || defined(IP_RECVDSTADDR) */

static int convert_control_headers(struct msghdr *hdr, WSABUF *control)
{
#if defined(IP_PKTINFO) || defined(IP_RECVDSTADDR)
    WSACMSGHDR *cmsg_win = (WSACMSGHDR *)control->buf, *ptr;
    ULONG ctlsize = control->len;
    struct cmsghdr *cmsg_unix;

    ptr = cmsg_win;
    for (cmsg_unix = CMSG_FIRSTHDR(hdr); cmsg_unix != NULL; cmsg_unix = CMSG_NXTHDR(hdr, cmsg_unix))
    {
        switch (cmsg_unix->cmsg_level)
        {
            case IPPROTO_IP:
                switch (cmsg_unix->cmsg_type)
                {
#if defined(IP_PKTINFO)
                    case IP_PKTINFO:
                    {
                        const struct in_pktinfo *data_unix = (struct in_pktinfo *)CMSG_DATA(cmsg_unix);
                        struct WS_in_pktinfo data_win;

                        memcpy( &data_win.ipi_addr, &data_unix->ipi_addr.s_addr, 4 ); /* 4 bytes = 32 address bits */
                        data_win.ipi_ifindex = data_unix->ipi_ifindex;
                        ptr = fill_control_message( WS_IPPROTO_IP, WS_IP_PKTINFO, ptr, &ctlsize,
                                                    (void *)&data_win, sizeof(data_win) );
                        if (!ptr) goto error;
                        break;
                    }
#elif defined(IP_RECVDSTADDR)
                    case IP_RECVDSTADDR:
                    {
                        const struct in_addr *addr_unix = (struct in_addr *)CMSG_DATA(cmsg_unix);
                        struct WS_in_pktinfo data_win;

                        memcpy( &data_win.ipi_addr, &addr_unix->s_addr, 4 ); /* 4 bytes = 32 address bits */
                        data_win.ipi_ifindex = 0; /* FIXME */
                        ptr = fill_control_message( WS_IPPROTO_IP, WS_IP_PKTINFO, ptr, &ctlsize,
                                                    (void *)&data_win, sizeof(data_win) );
                        if (!ptr) goto error;
                        break;
                    }
#endif /* IP_PKTINFO */
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

    control->len = (char *)ptr - (char *)cmsg_win;
    return 1;

error:
    control->len = 0;
    return 0;
#else /* defined(IP_PKTINFO) || defined(IP_RECVDSTADDR) */
    control->len = 0;
    return 1;
#endif /* defined(IP_PKTINFO) || defined(IP_RECVDSTADDR) */
}
#endif /* HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

static NTSTATUS try_recv( int fd, struct async_recv_ioctl *async, ULONG_PTR *size )
{
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    char control_buffer[512];
#endif
    union unix_sockaddr unix_addr;
    struct msghdr hdr;
    NTSTATUS status;
    ssize_t ret;

    memset( &hdr, 0, sizeof(hdr) );
    if (async->addr)
    {
        hdr.msg_name = &unix_addr.addr;
        hdr.msg_namelen = sizeof(unix_addr);
    }
    hdr.msg_iov = async->iov;
    hdr.msg_iovlen = async->count;
#ifndef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    hdr.msg_control = control_buffer;
    hdr.msg_controllen = sizeof(control_buffer);
#endif
    while ((ret = virtual_locked_recvmsg( fd, &hdr, async->unix_flags )) < 0 && errno == EINTR);

    if (ret < 0)
    {
        /* Unix-like systems return EINVAL when attempting to read OOB data from
         * an empty socket buffer; Windows returns WSAEWOULDBLOCK. */
        if ((async->unix_flags & MSG_OOB) && errno == EINVAL)
            errno = EWOULDBLOCK;

        if (errno != EWOULDBLOCK) WARN( "recvmsg: %s\n", strerror( errno ) );
        return sock_errno_to_status( errno );
    }

    status = (hdr.msg_flags & MSG_TRUNC) ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;

#ifdef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS
    if (async->control)
    {
        ERR( "Message control headers cannot be properly supported on this system.\n" );
        async->control->len = 0;
    }
#else
    if (async->control && !convert_control_headers( &hdr, async->control ))
    {
        WARN( "Application passed insufficient room for control headers.\n" );
        *async->ret_flags |= WS_MSG_CTRUNC;
        status = STATUS_BUFFER_OVERFLOW;
    }
#endif

    /* If this socket is connected, Linux doesn't give us msg_name and
     * msg_namelen from recvmsg, but it does set msg_namelen to zero.
     *
     * MSDN says that the address is ignored for connection-oriented sockets, so
     * don't try to translate it.
     */
    if (async->addr && hdr.msg_namelen)
        *async->addr_len = sockaddr_from_unix( &unix_addr, async->addr, *async->addr_len );

    *size = ret;
    return status;
}

static NTSTATUS async_recv_proc( void *user, ULONG_PTR *info, NTSTATUS status )
{
    struct async_recv_ioctl *async = user;
    ULONG_PTR information = 0;
    int fd, needs_close;

    TRACE( "%#x\n", status );

    if (status == STATUS_ALERTED)
    {
        if ((status = server_get_unix_fd( async->io.handle, 0, &fd, &needs_close, NULL, NULL )))
            return status;

        status = try_recv( fd, async, &information );
        TRACE( "got status %#x, %#lx bytes read\n", status, information );

        if (status == STATUS_DEVICE_NOT_READY)
            status = STATUS_PENDING;

        if (needs_close) close( fd );
    }
    if (status != STATUS_PENDING)
    {
        *info = information;
        release_fileio( &async->io );
    }
    return status;
}

static NTSTATUS sock_recv( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io,
                           int fd, const WSABUF *buffers, unsigned int count, WSABUF *control,
                           struct WS_sockaddr *addr, int *addr_len, DWORD *ret_flags, int unix_flags, int force_async )
{
    struct async_recv_ioctl *async;
    ULONG_PTR information;
    HANDLE wait_handle;
    DWORD async_size;
    NTSTATUS status;
    unsigned int i;
    ULONG options;

    if (unix_flags & MSG_OOB)
    {
        int oobinline;
        socklen_t len = sizeof(oobinline);
        if (!getsockopt( fd, SOL_SOCKET, SO_OOBINLINE, (char *)&oobinline, &len ) && oobinline)
            return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < count; ++i)
    {
        if (!virtual_check_buffer_for_write( buffers[i].buf, buffers[i].len ))
            return STATUS_ACCESS_VIOLATION;
    }

    async_size = offsetof( struct async_recv_ioctl, iov[count] );

    if (!(async = (struct async_recv_ioctl *)alloc_fileio( async_size, async_recv_proc, handle )))
        return STATUS_NO_MEMORY;

    async->count = count;
    for (i = 0; i < count; ++i)
    {
        async->iov[i].iov_base = buffers[i].buf;
        async->iov[i].iov_len = buffers[i].len;
    }
    async->unix_flags = unix_flags;
    async->control = control;
    async->addr = addr;
    async->addr_len = addr_len;
    async->ret_flags = ret_flags;

    status = try_recv( fd, async, &information );

    if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW && status != STATUS_DEVICE_NOT_READY)
    {
        release_fileio( &async->io );
        return status;
    }

    if (status == STATUS_DEVICE_NOT_READY && force_async)
        status = STATUS_PENDING;

    SERVER_START_REQ( recv_socket )
    {
        req->status = status;
        req->total  = information;
        req->async  = server_async( handle, &async->io, event, apc, apc_user, io );
        req->oob    = !!(unix_flags & MSG_OOB);
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if ((!NT_ERROR(status) || wait_handle) && status != STATUS_PENDING)
        {
            io->Status = status;
            io->Information = information;
        }
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) release_fileio( &async->io );

    if (wait_handle) status = wait_async( wait_handle, options & FILE_SYNCHRONOUS_IO_ALERT );
    return status;
}


struct async_poll_ioctl
{
    struct async_fileio io;
    unsigned int count;
    struct afd_poll_params *input, *output;
    struct poll_socket_output sockets[1];
};

static ULONG_PTR fill_poll_output( struct async_poll_ioctl *async, NTSTATUS status )
{
    struct afd_poll_params *input = async->input, *output = async->output;
    unsigned int i, count = 0;

    memcpy( output, input, offsetof( struct afd_poll_params, sockets[0] ) );

    if (!status)
    {
        for (i = 0; i < async->count; ++i)
        {
            if (async->sockets[i].flags)
            {
                output->sockets[count].socket = input->sockets[i].socket;
                output->sockets[count].flags = async->sockets[i].flags;
                output->sockets[count].status = async->sockets[i].status;
                ++count;
            }
        }
    }
    output->count = count;
    return offsetof( struct afd_poll_params, sockets[count] );
}

static NTSTATUS async_poll_proc( void *user, ULONG_PTR *info, NTSTATUS status )
{
    struct async_poll_ioctl *async = user;
    ULONG_PTR information = 0;

    if (status == STATUS_ALERTED)
    {
        SERVER_START_REQ( get_async_result )
        {
            req->user_arg = wine_server_client_ptr( async );
            wine_server_set_reply( req, async->sockets, async->count * sizeof(async->sockets[0]) );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;

        information = fill_poll_output( async, status );
    }

    if (status != STATUS_PENDING)
    {
        *info = information;
        free( async->input );
        release_fileio( &async->io );
    }
    return status;
}


/* we could handle this ioctl entirely on the server side, but the differing
 * structure size makes it painful */
static NTSTATUS sock_poll( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io,
                           void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size )
{
    const struct afd_poll_params *params = in_buffer;
    struct poll_socket_input *input;
    struct async_poll_ioctl *async;
    HANDLE wait_handle;
    DWORD async_size;
    NTSTATUS status;
    unsigned int i;
    ULONG options;

    if (in_size < sizeof(*params) || out_size < in_size || !params->count
            || in_size < offsetof( struct afd_poll_params, sockets[params->count] ))
        return STATUS_INVALID_PARAMETER;

    TRACE( "timeout %s, count %u, unknown %#x, padding (%#x, %#x, %#x), sockets[0] {%04lx, %#x}\n",
            wine_dbgstr_longlong(params->timeout), params->count, params->unknown,
            params->padding[0], params->padding[1], params->padding[2],
            params->sockets[0].socket, params->sockets[0].flags );

    if (params->unknown) FIXME( "unknown boolean is %#x\n", params->unknown );
    if (params->padding[0]) FIXME( "padding[0] is %#x\n", params->padding[0] );
    if (params->padding[1]) FIXME( "padding[1] is %#x\n", params->padding[1] );
    if (params->padding[2]) FIXME( "padding[2] is %#x\n", params->padding[2] );
    for (i = 0; i < params->count; ++i)
    {
        if (params->sockets[i].flags & ~0x1ff)
            FIXME( "unknown socket flags %#x\n", params->sockets[i].flags );
    }

    if (!(input = malloc( params->count * sizeof(*input) )))
        return STATUS_NO_MEMORY;

    async_size = offsetof( struct async_poll_ioctl, sockets[params->count] );

    if (!(async = (struct async_poll_ioctl *)alloc_fileio( async_size, async_poll_proc, handle )))
    {
        free( input );
        return STATUS_NO_MEMORY;
    }

    if (!(async->input = malloc( in_size )))
    {
        release_fileio( &async->io );
        free( input );
        return STATUS_NO_MEMORY;
    }
    memcpy( async->input, in_buffer, in_size );

    async->count = params->count;
    async->output = out_buffer;

    for (i = 0; i < params->count; ++i)
    {
        input[i].socket = params->sockets[i].socket;
        input[i].flags = params->sockets[i].flags;
    }

    SERVER_START_REQ( poll_socket )
    {
        req->async = server_async( handle, &async->io, event, apc, apc_user, io );
        req->timeout = params->timeout;
        wine_server_add_data( req, input, params->count * sizeof(*input) );
        wine_server_set_reply( req, async->sockets, params->count * sizeof(async->sockets[0]) );
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options = reply->options;
        if (wait_handle && status != STATUS_PENDING)
        {
            io->Status = status;
            io->Information = fill_poll_output( async, status );
        }
    }
    SERVER_END_REQ;

    free( input );

    if (status != STATUS_PENDING)
    {
        free( async->input );
        release_fileio( &async->io );
    }

    if (wait_handle) status = wait_async( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT) );
    return status;
}

static NTSTATUS try_send( int fd, struct async_send_ioctl *async )
{
    union unix_sockaddr unix_addr;
    struct msghdr hdr;
    ssize_t ret;

    memset( &hdr, 0, sizeof(hdr) );
    if (async->addr)
    {
        hdr.msg_name = &unix_addr;
        hdr.msg_namelen = sockaddr_to_unix( async->addr, async->addr_len, &unix_addr );
        if (!hdr.msg_namelen)
        {
            ERR( "failed to convert address\n" );
            return STATUS_ACCESS_VIOLATION;
        }

#if defined(HAS_IPX) && defined(SOL_IPX)
        if (async->addr->sa_family == WS_AF_IPX)
        {
            int type;
            socklen_t len = sizeof(type);

            /* The packet type is stored at the IPX socket level. At least the
             * linux kernel seems to do something with it in case hdr.msg_name
             * is NULL. Nonetheless we can use it to store the packet type, and
             * then we can retrieve it using getsockopt. After that we can set
             * the IPX type in the sockaddr_ipx structure with the stored value.
             */
            if (getsockopt(fd, SOL_IPX, IPX_TYPE, &type, &len) >= 0)
                unix_addr.ipx.sipx_type = type;
        }
#endif
    }

    hdr.msg_iov = async->iov + async->iov_cursor;
    hdr.msg_iovlen = async->count - async->iov_cursor;

    while ((ret = sendmsg( fd, &hdr, async->unix_flags )) == -1)
    {
        if (errno == EISCONN)
        {
            hdr.msg_name = NULL;
            hdr.msg_namelen = 0;
        }
        else if (errno != EINTR)
        {
            if (errno != EWOULDBLOCK) WARN( "sendmsg: %s\n", strerror( errno ) );
            return sock_errno_to_status( errno );
        }
    }

    async->sent_len += ret;

    while (async->iov_cursor < async->count && ret >= async->iov[async->iov_cursor].iov_len)
        ret -= async->iov[async->iov_cursor++].iov_len;
    if (async->iov_cursor < async->count)
    {
        async->iov[async->iov_cursor].iov_base = (char *)async->iov[async->iov_cursor].iov_base + ret;
        async->iov[async->iov_cursor].iov_len -= ret;
        return STATUS_DEVICE_NOT_READY;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS async_send_proc( void *user, ULONG_PTR *info, NTSTATUS status )
{
    struct async_send_ioctl *async = user;
    int fd, needs_close;

    TRACE( "%#x\n", status );

    if (status == STATUS_ALERTED)
    {
        if ((status = server_get_unix_fd( async->io.handle, 0, &fd, &needs_close, NULL, NULL )))
            return status;

        status = try_send( fd, async );
        TRACE( "got status %#x\n", status );

        if (status == STATUS_DEVICE_NOT_READY)
            status = STATUS_PENDING;

        if (needs_close) close( fd );
    }
    if (status != STATUS_PENDING)
    {
        *info = async->sent_len;
        release_fileio( &async->io );
    }
    return status;
}

static NTSTATUS sock_send( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                           IO_STATUS_BLOCK *io, int fd, const WSABUF *buffers, unsigned int count,
                           const struct WS_sockaddr *addr, unsigned int addr_len, int unix_flags, int force_async )
{
    struct async_send_ioctl *async;
    HANDLE wait_handle;
    DWORD async_size;
    NTSTATUS status;
    unsigned int i;
    ULONG options;

    async_size = offsetof( struct async_send_ioctl, iov[count] );

    if (!(async = (struct async_send_ioctl *)alloc_fileio( async_size, async_send_proc, handle )))
        return STATUS_NO_MEMORY;

    async->count = count;
    for (i = 0; i < count; ++i)
    {
        async->iov[i].iov_base = buffers[i].buf;
        async->iov[i].iov_len = buffers[i].len;
    }
    async->unix_flags = unix_flags;
    async->addr = addr;
    async->addr_len = addr_len;
    async->iov_cursor = 0;
    async->sent_len = 0;

    status = try_send( fd, async );

    if (status != STATUS_SUCCESS && status != STATUS_DEVICE_NOT_READY)
    {
        release_fileio( &async->io );
        return status;
    }

    if (status == STATUS_DEVICE_NOT_READY && force_async)
        status = STATUS_PENDING;

    SERVER_START_REQ( send_socket )
    {
        req->status = status;
        req->total  = async->sent_len;
        req->async  = server_async( handle, &async->io, event, apc, apc_user, io );
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if ((!NT_ERROR(status) || wait_handle) && status != STATUS_PENDING)
        {
            io->Status = status;
            io->Information = async->sent_len;
        }
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) release_fileio( &async->io );

    if (wait_handle) status = wait_async( wait_handle, options & FILE_SYNCHRONOUS_IO_ALERT );
    return status;
}

static ssize_t do_send( int fd, const void *buffer, size_t len, int flags )
{
    ssize_t ret;
    while ((ret = send( fd, buffer, len, flags )) < 0 && errno == EINTR);
    if (ret < 0 && errno != EWOULDBLOCK) WARN( "send: %s\n", strerror( errno ) );
    return ret;
}

static NTSTATUS try_transmit( int sock_fd, int file_fd, struct async_transmit_ioctl *async )
{
    ssize_t ret;

    while (async->head_cursor < async->buffers.HeadLength)
    {
        TRACE( "sending %u bytes of header data\n", async->buffers.HeadLength - async->head_cursor );
        ret = do_send( sock_fd, (char *)async->buffers.Head + async->head_cursor,
                       async->buffers.HeadLength - async->head_cursor, 0 );
        if (ret < 0) return sock_errno_to_status( errno );
        TRACE( "send returned %zd\n", ret );
        async->head_cursor += ret;
    }

    while (async->buffer_cursor < async->read_len)
    {
        TRACE( "sending %u bytes of file data\n", async->read_len - async->buffer_cursor );
        ret = do_send( sock_fd, async->buffer + async->buffer_cursor,
                       async->read_len - async->buffer_cursor, 0 );
        if (ret < 0) return sock_errno_to_status( errno );
        TRACE( "send returned %zd\n", ret );
        async->buffer_cursor += ret;
        async->file_cursor += ret;
    }

    if (async->file && async->buffer_cursor == async->read_len)
    {
        unsigned int read_size = async->buffer_size;

        if (async->file_len)
            read_size = min( read_size, async->file_len - async->file_cursor );

        TRACE( "reading %u bytes of file data\n", read_size );
        do
        {
            if (async->offset.QuadPart == FILE_USE_FILE_POINTER_POSITION)
                ret = read( file_fd, async->buffer, read_size );
            else
                ret = pread( file_fd, async->buffer, read_size, async->offset.QuadPart );
        } while (ret < 0 && errno == EINTR);
        if (ret < 0) return errno_to_status( errno );
        TRACE( "read returned %zd\n", ret );

        async->read_len = ret;
        async->buffer_cursor = 0;
        if (async->offset.QuadPart != FILE_USE_FILE_POINTER_POSITION)
            async->offset.QuadPart += ret;

        if (ret < read_size || (async->file_len && async->file_cursor == async->file_len))
            async->file = NULL;
        return STATUS_PENDING; /* still more data to send */
    }

    while (async->tail_cursor < async->buffers.TailLength)
    {
        TRACE( "sending %u bytes of tail data\n", async->buffers.TailLength - async->tail_cursor );
        ret = do_send( sock_fd, (char *)async->buffers.Tail + async->tail_cursor,
                       async->buffers.TailLength - async->tail_cursor, 0 );
        if (ret < 0) return sock_errno_to_status( errno );
        TRACE( "send returned %zd\n", ret );
        async->tail_cursor += ret;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS async_transmit_proc( void *user, ULONG_PTR *info, NTSTATUS status )
{
    int sock_fd, file_fd = -1, sock_needs_close = FALSE, file_needs_close = FALSE;
    struct async_transmit_ioctl *async = user;

    TRACE( "%#x\n", status );

    if (status == STATUS_ALERTED)
    {
        if ((status = server_get_unix_fd( async->io.handle, 0, &sock_fd, &sock_needs_close, NULL, NULL )))
            return status;

        if (async->file && (status = server_get_unix_fd( async->file, 0, &file_fd, &file_needs_close, NULL, NULL )))
        {
            if (sock_needs_close) close( sock_fd );
            return status;
        }

        status = try_transmit( sock_fd, file_fd, async );
        TRACE( "got status %#x\n", status );

        if (status == STATUS_DEVICE_NOT_READY)
            status = STATUS_PENDING;

        if (sock_needs_close) close( sock_fd );
        if (file_needs_close) close( file_fd );
    }
    if (status != STATUS_PENDING)
    {
        *info = async->head_cursor + async->file_cursor + async->tail_cursor;
        release_fileio( &async->io );
    }
    return status;
}

static NTSTATUS sock_transmit( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                               IO_STATUS_BLOCK *io, int fd, const struct afd_transmit_params *params )
{
    int file_fd, file_needs_close = FALSE;
    struct async_transmit_ioctl *async;
    enum server_fd_type file_type;
    union unix_sockaddr addr;
    socklen_t addr_len;
    HANDLE wait_handle;
    NTSTATUS status;
    ULONG options;

    addr_len = sizeof(addr);
    if (getpeername( fd, &addr.addr, &addr_len ) != 0)
        return STATUS_INVALID_CONNECTION;

    if (params->file)
    {
        if ((status = server_get_unix_fd( params->file, 0, &file_fd, &file_needs_close, &file_type, NULL )))
            return status;
        if (file_needs_close) close( file_fd );

        if (file_type != FD_TYPE_FILE)
        {
            FIXME( "unsupported file type %#x\n", file_type );
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    if (!(async = (struct async_transmit_ioctl *)alloc_fileio( sizeof(*async), async_transmit_proc, handle )))
        return STATUS_NO_MEMORY;

    async->file = params->file;
    async->buffer_size = params->buffer_size ? params->buffer_size : 65536;
    if (!(async->buffer = malloc( async->buffer_size )))
    {
        release_fileio( &async->io );
        return STATUS_NO_MEMORY;
    }
    async->read_len = 0;
    async->head_cursor = 0;
    async->file_cursor = 0;
    async->buffer_cursor = 0;
    async->tail_cursor = 0;
    async->file_len = params->file_len;
    async->flags = params->flags;
    async->buffers = params->buffers;
    async->offset = params->offset;

    SERVER_START_REQ( send_socket )
    {
        req->status = STATUS_PENDING;
        req->total  = 0;
        req->async  = server_async( handle, &async->io, event, apc, apc_user, io );
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        /* In theory we'd fill the iosb here, as above in sock_send(), but it's
         * actually currently impossible to get STATUS_SUCCESS. The server will
         * either return STATUS_PENDING or an error code, and in neither case
         * should the iosb be filled. */
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) release_fileio( &async->io );

    if (wait_handle) status = wait_async( wait_handle, options & FILE_SYNCHRONOUS_IO_ALERT );
    return status;
}

static void complete_async( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                            IO_STATUS_BLOCK *io, NTSTATUS status, ULONG_PTR information )
{
    io->Status = status;
    io->Information = information;
    if (event) NtSetEvent( event, NULL );
    if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc, (ULONG_PTR)apc_user, (ULONG_PTR)io, 0 );
    if (apc_user) add_completion( handle, (ULONG_PTR)apc_user, status, information, FALSE );
}


static NTSTATUS do_getsockopt( HANDLE handle, IO_STATUS_BLOCK *io, int level,
                               int option, void *out_buffer, ULONG out_size )
{
    int fd, needs_close = FALSE;
    socklen_t len = out_size;
    NTSTATUS status;
    int ret;

    if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
        return status;

    ret = getsockopt( fd, level, option, out_buffer, &len );
    if (needs_close) close( fd );
    if (ret) return sock_errno_to_status( errno );
    io->Information = len;
    return STATUS_SUCCESS;
}


static NTSTATUS do_setsockopt( HANDLE handle, IO_STATUS_BLOCK *io, int level,
                               int option, const void *optval, socklen_t optlen )
{
    int fd, needs_close = FALSE;
    NTSTATUS status;
    int ret;

    if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
        return status;

    ret = setsockopt( fd, level, option, optval, optlen );
    if (needs_close) close( fd );
    return ret ? sock_errno_to_status( errno ) : STATUS_SUCCESS;
}


NTSTATUS sock_ioctl( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io,
                     ULONG code, void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size )
{
    int fd, needs_close = FALSE;
    NTSTATUS status;

    TRACE( "handle %p, code %#x, in_buffer %p, in_size %u, out_buffer %p, out_size %u\n",
           handle, code, in_buffer, in_size, out_buffer, out_size );

    switch (code)
    {
        case IOCTL_AFD_BIND:
        {
            const struct afd_bind_params *params = in_buffer;

            if (params->unknown) FIXME( "bind: got unknown %#x\n", params->unknown );

            status = STATUS_BAD_DEVICE_TYPE;
            break;
        }

        case IOCTL_AFD_GETSOCKNAME:
            if (in_size) FIXME( "unexpected input size %u\n", in_size );

            status = STATUS_BAD_DEVICE_TYPE;
            break;

        case IOCTL_AFD_LISTEN:
        {
            const struct afd_listen_params *params = in_buffer;

            TRACE( "backlog %u\n", params->backlog );
            if (out_size) FIXME( "unexpected output size %u\n", out_size );
            if (params->unknown1) FIXME( "listen: got unknown1 %#x\n", params->unknown1 );
            if (params->unknown2) FIXME( "listen: got unknown2 %#x\n", params->unknown2 );

            status = STATUS_BAD_DEVICE_TYPE;
            break;
        }

        case IOCTL_AFD_EVENT_SELECT:
        {
            const struct afd_event_select_params *params = in_buffer;

            TRACE( "event %p, mask %#x\n", params->event, params->mask );
            if (out_size) FIXME( "unexpected output size %u\n", out_size );

            status = STATUS_BAD_DEVICE_TYPE;
            break;
        }

        case IOCTL_AFD_GET_EVENTS:
            if (in_size) FIXME( "unexpected input size %u\n", in_size );

            status = STATUS_BAD_DEVICE_TYPE;
            break;

        case IOCTL_AFD_RECV:
        {
            const struct afd_recv_params *params = in_buffer;
            int unix_flags = 0;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (out_size) FIXME( "unexpected output size %u\n", out_size );

            if (in_size < sizeof(struct afd_recv_params))
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if ((params->msg_flags & (AFD_MSG_NOT_OOB | AFD_MSG_OOB)) == 0 ||
                (params->msg_flags & (AFD_MSG_NOT_OOB | AFD_MSG_OOB)) == (AFD_MSG_NOT_OOB | AFD_MSG_OOB))
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (params->msg_flags & ~(AFD_MSG_NOT_OOB | AFD_MSG_OOB | AFD_MSG_PEEK | AFD_MSG_WAITALL))
                FIXME( "unknown msg_flags %#x\n", params->msg_flags );
            if (params->recv_flags & ~AFD_RECV_FORCE_ASYNC)
                FIXME( "unknown recv_flags %#x\n", params->recv_flags );

            if (params->msg_flags & AFD_MSG_OOB)
                unix_flags |= MSG_OOB;
            if (params->msg_flags & AFD_MSG_PEEK)
                unix_flags |= MSG_PEEK;
            if (params->msg_flags & AFD_MSG_WAITALL)
                FIXME( "MSG_WAITALL is not supported\n" );

            status = sock_recv( handle, event, apc, apc_user, io, fd, params->buffers, params->count, NULL,
                                NULL, NULL, NULL, unix_flags, !!(params->recv_flags & AFD_RECV_FORCE_ASYNC) );
            break;
        }

        case IOCTL_AFD_WINE_RECVMSG:
        {
            struct afd_recvmsg_params *params = in_buffer;
            int unix_flags = 0;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (in_size < sizeof(*params))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (*params->ws_flags & WS_MSG_OOB)
                unix_flags |= MSG_OOB;
            if (*params->ws_flags & WS_MSG_PEEK)
                unix_flags |= MSG_PEEK;
            if (*params->ws_flags & WS_MSG_WAITALL)
                FIXME( "MSG_WAITALL is not supported\n" );

            status = sock_recv( handle, event, apc, apc_user, io, fd, params->buffers, params->count, params->control,
                                params->addr, params->addr_len, params->ws_flags, unix_flags, params->force_async );
            break;
        }

        case IOCTL_AFD_WINE_SENDMSG:
        {
            const struct afd_sendmsg_params *params = in_buffer;
            int unix_flags = 0;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (in_size < sizeof(*params))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (params->ws_flags & WS_MSG_OOB)
                unix_flags |= MSG_OOB;
            if (params->ws_flags & WS_MSG_PARTIAL)
                WARN( "ignoring MSG_PARTIAL\n" );
            if (params->ws_flags & ~(WS_MSG_OOB | WS_MSG_PARTIAL))
                FIXME( "unknown flags %#x\n", params->ws_flags );

            status = sock_send( handle, event, apc, apc_user, io, fd, params->buffers, params->count,
                                params->addr, params->addr_len, unix_flags, params->force_async );
            break;
        }

        case IOCTL_AFD_WINE_TRANSMIT:
        {
            const struct afd_transmit_params *params = in_buffer;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (in_size < sizeof(*params))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = sock_transmit( handle, event, apc, apc_user, io, fd, params );
            break;
        }

        case IOCTL_AFD_WINE_COMPLETE_ASYNC:
        {
            if (in_size != sizeof(NTSTATUS))
                return STATUS_BUFFER_TOO_SMALL;

            status = *(NTSTATUS *)in_buffer;
            complete_async( handle, event, apc, apc_user, io, status, 0 );
            break;
        }

        case IOCTL_AFD_POLL:
            status = sock_poll( handle, event, apc, apc_user, io, in_buffer, in_size, out_buffer, out_size );
            break;

        case IOCTL_AFD_WINE_FIONREAD:
        {
            int value, ret;

            if (out_size < sizeof(int))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

#ifdef linux
            {
                socklen_t len = sizeof(value);

                /* FIONREAD on a listening socket always fails (see tcp(7)). */
                if (!getsockopt( fd, SOL_SOCKET, SO_ACCEPTCONN, &value, &len ) && value)
                {
                    *(int *)out_buffer = 0;
                    status = STATUS_SUCCESS;
                    complete_async( handle, event, apc, apc_user, io, status, 0 );
                    break;
                }
            }
#endif

            if ((ret = ioctl( fd, FIONREAD, &value )) < 0)
            {
                status = sock_errno_to_status( errno );
                break;
            }
            *(int *)out_buffer = value;
            status = STATUS_SUCCESS;
            complete_async( handle, event, apc, apc_user, io, status, 0 );
            break;
        }

        case IOCTL_AFD_WINE_SIOCATMARK:
        {
            int value, ret;
            socklen_t len = sizeof(value);

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (out_size < sizeof(int))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (getsockopt( fd, SOL_SOCKET, SO_OOBINLINE, &value, &len ) < 0)
            {
                status = sock_errno_to_status( errno );
                break;
            }

            if (value)
            {
                *(int *)out_buffer = TRUE;
            }
            else
            {
                if ((ret = ioctl( fd, SIOCATMARK, &value )) < 0)
                {
                    status = sock_errno_to_status( errno );
                    break;
                }
                /* windows is reversed with respect to unix */
                *(int *)out_buffer = !value;
            }
            status = STATUS_SUCCESS;
            complete_async( handle, event, apc, apc_user, io, status, 0 );
            break;
        }

        case IOCTL_AFD_WINE_GET_INTERFACE_LIST:
        {
#ifdef HAVE_GETIFADDRS
            INTERFACE_INFO *info = out_buffer;
            struct ifaddrs *ifaddrs, *ifaddr;
            unsigned int count = 0;
            ULONG ret_size;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (getifaddrs( &ifaddrs ) < 0)
            {
                status = sock_errno_to_status( errno );
                break;
            }

            for (ifaddr = ifaddrs; ifaddr != NULL; ifaddr = ifaddr->ifa_next)
            {
                if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET) ++count;
            }

            ret_size = count * sizeof(*info);
            if (out_size < ret_size)
            {
                status = STATUS_PENDING;
                complete_async( handle, event, apc, apc_user, io, STATUS_BUFFER_TOO_SMALL, 0 );
                freeifaddrs( ifaddrs );
                break;
            }

            memset( out_buffer, 0, ret_size );

            count = 0;
            for (ifaddr = ifaddrs; ifaddr != NULL; ifaddr = ifaddr->ifa_next)
            {
                in_addr_t addr, mask;

                if (!ifaddr->ifa_addr || ifaddr->ifa_addr->sa_family != AF_INET)
                    continue;

                addr = ((const struct sockaddr_in *)ifaddr->ifa_addr)->sin_addr.s_addr;
                mask = ((const struct sockaddr_in *)ifaddr->ifa_netmask)->sin_addr.s_addr;

                info[count].iiFlags = 0;
                if (ifaddr->ifa_flags & IFF_BROADCAST)
                    info[count].iiFlags |= WS_IFF_BROADCAST;
                if (ifaddr->ifa_flags & IFF_LOOPBACK)
                    info[count].iiFlags |= WS_IFF_LOOPBACK;
                if (ifaddr->ifa_flags & IFF_MULTICAST)
                    info[count].iiFlags |= WS_IFF_MULTICAST;
#ifdef IFF_POINTTOPOINT
                if (ifaddr->ifa_flags & IFF_POINTTOPOINT)
                    info[count].iiFlags |= WS_IFF_POINTTOPOINT;
#endif
                if (ifaddr->ifa_flags & IFF_UP)
                    info[count].iiFlags |= WS_IFF_UP;

                info[count].iiAddress.AddressIn.sin_family = WS_AF_INET;
                info[count].iiAddress.AddressIn.sin_port = 0;
                info[count].iiAddress.AddressIn.sin_addr.WS_s_addr = addr;

                info[count].iiNetmask.AddressIn.sin_family = WS_AF_INET;
                info[count].iiNetmask.AddressIn.sin_port = 0;
                info[count].iiNetmask.AddressIn.sin_addr.WS_s_addr = mask;

                if (ifaddr->ifa_flags & IFF_BROADCAST)
                {
                    info[count].iiBroadcastAddress.AddressIn.sin_family = WS_AF_INET;
                    info[count].iiBroadcastAddress.AddressIn.sin_port = 0;
                    info[count].iiBroadcastAddress.AddressIn.sin_addr.WS_s_addr = addr | ~mask;
                }

                ++count;
            }

            freeifaddrs( ifaddrs );
            status = STATUS_PENDING;
            complete_async( handle, event, apc, apc_user, io, STATUS_SUCCESS, ret_size );
#else
            FIXME( "Interface list queries are currently not supported on this platform.\n" );
            status = STATUS_NOT_SUPPORTED;
#endif
            break;
        }

        case IOCTL_AFD_WINE_KEEPALIVE_VALS:
        {
            struct tcp_keepalive *k = in_buffer;
            int keepalive;

            if (!in_buffer || in_size < sizeof(struct tcp_keepalive))
                return STATUS_BUFFER_TOO_SMALL;
            keepalive = !!k->onoff;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int) ) < 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (keepalive)
            {
#ifdef TCP_KEEPIDLE
                int idle = max( 1, (k->keepalivetime + 500) / 1000 );

                if (setsockopt( fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int) ) < 0)
                {
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
#else
                FIXME("ignoring keepalive timeout\n");
#endif
            }

            if (keepalive)
            {
#ifdef TCP_KEEPINTVL
                int interval = max( 1, (k->keepaliveinterval + 500) / 1000 );

                if (setsockopt( fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int) ) < 0)
                    status = STATUS_INVALID_PARAMETER;
#else
                FIXME("ignoring keepalive interval\n");
#endif
            }

            status = STATUS_SUCCESS;
            complete_async( handle, event, apc, apc_user, io, status, 0 );
            break;
        }

        case IOCTL_AFD_WINE_GETPEERNAME:
        {
            union unix_sockaddr unix_addr;
            socklen_t unix_len = sizeof(unix_addr);
            int len;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (getpeername( fd, &unix_addr.addr, &unix_len ) < 0)
            {
                status = sock_errno_to_status( errno );
                break;
            }

            len = sockaddr_from_unix( &unix_addr, out_buffer, out_size );
            if (out_size < len)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            io->Information = len;
            status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_AFD_WINE_GET_SO_BROADCAST:
            return do_getsockopt( handle, io, SOL_SOCKET, SO_BROADCAST, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_SO_BROADCAST:
            return do_setsockopt( handle, io, SOL_SOCKET, SO_BROADCAST, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_SO_KEEPALIVE:
            return do_getsockopt( handle, io, SOL_SOCKET, SO_KEEPALIVE, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_SO_KEEPALIVE:
            return do_setsockopt( handle, io, SOL_SOCKET, SO_KEEPALIVE, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_SO_LINGER:
        {
            struct WS_linger *ws_linger = out_buffer;
            struct linger unix_linger;
            socklen_t len = sizeof(unix_linger);
            int ret;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            ret = getsockopt( fd, SOL_SOCKET, SO_LINGER, &unix_linger, &len );
            if (needs_close) close( fd );
            if (!ret)
            {
                ws_linger->l_onoff = unix_linger.l_onoff;
                ws_linger->l_linger = unix_linger.l_linger;
                io->Information = sizeof(*ws_linger);
            }

            return ret ? sock_errno_to_status( errno ) : STATUS_SUCCESS;
        }

        case IOCTL_AFD_WINE_SET_SO_LINGER:
        {
            const struct WS_linger *ws_linger = in_buffer;
            struct linger unix_linger;

            unix_linger.l_onoff = ws_linger->l_onoff;
            unix_linger.l_linger = ws_linger->l_linger;

            return do_setsockopt( handle, io, SOL_SOCKET, SO_LINGER, &unix_linger, sizeof(unix_linger) );
        }

        case IOCTL_AFD_WINE_GET_SO_OOBINLINE:
            return do_getsockopt( handle, io, SOL_SOCKET, SO_OOBINLINE, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_SO_OOBINLINE:
            return do_setsockopt( handle, io, SOL_SOCKET, SO_OOBINLINE, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_SO_REUSEADDR:
            return do_getsockopt( handle, io, SOL_SOCKET, SO_REUSEADDR, out_buffer, out_size );

        /* BSD socket SO_REUSEADDR is not 100% compatible to winsock semantics;
         * however, using it the BSD way fixes bug 8513 and seems to be what
         * most programmers assume, anyway */
        case IOCTL_AFD_WINE_SET_SO_REUSEADDR:
        {
            int fd, needs_close = FALSE;
            NTSTATUS status;
            int ret;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            ret = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, in_buffer, in_size );
#ifdef __APPLE__
            if (!ret) ret = setsockopt( fd, SOL_SOCKET, SO_REUSEPORT, in_buffer, in_size );
#endif
            if (needs_close) close( fd );
            return ret ? sock_errno_to_status( errno ) : STATUS_SUCCESS;
        }

        case IOCTL_AFD_WINE_SET_IP_ADD_MEMBERSHIP:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_ADD_MEMBERSHIP, in_buffer, in_size );

        case IOCTL_AFD_WINE_SET_IP_ADD_SOURCE_MEMBERSHIP:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, in_buffer, in_size );

        case IOCTL_AFD_WINE_SET_IP_BLOCK_SOURCE:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_BLOCK_SOURCE, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IP_DONTFRAGMENT:
        {
            socklen_t len = out_size;
            int ret;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

#ifdef IP_DONTFRAG
            ret = getsockopt( fd, IPPROTO_IP, IP_DONTFRAG, out_buffer, &len );
#elif defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_DONT)
            {
                int value;

                len = sizeof(value);
                ret = getsockopt( fd, IPPROTO_IP, IP_MTU_DISCOVER, &value, &len );
                if (!ret) *(DWORD *)out_buffer = (value != IP_PMTUDISC_DONT);
            }
#else
            {
                static int once;

                if (!once++)
                    FIXME( "IP_DONTFRAGMENT is not supported on this platform\n" );
                ret = 0; /* fake success */
            }
#endif
            if (needs_close) close( fd );
            if (ret) return sock_errno_to_status( errno );
            io->Information = len;
            return STATUS_SUCCESS;
        }

        case IOCTL_AFD_WINE_SET_IP_DONTFRAGMENT:
#ifdef IP_DONTFRAG
            return do_setsockopt( handle, io, IPPROTO_IP, IP_DONTFRAG, in_buffer, in_size );
#elif defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_DO) && defined(IP_PMTUDISC_DONT)
        {
            int value = *(DWORD *)in_buffer ? IP_PMTUDISC_DO : IP_PMTUDISC_DONT;

            return do_setsockopt( handle, io, IPPROTO_IP, IP_MTU_DISCOVER, &value, sizeof(value) );
        }
#else
        {
            static int once;

            if (!once++)
                FIXME( "IP_DONTFRAGMENT is not supported on this platform\n" );
            return STATUS_SUCCESS; /* fake success */
        }
#endif

        case IOCTL_AFD_WINE_SET_IP_DROP_MEMBERSHIP:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_DROP_MEMBERSHIP, in_buffer, in_size );

        case IOCTL_AFD_WINE_SET_IP_DROP_SOURCE_MEMBERSHIP:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, in_buffer, in_size );

#ifdef IP_HDRINCL
        case IOCTL_AFD_WINE_GET_IP_HDRINCL:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_HDRINCL, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_HDRINCL:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_HDRINCL, in_buffer, in_size );
#endif

        case IOCTL_AFD_WINE_GET_IP_MULTICAST_IF:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_MULTICAST_IF, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_MULTICAST_IF:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_MULTICAST_IF, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IP_MULTICAST_LOOP:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_MULTICAST_LOOP, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_MULTICAST_LOOP:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_MULTICAST_LOOP, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IP_MULTICAST_TTL:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_MULTICAST_TTL, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_MULTICAST_TTL:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_MULTICAST_TTL, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IP_OPTIONS:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_OPTIONS, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_OPTIONS:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_OPTIONS, in_buffer, in_size );

#ifdef IP_PKTINFO
        case IOCTL_AFD_WINE_GET_IP_PKTINFO:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_PKTINFO, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_PKTINFO:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_PKTINFO, in_buffer, in_size );
#elif defined(IP_RECVDSTADDR)
        case IOCTL_AFD_WINE_GET_IP_PKTINFO:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_RECVDSTADDR, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_PKTINFO:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_RECVDSTADDR, in_buffer, in_size );
#endif

        case IOCTL_AFD_WINE_GET_IP_TOS:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_TOS, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_TOS:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_TOS, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IP_TTL:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_TTL, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_TTL:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_TTL, in_buffer, in_size );

        case IOCTL_AFD_WINE_SET_IP_UNBLOCK_SOURCE:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_UNBLOCK_SOURCE, in_buffer, in_size );

#ifdef IP_UNICAST_IF
        case IOCTL_AFD_WINE_GET_IP_UNICAST_IF:
            return do_getsockopt( handle, io, IPPROTO_IP, IP_UNICAST_IF, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IP_UNICAST_IF:
            return do_setsockopt( handle, io, IPPROTO_IP, IP_UNICAST_IF, in_buffer, in_size );
#endif

#ifdef IPV6_ADD_MEMBERSHIP
        case IOCTL_AFD_WINE_SET_IPV6_ADD_MEMBERSHIP:
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, in_buffer, in_size );
#endif

        case IOCTL_AFD_WINE_GET_IPV6_DONTFRAG:
        {
            socklen_t len = out_size;
            int ret;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

#ifdef IPV6_DONTFRAG
            ret = getsockopt( fd, IPPROTO_IPV6, IPV6_DONTFRAG, out_buffer, &len );
#elif defined(IPV6_MTU_DISCOVER) && defined(IPV6_PMTUDISC_DONT)
            {
                int value;

                len = sizeof(value);
                ret = getsockopt( fd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &value, &len );
                if (!ret) *(DWORD *)out_buffer = (value != IPV6_PMTUDISC_DONT);
            }
#else
            {
                static int once;

                if (!once++)
                    FIXME( "IPV6_DONTFRAGMENT is not supported on this platform\n" );
                ret = 0; /* fake success */
            }
#endif
            if (needs_close) close( fd );
            if (ret) return sock_errno_to_status( errno );
            io->Information = len;
            return STATUS_SUCCESS;
        }

        case IOCTL_AFD_WINE_SET_IPV6_DONTFRAG:
#ifdef IPV6_DONTFRAG
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_DONTFRAG, in_buffer, in_size );
#elif defined(IPV6_MTU_DISCOVER) && defined(IPV6_PMTUDISC_DO) && defined(IPV6_PMTUDISC_DONT)
        {
            int value = *(DWORD *)in_buffer ? IPV6_PMTUDISC_DO : IPV6_PMTUDISC_DONT;

            return do_setsockopt( handle, io, IPPROTO_IP, IPV6_MTU_DISCOVER, &value, sizeof(value) );
        }
#else
        {
            static int once;

            if (!once++)
                FIXME( "IPV6_DONTFRAGMENT is not supported on this platform\n" );
            return STATUS_SUCCESS; /* fake success */
        }
#endif

#ifdef IPV6_DROP_MEMBERSHIP
        case IOCTL_AFD_WINE_SET_IPV6_DROP_MEMBERSHIP:
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, in_buffer, in_size );
#endif

        case IOCTL_AFD_WINE_GET_IPV6_MULTICAST_HOPS:
            return do_getsockopt( handle, io, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IPV6_MULTICAST_HOPS:
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IPV6_MULTICAST_IF:
            return do_getsockopt( handle, io, IPPROTO_IPV6, IPV6_MULTICAST_IF, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IPV6_MULTICAST_IF:
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_MULTICAST_IF, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IPV6_MULTICAST_LOOP:
            return do_getsockopt( handle, io, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IPV6_MULTICAST_LOOP:
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, in_buffer, in_size );

        case IOCTL_AFD_WINE_GET_IPV6_UNICAST_HOPS:
            return do_getsockopt( handle, io, IPPROTO_IPV6, IPV6_UNICAST_HOPS, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IPV6_UNICAST_HOPS:
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_UNICAST_HOPS, in_buffer, in_size );

#ifdef IPV6_UNICAST_IF
        case IOCTL_AFD_WINE_GET_IPV6_UNICAST_IF:
            return do_getsockopt( handle, io, IPPROTO_IPV6, IPV6_UNICAST_IF, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IPV6_UNICAST_IF:
            return do_setsockopt( handle, io, IPPROTO_IPV6, IPV6_UNICAST_IF, in_buffer, in_size );
#endif

        case IOCTL_AFD_WINE_GET_IPV6_V6ONLY:
            return do_getsockopt( handle, io, IPPROTO_IPV6, IPV6_V6ONLY, out_buffer, out_size );

        case IOCTL_AFD_WINE_SET_IPV6_V6ONLY:
        {
            int fd, needs_close = FALSE;
            union unix_sockaddr addr;
            socklen_t len = sizeof(addr);
            NTSTATUS status;
            int ret;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return status;

            if (!getsockname( fd, &addr.addr, &len ) && addr.addr.sa_family == AF_INET && !addr.in.sin_port)
            {
                /* changing IPV6_V6ONLY succeeds on an unbound IPv4 socket */
                WARN( "ignoring IPV6_V6ONLY on an unbound IPv4 socket\n" );
                return STATUS_SUCCESS;
            }

            ret = setsockopt( fd, IPPROTO_IPV6, IPV6_V6ONLY, in_buffer, in_size );
            if (needs_close) close( fd );
            return ret ? sock_errno_to_status( errno ) : STATUS_SUCCESS;
        }

        default:
        {
            if ((code >> 16) == FILE_DEVICE_NETWORK)
            {
                /* Wine-internal ioctl */
                status = STATUS_BAD_DEVICE_TYPE;
            }
            else
            {
                FIXME( "Unknown ioctl %#x (device %#x, access %#x, function %#x, method %#x)\n",
                       code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3 );
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        }
    }

    if (needs_close) close( fd );
    return status;
}
