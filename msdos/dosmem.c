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

#include "winbase.h"
#include "wine/winbase16.h"

#include "global.h"
#include "selectors.h"
#include "miscemu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosmem);
WINE_DECLARE_DEBUG_CHANNEL(selector);

WORD DOSMEM_0000H;        /* segment at 0:0 */
WORD DOSMEM_BiosDataSeg;  /* BIOS data segment at 0x40:0 */
WORD DOSMEM_BiosSysSeg;   /* BIOS ROM segment at 0xf000:0 */

DWORD DOSMEM_CollateTable;

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

/* various real-mode code stubs */
struct DPMI_segments DOSMEM_dpmi_segments;

/***********************************************************************
 *           DOSMEM_GetDPMISegments
 */
const struct DPMI_segments *DOSMEM_GetDPMISegments(void)
{
    return &DOSMEM_dpmi_segments;
}

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
    return (dosmem_info*)(DOSMEM_dosmem+0x10000); /* 64K */
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

/***********************************************************************
 *           DOSMEM_InitDPMI
 *
 * Allocate the global DPMI RMCB wrapper.
 */
static void DOSMEM_InitDPMI(void)
{
    LPSTR ptr;
    int   i;

    static const char wrap_code[]={
     0xCD,0x31, /* int $0x31 */
     0xCB       /* lret */
    };

    static const char enter_xms[]=
    {
        /* XMS hookable entry point */
        0xEB,0x03,           /* jmp entry */
        0x90,0x90,0x90,      /* nop;nop;nop */
                             /* entry: */
        /* real entry point */
        /* for simplicity, we'll just use the same hook as DPMI below */
        0xCD,0x31,           /* int $0x31 */
        0xCB                 /* lret */
    };

    static const char enter_pm[]=
    {
        0x50,                /* pushw %ax */
        0x52,                /* pushw %dx */
        0x55,                /* pushw %bp */
        0x89,0xE5,           /* movw %sp,%bp */
        /* get return CS */
        0x8B,0x56,0x08,      /* movw 8(%bp),%dx */
        /* just call int 31 here to get into protected mode... */
        /* it'll check whether it was called from dpmi_seg... */
        0xCD,0x31,           /* int $0x31 */
        /* we are now in the context of a 16-bit relay call */
        /* need to fixup our stack;
         * 16-bit relay return address will be lost, but we won't worry quite yet */
        0x8E,0xD0,           /* movw %ax,%ss */
        0x66,0x0F,0xB7,0xE5, /* movzwl %bp,%esp */
        /* set return CS */
        0x89,0x56,0x08,      /* movw %dx,8(%bp) */
        0x5D,                /* popw %bp */
        0x5A,                /* popw %dx */
        0x58,                /* popw %ax */
        0xCB                 /* lret */
    };

    ptr = DOSMEM_GetBlock( sizeof(wrap_code), &DOSMEM_dpmi_segments.wrap_seg );
    memcpy( ptr, wrap_code, sizeof(wrap_code) );
    ptr = DOSMEM_GetBlock( sizeof(enter_xms), &DOSMEM_dpmi_segments.xms_seg );
    memcpy( ptr, enter_xms, sizeof(enter_xms) );
    ptr = DOSMEM_GetBlock( sizeof(enter_pm), &DOSMEM_dpmi_segments.dpmi_seg );
    memcpy( ptr, enter_pm, sizeof(enter_pm) );
    DOSMEM_dpmi_segments.dpmi_sel = SELECTOR_AllocBlock( ptr, sizeof(enter_pm), WINE_LDT_FLAGS_CODE );

    ptr = DOSMEM_GetBlock( 6 * 256, &DOSMEM_dpmi_segments.int48_seg );
    for(i=0; i<256; i++) {
        /*
         * Each 32-bit interrupt handler is 6 bytes:
         * 0xCD,<i>            = int <i> (nested 16-bit interrupt)
         * 0x66,0xCA,0x04,0x00 = ret 4   (32-bit far return and pop 4 bytes)
         */
        ptr[i * 6 + 0] = 0xCD;
        ptr[i * 6 + 1] = i;
        ptr[i * 6 + 2] = 0x66;
        ptr[i * 6 + 3] = 0xCA;
        ptr[i * 6 + 4] = 0x04;
        ptr[i * 6 + 5] = 0x00;
    }
    DOSMEM_dpmi_segments.int48_sel = SELECTOR_AllocBlock( ptr, 6 * 256, WINE_LDT_FLAGS_CODE );
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
    BIOS_EXTRA *extra = (BIOS_EXTRA *)(DOSMEM_dosmem + (int)BIOS_EXTRA_PTR);
    BIOSDATA *pBiosData = DOSMEM_BiosData();

    /* Supported VESA mode, see int10.c */
    WORD ConstVesaModeList[]={0x00,0x01,0x02,0x03,0x07,0x0D,0x0E,0x10,0x12,0x13,
                              0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x10D,0x10E,
                              0x10F,0x110,0x111,0x112,0x113,0x114,0x115,0x116,0x117,0x118,
                              0x119,0x11A,0x11B,0xFFFF};
    char * ConstVesaString = "WINE SVGA BOARD";
    int i;

    VIDEOFUNCTIONALITY *pVidFunc = &extra->vid_func;
    VIDEOSTATE *pVidState = &extra->vid_state;
    VESAINFO *pVesaInfo = &extra->vesa_info;
    char * VesaString = extra->vesa_string;
    WORD * VesaModeList = extra->vesa_modes;

      /* Clear all unused values */
    memset( pBiosData, 0, sizeof(*pBiosData) );
    memset( pVidFunc,  0, sizeof(*pVidFunc ) );
    memset( pVidState, 0, sizeof(*pVidState) );
    memset( pVesaInfo, 0, sizeof(*pVesaInfo) );

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


    for (i = 0; i < 7; i++)
        pVidFunc->ModeSupport[i] = 0xff;

    pVidFunc->ScanlineSupport     = 7;
    pVidFunc->NumberCharBlocks    = 0;
    pVidFunc->ActiveCharBlocks    = 0;
    pVidFunc->MiscFlags           = 0x8ff;
    pVidFunc->SavePointerFlags    = 0x3f;

                                    /* FIXME: always real mode ? */
    pVidState->StaticFuncTable    = BIOS_EXTRA_SEGPTR + offsetof(BIOS_EXTRA,vid_func);
    pVidState->VideoMode          = pBiosData->VideoMode; /* needs updates! */
    pVidState->NumberColumns      = pBiosData->VideoColumns; /* needs updates! */
    pVidState->RegenBufLen        = 0;
    pVidState->RegenBufAddr       = 0;

    for (i = 0; i < 8; i++)
        pVidState->CursorPos[i] = 0;

    pVidState->CursorType         = 0x0a0b;  /* start/end line */
    pVidState->ActivePage         = 0;
    pVidState->CRTCPort           = 0x3da;
    pVidState->Port3x8            = 0;
    pVidState->Port3x9            = 0;
    pVidState->NumberRows         = 23;     /* number of rows - 1 */
    pVidState->BytesPerChar       = 0x10;
    pVidState->DCCActive          = pBiosData->DisplayCombination;
    pVidState->DCCAlternate       = 0;
    pVidState->NumberColors       = 16;
    pVidState->NumberPages        = 1;
    pVidState->NumberScanlines    = 3; /* (0,1,2,3) = (200,350,400,480) */
    pVidState->CharBlockPrimary   = 0;
    pVidState->CharBlockSecondary = 0;
    pVidState->MiscFlags =
                           (pBiosData->VGASettings & 0x0f)
                         | ((pBiosData->ModeOptions & 1) << 4); /* cursor emulation */
    pVidState->NonVGASupport      = 0;
    pVidState->VideoMem           = (pBiosData->ModeOptions & 0x60 >> 5);
    pVidState->SavePointerState   = 0;
    pVidState->DisplayStatus      = 4;

    /* SVGA structures */
    pVesaInfo->Signature          = *(DWORD*)"VESA";
    pVesaInfo->Major              = 2;
    pVesaInfo->Minor              = 0;
                                    /* FIXME: always real mode ? */
    pVesaInfo->StaticVendorString = BIOS_EXTRA_SEGPTR + offsetof(BIOS_EXTRA,vesa_string);
    pVesaInfo->CapabilitiesFlags  = 0xfffffffd; /* FIXME: not really supported */
                                    /* FIXME: always real mode ? */
    pVesaInfo->StaticModeList     = BIOS_EXTRA_SEGPTR + offsetof(BIOS_EXTRA,vesa_modes);

    strcpy(VesaString,ConstVesaString);
    memcpy(VesaModeList,ConstVesaModeList,sizeof(ConstVesaModeList));

    /* BIOS date string */
    strcpy((char *)pBiosSys+0xfff5, "13/01/99");

    /* BIOS ID */
    *(pBiosSys+0xfffe) = 0xfc;
}

