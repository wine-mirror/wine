/*
 * Win32 exception functions
 *
 * Copyright (c) 1996 Onno Hovers, (onno@stack.urc.tue.nl)
 * Copyright (c) 1999 Alexandre Julliard
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

#include <stdio.h>
#include "windef.h"
#include "winerror.h"
#include "ntddk.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/exception.h"
#include "thread.h"
#include "stackframe.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(seh);

static PTOP_LEVEL_EXCEPTION_FILTER top_filter;

typedef INT (WINAPI *MessageBoxA_funcptr)(HWND,LPCSTR,LPCSTR,UINT);
typedef INT (WINAPI *MessageBoxW_funcptr)(HWND,LPCWSTR,LPCWSTR,UINT);

/*******************************************************************
 *         RaiseException  (KERNEL32.@)
 */
void WINAPI RaiseException( DWORD code, DWORD flags, DWORD nbargs, const LPDWORD args )
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
static void format_exception_msg( const EXCEPTION_POINTERS *ptr, char *buffer )
{
    const EXCEPTION_RECORD *rec = ptr->ExceptionRecord;

    switch(rec->ExceptionCode)
    {
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        sprintf( buffer, "Unhandled division by zero" );
        break;
    case EXCEPTION_INT_OVERFLOW:
        sprintf( buffer, "Unhandled overflow" );
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        sprintf( buffer, "Unhandled array bounds" );
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        sprintf( buffer, "Unhandled illegal instruction" );
        break;
    case EXCEPTION_STACK_OVERFLOW:
        sprintf( buffer, "Unhandled stack overflow" );
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        sprintf( buffer, "Unhandled priviledged instruction" );
        break;
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
            sprintf( buffer, "Unhandled page fault on %s access to 0x%08lx",
                     rec->ExceptionInformation[0] ? "write" : "read",
                     rec->ExceptionInformation[1]);
        else
            sprintf( buffer, "Unhandled page fault");
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        sprintf( buffer, "Unhandled alignment" );
        break;
    case CONTROL_C_EXIT:
        sprintf( buffer, "Unhandled ^C");
        break;
    case EXCEPTION_CRITICAL_SECTION_WAIT:
        sprintf( buffer, "Critical section %08lx wait failed",
                 rec->ExceptionInformation[0]);
        break;
    case EXCEPTION_WINE_STUB:
        sprintf( buffer, "Unimplemented function %s.%s called",
                 (char *)rec->ExceptionInformation[0], (char *)rec->ExceptionInformation[1] );
        break;
    case EXCEPTION_VM86_INTx:
        sprintf( buffer, "Unhandled interrupt %02lx in vm86 mode",
                 rec->ExceptionInformation[0]);
        break;
    case EXCEPTION_VM86_STI:
        sprintf( buffer, "Unhandled sti in vm86 mode");
        break;
    case EXCEPTION_VM86_PICRETURN:
        sprintf( buffer, "Unhandled PIC return in vm86 mode");
        break;
    default:
        sprintf( buffer, "Unhandled exception 0x%08lx", rec->ExceptionCode);
        break;
    }
#ifdef __i386__
    if (ptr->ContextRecord->SegCs != __get_cs())
        sprintf( buffer+strlen(buffer), " at address 0x%04lx:0x%08lx.\n",
                 ptr->ContextRecord->SegCs, (DWORD)ptr->ExceptionRecord->ExceptionAddress );
    else
#endif
    sprintf( buffer+strlen(buffer), " at address 0x%08lx.\n",
             (DWORD)ptr->ExceptionRecord->ExceptionAddress );
    strcat( buffer, "Do you wish to debug it ?" );
}


/**********************************************************************
 *           send_debug_event
 *
 * Send an EXCEPTION_DEBUG_EVENT event to the debugger.
 */
static int send_debug_event( EXCEPTION_RECORD *rec, int first_chance, CONTEXT *context )
{
    int ret;
    HANDLE handle = 0;

    SERVER_START_REQ
    {
        struct queue_exception_event_request *req = server_alloc_req( sizeof(*req),
                                                                      sizeof(*rec)+sizeof(*context) );
        CONTEXT *context_ptr = server_data_ptr(req);
        EXCEPTION_RECORD *rec_ptr = (EXCEPTION_RECORD *)(context_ptr + 1);
        req->first   = first_chance;
        *rec_ptr     = *rec;
        *context_ptr = *context;
        if (!server_call_noerr( REQ_QUEUE_EXCEPTION_EVENT )) handle = req->handle;
    }
    SERVER_END_REQ;
    if (!handle) return 0;  /* no debugger present or other error */

    /* No need to wait on the handle since the process gets suspended
     * once the event is passed to the debugger, so when we get back
     * here the event has been continued already.
     */
    SERVER_START_REQ
    {
        struct get_exception_status_request *req = server_alloc_req( sizeof(*req), sizeof(*context) );
        req->handle = handle;
        if (!server_call_noerr( REQ_GET_EXCEPTION_STATUS ))
            *context = *(CONTEXT *)server_data_ptr(req);
        ret = req->status;
    }
    SERVER_END_REQ;
    NtClose( handle );
    return ret;
}


