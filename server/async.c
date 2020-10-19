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
#include "process.h"
#include "handle.h"

struct async
{
    struct object        obj;             /* object header */
    struct thread       *thread;          /* owning thread */
    struct list          queue_entry;     /* entry in async queue list */
    struct list          process_entry;   /* entry in process list */
    struct async_queue  *queue;           /* queue containing this async */
    struct fd           *fd;              /* fd associated with an unqueued async */
    unsigned int         status;          /* current status */
    struct timeout_user *timeout;
    unsigned int         timeout_status;  /* status to report upon timeout */
    struct event        *event;
    async_data_t         data;            /* data for async I/O call */
    struct iosb         *iosb;            /* I/O status block */
    obj_handle_t         wait_handle;     /* pre-allocated wait handle */
    unsigned int         signaled :1;
    unsigned int         pending :1;      /* request successfully queued, but pending */
    unsigned int         direct_result :1;/* a flag if we're passing result directly from request instead of APC  */
    struct completion   *completion;      /* completion associated with fd */
    apc_param_t          comp_key;        /* completion key associated with fd */
    unsigned int         comp_flags;      /* completion flags */
};

static void async_dump( struct object *obj, int verbose );
static int async_signaled( struct object *obj, struct wait_queue_entry *entry );
static void async_satisfied( struct object * obj, struct wait_queue_entry *entry );
static void async_destroy( struct object *obj );

static const struct object_ops async_ops =
{
    sizeof(struct async),      /* size */
    async_dump,                /* dump */
    no_get_type,               /* get_type */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    async_signaled,            /* signaled */
    async_satisfied,           /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    no_map_access,             /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_get_full_name,          /* get_full_name */
    no_lookup_name,            /* lookup_name */
    no_link_name,              /* link_name */
    NULL,                      /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    async_destroy              /* destroy */
};

static inline void async_reselect( struct async *async )
{
    if (async->queue && async->fd) fd_reselect_async( async->fd, async->queue );
}

static void async_dump( struct object *obj, int verbose )
{
    struct async *async = (struct async *)obj;
    assert( obj->ops == &async_ops );
    fprintf( stderr, "Async thread=%p\n", async->thread );
}

static int async_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct async *async = (struct async *)obj;
    assert( obj->ops == &async_ops );
    return async->signaled;
}

static void async_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
    struct async *async = (struct async *)obj;
    assert( obj->ops == &async_ops );

    if (async->direct_result)
    {
        async_set_result( &async->obj, async->iosb->status, async->iosb->result );
        async->direct_result = 0;
    }

    /* close wait handle here to avoid extra server round trip */
    if (async->wait_handle)
    {
        close_handle( async->thread->process, async->wait_handle );
        async->wait_handle = 0;
    }

    if (async->status == STATUS_PENDING) make_wait_abandoned( entry );
}

static void async_destroy( struct object *obj )
{
    struct async *async = (struct async *)obj;
    assert( obj->ops == &async_ops );

    list_remove( &async->process_entry );

    if (async->queue)
    {
        list_remove( &async->queue_entry );
        async_reselect( async );
    }
    else if (async->fd) release_object( async->fd );

    if (async->timeout) remove_timeout_user( async->timeout );
    if (async->completion) release_object( async->completion );
    if (async->event) release_object( async->event );
    if (async->iosb) release_object( async->iosb );
    release_object( async->thread );
}

/* notifies client thread of new status of its async request */
void async_terminate( struct async *async, unsigned int status )
{
    assert( status != STATUS_PENDING );

    if (async->status != STATUS_PENDING)
    {
        /* already terminated, just update status */
        async->status = status;
        return;
    }

    async->status = status;
    if (async->iosb && async->iosb->status == STATUS_PENDING) async->iosb->status = status;

    if (!async->direct_result)
    {
        if (async->data.user)
        {
            apc_call_t data;

            memset( &data, 0, sizeof(data) );
            data.type            = APC_ASYNC_IO;
            data.async_io.user   = async->data.user;
            data.async_io.sb     = async->data.iosb;
            data.async_io.status = status;
            thread_queue_apc( async->thread->process, async->thread, &async->obj, &data );
        }
        else async_set_result( &async->obj, STATUS_SUCCESS, 0 );
    }

    async_reselect( async );
    if (async->queue) release_object( async );  /* so that it gets destroyed when the async is done */
}

/* callback for timeout on an async request */
static void async_timeout( void *private )
{
    struct async *async = private;

    async->timeout = NULL;
    async_terminate( async, async->timeout_status );
}

/* free an async queue, cancelling all async operations */
void free_async_queue( struct async_queue *queue )
{
    struct async *async, *next;

    LIST_FOR_EACH_ENTRY_SAFE( async, next, &queue->queue, struct async, queue_entry )
    {
        grab_object( &async->obj );
        if (!async->completion) async->completion = fd_get_completion( async->fd, &async->comp_key );
        async->fd = NULL;
        async_terminate( async, STATUS_HANDLES_CLOSED );
        async->queue = NULL;
        release_object( &async->obj );
    }
}

