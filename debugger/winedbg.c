/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/* Wine internal debugger
 * Interface to Windows debugger API
 * Eric Pouech (c) 2000
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debugger.h"

#include "thread.h"
#include "process.h"
#include "file.h"
#include "wincon.h"
#include "wingdi.h"
#include "winuser.h"

#include "winreg.h"

#ifdef DBG_need_heap
HANDLE dbg_heap = 0;
#endif

DBG_PROCESS*	DEBUG_CurrProcess = NULL;
DBG_THREAD*	DEBUG_CurrThread = NULL;
DWORD		DEBUG_CurrTid;
DWORD		DEBUG_CurrPid;
CONTEXT		DEBUG_context;
int 		curr_frame = 0;
static char*	DEBUG_LastCmdLine = NULL;

static DBG_PROCESS* DEBUG_ProcessList = NULL;
DBG_INTVAR DEBUG_IntVars[DBG_IV_LAST];

void	DEBUG_Output(int chn, const char* buffer, int len)
{
    if (DBG_IVAR(ConChannelMask) & chn)
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, len, NULL, NULL);
    if (DBG_IVAR(StdChannelMask) & chn)
	fwrite(buffer, len, 1, stderr);
}

int	DEBUG_Printf(int chn, const char* format, ...)
{
    char	buf[1024];
    va_list 	valist;
    int		len;

    va_start(valist, format);
    len = wvsnprintf(buf, sizeof(buf), format, valist);
    va_end(valist);

    if (len <= -1) {
	len = sizeof(buf) - 1;
	buf[len] = 0;
	buf[len - 1] = buf[len - 2] = buf[len - 3] = '.';
    }
    DEBUG_Output(chn, buf, len);
    return len;
}

static	BOOL DEBUG_IntVarsRW(int read)
{
    HKEY	hkey;
    DWORD 	type = REG_DWORD;
    DWORD	val;
    DWORD 	count = sizeof(val);
    int		i;
    DBG_INTVAR* div = DEBUG_IntVars;

    if (read) {
/* initializes internal vars table */
#define  INTERNAL_VAR(_var,_val,_ref,_typ) 			\
        div->val = _val; div->name = #_var; div->pval = _ref;	\
        div->type = _typ; div++;
#include "intvar.h"
#undef   INTERNAL_VAR
    }

    if (RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\WineDbg", &hkey)) {
	/* since the IVars are not yet setup, DEBUG_Printf doesn't work,
	 * so don't use it */
	fprintf(stderr, "Cannot create WineDbg key in registry\n");
	return FALSE;
    }

    for (i = 0; i < DBG_IV_LAST; i++) {
	if (read) {
	    if (!DEBUG_IntVars[i].pval) {
		if (!RegQueryValueEx(hkey, DEBUG_IntVars[i].name, 0, 
				     &type, (LPSTR)&val, &count))
		    DEBUG_IntVars[i].val = val;
		DEBUG_IntVars[i].pval = &DEBUG_IntVars[i].val;
	    } else {
		*DEBUG_IntVars[i].pval = 0;
	    }
	} else {
	    /* FIXME: type should be infered from basic type -if any- of intvar */
	    if (DEBUG_IntVars[i].pval == &DEBUG_IntVars[i].val)
		RegSetValueEx(hkey, DEBUG_IntVars[i].name, 0, 
			      type, (LPCVOID)DEBUG_IntVars[i].pval, count);
	}
    }
    RegCloseKey(hkey);
    return TRUE;
}

DBG_INTVAR*	DEBUG_GetIntVar(const char* name)
{
    int		i;

    for (i = 0; i < DBG_IV_LAST; i++) {
	if (!strcmp(DEBUG_IntVars[i].name, name))
	    return &DEBUG_IntVars[i];
    }
    return NULL;
}
		       
static WINE_EXCEPTION_FILTER(wine_dbg)
{
    DEBUG_Printf(DBG_CHN_MESG, "\nwine_dbg: Exception (%lx) inside debugger, continuing...\n", GetExceptionCode());
    DEBUG_ExternalDebugger();
    return EXCEPTION_EXECUTE_HANDLER;
}

