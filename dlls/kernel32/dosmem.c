/*
 * DOS memory emulation
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Marcus Meissner
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

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "excpt.h"
#include "winternl.h"
#include "wine/winbase16.h"

#include "kernel16_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosmem);
WINE_DECLARE_DEBUG_CHANNEL(selector);

WORD DOSMEM_0000H;        /* segment at 0:0 */
WORD DOSMEM_BiosDataSeg;  /* BIOS data segment at 0x40:0 */
WORD DOSMEM_BiosSysSeg;   /* BIOS ROM segment at 0xf000:0 */

/* DOS memory highest address (including HMA) */
#define DOSMEM_SIZE             0x110000
#define DOSMEM_64KB             0x10000

/* when looking at DOS and real mode memory, we activate in three different
 * modes, depending the situation.
 * 1/ By default (protected mode), the first MB of memory (actually 0x110000,
 *    when you also look at the HMA part) is always reserved, whatever you do.
 *    We allocated some PM selectors to this memory, even if this area is not
 *    committed at startup
 * 2/ if a program tries to use the memory through the selectors, we actually
 *    commit this memory, made of: BIOS segment, but also some system 
 *    information, usually low in memory that we map for the circumstance also
 *    in the BIOS segment, so that we keep the low memory protected (for NULL
 *    pointer deref catching for example). In this case, we're still in PM
 *    mode, accessing part of the "physical" real mode memory. In fact, we don't
 *    map all the first meg, we keep 64k uncommitted to still catch NULL 
 *    pointers dereference
 * 3/ if the process enters the real mode, then we (also) commit the full first
 *    MB of memory (and also initialize the DOS structures in it).
 */

/* DOS memory base (linear in process address space) */
static char *DOSMEM_dosmem;
/* number of bytes protected from _dosmem. 0 when DOS memory is initialized, 
 * 64k otherwise to trap NULL pointers deref */
static DWORD DOSMEM_protect;

static LONG WINAPI dosmem_handler(EXCEPTION_POINTERS* except);

struct winedos_exports winedos;

BOOL load_winedos(void)
{
    static HANDLE	hRunOnce /* = 0 */;
    static HMODULE      hWineDos /* = 0 */;

    /* FIXME: this isn't 100% thread safe, as we won't catch access to 1MB while
     * loading winedos (and may return uninitialized valued)
     */
    if (hWineDos) goto done;
    if (hRunOnce == 0)
    {
	HANDLE hEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
        if (InterlockedCompareExchangePointer( &hRunOnce, hEvent, 0 ) == 0)
	{
            HMODULE hModule;

	    /* ok, we're the winning thread */
            if (!VirtualProtect( DOSMEM_dosmem + DOSMEM_protect,
                                 DOSMEM_SIZE - DOSMEM_protect,
                                 PAGE_READWRITE, NULL ) ||
                !(hModule = LoadLibraryA( "winedos.dll" )))
            {
                ERR("Could not load winedos.dll, DOS subsystem unavailable\n");
                hModule = (HMODULE)1; /* not to try to load it again */
            }
            else
            {
#define GET_ADDR(func)  winedos.func = (void *)GetProcAddress( hModule, #func );
                GET_ADDR(AllocDosBlock);
                GET_ADDR(FreeDosBlock);
                GET_ADDR(ResizeDosBlock);
                GET_ADDR(inport);
                GET_ADDR(outport);
                GET_ADDR(EmulateInterruptPM);
                GET_ADDR(CallBuiltinHandler);
#undef GET_ADDR
            }
            RtlRemoveVectoredExceptionHandler( dosmem_handler );
            hWineDos = hModule;
            SetEvent( hRunOnce );
            goto done;
	}
	/* someone beat us here... */
	CloseHandle( hEvent );
    }

    /* and wait for the winner to have finished */
    WaitForSingleObject( hRunOnce, INFINITE );
 done:
    return (hWineDos != (HMODULE)1);
}

/******************************************************************
 *		dosmem_handler
 *
 * Handler to catch access to our 1MB address space reserved for real memory
 */
static LONG WINAPI dosmem_handler(EXCEPTION_POINTERS* except)
{
    if (except->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        char *addr = (char *)except->ExceptionRecord->ExceptionInformation[1];
        if (addr >= DOSMEM_dosmem + DOSMEM_protect && addr < DOSMEM_dosmem + DOSMEM_SIZE)
        {
            if (load_winedos()) return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values.
 */
BOOL DOSMEM_Init(void)
{
    char *sysmem;
    void *addr = (void *)1;
    SIZE_T size = DOSMEM_SIZE - 1;

    if (NtAllocateVirtualMemory( GetCurrentProcess(), &addr, 0, &size,
                                 MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS ))
    {
        ERR( "Cannot allocate DOS memory\n" );
        ExitProcess(1);
    }

    if (addr <= (void *)DOSMEM_64KB)
    {
        DOSMEM_dosmem = 0;
        DOSMEM_protect = DOSMEM_64KB;
        sysmem = (char *)0xf0000;  /* store sysmem in high addresses for now */
    }
    else
    {
        WARN( "First megabyte not available for DOS address space.\n" );
        DOSMEM_dosmem = addr;
        DOSMEM_protect = 0;
        sysmem = DOSMEM_dosmem;
    }

    RtlAddVectoredExceptionHandler(FALSE, dosmem_handler);
    DOSMEM_0000H = GLOBAL_CreateBlock( GMEM_FIXED, sysmem,
                                       DOSMEM_64KB, 0, WINE_LDT_FLAGS_DATA );
    DOSMEM_BiosDataSeg = GLOBAL_CreateBlock( GMEM_FIXED, sysmem + 0x400,
                                             0x100, 0, WINE_LDT_FLAGS_DATA );
    DOSMEM_BiosSysSeg = GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem + 0xf0000,
                                            DOSMEM_64KB, 0, WINE_LDT_FLAGS_DATA );

    return TRUE;
}

/***********************************************************************
 *           DOSMEM_MapLinearToDos
 *
 * Linear address to the DOS address space.
 */
UINT DOSMEM_MapLinearToDos(LPVOID ptr)
{
    if (((char*)ptr >= DOSMEM_dosmem) &&
        ((char*)ptr < DOSMEM_dosmem + DOSMEM_SIZE))
          return (char *)ptr - DOSMEM_dosmem;
    return (UINT)ptr;
}


/***********************************************************************
 *           DOSMEM_MapDosToLinear
 *
 * DOS linear address to the linear address space.
 */
LPVOID DOSMEM_MapDosToLinear(UINT ptr)
{
    if (ptr < DOSMEM_SIZE) return DOSMEM_dosmem + ptr;
    return (LPVOID)ptr;
}


/***********************************************************************
 *           DOSMEM_MapRealToLinear
 *
 * Real mode DOS address into a linear pointer
 */
LPVOID DOSMEM_MapRealToLinear(DWORD x)
{
   LPVOID       lin;

   lin = DOSMEM_dosmem + HIWORD(x) * 16 + LOWORD(x);
   TRACE_(selector)("(0x%08x) returns %p.\n", x, lin );
   return lin;
}
