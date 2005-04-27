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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "winreg.h"
#include "excpt.h"
#include "winternl.h"
#include "wine/winbase16.h"

#include "kernel_private.h"
#include "toolhelp.h"
#include "miscemu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosmem);
WINE_DECLARE_DEBUG_CHANNEL(selector);

WORD DOSMEM_0000H;        /* segment at 0:0 */
WORD DOSMEM_BiosDataSeg;  /* BIOS data segment at 0x40:0 */
WORD DOSMEM_BiosSysSeg;   /* BIOS ROM segment at 0xf000:0 */

/* DOS memory highest address (including HMA) */
#define DOSMEM_SIZE             0x110000
#define DOSMEM_64KB             0x10000

/* use 2 low bits of 'size' for the housekeeping */

#define DM_BLOCK_DEBUG		0xABE00000
#define DM_BLOCK_TERMINAL	0x00000001
#define DM_BLOCK_FREE		0x00000002
#define DM_BLOCK_MASK		0x001FFFFC

/*
#define __DOSMEM_DEBUG__
 */

typedef struct {
   unsigned	size;
} dosmem_entry;

typedef struct {
  unsigned      blocks;
  unsigned      free;
} dosmem_info;

#define NEXT_BLOCK(block) \
        (dosmem_entry*)(((char*)(block)) + \
	 sizeof(dosmem_entry) + ((block)->size & DM_BLOCK_MASK))

#define VM_STUB(x) (0x90CF00CD|(x<<8)) /* INT x; IRET; NOP */
#define VM_STUB_SEGMENT 0xf000         /* BIOS segment */

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
 *    pointer deref catching for example). In this case, we'res still in PM
 *    mode, accessing part of the "physicale" real mode memory.
 * 3/ if the process enters the real mode, then we commit the full first MB of
 *    memory (and also initialize the DOS structures in it).
 */

/* DOS memory base (linear in process address space) */
static char *DOSMEM_dosmem;
/* DOS system base (for interrupt vector table and BIOS data area)
 * ...should in theory (i.e. Windows) be equal to DOSMEM_dosmem (NULL),
 * but is normally set to 0xf0000 in Wine to allow trapping of NULL pointers,
 * and only relocated to NULL when absolutely necessary */
static char *DOSMEM_sysmem;
/* number of bytes protected from _dosmem. 0 when DOS memory is initialized, 
 * 64k otherwise to trap NULL pointers deref */
static DWORD DOSMEM_protect;

static void DOSMEM_InitMemory(void);

/***********************************************************************
 *           DOSMEM_MemoryTop
 *
 * Gets the DOS memory top.
 */
static char *DOSMEM_MemoryTop(void)
{
    return DOSMEM_dosmem+0x9FFFC; /* 640K */
}

/***********************************************************************
 *           DOSMEM_InfoBlock
 *
 * Gets the DOS memory info block.
 */
static dosmem_info *DOSMEM_InfoBlock(void)
{
    /* Start of DOS conventional memory */
    static char *DOSMEM_membase;

    if (!DOSMEM_membase)
    {
        DWORD         reserve;

        /*
         * Reserve either:
         * - lowest 64k for NULL pointer catching (Win16)
         * - lowest 1k for interrupt handlers and 
         *   another 0.5k for BIOS, DOS and intra-application
         *   areas (DOS)
         */
        if (DOSMEM_dosmem != DOSMEM_sysmem)
            reserve = DOSMEM_64KB;
        else
            reserve = 0x600; /* 1.5k */

        /*
         * Round to paragraph boundary in order to make 
         * sure the alignment is correct.
         */
        reserve = ((reserve + 15) >> 4) << 4;

        /*
         * Set DOS memory base and initialize conventional memory.
         */
        DOSMEM_membase = DOSMEM_dosmem + reserve;
        DOSMEM_InitMemory();
    }

    return (dosmem_info*)DOSMEM_membase;
}

/***********************************************************************
 *           DOSMEM_RootBlock
 *
 * Gets the DOS memory root block.
 */