static	DBG_PROCESS*	DEBUG_GetProcess(DWORD pid)
{
    DBG_PROCESS*	p;
    
    for (p = DEBUG_ProcessList; p; p = p->next)
	if (p->pid == pid) break;
    return p;
}

static	DBG_PROCESS*	DEBUG_AddProcess(DWORD pid, HANDLE h)
{
    DBG_PROCESS*	p = DBG_alloc(sizeof(DBG_PROCESS));
    if (!p)
	return NULL;
    p->handle = h;
    p->pid = pid;
    p->threads = NULL;
    p->num_threads = 0;
    p->continue_on_first_exception = FALSE;
    p->modules = NULL;
    p->next_index = 0;
    p->dbg_hdr_addr = 0;

    p->next = DEBUG_ProcessList;
    p->prev = NULL;
    if (DEBUG_ProcessList) DEBUG_ProcessList->prev = p;
    DEBUG_ProcessList = p;
    return p;
}

static	void			DEBUG_DelThread(DBG_THREAD* p);

static	void			DEBUG_DelProcess(DBG_PROCESS* p)
{
    if (p->threads != NULL) {
	DEBUG_Printf(DBG_CHN_ERR, "Shouldn't happen\n");
	while (p->threads) DEBUG_DelThread(p->threads);
    }
    if (p->prev) p->prev->next = p->next;
    if (p->next) p->next->prev = p->prev;
    if (p == DEBUG_ProcessList) DEBUG_ProcessList = p->next;
    if (p == DEBUG_CurrProcess) DEBUG_CurrProcess = NULL;
    DBG_free(p);
}

static	void			DEBUG_InitCurrProcess(void)
{
}

static BOOL DEBUG_ProcessGetString(char* buffer, int size, HANDLE hp, LPSTR addr)
{
    DWORD sz;
    *(WCHAR*)buffer = 0;
    return (addr && ReadProcessMemory(hp, addr, buffer, size, &sz));
}

static BOOL DEBUG_ProcessGetStringIndirect(char* buffer, int size, HANDLE hp, LPVOID addr)
{
    LPVOID	ad;
    DWORD	sz;
    
    if (   addr 
	&& ReadProcessMemory(hp, addr, &ad, sizeof(ad), &sz) 
	&& sz == sizeof(ad) 
        && ad 
        && ReadProcessMemory(hp, ad, buffer, size, &sz))
	return TRUE;
    *(WCHAR*)buffer = 0;
    return FALSE;
}

static	DBG_THREAD*	DEBUG_GetThread(DBG_PROCESS* p, DWORD tid)
{
    DBG_THREAD*	t;
    
    for (t = p->threads; t; t = t->next)
	if (t->tid == tid) break;
    return t;
}

static	DBG_THREAD*	DEBUG_AddThread(DBG_PROCESS* p, DWORD tid, 
					HANDLE h, LPVOID start, LPVOID teb)
{
    DBG_THREAD*	t = DBG_alloc(sizeof(DBG_THREAD));
    if (!t)
	return NULL;
    
    t->handle = h;
    t->tid = tid;
    t->start = start;
    t->teb = teb;
    t->process = p;
    t->wait_for_first_exception = 0;
    t->dbg_exec_mode = EXEC_CONT;
    t->dbg_exec_count = 0;

    p->num_threads++;
    t->next = p->threads;
    t->prev = NULL;
    if (p->threads) p->threads->prev = t;
    p->threads = t;

    return t;
}

static	void			DEBUG_InitCurrThread(void)
{
    if (DEBUG_CurrThread->start) {
	if (DEBUG_CurrThread->process->num_threads == 1 || 
	    DBG_IVAR(BreakAllThreadsStartup)) {
	    DBG_VALUE	value;
	    
	    DEBUG_SetBreakpoints(FALSE);
	    value.type = NULL;
	    value.cookie = DV_TARGET;
	    value.addr.seg = 0;
	    value.addr.off = (DWORD)DEBUG_CurrThread->start;
	    DEBUG_AddBreakpoint(&value, NULL);
	    DEBUG_SetBreakpoints(TRUE);
	}
    } else {
	DEBUG_CurrThread->wait_for_first_exception = 1;
    }
}

