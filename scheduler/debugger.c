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

DECLARE_DEBUG_CHANNEL(debugstr)

/**********************************************************************
 *           DEBUG_SendEvent
 *
 * Internal helper to send a debug event request to the server.
 */
static DWORD DEBUG_SendEvent( int code, void *data, int size )
{
    DWORD ret = 0;
    struct send_debug_event_request *req = get_req_buffer();
    req->code = code;
    memcpy( req + 1, data, size );
    if (!server_call( REQ_SEND_DEBUG_EVENT )) ret = req->status;
    return ret;
}


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
    struct debug_event_exception *event = (struct debug_event_exception *)(req + 1);

    req->code        = EXCEPTION_DEBUG_EVENT;
    event->code      = rec->ExceptionCode;
    event->flags     = rec->ExceptionFlags;
    event->record    = rec->ExceptionRecord;
    event->addr      = rec->ExceptionAddress;
    event->nb_params = rec->NumberParameters;
    for (i = 0; i < event->nb_params; i++) event->params[i] = rec->ExceptionInformation[i];
    event->first_chance = first_chance;
    event->context      = *context;
    if (!server_call( REQ_SEND_DEBUG_EVENT ))
    {
        ret = req->status;
        *context = event->context;
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
    struct debug_event_create_process event;

    event.file       = file;
    event.process    = 0; /* will be filled by server */
    event.thread     = 0; /* will be filled by server */
    event.base       = (void *)module;
    event.dbg_offset = 0; /* FIXME */
    event.dbg_size   = 0; /* FIXME */
    event.teb        = NtCurrentTeb();
    event.start      = entry;
    event.name       = 0; /* FIXME */
    event.unicode    = 0; /* FIXME */
    return DEBUG_SendEvent( CREATE_PROCESS_DEBUG_EVENT, &event, sizeof(event) );
}


/**********************************************************************
 *           DEBUG_SendCreateThreadEvent
 *
 * Send an CREATE_THREAD_DEBUG_EVENT event to the current process debugger.
 * Must be called from the context of the new thread.
 */
DWORD DEBUG_SendCreateThreadEvent( void *entry )
{
    struct debug_event_create_thread event;

    event.handle = 0; /* will be filled by server */
    event.teb    = NtCurrentTeb();
    event.start  = entry;
    return DEBUG_SendEvent( CREATE_THREAD_DEBUG_EVENT, &event, sizeof(event) );
}


/**********************************************************************
 *           DEBUG_SendLoadDLLEvent
 *
 * Send an LOAD_DLL_DEBUG_EVENT event to the current process debugger.
 */
DWORD DEBUG_SendLoadDLLEvent( HFILE file, HMODULE module, LPSTR *name )
{
    struct debug_event_load_dll event;

    event.handle     = file;
    event.base       = (void *)module;
    event.dbg_offset = 0;  /* FIXME */
    event.dbg_size   = 0;  /* FIXME */
    event.name       = name;
    event.unicode    = 0;
    return DEBUG_SendEvent( LOAD_DLL_DEBUG_EVENT, &event, sizeof(event) );
}


/**********************************************************************
 *           DEBUG_SendUnloadDLLEvent
 *
 * Send an UNLOAD_DLL_DEBUG_EVENT event to the current process debugger.
 */
