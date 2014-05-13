/*
 * Win32 virtual memory functions
 *
 * Copyright 1997, 2002 Alexandre Julliard
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
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_VALGRIND_VALGRIND_H
# include <valgrind/valgrind.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(virtual);
WINE_DECLARE_DEBUG_CHANNEL(module);

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

/* File view */
struct file_view
{
    struct list   entry;       /* Entry in global view list */
    void         *base;        /* Base address */
    size_t        size;        /* Size in bytes */
    HANDLE        mapping;     /* Handle to the file mapping */
    unsigned int  map_protect; /* Mapping protection */
    unsigned int  protect;     /* Protection for all pages at allocation time */
    BYTE          prot[1];     /* Protection byte for each page */
};


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

static struct list views_list = LIST_INIT(views_list);

static RTL_CRITICAL_SECTION csVirtual;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csVirtual,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csVirtual") }
};
static RTL_CRITICAL_SECTION csVirtual = { &critsect_debug, -1, 0, 0, 0, 0 };

#ifdef __i386__
static const UINT page_shift = 12;
static const UINT_PTR page_mask = 0xfff;
/* Note: these are Windows limits, you cannot change them. */
static void *address_space_limit = (void *)0xc0000000;  /* top of the total available address space */
static void *user_space_limit    = (void *)0x7fff0000;  /* top of the user address space */
static void *working_set_limit   = (void *)0x7fff0000;  /* top of the current working set */
static void *address_space_start = (void *)0x110000;    /* keep DOS area clear */
#elif defined(__x86_64__)
static const UINT page_shift = 12;
static const UINT_PTR page_mask = 0xfff;
static void *address_space_limit = (void *)0x7fffffff0000;
static void *user_space_limit    = (void *)0x7fffffff0000;
static void *working_set_limit   = (void *)0x7fffffff0000;
static void *address_space_start = (void *)0x10000;
#else
UINT_PTR page_size = 0;
static UINT page_shift;
static UINT_PTR page_mask;
static void *address_space_limit;
static void *user_space_limit;
static void *working_set_limit;
static void *address_space_start = (void *)0x10000;
#endif  /* __i386__ */
static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

#define ROUND_ADDR(addr,mask) \
   ((void *)((UINT_PTR)(addr) & ~(UINT_PTR)(mask)))

#define ROUND_SIZE(addr,size) \
   (((SIZE_T)(size) + ((UINT_PTR)(addr) & page_mask) + page_mask) & ~page_mask)

#define VIRTUAL_DEBUG_DUMP_VIEW(view) \
    do { if (TRACE_ON(virtual)) VIRTUAL_DumpView(view); } while (0)

#define VIRTUAL_HEAP_SIZE (sizeof(void*)*1024*1024)

static HANDLE virtual_heap;
static void *preload_reserve_start;
static void *preload_reserve_end;
static BOOL use_locks;
static BOOL force_exec_prot;  /* whether to force PROT_EXEC on all PROT_READ mmaps */


/***********************************************************************
 *           VIRTUAL_GetProtStr
 */
static const char *VIRTUAL_GetProtStr( BYTE prot )
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
 *           VIRTUAL_GetUnixProt
 *
 * Convert page protections to protection for mmap/mprotect.
 */
static int VIRTUAL_GetUnixProt( BYTE vprot )
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
 *           VIRTUAL_DumpView
 */
static void VIRTUAL_DumpView( struct file_view *view )
{
    UINT i, count;
    char *addr = view->base;
    BYTE prot = view->prot[0];

    TRACE( "View: %p - %p", addr, addr + view->size - 1 );
    if (view->protect & VPROT_SYSTEM)
        TRACE( " (system)\n" );
    else if (view->protect & VPROT_VALLOC)
        TRACE( " (valloc)\n" );
    else if (view->mapping)
        TRACE( " %p\n", view->mapping );
    else
        TRACE( " (anonymous)\n");

    for (count = i = 1; i < view->size >> page_shift; i++, count++)
    {
        if (view->prot[i] == prot) continue;
        TRACE( "      %p - %p %s\n",
                 addr, addr + (count << page_shift) - 1, VIRTUAL_GetProtStr(prot) );
        addr += (count << page_shift);
        prot = view->prot[i];
        count = 0;
    }
    if (count)
        TRACE( "      %p - %p %s\n",
                 addr, addr + (count << page_shift) - 1, VIRTUAL_GetProtStr(prot) );
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
    server_enter_uninterrupted_section( &csVirtual, &sigset );
    LIST_FOR_EACH_ENTRY( view, &views_list, struct file_view, entry )
    {
        VIRTUAL_DumpView( view );
    }
    server_leave_uninterrupted_section( &csVirtual, &sigset );
}
#endif


/***********************************************************************
 *           VIRTUAL_FindView
 *
 * Find the view containing a given address. The csVirtual section must be held by caller.
 *
 * PARAMS
 *      addr  [I] Address
 *
 * RETURNS
 *	View: Success
 *	NULL: Failure
 */
static struct file_view *VIRTUAL_FindView( const void *addr, size_t size )
{
    struct file_view *view;

    LIST_FOR_EACH_ENTRY( view, &views_list, struct file_view, entry )
    {
        if (view->base > addr) break;  /* no matching view */
        if ((const char *)view->base + view->size <= (const char *)addr) continue;
        if ((const char *)view->base + view->size < (const char *)addr + size) break;  /* size too large */
        if ((const char *)addr + size < (const char *)addr) break; /* overflow */
        return view;
    }
    return NULL;
}


/***********************************************************************
 *           get_mask
 */
static inline UINT_PTR get_mask( ULONG zero_bits )
{
    if (!zero_bits) return 0xffff;  /* allocations are aligned to 64K by default */
    if (zero_bits < page_shift) zero_bits = page_shift;
    return (1 << zero_bits) - 1;
}


/***********************************************************************
 *           find_view_range
 *
 * Find the first view overlapping at least part of the specified range.
 * The csVirtual section must be held by caller.
 */
static struct file_view *find_view_range( const void *addr, size_t size )
{
    struct file_view *view;

    LIST_FOR_EACH_ENTRY( view, &views_list, struct file_view, entry )
    {
        if ((const char *)view->base >= (const char *)addr + size) break;
        if ((const char *)view->base + view->size > (const char *)addr) return view;
    }
    return NULL;
}


/***********************************************************************
 *           find_free_area
 *
 * Find a free area between views inside the specified range.
 * The csVirtual section must be held by caller.
 */
static void *find_free_area( void *base, void *end, size_t size, size_t mask, int top_down )
{
    struct list *ptr;
    void *start;

    if (top_down)
    {
        start = ROUND_ADDR( (char *)end - size, mask );
        if (start >= end || start < base) return NULL;

        for (ptr = views_list.prev; ptr != &views_list; ptr = ptr->prev)
        {
            struct file_view *view = LIST_ENTRY( ptr, struct file_view, entry );

            if ((char *)view->base + view->size <= (char *)start) break;
            if ((char *)view->base >= (char *)start + size) continue;
            start = ROUND_ADDR( (char *)view->base - size, mask );
            /* stop if remaining space is not large enough */
            if (!start || start >= end || start < base) return NULL;
        }
    }
    else
    {
        start = ROUND_ADDR( (char *)base + mask, mask );
        if (start >= end || (char *)end - (char *)start < size) return NULL;

        for (ptr = views_list.next; ptr != &views_list; ptr = ptr->next)
        {
            struct file_view *view = LIST_ENTRY( ptr, struct file_view, entry );

            if ((char *)view->base >= (char *)start + size) break;
            if ((char *)view->base + view->size <= (char *)start) continue;
            start = ROUND_ADDR( (char *)view->base + view->size + mask, mask );
            /* stop if remaining space is not large enough */
            if (!start || start >= end || (char *)end - (char *)start < size) return NULL;
        }
    }
    return start;
}


/***********************************************************************
 *           add_reserved_area
 *
 * Add a reserved area to the list maintained by libwine.
 * The csVirtual section must be held by caller.
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
    wine_anon_mmap( addr, size, PROT_NONE, MAP_NORESERVE | MAP_FIXED );
    wine_mmap_add_reserved_area( addr, size );
}


/***********************************************************************
 *           remove_reserved_area
 *
 * Remove a reserved area from the list maintained by libwine.
 * The csVirtual section must be held by caller.
 */
static void remove_reserved_area( void *addr, size_t size )
{
    struct file_view *view;

    TRACE( "removing %p-%p\n", addr, (char *)addr + size );
    wine_mmap_remove_reserved_area( addr, size, 0 );

    /* unmap areas not covered by an existing view */
    LIST_FOR_EACH_ENTRY( view, &views_list, struct file_view, entry )
    {
        if ((char *)view->base >= (char *)addr + size)
        {
            munmap( addr, size );
            break;
        }
        if ((char *)view->base + view->size <= (char *)addr) continue;
        if (view->base > addr) munmap( addr, (char *)view->base - (char *)addr );
        if ((char *)view->base + view->size > (char *)addr + size) break;
        size = (char *)addr + size - ((char *)view->base + view->size);
        addr = (char *)view->base + view->size;
    }
}


/***********************************************************************
 *           is_beyond_limit
 *
 * Check if an address range goes beyond a given limit.
 */
