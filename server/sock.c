/*
 * Server-side socket management
 *
 * Copyright (C) 1999 Marcus Meissner, Ove Kåven
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
 * FIXME: we use read|write access in all cases. Shouldn't we depend that
 * on the access of the current handle?
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_IFADDRS_H
# include <ifaddrs.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#include <time.h>
#include <unistd.h>
#include <limits.h>
#ifdef HAVE_LINUX_FILTER_H
# include <linux/filter.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
# include <linux/rtnetlink.h>
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
#include "winternl.h"
#include "winerror.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ws2tcpip.h"
#include "wsipx.h"
#include "af_irda.h"
#include "wine/afd.h"

#include "process.h"
#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "user.h"

#if defined(linux) && !defined(IP_UNICAST_IF)
#define IP_UNICAST_IF 50
#endif

static const char magic_loopback_addr[] = {127, 12, 34, 56};

union win_sockaddr
{
    struct WS_sockaddr addr;
    struct WS_sockaddr_in in;
    struct WS_sockaddr_in6 in6;
    struct WS_sockaddr_ipx ipx;
    SOCKADDR_IRDA irda;
};

static struct list poll_list = LIST_INIT( poll_list );

struct poll_req
{
    struct list entry;
    struct async *async;
    struct iosb *iosb;
    struct timeout_user *timeout;
    timeout_t orig_timeout;
    int exclusive;
    unsigned int count;
    struct
    {
        struct sock *sock;
        int mask;
        obj_handle_t handle;
        int flags;
        unsigned int status;
    } sockets[1];
};

struct accept_req
{
    struct list entry;
    struct async *async;
    struct iosb *iosb;
    struct sock *sock, *acceptsock;
    int accepted;
    unsigned int recv_len, local_len;
};

struct connect_req
{
    struct async *async;
    struct iosb *iosb;
    struct sock *sock;
    unsigned int addr_len, send_len, send_cursor;
};

enum connection_state
{
    SOCK_LISTENING,
    SOCK_UNCONNECTED,
    SOCK_CONNECTING,
    SOCK_CONNECTED,
    SOCK_CONNECTIONLESS,
};

struct sock
{
    struct object       obj;         /* object header */
    struct fd          *fd;          /* socket file descriptor */
    enum connection_state state;     /* connection state */
    unsigned int        mask;        /* event mask */
    /* pending AFD_POLL_* events which have not yet been reported to the application */
    unsigned int        pending_events;
    /* AFD_POLL_* events which have already been reported and should not be
     * selected for again until reset by a relevant call.
     *
     * For example, if AFD_POLL_READ is set here and not in pending_events, it
     * has already been reported and consumed, and we should not report it
     * again, even if POLLIN is signaled, until it is reset by e.g recv().
     *
     * If an event has been signaled and not consumed yet, it will be set in
     * both pending_events and reported_events (as we should only ever report
     * any event once until it is reset.) */
    unsigned int        reported_events;
    unsigned int        flags;       /* socket flags */
    unsigned short      proto;       /* socket protocol */
    unsigned short      type;        /* socket type */
    unsigned short      family;      /* socket family */
    struct event       *event;       /* event object */
    user_handle_t       window;      /* window to send the message to */
    unsigned int        message;     /* message to send */
    obj_handle_t        wparam;      /* message wparam (socket handle) */
    int                 errors[AFD_POLL_BIT_COUNT]; /* event errors */
    timeout_t           connect_time;/* time the socket was connected */
    struct sock        *deferred;    /* socket that waits for a deferred accept */
    struct async_queue  read_q;      /* queue for asynchronous reads */
    struct async_queue  write_q;     /* queue for asynchronous writes */
    struct async_queue  ifchange_q;  /* queue for interface change notifications */
    struct async_queue  accept_q;    /* queue for asynchronous accepts */
    struct async_queue  connect_q;   /* queue for asynchronous connects */
    struct async_queue  poll_q;      /* queue for asynchronous polls */
    struct object      *ifchange_obj; /* the interface change notification object */
    struct list         ifchange_entry; /* entry in ifchange notification list */
    struct list         accept_list; /* list of pending accept requests */
    struct accept_req  *accept_recv_req; /* pending accept-into request which will recv on this socket */
    struct connect_req *connect_req; /* pending connection request */
    struct poll_req    *main_poll;   /* main poll */
    union win_sockaddr  addr;        /* socket name */
    int                 addr_len;    /* socket name length */
    unsigned int        rcvbuf;      /* advisory recv buffer size */
    unsigned int        sndbuf;      /* advisory send buffer size */
    unsigned int        rcvtimeo;    /* receive timeout in ms */
    unsigned int        sndtimeo;    /* send timeout in ms */
    unsigned int        rd_shutdown : 1; /* is the read end shut down? */
    unsigned int        wr_shutdown : 1; /* is the write end shut down? */
    unsigned int        wr_shutdown_pending : 1; /* is a write shutdown pending? */
    unsigned int        hangup : 1;  /* has the read end received a hangup? */
    unsigned int        aborted : 1; /* did we get a POLLERR or irregular POLLHUP? */
    unsigned int        nonblocking : 1; /* is the socket nonblocking? */
    unsigned int        bound : 1;   /* is the socket bound? */
};

static void sock_dump( struct object *obj, int verbose );
static struct fd *sock_get_fd( struct object *obj );
static int sock_close_handle( struct object *obj, struct process *process, obj_handle_t handle );
static void sock_destroy( struct object *obj );
static struct object *sock_get_ifchange( struct sock *sock );
static void sock_release_ifchange( struct sock *sock );

static int sock_get_poll_events( struct fd *fd );
static void sock_poll_event( struct fd *fd, int event );
static enum server_fd_type sock_get_fd_type( struct fd *fd );
static void sock_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );
static void sock_cancel_async( struct fd *fd, struct async *async );
static void sock_queue_async( struct fd *fd, struct async *async, int type, int count );
static void sock_reselect_async( struct fd *fd, struct async_queue *queue );

static int accept_into_socket( struct sock *sock, struct sock *acceptsock );
static struct sock *accept_socket( struct sock *sock );
static int sock_get_ntstatus( int err );
static unsigned int sock_get_error( int err );
static void poll_socket( struct sock *poll_sock, struct async *async, int exclusive, timeout_t timeout,
                         unsigned int count, const struct afd_poll_socket_64 *sockets );

static const struct object_ops sock_ops =
{
    sizeof(struct sock),          /* size */
    &file_type,                   /* type */
    sock_dump,                    /* dump */
    add_queue,                    /* add_queue */
    remove_queue,                 /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    no_signal,                    /* signal */
    sock_get_fd,                  /* get_fd */
    default_map_access,           /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_get_full_name,             /* get_full_name */
    no_lookup_name,               /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    sock_close_handle,            /* close_handle */
    sock_destroy                  /* destroy */
};

static const struct fd_ops sock_fd_ops =
{
    sock_get_poll_events,         /* get_poll_events */
    sock_poll_event,              /* poll_event */
    sock_get_fd_type,             /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
    no_fd_flush,                  /* flush */
    default_fd_get_file_info,     /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    sock_ioctl,                   /* ioctl */
    sock_cancel_async,            /* cancel_async */
    sock_queue_async,             /* queue_async */
    sock_reselect_async           /* reselect_async */
};

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
        return -1;

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
        return 0;
    }
}

/* some events are generated at the same time but must be sent in a particular
 * order (e.g. CONNECT must be sent before READ) */
static const enum afd_poll_bit event_bitorder[] =
{
    AFD_POLL_BIT_CONNECT,
    AFD_POLL_BIT_CONNECT_ERR,
    AFD_POLL_BIT_ACCEPT,
    AFD_POLL_BIT_OOB,
    AFD_POLL_BIT_WRITE,
    AFD_POLL_BIT_READ,
    AFD_POLL_BIT_RESET,
    AFD_POLL_BIT_HUP,
    AFD_POLL_BIT_CLOSE,
};

typedef enum {
    SOCK_SHUTDOWN_ERROR = -1,
    SOCK_SHUTDOWN_EOF = 0,
    SOCK_SHUTDOWN_POLLHUP = 1
} sock_shutdown_t;

static sock_shutdown_t sock_shutdown_type = SOCK_SHUTDOWN_ERROR;

static sock_shutdown_t sock_check_pollhup(void)
{
    sock_shutdown_t ret = SOCK_SHUTDOWN_ERROR;
    int fd[2], n;
    struct pollfd pfd;
    char dummy;

    if ( socketpair( AF_UNIX, SOCK_STREAM, 0, fd ) ) return ret;
    if ( shutdown( fd[0], 1 ) ) goto out;

    pfd.fd = fd[1];
    pfd.events = POLLIN;
    pfd.revents = 0;

    /* Solaris' poll() sometimes returns nothing if given a 0ms timeout here */
    n = poll( &pfd, 1, 1 );
    if ( n != 1 ) goto out; /* error or timeout */
    if ( pfd.revents & POLLHUP )
        ret = SOCK_SHUTDOWN_POLLHUP;
    else if ( pfd.revents & POLLIN &&
              read( fd[1], &dummy, 1 ) == 0 )
        ret = SOCK_SHUTDOWN_EOF;

out:
    close( fd[0] );
    close( fd[1] );
    return ret;
}

void sock_init(void)
{
    sock_shutdown_type = sock_check_pollhup();

    switch ( sock_shutdown_type )
    {
    case SOCK_SHUTDOWN_EOF:
        if (debug_level) fprintf( stderr, "sock_init: shutdown() causes EOF\n" );
        break;
    case SOCK_SHUTDOWN_POLLHUP:
        if (debug_level) fprintf( stderr, "sock_init: shutdown() causes POLLHUP\n" );
        break;
    default:
        fprintf( stderr, "sock_init: ERROR in sock_check_pollhup()\n" );
        sock_shutdown_type = SOCK_SHUTDOWN_EOF;
    }
}

