/*
 * Server-side change notification management
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"

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
    sizeof(struct change),    /* size */
    change_dump,              /* dump */
    add_queue,                /* add_queue */
    remove_queue,             /* remove_queue */
    change_signaled,          /* signaled */
    no_satisfied,             /* satisfied */
    NULL,                     /* get_poll_events */
    NULL,                     /* poll_event */
    no_get_fd,                /* get_fd */
    no_flush,                 /* flush */
    no_get_file_info,         /* get_file_info */
    NULL,                     /* queue_async */
    no_destroy                /* destroy */
};


static struct change *create_change_notification( int subtree, int filter )
{
    struct change *change;
    if ((change = alloc_object( &change_ops, -1 )))
    {
        change->subtree = subtree;
        change->filter  = filter;
    }
    return change;
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
    struct change *change;

    reply->handle = 0;
    if ((change = create_change_notification( req->subtree, req->filter )))
    {
        reply->handle = alloc_handle( current->process, change,
                                      STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE, 0 );
        release_object( change );
    }
}
