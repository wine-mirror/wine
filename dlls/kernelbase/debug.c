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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/exception.h"
#include "wine/server.h"
#include "wine/asm.h"
#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(winedbg);

typedef INT (WINAPI *MessageBoxA_funcptr)(HWND,LPCSTR,LPCSTR,UINT);
typedef INT (WINAPI *MessageBoxW_funcptr)(HWND,LPCWSTR,LPCWSTR,UINT);

static PTOP_LEVEL_EXCEPTION_FILTER top_filter;

void *dummy = RtlUnwind;  /* force importing RtlUnwind from ntdll */

/***********************************************************************
 *           CheckRemoteDebuggerPresent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CheckRemoteDebuggerPresent( HANDLE process, BOOL *present )
{
    DWORD_PTR port;

    if (!process || !present)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessDebugPort, &port, sizeof(port), NULL )))
        return FALSE;
    *present = !!port;
    return TRUE;
}


/**********************************************************************
 *           ContinueDebugEvent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ContinueDebugEvent( DWORD pid, DWORD tid, DWORD status )
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
 *           DebugActiveProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DebugActiveProcess( DWORD pid )
{
    HANDLE process;
    BOOL ret;

    SERVER_START_REQ( debug_process )
    {
        req->pid = pid;
        req->attach = 1;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    if (!(process = OpenProcess( PROCESS_CREATE_THREAD, FALSE, pid ))) return FALSE;
    ret = set_ntstatus( DbgUiIssueRemoteBreakin( process ));
    NtClose( process );
    if (!ret) DebugActiveProcessStop( pid );
    return ret;
}


/**********************************************************************
 *           DebugActiveProcessStop   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DebugActiveProcessStop( DWORD pid )
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
 *           DebugBreak   (kernelbase.@)
 */
#if defined(__i386__) || defined(__x86_64__)
__ASM_STDCALL_FUNC( DebugBreak, 0, "jmp " __ASM_STDCALL("DbgBreakPoint", 0) )
#else
void WINAPI DebugBreak(void)
{
    DbgBreakPoint();
}
#endif


/**************************************************************************
 *           FatalAppExitA   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH FatalAppExitA( UINT action, LPCSTR str )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    MessageBoxA_funcptr pMessageBoxA = NULL;

    if (mod) pMessageBoxA = (MessageBoxA_funcptr)GetProcAddress( mod, "MessageBoxA" );
    if (pMessageBoxA) pMessageBoxA( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    else ERR( "%s\n", debugstr_a(str) );
    RtlExitUserProcess( 1 );
}


/**************************************************************************
 *           FatalAppExitW   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH FatalAppExitW( UINT action, LPCWSTR str )
{
    static const WCHAR User32DllW[] = {'u','s','e','r','3','2','.','d','l','l',0};

    HMODULE mod = GetModuleHandleW( User32DllW );
    MessageBoxW_funcptr pMessageBoxW = NULL;

    if (mod) pMessageBoxW = (MessageBoxW_funcptr)GetProcAddress( mod, "MessageBoxW" );
    if (pMessageBoxW) pMessageBoxW( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    else ERR( "%s\n", debugstr_w(str) );
    RtlExitUserProcess( 1 );
}


/***********************************************************************
 *           IsDebuggerPresent   (kernelbase.@)
 */
BOOL WINAPI IsDebuggerPresent(void)
{
    return NtCurrentTeb()->Peb->BeingDebugged;
}


