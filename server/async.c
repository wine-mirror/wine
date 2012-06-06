/*
 * Server-side async I/O support
 *
 * Copyright (C) 2007 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "object.h"
#include "file.h"
#include "request.h"

struct async
{
    struct object        obj;             /* object header */
    struct thread       *thread;          /* owning thread */
    struct list          queue_entry;     /* entry in async queue list */
    struct async_queue  *queue;           /* queue containing this async */
    unsigned int         status;          /* current status */
    struct timeout_user *timeout;
    unsigned int         timeout_status;  /* status to report upon timeout */
    struct event        *event;
    async_data_t         data;            /* data for async I/O call */
};

static void async_dump( struct object *obj, int verbose );
static void async_destroy( struct object *obj );

static const struct object_ops async_ops =
{
    sizeof(struct async),      /* size */
    async_dump,                /* dump */
    no_get_type,               /* get_type */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    no_map_access,             /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_lookup_name,            /* lookup_name */
    no_open_file,              /* open_file */
    no_close_handle,           /* close_handle */
    async_destroy              /* destroy */
};


struct async_queue
{
    struct object        obj;             /* object header */
    struct fd           *fd;              /* file descriptor owning this queue */
    struct completion   *completion;      /* completion associated with a recently closed file descriptor */
    apc_param_t          comp_key;        /* completion key associated with a recently closed file descriptor */
    struct list          queue;           /* queue of async objects */
};

static void async_queue_dump( struct object *obj, int verbose );
static void async_queue_destroy( struct object *obj );

static const struct object_ops async_queue_ops =
{
    sizeof(struct async_queue),      /* size */
    async_queue_dump,                /* dump */
    no_get_type,                     /* get_type */
    no_add_queue,                    /* add_queue */
    NULL,                            /* remove_queue */
    NULL,                            /* signaled */
    NULL,                            /* satisfied */
    no_signal,                       /* signal */
    no_get_fd,                       /* get_fd */
    no_map_access,                   /* map_access */
    default_get_sd,                  /* get_sd */
    default_set_sd,                  /* set_sd */
    no_lookup_name,                  /* lookup_name */
    no_open_file,                    /* open_file */
    no_close_handle,                 /* close_handle */
    async_queue_destroy              /* destroy */
};


static inline void async_reselect( struct async *async )
{
    if (async->queue->fd) fd_reselect_async( async->queue->fd, async->queue );
}

static void async_dump( struct object *obj, int verbose )
{
    struct async *async = (struct async *)obj;
    assert( obj->ops == &async_ops );
    fprintf( stderr, "Async thread=%p\n", async->thread );
}

static void async_destroy( struct object *obj )
{
    struct async *async = (struct async *)obj;
    assert( obj->ops == &async_ops );

    list_remove( &async->queue_entry );
    async_reselect( async );

    if (async->timeout) remove_timeout_user( async->timeout );
    if (async->event) release_object( async->event );
    release_object( async->queue );
    release_object( async->thread );
}

static void async_queue_dump( struct object *obj, int verbose )
{
    struct async_queue *async_queue = (struct async_queue *)obj;
    assert( obj->ops == &async_queue_ops );
    fprintf( stderr, "Async queue fd=%p\n", async_queue->fd );
}

static void async_queue_destroy( struct object *obj )
{
    struct async_queue *async_queue = (struct async_queue *)obj;
    assert( obj->ops == &async_queue_ops );
    if (async_queue->completion) release_object( async_queue->completion );
}

/* notifies client thread of new status of its async request */
void async_terminate( struct async *async, unsigned int status )
{
    apc_call_t data;

    assert( status != STATUS_PENDING );

    if (async->status != STATUS_PENDING)
    {
        /* already terminated, just update status */
        async->status = status;
        return;
    }

    memset( &data, 0, sizeof(data) );
    data.type            = APC_ASYNC_IO;
    data.async_io.func   = async->data.callback;
    data.async_io.user   = async->data.arg;
    data.async_io.sb     = async->data.iosb;
    data.async_io.status = status;
    thread_queue_apc( async->thread, &async->obj, &data );
    async->status = status;
    async_reselect( async );
    release_object( async );  /* so that it gets destroyed when the async is done */
}

/* callback for timeout on an async request */
static void async_timeout( void *private )
{
    struct async *async = private;

    async->timeout = NULL;
    async_terminate( async, async->timeout_status );
}

/* create a new async queue for a given fd */
struct async_queue *create_async_queue( struct fd *fd )
{
    struct async_queue *queue = alloc_object( &async_queue_ops );

    if (queue)
    {
        queue->fd = fd;
        queue->completion = NULL;
        list_init( &queue->queue );
    }
    return queue;
}

/* free an async queue, cancelling all async operations */
void free_async_queue( struct async_queue *queue )
{
    if (!queue) return;
    if (queue->fd) queue->completion = fd_get_completion( queue->fd, &queue->comp_key );
    queue->fd = NULL;
    async_wake_up( queue, STATUS_HANDLES_CLOSED );
    release_object( queue );
}

