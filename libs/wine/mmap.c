/*
 * Wine memory mappings support
 *
 * Copyright 2000, 2004 Alexandre Julliard
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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#include "wine/list.h"
#include "wine/asm.h"

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0
#endif
#ifndef MAP_ANON
#define MAP_ANON 0
#endif

static inline int get_fdzero(void)
{
    static int fd = -1;

    if (MAP_ANON == 0 && fd == -1)
    {
        if ((fd = open( "/dev/zero", O_RDONLY )) == -1)
        {
            perror( "/dev/zero: open" );
            exit(1);
        }
    }
    return fd;
}

#if (defined(__svr4__) || defined(__NetBSD__)) && !defined(MAP_TRYFIXED)
/***********************************************************************
 *             try_mmap_fixed
 *
 * The purpose of this routine is to emulate the behaviour of
 * the Linux mmap() routine if a non-NULL address is passed,
 * but the MAP_FIXED flag is not set.  Linux in this case tries
 * to place the mapping at the specified address, *unless* the
 * range is already in use.  Solaris, however, completely ignores
 * the address argument in this case.
 *
 * As Wine code occasionally relies on the Linux behaviour, e.g. to
 * be able to map non-relocatable PE executables to their proper
 * start addresses, or to map the DOS memory to 0, this routine
 * emulates the Linux behaviour by checking whether the desired
 * address range is still available, and placing the mapping there
 * using MAP_FIXED if so.
 */
static int try_mmap_fixed (void *addr, size_t len, int prot, int flags,
                           int fildes, off_t off)
{
    char * volatile result = NULL;
    const size_t pagesize = sysconf( _SC_PAGESIZE );
    pid_t pid, wret;

    /* We only try to map to a fixed address if
       addr is non-NULL and properly aligned,
       and MAP_FIXED isn't already specified. */

    if ( !addr )
        return 0;
    if ( (uintptr_t)addr & (pagesize-1) )
        return 0;
    if ( flags & MAP_FIXED )
        return 0;

    /* We use vfork() to freeze all threads of the
       current process.  This allows us to check without
       race condition whether the desired memory range is
       already in use.  Note that because vfork() shares
       the address spaces between parent and child, we
       can actually perform the mapping in the child. */

    if ( (pid = vfork()) == -1 )
    {
        perror("try_mmap_fixed: vfork");
        exit(1);
    }
    if ( pid == 0 )
    {
        int i;
        char vec;

        /* We call mincore() for every page in the desired range.
           If any of these calls succeeds, the page is already
           mapped and we must fail. */
        for ( i = 0; i < len; i += pagesize )
            if ( mincore( (caddr_t)addr + i, pagesize, &vec ) != -1 )
               _exit(1);

        /* Perform the mapping with MAP_FIXED set.  This is safe
           now, as none of the pages is currently in use. */
        result = mmap( addr, len, prot, flags | MAP_FIXED, fildes, off );
        if ( result == addr )
            _exit(0);

        if ( result != (void *) -1 ) /* This should never happen ... */
            munmap( result, len );

       _exit(1);
    }

    /* reap child */
    do {
        wret = waitpid(pid, NULL, 0);
    } while (wret < 0 && errno == EINTR);

    return result == addr;
}

#elif defined(__APPLE__)

#include <mach/mach_init.h>
#include <mach/mach_vm.h>

/*
 * On Darwin, we can use the Mach call mach_vm_map to allocate
 * anonymous memory at the specified address and then, if necessary, use
 * mmap with MAP_FIXED to replace the mapping.
 */
static int try_mmap_fixed (void *addr, size_t len, int prot, int flags,
                           int fildes, off_t off)
{
    mach_vm_address_t result = (mach_vm_address_t)addr;
    int vm_flags = VM_FLAGS_FIXED;

    if (flags & MAP_NOCACHE)
        vm_flags |= VM_FLAGS_NO_CACHE;
    if (!mach_vm_map( mach_task_self(), &result, len, 0, vm_flags, MEMORY_OBJECT_NULL,
                      0, 0, prot, VM_PROT_ALL, VM_INHERIT_COPY ))
    {
        flags |= MAP_FIXED;
        if (((flags & ~(MAP_NORESERVE | MAP_NOCACHE)) == (MAP_ANON | MAP_FIXED | MAP_PRIVATE)) ||
            mmap( (void *)result, len, prot, flags, fildes, off ) != MAP_FAILED)
            return 1;
        mach_vm_deallocate(mach_task_self(),result,len);
    }
    return 0;
}

#endif  /* (__svr4__ || __NetBSD__) && !MAP_TRYFIXED */


/***********************************************************************
 *		wine_anon_mmap
 *
 * Portable wrapper for anonymous mmaps
 */
