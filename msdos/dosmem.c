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
#include "module.h"
#include "task.h"
#include "dosexe.h"
#include "debug.h"

HANDLE16 DOSMEM_BiosSeg;  /* BIOS data segment at 0x40:0 */

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
} BIOSDATA;

#pragma pack(4)

static BIOSDATA *pBiosData = NULL;
static char	*DOSMEM_dosmem;

       DWORD 	 DOSMEM_CollateTable;

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

static dosmem_entry* 	root_block = NULL;
static dosmem_info*	info_block = NULL;

#define NEXT_BLOCK(block) \
        (dosmem_entry*)(((char*)(block)) + \
	 sizeof(dosmem_entry) + ((block)->size & DM_BLOCK_MASK))

/***********************************************************************
 *           DOSMEM_MemoryBase
 *
 * Gets the DOS memory base.
 */
static char *DOSMEM_MemoryBase(HMODULE16 hModule)
{
    TDB *pTask = hModule ? NULL : (TDB *)GlobalLock16( GetCurrentTask() );
    NE_MODULE *pModule = (hModule || pTask) ? NE_GetPtr( hModule ? hModule : pTask->hModule ) : NULL;

    if (pModule && pModule->lpDosTask)
        return pModule->lpDosTask->img;
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
 *           DOSMEM_FillBiosSegment
 *
 * Fill the BIOS data segment with dummy values.
 */
static void DOSMEM_FillBiosSegment(void)
{
    pBiosData = (BIOSDATA *)GlobalLock16( DOSMEM_BiosSeg );

      /* Clear all unused values */
    memset( pBiosData, 0, sizeof(*pBiosData) );

    /* FIXME: should check the number of configured drives and ports */

    pBiosData->Com1Addr             = 0x3e8;
    pBiosData->Com2Addr             = 0x2e8;
    pBiosData->Lpt1Addr             = 0x378;
    pBiosData->Lpt2Addr             = 0x278;
    pBiosData->InstalledHardware    = 0x8443;
    pBiosData->MemSize              = 640;
    pBiosData->NextKbdCharPtr       = 0x1e;
    pBiosData->FirstKbdCharPtr      = 0x1e;
    pBiosData->VideoMode            = 0;
    pBiosData->VideoColumns         = 80;
    pBiosData->VideoPageSize        = 80 * 25 * 2;
    pBiosData->VideoPageStartAddr   = 0xb800;
    pBiosData->VideoCtrlAddr        = 0x3d4;
    pBiosData->Ticks                = INT1A_GetTicksSinceMidnight();
    pBiosData->NbHardDisks          = 2;
    pBiosData->KbdBufferStart       = 0x1e;
    pBiosData->KbdBufferEnd         = 0x3e;
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
 *           DOSMEM_InitMemory
 *
 * Initialises the DOS memory structures.
 */
static void DOSMEM_InitMemory(HMODULE16 hModule)
{
   /* Low 64Kb are reserved for DOS/BIOS so the useable area starts at
    * 1000:0000 and ends at 9FFF:FFEF. */

    char*               dosmem = DOSMEM_MemoryBase(hModule);
    dosmem_entry*       dm;

    /* FIXME: these static variables can't cope with several DOS heaps */
    info_block = (dosmem_info*)( dosmem + 0x10000 );

    /* first block has to be paragraph-aligned relative to the DOSMEM_dosmem */

    root_block = (dosmem_entry*)( dosmem + 0x10000 +
                 ((((sizeof(dosmem_info) + 0xf) & ~0xf) - sizeof(dosmem_entry))));
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
         * - it is mostly wasted but we use can some of it to 
         *   store internal translation tables, etc...
         */
        DOSMEM_dosmem = VirtualAlloc( NULL, 0x100000, MEM_COMMIT,
                                      PAGE_EXECUTE_READWRITE );
        if (!DOSMEM_dosmem)
        {
            WARN(dosmem, "Could not allocate DOS memory.\n" );
            return FALSE;
        }
        DOSMEM_BiosSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0x400,0x100,
                                        0, FALSE, FALSE, FALSE, NULL );
        DOSMEM_FillBiosSegment();
        DOSMEM_InitMemory(0);
        DOSMEM_InitCollateTable();
    }
    else
        DOSMEM_InitMemory(hModule);
    return TRUE;
}


/***********************************************************************
 *           DOSMEM_Tick
 *
 * Increment the BIOS tick counter. Called by timer signal handler.
 */
void DOSMEM_Tick(void)
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
   dosmem_entry *dm;
#ifdef __DOSMEM_DEBUG_
   dosmem_entry *prev = NULL;
#endif
 
   if( size > info_block->free ) return NULL;
   dm = root_block;

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
   if( ptr >= (void*)(((char*)root_block) + sizeof(dosmem_entry)) &&
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

   if( ptr >= (void*)(((char*)root_block) + sizeof(dosmem_entry)) &&
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
   char         *block = NULL;
   dosmem_entry *dm;
   
   dm = root_block;

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

