/*
 * Win32 virtual memory functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include "winbase.h"
#include "winerror.h"
#include "file.h"
#include "process.h"
#include "global.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(virtual);

#ifndef MS_SYNC
#define MS_SYNC 0
#endif

/* File view */
typedef struct _FV
{
    struct _FV   *next;        /* Next view */
    struct _FV   *prev;        /* Prev view */
    UINT        base;        /* Base address */
    UINT        size;        /* Size in bytes */
    UINT        flags;       /* Allocation flags */
    UINT        offset;      /* Offset from start of mapped file */
    HANDLE      mapping;     /* Handle to the file mapping */
    HANDLERPROC   handlerProc; /* Fault handler */
    LPVOID        handlerArg;  /* Fault handler argument */
    BYTE          protect;     /* Protection for all pages at allocation time */
    BYTE          prot[1];     /* Protection byte for each page */
} FILE_VIEW;

/* Per-view flags */
#define VFLAG_SYSTEM     0x01

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


static FILE_VIEW *VIRTUAL_FirstView;

#ifdef __i386__
/* These are always the same on an i386, and it will be faster this way */
# define page_mask  0xfff
# define page_shift 12
# define granularity_mask 0xffff
#else
static UINT page_shift;
static UINT page_mask;
static UINT granularity_mask;  /* Allocation granularity (usually 64k) */
#endif  /* __i386__ */

#define ROUND_ADDR(addr) \
   ((UINT)(addr) & ~page_mask)

#define ROUND_SIZE(addr,size) \
   (((UINT)(size) + ((UINT)(addr) & page_mask) + page_mask) & ~page_mask)

#define VIRTUAL_DEBUG_DUMP_VIEW(view) \
   if (!TRACE_ON(virtual)); else VIRTUAL_DumpView(view)

/***********************************************************************
 *           VIRTUAL_GetProtStr
 */
static const char *VIRTUAL_GetProtStr( BYTE prot )
{
    static char buffer[6];
    buffer[0] = (prot & VPROT_COMMITTED) ? 'c' : '-';
    buffer[1] = (prot & VPROT_GUARD) ? 'g' : '-';
    buffer[2] = (prot & VPROT_READ) ? 'r' : '-';
    buffer[3] = (prot & VPROT_WRITE) ?
                    ((prot & VPROT_WRITECOPY) ? 'w' : 'W') : '-';
    buffer[4] = (prot & VPROT_EXEC) ? 'x' : '-';
    buffer[5] = 0;
    return buffer;
}


/***********************************************************************
 *           VIRTUAL_DumpView
 */
static void VIRTUAL_DumpView( FILE_VIEW *view )
{
    UINT i, count;
    UINT addr = view->base;
    BYTE prot = view->prot[0];

    DPRINTF( "View: %08x - %08x%s",
	  view->base, view->base + view->size - 1,
	  (view->flags & VFLAG_SYSTEM) ? " (system)" : "" );
    if (view->mapping)
        DPRINTF( " %d @ %08x\n", view->mapping, view->offset );
    else
        DPRINTF( " (anonymous)\n");

    for (count = i = 1; i < view->size >> page_shift; i++, count++)
    {
        if (view->prot[i] == prot) continue;
        DPRINTF( "      %08x - %08x %s\n",
	      addr, addr + (count << page_shift) - 1,
	      VIRTUAL_GetProtStr(prot) );
        addr += (count << page_shift);
        prot = view->prot[i];
        count = 0;
    }
    if (count)
        DPRINTF( "      %08x - %08x %s\n",
	      addr, addr + (count << page_shift) - 1,
	      VIRTUAL_GetProtStr(prot) );
}


/***********************************************************************
 *           VIRTUAL_Dump
 */
void VIRTUAL_Dump(void)
{
    FILE_VIEW *view = VIRTUAL_FirstView;
    DPRINTF( "\nDump of all virtual memory views:\n\n" );
    while (view)
    {
        VIRTUAL_DumpView( view );
        view = view->next;
    }
}


/***********************************************************************
 *           VIRTUAL_FindView
 *
 * Find the view containing a given address.
 *
 * RETURNS
 *	View: Success
 *	NULL: Failure
 */
static FILE_VIEW *VIRTUAL_FindView(
                  UINT addr /* [in] Address */
) {
    FILE_VIEW *view = VIRTUAL_FirstView;
    while (view)
    {
        if (view->base > addr) return NULL;
        if (view->base + view->size > addr) return view;
        view = view->next;
    }
    return NULL;
}


/***********************************************************************
 *           VIRTUAL_CreateView
 *
 * Create a new view and add it in the linked list.
 */
