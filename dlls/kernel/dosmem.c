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

#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include "windef.h"
#include "winbase.h"
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

/* DOS memory base */
static char *DOSMEM_dosmem;
/* DOS system base (for interrupt vector table and BIOS data area)
 * ...should in theory (i.e. Windows) be equal to DOSMEM_dosmem (NULL),
 * but is normally set to 0xf0000 in Wine to allow trapping of NULL pointers,
 * and only relocated to NULL when absolutely necessary */
static char *DOSMEM_sysmem;

/* Start of DOS conventional memory */
static char *DOSMEM_membase;

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
            reserve = 0x10000; /* 64k */
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
    struct tm *bdtime;
    struct timeval tvs;
    time_t seconds;

    /* This should give us the (approximately) correct
     * 18.206 clock ticks per second since midnight.
     */
    gettimeofday( &tvs, NULL );
    seconds = tvs.tv_sec;
    bdtime = localtime( &seconds );
    return (((bdtime->tm_hour * 3600 + bdtime->tm_min * 60 +
              bdtime->tm_sec) * 18206) / 1000) +
                  (tvs.tv_usec / 54927);
}

/***********************************************************************
 *           DOSMEM_FillBiosSegments
 *
 * Fill the BIOS data segment with dummy values.
 */
static void DOSMEM_FillBiosSegments(void)
{
    BYTE *pBiosSys = DOSMEM_dosmem + 0xf0000;
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
    strcpy((char *)pBiosSys+0xfff5, "13/01/99");

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


/**********************************************************************
 *		setup_dos_mem
 *
 * Setup the first megabyte for DOS memory access
 */
static void setup_dos_mem( int dos_init )
{
    int sys_offset = 0;
    int page_size = getpagesize();
    void *addr = wine_anon_mmap( (void *)page_size, 0x110000-page_size,
                                 PROT_READ | PROT_WRITE | PROT_EXEC, 0 );
    if (addr == (void *)page_size)  /* we got what we wanted */
    {
        /* now map from address 0 */
        addr = wine_anon_mmap( NULL, 0x110000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED );
        if (addr)
        {
            ERR("MAP_FIXED failed at address 0 for DOS address space\n" );
            ExitProcess(1);
        }

        /* inform the memory manager that there is a mapping here */
        VirtualAlloc( addr, 0x110000, MEM_RESERVE | MEM_SYSTEM, PAGE_EXECUTE_READWRITE );

        /* protect the first 64K to catch NULL pointers */
        if (!dos_init)
        {
            VirtualProtect( addr, 0x10000, PAGE_NOACCESS, NULL );
            /* move the BIOS and ISR area from 0x00000 to 0xf0000 */
            sys_offset += 0xf0000;
        }
    }
    else
    {
        ERR("Cannot use first megabyte for DOS address space, please report\n" );
        if (dos_init) ExitProcess(1);
        /* allocate the DOS area somewhere else */
        addr = VirtualAlloc( NULL, 0x110000, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
        if (!addr)
        {
            ERR( "Cannot allocate DOS memory\n" );
            ExitProcess(1);
        }
    }
    DOSMEM_dosmem = addr;
    DOSMEM_sysmem = (char*)addr + sys_offset;
}


/***********************************************************************
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values.
 */
BOOL DOSMEM_Init(BOOL dos_init)
{
    static int already_done, already_mapped;

    if (!already_done)
    {
        setup_dos_mem( dos_init );

        DOSMEM_0000H = GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_sysmem,
                                           0x10000, 0, WINE_LDT_FLAGS_DATA );
        DOSMEM_BiosDataSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_sysmem + 0x400,
                                                0x100, 0, WINE_LDT_FLAGS_DATA );
        DOSMEM_BiosSysSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0xf0000,
                                               0x10000, 0, WINE_LDT_FLAGS_DATA );
        DOSMEM_FillBiosSegments();
        DOSMEM_FillIsrTable();
        already_done = 1;
    }
    else if (dos_init && !already_mapped)
    {
        if (DOSMEM_dosmem)
        {
            ERR( "Needs access to the first megabyte for DOS mode\n" );
            ExitProcess(1);
        }
        MESSAGE( "Warning: unprotecting the first 64KB of memory to allow real-mode calls.\n"
                 "         NULL pointer accesses will no longer be caught.\n" );
        VirtualProtect( NULL, 0x10000, PAGE_EXECUTE_READWRITE, NULL );
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
        ((char*)ptr < DOSMEM_dosmem + 0x100000))
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
    if (ptr < 0x100000) return (LPVOID)(ptr + (UINT)DOSMEM_dosmem);
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

   lin=DOSMEM_dosmem+(x&0xffff)+(((x&0xffff0000)>>16)*16);
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

	sel=GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+realsel*16, 0x10000,
                                hModule, WINE_LDT_FLAGS_DATA );
	TRACE_(selector)("(0x%04x) returns 0x%04x.\n", realsel,sel);
	return sel;
}
