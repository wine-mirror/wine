/*
 * NT exception handling routines
 * 
 * Copyright 1999 Turchanov Sergey
 * Copyright 1999 Alexandre Julliard
 */

#include <signal.h>
#include "winnt.h"
#include "ntddk.h"
#include "process.h"
#include "global.h"
#include "wine/exception.h"
#include "stackframe.h"
#include "miscemu.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(seh)

/* Exception record for handling exceptions happening inside exception handlers */
typedef struct
{
    EXCEPTION_FRAME frame;
    EXCEPTION_FRAME *prevFrame;
} EXC_NESTED_FRAME;

 
#ifdef __i386__
# define GET_IP(context) ((LPVOID)(context)->Eip)
#else
# error You must define GET_IP for this CPU
#endif  /* __i386__ */

/* Default hook for built-in debugger */
static DWORD default_hook( EXCEPTION_RECORD *rec, CONTEXT *ctx, BOOL first )
{
    if (!first)
    {
    	DPRINTF( "stopping process due to unhandled exception %08lx.\n",
                 rec->ExceptionCode );
	raise( SIGSTOP );
    }
    return 0;  /* not handled */
}
static DEBUGHOOK debug_hook = default_hook;

/*******************************************************************
 *         EXC_SetDebugEventHook
 *         EXC_GetDebugEventHook
 *
 * Set/Get the hook for the built-in debugger.
 *
 * FIXME: the built-in debugger should use the normal debug events.
 */
void EXC_SetDebugEventHook( DEBUGHOOK hook )
{
    debug_hook = hook;
}
DEBUGHOOK EXC_GetDebugEventHook(void)
{
    return debug_hook;
}

/*******************************************************************
 *         EXC_RaiseHandler
 *
 * Handler for exceptions happening inside a handler.
 */
static DWORD EXC_RaiseHandler( EXCEPTION_RECORD *rec, EXCEPTION_FRAME *frame,
                               CONTEXT *context, EXCEPTION_FRAME **dispatcher )
{
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionNestedException;
}


/*******************************************************************
 *         EXC_UnwindHandler
 *
 * Handler for exceptions happening inside an unwind handler.
 */
static DWORD EXC_UnwindHandler( EXCEPTION_RECORD *rec, EXCEPTION_FRAME *frame,
                                CONTEXT *context, EXCEPTION_FRAME **dispatcher )
{
    if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionCollidedUnwind;
}


/*******************************************************************
 *         EXC_CallHandler
 *
 * Call an exception handler, setting up an exception frame to catch exceptions
 * happening during the handler execution.
 * Please do not change the first 4 parameters order in any way - some exceptions handlers
 * rely on Base Pointer (EBP) to have a fixed position related to the exception frame
 */
static DWORD EXC_CallHandler( EXCEPTION_RECORD *record, EXCEPTION_FRAME *frame,
                              CONTEXT *context, EXCEPTION_FRAME **dispatcher, 
                              PEXCEPTION_HANDLER handler, PEXCEPTION_HANDLER nested_handler)
{
    EXC_NESTED_FRAME newframe;
    DWORD ret;

    newframe.frame.Handler = nested_handler;
    newframe.prevFrame     = frame;
    EXC_push_frame( &newframe.frame );
    TRACE( "calling handler at %p code=%lx flags=%lx\n",
           handler, record->ExceptionCode, record->ExceptionFlags );
    ret = handler( record, frame, context, dispatcher );
    TRACE( "handler returned %lx\n", ret );
    EXC_pop_frame( &newframe.frame );
    return ret;
}


/*******************************************************************
 *         EXC_DefaultHandling
 *
 * Default handling for exceptions. Called when we didn't find a suitable handler.
 */
