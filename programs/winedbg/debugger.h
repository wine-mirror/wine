/*
 * Debugger definitions
 *
 * Copyright 1995 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DEBUGGER_H
#define __WINE_DEBUGGER_H

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ntstatus.h"
#include "wine/exception.h"

#ifdef __i386__
#define STEP_FLAG 0x00000100 /* single step flag */
#define V86_FLAG  0x00020000
#endif

#define SYM_FUNC	 0x0
#define SYM_DATA	 0x1
#define SYM_WIN32	 0x2
#define SYM_WINE	 0x4
#define SYM_INVALID	 0x8
#define SYM_TRAMPOLINE	 0x10
#define SYM_STEP_THROUGH 0x20

enum	debug_type {DT_BASIC, DT_POINTER, DT_ARRAY, DT_STRUCT, DT_ENUM,
		    DT_FUNC, DT_BITFIELD};

enum    debug_type_basic {DT_BASIC_INT = 1, DT_BASIC_CHAR, DT_BASIC_LONGINT, DT_BASIC_UINT,
                          DT_BASIC_ULONGINT, DT_BASIC_LONGLONGINT, DT_BASIC_ULONGLONGINT,
                          DT_BASIC_SHORTINT, DT_BASIC_USHORTINT, DT_BASIC_SCHAR, DT_BASIC_UCHAR,
                          DT_BASIC_FLOAT, DT_BASIC_LONGDOUBLE, DT_BASIC_DOUBLE,
                          DT_BASIC_CMPLX_INT, DT_BASIC_CMPLX_FLOAT, DT_BASIC_CMPLX_DOUBLE,
                          DT_BASIC_CMPLX_LONGDOUBLE, DT_BASIC_VOID,
                          /* modifier on size isn't possible on current types definitions
                           * so we need to add more types... */
                          DT_BASIC_BOOL1, DT_BASIC_BOOL2, DT_BASIC_BOOL4,
                          /* this is not really a basic type... */
                          DT_BASIC_STRING,
                          /* this is for historical reasons... should take care of it RSN */
                          DT_BASIC_CONST_INT,
                          /* not so basic, but handy */
                          DT_BASIC_CONTEXT,
                          /* to be kept as last... sentinel entry... do not use */
                          DT_BASIC_LAST};

/*
 * Return values for DEBUG_CheckLinenoStatus.  Used to determine
 * what to do when the 'step' command is given.
 */
#define FUNC_HAS_NO_LINES	(0)
#define NOT_ON_LINENUMBER	(1)
#define AT_LINENUMBER		(2)
#define FUNC_IS_TRAMPOLINE	(3)

typedef struct
{
    DWORD		seg;  /* 0xffffffff means current default segment (cs or ds) */
    DWORD 		off;
} DBG_ADDR;

typedef struct
{
   struct datatype*	type;
   int			cookie;	/* DV_??? */
/* DV_TARGET references an address in debugger's address space, whereas DV_HOST
 * references the debuggee address space
 */
#	define	DV_TARGET	0xF00D
#	define	DV_HOST		0x50DA
#	define	DV_INVALID      0x0000

   DBG_ADDR		addr;
} DBG_VALUE;

struct list_id
{
    char * sourcefile;
    int    line;
};

struct  wine_lines {
  unsigned long		line_number;
  DBG_ADDR		pc_offset;
};

struct symbol_info
{
  struct name_hash * sym;
  struct list_id     list;
};

typedef struct wine_lines WineLineNo;

/*
 * This structure holds information about stack variables, function
 * parameters, and register variables, which are all local to this
 * function.
 */
struct  wine_locals {
  unsigned int		regno:8;	/* For register symbols */
  signed int		offset:24;	/* offset from esp/ebp to symbol */
  unsigned int		pc_start;	/* For RBRAC/LBRAC */
  unsigned int		pc_end;		/* For RBRAC/LBRAC */
  char		      * name;		/* Name of symbol */
  struct datatype     * type;		/* Datatype of symbol */
};

