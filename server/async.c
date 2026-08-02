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
    struct timeout_user *timeout;
    unsigned int         timeout_status;  /* status to report upon timeout */
    struct event        *event;
    struct async_data    data;            /* data for async I/O call */
    struct iosb         *iosb;            /* I/O status block */
    obj_handle_t         wait_handle;     /* pre-allocated wait handle */
    unsigned int         initial_status;  /* status returned from initial request */
    unsigned int         signaled :1;
    unsigned int         pending :1;      /* request successfully queued, but pending */
    unsigned int         direct_result :1;/* a flag if we're passing result directly from request instead of APC  */
    unsigned int         alerted :1;      /* fd is signaled, but we are waiting for client-side I/O */
    unsigned int         terminated :1;   /* async has been terminated */
    unsigned int         canceled :1;     /* have we already queued cancellation for this async? */
    unsigned int         unknown_status :1; /* initial status is not known yet */
    unsigned int         blocking :1;     /* async is blocking */
    unsigned int         is_system :1;    /* background system operation not affecting userspace visible state. */
    struct completion   *completion;      /* completion associated with fd */
    apc_param_t          comp_key;        /* completion key associated with fd */
    unsigned int         comp_flags;      /* completion flags */
    async_completion_callback completion_callback; /* callback to be called on completion */
    void                *completion_callback_private; /* argument to completion_callback */
};

static void async_dump( struct object *obj, int verbose );
static int async_signaled( struct object *obj, struct wait_queue_entry *entry );
static void async_satisfied( struct object * obj, struct wait_queue_entry *entry );
static void async_destroy( struct object *obj );

static const struct object_ops async_ops =
{
    sizeof(struct async),      /* size */
    &no_type,                  /* type */
    async_dump,                /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    async_signaled,            /* signaled */
    async_satisfied,           /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
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

    /* we only return an async handle for asyncs created via create_request_async() */
    assert( async->iosb );

    if (async->direct_result)
    {
        async_set_result( &async->obj, async->iosb->status, async->iosb->result );
        async->direct_result = 0;
    }

    if (async->initial_status == STATUS_PENDING && async->blocking)
        set_wait_status( entry, async->iosb->status );
    else
        set_wait_status( entry, async->initial_status );

    /* close wait handle here to avoid extra server round trip */
    if (async->wait_handle)
    {
        close_handle( async->thread->process, async->wait_handle );
        async->wait_handle = 0;
    }
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
    struct iosb *iosb = async->iosb;

    if (async->terminated) return;

    async->terminated = 1;
    if (async->iosb && async->iosb->status == STATUS_PENDING) async->iosb->status = status;
    if (status == STATUS_ALERTED)
        async->alerted = 1;

    /* if no APC could be queued (e.g. the process is terminated),
     * thread_queue_apc() may trigger async_set_result(), which may drop the
     * last reference to the async, so grab a temporary reference here */
    grab_object( async );

    if (!async->direct_result)
    {
        union apc_call data;

        memset( &data, 0, sizeof(data) );
        data.type            = APC_ASYNC_IO;
        data.async_io.user   = async->data.user;
        data.async_io.result = iosb ? iosb->result : 0;

        /* this can happen if the initial status was unknown (i.e. for device
         * files). the client should not fill the IOSB in this case; pass it as
         * NULL to communicate that.
         * note that we check the IOSB status and not the initial status */
        if (NT_ERROR( status ) && (!is_fd_overlapped( async->fd ) || !async->pending))
            data.async_io.sb = 0;
        else
            data.async_io.sb = async->data.iosb;

        /* if there is output data, the client needs to make an extra request
         * to retrieve it; use STATUS_ALERTED to signal this case */
        if (iosb && iosb->out_data)
            data.async_io.status = STATUS_ALERTED;
        else
            data.async_io.status = status;

        thread_queue_apc( async->thread->process, async->thread, &async->obj, &data );
    }

    async_reselect( async );

    release_object( async );
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

    if (!async->is_system) set_fd_signaled( async->fd, 0 );
}

/* create an async on a given queue of a fd */
struct async *create_async( struct fd *fd, struct thread *thread, const struct async_data *data, struct iosb *iosb )
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
    async->data          = *data;
    async->timeout       = NULL;
    async->queue         = NULL;
    async->fd            = (struct fd *)grab_object( fd );
    async->initial_status = STATUS_PENDING;
    async->signaled      = 0;
    async->pending       = 1;
    async->wait_handle   = 0;
    async->direct_result = 0;
    async->alerted       = 0;
    async->terminated    = 0;
    async->canceled      = 0;
    async->unknown_status = 0;
    async->blocking      = !is_fd_overlapped( fd );
    async->is_system     = 0;
    async->completion    = fd_get_completion( fd, &async->comp_key );
    async->comp_flags    = 0;
    async->completion_callback = NULL;
    async->completion_callback_private = NULL;

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

