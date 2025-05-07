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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "thread.h"
#include "request.h"
#include "security.h"

static const WCHAR semaphore_name[] = {'S','e','m','a','p','h','o','r','e'};

struct type_descr semaphore_type =
{
    { semaphore_name, sizeof(semaphore_name) },   /* name */
    SEMAPHORE_ALL_ACCESS,                         /* valid_access */
    {                                             /* mapping */
        STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE,
        STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE,
        STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
        SEMAPHORE_ALL_ACCESS
    },
};

struct semaphore_sync
{
    struct object       obj;                /* object header */
    unsigned int        count;              /* current count */
    unsigned int        max;                /* maximum possible count */
};

static void semaphore_sync_dump( struct object *obj, int verbose );
static int semaphore_sync_signaled( struct object *obj, struct wait_queue_entry *entry );
static void semaphore_sync_satisfied( struct object *obj, struct wait_queue_entry *entry );

static const struct object_ops semaphore_sync_ops =
{
    sizeof(struct semaphore_sync), /* size */
    &no_type,                      /* type */
    semaphore_sync_dump,           /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    semaphore_sync_signaled,       /* signaled */
    semaphore_sync_satisfied,      /* satisfied */
    no_signal,                     /* signal */
    no_get_fd,                     /* get_fd */
    default_get_sync,              /* get_sync */
    default_map_access,            /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    default_get_full_name,         /* get_full_name */
    no_lookup_name,                /* lookup_name */
    directory_link_name,           /* link_name */
    default_unlink_name,           /* unlink_name */
    no_open_file,                  /* open_file */
    no_kernel_obj_list,            /* get_kernel_obj_list */
    no_close_handle,               /* close_handle */
    no_destroy                     /* destroy */
};

static int release_semaphore( struct semaphore_sync *sem, unsigned int count,
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

static void semaphore_sync_dump( struct object *obj, int verbose )
{
    struct semaphore_sync *sem = (struct semaphore_sync *)obj;
    assert( obj->ops == &semaphore_sync_ops );
    fprintf( stderr, "Semaphore count=%d max=%d\n", sem->count, sem->max );
}

static int semaphore_sync_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct semaphore_sync *sem = (struct semaphore_sync *)obj;
    assert( obj->ops == &semaphore_sync_ops );
    return (sem->count > 0);
}

static void semaphore_sync_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
    struct semaphore_sync *sem = (struct semaphore_sync *)obj;
    assert( obj->ops == &semaphore_sync_ops );
    assert( sem->count );
    sem->count--;
}

static struct semaphore_sync *create_semaphore_sync( unsigned int initial, unsigned int max )
{
    struct semaphore_sync *sem;

    if (!(sem = alloc_object( &semaphore_sync_ops ))) return NULL;
    sem->count = initial;
    sem->max   = max;
    return sem;
}

struct semaphore
{
    struct object          obj;    /* object header */
    struct semaphore_sync *sync;   /* semaphore sync object */
};

static void semaphore_dump( struct object *obj, int verbose );
static struct object *semaphore_get_sync( struct object *obj );
static int semaphore_signal( struct object *obj, unsigned int access );
static void semaphore_destroy( struct object *obj );

static const struct object_ops semaphore_ops =
{
    sizeof(struct semaphore),      /* size */
    &semaphore_type,               /* type */
    semaphore_dump,                /* dump */
    NULL,                          /* add_queue */
    NULL,                          /* remove_queue */
    NULL,                          /* signaled */
    NULL,                          /* satisfied */
    semaphore_signal,              /* signal */
    no_get_fd,                     /* get_fd */
    semaphore_get_sync,            /* get_sync */
    default_map_access,            /* map_access */
    default_get_sd,                /* get_sd */
    default_set_sd,                /* set_sd */
    default_get_full_name,         /* get_full_name */
    no_lookup_name,                /* lookup_name */
    directory_link_name,           /* link_name */
    default_unlink_name,           /* unlink_name */
    no_open_file,                  /* open_file */
    no_kernel_obj_list,            /* get_kernel_obj_list */
    no_close_handle,               /* close_handle */
    semaphore_destroy,             /* destroy */
};

static struct semaphore *create_semaphore( struct object *root, const struct unicode_str *name,
                                           unsigned int attr, unsigned int initial, unsigned int max,
                                           const struct security_descriptor *sd )
{
    struct semaphore *sem;

    if (!max || (initial > max))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    if ((sem = create_named_object( root, &semaphore_ops, name, attr, sd )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            sem->sync = NULL;

            if (!(sem->sync = create_semaphore_sync( initial, max )))
            {
                release_object( sem );
                return NULL;
            }
        }
    }
    return sem;
}

static void semaphore_dump( struct object *obj, int verbose )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    sem->sync->obj.ops->dump( &sem->sync->obj, verbose );
}

static struct object *semaphore_get_sync( struct object *obj )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    return grab_object( sem->sync );
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
    return release_semaphore( sem->sync, 1, NULL );
}

static void semaphore_destroy( struct object *obj )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    if (sem->sync) release_object( sem->sync );
}

/* create a semaphore */
DECL_HANDLER(create_semaphore)
{
    struct semaphore *sem;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((sem = create_semaphore( root, &name, objattr->attributes, req->initial, req->max, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, sem, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, sem,
                                                          req->access, objattr->attributes );
        release_object( sem );
    }

    if (root) release_object( root );
}

/* open a handle to a semaphore */
DECL_HANDLER(open_semaphore)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &semaphore_ops, &name, req->attributes );
}

/* release a semaphore */
DECL_HANDLER(release_semaphore)
{
    struct semaphore *sem;

    if ((sem = (struct semaphore *)get_handle_obj( current->process, req->handle,
                                                   SEMAPHORE_MODIFY_STATE, &semaphore_ops )))
    {
        release_semaphore( sem->sync, req->count, &reply->prev_count );
        release_object( sem );
    }
}

/* query details about the semaphore */
DECL_HANDLER(query_semaphore)
{
    struct semaphore *sem;

    if ((sem = (struct semaphore *)get_handle_obj( current->process, req->handle,
                                                   SEMAPHORE_QUERY_STATE, &semaphore_ops )))
    {
        reply->current = sem->sync->count;
        reply->max = sem->sync->max;
        release_object( sem );
    }
}