static	void			DEBUG_DelThread(DBG_THREAD* t)
{
    if (t->prev) t->prev->next = t->next;
    if (t->next) t->next->prev = t->prev;
    if (t == t->process->threads) t->process->threads = t->next;
    t->process->num_threads--;
    if (t == DEBUG_CurrThread) DEBUG_CurrThread = NULL;
    DBG_free(t);
}

BOOL				DEBUG_Attach(DWORD pid, BOOL cofe)
{
    if (!(DEBUG_CurrProcess = DEBUG_AddProcess(pid, 0))) return FALSE;

    if (!DebugActiveProcess(pid)) {
	DEBUG_Printf(DBG_CHN_ERR, "Can't attach process %ld: %ld\n", 
		     pid, GetLastError());
	return FALSE;
    }
    DEBUG_CurrProcess->continue_on_first_exception = cofe;
    return TRUE;
}

static  BOOL	DEBUG_ExceptionProlog(BOOL is_debug, BOOL force, DWORD code)
{
    DBG_ADDR	addr;
    int		newmode;

    DEBUG_GetCurrentAddress(&addr);
    DEBUG_SuspendExecution();

    if (!is_debug) {
#ifdef __i386__
        if (DEBUG_IsSelectorSystem(DEBUG_context.SegCs))
            DEBUG_Printf(DBG_CHN_MESG, " in 32-bit code (0x%08lx).\n", addr.off);
        else
            DEBUG_Printf(DBG_CHN_MESG, " in 16-bit code (%04x:%04lx).\n",
			 LOWORD(addr.seg), addr.off);
#else
        DEBUG_Printf(DBG_CHN_MESG, " (0x%08lx).\n", addr.off);
#endif
    }

    DEBUG_LoadEntryPoints("Loading new modules symbols:\n");

    if (!force && is_debug && 
	DEBUG_ShouldContinue(code, 
			     DEBUG_CurrThread->dbg_exec_mode, 
			     &DEBUG_CurrThread->dbg_exec_count))
	return FALSE;

#ifdef __i386__
    switch (newmode = DEBUG_GetSelectorType(addr.seg)) {
    case 16: case 32: break;
    default: DEBUG_Printf(DBG_CHN_MESG, "Bad CS (%ld)\n", addr.seg); newmode = 32;
    }
#else
    newmode = 32;
#endif
    if (newmode != DEBUG_CurrThread->dbg_mode)
	DEBUG_Printf(DBG_CHN_MESG,"In %d bit mode.\n", DEBUG_CurrThread->dbg_mode = newmode);

    DEBUG_DoDisplay();

    if (is_debug || force) {
	/*
	 * Do a quiet backtrace so that we have an idea of what the situation
	 * is WRT the source files.
	 */
	DEBUG_BackTrace(FALSE);
    } else {
	/* This is a real crash, dump some info */
	DEBUG_InfoRegisters();
	DEBUG_InfoStack();
#ifdef __i386__
	if (DEBUG_CurrThread->dbg_mode == 16) {
	    DEBUG_InfoSegments(DEBUG_context.SegDs >> 3, 1);
	    if (DEBUG_context.SegEs != DEBUG_context.SegDs)
		DEBUG_InfoSegments(DEBUG_context.SegEs >> 3, 1);
	}
	DEBUG_InfoSegments(DEBUG_context.SegFs >> 3, 1);
#endif
	DEBUG_BackTrace(TRUE);
    }

    if (!is_debug ||
	(DEBUG_CurrThread->dbg_exec_mode == EXEC_STEPI_OVER) ||
	(DEBUG_CurrThread->dbg_exec_mode == EXEC_STEPI_INSTR)) {
	/* Show where we crashed */
	curr_frame = 0;
	DEBUG_PrintAddress( &addr, DEBUG_CurrThread->dbg_mode, TRUE );
	DEBUG_Printf(DBG_CHN_MESG,":  ");
	DEBUG_Disasm( &addr, TRUE );
	DEBUG_Printf( DBG_CHN_MESG, "\n" );
    }
    return TRUE;
}

