/*
 * Debugger definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_DEBUGGER_H
#define __WINE_DEBUGGER_H

#include "ldt.h"
#include "registers.h"

#include "pe_image.h"

#define STEP_FLAG 0x100 /* single step flag */

typedef struct
{
    DWORD seg;  /* 0xffffffff means current default segment (cs or ds) */
    DWORD off;
} DBG_ADDR;

struct  wine_lines {
  unsigned long		line_number;
  DBG_ADDR		pc_offset;
};

typedef struct wine_lines WineLineNo;

/*
 * This structure holds information about stack variables, function
 * parameters, and register variables, which are all local to this
 * function.
 */
struct  wine_locals {
  unsigned int		regno:8;	/* For register symbols */
  unsigned int		offset:24;	/* offset from esp/ebp to symbol */
  unsigned int		pc_start;	/* For RBRAC/LBRAC */
  unsigned int		pc_end;		/* For RBRAC/LBRAC */
  char		      * name;		/* Name of symbol */
};

typedef struct wine_locals WineLocals;

struct name_hash
{
    struct name_hash * next;
    char *             name;
    char *             sourcefile;

    int		       n_locals;
    int		       locals_alloc;
    WineLocals       * local_vars;
  
    int		       n_lines;
    int		       lines_alloc;
    WineLineNo       * linetab;

    DBG_ADDR           addr;
};

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
      DEBUG_PrintAddress((addr),dbg_mode, FALSE), \
      fprintf(stderr,"\n"),0))

#define DBG_CHECK_WRITE_PTR(addr,len) \
    (!DEBUG_IsBadWritePtr((addr),(len)) || \
     (fprintf(stderr,"*** Invalid address "), \
      DEBUG_PrintAddress(addr,dbg_mode, FALSE), \
      fprintf(stderr,"\n"),0))

#ifdef REG_SP  /* Some Sun includes define this */
#undef REG_SP
#endif

enum debug_regs
{
    REG_EAX, REG_EBX, REG_ECX, REG_EDX, REG_ESI,
    REG_EDI, REG_EBP, REG_EFL, REG_EIP, REG_ESP,
    REG_AX, REG_BX, REG_CX, REG_DX, REG_SI,
    REG_DI, REG_BP, REG_FL, REG_IP, REG_SP,
    REG_CS, REG_DS, REG_ES, REG_SS, REG_FS, REG_GS
};


enum exec_mode
{
    EXEC_CONT,       /* Continuous execution */
    EXEC_STEP_OVER,  /* Stepping over a call */
    EXEC_STEP_INSTR  /* Single-stepping an instruction */
};

extern SIGCONTEXT *DEBUG_context;  /* debugger/registers.c */
extern unsigned int dbg_mode;

  /* debugger/break.c */
extern void DEBUG_SetBreakpoints( BOOL32 set );
extern int DEBUG_FindBreakpoint( const DBG_ADDR *addr );
extern void DEBUG_AddBreakpoint( const DBG_ADDR *addr );
extern void DEBUG_DelBreakpoint( int num );
extern void DEBUG_EnableBreakpoint( int num, BOOL32 enable );
extern void DEBUG_InfoBreakpoints(void);
extern BOOL32 DEBUG_HandleTrap( SIGCONTEXT *context );
extern BOOL32 DEBUG_ShouldContinue( SIGCONTEXT *context, enum exec_mode mode );
extern void DEBUG_RestartExecution( SIGCONTEXT *context, enum exec_mode mode,
                                    int instr_len );

  /* debugger/db_disasm.c */
extern void DEBUG_Disasm( DBG_ADDR *addr );

  /* debugger/hash.c */
extern struct name_hash * DEBUG_AddSymbol( const char *name, 
					   const DBG_ADDR *addr,
					   const char * sourcefile);
extern BOOL32 DEBUG_GetSymbolValue( const char * name, const int lineno,
				    DBG_ADDR *addr );
extern BOOL32 DEBUG_SetSymbolValue( const char * name, const DBG_ADDR *addr );
extern const char * DEBUG_FindNearestSymbol( const DBG_ADDR *addr, int flag,
					     struct name_hash ** rtn,
					     unsigned int ebp);
extern void DEBUG_ReadSymbolTable( const char * filename );
extern void DEBUG_LoadEntryPoints(void);
extern void DEBUG_AddLineNumber( struct name_hash * func, int line_num, 
		     unsigned long offset );
extern void DEBUG_AddLocal( struct name_hash * func, int regno, 
			    int offset,
			    int pc_start,
			    int pc_end,
			    char * name);


  /* debugger/info.c */
extern void DEBUG_Print( const DBG_ADDR *addr, int count, char format );
extern struct name_hash * DEBUG_PrintAddress( const DBG_ADDR *addr, 
					      int addrlen, int flag );
extern void DEBUG_Help(void);
extern void DEBUG_List( DBG_ADDR *addr, int count );
extern struct name_hash * DEBUG_PrintAddressAndArgs( const DBG_ADDR *addr, 
						     int addrlen, 
						     unsigned int ebp, 
						     int flag );

  /* debugger/memory.c */
extern BOOL32 DEBUG_IsBadReadPtr( const DBG_ADDR *address, int size );
extern BOOL32 DEBUG_IsBadWritePtr( const DBG_ADDR *address, int size );
extern int DEBUG_ReadMemory( const DBG_ADDR *address );
extern void DEBUG_WriteMemory( const DBG_ADDR *address, int value );
extern void DEBUG_ExamineMemory( const DBG_ADDR *addr, int count, char format);

  /* debugger/registers.c */
extern void DEBUG_SetRegister( enum debug_regs reg, int val );
extern int DEBUG_GetRegister( enum debug_regs reg );
extern void DEBUG_InfoRegisters(void);
extern BOOL32 DEBUG_ValidateRegisters(void);

  /* debugger/stack.c */
extern void DEBUG_InfoStack(void);
extern void DEBUG_BackTrace(void);
extern BOOL32 DEBUG_GetStackSymbolValue( const char * name, DBG_ADDR *addr );
extern int  DEBUG_InfoLocals(void);
extern int  DEBUG_SetFrame(int newframe);

  /* debugger/stabs.c */
extern int DEBUG_ReadExecutableDbgInfo(void);

  /* debugger/msc.c */
extern int DEBUG_RegisterDebugInfo(int, struct pe_data *pe,
				   int, unsigned long, unsigned long);
extern int DEBUG_ProcessDeferredDebug(void);

  /* debugger/dbg.y */
extern void DEBUG_EnterDebugger(void);
extern void wine_debug( int signal, SIGCONTEXT *regs );

#endif  /* __WINE_DEBUGGER_H */
