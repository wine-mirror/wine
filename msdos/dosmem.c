/*
 * DOS memory emulation
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Marcus Meissner
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "global.h"
#include "ldt.h"
#include "miscemu.h"
#include "vga.h"
#include "module.h"
#include "task.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dosmem)
DECLARE_DEBUG_CHANNEL(selector)

HANDLE16 DOSMEM_BiosDataSeg;  /* BIOS data segment at 0x40:0 */
HANDLE16 DOSMEM_BiosSysSeg;   /* BIOS ROM segment at 0xf000:0 */

static char	*DOSMEM_dosmem;

       DWORD 	 DOSMEM_CollateTable;

       DWORD	 DOSMEM_ErrorCall;
       DWORD	 DOSMEM_ErrorBuffer;

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

/***********************************************************************
 *           DOSMEM_MemoryBase
 *
 * Gets the DOS memory base.
 */
char *DOSMEM_MemoryBase(void)
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = pTask ? NE_GetPtr( pTask->hModule ) : NULL;

    GlobalUnlock16( GetCurrentTask() );
    if (pModule && pModule->dos_image)
        return pModule->dos_image;
    else
        return DOSMEM_dosmem;
}

/***********************************************************************
 *           DOSMEM_MemoryTop
 *
 * Gets the DOS memory top.
 */
static char *DOSMEM_MemoryTop(void)
{
    return DOSMEM_MemoryBase()+0x9FFFC; /* 640K */
}

/***********************************************************************
 *           DOSMEM_InfoBlock
 *
 * Gets the DOS memory info block.
 */
static dosmem_info *DOSMEM_InfoBlock(void)
{
    return (dosmem_info*)(DOSMEM_MemoryBase()+0x10000); /* 64K */
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
    SEGPTR *isr = (SEGPTR*)DOSMEM_MemoryBase();
    DWORD *stub = (DWORD*)((char*)isr + (VM_STUB_SEGMENT << 4));
    int x;
 
    for (x=0; x<256; x++) isr[x]=PTR_SEG_OFF_TO_SEGPTR(VM_STUB_SEGMENT,x*4);
    for (x=0; x<256; x++) stub[x]=VM_STUB(x);
} 

/***********************************************************************
 *           DOSMEM_InitDPMI
 *
 * Allocate the global DPMI RMCB wrapper.
 */
static void DOSMEM_InitDPMI(void)
{
    extern UINT16 DPMI_wrap_seg;
    static char wrap_code[]={
     0xCD,0x31, /* int $0x31 */
     0xCB       /* lret */
    };
    LPSTR wrapper = (LPSTR)DOSMEM_GetBlock(sizeof(wrap_code), &DPMI_wrap_seg);

    memcpy(wrapper, wrap_code, sizeof(wrap_code));
}

BIOSDATA * DOSMEM_BiosData()
{
    return (BIOSDATA *)(DOSMEM_MemoryBase()+0x400);
}

BYTE * DOSMEM_BiosSys()
{
    return DOSMEM_MemoryBase()+0xf0000;
}

struct _DOS_LISTOFLISTS * DOSMEM_LOL()
{
    return (struct _DOS_LISTOFLISTS *)DOSMEM_MapRealToLinear
      (PTR_SEG_OFF_TO_SEGPTR(HIWORD(DOS_LOLSeg),0));
}

/***********************************************************************
 *           DOSMEM_FillBiosSegments
 *
 * Fill the BIOS data segment with dummy values.
 */
static void DOSMEM_FillBiosSegments(void)
{
    BYTE *pBiosSys = (BYTE *)GlobalLock16( DOSMEM_BiosSysSeg );
    BYTE *pBiosROMTable = pBiosSys+0xe6f5;
    BIOSDATA *pBiosData = (BIOSDATA *)GlobalLock16( DOSMEM_BiosDataSeg );

    /* bogus 0xe0xx addresses !! Adapt int 0x10/0x1b if change needed */
    VIDEOFUNCTIONALITY *pVidFunc = (VIDEOFUNCTIONALITY *)(pBiosSys+0xe000);
    VIDEOSTATE *pVidState = (VIDEOSTATE *)(pBiosSys+0xe010);
    int i;

      /* Clear all unused values */
    memset( pBiosData, 0, sizeof(*pBiosData) );
    memset( pVidFunc,  0, sizeof(*pVidFunc ) );
    memset( pVidState, 0, sizeof(*pVidState) );

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
    pBiosData->Ticks                = INT1A_GetTicksSinceMidnight();
    pBiosData->NbHardDisks          = 2;
    pBiosData->KbdBufferStart       = 0x1e;
    pBiosData->KbdBufferEnd         = 0x3e;
    pBiosData->RowsOnScreenMinus1   = 23;
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

    pVidState->StaticFuncTable    = 0xf000e000;  /* FIXME: always real mode ? */
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

        /* Fixme - Copy assembly into buffer here */        
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
		     | DM_BLOCK_DEBUG;
#endif
		     ;
}

