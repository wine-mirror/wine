/*
 * Server-side socket management
 *
 * Copyright (C) 1999 Marcus Meissner, Ove Kåven
 *
 * FIXME: we use read|write access in all cases. Shouldn't we depend that
 * on the access of the current handle?
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#include <time.h>
#include <unistd.h>

#include "winerror.h"
#include "winbase.h"
#include "winsock2.h"
#include "process.h"
#include "handle.h"
#include "thread.h"
#include "request.h"

struct sock
{
    struct object       obj;         /* object header */
    unsigned int        state;       /* status bits */
    unsigned int        mask;        /* event mask */
    unsigned int        hmask;       /* held (blocked) events */
    unsigned int        pmask;       /* pending events */
    struct event       *event;       /* event object */
    int                 errors[FD_MAX_EVENTS]; /* event errors */
};

static void sock_dump( struct object *obj, int verbose );
static int sock_signaled( struct object *obj, struct thread *thread );
static int sock_get_poll_events( struct object *obj );
static void sock_poll_event( struct object *obj, int event );
static int sock_get_fd( struct object *obj );
static void sock_destroy( struct object *obj );
static void sock_set_error(void);

static const struct object_ops sock_ops =
{
    sizeof(struct sock),          /* size */
    sock_dump,                    /* dump */
    add_queue,                    /* add_queue */
    remove_queue,                 /* remove_queue */
    sock_signaled,                /* signaled */
    no_satisfied,                 /* satisfied */
    sock_get_poll_events,         /* get_poll_events */
    sock_poll_event,              /* poll_event */
    sock_get_fd,                  /* get_read_fd */
    sock_get_fd,                  /* get_write_fd */
    no_flush,                     /* flush */
    no_get_file_info,             /* get_file_info */
    sock_destroy                  /* destroy */
};

static void sock_reselect( struct sock *sock )
{
    int ev = sock_get_poll_events( &sock->obj );
    struct pollfd pfd;

    if (debug_level)
        fprintf(stderr,"sock_reselect(%d): new mask %x\n", sock->obj.fd, ev);
    set_select_events( &sock->obj, ev );

    /* check whether condition is satisfied already */
    pfd.fd = sock->obj.fd;
    pfd.events = ev;
    pfd.revents = 0;
    poll( &pfd, 1, 0 );
    if (pfd.revents & (POLLIN|POLLOUT|POLLPRI))
        sock_poll_event( &sock->obj, pfd.revents);
}

inline static int sock_error(int s)
{
    unsigned int optval, optlen;
    
    optlen = sizeof(optval);
    getsockopt(s, SOL_SOCKET, SO_ERROR, (void *) &optval, &optlen);
    return optval;
}

