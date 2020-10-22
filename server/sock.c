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
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#include <time.h>
#include <unistd.h>
#include <limits.h>
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

/* From winsock.h */
#define FD_MAX_EVENTS              10
#define FD_READ_BIT                0
#define FD_WRITE_BIT               1
#define FD_OOB_BIT                 2
#define FD_ACCEPT_BIT              3
#define FD_CONNECT_BIT             4
#define FD_CLOSE_BIT               5

/*
 * Define flags to be used with the WSAAsyncSelect() call.
 */
#define FD_READ                    0x00000001
#define FD_WRITE                   0x00000002
#define FD_OOB                     0x00000004
#define FD_ACCEPT                  0x00000008
#define FD_CONNECT                 0x00000010
#define FD_CLOSE                   0x00000020

/* internal per-socket flags */
#define FD_WINE_LISTENING          0x10000000
#define FD_WINE_NONBLOCKING        0x20000000
#define FD_WINE_CONNECTED          0x40000000
#define FD_WINE_RAW                0x80000000
#define FD_WINE_INTERNAL           0xFFFF0000

struct accept_req
{
    struct list entry;
    struct async *async;
    struct sock *acceptsock;
    int accepted;
    unsigned int recv_len, local_len;
};

struct sock
{
    struct object       obj;         /* object header */
    struct fd          *fd;          /* socket file descriptor */
    unsigned int        state;       /* status bits */
    unsigned int        mask;        /* event mask */
    unsigned int        hmask;       /* held (blocked) events */
    unsigned int        pmask;       /* pending events */
    unsigned int        flags;       /* socket flags */
    int                 polling;     /* is socket being polled? */
    unsigned short      proto;       /* socket protocol */
    unsigned short      type;        /* socket type */
    unsigned short      family;      /* socket family */
    struct event       *event;       /* event object */
    user_handle_t       window;      /* window to send the message to */
    unsigned int        message;     /* message to send */
    obj_handle_t        wparam;      /* message wparam (socket handle) */
    unsigned int        errors[FD_MAX_EVENTS]; /* event errors */
    timeout_t           connect_time;/* time the socket was connected */
    struct sock        *deferred;    /* socket that waits for a deferred accept */
    struct async_queue  read_q;      /* queue for asynchronous reads */
    struct async_queue  write_q;     /* queue for asynchronous writes */
    struct async_queue  ifchange_q;  /* queue for interface change notifications */
    struct async_queue  accept_q;    /* queue for asynchronous accepts */
    struct object      *ifchange_obj; /* the interface change notification object */
    struct list         ifchange_entry; /* entry in ifchange notification list */
    struct list         accept_list; /* list of pending accept requests */
    struct accept_req  *accept_recv_req; /* pending accept-into request which will recv on this socket */
};

static void sock_dump( struct object *obj, int verbose );
static int sock_signaled( struct object *obj, struct wait_queue_entry *entry );
static struct fd *sock_get_fd( struct object *obj );
static void sock_destroy( struct object *obj );
static struct object *sock_get_ifchange( struct sock *sock );
static void sock_release_ifchange( struct sock *sock );

static int sock_get_poll_events( struct fd *fd );
static void sock_poll_event( struct fd *fd, int event );
static enum server_fd_type sock_get_fd_type( struct fd *fd );
static int sock_ioctl( struct fd *fd, ioctl_code_t code, struct async *async );
static void sock_queue_async( struct fd *fd, struct async *async, int type, int count );
static void sock_reselect_async( struct fd *fd, struct async_queue *queue );

static int accept_into_socket( struct sock *sock, struct sock *acceptsock );
static struct sock *accept_socket( struct sock *sock );
static int sock_get_ntstatus( int err );
static unsigned int sock_get_error( int err );