static int sock_reselect( struct sock *sock )
{
    int ev = sock_get_poll_events( sock->fd );

    if (debug_level)
        fprintf(stderr,"sock_reselect(%p): new mask %x\n", sock, ev);

    set_fd_events( sock->fd, ev );
    return ev;
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

/* wake anybody waiting on the socket event or send the associated message */
static void sock_wake_up( struct sock *sock )
{
    unsigned int events = sock->pending_events & sock->mask;
    int i;

    if (sock->event)
    {
        if (debug_level) fprintf(stderr, "signalling events %x ptr %p\n", events, sock->event );
        if (events)
            set_event( sock->event );
    }
    if (sock->window)
    {
        if (debug_level) fprintf(stderr, "signalling events %x win %08x\n", events, sock->window );
        for (i = 0; i < ARRAY_SIZE(event_bitorder); i++)
        {
            enum afd_poll_bit event = event_bitorder[i];
            if (events & (1 << event))
            {
                lparam_t lparam = afd_poll_flag_to_win32(1 << event) | (sock_get_error( sock->errors[event] ) << 16);
                post_message( sock->window, sock->message, sock->wparam, lparam );
            }
        }
        sock->pending_events = 0;
        sock_reselect( sock );
    }
}

static inline int sock_error( struct fd *fd )
{
    unsigned int optval = 0;
    socklen_t optlen = sizeof(optval);

    getsockopt( get_unix_fd(fd), SOL_SOCKET, SO_ERROR, (void *) &optval, &optlen);
    return optval;
}

static void free_accept_req( void *private )
{
    struct accept_req *req = private;
    list_remove( &req->entry );
    if (req->acceptsock)
    {
        req->acceptsock->accept_recv_req = NULL;
        release_object( req->acceptsock );
    }
    release_object( req->async );
    release_object( req->iosb );
    release_object( req->sock );
    free( req );
}

static void fill_accept_output( struct accept_req *req )
{
    const data_size_t out_size = req->iosb->out_size;
    struct async *async = req->async;
    union unix_sockaddr unix_addr;
    struct WS_sockaddr *win_addr;
    unsigned int remote_len;
    socklen_t unix_len;
    int fd, size = 0;
    char *out_data;
    int win_len;

    if (!(out_data = mem_alloc( out_size )))
    {
        async_terminate( async, get_error() );
        return;
    }

    fd = get_unix_fd( req->acceptsock->fd );

    if (req->recv_len && (size = recv( fd, out_data, req->recv_len, 0 )) < 0)
    {
        if (!req->accepted && errno == EWOULDBLOCK)
        {
            req->accepted = 1;
            sock_reselect( req->acceptsock );
            return;
        }

        async_terminate( async, sock_get_ntstatus( errno ) );
        free( out_data );
        return;
    }

    if (req->local_len)
    {
        if (req->local_len < sizeof(int))
        {
            async_terminate( async, STATUS_BUFFER_TOO_SMALL );
            free( out_data );
            return;
        }

        unix_len = sizeof(unix_addr);
        win_addr = (struct WS_sockaddr *)(out_data + req->recv_len + sizeof(int));
        if (getsockname( fd, &unix_addr.addr, &unix_len ) < 0 ||
            (win_len = sockaddr_from_unix( &unix_addr, win_addr, req->local_len - sizeof(int) )) < 0)
        {
            async_terminate( async, sock_get_ntstatus( errno ) );
            free( out_data );
            return;
        }
        memcpy( out_data + req->recv_len, &win_len, sizeof(int) );
    }

    unix_len = sizeof(unix_addr);
    win_addr = (struct WS_sockaddr *)(out_data + req->recv_len + req->local_len + sizeof(int));
    remote_len = out_size - req->recv_len - req->local_len;
    if (getpeername( fd, &unix_addr.addr, &unix_len ) < 0 ||
        (win_len = sockaddr_from_unix( &unix_addr, win_addr, remote_len - sizeof(int) )) < 0)
    {
        async_terminate( async, sock_get_ntstatus( errno ) );
        free( out_data );
        return;
    }
    memcpy( out_data + req->recv_len + req->local_len, &win_len, sizeof(int) );

    async_request_complete( req->async, STATUS_SUCCESS, size, out_size, out_data );
}

static void complete_async_accept( struct sock *sock, struct accept_req *req )
{
    struct sock *acceptsock = req->acceptsock;
    struct async *async = req->async;

    if (debug_level) fprintf( stderr, "completing accept request for socket %p\n", sock );

    if (acceptsock)
    {
        if (!accept_into_socket( sock, acceptsock ))
        {
            async_terminate( async, get_error() );
            return;
        }
        fill_accept_output( req );
    }
    else
    {
        obj_handle_t handle;

        if (!(acceptsock = accept_socket( sock )))
        {
            async_terminate( async, get_error() );
            return;
        }
        handle = alloc_handle_no_access_check( async_get_thread( async )->process, &acceptsock->obj,
                                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, OBJ_INHERIT );
        acceptsock->wparam = handle;
        sock_reselect( acceptsock );
        release_object( acceptsock );
        if (!handle)
        {
            async_terminate( async, get_error() );
            return;
        }

        async_request_complete_alloc( req->async, STATUS_SUCCESS, 0, sizeof(handle), &handle );
    }
}

static void complete_async_accept_recv( struct accept_req *req )
{
    if (debug_level) fprintf( stderr, "completing accept recv request for socket %p\n", req->acceptsock );

    assert( req->recv_len );

    fill_accept_output( req );
}

static void free_connect_req( void *private )
{
    struct connect_req *req = private;

    req->sock->connect_req = NULL;
    release_object( req->async );
    release_object( req->iosb );
    release_object( req->sock );
    free( req );
}

static void complete_async_connect( struct sock *sock )
{
    struct connect_req *req = sock->connect_req;
    const char *in_buffer;
    size_t len;
    int ret;

    if (debug_level) fprintf( stderr, "completing connect request for socket %p\n", sock );

    sock->state = SOCK_CONNECTED;

    if (!req->send_len)
    {
        async_terminate( req->async, STATUS_SUCCESS );
        return;
    }

    in_buffer = (const char *)req->iosb->in_data + sizeof(struct afd_connect_params) + req->addr_len;
    len = req->send_len - req->send_cursor;

    ret = send( get_unix_fd( sock->fd ), in_buffer + req->send_cursor, len, 0 );
    if (ret < 0 && errno != EWOULDBLOCK)
        async_terminate( req->async, sock_get_ntstatus( errno ) );
    else if (ret == len)
        async_request_complete( req->async, STATUS_SUCCESS, req->send_len, 0, NULL );
    else
        req->send_cursor += ret;
}

static void free_poll_req( void *private )
{
    struct poll_req *req = private;
    unsigned int i;

    if (req->timeout) remove_timeout_user( req->timeout );

    for (i = 0; i < req->count; ++i)
        release_object( req->sockets[i].sock );
    release_object( req->async );
    release_object( req->iosb );
    list_remove( &req->entry );
    free( req );
}

static int is_oobinline( struct sock *sock )
{
    int oobinline;
    socklen_t len = sizeof(oobinline);
    return !getsockopt( get_unix_fd( sock->fd ), SOL_SOCKET, SO_OOBINLINE, (char *)&oobinline, &len ) && oobinline;
}

static int get_poll_flags( struct sock *sock, int event )
{
    int flags = 0;

    /* A connection-mode socket which has never been connected does not return
     * write or hangup events, but Linux reports POLLOUT | POLLHUP. */
    if (sock->state == SOCK_UNCONNECTED)
        event &= ~(POLLOUT | POLLHUP);

    if (event & POLLIN)
    {
        if (sock->state == SOCK_LISTENING)
            flags |= AFD_POLL_ACCEPT;
        else
            flags |= AFD_POLL_READ;
    }
    if (event & POLLPRI)
        flags |= is_oobinline( sock ) ? AFD_POLL_READ : AFD_POLL_OOB;
    if (event & POLLOUT)
        flags |= AFD_POLL_WRITE;
    if (sock->state == SOCK_CONNECTED)
        flags |= AFD_POLL_CONNECT;
    if (event & POLLHUP)
        flags |= AFD_POLL_HUP;
    if (event & POLLERR)
        flags |= AFD_POLL_CONNECT_ERR;

    return flags;
}

static void complete_async_poll( struct poll_req *req, unsigned int status )
{
    unsigned int i, signaled_count = 0;

    for (i = 0; i < req->count; ++i)
    {
        struct sock *sock = req->sockets[i].sock;

        if (sock->main_poll == req)
            sock->main_poll = NULL;
    }

    if (!status)
    {
        for (i = 0; i < req->count; ++i)
        {
            if (req->sockets[i].flags)
                ++signaled_count;
        }
    }

    if (is_machine_64bit( async_get_thread( req->async )->process->machine ))
    {
        size_t output_size = offsetof( struct afd_poll_params_64, sockets[signaled_count] );
        struct afd_poll_params_64 *output;

        if (!(output = mem_alloc( output_size )))
        {
            async_terminate( req->async, get_error() );
            return;
        }
        memset( output, 0, output_size );
        output->timeout = req->orig_timeout;
        output->exclusive = req->exclusive;
        for (i = 0; i < req->count; ++i)
        {
            if (!req->sockets[i].flags) continue;
            output->sockets[output->count].socket = req->sockets[i].handle;
            output->sockets[output->count].flags = req->sockets[i].flags;
            output->sockets[output->count].status = req->sockets[i].status;
            ++output->count;
        }
        assert( output->count == signaled_count );

        async_request_complete( req->async, status, output_size, output_size, output );
    }
    else
    {
        size_t output_size = offsetof( struct afd_poll_params_32, sockets[signaled_count] );
        struct afd_poll_params_32 *output;

        if (!(output = mem_alloc( output_size )))
        {
            async_terminate( req->async, get_error() );
            return;
        }
        memset( output, 0, output_size );
        output->timeout = req->orig_timeout;
        output->exclusive = req->exclusive;
        for (i = 0; i < req->count; ++i)
        {
            if (!req->sockets[i].flags) continue;
            output->sockets[output->count].socket = req->sockets[i].handle;
            output->sockets[output->count].flags = req->sockets[i].flags;
            output->sockets[output->count].status = req->sockets[i].status;
            ++output->count;
        }
        assert( output->count == signaled_count );

        async_request_complete( req->async, status, output_size, output_size, output );
    }
}

static void complete_async_polls( struct sock *sock, int event, int error )
{
    int flags = get_poll_flags( sock, event );
    struct poll_req *req, *next;

    LIST_FOR_EACH_ENTRY_SAFE( req, next, &poll_list, struct poll_req, entry )
    {
        unsigned int i;

        if (req->iosb->status != STATUS_PENDING) continue;

        for (i = 0; i < req->count; ++i)
        {
            if (req->sockets[i].sock != sock) continue;
            if (!(req->sockets[i].mask & flags)) continue;

            if (debug_level)
                fprintf( stderr, "completing poll for socket %p, wanted %#x got %#x\n",
                         sock, req->sockets[i].mask, flags );

            req->sockets[i].flags = req->sockets[i].mask & flags;
            req->sockets[i].status = sock_get_ntstatus( error );

            complete_async_poll( req, STATUS_SUCCESS );
            break;
        }
    }
}

static void async_poll_timeout( void *private )
{
    struct poll_req *req = private;

    req->timeout = NULL;

    if (req->iosb->status != STATUS_PENDING) return;

    complete_async_poll( req, STATUS_TIMEOUT );
}

static int sock_dispatch_asyncs( struct sock *sock, int event, int error )
{
    if (event & (POLLIN | POLLPRI))
    {
        struct accept_req *req;

        LIST_FOR_EACH_ENTRY( req, &sock->accept_list, struct accept_req, entry )
        {
            if (req->iosb->status == STATUS_PENDING && !req->accepted)
            {
                complete_async_accept( sock, req );
                break;
            }
        }

        if (sock->accept_recv_req && sock->accept_recv_req->iosb->status == STATUS_PENDING)
            complete_async_accept_recv( sock->accept_recv_req );
    }

    if ((event & POLLOUT) && sock->connect_req && sock->connect_req->iosb->status == STATUS_PENDING)
        complete_async_connect( sock );

    if (event & (POLLIN | POLLPRI) && async_waiting( &sock->read_q ))
    {
        if (debug_level) fprintf( stderr, "activating read queue for socket %p\n", sock );
        async_wake_up( &sock->read_q, STATUS_ALERTED );
        event &= ~(POLLIN | POLLPRI);
    }

    if (event & POLLOUT && async_waiting( &sock->write_q ))
    {
        if (debug_level) fprintf( stderr, "activating write queue for socket %p\n", sock );
        async_wake_up( &sock->write_q, STATUS_ALERTED );
        event &= ~POLLOUT;
    }

    if (event & (POLLERR | POLLHUP))
    {
        int status = sock_get_ntstatus( error );
        struct accept_req *req, *next;

        if (sock->rd_shutdown || sock->hangup)
            async_wake_up( &sock->read_q, status );
        if (sock->wr_shutdown)
            async_wake_up( &sock->write_q, status );

        LIST_FOR_EACH_ENTRY_SAFE( req, next, &sock->accept_list, struct accept_req, entry )
        {
            if (req->iosb->status == STATUS_PENDING)
                async_terminate( req->async, status );
        }

        if (sock->accept_recv_req && sock->accept_recv_req->iosb->status == STATUS_PENDING)
            async_terminate( sock->accept_recv_req->async, status );

        if (sock->connect_req)
            async_terminate( sock->connect_req->async, status );
    }

    return event;
}

static void post_socket_event( struct sock *sock, enum afd_poll_bit event_bit, int error )
{
    unsigned int event = (1 << event_bit);

    if (!(sock->reported_events & event))
    {
        sock->pending_events |= event;
        sock->reported_events |= event;
        sock->errors[event_bit] = error;
    }
}

static void sock_dispatch_events( struct sock *sock, enum connection_state prevstate, int event, int error )
{
    switch (prevstate)
    {
    case SOCK_UNCONNECTED:
        break;

    case SOCK_CONNECTING:
        if (event & POLLOUT)
        {
            post_socket_event( sock, AFD_POLL_BIT_CONNECT, 0 );
            sock->errors[AFD_POLL_BIT_CONNECT_ERR] = 0;
        }
        if (event & (POLLERR | POLLHUP))
            post_socket_event( sock, AFD_POLL_BIT_CONNECT_ERR, error );
        break;

    case SOCK_LISTENING:
        if (event & (POLLIN | POLLERR | POLLHUP))
            post_socket_event( sock, AFD_POLL_BIT_ACCEPT, error );
        break;

    case SOCK_CONNECTED:
    case SOCK_CONNECTIONLESS:
        if (event & POLLIN)
            post_socket_event( sock, AFD_POLL_BIT_READ, 0 );

        if (event & POLLOUT)
            post_socket_event( sock, AFD_POLL_BIT_WRITE, 0 );

        if (event & POLLPRI)
            post_socket_event( sock, AFD_POLL_BIT_OOB, 0 );

        if (event & (POLLERR | POLLHUP))
            post_socket_event( sock, AFD_POLL_BIT_HUP, error );
        break;
    }

    sock_wake_up( sock );
}

static void sock_poll_event( struct fd *fd, int event )
{
    struct sock *sock = get_fd_user( fd );
    int hangup_seen = 0;
    enum connection_state prevstate = sock->state;
    int error = 0;

    assert( sock->obj.ops == &sock_ops );
    if (debug_level)
        fprintf(stderr, "socket %p select event: %x\n", sock, event);

    /* we may change event later, remove from loop here */
    if (event & (POLLERR|POLLHUP)) set_fd_events( sock->fd, -1 );

    switch (sock->state)
    {
    case SOCK_UNCONNECTED:
        break;

    case SOCK_CONNECTING:
        if (event & (POLLERR|POLLHUP))
        {
            sock->state = SOCK_UNCONNECTED;
            event &= ~POLLOUT;
            error = sock_error( fd );
        }
        else if (event & POLLOUT)
        {
            sock->state = SOCK_CONNECTED;
            sock->connect_time = current_time;
        }
        break;

    case SOCK_LISTENING:
        if (event & (POLLERR|POLLHUP))
            error = sock_error( fd );
        break;

    case SOCK_CONNECTED:
    case SOCK_CONNECTIONLESS:
        if (sock->type == WS_SOCK_STREAM && (event & POLLIN))
        {
            char dummy;
            int nr;

            /* Linux 2.4 doesn't report POLLHUP if only one side of the socket
             * has been closed, so we need to check for it explicitly here */
            nr  = recv( get_unix_fd( fd ), &dummy, 1, MSG_PEEK );
            if ( nr == 0 )
            {
                hangup_seen = 1;
                event &= ~POLLIN;
            }
            else if ( nr < 0 )
            {
                event &= ~POLLIN;
                /* EAGAIN can happen if an async recv() falls between the server's poll()
                   call and the invocation of this routine */
                if ( errno != EAGAIN )
                {
                    error = errno;
                    event |= POLLERR;
                    if ( debug_level )
                        fprintf( stderr, "recv error on socket %p: %d\n", sock, errno );
                }
            }
        }

        if (hangup_seen || (sock_shutdown_type == SOCK_SHUTDOWN_POLLHUP && (event & POLLHUP)))
        {
            sock->hangup = 1;
        }
        else if (event & (POLLHUP | POLLERR))
        {
            sock->aborted = 1;

            if (debug_level)
                fprintf( stderr, "socket %p aborted by error %d, event %#x\n", sock, error, event );
        }

        if (hangup_seen)
            event |= POLLHUP;
        break;
    }

    complete_async_polls( sock, event, error );

    event = sock_dispatch_asyncs( sock, event, error );
    sock_dispatch_events( sock, prevstate, event, error );

    sock_reselect( sock );
}

static void sock_dump( struct object *obj, int verbose )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );
    fprintf( stderr, "Socket fd=%p, state=%x, mask=%x, pending=%x, reported=%x\n",
            sock->fd, sock->state,
            sock->mask, sock->pending_events, sock->reported_events );
}

