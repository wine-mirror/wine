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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#include "thread.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(virtual);
WINE_DECLARE_DEBUG_CHANNEL(module);

#ifndef MS_SYNC
#define MS_SYNC 0
#endif

/* File view */
typedef struct _FV
{
    struct _FV   *next;        /* Next view */
    struct _FV   *prev;        /* Prev view */
    void         *base;        /* Base address */
    UINT          size;        /* Size in bytes */
    UINT          flags;       /* Allocation flags */
    HANDLE        mapping;     /* Handle to the file mapping */
    HANDLERPROC   handlerProc; /* Fault handler */
    LPVOID        handlerArg;  /* Fault handler argument */
    BYTE          protect;     /* Protection for all pages at allocation time */
    BYTE          prot[1];     /* Protection byte for each page */
} FILE_VIEW;

/* Per-view flags */
#define VFLAG_SYSTEM     0x01
#define VFLAG_VALLOC     0x02  /* allocated by VirtualAlloc */

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

static CRITICAL_SECTION csVirtual;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csVirtual,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": csVirtual") }
};
static CRITICAL_SECTION csVirtual = { &critsect_debug, -1, 0, 0, 0, 0 };

#ifdef __i386__
/* These are always the same on an i386, and it will be faster this way */
# define page_mask  0xfff
# define page_shift 12
# define page_size  0x1000
/* Note: ADDRESS_SPACE_LIMIT is a Windows limit, you cannot change it.
 * If you are on Solaris you need to find a way to avoid having the system
 * allocate things above 0xc000000. Don't touch that define.
 */
# define ADDRESS_SPACE_LIMIT  ((void *)0xc0000000)  /* top of the user address space */
#else
static UINT page_shift;
static UINT page_mask;
static UINT page_size;
# define ADDRESS_SPACE_LIMIT  0   /* no limit needed on other platforms */
#endif  /* __i386__ */
#define granularity_mask 0xffff  /* Allocation granularity (usually 64k) */

#define ROUND_ADDR(addr,mask) \
   ((void *)((UINT_PTR)(addr) & ~(mask)))

#define ROUND_SIZE(addr,size) \
   (((UINT)(size) + ((UINT_PTR)(addr) & page_mask) + page_mask) & ~page_mask)

#define VIRTUAL_DEBUG_DUMP_VIEW(view) \
   if (!TRACE_ON(virtual)); else VIRTUAL_DumpView(view)

static LPVOID VIRTUAL_mmap( int fd, LPVOID start, DWORD size, DWORD offset_low,
                            DWORD offset_high, int prot, int flags, BOOL *removable );


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
    char *addr = view->base;
    BYTE prot = view->prot[0];

    DPRINTF( "View: %p - %p", addr, addr + view->size - 1 );
    if (view->flags & VFLAG_SYSTEM)
        DPRINTF( " (system)\n" );
    else if (view->flags & VFLAG_VALLOC)
        DPRINTF( " (valloc)\n" );
    else if (view->mapping)
        DPRINTF( " %p\n", view->mapping );
    else
        DPRINTF( " (anonymous)\n");

    for (count = i = 1; i < view->size >> page_shift; i++, count++)
    {
        if (view->prot[i] == prot) continue;
        DPRINTF( "      %p - %p %s\n",
                 addr, addr + (count << page_shift) - 1, VIRTUAL_GetProtStr(prot) );
        addr += (count << page_shift);
        prot = view->prot[i];
        count = 0;
    }
    if (count)
        DPRINTF( "      %p - %p %s\n",
                 addr, addr + (count << page_shift) - 1, VIRTUAL_GetProtStr(prot) );
}


/***********************************************************************
 *           VIRTUAL_Dump
 */
