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

struct mutex
{
    struct object  obj;             /* object header */
    struct thread *owner;           /* mutex owner */
    unsigned int   count;           /* recursion count */
    int            abandoned;       /* has it been abandoned? */
    struct list    entry;           /* entry in owner thread mutex list */
};

static void mutex_dump( struct object *obj, int verbose );
static int mutex_signaled( struct object *obj, struct thread *thread );
static int mutex_satisfied( struct object *obj, struct thread *thread );
static unsigned int mutex_map_access( struct object *obj, unsigned int access );
static void mutex_destroy( struct object *obj );
static int mutex_signal( struct object *obj, unsigned int access );

static const struct object_ops mutex_ops =
{
    sizeof(struct mutex),      /* size */
    mutex_dump,                /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    mutex_signaled,            /* signaled */
    mutex_satisfied,           /* satisfied */
    mutex_signal,              /* signal */
    no_get_fd,                 /* get_fd */
    mutex_map_access,          /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_lookup_name,            /* lookup_name */
    no_open_file,              /* open_file */
    no_close_handle,           /* close_handle */
    mutex_destroy              /* destroy */
};


static struct mutex *create_mutex( struct directory *root, const struct unicode_str *name,
                                   unsigned int attr, int owned )
{
    struct mutex *mutex;

    if ((mutex = create_named_object_dir( root, name, attr, &mutex_ops )))
    {
        if (get_error() != STATUS_OBJECT_NAME_EXISTS)
        {
            /* initialize it if it didn't already exist */
            mutex->count = 0;
            mutex->owner = NULL;
            mutex->abandoned = 0;
            if (owned) mutex_satisfied( &mutex->obj, current );
        }
    }
    return mutex;
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
    fprintf( stderr, "Mutex count=%u owner=%p ", mutex->count, mutex->owner );
    dump_object_name( &mutex->obj );
    fputc( '\n', stderr );
}

static int mutex_signaled( struct object *obj, struct thread *thread )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );
    return (!mutex->count || (mutex->owner == thread));
}

static int mutex_satisfied( struct object *obj, struct thread *thread )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );
    assert( !mutex->count || (mutex->owner == thread) );

    if (!mutex->count++)  /* FIXME: avoid wrap-around */
    {
        assert( !mutex->owner );
        mutex->owner = thread;
        list_add_head( &thread->mutex_list, &mutex->entry );
    }
    if (!mutex->abandoned) return 0;
    mutex->abandoned = 0;
    return 1;
}

static unsigned int mutex_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SYNCHRONIZE;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | MUTEX_MODIFY_STATE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= STANDARD_RIGHTS_ALL | MUTEX_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static int mutex_signal( struct object *obj, unsigned int access )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );

    if (!(access & SYNCHRONIZE))  /* FIXME: MUTEX_MODIFY_STATE? */
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
    struct directory *root = NULL;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((mutex = create_mutex( root, &name, req->attributes, req->owned )))
    {
        reply->handle = alloc_handle( current->process, mutex, req->access, req->attributes );
        release_object( mutex );
    }

    if (root) release_object( root );
}

/* open a handle to a mutex */
DECL_HANDLER(open_mutex)
{
    struct unicode_str name;
    struct directory *root = NULL;
    struct mutex *mutex;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((mutex = open_object_dir( root, &name, req->attributes, &mutex_ops )))
    {
        reply->handle = alloc_handle( current->process, &mutex->obj, req->access, req->attributes );
        release_object( mutex );
    }

    if (root) release_object( root );
}

/* release a mutex */
DECL_HANDLER(release_mutex)
{
    struct mutex *mutex;

    if ((mutex = (struct mutex *)get_handle_obj( current->process, req->handle,
                                                 MUTEX_MODIFY_STATE, &mutex_ops )))
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
