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
#include "winreg.h"
#include "excpt.h"
#include "winternl.h"
#include "wine/winbase16.h"

#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosmem);

/* DOS memory highest address (including HMA) */
#define DOSMEM_SIZE             0x110000
#define DOSMEM_64KB             0x10000

/* see dlls/kernel/dosmem.c for the details */
static char *DOSMEM_dosmem;
static char *DOSMEM_sysmem;

/*
 * Memory Control Block (MCB) definition
 * FIXME: implement Allocation Strategy
 */

#define MCB_DUMP(mc) \
    TRACE ("MCB_DUMP base=%p type=%02xh psp=%04xh size=%04xh\n", mc, mc->type, mc->psp , mc->size )
 
#define MCB_NEXT(mc) \
    (MCB*) ((mc->type==MCB_TYPE_LAST) ? NULL : (char*)(mc) + ((mc->size + 1) << 4) )

/* FIXME: should we check more? */
#define MCB_VALID(mc) \
    ((mc->type==MCB_TYPE_NORMAL) || (mc->type==MCB_TYPE_LAST))


#define MCB_TYPE_NORMAL    0x4d
#define MCB_TYPE_LAST      0x5a

#define MCB_PSP_DOS        0x0060  
#define MCB_PSP_FREE       0

#include "pshpack1.h"
typedef struct {
    BYTE type;
    WORD psp;     /* segment of owner psp */
    WORD size;    /* in paragraphs */
    BYTE pad[3];
    BYTE name[8];
} MCB;
#include "poppack.h"

/*
#define __DOSMEM_DEBUG__
 */

#define VM_STUB(x) (0x90CF00CD|(x<<8)) /* INT x; IRET; NOP */
#define VM_STUB_SEGMENT 0xf000         /* BIOS segment */

/* FIXME: this should be moved to the LOL */
static MCB* DOSMEM_root_block;

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

BIOSDATA* DOSVM_BiosData(void)
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
    BYTE *pBiosSys = (BYTE*)DOSMEM_dosmem + 0xf0000;
    BYTE *pBiosROMTable = pBiosSys+0xe6f5;
    BIOSDATA *pBiosData = DOSVM_BiosData();
    static const char bios_date[] = "13/01/99";

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
    memcpy(pBiosSys+0xfff5, bios_date, sizeof bios_date);

    /* BIOS ID */
    *(pBiosSys+0xfffe) = 0xfc;

    /* Reboot vector (f000:fff0 or ffff:0000) */
    *(DWORD*)(pBiosSys + 0xfff0) = VM_STUB(0x19);
}

/***********************************************************************
 *           BiosTick
 *
 * Increment the BIOS tick counter. Called by timer signal handler.
 */
void BiosTick( WORD timer )
{
    BIOSDATA *pBiosData = DOSVM_BiosData();
    if (pBiosData) pBiosData->Ticks++;
}

/***********************************************************************
 *           DOSMEM_Collapse
 *
 * Helper function for internal use only.
 * Attach all following free blocks to this one, even if this one is not free.
 */
static void DOSMEM_Collapse( MCB* mcb )
{
    MCB* next = MCB_NEXT( mcb );

    while (next && next->psp == MCB_PSP_FREE)
    {
        mcb->size = mcb->size + next->size + 1;
        mcb->type = next->type;    /* make sure keeping MCB_TYPE_LAST */
        next = MCB_NEXT( next );
    }
}

/***********************************************************************
 *           DOSMEM_AllocBlock
 *
 * Carve a chunk of the DOS memory block (without selector).
 */
