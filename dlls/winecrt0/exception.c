/*
 * Support functions for Wine exception handling
 *
 * Copyright (c) 1999, 2010 Alexandre Julliard
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
#include <stdarg.h>
#include "winternl.h"
#include "wine/exception.h"

#if defined(__x86_64__) && defined(__ASM_GLOBAL_FUNC)
extern void __wine_unwind_trampoline(void);
/* we need an extra call to make sure the stack is correctly aligned */
__ASM_GLOBAL_FUNC( __wine_unwind_trampoline, "callq *%rax" );
#endif

/* wrapper for RtlUnwind since it clobbers registers on Windows */
void __wine_rtl_unwind( EXCEPTION_REGISTRATION_RECORD* frame, EXCEPTION_RECORD *record,
                        void (*target)(void) )
{
#if defined(__GNUC__) && defined(__i386__)
    int dummy1, dummy2, dummy3, dummy4;
    __asm__ __volatile__("pushl %%ebp\n\t"
                         "pushl %%ebx\n\t"
                         "pushl $0\n\t"
                         "pushl %3\n\t"
                         "pushl %2\n\t"
                         "pushl %1\n\t"
                         "call *%0\n\t"
                         "popl %%ebx\n\t"
                         "popl %%ebp"
                         : "=a" (dummy1), "=S" (dummy2), "=D" (dummy3), "=c" (dummy4)
                         : "0" (RtlUnwind), "1" (frame), "2" (target), "3" (record)
                         : "edx", "memory" );
#elif defined(__x86_64__) && defined(__ASM_GLOBAL_FUNC)
    RtlUnwind( frame, __wine_unwind_trampoline, record, target );
#else
    RtlUnwind( frame, target, record, 0 );
#endif
    for (;;) target();
}

static void DECLSPEC_NORETURN unwind_target(void)
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)__wine_get_frame();
    __wine_pop_frame( &wine_frame->frame );
    siglongjmp( wine_frame->jmp, 1 );
}

static void DECLSPEC_NORETURN unwind_frame( EXCEPTION_RECORD *record,
                                            EXCEPTION_REGISTRATION_RECORD *frame )
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;

    /* hack to make GetExceptionCode() work in handler */
    wine_frame->ExceptionCode   = record->ExceptionCode;
    wine_frame->ExceptionRecord = wine_frame;

    __wine_rtl_unwind( frame, record, unwind_target );
}

DWORD __wine_exception_handler( EXCEPTION_RECORD *record,
                                EXCEPTION_REGISTRATION_RECORD *frame,
                                CONTEXT *context,
                                EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
    EXCEPTION_POINTERS ptrs;

    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND | EH_NESTED_CALL))
        return ExceptionContinueSearch;

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
    }
    unwind_frame( record, frame );
}

DWORD __wine_exception_ctx_handler( EXCEPTION_RECORD *record,
                                    EXCEPTION_REGISTRATION_RECORD *frame,
                                    CONTEXT *context,
                                    EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
    EXCEPTION_POINTERS ptrs;

    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND | EH_NESTED_CALL))
        return ExceptionContinueSearch;

    ptrs.ExceptionRecord = record;
    ptrs.ContextRecord = context;
    switch(wine_frame->u.filter_ctx( &ptrs, wine_frame->ctx ))
    {
    case EXCEPTION_CONTINUE_SEARCH:
        return ExceptionContinueSearch;
    case EXCEPTION_CONTINUE_EXECUTION:
        return ExceptionContinueExecution;
    case EXCEPTION_EXECUTE_HANDLER:
        break;
    }
    unwind_frame( record, frame );
}

DWORD __wine_exception_handler_page_fault( EXCEPTION_RECORD *record,
                                           EXCEPTION_REGISTRATION_RECORD *frame,
                                           CONTEXT *context,
                                           EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND | EH_NESTED_CALL))
        return ExceptionContinueSearch;
    if (record->ExceptionCode != STATUS_ACCESS_VIOLATION)
        return ExceptionContinueSearch;
    unwind_frame( record, frame );
}

DWORD __wine_exception_handler_all( EXCEPTION_RECORD *record,
                                    EXCEPTION_REGISTRATION_RECORD *frame,
                                    CONTEXT *context,
                                    EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND | EH_NESTED_CALL))
        return ExceptionContinueSearch;
    unwind_frame( record, frame );
}

DWORD __wine_finally_handler( EXCEPTION_RECORD *record,
                              EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context,
                              EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
        wine_frame->u.finally_func( FALSE );
    }
    return ExceptionContinueSearch;
}

DWORD __wine_finally_ctx_handler( EXCEPTION_RECORD *record,
                                  EXCEPTION_REGISTRATION_RECORD *frame,
                                  CONTEXT *context,
                                  EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
        wine_frame->u.finally_func_ctx( FALSE, wine_frame->ctx );
    }
    return ExceptionContinueSearch;
}
