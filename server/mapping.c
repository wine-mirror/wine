/*
 * Server-side file mapping management
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"

struct mapping
{
    struct object   obj;             /* object header */
    file_pos_t      size;            /* mapping size */
    int             protect;         /* protection flags */
    struct file    *file;            /* file mapped */
    int             header_size;     /* size of headers (for PE image mapping) */
    void           *base;            /* default base addr (for PE image mapping) */
    struct file    *shared_file;     /* temp file for shared PE mapping */
    int             shared_size;     /* shared mapping total size */
    struct mapping *shared_next;     /* next in shared PE mapping list */
    struct mapping *shared_prev;     /* prev in shared PE mapping list */
};

static void mapping_dump( struct object *obj, int verbose );
static struct fd *mapping_get_fd( struct object *obj );
static void mapping_destroy( struct object *obj );

static const struct object_ops mapping_ops =
{
    sizeof(struct mapping),      /* size */
    mapping_dump,                /* dump */
    no_add_queue,                /* add_queue */
    NULL,                        /* remove_queue */
    NULL,                        /* signaled */
    NULL,                        /* satisfied */
    no_signal,                   /* signal */
    mapping_get_fd,              /* get_fd */
    no_close_handle,             /* close_handle */
    mapping_destroy              /* destroy */
};

static struct mapping *shared_first;

#ifdef __i386__

/* These are always the same on an i386, and it will be faster this way */
# define page_mask  0xfff
# define page_shift 12
# define init_page_size() do { /* nothing */ } while(0)

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

#define ROUND_SIZE_MASK(addr,size,mask) \
   (((int)(size) + ((int)(addr) & (mask)) + (mask)) & ~(mask))

#define ROUND_SIZE(size)  (((size) + page_mask) & ~page_mask)


/* find the shared PE mapping for a given mapping */
static struct file *get_shared_file( struct mapping *mapping )
{
    struct mapping *ptr;

    for (ptr = shared_first; ptr; ptr = ptr->shared_next)
        if (is_same_file( ptr->file, mapping->file ))
            return (struct file *)grab_object( ptr->shared_file );
    return NULL;
}