static FILE_VIEW *VIRTUAL_CreateView( UINT base, UINT size, UINT offset,
                                      UINT flags, BYTE vprot,
                                      HANDLE mapping )
{
    FILE_VIEW *view, *prev;

    /* Create the view structure */

    assert( !(base & page_mask) );
    assert( !(size & page_mask) );
    size >>= page_shift;
    if (!(view = (FILE_VIEW *)malloc( sizeof(*view) + size - 1 ))) return NULL;
    view->base    = base;
    view->size    = size << page_shift;
    view->flags   = flags;
    view->offset  = offset;
    view->mapping = mapping;
    view->protect = vprot;
    view->handlerProc = NULL;
    memset( view->prot, vprot, size );

    /* Duplicate the mapping handle */

    if ((view->mapping != -1) &&
        !DuplicateHandle( GetCurrentProcess(), view->mapping,
                          GetCurrentProcess(), &view->mapping,
                          0, FALSE, DUPLICATE_SAME_ACCESS ))
    {
        free( view );
        return NULL;
    }

    /* Insert it in the linked list */

    if (!VIRTUAL_FirstView || (VIRTUAL_FirstView->base > base))
    {
        view->next = VIRTUAL_FirstView;
        view->prev = NULL;
        if (view->next) view->next->prev = view;
        VIRTUAL_FirstView = view;
    }
    else
    {
        prev = VIRTUAL_FirstView;
        while (prev->next && (prev->next->base < base)) prev = prev->next;
        view->next = prev->next;
        view->prev = prev;
        if (view->next) view->next->prev = view;
        prev->next  = view;
    }
    VIRTUAL_DEBUG_DUMP_VIEW( view );
    return view;
}


/***********************************************************************
 *           VIRTUAL_DeleteView
 * Deletes a view.
 *
 * RETURNS
 *	None
 */
static void VIRTUAL_DeleteView(
            FILE_VIEW *view /* [in] View */
) {
    if (!(view->flags & VFLAG_SYSTEM))
        FILE_munmap( (void *)view->base, 0, view->size );
    if (view->next) view->next->prev = view->prev;
    if (view->prev) view->prev->next = view->next;
    else VIRTUAL_FirstView = view->next;
    if (view->mapping) CloseHandle( view->mapping );
    free( view );
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
        if (vprot & VPROT_WRITE) prot |= PROT_WRITE;
        if (vprot & VPROT_WRITECOPY) prot |= PROT_WRITE;
        if (vprot & VPROT_EXEC) prot |= PROT_EXEC;
    }
    return prot;
}


/***********************************************************************
 *           VIRTUAL_GetWin32Prot
 *
 * Convert page protections to Win32 flags.
 *
 * RETURNS
 *	None
 */
static void VIRTUAL_GetWin32Prot(
            BYTE vprot,     /* [in] Page protection flags */
            DWORD *protect, /* [out] Location to store Win32 protection flags */
            DWORD *state    /* [out] Location to store mem state flag */
) {
    if (protect) {
    	*protect = VIRTUAL_Win32Flags[vprot & 0x0f];
/*    	if (vprot & VPROT_GUARD) *protect |= PAGE_GUARD;*/
    	if (vprot & VPROT_NOCACHE) *protect |= PAGE_NOCACHE;

    	if (vprot & VPROT_GUARD) *protect = PAGE_NOACCESS;
    }

    if (state) *state = (vprot & VPROT_COMMITTED) ? MEM_COMMIT : MEM_RESERVE;
}


/***********************************************************************
 *           VIRTUAL_GetProt
 *
 * Build page protections from Win32 flags.
 *
 * RETURNS
 *	Value of page protection flags
 */
