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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debugger.h"

#include "winternl.h"
#include "wine/debug.h"

/* TODO list:
 *
 * - minidump
 *      + ensure that all commands work as expected in minidump reload function
 *        (and re-enable parser usage)
 * - CPU adherence
 *      + we always assume the stack grows as on i386 (i.e. downwards)
 * - UI
 *      + re-enable the limited output (depth of structure printing and number of
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
 *      + all computations should be made on 64bit
 *              o bitfield spreading on more bytes than dbg_lgint_t isn't supported
 *                (can happen on 128bit integers, of an ELF build...)
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
 */

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

struct dbg_process*	dbg_curr_process = NULL;
struct dbg_thread*	dbg_curr_thread = NULL;
DWORD	                dbg_curr_tid = 0;
DWORD	                dbg_curr_pid = 0;
dbg_ctx_t               dbg_context;
BOOL    	        dbg_interactiveP = FALSE;
HANDLE                  dbg_houtput = 0;

static struct list      dbg_process_list = LIST_INIT(dbg_process_list);

struct dbg_internal_var         dbg_internal_vars[DBG_IV_LAST];

static void dbg_outputA(const char* buffer, int len)
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

int WINAPIV dbg_printf(const char* format, ...)
{
    static    char	buf[4*1024];
    va_list valist;
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
            if (!RegQueryValueExA(hkey, dbg_internal_vars[i].name, 0,
                                  &type, (LPBYTE)&val, &count))
                dbg_internal_vars[i].val = val;
            dbg_internal_vars[i].pval = &dbg_internal_vars[i].val;
        }
    }
    RegCloseKey(hkey);

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
        /* FIXME: type should be inferred from basic type -if any- of intvar */
        if (dbg_internal_vars[i].pval == &dbg_internal_vars[i].val)
        {
            DWORD val = dbg_internal_vars[i].val;
            RegSetValueExA(hkey, dbg_internal_vars[i].name, 0, REG_DWORD, (BYTE *)&val, sizeof(val));
        }
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
    for (div = dbg_curr_process->be_cpu->context_vars; div->name; div++)
    {
	if (!strcasecmp(div->name, name))
        {
            struct dbg_internal_var*    ret = (void*)lexeme_alloc_size(sizeof(*ret));
            /* relocate register's field against current context */
            *ret = *div;
            ret->pval = (char*)&dbg_context + (DWORD_PTR)div->pval;
            return ret;
        }
    }

    return NULL;
}

unsigned         dbg_num_processes(void)
{
    return list_count(&dbg_process_list);
}

struct dbg_process*     dbg_get_process(DWORD pid)
{
    struct dbg_process*	p;

    LIST_FOR_EACH_ENTRY(p, &dbg_process_list, struct dbg_process, entry)
	if (p->pid == pid) return p;
    return NULL;
}

struct dbg_process*     dbg_get_process_h(HANDLE h)
{
    struct dbg_process*	p;

    LIST_FOR_EACH_ENTRY(p, &dbg_process_list, struct dbg_process, entry)
	if (p->handle == h) return p;
    return NULL;
}

#ifdef __i386__
extern struct backend_cpu be_i386;
#elif defined(__x86_64__)
extern struct backend_cpu be_i386;
extern struct backend_cpu be_x86_64;
#elif defined(__arm__) && !defined(__ARMEB__)
extern struct backend_cpu be_arm;
#elif defined(__aarch64__) && !defined(__AARCH64EB__)
extern struct backend_cpu be_arm64;
#else
# error CPU unknown
#endif

struct dbg_process*	dbg_add_process(const struct be_process_io* pio, DWORD pid, HANDLE h)
{
    struct dbg_process*	p;
    BOOL wow64;

    if ((p = dbg_get_process(pid)))
        return p;

