/*
 * DOS memory emulation
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Marcus Meissner
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "winbase.h"
#include "global.h"
#include "ldt.h"
#include "miscemu.h"
#include "module.h"
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
char	*DOSMEM_dosmem;
struct dosmem_entry {
	struct	dosmem_entry	*next;
	BYTE			isfree;
};


/***********************************************************************
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values. BUILTIN_Init() must already have been called.
 */
BOOL32 DOSMEM_Init(void)
{
    HMODULE16 hModule = GetModuleHandle( "KERNEL" );

    /* Allocate 1 MB dosmemory */
    /* Yes, allocating 1 MB of memory, which is usually not even used, is a 
     * waste of memory. But I (MM) don't see any easy method to use 
     * GlobalDOS{Alloc,Free} within an area of memory, with protected mode
     * selectors pointing into it, and the possibilty, that the userprogram
     * calls SetSelectorBase(,physical_address_in_DOSMEM); that includes 
     * dynamical enlarging (reallocing) the dosmem area.
     * Yes, one could walk the ldt_copy on every realloc() on DOSMEM, but
     * this feels more like a hack to me than this current implementation is.
     * If you find another, better, method, just change it. -Marcus Meissner
     */
    DOSMEM_dosmem = VirtualAlloc(NULL,0x1000000,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if (!DOSMEM_dosmem)
    {
        fprintf( stderr, "Could not allocate DOS memory.\n" );
        return FALSE;
    }

    MODULE_SetEntryPoint( hModule, 183,  /* KERNEL.183: __0000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    DOSMEM_BiosSeg = GLOBAL_CreateBlock(GMEM_FIXED,DOSMEM_dosmem+0x400,0x100,
                                         hModule, FALSE, FALSE, FALSE, NULL );

    MODULE_SetEntryPoint( hModule, 193,  /* KERNEL.193: __0040H */
                          DOSMEM_BiosSeg );
    MODULE_SetEntryPoint( hModule, 174,  /* KERNEL.174: __A000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xA0000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 181,  /* KERNEL.181: __B000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xB0000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 182,  /* KERNEL.182: __B800H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xB8000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 195,  /* KERNEL.195: __C000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xC0000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 179,  /* KERNEL.179: __D000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xD0000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 190,  /* KERNEL.190: __E000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xE0000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 173,  /* KERNEL.173: __ROMBIOS */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xF0000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 194,  /* KERNEL.194: __F000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, DOSMEM_dosmem+0xF0000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    DOSMEM_FillBiosSegment();

    DOSMEM_InitMemoryHandling();
    DOSMEM_InitCollateTable();

    return TRUE;
}

/***********************************************************************
 *           DOSMEM_InitMemoryHandling
 *
 * Initialises the DOS Memory structures.
 */
void
DOSMEM_InitMemoryHandling()
{
    struct	dosmem_entry	*dm;

    dm = (struct dosmem_entry*)(DOSMEM_dosmem+0x10000);
    dm->isfree	=  1;
    dm->next	=  (struct dosmem_entry*)(DOSMEM_dosmem+0x9FFF0);
    dm		=  dm->next;
    dm->isfree	= 0;
    dm->next	= NULL;
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
 *           DOSMEM_FillBiosSegment
 *
 * Fill the BIOS data segment with dummy values.
 */
void DOSMEM_FillBiosSegment(void)
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
 * Initialises the collate table (character sorting, language dependend)
 */
DWORD DOSMEM_CollateTable;

void
DOSMEM_InitCollateTable()
{
	DWORD		x;
	unsigned char	*tbl;
	int		i;

	x=GlobalDOSAlloc(258);
	DOSMEM_CollateTable=MAKELONG(0,(x>>16));
	tbl=DOSMEM_RealMode2Linear(DOSMEM_CollateTable);
	*(WORD*)tbl	= 0x100;
	tbl+=2;
	for (i=0;i<0x100;i++)
		*tbl++=i;
}

/***********************************************************************
 *           GlobalDOSAlloc	(KERNEL.184)
 *
 * Allocates a piece of DOS Memory, in the first 1 MB physical memory.
 * 
 * operates on the preallocated DOSMEM_dosmem (1MB). The useable area
 * starts at 1000:0000 and ends at 9FFF:FFEF
 * Memory allocation strategy is First Fit. (FIXME: Yes,I know that First Fit
 * is a rather bad strategy. But since those functions are rather seldom
 * called, it's easyness fits the purpose well.)
 * 
 */

DWORD GlobalDOSAlloc(DWORD size)
{
	struct	dosmem_entry	*dm,*ndm;
	DWORD	start,blocksize;
	WORD	sel;
	HMODULE16 hModule=GetModuleHandle("KERNEL");


	start	= 0;
	dm	= (struct dosmem_entry*)(DOSMEM_dosmem+0x10000);
	size	= (size+0xf)&~0xf;
	while (dm && dm->next) {
		blocksize = ((char*)dm->next-(char*)dm)-16;
		if ((dm->isfree) && (blocksize>=size)) {
			dm->isfree = 0;
			start = ((((char*)dm)-DOSMEM_dosmem)+0x10)& ~0xf;
			if ((blocksize-size) >= 0x20) {
				/* if enough memory is left for a new block
				 * split this area into two blocks
				 */
				ndm=(struct dosmem_entry*)((char*)dm+0x10+size);
				ndm->isfree	= 1;
				ndm->next	= dm->next;
				dm->next	= ndm;
			}
			break;
		}
		dm=dm->next;
	}
	if (!start)
		return 0;
	sel=GLOBAL_CreateBlock(
		GMEM_FIXED,DOSMEM_dosmem+start,size,
		hModule,FALSE,FALSE,FALSE,NULL
	);
	return MAKELONG(sel,start>>4);
}

/***********************************************************************
 *           GlobalDOSFree	(KERNEL.185)
 *
 * Frees allocated dosmemory and corresponding selector.
 */

WORD
GlobalDOSFree(WORD sel)
{
	DWORD	base;
	struct	dosmem_entry	*dm;

	base = GetSelectorBase(sel);
	/* base has already been conversed to a physical address */
	if (base>=0x100000)
		return sel;
	dm	= (struct dosmem_entry*)(DOSMEM_dosmem+base-0x10);
	if (dm->isfree) {
		fprintf(stderr,"Freeing already freed DOSMEM.\n");
		return 0;
	}
	dm->isfree = 1;

	/* collapse adjunct free blocks into one */
	dm = (struct dosmem_entry*)(DOSMEM_dosmem+0x10000);
	while (dm && dm->next) {
		if (dm->isfree && dm->next->isfree)
			dm->next = dm->next->next;
		dm = dm->next;
	}
	GLOBAL_FreeBlock(sel);
	return 0;
}

/***********************************************************************
 *           DOSMEM_RealMode2Linear
 *
 * Converts a realmode segment:offset address into a linear pointer
 */
LPVOID DOSMEM_RealMode2Linear(DWORD x)
{
	LPVOID	lin;

	lin=DOSMEM_dosmem+(x&0xffff)+(((x&0xffff0000)>>16)*16);
	dprintf_selector(stddeb,"DOSMEM_RealMode2Linear(0x%08lx) returns 0x%p.\n",
		x,lin
	);
	return lin;
}

/***********************************************************************
 *           DOSMEM_RealMode2Linear
 *
 * Allocates a protected mode selector for a realmode segment.
 */
WORD DOSMEM_AllocSelector(WORD realsel)
{
	HMODULE16 hModule=GetModuleHandle("KERNEL");
	WORD	sel;

	sel=GLOBAL_CreateBlock(
		GMEM_FIXED,DOSMEM_dosmem+realsel*16,0x10000,
		hModule,FALSE,FALSE,FALSE,NULL
	);
	dprintf_selector(stddeb,"DOSMEM_AllocSelector(0x%04x) returns 0x%04x.\n",
		realsel,sel
	);
	return sel;
}