/* allocate and fill the temp file for a shared PE image mapping */
static int build_shared_mapping( struct mapping *mapping, int fd,
                                 IMAGE_SECTION_HEADER *sec, unsigned int nb_sec )
{
    unsigned int i, max_size, total_size;
    off_t shared_pos, read_pos, write_pos;
    char *buffer = NULL;
    int shared_fd;
    long toread;

    /* compute the total size of the shared mapping */

    total_size = max_size = 0;
    for (i = 0; i < nb_sec; i++)
    {
        if ((sec[i].Characteristics & IMAGE_SCN_MEM_SHARED) &&
            (sec[i].Characteristics & IMAGE_SCN_MEM_WRITE))
        {
            unsigned int size = ROUND_SIZE( sec[i].Misc.VirtualSize );
            if (size > max_size) max_size = size;
            total_size += size;
        }
    }
    if (!(mapping->shared_size = total_size)) return 1;  /* nothing to do */

    if ((mapping->shared_file = get_shared_file( mapping ))) return 1;

    /* create a temp file for the mapping */

    if (!(mapping->shared_file = create_temp_file( GENERIC_READ|GENERIC_WRITE ))) return 0;
    if (!grow_file( mapping->shared_file, total_size )) goto error;
    if ((shared_fd = get_file_unix_fd( mapping->shared_file )) == -1) goto error;

    if (!(buffer = malloc( max_size ))) goto error;

    /* copy the shared sections data into the temp file */

    shared_pos = 0;
    for (i = 0; i < nb_sec; i++)
    {
        if (!(sec[i].Characteristics & IMAGE_SCN_MEM_SHARED)) continue;
        if (!(sec[i].Characteristics & IMAGE_SCN_MEM_WRITE)) continue;
        write_pos = shared_pos;
        shared_pos += ROUND_SIZE( sec[i].Misc.VirtualSize );
        if ((sec[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
            !(sec[i].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)) continue;
        if (!sec[i].PointerToRawData || !sec[i].SizeOfRawData) continue;
        read_pos = sec[i].PointerToRawData;
        toread = sec[i].SizeOfRawData;
        while (toread)
        {
            long res = pread( fd, buffer + sec[i].SizeOfRawData - toread, toread, read_pos );
            if (res <= 0) goto error;
            toread -= res;
            read_pos += res;
        }
        if (pwrite( shared_fd, buffer, sec[i].SizeOfRawData, write_pos ) != sec[i].SizeOfRawData)
            goto error;
    }
    free( buffer );
    return 1;

 error:
    release_object( mapping->shared_file );
    mapping->shared_file = NULL;
    if (buffer) free( buffer );
    return 0;
}

/* retrieve the mapping parameters for an executable (PE) image */
static int get_image_params( struct mapping *mapping )
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_SECTION_HEADER *sec = NULL;
    struct fd *fd;
    off_t pos;
    int unix_fd, size;

    /* load the headers */

    if (!(fd = mapping_get_fd( &mapping->obj ))) return 0;
    unix_fd = get_unix_fd( fd );
    if (pread( unix_fd, &dos, sizeof(dos), 0 ) != sizeof(dos)) goto error;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) goto error;
    pos = dos.e_lfanew;

    if (pread( unix_fd, &nt.Signature, sizeof(nt.Signature), pos ) != sizeof(nt.Signature))
        goto error;
    pos += sizeof(nt.Signature);
    if (nt.Signature != IMAGE_NT_SIGNATURE) goto error;
    if (pread( unix_fd, &nt.FileHeader, sizeof(nt.FileHeader), pos ) != sizeof(nt.FileHeader))
        goto error;
    pos += sizeof(nt.FileHeader);
    /* zero out Optional header in the case it's not present or partial */
    memset(&nt.OptionalHeader, 0, sizeof(nt.OptionalHeader));
    if (pread( unix_fd, &nt.OptionalHeader, nt.FileHeader.SizeOfOptionalHeader,
               pos ) != nt.FileHeader.SizeOfOptionalHeader) goto error;
    pos += nt.FileHeader.SizeOfOptionalHeader;

    /* load the section headers */

    size = sizeof(*sec) * nt.FileHeader.NumberOfSections;
    if (!(sec = malloc( size ))) goto error;
    if (pread( unix_fd, sec, size, pos ) != size) goto error;

    if (!build_shared_mapping( mapping, unix_fd, sec, nt.FileHeader.NumberOfSections )) goto error;

    if (mapping->shared_file)  /* link it in the list */
    {
        if ((mapping->shared_next = shared_first)) shared_first->shared_prev = mapping;
        mapping->shared_prev = NULL;
        shared_first = mapping;
    }

    mapping->size        = ROUND_SIZE( nt.OptionalHeader.SizeOfImage );
    mapping->base        = (void *)nt.OptionalHeader.ImageBase;
    mapping->header_size = ROUND_SIZE_MASK( mapping->base, nt.OptionalHeader.SizeOfHeaders,
                                            nt.OptionalHeader.SectionAlignment - 1 );
    mapping->protect     = VPROT_IMAGE;

    /* sanity check */
    if (mapping->header_size > mapping->size) goto error;

    free( sec );
    release_object( fd );
    return 1;

 error:
    if (sec) free( sec );
    release_object( fd );
    set_error( STATUS_INVALID_FILE_FOR_SECTION );
    return 0;
}

/* get the size of the unix file associated with the mapping */
inline static int get_file_size( struct file *file, file_pos_t *size )
{
    struct stat st;
    int unix_fd = get_file_unix_fd( file );

    if (fstat( unix_fd, &st ) == -1) return 0;
    *size = st.st_size;
    return 1;
}

