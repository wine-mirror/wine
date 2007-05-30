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

#include "windef.h"
#include "wine/library.h"
#include "wine/windef16.h"
#include "winbase.h"
#include "winnt.h"     /* for PCONTEXT */
#include "wincon.h"    /* for MOUSE_EVENT_RECORD */

#define MAX_DOS_DRIVES  26

struct _DOSEVENT;

/* amount of space reserved for relay stack */
#define DOSVM_RELAY_DATA_SIZE 4096

/* various real-mode code stubs */
struct DPMI_segments
{
    WORD wrap_seg;
    WORD xms_seg;
    WORD dpmi_seg;
    WORD dpmi_sel;
    WORD int48_sel;
    WORD int16_sel;
    WORD relay_code_sel;
    WORD relay_data_sel;
};

/* 48-bit segmented pointers for DOS DPMI32 */
typedef struct {
  WORD  selector;
  DWORD offset;
} SEGPTR48, FARPROC48;

#define DOSCONF_MEM_HIGH        0x0001
#define DOSCONF_MEM_UMB         0x0002
#define DOSCONF_NUMLOCK         0x0004
#define DOSCONF_KEYB_CONV       0x0008

typedef struct {
    char lastdrive;
    int brk_flag;
    int files;
    int stacks_nr;
    int stacks_sz;
    int buf;
    int buf2;
    int fcbs;
    int flags;
    char *shell;
    char *country;
} DOSCONF;

typedef void (*DOSRELAY)(CONTEXT86*,void*);
typedef void (WINAPI *RMCBPROC)(CONTEXT86*);
typedef void (WINAPI *INTPROC)(CONTEXT86*);

#define DOS_PRIORITY_REALTIME 0  /* IRQ0 */
#define DOS_PRIORITY_KEYBOARD 1  /* IRQ1 */
#define DOS_PRIORITY_VGA      2  /* IRQ9 */
#define DOS_PRIORITY_MOUSE    5  /* IRQ12 */
#define DOS_PRIORITY_SERIAL   10 /* IRQ4 */

extern WORD DOSVM_psp;     /* psp of current DOS task */
extern WORD DOSVM_retval;  /* return value of previous DOS task */
extern struct DPMI_segments *DOSVM_dpmi_segments;

#if defined(linux) && defined(__i386__) && defined(HAVE_SYS_VM86_H)
# define MZ_SUPPORTED
#endif /* linux-i386 */

/*
 * Declare some CONTEXT86.EFlags bits.
 * IF_MASK is only pushed into real mode stack.
 */
#define V86_FLAG 0x00020000
#define TF_MASK  0x00000100
#define IF_MASK  0x00000200
#define VIF_MASK 0x00080000
#define VIP_MASK 0x00100000

#define ADD_LOWORD(dw,val)  ((dw) = ((dw) & 0xffff0000) | LOWORD((DWORD)(dw)+(val)))

#define PTR_REAL_TO_LIN(seg,off) ((void*)(((unsigned int)(seg) << 4) + LOWORD(off)))

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
    (ISV86(context) ? PTR_REAL_TO_LIN((seg),(off)) : wine_ldt_get_ptr((seg),(off)))

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
#define ISV86(context)       ((context)->EFlags & 0x00020000)

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

#define NONEXT ((DWORD)-1)

#define ATTR_STDIN     0x0001
#define ATTR_STDOUT    0x0002
#define ATTR_NUL       0x0004
#define ATTR_CLOCK     0x0008
#define ATTR_FASTCON   0x0010
#define ATTR_RAW       0x0020
#define ATTR_NOTEOF    0x0040
#define ATTR_DEVICE    0x0080
#define ATTR_REMOVABLE 0x0800
#define ATTR_NONIBM    0x2000 /* block devices */
#define ATTR_UNTILBUSY 0x2000 /* char devices */
#define ATTR_IOCTL     0x4000
#define ATTR_CHAR      0x8000

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

/* DOS Device requests */

#define CMD_INIT       0
#define CMD_MEDIACHECK 1 /* block devices */
#define CMD_BUILDBPB   2 /* block devices */
#define CMD_INIOCTL    3
#define CMD_INPUT      4 /* read data */
#define CMD_SAFEINPUT  5 /* "non-destructive input no wait", char devices */
#define CMD_INSTATUS   6 /* char devices */
#define CMD_INFLUSH    7 /* char devices */
#define CMD_OUTPUT     8 /* write data */
#define CMD_SAFEOUTPUT 9 /* write data with verify */
#define CMD_OUTSTATUS 10 /* char devices */
#define CMD_OUTFLUSH  11 /* char devices */
#define CMD_OUTIOCTL  12
#define CMD_DEVOPEN   13
#define CMD_DEVCLOSE  14
#define CMD_REMOVABLE 15 /* block devices */
#define CMD_UNTILBUSY 16 /* output until busy */

