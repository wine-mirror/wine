/* Wine internal debugger
 * Interface to Windows debugger API
 * Copyright 2000-2004 Eric Pouech
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debugger.h"

#include "winternl.h"
#include "wine/exception.h"
#include "wine/library.h"

#include "wine/debug.h"

/* TODO list:
 *
 * - minidump
 *      + allow winedbg in automatic mode to create a minidump (or add another option
 *        for that)
 *      + set a mode where winedbg would start (postmortem debugging) from a minidump
 * - CPU adherence
 *      + we always assume the stack grows as on i386 (ie downwards)
 * - UI
 *      + enable back the limited output (depth of structure printing and number of 
 *        lines)
 *      + make the output as close as possible to what gdb does
 * - symbol management:
 *      + symbol table loading is broken
 *      + in symbol_get_lvalue, we don't do any scoping (as C does) between local and
 *        global vars (we may need this to force some display for example). A solution
 *        would be always to return arrays with: local vars, global vars, thunks
 * - type management:
 *      + some bits of internal types are missing (like type casts and the address
 *        operator)
 *      + the type for an enum's value is always inferred as int (winedbg & dbghelp)
 *      + most of the code implies that sizeof(void*) = sizeof(int)
 *      + all computations should be made on long long
 *              o expr computations are in int:s
 *              o bitfield size is on a 4-bytes
 *      + array_index and deref should be the same function (or should share the same
 *        core)
 * - execution:
 *      + set a better fix for gdb (proxy mode) than the step-mode hack
 *      + implement function call in debuggee
 *      + trampoline management is broken when getting 16 <=> 32 thunk destination
 *        address
 *      + thunking of delayed imports doesn't work as expected (ie, when stepping,
 *        it currently stops at first insn with line number during the library 
 *        loading). We should identify this (__wine_delay_import) and set a
 *        breakpoint instead of single stepping the library loading.
 *      + it's wrong to copy thread->step_over_bp into process->bp[0] (when 
 *        we have a multi-thread debuggee). complete fix must include storing all
 *        thread's step-over bp in process-wide bp array, and not to handle bp
 *        when we have the wrong thread running into that bp
 *      + code in CREATE_PROCESS debug event doesn't work on Windows, as we cannot
 *        get the name of the main module this way. We should rewrite all this code
 *        and store in struct dbg_process as early as possible (before process
 *        creation or attachment), the name of the main module
 * - global:
 *      + define a better way to enable the wine extensions (either DBG SDK function
 *        in dbghelp, or TLS variable, or environment variable or ...)
 *      + audit all files to ensure that we check all potential return values from
 *        every function call to catch the errors
 *      + BTW check also whether the exception mechanism is the best way to return
 *        errors (or find a proper fix for MinGW port)
 *      + use Wine standard list mechanism for all list handling
 */

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

struct dbg_process*	dbg_curr_process = NULL;
struct dbg_thread*	dbg_curr_thread = NULL;
DWORD		        dbg_curr_tid;
DWORD		        dbg_curr_pid;
CONTEXT                 dbg_context;
BOOL    	        dbg_interactiveP = FALSE;
static char*	        dbg_last_cmd_line = NULL;

static struct dbg_process*      dbg_process_list = NULL;
static enum {none_mode = 0, winedbg_mode, automatic_mode, gdb_mode} dbg_action_mode;

struct dbg_internal_var         dbg_internal_vars[DBG_IV_LAST];
const struct dbg_internal_var*  dbg_context_vars;
static HANDLE                   dbg_houtput;

void	dbg_outputA(const char* buffer, int len)
{
    static char line_buff[4096];
    static unsigned int line_pos;

    DWORD w, i;

    while (len > 0)
    {
        unsigned int count = min( len, sizeof(line_buff) - line_pos );
        memcpy( line_buff + line_pos, buffer, count );
        buffer += count;
        len -= count;
        line_pos += count;
        for (i = line_pos; i > 0; i--) if (line_buff[i-1] == '\n') break;
        if (!i)  /* no newline found */
        {
            if (len > 0) i = line_pos;  /* buffer is full, flush anyway */
            else break;
        }
        WriteFile(dbg_houtput, line_buff, i, &w, NULL);
        memmove( line_buff, line_buff + i, line_pos - i );
        line_pos -= i;
    }
}

void	dbg_outputW(const WCHAR* buffer, int len)
{
    char* ansi = NULL;
    int newlen;
	
    /* do a serious Unicode to ANSI conversion
     * FIXME: should CP_ACP be GetConsoleCP()?
     */
    newlen = WideCharToMultiByte(CP_ACP, 0, buffer, len, NULL, 0, NULL, NULL);
    if (newlen)
    {
        if (!(ansi = HeapAlloc(GetProcessHeap(), 0, newlen))) return;
        WideCharToMultiByte(CP_ACP, 0, buffer, len, ansi, newlen, NULL, NULL);
        dbg_outputA(ansi, newlen);
        HeapFree(GetProcessHeap(), 0, ansi);
    }
}

int	dbg_printf(const char* format, ...)
{
    static    char	buf[4*1024];
    va_list 	valist;
    int		len;

    va_start(valist, format);
    len = vsnprintf(buf, sizeof(buf), format, valist);
    va_end(valist);

    if (len <= -1 || len >= sizeof(buf)) 
    {
	len = sizeof(buf) - 1;
	buf[len] = 0;
	buf[len - 1] = buf[len - 2] = buf[len - 3] = '.';
    }
    dbg_outputA(buf, len);
    return len;
}