static int poll_flags_from_afd( struct sock *sock, int flags )
{
    int ev = 0;

    /* A connection-mode socket which has never been connected does
     * not return write or hangup events, but Linux returns
     * POLLOUT | POLLHUP. */
    if (sock->state == SOCK_UNCONNECTED)
        return -1;

    if (flags & (AFD_POLL_READ | AFD_POLL_ACCEPT))
        ev |= POLLIN;
    if ((flags & AFD_POLL_HUP) && sock->type == WS_SOCK_STREAM)
        ev |= POLLIN;
    if (flags & AFD_POLL_OOB)
        ev |= is_oobinline( sock ) ? POLLIN : POLLPRI;
    if (flags & AFD_POLL_WRITE)
        ev |= POLLOUT;

    return ev;
}

static int sock_get_poll_events( struct fd *fd )
{
    struct sock *sock = get_fd_user( fd );
    unsigned int mask = sock->mask & ~sock->reported_events;
    struct poll_req *req;
    int ev = 0;

    assert( sock->obj.ops == &sock_ops );

    if (!sock->type) /* not initialized yet */
        return -1;

    switch (sock->state)
    {
    case SOCK_UNCONNECTED:
        /* A connection-mode Windows socket which has never been connected does
         * not return any events, but Linux returns POLLOUT | POLLHUP. Hence we
         * need to return -1 here, to prevent the socket from being polled on at
         * all. */
        return -1;

    case SOCK_CONNECTING:
        return POLLOUT;

    case SOCK_LISTENING:
        if (!list_empty( &sock->accept_list ) || (mask & AFD_POLL_ACCEPT))
            ev |= POLLIN;
        break;

    case SOCK_CONNECTED:
    case SOCK_CONNECTIONLESS:
        if (sock->hangup && sock->wr_shutdown && !sock->wr_shutdown_pending)
        {
            /* Linux returns POLLHUP if a socket is both SHUT_RD and SHUT_WR, or
             * if both the socket and its peer are SHUT_WR.
             *
             * We don't use SHUT_RD, so we can only encounter this in the latter
             * case. In that case there can't be any pending read requests (they
             * would have already been completed with a length of zero), the
             * above condition ensures that we don't have any pending write
             * requests, and nothing that can change about the socket state that
             * would complete a pending poll request. */
            return -1;
        }

        if (sock->aborted)
            return -1;

        if (sock->accept_recv_req)
        {
            ev |= POLLIN;
        }
        else if (async_queued( &sock->read_q ))
        {
            if (async_waiting( &sock->read_q )) ev |= POLLIN | POLLPRI;
        }
        else
        {
            /* Don't ask for POLLIN if we got a hangup. We won't receive more
             * data anyway, but we will get POLLIN if SOCK_SHUTDOWN_EOF. */
            if (!sock->hangup)
            {
                if (mask & AFD_POLL_READ)
                    ev |= POLLIN;
                if (mask & AFD_POLL_OOB)
                    ev |= POLLPRI;
            }

            /* We use POLLIN with 0 bytes recv() as hangup indication for stream sockets. */
            if (sock->state == SOCK_CONNECTED && (mask & AFD_POLL_HUP) && !(sock->reported_events & AFD_POLL_READ))
                ev |= POLLIN;
        }

        if (async_queued( &sock->write_q ))
        {
            if (async_waiting( &sock->write_q )) ev |= POLLOUT;
        }
        else if (!sock->wr_shutdown && (mask & AFD_POLL_WRITE))
        {
            ev |= POLLOUT;
        }

        break;
    }

    LIST_FOR_EACH_ENTRY( req, &poll_list, struct poll_req, entry )
    {
        unsigned int i;

        for (i = 0; i < req->count; ++i)
        {
            if (req->sockets[i].sock != sock) continue;

            ev |= poll_flags_from_afd( sock, req->sockets[i].mask );
        }
    }

    return ev;
}

static enum server_fd_type sock_get_fd_type( struct fd *fd )
{
    return FD_TYPE_SOCKET;
}

static void sock_cancel_async( struct fd *fd, struct async *async )
{
    struct poll_req *req;

    LIST_FOR_EACH_ENTRY( req, &poll_list, struct poll_req, entry )
    {
        unsigned int i;

        if (req->async != async)
            continue;

        for (i = 0; i < req->count; i++)
        {
            struct sock *sock = req->sockets[i].sock;

            if (sock->main_poll == req)
                sock->main_poll = NULL;
        }
    }

    async_terminate( async, STATUS_CANCELLED );
}

static void sock_queue_async( struct fd *fd, struct async *async, int type, int count )
{
    struct sock *sock = get_fd_user( fd );
    struct async_queue *queue;

    assert( sock->obj.ops == &sock_ops );

    switch (type)
    {
    case ASYNC_TYPE_READ:
        if (sock->rd_shutdown)
        {
            set_error( STATUS_PIPE_DISCONNECTED );
            return;
        }
        queue = &sock->read_q;
        break;

    case ASYNC_TYPE_WRITE:
        if (sock->wr_shutdown)
        {
            set_error( STATUS_PIPE_DISCONNECTED );
            return;
        }
        queue = &sock->write_q;
        break;

    default:
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (sock->state != SOCK_CONNECTED)
    {
        set_error( STATUS_PIPE_DISCONNECTED );
        return;
    }

    queue_async( queue, async );
    sock_reselect( sock );

    set_error( STATUS_PENDING );
}

static void sock_reselect_async( struct fd *fd, struct async_queue *queue )
{
    struct sock *sock = get_fd_user( fd );

    if (sock->wr_shutdown_pending && list_empty( &sock->write_q.queue ))
    {
        shutdown( get_unix_fd( sock->fd ), SHUT_WR );
        sock->wr_shutdown_pending = 0;
    }

    /* Don't reselect the ifchange queue; we always ask for POLLIN.
     * Don't reselect an uninitialized socket; we can't call set_fd_events() on
     * a pseudo-fd. */
    if (queue != &sock->ifchange_q && sock->type)
        sock_reselect( sock );
}

static struct fd *sock_get_fd( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    return (struct fd *)grab_object( sock->fd );
}

static int sock_close_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    struct sock *sock = (struct sock *)obj;

    if (sock->obj.handle_count == 1) /* last handle */
    {
        struct accept_req *accept_req, *accept_next;
        struct poll_req *poll_req, *poll_next;

        if (sock->accept_recv_req)
            async_terminate( sock->accept_recv_req->async, STATUS_CANCELLED );

        LIST_FOR_EACH_ENTRY_SAFE( accept_req, accept_next, &sock->accept_list, struct accept_req, entry )
            async_terminate( accept_req->async, STATUS_CANCELLED );

        if (sock->connect_req)
            async_terminate( sock->connect_req->async, STATUS_CANCELLED );

        LIST_FOR_EACH_ENTRY_SAFE( poll_req, poll_next, &poll_list, struct poll_req, entry )
        {
            struct iosb *iosb = poll_req->iosb;
            BOOL signaled = FALSE;
            unsigned int i;

            if (iosb->status != STATUS_PENDING) continue;

            for (i = 0; i < poll_req->count; ++i)
            {
                if (poll_req->sockets[i].sock == sock)
                {
                    signaled = TRUE;
                    poll_req->sockets[i].flags = AFD_POLL_CLOSE;
                    poll_req->sockets[i].status = 0;
                }
            }

            if (signaled) complete_async_poll( poll_req, STATUS_SUCCESS );
        }
    }

    return 1;
}

static void sock_destroy( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;

    assert( obj->ops == &sock_ops );

    /* FIXME: special socket shutdown stuff? */

    if ( sock->deferred )
        release_object( sock->deferred );

    async_wake_up( &sock->ifchange_q, STATUS_CANCELLED );
    sock_release_ifchange( sock );
    free_async_queue( &sock->read_q );
    free_async_queue( &sock->write_q );
    free_async_queue( &sock->ifchange_q );
    free_async_queue( &sock->accept_q );
    free_async_queue( &sock->connect_q );
    free_async_queue( &sock->poll_q );
    if (sock->event) release_object( sock->event );
    if (sock->fd)
    {
        /* shut the socket down to force pending poll() calls in the client to return */
        shutdown( get_unix_fd(sock->fd), SHUT_RDWR );
        release_object( sock->fd );
    }
}

