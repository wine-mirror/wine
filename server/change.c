/*
 * Server-side change notification management
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
#include "winnt.h"
#include "server/thread.h"

struct change
{
    struct object  obj;      /* object header */
    int            subtree;  /* watch all the subtree */
    int            filter;   /* notification filter */
};

static void change_dump( struct object *obj, int verbose );
static int change_signaled( struct object *obj, struct thread *thread );
static void change_destroy( struct object *obj );

static const struct object_ops change_ops =
{
    change_dump,
    add_queue,
    remove_queue,
    change_signaled,
    no_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    change_destroy
};


struct object *create_change_notification( int subtree, int filter )
{
    struct change *change;
    if (!(change = mem_alloc( sizeof(*change) ))) return NULL;
    init_object( &change->obj, &change_ops, NULL );
    change->subtree = subtree;
    change->filter  = filter;
    return &change->obj;
}

static void change_dump( struct object *obj, int verbose )
{
    struct change *change = (struct change *)obj;
    assert( obj->ops == &change_ops );
    printf( "Change notification sub=%d filter=%08x\n",
            change->subtree, change->filter );
}

static int change_signaled( struct object *obj, struct thread *thread )
{
/*    struct change *change = (struct change *)obj;*/
    assert( obj->ops == &change_ops );
    return 0;  /* never signaled for now */
}

static void change_destroy( struct object *obj )
{
    struct change *change = (struct change *)obj;
    assert( obj->ops == &change_ops );
    free( change );
}
