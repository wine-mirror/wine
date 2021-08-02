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
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "process.h"
#include "request.h"
#include "security.h"

/* list of memory ranges, used to store committed info */
struct ranges
{
    struct object   obj;             /* object header */
    unsigned int    count;           /* number of used ranges */
    unsigned int    max;             /* number of allocated ranges */
    struct range
    {
        file_pos_t  start;
        file_pos_t  end;
    } *ranges;
};

static void ranges_dump( struct object *obj, int verbose );
static void ranges_destroy( struct object *obj );

static const struct object_ops ranges_ops =
{
    sizeof(struct ranges),     /* size */
    &no_type,                  /* type */
    ranges_dump,               /* dump */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_get_full_name,          /* get_full_name */
    no_lookup_name,            /* lookup_name */
    no_link_name,              /* link_name */
    NULL,                      /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    ranges_destroy             /* destroy */
};

/* file backing the shared sections of a PE image mapping */
struct shared_map
{
    struct object   obj;             /* object header */
    struct fd      *fd;              /* file descriptor of the mapped PE file */
    struct file    *file;            /* temp file holding the shared data */
    struct list     entry;           /* entry in global shared maps list */
};

static void shared_map_dump( struct object *obj, int verbose );
static void shared_map_destroy( struct object *obj );

static const struct object_ops shared_map_ops =
{
    sizeof(struct shared_map), /* size */
    &no_type,                  /* type */
    shared_map_dump,           /* dump */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_get_full_name,          /* get_full_name */
    no_lookup_name,            /* lookup_name */
    no_link_name,              /* link_name */
    NULL,                      /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    shared_map_destroy         /* destroy */
};

static struct list shared_map_list = LIST_INIT( shared_map_list );

/* memory view mapped in client address space */
struct memory_view
{
    struct list     entry;           /* entry in per-process view list */
    struct fd      *fd;              /* fd for mapped file */
    struct ranges  *committed;       /* list of committed ranges in this mapping */
    struct shared_map *shared;       /* temp file for shared PE mapping */
    pe_image_info_t image;           /* image info (for PE image mapping) */
    unsigned int    flags;           /* SEC_* flags */
    client_ptr_t    base;            /* view base address (in process addr space) */
    mem_size_t      size;            /* view size */
    file_pos_t      start;           /* start offset in mapping */
    data_size_t     namelen;
    WCHAR           name[1];         /* filename for .so dll image views */
};


static const WCHAR mapping_name[] = {'S','e','c','t','i','o','n'};

struct type_descr mapping_type =
{
    { mapping_name, sizeof(mapping_name) },   /* name */
    SECTION_ALL_ACCESS | SYNCHRONIZE,         /* valid_access */
    {                                         /* mapping */
        STANDARD_RIGHTS_READ | SECTION_QUERY | SECTION_MAP_READ,
        STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
        STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
        SECTION_ALL_ACCESS
    },
};

struct mapping
{
    struct object   obj;             /* object header */
    mem_size_t      size;            /* mapping size */
    unsigned int    flags;           /* SEC_* flags */
    struct fd      *fd;              /* fd for mapped file */
    pe_image_info_t image;           /* image info (for PE image mapping) */
    struct ranges  *committed;       /* list of committed ranges in this mapping */
    struct shared_map *shared;       /* temp file for shared PE mapping */
};

static void mapping_dump( struct object *obj, int verbose );
static struct fd *mapping_get_fd( struct object *obj );
static void mapping_destroy( struct object *obj );
static enum server_fd_type mapping_get_fd_type( struct fd *fd );

static const struct object_ops mapping_ops =
{
    sizeof(struct mapping),      /* size */
    &mapping_type,               /* type */
    mapping_dump,                /* dump */
    no_add_queue,                /* add_queue */
    NULL,                        /* remove_queue */
    NULL,                        /* signaled */
    NULL,                        /* satisfied */
    no_signal,                   /* signal */
    mapping_get_fd,              /* get_fd */
    default_map_access,          /* map_access */
    default_get_sd,              /* get_sd */
    default_set_sd,              /* set_sd */
    default_get_full_name,       /* get_full_name */
    no_lookup_name,              /* lookup_name */
    directory_link_name,         /* link_name */
    default_unlink_name,         /* unlink_name */
    no_open_file,                /* open_file */
    no_kernel_obj_list,          /* get_kernel_obj_list */
    no_close_handle,             /* close_handle */
    mapping_destroy              /* destroy */
};

static const struct fd_ops mapping_fd_ops =
{
    default_fd_get_poll_events,   /* get_poll_events */
    default_poll_event,           /* poll_event */
    mapping_get_fd_type,          /* get_fd_type */
    no_fd_read,                   /* read */
    no_fd_write,                  /* write */
    no_fd_flush,                  /* flush */
    no_fd_get_file_info,          /* get_file_info */
    no_fd_get_volume_info,        /* get_volume_info */
    no_fd_ioctl,                  /* ioctl */
    no_fd_queue_async,            /* queue_async */
    default_fd_reselect_async     /* reselect_async */
};

static size_t page_mask;

#define ROUND_SIZE(size)  (((size) + page_mask) & ~page_mask)


static void ranges_dump( struct object *obj, int verbose )
{
    struct ranges *ranges = (struct ranges *)obj;
    fprintf( stderr, "Memory ranges count=%u\n", ranges->count );
}

static void ranges_destroy( struct object *obj )
{
    struct ranges *ranges = (struct ranges *)obj;
    free( ranges->ranges );
}