static dosmem_entry *DOSMEM_RootBlock(void)
{
    /* first block has to be paragraph-aligned */
    return (dosmem_entry*)(((char*)DOSMEM_InfoBlock()) +
                           ((((sizeof(dosmem_info) + 0xf) & ~0xf) - sizeof(dosmem_entry))));
}

/***********************************************************************
 *           DOSMEM_FillIsrTable
 *
 * Fill the interrupt table with fake BIOS calls to BIOSSEG (0xf000).
 *
 * NOTES:
 * Linux normally only traps INTs performed from or destined to BIOSSEG
 * for us to handle, if the int_revectored table is empty. Filling the
 * interrupt table with calls to INT stubs in BIOSSEG allows DOS programs
 * to hook interrupts, as well as use their familiar retf tricks to call
 * them, AND let Wine handle any unhooked interrupts transparently.
 */
static void DOSMEM_FillIsrTable(void)
{
    SEGPTR *isr = (SEGPTR*)DOSMEM_sysmem;
    int x;

    for (x=0; x<256; x++) isr[x]=MAKESEGPTR(VM_STUB_SEGMENT,x*4);
}

static void DOSMEM_MakeIsrStubs(void)
{
    DWORD *stub = (DWORD*)(DOSMEM_dosmem + (VM_STUB_SEGMENT << 4));
    int x;

    for (x=0; x<256; x++) stub[x]=VM_STUB(x);
}

static BIOSDATA * DOSMEM_BiosData(void)
{
    return (BIOSDATA *)(DOSMEM_sysmem + 0x400);
}

/**********************************************************************
 *          DOSMEM_GetTicksSinceMidnight
 *
 * Return number of clock ticks since midnight.
 */
static DWORD DOSMEM_GetTicksSinceMidnight(void)
{
    SYSTEMTIME time;

    /* This should give us the (approximately) correct
     * 18.206 clock ticks per second since midnight.
     */

    GetLocalTime( &time );

    return (((time.wHour * 3600 + time.wMinute * 60 +
              time.wSecond) * 18206) / 1000) +
             (time.wMilliseconds * 1000 / 54927);
}

/***********************************************************************
 *           DOSMEM_FillBiosSegments
 *
 * Fill the BIOS data segment with dummy values.
 */
static void DOSMEM_FillBiosSegments(void)
{
    char *pBiosSys = DOSMEM_dosmem + 0xf0000;
    BYTE *pBiosROMTable = pBiosSys+0xe6f5;
    BIOSDATA *pBiosData = DOSMEM_BiosData();

      /* Clear all unused values */
    memset( pBiosData, 0, sizeof(*pBiosData) );

    /* FIXME: should check the number of configured drives and ports */
    pBiosData->Com1Addr             = 0x3f8;
    pBiosData->Com2Addr             = 0x2f8;
    pBiosData->Lpt1Addr             = 0x378;
    pBiosData->Lpt2Addr             = 0x278;
    pBiosData->InstalledHardware    = 0x5463;
    pBiosData->MemSize              = 640;
    pBiosData->NextKbdCharPtr       = 0x1e;
    pBiosData->FirstKbdCharPtr      = 0x1e;
    pBiosData->VideoMode            = 3;
    pBiosData->VideoColumns         = 80;
    pBiosData->VideoPageSize        = 80 * 25 * 2;
    pBiosData->VideoPageStartAddr   = 0xb800;
    pBiosData->VideoCtrlAddr        = 0x3d4;
    pBiosData->Ticks                = DOSMEM_GetTicksSinceMidnight();
    pBiosData->NbHardDisks          = 2;
    pBiosData->KbdBufferStart       = 0x1e;
    pBiosData->KbdBufferEnd         = 0x3e;
    pBiosData->RowsOnScreenMinus1   = 24;
    pBiosData->BytesPerChar         = 0x10;
    pBiosData->ModeOptions          = 0x64;
    pBiosData->FeatureBitsSwitches  = 0xf9;
    pBiosData->VGASettings          = 0x51;
    pBiosData->DisplayCombination   = 0x08;
    pBiosData->DiskDataRate         = 0;

    /* fill ROM configuration table (values from Award) */
    *(pBiosROMTable+0x0)	= 0x08; /* number of bytes following LO */
    *(pBiosROMTable+0x1)	= 0x00; /* number of bytes following HI */
    *(pBiosROMTable+0x2)	= 0xfc; /* model */
    *(pBiosROMTable+0x3)	= 0x01; /* submodel */
    *(pBiosROMTable+0x4)	= 0x00; /* BIOS revision */
    *(pBiosROMTable+0x5)	= 0x74; /* feature byte 1 */
    *(pBiosROMTable+0x6)	= 0x00; /* feature byte 2 */
    *(pBiosROMTable+0x7)	= 0x00; /* feature byte 3 */
    *(pBiosROMTable+0x8)	= 0x00; /* feature byte 4 */
    *(pBiosROMTable+0x9)	= 0x00; /* feature byte 5 */

    /* BIOS date string */
    strcpy(pBiosSys+0xfff5, "13/01/99");

    /* BIOS ID */
    *(pBiosSys+0xfffe) = 0xfc;

    /* Reboot vector (f000:fff0 or ffff:0000) */
    *(DWORD*)(pBiosSys + 0xfff0) = VM_STUB(0x19);
}

