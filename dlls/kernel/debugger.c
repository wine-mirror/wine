/*
 * Win32 debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <stdio.h>
#include <string.h>

#include "winerror.h"
#include "wine/winbase16.h"
#include "server.h"
#include "stackframe.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(debugstr);


/******************************************************************************
 *           WaitForDebugEvent   (KERNEL32.@)
 *
 *  Waits for a debugging event to occur in a process being debugged before
 *  filling out the debug event structure.
 *
 * RETURNS
 *
 *  Returns true if a debug event occurred and false if the call timed out.
 */
BOOL WINAPI WaitForDebugEvent(
    LPDEBUG_EVENT event,   /* [out] Address of structure for event information. */
    DWORD         timeout) /* [in] Number of milliseconds to wait for event. */
{
    BOOL ret;
    DWORD res;

    for (;;)
    {
        HANDLE wait = 0;
        debug_event_t *data;
        SERVER_START_VAR_REQ( wait_debug_event, sizeof(*data) )
        {
            req->get_handle = (timeout != 0);
            if (!(ret = !SERVER_CALL_ERR())) goto done;

            if (!server_data_size(req))  /* timeout */
            {
                wait = req->wait;
                ret = FALSE;
                goto done;
            }
            data = server_data_ptr(req);
            event->dwDebugEventCode = data->code;
            event->dwProcessId      = (DWORD)req->pid;
            event->dwThreadId       = (DWORD)req->tid;
            switch(data->code)
            {
            case EXCEPTION_DEBUG_EVENT:
                event->u.Exception.ExceptionRecord = data->info.exception.record;
                event->u.Exception.dwFirstChance   = data->info.exception.first;
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                event->u.CreateThread.hThread           = data->info.create_thread.handle;
                event->u.CreateThread.lpThreadLocalBase = data->info.create_thread.teb;
                event->u.CreateThread.lpStartAddress    = data->info.create_thread.start;
                break;
            case CREATE_PROCESS_DEBUG_EVENT:
                event->u.CreateProcessInfo.hFile                 = data->info.create_process.file;
                event->u.CreateProcessInfo.hProcess              = data->info.create_process.process;
                event->u.CreateProcessInfo.hThread               = data->info.create_process.thread;
                event->u.CreateProcessInfo.lpBaseOfImage         = data->info.create_process.base;
                event->u.CreateProcessInfo.dwDebugInfoFileOffset = data->info.create_process.dbg_offset;
                event->u.CreateProcessInfo.nDebugInfoSize        = data->info.create_process.dbg_size;
                event->u.CreateProcessInfo.lpThreadLocalBase     = data->info.create_process.teb;
                event->u.CreateProcessInfo.lpStartAddress        = data->info.create_process.start;
                event->u.CreateProcessInfo.lpImageName           = data->info.create_process.name;
                event->u.CreateProcessInfo.fUnicode              = data->info.create_process.unicode;
                if (data->info.create_process.file == -1) event->u.CreateProcessInfo.hFile = 0;
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                event->u.ExitThread.dwExitCode = data->info.exit.exit_code;
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                event->u.ExitProcess.dwExitCode = data->info.exit.exit_code;
                break;
            case LOAD_DLL_DEBUG_EVENT:
                event->u.LoadDll.hFile                 = data->info.load_dll.handle;
                event->u.LoadDll.lpBaseOfDll           = data->info.load_dll.base;
                event->u.LoadDll.dwDebugInfoFileOffset = data->info.load_dll.dbg_offset;
                event->u.LoadDll.nDebugInfoSize        = data->info.load_dll.dbg_size;
                event->u.LoadDll.lpImageName           = data->info.load_dll.name;
                event->u.LoadDll.fUnicode              = data->info.load_dll.unicode;
                if (data->info.load_dll.handle == -1) event->u.LoadDll.hFile = 0;
                break;
            case UNLOAD_DLL_DEBUG_EVENT:
                event->u.UnloadDll.lpBaseOfDll = data->info.unload_dll.base;
                break;
            case OUTPUT_DEBUG_STRING_EVENT:
                event->u.DebugString.lpDebugStringData  = data->info.output_string.string;
                event->u.DebugString.fUnicode           = data->info.output_string.unicode;
                event->u.DebugString.nDebugStringLength = data->info.output_string.length;
                break;
            case RIP_EVENT:
                event->u.RipInfo.dwError = data->info.rip_info.error;
                event->u.RipInfo.dwType  = data->info.rip_info.type;
                break;
            }
        done:
        }
        SERVER_END_VAR_REQ;
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
 *           ContinueDebugEvent   (WIN32S16.5)
 *
 *  Enables a thread that previously produced a debug event to continue.
 *
 * RETURNS
 *
 *  True if the debugger is listed as the processes owner and the process
 *  and thread are valid.
 */
BOOL WINAPI ContinueDebugEvent(
    DWORD pid,    /* [in] The id of the process to continue. */
    DWORD tid,    /* [in] The id of the thread to continue. */
    DWORD status) /* [in] The rule to apply to unhandled exeptions. */
{
    BOOL ret;
    SERVER_START_REQ( continue_debug_event )
    {
        req->pid    = (void *)pid;
        req->tid    = (void *)tid;
        req->status = status;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           DebugActiveProcess   (KERNEL32.@)
 *
 *  Attempts to attach the dugger to a process.
 *
 * RETURNS
 *
 *  True if the debugger was attached to process.
 */
BOOL WINAPI DebugActiveProcess(
    DWORD pid) /* [in] The process to be debugged. */
{
    BOOL ret;
    SERVER_START_REQ( debug_process )
    {
        req->pid = (void *)pid;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           OutputDebugStringA   (KERNEL32.@)
 *
 *  Output by an application of a unicode string to a debugger (if attached)
 *  and program log.
 */
void WINAPI OutputDebugStringA(
    LPCSTR str) /* [in] The message to be logged and given to the debugger. */
{
    SERVER_START_REQ( output_debug_string )
    {
        req->string  = (void *)str;
        req->unicode = 0;
        req->length  = strlen(str) + 1;
        SERVER_CALL();
    }
    SERVER_END_REQ;
    WARN("%s\n", str);
}


/***********************************************************************
 *           OutputDebugStringW   (KERNEL32.@)
 *
 *  Output by an appliccation of a unicode string to a debugger (if attached)
 *  and program log.
 */
void WINAPI OutputDebugStringW(
    LPCWSTR str) /* [in] The message to be logged and given to the debugger. */
{
    SERVER_START_REQ( output_debug_string )
    {
        req->string  = (void *)str;
        req->unicode = 1;
        req->length  = (lstrlenW(str) + 1) * sizeof(WCHAR);
        SERVER_CALL();
    }
    SERVER_END_REQ;
    WARN("%s\n", debugstr_w(str));
}


/***********************************************************************
 *           OutputDebugString16   (KERNEL.115)
 *
 *  Output by a 16 bit application of an ascii string to a debugger (if attached)
 *  and program log.
 */
void WINAPI OutputDebugString16(
    LPCSTR str) /* [in] The message to be logged and given to the debugger. */
{
    OutputDebugStringA( str );
}


/***********************************************************************
 *           DebugBreak   (KERNEL32.@)
 *
 *  Raises an exception so that a debugger (if attached)
 *  can take some action.
 */
void WINAPI DebugBreak(void)
{
    DbgBreakPoint();
}


/***********************************************************************
 *           DebugBreak16   (KERNEL.203)
 *
 *  Raises an expection in a 16 bit application so that a debugger (if attached)
 *  can take some action.
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
 * RETURNS
 *
 *  True if there is a debugger attached.
 */
BOOL WINAPI IsDebuggerPresent(void)
{
    BOOL ret = FALSE;
    SERVER_START_REQ( get_process_info )
    {
        req->handle = GetCurrentProcess();
        if (!SERVER_CALL_ERR()) ret = req->debugged;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           _DebugOutput                    (KERNEL.328)
 */
void WINAPIV _DebugOutput( void )
{
    VA_LIST16 valist;
    WORD flags;
    SEGPTR spec;
    char caller[101];

    /* Decode caller address */
    if (!GetModuleName16( GetExePtr(CURRENT_STACK16->cs), caller, sizeof(caller) ))
        sprintf( caller, "%04X:%04X", CURRENT_STACK16->cs, CURRENT_STACK16->ip );

    /* Build debug message string */
    VA_START16( valist );
    flags = VA_ARG16( valist, WORD );
    spec  = VA_ARG16( valist, SEGPTR );
    /* FIXME: cannot use wvsnprintf16 from kernel */
    /* wvsnprintf16( temp, sizeof(temp), MapSL(spec), valist ); */

    /* Output */
    FIXME("%s %04x %s\n", caller, flags, debugstr_a(MapSL(spec)) );
}