static const struct object_ops sock_ops =
{
    sizeof(struct sock),          /* size */
    sock_dump,                    /* dump */
    no_get_type,                  /* get_type */
    add_queue,                    /* add_queue */
    remove_queue,                 /* remove_queue */
    sock_signaled,                /* signaled */
    no_satisfied,                 /* satisfied */
    no_signal,                    /* signal */
    sock_get_fd,                  /* get_fd */
    default_fd_map_access,        /* map_access */
    default_get_sd,               /* get_sd */
    default_set_sd,               /* set_sd */
    no_get_full_name,             /* get_full_name */
    no_lookup_name,               /* lookup_name */
    no_link_name,                 /* link_name */
    NULL,                         /* unlink_name */
    no_open_file,                 /* open_file */
    no_kernel_obj_list,           /* get_kernel_obj_list */
    fd_close_handle,              /* close_handle */
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
        return -1;

    }
}

/* Permutation of 0..FD_MAX_EVENTS - 1 representing the order in which
 * we post messages if there are multiple events.  Used to send
 * messages.  The problem is if there is both a FD_CONNECT event and,
 * say, an FD_READ event available on the same socket, we want to
 * notify the app of the connect event first.  Otherwise it may
 * discard the read event because it thinks it hasn't connected yet.
 */
static const int event_bitorder[FD_MAX_EVENTS] =
{
    FD_CONNECT_BIT,
    FD_ACCEPT_BIT,
    FD_OOB_BIT,
    FD_WRITE_BIT,
    FD_READ_BIT,
    FD_CLOSE_BIT,
    6, 7, 8, 9  /* leftovers */
};

/* Flags that make sense only for SOCK_STREAM sockets */
#define STREAM_FLAG_MASK ((unsigned int) (FD_CONNECT | FD_ACCEPT | FD_WINE_LISTENING | FD_WINE_CONNECTED))

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

    if (!sock->polling)  /* FIXME: should find a better way to do this */
    {
        /* previously unconnected socket, is this reselect supposed to connect it? */
        if (!(sock->state & ~FD_WINE_NONBLOCKING)) return 0;
        /* ok, it is, attach it to the wineserver's main poll loop */
        sock->polling = 1;
        allow_fd_caching( sock->fd );
    }
    /* update condition mask */
    set_fd_events( sock->fd, ev );
    return ev;
}

