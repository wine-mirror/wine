/*
 * Server-side pipe management
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2001 Mike McCormack
 *
 * TODO:
 *   improve error handling
 */

#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "winbase.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

enum pipe_state
{
    ps_none,
    ps_idle_server,
    ps_wait_open,
    ps_wait_connect,
    ps_connected_server,
    ps_connected_client,
    ps_disconnected
};

struct named_pipe;

struct pipe_user
{
    struct object       obj;
    enum pipe_state     state;
    struct pipe_user   *other;
    struct named_pipe  *pipe;
    struct pipe_user   *next;
    struct pipe_user   *prev;
    struct event       *event;
};

struct named_pipe
{
    struct object       obj;         /* object header */
    unsigned int        pipemode;
    unsigned int        maxinstances;
    unsigned int        outsize;
    unsigned int        insize;
    unsigned int        timeout;
    struct pipe_user   *users;
};

static void named_pipe_dump( struct object *obj, int verbose );
static void named_pipe_destroy( struct object *obj);

static const struct object_ops named_pipe_ops =
{
    sizeof(struct named_pipe),    /* size */
    named_pipe_dump,              /* dump */
    no_add_queue,                 /* add_queue */
    NULL,                         /* remove_queue */
    NULL,                         /* signaled */
    NULL,                         /* satisfied */
    NULL,                         /* get_poll_events */
    NULL,                         /* poll_event */
    no_get_fd,                    /* get_fd */
    no_flush,                     /* flush */
    no_get_file_info,             /* get_file_info */
    named_pipe_destroy            /* destroy */
};

static void pipe_user_dump( struct object *obj, int verbose );
static void pipe_user_destroy( struct object *obj);
static int pipe_user_get_fd( struct object *obj );

static const struct object_ops pipe_user_ops =
{
    sizeof(struct pipe_user),     /* size */
    pipe_user_dump,               /* dump */
    default_poll_add_queue,       /* add_queue */
    default_poll_remove_queue,    /* remove_queue */
    default_poll_signaled,        /* signaled */
    no_satisfied,                 /* satisfied */
    NULL,                         /* get_poll_events */
    default_poll_event,           /* poll_event */
    pipe_user_get_fd,             /* get_fd */
    no_flush,                     /* flush */
    no_get_file_info,             /* get_file_info */
    pipe_user_destroy             /* destroy */
};

static void named_pipe_dump( struct object *obj, int verbose )
{
    struct named_pipe *pipe = (struct named_pipe *)obj;
    assert( obj->ops == &named_pipe_ops );
    fprintf( stderr, "named pipe %p\n" ,pipe);
}

static void pipe_user_dump( struct object *obj, int verbose )
{
    struct pipe_user *user = (struct pipe_user *)obj;
    assert( obj->ops == &pipe_user_ops );
    fprintf( stderr, "named pipe user %p (state %d)\n", user, user->state );
}

static void named_pipe_destroy( struct object *obj)
{
    struct named_pipe *pipe = (struct named_pipe *)obj;
    assert( !pipe->users );
}

static void pipe_user_destroy( struct object *obj)
{
    struct pipe_user *user = (struct pipe_user *)obj;

    assert( obj->ops == &pipe_user_ops );

    if(user->event)
    {
        /* FIXME: signal waiter of failure */
        release_object(user->event);
        user->event = NULL;
    }
    if(user->other)
    {
        close(user->other->obj.fd);
        user->other->obj.fd = -1;
        switch(user->other->state)
        {
        case ps_connected_server:
            user->other->state = ps_idle_server;
            break;
        case ps_connected_client:
            user->other->state = ps_disconnected;
            break;
        default:
            fprintf(stderr,"connected pipe has strange state %d!\n",
                            user->other->state);
        }
        user->other->other=NULL;
        user->other = NULL;
    }

    /* remove user from pipe's user list */
    if (user->next) user->next->prev = user->prev;
    if (user->prev) user->prev->next = user->next;
    else user->pipe->users = user->next;
    release_object(user->pipe);
}

static int pipe_user_get_fd( struct object *obj )
{
    struct pipe_user *user = (struct pipe_user *)obj;
    assert( obj->ops == &pipe_user_ops );
    return user->obj.fd;
}

static struct named_pipe *create_named_pipe( const WCHAR *name, size_t len )
{
    struct named_pipe *pipe;

    if ((pipe = create_named_object( &named_pipe_ops, name, len )))
    {
        if (get_error() != STATUS_OBJECT_NAME_COLLISION)
        {
            /* initialize it if it didn't already exist */
            pipe->users = 0;
        }
    }
    return pipe;
}

static struct pipe_user *get_pipe_user_obj( struct process *process, handle_t handle,
                                            unsigned int access )
{
    return (struct pipe_user *)get_handle_obj( process, handle, access, &pipe_user_ops );
}

static struct pipe_user *create_pipe_user( struct named_pipe *pipe, int fd )
{
    struct pipe_user *user;

