/*
 * Win32 exception functions
 *
 * Copyright (c) 1996 Onno Hovers, (onno@stack.urc.tue.nl)
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
#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "ldt.h"
#include "process.h"
#include "thread.h"
#include "stddebug.h"
#include "debug.h"
#include "except.h"

#define TEB_EXCEPTION_FRAME(pcontext) \
    ((PEXCEPTION_FRAME)((TEB *)GET_SEL_BASE((pcontext)->SegFs))->except)


/*******************************************************************
 *         RtlUnwind  (KERNEL32.443)
 *
 *  This function is undocumented. This is the general idea of 
 *  RtlUnwind, though. Note that error handling is not yet implemented.
 */
void RtlUnwind( PEXCEPTION_FRAME pEndFrame, LPVOID unusedEip, 
                PEXCEPTION_RECORD pRecord, DWORD returnEax,
                PCONTEXT pcontext /* Wine additional parameter */ )
{   
   EXCEPTION_RECORD record;
   DWORD            dispatch;
   int              retval;
  
   pcontext->Eax=returnEax;
   
   /* build an exception record, if we do not have one */
   if(!pRecord)
   {
     record.ExceptionCode    = STATUS_INVALID_DISPOSITION;
     record.ExceptionFlags   = 0;
     record.ExceptionRecord  = NULL;
     record.ExceptionAddress = (LPVOID)pcontext->Eip; 
     record.NumberParameters = 0;
     pRecord = &record;
   }

   if(pEndFrame)
     pRecord->ExceptionFlags|=EH_UNWINDING;
   else
     pRecord->ExceptionFlags|=EH_UNWINDING | EH_EXIT_UNWIND;
  
   /* get chain of exception frames */      
   while ((TEB_EXCEPTION_FRAME(pcontext) != NULL) &&
          (TEB_EXCEPTION_FRAME(pcontext) != ((void *)0xffffffff)) &&
          (TEB_EXCEPTION_FRAME(pcontext) != pEndFrame))
   {
       dprintf_win32( stddeb, "calling exception handler at 0x%x\n",
                      (int)TEB_EXCEPTION_FRAME(pcontext)->Handler );

       dispatch=0;       
       retval = TEB_EXCEPTION_FRAME(pcontext)->Handler( pRecord,
                                                TEB_EXCEPTION_FRAME(pcontext),
                                                pcontext, &dispatch);
                                         
       dprintf_win32(stddeb,"exception handler returns 0x%x, dispatch=0x%x\n",
                              retval, (int) dispatch);
  
       if (retval == ExceptionCollidedUnwind)
           TEB_EXCEPTION_FRAME(pcontext) = (LPVOID)dispatch;
       else if (TEB_EXCEPTION_FRAME(pcontext) != pEndFrame)
           TEB_EXCEPTION_FRAME(pcontext) = TEB_EXCEPTION_FRAME(pcontext)->Prev;
       else
          break;  
   }
}


/*******************************************************************
 *         RaiseException  (KERNEL32.418)
 */
void RaiseException(DWORD dwExceptionCode,
		    DWORD dwExceptionFlags,
		    DWORD cArguments,
		    const LPDWORD lpArguments,
		    PCONTEXT pcontext /* Wine additional parameter */ )
{
    PEXCEPTION_FRAME    pframe; 
    EXCEPTION_RECORD    record;
    DWORD               dispatch; /* is this used in raising exceptions ?? */
    int                 retval;
    int                 i;
    
    /* compose an exception record */ 
    
    record.ExceptionCode       = dwExceptionCode;   
    record.ExceptionFlags      = dwExceptionFlags;
    record.ExceptionRecord     = NULL;
    record.NumberParameters    = cArguments;
    record.ExceptionAddress    = (LPVOID) pcontext->Eip;
    
    if (lpArguments) for( i = 0; i < cArguments; i++)
        record.ExceptionInformation[i] = lpArguments[i];
    
    /* get chain of exception frames */    
    
    retval = ExceptionContinueSearch;    
    pframe = TEB_EXCEPTION_FRAME( pcontext );
    
    while((pframe!=NULL)&&(pframe!=((void *)0xFFFFFFFF)))
    {
       dprintf_win32(stddeb,"calling exception handler at 0x%x\n",
                                                (int) pframe->Handler);
       dispatch=0;  
       retval=pframe->Handler(&record,pframe,pcontext,&dispatch);
 
       dprintf_win32(stddeb,"exception handler returns 0x%x, dispatch=0x%x\n",
                              retval, (int) dispatch);
                              
       if(retval==ExceptionContinueExecution)
          break;
       pframe=pframe->Prev;
   }

   if (retval!=ExceptionContinueExecution)
   {    
       /* FIXME: what should we do here? */
       dprintf_win32(stddeb,"no handler wanted to handle the exception, exiting\n");
       ExitProcess(dwExceptionCode); /* what status should be used here ? */
   }
}


/*******************************************************************
 *         UnhandledExceptionFilter   (KERNEL32.537)
 */
DWORD UnhandledExceptionFilter(PEXCEPTION_POINTERS epointers)
{
    char message[80];

    /* FIXME: Should check if the process is being debugged */

    if (pCurrentProcess && pCurrentProcess->top_filter)
    {
        DWORD ret = pCurrentProcess->top_filter( epointers );
        if (ret != EXCEPTION_CONTINUE_SEARCH) return ret;
    }

    /* FIXME: Should check the current error mode */

    sprintf( message, "Unhandled exception 0x%08lx at address 0x%08lx.",
             epointers->ExceptionRecord->ExceptionCode,
             (DWORD)epointers->ExceptionRecord->ExceptionAddress );
    MessageBox32A( 0, message, "Error", MB_OK | MB_ICONHAND );
    return EXCEPTION_EXECUTE_HANDLER;
}


/*************************************************************
 *            SetUnhandledExceptionFilter   (KERNEL32.516)
 */
LPTOP_LEVEL_EXCEPTION_FILTER SetUnhandledExceptionFilter(
                                          LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
    LPTOP_LEVEL_EXCEPTION_FILTER old = pCurrentProcess->top_filter;
    pCurrentProcess->top_filter = filter;
    return old;
}