    if (!h)
        h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (!(p = malloc(sizeof(struct dbg_process)))) return NULL;
    p->handle = h;
    p->pid = pid;
    p->process_io = pio;
    p->pio_data = NULL;
    p->imageName = NULL;
    list_init(&p->threads);
    list_init(&p->modules);
    p->event_on_first_exception = NULL;
    p->active_debuggee = FALSE;
    p->next_bp = 1;  /* breakpoint 0 is reserved for step-over */
    memset(p->bp, 0, sizeof(p->bp));
    p->delayed_bp = NULL;
    p->num_delayed_bp = 0;
    p->source_ofiles = NULL;
    p->search_path = NULL;
    p->source_current_file[0] = '\0';
    p->source_start_line = -1;
    p->source_end_line = -1;
    p->data_model = NULL;
    p->synthetized_types = NULL;
    p->num_synthetized_types = 0;

    list_add_head(&dbg_process_list, &p->entry);

    IsWow64Process(h, &wow64);

#ifdef __i386__
    p->be_cpu = &be_i386;
#elif defined(__x86_64__)
    p->be_cpu = wow64 ? &be_i386 : &be_x86_64;
#elif defined(__arm__) && !defined(__ARMEB__)
    p->be_cpu = &be_arm;
#elif defined(__aarch64__) && !defined(__AARCH64EB__)
    p->be_cpu = &be_arm64;
#else
# error CPU unknown
#endif
    return p;
}

void dbg_set_process_name(struct dbg_process* p, const WCHAR* imageName)
{
    assert(p->imageName == NULL);
    if (imageName) p->imageName = wcsdup(imageName);
}

void dbg_del_process(struct dbg_process* p)
{
    struct dbg_thread*  t;
    struct dbg_thread*  t2;
    struct dbg_module*  mod;
    struct dbg_module*  mod2;
    int	i;

    LIST_FOR_EACH_ENTRY_SAFE(t, t2, &p->threads, struct dbg_thread, entry)
        dbg_del_thread(t);

    LIST_FOR_EACH_ENTRY_SAFE(mod, mod2, &p->modules, struct dbg_module, entry)
        dbg_del_module(mod);

    for (i = 0; i < p->num_delayed_bp; i++)
        if (p->delayed_bp[i].is_symbol)
            free(p->delayed_bp[i].u.symbol.name);

    free(p->delayed_bp);
    source_nuke_path(p);
    source_free_files(p);
    list_remove(&p->entry);
    if (p == dbg_curr_process) dbg_curr_process = NULL;
    if (p->event_on_first_exception) CloseHandle(p->event_on_first_exception);
    free((char*)p->imageName);
    free(p->synthetized_types);
    free(p);
}

/******************************************************************
 *		dbg_init
 *
 * Initializes the dbghelp library, and also sets the application directory
 * as a place holder for symbol searches.
 */
BOOL dbg_init(HANDLE hProc, const WCHAR* in, BOOL invade)
{
    BOOL        ret;

    ret = SymInitialize(hProc, NULL, invade);
    if (ret && in)
    {
        const WCHAR*    last;

        for (last = in + lstrlenW(in) - 1; last >= in; last--)
        {
            if (*last == '/' || *last == '\\')
            {
                WCHAR*  tmp;
                tmp = malloc((1024 + 1 + (last - in) + 1) * sizeof(WCHAR));
                if (tmp && SymGetSearchPathW(hProc, tmp, 1024))
                {
                    WCHAR*      x = tmp + lstrlenW(tmp);

                    *x++ = ';';
                    memcpy(x, in, (last - in) * sizeof(WCHAR));
                    x[last - in] = '\0';
                    ret = SymSetSearchPathW(hProc, tmp);
                }
                else ret = FALSE;
                free(tmp);
                break;
            }
        }
    }
    return ret;
}