/* set the initial status of an async whose status was previously unknown
 * the initial status may be STATUS_PENDING */
void async_set_initial_status( struct async *async, unsigned int status )
{
    async->initial_status = status;
    async->unknown_status = 0;
}

void set_async_pending( struct async *async )
{
    if (!async->terminated)
        async->pending = 1;
}

void async_wake_obj( struct async *async )
{
    assert( !async->unknown_status );
    if (!async->blocking)
    {
        async->signaled = 1;
        wake_up( &async->obj, 0 );
    }
}

static void async_call_completion_callback( struct async *async )
{
    if (async->completion_callback)
        async->completion_callback( async->completion_callback_private );
    async->completion_callback = NULL;
}

/* return async object status and wait handle to client */
obj_handle_t async_handoff( struct async *async, data_size_t *result, int force_blocking )
{
    async->blocking = force_blocking || async->blocking;

    if (async->unknown_status)
    {
        /* even the initial status is not known yet */
        set_error( STATUS_PENDING );
        return async->wait_handle;
    }

    if (get_error() == STATUS_ALERTED)
    {
        /* give the client opportunity to complete synchronously.  after the
         * client performs the I/O, it reports the result back to the server
         * via the set_async_direct_result request.  if it turns out that the
         * I/O request is not actually immediately satiable, the client may
         * then choose to re-queue the async by reporting STATUS_PENDING
         * instead.
         *
         * since we're deferring the initial I/O (to the client), we mark the
         * async as having unknown initial status (unknown_status = 1).  note
         * that we don't reuse async_set_unknown_status() here.  this is because
         * the one responsible for performing the I/O is not the device driver,
         * but instead the client that requested the I/O in the first place.
         *
         * also, async_set_unknown_status() would set direct_result to zero
         * forcing APC_ASYNC_IO to fire in async_terminate(), which is not
         * useful due to subtle semantic differences between synchronous and
         * asynchronous completion.
         */
        async->unknown_status = 1;
        async_terminate( async, STATUS_ALERTED );
        return async->wait_handle;
    }

    async->initial_status = get_error();

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

    if (!async->pending && NT_ERROR( async->iosb->status ))
    {
        async_call_completion_callback( async );

        close_handle( async->thread->process, async->wait_handle );
        async->wait_handle = 0;
        set_error( async->iosb->status );
        return 0;
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
        if (!async->blocking)
        {
            close_handle( async->thread->process, async->wait_handle);
            async->wait_handle = 0;
        }
    }
    async->initial_status = async->iosb->status;
    set_error( async->iosb->status );
    return async->wait_handle;
}

/* complete a request-based async with a pre-allocated buffer */
void async_request_complete( struct async *async, unsigned int status, data_size_t result,
                             data_size_t out_size, void *out_data )
{
    struct iosb *iosb = async_get_iosb( async );

    /* the async may have already been canceled */
    if (iosb->status != STATUS_PENDING)
    {
        release_object( iosb );
        free( out_data );
        return;
    }

    iosb->status = status;
    iosb->result = result;
    iosb->out_data = out_data;
    iosb->out_size = out_size;

    release_object( iosb );

    async_terminate( async, status );
}

/* complete a request-based async */
void async_request_complete_alloc( struct async *async, unsigned int status, data_size_t result,
                                   data_size_t out_size, const void *out_data )
{
    void *out_data_copy = NULL;

    if (out_size && !(out_data_copy = memdup( out_data, out_size )))
    {
        async_terminate( async, STATUS_NO_MEMORY );
        return;
    }

    async_request_complete( async, status, result, out_size, out_data_copy );
}

/* mark an async as having unknown initial status */
void async_set_unknown_status( struct async *async )
{
    async->unknown_status = 1;
    async->direct_result = 0;
}

/* set the timeout of an async operation */
void async_set_timeout( struct async *async, timeout_t timeout, unsigned int status )
{
    if (async->timeout) remove_timeout_user( async->timeout );
    if (timeout != TIMEOUT_INFINITE) async->timeout = add_timeout_user( timeout, async_timeout, async );
    else async->timeout = NULL;
    async->timeout_status = status;
}

