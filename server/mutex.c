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
    mutex_dump,
    add_queue,
    remove_queue,
    mutex_signaled,
    mutex_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    mutex_destroy
};


static struct object *create_mutex( const char *name, int owned )
{
    struct mutex *mutex;

    if (!(mutex = (struct mutex *)create_named_object( name, &mutex_ops, sizeof(*mutex) )))
        return NULL;
    if (GET_ERROR() != ERROR_ALREADY_EXISTS)
    {
        /* initialize it if it didn't already exist */
        mutex->count = 0;
        mutex->owner = NULL;
        mutex->abandoned = 0;
        mutex->next = mutex->prev = NULL;
        if (owned) mutex_satisfied( &mutex->obj, current );
    }
    return &mutex->obj;
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

static int release_mutex( int handle )
{
    struct mutex *mutex;

    if (!(mutex = (struct mutex *)get_handle_obj( current->process, handle,
                                                  MUTEX_MODIFY_STATE, &mutex_ops )))
        return 0;
    if (!mutex->count || (mutex->owner != current))
    {
        SET_ERROR( ERROR_NOT_OWNER );
        return 0;
    }
    if (!--mutex->count) do_release( mutex, current );
    release_object( mutex );
    return 1;
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

static void mutex_destroy( struct object *obj )
{
    struct mutex *mutex = (struct mutex *)obj;
    assert( obj->ops == &mutex_ops );
    free( mutex );
}

/* create a mutex */
DECL_HANDLER(create_mutex)
{
    struct create_mutex_reply reply = { -1 };
    struct object *obj;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_mutex", name, len );

    obj = create_mutex( name, req->owned );
    if (obj)
    {
        reply.handle = alloc_handle( current->process, obj, MUTEX_ALL_ACCESS, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* open a handle to a mutex */
DECL_HANDLER(open_mutex)
{
    struct open_mutex_reply reply;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "open_mutex", name, len );

    reply.handle = open_object( name, &mutex_ops, req->access, req->inherit );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* release a mutex */
DECL_HANDLER(release_mutex)
{
    if (release_mutex( req->handle )) CLEAR_ERROR();
    send_reply( current, -1, 0 );
}
