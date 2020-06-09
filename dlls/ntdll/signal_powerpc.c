/*
 * PowerPC signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
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

#ifdef __powerpc__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
void WINAPI RtlCaptureContext( CONTEXT *context )
{
    FIXME("not implemented\n");
    memset( context, 0, sizeof(*context) );
}


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_POINTERS ptrs;

    FIXME( "not implemented on PowerPC\n" );

    /* hack: call unhandled exception filter directly */
    ptrs.ExceptionRecord = rec;
    ptrs.ContextRecord = context;
    call_unhandled_exception_filter( &ptrs );
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
NTSTATUS WINAPI KiUserExceptionDispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;
    DWORD c;

    TRACE( "code=%x flags=%x addr=%p ip=%x tid=%04x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           context->Iar, GetCurrentThreadId() );
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
    else
    {
        /* FIXME: dump context */
    }

    if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
        NtContinue( context, FALSE );

    if ((status = call_stack_handlers( rec, context )) == STATUS_SUCCESS)
        NtContinue( context, FALSE );

    if (status != STATUS_UNHANDLED_EXCEPTION) RtlRaiseStatus( status );
    return NtRaiseException( rec, context, FALSE );
}


/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( PVOID pEndFrame, PVOID targetIp, PEXCEPTION_RECORD pRecord, PVOID retval )
{
    FIXME( "Not implemented on PowerPC\n" );
}

/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void WINAPI RtlRaiseException( EXCEPTION_RECORD *rec )
{
    CONTEXT context;

    RtlCaptureContext( &context );
    rec->ExceptionAddress = (void *)context.Iar;
    RtlRaiseStatus( NtRaiseException( rec, &context, TRUE ));
}

/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "(%d, %d, %p, %p) stub!\n", skip, count, buffer, hash );
    return 0;
}

/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
void WINAPI DbgBreakPoint(void)
{
     kill(getpid(), SIGTRAP);
}

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
void WINAPI DbgUserBreakPoint(void)
{
     kill(getpid(), SIGTRAP);
}

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return unix_funcs->NtCurrentTeb();
}

#endif  /* __powerpc__ */