/***********************************************************************
 *           DOSMEM_InitMemory
 *
 * Initialises the DOS memory structures.
 */
static void DOSMEM_InitMemory(void)
{
    dosmem_info*        info_block = DOSMEM_InfoBlock();
    dosmem_entry*       root_block = DOSMEM_RootBlock();
    dosmem_entry*       dm;

    root_block->size = DOSMEM_MemoryTop() - (((char*)root_block) + sizeof(dosmem_entry));

    info_block->blocks = 0;
    info_block->free = root_block->size;

    dm = NEXT_BLOCK(root_block);
    dm->size = DM_BLOCK_TERMINAL;
    root_block->size |= DM_BLOCK_FREE
#ifdef __DOSMEM_DEBUG__
		     | DM_BLOCK_DEBUG
#endif
		     ;

    TRACE( "DOS conventional memory initialized, %d bytes free.\n", 
           DOSMEM_Available() );
}

static void dosmem_bios_init(void)
{
    static      int bios_created;

    if (!bios_created)
    {
        DOSMEM_FillBiosSegments();
        DOSMEM_FillIsrTable();
        bios_created = TRUE;
    }
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
        DWORD   addr = except->ExceptionRecord->ExceptionInformation[1];
        if (addr >= (ULONG_PTR)DOSMEM_sysmem &&
            addr < (ULONG_PTR)DOSMEM_sysmem + DOSMEM_64KB)
        {
            VirtualProtect( DOSMEM_sysmem, DOSMEM_64KB, PAGE_EXECUTE_READWRITE, NULL );
            dosmem_bios_init();
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (addr >= (ULONG_PTR)DOSMEM_dosmem + DOSMEM_protect &&
            addr < (ULONG_PTR)DOSMEM_dosmem + DOSMEM_SIZE)
        {
            VirtualProtect( DOSMEM_dosmem + DOSMEM_protect, DOSMEM_SIZE - DOSMEM_protect,
                            PAGE_EXECUTE_READWRITE, NULL );
            dosmem_bios_init();
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

/**********************************************************************
 *		setup_dos_mem
 *
 * Setup the first megabyte for DOS memory access
 */
static void setup_dos_mem(void)
{
    int sys_offset = 0;
    int page_size = getpagesize();
    void *addr = NULL;

    if (wine_mmap_is_in_reserved_area( NULL, DOSMEM_SIZE ) != 1)
    {
        addr = wine_anon_mmap( (void *)page_size, DOSMEM_SIZE-page_size,
                               PROT_READ | PROT_WRITE | PROT_EXEC, 0 );
        if (addr == (void *)page_size) addr = NULL; /* we got what we wanted */
        else munmap( addr, DOSMEM_SIZE - page_size );
    }

    if (!addr)
    {
        /* now reserve from address 0 */
        wine_anon_mmap( NULL, DOSMEM_SIZE, 0, MAP_FIXED );

        /* inform the memory manager that there is a mapping here, but don't commit yet */
        VirtualAlloc( NULL, DOSMEM_SIZE, MEM_RESERVE | MEM_SYSTEM, PAGE_NOACCESS );
        sys_offset = 0xf0000;
        DOSMEM_protect = DOSMEM_64KB;
    }
    else
    {
        ERR("Cannot use first megabyte for DOS address space, please report\n" );
        /* allocate the DOS area somewhere else */
        addr = VirtualAlloc( NULL, DOSMEM_SIZE, MEM_RESERVE, PAGE_NOACCESS );
        if (!addr)
        {
            ERR( "Cannot allocate DOS memory\n" );
            ExitProcess(1);
        }
    }
    DOSMEM_dosmem = addr;
    DOSMEM_sysmem = (char*)addr + sys_offset;
    RtlAddVectoredExceptionHandler(FALSE, dosmem_handler);
}


/***********************************************************************
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values.
 */
BOOL DOSMEM_Init(void)
{
    setup_dos_mem();

    DOSMEM_0000H = GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_sysmem,
                                       DOSMEM_64KB, 0, WINE_LDT_FLAGS_DATA );
    DOSMEM_BiosDataSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_sysmem + 0x400,
                                            0x100, 0, WINE_LDT_FLAGS_DATA );
    DOSMEM_BiosSysSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem + 0xf0000,
                                           DOSMEM_64KB, 0, WINE_LDT_FLAGS_DATA );

    return TRUE;
}

