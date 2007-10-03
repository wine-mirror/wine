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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
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
static unsigned int semaphore_map_access( struct object *obj, unsigned int access );
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
    semaphore_map_access,          /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    no_lookup_name,                /* lookup_name */
    no_open_file,                  /* open_file */
    no_close_handle,               /* close_handle */
    no_destroy                     /* destroy */
};


static struct semaphore *create_semaphore( struct directory *root, const struct unicode_str *name,
                                           unsigned int attr, unsigned int initial, unsigned int max )
{
    struct semaphore *sem;

    if (!max || (initial > max))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    if ((sem = create_named_object_dir( root, name, attr, &semaphore_ops )))
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

static unsigned int semaphore_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SYNCHRONIZE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_ALL | SEMAPHORE_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
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
    struct directory *root = NULL;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((sem = create_semaphore( root, &name, req->attributes, req->initial, req->max )))
    {
        reply->handle = alloc_handle( current->process, sem, req->access, req->attributes );
        release_object( sem );
    }

    if (root) release_object( root );
}

/* open a handle to a semaphore */
DECL_HANDLER(open_semaphore)
{
    struct unicode_str name;
    struct directory *root = NULL;
    struct semaphore *sem;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((sem = open_object_dir( root, &name, req->attributes, &semaphore_ops )))
    {
        reply->handle = alloc_handle( current->process, &sem->obj, req->access, req->attributes );
        release_object( sem );
    }

    if (root) release_object( root );
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
