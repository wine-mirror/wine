/*
 * Win32 exception functions
 *
 * Copyright (c) 1996 Onno Hovers, (onno@stack.urc.tue.nl)
 * Copyright (c) 1999 Alexandre Julliard
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
 *
 * Notes:
 *  What really happens behind the scenes of those new
 *  __try{...}__except(..){....}  and
 *  __try{...}__finally{...}
 *  statements is simply not documented by Microsoft. There could be different
 *  reasons for this:
 *  One reason could be that they try to hide the fact that exception
 *  handling in Win32 looks almost the same as in OS/2 2.x.
 *  Another reason could be that Microsoft does not want others to write
 *  binary compatible implementations of the Win32 API (like us).
 *
 *  Whatever the reason, THIS SUCKS!! Ensuring portability or future
 *  compatibility may be valid reasons to keep some things undocumented.
 *  But exception handling is so basic to Win32 that it should be
 *  documented!
 *
 */
#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "excpt.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

static PTOP_LEVEL_EXCEPTION_FILTER top_filter;

typedef INT (WINAPI *MessageBoxA_funcptr)(HWND,LPCSTR,LPCSTR,UINT);
typedef INT (WINAPI *MessageBoxW_funcptr)(HWND,LPCWSTR,LPCWSTR,UINT);

/*******************************************************************
 *         RaiseException  (KERNEL32.@)
 */
void WINAPI RaiseException( DWORD code, DWORD flags, DWORD nbargs, const ULONG_PTR *args )
{
    EXCEPTION_RECORD record;

    /* Compose an exception record */

    record.ExceptionCode    = code;
    record.ExceptionFlags   = flags & EH_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = RaiseException;
    if (nbargs && args)
    {
        if (nbargs > EXCEPTION_MAXIMUM_PARAMETERS) nbargs = EXCEPTION_MAXIMUM_PARAMETERS;
        record.NumberParameters = nbargs;
        memcpy( record.ExceptionInformation, args, nbargs * sizeof(*args) );
    }
    else record.NumberParameters = 0;

    RtlRaiseException( &record );
}


/*******************************************************************
 *         format_exception_msg
 */
static int format_exception_msg( const EXCEPTION_POINTERS *ptr, char *buffer, int size )
{
    const EXCEPTION_RECORD *rec = ptr->ExceptionRecord;
    int len,len2;

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
            len = snprintf( buffer, size, "Unhandled page fault on %s access to 0x%08lx",
                            rec->ExceptionInformation[0] == EXCEPTION_WRITE_FAULT ? "write" :
                            rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT ? "execute" : "read",
                            rec->ExceptionInformation[1]);
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
        len = snprintf( buffer, size, "Critical section %08lx wait failed",
                 rec->ExceptionInformation[0]);
        break;
    case EXCEPTION_WINE_STUB:
        if (HIWORD(rec->ExceptionInformation[1]))
            len = snprintf( buffer, size, "Unimplemented function %s.%s called",
                            (char *)rec->ExceptionInformation[0], (char *)rec->ExceptionInformation[1] );
        else
            len = snprintf( buffer, size, "Unimplemented function %s.%ld called",
                            (char *)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        break;
    case EXCEPTION_WINE_ASSERTION:
        len = snprintf( buffer, size, "Assertion failed" );
        break;
    case EXCEPTION_VM86_INTx:
        len = snprintf( buffer, size, "Unhandled interrupt %02lx in vm86 mode",
                 rec->ExceptionInformation[0]);
        break;
    case EXCEPTION_VM86_STI:
        len = snprintf( buffer, size, "Unhandled sti in vm86 mode");
        break;
    case EXCEPTION_VM86_PICRETURN:
        len = snprintf( buffer, size, "Unhandled PIC return in vm86 mode");
        break;
    default:
        len = snprintf( buffer, size, "Unhandled exception 0x%08x", rec->ExceptionCode);
        break;
    }
    if ((len<0) || (len>=size))
        return -1;
#ifdef __i386__
    if (ptr->ContextRecord->SegCs != wine_get_cs())
        len2 = snprintf(buffer+len, size-len, " at address 0x%04x:0x%08x",
                        ptr->ContextRecord->SegCs,
                        (DWORD)ptr->ExceptionRecord->ExceptionAddress);
    else
#endif
        len2 = snprintf(buffer+len, size-len, " at address %p",
                        ptr->ExceptionRecord->ExceptionAddress);
    if ((len2<0) || (len>=size-len))
        return -1;
    return len+len2;
}


