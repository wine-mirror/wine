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
#include "server/thread.h"

struct semaphore
{
    struct object  obj;    /* object header */
    unsigned int   count;  /* current count */
    unsigned int   max;    /* maximum possible count */
};

static void dump_semaphore( struct object *obj, int verbose );
static int semaphore_signaled( struct object *obj, struct thread *thread );
static int semaphore_satisfied( struct object *obj, struct thread *thread );
static void destroy_semaphore( struct object *obj );

static const struct object_ops semaphore_ops =
{
    dump_semaphore,
    semaphore_signaled,
    semaphore_satisfied,
    destroy_semaphore
};


struct object *create_semaphore( const char *name, unsigned int initial, unsigned int max )
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

int open_semaphore( unsigned int access, int inherit, const char *name )
{
    return open_object( name, &semaphore_ops, access, inherit );
}

int release_semaphore( int handle, unsigned int count, unsigned int *prev_count )
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

static void dump_semaphore( struct object *obj, int verbose )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    printf( "Semaphore count=%d max=%d\n", sem->count, sem->max );
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

static void destroy_semaphore( struct object *obj )
{
    struct semaphore *sem = (struct semaphore *)obj;
    assert( obj->ops == &semaphore_ops );
    free( sem );
}
