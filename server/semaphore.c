/*
 * Server-side semaphore management
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

struct semaphore
{
    struct object  obj;    /* object header */
    unsigned int   count;  /* current count */
    unsigned int   max;    /* maximum possible count */
};

static void semaphore_dump( struct object *obj, int verbose );
static int semaphore_signaled( struct object *obj, struct thread *thread );
static int semaphore_satisfied( struct object *obj, struct thread *thread );
static void semaphore_destroy( struct object *obj );

static const struct object_ops semaphore_ops =
{
    semaphore_dump,
    add_queue,
    remove_queue,
    semaphore_signaled,
    semaphore_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    semaphore_destroy
};


static struct object *create_semaphore( const char *name, unsigned int initial, unsigned int max )
{
    struct semaphore *sem;

    if (!max || (initial > max))
    {
        SET_ERROR( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!(sem = (struct semaphore *)create_named_object( name, &semaphore_ops, sizeof(*sem) )))
        return NULL;
    if (GET_ERROR() != ERROR_ALREADY_EXISTS)
    {
        /* initialize it if it didn't already exist */
        sem->count = initial;
        sem->max   = max;
    }
    return &sem->obj;
}

static int release_semaphore( int handle, unsigned int count, unsigned int *prev_count )
{
    struct semaphore *sem;

    if (!(sem = (struct semaphore *)get_handle_obj( current->process, handle,
                                                    SEMAPHORE_MODIFY_STATE, &semaphore_ops )))
        return 0;

    *prev_count = sem->count;
    if (sem->count + count < sem->count || sem->count + count > sem->max)
    {
        SET_ERROR( ERROR_TOO_MANY_POSTS );
        return 0;
    }
    if (sem->count)
    {
        /* there cannot be any thread waiting if the count is != 0 */
        assert( !sem->obj.head );
        sem->count += count;
    }
    else
    {
        sem->count = count;
        wake_up( &sem->obj, count );
    }
    release_object( sem );
    return 1;
}

static void semaphore_dump( struct object *obj, int verbose )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    fprintf( stderr, "Semaphore count=%d max=%d name='%s'\n",
             sem->count, sem->max, get_object_name( &sem->obj ) );
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

static void semaphore_destroy( struct object *obj )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    free( sem );
}

/* create a semaphore */
DECL_HANDLER(create_semaphore)
{
    struct create_semaphore_reply reply = { -1 };
    struct object *obj;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_semaphore", name, len );

    obj = create_semaphore( name, req->initial, req->max );
    if (obj)
    {
        reply.handle = alloc_handle( current->process, obj, SEMAPHORE_ALL_ACCESS, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* open a handle to a semaphore */
DECL_HANDLER(open_semaphore)
{
    struct open_semaphore_reply reply;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "open_semaphore", name, len );

    reply.handle = open_object( name, &semaphore_ops, req->access, req->inherit );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* release a semaphore */
DECL_HANDLER(release_semaphore)
{
    struct release_semaphore_reply reply;
    if (release_semaphore( req->handle, req->count, &reply.prev_count )) CLEAR_ERROR();
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}
