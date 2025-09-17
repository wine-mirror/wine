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
#include <stdbool.h>
#include <stdio.h>

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
    d3dkmt_handle_t     global;         /* object global handle */
    void               *runtime;        /* client runtime data */
    data_size_t         runtime_size;   /* size of client runtime data */
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

static struct d3dkmt_object **objects, **objects_end, **objects_next;

#define D3DKMT_HANDLE_BIT  0x40000000

static d3dkmt_handle_t index_to_handle( int index )
{
    return (index << 6) | D3DKMT_HANDLE_BIT | 2;
}

static int handle_to_index( d3dkmt_handle_t handle )
{
    return (handle & ~0xc000003f) >> 6;
}

static bool init_handle_table(void)
{
    static const size_t initial_capacity = 1024;

    if (!(objects = mem_alloc( initial_capacity * sizeof(*objects) ))) return false;
    memset( objects, 0, initial_capacity * sizeof(*objects) );
    objects_end = objects + initial_capacity;
    objects_next = objects;

    return true;
}

static struct d3dkmt_object **grow_handle_table(void)
{
    size_t old_capacity = objects_end - objects, max_capacity = handle_to_index( D3DKMT_HANDLE_BIT - 1 );
    unsigned int new_capacity = old_capacity * 3 / 2;
    struct d3dkmt_object **tmp;

    if (new_capacity > max_capacity) new_capacity = max_capacity;
    if (new_capacity <= old_capacity) return NULL; /* exhausted handle capacity */

    if (!(tmp = realloc( objects, new_capacity * sizeof(*objects) ))) return NULL;
    memset( tmp + old_capacity, 0, (new_capacity - old_capacity) * sizeof(*tmp) );

    objects = tmp;
    objects_end = tmp + new_capacity;
    objects_next = tmp + old_capacity;

    return objects_next;
}

/* allocate a d3dkmt object with a global handle */
static d3dkmt_handle_t alloc_object_handle( struct d3dkmt_object *object )
{
    struct d3dkmt_object **entry;
    d3dkmt_handle_t handle = 0;

    if (!objects && !init_handle_table()) goto done;

    for (entry = objects_next; entry < objects_end; entry++) if (!*entry) break;
    if (entry == objects_end)
    {
        for (entry = objects; entry < objects_next; entry++) if (!*entry) break;
        if (entry == objects_next && !(entry = grow_handle_table())) goto done;
    }

    handle = index_to_handle( entry - objects );
    objects_next = entry + 1;
    *entry = object;

done:
    if (!handle) set_error( STATUS_NO_MEMORY );
    return handle;
}

/* free a d3dkmt global object handle */
static void free_object_handle( d3dkmt_handle_t global )
{
    unsigned int index = handle_to_index( global );
    assert( objects + index < objects_end );
    objects[index] = NULL;
}

static void d3dkmt_object_dump( struct object *obj, int verbose )
{
    struct d3dkmt_object *object = (struct d3dkmt_object *)obj;
    assert( obj->ops == &d3dkmt_object_ops );

    fprintf( stderr, "type=%#x global=%#x\n", object->type, object->global );
}

static void d3dkmt_object_destroy( struct object *obj )
{
    struct d3dkmt_object *object = (struct d3dkmt_object *)obj;
    assert( obj->ops == &d3dkmt_object_ops );

    if (object->global) free_object_handle( object->global );
    free( object->runtime );
}

static struct d3dkmt_object *d3dkmt_object_create( enum d3dkmt_type type, data_size_t runtime_size, const void *runtime )
{
    struct d3dkmt_object *object;

    if (!(object = alloc_object( &d3dkmt_object_ops ))) return NULL;
    object->type            = type;
    object->global          = 0;
    object->runtime_size    = runtime_size;

    if (!(object->runtime = memdup( runtime, runtime_size )) ||
        !(object->global = alloc_object_handle( object )))
    {
        release_object( object );
        return NULL;
    }

    return object;
}

/* create a global d3dkmt object */
DECL_HANDLER(d3dkmt_object_create)
{
    struct d3dkmt_object *object;

    if (!(object = d3dkmt_object_create( req->type, get_req_data_size(), get_req_data() ))) return;
    reply->handle = alloc_handle( current->process, object, STANDARD_RIGHTS_ALL, OBJ_INHERIT );
    reply->global = object->global;
    release_object( object );
}