static LONG WINAPI debug_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == DBG_PRINTEXCEPTION_C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           OutputDebugStringA   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH OutputDebugStringA( LPCSTR str )
{
    static HANDLE DBWinMutex = NULL;
    static BOOL mutex_inited = FALSE;
    BOOL caught_by_dbg = TRUE;

    if (!str) str = "";
    WARN( "%s\n", debugstr_a(str) );

    /* raise exception, WaitForDebugEvent() will generate a corresponding debug event */
    __TRY
    {
        ULONG_PTR args[2];
        args[0] = strlen(str) + 1;
        args[1] = (ULONG_PTR)str;
        RaiseException( DBG_PRINTEXCEPTION_C, 0, 2, args );
    }
    __EXCEPT(debug_exception_handler)
    {
        caught_by_dbg = FALSE;
    }
    __ENDTRY
    if (caught_by_dbg) return;

    /* send string to a system-wide monitor */
    if (!mutex_inited)
    {
        /* first call to OutputDebugString, initialize mutex handle */
        static const WCHAR mutexname[] = {'D','B','W','i','n','M','u','t','e','x',0};
        HANDLE mutex = CreateMutexExW( NULL, mutexname, 0, SYNCHRONIZE );
        if (mutex)
        {
            if (InterlockedCompareExchangePointer( &DBWinMutex, mutex, 0 ) != 0)
                /* someone beat us here... */
                CloseHandle( mutex );
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
                    int str_len = strlen( str );
                    struct _mon_buffer_t
                    {
                        DWORD pid;
                        char buffer[1];
                    } *mon_buffer = (struct _mon_buffer_t*) buffer;

                    if (str_len > (4096 - sizeof(DWORD) - 1)) str_len = 4096 - sizeof(DWORD) - 1;
                    mon_buffer->pid = GetCurrentProcessId();
                    memcpy( mon_buffer->buffer, str, str_len );
                    mon_buffer->buffer[str_len] = 0;

                    /* signal data ready */
                    SetEvent( eventdata );
                }
                ReleaseMutex( DBWinMutex );
            }

            if (buffer) UnmapViewOfFile( buffer );
            if (eventbuffer) CloseHandle( eventbuffer );
            if (eventdata) CloseHandle( eventdata );
            CloseHandle( mapping );
        }
    }
}


/***********************************************************************
 *           OutputDebugStringW   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH OutputDebugStringW( LPCWSTR str )
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


/*******************************************************************
 *           RaiseException  (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH RaiseException( DWORD code, DWORD flags, DWORD count, const ULONG_PTR *args )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = code;
    record.ExceptionFlags   = flags & EH_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = RaiseException;
    if (count && args)
    {
        if (count > EXCEPTION_MAXIMUM_PARAMETERS) count = EXCEPTION_MAXIMUM_PARAMETERS;
        record.NumberParameters = count;
        memcpy( record.ExceptionInformation, args, count * sizeof(*args) );
    }
    else record.NumberParameters = 0;

    RtlRaiseException( &record );
}


/***********************************************************************
 *           SetUnhandledExceptionFilter   (kernelbase.@)
 */
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI DECLSPEC_HOTPATCH SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
    return InterlockedExchangePointer( (void **)&top_filter, filter );
}


