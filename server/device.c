/*
 * Server-side device management
 *
 * Copyright (C) 1999 Alexandre Julliard
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

/*
 * FIXME:
 * all this stuff is a simple hack to avoid breaking
 * client-side device support.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winbase.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

struct device
{
    struct object  obj;             /* object header */
    int            id;              /* client identifier */
};

static void device_dump( struct object *obj, int verbose );

static const struct object_ops device_ops =
{
    sizeof(struct device),    /* size */
    device_dump,              /* dump */
    no_add_queue,             /* add_queue */
    NULL,                     /* remove_queue */
    NULL,                     /* signaled */
    NULL,                     /* satisfied */
    no_get_fd,                /* get_fd */
    no_destroy                /* destroy */
};

static struct device *create_device( int id )
{
    struct device *dev;
    if ((dev = alloc_object( &device_ops, -1 )))
    {
        dev->id = id;
    }
    return dev;
}

static void device_dump( struct object *obj, int verbose )
{
    struct device *dev = (struct device *)obj;
    assert( obj->ops == &device_ops );
    fprintf( stderr, "Device id=%08x\n", dev->id );
}

/* create a device */
DECL_HANDLER(create_device)
{
    struct device *dev;

    reply->handle = 0;
    if ((dev = create_device( req->id )))
    {
        reply->handle = alloc_handle( current->process, dev, req->access, req->inherit );
        release_object( dev );
    }
}


/* Retrieve the client private id for a device */
DECL_HANDLER(get_device_id)
{
    struct device *dev;

    if ((dev = (struct device *)get_handle_obj( current->process, req->handle, 0, &device_ops )))
    {
        reply->id = dev->id;
        release_object( dev );
    }
}
