/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/* Wine internal debugger
 * Interface to Windows debugger API
 * Copyright 2000 Eric Pouech
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "debugger.h"

#include "winternl.h"
#include "thread.h"
#include "wincon.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "excpt.h"
#include "wine/library.h"

DBG_PROCESS*	DEBUG_CurrProcess = NULL;
DBG_THREAD*	DEBUG_CurrThread = NULL;
DWORD		DEBUG_CurrTid;
DWORD		DEBUG_CurrPid;
CONTEXT         DEBUG_context;
BOOL		DEBUG_InteractiveP = FALSE;
static BOOL     DEBUG_InException = FALSE;
int 		curr_frame = 0;
static char*	DEBUG_LastCmdLine = NULL;

static DBG_PROCESS* DEBUG_ProcessList = NULL;
static enum {none_mode = 0, winedbg_mode, automatic_mode, gdb_mode} local_mode;

DBG_INTVAR DEBUG_IntVars[DBG_IV_LAST];

void	DEBUG_OutputA(int chn, const char* buffer, int len)
{
    if (DBG_IVAR(ConChannelMask) & chn)
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, len, NULL, NULL);
    if (DBG_IVAR(StdChannelMask) & chn)
	fwrite(buffer, len, 1, stderr);
}

void	DEBUG_OutputW(int chn, const WCHAR* buffer, int len)
{
    /* FIXME: this won't work is std output isn't attached to a console */
    if (DBG_IVAR(ConChannelMask) & chn)
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buffer, len, NULL, NULL);
    /* simplistic Unicode to ANSI conversion */
    if (DBG_IVAR(StdChannelMask) & chn)
        while (len--) fputc((char)*buffer++, stderr);
}

int	DEBUG_Printf(int chn, const char* format, ...)
{
static    char	buf[4*1024];
    va_list 	valist;
    int		len;

    va_start(valist, format);
    len = vsnprintf(buf, sizeof(buf), format, valist);
    va_end(valist);

    if (len <= -1 || len >= sizeof(buf)) {
	len = sizeof(buf) - 1;
	buf[len] = 0;
	buf[len - 1] = buf[len - 2] = buf[len - 3] = '.';
    }
    DEBUG_OutputA(chn, buf, len);
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
        div->type = DEBUG_GetBasicType(_typ); div++;
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

DBG_PROCESS*	DEBUG_GetProcess(DWORD pid)
{
    DBG_PROCESS*	p;

    for (p = DEBUG_ProcessList; p; p = p->next)
	if (p->pid == pid) break;
    return p;
}

DBG_PROCESS*	DEBUG_AddProcess(DWORD pid, HANDLE h, const char* imageName)
{
    DBG_PROCESS*	p;

    if ((p = DEBUG_GetProcess(pid)))
    {
        if (p->handle != 0)
        {
            DEBUG_Printf(DBG_CHN_ERR, "Process (%lu) is already defined\n", pid);
        }
        else
        {
            p->handle = h;
            p->imageName = imageName ? DBG_strdup(imageName) : NULL;
        }
        return p;
    }

    if (!(p = DBG_alloc(sizeof(DBG_PROCESS)))) return NULL;
    p->handle = h;
    p->pid = pid;
    p->imageName = imageName ? DBG_strdup(imageName) : NULL;
    p->threads = NULL;
    p->num_threads = 0;
    p->continue_on_first_exception = FALSE;
    p->modules = NULL;
    p->num_modules = 0;
    p->next_index = 0;
    p->dbg_hdr_addr = 0;
    p->delayed_bp = NULL;
    p->num_delayed_bp = 0;

    p->next = DEBUG_ProcessList;
    p->prev = NULL;
    if (DEBUG_ProcessList) DEBUG_ProcessList->prev = p;
    DEBUG_ProcessList = p;
    return p;
}

void DEBUG_DelProcess(DBG_PROCESS* p)
{
    int	i;

    while (p->threads) DEBUG_DelThread(p->threads);

    for (i = 0; i < p->num_delayed_bp; i++)
        if (p->delayed_bp[i].is_symbol)
            DBG_free(p->delayed_bp[i].u.symbol.name);

    DBG_free(p->delayed_bp);
    if (p->prev) p->prev->next = p->next;
    if (p->next) p->next->prev = p->prev;
    if (p == DEBUG_ProcessList) DEBUG_ProcessList = p->next;
    if (p == DEBUG_CurrProcess) DEBUG_CurrProcess = NULL;
    DBG_free((char*)p->imageName);
    DBG_free(p);
}

static	void			DEBUG_InitCurrProcess(void)
{
}

BOOL DEBUG_ProcessGetString(char* buffer, int size, HANDLE hp, LPSTR addr)
{
    DWORD sz;
    *(WCHAR*)buffer = 0;
    return (addr && ReadProcessMemory(hp, addr, buffer, size, &sz));
}

BOOL DEBUG_ProcessGetStringIndirect(char* buffer, int size, HANDLE hp, LPVOID addr)
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

DBG_THREAD*	DEBUG_GetThread(DBG_PROCESS* p, DWORD tid)
{
    DBG_THREAD*	t;

    if (!p) return NULL;
    for (t = p->threads; t; t = t->next)
	if (t->tid == tid) break;
    return t;
}

DBG_THREAD*	DEBUG_AddThread(DBG_PROCESS* p, DWORD tid,
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
    t->exec_mode = EXEC_CONT;
    t->exec_count = 0;

    sprintf(t->name, "%08lx", tid);

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
	    DEBUG_AddBreakpointFromValue(&value);
	    DEBUG_SetBreakpoints(TRUE);
	}
    } else {
	DEBUG_CurrThread->wait_for_first_exception = 1;
    }
}

