/*
 * Misc. emulation definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MISCEMU_H
#define __WINE_MISCEMU_H

#include <stdio.h>
#include "winnt.h"

  /* msdos/dosmem.c */
extern HANDLE16 DOSMEM_BiosSeg;
extern DWORD DOSMEM_CollateTable;

extern BOOL32 DOSMEM_Init(HMODULE16 hModule);
extern void   DOSMEM_Tick(void);
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
extern FARPROC16 INT_GetHandler( BYTE intnum );
extern void INT_SetHandler( BYTE intnum, FARPROC16 handler );

/* msdos/ioports.c */
extern void IO_port_init (void);
extern DWORD IO_inport( int port, int count );
extern void IO_outport( int port, int count, DWORD value );

/* msdos/int1a.c */
extern DWORD INT1A_GetTicksSinceMidnight(void);
extern void WINAPI INT_Int1aHandler(CONTEXT*);

/* msdos/int2f.c */
extern void WINAPI INT_Int2fHandler(CONTEXT*);

/* loader/signal.c */
extern BOOL32 SIGNAL_Init(void);
extern void SIGNAL_SetHandler( int sig, void (*func)(), int flags );
extern void SIGNAL_MaskAsyncEvents( BOOL32 flag );

/* if1632/signal.c */
extern BOOL32 SIGNAL_InitEmulator(void);

/* misc/aspi.c */
extern void ASPI_DOS_HandleInt(CONTEXT *context);

#define CTX_SEG_OFF_TO_LIN(context,seg,off) \
    (ISV86(context) ? (void*)(V86BASE(context)+((seg)<<4)+off) \
                    : PTR_SEG_OFF_TO_LIN(seg,off))

#define INT_BARF(context,num) \
    fprintf( stderr, "int%x: unknown/not implemented parameters:\n" \
                     "int%x: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (num), (num), AX_reg(context), BX_reg(context), CX_reg(context), \
             DX_reg(context), SI_reg(context), DI_reg(context), \
             (WORD)DS_reg(context), (WORD)ES_reg(context) )

#endif /* __WINE_MISCEMU_H */
