/*
 * Server-side pipe management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2001 Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 *   message mode
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <time.h>
#include <unistd.h>

#include "winbase.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"

enum pipe_state
{
    ps_none,
    ps_idle_server,
    ps_wait_open,
    ps_connected_server,
    ps_wait_disconnect,
    ps_disconnected_server,
    ps_wait_connect
};

struct wait_info
{
    struct thread       *thread;
    void                *func;
    void                *overlapped;
};

struct named_pipe;

struct pipe_server
{
    struct object        obj;
    struct fd           *fd;
    enum pipe_state      state;
    struct pipe_client  *client;
    struct named_pipe   *pipe;
    struct pipe_server  *next;
    struct pipe_server  *prev;
    struct timeout_user *flush_poll;
    struct event        *event;
    struct wait_info     wait;
};

struct pipe_client
{
    struct object        obj;
    struct fd           *fd;
    struct pipe_server  *server;
    struct wait_info     wait;
};

struct connect_wait
{
    struct wait_info     wait;
    struct connect_wait *next;
};

struct named_pipe
{
    struct object       obj;         /* object header */
    unsigned int        pipemode;
    unsigned int        maxinstances;
    unsigned int        outsize;
    unsigned int        insize;
    unsigned int        timeout;
    unsigned int        instances;
    struct pipe_server *servers;
    struct connect_wait *connect_waiters;
};

static void named_pipe_dump( struct object *obj, int verbose );
static void named_pipe_destroy( struct object *obj );

static const struct object_ops named_pipe_ops =
{
    sizeof(struct named_pipe),    /* size */
    named_pipe_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    no_get_fd,                    /* get_fd */
    named_pipe_destroy            /* destroy */
};

/* common to clients and servers */
static int pipe_end_get_poll_events( struct fd *fd );
static int pipe_end_get_info( struct fd *fd, 
                  struct get_file_info_reply *reply, int *flags );

/* server end functions */
static void pipe_server_dump( struct object *obj, int verbose );
static struct fd *pipe_server_get_fd( struct object *obj );
static void pipe_server_destroy( struct object *obj);
static int pipe_server_flush( struct fd *fd, struct event **event );

static const struct object_ops pipe_server_ops =
{
    sizeof(struct pipe_server),   /* size */
    pipe_server_dump,             /* dump */
    default_fd_add_queue,         /* add_queue */
    default_fd_remove_queue,      /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    pipe_server_get_fd,           /* get_fd */
    pipe_server_destroy           /* destroy */
};

static const struct fd_ops pipe_server_fd_ops =
{
    pipe_end_get_poll_events,     /* get_poll_events */
    default_poll_event,           /* poll_event */
    pipe_server_flush,            /* flush */
    pipe_end_get_info,            /* get_file_info */
    no_queue_async                /* queue_async */
};

/* client end functions */
static void pipe_client_dump( struct object *obj, int verbose );
static struct fd *pipe_client_get_fd( struct object *obj );
static void pipe_client_destroy( struct object *obj );
static int pipe_client_flush( struct fd *fd, struct event **event );

static const struct object_ops pipe_client_ops =
{
    sizeof(struct pipe_client),   /* size */
    pipe_client_dump,             /* dump */
    default_fd_add_queue,         /* add_queue */
    default_fd_remove_queue,      /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    pipe_client_get_fd,           /* get_fd */
    pipe_client_destroy           /* destroy */
};

static const struct fd_ops pipe_client_fd_ops =
{
    pipe_end_get_poll_events,     /* get_poll_events */
    default_poll_event,           /* poll_event */
    pipe_client_flush,            /* flush */
    pipe_end_get_info,            /* get_file_info */
    no_queue_async                /* queue_async */
};

static void named_pipe_dump( struct object *obj, int verbose )
{
    struct named_pipe *pipe = (struct named_pipe *) obj;
    assert( obj->ops == &named_pipe_ops );
    fprintf( stderr, "named pipe %p\n" ,pipe);
}

