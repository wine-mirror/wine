/*
 * DOS memory emulation
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Marcus Meissner
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "winbase.h"
#include "global.h"
#include "ldt.h"
#include "miscemu.h"
#include "vga.h"
#include "module.h"
#include "task.h"
#include "debug.h"

HANDLE16 DOSMEM_BiosDataSeg;  /* BIOS data segment at 0x40:0 */
HANDLE16 DOSMEM_BiosSysSeg;   /* BIOS ROM segment at 0xf000:0 */

#pragma pack(1)

typedef struct
{
    WORD  Com1Addr;                  /* 00: COM1 I/O address */
    WORD  Com2Addr;                  /* 02: COM2 I/O address */
    WORD  Com3Addr;                  /* 04: COM3 I/O address */
    WORD  Com4Addr;                  /* 06: COM4 I/O address */
    WORD  Lpt1Addr;                  /* 08: LPT1 I/O address */
    WORD  Lpt2Addr;                  /* 0a: LPT2 I/O address */
    WORD  Lpt3Addr;                  /* 0c: LPT3 I/O address */
    WORD  Lpt4Addr;                  /* 0e: LPT4 I/O address */
    WORD  InstalledHardware;         /* 10: Installed hardware flags */
    BYTE  POSTstatus;                /* 12: Power-On Self Test status */
    WORD  MemSize WINE_PACKED;       /* 13: Base memory size in Kb */
    WORD  unused1 WINE_PACKED;       /* 15: Manufacturing test scratch pad */
    BYTE  KbdFlags1;                 /* 17: Keyboard flags 1 */
    BYTE  KbdFlags2;                 /* 18: Keyboard flags 2 */
    BYTE  unused2;                   /* 19: Keyboard driver workspace */
    WORD  NextKbdCharPtr;            /* 1a: Next character in kbd buffer */
    WORD  FirstKbdCharPtr;           /* 1c: First character in kbd buffer */
    WORD  KbdBuffer[16];             /* 1e: Keyboard buffer */
    BYTE  DisketteStatus1;           /* 3e: Diskette recalibrate status */
    BYTE  DisketteStatus2;           /* 3f: Diskette motor status */
    BYTE  DisketteStatus3;           /* 40: Diskette motor timeout */
    BYTE  DisketteStatus4;           /* 41: Diskette last operation status */
    BYTE  DiskStatus[7];             /* 42: Disk status/command bytes */
    BYTE  VideoMode;                 /* 49: Video mode */
    WORD  VideoColumns;              /* 4a: Number of columns */
    WORD  VideoPageSize;             /* 4c: Video page size in bytes */
    WORD  VideoPageStartAddr;        /* 4e: Video page start address */
    BYTE  VideoCursorPos[16];        /* 50: Cursor position for 8 pages */
    WORD  VideoCursorType;           /* 60: Video cursor type */
    BYTE  VideoCurPage;              /* 62: Video current page */
    WORD  VideoCtrlAddr WINE_PACKED; /* 63: Video controller address */
    BYTE  VideoReg1;                 /* 65: Video mode select register */
    BYTE  VideoReg2;                 /* 66: Video CGA palette register */
    DWORD ResetEntry WINE_PACKED;    /* 67: Warm reset entry point */
    BYTE  LastIRQ;                   /* 6b: Last unexpected interrupt */
    DWORD Ticks;                     /* 6c: Ticks since midnight */
    BYTE  TicksOverflow;             /* 70: Timer overflow if past midnight */
    BYTE  CtrlBreakFlag;             /* 71: Ctrl-Break flag */
    WORD  ResetFlag;                 /* 72: POST Reset flag */
    BYTE  DiskOpStatus;              /* 74: Last hard-disk operation status */
    BYTE  NbHardDisks;               /* 75: Number of hard disks */
    BYTE  DiskCtrlByte;              /* 76: Disk control byte */
    BYTE  DiskIOPort;                /* 77: Disk I/O port offset */
    BYTE  LptTimeout[4];             /* 78: Timeouts for parallel ports */
    BYTE  ComTimeout[4];             /* 7c: Timeouts for serial ports */
    WORD  KbdBufferStart;            /* 80: Keyboard buffer start */
    WORD  KbdBufferEnd;              /* 82: Keyboard buffer end */
    BYTE  RowsOnScreenMinus1;        /* 84: EGA only */
    WORD  BytesPerChar;              /* 85: EGA only */
    BYTE  ModeOptions;               /* 87: EGA only */
    BYTE  FeatureBitsSwitches;       /* 88: EGA only */
    BYTE  unknown;                   /* 89: ??? */
    BYTE  DisplayCombination;        /* 8A: VGA display combinations */
    BYTE  DiskDataRate;              /* 8B: Last disk data rate selected */
} BIOSDATA;

