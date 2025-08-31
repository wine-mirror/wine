/*
 * Server-side D3DKMT resource management
 *
 * Copyright 2025 Rémi Bernon for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"

#include "file.h"
#include "handle.h"
#include "request.h"
#include "security.h"

struct d3dkmt_object
{
    struct object       obj;            /* object header */
    enum d3dkmt_type    type;           /* object type */
};

static void d3dkmt_object_dump( struct object *obj, int verbose );
static void d3dkmt_object_destroy( struct object *obj );

static const struct object_ops d3dkmt_object_ops =
{
    sizeof(struct d3dkmt_object),   /* size */
    &no_type,                       /* type */
    d3dkmt_object_dump,             /* dump */
    no_add_queue,                   /* add_queue */
    NULL,                           /* remove_queue */
    NULL,                           /* signaled */
    NULL,                           /* satisfied */
    no_signal,                      /* signal */
    no_get_fd,                      /* get_fd */
    default_get_sync,               /* get_sync */
    default_map_access,             /* map_access */
    default_get_sd,                 /* get_sd */
    default_set_sd,                 /* set_sd */
    no_get_full_name,               /* get_full_name */
    no_lookup_name,                 /* lookup_name */
    no_link_name,                   /* link_name */
    NULL,                           /* unlink_name */
    no_open_file,                   /* open_file */
    no_kernel_obj_list,             /* get_kernel_obj_list */
    no_close_handle,                /* close_handle */
    d3dkmt_object_destroy,          /* destroy */
};

static void d3dkmt_object_dump( struct object *obj, int verbose )
{
    struct d3dkmt_object *object = (struct d3dkmt_object *)obj;
    assert( obj->ops == &d3dkmt_object_ops );

    fprintf( stderr, "type=%#x\n", object->type );
}

static void d3dkmt_object_destroy( struct object *obj )
{
    assert( obj->ops == &d3dkmt_object_ops );
}

static struct d3dkmt_object *d3dkmt_object_create( enum d3dkmt_type type )
{
    struct d3dkmt_object *object;

    if (!(object = alloc_object( &d3dkmt_object_ops ))) return NULL;
    object->type = type;

    return object;
}

/* create a global d3dkmt object */
DECL_HANDLER(d3dkmt_object_create)
{
    struct d3dkmt_object *object;

    if (!(object = d3dkmt_object_create( req->type ))) return;
    reply->handle = alloc_handle( current->process, object, STANDARD_RIGHTS_ALL, OBJ_INHERIT );
    release_object( object );
}
