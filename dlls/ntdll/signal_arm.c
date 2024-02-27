/*
 * ARM signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
 * Copyright 2010-2013, 2015 André Hentschel
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

#ifdef __arm__

#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "ntsyscalls.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);


/* layering violation: the setjmp buffer is defined in msvcrt, but used by RtlUnwindEx */
struct MSVCRT_JUMP_BUFFER
{
    unsigned long Frame;
    unsigned long R4;
    unsigned long R5;
    unsigned long R6;
    unsigned long R7;
    unsigned long R8;
    unsigned long R9;
    unsigned long R10;
    unsigned long R11;
    unsigned long Sp;
    unsigned long Pc;
    unsigned long Fpscr;
    unsigned long long D[8];
};


static void dump_scope_table( ULONG base, const SCOPE_TABLE *table )
{
    unsigned int i;

    TRACE( "scope table at %p\n", table );
    for (i = 0; i < table->Count; i++)
        TRACE( "  %u: %lx-%lx handler %lx target %lx\n", i,
               base + table->ScopeRecord[i].BeginAddress,
               base + table->ScopeRecord[i].EndAddress,
               base + table->ScopeRecord[i].HandlerAddress,
               base + table->ScopeRecord[i].JumpTarget );
}

/*******************************************************************
 *         syscalls
 */
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name, args )
ALL_SYSCALLS32
DEFINE_SYSCALL_HELPER32()
#undef SYSCALL_ENTRY


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Incoming r4 contains words to allocate, converting to bytes then return
 */
__ASM_GLOBAL_FUNC( __chkstk, "lsl r4, r4, #2\n\t"
                             "bx lr" )

/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                    "str r1, [r0, #0x8]\n\t"   /* context->R1 */
                    "mov r1, #0x0200000\n\t"   /* CONTEXT_ARM */
                    "add r1, r1, #0x7\n\t"     /* CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_FLOATING_POINT */
                    "str r1, [r0]\n\t"         /* context->ContextFlags */
                    "str SP, [r0, #0x38]\n\t"  /* context->Sp */
                    "str LR, [r0, #0x40]\n\t"  /* context->Pc */
                    "mrs r1, CPSR\n\t"
                    "bfi r1, lr, #5, #1\n\t"   /* Thumb bit */
                    "str r1, [r0, #0x44]\n\t"  /* context->Cpsr */
                    "mov r1, #0\n\t"
                    "str r1, [r0, #0x4]\n\t"   /* context->R0 */
                    "str r1, [r0, #0x3c]\n\t"  /* context->Lr */
                    "add r0, #0x0c\n\t"
                    "stm r0, {r2-r12}\n\t"     /* context->R2..R12 */
#ifndef __SOFTFP__
                    "add r0, #0x44\n\t"        /* 0x50 - 0x0c */
                    "vstm r0, {d0-d15}\n\t"    /* context->D0-D15 */
#endif
                    "bx lr" )


/**********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    DWORD pc = context->Pc;

    dispatch->ScopeIndex = 0;
    dispatch->ControlPc  = pc;
    dispatch->ControlPcIsUnwound = (context->ContextFlags & CONTEXT_UNWOUND_TO_CALL) != 0;
    if (dispatch->ControlPcIsUnwound) pc -= 2;

    dispatch->FunctionEntry = RtlLookupFunctionEntry( pc, (DWORD_PTR *)&dispatch->ImageBase,
                                                      dispatch->HistoryTable );
    dispatch->LanguageHandler = RtlVirtualUnwind( type, dispatch->ImageBase, pc, dispatch->FunctionEntry,
                                                  context, &dispatch->HandlerData,
                                                  (ULONG_PTR *)&dispatch->EstablisherFrame, NULL );
    if (!context->Pc)
    {
        WARN( "exception data not found for pc %p, lr %p\n", (void *)pc, (void *)context->Lr );
        return STATUS_INVALID_DISPOSITION;
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           unwind_exception_handler
 *
 * Handler for exceptions happening while calling an unwind handler.
 */