static inline BOOL is_beyond_limit( const void *addr, size_t size, const void *limit )
{
    return (addr >= limit || (const char *)addr + size > (const char *)limit);
}


/***********************************************************************
 *           unmap_area
 *
 * Unmap an area, or simply replace it by an empty mapping if it is
 * in a reserved area. The csVirtual section must be held by caller.
 */
static inline void unmap_area( void *addr, size_t size )
{
    if (wine_mmap_is_in_reserved_area( addr, size ))
        wine_anon_mmap( addr, size, PROT_NONE, MAP_NORESERVE | MAP_FIXED );
    else if (is_beyond_limit( addr, size, user_space_limit ))
        add_reserved_area( addr, size );
    else
        munmap( addr, size );
}


/***********************************************************************
 *           delete_view
 *
 * Deletes a view. The csVirtual section must be held by caller.
 */
static void delete_view( struct file_view *view ) /* [in] View */
{
    if (!(view->protect & VPROT_SYSTEM)) unmap_area( view->base, view->size );
    list_remove( &view->entry );
    if (view->mapping) close_handle( view->mapping );
    RtlFreeHeap( virtual_heap, 0, view );
}


/***********************************************************************
 *           create_view
 *
 * Create a view. The csVirtual section must be held by caller.
 */
static NTSTATUS create_view( struct file_view **view_ret, void *base, size_t size, unsigned int vprot )
{
    struct file_view *view;
    struct list *ptr;
    int unix_prot = VIRTUAL_GetUnixProt( vprot );

    assert( !((UINT_PTR)base & page_mask) );
    assert( !(size & page_mask) );

    /* Create the view structure */

    if (!(view = RtlAllocateHeap( virtual_heap, 0, sizeof(*view) + (size >> page_shift) - 1 )))
    {
        FIXME( "out of memory in virtual heap for %p-%p\n", base, (char *)base + size );
        return STATUS_NO_MEMORY;
    }

    view->base    = base;
    view->size    = size;
    view->mapping = 0;
    view->map_protect = 0;
    view->protect = vprot;
    memset( view->prot, vprot, size >> page_shift );

    /* Insert it in the linked list */

    LIST_FOR_EACH( ptr, &views_list )
    {
        struct file_view *next = LIST_ENTRY( ptr, struct file_view, entry );
        if (next->base > base) break;
    }
    list_add_before( ptr, &view->entry );

    /* Check for overlapping views. This can happen if the previous view
     * was a system view that got unmapped behind our back. In that case
     * we recover by simply deleting it. */

    if ((ptr = list_prev( &views_list, &view->entry )) != NULL)
    {
        struct file_view *prev = LIST_ENTRY( ptr, struct file_view, entry );
        if ((char *)prev->base + prev->size > (char *)base)
        {
            TRACE( "overlapping prev view %p-%p for %p-%p\n",
                   prev->base, (char *)prev->base + prev->size,
                   base, (char *)base + view->size );
            assert( prev->protect & VPROT_SYSTEM );
            delete_view( prev );
        }
    }
    if ((ptr = list_next( &views_list, &view->entry )) != NULL)
    {
        struct file_view *next = LIST_ENTRY( ptr, struct file_view, entry );
        if ((char *)base + view->size > (char *)next->base)
        {
            TRACE( "overlapping next view %p-%p for %p-%p\n",
                   next->base, (char *)next->base + next->size,
                   base, (char *)base + view->size );
            assert( next->protect & VPROT_SYSTEM );
            delete_view( next );
        }
    }

    *view_ret = view;
    VIRTUAL_DEBUG_DUMP_VIEW( view );

    if (force_exec_prot && !(vprot & VPROT_NOEXEC) && (unix_prot & PROT_READ) && !(unix_prot & PROT_EXEC))
    {
        TRACE( "forcing exec permission on %p-%p\n", base, (char *)base + size - 1 );
        mprotect( base, size, unix_prot | PROT_EXEC );
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           VIRTUAL_GetWin32Prot
 *
 * Convert page protections to Win32 flags.
 */
static DWORD VIRTUAL_GetWin32Prot( BYTE vprot )
{
    DWORD ret = VIRTUAL_Win32Flags[vprot & 0x0f];
    if (vprot & VPROT_NOCACHE) ret |= PAGE_NOCACHE;
    if (vprot & VPROT_GUARD) ret |= PAGE_GUARD;
    return ret;
}


/***********************************************************************
 *           get_vprot_flags
 *
 * Build page protections from Win32 flags.
 *
 * PARAMS
 *      protect [I] Win32 protection flags
 *
 * RETURNS
 *	Value of page protection flags
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
    if (protect & PAGE_NOCACHE) *vprot |= VPROT_NOCACHE;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           VIRTUAL_SetProt
 *
 * Change the protection of a range of pages.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
static BOOL VIRTUAL_SetProt( struct file_view *view, /* [in] Pointer to view */
                             void *base,      /* [in] Starting address */
                             size_t size,     /* [in] Size in bytes */
                             BYTE vprot )     /* [in] Protections to use */
{
    int unix_prot = VIRTUAL_GetUnixProt(vprot);
    BYTE *p = view->prot + (((char *)base - (char *)view->base) >> page_shift);

    TRACE("%p-%p %s\n",
          base, (char *)base + size - 1, VIRTUAL_GetProtStr( vprot ) );

    if (view->protect & VPROT_WRITEWATCH)
    {
        /* each page may need different protections depending on write watch flag */
        UINT i, count;
        char *addr = base;
        int prot;

        p[0] = vprot | (p[0] & VPROT_WRITEWATCH);
        unix_prot = VIRTUAL_GetUnixProt( p[0] );
        for (count = i = 1; i < size >> page_shift; i++, count++)
        {
            p[i] = vprot | (p[i] & VPROT_WRITEWATCH);
            prot = VIRTUAL_GetUnixProt( p[i] );
            if (prot == unix_prot) continue;
            mprotect( addr, count << page_shift, unix_prot );
            addr += count << page_shift;
            unix_prot = prot;
            count = 0;
        }
        if (count) mprotect( addr, count << page_shift, unix_prot );
        VIRTUAL_DEBUG_DUMP_VIEW( view );
        return TRUE;
    }

    /* if setting stack guard pages, store the permissions first, as the guard may be
     * triggered at any point after mprotect and change the permissions again */
    if ((vprot & VPROT_GUARD) &&
        (base >= NtCurrentTeb()->DeallocationStack) &&
        (base < NtCurrentTeb()->Tib.StackBase))
    {
        memset( p, vprot, size >> page_shift );
        mprotect( base, size, unix_prot );
        VIRTUAL_DEBUG_DUMP_VIEW( view );
        return TRUE;
    }

    if (force_exec_prot && !(view->protect & VPROT_NOEXEC) &&
        (unix_prot & PROT_READ) && !(unix_prot & PROT_EXEC))
    {
        TRACE( "forcing exec permission on %p-%p\n", base, (char *)base + size - 1 );
        if (!mprotect( base, size, unix_prot | PROT_EXEC )) goto done;
        /* exec + write may legitimately fail, in that case fall back to write only */
        if (!(unix_prot & PROT_WRITE)) return FALSE;
    }

    if (mprotect( base, size, unix_prot )) return FALSE;  /* FIXME: last error */

done:
    memset( p, vprot, size >> page_shift );
    VIRTUAL_DEBUG_DUMP_VIEW( view );
    return TRUE;
}


/***********************************************************************
 *           reset_write_watches
 *
 * Reset write watches in a memory range.
 */
static void reset_write_watches( struct file_view *view, void *base, SIZE_T size )
{
    SIZE_T i, count;
    int prot, unix_prot;
    char *addr = base;
    BYTE *p = view->prot + ((addr - (char *)view->base) >> page_shift);

    p[0] |= VPROT_WRITEWATCH;
    unix_prot = VIRTUAL_GetUnixProt( p[0] );
    for (count = i = 1; i < size >> page_shift; i++, count++)
    {
        p[i] |= VPROT_WRITEWATCH;
        prot = VIRTUAL_GetUnixProt( p[i] );
        if (prot == unix_prot) continue;
        mprotect( addr, count << page_shift, unix_prot );
        addr += count << page_shift;
        unix_prot = prot;
        count = 0;
    }
    if (count) mprotect( addr, count << page_shift, unix_prot );
}


/***********************************************************************
 *           unmap_extra_space
 *
 * Release the extra memory while keeping the range starting on the granularity boundary.
 */
static inline void *unmap_extra_space( void *ptr, size_t total_size, size_t wanted_size, size_t mask )
{
    if ((ULONG_PTR)ptr & mask)
    {
        size_t extra = mask + 1 - ((ULONG_PTR)ptr & mask);
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
    size_t mask;
    int    top_down;
    void  *limit;
    void  *result;
};

/***********************************************************************
 *           alloc_reserved_area_callback
 *
 * Try to map some space inside a reserved area. Callback for wine_mmap_enum_reserved_areas.
 */
static int alloc_reserved_area_callback( void *start, size_t size, void *arg )
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
            if ((alloc->result = find_free_area( start, preload_reserve_start, alloc->size,
                                                 alloc->mask, alloc->top_down )))
                return 1;
            /* then fall through to try second part */
            start = preload_reserve_end;
        }
    }
    if ((alloc->result = find_free_area( start, end, alloc->size, alloc->mask, alloc->top_down )))
        return 1;

    return 0;
}