BOOL dbg_load_module(HANDLE hProc, HANDLE hFile, const WCHAR* name, DWORD_PTR base, DWORD size)
{
    struct dbg_process* pcs = dbg_get_process_h(hProc);
    struct dbg_module* mod;
    IMAGEHLP_MODULEW64 info;
    HANDLE hMap;
    void* image;

    if (!pcs) return FALSE;
    mod = malloc(sizeof(struct dbg_module));
    if (!mod) return FALSE;
    if (!SymLoadModuleExW(hProc, hFile, name, NULL, base, size, NULL, 0))
    {
        free(mod);
        return FALSE;
    }
    mod->base = base;
    list_add_head(&pcs->modules, &mod->entry);

    mod->tls_index_offset = 0;
    if ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)))
    {
        if ((image = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)))
        {
            IMAGE_NT_HEADERS* nth = RtlImageNtHeader(image);
            const void* tlsdir;
            ULONG sz;

            tlsdir = RtlImageDirectoryEntryToData(image, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &sz);
            switch (nth->OptionalHeader.Magic)
            {
            case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
                if (tlsdir && sz >= sizeof(IMAGE_TLS_DIRECTORY32))
                    mod->tls_index_offset = (const char*)tlsdir - (const char*)image +
                        offsetof(IMAGE_TLS_DIRECTORY32, AddressOfIndex);
                break;
            case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
                if (tlsdir && sz >= sizeof(IMAGE_TLS_DIRECTORY64))
                    mod->tls_index_offset = (const char*)tlsdir - (const char*)image +
                        offsetof(IMAGE_TLS_DIRECTORY64, AddressOfIndex);
                break;
            }
            UnmapViewOfFile(image);
        }
        CloseHandle(hMap);
    }
    info.SizeOfStruct = sizeof(info);
    if (SymGetModuleInfoW64(hProc, base, &info))
    if (info.PdbUnmatched || info.DbgUnmatched)
        dbg_printf("Loaded unmatched debug information for %s\n", wine_dbgstr_w(name));

    return TRUE;
}

void dbg_del_module(struct dbg_module* mod)
{
    list_remove(&mod->entry);
    free(mod);
}

struct dbg_module* dbg_get_module(struct dbg_process* pcs, DWORD_PTR base)
{
    struct dbg_module* mod;

    if (!pcs)
        return NULL;
    LIST_FOR_EACH_ENTRY(mod, &pcs->modules, struct dbg_module, entry)
        if (mod->base == base)
            return mod;
    return NULL;
}

BOOL dbg_unload_module(struct dbg_process* pcs, DWORD_PTR base)
{
    struct dbg_module* mod = dbg_get_module(pcs, base);

    types_unload_module(pcs, base);
    SymUnloadModule64(pcs->handle, base);
    dbg_del_module(mod);

    return !!mod;
}

struct dbg_thread* dbg_get_thread(struct dbg_process* p, DWORD tid)
{
    struct dbg_thread*	t;

    if (!p) return NULL;
    LIST_FOR_EACH_ENTRY(t, &p->threads, struct dbg_thread, entry)
	if (t->tid == tid) return t;
    return NULL;
}

struct dbg_thread* dbg_add_thread(struct dbg_process* p, DWORD tid,
                                  HANDLE h, void* teb)
{
    struct dbg_thread*	t = malloc(sizeof(struct dbg_thread));

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
    t->name[0] = '\0';
    t->in_exception = FALSE;
    t->frames = NULL;
    t->num_frames = 0;
    t->curr_frame = -1;
    t->addr_mode = AddrModeFlat;
    t->suspended = FALSE;

    list_add_head(&p->threads, &t->entry);

    return t;
}

void dbg_del_thread(struct dbg_thread* t)
{
    free(t->frames);
    list_remove(&t->entry);
    if (t == dbg_curr_thread) dbg_curr_thread = NULL;
    free(t);
}