EXCEPTION_DISPOSITION WINAPI unwind_exception_handler( EXCEPTION_RECORD *record, void *frame,
                                                       CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    DISPATCHER_CONTEXT *orig_dispatch = ((DISPATCHER_CONTEXT **)frame)[-2];

    /* copy the original dispatcher into the current one, except for the TargetIp */
    dispatch->ControlPc        = orig_dispatch->ControlPc;
    dispatch->ImageBase        = orig_dispatch->ImageBase;
    dispatch->FunctionEntry    = orig_dispatch->FunctionEntry;
    dispatch->EstablisherFrame = orig_dispatch->EstablisherFrame;
    dispatch->ContextRecord    = orig_dispatch->ContextRecord;
    dispatch->LanguageHandler  = orig_dispatch->LanguageHandler;
    dispatch->HandlerData      = orig_dispatch->HandlerData;
    dispatch->HistoryTable     = orig_dispatch->HistoryTable;
    dispatch->ScopeIndex       = orig_dispatch->ScopeIndex;
    TRACE( "detected collided unwind\n" );
    return ExceptionCollidedUnwind;
}

/**********************************************************************
 *           unwind_handler_wrapper
 */
#ifdef __WINE_PE_BUILD
extern DWORD WINAPI unwind_handler_wrapper( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch );
__ASM_GLOBAL_FUNC( unwind_handler_wrapper,
                   "push {r1,lr}\n\t"
                   ".seh_save_regs {r1,lr}\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler " __ASM_NAME("unwind_exception_handler") ", %except, %unwind\n\t"
                   "mov r3, r1\n\t"           /* dispatch */
                   "ldr r1, [r3, #0x0c]\n\t"  /* dispatch->EstablisherFrame */
                   "ldr r2, [r3, #0x14]\n\t"  /* dispatch->ContextRecord */
                   "ldr ip, [r3, #0x18]\n\t"  /* dispatch->LanguageHandler */
                   "blx ip\n\t"
                   "pop {r1,pc}\n\t" )
#else
static DWORD unwind_handler_wrapper( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch )
{
    struct
    {
        DISPATCHER_CONTEXT *dispatch;
        ULONG lr;
        EXCEPTION_REGISTRATION_RECORD frame;
    } frame;
    DWORD res;

    frame.frame.Handler = (PEXCEPTION_HANDLER)unwind_exception_handler;
    frame.dispatch = dispatch;
    __wine_push_frame( &frame.frame );
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    __wine_pop_frame( &frame.frame );
    return res;
}
#endif


/**********************************************************************
 *           call_unwind_handler
 *
 * Call a single unwind handler.
 */
static DWORD call_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
         dispatch->LanguageHandler, rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = unwind_handler_wrapper( rec, dispatch );
    TRACE( "handler %p returned %lx\n", dispatch->LanguageHandler, res );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


/**********************************************************************
 *           call_teb_unwind_handler
 *
 * Call a single unwind handler from the TEB chain.
 */
static DWORD call_teb_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch,
                                     EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, dispatch->ContextRecord, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE( "handler at %p returned %lu\n", teb_frame->Handler, res );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


/***********************************************************************
 *		call_handler_wrapper
 */
#ifdef __WINE_PE_BUILD
extern DWORD WINAPI call_handler_wrapper( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch );
__ASM_GLOBAL_FUNC( call_handler_wrapper,
                   "push {r4,lr}\n\t"
                   ".seh_save_regs {r4,lr}\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler " __ASM_NAME("nested_exception_handler") ", %except\n\t"
                   "mov r3, r2\n\t" /* dispatch */
                   "mov r2, r1\n\t" /* context */
                   "ldr r1, [r3, #0x0c]\n\t"  /* dispatch->EstablisherFrame */
                   "ldr ip, [r3, #0x18]\n\t"  /* dispatch->LanguageHandler */
                   "blx ip\n\t"
                   "pop {r4,pc}\n\t" )
#else
static DWORD call_handler_wrapper( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    EXCEPTION_REGISTRATION_RECORD frame;
    DWORD res;

    frame.Handler = (PEXCEPTION_HANDLER)nested_exception_handler;
    __wine_push_frame( &frame );
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, context, dispatch );
    __wine_pop_frame( &frame );
    return res;
}
#endif


/**********************************************************************
 *           call_handler
 *
 * Call a single exception handler.
 */
static DWORD call_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
           dispatch->LanguageHandler, rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = call_handler_wrapper( rec, context, dispatch );
    TRACE( "handler at %p returned %lu\n", dispatch->LanguageHandler, res );

    rec->ExceptionFlags &= EH_NONCONTINUABLE;
    return res;
}


