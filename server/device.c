/*
 * Server-side device management
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include "winerror.h"
#include "winbase.h"
#include "server/thread.h"

struct device
{
    struct object  obj;             /* object header */
    int            id;              /* client identifier */
};

static void device_dump( struct object *obj, int verbose );
static int device_get_info( struct object *obj, struct get_file_info_reply *reply );
static void device_destroy( struct object *obj );

static const struct object_ops device_ops =
{
    device_dump,
    no_add_queue,
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    no_read_fd,
    no_write_fd,
    no_flush,
    device_get_info,
    device_destroy
};

struct object *create_device( int id )
{
    struct device *dev;

    if (!(dev = mem_alloc(sizeof(*dev)))) return NULL;
    init_object( &dev->obj, &device_ops, NULL );
    dev->id = id;
    return &dev->obj;
}

static void device_dump( struct object *obj, int verbose )
{
    struct device *dev = (struct device *)obj;
    assert( obj->ops == &device_ops );
    fprintf( stderr, "Device id=%08x\n", dev->id );
}

static int device_get_info( struct object *obj, struct get_file_info_reply *reply )
{
    struct device *dev = (struct device *)obj;
    assert( obj->ops == &device_ops );
    memset( reply, 0, sizeof(*reply) );
    reply->type = FILE_TYPE_UNKNOWN;
    reply->attr = dev->id;  /* hack! */
    return 1;
}

static void device_destroy( struct object *obj )
{
    struct device *dev = (struct device *)obj;
    assert( obj->ops == &device_ops );
    free( dev );
}