/* wake anybody waiting on the socket event or send the associated message */
static void sock_wake_up( struct sock *sock )
{
    unsigned int events = sock->pmask & sock->mask;
    int i;

    if ( !events ) return;

    if (sock->event)
    {
        if (debug_level) fprintf(stderr, "signalling events %x ptr %p\n", events, sock->event );
        set_event( sock->event );
    }
    if (sock->window)
    {
        if (debug_level) fprintf(stderr, "signalling events %x win %08x\n", events, sock->window );
        for (i = 0; i < FD_MAX_EVENTS; i++)
        {
            int event = event_bitorder[i];
            if (sock->pmask & (1 << event))
            {
                lparam_t lparam = (1 << event) | (sock->errors[event] << 16);
                post_message( sock->window, sock->message, sock->wparam, lparam );
            }
        }
        sock->pmask = 0;
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

static void free_accept_req( struct accept_req *req )
{
    list_remove( &req->entry );
    if (req->acceptsock) req->acceptsock->accept_recv_req = NULL;
    release_object( req->async );
    free( req );
}

static void fill_accept_output( struct accept_req *req, struct iosb *iosb )
{
    union unix_sockaddr unix_addr;
    struct WS_sockaddr *win_addr;
    socklen_t unix_len;
    int fd, size = 0;
    char *out_data;
    int win_len;

    if (!(out_data = mem_alloc( iosb->out_size ))) return;

    fd = get_unix_fd( req->acceptsock->fd );

    if (req->recv_len && (size = recv( fd, out_data, req->recv_len, 0 )) < 0)
    {
        if (!req->accepted && errno == EWOULDBLOCK)
        {
            req->accepted = 1;
            sock_reselect( req->acceptsock );
            set_error( STATUS_PENDING );
            return;
        }

        set_win32_error( sock_get_error( errno ) );
        free( out_data );
        return;
    }

    if (req->local_len)
    {
        if (req->local_len < sizeof(int))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            free( out_data );
            return;
        }

        unix_len = sizeof(unix_addr);
        win_addr = (struct WS_sockaddr *)(out_data + req->recv_len + sizeof(int));
        if (getsockname( fd, &unix_addr.addr, &unix_len ) < 0 ||
            (win_len = sockaddr_from_unix( &unix_addr, win_addr, req->local_len )) < 0)
        {
            set_win32_error( sock_get_error( errno ) );
            free( out_data );
            return;
        }
        memcpy( out_data + req->recv_len, &win_len, sizeof(int) );
    }

    unix_len = sizeof(unix_addr);
    win_addr = (struct WS_sockaddr *)(out_data + req->recv_len + req->local_len + sizeof(int));
    if (getpeername( fd, &unix_addr.addr, &unix_len ) < 0 ||
        (win_len = sockaddr_from_unix( &unix_addr, win_addr, iosb->out_size - req->recv_len - req->local_len )) < 0)
    {
        set_win32_error( sock_get_error( errno ) );
        free( out_data );
        return;
    }
    memcpy( out_data + req->recv_len + req->local_len, &win_len, sizeof(int) );

    iosb->status = STATUS_SUCCESS;
    iosb->result = size;
    iosb->out_data = out_data;
    set_error( STATUS_ALERTED );
}

static void complete_async_accept( struct sock *sock, struct accept_req *req )
{
    struct sock *acceptsock = req->acceptsock;
    struct async *async = req->async;
    struct iosb *iosb;

    if (debug_level) fprintf( stderr, "completing accept request for socket %p\n", sock );

    if (acceptsock)
    {
        if (!accept_into_socket( sock, acceptsock )) return;

        iosb = async_get_iosb( async );
        fill_accept_output( req, iosb );
        release_object( iosb );
    }
    else
    {
        obj_handle_t handle;

        if (!(acceptsock = accept_socket( sock ))) return;
        handle = alloc_handle_no_access_check( async_get_thread( async )->process, &acceptsock->obj,
                                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, OBJ_INHERIT );
        acceptsock->wparam = handle;
        release_object( acceptsock );
        if (!handle) return;

        iosb = async_get_iosb( async );
        if (!(iosb->out_data = malloc( sizeof(handle) )))
        {
            release_object( iosb );
            return;
        }
        iosb->status = STATUS_SUCCESS;
        iosb->out_size = sizeof(handle);
        memcpy( iosb->out_data, &handle, sizeof(handle) );
        release_object( iosb );
        set_error( STATUS_ALERTED );
    }
}

static void complete_async_accept_recv( struct accept_req *req )
{
    struct async *async = req->async;
    struct iosb *iosb;

    if (debug_level) fprintf( stderr, "completing accept recv request for socket %p\n", req->acceptsock );

    assert( req->recv_len );

    iosb = async_get_iosb( async );
    fill_accept_output( req, iosb );
    release_object( iosb );
}

static int sock_dispatch_asyncs( struct sock *sock, int event, int error )
{
    if (event & (POLLIN | POLLPRI))
    {
        struct accept_req *req;

        LIST_FOR_EACH_ENTRY( req, &sock->accept_list, struct accept_req, entry )
        {
            if (!req->accepted)
            {
                complete_async_accept( sock, req );
                if (get_error() != STATUS_PENDING)
                    async_terminate( req->async, get_error() );
                break;
            }
        }

        if (sock->accept_recv_req)
        {
            complete_async_accept_recv( sock->accept_recv_req );
            if (get_error() != STATUS_PENDING)
                async_terminate( sock->accept_recv_req->async, get_error() );
        }
    }

    if (is_fd_overlapped( sock->fd ))
    {
        if (event & (POLLIN|POLLPRI) && async_waiting( &sock->read_q ))
        {
            if (debug_level) fprintf( stderr, "activating read queue for socket %p\n", sock );
            async_wake_up( &sock->read_q, STATUS_ALERTED );
            event &= ~(POLLIN|POLLPRI);
        }
        if (event & POLLOUT && async_waiting( &sock->write_q ))
        {
            if (debug_level) fprintf( stderr, "activating write queue for socket %p\n", sock );
            async_wake_up( &sock->write_q, STATUS_ALERTED );
            event &= ~POLLOUT;
        }
    }

    if (event & (POLLERR | POLLHUP))
    {
        int status = sock_get_ntstatus( error );
        struct accept_req *req, *next;

        if (!(sock->state & FD_READ))
            async_wake_up( &sock->read_q, status );
        if (!(sock->state & FD_WRITE))
            async_wake_up( &sock->write_q, status );

        LIST_FOR_EACH_ENTRY_SAFE( req, next, &sock->accept_list, struct accept_req, entry )
            async_terminate( req->async, status );

        if (sock->accept_recv_req)
            async_terminate( sock->accept_recv_req->async, status );
    }

    return event;
}

static void sock_dispatch_events( struct sock *sock, int prevstate, int event, int error )
{
    if (prevstate & FD_CONNECT)
    {
        sock->pmask |= FD_CONNECT;
        sock->hmask |= FD_CONNECT;
        sock->errors[FD_CONNECT_BIT] = sock_get_error( error );
        goto end;
    }
    if (prevstate & FD_WINE_LISTENING)
    {
        sock->pmask |= FD_ACCEPT;
        sock->hmask |= FD_ACCEPT;
        sock->errors[FD_ACCEPT_BIT] = sock_get_error( error );
        goto end;
    }

    if (event & POLLIN)
    {
        sock->pmask |= FD_READ;
        sock->hmask |= FD_READ;
        sock->errors[FD_READ_BIT] = 0;
    }

    if (event & POLLOUT)
    {
        sock->pmask |= FD_WRITE;
        sock->hmask |= FD_WRITE;
        sock->errors[FD_WRITE_BIT] = 0;
    }

    if (event & POLLPRI)
    {
        sock->pmask |= FD_OOB;
        sock->hmask |= FD_OOB;
        sock->errors[FD_OOB_BIT] = 0;
    }

    if (event & (POLLERR|POLLHUP))
    {
        sock->pmask |= FD_CLOSE;
        sock->hmask |= FD_CLOSE;
        sock->errors[FD_CLOSE_BIT] = sock_get_error( error );
    }
end:
    sock_wake_up( sock );
}

static void sock_poll_event( struct fd *fd, int event )
{
    struct sock *sock = get_fd_user( fd );
    int hangup_seen = 0;
    int prevstate = sock->state;
    int error = 0;

    assert( sock->obj.ops == &sock_ops );
    if (debug_level)
        fprintf(stderr, "socket %p select event: %x\n", sock, event);

    /* we may change event later, remove from loop here */
    if (event & (POLLERR|POLLHUP)) set_fd_events( sock->fd, -1 );

    if (sock->state & FD_CONNECT)
    {
        if (event & (POLLERR|POLLHUP))
        {
            /* we didn't get connected? */
            sock->state &= ~FD_CONNECT;
            event &= ~POLLOUT;
            error = sock_error( fd );
        }
        else if (event & POLLOUT)
        {
            /* we got connected */
            sock->state |= FD_WINE_CONNECTED|FD_READ|FD_WRITE;
            sock->state &= ~FD_CONNECT;
            sock->connect_time = current_time;
        }
    }
    else if (sock->state & FD_WINE_LISTENING)
    {
        /* listening */
        if (event & (POLLERR|POLLHUP))
            error = sock_error( fd );
    }
    else
    {
        /* normal data flow */
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

        if ( (hangup_seen || event & (POLLHUP|POLLERR)) && (sock->state & (FD_READ|FD_WRITE)) )
        {
            error = error ? error : sock_error( fd );
            if ( (event & POLLERR) || ( sock_shutdown_type == SOCK_SHUTDOWN_EOF && (event & POLLHUP) ))
                sock->state &= ~FD_WRITE;
            sock->state &= ~FD_READ;

            if (debug_level)
                fprintf(stderr, "socket %p aborted by error %d, event: %x\n", sock, error, event);
        }

        if (hangup_seen)
            event |= POLLHUP;
    }

    event = sock_dispatch_asyncs( sock, event, error );
    sock_dispatch_events( sock, prevstate, event, error );

    /* if anyone is stupid enough to wait on the socket object itself,
     * maybe we should wake them up too, just in case? */
    wake_up( &sock->obj, 0 );

    sock_reselect( sock );
}

static void sock_dump( struct object *obj, int verbose )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );
    fprintf( stderr, "Socket fd=%p, state=%x, mask=%x, pending=%x, held=%x\n",
            sock->fd, sock->state,
            sock->mask, sock->pmask, sock->hmask );
}