#define STAT_MASK  0x00FF
#define STAT_DONE  0x0100
#define STAT_BUSY  0x0200
#define STAT_ERROR 0x8000

#include <pshpack1.h>

typedef struct {
    BYTE size;          /* length of header + data */
    BYTE unit;          /* unit (block devices only) */
    BYTE command;
    WORD status;
    BYTE reserved[8];
} REQUEST_HEADER;

typedef struct {
    REQUEST_HEADER hdr;
    BYTE media;         /* media descriptor from BPB */
    SEGPTR buffer;
    WORD count;         /* byte/sector count */
    WORD sector;        /* starting sector (block devices) */
    DWORD volume;       /* volume ID (block devices) */
} REQ_IO;

typedef struct {
    REQUEST_HEADER hdr;
    BYTE data;
} REQ_SAFEINPUT;

/* WINE device driver thunk from RM */
typedef struct {
    BYTE ljmp1;
    FARPROC16 strategy;
    BYTE ljmp2;
    FARPROC16 interrupt;
} WINEDEV_THUNK;

#include <poppack.h>

/* Device driver info (used for initialization) */
typedef struct
{
    char name[8];
    WORD attr;
    RMCBPROC strategy;
    RMCBPROC interrupt;
} WINEDEV;

/* module.c */
extern void WINAPI MZ_LoadImage( LPCSTR filename, HANDLE hFile );
extern BOOL WINAPI MZ_Exec( CONTEXT86 *context, LPCSTR filename, BYTE func, LPVOID paramblk );
extern void WINAPI MZ_Exit( CONTEXT86 *context, BOOL cs_psp, WORD retval );
extern BOOL WINAPI MZ_Current( void );
extern void WINAPI MZ_AllocDPMITask( void );
extern void WINAPI MZ_RunInThread( PAPCFUNC proc, ULONG_PTR arg );
extern BOOL DOSVM_IsWin16(void);

/* dosvm.c */
extern void DOSVM_SendQueuedEvents( CONTEXT86 * );
extern void WINAPI DOSVM_AcknowledgeIRQ( CONTEXT86 * );
extern INT WINAPI DOSVM_Enter( CONTEXT86 *context );
extern void WINAPI DOSVM_Wait( CONTEXT86 * );
extern DWORD WINAPI DOSVM_Loop( HANDLE hThread );
extern void WINAPI DOSVM_QueueEvent( INT irq, INT priority, DOSRELAY relay, LPVOID data );
extern void WINAPI DOSVM_PIC_ioport_out( WORD port, BYTE val );
extern void WINAPI DOSVM_SetTimer( UINT ticks );
extern UINT WINAPI DOSVM_GetTimer( void );

/* devices.c */
extern void DOSDEV_InstallDOSDevices(void);
extern void DOSDEV_SetupDevice(const WINEDEV * devinfo,
                               WORD seg, WORD off_dev, WORD off_thunk);
extern DWORD DOSDEV_Console(void);
extern DWORD DOSDEV_FindCharDevice(char*name);
extern int DOSDEV_Peek(DWORD dev, BYTE*data);
extern int DOSDEV_Read(DWORD dev, DWORD buf, int buflen);
extern int DOSDEV_Write(DWORD dev, DWORD buf, int buflen, int verify);
extern int DOSDEV_IoctlRead(DWORD dev, DWORD buf, int buflen);
extern int DOSDEV_IoctlWrite(DWORD dev, DWORD buf, int buflen);
extern void DOSDEV_SetSharingRetry(WORD delay, WORD count);
extern SEGPTR DOSDEV_GetLOL(BOOL v86);

/* dma.c */
extern int DMA_Transfer(int channel,int reqlength,void* buffer);
extern void DMA_ioport_out( WORD port, BYTE val );
extern BYTE DMA_ioport_in( WORD port );

/* dosaspi.c */
extern void WINAPI DOSVM_ASPIHandler(CONTEXT86*);

/* dosconf.c */
extern DOSCONF *DOSCONF_GetConfig( void );

/* dosmem.c */
extern BOOL DOSMEM_InitDosMemory(void);
extern BOOL DOSMEM_MapDosLayout(void);
extern WORD   DOSMEM_AllocSelector(WORD); /* FIXME: to be removed */
extern LPVOID DOSMEM_AllocBlock(UINT size, WORD* p);
extern BOOL DOSMEM_FreeBlock(void* ptr);
extern UINT DOSMEM_ResizeBlock(void* ptr, UINT size, BOOL exact);
extern UINT DOSMEM_Available(void);
extern BIOSDATA *DOSVM_BiosData( void );

/* fpu.c */
extern void WINAPI DOSVM_Int34Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int35Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int36Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int37Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int38Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int39Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int3aHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3bHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3cHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3dHandler(CONTEXT86*);
extern void WINAPI DOSVM_Int3eHandler(CONTEXT86*);

