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

#include "handle.h"
#include "thread.h"
#include "request.h"

struct change
{
    struct object  obj;      /* object header */
    int            subtree;  /* watch all the subtree */
    int            filter;   /* notification filter */
};

static void change_dump( struct object *obj, int verbose );
static int change_signaled( struct object *obj, struct thread *thread );

static const struct object_ops change_ops =
{
    sizeof(struct change),
    change_dump,
    add_queue,
    remove_queue,
    change_signaled,
    no_satisfied,
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    no_destroy
};


static struct object *create_change_notification( int subtree, int filter )
{
    struct change *change;
    if (!(change = alloc_object( &change_ops ))) return NULL;
    change->subtree = subtree;
    change->filter  = filter;
    return &change->obj;
}

static void change_dump( struct object *obj, int verbose )
{
    struct change *change = (struct change *)obj;
    assert( obj->ops == &change_ops );
    fprintf( stderr, "Change notification sub=%d filter=%08x\n",
             change->subtree, change->filter );
}

static int change_signaled( struct object *obj, struct thread *thread )
{
/*    struct change *change = (struct change *)obj;*/
    assert( obj->ops == &change_ops );
    return 0;  /* never signaled for now */
}

/* create a change notification */
DECL_HANDLER(create_change_notification)
{
    struct object *obj;
    struct create_change_notification_reply *reply = push_reply_data( current, sizeof(*reply) );

    if ((obj = create_change_notification( req->subtree, req->filter )))
    {
        reply->handle = alloc_handle( current->process, obj,
                                      STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE, 0 );
        release_object( obj );
    }
}