/* set a callback to be notified when the async is completed */
void async_set_completion_callback( struct async *async, async_completion_callback func, void *private )
{
    async->completion_callback = func;
    async->completion_callback_private = private;
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

    assert( async->terminated );  /* it must have been woken up if we get a result */

    if (async->unknown_status) async_set_initial_status( async, status );

    if (async->alerted && status == STATUS_PENDING)  /* restart it */
    {
        async->terminated = 0;
        async->alerted = 0;
        async_reselect( async );
    }
    else
    {
        if (async->timeout) remove_timeout_user( async->timeout );
        async->timeout = NULL;
        async->terminated = 1;
        if (async->iosb) async->iosb->status = status;

        /* don't signal completion if the async failed synchronously
         * this can happen if the initial status was unknown (i.e. for device files)
         * note that we check the IOSB status here, not the initial status */
        if (async->pending || !NT_ERROR( status ))
        {
            if (async->data.apc)
            {
                union apc_call data;
                memset( &data, 0, sizeof(data) );
                data.type         = APC_USER;
                data.user.func    = async->data.apc;
                data.user.args[0] = async->data.apc_context;
                data.user.args[1] = async->data.iosb;
                data.user.args[2] = 0;
                thread_queue_apc( NULL, async->thread, NULL, &data );
            }
            else if (async->data.apc_context && (async->pending ||
                     !(async->comp_flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS)))
            {
                add_async_completion( async, async->data.apc_context, status, total );
            }

            if (async->event) set_event( async->event );
            else if (async->fd && !async->is_system) set_fd_signaled( async->fd, 1 );
        }

        if (!async->signaled)
        {
            async->signaled = 1;
            wake_up( &async->obj, 0 );
        }

        async_call_completion_callback( async );

        if (async->queue)
        {
            list_remove( &async->queue_entry );
            async_reselect( async );
            async->fd = NULL;
            async->queue = NULL;
            release_object( async );
        }
    }
}

int async_queue_has_waiting_asyncs( struct async_queue *queue )
{
    struct async *async;

    LIST_FOR_EACH_ENTRY( async, &queue->queue, struct async, queue_entry )
        if (!async->unknown_status) return 1;

    return 0;
}

/* check if an async operation is waiting to be alerted */
int async_waiting( struct async_queue *queue )
{
    struct list *ptr;
    struct async *async;

    if (!(ptr = list_head( &queue->queue ))) return 0;
    async = LIST_ENTRY( ptr, struct async, queue_entry );
    return !async->terminated;
}

static int cancel_async( struct process *process, struct object *obj, struct thread *thread, client_ptr_t iosb )
{
    struct async *async;
    int woken = 0;

    /* FIXME: it would probably be nice to replace the "canceled" flag with a
     * single LIST_FOR_EACH_ENTRY_SAFE, but currently cancelling an async can
     * cause other asyncs to be removed via async_reselect() */

restart:
    LIST_FOR_EACH_ENTRY( async, &process->asyncs, struct async, process_entry )
    {
        if (async->terminated || async->canceled || async->is_system) continue;
        if ((!obj || (get_fd_user( async->fd ) == obj)) &&
            (!thread || async->thread == thread) &&
            (!iosb || async->data.iosb == iosb))
        {
            async->canceled = 1;
            fd_cancel_async( async->fd, async );
            woken++;
            goto restart;
        }
    }
    return woken;
}

static int cancel_blocking( struct process *process, struct thread *thread, client_ptr_t iosb )
{
    struct async *async;
    int woken = 0;

restart:
    LIST_FOR_EACH_ENTRY( async, &process->asyncs, struct async, process_entry )
    {
        if (async->terminated || async->canceled) continue;
        if (async->blocking && async->thread == thread &&
            (!iosb || async->data.iosb == iosb))
        {
            async->canceled = 1;
            fd_cancel_async( async->fd, async );
            woken++;
            goto restart;
        }
    }
    return woken;
}

void cancel_process_asyncs( struct process *process )
{
    struct async *async;

restart:
    LIST_FOR_EACH_ENTRY( async, &process->asyncs, struct async, process_entry )
    {
        if (async->terminated || async->canceled) continue;
        async->canceled = 1;
        fd_cancel_async( async->fd, async );
        goto restart;
    }
}