static void sock_poll_event( struct object *obj, int event )
{
    struct sock *sock = (struct sock *)obj;
    unsigned int emask;
    assert( sock->obj.ops == &sock_ops );
    if (debug_level)
        fprintf(stderr, "socket %d select event: %x\n", sock->obj.fd, event);
    if (sock->state & WS_FD_CONNECT)
    {
        /* connecting */
        if (event & POLLOUT)
        {
            /* we got connected */
            sock->state |= WS_FD_CONNECTED|WS_FD_READ|WS_FD_WRITE;
            sock->state &= ~WS_FD_CONNECT;
            sock->pmask |= FD_CONNECT;
            sock->errors[FD_CONNECT_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %d connection success\n", sock->obj.fd);
        }
        else if (event & (POLLERR|POLLHUP))
        {
            /* we didn't get connected? */
            sock->state &= ~WS_FD_CONNECT;
            sock->pmask |= FD_CONNECT;
            sock->errors[FD_CONNECT_BIT] = sock_error( sock->obj.fd );
            if (debug_level)
                fprintf(stderr, "socket %d connection failure\n", sock->obj.fd);
        }
    } else
    if (sock->state & WS_FD_LISTENING)
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
            sock->errors[FD_ACCEPT_BIT] = sock_error( sock->obj.fd );
            sock->hmask |= FD_ACCEPT;
        }
    } else
    {
        /* normal data flow */
        if (event & POLLIN)
        {
            /* incoming data */
            sock->pmask |= FD_READ;
            sock->hmask |= FD_READ;
            sock->errors[FD_READ_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %d is readable\n", sock->obj.fd );
        }
        if (event & POLLOUT)
        {
            sock->pmask |= FD_WRITE;
            sock->hmask |= FD_WRITE;
            sock->errors[FD_WRITE_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %d is writable\n", sock->obj.fd);
        }
        if (event & POLLPRI)
        {
            sock->pmask |= FD_OOB;
            sock->hmask |= FD_OOB;
            sock->errors[FD_OOB_BIT] = 0;
            if (debug_level)
                fprintf(stderr, "socket %d got OOB data\n", sock->obj.fd);
        }
        if (event & (POLLERR|POLLHUP))
        {
            sock->errors[FD_CLOSE_BIT] = sock_error( sock->obj.fd );
            /* we got an error, socket closing? */
            sock->state &= ~(WS_FD_CONNECTED|WS_FD_READ|WS_FD_WRITE);
            sock->pmask |= FD_CLOSE;
            if (debug_level)
                fprintf(stderr, "socket %d aborted by error %d\n",
                        sock->obj.fd, sock->errors[FD_CLOSE_BIT]);
        }
    }

    if (event & (POLLERR|POLLHUP))
        set_select_events( &sock->obj, -1 );
    else
        sock_reselect( sock );
    /* wake up anyone waiting for whatever just happened */
    emask = sock->pmask & sock->mask;
    if (debug_level && emask)
        fprintf(stderr, "socket %d pending events: %x\n", sock->obj.fd, emask);
    if (emask && sock->event) {
        if (debug_level) fprintf(stderr, "signalling event ptr %p\n", sock->event);
        set_event(sock->event);
    }

    /* if anyone is stupid enough to wait on the socket object itself,
     * maybe we should wake them up too, just in case? */
    wake_up( &sock->obj, 0 );
}

static void sock_dump( struct object *obj, int verbose )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );
    printf( "Socket fd=%d, state=%x, mask=%x, pending=%x, held=%x\n",
            sock->obj.fd, sock->state,
            sock->mask, sock->pmask, sock->hmask );
}

static int sock_signaled( struct object *obj, struct thread *thread )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );

    return check_select_events( sock->obj.fd, sock_get_poll_events( &sock->obj ) );
}

static int sock_get_poll_events( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    unsigned int mask = sock->mask & sock->state & ~sock->hmask;
    int ev = 0;

    assert( obj->ops == &sock_ops );

    if (sock->state & WS_FD_CONNECT)
        /* connecting, wait for writable */
        return POLLOUT;
    if (sock->state & WS_FD_LISTENING)
        /* listening, wait for readable */
        return (sock->hmask & FD_ACCEPT) ? 0 : POLLIN;

    if (mask & FD_READ)  ev |= POLLIN | POLLPRI;
    if (mask & FD_WRITE) ev |= POLLOUT;
    return ev;
}

static int sock_get_fd( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    int fd;
    assert( obj->ops == &sock_ops );
    fd = dup( sock->obj.fd );
    if (fd==-1)
    	sock_set_error();
    return fd;
}

static void sock_destroy( struct object *obj )
{
    struct sock *sock = (struct sock *)obj;
    assert( obj->ops == &sock_ops );

    /* FIXME: special socket shutdown stuff? */
    if (sock->event)
    {
        /* if the service thread was waiting for the event object,
         * we should now signal it, to let the service thread
         * object detect that it is now orphaned... */
        if (sock->mask & WS_FD_SERVEVENT)
            set_event( sock->event );
        /* we're through with it */
        release_object( sock->event );
    }
}

/* create a new and unconnected socket */
static struct object *create_socket( int family, int type, int protocol )
{
    struct sock *sock;
    int sockfd;

    sockfd = socket( family, type, protocol );
    if (debug_level)
        fprintf(stderr,"socket(%d,%d,%d)=%d\n",family,type,protocol,sockfd);
    if (sockfd == -1) {
        sock_set_error();
        return NULL;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK); /* make socket nonblocking */
    if (!(sock = alloc_object( &sock_ops, sockfd ))) return NULL;
    sock->state = (type!=SOCK_STREAM) ? WS_FD_READ|WS_FD_WRITE : 0;
    sock->mask  = 0;
    sock->hmask = 0;
    sock->pmask = 0;
    sock->event = NULL;
    sock_reselect( sock );
    clear_error();
    return &sock->obj;
}

