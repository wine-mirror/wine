/*
 * Win32 virtual memory functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "winbase.h"
#include "winerror.h"
#include "file.h"
#include "heap.h"
#include "process.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

#ifndef MS_SYNC
#define MS_SYNC 0
#endif

/* File mapping */
typedef struct
{
    K32OBJ        header;
    DWORD         size_high;
    DWORD         size_low;
    FILE_OBJECT  *file;
    BYTE          protect;
} FILE_MAPPING;

/* File view */
typedef struct _FV
{
    struct _FV   *next;     /* Next view */
    struct _FV   *prev;     /* Prev view */
    UINT32        base;     /* Base address */
    UINT32        size;     /* Size in bytes */
    UINT32        flags;    /* Allocation flags */
    UINT32        offset;   /* Offset from start of mapped file */
    FILE_MAPPING *mapping;  /* File mapping */
    BYTE          protect;  /* Protection for all pages at allocation time */
    BYTE          prot[1];  /* Protection byte for each page */
} FILE_VIEW;

/* Per-page protection byte values */
#define VPROT_READ       0x01
#define VPROT_WRITE      0x02
#define VPROT_EXEC       0x04
#define VPROT_WRITECOPY  0x08
#define VPROT_GUARD      0x10
#define VPROT_NOCACHE    0x20
#define VPROT_COMMITTED  0x40

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

static UINT32 page_shift;
static UINT32 page_mask;
static UINT32 granularity_mask;  /* Allocation granularity (usually 64k) */

#define ROUND_ADDR(addr) \
   ((UINT32)(addr) & ~page_mask)

#define ROUND_SIZE(addr,size) \
   (((UINT32)(size) + ((UINT32)(addr) & page_mask) + page_mask) & ~page_mask)

static void VIRTUAL_DestroyMapping( K32OBJ *obj );

const K32OBJ_OPS MEM_MAPPED_FILE_Ops =
{
    /* Object cannot be waited upon, so we don't need these (except destroy) */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    NULL,                      /* add_wait */
    NULL,                      /* remove_wait */
    VIRTUAL_DestroyMapping     /* destroy */
};

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
    UINT32 i, count;
    UINT32 addr = view->base;
    BYTE prot = view->prot[0];

    dprintf_virtual( stddeb, "View: %08x - %08x%s",
             view->base, view->base + view->size - 1,
             (view->flags & VFLAG_SYSTEM) ? " (system)" : "" );
    if (view->mapping && view->mapping->file)
        dprintf_virtual( stddeb, " %s @ %08x\n",
                 view->mapping->file->unix_name, view->offset );
    else
        dprintf_virtual( stddeb, " (anonymous)\n");

    for (count = i = 1; i < view->size >> page_shift; i++, count++)
    {
        if (view->prot[i] == prot) continue;
        dprintf_virtual( stddeb, "      %08x - %08x %s\n",
                 addr, addr + (count << page_shift) - 1,
                 VIRTUAL_GetProtStr(prot) );
        addr += (count << page_shift);
        prot = view->prot[i];
        count = 0;
    }
    if (count)
        dprintf_virtual( stddeb, "      %08x - %08x %s\n",
                 addr, addr + (count << page_shift) - 1,
                 VIRTUAL_GetProtStr(prot) );
}


/***********************************************************************
 *           VIRTUAL_Dump
 */
void VIRTUAL_Dump(void)
{
    FILE_VIEW *view = VIRTUAL_FirstView;
    dprintf_virtual( stddeb, "\nDump of all virtual memory views:\n\n" );
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
 */
static FILE_VIEW *VIRTUAL_FindView( UINT32 addr )
{
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
static FILE_VIEW *VIRTUAL_CreateView( UINT32 base, UINT32 size, UINT32 offset,
                                      UINT32 flags, BYTE vprot,
                                      FILE_MAPPING *mapping )
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
    memset( view->prot, vprot, size );

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
    if (debugging_virtual) VIRTUAL_DumpView( view );
    return view;
}


/***********************************************************************
 *           VIRTUAL_DeleteView
 *
 * Delete an view.
 */
static void VIRTUAL_DeleteView( FILE_VIEW *view )
{
    FILE_munmap( (void *)view->base, 0, view->size );
    if (view->next) view->next->prev = view->prev;
    if (view->prev) view->prev->next = view->next;
    else VIRTUAL_FirstView = view->next;
    if (view->mapping) K32OBJ_DecCount( &view->mapping->header );
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
        if (vprot & VPROT_EXEC) prot |= PROT_EXEC;
    }
    return prot;
}