/**********************************************************************
 *           call_teb_handler
 *
 * Call a single exception handler from the TEB chain.
 * FIXME: Handle nested exceptions.
 */
static DWORD call_teb_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                                  EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, context, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE( "handler at %p returned %lu\n", teb_frame->Handler, res );
    return res;
}


/**********************************************************************
 *           call_seh_handlers
 *
 * Call the SEH handlers.
 */
NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context, prev_context;
    NTSTATUS status;

    context = *orig_context;
    dispatch.TargetPc      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    prev_context = context;
    dispatch.NonVolatileRegisters = (BYTE *)&prev_context.R4;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return status;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            switch (call_handler( rec, orig_context, &dispatch ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EH_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG_PTR frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, (PVOID *)&dispatch.HandlerData, &frame, NULL );
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while (is_valid_frame( (ULONG_PTR)teb_frame ) && (DWORD)teb_frame < context.Sp)
        {
            TRACE( "found wine frame %p rsp %lx handler %p\n",
                    teb_frame, context.Sp, teb_frame->Handler );
            dispatch.EstablisherFrame = (DWORD)teb_frame;
            switch (call_teb_handler( rec, orig_context, &dispatch, teb_frame ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EH_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG_PTR frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, (PVOID *)&dispatch.HandlerData, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Sp == (DWORD)NtCurrentTeb()->Tib.StackBase) break;
        prev_context = context;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserExceptionDispatcher,
                   __ASM_SEH(".seh_custom 0xee,0x02\n\t")  /* MSFT_OP_CONTEXT */
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_EHABI(".save {sp}\n\t") /* Restore Sp last */
                   __ASM_EHABI(".pad #-(0x80 + 0x0c + 0x0c)\n\t") /* Move back across D0-D15, Cpsr, Fpscr, Padding, Pc, Lr and Sp */
                   __ASM_EHABI(".vsave {d8-d15}\n\t")
                   __ASM_EHABI(".pad #0x40\n\t") /* Skip past D0-D7 */
                   __ASM_EHABI(".pad #0x0c\n\t") /* Skip past Cpsr, Fpscr and Padding */
                   __ASM_EHABI(".save {lr, pc}\n\t")
                   __ASM_EHABI(".pad #0x08\n\t") /* Skip past R12 and Sp - Sp is restored last */
                   __ASM_EHABI(".save {r4-r11}\n\t")
                   __ASM_EHABI(".pad #0x14\n\t") /* Skip past ContextFlags and R0-R3 */
                   "add r0, sp, #0x1a0\n\t"     /* rec (context + 1) */
                   "mov r1, sp\n\t"             /* context */
                   "bl " __ASM_NAME("dispatch_exception") "\n\t"
                   "udf #1" )


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserApcDispatcher,
                   __ASM_SEH(".seh_custom 0xee,0x02\n\t")  /* MSFT_OP_CONTEXT */
                   "nop\n\t"
                   __ASM_SEH(".seh_stackalloc 0x18\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_EHABI(".save {sp}\n\t") /* Restore Sp last */
                   __ASM_EHABI(".pad #-(0x80 + 0x0c + 0x0c)\n\t") /* Move back across D0-D15, Cpsr, Fpscr, Padding, Pc, Lr and Sp */
                   __ASM_EHABI(".vsave {d8-d15}\n\t")
                   __ASM_EHABI(".pad #0x40\n\t") /* Skip past D0-D7 */
                   __ASM_EHABI(".pad #0x0c\n\t") /* Skip past Cpsr, Fpscr and Padding */
                   __ASM_EHABI(".save {lr, pc}\n\t")
                   __ASM_EHABI(".pad #0x08\n\t") /* Skip past R12 and Sp - Sp is restored last */
                   __ASM_EHABI(".save {r4-r11}\n\t")
                   __ASM_EHABI(".pad #0x2c\n\t") /* Skip past args, ContextFlags and R0-R3 */
                   "ldr r0, [sp, #0x04]\n\t"      /* arg1 */
                   "ldr r1, [sp, #0x08]\n\t"      /* arg2 */
                   "ldr r2, [sp, #0x0c]\n\t"      /* arg3 */
                   "ldr ip, [sp]\n\t"             /* func */
                   "blx ip\n\t"
                   "add r0, sp, #0x18\n\t"        /* context */
                   "ldr r1, [sp, #0x10]\n\t"      /* alertable */
                   "bl " __ASM_NAME("NtContinue") "\n\t"
                   "udf #1" )


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher,
                   __ASM_SEH(".seh_custom 0xee,0x01\n\t")  /* MSFT_OP_MACHINE_FRAME */
                   "nop\n\t"
                   __ASM_SEH(".seh_save_regs {lr}\n\t")
                   "nop\n\t"
                   __ASM_SEH(".seh_stackalloc 0xc\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_EHABI(".save {sp, pc}\n\t")
                   __ASM_EHABI(".save {lr}\n\t")
                   __ASM_EHABI(".pad #0x0c\n\t")
                   "ldr r0, [sp]\n\t"               /* args */
                   "ldr r1, [sp, #0x04]\n\t"        /* len */
                   "ldr r2, [sp, #0x08]\n\t"        /* id */
#ifdef __WINE_PE_BUILD
                   "mrc p15, 0, r3, c13, c0, 2\n\t" /* NtCurrentTeb() */
                   "ldr r3, [r3, 0x30]\n\t"         /* peb */
                   "ldr r3, [r3, 0x2c]\n\t"         /* peb->KernelCallbackTable */
                   "ldr ip, [r3, r2, lsl #2]\n\t"
                   "blx ip\n\t"
                   ".seh_handler " __ASM_NAME("user_callback_handler") ", %except\n\t"
#else
                   "bl " __ASM_NAME("dispatch_user_callback") "\n\t"
#endif
                   ".globl " __ASM_NAME("KiUserCallbackDispatcherReturn") "\n"
                   __ASM_NAME("KiUserCallbackDispatcherReturn") ":\n\t"
                   "mov r2, r0\n\t"  /* status */
                   "mov r1, #0\n\t"  /* ret_len */
                   "mov r0, r1\n\t"  /* ret_ptr */
                   "bl " __ASM_NAME("NtCallbackReturn") "\n\t"
                   "bl " __ASM_NAME("RtlRaiseStatus") "\n\t"
                   "udf #1" )



/**********************************************************************
 *           call_consolidate_callback
 *
 * Wrapper function to call a consolidate callback from a fake frame.
 * If the callback executes RtlUnwindEx (like for example done in C++ handlers),
 * we have to skip all frames which were already processed. To do that we
 * trick the unwinding functions into thinking the call came from somewhere
 * else. All CFI instructions are either DW_CFA_def_cfa_expression or
 * DW_CFA_expression, and the expressions have the following format:
 *
 * DW_OP_breg13; sleb128 <OFFSET>       | Load SP + struct member offset
 * [DW_OP_deref]                        | Dereference, only for CFA
 */
extern void * WINAPI call_consolidate_callback( CONTEXT *context,
                                                void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                EXCEPTION_RECORD *rec );
__ASM_GLOBAL_FUNC( call_consolidate_callback,
                   "push {r0-r2,lr}\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "sub sp, sp, #0x1a0\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "mov r1, r0\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "mov r0, sp\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "mov r2, #0x1a0\n\t"
                   __ASM_SEH(".seh_nop_w\n\t")
                   "bl " __ASM_NAME("memcpy") "\n\t"
                   __ASM_SEH(".seh_custom 0xee,0x02\n\t")  /* MSFT_OP_CONTEXT */
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_def_cfa 13, 0\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x04,0x7d,0xb8,0x00,0x06\n\t") /* DW_CFA_def_cfa_expression: DW_OP_breg13 + 56, DW_OP_deref */
                   __ASM_CFI(".cfi_escape 0x10,0x04,0x02,0x7d,0x14\n\t") /* DW_CFA_expression: R4 DW_OP_breg13 + 20 */
                   __ASM_CFI(".cfi_escape 0x10,0x05,0x02,0x7d,0x18\n\t") /* DW_CFA_expression: R5 DW_OP_breg13 + 24 */
                   __ASM_CFI(".cfi_escape 0x10,0x06,0x02,0x7d,0x1c\n\t") /* DW_CFA_expression: R6 DW_OP_breg13 + 28 */
                   __ASM_CFI(".cfi_escape 0x10,0x07,0x02,0x7d,0x20\n\t") /* DW_CFA_expression: R7 DW_OP_breg13 + 32 */
                   __ASM_CFI(".cfi_escape 0x10,0x08,0x02,0x7d,0x24\n\t") /* DW_CFA_expression: R8 DW_OP_breg13 + 36 */
                   __ASM_CFI(".cfi_escape 0x10,0x09,0x02,0x7d,0x28\n\t") /* DW_CFA_expression: R9 DW_OP_breg13 + 40 */
                   __ASM_CFI(".cfi_escape 0x10,0x0a,0x02,0x7d,0x2c\n\t") /* DW_CFA_expression: R10 DW_OP_breg13 + 44 */
                   __ASM_CFI(".cfi_escape 0x10,0x0b,0x02,0x7d,0x30\n\t") /* DW_CFA_expression: R11 DW_OP_breg13 + 48 */
                   __ASM_CFI(".cfi_escape 0x10,0x0e,0x03,0x7d,0xc0,0x00\n\t") /* DW_CFA_expression: LR DW_OP_breg13 + 64 (PC) */
                   /* Libunwind doesn't support the registers D8-D15 like this */
#if 0
                   __ASM_CFI(".cfi_escape 0x10,0x88,0x02,0x03,0x7d,0x90,0x01\n\t") /* DW_CFA_expression: D8 DW_OP_breg13 + 144 */
                   __ASM_CFI(".cfi_escape 0x10,0x89,0x02,0x03,0x7d,0x98,0x01\n\t") /* DW_CFA_expression: D9 DW_OP_breg13 + 152 */
                   __ASM_CFI(".cfi_escape 0x10,0x8a,0x02,0x03,0x7d,0xa0,0x01\n\t") /* DW_CFA_expression: D10 DW_OP_breg13 + 160 */
                   __ASM_CFI(".cfi_escape 0x10,0x8b,0x02,0x03,0x7d,0xa8,0x01\n\t") /* DW_CFA_expression: D11 DW_OP_breg13 + 168 */
                   __ASM_CFI(".cfi_escape 0x10,0x8c,0x02,0x03,0x7d,0xb0,0x01\n\t") /* DW_CFA_expression: D12 DW_OP_breg13 + 176 */
                   __ASM_CFI(".cfi_escape 0x10,0x8d,0x02,0x03,0x7d,0xb8,0x01\n\t") /* DW_CFA_expression: D13 DW_OP_breg13 + 184 */
                   __ASM_CFI(".cfi_escape 0x10,0x8e,0x02,0x03,0x7d,0xc0,0x01\n\t") /* DW_CFA_expression: D14 DW_OP_breg13 + 192 */
                   __ASM_CFI(".cfi_escape 0x10,0x8f,0x02,0x03,0x7d,0xc8,0x01\n\t") /* DW_CFA_expression: D15 DW_OP_breg13 + 200 */
#endif
                   /* These EHABI opcodes are to be read bottom up - they
                    * restore relevant registers from the CONTEXT. */
                   __ASM_EHABI(".save {sp}\n\t") /* Restore Sp last */
                   __ASM_EHABI(".pad #-(0x80 + 0x0c + 0x0c)\n\t") /* Move back across D0-D15, Cpsr, Fpscr, Padding, Pc, Lr and Sp */
                   __ASM_EHABI(".vsave {d8-d15}\n\t")
                   __ASM_EHABI(".pad #0x40\n\t") /* Skip past D0-D7 */
                   __ASM_EHABI(".pad #0x0c\n\t") /* Skip past Cpsr, Fpscr and Padding */
                   __ASM_EHABI(".save {lr, pc}\n\t")
                   __ASM_EHABI(".pad #0x08\n\t") /* Skip past R12 and Sp - Sp is restored last */
                   __ASM_EHABI(".save {r4-r11}\n\t")
                   __ASM_EHABI(".pad #0x14\n\t") /* Skip past ContextFlags and R0-R3 */

                   "ldrd r1, r2, [sp, #0x1a4]\n\t"
                   "mov r0, r2\n\t"
                   "blx r1\n\t"
                   "add sp, sp, #0x1ac\n\t"
                   "pop {pc}\n\t")



/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct MSVCRT_JUMP_BUFFER *jmp = (struct MSVCRT_JUMP_BUFFER *)rec->ExceptionInformation[0];
        int i;

        for (i = 4; i <= 11; i++)
            (&context->R4)[i-4] = (&jmp->R4)[i-4];
        context->Lr      = jmp->Pc;
        context->Sp      = jmp->Sp;
        context->Fpscr   = jmp->Fpscr;

        for (i = 0; i < 8; i++)
            context->D[8+i] = jmp->D[i];
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        rec->ExceptionInformation[10] = (ULONG_PTR)&context->R4;

        context->Pc = (DWORD)call_consolidate_callback( context, consolidate, rec );
    }

    /* hack: remove no longer accessible TEB frames */
    while (is_valid_frame( (ULONG_PTR)teb_frame ) && (DWORD)teb_frame < context->Sp)
    {
        TRACE( "removing TEB frame: %p\n", teb_frame );
        teb_frame = __wine_pop_frame( teb_frame );
    }

    TRACE( "returning to %lx stack %lx\n", context->Pc, context->Sp );
    NtContinue( context, FALSE );
}