void *wine_anon_mmap( void *start, size_t size, int prot, int flags )
{
#ifdef MAP_SHARED
    flags &= ~MAP_SHARED;
#endif

    /* Linux EINVAL's on us if we don't pass MAP_PRIVATE to an anon mmap */
    flags |= MAP_PRIVATE | MAP_ANON;

    if (!(flags & MAP_FIXED))
    {
#ifdef MAP_TRYFIXED
        /* If available, this will attempt a fixed mapping in-kernel */
        flags |= MAP_TRYFIXED;
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
        if ( start && mmap( start, size, prot, flags | MAP_FIXED | MAP_EXCL, get_fdzero(), 0 ) != MAP_FAILED )
            return start;
#elif defined(__svr4__) || defined(__NetBSD__) || defined(__APPLE__)
        if ( try_mmap_fixed( start, size, prot, flags, get_fdzero(), 0 ) )
            return start;
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
        /* Even FreeBSD 5.3 does not properly support NULL here. */
        if( start == NULL ) start = (void *)0x110000;
#endif
    }
    return mmap( start, size, prot, flags, get_fdzero(), 0 );
}

#ifdef __ASM_OBSOLETE

struct reserved_area
{
    struct list entry;
    void       *base;
    size_t      size;
};

static struct list reserved_areas = LIST_INIT(reserved_areas);
#ifndef __APPLE__
static const unsigned int granularity_mask = 0xffff;  /* reserved areas have 64k granularity */
#endif

void wine_mmap_add_reserved_area_obsolete( void *addr, size_t size );

#ifdef __APPLE__

/***********************************************************************
 *           reserve_area
 *
 * Reserve as much memory as possible in the given area.
 */
static inline void reserve_area( void *addr, void *end )
{
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
            if (!ret)
                wine_mmap_add_reserved_area_obsolete( (void*)hole_address, hole_size );
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
}

#else

/***********************************************************************
 *		mmap_reserve
 *
 * mmap wrapper used for reservations, only maps the specified address
 */
static inline int mmap_reserve( void *addr, size_t size )
{
    void *ptr;
    int flags = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;

#ifdef MAP_TRYFIXED
    flags |= MAP_TRYFIXED;
#elif defined(__APPLE__)
    return try_mmap_fixed( addr, size, PROT_NONE, flags, get_fdzero(), 0 );
#endif
    ptr = mmap( addr, size, PROT_NONE, flags, get_fdzero(), 0 );
    if (ptr != addr && ptr != (void *)-1)  munmap( ptr, size );
    return (ptr == addr);
}


/***********************************************************************
 *           reserve_area
 *
 * Reserve as much memory as possible in the given area.
 */
static inline void reserve_area( void *addr, void *end )
{
    size_t size = (char *)end - (char *)addr;

#if (defined(__svr4__) || defined(__NetBSD__)) && !defined(MAP_TRYFIXED)
    /* try_mmap_fixed is inefficient when using vfork, so we need a different algorithm here */
    /* we assume no other thread is running at this point */
    size_t i, pagesize = sysconf( _SC_PAGESIZE );
    char vec;

    while (size)
    {
        for (i = 0; i < size; i += pagesize)
            if (mincore( (caddr_t)addr + i, pagesize, &vec ) != -1) break;

        i &= ~granularity_mask;
        if (i && mmap( addr, i, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON | MAP_NORESERVE,
                       get_fdzero(), 0 ) != (void *)-1)
            wine_mmap_add_reserved_area_obsolete( addr, i );

        i += granularity_mask + 1;
        if ((char *)addr + i < (char *)addr) break;  /* overflow */
        addr = (char *)addr + i;
        if (addr >= end) break;
        size = (char *)end - (char *)addr;
    }
#else
    if (!size) return;

    if (mmap_reserve( addr, size ))
    {
        wine_mmap_add_reserved_area_obsolete( addr, size );
        return;
    }
    size = (size / 2) & ~granularity_mask;
    if (size)
    {
        reserve_area( addr, (char *)addr + size );
        reserve_area( (char *)addr + size, end );
    }
#endif
}

#endif /* __APPLE__ */

#ifdef __i386__
/***********************************************************************
 *           reserve_malloc_space
 *
 * Solaris malloc is not smart enough to obtain space through mmap(), so try to make
 * sure that there is some available sbrk() space before we reserve other things.
 */
static inline void reserve_malloc_space( size_t size )
{
#ifdef __sun
    size_t i, count = size / 1024;
    void **ptrs = malloc( count * sizeof(ptrs[0]) );

    if (!ptrs) return;

    for (i = 0; i < count; i++) if (!(ptrs[i] = malloc( 1024 ))) break;
    if (i--)  /* free everything except the last one */
        while (i) free( ptrs[--i] );
    free( ptrs );
#endif
}


/***********************************************************************
 *           reserve_dos_area
 *
 * Reserve the DOS area (0x00000000-0x00110000).
 */
static inline void reserve_dos_area(void)
{
    const size_t first_page = 0x1000;
    const size_t dos_area_size = 0x110000;
    void *ptr;

    /* first page has to be handled specially */
    ptr = wine_anon_mmap( (void *)first_page, dos_area_size - first_page, PROT_NONE, MAP_NORESERVE );
    if (ptr != (void *)first_page)
    {
        if (ptr != (void *)-1) munmap( ptr, dos_area_size - first_page );
        return;
    }
    /* now add first page with MAP_FIXED */
    wine_anon_mmap( NULL, first_page, PROT_NONE, MAP_NORESERVE|MAP_FIXED );
    wine_mmap_add_reserved_area_obsolete( NULL, dos_area_size );
}
#endif