/***********************************************************************
 *           map_view
 *
 * Create a view and mmap the corresponding memory area.
 * The csVirtual section must be held by caller.
 */
static NTSTATUS map_view( struct file_view **view_ret, void *base, size_t size, size_t mask,
                          int top_down, unsigned int vprot )
{
    void *ptr;
    NTSTATUS status;

    if (base)
    {
        if (is_beyond_limit( base, size, address_space_limit ))
            return STATUS_WORKING_SET_LIMIT_RANGE;

        switch (wine_mmap_is_in_reserved_area( base, size ))
        {
        case -1: /* partially in a reserved area */
            return STATUS_CONFLICTING_ADDRESSES;

        case 0:  /* not in a reserved area, do a normal allocation */
            if ((ptr = wine_anon_mmap( base, size, VIRTUAL_GetUnixProt(vprot), 0 )) == (void *)-1)
            {
                if (errno == ENOMEM) return STATUS_NO_MEMORY;
                return STATUS_INVALID_PARAMETER;
            }
            if (ptr != base)
            {
                /* We couldn't get the address we wanted */
                if (is_beyond_limit( ptr, size, user_space_limit )) add_reserved_area( ptr, size );
                else munmap( ptr, size );
                return STATUS_CONFLICTING_ADDRESSES;
            }
            break;

        default:
        case 1:  /* in a reserved area, make sure the address is available */
            if (find_view_range( base, size )) return STATUS_CONFLICTING_ADDRESSES;
            /* replace the reserved area by our mapping */
            if ((ptr = wine_anon_mmap( base, size, VIRTUAL_GetUnixProt(vprot), MAP_FIXED )) != base)
                return STATUS_INVALID_PARAMETER;
            break;
        }
        if (is_beyond_limit( ptr, size, working_set_limit )) working_set_limit = address_space_limit;
    }
    else
    {
        size_t view_size = size + mask + 1;
        struct alloc_area alloc;

        alloc.size = size;
        alloc.mask = mask;
        alloc.top_down = top_down;
        alloc.limit = user_space_limit;
        if (wine_mmap_enum_reserved_areas( alloc_reserved_area_callback, &alloc, top_down ))
        {
            ptr = alloc.result;
            TRACE( "got mem in reserved area %p-%p\n", ptr, (char *)ptr + size );
            if (wine_anon_mmap( ptr, size, VIRTUAL_GetUnixProt(vprot), MAP_FIXED ) != ptr)
                return STATUS_INVALID_PARAMETER;
            goto done;
        }

        for (;;)
        {
            if ((ptr = wine_anon_mmap( NULL, view_size, VIRTUAL_GetUnixProt(vprot), 0 )) == (void *)-1)
            {
                if (errno == ENOMEM) return STATUS_NO_MEMORY;
                return STATUS_INVALID_PARAMETER;
            }
            TRACE( "got mem with anon mmap %p-%p\n", ptr, (char *)ptr + size );
            /* if we got something beyond the user limit, unmap it and retry */
            if (is_beyond_limit( ptr, view_size, user_space_limit )) add_reserved_area( ptr, view_size );
            else break;
        }
        ptr = unmap_extra_space( ptr, view_size, size, mask );
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
 * The csVirtual section must be held by caller.
 */
static NTSTATUS map_file_into_view( struct file_view *view, int fd, size_t start, size_t size,
                                    off_t offset, unsigned int vprot, BOOL removable )
{
    void *ptr;
    int prot = VIRTUAL_GetUnixProt( vprot | VPROT_COMMITTED /* make sure it is accessible */ );
    unsigned int flags = MAP_FIXED | ((vprot & VPROT_WRITECOPY) ? MAP_PRIVATE : MAP_SHARED);

    assert( start < view->size );
    assert( start + size <= view->size );

    if (force_exec_prot && !(vprot & VPROT_NOEXEC) && (vprot & VPROT_READ))
    {
        TRACE( "forcing exec permission on mapping %p-%p\n",
               (char *)view->base + start, (char *)view->base + start + size - 1 );
        prot |= PROT_EXEC;
    }

    /* only try mmap if media is not removable (or if we require write access) */
    if (!removable || (flags & MAP_SHARED))
    {
        if (mmap( (char *)view->base + start, size, prot, flags, fd, offset ) != (void *)-1)
            goto done;

        if ((errno == EPERM) && (prot & PROT_EXEC))
            ERR( "failed to set %08x protection on file map, noexec filesystem?\n", prot );

        /* mmap() failed; if this is because the file offset is not    */
        /* page-aligned (EINVAL), or because the underlying filesystem */
        /* does not support mmap() (ENOEXEC,ENODEV), we do it by hand. */
        if ((errno != ENOEXEC) && (errno != EINVAL) && (errno != ENODEV)) return FILE_GetNtStatus();
        if (flags & MAP_SHARED)  /* we cannot fake shared mappings */
        {
            if (errno == EINVAL) return STATUS_INVALID_PARAMETER;
            ERR( "shared writable mmap not supported, broken filesystem?\n" );
            return STATUS_NOT_SUPPORTED;
        }
    }

    /* Reserve the memory with an anonymous mmap */
    ptr = wine_anon_mmap( (char *)view->base + start, size, PROT_READ | PROT_WRITE, MAP_FIXED );
    if (ptr == (void *)-1) return FILE_GetNtStatus();
    /* Now read in the file */
    pread( fd, ptr, size, offset );
    if (prot != (PROT_READ|PROT_WRITE)) mprotect( ptr, size, prot );  /* Set the right protection */
done:
    memset( view->prot + (start >> page_shift), vprot, ROUND_SIZE(start,size) >> page_shift );
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
    *vprot = view->prot[start];

    if (view->mapping && !(view->protect & VPROT_COMMITTED))
    {
        SIZE_T ret = 0;
        SERVER_START_REQ( get_mapping_committed_range )
        {
            req->handle = wine_server_obj_handle( view->mapping );
            req->offset = start << page_shift;
            if (!wine_server_call( req ))
            {
                ret = reply->size;
                if (reply->committed)
                {
                    *vprot |= VPROT_COMMITTED;
                    for (i = 0; i < ret >> page_shift; i++) view->prot[start+i] |= VPROT_COMMITTED;
                }
            }
        }
        SERVER_END_REQ;
        return ret;
    }
    for (i = start + 1; i < view->size >> page_shift; i++)
        if ((*vprot ^ view->prot[i]) & VPROT_COMMITTED) break;
    return (i - start) << page_shift;
}


/***********************************************************************
 *           decommit_view
 *
 * Decommit some pages of a given view.
 * The csVirtual section must be held by caller.
 */
static NTSTATUS decommit_pages( struct file_view *view, size_t start, size_t size )
{
    if (wine_anon_mmap( (char *)view->base + start, size, PROT_NONE, MAP_FIXED ) != (void *)-1)
    {
        BYTE *p = view->prot + (start >> page_shift);
        size >>= page_shift;
        while (size--) *p++ &= ~VPROT_COMMITTED;
        return STATUS_SUCCESS;
    }
    return FILE_GetNtStatus();
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
    int unix_prot = VIRTUAL_GetUnixProt( vprot );
    struct list *ptr;

    /* check for existing view */

    if ((ptr = list_head( &views_list )))
    {
        struct file_view *first_view = LIST_ENTRY( ptr, struct file_view, entry );
        if (first_view->base < (void *)dosmem_size) return STATUS_CONFLICTING_ADDRESSES;
    }

    /* check without the first 64K */

    if (wine_mmap_is_in_reserved_area( low_64k, dosmem_size - 0x10000 ) != 1)
    {
        addr = wine_anon_mmap( low_64k, dosmem_size - 0x10000, unix_prot, 0 );
        if (addr != low_64k)
        {
            if (addr != (void *)-1) munmap( addr, dosmem_size - 0x10000 );
            return map_view( view, NULL, dosmem_size, 0xffff, 0, vprot );
        }
    }

    /* now try to allocate the low 64K too */

    if (wine_mmap_is_in_reserved_area( NULL, 0x10000 ) != 1)
    {
        addr = wine_anon_mmap( (void *)page_size, 0x10000 - page_size, unix_prot, 0 );
        if (addr == (void *)page_size)
        {
            if (!wine_anon_mmap( NULL, page_size, unix_prot, MAP_FIXED ))
            {
                addr = NULL;
                TRACE( "successfully mapped low 64K range\n" );
            }
            else TRACE( "failed to map page 0\n" );
        }
        else
        {
            if (addr != (void *)-1) munmap( addr, 0x10000 - page_size );
            addr = low_64k;
            TRACE( "failed to map low 64K range\n" );
        }
    }

    /* now reserve the whole range */

    size = (char *)dosmem_size - (char *)addr;
    wine_anon_mmap( addr, size, unix_prot, MAP_FIXED );
    return create_view( view, addr, size, vprot );
}


/***********************************************************************
 *           stat_mapping_file
 *
 * Stat the underlying file for a memory view.
 */
static NTSTATUS stat_mapping_file( struct file_view *view, struct stat *st )
{
    NTSTATUS status;
    int unix_fd, needs_close;

    if (!view->mapping) return STATUS_NOT_MAPPED_VIEW;
    if (!(status = server_get_unix_fd( view->mapping, 0, &unix_fd, &needs_close, NULL, NULL )))
    {
        if (fstat( unix_fd, st ) == -1) status = FILE_GetNtStatus();
        if (needs_close) close( unix_fd );
    }
    return status;
}


/***********************************************************************
 *           map_image
 *
 * Map an executable (PE format) image into memory.
 */
static NTSTATUS map_image( HANDLE hmapping, int fd, char *base, SIZE_T total_size, SIZE_T mask,
                           SIZE_T header_size, int shared_fd, HANDLE dup_mapping, unsigned int map_vprot, PVOID *addr_ptr )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER sections[96];
    IMAGE_SECTION_HEADER *sec;
    IMAGE_DATA_DIRECTORY *imports;
    NTSTATUS status = STATUS_CONFLICTING_ADDRESSES;
    int i;
    off_t pos;
    sigset_t sigset;
    struct stat st;
    struct file_view *view = NULL;
    char *ptr, *header_end, *header_start;
    INT_PTR delta = 0;

    /* zero-map the whole range */

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    if (base >= (char *)address_space_start)  /* make sure the DOS area remains free */
        status = map_view( &view, base, total_size, mask, FALSE,
                           VPROT_COMMITTED | VPROT_READ | VPROT_EXEC | VPROT_WRITECOPY | VPROT_IMAGE );

    if (status != STATUS_SUCCESS)
        status = map_view( &view, NULL, total_size, mask, FALSE,
                           VPROT_COMMITTED | VPROT_READ | VPROT_EXEC | VPROT_WRITECOPY | VPROT_IMAGE );

    if (status != STATUS_SUCCESS) goto error;

    ptr = view->base;
    TRACE_(module)( "mapped PE file at %p-%p\n", ptr, ptr + total_size );

    /* map the header */

    if (fstat( fd, &st ) == -1)
    {
        status = FILE_GetNtStatus();
        goto error;
    }
    status = STATUS_INVALID_IMAGE_FORMAT;  /* generic error */
    if (!st.st_size) goto error;
    header_size = min( header_size, st.st_size );
    if (map_file_into_view( view, fd, 0, header_size, 0, VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY,
                            !dup_mapping ) != STATUS_SUCCESS) goto error;
    dos = (IMAGE_DOS_HEADER *)ptr;
    nt = (IMAGE_NT_HEADERS *)(ptr + dos->e_lfanew);
    header_end = ptr + ROUND_SIZE( 0, header_size );
    memset( ptr + header_size, 0, header_end - (ptr + header_size) );
    if ((char *)(nt + 1) > header_end) goto error;
    header_start = (char*)&nt->OptionalHeader+nt->FileHeader.SizeOfOptionalHeader;
    if (nt->FileHeader.NumberOfSections > sizeof(sections)/sizeof(*sections)) goto error;
    if (header_start + sizeof(*sections) * nt->FileHeader.NumberOfSections > header_end) goto error;
    /* Some applications (e.g. the Steam version of Borderlands) map over the top of the section headers,
     * copying the headers into local memory is necessary to properly load such applications. */
    memcpy(sections, header_start, sizeof(*sections) * nt->FileHeader.NumberOfSections);
    sec = sections;

    imports = nt->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_IMPORT;
    if (!imports->Size || !imports->VirtualAddress) imports = NULL;

    /* check for non page-aligned binary */

    if (nt->OptionalHeader.SectionAlignment <= page_mask)
    {
        /* unaligned sections, this happens for native subsystem binaries */
        /* in that case Windows simply maps in the whole file */

        if (map_file_into_view( view, fd, 0, total_size, 0, VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY,
                                !dup_mapping ) != STATUS_SUCCESS) goto error;

        /* check that all sections are loaded at the right offset */
        if (nt->OptionalHeader.FileAlignment != nt->OptionalHeader.SectionAlignment) goto error;
        for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
        {
            if (sec[i].VirtualAddress != sec[i].PointerToRawData)
                goto error;  /* Windows refuses to load in that case too */
        }

        /* set the image protections */
        VIRTUAL_SetProt( view, ptr, total_size,
                         VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY | VPROT_EXEC );

        /* no relocations are performed on non page-aligned binaries */
        goto done;
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
            WARN_(module)( "Section %.8s too large (%x+%lx/%lx)\n",
                           sec->Name, sec->VirtualAddress, map_size, total_size );
            goto error;
        }

        if ((sec->Characteristics & IMAGE_SCN_MEM_SHARED) &&
            (sec->Characteristics & IMAGE_SCN_MEM_WRITE))
        {
            TRACE_(module)( "mapping shared section %.8s at %p off %x (%x) size %lx (%lx) flags %x\n",
                            sec->Name, ptr + sec->VirtualAddress,
                            sec->PointerToRawData, (int)pos, file_size, map_size,
                            sec->Characteristics );
            if (map_file_into_view( view, shared_fd, sec->VirtualAddress, map_size, pos,
                                    VPROT_COMMITTED | VPROT_READ | VPROT_WRITE, FALSE ) != STATUS_SUCCESS)
            {
                ERR_(module)( "Could not map shared section %.8s\n", sec->Name );
                goto error;
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

        TRACE_(module)( "mapping section %.8s at %p off %x size %x virt %x flags %x\n",
                        sec->Name, ptr + sec->VirtualAddress,
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
                                !dup_mapping ) != STATUS_SUCCESS)
        {
            ERR_(module)( "Could not map section %.8s, file probably truncated\n", sec->Name );
            goto error;
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


    /* perform base relocation, if necessary */

    if (ptr != base &&
        ((nt->FileHeader.Characteristics & IMAGE_FILE_DLL) ||
          !NtCurrentTeb()->Peb->ImageBaseAddress) )
    {
        IMAGE_BASE_RELOCATION *rel, *end;
        const IMAGE_DATA_DIRECTORY *relocs;

        if (nt->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
        {
            WARN_(module)( "Need to relocate module from %p to %p, but there are no relocation records\n",
                           base, ptr );
            status = STATUS_CONFLICTING_ADDRESSES;
            goto error;
        }

        TRACE_(module)( "relocating from %p-%p to %p-%p\n",
                        base, base + total_size, ptr, ptr + total_size );

        relocs = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        rel = (IMAGE_BASE_RELOCATION *)(ptr + relocs->VirtualAddress);
        end = (IMAGE_BASE_RELOCATION *)(ptr + relocs->VirtualAddress + relocs->Size);
        delta = ptr - base;

        while (rel < end - 1 && rel->SizeOfBlock)
        {
            if (rel->VirtualAddress >= total_size)
            {
                WARN_(module)( "invalid address %p in relocation %p\n", ptr + rel->VirtualAddress, rel );
                status = STATUS_ACCESS_VIOLATION;
                goto error;
            }
            rel = LdrProcessRelocationBlock( ptr + rel->VirtualAddress,
                                             (rel->SizeOfBlock - sizeof(*rel)) / sizeof(USHORT),
                                             (USHORT *)(rel + 1), delta );
            if (!rel) goto error;
        }
    }

    /* set the image protections */

    VIRTUAL_SetProt( view, ptr, ROUND_SIZE( 0, header_size ), VPROT_COMMITTED | VPROT_READ );

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

        if (!VIRTUAL_SetProt( view, ptr + sec->VirtualAddress, size, vprot ) && (vprot & VPROT_EXEC))
            ERR( "failed to set %08x protection on section %.8s, noexec filesystem?\n",
                 sec->Characteristics, sec->Name );
    }

 done:
    view->mapping = dup_mapping;
    view->map_protect = map_vprot;
    server_leave_uninterrupted_section( &csVirtual, &sigset );

    *addr_ptr = ptr;
#ifdef VALGRIND_LOAD_PDB_DEBUGINFO
    VALGRIND_LOAD_PDB_DEBUGINFO(fd, ptr, total_size, delta);
#endif
    if (ptr != base) return STATUS_IMAGE_NOT_AT_BASE;
    return STATUS_SUCCESS;

 error:
    if (view) delete_view( view );
    server_leave_uninterrupted_section( &csVirtual, &sigset );
    if (dup_mapping) NtClose( dup_mapping );
    return status;
}


/* callback for wine_mmap_enum_reserved_areas to allocate space for the virtual heap */
static int alloc_virtual_heap( void *base, size_t size, void *arg )
{
    void **heap_base = arg;

    if (is_beyond_limit( base, size, address_space_limit )) address_space_limit = (char *)base + size;
    if (size < VIRTUAL_HEAP_SIZE) return 0;
    if (is_win64 && base < (void *)0x80000000) return 0;
    *heap_base = wine_anon_mmap( (char *)base + size - VIRTUAL_HEAP_SIZE,
                                 VIRTUAL_HEAP_SIZE, PROT_READ|PROT_WRITE, MAP_FIXED );
    return (*heap_base != (void *)-1);
}

/***********************************************************************
 *           virtual_init
 */
void virtual_init(void)
{
    const char *preload;
    void *heap_base;
    size_t size;
    struct file_view *heap_view;

#if !defined(__i386__) && !defined(__x86_64__)
    page_size = sysconf( _SC_PAGESIZE );
    page_mask = page_size - 1;
    /* Make sure we have a power of 2 */
    assert( !(page_size & page_mask) );
    page_shift = 0;
    while ((1 << page_shift) != page_size) page_shift++;
    user_space_limit = working_set_limit = address_space_limit = (void *)~page_mask;
#endif  /* page_mask */
    if ((preload = getenv("WINEPRELOADRESERVE")))
    {
        unsigned long start, end;
        if (sscanf( preload, "%lx-%lx", &start, &end ) == 2)
        {
            preload_reserve_start = (void *)start;
            preload_reserve_end = (void *)end;
        }
    }

    /* try to find space in a reserved area for the virtual heap */
    if (!wine_mmap_enum_reserved_areas( alloc_virtual_heap, &heap_base, 1 ))
        heap_base = wine_anon_mmap( NULL, VIRTUAL_HEAP_SIZE, PROT_READ|PROT_WRITE, 0 );

    assert( heap_base != (void *)-1 );
    virtual_heap = RtlCreateHeap( HEAP_NO_SERIALIZE, heap_base, VIRTUAL_HEAP_SIZE,
                                  VIRTUAL_HEAP_SIZE, NULL, NULL );
    create_view( &heap_view, heap_base, VIRTUAL_HEAP_SIZE, VPROT_COMMITTED | VPROT_READ | VPROT_WRITE );

    /* make the DOS area accessible (except the low 64K) to hide bugs in broken apps like Excel 2003 */
    size = (char *)address_space_start - (char *)0x10000;
    if (size && wine_mmap_is_in_reserved_area( (void*)0x10000, size ) == 1)
        wine_anon_mmap( (void *)0x10000, size, PROT_READ | PROT_WRITE, MAP_FIXED );
}


/***********************************************************************
 *           virtual_init_threading
 */
void virtual_init_threading(void)
{
    use_locks = TRUE;
}


/***********************************************************************
 *           virtual_get_system_info
 */
void virtual_get_system_info( SYSTEM_BASIC_INFORMATION *info )
{
    info->unknown                 = 0;
    info->KeMaximumIncrement      = 0;  /* FIXME */
    info->PageSize                = page_size;
    info->MmLowestPhysicalPage    = 1;
    info->MmHighestPhysicalPage   = 0x7fffffff / page_size;
    info->MmNumberOfPhysicalPages = info->MmHighestPhysicalPage - info->MmLowestPhysicalPage;
    info->AllocationGranularity   = get_mask(0) + 1;
    info->LowestUserAddress       = (void *)0x10000;
    info->HighestUserAddress      = (char *)user_space_limit - 1;
    info->ActiveProcessorsAffinityMask = (1 << NtCurrentTeb()->Peb->NumberOfProcessors) - 1;
    info->NumberOfProcessors      = NtCurrentTeb()->Peb->NumberOfProcessors;
}


/***********************************************************************
 *           virtual_create_builtin_view
 */
NTSTATUS virtual_create_builtin_view( void *module )
{
    NTSTATUS status;
    sigset_t sigset;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module );
    SIZE_T size = nt->OptionalHeader.SizeOfImage;
    IMAGE_SECTION_HEADER *sec;
    struct file_view *view;
    void *base;
    int i;

    size = ROUND_SIZE( module, size );
    base = ROUND_ADDR( module, page_mask );
    server_enter_uninterrupted_section( &csVirtual, &sigset );
    status = create_view( &view, base, size, VPROT_SYSTEM | VPROT_IMAGE |
                          VPROT_COMMITTED | VPROT_READ | VPROT_WRITECOPY | VPROT_EXEC );
    if (!status) TRACE( "created %p-%p\n", base, (char *)base + size );
    server_leave_uninterrupted_section( &csVirtual, &sigset );

    if (status) return status;

    /* The PE header is always read-only, no write, no execute. */
    view->prot[0] = VPROT_COMMITTED | VPROT_READ;

    sec = (IMAGE_SECTION_HEADER *)((char *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        BYTE flags = VPROT_COMMITTED;

        if (sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) flags |= VPROT_EXEC;
        if (sec[i].Characteristics & IMAGE_SCN_MEM_READ) flags |= VPROT_READ;
        if (sec[i].Characteristics & IMAGE_SCN_MEM_WRITE) flags |= VPROT_WRITE;
        memset (view->prot + (sec[i].VirtualAddress >> page_shift), flags,
                ROUND_SIZE( sec[i].VirtualAddress, sec[i].Misc.VirtualSize ) >> page_shift );
    }

    return status;
}


/***********************************************************************
 *           virtual_alloc_thread_stack
 */
NTSTATUS virtual_alloc_thread_stack( TEB *teb, SIZE_T reserve_size, SIZE_T commit_size )
{
    struct file_view *view;
    NTSTATUS status;
    sigset_t sigset;
    SIZE_T size;

    if (!reserve_size || !commit_size)
    {
        IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
        if (!reserve_size) reserve_size = nt->OptionalHeader.SizeOfStackReserve;
        if (!commit_size) commit_size = nt->OptionalHeader.SizeOfStackCommit;
    }

    size = max( reserve_size, commit_size );
    if (size < 1024 * 1024) size = 1024 * 1024;  /* Xlib needs a large stack */
    size = (size + 0xffff) & ~0xffff;  /* round to 64K boundary */

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    if ((status = map_view( &view, NULL, size, 0xffff, 0,
                            VPROT_READ | VPROT_WRITE | VPROT_COMMITTED | VPROT_VALLOC )) != STATUS_SUCCESS)
        goto done;

#ifdef VALGRIND_STACK_REGISTER
    VALGRIND_STACK_REGISTER( view->base, (char *)view->base + view->size );
#endif

    /* setup no access guard page */
    VIRTUAL_SetProt( view, view->base, page_size, VPROT_COMMITTED );
    VIRTUAL_SetProt( view, (char *)view->base + page_size, page_size,
                     VPROT_READ | VPROT_WRITE | VPROT_COMMITTED | VPROT_GUARD );

    /* note: limit is lower than base since the stack grows down */
    teb->DeallocationStack = view->base;
    teb->Tib.StackBase     = (char *)view->base + view->size;
    teb->Tib.StackLimit    = (char *)view->base + 2 * page_size;
done:
    server_leave_uninterrupted_section( &csVirtual, &sigset );
    return status;
}


/***********************************************************************
 *           virtual_clear_thread_stack
 *
 * Clear the stack contents before calling the main entry point, some broken apps need that.
 */
void virtual_clear_thread_stack(void)
{
    void *stack = NtCurrentTeb()->Tib.StackLimit;
    size_t size = (char *)NtCurrentTeb()->Tib.StackBase - (char *)NtCurrentTeb()->Tib.StackLimit;

    wine_anon_mmap( stack, size, PROT_READ | PROT_WRITE, MAP_FIXED );
    if (force_exec_prot) mprotect( stack, size, PROT_READ | PROT_WRITE | PROT_EXEC );
}


/***********************************************************************
 *           virtual_handle_fault
 */
NTSTATUS virtual_handle_fault( LPCVOID addr, DWORD err )
{
    struct file_view *view;
    NTSTATUS ret = STATUS_ACCESS_VIOLATION;
    sigset_t sigset;

    server_enter_uninterrupted_section( &csVirtual, &sigset );
    if ((view = VIRTUAL_FindView( addr, 0 )))
    {
        void *page = ROUND_ADDR( addr, page_mask );
        BYTE *vprot = &view->prot[((const char *)page - (const char *)view->base) >> page_shift];
        if (*vprot & VPROT_GUARD)
        {
            VIRTUAL_SetProt( view, page, page_size, *vprot & ~VPROT_GUARD );
            ret = STATUS_GUARD_PAGE_VIOLATION;
        }
        if ((err & EXCEPTION_WRITE_FAULT) && (view->protect & VPROT_WRITEWATCH))
        {
            if (*vprot & VPROT_WRITEWATCH)
            {
                *vprot &= ~VPROT_WRITEWATCH;
                VIRTUAL_SetProt( view, page, page_size, *vprot );
            }
            /* ignore fault if page is writable now */
            if (VIRTUAL_GetUnixProt( *vprot ) & PROT_WRITE) ret = STATUS_SUCCESS;
        }
    }
    server_leave_uninterrupted_section( &csVirtual, &sigset );
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

    server_enter_uninterrupted_section( &csVirtual, &sigset );
    if ((view = VIRTUAL_FindView( addr, size )))
        ret = !(view->protect & VPROT_SYSTEM);  /* system views are not visible to the app */
    server_leave_uninterrupted_section( &csVirtual, &sigset );
    return ret;
}


/***********************************************************************
 *           virtual_handle_stack_fault
 *
 * Handle an access fault inside the current thread stack.
 * Called from inside a signal handler.
 */
BOOL virtual_handle_stack_fault( void *addr )
{
    struct file_view *view;
    BOOL ret = FALSE;

    RtlEnterCriticalSection( &csVirtual );  /* no need for signal masking inside signal handler */
    if ((view = VIRTUAL_FindView( addr, 0 )))
    {
        void *page = ROUND_ADDR( addr, page_mask );
        BYTE vprot = view->prot[((const char *)page - (const char *)view->base) >> page_shift];
        if (vprot & VPROT_GUARD)
        {
            VIRTUAL_SetProt( view, page, page_size, vprot & ~VPROT_GUARD );
            NtCurrentTeb()->Tib.StackLimit = page;
            if ((char *)page >= (char *)NtCurrentTeb()->DeallocationStack + 2*page_size)
            {
                vprot = view->prot[((char *)page - page_size - (char *)view->base) >> page_shift];
                VIRTUAL_SetProt( view, (char *)page - page_size, page_size, vprot | VPROT_GUARD );
            }
            ret = TRUE;
        }
    }
    RtlLeaveCriticalSection( &csVirtual );
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
    __EXCEPT_PAGE_FAULT
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
    __EXCEPT_PAGE_FAULT
    {
        return FALSE;
    }
    __ENDTRY
    return TRUE;
}


/***********************************************************************
 *           VIRTUAL_SetForceExec
 *
 * Whether to force exec prot on all views.
 */
void VIRTUAL_SetForceExec( BOOL enable )
{
    struct file_view *view;
    sigset_t sigset;

    server_enter_uninterrupted_section( &csVirtual, &sigset );
    if (!force_exec_prot != !enable)  /* change all existing views */
    {
        force_exec_prot = enable;

        LIST_FOR_EACH_ENTRY( view, &views_list, struct file_view, entry )
        {
            UINT i, count;
            char *addr = view->base;
            BYTE commit = view->mapping ? VPROT_COMMITTED : 0;  /* file mappings are always accessible */
            int unix_prot = VIRTUAL_GetUnixProt( view->prot[0] | commit );

            if (view->protect & VPROT_NOEXEC) continue;
            for (count = i = 1; i < view->size >> page_shift; i++, count++)
            {
                int prot = VIRTUAL_GetUnixProt( view->prot[i] | commit );
                if (prot == unix_prot) continue;
                if ((unix_prot & PROT_READ) && !(unix_prot & PROT_EXEC))
                {
                    TRACE( "%s exec prot for %p-%p\n",
                           force_exec_prot ? "enabling" : "disabling",
                           addr, addr + (count << page_shift) - 1 );
                    mprotect( addr, count << page_shift,
                              unix_prot | (force_exec_prot ? PROT_EXEC : 0) );
                }
                addr += (count << page_shift);
                unix_prot = prot;
                count = 0;
            }
            if (count)
            {
                if ((unix_prot & PROT_READ) && !(unix_prot & PROT_EXEC))
                {
                    TRACE( "%s exec prot for %p-%p\n",
                           force_exec_prot ? "enabling" : "disabling",
                           addr, addr + (count << page_shift) - 1 );
                    mprotect( addr, count << page_shift,
                              unix_prot | (force_exec_prot ? PROT_EXEC : 0) );
                }
            }
        }
    }
    server_leave_uninterrupted_section( &csVirtual, &sigset );
}

struct free_range
{
    char *base;
    char *limit;
};

/* free reserved areas above the limit; callback for wine_mmap_enum_reserved_areas */
static int free_reserved_memory( void *base, size_t size, void *arg )
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
void virtual_release_address_space(void)
{
    struct free_range range;
    sigset_t sigset;

    if (is_win64) return;

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    range.base  = (char *)0x82000000;
    range.limit = user_space_limit;

    if (range.limit > range.base)
    {
        while (wine_mmap_enum_reserved_areas( free_reserved_memory, &range, 1 )) /* nothing */;
    }
    else
    {
#ifndef __APPLE__  /* dyld doesn't support parts of the WINE_DOS segment being unmapped */
        range.base  = (char *)0x20000000;
        range.limit = (char *)0x7f000000;
        while (wine_mmap_enum_reserved_areas( free_reserved_memory, &range, 0 )) /* nothing */;
#endif
    }

    server_leave_uninterrupted_section( &csVirtual, &sigset );
}


/***********************************************************************
 *           virtual_set_large_address_space
 *
 * Enable use of a large address space when allowed by the application.
 */
void virtual_set_large_address_space(void)
{
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );

    if (!(nt->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)) return;
    /* no large address space on win9x */
    if (NtCurrentTeb()->Peb->OSPlatformId != VER_PLATFORM_WIN32_NT) return;

    user_space_limit = working_set_limit = address_space_limit;
}


/***********************************************************************
 *             NtAllocateVirtualMemory   (NTDLL.@)
 *             ZwAllocateVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtAllocateVirtualMemory( HANDLE process, PVOID *ret, ULONG zero_bits,
                                         SIZE_T *size_ptr, ULONG type, ULONG protect )
{
    void *base;
    unsigned int vprot;
    SIZE_T size = *size_ptr;
    SIZE_T mask = get_mask( zero_bits );
    NTSTATUS status = STATUS_SUCCESS;
    struct file_view *view;
    sigset_t sigset;

    TRACE("%p %p %08lx %x %08x\n", process, *ret, size, type, protect );

    if (!size) return STATUS_INVALID_PARAMETER;

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.virtual_alloc.type      = APC_VIRTUAL_ALLOC;
        call.virtual_alloc.addr      = wine_server_client_ptr( *ret );
        call.virtual_alloc.size      = *size_ptr;
        call.virtual_alloc.zero_bits = zero_bits;
        call.virtual_alloc.op_type   = type;
        call.virtual_alloc.prot      = protect;
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

    if ((status = get_vprot_flags( protect, &vprot, FALSE ))) return status;
    if (vprot & VPROT_WRITECOPY) return STATUS_INVALID_PAGE_PROTECTION;
    vprot |= VPROT_VALLOC;
    if (type & MEM_COMMIT) vprot |= VPROT_COMMITTED;

    if (*ret)
    {
        if (type & MEM_RESERVE) /* Round down to 64k boundary */
            base = ROUND_ADDR( *ret, mask );
        else
            base = ROUND_ADDR( *ret, page_mask );
        size = (((UINT_PTR)*ret + size + page_mask) & ~page_mask) - (UINT_PTR)base;

        /* address 1 is magic to mean DOS area */
        if (!base && *ret == (void *)1 && size == 0x110000)
        {
            server_enter_uninterrupted_section( &csVirtual, &sigset );
            status = allocate_dos_memory( &view, vprot );
            if (status == STATUS_SUCCESS)
            {
                *ret = view->base;
                *size_ptr = view->size;
            }
            server_leave_uninterrupted_section( &csVirtual, &sigset );
            return status;
        }

        /* disallow low 64k, wrap-around and kernel space */
        if (((char *)base < (char *)0x10000) ||
            ((char *)base + size < (char *)base) ||
            is_beyond_limit( base, size, address_space_limit ))
            return STATUS_INVALID_PARAMETER;
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

    if (use_locks) server_enter_uninterrupted_section( &csVirtual, &sigset );

    if ((type & MEM_RESERVE) || !base)
    {
        if (type & MEM_WRITE_WATCH) vprot |= VPROT_WRITEWATCH;
        status = map_view( &view, base, size, mask, type & MEM_TOP_DOWN, vprot );
        if (status == STATUS_SUCCESS) base = view->base;
    }
    else if (type & MEM_RESET)
    {
        if (!(view = VIRTUAL_FindView( base, size ))) status = STATUS_NOT_MAPPED_VIEW;
        else madvise( base, size, MADV_DONTNEED );
    }
    else  /* commit the pages */
    {
        if (!(view = VIRTUAL_FindView( base, size ))) status = STATUS_NOT_MAPPED_VIEW;
        else if (view->mapping && (view->protect & VPROT_COMMITTED)) status = STATUS_ALREADY_COMMITTED;
        else if (!VIRTUAL_SetProt( view, base, size, vprot )) status = STATUS_ACCESS_DENIED;
        else if (view->mapping && !(view->protect & VPROT_COMMITTED))
        {
            SERVER_START_REQ( add_mapping_committed_range )
            {
                req->handle = wine_server_obj_handle( view->mapping );
                req->offset = (char *)base - (char *)view->base;
                req->size   = size;
                wine_server_call( req );
            }
            SERVER_END_REQ;
        }
    }

    if (use_locks) server_leave_uninterrupted_section( &csVirtual, &sigset );

    if (status == STATUS_SUCCESS)
    {
        *ret = base;
        *size_ptr = size;
    }
    return status;
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

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    if (!(view = VIRTUAL_FindView( base, size )) || !(view->protect & VPROT_VALLOC))
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

    server_leave_uninterrupted_section( &csVirtual, &sigset );
    return status;
}

static ULONG map_protection_to_access( ULONG vprot )
{
    vprot &= VPROT_READ | VPROT_WRITE | VPROT_EXEC | VPROT_WRITECOPY;
    if (vprot & VPROT_EXEC)
    {
        if (vprot & VPROT_WRITE) vprot |= VPROT_WRITECOPY;
    }
    else vprot &= ~VPROT_WRITECOPY;
    return vprot;
}

static BOOL is_compatible_protection( const struct file_view *view, ULONG new_prot )
{
    ULONG view_prot, map_prot;

    view_prot = map_protection_to_access( view->protect );
    new_prot = map_protection_to_access( new_prot );

    if (view_prot == new_prot) return TRUE;
    if (!view_prot) return FALSE;

    if ((view_prot & new_prot) != new_prot) return FALSE;

    map_prot = map_protection_to_access( view->map_protect );
    if ((map_prot & new_prot) == new_prot) return TRUE;

    return FALSE;
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
    unsigned int new_vprot;
    SIZE_T size = *size_ptr;
    LPVOID addr = *addr_ptr;

    TRACE("%p %p %08lx %08x\n", process, addr, size, new_prot );

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
            if (old_prot) *old_prot = result.virtual_protect.prot;
        }
        return result.virtual_protect.status;
    }

    /* Fix the parameters */

    size = ROUND_SIZE( addr, size );
    base = ROUND_ADDR( addr, page_mask );

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    if ((view = VIRTUAL_FindView( base, size )))
    {
        /* Make sure all the pages are committed */
        if (get_committed_size( view, base, &vprot ) >= size && (vprot & VPROT_COMMITTED))
        {
            if (!(status = get_vprot_flags( new_prot, &new_vprot, view->protect & VPROT_IMAGE )))
            {
                if ((new_vprot & VPROT_WRITECOPY) && (view->protect & VPROT_VALLOC))
                    status = STATUS_INVALID_PAGE_PROTECTION;
                else
                {
                    if (!view->mapping || is_compatible_protection( view, new_vprot ))
                    {
                        new_vprot |= VPROT_COMMITTED;
                        if (old_prot) *old_prot = VIRTUAL_GetWin32Prot( vprot );
                        if (!VIRTUAL_SetProt( view, base, size, new_vprot )) status = STATUS_ACCESS_DENIED;
                    }
                    else status = STATUS_INVALID_PAGE_PROTECTION;
                }
            }
        }
        else status = STATUS_NOT_COMMITTED;
    }
    else status = STATUS_INVALID_PARAMETER;

    server_leave_uninterrupted_section( &csVirtual, &sigset );

    if (status == STATUS_SUCCESS)
    {
        *addr_ptr = base;
        *size_ptr = size;
    }
    return status;
}


