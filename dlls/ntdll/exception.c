/*
 * NT exception handling routines
 *
 * Copyright 1999 Turchanov Sergey
 * Copyright 1999 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <signal.h>
#include <stdarg.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "thread.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "excpt.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

/* Exception record for handling exceptions happening inside exception handlers */
typedef struct
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_REGISTRATION_RECORD *prevFrame;
} EXC_NESTED_FRAME;

typedef struct
{
    struct list                 entry;
    PVECTORED_EXCEPTION_HANDLER func;
} VECTORED_HANDLER;

static struct list vectored_handlers = LIST_INIT(vectored_handlers);

static CRITICAL_SECTION vectored_handlers_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &vectored_handlers_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": vectored_handlers_section") }
};
static CRITICAL_SECTION vectored_handlers_section = { &critsect_debug, -1, 0, 0, 0, 0 };

#ifdef __i386__
# define GET_IP(context) ((LPVOID)(context)->Eip)
#elif defined(__sparc__)
# define GET_IP(context) ((LPVOID)(context)->pc)
#elif defined(__powerpc__)
# define GET_IP(context) ((LPVOID)(context)->Iar)
#else
# error You must define GET_IP for this CPU
#endif

void WINAPI EXC_RtlRaiseException( PEXCEPTION_RECORD, PCONTEXT );
void WINAPI EXC_RtlUnwind( PEXCEPTION_REGISTRATION_RECORD, LPVOID,
                           PEXCEPTION_RECORD, DWORD, PCONTEXT );
void WINAPI EXC_NtRaiseException( PEXCEPTION_RECORD, PCONTEXT,
                                  BOOL, PCONTEXT );

/*******************************************************************
 *         EXC_RaiseHandler
 *
 * Handler for exceptions happening inside a handler.
 */
static DWORD EXC_RaiseHandler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                               CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
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
static DWORD EXC_UnwindHandler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
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
static DWORD EXC_CallHandler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher,
                              PEXCEPTION_HANDLER handler, PEXCEPTION_HANDLER nested_handler)
{
    EXC_NESTED_FRAME newframe;
    DWORD ret;

    newframe.frame.Handler = nested_handler;
    newframe.prevFrame     = frame;
    __wine_push_frame( &newframe.frame );
    TRACE( "calling handler at %p code=%lx flags=%lx\n",
           handler, record->ExceptionCode, record->ExceptionFlags );
    ret = handler( record, frame, context, dispatcher );
    TRACE( "handler returned %lx\n", ret );
    __wine_pop_frame( &newframe.frame );
    return ret;
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

    if (!NtCurrentTeb()->Peb->BeingDebugged) return 0;  /* no debugger present */

    SERVER_START_REQ( queue_exception_event )
    {
        req->first   = first_chance;
        wine_server_add_data( req, context, sizeof(*context) );
        wine_server_add_data( req, rec, sizeof(*rec) );
        if (!wine_server_call( req )) handle = reply->handle;
    }
    SERVER_END_REQ;
    if (!handle) return 0;

    /* No need to wait on the handle since the process gets suspended
     * once the event is passed to the debugger, so when we get back
     * here the event has been continued already.
     */
    SERVER_START_REQ( get_exception_status )
    {
        req->handle = handle;
        wine_server_set_reply( req, context, sizeof(*context) );
        wine_server_call( req );
        ret = reply->status;
    }
    SERVER_END_REQ;
    NtClose( handle );
    return ret;
}


/**********************************************************************
 *           call_vectored_handlers
 *
 * Call the vectored handlers chain.
 */
static LONG call_vectored_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct list *ptr;
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    EXCEPTION_POINTERS except_ptrs;

    except_ptrs.ExceptionRecord = rec;
    except_ptrs.ContextRecord = context;

    RtlEnterCriticalSection( &vectored_handlers_section );
    LIST_FOR_EACH( ptr, &vectored_handlers )
    {
        VECTORED_HANDLER *handler = LIST_ENTRY( ptr, VECTORED_HANDLER, entry );
        ret = handler->func( &except_ptrs );
        if (ret == EXCEPTION_CONTINUE_EXECUTION) break;
    }
    RtlLeaveCriticalSection( &vectored_handlers_section );
    return ret;
}


/*******************************************************************
 *         EXC_DefaultHandling
 *
 * Default handling for exceptions. Called when we didn't find a suitable handler.
 */
