/*
 * Server-side mutex management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "winnt.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

struct mutex
{
    struct object  obj;             /* object header */
    struct thread *owner;           /* mutex owner */
    unsigned int   count;           /* recursion count */
    int            abandoned;       /* has it been abandoned? */
    struct mutex  *next;
    struct mutex  *prev;
};

static void mutex_dump( struct object *obj, int verbose );
static int mutex_signaled( struct object *obj, struct thread *thread );
static int mutex_satisfied( struct object *obj, struct thread *thread );
static void mutex_destroy( struct object *obj );

static const struct object_ops mutex_ops =
{
    sizeof(struct mutex),      /* size */
    mutex_dump,                /* dump */
    add_queue,                 /* add_queue */
    remove_queue,              /* remove_queue */
    mutex_signaled,            /* signaled */
    mutex_satisfied,           /* satisfied */
    NULL,                      /* get_poll_events */
    NULL,                      /* poll_event */
    no_read_fd,                /* get_read_fd */
    no_write_fd,               /* get_write_fd */
    no_flush,                  /* flush */
    no_get_file_info,          /* get_file_info */
    mutex_destroy              /* destroy */
};


static struct mutex *create_mutex( const WCHAR *name, size_t len, int owned )
{
    struct mutex *mutex;

    if ((mutex = create_named_object( &mutex_ops, name, len )))
    {
        if (get_error() != STATUS_OBJECT_NAME_COLLISION)
        {
            /* initialize it if it didn't already exist */
            mutex->count = 0;
            mutex->owner = NULL;
            mutex->abandoned = 0;
            mutex->next = mutex->prev = NULL;
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
    if (mutex->next) mutex->next->prev = mutex->prev;
    if (mutex->prev) mutex->prev->next = mutex->next;
    else mutex->owner->mutex = mutex->next;
    mutex->owner = NULL;
    mutex->next = mutex->prev = NULL;
    wake_up( &mutex->obj, 0 );
}

void abandon_mutexes( struct thread *thread )
{
    while (thread->mutex)
    {
        struct mutex *mutex = thread->mutex;
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
        mutex->prev  = NULL;
        if ((mutex->next = thread->mutex)) mutex->next->prev = mutex;
        thread->mutex = mutex;
    }
    if (!mutex->abandoned) return 0;
    mutex->abandoned = 0;
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
    size_t len = get_req_strlenW( req, req->name );
    struct mutex *mutex;

    req->handle = -1;
    if ((mutex = create_mutex( req->name, len, req->owned )))
    {
        req->handle = alloc_handle( current->process, mutex, MUTEX_ALL_ACCESS, req->inherit );
        release_object( mutex );
    }
}

/* open a handle to a mutex */
DECL_HANDLER(open_mutex)
{
    size_t len = get_req_strlenW( req, req->name );
    req->handle = open_object( req->name, len, &mutex_ops, req->access, req->inherit );
}

/* release a mutex */
DECL_HANDLER(release_mutex)
{
    struct mutex *mutex;

    if ((mutex = (struct mutex *)get_handle_obj( current->process, req->handle,
                                                 MUTEX_MODIFY_STATE, &mutex_ops )))
    {
        if (!mutex->count || (mutex->owner != current)) set_error( STATUS_MUTANT_NOT_OWNED );
        else if (!--mutex->count) do_release( mutex );
        release_object( mutex );
    }
}
