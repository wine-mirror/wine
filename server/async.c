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
    struct list          queue_entry;     /* entry in file descriptor queue */
    struct timeout_user *timeout;
    async_data_t         data;            /* data for async I/O call */
};

static void async_dump( struct object *obj, int verbose );
static void async_destroy( struct object *obj );

static const struct object_ops async_ops =
{
    sizeof(struct async),      /* size */
    async_dump,                /* dump */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    no_map_access,             /* map_access */
    no_lookup_name,            /* lookup_name */
    no_close_handle,           /* close_handle */
    async_destroy              /* destroy */
};

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

    if (async->timeout) remove_timeout_user( async->timeout );
    release_object( async->thread );
}

/* notifies client thread of new status of its async request */
/* destroys the server side of it */
static void async_terminate( struct async *async, unsigned int status )
{
    apc_call_t data;

    memset( &data, 0, sizeof(data) );
    data.type            = APC_ASYNC_IO;
    data.async_io.func   = async->data.callback;
    data.async_io.user   = async->data.arg;
    data.async_io.sb     = async->data.iosb;
    data.async_io.status = status;
    thread_queue_apc( async->thread, &async->obj, &data );

    if (async->timeout) remove_timeout_user( async->timeout );
    async->timeout = NULL;
    list_remove( &async->queue_entry );
    release_object( async );
}

/* callback for timeout on an async request */
static void async_timeout( void *private )
{
    struct async *async = private;

    async->timeout = NULL;
    async_terminate( async, STATUS_TIMEOUT );
}

/* create an async on a given queue of a fd */
struct async *create_async( struct thread *thread, const struct timeval *timeout,
                            struct list *queue, const async_data_t *data )
{
    struct async *async = alloc_object( &async_ops );

    if (!async) return NULL;

    async->thread = (struct thread *)grab_object( thread );
    async->data = *data;

    list_add_tail( queue, &async->queue_entry );

    if (timeout) async->timeout = add_timeout_user( timeout, async_timeout, async );
    else async->timeout = NULL;

    return async;
}

/* terminate the async operation at the head of the queue */
void async_terminate_head( struct list *queue, unsigned int status )
{
    struct list *ptr = list_head( queue );
    if (ptr) async_terminate( LIST_ENTRY( ptr, struct async, queue_entry ), status );
}

/* terminate all async operations on the queue */
void async_terminate_queue( struct list *queue, unsigned int status )
{
    struct list *ptr, *next;

    LIST_FOR_EACH_SAFE( ptr, next, queue )
    {
        struct async *async = LIST_ENTRY( ptr, struct async, queue_entry );
        async_terminate( async, status );
    }
}
