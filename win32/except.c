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
#ifndef WINELIB

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "stddebug.h"
#include "debug.h"
#include "except.h"

WINAPI DWORD KERNEL32_537(PEXCEPTION_POINTERS ptrs);

LPTOP_LEVEL_EXCEPTION_FILTER       pTopExcHandler=
           (LPTOP_LEVEL_EXCEPTION_FILTER) KERNEL32_537;

/*
 *       EXC_RtlUnwind
 *
 *  This function is undocumented. This is the general idea of 
 *  RtlUnwind, though. Note that error handling is not yet implemented
 *  
 */

void EXC_RtlUnwind(PEXCEPTION_FRAME pEndFrame,PVOID unusedEip, 
                   PEXCEPTION_RECORD pRecord, DWORD returnEax,
                   PCONTEXT pcontext)
{   
   EXCEPTION_RECORD record;
   DWORD            dispatch;
   int              retval;
  
   pcontext->Eax=returnEax;
   
   /* build an exception record, if we do not have one */
   if(!pRecord)
   {
     record.ExceptionCode=   0xC0000026;  /* invalid disposition */ 
     record.ExceptionFlags=  0;
     record.ExceptionRecord= NULL;
     record.ExceptionAddress=(PVOID) pcontext->Eip; 
     record.NumberParameters= 0;
     pRecord=&record;     
   }

   if(pEndFrame)
     pRecord->ExceptionFlags|=EH_UNWINDING;
   else
     pRecord->ExceptionFlags|=EH_UNWINDING | EH_EXIT_UNWIND;
  
   /* get chain of exception frames */      
   while((TebExceptionFrame!=NULL)&&
         (TebExceptionFrame!=((void *)-1)) &&
         (TebExceptionFrame!=pEndFrame))
   {
       dprintf_win32(stddeb,"calling exception handler at 0x%x\n",
                                           (int) TebExceptionFrame->Handler);

       dispatch=0;       
       retval=TebExceptionFrame->Handler(pRecord, TebExceptionFrame,
                                         pcontext, &dispatch);
                                         
       dprintf_win32(stddeb,"exception handler returns 0x%x, dispatch=0x%x\n",
                              retval, (int) dispatch);
  
       if(retval==ExceptionCollidedUnwind)
          TebExceptionFrame=(PVOID) dispatch;
       else if(TebExceptionFrame!=pEndFrame)
          TebExceptionFrame=TebExceptionFrame->Prev;
       else
          break;  
   }
}

/*
 *    EXC_RaiseException
 *
 */
 
VOID EXC_RaiseException(DWORD dwExceptionCode,
		    DWORD dwExceptionFlags,
		    DWORD cArguments,
		    const LPDWORD lpArguments,
		    PCONTEXT pcontext)
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
    record.ExceptionAddress    = (PVOID) pcontext->Eip;
    
    for(i=0;i<cArguments;i++)
       record.ExceptionInformation[i]=lpArguments[i];
    
    /* get chain of exception frames */    
    
    retval=ExceptionContinueSearch;    
    pframe=TebExceptionFrame;
    
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

   if(retval!=ExceptionContinueExecution)
   {    
      retval=EXC_CallUnhandledExceptionFilter(&record,pcontext);
      if(retval!=EXCEPTION_CONTINUE_EXECUTION)
      {
         dprintf_win32(stddeb,"no handler wanted to handle "
                              "the exception, exiting\n"     );
         ExitProcess(dwExceptionCode); /* what status should be used here ? */         
      }                   
   } 
}

/*******************************************************************
 *         UnhandledExceptionFilter (KERNEL32.537)
 * 
 *  This is the unhandled exception code. 
 *  Actually, this should show up a dialog box, with all kinds of
 *  fancy debugging information. It does nothing now!
 */
 
DWORD WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS epointers)
{
   PEXCEPTION_RECORD pRecord;
   PCONTEXT          pContext;
   
   pRecord=epointers->ExceptionRecord;
   pContext=epointers->ContextRecord;
   
   if(pRecord->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND) )
   {
      dprintf_win32(stddeb,"UnhandledExceptionFilter: exiting\n");
      ExitProcess(pRecord->ExceptionCode);            
   }
   else
   {
      RtlUnwind(0,pRecord,0,-1 );
   }
   
   /* 
    * This is just to avoid a warning, code should not get here 
    * if it does, EXC_RaiseException will terminate it. 
    */
   return EXCEPTION_CONTINUE_SEARCH;
}

/*************************************************************
 *    SetUnhandledExceptionFilter    (KERNEL32.516)
 *  
 *
 */
  
WINAPI LPTOP_LEVEL_EXCEPTION_FILTER 
        SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER efilter)
{  
   pTopExcHandler=efilter;
   return efilter;
}

#endif  /* WINELIB */