/******************************************************************
 *		start_debugger
 *
 * Does the effective debugger startup according to 'format'
 */
static BOOL	start_debugger(PEXCEPTION_POINTERS epointers, HANDLE hEvent)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    char *cmdline, *env, *p;
    HANDLE		hDbgConf;
    DWORD		bAuto = TRUE;
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    char*		format = NULL;
    BOOL		ret = FALSE;
    char buffer[256];

    static const WCHAR AeDebugW[] = {'M','a','c','h','i','n','e','\\',
                                     'S','o','f','t','w','a','r','e','\\',
                                     'M','i','c','r','o','s','o','f','t','\\',
                                     'W','i','n','d','o','w','s',' ','N','T','\\',
                                     'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                     'A','e','D','e','b','u','g',0};
    static const WCHAR DebuggerW[] = {'D','e','b','u','g','g','e','r',0};
    static const WCHAR AutoW[] = {'A','u','t','o',0};

    format_exception_msg( epointers, buffer, sizeof(buffer) );
    MESSAGE("wine: %s (thread %04x), starting debugger...\n", buffer, GetCurrentThreadId());

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, AeDebugW );

    if (!NtOpenKey( &hDbgConf, KEY_ALL_ACCESS, &attr ))
    {
        char buffer[64];
        KEY_VALUE_PARTIAL_INFORMATION *info;
        DWORD format_size = 0;

        RtlInitUnicodeString( &nameW, DebuggerW );
        if (NtQueryValueKey( hDbgConf, &nameW, KeyValuePartialInformation,
                             NULL, 0, &format_size ) == STATUS_BUFFER_OVERFLOW)
        {
            char *data = HeapAlloc(GetProcessHeap(), 0, format_size);
            NtQueryValueKey( hDbgConf, &nameW, KeyValuePartialInformation,
                             data, format_size, &format_size );
            info = (KEY_VALUE_PARTIAL_INFORMATION *)data;
            RtlUnicodeToMultiByteSize( &format_size, (WCHAR *)info->Data, info->DataLength );
            format = HeapAlloc( GetProcessHeap(), 0, format_size+1 );
            RtlUnicodeToMultiByteN( format, format_size, NULL,
                                    (WCHAR *)info->Data, info->DataLength );
            format[format_size] = 0;

            if (info->Type == REG_EXPAND_SZ)
            {
                char* tmp;

                /* Expand environment variable references */
                format_size=ExpandEnvironmentStringsA(format,NULL,0);
                tmp=HeapAlloc(GetProcessHeap(), 0, format_size);
                ExpandEnvironmentStringsA(format,tmp,format_size);
                HeapFree(GetProcessHeap(), 0, format);
                format=tmp;
            }
            HeapFree( GetProcessHeap(), 0, data );
        }

        RtlInitUnicodeString( &nameW, AutoW );
        if (!NtQueryValueKey( hDbgConf, &nameW, KeyValuePartialInformation,
                              buffer, sizeof(buffer)-sizeof(WCHAR), &format_size ))
       {
           info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
           if (info->Type == REG_DWORD) memcpy( &bAuto, info->Data, sizeof(DWORD) );
           else if (info->Type == REG_SZ)
           {
               WCHAR *str = (WCHAR *)info->Data;
               str[info->DataLength/sizeof(WCHAR)] = 0;
               bAuto = atoiW( str );
           }
       }

       NtClose(hDbgConf);
    }

    if (format)
    {
        cmdline = HeapAlloc(GetProcessHeap(), 0, strlen(format) + 2*20);
        sprintf(cmdline, format, GetCurrentProcessId(), hEvent);
        HeapFree(GetProcessHeap(), 0, format);
    }
    else
    {
        cmdline = HeapAlloc(GetProcessHeap(), 0, 80);
        sprintf(cmdline, "winedbg --auto %d %ld",
                GetCurrentProcessId(), (ULONG_PTR)hEvent);
    }

    if (!bAuto)
    {
	HMODULE			mod = GetModuleHandleA( "user32.dll" );
	MessageBoxA_funcptr	pMessageBoxA = NULL;

	if (mod) pMessageBoxA = (MessageBoxA_funcptr)GetProcAddress( mod, "MessageBoxA" );
	if (pMessageBoxA)
	{
            static const char msg[] = ".\nDo you wish to debug it?";
	    char buffer[256];

            format_exception_msg( epointers, buffer, sizeof(buffer)-sizeof(msg) );
            strcat( buffer, msg );
	    if (pMessageBoxA( 0, buffer, "Exception raised", MB_YESNO | MB_ICONHAND ) == IDNO)
	    {
		TRACE("Killing process\n");
		goto EXIT;
	    }
	}
    }

    /* make WINEDEBUG empty in the environment */
    env = GetEnvironmentStringsA();
    for (p = env; *p; p += strlen(p) + 1)
    {
        if (!memcmp( p, "WINEDEBUG=", sizeof("WINEDEBUG=")-1 ))
        {
            char *next = p + strlen(p);
            char *end = next + 1;
            while (*end) end += strlen(end) + 1;
            memmove( p + sizeof("WINEDEBUG=") - 1, next, end + 1 - next );
            break;
        }
    }

    TRACE("Starting debugger %s\n", debugstr_a(cmdline));
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, env, NULL, &startup, &info);
    FreeEnvironmentStringsA( env );

    if (ret) WaitForSingleObject(hEvent, INFINITE);  /* wait for debugger to come up... */
    else ERR("Couldn't start debugger (%s) (%d)\n"
             "Read the Wine Developers Guide on how to set up winedbg or another debugger\n",
             debugstr_a(cmdline), GetLastError());