static void shared_map_dump( struct object *obj, int verbose )
{
    struct shared_map *shared = (struct shared_map *)obj;
    fprintf( stderr, "Shared mapping fd=%p file=%p\n", shared->fd, shared->file );
}

static void shared_map_destroy( struct object *obj )
{
    struct shared_map *shared = (struct shared_map *)obj;

    release_object( shared->fd );
    release_object( shared->file );
    list_remove( &shared->entry );
}

/* extend a file beyond the current end of file */
int grow_file( int unix_fd, file_pos_t new_size )
{
    static const char zero;
    off_t size = new_size;

    if (sizeof(new_size) > sizeof(size) && size != new_size)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    /* extend the file one byte beyond the requested size and then truncate it */
    /* this should work around ftruncate implementations that can't extend files */
    if (pwrite( unix_fd, &zero, 1, size ) != -1)
    {
        ftruncate( unix_fd, size );
        return 1;
    }
    file_set_error();
    return 0;
}

/* check if the current directory allows exec mappings */
static int check_current_dir_for_exec(void)
{
    int fd;
    char tmpfn[] = "anonmap.XXXXXX";
    void *ret = MAP_FAILED;

    fd = mkstemps( tmpfn, 0 );
    if (fd == -1) return 0;
    if (grow_file( fd, 1 ))
    {
        ret = mmap( NULL, get_page_size(), PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0 );
        if (ret != MAP_FAILED) munmap( ret, get_page_size() );
    }
    close( fd );
    unlink( tmpfn );
    return (ret != MAP_FAILED);
}

/* create a temp file for anonymous mappings */
static int create_temp_file( file_pos_t size )
{
    static int temp_dir_fd = -1;
    char tmpfn[] = "anonmap.XXXXXX";
    int fd;

    if (temp_dir_fd == -1)
    {
        temp_dir_fd = server_dir_fd;
        if (!check_current_dir_for_exec())
        {
            /* the server dir is noexec, try the config dir instead */
            fchdir( config_dir_fd );
            if (check_current_dir_for_exec())
                temp_dir_fd = config_dir_fd;
            else  /* neither works, fall back to server dir */
                fchdir( server_dir_fd );
        }
    }
    else if (temp_dir_fd != server_dir_fd) fchdir( temp_dir_fd );

    fd = mkstemps( tmpfn, 0 );
    if (fd != -1)
    {
        if (!grow_file( fd, size ))
        {
            close( fd );
            fd = -1;
        }
        unlink( tmpfn );
    }
    else file_set_error();

    if (temp_dir_fd != server_dir_fd) fchdir( server_dir_fd );
    return fd;
}

/* find a memory view from its base address */
struct memory_view *find_mapped_view( struct process *process, client_ptr_t base )
{
    struct memory_view *view;

    LIST_FOR_EACH_ENTRY( view, &process->views, struct memory_view, entry )
        if (view->base == base) return view;

    set_error( STATUS_NOT_MAPPED_VIEW );
    return NULL;
}

/* find a memory view from any address inside it */
static struct memory_view *find_mapped_addr( struct process *process, client_ptr_t addr )
{
    struct memory_view *view;

    LIST_FOR_EACH_ENTRY( view, &process->views, struct memory_view, entry )
        if (addr >= view->base && addr < view->base + view->size) return view;

    set_error( STATUS_NOT_MAPPED_VIEW );
    return NULL;
}

/* get the main exe memory view */
struct memory_view *get_exe_view( struct process *process )
{
    return LIST_ENTRY( list_head( &process->views ), struct memory_view, entry );
}

static void set_process_machine( struct process *process, struct memory_view *view )
{
    unsigned short machine = view->image.machine;

    if (machine == IMAGE_FILE_MACHINE_I386 && (view->image.image_flags & IMAGE_FLAGS_ComPlusNativeReady))
    {
        if (is_machine_supported( IMAGE_FILE_MACHINE_AMD64 )) machine = IMAGE_FILE_MACHINE_AMD64;
        else if (is_machine_supported( IMAGE_FILE_MACHINE_ARM64 )) machine = IMAGE_FILE_MACHINE_ARM64;
    }
    process->machine = machine;
}

static int generate_dll_event( struct thread *thread, int code, struct memory_view *view )
{
    unsigned short process_machine = thread->process->machine;

    if (!(view->flags & SEC_IMAGE)) return 0;
    if (process_machine != native_machine && process_machine != view->image.machine) return 0;
    generate_debug_event( thread, code, view );
    return 1;
}

/* add a view to the process list */
static void add_process_view( struct thread *thread, struct memory_view *view )
{
    struct process *process = thread->process;
    struct unicode_str name;

    if (view->flags & SEC_IMAGE)
    {
        if (is_process_init_done( process ))
        {
            generate_dll_event( thread, DbgLoadDllStateChange, view );
        }
        else if (!(view->image.image_charact & IMAGE_FILE_DLL))
        {
            /* main exe */
            set_process_machine( process, view );
            list_add_head( &process->views, &view->entry );

            free( process->image );
            process->image = NULL;
            if (get_view_nt_name( view, &name ) && (process->image = memdup( name.str, name.len )))
                process->imagelen = name.len;
            return;
        }
    }
    list_add_tail( &process->views, &view->entry );
}

static void free_memory_view( struct memory_view *view )
{
    if (view->fd) release_object( view->fd );
    if (view->committed) release_object( view->committed );
    if (view->shared) release_object( view->shared );
    list_remove( &view->entry );
    free( view );
}

