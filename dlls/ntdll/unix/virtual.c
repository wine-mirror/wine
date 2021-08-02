/*
 * Win32 virtual memory functions
 *
 * Copyright 1997, 2002, 2020 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_SYSINFO_H
# include <sys/sysinfo.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_VALGRIND_VALGRIND_H
# include <valgrind/valgrind.h>
#endif
#if defined(__APPLE__)
# include <mach/mach_init.h>
# include <mach/mach_vm.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "wine/rbtree.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(virtual);
WINE_DECLARE_DEBUG_CHANNEL(module);

struct preload_info
{
    void  *addr;
    size_t size;
};

struct reserved_area
{
    struct list entry;
    void       *base;
    size_t      size;
};

static struct list reserved_areas = LIST_INIT(reserved_areas);

struct builtin_module
{
    struct list  entry;
    unsigned int refcount;
    void        *handle;
    void        *module;
    char        *unix_name;
    void        *unix_handle;
    void        *unix_entry;
};

static struct list builtin_modules = LIST_INIT( builtin_modules );

struct file_view
{
    struct wine_rb_entry entry;  /* entry in global view tree */
    void         *base;          /* base address */
    size_t        size;          /* size in bytes */
    unsigned int  protect;       /* protection for all pages at allocation time and SEC_* flags */
};

#undef __TRY
#undef __EXCEPT
#undef __ENDTRY

#define __TRY \
    do { __wine_jmp_buf __jmp; \
         int __first = 1; \
         assert( !ntdll_get_thread_data()->jmp_buf ); \
         for (;;) if (!__first) \
         { \
             do {

#define __EXCEPT \
             } while(0); \
             ntdll_get_thread_data()->jmp_buf = NULL; \
             break; \
         } else { \
             if (__wine_setjmpex( &__jmp, NULL )) { \
                 do {

#define __ENDTRY \
                 } while (0); \
                 break; \
             } \
             ntdll_get_thread_data()->jmp_buf = &__jmp; \
             __first = 0; \
         } \
    } while (0);

/* per-page protection flags */
#define VPROT_READ       0x01
#define VPROT_WRITE      0x02
#define VPROT_EXEC       0x04
#define VPROT_WRITECOPY  0x08
#define VPROT_GUARD      0x10
#define VPROT_COMMITTED  0x20
#define VPROT_WRITEWATCH 0x40
/* per-mapping protection flags */
#define VPROT_SYSTEM     0x0200  /* system view (underlying mmap not under our control) */

/* Conversion from VPROT_* to Win32 flags */
static const BYTE VIRTUAL_Win32Flags[16] =
{
    PAGE_NOACCESS,              /* 0 */
    PAGE_READONLY,              /* READ */
    PAGE_READWRITE,             /* WRITE */
    PAGE_READWRITE,             /* READ | WRITE */
    PAGE_EXECUTE,               /* EXEC */
    PAGE_EXECUTE_READ,          /* READ | EXEC */
    PAGE_EXECUTE_READWRITE,     /* WRITE | EXEC */
    PAGE_EXECUTE_READWRITE,     /* READ | WRITE | EXEC */
    PAGE_WRITECOPY,             /* WRITECOPY */
    PAGE_WRITECOPY,             /* READ | WRITECOPY */
    PAGE_WRITECOPY,             /* WRITE | WRITECOPY */
    PAGE_WRITECOPY,             /* READ | WRITE | WRITECOPY */
    PAGE_EXECUTE_WRITECOPY,     /* EXEC | WRITECOPY */
    PAGE_EXECUTE_WRITECOPY,     /* READ | EXEC | WRITECOPY */
    PAGE_EXECUTE_WRITECOPY,     /* WRITE | EXEC | WRITECOPY */
    PAGE_EXECUTE_WRITECOPY      /* READ | WRITE | EXEC | WRITECOPY */
};

static struct wine_rb_tree views_tree;
static pthread_mutex_t virtual_mutex;

static const UINT page_shift = 12;
static const UINT_PTR page_mask = 0xfff;
static const UINT_PTR granularity_mask = 0xffff;

/* Note: these are Windows limits, you cannot change them. */
#ifdef __i386__
static void *address_space_start = (void *)0x110000; /* keep DOS area clear */
#else
static void *address_space_start = (void *)0x10000;
#endif

#ifdef __aarch64__
static void *address_space_limit = (void *)0xffffffff0000;  /* top of the total available address space */
#elif defined(_WIN64)
static void *address_space_limit = (void *)0x7fffffff0000;
#else
static void *address_space_limit = (void *)0xc0000000;
#endif

#ifdef _WIN64
static void *user_space_limit    = (void *)0x7fffffff0000;  /* top of the user address space */
static void *working_set_limit   = (void *)0x7fffffff0000;  /* top of the current working set */
#else
static void *user_space_limit    = (void *)0x7fff0000;
static void *working_set_limit   = (void *)0x7fff0000;
#endif

struct _KUSER_SHARED_DATA *user_shared_data = (void *)0x7ffe0000;

/* TEB allocation blocks */
static void *teb_block;
static void **next_free_teb;
static int teb_block_pos;
static struct list teb_list = LIST_INIT( teb_list );

#define ROUND_ADDR(addr,mask) ((void *)((UINT_PTR)(addr) & ~(UINT_PTR)(mask)))
#define ROUND_SIZE(addr,size) (((SIZE_T)(size) + ((UINT_PTR)(addr) & page_mask) + page_mask) & ~page_mask)

#define VIRTUAL_DEBUG_DUMP_VIEW(view) do { if (TRACE_ON(virtual)) dump_view(view); } while (0)

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

#ifdef _WIN64  /* on 64-bit the page protection bytes use a 2-level table */
static const size_t pages_vprot_shift = 20;
static const size_t pages_vprot_mask = (1 << 20) - 1;
static size_t pages_vprot_size;
static BYTE **pages_vprot;
#else  /* on 32-bit we use a simple array with one byte per page */
static BYTE *pages_vprot;
#endif

static struct file_view *view_block_start, *view_block_end, *next_free_view;
static const size_t view_block_size = 0x100000;
static void *preload_reserve_start;
static void *preload_reserve_end;
static BOOL force_exec_prot;  /* whether to force PROT_EXEC on all PROT_READ mmaps */

struct range_entry
{
    void *base;
    void *end;
};

static struct range_entry *free_ranges;
static struct range_entry *free_ranges_end;


static inline BOOL is_beyond_limit( const void *addr, size_t size, const void *limit )
{
    return (addr >= limit || (const char *)addr + size > (const char *)limit);
}

/* mmap() anonymous memory at a fixed address */
void *anon_mmap_fixed( void *start, size_t size, int prot, int flags )
{
    return mmap( start, size, prot, MAP_PRIVATE | MAP_ANON | MAP_FIXED | flags, -1, 0 );
}

/* allocate anonymous mmap() memory at any address */
void *anon_mmap_alloc( size_t size, int prot )
{
    return mmap( NULL, size, prot, MAP_PRIVATE | MAP_ANON, -1, 0 );
}


static void mmap_add_reserved_area( void *addr, SIZE_T size )
{
    struct reserved_area *area;
    struct list *ptr;

    if (!((char *)addr + size)) size--;  /* avoid wrap-around */

    LIST_FOR_EACH( ptr, &reserved_areas )
    {
        area = LIST_ENTRY( ptr, struct reserved_area, entry );
        if (area->base > addr)
        {
            /* try to merge with the next one */
            if ((char *)addr + size == (char *)area->base)
            {
                area->base = addr;
                area->size += size;
                return;
            }
            break;
        }
        else if ((char *)area->base + area->size == (char *)addr)
        {
            /* merge with the previous one */
            area->size += size;

            /* try to merge with the next one too */
            if ((ptr = list_next( &reserved_areas, ptr )))
            {
                struct reserved_area *next = LIST_ENTRY( ptr, struct reserved_area, entry );
                if ((char *)addr + size == (char *)next->base)
                {
                    area->size += next->size;
                    list_remove( &next->entry );
                    free( next );
                }
            }
            return;
        }
    }

    if ((area = malloc( sizeof(*area) )))
    {
        area->base = addr;
        area->size = size;
        list_add_before( ptr, &area->entry );
    }
}

static void mmap_remove_reserved_area( void *addr, SIZE_T size )
{
    struct reserved_area *area;
    struct list *ptr;

    if (!((char *)addr + size)) size--;  /* avoid wrap-around */

    ptr = list_head( &reserved_areas );
    /* find the first area covering address */
    while (ptr)
    {
        area = LIST_ENTRY( ptr, struct reserved_area, entry );
        if ((char *)area->base >= (char *)addr + size) break;  /* outside the range */
        if ((char *)area->base + area->size > (char *)addr)  /* overlaps range */
        {
            if (area->base >= addr)
            {
                if ((char *)area->base + area->size > (char *)addr + size)
                {
                    /* range overlaps beginning of area only -> shrink area */
                    area->size -= (char *)addr + size - (char *)area->base;
                    area->base = (char *)addr + size;
                    break;
                }
                else
                {
                    /* range contains the whole area -> remove area completely */
                    ptr = list_next( &reserved_areas, ptr );
                    list_remove( &area->entry );
                    free( area );
                    continue;
                }
            }
            else
            {
                if ((char *)area->base + area->size > (char *)addr + size)
                {
                    /* range is in the middle of area -> split area in two */
                    struct reserved_area *new_area = malloc( sizeof(*new_area) );
                    if (new_area)
                    {
                        new_area->base = (char *)addr + size;
                        new_area->size = (char *)area->base + area->size - (char *)new_area->base;
                        list_add_after( ptr, &new_area->entry );
                    }
                    else size = (char *)area->base + area->size - (char *)addr;
                    area->size = (char *)addr - (char *)area->base;
                    break;
                }
                else
                {
                    /* range overlaps end of area only -> shrink area */
                    area->size = (char *)addr - (char *)area->base;
                }
            }
        }
        ptr = list_next( &reserved_areas, ptr );
    }
}

static int mmap_is_in_reserved_area( void *addr, SIZE_T size )
{
    struct reserved_area *area;
    struct list *ptr;

    LIST_FOR_EACH( ptr, &reserved_areas )
    {
        area = LIST_ENTRY( ptr, struct reserved_area, entry );
        if (area->base > addr) break;
        if ((char *)area->base + area->size <= (char *)addr) continue;
        /* area must contain block completely */
        if ((char *)area->base + area->size < (char *)addr + size) return -1;
        return 1;
    }
    return 0;
}

static int mmap_enum_reserved_areas( int (CDECL *enum_func)(void *base, SIZE_T size, void *arg),
                                     void *arg, int top_down )
{
    int ret = 0;
    struct list *ptr;

    if (top_down)
    {
        for (ptr = reserved_areas.prev; ptr != &reserved_areas; ptr = ptr->prev)
        {
            struct reserved_area *area = LIST_ENTRY( ptr, struct reserved_area, entry );
            if ((ret = enum_func( area->base, area->size, arg ))) break;
        }
    }
    else
    {
        for (ptr = reserved_areas.next; ptr != &reserved_areas; ptr = ptr->next)
        {
            struct reserved_area *area = LIST_ENTRY( ptr, struct reserved_area, entry );
            if ((ret = enum_func( area->base, area->size, arg ))) break;
        }
    }
    return ret;
}

static void *anon_mmap_tryfixed( void *start, size_t size, int prot, int flags )
{
    void *ptr;

#ifdef MAP_FIXED_NOREPLACE
    ptr = mmap( start, size, prot, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANON | flags, -1, 0 );
#elif defined(MAP_TRYFIXED)
    ptr = mmap( start, size, prot, MAP_TRYFIXED | MAP_PRIVATE | MAP_ANON | flags, -1, 0 );
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    ptr = mmap( start, size, prot, MAP_FIXED | MAP_EXCL | MAP_PRIVATE | MAP_ANON | flags, -1, 0 );
    if (ptr == MAP_FAILED && errno == EINVAL) errno = EEXIST;
#elif defined(__APPLE__)
    mach_vm_address_t result = (mach_vm_address_t)start;
    kern_return_t ret = mach_vm_map( mach_task_self(), &result, size, 0, VM_FLAGS_FIXED,
                                     MEMORY_OBJECT_NULL, 0, 0, prot, VM_PROT_ALL, VM_INHERIT_COPY );

    if (!ret)
    {
        if ((ptr = anon_mmap_fixed( start, size, prot, flags )) == MAP_FAILED)
            mach_vm_deallocate( mach_task_self(), result, size );
    }
    else
    {
        errno = (ret == KERN_NO_SPACE ? EEXIST : ENOMEM);
        ptr = MAP_FAILED;
    }
#else
    ptr = mmap( start, size, prot, MAP_PRIVATE | MAP_ANON | flags, -1, 0 );
#endif
    if (ptr != MAP_FAILED && ptr != start)
    {
        if (is_beyond_limit( ptr, size, user_space_limit ))
        {
            anon_mmap_fixed( ptr, size, PROT_NONE, MAP_NORESERVE );
            mmap_add_reserved_area( ptr, size );
        }
        else munmap( ptr, size );
        ptr = MAP_FAILED;
        errno = EEXIST;
    }
    return ptr;
}

static void reserve_area( void *addr, void *end )
{
#ifdef __APPLE__

#ifdef __i386__
    static const mach_vm_address_t max_address = VM_MAX_ADDRESS;
#else
    static const mach_vm_address_t max_address = MACH_VM_MAX_ADDRESS;
#endif
    mach_vm_address_t address = (mach_vm_address_t)addr;
    mach_vm_address_t end_address = (mach_vm_address_t)end;

    if (!end_address || max_address < end_address)
        end_address = max_address;

    while (address < end_address)
    {
        mach_vm_address_t hole_address = address;
        kern_return_t ret;
        mach_vm_size_t size;
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t dummy_object_name = MACH_PORT_NULL;

        /* find the mapped region at or above the current address. */
        ret = mach_vm_region(mach_task_self(), &address, &size, VM_REGION_BASIC_INFO_64,
                             (vm_region_info_t)&info, &count, &dummy_object_name);
        if (ret != KERN_SUCCESS)
        {
            address = max_address;
            size = 0;
        }

        if (end_address < address)
            address = end_address;
        if (hole_address < address)
        {
            /* found a hole, attempt to reserve it. */
            size_t hole_size = address - hole_address;
            mach_vm_address_t alloc_address = hole_address;

            ret = mach_vm_map( mach_task_self(), &alloc_address, hole_size, 0, VM_FLAGS_FIXED,
                               MEMORY_OBJECT_NULL, 0, 0, PROT_NONE, VM_PROT_ALL, VM_INHERIT_COPY );
            if (!ret) mmap_add_reserved_area( (void*)hole_address, hole_size );
            else if (ret == KERN_NO_SPACE)
            {
                /* something filled (part of) the hole before we could.
                   go back and look again. */
                address = hole_address;
                continue;
            }
        }
        address += size;
    }
#else
    void *ptr;
    size_t size = (char *)end - (char *)addr;

    if (!size) return;

    if ((ptr = anon_mmap_tryfixed( addr, size, PROT_NONE, MAP_NORESERVE )) != MAP_FAILED)
    {
        mmap_add_reserved_area( addr, size );
        return;
    }
    size = (size / 2) & ~granularity_mask;
    if (size)
    {
        reserve_area( addr, (char *)addr + size );
        reserve_area( (char *)addr + size, end );
    }
#endif /* __APPLE__ */
}


static void mmap_init( const struct preload_info *preload_info )
{
#ifndef _WIN64
#ifndef __APPLE__
    char stack;
    char * const stack_ptr = &stack;
#endif
    char *user_space_limit = (char *)0x7ffe0000;
    int i;

    if (preload_info)
    {
        /* check for a reserved area starting at the user space limit */
        /* to avoid wasting time trying to allocate it again */
        for (i = 0; preload_info[i].size; i++)
        {
            if ((char *)preload_info[i].addr > user_space_limit) break;
            if ((char *)preload_info[i].addr + preload_info[i].size > user_space_limit)
            {
                user_space_limit = (char *)preload_info[i].addr + preload_info[i].size;
                break;
            }
        }
    }
    else reserve_area( (void *)0x00010000, (void *)0x40000000 );


#ifndef __APPLE__
    if (stack_ptr >= user_space_limit)
    {
        char *end = 0;
        char *base = stack_ptr - ((unsigned int)stack_ptr & granularity_mask) - (granularity_mask + 1);
        if (base > user_space_limit) reserve_area( user_space_limit, base );
        base = stack_ptr - ((unsigned int)stack_ptr & granularity_mask) + (granularity_mask + 1);
#if defined(linux) || defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__)
        /* Heuristic: assume the stack is near the end of the address */
        /* space, this avoids a lot of futile allocation attempts */
        end = (char *)(((unsigned long)base + 0x0fffffff) & 0xf0000000);
#endif
        reserve_area( base, end );
    }
    else
#endif
        reserve_area( user_space_limit, 0 );

#else

    if (preload_info) return;
    /* if we don't have a preloader, try to reserve the space now */
    reserve_area( (void *)0x000000010000, (void *)0x000068000000 );
    reserve_area( (void *)0x00007ff00000, (void *)0x00007fff0000 );
    reserve_area( (void *)0x7ffffe000000, (void *)0x7fffffff0000 );

#endif
}

/***********************************************************************
 *           add_builtin_module
 */
static void add_builtin_module( void *module, void *handle )
{
    struct builtin_module *builtin;

    if (!(builtin = malloc( sizeof(*builtin) ))) return;
    builtin->handle      = handle;
    builtin->module      = module;
    builtin->refcount    = 1;
    builtin->unix_name   = NULL;
    builtin->unix_handle = NULL;
    builtin->unix_entry  = NULL;
    list_add_tail( &builtin_modules, &builtin->entry );
}


/***********************************************************************
 *           release_builtin_module
 */