LPVOID DOSMEM_AllocBlock(UINT size, UINT16* pseg)
{
    MCB *curr = DOSMEM_root_block;
    MCB *next = NULL;
    WORD psp = DOSVM_psp;
    
    if (!psp)
        psp = MCB_PSP_DOS;
    
    *pseg = 0;
    
    TRACE( "(%04xh)\n", size );
    
    /* round up to paragraph */
    size = (size + 15) >> 4;

#ifdef __DOSMEM_DEBUG__
    DOSMEM_Available();     /* checks the whole MCB list */
#endif
    
    /* loop over all MCB and search the next large enough MCB */
    while (curr)
    {
        if (!MCB_VALID (curr))
        {
            ERR( "MCB List Corrupt\n" );
            MCB_DUMP( curr );
            return NULL;
        }
        if (curr->psp == MCB_PSP_FREE)
        {
            DOSMEM_Collapse( curr );            
            /* is it large enough (one paragraph for the MCB)? */
            if (curr->size >= size)
            {
                if (curr->size > size)
                {
                    /* split curr */
                    next = (MCB *) ((char*) curr + ((size+1) << 4));
                    next->psp = MCB_PSP_FREE;
                    next->size = curr->size - (size+1);
                    next->type = curr->type;
                    curr->type = MCB_TYPE_NORMAL;
                    curr->size = size;
                }
                /* curr is the found block */
                curr->psp = psp;
                if( pseg ) *pseg = (((char*)curr) + 16 - DOSMEM_dosmem) >> 4;
                return (LPVOID) ((char*)curr + 16);
            }
        }
        curr = MCB_NEXT(curr);
    }
    return NULL;
}

/***********************************************************************
 *           DOSMEM_FreeBlock
 */
BOOL DOSMEM_FreeBlock(void* ptr)
{
    MCB* mcb = (MCB*) ((char*)ptr - 16);
    
    TRACE( "(%p)\n", ptr );

#ifdef __DOSMEM_DEBUG__
    DOSMEM_Available();
#endif
    
    if (!MCB_VALID (mcb))
    {
        ERR( "MCB invalid\n" );
        MCB_DUMP( mcb );
        return FALSE;
    }
    
    mcb->psp = MCB_PSP_FREE;
    DOSMEM_Collapse( mcb );
    return TRUE;
}

/***********************************************************************
 *           DOSMEM_ResizeBlock
 *
 * Resize DOS memory block in place. Returns block size or -1 on error.
 *
 * If exact is TRUE, returned value is either old or requested block
 * size. If exact is FALSE, block is expanded even if there is not
 * enough space for full requested block size.
 *
 * TODO: return also biggest block size
 */
UINT DOSMEM_ResizeBlock(void *ptr, UINT size, BOOL exact)
{
    MCB* mcb = (MCB*) ((char*)ptr - 16);
    MCB* next;
    
    TRACE( "(%p,%04xh,%s)\n", ptr, size, exact ? "TRUE" : "FALSE" );
    
    /* round up to paragraph */
    size = (size + 15) >> 4;

#ifdef __DOSMEM_DEBUG__
    DOSMEM_Available();
#endif
    
    if (!MCB_VALID (mcb))
    {
        ERR( "MCB invalid\n" );
        MCB_DUMP( mcb );
        return -1;
    }

    /* resize needed? */
    if (mcb->size == size)
        return size;

    /* collapse free blocks */
    DOSMEM_Collapse( mcb );    
    
    /* shrink mcb ? */
    if (mcb->size > size)
    {
        next = (MCB *) ((char*)mcb + ((size+1) << 4));
        next->type = mcb->type;
        next->psp = MCB_PSP_FREE;
        next->size = mcb->size - (size+1);
        mcb->type = MCB_TYPE_NORMAL;
        mcb->size = size;
        return size << 4;
    }
    
    if (!exact)
    {
        return mcb->size << 4;
    }
    
    return -1;
}

/***********************************************************************
 *           DOSMEM_Available
 */
UINT DOSMEM_Available(void)
{
    UINT  available = 0;
    UINT  total = 0;
    MCB *curr = DOSMEM_root_block;
    /* loop over all MCB and search the largest free MCB */
    while (curr)
    {
#ifdef __DOSMEM_DEBUG__
        MCB_DUMP( curr );
#endif
        if (!MCB_VALID (curr))
        {
            ERR( "MCB List Corrupt\n" );
            MCB_DUMP( curr );
            return 0;
        }
        if (curr->psp == MCB_PSP_FREE &&
            curr->size > available )
            available = curr->size;
        
        total += curr->size + 1;
        curr = MCB_NEXT( curr );
    }
    TRACE( " %04xh of %04xh paragraphs available\n", available, total );
    return available << 4;   
}

