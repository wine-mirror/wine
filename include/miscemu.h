/*
 * Misc. emulation definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_MISCEMU_H
#define __WINE_MISCEMU_H

#include "wintypes.h"
#include "registers.h"

  /* miscemu/dosmem.c */
extern BOOL DOSMEM_Init(void);
extern void DOSMEM_Tick(void);
extern void DOSMEM_FillBiosSegment(void);
extern HANDLE DOSMEM_BiosSeg;

  /* miscemu/instr.c */
extern BOOL INSTR_EmulateInstruction( struct sigcontext_struct *context );

  /* miscemu/interrupts.c */
extern BOOL INT_Init(void);
extern SEGPTR INT_GetHandler( BYTE intnum );
extern void INT_SetHandler( BYTE intnum, SEGPTR handler );

  /* miscemu/int1a.c */
extern DWORD INT1A_GetTicksSinceMidnight(void);

  /* miscemu/int21.c */
extern BOOL INT21_Init(void);

  /* miscemu/ioports.c */
extern DWORD inport( int port, int count );
extern void outport( int port, int count, DWORD value );


#define INT_BARF(context,num) \
    fprintf( stderr, "int%x: unknown/not implemented parameters:\n" \
                     "int%x: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (num), (num), AX_reg(context), BX_reg(context), CX_reg(context), \
             DX_reg(context), SI_reg(context), DI_reg(context), \
             DS_reg(context), ES_reg(context) )

#endif /* __WINE_MISCEMU_H */