static	unsigned dbg_load_internal_vars(void)
{
    HKEY	                hkey;
    DWORD 	                type = REG_DWORD;
    DWORD	                val;
    DWORD 	                count = sizeof(val);
    int		                i;
    struct dbg_internal_var*    div = dbg_internal_vars;

/* initializes internal vars table */
#define  INTERNAL_VAR(_var,_val,_ref,_tid) 			\
        div->val = _val; div->name = #_var; div->pval = _ref;	\
        div->typeid = _tid; div++;
#include "intvar.h"
#undef   INTERNAL_VAR

    /* @@ Wine registry key: HKCU\Software\Wine\WineDbg */
    if (RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\WineDbg", &hkey)) 
    {
	WINE_ERR("Cannot create WineDbg key in registry\n");
	return FALSE;
    }

    for (i = 0; i < DBG_IV_LAST; i++) 
    {
        if (!dbg_internal_vars[i].pval) 
        {
            if (!RegQueryValueEx(hkey, dbg_internal_vars[i].name, 0,
                                 &type, (LPBYTE)&val, &count))
                dbg_internal_vars[i].val = val;
            dbg_internal_vars[i].pval = &dbg_internal_vars[i].val;
        }
    }
    RegCloseKey(hkey);
    /* set up the debug variables for the CPU context */
    dbg_context_vars = be_cpu->init_registers(&dbg_context);
    return TRUE;
}

static	unsigned dbg_save_internal_vars(void)
{
    HKEY	                hkey;
    int		                i;

    /* @@ Wine registry key: HKCU\Software\Wine\WineDbg */
    if (RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\WineDbg", &hkey)) 
    {
	WINE_ERR("Cannot create WineDbg key in registry\n");
	return FALSE;
    }

    for (i = 0; i < DBG_IV_LAST; i++) 
    {
        /* FIXME: type should be infered from basic type -if any- of intvar */
        if (dbg_internal_vars[i].pval == &dbg_internal_vars[i].val)
            RegSetValueEx(hkey, dbg_internal_vars[i].name, 0,
                          REG_DWORD, (const void*)dbg_internal_vars[i].pval, 
                          sizeof(*dbg_internal_vars[i].pval));
    }
    RegCloseKey(hkey);
    return TRUE;
}

const struct dbg_internal_var* dbg_get_internal_var(const char* name)
{
    const struct dbg_internal_var*      div;

    for (div = &dbg_internal_vars[DBG_IV_LAST - 1]; div >= dbg_internal_vars; div--)
    {
	if (!strcmp(div->name, name)) return div;
    }
    for (div = dbg_context_vars; div->name; div++)
    {
	if (!strcasecmp(div->name, name)) return div;
    }

    return NULL;
}

struct dbg_process*     dbg_get_process(DWORD pid)
{
    struct dbg_process*	p;

    for (p = dbg_process_list; p; p = p->next)
	if (p->pid == pid) break;
    return p;
}

struct dbg_process*	dbg_add_process(DWORD pid, HANDLE h)
{
    /* FIXME: temporary */
    extern struct be_process_io be_process_active_io;

    struct dbg_process*	p;

    if ((p = dbg_get_process(pid)))
    {
        if (p->handle != 0)
        {
            WINE_ERR("Process (%lu) is already defined\n", pid);
        }
        else
        {
            p->handle = h;
            p->imageName = NULL;
        }
        return p;
    }

    if (!(p = HeapAlloc(GetProcessHeap(), 0, sizeof(struct dbg_process)))) return NULL;
    p->handle = h;
    p->pid = pid;
    p->process_io = &be_process_active_io;
    p->imageName = NULL;
    p->threads = NULL;
    p->continue_on_first_exception = FALSE;
    p->next_bp = 1;  /* breakpoint 0 is reserved for step-over */
    memset(p->bp, 0, sizeof(p->bp));
    p->delayed_bp = NULL;
    p->num_delayed_bp = 0;

    p->next = dbg_process_list;
    p->prev = NULL;
    if (dbg_process_list) dbg_process_list->prev = p;
    dbg_process_list = p;
    return p;
}

void dbg_set_process_name(struct dbg_process* p, const char* imageName)
{
    assert(p->imageName == NULL);
    if (imageName)
    {
        char* tmp = HeapAlloc(GetProcessHeap(), 0, strlen(imageName) + 1);
        if (tmp) p->imageName = strcpy(tmp, imageName);
    }
}

void dbg_del_process(struct dbg_process* p)
{
    int	i;

    while (p->threads) dbg_del_thread(p->threads);

    for (i = 0; i < p->num_delayed_bp; i++)
        if (p->delayed_bp[i].is_symbol)
            HeapFree(GetProcessHeap(), 0, p->delayed_bp[i].u.symbol.name);

    HeapFree(GetProcessHeap(), 0, p->delayed_bp);
    if (p->prev) p->prev->next = p->next;
    if (p->next) p->next->prev = p->prev;
    if (p == dbg_process_list) dbg_process_list = p->next;
    if (p == dbg_curr_process) dbg_curr_process = NULL;
    HeapFree(GetProcessHeap(), 0, (char*)p->imageName);
    HeapFree(GetProcessHeap(), 0, p);
}

static void dbg_init_current_process(void)
{
}

struct mod_loader_info
{
    HANDLE              handle;
    IMAGEHLP_MODULE*    imh_mod;
};

static BOOL CALLBACK mod_loader_cb(PSTR mod_name, DWORD base, void* ctx)
{
    struct mod_loader_info*     mli = (struct mod_loader_info*)ctx;

    if (!strcmp(mod_name, "<wine-loader>"))
    {
        if (SymGetModuleInfo(mli->handle, base, mli->imh_mod))
            return FALSE; /* stop enum */
    }
    return TRUE;
}

BOOL dbg_get_debuggee_info(HANDLE hProcess, IMAGEHLP_MODULE* imh_mod)
{
    struct mod_loader_info  mli;
    DWORD                   opt;

    /* this will resynchronize builtin dbghelp's internal ELF module list */
    SymLoadModule(hProcess, 0, 0, 0, 0, 0);
    mli.handle  = hProcess;
    mli.imh_mod = imh_mod;
    imh_mod->SizeOfStruct = sizeof(*imh_mod);
    imh_mod->BaseOfImage = 0;
    /* this is a wine specific options to return also ELF modules in the
     * enumeration
     */
    SymSetOptions((opt = SymGetOptions()) | 0x40000000);
    SymEnumerateModules(hProcess, mod_loader_cb, (void*)&mli);
    SymSetOptions(opt);

    return imh_mod->BaseOfImage != 0;
}

struct dbg_thread* dbg_get_thread(struct dbg_process* p, DWORD tid)
{
    struct dbg_thread*	t;

    if (!p) return NULL;
    for (t = p->threads; t; t = t->next)
	if (t->tid == tid) break;
    return t;
}

struct dbg_thread* dbg_add_thread(struct dbg_process* p, DWORD tid,
                                  HANDLE h, void* teb)
{
    struct dbg_thread*	t = HeapAlloc(GetProcessHeap(), 0, sizeof(struct dbg_thread));

    if (!t)
	return NULL;

    t->handle = h;
    t->tid = tid;
    t->teb = teb;
    t->process = p;
    t->exec_mode = dbg_exec_cont;
    t->exec_count = 0;
    t->step_over_bp.enabled = FALSE;
    t->step_over_bp.refcount = 0;
    t->stopped_xpoint = -1;
    t->in_exception = FALSE;
    t->frames = NULL;
    t->num_frames = 0;
    t->curr_frame = -1;
    t->addr_mode = AddrModeFlat;

    snprintf(t->name, sizeof(t->name), "0x%08lx", tid);

    t->next = p->threads;
    t->prev = NULL;
    if (p->threads) p->threads->prev = t;
    p->threads = t;

    return t;
}

static void dbg_init_current_thread(void* start)
{
    if (start)
    {
	if (dbg_curr_process->threads && 
            !dbg_curr_process->threads->next && /* first thread ? */
	    DBG_IVAR(BreakAllThreadsStartup)) 
        {
	    ADDRESS     addr;

            break_set_xpoints(FALSE);
	    addr.Mode   = AddrModeFlat;
	    addr.Offset = (DWORD)start;
	    break_add_break(&addr, TRUE, TRUE);
	    break_set_xpoints(TRUE);
	}
    } 
}

void dbg_del_thread(struct dbg_thread* t)
{
    HeapFree(GetProcessHeap(), 0, t->frames);
    if (t->prev) t->prev->next = t->next;
    if (t->next) t->next->prev = t->prev;
    if (t == t->process->threads) t->process->threads = t->next;
    if (t == dbg_curr_thread) dbg_curr_thread = NULL;
    HeapFree(GetProcessHeap(), 0, t);
}

static unsigned dbg_handle_debug_event(DEBUG_EVENT* de);

/******************************************************************
 *		dbg_attach_debuggee
 *
 * Sets the debuggee to <pid>
 * cofe instructs winedbg what to do when first exception is received 
 * (break=FALSE, continue=TRUE)
 * wfe is set to TRUE if dbg_attach_debuggee should also proceed with all debug events
 * until the first exception is received (aka: attach to an already running process)
 */
BOOL dbg_attach_debuggee(DWORD pid, BOOL cofe, BOOL wfe)
{
    DEBUG_EVENT         de;

    if (!(dbg_curr_process = dbg_add_process(pid, 0))) return FALSE;

    if (!DebugActiveProcess(pid)) 
    {
        dbg_printf("Can't attach process %lx: error %ld\n", pid, GetLastError());
        dbg_del_process(dbg_curr_process);
	return FALSE;
    }
    dbg_curr_process->continue_on_first_exception = cofe;

    if (wfe) /* shall we proceed all debug events until we get an exception ? */
    {
        dbg_interactiveP = FALSE;
        while (dbg_curr_process && WaitForDebugEvent(&de, INFINITE))
        {
            if (dbg_handle_debug_event(&de)) break;
        }
        if (dbg_curr_process) dbg_interactiveP = TRUE;
    }
    return TRUE;
}

BOOL dbg_detach_debuggee(void)
{
    /* remove all set breakpoints in debuggee code */
    break_set_xpoints(FALSE);
    /* needed for single stepping (ugly).
     * should this be handled inside the server ??? 
     */
    be_cpu->single_step(&dbg_context, FALSE);
    SetThreadContext(dbg_curr_thread->handle, &dbg_context);
    if (dbg_curr_thread->in_exception)
        ContinueDebugEvent(dbg_curr_pid, dbg_curr_tid, DBG_CONTINUE);
    if (!DebugActiveProcessStop(dbg_curr_pid)) return FALSE;
    dbg_del_process(dbg_curr_process);

    return TRUE;
}

static unsigned dbg_fetch_context(void)
{
    dbg_context.ContextFlags = CONTEXT_CONTROL
        | CONTEXT_INTEGER
#ifdef CONTEXT_SEGMENTS
        | CONTEXT_SEGMENTS
#endif
#ifdef CONTEXT_DEBUG_REGISTERS
        | CONTEXT_DEBUG_REGISTERS
#endif
        ;
    if (!GetThreadContext(dbg_curr_thread->handle, &dbg_context))
    {
        WINE_WARN("Can't get thread's context\n");
        return FALSE;
    }
    return TRUE;
}

static unsigned dbg_exception_prolog(BOOL is_debug, const EXCEPTION_RECORD* rec)
{
    ADDRESS     addr;
    BOOL        is_break;

    memory_get_current_pc(&addr);
    break_suspend_execution();
    dbg_curr_thread->excpt_record = *rec;
    dbg_curr_thread->in_exception = TRUE;

    if (!is_debug)
    {
        switch (addr.Mode)
        {
        case AddrModeFlat: dbg_printf(" in 32-bit code (0x%08lx)", addr.Offset); break;
        case AddrModeReal: dbg_printf(" in vm86 code (%04x:%04lx)", addr.Segment, addr.Offset); break;
        case AddrMode1616: dbg_printf(" in 16-bit code (%04x:%04lx)", addr.Segment, addr.Offset); break;
        case AddrMode1632: dbg_printf(" in 32-bit code (%04x:%08lx)", addr.Segment, addr.Offset); break;
        default: dbg_printf(" bad address");
        }
	dbg_printf(".\n");
    }

    /* this will resynchronize builtin dbghelp's internal ELF module list */
    SymLoadModule(dbg_curr_process->handle, 0, 0, 0, 0, 0);

    /*
     * Do a quiet backtrace so that we have an idea of what the situation
     * is WRT the source files.
     */
    stack_fetch_frames();
    if (is_debug &&
	break_should_continue(&addr, rec->ExceptionCode, &dbg_curr_thread->exec_count, &is_break))
	return FALSE;

    if (addr.Mode != dbg_curr_thread->addr_mode)
    {
        const char* name = NULL;
        
        switch (addr.Mode)
        {
        case AddrMode1616: name = "16 bit";     break;
        case AddrMode1632: name = "32 bit";     break;
        case AddrModeReal: name = "vm86";       break;
        case AddrModeFlat: name = "32 bit";     break;
        }
        
        dbg_printf("In %s mode.\n", name);
        dbg_curr_thread->addr_mode = addr.Mode;
    }
    display_print();

    if (!is_debug)
    {
	/* This is a real crash, dump some info */
	be_cpu->print_context(dbg_curr_thread->handle, &dbg_context);
	stack_info();
        be_cpu->print_segment_info(dbg_curr_thread->handle, &dbg_context);
	stack_backtrace(dbg_curr_tid);
    }
    else
    {
        static char*        last_name;
        static char*        last_file;

        char                buffer[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO*        si = (SYMBOL_INFO*)buffer;
        void*               lin = memory_to_linear_addr(&addr);
        DWORD64             disp64;
        IMAGEHLP_LINE       il;
        DWORD               disp;

        si->SizeOfStruct = sizeof(*si);
        si->MaxNameLen   = 256;
        il.SizeOfStruct = sizeof(il);
        if (SymFromAddr(dbg_curr_process->handle, (DWORD_PTR)lin, &disp64, si) &&
            SymGetLineFromAddr(dbg_curr_process->handle, (DWORD_PTR)lin, &disp, &il))
        {
            if ((!last_name || strcmp(last_name, si->Name)) ||
                (!last_file || strcmp(last_file, il.FileName)))
            {
                HeapFree(GetProcessHeap(), 0, last_name);
                HeapFree(GetProcessHeap(), 0, last_file);
                last_name = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(si->Name) + 1), si->Name);
                last_file = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(il.FileName) + 1), il.FileName);
                dbg_printf("%s () at %s:%ld\n", last_name, last_file, il.LineNumber);
            }
        }
    }
    if (!is_debug || is_break ||
        dbg_curr_thread->exec_mode == dbg_exec_step_over_insn ||
        dbg_curr_thread->exec_mode == dbg_exec_step_into_insn)
    {
        ADDRESS tmp = addr;
        /* Show where we crashed */
        memory_disasm_one_insn(&tmp);
    }
    source_list_from_addr(&addr, 0);

    return TRUE;
}

