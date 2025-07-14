/*
 * x86-64 signal handling routines
 *
 * Copyright 1999, 2005 Alexandre Julliard
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

#if 0
#pragma makedep arm64ec_x64
#endif

#ifdef __x86_64__

#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "ntsyscalls.h"


/*******************************************************************
 *         syscalls
 */
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name )
ALL_SYSCALLS
#undef SYSCALL_ENTRY


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
__ASM_GLOBAL_FUNC( __chkstk, "ret" );


#ifdef __arm64ec_x64__

/**************************************************************************
 *		invoke_arm64ec_syscall
 *
 * Helper to call a syscall entry point from x64 code.
 */
__ASM_GLOBAL_FUNC( invoke_arm64ec_syscall,
                   "movq %r10,8(%rsp)\n\t"  /* ret address to syscall thunk */
                   "popq %r10\n\t"          /* remove ret address to thunk caller */
                   "movq %r10,8(%rsp)\n\t"  /* and save it */
                   "leaq arm64ec_syscalls(%rip),%r10\n\t"
                   "callq *(%r10,%rax,8)\n\t"
                   "movq (%rsp),%r10\n\t"
                   "pushq 8(%rsp)\n\t"      /* restore ret to caller */
                   "pushq %r10\n\t"         /* and return to syscall thunk */
                   "ret" )

/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( "EXP+#KiUserExceptionDispatcher",
                   "cld\n\t"
                   "mov pWow64PrepareForException(%rip),%rax\n\t"
                   "test %rax,%rax\n\t"
                   "jz 1f\n\t"
                   "subq $0x28,%rsp\n\t"
                   "leaq 0x30+0x3b0+0x4d0(%rsp),%rcx\n\t" /* rec */
                   "leaq 0x30(%rsp),%rdx\n\t"             /* context */
                   "call *%rax\n"
                   "addq $0x28,%rsp\n"
                   "1:\tret" )

#else  /* __arm64ec_x64__ */

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);


/***********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    LDR_DATA_TABLE_ENTRY *module;
    NTSTATUS status;

    dispatch->ImageBase = 0;
    dispatch->ScopeIndex = 0;
    dispatch->ControlPc = context->Rip;
    dispatch->FunctionEntry = RtlLookupFunctionEntry( context->Rip, &dispatch->ImageBase,
                                                      dispatch->HistoryTable );

    /* look for host system exception information */
    if (!dispatch->FunctionEntry &&
        (LdrFindEntryForAddress( (void *)context->Rip, &module ) || (module->Flags & LDR_WINE_INTERNAL)))
    {
        struct unwind_builtin_dll_params params = { type, dispatch, context };

        status = WINE_UNIX_CALL( unix_unwind_builtin_dll, &params );
        if (!status && dispatch->LanguageHandler && !module)
        {
            FIXME( "calling personality routine in system library not supported yet\n" );
            dispatch->LanguageHandler = NULL;
        }
        if (status != STATUS_UNSUCCESSFUL) return status;
    }

    return RtlVirtualUnwind2( type, dispatch->ImageBase, context->Rip, dispatch->FunctionEntry,
                              context, NULL, &dispatch->HandlerData, &dispatch->EstablisherFrame,
                              NULL, NULL, NULL, &dispatch->LanguageHandler, 0 );
}


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                   "pushfq\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "movl $0x10000f,0x30(%rcx)\n\t"  /* context->ContextFlags */
                   "stmxcsr 0x34(%rcx)\n\t"         /* context->MxCsr */
                   "movw %cs,0x38(%rcx)\n\t"        /* context->SegCs */
                   "movw %ds,0x3a(%rcx)\n\t"        /* context->SegDs */
                   "movw %es,0x3c(%rcx)\n\t"        /* context->SegEs */
                   "movw %fs,0x3e(%rcx)\n\t"        /* context->SegFs */
                   "movw %gs,0x40(%rcx)\n\t"        /* context->SegGs */
                   "movw %ss,0x42(%rcx)\n\t"        /* context->SegSs */
                   "popq 0x44(%rcx)\n\t"            /* context->Eflags */
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   "movq %rax,0x78(%rcx)\n\t"       /* context->Rax */
                   "movq %rcx,0x80(%rcx)\n\t"       /* context->Rcx */
                   "movq %rdx,0x88(%rcx)\n\t"       /* context->Rdx */
                   "movq %rbx,0x90(%rcx)\n\t"       /* context->Rbx */
                   "leaq 8(%rsp),%rax\n\t"
                   "movq %rax,0x98(%rcx)\n\t"       /* context->Rsp */
                   "movq %rbp,0xa0(%rcx)\n\t"       /* context->Rbp */
                   "movq %rsi,0xa8(%rcx)\n\t"       /* context->Rsi */
                   "movq %rdi,0xb0(%rcx)\n\t"       /* context->Rdi */
                   "movq %r8,0xb8(%rcx)\n\t"        /* context->R8 */
                   "movq %r9,0xc0(%rcx)\n\t"        /* context->R9 */
                   "movq %r10,0xc8(%rcx)\n\t"       /* context->R10 */
                   "movq %r11,0xd0(%rcx)\n\t"       /* context->R11 */
                   "movq %r12,0xd8(%rcx)\n\t"       /* context->R12 */
                   "movq %r13,0xe0(%rcx)\n\t"       /* context->R13 */
                   "movq %r14,0xe8(%rcx)\n\t"       /* context->R14 */
                   "movq %r15,0xf0(%rcx)\n\t"       /* context->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0xf8(%rcx)\n\t"       /* context->Rip */
                   "fxsave 0x100(%rcx)\n\t"         /* context->FltSave */
                   "ret" );