static int sock_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );

    return check_fd_events( sock->fd, sock_get_poll_events( sock->fd ) ) != 0;
}

static int sock_get_poll_events( struct fd *fd )
{
    struct sock *sock = get_fd_user( fd );
    unsigned int mask = sock->mask & ~sock->hmask;
    unsigned int smask = sock->state & mask;
    int ev = 0;

    assert( sock->obj.ops == &sock_ops );

    if (sock->state & FD_CONNECT)
        /* connecting, wait for writable */
        return POLLOUT;

    if (!list_empty( &sock->accept_list ) || sock->accept_recv_req )
    {
        ev |= POLLIN | POLLPRI;
    }
    else if (async_queued( &sock->read_q ))
    {
        if (async_waiting( &sock->read_q )) ev |= POLLIN | POLLPRI;
    }
    else if (smask & FD_READ || (sock->state & FD_WINE_LISTENING && mask & FD_ACCEPT))
        ev |= POLLIN | POLLPRI;
    /* We use POLLIN with 0 bytes recv() as FD_CLOSE indication for stream sockets. */
    else if (sock->type == WS_SOCK_STREAM && (sock->state & FD_READ) && (mask & FD_CLOSE) &&
              !(sock->hmask & FD_READ))
        ev |= POLLIN;

    if (async_queued( &sock->write_q ))
    {
        if (async_waiting( &sock->write_q )) ev |= POLLOUT;
    }
    else if (smask & FD_WRITE)
        ev |= POLLOUT;

    return ev;
}