#pragma pack(4)

static BIOSDATA *pBiosData = NULL;
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
char *DOSMEM_MemoryBase(HMODULE16 hModule)
{
    TDB *pTask = hModule ? NULL : (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = (hModule || pTask) ? NE_GetPtr( hModule ? hModule : pTask->hModule ) : NULL;

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
static char *DOSMEM_MemoryTop(HMODULE16 hModule)
{
    return DOSMEM_MemoryBase(hModule)+0x9FFFC; /* 640K */
}

/***********************************************************************
 *           DOSMEM_InfoBlock
 *
 * Gets the DOS memory info block.
 */
static dosmem_info *DOSMEM_InfoBlock(HMODULE16 hModule)
{
    return (dosmem_info*)(DOSMEM_MemoryBase(hModule)+0x10000); /* 64K */
}

/***********************************************************************
 *           DOSMEM_RootBlock
 *
 * Gets the DOS memory root block.
 */
static dosmem_entry *DOSMEM_RootBlock(HMODULE16 hModule)
{
    /* first block has to be paragraph-aligned */
    return (dosmem_entry*)(((char*)DOSMEM_InfoBlock(hModule)) +
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
static void DOSMEM_FillIsrTable(HMODULE16 hModule)
{
    SEGPTR *isr = (SEGPTR*)DOSMEM_MemoryBase(hModule);
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
    LPSTR wrapper = (LPSTR)DOSMEM_GetBlock(0, sizeof(wrap_code), &DPMI_wrap_seg);

    memcpy(wrapper, wrap_code, sizeof(wrap_code));
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

    pBiosData = (BIOSDATA *)GlobalLock16( DOSMEM_BiosDataSeg );

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
    pBiosData->Ticks                = INT1A_GetTicksSinceMidnight();
    pBiosData->NbHardDisks          = 2;
    pBiosData->KbdBufferStart       = 0x1e;
    pBiosData->KbdBufferEnd         = 0x3e;
    pBiosData->RowsOnScreenMinus1   = 23;
    pBiosData->BytesPerChar         = 0x10;
    pBiosData->ModeOptions          = 0x64;
    pBiosData->FeatureBitsSwitches  = 0xf9;
    pBiosData->unknown              = 0x51;
    pBiosData->DisplayCombination   = 0x08;
    pBiosData->DiskDataRate         = 0;

    /* fill ROM configuration table (values from Award) */
    *(WORD *)(pBiosROMTable)= 0x08; /* number of bytes following */
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

	x = GlobalDOSAlloc(258);
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
 
 	x = GlobalDOSAlloc(SIZE_TO_ALLOCATE);  

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
static void DOSMEM_InitMemory(HMODULE16 hModule)
{
   /* Low 64Kb are reserved for DOS/BIOS so the useable area starts at
    * 1000:0000 and ends at 9FFF:FFEF. */

    dosmem_info*        info_block = DOSMEM_InfoBlock(hModule);
    dosmem_entry*       root_block = DOSMEM_RootBlock(hModule);
    dosmem_entry*       dm;

    root_block->size = DOSMEM_MemoryTop(hModule) - (((char*)root_block) + sizeof(dosmem_entry));

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
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values.
 */
BOOL32 DOSMEM_Init(HMODULE16 hModule)
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
            WARN(dosmem, "Could not allocate DOS memory.\n" );
            return FALSE;
        }
        DOSMEM_BiosDataSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0x400,
                                     0x100, 0, FALSE, FALSE, FALSE, NULL );
        DOSMEM_BiosSysSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0xf0000,
                                     0x10000, 0, FALSE, FALSE, FALSE, NULL );
        DOSMEM_FillIsrTable(0);
        DOSMEM_FillBiosSegments();
        DOSMEM_InitMemory(0);
        DOSMEM_InitCollateTable();
        DOSMEM_InitErrorTable();
        DOSMEM_InitDPMI();
    }
    else
    {
#if 0
        DOSMEM_FillIsrTable(hModule);
        DOSMEM_InitMemory(hModule);
#else
        /* bootstrap the new V86 task with a copy of the "system" memory */
        memcpy(DOSMEM_MemoryBase(hModule), DOSMEM_dosmem, 0x100000);
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
    if (pBiosData) pBiosData->Ticks++;
}

/***********************************************************************
 *           DOSMEM_GetBlock
 *
 * Carve a chunk of the DOS memory block (without selector).
 */
LPVOID DOSMEM_GetBlock(HMODULE16 hModule, UINT32 size, UINT16* pseg)
{
   UINT32  	 blocksize;
   char         *block = NULL;
   dosmem_info  *info_block = DOSMEM_InfoBlock(hModule);
   dosmem_entry *dm;
#ifdef __DOSMEM_DEBUG_
   dosmem_entry *prev = NULL;
#endif
 
   if( size > info_block->free ) return NULL;
   dm = DOSMEM_RootBlock(hModule);

   while (dm && dm->size != DM_BLOCK_TERMINAL)
   {
#ifdef __DOSMEM_DEBUG__
       if( (dm->size & DM_BLOCK_DEBUG) != DM_BLOCK_DEBUG )
       {
	    WARN(dosmem,"MCB overrun! [prev = 0x%08x]\n", 4 + (UINT32)prev);
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
	       if( pseg ) *pseg = (block - DOSMEM_MemoryBase(hModule)) >> 4;
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
BOOL32 DOSMEM_FreeBlock(HMODULE16 hModule, void* ptr)
{
   dosmem_info  *info_block = DOSMEM_InfoBlock(hModule);

   if( ptr >= (void*)(((char*)DOSMEM_RootBlock(hModule)) + sizeof(dosmem_entry)) &&
       ptr < (void*)DOSMEM_MemoryTop(hModule) && !((((char*)ptr)
                  - DOSMEM_MemoryBase(hModule)) & 0xf) )
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
LPVOID DOSMEM_ResizeBlock(HMODULE16 hModule, void* ptr, UINT32 size, UINT16* pseg)
{
   char         *block = NULL;
   dosmem_info  *info_block = DOSMEM_InfoBlock(hModule);

   if( ptr >= (void*)(((char*)DOSMEM_RootBlock(hModule)) + sizeof(dosmem_entry)) &&
       ptr < (void*)DOSMEM_MemoryTop(hModule) && !((((char*)ptr)
                  - DOSMEM_MemoryBase(hModule)) & 0xf) )
   {
       dosmem_entry  *dm = (dosmem_entry*)(((char*)ptr) - sizeof(dosmem_entry));

       if( pseg ) *pseg = ((char*)ptr - DOSMEM_MemoryBase(hModule)) >> 4;

       if( !(dm->size & (DM_BLOCK_FREE | DM_BLOCK_TERMINAL))
	 )
       {
	     dosmem_entry  *next = NEXT_BLOCK(dm);
	     UINT32 blocksize, orgsize = dm->size & DM_BLOCK_MASK;

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
		 block = DOSMEM_GetBlock(hModule, size, pseg);
		 if (block) {
		     info_block->blocks--;
		     info_block->free += dm->size;

		     dm->size |= DM_BLOCK_FREE;
		 }
	     }
       }
   }
   return (LPVOID)block;
}


/***********************************************************************
 *           DOSMEM_Available
 */
UINT32 DOSMEM_Available(HMODULE16 hModule)
{
   UINT32  	 blocksize, available = 0;
   dosmem_entry *dm;
   
   dm = DOSMEM_RootBlock(hModule);

   while (dm && dm->size != DM_BLOCK_TERMINAL)
   {
#ifdef __DOSMEM_DEBUG__
       if( (dm->size & DM_BLOCK_DEBUG) != DM_BLOCK_DEBUG )
       {
	    WARN(dosmem,"MCB overrun! [prev = 0x%08x]\n", 4 + (UINT32)prev);
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
UINT32 DOSMEM_MapLinearToDos(LPVOID ptr)
{
    if (((char*)ptr >= DOSMEM_MemoryBase(0)) &&
        ((char*)ptr < DOSMEM_MemoryBase(0) + 0x100000))
	  return (UINT32)ptr - (UINT32)DOSMEM_MemoryBase(0);
    return (UINT32)ptr;
}


/***********************************************************************
 *           DOSMEM_MapDosToLinear
 *
 * DOS linear address to the linear address space.
 */
LPVOID DOSMEM_MapDosToLinear(UINT32 ptr)
{
    if (ptr < 0x100000) return (LPVOID)(ptr + (UINT32)DOSMEM_MemoryBase(0));
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

   lin=DOSMEM_MemoryBase(0)+(x&0xffff)+(((x&0xffff0000)>>16)*16);
   TRACE(selector,"(0x%08lx) returns 0x%p.\n",
                    x,lin );
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
	TRACE(selector,"(0x%04x) returns 0x%04x.\n",
		realsel,sel
	);
	return sel;
}