static struct sock *create_socket(void)
{
    struct sock *sock;

    if (!(sock = alloc_object( &sock_ops ))) return NULL;
    sock->fd      = NULL;
    sock->state   = SOCK_UNCONNECTED;
    sock->mask    = 0;
    sock->pending_events = 0;
    sock->reported_events = 0;
    sock->flags   = 0;
    sock->proto   = 0;
    sock->type    = 0;
    sock->family  = 0;
    sock->event   = NULL;
    sock->window  = 0;
    sock->message = 0;
    sock->wparam  = 0;
    sock->connect_time = 0;
    sock->deferred = NULL;
    sock->ifchange_obj = NULL;
    sock->accept_recv_req = NULL;
    sock->connect_req = NULL;
    sock->main_poll = NULL;
    memset( &sock->addr, 0, sizeof(sock->addr) );
    sock->addr_len = 0;
    sock->rd_shutdown = 0;
    sock->wr_shutdown = 0;
    sock->wr_shutdown_pending = 0;
    sock->hangup = 0;
    sock->aborted = 0;
    sock->nonblocking = 0;
    sock->bound = 0;
    sock->rcvbuf = 0;
    sock->sndbuf = 0;
    sock->rcvtimeo = 0;
    sock->sndtimeo = 0;
    init_async_queue( &sock->read_q );
    init_async_queue( &sock->write_q );
    init_async_queue( &sock->ifchange_q );
    init_async_queue( &sock->accept_q );
    init_async_queue( &sock->connect_q );
    init_async_queue( &sock->poll_q );
    memset( sock->errors, 0, sizeof(sock->errors) );
    list_init( &sock->accept_list );
    return sock;
}

static int get_unix_family( int family )
{
    switch (family)
    {
        case WS_AF_INET: return AF_INET;
        case WS_AF_INET6: return AF_INET6;
#ifdef HAS_IPX
        case WS_AF_IPX: return AF_IPX;
#endif
#ifdef AF_IRDA
        case WS_AF_IRDA: return AF_IRDA;
#endif
        case WS_AF_UNSPEC: return AF_UNSPEC;
        default: return -1;
    }
}

static int get_unix_type( int type )
{
    switch (type)
    {
        case WS_SOCK_DGRAM: return SOCK_DGRAM;
        case WS_SOCK_RAW: return SOCK_RAW;
        case WS_SOCK_STREAM: return SOCK_STREAM;
        default: return -1;
    }
}

static int get_unix_protocol( int protocol )
{
    if (protocol >= WS_NSPROTO_IPX && protocol <= WS_NSPROTO_IPX + 255)
        return protocol;

    switch (protocol)
    {
        case WS_IPPROTO_ICMP: return IPPROTO_ICMP;
        case WS_IPPROTO_IGMP: return IPPROTO_IGMP;
        case WS_IPPROTO_IP: return IPPROTO_IP;
        case WS_IPPROTO_IPV4: return IPPROTO_IPIP;
        case WS_IPPROTO_IPV6: return IPPROTO_IPV6;
        case WS_IPPROTO_RAW: return IPPROTO_RAW;
        case WS_IPPROTO_TCP: return IPPROTO_TCP;
        case WS_IPPROTO_UDP: return IPPROTO_UDP;
        default: return -1;
    }
}

static void set_dont_fragment( int fd, int level, int value )
{
    int optname;

    if (level == IPPROTO_IP)
    {
#ifdef IP_DONTFRAG
        optname = IP_DONTFRAG;
#elif defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_DO) && defined(IP_PMTUDISC_DONT)
        optname = IP_MTU_DISCOVER;
        value = value ? IP_PMTUDISC_DO : IP_PMTUDISC_DONT;
#else
        return;
#endif
    }
    else
    {
#ifdef IPV6_DONTFRAG
        optname = IPV6_DONTFRAG;
#elif defined(IPV6_MTU_DISCOVER) && defined(IPV6_PMTUDISC_DO) && defined(IPV6_PMTUDISC_DONT)
        optname = IPV6_MTU_DISCOVER;
        value = value ? IPV6_PMTUDISC_DO : IPV6_PMTUDISC_DONT;
#else
        return;
#endif
    }

    setsockopt( fd, level, optname, &value, sizeof(value) );
}

static int init_socket( struct sock *sock, int family, int type, int protocol, unsigned int flags )
{
    unsigned int options = 0;
    int sockfd, unix_type, unix_family, unix_protocol, value;
    socklen_t len;

    unix_family = get_unix_family( family );
    unix_type = get_unix_type( type );
    unix_protocol = get_unix_protocol( protocol );

    if (unix_protocol < 0)
    {
        if (type && unix_type < 0)
            set_win32_error( WSAESOCKTNOSUPPORT );
        else
            set_win32_error( WSAEPROTONOSUPPORT );
        return -1;
    }
    if (unix_family < 0)
    {
        if (family >= 0 && unix_type < 0)
            set_win32_error( WSAESOCKTNOSUPPORT );
        else
            set_win32_error( WSAEAFNOSUPPORT );
        return -1;
    }

    sockfd = socket( unix_family, unix_type, unix_protocol );
    if (sockfd == -1)
    {
        if (errno == EINVAL) set_win32_error( WSAESOCKTNOSUPPORT );
        else set_win32_error( sock_get_error( errno ));
        return -1;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK); /* make socket nonblocking */

    if (family == WS_AF_IPX && protocol >= WS_NSPROTO_IPX && protocol <= WS_NSPROTO_IPX + 255)
    {
#ifdef HAS_IPX
        int ipx_type = protocol - WS_NSPROTO_IPX;

#ifdef SOL_IPX
        setsockopt( sockfd, SOL_IPX, IPX_TYPE, &ipx_type, sizeof(ipx_type) );
#else
        struct ipx val;
        /* Should we retrieve val using a getsockopt call and then
         * set the modified one? */
        val.ipx_pt = ipx_type;
        setsockopt( sockfd, 0, SO_DEFAULT_HEADERS, &val, sizeof(val) );
#endif
#endif
    }

    if (unix_family == AF_INET || unix_family == AF_INET6)
    {
        /* ensure IP_DONTFRAGMENT is disabled for SOCK_DGRAM and SOCK_RAW, enabled for SOCK_STREAM */
        if (unix_type == SOCK_DGRAM || unix_type == SOCK_RAW) /* in Linux the global default can be enabled */
            set_dont_fragment( sockfd, unix_family == AF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP, FALSE );
        else if (unix_type == SOCK_STREAM)
            set_dont_fragment( sockfd, unix_family == AF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP, TRUE );
    }

#ifdef IPV6_V6ONLY
    if (unix_family == AF_INET6)
    {
        static const int enable = 1;
        setsockopt( sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &enable, sizeof(enable) );
    }
#endif

    len = sizeof(value);
    if (!getsockopt( sockfd, SOL_SOCKET, SO_RCVBUF, &value, &len ))
        sock->rcvbuf = value;

    len = sizeof(value);
    if (!getsockopt( sockfd, SOL_SOCKET, SO_SNDBUF, &value, &len ))
        sock->sndbuf = value;

    sock->state  = (type == WS_SOCK_STREAM ? SOCK_UNCONNECTED : SOCK_CONNECTIONLESS);
    sock->flags  = flags;
    sock->proto  = protocol;
    sock->type   = type;
    sock->family = family;

    if (sock->fd)
    {
        options = get_fd_options( sock->fd );
        release_object( sock->fd );
    }

    if (!(sock->fd = create_anonymous_fd( &sock_fd_ops, sockfd, &sock->obj, options )))
    {
        return -1;
    }

    /* We can't immediately allow caching for a connection-mode socket, since it
     * might be accepted into (changing the underlying fd object.) */
    if (sock->type != WS_SOCK_STREAM) allow_fd_caching( sock->fd );

    return 0;
}

/* accepts a socket and inits it */
static int accept_new_fd( struct sock *sock )
{

    /* Try to accept(2). We can't be safe that this an already connected socket
     * or that accept() is allowed on it. In those cases we will get -1/errno
     * return.
     */
    struct sockaddr saddr;
    socklen_t slen = sizeof(saddr);
    int acceptfd = accept( get_unix_fd(sock->fd), &saddr, &slen );
    if (acceptfd != -1)
        fcntl( acceptfd, F_SETFL, O_NONBLOCK );
    else
        set_error( sock_get_ntstatus( errno ));
    return acceptfd;
}

/* accept a socket (creates a new fd) */
static struct sock *accept_socket( struct sock *sock )
{
    struct sock *acceptsock;
    int	acceptfd;

    if (get_unix_fd( sock->fd ) == -1) return NULL;

    if ( sock->deferred )
    {
        acceptsock = sock->deferred;
        sock->deferred = NULL;
    }
    else
    {
        union unix_sockaddr unix_addr;
        socklen_t unix_len;

        if ((acceptfd = accept_new_fd( sock )) == -1) return NULL;
        if (!(acceptsock = create_socket()))
        {
            close( acceptfd );
            return NULL;
        }

        /* newly created socket gets the same properties of the listening socket */
        acceptsock->state   = SOCK_CONNECTED;
        acceptsock->bound   = 1;
        acceptsock->nonblocking = sock->nonblocking;
        acceptsock->mask    = sock->mask;
        acceptsock->proto   = sock->proto;
        acceptsock->type    = sock->type;
        acceptsock->family  = sock->family;
        acceptsock->window  = sock->window;
        acceptsock->message = sock->message;
        acceptsock->connect_time = current_time;
        if (sock->event) acceptsock->event = (struct event *)grab_object( sock->event );
        acceptsock->flags = sock->flags;
        if (!(acceptsock->fd = create_anonymous_fd( &sock_fd_ops, acceptfd, &acceptsock->obj,
                                                    get_fd_options( sock->fd ) )))
        {
            release_object( acceptsock );
            return NULL;
        }
        unix_len = sizeof(unix_addr);
        if (!getsockname( acceptfd, &unix_addr.addr, &unix_len ))
            acceptsock->addr_len = sockaddr_from_unix( &unix_addr, &acceptsock->addr.addr, sizeof(acceptsock->addr) );
    }
    clear_error();
    sock->pending_events &= ~AFD_POLL_ACCEPT;
    sock->reported_events &= ~AFD_POLL_ACCEPT;
    sock_reselect( sock );
    return acceptsock;
}

static int accept_into_socket( struct sock *sock, struct sock *acceptsock )
{
    union unix_sockaddr unix_addr;
    socklen_t unix_len;
    int acceptfd;
    struct fd *newfd;

    if (get_unix_fd( sock->fd ) == -1) return FALSE;

    if ( sock->deferred )
    {
        newfd = dup_fd_object( sock->deferred->fd, 0, 0,
                               get_fd_options( acceptsock->fd ) );
        if ( !newfd )
            return FALSE;

        set_fd_user( newfd, &sock_fd_ops, &acceptsock->obj );

        release_object( sock->deferred );
        sock->deferred = NULL;
    }
    else
    {
        if ((acceptfd = accept_new_fd( sock )) == -1)
            return FALSE;

        if (!(newfd = create_anonymous_fd( &sock_fd_ops, acceptfd, &acceptsock->obj,
                                            get_fd_options( acceptsock->fd ) )))
            return FALSE;
    }

    acceptsock->state = SOCK_CONNECTED;
    acceptsock->pending_events = 0;
    acceptsock->reported_events = 0;
    acceptsock->proto   = sock->proto;
    acceptsock->type    = sock->type;
    acceptsock->family  = sock->family;
    acceptsock->wparam  = 0;
    acceptsock->deferred = NULL;
    acceptsock->connect_time = current_time;
    fd_copy_completion( acceptsock->fd, newfd );
    release_object( acceptsock->fd );
    acceptsock->fd = newfd;

    unix_len = sizeof(unix_addr);
    if (!getsockname( get_unix_fd( newfd ), &unix_addr.addr, &unix_len ))
        acceptsock->addr_len = sockaddr_from_unix( &unix_addr, &acceptsock->addr.addr, sizeof(acceptsock->addr) );

    clear_error();
    sock->pending_events &= ~AFD_POLL_ACCEPT;
    sock->reported_events &= ~AFD_POLL_ACCEPT;
    sock_reselect( sock );

    return TRUE;
}