/***********************************************************************
 *		call_seh_handler
 */
#ifdef __WINE_PE_BUILD
DWORD WINAPI call_seh_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                               CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler );
__ASM_GLOBAL_FUNC( call_seh_handler,
                   "subq $0x28, %rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler nested_exception_handler, @except\n\t"
                   "callq *0x50(%rsp)\n\t"      /* handler */
                   "nop\n\t"                    /* avoid epilogue so handler is called */
                   "addq $0x28, %rsp\n\t"
                   "ret" )
#else
static DWORD call_seh_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                               CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler )
{
    EXCEPTION_REGISTRATION_RECORD wrapper_frame;
    DWORD res;

    wrapper_frame.Handler = (PEXCEPTION_HANDLER)nested_exception_handler;
    __wine_push_frame( &wrapper_frame );
    res = handler( rec, (void *)frame, context, dispatch );
    __wine_pop_frame( &wrapper_frame );
    return res;
}
#endif


/**********************************************************************
 *           call_seh_handlers
 *
 * Call the SEH handlers chain.
 */
NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    NTSTATUS status;
    ULONG_PTR frame;
    DWORD res;

    context = *orig_context;
    context.ContextFlags &= ~0x40; /* Clear xstate flag. */

    dispatch.TargetIp      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return status;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %p (%p-%p)\n", (void *)dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            TRACE( "calling handler %p (rec=%p, frame=%I64x context=%p, dispatch=%p)\n",
                   dispatch.LanguageHandler, rec, dispatch.EstablisherFrame, orig_context, &dispatch );
            res = call_seh_handler( rec, dispatch.EstablisherFrame, orig_context,
                                    &dispatch, dispatch.LanguageHandler );
            rec->ExceptionFlags &= EXCEPTION_NONCONTINUABLE;
            TRACE( "handler at %p returned %lu\n", dispatch.LanguageHandler, res );

            switch (res)
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind:
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase, dispatch.ControlPc,
                                  dispatch.FunctionEntry, &context, NULL, &frame, NULL );
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context.Rsp)
        {
            TRACE( "calling TEB handler %p (rec=%p frame=%p context=%p dispatch=%p) sp=%I64x\n",
                   teb_frame->Handler, rec, teb_frame, orig_context, &dispatch, context.Rsp );
            res = call_seh_handler( rec, (ULONG_PTR)teb_frame, orig_context,
                                    &dispatch, (PEXCEPTION_ROUTINE)teb_frame->Handler );
            TRACE( "TEB handler at %p returned %lu\n", teb_frame->Handler, res );

            switch (res)
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind:
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase, dispatch.ControlPc,
                                  dispatch.FunctionEntry, &context, NULL, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Rsp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserExceptionDispatcher,
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_stackalloc 0x590\n\t")
                   __ASM_SEH(".seh_savereg %rbx,0x90\n\t")
                   __ASM_SEH(".seh_savereg %rbp,0xa0\n\t")
                   __ASM_SEH(".seh_savereg %rsi,0xa8\n\t")
                   __ASM_SEH(".seh_savereg %rdi,0xb0\n\t")
                   __ASM_SEH(".seh_savereg %r12,0xd8\n\t")
                   __ASM_SEH(".seh_savereg %r13,0xe0\n\t")
                   __ASM_SEH(".seh_savereg %r14,0xe8\n\t")
                   __ASM_SEH(".seh_savereg %r15,0xf0\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa_offset 0\n\t")
                   __ASM_CFI(".cfi_offset %rbx,0x90\n\t")
                   __ASM_CFI(".cfi_offset %rbp,0xa0\n\t")
                   __ASM_CFI(".cfi_offset %rsi,0xa8\n\t")
                   __ASM_CFI(".cfi_offset %rdi,0xb0\n\t")
                   __ASM_CFI(".cfi_offset %r12,0xd8\n\t")
                   __ASM_CFI(".cfi_offset %r13,0xe0\n\t")
                   __ASM_CFI(".cfi_offset %r14,0xe8\n\t")
                   __ASM_CFI(".cfi_offset %r15,0xf0\n\t")
                   __ASM_CFI(".cfi_offset %rip,0x590\n\t")
                   __ASM_CFI(".cfi_offset %rsp,0x5a8\n\t")
                   "cld\n\t"
                   /* some anticheats need this exact instruction here */
                   "mov " __ASM_NAME("pWow64PrepareForException") "(%rip),%rax\n\t"
                   "test %rax,%rax\n\t"
                   "jz 1f\n\t"
                   "mov %rsp,%rdx\n\t"           /* context */
                   "lea 0x4f0(%rsp),%rcx\n\t"    /* rec */
                   "call *%rax\n"
                   "1:\tmov %rsp,%rdx\n\t" /* context */
                   "lea 0x4f0(%rsp),%rcx\n\t" /* rec */
                   "call " __ASM_NAME("dispatch_exception") "\n\t"
                   "int3" )


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserApcDispatcher,
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_stackalloc 0x530\n\t")  /* machine_frame offset */
                   __ASM_SEH(".seh_savereg %rbx,0x90\n\t")
                   __ASM_SEH(".seh_savereg %rbp,0xa0\n\t")
                   __ASM_SEH(".seh_savereg %rsi,0xa8\n\t")
                   __ASM_SEH(".seh_savereg %rdi,0xb0\n\t")
                   __ASM_SEH(".seh_savereg %r12,0xd8\n\t")
                   __ASM_SEH(".seh_savereg %r13,0xe0\n\t")
                   __ASM_SEH(".seh_savereg %r14,0xe8\n\t")
                   __ASM_SEH(".seh_savereg %r15,0xf0\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa_offset 0\n\t")
                   __ASM_CFI(".cfi_offset %rbx,0x90\n\t")
                   __ASM_CFI(".cfi_offset %rbp,0xa0\n\t")
                   __ASM_CFI(".cfi_offset %rsi,0xa8\n\t")
                   __ASM_CFI(".cfi_offset %rdi,0xb0\n\t")
                   __ASM_CFI(".cfi_offset %r12,0xd8\n\t")
                   __ASM_CFI(".cfi_offset %r13,0xe0\n\t")
                   __ASM_CFI(".cfi_offset %r14,0xe8\n\t")
                   __ASM_CFI(".cfi_offset %r15,0xf0\n\t")
                   __ASM_CFI(".cfi_offset %rip,0x530\n\t")
                   __ASM_CFI(".cfi_offset %rsp,0x548\n\t")
                   "movq 0x00(%rsp),%rcx\n\t"  /* context->P1Home = arg1 */
                   "movq 0x08(%rsp),%rdx\n\t"  /* context->P2Home = arg2 */
                   "movq 0x10(%rsp),%r8\n\t"   /* context->P3Home = arg3 */
                   "movq 0x18(%rsp),%rax\n\t"  /* context->P4Home = func */
                   "movq %rsp,%r9\n\t"         /* context */
                   "callq *%rax\n\t"
                   "movq %rsp,%rcx\n\t"        /* context */
                   "leaq 0x4f0(%rsp),%rdx\n\t" /* continue_arg */
                   "call " __ASM_NAME("NtContinueEx") "\n\t"
                   "int3" )


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher,
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_stackalloc 0x30\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa_offset 0\n\t")
                   __ASM_CFI(".cfi_offset %rip,0x30\n\t")
                   __ASM_CFI(".cfi_offset %rsp,0x48\n\t")
                   "movq 0x20(%rsp),%rcx\n\t"  /* args */
                   "movl 0x28(%rsp),%edx\n\t"  /* len */
                   "movl 0x2c(%rsp),%r8d\n\t"  /* id */