static enum server_fd_type sock_get_fd_type( struct fd *fd )
{
    return FD_TYPE_SOCKET;
}

static void sock_queue_async( struct fd *fd, struct async *async, int type, int count )
{
    struct sock *sock = get_fd_user( fd );
    struct async_queue *queue;

    assert( sock->obj.ops == &sock_ops );

    switch (type)
    {
    case ASYNC_TYPE_READ:
        queue = &sock->read_q;
        break;
    case ASYNC_TYPE_WRITE:
        queue = &sock->write_q;
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if ( ( !( sock->state & (FD_READ|FD_CONNECT|FD_WINE_LISTENING) ) && type == ASYNC_TYPE_READ  ) ||
         ( !( sock->state & (FD_WRITE|FD_CONNECT) ) && type == ASYNC_TYPE_WRITE ) )
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
    struct accept_req *req, *next;

    LIST_FOR_EACH_ENTRY_SAFE( req, next, &sock->accept_list, struct accept_req, entry )
    {
        struct iosb *iosb = async_get_iosb( req->async );
        if (iosb->status != STATUS_PENDING)
            free_accept_req( req );
        release_object( iosb );
    }

    /* ignore reselect on ifchange queue */
    if (&sock->ifchange_q != queue)
        sock_reselect( sock );
}

static struct fd *sock_get_fd( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    return (struct fd *)grab_object( sock->fd );
}

static void sock_destroy( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    struct accept_req *req, *next;

    assert( obj->ops == &sock_ops );

    /* FIXME: special socket shutdown stuff? */

    if ( sock->deferred )
        release_object( sock->deferred );

    if (sock->accept_recv_req)
        async_terminate( sock->accept_recv_req->async, STATUS_CANCELLED );

    LIST_FOR_EACH_ENTRY_SAFE( req, next, &sock->accept_list, struct accept_req, entry )
        async_terminate( req->async, STATUS_CANCELLED );

    async_wake_up( &sock->ifchange_q, STATUS_CANCELLED );
    sock_release_ifchange( sock );
    free_async_queue( &sock->read_q );
    free_async_queue( &sock->write_q );
    free_async_queue( &sock->ifchange_q );
    free_async_queue( &sock->accept_q );
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
    sock->state   = 0;
    sock->mask    = 0;
    sock->hmask   = 0;
    sock->pmask   = 0;
    sock->polling = 0;
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
    init_async_queue( &sock->read_q );
    init_async_queue( &sock->write_q );
    init_async_queue( &sock->ifchange_q );
    init_async_queue( &sock->accept_q );
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
        case WS_IPPROTO_IPIP: return IPPROTO_IPIP;
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
    int sockfd, unix_type, unix_family, unix_protocol;

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

    sock->state  = (type != SOCK_STREAM) ? (FD_READ|FD_WRITE) : 0;
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
    sock_reselect( sock );
    clear_error();
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
        set_win32_error( sock_get_error( errno ));
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
        if ((acceptfd = accept_new_fd( sock )) == -1) return NULL;
        if (!(acceptsock = create_socket()))
        {
            close( acceptfd );
            return NULL;
        }

        /* newly created socket gets the same properties of the listening socket */
        acceptsock->state  = FD_WINE_CONNECTED|FD_READ|FD_WRITE;
        if (sock->state & FD_WINE_NONBLOCKING)
            acceptsock->state |= FD_WINE_NONBLOCKING;
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
    }
    clear_error();
    sock->pmask &= ~FD_ACCEPT;
    sock->hmask &= ~FD_ACCEPT;
    sock_reselect( sock );
    return acceptsock;
}