#ifdef IP_BOUND_IF

static int bind_to_iface_name( int fd, in_addr_t bind_addr, const char *name )
{
    static const int enable = 1;
    unsigned int index;

    if (!(index = if_nametoindex( name )))
        return -1;

    if (setsockopt( fd, IPPROTO_IP, IP_BOUND_IF, &index, sizeof(index) ))
        return -1;

    return setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable) );
}

#elif defined(IP_UNICAST_IF) && defined(SO_ATTACH_FILTER) && defined(SO_BINDTODEVICE)

struct interface_filter
{
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
static struct interface_filter generic_interface_filter =
{
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

static int bind_to_iface_name( int fd, in_addr_t bind_addr, const char *name )
{
    struct interface_filter specific_interface_filter;
    struct sock_fprog filter_prog;
    static const int enable = 1;
    unsigned int index;
    in_addr_t ifindex;

    if (!setsockopt( fd, SOL_SOCKET, SO_BINDTODEVICE, name, strlen( name ) + 1 ))
        return 0;

    /* SO_BINDTODEVICE requires NET_CAP_RAW until Linux 5.7. */
    if (debug_level)
        fprintf( stderr, "setsockopt SO_BINDTODEVICE fd %d, name %s failed: %s, falling back to SO_REUSE_ADDR\n",
                 fd, name, strerror( errno ));

    if (!(index = if_nametoindex( name )))
        return -1;

    ifindex = htonl( index );
    if (setsockopt( fd, IPPROTO_IP, IP_UNICAST_IF, &ifindex, sizeof(ifindex) ) < 0)
        return -1;

    specific_interface_filter = generic_interface_filter;
    specific_interface_filter.iface_rule.k = index;
    specific_interface_filter.ip_rule.k = htonl( bind_addr );
    filter_prog.len = sizeof(generic_interface_filter) / sizeof(struct sock_filter);
    filter_prog.filter = (struct sock_filter *)&specific_interface_filter;
    if (setsockopt( fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter_prog, sizeof(filter_prog) ))
        return -1;

    return setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable) );
}

#else

static int bind_to_iface_name( int fd, in_addr_t bind_addr, const char *name )
{
    errno = EOPNOTSUPP;
    return -1;
}

#endif /* LINUX_BOUND_IF */

/* Take bind() calls on any name corresponding to a local network adapter and
 * restrict the given socket to operating only on the specified interface. This
 * restriction consists of two components:
 *  1) An outgoing packet restriction suggesting the egress interface for all
 *     packets.
 *  2) An incoming packet restriction dropping packets not meant for the
 *     interface.
 * If the function succeeds in placing these restrictions, then the name for the
 * bind() may safely be changed to INADDR_ANY, permitting the transmission and
 * receipt of broadcast packets on the socket. This behavior is only relevant to
 * UDP sockets and is needed for applications that expect to be able to receive
 * broadcast packets on a socket that is bound to a specific network interface.
 */
static int bind_to_interface( struct sock *sock, const struct sockaddr_in *addr )
{
    in_addr_t bind_addr = addr->sin_addr.s_addr;
    struct ifaddrs *ifaddrs, *ifaddr;
    int fd = get_unix_fd( sock->fd );
    int err = 0;

    if (bind_addr == htonl( INADDR_ANY ) || bind_addr == htonl( INADDR_LOOPBACK ))
        return 0;
    if (sock->type != WS_SOCK_DGRAM)
        return 0;

    if (getifaddrs( &ifaddrs ) < 0) return 0;

    for (ifaddr = ifaddrs; ifaddr != NULL; ifaddr = ifaddr->ifa_next)
    {
        if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET
                && ((struct sockaddr_in *)ifaddr->ifa_addr)->sin_addr.s_addr == bind_addr)
        {
            if ((err = bind_to_iface_name( fd, bind_addr, ifaddr->ifa_name )) < 0)
            {
                if (debug_level)
                    fprintf( stderr, "failed to bind to interface: %s\n", strerror( errno ) );
            }
            break;
        }
    }
    freeifaddrs( ifaddrs );
    return !err;
}

#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
static unsigned int get_ipv6_interface_index( const struct in6_addr *addr )
{
    struct ifaddrs *ifaddrs, *ifaddr;

    if (getifaddrs( &ifaddrs ) < 0) return 0;

    for (ifaddr = ifaddrs; ifaddr != NULL; ifaddr = ifaddr->ifa_next)
    {
        if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET6
                && !memcmp( &((struct sockaddr_in6 *)ifaddr->ifa_addr)->sin6_addr, addr, sizeof(*addr) ))
        {
            unsigned int index = if_nametoindex( ifaddr->ifa_name );

            if (!index)
            {
                if (debug_level)
                    fprintf( stderr, "Unable to look up interface index for %s: %s\n",
                             ifaddr->ifa_name, strerror( errno ) );
                continue;
            }

            freeifaddrs( ifaddrs );
            return index;
        }
    }

    freeifaddrs( ifaddrs );
    return 0;
}
#endif

/* return an errno value mapped to a WSA error */
static unsigned int sock_get_error( int err )
{
    switch (err)
    {
        case EINTR:             return WSAEINTR;
        case EBADF:             return WSAEBADF;
        case EPERM:
        case EACCES:            return WSAEACCES;
        case EFAULT:            return WSAEFAULT;
        case EINVAL:            return WSAEINVAL;
        case EMFILE:            return WSAEMFILE;
        case EINPROGRESS:
        case EWOULDBLOCK:       return WSAEWOULDBLOCK;
        case EALREADY:          return WSAEALREADY;
        case ENOTSOCK:          return WSAENOTSOCK;
        case EDESTADDRREQ:      return WSAEDESTADDRREQ;
        case EMSGSIZE:          return WSAEMSGSIZE;
        case EPROTOTYPE:        return WSAEPROTOTYPE;
        case ENOPROTOOPT:       return WSAENOPROTOOPT;
        case EPROTONOSUPPORT:   return WSAEPROTONOSUPPORT;
        case ESOCKTNOSUPPORT:   return WSAESOCKTNOSUPPORT;
        case EOPNOTSUPP:        return WSAEOPNOTSUPP;
        case EPFNOSUPPORT:      return WSAEPFNOSUPPORT;
        case EAFNOSUPPORT:      return WSAEAFNOSUPPORT;
        case EADDRINUSE:        return WSAEADDRINUSE;
        case EADDRNOTAVAIL:     return WSAEADDRNOTAVAIL;
        case ENETDOWN:          return WSAENETDOWN;
        case ENETUNREACH:       return WSAENETUNREACH;
        case ENETRESET:         return WSAENETRESET;
        case ECONNABORTED:      return WSAECONNABORTED;
        case EPIPE:
        case ECONNRESET:        return WSAECONNRESET;
        case ENOBUFS:           return WSAENOBUFS;
        case EISCONN:           return WSAEISCONN;
        case ENOTCONN:          return WSAENOTCONN;
        case ESHUTDOWN:         return WSAESHUTDOWN;
        case ETOOMANYREFS:      return WSAETOOMANYREFS;
        case ETIMEDOUT:         return WSAETIMEDOUT;
        case ECONNREFUSED:      return WSAECONNREFUSED;
        case ELOOP:             return WSAELOOP;
        case ENAMETOOLONG:      return WSAENAMETOOLONG;
        case EHOSTDOWN:         return WSAEHOSTDOWN;
        case EHOSTUNREACH:      return WSAEHOSTUNREACH;
        case ENOTEMPTY:         return WSAENOTEMPTY;
#ifdef EPROCLIM
        case EPROCLIM:          return WSAEPROCLIM;
#endif
#ifdef EUSERS
        case EUSERS:            return WSAEUSERS;
#endif
#ifdef EDQUOT
        case EDQUOT:            return WSAEDQUOT;
#endif
#ifdef ESTALE
        case ESTALE:            return WSAESTALE;
#endif
#ifdef EREMOTE
        case EREMOTE:           return WSAEREMOTE;
#endif

        case 0:                 return 0;
        default:
            errno = err;
            perror("wineserver: sock_get_error() can't map error");
            return WSAEFAULT;
    }
}

static int sock_get_ntstatus( int err )
{
    switch ( err )
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
        /* Linux returns ENODEV when specifying an invalid sin6_scope_id;
         * Windows returns STATUS_INVALID_ADDRESS_COMPONENT */
        case ENODEV:
        case EADDRNOTAVAIL:     return STATUS_INVALID_ADDRESS_COMPONENT;
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
            errno = err;
            perror("wineserver: sock_get_ntstatus() can't map error");
            return STATUS_UNSUCCESSFUL;
    }
}

static struct accept_req *alloc_accept_req( struct sock *sock, struct sock *acceptsock, struct async *async,
                                            const struct afd_accept_into_params *params )
{
    struct accept_req *req = mem_alloc( sizeof(*req) );

    if (req)
    {
        req->async = (struct async *)grab_object( async );
        req->iosb = async_get_iosb( async );
        req->sock = (struct sock *)grab_object( sock );
        req->acceptsock = acceptsock;
        if (acceptsock) grab_object( acceptsock );
        req->accepted = 0;
        req->recv_len = 0;
        req->local_len = 0;
        if (params)
        {
            req->recv_len = params->recv_len;
            req->local_len = params->local_len;
        }
    }
    return req;
}