typedef struct wine_locals WineLocals;

enum exec_mode
{
    EXEC_CONT,       		/* Continuous execution */
    EXEC_STEP_OVER,  		/* Stepping over a call to next source line */
    EXEC_STEP_INSTR,  		/* Step to next source line, stepping in if needed */
    EXEC_STEPI_OVER,  		/* Stepping over a call */
    EXEC_STEPI_INSTR,  		/* Single-stepping an instruction */
    EXEC_FINISH,		/* Step until we exit current frame */
    EXEC_STEP_OVER_TRAMPOLINE, 	/* Step over trampoline.  Requires that
				 * we dig the real return value off the stack
				 * and set breakpoint there - not at the
				 * instr just after the call.
				 */
};

enum dbg_mode
{
    MODE_INVALID, MODE_16, MODE_32, MODE_VM86
};

enum exit_mode /* of exception handling */
{
    EXIT_CONTINUE,              /* continue execution */
    EXIT_PASS,                  /* pass exception back to app (1st chance) */
    EXIT_DETACH,                /* detach debugger */
    EXIT_QUIT,                  /* exit debugger and kill debuggee */
};

#define	DBG_BREAK 	0
#define	DBG_WATCH 	1

typedef struct
{
    DBG_ADDR      addr;
    WORD	  enabled : 1,
                  type : 1,
                  is32 : 1,
                  refcount : 13;
    WORD	  skipcount;
    union {
       struct {
	  BYTE          opcode;
	  BOOL		(*func)(void);
       } b;
       struct {
	  BYTE		rw : 1,
	                len : 2;
	  BYTE		reg;
	  DWORD		oldval;
       } w;
    } u;
    struct expr * condition;
} DBG_BREAKPOINT;

/* Wine extension; Windows doesn't have a name for this code.  This is an
   undocumented exception understood by MS VC debugger, allowing the program
   to name a particular thread.  Search google.com or deja.com for "0x406d1388"
   for more info. */
#define EXCEPTION_NAME_THREAD               0x406D1388

/* Helper structure */
typedef struct tagTHREADNAME_INFO
{
   DWORD   dwType;     /* Must be 0x1000 */
   LPCTSTR szName;     /* Pointer to name - limited to 9 bytes (8 characters + terminator) */
   DWORD   dwThreadID; /* Thread ID (-1 = caller thread) */
   DWORD   dwFlags;    /* Reserved for future use.  Must be zero. */
} THREADNAME_INFO;

typedef struct tagDBG_THREAD {
    struct tagDBG_PROCESS*	process;
    HANDLE			handle;
    DWORD			tid;
    LPVOID			start;
    LPVOID			teb;
    int				wait_for_first_exception;
    enum exec_mode              exec_mode;      /* mode the thread is run (step/run...) */
    int			        exec_count;     /* count of mode operations */
    enum dbg_mode	        dbg_mode;       /* mode (VM86, 32bit, 16bit) */
    DBG_BREAKPOINT		stepOverBP;
    char                        name[9];
    struct tagDBG_THREAD* 	next;
    struct tagDBG_THREAD* 	prev;
} DBG_THREAD;

typedef struct tagDBG_DELAYED_BP {
    BOOL                        is_symbol;
    union {
        struct {
            int				lineno;
            char*			name;
        } symbol;
        DBG_VALUE                       value;
    } u;
} DBG_DELAYED_BP;

typedef struct tagDBG_PROCESS {
    HANDLE			handle;
    DWORD			pid;
    const char*			imageName;
    DBG_THREAD*			threads;
    int				num_threads;
    unsigned			continue_on_first_exception;
    struct tagDBG_MODULE**	modules;
    int				num_modules;
    unsigned long		dbg_hdr_addr;
    DBG_DELAYED_BP*		delayed_bp;
    int				num_delayed_bp;
    /*
     * This is an index we use to keep track of the debug information
     * when we have multiple sources. We use the same database to also
     * allow us to do an 'info shared' type of deal, and we use the index
     * to eliminate duplicates.
     */
    int				next_index;
    struct tagDBG_PROCESS*	next;
    struct tagDBG_PROCESS*	prev;
} DBG_PROCESS;

