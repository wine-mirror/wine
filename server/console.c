/*
 * Server-side console management
 *
 * Copyright (C) 1998 Alexandre Julliard
 *
 * FIXME: all this stuff is a hack to avoid breaking
 *        the client-side console support.
 */

#include <assert.h>
#include <fcntl.h>
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

struct console
{
    struct object  obj;             /* object header */
    int            fd;              /* Unix file descriptor */
    int            is_read;         /* is this the read or write part? */
};

static void console_dump( struct object *obj, int verbose );
static int console_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void console_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int console_signaled( struct object *obj, struct thread *thread );
static int console_get_read_fd( struct object *obj );
static int console_get_write_fd( struct object *obj );
static void console_destroy( struct object *obj );

static const struct object_ops console_ops =
{
    console_dump,
    console_add_queue,
    console_remove_queue,
    console_signaled,
    no_satisfied,
    console_get_read_fd,
    console_get_write_fd,
    no_flush,
    console_destroy
};

static const struct select_ops select_ops =
{
    default_select_event,
    NULL   /* we never set a timeout on a console */
};

int create_console( int fd, struct object *obj[2] )
{
    struct console *console_read, *console_write;
    int read_fd, write_fd;

    if ((read_fd = (fd != -1) ? dup(fd) : dup(0)) == -1)
    {
        file_set_error();
        return 0;
    }
    if ((write_fd = (fd != -1) ? dup(fd) : dup(0)) == -1)
    {
        file_set_error();
        close( read_fd );
        return 0;
    }
    if (!(console_read = mem_alloc( sizeof(struct console) )))
    {
        close( read_fd );
        close( write_fd );
        return 0;
    }
    if (!(console_write = mem_alloc( sizeof(struct console) )))
    {
        close( read_fd );
        close( write_fd );
        free( console_read );
        return 0;
    }
    init_object( &console_read->obj, &console_ops, NULL );
    init_object( &console_write->obj, &console_ops, NULL );
    console_read->fd       = read_fd;
    console_read->is_read  = 1;
    console_write->fd      = write_fd;
    console_write->is_read = 0;
    CLEAR_ERROR();
    obj[0] = &console_read->obj;
    obj[1] = &console_write->obj;
    return 1;
}

int set_console_fd( int handle, int fd )
{
    struct console *console;

    if (!(console = (struct console *)get_handle_obj( current->process, handle,
                                                      0, &console_ops )))
        return 0;
    if ((fd = dup(fd)) == -1)
    {
        file_set_error();
        release_object( console );
        return 0;
    }
    close( console->fd );
    console->fd = fd;
    release_object( console );
    return 1;
}

static void console_dump( struct object *obj, int verbose )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );
    printf( "Console %s fd=%d\n",
            console->is_read ? "input" : "output", console->fd );
}

static int console_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );
    if (!obj->head)  /* first on the queue */
    {
        if (!add_select_user( console->fd,
                              console->is_read ? READ_EVENT : WRITE_EVENT,
                              &select_ops, console ))
        {
            SET_ERROR( ERROR_OUTOFMEMORY );
            return 0;
        }
    }
    add_queue( obj, entry );
    return 1;
}

static void console_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct console *console = (struct console *)grab_object(obj);
    assert( obj->ops == &console_ops );

    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        remove_select_user( console->fd );
    release_object( obj );
}

static int console_signaled( struct object *obj, struct thread *thread )
{
    fd_set fds;
    struct timeval tv = { 0, 0 };
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );

    FD_ZERO( &fds );
    FD_SET( console->fd, &fds );
    if (console->is_read)
        return select( console->fd + 1, &fds, NULL, NULL, &tv ) > 0;
    else
        return select( console->fd + 1, NULL, &fds, NULL, &tv ) > 0;
}

static int console_get_read_fd( struct object *obj )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );

    if (!console->is_read)
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( console->fd );
}

static int console_get_write_fd( struct object *obj )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );

    if (console->is_read)
    {
        SET_ERROR( ERROR_ACCESS_DENIED );
        return -1;
    }
    return dup( console->fd );
}

static void console_destroy( struct object *obj )
{
    struct console *console = (struct console *)obj;
    assert( obj->ops == &console_ops );
    close( console->fd );
    free( console );
}