#ifdef __WINE_PE_BUILD
                   "movq %gs:0x60,%rax\n\t"     /* peb */
                   "movq 0x58(%rax),%rax\n\t"   /* peb->KernelCallbackTable */
                   "call *(%rax,%r8,8)\n\t"     /* KernelCallbackTable[id] */
                   ".seh_handler " __ASM_NAME("user_callback_handler") ", @except\n\t"
                   ".globl " __ASM_NAME("KiUserCallbackDispatcherReturn") "\n"
                   __ASM_NAME("KiUserCallbackDispatcherReturn") ":\n\t"
#else
                   "call " __ASM_NAME("dispatch_user_callback") "\n\t"
#endif
                   "xorq %rcx,%rcx\n\t"         /* ret_ptr */
                   "xorl %edx,%edx\n\t"         /* ret_len */
                   "movl %eax,%r8d\n\t"         /* status */
                   "call " __ASM_NAME("NtCallbackReturn") "\n\t"
                   "movl %eax,%ecx\n\t"         /* status */
                   "call " __ASM_NAME("RtlRaiseStatus") "\n\t"
                   "int3" )


/**************************************************************************
 *              RtlIsEcCode (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsEcCode( ULONG_PTR ptr )
{
    return FALSE;
}


struct unwind_exception_frame
{
    EXCEPTION_REGISTRATION_RECORD frame;
    char dummy[0x10]; /* Layout 'dispatch' accessed from unwind_exception_handler() so it is above register
                       * save space when .seh handler is used. */
    DISPATCHER_CONTEXT *dispatch;
};
C_ASSERT( sizeof(struct unwind_exception_frame) == 0x28 );
C_ASSERT( offsetof(struct unwind_exception_frame, dispatch) == 0x20 );


/**********************************************************************
 *           unwind_exception_handler
 *
 * Handler for exceptions happening while calling an unwind handler.
 */
