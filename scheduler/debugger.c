/*
 * Win32 debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <string.h>

#include "winerror.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(debugstr);


/******************************************************************************
 *           WaitForDebugEvent   (KERNEL32.720)
 *
 * Waits for a debugging event to occur in a process being debugged
 *
 * PARAMS
 *    event   [I] Address of structure for event information
 *    timeout [I] Number of milliseconds to wait for event
 *
 * RETURNS STD
 */
BOOL WINAPI WaitForDebugEvent( LPDEBUG_EVENT event, DWORD timeout )
{
    struct wait_debug_event_request *req = get_req_buffer();

    req->timeout = timeout;
    if (server_call( REQ_WAIT_DEBUG_EVENT )) return FALSE;
    if ((req->event.code < 0) || (req->event.code > RIP_EVENT))
        server_protocol_error( "WaitForDebugEvent: bad code %d\n", req->event.code );

    event->dwDebugEventCode = req->event.code;
    event->dwProcessId      = (DWORD)req->pid;
    event->dwThreadId       = (DWORD)req->tid;
    switch(req->event.code)
    {
    case 0:  /* timeout */
        SetLastError( ERROR_SEM_TIMEOUT );
        return FALSE;
    case EXCEPTION_DEBUG_EVENT:
        event->u.Exception.ExceptionRecord = req->event.info.exception.record;
        event->u.Exception.dwFirstChance   = req->event.info.exception.first;
        break;
    case CREATE_THREAD_DEBUG_EVENT:
        event->u.CreateThread.hThread           = req->event.info.create_thread.handle;
        event->u.CreateThread.lpThreadLocalBase = req->event.info.create_thread.teb;
        event->u.CreateThread.lpStartAddress    = req->event.info.create_thread.start;
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        event->u.CreateProcessInfo.hFile                 = req->event.info.create_process.file;
        event->u.CreateProcessInfo.hProcess              = req->event.info.create_process.process;
        event->u.CreateProcessInfo.hThread               = req->event.info.create_process.thread;
        event->u.CreateProcessInfo.lpBaseOfImage         = req->event.info.create_process.base;
        event->u.CreateProcessInfo.dwDebugInfoFileOffset = req->event.info.create_process.dbg_offset;
        event->u.CreateProcessInfo.nDebugInfoSize        = req->event.info.create_process.dbg_size;
        event->u.CreateProcessInfo.lpThreadLocalBase     = req->event.info.create_process.teb;
        event->u.CreateProcessInfo.lpStartAddress        = req->event.info.create_process.start;
        event->u.CreateProcessInfo.lpImageName           = req->event.info.create_process.name;
        event->u.CreateProcessInfo.fUnicode              = req->event.info.create_process.unicode;
        if (req->event.info.create_process.file == -1) event->u.CreateProcessInfo.hFile = 0;
        break;
    case EXIT_THREAD_DEBUG_EVENT:
        event->u.ExitThread.dwExitCode = req->event.info.exit.exit_code;
        break;
    case EXIT_PROCESS_DEBUG_EVENT:
        event->u.ExitProcess.dwExitCode = req->event.info.exit.exit_code;
        break;
    case LOAD_DLL_DEBUG_EVENT:
        event->u.LoadDll.hFile                 = req->event.info.load_dll.handle;
        event->u.LoadDll.lpBaseOfDll           = req->event.info.load_dll.base;
        event->u.LoadDll.dwDebugInfoFileOffset = req->event.info.load_dll.dbg_offset;
        event->u.LoadDll.nDebugInfoSize        = req->event.info.load_dll.dbg_size;
        event->u.LoadDll.lpImageName           = req->event.info.load_dll.name;
        event->u.LoadDll.fUnicode              = req->event.info.load_dll.unicode;
        if (req->event.info.load_dll.handle == -1) event->u.LoadDll.hFile = 0;
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        event->u.UnloadDll.lpBaseOfDll = req->event.info.unload_dll.base;
        break;
    case OUTPUT_DEBUG_STRING_EVENT:
        event->u.DebugString.lpDebugStringData  = req->event.info.output_string.string;
        event->u.DebugString.fUnicode           = req->event.info.output_string.unicode;
        event->u.DebugString.nDebugStringLength = req->event.info.output_string.length;
        break;
    case RIP_EVENT:
        event->u.RipInfo.dwError = req->event.info.rip_info.error;
        event->u.RipInfo.dwType  = req->event.info.rip_info.type;
        break;
    }
    return TRUE;
}


/**********************************************************************
 *           ContinueDebugEvent   (KERNEL32.146)
 */
BOOL WINAPI ContinueDebugEvent( DWORD pid, DWORD tid, DWORD status )
{
    struct continue_debug_event_request *req = get_req_buffer();
    req->pid    = (void *)pid;
    req->tid    = (void *)tid;
    req->status = status;
    return !server_call( REQ_CONTINUE_DEBUG_EVENT );
}


/**********************************************************************
 *           DebugActiveProcess   (KERNEL32.180)
 */
BOOL WINAPI DebugActiveProcess( DWORD pid )
{
    struct debug_process_request *req = get_req_buffer();
    req->pid = (void *)pid;
    return !server_call( REQ_DEBUG_PROCESS );
}


/***********************************************************************
 *           OutputDebugStringA   (KERNEL32.548)
 */
void WINAPI OutputDebugStringA( LPCSTR str )
{
    struct output_debug_string_request *req = get_req_buffer();
    req->string  = (void *)str;
    req->unicode = 0;
    req->length  = strlen(str) + 1;
    server_call_noerr( REQ_OUTPUT_DEBUG_STRING );
    TRACE("%s\n", str);
}


/***********************************************************************
 *           OutputDebugStringW   (KERNEL32.549)
 */
void WINAPI OutputDebugStringW( LPCWSTR str )
{
    struct output_debug_string_request *req = get_req_buffer();
    req->string  = (void *)str;
    req->unicode = 1;
    req->length  = (lstrlenW(str) + 1) * sizeof(WCHAR);
    server_call_noerr( REQ_OUTPUT_DEBUG_STRING );
    TRACE("%s\n", debugstr_w(str));
}


/***********************************************************************
 *           OutputDebugString16   (KERNEL.115)
 */
void WINAPI OutputDebugString16( LPCSTR str )
{
    OutputDebugStringA( str );
}


/***********************************************************************
 *           DebugBreak   (KERNEL32.181)
 */
void WINAPI DebugBreak(void)
{
    DbgBreakPoint();
}


/***********************************************************************
 *           DebugBreak16   (KERNEL.203)
 */
void WINAPI DebugBreak16( CONTEXT86 *context )
{
#ifdef __i386__
    EXCEPTION_RECORD rec;

    rec.ExceptionCode    = EXCEPTION_BREAKPOINT;
    rec.ExceptionFlags   = 0;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = GET_IP(context); 
    rec.NumberParameters = 0;
    NtRaiseException( &rec, context, TRUE );
#endif  /* defined(__i386__) */
}


/***********************************************************************
 *           IsDebuggerPresent   (KERNEL32)
 */
BOOL WINAPI IsDebuggerPresent(void)
{
    BOOL ret = FALSE;
    struct get_process_info_request *req = get_req_buffer();
    req->handle = GetCurrentProcess();
    if (!server_call( REQ_GET_PROCESS_INFO )) ret = req->debugged;
    return ret;
}