static void pipe_server_dump( struct object *obj, int verbose )
{
    struct pipe_server *server = (struct pipe_server *) obj;
    assert( obj->ops == &pipe_server_ops );
    fprintf( stderr, "named pipe server %p (state %d)\n",
             server, server->state );
}

static void pipe_client_dump( struct object *obj, int verbose )
{
    struct pipe_client *client = (struct pipe_client *) obj;
    assert( obj->ops == &pipe_server_ops );
    fprintf( stderr, "named pipe client %p (server state %d)\n",
             client, client->server->state );
}

static void named_pipe_destroy( struct object *obj)
{
    struct named_pipe *pipe = (struct named_pipe *) obj;
    assert( !pipe->servers );
    assert( !pipe->instances );
}

static void notify_waiter( struct wait_info *wait, unsigned int status )
{
    if( wait->thread && wait->func && wait->overlapped )
    {
        /* queue a system APC, to notify a waiting thread */
        thread_queue_apc( wait->thread, NULL, wait->func, APC_ASYNC,
                          1, wait->overlapped, (void *)status, NULL );
    }
    if( wait->thread ) release_object( wait->thread );
    wait->thread = NULL;
}

static void set_waiter( struct wait_info *wait, void *func, void *ov )
{
    wait->thread = (struct thread *) grab_object( current );
    wait->func = func;
    wait->overlapped = ov;
}

static void notify_connect_waiters( struct named_pipe *pipe )
{
    struct connect_wait *cw, **x = &pipe->connect_waiters;

    while( *x )
    {
        cw = *x;
        notify_waiter( &cw->wait, STATUS_SUCCESS );
        release_object( pipe );
        *x = cw->next;
        free( cw );
    }
}

static void queue_connect_waiter( struct named_pipe *pipe,
                                  void *func, void *overlapped )
{
    struct connect_wait *waiter;

    waiter = mem_alloc( sizeof *waiter );
    if( waiter )
    {
        set_waiter( &waiter->wait, func, overlapped );
        waiter->next = pipe->connect_waiters;
        pipe->connect_waiters = waiter;
        grab_object( pipe );
    }
}

static struct fd *pipe_client_get_fd( struct object *obj )
{
    struct pipe_client *client = (struct pipe_client *) obj;
    if( client->fd )
        return (struct fd *) grab_object( client->fd );
    set_error( STATUS_PIPE_DISCONNECTED );
    return NULL;
}

static struct fd *pipe_server_get_fd( struct object *obj )
{
    struct pipe_server *server = (struct pipe_server *) obj;

    switch(server->state)
    {
    case ps_connected_server:
    case ps_wait_disconnect:
        assert( server->fd );
        return (struct fd *) grab_object( server->fd );

    case ps_wait_open:
    case ps_idle_server:
        set_error( STATUS_PIPE_LISTENING );
        break;

    case ps_disconnected_server:
    case ps_wait_connect:
        set_error( STATUS_PIPE_DISCONNECTED );
        break;

    default:
        assert( 0 );
    }
    return NULL;
}


static void notify_empty( struct pipe_server *server )
{
    if( !server->flush_poll )
        return;
    assert( server->state == ps_connected_server );
    assert( server->event );
    remove_timeout_user( server->flush_poll );
    server->flush_poll = NULL;
    set_event( server->event );
    release_object( server->event );
    server->event = NULL;
}

static void do_disconnect( struct pipe_server *server )
{
    /* we may only have a server fd, if the client disconnected */
    if( server->client )
    {
        assert( server->client->server == server );
        assert( server->client->fd );
        release_object( server->client->fd );
        server->client->fd = NULL;
    }
    assert( server->fd );
    release_object( server->fd );
    server->fd = NULL;
}