NTSTATUS release_builtin_module( void *module )
{
    struct builtin_module *builtin;

    LIST_FOR_EACH_ENTRY( builtin, &builtin_modules, struct builtin_module, entry )
    {
        if (builtin->module != module) continue;
        if (!--builtin->refcount)
        {
            list_remove( &builtin->entry );
            if (builtin->handle) dlclose( builtin->handle );
            if (builtin->unix_handle) dlclose( builtin->unix_handle );
            free( builtin->unix_name );
            free( builtin );
        }
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_PARAMETER;
}


/***********************************************************************
 *           get_builtin_so_handle
 */
void *get_builtin_so_handle( void *module )
{
    sigset_t sigset;
    void *ret = NULL;
    struct builtin_module *builtin;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    LIST_FOR_EACH_ENTRY( builtin, &builtin_modules, struct builtin_module, entry )
    {
        if (builtin->module != module) continue;
        ret = builtin->handle;
        if (ret) builtin->refcount++;
        break;
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return ret;
}


/***********************************************************************
 *           get_builtin_unix_info
 */
NTSTATUS get_builtin_unix_info( void *module, const char **name, void **handle, void **entry )
{
    sigset_t sigset;
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    struct builtin_module *builtin;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    LIST_FOR_EACH_ENTRY( builtin, &builtin_modules, struct builtin_module, entry )
    {
        if (builtin->module != module) continue;
        *name   = builtin->unix_name;
        *handle = builtin->unix_handle;
        *entry  = builtin->unix_entry;
        status = STATUS_SUCCESS;
        break;
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/***********************************************************************
 *           set_builtin_unix_info
 */
NTSTATUS set_builtin_unix_info( void *module, const char *name, void *handle, void *entry )
{
    sigset_t sigset;
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    struct builtin_module *builtin;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    LIST_FOR_EACH_ENTRY( builtin, &builtin_modules, struct builtin_module, entry )
    {
        if (builtin->module != module) continue;
        if (!builtin->unix_handle)
        {
            free( builtin->unix_name );
            builtin->unix_name = name ? strdup( name ) : NULL;
            builtin->unix_handle = handle;
            builtin->unix_entry = entry;
            status = STATUS_SUCCESS;
        }
        else status = STATUS_IMAGE_ALREADY_LOADED;
        break;
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/***********************************************************************
 *           free_ranges_lower_bound
 *
 * Returns the first range whose end is not less than addr, or end if there's none.
 */
static struct range_entry *free_ranges_lower_bound( void *addr )
{
    struct range_entry *begin = free_ranges;
    struct range_entry *end = free_ranges_end;
    struct range_entry *mid;

    while (begin < end)
    {
        mid = begin + (end - begin) / 2;
        if (mid->end < addr)
            begin = mid + 1;
        else
            end = mid;
    }

    return begin;
}


/***********************************************************************
 *           free_ranges_insert_view
 *
 * Updates the free_ranges after a new view has been created.
 */
static void free_ranges_insert_view( struct file_view *view )
{
    void *view_base = ROUND_ADDR( view->base, granularity_mask );
    void *view_end = ROUND_ADDR( (char *)view->base + view->size + granularity_mask, granularity_mask );
    struct range_entry *range = free_ranges_lower_bound( view_base );
    struct range_entry *next = range + 1;

    /* free_ranges initial value is such that the view is either inside range or before another one. */
    assert( range != free_ranges_end );
    assert( range->end > view_base || next != free_ranges_end );

    /* this happens because virtual_alloc_thread_stack shrinks a view, then creates another one on top,
     * or because AT_ROUND_TO_PAGE was used with NtMapViewOfSection to force 4kB aligned mapping. */
    if ((range->end > view_base && range->base >= view_end) ||
        (range->end == view_base && next->base >= view_end))
    {
        /* on Win64, assert that it's correctly aligned so we're not going to be in trouble later */
#ifdef _WIN64
        assert( view->base == view_base );
#endif
        WARN( "range %p - %p is already mapped\n", view_base, view_end );
        return;
    }

    /* this should never happen */
    if (range->base > view_base || range->end < view_end)
        ERR( "range %p - %p is already partially mapped\n", view_base, view_end );
    assert( range->base <= view_base && range->end >= view_end );

    /* need to split the range in two */
    if (range->base < view_base && range->end > view_end)
    {
        memmove( next + 1, next, (free_ranges_end - next) * sizeof(struct range_entry) );
        free_ranges_end += 1;
        if ((char *)free_ranges_end - (char *)free_ranges > view_block_size)
            ERR( "Free range sequence is full, trouble ahead!\n" );
        assert( (char *)free_ranges_end - (char *)free_ranges <= view_block_size );

        next->base = view_end;
        next->end = range->end;
        range->end = view_base;
    }
    else
    {
        /* otherwise we just have to shrink it */
        if (range->base < view_base)
            range->end = view_base;
        else
            range->base = view_end;

        if (range->base < range->end) return;

        /* and possibly remove it if it's now empty */
        memmove( range, next, (free_ranges_end - next) * sizeof(struct range_entry) );
        free_ranges_end -= 1;
        assert( free_ranges_end - free_ranges > 0 );
    }
}


/***********************************************************************
 *           free_ranges_remove_view
 *
 * Updates the free_ranges after a view has been destroyed.
 */
static void free_ranges_remove_view( struct file_view *view )
{
    void *view_base = ROUND_ADDR( view->base, granularity_mask );
    void *view_end = ROUND_ADDR( (char *)view->base + view->size + granularity_mask, granularity_mask );
    struct range_entry *range = free_ranges_lower_bound( view_base );
    struct range_entry *next = range + 1;

    /* It's possible to use AT_ROUND_TO_PAGE on 32bit with NtMapViewOfSection to force 4kB alignment,
     * and this breaks our assumptions. Look at the views around to check if the range is still in use. */
#ifndef _WIN64
    struct file_view *prev_view = WINE_RB_ENTRY_VALUE( wine_rb_prev( &view->entry ), struct file_view, entry );
    struct file_view *next_view = WINE_RB_ENTRY_VALUE( wine_rb_next( &view->entry ), struct file_view, entry );
    void *prev_view_base = prev_view ? ROUND_ADDR( prev_view->base, granularity_mask ) : NULL;
    void *prev_view_end = prev_view ? ROUND_ADDR( (char *)prev_view->base + prev_view->size + granularity_mask, granularity_mask ) : NULL;
    void *next_view_base = next_view ? ROUND_ADDR( next_view->base, granularity_mask ) : NULL;
    void *next_view_end = next_view ? ROUND_ADDR( (char *)next_view->base + next_view->size + granularity_mask, granularity_mask ) : NULL;

    if ((prev_view_base < view_end && prev_view_end > view_base) ||
        (next_view_base < view_end && next_view_end > view_base))
    {
        WARN( "range %p - %p is still mapped\n", view_base, view_end );
        return;
    }
#endif

    /* free_ranges initial value is such that the view is either inside range or before another one. */
    assert( range != free_ranges_end );
    assert( range->end > view_base || next != free_ranges_end );

    /* this should never happen, but we can safely ignore it */
    if (range->base <= view_base && range->end >= view_end)
    {
        WARN( "range %p - %p is already unmapped\n", view_base, view_end );
        return;
    }

    /* this should never happen */
    if (range->base < view_end && range->end > view_base)
        ERR( "range %p - %p is already partially unmapped\n", view_base, view_end );
    assert( range->end <= view_base || range->base >= view_end );

    /* merge with next if possible */
    if (range->end == view_base && next->base == view_end)
    {
        range->end = next->end;
        memmove( next, next + 1, (free_ranges_end - next - 1) * sizeof(struct range_entry) );
        free_ranges_end -= 1;
        assert( free_ranges_end - free_ranges > 0 );
    }
    /* or try growing the range */
    else if (range->end == view_base)
        range->end = view_end;
    else if (range->base == view_end)
        range->base = view_base;
    /* otherwise create a new one */
    else
    {
        memmove( range + 1, range, (free_ranges_end - range) * sizeof(struct range_entry) );
        free_ranges_end += 1;
        if ((char *)free_ranges_end - (char *)free_ranges > view_block_size)
            ERR( "Free range sequence is full, trouble ahead!\n" );
        assert( (char *)free_ranges_end - (char *)free_ranges <= view_block_size );

        range->base = view_base;
        range->end = view_end;
    }
}


static inline int is_view_valloc( const struct file_view *view )
{
    return !(view->protect & (SEC_FILE | SEC_RESERVE | SEC_COMMIT));
}

/***********************************************************************
 *           get_page_vprot
 *
 * Return the page protection byte.
 */
static BYTE get_page_vprot( const void *addr )
{
    size_t idx = (size_t)addr >> page_shift;

#ifdef _WIN64
    if ((idx >> pages_vprot_shift) >= pages_vprot_size) return 0;
    if (!pages_vprot[idx >> pages_vprot_shift]) return 0;
    return pages_vprot[idx >> pages_vprot_shift][idx & pages_vprot_mask];
#else
    return pages_vprot[idx];
#endif
}


/***********************************************************************
 *           set_page_vprot
 *
 * Set a range of page protection bytes.
 */
static void set_page_vprot( const void *addr, size_t size, BYTE vprot )
{
    size_t idx = (size_t)addr >> page_shift;
    size_t end = ((size_t)addr + size + page_mask) >> page_shift;

#ifdef _WIN64
    while (idx >> pages_vprot_shift != end >> pages_vprot_shift)
    {
        size_t dir_size = pages_vprot_mask + 1 - (idx & pages_vprot_mask);
        memset( pages_vprot[idx >> pages_vprot_shift] + (idx & pages_vprot_mask), vprot, dir_size );
        idx += dir_size;
    }
    memset( pages_vprot[idx >> pages_vprot_shift] + (idx & pages_vprot_mask), vprot, end - idx );
#else
    memset( pages_vprot + idx, vprot, end - idx );
#endif
}


/***********************************************************************
 *           set_page_vprot_bits
 *
 * Set or clear bits in a range of page protection bytes.
 */
static void set_page_vprot_bits( const void *addr, size_t size, BYTE set, BYTE clear )
{
    size_t idx = (size_t)addr >> page_shift;
    size_t end = ((size_t)addr + size + page_mask) >> page_shift;

#ifdef _WIN64
    for ( ; idx < end; idx++)
    {
        BYTE *ptr = pages_vprot[idx >> pages_vprot_shift] + (idx & pages_vprot_mask);
        *ptr = (*ptr & ~clear) | set;
    }
#else
    for ( ; idx < end; idx++) pages_vprot[idx] = (pages_vprot[idx] & ~clear) | set;
#endif
}


/***********************************************************************
 *           alloc_pages_vprot
 *
 * Allocate the page protection bytes for a given range.
 */
static BOOL alloc_pages_vprot( const void *addr, size_t size )
{
#ifdef _WIN64
    size_t idx = (size_t)addr >> page_shift;
    size_t end = ((size_t)addr + size + page_mask) >> page_shift;
    size_t i;
    void *ptr;

    assert( end <= pages_vprot_size << pages_vprot_shift );
    for (i = idx >> pages_vprot_shift; i < (end + pages_vprot_mask) >> pages_vprot_shift; i++)
    {
        if (pages_vprot[i]) continue;
        if ((ptr = anon_mmap_alloc( pages_vprot_mask + 1, PROT_READ | PROT_WRITE )) == MAP_FAILED)
            return FALSE;
        pages_vprot[i] = ptr;
    }
#endif
    return TRUE;
}


/***********************************************************************
 *           compare_view
 *
 * View comparison function used for the rb tree.
 */
static int compare_view( const void *addr, const struct wine_rb_entry *entry )
{
    struct file_view *view = WINE_RB_ENTRY_VALUE( entry, struct file_view, entry );

    if (addr < view->base) return -1;
    if (addr > view->base) return 1;
    return 0;
}


/***********************************************************************
 *           get_prot_str
 */
static const char *get_prot_str( BYTE prot )
{
    static char buffer[6];
    buffer[0] = (prot & VPROT_COMMITTED) ? 'c' : '-';
    buffer[1] = (prot & VPROT_GUARD) ? 'g' : ((prot & VPROT_WRITEWATCH) ? 'H' : '-');
    buffer[2] = (prot & VPROT_READ) ? 'r' : '-';
    buffer[3] = (prot & VPROT_WRITECOPY) ? 'W' : ((prot & VPROT_WRITE) ? 'w' : '-');
    buffer[4] = (prot & VPROT_EXEC) ? 'x' : '-';
    buffer[5] = 0;
    return buffer;
}


/***********************************************************************
 *           get_unix_prot
 *
 * Convert page protections to protection for mmap/mprotect.
 */
static int get_unix_prot( BYTE vprot )
{
    int prot = 0;
    if ((vprot & VPROT_COMMITTED) && !(vprot & VPROT_GUARD))
    {
        if (vprot & VPROT_READ) prot |= PROT_READ;
        if (vprot & VPROT_WRITE) prot |= PROT_WRITE | PROT_READ;
        if (vprot & VPROT_WRITECOPY) prot |= PROT_WRITE | PROT_READ;
        if (vprot & VPROT_EXEC) prot |= PROT_EXEC | PROT_READ;
        if (vprot & VPROT_WRITEWATCH) prot &= ~PROT_WRITE;
    }
    if (!prot) prot = PROT_NONE;
    return prot;
}


/***********************************************************************
 *           dump_view
 */
static void dump_view( struct file_view *view )
{
    UINT i, count;
    char *addr = view->base;
    BYTE prot = get_page_vprot( addr );

    TRACE( "View: %p - %p", addr, addr + view->size - 1 );
    if (view->protect & VPROT_SYSTEM)
        TRACE( " (builtin image)\n" );
    else if (view->protect & SEC_IMAGE)
        TRACE( " (image)\n" );
    else if (view->protect & SEC_FILE)
        TRACE( " (file)\n" );
    else if (view->protect & (SEC_RESERVE | SEC_COMMIT))
        TRACE( " (anonymous)\n" );
    else
        TRACE( " (valloc)\n");

    for (count = i = 1; i < view->size >> page_shift; i++, count++)
    {
        BYTE next = get_page_vprot( addr + (count << page_shift) );
        if (next == prot) continue;
        TRACE( "      %p - %p %s\n",
                 addr, addr + (count << page_shift) - 1, get_prot_str(prot) );
        addr += (count << page_shift);
        prot = next;
        count = 0;
    }
    if (count)
        TRACE( "      %p - %p %s\n",
                 addr, addr + (count << page_shift) - 1, get_prot_str(prot) );
}


/***********************************************************************
 *           VIRTUAL_Dump
 */
#ifdef WINE_VM_DEBUG
static void VIRTUAL_Dump(void)
{
    sigset_t sigset;
    struct file_view *view;

    TRACE( "Dump of all virtual memory views:\n" );
    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    WINE_RB_FOR_EACH_ENTRY( view, &views_tree, struct file_view, entry )
    {
        dump_view( view );
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
}
#endif


/***********************************************************************
 *           find_view
 *
 * Find the view containing a given address. virtual_mutex must be held by caller.
 *
 * PARAMS
 *      addr  [I] Address
 *
 * RETURNS
 *	View: Success
 *	NULL: Failure
 */
static struct file_view *find_view( const void *addr, size_t size )
{
    struct wine_rb_entry *ptr = views_tree.root;

    if ((const char *)addr + size < (const char *)addr) return NULL; /* overflow */

    while (ptr)
    {
        struct file_view *view = WINE_RB_ENTRY_VALUE( ptr, struct file_view, entry );

        if (view->base > addr) ptr = ptr->left;
        else if ((const char *)view->base + view->size <= (const char *)addr) ptr = ptr->right;
        else if ((const char *)view->base + view->size < (const char *)addr + size) break;  /* size too large */
        else return view;
    }
    return NULL;
}


/***********************************************************************
 *           get_zero_bits_mask
 */
static inline UINT_PTR get_zero_bits_mask( ULONG_PTR zero_bits )
{
    unsigned int shift;

    if (zero_bits == 0) return ~(UINT_PTR)0;

    if (zero_bits < 32) shift = 32 + zero_bits;
    else
    {
        shift = 63;
#ifdef _WIN64
        if (zero_bits >> 32) { shift -= 32; zero_bits >>= 32; }
#endif
        if (zero_bits >> 16) { shift -= 16; zero_bits >>= 16; }
        if (zero_bits >> 8) { shift -= 8; zero_bits >>= 8; }
        if (zero_bits >> 4) { shift -= 4; zero_bits >>= 4; }
        if (zero_bits >> 2) { shift -= 2; zero_bits >>= 2; }
        if (zero_bits >> 1) { shift -= 1; }
    }
    return (UINT_PTR)((~(UINT64)0) >> shift);
}


/***********************************************************************
 *           is_write_watch_range
 */
static inline BOOL is_write_watch_range( const void *addr, size_t size )
{
    struct file_view *view = find_view( addr, size );
    return view && (view->protect & VPROT_WRITEWATCH);
}


/***********************************************************************
 *           find_view_range
 *
 * Find the first view overlapping at least part of the specified range.
 * virtual_mutex must be held by caller.
 */
static struct file_view *find_view_range( const void *addr, size_t size )
{
    struct wine_rb_entry *ptr = views_tree.root;

    while (ptr)
    {
        struct file_view *view = WINE_RB_ENTRY_VALUE( ptr, struct file_view, entry );

        if ((const char *)view->base >= (const char *)addr + size) ptr = ptr->left;
        else if ((const char *)view->base + view->size <= (const char *)addr) ptr = ptr->right;
        else return view;
    }
    return NULL;
}


/***********************************************************************
 *           find_view_inside_range
 *
 * Find first (resp. last, if top_down) view inside a range.
 * virtual_mutex must be held by caller.
 */
static struct wine_rb_entry *find_view_inside_range( void **base_ptr, void **end_ptr, int top_down )
{
    struct wine_rb_entry *first = NULL, *ptr = views_tree.root;
    void *base = *base_ptr, *end = *end_ptr;

    /* find the first (resp. last) view inside the range */
    while (ptr)
    {
        struct file_view *view = WINE_RB_ENTRY_VALUE( ptr, struct file_view, entry );
        if ((char *)view->base + view->size >= (char *)end)
        {
            end = min( end, view->base );
            ptr = ptr->left;
        }
        else if (view->base <= base)
        {
            base = max( (char *)base, (char *)view->base + view->size );
            ptr = ptr->right;
        }
        else
        {
            first = ptr;
            ptr = top_down ? ptr->right : ptr->left;
        }
    }

    *base_ptr = base;
    *end_ptr = end;
    return first;
}


/***********************************************************************
 *           try_map_free_area
 *
 * Try mmaping some expected free memory region, eventually stepping and
 * retrying inside it, and return where it actually succeeded, or NULL.
 */
static void* try_map_free_area( void *base, void *end, ptrdiff_t step,
                                void *start, size_t size, int unix_prot )
{
    void *ptr;

    while (start && base <= start && (char*)start + size <= (char*)end)
    {
        if ((ptr = anon_mmap_tryfixed( start, size, unix_prot, 0 )) != MAP_FAILED) return start;
        TRACE( "Found free area is already mapped, start %p.\n", start );
        if (errno != EEXIST)
        {
            ERR( "mmap() error %s, range %p-%p, unix_prot %#x.\n",
                 strerror(errno), start, (char *)start + size, unix_prot );
            return NULL;
        }
        if ((step > 0 && (char *)end - (char *)start < step) ||
            (step < 0 && (char *)start - (char *)base < -step) ||
            step == 0)
            break;
        start = (char *)start + step;
    }

    return NULL;
}


/***********************************************************************
 *           map_free_area
 *
 * Find a free area between views inside the specified range and map it.
 * virtual_mutex must be held by caller.
 */
static void *map_free_area( void *base, void *end, size_t size, int top_down, int unix_prot )
{
    struct wine_rb_entry *first = find_view_inside_range( &base, &end, top_down );
    ptrdiff_t step = top_down ? -(granularity_mask + 1) : (granularity_mask + 1);
    void *start;

    if (top_down)
    {
        start = ROUND_ADDR( (char *)end - size, granularity_mask );
        if (start >= end || start < base) return NULL;

        while (first)
        {
            struct file_view *view = WINE_RB_ENTRY_VALUE( first, struct file_view, entry );
            if ((start = try_map_free_area( (char *)view->base + view->size, (char *)start + size, step,
                                            start, size, unix_prot ))) break;
            start = ROUND_ADDR( (char *)view->base - size, granularity_mask );
            /* stop if remaining space is not large enough */
            if (!start || start >= end || start < base) return NULL;
            first = wine_rb_prev( first );
        }
    }
    else
    {
        start = ROUND_ADDR( (char *)base + granularity_mask, granularity_mask );
        if (!start || start >= end || (char *)end - (char *)start < size) return NULL;

        while (first)
        {
            struct file_view *view = WINE_RB_ENTRY_VALUE( first, struct file_view, entry );
            if ((start = try_map_free_area( start, view->base, step,
                                            start, size, unix_prot ))) break;
            start = ROUND_ADDR( (char *)view->base + view->size + granularity_mask, granularity_mask );
            /* stop if remaining space is not large enough */
            if (!start || start >= end || (char *)end - (char *)start < size) return NULL;
            first = wine_rb_next( first );
        }
    }

    if (!first)
        return try_map_free_area( base, end, step, start, size, unix_prot );

    return start;
}


/***********************************************************************
 *           find_reserved_free_area
 *
 * Find a free area between views inside the specified range.
 * virtual_mutex must be held by caller.
 * The range must be inside the preloader reserved range.
 */
static void *find_reserved_free_area( void *base, void *end, size_t size, int top_down )
{
    struct range_entry *range;
    void *start;

    base = ROUND_ADDR( (char *)base + granularity_mask, granularity_mask );
    end = (char *)ROUND_ADDR( (char *)end - size, granularity_mask ) + size;

    if (top_down)
    {
        start = (char *)end - size;
        range = free_ranges_lower_bound( start );
        assert(range != free_ranges_end && range->end >= start);

        if ((char *)range->end - (char *)start < size) start = ROUND_ADDR( (char *)range->end - size, granularity_mask );
        do
        {
            if (start >= end || start < base || (char *)end - (char *)start < size) return NULL;
            if (start < range->end && start >= range->base && (char *)range->end - (char *)start >= size) break;
            if (--range < free_ranges) return NULL;
            start = ROUND_ADDR( (char *)range->end - size, granularity_mask );
        }
        while (1);
    }
    else
    {
        start = base;
        range = free_ranges_lower_bound( start );
        assert(range != free_ranges_end && range->end >= start);

        if (start < range->base) start = ROUND_ADDR( (char *)range->base + granularity_mask, granularity_mask );
        do
        {
            if (start >= end || start < base || (char *)end - (char *)start < size) return NULL;
            if (start < range->end && start >= range->base && (char *)range->end - (char *)start >= size) break;
            if (++range == free_ranges_end) return NULL;
            start = ROUND_ADDR( (char *)range->base + granularity_mask, granularity_mask );
        }
        while (1);
    }
    return start;
}


/***********************************************************************
 *           add_reserved_area
 *
 * Add a reserved area to the list maintained by libwine.
 * virtual_mutex must be held by caller.
 */
static void add_reserved_area( void *addr, size_t size )
{
    TRACE( "adding %p-%p\n", addr, (char *)addr + size );

    if (addr < user_space_limit)
    {
        /* unmap the part of the area that is below the limit */
        assert( (char *)addr + size > (char *)user_space_limit );
        munmap( addr, (char *)user_space_limit - (char *)addr );
        size -= (char *)user_space_limit - (char *)addr;
        addr = user_space_limit;
    }
    /* blow away existing mappings */
    anon_mmap_fixed( addr, size, PROT_NONE, MAP_NORESERVE );
    mmap_add_reserved_area( addr, size );
}


/***********************************************************************
 *           remove_reserved_area
 *
 * Remove a reserved area from the list maintained by libwine.
 * virtual_mutex must be held by caller.
 */
static void remove_reserved_area( void *addr, size_t size )
{
    struct file_view *view;

    TRACE( "removing %p-%p\n", addr, (char *)addr + size );
    mmap_remove_reserved_area( addr, size );

    /* unmap areas not covered by an existing view */
    WINE_RB_FOR_EACH_ENTRY( view, &views_tree, struct file_view, entry )
    {
        if ((char *)view->base >= (char *)addr + size) break;
        if ((char *)view->base + view->size <= (char *)addr) continue;
        if (view->base > addr) munmap( addr, (char *)view->base - (char *)addr );
        if ((char *)view->base + view->size > (char *)addr + size) return;
        size = (char *)addr + size - ((char *)view->base + view->size);
        addr = (char *)view->base + view->size;
    }
    munmap( addr, size );
}


struct area_boundary
{
    void  *base;
    size_t size;
    void  *boundary;
};

/***********************************************************************
 *           get_area_boundary_callback
 *
 * Get lowest boundary address between reserved area and non-reserved area
 * in the specified region. If no boundaries are found, result is NULL.
 * virtual_mutex must be held by caller.
 */
static int CDECL get_area_boundary_callback( void *start, SIZE_T size, void *arg )
{
    struct area_boundary *area = arg;
    void *end = (char *)start + size;

    area->boundary = NULL;
    if (area->base >= end) return 0;
    if ((char *)start >= (char *)area->base + area->size) return 1;
    if (area->base >= start)
    {
        if ((char *)area->base + area->size > (char *)end)
        {
            area->boundary = end;
            return 1;
        }
        return 0;
    }
    area->boundary = start;
    return 1;
}


/***********************************************************************
 *           unmap_area
 *
 * Unmap an area, or simply replace it by an empty mapping if it is
 * in a reserved area. virtual_mutex must be held by caller.
 */
static inline void unmap_area( void *addr, size_t size )
{
    switch (mmap_is_in_reserved_area( addr, size ))
    {
    case -1: /* partially in a reserved area */
    {
        struct area_boundary area;
        size_t lower_size;
        area.base = addr;
        area.size = size;
        mmap_enum_reserved_areas( get_area_boundary_callback, &area, 0 );
        assert( area.boundary );
        lower_size = (char *)area.boundary - (char *)addr;
        unmap_area( addr, lower_size );
        unmap_area( area.boundary, size - lower_size );
        break;
    }
    case 1:  /* in a reserved area */
        anon_mmap_fixed( addr, size, PROT_NONE, MAP_NORESERVE );
        break;
    default:
    case 0:  /* not in a reserved area */
        if (is_beyond_limit( addr, size, user_space_limit ))
            add_reserved_area( addr, size );
        else
            munmap( addr, size );
        break;
    }
}


/***********************************************************************
 *           alloc_view
 *
 * Allocate a new view. virtual_mutex must be held by caller.
 */
static struct file_view *alloc_view(void)
{
    if (next_free_view)
    {
        struct file_view *ret = next_free_view;
        next_free_view = *(struct file_view **)ret;
        return ret;
    }
    if (view_block_start == view_block_end)
    {
        void *ptr = anon_mmap_alloc( view_block_size, PROT_READ | PROT_WRITE );
        if (ptr == MAP_FAILED) return NULL;
        view_block_start = ptr;
        view_block_end = view_block_start + view_block_size / sizeof(*view_block_start);
    }
    return view_block_start++;
}


/***********************************************************************
 *           delete_view
 *
 * Deletes a view. virtual_mutex must be held by caller.
 */
static void delete_view( struct file_view *view ) /* [in] View */
{
    if (!(view->protect & VPROT_SYSTEM)) unmap_area( view->base, view->size );
    set_page_vprot( view->base, view->size, 0 );
    if (mmap_is_in_reserved_area( view->base, view->size ))
        free_ranges_remove_view( view );
    wine_rb_remove( &views_tree, &view->entry );
    *(struct file_view **)view = next_free_view;
    next_free_view = view;
}


/***********************************************************************
 *           create_view
 *
 * Create a view. virtual_mutex must be held by caller.
 */
static NTSTATUS create_view( struct file_view **view_ret, void *base, size_t size, unsigned int vprot )
{
    struct file_view *view;
    int unix_prot = get_unix_prot( vprot );

    assert( !((UINT_PTR)base & page_mask) );
    assert( !(size & page_mask) );

    /* Check for overlapping views. This can happen if the previous view
     * was a system view that got unmapped behind our back. In that case
     * we recover by simply deleting it. */

    while ((view = find_view_range( base, size )))
    {
        TRACE( "overlapping view %p-%p for %p-%p\n",
               view->base, (char *)view->base + view->size, base, (char *)base + size );
        assert( view->protect & VPROT_SYSTEM );
        delete_view( view );
    }

    if (!alloc_pages_vprot( base, size )) return STATUS_NO_MEMORY;

    /* Create the view structure */

    if (!(view = alloc_view()))
    {
        FIXME( "out of memory for %p-%p\n", base, (char *)base + size );
        return STATUS_NO_MEMORY;
    }

    view->base    = base;
    view->size    = size;
    view->protect = vprot;
    set_page_vprot( base, size, vprot );

    wine_rb_put( &views_tree, view->base, &view->entry );
    if (mmap_is_in_reserved_area( view->base, view->size ))
        free_ranges_insert_view( view );

    *view_ret = view;

    if (force_exec_prot && (unix_prot & PROT_READ) && !(unix_prot & PROT_EXEC))
    {
        TRACE( "forcing exec permission on %p-%p\n", base, (char *)base + size - 1 );
        mprotect( base, size, unix_prot | PROT_EXEC );
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           get_win32_prot
 *
 * Convert page protections to Win32 flags.
 */
static DWORD get_win32_prot( BYTE vprot, unsigned int map_prot )
{
    DWORD ret = VIRTUAL_Win32Flags[vprot & 0x0f];
    if (vprot & VPROT_GUARD) ret |= PAGE_GUARD;
    if (map_prot & SEC_NOCACHE) ret |= PAGE_NOCACHE;
    return ret;
}


/***********************************************************************
 *           get_vprot_flags
 *
 * Build page protections from Win32 flags.
 */
static NTSTATUS get_vprot_flags( DWORD protect, unsigned int *vprot, BOOL image )
{
    switch(protect & 0xff)
    {
    case PAGE_READONLY:
        *vprot = VPROT_READ;
        break;
    case PAGE_READWRITE:
        if (image)
            *vprot = VPROT_READ | VPROT_WRITECOPY;
        else
            *vprot = VPROT_READ | VPROT_WRITE;
        break;
    case PAGE_WRITECOPY:
        *vprot = VPROT_READ | VPROT_WRITECOPY;
        break;
    case PAGE_EXECUTE:
        *vprot = VPROT_EXEC;
        break;
    case PAGE_EXECUTE_READ:
        *vprot = VPROT_EXEC | VPROT_READ;
        break;
    case PAGE_EXECUTE_READWRITE:
        if (image)
            *vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITECOPY;
        else
            *vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITE;
        break;
    case PAGE_EXECUTE_WRITECOPY:
        *vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITECOPY;
        break;
    case PAGE_NOACCESS:
        *vprot = 0;
        break;
    default:
        return STATUS_INVALID_PAGE_PROTECTION;
    }
    if (protect & PAGE_GUARD) *vprot |= VPROT_GUARD;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           mprotect_exec
 *
 * Wrapper for mprotect, adds PROT_EXEC if forced by force_exec_prot
 */
static inline int mprotect_exec( void *base, size_t size, int unix_prot )
{
    if (force_exec_prot && (unix_prot & PROT_READ) && !(unix_prot & PROT_EXEC))
    {
        TRACE( "forcing exec permission on %p-%p\n", base, (char *)base + size - 1 );
        if (!mprotect( base, size, unix_prot | PROT_EXEC )) return 0;
        /* exec + write may legitimately fail, in that case fall back to write only */
        if (!(unix_prot & PROT_WRITE)) return -1;
    }

    return mprotect( base, size, unix_prot );
}


/***********************************************************************
 *           mprotect_range
 *
 * Call mprotect on a page range, applying the protections from the per-page byte.
 */
static void mprotect_range( void *base, size_t size, BYTE set, BYTE clear )
{
    size_t i, count;
    char *addr = ROUND_ADDR( base, page_mask );
    int prot, next;

    size = ROUND_SIZE( base, size );
    prot = get_unix_prot( (get_page_vprot( addr ) & ~clear ) | set );
    for (count = i = 1; i < size >> page_shift; i++, count++)
    {
        next = get_unix_prot( (get_page_vprot( addr + (count << page_shift) ) & ~clear) | set );
        if (next == prot) continue;
        mprotect_exec( addr, count << page_shift, prot );
        addr += count << page_shift;
        prot = next;
        count = 0;
    }
    if (count) mprotect_exec( addr, count << page_shift, prot );
}


/***********************************************************************
 *           set_vprot
 *
 * Change the protection of a range of pages.
 */
static BOOL set_vprot( struct file_view *view, void *base, size_t size, BYTE vprot )
{
    int unix_prot = get_unix_prot(vprot);

    if (view->protect & VPROT_WRITEWATCH)
    {
        /* each page may need different protections depending on write watch flag */
        set_page_vprot_bits( base, size, vprot & ~VPROT_WRITEWATCH, ~vprot & ~VPROT_WRITEWATCH );
        mprotect_range( base, size, 0, 0 );
        return TRUE;
    }
    if (mprotect_exec( base, size, unix_prot )) return FALSE;
    set_page_vprot( base, size, vprot );
    return TRUE;
}


/***********************************************************************
 *           set_protection
 *
 * Set page protections on a range of pages
 */
static NTSTATUS set_protection( struct file_view *view, void *base, SIZE_T size, ULONG protect )
{
    unsigned int vprot;
    NTSTATUS status;

    if ((status = get_vprot_flags( protect, &vprot, view->protect & SEC_IMAGE ))) return status;
    if (is_view_valloc( view ))
    {
        if (vprot & VPROT_WRITECOPY) return STATUS_INVALID_PAGE_PROTECTION;
    }
    else
    {
        BYTE access = vprot & (VPROT_READ | VPROT_WRITE | VPROT_EXEC);
        if ((view->protect & access) != access) return STATUS_INVALID_PAGE_PROTECTION;
    }

    if (!set_vprot( view, base, size, vprot | VPROT_COMMITTED )) return STATUS_ACCESS_DENIED;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           update_write_watches
 */
static void update_write_watches( void *base, size_t size, size_t accessed_size )
{
    TRACE( "updating watch %p-%p-%p\n", base, (char *)base + accessed_size, (char *)base + size );
    /* clear write watch flag on accessed pages */
    set_page_vprot_bits( base, accessed_size, 0, VPROT_WRITEWATCH );
    /* restore page protections on the entire range */
    mprotect_range( base, size, 0, 0 );
}


/***********************************************************************
 *           reset_write_watches
 *
 * Reset write watches in a memory range.
 */
static void reset_write_watches( void *base, SIZE_T size )
{
    set_page_vprot_bits( base, size, VPROT_WRITEWATCH, 0 );
    mprotect_range( base, size, 0, 0 );
}


/***********************************************************************
 *           unmap_extra_space
 *
 * Release the extra memory while keeping the range starting on the granularity boundary.
 */
static inline void *unmap_extra_space( void *ptr, size_t total_size, size_t wanted_size )
{
    if ((ULONG_PTR)ptr & granularity_mask)
    {
        size_t extra = granularity_mask + 1 - ((ULONG_PTR)ptr & granularity_mask);
        munmap( ptr, extra );
        ptr = (char *)ptr + extra;
        total_size -= extra;
    }
    if (total_size > wanted_size)
        munmap( (char *)ptr + wanted_size, total_size - wanted_size );
    return ptr;
}


struct alloc_area
{
    size_t size;
    int    top_down;
    void  *limit;
    void  *result;
};

/***********************************************************************
 *           alloc_reserved_area_callback
 *
 * Try to map some space inside a reserved area. Callback for mmap_enum_reserved_areas.
 */
static int CDECL alloc_reserved_area_callback( void *start, SIZE_T size, void *arg )
{
    struct alloc_area *alloc = arg;
    void *end = (char *)start + size;

    if (start < address_space_start) start = address_space_start;
    if (is_beyond_limit( start, size, alloc->limit )) end = alloc->limit;
    if (start >= end) return 0;

    /* make sure we don't touch the preloader reserved range */
    if (preload_reserve_end >= start)
    {
        if (preload_reserve_end >= end)
        {
            if (preload_reserve_start <= start) return 0;  /* no space in that area */
            if (preload_reserve_start < end) end = preload_reserve_start;
        }
        else if (preload_reserve_start <= start) start = preload_reserve_end;
        else
        {
            /* range is split in two by the preloader reservation, try first part */
            if ((alloc->result = find_reserved_free_area( start, preload_reserve_start, alloc->size,
                                                          alloc->top_down )))
                return 1;
            /* then fall through to try second part */
            start = preload_reserve_end;
        }
    }
    if ((alloc->result = find_reserved_free_area( start, end, alloc->size, alloc->top_down )))
        return 1;

    return 0;
}

/***********************************************************************
 *           map_fixed_area
 *
 * mmap the fixed memory area.
 * virtual_mutex must be held by caller.
 */
static NTSTATUS map_fixed_area( void *base, size_t size, unsigned int vprot )
{
    void *ptr;

    switch (mmap_is_in_reserved_area( base, size ))
    {
    case -1: /* partially in a reserved area */
    {
        NTSTATUS status;
        struct area_boundary area;
        size_t lower_size;
        area.base = base;
        area.size = size;
        mmap_enum_reserved_areas( get_area_boundary_callback, &area, 0 );
        assert( area.boundary );
        lower_size = (char *)area.boundary - (char *)base;
        status = map_fixed_area( base, lower_size, vprot );
        if (status == STATUS_SUCCESS)
        {
            status = map_fixed_area( area.boundary, size - lower_size, vprot);
            if (status != STATUS_SUCCESS) unmap_area( base, lower_size );
        }
        return status;
    }
    case 0:  /* not in a reserved area, do a normal allocation */
        if ((ptr = anon_mmap_tryfixed( base, size, get_unix_prot(vprot), 0 )) == MAP_FAILED)
        {
            if (errno == ENOMEM) return STATUS_NO_MEMORY;
            if (errno == EEXIST) return STATUS_CONFLICTING_ADDRESSES;
            return STATUS_INVALID_PARAMETER;
        }
        break;

    default:
    case 1:  /* in a reserved area, make sure the address is available */
        if (find_view_range( base, size )) return STATUS_CONFLICTING_ADDRESSES;
        /* replace the reserved area by our mapping */
        if ((ptr = anon_mmap_fixed( base, size, get_unix_prot(vprot), 0 )) != base)
            return STATUS_INVALID_PARAMETER;
        break;
    }
    if (is_beyond_limit( ptr, size, working_set_limit )) working_set_limit = address_space_limit;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           map_view
 *
 * Create a view and mmap the corresponding memory area.
 * virtual_mutex must be held by caller.
 */
static NTSTATUS map_view( struct file_view **view_ret, void *base, size_t size,
                          int top_down, unsigned int vprot, ULONG_PTR zero_bits )
{
    void *ptr;
    NTSTATUS status;

    if (base)
    {
        if (is_beyond_limit( base, size, address_space_limit ))
            return STATUS_WORKING_SET_LIMIT_RANGE;
        status = map_fixed_area( base, size, vprot );
        if (status != STATUS_SUCCESS) return status;
        ptr = base;
    }
    else
    {
        size_t view_size = size + granularity_mask + 1;
        struct alloc_area alloc;

        alloc.size = size;
        alloc.top_down = top_down;
        alloc.limit = (void*)(get_zero_bits_mask( zero_bits ) & (UINT_PTR)user_space_limit);

        if (mmap_enum_reserved_areas( alloc_reserved_area_callback, &alloc, top_down ))
        {
            ptr = alloc.result;
            TRACE( "got mem in reserved area %p-%p\n", ptr, (char *)ptr + size );
            if (anon_mmap_fixed( ptr, size, get_unix_prot(vprot), 0 ) != ptr)
                return STATUS_INVALID_PARAMETER;
            goto done;
        }

        if (zero_bits)
        {
            if (!(ptr = map_free_area( address_space_start, alloc.limit, size,
                                       top_down, get_unix_prot(vprot) )))
                return STATUS_NO_MEMORY;
            TRACE( "got mem with map_free_area %p-%p\n", ptr, (char *)ptr + size );
            goto done;
        }

        for (;;)
        {
            if ((ptr = anon_mmap_alloc( view_size, get_unix_prot(vprot) )) == MAP_FAILED)
            {
                if (errno == ENOMEM) return STATUS_NO_MEMORY;
                return STATUS_INVALID_PARAMETER;
            }
            TRACE( "got mem with anon mmap %p-%p\n", ptr, (char *)ptr + size );
            /* if we got something beyond the user limit, unmap it and retry */
            if (is_beyond_limit( ptr, view_size, user_space_limit )) add_reserved_area( ptr, view_size );
            else break;
        }
        ptr = unmap_extra_space( ptr, view_size, size );
    }
done:
    status = create_view( view_ret, ptr, size, vprot );
    if (status != STATUS_SUCCESS) unmap_area( ptr, size );
    return status;
}


/***********************************************************************
 *           map_file_into_view
 *
 * Wrapper for mmap() to map a file into a view, falling back to read if mmap fails.
 * virtual_mutex must be held by caller.
 */
static NTSTATUS map_file_into_view( struct file_view *view, int fd, size_t start, size_t size,
                                    off_t offset, unsigned int vprot, BOOL removable )
{
    void *ptr;
    int prot = get_unix_prot( vprot | VPROT_COMMITTED /* make sure it is accessible */ );
    unsigned int flags = MAP_FIXED | ((vprot & VPROT_WRITECOPY) ? MAP_PRIVATE : MAP_SHARED);

    assert( start < view->size );
    assert( start + size <= view->size );

    if (force_exec_prot && (vprot & VPROT_READ))
    {
        TRACE( "forcing exec permission on mapping %p-%p\n",
               (char *)view->base + start, (char *)view->base + start + size - 1 );
        prot |= PROT_EXEC;
    }

    /* only try mmap if media is not removable (or if we require write access) */
    if (!removable || (flags & MAP_SHARED))
    {
        if (mmap( (char *)view->base + start, size, prot, flags, fd, offset ) != MAP_FAILED)
            goto done;

        switch (errno)
        {
        case EINVAL:  /* file offset is not page-aligned, fall back to read() */
            if (flags & MAP_SHARED) return STATUS_INVALID_PARAMETER;
            break;
        case ENOEXEC:
        case ENODEV:  /* filesystem doesn't support mmap(), fall back to read() */
            if (vprot & VPROT_WRITE)
            {
                ERR( "shared writable mmap not supported, broken filesystem?\n" );
                return STATUS_NOT_SUPPORTED;
            }
            break;
        case EACCES:
        case EPERM:  /* noexec filesystem, fall back to read() */
            if (flags & MAP_SHARED)
            {
                if (prot & PROT_EXEC) ERR( "failed to set PROT_EXEC on file map, noexec filesystem?\n" );
                return STATUS_ACCESS_DENIED;
            }
            if (prot & PROT_EXEC) WARN( "failed to set PROT_EXEC on file map, noexec filesystem?\n" );
            break;
        default:
            return STATUS_NO_MEMORY;
        }
    }

    /* Reserve the memory with an anonymous mmap */
    ptr = anon_mmap_fixed( (char *)view->base + start, size, PROT_READ | PROT_WRITE, 0 );
    if (ptr == MAP_FAILED) return STATUS_NO_MEMORY;
    /* Now read in the file */
    pread( fd, ptr, size, offset );
    if (prot != (PROT_READ|PROT_WRITE)) mprotect( ptr, size, prot );  /* Set the right protection */
done:
    set_page_vprot( (char *)view->base + start, size, vprot );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           get_committed_size
 *
 * Get the size of the committed range starting at base.
 * Also return the protections for the first page.
 */
static SIZE_T get_committed_size( struct file_view *view, void *base, BYTE *vprot )
{
    SIZE_T i, start;

    start = ((char *)base - (char *)view->base) >> page_shift;
    *vprot = get_page_vprot( base );

    if (view->protect & SEC_RESERVE)
    {
        SIZE_T ret = 0;
        SERVER_START_REQ( get_mapping_committed_range )
        {
            req->base   = wine_server_client_ptr( view->base );
            req->offset = start << page_shift;
            if (!wine_server_call( req ))
            {
                ret = reply->size;
                if (reply->committed)
                {
                    *vprot |= VPROT_COMMITTED;
                    set_page_vprot_bits( base, ret, VPROT_COMMITTED, 0 );
                }
            }
        }
        SERVER_END_REQ;
        return ret;
    }
    for (i = start + 1; i < view->size >> page_shift; i++)
        if ((*vprot ^ get_page_vprot( (char *)view->base + (i << page_shift) )) & VPROT_COMMITTED) break;
    return (i - start) << page_shift;
}


/***********************************************************************
 *           decommit_view
 *
 * Decommit some pages of a given view.
 * virtual_mutex must be held by caller.
 */
static NTSTATUS decommit_pages( struct file_view *view, size_t start, size_t size )
{
    if (anon_mmap_fixed( (char *)view->base + start, size, PROT_NONE, 0 ) != MAP_FAILED)
    {
        set_page_vprot_bits( (char *)view->base + start, size, 0, VPROT_COMMITTED );
        return STATUS_SUCCESS;
    }
    return STATUS_NO_MEMORY;
}


/***********************************************************************
 *           allocate_dos_memory
 *
 * Allocate the DOS memory range.
 */
static NTSTATUS allocate_dos_memory( struct file_view **view, unsigned int vprot )
{
    size_t size;
    void *addr = NULL;
    void * const low_64k = (void *)0x10000;
    const size_t dosmem_size = 0x110000;
    int unix_prot = get_unix_prot( vprot );

    /* check for existing view */

    if (find_view_range( 0, dosmem_size )) return STATUS_CONFLICTING_ADDRESSES;

    /* check without the first 64K */

    if (mmap_is_in_reserved_area( low_64k, dosmem_size - 0x10000 ) != 1)
    {
        addr = anon_mmap_tryfixed( low_64k, dosmem_size - 0x10000, unix_prot, 0 );
        if (addr == MAP_FAILED) return map_view( view, NULL, dosmem_size, FALSE, vprot, 0 );
    }

    /* now try to allocate the low 64K too */

    if (mmap_is_in_reserved_area( NULL, 0x10000 ) != 1)
    {
        addr = anon_mmap_tryfixed( (void *)page_size, 0x10000 - page_size, unix_prot, 0 );
        if (addr != MAP_FAILED)
        {
            if (!anon_mmap_fixed( NULL, page_size, unix_prot, 0 ))
            {
                addr = NULL;
                TRACE( "successfully mapped low 64K range\n" );
            }
            else TRACE( "failed to map page 0\n" );
        }
        else
        {
            addr = low_64k;
            TRACE( "failed to map low 64K range\n" );
        }
    }

    /* now reserve the whole range */

    size = (char *)dosmem_size - (char *)addr;
    anon_mmap_fixed( addr, size, unix_prot, 0 );
    return create_view( view, addr, size, vprot );
}


/***********************************************************************
 *           map_pe_header
 *
 * Map the header of a PE file into memory.
 */
static NTSTATUS map_pe_header( void *ptr, size_t size, int fd, BOOL *removable )
{
    if (!size) return STATUS_INVALID_IMAGE_FORMAT;

    if (!*removable)
    {
        if (mmap( ptr, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_FIXED|MAP_PRIVATE, fd, 0 ) != MAP_FAILED)
            return STATUS_SUCCESS;

        switch (errno)
        {
        case EPERM:
        case EACCES:
            WARN( "noexec file system, falling back to read\n" );
            break;
        case ENOEXEC:
        case ENODEV:
            WARN( "file system doesn't support mmap, falling back to read\n" );
            break;
        default:
            return STATUS_NO_MEMORY;
        }
        *removable = TRUE;
    }
    pread( fd, ptr, size, 0 );
    return STATUS_SUCCESS;  /* page protections will be updated later */
}


/***********************************************************************
 *           map_image_into_view
 *
 * Map an executable (PE format) image into an existing view.
 * virtual_mutex must be held by caller.
 */
static NTSTATUS map_image_into_view( struct file_view *view, const WCHAR *filename, int fd, void *orig_base,
                                     SIZE_T header_size, ULONG image_flags, int shared_fd, BOOL removable )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER sections[96];
    IMAGE_SECTION_HEADER *sec;
    IMAGE_DATA_DIRECTORY *imports;
    NTSTATUS status = STATUS_CONFLICTING_ADDRESSES;
    int i;
    off_t pos;
    struct stat st;
    char *header_end, *header_start;
    char *ptr = view->base;
    SIZE_T total_size = view->size;

    TRACE_(module)( "mapping PE file %s at %p-%p\n", debugstr_w(filename), ptr, ptr + total_size );

    /* map the header */

    fstat( fd, &st );
    header_size = min( header_size, st.st_size );
    if ((status = map_pe_header( view->base, header_size, fd, &removable ))) return status;

    status = STATUS_INVALID_IMAGE_FORMAT;  /* generic error */
    dos = (IMAGE_DOS_HEADER *)ptr;
    nt = (IMAGE_NT_HEADERS *)(ptr + dos->e_lfanew);
    header_end = ptr + ROUND_SIZE( 0, header_size );
    memset( ptr + header_size, 0, header_end - (ptr + header_size) );
    if ((char *)(nt + 1) > header_end) return status;
    header_start = (char*)&nt->OptionalHeader+nt->FileHeader.SizeOfOptionalHeader;
    if (nt->FileHeader.NumberOfSections > ARRAY_SIZE( sections )) return status;
    if (header_start + sizeof(*sections) * nt->FileHeader.NumberOfSections > header_end) return status;
    /* Some applications (e.g. the Steam version of Borderlands) map over the top of the section headers,
     * copying the headers into local memory is necessary to properly load such applications. */
    memcpy(sections, header_start, sizeof(*sections) * nt->FileHeader.NumberOfSections);
    sec = sections;

    imports = nt->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_IMPORT;
    if (!imports->Size || !imports->VirtualAddress) imports = NULL;

    /* check for non page-aligned binary */

    if (image_flags & IMAGE_FLAGS_ImageMappedFlat)
    {
        /* unaligned sections, this happens for native subsystem binaries */
        /* in that case Windows simply maps in the whole file */

        total_size = min( total_size, ROUND_SIZE( 0, st.st_size ));
        if (map_file_into_view( view, fd, 0, total_size, 0, VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY,
                                removable ) != STATUS_SUCCESS) return status;

        /* check that all sections are loaded at the right offset */
        if (nt->OptionalHeader.FileAlignment != nt->OptionalHeader.SectionAlignment) return status;
        for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
        {
            if (sec[i].VirtualAddress != sec[i].PointerToRawData)
                return status;  /* Windows refuses to load in that case too */
        }

        /* set the image protections */
        set_vprot( view, ptr, total_size, VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY | VPROT_EXEC );

        /* no relocations are performed on non page-aligned binaries */
        return STATUS_SUCCESS;
    }


    /* map all the sections */

    for (i = pos = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        static const SIZE_T sector_align = 0x1ff;
        SIZE_T map_size, file_start, file_size, end;

        if (!sec->Misc.VirtualSize)
            map_size = ROUND_SIZE( 0, sec->SizeOfRawData );
        else
            map_size = ROUND_SIZE( 0, sec->Misc.VirtualSize );

        /* file positions are rounded to sector boundaries regardless of OptionalHeader.FileAlignment */
        file_start = sec->PointerToRawData & ~sector_align;
        file_size = (sec->SizeOfRawData + (sec->PointerToRawData & sector_align) + sector_align) & ~sector_align;
        if (file_size > map_size) file_size = map_size;

        /* a few sanity checks */
        end = sec->VirtualAddress + ROUND_SIZE( sec->VirtualAddress, map_size );
        if (sec->VirtualAddress > total_size || end > total_size || end < sec->VirtualAddress)
        {
            WARN_(module)( "%s section %.8s too large (%x+%lx/%lx)\n",
                           debugstr_w(filename), sec->Name, sec->VirtualAddress, map_size, total_size );
            return status;
        }

        if ((sec->Characteristics & IMAGE_SCN_MEM_SHARED) &&
            (sec->Characteristics & IMAGE_SCN_MEM_WRITE))
        {
            TRACE_(module)( "%s mapping shared section %.8s at %p off %x (%x) size %lx (%lx) flags %x\n",
                            debugstr_w(filename), sec->Name, ptr + sec->VirtualAddress,
                            sec->PointerToRawData, (int)pos, file_size, map_size,
                            sec->Characteristics );
            if (map_file_into_view( view, shared_fd, sec->VirtualAddress, map_size, pos,
                                    VPROT_COMMITTED | VPROT_READ | VPROT_WRITE, FALSE ) != STATUS_SUCCESS)
            {
                ERR_(module)( "Could not map %s shared section %.8s\n", debugstr_w(filename), sec->Name );
                return status;
            }

            /* check if the import directory falls inside this section */
            if (imports && imports->VirtualAddress >= sec->VirtualAddress &&
                imports->VirtualAddress < sec->VirtualAddress + map_size)
            {
                UINT_PTR base = imports->VirtualAddress & ~page_mask;
                UINT_PTR end = base + ROUND_SIZE( imports->VirtualAddress, imports->Size );
                if (end > sec->VirtualAddress + map_size) end = sec->VirtualAddress + map_size;
                if (end > base)
                    map_file_into_view( view, shared_fd, base, end - base,
                                        pos + (base - sec->VirtualAddress),
                                        VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY, FALSE );
            }
            pos += map_size;
            continue;
        }

        TRACE_(module)( "mapping %s section %.8s at %p off %x size %x virt %x flags %x\n",
                        debugstr_w(filename), sec->Name, ptr + sec->VirtualAddress,
                        sec->PointerToRawData, sec->SizeOfRawData,
                        sec->Misc.VirtualSize, sec->Characteristics );

        if (!sec->PointerToRawData || !file_size) continue;

        /* Note: if the section is not aligned properly map_file_into_view will magically
         *       fall back to read(), so we don't need to check anything here.
         */
        end = file_start + file_size;
        if (sec->PointerToRawData >= st.st_size ||
            end > ((st.st_size + sector_align) & ~sector_align) ||
            end < file_start ||
            map_file_into_view( view, fd, sec->VirtualAddress, file_size, file_start,
                                VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY,
                                removable ) != STATUS_SUCCESS)
        {
            ERR_(module)( "Could not map %s section %.8s, file probably truncated\n",
                          debugstr_w(filename), sec->Name );
            return status;
        }

        if (file_size & page_mask)
        {
            end = ROUND_SIZE( 0, file_size );
            if (end > map_size) end = map_size;
            TRACE_(module)("clearing %p - %p\n",
                           ptr + sec->VirtualAddress + file_size,
                           ptr + sec->VirtualAddress + end );
            memset( ptr + sec->VirtualAddress + file_size, 0, end - file_size );
        }
    }

    /* set the image protections */

    set_vprot( view, ptr, ROUND_SIZE( 0, header_size ), VPROT_COMMITTED | VPROT_READ );

    sec = sections;
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        SIZE_T size;
        BYTE vprot = VPROT_COMMITTED;

        if (sec->Misc.VirtualSize)
            size = ROUND_SIZE( sec->VirtualAddress, sec->Misc.VirtualSize );
        else
            size = ROUND_SIZE( sec->VirtualAddress, sec->SizeOfRawData );

        if (sec->Characteristics & IMAGE_SCN_MEM_READ)    vprot |= VPROT_READ;
        if (sec->Characteristics & IMAGE_SCN_MEM_WRITE)   vprot |= VPROT_WRITECOPY;
        if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) vprot |= VPROT_EXEC;

        /* Dumb game crack lets the AOEP point into a data section. Adjust. */
        if ((nt->OptionalHeader.AddressOfEntryPoint >= sec->VirtualAddress) &&
            (nt->OptionalHeader.AddressOfEntryPoint < sec->VirtualAddress + size))
            vprot |= VPROT_EXEC;

        if (!set_vprot( view, ptr + sec->VirtualAddress, size, vprot ) && (vprot & VPROT_EXEC))
            ERR( "failed to set %08x protection on %s section %.8s, noexec filesystem?\n",
                 sec->Characteristics, debugstr_w(filename), sec->Name );
    }

#ifdef VALGRIND_LOAD_PDB_DEBUGINFO
    VALGRIND_LOAD_PDB_DEBUGINFO(fd, ptr, total_size, ptr - (char *)orig_base);
#endif
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             get_mapping_info
 */
static NTSTATUS get_mapping_info( HANDLE handle, ACCESS_MASK access, unsigned int *sec_flags,
                                  mem_size_t *full_size, HANDLE *shared_file, pe_image_info_t **info )
{
    pe_image_info_t *image_info;
    SIZE_T total, size = 1024;
    NTSTATUS status;

    for (;;)
    {
        if (!(image_info = malloc( size ))) return STATUS_NO_MEMORY;

        SERVER_START_REQ( get_mapping_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->access = access;
            wine_server_set_reply( req, image_info, size );
            status = wine_server_call( req );
            *sec_flags   = reply->flags;
            *full_size   = reply->size;
            total        = reply->total;
            *shared_file = wine_server_ptr_handle( reply->shared_file );
        }
        SERVER_END_REQ;
        if (!status && total <= size - sizeof(WCHAR)) break;
        free( image_info );
        if (status) return status;
        if (*shared_file) NtClose( *shared_file );
        size = total + sizeof(WCHAR);
    }

    if (total)
    {
        WCHAR *filename = (WCHAR *)(image_info + 1);

        assert( total >= sizeof(*image_info) );
        total -= sizeof(*image_info);
        filename[total / sizeof(WCHAR)] = 0;
        *info = image_info;
    }
    else free( image_info );

    return STATUS_SUCCESS;
}


/***********************************************************************
 *             virtual_map_image
 *
 * Map a PE image section into memory.
 */
static NTSTATUS virtual_map_image( HANDLE mapping, ACCESS_MASK access, void **addr_ptr, SIZE_T *size_ptr,
                                   ULONG_PTR zero_bits, HANDLE shared_file, ULONG alloc_type,
                                   pe_image_info_t *image_info, WCHAR *filename, BOOL is_builtin )
{
    unsigned int vprot = SEC_IMAGE | SEC_FILE | VPROT_COMMITTED | VPROT_READ | VPROT_EXEC | VPROT_WRITECOPY;
    int unix_fd = -1, needs_close;
    int shared_fd = -1, shared_needs_close = 0;
    SIZE_T size = image_info->map_size;
    struct file_view *view;
    NTSTATUS status;
    sigset_t sigset;
    void *base;

    if ((status = server_get_unix_fd( mapping, 0, &unix_fd, &needs_close, NULL, NULL )))
        return status;

    if (shared_file && ((status = server_get_unix_fd( shared_file, FILE_READ_DATA|FILE_WRITE_DATA,
                                                      &shared_fd, &shared_needs_close, NULL, NULL ))))
    {
        if (needs_close) close( unix_fd );
        return status;
    }

    status = STATUS_INVALID_PARAMETER;
    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    base = wine_server_get_ptr( image_info->base );
    if ((ULONG_PTR)base != image_info->base) base = NULL;

    if ((char *)base >= (char *)address_space_start)  /* make sure the DOS area remains free */
        status = map_view( &view, base, size, alloc_type & MEM_TOP_DOWN, vprot, zero_bits );

    if (status) status = map_view( &view, NULL, size, alloc_type & MEM_TOP_DOWN, vprot, zero_bits );
    if (status) goto done;

    status = map_image_into_view( view, filename, unix_fd, base, image_info->header_size,
                                  image_info->image_flags, shared_fd, needs_close );
    if (status == STATUS_SUCCESS)
    {
        SERVER_START_REQ( map_view )
        {
            req->mapping = wine_server_obj_handle( mapping );
            req->access  = access;
            req->base    = wine_server_client_ptr( view->base );
            req->size    = size;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    if (status >= 0)
    {
        if (is_builtin) add_builtin_module( view->base, NULL );
        *addr_ptr = view->base;
        *size_ptr = size;
        VIRTUAL_DEBUG_DUMP_VIEW( view );
    }
    else delete_view( view );

done:
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    if (needs_close) close( unix_fd );
    if (shared_needs_close) close( shared_fd );
    return status;
}


/***********************************************************************
 *             virtual_map_section
 *
 * Map a file section into memory.
 */
static NTSTATUS virtual_map_section( HANDLE handle, PVOID *addr_ptr, ULONG_PTR zero_bits,
                                     SIZE_T commit_size, const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                     ULONG alloc_type, ULONG protect )
{
    NTSTATUS res;
    mem_size_t full_size;
    ACCESS_MASK access;
    SIZE_T size;
    pe_image_info_t *image_info = NULL;
    WCHAR *filename;
    void *base;
    int unix_handle = -1, needs_close;
    unsigned int vprot, sec_flags;
    struct file_view *view;
    HANDLE shared_file;
    LARGE_INTEGER offset;
    sigset_t sigset;

    switch(protect)
    {
    case PAGE_NOACCESS:
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
        access = SECTION_MAP_READ;
        break;
    case PAGE_READWRITE:
        access = SECTION_MAP_WRITE;
        break;
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_WRITECOPY:
        access = SECTION_MAP_READ | SECTION_MAP_EXECUTE;
        break;
    case PAGE_EXECUTE_READWRITE:
        access = SECTION_MAP_WRITE | SECTION_MAP_EXECUTE;
        break;
    default:
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    res = get_mapping_info( handle, access, &sec_flags, &full_size, &shared_file, &image_info );
    if (res) return res;

    if (image_info)
    {
        filename = (WCHAR *)(image_info + 1);
        /* check if we can replace that mapping with the builtin */
        res = load_builtin( image_info, filename, addr_ptr, size_ptr );
        if (res == STATUS_IMAGE_ALREADY_LOADED)
            res = virtual_map_image( handle, access, addr_ptr, size_ptr, zero_bits, shared_file,
                                     alloc_type, image_info, filename, FALSE );
        if (shared_file) NtClose( shared_file );
        free( image_info );
        return res;
    }

    base = *addr_ptr;
    offset.QuadPart = offset_ptr ? offset_ptr->QuadPart : 0;
    if (offset.QuadPart >= full_size) return STATUS_INVALID_PARAMETER;
    if (*size_ptr)
    {
        size = *size_ptr;
        if (size > full_size - offset.QuadPart) return STATUS_INVALID_VIEW_SIZE;
    }
    else
    {
        size = full_size - offset.QuadPart;
        if (size != full_size - offset.QuadPart)  /* truncated */
        {
            WARN( "Files larger than 4Gb (%s) not supported on this platform\n",
                  wine_dbgstr_longlong(full_size) );
            return STATUS_INVALID_PARAMETER;
        }
    }
    if (!(size = ROUND_SIZE( 0, size ))) return STATUS_INVALID_PARAMETER;  /* wrap-around */

    get_vprot_flags( protect, &vprot, FALSE );
    vprot |= sec_flags;
    if (!(sec_flags & SEC_RESERVE)) vprot |= VPROT_COMMITTED;

    if ((res = server_get_unix_fd( handle, 0, &unix_handle, &needs_close, NULL, NULL ))) return res;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    res = map_view( &view, base, size, alloc_type & MEM_TOP_DOWN, vprot, zero_bits );
    if (res) goto done;

    TRACE( "handle=%p size=%lx offset=%x%08x\n", handle, size, offset.u.HighPart, offset.u.LowPart );
    res = map_file_into_view( view, unix_handle, 0, size, offset.QuadPart, vprot, needs_close );
    if (res == STATUS_SUCCESS)
    {
        SERVER_START_REQ( map_view )
        {
            req->mapping = wine_server_obj_handle( handle );
            req->access  = access;
            req->base    = wine_server_client_ptr( view->base );
            req->size    = size;
            req->start   = offset.QuadPart;
            res = wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    else ERR( "mapping %p %lx %x%08x failed\n", view->base, size, offset.u.HighPart, offset.u.LowPart );

    if (res >= 0)
    {
        *addr_ptr = view->base;
        *size_ptr = size;
        VIRTUAL_DEBUG_DUMP_VIEW( view );
    }
    else delete_view( view );

done:
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    if (needs_close) close( unix_handle );
    return res;
}


struct alloc_virtual_heap
{
    void  *base;
    size_t size;
};

/* callback for mmap_enum_reserved_areas to allocate space for the virtual heap */
static int CDECL alloc_virtual_heap( void *base, SIZE_T size, void *arg )
{
    struct alloc_virtual_heap *alloc = arg;
    void *end = (char *)base + size;

    if (is_beyond_limit( base, size, address_space_limit )) address_space_limit = (char *)base + size;
    if (is_win64 && base < (void *)0x80000000) return 0;
    if (preload_reserve_end >= end)
    {
        if (preload_reserve_start <= base) return 0;  /* no space in that area */
        if (preload_reserve_start < end) end = preload_reserve_start;
    }
    else if (preload_reserve_end > base)
    {
        if (preload_reserve_start <= base) base = preload_reserve_end;
        else if ((char *)end - (char *)preload_reserve_end >= alloc->size) base = preload_reserve_end;
        else end = preload_reserve_start;
    }
    if ((char *)end - (char *)base < alloc->size) return 0;
    alloc->base = anon_mmap_fixed( (char *)end - alloc->size, alloc->size, PROT_READ|PROT_WRITE, 0 );
    return (alloc->base != MAP_FAILED);
}

/***********************************************************************
 *           virtual_init
 */
void virtual_init(void)
{
    const struct preload_info **preload_info = dlsym( RTLD_DEFAULT, "wine_main_preload_info" );
    const char *preload = getenv( "WINEPRELOADRESERVE" );
    struct alloc_virtual_heap alloc_views;
    size_t size;
    int i;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &virtual_mutex, &attr );
    pthread_mutexattr_destroy( &attr );

    if (preload_info && *preload_info)
        for (i = 0; (*preload_info)[i].size; i++)
            mmap_add_reserved_area( (*preload_info)[i].addr, (*preload_info)[i].size );

    mmap_init( preload_info ? *preload_info : NULL );

    if ((preload = getenv("WINEPRELOADRESERVE")))
    {
        unsigned long start, end;
        if (sscanf( preload, "%lx-%lx", &start, &end ) == 2)
        {
            preload_reserve_start = (void *)start;
            preload_reserve_end = (void *)end;
            /* some apps start inside the DOS area */
            if (preload_reserve_start)
                address_space_start = min( address_space_start, preload_reserve_start );
        }
    }

    /* try to find space in a reserved area for the views and pages protection table */
#ifdef _WIN64
    pages_vprot_size = ((size_t)address_space_limit >> page_shift >> pages_vprot_shift) + 1;
    alloc_views.size = 2 * view_block_size + pages_vprot_size * sizeof(*pages_vprot);
#else
    alloc_views.size = 2 * view_block_size + (1U << (32 - page_shift));
#endif
    if (mmap_enum_reserved_areas( alloc_virtual_heap, &alloc_views, 1 ))
        mmap_remove_reserved_area( alloc_views.base, alloc_views.size );
    else
        alloc_views.base = anon_mmap_alloc( alloc_views.size, PROT_READ | PROT_WRITE );

    assert( alloc_views.base != MAP_FAILED );
    view_block_start = alloc_views.base;
    view_block_end = view_block_start + view_block_size / sizeof(*view_block_start);
    free_ranges = (void *)((char *)alloc_views.base + view_block_size);
    pages_vprot = (void *)((char *)alloc_views.base + 2 * view_block_size);
    wine_rb_init( &views_tree, compare_view );

    free_ranges[0].base = (void *)0;
    free_ranges[0].end = (void *)~0;
    free_ranges_end = free_ranges + 1;

    /* make the DOS area accessible (except the low 64K) to hide bugs in broken apps like Excel 2003 */
    size = (char *)address_space_start - (char *)0x10000;
    if (size && mmap_is_in_reserved_area( (void*)0x10000, size ) == 1)
        anon_mmap_fixed( (void *)0x10000, size, PROT_READ | PROT_WRITE, 0 );
}


/***********************************************************************
 *           get_system_affinity_mask
 */
ULONG_PTR get_system_affinity_mask(void)
{
    ULONG num_cpus = peb->NumberOfProcessors;
    if (num_cpus >= sizeof(ULONG_PTR) * 8) return ~(ULONG_PTR)0;
    return ((ULONG_PTR)1 << num_cpus) - 1;
}

/***********************************************************************
 *           virtual_get_system_info
 */
void virtual_get_system_info( SYSTEM_BASIC_INFORMATION *info, BOOL wow64 )
{
#if defined(HAVE_STRUCT_SYSINFO_TOTALRAM) && defined(HAVE_STRUCT_SYSINFO_MEM_UNIT)
    struct sysinfo sinfo;

    if (!sysinfo(&sinfo))
    {
        ULONG64 total = (ULONG64)sinfo.totalram * sinfo.mem_unit;
        info->MmHighestPhysicalPage = max(1, total / page_size);
    }
#elif defined(_SC_PHYS_PAGES)
    LONG64 phys_pages = sysconf( _SC_PHYS_PAGES );

    info->MmHighestPhysicalPage = max(1, phys_pages);
#else
    info->MmHighestPhysicalPage = 0x7fffffff / page_size;
#endif

    info->unknown                 = 0;
    info->KeMaximumIncrement      = 0;  /* FIXME */
    info->PageSize                = page_size;
    info->MmLowestPhysicalPage    = 1;
    info->MmNumberOfPhysicalPages = info->MmHighestPhysicalPage - info->MmLowestPhysicalPage;
    info->AllocationGranularity   = granularity_mask + 1;
    info->LowestUserAddress       = (void *)0x10000;
    info->ActiveProcessorsAffinityMask = get_system_affinity_mask();
    info->NumberOfProcessors      = peb->NumberOfProcessors;
#ifdef _WIN64
    if (wow64)
    {
        if (main_image_info.ImageCharacteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
            info->HighestUserAddress = (char *)0xc0000000 - 1;
        else
            info->HighestUserAddress = (char *)0x7fff0000 - 1;
        return;
    }
#endif
    info->HighestUserAddress = (char *)user_space_limit - 1;
}


/***********************************************************************
 *           virtual_map_builtin_module
 */
NTSTATUS virtual_map_builtin_module( HANDLE mapping, void **module, SIZE_T *size,
                                     SECTION_IMAGE_INFORMATION *info, WORD machine, BOOL prefer_native )
{
    mem_size_t full_size;
    unsigned int sec_flags;
    HANDLE shared_file;
    pe_image_info_t *image_info = NULL;
    ACCESS_MASK access = SECTION_MAP_READ | SECTION_MAP_EXECUTE;
    NTSTATUS status;
    WCHAR *filename;

    if ((status = get_mapping_info( mapping, access, &sec_flags, &full_size, &shared_file, &image_info )))
        return status;

    if (!image_info) return STATUS_INVALID_PARAMETER;

    *module = NULL;
    *size = 0;
    filename = (WCHAR *)(image_info + 1);

    if (!(image_info->image_flags & IMAGE_FLAGS_WineBuiltin)) /* ignore non-builtins */
    {
        WARN( "%s found in WINEDLLPATH but not a builtin, ignoring\n", debugstr_w(filename) );
        status = STATUS_DLL_NOT_FOUND;
    }
    else if (machine && image_info->machine != machine)
    {
        TRACE( "%s is for arch %04x, continuing search\n", debugstr_w(filename), image_info->machine );
        status = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
    }
    else if (prefer_native && (image_info->dll_charact & IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE))
    {
        TRACE( "%s has prefer-native flag, ignoring builtin\n", debugstr_w(filename) );
        status = STATUS_IMAGE_ALREADY_LOADED;
    }
    else
    {
        status = virtual_map_image( mapping, SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                                    module, size, 0, shared_file, 0, image_info, filename, TRUE );
        virtual_fill_image_information( image_info, info );
    }

    if (shared_file) NtClose( shared_file );
    free( image_info );
    return status;
}


/***********************************************************************
 *           virtual_create_builtin_view
 */
NTSTATUS virtual_create_builtin_view( void *module, const UNICODE_STRING *nt_name,
                                      pe_image_info_t *info, void *so_handle )
{
    NTSTATUS status;
    sigset_t sigset;
    IMAGE_DOS_HEADER *dos = module;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((char *)dos + dos->e_lfanew);
    SIZE_T size = info->map_size;
    IMAGE_SECTION_HEADER *sec;
    struct file_view *view;
    void *base = wine_server_get_ptr( info->base );
    int i;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    status = create_view( &view, base, size, SEC_IMAGE | SEC_FILE | VPROT_SYSTEM |
                          VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY | VPROT_EXEC );
    if (!status)
    {
        TRACE( "created %p-%p for %s\n", base, (char *)base + size, debugstr_us(nt_name) );

        /* The PE header is always read-only, no write, no execute. */
        set_page_vprot( base, page_size, VPROT_COMMITTED | VPROT_READ );

        sec = (IMAGE_SECTION_HEADER *)((char *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
        for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
        {
            BYTE flags = VPROT_COMMITTED;

            if (sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) flags |= VPROT_EXEC;
            if (sec[i].Characteristics & IMAGE_SCN_MEM_READ) flags |= VPROT_READ;
            if (sec[i].Characteristics & IMAGE_SCN_MEM_WRITE) flags |= VPROT_WRITE;
            set_page_vprot( (char *)base + sec[i].VirtualAddress, sec[i].Misc.VirtualSize, flags );
        }

        SERVER_START_REQ( map_view )
        {
            req->base = wine_server_client_ptr( view->base );
            req->size = size;
            wine_server_add_data( req, info, sizeof(*info) );
            wine_server_add_data( req, nt_name->Buffer, nt_name->Length );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;

        if (status >= 0)
        {
            add_builtin_module( view->base, so_handle );
            VIRTUAL_DEBUG_DUMP_VIEW( view );
            if (is_beyond_limit( base, size, working_set_limit )) working_set_limit = address_space_limit;
        }
        else delete_view( view );
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );

    return status;
}


/* set some initial values in a new TEB */
static TEB *init_teb( void *ptr, BOOL is_wow )
{
    struct ntdll_thread_data *thread_data;
    TEB *teb;
    TEB64 *teb64 = ptr;
    TEB32 *teb32 = (TEB32 *)((char *)ptr + teb_offset);

#ifdef _WIN64
    teb = (TEB *)teb64;
    teb32->Peb = PtrToUlong( (char *)peb + page_size );
    teb32->Tib.Self = PtrToUlong( teb32 );
    teb32->Tib.ExceptionList = ~0u;
    teb32->ActivationContextStackPointer = PtrToUlong( &teb32->ActivationContextStack );
    teb32->ActivationContextStack.FrameListCache.Flink =
        teb32->ActivationContextStack.FrameListCache.Blink =
            PtrToUlong( &teb32->ActivationContextStack.FrameListCache );
    teb32->StaticUnicodeString.Buffer = PtrToUlong( teb32->StaticUnicodeBuffer );
    teb32->StaticUnicodeString.MaximumLength = sizeof( teb32->StaticUnicodeBuffer );
    teb32->GdiBatchCount = PtrToUlong( teb64 );
    teb32->WowTebOffset  = -teb_offset;
    if (is_wow) teb64->WowTebOffset = teb_offset;
#else
    teb = (TEB *)teb32;
    teb64->Peb = PtrToUlong( (char *)peb - page_size );
    teb64->Tib.Self = PtrToUlong( teb64 );
    teb64->Tib.ExceptionList = PtrToUlong( teb32 );
    teb64->ActivationContextStackPointer = PtrToUlong( &teb64->ActivationContextStack );
    teb64->ActivationContextStack.FrameListCache.Flink =
        teb64->ActivationContextStack.FrameListCache.Blink =
            PtrToUlong( &teb64->ActivationContextStack.FrameListCache );
    teb64->StaticUnicodeString.Buffer = PtrToUlong( teb64->StaticUnicodeBuffer );
    teb64->StaticUnicodeString.MaximumLength = sizeof( teb64->StaticUnicodeBuffer );
    teb64->WowTebOffset = teb_offset;
    if (is_wow)
    {
        teb32->GdiBatchCount = PtrToUlong( teb64 );
        teb32->WowTebOffset  = -teb_offset;
    }
#endif
    teb->Peb = peb;
    teb->Tib.Self = &teb->Tib;
    teb->Tib.ExceptionList = (void *)~0ul;
    teb->Tib.StackBase = (void *)~0ul;
    teb->ActivationContextStackPointer = &teb->ActivationContextStack;
    InitializeListHead( &teb->ActivationContextStack.FrameListCache );
    teb->StaticUnicodeString.Buffer = teb->StaticUnicodeBuffer;
    teb->StaticUnicodeString.MaximumLength = sizeof(teb->StaticUnicodeBuffer);
    thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    thread_data->request_fd = -1;
    thread_data->reply_fd   = -1;
    thread_data->wait_fd[0] = -1;
    thread_data->wait_fd[1] = -1;
    list_add_head( &teb_list, &thread_data->entry );
    return teb;
}


/***********************************************************************
 *           virtual_alloc_first_teb
 */
TEB *virtual_alloc_first_teb(void)
{
    void *ptr;
    NTSTATUS status;
    SIZE_T data_size = page_size;
    SIZE_T block_size = signal_stack_mask + 1;
    SIZE_T total = 32 * block_size;

    /* reserve space for shared user data */
    status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&user_shared_data, 0, &data_size,
                                      MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
    if (status)
    {
        ERR( "wine: failed to map the shared user data: %08x\n", status );
        exit(1);
    }

    NtAllocateVirtualMemory( NtCurrentProcess(), &teb_block, is_win64 ? 0x7fffffff : 0, &total,
                             MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE );
    teb_block_pos = 30;
    ptr = (char *)teb_block + 30 * block_size;
    data_size = 2 * block_size;
    NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&ptr, 0, &data_size, MEM_COMMIT, PAGE_READWRITE );
    peb = (PEB *)((char *)teb_block + 31 * block_size + (is_win64 ? 0 : page_size));
    return init_teb( ptr, FALSE );
}


/***********************************************************************
 *           virtual_alloc_teb
 */
NTSTATUS virtual_alloc_teb( TEB **ret_teb )
{
    sigset_t sigset;
    TEB *teb;
    void *ptr = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T block_size = signal_stack_mask + 1;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if (next_free_teb)
    {
        ptr = next_free_teb;
        next_free_teb = *(void **)ptr;
        memset( ptr, 0, teb_size );
    }
    else
    {
        if (!teb_block_pos)
        {
            SIZE_T total = 32 * block_size;

            if ((status = NtAllocateVirtualMemory( NtCurrentProcess(), &ptr, is_win64 ? 0x7fffffff : 0,
                                                   &total, MEM_RESERVE, PAGE_READWRITE )))
            {
                server_leave_uninterrupted_section( &virtual_mutex, &sigset );
                return status;
            }
            teb_block = ptr;
            teb_block_pos = 32;
        }
        ptr = ((char *)teb_block + --teb_block_pos * block_size);
        NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&ptr, 0, &block_size,
                                 MEM_COMMIT, PAGE_READWRITE );
    }
    *ret_teb = teb = init_teb( ptr, !!NtCurrentTeb()->WowTebOffset );
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );

    if ((status = signal_alloc_thread( teb )))
    {
        server_enter_uninterrupted_section( &virtual_mutex, &sigset );
        *(void **)ptr = next_free_teb;
        next_free_teb = ptr;
        server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    }
    return status;
}


/***********************************************************************
 *           virtual_free_teb
 */
void virtual_free_teb( TEB *teb )
{
    struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&teb->GdiTebBatch;
    void *ptr;
    SIZE_T size;
    sigset_t sigset;
    WOW_TEB *wow_teb = get_wow_teb( teb );

    signal_free_thread( teb );
    if (teb->DeallocationStack)
    {
        size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &teb->DeallocationStack, &size, MEM_RELEASE );
    }
    if (thread_data->kernel_stack)
    {
        size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &thread_data->kernel_stack, &size, MEM_RELEASE );
    }
    if (wow_teb && (ptr = ULongToPtr( wow_teb->DeallocationStack )))
    {
        size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &ptr, &size, MEM_RELEASE );
    }

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    list_remove( &thread_data->entry );
    ptr = teb;
    if (!is_win64) ptr = (char *)ptr - teb_offset;
    *(void **)ptr = next_free_teb;
    next_free_teb = ptr;
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
}


/***********************************************************************
 *           virtual_clear_tls_index
 */
NTSTATUS virtual_clear_tls_index( ULONG index )
{
    struct ntdll_thread_data *thread_data;
    sigset_t sigset;

    if (index < TLS_MINIMUM_AVAILABLE)
    {
        server_enter_uninterrupted_section( &virtual_mutex, &sigset );
        LIST_FOR_EACH_ENTRY( thread_data, &teb_list, struct ntdll_thread_data, entry )
        {
            TEB *teb = CONTAINING_RECORD( thread_data, TEB, GdiTebBatch );
#ifdef _WIN64
            WOW_TEB *wow_teb = get_wow_teb( teb );
            if (wow_teb) wow_teb->TlsSlots[index] = 0;
            else
#endif
            teb->TlsSlots[index] = 0;
        }
        server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    }
    else
    {
        index -= TLS_MINIMUM_AVAILABLE;
        if (index >= 8 * sizeof(peb->TlsExpansionBitmapBits)) return STATUS_INVALID_PARAMETER;

        server_enter_uninterrupted_section( &virtual_mutex, &sigset );
        LIST_FOR_EACH_ENTRY( thread_data, &teb_list, struct ntdll_thread_data, entry )
        {
            TEB *teb = CONTAINING_RECORD( thread_data, TEB, GdiTebBatch );
#ifdef _WIN64
            WOW_TEB *wow_teb = get_wow_teb( teb );
            if (wow_teb)
            {
                if (wow_teb->TlsExpansionSlots)
                    ((ULONG *)ULongToPtr( wow_teb->TlsExpansionSlots ))[index] = 0;
            }
            else
#endif
            if (teb->TlsExpansionSlots) teb->TlsExpansionSlots[index] = 0;
        }
        server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           virtual_alloc_thread_stack
 */
NTSTATUS virtual_alloc_thread_stack( INITIAL_TEB *stack, ULONG_PTR zero_bits, SIZE_T reserve_size,
                                     SIZE_T commit_size, SIZE_T extra_size )
{
    struct file_view *view;
    NTSTATUS status;
    sigset_t sigset;
    SIZE_T size;

    if (!reserve_size) reserve_size = main_image_info.MaximumStackSize;
    if (!commit_size) commit_size = main_image_info.CommittedStackSize;

    size = max( reserve_size, commit_size );
    if (size < 1024 * 1024) size = 1024 * 1024;  /* Xlib needs a large stack */
    size = (size + 0xffff) & ~0xffff;  /* round to 64K boundary */

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    if ((status = map_view( &view, NULL, size + extra_size, FALSE,
                            VPROT_READ | VPROT_WRITE | VPROT_COMMITTED, zero_bits )) != STATUS_SUCCESS)
        goto done;

#ifdef VALGRIND_STACK_REGISTER
    VALGRIND_STACK_REGISTER( view->base, (char *)view->base + view->size );
#endif

    /* setup no access guard page */
    set_page_vprot( view->base, page_size, VPROT_COMMITTED );
    set_page_vprot( (char *)view->base + page_size, page_size,
                    VPROT_READ | VPROT_WRITE | VPROT_COMMITTED | VPROT_GUARD );
    mprotect_range( view->base, 2 * page_size, 0, 0 );
    VIRTUAL_DEBUG_DUMP_VIEW( view );

    if (extra_size)
    {
        struct file_view *extra_view;

        /* shrink the first view and create a second one for the extra size */
        /* this allows the app to free the stack without freeing the thread start portion */
        view->size -= extra_size;
        status = create_view( &extra_view, (char *)view->base + view->size, extra_size,
                              VPROT_READ | VPROT_WRITE | VPROT_COMMITTED );
        if (status != STATUS_SUCCESS)
        {
            view->size += extra_size;
            delete_view( view );
            goto done;
        }
    }

    /* note: limit is lower than base since the stack grows down */
    stack->OldStackBase = 0;
    stack->OldStackLimit = 0;
    stack->DeallocationStack = view->base;
    stack->StackBase = (char *)view->base + view->size;
    stack->StackLimit = (char *)view->base + 2 * page_size;
done:
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/***********************************************************************
 *           virtual_map_user_shared_data
 */
void virtual_map_user_shared_data(void)
{
    static const WCHAR nameW[] = {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
                                  '\\','_','_','w','i','n','e','_','u','s','e','r','_','s','h','a','r','e','d','_','d','a','t','a',0};
    UNICODE_STRING name_str = { sizeof(nameW) - sizeof(WCHAR), sizeof(nameW), (WCHAR *)nameW };
    OBJECT_ATTRIBUTES attr = { sizeof(attr), 0, &name_str };
    NTSTATUS status;
    HANDLE section;
    int res, fd, needs_close;

    if ((status = NtOpenSection( &section, SECTION_ALL_ACCESS, &attr )))
    {
        ERR( "failed to open the USD section: %08x\n", status );
        exit(1);
    }
    if ((res = server_get_unix_fd( section, 0, &fd, &needs_close, NULL, NULL )) ||
        (user_shared_data != mmap( user_shared_data, page_size, PROT_READ, MAP_SHARED|MAP_FIXED, fd, 0 )))
    {
        ERR( "failed to remap the process USD: %d\n", res );
        exit(1);
    }
    if (needs_close) close( fd );
    NtClose( section );
}


struct thread_stack_info
{
    char  *start;
    char  *limit;
    char  *end;
    SIZE_T guaranteed;
    BOOL   is_wow;
};

/***********************************************************************
 *           is_inside_thread_stack
 */
static BOOL is_inside_thread_stack( void *ptr, struct thread_stack_info *stack )
{
    TEB *teb = NtCurrentTeb();
    WOW_TEB *wow_teb = get_wow_teb( teb );

    stack->start = teb->DeallocationStack;
    stack->limit = teb->Tib.StackLimit;
    stack->end   = teb->Tib.StackBase;
    stack->guaranteed = max( teb->GuaranteedStackBytes, page_size * (is_win64 ? 2 : 1) );
    stack->is_wow = FALSE;
    if ((char *)ptr > stack->start && (char *)ptr <= stack->end) return TRUE;

    if (!wow_teb) return FALSE;
    stack->start = ULongToPtr( wow_teb->DeallocationStack );
    stack->limit = ULongToPtr( wow_teb->Tib.StackLimit );
    stack->end   = ULongToPtr( wow_teb->Tib.StackBase );
    stack->guaranteed = max( wow_teb->GuaranteedStackBytes, page_size * (is_win64 ? 1 : 2) );
    stack->is_wow = TRUE;
    return ((char *)ptr > stack->start && (char *)ptr <= stack->end);
}


/***********************************************************************
 *           grow_thread_stack
 */
static NTSTATUS grow_thread_stack( char *page, struct thread_stack_info *stack_info )
{
    NTSTATUS ret = 0;

    set_page_vprot_bits( page, page_size, 0, VPROT_GUARD );
    mprotect_range( page, page_size, 0, 0 );
    if (page >= stack_info->start + page_size + stack_info->guaranteed)
    {
        set_page_vprot_bits( page - page_size, page_size, VPROT_COMMITTED | VPROT_GUARD, 0 );
        mprotect_range( page - page_size, page_size, 0, 0 );
    }
    else  /* inside guaranteed space -> overflow exception */
    {
        page = stack_info->start + page_size;
        set_page_vprot_bits( page, stack_info->guaranteed, VPROT_COMMITTED, VPROT_GUARD );
        mprotect_range( page, stack_info->guaranteed, 0, 0 );
        ret = STATUS_STACK_OVERFLOW;
    }
    if (stack_info->is_wow)
    {
        WOW_TEB *wow_teb = get_wow_teb( NtCurrentTeb() );
        wow_teb->Tib.StackLimit = PtrToUlong( page );
    }
    else NtCurrentTeb()->Tib.StackLimit = page;
    return ret;
}


/***********************************************************************
 *           virtual_handle_fault
 */
NTSTATUS virtual_handle_fault( void *addr, DWORD err, void *stack )
{
    NTSTATUS ret = STATUS_ACCESS_VIOLATION;
    char *page = ROUND_ADDR( addr, page_mask );
    BYTE vprot;

    mutex_lock( &virtual_mutex );  /* no need for signal masking inside signal handler */
    vprot = get_page_vprot( page );
    if (!is_inside_signal_stack( stack ) && (vprot & VPROT_GUARD))
    {
        struct thread_stack_info stack_info;
        if (!is_inside_thread_stack( page, &stack_info ))
        {
            set_page_vprot_bits( page, page_size, 0, VPROT_GUARD );
            mprotect_range( page, page_size, 0, 0 );
            ret = STATUS_GUARD_PAGE_VIOLATION;
        }
        else ret = grow_thread_stack( page, &stack_info );
    }
    else if (err & EXCEPTION_WRITE_FAULT)
    {
        if (vprot & VPROT_WRITEWATCH)
        {
            set_page_vprot_bits( page, page_size, 0, VPROT_WRITEWATCH );
            mprotect_range( page, page_size, 0, 0 );
        }
        /* ignore fault if page is writable now */
        if (get_unix_prot( get_page_vprot( page )) & PROT_WRITE)
        {
            if ((vprot & VPROT_WRITEWATCH) || is_write_watch_range( page, page_size ))
                ret = STATUS_SUCCESS;
        }
    }
    mutex_unlock( &virtual_mutex );
    return ret;
}


/***********************************************************************
 *           virtual_setup_exception
 */
void *virtual_setup_exception( void *stack_ptr, size_t size, EXCEPTION_RECORD *rec )
{
    char *stack = stack_ptr;
    struct thread_stack_info stack_info;

    if (!is_inside_thread_stack( stack, &stack_info ))
    {
        if (is_inside_signal_stack( stack ))
        {
            ERR( "nested exception on signal stack in thread %04x addr %p stack %p\n",
                 GetCurrentThreadId(), rec->ExceptionAddress, stack );
            abort_thread(1);
        }
        WARN( "exception outside of stack limits in thread %04x addr %p stack %p (%p-%p-%p)\n",
              GetCurrentThreadId(), rec->ExceptionAddress, stack, NtCurrentTeb()->DeallocationStack,
              NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
        return stack - size;
    }

    stack -= size;

    if (stack < stack_info.start + 4096)
    {
        /* stack overflow on last page, unrecoverable */
        UINT diff = stack_info.start + 4096 - stack;
        ERR( "stack overflow %u bytes in thread %04x addr %p stack %p (%p-%p-%p)\n",
             diff, GetCurrentThreadId(), rec->ExceptionAddress, stack, stack_info.start,
             stack_info.limit, stack_info.end );
        abort_thread(1);
    }
    else if (stack < stack_info.limit)
    {
        mutex_lock( &virtual_mutex );  /* no need for signal masking inside signal handler */
        if ((get_page_vprot( stack ) & VPROT_GUARD) &&
            grow_thread_stack( ROUND_ADDR( stack, page_mask ), &stack_info ))
        {
            rec->ExceptionCode = STATUS_STACK_OVERFLOW;
            rec->NumberParameters = 0;
        }
        mutex_unlock( &virtual_mutex );
    }
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_MAKE_MEM_UNDEFINED( stack, size );
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_MAKE_WRITABLE( stack, size );
#endif
    return stack;
}


/***********************************************************************
 *           check_write_access
 *
 * Check if the memory range is writable, temporarily disabling write watches if necessary.
 */
static NTSTATUS check_write_access( void *base, size_t size, BOOL *has_write_watch )
{
    size_t i;
    char *addr = ROUND_ADDR( base, page_mask );

    size = ROUND_SIZE( base, size );
    for (i = 0; i < size; i += page_size)
    {
        BYTE vprot = get_page_vprot( addr + i );
        if (vprot & VPROT_WRITEWATCH) *has_write_watch = TRUE;
        if (!(get_unix_prot( vprot & ~VPROT_WRITEWATCH ) & PROT_WRITE))
            return STATUS_INVALID_USER_BUFFER;
    }
    if (*has_write_watch)
        mprotect_range( addr, size, 0, VPROT_WRITEWATCH );  /* temporarily enable write access */
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           virtual_locked_server_call
 */
unsigned int virtual_locked_server_call( void *req_ptr )
{
    struct __server_request_info * const req = req_ptr;
    sigset_t sigset;
    void *addr = req->reply_data;
    data_size_t size = req->u.req.request_header.reply_size;
    BOOL has_write_watch = FALSE;
    unsigned int ret = STATUS_ACCESS_VIOLATION;

    if (!size) return wine_server_call( req_ptr );

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if (!(ret = check_write_access( addr, size, &has_write_watch )))
    {
        ret = server_call_unlocked( req );
        if (has_write_watch) update_write_watches( addr, size, wine_server_reply_size( req ));
    }
    else memset( &req->u.reply, 0, sizeof(req->u.reply) );
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return ret;
}


/***********************************************************************
 *           virtual_locked_read
 */
ssize_t virtual_locked_read( int fd, void *addr, size_t size )
{
    sigset_t sigset;
    BOOL has_write_watch = FALSE;
    int err = EFAULT;

    ssize_t ret = read( fd, addr, size );
    if (ret != -1 || errno != EFAULT) return ret;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if (!check_write_access( addr, size, &has_write_watch ))
    {
        ret = read( fd, addr, size );
        err = errno;
        if (has_write_watch) update_write_watches( addr, size, max( 0, ret ));
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    errno = err;
    return ret;
}


/***********************************************************************
 *           virtual_locked_pread
 */
ssize_t virtual_locked_pread( int fd, void *addr, size_t size, off_t offset )
{
    sigset_t sigset;
    BOOL has_write_watch = FALSE;
    int err = EFAULT;

    ssize_t ret = pread( fd, addr, size, offset );
    if (ret != -1 || errno != EFAULT) return ret;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if (!check_write_access( addr, size, &has_write_watch ))
    {
        ret = pread( fd, addr, size, offset );
        err = errno;
        if (has_write_watch) update_write_watches( addr, size, max( 0, ret ));
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    errno = err;
    return ret;
}


/***********************************************************************
 *           virtual_locked_recvmsg
 */
ssize_t virtual_locked_recvmsg( int fd, struct msghdr *hdr, int flags )
{
    sigset_t sigset;
    size_t i;
    BOOL has_write_watch = FALSE;
    int err = EFAULT;

    ssize_t ret = recvmsg( fd, hdr, flags );
    if (ret != -1 || errno != EFAULT) return ret;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    for (i = 0; i < hdr->msg_iovlen; i++)
        if (check_write_access( hdr->msg_iov[i].iov_base, hdr->msg_iov[i].iov_len, &has_write_watch ))
            break;
    if (i == hdr->msg_iovlen)
    {
        ret = recvmsg( fd, hdr, flags );
        err = errno;
    }
    if (has_write_watch)
        while (i--) update_write_watches( hdr->msg_iov[i].iov_base, hdr->msg_iov[i].iov_len, 0 );

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    errno = err;
    return ret;
}


/***********************************************************************
 *           virtual_is_valid_code_address
 */
BOOL virtual_is_valid_code_address( const void *addr, SIZE_T size )
{
    struct file_view *view;
    BOOL ret = FALSE;
    sigset_t sigset;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if ((view = find_view( addr, size )))
        ret = !(view->protect & VPROT_SYSTEM);  /* system views are not visible to the app */
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return ret;
}


/***********************************************************************
 *           virtual_check_buffer_for_read
 *
 * Check if a memory buffer can be read, triggering page faults if needed for DIB section access.
 */
BOOL virtual_check_buffer_for_read( const void *ptr, SIZE_T size )
{
    if (!size) return TRUE;
    if (!ptr) return FALSE;

    __TRY
    {
        volatile const char *p = ptr;
        char dummy __attribute__((unused));
        SIZE_T count = size;

        while (count > page_size)
        {
            dummy = *p;
            p += page_size;
            count -= page_size;
        }
        dummy = p[0];
        dummy = p[count - 1];
    }
    __EXCEPT
    {
        return FALSE;
    }
    __ENDTRY
    return TRUE;
}


/***********************************************************************
 *           virtual_check_buffer_for_write
 *
 * Check if a memory buffer can be written to, triggering page faults if needed for write watches.
 */
BOOL virtual_check_buffer_for_write( void *ptr, SIZE_T size )
{
    if (!size) return TRUE;
    if (!ptr) return FALSE;

    __TRY
    {
        volatile char *p = ptr;
        SIZE_T count = size;

        while (count > page_size)
        {
            *p |= 0;
            p += page_size;
            count -= page_size;
        }
        p[0] |= 0;
        p[count - 1] |= 0;
    }
    __EXCEPT
    {
        return FALSE;
    }
    __ENDTRY
    return TRUE;
}


/***********************************************************************
 *           virtual_uninterrupted_read_memory
 *
 * Similar to NtReadVirtualMemory, but without wineserver calls. Moreover
 * permissions are checked before accessing each page, to ensure that no
 * exceptions can happen.
 */
SIZE_T virtual_uninterrupted_read_memory( const void *addr, void *buffer, SIZE_T size )
{
    struct file_view *view;
    sigset_t sigset;
    SIZE_T bytes_read = 0;

    if (!size) return 0;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if ((view = find_view( addr, size )))
    {
        if (!(view->protect & VPROT_SYSTEM))
        {
            while (bytes_read < size && (get_unix_prot( get_page_vprot( addr )) & PROT_READ))
            {
                SIZE_T block_size = min( size - bytes_read, page_size - ((UINT_PTR)addr & page_mask) );
                memcpy( buffer, addr, block_size );

                addr   = (const void *)((const char *)addr + block_size);
                buffer = (void *)((char *)buffer + block_size);
                bytes_read += block_size;
            }
        }
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return bytes_read;
}


/***********************************************************************
 *           virtual_uninterrupted_write_memory
 *
 * Similar to NtWriteVirtualMemory, but without wineserver calls. Moreover
 * permissions are checked before accessing each page, to ensure that no
 * exceptions can happen.
 */
NTSTATUS virtual_uninterrupted_write_memory( void *addr, const void *buffer, SIZE_T size )
{
    BOOL has_write_watch = FALSE;
    sigset_t sigset;
    NTSTATUS ret;

    if (!size) return STATUS_SUCCESS;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if (!(ret = check_write_access( addr, size, &has_write_watch )))
    {
        memcpy( addr, buffer, size );
        if (has_write_watch) update_write_watches( addr, size, size );
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return ret;
}


/***********************************************************************
 *           virtual_set_force_exec
 *
 * Whether to force exec prot on all views.
 */
void virtual_set_force_exec( BOOL enable )
{
    struct file_view *view;
    sigset_t sigset;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if (!force_exec_prot != !enable)  /* change all existing views */
    {
        force_exec_prot = enable;

        WINE_RB_FOR_EACH_ENTRY( view, &views_tree, struct file_view, entry )
        {
            /* file mappings are always accessible */
            BYTE commit = is_view_valloc( view ) ? 0 : VPROT_COMMITTED;

            mprotect_range( view->base, view->size, commit, 0 );
        }
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
}

struct free_range
{
    char *base;
    char *limit;
};

/* free reserved areas above the limit; callback for mmap_enum_reserved_areas */
static int CDECL free_reserved_memory( void *base, SIZE_T size, void *arg )
{
    struct free_range *range = arg;

    if ((char *)base >= range->limit) return 0;
    if ((char *)base + size <= range->base) return 0;
    if ((char *)base < range->base)
    {
        size -= range->base - (char *)base;
        base = range->base;
    }
    if ((char *)base + size > range->limit) size = range->limit - (char *)base;
    remove_reserved_area( base, size );
    return 1;  /* stop enumeration since the list has changed */
}

/***********************************************************************
 *           virtual_release_address_space
 *
 * Release some address space once we have loaded and initialized the app.
 */
void CDECL virtual_release_address_space(void)
{
    struct free_range range;
    sigset_t sigset;

    if (is_win64) return;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    range.base  = (char *)0x82000000;
    range.limit = user_space_limit;

    if (range.limit > range.base)
    {
        while (mmap_enum_reserved_areas( free_reserved_memory, &range, 1 )) /* nothing */;
#ifdef __APPLE__
        /* On macOS, we still want to free some of low memory, for OpenGL resources */
        range.base  = (char *)0x40000000;
#else
        range.base  = NULL;
#endif
    }
    else
        range.base = (char *)0x20000000;

    if (range.base)
    {
        range.limit = (char *)0x7f000000;
        while (mmap_enum_reserved_areas( free_reserved_memory, &range, 0 )) /* nothing */;
    }

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
}


/***********************************************************************
 *           virtual_set_large_address_space
 *
 * Enable use of a large address space when allowed by the application.
 */
void virtual_set_large_address_space(void)
{
    /* no large address space on win9x */
    if (peb->OSPlatformId != VER_PLATFORM_WIN32_NT) return;

    user_space_limit = working_set_limit = address_space_limit;
}


/***********************************************************************
 *             NtAllocateVirtualMemory   (NTDLL.@)
 *             ZwAllocateVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtAllocateVirtualMemory( HANDLE process, PVOID *ret, ULONG_PTR zero_bits,
                                         SIZE_T *size_ptr, ULONG type, ULONG protect )
{
    void *base;
    unsigned int vprot;
    BOOL is_dos_memory = FALSE;
    struct file_view *view;
    sigset_t sigset;
    SIZE_T size = *size_ptr;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE("%p %p %08lx %x %08x\n", process, *ret, size, type, protect );

    if (!size) return STATUS_INVALID_PARAMETER;
    if (zero_bits > 21 && zero_bits < 32) return STATUS_INVALID_PARAMETER_3;
    if (zero_bits > 32 && zero_bits < granularity_mask) return STATUS_INVALID_PARAMETER_3;
#ifndef _WIN64
    if (!is_wow64 && zero_bits >= 32) return STATUS_INVALID_PARAMETER_3;
#endif

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_alloc.type         = APC_VIRTUAL_ALLOC;
        call.virtual_alloc.addr         = wine_server_client_ptr( *ret );
        call.virtual_alloc.size         = *size_ptr;
        call.virtual_alloc.zero_bits    = zero_bits;
        call.virtual_alloc.op_type      = type;
        call.virtual_alloc.prot         = protect;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_alloc.status == STATUS_SUCCESS)
        {
            *ret      = wine_server_get_ptr( result.virtual_alloc.addr );
            *size_ptr = result.virtual_alloc.size;
        }
        return result.virtual_alloc.status;
    }

    /* Round parameters to a page boundary */

    if (is_beyond_limit( 0, size, working_set_limit )) return STATUS_WORKING_SET_LIMIT_RANGE;

    if (*ret)
    {
        if (type & MEM_RESERVE) /* Round down to 64k boundary */
            base = ROUND_ADDR( *ret, granularity_mask );
        else
            base = ROUND_ADDR( *ret, page_mask );
        size = (((UINT_PTR)*ret + size + page_mask) & ~page_mask) - (UINT_PTR)base;

        /* disallow low 64k, wrap-around and kernel space */
        if (((char *)base < (char *)0x10000) ||
            ((char *)base + size < (char *)base) ||
            is_beyond_limit( base, size, address_space_limit ))
        {
            /* address 1 is magic to mean DOS area */
            if (!base && *ret == (void *)1 && size == 0x110000) is_dos_memory = TRUE;
            else return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        base = NULL;
        size = (size + page_mask) & ~page_mask;
    }

    /* Compute the alloc type flags */

    if (!(type & (MEM_COMMIT | MEM_RESERVE | MEM_RESET)) ||
        (type & ~(MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN | MEM_WRITE_WATCH | MEM_RESET)))
    {
        WARN("called with wrong alloc type flags (%08x) !\n", type);
        return STATUS_INVALID_PARAMETER;
    }

    /* Reserve the memory */

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    if ((type & MEM_RESERVE) || !base)
    {
        if (!(status = get_vprot_flags( protect, &vprot, FALSE )))
        {
            if (type & MEM_COMMIT) vprot |= VPROT_COMMITTED;
            if (type & MEM_WRITE_WATCH) vprot |= VPROT_WRITEWATCH;
            if (protect & PAGE_NOCACHE) vprot |= SEC_NOCACHE;

            if (vprot & VPROT_WRITECOPY) status = STATUS_INVALID_PAGE_PROTECTION;
            else if (is_dos_memory) status = allocate_dos_memory( &view, vprot );
            else status = map_view( &view, base, size, type & MEM_TOP_DOWN, vprot, zero_bits );

            if (status == STATUS_SUCCESS) base = view->base;
        }
    }
    else if (type & MEM_RESET)
    {
        if (!(view = find_view( base, size ))) status = STATUS_NOT_MAPPED_VIEW;
        else madvise( base, size, MADV_DONTNEED );
    }
    else  /* commit the pages */
    {
        if (!(view = find_view( base, size ))) status = STATUS_NOT_MAPPED_VIEW;
        else if (view->protect & SEC_FILE) status = STATUS_ALREADY_COMMITTED;
        else if (!(status = set_protection( view, base, size, protect )) && (view->protect & SEC_RESERVE))
        {
            SERVER_START_REQ( add_mapping_committed_range )
            {
                req->base   = wine_server_client_ptr( view->base );
                req->offset = (char *)base - (char *)view->base;
                req->size   = size;
                wine_server_call( req );
            }
            SERVER_END_REQ;
        }
    }

    if (!status) VIRTUAL_DEBUG_DUMP_VIEW( view );

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );

    if (status == STATUS_SUCCESS)
    {
        *ret = base;
        *size_ptr = size;
    }
    return status;
}

/***********************************************************************
 *             NtAllocateVirtualMemoryEx   (NTDLL.@)
 *             ZwAllocateVirtualMemoryEx   (NTDLL.@)
 */
NTSTATUS WINAPI NtAllocateVirtualMemoryEx( HANDLE process, PVOID *ret, SIZE_T *size_ptr, ULONG type,
                                           ULONG protect, MEM_EXTENDED_PARAMETER *parameters,
                                           ULONG count )
{
    if (count && !parameters) return STATUS_INVALID_PARAMETER;

    if (count) FIXME( "Ignoring %d extended parameters %p\n", count, parameters );

    return NtAllocateVirtualMemory( process, ret, 0, size_ptr, type, protect );
}


/***********************************************************************
 *             NtFreeVirtualMemory   (NTDLL.@)
 *             ZwFreeVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtFreeVirtualMemory( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr, ULONG type )
{
    struct file_view *view;
    char *base;
    sigset_t sigset;
    NTSTATUS status = STATUS_SUCCESS;
    LPVOID addr = *addr_ptr;
    SIZE_T size = *size_ptr;

    TRACE("%p %p %08lx %x\n", process, addr, size, type );

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_free.type      = APC_VIRTUAL_FREE;
        call.virtual_free.addr      = wine_server_client_ptr( addr );
        call.virtual_free.size      = size;
        call.virtual_free.op_type   = type;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_free.status == STATUS_SUCCESS)
        {
            *addr_ptr = wine_server_get_ptr( result.virtual_free.addr );
            *size_ptr = result.virtual_free.size;
        }
        return result.virtual_free.status;
    }

    /* Fix the parameters */

    size = ROUND_SIZE( addr, size );
    base = ROUND_ADDR( addr, page_mask );

    /* avoid freeing the DOS area when a broken app passes a NULL pointer */
    if (!base) return STATUS_INVALID_PARAMETER;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    if (!(view = find_view( base, size )) || !is_view_valloc( view ))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if (type == MEM_RELEASE)
    {
        /* Free the pages */

        if (size || (base != view->base)) status = STATUS_INVALID_PARAMETER;
        else
        {
            delete_view( view );
            *addr_ptr = base;
            *size_ptr = size;
        }
    }
    else if (type == MEM_DECOMMIT)
    {
        status = decommit_pages( view, base - (char *)view->base, size );
        if (status == STATUS_SUCCESS)
        {
            *addr_ptr = base;
            *size_ptr = size;
        }
    }
    else
    {
        WARN("called with wrong free type flags (%08x) !\n", type);
        status = STATUS_INVALID_PARAMETER;
    }

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/***********************************************************************
 *             NtProtectVirtualMemory   (NTDLL.@)
 *             ZwProtectVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtProtectVirtualMemory( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr,
                                        ULONG new_prot, ULONG *old_prot )
{
    struct file_view *view;
    sigset_t sigset;
    NTSTATUS status = STATUS_SUCCESS;
    char *base;
    BYTE vprot;
    SIZE_T size = *size_ptr;
    LPVOID addr = *addr_ptr;
    DWORD old;

    TRACE("%p %p %08lx %08x\n", process, addr, size, new_prot );

    if (!old_prot)
        return STATUS_ACCESS_VIOLATION;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_protect.type = APC_VIRTUAL_PROTECT;
        call.virtual_protect.addr = wine_server_client_ptr( addr );
        call.virtual_protect.size = size;
        call.virtual_protect.prot = new_prot;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_protect.status == STATUS_SUCCESS)
        {
            *addr_ptr = wine_server_get_ptr( result.virtual_protect.addr );
            *size_ptr = result.virtual_protect.size;
            *old_prot = result.virtual_protect.prot;
        }
        return result.virtual_protect.status;
    }

    /* Fix the parameters */

    size = ROUND_SIZE( addr, size );
    base = ROUND_ADDR( addr, page_mask );

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    if ((view = find_view( base, size )))
    {
        /* Make sure all the pages are committed */
        if (get_committed_size( view, base, &vprot ) >= size && (vprot & VPROT_COMMITTED))
        {
            old = get_win32_prot( vprot, view->protect );
            status = set_protection( view, base, size, new_prot );
        }
        else status = STATUS_NOT_COMMITTED;
    }
    else status = STATUS_INVALID_PARAMETER;

    if (!status) VIRTUAL_DEBUG_DUMP_VIEW( view );

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );

    if (status == STATUS_SUCCESS)
    {
        *addr_ptr = base;
        *size_ptr = size;
        *old_prot = old;
    }
    return status;
}


/* retrieve state for a free memory area; callback for mmap_enum_reserved_areas */
static int CDECL get_free_mem_state_callback( void *start, SIZE_T size, void *arg )
{
    MEMORY_BASIC_INFORMATION *info = arg;
    void *end = (char *)start + size;

    if ((char *)info->BaseAddress + info->RegionSize <= (char *)start) return 0;

    if (info->BaseAddress >= end)
    {
        if (info->AllocationBase < end) info->AllocationBase = end;
        return 0;
    }

    if (info->BaseAddress >= start || start <= address_space_start)
    {
        /* it's a real free area */
        info->State             = MEM_FREE;
        info->Protect           = PAGE_NOACCESS;
        info->AllocationBase    = 0;
        info->AllocationProtect = 0;
        info->Type              = 0;
        if ((char *)info->BaseAddress + info->RegionSize > (char *)end)
            info->RegionSize = (char *)end - (char *)info->BaseAddress;
    }
    else /* outside of the reserved area, pretend it's allocated */
    {
        info->RegionSize        = (char *)start - (char *)info->BaseAddress;
#ifdef __i386__
        info->State             = MEM_RESERVE;
        info->Protect           = PAGE_NOACCESS;
        info->AllocationProtect = PAGE_NOACCESS;
        info->Type              = MEM_PRIVATE;
#else
        info->State             = MEM_FREE;
        info->Protect           = PAGE_NOACCESS;
        info->AllocationBase    = 0;
        info->AllocationProtect = 0;
        info->Type              = 0;
#endif
    }
    return 1;
}

/* get basic information about a memory block */
static NTSTATUS get_basic_memory_info( HANDLE process, LPCVOID addr,
                                       MEMORY_BASIC_INFORMATION *info,
                                       SIZE_T len, SIZE_T *res_len )
{
    struct file_view *view;
    char *base, *alloc_base = 0, *alloc_end = working_set_limit;
    struct wine_rb_entry *ptr;
    sigset_t sigset;

    if (len < sizeof(MEMORY_BASIC_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    if (process != NtCurrentProcess())
    {
        NTSTATUS status;
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_query.type = APC_VIRTUAL_QUERY;
        call.virtual_query.addr = wine_server_client_ptr( addr );
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_query.status == STATUS_SUCCESS)
        {
            info->BaseAddress       = wine_server_get_ptr( result.virtual_query.base );
            info->AllocationBase    = wine_server_get_ptr( result.virtual_query.alloc_base );
            info->RegionSize        = result.virtual_query.size;
            info->Protect           = result.virtual_query.prot;
            info->AllocationProtect = result.virtual_query.alloc_prot;
            info->State             = (DWORD)result.virtual_query.state << 12;
            info->Type              = (DWORD)result.virtual_query.alloc_type << 16;
            if (info->RegionSize != result.virtual_query.size)  /* truncated */
                return STATUS_INVALID_PARAMETER;  /* FIXME */
            if (res_len) *res_len = sizeof(*info);
        }
        return result.virtual_query.status;
    }

    base = ROUND_ADDR( addr, page_mask );

    if (is_beyond_limit( base, 1, working_set_limit )) return STATUS_INVALID_PARAMETER;

    /* Find the view containing the address */

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    ptr = views_tree.root;
    while (ptr)
    {
        view = WINE_RB_ENTRY_VALUE( ptr, struct file_view, entry );
        if ((char *)view->base > base)
        {
            alloc_end = view->base;
            ptr = ptr->left;
        }
        else if ((char *)view->base + view->size <= base)
        {
            alloc_base = (char *)view->base + view->size;
            ptr = ptr->right;
        }
        else
        {
            alloc_base = view->base;
            alloc_end = (char *)view->base + view->size;
            break;
        }
    }

    /* Fill the info structure */

    info->AllocationBase = alloc_base;
    info->BaseAddress    = base;
    info->RegionSize     = alloc_end - base;

    if (!ptr)
    {
        if (!mmap_enum_reserved_areas( get_free_mem_state_callback, info, 0 ))
        {
            /* not in a reserved area at all, pretend it's allocated */
#ifdef __i386__
            if (base >= (char *)address_space_start)
            {
                info->State             = MEM_RESERVE;
                info->Protect           = PAGE_NOACCESS;
                info->AllocationProtect = PAGE_NOACCESS;
                info->Type              = MEM_PRIVATE;
            }
            else
#endif
            {
                info->State             = MEM_FREE;
                info->Protect           = PAGE_NOACCESS;
                info->AllocationBase    = 0;
                info->AllocationProtect = 0;
                info->Type              = 0;
            }
        }
    }
    else
    {
        BYTE vprot;
        char *ptr;
        SIZE_T range_size = get_committed_size( view, base, &vprot );

        info->State = (vprot & VPROT_COMMITTED) ? MEM_COMMIT : MEM_RESERVE;
        info->Protect = (vprot & VPROT_COMMITTED) ? get_win32_prot( vprot, view->protect ) : 0;
        info->AllocationProtect = get_win32_prot( view->protect, view->protect );
        if (view->protect & SEC_IMAGE) info->Type = MEM_IMAGE;
        else if (view->protect & (SEC_FILE | SEC_RESERVE | SEC_COMMIT)) info->Type = MEM_MAPPED;
        else info->Type = MEM_PRIVATE;
        for (ptr = base; ptr < base + range_size; ptr += page_size)
            if ((get_page_vprot( ptr ) ^ vprot) & ~VPROT_WRITEWATCH) break;
        info->RegionSize = ptr - base;
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );

    if (res_len) *res_len = sizeof(*info);
    return STATUS_SUCCESS;
}

static NTSTATUS get_working_set_ex( HANDLE process, LPCVOID addr,
                                    MEMORY_WORKING_SET_EX_INFORMATION *info,
                                    SIZE_T len, SIZE_T *res_len )
{
    FILE *f;
    MEMORY_WORKING_SET_EX_INFORMATION *p;
    sigset_t sigset;

    if (process != NtCurrentProcess())
    {
        FIXME( "(process=%p,addr=%p) Unimplemented information class: MemoryWorkingSetExInformation\n", process, addr );
        return STATUS_INVALID_INFO_CLASS;
    }

    f = fopen( "/proc/self/pagemap", "rb" );
    if (!f)
    {
        static int once;
        if (!once++) WARN( "unable to open /proc/self/pagemap\n" );
    }

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    for (p = info; (UINT_PTR)(p + 1) <= (UINT_PTR)info + len; p++)
    {
        BYTE vprot;
        UINT64 pagemap;
        struct file_view *view;

        memset( &p->VirtualAttributes, 0, sizeof(p->VirtualAttributes) );

        /* If we don't have pagemap information, default to invalid. */
        if (!f || fseek( f, ((UINT_PTR)p->VirtualAddress >> 12) * sizeof(pagemap), SEEK_SET ) == -1 ||
                fread( &pagemap, sizeof(pagemap), 1, f ) != 1)
        {
            pagemap = 0;
        }

        if ((view = find_view( p->VirtualAddress, 0 )) &&
                get_committed_size( view, p->VirtualAddress, &vprot ) &&
                (vprot & VPROT_COMMITTED))
        {
            p->VirtualAttributes.Valid = !(vprot & VPROT_GUARD) && (vprot & 0x0f) && (pagemap >> 63);
            p->VirtualAttributes.Shared = !is_view_valloc( view ) && ((pagemap >> 61) & 1);
            if (p->VirtualAttributes.Shared && p->VirtualAttributes.Valid)
                p->VirtualAttributes.ShareCount = 1; /* FIXME */
            if (p->VirtualAttributes.Valid)
                p->VirtualAttributes.Win32Protection = get_win32_prot( vprot, view->protect );
        }
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );

    if (f)
        fclose( f );
    if (res_len)
        *res_len = (UINT_PTR)p - (UINT_PTR)info;
    return STATUS_SUCCESS;
}

static NTSTATUS get_memory_section_name( HANDLE process, LPCVOID addr,
                                         MEMORY_SECTION_NAME *info, SIZE_T len, SIZE_T *ret_len )
{
    NTSTATUS status;

    if (!info) return STATUS_ACCESS_VIOLATION;

    SERVER_START_REQ( get_mapping_filename )
    {
        req->process = wine_server_obj_handle( process );
        req->addr = wine_server_client_ptr( addr );
        if (len > sizeof(*info) + sizeof(WCHAR))
            wine_server_set_reply( req, info + 1, len - sizeof(*info) - sizeof(WCHAR) );
        status = wine_server_call( req );
        if (!status || status == STATUS_BUFFER_OVERFLOW)
        {
            if (ret_len) *ret_len = sizeof(*info) + reply->len + sizeof(WCHAR);
            if (len < sizeof(*info)) status = STATUS_INFO_LENGTH_MISMATCH;
            if (!status)
            {
                info->SectionFileName.Buffer = (WCHAR *)(info + 1);
                info->SectionFileName.Length = reply->len;
                info->SectionFileName.MaximumLength = reply->len + sizeof(WCHAR);
                info->SectionFileName.Buffer[reply->len / sizeof(WCHAR)] = 0;
            }
        }
    }
    SERVER_END_REQ;
    return status;
}

#define UNIMPLEMENTED_INFO_CLASS(c) \
    case c: \
        FIXME("(process=%p,addr=%p) Unimplemented information class: " #c "\n", process, addr); \
        return STATUS_INVALID_INFO_CLASS

/***********************************************************************
 *             NtQueryVirtualMemory   (NTDLL.@)
 *             ZwQueryVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryVirtualMemory( HANDLE process, LPCVOID addr,
                                      MEMORY_INFORMATION_CLASS info_class,
                                      PVOID buffer, SIZE_T len, SIZE_T *res_len )
{
    TRACE("(%p, %p, info_class=%d, %p, %ld, %p)\n",
          process, addr, info_class, buffer, len, res_len);

    switch(info_class)
    {
        case MemoryBasicInformation:
            return get_basic_memory_info( process, addr, buffer, len, res_len );
        case MemoryWorkingSetExInformation:
            return get_working_set_ex( process, addr, buffer, len, res_len );
        case MemorySectionName:
            return get_memory_section_name( process, addr, buffer, len, res_len );

        UNIMPLEMENTED_INFO_CLASS(MemoryWorkingSetList);
        UNIMPLEMENTED_INFO_CLASS(MemoryBasicVlmInformation);

        default:
            FIXME("(%p,%p,info_class=%d,%p,%ld,%p) Unknown information class\n",
                  process, addr, info_class, buffer, len, res_len);
            return STATUS_INVALID_INFO_CLASS;
    }
}


/***********************************************************************
 *             NtLockVirtualMemory   (NTDLL.@)
 *             ZwLockVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtLockVirtualMemory( HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_lock.type = APC_VIRTUAL_LOCK;
        call.virtual_lock.addr = wine_server_client_ptr( *addr );
        call.virtual_lock.size = *size;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_lock.status == STATUS_SUCCESS)
        {
            *addr = wine_server_get_ptr( result.virtual_lock.addr );
            *size = result.virtual_lock.size;
        }
        return result.virtual_lock.status;
    }

    *size = ROUND_SIZE( *addr, *size );
    *addr = ROUND_ADDR( *addr, page_mask );

    if (mlock( *addr, *size )) status = STATUS_ACCESS_DENIED;
    return status;
}


/***********************************************************************
 *             NtUnlockVirtualMemory   (NTDLL.@)
 *             ZwUnlockVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtUnlockVirtualMemory( HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_unlock.type = APC_VIRTUAL_UNLOCK;
        call.virtual_unlock.addr = wine_server_client_ptr( *addr );
        call.virtual_unlock.size = *size;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_unlock.status == STATUS_SUCCESS)
        {
            *addr = wine_server_get_ptr( result.virtual_unlock.addr );
            *size = result.virtual_unlock.size;
        }
        return result.virtual_unlock.status;
    }

    *size = ROUND_SIZE( *addr, *size );
    *addr = ROUND_ADDR( *addr, page_mask );

    if (munlock( *addr, *size )) status = STATUS_ACCESS_DENIED;
    return status;
}


/***********************************************************************
 *             NtMapViewOfSection   (NTDLL.@)
 *             ZwMapViewOfSection   (NTDLL.@)
 */
NTSTATUS WINAPI NtMapViewOfSection( HANDLE handle, HANDLE process, PVOID *addr_ptr, ULONG_PTR zero_bits,
                                    SIZE_T commit_size, const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                    SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect )
{
    NTSTATUS res;
    SIZE_T mask = granularity_mask;
    LARGE_INTEGER offset;

    offset.QuadPart = offset_ptr ? offset_ptr->QuadPart : 0;

    TRACE("handle=%p process=%p addr=%p off=%x%08x size=%lx access=%x\n",
          handle, process, *addr_ptr, offset.u.HighPart, offset.u.LowPart, *size_ptr, protect );

    /* Check parameters */
    if (zero_bits > 21 && zero_bits < 32)
        return STATUS_INVALID_PARAMETER_4;

    /* If both addr_ptr and zero_bits are passed, they have match */
    if (*addr_ptr && zero_bits && zero_bits < 32 &&
        (((UINT_PTR)*addr_ptr) >> (32 - zero_bits)))
        return STATUS_INVALID_PARAMETER_4;
    if (*addr_ptr && zero_bits >= 32 &&
        (((UINT_PTR)*addr_ptr) & ~zero_bits))
        return STATUS_INVALID_PARAMETER_4;

#ifndef _WIN64
    if (!is_wow64)
    {
        if (zero_bits >= 32) return STATUS_INVALID_PARAMETER_4;
        if (alloc_type & AT_ROUND_TO_PAGE)
        {
            *addr_ptr = ROUND_ADDR( *addr_ptr, page_mask );
            mask = page_mask;
        }
    }
#endif

    if ((offset.u.LowPart & mask) || (*addr_ptr && ((UINT_PTR)*addr_ptr & mask)))
        return STATUS_MAPPED_ALIGNMENT;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.map_view.type         = APC_MAP_VIEW;
        call.map_view.handle       = wine_server_obj_handle( handle );
        call.map_view.addr         = wine_server_client_ptr( *addr_ptr );
        call.map_view.size         = *size_ptr;
        call.map_view.offset       = offset.QuadPart;
        call.map_view.zero_bits    = zero_bits;
        call.map_view.alloc_type   = alloc_type;
        call.map_view.prot         = protect;
        res = server_queue_process_apc( process, &call, &result );
        if (res != STATUS_SUCCESS) return res;

        if ((NTSTATUS)result.map_view.status >= 0)
        {
            *addr_ptr = wine_server_get_ptr( result.map_view.addr );
            *size_ptr = result.map_view.size;
        }
        return result.map_view.status;
    }

    return virtual_map_section( handle, addr_ptr, zero_bits, commit_size,
                                offset_ptr, size_ptr, alloc_type, protect );
}


/***********************************************************************
 *             NtUnmapViewOfSection   (NTDLL.@)
 *             ZwUnmapViewOfSection   (NTDLL.@)
 */
NTSTATUS WINAPI NtUnmapViewOfSection( HANDLE process, PVOID addr )
{
    struct file_view *view;
    NTSTATUS status = STATUS_NOT_MAPPED_VIEW;
    sigset_t sigset;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.unmap_view.type = APC_UNMAP_VIEW;
        call.unmap_view.addr = wine_server_client_ptr( addr );
        status = server_queue_process_apc( process, &call, &result );
        if (status == STATUS_SUCCESS) status = result.unmap_view.status;
        return status;
    }

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if ((view = find_view( addr, 0 )) && !is_view_valloc( view ))
    {
        if (view->protect & VPROT_SYSTEM)
        {
            struct builtin_module *builtin;

            LIST_FOR_EACH_ENTRY( builtin, &builtin_modules, struct builtin_module, entry )
            {
                if (builtin->module != view->base) continue;
                if (builtin->refcount > 1)
                {
                    TRACE( "not freeing in-use builtin %p\n", view->base );
                    builtin->refcount--;
                    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
                    return STATUS_SUCCESS;
                }
            }
        }

        SERVER_START_REQ( unmap_view )
        {
            req->base = wine_server_client_ptr( view->base );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        if (!status)
        {
            if (view->protect & SEC_IMAGE) release_builtin_module( view->base );
            delete_view( view );
        }
        else FIXME( "failed to unmap %p %x\n", view->base, status );
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/******************************************************************************
 *             virtual_fill_image_information
 *
 * Helper for NtQuerySection.
 */
void virtual_fill_image_information( const pe_image_info_t *pe_info, SECTION_IMAGE_INFORMATION *info )
{
    info->TransferAddress             = wine_server_get_ptr( pe_info->base + pe_info->entry_point );
    info->ZeroBits                    = pe_info->zerobits;
    info->MaximumStackSize            = pe_info->stack_size;
    info->CommittedStackSize          = pe_info->stack_commit;
    info->SubSystemType               = pe_info->subsystem;
    info->MinorSubsystemVersion       = pe_info->subsystem_minor;
    info->MajorSubsystemVersion       = pe_info->subsystem_major;
    info->MajorOperatingSystemVersion = pe_info->osversion_major;
    info->MinorOperatingSystemVersion = pe_info->osversion_minor;
    info->ImageCharacteristics        = pe_info->image_charact;
    info->DllCharacteristics          = pe_info->dll_charact;
    info->Machine                     = pe_info->machine;
    info->ImageContainsCode           = pe_info->contains_code;
    info->ImageFlags                  = pe_info->image_flags;
    info->LoaderFlags                 = pe_info->loader_flags;
    info->ImageFileSize               = pe_info->file_size;
    info->CheckSum                    = pe_info->checksum;
#ifndef _WIN64 /* don't return 64-bit values to 32-bit processes */
    if (is_machine_64bit( pe_info->machine ))
    {
        info->TransferAddress = (void *)0x81231234;  /* sic */
        info->MaximumStackSize = 0x100000;
        info->CommittedStackSize = 0x10000;
    }
#endif
}

/******************************************************************************
 *             NtQuerySection   (NTDLL.@)
 *             ZwQuerySection   (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySection( HANDLE handle, SECTION_INFORMATION_CLASS class, void *ptr,
                                SIZE_T size, SIZE_T *ret_size )
{
    NTSTATUS status;
    pe_image_info_t image_info;

    switch (class)
    {
    case SectionBasicInformation:
        if (size < sizeof(SECTION_BASIC_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;
        break;
    case SectionImageInformation:
        if (size < sizeof(SECTION_IMAGE_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;
        break;
    default:
	FIXME( "class %u not implemented\n", class );
	return STATUS_NOT_IMPLEMENTED;
    }
    if (!ptr) return STATUS_ACCESS_VIOLATION;

    SERVER_START_REQ( get_mapping_info )
    {
        req->handle = wine_server_obj_handle( handle );
        req->access = SECTION_QUERY;
        wine_server_set_reply( req, &image_info, sizeof(image_info) );
        if (!(status = wine_server_call( req )))
        {
            if (class == SectionBasicInformation)
            {
                SECTION_BASIC_INFORMATION *info = ptr;
                info->Attributes    = reply->flags;
                info->BaseAddress   = NULL;
                info->Size.QuadPart = reply->size;
                if (ret_size) *ret_size = sizeof(*info);
            }
            else if (reply->flags & SEC_IMAGE)
            {
                SECTION_IMAGE_INFORMATION *info = ptr;
                virtual_fill_image_information( &image_info, info );
                if (ret_size) *ret_size = sizeof(*info);
            }
            else status = STATUS_SECTION_NOT_IMAGE;
        }
    }
    SERVER_END_REQ;

    return status;
}


/***********************************************************************
 *             NtFlushVirtualMemory   (NTDLL.@)
 *             ZwFlushVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushVirtualMemory( HANDLE process, LPCVOID *addr_ptr,
                                      SIZE_T *size_ptr, ULONG unknown )
{
    struct file_view *view;
    NTSTATUS status = STATUS_SUCCESS;
    sigset_t sigset;
    void *addr = ROUND_ADDR( *addr_ptr, page_mask );

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_flush.type = APC_VIRTUAL_FLUSH;
        call.virtual_flush.addr = wine_server_client_ptr( addr );
        call.virtual_flush.size = *size_ptr;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_flush.status == STATUS_SUCCESS)
        {
            *addr_ptr = wine_server_get_ptr( result.virtual_flush.addr );
            *size_ptr = result.virtual_flush.size;
        }
        return result.virtual_flush.status;
    }

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );
    if (!(view = find_view( addr, *size_ptr ))) status = STATUS_INVALID_PARAMETER;
    else
    {
        if (!*size_ptr) *size_ptr = view->size;
        *addr_ptr = addr;
#ifdef MS_ASYNC
        if (msync( addr, *size_ptr, MS_ASYNC )) status = STATUS_NOT_MAPPED_DATA;
#endif
    }
    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/***********************************************************************
 *             NtGetWriteWatch   (NTDLL.@)
 *             ZwGetWriteWatch   (NTDLL.@)
 */
NTSTATUS WINAPI NtGetWriteWatch( HANDLE process, ULONG flags, PVOID base, SIZE_T size, PVOID *addresses,
                                 ULONG_PTR *count, ULONG *granularity )
{
    NTSTATUS status = STATUS_SUCCESS;
    sigset_t sigset;

    size = ROUND_SIZE( base, size );
    base = ROUND_ADDR( base, page_mask );

    if (!count || !granularity) return STATUS_ACCESS_VIOLATION;
    if (!*count || !size) return STATUS_INVALID_PARAMETER;
    if (flags & ~WRITE_WATCH_FLAG_RESET) return STATUS_INVALID_PARAMETER;

    if (!addresses) return STATUS_ACCESS_VIOLATION;

    TRACE( "%p %x %p-%p %p %lu\n", process, flags, base, (char *)base + size,
           addresses, *count );

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    if (is_write_watch_range( base, size ))
    {
        ULONG_PTR pos = 0;
        char *addr = base;
        char *end = addr + size;

        while (pos < *count && addr < end)
        {
            if (!(get_page_vprot( addr ) & VPROT_WRITEWATCH)) addresses[pos++] = addr;
            addr += page_size;
        }
        if (flags & WRITE_WATCH_FLAG_RESET) reset_write_watches( base, addr - (char *)base );
        *count = pos;
        *granularity = page_size;
    }
    else status = STATUS_INVALID_PARAMETER;

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/***********************************************************************
 *             NtResetWriteWatch   (NTDLL.@)
 *             ZwResetWriteWatch   (NTDLL.@)
 */
NTSTATUS WINAPI NtResetWriteWatch( HANDLE process, PVOID base, SIZE_T size )
{
    NTSTATUS status = STATUS_SUCCESS;
    sigset_t sigset;

    size = ROUND_SIZE( base, size );
    base = ROUND_ADDR( base, page_mask );

    TRACE( "%p %p-%p\n", process, base, (char *)base + size );

    if (!size) return STATUS_INVALID_PARAMETER;

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    if (is_write_watch_range( base, size ))
        reset_write_watches( base, size );
    else
        status = STATUS_INVALID_PARAMETER;

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/***********************************************************************
 *             NtReadVirtualMemory   (NTDLL.@)
 *             ZwReadVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtReadVirtualMemory( HANDLE process, const void *addr, void *buffer,
                                     SIZE_T size, SIZE_T *bytes_read )
{
    NTSTATUS status;

    if (virtual_check_buffer_for_write( buffer, size ))
    {
        SERVER_START_REQ( read_process_memory )
        {
            req->handle = wine_server_obj_handle( process );
            req->addr   = wine_server_client_ptr( addr );
            wine_server_set_reply( req, buffer, size );
            if ((status = wine_server_call( req ))) size = 0;
        }
        SERVER_END_REQ;
    }
    else
    {
        status = STATUS_ACCESS_VIOLATION;
        size = 0;
    }
    if (bytes_read) *bytes_read = size;
    return status;
}


/***********************************************************************
 *             NtWriteVirtualMemory   (NTDLL.@)
 *             ZwWriteVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtWriteVirtualMemory( HANDLE process, void *addr, const void *buffer,
                                      SIZE_T size, SIZE_T *bytes_written )
{
    NTSTATUS status;

    if (virtual_check_buffer_for_read( buffer, size ))
    {
        SERVER_START_REQ( write_process_memory )
        {
            req->handle     = wine_server_obj_handle( process );
            req->addr       = wine_server_client_ptr( addr );
            wine_server_add_data( req, buffer, size );
            if ((status = wine_server_call( req ))) size = 0;
        }
        SERVER_END_REQ;
    }
    else
    {
        status = STATUS_PARTIAL_COPY;
        size = 0;
    }
    if (bytes_written) *bytes_written = size;
    return status;
}


/***********************************************************************
 *             NtAreMappedFilesTheSame   (NTDLL.@)
 *             ZwAreMappedFilesTheSame   (NTDLL.@)
 */
NTSTATUS WINAPI NtAreMappedFilesTheSame(PVOID addr1, PVOID addr2)
{
    struct file_view *view1, *view2;
    NTSTATUS status;
    sigset_t sigset;

    TRACE("%p %p\n", addr1, addr2);

    server_enter_uninterrupted_section( &virtual_mutex, &sigset );

    view1 = find_view( addr1, 0 );
    view2 = find_view( addr2, 0 );

    if (!view1 || !view2)
        status = STATUS_INVALID_ADDRESS;
    else if (is_view_valloc( view1 ) || is_view_valloc( view2 ))
        status = STATUS_CONFLICTING_ADDRESSES;
    else if (view1 == view2)
        status = STATUS_SUCCESS;
    else if ((view1->protect & VPROT_SYSTEM) || (view2->protect & VPROT_SYSTEM))
        status = STATUS_NOT_SAME_DEVICE;
    else
    {
        SERVER_START_REQ( is_same_mapping )
        {
            req->base1 = wine_server_client_ptr( view1->base );
            req->base2 = wine_server_client_ptr( view2->base );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    server_leave_uninterrupted_section( &virtual_mutex, &sigset );
    return status;
}


/**********************************************************************
 *           NtFlushInstructionCache  (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushInstructionCache( HANDLE handle, const void *addr, SIZE_T size )
{
#if defined(__x86_64__) || defined(__i386__)
    /* no-op */
#elif defined(HAVE___CLEAR_CACHE)
    if (handle == GetCurrentProcess())
    {
        __clear_cache( (char *)addr, (char *)addr + size );
    }
    else
    {
        static int once;
        if (!once++) FIXME( "%p %p %ld other process not supported\n", handle, addr, size );
    }
#else
    static int once;
    if (!once++) FIXME( "%p %p %ld\n", handle, addr, size );
#endif
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           NtFlushProcessWriteBuffers  (NTDLL.@)
 */
void WINAPI NtFlushProcessWriteBuffers(void)
{
    static int once = 0;
    if (!once++) FIXME( "stub\n" );
}


/**********************************************************************
 *           NtCreatePagingFile  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreatePagingFile( UNICODE_STRING *name, LARGE_INTEGER *min_size,
                                    LARGE_INTEGER *max_size, LARGE_INTEGER *actual_size )
{
    FIXME( "(%s %p %p %p) stub\n", debugstr_us(name), min_size, max_size, actual_size );
    return STATUS_SUCCESS;
}

#ifndef _WIN64

/***********************************************************************
 *             NtWow64AllocateVirtualMemory64   (NTDLL.@)
 *             ZwWow64AllocateVirtualMemory64   (NTDLL.@)
 */
NTSTATUS WINAPI NtWow64AllocateVirtualMemory64( HANDLE process, ULONG64 *ret, ULONG64 zero_bits,
                                                ULONG64 *size_ptr, ULONG type, ULONG protect )
{
    void *base;
    SIZE_T size;
    NTSTATUS status;

    TRACE("%p %s %s %x %08x\n", process,
          wine_dbgstr_longlong(*ret), wine_dbgstr_longlong(*size_ptr), type, protect );

    if (!*size_ptr) return STATUS_INVALID_PARAMETER_4;
    if (zero_bits > 21 && zero_bits < 32) return STATUS_INVALID_PARAMETER_3;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_alloc.type         = APC_VIRTUAL_ALLOC;
        call.virtual_alloc.addr         = *ret;
        call.virtual_alloc.size         = *size_ptr;
        call.virtual_alloc.zero_bits    = zero_bits;
        call.virtual_alloc.op_type      = type;
        call.virtual_alloc.prot         = protect;
        status = server_queue_process_apc( process, &call, &result );
        if (status != STATUS_SUCCESS) return status;

        if (result.virtual_alloc.status == STATUS_SUCCESS)
        {
            *ret      = result.virtual_alloc.addr;
            *size_ptr = result.virtual_alloc.size;
        }
        return result.virtual_alloc.status;
    }

    base = (void *)(ULONG_PTR)*ret;
    size = *size_ptr;
    if ((ULONG_PTR)base != *ret) return STATUS_CONFLICTING_ADDRESSES;
    if (size != *size_ptr) return STATUS_WORKING_SET_LIMIT_RANGE;

    status = NtAllocateVirtualMemory( process, &base, zero_bits, &size, type, protect );
    if (!status)
    {
        *ret = (ULONG_PTR)base;
        *size_ptr = size;
    }
    return status;
}


/***********************************************************************
 *             NtWow64ReadVirtualMemory64   (NTDLL.@)
 *             ZwWow64ReadVirtualMemory64   (NTDLL.@)
 */
NTSTATUS WINAPI NtWow64ReadVirtualMemory64( HANDLE process, ULONG64 addr, void *buffer,
                                            ULONG64 size, ULONG64 *bytes_read )
{
    NTSTATUS status;

    if (size > MAXLONG) size = MAXLONG;

    if (virtual_check_buffer_for_write( buffer, size ))
    {
        SERVER_START_REQ( read_process_memory )
        {
            req->handle = wine_server_obj_handle( process );
            req->addr   = addr;
            wine_server_set_reply( req, buffer, size );
            if ((status = wine_server_call( req ))) size = 0;
        }
        SERVER_END_REQ;
    }
    else
    {
        status = STATUS_ACCESS_VIOLATION;
        size = 0;
    }
    if (bytes_read) *bytes_read = size;
    return status;
}


/***********************************************************************
 *             NtWow64WriteVirtualMemory64   (NTDLL.@)
 *             ZwWow64WriteVirtualMemory64   (NTDLL.@)
 */
NTSTATUS WINAPI NtWow64WriteVirtualMemory64( HANDLE process, ULONG64 addr, const void *buffer,
                                             ULONG64 size, ULONG64 *bytes_written )
{
    NTSTATUS status;

    if (size > MAXLONG) size = MAXLONG;

    if (virtual_check_buffer_for_read( buffer, size ))
    {
        SERVER_START_REQ( write_process_memory )
        {
            req->handle     = wine_server_obj_handle( process );
            req->addr       = addr;
            wine_server_add_data( req, buffer, size );
            if ((status = wine_server_call( req ))) size = 0;
        }
        SERVER_END_REQ;
    }
    else
    {
        status = STATUS_PARTIAL_COPY;
        size = 0;
    }
    if (bytes_written) *bytes_written = size;
    return status;
}


/***********************************************************************
 *             NtWow64GetNativeSystemInformation   (NTDLL.@)
 *             ZwWow64GetNativeSystemInformation   (NTDLL.@)
 */
NTSTATUS WINAPI NtWow64GetNativeSystemInformation( SYSTEM_INFORMATION_CLASS class, void *info,
                                                   ULONG len, ULONG *retlen )
{
    switch (class)
    {
    case SystemBasicInformation:
    case SystemCpuInformation:
    case SystemEmulationBasicInformation:
    case SystemEmulationProcessorInformation:
        return NtQuerySystemInformation( class, info, len, retlen );
    case SystemNativeBasicInformation:
        return NtQuerySystemInformation( SystemBasicInformation, info, len, retlen );
    default:
        if (is_wow64) return STATUS_INVALID_INFO_CLASS;
        return NtQuerySystemInformation( class, info, len, retlen );
    }
}

#endif  /* _WIN64 */