/***********************************************************************
 *            RtlUnwindEx  (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT new_context;
    NTSTATUS status;
    DWORD i;

    RtlCaptureContext( context );
    new_context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Pc;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EH_UNWINDING | (end_frame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx end_frame=%p target_ip=%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%08Ix\n", i, rec->ExceptionInformation[i] );
    TRACE_CONTEXT( context );

    dispatch.TargetPc         = (ULONG_PTR)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;
    dispatch.NonVolatileRegisters = (BYTE *)&context->R4;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &new_context );
        if (status != STATUS_SUCCESS) raise_status( status, rec );

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (DWORD)end_frame))
            {
                ERR( "invalid end frame %lx/%p\n", dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (DWORD)end_frame) rec->ExceptionFlags |= EH_TARGET_UNWIND;
            if (call_unwind_handler( rec, &dispatch ) == ExceptionCollidedUnwind)
            {
                ULONG_PTR frame;

                *context = new_context = *dispatch.ContextRecord;
                dispatch.ContextRecord = context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &new_context, &dispatch.HandlerData, &frame,
                                  NULL );
                rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                goto unwind_done;
            }
            rec->ExceptionFlags &= ~EH_COLLIDED_UNWIND;
        }
        else  /* hack: call builtin handlers registered in the tib list */
        {
            DWORD backup_frame = dispatch.EstablisherFrame;
            while (is_valid_frame( (ULONG_PTR)teb_frame ) &&
                   (DWORD)teb_frame < new_context.Sp &&
                   (DWORD)teb_frame < (DWORD)end_frame)
            {
                TRACE( "found builtin frame %p handler %p\n", teb_frame, teb_frame->Handler );
                dispatch.EstablisherFrame = (DWORD)teb_frame;
                if (call_teb_unwind_handler( rec, &dispatch, teb_frame ) == ExceptionCollidedUnwind)
                {
                    ULONG_PTR frame;

                    teb_frame = __wine_pop_frame( teb_frame );

                    *context = new_context = *dispatch.ContextRecord;
                    dispatch.ContextRecord = context;
                    RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                      dispatch.ControlPc, dispatch.FunctionEntry,
                                      &new_context, &dispatch.HandlerData,
                                      &frame, NULL );
                    rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                    goto unwind_done;
                }
                teb_frame = __wine_pop_frame( teb_frame );
            }
            if ((DWORD)teb_frame == (DWORD)end_frame && (DWORD)end_frame < new_context.Sp) break;
            dispatch.EstablisherFrame = backup_frame;
        }

        if (dispatch.EstablisherFrame == (DWORD)end_frame) break;
        *context = new_context;
    }

    context->R0     = (DWORD)retval;
    context->Pc     = (DWORD)target_ip;
    RtlRestoreContext(context, rec);
}