static void dbg_exception_epilog(void)
{
    break_restart_execution(dbg_curr_thread->exec_count);
    /*
     * This will have gotten absorbed into the breakpoint info
     * if it was used.  Otherwise it would have been ignored.
     * In any case, we don't mess with it any more.
     */
    if (dbg_curr_thread->exec_mode == dbg_exec_cont)
	dbg_curr_thread->exec_count = 0;
    dbg_curr_thread->in_exception = FALSE;
}

static DWORD dbg_handle_exception(const EXCEPTION_RECORD* rec, BOOL first_chance)
{
    BOOL                is_debug = FALSE;
    THREADNAME_INFO*    pThreadName;
    struct dbg_thread*  pThread;

    assert(dbg_curr_thread);

    WINE_TRACE("exception=%lx first_chance=%c\n",
               rec->ExceptionCode, first_chance ? 'Y' : 'N');

    switch (rec->ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
        is_debug = TRUE;
        break;
    case EXCEPTION_NAME_THREAD:
        pThreadName = (THREADNAME_INFO*)(rec->ExceptionInformation);
        if (pThreadName->dwThreadID == -1)
            pThread = dbg_curr_thread;
        else
            pThread = dbg_get_thread(dbg_curr_process, pThreadName->dwThreadID);

        if (dbg_read_memory(pThreadName->szName, pThread->name, 9))
            dbg_printf("Thread ID=0x%lx renamed using MS VC6 extension (name==\"%s\")\n",
                       pThread->tid, pThread->name);
        return DBG_CONTINUE;
    }

    if (first_chance && !is_debug && !DBG_IVAR(BreakOnFirstChance) &&
	!(rec->ExceptionFlags & EH_STACK_INVALID))
    {
        /* pass exception to program except for debug exceptions */
        return DBG_EXCEPTION_NOT_HANDLED;
    }

    if (!is_debug)
    {
        /* print some infos */
        dbg_printf("%s: ",
                   first_chance ? "First chance exception" : "Unhandled exception");
        switch (rec->ExceptionCode)
        {
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            dbg_printf("divide by zero");
            break;
        case EXCEPTION_INT_OVERFLOW:
            dbg_printf("overflow");
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            dbg_printf("array bounds");
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            dbg_printf("illegal instruction");
            break;
        case EXCEPTION_STACK_OVERFLOW:
            dbg_printf("stack overflow");
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            dbg_printf("privileged instruction");
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            if (rec->NumberParameters == 2)
                dbg_printf("page fault on %s access to 0x%08lx",
                           rec->ExceptionInformation[0] ? "write" : "read",
                           rec->ExceptionInformation[1]);
            else
                dbg_printf("page fault");
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            dbg_printf("Alignment");
            break;
	case DBG_CONTROL_C:
            dbg_printf("^C");
            break;
        case CONTROL_C_EXIT:
            dbg_printf("^C");
            break;
        case STATUS_POSSIBLE_DEADLOCK:
        {
            ADDRESS         addr;

            addr.Mode   = AddrModeFlat;
            addr.Offset = rec->ExceptionInformation[0];

            dbg_printf("wait failed on critical section ");
            print_address(&addr, FALSE);
        }
        if (!DBG_IVAR(BreakOnCritSectTimeOut))
        {
            dbg_printf("\n");
            return DBG_EXCEPTION_NOT_HANDLED;
        }
        break;
        case EXCEPTION_WINE_STUB:
        {
            char dll[32], name[64];
            memory_get_string(dbg_curr_process,
                              (void*)rec->ExceptionInformation[0], TRUE, FALSE,
                              dll, sizeof(dll));
            if (HIWORD(rec->ExceptionInformation[1]))
                memory_get_string(dbg_curr_process,
                                  (void*)rec->ExceptionInformation[1], TRUE, FALSE,
                                  name, sizeof(name));
            else
                sprintf( name, "%ld", rec->ExceptionInformation[1] );
            dbg_printf("unimplemented function %s.%s called", dll, name);
        }
        break;
        case EXCEPTION_WINE_ASSERTION:
            dbg_printf("assertion failed");
            break;
        case EXCEPTION_VM86_INTx:
            dbg_printf("interrupt %02lx in vm86 mode", rec->ExceptionInformation[0]);
            break;
        case EXCEPTION_VM86_STI:
            dbg_printf("sti in vm86 mode");
            break;
        case EXCEPTION_VM86_PICRETURN:
            dbg_printf("PIC return in vm86 mode");
            break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
            dbg_printf("denormal float operand");
            break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            dbg_printf("divide by zero");
            break;
	case EXCEPTION_FLT_INEXACT_RESULT:
            dbg_printf("inexact float result");
            break;
	case EXCEPTION_FLT_INVALID_OPERATION:
            dbg_printf("invalid float operation");
            break;
	case EXCEPTION_FLT_OVERFLOW:
            dbg_printf("floating pointer overflow");
            break;
	case EXCEPTION_FLT_UNDERFLOW:
            dbg_printf("floating pointer underflow");
            break;
	case EXCEPTION_FLT_STACK_CHECK:
            dbg_printf("floating point stack check");
            break;
        default:
            dbg_printf("0x%08lx", rec->ExceptionCode);
            break;
        }
    }
    if( (rec->ExceptionFlags & EH_STACK_INVALID) ) {
        dbg_printf( ", invalid program stack" );
    }

    if (dbg_action_mode == automatic_mode)
    {
        dbg_exception_prolog(is_debug, rec);
        dbg_exception_epilog();
        return 0;  /* terminate execution */
    }

    if (dbg_exception_prolog(is_debug, rec))
    {
	dbg_interactiveP = TRUE;
        return 0;
    }
    dbg_exception_epilog();

    return DBG_CONTINUE;
}