/***********************************************************************
 *           DOSMEM_InitCollateTable
 *
 * Initialises the collate table (character sorting, language dependent)
 */
static void DOSMEM_InitCollateTable()
{
	DWORD		x;
	unsigned char	*tbl;
	int		i;

	x = GlobalDOSAlloc16(258);
	DOSMEM_CollateTable = MAKELONG(0,(x>>16));
	tbl = DOSMEM_MapRealToLinear(DOSMEM_CollateTable);
	*(WORD*)tbl	= 0x100;
	tbl += 2;
	for ( i = 0; i < 0x100; i++) *tbl++ = i;
}

/***********************************************************************
 *           DOSMEM_InitErrorTable
 *
 * Initialises the error tables (DOS 5+)
 */
static void DOSMEM_InitErrorTable()
{
#if 0  /* no longer used */
	DWORD		x;
	char 		*call;

        /* We will use a snippet of real mode code that calls */
        /* a WINE-only interrupt to handle moving the requested */
        /* message into the buffer... */

        /* FIXME - There is still something wrong... */

        /* FIXME - Find hex values for opcodes...

           (On call, AX contains message number
                     DI contains 'offset' (??)
            Resturn, ES:DI points to counted string )

           PUSH BX
           MOV BX, AX
           MOV AX, (arbitrary subfunction number)
           INT (WINE-only interrupt)
           POP BX
           RET

        */

        const int	code = 4;
        const int	buffer = 80;
        const int 	SIZE_TO_ALLOCATE = code + buffer;

        /* FIXME - Complete rewrite of the table system to save */
        /* precious DOS space. Now, we return the 0001:???? as */
        /* DOS 4+ (??, it seems to be the case in MS 7.10) treats that */
        /* as a special case and programs will use the alternate */
        /* interface (a farcall returned with INT 24 (AX = 0x122e, DL = */
        /* 0x08) which lets us have a smaller memory footprint anyway. */

 	x = GlobalDOSAlloc16(SIZE_TO_ALLOCATE);

	DOSMEM_ErrorCall = MAKELONG(0,(x>>16));
        DOSMEM_ErrorBuffer = DOSMEM_ErrorCall + code;

	call = DOSMEM_MapRealToLinear(DOSMEM_ErrorCall);

        memset(call, 0, SIZE_TO_ALLOCATE);
#endif
        /* FIXME - Copy assembly into buffer here */
}

