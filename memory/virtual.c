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
#include "wine/exception.h"
#include "wine/unicode.h"
#include "winerror.h"
#include "file.h"
#include "process.h"
#include "global.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(virtual);
DECLARE_DEBUG_CHANNEL(module);

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
#else
static UINT page_shift;
static UINT page_mask;
#endif  /* __i386__ */
#define granularity_mask 0xffff  /* Allocation granularity (usually 64k) */

#define ROUND_ADDR(addr) \
   ((UINT)(addr) & ~page_mask)

#define ROUND_SIZE(addr,size) \
   (((UINT)(size) + ((UINT)(addr) & page_mask) + page_mask) & ~page_mask)

#define VIRTUAL_DEBUG_DUMP_VIEW(view) \
   if (!TRACE_ON(virtual)); else VIRTUAL_DumpView(view)


/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

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
        DPRINTF( " %d\n", view->mapping );
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
static FILE_VIEW *VIRTUAL_CreateView( UINT base, UINT size, UINT flags,
                                      BYTE vprot, HANDLE mapping )
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
 *           map_image
 *
 * Map an executable (PE format) image into memory.
 */
static LPVOID map_image( HANDLE hmapping, int fd, char *base, DWORD total_size,
                         DWORD header_size, HANDLE shared_file, DWORD shared_size )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    int i, pos;
    DWORD err = GetLastError();
    FILE_VIEW *view = NULL;
    char *ptr;
    int shared_fd = -1;

    SetLastError( ERROR_BAD_EXE_FORMAT );  /* generic error */

    /* zero-map the whole range */

    if ((ptr = FILE_dommap( -1, base, 0, total_size, 0, 0, PROT_READ|PROT_WRITE|PROT_EXEC,
                            MAP_PRIVATE )) == (char *)-1)
    {
        ptr = FILE_dommap( -1, NULL, 0, total_size, 0, 0,
                           PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE );
        if (ptr == (char *)-1)
        {
            ERR_(module)("Not enough memory for module (%ld bytes)\n", total_size);
            goto error;
        }
    }
    TRACE_(module)( "mapped PE file at %p-%p\n", ptr, ptr + total_size );

    if (!(view = VIRTUAL_CreateView( (UINT)ptr, total_size, 0,
                                     VPROT_COMMITTED|VPROT_READ|VPROT_WRITE|VPROT_WRITECOPY,
                                     hmapping )))
    {
        FILE_munmap( ptr, 0, total_size );
        SetLastError( ERROR_OUTOFMEMORY );
        goto error;
    }

    /* map the header */

    if (FILE_dommap( fd, ptr, 0, header_size, 0, 0,
                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED ) == (char *)-1) goto error;
    dos = (IMAGE_DOS_HEADER *)ptr;
    nt = (IMAGE_NT_HEADERS *)(ptr + dos->e_lfanew);
    if ((char *)(nt + 1) > ptr + header_size) goto error;

    sec = (IMAGE_SECTION_HEADER*)((char*)&nt->OptionalHeader+nt->FileHeader.SizeOfOptionalHeader);
    if ((char *)(sec + nt->FileHeader.NumberOfSections) > ptr + header_size) goto error;

    /* check the architecture */

    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        MESSAGE("Trying to load PE image for unsupported architecture (");
        switch (nt->FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_UNKNOWN: MESSAGE("Unknown"); break;
        case IMAGE_FILE_MACHINE_I860:    MESSAGE("I860"); break;
        case IMAGE_FILE_MACHINE_R3000:   MESSAGE("R3000"); break;
        case IMAGE_FILE_MACHINE_R4000:   MESSAGE("R4000"); break;
        case IMAGE_FILE_MACHINE_R10000:  MESSAGE("R10000"); break;
        case IMAGE_FILE_MACHINE_ALPHA:   MESSAGE("Alpha"); break;
        case IMAGE_FILE_MACHINE_POWERPC: MESSAGE("PowerPC"); break;
        default: MESSAGE("Unknown-%04x", nt->FileHeader.Machine); break;
        }
        MESSAGE(")\n");
        goto error;
    }
    
    /* retrieve the shared sections file */

    if (shared_size)
    {
        if ((shared_fd = FILE_GetUnixHandle( shared_file, GENERIC_READ )) == -1) goto error;
        CloseHandle( shared_file );  /* we no longer need it */
        shared_file = INVALID_HANDLE_VALUE; 
    }

    /* map all the sections */

    for (i = pos = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        DWORD size;

        /* a few sanity checks */
        size = sec->VirtualAddress + ROUND_SIZE( sec->VirtualAddress, sec->Misc.VirtualSize );
        if (sec->VirtualAddress > total_size || size > total_size || size < sec->VirtualAddress)
        {
            ERR_(module)( "Section %.8s too large (%lx+%lx/%lx)\n",
                          sec->Name, sec->VirtualAddress, sec->Misc.VirtualSize, total_size );
            goto error;
        }

        if ((sec->Characteristics & IMAGE_SCN_MEM_SHARED) &&
            (sec->Characteristics & IMAGE_SCN_MEM_WRITE))
        {
            size = ROUND_SIZE( 0, sec->Misc.VirtualSize );
            TRACE_(module)( "mapping shared section %.8s at %p off %lx (%x) size %lx (%lx) flags %lx\n",
                          sec->Name, (char *)ptr + sec->VirtualAddress,
                          sec->PointerToRawData, pos, sec->SizeOfRawData,
                          size, sec->Characteristics );
            if (FILE_dommap( shared_fd, (char *)ptr + sec->VirtualAddress, 0, size,
                             0, pos, PROT_READ|PROT_WRITE|PROT_EXEC,
                             MAP_SHARED|MAP_FIXED ) == (void *)-1)
            {
                ERR_(module)( "Could not map shared section %.8s\n", sec->Name );
                goto error;
            }
            pos += size;
            continue;
        }

    	if (sec->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) continue;
        if (!sec->PointerToRawData || !sec->SizeOfRawData) continue;

        TRACE_(module)( "mapping section %.8s at %p off %lx size %lx flags %lx\n",
                        sec->Name, (char *)ptr + sec->VirtualAddress,
                        sec->PointerToRawData, sec->SizeOfRawData,
                        sec->Characteristics );

        /* Note: if the section is not aligned properly FILE_dommap will magically
         *       fall back to read(), so we don't need to check anything here.
         */
        if (FILE_dommap( fd, (char *)ptr + sec->VirtualAddress, 0, sec->SizeOfRawData,
                         0, sec->PointerToRawData, PROT_READ|PROT_WRITE|PROT_EXEC,
                         MAP_PRIVATE | MAP_FIXED ) == (void *)-1)
        {
            ERR_(module)( "Could not map section %.8s, file probably truncated\n", sec->Name );
            goto error;
        }

        if ((sec->SizeOfRawData < sec->Misc.VirtualSize) && (sec->SizeOfRawData & page_mask))
        {
            DWORD end = ROUND_SIZE( 0, sec->SizeOfRawData );
            if (end > sec->Misc.VirtualSize) end = sec->Misc.VirtualSize;
            TRACE_(module)("clearing %p - %p\n",
                           (char *)ptr + sec->VirtualAddress + sec->SizeOfRawData,
                           (char *)ptr + sec->VirtualAddress + end );
            memset( (char *)ptr + sec->VirtualAddress + sec->SizeOfRawData, 0,
                    end - sec->SizeOfRawData );
        }
    }

    SetLastError( err );  /* restore last error */
    close( fd );
    if (shared_fd != -1) close( shared_fd );
    return ptr;

 error:
    if (view) VIRTUAL_DeleteView( view );
    close( fd );
    if (shared_fd != -1) close( shared_fd );
    if (shared_file != INVALID_HANDLE_VALUE) CloseHandle( shared_file );
    return NULL;
}