static void pipe_server_destroy( struct object *obj)
{
    struct pipe_server *server = (struct pipe_server *)obj;

    assert( obj->ops == &pipe_server_ops );

    if( server->fd )
    {
        notify_empty( server );
        do_disconnect( server );
    }

    if( server->client )
    {
        server->client->server = NULL;
        server->client = NULL;
    }

    notify_waiter( &server->wait, STATUS_HANDLES_CLOSED );

    assert( server->pipe->instances );
    server->pipe->instances--;

    /* remove server from pipe's server list */
    if( server->next ) server->next->prev = server->prev;
    if( server->prev ) server->prev->next = server->next;
    else server->pipe->servers = server->next;
    release_object( server->pipe );
}

static void pipe_client_destroy( struct object *obj)
{
    struct pipe_client *client = (struct pipe_client *)obj;
    struct pipe_server *server = client->server;

    assert( obj->ops == &pipe_client_ops );

    notify_waiter( &client->wait, STATUS_HANDLES_CLOSED );

    if( server )
    {
        notify_empty( server );

        switch( server->state )
        {
        case ps_connected_server:
            /* Don't destroy the server's fd here as we can't
               do a successful flush without it. */
            server->state = ps_wait_disconnect;
            release_object( client->fd );
            client->fd = NULL;
            break;
        case ps_disconnected_server:
            server->state = ps_wait_connect;
            break;
        default:
            assert( 0 );
        }
        assert( server->client );
        server->client = NULL;
        client->server = NULL;
    }
    assert( !client->fd );
}

static int pipe_end_get_poll_events( struct fd *fd )
{
    return POLLIN | POLLOUT;  /* FIXME */
}

static int pipe_data_remaining( struct pipe_server *server )
{
    struct pollfd pfd;
    int fd;

    assert( server->client );

    fd = get_unix_fd( server->client->fd );
    if( fd < 0 )
        return 0;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if( 0 > poll( &pfd, 1, 0 ) )
        return 0;
 
    return pfd.revents&POLLIN;
}

static void check_flushed( void *arg )
{
    struct pipe_server *server = (struct pipe_server*) arg;

    assert( server->event );
    if( pipe_data_remaining( server ) )
    {
        struct timeval tv;

        gettimeofday( &tv, 0 );
        add_timeout( &tv, 100 );
        server->flush_poll = add_timeout_user( &tv, check_flushed, server );
    }
    else
        notify_empty( server );
}

static int pipe_server_flush( struct fd *fd, struct event **event )
{
    struct pipe_server *server = get_fd_user( fd );

    if( !server )
        return 0;

    if( server->state != ps_connected_server )
        return 0;

    /* FIXME: if multiple threads flush the same pipe,
              maybe should create a list of processes to notify */
    if( server->flush_poll )
        return 0;

    if( pipe_data_remaining( server ) )
    {
        struct timeval tv;

        /* this kind of sux - 
           there's no unix way to be alerted when a pipe becomes empty */
        server->event = create_event( NULL, 0, 0, 0 );
        if( !server->event )
            return 0;
        gettimeofday( &tv, 0 );
        add_timeout( &tv, 100 );
        server->flush_poll = add_timeout_user( &tv, check_flushed, server );
        *event = server->event;
    }

    return 0; 
}

static int pipe_client_flush( struct fd *fd, struct event **event )
{
    /* FIXME: what do we have to do for this? */
    return 0;
}

static int pipe_end_get_info( struct fd *fd, 
                        struct get_file_info_reply *reply, int *flags )
{
    if (reply)
    {
        reply->type        = FILE_TYPE_PIPE;
        reply->attr        = 0;
        reply->access_time = 0;
        reply->write_time  = 0;
        reply->size_high   = 0;
        reply->size_low    = 0;
        reply->links       = 0;
        reply->index_high  = 0;
        reply->index_low   = 0;
        reply->serial      = 0;
    }
    *flags = 0;
    return FD_TYPE_DEFAULT;
}

static struct named_pipe *create_named_pipe( const WCHAR *name, size_t len )
{
    struct named_pipe *pipe;

    pipe = create_named_object( sync_namespace, &named_pipe_ops, name, len );
    if( pipe )
    {
        if( get_error() != STATUS_OBJECT_NAME_COLLISION )
        {
            /* initialize it if it didn't already exist */
            pipe->servers = 0;
            pipe->instances = 0;
            pipe->connect_waiters = NULL;
        }
    }
    return pipe;
}