/******************************************************************************
 *           WaitForDebugEvent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WaitForDebugEvent( DEBUG_EVENT *event, DWORD timeout )
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

            if (!wine_server_reply_size( reply ))  /* timeout */
            {
                wait = wine_server_ptr_handle( reply->wait );
                ret = FALSE;
                goto done;
            }
            event->dwDebugEventCode = data.code;
            event->dwProcessId      = (DWORD)reply->pid;
            event->dwThreadId       = (DWORD)reply->tid;
            switch (data.code)
            {
            case EXCEPTION_DEBUG_EVENT:
                if (data.exception.exc_code == DBG_PRINTEXCEPTION_C && data.exception.nb_params >= 2)
                {
                    event->dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;
                    event->u.DebugString.lpDebugStringData  = wine_server_get_ptr( data.exception.params[1] );
                    event->u.DebugString.fUnicode           = FALSE;
                    event->u.DebugString.nDebugStringLength = data.exception.params[0];
                    break;
                }
                else if (data.exception.exc_code == DBG_RIPEXCEPTION && data.exception.nb_params >= 2)
                {
                    event->dwDebugEventCode = RIP_EVENT;
                    event->u.RipInfo.dwError = data.exception.params[0];
                    event->u.RipInfo.dwType  = data.exception.params[1];
                    break;
                }
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


/*******************************************************************
 *         format_exception_msg
 */
static void format_exception_msg( const EXCEPTION_POINTERS *ptr, char *buffer, int size )
{
    const EXCEPTION_RECORD *rec = ptr->ExceptionRecord;
    int len;

    switch(rec->ExceptionCode)
    {
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        len = snprintf( buffer, size, "Unhandled division by zero" );
        break;
    case EXCEPTION_INT_OVERFLOW:
        len = snprintf( buffer, size, "Unhandled overflow" );
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        len = snprintf( buffer, size, "Unhandled array bounds" );
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        len = snprintf( buffer, size, "Unhandled illegal instruction" );
        break;
    case EXCEPTION_STACK_OVERFLOW:
        len = snprintf( buffer, size, "Unhandled stack overflow" );
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        len = snprintf( buffer, size, "Unhandled privileged instruction" );
        break;
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
            len = snprintf( buffer, size, "Unhandled page fault on %s access to %p",
                            rec->ExceptionInformation[0] == EXCEPTION_WRITE_FAULT ? "write" :
                            rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT ? "execute" : "read",
                            (void *)rec->ExceptionInformation[1]);
        else
            len = snprintf( buffer, size, "Unhandled page fault");
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        len = snprintf( buffer, size, "Unhandled alignment" );
        break;
    case CONTROL_C_EXIT:
        len = snprintf( buffer, size, "Unhandled ^C");
        break;
    case STATUS_POSSIBLE_DEADLOCK:
        len = snprintf( buffer, size, "Critical section %p wait failed",
                        (void *)rec->ExceptionInformation[0]);
        break;
    case EXCEPTION_WINE_STUB:
        if ((ULONG_PTR)rec->ExceptionInformation[1] >> 16)
            len = snprintf( buffer, size, "Unimplemented function %s.%s called",
                            (char *)rec->ExceptionInformation[0], (char *)rec->ExceptionInformation[1] );
        else
            len = snprintf( buffer, size, "Unimplemented function %s.%ld called",
                            (char *)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        break;
    case EXCEPTION_WINE_ASSERTION:
        len = snprintf( buffer, size, "Assertion failed" );
        break;
    default:
        len = snprintf( buffer, size, "Unhandled exception 0x%08x in thread %x",
                        rec->ExceptionCode, GetCurrentThreadId());
        break;
    }
    if (len < 0 || len >= size) return;
    snprintf( buffer + len,  size - len, " at address %p", ptr->ExceptionRecord->ExceptionAddress );
}


/******************************************************************
 *		start_debugger
 *
 * Does the effective debugger startup according to 'format'
 */
static BOOL start_debugger( EXCEPTION_POINTERS *epointers, HANDLE event )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR *cmdline, *env, *p, *format = NULL;
    HANDLE dbg_key;
    DWORD autostart = TRUE;
    PROCESS_INFORMATION	info;
    STARTUPINFOW startup;
    BOOL ret = FALSE;
    char buffer[256];

    static const WCHAR AeDebugW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                     'M','a','c','h','i','n','e','\\',
                                     'S','o','f','t','w','a','r','e','\\',
                                     'M','i','c','r','o','s','o','f','t','\\',
                                     'W','i','n','d','o','w','s',' ','N','T','\\',
                                     'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                     'A','e','D','e','b','u','g',0};
    static const WCHAR DebuggerW[] = {'D','e','b','u','g','g','e','r',0};
    static const WCHAR AutoW[] = {'A','u','t','o',0};

    format_exception_msg( epointers, buffer, sizeof(buffer) );
    MESSAGE( "wine: %s (thread %04x), starting debugger...\n", buffer, GetCurrentThreadId() );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, AeDebugW );

    if (!NtOpenKey( &dbg_key, KEY_READ, &attr ))
    {
        KEY_VALUE_PARTIAL_INFORMATION *info;
        DWORD format_size = 0;

        RtlInitUnicodeString( &nameW, DebuggerW );
        if (NtQueryValueKey( dbg_key, &nameW, KeyValuePartialInformation,
                             NULL, 0, &format_size ) == STATUS_BUFFER_TOO_SMALL)
        {
            char *data = HeapAlloc( GetProcessHeap(), 0, format_size );
            NtQueryValueKey( dbg_key, &nameW, KeyValuePartialInformation,
                             data, format_size, &format_size );
            info = (KEY_VALUE_PARTIAL_INFORMATION *)data;
            format = HeapAlloc( GetProcessHeap(), 0, info->DataLength + sizeof(WCHAR) );
            memcpy( format, info->Data, info->DataLength );
            format[info->DataLength / sizeof(WCHAR)] = 0;

            if (info->Type == REG_EXPAND_SZ)
            {
                WCHAR *tmp;

                format_size = ExpandEnvironmentStringsW( format, NULL, 0 );
                tmp = HeapAlloc( GetProcessHeap(), 0, format_size * sizeof(WCHAR));
                ExpandEnvironmentStringsW( format, tmp, format_size );
                HeapFree( GetProcessHeap(), 0, format );
                format = tmp;
            }
            HeapFree( GetProcessHeap(), 0, data );
        }

        RtlInitUnicodeString( &nameW, AutoW );
        if (!NtQueryValueKey( dbg_key, &nameW, KeyValuePartialInformation,
                              buffer, sizeof(buffer)-sizeof(WCHAR), &format_size ))
       {
           info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
           if (info->Type == REG_DWORD) memcpy( &autostart, info->Data, sizeof(DWORD) );
           else if (info->Type == REG_SZ)
           {
               WCHAR *str = (WCHAR *)info->Data;
               str[info->DataLength/sizeof(WCHAR)] = 0;
               autostart = wcstol( str, NULL, 10 );
           }
       }

       NtClose( dbg_key );
    }

    if (format)
    {
        size_t format_size = lstrlenW( format ) + 2*20;
        cmdline = HeapAlloc( GetProcessHeap(), 0, format_size * sizeof(WCHAR) );
        swprintf( cmdline, format_size, format, (long)GetCurrentProcessId(), (long)HandleToLong(event) );
        HeapFree( GetProcessHeap(), 0, format );
    }
    else
    {
        static const WCHAR fmtW[] = {'w','i','n','e','d','b','g',' ','-','-','a','u','t','o',' ','%','l','d',' ','%','l','d',0};
        cmdline = HeapAlloc( GetProcessHeap(), 0, 80 * sizeof(WCHAR) );
        swprintf( cmdline, 80, fmtW, (long)GetCurrentProcessId(), (long)HandleToLong(event) );
    }

    if (!autostart)
    {
	HMODULE mod = GetModuleHandleA( "user32.dll" );
	MessageBoxA_funcptr pMessageBoxA = NULL;

	if (mod) pMessageBoxA = (void *)GetProcAddress( mod, "MessageBoxA" );
	if (pMessageBoxA)
	{
            static const char msg[] = ".\nDo you wish to debug it?";

            format_exception_msg( epointers, buffer, sizeof(buffer) - sizeof(msg) );
            strcat( buffer, msg );
	    if (pMessageBoxA( 0, buffer, "Exception raised", MB_YESNO | MB_ICONHAND ) == IDNO)
	    {
		TRACE( "Killing process\n" );
		goto exit;
	    }
	}
    }

    /* make WINEDEBUG empty in the environment */
    env = GetEnvironmentStringsW();
    if (!TRACE_ON(winedbg))
    {
        static const WCHAR winedebugW[] = {'W','I','N','E','D','E','B','U','G','=',0};
        for (p = env; *p; p += lstrlenW(p) + 1)
        {
            if (!wcsncmp( p, winedebugW, lstrlenW(winedebugW) ))
            {
                WCHAR *next = p + lstrlenW(p);
                WCHAR *end = next + 1;
                while (*end) end += lstrlenW(end) + 1;
                memmove( p + lstrlenW(winedebugW), next, end + 1 - next );
                break;
            }
        }
    }

    TRACE( "Starting debugger %s\n", debugstr_w(cmdline) );
    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    ret = CreateProcessW( NULL, cmdline, NULL, NULL, TRUE, 0, env, NULL, &startup, &info );
    FreeEnvironmentStringsW( env );

    if (ret)
    {
        /* wait for debugger to come up... */
        HANDLE handles[2];
        CloseHandle( info.hThread );
        handles[0] = event;
        handles[1] = info.hProcess;
        WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
        CloseHandle( info.hProcess );
    }
    else ERR( "Couldn't start debugger %s (%d)\n"
              "Read the Wine Developers Guide on how to set up winedbg or another debugger\n",
              debugstr_w(cmdline), GetLastError() );
exit:
    HeapFree(GetProcessHeap(), 0, cmdline);
    return ret;
}