void dbg_set_option(const char* option, const char* val)
{
    if (!strcasecmp(option, "module_load_mismatched"))
    {
        DWORD   opt = SymGetOptions();
        if (!val)
            dbg_printf("Option: module_load_mismatched %s\n", opt & SYMOPT_LOAD_ANYTHING ? "true" : "false");
        else if (!strcasecmp(val, "true"))      opt |= SYMOPT_LOAD_ANYTHING;
        else if (!strcasecmp(val, "false"))     opt &= ~SYMOPT_LOAD_ANYTHING;
        else
        {
            dbg_printf("Syntax: module_load_mismatched [true|false]\n");
            return;
        }
        SymSetOptions(opt);
    }
    else if (!strcasecmp(option, "symbol_picker"))
    {
        if (!val)
            dbg_printf("Option: symbol_picker %s\n",
                       symbol_current_picker == symbol_picker_interactive ? "interactive" : "scoped");
        else if (!strcasecmp(val, "interactive"))
            symbol_current_picker = symbol_picker_interactive;
        else if (!strcasecmp(val, "scoped"))
            symbol_current_picker = symbol_picker_scoped;
        else
        {
            dbg_printf("Syntax: symbol_picker [interactive|scoped]\n");
            return;
        }
    }
    else if (!strcasecmp(option, "data_model"))
    {
        if (!dbg_curr_process)
        {
            dbg_printf("Not attached to a process\n");
            return;
        }
        if (!val)
        {
            const char* model = "";
            if      (dbg_curr_process->data_model == NULL)             model = "auto";
            else if (dbg_curr_process->data_model == ilp32_data_model) model = "ilp32";
            else if (dbg_curr_process->data_model == llp64_data_model) model = "llp64";
            else if (dbg_curr_process->data_model == lp64_data_model)  model = "lp64";
            dbg_printf("Option: data_model %s\n", model);
        }
        else if (!strcasecmp(val, "auto"))  dbg_curr_process->data_model = NULL;
        else if (!strcasecmp(val, "ilp32")) dbg_curr_process->data_model = ilp32_data_model;
        else if (!strcasecmp(val, "llp64")) dbg_curr_process->data_model = llp64_data_model;
        else if (!strcasecmp(val, "lp64"))  dbg_curr_process->data_model = lp64_data_model;
        else
            dbg_printf("Unknown data model %s\n", val);
    }
    else dbg_printf("Unknown option '%s'\n", option);
}

BOOL dbg_interrupt_debuggee(void)
{
    struct dbg_process* p;
    if (list_empty(&dbg_process_list)) return FALSE;
    /* FIXME: since we likely have a single process, signal the first process
     * in list
     */
    p = LIST_ENTRY(list_head(&dbg_process_list), struct dbg_process, entry);
    if (list_next(&dbg_process_list, &p->entry)) dbg_printf("Ctrl-C: only stopping the first process\n");
    else dbg_printf("Ctrl-C: stopping debuggee\n");
    if (p->event_on_first_exception)
    {
        SetEvent(p->event_on_first_exception);
        CloseHandle(p->event_on_first_exception);
        p->event_on_first_exception = NULL;
    }
    return DebugBreakProcess(p->handle);
}

static BOOL WINAPI ctrl_c_handler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        return dbg_interrupt_debuggee();
    }
    return FALSE;
}

void dbg_init_console(void)
{
    /* set the output handle */
    dbg_houtput = GetStdHandle(STD_OUTPUT_HANDLE);

    /* set our control-C handler */
    SetConsoleCtrlHandler(ctrl_c_handler, TRUE);

    /* set our own title */
    SetConsoleTitleA("Wine Debugger");
}

static int dbg_winedbg_usage(BOOL advanced)
{
    if (advanced)
    {
    dbg_printf("Usage:\n"
               "   winedbg <cmdline>       launch process <cmdline> (as if you were starting\n"
               "                           it with wine) and run WineDbg on it\n"
               "   winedbg <num>           attach to running process of wpid <num> and run\n"
               "                           WineDbg on it\n"
               "   winedbg --gdb <cmdline> launch process <cmdline> (as if you were starting\n"
               "                           wine) and run gdb (proxied) on it\n"
               "   winedbg --gdb <num>     attach to running process of wpid <num> and run\n"
               "                           gdb (proxied) on it\n"
               "   winedbg <file.mdmp>     reload the minidump <file.mdmp> into memory and run\n"
               "                           WineDbg on it\n"
               "   winedbg --help          prints advanced options\n");
    }
    else
        dbg_printf("Usage:\n\twinedbg [ [ --gdb ] [ <prog-name> [ <prog-args> ] | <num> | <file.mdmp> | --help ]\n");
    return 0;
}

