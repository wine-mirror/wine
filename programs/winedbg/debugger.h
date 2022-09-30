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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DEBUGGER_H
#define __WINE_DEBUGGER_H

#include <assert.h>
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN
#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "winternl.h"
#include "dbghelp.h"
#include "cvconst.h"
#include "objbase.h"
#include "oaidl.h"
#include <wine/list.h>

#define ADDRSIZE        ((int)(dbg_curr_process ? dbg_curr_process->be_cpu->pointer_size : sizeof(void*)))
#define ADDRWIDTH       (ADDRSIZE * 2)

/* the debugger uses these exceptions for its internal use */
#define	DEBUG_STATUS_OFFSET		0x80003000
#define	DEBUG_STATUS_INTERNAL_ERROR	(DEBUG_STATUS_OFFSET+0) /* something went wrong */
#define	DEBUG_STATUS_NO_SYMBOL		(DEBUG_STATUS_OFFSET+1) /* no symbol found in lookup */
#define	DEBUG_STATUS_DIV_BY_ZERO	(DEBUG_STATUS_OFFSET+2)
#define	DEBUG_STATUS_BAD_TYPE		(DEBUG_STATUS_OFFSET+3) /* no type found, when type was expected */
#define DEBUG_STATUS_NO_FIELD		(DEBUG_STATUS_OFFSET+4) /* when dereferencing a struct, the field was not found */
#define DEBUG_STATUS_ABORT              (DEBUG_STATUS_OFFSET+5) /* user aborted on going action */
#define DEBUG_STATUS_CANT_DEREF         (DEBUG_STATUS_OFFSET+6) /* either not deref:able, or index out of bounds */
#define DEBUG_STATUS_NOT_AN_INTEGER     (DEBUG_STATUS_OFFSET+7) /* requiring an integral value */

/*
 * Return values for symbol_get_function_line_status.  Used to determine
 * what to do when the 'step' command is given.
 */
enum dbg_line_status
{
    dbg_no_line_info,
    dbg_not_on_a_line_number,
    dbg_on_a_line_number,
    dbg_in_a_thunk,
};

enum dbg_internal_types
{
    /* types that we synthetize inside the debugger */
    dbg_itype_synthetized       = 0xf0000000,
    /* order here must match types.c:basic_types_details table */
    dbg_itype_first             = 0xffffff00,
    dbg_itype_void              = dbg_itype_first,
    dbg_itype_bool,
    dbg_itype_char,
    dbg_itype_wchar,
    dbg_itype_char8,
    dbg_itype_char16,
    dbg_itype_char32,

    dbg_itype_unsigned_int8,
    dbg_itype_unsigned_int16,
    dbg_itype_unsigned_int32,
    dbg_itype_unsigned_int64,
    dbg_itype_unsigned_int128,
    dbg_itype_unsigned_long32,
    dbg_itype_unsigned_long64,

    dbg_itype_signed_int8,
    dbg_itype_signed_int16,
    dbg_itype_signed_int32,
    dbg_itype_signed_int64,
    dbg_itype_signed_int128,
    dbg_itype_signed_long32,
    dbg_itype_signed_long64,

    dbg_itype_short_real, /* aka float */
    dbg_itype_real,       /* aka double */
    dbg_itype_long_real,  /* aka long double */

    dbg_itype_last,

    /* they represent the dbg_lg(u)int_t types */
    dbg_itype_lgint,
    dbg_itype_lguint,

    dbg_itype_astring,
    dbg_itype_ustring,
    dbg_itype_segptr,     /* hack for segmented pointers */
    dbg_itype_m128a,      /* 128-bit (XMM) registers */
    dbg_itype_none              = 0xffffffff
};

/* Largest integers the debugger's compiler can support.
 * It's large enough to store a pointer (in debuggee or debugger's address space).
 * It can be smaller than the largest integer(s) of the debuggee.
 * (eg. 64 bit on PE build of debugger, vs 128 int in ELF build of a library)
 */
typedef LONG64  dbg_lgint_t;
typedef ULONG64 dbg_lguint_t;

/* type description (in the following order):
 * - if 'id' is dbg_itype_none (whatever 'module' value), the type isn't known
 * - if 'module' is 0, it's an internal type (id is one of dbg_itype...)
 * - if 'module' is non 0, then 'id' is a type ID referring to module (loaded in
 *   dbghelp) which (linear) contains address 'module'.
 */