static void sock_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct sock *sock = get_fd_user( fd );
    int unix_fd;

    assert( sock->obj.ops == &sock_ops );

    if (code != IOCTL_AFD_WINE_CREATE && (unix_fd = get_unix_fd( fd )) < 0) return;

    switch(code)
    {
    case IOCTL_AFD_WINE_CREATE:
    {
        const struct afd_create_params *params = get_req_data();

        if (get_req_data_size() != sizeof(*params))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
        init_socket( sock, params->family, params->type, params->protocol, params->flags );
        return;
    }

    case IOCTL_AFD_WINE_ACCEPT:
    {
        struct sock *acceptsock;
        obj_handle_t handle;

        if (get_reply_max_size() != sizeof(handle))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        if (!(acceptsock = accept_socket( sock )))
        {
            struct accept_req *req;

            if (sock->nonblocking) return;
            if (get_error() != STATUS_DEVICE_NOT_READY) return;

            if (!(req = alloc_accept_req( sock, NULL, async, NULL ))) return;
            list_add_tail( &sock->accept_list, &req->entry );

            async_set_completion_callback( async, free_accept_req, req );
            queue_async( &sock->accept_q, async );
            sock_reselect( sock );
            set_error( STATUS_PENDING );
            return;
        }
        handle = alloc_handle( current->process, &acceptsock->obj,
                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, OBJ_INHERIT );
        acceptsock->wparam = handle;
        sock_reselect( acceptsock );
        release_object( acceptsock );
        set_reply_data( &handle, sizeof(handle) );
        return;
    }

    case IOCTL_AFD_WINE_ACCEPT_INTO:
    {
        static const int access = FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | FILE_READ_DATA;
        const struct afd_accept_into_params *params = get_req_data();
        struct sock *acceptsock;
        unsigned int remote_len;
        struct accept_req *req;

        if (get_req_data_size() != sizeof(*params) ||
            get_reply_max_size() < params->recv_len ||
            get_reply_max_size() - params->recv_len < params->local_len)
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        remote_len = get_reply_max_size() - params->recv_len - params->local_len;
        if (remote_len < sizeof(int))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (!(acceptsock = (struct sock *)get_handle_obj( current->process, params->accept_handle, access, &sock_ops )))
            return;

        if (acceptsock->accept_recv_req)
        {
            release_object( acceptsock );
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (!(req = alloc_accept_req( sock, acceptsock, async, params )))
        {
            release_object( acceptsock );
            return;
        }
        list_add_tail( &sock->accept_list, &req->entry );
        acceptsock->accept_recv_req = req;
        release_object( acceptsock );

        acceptsock->wparam = params->accept_handle;
        async_set_completion_callback( async, free_accept_req, req );
        queue_async( &sock->accept_q, async );
        sock_reselect( sock );
        set_error( STATUS_PENDING );
        return;
    }

    case IOCTL_AFD_LISTEN:
    {
        const struct afd_listen_params *params = get_req_data();

        if (get_req_data_size() < sizeof(*params))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (!sock->bound)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (listen( unix_fd, params->backlog ) < 0)
        {
            set_error( sock_get_ntstatus( errno ) );
            return;
        }

        sock->state = SOCK_LISTENING;

        /* a listening socket can no longer be accepted into */
        allow_fd_caching( sock->fd );

        /* we may already be selecting for AFD_POLL_ACCEPT */
        sock_reselect( sock );
        return;
    }

    case IOCTL_AFD_WINE_CONNECT:
    {
        const struct afd_connect_params *params = get_req_data();
        const struct WS_sockaddr *addr;
        union unix_sockaddr unix_addr;
        struct connect_req *req;
        socklen_t unix_len;
        int send_len, ret;

        if (get_req_data_size() < sizeof(*params) ||
            get_req_data_size() - sizeof(*params) < params->addr_len)
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        send_len = get_req_data_size() - sizeof(*params) - params->addr_len;
        addr = (const struct WS_sockaddr *)(params + 1);

        if (!params->synchronous && !sock->bound)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (sock->accept_recv_req)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (sock->connect_req)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        switch (sock->state)
        {
            case SOCK_LISTENING:
                set_error( STATUS_INVALID_PARAMETER );
                return;

            case SOCK_CONNECTING:
                /* FIXME: STATUS_ADDRESS_ALREADY_ASSOCIATED probably isn't right,
                 * but there's no status code that maps to WSAEALREADY... */
                set_error( params->synchronous ? STATUS_ADDRESS_ALREADY_ASSOCIATED : STATUS_INVALID_PARAMETER );
                return;

            case SOCK_CONNECTED:
                set_error( STATUS_CONNECTION_ACTIVE );
                return;

            case SOCK_UNCONNECTED:
            case SOCK_CONNECTIONLESS:
                break;
        }

        unix_len = sockaddr_to_unix( addr, params->addr_len, &unix_addr );
        if (!unix_len)
        {
            set_error( STATUS_INVALID_ADDRESS );
            return;
        }
        if (unix_addr.addr.sa_family == AF_INET && !memcmp( &unix_addr.in.sin_addr, magic_loopback_addr, 4 ))
            unix_addr.in.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

        ret = connect( unix_fd, &unix_addr.addr, unix_len );
        if (ret < 0 && errno != EINPROGRESS)
        {
            set_error( sock_get_ntstatus( errno ) );
            return;
        }

        /* a connected or connecting socket can no longer be accepted into */
        allow_fd_caching( sock->fd );

        unix_len = sizeof(unix_addr);
        if (!getsockname( unix_fd, &unix_addr.addr, &unix_len ))
            sock->addr_len = sockaddr_from_unix( &unix_addr, &sock->addr.addr, sizeof(sock->addr) );
        sock->bound = 1;

        if (!ret)
        {
            sock->state = SOCK_CONNECTED;

            if (!send_len) return;
        }

        sock->state = SOCK_CONNECTING;

        if (params->synchronous && sock->nonblocking)
        {
            sock_reselect( sock );
            set_error( STATUS_DEVICE_NOT_READY );
            return;
        }

        if (!(req = mem_alloc( sizeof(*req) )))
            return;

        req->async = (struct async *)grab_object( async );
        req->iosb = async_get_iosb( async );
        req->sock = (struct sock *)grab_object( sock );
        req->addr_len = params->addr_len;
        req->send_len = send_len;
        req->send_cursor = 0;

        async_set_completion_callback( async, free_connect_req, req );
        sock->connect_req = req;
        queue_async( &sock->connect_q, async );
        sock_reselect( sock );
        set_error( STATUS_PENDING );
        return;
    }

    case IOCTL_AFD_WINE_SHUTDOWN:
    {
        unsigned int how;

        if (get_req_data_size() < sizeof(int))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        how = *(int *)get_req_data();

        if (how > SD_BOTH)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (sock->state != SOCK_CONNECTED && sock->state != SOCK_CONNECTIONLESS)
        {
            set_error( STATUS_INVALID_CONNECTION );
            return;
        }

        if (how != SD_SEND)
        {
            sock->rd_shutdown = 1;
        }
        if (how != SD_RECEIVE)
        {
            sock->wr_shutdown = 1;
            if (list_empty( &sock->write_q.queue ))
                shutdown( unix_fd, SHUT_WR );
            else
                sock->wr_shutdown_pending = 1;
        }

        if (how == SD_BOTH)
        {
            if (sock->event) release_object( sock->event );
            sock->event = NULL;
            sock->window = 0;
            sock->mask = 0;
            sock->nonblocking = 1;
        }

        sock_reselect( sock );
        return;
    }

    case IOCTL_AFD_WINE_ADDRESS_LIST_CHANGE:
    {
        int force_async;

        if (get_req_data_size() < sizeof(int))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        force_async = *(int *)get_req_data();

        if (sock->nonblocking && !force_async)
        {
            set_error( STATUS_DEVICE_NOT_READY );
            return;
        }
        if (!sock_get_ifchange( sock )) return;
        queue_async( &sock->ifchange_q, async );
        set_error( STATUS_PENDING );
        return;
    }

    case IOCTL_AFD_WINE_FIONBIO:
        if (get_req_data_size() < sizeof(int))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        if (*(int *)get_req_data())
        {
            sock->nonblocking = 1;
        }
        else
        {
            if (sock->mask)
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }
            sock->nonblocking = 0;
        }
        return;

    case IOCTL_AFD_GET_EVENTS:
    {
        struct afd_get_events_params params = {0};
        unsigned int i;

        if (get_reply_max_size() < sizeof(params))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        params.flags = sock->pending_events & sock->mask;
        for (i = 0; i < ARRAY_SIZE( params.status ); ++i)
            params.status[i] = sock_get_ntstatus( sock->errors[i] );

        sock->pending_events = 0;
        sock_reselect( sock );

        set_reply_data( &params, sizeof(params) );
        return;
    }

    case IOCTL_AFD_EVENT_SELECT:
    {
        struct event *event = NULL;
        obj_handle_t event_handle;
        int mask;

        set_async_pending( async );

        if (is_machine_64bit( current->process->machine ))
        {
            const struct afd_event_select_params_64 *params = get_req_data();

            if (get_req_data_size() < sizeof(*params))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }

            event_handle = params->event;
            mask = params->mask;
        }
        else
        {
            const struct afd_event_select_params_32 *params = get_req_data();

            if (get_req_data_size() < sizeof(*params))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }

            event_handle = params->event;
            mask = params->mask;
        }

        if ((event_handle || mask) &&
            !(event = get_event_obj( current->process, event_handle, EVENT_MODIFY_STATE )))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (sock->event) release_object( sock->event );
        sock->event = event;
        sock->mask = mask;
        sock->window = 0;
        sock->message = 0;
        sock->wparam = 0;
        sock->nonblocking = 1;

        sock_reselect( sock );
        /* Explicitly wake the socket up if the mask causes it to become
         * signaled. Note that reselecting isn't enough, since we might already
         * have had events recorded in sock->reported_events and we don't want
         * to select for them again. */
        sock_wake_up( sock );

        return;
    }

    case IOCTL_AFD_WINE_MESSAGE_SELECT:
    {
        const struct afd_message_select_params *params = get_req_data();

        if (get_req_data_size() < sizeof(params))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        if (sock->event) release_object( sock->event );

        if (params->window)
        {
            sock->pending_events = 0;
            sock->reported_events = 0;
        }
        sock->event = NULL;
        sock->mask = params->mask;
        sock->window = params->window;
        sock->message = params->message;
        sock->wparam = params->handle;
        sock->nonblocking = 1;

        sock_reselect( sock );

        return;
    }

    case IOCTL_AFD_BIND:
    {
        const struct afd_bind_params *params = get_req_data();
        union unix_sockaddr unix_addr, bind_addr;
        data_size_t in_size;
        socklen_t unix_len;

        /* the ioctl is METHOD_NEITHER, so ntdll gives us the output buffer as
         * input */
        if (get_req_data_size() < get_reply_max_size())
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        in_size = get_req_data_size() - get_reply_max_size();
        if (in_size < offsetof(struct afd_bind_params, addr.sa_data)
                || get_reply_max_size() < in_size - sizeof(int))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (sock->bound)
        {
            set_error( STATUS_ADDRESS_ALREADY_ASSOCIATED );
            return;
        }

        unix_len = sockaddr_to_unix( &params->addr, in_size - sizeof(int), &unix_addr );
        if (!unix_len)
        {
            set_error( STATUS_INVALID_ADDRESS );
            return;
        }
        bind_addr = unix_addr;

        if (unix_addr.addr.sa_family == AF_INET)
        {
            if (!memcmp( &unix_addr.in.sin_addr, magic_loopback_addr, 4 )
                    || bind_to_interface( sock, &unix_addr.in ))
                bind_addr.in.sin_addr.s_addr = htonl( INADDR_ANY );
        }
        else if (unix_addr.addr.sa_family == AF_INET6)
        {
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
            /* Windows allows specifying zero to use the default scope. Linux
             * interprets it as an interface index and requires that it be
             * nonzero. */
            if (!unix_addr.in6.sin6_scope_id)
                bind_addr.in6.sin6_scope_id = get_ipv6_interface_index( &unix_addr.in6.sin6_addr );
#endif
        }

        set_async_pending( async );

        if (bind( unix_fd, &bind_addr.addr, unix_len ) < 0)
        {
            if (errno == EADDRINUSE)
            {
                int reuse;
                socklen_t len = sizeof(reuse);

                if (!getsockopt( unix_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, &len ) && reuse)
                    errno = EACCES;
            }

            set_error( sock_get_ntstatus( errno ) );
            return;
        }

        sock->bound = 1;

        unix_len = sizeof(bind_addr);
        if (!getsockname( unix_fd, &bind_addr.addr, &unix_len ))
        {
            /* store the interface or magic loopback address instead of the
             * actual unix address */
            if (bind_addr.addr.sa_family == AF_INET)
                bind_addr.in.sin_addr = unix_addr.in.sin_addr;
            sock->addr_len = sockaddr_from_unix( &bind_addr, &sock->addr.addr, sizeof(sock->addr) );
        }

        if (get_reply_max_size() >= sock->addr_len)
            set_reply_data( &sock->addr, sock->addr_len );
        return;
    }

    case IOCTL_AFD_GETSOCKNAME:
        if (!sock->bound)
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (get_reply_max_size() < sock->addr_len)
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        set_reply_data( &sock->addr, sock->addr_len );
        return;

    case IOCTL_AFD_WINE_DEFER:
    {
        const obj_handle_t *handle = get_req_data();
        struct sock *acceptsock;

        if (get_req_data_size() < sizeof(*handle))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        acceptsock = (struct sock *)get_handle_obj( current->process, *handle, 0, &sock_ops );
        if (!acceptsock) return;

        sock->deferred = acceptsock;
        return;
    }

    case IOCTL_AFD_WINE_GET_INFO:
    {
        struct afd_get_info_params params;

        if (get_reply_max_size() < sizeof(params))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        params.family = sock->family;
        params.type = sock->type;
        params.protocol = sock->proto;
        set_reply_data( &params, sizeof(params) );
        return;
    }

    case IOCTL_AFD_WINE_GET_SO_ACCEPTCONN:
    {
        int listening = (sock->state == SOCK_LISTENING);

        if (get_reply_max_size() < sizeof(listening))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        set_reply_data( &listening, sizeof(listening) );
        return;
    }

    case IOCTL_AFD_WINE_GET_SO_ERROR:
    {
        int error;
        socklen_t len = sizeof(error);
        unsigned int i;

        if (get_reply_max_size() < sizeof(error))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        if (getsockopt( unix_fd, SOL_SOCKET, SO_ERROR, (char *)&error, &len ) < 0)
        {
            set_error( sock_get_ntstatus( errno ) );
            return;
        }

        if (!error)
        {
            for (i = 0; i < ARRAY_SIZE( sock->errors ); ++i)
            {
                if (sock->errors[i])
                {
                    error = sock_get_error( sock->errors[i] );
                    break;
                }
            }
        }

        set_reply_data( &error, sizeof(error) );
        return;
    }

    case IOCTL_AFD_WINE_GET_SO_RCVBUF:
    {
        int rcvbuf = sock->rcvbuf;

        if (get_reply_max_size() < sizeof(rcvbuf))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        set_reply_data( &rcvbuf, sizeof(rcvbuf) );
        return;
    }

    case IOCTL_AFD_WINE_SET_SO_RCVBUF:
    {
        DWORD rcvbuf;

        if (get_req_data_size() < sizeof(rcvbuf))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        rcvbuf = *(DWORD *)get_req_data();

        if (!setsockopt( unix_fd, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, sizeof(rcvbuf) ))
            sock->rcvbuf = rcvbuf;
        else
            set_error( sock_get_ntstatus( errno ) );
        return;
    }

    case IOCTL_AFD_WINE_GET_SO_RCVTIMEO:
    {
        DWORD rcvtimeo = sock->rcvtimeo;

        if (get_reply_max_size() < sizeof(rcvtimeo))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        set_reply_data( &rcvtimeo, sizeof(rcvtimeo) );
        return;
    }

    case IOCTL_AFD_WINE_SET_SO_RCVTIMEO:
    {
        DWORD rcvtimeo;

        if (get_req_data_size() < sizeof(rcvtimeo))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        rcvtimeo = *(DWORD *)get_req_data();

        sock->rcvtimeo = rcvtimeo;
        return;
    }

    case IOCTL_AFD_WINE_GET_SO_SNDBUF:
    {
        int sndbuf = sock->sndbuf;

        if (get_reply_max_size() < sizeof(sndbuf))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        set_reply_data( &sndbuf, sizeof(sndbuf) );
        return;
    }

    case IOCTL_AFD_WINE_SET_SO_SNDBUF:
    {
        DWORD sndbuf;

        if (get_req_data_size() < sizeof(sndbuf))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        sndbuf = *(DWORD *)get_req_data();

#ifdef __APPLE__
        if (!sndbuf)
        {
            /* setsockopt fails if a zero value is passed */
            sock->sndbuf = sndbuf;
            return;
        }
#endif

        if (!setsockopt( unix_fd, SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, sizeof(sndbuf) ))
            sock->sndbuf = sndbuf;
        else
            set_error( sock_get_ntstatus( errno ) );
        return;
    }

    case IOCTL_AFD_WINE_GET_SO_SNDTIMEO:
    {
        DWORD sndtimeo = sock->sndtimeo;

        if (get_reply_max_size() < sizeof(sndtimeo))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        set_reply_data( &sndtimeo, sizeof(sndtimeo) );
        return;
    }

    case IOCTL_AFD_WINE_SET_SO_SNDTIMEO:
    {
        DWORD sndtimeo;

        if (get_req_data_size() < sizeof(sndtimeo))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }
        sndtimeo = *(DWORD *)get_req_data();

        sock->sndtimeo = sndtimeo;
        return;
    }

    case IOCTL_AFD_WINE_GET_SO_CONNECT_TIME:
    {
        DWORD time = ~0u;

        if (get_reply_max_size() < sizeof(time))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return;
        }

        if (sock->state == SOCK_CONNECTED)
            time = (current_time - sock->connect_time) / 10000000;

        set_reply_data( &time, sizeof(time) );
        return;
    }

    case IOCTL_AFD_POLL:
    {
        if (get_reply_max_size() < get_req_data_size())
        {
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        if (is_machine_64bit( current->process->machine ))
        {
            const struct afd_poll_params_64 *params = get_req_data();

            if (get_req_data_size() < sizeof(struct afd_poll_params_64) ||
                get_req_data_size() < offsetof( struct afd_poll_params_64, sockets[params->count] ))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }

            poll_socket( sock, async, params->exclusive, params->timeout, params->count, params->sockets );
        }
        else
        {
            const struct afd_poll_params_32 *params = get_req_data();
            struct afd_poll_socket_64 *sockets;
            unsigned int i;

            if (get_req_data_size() < sizeof(struct afd_poll_params_32) ||
                get_req_data_size() < offsetof( struct afd_poll_params_32, sockets[params->count] ))
            {
                set_error( STATUS_INVALID_PARAMETER );
                return;
            }

            if (!(sockets = mem_alloc( params->count * sizeof(*sockets) ))) return;
            for (i = 0; i < params->count; ++i)
            {
                sockets[i].socket = params->sockets[i].socket;
                sockets[i].flags = params->sockets[i].flags;
                sockets[i].status = params->sockets[i].status;
            }

            poll_socket( sock, async, params->exclusive, params->timeout, params->count, sockets );
            free( sockets );
        }

        return;
    }

    default:
        set_error( STATUS_NOT_SUPPORTED );
        return;
    }
}