void dbg_start_interactive(const char* filename, HANDLE hFile)
{
    struct dbg_process* p;
    struct dbg_process* p2;

    if (dbg_curr_process)
    {
        dbg_printf("WineDbg starting on pid %04lx\n", dbg_curr_pid);
        if (dbg_curr_process->active_debuggee) dbg_active_wait_for_first_exception();
    }

    dbg_interactiveP = TRUE;
    parser_handle(filename, hFile);

    LIST_FOR_EACH_ENTRY_SAFE(p, p2, &dbg_process_list, struct dbg_process, entry)
        p->process_io->close_process(p, FALSE);

    dbg_save_internal_vars();
}

static LONG CALLBACK top_filter( EXCEPTION_POINTERS *ptr )
{
    dbg_printf( "winedbg: Internal crash at %p\n", ptr->ExceptionRecord->ExceptionAddress );
    return EXCEPTION_EXECUTE_HANDLER;
}

static void restart_if_wow64(void)
{
    BOOL is_wow64;

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        WCHAR filename[MAX_PATH];
        void *redir;
        DWORD exit_code;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        GetSystemDirectoryW( filename, MAX_PATH );
        lstrcatW( filename, L"\\winedbg.exe" );

        Wow64DisableWow64FsRedirection( &redir );
        if (CreateProcessW( filename, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            WINE_TRACE( "restarting %s\n", wine_dbgstr_w(filename) );
            SetConsoleCtrlHandler( NULL, TRUE ); /* Ignore ^C */
            WaitForSingleObject( pi.hProcess, INFINITE );
            GetExitCodeProcess( pi.hProcess, &exit_code );
            ExitProcess( exit_code );
        }
        else WINE_ERR( "failed to restart 64-bit %s, err %ld\n", wine_dbgstr_w(filename), GetLastError() );
        Wow64RevertWow64FsRedirection( redir );
    }
}

int main(int argc, char** argv)
{
    int 	        retv = 0;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    enum dbg_start      ds;
    const char*         filename = NULL;

    /* Initialize the output */
    dbg_houtput = GetStdHandle(STD_OUTPUT_HANDLE);

    SetUnhandledExceptionFilter( top_filter );

    /* Initialize internal vars */
    if (!dbg_load_internal_vars()) return -1;

    /* as we don't care about exec name */
    argc--; argv++;

    if (argc && !strcmp(argv[0], "--help"))
        return dbg_winedbg_usage(TRUE);

    if (argc && !strcmp(argv[0], "--gdb"))
    {
        restart_if_wow64();
        retv = gdb_main(argc, argv);
        if (retv == -1) dbg_winedbg_usage(FALSE);
        return retv;
    }
    dbg_init_console();

    SymSetOptions((SymGetOptions() & ~(SYMOPT_UNDNAME)) |
                  SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_AUTO_PUBLICS |
                  SYMOPT_INCLUDE_32BIT_MODULES);

    if (argc && !strcmp(argv[0], "--auto"))
    {
        switch (dbg_active_auto(argc, argv))
        {
        case start_ok:          return 0;
        case start_error_parse: return dbg_winedbg_usage(FALSE);
        case start_error_init:  return -1;
        }
    }
    if (argc && !strcmp(argv[0], "--minidump"))
    {
        switch (dbg_active_minidump(argc, argv))
        {
        case start_ok:          return 0;
        case start_error_parse: return dbg_winedbg_usage(FALSE);
        case start_error_init:  return -1;
        }
    }
    /* parse options */
    while (argc > 0 && argv[0][0] == '-')
    {
        if (!strcmp(argv[0], "--command") && argc > 1)
        {
            argc--; argv++;
            hFile = parser_generate_command_file(argv[0], NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                dbg_printf("Couldn't open temp file (%lu)\n", GetLastError());
                return 1;
            }
            argc--; argv++;
            continue;
        }
        if (!strcmp(argv[0], "--file") && argc > 1)
        {
            argc--; argv++;
            filename = argv[0];
            hFile = CreateFileA(argv[0], GENERIC_READ|DELETE, 0, 
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                dbg_printf("Couldn't open file %s (%lu)\n", argv[0], GetLastError());
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

    restart_if_wow64();

    dbg_start_interactive(filename, hFile);

    return 0;
}
