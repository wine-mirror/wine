/*
 * DOS memory emulation
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "global.h"
#include "ldt.h"
#include "miscemu.h"
#include "module.h"
#include "xmalloc.h"


HANDLE DOSMEM_BiosSeg;  /* BIOS data segment at 0x40:0 */


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


/***********************************************************************
 *           DOSMEM_Init
 *
 * Create the dos memory segments, and store them into the KERNEL
 * exported values. MODULE_Init() must already have been called.
 */
BOOL DOSMEM_Init(void)
{
    HMODULE hModule = GetModuleHandle( "KERNEL" );
    char *dosmem;

    /* Allocate 7 64k segments for 0000, A000, B000, C000, D000, E000, F000. */

    dosmem = xmalloc( 0x70000 );

    MODULE_SetEntryPoint( hModule, 183,  /* KERNEL.183: __0000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    DOSMEM_BiosSeg = GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x400, 0x100,
                                         hModule, FALSE, FALSE, FALSE, NULL );

    MODULE_SetEntryPoint( hModule, 193,  /* KERNEL.193: __0040H */
                          DOSMEM_BiosSeg );
    MODULE_SetEntryPoint( hModule, 174,  /* KERNEL.174: __A000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x10000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 181,  /* KERNEL.181: __B000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x20000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 182,  /* KERNEL.182: __B800H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x28000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 195,  /* KERNEL.195: __C000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x30000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 179,  /* KERNEL.179: __D000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x40000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 190,  /* KERNEL.190: __E000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x50000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 173,  /* KERNEL.173: __ROMBIOS */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x60000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 194,  /* KERNEL.194: __F000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x60000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    DOSMEM_FillBiosSegment();

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
 *           DOSMEM_FillBiosSegment
 *
 * Fill the BIOS data segment with dummy values.
 */
void DOSMEM_FillBiosSegment(void)
{
    pBiosData = (BIOSDATA *)GlobalLock( DOSMEM_BiosSeg );

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