/***********************************************************************
 *           VIRTUAL_Init
 */
#ifndef page_mask
DECL_GLOBAL_CONSTRUCTOR(VIRTUAL_Init)
{
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
    /* Make sure we have a power of 2 */
    assert( !(page_size & page_mask) );
    page_shift = 0;
    while ((1 << page_shift) != page_size) page_shift++;
}
#endif  /* page_mask */


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
    /* Compute the alloc type flags */

    if (!(type & (MEM_COMMIT | MEM_RESERVE | MEM_SYSTEM)) ||
        (type & ~(MEM_COMMIT | MEM_RESERVE | MEM_SYSTEM)))
    {
	ERR("called with wrong alloc type flags (%08lx) !\n", type);
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
        if (!(view = VIRTUAL_CreateView( ptr, size, (type & MEM_SYSTEM) ?
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
 *             VirtualAllocEx   (KERNEL32.548)
 *
 * Seems to be just as VirtualAlloc, but with process handle.
 */
LPVOID WINAPI VirtualAllocEx(
              HANDLE hProcess, /* [in] Handle of process to do mem operation */
              LPVOID addr,  /* [in] Address of region to reserve or commit */
              DWORD size,   /* [in] Size of region */
              DWORD type,   /* [in] Type of allocation */
              DWORD protect /* [in] Type of access protection */
) {
    if (MapProcessHandle( hProcess ) == GetCurrentProcessId())
        return VirtualAlloc( addr, size, type, protect );
    ERR("Unsupported on other process\n");
    return NULL;
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
	ERR("called with wrong free type flags (%08lx) !\n", type);
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
              UINT size )  /* Size of block */
{
    __TRY
    {
        volatile const char *p = ptr;
        volatile const char *end = p + size - 1;
        char dummy;

        while (p < end)
        {
            dummy = *p;
            p += page_mask + 1;
        }
        dummy = *end;
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
    return FALSE;
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
              UINT size ) /* [in] Size of block in bytes */
{
    __TRY
    {
        volatile char *p = ptr;
        volatile char *end = p + size - 1;

        while (p < end)
        {
            *p |= 0;
            p += page_mask + 1;
        }
        *end |= 0;
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
    return FALSE;
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
BOOL WINAPI IsBadCodePtr( FARPROC ptr ) /* [in] Address of function */
{
    return IsBadReadPtr( ptr, 1 );
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
              UINT max )  /* [in] Maximum size of string */
{
    __TRY
    {
        volatile const char *p = str;
        while (p < str + max) if (!*p++) break;
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 *             IsBadStringPtrW   (KERNEL32.356)
 * See IsBadStringPtrA
 */
BOOL WINAPI IsBadStringPtrW( LPCWSTR str, UINT max )
{
    __TRY
    {
        volatile const WCHAR *p = str;
        while (p < str + max) if (!*p++) break;
    }
    __EXCEPT(page_fault) { return TRUE; }
    __ENDTRY
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
    HANDLE ret;
    BYTE vprot;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;

    /* Check parameters */

    TRACE("(%x,%p,%08lx,%08lx%08lx,%s)\n",
          hFile, sa, protect, size_high, size_low, debugstr_a(name) );

    if (len > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
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
    if (protect & SEC_IMAGE) vprot |= VPROT_IMAGE;

    /* Create the server object */

    SERVER_START_REQ
    {
        struct create_mapping_request *req = server_alloc_req( sizeof(*req),
                                                               len * sizeof(WCHAR) );
        req->file_handle = hFile;
        req->size_high   = size_high;
        req->size_low    = size_low;
        req->protect     = vprot;
        req->inherit     = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        server_call( REQ_CREATE_MAPPING );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *             CreateFileMappingW   (KERNEL32.47)
 * See CreateFileMappingA
 */
HANDLE WINAPI CreateFileMappingW( HFILE hFile, LPSECURITY_ATTRIBUTES sa, 
                                  DWORD protect, DWORD size_high,  
                                  DWORD size_low, LPCWSTR name )
{
    HANDLE ret;
    BYTE vprot;
    DWORD len = name ? strlenW(name) : 0;

    /* Check parameters */

    TRACE("(%x,%p,%08lx,%08lx%08lx,%s)\n",
          hFile, sa, protect, size_high, size_low, debugstr_w(name) );

    if (len > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }

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
    if (protect & SEC_IMAGE) vprot |= VPROT_IMAGE;

    /* Create the server object */

    SERVER_START_REQ
    {
        struct create_mapping_request *req = server_alloc_req( sizeof(*req),
                                                               len * sizeof(WCHAR) );
        req->file_handle = hFile;
        req->size_high   = size_high;
        req->size_low    = size_low;
        req->protect     = vprot;
        req->inherit     = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        server_call( REQ_CREATE_MAPPING );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
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
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct open_mapping_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        server_call( REQ_OPEN_MAPPING );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *             OpenFileMappingW   (KERNEL32.398)
 * See OpenFileMappingA
 */
HANDLE WINAPI OpenFileMappingW( DWORD access, BOOL inherit, LPCWSTR name)
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;
    if (len > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct open_mapping_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        server_call( REQ_OPEN_MAPPING );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
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
    prot = req->protect;

    if (prot & VPROT_IMAGE)
        return map_image( handle, unix_handle, req->base, req->size_low, req->header_size,
                          req->shared_file, req->shared_size );

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

    /* FIXME: If a mapping is created with SEC_RESERVE and a process,
     * which has a view of this mapping commits some pages, they will
     * appear commited in all other processes, which have the same
     * view created. Since we don`t support this yet, we create the
     * whole mapping commited.
     */
    prot |= VPROT_COMMITTED;

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

    if (!(view = VIRTUAL_CreateView( ptr, size, 0, prot, handle )))
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