void			DEBUG_DelThread(DBG_THREAD* t)
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
    if (!(DEBUG_CurrProcess = DEBUG_AddProcess(pid, 0, NULL))) return FALSE;

    if (!DebugActiveProcess(pid)) {
        DEBUG_Printf(DBG_CHN_MESG, "Can't attach process %lx: error %ld\n", pid, GetLastError());
        DEBUG_DelProcess(DEBUG_CurrProcess);
        DEBUG_CurrProcess = NULL;
	return FALSE;
    }
    DEBUG_CurrProcess->continue_on_first_exception = cofe;
    return TRUE;
}

BOOL				DEBUG_Detach(void)
{
    /* remove all set breakpoints in debuggee code */
    DEBUG_SetBreakpoints(FALSE);
    /* needed for single stepping (ugly).
     * should this be handled inside the server ??? */
#ifdef __i386__
    DEBUG_context.EFlags &= ~STEP_FLAG;
#endif
    SetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context);
    DebugActiveProcessStop(DEBUG_CurrProcess->pid);
    DEBUG_DelProcess(DEBUG_CurrProcess);
    DEBUG_CurrProcess = NULL;
    /* FIXME: should zero out the symbol table too */
    return TRUE;
}

static BOOL                     DEBUG_FetchContext(void)
{
    DEBUG_context.ContextFlags = CONTEXT_CONTROL
                               | CONTEXT_INTEGER
#ifdef CONTEXT_SEGMENTS
                               | CONTEXT_SEGMENTS
#endif
#ifdef CONTEXT_DEBUG_REGISTERS
	                       | CONTEXT_DEBUG_REGISTERS
#endif
                               ;
    if (!GetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context))
    {
        DEBUG_Printf(DBG_CHN_WARN, "Can't get thread's context\n");
        return FALSE;
    }
    return TRUE;
}