/* create an async on a given queue of a fd */
struct async *create_async( struct thread *thread, struct async_queue *queue, const async_data_t *data )
{
    struct event *event = NULL;
    struct async *async;

    if (data->event && !(event = get_event_obj( thread->process, data->event, EVENT_MODIFY_STATE )))
        return NULL;

    if (!(async = alloc_object( &async_ops )))
    {
        if (event) release_object( event );
        return NULL;
    }

    async->thread  = (struct thread *)grab_object( thread );
    async->event   = event;
    async->status  = STATUS_PENDING;
    async->data    = *data;
    async->timeout = NULL;
    async->queue   = (struct async_queue *)grab_object( queue );

    list_add_tail( &queue->queue, &async->queue_entry );
    grab_object( async );

    if (queue->fd) set_fd_signaled( queue->fd, 0 );
    if (event) reset_event( event );
    return async;
}

/* set the timeout of an async operation */
void async_set_timeout( struct async *async, timeout_t timeout, unsigned int status )
{
    if (async->timeout) remove_timeout_user( async->timeout );
    if (timeout != TIMEOUT_INFINITE) async->timeout = add_timeout_user( timeout, async_timeout, async );
    else async->timeout = NULL;
    async->timeout_status = status;
}

static void add_async_completion( struct async_queue *queue, apc_param_t cvalue, unsigned int status,
                                  unsigned int information )
{
    if (status == STATUS_MORE_PROCESSING_REQUIRED)
        return; /* The async callback has successfully finished but no completion should be reported */

    if (queue->fd)
    {
        apc_param_t ckey;
        struct completion *completion = fd_get_completion( queue->fd, &ckey );

        if (completion)
        {
            add_completion( completion, ckey, cvalue, status, information );
            release_object( completion );
        }
    }
    else if (queue->completion) add_completion( queue->completion, queue->comp_key,
                                                cvalue, status, information );
}

/* store the result of the client-side async callback */
void async_set_result( struct object *obj, unsigned int status, unsigned int total, client_ptr_t apc )
{
    struct async *async = (struct async *)obj;

    if (obj->ops != &async_ops) return;  /* in case the client messed up the APC results */

    assert( async->status != STATUS_PENDING );  /* it must have been woken up if we get a result */

    if (status == STATUS_PENDING)  /* restart it */
    {
        status = async->status;
        async->status = STATUS_PENDING;
        grab_object( async );

        if (status != STATUS_ALERTED)  /* it was terminated in the meantime */
            async_terminate( async, status );
        else
            async_reselect( async );
    }
    else
    {
        if (async->timeout) remove_timeout_user( async->timeout );
        async->timeout = NULL;
        async->status = status;
        if (async->data.cvalue) add_async_completion( async->queue, async->data.cvalue, status, total );
        if (apc)
        {
            apc_call_t data;
            memset( &data, 0, sizeof(data) );
            data.type         = APC_USER;
            data.user.func    = apc;
            data.user.args[0] = async->data.arg;
            data.user.args[1] = async->data.iosb;
            data.user.args[2] = 0;
            thread_queue_apc( async->thread, NULL, &data );
        }
        if (async->event) set_event( async->event );
        else if (async->queue->fd) set_fd_signaled( async->queue->fd, 1 );
    }
}

/* check if there are any queued async operations */
int async_queued( struct async_queue *queue )
{
    return queue && list_head( &queue->queue );
}

/* check if an async operation is waiting to be alerted */
int async_waiting( struct async_queue *queue )
{
    struct list *ptr;
    struct async *async;

    if (!queue) return 0;
    if (!(ptr = list_head( &queue->queue ))) return 0;
    async = LIST_ENTRY( ptr, struct async, queue_entry );
    return async->status == STATUS_PENDING;
}

int async_wake_up_by( struct async_queue *queue, struct process *process,
                      struct thread *thread, client_ptr_t iosb, unsigned int status )
{
    struct list *ptr, *next;
    int woken = 0;

    if (!queue || (!process && !thread && !iosb)) return 0;

    LIST_FOR_EACH_SAFE( ptr, next, &queue->queue )
    {
        struct async *async = LIST_ENTRY( ptr, struct async, queue_entry );
        if ( (!process || async->thread->process == process) &&
             (!thread || async->thread == thread) &&
             (!iosb || async->data.iosb == iosb) )
        {
            async_terminate( async, status );
            woken++;
        }
    }
    return woken;
}

/* wake up async operations on the queue */
void async_wake_up( struct async_queue *queue, unsigned int status )
{
    struct list *ptr, *next;

    if (!queue) return;

    LIST_FOR_EACH_SAFE( ptr, next, &queue->queue )
    {
        struct async *async = LIST_ENTRY( ptr, struct async, queue_entry );
        async_terminate( async, status );
        if (status == STATUS_ALERTED) break;  /* only wake up the first one */
    }
}