/***********************************************************************
 *           DOSMEM_InitMemory
 *
 * Initialises the DOS memory structures.
 */
static void DOSMEM_InitMemory(void)
{
   /* Low 64Kb are reserved for DOS/BIOS so the useable area starts at
    * 1000:0000 and ends at 9FFF:FFEF. */

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
        DOSMEM_InitMemory();
        DOSMEM_InitCollateTable();
        DOSMEM_InitErrorTable();
        DOSMEM_InitDPMI();
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
 */
LPVOID DOSMEM_ResizeBlock(void* ptr, UINT size, UINT16* pseg)
{
   char         *block = NULL;
   dosmem_info  *info_block = DOSMEM_InfoBlock();

   if( ptr >= (void*)(((char*)DOSMEM_RootBlock()) + sizeof(dosmem_entry)) &&
       ptr < (void*)DOSMEM_MemoryTop() && !((((char*)ptr)
                  - DOSMEM_dosmem) & 0xf) )
   {
       dosmem_entry  *dm = (dosmem_entry*)(((char*)ptr) - sizeof(dosmem_entry));

       if( pseg ) *pseg = ((char*)ptr - DOSMEM_dosmem) >> 4;

       if( !(dm->size & (DM_BLOCK_FREE | DM_BLOCK_TERMINAL))
	 )
       {
	     dosmem_entry  *next = NEXT_BLOCK(dm);
	     UINT blocksize, orgsize = dm->size & DM_BLOCK_MASK;

	     while( next->size & DM_BLOCK_FREE ) /* collapse free blocks */
	     {
	         dm->size += sizeof(dosmem_entry) + (next->size & DM_BLOCK_MASK);
	         next->size = (DM_BLOCK_FREE | DM_BLOCK_TERMINAL);
	         next = NEXT_BLOCK(dm);
	     }

	     blocksize = dm->size & DM_BLOCK_MASK;
	     if (blocksize >= size)
	     {
	         block = ((char*)dm) + sizeof(dosmem_entry);
	         if( blocksize - size > 0x20 )
	         {
		     /* split dm so that the next one stays
		      * paragraph-aligned (and next gains free bit) */

	             dm->size = (((size + 0xf + sizeof(dosmem_entry)) & ~0xf) -
			         	        sizeof(dosmem_entry));
	             next = (dosmem_entry*)(((char*)dm) +
	 		     sizeof(dosmem_entry) + dm->size);
	             next->size = (blocksize - (dm->size +
			     sizeof(dosmem_entry))) | DM_BLOCK_FREE
						    ;
	         } else dm->size &= DM_BLOCK_MASK;

		 info_block->free += orgsize - dm->size;
	     } else {
		 /* the collapse didn't help, try getting a new block */
		 block = DOSMEM_GetBlock(size, pseg);
		 if (block) {
		     /* we got one, copy the old data there (we do need to, right?) */
		     memcpy(block, ((char*)dm) + sizeof(dosmem_entry),
				   (size<orgsize) ? size : orgsize);
		     /* free old block */
		     info_block->blocks--;
		     info_block->free += dm->size;

		     dm->size |= DM_BLOCK_FREE;
		 } else {
		     /* and Bill Gates said 640K should be enough for everyone... */

		     /* need to split original and collapsed blocks apart again,
		      * and free the collapsed blocks again, before exiting */
		     if( blocksize - orgsize > 0x20 )
		     {
			 /* split dm so that the next one stays
			  * paragraph-aligned (and next gains free bit) */

			 dm->size = (((orgsize + 0xf + sizeof(dosmem_entry)) & ~0xf) -
						       sizeof(dosmem_entry));
			 next = (dosmem_entry*)(((char*)dm) +
				 sizeof(dosmem_entry) + dm->size);
			 next->size = (blocksize - (dm->size +
				 sizeof(dosmem_entry))) | DM_BLOCK_FREE
							;
		     } else dm->size &= DM_BLOCK_MASK;
		 }
	     }
       }
   }
   return (LPVOID)block;
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