/******************************************************************
 *		start_debugger_atomic
 *
 * starts the debugger in an atomic way:
 *	- either the debugger is not started and it is started
 *	- or the debugger has already been started by another thread
 *	- or the debugger couldn't be started
 *
 * returns TRUE for the two first conditions, FALSE for the last
 */
static BOOL start_debugger_atomic( EXCEPTION_POINTERS *epointers )
{
    static HANDLE once;

    if (once == 0)
    {
	OBJECT_ATTRIBUTES attr;
	HANDLE event;

	attr.Length                   = sizeof(attr);
	attr.RootDirectory            = 0;
	attr.Attributes               = OBJ_INHERIT;
	attr.ObjectName               = NULL;
	attr.SecurityDescriptor       = NULL;
	attr.SecurityQualityOfService = NULL;

	/* ask for manual reset, so that once the debugger is started,
	 * every thread will know it */
	NtCreateEvent( &event, EVENT_ALL_ACCESS, &attr, NotificationEvent, FALSE );
        if (InterlockedCompareExchangePointer( &once, event, 0 ) == 0)
	{
	    /* ok, our event has been set... we're the winning thread */
	    BOOL ret = start_debugger( epointers, once );

	    if (!ret)
	    {
		/* so that the other threads won't be stuck */
		NtSetEvent( once, NULL );
	    }
	    return ret;
	}

	/* someone beat us here... */
	CloseHandle( event );
    }

    /* and wait for the winner to have actually created the debugger */
    WaitForSingleObject( once, INFINITE );
    /* in fact, here, we only know that someone has tried to start the debugger,
     * we'll know by reposting the exception if it has actually attached
     * to the current process */
    return TRUE;
}