static int accept_into_socket( struct sock *sock, struct sock *acceptsock )
{
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

    acceptsock->state  |= FD_WINE_CONNECTED|FD_READ|FD_WRITE;
    acceptsock->hmask   = 0;
    acceptsock->pmask   = 0;
    acceptsock->polling = 0;
    acceptsock->proto   = sock->proto;
    acceptsock->type    = sock->type;
    acceptsock->family  = sock->family;
    acceptsock->wparam  = 0;
    acceptsock->deferred = NULL;
    acceptsock->connect_time = current_time;
    fd_copy_completion( acceptsock->fd, newfd );
    release_object( acceptsock->fd );
    acceptsock->fd = newfd;

    clear_error();
    sock->pmask &= ~FD_ACCEPT;
    sock->hmask &= ~FD_ACCEPT;
    sock_reselect( sock );

    return TRUE;
}

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
        case EWOULDBLOCK:       return WSAEWOULDBLOCK;
        case EINPROGRESS:       return WSAEINPROGRESS;
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
        case EHOSTUNREACH:      return STATUS_HOST_UNREACHABLE;
        case ENETDOWN:          return STATUS_NETWORK_BUSY;
        case EPIPE:
        case ECONNRESET:        return STATUS_CONNECTION_RESET;
        case ECONNABORTED:      return STATUS_CONNECTION_ABORTED;

        case 0:                 return STATUS_SUCCESS;
        default:
            errno = err;
            perror("wineserver: sock_get_ntstatus() can't map error");
            return STATUS_UNSUCCESSFUL;
    }
}

