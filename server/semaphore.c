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

static const struct object_ops semaphore_ops =
{
    sizeof(struct semaphore),
    semaphore_dump,
    add_queue,
    remove_queue,
    semaphore_signaled,
    semaphore_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    no_destroy
};


static struct semaphore *create_semaphore( const char *name, size_t len,
                                           unsigned int initial, unsigned int max )
{
    struct semaphore *sem;

    if (!max || (initial > max))
    {
        set_error( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if ((sem = create_named_object( &semaphore_ops, name, len )))
    {
        if (get_error() != ERROR_ALREADY_EXISTS)
        {
            /* initialize it if it didn't already exist */
            sem->count = initial;
            sem->max   = max;
        }
    }
    return sem;
}

static void release_semaphore( int handle, unsigned int count, unsigned int *prev_count )
{
    struct semaphore *sem;

    if (!(sem = (struct semaphore *)get_handle_obj( current->process, handle,
                                                    SEMAPHORE_MODIFY_STATE, &semaphore_ops )))
        return;

    *prev_count = sem->count;
    if (sem->count + count < sem->count || sem->count + count > sem->max)
    {
        set_error( ERROR_TOO_MANY_POSTS );
    }
    else if (sem->count)
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

/* create a semaphore */
DECL_HANDLER(create_semaphore)
{
    size_t len = get_req_strlen();
    struct create_semaphore_reply *reply = push_reply_data( current, sizeof(*reply) );
    struct semaphore *sem;

    if ((sem = create_semaphore( get_req_data( len + 1 ), len, req->initial, req->max )))
    {
        reply->handle = alloc_handle( current->process, sem, SEMAPHORE_ALL_ACCESS, req->inherit );
        release_object( sem );
    }
    else reply->handle = -1;
}

/* open a handle to a semaphore */
DECL_HANDLER(open_semaphore)
{
    size_t len = get_req_strlen();
    struct open_semaphore_reply *reply = push_reply_data( current, sizeof(*reply) );
    reply->handle = open_object( get_req_data( len + 1 ), len, &semaphore_ops,
                                 req->access, req->inherit );
}

/* release a semaphore */
DECL_HANDLER(release_semaphore)
{
    struct release_semaphore_reply *reply = push_reply_data( current, sizeof(*reply) );
    release_semaphore( req->handle, req->count, &reply->prev_count );
}