extern	DBG_PROCESS*	DEBUG_CurrProcess;
extern	DBG_THREAD*	DEBUG_CurrThread;
extern	DWORD		DEBUG_CurrTid;
extern	DWORD		DEBUG_CurrPid;
extern  CONTEXT 	DEBUG_context;
extern  BOOL		DEBUG_InteractiveP;
extern  enum exit_mode  DEBUG_ExitMode;
extern  HANDLE          DEBUG_hParserInput;
extern  HANDLE          DEBUG_hParserOutput;

#define DEBUG_READ_MEM(addr, buf, len) \
      (ReadProcessMemory(DEBUG_CurrProcess->handle, (addr), (buf), (len), NULL))

#define DEBUG_WRITE_MEM(addr, buf, len) \
      (WriteProcessMemory(DEBUG_CurrProcess->handle, (addr), (buf), (len), NULL))

#define DEBUG_READ_MEM_VERBOSE(addr, buf, len) \
      (DEBUG_READ_MEM((addr), (buf), (len)) || (DEBUG_InvalLinAddr( addr ),0))

#define DEBUG_WRITE_MEM_VERBOSE(addr, buf, len) \
      (DEBUG_WRITE_MEM((addr), (buf), (len)) || (DEBUG_InvalLinAddr( addr ),0))

enum DbgInfoLoad {DIL_DEFERRED, DIL_LOADED, DIL_NOINFO, DIL_ERROR};
enum DbgModuleType {DMT_UNKNOWN, DMT_ELF, DMT_NE, DMT_PE};

typedef struct tagDBG_MODULE {
   void*			load_addr;
   unsigned long		size;
   char*			module_name;
   enum DbgInfoLoad		dil;
   enum DbgModuleType		type;
   unsigned short		main : 1;
   short int			dbg_index;
   HMODULE                      handle;
   struct tagMSC_DBG_INFO*	msc_info;
   struct tagELF_DBG_INFO*	elf_info;
} DBG_MODULE;

typedef struct {
   DWORD		val;
   const char*		name;
   LPDWORD		pval;
   struct datatype*	type;
} DBG_INTVAR;

#define	OFFSET_OF(__c,__f)		((int)(((char*)&(((__c*)0)->__f))-((char*)0)))

enum get_sym_val {gsv_found, gsv_unknown, gsv_aborted};

  /* from winelib.so */
extern void DEBUG_ExternalDebugger(void);

  /* debugger/break.c */
extern void DEBUG_SetBreakpoints( BOOL set );
extern BOOL DEBUG_AddBreakpoint( const DBG_VALUE *addr, BOOL (*func)(void), BOOL verbose );
extern BOOL DEBUG_AddBreakpointFromValue( const DBG_VALUE *addr );
extern void DEBUG_AddBreakpointFromId( const char *name, int lineno );
extern void DEBUG_AddBreakpointFromLineno( int lineno );
extern void DEBUG_AddWatchpoint( const DBG_VALUE *addr, int is_write );
extern void DEBUG_AddWatchpointFromId( const char *name );
extern void DEBUG_CheckDelayedBP( void );
extern void DEBUG_DelBreakpoint( int num );
extern void DEBUG_EnableBreakpoint( int num, BOOL enable );
extern void DEBUG_InfoBreakpoints(void);
extern BOOL DEBUG_HandleTrap(void);
extern BOOL DEBUG_ShouldContinue( DBG_ADDR* addr, DWORD code, int* count );
extern void DEBUG_SuspendExecution( void );
extern void DEBUG_RestartExecution( int count );
extern BOOL DEBUG_IsFctReturn(void);
extern int  DEBUG_AddBPCondition(int bpnum, struct expr * exp);

  /* debugger/db_disasm.c */
extern void DEBUG_Disasm( DBG_ADDR *addr, int display );

  /* debugger/dbg.y */
