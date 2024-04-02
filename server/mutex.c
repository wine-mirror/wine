/*
 * Server-side mutex management
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

static const WCHAR mutex_name[] = {'M','u','t','a','n','t'};

struct type_descr mutex_type =
{
    { mutex_name, sizeof(mutex_name) },   /* name */
    MUTANT_ALL_ACCESS,                    /* valid_access */
    {                                     /* mapping */
        STANDARD_RIGHTS_READ | MUTANT_QUERY_STATE,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
        MUTANT_ALL_ACCESS
    },
};

struct mutex
{
    struct object  obj;             /* object header */
    struct thread *owner;           /* mutex owner */
    unsigned int   count;           /* recursion count */
    int            abandoned;       /* has it been abandoned? */
    struct list    entry;           /* entry in owner thread mutex list */
};

static void mutex_dump( struct object *obj, int verbose );
static int mutex_signaled( struct object *obj, struct wait_queue_entry *entry );
static void mutex_satisfied( struct object *obj, struct wait_queue_entry *entry );
static void mutex_destroy( struct object *obj );
static int mutex_signal( struct object *obj, unsigned int access );

static const struct object_ops mutex_ops =
{
    sizeof(struct mutex),      /* size */
    &mutex_type,               /* type */
    mutex_dump,                /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    mutex_signaled,            /* signaled */
    NULL,                      /* get_esync_fd */
    mutex_satisfied,           /* satisfied */
    mutex_signal,              /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    directory_link_name,       /* link_name */
    default_unlink_name,       /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    mutex_destroy              /* destroy */
};


/* grab a mutex for a given thread */
static void do_grab( struct mutex *mutex, struct thread *thread )
{
    assert( !mutex->count || (mutex->owner == thread) );

    if (!mutex->count++)  /* FIXME: avoid wrap-around */
    {
        assert( !mutex->owner );
        mutex->owner = thread;
        list_add_head( &thread->mutex_list, &mutex->entry );
    }
}

/* release a mutex once the recursion count is 0 */
static void do_release( struct mutex *mutex )
{
    assert( !mutex->count );
    /* remove the mutex from the thread list of owned mutexes */
    list_remove( &mutex->entry );
    mutex->owner = NULL;
    wake_up( &mutex->obj, 0 );
}

static struct mutex *create_mutex( struct object *root, const struct unicode_str *name,
                                   unsigned int attr, int owned, const struct security_descriptor *sd )
{
    struct mutex *mutex;

    if ((mutex = create_named_object( root, &mutex_ops, name, attr, sd )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            mutex->count = 0;
            mutex->owner = NULL;
            mutex->abandoned = 0;
            if (owned) do_grab( mutex, current );
        }
    }
    return mutex;
}

void abandon_mutexes( struct thread *thread )
{
    struct list *ptr;

    while ((ptr = list_head( &thread->mutex_list )) != NULL)
    {
        struct mutex *mutex = LIST_ENTRY( ptr, struct mutex, entry );
        assert( mutex->owner == thread );
        mutex->count = 0;
        mutex->abandoned = 1;
        do_release( mutex );
    }
}

static void mutex_dump( struct object *obj, int verbose )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );
    fprintf( stderr, "Mutex count=%u owner=%p\n", mutex->count, mutex->owner );
}

static int mutex_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );
    return (!mutex->count || (mutex->owner == get_wait_queue_thread( entry )));
}

static void mutex_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );

    do_grab( mutex, get_wait_queue_thread( entry ));
    if (mutex->abandoned) make_wait_abandoned( entry );
    mutex->abandoned = 0;
}

static int mutex_signal( struct object *obj, unsigned int access )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );

    if (!(access & SYNCHRONIZE))
    {
        set_error( STATUS_ACCESS_DENIED );
        return 0;
    }
    if (!mutex->count || (mutex->owner != current))
    {
        set_error( STATUS_MUTANT_NOT_OWNED );
        return 0;
    }
    if (!--mutex->count) do_release( mutex );
    return 1;
}

static void mutex_destroy( struct object *obj )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );

    if (!mutex->count) return;
    mutex->count = 0;
    do_release( mutex );
}

/* create a mutex */
DECL_HANDLER(create_mutex)
{
    struct mutex *mutex;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((mutex = create_mutex( root, &name, objattr->attributes, req->owned, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, mutex, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, mutex,
                                                          req->access, objattr->attributes );
        release_object( mutex );
    }

    if (root) release_object( root );
}

/* open a handle to a mutex */
DECL_HANDLER(open_mutex)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &mutex_ops, &name, req->attributes );
}

/* release a mutex */
DECL_HANDLER(release_mutex)
{
    struct mutex *mutex;

    if ((mutex = (struct mutex *)get_handle_obj( current->process, req->handle,
                                                 0, &mutex_ops )))
    {
        if (!mutex->count || (mutex->owner != current)) set_error( STATUS_MUTANT_NOT_OWNED );
        else
        {
            reply->prev_count = mutex->count;
            if (!--mutex->count) do_release( mutex );
        }
        release_object( mutex );
    }
}

/* return details about the mutex */
DECL_HANDLER(query_mutex)
{
    struct mutex *mutex;

    if ((mutex = (struct mutex *)get_handle_obj( current->process, req->handle,
                                                 MUTANT_QUERY_STATE, &mutex_ops )))
    {
        reply->count = mutex->count;
        reply->owned = (mutex->owner == current);
        reply->abandoned = mutex->abandoned;

        release_object( mutex );
    }
}