/* accept a socket (creates a new fd) */
static struct object *accept_socket( int handle )
{
    struct sock *acceptsock;
    struct sock *sock;
    int	acceptfd;
    struct sockaddr	saddr;
    int			slen;

    sock=(struct sock*)get_handle_obj(current->process,handle,
                                      GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,&sock_ops);
    if (!sock)
    	return NULL;
    /* Try to accept(2). We can't be safe that this an already connected socket 
     * or that accept() is allowed on it. In those cases we will get -1/errno
     * return.
     */
    slen = sizeof(saddr);
    acceptfd = accept(sock->obj.fd,&saddr,&slen);
    if (acceptfd==-1) {
    	sock_set_error();
        release_object( sock );
	return NULL;
    }
    if (!(acceptsock = alloc_object( &sock_ops, acceptfd )))
    {
        release_object( sock );
        return NULL;
    }

    acceptsock->state  = WS_FD_CONNECTED|WS_FD_READ|WS_FD_WRITE;
    acceptsock->mask   = sock->mask;
    acceptsock->hmask  = 0;
    acceptsock->pmask  = 0;
    acceptsock->event  = NULL;
    if (sock->event) acceptsock->event = (struct event *)grab_object( sock->event );

    sock_reselect( acceptsock );
    clear_error();
    sock->pmask &= ~FD_ACCEPT;
    sock->hmask &= ~FD_ACCEPT;
    release_object( sock );
    return &acceptsock->obj;
}

/* set the last error depending on errno */
static void sock_set_error(void)
{
    switch (errno)
    {
        case EINTR:             set_error(WSAEINTR);break;
        case EBADF:             set_error(WSAEBADF);break;
        case EPERM:
        case EACCES:            set_error(WSAEACCES);break;
        case EFAULT:            set_error(WSAEFAULT);break;
        case EINVAL:            set_error(WSAEINVAL);break;
        case EMFILE:            set_error(WSAEMFILE);break;
        case EWOULDBLOCK:       set_error(WSAEWOULDBLOCK);break;
        case EINPROGRESS:       set_error(WSAEINPROGRESS);break;
        case EALREADY:          set_error(WSAEALREADY);break;
        case ENOTSOCK:          set_error(WSAENOTSOCK);break;
        case EDESTADDRREQ:      set_error(WSAEDESTADDRREQ);break;
        case EMSGSIZE:          set_error(WSAEMSGSIZE);break;
        case EPROTOTYPE:        set_error(WSAEPROTOTYPE);break;
        case ENOPROTOOPT:       set_error(WSAENOPROTOOPT);break;
        case EPROTONOSUPPORT:   set_error(WSAEPROTONOSUPPORT);break;
        case ESOCKTNOSUPPORT:   set_error(WSAESOCKTNOSUPPORT);break;
        case EOPNOTSUPP:        set_error(WSAEOPNOTSUPP);break;
        case EPFNOSUPPORT:      set_error(WSAEPFNOSUPPORT);break;
        case EAFNOSUPPORT:      set_error(WSAEAFNOSUPPORT);break;
        case EADDRINUSE:        set_error(WSAEADDRINUSE);break;
        case EADDRNOTAVAIL:     set_error(WSAEADDRNOTAVAIL);break;
        case ENETDOWN:          set_error(WSAENETDOWN);break;
        case ENETUNREACH:       set_error(WSAENETUNREACH);break;
        case ENETRESET:         set_error(WSAENETRESET);break;
        case ECONNABORTED:      set_error(WSAECONNABORTED);break;
        case EPIPE:
        case ECONNRESET:        set_error(WSAECONNRESET);break;
        case ENOBUFS:           set_error(WSAENOBUFS);break;
        case EISCONN:           set_error(WSAEISCONN);break;
        case ENOTCONN:          set_error(WSAENOTCONN);break;
        case ESHUTDOWN:         set_error(WSAESHUTDOWN);break;
        case ETOOMANYREFS:      set_error(WSAETOOMANYREFS);break;
        case ETIMEDOUT:         set_error(WSAETIMEDOUT);break;
        case ECONNREFUSED:      set_error(WSAECONNREFUSED);break;
        case ELOOP:             set_error(WSAELOOP);break;
        case ENAMETOOLONG:      set_error(WSAENAMETOOLONG);break;
        case EHOSTDOWN:         set_error(WSAEHOSTDOWN);break;
        case EHOSTUNREACH:      set_error(WSAEHOSTUNREACH);break;
        case ENOTEMPTY:         set_error(WSAENOTEMPTY);break;
#ifdef EPROCLIM
        case EPROCLIM:          set_error(WSAEPROCLIM);break;
#endif
#ifdef EUSERS
        case EUSERS:            set_error(WSAEUSERS);break;
#endif
#ifdef EDQUOT
        case EDQUOT:            set_error(WSAEDQUOT);break;
#endif
#ifdef ESTALE
        case ESTALE:            set_error(WSAESTALE);break;
#endif
#ifdef EREMOTE
        case EREMOTE:           set_error(WSAEREMOTE);break;
#endif
    default:        perror("sock_set_error"); set_error( ERROR_UNKNOWN ); break;
    }
}