/*******************************************************************
 *         UnhandledExceptionFilter   (KERNEL32.@)
 */
DWORD WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS epointers)
{
    char 		format[256];
    char 		buffer[256];
    HKEY		hDbgConf;
    DWORD		bAuto = FALSE;
    DWORD		ret = EXCEPTION_EXECUTE_HANDLER;
    int status;

    /* send a last chance event to the debugger */
    status = send_debug_event( epointers->ExceptionRecord, FALSE, epointers->ContextRecord );
    switch (status)
    {
    case DBG_CONTINUE: 
        return EXCEPTION_CONTINUE_EXECUTION;
    case DBG_EXCEPTION_NOT_HANDLED: 
        TerminateProcess( GetCurrentProcess(), epointers->ExceptionRecord->ExceptionCode );
        break; /* not reached */
    case 0: /* no debugger is present */
        break;
    default: 	
        FIXME("Unsupported yet debug continue value %d (please report)\n", status);
    }

    if (top_filter)
    {
        DWORD ret = top_filter( epointers );
        if (ret != EXCEPTION_CONTINUE_SEARCH) return ret;
    }

    /* FIXME: Should check the current error mode */

    if (!RegOpenKeyA(HKEY_LOCAL_MACHINE, 
		     "Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", 
		     &hDbgConf)) {
       DWORD 	type;
       DWORD 	count;

       count = sizeof(format);
       if (RegQueryValueExA(hDbgConf, "Debugger", 0, &type, format, &count))
	  format[0] = 0;

       count = sizeof(bAuto);
       if (RegQueryValueExA(hDbgConf, "Auto", 0, &type, (char*)&bAuto, &count))
	  bAuto = TRUE;
       else if (type == REG_SZ)
       {
           char autostr[10];
           count = sizeof(autostr);
           if (!RegQueryValueExA(hDbgConf, "Auto", 0, &type, autostr, &count))
               bAuto = atoi(autostr);
       }
       RegCloseKey(hDbgConf);
    } else {
       /* format[0] = 0; */
       strcpy(format, "debugger/winedbg %ld %ld");
    }

    if (!bAuto)
    {
        HMODULE mod = GetModuleHandleA( "user32.dll" );
        MessageBoxA_funcptr pMessageBoxA = NULL;
        if (mod) pMessageBoxA = (MessageBoxA_funcptr)GetProcAddress( mod, "MessageBoxA" );
        if (pMessageBoxA)
        {
            format_exception_msg( epointers, buffer );
            if (pMessageBoxA( 0, buffer, "Error", MB_YESNO | MB_ICONHAND ) == IDNO)
            {
                TRACE("Killing process\n");
                return EXCEPTION_EXECUTE_HANDLER;
            }
        }
    }

    if (format[0]) {
       HANDLE			hEvent;
       PROCESS_INFORMATION	info;
       STARTUPINFOA		startup;
       OBJECT_ATTRIBUTES	attr;

       attr.Length                   = sizeof(attr);
       attr.RootDirectory            = 0;
       attr.Attributes               = OBJ_INHERIT;
       attr.ObjectName               = NULL;
       attr.SecurityDescriptor       = NULL;
       attr.SecurityQualityOfService = NULL;

       TRACE("Starting debugger (fmt=%s)\n", format);
       NtCreateEvent( &hEvent, EVENT_ALL_ACCESS, &attr, FALSE, FALSE );
       sprintf(buffer, format, GetCurrentProcessId(), hEvent);
       memset(&startup, 0, sizeof(startup));
       startup.cb = sizeof(startup);
       startup.dwFlags = STARTF_USESHOWWINDOW;
       startup.wShowWindow = SW_SHOWNORMAL;
       if (CreateProcessA(NULL, buffer, NULL, NULL, 
			  TRUE, 0, NULL, NULL, &startup, &info)) {
	  WaitForSingleObject(hEvent, INFINITE);
	  ret = EXCEPTION_CONTINUE_SEARCH;
       } else {
           ERR("Couldn't start debugger (%s) (%ld)\n"
               "Read the Wine Developers Guide on how to set up winedbg or another debugger\n",
               buffer, GetLastError());
       }
       CloseHandle(hEvent);
    } else {
       ERR("No standard debugger defined in the registry => no debugging session\n");
    }
    
    return ret;
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
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    MessageBoxW_funcptr pMessageBoxW = NULL;

    WARN("AppExit\n");

    if (mod) pMessageBoxW = (MessageBoxW_funcptr)GetProcAddress( mod, "MessageBoxW" );
    if (pMessageBoxW) pMessageBoxW( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    else ERR( "%s\n", debugstr_w(str) );
    ExitProcess(0);
}