    user = alloc_object( &pipe_user_ops, fd );
    if(!user)
        return NULL;

    user->pipe = pipe;
    user->state = ps_none;
    user->event = NULL;   /* thread wait on this pipe */
    user->other = NULL;

    /* add to list of pipe users */
    if ((user->next = pipe->users)) user->next->prev = user;
    user->prev = NULL;
    pipe->users = user;

    grab_object(pipe);

    return user;
}

static struct pipe_user *find_partner(struct named_pipe *pipe, enum pipe_state state)
{
    struct pipe_user *x;

    for(x = pipe->users; x; x=x->next)
    {
        if(x->state==state)
        break;
    }

    if(!x)
        return NULL;

    return (struct pipe_user *)grab_object( x );
}

DECL_HANDLER(create_named_pipe)
{
    struct named_pipe *pipe;
    struct pipe_user *user;

    req->handle = 0;
    pipe = create_named_pipe( get_req_data(req), get_req_data_size(req) );
    if(!pipe)
        return;

    user = create_pipe_user (pipe, -1);

    if(user)
    {
        user->state = ps_idle_server;
        req->handle = alloc_handle( current->process, user, GENERIC_READ|GENERIC_WRITE, 0 );
        release_object( user );
    }

    release_object( pipe );
}

DECL_HANDLER(open_named_pipe)
{
    struct named_pipe *pipe;

    req->handle = 0;
    pipe = create_named_pipe( get_req_data(req), get_req_data_size(req) );
    if(!pipe)
        return;

    if (get_error() == STATUS_OBJECT_NAME_COLLISION)
    {
        struct pipe_user *partner;

        if ((partner = find_partner(pipe, ps_wait_open)))
        {
            int fds[2];

            if(!socketpair(PF_UNIX, SOCK_STREAM, 0, fds))
            {
                struct pipe_user *user;

                if( (user = create_pipe_user (pipe, fds[1])) )
                {
                    partner->obj.fd = fds[0];
                    set_event(partner->event);
                    release_object(partner->event);
                    partner->event = NULL;
                    partner->state = ps_connected_server;
                    partner->other = user;
                    user->state = ps_connected_client;
                    user->other = partner;
                    req->handle = alloc_handle( current->process, user, req->access, 0 );
                    release_object(user);
                }
                else
                {
                    close(fds[0]);
                }
            }
            release_object( partner );
        }
        else
        {
            set_error(STATUS_PIPE_NOT_AVAILABLE);
        }
    }
    else
    {
        set_error(STATUS_NO_SUCH_FILE);
    }

    release_object(pipe);
}

DECL_HANDLER(connect_named_pipe)
{
    struct pipe_user *user, *partner;
    struct event *event;

    user = get_pipe_user_obj(current->process, req->handle, 0);
    if(!user)
        return;

    if( user->state != ps_idle_server )
    {
        set_error(STATUS_PORT_ALREADY_SET);
    }
    else
    {
        user->state = ps_wait_open;
        event = get_event_obj(current->process, req->event, 0);
        if(event)
            user->event = event;

        /* notify all waiters that a pipe just became available */
        while( (partner = find_partner(user->pipe,ps_wait_connect)) )
        {
            set_event(partner->event);
            release_object(partner->event);
            partner->event = NULL;
            release_object(partner);
            release_object(partner);
        }
    }

    release_object(user);
}

DECL_HANDLER(wait_named_pipe)
{
    struct event *event;
    struct named_pipe *pipe;

    event = get_event_obj(current->process, req->event, 0);
    if(!event)
        return;

    pipe = create_named_pipe( get_req_data(req), get_req_data_size(req) );
    if( pipe )
    {
        /* only wait if the pipe already exists */
        if(get_error() == STATUS_OBJECT_NAME_COLLISION)
        {
            struct pipe_user *partner;

            set_error(STATUS_SUCCESS);
            if( (partner = find_partner(pipe,ps_wait_open)) )
            {
                set_event(event);
                release_object(partner);
            }
            else
            {
                struct pipe_user *user;

                if( (user = create_pipe_user (pipe, -1)) )
                {
                    user->event = (struct event *)grab_object( event );
                    user->state = ps_wait_connect;
                    /* don't release it */
                }
            }
        }
        else
        {
            set_error(STATUS_PIPE_NOT_AVAILABLE);
        }
        release_object(pipe);
    }
    release_object(event);
}

DECL_HANDLER(disconnect_named_pipe)
{
    struct pipe_user *user;

    user = get_pipe_user_obj(current->process, req->handle, 0);
    if(!user)
        return;
    if( (user->state == ps_connected_server) &&
        (user->other->state == ps_connected_client) )
    {
        close(user->other->obj.fd);
        user->other->obj.fd = -1;
        user->other->state = ps_disconnected;
        user->other->other = NULL;

        close(user->obj.fd);
        user->obj.fd = -1;
        user->state = ps_idle_server;
        user->other = NULL;
    }
    release_object(user);
}