extern LONG __C_ExecuteExceptionFilter(PEXCEPTION_POINTERS ptrs, PVOID frame,
                                       PEXCEPTION_FILTER filter,
                                       PUCHAR nonvolatile);
__ASM_GLOBAL_FUNC( __C_ExecuteExceptionFilter,
                   "push {r4-r11,lr}\n\t"
                   __ASM_EHABI(".save {r4-r11,lr}\n\t")
                   __ASM_SEH(".seh_save_regs_w {r4-r11,lr}\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")

                   __ASM_CFI(".cfi_def_cfa 13, 36\n\t")
                   __ASM_CFI(".cfi_offset r4, -36\n\t")
                   __ASM_CFI(".cfi_offset r5, -32\n\t")
                   __ASM_CFI(".cfi_offset r6, -28\n\t")
                   __ASM_CFI(".cfi_offset r7, -24\n\t")
                   __ASM_CFI(".cfi_offset r8, -20\n\t")
                   __ASM_CFI(".cfi_offset r9, -16\n\t")
                   __ASM_CFI(".cfi_offset r10, -12\n\t")
                   __ASM_CFI(".cfi_offset r11, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")

                   "ldm r3, {r4-r11,lr}\n\t"
                   "blx r2\n\t"
                   "pop {r4-r11,pc}\n\t" )

