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
 *   improve error handling
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
    struct thread      *thread;
    void               *func;
    void               *overlapped;
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
    NULL,                         /* queue_async */
    named_pipe_destroy            /* destroy */
};

static void pipe_user_dump( struct object *obj, int verbose );
static void pipe_user_destroy( struct object *obj);
static int pipe_user_get_fd( struct object *obj );
static int pipe_user_get_info( struct object *obj, struct get_file_info_reply *reply, int *flags );

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
    pipe_user_get_info,           /* get_file_info */
    NULL,                         /* queue_async */
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

static void notify_waiter( struct pipe_user *user, unsigned int status)
{
    if(user->thread && user->func && user->overlapped)
    {
        /* queue a system APC, to notify a waiting thread */
        thread_queue_apc(user->thread,NULL,user->func,
            APC_ASYNC,1,2,user->overlapped,status);
    }
    if (user->thread) release_object(user->thread);
    user->thread = NULL;
    user->func = NULL;
    user->overlapped=NULL;
}

static void pipe_user_destroy( struct object *obj)
{
    struct pipe_user *user = (struct pipe_user *)obj;

    assert( obj->ops == &pipe_user_ops );

    if(user->overlapped)
        notify_waiter(user,STATUS_HANDLES_CLOSED);

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
    if (user->thread) release_object(user->thread);
    release_object(user->pipe);
}

static int pipe_user_get_fd( struct object *obj )
{
    struct pipe_user *user = (struct pipe_user *)obj;
    assert( obj->ops == &pipe_user_ops );
    return user->obj.fd;
}

static int pipe_user_get_info( struct object *obj, struct get_file_info_reply *reply, int *flags )
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

    if ((pipe = create_named_object( sync_namespace, &named_pipe_ops, name, len )))
    {
        if (get_error() != STATUS_OBJECT_NAME_COLLISION)
        {
            /* initialize it if it didn't already exist */
            pipe->users = 0;
        }
    }
    return pipe;
}

static struct pipe_user *get_pipe_user_obj( struct process *process, obj_handle_t handle,
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
    user->other = NULL;
    user->thread = NULL;
    user->func = NULL;
    user->overlapped = NULL;

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

    reply->handle = 0;
    pipe = create_named_pipe( get_req_data(), get_req_data_size() );
    if(!pipe)
        return;

    if (get_error() != STATUS_OBJECT_NAME_COLLISION)
    {
        pipe->insize = req->insize;
        pipe->outsize = req->outsize;
        pipe->maxinstances = req->maxinstances;
        pipe->timeout = req->timeout;
        pipe->pipemode = req->pipemode;
    }

    user = create_pipe_user (pipe, -1);

    if(user)
    {
        user->state = ps_idle_server;
        reply->handle = alloc_handle( current->process, user, GENERIC_READ|GENERIC_WRITE, 0 );
        release_object( user );
    }

    release_object( pipe );
}

DECL_HANDLER(open_named_pipe)
{
    struct named_pipe *pipe;

    reply->handle = 0;
    pipe = create_named_pipe( get_req_data(), get_req_data_size() );
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
                    notify_waiter(partner,STATUS_SUCCESS);
                    partner->state = ps_connected_server;
                    partner->other = user;
                    user->state = ps_connected_client;
                    user->other = partner;
                    reply->handle = alloc_handle( current->process, user, req->access, 0 );
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
        user->thread = (struct thread *)grab_object(current);
        user->func = req->func;
        user->overlapped = req->overlapped;

        /* notify all waiters that a pipe just became available */
        while( (partner = find_partner(user->pipe,ps_wait_connect)) )
        {
            notify_waiter(partner,STATUS_SUCCESS);
            release_object(partner);
            release_object(partner);
        }
    }

    release_object(user);
}

DECL_HANDLER(wait_named_pipe)
{
    struct named_pipe *pipe;

    pipe = create_named_pipe( get_req_data(), get_req_data_size() );
    if( pipe )
    {
        /* only wait if the pipe already exists */
        if(get_error() == STATUS_OBJECT_NAME_COLLISION)
        {
            struct pipe_user *partner;

            set_error(STATUS_SUCCESS);
            if( (partner = find_partner(pipe,ps_wait_open)) )
            {
                /* this should use notify_waiter,
                   but no pipe_user object exists now... */
                thread_queue_apc(current,NULL,req->func,
                    APC_ASYNC,1,2,req->overlapped,STATUS_SUCCESS);
                release_object(partner);
            }
            else
            {
                struct pipe_user *user;

                if( (user = create_pipe_user (pipe, -1)) )
                {
                    user->state = ps_wait_connect;
                    user->thread = (struct thread *)grab_object(current);
                    user->func = req->func;
                    user->overlapped = req->overlapped;
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

DECL_HANDLER(get_named_pipe_info)
{
    struct pipe_user *user;

    user = get_pipe_user_obj(current->process, req->handle, 0);
    if(!user)
        return;

    reply->flags        = user->pipe->pipemode;
    reply->maxinstances = user->pipe->maxinstances;
    reply->insize       = user->pipe->insize;
    reply->outsize      = user->pipe->outsize;

    release_object(user);
}