static struct accept_req *alloc_accept_req( struct sock *acceptsock, struct async *async,
                                            const struct afd_accept_into_params *params )
{
    struct accept_req *req = mem_alloc( sizeof(*req) );

    if (req)
    {
        req->async = (struct async *)grab_object( async );
        req->acceptsock = acceptsock;
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

static int sock_ioctl( struct fd *fd, ioctl_code_t code, struct async *async )
{
    struct sock *sock = get_fd_user( fd );

    assert( sock->obj.ops == &sock_ops );

    if (get_unix_fd( fd ) == -1 && code != IOCTL_AFD_CREATE) return 0;

    switch(code)
    {
    case IOCTL_AFD_CREATE:
    {
        const struct afd_create_params *params = get_req_data();

        if (get_req_data_size() != sizeof(*params))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }
        init_socket( sock, params->family, params->type, params->protocol, params->flags );
        return 0;
    }

    case IOCTL_AFD_ACCEPT:
    {
        struct sock *acceptsock;
        obj_handle_t handle;

        if (get_reply_max_size() != sizeof(handle))
        {
            set_error( STATUS_BUFFER_TOO_SMALL );
            return 0;
        }

        if (!(acceptsock = accept_socket( sock )))
        {
            struct accept_req *req;

            if (sock->state & FD_WINE_NONBLOCKING) return 0;
            if (get_error() != (0xc0010000 | WSAEWOULDBLOCK)) return 0;

            if (!(req = alloc_accept_req( NULL, async, NULL ))) return 0;
            list_add_tail( &sock->accept_list, &req->entry );

            queue_async( &sock->accept_q, async );
            sock_reselect( sock );
            set_error( STATUS_PENDING );
            return 1;
        }
        handle = alloc_handle( current->process, &acceptsock->obj,
                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, OBJ_INHERIT );
        acceptsock->wparam = handle;
        release_object( acceptsock );
        set_reply_data( &handle, sizeof(handle) );
        return 0;
    }

    case IOCTL_AFD_ACCEPT_INTO:
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
            return 0;
        }

        remote_len = get_reply_max_size() - params->recv_len - params->local_len;
        if (remote_len < sizeof(int))
        {
            set_error( STATUS_INVALID_PARAMETER );
            return 0;
        }

        if (!(acceptsock = (struct sock *)get_handle_obj( current->process, params->accept_handle, access, &sock_ops )))
            return 0;

        if (acceptsock->accept_recv_req)
        {
            release_object( acceptsock );
            set_win32_error( WSAEINVAL );
            return 0;
        }

        if (!(req = alloc_accept_req( acceptsock, async, params ))) return 0;
        list_add_tail( &sock->accept_list, &req->entry );
        acceptsock->accept_recv_req = req;
        release_object( acceptsock );

        acceptsock->wparam = params->accept_handle;
        queue_async( &sock->accept_q, async );
        sock_reselect( sock );
        set_error( STATUS_PENDING );
        return 1;
    }

    case IOCTL_AFD_ADDRESS_LIST_CHANGE:
        if ((sock->state & FD_WINE_NONBLOCKING) && async_is_blocking( async ))
        {
            set_win32_error( WSAEWOULDBLOCK );
            return 0;
        }
        if (!sock_get_ifchange( sock )) return 0;
        queue_async( &sock->ifchange_q, async );
        set_error( STATUS_PENDING );
        return 1;

    default:
        set_error( STATUS_NOT_SUPPORTED );
        return 0;
    }
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
    ifchange_dump,           /* dump */
    no_get_type,             /* get_type */
    add_queue,               /* add_queue */
    NULL,                    /* remove_queue */
    NULL,                    /* signaled */
    no_satisfied,            /* satisfied */
    no_signal,               /* signal */
    ifchange_get_fd,         /* get_fd */
    default_fd_map_access,   /* map_access */
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
        set_win32_error( sock_get_error( errno ));
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
        set_win32_error( sock_get_error( errno ));
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

static struct object_type *socket_device_get_type( struct object *obj );
static void socket_device_dump( struct object *obj, int verbose );
static struct object *socket_device_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr );
static struct object *socket_device_open_file( struct object *obj, unsigned int access,
                                               unsigned int sharing, unsigned int options );