/*******************************************************************
 *         check_resource_write
 *
 * Check if the exception is a write attempt to the resource data.
 * If yes, we unprotect the resources to let broken apps continue
 * (Windows does this too).
 */
static BOOL check_resource_write( void *addr )
{
    DWORD old_prot;
    void *rsrc;
    DWORD size;
    MEMORY_BASIC_INFORMATION info;

    if (!VirtualQuery( addr, &info, sizeof(info) )) return FALSE;
    if (info.State == MEM_FREE || !(info.Type & MEM_IMAGE)) return FALSE;
    if (!(rsrc = RtlImageDirectoryEntryToData( info.AllocationBase, TRUE,
                                               IMAGE_DIRECTORY_ENTRY_RESOURCE, &size )))
        return FALSE;
    if (addr < rsrc || (char *)addr >= (char *)rsrc + size) return FALSE;
    TRACE( "Broken app is writing to the resource data, enabling work-around\n" );
    VirtualProtect( rsrc, size, PAGE_READWRITE, &old_prot );
    return TRUE;
}


/*******************************************************************
 *         UnhandledExceptionFilter   (kernelbase.@)
 */
LONG WINAPI UnhandledExceptionFilter( EXCEPTION_POINTERS *epointers )
{
    const EXCEPTION_RECORD *rec = epointers->ExceptionRecord;

    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2)
    {
        switch (rec->ExceptionInformation[0])
        {
        case EXCEPTION_WRITE_FAULT:
            if (check_resource_write( (void *)rec->ExceptionInformation[1] ))
                return EXCEPTION_CONTINUE_EXECUTION;
            break;
        }
    }

    if (!NtCurrentTeb()->Peb->BeingDebugged)
    {
        if (rec->ExceptionCode == CONTROL_C_EXIT)
        {
            /* do not launch the debugger on ^C, simply terminate the process */
            TerminateProcess( GetCurrentProcess(), 1 );
        }

        if (top_filter)
        {
            LONG ret = top_filter( epointers );
            if (ret != EXCEPTION_CONTINUE_SEARCH) return ret;
        }

        /* FIXME: Should check the current error mode */

        if (!start_debugger_atomic( epointers ) || !NtCurrentTeb()->Peb->BeingDebugged)
            return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