static void EXC_DefaultHandling( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    if ((PROCESS_Current()->flags & PDB32_DEBUGGED) &&
        (DEBUG_SendExceptionEvent( rec, FALSE ) == DBG_CONTINUE))
        return;  /* continue execution */

    if (debug_hook( rec, context, FALSE ) == DBG_CONTINUE)
        return;  /* continue execution */

    if (rec->ExceptionFlags & EH_STACK_INVALID)
        ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
    else if (rec->ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
        ERR("Process attempted to continue execution after noncontinuable exception.\n");
    else
        ERR("Unhandled exception code %lx flags %lx addr %p\n",
            rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
    TerminateProcess( GetCurrentProcess(), 1 );
}


/***********************************************************************
 *            RtlRaiseException  (NTDLL.464)
 */
void WINAPI REGS_FUNC(RtlRaiseException)( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    PEXCEPTION_FRAME frame, dispatch, nested_frame;
    EXCEPTION_RECORD newrec;
    DWORD res;

    TRACE( "code=%lx flags=%lx\n", rec->ExceptionCode, rec->ExceptionFlags );

    if ((PROCESS_Current()->flags & PDB32_DEBUGGED) &&
        (DEBUG_SendExceptionEvent( rec, TRUE ) == DBG_CONTINUE))
        return;  /* continue execution */

    if (debug_hook( rec, context, TRUE ) == DBG_CONTINUE)
        return;  /* continue execution */

    frame = NtCurrentTeb()->except;
    nested_frame = NULL;
    while (frame != (PEXCEPTION_FRAME)0xFFFFFFFF)
    {
        /* Check frame address */
        if (((void*)frame < NtCurrentTeb()->stack_low) ||
            ((void*)(frame+1) > NtCurrentTeb()->stack_top) ||
            (int)frame & 3)
        {
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        /* Call handler */
        res = EXC_CallHandler( rec, frame, context, &dispatch, frame->Handler, EXC_RaiseHandler );
        if (frame == nested_frame)
        {
            /* no longer nested */
            nested_frame = NULL;
            rec->ExceptionFlags &= ~EH_NESTED_CALL;
        }

        switch(res)
        {
        case ExceptionContinueExecution:
            if (!(rec->ExceptionFlags & EH_NONCONTINUABLE)) return;
            newrec.ExceptionCode    = STATUS_NONCONTINUABLE_EXCEPTION;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = rec;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
            break;
        case ExceptionContinueSearch:
            break;
        case ExceptionNestedException:
            if (nested_frame < dispatch) nested_frame = dispatch;
            rec->ExceptionFlags |= EH_NESTED_CALL;
            break;
        default:
            newrec.ExceptionCode    = STATUS_INVALID_DISPOSITION;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = rec;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
            break;
        }
        frame = frame->Prev;
    }
    EXC_DefaultHandling( rec, context );
}


/*******************************************************************
 *         RtlUnwind  (KERNEL32.590) (NTDLL.518)
 */
void WINAPI REGS_FUNC(RtlUnwind)( PEXCEPTION_FRAME pEndFrame, LPVOID unusedEip, 
                                  PEXCEPTION_RECORD pRecord, DWORD returnEax,
                                  CONTEXT *context )
{
    EXCEPTION_RECORD record, newrec;
    PEXCEPTION_FRAME frame, dispatch;

#ifdef __i386__
    context->Eax = returnEax;
#endif

    /* build an exception record, if we do not have one */
    if (!pRecord)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = GET_IP(context); 
        record.NumberParameters = 0;
        pRecord = &record;
    }

    pRecord->ExceptionFlags |= EH_UNWINDING | (pEndFrame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx\n", pRecord->ExceptionCode, pRecord->ExceptionFlags );

    /* get chain of exception frames */
    frame = NtCurrentTeb()->except;
    while ((frame != (PEXCEPTION_FRAME)0xffffffff) && (frame != pEndFrame))
    {
        /* Check frame address */
        if (pEndFrame && (frame > pEndFrame))
        {
            newrec.ExceptionCode    = STATUS_INVALID_UNWIND_TARGET;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = pRecord;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
        }
        if (((void*)frame < NtCurrentTeb()->stack_low) ||
            ((void*)(frame+1) > NtCurrentTeb()->stack_top) ||
            (int)frame & 3)
        {
            newrec.ExceptionCode    = STATUS_BAD_STACK;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = pRecord;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
        }

        /* Call handler */
        switch(EXC_CallHandler( pRecord, frame, context, &dispatch,
                                frame->Handler, EXC_UnwindHandler ))
        {
        case ExceptionContinueSearch:
            break;
        case ExceptionCollidedUnwind:
            frame = dispatch;
            break;
        default:
            newrec.ExceptionCode    = STATUS_INVALID_DISPOSITION;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = pRecord;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
            break;
        }
        frame = EXC_pop_frame( frame );
    }
}


/*******************************************************************
 *         NtRaiseException    (NTDLL.175)
 *
 * Real prototype:
 *    DWORD WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *ctx, BOOL first );
 */
void WINAPI REGS_FUNC(NtRaiseException)( EXCEPTION_RECORD *rec, CONTEXT *ctx,
                                         BOOL first, CONTEXT *context )
{
    REGS_FUNC(RtlRaiseException)( rec, ctx );
    *context = *ctx;
}


/***********************************************************************
 *            RtlRaiseStatus  (NTDLL.465)
 *
 * Raise an exception with ExceptionCode = status
 */
void WINAPI RtlRaiseStatus( NTSTATUS status )
{
    EXCEPTION_RECORD ExceptionRec;

    ExceptionRec.ExceptionCode    = status;
    ExceptionRec.ExceptionFlags   = EH_NONCONTINUABLE;
    ExceptionRec.ExceptionRecord  = NULL;
    ExceptionRec.NumberParameters = 0;
    RtlRaiseException( &ExceptionRec );
}


/***********************************************************************
 *           DebugBreak   (KERNEL32.181)
 */
void WINAPI REGS_FUNC(DebugBreak)( CONTEXT *context )
{
    EXCEPTION_RECORD rec;

    rec.ExceptionCode    = EXCEPTION_BREAKPOINT;
    rec.ExceptionFlags   = 0;
    rec.ExceptionRecord  = NULL;
    rec.NumberParameters = 0;
    REGS_FUNC(RtlRaiseException)( &rec, context );
}


/***********************************************************************
 *           DebugBreak16   (KERNEL.203)
 */
void WINAPI DebugBreak16( CONTEXT86 *context )
{
#ifdef __i386__
    REGS_FUNC(DebugBreak)( context );
#endif  /* defined(__i386__) */
}