/* retrieve state for a free memory area; callback for wine_mmap_enum_reserved_areas */
static int get_free_mem_state_callback( void *start, size_t size, void *arg )
{
    MEMORY_BASIC_INFORMATION *info = arg;
    void *end = (char *)start + size;

    if ((char *)info->BaseAddress + info->RegionSize < (char *)start) return 0;

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
        info->State             = MEM_RESERVE;
        info->Protect           = PAGE_NOACCESS;
        info->AllocationProtect = PAGE_NOACCESS;
        info->Type              = MEM_PRIVATE;
    }
    return 1;
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
                                      MEMORY_INFORMATION_CLASS info_class, PVOID buffer,
                                      SIZE_T len, SIZE_T *res_len )
{
    struct file_view *view;
    char *base, *alloc_base = 0;
    struct list *ptr;
    SIZE_T size = 0;
    MEMORY_BASIC_INFORMATION *info = buffer;
    sigset_t sigset;

    if (info_class != MemoryBasicInformation)
    {
        switch(info_class)
        {
            UNIMPLEMENTED_INFO_CLASS(MemoryWorkingSetList);
            UNIMPLEMENTED_INFO_CLASS(MemorySectionName);
            UNIMPLEMENTED_INFO_CLASS(MemoryBasicVlmInformation);

            default:
                FIXME("(%p,%p,info_class=%d,%p,%ld,%p) Unknown information class\n", 
                      process, addr, info_class, buffer, len, res_len);
                return STATUS_INVALID_INFO_CLASS;
        }
    }

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

    if (is_beyond_limit( base, 1, working_set_limit )) return STATUS_WORKING_SET_LIMIT_RANGE;

    /* Find the view containing the address */

    server_enter_uninterrupted_section( &csVirtual, &sigset );
    ptr = list_head( &views_list );
    for (;;)
    {
        if (!ptr)
        {
            size = (char *)working_set_limit - alloc_base;
            view = NULL;
            break;
        }
        view = LIST_ENTRY( ptr, struct file_view, entry );
        if ((char *)view->base > base)
        {
            size = (char *)view->base - alloc_base;
            view = NULL;
            break;
        }
        if ((char *)view->base + view->size > base)
        {
            alloc_base = view->base;
            size = view->size;
            break;
        }
        alloc_base = (char *)view->base + view->size;
        ptr = list_next( &views_list, ptr );
    }

    /* Fill the info structure */

    info->AllocationBase = alloc_base;
    info->BaseAddress    = base;
    info->RegionSize     = size - (base - alloc_base);

    if (!view)
    {
        if (!wine_mmap_enum_reserved_areas( get_free_mem_state_callback, info, 0 ))
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
        SIZE_T range_size = get_committed_size( view, base, &vprot );

        info->State = (vprot & VPROT_COMMITTED) ? MEM_COMMIT : MEM_RESERVE;
        info->Protect = (vprot & VPROT_COMMITTED) ? VIRTUAL_GetWin32Prot( vprot ) : 0;
        info->AllocationBase = alloc_base;
        info->AllocationProtect = VIRTUAL_GetWin32Prot( view->protect );
        if (view->protect & VPROT_IMAGE) info->Type = MEM_IMAGE;
        else if (view->protect & VPROT_VALLOC) info->Type = MEM_PRIVATE;
        else info->Type = MEM_MAPPED;
        for (size = base - alloc_base; size < base + range_size - alloc_base; size += page_size)
            if ((view->prot[size >> page_shift] ^ vprot) & ~VPROT_WRITEWATCH) break;
        info->RegionSize = size - (base - alloc_base);
    }
    server_leave_uninterrupted_section( &csVirtual, &sigset );

    if (res_len) *res_len = sizeof(*info);
    return STATUS_SUCCESS;
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
 *             NtCreateSection   (NTDLL.@)
 *             ZwCreateSection   (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateSection( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                 const LARGE_INTEGER *size, ULONG protect,
                                 ULONG sec_flags, HANDLE file )
{
    NTSTATUS ret;
    unsigned int vprot;
    DWORD len = (attr && attr->ObjectName) ? attr->ObjectName->Length : 0;
    struct security_descriptor *sd = NULL;
    struct object_attributes objattr;

    /* Check parameters */

    if (len > MAX_PATH*sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    if ((ret = get_vprot_flags( protect, &vprot, sec_flags & SEC_IMAGE ))) return ret;

    objattr.rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
    objattr.sd_len = 0;
    objattr.name_len = len;
    if (attr)
    {
        ret = NTDLL_create_struct_sd( attr->SecurityDescriptor, &sd, &objattr.sd_len );
        if (ret != STATUS_SUCCESS) return ret;
    }

    if (!(sec_flags & SEC_RESERVE)) vprot |= VPROT_COMMITTED;
    if (sec_flags & SEC_NOCACHE) vprot |= VPROT_NOCACHE;
    if (sec_flags & SEC_IMAGE) vprot |= VPROT_IMAGE;

    /* Create the server object */

    SERVER_START_REQ( create_mapping )
    {
        req->access      = access;
        req->attributes  = (attr) ? attr->Attributes : 0;
        req->file_handle = wine_server_obj_handle( file );
        req->size        = size ? size->QuadPart : 0;
        req->protect     = vprot;
        wine_server_add_data( req, &objattr, sizeof(objattr) );
        if (objattr.sd_len) wine_server_add_data( req, sd, objattr.sd_len );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    NTDLL_free_struct_sd( sd );

    return ret;
}


/***********************************************************************
 *             NtOpenSection   (NTDLL.@)
 *             ZwOpenSection   (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenSection( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;
    DWORD len = attr->ObjectName->Length;

    if (len > MAX_PATH*sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( open_mapping )
    {
        req->access  = access;
        req->attributes = attr->Attributes;
        req->rootdir = wine_server_obj_handle( attr->RootDirectory );
        wine_server_add_data( req, attr->ObjectName->Buffer, len );
        if (!(ret = wine_server_call( req ))) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *             NtMapViewOfSection   (NTDLL.@)
 *             ZwMapViewOfSection   (NTDLL.@)
 */
NTSTATUS WINAPI NtMapViewOfSection( HANDLE handle, HANDLE process, PVOID *addr_ptr, ULONG zero_bits,
                                    SIZE_T commit_size, const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                    SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect )
{
    NTSTATUS res;
    mem_size_t full_size;
    ACCESS_MASK access;
    SIZE_T size, mask = get_mask( zero_bits );
    int unix_handle = -1, needs_close;
    unsigned int map_vprot, vprot;
    void *base;
    struct file_view *view;
    DWORD header_size;
    HANDLE dup_mapping, shared_file;
    LARGE_INTEGER offset;
    sigset_t sigset;

    offset.QuadPart = offset_ptr ? offset_ptr->QuadPart : 0;

    TRACE("handle=%p process=%p addr=%p off=%x%08x size=%lx access=%x\n",
          handle, process, *addr_ptr, offset.u.HighPart, offset.u.LowPart, *size_ptr, protect );

    /* Check parameters */

    if ((offset.u.LowPart & mask) || (*addr_ptr && ((UINT_PTR)*addr_ptr & mask)))
        return STATUS_INVALID_PARAMETER;

    switch(protect)
    {
    case PAGE_NOACCESS:
        access = SECTION_MAP_READ;
        break;
    case PAGE_READWRITE:
    case PAGE_EXECUTE_READWRITE:
        access = SECTION_MAP_WRITE;
        break;
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_WRITECOPY:
        access = SECTION_MAP_READ;
        break;
    default:
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    if (process != NtCurrentProcess())
    {
        apc_call_t call;
        apc_result_t result;

        memset( &call, 0, sizeof(call) );

        call.map_view.type        = APC_MAP_VIEW;
        call.map_view.handle      = wine_server_obj_handle( handle );
        call.map_view.addr        = wine_server_client_ptr( *addr_ptr );
        call.map_view.size        = *size_ptr;
        call.map_view.offset      = offset.QuadPart;
        call.map_view.zero_bits   = zero_bits;
        call.map_view.alloc_type  = alloc_type;
        call.map_view.prot        = protect;
        res = server_queue_process_apc( process, &call, &result );
        if (res != STATUS_SUCCESS) return res;

        if ((NTSTATUS)result.map_view.status >= 0)
        {
            *addr_ptr = wine_server_get_ptr( result.map_view.addr );
            *size_ptr = result.map_view.size;
        }
        return result.map_view.status;
    }

    SERVER_START_REQ( get_mapping_info )
    {
        req->handle = wine_server_obj_handle( handle );
        req->access = access;
        res = wine_server_call( req );
        map_vprot   = reply->protect;
        base        = wine_server_get_ptr( reply->base );
        full_size   = reply->size;
        header_size = reply->header_size;
        dup_mapping = wine_server_ptr_handle( reply->mapping );
        shared_file = wine_server_ptr_handle( reply->shared_file );
        if ((ULONG_PTR)base != reply->base) base = NULL;
    }
    SERVER_END_REQ;
    if (res) return res;

    if ((res = server_get_unix_fd( handle, 0, &unix_handle, &needs_close, NULL, NULL ))) goto done;

    if (map_vprot & VPROT_IMAGE)
    {
        size = full_size;
        if (size != full_size)  /* truncated */
        {
            WARN( "Modules larger than 4Gb (%s) not supported\n", wine_dbgstr_longlong(full_size) );
            res = STATUS_INVALID_PARAMETER;
            goto done;
        }
        if (shared_file)
        {
            int shared_fd, shared_needs_close;

            if ((res = server_get_unix_fd( shared_file, FILE_READ_DATA|FILE_WRITE_DATA,
                                           &shared_fd, &shared_needs_close, NULL, NULL ))) goto done;
            res = map_image( handle, unix_handle, base, size, mask, header_size,
                             shared_fd, dup_mapping, map_vprot, addr_ptr );
            if (shared_needs_close) close( shared_fd );
            NtClose( shared_file );
        }
        else
        {
            res = map_image( handle, unix_handle, base, size, mask, header_size,
                             -1, dup_mapping, map_vprot, addr_ptr );
        }
        if (needs_close) close( unix_handle );
        if (res >= 0) *size_ptr = size;
        return res;
    }

    res = STATUS_INVALID_PARAMETER;
    if (offset.QuadPart >= full_size) goto done;
    if (*size_ptr)
    {
        if (*size_ptr > full_size - offset.QuadPart) goto done;
        size = ROUND_SIZE( offset.u.LowPart, *size_ptr );
        if (size < *size_ptr) goto done;  /* wrap-around */
    }
    else
    {
        size = full_size - offset.QuadPart;
        if (size != full_size - offset.QuadPart)  /* truncated */
        {
            WARN( "Files larger than 4Gb (%s) not supported on this platform\n",
                  wine_dbgstr_longlong(full_size) );
            goto done;
        }
    }

    /* Reserve a properly aligned area */

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    get_vprot_flags( protect, &vprot, map_vprot & VPROT_IMAGE );
    vprot |= (map_vprot & VPROT_COMMITTED);
    res = map_view( &view, *addr_ptr, size, mask, FALSE, vprot );
    if (res)
    {
        server_leave_uninterrupted_section( &csVirtual, &sigset );
        goto done;
    }

    /* Map the file */

    TRACE("handle=%p size=%lx offset=%x%08x\n",
          handle, size, offset.u.HighPart, offset.u.LowPart );

    res = map_file_into_view( view, unix_handle, 0, size, offset.QuadPart, vprot, !dup_mapping );
    if (res == STATUS_SUCCESS)
    {
        *addr_ptr = view->base;
        *size_ptr = size;
        view->mapping = dup_mapping;
        view->map_protect = map_vprot;
        dup_mapping = 0;  /* don't close it */
    }
    else
    {
        ERR( "map_file_into_view %p %lx %x%08x failed\n",
             view->base, size, offset.u.HighPart, offset.u.LowPart );
        delete_view( view );
    }

    server_leave_uninterrupted_section( &csVirtual, &sigset );

done:
    if (dup_mapping) NtClose( dup_mapping );
    if (needs_close) close( unix_handle );
    return res;
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
    void *base = ROUND_ADDR( addr, page_mask );

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

    server_enter_uninterrupted_section( &csVirtual, &sigset );
    if ((view = VIRTUAL_FindView( base, 0 )) && (base == view->base) && !(view->protect & VPROT_VALLOC))
    {
        delete_view( view );
        status = STATUS_SUCCESS;
    }
    server_leave_uninterrupted_section( &csVirtual, &sigset );
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

    server_enter_uninterrupted_section( &csVirtual, &sigset );
    if (!(view = VIRTUAL_FindView( addr, *size_ptr ))) status = STATUS_INVALID_PARAMETER;
    else
    {
        if (!*size_ptr) *size_ptr = view->size;
        *addr_ptr = addr;
#ifdef MS_ASYNC
        if (msync( addr, *size_ptr, MS_ASYNC )) status = STATUS_NOT_MAPPED_DATA;
#endif
    }
    server_leave_uninterrupted_section( &csVirtual, &sigset );
    return status;
}


/***********************************************************************
 *             NtGetWriteWatch   (NTDLL.@)
 *             ZwGetWriteWatch   (NTDLL.@)
 */
NTSTATUS WINAPI NtGetWriteWatch( HANDLE process, ULONG flags, PVOID base, SIZE_T size, PVOID *addresses,
                                 ULONG_PTR *count, ULONG *granularity )
{
    struct file_view *view;
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

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    if ((view = VIRTUAL_FindView( base, size )) && (view->protect & VPROT_WRITEWATCH))
    {
        ULONG_PTR pos = 0;
        char *addr = base;
        char *end = addr + size;

        while (pos < *count && addr < end)
        {
            BYTE prot = view->prot[(addr - (char *)view->base) >> page_shift];
            if (!(prot & VPROT_WRITEWATCH)) addresses[pos++] = addr;
            addr += page_size;
        }
        if (flags & WRITE_WATCH_FLAG_RESET) reset_write_watches( view, base, addr - (char *)base );
        *count = pos;
        *granularity = page_size;
    }
    else status = STATUS_INVALID_PARAMETER;

    server_leave_uninterrupted_section( &csVirtual, &sigset );
    return status;
}


/***********************************************************************
 *             NtResetWriteWatch   (NTDLL.@)
 *             ZwResetWriteWatch   (NTDLL.@)
 */
NTSTATUS WINAPI NtResetWriteWatch( HANDLE process, PVOID base, SIZE_T size )
{
    struct file_view *view;
    NTSTATUS status = STATUS_SUCCESS;
    sigset_t sigset;

    size = ROUND_SIZE( base, size );
    base = ROUND_ADDR( base, page_mask );

    TRACE( "%p %p-%p\n", process, base, (char *)base + size );

    if (!size) return STATUS_INVALID_PARAMETER;

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    if ((view = VIRTUAL_FindView( base, size )) && (view->protect & VPROT_WRITEWATCH))
        reset_write_watches( view, base, size );
    else
        status = STATUS_INVALID_PARAMETER;

    server_leave_uninterrupted_section( &csVirtual, &sigset );
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
    struct stat st1, st2;
    NTSTATUS status;
    sigset_t sigset;

    TRACE("%p %p\n", addr1, addr2);

    server_enter_uninterrupted_section( &csVirtual, &sigset );

    view1 = VIRTUAL_FindView( addr1, 0 );
    view2 = VIRTUAL_FindView( addr2, 0 );

    if (!view1 || !view2)
        status = STATUS_INVALID_ADDRESS;
    else if ((view1->protect & VPROT_VALLOC) || (view2->protect & VPROT_VALLOC))
        status = STATUS_CONFLICTING_ADDRESSES;
    else if (view1 == view2)
        status = STATUS_SUCCESS;
    else if (!(view1->protect & VPROT_IMAGE) || !(view2->protect & VPROT_IMAGE))
        status = STATUS_NOT_SAME_DEVICE;
    else if (!stat_mapping_file( view1, &st1 ) && !stat_mapping_file( view2, &st2 ) &&
             st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
        status = STATUS_SUCCESS;
    else
        status = STATUS_NOT_SAME_DEVICE;

    server_leave_uninterrupted_section( &csVirtual, &sigset );
    return status;
}
