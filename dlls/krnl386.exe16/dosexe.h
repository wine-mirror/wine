/*
 * DOS EXE loader
 *
 * Copyright 1998 Ove Kåven
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

#ifndef __WINE_DOSEXE_H
#define __WINE_DOSEXE_H

#include <stdarg.h>
#include <sys/types.h>

#include "windef.h"
#include "wine/windef16.h"
#include "winbase.h"
#include "winnt.h"     /* for PCONTEXT */
#include "kernel16_private.h"

#define MAX_DOS_DRIVES  26

/* amount of space reserved for relay stack */
#define DOSVM_RELAY_DATA_SIZE 4096

typedef void (*DOSRELAY)(CONTEXT*,void*);
typedef void (WINAPI *INTPROC)(CONTEXT*);

extern WORD DOSVM_psp;     /* psp of current DOS task */
extern WORD int16_sel;

#define ADD_LOWORD(dw,val)  ((dw) = ((dw) & 0xffff0000) | LOWORD((DWORD)(dw)+(val)))

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
#define CTX_SEG_OFF_TO_LIN(context,seg,off) (ldt_get_ptr((seg),(off)))

#define INT_BARF(context,num) \
    ERR( "int%x: unknown/not implemented parameters:\n" \
                     "int%x: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (num), (num), LOWORD((context)->Eax), LOWORD((context)->Ebx), \
             LOWORD((context)->Ecx), LOWORD((context)->Edx), LOWORD((context)->Esi), \
             LOWORD((context)->Edi), (WORD)(context)->SegDs, (WORD)(context)->SegEs )

/* pushing on stack in 16 bit needs segment wrap around */
#define PUSH_WORD16(context,val) \
    *((WORD*)CTX_SEG_OFF_TO_LIN((context), \
        (context)->SegSs, ADD_LOWORD( context->Esp, -2 ) )) = (val)

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

#define SET_AX(context,val)  ((void)((context)->Eax = ((context)->Eax & ~0xffff) | (WORD)(val)))
#define SET_BX(context,val)  ((void)((context)->Ebx = ((context)->Ebx & ~0xffff) | (WORD)(val)))
#define SET_CX(context,val)  ((void)((context)->Ecx = ((context)->Ecx & ~0xffff) | (WORD)(val)))
#define SET_DX(context,val)  ((void)((context)->Edx = ((context)->Edx & ~0xffff) | (WORD)(val)))
#define SET_SI(context,val)  ((void)((context)->Esi = ((context)->Esi & ~0xffff) | (WORD)(val)))
#define SET_DI(context,val)  ((void)((context)->Edi = ((context)->Edi & ~0xffff) | (WORD)(val)))

#define SET_AL(context,val)  ((void)((context)->Eax = ((context)->Eax & ~0xff) | (BYTE)(val)))
#define SET_BL(context,val)  ((void)((context)->Ebx = ((context)->Ebx & ~0xff) | (BYTE)(val)))
#define SET_CL(context,val)  ((void)((context)->Ecx = ((context)->Ecx & ~0xff) | (BYTE)(val)))
#define SET_DL(context,val)  ((void)((context)->Edx = ((context)->Edx & ~0xff) | (BYTE)(val)))

#define SET_AH(context,val)  ((void)((context)->Eax = ((context)->Eax & ~0xff00) | (((BYTE)(val)) << 8)))
#define SET_BH(context,val)  ((void)((context)->Ebx = ((context)->Ebx & ~0xff00) | (((BYTE)(val)) << 8)))
#define SET_CH(context,val)  ((void)((context)->Ecx = ((context)->Ecx & ~0xff00) | (((BYTE)(val)) << 8)))
#define SET_DH(context,val)  ((void)((context)->Edx = ((context)->Edx & ~0xff00) | (((BYTE)(val)) << 8)))

#include <pshpack1.h>

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
    WORD  MemSize;                   /* 13: Base memory size in Kb */
    WORD  unused1;                   /* 15: Manufacturing test scratch pad */
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
    WORD  VideoCtrlAddr;             /* 63: Video controller address */
    BYTE  VideoReg1;                 /* 65: Video mode select register */
    BYTE  VideoReg2;                 /* 66: Video CGA palette register */
    DWORD ResetEntry;                /* 67: Warm reset entry point */
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
    BYTE  VGASettings;               /* 89: VGA misc settings */
    BYTE  DisplayCombination;        /* 8A: VGA display combinations */
    BYTE  DiskDataRate;              /* 8B: Last disk data rate selected */
} BIOSDATA;

#include <poppack.h>

/* Device driver header */

#include <pshpack1.h>

typedef struct
{
    DWORD next_dev;
    WORD  attr;
    WORD  strategy;
    WORD  interrupt;
    char  name[8];
} DOS_DEVICE_HEADER;

#include <poppack.h>

/* dosvm.c */
extern void DOSVM_Exit( WORD retval );

/* dosmem.c */
extern BIOSDATA *DOSVM_BiosData( void );
extern void DOSVM_start_bios_timer(void);

/* fpu.c */
extern void WINAPI DOSVM_Int34Handler(CONTEXT*);
extern void WINAPI DOSVM_Int35Handler(CONTEXT*);
extern void WINAPI DOSVM_Int36Handler(CONTEXT*);
extern void WINAPI DOSVM_Int37Handler(CONTEXT*);
extern void WINAPI DOSVM_Int38Handler(CONTEXT*);
extern void WINAPI DOSVM_Int39Handler(CONTEXT*);
extern void WINAPI DOSVM_Int3aHandler(CONTEXT*);
extern void WINAPI DOSVM_Int3bHandler(CONTEXT*);
extern void WINAPI DOSVM_Int3cHandler(CONTEXT*);
extern void WINAPI DOSVM_Int3dHandler(CONTEXT*);
extern void WINAPI DOSVM_Int3eHandler(CONTEXT*);

/* int15.c */
extern void WINAPI DOSVM_Int15Handler(CONTEXT*);

/* int21.c */
extern void WINAPI DOSVM_Int21Handler(CONTEXT*);

/* int25.c */
BOOL DOSVM_RawRead( BYTE, DWORD, DWORD, BYTE *, BOOL );
void WINAPI DOSVM_Int25Handler( CONTEXT * );

/* int26.c */
BOOL DOSVM_RawWrite( BYTE, DWORD, DWORD, BYTE *, BOOL );
void WINAPI DOSVM_Int26Handler( CONTEXT * );

/* int2f.c */
extern void WINAPI DOSVM_Int2fHandler(CONTEXT*);

/* int31.c */
extern void WINAPI DOSVM_Int31Handler(CONTEXT*);

/* interrupts.c */
extern void WINAPI __wine_call_int_handler16( BYTE, CONTEXT * );
extern BOOL        DOSVM_EmulateInterruptPM( CONTEXT *, BYTE );
extern FARPROC16   DOSVM_GetPMHandler16( BYTE );
extern void        DOSVM_SetPMHandler16( BYTE, FARPROC16 );

/* ioports.c */
extern DWORD DOSVM_inport( int port, int size );
extern void DOSVM_outport( int port, int size, DWORD value );

#endif /* __WINE_DOSEXE_H */