/***********************************************************************
 *           DOSMEM_MovePointers
 *
 * Relocates any pointers into DOS memory to a new address space.
 */
static void DOSMEM_MovePointers(LPVOID dest, LPVOID src, DWORD size)
{
  unsigned long delta = (char *) dest - (char *) src;
  unsigned cnt;
  ldt_entry ent;

  /* relocate base addresses of any selectors pointing into memory */
  for (cnt=FIRST_LDT_ENTRY_TO_ALLOC; cnt<LDT_SIZE; cnt++) {
    LDT_GetEntry(cnt, &ent);
    if ((ent.base >= (unsigned long)src) && \
	(ent.base < ((unsigned long)src + size))) {
      ent.base += delta;
      LDT_SetEntry(cnt, &ent);
    }
  }
}

/***********************************************************************
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values.
 */
BOOL DOSMEM_Init(HMODULE16 hModule)
{
    if (!hModule)
    {
        /* Allocate 1 MB dosmemory 
         * - it is mostly wasted but we can use some of it to 
         *   store internal translation tables, etc...
         */
        DOSMEM_dosmem = VirtualAlloc( NULL, 0x100000, MEM_COMMIT,
                                      PAGE_EXECUTE_READWRITE );
        if (!DOSMEM_dosmem)
        {
            WARN("Could not allocate DOS memory.\n" );
            return FALSE;
        }
        DOSMEM_BiosDataSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0x400,
                                     0x100, 0, FALSE, FALSE, FALSE, NULL );
        DOSMEM_BiosSysSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0xf0000,
                                     0x10000, 0, FALSE, FALSE, FALSE, NULL );
        DOSMEM_FillIsrTable();
        DOSMEM_FillBiosSegments();
        DOSMEM_InitMemory();
        DOSMEM_InitCollateTable();
        DOSMEM_InitErrorTable();
        DOSMEM_InitDPMI();
	DOSDEV_InstallDOSDevices();
    }
    else
    {
#if 0
        DOSMEM_FillIsrTable(hModule);
        DOSMEM_InitMemory(hModule);
#else
	LPVOID base = DOSMEM_MemoryBase();

        /* bootstrap the new V86 task with a copy of the "system" memory */
        memcpy(base, DOSMEM_dosmem, 0x100000);
	/* then move existing selectors to it */
	DOSMEM_MovePointers(base, DOSMEM_dosmem, 0x100000);
#endif
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
	       if( pseg ) *pseg = (block - DOSMEM_MemoryBase()) >> 4;
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
                  - DOSMEM_MemoryBase()) & 0xf) )
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
                  - DOSMEM_MemoryBase()) & 0xf) )
   {
       dosmem_entry  *dm = (dosmem_entry*)(((char*)ptr) - sizeof(dosmem_entry));

       if( pseg ) *pseg = ((char*)ptr - DOSMEM_MemoryBase()) >> 4;

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
    if (((char*)ptr >= DOSMEM_MemoryBase()) &&
        ((char*)ptr < DOSMEM_MemoryBase() + 0x100000))
	  return (UINT)ptr - (UINT)DOSMEM_MemoryBase();
    return (UINT)ptr;
}


/***********************************************************************
 *           DOSMEM_MapDosToLinear
 *
 * DOS linear address to the linear address space.
 */
LPVOID DOSMEM_MapDosToLinear(UINT ptr)
{
    if (ptr < 0x100000) return (LPVOID)(ptr + (UINT)DOSMEM_MemoryBase());
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

   lin=DOSMEM_MemoryBase()+(x&0xffff)+(((x&0xffff0000)>>16)*16);
   TRACE_(selector)("(0x%08lx) returns 0x%p.\n", x, lin );
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

	sel=GLOBAL_CreateBlock(
		GMEM_FIXED,DOSMEM_dosmem+realsel*16,0x10000,
		hModule,FALSE,FALSE,FALSE,NULL
	);
	TRACE_(selector)("(0x%04x) returns 0x%04x.\n", realsel,sel);
	return sel;
}