static BYTE VIRTUAL_GetProt(
            DWORD protect  /* [in] Win32 protection flags */
) {
    BYTE vprot;

    switch(protect & 0xff)
    {
    case PAGE_READONLY:
        vprot = VPROT_READ;
        break;
    case PAGE_READWRITE:
        vprot = VPROT_READ | VPROT_WRITE;
        break;
    case PAGE_WRITECOPY:
        vprot = VPROT_READ | VPROT_WRITE | VPROT_WRITECOPY;
        break;
    case PAGE_EXECUTE:
        vprot = VPROT_EXEC;
        break;
    case PAGE_EXECUTE_READ:
        vprot = VPROT_EXEC | VPROT_READ;
        break;
    case PAGE_EXECUTE_READWRITE:
        vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITE;
        break;
    case PAGE_EXECUTE_WRITECOPY:
        vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITE | VPROT_WRITECOPY;
        break;
    case PAGE_NOACCESS:
    default:
        vprot = 0;
        break;
    }
    if (protect & PAGE_GUARD) vprot |= VPROT_GUARD;
    if (protect & PAGE_NOCACHE) vprot |= VPROT_NOCACHE;
    return vprot;
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
static BOOL VIRTUAL_SetProt(
              FILE_VIEW *view, /* [in] Pointer to view */
              UINT base,     /* [in] Starting address */
              UINT size,     /* [in] Size in bytes */
              BYTE vprot       /* [in] Protections to use */
) {
    TRACE("%08x-%08x %s\n",
                     base, base + size - 1, VIRTUAL_GetProtStr( vprot ) );

    if (mprotect( (void *)base, size, VIRTUAL_GetUnixProt(vprot) ))
        return FALSE;  /* FIXME: last error */

    memset( view->prot + ((base - view->base) >> page_shift),
            vprot, size >> page_shift );
    VIRTUAL_DEBUG_DUMP_VIEW( view );
    return TRUE;
}


/***********************************************************************
 *             VIRTUAL_CheckFlags
 *
 * Check that all pages in a range have the given flags.
 *
 * RETURNS
 *	TRUE: They do
 *	FALSE: They do not
 */
static BOOL VIRTUAL_CheckFlags(
              UINT base, /* [in] Starting address */
              UINT size, /* [in] Size in bytes */
              BYTE flags   /* [in] Flags to check for */
) {
    FILE_VIEW *view;
    UINT page;

    if (!size) return TRUE;
    if (!(view = VIRTUAL_FindView( base ))) return FALSE;
    if (view->base + view->size < base + size) return FALSE;
    page = (base - view->base) >> page_shift;
    size = ROUND_SIZE( base, size ) >> page_shift;
    while (size--) if ((view->prot[page++] & flags) != flags) return FALSE;
    return TRUE;
}


/***********************************************************************
 *           VIRTUAL_Init
 */
BOOL VIRTUAL_Init(void)
{
#ifndef __i386__
    DWORD page_size;

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
    granularity_mask = 0xffff;  /* hard-coded for now */
    /* Make sure we have a power of 2 */
    assert( !(page_size & page_mask) );
    page_shift = 0;
    while ((1 << page_shift) != page_size) page_shift++;
#endif  /* !__i386__ */

#ifdef linux
    {
        /* Do not use stdio here since it may temporarily change the size
         * of some segments (ie libc6 adds 0x1000 per open FILE)
         */
        int fd = open ("/proc/self/maps", O_RDONLY);
        if (fd >= 0)
        {
            char buffer[512]; /* line might be rather long in 2.1 */

            for (;;)
            {
                int start, end, offset;
                char r, w, x, p;
                BYTE vprot = VPROT_COMMITTED;

                char * ptr = buffer;
                int count = sizeof(buffer);
                while (1 == read(fd, ptr, 1) && *ptr != '\n' && --count > 0)
                    ptr++;

                if (*ptr != '\n') break;
                *ptr = '\0';

                sscanf( buffer, "%x-%x %c%c%c%c %x",
                        &start, &end, &r, &w, &x, &p, &offset );
                if (r == 'r') vprot |= VPROT_READ;
                if (w == 'w') vprot |= VPROT_WRITE;
                if (x == 'x') vprot |= VPROT_EXEC;
                if (p == 'p') vprot |= VPROT_WRITECOPY;
                VIRTUAL_CreateView( start, end - start, 0,
                                    VFLAG_SYSTEM, vprot, -1 );
            }
            close (fd);
        }
    }
#endif  /* linux */
    return TRUE;
}


/***********************************************************************
 *           VIRTUAL_GetPageSize
 */
DWORD VIRTUAL_GetPageSize(void)
{
    return 1 << page_shift;
}


/***********************************************************************
 *           VIRTUAL_GetGranularity
 */
DWORD VIRTUAL_GetGranularity(void)
{
    return granularity_mask + 1;
}


/***********************************************************************
 *           VIRTUAL_SetFaultHandler
 */
BOOL VIRTUAL_SetFaultHandler( LPCVOID addr, HANDLERPROC proc, LPVOID arg )
{
    FILE_VIEW *view;

    if (!(view = VIRTUAL_FindView((UINT)addr))) return FALSE;
    view->handlerProc = proc;
    view->handlerArg  = arg;
    return TRUE;
}

/***********************************************************************
 *           VIRTUAL_HandleFault
 */
DWORD VIRTUAL_HandleFault( LPCVOID addr )
{
    FILE_VIEW *view = VIRTUAL_FindView((UINT)addr);
    DWORD ret = EXCEPTION_ACCESS_VIOLATION;

    if (view)
    {
        if (view->handlerProc)
        {
            if (view->handlerProc(view->handlerArg, addr)) ret = 0;  /* handled */
        }
        else
        {
            BYTE vprot = view->prot[((UINT)addr - view->base) >> page_shift];
            UINT page = (UINT)addr & ~page_mask;
            char *stack = (char *)NtCurrentTeb()->stack_base + SIGNAL_STACK_SIZE + page_mask + 1;
            if (vprot & VPROT_GUARD)
            {
                VIRTUAL_SetProt( view, page, page_mask + 1, vprot & ~VPROT_GUARD );
                ret = STATUS_GUARD_PAGE_VIOLATION;
            }
            /* is it inside the stack guard pages? */
            if (((char *)addr >= stack) && ((char *)addr < stack + 2*(page_mask+1)))
                ret = STATUS_STACK_OVERFLOW;
        }
    }
    return ret;
}


/***********************************************************************
 *             VirtualAlloc   (KERNEL32.548)
 * Reserves or commits a region of pages in virtual address space
 *
 * RETURNS
 *	Base address of allocated region of pages
 *	NULL: Failure
 */
LPVOID WINAPI VirtualAlloc(
              LPVOID addr,  /* [in] Address of region to reserve or commit */
              DWORD size,   /* [in] Size of region */
              DWORD type,   /* [in] Type of allocation */
              DWORD protect /* [in] Type of access protection */
) {
    FILE_VIEW *view;
    UINT base, ptr, view_size;
    BYTE vprot;

    TRACE("%08x %08lx %lx %08lx\n",
                     (UINT)addr, size, type, protect );

    /* Round parameters to a page boundary */

    if (size > 0x7fc00000)  /* 2Gb - 4Mb */
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }
    if (addr)
    {
        if (type & MEM_RESERVE) /* Round down to 64k boundary */
            base = (UINT)addr & ~granularity_mask;
        else
            base = ROUND_ADDR( addr );
        size = (((UINT)addr + size + page_mask) & ~page_mask) - base;
        if ((base <= granularity_mask) || (base + size < base))
        {
            /* disallow low 64k and wrap-around */
            SetLastError( ERROR_INVALID_PARAMETER );
            return NULL;
        }
    }
    else
    {
        base = 0;
        size = (size + page_mask) & ~page_mask;
    }

    if (type & MEM_TOP_DOWN) {
    	/* FIXME: MEM_TOP_DOWN allocates the largest possible address.
	 *  	  Is there _ANY_ way to do it with UNIX mmap()?
	 */
    	WARN("MEM_TOP_DOWN ignored\n");
    	type &= ~MEM_TOP_DOWN;
    }
    /* Compute the protection flags */

    if (!(type & (MEM_COMMIT | MEM_RESERVE | MEM_SYSTEM)) ||
        (type & ~(MEM_COMMIT | MEM_RESERVE | MEM_SYSTEM)))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (type & (MEM_COMMIT | MEM_SYSTEM))
        vprot = VIRTUAL_GetProt( protect ) | VPROT_COMMITTED;
    else vprot = 0;

    /* Reserve the memory */

    if ((type & MEM_RESERVE) || !base)
    {
        view_size = size + (base ? 0 : granularity_mask + 1);
        if (type & MEM_SYSTEM)
             ptr = base;
        else
             ptr = (UINT)FILE_dommap( -1, (LPVOID)base, 0, view_size, 0, 0,
                                       VIRTUAL_GetUnixProt( vprot ), MAP_PRIVATE );
        if (ptr == (UINT)-1)
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }
        if (!base)
        {
            /* Release the extra memory while keeping the range */
            /* starting on a 64k boundary. */

            if (ptr & granularity_mask)
            {
                UINT extra = granularity_mask + 1 - (ptr & granularity_mask);
                FILE_munmap( (void *)ptr, 0, extra );
                ptr += extra;
                view_size -= extra;
            }
            if (view_size > size)
                FILE_munmap( (void *)(ptr + size), 0, view_size - size );
        }
        else if (ptr != base)
        {
            /* We couldn't get the address we wanted */
            FILE_munmap( (void *)ptr, 0, view_size );
	    SetLastError( ERROR_INVALID_ADDRESS );
	    return NULL;
        }
        if (!(view = VIRTUAL_CreateView( ptr, size, 0, (type & MEM_SYSTEM) ?
                                         VFLAG_SYSTEM : 0, vprot, -1 )))
        {
            FILE_munmap( (void *)ptr, 0, size );
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }
        VIRTUAL_DEBUG_DUMP_VIEW( view );
        return (LPVOID)ptr;
    }

    /* Commit the pages */

    if (!(view = VIRTUAL_FindView( base )) ||
        (base + size > view->base + view->size))
    {
        SetLastError( ERROR_INVALID_ADDRESS );
        return NULL;
    }

    if (!VIRTUAL_SetProt( view, base, size, vprot )) return NULL;
    return (LPVOID)base;
}