static unsigned dbg_handle_debug_event(DEBUG_EVENT* de)
{
    char	buffer[256];
    DWORD       cont = DBG_CONTINUE;

    dbg_curr_pid = de->dwProcessId;
    dbg_curr_tid = de->dwThreadId;

    if ((dbg_curr_process = dbg_get_process(de->dwProcessId)) != NULL)
        dbg_curr_thread = dbg_get_thread(dbg_curr_process, de->dwThreadId);
    else
        dbg_curr_thread = NULL;

    switch (de->dwDebugEventCode)
    {
    case EXCEPTION_DEBUG_EVENT:
        if (!dbg_curr_thread)
        {
            WINE_ERR("%08lx:%08lx: not a registered process or thread (perhaps a 16 bit one ?)\n",
                     de->dwProcessId, de->dwThreadId);
            break;
        }

        WINE_TRACE("%08lx:%08lx: exception code=%08lx\n",
                   de->dwProcessId, de->dwThreadId,
                   de->u.Exception.ExceptionRecord.ExceptionCode);

        if (dbg_curr_process->continue_on_first_exception)
        {
            dbg_curr_process->continue_on_first_exception = FALSE;
            if (!DBG_IVAR(BreakOnAttach)) break;
        }
        if (dbg_fetch_context())
        {
            cont = dbg_handle_exception(&de->u.Exception.ExceptionRecord,
                                        de->u.Exception.dwFirstChance);
            if (cont && dbg_curr_thread)
            {
                SetThreadContext(dbg_curr_thread->handle, &dbg_context);
            }
        }
        break;

    case CREATE_PROCESS_DEBUG_EVENT:
        dbg_curr_process = dbg_add_process(de->dwProcessId,
                                           de->u.CreateProcessInfo.hProcess);
        if (dbg_curr_process == NULL)
        {
            WINE_ERR("Couldn't create process\n");
            break;
        }
        memory_get_string_indirect(dbg_curr_process,
                                   de->u.CreateProcessInfo.lpImageName,
                                   de->u.CreateProcessInfo.fUnicode,
                                   buffer, sizeof(buffer));
        if (!buffer[0]) strcpy(buffer, "<Debugged Process>");

        WINE_TRACE("%08lx:%08lx: create process '%s'/%p @%08lx (%ld<%ld>)\n",
                   de->dwProcessId, de->dwThreadId,
                   buffer, de->u.CreateProcessInfo.lpImageName,
                   (unsigned long)(void*)de->u.CreateProcessInfo.lpStartAddress,
                   de->u.CreateProcessInfo.dwDebugInfoFileOffset,
                   de->u.CreateProcessInfo.nDebugInfoSize);
        dbg_set_process_name(dbg_curr_process, buffer);

        if (!SymInitialize(dbg_curr_process->handle, NULL, TRUE))
            dbg_printf("Couldn't initiate DbgHelp\n");

        WINE_TRACE("%08lx:%08lx: create thread I @%08lx\n",
                   de->dwProcessId, de->dwThreadId,
                   (unsigned long)(void*)de->u.CreateProcessInfo.lpStartAddress);

        dbg_curr_thread = dbg_add_thread(dbg_curr_process,
                                         de->dwThreadId,
                                         de->u.CreateProcessInfo.hThread,
                                         de->u.CreateProcessInfo.lpThreadLocalBase);
        if (!dbg_curr_thread)
        {
            WINE_ERR("Couldn't create thread\n");
            break;
        }
        dbg_init_current_process();
        dbg_init_current_thread(de->u.CreateProcessInfo.lpStartAddress);
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        WINE_TRACE("%08lx:%08lx: exit process (%ld)\n",
                   de->dwProcessId, de->dwThreadId, de->u.ExitProcess.dwExitCode);

        if (dbg_curr_process == NULL)
        {
            WINE_ERR("Unknown process\n");
            break;
        }
        if (!SymCleanup(dbg_curr_process->handle))
            dbg_printf("Couldn't initiate DbgHelp\n");
        /* just in case */
        break_set_xpoints(FALSE);
        /* kill last thread */
        dbg_del_thread(dbg_curr_process->threads);
        dbg_del_process(dbg_curr_process);

        dbg_printf("Process of pid=0x%08lx has terminated\n", dbg_curr_pid);
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        WINE_TRACE("%08lx:%08lx: create thread D @%08lx\n",
                   de->dwProcessId, de->dwThreadId,
                   (unsigned long)(void*)de->u.CreateThread.lpStartAddress);

        if (dbg_curr_process == NULL)
        {
            WINE_ERR("Unknown process\n");
            break;
        }
        if (dbg_get_thread(dbg_curr_process, de->dwThreadId) != NULL)
        {
            WINE_TRACE("Thread already listed, skipping\n");
            break;
        }

        dbg_curr_thread = dbg_add_thread(dbg_curr_process,
                                         de->dwThreadId,
                                         de->u.CreateThread.hThread,
                                         de->u.CreateThread.lpThreadLocalBase);
        if (!dbg_curr_thread)
        {
            WINE_ERR("Couldn't create thread\n");
            break;
        }
        dbg_init_current_thread(de->u.CreateThread.lpStartAddress);
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        WINE_TRACE("%08lx:%08lx: exit thread (%ld)\n",
                   de->dwProcessId, de->dwThreadId, de->u.ExitThread.dwExitCode);

        if (dbg_curr_thread == NULL)
        {
            WINE_ERR("Unknown thread\n");
            break;
        }
        /* FIXME: remove break point set on thread startup */
        dbg_del_thread(dbg_curr_thread);
        break;

    case LOAD_DLL_DEBUG_EVENT:
        if (dbg_curr_thread == NULL)
        {
            WINE_ERR("Unknown thread\n");
            break;
        }
        memory_get_string_indirect(dbg_curr_process, 
                                   de->u.LoadDll.lpImageName,
                                   de->u.LoadDll.fUnicode,
                                   buffer, sizeof(buffer));

        WINE_TRACE("%08lx:%08lx: loads DLL %s @%08lx (%ld<%ld>)\n",
                   de->dwProcessId, de->dwThreadId,
                   buffer, (unsigned long)de->u.LoadDll.lpBaseOfDll,
                   de->u.LoadDll.dwDebugInfoFileOffset,
                   de->u.LoadDll.nDebugInfoSize);
        SymLoadModule(dbg_curr_process->handle, de->u.LoadDll.hFile, buffer, NULL,
                      (unsigned long)de->u.LoadDll.lpBaseOfDll, 0);
        break_set_xpoints(FALSE);
        break_check_delayed_bp();
        break_set_xpoints(TRUE);
        if (DBG_IVAR(BreakOnDllLoad))
        {
            dbg_printf("Stopping on DLL %s loading at 0x%08lx\n",
                       buffer, (unsigned long)de->u.LoadDll.lpBaseOfDll);
            if (dbg_fetch_context()) cont = 0;
        }
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        WINE_TRACE("%08lx:%08lx: unload DLL @%08lx\n", 
                   de->dwProcessId, de->dwThreadId,
                   (unsigned long)de->u.UnloadDll.lpBaseOfDll);
        break_delete_xpoints_from_module((unsigned long)de->u.UnloadDll.lpBaseOfDll);
        SymUnloadModule(dbg_curr_process->handle, 
                        (unsigned long)de->u.UnloadDll.lpBaseOfDll);
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        if (dbg_curr_thread == NULL)
        {
            WINE_ERR("Unknown thread\n");
            break;
        }

        memory_get_string(dbg_curr_process,
                          de->u.DebugString.lpDebugStringData, TRUE,
                          de->u.DebugString.fUnicode, buffer, sizeof(buffer));
        WINE_TRACE("%08lx:%08lx: output debug string (%s)\n",
                   de->dwProcessId, de->dwThreadId, buffer);
        break;

    case RIP_EVENT:
        WINE_TRACE("%08lx:%08lx: rip error=%ld type=%ld\n",
                   de->dwProcessId, de->dwThreadId, de->u.RipInfo.dwError,
                   de->u.RipInfo.dwType);
        break;

    default:
        WINE_TRACE("%08lx:%08lx: unknown event (%ld)\n",
                   de->dwProcessId, de->dwThreadId, de->dwDebugEventCode);
    }
    if (!cont) return TRUE;  /* stop execution */
    ContinueDebugEvent(de->dwProcessId, de->dwThreadId, cont);
    return FALSE;  /* continue execution */
}