/* free all mapped views at process exit */
void free_mapped_views( struct process *process )
{
    struct list *ptr;

    while ((ptr = list_head( &process->views )))
        free_memory_view( LIST_ENTRY( ptr, struct memory_view, entry ));
}

/* find the shared PE mapping for a given mapping */
static struct shared_map *get_shared_file( struct fd *fd )
{
    struct shared_map *ptr;

    LIST_FOR_EACH_ENTRY( ptr, &shared_map_list, struct shared_map, entry )
        if (is_same_file_fd( ptr->fd, fd ))
            return (struct shared_map *)grab_object( ptr );
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

/* add a range to the committed list */
static void add_committed_range( struct memory_view *view, file_pos_t start, file_pos_t end )
{
    unsigned int i, j;
    struct ranges *committed = view->committed;
    struct range *ranges;

    if ((start & page_mask) || (end & page_mask) ||
        start >= view->size || end >= view->size ||
        start >= end)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!committed) return;  /* everything committed already */

    start += view->start;
    end += view->start;

    for (i = 0, ranges = committed->ranges; i < committed->count; i++)
    {
        if (ranges[i].start > end) break;
        if (ranges[i].end < start) continue;
        if (ranges[i].start > start) ranges[i].start = start;   /* extend downwards */
        if (ranges[i].end < end)  /* extend upwards and maybe merge with next */
        {
            for (j = i + 1; j < committed->count; j++)
            {
                if (ranges[j].start > end) break;
                if (ranges[j].end > end) end = ranges[j].end;
            }
            if (j > i + 1)
            {
                memmove( &ranges[i + 1], &ranges[j], (committed->count - j) * sizeof(*ranges) );
                committed->count -= j - (i + 1);
            }
            ranges[i].end = end;
        }
        return;
    }

    /* now add a new range */

    if (committed->count == committed->max)
    {
        unsigned int new_size = committed->max * 2;
        struct range *new_ptr = realloc( committed->ranges, new_size * sizeof(*new_ptr) );
        if (!new_ptr) return;
        committed->max = new_size;
        ranges = committed->ranges = new_ptr;
    }
    memmove( &ranges[i + 1], &ranges[i], (committed->count - i) * sizeof(*ranges) );
    ranges[i].start = start;
    ranges[i].end = end;
    committed->count++;
}

/* find the range containing start and return whether it's committed */
static int find_committed_range( struct memory_view *view, file_pos_t start, mem_size_t *size )
{
    unsigned int i;
    struct ranges *committed = view->committed;
    struct range *ranges;

    if ((start & page_mask) || start >= view->size)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    if (!committed)  /* everything is committed */
    {
        *size = view->size - start;
        return 1;
    }
    for (i = 0, ranges = committed->ranges; i < committed->count; i++)
    {
        if (ranges[i].start > view->start + start)
        {
            *size = min( ranges[i].start, view->start + view->size ) - (view->start + start);
            return 0;
        }
        if (ranges[i].end > view->start + start)
        {
            *size = min( ranges[i].end, view->start + view->size ) - (view->start + start);
            return 1;
        }
    }
    *size = view->size - start;
    return 0;
}

/* allocate and fill the temp file for a shared PE image mapping */
static int build_shared_mapping( struct mapping *mapping, int fd,
                                 IMAGE_SECTION_HEADER *sec, unsigned int nb_sec )
{
    struct shared_map *shared;
    struct file *file;
    unsigned int i;
    mem_size_t total_size;
    size_t file_size, map_size, max_size;
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
    if (!total_size) return 1;  /* nothing to do */

    if ((mapping->shared = get_shared_file( mapping->fd ))) return 1;

    /* create a temp file for the mapping */

    if ((shared_fd = create_temp_file( total_size )) == -1) return 0;
    if (!(file = create_file_for_fd( shared_fd, FILE_GENERIC_READ|FILE_GENERIC_WRITE, 0 ))) return 0;

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

    if (!(shared = alloc_object( &shared_map_ops ))) goto error;
    shared->fd = (struct fd *)grab_object( mapping->fd );
    shared->file = file;
    list_add_head( &shared_map_list, &shared->entry );
    mapping->shared = shared;
    free( buffer );
    return 1;

 error:
    release_object( file );
    free( buffer );
    return 0;
}

/* load the CLR header from its section */
static int load_clr_header( IMAGE_COR20_HEADER *hdr, size_t va, size_t size, int unix_fd,
                            IMAGE_SECTION_HEADER *sec, unsigned int nb_sec )
{
    ssize_t ret;
    size_t map_size, file_size;
    off_t file_start;
    unsigned int i;

    if (!va || !size) return 0;

    for (i = 0; i < nb_sec; i++)
    {
        if (va < sec[i].VirtualAddress) continue;
        if (sec[i].Misc.VirtualSize && va - sec[i].VirtualAddress >= sec[i].Misc.VirtualSize) continue;
        get_section_sizes( &sec[i], &map_size, &file_start, &file_size );
        if (size >= map_size) continue;
        if (va - sec[i].VirtualAddress >= map_size - size) continue;
        file_size = min( file_size, map_size );
        size = min( size, sizeof(*hdr) );
        ret = pread( unix_fd, hdr, min( size, file_size ), file_start + va - sec[i].VirtualAddress );
        if (ret <= 0) break;
        if (ret < sizeof(*hdr)) memset( (char *)hdr + ret, 0, sizeof(*hdr) - ret );
        return (hdr->MajorRuntimeVersion > COR_VERSION_MAJOR_V2 ||
                (hdr->MajorRuntimeVersion == COR_VERSION_MAJOR_V2 &&
                 hdr->MinorRuntimeVersion >= COR_VERSION_MINOR));
    }
    return 0;
}