static struct named_pipe *open_named_pipe( const WCHAR *name, size_t len )
{
    struct object *obj;

    if ((obj = find_object( sync_namespace, name, len )))
    {
        if (obj->ops == &named_pipe_ops) return (struct named_pipe *)obj;
        release_object( obj );
        set_error( STATUS_OBJECT_TYPE_MISMATCH );
    }
    else set_error( STATUS_OBJECT_NAME_NOT_FOUND );

    return NULL;
}

static struct pipe_server *get_pipe_server_obj( struct process *process,
                                obj_handle_t handle, unsigned int access )
{
    struct object *obj;
    obj = get_handle_obj( process, handle, access, &pipe_server_ops );
    return (struct pipe_server *) obj;
}

static struct pipe_server *create_pipe_server( struct named_pipe *pipe )
{
    struct pipe_server *server;

    server = alloc_object( &pipe_server_ops );
    if( !server )
        return NULL;

    server->fd = NULL;
    server->pipe = pipe;
    server->state = ps_none;
    server->client = NULL;
    server->flush_poll = NULL;
    server->wait.thread = NULL;

    /* add to list of pipe servers */
    if ((server->next = pipe->servers)) server->next->prev = server;
    server->prev = NULL;
    pipe->servers = server;

    grab_object( pipe );

    return server;
}

static struct pipe_client *create_pipe_client( struct pipe_server *server )
{
    struct pipe_client *client;

    client = alloc_object( &pipe_client_ops );
    if( !client )
        return NULL;

    client->fd = NULL;
    client->server = server;
    client->wait.thread = NULL;

    return client;
}

static struct pipe_server *find_server( struct named_pipe *pipe,
                                        enum pipe_state state )
{
    struct pipe_server *x;

    for( x = pipe->servers; x; x = x->next )
        if( x->state == state )
            break;

    if( !x )
        return NULL;

    return (struct pipe_server *) grab_object( x );
}

DECL_HANDLER(create_named_pipe)
{
    struct named_pipe *pipe;
    struct pipe_server *server;

    reply->handle = 0;
    pipe = create_named_pipe( get_req_data(), get_req_data_size() );
    if( !pipe )
        return;

    if( get_error() != STATUS_OBJECT_NAME_COLLISION )
    {
        pipe->insize = req->insize;
        pipe->outsize = req->outsize;
        pipe->maxinstances = req->maxinstances;
        pipe->timeout = req->timeout;
        pipe->pipemode = req->pipemode;
    }
    else
    {
        set_error( 0 );  /* clear the name collision */
        if( pipe->maxinstances <= pipe->instances )
        {
            set_error( STATUS_PIPE_BUSY );
            release_object( pipe );
            return;
        }
        if( ( pipe->maxinstances != req->maxinstances ) ||
            ( pipe->timeout != req->timeout ) ||
            ( pipe->pipemode != req->pipemode ) )
        {
            set_error( STATUS_ACCESS_DENIED );
            release_object( pipe );
            return;
        }
    }

    server = create_pipe_server( pipe );
    if(server)
    {
        server->state = ps_idle_server;
        reply->handle = alloc_handle( current->process, server,
                                      GENERIC_READ|GENERIC_WRITE, 0 );
        server->pipe->instances++;
        release_object( server );
    }

    release_object( pipe );
}

