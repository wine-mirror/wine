/*
 * Server-side file descriptor management
 *
 * Copyright (C) 2003 Alexandre Julliard
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
 */


#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "object.h"
#include "file.h"
#include "handle.h"
#include "process.h"
#include "request.h"
#include "console.h"

struct fd
{
    struct object        obj;        /* object header */
    const struct fd_ops *fd_ops;     /* file descriptor operations */
    struct object       *user;       /* object using this file descriptor */
    int                  unix_fd;    /* unix file descriptor */
    int                  mode;       /* file protection mode */
};

static void fd_dump( struct object *obj, int verbose );
static void fd_destroy( struct object *obj );

static const struct object_ops fd_ops =
{
    sizeof(struct fd),        /* size */
    fd_dump,                  /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    default_get_fd,           /* get_fd */
    fd_destroy                /* destroy */
};


static void fd_dump( struct object *obj, int verbose )
{
    struct fd *fd = (struct fd *)obj;
    fprintf( stderr, "Fd unix_fd=%d mode=%06o user=%p\n", fd->unix_fd, fd->mode, fd->user );
}

static void fd_destroy( struct object *obj )
{
#if 0
    struct fd *fd = (struct fd *)obj;
    close( fd->unix_fd );
#endif
}

/* allocate an object that has an associated fd */
void *alloc_fd_object( const struct object_ops *ops,
                       const struct fd_ops *fd_user_ops, int unix_fd )
{
    struct object *user;
    struct fd *fd = alloc_object( &fd_ops, -1 );

    if (!fd) return NULL;
    if (!(user = alloc_object( ops, unix_fd )))
    {
        release_object( fd );
        return NULL;
    }
    fd->fd_ops   = fd_user_ops;
    fd->user     = user;
    fd->unix_fd  = unix_fd;
    fd->mode     = 0;

    user->fd_obj = fd;
    return user;
}

/* retrieve the object that is using an fd */
void *get_fd_user( struct fd *fd )
{
    return fd->user;
}

/* retrieve the unix fd for an object */
int get_unix_fd( struct fd *fd )
{
    if (fd) return fd->unix_fd;
    return -1;
}

/* set the unix fd for an object; can only be done once */
void set_unix_fd( struct object *obj, int unix_fd )
{
    struct fd *fd = obj->fd_obj;

    assert( fd );
    assert( fd->unix_fd == -1 );

    fd->unix_fd = unix_fd;
    obj->fd = unix_fd;
}

/* callback for event happening in the main poll() loop */
void fd_poll_event( struct object *obj, int event )
{
    struct fd *fd = obj->fd_obj;
    return fd->fd_ops->poll_event( fd, event );
}

/* check if events are pending and if yes return which one(s) */
int check_fd_events( struct fd *fd, int events )
{
    struct pollfd pfd;

    pfd.fd     = fd->unix_fd;
    pfd.events = events;
    if (poll( &pfd, 1, 0 ) <= 0) return 0;
    return pfd.revents;
}

/* default add_queue() routine for objects that poll() on an fd */
int default_fd_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct fd *fd = obj->fd_obj;

    if (!obj->head)  /* first on the queue */
        set_select_events( obj, fd->fd_ops->get_poll_events( fd ) );
    add_queue( obj, entry );
    return 1;
}

/* default remove_queue() routine for objects that poll() on an fd */
void default_fd_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    grab_object( obj );
    remove_queue( obj, entry );
    if (!obj->head)  /* last on the queue is gone */
        set_select_events( obj, 0 );
    release_object( obj );
}

/* default signaled() routine for objects that poll() on an fd */
int default_fd_signaled( struct object *obj, struct thread *thread )
{
    struct fd *fd = obj->fd_obj;
    int events = fd->fd_ops->get_poll_events( fd );

    if (check_fd_events( fd, events ))
    {
        /* stop waiting on select() if we are signaled */
        set_select_events( obj, 0 );
        return 1;
    }
    /* restart waiting on select() if we are no longer signaled */
    if (obj->head) set_select_events( obj, events );
    return 0;
}