/* retrieve the mapping parameters for an executable (PE) image */
static unsigned int get_image_params( struct mapping *mapping, file_pos_t file_size, int unix_fd )
{
    static const char builtin_signature[] = "Wine builtin DLL";
    static const char fakedll_signature[] = "Wine placeholder DLL";

    IMAGE_COR20_HEADER clr;
    IMAGE_SECTION_HEADER sec[96];
    struct
    {
        IMAGE_DOS_HEADER dos;
        char buffer[32];
    } mz;
    struct
    {
        DWORD Signature;
        IMAGE_FILE_HEADER FileHeader;
        union
        {
            IMAGE_OPTIONAL_HEADER32 hdr32;
            IMAGE_OPTIONAL_HEADER64 hdr64;
        } opt;
    } nt;
    off_t pos;
    int size, opt_size;
    size_t mz_size, clr_va, clr_size;
    unsigned int i;

    /* load the headers */

    if (!file_size) return STATUS_INVALID_FILE_FOR_SECTION;
    size = pread( unix_fd, &mz, sizeof(mz), 0 );
    if (size < sizeof(mz.dos)) return STATUS_INVALID_IMAGE_NOT_MZ;
    if (mz.dos.e_magic != IMAGE_DOS_SIGNATURE) return STATUS_INVALID_IMAGE_NOT_MZ;
    mz_size = size;
    pos = mz.dos.e_lfanew;

    size = pread( unix_fd, &nt, sizeof(nt), pos );
    if (size < sizeof(nt.Signature) + sizeof(nt.FileHeader)) return STATUS_INVALID_IMAGE_PROTECT;
    /* zero out Optional header in the case it's not present or partial */
    opt_size = max( nt.FileHeader.SizeOfOptionalHeader, offsetof( IMAGE_OPTIONAL_HEADER32, CheckSum ));
    size = min( size, sizeof(nt.Signature) + sizeof(nt.FileHeader) + opt_size );
    if (size < sizeof(nt)) memset( (char *)&nt + size, 0, sizeof(nt) - size );
    if (nt.Signature != IMAGE_NT_SIGNATURE)
    {
        IMAGE_OS2_HEADER *os2 = (IMAGE_OS2_HEADER *)&nt;
        if (os2->ne_magic != IMAGE_OS2_SIGNATURE) return STATUS_INVALID_IMAGE_PROTECT;
        if (os2->ne_exetyp == 2) return STATUS_INVALID_IMAGE_WIN_16;
        if (os2->ne_exetyp == 5) return STATUS_INVALID_IMAGE_PROTECT;
        return STATUS_INVALID_IMAGE_NE_FORMAT;
    }

    switch (nt.opt.hdr32.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        if (!is_machine_32bit( nt.FileHeader.Machine )) return STATUS_INVALID_IMAGE_FORMAT;
        if (!is_machine_supported( nt.FileHeader.Machine )) return STATUS_INVALID_IMAGE_FORMAT;

        clr_va = nt.opt.hdr32.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
        clr_size = nt.opt.hdr32.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;

        mapping->image.base            = nt.opt.hdr32.ImageBase;
        mapping->image.entry_point     = nt.opt.hdr32.AddressOfEntryPoint;
        mapping->image.map_size        = ROUND_SIZE( nt.opt.hdr32.SizeOfImage );
        mapping->image.stack_size      = nt.opt.hdr32.SizeOfStackReserve;
        mapping->image.stack_commit    = nt.opt.hdr32.SizeOfStackCommit;
        mapping->image.subsystem       = nt.opt.hdr32.Subsystem;
        mapping->image.subsystem_minor = nt.opt.hdr32.MinorSubsystemVersion;
        mapping->image.subsystem_major = nt.opt.hdr32.MajorSubsystemVersion;
        mapping->image.osversion_minor = nt.opt.hdr32.MinorOperatingSystemVersion;
        mapping->image.osversion_major = nt.opt.hdr32.MajorOperatingSystemVersion;
        mapping->image.dll_charact     = nt.opt.hdr32.DllCharacteristics;
        mapping->image.contains_code   = (nt.opt.hdr32.SizeOfCode ||
                                          nt.opt.hdr32.AddressOfEntryPoint ||
                                          nt.opt.hdr32.SectionAlignment & page_mask);
        mapping->image.header_size     = nt.opt.hdr32.SizeOfHeaders;
        mapping->image.checksum        = nt.opt.hdr32.CheckSum;
        mapping->image.image_flags     = 0;
        if (nt.opt.hdr32.SectionAlignment & page_mask)
            mapping->image.image_flags |= IMAGE_FLAGS_ImageMappedFlat;
        if ((nt.opt.hdr32.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) &&
            mapping->image.contains_code && !(clr_va && clr_size))
            mapping->image.image_flags |= IMAGE_FLAGS_ImageDynamicallyRelocated;
        break;

    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        if (!is_machine_64bit( native_machine )) return STATUS_INVALID_IMAGE_WIN_64;
        if (!is_machine_64bit( nt.FileHeader.Machine )) return STATUS_INVALID_IMAGE_FORMAT;
        if (!is_machine_supported( nt.FileHeader.Machine )) return STATUS_INVALID_IMAGE_FORMAT;

        clr_va = nt.opt.hdr64.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
        clr_size = nt.opt.hdr64.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;

        mapping->image.base            = nt.opt.hdr64.ImageBase;
        mapping->image.entry_point     = nt.opt.hdr64.AddressOfEntryPoint;
        mapping->image.map_size        = ROUND_SIZE( nt.opt.hdr64.SizeOfImage );
        mapping->image.stack_size      = nt.opt.hdr64.SizeOfStackReserve;
        mapping->image.stack_commit    = nt.opt.hdr64.SizeOfStackCommit;
        mapping->image.subsystem       = nt.opt.hdr64.Subsystem;
        mapping->image.subsystem_minor = nt.opt.hdr64.MinorSubsystemVersion;
        mapping->image.subsystem_major = nt.opt.hdr64.MajorSubsystemVersion;
        mapping->image.osversion_minor = nt.opt.hdr64.MinorOperatingSystemVersion;
        mapping->image.osversion_major = nt.opt.hdr64.MajorOperatingSystemVersion;
        mapping->image.dll_charact     = nt.opt.hdr64.DllCharacteristics;
        mapping->image.contains_code   = (nt.opt.hdr64.SizeOfCode ||
                                          nt.opt.hdr64.AddressOfEntryPoint ||
                                          nt.opt.hdr64.SectionAlignment & page_mask);
        mapping->image.header_size     = nt.opt.hdr64.SizeOfHeaders;
        mapping->image.checksum        = nt.opt.hdr64.CheckSum;
        mapping->image.image_flags     = 0;
        if (nt.opt.hdr64.SectionAlignment & page_mask)
            mapping->image.image_flags |= IMAGE_FLAGS_ImageMappedFlat;
        if ((nt.opt.hdr64.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) &&
            mapping->image.contains_code && !(clr_va && clr_size))
            mapping->image.image_flags |= IMAGE_FLAGS_ImageDynamicallyRelocated;
        break;

    default:
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    mapping->image.image_charact = nt.FileHeader.Characteristics;
    mapping->image.machine       = nt.FileHeader.Machine;
    mapping->image.dbg_offset    = nt.FileHeader.PointerToSymbolTable;
    mapping->image.dbg_size      = nt.FileHeader.NumberOfSymbols;
    mapping->image.zerobits      = 0; /* FIXME */
    mapping->image.file_size     = file_size;
    mapping->image.loader_flags  = clr_va && clr_size;
    if (mz_size == sizeof(mz) && !memcmp( mz.buffer, builtin_signature, sizeof(builtin_signature) ))
        mapping->image.image_flags |= IMAGE_FLAGS_WineBuiltin;
    else if (mz_size == sizeof(mz) && !memcmp( mz.buffer, fakedll_signature, sizeof(fakedll_signature) ))
        mapping->image.image_flags |= IMAGE_FLAGS_WineFakeDll;

    /* load the section headers */

    pos += sizeof(nt.Signature) + sizeof(nt.FileHeader) + nt.FileHeader.SizeOfOptionalHeader;
    if (nt.FileHeader.NumberOfSections > ARRAY_SIZE( sec )) return STATUS_INVALID_IMAGE_FORMAT;
    size = sizeof(*sec) * nt.FileHeader.NumberOfSections;
    if (!mapping->size) mapping->size = mapping->image.map_size;
    else if (mapping->size > mapping->image.map_size) return STATUS_SECTION_TOO_BIG;
    if (pos + size > mapping->image.map_size) return STATUS_INVALID_FILE_FOR_SECTION;
    if (pos + size > mapping->image.header_size) mapping->image.header_size = pos + size;
    if (pread( unix_fd, sec, size, pos ) != size) return STATUS_INVALID_FILE_FOR_SECTION;

    for (i = 0; i < nt.FileHeader.NumberOfSections && !mapping->image.contains_code; i++)
        if (sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) mapping->image.contains_code = 1;

    if (load_clr_header( &clr, clr_va, clr_size, unix_fd, sec, nt.FileHeader.NumberOfSections ) &&
        (clr.Flags & COMIMAGE_FLAGS_ILONLY))
    {
        mapping->image.image_flags |= IMAGE_FLAGS_ComPlusILOnly;
        if (nt.opt.hdr32.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            if (!(clr.Flags & COMIMAGE_FLAGS_32BITREQUIRED))
                mapping->image.image_flags |= IMAGE_FLAGS_ComPlusNativeReady;
            if (clr.Flags & COMIMAGE_FLAGS_32BITPREFERRED)
                mapping->image.image_flags |= IMAGE_FLAGS_ComPlusPrefer32bit;
        }
    }

    if (!build_shared_mapping( mapping, unix_fd, sec, nt.FileHeader.NumberOfSections ))
        return STATUS_INVALID_FILE_FOR_SECTION;

    return STATUS_SUCCESS;
}

static struct ranges *create_ranges(void)
{
    struct ranges *ranges = alloc_object( &ranges_ops );

    if (!ranges) return NULL;
    ranges->count = 0;
    ranges->max   = 8;
    if (!(ranges->ranges = mem_alloc( ranges->max * sizeof(*ranges->ranges) )))
    {
        release_object( ranges );
        return NULL;
    }
    return ranges;
}

static unsigned int get_mapping_flags( obj_handle_t handle, unsigned int flags )
{
    switch (flags & (SEC_IMAGE | SEC_RESERVE | SEC_COMMIT | SEC_FILE))
    {
    case SEC_IMAGE:
        if (flags & (SEC_WRITECOMBINE | SEC_LARGE_PAGES)) break;
        if (handle) return SEC_FILE | SEC_IMAGE;
        set_error( STATUS_INVALID_FILE_FOR_SECTION );
        return 0;
    case SEC_COMMIT:
        if (!handle) return flags;
        /* fall through */
    case SEC_RESERVE:
        if (flags & SEC_LARGE_PAGES) break;
        if (handle) return SEC_FILE | (flags & (SEC_NOCACHE | SEC_WRITECOMBINE));
        return flags;
    }
    set_error( STATUS_INVALID_PARAMETER );
    return 0;
}


static struct mapping *create_mapping( struct object *root, const struct unicode_str *name,
                                       unsigned int attr, mem_size_t size, unsigned int flags,
                                       obj_handle_t handle, unsigned int file_access,
                                       const struct security_descriptor *sd )
{
    struct mapping *mapping;
    struct file *file;
    struct fd *fd;
    int unix_fd;
    struct stat st;

    if (!page_mask) page_mask = sysconf( _SC_PAGESIZE ) - 1;

    if (!(mapping = create_named_object( root, &mapping_ops, name, attr, sd )))
        return NULL;
    if (get_error() == STATUS_OBJECT_NAME_EXISTS)
        return mapping;  /* Nothing else to do */

    mapping->size        = size;
    mapping->fd          = NULL;
    mapping->shared      = NULL;
    mapping->committed   = NULL;

    if (!(mapping->flags = get_mapping_flags( handle, flags ))) goto error;

    if (handle)
    {
        const unsigned int sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        unsigned int mapping_access = FILE_MAPPING_ACCESS;

        if (!(file = get_file_obj( current->process, handle, file_access ))) goto error;
        fd = get_obj_fd( (struct object *)file );

        /* file sharing rules for mappings are different so we use magic the access rights */
        if (flags & SEC_IMAGE) mapping_access |= FILE_MAPPING_IMAGE;
        else if (file_access & FILE_WRITE_DATA) mapping_access |= FILE_MAPPING_WRITE;

        if (!(mapping->fd = get_fd_object_for_mapping( fd, mapping_access, sharing )))
        {
            mapping->fd = dup_fd_object( fd, mapping_access, sharing, FILE_SYNCHRONOUS_IO_NONALERT );
            if (mapping->fd) set_fd_user( mapping->fd, &mapping_fd_ops, NULL );
        }
        release_object( file );
        release_object( fd );
        if (!mapping->fd) goto error;

        if ((unix_fd = get_unix_fd( mapping->fd )) == -1) goto error;
        if (fstat( unix_fd, &st ) == -1)
        {
            file_set_error();
            goto error;
        }
        if (flags & SEC_IMAGE)
        {
            unsigned int err = get_image_params( mapping, st.st_size, unix_fd );
            if (!err) return mapping;
            set_error( err );
            goto error;
        }
        if (!mapping->size)
        {
            if (!(mapping->size = st.st_size))
            {
                set_error( STATUS_MAPPED_FILE_SIZE_ZERO );
                goto error;
            }
        }
        else if (st.st_size < mapping->size)
        {
            if (!(file_access & FILE_WRITE_DATA))
            {
                set_error( STATUS_SECTION_TOO_BIG );
                goto error;
            }
            if (!grow_file( unix_fd, mapping->size )) goto error;
        }
    }
    else  /* Anonymous mapping (no associated file) */
    {
        if (!mapping->size)
        {
            set_error( STATUS_INVALID_PARAMETER );
            goto error;
        }
        if ((flags & SEC_RESERVE) && !(mapping->committed = create_ranges())) goto error;
        mapping->size = (mapping->size + page_mask) & ~((mem_size_t)page_mask);
        if ((unix_fd = create_temp_file( mapping->size )) == -1) goto error;
        if (!(mapping->fd = create_anonymous_fd( &mapping_fd_ops, unix_fd, &mapping->obj,
                                                 FILE_SYNCHRONOUS_IO_NONALERT ))) goto error;
        allow_fd_caching( mapping->fd );
    }
    return mapping;

 error:
    release_object( mapping );
    return NULL;
}

/* create a read-only file mapping for the specified fd */
struct mapping *create_fd_mapping( struct object *root, const struct unicode_str *name,
                                   struct fd *fd, unsigned int attr, const struct security_descriptor *sd )
{
    struct mapping *mapping;
    int unix_fd;
    struct stat st;

    if (!(mapping = create_named_object( root, &mapping_ops, name, attr, sd ))) return NULL;
    if (get_error() == STATUS_OBJECT_NAME_EXISTS) return mapping;  /* Nothing else to do */

    mapping->shared    = NULL;
    mapping->committed = NULL;
    mapping->flags     = SEC_FILE;
    mapping->fd        = (struct fd *)grab_object( fd );
    set_fd_user( mapping->fd, &mapping_fd_ops, NULL );

    if ((unix_fd = get_unix_fd( mapping->fd )) == -1) goto error;
    if (fstat( unix_fd, &st ) == -1)
    {
        file_set_error();
        goto error;
    }
    if (!(mapping->size = st.st_size))
    {
        set_error( STATUS_MAPPED_FILE_SIZE_ZERO );
        goto error;
    }
    return mapping;

 error:
    release_object( mapping );
    return NULL;
}

static struct mapping *get_mapping_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct mapping *)get_handle_obj( process, handle, access, &mapping_ops );
}