struct dbg_type
{
    ULONG               id;
    DWORD_PTR           module;
};

struct dbg_lvalue       /* structure to hold left-values... */
{
    unsigned            in_debuggee : 1, /* 1 = debuggee address space, 0 = debugger address space */
                        bitstart : 8, /* in fact, 7 should be sufficient for underlying 128bit integers */
                        bitlen;
    ADDRESS64           addr;
    struct dbg_type     type;
};

static inline void init_lvalue(struct dbg_lvalue* lv, BOOL in_debuggee, void* addr)
{
    lv->in_debuggee = !!in_debuggee;
    lv->bitstart = 0;
    lv->bitlen = 0;
    lv->addr.Mode = AddrModeFlat;
    lv->addr.Offset = (DWORD_PTR)addr;
    lv->type.module = 0;
    lv->type.id = dbg_itype_none;
}

static inline void init_lvalue_in_debugger(struct dbg_lvalue* lv, DWORD_PTR module,
                                           enum dbg_internal_types it, void* addr)
{
    lv->in_debuggee = 0;
    lv->bitstart = 0;
    lv->bitlen = 0;
    lv->addr.Mode = AddrModeFlat;
    lv->addr.Offset = (DWORD_PTR)addr;
    lv->type.module = module;
    lv->type.id = it;
}

enum dbg_exec_mode
{
    dbg_exec_cont,       		/* Continue execution */
    dbg_exec_step_over_line,  		/* Stepping over a call to next source line */
    dbg_exec_step_into_line,  		/* Step to next source line, stepping in if needed */
    dbg_exec_step_over_insn,  		/* Stepping over a call */
    dbg_exec_step_into_insn,  		/* Single-stepping an instruction */
    dbg_exec_finish,		        /* Single-step until we exit current frame */
#if 0
    EXEC_STEP_OVER_TRAMPOLINE, 	/* Step over trampoline.  Requires that we dig the real
                                 * return value off the stack and set breakpoint there - 
                                 * not at the instr just after the call.
				 */
#endif
};

struct dbg_breakpoint
{
    ADDRESS64           addr;
    unsigned int        enabled : 1,
                        xpoint_type : 2,
                        refcount : 13,
                        skipcount : 16;
    unsigned int        info;
    struct              /* only used for watchpoints */
    {
        BYTE		len : 2;
        DWORD64		oldval;
    } w;
    struct expr*        condition;
};

/* Helper structure */
typedef struct tagTHREADNAME_INFO
{
   DWORD   dwType;     /* Must be 0x1000 */
   LPCSTR  szName;     /* Pointer to name - limited to 9 bytes (8 characters + terminator) */
   DWORD   dwThreadID; /* Thread ID (-1 = caller thread) */
   DWORD   dwFlags;    /* Reserved for future use.  Must be zero. */
} THREADNAME_INFO;

typedef union dbg_ctx
{
    CONTEXT ctx;
    WOW64_CONTEXT x86;
} dbg_ctx_t;

struct dbg_thread
{
    struct list                 entry;
    struct dbg_process* 	process;
    HANDLE			handle;
    DWORD			tid;
    void*			teb;
    enum dbg_exec_mode          exec_mode;      /* mode the thread is run (step/run...) */
    int			        exec_count;     /* count of mode operations */
    ADDRESS_MODE	        addr_mode;      /* mode */
    int                         stopped_xpoint; /* xpoint on which the thread has stopped (-1 if none) */
    struct dbg_breakpoint	step_over_bp;
    char                        name[9];
    BOOL                        in_exception;   /* TRUE if thread stopped with an exception */
    BOOL                        first_chance;   /* TRUE if thread stopped with a first chance exception
                                                 *      - only valid when in_exception is TRUE
                                                 */
    EXCEPTION_RECORD            excpt_record;   /* only valid when in_exception is TRUE */
    struct dbg_frame
    {
        ADDRESS64               addr_pc;
        ADDRESS64               addr_frame;
        ADDRESS64               addr_stack;
        DWORD_PTR               linear_pc;
        DWORD_PTR               linear_frame;
        DWORD_PTR               linear_stack;
        dbg_ctx_t               context;        /* context we got out of stackwalk for this frame */
        DWORD                   inline_ctx;
        BOOL                    is_ctx_valid;   /* is the context above valid */
    }*                          frames;
    int                         num_frames;
    int                         curr_frame;
    BOOL                        suspended;
};

