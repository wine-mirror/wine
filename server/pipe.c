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
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "winerror.h"
#include "winbase.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

enum side { READ_SIDE, WRITE_SIDE };

struct pipe
{
    struct object       obj;         /* object header */
    struct pipe        *other;       /* the pipe other end */
    int                 fd;          /* file descriptor */
    int                 select;      /* select user id */
    enum side           side;        /* which side of the pipe is this */
};

static void pipe_dump( struct object *obj, int verbose );
static int pipe_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void pipe_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int pipe_signaled( struct object *obj, struct thread *thread );
static int pipe_get_read_fd( struct object *obj );
static int pipe_get_write_fd( struct object *obj );
static int pipe_get_info( struct object *obj, struct get_file_info_request *req );
static void pipe_destroy( struct object *obj );

static const struct object_ops pipe_ops =
{
    sizeof(struct pipe),
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


static struct pipe *create_pipe_side( int fd, int side )
{
    struct pipe *pipe;

    if ((pipe = alloc_object( &pipe_ops )))
    {
        pipe->fd     = fd;
        pipe->other  = NULL;
        pipe->side   = side;
        if ((pipe->select = add_select_user( fd, default_select_event, pipe )) == -1)
        {
            release_object( pipe );
            pipe = NULL;
        }
    }
    return pipe;
}

static int create_pipe( struct object *obj[2] )
{
    struct pipe *read_pipe;
    struct pipe *write_pipe;
    int fd[2];

    if (pipe( fd ) == -1)
    {
        file_set_error();
        return 0;
    }
    if ((read_pipe = create_pipe_side( fd[0], READ_SIDE )))
    {
        if ((write_pipe = create_pipe_side( fd[1], WRITE_SIDE )))
        {
            write_pipe->other = read_pipe;
            read_pipe->other  = write_pipe;
            obj[0] = &read_pipe->obj;
            obj[1] = &write_pipe->obj;
            return 1;
        }
        release_object( read_pipe );
    }
    close( fd[0] );
    close( fd[1] );
    return 0;
}

static void pipe_dump( struct object *obj, int verbose )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );
    fprintf( stderr, "Pipe %s-side fd=%d\n",
             (pipe->side == READ_SIDE) ? "read" : "write", pipe->fd );
}

static int pipe_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );
    if (!obj->head)  /* first on the queue */
        set_select_events( pipe->select, (pipe->side == READ_SIDE) ? POLLIN : POLLOUT );
    add_queue( obj, entry );
    return 1;
}

static void pipe_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct pipe *pipe = (struct pipe *)grab_object(obj);
    assert( obj->ops == &pipe_ops );

    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        set_select_events( pipe->select, 0 );
    release_object( obj );
}

static int pipe_signaled( struct object *obj, struct thread *thread )
{
    int event;
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    event = (pipe->side == READ_SIDE) ? POLLIN : POLLOUT;
    if (check_select_events( pipe->fd, event ))
    {
        /* stop waiting on select() if we are signaled */
        set_select_events( pipe->select, 0 );
        return 1;
    }
    else
    {
        /* restart waiting on select() if we are no longer signaled */
        if (obj->head) set_select_events( pipe->select, event );
        return 0;
    }
}

static int pipe_get_read_fd( struct object *obj )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    if (!pipe->other)
    {
        set_error( ERROR_BROKEN_PIPE );
        return -1;
    }
    if (pipe->side != READ_SIDE)  /* FIXME: should not be necessary */
    {
        set_error( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( pipe->fd );
}

static int pipe_get_write_fd( struct object *obj )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    if (!pipe->other)
    {
        set_error( ERROR_BROKEN_PIPE );
        return -1;
    }
    if (pipe->side != WRITE_SIDE)  /* FIXME: should not be necessary */
    {
        set_error( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( pipe->fd );
}

static int pipe_get_info( struct object *obj, struct get_file_info_request *req )
{
    req->type        = FILE_TYPE_PIPE;
    req->attr        = 0;
    req->access_time = 0;
    req->write_time  = 0;
    req->size_high   = 0;
    req->size_low    = 0;
    req->links       = 0;
    req->index_high  = 0;
    req->index_low   = 0;
    req->serial      = 0;
    return 1;
}

static void pipe_destroy( struct object *obj )
{
    struct pipe *pipe = (struct pipe *)obj;
    assert( obj->ops == &pipe_ops );

    if (pipe->other) pipe->other->other = NULL;
    remove_select_user( pipe->select );
}

/* create an anonymous pipe */
DECL_HANDLER(create_pipe)
{
    struct object *obj[2];
    int hread = -1, hwrite = -1;

    if (create_pipe( obj ))
    {
        hread = alloc_handle( current->process, obj[0],
                              STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_READ,
                              req->inherit );
        if (hread != -1)
        {
            hwrite = alloc_handle( current->process, obj[1],
                                   STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|GENERIC_WRITE,
                                   req->inherit );
            if (hwrite == -1)
                close_handle( current->process, hread );
        }
        release_object( obj[0] );
        release_object( obj[1] );
    }
    req->handle_read  = hread;
    req->handle_write = hwrite;
}
