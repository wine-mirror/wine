/*
 * Misc. emulation definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MISCEMU_H
#define __WINE_MISCEMU_H

#include <stdio.h>
#include "winnt.h"

  /* miscemu/dosmem.c */
extern BOOL32 DOSMEM_Init(void);
extern void DOSMEM_Tick(void);
extern void DOSMEM_FillBiosSegment(void);
extern void DOSMEM_InitMemoryHandling();
extern LPVOID DOSMEM_RealMode2Linear(DWORD);
extern WORD DOSMEM_AllocSelector(WORD);
extern HANDLE16 DOSMEM_BiosSeg;
extern char *DOSMEM_dosmem;
extern DWORD DOSMEM_CollateTable;

/* miscemu/interrupts.c */
extern BOOL32 INT_Init(void);

/* msdos/interrupts.c */
extern FARPROC16 INT_GetHandler( BYTE intnum );
extern void INT_SetHandler( BYTE intnum, FARPROC16 handler );

/* msdos/ioports.c */
extern DWORD IO_inport( int port, int count );
extern void IO_outport( int port, int count, DWORD value );

/* msdos/int1a.c */
extern DWORD INT1A_GetTicksSinceMidnight(void);

/* misc/cpu.c */
extern int runtime_cpu(void);

#define INT_BARF(context,num) \
    fprintf( stderr, "int%x: unknown/not implemented parameters:\n" \
                     "int%x: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (num), (num), AX_reg(context), BX_reg(context), CX_reg(context), \
             DX_reg(context), SI_reg(context), DI_reg(context), \
             (WORD)DS_reg(context), (WORD)ES_reg(context) )

#endif /* __WINE_MISCEMU_H */