/***********************************************************************
 *           DOSMEM_InitMemory
 *
 * Initialises the DOS memory structures.
 */
static void DOSMEM_InitMemory(char* addr)
{
    DOSMEM_FillBiosSegments();
    DOSMEM_FillIsrTable();

    /* align root block to paragraph */
    DOSMEM_root_block = (MCB*) (( (DWORD_PTR)(addr+0xf) >> 4) << 4);
    DOSMEM_root_block->type = MCB_TYPE_LAST;
    DOSMEM_root_block->psp = MCB_PSP_FREE;
    DOSMEM_root_block->size = (DOSMEM_MemoryTop() - ((char*)DOSMEM_root_block)) >> 4;

    TRACE("DOS conventional memory initialized, %d bytes free.\n",
          DOSMEM_Available());
}

/******************************************************************
 *             DOSMEM_InitDosMemory
 *
 * When WineDOS is loaded, initializes the current DOS memory layout.
 */
BOOL DOSMEM_InitDosMemory(void)
{
    HMODULE16           hModule;
    unsigned short      sel;
    LDT_ENTRY           entry;
    DWORD               reserve;

    if (!(hModule = GetModuleHandle16("KERNEL"))) return FALSE;
    /* KERNEL.194: __F000H */
    sel = LOWORD(GetProcAddress16(hModule, (LPCSTR)(ULONG_PTR)194));
    wine_ldt_get_entry(sel, &entry);
    DOSMEM_dosmem = (char*)wine_ldt_get_base(&entry) - 0xF0000;
    /* KERNEL.183: __0000H */
    sel = LOWORD(GetProcAddress16(hModule, (LPCSTR)(DWORD_PTR)183));
    wine_ldt_get_entry(sel, &entry);
    DOSMEM_sysmem = wine_ldt_get_base(&entry);

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
    DOSMEM_InitMemory(DOSMEM_dosmem + reserve);
    return TRUE;
}

/******************************************************************
 *		DOSMEM_MapDosLayout
 *
 * Initialize the first MB of memory to look like a real DOS setup
 */
BOOL DOSMEM_MapDosLayout(void)
{
    static int already_mapped;

    if (!already_mapped)
    {
        HMODULE16       hModule;
        unsigned short  sel;
        LDT_ENTRY       entry;

        if (DOSMEM_dosmem)
        {
            ERR( "Needs access to the first megabyte for DOS mode\n" );
            ExitProcess(1);
        }
        MESSAGE( "Warning: unprotecting memory to allow real-mode calls.\n"
                 "         NULL pointer accesses will no longer be caught.\n" );
        VirtualProtect( NULL, DOSMEM_SIZE, PAGE_EXECUTE_READWRITE, NULL );
        /* copy the BIOS and ISR area down */
        memcpy( DOSMEM_dosmem, DOSMEM_sysmem, 0x400 + 0x100 );
        DOSMEM_sysmem = DOSMEM_dosmem;
        hModule = GetModuleHandle16("KERNEL");
        /* selector to 0000H */
        sel = LOWORD(GetProcAddress16(hModule, (LPCSTR)(DWORD_PTR)183));
        wine_ldt_get_entry(sel, &entry);
        wine_ldt_set_base(&entry, NULL);
        wine_ldt_set_entry(sel, &entry);
        /* selector to BiosData */
        sel = LOWORD(GetProcAddress16(hModule, (LPCSTR)(DWORD_PTR)193));
        wine_ldt_get_entry(sel, &entry);
        wine_ldt_set_base(&entry, (const void*)0x400);
        wine_ldt_set_entry(sel, &entry);
        /* we may now need the actual interrupt stubs, and since we've just moved the
         * interrupt vector table away, we can fill the area with stubs instead... */
        DOSMEM_MakeIsrStubs();
        already_mapped = 1;
    }
    return TRUE;
}
