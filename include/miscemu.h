/*
 * Misc. emulation definitions
 *
 * Copyright 1995 Alexandre Julliard
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

#ifndef __WINE_MISCEMU_H
#define __WINE_MISCEMU_H

#include "winnt.h"
#include "selectors.h"
#include "wine/windef16.h"

/* msdos/dosconf.c */
extern int DOSCONF_ReadConfig(void);

/* msdos/dosmem.c */
#include "pshpack1.h"

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
    BYTE  VideoCursorPos[16];        /* 50: Cursor position for 8 pages, column/row order */
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
    WORD  BytesPerChar WINE_PACKED;  /* 85: EGA only */
    BYTE  ModeOptions;               /* 87: EGA only */
    BYTE  FeatureBitsSwitches;       /* 88: EGA only */
    BYTE  VGASettings;               /* 89: VGA misc settings */
    BYTE  DisplayCombination;        /* 8A: VGA display combinations */
    BYTE  DiskDataRate;              /* 8B: Last disk data rate selected */
} BIOSDATA;

typedef struct
{
    DWORD StaticFuncTable;           /* 00: static functionality table */
    BYTE  VideoMode;                 /* 04: video mode in effect */
    WORD  NumberColumns;             /* 05: number of columns */
    WORD  RegenBufLen;               /* 07: length of regen buffer in bytes */
    WORD  RegenBufAddr;              /* 09: starting address of regen buffer */
    WORD  CursorPos[8];              /* 0B: cursor position for page 0..7 */
    WORD  CursorType;                /* 1B: cursor "type" (start/stop scan lines) */
    BYTE  ActivePage;                /* 1D: active display page */
    WORD  CRTCPort;                  /* 1E: CRTC port address */
    BYTE  Port3x8;                   /* 20: current setting of PORT 03x8h */
    BYTE  Port3x9;                   /* 21: current setting of PORT 03x9h */
    BYTE  NumberRows;                /* 22: number of rows - 1 */
    WORD  BytesPerChar;              /* 23: bytes/character */
    BYTE  DCCActive;                 /* 25: display combination code of active display */
    BYTE  DCCAlternate;              /* 26: DCC of alternate display */
    WORD  NumberColors;              /* 27: number of colors supported in current mode (0000h = mono) */
    BYTE  NumberPages;               /* 29: number of pages supported in current mode */
    BYTE  NumberScanlines;           /* 2A: number of scan lines active */
    BYTE  CharBlockPrimary;          /* 2B: primary character block */
    BYTE  CharBlockSecondary;        /* 2C: secondary character block */
    BYTE  MiscFlags;                 /* 2D: miscellaneous flags */
    BYTE  NonVGASupport;             /* 2E: non-VGA mode support */
    BYTE  _reserved1[2];             /* 2F: */
    BYTE  VideoMem;                  /* 31: video memory available */
    BYTE  SavePointerState;          /* 32: save pointer state flags */
    BYTE  DisplayStatus;             /* 33: display information and status */
    BYTE  _reserved2[12];            /* 34: */

} VIDEOSTATE;

typedef struct
{
    BYTE  ModeSupport[7];           /* 00: modes supported 1..7 */
    BYTE  ScanlineSupport;          /* 07: scan lines supported */
    BYTE  NumberCharBlocks;         /* 08: total number of character blocks */
    BYTE  ActiveCharBlocks;         /* 09: max. number of active character blocks */
    WORD  MiscFlags;                /* 0A: miscellaneous function support flags */
    WORD  _reserved1;               /* 0C: */
    BYTE  SavePointerFlags;         /* 0E: save pointer function flags */
    BYTE  _reserved2;               /* OF: */

} VIDEOFUNCTIONALITY;

typedef struct
{
    DWORD Signature;
    BYTE  Minor;
    BYTE  Major;
    DWORD StaticVendorString;
    DWORD CapabilitiesFlags;
    DWORD StaticModeList;
} VESAINFO;

/* layout of BIOS extra data starting at f000:e000 */
typedef struct
{
    VIDEOFUNCTIONALITY vid_func;
    VIDEOSTATE         vid_state;
    VESAINFO           vesa_info;
    char               vesa_string[32];
    WORD               vesa_modes[40];
} BIOS_EXTRA;

#define BIOS_EXTRA_PTR    ((BIOS_EXTRA *)0xfe000)
#define BIOS_EXTRA_SEGPTR MAKESEGPTR(0xf000,0xe000)

#include "poppack.h"

extern WORD DOSMEM_0000H;
extern WORD DOSMEM_BiosDataSeg;
extern WORD DOSMEM_BiosSysSeg;

/* msdos/dosmem.c */
extern BOOL DOSMEM_Init(BOOL);
extern void   DOSMEM_Tick(WORD timer);
extern WORD   DOSMEM_AllocSelector(WORD);
extern LPVOID DOSMEM_GetBlock(UINT size, WORD* p);
extern BOOL DOSMEM_FreeBlock(void* ptr);
extern LPVOID DOSMEM_ResizeBlock(void* ptr, UINT size, WORD* p);
extern UINT DOSMEM_Available(void);
extern LPVOID DOSMEM_MapRealToLinear(DWORD); /* real-mode to linear */
extern LPVOID DOSMEM_MapDosToLinear(UINT); /* linear DOS to Wine */
extern UINT DOSMEM_MapLinearToDos(LPVOID); /* linear Wine to DOS */

/* memory/instr.c */
extern BOOL INSTR_EmulateInstruction( CONTEXT86 *context );

/* msdos/ioports.c */
extern DWORD IO_inport( int port, int count );
extern void IO_outport( int port, int count, DWORD value );

/* msdos/dpmi.c */
extern BOOL DPMI_LoadDosSystem(void);

