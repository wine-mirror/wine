/*
 * Misc. emulation definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MISCEMU_H
#define __WINE_MISCEMU_H

#include <stdio.h>
#include "winnt.h"
#include "ldt.h"

/* msdos/dosconf.c */
extern int DOSCONF_ReadConfig(void);

/* msdos/dosmem.c */
extern HANDLE16 DOSMEM_BiosDataSeg;
extern HANDLE16 DOSMEM_BiosSysSeg;

extern DWORD DOSMEM_CollateTable;

extern DWORD DOSMEM_ErrorCall;
extern DWORD DOSMEM_ErrorBuffer;

extern BOOL32 DOSMEM_Init(HMODULE16 hModule);
extern void   DOSMEM_Tick(WORD timer);
extern WORD   DOSMEM_AllocSelector(WORD);
extern char * DOSMEM_MemoryBase(HMODULE16 hModule);
extern LPVOID DOSMEM_GetBlock(HMODULE16 hModule, UINT32 size, UINT16* p);
extern BOOL32 DOSMEM_FreeBlock(HMODULE16 hModule, void* ptr);
extern LPVOID DOSMEM_ResizeBlock(HMODULE16 hModule, void* ptr, UINT32 size, UINT16* p);
extern UINT32 DOSMEM_Available(HMODULE16 hModule);
extern LPVOID DOSMEM_MapRealToLinear(DWORD); /* real-mode to linear */
extern LPVOID DOSMEM_MapDosToLinear(UINT32); /* linear DOS to Wine */
extern UINT32 DOSMEM_MapLinearToDos(LPVOID); /* linear Wine to DOS */

/* msdos/interrupts.c */
extern FARPROC16 INT_GetPMHandler( BYTE intnum );
extern void INT_SetPMHandler( BYTE intnum, FARPROC16 handler );
extern FARPROC16 INT_GetRMHandler( BYTE intnum );
extern void INT_SetRMHandler( BYTE intnum, FARPROC16 handler );
extern FARPROC16 INT_CtxGetHandler( CONTEXT *context, BYTE intnum );
extern void INT_CtxSetHandler( CONTEXT *context, BYTE intnum, FARPROC16 handler );
extern int INT_RealModeInterrupt( BYTE intnum, PCONTEXT context );

/* msdos/ioports.c */
extern void IO_port_init (void);
extern DWORD IO_inport( int port, int count );
extern void IO_outport( int port, int count, DWORD value );

/* msdos/int10.c */
extern void WINAPI INT_Int10Handler(CONTEXT*);

/* msdos/int11.c */
extern void WINAPI INT_Int11Handler(CONTEXT*);

/* msdos/int13.c */
extern void WINAPI INT_Int13Handler(CONTEXT*);

/* msdos/int16.c */
extern void WINAPI INT_Int16Handler(CONTEXT*);

/* msdos/int17.c */
extern void WINAPI INT_Int17Handler(CONTEXT*);

/* msdos/int19.c */
extern void WINAPI INT_Int19Handler(CONTEXT*);

/* msdos/int1a.c */
extern DWORD INT1A_GetTicksSinceMidnight(void);
extern void WINAPI INT_Int1aHandler(CONTEXT*);

/* msdos/int20.c */
extern void WINAPI INT_Int20Handler(CONTEXT*);

/* msdos/int25.c */
extern void WINAPI INT_Int25Handler(CONTEXT*);

/* msdos/int26.c */
extern void WINAPI INT_Int26Handler(CONTEXT*);

/* msdos/int29.c */
extern void WINAPI INT_Int29Handler(CONTEXT*);

/* msdos/int2f.c */
extern void WINAPI INT_Int2fHandler(CONTEXT*);

/* msdos/dpmi.c */
typedef void WINAPI (*RMCBPROC)(CONTEXT*);
extern void WINAPI INT_Int31Handler(CONTEXT*);
extern FARPROC16 WINAPI DPMI_AllocInternalRMCB(RMCBPROC);
extern void WINAPI DPMI_FreeInternalRMCB(FARPROC16);

/* msdos/xms.c */
extern void WINAPI XMS_Handler(CONTEXT*);

/* loader/signal.c */
extern BOOL32 SIGNAL_Init(void);
extern void SIGNAL_SetHandler( int sig, void (*func)(), int flags );
extern void SIGNAL_MaskAsyncEvents( BOOL32 flag );

/* if1632/signal.c */
extern BOOL32 SIGNAL_InitEmulator(void);
extern BOOL32 (*SIGNAL_Reinit)(void);

/* misc/aspi.c */
extern void ASPI_DOS_HandleInt(CONTEXT *context);

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