static  DWORD	DEBUG_ExceptionEpilog(void)
{
    DEBUG_CurrThread->dbg_exec_mode = DEBUG_RestartExecution(DEBUG_CurrThread->dbg_exec_mode, 
							     DEBUG_CurrThread->dbg_exec_count);
    /*
     * This will have gotten absorbed into the breakpoint info
     * if it was used.  Otherwise it would have been ignored.
     * In any case, we don't mess with it any more.
     */
    if (DEBUG_CurrThread->dbg_exec_mode == EXEC_CONT || DEBUG_CurrThread->dbg_exec_mode == EXEC_PASS)
	DEBUG_CurrThread->dbg_exec_count = 0;
    
    return (DEBUG_CurrThread->dbg_exec_mode == EXEC_PASS) ? DBG_EXCEPTION_NOT_HANDLED : DBG_CONTINUE;
}

static	BOOL	DEBUG_HandleException(EXCEPTION_RECORD *rec, BOOL first_chance, BOOL force, LPDWORD cont)
{
    BOOL	is_debug = FALSE;
    BOOL	ret = TRUE;

    *cont = DBG_CONTINUE;
    if (first_chance && !force && !DBG_IVAR(BreakOnFirstChance)) return TRUE;

    switch (rec->ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
        is_debug = TRUE;
        break;
    }

    if (!is_debug)
    {
        /* print some infos */
        DEBUG_Printf(DBG_CHN_MESG, "%s: ",
		      first_chance ? "First chance exception" : "Unhandled exception");
        switch (rec->ExceptionCode)
        {
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            DEBUG_Printf(DBG_CHN_MESG, "divide by zero");
            break;
        case EXCEPTION_INT_OVERFLOW:
            DEBUG_Printf(DBG_CHN_MESG, "overflow");
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            DEBUG_Printf(DBG_CHN_MESG, "array bounds ");
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            DEBUG_Printf(DBG_CHN_MESG, "illegal instruction");
            break;
        case EXCEPTION_STACK_OVERFLOW:
            DEBUG_Printf(DBG_CHN_MESG, "stack overflow");
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            DEBUG_Printf(DBG_CHN_MESG, "priviledged instruction");
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            if (rec->NumberParameters == 2)
                DEBUG_Printf(DBG_CHN_MESG, "page fault on %s access to 0x%08lx", 
			      rec->ExceptionInformation[0] ? "write" : "read",
			      rec->ExceptionInformation[1]);
            else
                DEBUG_Printf(DBG_CHN_MESG, "page fault");
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            DEBUG_Printf(DBG_CHN_MESG, "Alignment");
            break;
        case CONTROL_C_EXIT:
            DEBUG_Printf(DBG_CHN_MESG, "^C");
            break;
        case EXCEPTION_CRITICAL_SECTION_WAIT:
            DEBUG_Printf(DBG_CHN_MESG, "critical section %08lx wait failed", 
			  rec->ExceptionInformation[0]);
	    if (!DBG_IVAR(BreakOnCritSectTimeOut))
		return TRUE;
            break;
        default:
            DEBUG_Printf(DBG_CHN_MESG, "%08lx", rec->ExceptionCode);
            break;
        }
	DEBUG_Printf(DBG_CHN_MESG, "\n");
    }

    DEBUG_Printf(DBG_CHN_TRACE, 
		 "Entering debugger 	PC=%lx EFL=%08lx mode=%d count=%d\n",
#ifdef __i386__
		 DEBUG_context.Eip, DEBUG_context.EFlags, 
#else
		 0L, 0L,
#endif
		 DEBUG_CurrThread->dbg_exec_mode, DEBUG_CurrThread->dbg_exec_count);

    if (DEBUG_ExceptionProlog(is_debug, force, rec->ExceptionCode)) {
	while ((ret = DEBUG_Parser())) {
	    if (DEBUG_ValidateRegisters()) {
		if (DEBUG_CurrThread->dbg_exec_mode != EXEC_PASS || first_chance)
		    break;
		DEBUG_Printf(DBG_CHN_MESG, "Cannot pass on last chance exception. You must use cont\n");
	    }
	}
    }
    *cont = DEBUG_ExceptionEpilog();

    DEBUG_Printf(DBG_CHN_TRACE, 
		 "Exiting debugger 	PC=%lx EFL=%08lx mode=%d count=%d\n",
#ifdef __i386__
		 DEBUG_context.Eip, DEBUG_context.EFlags, 
#else
		 0L, 0L,
#endif
		 DEBUG_CurrThread->dbg_exec_mode, DEBUG_CurrThread->dbg_exec_count);

    return ret;
}

