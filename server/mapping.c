/*
 * Server-side file mapping management
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "winerror.h"
#include "winnt.h"
#include "winbase.h"

#include "handle.h"
#include "thread.h"

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

#ifdef __i386__

/* These are always the same on an i386, and it will be faster this way */
# define page_mask  0xfff
# define page_shift 12
# define init_page_size() /* nothing */

#else  /* __i386__ */

static int page_shift, page_mask;

static void init_page_size(void)
{
    int page_size;
# ifdef HAVE_GETPAGESIZE
    page_size = getpagesize();
# else
#  ifdef __svr4__
    page_size = sysconf(_SC_PAGESIZE);
#  else
#   error Cannot get the page size on this platform
#  endif
# endif
    page_mask = page_size - 1;
    /* Make sure we have a power of 2 */
    assert( !(page_size & page_mask) );
    page_shift = 0;
    while ((1 << page_shift) != page_size) page_shift++;
}
#endif  /* __i386__ */

#define ROUND_ADDR(addr) \
   ((int)(addr) & ~page_mask)

#define ROUND_SIZE(addr,size) \
   (((int)(size) + ((int)(addr) & page_mask) + page_mask) & ~page_mask)


static struct object *create_mapping( int size_high, int size_low, int protect,
                                      int handle, const char *name )
{
    struct mapping *mapping;
    int access = 0;

    if (!page_mask) init_page_size();

    if (!(mapping = (struct mapping *)create_named_object( name, &mapping_ops,
                                                           sizeof(*mapping) )))
        return NULL;
    if (GET_ERROR() == ERROR_ALREADY_EXISTS)
        return &mapping->obj;  /* Nothing else to do */

    if (protect & VPROT_READ) access |= GENERIC_READ;
    if (protect & VPROT_WRITE) access |= GENERIC_WRITE;

    if (handle != -1)
    {
        if (!(mapping->file = get_file_obj( current->process, handle, access ))) goto error;
        if (!size_high && !size_low)
        {
            struct get_file_info_reply reply;
            struct object *obj = (struct object *)mapping->file;
            obj->ops->get_file_info( obj, &reply );
            size_high = reply.size_high;
            size_low  = ROUND_SIZE( 0, reply.size_low );
        }
        else if (!grow_file( mapping->file, size_high, size_low )) goto error;
    }
    else  /* Anonymous mapping (no associated file) */
    {
        if (!size_high && !size_low)
        {
            SET_ERROR( ERROR_INVALID_PARAMETER );
            mapping->file = NULL;
            goto error;
        }
        if (!(mapping->file = create_temp_file( access ))) goto error;
        if (!grow_file( mapping->file, size_high, size_low )) goto error;
    }
    mapping->size_high = size_high;
    mapping->size_low  = ROUND_SIZE( 0, size_low );
    mapping->protect   = protect;
    return &mapping->obj;

 error:
    release_object( mapping );
    return NULL;
}

static int get_mapping_info( int handle, struct get_mapping_info_reply *reply )
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

/* create a file mapping */
DECL_HANDLER(create_mapping)
{
    struct object *obj;
    struct create_mapping_reply reply = { -1 };
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "create_mapping", name, len );

    if ((obj = create_mapping( req->size_high, req->size_low,
                               req->protect, req->handle, name )))
    {
        int access = FILE_MAP_ALL_ACCESS;
        if (!(req->protect & VPROT_WRITE)) access &= ~FILE_MAP_WRITE;
        reply.handle = alloc_handle( current->process, obj, access, req->inherit );
        release_object( obj );
    }
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* open a handle to a mapping */
DECL_HANDLER(open_mapping)
{
    struct open_mapping_reply reply;
    char *name = (char *)data;
    if (!len) name = NULL;
    else CHECK_STRING( "open_mapping", name, len );

    reply.handle = open_object( name, &mapping_ops, req->access, req->inherit );
    send_reply( current, -1, 1, &reply, sizeof(reply) );
}

/* get a mapping information */
DECL_HANDLER(get_mapping_info)
{
    struct get_mapping_info_reply reply;
    int map_fd = get_mapping_info( req->handle, &reply );
    send_reply( current, map_fd, 1, &reply, sizeof(reply) );
}