static  BOOL	DEBUG_ExceptionProlog(BOOL is_debug, BOOL force, DWORD code)
{
    DBG_ADDR	addr;
    int		newmode;

    DEBUG_InException = TRUE;
    DEBUG_GetCurrentAddress(&addr);
    DEBUG_SuspendExecution();

    if (!is_debug)
    {
        if (!addr.seg)
            DEBUG_Printf(DBG_CHN_MESG, " in 32-bit code (0x%08lx)", addr.off);
        else
            switch(DEBUG_GetSelectorType(addr.seg))
            {
            case MODE_32:
                DEBUG_Printf(DBG_CHN_MESG, " in 32-bit code (%04lx:%08lx)", addr.seg, addr.off);
                break;
            case MODE_16:
                DEBUG_Printf(DBG_CHN_MESG, " in 16-bit code (%04lx:%04lx)", addr.seg, addr.off);
                break;
            case MODE_VM86:
                DEBUG_Printf(DBG_CHN_MESG, " in vm86 code (%04lx:%04lx)", addr.seg, addr.off);
                break;
            case MODE_INVALID:
                DEBUG_Printf(DBG_CHN_MESG, " bad CS (%lx)", addr.seg);
                break;
        }
	DEBUG_Printf(DBG_CHN_MESG, ".\n");
    }

    DEBUG_LoadEntryPoints("Loading new modules symbols:\n");

    if (!force && is_debug &&
	DEBUG_ShouldContinue(&addr, code,
			     &DEBUG_CurrThread->exec_count))
	return FALSE;

    if ((newmode = DEBUG_GetSelectorType(addr.seg)) == MODE_INVALID) newmode = MODE_32;
    if (newmode != DEBUG_CurrThread->dbg_mode)
    {
        static const char * const names[] = { "???", "16-bit", "32-bit", "vm86" };
        DEBUG_Printf(DBG_CHN_MESG,"In %s mode.\n", names[newmode] );
        DEBUG_CurrThread->dbg_mode = newmode;
    }

    DEBUG_DoDisplay();

    if (is_debug || force) {
	/*
	 * Do a quiet backtrace so that we have an idea of what the situation
	 * is WRT the source files.
	 */
	DEBUG_BackTrace(DEBUG_CurrTid, FALSE);
    } else {
	/* This is a real crash, dump some info */
	DEBUG_InfoRegisters(&DEBUG_context);
	DEBUG_InfoStack();
#ifdef __i386__
	if (DEBUG_CurrThread->dbg_mode == MODE_16) {
	    DEBUG_InfoSegments(DEBUG_context.SegDs >> 3, 1);
	    if (DEBUG_context.SegEs != DEBUG_context.SegDs)
		DEBUG_InfoSegments(DEBUG_context.SegEs >> 3, 1);
	}
	DEBUG_InfoSegments(DEBUG_context.SegFs >> 3, 1);
#endif
	DEBUG_BackTrace(DEBUG_CurrTid, TRUE);
    }

    if (!is_debug ||
	(DEBUG_CurrThread->exec_mode == EXEC_STEPI_OVER) ||
	(DEBUG_CurrThread->exec_mode == EXEC_STEPI_INSTR)) {

	struct list_id list;

	/* Show where we crashed */
	curr_frame = 0;
	DEBUG_DisassembleInstruction(&addr);

	/* resets list internal arguments so we can look at source code when needed */
	DEBUG_FindNearestSymbol(&addr, TRUE, NULL, 0, &list);
	if (list.sourcefile) DEBUG_List(&list, NULL, 0);
    }
    return TRUE;
}

static  void	DEBUG_ExceptionEpilog(void)
{
    DEBUG_RestartExecution(DEBUG_CurrThread->exec_count);
    /*
     * This will have gotten absorbed into the breakpoint info
     * if it was used.  Otherwise it would have been ignored.
     * In any case, we don't mess with it any more.
     */
    if (DEBUG_CurrThread->exec_mode == EXEC_CONT)
	DEBUG_CurrThread->exec_count = 0;
    DEBUG_InException = FALSE;
}