static int poll_single_socket( struct sock *sock, int mask )
{
    struct pollfd pollfd;

    pollfd.fd = get_unix_fd( sock->fd );
    pollfd.events = poll_flags_from_afd( sock, mask );
    if (pollfd.events < 0 || poll( &pollfd, 1, 0 ) < 0)
        return 0;

    if (sock->state == SOCK_CONNECTING && (pollfd.revents & (POLLERR | POLLHUP)))
        pollfd.revents &= ~POLLOUT;

    if ((mask & AFD_POLL_HUP) && (pollfd.revents & POLLIN) && sock->type == WS_SOCK_STREAM)
    {
        char dummy;

        if (!recv( get_unix_fd( sock->fd ), &dummy, 1, MSG_PEEK ))
        {
            pollfd.revents &= ~POLLIN;
            pollfd.revents |= POLLHUP;
        }
    }

    return get_poll_flags( sock, pollfd.revents ) & mask;
}

static void handle_exclusive_poll(struct poll_req *req)
{
    unsigned int i;

    for (i = 0; i < req->count; ++i)
    {
        struct sock *sock = req->sockets[i].sock;
        struct poll_req *main_poll = sock->main_poll;

        if (main_poll && main_poll->exclusive && req->exclusive)
        {
            complete_async_poll( main_poll, STATUS_SUCCESS );
            main_poll = NULL;
        }

        if (!main_poll)
            sock->main_poll = req;
    }
}

static void poll_socket( struct sock *poll_sock, struct async *async, int exclusive, timeout_t timeout,
                         unsigned int count, const struct afd_poll_socket_64 *sockets )
{
    BOOL signaled = FALSE;
    struct poll_req *req;
    unsigned int i, j;

    if (!count)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!(req = mem_alloc( offsetof( struct poll_req, sockets[count] ) )))
        return;

    req->timeout = NULL;
    if (timeout && timeout != TIMEOUT_INFINITE &&
        !(req->timeout = add_timeout_user( timeout, async_poll_timeout, req )))
    {
        free( req );
        return;
    }
    req->orig_timeout = timeout;

    for (i = 0; i < count; ++i)
    {
        req->sockets[i].sock = (struct sock *)get_handle_obj( current->process, sockets[i].socket, 0, &sock_ops );
        if (!req->sockets[i].sock)
        {
            for (j = 0; j < i; ++j) release_object( req->sockets[j].sock );
            if (req->timeout) remove_timeout_user( req->timeout );
            free( req );
            return;
        }
        req->sockets[i].handle = sockets[i].socket;
        req->sockets[i].mask = sockets[i].flags;
        req->sockets[i].flags = 0;
    }

    req->exclusive = exclusive;
    req->count = count;
    req->async = (struct async *)grab_object( async );
    req->iosb = async_get_iosb( async );

    handle_exclusive_poll(req);

    list_add_tail( &poll_list, &req->entry );
    async_set_completion_callback( async, free_poll_req, req );
    queue_async( &poll_sock->poll_q, async );

    for (i = 0; i < count; ++i)
    {
        struct sock *sock = req->sockets[i].sock;
        int mask = req->sockets[i].mask;
        int flags = poll_single_socket( sock, mask );

        if (flags)
        {
            signaled = TRUE;
            req->sockets[i].flags = flags;
            req->sockets[i].status = sock_get_ntstatus( sock_error( sock->fd ) );
        }

        /* FIXME: do other error conditions deserve a similar treatment? */
        if (sock->state != SOCK_CONNECTING && sock->errors[AFD_POLL_BIT_CONNECT_ERR] && (mask & AFD_POLL_CONNECT_ERR))
        {
            signaled = TRUE;
            req->sockets[i].flags |= AFD_POLL_CONNECT_ERR;
            req->sockets[i].status = sock_get_ntstatus( sock->errors[AFD_POLL_BIT_CONNECT_ERR] );
        }
    }

    if (!timeout || signaled)
        complete_async_poll( req, STATUS_SUCCESS );

    for (i = 0; i < req->count; ++i)
        sock_reselect( req->sockets[i].sock );
    set_error( STATUS_PENDING );
}

#ifdef HAVE_LINUX_RTNETLINK_H

/* only keep one ifchange object around, all sockets waiting for wakeups will look to it */
static struct object *ifchange_object;

static void ifchange_dump( struct object *obj, int verbose );
static struct fd *ifchange_get_fd( struct object *obj );
static void ifchange_destroy( struct object *obj );

static int ifchange_get_poll_events( struct fd *fd );
static void ifchange_poll_event( struct fd *fd, int event );

struct ifchange
{
    struct object       obj;     /* object header */
    struct fd          *fd;      /* interface change file descriptor */
    struct list         sockets; /* list of sockets to send interface change notifications */
};

static const struct object_ops ifchange_ops =
{
    sizeof(struct ifchange), /* size */
    &no_type,                /* type */
    ifchange_dump,           /* dump */
    no_add_queue,            /* add_queue */
    NULL,                    /* remove_queue */
    NULL,                    /* signaled */
    no_satisfied,            /* satisfied */
    no_signal,               /* signal */
    ifchange_get_fd,         /* get_fd */
    default_map_access,      /* map_access */
    default_get_sd,          /* get_sd */
    default_set_sd,          /* set_sd */
    no_get_full_name,        /* get_full_name */
    no_lookup_name,          /* lookup_name */
    no_link_name,            /* link_name */
    NULL,                    /* unlink_name */
    no_open_file,            /* open_file */
    no_kernel_obj_list,      /* get_kernel_obj_list */
    no_close_handle,         /* close_handle */
    ifchange_destroy         /* destroy */
};

static const struct fd_ops ifchange_fd_ops =
{
    ifchange_get_poll_events, /* get_poll_events */
    ifchange_poll_event,      /* poll_event */
    NULL,                     /* get_fd_type */
    no_fd_read,               /* read */
    no_fd_write,              /* write */
    no_fd_flush,              /* flush */
    no_fd_get_file_info,      /* get_file_info */
    no_fd_get_volume_info,    /* get_volume_info */
    no_fd_ioctl,              /* ioctl */
    NULL,                     /* cancel_async */
    NULL,                     /* queue_async */
    NULL                      /* reselect_async */
};

static void ifchange_dump( struct object *obj, int verbose )
{
    assert( obj->ops == &ifchange_ops );
    fprintf( stderr, "Interface change\n" );
}

static struct fd *ifchange_get_fd( struct object *obj )
{
    struct ifchange *ifchange = (struct ifchange *)obj;
    return (struct fd *)grab_object( ifchange->fd );
}