void queue_async( struct async_queue *queue, struct async *async )
{
    /* fd will be set to NULL in free_async_queue when fd is destroyed */
    release_object( async->fd );

    async->queue = queue;
    grab_object( async );
    list_add_tail( &queue->queue, &async->queue_entry );

    set_fd_signaled( async->fd, 0 );
}

/* create an async on a given queue of a fd */
struct async *create_async( struct fd *fd, struct thread *thread, const async_data_t *data, struct iosb *iosb )
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

    async->thread        = (struct thread *)grab_object( thread );
    async->event         = event;
    async->status        = STATUS_PENDING;
    async->data          = *data;
    async->timeout       = NULL;
    async->queue         = NULL;
    async->fd            = (struct fd *)grab_object( fd );
    async->signaled      = 0;
    async->pending       = 1;
    async->wait_handle   = 0;
    async->direct_result = 0;
    async->completion    = fd_get_completion( fd, &async->comp_key );
    async->comp_flags    = 0;

    if (iosb) async->iosb = (struct iosb *)grab_object( iosb );
    else async->iosb = NULL;

    list_add_head( &thread->process->asyncs, &async->process_entry );
    if (event) reset_event( event );

    if (async->completion && data->apc)
    {
        release_object( async );
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    return async;
}

void set_async_pending( struct async *async, int signal )
{
    if (async->status == STATUS_PENDING)
    {
        async->pending = 1;
        if (signal && !async->signaled)
        {
            async->signaled = 1;
            wake_up( &async->obj, 0 );
        }
    }
}

/* create an async associated with iosb for async-based requests
 * returned async must be passed to async_handoff */
struct async *create_request_async( struct fd *fd, unsigned int comp_flags, const async_data_t *data )
{
    struct async *async;
    struct iosb *iosb;

    if (!(iosb = create_iosb( get_req_data(), get_req_data_size(), get_reply_max_size() )))
        return NULL;

    async = create_async( fd, current, data, iosb );
    release_object( iosb );
    if (async)
    {
        if (!(async->wait_handle = alloc_handle( current->process, async, SYNCHRONIZE, 0 )))
        {
            release_object( async );
            return NULL;
        }
        async->pending       = 0;
        async->direct_result = 1;
        async->comp_flags    = comp_flags;
    }
    return async;
}

/* return async object status and wait handle to client */
obj_handle_t async_handoff( struct async *async, int success, data_size_t *result, int force_blocking )
{
    if (!success)
    {
        if (get_error() == STATUS_PENDING)
        {
            /* we don't know the result yet, so client needs to wait */
            async->direct_result = 0;
            return async->wait_handle;
        }
        close_handle( async->thread->process, async->wait_handle );
        async->wait_handle = 0;
        return 0;
    }

    if (get_error() != STATUS_PENDING)
    {
        /* status and data are already set and returned */
        async_terminate( async, get_error() );
    }
    else if (async->iosb->status != STATUS_PENDING)
    {
        /* result is already available in iosb, return it */
        if (async->iosb->out_data)
        {
            set_reply_data_ptr( async->iosb->out_data, async->iosb->out_size );
            async->iosb->out_data = NULL;
        }
    }

    if (async->iosb->status != STATUS_PENDING)
    {
        if (result) *result = async->iosb->result;
        async->signaled = 1;
    }
    else
    {
        async->direct_result = 0;
        async->pending = 1;
        if (!force_blocking && async->fd && is_fd_overlapped( async->fd ))
        {
            close_handle( async->thread->process, async->wait_handle);
            async->wait_handle = 0;
        }
    }
    set_error( async->iosb->status );
    return async->wait_handle;
}

/* set the timeout of an async operation */
void async_set_timeout( struct async *async, timeout_t timeout, unsigned int status )
{
    if (async->timeout) remove_timeout_user( async->timeout );
    if (timeout != TIMEOUT_INFINITE) async->timeout = add_timeout_user( timeout, async_timeout, async );
    else async->timeout = NULL;
    async->timeout_status = status;
}

static void add_async_completion( struct async *async, apc_param_t cvalue, unsigned int status,
                                  apc_param_t information )
{
    if (async->fd && !async->completion) async->completion = fd_get_completion( async->fd, &async->comp_key );
    if (async->completion) add_completion( async->completion, async->comp_key, cvalue, status, information );
}

/* store the result of the client-side async callback */
void async_set_result( struct object *obj, unsigned int status, apc_param_t total )
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
        if (status == STATUS_MORE_PROCESSING_REQUIRED) return;  /* don't report the completion */

        if (async->data.apc)
        {
            apc_call_t data;
            memset( &data, 0, sizeof(data) );
            data.type              = APC_USER;
            data.user.user.func    = async->data.apc;
            data.user.user.args[0] = async->data.apc_context;
            data.user.user.args[1] = async->data.iosb;
            data.user.user.args[2] = 0;
            thread_queue_apc( NULL, async->thread, NULL, &data );
        }
        else if (async->data.apc_context && (async->pending ||
                 !(async->comp_flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS)))
        {
            add_async_completion( async, async->data.apc_context, status, total );
        }

        if (async->event) set_event( async->event );
        else if (async->fd) set_fd_signaled( async->fd, 1 );
        if (!async->signaled)
        {
            async->signaled = 1;
            wake_up( &async->obj, 0 );
        }
    }
}