static void EXC_DefaultHandling( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    if (send_debug_event( rec, FALSE, context ) == DBG_CONTINUE) return;  /* continue execution */

    if (rec->ExceptionFlags & EH_STACK_INVALID)
        ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
    else if (rec->ExceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
        ERR("Process attempted to continue execution after noncontinuable exception.\n");
    else
        ERR("Unhandled exception code %lx flags %lx addr %p\n",
            rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
    NtTerminateProcess( NtCurrentProcess(), 1 );
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
DEFINE_REGS_ENTRYPOINT_1( RtlRaiseException, EXC_RtlRaiseException, EXCEPTION_RECORD * );
void WINAPI EXC_RtlRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    PEXCEPTION_REGISTRATION_RECORD frame, dispatch, nested_frame;
    EXCEPTION_RECORD newrec;
    DWORD res, c;

    TRACE( "code=%lx flags=%lx addr=%p\n", rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
    for (c=0; c<rec->NumberParameters; c++) TRACE(" info[%ld]=%08lx\n", c, rec->ExceptionInformation[c]);
    if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
        FIXME( "call to unimplemented function %s.%s\n",
               (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );

    if (send_debug_event( rec, TRUE, context ) == DBG_CONTINUE) return;  /* continue execution */

    if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION) return;

    frame = NtCurrentTeb()->Tib.ExceptionList;
    nested_frame = NULL;
    while (frame != (PEXCEPTION_REGISTRATION_RECORD)~0UL)
    {
        /* Check frame address */
        if (((void*)frame < NtCurrentTeb()->Tib.StackLimit) ||
            ((void*)(frame+1) > NtCurrentTeb()->Tib.StackBase) ||
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
 *		RtlUnwind (NTDLL.@)
 */
DEFINE_REGS_ENTRYPOINT_4( RtlUnwind, EXC_RtlUnwind,
                          PVOID, PVOID, PEXCEPTION_RECORD, PVOID );
void WINAPI EXC_RtlUnwind( PEXCEPTION_REGISTRATION_RECORD pEndFrame, LPVOID unusedEip,
                           PEXCEPTION_RECORD pRecord, DWORD returnEax,
                           CONTEXT *context )
{
    EXCEPTION_RECORD record, newrec;
    PEXCEPTION_REGISTRATION_RECORD frame, dispatch;

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
    frame = NtCurrentTeb()->Tib.ExceptionList;
    while ((frame != (PEXCEPTION_REGISTRATION_RECORD)~0UL) && (frame != pEndFrame))
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
        if (((void*)frame < NtCurrentTeb()->Tib.StackLimit) ||
            ((void*)(frame+1) > NtCurrentTeb()->Tib.StackBase) ||
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
        frame = __wine_pop_frame( frame );
    }
}


/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
DEFINE_REGS_ENTRYPOINT_3( NtRaiseException, EXC_NtRaiseException,
                          EXCEPTION_RECORD *, CONTEXT *, BOOL );
void WINAPI EXC_NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *ctx,
                                  BOOL first, CONTEXT *context )
{
    EXC_RtlRaiseException( rec, ctx );
    *context = *ctx;
}


/***********************************************************************
 *            RtlRaiseStatus  (NTDLL.@)
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


/*******************************************************************
 *         RtlAddVectoredExceptionHandler   (NTDLL.@)
 */
PVOID WINAPI RtlAddVectoredExceptionHandler( ULONG first, PVECTORED_EXCEPTION_HANDLER func )
{
    VECTORED_HANDLER *handler = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*handler) );
    if (handler)
    {
        handler->func = func;
        RtlEnterCriticalSection( &vectored_handlers_section );
        if (first) list_add_head( &vectored_handlers, &handler->entry );
        else list_add_tail( &vectored_handlers, &handler->entry );
        RtlLeaveCriticalSection( &vectored_handlers_section );
    }
    return handler;
}


/*******************************************************************
 *         RtlRemoveVectoredExceptionHandler   (NTDLL.@)
 */
ULONG WINAPI RtlRemoveVectoredExceptionHandler( PVOID handler )
{
    struct list *ptr;
    ULONG ret = FALSE;

    RtlEnterCriticalSection( &vectored_handlers_section );
    LIST_FOR_EACH( ptr, &vectored_handlers )
    {
        if (ptr == &((VECTORED_HANDLER *)handler)->entry)
        {
            list_remove( ptr );
            ret = TRUE;
            break;
        }
    }
    RtlLeaveCriticalSection( &vectored_handlers_section );
    if (ret) RtlFreeHeap( GetProcessHeap(), 0, handler );
    return ret;
}


/*************************************************************
 *            __wine_exception_handler (NTDLL.@)
 *
 * Exception handler for exception blocks declared in Wine code.
 */
DWORD __wine_exception_handler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                                CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;

    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND | EH_NESTED_CALL))
        return ExceptionContinueSearch;
    if (wine_frame->u.filter)
    {
        EXCEPTION_POINTERS ptrs;
        ptrs.ExceptionRecord = record;
        ptrs.ContextRecord = context;
        switch(wine_frame->u.filter( &ptrs ))
        {
        case EXCEPTION_CONTINUE_SEARCH:
            return ExceptionContinueSearch;
        case EXCEPTION_CONTINUE_EXECUTION:
            return ExceptionContinueExecution;
        case EXCEPTION_EXECUTE_HANDLER:
            break;
        default:
            MESSAGE( "Invalid return value from exception filter\n" );
            assert( FALSE );
        }
    }
    /* hack to make GetExceptionCode() work in handler */
    wine_frame->ExceptionCode   = record->ExceptionCode;
    wine_frame->ExceptionRecord = wine_frame;

    RtlUnwind( frame, 0, record, 0 );
    __wine_pop_frame( frame );
    siglongjmp( wine_frame->jmp, 1 );
}


/*************************************************************
 *            __wine_finally_handler (NTDLL.@)
 *
 * Exception handler for try/finally blocks declared in Wine code.
 */
DWORD __wine_finally_handler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
        wine_frame->u.finally_func( FALSE );
    }
    return ExceptionContinueSearch;
}