static struct object *create_mapping( file_pos_t size, int protect, obj_handle_t handle,
                                      const WCHAR *name, size_t len )
{
    struct mapping *mapping;
    int access = 0;

    if (!page_mask) init_page_size();

    if (!(mapping = create_named_object( sync_namespace, &mapping_ops, name, len )))
        return NULL;
    if (get_error() == STATUS_OBJECT_NAME_COLLISION)
        return &mapping->obj;  /* Nothing else to do */

    mapping->header_size = 0;
    mapping->base        = NULL;
    mapping->shared_file = NULL;
    mapping->shared_size = 0;

    if (protect & VPROT_READ) access |= GENERIC_READ;
    if (protect & VPROT_WRITE) access |= GENERIC_WRITE;

    if (handle)
    {
        if (!(mapping->file = get_file_obj( current->process, handle, access ))) goto error;
        if (protect & VPROT_IMAGE)
        {
            if (!get_image_params( mapping )) goto error;
            return &mapping->obj;
        }
        if (!size)
        {
            if (!get_file_size( mapping->file, &size )) goto error;
            if (!size)
            {
                set_error( STATUS_FILE_INVALID );
                goto error;
            }
        }
        else
        {
            if (!grow_file( mapping->file, size )) goto error;
        }
    }
    else  /* Anonymous mapping (no associated file) */
    {
        if (!size || (protect & VPROT_IMAGE))
        {
            set_error( STATUS_INVALID_PARAMETER );
            mapping->file = NULL;
            goto error;
        }
        if (!(mapping->file = create_temp_file( access ))) goto error;
        if (!grow_file( mapping->file, size )) goto error;
    }
    mapping->size    = (size + page_mask) & ~((file_pos_t)page_mask);
    mapping->protect = protect;
    return &mapping->obj;

 error:
    release_object( mapping );
    return NULL;
}

static void mapping_dump( struct object *obj, int verbose )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    fprintf( stderr, "Mapping size=%08x%08x prot=%08x file=%p header_size=%08x base=%p "
             "shared_file=%p shared_size=%08x ",
             (unsigned int)(mapping->size >> 32), (unsigned int)mapping->size,
             mapping->protect, mapping->file, mapping->header_size,
             mapping->base, mapping->shared_file, mapping->shared_size );
    dump_object_name( &mapping->obj );
    fputc( '\n', stderr );
}

static struct fd *mapping_get_fd( struct object *obj )
{
    struct mapping *mapping = (struct mapping *)obj;
    return get_obj_fd( (struct object *)mapping->file );
}

static void mapping_destroy( struct object *obj )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    if (mapping->file) release_object( mapping->file );
    if (mapping->shared_file)
    {
        release_object( mapping->shared_file );
        if (mapping->shared_next) mapping->shared_next->shared_prev = mapping->shared_prev;
        if (mapping->shared_prev) mapping->shared_prev->shared_next = mapping->shared_next;
        else shared_first = mapping->shared_next;
    }
}

int get_page_size(void)
{
    if (!page_mask) init_page_size();
    return page_mask + 1;
}

/* create a file mapping */
DECL_HANDLER(create_mapping)
{
    struct object *obj;
    file_pos_t size = ((file_pos_t)req->size_high << 32) | req->size_low;

    reply->handle = 0;
    if ((obj = create_mapping( size, req->protect, req->file_handle,
                               get_req_data(), get_req_data_size() )))
    {
        reply->handle = alloc_handle( current->process, obj, req->access, req->inherit );
        release_object( obj );
    }
}

/* open a handle to a mapping */
DECL_HANDLER(open_mapping)
{
    reply->handle = open_object( sync_namespace, get_req_data(), get_req_data_size(),
                                 &mapping_ops, req->access, req->inherit );
}

/* get a mapping information */
DECL_HANDLER(get_mapping_info)
{
    struct mapping *mapping;

    if ((mapping = (struct mapping *)get_handle_obj( current->process, req->handle,
                                                     0, &mapping_ops )))
    {
        reply->size_high   = (unsigned int)(mapping->size >> 32);
        reply->size_low    = (unsigned int)mapping->size;
        reply->protect     = mapping->protect;
        reply->header_size = mapping->header_size;
        reply->base        = mapping->base;
        reply->shared_file = 0;
        reply->shared_size = mapping->shared_size;
        if (mapping->shared_file)
            reply->shared_file = alloc_handle( current->process, mapping->shared_file,
                                               GENERIC_READ|GENERIC_WRITE, 0 );
        release_object( mapping );
    }
}
