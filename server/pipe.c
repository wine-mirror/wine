/*
 * Server-side pipe management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "winerror.h"
#include "winbase.h"

#include "handle.h"
#include "thread.h"

enum side { READ_SIDE, WRITE_SIDE };

struct pipe
{
    struct object       obj;         /* object header */
    struct pipe        *other;       /* the pipe other end */
    struct select_user  select;      /* select user */
    enum side           side;        /* which side of the pipe is this */
};

static void pipe_dump( struct object *obj, int verbose );
static int pipe_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void pipe_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int pipe_signaled( struct object *obj, struct thread *thread );
static int pipe_get_read_fd( struct object *obj );
static int pipe_get_write_fd( struct object *obj );
static int pipe_get_info( struct object *obj, struct get_file_info_reply *reply );
static void pipe_destroy( struct object *obj );

static const struct object_ops pipe_ops =
{
    pipe_dump,
    pipe_add_queue,
    pipe_remove_queue,
    pipe_signaled,
    no_satisfied,
    pipe_get_read_fd,
    pipe_get_write_fd,
    no_flush,
    pipe_get_info,
    pipe_destroy
};


static int create_pipe( struct object *obj[2] )
{
    struct pipe *newpipe[2];
    int fd[2];

    if (pipe( fd ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (!(newpipe[0] = mem_alloc( sizeof(struct pipe) )))
    {
        close( fd[0] );
        close( fd[1] );
        return 0;
    }
    if (!(newpipe[1] = mem_alloc( sizeof(struct pipe) )))
    {
        close( fd[0] );
        close( fd[1] );
        free( newpipe[0] );
        return 0;
    }
    init_object( &newpipe[0]->obj, &pipe_ops, NULL );
    init_object( &newpipe[1]->obj, &pipe_ops, NULL );
    newpipe[0]->select.fd      = fd[0];
    newpipe[0]->select.func    = default_select_event;
    newpipe[0]->select.private = newpipe[0];
    newpipe[0]->other          = newpipe[1];
    newpipe[0]->side           = READ_SIDE;
    newpipe[1]->select.fd      = fd[1];
    newpipe[1]->select.func    = default_select_event;
    newpipe[1]->select.private = newpipe[1];
    newpipe[1]->other          = newpipe[0];
    newpipe[1]->side           = WRITE_SIDE;
    obj[0] = &newpipe[0]->obj;
    obj[1] = &newpipe[1]->obj;
    register_select_user( &newpipe[0]->select );
    register_select_user( &newpipe[1]->select );
    CLEAR_ERROR();
    return 1;
}

static void pipe_dump( struct object *obj, int verbose )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );
    fprintf( stderr, "Pipe %s-side fd=%d\n",
             (pipe->side == READ_SIDE) ? "read" : "write", pipe->select.fd );
}

static int pipe_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );
    if (!obj->head)  /* first on the queue */
        set_select_events( &pipe->select,
                           (pipe->side == READ_SIDE) ? READ_EVENT : WRITE_EVENT );
    add_queue( obj, entry );
    return 1;
}

static void pipe_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct pipe *pipe = (struct pipe *)grab_object(obj);
    assert( obj->ops == &pipe_ops );

    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        set_select_events( &pipe->select, 0 );
    release_object( obj );
}

static int pipe_signaled( struct object *obj, struct thread *thread )
{
    int event;
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    event = (pipe->side == READ_SIDE) ? READ_EVENT : WRITE_EVENT;
    if (check_select_events( &pipe->select, event ))
    {
        /* stop waiting on select() if we are signaled */
        set_select_events( &pipe->select, 0 );
        return 1;
    }
    else
    {
        /* restart waiting on select() if we are no longer signaled */
        if (obj->head) set_select_events( &pipe->select, event );
        return 0;
    }
}

static int pipe_get_read_fd( struct object *obj )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    if (!pipe->other)
    {
        SET_ERROR( ERROR_BROKEN_PIPE );
        return -1;
    }
    if (pipe->side != READ_SIDE)  /* FIXME: should not be necessary */
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( pipe->select.fd );
}

static int pipe_get_write_fd( struct object *obj )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    if (!pipe->other)
    {
        SET_ERROR( ERROR_BROKEN_PIPE );
        return -1;
    }
    if (pipe->side != WRITE_SIDE)  /* FIXME: should not be necessary */
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( pipe->select.fd );
}

static int pipe_get_info( struct object *obj, struct get_file_info_reply *reply )
{
    memset( reply, 0, sizeof(*reply) );
    reply->type = FILE_TYPE_PIPE;
    return 1;
}

static void pipe_destroy( struct object *obj )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    if (pipe->other) pipe->other->other = NULL;
    unregister_select_user( &pipe->select );
    close( pipe->select.fd );
    free( pipe );
}

/* create an anonymous pipe */
DECL_HANDLER(create_pipe)
{
    struct create_pipe_reply reply = { -1, -1 };
    struct object *obj[2];
    if (create_pipe( obj ))
    {
        reply.handle_read = alloc_handle( current->process, obj[0],
                                          STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_READ,
                                          req->inherit );
        if (reply.handle_read != -1)
        {
            reply.handle_write = alloc_handle( current->process, obj[1],
                                               STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_WRITE,
                                               req->inherit );
            if (reply.handle_write == -1)
                close_handle( current->process, reply.handle_read );
        }
        release_object( obj[0] );
        release_object( obj[1] );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}