/* open a new file for the file descriptor backing the view */
struct file *get_view_file( const struct memory_view *view, unsigned int access, unsigned int sharing )
{
    if (!view->fd) return NULL;
    return create_file_for_fd_obj( view->fd, access, sharing );
}

/* get the image info for a SEC_IMAGE mapped view */
const pe_image_info_t *get_view_image_info( const struct memory_view *view, client_ptr_t *base )
{
    if (!(view->flags & SEC_IMAGE)) return NULL;
    *base = view->base;
    return &view->image;
}

/* get the file name for a mapped view */
int get_view_nt_name( const struct memory_view *view, struct unicode_str *name )
{
    if (view->namelen)  /* .so builtin */
    {
        name->str = view->name;
        name->len = view->namelen;
        return 1;
    }
    if (!view->fd) return 0;
    get_nt_name( view->fd, name );
    return 1;
}

/* generate all startup events of a given process */
void generate_startup_debug_events( struct process *process )
{
    struct memory_view *view;
    struct list *ptr = list_head( &process->views );
    struct thread *thread, *first_thread = get_process_first_thread( process );

    if (!ptr) return;
    view = LIST_ENTRY( ptr, struct memory_view, entry );
    generate_debug_event( first_thread, DbgCreateProcessStateChange, view );

    /* generate ntdll.dll load event */
    while (ptr && (ptr = list_next( &process->views, ptr )))
    {
        view = LIST_ENTRY( ptr, struct memory_view, entry );
        if (generate_dll_event( first_thread, DbgLoadDllStateChange, view )) break;
    }

    /* generate creation events */
    LIST_FOR_EACH_ENTRY( thread, &process->thread_list, struct thread, proc_entry )
    {
        if (thread != first_thread)
            generate_debug_event( thread, DbgCreateThreadStateChange, NULL );
    }

    /* generate dll events (in loading order) */
    while (ptr && (ptr = list_next( &process->views, ptr )))
    {
        view = LIST_ENTRY( ptr, struct memory_view, entry );
        generate_dll_event( first_thread, DbgLoadDllStateChange, view );
    }
}