extern void __C_ExecuteTerminationHandler(BOOL abnormal, PVOID frame,
                                          PTERMINATION_HANDLER handler,
                                          PUCHAR nonvolatile);
/* This is, implementation wise, identical to __C_ExecuteExceptionFilter. */
__ASM_GLOBAL_FUNC( __C_ExecuteTerminationHandler,
                   "b " __ASM_NAME("__C_ExecuteExceptionFilter") "\n\t");

/*******************************************************************
 *              __C_specific_handler (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec,
                                                   void *frame,
                                                   CONTEXT *context,
                                                   struct _DISPATCHER_CONTEXT *dispatch )
{
    SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG i;
    DWORD ControlPc = dispatch->ControlPc;

    TRACE( "%p %p %p %p\n", rec, frame, context, dispatch );
    if (TRACE_ON(seh)) dump_scope_table( dispatch->ImageBase, table );

    if (dispatch->ControlPcIsUnwound)
        ControlPc -= 2;

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
            {
                PTERMINATION_HANDLER handler;

                if (table->ScopeRecord[i].JumpTarget) continue;

                if (rec->ExceptionFlags & EH_TARGET_UNWIND &&
                    dispatch->TargetPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                    dispatch->TargetPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
                {
                    break;
                }

                handler = (PTERMINATION_HANDLER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                dispatch->ScopeIndex = i+1;

                TRACE( "calling __finally %p frame %p\n", handler, frame );
                __C_ExecuteTerminationHandler( TRUE, frame, handler,
                                               dispatch->NonVolatileRegisters );
            }
        }
        return ExceptionContinueSearch;
    }

    for (i = dispatch->ScopeIndex; i < table->Count; i++)
    {
        if (ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
            ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
        {
            if (!table->ScopeRecord[i].JumpTarget) continue;
            if (table->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
            {
                EXCEPTION_POINTERS ptrs;
                PEXCEPTION_FILTER filter;

                filter = (PEXCEPTION_FILTER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                ptrs.ExceptionRecord = rec;
                ptrs.ContextRecord = context;
                TRACE( "calling filter %p ptrs %p frame %p\n", filter, &ptrs, frame );
                switch (__C_ExecuteExceptionFilter( &ptrs, frame, filter,
                                                    dispatch->NonVolatileRegisters ))
                {
                case EXCEPTION_EXECUTE_HANDLER:
                    break;
                case EXCEPTION_CONTINUE_SEARCH:
                    continue;
                case EXCEPTION_CONTINUE_EXECUTION:
                    return ExceptionContinueExecution;
                }
            }
            TRACE( "unwinding to target %lx\n", dispatch->ImageBase + table->ScopeRecord[i].JumpTarget );
            RtlUnwindEx( frame, (char *)dispatch->ImageBase + table->ScopeRecord[i].JumpTarget,
                         rec, 0, dispatch->ContextRecord, dispatch->HistoryTable );
        }
    }
    return ExceptionContinueSearch;
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlRaiseException,
                    "push {r0, lr}\n\t"
                    __ASM_EHABI(".save {r0, lr}\n\t")
                    __ASM_SEH(".seh_save_regs {r0, lr}\n\t")
                    "sub sp, sp, #0x1a0\n\t"  /* sizeof(CONTEXT) */
                    __ASM_EHABI(".pad #0x1a0\n\t")
                    __ASM_SEH(".seh_stackalloc 0x1a0\n\t")
                    __ASM_SEH(".seh_endprologue\n\t")
                    __ASM_CFI(".cfi_adjust_cfa_offset 424\n\t")
                    __ASM_CFI(".cfi_offset lr, -4\n\t")
                    "mov r0, sp\n\t"  /* context */
                    "bl " __ASM_NAME("RtlCaptureContext") "\n\t"
                    "ldr r0, [sp, #0x1a0]\n\t" /* rec */
                    "ldr r1, [sp, #0x1a4]\n\t"
                    "str r1, [sp, #0x3c]\n\t"  /* context->Lr */
                    "str r1, [sp, #0x40]\n\t"  /* context->Pc */
                    "mrs r2, CPSR\n\t"
                    "bfi r2, r1, #5, #1\n\t"   /* Thumb bit */
                    "str r2, [sp, #0x44]\n\t"  /* context->Cpsr */
                    "str r1, [r0, #12]\n\t"    /* rec->ExceptionAddress */
                    "add r1, sp, #0x1a8\n\t"
                    "str r1, [sp, #0x38]\n\t"  /* context->Sp */
                    "mov r1, sp\n\t"
                    "mrc p15, 0, r3, c13, c0, 2\n\t" /* NtCurrentTeb() */
                    "ldr r3, [r3, #0x30]\n\t"  /* peb */
                    "ldrb r2, [r3, #2]\n\t"    /* peb->BeingDebugged */
                    "cbnz r2, 1f\n\t"
                    "bl " __ASM_NAME("dispatch_exception") "\n"
                    "1:\tmov r2, #1\n\t"
                    "bl " __ASM_NAME("NtRaiseException") "\n\t"
                    "bl " __ASM_NAME("RtlRaiseStatus") )

