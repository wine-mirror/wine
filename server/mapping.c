/*
 * Server-side file mapping management
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "winerror.h"
#include "winnt.h"
#include "server/thread.h"

struct mapping
{
    struct object  obj;             /* object header */
    int            size_high;       /* mapping size */
    int            size_low;        /* mapping size */
    int            protect;         /* protection flags */
    struct file   *file;            /* file mapped */
};

static void mapping_dump( struct object *obj, int verbose );
static void mapping_destroy( struct object *obj );

static const struct object_ops mapping_ops =
{
    mapping_dump,
    no_add_queue,
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    NULL,  /* should never get called */
    no_read_fd,
    no_write_fd,
    no_flush,
    no_get_file_info,
    mapping_destroy
};

struct object *create_mapping( int size_high, int size_low, int protect,
                               int handle, const char *name )
{
    struct mapping *mapping;

    if (!(mapping = (struct mapping *)create_named_object( name, &mapping_ops,
                                                           sizeof(*mapping) )))
        return NULL;
    if (GET_ERROR() == ERROR_ALREADY_EXISTS)
        return &mapping->obj;  /* Nothing else to do */

    mapping->size_high = size_high;
    mapping->size_low  = size_low;
    mapping->protect   = protect;
    if (handle != -1)
    {
        int access = 0;
        if (protect & VPROT_READ) access |= GENERIC_READ;
        if (protect & VPROT_WRITE) access |= GENERIC_WRITE;
        if (!(mapping->file = get_file_obj( current->process, handle, access )))
        {
            release_object( mapping );
            return NULL;
        }
    }
    else mapping->file = NULL;
    return &mapping->obj;
}

int open_mapping( unsigned int access, int inherit, const char *name )
{
    return open_object( name, &mapping_ops, access, inherit );
}

int get_mapping_info( int handle, struct get_mapping_info_reply *reply )
{
    struct mapping *mapping;
    int fd;

    if (!(mapping = (struct mapping *)get_handle_obj( current->process, handle,
                                                      0, &mapping_ops )))
        return -1;
    reply->size_high = mapping->size_high;
    reply->size_low  = mapping->size_low;
    reply->protect   = mapping->protect;
    if (mapping->file) fd = file_get_mmap_fd( mapping->file );
    else fd = -1;
    release_object( mapping );
    return fd;
}

static void mapping_dump( struct object *obj, int verbose )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    fprintf( stderr, "Mapping size=%08x%08x prot=%08x file=%p name='%s'\n",
             mapping->size_high, mapping->size_low, mapping->protect,
             mapping->file, get_object_name( &mapping->obj ) );
}

static void mapping_destroy( struct object *obj )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    if (mapping->file) release_object( mapping->file );
    free( mapping );
}