static void mapping_dump( struct object *obj, int verbose )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    fprintf( stderr, "Mapping size=%08x%08x flags=%08x fd=%p shared=%p\n",
             (unsigned int)(mapping->size >> 32), (unsigned int)mapping->size,
             mapping->flags, mapping->fd, mapping->shared );
}

static struct fd *mapping_get_fd( struct object *obj )
{
    struct mapping *mapping = (struct mapping *)obj;
    return (struct fd *)grab_object( mapping->fd );
}

static void mapping_destroy( struct object *obj )
{
    struct mapping *mapping = (struct mapping *)obj;
    assert( obj->ops == &mapping_ops );
    if (mapping->fd) release_object( mapping->fd );
    if (mapping->committed) release_object( mapping->committed );
    if (mapping->shared) release_object( mapping->shared );
}

static enum server_fd_type mapping_get_fd_type( struct fd *fd )
{
    return FD_TYPE_FILE;
}

int get_page_size(void)
{
    if (!page_mask) page_mask = sysconf( _SC_PAGESIZE ) - 1;
    return page_mask + 1;
}

struct object *create_user_data_mapping( struct object *root, const struct unicode_str *name,
                                        unsigned int attr, const struct security_descriptor *sd )
{
    void *ptr;
    struct mapping *mapping;

    if (!(mapping = create_mapping( root, name, attr, sizeof(KSHARED_USER_DATA),
                                    SEC_COMMIT, 0, FILE_READ_DATA | FILE_WRITE_DATA, sd ))) return NULL;
    ptr = mmap( NULL, mapping->size, PROT_WRITE, MAP_SHARED, get_unix_fd( mapping->fd ), 0 );
    if (ptr != MAP_FAILED)
    {
        user_shared_data = ptr;
        user_shared_data->SystemCall = 1;
    }
    return &mapping->obj;
}

