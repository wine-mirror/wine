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

typedef struct
{
    DWORD seg;  /* 0xffffffff means current default segment (cs or ds) */
    DWORD off;
} DBG_ADDR;

#define DBG_FIX_ADDR_SEG(addr,default) \
    { if ((addr)->seg == 0xffffffff) (addr)->seg = (default); \
      if (((addr)->seg == WINE_CODE_SELECTOR) || \
           (addr)->seg == WINE_DATA_SELECTOR) (addr)->seg = 0; }

#define DBG_ADDR_TO_LIN(addr) \
    ((addr)->seg ? (char *)PTR_SEG_OFF_TO_LIN((addr)->seg,(addr)->off) \
                 : (char *)(addr)->off)

#define DBG_CHECK_READ_PTR(addr,len) \
    (!DEBUG_IsBadReadPtr((addr),(len)) || \
     (fprintf(stderr,"*** Invalid address "), \
      DEBUG_PrintAddress((addr),dbg_mode), \
      fprintf(stderr,"\n"),0))

#define DBG_CHECK_WRITE_PTR(addr,len) \
    (!DEBUG_IsBadWritePtr((addr),(len)) || \
     (fprintf(stderr,"*** Invalid address "), \
      DEBUG_PrintAddress(addr,dbg_mode), \
      fprintf(stderr,"\n"),0))

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

extern struct sigcontext_struct *DEBUG_context;  /* debugger/registers.c */
extern unsigned int dbg_mode;

  /* debugger/break.c */
extern void DEBUG_SetBreakpoints( BOOL set );
extern int DEBUG_FindBreakpoint( const DBG_ADDR *addr );
extern void DEBUG_AddBreakpoint( const DBG_ADDR *addr );
extern void DEBUG_DelBreakpoint( int num );
extern void DEBUG_EnableBreakpoint( int num, BOOL enable );
extern void DEBUG_InfoBreakpoints(void);
extern BOOL DEBUG_HandleTrap( struct sigcontext_struct *context );
extern BOOL DEBUG_ShouldContinue( struct sigcontext_struct *context,
                                  enum exec_mode mode );
extern void DEBUG_RestartExecution( struct sigcontext_struct *context,
                                    enum exec_mode mode, int instr_len );

  /* debugger/db_disasm.c */
extern void DEBUG_Disasm( DBG_ADDR *addr );

  /* debugger/hash.c */
extern void DEBUG_AddSymbol( const char *name, const DBG_ADDR *addr );
extern BOOL DEBUG_GetSymbolValue( const char * name, DBG_ADDR *addr );
extern BOOL DEBUG_SetSymbolValue( const char * name, const DBG_ADDR *addr );
extern const char * DEBUG_FindNearestSymbol( const DBG_ADDR *addr );
extern void DEBUG_ReadSymbolTable( const char * filename );
extern void DEBUG_LoadEntryPoints(void);

  /* debugger/info.c */
extern void DEBUG_Print( const DBG_ADDR *addr, int count, char format );
extern void DEBUG_PrintAddress( const DBG_ADDR *addr, int addrlen );
extern void DEBUG_Help(void);
extern void DEBUG_List( DBG_ADDR *addr, int count );

  /* debugger/memory.c */
extern BOOL DEBUG_IsBadReadPtr( const DBG_ADDR *address, int size );
extern BOOL DEBUG_IsBadWritePtr( const DBG_ADDR *address, int size );
extern int DEBUG_ReadMemory( const DBG_ADDR *address );
extern void DEBUG_WriteMemory( const DBG_ADDR *address, int value );
extern void DEBUG_ExamineMemory( const DBG_ADDR *addr, int count, char format);

  /* debugger/registers.c */
extern void DEBUG_SetRegister( enum debug_regs reg, int val );
extern int DEBUG_GetRegister( enum debug_regs reg );
extern void DEBUG_InfoRegisters(void);

  /* debugger/stack.c */
extern void DEBUG_InfoStack(void);
extern void DEBUG_BackTrace(void);

  /* debugger/dbg.y */
extern void wine_debug( int signal, struct sigcontext_struct * regs );

#endif  /* DEBUGGER_H */