extern void DEBUG_Parser(LPCSTR);
extern void DEBUG_Exit(DWORD);

  /* debugger/debug.l */
extern void DEBUG_FlushSymbols(void);
extern char*DEBUG_MakeSymbol(const char*);
extern int  DEBUG_ReadLine(const char* pfx, char* buffer, int size);

  /* debugger/display.c */
extern int DEBUG_DoDisplay(void);
extern int DEBUG_AddDisplay(struct expr * exp, int count, char format, int local_frame);
extern int DEBUG_DelDisplay(int displaynum);
extern int DEBUG_InfoDisplay(void);
extern int DEBUG_EnableDisplay(int displaynum, int enable);

  /* debugger/expr.c */
extern void DEBUG_FreeExprMem(void);
struct expr * DEBUG_IntVarExpr(const char* name);
struct expr * DEBUG_SymbolExpr(const char * name);
struct expr * DEBUG_ConstExpr(int val);
struct expr * DEBUG_StringExpr(const char * str);
struct expr * DEBUG_SegAddr(struct expr *, struct expr *);
struct expr * DEBUG_USConstExpr(unsigned int val);
struct expr * DEBUG_BinopExpr(int oper, struct expr *, struct expr *);
struct expr * DEBUG_UnopExpr(int oper, struct expr *);
struct expr * DEBUG_StructPExpr(struct expr *, const char * element);
struct expr * DEBUG_StructExpr(struct expr *, const char * element);
struct expr * DEBUG_ArrayExpr(struct expr *, struct expr * index);
struct expr * DEBUG_CallExpr(const char *, int nargs, ...);
struct expr * DEBUG_TypeCastExpr(struct datatype *, struct expr *);
extern DBG_VALUE DEBUG_EvalExpr(struct expr *);
extern int DEBUG_DelDisplay(int displaynum);
extern struct expr * DEBUG_CloneExpr(const struct expr * exp);
extern int DEBUG_FreeExpr(struct expr * exp);
extern int DEBUG_DisplayExpr(const struct expr * exp);

  /* debugger/hash.c */
extern struct name_hash * DEBUG_AddSymbol( const char *name,
					   const DBG_VALUE *addr,
					   const char *sourcefile,
					   int flags);
extern enum get_sym_val DEBUG_GetSymbolValue( const char * name, const int lineno, DBG_VALUE *addr, int );
extern const char * DEBUG_FindNearestSymbol( const DBG_ADDR *addr, int flag,
					     struct name_hash ** rtn,
					     unsigned int ebp,
					     struct list_id * source);
extern void DEBUG_ReadSymbolTable( const char * filename, unsigned long offset );
extern void DEBUG_AddLineNumber( struct name_hash * func, int line_num,
				 unsigned long offset );
extern struct wine_locals *
            DEBUG_AddLocal( struct name_hash * func, int regno,
			    int offset,
			    int pc_start,
			    int pc_end,
			    char * name);
extern int DEBUG_CheckLinenoStatus(const DBG_ADDR *addr);
extern void DEBUG_GetFuncInfo(struct list_id * ret, const char * file,
			      const char * func);
extern int DEBUG_SetSymbolSize(struct name_hash * sym, unsigned int len);
extern int DEBUG_SetSymbolBPOff(struct name_hash * sym, unsigned int len);
extern int DEBUG_GetSymbolAddr(struct name_hash * sym, DBG_ADDR * addr);
extern int DEBUG_cmp_sym(const void * p1, const void * p2);
extern BOOL DEBUG_GetLineNumberAddr( const struct name_hash *, const int lineno,
				     DBG_ADDR *addr, int bp_flag );

extern int DEBUG_SetLocalSymbolType(struct wine_locals * sym,
				    struct datatype * type);
extern BOOL DEBUG_Normalize(struct name_hash * nh );
void DEBUG_InfoSymbols(const char* str);
const char *DEBUG_GetSymbolName(const struct name_hash *);

  /* debugger/info.c */