static const struct object_ops socket_device_ops =
{
    sizeof(struct object),      /* size */
    socket_device_dump,         /* dump */
    socket_device_get_type,     /* get_type */
    no_add_queue,               /* add_queue */
    NULL,                       /* remove_queue */
    NULL,                       /* signaled */
    no_satisfied,               /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    default_fd_map_access,      /* map_access */
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

static struct object_type *socket_device_get_type( struct object *obj )
{
    static const WCHAR name[] = {'D','e','v','i','c','e'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static void socket_device_dump( struct object *obj, int verbose )
{
    fputs( "Socket device\n", stderr );
}

static struct object *socket_device_lookup_name( struct object *obj, struct unicode_str *name, unsigned int attr )
{
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

/* set socket event parameters */
DECL_HANDLER(set_socket_event)
{
    struct sock *sock;
    struct event *old_event;

    if (!(sock = (struct sock *)get_handle_obj( current->process, req->handle,
                                                FILE_WRITE_ATTRIBUTES, &sock_ops))) return;
    if (get_unix_fd( sock->fd ) == -1) return;
    old_event = sock->event;
    sock->mask    = req->mask;
    sock->hmask   &= ~req->mask; /* re-enable held events */
    sock->event   = NULL;
    sock->window  = req->window;
    sock->message = req->msg;
    sock->wparam  = req->handle;  /* wparam is the socket handle */
    if (req->event) sock->event = get_event_obj( current->process, req->event, EVENT_MODIFY_STATE );

    if (debug_level && sock->event) fprintf(stderr, "event ptr: %p\n", sock->event);

    sock_reselect( sock );

    sock->state |= FD_WINE_NONBLOCKING;

    /* if a network event is pending, signal the event object
       it is possible that FD_CONNECT or FD_ACCEPT network events has happened
       before a WSAEventSelect() was done on it.
       (when dealing with Asynchronous socket)  */
    sock_wake_up( sock );

    if (old_event) release_object( old_event ); /* we're through with it */
    release_object( &sock->obj );
}

/* get socket event parameters */
DECL_HANDLER(get_socket_event)
{
    struct sock *sock;

    if (!(sock = (struct sock *)get_handle_obj( current->process, req->handle,
                                                FILE_READ_ATTRIBUTES, &sock_ops ))) return;
    if (get_unix_fd( sock->fd ) == -1) return;
    reply->mask  = sock->mask;
    reply->pmask = sock->pmask;
    reply->state = sock->state;
    set_reply_data( sock->errors, min( get_reply_max_size(), sizeof(sock->errors) ));

    if (req->service)
    {
        if (req->c_event)
        {
            struct event *cevent = get_event_obj( current->process, req->c_event,
                                                  EVENT_MODIFY_STATE );
            if (cevent)
            {
                reset_event( cevent );
                release_object( cevent );
            }
        }
        sock->pmask = 0;
        sock_reselect( sock );
    }
    release_object( &sock->obj );
}

/* re-enable pending socket events */
DECL_HANDLER(enable_socket_event)
{
    struct sock *sock;

    if (!(sock = (struct sock*)get_handle_obj( current->process, req->handle,
                                               FILE_WRITE_ATTRIBUTES, &sock_ops)))
        return;

    if (get_unix_fd( sock->fd ) == -1) return;

    /* for event-based notification, windows erases stale events */
    sock->pmask &= ~req->mask;

    sock->hmask &= ~req->mask;
    sock->state |= req->sstate;
    sock->state &= ~req->cstate;
    if (sock->type != WS_SOCK_STREAM) sock->state &= ~STREAM_FLAG_MASK;

    sock_reselect( sock );

    release_object( &sock->obj );
}

DECL_HANDLER(set_socket_deferred)
{
    struct sock *sock, *acceptsock;

    sock=(struct sock *)get_handle_obj( current->process, req->handle, FILE_WRITE_ATTRIBUTES, &sock_ops );
    if ( !sock )
        return;

    acceptsock = (struct sock *)get_handle_obj( current->process, req->deferred, 0, &sock_ops );
    if ( !acceptsock )
    {
        release_object( sock );
        return;
    }
    sock->deferred = acceptsock;
    release_object( sock );
}

DECL_HANDLER(get_socket_info)
{
    struct sock *sock;

    sock = (struct sock *)get_handle_obj( current->process, req->handle, FILE_READ_ATTRIBUTES, &sock_ops );
    if (!sock) return;

    if (get_unix_fd( sock->fd ) == -1) return;

    reply->family   = sock->family;
    reply->type     = sock->type;
    reply->protocol = sock->proto;

    release_object( &sock->obj );
}