DECL_HANDLER(open_named_pipe)
{
    struct pipe_server *server;
    struct pipe_client *client;
    struct named_pipe *pipe;
    int fds[2];

    reply->handle = 0;

    pipe = open_named_pipe( get_req_data(), get_req_data_size() );
    if ( !pipe )
    {
        set_error( STATUS_NO_SUCH_FILE );
        return;
    }

    for( server = pipe->servers; server; server = server->next )
        if( ( server->state==ps_idle_server ) ||
            ( server->state==ps_wait_open ) )
            break;
    release_object( pipe );

    if ( !server )
    {
        set_error( STATUS_PIPE_NOT_AVAILABLE );
        return;
    }

    client = create_pipe_client( server );
    if( client )
    {
        if( !socketpair( PF_UNIX, SOCK_STREAM, 0, fds ) )
        {
            assert( !client->fd );
            assert( !server->fd );
            client->fd = create_anonymous_fd( &pipe_server_fd_ops,
                                            fds[1], &client->obj );
            server->fd = create_anonymous_fd( &pipe_server_fd_ops,
                                            fds[0], &server->obj );
            if (client->fd && server->fd)
            {
                if( server->state == ps_wait_open )
                    notify_waiter( &server->wait, STATUS_SUCCESS );
                assert( !server->wait.thread );
                server->state = ps_connected_server;
                server->client = client;
                client->server = server;
                reply->handle = alloc_handle( current->process, client,
                                              req->access, req->inherit );
            }
        }
        else
            file_set_error();

        release_object( client );
    }
}

DECL_HANDLER(connect_named_pipe)
{
    struct pipe_server *server;

    server = get_pipe_server_obj(current->process, req->handle, 0);
    if(!server)
        return;

    switch( server->state )
    {
    case ps_idle_server:
    case ps_wait_connect:
        assert( !server->fd );
        server->state = ps_wait_open;
        set_waiter( &server->wait, req->func, req->overlapped );
        notify_connect_waiters( server->pipe );
        break;
    case ps_connected_server:
        assert( server->fd );
        set_error( STATUS_PIPE_CONNECTED );
        break;
    case ps_disconnected_server:
        set_error( STATUS_PIPE_BUSY );
        break;
    case ps_wait_disconnect:
        set_error( STATUS_NO_DATA_DETECTED );
        break;
    default:
        set_error( STATUS_INVALID_HANDLE );
        break;
    }

    release_object(server);
}

DECL_HANDLER(wait_named_pipe)
{
    struct named_pipe *pipe;
    struct pipe_server *server;

    if (!(pipe = open_named_pipe( get_req_data(), get_req_data_size() )))
    {
        set_error( STATUS_PIPE_NOT_AVAILABLE );
        return;
    }
    server = find_server( pipe, ps_wait_open );
    if( server )
    {
        /* there's already a server waiting for a client to connect */
        struct wait_info wait;
        set_waiter( &wait, req->func, req->overlapped );
        notify_waiter( &wait, STATUS_SUCCESS );
        release_object( server );
    }
    else
        queue_connect_waiter( pipe, req->func, req->overlapped );

    release_object( pipe );
}

DECL_HANDLER(disconnect_named_pipe)
{
    struct pipe_server *server;

    reply->fd = -1;
    server = get_pipe_server_obj( current->process, req->handle, 0 );
    if( !server )
        return;
    switch( server->state )
    {
    case ps_connected_server:
        assert( server->fd );
        assert( server->client );
        assert( server->client->fd );

        notify_empty( server );
        notify_waiter( &server->client->wait, STATUS_PIPE_DISCONNECTED );

        /* Dump the client and server fds, but keep the pointers
           around - client loses all waiting data */
        server->state = ps_disconnected_server;
        do_disconnect( server );
        reply->fd = flush_cached_fd( current->process, req->handle );
        break;

    case ps_wait_disconnect:
        assert( !server->client );
        assert( server->fd );
        do_disconnect( server );
        server->state = ps_wait_connect;
        reply->fd = flush_cached_fd( current->process, req->handle );
        break;

    default:
        set_error( STATUS_PIPE_DISCONNECTED );
    }
    release_object( server );
}

DECL_HANDLER(get_named_pipe_info)
{
    struct pipe_server *server;

    server = get_pipe_server_obj( current->process, req->handle, 0 );
    if(!server)
        return;

    reply->flags        = server->pipe->pipemode;
    reply->maxinstances = server->pipe->maxinstances;
    reply->insize       = server->pipe->insize;
    reply->outsize      = server->pipe->outsize;

    release_object(server);
}
