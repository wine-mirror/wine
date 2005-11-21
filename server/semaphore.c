/*
 * Server-side semaphore management
 *
 * Copyright (C) 1998 Alexandre Julliard
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
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

struct semaphore
{
    struct object  obj;    /* object header */
    unsigned int   count;  /* current count */
    unsigned int   max;    /* maximum possible count */
};

static void semaphore_dump( struct object *obj, int verbose );
static int semaphore_signaled( struct object *obj, struct thread *thread );
static int semaphore_satisfied( struct object *obj, struct thread *thread );
static int semaphore_signal( struct object *obj, unsigned int access );

static const struct object_ops semaphore_ops =
{
    sizeof(struct semaphore),      /* size */
    semaphore_dump,                /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    semaphore_signaled,            /* signaled */
    semaphore_satisfied,           /* satisfied */
    semaphore_signal,              /* signal */
    no_get_fd,                     /* get_fd */
    no_close_handle,               /* close_handle */
    no_destroy                     /* destroy */
};


static struct semaphore *create_semaphore( const struct unicode_str *name, unsigned int attr,
                                           unsigned int initial, unsigned int max )
{
    struct semaphore *sem;

    if (!max || (initial > max))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    if ((sem = create_named_object( sync_namespace, &semaphore_ops, name, attr )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            sem->count = initial;
            sem->max   = max;
        }
    }
    return sem;
}

static int release_semaphore( struct semaphore *sem, unsigned int count,
                              unsigned int *prev )
{
    if (prev) *prev = sem->count;
    if (sem->count + count < sem->count || sem->count + count > sem->max)
    {
        set_error( STATUS_SEMAPHORE_LIMIT_EXCEEDED );
        return 0;
    }
    else if (sem->count)
    {
        /* there cannot be any thread to wake up if the count is != 0 */
        sem->count += count;
    }
    else
    {
        sem->count = count;
        wake_up( &sem->obj, count );
    }
    return 1;
}

static void semaphore_dump( struct object *obj, int verbose )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    fprintf( stderr, "Semaphore count=%d max=%d ", sem->count, sem->max );
    dump_object_name( &sem->obj );
    fputc( '\n', stderr );
}

static int semaphore_signaled( struct object *obj, struct thread *thread )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    return (sem->count > 0);
}

static int semaphore_satisfied( struct object *obj, struct thread *thread )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    assert( sem->count );
    sem->count--;
    return 0;  /* not abandoned */
}

static int semaphore_signal( struct object *obj, unsigned int access )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );

    if (!(access & SEMAPHORE_MODIFY_STATE))
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    return release_semaphore( sem, 1, NULL );
}

/* create a semaphore */
DECL_HANDLER(create_semaphore)
{
    struct semaphore *sem;
    struct unicode_str name;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if ((sem = create_semaphore( &name, req->attributes, req->initial, req->max )))
    {
        reply->handle = alloc_handle( current->process, sem, req->access,
                                      req->attributes & OBJ_INHERIT );
        release_object( sem );
    }
}

/* open a handle to a semaphore */
DECL_HANDLER(open_semaphore)
{
    struct unicode_str name;

    get_req_unicode_str( &name );
    reply->handle = open_object( sync_namespace, &name, &semaphore_ops, req->access, req->attributes );
}

/* release a semaphore */
DECL_HANDLER(release_semaphore)
{
    struct semaphore *sem;

    if ((sem = (struct semaphore *)get_handle_obj( current->process, req->handle,
                                                   SEMAPHORE_MODIFY_STATE, &semaphore_ops )))
    {
        release_semaphore( sem, req->count, &reply->prev_count );
        release_object( sem );
    }
}