struct dbg_delayed_bp
{
    BOOL                        is_symbol;
    BOOL                        software_bp;
    union
    {
        struct
        {
            int				lineno;
            char*			name;
        } symbol;
        ADDRESS64               addr;
    } u;
};

#define MAX_BREAKPOINTS 100
struct dbg_process
{
    struct list                 entry;
    HANDLE			handle;
    DWORD			pid;
    const struct be_process_io* process_io;
    void*                       pio_data;
    const WCHAR*		imageName;
    struct list           	threads;
    struct backend_cpu*         be_cpu;
    HANDLE                      event_on_first_exception;
    BOOL                        active_debuggee;
    struct dbg_breakpoint       bp[MAX_BREAKPOINTS];
    unsigned                    next_bp;
    struct dbg_delayed_bp*      delayed_bp;
    int				num_delayed_bp;
    struct open_file_list*      source_ofiles;
    char*                       search_path;
    char                        source_current_file[MAX_PATH];
    int                         source_start_line;
    int                         source_end_line;
    const struct data_model*    data_model;
    struct dbg_type*            synthetized_types;
    unsigned                    num_synthetized_types;
};

/* describes the way the debugger interacts with a given process */
struct be_process_io
{
    BOOL        (*close_process)(struct dbg_process*, BOOL);
    BOOL        (*read)(HANDLE, const void*, void*, SIZE_T, SIZE_T*);
    BOOL        (*write)(HANDLE, void*, const void*, SIZE_T, SIZE_T*);
    BOOL        (*get_selector)(HANDLE, DWORD, LDT_ENTRY*);
};

extern	struct dbg_process*	dbg_curr_process;
extern	DWORD	                dbg_curr_pid;
extern	struct dbg_thread*	dbg_curr_thread;
extern	DWORD	                dbg_curr_tid;
extern  dbg_ctx_t               dbg_context;
extern  BOOL                    dbg_interactiveP;
extern  HANDLE                  dbg_houtput;

struct dbg_internal_var
{
    DWORD                       val;
    const char*                 name;
    void*                       pval;
    ULONG                       typeid; /* always internal type */
};

enum sym_get_lval {sglv_found, sglv_unknown, sglv_aborted};

enum dbg_start {start_ok, start_error_parse, start_error_init};

  /* break.c */
extern void             break_set_xpoints(BOOL set);
extern BOOL             break_add_break(const ADDRESS64* addr, BOOL verbose, BOOL swbp);
extern BOOL             break_add_break_from_lvalue(const struct dbg_lvalue* value, BOOL swbp);
extern void             break_add_break_from_id(const char* name, int lineno, BOOL swbp);
extern void             break_add_break_from_lineno(const char *filename, int lineno, BOOL swbp);
extern void             break_add_watch_from_lvalue(const struct dbg_lvalue* lvalue, BOOL is_write);
extern void             break_add_watch_from_id(const char* name, BOOL is_write);
extern void             break_check_delayed_bp(void);
extern void             break_delete_xpoint(int num);
extern void             break_delete_xpoints_from_module(DWORD64 base);
extern void             break_enable_xpoint(int num, BOOL enable);
extern void             break_info(void);
extern void             break_adjust_pc(ADDRESS64* addr, DWORD code, BOOL first_chance, BOOL* is_break);
extern BOOL             break_should_continue(ADDRESS64* addr, DWORD code);
extern void             break_suspend_execution(void);
extern void             break_restart_execution(int count);
extern int              break_add_condition(int bpnum, struct expr* exp);

  /* crashdlg.c */
extern int              display_crash_dialog(void);
extern HANDLE           display_crash_details(HANDLE event);
extern int              msgbox_res_id(HWND hwnd, UINT textId, UINT captionId, UINT uType);

  /* dbg.y */
extern void             parser_handle(const char*, HANDLE);
extern int              input_read_line(const char* pfx, char* buffer, int size);
extern size_t           input_lex_read_buffer(char* pfx, int size);
extern HANDLE           WINAPIV parser_generate_command_file(const char*, ...);

  /* debug.l */