/***********************************************************************
 *           mmap_init
 */
void mmap_init(void)
{
#ifdef __i386__
    struct reserved_area *area;
    struct list *ptr;
#ifndef __APPLE__
    char stack;
    char * const stack_ptr = &stack;
#endif
    char *user_space_limit = (char *)0x7ffe0000;

    reserve_malloc_space( 8 * 1024 * 1024 );

    if (!list_head( &reserved_areas ))
    {
        /* if we don't have a preloader, try to reserve some space below 2Gb */
        reserve_area( (void *)0x00110000, (void *)0x40000000 );
    }

    /* check for a reserved area starting at the user space limit */
    /* to avoid wasting time trying to allocate it again */
    LIST_FOR_EACH( ptr, &reserved_areas )
    {
        area = LIST_ENTRY( ptr, struct reserved_area, entry );
        if ((char *)area->base > user_space_limit) break;
        if ((char *)area->base + area->size > user_space_limit)
        {
            user_space_limit = (char *)area->base + area->size;
            break;
        }
    }

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

    /* reserve the DOS area if not already done */

    ptr = list_head( &reserved_areas );
    if (ptr)
    {
        area = LIST_ENTRY( ptr, struct reserved_area, entry );
        if (!area->base) return;  /* already reserved */
    }
    reserve_dos_area();

#elif defined(__x86_64__) || defined(__aarch64__)

    if (!list_head( &reserved_areas ))
    {
        /* if we don't have a preloader, try to reserve the space now */
        reserve_area( (void *)0x000000010000, (void *)0x000068000000 );
        reserve_area( (void *)0x00007ff00000, (void *)0x00007fff0000 );
        reserve_area( (void *)0x7ffffe000000, (void *)0x7fffffff0000 );
    }

#endif
}


/***********************************************************************
 *           wine_mmap_add_reserved_area
 *
 * Add an address range to the list of reserved areas.
 * Caller must have made sure the range is not used by anything else.
 *
 * Note: the reserved areas functions are not reentrant, caller is
 * responsible for proper locking.
 */
void wine_mmap_add_reserved_area_obsolete( void *addr, size_t size )
{
    struct reserved_area *area;
    struct list *ptr;

    if (!((intptr_t)addr + size)) size--;  /* avoid wrap-around */

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


/***********************************************************************
 *           wine_mmap_remove_reserved_area
 *
 * Remove an address range from the list of reserved areas.
 * If 'unmap' is non-zero the range is unmapped too.
 *
 * Note: the reserved areas functions are not reentrant, caller is
 * responsible for proper locking.
 */
void wine_mmap_remove_reserved_area_obsolete( void *addr, size_t size, int unmap )
{
    struct reserved_area *area;
    struct list *ptr;

    if (!((intptr_t)addr + size)) size--;  /* avoid wrap-around */

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
                    if (unmap) munmap( area->base, (char *)addr + size - (char *)area->base );
                    area->size -= (char *)addr + size - (char *)area->base;
                    area->base = (char *)addr + size;
                    break;
                }
                else
                {
                    /* range contains the whole area -> remove area completely */
                    ptr = list_next( &reserved_areas, ptr );
                    if (unmap) munmap( area->base, area->size );
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
                    if (unmap) munmap( addr, size );
                    break;
                }
                else
                {
                    /* range overlaps end of area only -> shrink area */
                    if (unmap) munmap( addr, (char *)area->base + area->size - (char *)addr );
                    area->size = (char *)addr - (char *)area->base;
                }
            }
        }
        ptr = list_next( &reserved_areas, ptr );
    }
}


/***********************************************************************
 *           wine_mmap_is_in_reserved_area
 *
 * Check if the specified range is included in a reserved area.
 * Returns 1 if range is fully included, 0 if range is not included
 * at all, and -1 if it is only partially included.
 *
 * Note: the reserved areas functions are not reentrant, caller is
 * responsible for proper locking.
 */
int wine_mmap_is_in_reserved_area_obsolete( void *addr, size_t size )
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


/***********************************************************************
 *           wine_mmap_enum_reserved_areas
 *
 * Enumerate the list of reserved areas, sorted by addresses.
 * If enum_func returns a non-zero value, enumeration is stopped and the value is returned.
 *
 * Note: the reserved areas functions are not reentrant, caller is
 * responsible for proper locking.
 */
int wine_mmap_enum_reserved_areas_obsolete( int (*enum_func)(void *base, size_t size, void *arg), void *arg,
                                            int top_down )
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

__ASM_OBSOLETE(wine_mmap_add_reserved_area);
__ASM_OBSOLETE(wine_mmap_remove_reserved_area);
__ASM_OBSOLETE(wine_mmap_is_in_reserved_area);
__ASM_OBSOLETE(wine_mmap_enum_reserved_areas);

#endif /* __ASM_OBSOLETE */