/* create a file mapping */
DECL_HANDLER(create_mapping)
{
    struct object *root;
    struct mapping *mapping;
    struct unicode_str name;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((mapping = create_mapping( root, &name, objattr->attributes, req->size, req->flags,
                                   req->file_handle, req->file_access, sd )))
    {
        if (get_error() == STATUS_OBJECT_NAME_EXISTS)
            reply->handle = alloc_handle( current->process, &mapping->obj, req->access, objattr->attributes );
        else
            reply->handle = alloc_handle_no_access_check( current->process, &mapping->obj,
                                                          req->access, objattr->attributes );
        release_object( mapping );
    }

    if (root) release_object( root );
}

/* open a handle to a mapping */
DECL_HANDLER(open_mapping)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &mapping_ops, &name, req->attributes );
}

/* get a mapping information */
DECL_HANDLER(get_mapping_info)
{
    struct mapping *mapping;

    if (!(mapping = get_mapping_obj( current->process, req->handle, req->access ))) return;

    reply->size    = mapping->size;
    reply->flags   = mapping->flags;

    if (mapping->flags & SEC_IMAGE)
    {
        struct unicode_str name = { NULL, 0 };
        data_size_t size;
        void *data;

        if (mapping->fd) get_nt_name( mapping->fd, &name );
        size = min( sizeof(pe_image_info_t) + name.len, get_reply_max_size() );
        if ((data = set_reply_data_size( size )))
        {
            memcpy( data, &mapping->image, min( sizeof(pe_image_info_t), size ));
            if (size > sizeof(pe_image_info_t))
                memcpy( (pe_image_info_t *)data + 1, name.str, size - sizeof(pe_image_info_t) );
        }
        reply->total = sizeof(pe_image_info_t) + name.len;
    }

    if (!(req->access & (SECTION_MAP_READ | SECTION_MAP_WRITE)))  /* query only */
    {
        release_object( mapping );
        return;
    }

    if (mapping->shared)
        reply->shared_file = alloc_handle( current->process, mapping->shared->file,
                                           GENERIC_READ|GENERIC_WRITE, 0 );
    release_object( mapping );
}

