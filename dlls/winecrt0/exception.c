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

#include <stdarg.h>
#include "excpt.h"
#undef USE_COMPILER_EXCEPTIONS
#undef GetExceptionInformation
#undef GetExceptionCode
#undef AbnormalTermination
#include "winternl.h"
#include "wine/exception.h"
#include "wine/asm.h"

#if defined(__GNUC__) || defined(__clang__)

#if defined(__i386__)

__ASM_GLOBAL_FUNC( __wine_rtl_unwind,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "subl $8,%esp\n\t"
                   "pushl $0\n\t"       /* retval */
                   "pushl 12(%ebp)\n\t" /* record */
                   "pushl 16(%ebp)\n\t" /* target */
                   "pushl 8(%ebp)\n\t"  /* frame */
                   "call " __ASM_STDCALL("RtlUnwind",16) "\n\t"
                   "call *16(%ebp)" )

#elif defined(__x86_64__) && !defined(__arm64ec__)

__ASM_GLOBAL_FUNC( __wine_rtl_unwind,
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "subq $0x20,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x20\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "movq %r8,%r9\n\t"  /* retval = final target */
                   "movq %rdx,%r8\n\t" /* record */
                   "leaq __wine_unwind_trampoline(%rip),%rdx\n\t"  /* target = trampoline */
                   "call " __ASM_NAME("RtlUnwind") "\n"
                   "__wine_unwind_trampoline:\n\t"
                   /* we need an extra call to make sure the stack is correctly aligned */
                   "callq *%rax" )

#else

void __cdecl __wine_rtl_unwind( EXCEPTION_REGISTRATION_RECORD* frame, EXCEPTION_RECORD *record,
                                void (*target)(void) )
{
    RtlUnwind( frame, target, record, 0 );
    for (;;) target();
}

#endif

static void DECLSPEC_NORETURN unwind_target(void)
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)__wine_get_frame();
    __wine_pop_frame( &wine_frame->frame );
    for (;;) __wine_longjmp( &wine_frame->jmp, 1 );
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

DWORD __cdecl __wine_exception_handler( EXCEPTION_RECORD *record,
                                        EXCEPTION_REGISTRATION_RECORD *frame,
                                        CONTEXT *context,
                                        EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
    EXCEPTION_POINTERS ptrs;

    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | EXCEPTION_NESTED_CALL))
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

DWORD __cdecl __wine_exception_ctx_handler( EXCEPTION_RECORD *record,
                                            EXCEPTION_REGISTRATION_RECORD *frame,
                                            CONTEXT *context,
                                            EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
    EXCEPTION_POINTERS ptrs;

    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | EXCEPTION_NESTED_CALL))
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

DWORD __cdecl __wine_exception_handler_page_fault( EXCEPTION_RECORD *record,
                                                   EXCEPTION_REGISTRATION_RECORD *frame,
                                                   CONTEXT *context,
                                                   EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | EXCEPTION_NESTED_CALL))
        return ExceptionContinueSearch;
    if (record->ExceptionCode != STATUS_ACCESS_VIOLATION)
        return ExceptionContinueSearch;
    unwind_frame( record, frame );
}

DWORD __cdecl __wine_exception_handler_all( EXCEPTION_RECORD *record,
                                            EXCEPTION_REGISTRATION_RECORD *frame,
                                            CONTEXT *context,
                                            EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | EXCEPTION_NESTED_CALL))
        return ExceptionContinueSearch;
    unwind_frame( record, frame );
}

DWORD __cdecl __wine_finally_handler( EXCEPTION_RECORD *record,
                                      EXCEPTION_REGISTRATION_RECORD *frame,
                                      CONTEXT *context,
                                      EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
    {
        __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
        wine_frame->u.finally_func( FALSE );
    }
    return ExceptionContinueSearch;
}

DWORD __cdecl __wine_finally_ctx_handler( EXCEPTION_RECORD *record,
                                          EXCEPTION_REGISTRATION_RECORD *frame,
                                          CONTEXT *context,
                                          EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
    {
        __WINE_FRAME *wine_frame = (__WINE_FRAME *)frame;
        wine_frame->u.finally_func_ctx( FALSE, wine_frame->ctx );
    }
    return ExceptionContinueSearch;
}

#endif /* __GNUC__ || __clang__ */