/***********************************************************************
 *           VIRTUAL_GetWin32Prot
 *
 * Convert page protections to Win32 flags.
 */
static void VIRTUAL_GetWin32Prot( BYTE vprot, DWORD *protect, DWORD *state )
{
    *protect = VIRTUAL_Win32Flags[vprot & 0x0f];
    if (vprot & VPROT_GUARD) *protect |= PAGE_GUARD;
    if (vprot & VPROT_NOCACHE) *protect |= PAGE_NOCACHE;

    if (state) *state = (vprot & VPROT_COMMITTED) ? MEM_COMMIT : MEM_RESERVE;
}


/***********************************************************************
 *           VIRTUAL_GetProt
 *
 * Build page protections from Win32 flags.
 */
static BYTE VIRTUAL_GetProt( DWORD protect )
{
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
        vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITE | VPROT_WRITECOPY;
        break;
    case PAGE_EXECUTE_WRITECOPY:
        vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITE;
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
 */
static BOOL32 VIRTUAL_SetProt( FILE_VIEW *view, UINT32 base,
                               UINT32 size, BYTE vprot )
{
    dprintf_virtual( stddeb, "VIRTUAL_SetProt: %08x-%08x %s\n",
                     base, base + size - 1, VIRTUAL_GetProtStr( vprot ) );

    if (mprotect( (void *)base, size, VIRTUAL_GetUnixProt(vprot) ))
        return FALSE;  /* FIXME: last error */

    memset( view->prot + ((base - view->base) >> page_shift),
            vprot, size >> page_shift );
    if (debugging_virtual) VIRTUAL_DumpView( view );
    return TRUE;
}


/***********************************************************************
 *             VIRTUAL_CheckFlags
 *
 * Check that all pages in a range have the given flags.
 */
static BOOL32 VIRTUAL_CheckFlags( UINT32 base, UINT32 size, BYTE flags )
{
    FILE_VIEW *view;
    UINT32 page;

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
BOOL32 VIRTUAL_Init(void)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );

    page_mask = sysinfo.dwPageSize - 1;
    granularity_mask = sysinfo.dwAllocationGranularity - 1;
    /* Make sure we have a power of 2 */
    assert( !(sysinfo.dwPageSize & page_mask) );
    assert( !(sysinfo.dwAllocationGranularity & granularity_mask) );
    page_shift = 0;
    while ((1 << page_shift) != sysinfo.dwPageSize) page_shift++;

#ifdef linux
    {
        /* Do not use stdio here since it may temporarily change the size
         * of some segments (ie libc6 adds 0x1000 per open FILE)
         */
        int fd = open ("/proc/self/maps", O_RDONLY);
        if (fd >= 0)
        {
            char buffer[80];

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
                                    VFLAG_SYSTEM, vprot, NULL );
            }
            close (fd);
        }
    }
#endif  /* linux */
    return TRUE;
}


/***********************************************************************
 *             VirtualAlloc   (KERNEL32.548)
 */
