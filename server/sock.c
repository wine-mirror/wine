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
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "process.h"
#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "user.h"

/* To avoid conflicts with the Unix socket headers. Plus we only need a few
 * macros anyway.
 */
#define USE_WS_PREFIX
#include "winsock2.h"

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
    unsigned short      type;        /* socket type */
    unsigned short      family;      /* socket family */
    struct event       *event;       /* event object */
    user_handle_t       window;      /* window to send the message to */
    unsigned int        message;     /* message to send */
    obj_handle_t        wparam;      /* message wparam (socket handle) */
    int                 errors[FD_MAX_EVENTS]; /* event errors */
    struct sock        *deferred;    /* socket that waits for a deferred accept */
    struct async_queue *read_q;      /* queue for asynchronous reads */
    struct async_queue *write_q;     /* queue for asynchronous writes */
};

static void sock_dump( struct object *obj, int verbose );
static int sock_signaled( struct object *obj, struct thread *thread );
static struct fd *sock_get_fd( struct object *obj );
static void sock_destroy( struct object *obj );

static int sock_get_poll_events( struct fd *fd );
static void sock_poll_event( struct fd *fd, int event );
static enum server_fd_type sock_get_fd_type( struct fd *fd );
static void sock_queue_async( struct fd *fd, const async_data_t *data, int type, int count );
static void sock_reselect_async( struct fd *fd, struct async_queue *queue );
static void sock_cancel_async( struct fd *fd );

static int sock_get_error( int err );
static void sock_set_error(void);

static const struct object_ops sock_ops =
{
    sizeof(struct sock),          /* size */
    sock_dump,                    /* dump */
    add_queue,                    /* add_queue */
    remove_queue,                 /* remove_queue */
    sock_signaled,                /* signaled */
    no_satisfied,                 /* satisfied */
    no_signal,                    /* signal */
    sock_get_fd,                  /* get_fd */
    default_fd_map_access,        /* map_access */
    no_lookup_name,               /* lookup_name */
    no_open_file,                 /* open_file */
    fd_close_handle,              /* close_handle */
    sock_destroy                  /* destroy */
};

static const struct fd_ops sock_fd_ops =
{
    sock_get_poll_events,         /* get_poll_events */
    sock_poll_event,              /* poll_event */
    no_flush,                     /* flush */
    sock_get_fd_type,             /* get_file_info */
    default_fd_ioctl,             /* ioctl */
    sock_queue_async,             /* queue_async */
    sock_reselect_async,          /* reselect_async */
    sock_cancel_async             /* cancel_async */
};


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

    if ( socketpair( AF_UNIX, SOCK_STREAM, 0, fd ) ) goto out;
    if ( shutdown( fd[0], 1 ) ) goto out;

    pfd.fd = fd[1];
    pfd.events = POLLIN;
    pfd.revents = 0;

    n = poll( &pfd, 1, 0 );
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
    }
    /* update condition mask */
    set_fd_events( sock->fd, ev );
    return ev;
}

/* After POLLHUP is received, the socket will no longer be in the main select loop.
   This function is used to signal pending events nevertheless */
static void sock_try_event( struct sock *sock, int event )
{
    event = check_fd_events( sock->fd, event );
    if (event)
    {
        if ( debug_level ) fprintf( stderr, "sock_try_event: %x\n", event );
        sock_poll_event( sock->fd, event );
    }
}

/* wake anybody waiting on the socket event or send the associated message */
static void sock_wake_up( struct sock *sock, int pollev )
{
    unsigned int events = sock->pmask & sock->mask;
    int i;
    int async_active = 0;

    if ( pollev & (POLLIN|POLLPRI) && async_waiting( sock->read_q ))
    {
        if (debug_level) fprintf( stderr, "activating read queue for socket %p\n", sock );
        async_wake_up( sock->read_q, STATUS_ALERTED );
        async_active = 1;
    }
    if ( pollev & POLLOUT && async_waiting( sock->write_q ))
    {
        if (debug_level) fprintf( stderr, "activating write queue for socket %p\n", sock );
        async_wake_up( sock->write_q, STATUS_ALERTED );
        async_active = 1;
    }

    /* Do not signal events if there are still pending asynchronous IO requests */
    /* We need this to delay FD_CLOSE events until all pending overlapped requests are processed */
    if ( !events || async_active ) return;

    if (sock->event)
    {
        if (debug_level) fprintf(stderr, "signalling events %x ptr %p\n", events, sock->event );
        set_event( sock->event );
    }
    if (sock->window)
    {
        if (debug_level) fprintf(stderr, "signalling events %x win %p\n", events, sock->window );
        for (i = 0; i < FD_MAX_EVENTS; i++)
        {
            int event = event_bitorder[i];
            if (sock->pmask & (1 << event))
            {
                unsigned int lparam = (1 << event) | (sock->errors[event] << 16);
                post_message( sock->window, sock->message, (unsigned long)sock->wparam, lparam );
            }
        }
        sock->pmask = 0;
        sock_reselect( sock );
    }
}

