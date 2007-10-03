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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

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
    struct list     shared_entry;    /* entry in global shared PE mappings list */
};

static void mapping_dump( struct object *obj, int verbose );
static struct fd *mapping_get_fd( struct object *obj );
static unsigned int mapping_map_access( struct object *obj, unsigned int access );
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
    mapping_map_access,          /* map_access */
    default_get_sd,              /* get_sd */
    default_set_sd,              /* set_sd */
    no_lookup_name,              /* lookup_name */
    no_open_file,                /* open_file */
    fd_close_handle,             /* close_handle */
    mapping_destroy              /* destroy */
};

static struct list shared_list = LIST_INIT(shared_list);

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

#define ROUND_SIZE(size)  (((size) + page_mask) & ~page_mask)


/* find the shared PE mapping for a given mapping */
static struct file *get_shared_file( struct mapping *mapping )
{
    struct mapping *ptr;

    LIST_FOR_EACH_ENTRY( ptr, &shared_list, struct mapping, shared_entry )
        if (is_same_file( ptr->file, mapping->file ))
            return (struct file *)grab_object( ptr->shared_file );
    return NULL;
}

/* return the size of the memory mapping and file range of a given section */
static inline void get_section_sizes( const IMAGE_SECTION_HEADER *sec, size_t *map_size,
                                      off_t *file_start, size_t *file_size )
{
    static const unsigned int sector_align = 0x1ff;

    if (!sec->Misc.VirtualSize) *map_size = ROUND_SIZE( sec->SizeOfRawData );
    else *map_size = ROUND_SIZE( sec->Misc.VirtualSize );

    *file_start = sec->PointerToRawData & ~sector_align;
    *file_size = (sec->SizeOfRawData + (sec->PointerToRawData & sector_align) + sector_align) & ~sector_align;
    if (*file_size > *map_size) *file_size = *map_size;
}

/* allocate and fill the temp file for a shared PE image mapping */
static int build_shared_mapping( struct mapping *mapping, int fd,
                                 IMAGE_SECTION_HEADER *sec, unsigned int nb_sec )
{
    unsigned int i;
    size_t file_size, map_size, max_size, total_size;
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
            get_section_sizes( &sec[i], &map_size, &read_pos, &file_size );
            if (file_size > max_size) max_size = file_size;
            total_size += map_size;
        }
    }
    if (!(mapping->shared_size = total_size)) return 1;  /* nothing to do */

    if ((mapping->shared_file = get_shared_file( mapping ))) return 1;

    /* create a temp file for the mapping */

    if (!(mapping->shared_file = create_temp_file( FILE_GENERIC_READ|FILE_GENERIC_WRITE ))) return 0;
    if (!grow_file( mapping->shared_file, total_size )) goto error;
    if ((shared_fd = get_file_unix_fd( mapping->shared_file )) == -1) goto error;

    if (!(buffer = malloc( max_size ))) goto error;

    /* copy the shared sections data into the temp file */

    shared_pos = 0;
    for (i = 0; i < nb_sec; i++)
    {
        if (!(sec[i].Characteristics & IMAGE_SCN_MEM_SHARED)) continue;
        if (!(sec[i].Characteristics & IMAGE_SCN_MEM_WRITE)) continue;
        get_section_sizes( &sec[i], &map_size, &read_pos, &file_size );
        write_pos = shared_pos;
        shared_pos += map_size;
        if (!sec[i].PointerToRawData || !file_size) continue;
        toread = file_size;
        while (toread)
        {
            long res = pread( fd, buffer + file_size - toread, toread, read_pos );
            if (!res && toread < 0x200)  /* partial sector at EOF is not an error */
            {
                file_size -= toread;
                break;
            }
            if (res <= 0) goto error;
            toread -= res;
            read_pos += res;
        }
        if (pwrite( shared_fd, buffer, file_size, write_pos ) != file_size) goto error;
    }
    free( buffer );
    return 1;

 error:
    release_object( mapping->shared_file );
    mapping->shared_file = NULL;
    free( buffer );
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
    int unix_fd, size, toread;

    /* load the headers */

    if (!(fd = mapping_get_fd( &mapping->obj ))) return 0;
    if ((unix_fd = get_unix_fd( fd )) == -1) goto error;
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
    toread = min( sizeof(nt.OptionalHeader), nt.FileHeader.SizeOfOptionalHeader );
    if (pread( unix_fd, &nt.OptionalHeader, toread, pos ) != toread) goto error;
    pos += nt.FileHeader.SizeOfOptionalHeader;

    /* load the section headers */

    size = sizeof(*sec) * nt.FileHeader.NumberOfSections;
    if (!(sec = malloc( size ))) goto error;
    if (pread( unix_fd, sec, size, pos ) != size) goto error;

    if (!build_shared_mapping( mapping, unix_fd, sec, nt.FileHeader.NumberOfSections )) goto error;

    if (mapping->shared_file) list_add_head( &shared_list, &mapping->shared_entry );

    mapping->size        = ROUND_SIZE( nt.OptionalHeader.SizeOfImage );
    mapping->base        = (void *)nt.OptionalHeader.ImageBase;
    mapping->header_size = max( pos + size, nt.OptionalHeader.SizeOfHeaders );
    mapping->protect     = VPROT_IMAGE;

    /* sanity check */
    if (pos + size > mapping->size) goto error;

    free( sec );
    release_object( fd );
    return 1;

 error:
    free( sec );
    release_object( fd );
    set_error( STATUS_INVALID_FILE_FOR_SECTION );
    return 0;
}