extern void             lexeme_flush(void);
extern char*            lexeme_alloc_size(int);

  /* display.c */
extern BOOL             display_print(void);
extern BOOL             display_add(struct expr* exp, int count, char format);
extern BOOL             display_delete(int displaynum);
extern BOOL             display_info(void);
extern BOOL             display_enable(int displaynum, int enable);

  /* expr.c */
extern void             expr_free_all(void);
extern struct expr*     expr_alloc_internal_var(const char* name);
extern struct expr*     expr_alloc_symbol(const char* name);
extern struct expr*     expr_alloc_sconstant(dbg_lgint_t val);
extern struct expr*     expr_alloc_uconstant(dbg_lguint_t val);
extern struct expr*     expr_alloc_string(const char* str);
extern struct expr*     expr_alloc_binary_op(int oper, struct expr*, struct expr*);
extern struct expr*     expr_alloc_unary_op(int oper, struct expr*);
extern struct expr*     expr_alloc_pstruct(struct expr*, const char* element);
extern struct expr*     expr_alloc_struct(struct expr*, const char* element);
extern struct expr*     WINAPIV expr_alloc_func_call(const char*, int nargs, ...);
extern struct expr*     expr_alloc_typecast(struct dbg_type*, struct expr*);
extern struct dbg_lvalue expr_eval(struct expr*);
extern struct expr*     expr_clone(const struct expr* exp, BOOL *local_binding);
extern BOOL             expr_free(struct expr* exp);
extern BOOL             expr_print(const struct expr* exp);

  /* info.c */
extern void             print_help(void);
extern void             info_help(void);
extern void             info_win32_module(DWORD64 mod);
extern void             info_win32_class(HWND hWnd, const char* clsName);
extern void             info_win32_window(HWND hWnd, BOOL detailed);
extern void             info_win32_processes(void);
extern void             info_win32_threads(void);
extern void             info_win32_frame_exceptions(DWORD tid);
extern void             info_win32_virtual(DWORD pid);
extern void             info_win32_segments(DWORD start, int length);
extern void             info_win32_exception(void);
extern void             info_wine_dbg_channel(BOOL add, const char* chnl, const char* name);
extern WCHAR*           fetch_thread_description(DWORD tid);

  /* memory.c */
extern BOOL             memory_read_value(const struct dbg_lvalue* lvalue, DWORD size, void* result);
extern BOOL             memory_write_value(const struct dbg_lvalue* val, DWORD size, void* value);
extern BOOL             memory_transfer_value(const struct dbg_lvalue* to, const struct dbg_lvalue* from);
extern BOOL             memory_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                                             BOOL is_signed, dbg_lgint_t* ret);
extern BOOL             memory_store_integer(const struct dbg_lvalue* lvalue, dbg_lgint_t val);
extern BOOL             memory_fetch_float(const struct dbg_lvalue* lvalue, double *ret);
extern BOOL             memory_store_float(const struct dbg_lvalue* lvalue, double *ret);
extern void             memory_examine(const struct dbg_lvalue *lvalue, int count, char format);
extern void*            memory_to_linear_addr(const ADDRESS64* address);
extern BOOL             memory_get_current_pc(ADDRESS64* address);
extern BOOL             memory_get_current_stack(ADDRESS64* address);
extern BOOL             memory_get_string(struct dbg_process* pcs, void* addr, BOOL in_debuggee, BOOL unicode, char* buffer, int size);
extern BOOL             memory_get_string_indirect(struct dbg_process* pcs, void* addr, BOOL unicode, WCHAR* buffer, int size);
extern BOOL             memory_get_register(DWORD regno, struct dbg_lvalue* value, char* buffer, int len);
extern void             memory_disassemble(const struct dbg_lvalue*, const struct dbg_lvalue*, int instruction_count);
extern BOOL             memory_disasm_one_insn(ADDRESS64* addr);
#define MAX_OFFSET_TO_STR_LEN 19
extern char*            memory_offset_to_string(char *str, DWORD64 offset, unsigned mode);
extern void             print_bare_address(const ADDRESS64* addr);
extern void             print_address(const ADDRESS64* addr, BOOLEAN with_line);
extern void             print_basic(const struct dbg_lvalue* value, char format);

  /* source.c */
