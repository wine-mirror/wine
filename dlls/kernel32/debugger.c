/*
 * Win32 debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include <stdio.h>
#include <string.h>

#include "winerror.h"
#include "wine/winbase16.h"
#include "wine/server.h"
#include "kernel_private.h"
#include "kernel16_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(debugstr);


/******************************************************************************
 *           WaitForDebugEvent   (KERNEL32.@)
 *
 *  Waits for a debugging event to occur in a process being debugged before
 *  filling out the debug event structure.
 *
 * PARAMS
 *  event   [O] Address of structure for event information.
 *  timeout [I] Number of milliseconds to wait for event.
 *
 * RETURNS
 *
 *  Returns true if a debug event occurred and false if the call timed out.
 */
BOOL WINAPI WaitForDebugEvent(
    LPDEBUG_EVENT event,
    DWORD         timeout)
{
    BOOL ret;
    DWORD res;

    for (;;)
    {
        HANDLE wait = 0;
        debug_event_t data;
        SERVER_START_REQ( wait_debug_event )
        {
            req->get_handle = (timeout != 0);
            wine_server_set_reply( req, &data, sizeof(data) );
            if (!(ret = !wine_server_call_err( req ))) goto done;

            if (!wine_server_reply_size(reply))  /* timeout */
            {
                wait = reply->wait;
                ret = FALSE;
                goto done;
            }
            event->dwDebugEventCode = data.code;
            event->dwProcessId      = (DWORD)reply->pid;
            event->dwThreadId       = (DWORD)reply->tid;
            switch(data.code)
            {
            case EXCEPTION_DEBUG_EVENT:
                event->u.Exception.ExceptionRecord = data.info.exception.record;
                event->u.Exception.dwFirstChance   = data.info.exception.first;
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                event->u.CreateThread.hThread           = data.info.create_thread.handle;
                event->u.CreateThread.lpThreadLocalBase = data.info.create_thread.teb;
                event->u.CreateThread.lpStartAddress    = data.info.create_thread.start;
                break;
            case CREATE_PROCESS_DEBUG_EVENT:
                event->u.CreateProcessInfo.hFile                 = data.info.create_process.file;
                event->u.CreateProcessInfo.hProcess              = data.info.create_process.process;
                event->u.CreateProcessInfo.hThread               = data.info.create_process.thread;
                event->u.CreateProcessInfo.lpBaseOfImage         = data.info.create_process.base;
                event->u.CreateProcessInfo.dwDebugInfoFileOffset = data.info.create_process.dbg_offset;
                event->u.CreateProcessInfo.nDebugInfoSize        = data.info.create_process.dbg_size;
                event->u.CreateProcessInfo.lpThreadLocalBase     = data.info.create_process.teb;
                event->u.CreateProcessInfo.lpStartAddress        = data.info.create_process.start;
                event->u.CreateProcessInfo.lpImageName           = data.info.create_process.name;
                event->u.CreateProcessInfo.fUnicode              = data.info.create_process.unicode;
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                event->u.ExitThread.dwExitCode = data.info.exit.exit_code;
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                event->u.ExitProcess.dwExitCode = data.info.exit.exit_code;
                break;
            case LOAD_DLL_DEBUG_EVENT:
                event->u.LoadDll.hFile                 = data.info.load_dll.handle;
                event->u.LoadDll.lpBaseOfDll           = data.info.load_dll.base;
                event->u.LoadDll.dwDebugInfoFileOffset = data.info.load_dll.dbg_offset;
                event->u.LoadDll.nDebugInfoSize        = data.info.load_dll.dbg_size;
                event->u.LoadDll.lpImageName           = data.info.load_dll.name;
                event->u.LoadDll.fUnicode              = data.info.load_dll.unicode;
                break;
            case UNLOAD_DLL_DEBUG_EVENT:
                event->u.UnloadDll.lpBaseOfDll = data.info.unload_dll.base;
                break;
            case OUTPUT_DEBUG_STRING_EVENT:
                event->u.DebugString.lpDebugStringData  = data.info.output_string.string;
                event->u.DebugString.fUnicode           = data.info.output_string.unicode;
                event->u.DebugString.nDebugStringLength = data.info.output_string.length;
                break;
            case RIP_EVENT:
                event->u.RipInfo.dwError = data.info.rip_info.error;
                event->u.RipInfo.dwType  = data.info.rip_info.type;
                break;
            }
        done:
            /* nothing */ ;
        }
        SERVER_END_REQ;
        if (ret) return TRUE;
        if (!wait) break;
        res = WaitForSingleObject( wait, timeout );
        CloseHandle( wait );
        if (res != STATUS_WAIT_0) break;
    }
    SetLastError( ERROR_SEM_TIMEOUT );
    return FALSE;
}