static inline int sock_error( struct fd *fd )
{
    unsigned int optval = 0, optlen;

    optlen = sizeof(optval);
    getsockopt( get_unix_fd(fd), SOL_SOCKET, SO_ERROR, (void *) &optval, &optlen);
    return optval ? sock_get_error(optval) : 0;
}

static void sock_poll_event( struct fd *fd, int event )
{
    struct sock *sock = get_fd_user( fd );
    int hangup_seen = 0;

    assert( sock->obj.ops == &sock_ops );
    if (debug_level)
        fprintf(stderr, "socket %p select event: %x\n", sock, event);
    if (sock->state & FD_CONNECT)
    {
        /* connecting */
        if (event & POLLOUT)
        {
            /* we got connected */
            sock->state |= FD_WINE_CONNECTED|FD_READ|FD_WRITE;
            sock->state &= ~FD_CONNECT;
            sock->pmask |= FD_CONNECT;
            sock->errors[FD_CONNECT_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %p connection success\n", sock);
        }
        else if (event & (POLLERR|POLLHUP))
        {
            /* we didn't get connected? */
            sock->state &= ~FD_CONNECT;
            sock->pmask |= FD_CONNECT;
            sock->errors[FD_CONNECT_BIT] = sock_error( fd );
            if (debug_level)
                fprintf(stderr, "socket %p connection failure\n", sock);
        }
    }
    else if (sock->state & FD_WINE_LISTENING)
    {
        /* listening */
        if (event & POLLIN)
        {
            /* incoming connection */
            sock->pmask |= FD_ACCEPT;
            sock->errors[FD_ACCEPT_BIT] = 0;
            sock->hmask |= FD_ACCEPT;
        }
        else if (event & (POLLERR|POLLHUP))
        {
            /* failed incoming connection? */
            sock->pmask |= FD_ACCEPT;
            sock->errors[FD_ACCEPT_BIT] = sock_error( fd );
            sock->hmask |= FD_ACCEPT;
        }
    }
    else
    {
        /* normal data flow */
        if ( sock->type == SOCK_STREAM && ( event & POLLIN ) )
        {
            char dummy;
            int nr;

            /* Linux 2.4 doesn't report POLLHUP if only one side of the socket
             * has been closed, so we need to check for it explicitly here */
            nr  = recv( get_unix_fd( fd ), &dummy, 1, MSG_PEEK );
            if ( nr > 0 )
            {
                /* incoming data */
                sock->pmask |= FD_READ;
                sock->hmask |= (FD_READ|FD_CLOSE);
                sock->errors[FD_READ_BIT] = 0;
                if (debug_level)
                    fprintf(stderr, "socket %p is readable\n", sock );
            }
            else if ( nr == 0 )
                hangup_seen = 1;
            else
            {
                /* EAGAIN can happen if an async recv() falls between the server's poll()
                   call and the invocation of this routine */
                if ( errno == EAGAIN )
                    event &= ~POLLIN;
                else
                {
                    if ( debug_level )
                        fprintf( stderr, "recv error on socket %p: %d\n", sock, errno );
                    event = POLLERR;
                }
            }

        }
        else if ( sock_shutdown_type == SOCK_SHUTDOWN_POLLHUP && (event & POLLHUP) )
        {
            hangup_seen = 1;
        }
        else if ( event & POLLIN ) /* POLLIN for non-stream socket */
        {
            sock->pmask |= FD_READ;
            sock->hmask |= (FD_READ|FD_CLOSE);
            sock->errors[FD_READ_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %p is readable\n", sock );

        }

        if (event & POLLOUT)
        {
            sock->pmask |= FD_WRITE;
            sock->hmask |= FD_WRITE;
            sock->errors[FD_WRITE_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %p is writable\n", sock);
        }
        if (event & POLLPRI)
        {
            sock->pmask |= FD_OOB;
            sock->hmask |= FD_OOB;
            sock->errors[FD_OOB_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %p got OOB data\n", sock);
        }
        /* According to WS2 specs, FD_CLOSE is only delivered when there is
           no more data to be read (i.e. hangup_seen = 1) */
        else if ( hangup_seen && (sock->state & (FD_READ|FD_WRITE) ))
        {
            sock->errors[FD_CLOSE_BIT] = sock_error( fd );
            if ( (event & POLLERR) || ( sock_shutdown_type == SOCK_SHUTDOWN_EOF && (event & POLLHUP) ))
                sock->state &= ~FD_WRITE;
            sock->pmask |= FD_CLOSE;
            sock->hmask |= FD_CLOSE;
            if (debug_level)
                fprintf(stderr, "socket %p aborted by error %d, event: %x - removing from select loop\n",
                        sock, sock->errors[FD_CLOSE_BIT], event);
        }
    }

    if ( sock->pmask & FD_CLOSE || event & (POLLERR|POLLHUP) )
    {
        if ( debug_level )
            fprintf( stderr, "removing socket %p from select loop\n", sock );
        set_fd_events( sock->fd, -1 );
    }
    else
        sock_reselect( sock );

    /* wake up anyone waiting for whatever just happened */
    if ( sock->pmask & sock->mask || sock->flags & WSA_FLAG_OVERLAPPED ) sock_wake_up( sock, event );

    /* if anyone is stupid enough to wait on the socket object itself,
     * maybe we should wake them up too, just in case? */
    wake_up( &sock->obj, 0 );
}

static void sock_dump( struct object *obj, int verbose )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );
    printf( "Socket fd=%p, state=%x, mask=%x, pending=%x, held=%x\n",
            sock->fd, sock->state,
            sock->mask, sock->pmask, sock->hmask );
}

static int sock_signaled( struct object *obj, struct thread *thread )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );

    return check_fd_events( sock->fd, sock_get_poll_events( sock->fd ) ) != 0;
}