DWORD DEBUG_SendUnloadDLLEvent( HMODULE module )
{
    struct debug_event_unload_dll event;

    event.base = (void *)module;
    return DEBUG_SendEvent( UNLOAD_DLL_DEBUG_EVENT, &event, sizeof(event) );
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
    union debug_event_data *data = (union debug_event_data *)(req + 1);
    int i;

    req->timeout = timeout;
    if (server_call( REQ_WAIT_DEBUG_EVENT )) return FALSE;
    if ((req->code < 0) || (req->code > RIP_EVENT))
        server_protocol_error( "WaitForDebugEvent: bad code %d\n", req->code );

    event->dwDebugEventCode = req->code;
    event->dwProcessId      = (DWORD)req->pid;
    event->dwThreadId       = (DWORD)req->tid;
    switch(req->code)
    {
    case EXCEPTION_DEBUG_EVENT:
        event->u.Exception.ExceptionRecord.ExceptionCode    = data->exception.code;
        event->u.Exception.ExceptionRecord.ExceptionFlags   = data->exception.flags;
        event->u.Exception.ExceptionRecord.ExceptionRecord  = data->exception.record;
        event->u.Exception.ExceptionRecord.ExceptionAddress = data->exception.addr;
        event->u.Exception.ExceptionRecord.NumberParameters = data->exception.nb_params;
        for (i = 0; i < data->exception.nb_params; i++)
            event->u.Exception.ExceptionRecord.ExceptionInformation[i] = data->exception.params[i];
        event->u.Exception.dwFirstChance = data->exception.first_chance;
        break;
    case CREATE_THREAD_DEBUG_EVENT:
        event->u.CreateThread.hThread           = data->create_thread.handle;
        event->u.CreateThread.lpThreadLocalBase = data->create_thread.teb;
        event->u.CreateThread.lpStartAddress    = data->create_thread.start;
        break;
    case CREATE_PROCESS_DEBUG_EVENT:
        event->u.CreateProcessInfo.hFile                 = data->create_process.file;
        event->u.CreateProcessInfo.hProcess              = data->create_process.process;
        event->u.CreateProcessInfo.hThread               = data->create_process.thread;
        event->u.CreateProcessInfo.lpBaseOfImage         = data->create_process.base;
        event->u.CreateProcessInfo.dwDebugInfoFileOffset = data->create_process.dbg_offset;
        event->u.CreateProcessInfo.nDebugInfoSize        = data->create_process.dbg_size;
        event->u.CreateProcessInfo.lpThreadLocalBase     = data->create_process.teb;
        event->u.CreateProcessInfo.lpStartAddress        = data->create_process.start;
        event->u.CreateProcessInfo.lpImageName           = data->create_process.name;
        event->u.CreateProcessInfo.fUnicode              = data->create_process.unicode;
        if (data->create_process.file == -1) event->u.CreateProcessInfo.hFile = 0;
        break;
    case EXIT_THREAD_DEBUG_EVENT:
        event->u.ExitThread.dwExitCode = data->exit.exit_code;
        break;
    case EXIT_PROCESS_DEBUG_EVENT:
        event->u.ExitProcess.dwExitCode = data->exit.exit_code;
        break;
    case LOAD_DLL_DEBUG_EVENT:
        event->u.LoadDll.hFile                 = data->load_dll.handle;
        event->u.LoadDll.lpBaseOfDll           = data->load_dll.base;
        event->u.LoadDll.dwDebugInfoFileOffset = data->load_dll.dbg_offset;
        event->u.LoadDll.nDebugInfoSize        = data->load_dll.dbg_size;
        event->u.LoadDll.lpImageName           = data->load_dll.name;
        event->u.LoadDll.fUnicode              = data->load_dll.unicode;
        if (data->load_dll.handle == -1) event->u.LoadDll.hFile = 0;
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        event->u.UnloadDll.lpBaseOfDll = data->unload_dll.base;
        break;
    case OUTPUT_DEBUG_STRING_EVENT:
        event->u.DebugString.lpDebugStringData  = data->output_string.string;
        event->u.DebugString.fUnicode           = data->output_string.unicode;
        event->u.DebugString.nDebugStringLength = data->output_string.length;
        break;
    case RIP_EVENT:
        event->u.RipInfo.dwError = data->rip_info.error;
        event->u.RipInfo.dwType  = data->rip_info.type;
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
        struct debug_event_output_string event;
        event.string  = (void *)str;
        event.unicode = 0;
        event.length  = strlen(str) + 1;
        DEBUG_SendEvent( OUTPUT_DEBUG_STRING_EVENT, &event, sizeof(event) );
    }

    TRACE_(debugstr)("%s\n", str);
}


/***********************************************************************
 *           OutputDebugStringW   (KERNEL32.549)
 */
void WINAPI OutputDebugStringW( LPCWSTR str )
{
    if (PROCESS_Current()->flags & PDB32_DEBUGGED)
    {
        struct debug_event_output_string event;
        event.string  = (void *)str;
        event.unicode = 1;
        event.length  = (lstrlenW(str) + 1) * sizeof(WCHAR);
        DEBUG_SendEvent( OUTPUT_DEBUG_STRING_EVENT, &event, sizeof(event) );
    }

    TRACE_(debugstr)("%s\n", debugstr_w(str));
}


/***********************************************************************
 *           OutputDebugString16   (KERNEL.115)
 */
void WINAPI OutputDebugString16( LPCSTR str )
{
    OutputDebugStringA( str );
}
