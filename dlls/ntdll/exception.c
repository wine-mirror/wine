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

typedef struct
{
    struct list                 entry;
    PVECTORED_EXCEPTION_HANDLER func;
} VECTORED_HANDLER;

static struct list vectored_handlers = LIST_INIT(vectored_handlers);

static RTL_RWLOCK vectored_handlers_lock;

/**********************************************************************
 *           exceptions_init
 *
 * Initialize read/write lock used by the vectored exception handling.
 */
void exceptions_init(void)
{
    RtlInitializeResource(&vectored_handlers_lock);
}

/**********************************************************************
 *           wait_suspend
 *
 * Wait until the thread is no longer suspended.
 */
void wait_suspend( CONTEXT *context )
{
    LARGE_INTEGER timeout;
    int saved_errno = errno;
    context_t server_context;

    context_to_server( &server_context, context );

    /* store the context we got at suspend time */
    SERVER_START_REQ( set_thread_context )
    {
        req->handle  = wine_server_obj_handle( GetCurrentThread() );
        req->suspend = 1;
        wine_server_add_data( req, &server_context, sizeof(server_context) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* wait with 0 timeout, will only return once the thread is no longer suspended */
    timeout.QuadPart = 0;
    NTDLL_wait_for_multiple_objects( 0, NULL, SELECT_INTERRUPTIBLE, &timeout, 0 );

    /* retrieve the new context */
    SERVER_START_REQ( get_thread_context )
    {
        req->handle  = wine_server_obj_handle( GetCurrentThread() );
        req->suspend = 1;
        wine_server_set_reply( req, &server_context, sizeof(server_context) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    context_from_server( context, &server_context );
    errno = saved_errno;
}


/**********************************************************************
 *           send_debug_event
 *
 * Send an EXCEPTION_DEBUG_EVENT event to the debugger.
 */
NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, int first_chance, CONTEXT *context )
{
    NTSTATUS ret;
    DWORD i;
    HANDLE handle = 0;
    client_ptr_t params[EXCEPTION_MAXIMUM_PARAMETERS];
    context_t server_context;

    if (!NtCurrentTeb()->Peb->BeingDebugged) return 0;  /* no debugger present */

    for (i = 0; i < min( rec->NumberParameters, EXCEPTION_MAXIMUM_PARAMETERS ); i++)
        params[i] = rec->ExceptionInformation[i];

    context_to_server( &server_context, context );

    SERVER_START_REQ( queue_exception_event )
    {
        req->first   = first_chance;
        req->code    = rec->ExceptionCode;
        req->flags   = rec->ExceptionFlags;
        req->record  = wine_server_client_ptr( rec->ExceptionRecord );
        req->address = wine_server_client_ptr( rec->ExceptionAddress );
        req->len     = i * sizeof(params[0]);
        wine_server_add_data( req, params, req->len );
        wine_server_add_data( req, &server_context, sizeof(server_context) );
        if (!wine_server_call( req )) handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    if (!handle) return 0;

    NTDLL_wait_for_multiple_objects( 1, &handle, SELECT_INTERRUPTIBLE, NULL, 0 );

    SERVER_START_REQ( get_exception_status )
    {
        req->handle = wine_server_obj_handle( handle );
        wine_server_set_reply( req, &server_context, sizeof(server_context) );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (ret >= 0) context_from_server( context, &server_context );
    return ret;
}


/**********************************************************************
 *           call_vectored_handlers
 *
 * Call the vectored handlers chain.
 */
LONG call_vectored_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct list *ptr;
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    EXCEPTION_POINTERS except_ptrs;

    except_ptrs.ExceptionRecord = rec;
    except_ptrs.ContextRecord = context;

    RtlAcquireResourceShared( &vectored_handlers_lock, TRUE );
    LIST_FOR_EACH( ptr, &vectored_handlers )
    {
        VECTORED_HANDLER *handler = LIST_ENTRY( ptr, VECTORED_HANDLER, entry );
        TRACE( "calling handler at %p code=%x flags=%x\n",
               handler->func, rec->ExceptionCode, rec->ExceptionFlags );
        ret = handler->func( &except_ptrs );
        TRACE( "handler at %p returned %x\n", handler->func, ret );
        if (ret == EXCEPTION_CONTINUE_EXECUTION) break;
    }
    RtlReleaseResource( &vectored_handlers_lock );
    return ret;
}


/*******************************************************************
 *		raise_status
 *
 * Implementation of RtlRaiseStatus with a specific exception record.
 */
void raise_status( NTSTATUS status, EXCEPTION_RECORD *rec )
{
    EXCEPTION_RECORD ExceptionRec;

    ExceptionRec.ExceptionCode    = status;
    ExceptionRec.ExceptionFlags   = EH_NONCONTINUABLE;
    ExceptionRec.ExceptionRecord  = rec;
    ExceptionRec.NumberParameters = 0;
    for (;;) RtlRaiseException( &ExceptionRec );  /* never returns */
}


/***********************************************************************
 *            RtlRaiseStatus  (NTDLL.@)
 *
 * Raise an exception with ExceptionCode = status
 */
void WINAPI RtlRaiseStatus( NTSTATUS status )
{
    raise_status( status, NULL );
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
        RtlAcquireResourceExclusive( &vectored_handlers_lock, TRUE );
        if (first) list_add_head( &vectored_handlers, &handler->entry );
        else list_add_tail( &vectored_handlers, &handler->entry );
        RtlReleaseResource( &vectored_handlers_lock );
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

    RtlAcquireResourceExclusive( &vectored_handlers_lock, TRUE );
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
    RtlReleaseResource( &vectored_handlers_lock );
    if (ret) RtlFreeHeap( GetProcessHeap(), 0, handler );
    return ret;
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
    for (;;) RtlRaiseException( &record );
}
