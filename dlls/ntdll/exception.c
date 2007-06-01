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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "excpt.h"
#include "ntdll_misc.h"

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

static RTL_CRITICAL_SECTION vectored_handlers_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &vectored_handlers_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": vectored_handlers_section") }
};
static RTL_CRITICAL_SECTION vectored_handlers_section = { &critsect_debug, -1, 0, 0, 0, 0 };

#ifdef __i386__
# define GET_IP(context) ((LPVOID)(context)->Eip)
#elif defined(__sparc__)
# define GET_IP(context) ((LPVOID)(context)->pc)
#elif defined(__powerpc__)
# define GET_IP(context) ((LPVOID)(context)->Iar)
#elif defined(__ALPHA__)
# define GET_IP(context) ((LPVOID)(context)->Fir)
#elif defined(__x86_64__)
# define GET_IP(context) ((LPVOID)(context)->Rip)
#else
# error You must define GET_IP for this CPU
#endif

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
 *
 * For i386 this function is implemented in assembler in signal_i386.c.
 */
#ifndef __i386__
static DWORD EXC_CallHandler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher,
                              PEXCEPTION_HANDLER handler, PEXCEPTION_HANDLER nested_handler)
{
    EXC_NESTED_FRAME newframe;
    DWORD ret;

    newframe.frame.Handler = nested_handler;
    newframe.prevFrame     = frame;
    __wine_push_frame( &newframe.frame );
    ret = handler( record, frame, context, dispatcher );
    __wine_pop_frame( &newframe.frame );
    return ret;
}
#else
/* in signal_i386.c */
extern DWORD EXC_CallHandler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher,
                              PEXCEPTION_HANDLER handler, PEXCEPTION_HANDLER nested_handler);
#endif

/**********************************************************************
 *           wait_suspend
 *
 * Wait until the thread is no longer suspended.
 */