DWORD __cdecl unwind_exception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    struct unwind_exception_frame *unwind_frame = (struct unwind_exception_frame *)frame;
    DISPATCHER_CONTEXT *dispatch = (DISPATCHER_CONTEXT *)dispatcher;

    /* copy the original dispatcher into the current one, except for the TargetIp */
    dispatch->ControlPc        = unwind_frame->dispatch->ControlPc;
    dispatch->ImageBase        = unwind_frame->dispatch->ImageBase;
    dispatch->FunctionEntry    = unwind_frame->dispatch->FunctionEntry;
    dispatch->EstablisherFrame = unwind_frame->dispatch->EstablisherFrame;
    dispatch->LanguageHandler  = unwind_frame->dispatch->LanguageHandler;
    dispatch->HandlerData      = unwind_frame->dispatch->HandlerData;
    dispatch->HistoryTable     = unwind_frame->dispatch->HistoryTable;
    dispatch->ScopeIndex       = unwind_frame->dispatch->ScopeIndex;
    *dispatch->ContextRecord   = *unwind_frame->dispatch->ContextRecord;
    dispatch->ContextRecord->ContextFlags &= ~0x40;
    TRACE( "detected collided unwind\n" );
    return ExceptionCollidedUnwind;
}


/**********************************************************************
 *           call_unwind_handler
 */
#ifdef __WINE_PE_BUILD
DWORD WINAPI call_unwind_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                                  CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler );
__ASM_GLOBAL_FUNC( call_unwind_handler,
                   "subq $0x28,%rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler unwind_exception_handler, @except, @unwind\n\t"
                   "movq %r9,0x20(%rsp)\n\t"   /* unwind_exception_frame->dispatch */
                   "callq *0x50(%rsp)\n\t"     /* handler */
                   "nop\n\t"                   /* avoid epilogue so handler is called */
                   "addq $0x28, %rsp\n\t"
                   "ret" )
#else
static DWORD call_unwind_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                                  CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler )
{
    struct unwind_exception_frame wrapper_frame;
    DWORD res;

    wrapper_frame.frame.Handler = unwind_exception_handler;
    wrapper_frame.dispatch = dispatch;
    __wine_push_frame( &wrapper_frame.frame );
    res = handler( rec, (void *)frame, context, dispatch );
    __wine_pop_frame( &wrapper_frame.frame );
    return res;
}
#endif


/**********************************************************************
 *           call_consolidate_callback
 *
 * Wrapper function to call a consolidate callback from a fake frame.
 * If the callback executes RtlUnwindEx (like for example done in C++ handlers),
 * we have to skip all frames which were already processed. To do that we
 * trick the unwinding functions into thinking the call came from the specified
 * context. All CFI instructions are either DW_CFA_def_cfa_expression or
 * DW_CFA_expression, and the expressions have the following format:
 *
 * DW_OP_breg6; sleb128 0x10            | Load %rbp + 0x10
 * DW_OP_deref                          | Get *(%rbp + 0x10) == context
 * DW_OP_plus_uconst; uleb128 <OFFSET>  | Add offset to get struct member
 * [DW_OP_deref]                        | Dereference, only for CFA
 */
extern void * WINAPI call_consolidate_callback( CONTEXT *context,
                                                void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                EXCEPTION_RECORD *rec );