static void dbg_resume_debuggee(DWORD cont)
{
    if (dbg_curr_thread->in_exception)
    {
        ADDRESS         addr;

        dbg_exception_epilog();
        memory_get_current_pc(&addr);
        WINE_TRACE("Exiting debugger      PC=0x%lx mode=%d count=%d\n",
                   addr.Offset, dbg_curr_thread->exec_mode,
                   dbg_curr_thread->exec_count);
        if (dbg_curr_thread)
        {
            if (!SetThreadContext(dbg_curr_thread->handle, &dbg_context))
                dbg_printf("Cannot set ctx on %lu\n", dbg_curr_tid);
        }
    }
    dbg_interactiveP = FALSE;
    if (!ContinueDebugEvent(dbg_curr_pid, dbg_curr_tid, cont))
        dbg_printf("Cannot continue on %lu (%lu)\n", dbg_curr_tid, cont);
}

void dbg_wait_next_exception(DWORD cont, int count, int mode)
{
    DEBUG_EVENT         de;
    ADDRESS             addr;

    if (cont == DBG_CONTINUE)
    {
        dbg_curr_thread->exec_count = count;
        dbg_curr_thread->exec_mode = mode;
    }
    dbg_resume_debuggee(cont);

    while (dbg_curr_process && WaitForDebugEvent(&de, INFINITE))
    {
        if (dbg_handle_debug_event(&de)) break;
    }
    if (!dbg_curr_process) return;
    dbg_interactiveP = TRUE;

    memory_get_current_pc(&addr);
    WINE_TRACE("Entering debugger     PC=0x%lx mode=%d count=%d\n",
               addr.Offset, dbg_curr_thread->exec_mode,
               dbg_curr_thread->exec_count);
}