/* get the size of the unix file associated with the mapping */
static inline int get_file_size( struct file *file, file_pos_t *size )
{
    struct stat st;
    int unix_fd = get_file_unix_fd( file );

    if (unix_fd == -1 || fstat( unix_fd, &st ) == -1) return 0;
    *size = st.st_size;
    return 1;
}

static struct object *create_mapping( struct directory *root, const struct unicode_str *name,
                                      unsigned int attr, file_pos_t size, int protect,
                                      obj_handle_t handle )
{
    struct mapping *mapping;
    int access = 0;

    if (!page_mask) init_page_size();

    if (!(mapping = create_named_object_dir( root, name, attr, &mapping_ops )))
        return NULL;
    if (get_error() == STATUS_OBJECT_NAME_EXISTS)
        return &mapping->obj;  /* Nothing else to do */

    mapping->header_size = 0;
    mapping->base        = NULL;
    mapping->shared_file = NULL;
    mapping->shared_size = 0;

    if (protect & VPROT_READ) access |= FILE_READ_DATA;
    if (protect & VPROT_WRITE) access |= FILE_WRITE_DATA;

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
                set_error( STATUS_MAPPED_FILE_SIZE_ZERO );
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

static unsigned int mapping_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= STANDARD_RIGHTS_READ | SECTION_QUERY | SECTION_MAP_READ;
    if (access & GENERIC_WRITE)   access |= STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE;
    if (access & GENERIC_ALL)     access |= SECTION_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static void mapping_destroy( struct object *obj )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    if (mapping->file) release_object( mapping->file );
    if (mapping->shared_file)
    {
        release_object( mapping->shared_file );
        list_remove( &mapping->shared_entry );
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
    struct unicode_str name;
    struct directory *root = NULL;
    file_pos_t size = ((file_pos_t)req->size_high << 32) | req->size_low;

    reply->handle = 0;
    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((obj = create_mapping( root, &name, req->attributes, size, req->protect, req->file_handle )))
    {
        reply->handle = alloc_handle( current->process, obj, req->access, req->attributes );
        release_object( obj );
    }

    if (root) release_object( root );
}

/* open a handle to a mapping */
DECL_HANDLER(open_mapping)
{
    struct unicode_str name;
    struct directory *root = NULL;
    struct mapping *mapping;

    get_req_unicode_str( &name );
    if (req->rootdir && !(root = get_directory_obj( current->process, req->rootdir, 0 )))
        return;

    if ((mapping = open_object_dir( root, &name, req->attributes, &mapping_ops )))
    {
        reply->handle = alloc_handle( current->process, &mapping->obj, req->access, req->attributes );
        release_object( mapping );
    }

    if (root) release_object( root );
}

/* get a mapping information */
DECL_HANDLER(get_mapping_info)
{
    struct mapping *mapping;
    struct fd *fd;

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
        if ((fd = get_obj_fd( &mapping->obj )))
        {
            if (!is_fd_removable(fd))
                reply->mapping = alloc_handle( current->process, mapping, 0, 0 );
            release_object( fd );
        }
        if (mapping->shared_file)
        {
            if (!(reply->shared_file = alloc_handle( current->process, mapping->shared_file,
                                                     GENERIC_READ|GENERIC_WRITE, 0 )))
            {
                if (reply->mapping) close_handle( current->process, reply->mapping );
            }
        }
        release_object( mapping );
    }
}
