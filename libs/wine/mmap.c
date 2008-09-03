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
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#include "wine/library.h"
#include "wine/list.h"

struct reserved_area
{
    struct list entry;
    void       *base;
    size_t      size;
};

static struct list reserved_areas = LIST_INIT(reserved_areas);
static const unsigned int granularity_mask = 0xffff;  /* reserved areas have 64k granularity */

#ifdef HAVE_MMAP

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
    int pagesize = getpagesize();
    pid_t pid;

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

    /* vfork() lets the parent continue only after the child
       has exited.  Furthermore, Wine sets SIGCHLD to SIG_IGN,
       so we don't need to wait for the child. */

    return result == addr;
}

#elif defined(__APPLE__)

#include <mach/mach_init.h>
#include <mach/vm_map.h>

/*
 * On Darwin, we can use the Mach call vm_allocate to allocate
 * anonymous memory at the specified address, and then use mmap with
 * MAP_FIXED to replace the mapping.
 */
static int try_mmap_fixed (void *addr, size_t len, int prot, int flags,
                           int fildes, off_t off)
{
    vm_address_t result = (vm_address_t)addr;

    if (!vm_allocate(mach_task_self(),&result,len,0))
    {
        if (mmap( (void *)result, len, prot, flags | MAP_FIXED, fildes, off ) != MAP_FAILED)
            return 1;
        vm_deallocate(mach_task_self(),result,len);
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
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
        /* Even FreeBSD 5.3 does not properly support NULL here. */
        if( start == NULL ) start = (void *)0x110000;
#endif

#ifdef MAP_TRYFIXED
        /* If available, this will attempt a fixed mapping in-kernel */
        flags |= MAP_TRYFIXED;
#elif defined(__svr4__) || defined(__NetBSD__) || defined(__APPLE__)
        if ( try_mmap_fixed( start, size, prot, flags, get_fdzero(), 0 ) )
            return start;
#endif
    }
    return mmap( start, size, prot, flags, get_fdzero(), 0 );
}


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
#if defined(__i386__) && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)  /* commented out until FreeBSD gets fixed */
static void reserve_area( void *addr, void *end )
{
    size_t size = (char *)end - (char *)addr;

#if (defined(__svr4__) || defined(__NetBSD__)) && !defined(MAP_TRYFIXED)
    /* try_mmap_fixed is inefficient when using vfork, so we need a different algorithm here */
    /* we assume no other thread is running at this point */
    size_t i, pagesize = getpagesize();
    char vec;

    while (size)
    {
        for (i = 0; i < size; i += pagesize)
            if (mincore( (caddr_t)addr + i, pagesize, &vec ) != -1) break;

        i &= ~granularity_mask;
        if (i && mmap( addr, i, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON | MAP_NORESERVE,
                       get_fdzero(), 0 ) != (void *)-1)
            wine_mmap_add_reserved_area( addr, i );

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
        wine_mmap_add_reserved_area( addr, size );
        return;
    }
    if (size > granularity_mask + 1)
    {
        size_t new_size = (size / 2) & ~granularity_mask;
        reserve_area( addr, (char *)addr + new_size );
        reserve_area( (char *)addr + new_size, end );
    }
#endif
}


/***********************************************************************
 *           reserve_malloc_space
 *
 * Solaris malloc is not smart enough to obtain space through mmap(), so try to make
 * sure that there is some available sbrk() space before we reserve other things.
 */
static void reserve_malloc_space( size_t size )
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

#endif  /* __i386__ */


/***********************************************************************
 *           reserve_dos_area
 *
 * Reserve the DOS area (0x00000000-0x00110000).
 */
static void reserve_dos_area(void)
{
    const size_t page_size = getpagesize();
    const size_t dos_area_size = 0x110000;
    void *ptr;

    /* first page has to be handled specially */
    ptr = wine_anon_mmap( (void *)page_size, dos_area_size - page_size, PROT_NONE, MAP_NORESERVE );
    if (ptr != (void *)page_size)
    {
        if (ptr != (void *)-1) munmap( ptr, dos_area_size - page_size );
        return;
    }
    /* now add first page with MAP_FIXED */
    wine_anon_mmap( NULL, page_size, PROT_NONE, MAP_NORESERVE|MAP_FIXED );
    wine_mmap_add_reserved_area( NULL, dos_area_size );
}


/***********************************************************************
 *           mmap_init
 */
void mmap_init(void)
{
    struct reserved_area *area;
    struct list *ptr;
#if defined(__i386__) && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)  /* commented out until FreeBSD gets fixed */
    char stack;
    char * const stack_ptr = &stack;
    char *user_space_limit = (char *)0x7ffe0000;

    reserve_malloc_space( 8 * 1024 * 1024 );

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

    if (stack_ptr >= user_space_limit)
    {
        char *end = 0;
        char *base = stack_ptr - ((unsigned int)stack_ptr & granularity_mask) - (granularity_mask + 1);
        if (base > user_space_limit) reserve_area( user_space_limit, base );
        base = stack_ptr - ((unsigned int)stack_ptr & granularity_mask) + (granularity_mask + 1);
#ifdef linux
        /* Linux heuristic: assume the stack is near the end of the address */
        /* space, this avoids a lot of futile allocation attempts */
        end = (char *)(((unsigned long)base + 0x0fffffff) & 0xf0000000);
#endif
        reserve_area( base, end );
    }
    else reserve_area( user_space_limit, 0 );
#endif /* __i386__ */

    /* reserve the DOS area if not already done */

    ptr = list_head( &reserved_areas );
    if (ptr)
    {
        area = LIST_ENTRY( ptr, struct reserved_area, entry );
        if (!area->base) return;  /* already reserved */
    }
    reserve_dos_area();
}

#else /* HAVE_MMAP */

void *wine_anon_mmap( void *start, size_t size, int prot, int flags )
{
    return (void *)-1;
}

static inline int munmap( void *ptr, size_t size )
{
    return 0;
}

void mmap_init(void)
{
}

#endif

/***********************************************************************
 *           wine_mmap_add_reserved_area
 *
 * Add an address range to the list of reserved areas.
 * Caller must have made sure the range is not used by anything else.
 *
 * Note: the reserved areas functions are not reentrant, caller is
 * responsible for proper locking.
 */
void wine_mmap_add_reserved_area( void *addr, size_t size )
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


/***********************************************************************
 *           wine_mmap_remove_reserved_area
 *
 * Remove an address range from the list of reserved areas.
 * If 'unmap' is non-zero the range is unmapped too.
 *
 * Note: the reserved areas functions are not reentrant, caller is
 * responsible for proper locking.
 */
void wine_mmap_remove_reserved_area( void *addr, size_t size, int unmap )
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
int wine_mmap_is_in_reserved_area( void *addr, size_t size )
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
int wine_mmap_enum_reserved_areas( int (*enum_func)(void *base, size_t size, void *arg), void *arg,
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