/*************************************************************************
 *             RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "(%ld, %ld, %p, %p) stub!\n", skip, count, buffer, hash );
    return 0;
}

/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlUserThreadStart,
                   ".seh_endprologue\n\t"
                   "mov r2, r1\n\t"
                   "mov r1, r0\n\t"
                   "mov r0, #0\n\t"
                   "ldr ip, 1f\n\t"
                   "ldr ip, [ip]\n\t"
                   "blx ip\n"
                   "1:\t.long " __ASM_NAME("pBaseThreadInitThunk") "\n\t"
                   ".seh_handler " __ASM_NAME("call_unhandled_exception_handler") ", %except" )

/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    loader_init( context, (void **)&context->R0 );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context->R0, (void *)context->R1 );
    NtContinue( context, TRUE );
}

/***********************************************************************
 *           process_breakpoint
 */
__ASM_GLOBAL_FUNC( process_breakpoint,
                   ".seh_endprologue\n\t"
                   ".seh_handler process_breakpoint_handler, %except\n\t"
                   "udf #0xfe\n\t"
                   "bx lr\n"
                   "process_breakpoint_handler:\n\t"
                   "ldr r0, [r2, #0x40]\n\t" /* context->Pc */
                   "add r0, r0, #2\n\t"
                   "str r0, [r2, #0x40]\n\t"
                   "mov r0, #0\n\t"          /* ExceptionContinueExecution */
                   "bx lr" )

/***********************************************************************
 *		DbgUiRemoteBreakin   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUiRemoteBreakin,
                   ".seh_endprologue\n\t"
                   ".seh_handler DbgUiRemoteBreakin_handler, %except\n\t"
                   "mrc p15, 0, r0, c13, c0, 2\n\t" /* NtCurrentTeb() */
                   "ldr r0, [r0, #0x30]\n\t"        /* NtCurrentTeb()->Peb */
                   "ldrb r0, [r0, 0x02]\n\t"        /* peb->BeingDebugged */
                   "cbz r0, 1f\n\t"
                   "bl " __ASM_NAME("DbgBreakPoint") "\n"
                   "1:\tmov r0, #0\n\t"
                   "bl " __ASM_NAME("RtlExitUserThread") "\n"
                   "DbgUiRemoteBreakin_handler:\n\t"
                   "mov sp, r1\n\t"                 /* frame */
                   "b 1b" )

/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "udf #0xfe; bx lr; nop; nop; nop; nop" );

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "udf #0xfe; bx lr; nop; nop; nop; nop" );

#endif  /* __arm__ */