static	BOOL	DEBUG_HandleDebugEvent(DEBUG_EVENT* de, LPDWORD cont)
{
    char		buffer[256];
    BOOL		ret;

    DEBUG_CurrPid = de->dwProcessId;
    DEBUG_CurrTid = de->dwThreadId;

    __TRY {
	ret = TRUE;
	*cont = 0L;
	
	if ((DEBUG_CurrProcess = DEBUG_GetProcess(de->dwProcessId)) != NULL)
	    DEBUG_CurrThread = DEBUG_GetThread(DEBUG_CurrProcess, de->dwThreadId);
	else 
	    DEBUG_CurrThread = NULL;
	
	switch (de->dwDebugEventCode) {
	case EXCEPTION_DEBUG_EVENT:
	    if (!DEBUG_CurrThread) {
		DEBUG_Printf(DBG_CHN_ERR, "%08lx:%08lx: not a registered process or thread (perhaps a 16 bit one ?)\n",
			     de->dwProcessId, de->dwThreadId);
		break;
	    }
	    
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: exception code=%08lx\n", 
			 de->dwProcessId, de->dwThreadId, 
			 de->u.Exception.ExceptionRecord.ExceptionCode);

	    if (DEBUG_CurrProcess->continue_on_first_exception) {
		DEBUG_CurrProcess->continue_on_first_exception = FALSE;
		if (!DBG_IVAR(BreakOnAttach)) {
		    *cont = DBG_CONTINUE;
		    break;
		}
	    }

	    DEBUG_context.ContextFlags =  CONTEXT_CONTROL
                                        | CONTEXT_INTEGER
#ifdef CONTEXT_SEGMENTS
	                                | CONTEXT_SEGMENTS
#endif
#ifdef CONTEXT_DEBUG_REGISTERS
		                        | CONTEXT_DEBUG_REGISTERS
#endif
					;

	    if (!GetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context)) {
		DEBUG_Printf(DBG_CHN_WARN, "Can't get thread's context\n");
		break;
	    }
	    
	    ret = DEBUG_HandleException(&de->u.Exception.ExceptionRecord, 
					de->u.Exception.dwFirstChance, 
					DEBUG_CurrThread->wait_for_first_exception,
					cont);
	    if (DEBUG_CurrThread) {
		DEBUG_CurrThread->wait_for_first_exception = 0;
		SetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context);
	    }
	    break;
	    
	case CREATE_THREAD_DEBUG_EVENT:
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: create thread D @%08lx\n", de->dwProcessId, de->dwThreadId, 
			 (unsigned long)(LPVOID)de->u.CreateThread.lpStartAddress);
	    
	    if (DEBUG_CurrProcess == NULL) {
		DEBUG_Printf(DBG_CHN_ERR, "Unknown process\n");
		break;
	    }
	    if (DEBUG_GetThread(DEBUG_CurrProcess, de->dwThreadId) != NULL) {
		DEBUG_Printf(DBG_CHN_TRACE, "Thread already listed, skipping\n");
		break;
	    }
	    
	    DEBUG_CurrThread = DEBUG_AddThread(DEBUG_CurrProcess, 
					       de->dwThreadId, 
					       de->u.CreateThread.hThread, 
					       de->u.CreateThread.lpStartAddress, 
					       de->u.CreateThread.lpThreadLocalBase);
	    if (!DEBUG_CurrThread) {
		DEBUG_Printf(DBG_CHN_ERR, "Couldn't create thread\n");
		break;
	    }
	    DEBUG_InitCurrThread();
	    break;
	    
	case CREATE_PROCESS_DEBUG_EVENT:
	    DEBUG_ProcessGetStringIndirect(buffer, sizeof(buffer), 
                                           de->u.CreateProcessInfo.hProcess, 
                                           de->u.CreateProcessInfo.lpImageName);
	    
	    /* FIXME unicode ? de->u.CreateProcessInfo.fUnicode */
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: create process %s @%08lx (%ld<%ld>)\n", 
			 de->dwProcessId, de->dwThreadId, 
			 buffer,
			 (unsigned long)(LPVOID)de->u.CreateProcessInfo.lpStartAddress,
			 de->u.CreateProcessInfo.dwDebugInfoFileOffset,
			 de->u.CreateProcessInfo.nDebugInfoSize);
	    
	    if ((DEBUG_CurrProcess = DEBUG_GetProcess(de->dwProcessId)) != NULL) {
		if (DEBUG_CurrProcess->handle) {
		    DEBUG_Printf(DBG_CHN_ERR, "Skipping already defined process\n");
		    break;
		}
		DEBUG_CurrProcess->handle = de->u.CreateProcessInfo.hProcess;
	    } else {
		DEBUG_CurrProcess = DEBUG_AddProcess(de->dwProcessId,
						     de->u.CreateProcessInfo.hProcess);
		if (DEBUG_CurrProcess == NULL) {
		    DEBUG_Printf(DBG_CHN_ERR, "Unknown process\n");
		    break;
		}
	    }
	    
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: create thread I @%08lx\n", 
			 de->dwProcessId, de->dwThreadId, 
			 (unsigned long)(LPVOID)de->u.CreateProcessInfo.lpStartAddress);
	    
	    DEBUG_CurrThread = DEBUG_AddThread(DEBUG_CurrProcess, 	
					       de->dwThreadId, 
					       de->u.CreateProcessInfo.hThread, 
					       de->u.CreateProcessInfo.lpStartAddress, 
					       de->u.CreateProcessInfo.lpThreadLocalBase);
	    if (!DEBUG_CurrThread) {
		DEBUG_Printf(DBG_CHN_ERR, "Couldn't create thread\n");
		break;
	    }
	    
	    DEBUG_InitCurrProcess();
	    DEBUG_InitCurrThread();

            DEBUG_ProcessGetStringIndirect(buffer, sizeof(buffer), 
                                           DEBUG_CurrThread->process->handle, 
                                           de->u.CreateProcessInfo.lpImageName);
	    DEBUG_LoadModule32(buffer[0] ? buffer : "<Debugged process>",
                               de->u.CreateProcessInfo.hFile, 
			       (DWORD)de->u.CreateProcessInfo.lpBaseOfImage);

            if (buffer[0])  /* we got a process name */
            {
                DWORD type;
                if (!GetBinaryTypeA( buffer, &type ))
                {
                    /* not a Windows binary, assume it's a Unix executable then */
                    DOS_FULL_NAME fullname;
                    /* HACK!! should fix DEBUG_ReadExecutableDbgInfo to accept DOS filenames */
                    if (DOSFS_GetFullName( buffer, TRUE, &fullname ))
                    {
                        DEBUG_ReadExecutableDbgInfo( fullname.long_name );
                        break;
                    }
                }
            }
            /* if it is a Windows binary, or an invalid or missing file name,
             * we use wine itself as the main executable */
            DEBUG_ReadExecutableDbgInfo( "wine" );
	    break;
	    
	case EXIT_THREAD_DEBUG_EVENT:
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: exit thread (%ld)\n", 
			 de->dwProcessId, de->dwThreadId, de->u.ExitThread.dwExitCode);
	    
	    if (DEBUG_CurrThread == NULL) {
		DEBUG_Printf(DBG_CHN_ERR, "Unknown thread\n");
		break;
	    }
	    /* FIXME: remove break point set on thread startup */
	    DEBUG_DelThread(DEBUG_CurrThread);
	    break;
	    
	case EXIT_PROCESS_DEBUG_EVENT:
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: exit process (%ld)\n", 
			 de->dwProcessId, de->dwThreadId, de->u.ExitProcess.dwExitCode);
	    
	    if (DEBUG_CurrProcess == NULL) {
		DEBUG_Printf(DBG_CHN_ERR, "Unknown process\n");
		break;
	    }
	    /* just in case */
	    DEBUG_SetBreakpoints(FALSE);
	    /* kill last thread */
	    DEBUG_DelThread(DEBUG_CurrProcess->threads);
	    DEBUG_DelProcess(DEBUG_CurrProcess);

	    DEBUG_Printf(DBG_CHN_MESG, "Process of pid=%08lx has terminated\n", DEBUG_CurrPid);
	    break;
	    
	case LOAD_DLL_DEBUG_EVENT:
	    if (DEBUG_CurrThread == NULL) {
		DEBUG_Printf(DBG_CHN_ERR, "Unknown thread\n");
		break;
	    }
	    DEBUG_ProcessGetStringIndirect(buffer, sizeof(buffer), 
                                           DEBUG_CurrThread->process->handle, 
                                           de->u.LoadDll.lpImageName);
	    
	    /* FIXME unicode: de->u.LoadDll.fUnicode */
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: loads DLL %s @%08lx (%ld<%ld>)\n", 
			 de->dwProcessId, de->dwThreadId, 
			 buffer, (unsigned long)de->u.LoadDll.lpBaseOfDll,
			 de->u.LoadDll.dwDebugInfoFileOffset,
			 de->u.LoadDll.nDebugInfoSize);
	    CharUpper(buffer);
	    DEBUG_LoadModule32(buffer, de->u.LoadDll.hFile, (DWORD)de->u.LoadDll.lpBaseOfDll);
	    if (DBG_IVAR(BreakOnDllLoad)) {
		DEBUG_Printf(DBG_CHN_MESG, "Stopping on DLL %s loading at %08lx\n", 
			     buffer, (unsigned long)de->u.LoadDll.lpBaseOfDll);
		ret = DEBUG_Parser();
	    }
	    break;
	    
	case UNLOAD_DLL_DEBUG_EVENT:
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: unload DLL @%08lx\n", de->dwProcessId, de->dwThreadId, 
			 (unsigned long)de->u.UnloadDll.lpBaseOfDll);
	    break;
	    
	case OUTPUT_DEBUG_STRING_EVENT:
	    if (DEBUG_CurrThread == NULL) {
		DEBUG_Printf(DBG_CHN_ERR, "Unknown thread\n");
		break;
	    }
	    
	    DEBUG_ProcessGetString(buffer, sizeof(buffer), 
				   DEBUG_CurrThread->process->handle, 
				   de->u.DebugString.lpDebugStringData);
	    
	    /* fixme unicode de->u.DebugString.fUnicode ? */
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: output debug string (%s)\n", 
			 de->dwProcessId, de->dwThreadId, buffer);
	    break;
	    
	case RIP_EVENT:
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: rip error=%ld type=%ld\n", 
			 de->dwProcessId, de->dwThreadId, de->u.RipInfo.dwError, 
			 de->u.RipInfo.dwType);
	    break;
	    
	default:
	    DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: unknown event (%ld)\n", 
			 de->dwProcessId, de->dwThreadId, de->dwDebugEventCode);
	}
	
    } __EXCEPT(wine_dbg) {
	*cont = 0;
	ret = TRUE;
    }
    __ENDTRY;
    return ret;
}