/******************************************************************
 *		DOSMEM_InitDosMem
 *
 * Initialize the first MB of memory to look like a real DOS setup
 */
BOOL DOSMEM_InitDosMem(void)
{
    static int already_mapped;

    if (!already_mapped)
    {
        if (DOSMEM_dosmem)
        {
            ERR( "Needs access to the first megabyte for DOS mode\n" );
            ExitProcess(1);
        }
        MESSAGE( "Warning: unprotecting memory to allow real-mode calls.\n"
                 "         NULL pointer accesses will no longer be caught.\n" );
        VirtualProtect( NULL, DOSMEM_SIZE, PAGE_EXECUTE_READWRITE, NULL );
        DOSMEM_protect = 0;
        dosmem_bios_init();
        /* copy the BIOS and ISR area down */
        memcpy( DOSMEM_dosmem, DOSMEM_sysmem, 0x400 + 0x100 );
        DOSMEM_sysmem = DOSMEM_dosmem;
        SetSelectorBase( DOSMEM_0000H, 0 );
        SetSelectorBase( DOSMEM_BiosDataSeg, 0x400 );
        /* we may now need the actual interrupt stubs, and since we've just moved the
         * interrupt vector table away, we can fill the area with stubs instead... */
        DOSMEM_MakeIsrStubs();
        already_mapped = 1;
    }
    return TRUE;
}


/***********************************************************************
 *           DOSMEM_Tick
 *
 * Increment the BIOS tick counter. Called by timer signal handler.
 */
void DOSMEM_Tick( WORD timer )
{
    BIOSDATA *pBiosData = DOSMEM_BiosData();
    if (pBiosData) pBiosData->Ticks++;
}

/***********************************************************************
 *           DOSMEM_GetBlock
 *
 * Carve a chunk of the DOS memory block (without selector).
 */