/***********************************************************************
 *             VirtualFree   (KERNEL32.550)
 * Release or decommits a region of pages in virtual address space.
 * 
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualFree(
              LPVOID addr, /* [in] Address of region of committed pages */
              DWORD size,  /* [in] Size of region */
              DWORD type   /* [in] Type of operation */
) {
    FILE_VIEW *view;
    UINT base;

    TRACE("%08x %08lx %lx\n",
                     (UINT)addr, size, type );

    /* Fix the parameters */

    size = ROUND_SIZE( addr, size );
    base = ROUND_ADDR( addr );

    if (!(view = VIRTUAL_FindView( base )) ||
        (base + size > view->base + view->size))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* Compute the protection flags */

    if ((type != MEM_DECOMMIT) && (type != MEM_RELEASE))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* Free the pages */

    if (type == MEM_RELEASE)
    {
        if (size || (base != view->base))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        VIRTUAL_DeleteView( view );
        return TRUE;
    }

    /* Decommit the pages by remapping zero-pages instead */

    if (FILE_dommap( -1, (LPVOID)base, 0, size, 0, 0,
                     VIRTUAL_GetUnixProt( 0 ), MAP_PRIVATE|MAP_FIXED ) 
        != (LPVOID)base)
        ERR( "Could not remap pages, expect trouble\n" );
    return VIRTUAL_SetProt( view, base, size, 0 );
}