/* add a memory view in the current process */
DECL_HANDLER(map_view)
{
    struct mapping *mapping = NULL;
    struct memory_view *view;
    data_size_t namelen = 0;

    if (!req->size || (req->base & page_mask) || req->base + req->size < req->base)  /* overflow */
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    /* make sure we don't already have an overlapping view */
    LIST_FOR_EACH_ENTRY( view, &current->process->views, struct memory_view, entry )
    {
        if (view->base + view->size <= req->base) continue;
        if (view->base >= req->base + req->size) continue;
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (!req->mapping)  /* image mapping for a .so dll */
    {
        if (get_req_data_size() > sizeof(view->image)) namelen = get_req_data_size() - sizeof(view->image);
        if (!(view = mem_alloc( offsetof( struct memory_view, name[namelen] )))) return;
        memset( view, 0, sizeof(*view) );
        view->base    = req->base;
        view->size    = req->size;
        view->start   = req->start;
        view->flags   = SEC_IMAGE;
        view->namelen = namelen;
        memcpy( &view->image, get_req_data(), min( sizeof(view->image), get_req_data_size() ));
        memcpy( view->name, (pe_image_info_t *)get_req_data() + 1, namelen );
        add_process_view( current, view );
        return;
    }

    if (!(mapping = get_mapping_obj( current->process, req->mapping, req->access ))) return;

    if (mapping->flags & SEC_IMAGE)
    {
        if (req->start || req->size > mapping->image.map_size)
        {
            set_error( STATUS_INVALID_PARAMETER );
            goto done;
        }
    }
    else if (req->start >= mapping->size ||
             req->start + req->size < req->start ||
             req->start + req->size > ((mapping->size + page_mask) & ~(mem_size_t)page_mask))
    {
        set_error( STATUS_INVALID_PARAMETER );
        goto done;
    }

    if ((view = mem_alloc( offsetof( struct memory_view, name[namelen] ))))
    {
        view->base      = req->base;
        view->size      = req->size;
        view->start     = req->start;
        view->flags     = mapping->flags;
        view->namelen   = namelen;
        view->fd        = !is_fd_removable( mapping->fd ) ? (struct fd *)grab_object( mapping->fd ) : NULL;
        view->committed = mapping->committed ? (struct ranges *)grab_object( mapping->committed ) : NULL;
        view->shared    = mapping->shared ? (struct shared_map *)grab_object( mapping->shared ) : NULL;
        if (view->flags & SEC_IMAGE) view->image = mapping->image;
        add_process_view( current, view );
        if (view->flags & SEC_IMAGE && view->base != mapping->image.base)
            set_error( STATUS_IMAGE_NOT_AT_BASE );
    }

done:
    release_object( mapping );
}

/* unmap a memory view from the current process */
DECL_HANDLER(unmap_view)
{
    struct memory_view *view = find_mapped_view( current->process, req->base );

    if (!view) return;
    generate_dll_event( current, DbgUnloadDllStateChange, view );
    free_memory_view( view );
}

/* get a range of committed pages in a file mapping */
DECL_HANDLER(get_mapping_committed_range)
{
    struct memory_view *view = find_mapped_view( current->process, req->base );

    if (view) reply->committed = find_committed_range( view, req->offset, &reply->size );
}

/* add a range to the committed pages in a file mapping */
DECL_HANDLER(add_mapping_committed_range)
{
    struct memory_view *view = find_mapped_view( current->process, req->base );

    if (view) add_committed_range( view, req->offset, req->offset + req->size );
}

/* check if two memory maps are for the same file */
DECL_HANDLER(is_same_mapping)
{
    struct memory_view *view1 = find_mapped_view( current->process, req->base1 );
    struct memory_view *view2 = find_mapped_view( current->process, req->base2 );

    if (!view1 || !view2) return;
    if (!view1->fd || !view2->fd || !(view1->flags & SEC_IMAGE) || !is_same_file_fd( view1->fd, view2->fd ))
        set_error( STATUS_NOT_SAME_DEVICE );
}

/* get the filename of a mapping */
DECL_HANDLER(get_mapping_filename)
{
    struct process *process;
    struct memory_view *view;
    struct unicode_str name;

    if (!(process = get_process_from_handle( req->process, PROCESS_QUERY_INFORMATION ))) return;

    if ((view = find_mapped_addr( process, req->addr )) && get_view_nt_name( view, &name ))
    {
        reply->len = name.len;
        if (name.len > get_reply_max_size()) set_error( STATUS_BUFFER_OVERFLOW );
        else if (!name.len) set_error( STATUS_FILE_INVALID );
        else set_reply_data( name.str, name.len );
    }
    else set_error( STATUS_INVALID_ADDRESS );

    release_object( process );
}