__ASM_GLOBAL_FUNC( call_consolidate_callback,
                   "pushq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")

                   /* Setup SEH machine frame. */
                   "subq $0x28,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x28\n\t")
                   "movq 0xf8(%rcx),%rax\n\t" /* Context->Rip */
                   "movq %rax,(%rsp)\n\t"
                   "movq 0x98(%rcx),%rax\n\t" /* context->Rsp */
                   "movq %rax,0x18(%rsp)\n\t"
                   __ASM_SEH(".seh_pushframe\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")

                   "subq $0x108,%rsp\n\t" /* 10*16 (float regs) + 8*8 (int regs) + 32 (shadow store) + 8 (align). */
                   __ASM_SEH(".seh_stackalloc 0x108\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x108\n\t")

                   /* Setup CFI unwind to context. */
                   "movq %rcx,0x10(%rbp)\n\t"
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x07,0x76,0x10,0x06,0x23,0x98,0x01,0x06\n\t") /* CFA    */
                   __ASM_CFI(".cfi_escape 0x10,0x03,0x06,0x76,0x10,0x06,0x23,0x90,0x01\n\t") /* %rbx   */
                   __ASM_CFI(".cfi_escape 0x10,0x04,0x06,0x76,0x10,0x06,0x23,0xa8,0x01\n\t") /* %rsi   */
                   __ASM_CFI(".cfi_escape 0x10,0x05,0x06,0x76,0x10,0x06,0x23,0xb0,0x01\n\t") /* %rdi   */
                   __ASM_CFI(".cfi_escape 0x10,0x06,0x06,0x76,0x10,0x06,0x23,0xa0,0x01\n\t") /* %rbp   */
                   __ASM_CFI(".cfi_escape 0x10,0x0c,0x06,0x76,0x10,0x06,0x23,0xd8,0x01\n\t") /* %r12   */
                   __ASM_CFI(".cfi_escape 0x10,0x0d,0x06,0x76,0x10,0x06,0x23,0xe0,0x01\n\t") /* %r13   */
                   __ASM_CFI(".cfi_escape 0x10,0x0e,0x06,0x76,0x10,0x06,0x23,0xe8,0x01\n\t") /* %r14   */
                   __ASM_CFI(".cfi_escape 0x10,0x0f,0x06,0x76,0x10,0x06,0x23,0xf0,0x01\n\t") /* %r15   */
                   __ASM_CFI(".cfi_escape 0x10,0x10,0x06,0x76,0x10,0x06,0x23,0xf8,0x01\n\t") /* %rip   */
                   __ASM_CFI(".cfi_escape 0x10,0x17,0x06,0x76,0x10,0x06,0x23,0x80,0x04\n\t") /* %xmm6  */
                   __ASM_CFI(".cfi_escape 0x10,0x18,0x06,0x76,0x10,0x06,0x23,0x90,0x04\n\t") /* %xmm7  */
                   __ASM_CFI(".cfi_escape 0x10,0x19,0x06,0x76,0x10,0x06,0x23,0xa0,0x04\n\t") /* %xmm8  */
                   __ASM_CFI(".cfi_escape 0x10,0x1a,0x06,0x76,0x10,0x06,0x23,0xb0,0x04\n\t") /* %xmm9  */
                   __ASM_CFI(".cfi_escape 0x10,0x1b,0x06,0x76,0x10,0x06,0x23,0xc0,0x04\n\t") /* %xmm10 */
                   __ASM_CFI(".cfi_escape 0x10,0x1c,0x06,0x76,0x10,0x06,0x23,0xd0,0x04\n\t") /* %xmm11 */
                   __ASM_CFI(".cfi_escape 0x10,0x1d,0x06,0x76,0x10,0x06,0x23,0xe0,0x04\n\t") /* %xmm12 */
                   __ASM_CFI(".cfi_escape 0x10,0x1e,0x06,0x76,0x10,0x06,0x23,0xf0,0x04\n\t") /* %xmm13 */
                   __ASM_CFI(".cfi_escape 0x10,0x1f,0x06,0x76,0x10,0x06,0x23,0x80,0x05\n\t") /* %xmm14 */
                   __ASM_CFI(".cfi_escape 0x10,0x20,0x06,0x76,0x10,0x06,0x23,0x90,0x05\n\t") /* %xmm15 */

                   /* Setup SEH unwind registers restore. */
                   "movq 0xa0(%rcx),%rax\n\t" /* context->Rbp */
                   "movq %rax,0x100(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rbp, 0x100\n\t")
                   "movq 0x90(%rcx),%rax\n\t" /* context->Rbx */
                   "movq %rax,0x20(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rbx, 0x20\n\t")
                   "movq 0xa8(%rcx),%rax\n\t" /* context->Rsi */
                   "movq %rax,0x28(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rsi, 0x28\n\t")
                   "movq 0xb0(%rcx),%rax\n\t" /* context->Rdi */
                   "movq %rax,0x30(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %rdi, 0x30\n\t")

                   "movq 0xd8(%rcx),%rax\n\t" /* context->R12 */
                   "movq %rax,0x38(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r12, 0x38\n\t")
                   "movq 0xe0(%rcx),%rax\n\t" /* context->R13 */
                   "movq %rax,0x40(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r13, 0x40\n\t")
                   "movq 0xe8(%rcx),%rax\n\t" /* context->R14 */
                   "movq %rax,0x48(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r14, 0x48\n\t")
                   "movq 0xf0(%rcx),%rax\n\t" /* context->R15 */
                   "movq %rax,0x50(%rsp)\n\t"
                   __ASM_SEH(".seh_savereg %r15, 0x50\n\t")
                   "pushq %rsi\n\t"
                   "pushq %rdi\n\t"
                   "leaq 0x200(%rcx),%rsi\n\t"
                   "leaq 0x70(%rsp),%rdi\n\t"
                   "movq $0x14,%rcx\n\t"
                   "cld\n\t"
                   "rep; movsq\n\t"
                   "popq %rdi\n\t"
                   "popq %rsi\n\t"
                   __ASM_SEH(".seh_savexmm %xmm6, 0x60\n\t")
                   __ASM_SEH(".seh_savexmm %xmm7, 0x70\n\t")
                   __ASM_SEH(".seh_savexmm %xmm8, 0x80\n\t")
                   __ASM_SEH(".seh_savexmm %xmm9, 0x90\n\t")
                   __ASM_SEH(".seh_savexmm %xmm10, 0xa0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm11, 0xb0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm12, 0xc0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm13, 0xd0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm14, 0xe0\n\t")
                   __ASM_SEH(".seh_savexmm %xmm15, 0xf0\n\t")

                   /* call the callback. */
                   "movq %r8,%rcx\n\t"
                   "callq *%rdx\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
                   "nop\n\t" /* Otherwise RtlVirtualUnwind() will think we are inside epilogue and
                              * interpret / execute the rest of opcodes here instead of unwind through
                              * machine frame. */
                   "leaq 0(%rbp),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret")

/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct _JUMP_BUFFER *jmp = (struct _JUMP_BUFFER *)rec->ExceptionInformation[0];
        context->Rbx   = jmp->Rbx;
        context->Rsp   = jmp->Rsp;
        context->Rbp   = jmp->Rbp;
        context->Rsi   = jmp->Rsi;
        context->Rdi   = jmp->Rdi;
        context->R12   = jmp->R12;
        context->R13   = jmp->R13;
        context->R14   = jmp->R14;
        context->R15   = jmp->R15;
        context->Rip   = jmp->Rip;
        context->MxCsr = jmp->MxCsr;
        context->FltSave.MxCsr = jmp->MxCsr;
        context->FltSave.ControlWord = jmp->FpCsr;
        memcpy( &context->Xmm6, &jmp->Xmm6, 10 * sizeof(M128A) );
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        context->Rip = (ULONG64)call_consolidate_callback( context, consolidate, rec );
    }

    /* hack: remove no longer accessible TEB frames */
    while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context->Rsp)
    {
        TRACE( "removing TEB frame: %p\n", teb_frame );
        teb_frame = __wine_pop_frame( teb_frame );
    }

    TRACE( "returning to %p stack %p\n", (void *)context->Rip, (void *)context->Rsp );
    NtContinue( context, FALSE );
}