void VIRTUAL_Dump(void)
{
    FILE_VIEW *view;
    DPRINTF( "\nDump of all virtual memory views:\n\n" );
    RtlEnterCriticalSection(&csVirtual);
    view = VIRTUAL_FirstView;
    while (view)
    {
        VIRTUAL_DumpView( view );
        view = view->next;
    }
    RtlLeaveCriticalSection(&csVirtual);
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
static FILE_VIEW *VIRTUAL_FindView( const void *addr ) /* [in] Address */
{
    FILE_VIEW *view;

    RtlEnterCriticalSection(&csVirtual);
    view = VIRTUAL_FirstView;
    while (view)
    {
        if (view->base > addr)
        {
            view = NULL;
            break;
        }
        if ((char*)view->base + view->size > (char*)addr) break;
        view = view->next;
    }
    RtlLeaveCriticalSection(&csVirtual);
    return view;
}


/***********************************************************************
 *           VIRTUAL_CreateView
 *
 * Create a new view and add it in the linked list.
 */
static FILE_VIEW *VIRTUAL_CreateView( void *base, UINT size, UINT flags,
                                      BYTE vprot, HANDLE mapping )
{
    FILE_VIEW *view, *prev;

    /* Create the view structure */

    assert( !((unsigned int)base & page_mask) );
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

    if (view->mapping &&
        NtDuplicateObject( GetCurrentProcess(), view->mapping,
                           GetCurrentProcess(), &view->mapping,
                           0, 0, DUPLICATE_SAME_ACCESS ))
    {
        free( view );
        return NULL;
    }

    /* Insert it in the linked list */

    RtlEnterCriticalSection(&csVirtual);
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
    RtlLeaveCriticalSection(&csVirtual);
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
static void VIRTUAL_DeleteView( FILE_VIEW *view ) /* [in] View */
{
    if (!(view->flags & VFLAG_SYSTEM))
        munmap( (void *)view->base, view->size );
    RtlEnterCriticalSection(&csVirtual);
    if (view->next) view->next->prev = view->prev;
    if (view->prev) view->prev->next = view->next;
    else VIRTUAL_FirstView = view->next;
    RtlLeaveCriticalSection(&csVirtual);
    if (view->mapping) NtClose( view->mapping );
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
            DWORD *state )  /* [out] Location to store mem state flag */
{
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
static BYTE VIRTUAL_GetProt( DWORD protect )  /* [in] Win32 protection flags */
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
        /* MSDN CreateFileMapping() states that if PAGE_WRITECOPY is given,
	 * that the hFile must have been opened with GENERIC_READ and
	 * GENERIC_WRITE access.  This is WRONG as tests show that you
	 * only need GENERIC_READ access (at least for Win9x,
	 * FIXME: what about NT?).  Thus, we don't put VPROT_WRITE in
	 * PAGE_WRITECOPY and PAGE_EXECUTE_WRITECOPY.
	 */
        vprot = VPROT_READ | VPROT_WRITECOPY;
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
        /* See comment for PAGE_WRITECOPY above */
        vprot = VPROT_EXEC | VPROT_READ | VPROT_WRITECOPY;
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
static BOOL VIRTUAL_SetProt( FILE_VIEW *view, /* [in] Pointer to view */
                             void *base,      /* [in] Starting address */
                             UINT size,       /* [in] Size in bytes */
                             BYTE vprot )     /* [in] Protections to use */
{
    TRACE("%p-%p %s\n",
          base, (char *)base + size - 1, VIRTUAL_GetProtStr( vprot ) );

    if (mprotect( base, size, VIRTUAL_GetUnixProt(vprot) ))
        return FALSE;  /* FIXME: last error */

    memset( view->prot + (((char *)base - (char *)view->base) >> page_shift),
            vprot, size >> page_shift );
    VIRTUAL_DEBUG_DUMP_VIEW( view );
    return TRUE;
}


/***********************************************************************
 *           anon_mmap_aligned
 *
 * Create an anonymous mapping aligned to the allocation granularity.
 */
static NTSTATUS anon_mmap_aligned( void **addr, unsigned int size, int prot, int flags )
{
    void *ptr, *base = *addr;
    unsigned int view_size = size + (base ? 0 : granularity_mask + 1);

    if ((ptr = wine_anon_mmap( base, view_size, prot, flags )) == (void *)-1)
    {
        if (errno == ENOMEM) return STATUS_NO_MEMORY;
        return STATUS_INVALID_PARAMETER;
    }

    if (!base)
    {
        /* Release the extra memory while keeping the range
         * starting on the granularity boundary. */
        if ((unsigned int)ptr & granularity_mask)
        {
            unsigned int extra = granularity_mask + 1 - ((unsigned int)ptr & granularity_mask);
            munmap( ptr, extra );
            ptr = (char *)ptr + extra;
            view_size -= extra;
        }
        if (view_size > size)
            munmap( (char *)ptr + size, view_size - size );
    }
    else if (ptr != base)
    {
        /* We couldn't get the address we wanted */
        munmap( ptr, view_size );
        return STATUS_CONFLICTING_ADDRESSES;
    }
    *addr = ptr;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           do_relocations
 *
 * Apply the relocations to a mapped PE image
 */
static int do_relocations( char *base, const IMAGE_DATA_DIRECTORY *dir,
                           int delta, DWORD total_size )
{
    IMAGE_BASE_RELOCATION *rel;

    TRACE_(module)( "relocating from %p-%p to %p-%p\n",
                    base - delta, base - delta + total_size, base, base + total_size );

    for (rel = (IMAGE_BASE_RELOCATION *)(base + dir->VirtualAddress);
         ((char *)rel < base + dir->VirtualAddress + dir->Size) && rel->SizeOfBlock;
         rel = (IMAGE_BASE_RELOCATION*)((char*)rel + rel->SizeOfBlock) )
    {
        char *page = base + rel->VirtualAddress;
        WORD *TypeOffset = (WORD *)(rel + 1);
        int i, count = (rel->SizeOfBlock - sizeof(*rel)) / sizeof(*TypeOffset);

        if (!count) continue;

        /* sanity checks */
        if ((char *)rel + rel->SizeOfBlock > base + dir->VirtualAddress + dir->Size ||
            page > base + total_size)
        {
            ERR_(module)("invalid relocation %p,%lx,%ld at %p,%lx,%lx\n",
                         rel, rel->VirtualAddress, rel->SizeOfBlock,
                         base, dir->VirtualAddress, dir->Size );
            return 0;
        }

        TRACE_(module)("%ld relocations for page %lx\n", rel->SizeOfBlock, rel->VirtualAddress);

        /* patching in reverse order */
        for (i = 0 ; i < count; i++)
        {
            int offset = TypeOffset[i] & 0xFFF;
            int type = TypeOffset[i] >> 12;
            switch(type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                break;
            case IMAGE_REL_BASED_HIGH:
                *(short*)(page+offset) += HIWORD(delta);
                break;
            case IMAGE_REL_BASED_LOW:
                *(short*)(page+offset) += LOWORD(delta);
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                *(int*)(page+offset) += delta;
                /* FIXME: if this is an exported address, fire up enhanced logic */
                break;
            default:
                FIXME_(module)("Unknown/unsupported fixup type %d.\n", type);
                break;
            }
        }
    }
    return 1;
}


/***********************************************************************
 *           map_image
 *
 * Map an executable (PE format) image into memory.
 */
static NTSTATUS map_image( HANDLE hmapping, int fd, char *base, DWORD total_size,
                           DWORD header_size, int shared_fd, BOOL removable, PVOID *addr_ptr )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_DATA_DIRECTORY *imports;
    NTSTATUS status = STATUS_INVALID_IMAGE_FORMAT;  /* generic error (FIXME) */
    int i, pos;
    FILE_VIEW *view;
    char *ptr;

    /* zero-map the whole range */

    if (base < (char *)0x110000 ||  /* make sure the DOS area remains free */
        (ptr = wine_anon_mmap( base, total_size,
                               PROT_READ | PROT_WRITE | PROT_EXEC, 0 )) == (char *)-1)
    {
        ptr = wine_anon_mmap( NULL, total_size,
                            PROT_READ | PROT_WRITE | PROT_EXEC, 0 );
        if (ptr == (char *)-1)
        {
            ERR_(module)("Not enough memory for module (%ld bytes)\n", total_size);
            goto error;
        }
    }
    TRACE_(module)( "mapped PE file at %p-%p\n", ptr, ptr + total_size );

    /* map the header */

    if (VIRTUAL_mmap( fd, ptr, header_size, 0, 0, PROT_READ,
                      MAP_PRIVATE | MAP_FIXED, &removable ) == (char *)-1) goto error;
    dos = (IMAGE_DOS_HEADER *)ptr;
    nt = (IMAGE_NT_HEADERS *)(ptr + dos->e_lfanew);
    if ((char *)(nt + 1) > ptr + header_size) goto error;

    sec = (IMAGE_SECTION_HEADER*)((char*)&nt->OptionalHeader+nt->FileHeader.SizeOfOptionalHeader);
    if ((char *)(sec + nt->FileHeader.NumberOfSections) > ptr + header_size) goto error;

    imports = nt->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_IMPORT;
    if (!imports->Size || !imports->VirtualAddress) imports = NULL;

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
                          sec->Name, ptr + sec->VirtualAddress,
                          sec->PointerToRawData, pos, sec->SizeOfRawData,
                          size, sec->Characteristics );
            if (VIRTUAL_mmap( shared_fd, ptr + sec->VirtualAddress, size,
                              pos, 0, PROT_READ|PROT_WRITE|PROT_EXEC,
                              MAP_SHARED|MAP_FIXED, NULL ) == (void *)-1)
            {
                ERR_(module)( "Could not map shared section %.8s\n", sec->Name );
                goto error;
            }

            /* check if the import directory falls inside this section */
            if (imports && imports->VirtualAddress >= sec->VirtualAddress &&
                imports->VirtualAddress < sec->VirtualAddress + size)
            {
                UINT_PTR base = imports->VirtualAddress & ~page_mask;
                UINT_PTR end = base + ROUND_SIZE( imports->VirtualAddress, imports->Size );
                if (end > sec->VirtualAddress + size) end = sec->VirtualAddress + size;
                if (end > base) VIRTUAL_mmap( shared_fd, ptr + base, end - base,
                                              pos + (base - sec->VirtualAddress), 0,
                                              PROT_READ|PROT_WRITE|PROT_EXEC,
                                              MAP_PRIVATE|MAP_FIXED, NULL );
            }
            pos += size;
            continue;
        }

        TRACE_(module)( "mapping section %.8s at %p off %lx size %lx flags %lx\n",
                        sec->Name, ptr + sec->VirtualAddress,
                        sec->PointerToRawData, sec->SizeOfRawData,
                        sec->Characteristics );

        if ((sec->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
            !(sec->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)) continue;
        if (!sec->PointerToRawData || !sec->SizeOfRawData) continue;

        /* Note: if the section is not aligned properly VIRTUAL_mmap will magically
         *       fall back to read(), so we don't need to check anything here.
         */
        if (VIRTUAL_mmap( fd, ptr + sec->VirtualAddress, sec->SizeOfRawData,
                          sec->PointerToRawData, 0, PROT_READ|PROT_WRITE|PROT_EXEC,
                          MAP_PRIVATE | MAP_FIXED, &removable ) == (void *)-1)
        {
            ERR_(module)( "Could not map section %.8s, file probably truncated\n", sec->Name );
            goto error;
        }

        if ((sec->SizeOfRawData < sec->Misc.VirtualSize) && (sec->SizeOfRawData & page_mask))
        {
            DWORD end = ROUND_SIZE( 0, sec->SizeOfRawData );
            if (end > sec->Misc.VirtualSize) end = sec->Misc.VirtualSize;
            TRACE_(module)("clearing %p - %p\n",
                           ptr + sec->VirtualAddress + sec->SizeOfRawData,
                           ptr + sec->VirtualAddress + end );
            memset( ptr + sec->VirtualAddress + sec->SizeOfRawData, 0,
                    end - sec->SizeOfRawData );
        }
    }


    /* perform base relocation, if necessary */

    if (ptr != base)
    {
        const IMAGE_DATA_DIRECTORY *relocs;

        relocs = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (!relocs->VirtualAddress || !relocs->Size)
        {
            if (nt->OptionalHeader.ImageBase == 0x400000)
                ERR("Standard load address for a Win32 program (0x00400000) not available - security-patched kernel ?\n");
            else
                ERR( "FATAL: Need to relocate module from addr %lx, but there are no relocation records\n",
                     nt->OptionalHeader.ImageBase );
            goto error;
        }

        /* FIXME: If we need to relocate a system DLL (base > 2GB) we should
         *        really make sure that the *new* base address is also > 2GB.
         *        Some DLLs really check the MSB of the module handle :-/
         */
        if ((nt->OptionalHeader.ImageBase & 0x80000000) && !((DWORD)base & 0x80000000))
            ERR( "Forced to relocate system DLL (base > 2GB). This is not good.\n" );

        if (!do_relocations( ptr, relocs, ptr - base, total_size ))
        {
            goto error;
        }
    }

    if (removable) hmapping = 0;  /* don't keep handle open on removable media */
    if (!(view = VIRTUAL_CreateView( ptr, total_size, 0,
                                     VPROT_COMMITTED | VPROT_READ | VPROT_WRITE |
                                     VPROT_EXEC | VPROT_WRITECOPY | VPROT_IMAGE, hmapping )))
    {
        status = STATUS_NO_MEMORY;
        goto error;
    }

    /* set the image protections */

    VIRTUAL_SetProt( view, ptr, header_size, VPROT_COMMITTED | VPROT_READ );
    sec = (IMAGE_SECTION_HEADER*)((char *)&nt->OptionalHeader+nt->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        DWORD size = ROUND_SIZE( sec->VirtualAddress, sec->Misc.VirtualSize );
        BYTE vprot = VPROT_COMMITTED;
        if (sec->Characteristics & IMAGE_SCN_MEM_READ)    vprot |= VPROT_READ;
        if (sec->Characteristics & IMAGE_SCN_MEM_WRITE)   vprot |= VPROT_WRITE|VPROT_WRITECOPY;
        if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) vprot |= VPROT_EXEC;

        /* make sure the import directory is writable */
        if (imports && imports->VirtualAddress >= sec->VirtualAddress &&
            imports->VirtualAddress < sec->VirtualAddress + size)
            vprot |= VPROT_READ|VPROT_WRITE|VPROT_WRITECOPY;

        VIRTUAL_SetProt( view, ptr + sec->VirtualAddress, size, vprot );
    }

    *addr_ptr = ptr;
    return STATUS_SUCCESS;

 error:
    if (ptr != (char *)-1) munmap( ptr, total_size );
    return status;
}