static	DWORD	DEBUG_MainLoop(void)
{
    DEBUG_EVENT		de;
    DWORD		cont;
    BOOL		ret;

    DEBUG_Printf(DBG_CHN_MESG, " on pid %ld\n", DEBUG_CurrPid);
    
    for (ret = TRUE; ret; ) {
	/* wait until we get at least one loaded process */
	while (!DEBUG_ProcessList && (ret = DEBUG_Parser()));
	if (!ret) break;

	while (ret && DEBUG_ProcessList && WaitForDebugEvent(&de, INFINITE)) {
	    ret = DEBUG_HandleDebugEvent(&de, &cont);
	    ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont);
	}
    };
    
    DEBUG_Printf(DBG_CHN_MESG, "WineDbg terminated on pid %ld\n", DEBUG_CurrPid);

    return 0;
}

static	BOOL	DEBUG_Start(LPSTR cmdLine)
{
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    
    if (!CreateProcess(NULL, cmdLine, NULL, NULL, 
		       FALSE, DEBUG_PROCESS, NULL, NULL, &startup, &info)) {
	DEBUG_Printf(DBG_CHN_MESG, "Couldn't start process '%s'\n", cmdLine);
	return FALSE;
    }
    DEBUG_CurrPid = info.dwProcessId;
    if (!(DEBUG_CurrProcess = DEBUG_AddProcess(DEBUG_CurrPid, 0))) return FALSE;

    return TRUE;
}