/*******************************************************************
 *		RtlUnwindEx (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT new_context;
    NTSTATUS status;
    ULONG_PTR frame;
    DWORD i, res;

    RtlCaptureContext( context );
    new_context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Rip;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EXCEPTION_UNWINDING | (end_frame ? 0 : EXCEPTION_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx end_frame=%p target_ip=%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%016I64x\n", i, rec->ExceptionInformation[i] );
    TRACE_CONTEXT( context );

    dispatch.EstablisherFrame = context->Rsp;
    dispatch.TargetIp         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &new_context );
        if (status != STATUS_SUCCESS) raise_status( status, rec );

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %p (%p-%p)\n", (void *)dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                ERR( "invalid end frame %p/%p\n", (void *)dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
            TRACE( "calling handler %p (rec=%p, frame=%I64x context=%p, dispatch=%p)\n",
                   dispatch.LanguageHandler, rec, dispatch.EstablisherFrame,
                   dispatch.ContextRecord, &dispatch );
            res = call_unwind_handler( rec, dispatch.EstablisherFrame, dispatch.ContextRecord,
                                       &dispatch, dispatch.LanguageHandler );
            TRACE( "handler %p returned %lx\n", dispatch.LanguageHandler, res );

            switch (res)
            {
            case ExceptionContinueSearch:
                rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                break;
            case ExceptionCollidedUnwind:
                context->ContextFlags &= ~0x40; /* clear xstate flag */
                new_context = *context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &new_context, NULL, &frame, NULL );
                rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                goto unwind_done;
            default:
                raise_status( STATUS_INVALID_DISPOSITION, rec );
                break;
            }
        }
        else  /* hack: call builtin handlers registered in the tib list */
        {
            while (is_valid_frame( (ULONG_PTR)teb_frame ) &&
                   (ULONG64)teb_frame < new_context.Rsp &&
                   (ULONG64)teb_frame < (ULONG64)end_frame)
            {
                TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
                       teb_frame->Handler, rec, teb_frame, dispatch.ContextRecord, &dispatch );
                res = call_unwind_handler( rec, (ULONG_PTR)teb_frame, dispatch.ContextRecord, &dispatch,
                                           (PEXCEPTION_ROUTINE)teb_frame->Handler );
                TRACE( "handler at %p returned %lu\n", teb_frame->Handler, res );
                teb_frame = __wine_pop_frame( teb_frame );

                switch (res)
                {
                case ExceptionContinueSearch:
                    rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                    break;
                case ExceptionCollidedUnwind:
                    context->ContextFlags &= ~0x40; /* clear xstate flag */
                    new_context = *context;
                    RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                      dispatch.ControlPc, dispatch.FunctionEntry,
                                      &new_context, &dispatch.HandlerData,
                                      &frame, NULL );
                    rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                    goto unwind_done;
                default:
                    raise_status( STATUS_INVALID_DISPOSITION, rec );
                    break;
                }
            }
            if ((ULONG64)teb_frame == (ULONG64)end_frame && (ULONG64)end_frame < new_context.Rsp) break;
        }

        if (dispatch.EstablisherFrame == (ULONG64)end_frame) break;
        *context = new_context;
    }

    context->Rax = (ULONG64)retval;
    if (rec->ExceptionCode != STATUS_UNWIND_CONSOLIDATE) context->Rip = (ULONG64)target_ip;
    RtlRestoreContext(context, rec);
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlRaiseException,
                   "sub $0x4f8,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x4f8\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x4f8\n\t")
                   "movq %rcx,0x500(%rsp)\n\t"
                   "leaq 0x20(%rsp),%rcx\n\t"
                   "call " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "leaq 0x20(%rsp),%rdx\n\t"   /* context pointer */
                   "leaq 0x500(%rsp),%rax\n\t"  /* orig stack pointer */
                   "movq %rax,0x98(%rdx)\n\t"   /* context->Rsp */
                   "movq (%rax),%rcx\n\t"       /* original first parameter */
                   "movq %rcx,0x80(%rdx)\n\t"   /* context->Rcx */
                   "movq 0x4f8(%rsp),%rax\n\t"  /* return address */
                   "movq %rax,0xf8(%rdx)\n\t"   /* context->Rip */
                   "movq %rax,0x10(%rcx)\n\t"   /* rec->ExceptionAddress */
                   "movl $1,%r8d\n\t"
                   "movq %gs:0x60,%rax\n\t"     /* Peb */
                   "cmpb $0,0x02(%rax)\n\t"     /* BeingDebugged */
                   "jne 1f\n\t"
                   "call " __ASM_NAME("dispatch_exception") "\n"
                   "1:\tcall " __ASM_NAME("NtRaiseException") "\n\t"
                   "movq %rax,%rcx\n\t"
                   "call " __ASM_NAME("RtlRaiseStatus") /* does not return */ );