int async_close_obj_handle( struct object *obj, struct process *process, obj_handle_t handle )
{
    /* Handle a special case when the last object handle in the given process is closed.
     * If this is the last object handle overall that is handled in object's close_handle and
     * destruction. */
    struct async *async;

    if (obj->handle_count == 1 || get_obj_handle_count( process, obj ) != 1) return 1;

restart:
    LIST_FOR_EACH_ENTRY( async, &process->asyncs, struct async, process_entry )
    {
        if (async->terminated || async->canceled || get_fd_user( async->fd ) != obj) continue;
        if (!async->completion || !async->data.apc_context || async->event) continue;

        async->canceled = 1;
        fd_cancel_async( async->fd, async );
        goto restart;
    }
    return 1;
}

void cancel_terminating_thread_asyncs( struct thread *thread )
{
    struct async *async;

restart:
    LIST_FOR_EACH_ENTRY( async, &thread->process->asyncs, struct async, process_entry )
    {
        if (async->thread != thread || async->terminated || async->canceled) continue;
        if (async->completion && async->data.apc_context && !async->event) continue;
        if (async->is_system) continue;

        async->canceled = 1;
        fd_cancel_async( async->fd, async );
        goto restart;
    }
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
    &no_type,                 /* type */
    iosb_dump,                /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_signal,                /* signal */
    no_get_fd,                /* get_fd */
    default_map_access,       /* map_access */
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
static struct iosb *create_iosb( const void *in_data, data_size_t in_size, data_size_t out_size )
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

/* create an async associated with iosb for async-based requests
 * returned async must be passed to async_handoff */
struct async *create_request_async( struct fd *fd, unsigned int comp_flags, const struct async_data *data, int is_system )
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
        async->is_system     = !!is_system;
        async->comp_flags    = comp_flags;
    }
    return async;
}

struct iosb *async_get_iosb( struct async *async )
{
    return async->iosb ? (struct iosb *)grab_object( async->iosb ) : NULL;
}

struct thread *async_get_thread( struct async *async )
{
    return async->thread;
}

/* find the first pending async in queue */
struct async *find_pending_async( struct async_queue *queue )
{
    struct async *async;
    LIST_FOR_EACH_ENTRY( async, &queue->queue, struct async, queue_entry )
        if (!async->terminated) return (struct async *)grab_object( async );
    return NULL;
}

/* cancels sync I/O on a thread */
DECL_HANDLER(cancel_sync)
{
    struct thread *thread = get_thread_from_handle( req->handle, THREAD_TERMINATE );

    if (thread)
    {
        if (!cancel_blocking( current->process, thread, req->iosb ))
            set_error( STATUS_NOT_FOUND );
        release_object( thread );
    }
}

/* cancels all async I/O */
DECL_HANDLER(cancel_async)
{
    struct object *obj = get_handle_obj( current->process, req->handle, 0, NULL );
    struct thread *thread = req->only_thread ? current : NULL;

    if (obj)
    {
        int count = cancel_async( current->process, obj, thread, req->iosb );
        if (!count && !thread) set_error( STATUS_NOT_FOUND );
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
    set_error( iosb->status );
}

/* notify direct completion of async and close the wait handle if not blocking */
DECL_HANDLER(set_async_direct_result)
{
    struct async *async = (struct async *)get_handle_obj( current->process, req->handle, 0, &async_ops );
    unsigned int status = req->status;

    if (!async) return;

    if (!async->unknown_status || !async->terminated || !async->alerted)
    {
        set_error( STATUS_INVALID_PARAMETER );
        release_object( &async->obj );
        return;
    }

    if (status == STATUS_PENDING)
    {
        async->direct_result = 0;
        async->pending = 1;
    }
    else if (req->mark_pending)
    {
        async->pending = 1;
    }

    /* if the I/O has completed successfully (or unsuccessfully, and
     * async->pending is set), the client would have already set the IOSB.
     * therefore, we can do async_set_result() directly and let the client skip
     * waiting on wait_handle.
     */
    async_set_result( &async->obj, status, req->information );

    /* close wait handle here to avoid extra server round trip, if the I/O
     * either has completed, or is pending and not blocking.
     */
    if (status != STATUS_PENDING || !async->blocking)
    {
        close_handle( async->thread->process, async->wait_handle );
        async->wait_handle = 0;
    }

    /* report back to the client whether the wait handle has been closed.
     * handle will be 0 if closed by us; otherwise the original value is
     * retained
     */
    reply->handle = async->wait_handle;

    release_object( &async->obj );
}