EXIT:
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
static	int	start_debugger_atomic(PEXCEPTION_POINTERS epointers)
{
    static HANDLE	hRunOnce /* = 0 */;

    if (hRunOnce == 0)
    {
	OBJECT_ATTRIBUTES	attr;
	HANDLE			hEvent;

	attr.Length                   = sizeof(attr);
	attr.RootDirectory            = 0;
	attr.Attributes               = OBJ_INHERIT;
	attr.ObjectName               = NULL;
	attr.SecurityDescriptor       = NULL;
	attr.SecurityQualityOfService = NULL;

	/* ask for manual reset, so that once the debugger is started,
	 * every thread will know it */
	NtCreateEvent( &hEvent, EVENT_ALL_ACCESS, &attr, TRUE, FALSE );
	if (InterlockedCompareExchangePointer( (PVOID)&hRunOnce, hEvent, 0 ) == 0)
	{
	    /* ok, our event has been set... we're the winning thread */
	    BOOL	ret = start_debugger( epointers, hRunOnce );
	    DWORD	tmp;

	    if (!ret)
	    {
		/* so that the other threads won't be stuck */
		NtSetEvent( hRunOnce, &tmp );
	    }
	    return ret;
	}

	/* someone beat us here... */
	CloseHandle( hEvent );
    }

    /* and wait for the winner to have actually created the debugger */
    WaitForSingleObject( hRunOnce, INFINITE );
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
static inline BOOL check_resource_write( void *addr )
{
    void *rsrc;
    DWORD size;
    MEMORY_BASIC_INFORMATION info;

    if (!VirtualQuery( addr, &info, sizeof(info) )) return FALSE;
    if (info.State == MEM_FREE || !(info.Type & MEM_IMAGE)) return FALSE;
    if (!(rsrc = RtlImageDirectoryEntryToData( (HMODULE)info.AllocationBase, TRUE,
                                              IMAGE_DIRECTORY_ENTRY_RESOURCE, &size )))
        return FALSE;
    if (addr < rsrc || (char *)addr >= (char *)rsrc + size) return FALSE;
    TRACE( "Broken app is writing to the resource data, enabling work-around\n" );
    VirtualProtect( rsrc, size, PAGE_WRITECOPY, NULL );
    return TRUE;
}


/*******************************************************************
 *         check_no_exec
 *
 * Check for executing a protected area.
 */
static inline BOOL check_no_exec( void *addr )
{
    MEMORY_BASIC_INFORMATION info;

    if (!VirtualQuery( addr, &info, sizeof(info) )) return FALSE;
    if (info.State == MEM_FREE) return FALSE;

    /* prot |= PAGE_EXECUTE would be a lot easier, but MS developers
     * apparently don't grasp the notion of protection bits */
    switch(info.Protect)
    {
    case PAGE_READONLY: info.Protect = PAGE_EXECUTE_READ; break;
    case PAGE_READWRITE: info.Protect = PAGE_EXECUTE_READWRITE; break;
    case PAGE_WRITECOPY: info.Protect = PAGE_EXECUTE_WRITECOPY; break;
    default: return FALSE;
    }
    /* FIXME: we should probably have a per-app option, and maybe a message box */
    FIXME( "No-exec fault triggered at %p, enabling work-around\n", addr );
    return VirtualProtect( addr, 1, info.Protect, NULL );
}


/*******************************************************************
 *         UnhandledExceptionFilter   (KERNEL32.@)
 */
LONG WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS epointers)
{
    const EXCEPTION_RECORD *rec = epointers->ExceptionRecord;

    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2)
    {
        switch(rec->ExceptionInformation[0])
        {
        case EXCEPTION_WRITE_FAULT:
            if (check_resource_write( (void *)rec->ExceptionInformation[1] ))
                return EXCEPTION_CONTINUE_EXECUTION;
            break;
        case EXCEPTION_EXECUTE_FAULT:
            if (check_no_exec( (void *)rec->ExceptionInformation[1] ))
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


/***********************************************************************
 *            SetUnhandledExceptionFilter   (KERNEL32.@)
 */
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(
                                          LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
    LPTOP_LEVEL_EXCEPTION_FILTER old = top_filter;
    top_filter = filter;
    return old;
}


/**************************************************************************
 *           FatalAppExitA   (KERNEL32.@)
 */
void WINAPI FatalAppExitA( UINT action, LPCSTR str )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    MessageBoxA_funcptr pMessageBoxA = NULL;

    WARN("AppExit\n");

    if (mod) pMessageBoxA = (MessageBoxA_funcptr)GetProcAddress( mod, "MessageBoxA" );
    if (pMessageBoxA) pMessageBoxA( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    else ERR( "%s\n", debugstr_a(str) );
    ExitProcess(0);
}


/**************************************************************************
 *           FatalAppExitW   (KERNEL32.@)
 */
void WINAPI FatalAppExitW( UINT action, LPCWSTR str )
{
    static const WCHAR User32DllW[] = {'u','s','e','r','3','2','.','d','l','l',0};

    HMODULE mod = GetModuleHandleW( User32DllW );
    MessageBoxW_funcptr pMessageBoxW = NULL;

    WARN("AppExit\n");

    if (mod) pMessageBoxW = (MessageBoxW_funcptr)GetProcAddress( mod, "MessageBoxW" );
    if (pMessageBoxW) pMessageBoxW( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    else ERR( "%s\n", debugstr_w(str) );
    ExitProcess(0);
}


/**************************************************************************
 *           FatalExit   (KERNEL32.@)
 */
void WINAPI FatalExit(int ExitCode)
{
    WARN("FatalExit\n");
    ExitProcess(ExitCode);
}