LPVOID DOSMEM_GetBlock(UINT size, UINT16* pseg)
{
   UINT  	 blocksize;
   char         *block = NULL;
   dosmem_info  *info_block = DOSMEM_InfoBlock();
   dosmem_entry *dm;
#ifdef __DOSMEM_DEBUG_
   dosmem_entry *prev = NULL;
#endif

   if( size > info_block->free ) return NULL;
   dm = DOSMEM_RootBlock();

   while (dm && dm->size != DM_BLOCK_TERMINAL)
   {
#ifdef __DOSMEM_DEBUG__
       if( (dm->size & DM_BLOCK_DEBUG) != DM_BLOCK_DEBUG )
       {
	    WARN("MCB overrun! [prev = 0x%08x]\n", 4 + (UINT)prev);
	    return NULL;
       }
       prev = dm;
#endif
       if( dm->size & DM_BLOCK_FREE )
       {
	   dosmem_entry  *next = NEXT_BLOCK(dm);

	   while( next->size & DM_BLOCK_FREE ) /* collapse free blocks */
	   {
	       dm->size += sizeof(dosmem_entry) + (next->size & DM_BLOCK_MASK);
	       next->size = (DM_BLOCK_FREE | DM_BLOCK_TERMINAL);
	       next = NEXT_BLOCK(dm);
	   }

	   blocksize = dm->size & DM_BLOCK_MASK;
	   if( blocksize >= size )
           {
	       block = ((char*)dm) + sizeof(dosmem_entry);
	       if( blocksize - size > 0x20 )
	       {
		   /* split dm so that the next one stays
		    * paragraph-aligned (and dm loses free bit) */

	           dm->size = (((size + 0xf + sizeof(dosmem_entry)) & ~0xf) -
			         	      sizeof(dosmem_entry));
	           next = (dosmem_entry*)(((char*)dm) +
	 		   sizeof(dosmem_entry) + dm->size);
	           next->size = (blocksize - (dm->size +
			   sizeof(dosmem_entry))) | DM_BLOCK_FREE
#ifdef __DOSMEM_DEBUG__
					          | DM_BLOCK_DEBUG
#endif
						  ;
	       } else dm->size &= DM_BLOCK_MASK;

	       info_block->blocks++;
	       info_block->free -= dm->size;
	       if( pseg ) *pseg = (block - DOSMEM_dosmem) >> 4;
#ifdef __DOSMEM_DEBUG__
               dm->size |= DM_BLOCK_DEBUG;
#endif
	       break;
	   }
 	   dm = next;
       }
       else dm = NEXT_BLOCK(dm);
   }
   return (LPVOID)block;
}

/***********************************************************************
 *           DOSMEM_FreeBlock
 */
BOOL DOSMEM_FreeBlock(void* ptr)
{
   dosmem_info  *info_block = DOSMEM_InfoBlock();

   if( ptr >= (void*)(((char*)DOSMEM_RootBlock()) + sizeof(dosmem_entry)) &&
       ptr < (void*)DOSMEM_MemoryTop() && !((((char*)ptr)
                  - DOSMEM_dosmem) & 0xf) )
   {
       dosmem_entry  *dm = (dosmem_entry*)(((char*)ptr) - sizeof(dosmem_entry));

       if( !(dm->size & (DM_BLOCK_FREE | DM_BLOCK_TERMINAL))
#ifdef __DOSMEM_DEBUG__
	 && ((dm->size & DM_BLOCK_DEBUG) == DM_BLOCK_DEBUG )
#endif
	 )
       {
	     info_block->blocks--;
	     info_block->free += dm->size;

	     dm->size |= DM_BLOCK_FREE;
	     return TRUE;
       }
   }
   return FALSE;
}

/***********************************************************************
 *           DOSMEM_ResizeBlock
 *
 * Resize DOS memory block in place. Returns block size or -1 on error.
 *
 * If exact is TRUE, returned value is either old or requested block
 * size. If exact is FALSE, block is expanded even if there is not
 * enough space for full requested block size.
 */
