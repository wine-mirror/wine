/*
 * Win32 virtual memory functions
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
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

#define ROUND_ADDR(addr) \
   ((UINT32)(addr) & ~page_mask)

#define ROUND_SIZE(addr,size) \
   (((UINT32)(size) + ((UINT32)(addr) & page_mask) + page_mask) & ~page_mask)


/***********************************************************************
 *           VIRTUAL_DestroyMapping
 *
 * Destroy a FILE_MAPPING object.
 */
void VIRTUAL_DestroyMapping( K32OBJ *ptr )
{
    FILE_MAPPING *mapping = (FILE_MAPPING *)ptr;
    assert( ptr->type == K32OBJ_MEM_MAPPED_FILE );

    if (mapping->file) K32OBJ_DecCount( &mapping->file->header );
    ptr->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, mapping );
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
    UINT32 i, count;
    UINT32 addr = view->base;
    BYTE prot = view->prot[0];

    dprintf_virtual( stddeb, "View: %08x - %08x%s\n",
                     view->base, view->base + view->size - 1,
                     (view->flags & VFLAG_SYSTEM) ? " (system)" : "" );

    for (count = i = 1; i < view->size >> page_shift; i++, count++)
    {
        if (view->prot[i] == prot) continue;
        dprintf_virtual( stddeb, "    %08x - %08x %s\n",
                         addr, addr + (count << page_shift) - 1,
                         VIRTUAL_GetProtStr(prot) );
        addr += (count << page_shift);
        prot = view->prot[i];
        count = 0;
    }
    if (count)
        dprintf_virtual( stddeb, "    %08x - %08x %s\n",
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
static FILE_VIEW *VIRTUAL_CreateView( UINT32 base, UINT32 size,
                                      UINT32 flags, BYTE vprot )
{
    FILE_VIEW *view, *prev;

    /* Create the view structure */

    size >>= page_shift;
    view = (FILE_VIEW *)xmalloc( sizeof(*view) + size - 1 );
    view->base    = base;
    view->size    = size << page_shift;
    view->flags   = flags;
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
    munmap( (void *)view->base, view->size );
    if (view->next) view->next->prev = view->prev;
    if (view->prev) view->prev->next = view->next;
    else VIRTUAL_FirstView = view->next;
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
    /* Make sure we have a power of 2 */
    assert( !(sysinfo.dwPageSize & page_mask) );
    page_shift = 0;
    while ((1 << page_shift) != sysinfo.dwPageSize) page_shift++;

#ifdef linux
    {
	FILE *f = fopen( "/proc/self/maps", "r" );
        if (f)
        {
            char buffer[80];
            while (fgets( buffer, sizeof(buffer), f ))
            {
                int start, end, offset;
                char r, w, x, p;
                BYTE vprot = VPROT_COMMITTED;

                sscanf( buffer, "%x-%x %c%c%c%c %x",
                        &start, &end, &r, &w, &x, &p, &offset );
                if (r == 'r') vprot |= VPROT_READ;
                if (w == 'w') vprot |= VPROT_WRITE;
                if (x == 'x') vprot |= VPROT_EXEC;
                if (p == 'p') vprot |= VPROT_WRITECOPY;
                VIRTUAL_CreateView( start, end - start, VFLAG_SYSTEM, vprot );
            }
            fclose( f );
        }
    }
#endif  /* linux */
    return TRUE;
}


/***********************************************************************
 *             VirtualAlloc   (KERNEL32.548)
 */
LPVOID VirtualAlloc( LPVOID addr, DWORD size, DWORD type, DWORD protect )
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
            base = ((UINT32)addr + 0xffff) & ~0xffff;
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
        view_size = size + (base ? 0 : 0x10000);
        ptr = (UINT32)FILE_mmap( NULL, (LPVOID)base, 0, view_size, 0, 0,
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

            if (ptr & 0xffff0000)
            {
                munmap( (void *)ptr, 0x10000 - (ptr & 0xffff) );
                view_size -= (ptr & 0xffff);
                ptr = (ptr + 0x10000) & 0xffff0000;
            }
            if (view_size > size)
                munmap( (void *)(ptr + size), view_size - size );
        }
        if (!(view = VIRTUAL_CreateView( ptr, size, 0, vprot )))
        {
            munmap( (void *)ptr, size );
            return NULL;  /* FIXME: last error */
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
BOOL32 VirtualFree( LPVOID addr, DWORD size, DWORD type )
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
BOOL32 VirtualLock( LPVOID addr, DWORD size )
{
    return TRUE;
}


/***********************************************************************
 *             VirtualUnlock   (KERNEL32.556)
 */
BOOL32 VirtualUnlock( LPVOID addr, DWORD size )
{
    return TRUE;
}


/***********************************************************************
 *             VirtualProtect   (KERNEL32.552)
 */
BOOL32 VirtualProtect( LPVOID addr, DWORD size, DWORD new_prot,
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
BOOL32 VirtualProtectEx( HANDLE32 handle, LPVOID addr, DWORD size,
                         DWORD new_prot, LPDWORD old_prot )
{
    BOOL32 ret = FALSE;

    PDB32 *pdb = (PDB32 *)PROCESS_GetObjPtr( handle, K32OBJ_PROCESS );
    if (pdb)
    {
        if (pdb == pCurrentProcess)
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
BOOL32 VirtualQuery( LPCVOID addr, LPMEMORY_BASIC_INFORMATION info, DWORD len )
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
BOOL32 VirtualQueryEx( HANDLE32 handle, LPCVOID addr,
                       LPMEMORY_BASIC_INFORMATION info, DWORD len )
{
    BOOL32 ret = FALSE;

    PDB32 *pdb = (PDB32 *)PROCESS_GetObjPtr( handle, K32OBJ_PROCESS );
    if (pdb)
    {
        if (pdb == pCurrentProcess)
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
BOOL32 IsBadReadPtr32( LPCVOID ptr, UINT32 size )
{
    return !VIRTUAL_CheckFlags( (UINT32)ptr, size,
                                VPROT_READ | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadWritePtr32   (KERNEL32.357)
 */
BOOL32 IsBadWritePtr32( LPVOID ptr, UINT32 size )
{
    return !VIRTUAL_CheckFlags( (UINT32)ptr, size,
                                VPROT_WRITE | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadHugeReadPtr32   (KERNEL32.352)
 */
BOOL32 IsBadHugeReadPtr32( LPCVOID ptr, UINT32 size )
{
    return IsBadReadPtr32( ptr, size );
}


/***********************************************************************
 *             IsBadHugeWritePtr32   (KERNEL32.353)
 */
BOOL32 IsBadHugeWritePtr32( LPVOID ptr, UINT32 size )
{
    return IsBadWritePtr32( ptr, size );
}


/***********************************************************************
 *             IsBadCodePtr32   (KERNEL32.351)
 */
BOOL32 IsBadCodePtr32( FARPROC32 ptr )
{
    return !VIRTUAL_CheckFlags( (UINT32)ptr, 1, VPROT_EXEC | VPROT_COMMITTED );
}


/***********************************************************************
 *             IsBadStringPtr32A   (KERNEL32.355)
 */
BOOL32 IsBadStringPtr32A( LPCSTR str, UINT32 max )
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
BOOL32 IsBadStringPtr32W( LPCWSTR str, UINT32 max )
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
HANDLE32 CreateFileMapping32A( HFILE32 hFile, LPSECURITY_ATTRIBUTES attr,
                               DWORD protect, DWORD size_high, DWORD size_low,
                               LPCSTR name )
{
    FILE_MAPPING *mapping = NULL;
    HANDLE32 handle;

    K32OBJ *obj = K32OBJ_FindName( name );
    if (obj)
    {
        if (obj->type == K32OBJ_MEM_MAPPED_FILE)
        {
            SetLastError( ERROR_ALREADY_EXISTS );
            return PROCESS_AllocHandle( obj, 0 );
        }
        SetLastError( ERROR_DUP_NAME );
        return 0;
    }

    printf( "CreateFileMapping32A(%x,%p,%08lx,%08lx%08lx,%s)\n",
            hFile, attr, protect, size_high, size_low, name );

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

    if (!(mapping = HeapAlloc( SystemHeap, 0, sizeof(*mapping) ))) goto error;
    mapping->header.type = K32OBJ_MEM_MAPPED_FILE;
    mapping->header.refcount = 1;
    mapping->protect   = VIRTUAL_GetProt( protect ) | VPROT_COMMITTED;
    mapping->size_high = size_high;
    mapping->size_low  = size_low;
    mapping->file      = (FILE_OBJECT *)obj;

    handle = PROCESS_AllocHandle( &mapping->header, 0 );
    if (handle != INVALID_HANDLE_VALUE32) return handle;

error:
    if (obj) K32OBJ_DecCount( obj );
    if (mapping) HeapFree( SystemHeap, 0, mapping );
    return 0;
}


/***********************************************************************
 *             CreateFileMapping32W   (KERNEL32.47)
 */
HANDLE32 CreateFileMapping32W( HFILE32 hFile, LPSECURITY_ATTRIBUTES attr,
                               DWORD protect, DWORD size_high, DWORD size_low,
                               LPCWSTR name )
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
HANDLE32 OpenFileMapping32A( DWORD access, BOOL32 inherit, LPCSTR name )
{
    K32OBJ *obj = K32OBJ_FindNameType( name, K32OBJ_MEM_MAPPED_FILE );
    if (!obj) return 0;
    return PROCESS_AllocHandle( obj, 0 );
}


/***********************************************************************
 *             OpenFileMapping32W   (KERNEL32.398)
 */
HANDLE32 OpenFileMapping32W( DWORD access, BOOL32 inherit, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE32 ret = OpenFileMapping32A( access, inherit, nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *             MapViewOfFile   (KERNEL32.385)
 */
LPVOID MapViewOfFile( HANDLE32 mapping, DWORD access, DWORD offset_high,
                      DWORD offset_low, DWORD count )
{
    return MapViewOfFileEx( mapping, access, offset_high,
                            offset_low, count, NULL );
}


/***********************************************************************
 *             MapViewOfFileEx   (KERNEL32.386)
 */
LPVOID MapViewOfFileEx( HANDLE32 handle, DWORD access, DWORD offset_high,
                        DWORD offset_low, DWORD count, LPVOID addr )
{
    FILE_MAPPING *mapping;
    LPVOID ret;

    if (!(mapping = (FILE_MAPPING *)PROCESS_GetObjPtr( handle,
                                                     K32OBJ_MEM_MAPPED_FILE )))
        return NULL;

    ret = FILE_mmap(mapping->file, addr, mapping->size_high, mapping->size_low,
                    offset_high, offset_low, mapping->protect, MAP_PRIVATE );

    K32OBJ_DecCount( &mapping->header );
    return ret;
}


/***********************************************************************
 *             UnmapViewOfFile   (KERNEL32.540)
 */
BOOL32 UnmapViewOfFile( LPVOID addr )
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