/* misc/ppdev.c */
extern BOOL IO_pp_outp(int port, DWORD* res);
extern int IO_pp_inp(int port, DWORD* res);
extern char IO_pp_init(void);

#define PTR_REAL_TO_LIN(seg,off) \
   ((void*)(((unsigned int)(seg) << 4) + LOWORD(off)))

/* NOTE: Interrupts might get called from four modes: real mode, 16-bit, 
 *       32-bit segmented (DPMI32) and 32-bit linear (via DeviceIoControl).
 *       For automatic conversion of pointer
 *       parameters, interrupt handlers should use CTX_SEG_OFF_TO_LIN with
 *       the contents of a segment register as second and the contents of
 *       a *32-bit* general register as third parameter, e.g.
 *          CTX_SEG_OFF_TO_LIN( context, DS_reg(context), EDX_reg(context) )
 *       This will generate a linear pointer in all three cases:
 *         Real-Mode:   Seg*16 + LOWORD(Offset)
 *         16-bit:      convert (Seg, LOWORD(Offset)) to linear
 *         32-bit segmented: convert (Seg, Offset) to linear
 *         32-bit linear:    use Offset as linear address (DeviceIoControl!)
 *
 *       Real-mode is recognized by checking the V86 bit in the flags register,
 *       32-bit linear mode is recognized by checking whether 'seg' is 
 *       a system selector (0 counts also as 32-bit segment) and 32-bit 
 *       segmented mode is recognized by checking whether 'seg' is 32-bit
 *       selector which is neither system selector nor zero.
 */
#define CTX_SEG_OFF_TO_LIN(context,seg,off) \
    (ISV86(context) ? PTR_REAL_TO_LIN((seg),(off)) : \
     (!seg || IS_SELECTOR_SYSTEM(seg))? (void *)(ULONG_PTR)(off) : \
      (IS_SELECTOR_32BIT(seg) ? \
       (void *)((off) + (char*)MapSL(MAKESEGPTR((seg),0))) : \
       MapSL(MAKESEGPTR((seg),(off)))))

#define INT_BARF(context,num) \
    ERR( "int%x: unknown/not implemented parameters:\n" \
                     "int%x: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (num), (num), LOWORD((context)->Eax), LOWORD((context)->Ebx), \
             LOWORD((context)->Ecx), LOWORD((context)->Edx), LOWORD((context)->Esi), \
             LOWORD((context)->Edi), (WORD)(context)->SegDs, (WORD)(context)->SegEs )

/* Macros for easier access to i386 context registers */

#define AX_reg(context)      ((WORD)(context)->Eax)
#define BX_reg(context)      ((WORD)(context)->Ebx)
#define CX_reg(context)      ((WORD)(context)->Ecx)
#define DX_reg(context)      ((WORD)(context)->Edx)
#define SI_reg(context)      ((WORD)(context)->Esi)
#define DI_reg(context)      ((WORD)(context)->Edi)

#define AL_reg(context)      ((BYTE)(context)->Eax)
#define AH_reg(context)      ((BYTE)((context)->Eax >> 8))
#define BL_reg(context)      ((BYTE)(context)->Ebx)
#define BH_reg(context)      ((BYTE)((context)->Ebx >> 8))
#define CL_reg(context)      ((BYTE)(context)->Ecx)
#define CH_reg(context)      ((BYTE)((context)->Ecx >> 8))
#define DL_reg(context)      ((BYTE)(context)->Edx)
#define DH_reg(context)      ((BYTE)((context)->Edx >> 8))

#define SET_CFLAG(context)   ((context)->EFlags |= 0x0001)
#define RESET_CFLAG(context) ((context)->EFlags &= ~0x0001)
#define SET_ZFLAG(context)   ((context)->EFlags |= 0x0040)
#define RESET_ZFLAG(context) ((context)->EFlags &= ~0x0040)
#define ISV86(context)       ((context)->EFlags & 0x00020000)

#define SET_AX(context,val)  ((context)->Eax = ((context)->Eax & ~0xffff) | (WORD)(val))
#define SET_BX(context,val)  ((context)->Ebx = ((context)->Ebx & ~0xffff) | (WORD)(val))
#define SET_CX(context,val)  ((context)->Ecx = ((context)->Ecx & ~0xffff) | (WORD)(val))
#define SET_DX(context,val)  ((context)->Edx = ((context)->Edx & ~0xffff) | (WORD)(val))
#define SET_SI(context,val)  ((context)->Esi = ((context)->Esi & ~0xffff) | (WORD)(val))
#define SET_DI(context,val)  ((context)->Edi = ((context)->Edi & ~0xffff) | (WORD)(val))

#define SET_AL(context,val)  ((context)->Eax = ((context)->Eax & ~0xff) | (BYTE)(val))
#define SET_BL(context,val)  ((context)->Ebx = ((context)->Ebx & ~0xff) | (BYTE)(val))
#define SET_CL(context,val)  ((context)->Ecx = ((context)->Ecx & ~0xff) | (BYTE)(val))
#define SET_DL(context,val)  ((context)->Edx = ((context)->Edx & ~0xff) | (BYTE)(val))

#define SET_AH(context,val)  ((context)->Eax = ((context)->Eax & ~0xff00) | (((BYTE)(val)) << 8))
#define SET_BH(context,val)  ((context)->Ebx = ((context)->Ebx & ~0xff00) | (((BYTE)(val)) << 8))
#define SET_CH(context,val)  ((context)->Ecx = ((context)->Ecx & ~0xff00) | (((BYTE)(val)) << 8))
#define SET_DH(context,val)  ((context)->Edx = ((context)->Edx & ~0xff00) | (((BYTE)(val)) << 8))

#endif /* __WINE_MISCEMU_H */