/* create a socket */
DECL_HANDLER(create_socket)
{
    struct object *obj;
    int s = -1;

    if ((obj = create_socket( req->family, req->type, req->protocol )) != NULL)
    {
        s = alloc_handle( current->process, obj, req->access, req->inherit );
        release_object( obj );
    }
    req->handle = s;
}

/* accept a socket */
DECL_HANDLER(accept_socket)
{
    struct object *obj;
    int s = -1;

    if ((obj = accept_socket( req->lhandle )) != NULL)
    {
        s = alloc_handle( current->process, obj, req->access, req->inherit );
        release_object( obj );
    }
    req->handle = s;
}

/* set socket event parameters */
DECL_HANDLER(set_socket_event)
{
    struct sock *sock;
    struct event *oevent;
    unsigned int omask;

    sock=(struct sock*)get_handle_obj(current->process,req->handle,GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,&sock_ops);
    if (!sock)
	return;
    oevent = sock->event;
    omask  = sock->mask;
    sock->mask    = req->mask;
    sock->event   = get_event_obj( current->process, req->event, EVENT_MODIFY_STATE );
    if (debug_level && sock->event) fprintf(stderr, "event ptr: %p\n", sock->event);
    sock_reselect( sock );
    if (sock->mask)
        sock->state |= WS_FD_NONBLOCKING;
    if (oevent)
    {
    	if ((oevent != sock->event) && (omask & WS_FD_SERVEVENT))
            /* if the service thread was waiting for the old event object,
             * we should now signal it, to let the service thread
             * object detect that it is now orphaned... */
            set_event( oevent );
        /* we're through with it */
        release_object( oevent );
    }
    release_object( &sock->obj );
}

/* get socket event parameters */
DECL_HANDLER(get_socket_event)
{
    struct sock *sock;

    sock=(struct sock*)get_handle_obj(current->process,req->handle,GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,&sock_ops);
    if (!sock)
    {
	req->mask  = 0;
	req->pmask = 0;
	req->state = 0;
	set_error(WSAENOTSOCK);
	return;
    }
    req->mask    = sock->mask;
    req->pmask   = sock->pmask;
    req->state   = sock->state;
    memcpy(req->errors, sock->errors, sizeof(sock->errors));
    clear_error();
    if (req->service)
    {
        if (req->s_event)
        {
            struct event *sevent = get_event_obj(current->process, req->s_event, 0);
            if (sevent == sock->event)
                req->s_event = 0;
            release_object( sevent );
        }
        if (!req->s_event)
        {
            sock->pmask = 0;
            sock_reselect( sock );
        }
        else set_error(WSAEINVAL);
    }
    release_object( &sock->obj );
}

/* re-enable pending socket events */
DECL_HANDLER(enable_socket_event)
{
    struct sock *sock;

    sock=(struct sock*)get_handle_obj(current->process,req->handle,GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,&sock_ops);
    if (!sock)
    	return;
    sock->pmask &= ~req->mask; /* is this safe? */
    sock->hmask &= ~req->mask;
    sock->state |= req->sstate;
    sock->state &= ~req->cstate;
    sock_reselect( sock );
    release_object( &sock->obj );
}
