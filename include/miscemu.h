/*
 * Misc. emulation definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MISCEMU_H
#define __WINE_MISCEMU_H

#include "winnt.h"
#include "ldt.h"

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
    WORD  BytesPerChar WINE_PACKED;  /* 85: EGA only */
    BYTE  ModeOptions;               /* 87: EGA only */
    BYTE  FeatureBitsSwitches;       /* 88: EGA only */
    BYTE  VGASettings;               /* 89: VGA misc settings */
    BYTE  DisplayCombination;        /* 8A: VGA display combinations */
    BYTE  DiskDataRate;              /* 8B: Last disk data rate selected */
} BIOSDATA;

#include "poppack.h"

extern HANDLE16 DOSMEM_BiosDataSeg;
extern HANDLE16 DOSMEM_BiosSysSeg;
extern BIOSDATA * DOSMEM_BiosData();
extern BYTE     * DOSMEM_BiosSys();

extern DWORD DOSMEM_CollateTable;

extern DWORD DOSMEM_ErrorCall;
extern DWORD DOSMEM_ErrorBuffer;

extern DWORD DOS_LOLSeg;
extern struct _DOS_LISTOFLISTS * DOSMEM_LOL();

extern BOOL DOSMEM_Init(HMODULE16 hModule);
extern void   DOSMEM_Tick(WORD timer);
extern WORD   DOSMEM_AllocSelector(WORD);
extern char * DOSMEM_MemoryBase(HMODULE16 hModule);
extern LPVOID DOSMEM_GetBlock(HMODULE16 hModule, UINT size, UINT16* p);
extern BOOL DOSMEM_FreeBlock(HMODULE16 hModule, void* ptr);
extern LPVOID DOSMEM_ResizeBlock(HMODULE16 hModule, void* ptr, UINT size, UINT16* p);
extern UINT DOSMEM_Available(HMODULE16 hModule);
extern LPVOID DOSMEM_MapRealToLinear(DWORD); /* real-mode to linear */
extern LPVOID DOSMEM_MapDosToLinear(UINT); /* linear DOS to Wine */
extern UINT DOSMEM_MapLinearToDos(LPVOID); /* linear Wine to DOS */

/* memory/instr.c */
extern BOOL INSTR_EmulateInstruction( CONTEXT86 *context );

/* msdos/devices.c */
extern void DOSDEV_InstallDOSDevices(void);
extern DWORD DOSDEV_Console(void);
extern DWORD DOSDEV_FindCharDevice(char*name);
extern int DOSDEV_Peek(DWORD dev, BYTE*data);
extern int DOSDEV_Read(DWORD dev, DWORD buf, int buflen);
extern int DOSDEV_Write(DWORD dev, DWORD buf, int buflen, int verify);
extern int DOSDEV_IoctlRead(DWORD dev, DWORD buf, int buflen);
extern int DOSDEV_IoctlWrite(DWORD dev, DWORD buf, int buflen);

/* msdos/interrupts.c */
extern FARPROC16 INT_GetPMHandler( BYTE intnum );
extern void INT_SetPMHandler( BYTE intnum, FARPROC16 handler );
extern FARPROC16 INT_GetRMHandler( BYTE intnum );
extern void INT_SetRMHandler( BYTE intnum, FARPROC16 handler );
extern FARPROC16 INT_CtxGetHandler( CONTEXT86 *context, BYTE intnum );
extern void INT_CtxSetHandler( CONTEXT86 *context, BYTE intnum, FARPROC16 handler );
extern int INT_RealModeInterrupt( BYTE intnum, CONTEXT86 *context );

/* msdos/ioports.c */
extern void IO_port_init (void);
extern DWORD IO_inport( int port, int count );
extern void IO_outport( int port, int count, DWORD value );

/* msdos/int09.c */
extern void WINAPI INT_Int09Handler(CONTEXT86*);
extern void WINAPI INT_Int09SendScan(BYTE);
extern BYTE WINAPI INT_Int09ReadScan(void);

/* msdos/int10.c */
extern void WINAPI INT_Int10Handler(CONTEXT86*);

/* msdos/int11.c */
extern void WINAPI INT_Int11Handler(CONTEXT86*);

