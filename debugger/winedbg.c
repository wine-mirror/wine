/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/* Wine internal debugger
 * Interface to Windows debugger API
 * Eric Pouech (c) 2000
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "debugger.h"
#include "winbase.h"
#include "winreg.h"
#include "debugtools.h"
#include "options.h"

#ifdef DBG_need_heap
HANDLE dbg_heap = 0;
#endif

DEFAULT_DEBUG_CHANNEL(winedbg);
    
WINE_DBG_PROCESS* DEBUG_CurrProcess = NULL;
WINE_DBG_THREAD*  DEBUG_CurrThread = NULL;
CONTEXT		  DEBUG_context;

static WINE_DBG_PROCESS* proc = NULL;

static	WINE_DBG_PROCESS*	DEBUG_GetProcess(DWORD pid)
{
    WINE_DBG_PROCESS*	p;
    
    for (p = proc; p; p = p->next)
	if (p->pid == pid) break;
    return p;
}

static	WINE_DBG_PROCESS*	DEBUG_AddProcess(DWORD pid, HANDLE h)
{
    WINE_DBG_PROCESS*	p = DBG_alloc(sizeof(WINE_DBG_PROCESS));
    if (!p)
	return NULL;
    p->handle = h;
    p->pid = pid;
    p->threads = NULL;

    p->next = proc;
    p->prev = NULL;
    if (proc) proc->prev = p;
    proc = p;
    return p;
}

static	void			DEBUG_DelThread(WINE_DBG_THREAD* p);

static	void			DEBUG_DelProcess(WINE_DBG_PROCESS* p)
{
    if (p->threads != NULL) {
	ERR("Shouldn't happen\n");
	while (p->threads) DEBUG_DelThread(p->threads);
    }
    if (p->prev) p->prev->next = p->next;
    if (p->next) p->next->prev = p->prev;
    if (p == proc) proc = p->next;
    DBG_free(p);
}

static	void			DEBUG_InitCurrProcess(void)
{
#ifdef DBG_need_heap
    /*
     * Initialize the debugger heap.
     */
    dbg_heap = HeapCreate(HEAP_NO_SERIALIZE, 0x1000, 0x8000000); /* 128MB */
#endif
    
    /*
     * Initialize the type handling stuff.
     */
    DEBUG_InitTypes();
    DEBUG_InitCVDataTypes();
    
    /*
     * In some cases we can read the stabs information directly
     * from the executable.  If this is the case, we don't need
     * to bother with trying to read a symbol file, as the stabs
     * also have line number and local variable information.
     * As long as gcc is used for the compiler, stabs will
     * be the default.  On SVr4, DWARF could be used, but we
     * don't grok that yet, and in this case we fall back to using
     * the wine.sym file.
     */
    if( DEBUG_ReadExecutableDbgInfo() == FALSE )
    {
	char*		symfilename = "wine.sym";
	struct stat	statbuf;
	HKEY 		hWineConf, hkey;
	DWORD 		count;
	char 		symbolTableFile[256];
	
	if (-1 == stat(symfilename, &statbuf) )
	    symfilename = LIBDIR "wine.sym";
	
	strcpy(symbolTableFile, symfilename);
	if (!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config", &hWineConf)) {
	    if (!RegOpenKeyA(hWineConf, "wine", &hkey)) {
		count = sizeof(symbolTableFile);
		RegQueryValueA(hkey, "SymbolTableFile", symbolTableFile, &count);
		RegCloseKey(hkey);
	    }
	    RegCloseKey(hWineConf);
	}
	DEBUG_ReadSymbolTable(symbolTableFile);
    }
    DEBUG_LoadEntryPoints(NULL);
    DEBUG_ProcessDeferredDebug();
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

static	WINE_DBG_THREAD*	DEBUG_GetThread(WINE_DBG_PROCESS* p, DWORD tid)
{
    WINE_DBG_THREAD*	t;
    
    for (t = p->threads; t; t = t->next)
	if (t->tid == tid) break;
    return t;
}

static	WINE_DBG_THREAD*	DEBUG_AddThread(WINE_DBG_PROCESS* p, DWORD tid, 
						HANDLE h, LPVOID start, LPVOID teb)
{
    WINE_DBG_THREAD*	t = DBG_alloc(sizeof(WINE_DBG_THREAD));
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
    
    t->next = p->threads;
    t->prev = NULL;
    if (p->threads) p->threads->prev = t;
    p->threads = t;

    return t;
}