static void ifchange_destroy( struct object *obj )
{
    struct ifchange *ifchange = (struct ifchange *)obj;
    assert( obj->ops == &ifchange_ops );

    release_object( ifchange->fd );

    /* reset the global ifchange object so that it will be recreated if it is needed again */
    assert( obj == ifchange_object );
    ifchange_object = NULL;
}

static int ifchange_get_poll_events( struct fd *fd )
{
    return POLLIN;
}

/* wake up all the sockets waiting for a change notification event */
static void ifchange_wake_up( struct object *obj, unsigned int status )
{
    struct ifchange *ifchange = (struct ifchange *)obj;
    struct list *ptr, *next;
    assert( obj->ops == &ifchange_ops );
    assert( obj == ifchange_object );

    LIST_FOR_EACH_SAFE( ptr, next, &ifchange->sockets )
    {
        struct sock *sock = LIST_ENTRY( ptr, struct sock, ifchange_entry );

        assert( sock->ifchange_obj );
        async_wake_up( &sock->ifchange_q, status ); /* issue ifchange notification for the socket */
        sock_release_ifchange( sock ); /* remove socket from list and decrement ifchange refcount */
    }
}

static void ifchange_poll_event( struct fd *fd, int event )
{
    struct object *ifchange = get_fd_user( fd );
    unsigned int status = STATUS_PENDING;
    char buffer[PIPE_BUF];
    int r;

    r = recv( get_unix_fd(fd), buffer, sizeof(buffer), MSG_DONTWAIT );
    if (r < 0)
    {
        if (errno == EWOULDBLOCK || (EWOULDBLOCK != EAGAIN && errno == EAGAIN))
            return;  /* retry when poll() says the socket is ready */
        status = sock_get_ntstatus( errno );
    }
    else if (r > 0)
    {
        struct nlmsghdr *nlh;

        for (nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, r); nlh = NLMSG_NEXT(nlh, r))
        {
            if (nlh->nlmsg_type == NLMSG_DONE)
                break;
            if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR)
                status = STATUS_SUCCESS;
        }
    }
    else status = STATUS_CANCELLED;

    if (status != STATUS_PENDING) ifchange_wake_up( ifchange, status );
}

#endif

/* we only need one of these interface notification objects, all of the sockets dependent upon
 * it will wake up when a notification event occurs */
 static struct object *get_ifchange( void )
 {
#ifdef HAVE_LINUX_RTNETLINK_H
    struct ifchange *ifchange;
    struct sockaddr_nl addr;
    int unix_fd;

    if (ifchange_object)
    {
        /* increment the refcount for each socket that uses the ifchange object */
        return grab_object( ifchange_object );
    }

    /* create the socket we need for processing interface change notifications */
    unix_fd = socket( PF_NETLINK, SOCK_RAW, NETLINK_ROUTE );
    if (unix_fd == -1)
    {
        set_error( sock_get_ntstatus( errno ));
        return NULL;
    }
    fcntl( unix_fd, F_SETFL, O_NONBLOCK ); /* make socket nonblocking */
    memset( &addr, 0, sizeof(addr) );
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR;
    /* bind the socket to the special netlink kernel interface */
    if (bind( unix_fd, (struct sockaddr *)&addr, sizeof(addr) ) == -1)
    {
        close( unix_fd );
        set_error( sock_get_ntstatus( errno ));
        return NULL;
    }
    if (!(ifchange = alloc_object( &ifchange_ops )))
    {
        close( unix_fd );
        set_error( STATUS_NO_MEMORY );
        return NULL;
    }
    list_init( &ifchange->sockets );
    if (!(ifchange->fd = create_anonymous_fd( &ifchange_fd_ops, unix_fd, &ifchange->obj, 0 )))
    {
        release_object( ifchange );
        set_error( STATUS_NO_MEMORY );
        return NULL;
    }
    set_fd_events( ifchange->fd, POLLIN ); /* enable read wakeup on the file descriptor */

    /* the ifchange object is now successfully configured */
    ifchange_object = &ifchange->obj;
    return &ifchange->obj;
#else
    set_error( STATUS_NOT_SUPPORTED );
    return NULL;
#endif
}

/* add the socket to the interface change notification list */
static void ifchange_add_sock( struct object *obj, struct sock *sock )
{
#ifdef HAVE_LINUX_RTNETLINK_H
    struct ifchange *ifchange = (struct ifchange *)obj;

    list_add_tail( &ifchange->sockets, &sock->ifchange_entry );
#endif
}

/* create a new ifchange queue for a specific socket or, if one already exists, reuse the existing one */
static struct object *sock_get_ifchange( struct sock *sock )
{
    struct object *ifchange;

    if (sock->ifchange_obj) /* reuse existing ifchange_obj for this socket */
        return sock->ifchange_obj;

    if (!(ifchange = get_ifchange()))
        return NULL;

    /* add the socket to the ifchange notification list */
    ifchange_add_sock( ifchange, sock );
    sock->ifchange_obj = ifchange;
    return ifchange;
}

/* destroy an existing ifchange queue for a specific socket */
static void sock_release_ifchange( struct sock *sock )
{
    if (sock->ifchange_obj)
    {
        list_remove( &sock->ifchange_entry );
        release_object( sock->ifchange_obj );
        sock->ifchange_obj = NULL;
    }
}

static void socket_device_dump( struct object *obj, int verbose );
static struct object *socket_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                 unsigned int attr, struct object *root );
static struct object *socket_device_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );

static const struct object_ops socket_device_ops =
{
    sizeof(struct object),      /* size */
    &device_type,               /* type */
    socket_device_dump,         /* dump */
    no_add_queue,               /* add_queue */
    NULL,                       /* remove_queue */
    NULL,                       /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    default_map_access,         /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    default_get_full_name,      /* get_full_name */
    socket_device_lookup_name,  /* lookup_name */
    directory_link_name,        /* link_name */
    default_unlink_name,        /* unlink_name */
    socket_device_open_file,    /* open_file */
    no_kernel_obj_list,         /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    no_destroy                  /* destroy */
};

static void socket_device_dump( struct object *obj, int verbose )
{
    fputs( "Socket device\n", stderr );
}

static struct object *socket_device_lookup_name( struct object *obj, struct unicode_str *name,
                                                 unsigned int attr, struct object *root )
{
    if (name) name->len = 0;
    return NULL;
}

static struct object *socket_device_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options )
{
    struct sock *sock;

    if (!(sock = create_socket())) return NULL;
    if (!(sock->fd = alloc_pseudo_fd( &sock_fd_ops, &sock->obj, options )))
    {
        release_object( sock );
        return NULL;
    }
    return &sock->obj;
}

struct object *create_socket_device( struct object *root, const struct unicode_str *name,
                                     unsigned int attr, const struct security_descriptor *sd )
{
    return create_named_object( root, &socket_device_ops, name, attr, sd );
}

DECL_HANDLER(recv_socket)
{
    struct sock *sock = (struct sock *)get_handle_obj( current->process, req->async.handle, 0, &sock_ops );
    unsigned int status = STATUS_PENDING;
    timeout_t timeout = 0;
    struct async *async;
    struct fd *fd;

    if (!sock) return;
    fd = sock->fd;

    if (!req->force_async && !sock->nonblocking && is_fd_overlapped( fd ))
        timeout = (timeout_t)sock->rcvtimeo * -10000;

    if (sock->rd_shutdown) status = STATUS_PIPE_DISCONNECTED;
    else if (!async_queued( &sock->read_q ))
    {
        /* If read_q is not empty, we cannot really tell if the already queued
         * asyncs will not consume all available data; if there's no data
         * available, the current request won't be immediately satiable.
         */
        struct pollfd pollfd;
        pollfd.fd = get_unix_fd( sock->fd );
        pollfd.events = req->oob ? POLLPRI : POLLIN;
        pollfd.revents = 0;
        if (poll(&pollfd, 1, 0) >= 0 && pollfd.revents)
        {
            /* Give the client opportunity to complete synchronously.
             * If it turns out that the I/O request is not actually immediately satiable,
             * the client may then choose to re-queue the async (with STATUS_PENDING). */
            status = STATUS_ALERTED;
        }
    }

    if (status == STATUS_PENDING && !req->force_async && sock->nonblocking)
        status = STATUS_DEVICE_NOT_READY;

    sock->pending_events &= ~(req->oob ? AFD_POLL_OOB : AFD_POLL_READ);
    sock->reported_events &= ~(req->oob ? AFD_POLL_OOB : AFD_POLL_READ);

    if ((async = create_request_async( fd, get_fd_comp_flags( fd ), &req->async )))
    {
        set_error( status );

        if (timeout)
            async_set_timeout( async, timeout, STATUS_IO_TIMEOUT );

        if (status == STATUS_PENDING || status == STATUS_ALERTED)
            queue_async( &sock->read_q, async );

        /* always reselect; we changed reported_events above */
        sock_reselect( sock );

        reply->wait = async_handoff( async, NULL, 0 );
        reply->options = get_fd_options( fd );
        reply->nonblocking = sock->nonblocking;
        release_object( async );
    }
    release_object( sock );
}

DECL_HANDLER(send_socket)
{
    struct sock *sock = (struct sock *)get_handle_obj( current->process, req->async.handle, 0, &sock_ops );
    unsigned int status = req->status;
    timeout_t timeout = 0;
    struct async *async;
    struct fd *fd;

    if (!sock) return;
    fd = sock->fd;

    if (sock->type == WS_SOCK_DGRAM)
    {
        /* sendto() and sendmsg() implicitly binds a socket */
        union unix_sockaddr unix_addr;
        socklen_t unix_len = sizeof(unix_addr);

        if (!sock->bound && !getsockname( get_unix_fd( fd ), &unix_addr.addr, &unix_len ))
            sock->addr_len = sockaddr_from_unix( &unix_addr, &sock->addr.addr, sizeof(sock->addr) );
        sock->bound = 1;
    }

    if (status != STATUS_SUCCESS)
    {
        /* send() calls only clear and reselect events if unsuccessful. */
        sock->pending_events &= ~AFD_POLL_WRITE;
        sock->reported_events &= ~AFD_POLL_WRITE;
    }

    /* If we had a short write and the socket is nonblocking (and the client is
     * not trying to force the operation to be asynchronous), return success.
     * Windows actually refuses to send any data in this case, and returns
     * EWOULDBLOCK, but we have no way of doing that. */
    if (status == STATUS_DEVICE_NOT_READY && req->total && sock->nonblocking)
        status = STATUS_SUCCESS;

    /* send() returned EWOULDBLOCK or a short write, i.e. cannot send all data yet */
    if (status == STATUS_DEVICE_NOT_READY && !sock->nonblocking)
    {
        /* Set a timeout on the async if necessary.
         *
         * We want to do this *only* if the client gave us STATUS_DEVICE_NOT_READY.
         * If the client gave us STATUS_PENDING, it expects the async to always
         * block (it was triggered by WSASend*() with a valid OVERLAPPED
         * structure) and for the timeout not to be respected. */
        if (is_fd_overlapped( fd ))
            timeout = (timeout_t)sock->sndtimeo * -10000;

        status = STATUS_PENDING;
    }

    if ((status == STATUS_PENDING || status == STATUS_DEVICE_NOT_READY) && sock->wr_shutdown)
        status = STATUS_PIPE_DISCONNECTED;

    if ((async = create_request_async( fd, get_fd_comp_flags( fd ), &req->async )))
    {
        if (status == STATUS_SUCCESS)
        {
            struct iosb *iosb = async_get_iosb( async );
            iosb->result = req->total;
            release_object( iosb );
        }
        set_error( status );

        if (timeout)
            async_set_timeout( async, timeout, STATUS_IO_TIMEOUT );

        if (status == STATUS_PENDING)
            queue_async( &sock->write_q, async );

        /* always reselect; we changed reported_events above */
        sock_reselect( sock );

        reply->wait = async_handoff( async, NULL, 0 );
        reply->options = get_fd_options( fd );
        release_object( async );
    }
    release_object( sock );
}
