/*
 * Server-side support for async i/o operations
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2000 Mike McCormack
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "handle.h"
#include "thread.h"
#include "request.h"

#include "async.h"

void destroy_async( struct async *async )
{
    struct async_queue *aq = async->q;

    /*fprintf(stderr,"destroyed async %p\n",async->overlapped); */

    if(async->timeout)
        remove_timeout_user(async->timeout);
    async->timeout = NULL;

    if(async->prev)
        async->prev->next = async->next;
    else
        aq->head = async->next;

    if(async->next)
        async->next->prev = async->prev;
    else
        aq->tail = async->prev;

    async->q = NULL;
    async->next = NULL;
    async->prev = NULL;

    free(async);
}

void async_notify(struct async *async, int status)
{
    /* fprintf(stderr,"notifying %p!\n",async->overlapped); */
    async->status = status;
    thread_queue_apc(async->thread, NULL, NULL, APC_ASYNC_IO, 1, 2, async->overlapped, status);
}

void destroy_async_queue( struct async_queue *q )
{
    while(q->head)
    {
        async_notify(q->head, STATUS_CANCELLED);
        destroy_async(q->head);
    }
}

struct async *find_async(struct async_queue *q, struct thread *thread, void *overlapped)
{
    struct async *async;

    /* fprintf(stderr,"find_async: %p\n",overlapped); */

    if(!q)
        return NULL;

    for(async = q->head; async; async = async->next)
        if((async->overlapped==overlapped) && (async->thread == thread))
             return async;

    return NULL;
}

void async_insert(struct async_queue *q, struct async *async)
{
    async->q = q;
    async->prev = q->tail;
    async->next = NULL;

    if(q->tail)
        q->tail->next = async;
    else
        q->head = async;

    q->tail = async;
}

static void async_callback(void *private)
{
    struct async *async = (struct async *)private;

    /* fprintf(stderr,"%p timeout out\n",async->overlapped); */
    async->timeout = NULL;
    async_notify(async, STATUS_TIMEOUT);
    destroy_async(async);
}

struct async *create_async(struct object *obj, struct thread *thread,
                           void *overlapped)
{
    struct async *async = (struct async *) malloc(sizeof(struct async));
    if(!async)
    {
        set_error(STATUS_NO_MEMORY);
        return NULL;
    }

    async->obj = obj;
    async->thread = thread;
    async->overlapped = overlapped;
    async->next = NULL;
    async->prev = NULL;
    async->q = NULL;
    async->status = STATUS_PENDING;
    async->timeout = NULL;

    return async;
}

void async_add_timeout(struct async *async, int timeout)
{
    if(timeout)
    {
        gettimeofday( &async->when, 0 );
        add_timeout( &async->when, timeout );
        async->timeout = add_timeout_user( &async->when, async_callback, async );
    }
}

DECL_HANDLER(register_async)
{
    struct object *obj;

    if (!(obj = get_handle_obj( current->process, req->handle, 0, NULL)) )
        return;

    if(obj->ops->queue_async)
    {
        struct async_queue *q = obj->ops->queue_async(obj, NULL, req->type, 0);
        struct async *async;

        async = find_async(q, current, req->overlapped);
        if(req->status==STATUS_PENDING)
        {
            if(!async)
                async = create_async(obj, current, req->overlapped);

            if(async)
            {
                async->status = req->status;
                if(!obj->ops->queue_async(obj, async, req->type, req->count))
                    destroy_async(async);
            }
        }
        else
        {
            if(async)
                destroy_async(async);
            else
                set_error(STATUS_INVALID_PARAMETER);
        }

        set_select_events(obj,obj->ops->get_poll_events(obj));
    }
    else
        set_error(STATUS_INVALID_HANDLE);

    release_object(obj);
}