/***********************************************************************
 *           is_current_process
 *
 * Check whether a process handle is for the current process.
 */
static BOOL is_current_process( HANDLE handle )
{
    BOOL ret = FALSE;

    if (handle == GetCurrentProcess()) return TRUE;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = handle;
        if (!wine_server_call( req ))
            ret = ((DWORD)reply->pid == GetCurrentProcessId());
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           VIRTUAL_Init
 */
#ifndef page_mask
DECL_GLOBAL_CONSTRUCTOR(VIRTUAL_Init)
{
    page_size = getpagesize();
    page_mask = page_size - 1;
    /* Make sure we have a power of 2 */
    assert( !(page_size & page_mask) );
    page_shift = 0;
    while ((1 << page_shift) != page_size) page_shift++;
}
#endif  /* page_mask */


/***********************************************************************
 *           VIRTUAL_SetFaultHandler
 */
BOOL VIRTUAL_SetFaultHandler( LPCVOID addr, HANDLERPROC proc, LPVOID arg )
{
    FILE_VIEW *view;

    if (!(view = VIRTUAL_FindView( addr ))) return FALSE;
    view->handlerProc = proc;
    view->handlerArg  = arg;
    return TRUE;
}

/***********************************************************************
 *           VIRTUAL_HandleFault
 */
DWORD VIRTUAL_HandleFault( LPCVOID addr )
{
    FILE_VIEW *view = VIRTUAL_FindView( addr );
    DWORD ret = EXCEPTION_ACCESS_VIOLATION;

    if (view)
    {
        if (view->handlerProc)
        {
            if (view->handlerProc(view->handlerArg, addr)) ret = 0;  /* handled */
        }
        else
        {
            BYTE vprot = view->prot[((char *)addr - (char *)view->base) >> page_shift];
            void *page = (void *)((UINT_PTR)addr & ~page_mask);
            char *stack = NtCurrentTeb()->Tib.StackLimit;
            if (vprot & VPROT_GUARD)
            {
                VIRTUAL_SetProt( view, page, page_mask + 1, vprot & ~VPROT_GUARD );
                ret = STATUS_GUARD_PAGE_VIOLATION;
            }
            /* is it inside the stack guard page? */
            if (((char *)addr >= stack) && ((char *)addr < stack + (page_mask+1)))
                ret = STATUS_STACK_OVERFLOW;
        }
    }
    return ret;
}



/***********************************************************************
 *           unaligned_mmap
 *
 * Linux kernels before 2.4.x can support non page-aligned offsets, as
 * long as the offset is aligned to the filesystem block size. This is
 * a big performance gain so we want to take advantage of it.
 *
 * However, when we use 64-bit file support this doesn't work because
 * glibc rejects unaligned offsets. Also glibc 2.1.3 mmap64 is broken
 * in that it rounds unaligned offsets down to a page boundary. For
 * these reasons we do a direct system call here.
 */
static void *unaligned_mmap( void *addr, size_t length, unsigned int prot,
                             unsigned int flags, int fd, unsigned int offset_low,
                             unsigned int offset_high )
{
#if defined(linux) && defined(__i386__) && defined(__GNUC__)
    if (!offset_high && (offset_low & page_mask))
    {
        int ret;

        struct
        {
            void        *addr;
            unsigned int length;
            unsigned int prot;
            unsigned int flags;
            unsigned int fd;
            unsigned int offset;
        } args;

        args.addr   = addr;
        args.length = length;
        args.prot   = prot;
        args.flags  = flags;
        args.fd     = fd;
        args.offset = offset_low;

        __asm__ __volatile__("push %%ebx\n\t"
                             "movl %2,%%ebx\n\t"
                             "int $0x80\n\t"
                             "popl %%ebx"
                             : "=a" (ret)
                             : "0" (90), /* SYS_mmap */
                               "g" (&args) );
        if (ret < 0 && ret > -4096)
        {
            errno = -ret;
            ret = -1;
        }
        return (void *)ret;
    }
#endif
    return mmap( addr, length, prot, flags, fd, ((off_t)offset_high << 32) | offset_low );
}


/***********************************************************************
 *           VIRTUAL_mmap
 *
 * Wrapper for mmap() that handles anonymous mappings portably,
 * and falls back to read if mmap of a file fails.
 */
static LPVOID VIRTUAL_mmap( int fd, LPVOID start, DWORD size,
                            DWORD offset_low, DWORD offset_high,
                            int prot, int flags, BOOL *removable )
{
    LPVOID ret;
    off_t offset;
    BOOL is_shared_write = FALSE;

    if (fd == -1) return wine_anon_mmap( start, size, prot, flags );

    if (prot & PROT_WRITE)
    {
#ifdef MAP_SHARED
        if (flags & MAP_SHARED) is_shared_write = TRUE;
#endif
#ifdef MAP_PRIVATE
        if (!(flags & MAP_PRIVATE)) is_shared_write = TRUE;
#endif
    }

    if (removable && *removable)
    {
        /* if on removable media, try using read instead of mmap */
        if (!is_shared_write) goto fake_mmap;
        *removable = FALSE;
    }

    if ((ret = unaligned_mmap( start, size, prot, flags, fd,
                               offset_low, offset_high )) != (LPVOID)-1) return ret;

    /* mmap() failed; if this is because the file offset is not    */
    /* page-aligned (EINVAL), or because the underlying filesystem */
    /* does not support mmap() (ENOEXEC,ENODEV), we do it by hand. */

    if ((errno != ENOEXEC) && (errno != EINVAL) && (errno != ENODEV)) return ret;
    if (is_shared_write) return ret;  /* we cannot fake shared write mappings */

 fake_mmap:
    /* Reserve the memory with an anonymous mmap */
    ret = wine_anon_mmap( start, size, PROT_READ | PROT_WRITE, flags );
    if (ret == (LPVOID)-1) return ret;
    /* Now read in the file */
    offset = ((off_t)offset_high << 32) | offset_low;
    pread( fd, ret, size, offset );
    mprotect( ret, size, prot );  /* Set the right protection */
    return ret;
}


/***********************************************************************
 *             NtAllocateVirtualMemory   (NTDLL.@)
 *             ZwAllocateVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtAllocateVirtualMemory( HANDLE process, PVOID *ret, PVOID addr,
                                         ULONG *size_ptr, ULONG type, ULONG protect )
{
    FILE_VIEW *view;
    void *base;
    BYTE vprot;
    DWORD size = *size_ptr;

    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }

    TRACE("%p %08lx %lx %08lx\n", addr, size, type, protect );

    /* Round parameters to a page boundary */

    if (size > 0x7fc00000) return STATUS_WORKING_SET_LIMIT_RANGE; /* 2Gb - 4Mb */

    if (addr)
    {
        if (type & MEM_RESERVE) /* Round down to 64k boundary */
            base = ROUND_ADDR( addr, granularity_mask );
        else
            base = ROUND_ADDR( addr, page_mask );
        size = (((UINT_PTR)addr + size + page_mask) & ~page_mask) - (UINT_PTR)base;

        /* disallow low 64k, wrap-around and kernel space */
        if (((char *)base <= (char *)granularity_mask) ||
            ((char *)base + size < (char *)base) ||
            (ADDRESS_SPACE_LIMIT && ((char *)base + size > (char *)ADDRESS_SPACE_LIMIT)))
            return STATUS_INVALID_PARAMETER;
    }
    else
    {
        base = NULL;
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

    if (type & MEM_SYSTEM)
    {
        vprot = VIRTUAL_GetProt( protect ) | VPROT_COMMITTED;
        if (type & MEM_IMAGE) vprot |= VPROT_IMAGE;
    }
    else
    {
        if (!(type & (MEM_COMMIT | MEM_RESERVE)) || (type & ~(MEM_COMMIT | MEM_RESERVE)))
        {
            WARN("called with wrong alloc type flags (%08lx) !\n", type);
            return STATUS_INVALID_PARAMETER;
        }
        if (type & MEM_COMMIT)
            vprot = VIRTUAL_GetProt( protect ) | VPROT_COMMITTED;
        else
            vprot = 0;
    }

    /* Reserve the memory */

    if (type & MEM_SYSTEM)
    {
        if (!(view = VIRTUAL_CreateView( base, size, VFLAG_VALLOC | VFLAG_SYSTEM, vprot, 0 )))
            return STATUS_NO_MEMORY;
    }
    else if ((type & MEM_RESERVE) || !base)
    {
        NTSTATUS res = anon_mmap_aligned( &base, size, VIRTUAL_GetUnixProt( vprot ), 0 );
        if (res) return res;

        if (!(view = VIRTUAL_CreateView( base, size, VFLAG_VALLOC, vprot, 0 )))
        {
            munmap( base, size );
            return STATUS_NO_MEMORY;
        }
    }
    else
    {
        /* Commit the pages */

        if (!(view = VIRTUAL_FindView( base )) ||
            ((char *)base + size > (char *)view->base + view->size)) return STATUS_NOT_MAPPED_VIEW;

        if (!VIRTUAL_SetProt( view, base, size, vprot )) return STATUS_ACCESS_DENIED;
    }

    *ret = base;
    *size_ptr = size;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             NtFreeVirtualMemory   (NTDLL.@)
 *             ZwFreeVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtFreeVirtualMemory( HANDLE process, PVOID *addr_ptr, ULONG *size_ptr, ULONG type )
{
    FILE_VIEW *view;
    char *base;
    LPVOID addr = *addr_ptr;
    DWORD size = *size_ptr;

    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }

    TRACE("%p %08lx %lx\n", addr, size, type );

    /* Fix the parameters */

    size = ROUND_SIZE( addr, size );
    base = ROUND_ADDR( addr, page_mask );

    if (!(view = VIRTUAL_FindView( base )) ||
        (base + size > (char *)view->base + view->size) ||
        !(view->flags & VFLAG_VALLOC))
        return STATUS_INVALID_PARAMETER;

    /* Check the type */

    if (type & MEM_SYSTEM)
    {
        /* return the values that the caller should use to unmap the area */
        *addr_ptr = view->base;
        *size_ptr = view->size;
        view->flags |= VFLAG_SYSTEM;
        VIRTUAL_DeleteView( view );
        return STATUS_SUCCESS;
    }

    if ((type != MEM_DECOMMIT) && (type != MEM_RELEASE))
    {
        ERR("called with wrong free type flags (%08lx) !\n", type);
        return STATUS_INVALID_PARAMETER;
    }

    /* Free the pages */

    if (type == MEM_RELEASE)
    {
        if (size || (base != view->base)) return STATUS_INVALID_PARAMETER;
        VIRTUAL_DeleteView( view );
    }
    else
    {
        /* Decommit the pages by remapping zero-pages instead */

        if (wine_anon_mmap( (LPVOID)base, size, VIRTUAL_GetUnixProt(0), MAP_FIXED ) != (LPVOID)base)
            ERR( "Could not remap pages, expect trouble\n" );
        if (!VIRTUAL_SetProt( view, base, size, 0 )) return STATUS_ACCESS_DENIED;  /* FIXME */
    }

    *addr_ptr = base;
    *size_ptr = size;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             NtProtectVirtualMemory   (NTDLL.@)
 *             ZwProtectVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtProtectVirtualMemory( HANDLE process, PVOID *addr_ptr, ULONG *size_ptr,
                                        ULONG new_prot, ULONG *old_prot )
{
    FILE_VIEW *view;
    char *base;
    UINT i;
    BYTE vprot, *p;
    DWORD prot, size = *size_ptr;
    LPVOID addr = *addr_ptr;

    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }

    TRACE("%p %08lx %08lx\n", addr, size, new_prot );

    /* Fix the parameters */

    size = ROUND_SIZE( addr, size );
    base = ROUND_ADDR( addr, page_mask );

    if (!(view = VIRTUAL_FindView( base )) ||
        (base + size > (char *)view->base + view->size))
        return STATUS_INVALID_PARAMETER;

    /* Make sure all the pages are committed */

    p = view->prot + ((base - (char *)view->base) >> page_shift);
    VIRTUAL_GetWin32Prot( *p, &prot, NULL );
    for (i = size >> page_shift; i; i--, p++)
    {
        if (!(*p & VPROT_COMMITTED)) return STATUS_INVALID_PARAMETER;
    }

    if (old_prot) *old_prot = prot;
    vprot = VIRTUAL_GetProt( new_prot ) | VPROT_COMMITTED;
    if (!VIRTUAL_SetProt( view, base, size, vprot )) return STATUS_ACCESS_DENIED;

    *addr_ptr = base;
    *size_ptr = size;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             NtQueryVirtualMemory   (NTDLL.@)
 *             ZwQueryVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryVirtualMemory( HANDLE process, LPCVOID addr,
                                      MEMORY_INFORMATION_CLASS info_class, PVOID buffer,
                                      ULONG len, ULONG *res_len )
{
    FILE_VIEW *view;
    char *base, *alloc_base = 0;
    UINT size = 0;
    MEMORY_BASIC_INFORMATION *info = buffer;

    if (info_class != MemoryBasicInformation) return STATUS_INVALID_INFO_CLASS;
    if (ADDRESS_SPACE_LIMIT && addr >= ADDRESS_SPACE_LIMIT)
        return STATUS_WORKING_SET_LIMIT_RANGE;  /* FIXME */

    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }

    base = ROUND_ADDR( addr, page_mask );

    /* Find the view containing the address */

    RtlEnterCriticalSection(&csVirtual);
    view = VIRTUAL_FirstView;
    for (;;)
    {
        if (!view)
        {
            size = (char *)ADDRESS_SPACE_LIMIT - alloc_base;
            break;
        }
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
        view = view->next;
    }
    RtlLeaveCriticalSection(&csVirtual);

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
        VIRTUAL_GetWin32Prot( view->protect, &info->AllocationProtect, NULL );
        if (view->protect & VPROT_IMAGE) info->Type = MEM_IMAGE;
        else if (view->flags & VFLAG_VALLOC) info->Type = MEM_PRIVATE;
        else info->Type = MEM_MAPPED;
    }

    info->BaseAddress    = (LPVOID)base;
    info->AllocationBase = (LPVOID)alloc_base;
    info->RegionSize     = size - (base - alloc_base);
    if (res_len) *res_len = sizeof(*info);
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             NtLockVirtualMemory   (NTDLL.@)
 *             ZwLockVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtLockVirtualMemory( HANDLE process, PVOID *addr, ULONG *size, ULONG unknown )
{
    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             NtUnlockVirtualMemory   (NTDLL.@)
 *             ZwUnlockVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtUnlockVirtualMemory( HANDLE process, PVOID *addr, ULONG *size, ULONG unknown )
{
    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }
    return STATUS_SUCCESS;
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
    BYTE vprot;
    DWORD len = attr->ObjectName ? attr->ObjectName->Length : 0;

    /* Check parameters */

    if (len > MAX_PATH*sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    vprot = VIRTUAL_GetProt( protect );
    if (sec_flags & SEC_RESERVE)
    {
        if (file) return STATUS_INVALID_PARAMETER;
    }
    else vprot |= VPROT_COMMITTED;
    if (sec_flags & SEC_NOCACHE) vprot |= VPROT_NOCACHE;
    if (sec_flags & SEC_IMAGE) vprot |= VPROT_IMAGE;

    /* Create the server object */

    SERVER_START_REQ( create_mapping )
    {
        req->file_handle = file;
        req->size_high   = size ? size->s.HighPart : 0;
        req->size_low    = size ? size->s.LowPart : 0;
        req->protect     = vprot;
        req->access      = access;
        req->inherit     = (attr->Attributes & OBJ_INHERIT) != 0;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *handle = reply->handle;
    }
    SERVER_END_REQ;
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
        req->inherit = (attr->Attributes & OBJ_INHERIT) != 0;
        wine_server_add_data( req, attr->ObjectName->Buffer, len );
        if (!(ret = wine_server_call( req ))) *handle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *             NtMapViewOfSection   (NTDLL.@)
 *             ZwMapViewOfSection   (NTDLL.@)
 */
NTSTATUS WINAPI NtMapViewOfSection( HANDLE handle, HANDLE process, PVOID *addr_ptr, ULONG zero_bits,
                                    ULONG commit_size, const LARGE_INTEGER *offset, ULONG *size_ptr,
                                    SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect )
{
    FILE_VIEW *view;
    NTSTATUS res;
    UINT size = 0;
    int flags = MAP_PRIVATE;
    int unix_handle = -1;
    int prot;
    void *base, *ptr = (void *)-1, *ret;
    DWORD size_low, size_high, header_size, shared_size;
    HANDLE shared_file;
    BOOL removable;

    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }

    TRACE("handle=%p addr=%p off=%lx%08lx size=%x access=%lx\n",
          handle, *addr_ptr, offset->s.HighPart, offset->s.LowPart, size, protect );

    /* Check parameters */

    if ((offset->s.LowPart & granularity_mask) ||
        (*addr_ptr && ((UINT_PTR)*addr_ptr & granularity_mask)))
        return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( get_mapping_info )
    {
        req->handle = handle;
        res = wine_server_call( req );
        prot        = reply->protect;
        base        = reply->base;
        size_low    = reply->size_low;
        size_high   = reply->size_high;
        header_size = reply->header_size;
        shared_file = reply->shared_file;
        shared_size = reply->shared_size;
        removable   = reply->removable;
    }
    SERVER_END_REQ;
    if (res) return res;

    if ((res = wine_server_handle_to_fd( handle, 0, &unix_handle, NULL, NULL ))) return res;

    if (prot & VPROT_IMAGE)
    {
        if (shared_file)
        {
            int shared_fd;

            if ((res = wine_server_handle_to_fd( shared_file, GENERIC_READ, &shared_fd,
                                                 NULL, NULL ))) goto error;
            res = map_image( handle, unix_handle, base, size_low, header_size,
                             shared_fd, removable, addr_ptr );
            wine_server_release_fd( shared_file, shared_fd );
            NtClose( shared_file );
        }
        else
        {
            res = map_image( handle, unix_handle, base, size_low, header_size,
                             -1, removable, addr_ptr );
        }
        wine_server_release_fd( handle, unix_handle );
        if (!res) *size_ptr = size_low;
        return res;
    }

    if (size_high)
        ERR("Sizes larger than 4Gb not supported\n");

    if ((offset->s.LowPart >= size_low) ||
        (*size_ptr > size_low - offset->s.LowPart))
    {
        res = STATUS_INVALID_PARAMETER;
        goto error;
    }
    if (*size_ptr) size = ROUND_SIZE( offset->s.LowPart, *size_ptr );
    else size = size_low - offset->s.LowPart;

    switch(protect)
    {
    case PAGE_NOACCESS:
        break;
    case PAGE_READWRITE:
    case PAGE_EXECUTE_READWRITE:
        if (!(prot & VPROT_WRITE))
        {
            res = STATUS_INVALID_PARAMETER;
            goto error;
        }
        flags = MAP_SHARED;
        /* fall through */
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_WRITECOPY:
        if (prot & VPROT_READ) break;
        /* fall through */
    default:
        res = STATUS_INVALID_PARAMETER;
        goto error;
    }

    /* FIXME: If a mapping is created with SEC_RESERVE and a process,
     * which has a view of this mapping commits some pages, they will
     * appear commited in all other processes, which have the same
     * view created. Since we don`t support this yet, we create the
     * whole mapping commited.
     */
    prot |= VPROT_COMMITTED;

    /* Reserve a properly aligned area */

    if ((res = anon_mmap_aligned( addr_ptr, size, PROT_NONE, 0 ))) goto error;
    ptr = *addr_ptr;

    /* Map the file */

    TRACE("handle=%p size=%x offset=%lx\n", handle, size, offset->s.LowPart );

    ret = VIRTUAL_mmap( unix_handle, ptr, size, offset->s.LowPart, offset->s.HighPart,
                        VIRTUAL_GetUnixProt( prot ), flags | MAP_FIXED, &removable );
    if (ret != ptr)
    {
        ERR( "VIRTUAL_mmap %p %x %lx%08lx failed\n",
             ptr, size, offset->s.HighPart, offset->s.LowPart );
        res = STATUS_NO_MEMORY;  /* FIXME */
        goto error;
    }
    if (removable) handle = 0;  /* don't keep handle open on removable media */

    if (!(view = VIRTUAL_CreateView( ptr, size, 0, prot, handle )))
    {
        res = STATUS_NO_MEMORY;
        goto error;
    }
    wine_server_release_fd( handle, unix_handle );
    *size_ptr = size;
    return STATUS_SUCCESS;

error:
    wine_server_release_fd( handle, unix_handle );
    if (ptr != (void *)-1) munmap( ptr, size );
    return res;
}


/***********************************************************************
 *             NtUnmapViewOfSection   (NTDLL.@)
 *             ZwUnmapViewOfSection   (NTDLL.@)
 */
NTSTATUS WINAPI NtUnmapViewOfSection( HANDLE process, PVOID addr )
{
    FILE_VIEW *view;
    void *base = ROUND_ADDR( addr, page_mask );

    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }
    if (!(view = VIRTUAL_FindView( base )) || (base != view->base)) return STATUS_INVALID_PARAMETER;
    VIRTUAL_DeleteView( view );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             NtFlushVirtualMemory   (NTDLL.@)
 *             ZwFlushVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushVirtualMemory( HANDLE process, LPCVOID *addr_ptr,
                                      ULONG *size_ptr, ULONG unknown )
{
    FILE_VIEW *view;
    void *addr = ROUND_ADDR( *addr_ptr, page_mask );

    if (!is_current_process( process ))
    {
        ERR("Unsupported on other process\n");
        return STATUS_ACCESS_DENIED;
    }
    if (!(view = VIRTUAL_FindView( addr ))) return STATUS_INVALID_PARAMETER;
    if (!*size_ptr) *size_ptr = view->size;
    *addr_ptr = addr;
    if (!msync( addr, *size_ptr, MS_SYNC )) return STATUS_SUCCESS;
    return STATUS_NOT_MAPPED_DATA;
}


/***********************************************************************
 *             NtReadVirtualMemory   (NTDLL.@)
 *             ZwReadVirtualMemory   (NTDLL.@)
 */
NTSTATUS WINAPI NtReadVirtualMemory( HANDLE process, const void *addr, void *buffer,
                                     SIZE_T size, SIZE_T *bytes_read )
{
    NTSTATUS status;

    SERVER_START_REQ( read_process_memory )
    {
        req->handle = process;
        req->addr   = (void *)addr;
        wine_server_set_reply( req, buffer, size );
        if ((status = wine_server_call( req ))) size = 0;
    }
    SERVER_END_REQ;
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
    static const unsigned int zero;
    unsigned int first_offset, last_offset, first_mask, last_mask;
    NTSTATUS status;

    if (!size) return STATUS_INVALID_PARAMETER;

    /* compute the mask for the first int */
    first_mask = ~0;
    first_offset = (unsigned int)addr % sizeof(int);
    memset( &first_mask, 0, first_offset );

    /* compute the mask for the last int */
    last_offset = (size + first_offset) % sizeof(int);
    last_mask = 0;
    memset( &last_mask, 0xff, last_offset ? last_offset : sizeof(int) );

    SERVER_START_REQ( write_process_memory )
    {
        req->handle     = process;
        req->addr       = (char *)addr - first_offset;
        req->first_mask = first_mask;
        req->last_mask  = last_mask;
        if (first_offset) wine_server_add_data( req, &zero, first_offset );
        wine_server_add_data( req, buffer, size );
        if (last_offset) wine_server_add_data( req, &zero, sizeof(int) - last_offset );

        if ((status = wine_server_call( req ))) size = 0;
    }
    SERVER_END_REQ;
    if (bytes_written) *bytes_written = size;
    return status;
}