extern void DEBUG_PrintBasic( const DBG_VALUE* value, int count, char format );
extern struct symbol_info DEBUG_PrintAddress( const DBG_ADDR *addr,
					      enum dbg_mode mode, int flag );
extern void DEBUG_Help(void);
extern void DEBUG_HelpInfo(void);
extern struct symbol_info DEBUG_PrintAddressAndArgs( const DBG_ADDR *addr,
						     enum dbg_mode mode,
						     unsigned int ebp,
						     int flag );
extern void DEBUG_InfoClass(const char* clsName);
extern void DEBUG_WalkClasses(void);
extern void DEBUG_DumpModule(DWORD mod);
extern void DEBUG_WalkModules(void);
extern void DEBUG_WalkProcess(void);
extern void DEBUG_WalkThreads(void);
extern void DEBUG_WalkExceptions(DWORD tid);
extern void DEBUG_InfoSegments(DWORD s, int v);
extern void DEBUG_InfoVirtual(DWORD pid);
extern void DEBUG_InfoWindow(HWND hWnd);
extern void DEBUG_WalkWindows(HWND hWnd, int indent);
extern void DEBUG_DbgChannel(BOOL add, const char* chnl, const char* name);

  /* debugger/memory.c */
extern int  DEBUG_ReadMemory( const DBG_VALUE* value );
extern void DEBUG_WriteMemory( const DBG_VALUE* val, int value );
extern void DEBUG_ExamineMemory( const DBG_VALUE *addr, int count, char format);
extern void DEBUG_InvalAddr( const DBG_ADDR* addr );
extern void DEBUG_InvalLinAddr( void* addr );
extern DWORD DEBUG_ToLinear( const DBG_ADDR *address );
extern void DEBUG_GetCurrentAddress( DBG_ADDR * );
extern BOOL DEBUG_GrabAddress( DBG_VALUE* value, BOOL fromCode );
extern enum dbg_mode DEBUG_GetSelectorType( WORD sel );
#ifdef __i386__
extern void DEBUG_FixAddress( DBG_ADDR *address, DWORD def );
extern int  DEBUG_IsSelectorSystem( WORD sel );
#endif
extern int  DEBUG_PrintStringA( int chnl, const DBG_ADDR* address, int len );
extern int  DEBUG_PrintStringW( int chnl, const DBG_ADDR* address, int len );

  /* debugger/module.c */
extern int  DEBUG_LoadEntryPoints( const char * prefix );
extern void DEBUG_LoadModule32( const char* name, HANDLE hFile, void *base );
extern DBG_MODULE* DEBUG_FindModuleByName(const char* name, enum DbgModuleType type);
extern DBG_MODULE* DEBUG_FindModuleByHandle(HANDLE handle, enum DbgModuleType type);
extern DBG_MODULE* DEBUG_FindModuleByAddr(void* addr, enum DbgModuleType type);
extern DBG_MODULE* DEBUG_GetProcessMainModule(DBG_PROCESS* process);
extern DBG_MODULE* DEBUG_RegisterELFModule(void *load_addr, unsigned long size,
					   const char* name);
extern enum DbgInfoLoad DEBUG_RegisterPEDebugInfo(DBG_MODULE* wmod, HANDLE hFile,
						  void* _nth, unsigned long nth_ofs);
extern void DEBUG_ReportDIL(enum DbgInfoLoad dil, const char* pfx,
			    const char* filename, void *load_addr);
extern void DEBUG_InfoShare(void);

  /* debugger/msc.c */
extern enum DbgInfoLoad DEBUG_RegisterMSCDebugInfo(DBG_MODULE* module, HANDLE hFile,
						   void* nth, unsigned long nth_ofs);
extern enum DbgInfoLoad DEBUG_RegisterStabsDebugInfo(DBG_MODULE* module,
						     HANDLE hFile, void* nth,
						     unsigned long nth_ofs);
extern void DEBUG_InitCVDataTypes(void);

  /* debugger/registers.c */