/**********************************************************************
 *           ContinueDebugEvent   (KERNEL32.@)
 *
 *  Enables a thread that previously produced a debug event to continue.
 *
 * PARAMS
 *  pid    [I] The id of the process to continue.
 *  tid    [I] The id of the thread to continue.
 *  status [I] The rule to apply to unhandled exeptions.
 *
 * RETURNS
 *
 *  True if the debugger is listed as the processes owner and the process
 *  and thread are valid.
 */
BOOL WINAPI ContinueDebugEvent(
    DWORD pid,
    DWORD tid,
    DWORD status)
{
    BOOL ret;
    SERVER_START_REQ( continue_debug_event )
    {
        req->pid    = pid;
        req->tid    = tid;
        req->status = status;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           DebugActiveProcess   (KERNEL32.@)
 *
 *  Attempts to attach the debugger to a process.
 *
 * PARAMS
 *  pid [I] The process to be debugged.
 *
 * RETURNS
 *
 *  True if the debugger was attached to process.
 */
BOOL WINAPI DebugActiveProcess( DWORD pid )
{
    BOOL ret;
    SERVER_START_REQ( debug_process )
    {
        req->pid = pid;
        req->attach = 1;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/**********************************************************************
 *           DebugActiveProcessStop   (KERNEL32.@)
 *
 *  Attempts to detach the debugger from a process.
 *
 * PARAMS
 *  pid [I] The process to be detached.
 *
 * RETURNS
 *
 *  True if the debugger was detached from the process.
 */
BOOL WINAPI DebugActiveProcessStop( DWORD pid )
{
    BOOL ret;
    SERVER_START_REQ( debug_process )
    {
        req->pid = pid;
        req->attach = 0;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           OutputDebugStringA   (KERNEL32.@)
 *
 *  Output by an application of an ascii string to a debugger (if attached)
 *  and program log.
 *
 * PARAMS
 *  str [I] The message to be logged and given to the debugger.
 *
 * RETURNS
 *
 *  Nothing.
 */
void WINAPI OutputDebugStringA( LPCSTR str )
{
    SERVER_START_REQ( output_debug_string )
    {
        req->string  = (void *)str;
        req->unicode = 0;
        req->length  = strlen(str) + 1;
        wine_server_call( req );
    }
    SERVER_END_REQ;
    WARN("%s\n", str);
}


/***********************************************************************
 *           OutputDebugStringW   (KERNEL32.@)
 *
 *  Output by an application of a unicode string to a debugger (if attached)
 *  and program log.
 *
 * PARAMS
 *  str [I] The message to be logged and given to the debugger.
 *
 * RETURNS
 *
 *  Nothing.
 */
void WINAPI OutputDebugStringW( LPCWSTR str )
{
    SERVER_START_REQ( output_debug_string )
    {
        req->string  = (void *)str;
        req->unicode = 1;
        req->length  = (lstrlenW(str) + 1) * sizeof(WCHAR);
        wine_server_call( req );
    }
    SERVER_END_REQ;
    WARN("%s\n", debugstr_w(str));
}


/***********************************************************************
 *           OutputDebugString   (KERNEL.115)
 *
 *  Output by a 16 bit application of an ascii string to a debugger (if attached)
 *  and program log.
 *
 * PARAMS
 *  str [I] The message to be logged and given to the debugger.
 *
 * RETURNS
 */
void WINAPI OutputDebugString16( LPCSTR str )
{
    OutputDebugStringA( str );
}


/***********************************************************************
 *           DebugBreak   (KERNEL32.@)
 *
 *  Raises an exception so that a debugger (if attached)
 *  can take some action.
 *
 * PARAMS
 *
 * RETURNS
 */
void WINAPI DebugBreak(void)
{
    DbgBreakPoint();
}

/***********************************************************************
 *           DebugBreakProcess   (KERNEL32.@)
 *
 *  Raises an exception so that a debugger (if attached)
 *  can take some action. Same as DebugBreak, but applies to any process.
 *
 * PARAMS
 *  hProc [I] Process to break into.
 *
 * RETURNS
 *
 *  True if successful.
 */
BOOL WINAPI DebugBreakProcess(HANDLE hProc)
{
    BOOL ret, self;

    TRACE("(%p)\n", hProc);

    SERVER_START_REQ( debug_break )
    {
        req->handle = hProc;
        ret = !wine_server_call_err( req );
        self = ret && reply->self;
    }
    SERVER_END_REQ;
    if (self) DbgBreakPoint();
    return ret;
}


/***********************************************************************
 *           DebugBreak   (KERNEL.203)
 *
 *  Raises an exception in a 16 bit application so that a debugger (if attached)
 *  can take some action.
 *
 * PARAMS
 *
 * RETURNS
 *
 * BUGS
 *
 *  Only 386 compatible processors implemented.
 */
void WINAPI DebugBreak16(
    CONTEXT86 *context) /* [in/out] A pointer to the 386 compatible processor state. */
{
#ifdef __i386__
    EXCEPTION_RECORD rec;

    rec.ExceptionCode    = EXCEPTION_BREAKPOINT;
    rec.ExceptionFlags   = 0;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context->Eip;
    rec.NumberParameters = 0;
    NtRaiseException( &rec, context, TRUE );
#endif  /* defined(__i386__) */
}


/***********************************************************************
 *           IsDebuggerPresent   (KERNEL32.@)
 *
 *  Allows a process to determine if there is a debugger attached.
 *
 * PARAMS
 *
 * RETURNS
 *
 *  True if there is a debugger attached.
 */
BOOL WINAPI IsDebuggerPresent(void)
{
    return NtCurrentTeb()->Peb->BeingDebugged;
}

/***********************************************************************
 *           CheckRemoteDebuggerPresent   (KERNEL32.@)
 *
 *  Allows a process to determine if there is a remote debugger
 *  attached.
 *
 * PARAMS
 *
 * RETURNS
 *
 *  TRUE because it is a stub.
 */
BOOL WINAPI CheckRemoteDebuggerPresent(HANDLE process, PBOOL DebuggerPresent)
{
    FIXME("(%p)->(%p): Stub!\n", process, DebuggerPresent);
    *DebuggerPresent = FALSE;
    return TRUE;
}

/***********************************************************************
 *           _DebugOutput                    (KERNEL.328)
 */
void WINAPIV _DebugOutput( WORD flags, LPCSTR spec, VA_LIST16 valist )
{
    char caller[101];

    /* Decode caller address */
    if (!GetModuleName16( GetExePtr(CURRENT_STACK16->cs), caller, sizeof(caller) ))
        sprintf( caller, "%04X:%04X", CURRENT_STACK16->cs, CURRENT_STACK16->ip );

    /* FIXME: cannot use wvsnprintf16 from kernel */
    /* wvsnprintf16( temp, sizeof(temp), spec, valist ); */

    /* Output */
    FIXME("%s %04x %s\n", caller, flags, debugstr_a(spec) );
}

/***********************************************************************
 *           DebugSetProcessKillOnExit                    (KERNEL32.@)
 *
 * Let a debugger decide whether a debuggee will be killed upon debugger
 * termination.
 *
 * PARAMS
 *  kill [I] If set to true then kill the process on exit.
 *
 * RETURNS
 *  True if successful, false otherwise.
 */
BOOL WINAPI DebugSetProcessKillOnExit(BOOL kill)
{
    BOOL ret = FALSE;

    SERVER_START_REQ( set_debugger_kill_on_exit )
    {
        req->kill_on_exit = kill;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}