static int sock_get_poll_events( struct fd *fd )
{
    struct sock *sock = get_fd_user( fd );
    unsigned int mask = sock->mask & sock->state & ~sock->hmask;
    int ev = 0;

    assert( sock->obj.ops == &sock_ops );

    if (sock->state & FD_CONNECT)
        /* connecting, wait for writable */
        return POLLOUT;
    if (sock->state & FD_WINE_LISTENING)
        /* listening, wait for readable */
        return (sock->hmask & FD_ACCEPT) ? 0 : POLLIN;

    if (mask & FD_READ  || async_waiting( sock->read_q )) ev |= POLLIN | POLLPRI;
    if (mask & FD_WRITE || async_waiting( sock->write_q )) ev |= POLLOUT;
    /* We use POLLIN with 0 bytes recv() as FD_CLOSE indication for stream sockets. */
    if ( sock->type == SOCK_STREAM && ( sock->mask & ~sock->hmask & FD_CLOSE) )
        ev |= POLLIN;

    return ev;
}

static enum server_fd_type sock_get_fd_type( struct fd *fd )
{
    return FD_TYPE_SOCKET;
}

static void sock_queue_async( struct fd *fd, const async_data_t *data, int type, int count )
{
    struct sock *sock = get_fd_user( fd );
    struct async_queue *queue;
    int pollev;

    assert( sock->obj.ops == &sock_ops );

    switch (type)
    {
    case ASYNC_TYPE_READ:
        if (!sock->read_q && !(sock->read_q = create_async_queue( sock->fd ))) return;
        queue = sock->read_q;
        sock->hmask &= ~FD_CLOSE;
        break;
    case ASYNC_TYPE_WRITE:
        if (!sock->write_q && !(sock->write_q = create_async_queue( sock->fd ))) return;
        queue = sock->write_q;
        break;
    default:
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if ( ( !( sock->state & FD_READ ) && type == ASYNC_TYPE_READ  ) ||
         ( !( sock->state & FD_WRITE ) && type == ASYNC_TYPE_WRITE ) )
    {
        set_error( STATUS_PIPE_DISCONNECTED );
    }
    else
    {
        struct async *async;
        if (!(async = create_async( current, queue, data ))) return;
        release_object( async );
        set_error( STATUS_PENDING );
    }

    pollev = sock_reselect( sock );
    if ( pollev ) sock_try_event( sock, pollev );
}

static void sock_reselect_async( struct fd *fd, struct async_queue *queue )
{
    struct sock *sock = get_fd_user( fd );
    int events = sock_reselect( sock );
    if (events) sock_try_event( sock, events );
}

static void sock_cancel_async( struct fd *fd )
{
    struct sock *sock = get_fd_user( fd );
    assert( sock->obj.ops == &sock_ops );

    async_wake_up( sock->read_q, STATUS_CANCELLED );
    async_wake_up( sock->write_q, STATUS_CANCELLED );
}

static struct fd *sock_get_fd( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    return (struct fd *)grab_object( sock->fd );
}

static void sock_destroy( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );

    /* FIXME: special socket shutdown stuff? */

    if ( sock->deferred )
        release_object( sock->deferred );

    free_async_queue( sock->read_q );
    free_async_queue( sock->write_q );
    if (sock->event) release_object( sock->event );
    if (sock->fd)
    {
        /* shut the socket down to force pending poll() calls in the client to return */
        shutdown( get_unix_fd(sock->fd), SHUT_RDWR );
        release_object( sock->fd );
    }
}