LPVOID WINAPI VirtualAlloc( LPVOID addr, DWORD size, DWORD type, DWORD protect)
{
    FILE_VIEW *view;
    UINT32 base, ptr, view_size;
    BYTE vprot;

    dprintf_virtual( stddeb, "VirtualAlloc: %08x %08lx %lx %08lx\n",
                     (UINT32)addr, size, type, protect );

    /* Round parameters to a page boundary */

    if (size > 0x7fc00000)  /* 2Gb - 4Mb */
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }
    if (addr)
    {
        if (type & MEM_RESERVE) /* Round down to 64k boundary */
            base = ((UINT32)addr + granularity_mask) & ~granularity_mask;
        else
            base = ROUND_ADDR( addr );
        size = (((UINT32)addr + size + page_mask) & ~page_mask) - base;
        if (base + size < base)  /* Disallow wrap-around */
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return NULL;
        }
    }
    else
    {
        base = 0;
        size = (size + page_mask) & ~page_mask;
    }

    /* Compute the protection flags */

    if (!(type & (MEM_COMMIT | MEM_RESERVE)) ||
        (type & ~(MEM_COMMIT | MEM_RESERVE)))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (type & MEM_COMMIT)
        vprot = VIRTUAL_GetProt( protect ) | VPROT_COMMITTED;
    else vprot = 0;

    /* Reserve the memory */

    if ((type & MEM_RESERVE) || !base)
    {
        view_size = size + (base ? 0 : granularity_mask + 1);
        ptr = (UINT32)FILE_dommap( NULL, (LPVOID)base, 0, view_size, 0, 0,
                                   VIRTUAL_GetUnixProt( vprot ), MAP_PRIVATE );
        if (ptr == (UINT32)-1)
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
                UINT32 extra = granularity_mask + 1 - (ptr & granularity_mask);
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
        if (!(view = VIRTUAL_CreateView( ptr, size, 0, 0, vprot, NULL )))
        {
            FILE_munmap( (void *)ptr, 0, size );
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }
        if (debugging_virtual) VIRTUAL_DumpView( view );
        return (LPVOID)ptr;
    }

    /* Commit the pages */

    if (!(view = VIRTUAL_FindView( base )) ||
        (base + size > view->base + view->size))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (!VIRTUAL_SetProt( view, base, size, vprot )) return NULL;
    return (LPVOID)base;
}


/***********************************************************************
 *             VirtualFree   (KERNEL32.550)
 */
BOOL32 WINAPI VirtualFree( LPVOID addr, DWORD size, DWORD type )
{
    FILE_VIEW *view;
    UINT32 base;

    dprintf_virtual( stddeb, "VirtualFree: %08x %08lx %lx\n",
                     (UINT32)addr, size, type );

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

    /* Decommit the pages */

    return VIRTUAL_SetProt( view, base, size, 0 );
}


/***********************************************************************
 *             VirtualLock   (KERNEL32.551)
 */
BOOL32 WINAPI VirtualLock( LPVOID addr, DWORD size )
{
    return TRUE;
}


/***********************************************************************
 *             VirtualUnlock   (KERNEL32.556)
 */
BOOL32 WINAPI VirtualUnlock( LPVOID addr, DWORD size )
{
    return TRUE;
}


/***********************************************************************
 *             VirtualProtect   (KERNEL32.552)
 */