/* default handler for poll() events */
void default_poll_event( struct fd *fd, int event )
{
    /* an error occurred, stop polling this fd to avoid busy-looping */
    if (event & (POLLERR | POLLHUP)) set_select_events( fd->user, -1 );
    wake_up( fd->user, 0 );
}

/* default flush() routine */
int no_flush( struct fd *fd )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    return 0;
}

/* default get_file_info() routine */
int no_get_file_info( struct fd *fd, struct get_file_info_reply *info, int *flags )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
    *flags = 0;
    return FD_TYPE_INVALID;
}

/* default queue_async() routine */
void no_queue_async( struct fd *fd, void* ptr, unsigned int status, int type, int count )
{
    set_error( STATUS_OBJECT_TYPE_MISMATCH );
}

/* same as get_handle_obj but retrieve the struct fd associated to the object */
static struct fd *get_handle_fd_obj( struct process *process, obj_handle_t handle,
                                     unsigned int access )
{
    struct fd *fd = NULL;
    struct object *obj;

    if ((obj = get_handle_obj( process, handle, access, NULL )))
    {
        if (obj->fd_obj) fd = (struct fd *)grab_object( obj->fd_obj );
        else set_error( STATUS_OBJECT_TYPE_MISMATCH );
        release_object( obj );
    }
    return fd;
}

/* flush a file buffers */
DECL_HANDLER(flush_file)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

    if (fd)
    {
        fd->fd_ops->flush( fd );
        release_object( fd );
    }
}

/* get a Unix fd to access a file */
DECL_HANDLER(get_handle_fd)
{
    struct fd *fd;

    reply->fd = -1;
    reply->type = FD_TYPE_INVALID;

    if ((fd = get_handle_fd_obj( current->process, req->handle, req->access )))
    {
        int unix_fd = get_handle_unix_fd( current->process, req->handle, req->access );
        if (unix_fd != -1) reply->fd = unix_fd;
        else if (!get_error())
        {
            unix_fd = fd->unix_fd;
            if (unix_fd != -1) send_client_fd( current->process, unix_fd, req->handle );
        }
        reply->type = fd->fd_ops->get_file_info( fd, NULL, &reply->flags );
        release_object( fd );
    }
    else  /* check for console handle (FIXME: should be done in the client) */
    {
        struct object *obj;

        if ((obj = get_handle_obj( current->process, req->handle, req->access, NULL )))
        {
            if (is_console_object( obj )) reply->type = FD_TYPE_CONSOLE;
            release_object( obj );
        }
    }
}

/* get a file information */
DECL_HANDLER(get_file_info)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

    if (fd)
    {
        int flags;
        fd->fd_ops->get_file_info( fd, reply, &flags );
        release_object( fd );
    }
}

/* create / reschedule an async I/O */
DECL_HANDLER(register_async)
{
    struct fd *fd = get_handle_fd_obj( current->process, req->handle, 0 );

/*
 * The queue_async method must do the following:
 *
 * 1. Get the async_queue for the request of given type.
 * 2. Call find_async() to look for the specific client request in the queue (=> NULL if not found).
 * 3. If status is STATUS_PENDING:
 *      a) If no async request found in step 2 (new request): call create_async() to initialize one.
 *      b) Set request's status to STATUS_PENDING.
 *      c) If the "queue" field of the async request is NULL: call async_insert() to put it into the queue.
 *    Otherwise:
 *      If the async request was found in step 2, destroy it by calling destroy_async().
 * 4. Carry out any operations necessary to adjust the object's poll events
 *    Usually: set_elect_events (obj, obj->ops->get_poll_events()).
 *
 * See also the implementations in file.c, serial.c, and sock.c.
*/

    if (fd)
    {
        fd->fd_ops->queue_async( fd, req->overlapped, req->status, req->type, req->count );
        release_object( fd );
    }
}
