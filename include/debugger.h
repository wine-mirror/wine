/*
 * Debugger definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "ldt.h"
#include "registers.h"
#include "wine.h"


enum debug_regs
{
    REG_EAX, REG_EBX, REG_ECX, REG_EDX, REG_ESI,
    REG_EDI, REG_EBP, REG_EFL, REG_EIP, REG_ESP,
    REG_AX, REG_BX, REG_CX, REG_DX, REG_SI,
    REG_DI, REG_BP, REG_FL, REG_IP, REG_SP,
    REG_CS, REG_DS, REG_ES, REG_SS
};


enum exec_mode
{
    EXEC_CONT,       /* Continuous execution */
    EXEC_STEP_OVER,  /* Stepping over a call */
    EXEC_STEP_INSTR  /* Single-stepping an instruction */
};

extern struct sigcontext_struct *context;  /* debugger/registers.c */
extern unsigned int dbg_mode;

  /* debugger/break.c */
extern void DEBUG_SetBreakpoints( BOOL set );
extern int DEBUG_FindBreakpoint( unsigned int segment, unsigned int addr );
extern void DEBUG_AddBreakpoint( unsigned int segment, unsigned int addr );
extern void DEBUG_DelBreakpoint( int num );
extern void DEBUG_EnableBreakpoint( int num, BOOL enable );
extern void DEBUG_InfoBreakpoints(void);
extern BOOL DEBUG_HandleTrap( struct sigcontext_struct *context );
extern BOOL DEBUG_ShouldContinue( struct sigcontext_struct *context,
                                  enum exec_mode mode );
extern void DEBUG_RestartExecution( struct sigcontext_struct *context,
                                    enum exec_mode mode, int instr_len );

  /* debugger/registers.c */
extern void DEBUG_SetRegister( enum debug_regs reg, int val );
extern int DEBUG_GetRegister( enum debug_regs reg );
extern void DEBUG_InfoRegisters(void);

  /* debugger/stack.c */
extern void DEBUG_InfoStack(void);
extern void DEBUG_BackTrace(void);

  /* debugger/dbg.y */
extern void wine_debug( int signal, struct sigcontext_struct * regs );

extern void print_address( unsigned int seg, unsigned int addr, int addrlen );
extern unsigned int db_disasm( unsigned int segment, unsigned int loc );

#endif  /* DEBUGGER_H */