extern void             source_list(IMAGEHLP_LINE64* src1, IMAGEHLP_LINE64* src2, int delta);
extern void             source_list_from_addr(const ADDRESS64* addr, int nlines);
extern void             source_show_path(void);
extern void             source_add_path(const char* path);
extern void             source_nuke_path(struct dbg_process* p);
extern void             source_free_files(struct dbg_process* p);

  /* stack.c */
extern void             stack_info(int len);
extern void             stack_backtrace(DWORD threadID);
extern BOOL             stack_set_frame(int newframe);
extern BOOL             stack_get_register_frame(const struct dbg_internal_var* div, struct dbg_lvalue* lvalue);
extern unsigned         stack_fetch_frames(const dbg_ctx_t *ctx);
extern BOOL             stack_get_current_symbol(SYMBOL_INFO* sym);
static inline struct dbg_frame*
                        stack_get_thread_frame(struct dbg_thread* thd, unsigned nf)
{
    if (!thd->frames || nf >= thd->num_frames) return NULL;
    return &thd->frames[nf];
}
static inline struct dbg_frame*
                        stack_get_curr_frame(void)
{
    return stack_get_thread_frame(dbg_curr_thread, dbg_curr_thread->curr_frame);
}

  /* symbol.c */
extern enum sym_get_lval symbol_get_lvalue(const char* name, const int lineno, struct dbg_lvalue* addr, BOOL bp_disp);
extern void             symbol_read_symtable(const char* filename, ULONG_PTR offset);
extern enum dbg_line_status symbol_get_function_line_status(const ADDRESS64* addr);
extern BOOL             symbol_get_line(const char* filename, const char* func, IMAGEHLP_LINE64* ret);
extern void             symbol_info(const char* str);
extern void             symbol_print_localvalue(const SYMBOL_INFO* sym, DWORD_PTR base, BOOL detailed);
extern BOOL             symbol_info_locals(void);
extern BOOL             symbol_is_local(const char* name);
struct sgv_data;
typedef enum sym_get_lval (*symbol_picker_t)(const char* name, const struct sgv_data* sgv,
                                             struct dbg_lvalue* rtn);
extern symbol_picker_t symbol_current_picker;
extern enum sym_get_lval symbol_picker_interactive(const char* name, const struct sgv_data* sgv,
                                                   struct dbg_lvalue* rtn);
extern enum sym_get_lval symbol_picker_scoped(const char* name, const struct sgv_data* sgv,
                                              struct dbg_lvalue* rtn);

  /* tgt_active.c */
struct list_string
{
    char* string;
    struct list_string* next;
};
extern void             dbg_run_debuggee(struct list_string* ls);
extern void             dbg_wait_next_exception(DWORD cont, int count, int mode);
extern enum dbg_start   dbg_active_attach(int argc, char* argv[]);
extern BOOL             dbg_set_curr_thread(DWORD tid);
extern enum dbg_start   dbg_active_launch(int argc, char* argv[]);
extern enum dbg_start   dbg_active_auto(int argc, char* argv[]);
extern enum dbg_start   dbg_active_minidump(int argc, char* argv[]);
extern void             dbg_active_wait_for_first_exception(void);
extern BOOL             dbg_attach_debuggee(DWORD pid);
extern void             fetch_module_name(void* name_addr, void* mod_addr, WCHAR* buffer, size_t bufsz);

  /* tgt_minidump.c */
extern void             minidump_write(const char*, const EXCEPTION_RECORD*);
extern enum dbg_start   minidump_reload(int argc, char* argv[]);

  /* tgt_module.c */
extern enum dbg_start   tgt_module_load(const char* name, BOOL keep);

  /* types.c */
