/*
 * Async i/o definitions
 *
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

#ifndef _SERVER_ASYNC_
#define _SERVER_ASYNC_

#include <sys/time.h>
#include "object.h"

struct async_queue;

struct async 
{
    struct object       *obj;
    struct thread       *thread;
    void                *overlapped;
    unsigned int         status;
    struct timeval       when;
    struct timeout_user *timeout;
    struct async        *next,*prev;
    struct async_queue  *q;
};

struct async_queue
{
    struct async        *head;
    struct async        *tail;
};

void destroy_async( struct async *async );
void destroy_async_queue( struct async_queue *q );
void async_notify(struct async *async, int status);
struct async *find_async(struct async_queue *q, struct thread *thread, void *overlapped);
void async_insert(struct async_queue *q, struct async *async);
struct async *create_async(struct object *obj, struct thread *thread, 
                           void *overlapped);
void async_add_timeout(struct async *async, int timeout);
static inline void init_async_queue(struct async_queue *q) 
{ 
    q->head = q->tail = NULL;
}

#define IS_READY(q) (((q).head) && ((q).head->status==STATUS_PENDING))

#endif /* _SERVER_ASYNC_ */

