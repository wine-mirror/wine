/*
 * Server-side pipe management
 *
 * Copyright (C) 1998 Alexandre Julliard
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

struct named_pipe;

struct pipe_user
{
    struct object       obj;
    int                 other_fd;
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
    fprintf( stderr, "named pipe user %p (%s)\n", user,
             (user->other_fd != -1) ? "server" : "client" );
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
    int fds[2];

    if(fd == -1)
    {
        /* FIXME: what about messages? */

        if(0>socketpair(PF_UNIX, SOCK_STREAM, 0, fds)) goto error;
    }
    else
    {
        if ((fds[0] = dup(fd)) == -1) goto error;
        fds[1] = -1;
    }
    user = alloc_object( &pipe_user_ops, fds[0] );
    if(!user)
    {
        if (fds[1] != -1) close( fds[1] );
        return NULL;
    }

    user->pipe = pipe;
    user->other_fd = fds[1];
    user->event = NULL;   /* thread wait on this pipe */

    /* add to list of pipe users */
    if ((user->next = pipe->users)) user->next->prev = user;
    user->prev = NULL;
    pipe->users = user;

    grab_object(pipe);

    return user;

 error:
    file_set_error();
    return NULL;
}

static struct pipe_user *find_partner(struct named_pipe *pipe)
{
    struct pipe_user *x;

    for(x = pipe->users; x; x=x->next)
    {
        /* only pair threads that are waiting */
        if(!x->event)
            continue;

        /* only pair with pipes that haven't been connected */
        if(x->other_fd == -1)
            continue;

        break;
    }

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
        req->handle = alloc_handle( current->process, user, GENERIC_READ|GENERIC_WRITE, 0 );
        release_object( user );
    }

    release_object( pipe );
}

DECL_HANDLER(open_named_pipe)
{
    struct named_pipe *pipe;
    struct pipe_user *user,*partner;

    req->handle = 0;
    pipe = create_named_pipe( get_req_data(req), get_req_data_size(req) );
    if(!pipe)
        return;

    if (get_error() == STATUS_OBJECT_NAME_COLLISION)
    {
        if ((partner = find_partner(pipe)))
        {
            user = create_pipe_user (pipe, partner->other_fd);
            if(user)
            {
                set_event(partner->event);
                release_object(partner->event);
                partner->event = NULL;
                close( partner->other_fd );
                partner->other_fd = -1;
                req->handle = alloc_handle( current->process, user, req->access, 0 );
                release_object(user);
            }
            release_object( partner );
        }
        else {
            set_error(STATUS_NO_SUCH_FILE);
        }
    }
    else {
        set_error(STATUS_NO_SUCH_FILE);
    }

    release_object(pipe);
}

DECL_HANDLER(connect_named_pipe)
{
    struct pipe_user *user;
    struct event *event;

    user = get_pipe_user_obj(current->process, req->handle, 0);
    if(!user)
        return;

    if( user->event || user->other_fd == -1)
    {
        /* fprintf(stderr,"fd = %x  event = %p\n",user->obj.fd,user->event);*/
        set_error(STATUS_PORT_ALREADY_SET);
    }
    else
    {
        event = get_event_obj(current->process, req->event, 0);
        if(event)
            user->event = event;
    }

    release_object(user);
}