static	void			DEBUG_InitCurrThread(void)
{
    if (!Options.debug) return;

    if (DEBUG_CurrThread->start) {
	DBG_VALUE	value;
	
	DEBUG_SetBreakpoints(FALSE);
	value.type = NULL;
	value.cookie = DV_TARGET;
	value.addr.seg = 0;
	value.addr.off = (DWORD)DEBUG_CurrThread->start;
	DEBUG_AddBreakpoint(&value);
	DEBUG_SetBreakpoints(TRUE);
    } else {
	DEBUG_CurrThread->wait_for_first_exception = 1;
    }
}

static	void			DEBUG_DelThread(WINE_DBG_THREAD* t)
{
    if (t->prev) t->prev->next = t->next;
    if (t->next) t->next->prev = t->prev;
    if (t == t->process->threads) t->process->threads = t->next;
    DBG_free(t);
}

static	BOOL	DEBUG_HandleException( EXCEPTION_RECORD *rec, BOOL first_chance, BOOL force )
{
    BOOL	is_debug = FALSE;
    BOOL	ret;

    if (first_chance && !Options.debug && !force ) return 0;  /* pass to app first */

    switch (rec->ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
        is_debug = TRUE;
        break;
    case CONTROL_C_EXIT:
        if (!Options.debug) DEBUG_Exit(0);
        break;
    }

    if (!is_debug)
    {
        /* print some infos */
        fprintf( stderr, "%s: ",
                 first_chance ? "First chance exception" : "Unhandled exception" );
        switch(rec->ExceptionCode)
        {
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            fprintf( stderr, "divide by zero" );
            break;
        case EXCEPTION_INT_OVERFLOW:
            fprintf( stderr, "overflow" );
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            fprintf( stderr, "array bounds " );
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            fprintf( stderr, "illegal instruction" );
            break;
        case EXCEPTION_STACK_OVERFLOW:
            fprintf( stderr, "stack overflow" );
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            fprintf( stderr, "priviledged instruction" );
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            if (rec->NumberParameters == 2)
                fprintf( stderr, "page fault on %s access to 0x%08lx", 
                         rec->ExceptionInformation[0] ? "write" : "read",
                         rec->ExceptionInformation[1] );
            else
                fprintf( stderr, "page fault" );
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            fprintf( stderr, "Alignment" );
            break;
        case CONTROL_C_EXIT:
            fprintf( stderr, "^C" );
            break;
        case EXCEPTION_CRITICAL_SECTION_WAIT:
            fprintf( stderr, "critical section %08lx wait failed", 
		     rec->ExceptionInformation[0] );
            break;
        default:
            fprintf( stderr, "%08lx", rec->ExceptionCode );
            break;
        }
    }

#if 1
    fprintf(stderr, "Entering debugger 	PC=%lx EFL=%08lx mode=%d count=%d\n",
	    DEBUG_context.Eip, DEBUG_context.EFlags, 
	    DEBUG_CurrThread->dbg_exec_mode, DEBUG_CurrThread->dbg_exec_count);
#endif

    ret = DEBUG_Main( is_debug, force, rec->ExceptionCode );
#if 1
    fprintf(stderr, "Exiting debugger 	PC=%lx EFL=%08lx mode=%d count=%d\n",
	    DEBUG_context.Eip, DEBUG_context.EFlags, 
	    DEBUG_CurrThread->dbg_exec_mode, DEBUG_CurrThread->dbg_exec_count);
#endif

    return ret;
}