UINT DOSMEM_ResizeBlock(void *ptr, UINT size, BOOL exact)
{
   char         *block = NULL;
   dosmem_info  *info_block = DOSMEM_InfoBlock();
   dosmem_entry *dm;
   dosmem_entry *next;
   UINT blocksize;
   UINT orgsize;

   if( (ptr < (void*)(sizeof(dosmem_entry) + (char*)DOSMEM_RootBlock())) ||
       (ptr >= (void*)DOSMEM_MemoryTop()) ||
       (((((char*)ptr) - DOSMEM_dosmem) & 0xf) != 0) )
     return (UINT)-1;

   dm = (dosmem_entry*)(((char*)ptr) - sizeof(dosmem_entry));
   if( dm->size & (DM_BLOCK_FREE | DM_BLOCK_TERMINAL) )
       return (UINT)-1;

   next = NEXT_BLOCK(dm);
   orgsize = dm->size & DM_BLOCK_MASK;

   /* collapse free blocks */
   while( next->size & DM_BLOCK_FREE )
   {
       dm->size += sizeof(dosmem_entry) + (next->size & DM_BLOCK_MASK);
       next->size = (DM_BLOCK_FREE | DM_BLOCK_TERMINAL);
       next = NEXT_BLOCK(dm);
   }

   blocksize = dm->size & DM_BLOCK_MASK;

   /*
    * If collapse didn't help we either expand block to maximum
    * available size (exact == FALSE) or give collapsed blocks
    * back to free storage (exact == TRUE).
    */
   if (blocksize < size)
       size = exact ? orgsize : blocksize;

   block = ((char*)dm) + sizeof(dosmem_entry);
   if( blocksize - size > 0x20 )
   {
       /*
        * split dm so that the next one stays
        * paragraph-aligned (and next gains free bit) 
        */

       dm->size = (((size + 0xf + sizeof(dosmem_entry)) & ~0xf) -
                   sizeof(dosmem_entry));
       next = (dosmem_entry*)(((char*)dm) +
                              sizeof(dosmem_entry) + dm->size);
       next->size = (blocksize - (dm->size +
                                  sizeof(dosmem_entry))) | DM_BLOCK_FREE;
   } 
   else 
   {
       dm->size &= DM_BLOCK_MASK;
   }

   /*
    * Adjust available memory if block size changes.
    */
   info_block->free += orgsize - dm->size;

   return size;
}

/***********************************************************************
 *           DOSMEM_Available
 */
UINT DOSMEM_Available(void)
{
   UINT  	 blocksize, available = 0;
   dosmem_entry *dm;

   dm = DOSMEM_RootBlock();

   while (dm && dm->size != DM_BLOCK_TERMINAL)
   {
#ifdef __DOSMEM_DEBUG__
       if( (dm->size & DM_BLOCK_DEBUG) != DM_BLOCK_DEBUG )
       {
	    WARN("MCB overrun! [prev = 0x%08x]\n", 4 + (UINT)prev);
	    return NULL;
       }
       prev = dm;
#endif
       if( dm->size & DM_BLOCK_FREE )
       {
	   dosmem_entry  *next = NEXT_BLOCK(dm);

	   while( next->size & DM_BLOCK_FREE ) /* collapse free blocks */
	   {
	       dm->size += sizeof(dosmem_entry) + (next->size & DM_BLOCK_MASK);
	       next->size = (DM_BLOCK_FREE | DM_BLOCK_TERMINAL);
	       next = NEXT_BLOCK(dm);
	   }

	   blocksize = dm->size & DM_BLOCK_MASK;
	   if ( blocksize > available ) available = blocksize;
 	   dm = next;
       }
       else dm = NEXT_BLOCK(dm);
   }
   return available;
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
	  return (UINT)ptr - (UINT)DOSMEM_dosmem;
    return (UINT)ptr;
}


/***********************************************************************
 *           DOSMEM_MapDosToLinear
 *
 * DOS linear address to the linear address space.
 */
LPVOID DOSMEM_MapDosToLinear(UINT ptr)
{
    if (ptr < DOSMEM_SIZE) return (LPVOID)(ptr + (UINT)DOSMEM_dosmem);
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
   TRACE_(selector)("(0x%08lx) returns %p.\n", x, lin );
   return lin;
}

/***********************************************************************
 *           DOSMEM_AllocSelector
 *
 * Allocates a protected mode selector for a realmode segment.
 */
WORD DOSMEM_AllocSelector(WORD realsel)
{
	HMODULE16 hModule = GetModuleHandle16("KERNEL");
	WORD	sel;

	sel=GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+realsel*16, DOSMEM_64KB,
                                hModule, WINE_LDT_FLAGS_DATA );
	TRACE_(selector)("(0x%04x) returns 0x%04x.\n", realsel,sel);
	return sel;
}