extern void             print_value(const struct dbg_lvalue* addr, char format, int level);
extern BOOL             types_print_type(const struct dbg_type*, BOOL details, const WCHAR* varname);
extern BOOL             print_types(void);
extern dbg_lgint_t      types_extract_as_integer(const struct dbg_lvalue*);
extern dbg_lgint_t      types_extract_as_lgint(const struct dbg_lvalue*, unsigned* psize, BOOL *pissigned);
extern void             types_extract_as_address(const struct dbg_lvalue*, ADDRESS64*);
extern BOOL             types_store_value(struct dbg_lvalue* lvalue_to, const struct dbg_lvalue* lvalue_from);
extern BOOL             types_udt_find_element(struct dbg_lvalue* value, const char* name);
extern BOOL             types_array_index(const struct dbg_lvalue* value, int index, struct dbg_lvalue* result);
extern BOOL             types_get_info(const struct dbg_type*, IMAGEHLP_SYMBOL_TYPE_INFO, void*);
extern BOOL             types_get_real_type(struct dbg_type* type, DWORD* tag);
extern BOOL             types_find_pointer(const struct dbg_type* type, struct dbg_type* outtype);
extern BOOL             types_find_type(const char* name, enum SymTagEnum tag, struct dbg_type* outtype);
extern BOOL             types_compare(const struct dbg_type, const struct dbg_type, BOOL* equal);
extern BOOL             types_is_integral_type(const struct dbg_lvalue*);
extern BOOL             types_is_float_type(const struct dbg_lvalue*);
extern BOOL             types_is_pointer_type(const struct dbg_lvalue*);
extern BOOL             types_find_basic(const WCHAR*, const char*, struct dbg_type* type);
extern BOOL             types_unload_module(DWORD_PTR linear);

  /* winedbg.c */
#ifdef __GNUC__
extern int WINAPIV      dbg_printf(const char* format, ...) __attribute__((format (printf,1,2)));
#else
extern int WINAPIV      dbg_printf(const char* format, ...);
#endif
extern const struct dbg_internal_var* dbg_get_internal_var(const char*);
extern BOOL             dbg_interrupt_debuggee(void);
extern unsigned         dbg_num_processes(void);
extern struct dbg_process* dbg_add_process(const struct be_process_io* pio, DWORD pid, HANDLE h);
extern void             dbg_set_process_name(struct dbg_process* p, const WCHAR* name);
extern struct dbg_process* dbg_get_process(DWORD pid);
extern struct dbg_process* dbg_get_process_h(HANDLE handle);
extern void             dbg_del_process(struct dbg_process* p);
struct dbg_thread*	dbg_add_thread(struct dbg_process* p, DWORD tid, HANDLE h, void* teb);
extern struct dbg_thread* dbg_get_thread(struct dbg_process* p, DWORD tid);
extern void             dbg_del_thread(struct dbg_thread* t);
extern BOOL             dbg_init(HANDLE hProc, const WCHAR* in, BOOL invade);
extern BOOL             dbg_load_module(HANDLE hProc, HANDLE hFile, const WCHAR* name, DWORD_PTR base, DWORD size);
extern void             dbg_set_option(const char*, const char*);
extern void             dbg_start_interactive(const char*, HANDLE hFile);
extern void             dbg_init_console(void);

  /* gdbproxy.c */
extern int              gdb_main(int argc, char* argv[]);

static inline BOOL dbg_read_memory(const void* addr, void* buffer, size_t len)
{
    SIZE_T rlen;
    return dbg_curr_process->process_io->read(dbg_curr_process->handle, addr, buffer, len, &rlen) && len == rlen;
}

static inline BOOL dbg_write_memory(void* addr, const void* buffer, size_t len)
{
    SIZE_T wlen;
    return dbg_curr_process->process_io->write(dbg_curr_process->handle, addr, buffer, len, &wlen) && len == wlen;
}

struct data_model
{
    enum dbg_internal_types     itype;
    const WCHAR*                name;
};

extern const struct data_model ilp32_data_model[];
extern const struct data_model lp64_data_model[];
extern const struct data_model llp64_data_model[];

extern struct dbg_internal_var          dbg_internal_vars[];

#define  DBG_IVARNAME(_var)	dbg_internal_var_##_var
#define  DBG_IVARSTRUCT(_var)	dbg_internal_vars[DBG_IVARNAME(_var)]
#define  DBG_IVAR(_var)		(DBG_IVARSTRUCT(_var).val)
#define  INTERNAL_VAR(_var,_val,_ref,itype) DBG_IVARNAME(_var),
enum debug_int_var
{
#include "intvar.h"
   DBG_IV_LAST
};
#undef   INTERNAL_VAR

/* include CPU dependent bits */
#include "be_cpu.h"

#endif  /* __WINE_DEBUGGER_H */
