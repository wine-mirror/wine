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
 *  Whatever the reason, THIS SUCKS!! Ensuring portabilty or future 
 *  compatability may be valid reasons to keep some things undocumented. 
 *  But exception handling is so basic to Win32 that it should be 
 *  documented!
 *
 */

#include <stdio.h>
#include "windef.h"
#include "winerror.h"
#include "ntddk.h"
#include "wine/exception.h"
#include "ldt.h"
#include "callback.h"
#include "process.h"
#include "thread.h"
#include "stackframe.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(seh);


/*******************************************************************
 *         RaiseException  (KERNEL32.418)
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
 *         UnhandledExceptionFilter   (KERNEL32.537)
 */
DWORD WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS epointers)
{
    struct exception_event_request *req = get_req_buffer();
    PDB*		pdb = PROCESS_Current();
    char 		format[256];
    char 		buffer[256];
    HKEY		hDbgConf;
    DWORD		bAuto = FALSE;
    DWORD		ret = EXCEPTION_EXECUTE_HANDLER;

    /* send a last chance event to the debugger */
    req->record  = *epointers->ExceptionRecord;
    req->first   = 0;
    req->context = *epointers->ContextRecord;
    if (!server_call_noerr( REQ_EXCEPTION_EVENT )) *epointers->ContextRecord = req->context;
    switch (req->status)
    {
    case DBG_CONTINUE: 
       return EXCEPTION_CONTINUE_EXECUTION;
    case DBG_EXCEPTION_NOT_HANDLED: 
       TerminateProcess( GetCurrentProcess(), epointers->ExceptionRecord->ExceptionCode );
       break; /* not reached */
    case 0: /* no debugger is present */
       break;
    default: 	
       FIXME("Unsupported yet debug continue value %d (please report)\n", req->status);
    }

    if (pdb->top_filter)
    {
        DWORD ret = pdb->top_filter( epointers );
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
	  bAuto = FALSE;
       
       RegCloseKey(hDbgConf);
    } else {
       /* format[0] = 0; */
       strcpy(format, "debugger/winedbg %ld %ld");
    }

    if (!bAuto && Callout.MessageBoxA) {
       sprintf( buffer, "Unhandled exception 0x%08lx at address 0x%08lx.\n"
	                "Do you wish to debug it ?",
		epointers->ExceptionRecord->ExceptionCode,
		(DWORD)epointers->ExceptionRecord->ExceptionAddress );
       if (Callout.MessageBoxA( 0, buffer, "Error", MB_YESNO | MB_ICONHAND ) == IDNO) {
	  TRACE("Killing process\n");
	  return EXCEPTION_EXECUTE_HANDLER;
       }
    }
    
    if (format[0]) {
       HANDLE			hEvent;
       PROCESS_INFORMATION	info;
       STARTUPINFOA		startup;

       TRACE("Starting debugger (fmt=%s)\n", format);
       hEvent = ConvertToGlobalHandle(CreateEventA(NULL, FALSE, FALSE, NULL));
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
	  ERR("Couldn't start debugger (%s) (%ld)\n", buffer, GetLastError());
       }
       CloseHandle(hEvent);
    } else {
       ERR("No standard debugger defined in the registry => no debugging session\n");
    }
    
    return ret;
}


/***********************************************************************
 *            SetUnhandledExceptionFilter   (KERNEL32.516)
 */
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(
                                          LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
    PDB *pdb = PROCESS_Current();
    LPTOP_LEVEL_EXCEPTION_FILTER old = pdb->top_filter;
    pdb->top_filter = filter;
    return old;
}


/**************************************************************************
 *           FatalAppExit16   (KERNEL.137)
 */
void WINAPI FatalAppExit16( UINT16 action, LPCSTR str )
{
    WARN("AppExit\n");
    FatalAppExitA( action, str );
}


/**************************************************************************
 *           FatalAppExitA   (KERNEL32.108)
 */
void WINAPI FatalAppExitA( UINT action, LPCSTR str )
{
    WARN("AppExit\n");
    Callout.MessageBoxA( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}


/**************************************************************************
 *           FatalAppExitW   (KERNEL32.109)
 */
void WINAPI FatalAppExitW( UINT action, LPCWSTR str )
{
    WARN("AppExit\n");
    Callout.MessageBoxW( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    ExitProcess(0);
}
