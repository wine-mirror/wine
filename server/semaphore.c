/*
 * Server-side semaphore management
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
    sizeof(struct semaphore),      /* size */
    semaphore_dump,                /* dump */
    add_queue,                     /* add_queue */
    remove_queue,                  /* remove_queue */
    semaphore_signaled,            /* signaled */
    semaphore_satisfied,           /* satisfied */
    NULL,                          /* get_poll_events */
    NULL,                          /* poll_event */
    no_read_fd,                    /* get_read_fd */
    no_write_fd,                   /* get_write_fd */
    no_flush,                      /* flush */
    no_get_file_info,              /* get_file_info */
    no_destroy                     /* destroy */
};


static struct semaphore *create_semaphore( const WCHAR *name, size_t len,
                                           unsigned int initial, unsigned int max )
{
    struct semaphore *sem;

    if (!max || (initial > max))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }
    if ((sem = create_named_object( &semaphore_ops, name, len )))
    {
        if (get_error() != STATUS_OBJECT_NAME_COLLISION)
        {
            /* initialize it if it didn't already exist */
            sem->count = initial;
            sem->max   = max;
        }
    }
    return sem;
}

static unsigned int release_semaphore( int handle, unsigned int count )
{
    struct semaphore *sem;
    unsigned int prev = 0;

    if ((sem = (struct semaphore *)get_handle_obj( current->process, handle,
                                                   SEMAPHORE_MODIFY_STATE, &semaphore_ops )))
    {
        prev = sem->count;
        if (sem->count + count < sem->count || sem->count + count > sem->max)
        {
            set_error( STATUS_SEMAPHORE_LIMIT_EXCEEDED );
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
    return prev;
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

/* create a semaphore */
DECL_HANDLER(create_semaphore)
{
    size_t len = get_req_strlenW( req->name );
    struct semaphore *sem;

    req->handle = -1;
    if ((sem = create_semaphore( req->name, len, req->initial, req->max )))
    {
        req->handle = alloc_handle( current->process, sem, SEMAPHORE_ALL_ACCESS, req->inherit );
        release_object( sem );
    }
}

/* open a handle to a semaphore */
DECL_HANDLER(open_semaphore)
{
    size_t len = get_req_strlenW( req->name );
    req->handle = open_object( req->name, len, &semaphore_ops, req->access, req->inherit );
}

/* release a semaphore */
DECL_HANDLER(release_semaphore)
{
    req->prev_count = release_semaphore( req->handle, req->count );
}