static	unsigned        dbg_main_loop(HANDLE hFile)
{
    DEBUG_EVENT		de;

    if (dbg_curr_process)
        dbg_printf("WineDbg starting on pid 0x%lx\n", dbg_curr_pid);

    /* wait for first exception */
    while (WaitForDebugEvent(&de, INFINITE))
    {
        if (dbg_handle_debug_event(&de)) break;
    }
    switch (dbg_action_mode)
    {
    case automatic_mode:
        /* print some extra information */
        dbg_printf("Modules:\n");
        info_win32_module(0); /* print all modules */
        dbg_printf("Threads:\n");
        info_win32_threads();
        break;
    default:
        dbg_interactiveP = TRUE;
        parser_handle(hFile);
    }
    dbg_printf("WineDbg terminated on pid 0x%lx\n", dbg_curr_pid);

    return 0;
}

static	unsigned dbg_start_debuggee(LPSTR cmdLine)
{
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* FIXME: shouldn't need the CREATE_NEW_CONSOLE, but as usual CUI:s need it
     * while GUI:s don't
     */
    if (!CreateProcess(NULL, cmdLine, NULL, NULL,
                       FALSE, 
                       DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS|CREATE_NEW_CONSOLE,
                       NULL, NULL, &startup, &info))
    {
	dbg_printf("Couldn't start process '%s'\n", cmdLine);
	return FALSE;
    }
    if (!info.dwProcessId)
    {
        /* this happens when the program being run is not a Wine binary
         * (for example, a shell wrapper around a WineLib app)
         */
        /* Current fix: list running processes and let the user attach
         * to one of them (sic)
         * FIXME: implement a real fix => grab the process (from the
         * running processes) from its name
         */
        dbg_printf("Debuggee has been started (%s)\n"
                   "But WineDbg isn't attached to it. Maybe you're trying to debug a winelib wrapper ??\n"
                   "Try to attach to one of those processes:\n", cmdLine);
        /* FIXME: (HACK) we need some time before the wrapper executes the winelib app */
        Sleep(100);
        info_win32_processes();
        return TRUE;
    }
    dbg_curr_pid = info.dwProcessId;
    if (!(dbg_curr_process = dbg_add_process(dbg_curr_pid, 0))) return FALSE;

    return TRUE;
}