/* himem.c */
extern void DOSVM_InitSegments(void);
extern LPVOID DOSVM_AllocUMB(DWORD);
extern LPVOID DOSVM_AllocCodeUMB(DWORD, WORD *, WORD *);
extern LPVOID DOSVM_AllocDataUMB(DWORD, WORD *, WORD *);

/* int09.c */
extern void WINAPI DOSVM_Int09Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int09SendScan(BYTE scan,BYTE ascii);
extern BYTE WINAPI DOSVM_Int09ReadScan(BYTE*ascii);

/* int10.c */
extern void WINAPI DOSVM_Int10Handler(CONTEXT86*);
extern void WINAPI DOSVM_PutChar(BYTE ascii);

/* int13.c */
extern void WINAPI DOSVM_Int13Handler(CONTEXT86*);

/* int15.c */
extern void WINAPI DOSVM_Int15Handler(CONTEXT86*);

/* int16.c */
extern void WINAPI DOSVM_Int16Handler(CONTEXT86*);
extern BOOL WINAPI DOSVM_Int16ReadChar( BYTE *, BYTE *, CONTEXT86 * );
extern int WINAPI DOSVM_Int16AddChar(BYTE ascii,BYTE scan);

/* int21.c */
extern void WINAPI DOSVM_Int21Handler(CONTEXT86*);

/* int25.c */
BOOL DOSVM_RawRead( BYTE, DWORD, DWORD, BYTE *, BOOL );
void WINAPI DOSVM_Int25Handler( CONTEXT86 * );

/* int26.c */
BOOL DOSVM_RawWrite( BYTE, DWORD, DWORD, BYTE *, BOOL );
void WINAPI DOSVM_Int26Handler( CONTEXT86 * );

/* int2f.c */
extern void WINAPI DOSVM_Int2fHandler(CONTEXT86*);
extern void MSCDEX_InstallCDROM(void);

/* int31.c */
extern void WINAPI DOSVM_Int31Handler(CONTEXT86*);
extern void WINAPI DOSVM_RawModeSwitchHandler(CONTEXT86*);
extern BOOL DOSVM_IsDos32(void);
extern FARPROC16 WINAPI DPMI_AllocInternalRMCB(RMCBPROC);
extern void WINAPI DPMI_FreeInternalRMCB(FARPROC16);
extern int DPMI_CallRMProc(CONTEXT86*,LPWORD,int,int);
extern BOOL DOSVM_CheckWrappers(CONTEXT86*);

/* int33.c */
extern void WINAPI DOSVM_Int33Handler(CONTEXT86*);
extern void WINAPI DOSVM_Int33Message(UINT,WPARAM,LPARAM);
extern void WINAPI DOSVM_Int33Console(MOUSE_EVENT_RECORD*);

/* int67.c */
extern void WINAPI DOSVM_Int67Handler(CONTEXT86*);
extern void WINAPI EMS_Ioctl_Handler(CONTEXT86*);

/* interrupts.c */
extern void WINAPI DOSVM_CallBuiltinHandler( CONTEXT86 *, BYTE );
extern void WINAPI DOSVM_EmulateInterruptPM( CONTEXT86 *, BYTE );
extern BOOL WINAPI DOSVM_EmulateInterruptRM( CONTEXT86 *, BYTE );
extern FARPROC16   DOSVM_GetPMHandler16( BYTE );
extern FARPROC48   DOSVM_GetPMHandler48( BYTE );
extern FARPROC16   DOSVM_GetRMHandler( BYTE );
extern void        DOSVM_HardwareInterruptPM( CONTEXT86 *, BYTE );
extern void        DOSVM_HardwareInterruptRM( CONTEXT86 *, BYTE );
extern void        DOSVM_SetPMHandler16( BYTE, FARPROC16 );
extern void        DOSVM_SetPMHandler48( BYTE, FARPROC48 );
extern void        DOSVM_SetRMHandler( BYTE, FARPROC16 );

/* relay.c */
void DOSVM_RelayHandler( CONTEXT86 * );
void DOSVM_BuildCallFrame( CONTEXT86 *, DOSRELAY, LPVOID );

/* soundblaster.c */
extern void SB_ioport_out( WORD port, BYTE val );
extern BYTE SB_ioport_in( WORD port );

/* ppdev.c */
extern BOOL IO_pp_outp(int port, DWORD* res);
extern int IO_pp_inp(int port, DWORD* res);
extern char IO_pp_init(void);

/* timer.c */
extern void WINAPI DOSVM_Int08Handler(CONTEXT86*);

/* xms.c */
extern void WINAPI XMS_Handler(CONTEXT86*);

#endif /* __WINE_DOSEXE_H */