static	DWORD	CALLBACK	DEBUG_MainLoop(DWORD pid)
{
    DEBUG_EVENT		de;
    char		buffer[256];
    DWORD		cont;

    while (WaitForDebugEvent(&de, INFINITE)) {
	cont = 0L;

	if ((DEBUG_CurrProcess = DEBUG_GetProcess(de.dwProcessId)) != NULL)
	    DEBUG_CurrThread = DEBUG_GetThread(DEBUG_CurrProcess, de.dwThreadId);
	else 
	    DEBUG_CurrThread = NULL;

	switch (de.dwDebugEventCode) {
	case EXCEPTION_DEBUG_EVENT:
	    if (!DEBUG_CurrThread) break;

	    TRACE("%08lx:%08lx: exception code=%08lx %d\n", 
		  de.dwProcessId, de.dwThreadId, 
		  de.u.Exception.ExceptionRecord.ExceptionCode,
		  DEBUG_CurrThread->wait_for_first_exception);

	    DEBUG_context.ContextFlags = CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS|CONTEXT_DEBUG_REGISTERS;
	    if (!GetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context)) {
		WARN("Can't get thread's context\n");
		break;
	    }

	    TRACE("%p:%p\n", de.u.Exception.ExceptionRecord.ExceptionAddress, 
		  (void*)DEBUG_context.Eip);

	    cont = DEBUG_HandleException(&de.u.Exception.ExceptionRecord, 
					 de.u.Exception.dwFirstChance, 
					 DEBUG_CurrThread->wait_for_first_exception);

	    if (DEBUG_CurrThread->wait_for_first_exception) {
		DEBUG_CurrThread->wait_for_first_exception = 0;
#ifdef __i386__
		DEBUG_context.Eip--;
#endif
	    }
	    SetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context);
	    break;
	    
	case CREATE_THREAD_DEBUG_EVENT:
	    TRACE("%08lx:%08lx: create thread D @%p\n", de.dwProcessId, de.dwThreadId, 
		  de.u.CreateThread.lpStartAddress);
	    
	    if (DEBUG_CurrProcess == NULL) {
		ERR("Unknown process\n");
		break;
	    }
	    if (DEBUG_GetThread(DEBUG_CurrProcess, de.dwThreadId) != NULL) {
		TRACE("Thread already listed, skipping\n");
		break;
	    }

	    DEBUG_CurrThread = DEBUG_AddThread(DEBUG_CurrProcess, 
					       de.dwThreadId, 
					       de.u.CreateThread.hThread, 
					       de.u.CreateThread.lpStartAddress, 
					       de.u.CreateThread.lpThreadLocalBase);
	    if (!DEBUG_CurrThread) {
		ERR("Couldn't create thread\n");
		break;
	    }
	    DEBUG_InitCurrThread();
	    break;
	    
	case CREATE_PROCESS_DEBUG_EVENT:
	    DEBUG_ProcessGetStringIndirect(buffer, sizeof(buffer), 
                                           de.u.CreateProcessInfo.hProcess, 
                                           de.u.LoadDll.lpImageName);
	    
	    /* FIXME unicode ? de.u.CreateProcessInfo.fUnicode */
	    TRACE("%08lx:%08lx: create process %s @%p\n", 
		  de.dwProcessId, de.dwThreadId, 
		  buffer,
		  de.u.CreateProcessInfo.lpStartAddress);
	    
	    if (DEBUG_GetProcess(de.dwProcessId) != NULL) {
		TRACE("Skipping already defined process\n");
		break;
	    }
	    DEBUG_CurrProcess = DEBUG_AddProcess(de.dwProcessId,
						 de.u.CreateProcessInfo.hProcess);
	    if (DEBUG_CurrProcess == NULL) {
		ERR("Unknown process\n");
		break;
	    }
	    
	    TRACE("%08lx:%08lx: create thread I @%p\n", de.dwProcessId, de.dwThreadId, 
		  de.u.CreateProcessInfo.lpStartAddress);
	    
	    DEBUG_CurrThread = DEBUG_AddThread(DEBUG_CurrProcess, 	
					       de.dwThreadId, 
					       de.u.CreateProcessInfo.hThread, 
					       de.u.CreateProcessInfo.lpStartAddress, 
					       de.u.CreateProcessInfo.lpThreadLocalBase);
	    if (!DEBUG_CurrThread) {
		ERR("Couldn't create thread\n");
		break;
	    }
	    
	    DEBUG_InitCurrProcess();
	    DEBUG_InitCurrThread();
#ifdef _WE_SUPPORT_THE_STAB_TYPES_USED_BY_MINGW_TOO
	    /* so far, process name is not set */
	    DEBUG_RegisterDebugInfo((DWORD)de.u.CreateProcessInfo.lpBaseOfImage, 
				    "wine-exec");
#endif
	    break;
	    
	case EXIT_THREAD_DEBUG_EVENT:
	    TRACE("%08lx:%08lx: exit thread (%ld)\n", 
		  de.dwProcessId, de.dwThreadId, de.u.ExitThread.dwExitCode);

	    if (DEBUG_CurrThread == NULL) {
		ERR("Unknown thread\n");
		break;
	    }
	    /* FIXME: remove break point set on thread startup */
	    DEBUG_DelThread(DEBUG_CurrThread);
	    break;
	    
	case EXIT_PROCESS_DEBUG_EVENT:
	    TRACE("%08lx:%08lx: exit process (%ld)\n", 
		  de.dwProcessId, de.dwThreadId, de.u.ExitProcess.dwExitCode);

	    if (DEBUG_CurrProcess == NULL) {
		ERR("Unknown process\n");
		break;
	    }
	    /* kill last thread */
	    DEBUG_DelThread(DEBUG_CurrProcess->threads);
	    /* FIXME: remove break point set on thread startup */
	    DEBUG_DelProcess(DEBUG_CurrProcess);
	    break;
	    
	case LOAD_DLL_DEBUG_EVENT:
	    if (DEBUG_CurrThread == NULL) {
		ERR("Unknown thread\n");
		break;
	    }
	    DEBUG_ProcessGetStringIndirect(buffer, sizeof(buffer), 
                                           DEBUG_CurrThread->process->handle, 
                                           de.u.LoadDll.lpImageName);
	    
	    /* FIXME unicode: de.u.LoadDll.fUnicode */
	    TRACE("%08lx:%08lx: loads DLL %s @%p\n", de.dwProcessId, de.dwThreadId, 
		  buffer, de.u.LoadDll.lpBaseOfDll);
	    break;
	    
	case UNLOAD_DLL_DEBUG_EVENT:
	    TRACE("%08lx:%08lx: unload DLL @%p\n", de.dwProcessId, de.dwThreadId, 
		  de.u.UnloadDll.lpBaseOfDll);
	    break;
	    
	case OUTPUT_DEBUG_STRING_EVENT:
	    if (DEBUG_CurrThread == NULL) {
		ERR("Unknown thread\n");
		break;
	    }

	    DEBUG_ProcessGetString(buffer, sizeof(buffer), 
				   DEBUG_CurrThread->process->handle, 
				   de.u.DebugString.lpDebugStringData);
	    
	    /* fixme unicode de.u.DebugString.fUnicode ? */
	    TRACE("%08lx:%08lx: output debug string (%s)\n", 
		  de.dwProcessId, de.dwThreadId, 
		  buffer);
	    break;
	    
	case RIP_EVENT:
	    TRACE("%08lx:%08lx: rip error=%ld type=%ld\n", 
		  de.dwProcessId, de.dwThreadId, de.u.RipInfo.dwError, 
		  de.u.RipInfo.dwType);
	    break;
	    
	default:
	    TRACE("%08lx:%08lx: unknown event (%ld)\n", 
		  de.dwProcessId, de.dwThreadId, de.dwDebugEventCode);
	}
	ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont);
    }
    
    TRACE("WineDbg terminated on pid %ld\n", pid);
    
    return 0L;
}