/* check if an async operation is waiting to be alerted */
int async_waiting( struct async_queue *queue )
{
    struct list *ptr;
    struct async *async;

    if (!(ptr = list_head( &queue->queue ))) return 0;
    async = LIST_ENTRY( ptr, struct async, queue_entry );
    return async->status == STATUS_PENDING;
}

static int cancel_async( struct process *process, struct object *obj, struct thread *thread, client_ptr_t iosb )
{
    struct async *async;
    int woken = 0;

restart:
    LIST_FOR_EACH_ENTRY( async, &process->asyncs, struct async, process_entry )
    {
        if (async->status == STATUS_CANCELLED) continue;
        if ((!obj || (async->fd && get_fd_user( async->fd ) == obj)) &&
            (!thread || async->thread == thread) &&
            (!iosb || async->data.iosb == iosb))
        {
            async_terminate( async, STATUS_CANCELLED );
            woken++;
            goto restart;
        }
    }
    return woken;
}

void cancel_process_asyncs( struct process *process )
{
    cancel_async( process, NULL, NULL, 0 );
}

/* wake up async operations on the queue */
void async_wake_up( struct async_queue *queue, unsigned int status )
{
    struct list *ptr, *next;

    LIST_FOR_EACH_SAFE( ptr, next, &queue->queue )
    {
        struct async *async = LIST_ENTRY( ptr, struct async, queue_entry );
        async_terminate( async, status );
        if (status == STATUS_ALERTED) break;  /* only wake up the first one */
    }
}

static void iosb_dump( struct object *obj, int verbose );
static void iosb_destroy( struct object *obj );

static const struct object_ops iosb_ops =
{
    sizeof(struct iosb),      /* size */
    iosb_dump,                /* dump */
    no_get_type,              /* get_type */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    no_map_access,            /* map_access */
    default_get_sd,           /* get_sd */
    default_set_sd,           /* set_sd */
    no_get_full_name,         /* get_full_name */
    no_lookup_name,           /* lookup_name */
    no_link_name,             /* link_name */
    NULL,                     /* unlink_name */
    no_open_file,             /* open_file */
    no_kernel_obj_list,       /* get_kernel_obj_list */
    no_close_handle,          /* close_handle */
    iosb_destroy              /* destroy */
};

static void iosb_dump( struct object *obj, int verbose )
{
    assert( obj->ops == &iosb_ops );
    fprintf( stderr, "I/O status block\n" );
}

static void iosb_destroy( struct object *obj )
{
    struct iosb *iosb = (struct iosb *)obj;

    free( iosb->in_data );
    free( iosb->out_data );
}

/* allocate iosb struct */
struct iosb *create_iosb( const void *in_data, data_size_t in_size, data_size_t out_size )
{
    struct iosb *iosb;

    if (!(iosb = alloc_object( &iosb_ops ))) return NULL;

    iosb->status = STATUS_PENDING;
    iosb->result = 0;
    iosb->in_size = in_size;
    iosb->in_data = NULL;
    iosb->out_size = out_size;
    iosb->out_data = NULL;

    if (in_size && !(iosb->in_data = memdup( in_data, in_size )))
    {
        release_object( iosb );
        iosb = NULL;
    }

    return iosb;
}

struct iosb *async_get_iosb( struct async *async )
{
    return async->iosb ? (struct iosb *)grab_object( async->iosb ) : NULL;
}

struct thread *async_get_thread( struct async *async )
{
    return async->thread;
}

int async_is_blocking( struct async *async )
{
    return !async->event && !async->data.apc && !async->data.apc_context;
}

/* find the first pending async in queue */
struct async *find_pending_async( struct async_queue *queue )
{
    struct async *async;
    LIST_FOR_EACH_ENTRY( async, &queue->queue, struct async, queue_entry )
        if (async->status == STATUS_PENDING) return (struct async *)grab_object( async );
    return NULL;
}

/* cancels all async I/O */
DECL_HANDLER(cancel_async)
{
    struct object *obj = get_handle_obj( current->process, req->handle, 0, NULL );
    struct thread *thread = req->only_thread ? current : NULL;

    if (obj)
    {
        int count = cancel_async( current->process, obj, thread, req->iosb );
        if (!count && req->iosb) set_error( STATUS_NOT_FOUND );
        release_object( obj );
    }
}

/* get async result from associated iosb */
DECL_HANDLER(get_async_result)
{
    struct iosb *iosb = NULL;
    struct async *async;

    LIST_FOR_EACH_ENTRY( async, &current->process->asyncs, struct async, process_entry )
        if (async->data.user == req->user_arg)
        {
            iosb = async->iosb;
            break;
        }

    if (!iosb)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (iosb->out_data)
    {
        data_size_t size = min( iosb->out_size, get_reply_max_size() );
        if (size)
        {
            set_reply_data_ptr( iosb->out_data, size );
            iosb->out_data = NULL;
        }
    }
    reply->size = iosb->result;
    set_error( iosb->status );
}