static	BOOL DEBUG_HandleException(EXCEPTION_RECORD *rec, BOOL first_chance, BOOL force)
{
    BOOL             is_debug = FALSE;
    THREADNAME_INFO *pThreadName;
    DBG_THREAD      *pThread;

    assert(DEBUG_CurrThread);

    switch (rec->ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
        is_debug = TRUE;
        break;
    case EXCEPTION_NAME_THREAD:
        pThreadName = (THREADNAME_INFO*)(rec->ExceptionInformation);
        if (pThreadName->dwThreadID == -1)
            pThread = DEBUG_CurrThread;
        else
            pThread = DEBUG_GetThread(DEBUG_CurrProcess, pThreadName->dwThreadID);

        if (ReadProcessMemory(DEBUG_CurrThread->process->handle, pThreadName->szName,
                              pThread->name, 9, NULL))
            DEBUG_Printf (DBG_CHN_MESG,
                          "Thread ID=0x%lx renamed using MS VC6 extension (name==\"%s\")\n",
                          pThread->tid, pThread->name);
        return FALSE;
    }

    if (first_chance && !is_debug && !force && !DBG_IVAR(BreakOnFirstChance))
    {
        /* pass exception to program except for debug exceptions */
        return FALSE;
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
            DEBUG_Printf(DBG_CHN_MESG, "privileged instruction");
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
	case DBG_CONTROL_C:
            DEBUG_Printf(DBG_CHN_MESG, "^C");
            break;
        case CONTROL_C_EXIT:
            DEBUG_Printf(DBG_CHN_MESG, "^C");
            break;
        case EXCEPTION_CRITICAL_SECTION_WAIT:
	    {
		DBG_ADDR	addr;

		addr.seg = 0;
		addr.off = rec->ExceptionInformation[0];

		DEBUG_Printf(DBG_CHN_MESG, "wait failed on critical section ");
		DEBUG_PrintAddress(&addr, DEBUG_CurrThread->dbg_mode, FALSE);
	    }
	    if (!DBG_IVAR(BreakOnCritSectTimeOut))
	    {
		DEBUG_Printf(DBG_CHN_MESG, "\n");
		return FALSE;
	    }
            break;
        case EXCEPTION_WINE_STUB:
            {
                char dll[32], name[64];
                DEBUG_ProcessGetString( dll, sizeof(dll), DEBUG_CurrThread->process->handle,
                                        (char *)rec->ExceptionInformation[0] );
                DEBUG_ProcessGetString( name, sizeof(name), DEBUG_CurrThread->process->handle,
                                        (char *)rec->ExceptionInformation[1] );
                DEBUG_Printf(DBG_CHN_MESG, "unimplemented function %s.%s called", dll, name );
            }
            break;
        case EXCEPTION_WINE_ASSERTION:
            DEBUG_Printf(DBG_CHN_MESG, "assertion failed");
            break;
        case EXCEPTION_VM86_INTx:
            DEBUG_Printf(DBG_CHN_MESG, "interrupt %02lx in vm86 mode",
                         rec->ExceptionInformation[0]);
            break;
        case EXCEPTION_VM86_STI:
            DEBUG_Printf(DBG_CHN_MESG, "sti in vm86 mode");
            break;
        case EXCEPTION_VM86_PICRETURN:
            DEBUG_Printf(DBG_CHN_MESG, "PIC return in vm86 mode");
            break;
        default:
            DEBUG_Printf(DBG_CHN_MESG, "%08lx", rec->ExceptionCode);
            break;
        }
    }

    if (local_mode == automatic_mode)
    {
        DEBUG_ExceptionProlog(is_debug, FALSE, rec->ExceptionCode);
        DEBUG_ExceptionEpilog();
        return TRUE;  /* terminate execution */
    }

    if (DEBUG_ExceptionProlog(is_debug, force, rec->ExceptionCode))
    {
	DEBUG_InteractiveP = TRUE;
        return TRUE;
    }
    DEBUG_ExceptionEpilog();

    return FALSE;
}