#include "thread.h"
#include "process.h"
#include "wingdi.h"
#include "winuser.h"

static	DWORD	CALLBACK	DEBUG_StarterFromPID(LPVOID pid)
{
    TRACE("WineDbg started on pid %ld\n", (DWORD)pid);
    
    if (!DebugActiveProcess((DWORD)pid)) {
	TRACE("Can't debug process %ld: %ld\n", (DWORD)pid, GetLastError());
	return 0;
    }
    return DEBUG_MainLoop((DWORD)pid);
}

void	DEBUG_Attach(DWORD pid)
{
    CreateThread(NULL, 0, DEBUG_StarterFromPID, (LPVOID)pid, 0, NULL);
}

struct dsfcl {
    HANDLE	hEvent;
    LPSTR	lpCmdLine;
    int		showWindow;
    DWORD	error;
};

static	DWORD	CALLBACK	DEBUG_StarterFromCmdLine(LPVOID p)
{
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    BOOL		ok = TRUE;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = ((struct dsfcl*)p)->showWindow;

    /* any value >= 32 will do, simulate a correct handle value */
    ((struct dsfcl*)p)->error = 0xFFFFFFFF;
    if (!CreateProcessA(NULL, ((struct dsfcl*)p)->lpCmdLine, NULL, NULL, 
			FALSE, DEBUG_PROCESS, NULL, NULL, &startup, &info)) {
	((struct dsfcl*)p)->error = GetLastError();
	ok = FALSE;
    }
    SetEvent(((struct dsfcl*)p)->hEvent);
    if (ok) DEBUG_MainLoop(info.dwProcessId);

    return 0;
}

DWORD	DEBUG_WinExec(LPSTR lpCmdLine, int sw)
{
    struct dsfcl 	s;
    BOOL		ret;

    if ((s.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL))) {
	s.lpCmdLine = lpCmdLine;
	s.showWindow = sw;
	if (CreateThread(NULL, 0, DEBUG_StarterFromCmdLine, (LPVOID)&s, 0, NULL)) {
	    WaitForSingleObject(s.hEvent, INFINITE);
	    ret = s.error;
	} else {
	    ret = 3; /* (dummy) error value for non created thread */
	}
	CloseHandle(s.hEvent);
    } else {
	ret = 1; /* (dummy) error value for non created event */
    }
    return ret;
}