/* msdos/int12.c */
extern void WINAPI INT_Int12Handler(CONTEXT86*);

/* msdos/int13.c */
extern void WINAPI INT_Int13Handler(CONTEXT86*);

/* msdos/int15.c */
extern void WINAPI INT_Int15Handler(CONTEXT86*);

/* msdos/int16.c */
extern void WINAPI INT_Int16Handler(CONTEXT86*);
extern int WINAPI INT_Int16ReadChar(BYTE*ascii,BYTE*scan,BOOL peek);
extern int WINAPI INT_Int16AddChar(BYTE ascii,BYTE scan);

/* msdos/int17.c */
extern void WINAPI INT_Int17Handler(CONTEXT86*);

/* msdos/int19.c */
extern void WINAPI INT_Int19Handler(CONTEXT86*);

/* msdos/int1a.c */
extern DWORD INT1A_GetTicksSinceMidnight(void);
extern void WINAPI INT_Int1aHandler(CONTEXT86*);

/* msdos/int20.c */
extern void WINAPI INT_Int20Handler(CONTEXT86*);

/* msdos/int25.c */
extern void WINAPI INT_Int25Handler(CONTEXT86*);

/* msdos/int26.c */
extern void WINAPI INT_Int26Handler(CONTEXT86*);

/* msdos/int29.c */
extern void WINAPI INT_Int29Handler(CONTEXT86*);

/* msdos/int2a.c */
extern void WINAPI INT_Int2aHandler(CONTEXT86*);

/* msdos/int2f.c */
extern void WINAPI INT_Int2fHandler(CONTEXT86*);

/* msdos/int33.c */
extern void WINAPI INT_Int33Handler(CONTEXT86*);
extern void WINAPI INT_Int33Message(UINT,WPARAM,LPARAM);

/* msdos/dpmi.c */
typedef void WINAPI (*RMCBPROC)(CONTEXT86*);
extern void WINAPI INT_Int31Handler(CONTEXT86*);
extern FARPROC16 WINAPI DPMI_AllocInternalRMCB(RMCBPROC);
extern void WINAPI DPMI_FreeInternalRMCB(FARPROC16);
extern int DPMI_CallRMProc(CONTEXT86*,LPWORD,int,int);

/* msdos/xms.c */
extern void WINAPI XMS_Handler(CONTEXT86*);

/* misc/aspi.c */
extern void ASPI_DOS_HandleInt(CONTEXT86 *context);

/* NOTE: Interrupts might get called from three modes: real mode, 16-bit, and 
 *        (via DeviceIoControl) 32-bit. For automatic conversion of pointer 
 *       parameters, interrupt handlers should use CTX_SEG_OFF_TO_LIN with
 *       the contents of a segement register as second and the contents of
 *       a *32-bit* general register as third parameter, e.g.
 *          CTX_SEG_OFF_TO_LIN( context, DS_reg(context), EDX_reg(context) )
 *       This will generate a linear pointer in all three cases:
 *         Real-Mode:   Seg*16 + LOWORD(Offset) + V86BASE
 *         16-bit:      convert (Seg, LOWORD(Offset)) to linear
 *         32-bit:      use Offset as linear address (DeviceIoControl!)
 *
 *       Real-mode is recognized by checking the V86 bit in the flags register,
 *       32-bit mode is recognized by checking whether 'seg' is a system selector
 *       (0 counts also as 32-bit segment).
 */
#define CTX_SEG_OFF_TO_LIN(context,seg,off) \
    (ISV86(context) ? (void*)(V86BASE(context)+((seg)<<4)+(off&0xffff)) : \
     (!seg || IS_SELECTOR_SYSTEM(seg))? (void *)off : PTR_SEG_OFF_TO_LIN(seg,off&0xffff))

#define INT_BARF(context,num) \
    fprintf( stderr, "int%x: unknown/not implemented parameters:\n" \
                     "int%x: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (num), (num), AX_reg(context), BX_reg(context), CX_reg(context), \
             DX_reg(context), SI_reg(context), DI_reg(context), \
             (WORD)DS_reg(context), (WORD)ES_reg(context) )

#endif /* __WINE_MISCEMU_H */