/***********************************************************************
 *             VirtualLock   (KERNEL32.551)
 * Locks the specified region of virtual address space
 * 
 * NOTE
 *	Always returns TRUE
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualLock(
              LPVOID addr, /* [in] Address of first byte of range to lock */
              DWORD size   /* [in] Number of bytes in range to lock */
) {
    return TRUE;
}


/***********************************************************************
 *             VirtualUnlock   (KERNEL32.556)
 * Unlocks a range of pages in the virtual address space
 *
 * NOTE
 *	Always returns TRUE
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualUnlock(
              LPVOID addr, /* [in] Address of first byte of range */
              DWORD size   /* [in] Number of bytes in range */
) {
    return TRUE;
}


/***********************************************************************
 *             VirtualProtect   (KERNEL32.552)
 * Changes the access protection on a region of committed pages
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualProtect(
              LPVOID addr,     /* [in] Address of region of committed pages */
              DWORD size,      /* [in] Size of region */
              DWORD new_prot,  /* [in] Desired access protection */
              LPDWORD old_prot /* [out] Address of variable to get old protection */
) {
    FILE_VIEW *view;
    UINT base, i;
    BYTE vprot, *p;

    TRACE("%08x %08lx %08lx\n",
                     (UINT)addr, size, new_prot );

    /* Fix the parameters */

    size = ROUND_SIZE( addr, size );
    base = ROUND_ADDR( addr );

    if (!(view = VIRTUAL_FindView( base )) ||
        (base + size > view->base + view->size))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* Make sure all the pages are committed */

    p = view->prot + ((base - view->base) >> page_shift);
    for (i = size >> page_shift; i; i--, p++)
    {
        if (!(*p & VPROT_COMMITTED))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    VIRTUAL_GetWin32Prot( view->prot[0], old_prot, NULL );
    vprot = VIRTUAL_GetProt( new_prot ) | VPROT_COMMITTED;
    return VIRTUAL_SetProt( view, base, size, vprot );
}


/***********************************************************************
 *             VirtualProtectEx   (KERNEL32.553)
 * Changes the access protection on a region of committed pages in the
 * virtual address space of a specified process
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI VirtualProtectEx(
              HANDLE handle, /* [in]  Handle of process */
              LPVOID addr,     /* [in]  Address of region of committed pages */
              DWORD size,      /* [in]  Size of region */
              DWORD new_prot,  /* [in]  Desired access protection */
              LPDWORD old_prot /* [out] Address of variable to get old protection */ )
{
    if (MapProcessHandle( handle ) == GetCurrentProcessId())
        return VirtualProtect( addr, size, new_prot, old_prot );
    ERR("Unsupported on other process\n");
    return FALSE;
}


/***********************************************************************
 *             VirtualQuery   (KERNEL32.554)
 * Provides info about a range of pages in virtual address space
 *
 * RETURNS
 *	Number of bytes returned in information buffer
 */
DWORD WINAPI VirtualQuery(
             LPCVOID addr,                    /* [in]  Address of region */
             LPMEMORY_BASIC_INFORMATION info, /* [out] Address of info buffer */
             DWORD len                        /* [in]  Size of buffer */
) {
    FILE_VIEW *view = VIRTUAL_FirstView;
    UINT base = ROUND_ADDR( addr );
    UINT alloc_base = 0;
    UINT size = 0;

    /* Find the view containing the address */

    for (;;)
    {
        if (!view)
        {
            size = 0xffff0000 - alloc_base;
            break;
        }
        if (view->base > base)
        {
            size = view->base - alloc_base;
            view = NULL;
            break;
        }
        if (view->base + view->size > base)
        {
            alloc_base = view->base;
            size = view->size;
            break;
        }
        alloc_base = view->base + view->size;
        view = view->next;
    }

    /* Fill the info structure */

    if (!view)
    {
        info->State             = MEM_FREE;
        info->Protect           = 0;
        info->AllocationProtect = 0;
        info->Type              = 0;
    }
    else
    {
        BYTE vprot = view->prot[(base - alloc_base) >> page_shift];
        VIRTUAL_GetWin32Prot( vprot, &info->Protect, &info->State );
        for (size = base - alloc_base; size < view->size; size += page_mask+1)
            if (view->prot[size >> page_shift] != vprot) break;
        info->AllocationProtect = view->protect;
        info->Type              = MEM_PRIVATE;  /* FIXME */
    }

    info->BaseAddress    = (LPVOID)base;
    info->AllocationBase = (LPVOID)alloc_base;
    info->RegionSize     = size - (base - alloc_base);
    return sizeof(*info);
}


