/*
 * Win32 debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#include <string.h>

#include "process.h"
#include "thread.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(debugstr);


/**********************************************************************
 *           DEBUG_SendExceptionEvent
 *
 * Send an EXCEPTION_DEBUG_EVENT event to the current process debugger.
 */
DWORD DEBUG_SendExceptionEvent( EXCEPTION_RECORD *rec, BOOL first_chance, CONTEXT *context )
{
    int i;
    DWORD ret = 0;
    struct send_debug_event_request *req = get_req_buffer();

    req->event.code = EXCEPTION_DEBUG_EVENT;
    req->event.info.exception.code         = rec->ExceptionCode;
    req->event.info.exception.flags        = rec->ExceptionFlags;
    req->event.info.exception.record       = rec->ExceptionRecord;
    req->event.info.exception.addr         = rec->ExceptionAddress;
    req->event.info.exception.nb_params    = rec->NumberParameters;
    req->event.info.exception.first_chance = first_chance;
    req->event.info.exception.context      = *context;
    for (i = 0; i < req->event.info.exception.nb_params; i++)
        req->event.info.exception.params[i] = rec->ExceptionInformation[i];
    if (!server_call( REQ_SEND_DEBUG_EVENT ))
    {
        ret = req->status;
        *context = req->event.info.exception.context;
    }
    return ret;
}


/**********************************************************************
 *           DEBUG_SendCreateProcessEvent
 *
 * Send an CREATE_PROCESS_DEBUG_EVENT event to the current process debugger.
 * Must be called from the context of the new process.
 */
DWORD DEBUG_SendCreateProcessEvent( HFILE file, HMODULE module, void *entry )
{
    DWORD ret = 0;
    struct send_debug_event_request *req = get_req_buffer();

    req->event.code = CREATE_PROCESS_DEBUG_EVENT;
    req->event.info.create_process.file       = file;
    req->event.info.create_process.process    = 0; /* will be filled by server */
    req->event.info.create_process.thread     = 0; /* will be filled by server */
    req->event.info.create_process.base       = (void *)module;
    req->event.info.create_process.dbg_offset = 0; /* FIXME */
    req->event.info.create_process.dbg_size   = 0; /* FIXME */
    req->event.info.create_process.teb        = NtCurrentTeb();
    req->event.info.create_process.start      = entry;
    req->event.info.create_process.name       = 0; /* FIXME */
    req->event.info.create_process.unicode    = 0; /* FIXME */
    if (!server_call( REQ_SEND_DEBUG_EVENT )) ret = req->status;
    return ret;
}

/**********************************************************************
 *           DEBUG_SendCreateThreadEvent
 *
 * Send an CREATE_THREAD_DEBUG_EVENT event to the current process debugger.
 * Must be called from the context of the new thread.
 */
DWORD DEBUG_SendCreateThreadEvent( void *entry )
{
    DWORD ret = 0;
    struct send_debug_event_request *req = get_req_buffer();

    req->event.code = CREATE_THREAD_DEBUG_EVENT;
    req->event.info.create_thread.handle = 0; /* will be filled by server */
    req->event.info.create_thread.teb    = NtCurrentTeb();
    req->event.info.create_thread.start  = entry;
    if (!server_call( REQ_SEND_DEBUG_EVENT )) ret = req->status;
    return ret;
}


/**********************************************************************
 *           DEBUG_SendLoadDLLEvent
 *
 * Send an LOAD_DLL_DEBUG_EVENT event to the current process debugger.
 */
DWORD DEBUG_SendLoadDLLEvent( HFILE file, HMODULE module, LPSTR *name )
{
    DWORD ret = 0;
    struct send_debug_event_request *req = get_req_buffer();

    req->event.code = LOAD_DLL_DEBUG_EVENT;
    req->event.info.load_dll.handle     = file;
    req->event.info.load_dll.base       = (void *)module;
    req->event.info.load_dll.dbg_offset = 0;  /* FIXME */
    req->event.info.load_dll.dbg_size   = 0;  /* FIXME */
    req->event.info.load_dll.name       = name;
    req->event.info.load_dll.unicode    = 0;
    if (!server_call( REQ_SEND_DEBUG_EVENT )) ret = req->status;
    return ret;
}


/**********************************************************************
 *           DEBUG_SendUnloadDLLEvent
 *
 * Send an UNLOAD_DLL_DEBUG_EVENT event to the current process debugger.
 */
DWORD DEBUG_SendUnloadDLLEvent( HMODULE module )
{
    DWORD ret = 0;
    struct send_debug_event_request *req = get_req_buffer();

    req->event.code = UNLOAD_DLL_DEBUG_EVENT;
    req->event.info.unload_dll.base = (void *)module;
    if (!server_call( REQ_SEND_DEBUG_EVENT )) ret = req->status;
    return ret;
}


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
    int i;

    req->timeout = timeout;
    if (server_call( REQ_WAIT_DEBUG_EVENT )) return FALSE;
    if ((req->event.code < 0) || (req->event.code > RIP_EVENT))
        server_protocol_error( "WaitForDebugEvent: bad code %d\n", req->event.code );

    event->dwDebugEventCode = req->event.code;
    event->dwProcessId      = (DWORD)req->pid;
    event->dwThreadId       = (DWORD)req->tid;
    switch(req->event.code)
    {
    case EXCEPTION_DEBUG_EVENT:
        event->u.Exception.ExceptionRecord.ExceptionCode    = req->event.info.exception.code;
        event->u.Exception.ExceptionRecord.ExceptionFlags   = req->event.info.exception.flags;
        event->u.Exception.ExceptionRecord.ExceptionRecord  = req->event.info.exception.record;
        event->u.Exception.ExceptionRecord.ExceptionAddress = req->event.info.exception.addr;
        event->u.Exception.ExceptionRecord.NumberParameters = req->event.info.exception.nb_params;
        for (i = 0; i < req->event.info.exception.nb_params; i++)
            event->u.Exception.ExceptionRecord.ExceptionInformation[i] = req->event.info.exception.params[i];
        event->u.Exception.dwFirstChance = req->event.info.exception.first_chance;
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
    if (PROCESS_Current()->flags & PDB32_DEBUGGED)
    {
        struct send_debug_event_request *req = get_req_buffer();
        req->event.code = OUTPUT_DEBUG_STRING_EVENT;
        req->event.info.output_string.string  = (void *)str;
        req->event.info.output_string.unicode = 0;
        req->event.info.output_string.length  = strlen(str) + 1;
        server_call( REQ_SEND_DEBUG_EVENT );
    }

    TRACE("%s\n", str);
}


/***********************************************************************
 *           OutputDebugStringW   (KERNEL32.549)
 */
void WINAPI OutputDebugStringW( LPCWSTR str )
{
    if (PROCESS_Current()->flags & PDB32_DEBUGGED)
    {
        struct send_debug_event_request *req = get_req_buffer();
        req->event.code = OUTPUT_DEBUG_STRING_EVENT;
        req->event.info.output_string.string  = (void *)str;
        req->event.info.output_string.unicode = 1;
        req->event.info.output_string.length  = (lstrlenW(str) + 1) * sizeof(WCHAR);
        server_call( REQ_SEND_DEBUG_EVENT );
    }

    TRACE("%s\n", debugstr_w(str));
}


/***********************************************************************
 *           OutputDebugString16   (KERNEL.115)
 */
void WINAPI OutputDebugString16( LPCSTR str )
{
    OutputDebugStringA( str );
}
