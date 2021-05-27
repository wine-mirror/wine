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
#include <unistd.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
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
#include "ws2tcpip.h"
#include "wsipx.h"
#include "af_irda.h"
#include "wine/afd.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

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

        if (wsaddrlen < sizeof(struct WS_sockaddr_in6_old)) return -1;
        win.sin6_family = WS_AF_INET6;
        win.sin6_port = uaddr->in6.sin6_port;
        win.sin6_flowinfo = uaddr->in6.sin6_flowinfo;
        memcpy( &win.sin6_addr, &uaddr->in6.sin6_addr, sizeof(win.sin6_addr) );
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
        win.sin6_scope_id = uaddr->in6.sin6_scope_id;
#endif
        if (wsaddrlen >= sizeof(struct WS_sockaddr_in6))
        {
            memcpy( wsaddr, &win, sizeof(struct WS_sockaddr_in6) );
            return sizeof(struct WS_sockaddr_in6);
        }
        memcpy( wsaddr, &win, sizeof(struct WS_sockaddr_in6_old) );
        return sizeof(struct WS_sockaddr_in6_old);
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

static NTSTATUS async_recv_proc( void *user, IO_STATUS_BLOCK *io, NTSTATUS status )
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
        io->Status = status;
        io->Information = information;
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

    if (!NT_ERROR(status))
    {
        io->Status = status;
        io->Information = information;
    }

    SERVER_START_REQ( recv_socket )
    {
        req->status = status;
        req->total  = information;
        req->async  = server_async( handle, &async->io, event, apc, apc_user, io );
        req->oob    = !!(unix_flags & MSG_OOB);
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
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

static NTSTATUS async_poll_proc( void *user, IO_STATUS_BLOCK *io, NTSTATUS status )
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
        io->Status = status;
        io->Information = information;
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


NTSTATUS sock_ioctl( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io,
                     ULONG code, void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size )
{
    int fd, needs_close = FALSE;
    NTSTATUS status;

    TRACE( "handle %p, code %#x, in_buffer %p, in_size %u, out_buffer %p, out_size %u\n",
           handle, code, in_buffer, in_size, out_buffer, out_size );

    switch (code)
    {
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

        case IOCTL_AFD_POLL:
            status = sock_poll( handle, event, apc, apc_user, io, in_buffer, in_size, out_buffer, out_size );
            break;

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