/* create a new and unconnected socket */
static struct object *create_socket( int family, int type, int protocol, unsigned int flags )
{
    struct sock *sock;
    int sockfd;

    sockfd = socket( family, type, protocol );
    if (debug_level)
        fprintf(stderr,"socket(%d,%d,%d)=%d\n",family,type,protocol,sockfd);
    if (sockfd == -1)
    {
        sock_set_error();
        return NULL;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK); /* make socket nonblocking */
    if (!(sock = alloc_object( &sock_ops )))
    {
        close( sockfd );
        return NULL;
    }
    sock->state = (type != SOCK_STREAM) ? (FD_READ|FD_WRITE) : 0;
    sock->mask    = 0;
    sock->hmask   = 0;
    sock->pmask   = 0;
    sock->polling = 0;
    sock->flags   = flags;
    sock->type    = type;
    sock->family  = family;
    sock->event   = NULL;
    sock->window  = 0;
    sock->message = 0;
    sock->wparam  = 0;
    sock->deferred = NULL;
    sock->read_q  = NULL;
    sock->write_q = NULL;
    if (!(sock->fd = create_anonymous_fd( &sock_fd_ops, sockfd, &sock->obj,
                            (flags & WSA_FLAG_OVERLAPPED) ? 0 : FILE_SYNCHRONOUS_IO_NONALERT )))
    {
        release_object( sock );
        return NULL;
    }
    sock_reselect( sock );
    clear_error();
    return &sock->obj;
}

/* accept a socket (creates a new fd) */
static struct sock *accept_socket( obj_handle_t handle )
{
    struct sock *acceptsock;
    struct sock *sock;
    int	acceptfd;
    struct sockaddr	saddr;

    sock = (struct sock *)get_handle_obj( current->process, handle, FILE_READ_DATA, &sock_ops );
    if (!sock)
    	return NULL;

    if ( sock->deferred )
    {
        acceptsock = sock->deferred;
        sock->deferred = NULL;
    }
    else
    {

        /* Try to accept(2). We can't be safe that this an already connected socket
         * or that accept() is allowed on it. In those cases we will get -1/errno
         * return.
         */
        unsigned int slen = sizeof(saddr);
        acceptfd = accept( get_unix_fd(sock->fd), &saddr, &slen);
        if (acceptfd==-1)
        {
            sock_set_error();
            release_object( sock );
            return NULL;
        }
        if (!(acceptsock = alloc_object( &sock_ops )))
        {
            close( acceptfd );
            release_object( sock );
            return NULL;
        }

        /* newly created socket gets the same properties of the listening socket */
        fcntl(acceptfd, F_SETFL, O_NONBLOCK); /* make socket nonblocking */
        acceptsock->state  = FD_WINE_CONNECTED|FD_READ|FD_WRITE;
        if (sock->state & FD_WINE_NONBLOCKING)
            acceptsock->state |= FD_WINE_NONBLOCKING;
        acceptsock->mask    = sock->mask;
        acceptsock->hmask   = 0;
        acceptsock->pmask   = 0;
        acceptsock->polling = 0;
        acceptsock->type    = sock->type;
        acceptsock->family  = sock->family;
        acceptsock->event   = NULL;
        acceptsock->window  = sock->window;
        acceptsock->message = sock->message;
        acceptsock->wparam  = 0;
        if (sock->event) acceptsock->event = (struct event *)grab_object( sock->event );
        acceptsock->flags = sock->flags;
        acceptsock->deferred = NULL;
        acceptsock->read_q  = NULL;
        acceptsock->write_q = NULL;
        if (!(acceptsock->fd = create_anonymous_fd( &sock_fd_ops, acceptfd, &acceptsock->obj,
                                                    get_fd_options( sock->fd ) )))
        {
            release_object( acceptsock );
            release_object( sock );
            return NULL;
        }
    }
    clear_error();
    sock->pmask &= ~FD_ACCEPT;
    sock->hmask &= ~FD_ACCEPT;
    sock_reselect( sock );
    release_object( sock );
    return acceptsock;
}