void	dbg_run_debuggee(const char* args)
{
    if (args)
    {
        WINE_FIXME("Re-running current program with %s as args is broken\n", args);
        return;
    }
    else 
    {
        DEBUG_EVENT     de;

	if (!dbg_last_cmd_line)
        {
	    dbg_printf("Cannot find previously used command line.\n");
	    return;
	}
	dbg_start_debuggee(dbg_last_cmd_line);
        while (dbg_curr_process && WaitForDebugEvent(&de, INFINITE))
        {
            if (dbg_handle_debug_event(&de)) break;
        }
        source_list_from_addr(NULL, 0);
    }
}

BOOL dbg_interrupt_debuggee(void)
{
    if (!dbg_process_list) return FALSE;
    /* FIXME: since we likely have a single process, signal the first process
     * in list
     */
    if (dbg_process_list->next) dbg_printf("Ctrl-C: only stopping the first process\n");
    else dbg_printf("Ctrl-C: stopping debuggee\n");
    dbg_process_list->continue_on_first_exception = FALSE;
    return DebugBreakProcess(dbg_process_list->handle);
}

static BOOL WINAPI ctrl_c_handler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        return dbg_interrupt_debuggee();
    }
    return FALSE;
}

static void dbg_init_console(void)
{
    /* set our control-C handler */
    SetConsoleCtrlHandler(ctrl_c_handler, TRUE);

    /* set our own title */
    SetConsoleTitle("Wine Debugger");
}

static int dbg_winedbg_usage(void)
{
    dbg_printf("Usage: winedbg [--command cmd|--file file|--auto] [--gdb [--no-start] [--with-xterm]] cmdline\n");
    return 1;
}

