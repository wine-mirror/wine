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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
 *      + ensure that all commands work as expected in minidump reload function
 *        (and reenable parser usager)
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

static struct dbg_process*      dbg_process_list = NULL;

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

unsigned         dbg_num_processes(void)
{
    struct dbg_process*	p;
    unsigned            num = 0;

    for (p = dbg_process_list; p; p = p->next)
	num++;
    return num;
}

struct dbg_process*     dbg_get_process(DWORD pid)
{
    struct dbg_process*	p;

    for (p = dbg_process_list; p; p = p->next)
	if (p->pid == pid) break;
    return p;
}

struct dbg_process*     dbg_get_process_h(HANDLE h)
{
    struct dbg_process*	p;

    for (p = dbg_process_list; p; p = p->next)
	if (p->handle == h) break;
    return p;
}

struct dbg_process*	dbg_add_process(const struct be_process_io* pio, DWORD pid, HANDLE h)
{
    struct dbg_process*	p;

    if ((p = dbg_get_process(pid)))
    {
        if (p->handle != 0)
        {
            WINE_ERR("Process (%04x) is already defined\n", pid);
        }
        else
        {
            p->handle = h;
            p->process_io = pio;
            p->imageName = NULL;
        }
        return p;
    }

    if (!(p = HeapAlloc(GetProcessHeap(), 0, sizeof(struct dbg_process)))) return NULL;
    p->handle = h;
    p->pid = pid;
    p->process_io = pio;
    p->pio_data = NULL;
    p->imageName = NULL;
    p->threads = NULL;
    p->continue_on_first_exception = FALSE;
    p->active_debuggee = FALSE;
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

struct mod_loader_info
{
    HANDLE              handle;
    IMAGEHLP_MODULE*    imh_mod;
};

static BOOL CALLBACK mod_loader_cb(PCSTR mod_name, ULONG base, PVOID ctx)
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

    snprintf(t->name, sizeof(t->name), "%04x", tid);

    t->next = p->threads;
    t->prev = NULL;
    if (p->threads) p->threads->prev = t;
    p->threads = t;

    return t;
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

static int dbg_winedbg_usage(BOOL advanced)
{
    if (advanced)
    {
    dbg_printf("Usage:\n"
               "   winedbg cmdline         launch process 'cmdline' (as if you were starting\n"
               "                           it with wine) and run WineDbg on it\n"
               "   winedbg <num>           attach to running process of pid <num> and run\n"
               "                           WineDbg on it\n"
               "   winedbg --gdb cmdline   launch process 'cmdline' (as if you were starting\n"
               "                           wine) and run gdb (proxied) on it\n"
               "   winedbg --gdb <num>     attach to running process of pid <num> and run\n"
               "                           gdb (proxied) on it\n"
               "   winedbg file.mdmp       reload the minidump file.mdmp into memory and run\n"
               "                           WineDbg on it\n"
               "   winedbg --help          prints advanced options\n");
    }
    else
        dbg_printf("Usage:\n\twinedbg [ [ --gdb ] [ prog-name [ prog-args ] | <num> | file.mdmp | --help ]\n");
    return -1;
}

struct backend_cpu* be_cpu;
#ifdef __i386__
extern struct backend_cpu be_i386;
#elif __powerpc__
extern struct backend_cpu be_ppc;
#elif __ALPHA__
extern struct backend_cpu be_alpha;
#elif __x86_64__
extern struct backend_cpu be_x86_64;
#else
# error CPU unknown
#endif

int main(int argc, char** argv)
{
    int 	        retv = 0;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    enum dbg_start      ds;

#ifdef __i386__
    be_cpu = &be_i386;
#elif __powerpc__
    be_cpu = &be_ppc;
#elif __ALPHA__
    be_cpu = &be_alpha;
#elif __x86_64__
    be_cpu = &be_x86_64;
#else
# error CPU unknown
#endif
    /* Initialize the output */
    dbg_houtput = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Initialize internal vars */
    if (!dbg_load_internal_vars()) return -1;

    /* as we don't care about exec name */
    argc--; argv++;

    if (argc && !strcmp(argv[0], "--help"))
        return dbg_winedbg_usage(TRUE);

    if (argc && !strcmp(argv[0], "--gdb"))
    {
        retv = gdb_main(argc, argv);
        if (retv == -1) dbg_winedbg_usage(FALSE);
        return retv;
    }
    dbg_init_console();

    SymSetOptions((SymGetOptions() & ~(SYMOPT_UNDNAME)) |
                  SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_AUTO_PUBLICS);

    if (argc && (!strcmp(argv[0], "--auto") || !strcmp(argv[0], "--minidump")))
    {
        /* force some internal variables */
        DBG_IVAR(BreakOnDllLoad) = 0;
        dbg_houtput = GetStdHandle(STD_ERROR_HANDLE);
        switch (dbg_active_auto(argc, argv))
        {
        case start_ok:          return 0;
        case start_error_parse: return dbg_winedbg_usage(FALSE);
        case start_error_init:  return -1;
        }
    }
    /* parse options */
    while (argc > 0 && argv[0][0] == '-')
    {
        if (!strcmp(argv[0], "--command"))
        {
            argc--; argv++;
            hFile = parser_generate_command_file(argv[0], NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                dbg_printf("Couldn't open temp file (%u)\n", GetLastError());
                return 1;
            }
            argc--; argv++;
            continue;
        }
        if (!strcmp(argv[0], "--file"))
        {
            argc--; argv++;
            hFile = CreateFileA(argv[0], GENERIC_READ|DELETE, 0, 
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                dbg_printf("Couldn't open file %s (%u)\n", argv[0], GetLastError());
                return 1;
            }
            argc--; argv++;
            continue;
        }
        if (!strcmp(argv[0], "--"))
        {
            argc--; argv++;
            break;
        }
        return dbg_winedbg_usage(FALSE);
    }
    if (!argc) ds = start_ok;
    else if ((ds = dbg_active_attach(argc, argv)) == start_error_parse &&
             (ds = minidump_reload(argc, argv)) == start_error_parse)
        ds = dbg_active_launch(argc, argv);
    switch (ds)
    {
    case start_ok:              break;
    case start_error_parse:     return dbg_winedbg_usage(FALSE);
    case start_error_init:      return -1;
    }

    if (dbg_curr_process)
    {
        dbg_printf("WineDbg starting on pid %04x\n", dbg_curr_pid);
        if (dbg_curr_process->active_debuggee) dbg_active_wait_for_first_exception();
    }

    dbg_interactiveP = TRUE;
    parser_handle(hFile);

    while (dbg_process_list)
        dbg_process_list->process_io->close_process(dbg_process_list, FALSE);

    dbg_save_internal_vars();

    return 0;
}