extern void DEBUG_InfoRegisters(const CONTEXT* ctx);
extern BOOL DEBUG_ValidateRegisters(void);

  /* debugger/source.c */
extern void DEBUG_ShowDir(void);
extern void DEBUG_AddPath(const char * path);
extern void DEBUG_List(struct list_id * line1, struct list_id * line2,
		       int delta);
extern void DEBUG_NukePath(void);
extern void DEBUG_Disassemble(const DBG_VALUE *, const DBG_VALUE*, int offset);
extern BOOL DEBUG_DisassembleInstruction(DBG_ADDR *addr);

  /* debugger/stack.c */
extern void DEBUG_InfoStack(void);
extern void DEBUG_BackTrace(DWORD threadID, BOOL noisy);
extern int  DEBUG_InfoLocals(void);
extern int  DEBUG_SetFrame(int newframe);
extern int  DEBUG_GetCurrentFrame(struct name_hash ** name,
				  unsigned int * eip,
				  unsigned int * ebp);

  /* debugger/stabs.c */
extern enum DbgInfoLoad DEBUG_ReadExecutableDbgInfo(const char* exe_name);
extern enum DbgInfoLoad DEBUG_ParseStabs(char * addr, void *load_offset,
					 unsigned int staboff, int stablen,
					 unsigned int strtaboff, int strtablen);

  /* debugger/types.c */
extern int DEBUG_nchar;
extern void DEBUG_InitTypes(void);
extern struct datatype * DEBUG_NewDataType(enum debug_type xtype,
					   const char * typename);
extern unsigned int DEBUG_TypeDerefPointer(const DBG_VALUE *value,
					   struct datatype ** newtype);
extern int DEBUG_AddStructElement(struct datatype * dt,
				  char * name, struct datatype * type,
				  int offset, int size);
extern int DEBUG_SetStructSize(struct datatype * dt, int size);
extern int DEBUG_SetPointerType(struct datatype * dt, struct datatype * dt2);
extern int DEBUG_SetArrayParams(struct datatype * dt, int min, int max,
				struct datatype * dt2);
extern void DEBUG_Print( const DBG_VALUE *addr, int count, char format, int level );
extern unsigned int DEBUG_FindStructElement(DBG_VALUE * addr,
					    const char * ele_name, int * tmpbuf);
extern struct datatype * DEBUG_GetPointerType(struct datatype * dt);
extern int DEBUG_GetObjectSize(struct datatype * dt);
extern unsigned int DEBUG_ArrayIndex(const DBG_VALUE * addr, DBG_VALUE * result,
				     int index);
extern struct datatype * DEBUG_FindOrMakePointerType(struct datatype * reftype);
extern long long int DEBUG_GetExprValue(const DBG_VALUE * addr, const char ** format);
extern int DEBUG_SetBitfieldParams(struct datatype * dt, int offset,
				   int nbits, struct datatype * dt2);
extern int DEBUG_CopyFieldlist(struct datatype * dt, struct datatype * dt2);
extern const char* DEBUG_GetName(struct datatype * dt);
extern enum debug_type DEBUG_GetType(struct datatype * dt);
extern struct datatype * DEBUG_TypeCast(enum debug_type, const char *);
extern int DEBUG_PrintTypeCast(const struct datatype *);
extern int DEBUG_PrintType( const DBG_VALUE* addr );
extern struct datatype * DEBUG_GetBasicType(enum debug_type_basic);
extern int DEBUG_DumpTypes(void);

  /* debugger/winedbg.c */