struct backend_cpu* be_cpu;
#ifdef __i386__
extern struct backend_cpu be_i386;
#elif __powerpc__
extern struct backend_cpu be_ppc;
#elif __ALPHA__
extern struct backend_cpu be_alpha;
#else
# error CPU unknown
#endif

int main(int argc, char** argv)
{
    DWORD	retv = 0;
    unsigned    gdb_flags = 0;
    HANDLE      hFile = INVALID_HANDLE_VALUE;

#ifdef __i386__
    be_cpu = &be_i386;
#elif __powerpc__
    be_cpu = &be_ppc;
#elif __ALPHA__
    be_cpu = &be_alpha;
#else
# error CPU unknown
#endif
    /* Initialize the output */
    dbg_houtput = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Initialize internal vars */
    if (!dbg_load_internal_vars()) return -1;

    /* parse options */
    while (argc > 1 && argv[1][0] == '-')
    {
        if (!strcmp(argv[1], "--command"))
        {
            char        path[MAX_PATH], file[MAX_PATH];
            DWORD       w;

            GetTempPath(sizeof(path), path);
            GetTempFileName(path, "WD", 0, file);
            argc--; argv++;
            hFile = CreateFileA(file, GENERIC_READ|GENERIC_WRITE|DELETE, FILE_SHARE_DELETE, 
                                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, 0);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                dbg_printf("Couldn't open temp file %s (%lu)\n", file, GetLastError());
                return 1;
            }
            WriteFile(hFile, argv[1], strlen(argv[1]), &w, 0);
            WriteFile(hFile, "\nquit\n", 6, &w, 0);
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

            argc--; argv++;
            continue;
        }
        if (!strcmp(argv[1], "--file"))
        {
            argc--; argv++;
            hFile = CreateFileA(argv[1], GENERIC_READ|DELETE, 0, 
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                dbg_printf("Couldn't open file %s (%lu)\n", argv[1], GetLastError());
                return 1;
            }
            argc--; argv++;
            continue;
        }
        if (!strcmp(argv[1], "--auto"))
        {
            if (dbg_action_mode != none_mode) return dbg_winedbg_usage();
            dbg_action_mode = automatic_mode;
            /* force some internal variables */
            DBG_IVAR(BreakOnDllLoad) = 0;
            argc--; argv++;
            dbg_houtput = GetStdHandle(STD_ERROR_HANDLE);
            continue;
        }
        if (!strcmp(argv[1], "--gdb"))
        {
            if (dbg_action_mode != none_mode) return dbg_winedbg_usage();
            dbg_action_mode = gdb_mode;
            argc--; argv++;
            continue;
        }
        if (strcmp(argv[1], "--no-start") == 0 && dbg_action_mode == gdb_mode)
        {
            gdb_flags |= 1;
            argc--; argv++; /* as we don't use argv[0] */
            continue;
        }
        if (strcmp(argv[1], "--with-xterm") == 0 && dbg_action_mode == gdb_mode)
        {
            gdb_flags |= 2;
            argc--; argv++; /* as we don't use argv[0] */
            continue;
        }
        return dbg_winedbg_usage();
    }

    if (dbg_action_mode == none_mode) dbg_action_mode = winedbg_mode;

    /* try the form <myself> pid */
    if (dbg_curr_pid == 0 && argc == 2)
    {
        char*   ptr;

        dbg_curr_pid = strtol(argv[1], &ptr, 10);
        if (dbg_curr_pid == 0 || ptr != argv[1] + strlen(argv[1]) ||
            !dbg_attach_debuggee(dbg_curr_pid, FALSE, FALSE))
            dbg_curr_pid = 0;
    }

    /* try the form <myself> pid evt (Win32 JIT debugger) */
    if (dbg_curr_pid == 0 && argc == 3)
    {
	HANDLE	hEvent;
	DWORD	pid;
        char*   ptr;

	if ((pid = strtol(argv[1], &ptr, 10)) != 0 && ptr != NULL &&
            (hEvent = (HANDLE)strtol(argv[2], &ptr, 10)) != 0 && ptr != NULL)
        {
	    if (!dbg_attach_debuggee(pid, TRUE, FALSE))
            {
		/* don't care about result */
		SetEvent(hEvent);
		goto leave;
	    }
	    if (!SetEvent(hEvent))
            {
		WINE_ERR("Invalid event handle: %p\n", hEvent);
		goto leave;
	    }
            CloseHandle(hEvent);
	    dbg_curr_pid = pid;
	}
    }

    if (dbg_curr_pid == 0 && argc > 1)
    {
	int	i, len;
	LPSTR	cmdLine;

	if (!(cmdLine = HeapAlloc(GetProcessHeap(), 0, len = 1))) goto oom_leave;
	cmdLine[0] = '\0';

	for (i = 1; i < argc; i++)
        {
	    len += strlen(argv[i]) + 1;
	    if (!(cmdLine = HeapReAlloc(GetProcessHeap(), 0, cmdLine, len)))
                goto oom_leave;
	    strcat(cmdLine, argv[i]);
	    cmdLine[len - 2] = ' ';
	    cmdLine[len - 1] = '\0';
	}

	if (!dbg_start_debuggee(cmdLine))
        {
	    dbg_printf("Couldn't start process '%s'\n", cmdLine);
	    goto leave;
	}
	dbg_last_cmd_line = cmdLine;
    }
    /* don't save local vars in gdb mode */
    if (dbg_action_mode == gdb_mode && dbg_curr_pid)
        return gdb_remote(gdb_flags);

    dbg_init_console();

    SymSetOptions((SymGetOptions() & ~(SYMOPT_UNDNAME)) |
                  SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_AUTO_PUBLICS);

    retv = dbg_main_loop(hFile);
    /* don't save modified variables in auto mode */
    if (dbg_action_mode != automatic_mode) dbg_save_internal_vars();

leave:
    return retv;

oom_leave:
    dbg_printf("Out of memory\n");
    return retv;
}
