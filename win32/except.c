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
 * Fixmes:
 *  -Most functions need better parameter checking.
 *  -I do not know how to handle exceptions within an exception handler.
 *   or what is done when ExceptionNestedException is returned from an 
 *   exception handler
 *  -Real exceptions are not yet implemented. only the exception functions
 *   are implemented. A real implementation needs some new code in
 *   loader/signal.c. There would also be a need for showing debugging
 *   information in UnhandledExceptionFilter.
 *
 */

#include <assert.h>
#include "winuser.h"
#include "winerror.h"
#include "ldt.h"
#include "process.h"
#include "thread.h"
#include "debugtools.h"
#include "except.h"
#include "stackframe.h"

DEFAULT_DEBUG_CHANNEL(seh)


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
    char message[80];
    PDB *pdb = PROCESS_Current();

    /* FIXME: Should check if the process is being debugged */

    if (pdb->top_filter)
    {
        DWORD ret = pdb->top_filter( epointers );
        if (ret != EXCEPTION_CONTINUE_SEARCH) return ret;
    }

    /* FIXME: Should check the current error mode */

    sprintf( message, "Unhandled exception 0x%08lx at address 0x%08lx.",
             epointers->ExceptionRecord->ExceptionCode,
             (DWORD)epointers->ExceptionRecord->ExceptionAddress );
    MessageBoxA( 0, message, "Error", MB_OK | MB_ICONHAND );
    return EXCEPTION_EXECUTE_HANDLER;
}


/*************************************************************
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
