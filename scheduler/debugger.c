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
    BOOL ret;
    SERVER_START_REQ
    {
        debug_event_t *data;
        struct wait_debug_event_request *req = server_alloc_req( sizeof(*req), sizeof(*data) );

        req->timeout = timeout;
        if (!(ret = !server_call( REQ_WAIT_DEBUG_EVENT ))) goto done;

        if (!server_data_size(req))  /* timeout */
        {
            SetLastError( ERROR_SEM_TIMEOUT );
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
        default:
            server_protocol_error( "WaitForDebugEvent: bad code %d\n", data->code );
        }
    done:
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           ContinueDebugEvent   (KERNEL32.146)
 */
BOOL WINAPI ContinueDebugEvent( DWORD pid, DWORD tid, DWORD status )
{
    BOOL ret;
    SERVER_START_REQ
    {
        struct continue_debug_event_request *req = server_alloc_req( sizeof(*req), 0 );
        req->pid    = (void *)pid;
        req->tid    = (void *)tid;
        req->status = status;
        ret = !server_call( REQ_CONTINUE_DEBUG_EVENT );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *           DebugActiveProcess   (KERNEL32.180)
 */
BOOL WINAPI DebugActiveProcess( DWORD pid )
{
    BOOL ret;
    SERVER_START_REQ
    {
        struct debug_process_request *req = server_alloc_req( sizeof(*req), 0 );
        req->pid = (void *)pid;
        ret = !server_call( REQ_DEBUG_PROCESS );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           OutputDebugStringA   (KERNEL32.548)
 */
void WINAPI OutputDebugStringA( LPCSTR str )
{
    SERVER_START_REQ
    {
        struct output_debug_string_request *req = server_alloc_req( sizeof(*req), 0 );
        req->string  = (void *)str;
        req->unicode = 0;
        req->length  = strlen(str) + 1;
        server_call_noerr( REQ_OUTPUT_DEBUG_STRING );
    }
    SERVER_END_REQ;
    WARN("%s\n", str);
}


/***********************************************************************
 *           OutputDebugStringW   (KERNEL32.549)
 */
void WINAPI OutputDebugStringW( LPCWSTR str )
{
    SERVER_START_REQ
    {
        struct output_debug_string_request *req = server_alloc_req( sizeof(*req), 0 );
        req->string  = (void *)str;
        req->unicode = 1;
        req->length  = (lstrlenW(str) + 1) * sizeof(WCHAR);
        server_call_noerr( REQ_OUTPUT_DEBUG_STRING );
    }
    SERVER_END_REQ;
    WARN("%s\n", debugstr_w(str));
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
    SERVER_START_REQ
    {
        struct get_process_info_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = GetCurrentProcess();
        if (!server_call( REQ_GET_PROCESS_INFO )) ret = req->debugged;
    }
    SERVER_END_REQ;
    return ret;
}