/* set the last error depending on errno */
static int sock_get_error( int err )
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
        default:
            errno = err;
            perror("wineserver: sock_get_error() can't map error");
            return WSAEFAULT;
    }
}

/* set the last error depending on errno */
static void sock_set_error(void)
{
    set_error( sock_get_error( errno ) );
}

/* create a socket */
DECL_HANDLER(create_socket)
{
    struct object *obj;

    reply->handle = 0;
    if ((obj = create_socket( req->family, req->type, req->protocol, req->flags )) != NULL)
    {
        reply->handle = alloc_handle( current->process, obj, req->access, req->attributes );
        release_object( obj );
    }
}

/* accept a socket */
DECL_HANDLER(accept_socket)
{
    struct sock *sock;

    reply->handle = 0;
    if ((sock = accept_socket( req->lhandle )) != NULL)
    {
        reply->handle = alloc_handle( current->process, &sock->obj, req->access, req->attributes );
        sock->wparam = reply->handle;  /* wparam for message is the socket handle */
        sock_reselect( sock );
        release_object( &sock->obj );
    }
}

/* set socket event parameters */
DECL_HANDLER(set_socket_event)
{
    struct sock *sock;
    struct event *old_event;
    int pollev;

    if (!(sock = (struct sock *)get_handle_obj( current->process, req->handle,
                                                FILE_WRITE_ATTRIBUTES, &sock_ops))) return;
    old_event = sock->event;
    sock->mask    = req->mask;
    sock->hmask   &= ~req->mask; /* re-enable held events */
    sock->event   = NULL;
    sock->window  = req->window;
    sock->message = req->msg;
    sock->wparam  = req->handle;  /* wparam is the socket handle */
    if (req->event) sock->event = get_event_obj( current->process, req->event, EVENT_MODIFY_STATE );

    if (debug_level && sock->event) fprintf(stderr, "event ptr: %p\n", sock->event);

    pollev = sock_reselect( sock );
    if ( pollev ) sock_try_event( sock, pollev );

    if (sock->mask)
        sock->state |= FD_WINE_NONBLOCKING;

    /* if a network event is pending, signal the event object
       it is possible that FD_CONNECT or FD_ACCEPT network events has happened
       before a WSAEventSelect() was done on it.
       (when dealing with Asynchronous socket)  */
    if (sock->pmask & sock->mask) sock_wake_up( sock, pollev );

    if (old_event) release_object( old_event ); /* we're through with it */
    release_object( &sock->obj );
}

/* get socket event parameters */
DECL_HANDLER(get_socket_event)
{
    struct sock *sock;

    sock = (struct sock *)get_handle_obj( current->process, req->handle, FILE_READ_ATTRIBUTES, &sock_ops );
    if (!sock)
    {
        reply->mask  = 0;
        reply->pmask = 0;
        reply->state = 0;
        set_error( WSAENOTSOCK );
        return;
    }
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
    int pollev;

    if (!(sock = (struct sock*)get_handle_obj( current->process, req->handle,
                                               FILE_WRITE_ATTRIBUTES, &sock_ops)))
        return;

    sock->pmask &= ~req->mask; /* is this safe? */
    sock->hmask &= ~req->mask;
    if ( req->mask & FD_READ )
        sock->hmask &= ~FD_CLOSE;
    sock->state |= req->sstate;
    sock->state &= ~req->cstate;
    if ( sock->type != SOCK_STREAM ) sock->state &= ~STREAM_FLAG_MASK;

    pollev = sock_reselect( sock );
    if ( pollev ) sock_try_event( sock, pollev );

    release_object( &sock->obj );
}

DECL_HANDLER(set_socket_deferred)
{
    struct sock *sock, *acceptsock;

    sock=(struct sock *)get_handle_obj( current->process, req->handle, FILE_WRITE_ATTRIBUTES, &sock_ops );
    if ( !sock )
    {
        set_error( WSAENOTSOCK );
        return;
    }
    acceptsock = (struct sock *)get_handle_obj( current->process, req->deferred, 0, &sock_ops );
    if ( !acceptsock )
    {
        release_object( sock );
        set_error( WSAENOTSOCK );
        return;
    }
    sock->deferred = acceptsock;
    release_object( sock );
}