/*************************************************************************
 *		RtlGetNativeSystemInformation (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetNativeSystemInformation( SYSTEM_INFORMATION_CLASS class,
                                               void *info, ULONG size, ULONG *ret_size )
{
    return NtQuerySystemInformation( class, info, size, ret_size );
}


/***********************************************************************
 *           RtlIsProcessorFeaturePresent [NTDLL.@]
 */
BOOLEAN WINAPI RtlIsProcessorFeaturePresent( UINT feature )
{
    return feature < PROCESSOR_FEATURE_MAX && user_shared_data->ProcessorFeatures[feature];
}


/*************************************************************************
 *		RtlWalkFrameChain (NTDLL.@)
 */
ULONG WINAPI RtlWalkFrameChain( void **buffer, ULONG count, ULONG flags )
{
    UNWIND_HISTORY_TABLE table;
    RUNTIME_FUNCTION *func;
    PEXCEPTION_ROUTINE handler;
    ULONG_PTR frame, base;
    CONTEXT context;
    void *data;
    ULONG i, skip = flags >> 8, num_entries = 0;

    RtlCaptureContext( &context );

    for (i = 0; i < count; i++)
    {
        func = RtlLookupFunctionEntry( context.Rip, &base, &table );
        if (RtlVirtualUnwind2( UNW_FLAG_NHANDLER, base, context.Rip, func, &context, NULL,
                               &data, &frame, NULL, NULL, NULL, &handler, 0 ))
            break;
        if (!context.Rip) break;
        if (!frame || !is_valid_frame( frame )) break;
        if (context.Rsp == (ULONG_PTR)NtCurrentTeb()->Tib.StackBase) break;
        if (i >= skip) buffer[num_entries++] = (void *)context.Rip;
    }
    return num_entries;
}


/***********************************************************************
 *           _setjmp (NTDLL.@)
 *           _setjmpex (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( NTDLL__setjmpex,
                   __ASM_SEH(".seh_endprologue\n\t")
                   "movq %rdx,(%rcx)\n\t"          /* jmp_buf->Frame */
                   "movq %rbx,0x8(%rcx)\n\t"       /* jmp_buf->Rbx */
                   "leaq 0x8(%rsp),%rax\n\t"
                   "movq %rax,0x10(%rcx)\n\t"      /* jmp_buf->Rsp */
                   "movq %rbp,0x18(%rcx)\n\t"      /* jmp_buf->Rbp */
                   "movq %rsi,0x20(%rcx)\n\t"      /* jmp_buf->Rsi */
                   "movq %rdi,0x28(%rcx)\n\t"      /* jmp_buf->Rdi */
                   "movq %r12,0x30(%rcx)\n\t"      /* jmp_buf->R12 */
                   "movq %r13,0x38(%rcx)\n\t"      /* jmp_buf->R13 */
                   "movq %r14,0x40(%rcx)\n\t"      /* jmp_buf->R14 */
                   "movq %r15,0x48(%rcx)\n\t"      /* jmp_buf->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0x50(%rcx)\n\t"      /* jmp_buf->Rip */
                   "stmxcsr 0x58(%rcx)\n\t"        /* jmp_buf->MxCsr */
                   "fnstcw 0x5c(%rcx)\n\t"         /* jmp_buf->FpCsr */
                   "movdqa %xmm6,0x60(%rcx)\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa %xmm7,0x70(%rcx)\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa %xmm8,0x80(%rcx)\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa %xmm9,0x90(%rcx)\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa %xmm10,0xa0(%rcx)\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa %xmm11,0xb0(%rcx)\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa %xmm12,0xc0(%rcx)\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa %xmm13,0xd0(%rcx)\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa %xmm14,0xe0(%rcx)\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa %xmm15,0xf0(%rcx)\n\t"  /* jmp_buf->Xmm15 */
                   "xorq %rax,%rax\n\t"
                   "retq" )


void __cdecl DECLSPEC_NORETURN longjmp_regs( _JUMP_BUFFER *buf, int retval );
__ASM_GLOBAL_FUNC( longjmp_regs,
                   __ASM_SEH(".seh_endprologue\n\t")
                   "movq %rdx,%rax\n\t"            /* retval */
                   "movq 0x8(%rcx),%rbx\n\t"       /* jmp_buf->Rbx */
                   "movq 0x18(%rcx),%rbp\n\t"      /* jmp_buf->Rbp */
                   "movq 0x20(%rcx),%rsi\n\t"      /* jmp_buf->Rsi */
                   "movq 0x28(%rcx),%rdi\n\t"      /* jmp_buf->Rdi */
                   "movq 0x30(%rcx),%r12\n\t"      /* jmp_buf->R12 */
                   "movq 0x38(%rcx),%r13\n\t"      /* jmp_buf->R13 */
                   "movq 0x40(%rcx),%r14\n\t"      /* jmp_buf->R14 */
                   "movq 0x48(%rcx),%r15\n\t"      /* jmp_buf->R15 */
                   "movq 0x50(%rcx),%rdx\n\t"      /* jmp_buf->Rip */
                   "ldmxcsr 0x58(%rcx)\n\t"        /* jmp_buf->MxCsr */
                   "fnclex\n\t"
                   "fldcw 0x5c(%rcx)\n\t"          /* jmp_buf->FpCsr */
                   "movdqa 0x60(%rcx),%xmm6\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa 0x70(%rcx),%xmm7\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa 0x80(%rcx),%xmm8\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa 0x90(%rcx),%xmm9\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa 0xa0(%rcx),%xmm10\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa 0xb0(%rcx),%xmm11\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa 0xc0(%rcx),%xmm12\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa 0xd0(%rcx),%xmm13\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa 0xe0(%rcx),%xmm14\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa 0xf0(%rcx),%xmm15\n\t"  /* jmp_buf->Xmm15 */
                   "movq 0x10(%rcx),%rsp\n\t"      /* jmp_buf->Rsp */
                   "jmp *%rdx" )

