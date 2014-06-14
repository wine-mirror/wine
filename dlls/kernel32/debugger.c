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

#include "config.h"
#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winerror.h"
#include "wine/server.h"
#include "kernel_private.h"
#include "wine/debug.h"
#include "wine/exception.h"

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
    int i;

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
                wait = wine_server_ptr_handle( reply->wait );
                ret = FALSE;
                goto done;
            }
            event->dwDebugEventCode = data.code;
            event->dwProcessId      = (DWORD)reply->pid;
            event->dwThreadId       = (DWORD)reply->tid;
            switch(data.code)
            {
            case EXCEPTION_DEBUG_EVENT:
                event->u.Exception.dwFirstChance = data.exception.first;
                event->u.Exception.ExceptionRecord.ExceptionCode    = data.exception.exc_code;
                event->u.Exception.ExceptionRecord.ExceptionFlags   = data.exception.flags;
                event->u.Exception.ExceptionRecord.ExceptionRecord  = wine_server_get_ptr( data.exception.record );
                event->u.Exception.ExceptionRecord.ExceptionAddress = wine_server_get_ptr( data.exception.address );
                event->u.Exception.ExceptionRecord.NumberParameters = data.exception.nb_params;
                for (i = 0; i < data.exception.nb_params; i++)
                    event->u.Exception.ExceptionRecord.ExceptionInformation[i] = data.exception.params[i];
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                event->u.CreateThread.hThread           = wine_server_ptr_handle( data.create_thread.handle );
                event->u.CreateThread.lpThreadLocalBase = wine_server_get_ptr( data.create_thread.teb );
                event->u.CreateThread.lpStartAddress    = wine_server_get_ptr( data.create_thread.start );
                break;
            case CREATE_PROCESS_DEBUG_EVENT:
                event->u.CreateProcessInfo.hFile                 = wine_server_ptr_handle( data.create_process.file );
                event->u.CreateProcessInfo.hProcess              = wine_server_ptr_handle( data.create_process.process );
                event->u.CreateProcessInfo.hThread               = wine_server_ptr_handle( data.create_process.thread );
                event->u.CreateProcessInfo.lpBaseOfImage         = wine_server_get_ptr( data.create_process.base );
                event->u.CreateProcessInfo.dwDebugInfoFileOffset = data.create_process.dbg_offset;
                event->u.CreateProcessInfo.nDebugInfoSize        = data.create_process.dbg_size;
                event->u.CreateProcessInfo.lpThreadLocalBase     = wine_server_get_ptr( data.create_process.teb );
                event->u.CreateProcessInfo.lpStartAddress        = wine_server_get_ptr( data.create_process.start );
                event->u.CreateProcessInfo.lpImageName           = wine_server_get_ptr( data.create_process.name );
                event->u.CreateProcessInfo.fUnicode              = data.create_process.unicode;
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                event->u.ExitThread.dwExitCode = data.exit.exit_code;
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                event->u.ExitProcess.dwExitCode = data.exit.exit_code;
                break;
            case LOAD_DLL_DEBUG_EVENT:
                event->u.LoadDll.hFile                 = wine_server_ptr_handle( data.load_dll.handle );
                event->u.LoadDll.lpBaseOfDll           = wine_server_get_ptr( data.load_dll.base );
                event->u.LoadDll.dwDebugInfoFileOffset = data.load_dll.dbg_offset;
                event->u.LoadDll.nDebugInfoSize        = data.load_dll.dbg_size;
                event->u.LoadDll.lpImageName           = wine_server_get_ptr( data.load_dll.name );
                event->u.LoadDll.fUnicode              = data.load_dll.unicode;
                break;
            case UNLOAD_DLL_DEBUG_EVENT:
                event->u.UnloadDll.lpBaseOfDll = wine_server_get_ptr( data.unload_dll.base );
                break;
            case OUTPUT_DEBUG_STRING_EVENT:
                event->u.DebugString.lpDebugStringData  = wine_server_get_ptr( data.output_string.string );
                event->u.DebugString.fUnicode           = FALSE;
                event->u.DebugString.nDebugStringLength = data.output_string.length;
                break;
            case RIP_EVENT:
                event->u.RipInfo.dwError = data.rip_info.error;
                event->u.RipInfo.dwType  = data.rip_info.type;
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

static LONG WINAPI debug_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == DBG_PRINTEXCEPTION_C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
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
    static HANDLE DBWinMutex = NULL;
    static BOOL mutex_inited = FALSE;

    if (!str) str = "";

    /* raise fake exception to make copy protections happy */
    __TRY
    {
        ULONG_PTR args[2];
        args[0] = strlen(str) + 1;
        args[1] = (ULONG_PTR)str;
        RaiseException( DBG_PRINTEXCEPTION_C, 0, 2, args );
    }
    __EXCEPT(debug_exception_handler)
    {
    }
    __ENDTRY

    /* send string to attached debugger */
    /* FIXME should only send to debugger if exception is not caught by user-mode application */

    SERVER_START_REQ( output_debug_string )
    {
        req->string  = wine_server_client_ptr( str );
        req->length  = strlen(str) + 1;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    WARN("%s\n", debugstr_a(str));

    /* send string to a system-wide monitor */
    /* FIXME should only send to monitor if no debuggers are attached */

    if (!mutex_inited)
    {
        /* first call to OutputDebugString, initialize mutex handle */
        static const WCHAR mutexname[] = {'D','B','W','i','n','M','u','t','e','x',0};
        HANDLE mutex = CreateMutexExW( NULL, mutexname, 0, SYNCHRONIZE );
        if (mutex)
        {
            if (InterlockedCompareExchangePointer( &DBWinMutex, mutex, 0 ) != 0)
            {
                /* someone beat us here... */
                CloseHandle( mutex );
            }
        }
        mutex_inited = TRUE;
    }

    if (DBWinMutex)
    {
        static const WCHAR shmname[] = {'D','B','W','I','N','_','B','U','F','F','E','R',0};
        static const WCHAR eventbuffername[] = {'D','B','W','I','N','_','B','U','F','F','E','R','_','R','E','A','D','Y',0};
        static const WCHAR eventdataname[] = {'D','B','W','I','N','_','D','A','T','A','_','R','E','A','D','Y',0};
        HANDLE mapping;

        mapping = OpenFileMappingW( FILE_MAP_WRITE, FALSE, shmname );
        if (mapping)
        {
            LPVOID buffer;
            HANDLE eventbuffer, eventdata;

            buffer = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
            eventbuffer = OpenEventW( SYNCHRONIZE, FALSE, eventbuffername );
            eventdata = OpenEventW( EVENT_MODIFY_STATE, FALSE, eventdataname );

            if (buffer && eventbuffer && eventdata)
            {
                /* monitor is present, synchronize with other OutputDebugString invocations */
                WaitForSingleObject( DBWinMutex, INFINITE );

                /* acquire control over the buffer */
                if (WaitForSingleObject( eventbuffer, 10000 ) == WAIT_OBJECT_0)
                {
                    int str_len;
                    struct _mon_buffer_t {
                        DWORD pid;
                        char buffer[1];
                    } *mon_buffer = (struct _mon_buffer_t*) buffer;

                    str_len = strlen( str );
                    if (str_len > (4096 - sizeof(DWORD) - 1))
                        str_len = 4096 - sizeof(DWORD) - 1;

                    mon_buffer->pid = GetCurrentProcessId();
                    memcpy( mon_buffer->buffer, str, str_len );
                    mon_buffer->buffer[str_len] = 0;

                    /* signal data ready */
                    SetEvent( eventdata );
                }
                ReleaseMutex( DBWinMutex );
            }

            if (buffer)
                UnmapViewOfFile( buffer );
            if (eventbuffer)
                CloseHandle( eventbuffer );
            if (eventdata)
                CloseHandle( eventdata );
            CloseHandle( mapping );
        }
    }
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
    UNICODE_STRING strW;
    STRING strA;

    RtlInitUnicodeString( &strW, str );
    if (!RtlUnicodeStringToAnsiString( &strA, &strW, TRUE ))
    {
        OutputDebugStringA( strA.Buffer );
        RtlFreeAnsiString( &strA );
    }
}


/***********************************************************************
 *           DebugBreak   (KERNEL32.@)
 *
 *  Raises an exception so that a debugger (if attached)
 *  can take some action.
 */
#if defined(__i386__) || defined(__x86_64__)
__ASM_STDCALL_FUNC( DebugBreak, 0, "jmp " __ASM_NAME("DbgBreakPoint") )
#else
void WINAPI DebugBreak(void)
{
    DbgBreakPoint();
}
#endif

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
        req->handle = wine_server_obj_handle( hProc );
        ret = !wine_server_call_err( req );
        self = ret && reply->self;
    }
    SERVER_END_REQ;
    if (self) DbgBreakPoint();
    return ret;
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
    NTSTATUS status;
    DWORD_PTR port;

    if(!process || !DebuggerPresent)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    status = NtQueryInformationProcess(process, ProcessDebugPort, &port, sizeof(port), NULL);
    if (status != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    *DebuggerPresent = !!port;
    return TRUE;
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