static	BOOL	DEBUG_HandleDebugEvent(DEBUG_EVENT* de)
{
    char		buffer[256];
    BOOL                ret = FALSE;

    DEBUG_CurrPid = de->dwProcessId;
    DEBUG_CurrTid = de->dwThreadId;

    if ((DEBUG_CurrProcess = DEBUG_GetProcess(de->dwProcessId)) != NULL)
        DEBUG_CurrThread = DEBUG_GetThread(DEBUG_CurrProcess, de->dwThreadId);
    else
        DEBUG_CurrThread = NULL;

    switch (de->dwDebugEventCode)
    {
    case EXCEPTION_DEBUG_EVENT:
        if (!DEBUG_CurrThread)
        {
            DEBUG_Printf(DBG_CHN_ERR, "%08lx:%08lx: not a registered process or thread (perhaps a 16 bit one ?)\n",
                         de->dwProcessId, de->dwThreadId);
            break;
        }

        DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: exception code=%08lx\n",
                     de->dwProcessId, de->dwThreadId,
                     de->u.Exception.ExceptionRecord.ExceptionCode);

        if (DEBUG_CurrProcess->continue_on_first_exception)
        {
            DEBUG_CurrProcess->continue_on_first_exception = FALSE;
            if (!DBG_IVAR(BreakOnAttach)) break;
        }

        if (DEBUG_FetchContext())
        {
            ret = DEBUG_HandleException(&de->u.Exception.ExceptionRecord,
                                        de->u.Exception.dwFirstChance,
                                        DEBUG_CurrThread->wait_for_first_exception);
            if (!ret && DEBUG_CurrThread)
            {
                DEBUG_CurrThread->wait_for_first_exception = 0;
                SetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context);
            }
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: create thread D @%08lx\n", de->dwProcessId, de->dwThreadId,
                     (unsigned long)(LPVOID)de->u.CreateThread.lpStartAddress);

        if (DEBUG_CurrProcess == NULL)
        {
            DEBUG_Printf(DBG_CHN_ERR, "Unknown process\n");
            break;
        }
        if (DEBUG_GetThread(DEBUG_CurrProcess, de->dwThreadId) != NULL)
        {
            DEBUG_Printf(DBG_CHN_TRACE, "Thread already listed, skipping\n");
            break;
        }

        DEBUG_CurrThread = DEBUG_AddThread(DEBUG_CurrProcess,
                                           de->dwThreadId,
                                           de->u.CreateThread.hThread,
                                           de->u.CreateThread.lpStartAddress,
                                           de->u.CreateThread.lpThreadLocalBase);
        if (!DEBUG_CurrThread)
        {
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
        DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: create process '%s'/%p @%08lx (%ld<%ld>)\n",
                     de->dwProcessId, de->dwThreadId,
                     buffer, de->u.CreateProcessInfo.lpImageName,
                     (unsigned long)(LPVOID)de->u.CreateProcessInfo.lpStartAddress,
                     de->u.CreateProcessInfo.dwDebugInfoFileOffset,
                     de->u.CreateProcessInfo.nDebugInfoSize);

        DEBUG_CurrProcess = DEBUG_AddProcess(de->dwProcessId,
                                             de->u.CreateProcessInfo.hProcess,
                                             buffer[0] ? buffer : "<Debugged Process>");
        if (DEBUG_CurrProcess == NULL)
        {
            DEBUG_Printf(DBG_CHN_ERR, "Couldn't create process\n");
            break;
        }

        DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: create thread I @%08lx\n",
                     de->dwProcessId, de->dwThreadId,
                     (unsigned long)(LPVOID)de->u.CreateProcessInfo.lpStartAddress);

        DEBUG_CurrThread = DEBUG_AddThread(DEBUG_CurrProcess,
                                           de->dwThreadId,
                                           de->u.CreateProcessInfo.hThread,
                                           de->u.CreateProcessInfo.lpStartAddress,
                                           de->u.CreateProcessInfo.lpThreadLocalBase);
        if (!DEBUG_CurrThread)
        {
            DEBUG_Printf(DBG_CHN_ERR, "Couldn't create thread\n");
            break;
        }

        DEBUG_InitCurrProcess();
        DEBUG_InitCurrThread();

        /* module is either PE, NE or ELF module (for WineLib), but all
         * are loaded with wine, so load its symbols, then the main module
         */
        do
        {
            char*   ptr = getenv("WINELOADER");

            if (!ptr || DEBUG_ReadExecutableDbgInfo( ptr ) == DIL_ERROR)
                DEBUG_ReadExecutableDbgInfo( "wine" );
        } while (0);

        DEBUG_LoadModule32(DEBUG_CurrProcess->imageName, de->u.CreateProcessInfo.hFile,
                           (DWORD)de->u.CreateProcessInfo.lpBaseOfImage);

        break;

    case EXIT_THREAD_DEBUG_EVENT:
        DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: exit thread (%ld)\n",
                     de->dwProcessId, de->dwThreadId, de->u.ExitThread.dwExitCode);

        if (DEBUG_CurrThread == NULL)
        {
            DEBUG_Printf(DBG_CHN_ERR, "Unknown thread\n");
            break;
        }
        /* FIXME: remove break point set on thread startup */
        DEBUG_DelThread(DEBUG_CurrThread);
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: exit process (%ld)\n",
                     de->dwProcessId, de->dwThreadId, de->u.ExitProcess.dwExitCode);

        if (DEBUG_CurrProcess == NULL)
        {
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
        if (DEBUG_CurrThread == NULL)
        {
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
        _strupr(buffer);
        DEBUG_LoadModule32(buffer, de->u.LoadDll.hFile, (DWORD)de->u.LoadDll.lpBaseOfDll);
        DEBUG_CheckDelayedBP();
        if (DBG_IVAR(BreakOnDllLoad))
        {
            DEBUG_Printf(DBG_CHN_MESG, "Stopping on DLL %s loading at %08lx\n",
                         buffer, (unsigned long)de->u.LoadDll.lpBaseOfDll);
            ret = DEBUG_FetchContext();
        }
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        DEBUG_Printf(DBG_CHN_TRACE, "%08lx:%08lx: unload DLL @%08lx\n", de->dwProcessId, de->dwThreadId,
                     (unsigned long)de->u.UnloadDll.lpBaseOfDll);
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        if (DEBUG_CurrThread == NULL)
        {
            DEBUG_Printf(DBG_CHN_ERR, "Unknown thread\n");
            break;
        }

        DEBUG_ProcessGetString(buffer, sizeof(buffer),
                               DEBUG_CurrThread->process->handle,
                               de->u.DebugString.lpDebugStringData);

        /* FIXME unicode de->u.DebugString.fUnicode ? */
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

    return ret;
}

static void                     DEBUG_ResumeDebuggee(DWORD cont)
{
    if (DEBUG_InException)
    {
        DEBUG_ExceptionEpilog();
#if 1
        DEBUG_Printf(DBG_CHN_TRACE,
                     "Exiting debugger      PC=%lx EFL=%08lx mode=%d count=%d\n",
#ifdef __i386__
                     DEBUG_context.Eip, DEBUG_context.EFlags,
#else
                     0L, 0L,
#endif
                     DEBUG_CurrThread->exec_mode, DEBUG_CurrThread->exec_count);
#endif
        if (DEBUG_CurrThread)
        {
            if (!SetThreadContext(DEBUG_CurrThread->handle, &DEBUG_context))
                DEBUG_Printf(DBG_CHN_MESG, "Cannot set ctx on %lu\n", DEBUG_CurrTid);
            DEBUG_CurrThread->wait_for_first_exception = 0;
        }
    }
    DEBUG_InteractiveP = FALSE;
    if (!ContinueDebugEvent(DEBUG_CurrPid, DEBUG_CurrTid, cont))
        DEBUG_Printf(DBG_CHN_MESG, "Cannot continue on %lu (%lu)\n",
                     DEBUG_CurrTid, cont);
}

void                            DEBUG_WaitNextException(DWORD cont, int count, int mode)
{
    DEBUG_EVENT         de;

    if (cont == DBG_CONTINUE)
    {
        DEBUG_CurrThread->exec_count = count;
        DEBUG_CurrThread->exec_mode = mode;
    }
    DEBUG_ResumeDebuggee(cont);

    while (DEBUG_CurrProcess && WaitForDebugEvent(&de, INFINITE))
    {
        if (DEBUG_HandleDebugEvent(&de)) break;
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
    }
    if (!DEBUG_CurrProcess) return;
    DEBUG_InteractiveP = TRUE;
#if 1
    DEBUG_Printf(DBG_CHN_TRACE,
                 "Entering debugger     PC=%lx EFL=%08lx mode=%d count=%d\n",
#ifdef __i386__
                 DEBUG_context.Eip, DEBUG_context.EFlags,
#else
                 0L, 0L,
#endif
                 DEBUG_CurrThread->exec_mode, DEBUG_CurrThread->exec_count);
#endif
}

static	DWORD	DEBUG_MainLoop(void)
{
    DEBUG_EVENT		de;

    DEBUG_Printf(DBG_CHN_MESG, "WineDbg starting on pid %lx\n", DEBUG_CurrPid);

    /* wait for first exception */
    while (WaitForDebugEvent(&de, INFINITE))
    {
        if (DEBUG_HandleDebugEvent(&de)) break;
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
    }
    if (local_mode == automatic_mode)
    {
        /* print some extra information */
        DEBUG_Printf(DBG_CHN_MESG, "Modules:\n");
        DEBUG_WalkModules();
        DEBUG_Printf(DBG_CHN_MESG, "Threads:\n");
        DEBUG_WalkThreads();
    }
    else
    {
        DEBUG_InteractiveP = TRUE;
        DEBUG_Parser(NULL);
    }
    DEBUG_Printf(DBG_CHN_MESG, "WineDbg terminated on pid %lx\n", DEBUG_CurrPid);

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

    /* FIXME: shouldn't need the CREATE_NEW_CONSOLE, but as usual CUI:s need it
     * while GUI:s don't
     */
    if (!CreateProcess(NULL, cmdLine, NULL, NULL,
		       FALSE, DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS|CREATE_NEW_CONSOLE,
                       NULL, NULL, &startup, &info))
    {
	DEBUG_Printf(DBG_CHN_MESG, "Couldn't start process '%s'\n", cmdLine);
	return FALSE;
    }
    DEBUG_CurrPid = info.dwProcessId;
    if (!(DEBUG_CurrProcess = DEBUG_AddProcess(DEBUG_CurrPid, 0, NULL))) return FALSE;

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

static BOOL WINAPI DEBUG_CtrlCHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        DEBUG_Printf(DBG_CHN_MESG, "Ctrl-C: stopping debuggee\n");
        /* FIXME: since we likely have a single process, signal the first process
         * in list
         */
        return DEBUG_ProcessList && DebugBreakProcess(DEBUG_ProcessList->handle);
    }
    return FALSE;
}

