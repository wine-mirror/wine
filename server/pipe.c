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
#include "winnt.h"
#include "server/thread.h"

enum side { READ_SIDE, WRITE_SIDE };

struct pipe
{
    struct object obj;             /* object header */
    struct pipe  *other;           /* the pipe other end */
    int           fd;              /* Unix file descriptor */
    enum side     side;            /* which side of the pipe is this */
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

static const struct select_ops select_ops =
{
    default_select_event,
    NULL   /* we never set a timeout on a pipe */
};

int create_pipe( struct object *obj[2] )
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
    newpipe[0]->fd    = fd[0];
    newpipe[0]->other = newpipe[1];
    newpipe[0]->side  = READ_SIDE;
    newpipe[1]->fd    = fd[1];
    newpipe[1]->other = newpipe[0];
    newpipe[1]->side  = WRITE_SIDE;
    obj[0] = &newpipe[0]->obj;
    obj[1] = &newpipe[1]->obj;
    CLEAR_ERROR();
    return 1;
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
    {
        if (!add_select_user( pipe->fd,
                              (pipe->side == READ_SIDE) ? READ_EVENT : WRITE_EVENT,
                              &select_ops, pipe ))
        {
            SET_ERROR( ERROR_OUTOFMEMORY );
            return 0;
        }
    }
    add_queue( obj, entry );
    return 1;
}

static void pipe_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct pipe *pipe = (struct pipe *)grab_object(obj);
    assert( obj->ops == &pipe_ops );

    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        remove_select_user( pipe->fd );
    release_object( obj );
}

static int pipe_signaled( struct object *obj, struct thread *thread )
{
    struct pipe *pipe = (struct pipe *)obj;
    struct timeval tv = { 0, 0 };
    fd_set fds;

    assert( obj->ops == &pipe_ops );
    FD_ZERO( &fds );
    FD_SET( pipe->fd, &fds );
    if (pipe->side == READ_SIDE)
        return select( pipe->fd + 1, &fds, NULL, NULL, &tv ) > 0;
    else
        return select( pipe->fd + 1, NULL, &fds, NULL, &tv ) > 0;
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
    return dup( pipe->fd );
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
    return dup( pipe->fd );
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
    close( pipe->fd );
    free( pipe );
}