/***********************************************************************
 *             VirtualQueryEx   (KERNEL32.555)
 * Provides info about a range of pages in virtual address space of a
 * specified process
 *
 * RETURNS
 *	Number of bytes returned in information buffer
 */
DWORD WINAPI VirtualQueryEx(
             HANDLE handle,                 /* [in] Handle of process */
             LPCVOID addr,                    /* [in] Address of region */
             LPMEMORY_BASIC_INFORMATION info, /* [out] Address of info buffer */
             DWORD len                        /* [in] Size of buffer */ )
{
    if (MapProcessHandle( handle ) == GetCurrentProcessId())
        return VirtualQuery( addr, info, len );
    ERR("Unsupported on other process\n");
    return 0;
}


/***********************************************************************
 *             IsBadReadPtr   (KERNEL32.354)
 *
 * RETURNS
 *	FALSE: Process has read access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadReadPtr(
              LPCVOID ptr, /* Address of memory block */
              UINT size  /* Size of block */
) {
    return !VIRTUAL_CheckFlags( (UINT)ptr, size,
                                VPROT_READ | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadWritePtr   (KERNEL32.357)
 *
 * RETURNS
 *	FALSE: Process has write access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadWritePtr(
              LPVOID ptr, /* [in] Address of memory block */
              UINT size /* [in] Size of block in bytes */
) {
    return !VIRTUAL_CheckFlags( (UINT)ptr, size,
                                VPROT_WRITE | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadHugeReadPtr   (KERNEL32.352)
 * RETURNS
 *	FALSE: Process has read access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadHugeReadPtr(
              LPCVOID ptr, /* [in] Address of memory block */
              UINT size  /* [in] Size of block */
) {
    return IsBadReadPtr( ptr, size );
}


/***********************************************************************
 *             IsBadHugeWritePtr   (KERNEL32.353)
 * RETURNS
 *	FALSE: Process has write access to entire block
 *      TRUE: Otherwise
 */
BOOL WINAPI IsBadHugeWritePtr(
              LPVOID ptr, /* [in] Address of memory block */
              UINT size /* [in] Size of block */
) {
    return IsBadWritePtr( ptr, size );
}


/***********************************************************************
 *             IsBadCodePtr   (KERNEL32.351)
 *
 * RETURNS
 *	FALSE: Process has read access to specified memory
 *	TRUE: Otherwise
 */
BOOL WINAPI IsBadCodePtr(
              FARPROC ptr /* [in] Address of function */
) {
    return !VIRTUAL_CheckFlags( (UINT)ptr, 1, VPROT_EXEC | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadStringPtrA   (KERNEL32.355)
 *
 * RETURNS
 *	FALSE: Read access to all bytes in string
 *	TRUE: Else
 */
BOOL WINAPI IsBadStringPtrA(
              LPCSTR str, /* [in] Address of string */
              UINT max  /* [in] Maximum size of string */
) {
    FILE_VIEW *view;
    UINT page, count;

    if (!max) return FALSE;
    if (!(view = VIRTUAL_FindView( (UINT)str ))) return TRUE;
    page  = ((UINT)str - view->base) >> page_shift;
    count = page_mask + 1 - ((UINT)str & page_mask);

    while (max)
    {
        if ((view->prot[page] & (VPROT_READ | VPROT_COMMITTED)) != 
                                                (VPROT_READ | VPROT_COMMITTED))
            return TRUE;
        if (count > max) count = max;
        max -= count;
        while (count--) if (!*str++) return FALSE;
        if (++page >= view->size >> page_shift) return TRUE;
        count = page_mask + 1;
    }
    return FALSE;
}


/***********************************************************************
 *             IsBadStringPtrW   (KERNEL32.356)
 * See IsBadStringPtrA
 */
BOOL WINAPI IsBadStringPtrW( LPCWSTR str, UINT max )
{
    FILE_VIEW *view;
    UINT page, count;

    if (!max) return FALSE;
    if (!(view = VIRTUAL_FindView( (UINT)str ))) return TRUE;
    page  = ((UINT)str - view->base) >> page_shift;
    count = (page_mask + 1 - ((UINT)str & page_mask)) / sizeof(WCHAR);

    while (max)
    {
        if ((view->prot[page] & (VPROT_READ | VPROT_COMMITTED)) != 
                                                (VPROT_READ | VPROT_COMMITTED))
            return TRUE;
        if (count > max) count = max;
        max -= count;
        while (count--) if (!*str++) return FALSE;
        if (++page >= view->size >> page_shift) return TRUE;
        count = (page_mask + 1) / sizeof(WCHAR);
    }
    return FALSE;
}


/***********************************************************************
 *             CreateFileMappingA   (KERNEL32.46)
 * Creates a named or unnamed file-mapping object for the specified file
 *
 * RETURNS
 *	Handle: Success
 *	0: Mapping object does not exist
 *	NULL: Failure
 */
HANDLE WINAPI CreateFileMappingA(
                HFILE hFile,   /* [in] Handle of file to map */
                SECURITY_ATTRIBUTES *sa, /* [in] Optional security attributes*/
                DWORD protect,   /* [in] Protection for mapping object */
                DWORD size_high, /* [in] High-order 32 bits of object size */
                DWORD size_low,  /* [in] Low-order 32 bits of object size */
                LPCSTR name      /* [in] Name of file-mapping object */ )
{
    struct create_mapping_request *req = get_req_buffer();
    BYTE vprot;

    /* Check parameters */

    TRACE("(%x,%p,%08lx,%08lx%08lx,%s)\n",
          hFile, sa, protect, size_high, size_low, debugstr_a(name) );

    vprot = VIRTUAL_GetProt( protect );
    if (protect & SEC_RESERVE)
    {
        if (hFile != INVALID_HANDLE_VALUE)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }
    else vprot |= VPROT_COMMITTED;
    if (protect & SEC_NOCACHE) vprot |= VPROT_NOCACHE;

    /* Create the server object */

    req->file_handle = hFile;
    req->size_high   = size_high;
    req->size_low    = size_low;
    req->protect     = vprot;
    req->inherit     = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyAtoW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_MAPPING );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *             CreateFileMappingW   (KERNEL32.47)
 * See CreateFileMappingA
 */
HANDLE WINAPI CreateFileMappingW( HFILE hFile, LPSECURITY_ATTRIBUTES sa, 
                                  DWORD protect, DWORD size_high,  
                                  DWORD size_low, LPCWSTR name )
{
    struct create_mapping_request *req = get_req_buffer();
    BYTE vprot;

    /* Check parameters */

    TRACE("(%x,%p,%08lx,%08lx%08lx,%s)\n",
          hFile, sa, protect, size_high, size_low, debugstr_w(name) );

    vprot = VIRTUAL_GetProt( protect );
    if (protect & SEC_RESERVE)
    {
        if (hFile != INVALID_HANDLE_VALUE)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }
    else vprot |= VPROT_COMMITTED;
    if (protect & SEC_NOCACHE) vprot |= VPROT_NOCACHE;

    /* Create the server object */

    req->file_handle = hFile;
    req->size_high   = size_high;
    req->size_low    = size_low;
    req->protect     = vprot;
    req->inherit     = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_MAPPING );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *             OpenFileMappingA   (KERNEL32.397)
 * Opens a named file-mapping object.
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HANDLE WINAPI OpenFileMappingA(
                DWORD access,   /* [in] Access mode */
                BOOL inherit, /* [in] Inherit flag */
                LPCSTR name )   /* [in] Name of file-mapping object */
{
    struct open_mapping_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyAtoW( req->name, name );
    server_call( REQ_OPEN_MAPPING );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
}


/***********************************************************************
 *             OpenFileMappingW   (KERNEL32.398)
 * See OpenFileMappingA
 */
HANDLE WINAPI OpenFileMappingW( DWORD access, BOOL inherit, LPCWSTR name)
{
    struct open_mapping_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyW( req->name, name );
    server_call( REQ_OPEN_MAPPING );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
}


/***********************************************************************
 *             MapViewOfFile   (KERNEL32.385)
 * Maps a view of a file into the address space
 *
 * RETURNS
 *	Starting address of mapped view
 *	NULL: Failure
 */
LPVOID WINAPI MapViewOfFile(
              HANDLE mapping,  /* [in] File-mapping object to map */
              DWORD access,      /* [in] Access mode */
              DWORD offset_high, /* [in] High-order 32 bits of file offset */
              DWORD offset_low,  /* [in] Low-order 32 bits of file offset */
              DWORD count        /* [in] Number of bytes to map */
) {
    return MapViewOfFileEx( mapping, access, offset_high,
                            offset_low, count, NULL );
}


/***********************************************************************
 *             MapViewOfFileEx   (KERNEL32.386)
 * Maps a view of a file into the address space
 *
 * RETURNS
 *	Starting address of mapped view
 *	NULL: Failure
 */
LPVOID WINAPI MapViewOfFileEx(
              HANDLE handle,   /* [in] File-mapping object to map */
              DWORD access,      /* [in] Access mode */
              DWORD offset_high, /* [in] High-order 32 bits of file offset */
              DWORD offset_low,  /* [in] Low-order 32 bits of file offset */
              DWORD count,       /* [in] Number of bytes to map */
              LPVOID addr        /* [in] Suggested starting address for mapped view */
) {
    FILE_VIEW *view;
    UINT ptr = (UINT)-1, size = 0;
    int flags = MAP_PRIVATE;
    int unix_handle = -1;
    int prot;
    struct get_mapping_info_request *req = get_req_buffer();

    /* Check parameters */

    if ((offset_low & granularity_mask) ||
        (addr && ((UINT)addr & granularity_mask)))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    req->handle = handle;
    if (server_call_fd( REQ_GET_MAPPING_INFO, -1, &unix_handle )) goto error;

    if (req->size_high || offset_high)
        ERR("Offsets larger than 4Gb not supported\n");

    if ((offset_low >= req->size_low) ||
        (count > req->size_low - offset_low))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto error;
    }
    if (count) size = ROUND_SIZE( offset_low, count );
    else size = req->size_low - offset_low;
    prot = req->protect;

    switch(access)
    {
    case FILE_MAP_ALL_ACCESS:
    case FILE_MAP_WRITE:
    case FILE_MAP_WRITE | FILE_MAP_READ:
        if (!(prot & VPROT_WRITE))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            goto error;
        }
        flags = MAP_SHARED;
        /* fall through */
    case FILE_MAP_READ:
    case FILE_MAP_COPY:
    case FILE_MAP_COPY | FILE_MAP_READ:
        if (prot & VPROT_READ) break;
        /* fall through */
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        goto error;
    }

    /* Map the file */

    TRACE("handle=%x size=%x offset=%lx\n", handle, size, offset_low );

    ptr = (UINT)FILE_dommap( unix_handle, addr, 0, size, 0, offset_low,
                             VIRTUAL_GetUnixProt( prot ), flags );
    if (ptr == (UINT)-1) {
        /* KB: Q125713, 25-SEP-1995, "Common File Mapping Problems and
	 * Platform Differences": 
	 * Windows NT: ERROR_INVALID_PARAMETER
	 * Windows 95: ERROR_INVALID_ADDRESS.
	 * FIXME: So should we add a module dependend check here? -MM
	 */
	if (errno==ENOMEM)
	    SetLastError( ERROR_OUTOFMEMORY );
	else
	    SetLastError( ERROR_INVALID_PARAMETER );
        goto error;
    }

    if (!(view = VIRTUAL_CreateView( ptr, size, offset_low, 0, prot, handle )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        goto error;
    }
    if (unix_handle != -1) close( unix_handle );
    return (LPVOID)ptr;

error:
    if (unix_handle != -1) close( unix_handle );
    if (ptr != (UINT)-1) FILE_munmap( (void *)ptr, 0, size );
    return NULL;
}


/***********************************************************************
 *             FlushViewOfFile   (KERNEL32.262)
 * Writes to the disk a byte range within a mapped view of a file
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI FlushViewOfFile(
              LPCVOID base, /* [in] Start address of byte range to flush */
              DWORD cbFlush /* [in] Number of bytes in range */
) {
    FILE_VIEW *view;
    UINT addr = ROUND_ADDR( base );

    TRACE("FlushViewOfFile at %p for %ld bytes\n",
                     base, cbFlush );

    if (!(view = VIRTUAL_FindView( addr )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!cbFlush) cbFlush = view->size;
    if (!msync( (void *)addr, cbFlush, MS_SYNC )) return TRUE;
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}


/***********************************************************************
 *             UnmapViewOfFile   (KERNEL32.540)
 * Unmaps a mapped view of a file.
 *
 * NOTES
 *	Should addr be an LPCVOID?
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI UnmapViewOfFile(
              LPVOID addr /* [in] Address where mapped view begins */
) {
    FILE_VIEW *view;
    UINT base = ROUND_ADDR( addr );
    if (!(view = VIRTUAL_FindView( base )) || (base != view->base))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    VIRTUAL_DeleteView( view );
    return TRUE;
}

/***********************************************************************
 *             VIRTUAL_MapFileW
 *
 * Helper function to map a file to memory:
 *  name			-	file name 
 *  [RETURN] ptr		-	pointer to mapped file
 */
LPVOID VIRTUAL_MapFileW( LPCWSTR name )
{
    HANDLE hFile, hMapping;
    LPVOID ptr = NULL;

    hFile = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ, NULL, 
                           OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        hMapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
        CloseHandle( hFile );
        if (hMapping)
        {
            ptr = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );
            CloseHandle( hMapping );
        }
    }
    return ptr;
}