static void DEBUG_InitConsole(void)
{
    DWORD	mode;

    /* keep it as a cuiexe for now, so that Wine won't touch the Unix stdin,
     * stdout and stderr streams
     */
    if (DBG_IVAR(UseXTerm))
    {
	FreeConsole();
	AllocConsole();
    }

    /* put the line editing mode with the nice emacs features (FIXME: could be triggered by a IVAR) */
    if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode))
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode | WINE_ENABLE_LINE_INPUT_EMACS);

    /* set our control-C handler */
    SetConsoleCtrlHandler(DEBUG_CtrlCHandler, TRUE);

    /* set our own title */
    SetConsoleTitle("Wine Debugger");
}

static int DEBUG_Usage(void)
{
    DEBUG_Printf(DBG_CHN_MESG, "Usage: winedbg [--debugmsg dbgoptions] [--auto] [--gdb] cmdline\n" );
    return 1;
}

int main(int argc, char** argv)
{
    DWORD	retv = 0;
    unsigned    gdb_flags = 0;

    /* Initialize the type handling stuff. */
    DEBUG_InitTypes();
    DEBUG_InitCVDataTypes();

    /* Initialize internal vars (types must have been initialized before) */
    if (!DEBUG_IntVarsRW(TRUE)) return -1;

    /* parse options */
    while (argc > 1 && argv[1][0] == '-')
    {
        if (!strcmp( argv[1], "--auto" ))
        {
            if (local_mode != none_mode) return DEBUG_Usage();
            local_mode = automatic_mode;
            /* force some internal variables */
            DBG_IVAR(UseXTerm) = 0;
            DBG_IVAR(BreakOnDllLoad) = 0;
            DBG_IVAR(ConChannelMask) = 0;
            DBG_IVAR(StdChannelMask) = DBG_CHN_MESG;
            argc--; argv++;
            continue;
        }
        if (!strcmp( argv[1], "--gdb" ))
        {
            if (local_mode != none_mode) return DEBUG_Usage();
            local_mode = gdb_mode;
            argc--; argv++;
            continue;
        }
        if (strcmp( argv[1], "--no-start") == 0 && local_mode == gdb_mode)
        {
            gdb_flags |= 1;
            argc--; argv++; /* as we don't use argv[0] */
            continue;
        }
        if (strcmp(argv[1], "--with-xterm") == 0 && local_mode == gdb_mode)
        {
            gdb_flags |= 2;
            argc--; argv++; /* as we don't use argv[0] */
            continue;
        }
        if (!strcmp( argv[1], "--debugmsg" ) && argv[2])
        {
            wine_dbg_parse_options( argv[2] );
            argc -= 2;
            argv += 2;
            continue;
        }
        return DEBUG_Usage();
    }

    if (local_mode == none_mode) local_mode = winedbg_mode;

    /* try the from <myself> pid */
    if (DEBUG_CurrPid == 0 && argc == 2)
    {
        char*   ptr;

        DEBUG_CurrPid = strtol(argv[1], &ptr, 10);
        if (DEBUG_CurrPid == 0 || ptr == NULL ||
            !DEBUG_Attach(DEBUG_CurrPid, local_mode != gdb_mode))
            DEBUG_CurrPid = 0;
    }

    /* try the from <myself> pid evt (Win32 JIT debugger) */
    if (DEBUG_CurrPid == 0 && argc == 3)
    {
	HANDLE	hEvent;
	DWORD	pid;
        char*   ptr;

	if ((pid = strtol(argv[1], &ptr, 10)) != 0 && ptr != NULL &&
            (hEvent = (HANDLE)strtol(argv[2], &ptr, 10)) != 0 && ptr != NULL)
        {
	    if (!DEBUG_Attach(pid, TRUE))
            {
		/* don't care about result */
		SetEvent(hEvent);
		goto leave;
	    }
	    if (!SetEvent(hEvent))
            {
		DEBUG_Printf(DBG_CHN_ERR, "Invalid event handle: %p\n", hEvent);
		goto leave;
	    }
            CloseHandle(hEvent);
	    DEBUG_CurrPid = pid;
	}
    }

    if (DEBUG_CurrPid == 0 && argc > 1)
    {
	int	i, len;
	LPSTR	cmdLine;

	if (!(cmdLine = DBG_alloc(len = 1))) goto oom_leave;
	cmdLine[0] = '\0';

	for (i = 1; i < argc; i++)
        {
	    len += strlen(argv[i]) + 1;
	    if (!(cmdLine = DBG_realloc(cmdLine, len))) goto oom_leave;
	    strcat(cmdLine, argv[i]);
	    cmdLine[len - 2] = ' ';
	    cmdLine[len - 1] = '\0';
	}

	if (!DEBUG_Start(cmdLine))
        {
	    DEBUG_Printf(DBG_CHN_MESG, "Couldn't start process '%s'\n", cmdLine);
	    goto leave;
	}
	DBG_free(DEBUG_LastCmdLine);
	DEBUG_LastCmdLine = cmdLine;
    }
    /* don't save local vars in gdb mode */
    if (local_mode == gdb_mode && DEBUG_CurrPid)
        return DEBUG_GdbRemote(gdb_flags);

    DEBUG_InitConsole();

    retv = DEBUG_MainLoop();
    /* don't save modified variables in auto mode */
    if (local_mode != automatic_mode) DEBUG_IntVarsRW(FALSE);

 leave:
    return retv;

 oom_leave:
    DEBUG_Printf(DBG_CHN_MESG, "Out of memory\n");
    goto leave;
}