BOOL32 WINAPI VirtualProtect( LPVOID addr, DWORD size, DWORD new_prot,
                              LPDWORD old_prot )
{
    FILE_VIEW *view;
    UINT32 base, i;
    BYTE vprot, *p;

    dprintf_virtual( stddeb, "VirtualProtect: %08x %08lx %08lx\n",
                     (UINT32)addr, size, new_prot );

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
 */
BOOL32 WINAPI VirtualProtectEx( HANDLE32 handle, LPVOID addr, DWORD size,
                                DWORD new_prot, LPDWORD old_prot )
{
    BOOL32 ret = FALSE;

    PDB32 *pdb = (PDB32 *)PROCESS_GetObjPtr( handle, K32OBJ_PROCESS );
    if (pdb)
    {
        if (pdb == PROCESS_Current())
            ret = VirtualProtect( addr, size, new_prot, old_prot );
        else
            fprintf(stderr,"Unsupported: VirtualProtectEx on other process\n");
        K32OBJ_DecCount( &pdb->header );
    }
    return ret;
}


/***********************************************************************
 *             VirtualQuery   (KERNEL32.554)
 */
BOOL32 WINAPI VirtualQuery( LPCVOID addr, LPMEMORY_BASIC_INFORMATION info,
                            DWORD len )
{
    FILE_VIEW *view = VIRTUAL_FirstView;
    UINT32 base = ROUND_ADDR( addr );
    UINT32 alloc_base = 0;
    UINT32 size = 0;

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
    return TRUE;
}


/***********************************************************************
 *             VirtualQueryEx   (KERNEL32.555)
 */
BOOL32 WINAPI VirtualQueryEx( HANDLE32 handle, LPCVOID addr,
                              LPMEMORY_BASIC_INFORMATION info, DWORD len )
{
    BOOL32 ret = FALSE;

    PDB32 *pdb = (PDB32 *)PROCESS_GetObjPtr( handle, K32OBJ_PROCESS );
    if (pdb)
    {
        if (pdb == PROCESS_Current())
            ret = VirtualQuery( addr, info, len );
        else
            fprintf(stderr,"Unsupported: VirtualQueryEx on other process\n");
        K32OBJ_DecCount( &pdb->header );
    }
    return ret;
}


/***********************************************************************
 *             IsBadReadPtr32   (KERNEL32.354)
 */
BOOL32 WINAPI IsBadReadPtr32( LPCVOID ptr, UINT32 size )
{
    return !VIRTUAL_CheckFlags( (UINT32)ptr, size,
                                VPROT_READ | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadWritePtr32   (KERNEL32.357)
 */
BOOL32 WINAPI IsBadWritePtr32( LPVOID ptr, UINT32 size )
{
    return !VIRTUAL_CheckFlags( (UINT32)ptr, size,
                                VPROT_WRITE | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadHugeReadPtr32   (KERNEL32.352)
 */
BOOL32 WINAPI IsBadHugeReadPtr32( LPCVOID ptr, UINT32 size )
{
    return IsBadReadPtr32( ptr, size );
}


/***********************************************************************
 *             IsBadHugeWritePtr32   (KERNEL32.353)
 */
BOOL32 WINAPI IsBadHugeWritePtr32( LPVOID ptr, UINT32 size )
{
    return IsBadWritePtr32( ptr, size );
}


/***********************************************************************
 *             IsBadCodePtr32   (KERNEL32.351)
 */
BOOL32 WINAPI IsBadCodePtr32( FARPROC32 ptr )
{
    return !VIRTUAL_CheckFlags( (UINT32)ptr, 1, VPROT_EXEC | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadStringPtr32A   (KERNEL32.355)
 */
BOOL32 WINAPI IsBadStringPtr32A( LPCSTR str, UINT32 max )
{
    FILE_VIEW *view;
    UINT32 page, count;

    if (!max) return FALSE;
    if (!(view = VIRTUAL_FindView( (UINT32)str ))) return TRUE;
    page  = ((UINT32)str - view->base) >> page_shift;
    count = page_mask + 1 - ((UINT32)str & page_mask);

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
 *             IsBadStringPtr32W   (KERNEL32.356)
 */
BOOL32 WINAPI IsBadStringPtr32W( LPCWSTR str, UINT32 max )
{
    FILE_VIEW *view;
    UINT32 page, count;

    if (!max) return FALSE;
    if (!(view = VIRTUAL_FindView( (UINT32)str ))) return TRUE;
    page  = ((UINT32)str - view->base) >> page_shift;
    count = (page_mask + 1 - ((UINT32)str & page_mask)) / sizeof(WCHAR);

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
 *             CreateFileMapping32A   (KERNEL32.46)
 */
HANDLE32 WINAPI CreateFileMapping32A(HFILE32 hFile, LPSECURITY_ATTRIBUTES attr,
                                     DWORD protect, DWORD size_high,
                                     DWORD size_low, LPCSTR name )
{
    FILE_MAPPING *mapping = NULL;
    HANDLE32 handle;
    BYTE vprot;

    /* First search for an object with the same name */

    K32OBJ *obj = K32OBJ_FindName( name );
    if (obj)
    {
        if (obj->type == K32OBJ_MEM_MAPPED_FILE)
        {
            SetLastError( ERROR_ALREADY_EXISTS );
            handle = PROCESS_AllocHandle( obj, 0 );
        }
        else
        {
            SetLastError( ERROR_DUP_NAME );
            handle = 0;
        }
        K32OBJ_DecCount( obj );
        return handle;
    }

    /* Check parameters */

    dprintf_virtual(stddeb,"CreateFileMapping32A(%x,%p,%08lx,%08lx%08lx,%s)\n",
                    hFile, attr, protect, size_high, size_low, name );

    vprot = VIRTUAL_GetProt( protect );
    if (protect & SEC_RESERVE)
    {
        if (hFile != INVALID_HANDLE_VALUE32)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }
    else vprot |= VPROT_COMMITTED;
    if (protect & SEC_NOCACHE) vprot |= VPROT_NOCACHE;

    /* Compute the size and extend the file if necessary */

    if (hFile == INVALID_HANDLE_VALUE32)
    {
        if (!size_high && !size_low)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        obj = NULL;
    }
    else  /* We have a file */
    {
        BY_HANDLE_FILE_INFORMATION info;
        if (!(obj = PROCESS_GetObjPtr( hFile, K32OBJ_FILE ))) goto error;
        /* FIXME: should check if the file permissions agree
         *        with the required protection flags */
        if (!GetFileInformationByHandle( hFile, &info )) goto error;
        if (!size_high && !size_low)
        {
            size_high = info.nFileSizeHigh;
            size_low  = info.nFileSizeLow;
        }
        else if ((size_high > info.nFileSizeHigh) ||
                 ((size_high == info.nFileSizeHigh) &&
                  (size_low > info.nFileSizeLow)))
        {
            /* We have to grow the file */
            if (SetFilePointer( hFile, size_low, &size_high,
                                FILE_BEGIN ) == 0xffffffff) goto error;
            if (!SetEndOfFile( hFile )) goto error;
        }
    }

    /* Allocate the mapping object */

    if (!(mapping = HeapAlloc( SystemHeap, 0, sizeof(*mapping) ))) goto error;
    mapping->header.type     = K32OBJ_MEM_MAPPED_FILE;
    mapping->header.refcount = 1;
    mapping->protect         = vprot;
    mapping->size_high       = size_high;
    mapping->size_low        = ROUND_SIZE( 0, size_low );
    mapping->file            = (FILE_OBJECT *)obj;

    if (!K32OBJ_AddName( &mapping->header, name )) handle = 0;
    else handle = PROCESS_AllocHandle( &mapping->header, 0 );
    K32OBJ_DecCount( &mapping->header );
    return handle;

error:
    if (obj) K32OBJ_DecCount( obj );
    if (mapping) HeapFree( SystemHeap, 0, mapping );
    return 0;
}


/***********************************************************************
 *             CreateFileMapping32W   (KERNEL32.47)
 */
HANDLE32 WINAPI CreateFileMapping32W(HFILE32 hFile, LPSECURITY_ATTRIBUTES attr,
                                     DWORD protect, DWORD size_high,
                                     DWORD size_low, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = CreateFileMapping32A( hFile, attr, protect,
                                         size_high, size_low, nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *             OpenFileMapping32A   (KERNEL32.397)
 */
HANDLE32 WINAPI OpenFileMapping32A( DWORD access, BOOL32 inherit, LPCSTR name )
{
    HANDLE32 handle = 0;
    K32OBJ *obj;
    SYSTEM_LOCK();
    if ((obj = K32OBJ_FindNameType( name, K32OBJ_MEM_MAPPED_FILE )))
    {
        handle = PROCESS_AllocHandle( obj, 0 );
        K32OBJ_DecCount( obj );
    }
    SYSTEM_UNLOCK();
    return handle;
}


/***********************************************************************
 *             OpenFileMapping32W   (KERNEL32.398)
 */
HANDLE32 WINAPI OpenFileMapping32W( DWORD access, BOOL32 inherit, LPCWSTR name)
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = OpenFileMapping32A( access, inherit, nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           VIRTUAL_DestroyMapping
 *
 * Destroy a FILE_MAPPING object.
 */
static void VIRTUAL_DestroyMapping( K32OBJ *ptr )
{
    FILE_MAPPING *mapping = (FILE_MAPPING *)ptr;
    assert( ptr->type == K32OBJ_MEM_MAPPED_FILE );

    if (mapping->file) K32OBJ_DecCount( &mapping->file->header );
    ptr->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, mapping );
}


/***********************************************************************
 *             MapViewOfFile   (KERNEL32.385)
 */
LPVOID WINAPI MapViewOfFile( HANDLE32 mapping, DWORD access, DWORD offset_high,
                             DWORD offset_low, DWORD count )
{
    return MapViewOfFileEx( mapping, access, offset_high,
                            offset_low, count, NULL );
}


/***********************************************************************
 *             MapViewOfFileEx   (KERNEL32.386)
 */
LPVOID WINAPI MapViewOfFileEx(HANDLE32 handle, DWORD access, DWORD offset_high,
                              DWORD offset_low, DWORD count, LPVOID addr )
{
    FILE_MAPPING *mapping;
    FILE_VIEW *view;
    UINT32 ptr = (UINT32)-1, size = 0;
    int flags = MAP_PRIVATE;

    /* Check parameters */

    if ((offset_low & granularity_mask) ||
        (addr && ((UINT32)addr & granularity_mask)))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (!(mapping = (FILE_MAPPING *)PROCESS_GetObjPtr( handle,
                                                     K32OBJ_MEM_MAPPED_FILE )))
        return NULL;

    if (mapping->size_high || offset_high)
        fprintf( stderr, "MapViewOfFileEx: offsets larger than 4Gb not supported\n");

    if ((offset_low >= mapping->size_low) ||
        (count > mapping->size_low - offset_low))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto error;
    }
    if (count) size = ROUND_SIZE( offset_low, count );
    else size = mapping->size_low - offset_low;

    switch(access)
    {
    case FILE_MAP_ALL_ACCESS:
    case FILE_MAP_WRITE:
    case FILE_MAP_WRITE | FILE_MAP_READ:
        if (!(mapping->protect & VPROT_WRITE))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            goto error;
        }
        flags = MAP_SHARED;
        /* fall through */
    case FILE_MAP_READ:
    case FILE_MAP_COPY:
    case FILE_MAP_COPY | FILE_MAP_READ:
        if (mapping->protect & VPROT_READ) break;
        /* fall through */
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        goto error;
    }

    /* Map the file */

    dprintf_virtual( stddeb, "MapViewOfFile: handle=%x size=%x offset=%lx\n",
                     handle, size, offset_low );

    ptr = (UINT32)FILE_dommap( mapping->file, addr, 0, size, 0, offset_low,
                               VIRTUAL_GetUnixProt( mapping->protect ),
                               flags );
    if (ptr == (UINT32)-1)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        goto error;
    }

    if (!(view = VIRTUAL_CreateView( ptr, size, offset_low, 0,
                                     mapping->protect, mapping )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        goto error;
    }
    return (LPVOID)ptr;

error:
    if (ptr != (UINT32)-1) FILE_munmap( (void *)ptr, 0, size );
    K32OBJ_DecCount( &mapping->header );
    return NULL;
}


/***********************************************************************
 *             FlushViewOfFile   (KERNEL32.262)
 */
BOOL32 WINAPI FlushViewOfFile( LPCVOID base, DWORD cbFlush )
{
    FILE_VIEW *view;
    UINT32 addr = ROUND_ADDR( base );

    dprintf_virtual( stddeb, "FlushViewOfFile at %p for %ld bytes\n",
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
 */
BOOL32 WINAPI UnmapViewOfFile( LPVOID addr )
{
    FILE_VIEW *view;
    UINT32 base = ROUND_ADDR( addr );
    if (!(view = VIRTUAL_FindView( base )) || (base != view->base))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    VIRTUAL_DeleteView( view );
    return TRUE;
}