void	DEBUG_Run(const char* args)
{
    DBG_MODULE*	wmod = DEBUG_GetProcessMainModule(DEBUG_CurrProcess);
    const char* pgm = (wmod) ? wmod->module_name : "none";

    if (args) {
	DEBUG_Printf(DBG_CHN_MESG, "Run (%s) with '%s'\n", pgm, args);
    } else {
	if (!DEBUG_LastCmdLine) {
	    DEBUG_Printf(DBG_CHN_MESG, "Cannot find previously used command line.\n");
	    return;
	}
	DEBUG_Start(DEBUG_LastCmdLine);
    }
}

int DEBUG_main(int argc, char** argv)
{
    DWORD	retv = 0;

#ifdef DBG_need_heap
    /* Initialize the debugger heap. */
    dbg_heap = HeapCreate(HEAP_NO_SERIALIZE, 0x1000, 0x8000000); /* 128MB */
#endif
    
    /* Initialize the type handling stuff. */
    DEBUG_InitTypes();
    DEBUG_InitCVDataTypes();    

    /* Initialize internal vars (types must be initialized before) */
    if (!DEBUG_IntVarsRW(TRUE)) return -1;

    /* keep it as a guiexe for now, so that Wine won't touch the Unix stdin, 
     * stdout and stderr streams
     */
    if (DBG_IVAR(UseXTerm)) {
	COORD		pos;
	
	/* This is a hack: it forces creation of an xterm, not done by default */
	pos.X = 0; pos.Y = 1;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    }

    DEBUG_Printf(DBG_CHN_MESG, "Starting WineDbg... ");

    if (argc == 3) {
	HANDLE	hEvent;
	DWORD	pid;

	if ((pid = atoi(argv[1])) != 0 && (hEvent = atoi(argv[2])) != 0) {
	    BOOL	ret = DEBUG_Attach(pid, TRUE);

	    SetEvent(hEvent);
	    if (!ret) {
		DEBUG_Printf(DBG_CHN_ERR, "Can't attach process %ld: %ld\n", 
			     DEBUG_CurrPid, GetLastError());
		goto leave;
	    }
	    DEBUG_CurrPid = pid;
	}
    }
	
    if (DEBUG_CurrPid == 0 && argc > 1) {
	int	i, len;
	LPSTR	cmdLine;
		
	if (!(cmdLine = DBG_alloc(len = 1))) goto oom_leave;
	cmdLine[0] = '\0';

	for (i = 1; i < argc; i++) {
	    len += strlen(argv[i]) + 1;
	    if (!(cmdLine = DBG_realloc(cmdLine, len))) goto oom_leave;
	    strcat(cmdLine, argv[i]);
	    cmdLine[len - 2] = ' ';
	    cmdLine[len - 1] = '\0';
	}

	if (!DEBUG_Start(cmdLine)) {
	    DEBUG_Printf(DBG_CHN_MESG, "Couldn't start process '%s'\n", cmdLine);
	    goto leave;
	}
	DBG_free(DEBUG_LastCmdLine);
	DEBUG_LastCmdLine = cmdLine;
    }

    retv = DEBUG_MainLoop();
 leave:
    /* saves modified variables */
    DEBUG_IntVarsRW(FALSE);

    return retv;

 oom_leave:
    DEBUG_Printf(DBG_CHN_MESG, "Out of memory\n");
    goto leave;
}