#define DBG_CHN_MESG	1
#define DBG_CHN_ERR	2
#define DBG_CHN_WARN	4
#define DBG_CHN_FIXME	8
#define DBG_CHN_TRACE	16
extern void	        DEBUG_OutputA(int chn, const char* buffer, int len);
extern void	        DEBUG_OutputW(int chn, const WCHAR* buffer, int len);
#ifdef __GNUC__
extern int	        DEBUG_Printf(int chn, const char* format, ...) __attribute__((format (printf,2,3)));
#else
extern int	        DEBUG_Printf(int chn, const char* format, ...);
#endif
extern DBG_INTVAR*	DEBUG_GetIntVar(const char*);
extern BOOL             DEBUG_Attach(DWORD pid, BOOL cofe, BOOL wfe);
extern BOOL             DEBUG_Detach(void);
extern void             DEBUG_Run(const char* args);
extern DBG_PROCESS*	DEBUG_AddProcess(DWORD pid, HANDLE h, const char* imageName);
extern DBG_PROCESS* 	DEBUG_GetProcess(DWORD pid);
extern void             DEBUG_DelProcess(DBG_PROCESS* p);
extern DBG_THREAD*	DEBUG_AddThread(DBG_PROCESS* p, DWORD tid, HANDLE h, LPVOID start, LPVOID teb);
extern DBG_THREAD* 	DEBUG_GetThread(DBG_PROCESS* p, DWORD tid);
extern void             DEBUG_DelThread(DBG_THREAD* t);
extern BOOL             DEBUG_ProcessGetString(char* buffer, int size, HANDLE hp, LPSTR addr);
extern BOOL             DEBUG_ProcessGetStringIndirect(char* buffer, int size, HANDLE hp, LPVOID addr, BOOL unicode);
extern void             DEBUG_WaitNextException(DWORD cont, int count, int mode);
extern BOOL             DEBUG_InterruptDebuggee(void);
extern int curr_frame;

/* gdbproxy.c */
extern BOOL DEBUG_GdbRemote(unsigned int);

/* Choose your allocator! */
#if 1
/* this one is libc's fast one */
extern void*	DEBUG_XMalloc(size_t size);
extern void*	DEBUG_XReAlloc(void *ptr, size_t size);
extern char*	DEBUG_XStrDup(const char *str);

#define DBG_alloc(x)		DEBUG_XMalloc(x)
#define DBG_realloc(x,y) 	DEBUG_XReAlloc(x,y)
#define DBG_free(x) 		free(x)
#define DBG_strdup(x) 		DEBUG_XStrDup(x)
#else
/* this one is slow (takes 5 minutes to load the debugger on my machine),
   if someone could make optimized routines so it wouldn't
   take so long to load, it could be made default) */
#define DBG_alloc(x) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,x)
#define DBG_realloc(x,y) HeapReAlloc(GetProcessHeap(),0,x,y)
#define DBG_free(x) HeapFree(GetProcessHeap(),0,x)
inline static LPSTR DBG_strdup( LPCSTR str )
{
    INT len = strlen(str) + 1;
    LPSTR p = DBG_alloc( len );
    if (p) memcpy( p, str, len );
    return p;
}
#endif

#define	DEBUG_STATUS_OFFSET		0x80003000
#define	DEBUG_STATUS_INTERNAL_ERROR	(DEBUG_STATUS_OFFSET+0)
#define	DEBUG_STATUS_NO_SYMBOL		(DEBUG_STATUS_OFFSET+1)
#define	DEBUG_STATUS_DIV_BY_ZERO	(DEBUG_STATUS_OFFSET+2)
#define	DEBUG_STATUS_BAD_TYPE		(DEBUG_STATUS_OFFSET+3)
#define DEBUG_STATUS_NO_FIELD		(DEBUG_STATUS_OFFSET+4)
#define DEBUG_STATUS_ABORT              (DEBUG_STATUS_OFFSET+5)

extern DBG_INTVAR		DEBUG_IntVars[];

#define  DBG_IVARNAME(_var)	DEBUG_IV_##_var
#define  DBG_IVARSTRUCT(_var)	DEBUG_IntVars[DBG_IVARNAME(_var)]
#define  DBG_IVAR(_var)		(*(DBG_IVARSTRUCT(_var).pval))
#define  INTERNAL_VAR(_var,_val,_ref,_typ) DBG_IVARNAME(_var),
enum debug_int_var {
#include "intvar.h"
   DBG_IV_LAST
};
#undef   INTERNAL_VAR

#endif  /* __WINE_DEBUGGER_H */
