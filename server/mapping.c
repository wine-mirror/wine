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
#include "winnt.h"
#include "winbase.h"

#include "handle.h"
#include "thread.h"
#include "request.h"

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
    sizeof(struct mapping),      /* size */
    mapping_dump,                /* dump */
    no_add_queue,                /* add_queue */
    NULL,                        /* remove_queue */
    NULL,                        /* signaled */
    NULL,                        /* satisfied */
    NULL,                        /* get_poll_events */
    NULL,                        /* poll_event */
    no_read_fd,                  /* get_read_fd */
    no_write_fd,                 /* get_write_fd */
    no_flush,                    /* flush */
    no_get_file_info,            /* get_file_info */
    mapping_destroy              /* destroy */
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
                                      int handle, const WCHAR *name, size_t len )
{
    struct mapping *mapping;
    int access = 0;

    if (!page_mask) init_page_size();

    if (!(mapping = create_named_object( &mapping_ops, name, len )))
        return NULL;
    if (get_error() == STATUS_OBJECT_NAME_COLLISION)
        return &mapping->obj;  /* Nothing else to do */

    if (protect & VPROT_READ) access |= GENERIC_READ;
    if (protect & VPROT_WRITE) access |= GENERIC_WRITE;

    if (handle != -1)
    {
        if (!(mapping->file = get_file_obj( current->process, handle, access ))) goto error;
        if (!size_high && !size_low)
        {
            struct get_file_info_request req;
            struct object *obj = (struct object *)mapping->file;
            obj->ops->get_file_info( obj, &req );
            size_high = req.size_high;
            size_low  = ROUND_SIZE( 0, req.size_low );
        }
        else if (!grow_file( mapping->file, size_high, size_low )) goto error;
    }
    else  /* Anonymous mapping (no associated file) */
    {
        if (!size_high && !size_low)
        {
            set_error( STATUS_INVALID_PARAMETER );
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

static void mapping_dump( struct object *obj, int verbose )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    fprintf( stderr, "Mapping size=%08x%08x prot=%08x file=%p ",
             mapping->size_high, mapping->size_low, mapping->protect, mapping->file );
    dump_object_name( &mapping->obj );
    fputc( '\n', stderr );
}

static void mapping_destroy( struct object *obj )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    if (mapping->file) release_object( mapping->file );
}

int get_page_size(void)
{
    if (!page_mask) init_page_size();
    return page_mask + 1;
}

/* create a file mapping */
DECL_HANDLER(create_mapping)
{
    size_t len = get_req_strlenW( req->name );
    struct object *obj;

    req->handle = -1;
    if ((obj = create_mapping( req->size_high, req->size_low,
                               req->protect, req->file_handle, req->name, len )))
    {
        int access = FILE_MAP_ALL_ACCESS;
        if (!(req->protect & VPROT_WRITE)) access &= ~FILE_MAP_WRITE;
        req->handle = alloc_handle( current->process, obj, access, req->inherit );
        release_object( obj );
    }
}

/* open a handle to a mapping */
DECL_HANDLER(open_mapping)
{
    size_t len = get_req_strlenW( req->name );
    req->handle = open_object( req->name, len, &mapping_ops, req->access, req->inherit );
}

/* get a mapping information */
DECL_HANDLER(get_mapping_info)
{
    struct mapping *mapping;

    if ((mapping = (struct mapping *)get_handle_obj( current->process, req->handle,
                                                     0, &mapping_ops )))
    {
        req->size_high = mapping->size_high;
        req->size_low  = mapping->size_low;
        req->protect   = mapping->protect;
        if (mapping->file) set_reply_fd( current, file_get_mmap_fd( mapping->file ) );
        release_object( mapping );
    }
}