/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
void __cdecl NTDLL_longjmp( _JUMP_BUFFER *buf, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;
    if (!buf->Frame) longjmp_regs( buf, retval );

    rec.ExceptionCode = STATUS_LONGJUMP;
    rec.ExceptionFlags = 0;
    rec.ExceptionRecord = NULL;
    rec.ExceptionAddress = NULL;
    rec.NumberParameters = 1;
    rec.ExceptionInformation[0] = (DWORD_PTR)buf;
    RtlUnwind( (void *)buf->Frame, (void *)buf->Rip, &rec, IntToPtr(retval) );
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
#ifdef __WINE_PE_BUILD
__ASM_GLOBAL_FUNC( RtlUserThreadStart,
                   "subq $0x28,%rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   "movq %rdx,%r8\n\t"
                   "movq %rcx,%rdx\n\t"
                   "xorq %rcx,%rcx\n\t"
                   "movq " __ASM_NAME( "pBaseThreadInitThunk" ) "(%rip),%r9\n\t"
                   "call *%r9\n\t"
                   "int3\n\t"
                   ".seh_handler " __ASM_NAME("call_unhandled_exception_handler") ", @except" )
#else
void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        pBaseThreadInitThunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
}
#endif


/***********************************************************************
 *           signal_start_thread
 */
extern void CDECL DECLSPEC_NORETURN signal_start_thread( CONTEXT *ctx );
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "movq %rcx,%rbx\n\t"        /* context */
                   /* clear the thread stack */
                   "andq $~0xfff,%rcx\n\t"     /* round down to page size */
                   "leaq -0xf000(%rcx),%rdi\n\t"
                   "movq %rdi,%rsp\n\t"
                   "subq %rdi,%rcx\n\t"
                   "xorl %eax,%eax\n\t"
                   "shrq $3,%rcx\n\t"
                   "rep; stosq\n\t"
                   /* switch to the initial context */
                   "leaq -32(%rbx),%rsp\n\t"
                   "movq %rbx,%rcx\n\t"
                   "movl $1,%edx\n\t"
                   "call " __ASM_NAME("NtContinue") )


/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    loader_init( context, (void **)&context->Rcx );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context->Rcx, (void *)context->Rdx );
    signal_start_thread( context );
}


/***********************************************************************
 *           process_breakpoint
 */
#ifdef __WINE_PE_BUILD
__ASM_GLOBAL_FUNC( process_breakpoint,
                   ".seh_endprologue\n\t"
                   ".seh_handler process_breakpoint_handler, @except\n\t"
                   "int $3\n\t"
                   "ret\n"
                   "process_breakpoint_handler:\n\t"
                   "incq 0xf8(%r8)\n\t"  /* context->Rip */
                   "xorl %eax,%eax\n\t"  /* ExceptionContinueExecution */
                   "ret" )
#else
void WINAPI process_breakpoint(void)
{
    __TRY
    {
        __asm__ volatile("int $3");
    }
    __EXCEPT_ALL
    {
        /* do nothing */
    }
    __ENDTRY
}
#endif


/***********************************************************************
 *		DbgUiRemoteBreakin   (NTDLL.@)
 */
#ifdef __WINE_PE_BUILD
__ASM_GLOBAL_FUNC( DbgUiRemoteBreakin,
                   "subq $0x28,%rsp\n\t"
                   ".seh_stackalloc 0x28\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler DbgUiRemoteBreakin_handler, @except\n\t"
                   "mov %gs:0x60,%rax\n\t"
                   "cmpb $0,2(%rax)\n\t"
                   "je 1f\n\t"
                   "call " __ASM_NAME("DbgBreakPoint") "\n"
                   "1:\txorl %ecx,%ecx\n\t"
                   "call " __ASM_NAME("RtlExitUserThread") "\n"
                   "DbgUiRemoteBreakin_handler:\n\t"
                   "movq %rdx,%rsp\n\t"  /* frame */
                   "jmp 1b" )
#else
void WINAPI DbgUiRemoteBreakin( void *arg )
{
    if (NtCurrentTeb()->Peb->BeingDebugged)
    {
        __TRY
        {
            DbgBreakPoint();
        }
        __EXCEPT_ALL
        {
            /* do nothing */
        }
        __ENDTRY
    }
    RtlExitUserThread( STATUS_SUCCESS );
}
#endif

/**********************************************************************
 *		DbgBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "int $3; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "int $3; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

#endif  /* __arm64ec_x64__ */
#endif  /* __x86_64__ */