void wait_suspend( CONTEXT *context )
{
    LARGE_INTEGER timeout;
    int saved_errno = errno;

    /* store the context we got at suspend time */
    SERVER_START_REQ( set_thread_context )
    {
        req->handle  = GetCurrentThread();
        req->flags   = CONTEXT_FULL;
        req->suspend = 1;
        wine_server_add_data( req, context, sizeof(*context) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* wait with 0 timeout, will only return once the thread is no longer suspended */
    timeout.QuadPart = 0;
    NTDLL_wait_for_multiple_objects( 0, NULL, SELECT_INTERRUPTIBLE, &timeout, 0 );

    /* retrieve the new context */
    SERVER_START_REQ( get_thread_context )
    {
        req->handle  = GetCurrentThread();
        req->flags   = CONTEXT_FULL;
        req->suspend = 1;
        wine_server_set_reply( req, context, sizeof(*context) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    errno = saved_errno;
}


/**********************************************************************
 *           send_debug_event
 *
 * Send an EXCEPTION_DEBUG_EVENT event to the debugger.
 */
static NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, int first_chance, CONTEXT *context )
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

    NTDLL_wait_for_multiple_objects( 1, &handle, SELECT_INTERRUPTIBLE, NULL, 0 );

    SERVER_START_REQ( get_exception_status )
    {
        req->handle = handle;
        wine_server_set_reply( req, context, sizeof(*context) );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
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


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch, *nested_frame;
    DWORD res;

    frame = NtCurrentTeb()->Tib.ExceptionList;
    nested_frame = NULL;
    while (frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL)
    {
        /* Check frame address */
        if (((void*)frame < NtCurrentTeb()->Tib.StackLimit) ||
            ((void*)(frame+1) > NtCurrentTeb()->Tib.StackBase) ||
            (ULONG_PTR)frame & 3)
        {
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        /* Call handler */
        TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, rec->ExceptionCode, rec->ExceptionFlags );
        res = EXC_CallHandler( rec, frame, context, &dispatch, frame->Handler, EXC_RaiseHandler );
        TRACE( "handler at %p returned %x\n", frame->Handler, res );

        if (frame == nested_frame)
        {
            /* no longer nested */
            nested_frame = NULL;
            rec->ExceptionFlags &= ~EH_NESTED_CALL;
        }

        switch(res)
        {
        case ExceptionContinueExecution:
            if (!(rec->ExceptionFlags & EH_NONCONTINUABLE)) return STATUS_SUCCESS;
            return STATUS_NONCONTINUABLE_EXCEPTION;
        case ExceptionContinueSearch:
            break;
        case ExceptionNestedException:
            if (nested_frame < dispatch) nested_frame = dispatch;
            rec->ExceptionFlags |= EH_NESTED_CALL;
            break;
        default:
            return STATUS_INVALID_DISPOSITION;
        }
        frame = frame->Prev;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		raise_exception
 *
 * Implementation of NtRaiseException.
 */
static NTSTATUS raise_exception( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status;

    if (first_chance)
    {
        DWORD c;

        TRACE( "code=%x flags=%x addr=%p\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        for (c = 0; c < rec->NumberParameters; c++)
            TRACE( " info[%d]=%08lx\n", c, rec->ExceptionInformation[c] );
        if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
        {
            if (rec->ExceptionInformation[1] >> 16)
                MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
            else
                MESSAGE( "wine: Call from %p to unimplemented function %s.%ld, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        }
#ifdef __i386__
        else
        {
            TRACE(" eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
                  context->Eax, context->Ebx, context->Ecx,
                  context->Edx, context->Esi, context->Edi );
            TRACE(" ebp=%08x esp=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
                  context->Ebp, context->Esp, context->SegCs, context->SegDs,
                  context->SegEs, context->SegFs, context->SegGs, context->EFlags );
        }
#endif
        status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
            return STATUS_SUCCESS;

#ifdef __i386__
        /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
        if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Eip--;
#endif

        if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
            return STATUS_SUCCESS;

        if ((status = call_stack_handlers( rec, context )) != STATUS_UNHANDLED_EXCEPTION)
            return status;
    }

    /* last chance exception */

    status = send_debug_event( rec, FALSE, context );
    if (status != DBG_CONTINUE)
    {
        if (rec->ExceptionFlags & EH_STACK_INVALID)
            ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            ERR("Unhandled exception code %x flags %x addr %p\n",
                rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        NtTerminateProcess( NtCurrentProcess(), 1 );
    }
    return STATUS_SUCCESS;
}


/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status = raise_exception( rec, context, first_chance );
    if (status == STATUS_SUCCESS) NtSetContextThread( GetCurrentThread(), context );
    return status;
}

/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void WINAPI __regs_RtlRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status = raise_exception( rec, context, TRUE );
    if (status != STATUS_SUCCESS)
    {
        EXCEPTION_RECORD newrec;
        newrec.ExceptionCode    = status;
        newrec.ExceptionFlags   = EH_NONCONTINUABLE;
        newrec.ExceptionRecord  = rec;
        newrec.NumberParameters = 0;
        RtlRaiseException( &newrec );  /* never returns */
    }
}

/**********************************************************************/

#ifdef DEFINE_REGS_ENTRYPOINT
DEFINE_REGS_ENTRYPOINT( RtlRaiseException, 4, 4 )
#else
void WINAPI RtlRaiseException( EXCEPTION_RECORD *rec )
{
    CONTEXT context;
    memset( &context, 0, sizeof(context) );
    __regs_RtlRaiseException( rec, &context );
}
#endif


/*******************************************************************
 *		RtlUnwind (NTDLL.@)
 */
void WINAPI __regs_RtlUnwind( EXCEPTION_REGISTRATION_RECORD* pEndFrame, PVOID unusedEip,
                              PEXCEPTION_RECORD pRecord, PVOID returnEax, CONTEXT *context )
{
    EXCEPTION_RECORD record, newrec;
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch;
    DWORD res;

#ifdef __i386__
    context->Eax = (DWORD)returnEax;
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

    TRACE( "code=%x flags=%x\n", pRecord->ExceptionCode, pRecord->ExceptionFlags );

    /* get chain of exception frames */
    frame = NtCurrentTeb()->Tib.ExceptionList;
    while ((frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL) && (frame != pEndFrame))
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
            (UINT_PTR)frame & 3)
        {
            newrec.ExceptionCode    = STATUS_BAD_STACK;
            newrec.ExceptionFlags   = EH_NONCONTINUABLE;
            newrec.ExceptionRecord  = pRecord;
            newrec.NumberParameters = 0;
            RtlRaiseException( &newrec );  /* never returns */
        }

        /* Call handler */
        TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, pRecord->ExceptionCode, pRecord->ExceptionFlags );
        res = EXC_CallHandler( pRecord, frame, context, &dispatch, frame->Handler, EXC_UnwindHandler );
        TRACE( "handler at %p returned %x\n", frame->Handler, res );

        switch(res)
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

/**********************************************************************/

#ifdef DEFINE_REGS_ENTRYPOINT
DEFINE_REGS_ENTRYPOINT( RtlUnwind, 16, 16 )
#else
void WINAPI RtlUnwind( PVOID pEndFrame, PVOID unusedEip,
                       PEXCEPTION_RECORD pRecord, PVOID returnEax )
{
    CONTEXT context;
    memset( &context, 0, sizeof(context) );
    __regs_RtlUnwind( pEndFrame, unusedEip, pRecord, returnEax, &context );
}
#endif


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
        VECTORED_HANDLER *curr_handler = LIST_ENTRY( ptr, VECTORED_HANDLER, entry );
        if (curr_handler == handler)
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

    if (wine_frame->u.filter == (void *)1)  /* special hack for page faults */
    {
        if (record->ExceptionCode != STATUS_ACCESS_VIOLATION)
            return ExceptionContinueSearch;
    }
    else if (wine_frame->u.filter)
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


/*************************************************************
 *            __wine_spec_unimplemented_stub
 *
 * ntdll-specific implementation to avoid depending on kernel functions.
 * Can be removed once ntdll.spec no longer contains stubs.
 */
void __wine_spec_unimplemented_stub( const char *module, const char *function )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = EXCEPTION_WINE_STUB;
    record.ExceptionFlags   = EH_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = __wine_spec_unimplemented_stub;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = (ULONG_PTR)module;
    record.ExceptionInformation[1] = (ULONG_PTR)function;
    RtlRaiseException( &record );
}
