/*
 * Server-side mutex management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
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

static const struct object_ops mutex_ops =
{
    sizeof(struct mutex),
    mutex_dump,
    add_queue,
    remove_queue,
    mutex_signaled,
    mutex_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    no_destroy
};


static struct mutex *create_mutex( const char *name, size_t len, int owned )
{
    struct mutex *mutex;

    if ((mutex = create_named_object( &mutex_ops, name, len )))
    {
        if (get_error() != ERROR_ALREADY_EXISTS)
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
static void do_release( struct mutex *mutex, struct thread *thread )
{
    assert( !mutex->count );
    /* remove the mutex from the thread list of owned mutexes */
    if (mutex->next) mutex->next->prev = mutex->prev;
    if (mutex->prev) mutex->prev->next = mutex->next;
    else thread->mutex = mutex->next;
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
        do_release( mutex, thread );
    }
}

static void mutex_dump( struct object *obj, int verbose )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );
    printf( "Mutex count=%u owner=%p name='%s'\n",
            mutex->count, mutex->owner, get_object_name( &mutex->obj) );
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

/* create a mutex */
DECL_HANDLER(create_mutex)
{
    size_t len = get_req_strlen();
    struct create_mutex_reply *reply = push_reply_data( current, sizeof(*reply) );
    struct mutex *mutex;

    if ((mutex = create_mutex( get_req_data( len + 1 ), len, req->owned )))
    {
        reply->handle = alloc_handle( current->process, mutex, MUTEX_ALL_ACCESS, req->inherit );
        release_object( mutex );
    }
    else reply->handle = -1;
}

/* open a handle to a mutex */
DECL_HANDLER(open_mutex)
{
    size_t len = get_req_strlen();
    struct open_mutex_reply *reply = push_reply_data( current, sizeof(*reply) );
    reply->handle = open_object( get_req_data( len + 1 ), len, &mutex_ops,
                                 req->access, req->inherit );
}

/* release a mutex */
DECL_HANDLER(release_mutex)
{
    struct mutex *mutex;

    if ((mutex = (struct mutex *)get_handle_obj( current->process, req->handle,
                                                 MUTEX_MODIFY_STATE, &mutex_ops )))
    {
        if (!mutex->count || (mutex->owner != current)) set_error( ERROR_NOT_OWNER );
        else if (!--mutex->count) do_release( mutex, current );
        release_object( mutex );
    }
}
